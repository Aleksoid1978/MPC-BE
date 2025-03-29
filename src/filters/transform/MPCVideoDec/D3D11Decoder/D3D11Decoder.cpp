/*
*      Copyright (C) 2017-2019 Hendrik Leppkes
*      http://www.1f0.de
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along
*  with this program; if not, write to the Free Software Foundation, Inc.,
*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*  Adaptation for MPC-BE (C) 2021-2025 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
*/

#include "stdafx.h"

#include <moreuuids.h>
#include <dxva2_guids.h>
#include "DSUtil/DSUtil.h"
#include "DSUtil/DXVAState.h"
#include "D3D11Decoder.h"
#include "../MPCVideoDec.h"
#include "../DxgiUtils.h"

#include <dxgi1_2.h>

#define FF_D3D11_WORKAROUND_NVIDIA_HEVC_420P12 10 ///< Work around for Nvidia Direct3D11 Hevc 420p12 decoder

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C"
{
	#include <ExtLib/ffmpeg/libavcodec/avcodec.h>
	#include <ExtLib/ffmpeg/libavutil/opt.h>
	#include <ExtLib/ffmpeg/libavutil/pixdesc.h>
	#include <ExtLib/ffmpeg/libavutil/hwcontext_d3d11va.h>
	#include <ExtLib/ffmpeg/libavcodec/d3d11va.h>
}
#pragma warning(pop)

CD3D11Decoder::CD3D11Decoder(CMPCVideoDecFilter* pFilter)
  : m_pFilter(pFilter)
{
}

CD3D11Decoder::~CD3D11Decoder()
{
	DestroyDecoder(true);

	if (m_pAllocator)
		m_pAllocator->DecoderDestruct();
	SAFE_RELEASE(m_pAllocator);
}

HRESULT CD3D11Decoder::Init()
{
	if (!SysVersion::IsWin8orLater()) {
		DLog(L"CD3D11Decoder::Init() : D3D11 decoding requires Windows 8 or newer");
		return E_NOINTERFACE;
	}

	m_dxLib.d3d11lib = LoadLibraryW(L"d3d11.dll");
	if (!m_dxLib.d3d11lib) {
		DLog(L"CD3D11Decoder::Init() : Cannot open d3d11.dll");
		return E_FAIL;
	}

	m_dxLib.mD3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(m_dxLib.d3d11lib, "D3D11CreateDevice");
	if (!m_dxLib.mD3D11CreateDevice) {
		DLog(L"CD3D11Decoder::Init() : D3D11CreateDevice() not available");
		return E_FAIL;
	}

	return S_OK;
}

#define DXVA_SURFACE_BASE_ALIGN 16
static DWORD dxva_align_dimensions(AVCodecID codec, DWORD dim)
{
	int align = DXVA_SURFACE_BASE_ALIGN;

	// MPEG-2 needs higher alignment on Intel cards, and it doesn't seem to harm anything to do it for all cards.
	if (codec == AV_CODEC_ID_MPEG2VIDEO)
		align <<= 1;
	else if (codec == AV_CODEC_ID_HEVC || codec == AV_CODEC_ID_AV1)
		align = 128;

	return FFALIGN(dim, align);
}

static DXGI_FORMAT d3d11va_map_sw_to_hw_format(enum AVPixelFormat pix_fmt, bool nvidia)
{
	switch (pix_fmt)
	{
		case AV_PIX_FMT_YUV420P10:
		case AV_PIX_FMT_P010:
			return DXGI_FORMAT_P010;
		case AV_PIX_FMT_YUV420P12:
			return nvidia ? DXGI_FORMAT_P010 : DXGI_FORMAT_P016;
		case AV_PIX_FMT_YUV422P:
			return DXGI_FORMAT_YUY2;
		case AV_PIX_FMT_YUV422P10:
			return DXGI_FORMAT_Y210;
		case AV_PIX_FMT_YUV422P12:
			return DXGI_FORMAT_Y216;
		case AV_PIX_FMT_YUV444P:
			return DXGI_FORMAT_AYUV;
		case AV_PIX_FMT_YUV444P10:
			return DXGI_FORMAT_Y410;
		case AV_PIX_FMT_YUV444P12:
			return DXGI_FORMAT_Y416;
		case AV_PIX_FMT_NV12:
		default:
			return DXGI_FORMAT_NV12;
	}
}

static const D3D_FEATURE_LEVEL s_D3D11Levels[] = {
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
};

HRESULT CD3D11Decoder::CreateD3D11Device(UINT nDeviceIndex, ID3D11Device** ppDevice, DXGI_ADAPTER_DESC* pDesc)
{
	IDXGIFactory1* pDXGIFactory1 = CDXGIFactory1::GetInstance().GetFactory();
	if (!pDXGIFactory1) {
		return E_ABORT;
	}

	ID3D11Device* pD3D11Device = nullptr;

	CComPtr<IDXGIAdapter> pDXGIAdapter;
	HRESULT hr = pDXGIFactory1->EnumAdapters(nDeviceIndex, &pDXGIAdapter);
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::CreateD3D11Device() : Failed to enumerate a valid DXGI device");
		return hr;
	}

	// Create a device with video support, and BGRA support for Direct2D interoperability (drawing UI, etc)
	UINT nCreationFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	D3D_FEATURE_LEVEL d3dFeatureLevel;
	hr = m_dxLib.mD3D11CreateDevice(
		pDXGIAdapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
#ifdef _DEBUG
		nCreationFlags | D3D11_CREATE_DEVICE_DEBUG,
#else
		nCreationFlags,
#endif
		s_D3D11Levels,
		std::size(s_D3D11Levels),
		D3D11_SDK_VERSION,
		&pD3D11Device,
		&d3dFeatureLevel,
		nullptr);
#ifdef _DEBUG
	if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
		DLog(L"WARNING: D3D11 debugging messages will not be displayed");
		hr = m_dxLib.mD3D11CreateDevice(
			pDXGIAdapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			nCreationFlags,
			s_D3D11Levels,
			std::size(s_D3D11Levels),
			D3D11_SDK_VERSION,
			&pD3D11Device,
			&d3dFeatureLevel,
			nullptr);
	}
#endif
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::CreateD3D11Device() : Failed to create a D3D11 device with video support");
		return hr;
	}

	DLog(L"CD3D11Decoder::CreateD3D11Device() : Created D3D11 device with feature level %d.%d", d3dFeatureLevel >> 12, (d3dFeatureLevel >> 8) & 0xF);

	// enable multithreaded protection
	ID3D10Multithread* pMultithread = nullptr;
	hr = pD3D11Device->QueryInterface(&pMultithread);
	if (SUCCEEDED(hr)) {
		pMultithread->SetMultithreadProtected(TRUE);
		SAFE_RELEASE(pMultithread);
	}

	// store adapter info
	if (pDesc) {
		ZeroMemory(pDesc, sizeof(*pDesc));
		pDXGIAdapter->GetDesc(pDesc);
	}

	// return device
	*ppDevice = pD3D11Device;

	return S_OK;
}

HRESULT CD3D11Decoder::CreateD3D11Decoder(AVCodecContext* c)
{
	HRESULT hr = S_OK;
	AVD3D11VADeviceContext* pDeviceContext = (AVD3D11VADeviceContext*)((AVHWDeviceContext*)m_pDevCtx->data)->hwctx;

	// release the old decoder, it needs to be re-created
	SAFE_RELEASE(m_pDecoder);

	// find a decoder configuration
	GUID profileGUID = GUID_NULL;
	DXGI_FORMAT surface_format = d3d11va_map_sw_to_hw_format(c->sw_pix_fmt, m_pFilter->m_nPCIVendor == PCIV_nVidia);
	hr = FindVideoServiceConversion(c, c->codec_id, c->profile, surface_format, &profileGUID);
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::CreateD3D11Decoder() : No video service profile found");
		return hr;
	}

	// get decoder configuration
	D3D11_VIDEO_DECODER_DESC desc = {};
	desc.Guid = profileGUID;
	desc.OutputFormat = surface_format;
	desc.SampleWidth = c->coded_width;
	desc.SampleHeight = c->coded_height;

	D3D11_VIDEO_DECODER_CONFIG decoder_config = {};
	hr = FindDecoderConfiguration(c, &desc, &decoder_config);
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::CreateD3D11Decoder() : No valid video decoder configuration found");
		return hr;
	}

	m_DecoderConfig = decoder_config;

	// update surface properties
	m_dwSurfaceWidth = dxva_align_dimensions(c->codec_id, c->coded_width);
	m_dwSurfaceHeight = dxva_align_dimensions(c->codec_id, c->coded_height);
	m_SurfaceFormat = surface_format;

	ALLOCATOR_PROPERTIES properties;
	hr = m_pAllocator->GetProperties(&properties);
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::CreateD3D11Decoder() : Error get allocator properties");
		return hr;
	}

	m_dwSurfaceCount = properties.cBuffers;

	// allocate a new frames context for the dimensions and format
	hr = AllocateFramesContext(c, m_dwSurfaceWidth, m_dwSurfaceHeight, c->sw_pix_fmt, m_dwSurfaceCount, &m_pFramesCtx);
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::CreateD3D11Decoder() : Error allocating frames context");
		return hr;
	}

	// release any old output views and allocate memory for the new ones
	if (m_pOutputViews) {
		for (int i = 0; i < m_nOutputViews; i++) {
			SAFE_RELEASE(m_pOutputViews[i]);
		}
		av_freep(&m_pOutputViews);
	}

	m_pOutputViews = (ID3D11VideoDecoderOutputView**)av_calloc(m_dwSurfaceCount, sizeof(*m_pOutputViews));
	m_nOutputViews = m_dwSurfaceCount;

	// allocate output views for the frames
	AVD3D11VAFramesContext* pFramesContext = (AVD3D11VAFramesContext*)((AVHWFramesContext*)m_pFramesCtx->data)->hwctx;
	for (int i = 0; i < m_nOutputViews; i++) {
		D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC viewDesc = {};
		viewDesc.DecodeProfile = profileGUID;
		viewDesc.ViewDimension = D3D11_VDOV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.ArraySlice = i;

		hr = pDeviceContext->video_device->CreateVideoDecoderOutputView(pFramesContext->texture, &viewDesc, &m_pOutputViews[i]);
		if (FAILED(hr)) {
			DLog(L"CD3D11Decoder::CreateD3D11Decoder() : Failed to create video decoder output views");
			return hr;
		}
	}

	// create the decoder
	hr = pDeviceContext->video_device->CreateVideoDecoder(&desc, &decoder_config, &m_pDecoder);
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::CreateD3D11Decoder() : Failed to create video decoder object");
		return hr;
	}

	FillHWContext((AVD3D11VAContext*)c->hwaccel_context);

	m_DecoderGUID = profileGUID;
	DXVAState::SetActiveState(m_DecoderGUID, L"D3D11 Native");

	return S_OK;
}

HRESULT CD3D11Decoder::AllocateFramesContext(AVCodecContext* c, int width, int height, enum AVPixelFormat format, int nSurfaces, AVBufferRef** ppFramesCtx)
{
	ASSERT(c);
	ASSERT(m_pDevCtx);
	ASSERT(ppFramesCtx);

	// unref any old buffer
	av_buffer_unref(ppFramesCtx);

	// allocate a new frames context for the device context
	*ppFramesCtx = av_hwframe_ctx_alloc(m_pDevCtx);
	if (*ppFramesCtx == nullptr)
		return E_OUTOFMEMORY;

	AVHWFramesContext* pFrames = (AVHWFramesContext*)(*ppFramesCtx)->data;
	pFrames->format = AV_PIX_FMT_D3D11;
	switch (format) {
	case AV_PIX_FMT_YUV420P10:
		pFrames->sw_format = AV_PIX_FMT_P010;
		break;
	case AV_PIX_FMT_YUV420P:
	case AV_PIX_FMT_YUVJ420P:
		pFrames->sw_format = AV_PIX_FMT_NV12;
		break;
	default:
		pFrames->sw_format = format;
	}
	pFrames->width = width;
	pFrames->height = height;
	pFrames->initial_pool_size = nSurfaces;

	AVD3D11VAFramesContext* pFramesHWContext = (AVD3D11VAFramesContext*)pFrames->hwctx;
	pFramesHWContext->BindFlags |= D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE;
	pFramesHWContext->MiscFlags |= D3D11_RESOURCE_MISC_SHARED;

	int ret = av_hwframe_ctx_init(*ppFramesCtx);
	if (ret < 0) {
		av_buffer_unref(ppFramesCtx);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CD3D11Decoder::FindVideoServiceConversion(AVCodecContext* c, enum AVCodecID codec, int profile, DXGI_FORMAT surface_format, GUID* input)
{
	AVD3D11VADeviceContext* pDeviceContext = (AVD3D11VADeviceContext*)((AVHWDeviceContext*)m_pDevCtx->data)->hwctx;
	HRESULT hr = S_OK;

	const int depth = GetLumaBits(c->sw_pix_fmt);
	m_pFilter->m_bHighBitdepth = (depth == 10) && ((codec == AV_CODEC_ID_HEVC && (c->profile == AV_PROFILE_HEVC_MAIN_10 || c->profile == AV_PROFILE_HEVC_REXT))
												|| (codec == AV_CODEC_ID_VP9 && c->profile == AV_PROFILE_VP9_2)
												|| (codec == AV_CODEC_ID_AV1 && c->profile == AV_PROFILE_AV1_MAIN));

	UINT nProfiles = pDeviceContext->video_device->GetVideoDecoderProfileCount();
	std::vector<GUID> supportedDecoderGuids;

#ifdef DEBUG_OR_LOG
	CString dbgstr;
	dbgstr.Format(L"CD3D11Decoder::FindVideoServiceConversion() : Enumerating supported D3D11 modes[%u]:\n", nProfiles);
#endif
	for (UINT i = 0; i < nProfiles; i++) {
		GUID guidProfile;
		hr = pDeviceContext->video_device->GetVideoDecoderProfile(i, &guidProfile);
		if (FAILED(hr)) {
			DLog(L"CD3D11Decoder::FindVideoServiceConversion() : Error retrieving decoder profile");
			return hr;
		}

		bool supported = m_pFilter->IsSupportedDecoderMode(guidProfile);
#ifdef DEBUG_OR_LOG
		dbgstr.AppendFormat(L"        %s", GetDXVAModeStringAndName(guidProfile));
		supported ? dbgstr.Append(L" - supported\n") : dbgstr.Append(L"\n");
#endif
		if (supported) {
			if (guidProfile == DXVA2_ModeH264_E || guidProfile == DXVA2_ModeH264_F) {
				supportedDecoderGuids.insert(supportedDecoderGuids.cbegin(), guidProfile);
			} else {
				supportedDecoderGuids.emplace_back(guidProfile);
			}
		}
	}
#ifdef DEBUG_OR_LOG
	DLog(dbgstr);
#endif

	if (!supportedDecoderGuids.empty()) {
		for (const auto& guid : supportedDecoderGuids) {
			DLog(L"    => Attempt : %s", GetDXVAModeString(guid));

			if (DXVA2_H264_VLD_Intel == guid) {
				const int width_mbs = m_dwSurfaceWidth / 16;
				const int height_mbs = m_dwSurfaceHeight / 16;
				const int max_ref_frames_dpb41 = std::min(11, 32768 / (width_mbs * height_mbs));
				if (c->refs > max_ref_frames_dpb41) {
					DLog(L"    => Too many reference frames for Intel H.264 ClearVideo decoder, skip");
					continue;
				}
			}

			BOOL bSupported = FALSE;
			hr = pDeviceContext->video_device->CheckVideoDecoderFormat(&guid, surface_format, &bSupported);
			if (SUCCEEDED(hr) && bSupported) {
				DLog(L"    => Use : %s", GetDXVAModeString(guid));
				*input = guid;
				return S_OK;
			}
		}
	}

	DLog(L"CD3D11Decoder::FindVideoServiceConversion() : no supported format found");

	return E_FAIL;
}

HRESULT CD3D11Decoder::FindDecoderConfiguration(AVCodecContext* c, const D3D11_VIDEO_DECODER_DESC* desc, D3D11_VIDEO_DECODER_CONFIG* pConfig)
{
	AVD3D11VADeviceContext* pDeviceContext = (AVD3D11VADeviceContext*)((AVHWDeviceContext*)m_pDevCtx->data)->hwctx;
	HRESULT hr = S_OK;

	UINT nConfig = 0;
	hr = pDeviceContext->video_device->GetVideoDecoderConfigCount(desc, &nConfig);
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::FindDecoderConfiguration() : Unable to retreive decoder configuration count");
		return E_FAIL;
	}

	int best_score = -1;
	D3D11_VIDEO_DECODER_CONFIG best_config = {};
	for (UINT i = 0; i < nConfig; i++) {
		D3D11_VIDEO_DECODER_CONFIG config = {};
		hr = pDeviceContext->video_device->GetVideoDecoderConfig(desc, i, &config);
		if (FAILED(hr))
			continue;

		DLog(L"CD3D11Decoder::FindDecoderConfiguration() : Configuration Record %d: ConfigBitstreamRaw = %d", i, config.ConfigBitstreamRaw);

		int score;
		if (config.ConfigBitstreamRaw == 1)
			score = 1;
		else if (c->codec_id == AV_CODEC_ID_H264 && config.ConfigBitstreamRaw == 2)
			score = 2;
		else if (c->codec_id == AV_CODEC_ID_VP9) // hack for broken AMD drivers
			score = 0;
		else
			continue;
		if (IsEqualGUID(config.guidConfigBitstreamEncryption, DXVA2_NoEncrypt))
			score += 16;
		if (score > best_score) {
			best_score = score;
			best_config = config;
		}
	}

	if (best_score < 0) {
		DLog(L"CD3D11Decoder::FindDecoderConfiguration() : No matching configuration available");
		return E_FAIL;
	}

	*pConfig = best_config;
	return S_OK;
}

HRESULT CD3D11Decoder::ReInitD3D11Decoder(AVCodecContext* c)
{
	HRESULT hr = S_OK;

	// Don't allow decoder creation during first init
	if (m_pFilter->m_bInInit)
		return S_FALSE;

	// sanity check that we have a device
	if (m_pDevCtx == nullptr)
		return E_FAIL;

	// we need an allocator at this point
	if (m_pAllocator == nullptr)
		return E_FAIL;

	if (m_pDecoder == nullptr || m_dwSurfaceWidth != dxva_align_dimensions(c->codec_id, c->coded_width) ||
			m_dwSurfaceHeight != dxva_align_dimensions(c->codec_id, c->coded_height) ||
			m_SurfaceFormat != d3d11va_map_sw_to_hw_format(c->sw_pix_fmt, m_pFilter->m_nPCIVendor == PCIV_nVidia)) {
		AVD3D11VADeviceContext* pDeviceContext = (AVD3D11VADeviceContext*)((AVHWDeviceContext*)m_pDevCtx->data)->hwctx;
		DLog(L"CD3D11Decoder::ReInitD3D11Decoder() : No D3D11 Decoder or video dimensions/format changed -> Re-Allocating resources");

		const auto bChangeFormat = m_SurfaceFormat != DXGI_FORMAT_UNKNOWN && m_SurfaceFormat != d3d11va_map_sw_to_hw_format(c->sw_pix_fmt, m_pFilter->m_nPCIVendor == PCIV_nVidia);

		if (m_pDecoder)
			avcodec_flush_buffers(c);

		pDeviceContext->lock(pDeviceContext->lock_ctx);
		hr = CreateD3D11Decoder(c);
		pDeviceContext->unlock(pDeviceContext->lock_ctx);
		if (FAILED(hr))
			return hr;

		if (bChangeFormat) {
			m_pFilter->ChangeOutputMediaFormat(2);
		}

		// decommit the allocator
		m_pAllocator->Decommit();

		// verify we were able to decommit all its resources
		if (m_pAllocator->DecommitInProgress()) {
			DLog(L"CD3D11Decoder::ReInitD3D11Decoder() : WARNING! D3D11 Allocator is still busy, trying to flush downstream");
			m_pFilter->GetOutputPin()->GetConnected()->BeginFlush();
			m_pFilter->GetOutputPin()->GetConnected()->EndFlush();
			if (m_pAllocator->DecommitInProgress()) {
				DLog(L"CD3D11Decoder::ReInitD3D11Decoder() : WARNING! Flush had no effect, decommit of the allocator still not complete");
				m_pAllocator->ForceDecommit();
			} else {
				DLog(L"CD3D11Decoder::ReInitD3D11Decoder() : Flush was successfull, decommit completed!");
			}
		}

		// re-commit it to update its frame reference
		m_pAllocator->Commit();
	}

	return S_OK;
}

void CD3D11Decoder::DestroyDecoder(const bool bFull)
{
	if (m_pOutputViews) {
		for (int i = 0; i < m_nOutputViews; i++) {
			SAFE_RELEASE(m_pOutputViews[i]);
		}
		av_freep(&m_pOutputViews);
		m_nOutputViews = 0;
	}

	SAFE_RELEASE(m_pDecoder);
	av_buffer_unref(&m_pFramesCtx);

	m_DecoderGUID = GUID_NULL;

	if (bFull) {
		av_buffer_unref(&m_pDevCtx);

		if (m_dxLib.d3d11lib) {
			FreeLibrary(m_dxLib.d3d11lib);
			m_dxLib.d3d11lib = nullptr;
		}
	}
}

void CD3D11Decoder::PostInitDecoder(AVCodecContext* c)
{
	HRESULT hr = S_OK;
	DLog(L"CD3D11Decoder::PostInitDecoder()");

	// Destroy old decoder
	DestroyDecoder(false);

	// Ensure software pixfmt is set, so hardware accels can use it immediately
	if (c->sw_pix_fmt == AV_PIX_FMT_NONE && c->pix_fmt != AV_PIX_FMT_DXVA2_VLD && c->pix_fmt != AV_PIX_FMT_D3D11) {
		c->sw_pix_fmt = c->pix_fmt;
	}

	// initialize surface format to ensure the default media type is set properly
	m_SurfaceFormat = d3d11va_map_sw_to_hw_format(c->sw_pix_fmt, m_pFilter->m_nPCIVendor == PCIV_nVidia);
	m_dwSurfaceWidth = dxva_align_dimensions(c->codec_id, c->coded_width);
	m_dwSurfaceHeight = dxva_align_dimensions(c->codec_id, c->coded_height);
}

HRESULT CD3D11Decoder::PostConnect(AVCodecContext* c, IPin* pPin)
{
	DLog(L"CD3D11Decoder::PostConnect()");

	if (m_pAllocator) {
		m_pAllocator->DecoderDestruct();
		SAFE_RELEASE(m_pAllocator);
	}

	CComPtr<ID3D11DecoderConfiguration> pD3D11DecoderConfiguration;
	HRESULT hr = pPin->QueryInterface(&pD3D11DecoderConfiguration);
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::PostConnect() : ID3D11DecoderConfiguration not available");
		return hr;
	}

	// free the decoder to force a re-init down the line
	SAFE_RELEASE(m_pDecoder);

	// and the old device
	av_buffer_unref(&m_pDevCtx);

	// device id
	UINT nDevice = pD3D11DecoderConfiguration->GetD3D11AdapterIndex();

	// create the device
	ID3D11Device* pD3D11Device = nullptr;
	hr = CreateD3D11Device(nDevice, &pD3D11Device, &m_AdapterDesc);
	if (FAILED(hr)) {
		return E_FAIL;
	}

	// allocate and fill device context
	m_pDevCtx = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
	AVD3D11VADeviceContext* pDeviceContext = (AVD3D11VADeviceContext*)((AVHWDeviceContext*)m_pDevCtx->data)->hwctx;
	pDeviceContext->device = pD3D11Device;

	// finalize the context
	int ret = av_hwdevice_ctx_init(m_pDevCtx);
	if (ret < 0) {
		av_buffer_unref(&m_pDevCtx);
		return E_FAIL;
	}

	// enable multithreaded protection
	ID3D10Multithread* pMultithread = nullptr;
	hr = pDeviceContext->device_context->QueryInterface(&pMultithread);
	if (SUCCEEDED(hr)) {
		pMultithread->SetMultithreadProtected(TRUE);
		SAFE_RELEASE(pMultithread);
	}

	// check if the connection supports native mode
	CMediaType& mt = m_pFilter->m_pOutput->CurrentMediaType();
	if ((m_SurfaceFormat == DXGI_FORMAT_NV12 && mt.subtype != MEDIASUBTYPE_NV12) ||
		(m_SurfaceFormat == DXGI_FORMAT_P010 && mt.subtype != MEDIASUBTYPE_P010) ||
		(m_SurfaceFormat == DXGI_FORMAT_P016 && mt.subtype != MEDIASUBTYPE_P016) ||
		(m_SurfaceFormat == DXGI_FORMAT_YUY2 && mt.subtype != MEDIASUBTYPE_YUY2) ||
		(m_SurfaceFormat == DXGI_FORMAT_Y210 && mt.subtype != MEDIASUBTYPE_Y210) ||
		(m_SurfaceFormat == DXGI_FORMAT_Y216 && mt.subtype != MEDIASUBTYPE_Y216) ||
		(m_SurfaceFormat == DXGI_FORMAT_AYUV && mt.subtype != MEDIASUBTYPE_AYUV) ||
		(m_SurfaceFormat == DXGI_FORMAT_Y410 && mt.subtype != MEDIASUBTYPE_Y410) ||
		(m_SurfaceFormat == DXGI_FORMAT_Y416 && mt.subtype != MEDIASUBTYPE_Y416)) {
		DLog(L"-> Connection is not the appropriate pixel format '%s' for D3D11 Native", GetGUIDString(mt.subtype));
		return E_FAIL;
	}

	// verify hardware support
	GUID guidConversion = GUID_NULL;
	hr = FindVideoServiceConversion(c, c->codec_id, c->profile, m_SurfaceFormat, &guidConversion);
	if (FAILED(hr)) {
		return hr;
	}

	// get decoder configuration
	D3D11_VIDEO_DECODER_DESC desc = {};
	desc.Guid = guidConversion;
	desc.OutputFormat = m_SurfaceFormat;
	desc.SampleWidth = c->coded_width;
	desc.SampleHeight = c->coded_height;

	D3D11_VIDEO_DECODER_CONFIG decoder_config = {};
	hr = FindDecoderConfiguration(c, &desc, &decoder_config);
	if (FAILED(hr)) {
		return hr;
	}

	// test creating a texture
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = FFALIGN(c->coded_width, 2);
	texDesc.Height = FFALIGN(c->coded_height, 2);
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 20;
	texDesc.Format = m_SurfaceFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	hr = pD3D11Device->CreateTexture2D(&texDesc, nullptr, nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	// Notice the connected pin that we're sending D3D11 textures
	hr = pD3D11DecoderConfiguration->ActivateD3D11Decoding(pDeviceContext->device, pDeviceContext->device_context, pDeviceContext->lock_ctx, 0);
	DLogIf(FAILED(hr), L"CD3D11Decoder::PostConnect() : ID3D11DecoderConfiguration::ActivateD3D11Decoding() failed");

	return hr;
}

void CD3D11Decoder::FillHWContext(AVD3D11VAContext* ctx)
{
	AVD3D11VADeviceContext* pDeviceContext = (AVD3D11VADeviceContext*)((AVHWDeviceContext*)m_pDevCtx->data)->hwctx;

	ctx->decoder = m_pDecoder;
	ctx->video_context = pDeviceContext->video_context;
	ctx->cfg = &m_DecoderConfig;
	ctx->surface_count = m_nOutputViews;
	ctx->surface = m_pOutputViews;

	ctx->context_mutex = pDeviceContext->lock_ctx;

	ctx->workaround = 0;

	if (m_pFilter->m_nPCIVendor == PCIV_nVidia) {
		ctx->workaround = FF_D3D11_WORKAROUND_NVIDIA_HEVC_420P12;
	}
}

void CD3D11Decoder::AdditionaDecoderInit(AVCodecContext* c)
{
	AVD3D11VAContext* ctx = av_d3d11va_alloc_context();

	if (m_pDecoder) {
		FillHWContext(ctx);
	}

	c->thread_count = 1;
	c->hwaccel_context = ctx;
	c->get_format = get_d3d11_format;
	c->get_buffer2 = get_d3d11_buffer;
	c->opaque = this;

	c->slice_flags |= SLICE_FLAG_ALLOW_FIELD;

	// disable error concealment in hwaccel mode, it doesn't work either way
	c->error_concealment = 0;
	av_opt_set_int(c, "enable_er", 0, AV_OPT_SEARCH_CHILDREN);
}

HRESULT CD3D11Decoder::DeliverFrame()
{
	CheckPointer(m_pFilter->m_pFrame, S_FALSE);
	AVFrame* pFrame = m_pFilter->m_pFrame;

	IMediaSample* pSample = (IMediaSample*)pFrame->data[3];
	CheckPointer(pSample, S_FALSE);

	REFERENCE_TIME rtStop, rtStart;
	m_pFilter->GetFrameTimeStamp(pFrame, rtStart, rtStop);
	m_pFilter->UpdateFrameTime(rtStart, rtStop);

	if (rtStart < 0) {
		return S_OK;
	}

	DXVA2_ExtendedFormat dxvaExtFormat = m_pFilter->GetDXVA2ExtendedFormat(m_pFilter->m_pAVCtx, pFrame);

	int w, h;
	m_pFilter->GetPictSize(w, h);
	m_pFilter->ReconnectOutput(w, h, false, m_pFilter->GetFrameDuration(), &dxvaExtFormat);

	m_pFilter->SetTypeSpecificFlags(pSample);
	pSample->SetTime(&rtStart, &rtStop);
	pSample->SetMediaTime(nullptr, nullptr);

	bool bSizeChanged = false;
	LONG biWidth, biHeight = 0;

	CMediaType& mt = m_pFilter->GetOutputPin()->CurrentMediaType();
	if (m_pFilter->m_bSendMediaType) {
		AM_MEDIA_TYPE* sendmt = CreateMediaType(&mt);
		BITMAPINFOHEADER* pBMI = nullptr;
		if (sendmt->formattype == FORMAT_VideoInfo) {
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)sendmt->pbFormat;
			pBMI = &vih->bmiHeader;
		} else if (sendmt->formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)sendmt->pbFormat;
			pBMI = &vih2->bmiHeader;
		} else {
			return E_FAIL;
		}

		biWidth = pBMI->biWidth;
		biHeight = abs(pBMI->biHeight);
		pSample->SetMediaType(sendmt);
		DeleteMediaType(sendmt);
		m_pFilter->m_bSendMediaType = false;
		bSizeChanged = true;
	}

	m_pFilter->AddFrameSideData(pSample, pFrame);

	HRESULT hr = m_pFilter->GetOutputPin()->Deliver(pSample);

	if (bSizeChanged && biHeight) {
		m_pFilter->NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(biWidth, abs(biHeight)), 0);
	}

	return hr;
}

HRESULT CD3D11Decoder::InitAllocator(IMemAllocator** ppAlloc)
{
	if (m_pAllocator == nullptr) {
		HRESULT hr = S_FALSE;
		m_pAllocator = new(std::nothrow) CD3D11SurfaceAllocator(this, &hr);
		if (!m_pAllocator) {
			return E_OUTOFMEMORY;
		}
		if (FAILED(hr)) {
			SAFE_DELETE(m_pAllocator);
			return hr;
		}

		// Hold a reference on the allocator
		m_pAllocator->AddRef();
	}

	return m_pAllocator->QueryInterface(__uuidof(IMemAllocator), (void**)ppAlloc);
}

enum AVPixelFormat CD3D11Decoder::get_d3d11_format(struct AVCodecContext* s, const enum AVPixelFormat* pix_fmts)
{
	CD3D11Decoder* pDec = (CD3D11Decoder*)s->opaque;
	const enum AVPixelFormat* p;
	for (p = pix_fmts; *p != -1; p++) {
		const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(*p);

		if (!desc || !(desc->flags & AV_PIX_FMT_FLAG_HWACCEL))
			break;

		if (*p == AV_PIX_FMT_D3D11) {
			HRESULT hr = pDec->ReInitD3D11Decoder(s);
			if (FAILED(hr)) {
				DLog(L"CD3D11Decoder::get_d3d11_format() : ReInitD3D11Decoder() failed");
				pDec->m_pFilter->m_bFailD3D11Decode = TRUE;
				continue;
			} else {
				break;
			}
		}
	}
	return *p;
}

int CD3D11Decoder::get_d3d11_buffer(struct AVCodecContext* c, AVFrame* frame, int flags)
{
	CD3D11Decoder* pDec = (CD3D11Decoder*)c->opaque;

	if (frame->format != AV_PIX_FMT_D3D11) {
		DLog(L"CD3D11Decoder::get_d3d11_buffer() : D3D11 buffer request, but not D3D11 pixfmt");
		return -1;
	}

	if (!pDec->m_pFilter->CheckDXVACompatible(c->codec_id, c->sw_pix_fmt, c->profile)) {
		DLog(L"CD3D11Decoder::get_d3d11_buffer() : format not compatible with DXVA");
		pDec->m_pFilter->m_bDXVACompatible = false;
		return -1;
	}

	HRESULT hr = pDec->ReInitD3D11Decoder(c);
	if (FAILED(hr)) {
		DLog(L"CD3D11Decoder::get_d3d11_buffer() : ReInitD3D11Decoder() failed");
		pDec->m_pFilter->m_bFailD3D11Decode = TRUE;
		return -1;
	}

	if (pDec->m_pAllocator) {
		IMediaSample* pSample = nullptr;
		hr = pDec->m_pAllocator->GetBuffer(&pSample, nullptr, nullptr, 0);
		if (SUCCEEDED(hr)) {
			CD3D11MediaSample* pD3D11Sample = dynamic_cast<CD3D11MediaSample*>(pSample);

			// fill the frame from the sample, including a reference to the sample
			pD3D11Sample->GetAVFrameBuffer(frame);

			frame->width = c->coded_width;
			frame->height = c->coded_height;

			// the frame holds the sample now, can release the direct interface
			pD3D11Sample->Release();
			return 0;
		}
	}

	return -1;
}
