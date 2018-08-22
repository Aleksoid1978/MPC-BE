/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include "NullRenderers.h"
#include <moreuuids.h>
#include "MediaTypeEx.h"
#include "D3D9Helper.h"
#include "Log.h"
#include "FileHandle.h"

#define USE_DXVA

#ifdef USE_DXVA

#include <d3d9.h>
#include <dxva2api.h>		// DXVA2
#include <evr.h>
#include <mfapi.h>	// API Media Foundation
#include <Mferror.h>

// dxva.dll
typedef HRESULT (__stdcall *PTR_DXVA2CreateDirect3DDeviceManager9)(UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager);
typedef HRESULT (__stdcall *PTR_DXVA2CreateVideoService)(IDirect3DDevice9* pDD, REFIID riid, void** ppService);

class CNullVideoRendererInputPin : public CRendererInputPin,
	public IMFGetService,
	public IDirectXVideoMemoryConfiguration,
	public IMFVideoDisplayControl
{
public :
	CNullVideoRendererInputPin(CBaseRenderer *pRenderer, HRESULT *phr, LPCWSTR Name);
	~CNullVideoRendererInputPin() {
		if (m_pD3DDeviceManager) {
			if (m_hDevice != INVALID_HANDLE_VALUE) {
				m_pD3DDeviceManager->CloseDeviceHandle(m_hDevice);
				m_hDevice = INVALID_HANDLE_VALUE;
			}
			m_pD3DDeviceManager = nullptr;
		}
		if (m_pD3DDev) {
			m_pD3DDev = nullptr;
		}
		if (m_hDXVA2Lib) {
			FreeLibrary(m_hDXVA2Lib);
		}
	}

	DECLARE_IUNKNOWN
	STDMETHODIMP	NonDelegatingQueryInterface(REFIID riid, void** ppv);

	STDMETHODIMP	GetAllocator(IMemAllocator **ppAllocator) {
		// Renderer shouldn't manage allocator for DXVA
		return E_NOTIMPL;
	}

	STDMETHODIMP	GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProps) {
		// 1 buffer required
		memset (pProps, 0, sizeof(ALLOCATOR_PROPERTIES));
		pProps->cbBuffer = 1;
		return S_OK;
	}

	// IMFGetService
	STDMETHODIMP	GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

	// IDirectXVideoMemoryConfiguration
	STDMETHODIMP	GetAvailableSurfaceTypeByIndex(DWORD dwTypeIndex, DXVA2_SurfaceType *pdwType);
	STDMETHODIMP	SetSurfaceType(DXVA2_SurfaceType dwType);

	// IMFVideoDisplayControl
	STDMETHODIMP	GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	SetAspectRatioMode(DWORD dwAspectRatioMode) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	GetAspectRatioMode(DWORD *pdwAspectRatioMode) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	SetVideoWindow(HWND hwndVideo) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	GetVideoWindow(HWND *phwndVideo);
	STDMETHODIMP	RepaintVideo( void) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	SetBorderColor(COLORREF Clr) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	GetBorderColor(COLORREF *pClr) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	SetRenderingPrefs(DWORD dwRenderFlags) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	GetRenderingPrefs(DWORD *pdwRenderFlags) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	SetFullscreen(BOOL fFullscreen) {
		return E_NOTIMPL;
	};
	STDMETHODIMP	GetFullscreen(BOOL *pfFullscreen) {
		return E_NOTIMPL;
	};

private :
	HMODULE									m_hDXVA2Lib;
	PTR_DXVA2CreateDirect3DDeviceManager9	pfDXVA2CreateDirect3DDeviceManager9;
	PTR_DXVA2CreateVideoService				pfDXVA2CreateVideoService;

	CComPtr<IDirect3D9>						m_pD3D;
	CComPtr<IDirect3DDevice9>				m_pD3DDev;
	CComPtr<IDirect3DDeviceManager9>		m_pD3DDeviceManager;
	UINT									m_nResetTocken;
	HANDLE									m_hDevice;
	HWND									m_hWnd;

	void		CreateSurface();
};

CNullVideoRendererInputPin::CNullVideoRendererInputPin(CBaseRenderer *pRenderer, HRESULT *phr, LPCWSTR Name)
	: CRendererInputPin(pRenderer, phr, Name)
	, m_hDXVA2Lib(nullptr)
	, m_pD3DDev(nullptr)
	, m_pD3DDeviceManager(nullptr)
	, m_hDevice(INVALID_HANDLE_VALUE)
{
	CreateSurface();

	m_hDXVA2Lib = LoadLibraryW(L"dxva2.dll");
	if (m_hDXVA2Lib) {
		pfDXVA2CreateDirect3DDeviceManager9 = reinterpret_cast<PTR_DXVA2CreateDirect3DDeviceManager9>(GetProcAddress(m_hDXVA2Lib, "DXVA2CreateDirect3DDeviceManager9"));
		pfDXVA2CreateVideoService = reinterpret_cast<PTR_DXVA2CreateVideoService>(GetProcAddress(m_hDXVA2Lib, "DXVA2CreateVideoService"));
		pfDXVA2CreateDirect3DDeviceManager9(&m_nResetTocken, &m_pD3DDeviceManager);
	}

	// Initialize Device Manager with DX surface
	if (m_pD3DDev) {
		HRESULT hr;
		hr = m_pD3DDeviceManager->ResetDevice (m_pD3DDev, m_nResetTocken);
		hr = m_pD3DDeviceManager->OpenDeviceHandle(&m_hDevice);
	}
}

void CNullVideoRendererInputPin::CreateSurface()
{
	m_pD3D.Attach(D3D9Helper::Direct3DCreate9());

	m_hWnd = nullptr;	// TODO : put true window

	D3DDISPLAYMODE d3ddm;
	ZeroMemory(&d3ddm, sizeof(d3ddm));
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	D3DPRESENT_PARAMETERS pp;
	ZeroMemory(&pp, sizeof(pp));

	pp.Windowed = TRUE;
	pp.hDeviceWindow = m_hWnd;
	pp.SwapEffect = D3DSWAPEFFECT_COPY;
	pp.Flags = D3DPRESENTFLAG_VIDEO;
	pp.BackBufferCount = 1;
	pp.BackBufferWidth = d3ddm.Width;
	pp.BackBufferHeight = d3ddm.Height;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	HRESULT hr = m_pD3D->CreateDevice(
					D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd,
					D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, //D3DCREATE_MANAGED
					&pp, &m_pD3DDev);

	UNREFERENCED_PARAMETER(hr);
}

STDMETHODIMP CNullVideoRendererInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		(riid == __uuidof(IMFGetService)) ? GetInterface((IMFGetService*)this, ppv) :
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CNullVideoRendererInputPin::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
	if (m_pD3DDeviceManager != nullptr && guidService == MR_VIDEO_ACCELERATION_SERVICE) {
		if (riid == __uuidof(IDirect3DDeviceManager9)) {
			return m_pD3DDeviceManager->QueryInterface (riid, ppvObject);
		} else if (riid == __uuidof(IDirectXVideoDecoderService) || riid == __uuidof(IDirectXVideoProcessorService) ) {
			return m_pD3DDeviceManager->GetVideoService (m_hDevice, riid, ppvObject);
		} else if (riid == __uuidof(IDirectXVideoAccelerationService)) {
			// TODO : to be tested....
			return pfDXVA2CreateVideoService(m_pD3DDev, riid, ppvObject);
		} else if (riid == __uuidof(IDirectXVideoMemoryConfiguration)) {
			GetInterface ((IDirectXVideoMemoryConfiguration*)this, ppvObject);
			return S_OK;
		}
	} else if (guidService == MR_VIDEO_RENDER_SERVICE) {
		if (riid == __uuidof(IMFVideoDisplayControl)) {
			GetInterface ((IMFVideoDisplayControl*)this, ppvObject);
			return S_OK;
		}
	}
	//else if (guidService == MR_VIDEO_MIXER_SERVICE)
	//{
	//	if (riid == __uuidof(IMFVideoMixerBitmap))
	//	{
	//		GetInterface ((IMFVideoMixerBitmap*)this, ppvObject);
	//		return S_OK;
	//	}
	//}
	return E_NOINTERFACE;
}

STDMETHODIMP CNullVideoRendererInputPin::GetAvailableSurfaceTypeByIndex(DWORD dwTypeIndex, DXVA2_SurfaceType *pdwType)
{
	if (dwTypeIndex == 0) {
		*pdwType = DXVA2_SurfaceType_DecoderRenderTarget;
		return S_OK;
	} else {
		return MF_E_NO_MORE_TYPES;
	}
}

STDMETHODIMP CNullVideoRendererInputPin::SetSurfaceType(DXVA2_SurfaceType dwType)
{
	return S_OK;
}

STDMETHODIMP CNullVideoRendererInputPin::GetVideoWindow(HWND *phwndVideo)
{
	CheckPointer(phwndVideo, E_POINTER);
	*phwndVideo = m_hWnd;	// Important to implement this method (used by mpc)
	return S_OK;
}

#endif

//
// CNullRenderer
//

CNullRenderer::CNullRenderer(REFCLSID clsid, WCHAR* pName, LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseRenderer(clsid, pName, pUnk, phr)
{
}

//
// CNullVideoRenderer
//

CNullVideoRenderer::CNullVideoRenderer(LPUNKNOWN pUnk, HRESULT* phr)
	: CNullRenderer(__uuidof(this), L"Null Video Renderer", pUnk, phr)
{
}

HRESULT CNullVideoRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Video
		   || pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO
		   ? S_OK
		   : E_FAIL;
}

HRESULT CNullVideoRenderer::DoRenderSample(IMediaSample* pSample)
{
#if _DEBUG && 0
	static int nNb = 1;
	if (nNb < 100) {
		const long lSize = pSample->GetActualDataLength();
		BYTE* pMediaBuffer = nullptr;
		HRESULT hr = pSample->GetPointer(&pMediaBuffer);

		WCHAR strFile[MAX_PATH];
		swprintf_s(strFile, L"C:\\TEMP\\VideoData%03d.bin", nNb++);

		FILE* hFile;
		if (_wfopen_s(&hFile, strFile, L"wb") == 0) {
			fwrite(pMediaBuffer,
				   1,
				   lSize,
				   hFile);
			fclose(hFile);
		}
	}
#endif

	return S_OK;
}

//
// CNullUVideoRenderer
//

CNullUVideoRenderer::CNullUVideoRenderer(LPUNKNOWN pUnk, HRESULT* phr)
	: CNullRenderer(__uuidof(this), L"Null Video Renderer (Uncompressed)", pUnk, phr)
{
#ifdef USE_DXVA
	m_pInputPin = DNew CNullVideoRendererInputPin(this,phr,L"In");
#endif
}

HRESULT CNullUVideoRenderer::SetMediaType(const CMediaType *pmt)
{
	HRESULT hr = __super::SetMediaType(pmt);

	if (S_OK == hr) {
		m_mt = *pmt;

		if (m_mt.formattype == FORMAT_VideoInfo2) {
			BITMAPINFOHEADER& bih = ((VIDEOINFOHEADER2*)pmt->pbFormat)->bmiHeader;
			DLog(L"CNullUVideoRenderer::SetMediaType : %s, %dx%d", GetGUIDString(m_mt.subtype), bih.biWidth, bih.biHeight);
		}
		else {
			DLog(L"CNullUVideoRenderer::SetMediaType : %s", GetGUIDString(m_mt.subtype));
		}
	}

	return hr;
}

HRESULT CNullUVideoRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Video
		   && (pmt->subtype == MEDIASUBTYPE_YV12
			   || pmt->subtype == MEDIASUBTYPE_NV12
			   || pmt->subtype == MEDIASUBTYPE_I420
			   || pmt->subtype == MEDIASUBTYPE_YUYV
			   || pmt->subtype == MEDIASUBTYPE_IYUV
			   || pmt->subtype == MEDIASUBTYPE_YVU9
			   || pmt->subtype == MEDIASUBTYPE_Y411
			   || pmt->subtype == MEDIASUBTYPE_Y41P
			   || pmt->subtype == MEDIASUBTYPE_YUY2
			   || pmt->subtype == MEDIASUBTYPE_YVYU
			   || pmt->subtype == MEDIASUBTYPE_UYVY
			   || pmt->subtype == MEDIASUBTYPE_Y211
			   || pmt->subtype == MEDIASUBTYPE_AYUV
			   || pmt->subtype == MEDIASUBTYPE_YV16
			   || pmt->subtype == MEDIASUBTYPE_YV24
			   || pmt->subtype == MEDIASUBTYPE_P010
			   || pmt->subtype == MEDIASUBTYPE_P016
			   || pmt->subtype == MEDIASUBTYPE_P210
			   || pmt->subtype == MEDIASUBTYPE_P216
			   || pmt->subtype == MEDIASUBTYPE_RGB1
			   || pmt->subtype == MEDIASUBTYPE_RGB4
			   || pmt->subtype == MEDIASUBTYPE_RGB8
			   || pmt->subtype == MEDIASUBTYPE_RGB565
			   || pmt->subtype == MEDIASUBTYPE_RGB555
			   || pmt->subtype == MEDIASUBTYPE_RGB24
			   || pmt->subtype == MEDIASUBTYPE_RGB32
			   || pmt->subtype == MEDIASUBTYPE_ARGB1555
			   || pmt->subtype == MEDIASUBTYPE_ARGB4444
			   || pmt->subtype == MEDIASUBTYPE_ARGB32
			   || pmt->subtype == MEDIASUBTYPE_A2R10G10B10
			   || pmt->subtype == MEDIASUBTYPE_A2B10G10R10)
		   ? S_OK
		   : E_FAIL;
}

#include "../filters/renderer/VideoRenderers/Utils.h"
HRESULT CNullUVideoRenderer::DoRenderSample(IMediaSample* pSample)
{
#if _DEBUG && 0
	static int cnt = 0;

	if (cnt >= 20) {
		if (0) {
			cnt = 0; // loop
		} else {
			return S_OK; // no dump after 20 frames
		}
	}

	wchar_t strFile[MAX_PATH] = { 0 };
	swprintf_s(strFile, L"%sVideoDump%03d.bmp", GetProgramDir(), cnt++);

	if (CComQIPtr<IMFGetService> pService = pSample) {
		CComPtr<IDirect3DSurface9> pSurface;
		if (SUCCEEDED(pService->GetService(MR_BUFFER_SERVICE, IID_PPV_ARGS(&pSurface)))) {
			D3DSURFACE_DESC desc = {};
			D3DLOCKED_RECT r = {};

			if (S_OK == pSurface->GetDesc(&desc) && S_OK == pSurface->LockRect(&r, nullptr, D3DLOCK_READONLY)) {
				SaveRAWVideoAsBMP((BYTE*)r.pBits, desc.Format, r.Pitch, desc.Width, desc.Height, strFile);
				pSurface->UnlockRect();
			};
		}
	}
	else if (m_mt.formattype == FORMAT_VideoInfo2) {
		BYTE* data = nullptr;
		long size = pSample->GetActualDataLength();
		if (size > 0 && S_OK == pSample->GetPointer(&data)) {
			BITMAPINFOHEADER& bih = ((VIDEOINFOHEADER2*)m_mt.pbFormat)->bmiHeader;

			DWORD format = bih.biCompression;
			if (format == BI_RGB) {
				if (m_mt.subtype == MEDIASUBTYPE_RGB32) {
					format = D3DFMT_X8R8G8B8;
				} else if (m_mt.subtype == MEDIASUBTYPE_ARGB32) {
					format = D3DFMT_A8R8G8B8;
				}
			}

			SaveRAWVideoAsBMP(data, format, 0, bih.biWidth, bih.biHeight, strFile);
		}
	}
#endif

	return S_OK;
}

//
// CNullAudioRenderer
//

CNullAudioRenderer::CNullAudioRenderer(LPUNKNOWN pUnk, HRESULT* phr)
	: CNullRenderer(__uuidof(this), L"Null Audio Renderer", pUnk, phr)
{
}

HRESULT CNullAudioRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Audio
		   || pmt->majortype == MEDIATYPE_Midi
		   || pmt->subtype == MEDIASUBTYPE_MPEG2_AUDIO
		   || pmt->subtype == MEDIASUBTYPE_DOLBY_AC3
		   || pmt->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
		   || pmt->subtype == MEDIASUBTYPE_DTS
		   || pmt->subtype == MEDIASUBTYPE_SDDS
		   || pmt->subtype == MEDIASUBTYPE_MPEG1AudioPayload
		   || pmt->subtype == MEDIASUBTYPE_MPEG1Audio
		   ? S_OK
		   : E_FAIL;
}

//
// CNullUAudioRenderer
//

CNullUAudioRenderer::CNullUAudioRenderer(LPUNKNOWN pUnk, HRESULT* phr)
	: CNullRenderer(__uuidof(this), L"Null Audio Renderer (Uncompressed)", pUnk, phr)
{
}

HRESULT CNullUAudioRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Audio
		   && (pmt->subtype == MEDIASUBTYPE_PCM
			   || pmt->subtype == MEDIASUBTYPE_IEEE_FLOAT
			   || pmt->subtype == MEDIASUBTYPE_DRM_Audio
			   || pmt->subtype == MEDIASUBTYPE_DOLBY_AC3_SPDIF
			   || pmt->subtype == MEDIASUBTYPE_RAW_SPORT
			   || pmt->subtype == MEDIASUBTYPE_SPDIF_TAG_241h)
		   ? S_OK
		   : E_FAIL;
}

HRESULT CNullUAudioRenderer::DoRenderSample(IMediaSample* pSample)
{
#if _DEBUG && 0
	static int nNb = 1;
	if (nNb < 100) {
		const long lSize = pSample->GetActualDataLength();
		BYTE* pMediaBuffer = nullptr;
		HRESULT hr = pSample->GetPointer(&pMediaBuffer);

		WCHAR strFile[MAX_PATH];
		swprintf_s(strFile, L"C:\\TEMP\\AudioData%03d.bin", nNb++);

		FILE* hFile;
		if (_wfopen_s(&hFile, strFile, L"wb") == 0) {
			fwrite(pMediaBuffer,
				   1,
				   lSize,
				   hFile);
			fclose(hFile);
		}
	}
#endif

	return S_OK;
}

//
// CNullTextRenderer
//

HRESULT CNullTextRenderer::CTextInputPin::CheckMediaType(const CMediaType* pmt)
{
	return CMediaTypeEx(*pmt).ValidateSubtitle() ? S_OK : E_FAIL;
}

CNullTextRenderer::CNullTextRenderer(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseFilter(L"CNullTextRenderer", pUnk, this, __uuidof(this), phr)
{
	m_pInput.Attach(DNew CTextInputPin(this, this, phr));
}
