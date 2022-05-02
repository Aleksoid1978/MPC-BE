/*
 *      Copyright (C) 2021      Benoit Plourde
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

#pragma once
#include <d3d12.h>
#include <d3d12video.h>
#include <dxgi1_3.h>
#include <vector>
#include <list>
#include "D3D12Commands.h"
#include "D3D12Format.h"

/*comptr*/
#define MAX_SURFACE_COUNT (64)
#include "./D3D12Decoder/D3D12SurfaceAllocator.h"

extern "C"
{
  #include <ExtLib/ffmpeg/libavcodec/d3d12va.h>
}
class CMPCVideoDecFilter;
struct AVCodecContext;
struct AVD3D12VAContext;
struct AVBufferRef;

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory);

class CD3D12Decoder
{
public:
  CD3D12Decoder(CMPCVideoDecFilter* pFilter);
  virtual ~CD3D12Decoder(void);

  HRESULT Init();

  void AdditionaDecoderInit(AVCodecContext* c);
  void PostInitDecoder(AVCodecContext* c);

  HRESULT PostConnect(AVCodecContext* c, IPin* pPin);
  HRESULT DeliverFrame();

  HRESULT InitAllocator(IMemAllocator** ppAlloc);

  GUID* GetDecoderGuid() { return m_DecoderGUID != GUID_NULL ? &m_DecoderGUID : nullptr; }
  DXGI_ADAPTER_DESC* GetAdapterDesc() { return &m_AdapterDesc; }
  void Flush();
  void DestroyDecoder(const bool bFull);
private:
  friend class CD3D12SurfaceAllocator;
  CMPCVideoDecFilter* m_pFilter = nullptr;

  struct {
    HMODULE d3d12lib;
    PFN_D3D12_CREATE_DEVICE mD3D12CreateDevice;

    HMODULE dxgilib;
    PFN_CREATE_DXGI_FACTORY2 mCreateDXGIFactory2;

  } m_dxLib = { 0 };

  DXGI_ADAPTER_DESC m_AdapterDesc = {};

  CD3D12SurfaceAllocator* m_pAllocator = nullptr;
  CD3D12Commands* m_pD3DCommands;
  ID3D12Debug* m_pD3DDebug = nullptr;
  ID3D12Debug1* m_pD3DDebug1 = nullptr;
  ID3D12Device2* m_pD3DDevice = nullptr;
  ID3D12VideoDevice* m_pVideoDevice = nullptr;
  D3D12_VIDEO_DECODER_HEAP_DESC m_pVideoDecoderCfg = {};

  ID3D12Resource* m_pTexture[MAX_SURFACE_COUNT];//texture array
  ID3D12Resource* ref_table[MAX_SURFACE_COUNT];//ref frame
  UINT                            ref_index[MAX_SURFACE_COUNT];//ref frame index
  std::list<int>                  ref_free_index;

  IDXGIAdapter* m_pDxgiAdapter = nullptr;
  IDXGIFactory2* m_pDxgiFactory = nullptr;
  ID3D12VideoDecoder* m_pVideoDecoder = nullptr;

  /* avcodec internals */
  struct AVD3D12VAContext m_pD3D12VAContext;

  int m_nOutputViews = 0;

  DWORD m_dwSurfaceWidth = 0;
  DWORD m_dwSurfaceHeight = 0;
  DWORD m_dwSurfaceCount = 0;
  DXGI_FORMAT m_SurfaceFormat = DXGI_FORMAT_UNKNOWN;
  GUID m_DecoderGUID = GUID_NULL;

  HRESULT CreateD3D12Device(UINT nDeviceIndex);
  HRESULT CreateD3D12Decoder(AVCodecContext* c);
  
  HRESULT FindVideoServiceConversion(AVCodecContext* c, enum AVCodecID codec, int profile, DXGI_FORMAT surface_format, GUID* input);
#if 0
  HRESULT FindDecoderConfiguration(AVCodecContext* c, const D3D11_VIDEO_DECODER_DESC* desc, D3D11_VIDEO_DECODER_CONFIG* pConfig);
#endif

  HRESULT ReInitD3D12Decoder(AVCodecContext* c);

  

  static enum AVPixelFormat get_d3d12_format(struct AVCodecContext* s, const enum AVPixelFormat* pix_fmts);
  static int get_d3d12_buffer(struct AVCodecContext* c, AVFrame* pic, int flags);

  STDMETHODIMP_(long) GetBufferCount(long* pMaxBuffers = nullptr);
  
};
#if 0



    // ILAVDecoder
    STDMETHODIMP Check();
    STDMETHODIMP InitDecoder(AVCodecID codec, const CMediaType *pmt);
    STDMETHODIMP GetPixelFormat(LAVPixelFormat *pPix, int *pBpp);
    STDMETHODIMP Flush();
    STDMETHODIMP EndOfStream();

    STDMETHODIMP InitAllocator(IMemAllocator **ppAlloc);
    STDMETHODIMP PostConnect(IPin *pPin);
    STDMETHODIMP BreakConnect();
    STDMETHODIMP_(long) GetBufferCount(long *pMaxBuffers = nullptr);
    STDMETHODIMP_(const WCHAR *) GetDecoderName()
    {
        return m_bReadBackFallback ? (m_bDirect ? L"D3D12 cb direct" : L"D3D12 cb") : L"D3D12 native";
    }
    STDMETHODIMP HasThreadSafeBuffers() { return S_FALSE; }
    STDMETHODIMP SetDirectOutput(BOOL bDirect)
    {
        m_bDirect = bDirect;
        return S_OK;
    }
    STDMETHODIMP_(DWORD) GetHWAccelNumDevices();
    STDMETHODIMP GetHWAccelDeviceInfo(DWORD dwIndex, BSTR *pstrDeviceName, DWORD *dwDeviceIdentifier);
    STDMETHODIMP GetHWAccelActiveDevice(BSTR *pstrDeviceName);

    // CDecBase
    STDMETHODIMP Init();

    void free_buffer(int idx);
  protected:
    HRESULT AdditionaDecoderInit();
    HRESULT PostDecode();

    HRESULT HandleDXVA2Frame(LAVFrame *pFrame);
    HRESULT DeliverD3D12Frame(LAVFrame *pFrame);
    HRESULT DeliverD3D12Readback(LAVFrame *pFrame);
    HRESULT DeliverD3D12ReadbackDirect(LAVFrame *pFrame);

    BOOL IsHardwareAccelerator() { return TRUE; }


    
  private:
    STDMETHODIMP DestroyDecoder(bool bFull);

    STDMETHODIMP ReInitD3D12Decoder(AVCodecContext *s);
    STDMETHODIMP CreateD3D12Device(UINT nDeviceIndex);
    STDMETHODIMP CreateD3D12Decoder();

    STDMETHODIMP FillHWContext(AVD3D12VAContext *ctx);

    STDMETHODIMP FlushDisplayQueue(BOOL bDeliver);

    STDMETHODIMP FindVideoServiceConversion(AVCodecID codec, int profile, DXGI_FORMAT surface_format, GUID* input);

    static enum AVPixelFormat get_d3d12_format(struct AVCodecContext* s, const enum AVPixelFormat* pix_fmts);
    static int get_d3d12_buffer(struct AVCodecContext *c, AVFrame *pic, int flags);
    static void ReleaseFrame12(void* opaque, uint8_t* data);
    static void log_callback_null(void *ptr, int level, const char *fmt, va_list vl);
    
    STDMETHODIMP UpdateStaging();
  private:
    CD3D12SurfaceAllocator* m_pAllocator = nullptr;
    
    CD3D12Commands* m_pD3DCommands;
    ID3D12Debug* m_pD3DDebug;
    ID3D12Debug1* m_pD3DDebug1;
    ID3D12Device2* m_pD3DDevice;
    ID3D12VideoDevice* m_pVideoDevice;
    D3D12_VIDEO_DECODER_HEAP_DESC m_pVideoDecoderCfg;

    //for the copy data
    CRITICAL_SECTION directLock; // the ReportSize callback cannot be called during/after the Cleanup_cb is called

    const d3d_format_t* render_fmt;
    ID3D12VideoDecoder* m_pVideoDecoder;
    
    /* avcodec internals */
    struct AVD3D12VAContext m_pD3D12VAContext;

    ID3D12Resource*                 m_pTexture[MAX_SURFACE_COUNT];//texture array
    ID3D12Resource*                 ref_table[MAX_SURFACE_COUNT];//ref frame
    UINT                            ref_index[MAX_SURFACE_COUNT];//ref frame index
    std::list<int>                  ref_free_index;
    /*Staging*/
    ID3D12Resource* m_pStagingTexture;
    D3D12_HEAP_PROPERTIES m_pStagingProp;
    D3D12_RESOURCE_DESC m_pStagingDesc;
    d3d12_footprint_t m_pStagingLayout;
    AVFrame* m_pFrame = nullptr;
    
    IDXGIAdapter *m_pDxgiAdapter;
    IDXGIFactory2 *m_pDxgiFactory;

    DWORD m_dwSurfaceWidth = 0;
    DWORD m_dwSurfaceHeight = 0;
    DWORD m_dwSurfaceCount = 0;
    DXGI_FORMAT m_SurfaceFormat = DXGI_FORMAT_UNKNOWN;


    BOOL m_bReadBackFallback = FALSE;
    BOOL m_bDirect = FALSE;
    BOOL m_bFailHWDecode = FALSE;

    
    
    //LAVFrame* m_FrameQueue[4];

    int m_FrameQueuePosition = 0;
    int m_DisplayDelay = 4;

    struct
    {
        HMODULE d3d12lib;
        PFN_D3D12_CREATE_DEVICE mD3D12CreateDevice;

        HMODULE dxgilib;
        PFN_CREATE_DXGI_FACTORY2 mCreateDXGIFactory2;

    } dx = { 0 };

    DXGI_ADAPTER_DESC m_AdapterDesc = {0};
    friend class CD3D12SurfaceAllocator;
};
#endif