/*
 *      Copyright (C) 2022      Benoit Plourde
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
 */

#include "stdafx.h"

 //#include "ID3DVideoMemoryConfiguration.h"
 //#include "dxva2/dxva_common.h"

#include "d3dx12.h"
#include <moreuuids.h>
#include "DSUtil/DSUtil.h"
#include "DSUtil/DXVAState.h"
#include "d3d12decoder.h"
#include "../MPCVideoDec.h"
#include "../DxgiUtils.h"


#pragma warning(push)
#pragma warning(disable: 4005)

//we dont link the d3d12 library in debug because we only need it for the debug interface
#ifdef DEBUG_OR_LOG
#pragma comment( lib, "d3d12.lib" )
#endif
extern "C"
{
#include <ExtLib/ffmpeg/libavcodec/avcodec.h>
#include <ExtLib/ffmpeg/libavutil/opt.h>
#include <ExtLib/ffmpeg/libavutil/pixdesc.h>
#include <ExtLib/ffmpeg/libavcodec/d3d12va.h>
}

#define countof(array) (sizeof(array) / sizeof(array[0]))

#define ROUND_UP_128(num) (((num) + 127) & ~127)

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

static DXGI_FORMAT d3d12va_map_sw_to_hw_format(enum AVPixelFormat pix_fmt)
{
  switch (pix_fmt)
  {
  case AV_PIX_FMT_YUV420P10:
  case AV_PIX_FMT_P010: return DXGI_FORMAT_P010;
  case AV_PIX_FMT_NV12:
  default: return DXGI_FORMAT_NV12;
  }
}

////////////////////////////////////////////////////////////////////////////////
// D3D12 decoder implementation
////////////////////////////////////////////////////////////////////////////////

CD3D12Decoder::CD3D12Decoder(CMPCVideoDecFilter* pFilter)
  : m_pFilter(pFilter)
{
  m_pD3DCommands = nullptr;
  m_pD3DDevice = nullptr;
  m_pVideoDevice = nullptr;
  m_pDxgiAdapter = nullptr;
  m_pDxgiFactory = nullptr;
  m_pVideoDecoder = nullptr;
  m_pD3DDebug = nullptr;
  m_pD3DDebug1 = nullptr;
  for (int x = 0; x < MAX_SURFACE_COUNT; x++) {
    m_pTexture[x] = nullptr;
  }
}

CD3D12Decoder::~CD3D12Decoder(void)
{
  DestroyDecoder(true);
  if (m_pAllocator)
    m_pAllocator->DecoderDestruct();
  SAFE_RELEASE(m_pAllocator);
}

void CD3D12Decoder::DestroyDecoder(const bool bFull)
{
  m_pD3DCommands = nullptr;
  SAFE_RELEASE(m_pD3DDebug);
  SAFE_RELEASE(m_pD3DDebug1);
  SAFE_RELEASE(m_pDxgiAdapter);
  for (int x = 0; x < MAX_SURFACE_COUNT; x++) {
    SAFE_RELEASE(m_pTexture[x]);
  }
  SAFE_RELEASE(m_pVideoDevice);
  SAFE_RELEASE(m_pDxgiFactory);
  SAFE_RELEASE(m_pVideoDecoder);
  SAFE_RELEASE(m_pD3DDevice);



  if (bFull) {
    if (m_dxLib.d3d12lib) {
      FreeLibrary(m_dxLib.d3d12lib);
      m_dxLib.d3d12lib = nullptr;
    }
  }
}

// ILAVDecoder
STDMETHODIMP CD3D12Decoder::Init()
{
  // D3D12 decoding requires Windows 8 or newer
  if (!SysVersion::IsWin8orLater())
    return E_NOINTERFACE;

  m_dxLib.d3d12lib = LoadLibrary(L"d3d12.dll");
  if (m_dxLib.d3d12lib == nullptr)
  {
    DbgLog((LOG_TRACE, 10, L"Cannot open d3d11.dll"));
    return E_FAIL;
  }

  m_dxLib.mD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(m_dxLib.d3d12lib, "D3D12CreateDevice");
  if (m_dxLib.mD3D12CreateDevice == nullptr)
  {
    DbgLog((LOG_TRACE, 10, L"D3D12CreateDevice not available"));
    return E_FAIL;
  }

  m_dxLib.dxgilib = LoadLibrary(L"dxgi.dll");
  if (m_dxLib.dxgilib == nullptr)
  {
    DbgLog((LOG_TRACE, 10, L"Cannot open dxgi.dll"));
    return E_FAIL;
  }

  m_dxLib.mCreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(m_dxLib.dxgilib, "CreateDXGIFactory2");
  if (m_dxLib.mCreateDXGIFactory2 == nullptr)
  {
    DbgLog((LOG_TRACE, 10, L"CreateDXGIFactory1 not available"));
    return E_FAIL;
  }

  return S_OK;
}

STDMETHODIMP CD3D12Decoder::InitAllocator(IMemAllocator** ppAlloc)
{
  if (m_pAllocator == nullptr) {
    HRESULT hr = S_FALSE;
    m_pAllocator = new(std::nothrow) CD3D12SurfaceAllocator(this, &hr);
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

STDMETHODIMP CD3D12Decoder::CreateD3D12Device(UINT nDeviceIndex)
{
  HRESULT hr = S_OK;
#ifdef DEBUG_OR_LOG
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_pD3DDebug))))
  {
    m_pD3DDebug->EnableDebugLayer();
    m_pD3DDebug->QueryInterface(IID_PPV_ARGS(&m_pD3DDebug1));
    m_pD3DDebug1->SetEnableSynchronizedCommandQueueValidation(1);
  }
#endif
  // create DXGI factory
  IDXGIAdapter* pDXGIAdapter = nullptr;
  IDXGIFactory1* pDXGIFactory = nullptr;
  hr = m_dxLib.mCreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_pDxgiFactory));
  if (FAILED(hr))
  {
    DbgLog((LOG_ERROR, 10, L"-> DXGIFactory creation failed"));
  }

  if FAILED(hr)
    assert(0);

  if (DXGI_ERROR_NOT_FOUND == m_pDxgiFactory->EnumAdapters(nDeviceIndex, &m_pDxgiAdapter))
  {
    hr = S_FALSE;
    ASSERT(0);
  }
  DXGI_ADAPTER_DESC desc;
  hr = m_pDxgiAdapter->GetDesc(&desc);
  if FAILED(hr)
    ASSERT(0);
  const D3D_FEATURE_LEVEL* levels = NULL;
  D3D_FEATURE_LEVEL max_level = D3D_FEATURE_LEVEL_12_1;

  hr = m_dxLib.mD3D12CreateDevice(m_pDxgiAdapter, max_level, IID_PPV_ARGS(&m_pD3DDevice));
  if FAILED(hr)
    ASSERT(0);
  hr = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&m_pVideoDevice));
  return hr;
}

STDMETHODIMP CD3D12Decoder::PostConnect(AVCodecContext* c, IPin* pPin)
{

  DbgLog((LOG_TRACE, 10, L"CD3D12Decoder::PostConnect()"));

  if (m_pAllocator) {
    m_pAllocator->DecoderDestruct();
    SAFE_RELEASE(m_pAllocator);
  }

  HRESULT hr = S_OK;
  ID3D12DecoderConfiguration* pD3D12DecoderConfiguration = nullptr;
  hr = pPin->QueryInterface(&pD3D12DecoderConfiguration);
  if (FAILED(hr)) {
    //if we couldn't query the interface d3d12 renderer is not on the other end
    //i didn't implement copyback the renderer can already convert sd to d3d12 texture
    DbgLog((LOG_ERROR, 10, L"-> ID3D12DecoderConfiguration not available, using fallback mode"));
    return E_FAIL;
  }

  UINT nDevice = pD3D12DecoderConfiguration->GetD3D12AdapterIndex();

  if (!m_pD3DDevice) {
    hr = CreateD3D12Device(nDevice);
  }
  //dont need to check the subtype of the output since we are only compatible with mpcvideorenderer
  //might change in the future but right now not needed

  // Give the renderer our d3d device
  hr = pD3D12DecoderConfiguration->ActivateD3D12Decoding(m_pD3DDevice, (HANDLE)0, 0);
  SAFE_RELEASE(pD3D12DecoderConfiguration);
  return S_OK;
}

HRESULT CD3D12Decoder::DeliverFrame()
{
  CheckPointer(m_pFilter->m_pFrame, S_FALSE);
  AVFrame* pFrame = m_pFilter->m_pFrame;
  CD3D12MediaSample* pD3D12Sample = (CD3D12MediaSample*)(pFrame->data[3]);
  pD3D12Sample->SetD3D12Texture(m_pTexture[(int)pFrame->data[1]]);
  IMediaSample* pSample = pD3D12Sample;
  CheckPointer(pSample, S_FALSE);

  REFERENCE_TIME rtStop, rtStart;
  m_pFilter->GetFrameTimeStamp(pFrame, rtStart, rtStop);
  m_pFilter->UpdateFrameTime(rtStart, rtStop);

  if (rtStart < 0) {
    return S_OK;
  }

  DXVA2_ExtendedFormat dxvaExtFormat = m_pFilter->GetDXVA2ExtendedFormat(m_pFilter->m_pAVCtx, pFrame);

  m_pFilter->ReconnectOutput(m_pFilter->PictWidth(), m_pFilter->PictHeight(), false, m_pFilter->GetFrameDuration(), &dxvaExtFormat);

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
    }
    else if (sendmt->formattype == FORMAT_VideoInfo2) {
      VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)sendmt->pbFormat;
      pBMI = &vih2->bmiHeader;
    }
    else {
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

void CD3D12Decoder::AdditionaDecoderInit(AVCodecContext* c)
{
  c->thread_count = 1;
  c->get_format = get_d3d12_format;
  c->get_buffer2 = get_d3d12_buffer;
  c->opaque = this;
  c->slice_flags |= SLICE_FLAG_ALLOW_FIELD;
  // disable error concealment in hwaccel mode, it doesn't work either way
  c->error_concealment = 0;
  av_opt_set_int(c, "enable_er", 0, AV_OPT_SEARCH_CHILDREN);
}

void CD3D12Decoder::PostInitDecoder(AVCodecContext* c)
{
  HRESULT hr = S_OK;
  DLog(L"CD3D12Decoder::PostInitDecoder()");

  // Destroy old decoder
  DestroyDecoder(false);

  // Ensure software pixfmt is set, so hardware accels can use it immediately
  if (c->sw_pix_fmt == AV_PIX_FMT_NONE && c->pix_fmt != AV_PIX_FMT_DXVA2_VLD && c->pix_fmt != AV_PIX_FMT_D3D11 && c->pix_fmt != AV_PIX_FMT_D3D12_VLD) {
    c->sw_pix_fmt = c->pix_fmt;
  }

  // initialize surface format to ensure the default media type is set properly
  m_SurfaceFormat = d3d12va_map_sw_to_hw_format(c->sw_pix_fmt);
  m_dwSurfaceWidth = dxva_align_dimensions(c->codec_id, c->coded_width);
  m_dwSurfaceHeight = dxva_align_dimensions(c->codec_id, c->coded_height);
}

enum AVPixelFormat CD3D12Decoder::get_d3d12_format(struct AVCodecContext* s, const enum AVPixelFormat* pix_fmts)
{
  CD3D12Decoder* pDec = (CD3D12Decoder*)s->opaque;
  const enum AVPixelFormat* p;
  for (p = pix_fmts; *p != -1; p++) {
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(*p);

    if (!desc || !(desc->flags & AV_PIX_FMT_FLAG_HWACCEL))
      break;

    if (*p == AV_PIX_FMT_D3D12_VLD) {
      HRESULT hr = pDec->ReInitD3D12Decoder(s);
      if (FAILED(hr)) {
        pDec->m_pFilter->m_bFailD3D12Decode = TRUE;
        continue;
      }
      else {
        break;
      }
    }
  }
  return *p;
}

int CD3D12Decoder::get_d3d12_buffer(struct AVCodecContext* c, AVFrame* frame, int flags)
{
  CD3D12Decoder* pDec = (CD3D12Decoder*)c->opaque;
  HRESULT hr = S_OK;

  if (frame->format != AV_PIX_FMT_D3D12_VLD) {
    DLog(L"CD3D12Decoder::get_d3d12_buffer() : D3D12 buffer request, but not AV_PIX_FMT_D3D12_VLD");
    return -1;
  }

  //av1 not implemented for d3d12 yet
  if (!pDec->m_pFilter->CheckDXVACompatible(c->codec_id, c->sw_pix_fmt, c->profile) && c->codec_id != AV_CODEC_ID_AV1) {
    DLog(L"CD3D11Decoder::get_d3d12_buffer() : format not compatible with DXVA");
    pDec->m_pFilter->m_bDXVACompatible = false;
    return -1;
  }

  hr = pDec->ReInitD3D12Decoder(c);

  if (FAILED(hr)) {
    pDec->m_pFilter->m_bFailD3D12Decode = TRUE;
    return -1;
  }

  if (pDec->m_pAllocator) {
    IMediaSample* pSample = nullptr;
    hr = pDec->m_pAllocator->GetBuffer(&pSample, nullptr, nullptr, 0);
    if (SUCCEEDED(hr)) {
      CD3D12MediaSample* pD3D12Sample = dynamic_cast<CD3D12MediaSample*>(pSample);

      // fill the frame from the sample, including a reference to the sample
      pD3D12Sample->GetAVFrameBuffer(frame);

      frame->width = c->coded_width;
      frame->height = c->coded_height;
      //hack because lavfilters and mpcvideodec need to have something in
      //data[0] to go forward in the decode loops
      frame->data[0] = (uint8_t*)1;
      pD3D12Sample->Release();
      return 0;
    }
  }
  return -1;
}

STDMETHODIMP_(long) CD3D12Decoder::GetBufferCount(long* pMaxBuffers)
{
  long buffers = 16;
  // Buffers based on max ref frames
  if (m_pFilter->m_CodecId == AV_CODEC_ID_H264 || m_pFilter->m_CodecId == AV_CODEC_ID_HEVC)
    buffers = 16;
  else if (m_pFilter->m_CodecId == AV_CODEC_ID_VP9)
    buffers = 8;
  else
    buffers = 2;
  buffers += 4;

  if (pMaxBuffers)
  {
    // cap at 127, because it needs to fit into the 7-bit DXVA structs
    *pMaxBuffers = 127;
    // VC-1 and VP9 decoding has stricter requirements (decoding flickers otherwise)
    if (m_pFilter->m_CodecId == AV_CODEC_ID_VC1 || m_pFilter->m_CodecId == AV_CODEC_ID_VP9)
      *pMaxBuffers = 32;
  }

  return buffers;

}

void CD3D12Decoder::Flush()
{
  DbgLog((LOG_TRACE, 10, L"CDecD3D12::Flush"));
  ref_free_index.clear();
  ref_free_index.resize(0);
  for (int i = 0; i < GetBufferCount(); i++)
    ref_free_index.push_back(i);
}

STDMETHODIMP CD3D12Decoder::FindVideoServiceConversion(AVCodecContext* c, enum AVCodecID codec, int profile, DXGI_FORMAT surface_format, GUID* input)
{
  HRESULT hr = S_OK;
  *input = GUID_NULL;
  if (codec == AV_CODEC_ID_MPEG2VIDEO)
    *input = D3D12_VIDEO_DECODE_PROFILE_MPEG1_AND_MPEG2;
  if (codec == AV_CODEC_ID_HEVC) {
    //need some testing
    if (!((profile) <= FF_PROFILE_HEVC_MAIN_10))
      *input = D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN;
    else
      *input = D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN10;
  }
  const int h264mb_count = (c->coded_width / 16) * (c->coded_height / 16);

  if (codec == AV_CODEC_ID_H264) {
    if (profile != FF_PROFILE_UNKNOWN && !(((profile) & ~FF_PROFILE_H264_CONSTRAINED) <= FF_PROFILE_H264_HIGH))
      *input = D3D12_VIDEO_DECODE_PROFILE_H264;
    *input = D3D12_VIDEO_DECODE_PROFILE_H264;
  }


  if ((codec == AV_CODEC_ID_WMV3 || codec == AV_CODEC_ID_VC1) && profile == FF_PROFILE_VC1_COMPLEX) {
    *input = D3D12_VIDEO_DECODE_PROFILE_VC1;
    //need to find which one we need
    //D3D12_VIDEO_DECODE_PROFILE_VC1_D2010

  }
  if (codec == AV_CODEC_ID_VP9) {
    if (profile == FF_PROFILE_VP9_2)
      *input == D3D12_VIDEO_DECODE_PROFILE_VP9_10BIT_PROFILE2;
    else
      *input = D3D12_VIDEO_DECODE_PROFILE_VP9;
  }



  if (codec == AV_CODEC_ID_AV1)
    *input = D3D12_VIDEO_DECODE_PROFILE_AV1_PROFILE0;

  if (IsEqualGUID(*input, GUID_NULL))
    return E_FAIL;
  return hr;

}

STDMETHODIMP CD3D12Decoder::ReInitD3D12Decoder(AVCodecContext* s)
{
  HRESULT hr = S_OK;
  // Don't allow decoder creation during first init
  if (m_pFilter->m_bInInit)
    return S_FALSE;

  // we need an allocator at this point
  if (m_pAllocator == nullptr)
    return E_FAIL;

  if (m_pVideoDecoder == nullptr || m_dwSurfaceWidth != dxva_align_dimensions(s->codec_id, s->coded_width) ||
    m_dwSurfaceHeight != dxva_align_dimensions(s->codec_id, s->coded_height) ||
    m_SurfaceFormat != d3d12va_map_sw_to_hw_format(s->sw_pix_fmt)) {
    hr = CreateD3D12Decoder(s);
    if (FAILED(hr)) {
      DbgLog((LOG_ERROR, 10, L"-> No video service profile found"));
      return hr;
    }
  }
  return S_OK;

}

STDMETHODIMP CD3D12Decoder::CreateD3D12Decoder(AVCodecContext* c)
{
  HRESULT hr = S_OK;

  // update surface properties
  m_dwSurfaceWidth = dxva_align_dimensions(c->codec_id, c->coded_width);
  m_dwSurfaceHeight = dxva_align_dimensions(c->codec_id, c->coded_height);

  if (m_pAllocator) {
    ALLOCATOR_PROPERTIES properties;
    hr = m_pAllocator->GetProperties(&properties);
    if (FAILED(hr))
      return hr;
    m_dwSurfaceCount = properties.cBuffers;
  }
  else {
    m_dwSurfaceCount = GetBufferCount();
  }
  D3D12_VIDEO_DECODER_DESC decoderDesc;
  D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT formats = {};
  GUID profileGUID = GUID_NULL;
  hr = FindVideoServiceConversion(c, c->codec_id, c->profile, d3d12va_map_sw_to_hw_format(c->sw_pix_fmt), &profileGUID);
  formats.Configuration.BitstreamEncryption = D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE;
  formats.Configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
  formats.Width = c->coded_width;
  formats.Height = c->coded_height;
  formats.DecodeFormat = d3d12va_map_sw_to_hw_format(c->sw_pix_fmt);
  formats.Configuration.DecodeProfile = profileGUID;
  DXGI_RATIONAL framerate = { (UINT)c->framerate.den, (UINT)c->framerate.num };
  formats.FrameRate = framerate;
  int idx, surface_idx = 0;

  int bitdepth = 8;
  //TODO add checkup for those flags in cd3d12format
  UINT supportFlags = D3D12_FORMAT_SUPPORT1_DECODER_OUTPUT | D3D12_FORMAT_SUPPORT1_SHADER_LOAD;

  m_SurfaceFormat = d3d12va_map_sw_to_hw_format(c->sw_pix_fmt);


  CD3DX12_HEAP_PROPERTIES m_textureProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_CUSTOM, 1, 1);
  CD3DX12_RESOURCE_DESC m_textureDesc = CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
    c->coded_width, c->coded_height, 1, 1, m_SurfaceFormat, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE);
  m_textureProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
  m_textureProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;
  //to do verify if device handle it
  m_textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;

  for (surface_idx = 0; surface_idx < GetBufferCount(); surface_idx++)
  {
    hr = m_pD3DDevice->CreateCommittedResource(&m_textureProp, D3D12_HEAP_FLAG_NONE,
      &m_textureDesc, D3D12_RESOURCE_STATE_VIDEO_DECODE_READ, NULL,
      IID_ID3D12Resource, (void**)&m_pTexture[surface_idx]);
    if (FAILED(hr))
      assert(0);
  }

  m_pD3D12VAContext = av_d3d12va_alloc_context();
  for (surface_idx = 0; surface_idx < GetBufferCount(); surface_idx++) {
    ref_table[surface_idx] = m_pTexture[surface_idx];
    ref_index[surface_idx] = 0;
  }
  m_pD3D12VAContext->surfaces.NumTexture2Ds = GetBufferCount();
  m_pD3D12VAContext->surfaces.ppTexture2Ds = m_pTexture;
  m_pD3D12VAContext->surfaces.pSubresources = ref_index;

  decoderDesc.Configuration.DecodeProfile = formats.Configuration.DecodeProfile;
  decoderDesc.NodeMask = 0;
  decoderDesc.Configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
  decoderDesc.Configuration.BitstreamEncryption = D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE;
  ID3D12VideoDecoder* decoder;

  hr = m_pVideoDevice->CreateVideoDecoder(&decoderDesc, IID_ID3D12VideoDecoder, (void**)&decoder);
  if (FAILED(hr))
    assert(0);
  m_pVideoDecoder = decoder;
  m_pD3D12VAContext->workaround = 0;
  m_pD3D12VAContext->report_id = 0;
  m_pD3D12VAContext->decoder = decoder;
  m_pVideoDecoderCfg.NodeMask = 0;
  m_pVideoDecoderCfg.Configuration = decoderDesc.Configuration;
  m_pVideoDecoderCfg.DecodeWidth = c->coded_width;
  m_pVideoDecoderCfg.DecodeHeight = c->coded_height;
  m_pVideoDecoderCfg.Format = m_SurfaceFormat;
  m_pVideoDecoderCfg.MaxDecodePictureBufferCount = GetBufferCount();
  m_pD3D12VAContext->cfg = &m_pVideoDecoderCfg;
  m_pVideoDecoderCfg.FrameRate.Denominator = c->framerate.den;
  m_pVideoDecoderCfg.FrameRate.Numerator = c->framerate.num;
  c->hwaccel_context = m_pD3D12VAContext;
  ref_free_index.clear();
  ref_free_index.resize(0);
  for (int i = 0; i < GetBufferCount(); i++)
    ref_free_index.push_back(i);

  return S_OK;
}
