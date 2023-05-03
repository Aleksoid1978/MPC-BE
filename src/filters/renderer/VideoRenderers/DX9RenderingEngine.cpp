/*
 * (C) 2006-2023 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#pragma warning(push)
#pragma warning(disable: 5033) // warning C5033: 'register' is no longer a supported storage class
#include <ExtLib/lcms2/include/lcms2.h>
#pragma warning(pop)
#include <mfobjects.h>
#include <DirectXPackedVector.h>
#include "Dither.h"
#include "DX9RenderingEngine.h"
#include "apps/mplayerc/resource.h"

#define RELEASE_PTR_ARRAY(a) for (auto& item : a) { item.Release(); }

#pragma pack(push, 1)
template<unsigned texcoords>
struct MYD3DVERTEX {
	float x, y, z, rhw;
	struct {
		float u, v;
	} t[texcoords];
};

template<>
struct MYD3DVERTEX<0> {
	float x, y, z, rhw;
	DWORD Diffuse;
};
#pragma pack(pop)

template<unsigned texcoords>
static void AdjustQuad(MYD3DVERTEX<texcoords>* v, double dx, double dy)
{
	for (int i = 0; i < 4; i++) {
		v[i].x -= 0.5f;
		v[i].y -= 0.5f;

		for (int j = 0; j < max(texcoords - 1, 1); j++) {
			v[i].t[j].u -= (float)(0.5f*dx);
			v[i].t[j].v -= (float)(0.5f*dy);
		}

		if (texcoords > 1) {
			v[i].t[texcoords - 1].u -= 0.5f;
			v[i].t[texcoords - 1].v -= 0.5f;
		}
	}
}

template<unsigned texcoords>
static HRESULT TextureBlt(IDirect3DDevice9* pD3DDev, MYD3DVERTEX<texcoords> v[4], D3DTEXTUREFILTERTYPE filter)
{
	if (!pD3DDev) {
		return E_POINTER;
	}

	DWORD FVF = 0;

	switch (texcoords) {
	case 1: FVF = D3DFVF_TEX1; break;
	case 2: FVF = D3DFVF_TEX2; break;
	case 3: FVF = D3DFVF_TEX3; break;
	case 4: FVF = D3DFVF_TEX4; break;
	case 5: FVF = D3DFVF_TEX5; break;
	case 6: FVF = D3DFVF_TEX6; break;
	case 7: FVF = D3DFVF_TEX7; break;
	case 8: FVF = D3DFVF_TEX8; break;
	default:
		return E_FAIL;
	}

	HRESULT hr;

	hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);

	for (unsigned i = 0; i < texcoords; i++) {
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MAGFILTER, filter);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MINFILTER, filter);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | FVF);
	//hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));
	std::swap(v[2], v[3]);
	hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));

	for (unsigned i = 0; i < texcoords; i++) {
		pD3DDev->SetTexture(i, nullptr);
	}

	return S_OK;
}


using namespace DSObjects;

CDX9RenderingEngine::CDX9RenderingEngine(HWND hWnd, HRESULT& hr, CString *_pError)
	: CAllocatorPresenterImpl(hWnd, hr, _pError)
{
	m_hDxva2Lib = LoadLibraryW(L"dxva2.dll");
	DLogIf(!m_hDxva2Lib, L"Failed to load dxva2.dll");
}

CDX9RenderingEngine::~CDX9RenderingEngine()
{
	if (m_hDxva2Lib) {
		FreeLibrary(m_hDxva2Lib);
	}
}

void CDX9RenderingEngine::InitRenderingEngine()
{
	// Get the device caps
	ZeroMemory(&m_Caps, sizeof(m_Caps));
	m_pDevice9Ex->GetDeviceCaps(&m_Caps);

	// Define the shader profile.
	if (m_Caps.PixelShaderVersion >= D3DPS_VERSION(3, 0)) {
		m_ShaderProfile = "ps_3_0";
	} else {
		m_ShaderProfile = nullptr;
	}

	// Initialize the pixel shader compiler
	m_pPSC.reset(DNew CPixelShaderCompiler(m_pDevice9Ex, true));
}

void CDX9RenderingEngine::CleanupRenderingEngine()
{
	m_pPSC.reset();

	RELEASE_PTR_ARRAY(m_pResizerPixelShaders);

	CleanupFinalPass();

	for (auto& Shader : m_pCustomScreenSpacePixelShaders) {
		Shader.m_pPixelShader.Release();
	}

	for (auto& Shader : m_pCustomPixelShaders) {
		Shader.m_pPixelShader.Release();
	}

	m_pRotateTexture.Release();
	RELEASE_PTR_ARRAY(m_pFrameTextures);
	RELEASE_PTR_ARRAY(m_pScreenSpaceTextures);
	m_pResizeTexture.Release();

	m_pPSCorrection.Release();
	m_pConvertToInterlacePixelShader.Release();

	FreeVideoSurfaces();
}

HRESULT CDX9RenderingEngine::CreateVideoSurfaces()
{
	HRESULT hr;

	// Free previously allocated video surfaces
	FreeVideoSurfaces();

	// Free previously allocated temporary video textures, because the native video size might have been changed!
	RELEASE_PTR_ARRAY(m_pFrameTextures);
	m_pRotateTexture.Release();

	if (m_D3D9VendorId == PCIV_Intel) {
		// on Intel EVR-Mixer can work with X8R8G8B8 surface only
		m_VideoBufferFmt = D3DFMT_X8R8G8B8;
	}
	else if (m_D3D9VendorId == PCIV_nVidia && m_nativeVideoSize.cy == 1088 && m_SurfaceFmt == D3DFMT_A16B16G16R16F) {
		// fix Nvidia driver bug ('Integer division by zero' in nvd3dum.dll)
		m_VideoBufferFmt = D3DFMT_A2R10G10B10;
	}
	else {
		m_VideoBufferFmt = m_SurfaceFmt;
	}
	m_strMixerOutputFmt = GetD3DFormatStr(m_VideoBufferFmt);

	for (unsigned i = 0; i < m_nSurfaces; i++) {
		if (FAILED(hr = m_pDevice9Ex->CreateTexture(
							m_nativeVideoSize.cx, m_nativeVideoSize.cy, 1,
							D3DUSAGE_RENDERTARGET, m_VideoBufferFmt,
							D3DPOOL_DEFAULT, &m_pVideoTextures[i], nullptr))) {
			return hr;
		}

		if (FAILED(hr = m_pVideoTextures[i]->GetSurfaceLevel(0, &m_pVideoSurfaces[i]))) {
			return hr;
		}
	}

	hr = m_pDevice9Ex->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);

	return S_OK;
}

void CDX9RenderingEngine::FreeVideoSurfaces()
{
	for (unsigned i = 0; i < m_nSurfaces; i++) {
		m_pVideoTextures[i].Release();
		m_pVideoSurfaces[i].Release();
	}
}

void CDX9RenderingEngine::FreeScreenTextures()
{
	RELEASE_PTR_ARRAY(m_pScreenSpaceTextures);
}

HRESULT CDX9RenderingEngine::RenderVideo(IDirect3DSurface9* pRenderTarget, const CRect& srcRect, const CRect& destRect)
{
	if (destRect.IsRectEmpty()) {
		return S_OK;
	}

	// Return if the video texture is not initialized
	if (m_pVideoTextures[m_iCurSurface] == nullptr) {
		return E_ABORT;
	}

	HRESULT hr = S_OK;

	// Initialize the processing pipeline
	bool bCustomPixelShaders = false;
	bool bCustomScreenSpacePixelShaders = false;
	bool bFinalPass = false;
	int iRotation = m_iRotation;
	bool bFlip = m_bFlip;

	{
		unsigned screenSpacePassCount = 0;

		// Final pass. Must be initialized first!
		hr = InitFinalPass();
		if (SUCCEEDED(hr)) {
			bFinalPass = m_bFinalPass;
		} else {
			bFinalPass = false;
		}

		if (bFinalPass) {
			screenSpacePassCount++;
		}

		if (!pRenderTarget) {
			screenSpacePassCount++;
		}

		// Custom screen space pixel shaders
		bCustomScreenSpacePixelShaders = !m_pCustomScreenSpacePixelShaders.empty();
		if (bCustomScreenSpacePixelShaders) {
			screenSpacePassCount += m_pCustomScreenSpacePixelShaders.size();
		}

		// Custom pixel shaders
		bCustomPixelShaders = !m_pCustomPixelShaders.empty();
		hr = InitVideoTextures();
		if (FAILED(hr)) {
			bCustomPixelShaders = false;
		}

		hr = InitScreenSpaceTextures(screenSpacePassCount);
		if (FAILED(hr)) {
			bCustomScreenSpacePixelShaders = false;
			bFinalPass = false;
		}
	}

	// Apply the custom pixel shaders if there are any. Result: pVideoTexture
	CComPtr<IDirect3DTexture9> pVideoTexture = m_pVideoTextures[m_iCurSurface];
	D3DSURFACE_DESC videoDesc;
	m_pVideoTextures[m_iCurSurface]->GetLevelDesc(0, &videoDesc);

	unsigned src = 1;
	unsigned dst = 0;
	bool first = true;

	// fix incorrect conversion in EVR mixer
	if (m_pPSCorrection) {
		const float fConstData[4] = {1.0f / videoDesc.Width, 1.0f / videoDesc.Height, 0, 0};
		hr = m_pDevice9Ex->SetPixelShaderConstantF(0, fConstData, 1);

		CComPtr<IDirect3DSurface9> pTemporarySurface;
		hr = m_pFrameTextures[dst]->GetSurfaceLevel(0, &pTemporarySurface);
		hr = m_pDevice9Ex->SetRenderTarget(0, pTemporarySurface);
		hr = m_pDevice9Ex->SetPixelShader(m_pPSCorrection);
		TextureCopy(m_pVideoTextures[m_iCurSurface]);
		first = false;
		std::swap(src, dst);
		pVideoTexture = m_pFrameTextures[src];
	}

	if (bCustomPixelShaders) {
		static __int64 counter = 0;
		static long start = clock();

		long stop = clock();
		long diff = stop - start;

		if (diff >= 10*60*CLOCKS_PER_SEC) {
			start = stop;    // reset after 10 min (ps float has its limits in both range and accuracy)
		}

#if 1
		float fConstData[][4] = {
			{(float)videoDesc.Width, (float)videoDesc.Height, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
			{1.0f / videoDesc.Width, 1.0f / videoDesc.Height, 0, 0},
		};
#else
		CSize VideoSize = m_nativeVideoSize;
		float fConstData[][4] = {
			{(float)VideoSize.cx, (float)VideoSize.cy, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
			{1.0f / VideoSize.cx, 1.0f / VideoSize.cy, 0, 0},
		};
#endif
		hr = m_pDevice9Ex->SetPixelShaderConstantF(0, (float*)fConstData, std::size(fConstData));

		for (auto& Shader : m_pCustomPixelShaders) {
			CComPtr<IDirect3DSurface9> pTemporarySurface;
			hr = m_pFrameTextures[dst]->GetSurfaceLevel(0, &pTemporarySurface);
			hr = m_pDevice9Ex->SetRenderTarget(0, pTemporarySurface);

			if (!Shader.m_pPixelShader) {
				Shader.Compile(m_pPSC.get());
			}
			hr = m_pDevice9Ex->SetPixelShader(Shader.m_pPixelShader);

			if (first) {
				TextureCopy(m_pVideoTextures[m_iCurSurface]);
				first = false;
			} else {
				TextureCopy(m_pFrameTextures[src]);
			}

			std::swap(src, dst);
		}

		pVideoTexture = m_pFrameTextures[src];
	}

	if (iRotation || bFlip) {
		Vector dest[4];
		CComPtr<IDirect3DSurface9> pTemporarySurface;
		switch (iRotation) {
		case 0:
			dest[0].Set(0.0f, 0.0f, 0.5f);
			dest[1].Set((float)videoDesc.Width, 0.0f, 0.5f);
			dest[2].Set(0.0f, (float)videoDesc.Height, 0.5f);
			dest[3].Set((float)videoDesc.Width, (float)videoDesc.Height, 0.5f);
			hr = m_pFrameTextures[dst]->GetSurfaceLevel(0, &pTemporarySurface);
			break;
		case 90:
			dest[0].Set((float)videoDesc.Width, 0.0f, 0.5f);
			dest[1].Set((float)videoDesc.Width, (float)videoDesc.Height, 0.5f);
			dest[2].Set(0.0f, 0.0f, 0.5f);
			dest[3].Set(0.0f, (float)videoDesc.Height, 0.5f);
			hr = m_pRotateTexture->GetSurfaceLevel(0, &pTemporarySurface);
			break;
		case 180:
			dest[0].Set((float)videoDesc.Width, (float)videoDesc.Height, 0.5f);
			dest[1].Set(0.0f, (float)videoDesc.Height, 0.5f);
			dest[2].Set((float)videoDesc.Width, 0.0f, 0.5f);
			dest[3].Set(0.0f, 0.0f, 0.5f);
			hr = m_pFrameTextures[dst]->GetSurfaceLevel(0, &pTemporarySurface);
			break;
		case 270:
			dest[0].Set(0.0f, (float)videoDesc.Height, 0.5f);
			dest[1].Set(0.0f, 0.0f, 0.5f);
			dest[2].Set((float)videoDesc.Width, (float)videoDesc.Height, 0.5f);
			dest[3].Set((float)videoDesc.Width, 0.0f, 0.5f);
			hr = m_pRotateTexture->GetSurfaceLevel(0, &pTemporarySurface);
			break;
		}
		if (bFlip) {
			std::swap(dest[0], dest[1]);
			std::swap(dest[2], dest[3]);
		}

		hr = m_pDevice9Ex->SetRenderTarget(0, pTemporarySurface);

		MYD3DVERTEX<1> v[] = {
			{ dest[0].x - 0.5f, dest[0].y - 0.5f, 0.5f, 2.0f, 0.0f, 0.0f },
			{ dest[1].x - 0.5f, dest[1].y - 0.5f, 0.5f, 2.0f, 1.0f, 0.0f },
			{ dest[2].x - 0.5f, dest[2].y - 0.5f, 0.5f, 2.0f, 0.0f, 1.0f },
			{ dest[3].x - 0.5f, dest[3].y - 0.5f, 0.5f, 2.0f, 1.0f, 1.0f },
		};

		hr = m_pDevice9Ex->SetTexture(0, pVideoTexture);
		hr = m_pDevice9Ex->SetPixelShader(nullptr);
		hr = TextureBlt(m_pDevice9Ex, v, D3DTEXF_LINEAR);

		if (iRotation == 90 || iRotation == 270) {
			pVideoTexture = m_pRotateTexture;
		}
		else { // 0+flip and 180
			pVideoTexture = m_pFrameTextures[dst];
		}
	}

	// Resize the frame
	if (bCustomScreenSpacePixelShaders || bFinalPass || !pRenderTarget) {
		CComPtr<IDirect3DSurface9> pTemporarySurface;
		hr = m_pScreenSpaceTextures[0]->GetSurfaceLevel(0, &pTemporarySurface);

		if (SUCCEEDED(hr)) {
			hr = m_pDevice9Ex->SetRenderTarget(0, pTemporarySurface);
			hr = m_pDevice9Ex->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
		}
	} else {
		hr = m_pDevice9Ex->SetRenderTarget(0, pRenderTarget);
	}

	hr = Resize(pVideoTexture, srcRect, destRect);

	src = 0;
	dst = 1;

	// Apply the custom screen size pixel shaders
	if (bCustomScreenSpacePixelShaders) {
		static __int64 counter = 555;
		static long start = clock() + 333;

		long stop = clock() + 333;
		long diff = stop - start;

		if (diff >= 10*60*CLOCKS_PER_SEC) {
			start = stop;    // reset after 10 min (ps float has its limits in both range and accuracy)
		}

		float fConstData[][4] = {
			{(float)m_ScreenSpaceTexWidth, (float)m_ScreenSpaceTexHeight, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
			{1.0f / m_ScreenSpaceTexWidth, 1.0f / m_ScreenSpaceTexHeight, 0, 0},
		};

		hr = m_pDevice9Ex->SetPixelShaderConstantF(0, (float*)fConstData, std::size(fConstData));

		if (m_pCustomScreenSpacePixelShaders.size()) {
			const auto it_last = --m_pCustomScreenSpacePixelShaders.end();

			for (auto it = m_pCustomScreenSpacePixelShaders.begin(), end = m_pCustomScreenSpacePixelShaders.end(); it != end; ++it) {

				if (it != it_last || bFinalPass || !pRenderTarget) {
					CComPtr<IDirect3DSurface9> pTemporarySurface;
					hr = m_pScreenSpaceTextures[dst]->GetSurfaceLevel(0, &pTemporarySurface);
					if (SUCCEEDED(hr)) {
						hr = m_pDevice9Ex->SetRenderTarget(0, pTemporarySurface);
					}
				}
				else {
					hr = m_pDevice9Ex->SetRenderTarget(0, pRenderTarget);
				}

				auto& Shader = *it;
				if (!Shader.m_pPixelShader) {
					Shader.Compile(m_pPSC.get());
				}
				hr = m_pDevice9Ex->SetPixelShader(Shader.m_pPixelShader);
				TextureCopy(m_pScreenSpaceTextures[src]);
				std::swap(src, dst);
			}
		}
	}

	// Final pass
	if (bFinalPass) {
		if (!pRenderTarget) {
			CComPtr<IDirect3DSurface9> pTemporarySurface;
			hr = m_pScreenSpaceTextures[dst]->GetSurfaceLevel(0, &pTemporarySurface);
			if (SUCCEEDED(hr)) {
				hr = m_pDevice9Ex->SetRenderTarget(0, pTemporarySurface);
			}
		} else {
			hr = m_pDevice9Ex->SetRenderTarget(0, pRenderTarget);
		}

		hr = FinalPass(m_pScreenSpaceTextures[src]);
		std::swap(src, dst);
	}
	hr = m_pDevice9Ex->SetPixelShader(nullptr);

	m_iScreenTex = src;

	return hr;
}

HRESULT CDX9RenderingEngine::Stereo3DTransform(IDirect3DSurface9* pRenderTarget, const CRect& destRect)
{
	if (destRect.IsRectEmpty() || m_Caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
		return S_OK;
	}

	HRESULT hr = S_OK;

	if (m_Stereo3DSets.iTransform == STEREO3D_HalfOverUnder_to_Interlace) {
		if (!m_pConvertToInterlacePixelShader) {
			hr = CreateShaderFromResource(m_pDevice9Ex, &m_pConvertToInterlacePixelShader, IDF_SHADER_CONVERT_TO_INTERLACE);
		}

		if (m_pConvertToInterlacePixelShader) {
			float fConstData[][4] = {
				{ (float)m_ScreenSpaceTexHeight, 0, (float)destRect.top / m_ScreenSpaceTexHeight, (float)destRect.bottom / m_ScreenSpaceTexHeight },
			};
			hr = m_pDevice9Ex->SetPixelShaderConstantF(0, (float*)fConstData, std::size(fConstData));
			hr = m_pDevice9Ex->SetPixelShader(m_pConvertToInterlacePixelShader);
			hr = m_pDevice9Ex->SetRenderTarget(0, pRenderTarget);
			hr = TextureCopy(m_pScreenSpaceTextures[m_iScreenTex]);
			m_pDevice9Ex->SetPixelShader(nullptr);
		}
	}

	return hr;
}

#if DXVA2VP || DXVAHDVP
const UINT VIDEO_FPS     = 60;
#endif
#if DXVA2VP
const UINT VIDEO_MSPF    = (1000 + VIDEO_FPS / 2) / VIDEO_FPS;
const UINT VIDEO_100NSPF = VIDEO_MSPF * 10000;

BOOL CDX9RenderingEngine::InitializeDXVA2VP(int width, int height)
{
	if (!m_hDxva2Lib) {
		return FALSE;
	}
	DLog(L"InitializeDXVA2VP: start");

	HRESULT (WINAPI *pfDXVA2CreateVideoService)(IDirect3DDevice9* pDD, REFIID riid, void** ppService);
	(FARPROC &)pfDXVA2CreateVideoService = GetProcAddress(m_hDxva2Lib, "DXVA2CreateVideoService");
	if (!pfDXVA2CreateVideoService) {
		return FALSE;
	}

	HRESULT hr = S_OK;
	// Create DXVA2 Video Processor Service.
	hr = pfDXVA2CreateVideoService(m_pDevice9Ex, IID_IDirectXVideoProcessorService, (VOID**)&m_pDXVA2_VPService);
	if (FAILED(hr)) {
		DLog(L"DXVA2CreateVideoService failed with error 0x%x.", hr);
		return FALSE;
	}

	// Initialize the video descriptor.
	m_VideoDesc.SampleWidth                         = width;
	m_VideoDesc.SampleHeight                        = height;
	// no need to fill the fields of m_VideoDesc.SampleFormat when converting RGB to RGB
	//m_VideoDesc.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Unknown;
	//m_VideoDesc.SampleFormat.NominalRange           = DXVA2_NominalRange_Unknown;
	//m_VideoDesc.SampleFormat.VideoTransferMatrix    = DXVA2_VideoTransferMatrix_Unknown;
	//m_VideoDesc.SampleFormat.VideoLighting          = DXVA2_VideoLighting_Unknown;
	//m_VideoDesc.SampleFormat.VideoPrimaries         = DXVA2_VideoPrimaries_Unknown;
	//m_VideoDesc.SampleFormat.VideoTransferFunction  = DXVA2_VideoTransFunc_Unknown;
	m_VideoDesc.SampleFormat.SampleFormat           = DXVA2_SampleProgressiveFrame;
	m_VideoDesc.Format                              = D3DFMT_YUY2; // for Nvidia must be YUY2, for Intel may be any.
	m_VideoDesc.InputSampleFreq.Numerator           = VIDEO_FPS;
	m_VideoDesc.InputSampleFreq.Denominator         = 1;
	m_VideoDesc.OutputFrameFreq.Numerator           = VIDEO_FPS;
	m_VideoDesc.OutputFrameFreq.Denominator         = 1;

	// Query DXVA2_VideoProcProgressiveDevice.
	CreateDXVA2VPDevice(DXVA2_VideoProcProgressiveDevice);

	if (!m_pDXVA2_VP) {
		DLog(L"Failed to create a DXVA2 device.");
		return FALSE;
	}

	return TRUE;
}

BOOL CDX9RenderingEngine::CreateDXVA2VPDevice(REFGUID guid)
{
	HRESULT hr;

	if (guid == DXVA2_VideoProcSoftwareDevice) {
		return FALSE; // use only hardware devices
	}

	// Query the supported render target format.
	UINT i, count;
	D3DFORMAT* formats = nullptr;
	hr = m_pDXVA2_VPService->GetVideoProcessorRenderTargets(guid, &m_VideoDesc, &count, &formats);
	if (FAILED(hr)) {
		DLog(L"GetVideoProcessorRenderTargets failed with error 0x%x.", hr);
		return FALSE;
	}
	for (i = 0; i < count; i++) {
		if (formats[i] == m_BackbufferFmt) {
			break;
		}
	}
	CoTaskMemFree(formats);
	if (i >= count) {
		DLog(L"GetVideoProcessorRenderTargets doesn't support that format.");
		return FALSE;
	}

	// Query video processor capabilities.
	hr = m_pDXVA2_VPService->GetVideoProcessorCaps(guid, &m_VideoDesc, m_BackbufferFmt, &m_VPCaps);
	if (FAILED(hr)) {
		DLog(L"GetVideoProcessorCaps failed with error 0x%x.", hr);
		return FALSE;
	}

	// Check to see if the device is hardware device.
	if (!(m_VPCaps.DeviceCaps & DXVA2_VPDev_HardwareDevice)) {
		DLog(L"The DXVA2 device isn't a hardware device.");
		return FALSE;
	}

	// This is a progressive device and we cannot provide any reference sample.
	if (m_VPCaps.NumForwardRefSamples > 0 || m_VPCaps.NumBackwardRefSamples > 0) {
		DLog(L"NumForwardRefSamples or NumBackwardRefSamples is greater than 0.");
		return FALSE;
	}

	// Check to see if the device supports all the VP operations we want.
	const UINT VIDEO_REQUIED_OP = DXVA2_VideoProcess_StretchX | DXVA2_VideoProcess_StretchY;
	if ((m_VPCaps.VideoProcessorOperations & VIDEO_REQUIED_OP) != VIDEO_REQUIED_OP) {
		DLog(L"The DXVA2 device doesn't support the VP operations.");
		return FALSE;
	}

	// Query ProcAmp ranges.
	DXVA2_ValueRange range;
	for (i = 0; i < ARRAYSIZE(m_ProcAmpValues); i++) {
		if (m_VPCaps.ProcAmpControlCaps & (1 << i)) {
			hr = m_pDXVA2_VPService->GetProcAmpRange(guid,
											 &m_VideoDesc,
											 m_BackbufferFmt,
											 1 << i,
											 &range);
			if (FAILED(hr)) {
				DLog(L"GetProcAmpRange failed with error 0x%x.", hr);
				return FALSE;
			}
			// Set to default value
			m_ProcAmpValues[i] = range.DefaultValue;
		}
	}

	// Query Noise Filter ranges.
	if (m_VPCaps.VideoProcessorOperations & DXVA2_VideoProcess_NoiseFilter) {
		for (i = 0; i < ARRAYSIZE(m_NFilterValues); i++) {
			hr = m_pDXVA2_VPService->GetFilterPropertyRange(guid,
													&m_VideoDesc,
													m_BackbufferFmt,
													DXVA2_NoiseFilterLumaLevel + i,
													&range);
			if (FAILED(hr)) {
				DLog(L"GetFilterPropertyRange(Noise) failed with error 0x%x.", hr);
				return FALSE;
			}
			// Set to default value
			m_NFilterValues[i] = range.DefaultValue;
		}
	}

	// Query Detail Filter ranges.
	if (m_VPCaps.VideoProcessorOperations & DXVA2_VideoProcess_DetailFilter) {
		for (i = 0; i < ARRAYSIZE(m_DFilterValues); i++) {
			hr = m_pDXVA2_VPService->GetFilterPropertyRange(guid,
													&m_VideoDesc,
													m_BackbufferFmt,
													DXVA2_DetailFilterLumaLevel + i,
													&range);
			if (FAILED(hr)) {
				DLog(L"GetFilterPropertyRange(Detail) failed with error 0x%x.", hr);
				return FALSE;
			}
			// Set to default value
			m_DFilterValues[i] = range.DefaultValue;
		}
	}

	// Finally create a video processor device.
	hr = m_pDXVA2_VPService->CreateVideoProcessor(guid, &m_VideoDesc, m_BackbufferFmt, 0, &m_pDXVA2_VP);
	if (FAILED(hr)) {
		DLog(L"CreateVideoProcessor failed with error 0x%x.", hr);
		return FALSE;
	}

	return TRUE;
}

HRESULT CDX9RenderingEngine::TextureResizeDXVA2(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect)
{
	HRESULT hr = S_OK;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	if (!m_pDXVA2_VP && !InitializeDXVA2VP(srcRect.Width(), srcRect.Height())) {
		return E_FAIL;
	}

	DXVA2_VideoProcessBltParams blt = {0};
	DXVA2_VideoSample samples[1] = {0};

	static DWORD frame = 0;
	LONGLONG start_100ns = frame * LONGLONG(VIDEO_100NSPF);
	LONGLONG end_100ns   = start_100ns + LONGLONG(VIDEO_100NSPF);
	frame++;

	CComPtr<IDirect3DSurface9> pRenderTarget;
	m_pDevice9Ex->GetRenderTarget(0, &pRenderTarget);
	CRect rSrcRect(srcRect);
	CRect rDstRect(destRect);
	ClipToSurface(pRenderTarget, rSrcRect, rDstRect);

	// Initialize VPBlt parameters.
	blt.TargetFrame = start_100ns;
	blt.TargetRect  = rDstRect;

	// DXVA2_VideoProcess_Constriction
	blt.ConstrictionSize.cx = rDstRect.Width();
	blt.ConstrictionSize.cy = rDstRect.Height();

	blt.BackgroundColor = { 128 * 0x100, 128 * 0x100, 16 * 0x100, 0xFFFF }; // black

	// DXVA2_VideoProcess_YUV2RGBExtended (not used)
	//blt.DestFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Unknown;
	//blt.DestFormat.NominalRange           = DXVA2_NominalRange_Unknown;
	//blt.DestFormat.VideoTransferMatrix    = DXVA2_VideoTransferMatrix_Unknown;
	//blt.DestFormat.VideoLighting          = DXVA2_VideoLighting_Unknown;
	//blt.DestFormat.VideoPrimaries         = DXVA2_VideoPrimaries_Unknown;
	//blt.DestFormat.VideoTransferFunction  = DXVA2_VideoTransFunc_Unknown;

	blt.DestFormat.SampleFormat = DXVA2_SampleProgressiveFrame;

	// DXVA2_ProcAmp_Brightness/Contrast/Hue/Saturation
	blt.ProcAmpValues.Brightness = m_ProcAmpValues[0];
	blt.ProcAmpValues.Contrast   = m_ProcAmpValues[1];
	blt.ProcAmpValues.Hue        = m_ProcAmpValues[2];
	blt.ProcAmpValues.Saturation = m_ProcAmpValues[3];

	// DXVA2_VideoProcess_AlphaBlend
	blt.Alpha = DXVA2_Fixed32OpaqueAlpha();

	// DXVA2_VideoProcess_NoiseFilter
	blt.NoiseFilterLuma.Level       = m_NFilterValues[0];
	blt.NoiseFilterLuma.Threshold   = m_NFilterValues[1];
	blt.NoiseFilterLuma.Radius      = m_NFilterValues[2];
	blt.NoiseFilterChroma.Level     = m_NFilterValues[3];
	blt.NoiseFilterChroma.Threshold = m_NFilterValues[4];
	blt.NoiseFilterChroma.Radius    = m_NFilterValues[5];

	// DXVA2_VideoProcess_DetailFilter
	blt.DetailFilterLuma.Level       = m_DFilterValues[0];
	blt.DetailFilterLuma.Threshold   = m_DFilterValues[1];
	blt.DetailFilterLuma.Radius      = m_DFilterValues[2];
	blt.DetailFilterChroma.Level     = m_DFilterValues[3];
	blt.DetailFilterChroma.Threshold = m_DFilterValues[4];
	blt.DetailFilterChroma.Radius    = m_DFilterValues[5];

	// Initialize main stream video sample.
	samples[0].Start = start_100ns;
	samples[0].End   = end_100ns;

	// DXVA2_VideoProcess_YUV2RGBExtended (not used)
	//samples[0].SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_Unknown;
	//samples[0].SampleFormat.NominalRange           = DXVA2_NominalRange_Unknown;
	//samples[0].SampleFormat.VideoTransferMatrix    = DXVA2_VideoTransferMatrix_Unknown;
	//samples[0].SampleFormat.VideoLighting          = DXVA2_VideoLighting_Unknown;
	//samples[0].SampleFormat.VideoPrimaries         = DXVA2_VideoPrimaries_Unknown;
	//samples[0].SampleFormat.VideoTransferFunction  = DXVA2_VideoTransFunc_Unknown;

	samples[0].SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;

	CComPtr<IDirect3DSurface9> pSurface;
	pTexture->GetSurfaceLevel(0, &pSurface);

	samples[0].SrcSurface = pSurface;

	// DXVA2_VideoProcess_SubRects
	samples[0].SrcRect = rSrcRect;

	// DXVA2_VideoProcess_StretchX, Y
	samples[0].DstRect = rDstRect;

	// DXVA2_VideoProcess_PlanarAlpha
	samples[0].PlanarAlpha = DXVA2_Fixed32OpaqueAlpha();

	// clear pRenderTarget, need for Nvidia graphics cards and Intel mobile graphics
	CRect clientRect;
	if (rDstRect.left > 0 || rDstRect.top > 0 ||
			GetClientRect(m_hWnd, clientRect) && (rDstRect.right < clientRect.Width() || rDstRect.bottom < clientRect.Height())) {
		//m_pDevice9Ex->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0); //  not worked
		m_pDevice9Ex->ColorFill(pRenderTarget, nullptr, 0);
	}

	hr = m_pDXVA2_VP->VideoProcessBlt(pRenderTarget, &blt, samples, 1, nullptr);
	if (FAILED(hr)) {
		DLog(L"TextureResizeDXVA2: VideoProcessBlt() failed with error 0x%x.", hr);
	}

	return hr;
}
#endif

#if DXVAHDVP
HRESULT DXVAHD_SetStreamFormat(IDXVAHD_VideoProcessor *pVP, UINT stream, D3DFORMAT format)
{
	DXVAHD_STREAM_STATE_D3DFORMAT_DATA d3dformat = { format };

	HRESULT hr = pVP->SetVideoProcessStreamState(
		stream,
		DXVAHD_STREAM_STATE_D3DFORMAT,
		sizeof(d3dformat),
		&d3dformat
	);

	return hr;
}

HRESULT DXVAHD_SetFrameFormat(IDXVAHD_VideoProcessor *pVP, UINT stream, DXVAHD_FRAME_FORMAT format)
{
	DXVAHD_STREAM_STATE_FRAME_FORMAT_DATA frame_format = { format };

	HRESULT hr = pVP->SetVideoProcessStreamState(
		stream,
		DXVAHD_STREAM_STATE_FRAME_FORMAT,
		sizeof(frame_format),
		&frame_format
	);

	return hr;
}

HRESULT DXVAHD_SetTargetRect(IDXVAHD_VideoProcessor *pVP, BOOL bEnable, const RECT &rect)
{
	DXVAHD_BLT_STATE_TARGET_RECT_DATA tr = { bEnable, rect };

	HRESULT hr = pVP->SetVideoProcessBltState(
		DXVAHD_BLT_STATE_TARGET_RECT,
		sizeof(tr),
		&tr
	);

	return hr;
}

HRESULT DXVAHD_SetSourceRect(IDXVAHD_VideoProcessor *pVP, UINT stream, BOOL bEnable, const RECT& rect)
{
	DXVAHD_STREAM_STATE_SOURCE_RECT_DATA src = { bEnable, rect };

	HRESULT hr = pVP->SetVideoProcessStreamState(
		stream,
		DXVAHD_STREAM_STATE_SOURCE_RECT,
		sizeof(src),
		&src
	);

	return hr;
}

HRESULT DXVAHD_SetDestinationRect(IDXVAHD_VideoProcessor *pVP, UINT stream, BOOL bEnable, const RECT &rect)
{
	DXVAHD_STREAM_STATE_DESTINATION_RECT_DATA DstRect = { bEnable, rect };

	HRESULT hr = pVP->SetVideoProcessStreamState(
		stream,
		DXVAHD_STREAM_STATE_DESTINATION_RECT,
		sizeof(DstRect),
		&DstRect
	);

	return hr;
}

BOOL CDX9RenderingEngine::InitializeDXVAHDVP(int width, int height)
{
	if (!m_hDxva2Lib) {
		return FALSE;
	}
	DLog(L"InitializeDXVAHDVP: start");

	HRESULT(WINAPI *pDXVAHD_CreateDevice)(IDirect3DDevice9Ex  *pD3DDevice, const DXVAHD_CONTENT_DESC *pContentDesc, DXVAHD_DEVICE_USAGE Usage, PDXVAHDSW_Plugin pPlugin, IDXVAHD_Device **ppDevice);
	(FARPROC &)pDXVAHD_CreateDevice = GetProcAddress(m_hDxva2Lib, "DXVAHD_CreateDevice");
	if (!pDXVAHD_CreateDevice) {
		return FALSE;
	}

	HRESULT hr = S_OK;

	DXVAHD_CONTENT_DESC desc;
	desc.InputFrameFormat = DXVAHD_FRAME_FORMAT_PROGRESSIVE;
	desc.InputFrameRate.Numerator = VIDEO_FPS;
	desc.InputFrameRate.Denominator = 1;
	desc.InputWidth = width;
	desc.InputHeight = height;
	desc.OutputFrameRate.Numerator = VIDEO_FPS;
	desc.OutputFrameRate.Denominator = 1;
	desc.OutputWidth = m_ScreenSpaceTexWidth;
	desc.OutputHeight = m_ScreenSpaceTexHeight;

	// Create the DXVA-HD device.
	hr = pDXVAHD_CreateDevice(m_pDevice9Ex, &desc, DXVAHD_DEVICE_USAGE_PLAYBACK_NORMAL, nullptr, &m_pDXVAHD_Device);
	if (FAILED(hr)) {
		DLog(L"InitializeDXVAHDVP: DXVAHD_CreateDevice() failed with error 0x%x.", hr);
		return FALSE;
	}

	// Get the DXVA-HD device caps.
	DXVAHD_VPDEVCAPS caps = {};
	hr = m_pDXVAHD_Device->GetVideoProcessorDeviceCaps(&caps);
	if (FAILED(hr) || caps.MaxInputStreams < 1) {
		return FALSE;
	}

	std::vector<D3DFORMAT> Formats;

	// Check the output formats.
	Formats.resize(caps.OutputFormatCount);
	hr = m_pDXVAHD_Device->GetVideoProcessorOutputFormats(caps.OutputFormatCount, Formats.data());
	if (FAILED(hr)) {
		return FALSE;
	}
#if _DEBUG
	{
		CString dbgstr = L"DXVA-HD output formats:";
		for (const auto& format : Formats) {
			dbgstr.AppendFormat(L"\n%s", D3DFormatToString(format));
		}
		DLog(dbgstr);
	}
#endif
	if (std::none_of(Formats.cbegin(), Formats.cend(), [](D3DFORMAT f) { return f == D3DFMT_X8R8G8B8; })) {
		return FALSE;
	}

	// Check the input formats.
	Formats.resize(caps.InputFormatCount);
	hr = m_pDXVAHD_Device->GetVideoProcessorInputFormats(caps.InputFormatCount, Formats.data());
	if (FAILED(hr)) {
		return FALSE;
	}
#if _DEBUG
	{
		CString dbgstr = L"DXVA-HD input formats:";
		for (const auto& format : Formats) {
			dbgstr.AppendFormat(L"\n%s", D3DFormatToString(format));
		}
		DLog(dbgstr);
	}
#endif
	if (std::none_of(Formats.cbegin(), Formats.cend(), [](D3DFORMAT f) { return f == D3DFMT_X8R8G8B8; })) {
		return FALSE;
	}

	// Create the VP device.
	std::vector<DXVAHD_VPCAPS> VPCaps;
	VPCaps.resize(caps.VideoProcessorCount);

	hr = m_pDXVAHD_Device->GetVideoProcessorCaps(caps.VideoProcessorCount, VPCaps.data());
	if (FAILED(hr)) {
		return FALSE;
	}

	hr = m_pDXVAHD_Device->CreateVideoProcessor(&VPCaps[0].VPGuid, &m_pDXVAHD_VP);
	if (FAILED(hr)) {
		DLog(L"InitializeDXVAHDVP: CreateVideoProcessor() failed with error 0x%x.", hr);
		return FALSE;
	}

	// Set the initial stream states for the primary stream.
	hr = DXVAHD_SetStreamFormat(m_pDXVAHD_VP, 0, D3DFMT_X8R8G8B8);
	if (FAILED(hr)) {
		return FALSE;
	}

	hr = DXVAHD_SetFrameFormat(m_pDXVAHD_VP, 0, DXVAHD_FRAME_FORMAT_PROGRESSIVE);
	if (FAILED(hr)) {
		return FALSE;
	}

	return TRUE;
}

HRESULT CDX9RenderingEngine::TextureResizeDXVAHD(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect)
{
	HRESULT hr = S_OK;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	if (!m_pDXVAHD_VP && !InitializeDXVAHDVP(srcRect.Width(), srcRect.Height())) {
		return E_FAIL;
	}

	static DWORD frame = 0;

	CComPtr<IDirect3DSurface9> pRenderTarget;
	hr = m_pDevice9Ex->GetRenderTarget(0, &pRenderTarget);

	CComPtr<IDirect3DSurface9> pSurface;
	hr = pTexture->GetSurfaceLevel(0, &pSurface);

	DXVAHD_STREAM_DATA stream_data = {};
	stream_data.Enable = TRUE;
	stream_data.OutputIndex = 0;
	stream_data.InputFrameOrField = frame;
	stream_data.pInputSurface = pSurface;

	//CRect rSrcRect(srcRect);
	//CRect rDstRect(destRect);
	//ClipToSurface(pRenderTarget, rSrcRect, rDstRect);

	hr = DXVAHD_SetSourceRect(m_pDXVAHD_VP, 0, TRUE, srcRect);
	hr = DXVAHD_SetDestinationRect(m_pDXVAHD_VP, 0, TRUE, destRect);

	// Perform the blit.
	hr = m_pDXVAHD_VP->VideoProcessBltHD(pRenderTarget, frame, 1, &stream_data);
	if (FAILED(hr)) {
		DLog(L"TextureResizeDXVAHD: VideoProcessBltHD() failed with error 0x%x.", hr);
	}
	frame++;

	return hr;
}
#endif

HRESULT CDX9RenderingEngine::InitVideoTextures()
{
	HRESULT hr = S_OK;

	size_t count = m_pCustomPixelShaders.size();

	if (m_pPSCorrection) {
		count++;
	}
	if (m_iRotation == 180 || !m_iRotation && m_bFlip) {
		count++;
	}
	if (count > std::size(m_pFrameTextures)) {
		count = std::size(m_pFrameTextures);
	}

	for (size_t i = 0; i < count; i++) {
		if (m_pFrameTextures[i] == nullptr) {
			hr = m_pDevice9Ex->CreateTexture(
					 m_nativeVideoSize.cx, m_nativeVideoSize.cy, 1, D3DUSAGE_RENDERTARGET, m_SurfaceFmt,
					 D3DPOOL_DEFAULT, &m_pFrameTextures[i], nullptr);

			if (FAILED(hr)) {
				// Free all textures
				RELEASE_PTR_ARRAY(m_pFrameTextures);

				return hr;
			}
		}
	}

	// Free unnecessary textures
	for (size_t i = count; i < std::size(m_pFrameTextures); i++) {
		m_pFrameTextures[i].Release();
	}

	if (m_iRotation == 90 || m_iRotation == 270) {
		if (m_pRotateTexture == nullptr) {
			UINT a = std::max(m_nativeVideoSize.cx, m_nativeVideoSize.cy);
			hr = m_pDevice9Ex->CreateTexture(
				 a, a, 1, D3DUSAGE_RENDERTARGET, m_SurfaceFmt,
				 D3DPOOL_DEFAULT, &m_pRotateTexture, nullptr);
		}
	}
	else {
		m_pRotateTexture.Release();
	}

	return hr;
}

HRESULT CDX9RenderingEngine::InitScreenSpaceTextures(unsigned count)
{
	HRESULT hr = S_OK;
	if (count > std::size(m_pScreenSpaceTextures)) {
		count = std::size(m_pScreenSpaceTextures);
	}

	m_ScreenSpaceTexWidth = std::min(m_ScreenSize.cx, (LONG)m_Caps.MaxTextureWidth);
	m_ScreenSpaceTexHeight = std::min(m_ScreenSize.cy, (LONG)m_Caps.MaxTextureHeight);

	for (unsigned i = 0; i < count; i++) {
		if (m_pScreenSpaceTextures[i] == nullptr) {
			hr = m_pDevice9Ex->CreateTexture(
					m_ScreenSpaceTexWidth, m_ScreenSpaceTexHeight, 1, D3DUSAGE_RENDERTARGET, m_SurfaceFmt,
					D3DPOOL_DEFAULT, &m_pScreenSpaceTextures[i], nullptr);

			if (FAILED(hr)) {
				// Free all textures
				RELEASE_PTR_ARRAY(m_pScreenSpaceTextures);

				return hr;
			}
		}
	}

	// Free unnecessary textures
	for (unsigned i = count; i < std::size(m_pScreenSpaceTextures); i++) {
		m_pScreenSpaceTextures[i].Release();
	}

	return hr;
}

HRESULT CDX9RenderingEngine::InitShaderResizer(int resizer)
{
	int iShader = -1;

	switch (resizer) {
	case RESIZER_NEAREST:
	case RESIZER_BILINEAR:
	case RESIZER_DXVA2:
	case RESIZER_DXVAHD:
		return S_FALSE;
	case RESIZER_SHADER_BSPLINE:   iShader = shader_bspline_x;             break;
	case RESIZER_SHADER_MITCHELL:  iShader = shader_mitchell_x;            break;
	case RESIZER_SHADER_CATMULL:   iShader = shader_catmull_x;             break;
	case RESIZER_SHADER_BICUBIC06: iShader = shader_bicubic06_x;           break;
	case RESIZER_SHADER_BICUBIC08: iShader = shader_bicubic08_x;           break;
	case RESIZER_SHADER_BICUBIC10: iShader = shader_bicubic10_x;           break;
	case RESIZER_SHADER_LANCZOS2:  iShader = shader_lanczos2_x;            break;
	case RESIZER_SHADER_LANCZOS3:  iShader = shader_lanczos3_x;            break;
	case DOWNSCALER_SIMPLE:        iShader = shader_downscaler_simple_x;   break;
	case DOWNSCALER_BOX:           iShader = shader_downscaler_box_x;      break;
	case DOWNSCALER_BILINEAR:      iShader = shader_downscaler_bilinear_x; break;
	case DOWNSCALER_HAMMING:       iShader = shader_downscaler_hamming_x;  break;
	case DOWNSCALER_BICUBIC:       iShader = shader_downscaler_bicubic_x;  break;
	case DOWNSCALER_LANCZOS:       iShader = shader_downscaler_lanczos_x;  break;
	default:
		return E_INVALIDARG;
	}

	if (m_pResizerPixelShaders[iShader]) {
		return S_OK;
	}

	if (m_Caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
		return E_FAIL;
	}

	UINT resid = 0;

	switch (iShader) {
	case shader_bspline_x:             resid = IDF_SHADER_RESIZER_BSPLINE4_X;    break;
	case shader_mitchell_x:            resid = IDF_SHADER_RESIZER_MITCHELL4_X;   break;
	case shader_catmull_x:             resid = IDF_SHADER_RESIZER_CATMULL4_X;    break;
	case shader_bicubic06_x:           resid = IDF_SHADER_RESIZER_BICUBIC06_X;   break;
	case shader_bicubic08_x:           resid = IDF_SHADER_RESIZER_BICUBIC08_X;   break;
	case shader_bicubic10_x:           resid = IDF_SHADER_RESIZER_BICUBIC10_X;   break;
	case shader_lanczos2_x:            resid = IDF_SHADER_RESIZER_LANCZOS2_X;    break;
	case shader_lanczos3_x:            resid = IDF_SHADER_RESIZER_LANCZOS3_X;    break;
	case shader_downscaler_simple_x:   resid = IDF_SHADER_DOWNSCALER_SIMPLE_X;   break;
	case shader_downscaler_box_x:      resid = IDF_SHADER_DOWNSCALER_BOX_X;      break;
	case shader_downscaler_bilinear_x: resid = IDF_SHADER_DOWNSCALER_BILINEAR_X; break;
	case shader_downscaler_hamming_x:  resid = IDF_SHADER_DOWNSCALER_HAMMING_X;  break;
	case shader_downscaler_bicubic_x:  resid = IDF_SHADER_DOWNSCALER_BICUBIC_X;  break;
	case shader_downscaler_lanczos_x:  resid = IDF_SHADER_DOWNSCALER_LANCZOS_X;  break;
	default:
		return E_INVALIDARG;
	}

	HRESULT hr = CreateShaderFromResource(m_pDevice9Ex, &m_pResizerPixelShaders[iShader], resid);
	if (S_OK == hr) {
		hr = CreateShaderFromResource(m_pDevice9Ex, &m_pResizerPixelShaders[iShader + 1], resid + 1);
	}
	if (FAILED(hr)) {
		ASSERT(0);
		m_pResizerPixelShaders[iShader].Release();
		m_pResizerPixelShaders[iShader + 1].Release();
		return hr;
	}

	return S_OK;
}

HRESULT CDX9RenderingEngine::TextureResize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, D3DTEXTUREFILTERTYPE filter)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	float dx = 1.0f / desc.Width;
	float dy = 1.0f / desc.Height;

	MYD3DVERTEX<1> v[] = {
		{(float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, {srcRect.left  * dx, srcRect.top    * dy} },
		{(float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, {srcRect.right * dx, srcRect.top    * dy} },
		{(float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, {srcRect.left  * dx, srcRect.bottom * dy} },
		{(float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, {srcRect.right * dx, srcRect.bottom * dy} },
	};

	hr = m_pDevice9Ex->SetTexture(0, pTexture);
	hr = m_pDevice9Ex->SetPixelShader(nullptr);
	hr = TextureBlt(m_pDevice9Ex, v, filter);

	return hr;
}

HRESULT CDX9RenderingEngine::TextureResizeShader(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, int iShader)
{
	HRESULT hr = S_OK;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	const float dx = 1.0f / desc.Width;
	const float dy = 1.0f / desc.Height;

	const float scale_x = (float)srcRect.Width() / destRect.Width();
	const float scale_y = (float)srcRect.Height() / destRect.Height();
	const float steps_x = floor(scale_x + 0.5f);
	const float steps_y = floor(scale_y + 0.5f);

	const float tx0 = (float)srcRect.left - 0.5f;
	const float ty0 = (float)srcRect.top - 0.5f;
	const float tx1 = (float)srcRect.right - 0.5f;
	const float ty1 = (float)srcRect.bottom - 0.5f;

	MYD3DVERTEX<1> v[] = {
		{(float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, { tx0, ty0 } },
		{(float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, { tx1, ty0 } },
		{(float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, { tx0, ty1 } },
		{(float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, { tx1, ty1 } },
	};

	float fConstData[][4] = {
		{ dx, dy, 0, 0 },
		{ scale_x, scale_y, 0, 0 },
		{ steps_x, steps_y, 0, 0 },
	};
	hr = m_pDevice9Ex->SetPixelShaderConstantF(0, (float*)fConstData, std::size(fConstData));
	hr = m_pDevice9Ex->SetPixelShader(m_pResizerPixelShaders[iShader]);

	hr = m_pDevice9Ex->SetTexture(0, pTexture);
	hr = TextureBlt(m_pDevice9Ex, v, D3DTEXF_POINT);
	m_pDevice9Ex->SetPixelShader(nullptr);

	return hr;
}

HRESULT CDX9RenderingEngine::ApplyResize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect, int resizer, int y)
{
	HRESULT hr = S_OK;

	const wchar_t* wsResizer = nullptr;

	switch (resizer) {
	case RESIZER_NEAREST:
		wsResizer = L"Nearest neighbor";
		hr = TextureResize(pTexture, srcRect, destRect, D3DTEXF_POINT);
		break;
	case RESIZER_BILINEAR:
		wsResizer = L"Bilinear";
		hr = TextureResize(pTexture, srcRect, destRect, D3DTEXF_LINEAR);
		break;
#if DXVA2VP
	case RESIZER_DXVA2:
		wsResizer = L"DXVA2 VP";
		hr = TextureResizeDXVA2(pTexture, srcRect, destRect);
		break;
#endif
#if DXVAHDVP
	case RESIZER_DXVAHD:
		wsResizer = L"DXVA-HD";
		hr = TextureResizeDXVAHD(pTexture, srcRect, destRect);
		break;
#endif
	case RESIZER_SHADER_BSPLINE:
		wsResizer = L"B-spline";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_bspline_x + y);
		break;
	case RESIZER_SHADER_MITCHELL:
		wsResizer = L"Mitchell-Netravali";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_mitchell_x + y);
		break;
	case RESIZER_SHADER_CATMULL:
		wsResizer = L"Catmull-Rom";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_catmull_x + y);
		break;
	case RESIZER_SHADER_BICUBIC06:
		wsResizer = L"Bicubic A=-0.6";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_bicubic06_x + y);
		break;
	case RESIZER_SHADER_BICUBIC08:
		wsResizer = L"Bicubic A=-0.8";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_bicubic08_x + y);
		break;
	case RESIZER_SHADER_BICUBIC10:
		wsResizer = L"Bicubic A=-1.0";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_bicubic10_x + y);
		break;
	case RESIZER_SHADER_LANCZOS2:
		wsResizer = L"Lanczos2";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_lanczos2_x + y);
		break;
	case RESIZER_SHADER_LANCZOS3:
		wsResizer = L"Lanczos3";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_lanczos3_x + y);
		break;
	case DOWNSCALER_SIMPLE:
		wsResizer = L"Simple averaging";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_downscaler_simple_x + y);
		break;
	case DOWNSCALER_BOX:
		wsResizer = L"Box";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_downscaler_box_x + y);
		break;
	case DOWNSCALER_BILINEAR:
		wsResizer = L"Bilinear";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_downscaler_bilinear_x + y);
		break;
	case DOWNSCALER_HAMMING:
		wsResizer = L"Hamming";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_downscaler_hamming_x + y);
		break;
	case DOWNSCALER_BICUBIC:
		wsResizer = L"Bicubic";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_downscaler_bicubic_x + y);
		break;
	case DOWNSCALER_LANCZOS:
		wsResizer = L"Lanczos";
		hr = TextureResizeShader(pTexture, srcRect, destRect, shader_downscaler_lanczos_x + y);
		break;
	}

	if (!m_wsResizer) {
		m_wsResizer = wsResizer;
	} else if (m_wsResizer != wsResizer) {
		m_wsResizer2 = wsResizer;
	}

	return hr;
}

HRESULT CDX9RenderingEngine::Resize(IDirect3DTexture9* pTexture, const CRect& srcRect, const CRect& destRect)
{
	m_wsResizer = nullptr;
	m_wsResizer2 = nullptr;

	int iResizer = m_ExtraSets.iResizer;
	int iDownscaler = m_ExtraSets.iDownscaler;
	if (FAILED(InitShaderResizer(iResizer))) {
		iResizer = RESIZER_BILINEAR;
	}
	if (FAILED(InitShaderResizer(iDownscaler))) {
		iDownscaler = RESIZER_BILINEAR;
	}

	const int w1 = srcRect.Width();
	const int h1 = srcRect.Height();
	const int w2 = destRect.Width();
	const int h2 = destRect.Height();
	HRESULT hr = S_OK;

	const int resizerX = (w1 == w2) ? -1 : (w1 > 2 * w2) ? iDownscaler : iResizer;
	const int resizerY = (h1 == h2) ? -1 : (h1 > 2 * h2) ? iDownscaler : iResizer;

	if (resizerX < 0 && resizerY < 0) {
		// no resize
		return TextureResize(pTexture, srcRect, destRect, D3DTEXF_POINT);
	}

	if (resizerX == resizerY && (resizerX <= RESIZER_DXVA2 || resizerX == RESIZER_DXVAHD)) {
		// one pass texture or dxva resize
		return ApplyResize(pTexture, srcRect, destRect, resizerX, 0);
	}

	if (resizerX >= 0 && resizerY >= 0) {
		// two pass resize

		// check intermediate texture
		UINT texWidth = std::min<UINT>(w2, m_Caps.MaxTextureWidth);
		UINT texHeight = std::min<UINT>(m_nativeVideoSize.cy, m_Caps.MaxTextureHeight);
		D3DSURFACE_DESC desc;

		if (m_pResizeTexture && m_pResizeTexture->GetLevelDesc(0, &desc) == D3D_OK) {
			if (texWidth != desc.Width || texHeight != desc.Height) {
				m_pResizeTexture.Release(); // need new texture
			}
		}

		if (!m_pResizeTexture) {
			hr = m_pDevice9Ex->CreateTexture(
				texWidth, texHeight, 1, D3DUSAGE_RENDERTARGET,
				D3DFMT_A16B16G16R16F, // use only float textures here
				D3DPOOL_DEFAULT, &m_pResizeTexture, nullptr);
			if (FAILED(hr) || FAILED(m_pResizeTexture->GetLevelDesc(0, &desc))) {
				m_pResizeTexture.Release();
				return TextureResize(pTexture, srcRect, destRect, D3DTEXF_LINEAR);
			}
		}

		CRect resizeRect(0, 0, desc.Width, desc.Height);

		// remember current RenderTarget
		CComPtr<IDirect3DSurface9> pRenderTarget;
		hr = m_pDevice9Ex->GetRenderTarget(0, &pRenderTarget);
		// set temp RenderTarget
		CComPtr<IDirect3DSurface9> pResizeSurface;
		hr = m_pResizeTexture->GetSurfaceLevel(0, &pResizeSurface);
		hr = m_pDevice9Ex->SetRenderTarget(0, pResizeSurface);

		// resize width
		hr = ApplyResize(pTexture, srcRect, resizeRect, resizerX, 0);

		// restore current RenderTarget
		hr = m_pDevice9Ex->SetRenderTarget(0, pRenderTarget);

		// resize height
		hr = ApplyResize(m_pResizeTexture, resizeRect, destRect, resizerY, 1);
	}
	else if (resizerX >= 0) {
		// one pass resize for width
		hr = ApplyResize(pTexture, srcRect, destRect, resizerX, 0);
	}
	else { // resizerY >= 0
		// one pass resize for height
		hr = ApplyResize(pTexture, srcRect, destRect, resizerY, 1);
	}

	return hr;
}

HRESULT CDX9RenderingEngine::InitFinalPass()
{
	HRESULT hr;

	const bool bColorManagement = m_bFP16Support && m_ExtraSets.bColorManagementEnable && m_SurfaceFmt != D3DFMT_X8R8G8B8;
	VideoSystem inputVideoSystem = (VideoSystem)m_ExtraSets.iColorManagementInput;
	AmbientLight ambientLight = (AmbientLight)m_ExtraSets.iColorManagementAmbientLight;
	ColorRenderingIntent renderingIntent = (ColorRenderingIntent)m_ExtraSets.iColorManagementIntent;

	const bool bDither = (m_bFP16Support && m_SurfaceFmt != D3DFMT_X8R8G8B8 && m_SurfaceFmt != m_DisplayFmt);

	if (!bColorManagement && !bDither) {
		m_bFinalPass = false;
		return S_FALSE;
	}

	bool bInitRequired = false;
	if (bColorManagement != m_bColorManagement || bDither != m_bDither ) {
		bInitRequired = true;
	}
	else if (bColorManagement && (m_InputVideoSystem != inputVideoSystem || m_RenderingIntent != renderingIntent || m_AmbientLight != ambientLight)) {
		bInitRequired = true;
	}

	if (!bInitRequired) {
		UpdateFinalPassStr();

		return S_OK;
	}

	// Update the settings
	m_bFinalPass = true;
	m_bColorManagement = bColorManagement;
	m_InputVideoSystem = inputVideoSystem;
	m_AmbientLight = ambientLight;
	m_RenderingIntent = renderingIntent;
	m_bDither = bDither;
	m_strFinalPass.Empty();

	// Initial cleanup
	m_pLut3DTexture.Release();
	m_pFinalPixelShader.Release();

	if (bDither && !m_pDitherTexture) {
		// Create the dither texture
		hr = m_pDevice9Ex->CreateTexture(DITHER_MATRIX_SIZE, DITHER_MATRIX_SIZE,
									  1,
									  D3DUSAGE_DYNAMIC,
									  D3DFMT_A16B16G16R16F,
									  D3DPOOL_DEFAULT,
									  &m_pDitherTexture,
									  nullptr);

		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}

		D3DLOCKED_RECT lockedRect;
		hr = m_pDitherTexture->LockRect(0, &lockedRect, nullptr, D3DLOCK_DISCARD);
		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}

		char* outputRowIterator = static_cast<char*>(lockedRect.pBits);
		for (int y = 0; y < DITHER_MATRIX_SIZE; y++) {
			unsigned short* outputIterator = reinterpret_cast<unsigned short*>(outputRowIterator);
			for (int x = 0; x < DITHER_MATRIX_SIZE; x++) {
				for (int i = 0; i < 4; i++) {
					*outputIterator++ = DITHER_MATRIX[y][x];
				}
			}

			outputRowIterator += lockedRect.Pitch;
		}

		hr = m_pDitherTexture->UnlockRect(0);
		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}
	}

	// Initialize the color management if necessary
	if (bColorManagement) {
		// Get the ICC profile path
		CStringW iccProfilePath;
		HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFOEXW miex;
		miex.cbSize = sizeof(miex);
		GetMonitorInfoW(hMonitor, &miex);
		HDC hDC = CreateDCW(L"DISPLAY", miex.szDevice, nullptr, nullptr);

		if (hDC != nullptr) {
			DWORD icmProfilePathSize = 0;
			GetICMProfileW(hDC, &icmProfilePathSize, nullptr);

			GetICMProfileW(hDC, &icmProfilePathSize, iccProfilePath.GetBuffer(icmProfilePathSize));
			iccProfilePath.ReleaseBuffer();

			DeleteDC(hDC);
		}

		// Create the 3D LUT texture
		hr = m_pDevice9Ex->CreateVolumeTexture(m_Lut3DSize, m_Lut3DSize, m_Lut3DSize,
											1,
											D3DUSAGE_DYNAMIC,
											D3DFMT_A16B16G16R16F,
											D3DPOOL_DEFAULT,
											&m_pLut3DTexture,
											nullptr);

		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}

		auto lut3DFloat32 = std::make_unique<float[]>(m_Lut3DEntryCount * 3);
		hr = CreateIccProfileLut(iccProfilePath.GetBuffer(), lut3DFloat32.get());
		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}

		auto lut3DFloat16 = std::make_unique<DirectX::PackedVector::HALF[]>(m_Lut3DEntryCount * 3);
		DirectX::PackedVector::XMConvertFloatToHalfStream(lut3DFloat16.get(), sizeof(lut3DFloat16[0]), lut3DFloat32.get(), sizeof(lut3DFloat32[0]), m_Lut3DEntryCount * 3);

		DirectX::PackedVector::HALF oneFloat16 = DirectX::PackedVector::XMConvertFloatToHalf(1.0f);

		D3DLOCKED_BOX lockedBox;
		hr = m_pLut3DTexture->LockBox(0, &lockedBox, nullptr, D3DLOCK_DISCARD);
		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}

		DirectX::PackedVector::HALF* lut3DFloat16Iterator = lut3DFloat16.get();
		char* outputSliceIterator = static_cast<char*>(lockedBox.pBits);
		for (unsigned b = 0; b < m_Lut3DSize; b++) {
			char* outputRowIterator = outputSliceIterator;

			for (unsigned g = 0; g < m_Lut3DSize; g++) {
				DirectX::PackedVector::HALF* outputIterator = reinterpret_cast<DirectX::PackedVector::HALF*>(outputRowIterator);

				for (unsigned r = 0; r < m_Lut3DSize; r++) {
					// R, G, B
					for (int i = 0; i < 3; i++) {
						*outputIterator++ = *lut3DFloat16Iterator++;
					}

					// A
					*outputIterator++ = oneFloat16;
				}

				outputRowIterator += lockedBox.RowPitch;
			}

			outputSliceIterator += lockedBox.SlicePitch;
		}

		hr = m_pLut3DTexture->UnlockBox(0);
		if (FAILED(hr)) {
			CleanupFinalPass();
			return hr;
		}
	}

	// Compile the final pixel shader
	CStringA srcdata;
	if (!LoadResource(IDF_SHADER_FINAL, srcdata, L"FILE")) {
		return E_FAIL;
	}

	D3D_SHADER_MACRO ShaderMacros[5] = {}; // number of macros + 1 
	size_t i = 0;

	ShaderMacros[i++] = { "QUANTIZATION", m_DisplayFmt == D3DFMT_A2R10G10B10 ? "1023.0" : "255.0" }; // 10-bit or 8-bit
	ShaderMacros[i++] = { "LUT3D_ENABLED", bColorManagement ? "1" : "0" };
	static char lut3DSizeStr[8];
	sprintf_s(lut3DSizeStr, "%u", m_Lut3DSize);
	ShaderMacros[i++] = { "LUT3D_SIZE", lut3DSizeStr };
	ShaderMacros[i++] = { "DITHER_ENABLED", bDither ? "1" : "0" };

	CString ErrorMessage;
	hr = m_pPSC->CompileShader(srcdata, "main", m_ShaderProfile, 0, ShaderMacros, &m_pFinalPixelShader, &ErrorMessage);
	if (FAILED(hr)) {
		DLog(L"CDX9RenderingEngine::InitFinalPass() : shader compilation failed\n%s", ErrorMessage.GetString());
		ASSERT(0);
		CleanupFinalPass();
		return hr;
	}

	UpdateFinalPassStr();

	return S_OK;
}

void CDX9RenderingEngine::UpdateFinalPassStr()
{
	if (m_strFinalPass.IsEmpty()) {
		if (m_bColorManagement) {
			m_strFinalPass = L" ColorManagement(";
			switch (m_AmbientLight) {
			case AMBIENT_LIGHT_BRIGHT: m_strFinalPass.Append(L"Bright"); break;
			case AMBIENT_LIGHT_DIM:    m_strFinalPass.Append(L"Dim");    break;
			case AMBIENT_LIGHT_DARK:   m_strFinalPass.Append(L"Dark");   break;
			}
			switch (m_RenderingIntent) {
			case COLOR_RENDERING_INTENT_PERCEPTUAL:            m_strFinalPass.Append(L",Perceptual");           break;
			case COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC: m_strFinalPass.Append(L",Relative�olorimetric"); break;
			case COLOR_RENDERING_INTENT_SATURATION:            m_strFinalPass.Append(L",Saturation");           break;
			case COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC: m_strFinalPass.Append(L",Absolute�olorimetric"); break;
			}
			m_strFinalPass.Append(L")");
		}

		if (m_bDither) {
			if (m_bColorManagement) {
				m_strFinalPass.Append(L", ");
			}
			m_strFinalPass.Append(L"Dither");
		}
	}
}

void CDX9RenderingEngine::CleanupFinalPass()
{
	m_bDither = false;
	m_bFinalPass = false;
	m_bColorManagement = false;
	m_pDitherTexture.Release();
	m_pLut3DTexture.Release();
	m_pFinalPixelShader.Release();
	m_strFinalPass.Empty();
}

HRESULT CDX9RenderingEngine::CreateIccProfileLut(wchar_t* profilePath, float* lut3D)
{
	// Get the input video system
	VideoSystem videoSystem;

	if (m_InputVideoSystem == VIDEO_SYSTEM_UNKNOWN) {
		// DVD-Video and D-1
		static const long ntscSizes[][2] = { {720, 480}, {704, 480}, {352, 480}, {352, 240}, {720, 486} };
		static const long palSizes[][2] = { {720, 576}, {704, 576}, {352, 576}, {352, 288} };

		videoSystem = VIDEO_SYSTEM_HDTV; // default

		for (unsigned i = 0; i < std::size(ntscSizes); i++) {
			if (m_nativeVideoSize.cx == ntscSizes[i][0] && m_nativeVideoSize.cy == ntscSizes[i][1]) {
				videoSystem = VIDEO_SYSTEM_SDTV_NTSC;
				break;
			}
		}

		for (unsigned i = 0; i < std::size(palSizes); i++) {
			if (m_nativeVideoSize.cx == palSizes[i][0] && m_nativeVideoSize.cy == palSizes[i][1]) {
				videoSystem = VIDEO_SYSTEM_SDTV_PAL;
				break;
			}
		}
	} else {
		videoSystem = m_InputVideoSystem;
	}

	// Get the gamma
	double gamma;

	switch (m_AmbientLight) {
	case AMBIENT_LIGHT_BRIGHT: gamma = 2.2;  break;
	case AMBIENT_LIGHT_DIM:    gamma = 2.35; break;
	case AMBIENT_LIGHT_DARK:   gamma = 2.4;  break;
	default:
		return E_INVALIDARG;
	}

	// Get the rendering intent
	cmsUInt32Number intent;

	switch (m_RenderingIntent) {
	case COLOR_RENDERING_INTENT_PERCEPTUAL:            intent = INTENT_PERCEPTUAL;            break;
	case COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC: intent = INTENT_RELATIVE_COLORIMETRIC; break;
	case COLOR_RENDERING_INTENT_SATURATION:            intent = INTENT_SATURATION;            break;
	case COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC: intent = INTENT_ABSOLUTE_COLORIMETRIC; break;
	default:
		return E_INVALIDARG;
	}

	// Set the input white point. It's D65 in all cases.
	cmsCIExyY whitePoint;

	whitePoint.x = 0.312713;
	whitePoint.y = 0.329016;
	whitePoint.Y = 1.0;

	// Set the input primaries
	cmsCIExyYTRIPLE primaries;

	switch (videoSystem) {
		case VIDEO_SYSTEM_HDTV:
			// Rec. 709
			primaries.Red.x   = 0.64;
			primaries.Red.y   = 0.33;
			primaries.Green.x = 0.30;
			primaries.Green.y = 0.60;
			primaries.Blue.x  = 0.15;
			primaries.Blue.y  = 0.06;
			break;

		case VIDEO_SYSTEM_SDTV_NTSC:
			// SMPTE-C
			primaries.Red.x   = 0.630;
			primaries.Red.y   = 0.340;
			primaries.Green.x = 0.310;
			primaries.Green.y = 0.595;
			primaries.Blue.x  = 0.155;
			primaries.Blue.y  = 0.070;
			break;

		case VIDEO_SYSTEM_SDTV_PAL:
			// PAL/SECAM
			primaries.Red.x   = 0.64;
			primaries.Red.y   = 0.33;
			primaries.Green.x = 0.29;
			primaries.Green.y = 0.60;
			primaries.Blue.x  = 0.15;
			primaries.Blue.y  = 0.06;
			break;

		default:
			return E_FAIL;
	}

	primaries.Red.Y   = 1.0;
	primaries.Green.Y = 1.0;
	primaries.Blue.Y  = 1.0;

	// Set the input gamma, which is the gamma of a reference studio display we want to simulate
	// For more information, see the paper at http://www.poynton.com/notes/PU-PR-IS/Poynton-PU-PR-IS.pdf
	cmsToneCurve* transferFunction = cmsBuildGamma(0, gamma);

	cmsToneCurve* transferFunctionRGB[3] = { transferFunction, transferFunction, transferFunction};

	// Create the input profile
	cmsHPROFILE hInputProfile = cmsCreateRGBProfile(&whitePoint, &primaries, transferFunctionRGB);
	cmsFreeToneCurve(transferFunction);

	if (hInputProfile == nullptr) {
		return E_FAIL;
	}

	// Open the output profile
	cmsHPROFILE hOutputProfile;
	FILE* outputProfileStream;

	if (profilePath && *profilePath) {
		if (_wfopen_s(&outputProfileStream, profilePath, L"rb") != 0) {
			cmsCloseProfile(hInputProfile);
			return E_FAIL;
		}

		hOutputProfile = cmsOpenProfileFromStream(outputProfileStream, "r");
	} else {
		hOutputProfile = cmsCreate_sRGBProfile();
	}

	if (hOutputProfile == nullptr) {
		if (profilePath != 0) {
			fclose(outputProfileStream);
		}

		cmsCloseProfile(hInputProfile);
		return E_FAIL;
	}

	// Create the transform
	cmsHTRANSFORM hTransform = cmsCreateTransform(hInputProfile, TYPE_RGB_16, hOutputProfile, TYPE_RGB_16, intent, cmsFLAGS_HIGHRESPRECALC);

	cmsCloseProfile(hOutputProfile);

	if (profilePath != 0) {
		fclose(outputProfileStream);
	}

	cmsCloseProfile(hInputProfile);

	if (hTransform == nullptr) {
		return E_FAIL;
	}

	// Create the 3D LUT input
	uint16_t* lut3DOutput = DNew uint16_t[m_Lut3DEntryCount * 3];
	uint16_t* lut3DInput  = DNew uint16_t[m_Lut3DEntryCount * 3];

	uint16_t* lut3DInputIterator = lut3DInput;

	for (unsigned b = 0; b < m_Lut3DSize; b++) {
		for (unsigned g = 0; g < m_Lut3DSize; g++) {
			for (unsigned r = 0; r < m_Lut3DSize; r++) {
				*lut3DInputIterator++ = r * 65535 / (m_Lut3DSize - 1);
				*lut3DInputIterator++ = g * 65535 / (m_Lut3DSize - 1);
				*lut3DInputIterator++ = b * 65535 / (m_Lut3DSize - 1);
			}
		}
	}

	// Do the transform
	cmsDoTransform(hTransform, lut3DInput, lut3DOutput, m_Lut3DEntryCount);

	// Convert the output to floating point
	for (unsigned i = 0; i < m_Lut3DEntryCount * 3; i++) {
		lut3D[i] = static_cast<float>(lut3DOutput[i]) * (1.0f / 65535.0f);
	}

	// Cleanup
	delete[] lut3DOutput;
	delete[] lut3DInput;
	cmsDeleteTransform(hTransform);

	return S_OK;
}

HRESULT CDX9RenderingEngine::FinalPass(IDirect3DTexture9* pTexture)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	float w = (float)desc.Width;
	float h = (float)desc.Height;

	MYD3DVERTEX<1> v[] = {
		{   -0.5f,    -0.5f, 0.5f, 2.0f, 0, 0},
		{w - 0.5f,    -0.5f, 0.5f, 2.0f, 1, 0},
		{   -0.5f, h - 0.5f, 0.5f, 2.0f, 0, 1},
		{w - 0.5f, h - 0.5f, 0.5f, 2.0f, 1, 1},
	};

	hr = m_pDevice9Ex->SetPixelShader(m_pFinalPixelShader);

	// Set sampler: image
	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	hr = m_pDevice9Ex->SetTexture(0, pTexture);

	// Set sampler: ditherMap
	hr = m_pDevice9Ex->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	hr = m_pDevice9Ex->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	hr = m_pDevice9Ex->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	hr = m_pDevice9Ex->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	hr = m_pDevice9Ex->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

	hr = m_pDevice9Ex->SetTexture(1, m_pDitherTexture);

	if (m_bColorManagement) {
		hr = m_pDevice9Ex->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		hr = m_pDevice9Ex->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		hr = m_pDevice9Ex->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		hr = m_pDevice9Ex->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = m_pDevice9Ex->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		hr = m_pDevice9Ex->SetSamplerState(2, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

		hr = m_pDevice9Ex->SetTexture(2, m_pLut3DTexture);
	}

	// Set constants
	float fConstData[][4] = {
		{w / DITHER_MATRIX_SIZE, h / DITHER_MATRIX_SIZE, 0.0f, 0.0f}
	};
	hr = m_pDevice9Ex->SetPixelShaderConstantF(0, (float*)fConstData, std::size(fConstData));

	hr = TextureBlt(m_pDevice9Ex, v, D3DTEXF_POINT);

	hr = m_pDevice9Ex->SetTexture(1, nullptr);

	if (m_bColorManagement) {
		hr = m_pDevice9Ex->SetTexture(2, nullptr);
	}

	return hr;
}

HRESULT CDX9RenderingEngine::TextureCopy(IDirect3DTexture9* pTexture)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	float w = (float)desc.Width - 0.5f;
	float h = (float)desc.Height - 0.5f;

	MYD3DVERTEX<1> v[] = {
		{-0.5f, -0.5f, 0.5f, 2.0f, 0, 0},
		{    w, -0.5f, 0.5f, 2.0f, 1, 0},
		{-0.5f,     h, 0.5f, 2.0f, 0, 1},
		{    w,     h, 0.5f, 2.0f, 1, 1},
	};

	hr = m_pDevice9Ex->SetTexture(0, pTexture);

	return TextureBlt(m_pDevice9Ex, v, D3DTEXF_POINT);
}

bool CDX9RenderingEngine::ClipToSurface(IDirect3DSurface9* pSurface, CRect& s, CRect& d)
{
	D3DSURFACE_DESC d3dsd;
	if (FAILED(pSurface->GetDesc(&d3dsd))) {
		return false;
	}

	int w = d3dsd.Width, h = d3dsd.Height;
	int sw = s.Width(), sh = s.Height();
	int dw = d.Width(), dh = d.Height();

	if (d.left >= w || d.right < 0 || d.top >= h || d.bottom < 0
			|| sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) {
		s.SetRectEmpty();
		d.SetRectEmpty();
		return true;
	}

	if (d.right > w) {
		s.right -= (d.right-w)*sw/dw;
		d.right = w;
	}
	if (d.bottom > h) {
		s.bottom -= (d.bottom-h)*sh/dh;
		d.bottom = h;
	}
	if (d.left < 0) {
		s.left += (0-d.left)*sw/dw;
		d.left = 0;
	}
	if (d.top < 0) {
		s.top += (0-d.top)*sh/dh;
		d.top = 0;
	}

	return true;
}

HRESULT CDX9RenderingEngine::DrawRect(const D3DCOLOR _Color, const CRect &_Rect)
{
	if (!m_pDevice9Ex) {
		return E_POINTER;
	}

	MYD3DVERTEX<0> v[] = {
		{float(_Rect.left), float(_Rect.top), 0.5f, 2.0f, _Color},
		{float(_Rect.right), float(_Rect.top), 0.5f, 2.0f, _Color},
		{float(_Rect.left), float(_Rect.bottom), 0.5f, 2.0f, _Color},
		{float(_Rect.right), float(_Rect.bottom), 0.5f, 2.0f, _Color},
	};

	for (unsigned i = 0; i < std::size(v); i++) {
		v[i].x -= 0.5f;
		v[i].y -= 0.5f;
	}

	HRESULT hr = m_pDevice9Ex->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	//D3DRS_COLORVERTEX
	hr = m_pDevice9Ex->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	hr = m_pDevice9Ex->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);

	hr = m_pDevice9Ex->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX0 | D3DFVF_DIFFUSE);
	// hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));
	std::swap(v[2], v[3]);
	hr = m_pDevice9Ex->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));

	return S_OK;
}

HRESULT CDX9RenderingEngine::AlphaBlt(RECT* pSrc, RECT* pDst, IDirect3DTexture9* pTexture)
{
	if (!pSrc || !pDst) {
		return E_POINTER;
	}

	CRect src(*pSrc), dst(*pDst);

	HRESULT hr;

	D3DSURFACE_DESC d3dsd;
	if (FAILED(pTexture->GetLevelDesc(0, &d3dsd)) /*|| d3dsd.Type != D3DRTYPE_TEXTURE*/) {
		return E_FAIL;
	}

	float w = (float)d3dsd.Width;
	float h = (float)d3dsd.Height;

	struct {
		float x, y, z, rhw;
		float tu, tv;
	}
	pVertices[] = {
		{(float)dst.left,  (float)dst.top,    0.5f, 2.0f, (float)src.left / w,  (float)src.top / h},
		{(float)dst.right, (float)dst.top,    0.5f, 2.0f, (float)src.right / w, (float)src.top / h},
		{(float)dst.left,  (float)dst.bottom, 0.5f, 2.0f, (float)src.left / w,  (float)src.bottom / h},
		{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / w, (float)src.bottom / h},
	};

	for (auto& pVertice : pVertices) {
		pVertice.x -= 0.5f;
		pVertice.y -= 0.5f;
	}

	hr = m_pDevice9Ex->SetTexture(0, pTexture);

	// GetRenderState fails for devices created with D3DCREATE_PUREDEVICE
	// so we need to provide default values in case GetRenderState fails
	DWORD abe, sb, db;
	if (FAILED(m_pDevice9Ex->GetRenderState(D3DRS_ALPHABLENDENABLE, &abe)))
		abe = FALSE;
	if (FAILED(m_pDevice9Ex->GetRenderState(D3DRS_SRCBLEND, &sb)))
		sb = D3DBLEND_ONE;
	if (FAILED(m_pDevice9Ex->GetRenderState(D3DRS_DESTBLEND, &db)))
		db = D3DBLEND_ZERO;

	hr = m_pDevice9Ex->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	hr = m_pDevice9Ex->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src and ...
	hr = m_pDevice9Ex->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA); // ... inverse alpha channel for dst

	hr = m_pDevice9Ex->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	hr = m_pDevice9Ex->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	hr = m_pDevice9Ex->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_pDevice9Ex->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	/*//

	D3DCAPS9 d3dcaps9;
	hr = m_pD3DDev->GetDeviceCaps(&d3dcaps9);
	if (d3dcaps9.AlphaCmpCaps & D3DPCMPCAPS_LESS)
	{
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)0x000000FE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DPCMPCAPS_LESS);
	}

	*///

	hr = m_pDevice9Ex->SetPixelShader(nullptr);

	hr = m_pDevice9Ex->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
	hr = m_pDevice9Ex->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

	//

	m_pDevice9Ex->SetTexture(0, nullptr);

	m_pDevice9Ex->SetRenderState(D3DRS_ALPHABLENDENABLE, abe);
	m_pDevice9Ex->SetRenderState(D3DRS_SRCBLEND, sb);
	m_pDevice9Ex->SetRenderState(D3DRS_DESTBLEND, db);

	return S_OK;
}

HRESULT CDX9RenderingEngine::ClearCustomPixelShaders(int target)
{
	if (target == TARGET_FRAME) {
		m_pCustomPixelShaders.clear();
	} else if (target == TARGET_SCREEN) {
		m_pCustomScreenSpacePixelShaders.clear();
	} else {
		return E_INVALIDARG;
	}
	m_pDevice9Ex->SetPixelShader(nullptr);

	return S_OK;
}

HRESULT CDX9RenderingEngine::AddCustomPixelShader(int target, LPCSTR sourceCode, LPCSTR profile)
{
	std::list<CExternalPixelShader> *pPixelShaders;
	if (target == TARGET_FRAME) {
		pPixelShaders = &m_pCustomPixelShaders;
	} else if (target == TARGET_SCREEN) {
		pPixelShaders = &m_pCustomScreenSpacePixelShaders;
	} else {
		return E_INVALIDARG;
	}

	if (!sourceCode || !profile) {
		return E_INVALIDARG;
	}

	CExternalPixelShader Shader;
	Shader.m_SourceCode = sourceCode;
	Shader.m_Profile = m_ShaderProfile;

	if (Shader.m_Profile.Compare(profile) < 0) {
		// shader is not supported by hardware
		return E_FAIL;
	}

	CComPtr<IDirect3DPixelShader9> pPixelShader;

	HRESULT hr = Shader.Compile(m_pPSC.get());
	if (FAILED(hr)) {
		return hr;
	}

	pPixelShaders->push_back(Shader);

	Paint(false);

	return S_OK;
}

HRESULT CDX9RenderingEngine::InitCorrectionPass(const AM_MEDIA_TYPE& input_mt)
{
	if (m_Caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
		return E_ABORT;
	}

	HRESULT hr = S_OK;

	DXVA2_ExtendedFormat extformat = {};
	if (input_mt.formattype == FORMAT_VideoInfo2) {
		extformat.value = ((VIDEOINFOHEADER2*)input_mt.pbFormat)->dwControlFlags;
	}

	if (m_inputExtFormat.value != extformat.value || !m_pPSCorrection) {
		m_wsCorrection = nullptr;

		// DXVA2_ExtendedFormat was chaged
		m_inputExtFormat.value = extformat.value;
		m_pPSCorrection.Release();

		if (extformat.VideoTransferMatrix == VIDEOTRANSFERMATRIX_YCgCo) {
			hr = CreateShaderFromResource(m_pDevice9Ex, &m_pPSCorrection, IDF_SHADER_CORRECTION_YCGCO);
			m_wsCorrection = L"Fix YCgCo";
		}
		else if (extformat.VideoTransferFunction == MFVideoTransFunc_2084) {
			hr = CreateShaderFromResource(m_pDevice9Ex, &m_pPSCorrection, IDF_SHADER_CONVERT_PQ_TO_SDR);
			m_wsCorrection = L"Convert PQ to SDR";
		}
		else if (extformat.VideoPrimaries == MFVideoPrimaries_BT2020) {
			hr = CreateShaderFromResource(m_pDevice9Ex, &m_pPSCorrection, IDF_SHADER_CONVERT_2020_TO_709);
			m_wsCorrection = L"Convert BT.2020 to BT.709";
		}
		else if (m_D3D9VendorId == PCIV_nVidia && (input_mt.subtype == MEDIASUBTYPE_YUY2 || input_mt.subtype == MEDIASUBTYPE_UYVY)) {
			hr = CreateShaderFromResource(m_pDevice9Ex, &m_pPSCorrection, IDF_SHADER_CORRECTION_422);
			m_wsCorrection = L"Fix Nvidia YUY2";
		}
	}

	return hr;
}

// IAllocatorPresenter

STDMETHODIMP_(SIZE) CDX9RenderingEngine::GetVideoSize()
{
	SIZE size = __super::GetVideoSize();

	if (m_iRotation == 90 || m_iRotation == 270) {
		std::swap(size.cx, size.cy);
	}

	return size;
}

STDMETHODIMP_(SIZE) CDX9RenderingEngine::GetVideoSizeAR()
{
	SIZE size = __super::GetVideoSizeAR();

	if (m_iRotation == 90 || m_iRotation == 270) {
		std::swap(size.cx, size.cy);
	}

	return size;
}

STDMETHODIMP CDX9RenderingEngine::SetRotation(int rotation)
{
	if (AngleStep90(rotation)) {
		if (rotation != m_iRotation) {
			m_iRotation = rotation;
			m_bOtherTransform = true;
		}

		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP_(int) CDX9RenderingEngine::GetRotation()
{
	return m_iRotation;
}

STDMETHODIMP CDX9RenderingEngine::SetFlip(bool flip)
{
	if (flip != m_bFlip) {
		m_bFlip = flip;
		m_bOtherTransform = true;
	}

	return S_OK;
}

STDMETHODIMP_(bool) CDX9RenderingEngine::GetFlip()
{
	return m_bFlip;
}

// ISubRenderOptions

STDMETHODIMP CDX9RenderingEngine::GetInt(LPCSTR field, int* value)
{
	CheckPointer(value, E_POINTER);
	if (strcmp(field, "rotation") == 0) {
		*value = m_iRotation;

		return S_OK;
	}

	if (strcmp(field, "supportedLevels") == 0) {
		if (m_ExtraSets.iEVROutputRange == 1) {
			*value = 3; // TV preferred
		} else {
			*value = 2; // PC preferred
		}

		return S_OK;
	}

	return __super::GetInt(field, value);
}

STDMETHODIMP CDX9RenderingEngine::GetString(LPCSTR field, LPWSTR* value, int* chars)
{
	CheckPointer(value, E_POINTER);
	CheckPointer(chars, E_POINTER);

	if (!strcmp(field, "name")) {
		CStringW ret = L"MPC-BE: EVR (custom presenter)";
		int len = ret.GetLength();
		size_t sz = (len + 1) * sizeof(WCHAR);
		LPWSTR buf = (LPWSTR)LocalAlloc(LPTR, sz);

		if (!buf) {
			return E_OUTOFMEMORY;
		}

		wcscpy_s(buf, len + 1, ret);
		*chars = len;
		*value = buf;

		return S_OK;
	}

	return __super::GetString(field, value, chars);
}
