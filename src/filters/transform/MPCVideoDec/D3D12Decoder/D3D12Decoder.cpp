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

extern "C"
{
#include <ExtLib/ffmpeg/libavcodec/avcodec.h>
#include <ExtLib/ffmpeg/libavutil/opt.h>
#include <ExtLib/ffmpeg/libavutil/pixdesc.h>

#include <ExtLib/ffmpeg/libavcodec/d3d12va.h>
}

#define ROUND_UP_128(num) (((num) + 127) & ~127)
#if 0
HRESULT VerifyD3D12Device(DWORD& dwIndex, DWORD dwDeviceId)
{
    HRESULT hr = S_OK;
    DXGI_ADAPTER_DESC desc;

    HMODULE dxgi = LoadLibrary(L"dxgi.dll");
    if (dxgi == nullptr)
    {
        hr = E_FAIL;
        goto done;
    }
    
    PFN_CREATE_DXGI_FACTORY2 mCreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgi, "CreateDXGIFactory2");
    if (mCreateDXGIFactory2 == nullptr)
    {
        hr = E_FAIL;
        goto done;
    }

    IDXGIAdapter* pDXGIAdapter = nullptr;
    IDXGIFactory2* pDXGIFactory = nullptr;

    hr = mCreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&pDXGIFactory));
    if (FAILED(hr))
        goto done;

    // check the adapter specified by dwIndex
    hr = pDXGIFactory->EnumAdapters(dwIndex, &pDXGIAdapter);
    if (FAILED(hr))
        goto done;

    // if it matches the device id, then all is well and we're done
    pDXGIAdapter->GetDesc(&desc);
    if (desc.DeviceId == dwDeviceId)
        goto done;

    SafeRelease(&pDXGIAdapter);

    // try to find a device that matches this device id
    UINT i = 0;
    while (SUCCEEDED(pDXGIFactory->EnumAdapters(i, &pDXGIAdapter)))
    {
        pDXGIAdapter->GetDesc(&desc);
        SafeRelease(&pDXGIAdapter);

        if (desc.DeviceId == dwDeviceId)
        {
            dwIndex = i;
            goto done;
        }
        i++;
    }

    // if none is found, fail
    hr = E_FAIL;

done:
    SafeRelease(&pDXGIAdapter);
    SafeRelease(&pDXGIFactory);
    FreeLibrary(dxgi);
    return hr;
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
#endif
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
    for (int x = 0; x < MAX_SURFACE_COUNT; x++)
        m_pTexture[x] = nullptr;

    m_pStagingTexture = nullptr;
    av_frame_free(&m_pFrame);
    m_pD3D12VAContext.decoder = nullptr;
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
    SafeRelease(&m_pD3DDebug);
    SafeRelease(&m_pDxgiAdapter);
    SafeRelease(&m_pD3DDebug1);
    SafeRelease(&m_pD3DDebug);
    SafeRelease(&m_pStagingTexture);
    for (int x = 0; x < MAX_SURFACE_COUNT; x++)
        SafeRelease(&m_pTexture[x]);

    SafeRelease(&m_pVideoDevice);
    SafeRelease(&m_pDxgiAdapter);
    SafeRelease(&m_pDxgiFactory);
    SafeRelease(&m_pD3DDebug);
    SafeRelease(&m_pD3DDevice);
    SafeRelease(&m_pVideoDecoder);
    
    av_frame_free(&m_pFrame);
    
    SafeRelease(&m_pD3D12VAContext.decoder);
    //todo add free libs
}

// ILAVDecoder
STDMETHODIMP CD3D12Decoder::Init()
{
    // D3D12 decoding requires Windows 8 or newer
  if (!SysVersion::IsWin8orLater())
        return E_NOINTERFACE;

    dx.d3d12lib = LoadLibrary(L"d3d12.dll");
    if (dx.d3d12lib == nullptr)
    {
        DbgLog((LOG_TRACE, 10, L"Cannot open d3d11.dll"));
        return E_FAIL;
    }

    dx.mD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(dx.d3d12lib, "D3D12CreateDevice");
    if (dx.mD3D12CreateDevice == nullptr)
    {
        DbgLog((LOG_TRACE, 10, L"D3D11CreateDevice not available"));
        return E_FAIL;
    }

    dx.dxgilib = LoadLibrary(L"dxgi.dll");
    if (dx.dxgilib == nullptr)
    {
        DbgLog((LOG_TRACE, 10, L"Cannot open dxgi.dll"));
        return E_FAIL;
    }

    dx.mCreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dx.dxgilib, "CreateDXGIFactory2");
    if (dx.mCreateDXGIFactory2 == nullptr)
    {
        DbgLog((LOG_TRACE, 10, L"CreateDXGIFactory1 not available"));
        return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP CD3D12Decoder::InitAllocator(IMemAllocator **ppAlloc)
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

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_pD3DDebug))))
    {
        m_pD3DDebug->EnableDebugLayer();
        m_pD3DDebug->QueryInterface(IID_PPV_ARGS(&m_pD3DDebug1));
        //m_pD3DDebug1->SetEnableGPUBasedValidation(true);
        m_pD3DDebug1->SetEnableSynchronizedCommandQueueValidation(1);
    }

    // create DXGI factory
    IDXGIAdapter* pDXGIAdapter = nullptr;
    IDXGIFactory1* pDXGIFactory = nullptr;
    hr = dx.mCreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_pDxgiFactory));
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 10, L"-> DXGIFactory creation failed"));
        //goto fail;
    }
    
    //hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_pDxgiFactory));
    if FAILED (hr)
        assert(0);

    if (DXGI_ERROR_NOT_FOUND == m_pDxgiFactory->EnumAdapters(nDeviceIndex, &m_pDxgiAdapter))
    {
        hr = S_FALSE;
        assert(0);
    }
    DXGI_ADAPTER_DESC desc;
    hr = m_pDxgiAdapter->GetDesc(&desc);
    if FAILED (hr)
        assert(0);
    const D3D_FEATURE_LEVEL *levels = NULL;
    D3D_FEATURE_LEVEL max_level = D3D_FEATURE_LEVEL_12_1;
    //int level_count = s_GetD3D12FeatureLevels(max_level, &levels);

    hr = dx.mD3D12CreateDevice(m_pDxgiAdapter, max_level, IID_PPV_ARGS(&m_pD3DDevice));
    if FAILED (hr)
        assert(0);
    
    
    
    hr = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&m_pVideoDevice));
    return hr;
}



static const D3D_FEATURE_LEVEL s_D3D12Levels[] = {
    D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
};

static int s_GetD3D12FeatureLevels(int max_fl, const D3D_FEATURE_LEVEL **out)
{
    static const int levels_len = countof(s_D3D12Levels);

    int start = 0;
    for (; start < levels_len; start++)
    {
        if (s_D3D12Levels[start] <= max_fl)
            break;
    }
    *out = &s_D3D12Levels[start];
    return levels_len - start;
}

STDMETHODIMP CD3D12Decoder::PostConnect(IPin *pPin)
{

    DbgLog((LOG_TRACE, 10, L"CD3D12Decoder::PostConnect()"));
    HRESULT hr = S_OK;
    ID3D12DecoderConfiguration* pD3D12DecoderConfiguration = nullptr;
    hr = pPin->QueryInterface(&pD3D12DecoderConfiguration);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 10, L"-> ID3D11DecoderConfiguration not available, using fallback mode"));
    }
    m_pCallback->ReleaseAllDXVAResources();
    UINT nDevice;
    DWORD wDevice = -1;
    nDevice = m_pSettings->GetHWAccelDeviceIndex(HWAccel_D3D12, &wDevice);

    //If we get a device back just use it
    if (wDevice != -1 && pD3D12DecoderConfiguration)
    {
        nDevice = pD3D12DecoderConfiguration->GetD3D12AdapterIndex();
    }
    else
    {
        // if a device is specified manually, fallback to copy-back and use the selected device
        SafeRelease(&pD3D12DecoderConfiguration);

        // use the configured device
        if (nDevice == LAVHWACCEL_DEVICE_DEFAULT)
            nDevice = 0;
    }
    //UINT nDevice = LAVHWACCEL_DEVICE_DEFAULT;
    
    if (!m_pD3DDevice)
    {
        hr = CreateD3D12Device(nDevice);
    }

    // enable multithreaded protection
    //not in d3d12 ?
    ID3D10Multithread* pMultithread = nullptr;
    hr = m_pD3DDevice->QueryInterface(&pMultithread);
    if (SUCCEEDED(hr))
    {
        pMultithread->SetMultithreadProtected(TRUE);
        SafeRelease(&pMultithread);
    }

    // check if the connection supports native mode
    if (pD3D12DecoderConfiguration)
    {
        CMediaType mt = m_pCallback->GetOutputMediaType();
        if ((m_SurfaceFormat == DXGI_FORMAT_NV12 && mt.subtype != MEDIASUBTYPE_NV12) ||
            (m_SurfaceFormat == DXGI_FORMAT_P010 && mt.subtype != MEDIASUBTYPE_P010) ||
            (m_SurfaceFormat == DXGI_FORMAT_P016 && mt.subtype != MEDIASUBTYPE_P016))
        {
            DbgLog((LOG_ERROR, 10, L"-> Connection is not the appropriate pixel format for D3D11 Native"));

            SafeRelease(&pD3D12DecoderConfiguration);
        }
    }

    // Notice the connected pin that we're sending D3D11 textures
    if (pD3D12DecoderConfiguration)
    {
        hr = pD3D12DecoderConfiguration->ActivateD3D12Decoding(m_pD3DDevice, (HANDLE)0/*directLock*/, 0);
        SafeRelease(&pD3D12DecoderConfiguration);

        m_bReadBackFallback = FAILED(hr);
    }
    else
    {
        m_bReadBackFallback = true;
    }
    return S_OK;
fail:
    return E_FAIL;
}

STDMETHODIMP CD3D12Decoder::BreakConnect()
{
    if (m_bReadBackFallback)
        return S_FALSE;

    // release any resources held by the core
    m_pCallback->ReleaseAllDXVAResources();

    // flush all buffers out of the decoder to ensure the allocator can be properly de-allocated
    if (m_pAVCtx && avcodec_is_open(m_pAVCtx))
        avcodec_flush_buffers(m_pAVCtx);

    return S_OK;
}

void CD3D12Decoder::log_callback_null(void *ptr, int level, const char *fmt, va_list vl)
{
    char out[4096];
    vsnprintf(out, sizeof(out), fmt, vl);
    DbgLog((LOG_TRACE, 10, out));
    
}

STDMETHODIMP CD3D12Decoder::InitDecoder(AVCodecID codec, const CMediaType *pmt)
{
    HRESULT hr = S_OK;
    DbgLog((LOG_TRACE, 10, L"CD3D12Decoder::InitDecoder(): Initializing D3D12 decoder"));

    // Destroy old decoder
    DestroyDecoder(false);

    // reset stream compatibility
    m_bFailHWDecode = false;
    hr = CDecAvcodec::InitDecoder(codec, pmt);
    m_pAVCtx->debug = 0;
#ifdef DEBUG
    av_log_set_level(AV_LOG_ERROR);
    av_log_set_callback(log_callback_null);
#endif
    if (FAILED(hr))
        return hr;
    
    if (m_pAVCtx->pix_fmt == AV_PIX_FMT_NONE)// check_dxva_codec_profile(m_pAVCtx, AV_PIX_FMT_D3D12_VLD) ==0)
    {
        DbgLog((LOG_TRACE, 10, L"-> Incompatible profile detected, falling back to software decoding"));
        return E_FAIL;
    }
    // initialize surface format to ensure the default media type is set properly
    m_SurfaceFormat = d3d12va_map_sw_to_hw_format(m_pAVCtx->sw_pix_fmt);
    m_dwSurfaceWidth = dxva_align_dimensions(m_pAVCtx->codec_id, m_pAVCtx->coded_width);
    m_dwSurfaceHeight = dxva_align_dimensions(m_pAVCtx->codec_id, m_pAVCtx->coded_height);

    return S_OK;
}

void CD3D12Decoder::AdditionaDecoderInit(AVCodecContext* c)
{
    AVD3D12VAContext *ctx = av_d3d12va_alloc_context();
    if (!m_pD3DDevice)
    {
        CreateD3D12Device(0);
    }
    if (m_pVideoDecoder)
    {
        FillHWContext(ctx);
        m_pAVCtx->hwaccel_context = ctx;
    }

    //TODO FIX FORMAT
    m_pAVCtx->thread_count = 1;
    m_pAVCtx->get_format = get_d3d12_format;
    m_pAVCtx->get_buffer2 = get_d3d12_buffer;
    m_pAVCtx->opaque = this;
    m_pAVCtx->slice_flags |= SLICE_FLAG_ALLOW_FIELD;
    // disable error concealment in hwaccel mode, it doesn't work either way
    m_pAVCtx->error_concealment = 0;
    av_opt_set_int(m_pAVCtx, "enable_er", 0, AV_OPT_SEARCH_CHILDREN);
    // ctx->test_buffer = TestBuffer;
    return S_OK;
}

enum AVPixelFormat CD3D12Decoder::get_d3d12_format(struct AVCodecContext *s, const enum AVPixelFormat *pix_fmts)
{
    CD3D12Decoder* pDec = (CD3D12Decoder*)s->opaque;
    const enum AVPixelFormat* p;
    for (p = pix_fmts; *p != -1; p++)
    {
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(*p);

        if (!desc || !(desc->flags & AV_PIX_FMT_FLAG_HWACCEL))
            break;

        if (*p == AV_PIX_FMT_D3D12_VLD)
        {
            HRESULT hr = pDec->ReInitD3D12Decoder(s);
            if (FAILED(hr))
            {
                pDec->m_bFailHWDecode = TRUE;
                continue;
            }
            else
            {
                break;
            }
        }
    }
    return *p;
}
void CD3D12Decoder::ReleaseFrame12(void* opaque, uint8_t* data)
{
    (void)data;

    //LAVFrame* pFrame = (LAVFrame*)opaque;
   
#if 0
    if (pFrame->destruct)
    {
        pFrame->destruct(pFrame);
        pFrame->destruct = nullptr;
        pFrame->priv_data = nullptr;
    }

    memset(pFrame->data, 0, sizeof(pFrame->data));
    memset(pFrame->stereo, 0, sizeof(pFrame->stereo));
    memset(pFrame->stride, 0, sizeof(pFrame->stride));

    for (int i = 0; i < pFrame->side_data_count; i++)
    {
        SAFE_CO_FREE(pFrame->side_data[i].data);
    }
    SAFE_CO_FREE(pFrame->side_data);

    pFrame->side_data_count = 0;
#endif
    //SAFE_CO_FREE(pFrame);

    
}

int CD3D12Decoder::get_d3d12_buffer(struct AVCodecContext *c, AVFrame *frame, int flags)
{
    CD3D12Decoder *pDec = (CD3D12Decoder *)c->opaque;

    frame->opaque = NULL;
    HRESULT hr = S_OK;
    
    UINT freeindex=-1;

    hr = pDec->ReInitD3D12Decoder(c);

    if (FAILED(hr))
    {
        pDec->m_bFailHWDecode = TRUE;
        return -1;
    }
    
    if (pDec->m_bReadBackFallback)
    {
        if (pDec->ref_free_index.size() == 0)
            return E_OUTOFMEMORY;
        frame->buf[0] = av_buffer_create(frame->data[0], 0, ReleaseFrame12, nullptr, 0);
        if (!frame->buf[0])
            assert(0);
        frame->data[0] = (uint8_t*)pDec->m_pD3D12VAContext.surfaces.NumTexture2Ds;//array size
        frame->data[3] = (uint8_t*)pDec->ref_free_index.front();//output index of the frame
        pDec->ref_free_index.pop_front();
        frame->width = c->coded_width;
        frame->height = c->coded_height;
        return 0;
    }
    else if (pDec->m_bReadBackFallback == false && pDec->m_pAllocator)
    {
        
        IMediaSample* pSample = nullptr;
        hr = pDec->m_pAllocator->GetBuffer(& pSample, nullptr, nullptr, 0);
        if (SUCCEEDED(hr))
        {
            CD3D12MediaSample* pD3D12Sample = dynamic_cast<CD3D12MediaSample*>(pSample);

            // fill the frame from the sample, including a reference to the sample
            pD3D12Sample->GetAVFrameBuffer(frame);
            
            frame->width = c->coded_width;
            frame->height = c->coded_height;
            frame->data[0] = (uint8_t*) "go";
            
            int frameindex = (int)frame->data[3];
                
            // the frame holds the sample now, can release the direct interface
            pD3D12Sample->Release();
            return 0;
        }

    }
    
    return -1;
}





void CD3D12Decoder::FillHWContext(AVD3D12VAContext *ctx)
{
    
    ctx->decoder = m_pVideoDecoder;
    

}

STDMETHODIMP_(long) CD3D12Decoder::GetBufferCount(long *pMaxBuffers)
{
    long buffers = 0;
    buffers = 16;
    // Native decoding should use 16 buffers to enable seamless codec changes

        // Buffers based on max ref frames
        if (m_nCodecId == AV_CODEC_ID_H264 || m_nCodecId == AV_CODEC_ID_HEVC)
            buffers = 16;
        else if (m_nCodecId == AV_CODEC_ID_VP9 || m_nCodecId == AV_CODEC_ID_AV1)
            buffers = 8;
        else
            buffers = 2;


    // 4 extra buffers for handling and safety
    //if (m_nCodecId != AV_CODEC_ID_H264 && m_nCodecId != AV_CODEC_ID_HEVC)
    buffers += 4;

    if (m_bReadBackFallback)
    {
        //not implemented yet
        buffers += m_DisplayDelay;
    }

    if (m_pCallback->GetDecodeFlags() & LAV_VIDEO_DEC_FLAG_DVD)
    {
        buffers += 4;
    }

    if (pMaxBuffers)
    {
        // cap at 127, because it needs to fit into the 7-bit DXVA structs
        *pMaxBuffers = 127;

        // VC-1 and VP9 decoding has stricter requirements (decoding flickers otherwise)
        if (m_nCodecId == AV_CODEC_ID_VC1 || m_nCodecId == AV_CODEC_ID_VP9 || m_nCodecId == AV_CODEC_ID_AV1)
            *pMaxBuffers = 32;
    }
    
    return buffers;

}

STDMETHODIMP CD3D12Decoder::FlushDisplayQueue(BOOL bDeliver)
{
    /*for (int i = 0; i < m_DisplayDelay; ++i)
    {
        if (m_FrameQueue[m_FrameQueuePosition])
        {
            if (bDeliver)
            {
                DeliverD3D12Frame(m_FrameQueue[m_FrameQueuePosition]);
                m_FrameQueue[m_FrameQueuePosition] = nullptr;
            }
            else
            {
                //ReleaseFrame(&m_FrameQueue[m_FrameQueuePosition]);
            }
        }
        m_FrameQueuePosition = (m_FrameQueuePosition + 1) % m_DisplayDelay;
    }*/

    return S_OK;
    DbgLog((LOG_TRACE, 10, L"FlushDisplayQueue"));
    return S_OK;
}

STDMETHODIMP CD3D12Decoder::Flush()
{
    DbgLog((LOG_TRACE, 10, L"CD3D12Decoder::Flush"));
    ref_free_index.clear();
    ref_free_index.resize(0);
    for (int i = 0; i < GetBufferCount(); i++)
        ref_free_index.push_back(i);

    CDecAvcodec::Flush();

    // Flush display queue
    //TODO add display queue
    FlushDisplayQueue(FALSE);

    return S_OK;
}

STDMETHODIMP CD3D12Decoder::EndOfStream()
{
    CDecAvcodec::EndOfStream();

    // Flush display queue
    FlushDisplayQueue(TRUE);

    return S_OK;
}

HRESULT CD3D12Decoder::PostDecode()
{
    if (m_bFailHWDecode)
    {
        DbgLog((LOG_TRACE, 10, L"::PostDecode(): HW Decoder failed, falling back to software decoding"));
        return E_FAIL;
    }
    return S_OK;
}

STDMETHODIMP CD3D12Decoder::FindVideoServiceConversion(AVCodecID codec, int profile, DXGI_FORMAT surface_format, GUID* input)
{
    
    

    HRESULT hr = S_OK;
    *input = GUID_NULL;
    if (codec == AV_CODEC_ID_MPEG2VIDEO)
        *input = D3D12_VIDEO_DECODE_PROFILE_MPEG1_AND_MPEG2;
    if (codec == AV_CODEC_ID_HEVC)
    {
        //need some testing
        if (!HEVC_CHECK_PROFILE(profile))
            *input = D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN;
        else
            *input = D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN10;
    }
    const int h264mb_count = (m_pAVCtx->coded_width / 16) * (m_pAVCtx->coded_height / 16);

    if (codec == AV_CODEC_ID_H264)
    {
        if (profile != FF_PROFILE_UNKNOWN && !H264_CHECK_PROFILE(profile))
            *input = D3D12_VIDEO_DECODE_PROFILE_H264;
        *input = D3D12_VIDEO_DECODE_PROFILE_H264;
    }


    if ((codec == AV_CODEC_ID_WMV3 || codec == AV_CODEC_ID_VC1) && profile == FF_PROFILE_VC1_COMPLEX)
    {
        *input = D3D12_VIDEO_DECODE_PROFILE_VC1;
        //need to find which one we need
        //D3D12_VIDEO_DECODE_PROFILE_VC1_D2010
        
    }
    if (codec == AV_CODEC_ID_VP9)
    {
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
    if (m_bInInit)
        return S_FALSE;

    // we need an allocator at this point
    if (m_bReadBackFallback == false && m_pAllocator == nullptr)
        return E_FAIL;

    DXGI_FORMAT surface_format;
    switch (m_pAVCtx->sw_pix_fmt)
    {
    case AV_PIX_FMT_YUV420P10LE:
    case AV_PIX_FMT_P010:
        surface_format = DXGI_FORMAT_P010;
        break;
    case AV_PIX_FMT_NV12:
    default: surface_format = DXGI_FORMAT_NV12;
    }

    if (m_pVideoDecoder == nullptr || m_dwSurfaceWidth != dxva_align_dimensions(s->codec_id, s->coded_width) ||
        m_dwSurfaceHeight != dxva_align_dimensions(s->codec_id, s->coded_height) ||
        m_SurfaceFormat != surface_format)
    {

        D3D12_VIDEO_DECODER_DESC decoderDesc;
        D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT formats;
        CD3D12Format* fmt = new CD3D12Format(m_pVideoDevice);
        GUID profileGUID = GUID_NULL;
        hr = fmt->FindVideoServiceConversion(s, surface_format, &profileGUID);
        if (SUCCEEDED(hr))
            formats = fmt->GetDecodeSupport();
        if (SUCCEEDED(hr))
            CreateD3D12Decoder();
        else
        {
            DbgLog((LOG_ERROR, 10, L"-> No video service profile found"));
            return hr;
        }
        int idx, surface_idx = 0;
        const d3d_format_t* decoder_format;
        int bitdepth = 8;
        int chromah, chromaw;
        //add checkup for those flags in cd3d12format
        UINT supportFlags = D3D12_FORMAT_SUPPORT1_DECODER_OUTPUT | D3D12_FORMAT_SUPPORT1_SHADER_LOAD;


        //m_SurfaceFormat = fmt->GetSupportedFormat();
        m_SurfaceFormat = d3d12va_map_sw_to_hw_format(m_pAVCtx->sw_pix_fmt);


        CD3DX12_HEAP_PROPERTIES m_textureProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_CUSTOM, 1, 1);
        CD3DX12_RESOURCE_DESC m_textureDesc = CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
            s->width, s->height, 1, 1, m_SurfaceFormat, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE);
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


        m_pD3D12VAContext.surfaces.NumTexture2Ds = GetBufferCount();
        m_pD3D12VAContext.surfaces.ppTexture2Ds = ref_table;
        m_pD3D12VAContext.surfaces.pSubresources = ref_index;
        m_pD3D12VAContext.surfaces.ppHeaps = NULL;
        for (surface_idx = 0; surface_idx < GetBufferCount(); surface_idx++) {
            ref_table[surface_idx] = m_pTexture[surface_idx];
            ref_index[surface_idx] = 0;// surface_idx;
        }
        decoderDesc.Configuration.DecodeProfile = formats.Configuration.DecodeProfile;
        decoderDesc.NodeMask = 0;
        decoderDesc.Configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
        decoderDesc.Configuration.BitstreamEncryption = D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE;

        ID3D12VideoDecoder* decoder;

        hr = m_pVideoDevice->CreateVideoDecoder(&decoderDesc, IID_ID3D12VideoDecoder, (void**)&decoder);
        if (FAILED(hr))
            assert(0);
        m_pVideoDecoder = decoder;
        m_pD3D12VAContext.workaround = 0;// FF_DXVA2_WORKAROUND_HEVC_REXT;
        m_pD3D12VAContext.report_id = 0;
        m_pD3D12VAContext.decoder = decoder;
        m_pVideoDecoderCfg.NodeMask = 0;
        m_pVideoDecoderCfg.Configuration = decoderDesc.Configuration;
        m_pVideoDecoderCfg.DecodeWidth = s->width;
        m_pVideoDecoderCfg.DecodeHeight = s->height;
        m_pVideoDecoderCfg.Format = m_SurfaceFormat;
        m_pVideoDecoderCfg.MaxDecodePictureBufferCount = GetBufferCount();
        m_pD3D12VAContext.cfg = &m_pVideoDecoderCfg;
        m_pVideoDecoderCfg.FrameRate.Denominator = m_pAVCtx->framerate.den;
        m_pVideoDecoderCfg.FrameRate.Numerator = m_pAVCtx->framerate.num;
        m_pAVCtx->hwaccel_context = &m_pD3D12VAContext;
        ref_free_index.clear();
        ref_free_index.resize(0);
        for (int i = 0; i < GetBufferCount(); i++)
            ref_free_index.push_back(i);
        fmt = nullptr;
        
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
    else
    {
        m_dwSurfaceCount = GetBufferCount();
    }
    
    return E_FAIL;
}


HRESULT CD3D12Decoder::HandleDXVA2Frame(LAVFrame *pFrame)
{
    
    if (pFrame->flags & LAV_FRAME_FLAG_FLUSH)
    {
        if (m_bReadBackFallback)
        {
            FlushDisplayQueue(TRUE);
        }
        Deliver(pFrame);
        return S_OK;
    }

    ASSERT(pFrame->format == LAVPixFmt_D3D12);

    if (m_bReadBackFallback == false)
    {
        DeliverD3D12Frame(pFrame);
    }
    else
    {
        DeliverD3D12Frame(pFrame);
        
    }

    return S_OK;
}

HRESULT CD3D12Decoder::DeliverD3D12Frame(LAVFrame *pFrame)
{
    if (m_bReadBackFallback)
    {
        if (m_bDirect)
            DeliverD3D12ReadbackDirect(pFrame);
        else
            DeliverD3D12Readback(pFrame);
    }
    else
    {
        AVFrame* pAVFrame = (AVFrame*)pFrame->priv_data;
        int textureindex = (int)pAVFrame->data[3];
        CD3D12MediaSample* pD3D12Sample = (CD3D12MediaSample*)(pAVFrame->data[1]);
        pD3D12Sample->SetD3D12Texture(m_pTexture[textureindex]);
        pFrame->data[0] = pAVFrame->data[1];
        
        //pFrame->data[1] = pFrame->data[2] = nullptr;
        pFrame->data[1] = nullptr;
        //need to keep data[3] its the index of the surface used

        GetPixelFormat(&pFrame->format, &pFrame->bpp);

        Deliver(pFrame);
        //FILTER_STATE state;
        //m_pCallback->GetOutputPin().m_pFilter->GetState(0, &state);
        
        if (m_pCallback->GetOutputPin()->IsStopped())
            assert(0);
    }
    
    return S_OK;
}

HRESULT CD3D12Decoder::DeliverD3D12Readback(LAVFrame *pFrame)
{
    
    return Deliver(pFrame);
}

struct D3D12DirectPrivate
{
    //AVBufferRef *pDeviceContex;
    d3d12_footprint_t pStagingLayout;
    ID3D12Resource *pStagingTexture;
    int frame_index;
    CD3D12Decoder* pDec;
    CRITICAL_SECTION crit;
};

static bool d3d12_direct_lock(LAVFrame *pFrame, LAVDirectBuffer *pBuffer)
{
    D3D12DirectPrivate* c = (D3D12DirectPrivate*)pFrame->priv_data;
    
    D3D12_RESOURCE_DESC desc;
    

    ASSERT(pFrame && pBuffer);

    // lock the device context
    EnterCriticalSection(&c->crit);

    desc = c->pStagingTexture->GetDesc();

    // map
    HRESULT hr = S_OK;// pDeviceContext->device_context->Map(c->pStagingTexture, 0, D3D11_MAP_READ, 0, &map);

    void* pData;
    D3D12_RANGE readRange;
    readRange.Begin = 0;
    readRange.End = c->pStagingLayout.RequiredSize;
    
    hr = c->pStagingTexture->Map(0, &readRange,&pData);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&c->crit);
        return false;
    }
    //D3D12_BOX pSrcBox;
    //675840 the data stop at that
    //hr = c->pStagingTexture->ReadFromSubresource(&pData, c->pStagingLayout.layoutplane[0].Footprint.RowPitch, 1,(UINT)0,nullptr);
    pBuffer->data[0] = (BYTE*)pData;
    pBuffer->data[1] = pBuffer->data[0] + c->pStagingLayout.layoutplane[0].Footprint.Height * c->pStagingLayout.layoutplane[0].Footprint.RowPitch;// map.RowPitch;

    pBuffer->stride[0] = c->pStagingLayout.layoutplane[0].Footprint.RowPitch;
    pBuffer->stride[1] = c->pStagingLayout.layoutplane[0].Footprint.RowPitch;
    
    //c->pStagingTexture->Unmap(0, nullptr);
    return true;
}

static void d3d12_direct_unlock(LAVFrame *pFrame)
{
    D3D12DirectPrivate* c = (D3D12DirectPrivate*)pFrame->priv_data;
    c->pStagingTexture->Unmap(0, nullptr);
    LeaveCriticalSection(&c->crit);
}

static void d3d12_direct_free(LAVFrame *pFrame)
{
    D3D12DirectPrivate *c = (D3D12DirectPrivate *)pFrame->priv_data;
    
    c->pStagingTexture->Release();
    c->pStagingLayout = c->pStagingLayout;
    
    c->pDec->free_buffer(c->frame_index);
    delete c;
}

void CD3D12Decoder::free_buffer(int idx)
{
    std::list<int>::iterator it;
    //stupid hack in case we lost some index
    if (ref_free_index.size() == 1)
        printf("yo");
    it = std::find(ref_free_index.begin(), ref_free_index.end(), idx);
    if (it != ref_free_index.end())
        ref_free_index.erase(it);
    else
        ref_free_index.push_back(idx);
    
}

HRESULT CD3D12Decoder::UpdateStaging()
{
    
    HRESULT hr = S_OK;
    if (m_pStagingTexture)
    {
        SafeRelease(&m_pStagingTexture);
    }
    
    if (!m_pD3DCommands)
        m_pD3DCommands = new CD3D12Commands(m_pD3DDevice);
    m_pStagingDesc = {};
        
    m_pStagingProp = {};
    UINT64 sss;
    D3D12_RESOURCE_DESC indesc = ref_table[0]->GetDesc();
    //nv12
    //DXGI_FORMAT_R8_UNORM first plane
    //DXGI_FORMAT_R8G8_UNORM second plane
    m_pD3DDevice->GetCopyableFootprints(&indesc,
        0, 2, 0, m_pStagingLayout.layoutplane, m_pStagingLayout.rows_plane, m_pStagingLayout.pitch_plane, &m_pStagingLayout.RequiredSize);


    m_pStagingProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    m_pStagingDesc = CD3DX12_RESOURCE_DESC::Buffer(m_pStagingLayout.RequiredSize);


    hr = m_pD3DDevice->CreateCommittedResource(&m_pStagingProp, D3D12_HEAP_FLAG_NONE,
        &m_pStagingDesc, D3D12_RESOURCE_STATE_COPY_DEST, NULL,//D3D12_RESOURCE_STATE_GENERIC_READ
        IID_ID3D12Resource, (void**)&m_pStagingTexture);

    if (FAILED(hr))
        assert(0);
    return hr;

}



HRESULT CD3D12Decoder::DeliverD3D12ReadbackDirect(LAVFrame *pFrame)
{
    AVFrame* srcframe = (AVFrame*)pFrame->priv_data;
    UINT outputindex;

    outputindex = (UINT)srcframe->data[3];
    ID3D12Resource* out = m_pTexture[outputindex];
    D3D12_RESOURCE_DESC desc = m_pTexture[outputindex]->GetDesc();
    UINT outsub = m_pD3D12VAContext.surfaces.pSubresources[outputindex];
    
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[2] = {};
    UINT numrows[2] = {};
    UINT64 rowsizes[2] = {};
    UINT64 totalbytes = 0;
    D3D12_FEATURE_DATA_FORMAT_INFO info;
    if (m_pStagingDesc.Width != desc.Width || m_pStagingDesc.Height != desc.Height ||
        m_pStagingDesc.Format != desc.Format || !m_pStagingTexture)
        UpdateStaging();
    
    HRESULT hr;
    ID3D12GraphicsCommandList *cmd = m_pD3DCommands->GetCommandBuffer();
    m_pD3DCommands->Reset(cmd);
    D3D12_TEXTURE_COPY_LOCATION dst;
    D3D12_TEXTURE_COPY_LOCATION src;   
    for (int i = 0; i < 2; i++) {
        dst = CD3DX12_TEXTURE_COPY_LOCATION(m_pStagingTexture, m_pStagingLayout.layoutplane[i]);
        src = CD3DX12_TEXTURE_COPY_LOCATION(out, i);
        cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }
    uint8_t* pdata;
    D3D12_RANGE range;
    range.Begin = 0;
    range.End = m_pStagingLayout.RequiredSize;

    hr = cmd->Close();
    m_pD3DCommands->Submit({ cmd });
    m_pD3DCommands->GPUSync();
    //we are done with this reference frame we can free it

    av_frame_free(&srcframe);
    D3D12DirectPrivate* c = new D3D12DirectPrivate;
    
    c->pStagingTexture = m_pStagingTexture;
    c->pStagingLayout = m_pStagingLayout;
    c->frame_index = (int)outputindex;
    c->pDec = this;
    c->crit = directLock;
    m_pStagingTexture->AddRef();
    pFrame->priv_data = c;
    pFrame->destruct = d3d12_direct_free;

    GetPixelFormat(&pFrame->format, &pFrame->bpp);

    pFrame->direct = true;
    pFrame->direct_lock = d3d12_direct_lock;
    pFrame->direct_unlock = d3d12_direct_unlock;

    return Deliver(pFrame);
}

STDMETHODIMP CD3D12Decoder::GetPixelFormat(LAVPixelFormat *pPix, int *pBpp)
{
    //*pPix = LAVPixFmt_D3D12;
    if (pPix)
        *pPix = m_bReadBackFallback == false
        ? LAVPixFmt_D3D12
        : ((m_SurfaceFormat == DXGI_FORMAT_P010 || m_SurfaceFormat == DXGI_FORMAT_P016) ? LAVPixFmt_P016
            : LAVPixFmt_NV12);

    if (pBpp)
        *pBpp = (m_SurfaceFormat == DXGI_FORMAT_P016) ? 16 : (m_SurfaceFormat == DXGI_FORMAT_P010 ? 10 : 8);
    return S_OK;
}

STDMETHODIMP_(DWORD) CD3D12Decoder::GetHWAccelNumDevices()
{
    DWORD nDevices = 0;
    UINT i = 0;
    IDXGIAdapter *pDXGIAdapter = nullptr;
    IDXGIFactory1 *pDXGIFactory = nullptr;

    HRESULT hr = CreateDXGIFactory1(IID_IDXGIFactory1, (void **)&pDXGIFactory);
    if (FAILED(hr))
        goto fail;

    DXGI_ADAPTER_DESC desc;
    while (SUCCEEDED(pDXGIFactory->EnumAdapters(i, &pDXGIAdapter)))
    {
        pDXGIAdapter->GetDesc(&desc);
        SafeRelease(&pDXGIAdapter);

        // stop when we hit the MS software device
        if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
            break;

        i++;
    }

    nDevices = i;

fail:
    SafeRelease(&pDXGIFactory);
    return nDevices;
}

STDMETHODIMP CD3D12Decoder::GetHWAccelDeviceInfo(DWORD dwIndex, BSTR *pstrDeviceName, DWORD *dwDeviceIdentifier)
{
    IDXGIAdapter *pDXGIAdapter = nullptr;
    IDXGIFactory1 *pDXGIFactory = nullptr;
    
    HRESULT hr = CreateDXGIFactory1(IID_IDXGIFactory1, (void **)&pDXGIFactory);
    if (FAILED(hr))
        goto fail;

    hr = pDXGIFactory->EnumAdapters(dwIndex, &pDXGIAdapter);
    if (FAILED(hr))
        goto fail;

    DXGI_ADAPTER_DESC desc;
    pDXGIAdapter->GetDesc(&desc);

    // stop when we hit the MS software device
    if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
    {
        hr = E_INVALIDARG;
        goto fail;
    }

    if (pstrDeviceName)
        *pstrDeviceName = SysAllocString(desc.Description);

    if (dwDeviceIdentifier)
        *dwDeviceIdentifier = desc.DeviceId;

fail:
    SafeRelease(&pDXGIFactory);
    SafeRelease(&pDXGIAdapter);
    return hr;
}

STDMETHODIMP CD3D12Decoder::GetHWAccelActiveDevice(BSTR *pstrDeviceName)
{
    CheckPointer(pstrDeviceName, E_POINTER);

    if (m_AdapterDesc.Description[0] == 0)
        return E_UNEXPECTED;

    *pstrDeviceName = SysAllocString(m_AdapterDesc.Description);
    return S_OK;
}
