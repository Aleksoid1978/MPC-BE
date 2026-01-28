/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2026 see Authors.txt
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
#include <strsafe.h> // Required in CGenlock
#include <d3d9.h>
#include <dx/d3dx9.h>
#include <evr9.h>
#include <evr.h>
#include <mfapi.h>
#include <Mferror.h>
#include "SubPic/DX9SubPic.h"
#include "SubPic/SubPicQueueImpl.h"
#include <clsids.h>
#include "MacrovisionKicker.h"
#include "IPinHook.h"
#include "PixelShaderCompiler.h"
#include "SyncRenderer.h"
#include "FocusThread.h"
#include "Variables.h"
#include "Utils.h"
#include "DSUtil/D3D9Helper.h"
#include "DSUtil/DXVAState.h"
#include "apps/mplayerc/resource.h"
#include "apps/mplayerc/mpc_messages.h"

using namespace GothSync;

//#undef DLog
//#define DLog(...) Log2File(__VA_ARGS__)

// Guid to tag IMFSample with DirectX surface index
static const GUID GUID_SURFACE_INDEX = { 0x30c8e9f6, 0x415, 0x4b81, { 0xa3, 0x15, 0x1, 0xa, 0xc6, 0xa9, 0xda, 0x19 } };

//
// CBaseAP
//

CBaseAP::CBaseAP(HWND hWnd, bool bFullscreen, HRESULT& hr, CString &_Error)
	: CAllocatorPresenterImpl(hWnd, hr, &_Error)
	, m_bIsFullscreen(bFullscreen)
{
	DLog(L"CBaseAP::CBaseAP()");

	if (FAILED(hr)) {
		_Error += L"IAllocatorPresenterImpl failed\n";
		return;
	}

	HINSTANCE hDll = D3D9Helper::GetD3X9Dll();
	if (hDll) {
		(FARPROC&)m_pfD3DXCreateLine = GetProcAddress(hDll, "D3DXCreateLine");
	}

	m_hD3D9 = LoadLibraryW(L"d3d9.dll");
	if (m_hD3D9) {
		(FARPROC&)m_pfDirect3DCreate9Ex = GetProcAddress(m_hD3D9, "Direct3DCreate9Ex");
	}

	if (m_pfDirect3DCreate9Ex) {
		m_pfDirect3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D9Ex);
		if (!m_pD3D9Ex) {
			m_pfDirect3DCreate9Ex(D3D9b_SDK_VERSION, &m_pD3D9Ex);
		}
	}

	if (!m_pD3D9Ex) {
		hr = E_FAIL;
		_Error += L"Failed to create Direct3D 9Ex\n";
		return;
	}
}

CBaseAP::~CBaseAP()
{
	DLog(L"CBaseAP::~CBaseAP()");

	m_pLine.Release();
	m_Font3D.InvalidateDeviceObjects();
	m_pAlphaBitmapTexture.Release();

	m_pDevice9Ex.Release();
	m_pPSC.reset();
	m_pD3D9Ex.Release();
	if (m_hD3D9) {
		FreeLibrary(m_hD3D9);
		m_hD3D9 = nullptr;
	}
	m_pAudioStats.Release();
	SAFE_DELETE(m_pGenlock);

	if (m_FocusThread) {
		m_FocusThread->PostThreadMessageW(WM_QUIT, 0, 0);
		if (WaitForSingleObject(m_FocusThread->m_hThread, 10000) == WAIT_TIMEOUT) {
			ASSERT(FALSE);
			TerminateThread(m_FocusThread->m_hThread, 0xDEAD);
		}
	}
}

template<unsigned texcoords>
HRESULT CBaseAP::TextureBlt(IDirect3DDevice9* pD3DDev, MYD3DVERTEX<texcoords> v[4], D3DTEXTUREFILTERTYPE filter = D3DTEXF_LINEAR)
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
	hr = pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);

	for (unsigned i = 0; i < texcoords; i++) {
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MAGFILTER, filter);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MINFILTER, filter);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | FVF);
	// hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));
	std::swap(v[2], v[3]);
	hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));

	for (unsigned i = 0; i < texcoords; i++) {
		pD3DDev->SetTexture(i, nullptr);
	}

	return S_OK;
}

HRESULT CBaseAP::DrawRectBase(IDirect3DDevice9* pD3DDev, MYD3DVERTEX<0> v[4])
{
	if (!pD3DDev) {
		return E_POINTER;
	}

	HRESULT hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	hr = pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	hr = pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	hr = pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	hr = pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);

	hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX0 | D3DFVF_DIFFUSE);
	// hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));
	std::swap(v[2], v[3]);
	hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));

	return S_OK;
}

MFOffset CBaseAP::GetOffset(float v)
{
	MFOffset offset;
	offset.value = short(v);
	offset.fract = WORD(65536 * (v-offset.value));
	return offset;
}

MFVideoArea CBaseAP::GetArea(float x, float y, DWORD width, DWORD height)
{
	MFVideoArea area;
	area.OffsetX = GetOffset(x);
	area.OffsetY = GetOffset(y);
	area.Area.cx = width;
	area.Area.cy = height;
	return area;
}

static void GetMaxResolution(IDirect3D9Ex* pD3DEx, CSize& size)
{
	UINT cx = 0;
	UINT cy = 0;
	for (UINT adp = 0, num_adp = pD3DEx->GetAdapterCount(); adp < num_adp; ++adp) {
		D3DDISPLAYMODE d3ddm = { sizeof(d3ddm) };
		if (SUCCEEDED(pD3DEx->GetAdapterDisplayMode(adp, &d3ddm))) {
			cx = std::max(cx, d3ddm.Width);
			cy = std::max(cy, d3ddm.Height);
		}
	}

	size.SetSize(cx, cy);
}

HRESULT CBaseAP::CreateDXDevice(CString &_Error)
{
	DLog(L"--> CBaseAP::CreateDXDevice on thread: %u", GetCurrentThreadId());

	CAutoLock cRenderLock(&m_allocatorLock);

	HRESULT hr = E_FAIL;

	m_pLine.Release();
	m_Font3D.InvalidateDeviceObjects();

	m_pPSC.reset();
	m_pDevice9Ex.Release();

	for (auto& resizerShader : m_pResizerPixelShaders) {
		resizerShader.Release();
	}

	for (auto& Shader : m_pPixelShadersScreenSpace) {
		Shader.m_pPixelShader.Release();
	}

	for (auto& Shader : m_pPixelShaders) {
		Shader.m_pPixelShader.Release();
	}

	UINT currentAdapter = D3D9Helper::GetAdapter(m_pD3D9Ex, m_hWnd);
	bool bTryToReset = (currentAdapter == m_CurrentAdapter);

	if (!bTryToReset) {
		m_pDevice9Ex.Release();
		m_CurrentAdapter = currentAdapter;
	}

	if (!m_pD3D9Ex) {
		_Error += L"Failed to create Direct3D device\n";
		return E_UNEXPECTED;
	}

	D3DDISPLAYMODE d3ddm;
	ZeroMemory(&d3ddm, sizeof(d3ddm));
	if (FAILED(m_pD3D9Ex->GetAdapterDisplayMode(m_CurrentAdapter, &d3ddm))) {
		_Error += L"Can not retrieve display mode data\n";
		return E_UNEXPECTED;
	}

	if (FAILED(m_pD3D9Ex->GetDeviceCaps(m_CurrentAdapter, D3DDEVTYPE_HAL, &m_Caps))) {
		if ((m_Caps.Caps & D3DCAPS_READ_SCANLINE) == 0) {
			_Error += L"Video card does not have scanline access. Display synchronization is not possible.\n";
			return E_UNEXPECTED;
		}
	}

	m_refreshRate = d3ddm.RefreshRate;
	m_ScreenSize.SetSize(d3ddm.Width, d3ddm.Height);
	m_pGenlock->SetDisplayResolution(d3ddm.Width, d3ddm.Height);

	DwmIsCompositionEnabled(&m_bCompositionEnabled);

	CSize backBufferSize;
	GetMaxResolution(m_pD3D9Ex, backBufferSize);

	m_b10BitOutput = m_ExtraSets.b10BitOutput;

	{
		HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFOEXW mi = { sizeof(mi) };
		GetMonitorInfoW(hMonitor, (MONITORINFO*)&mi);
		DisplayConfig_t dc = {};
		bool ret = GetDisplayConfig(mi.szDevice, dc);

		if (dc.refreshRate.Numerator && dc.refreshRate.Denominator) {
			m_dRefreshRate = (double)dc.refreshRate.Numerator / (double)dc.refreshRate.Denominator;
		} else {
			m_dRefreshRate = d3ddm.RefreshRate;
		}
		m_dD3DRefreshCycle = 1000.0 / m_dRefreshRate; // In ms

		if (m_b10BitOutput) {
			m_b10BitOutput = (dc.bitsPerChannel >= 10);
			if (!m_b10BitOutput) {
				m_strMsgError = L"10 bit RGB is not enabled on this display or is not supported.";
			}
		}
	}

	ZeroMemory(&m_pp, sizeof(m_pp));
	if (m_bIsFullscreen) { // Exclusive mode fullscreen
		m_pp.Windowed = FALSE;
		m_pp.BackBufferWidth = d3ddm.Width;
		m_pp.BackBufferHeight = d3ddm.Height;
		m_pp.hDeviceWindow = m_hWnd;
		DLog(L"CBaseAP::CreateDXDevice : m_hWnd = %p", m_hWnd);
		m_pp.BackBufferCount = 3;
		m_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		m_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		m_pp.Flags = D3DPRESENTFLAG_VIDEO;

		if (m_b10BitOutput) {
			if (FAILED(m_pD3D9Ex->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_A2R10G10B10, D3DFMT_A2R10G10B10, false))) {
				m_strMsgError = L"10 bit RGB is not supported by this graphics device in this resolution.";
				m_b10BitOutput = false;
			}
		}

		if (m_b10BitOutput) {
			m_pp.BackBufferFormat = D3DFMT_A2R10G10B10;
		} else {
			m_pp.BackBufferFormat = d3ddm.Format;
		}

		if (!m_FocusThread) {
			m_FocusThread = (CFocusThread*)AfxBeginThread(RUNTIME_CLASS(CFocusThread), 0, 0, 0);
		}

		D3DDISPLAYMODEEX DisplayMode;
		ZeroMemory(&DisplayMode, sizeof(DisplayMode));
		DisplayMode.Size = sizeof(DisplayMode);
		m_pD3D9Ex->GetAdapterDisplayModeEx(m_CurrentAdapter, &DisplayMode, nullptr);

		DisplayMode.Format = m_pp.BackBufferFormat;
		m_pp.FullScreen_RefreshRateInHz = DisplayMode.RefreshRate;

		bTryToReset = bTryToReset && m_pDevice9Ex && SUCCEEDED(hr = m_pDevice9Ex->ResetEx(&m_pp, &DisplayMode));

		if (!bTryToReset) {
			m_pDevice9Ex.Release();

			hr = m_pD3D9Ex->CreateDeviceEx(
					m_CurrentAdapter, D3DDEVTYPE_HAL, m_FocusThread->GetFocusWindow(),
					D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_ENABLE_PRESENTSTATS | D3DCREATE_NOWINDOWCHANGES,
					&m_pp, &DisplayMode, &m_pDevice9Ex);
		}

		if (m_pDevice9Ex) {
			m_BackbufferFmt = m_pp.BackBufferFormat;
			m_DisplayFmt = DisplayMode.Format;
		}
	} else { // Windowed
		m_pp.Windowed = TRUE;
		m_pp.hDeviceWindow = m_hWnd;
		m_pp.SwapEffect = D3DSWAPEFFECT_COPY;
		m_pp.Flags = D3DPRESENTFLAG_VIDEO;
		m_pp.BackBufferCount = 1;
		m_pp.BackBufferWidth = backBufferSize.cx;
		m_pp.BackBufferHeight = backBufferSize.cy;
		m_BackbufferFmt = d3ddm.Format;
		m_DisplayFmt = d3ddm.Format;

		if (m_b10BitOutput) {
			if (FAILED(m_pD3D9Ex->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_A2R10G10B10, D3DFMT_A2R10G10B10, true))) {
				m_strMsgError = L"10 bit RGB is not supported by this graphics device in windowed mode.";
				m_b10BitOutput = false;
			}
		}

		if (m_b10BitOutput) {
			m_BackbufferFmt = D3DFMT_A2R10G10B10;
			m_pp.BackBufferFormat = D3DFMT_A2R10G10B10;
		}
		if (m_bCompositionEnabled) {
			// Desktop composition presents the whole desktop
			m_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		} else {
			m_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		}

		bTryToReset = bTryToReset && m_pDevice9Ex && SUCCEEDED(hr = m_pDevice9Ex->ResetEx(&m_pp, nullptr));

		if (!bTryToReset) {
			m_pDevice9Ex.Release();

			hr = m_pD3D9Ex->CreateDeviceEx(
					m_CurrentAdapter, D3DDEVTYPE_HAL, m_hWnd,
					D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_ENABLE_PRESENTSTATS,
					&m_pp, nullptr, &m_pDevice9Ex);
		}
	}

	if (m_pDevice9Ex) {
		while (hr == D3DERR_DEVICELOST) {
			DLog(L"D3DERR_DEVICELOST. Trying to Reset.");
			hr = m_pDevice9Ex->TestCooperativeLevel();
		}
		if (hr == D3DERR_DEVICENOTRESET) {
			DLog(L"D3DERR_DEVICENOTRESET");
			hr = m_pDevice9Ex->Reset(&m_pp);
		}

		if (m_pDevice9Ex) {
			m_pDevice9Ex->SetGPUThreadPriority(7);
		}
	}

	if (FAILED(hr)) {
		_Error.AppendFormat(L"CreateDevice() failed: %s\n", GetWindowsErrorMessage(hr, m_hD3D9));
		return hr;
	}

	m_pPSC.reset(DNew CPixelShaderCompiler(m_pDevice9Ex, true));

	if (m_Caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MINFLINEAR && m_Caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MAGFLINEAR) {
		m_filter = D3DTEXF_LINEAR;
	} else {
		m_filter = D3DTEXF_NONE;
	}

	InitMaxSubtitleTextureSize(m_SubpicSets.iMaxTexWidth, m_bIsFullscreen ? m_ScreenSize : backBufferSize);

	if (m_pSubPicAllocator) {
		m_pSubPicAllocator->ChangeDevice(m_pDevice9Ex);
	} else {
		m_pSubPicAllocator = DNew CDX9SubPicAllocator(m_pDevice9Ex, m_maxSubtitleTextureSize, false);
		if (!m_pSubPicAllocator) {
			_Error += L"CDX9SubPicAllocator failed\n";
			return E_FAIL;
		}
	}

	CComPtr<ISubPicProvider> pSubPicProvider;
	if (m_pSubPicQueue) {
		m_pSubPicQueue->GetSubPicProvider(&pSubPicProvider);
	}

	hr = S_OK;
	if (!m_pSubPicQueue) {
		CAutoLock cAutoLock(this);
		m_pSubPicQueue = m_SubpicSets.nCount > 0
						 ? (ISubPicQueue*)DNew CSubPicQueue(m_SubpicSets.nCount, !m_SubpicSets.bAnimationWhenBuffering, m_SubpicSets.bAllowDrop, m_pSubPicAllocator, &hr)
						 : (ISubPicQueue*)DNew CSubPicQueueNoThread(!m_SubpicSets.bAnimationWhenBuffering, m_pSubPicAllocator, &hr);
	} else {
		m_pSubPicQueue->Invalidate();
	}
	if (!m_pSubPicQueue || FAILED(hr)) {
		_Error += L"m_pSubPicQueue failed\n";
		return E_FAIL;
	}

	if (pSubPicProvider) {
		m_pSubPicQueue->SetSubPicProvider(pSubPicProvider);
	}

	HRESULT hr2 = m_Font3D.InitDeviceObjects(m_pDevice9Ex);
	if (SUCCEEDED(hr2)) {
		const long minWidth = 1600;
		int baseWidth = std::min(m_ScreenSize.cx, minWidth);
		m_TextScale = double(baseWidth) / double(minWidth);
		UINT fontH = (UINT)(24 * m_TextScale);
		UINT fontW = (UINT)(11 * m_TextScale);
		hr2 = m_Font3D.CreateFontBitmap(L"Lucida Console", fontH, fontW, baseWidth < 800 ? 0 : D3DFONT_BOLD);
		SIZE charSize = {};
		if (SUCCEEDED(hr2)) {
			charSize = m_Font3D.GetMaxCharMetric();
		}
	}
	DLogIf(FAILED(hr2), L"m_Font3D failed with error %s", HR2Str(hr2));

	if (m_pfD3DXCreateLine) {
		m_pfD3DXCreateLine (m_pDevice9Ex, &m_pLine);
	}
	m_LastAdapterCheck = GetPerfCounter();
	return S_OK;
}

HRESULT CBaseAP::AllocSurfaces(D3DFORMAT Format)
{
	CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_allocatorLock);

	for (unsigned i = 0; i < m_nSurfaces+2; i++) {
		m_pVideoTextures[i].Release();
		m_pVideoSurfaces[i].Release();
	}
	m_pResizeTexture.Release();
	m_pScreenSizeTextures[0].Release();
	m_pScreenSizeTextures[1].Release();
	m_SurfaceFmt = Format;
	m_strMixerOutputFmt.Format(L"Mixer output : %s", GetD3DFormatStr(m_SurfaceFmt));

	HRESULT hr;
	unsigned nTexturesNeeded = m_nSurfaces+2;

	for (unsigned i = 0; i < nTexturesNeeded; i++) {
		if (FAILED(hr = m_pDevice9Ex->CreateTexture(
							m_nativeVideoSize.cx, m_nativeVideoSize.cy, 1, D3DUSAGE_RENDERTARGET, Format, D3DPOOL_DEFAULT, &m_pVideoTextures[i], nullptr))) {
			return hr;
		}

		if (FAILED(hr = m_pVideoTextures[i]->GetSurfaceLevel(0, &m_pVideoSurfaces[i]))) {
			return hr;
		}
	}

	hr = m_pDevice9Ex->ColorFill(m_pVideoSurfaces[m_iCurSurface], nullptr, 0);
	return S_OK;
}

void CBaseAP::DeleteSurfaces()
{
	CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_allocatorLock);

	for (unsigned i = 0; i < m_nSurfaces+2; i++) {
		m_pVideoTextures[i].Release();
		m_pVideoSurfaces[i].Release();
	}
	m_pResizeTexture.Release();
}

// IAllocatorPresenter

STDMETHODIMP CBaseAP::DisableSubPicInitialization()
{
	if (m_pDevice9Ex) {
		// must be called before calling CreateRenderer
		return E_ILLEGAL_METHOD_CALL;
	}

	m_bEnableSubPic = false;

	return S_OK;
}

STDMETHODIMP_(SIZE) CBaseAP::GetVideoSize()
{
	SIZE size = __super::GetVideoSize();

	if (m_iRotation == 90 || m_iRotation == 270) {
		std::swap(size.cx, size.cy);
	}

	return size;
}

STDMETHODIMP_(SIZE) CBaseAP::GetVideoSizeAR()
{
	SIZE size = __super::GetVideoSizeAR();

	if (m_iRotation == 90 || m_iRotation == 270) {
		std::swap(size.cx, size.cy);
	}

	return size;
}

STDMETHODIMP CBaseAP::SetRotation(int rotation)
{
	if (AngleStep90(rotation)) {
		if (rotation != m_iRotation) {
			m_iRotation = rotation;
			m_bOtherTransform = true;
		}

		return S_OK;
	}

	return  E_INVALIDARG;
}

STDMETHODIMP_(int) CBaseAP::GetRotation()
{
	return m_iRotation;
}

STDMETHODIMP CBaseAP::SetFlip(bool flip)
{
	if (flip != m_bFlip) {
		m_bFlip = flip;
		m_bOtherTransform = true;
	}

	return S_OK;
}

STDMETHODIMP_(bool) CBaseAP::GetFlip()
{
	return m_bFlip;
}

bool CBaseAP::ClipToSurface(IDirect3DSurface9* pSurface, CRect& s, CRect& d)
{
	D3DSURFACE_DESC d3dsd;
	ZeroMemory(&d3dsd, sizeof(d3dsd));
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

HRESULT CBaseAP::InitShaderResizer()
{
	int iShader = -1;

	switch (m_ExtraSets.iResizer) {
	case RESIZER_NEAREST:
	case RESIZER_BILINEAR:
		return S_FALSE;
	case RESIZER_SHADER_BSPLINE:   iShader = shader_bspline_x;   break;
	case RESIZER_SHADER_MITCHELL:  iShader = shader_mitchell_x;  break;
	case RESIZER_SHADER_CATMULL:   iShader = shader_catmull_x;   break;
	case RESIZER_SHADER_BICUBIC06: iShader = shader_bicubic06_x; break;
	case RESIZER_SHADER_BICUBIC08: iShader = shader_bicubic08_x; break;
	case RESIZER_SHADER_BICUBIC10: iShader = shader_bicubic10_x; break;
	case RESIZER_SHADER_LANCZOS2:  iShader = shader_lanczos2_x;  break;
	case RESIZER_SHADER_LANCZOS3:  iShader = shader_lanczos3_x;  break;
	case RESIZER_DXVA2:
	case RESIZER_DXVAHD:
	default:
		m_ExtraSets.iResizer = RESIZER_BILINEAR;
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
	case shader_bspline_x:   resid = IDF_SHADER_RESIZER_BSPLINE4_X;  break;
	case shader_mitchell_x:  resid = IDF_SHADER_RESIZER_MITCHELL4_X; break;
	case shader_catmull_x:   resid = IDF_SHADER_RESIZER_CATMULL4_X;  break;
	case shader_bicubic06_x: resid = IDF_SHADER_RESIZER_BICUBIC06_X; break;
	case shader_bicubic08_x: resid = IDF_SHADER_RESIZER_BICUBIC08_X; break;
	case shader_bicubic10_x: resid = IDF_SHADER_RESIZER_BICUBIC10_X; break;
	case shader_lanczos2_x:  resid = IDF_SHADER_RESIZER_LANCZOS2_X;  break;
	case shader_lanczos3_x:  resid = IDF_SHADER_RESIZER_LANCZOS3_X;  break;
	default:
		return E_INVALIDARG;
	}

	HRESULT hr = CreateShaderFromResource(m_pDevice9Ex, &m_pResizerPixelShaders[iShader], resid);
	if (S_OK == hr) {
		hr = CreateShaderFromResource(m_pDevice9Ex, &m_pResizerPixelShaders[iShader + 1], resid + 1);
	}
	if (FAILED(hr)) {
		ASSERT(0);
		return hr;
	}

	if (!m_pResizerPixelShaders[shader_downscaling_x] || !m_pResizerPixelShaders[shader_downscaling_y]) {
		resid = IDF_SHADER_DOWNSCALER_SIMPLE_X;
		hr = CreateShaderFromResource(m_pDevice9Ex, &m_pResizerPixelShaders[shader_downscaling_x], resid);
		if (S_OK == hr) {
			hr = CreateShaderFromResource(m_pDevice9Ex, &m_pResizerPixelShaders[shader_downscaling_y], resid + 1);
		}
		ASSERT(S_OK == hr);
	}

	return S_OK;
}

HRESULT CBaseAP::TextureCopy(IDirect3DTexture9* pTexture)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	float w = (float)desc.Width;
	float h = (float)desc.Height;
	MYD3DVERTEX<1> v[] = {
		{0, 0, 0.5f, 2.0f, 0, 0},
		{w, 0, 0.5f, 2.0f, 1, 0},
		{0, h, 0.5f, 2.0f, 0, 1},
		{w, h, 0.5f, 2.0f, 1, 1},
	};
	for (auto& item : v) {
		item.x -= 0.5;
		item.y -= 0.5;
	}
	hr = m_pDevice9Ex->SetTexture(0, pTexture);
	return TextureBlt(m_pDevice9Ex, v, D3DTEXF_POINT);
}

HRESULT CBaseAP::TextureCopyRect(
	IDirect3DTexture9* pTexture,
	const CRect& srcRect, const CRect& dstRect,
	const D3DTEXTUREFILTERTYPE filter,
	const int iRotation, const bool bFlip)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	float dx = 1.0f / desc.Width;
	float dy = 1.0f / desc.Height;

	POINT points[4];
	switch (iRotation) {
	case 90:
		points[0] = { dstRect.right, dstRect.top };
		points[1] = { dstRect.right, dstRect.bottom };
		points[2] = { dstRect.left,  dstRect.top };
		points[3] = { dstRect.left,  dstRect.bottom };
		break;
	case 180:
		points[0] = { dstRect.right, dstRect.bottom };
		points[1] = { dstRect.left,  dstRect.bottom };
		points[2] = { dstRect.right, dstRect.top };
		points[3] = { dstRect.left,  dstRect.top };
		break;
	case 270:
		points[0] = { dstRect.left,  dstRect.bottom };
		points[1] = { dstRect.left,  dstRect.top };
		points[2] = { dstRect.right, dstRect.bottom };
		points[3] = { dstRect.right, dstRect.top };
		break;
	default:
		points[0] = { dstRect.left,  dstRect.top };
		points[1] = { dstRect.right, dstRect.top };
		points[2] = { dstRect.left,  dstRect.bottom };
		points[3] = { dstRect.right, dstRect.bottom };
		break;
	}

	if (bFlip) {
		std::swap(points[0], points[1]);
		std::swap(points[2], points[3]);
	}

	MYD3DVERTEX<1> v[] = {
		{(float)points[0].x - 0.5f, (float)points[0].y - 0.5f, 0.5f, 2.0f, {srcRect.left * dx, srcRect.top * dy} },
		{(float)points[1].x - 0.5f, (float)points[1].y - 0.5f, 0.5f, 2.0f, {srcRect.right * dx, srcRect.top * dy} },
		{(float)points[2].x - 0.5f, (float)points[2].y - 0.5f, 0.5f, 2.0f, {srcRect.left * dx, srcRect.bottom * dy} },
		{(float)points[3].x - 0.5f, (float)points[3].y - 0.5f, 0.5f, 2.0f, {srcRect.right * dx, srcRect.bottom * dy} },
	};

	hr = m_pDevice9Ex->SetTexture(0, pTexture);
	hr = m_pDevice9Ex->SetPixelShader(nullptr);
	hr = TextureBlt(m_pDevice9Ex, v, filter);

	return hr;
}

HRESULT CBaseAP::DrawRect(const D3DCOLOR _Color, const CRect &_Rect)
{
	MYD3DVERTEX<0> v[] = {
		{float(_Rect.left), float(_Rect.top), 0.5f, 2.0f, _Color},
		{float(_Rect.right), float(_Rect.top), 0.5f, 2.0f, _Color},
		{float(_Rect.left), float(_Rect.bottom), 0.5f, 2.0f, _Color},
		{float(_Rect.right), float(_Rect.bottom), 0.5f, 2.0f, _Color},
	};
	for (auto& item : v) {
		item.x -= 0.5;
		item.y -= 0.5;
	}
	return DrawRectBase(m_pDevice9Ex, v);
}

HRESULT CBaseAP::TextureResizeShader(
	IDirect3DTexture9* pTexture,
	const CRect& srcRect, const CRect& dstRect,
	IDirect3DPixelShader9* pShader,
	const int iRotation, const bool bFlip)
{
	HRESULT hr = S_OK;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	const float dx = 1.0f / desc.Width;
	const float dy = 1.0f / desc.Height;

	float scale_x = (float)srcRect.Width();
	float scale_y = (float)srcRect.Height();
	if (iRotation == 90 || iRotation == 270) {
		scale_x /= dstRect.Height();
		scale_y /= dstRect.Width();
	} else {
		scale_x /= dstRect.Width();
		scale_y /= dstRect.Height();
	}

	//const float steps_x = floor(scale_x + 0.5f);
	//const float steps_y = floor(scale_y + 0.5f);

	const float tx0 = (float)srcRect.left - 0.5f;
	const float ty0 = (float)srcRect.top - 0.5f;
	const float tx1 = (float)srcRect.right - 0.5f;
	const float ty1 = (float)srcRect.bottom - 0.5f;

	POINT points[4];
	switch (iRotation) {
	case 90:
		points[0] = { dstRect.right, dstRect.top };
		points[1] = { dstRect.right, dstRect.bottom };
		points[2] = { dstRect.left,  dstRect.top };
		points[3] = { dstRect.left,  dstRect.bottom };
		break;
	case 180:
		points[0] = { dstRect.right, dstRect.bottom };
		points[1] = { dstRect.left,  dstRect.bottom };
		points[2] = { dstRect.right, dstRect.top };
		points[3] = { dstRect.left,  dstRect.top };
		break;
	case 270:
		points[0] = { dstRect.left,  dstRect.bottom };
		points[1] = { dstRect.left,  dstRect.top };
		points[2] = { dstRect.right, dstRect.bottom };
		points[3] = { dstRect.right, dstRect.top };
		break;
	default:
		points[0] = { dstRect.left,  dstRect.top };
		points[1] = { dstRect.right, dstRect.top };
		points[2] = { dstRect.left,  dstRect.bottom };
		points[3] = { dstRect.right, dstRect.bottom };
		break;
	}

	if (bFlip) {
		std::swap(points[0], points[1]);
		std::swap(points[2], points[3]);
	}

	MYD3DVERTEX<1> v[] = {
		{ (float)points[0].x - 0.5f, (float)points[0].y - 0.5f, 0.5f, 2.0f, {tx0, ty0} },
		{ (float)points[1].x - 0.5f, (float)points[1].y - 0.5f, 0.5f, 2.0f, {tx1, ty0} },
		{ (float)points[2].x - 0.5f, (float)points[2].y - 0.5f, 0.5f, 2.0f, {tx0, ty1} },
		{ (float)points[3].x - 0.5f, (float)points[3].y - 0.5f, 0.5f, 2.0f, {tx1, ty1} },
	};

	float fConstData[][4] = {
		{ dx, dy, 0, 0 },
		{ scale_x, scale_y, 0, 0 },
	};
	hr = m_pDevice9Ex->SetPixelShaderConstantF(0, (float*)fConstData, std::size(fConstData));
	hr = m_pDevice9Ex->SetPixelShader(pShader);

	hr = m_pDevice9Ex->SetTexture(0, pTexture);
	hr = TextureBlt(m_pDevice9Ex, v, D3DTEXF_POINT);
	m_pDevice9Ex->SetPixelShader(nullptr);

	return hr;
}

HRESULT CBaseAP::ResizeShaderPass(
	IDirect3DTexture9* pTexture, IDirect3DSurface9* pRenderTarget,
	const CRect& srcRect, const CRect& dstRect,
	const int iShaderX)
{
	HRESULT hr = S_OK;
	const int w2 = dstRect.Width();
	const int h2 = dstRect.Height();
	const int k = 2;

	IDirect3DPixelShader9* pShaderUpscaleX = m_pResizerPixelShaders[iShaderX];
	IDirect3DPixelShader9* pShaderUpscaleY = m_pResizerPixelShaders[iShaderX+1];
	IDirect3DPixelShader9* pShaderDownscaleX = m_pResizerPixelShaders[shader_downscaling_x];
	IDirect3DPixelShader9* pShaderDownscaleY = m_pResizerPixelShaders[shader_downscaling_y];

	int w1, h1;
	IDirect3DPixelShader9* resizerX;
	IDirect3DPixelShader9* resizerY;
	if (m_iRotation == 90 || m_iRotation == 270) {
		w1 = srcRect.Height();
		h1 = srcRect.Width();
		resizerX = (w1 == w2) ? nullptr : (w1 > 2 * w2) ? pShaderDownscaleY : pShaderUpscaleY; // use Y scaling here
		if (resizerX) {
			resizerY = (h1 == h2) ? nullptr : (h1 > 2 * h2) ? pShaderDownscaleY : pShaderUpscaleY;
		} else {
			resizerY = (h1 == h2) ? nullptr : (h1 > 2 * h2) ? pShaderDownscaleX : pShaderUpscaleX; // use X scaling here
		}
	} else {
		w1 = srcRect.Width();
		h1 = srcRect.Height();
		resizerX = (w1 == w2) ? nullptr : (w1 > 2 * w2) ? pShaderDownscaleX : pShaderUpscaleX;
		resizerY = (h1 == h2) ? nullptr : (h1 > 2 * h2) ? pShaderDownscaleY : pShaderUpscaleY;
	}

	if (resizerX && resizerY) {
		// two pass resize

		D3DSURFACE_DESC desc;
		pRenderTarget->GetDesc(&desc);

		// check intermediate texture
		const UINT texWidth = desc.Width;
		const UINT texHeight = h1;

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
				DLog(L"CDX9VideoProcessor::ProcessTex() : m_TexResize.Create() failed with error %s", HR2Str(hr));
				return TextureCopyRect(pTexture, srcRect, dstRect, D3DTEXF_LINEAR, m_iRotation, m_bFlip);
			}
		}

		CRect resizeRect(dstRect.left, 0, dstRect.right, texHeight);
		CComPtr<IDirect3DSurface9> pResizeSurface;
		hr = m_pResizeTexture->GetSurfaceLevel(0, &pResizeSurface);

		// resize width
		hr = m_pDevice9Ex->SetRenderTarget(0, pResizeSurface);
		hr = TextureResizeShader(pTexture, srcRect, resizeRect, resizerX, m_iRotation, m_bFlip);

		// resize height
		hr = m_pDevice9Ex->SetRenderTarget(0, pRenderTarget);
		hr = TextureResizeShader(m_pResizeTexture, resizeRect, dstRect, resizerY, 0, false);
	}
	else {
		hr = m_pDevice9Ex->SetRenderTarget(0, pRenderTarget);

		if (resizerX) {
			// one pass resize for width
			hr = TextureResizeShader(pTexture, srcRect, dstRect, resizerX, m_iRotation, m_bFlip);
		}
		else if (resizerY) {
			// one pass resize for height
			hr = TextureResizeShader(pTexture, srcRect, dstRect, resizerY, m_iRotation, m_bFlip);
		}
		else {
			// no resize
			hr = m_pDevice9Ex->SetRenderTarget(0, pRenderTarget);
			hr = TextureCopyRect(pTexture, srcRect, dstRect, D3DTEXF_POINT, m_iRotation, m_bFlip);
		}
	}

	DLogIf(FAILED(hr), L"CDX9VideoProcessor::ResizeShaderPass() : failed with error %s", HR2Str(hr));

	return hr;
}

HRESULT CBaseAP::AlphaBlt(RECT* pSrc, RECT* pDst, IDirect3DTexture9* pTexture)
{
	if (!pSrc || !pDst) {
		return E_POINTER;
	}

	CRect src(*pSrc), dst(*pDst);

	HRESULT hr;

	do {
		D3DSURFACE_DESC d3dsd;
		ZeroMemory(&d3dsd, sizeof(d3dsd));
		if (FAILED(pTexture->GetLevelDesc(0, &d3dsd)) /*|| d3dsd.Type != D3DRTYPE_TEXTURE*/) {
			break;
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

		hr = m_pDevice9Ex->SetPixelShader(nullptr);

		hr = m_pDevice9Ex->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
		hr = m_pDevice9Ex->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

		m_pDevice9Ex->SetTexture(0, nullptr);

		m_pDevice9Ex->SetRenderState(D3DRS_ALPHABLENDENABLE, abe);
		m_pDevice9Ex->SetRenderState(D3DRS_SRCBLEND, sb);
		m_pDevice9Ex->SetRenderState(D3DRS_DESTBLEND, db);

		return S_OK;
	} while (0);
	return E_FAIL;
}

// Update the array m_llJitters with a new vsync period. Calculate min, max and stddev.
void CBaseAP::SyncStats(LONGLONG syncTime)
{
	m_nNextJitter = (m_nNextJitter+1) % NB_JITTER;
	LONGLONG jitter = syncTime - m_llLastSyncTime;
	m_llJitters[m_nNextJitter] = jitter;
	double syncDeviation = (m_llJitters[m_nNextJitter] - m_fJitterMean) / 10000.0;
	if (abs(syncDeviation) > (GetDisplayCycle() / 2)) {
		m_uSyncGlitches++;
	}

	LONGLONG llJitterSum = 0;
	for (const auto& jitter : m_llJitters) {
		llJitterSum += jitter;
	}
	m_fJitterMean = double(llJitterSum) / NB_JITTER;

	double DeviationSum = 0;
	for (const auto& jitter : m_llJitters) {
		double deviation = jitter - m_fJitterMean;
		DeviationSum += deviation * deviation;
		LONGLONG deviationInt = std::llround(deviation);
		expand_range(deviationInt, m_MinJitter, m_MaxJitter);
	}

	m_fJitterStdDev = sqrt(DeviationSum/NB_JITTER);
	m_fAvrFps = 10000000.0/(double(llJitterSum)/NB_JITTER);
	m_llLastSyncTime = syncTime;
}

// Collect the difference between periodEnd and periodStart in an array, calculate mean and stddev.
void CBaseAP::SyncOffsetStats(LONGLONG syncOffset)
{
	m_nNextSyncOffset = (m_nNextSyncOffset+1) % NB_JITTER;
	m_llSyncOffsets[m_nNextSyncOffset] = syncOffset;

	LONGLONG AvrageSum = 0;
	for (const auto& offset : m_llSyncOffsets) {
		AvrageSum += offset;
		expand_range(offset, m_MinSyncOffset, m_MaxSyncOffset);
	}
	double MeanOffset = double(AvrageSum)/NB_JITTER;

	double DeviationSum = 0;
	for (const auto& offset : m_llSyncOffsets) {
		double Deviation = double(offset) - MeanOffset;
		DeviationSum += Deviation*Deviation;
	}
	double StdDev = sqrt(DeviationSum/NB_JITTER);

	m_fSyncOffsetAvr = MeanOffset;
	m_fSyncOffsetStdDev = StdDev;
}

// Present a sample (frame) using DirectX.
STDMETHODIMP_(bool) CBaseAP::Paint(bool fAll)
{
	if (m_bPendingResetDevice) {
		SendResetRequest();
		return false;
	}

	D3DRASTER_STATUS rasterStatus;
	REFERENCE_TIME llCurRefTime = 0;
	REFERENCE_TIME llSyncOffset = 0;
	double dSyncOffset = 0.0;

	CAutoLock cRenderLock(&m_allocatorLock);

	// Estimate time for next vblank based on number of remaining lines in this frame. This algorithm seems to be
	// accurate within one ms why there should not be any need for a more accurate one. The wiggly line seen
	// when using sync to nearest and sync display is most likely due to inaccuracies in the audio-card-based
	// reference clock. The wiggles are not seen with the perfcounter-based reference clock of the sync to video option.
	m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
	m_uScanLineEnteringPaint = rasterStatus.ScanLine;
	if (m_pRefClock) {
		m_pRefClock->GetTime(&llCurRefTime);
	}
	int dScanLines = std::max(m_ScreenSize.cy - (LONG)m_uScanLineEnteringPaint, 0L);
	dSyncOffset = dScanLines * m_dDetectedScanlineTime; // ms
	llSyncOffset = REFERENCE_TIME(10000.0 * dSyncOffset); // Reference time units (100 ns)
	m_llEstVBlankTime = llCurRefTime + llSyncOffset; // Estimated time for the start of next vblank

	if (m_windowRect.right <= m_windowRect.left || m_windowRect.bottom <= m_windowRect.top
			|| m_nativeVideoSize.cx <= 0 || m_nativeVideoSize.cy <= 0
			|| !m_pVideoSurfaces[0]) {
		return false;
	}

	HRESULT hr;
	CRect rSrcVid(CPoint(0, 0), m_nativeVideoSize);
	CRect rDstVid(m_videoRect);
	CRect rSrcPri(CPoint(0, 0), m_windowRect.Size());
	CRect rDstPri(m_windowRect);

	m_pDevice9Ex->BeginScene();

	CComPtr<IDirect3DSurface9> pBackBuffer;
	m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	m_pDevice9Ex->SetRenderTarget(0, pBackBuffer);
	hr = m_pDevice9Ex->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);

	if (!rDstVid.IsRectEmpty()) {
		if (m_pVideoTextures[m_iCurSurface]) {
			CComPtr<IDirect3DTexture9> pVideoTexture = m_pVideoTextures[m_iCurSurface];

			unsigned src = m_iCurSurface;
			unsigned dst = m_nSurfaces;

			if (m_inputExtFormat.VideoTransferMatrix == VIDEOTRANSFERMATRIX_YCgCo) {
				if (!m_pPSCorrectionYCgCo) {
					hr = CreateShaderFromResource(m_pDevice9Ex, &m_pPSCorrectionYCgCo, IDF_SHADER_CORRECTION_YCGCO);
				}

				if (m_pPSCorrectionYCgCo) {
					hr = m_pDevice9Ex->SetRenderTarget(0, m_pVideoSurfaces[dst]);
					hr = m_pDevice9Ex->SetPixelShader(m_pPSCorrectionYCgCo);
					TextureCopy(m_pVideoTextures[src]);
					pVideoTexture = m_pVideoTextures[dst];
					src = dst;
					dst++;

					hr = m_pDevice9Ex->SetRenderTarget(0, pBackBuffer);
					hr = m_pDevice9Ex->SetPixelShader(nullptr);
				}
			}

			// pre-resize pixel shaders
			if (m_pPixelShaders.size()) {
				static __int64 counter = 0;
				static long start = clock();

				long stop = clock();
				long diff = stop - start;

				if (diff >= 10*60*CLOCKS_PER_SEC) {
					start = stop;    // reset after 10 min (ps float has its limits in both range and accuracy)
				}

				D3DSURFACE_DESC desc;
				m_pVideoTextures[src]->GetLevelDesc(0, &desc);

				float fConstData[][4] = {
					{(float)desc.Width, (float)desc.Height, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
					{1.0f / desc.Width, 1.0f / desc.Height, 0, 0},
				};
				hr = m_pDevice9Ex->SetPixelShaderConstantF(0, (float*)fConstData, std::size(fConstData));

				for (auto& Shader : m_pPixelShaders) {
					hr = m_pDevice9Ex->SetRenderTarget(0, m_pVideoSurfaces[dst]);
					if (!Shader.m_pPixelShader) {
						Shader.Compile(m_pPSC.get());
					}
					hr = m_pDevice9Ex->SetPixelShader(Shader.m_pPixelShader);
					TextureCopy(m_pVideoTextures[src]);
					pVideoTexture = m_pVideoTextures[dst];

					src = dst;
					if (++dst >= m_nSurfaces+2) {
						dst = m_nSurfaces;
					}
				}

				hr = m_pDevice9Ex->SetRenderTarget(0, pBackBuffer);
				hr = m_pDevice9Ex->SetPixelShader(nullptr);
			}

			// init post-resize pixel shaders
			bool bScreenSpacePixelShaders = m_pPixelShadersScreenSpace.size() > 0;
			if (bScreenSpacePixelShaders && (!m_pScreenSizeTextures[0] || !m_pScreenSizeTextures[1])) {
				UINT texWidth = std::min((DWORD)m_ScreenSize.cx, m_Caps.MaxTextureWidth);
				UINT texHeight = std::min((DWORD)m_ScreenSize.cy, m_Caps.MaxTextureHeight);

				if (D3D_OK != m_pDevice9Ex->CreateTexture(
							texWidth, texHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
							D3DPOOL_DEFAULT, &m_pScreenSizeTextures[0], nullptr)
					|| D3D_OK != m_pDevice9Ex->CreateTexture(
							texWidth, texHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
							D3DPOOL_DEFAULT, &m_pScreenSizeTextures[1], nullptr)) {
					ASSERT(0);
					m_pScreenSizeTextures[0].Release();
					m_pScreenSizeTextures[1].Release();
					bScreenSpacePixelShaders = false; // will do 1 pass then
				}
			}

			CComPtr<IDirect3DSurface9> pRT;

			if (bScreenSpacePixelShaders) {
				hr = m_pScreenSizeTextures[1]->GetSurfaceLevel(0, &pRT);
				if (hr != S_OK) {
					bScreenSpacePixelShaders = false;
				}
				if (bScreenSpacePixelShaders) {
					hr = m_pDevice9Ex->SetRenderTarget(0, pRT);
					if (hr != S_OK) {
						bScreenSpacePixelShaders = false;
					}
					hr = m_pDevice9Ex->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
				}
			}
			if (!pRT) {
				pRT = pBackBuffer;
			}

			// resize
			if (rSrcVid.Size() != rDstVid.Size()) {
				// init resizer
				hr = InitShaderResizer();

				switch (m_ExtraSets.iResizer) {
				case RESIZER_NEAREST:
					m_wsResizer = L"Nearest neighbor";
					hr = TextureCopyRect(pVideoTexture, rSrcVid, rDstVid, D3DTEXF_POINT, m_iRotation, m_bFlip);
					break;
				case RESIZER_BILINEAR:
					m_wsResizer = L"Bilinear";
					hr = TextureCopyRect(pVideoTexture, rSrcVid, rDstVid, D3DTEXF_LINEAR, m_iRotation, m_bFlip);
					break;
				case RESIZER_SHADER_BSPLINE:
					m_wsResizer = L"B-spline";
					hr = ResizeShaderPass(pVideoTexture, pRT, rSrcVid, rDstVid, shader_bspline_x);
					break;
				case RESIZER_SHADER_MITCHELL:
					m_wsResizer = L"Mitchell-Netravali";
					hr = ResizeShaderPass(pVideoTexture, pRT, rSrcVid, rDstVid, shader_mitchell_x);
					break;
				case RESIZER_SHADER_CATMULL:
					m_wsResizer = L"Catmull-Rom";
					hr = ResizeShaderPass(pVideoTexture, pRT, rSrcVid, rDstVid, shader_catmull_x);
					break;
				case RESIZER_SHADER_BICUBIC06:
					m_wsResizer = L"Bicubic A=-0.6";
					hr = ResizeShaderPass(pVideoTexture, pRT, rSrcVid, rDstVid, shader_bicubic06_x);
					break;
				case RESIZER_SHADER_BICUBIC08:
					m_wsResizer = L"Bicubic A=-0.8";
					hr = ResizeShaderPass(pVideoTexture, pRT, rSrcVid, rDstVid, shader_bicubic08_x);
					break;
				case RESIZER_SHADER_BICUBIC10:
					m_wsResizer = L"Bicubic A=-1.0";
					hr = ResizeShaderPass(pVideoTexture, pRT, rSrcVid, rDstVid, shader_bicubic10_x);
					break;
				case RESIZER_SHADER_LANCZOS2:
					m_wsResizer = L"Lanczos2";
					hr = ResizeShaderPass(pVideoTexture, pRT, rSrcVid, rDstVid, shader_lanczos2_x);
					break;
				case RESIZER_SHADER_LANCZOS3:
					m_wsResizer = L"Lanczos3";
					hr = ResizeShaderPass(pVideoTexture, pRT, rSrcVid, rDstVid, shader_lanczos3_x);
					break;
				}
			} else {
				m_wsResizer = L""; // empty string, not nullptr
				hr = TextureCopyRect(pVideoTexture, rSrcVid, rDstVid, D3DTEXF_POINT, m_iRotation, m_bFlip);
			}

			// post-resize pixel shaders
			if (bScreenSpacePixelShaders) {
				static __int64 counter = 555;
				static long start = clock() + 333;

				long stop = clock() + 333;
				long diff = stop - start;

				if (diff >= 10*60*CLOCKS_PER_SEC) {
					start = stop;    // reset after 10 min (ps float has its limits in both range and accuracy)
				}

				D3DSURFACE_DESC desc;
				m_pScreenSizeTextures[0]->GetLevelDesc(0, &desc);

				float fConstData[][4] = {
					{(float)desc.Width, (float)desc.Height, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
					{1.0f / desc.Width, 1.0f / desc.Height, 0, 0},
				};
				hr = m_pDevice9Ex->SetPixelShaderConstantF(0, (float*)fConstData, std::size(fConstData));

				int src = 1, dst = 0;

				if (m_pPixelShadersScreenSpace.size()) {
					const auto it_last = --m_pPixelShadersScreenSpace.end();

					for (auto it = m_pPixelShadersScreenSpace.begin(), end = m_pPixelShadersScreenSpace.end(); it != end; ++it) {
						if (it == it_last) {
							m_pDevice9Ex->SetRenderTarget(0, pBackBuffer);
						}
						else {
							CComPtr<IDirect3DSurface9> pRT;
							hr = m_pScreenSizeTextures[dst]->GetSurfaceLevel(0, &pRT);
							m_pDevice9Ex->SetRenderTarget(0, pRT);
						}

						auto& Shader = (*it);
						if (!Shader.m_pPixelShader) {
							Shader.Compile(m_pPSC.get());
						}
						hr = m_pDevice9Ex->SetPixelShader(Shader.m_pPixelShader);
						TextureCopy(m_pScreenSizeTextures[src]);

						std::swap(src, dst);
					}
				}

				hr = m_pDevice9Ex->SetPixelShader(nullptr);
			}
		} else {
			if (pBackBuffer) {
				ClipToSurface(pBackBuffer, rSrcVid, rDstVid);
				// rSrcVid has to be aligned on mod2 for yuy2->rgb conversion with StretchRect
				rSrcVid.left &= ~1;
				rSrcVid.right &= ~1;
				rSrcVid.top &= ~1;
				rSrcVid.bottom &= ~1;
				hr = m_pDevice9Ex->StretchRect(m_pVideoSurfaces[m_iCurSurface], rSrcVid, pBackBuffer, rDstVid, m_filter);
				if (FAILED(hr)) {
					return false;
				}
			}
		}
	}

	AlphaBltSubPic(rDstPri, rDstVid);

	if (m_ExtraSets.iDisplayStats) {
		DrawStats();
	}

	if (m_bAlphaBitmapEnable && m_pAlphaBitmapTexture) {
		AlphaBlt(rSrcPri, rDstPri, m_pAlphaBitmapTexture);
	}

	m_pDevice9Ex->EndScene();

	if (m_bIsFullscreen) {
		hr = m_pDevice9Ex->PresentEx(nullptr, nullptr, nullptr, nullptr, 0);
	} else {
		hr = m_pDevice9Ex->PresentEx(rSrcPri, rDstPri, nullptr, nullptr, 0);
	}
	if (FAILED(hr)) {
		DLog(L"CBaseAP::Paint : Device lost or something");
	}

	// Calculate timing statistics
	if (m_pRefClock) {
		m_pRefClock->GetTime(&llCurRefTime);    // To check if we called Present too late to hit the right vsync
	}
	m_llEstVBlankTime = std::max(m_llEstVBlankTime, llCurRefTime); // Sometimes the real value is larger than the estimated value (but never smaller)
	if (m_ExtraSets.iDisplayStats < 3) { // Partial on-screen statistics
		SyncStats(m_llEstVBlankTime);    // Max of estimate and real. Sometimes Present may actually return immediately so we need the estimate as a lower bound
	}
	if (m_ExtraSets.iDisplayStats == 1) { // Full on-screen statistics
		SyncOffsetStats(-llSyncOffset);    // Minus because we want time to flow downward in the graph in DrawStats
	}

	// Adjust sync
	double frameCycle = (m_llSampleTime - m_llLastSampleTime) / 10000.0;
	if (frameCycle < 0) {
		frameCycle = 0.0;    // Happens when searching.
	}

	if (m_ExtraSets.iSynchronizeMode == SYNCHRONIZE_VIDEO) {
		m_pGenlock->ControlClock(dSyncOffset, frameCycle);
	} else {
		m_pGenlock->UpdateStats(dSyncOffset, frameCycle);    // No sync or sync to nearest neighbor
	}

	m_dFrameCycle = m_pGenlock->frameCycleAvg;
	if (m_dFrameCycle > 0.0) {
		m_fps = 1000.0 / m_dFrameCycle;
	}
	m_dCycleDifference = GetCycleDifference();
	if (abs(m_dCycleDifference) < 0.05) { // If less than 5% speed difference
		m_bSnapToVSync = true;
	} else {
		m_bSnapToVSync = false;
	}

	// Check how well audio is matching rate (if at all)
	DWORD tmp;
	if (m_pAudioStats != nullptr) {
		m_pAudioStats->GetStatParam(AM_AUDREND_STAT_PARAM_SLAVE_ACCUMERROR, (DWORD*)&m_lAudioLag, &tmp);
		expand_range((long)m_lAudioLag, m_lAudioLagMin, m_lAudioLagMax);
		m_pAudioStats->GetStatParam(AM_AUDREND_STAT_PARAM_SLAVE_MODE, &m_lAudioSlaveMode, &tmp);
	}

	bool bResetDevice = m_bPendingResetDevice;
	if (hr == D3DERR_DEVICELOST && m_pDevice9Ex->TestCooperativeLevel() == D3DERR_DEVICENOTRESET || hr == S_PRESENT_MODE_CHANGED) {
		bResetDevice = true;
	}
	if (m_bNeedResetDevice) {
		m_bNeedResetDevice = false;
		bResetDevice = true;
	}

	BOOL bCompositionEnabled = false;
	DwmIsCompositionEnabled(&bCompositionEnabled);

	if (bCompositionEnabled != m_bCompositionEnabled) {
		if (m_bIsFullscreen) {
			m_bCompositionEnabled = bCompositionEnabled;
		} else {
			bResetDevice = true;
		}
	}

	if (m_ExtraSets.bResetDevice) {
		LONGLONG time = GetPerfCounter();
		if (time > m_LastAdapterCheck + 20000000) { // check every 2 sec.
			m_LastAdapterCheck = time;
#ifdef _DEBUG
			D3DDEVICE_CREATION_PARAMETERS Parameters;
			if (SUCCEEDED(m_pDevice9Ex->GetCreationParameters(&Parameters))) {
				ASSERT(Parameters.AdapterOrdinal == m_CurrentAdapter);
			}
#endif
			if (m_CurrentAdapter != D3D9Helper::GetAdapter(m_pD3D9Ex, m_hWnd)) {
				bResetDevice = true;
			}
#ifdef _DEBUG
			else {
				ASSERT(m_pD3D9Ex->GetAdapterMonitor(m_CurrentAdapter) == m_pD3D9Ex->GetAdapterMonitor(D3D9Helper::GetAdapter(m_pD3D9Ex, m_hWnd)));
			}
#endif
		}
	}

	if (bResetDevice) {
		m_bPendingResetDevice = true;
		SendResetRequest();
	}
	return true;
}

void CBaseAP::SendResetRequest()
{
	if (!m_bDeviceResetRequested) {
		m_bDeviceResetRequested = true;
		AfxGetApp()->m_pMainWnd->PostMessage(WM_RESET_DEVICE);
	}
}

STDMETHODIMP_(bool) CBaseAP::ResetDevice()
{
	DeleteSurfaces();
	HRESULT hr;
	CString Error;
	if (FAILED(hr = CreateDXDevice(Error)) || FAILED(hr = AllocSurfaces())) {
		m_bDeviceResetRequested = false;
		return false;
	}
	m_pGenlock->SetMonitor(D3D9Helper::GetAdapter(m_pD3D9Ex, m_hWnd));
	OnResetDevice();
	m_bDeviceResetRequested = false;
	m_bPendingResetDevice = false;
	return true;
}

STDMETHODIMP_(bool) CBaseAP::DisplayChange()
{
	return true;
}

STDMETHODIMP_(void) CBaseAP::ResetStats()
{
	CAutoLock cRenderLock(&m_allocatorLock);

	m_pGenlock->ResetStats();
	m_lAudioLag = 0;
	m_lAudioLagMin = 10000;
	m_lAudioLagMax = -10000;
	m_MinJitter = MAXLONG64;
	m_MaxJitter = MINLONG64;
	m_MinSyncOffset = MAXLONG64;
	m_MaxSyncOffset = MINLONG64;
	m_uSyncGlitches = 0;
	m_pcFramesDropped = 0;
}

STDMETHODIMP_(void) CBaseAP::SetExtraSettings(ExtraRendererSettings* pExtraSets)
{
	if (pExtraSets) {
		CAutoLock cRenderLock(&m_allocatorLock);

		if (m_pDevice9Ex) {
			if (pExtraSets->b10BitOutput != m_ExtraSets.b10BitOutput || pExtraSets->iSurfaceFormat != m_ExtraSets.iSurfaceFormat) {
				m_bNeedResetDevice = true;
			}
		}

		m_ExtraSets = *pExtraSets;
		m_nSurfaces = std::clamp(m_ExtraSets.nEVRBuffers, 2, MAX_PICTURE_SLOTS - 2);
	}
}

// ISubRenderOptions

STDMETHODIMP CBaseAP::GetInt(LPCSTR field, int* value)
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

STDMETHODIMP CBaseAP::GetString(LPCSTR field, LPWSTR* value, int* chars)
{
	CheckPointer(value, E_POINTER);
	CheckPointer(chars, E_POINTER);

	if (!strcmp(field, "name")) {
		CStringW ret = L"MPC-BE: EVR Sync";
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

void CBaseAP::DrawStats()
{
	LONGLONG llMaxJitter = m_MaxJitter;
	LONGLONG llMinJitter = m_MinJitter;

	// pApp->m_iDisplayStats = 1 for full stats, 2 for little less, 3 for basic, 0 for no stats
	{
		CString strText;
		strText.Preallocate(1000);

		strText.Format(L"Frames drawn from stream start: %u | Sample time stamp: %d ms", m_pcFramesDrawn, (int)(m_llSampleTime / 10000));

		if (m_ExtraSets.iDisplayStats == 1) {
			strText.AppendFormat(L"\nFrame cycle  : %.3f ms [%.3f ms, %.3f ms]  Actual  %+5.3f ms [%+.3f ms, %+.3f ms]", m_dFrameCycle, m_pGenlock->minFrameCycle, m_pGenlock->maxFrameCycle, m_fJitterMean / 10000.0, (double(llMinJitter)/10000.0), (double(llMaxJitter)/10000.0));

			strText.AppendFormat(L"\nDisplay cycle: Measured closest match %.3f ms   Measured base %.3f ms", m_dOptimumDisplayCycle, m_dEstRefreshCycle);

			strText.AppendFormat(L"\nFrame rate   : %.3f fps   Actual frame rate: %.3f fps", m_fps, 10000000.0 / m_fJitterMean);

			strText.AppendFormat(L"\nDisplay      : %d x %d, %.3f Hz  Cycle %.3f ms", m_ScreenSize.cx, m_ScreenSize.cy, m_dRefreshRate, m_dD3DRefreshCycle);

			if ((m_Caps.Caps & D3DCAPS_READ_SCANLINE) == 0) {
				strText.Append(L"\nScan line err: Graphics device does not support scan line access. No sync is possible");
			}

#ifdef _DEBUG
			if (m_pDevice9Ex) {
				CComPtr<IDirect3DSwapChain9> pSC;
				HRESULT hr = m_pDevice9Ex->GetSwapChain(0, &pSC);
				CComQIPtr<IDirect3DSwapChain9Ex> pSCEx(pSC);
				if (pSCEx) {
					D3DPRESENTSTATS stats;
					hr = pSCEx->GetPresentStats(&stats);
					if (SUCCEEDED(hr)) {
						strText.Append(L"\nGraphics device present stats:");

						strText.AppendFormat(L"\n    PresentCount %u PresentRefreshCount %u SyncRefreshCount %u",
									   stats.PresentCount, stats.PresentRefreshCount, stats.SyncRefreshCount);

						LARGE_INTEGER Freq;
						QueryPerformanceFrequency (&Freq);
						Freq.QuadPart /= 1000;
						strText.AppendFormat(L"\n    SyncQPCTime %I64dms SyncGPUTime %I64dms",
									   stats.SyncQPCTime.QuadPart / Freq.QuadPart, stats.SyncGPUTime.QuadPart / Freq.QuadPart);
					} else {
						strText.Append(L"\nGraphics device does not support present stats");
					}
				}
			}
#endif

			strText.AppendFormat(L"\nVideo size   : %d x %d (%d:%d)", m_nativeVideoSize.cx, m_nativeVideoSize.cy, m_aspectRatio.cx, m_aspectRatio.cy);
			CSize videoSize = m_videoRect.Size();
			if (m_nativeVideoSize != videoSize) {
				strText.AppendFormat(L" -> %d x %d %s", videoSize.cx, videoSize.cy, m_wsResizer);
			}

			if (m_ExtraSets.iSynchronizeMode != SYNCHRONIZE_NEAREST) {
				strText.AppendFormat(L"\nSync adjust  : %d | # of adjustments: %u", m_pGenlock->adjDelta, m_pGenlock->clockAdjustmentsMade);
			}
		}

		strText.AppendFormat(L"\nSync offset  : Average %3.1f ms [%.1f ms, %.1f ms]   Target %3.1f ms", m_pGenlock->syncOffsetAvg, m_pGenlock->minSyncOffset, m_pGenlock->maxSyncOffset, m_ExtraSets.dTargetSyncOffset);

		strText.AppendFormat(L"\nSync status  : glitches %u,  display-frame cycle mismatch: %7.3f %%,  dropped frames %u", m_uSyncGlitches, 100 * m_dCycleDifference, m_pcFramesDropped);

		if (m_ExtraSets.iDisplayStats == 1) {
			if (m_pAudioStats && m_ExtraSets.iSynchronizeMode == SYNCHRONIZE_VIDEO) {
				strText.AppendFormat(L"\nAudio lag   : %3d ms [%d ms, %d ms] | %s", m_lAudioLag, m_lAudioLagMin, m_lAudioLagMax, (m_lAudioSlaveMode == 4) ? L"Audio renderer is matching rate (for analog sound output)" : L"Audio renderer is not matching rate");
			}

			strText.AppendFormat(L"\nSample time  : waiting %3d ms", m_lNextSampleWait);
			if (m_ExtraSets.iSynchronizeMode == SYNCHRONIZE_NEAREST) {
				strText.AppendFormat(L"  paint time correction: %3d ms  Hysteresis: %d", m_lShiftToNearest, (int)(m_llHysteresis / 10000));
			}

			strText.AppendFormat(L"\nBuffering    : Buffered %3d    Free %3u    Current Surface %3u", m_nUsedBuffer, m_nSurfaces - m_nUsedBuffer, m_iCurSurface);

			strText.Append(L"\nSettings     : ");
			if (m_bIsFullscreen) {
				strText += L"D3DFS ";
			}
			if (!m_bCompositionEnabled) {
				strText += L"DisDC ";
			}
			if (m_ExtraSets.iSynchronizeMode == SYNCHRONIZE_VIDEO) {
				strText += L"SyncVideo ";
			} else if (m_ExtraSets.iSynchronizeMode == SYNCHRONIZE_NEAREST) {
				strText += L"SyncNearest ";
			}
			if (m_b10BitOutput) {
				strText += L"10 bit ";
			}
			if (m_ExtraSets.iEVROutputRange == 0) {
				strText += L"0-255 ";
			} else if (m_ExtraSets.iEVROutputRange == 1) {
				strText += L"16-235 ";
			}

			strText.AppendFormat(L"\nDecoder info : %s", DXVAState::GetDescription());

			strText.Append(L"\n");
			strText.Append(m_strMixerOutputFmt);
			if (m_strMsgError.GetLength()) {
				strText.Append(L"\n");
				strText.Append(m_strMsgError);
			}
		}

		if (m_ExtraSets.iDisplayStats < 3 && !m_pfD3DXCreateLine) {
			strText.Append(L"\nERROR: The file d3dx9_43.dll is not loaded, so the graph will not be drawn.");
		}

		m_Font3D.Draw2DText(22, 22, D3DCOLOR_XRGB(0, 0, 0), strText);
		m_Font3D.Draw2DText(20, 20, D3DCOLOR_XRGB(255, 204, 0), strText);
	}

	if (m_ExtraSets.iDisplayStats < 3 && m_pLine) {
		D3DXVECTOR2 Points[NB_JITTER];
		int nIndex;

		int DrawWidth = 625;
		int DrawHeight = 250;
		int StartX = m_windowRect.Width() - (DrawWidth + 20);
		int StartY = m_windowRect.Height() - (DrawHeight + 20);
		const D3DCOLOR rectColor = D3DCOLOR_ARGB(80, 0, 0, 0);

		DrawRect(rectColor, CRect(StartX, StartY, StartX + DrawWidth, StartY + DrawHeight));
		m_pLine->SetWidth(2.5);
		m_pLine->SetAntialias(1);
		m_pLine->Begin();

		for (int i = 0; i <= DrawHeight; i += 5) {
			Points[0].x = (FLOAT)StartX;
			Points[0].y = (FLOAT)(StartY + i);
			Points[1].x = (FLOAT)(StartX + (((i + 25) % 25) ? 50 : 625));
			Points[1].y = (FLOAT)(StartY + i);
			m_pLine->Draw(Points, 2, D3DCOLOR_XRGB(100, 100, 255));
		}

		for (int i = 0; i < DrawWidth; i += 125) { // Every 25:th sample
			Points[0].x = (FLOAT)(StartX + i);
			Points[0].y = (FLOAT)(StartY + DrawHeight / 2);
			Points[1].x = (FLOAT)(StartX + i);
			Points[1].y = (FLOAT)(StartY + DrawHeight / 2 + 10);
			m_pLine->Draw(Points, 2, D3DCOLOR_XRGB(100, 100, 255));
		}

		for (int i = 0; i < NB_JITTER; i++) {
			nIndex = (m_nNextJitter + 1 + i) % NB_JITTER;
			if (nIndex < 0) {
				nIndex += NB_JITTER;
			}
			double Jitter = m_llJitters[nIndex] - m_fJitterMean;
			Points[i].x  = (FLOAT)(StartX + (i * 5));
			Points[i].y  = (FLOAT)(StartY + (Jitter / 2000.0 + 125.0));
		}
		m_pLine->Draw(Points, NB_JITTER, D3DCOLOR_XRGB(255, 100, 100));

		if (m_ExtraSets.iDisplayStats == 1) { // Full on-screen statistics
			for (int i = 0; i < NB_JITTER; i++) {
				nIndex = (m_nNextSyncOffset + 1 + i) % NB_JITTER;
				if (nIndex < 0) {
					nIndex += NB_JITTER;
				}
				Points[i].x  = (FLOAT)(StartX + (i * 5));
				Points[i].y  = (FLOAT)(StartY + ((m_llSyncOffsets[nIndex]) / 2000 + 125));
			}
			m_pLine->Draw(Points, NB_JITTER, D3DCOLOR_XRGB(100, 200, 100));
		}
		m_pLine->End();
	}
}

double CBaseAP::GetRefreshRate()
{
	return m_dRefreshRate;
}

double CBaseAP::GetDisplayCycle()
{
	return (double)m_dD3DRefreshCycle;
}

double CBaseAP::GetCycleDifference()
{
	double dBaseDisplayCycle = GetDisplayCycle();
	UINT i;
	double minDiff = 1.0;
	if (dBaseDisplayCycle == 0.0 || m_dFrameCycle == 0.0) {
		return 1.0;
	} else {
		for (i = 1; i <= 8; i++) { // Try a lot of multiples of the display frequency
			double dDisplayCycle = i * dBaseDisplayCycle;
			double diff = (dDisplayCycle - m_dFrameCycle) / m_dFrameCycle;
			if (abs(diff) < abs(minDiff)) {
				minDiff = diff;
				m_dOptimumDisplayCycle = dDisplayCycle;
			}
		}
	}
	return minDiff;
}

void CBaseAP::EstimateRefreshTimings()
{
	if (m_pDevice9Ex) {
		D3DRASTER_STATUS rasterStatus;
		m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
		while (rasterStatus.ScanLine != 0) {
			m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
		}
		while (rasterStatus.ScanLine == 0) {
			m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
		}
		m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
		LONGLONG startTime = GetPerfCounter();
		UINT startLine = rasterStatus.ScanLine;
		LONGLONG endTime = 0;
		LONGLONG time = 0;
		UINT endLine = 0;
		UINT line = 0;
		bool done = false;
		while (!done) { // Estimate time for one scan line
			m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
			line = rasterStatus.ScanLine;
			time = GetPerfCounter();
			if (line > 0) {
				endLine = line;
				endTime = time;
			} else {
				done = true;
			}
		}
		m_dDetectedScanlineTime = (endTime - startTime) /((endLine - startLine) * 10000.0);

		// Estimate the display refresh rate from the vsyncs
		m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
		while (rasterStatus.ScanLine != 0) {
			m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
		}
		// Now we're at the start of a vsync
		startTime = GetPerfCounter();
		UINT i;
		for (i = 1; i <= 50; i++) {
			m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
			while (rasterStatus.ScanLine == 0) {
				m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
			}
			while (rasterStatus.ScanLine != 0) {
				m_pDevice9Ex->GetRasterStatus(0, &rasterStatus);
			}
			// Now we're at the next vsync
		}
		endTime = GetPerfCounter();
		m_dEstRefreshCycle = (endTime - startTime) / ((i - 1) * 10000.0);
	}
}

STDMETHODIMP CBaseAP::GetDIB(BYTE* lpDib, DWORD* size)
{
	CheckPointer(size, E_POINTER);

	HRESULT hr;

	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	m_pVideoSurfaces[m_iCurSurface]->GetDesc(&desc);

	DWORD required = sizeof(BITMAPINFOHEADER) + (desc.Width * desc.Height * 4);
	if (!lpDib) {
		*size = required;
		return S_OK;
	}
	if (*size < required) {
		return E_OUTOFMEMORY;
	}
	*size = required;

	CComPtr<IDirect3DSurface9> pSurface = m_pVideoSurfaces[m_iCurSurface];

	D3DLOCKED_RECT r;
	hr = pSurface->LockRect(&r, nullptr, D3DLOCK_READONLY);
	if (FAILED(hr)) {
		pSurface.Release();
		hr = m_pDevice9Ex->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pSurface, nullptr);
		if (SUCCEEDED(hr)) {
			hr = m_pDevice9Ex->GetRenderTargetData(m_pVideoSurfaces[m_iCurSurface], pSurface);
			if (SUCCEEDED(hr)) {
				hr = pSurface->LockRect(&r, nullptr, D3DLOCK_READONLY);
			}
		}
		if (FAILED(hr)) {
			return hr;
		}
	}

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)lpDib;
	memset(bih, 0, sizeof(BITMAPINFOHEADER));
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = desc.Width;
	bih->biHeight = desc.Height;
	bih->biBitCount = 32;
	bih->biPlanes = 1;
	bih->biSizeImage = DIBSIZE(*bih);

	std::unique_ptr<uint32_t[]> tmp_frame;
	if (m_iRotation) {
		tmp_frame.reset(new(std::nothrow) uint32_t[bih->biWidth * bih->biHeight]);
	}

	RetrieveBitmapData(desc.Width, desc.Height, 32, tmp_frame ? (BYTE*)tmp_frame.get() : (BYTE*)(bih + 1), (BYTE*)r.pBits, r.Pitch);

	pSurface->UnlockRect();

	if (tmp_frame) {
		int w = bih->biWidth;
		int h = bih->biHeight;
		uint32_t* src = tmp_frame.get();
		uint32_t* out = (uint32_t*)(bih + 1);

		switch (m_iRotation) {
		case 90:
			for (int x = w-1; x >= 0; x--) {
				for (int y = 0; y < h; y++) {
					*out++ = src[x + w*y];
				}
			}
			std::swap(bih->biWidth, bih->biHeight);
			break;
		case 180:
			for (int i = w*h - 1; i >= 0; i--) {
				*out++ = src[i];
			}
			break;
		case 270:
			for (int x = 0; x < w; x++) {
				for (int y = h-1; y >= 0; y--) {
					*out++ = src[x + w*y];
				}
			}
			std::swap(bih->biWidth, bih->biHeight);
			break;
		}
	}

	return S_OK;
}

STDMETHODIMP CBaseAP::GetDisplayedImage(LPVOID* dibImage)
{
	CheckPointer(m_pDevice9Ex, E_ABORT);

	// use the back buffer, because the display profile is applied to the front buffer
	CComPtr<IDirect3DSurface9> pBackBuffer;
	HRESULT hr = m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	const UINT width  = m_windowRect.Width();
	const UINT height = m_windowRect.Height();
	CComPtr<IDirect3DSurface9> pDestSurface;
	hr = m_pDevice9Ex->CreateRenderTarget(width, height, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pDestSurface, nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_pDevice9Ex->StretchRect(pBackBuffer, m_windowRect, pDestSurface, nullptr, D3DTEXF_NONE);
	if (FAILED(hr)) {
		return hr;
	}

	const UINT len = width * height * 4 + sizeof(BITMAPINFOHEADER);
	BYTE* p = (BYTE*)LocalAlloc(LMEM_FIXED, len); // only this allocator can be used
	if (!p) {
		return E_OUTOFMEMORY;
	}

	BITMAPINFOHEADER* pBIH = (BITMAPINFOHEADER*)p;
	ZeroMemory(pBIH, sizeof(BITMAPINFOHEADER));
	pBIH->biSize      = sizeof(BITMAPINFOHEADER);
	pBIH->biWidth     = width;
	pBIH->biHeight    = height;
	pBIH->biBitCount  = 32;
	pBIH->biPlanes    = 1;
	pBIH->biSizeImage = DIBSIZE(*pBIH);

	D3DLOCKED_RECT r;
	hr = pDestSurface->LockRect(&r, nullptr, D3DLOCK_READONLY);
	if (S_OK == hr) {
		RetrieveBitmapData(width, height, 32, (BYTE*)(pBIH + 1), (BYTE*)r.pBits, r.Pitch);
		hr = pDestSurface->UnlockRect();
	}

	if (FAILED(hr)) {
		LocalFree(p);
		return hr;
	}

	*dibImage = p;

	return S_OK;
}

STDMETHODIMP CBaseAP::ClearPixelShaders(int target)
{
	CAutoLock cRenderLock(&m_allocatorLock);

	if (target == TARGET_FRAME) {
		m_pPixelShaders.clear();
	} else if (target == TARGET_SCREEN) {
		m_pPixelShadersScreenSpace.clear();
	} else {
		return E_INVALIDARG;
	}
	m_pDevice9Ex->SetPixelShader(nullptr);

	return S_OK;
}

STDMETHODIMP CBaseAP::AddPixelShader(int target, LPCWSTR name, LPCSTR profile, LPCSTR sourceCode)
{
	CAutoLock cRenderLock(&m_allocatorLock);

	std::list<CExternalPixelShader> *pPixelShaders;
	if (target == TARGET_FRAME) {
		pPixelShaders = &m_pPixelShaders;
	} else if (target == TARGET_SCREEN) {
		pPixelShaders = &m_pPixelShadersScreenSpace;
	} else {
		return E_INVALIDARG;
	}

	if (!sourceCode || !profile) {
		return E_INVALIDARG;
	}

	CExternalPixelShader Shader;
	Shader.m_SourceCode = sourceCode;
	Shader.m_Profile = profile;

	CComPtr<IDirect3DPixelShader9> pPixelShader;

	HRESULT hr = Shader.Compile(m_pPSC.get());
	if (FAILED(hr)) {
		return hr;
	}

	pPixelShaders->push_back(Shader);
	Paint(true);
	return S_OK;
}

// CSyncAP

CSyncAP::CSyncAP(HWND hWnd, bool bFullscreen, HRESULT& hr, CString &_Error)
	: CBaseAP(hWnd, bFullscreen, hr, _Error)
{
	DLog(L"CSyncAP::CSyncAP()");

	if (FAILED (hr)) {
		_Error += L"SyncAP failed\n";
		return;
	}

	// Load EVR specifics DLLs
	m_hDxva2Lib = LoadLibraryW(L"dxva2.dll");
	if (m_hDxva2Lib) {
		pfDXVA2CreateDirect3DDeviceManager9 = (PTR_DXVA2CreateDirect3DDeviceManager9)GetProcAddress(m_hDxva2Lib, "DXVA2CreateDirect3DDeviceManager9");
	}

	// Load EVR functions
	m_hEvrLib = LoadLibraryW(L"evr.dll");
	if (m_hEvrLib) {
		pfMFCreateVideoSampleFromSurface = (PTR_MFCreateVideoSampleFromSurface)GetProcAddress(m_hEvrLib, "MFCreateVideoSampleFromSurface");
		pfMFCreateVideoMediaType = (PTR_MFCreateVideoMediaType)GetProcAddress(m_hEvrLib, "MFCreateVideoMediaType");
	}

	if (!pfDXVA2CreateDirect3DDeviceManager9 || !pfMFCreateVideoSampleFromSurface || !pfMFCreateVideoMediaType) {
		if (!pfDXVA2CreateDirect3DDeviceManager9) {
			_Error += L"Could not find DXVA2CreateDirect3DDeviceManager9 (dxva2.dll)\n";
		}
		if (!pfMFCreateVideoSampleFromSurface) {
			_Error += L"Could not find MFCreateVideoSampleFromSurface (evr.dll)\n";
		}
		if (!pfMFCreateVideoMediaType) {
			_Error += L"Could not find MFCreateVideoMediaType (Mfplat.dll)\n";
		}
		hr = E_FAIL;
		return;
	}

	// Load Vista specific DLLs
	m_hAvrtLib = LoadLibraryW(L"avrt.dll");
	if (m_hAvrtLib) {
		pfAvSetMmThreadCharacteristicsW   = (PTR_AvSetMmThreadCharacteristicsW)GetProcAddress(m_hAvrtLib, "AvSetMmThreadCharacteristicsW");
		pfAvSetMmThreadPriority           = (PTR_AvSetMmThreadPriority)GetProcAddress(m_hAvrtLib, "AvSetMmThreadPriority");
		pfAvRevertMmThreadCharacteristics = (PTR_AvRevertMmThreadCharacteristics)GetProcAddress(m_hAvrtLib, "AvRevertMmThreadCharacteristics");
	}
}

CSyncAP::~CSyncAP(void)
{
	DLog(L"CSyncAP::~CSyncAP()");

	StopWorkerThreads();
	m_pMediaType.Release();
	m_pClock.Release();
	m_pD3DManager.Release();

	if (m_hAvrtLib) {
		FreeLibrary(m_hAvrtLib);
	}
	if (m_hEvrLib) {
		FreeLibrary(m_hEvrLib);
	}
	if (m_hDxva2Lib) {
		FreeLibrary(m_hDxva2Lib);
	}
}

HRESULT CSyncAP::CheckShutdown() const
{
	if (m_nRenderState == Shutdown) {
		return MF_E_SHUTDOWN;
	} else {
		return S_OK;
	}
}

void CSyncAP::StartWorkerThreads()
{
	DWORD dwThreadId;
	if (m_nRenderState == Shutdown) {
		m_hEvtQuit  = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		m_hEvtFlush = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		m_hEvtSkip  = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		m_hMixerThread = ::CreateThread(nullptr, 0, MixerThreadStatic, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hMixerThread, THREAD_PRIORITY_HIGHEST);
		m_hRenderThread = ::CreateThread(nullptr, 0, RenderThreadStatic, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hRenderThread, THREAD_PRIORITY_TIME_CRITICAL);
		m_nRenderState = Stopped;
	}
}

void CSyncAP::StopWorkerThreads()
{
	if (m_nRenderState != Shutdown) {
		SetEvent(m_hEvtFlush);
		m_bEvtFlush = true;
		SetEvent(m_hEvtQuit);
		m_bEvtQuit = true;
		SetEvent(m_hEvtSkip);
		m_bEvtSkip = true;
		if (m_hRenderThread && WaitForSingleObject (m_hRenderThread, 10000) == WAIT_TIMEOUT) {
			ASSERT (FALSE);
			TerminateThread(m_hRenderThread, 0xDEAD);
		}
		SAFE_CLOSE_HANDLE(m_hRenderThread);

		if (m_hMixerThread && WaitForSingleObject (m_hMixerThread, 10000) == WAIT_TIMEOUT) {
			ASSERT(FALSE);
			TerminateThread(m_hMixerThread, 0xDEAD);
		}

		SAFE_CLOSE_HANDLE(m_hMixerThread);
		SAFE_CLOSE_HANDLE(m_hEvtFlush);
		SAFE_CLOSE_HANDLE(m_hEvtQuit);
		SAFE_CLOSE_HANDLE(m_hEvtSkip);

		m_bEvtFlush = false;
		m_bEvtQuit = false;
		m_bEvtSkip = false;
	}
	m_nRenderState = Shutdown;
}

STDMETHODIMP CSyncAP::CreateRenderer(IUnknown** ppRenderer)
{
	ASSERT(m_pD3D9Ex && m_pDevice9Ex == nullptr && m_pD3DManager == nullptr && m_pGenlock == nullptr);

	CheckPointer(ppRenderer, E_POINTER);
	*ppRenderer = nullptr;

	HRESULT hr = E_FAIL;
	CStringW _Error;

	// Init DXVA manager
	hr = pfDXVA2CreateDirect3DDeviceManager9(&m_nResetToken, &m_pD3DManager);
	if (FAILED(hr)) {
		_Error = L"DXVA2CreateDirect3DDeviceManager9 failed\n";
		DLog(_Error);
		return hr;
	}

	m_pGenlock = DNew CGenlock(
		m_ExtraSets.dTargetSyncOffset, m_ExtraSets.dControlLimit,
		m_ExtraSets.dCycleDelta, 0); // Must be done before CreateDXDevice

	hr = CreateDXDevice(_Error);
	if (FAILED(hr)) {
		DLog(_Error);
		return hr;
	}

	// Define the shader profile.
	DLogIf(m_Caps.PixelShaderVersion < D3DPS_VERSION(3, 0), L"WARNING: Pixel shaders 3.0 not supported!");

	hr = m_pD3DManager->ResetDevice(m_pDevice9Ex, m_nResetToken);
	if (FAILED(hr)) {
		_Error = L"m_pD3DManager->ResetDevice failed\n";
		DLog(_Error);
		return hr;
	}

	do {
		CMacrovisionKicker* pMK = DNew CMacrovisionKicker(L"CMacrovisionKicker", nullptr);
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;

		CSyncRenderer *pOuterEVR = DNew CSyncRenderer(L"CSyncRenderer", pUnk, hr, this);
		m_pOuterEVR = pOuterEVR;

		pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)pOuterEVR);
		CComQIPtr<IBaseFilter> pBF(pUnk);

		if (FAILED(hr)) {
			break;
		}

		// Set EVR custom presenter
		CComPtr<IMFVideoPresenter>	pVP;
		CComPtr<IMFVideoRenderer>	pMFVR;
		CComQIPtr<IMFGetService>	pMFGS(pBF);
		CComQIPtr<IEVRFilterConfig> pConfig(pBF);

		hr = pMFGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pMFVR));
		if (SUCCEEDED(hr)) {
			hr = QueryInterface(IID_PPV_ARGS(&pVP));
		}
		if (SUCCEEDED(hr)) {
			hr = pMFVR->InitializeRenderer(nullptr, pVP);
		}

		if (m_bEnableSubPic) {
			m_bUseInternalTimer = HookNewSegmentAndReceive(GetFirstPin(pBF));
		}

		if (FAILED(hr)) {
			*ppRenderer = nullptr;
		} else {
			*ppRenderer = pBF.Detach();
		}
	} while (0);

	return hr;
}

STDMETHODIMP_(CLSID) CSyncAP::GetAPCLSID()
{
	return CLSID_SyncAllocatorPresenter;
}

STDMETHODIMP_(bool) CSyncAP::Paint(bool fAll)
{
	return __super::Paint(fAll);
}

STDMETHODIMP CSyncAP::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	HRESULT hr;

	if (riid == __uuidof(IMFClockStateSink)) {
		hr = GetInterface((IMFClockStateSink*)this, ppv);
	} else if (riid == __uuidof(IMFVideoPresenter)) {
		hr = GetInterface((IMFVideoPresenter*)this, ppv);
	} else if (riid == __uuidof(IMFTopologyServiceLookupClient)) {
		hr = GetInterface((IMFTopologyServiceLookupClient*)this, ppv);
	} else if (riid == __uuidof(IMFVideoDeviceID)) {
		hr = GetInterface((IMFVideoDeviceID*)this, ppv);
	} else if (riid == __uuidof(IMFGetService)) {
		hr = GetInterface((IMFGetService*)this, ppv);
	} else if (riid == __uuidof(IMFAsyncCallback)) {
		hr = GetInterface((IMFAsyncCallback*)this, ppv);
	} else if (riid == __uuidof(IMFVideoDisplayControl)) {
		hr = GetInterface((IMFVideoDisplayControl*)this, ppv);
	} else if (riid == __uuidof(IMFVideoMixerBitmap)) {
		hr = GetInterface((IMFVideoMixerBitmap*)this, ppv);
	} else if (riid == __uuidof(IEVRTrustedVideoPlugin)) {
		hr = GetInterface((IEVRTrustedVideoPlugin*)this, ppv);
	} else if (riid == IID_IQualProp) {
		hr = GetInterface((IQualProp*)this, ppv);
	} else if (riid == __uuidof(IMFRateSupport)) {
		hr = GetInterface((IMFRateSupport*)this, ppv);
	} else if (riid == __uuidof(IDirect3DDeviceManager9)) {
		hr = m_pD3DManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppv);
	} else if (riid == __uuidof(ISyncClockAdviser)) {
		hr = GetInterface((ISyncClockAdviser*)this, ppv);
	//} else if (riid == __uuidof(ID3DFullscreenControl)) {
	//	hr = GetInterface((ID3DFullscreenControl*)this, ppv);
	} else if (riid == __uuidof(IPlaybackNotify)) {
		hr = GetInterface((IPlaybackNotify*)this, ppv);
	} else {
		hr = __super::NonDelegatingQueryInterface(riid, ppv);
	}

	return hr;
}

// IMFClockStateSink
STDMETHODIMP CSyncAP::OnClockStart(MFTIME hnsSystemTime,  LONGLONG llClockStartOffset)
{
	m_nRenderState = Started;
	return S_OK;
}

STDMETHODIMP CSyncAP::OnClockStop(MFTIME hnsSystemTime)
{
	m_nRenderState = Stopped;
	return S_OK;
}

STDMETHODIMP CSyncAP::OnClockPause(MFTIME hnsSystemTime)
{
	m_nRenderState = Paused;
	return S_OK;
}

STDMETHODIMP CSyncAP::OnClockRestart(MFTIME hnsSystemTime)
{
	m_nRenderState = Started;
	return S_OK;
}

STDMETHODIMP CSyncAP::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
	return E_NOTIMPL;
}

// IBaseFilter delegate
bool CSyncAP::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue)
{
	CAutoLock lock(&m_SampleQueueLock);
	switch (m_nRenderState) {
		case Started:
			*State = State_Running;
			break;
		case Paused:
			*State = State_Paused;
			break;
		case Stopped:
			*State = State_Stopped;
			break;
		default:
			*State = State_Stopped;
			_ReturnValue = E_FAIL;
	}
	_ReturnValue = S_OK;
	return true;
}

// IQualProp
STDMETHODIMP CSyncAP::get_FramesDroppedInRenderer(int *pcFrames)
{
	*pcFrames = m_pcFramesDropped;
	return S_OK;
}

STDMETHODIMP CSyncAP::get_FramesDrawn(int *pcFramesDrawn)
{
	*pcFramesDrawn = m_pcFramesDrawn;
	return S_OK;
}

STDMETHODIMP CSyncAP::get_AvgFrameRate(int *piAvgFrameRate)
{
	*piAvgFrameRate = (int)(m_fAvrFps * 100);
	return S_OK;
}

STDMETHODIMP CSyncAP::get_Jitter(int *iJitter)
{
	*iJitter = (int)((m_fJitterStdDev/10000.0) + 0.5);
	return S_OK;
}

STDMETHODIMP CSyncAP::get_AvgSyncOffset(int *piAvg)
{
	*piAvg = (int)((m_fSyncOffsetAvr/10000.0) + 0.5);
	return S_OK;
}

STDMETHODIMP CSyncAP::get_DevSyncOffset(int *piDev)
{
	*piDev = (int)((m_fSyncOffsetStdDev/10000.0) + 0.5);
	return S_OK;
}

// IMFRateSupport
STDMETHODIMP CSyncAP::GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate)
{
	*pflRate = 0;
	return S_OK;
}

STDMETHODIMP CSyncAP::GetFastestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate)
{
	HRESULT hr = S_OK;
	float   fMaxRate = 0.0f;

	CAutoLock lock(this);

	CheckPointer(pflRate, E_POINTER);
	CHECK_HR(CheckShutdown());

	// Get the maximum forward rate.
	fMaxRate = GetMaxRate(fThin);

	// For reverse playback, swap the sign.
	if (eDirection == MFRATE_REVERSE) {
		fMaxRate = -fMaxRate;
	}

	*pflRate = fMaxRate;
	return hr;
}

STDMETHODIMP CSyncAP::IsRateSupported(BOOL fThin, float flRate, float *pflNearestSupportedRate)
{
	// fRate can be negative for reverse playback.
	// pfNearestSupportedRate can be nullptr.
	CAutoLock lock(this);
	HRESULT hr = S_OK;

	CheckPointer (pflNearestSupportedRate, E_POINTER);
	CHECK_HR(CheckShutdown());

	// Find the maximum forward rate.
	const float fMaxRate = GetMaxRate(fThin);
	float fNearestRate = flRate; // Default.

	if (fabsf(flRate) > fMaxRate) {
		// The (absolute) requested rate exceeds the maximum rate.
		hr = MF_E_UNSUPPORTED_RATE;

		// The nearest supported rate is fMaxRate.
		fNearestRate = fMaxRate;
		if (flRate < 0) {
			// For reverse playback, swap the sign.
			fNearestRate = -fNearestRate;
		}
	}

	// Return the nearest supported rate if the caller requested it.
	*pflNearestSupportedRate = fNearestRate;

	return hr;
}

float CSyncAP::GetMaxRate(BOOL bThin)
{
	float fMaxRate = FLT_MAX;  // Default.
	UINT32 fpsNumerator = 0, fpsDenominator = 0;
	UINT MonitorRateHz = 0;

	if (!bThin && (m_pMediaType != nullptr)) {
		// Non-thinned: Use the frame rate and monitor refresh rate.

		// Frame rate:
		MFGetAttributeRatio(m_pMediaType, MF_MT_FRAME_RATE,
							&fpsNumerator, &fpsDenominator);

		// Monitor refresh rate:
		MonitorRateHz = m_refreshRate; // D3DDISPLAYMODE

		if (fpsDenominator && fpsNumerator && MonitorRateHz) {
			// Max Rate = Refresh Rate / Frame Rate
			fMaxRate = (float)MulDiv(MonitorRateHz, fpsDenominator, fpsNumerator);
		}
	}
	return fMaxRate;
}

void CSyncAP::CompleteFrameStep(bool bCancel)
{
	if (m_nStepCount > 0) {
		if (bCancel || (m_nStepCount == 1)) {
			m_pSink->Notify(EC_STEP_COMPLETE, bCancel ? TRUE : FALSE, 0);
			m_nStepCount = 0;
		} else {
			m_nStepCount--;
		}
	}
}

// IMFVideoPresenter
STDMETHODIMP CSyncAP::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
	HRESULT hr = S_OK;

	switch (eMessage) {
		case MFVP_MESSAGE_BEGINSTREAMING:
			hr = BeginStreaming();
			m_llHysteresis = 0;
			m_lShiftToNearest = 0;
			m_bStepping = false;
			break;

		case MFVP_MESSAGE_CANCELSTEP:
			m_bStepping = false;
			CompleteFrameStep(true);
			break;

		case MFVP_MESSAGE_ENDOFSTREAM:
			m_bPendingMediaFinished = true;
			break;

		case MFVP_MESSAGE_ENDSTREAMING:
			m_pRefClock.Release();
			break;

		case MFVP_MESSAGE_FLUSH:
			SetEvent(m_hEvtFlush);
			m_bEvtFlush = true;
			while (WaitForSingleObject(m_hEvtFlush, 1) == WAIT_OBJECT_0) {
				;
			}
			break;

		case MFVP_MESSAGE_INVALIDATEMEDIATYPE:
			m_bPendingRenegotiate = true;
			while (m_bPendingRenegotiate) {
				Sleep(1);
			}
			break;

		case MFVP_MESSAGE_PROCESSINPUTNOTIFY:
			break;

		case MFVP_MESSAGE_STEP:
			m_nStepCount = (int)ulParam;
			m_bStepping = true;
			break;

		default :
			ASSERT(FALSE);
			break;
	}
	return hr;
}

HRESULT CSyncAP::IsMediaTypeSupported(IMFMediaType* pMixerType)
{
	HRESULT hr;
	AM_MEDIA_TYPE* pAMMedia;
	UINT nInterlaceMode;

	CHECK_HR(pMixerType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pAMMedia));
	CHECK_HR(pMixerType->GetUINT32 (MF_MT_INTERLACE_MODE, &nInterlaceMode));

	if ( (pAMMedia->majortype != MEDIATYPE_Video)) {
		hr = MF_E_INVALIDMEDIATYPE;
	}
	pMixerType->FreeRepresentation(FORMAT_VideoInfo2, (void*)pAMMedia);
	return hr;
}

HRESULT CSyncAP::CreateProposedOutputType(IMFMediaType* pMixerType, IMFMediaType** pType)
{
	HRESULT        hr;
	AM_MEDIA_TYPE* pAMMedia = nullptr;
	MFVIDEOFORMAT* VideoFormat;

	CHECK_HR(pMixerType->GetRepresentation(FORMAT_MFVideoFormat, (void**)&pAMMedia));

	VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;
	CHECK_HR(pfMFCreateVideoMediaType(VideoFormat, &m_pMediaType));

	m_pMediaType->SetUINT32(MF_MT_PAN_SCAN_ENABLED, 0);

	int iOutputRange = m_ExtraSets.iEVROutputRange;
	if (m_inputExtFormat.NominalRange == MFNominalRange_0_255) {
		m_pMediaType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, iOutputRange == 1 ? MFNominalRange_48_208 : MFNominalRange_16_235); // fix EVR bug
	} else {
		m_pMediaType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, iOutputRange == 1 ? MFNominalRange_16_235 : MFNominalRange_0_255);
	}
	m_LastSetOutputRange = iOutputRange;

	ULARGE_INTEGER ui64FrameSize;
	m_pMediaType->GetUINT64(MF_MT_FRAME_SIZE, &ui64FrameSize.QuadPart);

	CSize videoSize((LONG)ui64FrameSize.HighPart, (LONG)ui64FrameSize.LowPart);
	MFVideoArea Area = GetArea(0, 0, videoSize.cx, videoSize.cy);
	m_pMediaType->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&Area, sizeof(MFVideoArea));

	ULARGE_INTEGER ui64AspectRatio;
	m_pMediaType->GetUINT64(MF_MT_PIXEL_ASPECT_RATIO, &ui64AspectRatio.QuadPart);

	__int64 darx = (__int64)ui64AspectRatio.HighPart * videoSize.cx;
	__int64 dary = (__int64)ui64AspectRatio.LowPart * videoSize.cy;
	ReduceDim(darx, dary);
	CSize aspectRatio((LONG)darx, (LONG)dary);

	if (videoSize != m_nativeVideoSize || aspectRatio != m_aspectRatio) {
		m_nativeVideoSize = videoSize;
		m_aspectRatio = aspectRatio;

		// Notify the graph about the change
		if (m_pSink) {
			m_pSink->Notify(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(m_nativeVideoSize.cx, m_nativeVideoSize.cy), 0);
		}
	}

	pMixerType->FreeRepresentation(FORMAT_MFVideoFormat, (LPVOID)pAMMedia);
	m_pMediaType->QueryInterface(IID_PPV_ARGS(pType));

	return hr;
}

HRESULT CSyncAP::SetMediaType(IMFMediaType* pType)
{
	HRESULT hr;
	AM_MEDIA_TYPE* pAMMedia = nullptr;

	CheckPointer(pType, E_POINTER);
	CHECK_HR(pType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pAMMedia));

	hr = InitializeDevice(pAMMedia);

	pType->FreeRepresentation(FORMAT_VideoInfo2, (void*)pAMMedia);

	return hr;
}

int CSyncAP::GetOutputMediaTypeMerit(IMFMediaType *pMediaType)
{
	AM_MEDIA_TYPE *pAMMedia = nullptr;
	MFVIDEOFORMAT *VideoFormat;

	HRESULT hr;
	if (FAILED(hr = pMediaType->GetRepresentation(FORMAT_MFVideoFormat, (void**)&pAMMedia))) {
		return 0;
	};
	VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;

	int merit = 0;
	switch (VideoFormat->surfaceInfo.Format) {
		case D3DFMT_X8R8G8B8:
			merit = 80;
			break;
		case D3DFMT_A2R10G10B10:
			merit = 70;
			break;
		case D3DFMT_A2B10G10R10: // not tested
			merit = 20;
			break;
		case D3DFMT_A8R8G8B8: // an accepted format, but fails on most surface types
		default: // we do not support YUV formats here.
			merit = 0;
			break;
	}
	pMediaType->FreeRepresentation(FORMAT_MFVideoFormat, (void*)pAMMedia);

	return merit;
}

HRESULT CSyncAP::RenegotiateMediaType()
{
	if (!m_pMixer) {
		return MF_E_INVALIDREQUEST;
	}

	// Get the mixer's input type
	CComPtr<IMFMediaType> pMixerInputType;
	HRESULT hr = m_pMixer->GetInputCurrentType(0, &pMixerInputType);
	if (SUCCEEDED(hr)) {
		AM_MEDIA_TYPE* pMT;
		hr = pMixerInputType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pMT);
		if (SUCCEEDED(hr)) {
			m_inputMediaType = *pMT;
			pMixerInputType->FreeRepresentation(FORMAT_VideoInfo2, pMT);

			m_inputExtFormat.value = 0;
			if (m_inputMediaType.formattype == FORMAT_VideoInfo2) {
				m_inputExtFormat.value = ((VIDEOINFOHEADER2*)m_inputMediaType.pbFormat)->dwControlFlags;
			}
		}
	}

	std::vector<CComPtr<IMFMediaType>> ValidMixerTypes;

	// Loop through all of the mixer's proposed output types.
	DWORD iTypeIndex = 0;
	while ((hr != MF_E_NO_MORE_TYPES)) {
		CComPtr<IMFMediaType> pType;
		CComPtr<IMFMediaType> pMixerType;
		m_pMediaType.Release();

		// Step 1. Get the next media type supported by mixer.
		hr = m_pMixer->GetOutputAvailableType(0, iTypeIndex++, &pMixerType);
		if (FAILED(hr)) {
			break;
		}
		// Step 2. Check if we support this media type.
		if (SUCCEEDED(hr)) {
			hr = IsMediaTypeSupported(pMixerType);
		}
		if (SUCCEEDED(hr)) {
			hr = CreateProposedOutputType(pMixerType, &pType);
		}
		// Step 4. Check if the mixer will accept this media type.
		if (SUCCEEDED(hr)) {
			hr = m_pMixer->SetOutputType(0, pType, MFT_SET_TYPE_TEST_ONLY);
		}
		if (SUCCEEDED(hr)) {
			int Merit = GetOutputMediaTypeMerit(pType);

			auto itInsertPos = ValidMixerTypes.cbegin();
			for (auto it = ValidMixerTypes.cbegin(); it != ValidMixerTypes.end(); ++it) {
				int ThisMerit = GetOutputMediaTypeMerit(*it);
				if (Merit > ThisMerit) {
					itInsertPos = it;
					break;
				} else {
					itInsertPos = it+1;
				}
			}
			ValidMixerTypes.insert(itInsertPos, pType);
		}
	}

	for (const auto& pType : ValidMixerTypes) {
		hr = SetMediaType(pType);
		if (SUCCEEDED(hr)) {
			hr = m_pMixer->SetOutputType(0, pType, 0);
			// If something went wrong, clear the media type.
			if (FAILED(hr)) {
				SetMediaType(nullptr);
			} else {
				break;
			}
		}
	}

	return hr;
}

bool CSyncAP::GetSampleFromMixer()
{
	MFT_OUTPUT_DATA_BUFFER Buffer;
	HRESULT hr = S_OK;
	DWORD dwStatus;
	LONGLONG llClockBefore = 0;
	LONGLONG llClockAfter  = 0;
	LONGLONG llMixerLatency;

	UINT32 iSurface;
	bool newSample = false;

	while (SUCCEEDED(hr)) { // Get as many frames as there are and that we have samples for
		CComPtr<IMFSample> pSample;
		CComPtr<IMFSample> pNewSample;
		if (FAILED(GetFreeSample(&pSample))) { // All samples are taken for the moment. Better luck next time
			break;
		}

		memset(&Buffer, 0, sizeof(Buffer));
		Buffer.pSample = pSample;
		pSample->GetUINT32(GUID_SURFACE_INDEX, &iSurface);
		{
			llClockBefore = GetPerfCounter();
			hr = m_pMixer->ProcessOutput(0 , 1, &Buffer, &dwStatus);
			llClockAfter = GetPerfCounter();
		}

		//if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) { // There are no samples left in the mixer // <-- old code
		if (FAILED(hr)) {
			MoveToFreeList(pSample, false);
			DLogIf(hr != MF_E_TRANSFORM_NEED_MORE_INPUT, L"EVR Sync: GetImageFromMixer failed with error %s", HR2Str(hr));
			break;
		}

		if (m_pSink) {
			llMixerLatency = llClockAfter - llClockBefore;
			m_pSink->Notify (EC_PROCESSING_LATENCY, (LONG_PTR)&llMixerLatency, 0);
		}

		newSample = true;

		if (m_ExtraSets.bTearingTest) {
			RECT rcTearing;

			rcTearing.left = m_nTearingPos;
			rcTearing.top = 0;
			rcTearing.right = rcTearing.left + 4;
			rcTearing.bottom = m_nativeVideoSize.cy;
			m_pDevice9Ex->ColorFill(m_pVideoSurfaces[iSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));

			rcTearing.left = (rcTearing.right + 15) % m_nativeVideoSize.cx;
			rcTearing.right = rcTearing.left + 4;
			m_pDevice9Ex->ColorFill(m_pVideoSurfaces[iSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));
			m_nTearingPos = (m_nTearingPos + 7) % m_nativeVideoSize.cx;
		}
		MoveToScheduledList(pSample, false); // Schedule, then go back to see if there is more where that came from
	}
	return newSample;
}

STDMETHODIMP CSyncAP::GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType)
{
	HRESULT hr = S_OK;
	CAutoLock lock(this);
	CheckPointer(ppMediaType, E_POINTER);
	CHECK_HR(CheckShutdown());

	if (m_pMediaType == nullptr) {
		CHECK_HR(MF_E_NOT_INITIALIZED);
	}

	CHECK_HR(m_pMediaType->QueryInterface(__uuidof(IMFVideoMediaType), (void**)&ppMediaType));
	return hr;
}

// IMFTopologyServiceLookupClient
STDMETHODIMP CSyncAP::InitServicePointers(__in IMFTopologyServiceLookup *pLookup)
{
	HRESULT hr;
	DWORD dwObjects = 1;
	hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_MIXER_SERVICE, __uuidof (IMFTransform), (void**)&m_pMixer, &dwObjects);
	hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE, __uuidof (IMediaEventSink ), (void**)&m_pSink, &dwObjects);
	hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE, __uuidof (IMFClock ), (void**)&m_pClock, &dwObjects);
	StartWorkerThreads();
	return S_OK;
}

STDMETHODIMP CSyncAP::ReleaseServicePointers()
{
	StopWorkerThreads();
	m_pMixer.Release();
	m_pSink.Release();
	m_pClock.Release();
	return S_OK;
}

// IMFVideoDeviceID
STDMETHODIMP CSyncAP::GetDeviceID( __out  IID *pDeviceID)
{
	CheckPointer(pDeviceID, E_POINTER);
	*pDeviceID = IID_IDirect3DDevice9;
	return S_OK;
}

// IMFGetService
STDMETHODIMP CSyncAP::GetService( __RPC__in REFGUID guidService, __RPC__in REFIID riid, __RPC__deref_out_opt LPVOID *ppvObject)
{
	if (guidService == MR_VIDEO_RENDER_SERVICE) {
		return NonDelegatingQueryInterface (riid, ppvObject);
	} else if (guidService == MR_VIDEO_ACCELERATION_SERVICE) {
		return m_pD3DManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppvObject);
	}

	return E_NOINTERFACE;
}

// IMFAsyncCallback
STDMETHODIMP CSyncAP::GetParameters( __RPC__out DWORD *pdwFlags, __RPC__out DWORD *pdwQueue)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSyncAP::Invoke( __RPC__in_opt IMFAsyncResult *pAsyncResult)
{
	return E_NOTIMPL;
}

// IMFVideoDisplayControl
STDMETHODIMP CSyncAP::GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo)
{
	if (pszVideo) {
		pszVideo->cx = m_nativeVideoSize.cx;
		pszVideo->cy = m_nativeVideoSize.cy;
	}
	if (pszARVideo) {
		pszARVideo->cx = m_nativeVideoSize.cx * m_aspectRatio.cx;
		pszARVideo->cy = m_nativeVideoSize.cy * m_aspectRatio.cy;
	}
	return S_OK;
}

STDMETHODIMP CSyncAP::GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax)
{
	if (pszMin) {
		pszMin->cx = 1;
		pszMin->cy = 1;
	}

	if (pszMax) {
		D3DDISPLAYMODE d3ddm;

		ZeroMemory(&d3ddm, sizeof(d3ddm));
		if (SUCCEEDED(m_pD3D9Ex->GetAdapterDisplayMode(D3D9Helper::GetAdapter(m_pD3D9Ex, m_hWnd), &d3ddm))) {
			pszMax->cx = d3ddm.Width;
			pszMax->cy = d3ddm.Height;
		}
	}
	return S_OK;
}

STDMETHODIMP CSyncAP::SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest)
{
	return S_OK;
}

STDMETHODIMP CSyncAP::GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest)
{
	if (pnrcSource) {
		pnrcSource->left	= 0.0;
		pnrcSource->top		= 0.0;
		pnrcSource->right	= 1.0;
		pnrcSource->bottom	= 1.0;
	}
	if (prcDest) {
		memcpy (prcDest, &m_videoRect, sizeof(m_videoRect));    //GetClientRect (m_hWnd, prcDest);
	}
	return S_OK;
}

STDMETHODIMP CSyncAP::SetAspectRatioMode(DWORD dwAspectRatioMode)
{
	m_dwVideoAspectRatioMode = (MFVideoAspectRatioMode)dwAspectRatioMode;
	return S_OK;
}

STDMETHODIMP CSyncAP::GetAspectRatioMode(DWORD *pdwAspectRatioMode)
{
	CheckPointer (pdwAspectRatioMode, E_POINTER);
	*pdwAspectRatioMode = m_dwVideoAspectRatioMode;
	return S_OK;
}

STDMETHODIMP CSyncAP::SetVideoWindow(HWND hwndVideo)
{
	if (m_hWnd != hwndVideo) {
		CAutoLock lock(this);
		CAutoLock lock2(&m_ImageProcessingLock);
		CAutoLock cRenderLock(&m_allocatorLock);

		m_hWnd = hwndVideo;
		m_bPendingResetDevice = true;
		SendResetRequest();
	}
	return S_OK;
}

STDMETHODIMP CSyncAP::GetVideoWindow(HWND *phwndVideo)
{
	CheckPointer(phwndVideo, E_POINTER);
	*phwndVideo = m_hWnd;
	return S_OK;
}

STDMETHODIMP CSyncAP::RepaintVideo()
{
	Paint(true);
	return S_OK;
}

STDMETHODIMP CSyncAP::GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp)
{
	if (!pBih || !pDib || !pcbDib) {
		return E_POINTER;
	}
	CheckPointer(m_pDevice9Ex, E_ABORT);

	HRESULT hr = S_OK;
	const unsigned width  = m_windowRect.Width();
	const unsigned height = m_windowRect.Height();
	const unsigned len = width * height * 4;

	memset(pBih, 0, sizeof(BITMAPINFOHEADER));
	pBih->biSize      = sizeof(BITMAPINFOHEADER);
	pBih->biWidth     = width;
	pBih->biHeight    = height;
	pBih->biBitCount  = 32;
	pBih->biPlanes    = 1;
	pBih->biSizeImage = DIBSIZE(*pBih);

	BYTE* p = (BYTE*)CoTaskMemAlloc(len); // only this allocator can be used
	if (!p) {
		return E_OUTOFMEMORY;
	}

	CComPtr<IDirect3DSurface9> pBackBuffer;
	CComPtr<IDirect3DSurface9> pDestSurface;
	D3DLOCKED_RECT r;
	if (FAILED(hr = m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer))
		|| FAILED(hr = m_pDevice9Ex->CreateRenderTarget(width, height, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pDestSurface, nullptr))
		|| (FAILED(hr = m_pDevice9Ex->StretchRect(pBackBuffer, m_windowRect, pDestSurface, nullptr, D3DTEXF_NONE)))
		|| (FAILED(hr = pDestSurface->LockRect(&r, nullptr, D3DLOCK_READONLY)))) {
		DLog(L"CSyncAP::GetCurrentImage filed : %s", GetWindowsErrorMessage(hr, m_hD3D9).GetString());
		CoTaskMemFree(p);
		return hr;
	}

	RetrieveBitmapData(width, height, 32, p, (BYTE*)r.pBits, r.Pitch);

	pDestSurface->UnlockRect();

	*pDib = p;
	*pcbDib = len;

	return S_OK;
}

STDMETHODIMP CSyncAP::SetBorderColor(COLORREF Clr)
{
	m_BorderColor = Clr;
	return S_OK;
}

STDMETHODIMP CSyncAP::GetBorderColor(COLORREF *pClr)
{
	CheckPointer (pClr, E_POINTER);
	*pClr = m_BorderColor;
	return S_OK;
}

STDMETHODIMP CSyncAP::SetRenderingPrefs(DWORD dwRenderFlags)
{
	m_dwVideoRenderPrefs = (MFVideoRenderPrefs)dwRenderFlags;
	return S_OK;
}

STDMETHODIMP CSyncAP::GetRenderingPrefs(DWORD *pdwRenderFlags)
{
	CheckPointer(pdwRenderFlags, E_POINTER);
	*pdwRenderFlags = m_dwVideoRenderPrefs;
	return S_OK;
}

STDMETHODIMP CSyncAP::SetFullscreen(BOOL fFullscreen)
{
	ASSERT (FALSE);
	return E_NOTIMPL;
}

STDMETHODIMP CSyncAP::GetFullscreen(BOOL *pfFullscreen)
{
	ASSERT (FALSE);
	return E_NOTIMPL;
}

// IMFVideoMixerBitmap
STDMETHODIMP CSyncAP::ClearAlphaBitmap()
{
	CAutoLock cRenderLock(&m_allocatorLock);
	m_bAlphaBitmapEnable = false;

	return S_OK;
}

STDMETHODIMP CSyncAP::GetAlphaBitmapParameters(MFVideoAlphaBitmapParams *pBmpParms)
{
	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock cRenderLock(&m_allocatorLock);

	if (m_bAlphaBitmapEnable && m_pAlphaBitmapTexture) {
		*pBmpParms = m_AlphaBitmapParams; // formal implementation, don't believe it
		return S_OK;
	} else {
		return MF_E_NOT_INITIALIZED;
	}
}

STDMETHODIMP CSyncAP::SetAlphaBitmap(const MFVideoAlphaBitmap *pBmpParms)
{
	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock cRenderLock(&m_allocatorLock);

	CheckPointer(m_pDevice9Ex, E_ABORT);
	HRESULT hr = S_OK;

	if (pBmpParms->GetBitmapFromDC && pBmpParms->bitmap.hdc) {
		HBITMAP hBitmap = (HBITMAP)GetCurrentObject(pBmpParms->bitmap.hdc, OBJ_BITMAP);
		if (!hBitmap) {
			return E_INVALIDARG;
		}
		DIBSECTION info = { 0 };
		if (!::GetObjectW(hBitmap, sizeof(DIBSECTION), &info)) {
			return E_INVALIDARG;
		}
		BITMAP& bm = info.dsBm;
		if (!bm.bmWidth || !bm.bmHeight || bm.bmBitsPixel != 32 || !bm.bmBits) {
			return E_INVALIDARG;
		}

		if (m_pAlphaBitmapTexture) {
			D3DSURFACE_DESC desc = {};
			m_pAlphaBitmapTexture->GetLevelDesc(0, &desc);
			if (bm.bmWidth != desc.Width || bm.bmHeight != desc.Height) {
				m_pAlphaBitmapTexture.Release();
			}
		}

		if (!m_pAlphaBitmapTexture) {
			hr = m_pDevice9Ex->CreateTexture(bm.bmWidth, bm.bmHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pAlphaBitmapTexture, nullptr);
		}

		if (SUCCEEDED(hr)) {
			CComPtr<IDirect3DSurface9> pSurface;
			hr = m_pAlphaBitmapTexture->GetSurfaceLevel(0, &pSurface);
			if (SUCCEEDED(hr)) {
				D3DLOCKED_RECT lr;
				hr = pSurface->LockRect(&lr, nullptr, D3DLOCK_DISCARD);
				if (S_OK == hr) {
					if (bm.bmWidthBytes == lr.Pitch) {
						memcpy(lr.pBits, bm.bmBits, bm.bmWidthBytes * bm.bmHeight);
					}
					else {
						LONG linesize = std::min(bm.bmWidthBytes, (LONG)lr.Pitch);
						BYTE* src = (BYTE*)bm.bmBits;
						BYTE* dst = (BYTE*)lr.pBits;
						for (LONG y = 0; y < bm.bmHeight; ++y) {
							memcpy(dst, src, linesize);
							src += bm.bmWidthBytes;
							dst += lr.Pitch;
						}
					}
					hr = pSurface->UnlockRect();
				}
			}
		}
	} else {
		return E_INVALIDARG;
	}

	m_bAlphaBitmapEnable = SUCCEEDED(hr) && m_pAlphaBitmapTexture;

	if (m_bAlphaBitmapEnable) {
		hr = UpdateAlphaBitmapParameters(&pBmpParms->params);
	}

	return hr;
}

STDMETHODIMP CSyncAP::UpdateAlphaBitmapParameters(const MFVideoAlphaBitmapParams *pBmpParms)
{
	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock cRenderLock(&m_allocatorLock);

	m_AlphaBitmapParams = *pBmpParms; // formal implementation, don't believe it

	return S_OK;
}

// IEVRTrustedVideoPlugin
STDMETHODIMP CSyncAP::IsInTrustedVideoMode(BOOL *pYes)
{
	CheckPointer(pYes, E_POINTER);
	*pYes = TRUE;
	return S_OK;
}

STDMETHODIMP CSyncAP::CanConstrict(BOOL *pYes)
{
	CheckPointer(pYes, E_POINTER);
	*pYes = TRUE;
	return S_OK;
}

STDMETHODIMP CSyncAP::SetConstriction(DWORD dwKPix)
{
	return S_OK;
}

STDMETHODIMP CSyncAP::DisableImageExport(BOOL bDisable)
{
	return S_OK;
}

// IDirect3DDeviceManager9
STDMETHODIMP CSyncAP::ResetDevice(IDirect3DDevice9 *pDevice,UINT resetToken)
{
	HRESULT hr = m_pD3DManager->ResetDevice (pDevice, resetToken);
	return hr;
}

STDMETHODIMP CSyncAP::OpenDeviceHandle(HANDLE *phDevice)
{
	HRESULT hr = m_pD3DManager->OpenDeviceHandle (phDevice);
	return hr;
}

STDMETHODIMP CSyncAP::CloseDeviceHandle(HANDLE hDevice)
{
	HRESULT hr = m_pD3DManager->CloseDeviceHandle(hDevice);
	return hr;
}

STDMETHODIMP CSyncAP::TestDevice(HANDLE hDevice)
{
	HRESULT hr = m_pD3DManager->TestDevice(hDevice);
	return hr;
}

STDMETHODIMP CSyncAP::LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock)
{
	HRESULT hr = m_pD3DManager->LockDevice(hDevice, ppDevice, fBlock);
	return hr;
}

STDMETHODIMP CSyncAP::UnlockDevice(HANDLE hDevice, BOOL fSaveState)
{
	HRESULT hr = m_pD3DManager->UnlockDevice(hDevice, fSaveState);
	return hr;
}

STDMETHODIMP CSyncAP::GetVideoService(HANDLE hDevice, REFIID riid, void **ppService)
{
	HRESULT hr = m_pD3DManager->GetVideoService(hDevice, riid, ppService);

	if (riid == __uuidof(IDirectXVideoDecoderService)) {
		UINT nNbDecoder = 5;
		GUID* pDecoderGuid;
		IDirectXVideoDecoderService* pDXVAVideoDecoder = (IDirectXVideoDecoderService*) *ppService;
		pDXVAVideoDecoder->GetDecoderDeviceGuids (&nNbDecoder, &pDecoderGuid);
	} else if (riid == __uuidof(IDirectXVideoProcessorService)) {
		IDirectXVideoProcessorService* pDXVAProcessor = (IDirectXVideoProcessorService*) *ppService;
		UNREFERENCED_PARAMETER(pDXVAProcessor);
	}

	return hr;
}

STDMETHODIMP CSyncAP::InitializeDevice(AM_MEDIA_TYPE* pMediaType)
{
	HRESULT hr;
	CAutoLock lock(this);
	CAutoLock lock2(&m_ImageProcessingLock);
	CAutoLock cRenderLock(&m_allocatorLock);

	RemoveAllSamples();
	DeleteSurfaces();

	VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*) pMediaType->pbFormat;
	int w = vih2->bmiHeader.biWidth;
	int h = abs(vih2->bmiHeader.biHeight);

	m_nativeVideoSize = CSize(w, h);
	if (m_b10BitOutput) {
		hr = AllocSurfaces(D3DFMT_A2R10G10B10);
	} else {
		hr = AllocSurfaces(D3DFMT_X8R8G8B8);
	}

	for (unsigned i = 0; i < m_nSurfaces; i++) {
		CComPtr<IMFSample> pMFSample;
		hr = pfMFCreateVideoSampleFromSurface(m_pVideoSurfaces[i], &pMFSample);
		if (SUCCEEDED (hr)) {
			pMFSample->SetUINT32(GUID_SURFACE_INDEX, i);
			m_FreeSamples.emplace_back(pMFSample);
		}
		ASSERT (SUCCEEDED (hr));
	}

	return hr;
}

DWORD WINAPI CSyncAP::MixerThreadStatic(LPVOID lpParam)
{
	CSyncAP *pThis = (CSyncAP*) lpParam;
	pThis->MixerThread();
	return 0;
}

void CSyncAP::MixerThread()
{
	bool bQuit = false;
	DWORD dwUser = 0;

	TIMECAPS tc = {};
	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	const UINT wTimerRes = std::max(tc.wPeriodMin, 1u);
	timeBeginPeriod(wTimerRes);

	while (!bQuit) {
		DWORD dwObject = WaitForSingleObject(m_hEvtQuit, 1);
		switch (dwObject) {
			case WAIT_OBJECT_0: // Quit
				bQuit = true;
				break;
			case WAIT_TIMEOUT : {
				bool bNewSample = false;
				{
					CAutoLock AutoLock(&m_ImageProcessingLock);
					bNewSample = GetSampleFromMixer();
				}
				if (bNewSample && m_bUseInternalTimer && m_pSubPicQueue) {
					m_pSubPicQueue->SetFPS(m_fps);
				}
			}
			break;
		}
	}

	timeEndPeriod(wTimerRes);
}

DWORD WINAPI CSyncAP::RenderThreadStatic(LPVOID lpParam)
{
	CSyncAP *pThis = (CSyncAP*)lpParam;
	pThis->RenderThread();
	return 0;
}

// Get samples that have been received and queued up by MixerThread() and present them at the correct time by calling Paint().
void CSyncAP::RenderThread()
{
	HANDLE hEvts[] = {m_hEvtQuit, m_hEvtFlush, m_hEvtSkip};
	bool bQuit = false;
	LONGLONG llRefClockTime;
	MFTIME llSystemTime;
	DWORD dwUser = 0;
	DWORD dwObject;
	int nSamplesLeft;
	CComPtr<IMFSample>pNewSample; // The sample next in line to be presented

	// Tell Vista Multimedia Class Scheduler we are doing threaded playback (increase priority)
	HANDLE hAvrt = 0;
	if (pfAvSetMmThreadCharacteristicsW) {
		DWORD dwTaskIndex = 0;
		hAvrt = pfAvSetMmThreadCharacteristicsW (L"Playback", &dwTaskIndex);
		if (pfAvSetMmThreadPriority) {
			pfAvSetMmThreadPriority (hAvrt, AVRT_PRIORITY_HIGH);
		}
	}

	// Set timer resolution
	TIMECAPS tc = {};
	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	const UINT wTimerRes = std::max(tc.wPeriodMin, 1u);
	timeBeginPeriod(wTimerRes);

	auto SubPicSetTime = [&] {
		if (!g_bExternalSubtitleTime) {
			CAllocatorPresenterImpl::SetTime(g_tSegmentStart + m_llSampleTime * (g_bExternalSubtitle ? g_dRate : 1));
		}
	};

	while (!bQuit) {
		m_lNextSampleWait = 1; // Default value for running this loop
		nSamplesLeft = 0;
		bool stepForward = false;
		LONG lDisplayCycle = (LONG)(GetDisplayCycle());
		LONG lDisplayCycle2 = (LONG)(GetDisplayCycle() / 2.0); // These are a couple of empirically determined constants used the control the "snap" function
		LONG lDisplayCycle4 = (LONG)(GetDisplayCycle() / 4.0);

		if ((m_nRenderState == Started || !m_bPrerolled) && !pNewSample) { // If either streaming or the pre-roll sample and no sample yet fetched
			if (SUCCEEDED(GetScheduledSample(&pNewSample, nSamplesLeft))) { // Get the next sample
				m_llLastSampleTime = m_llSampleTime;
				if (!m_bPrerolled) {
					m_bPrerolled = true; // m_bPrerolled is a ticket to show one (1) frame immediately
					m_lNextSampleWait = 0; // Present immediately
				} else if (SUCCEEDED(pNewSample->GetSampleTime(&m_llSampleTime))) { // Get zero-based sample due time
					if (m_llLastSampleTime == m_llSampleTime) { // In the rare case there are duplicate frames in the movie. There really shouldn't be but it happens.
						MoveToFreeList(pNewSample, true);
						pNewSample.Release();
						m_lNextSampleWait = 0;
					} else {
						m_pClock->GetCorrelatedTime(0, &llRefClockTime, &llSystemTime); // Get zero-based reference clock time. llSystemTime is not used for anything here
						m_lNextSampleWait = (LONG)((m_llSampleTime - llRefClockTime) / 10000); // Time left until sample is due, in ms
						if (m_bStepping) {
							m_lNextSampleWait = 0;
						} else if (m_ExtraSets.iSynchronizeMode == SYNCHRONIZE_NEAREST) { // Present at the closest "safe" occasion at dTargetSyncOffset ms before vsync to avoid tearing
							if (m_lNextSampleWait < -lDisplayCycle) { // We have to allow slightly negative numbers at this stage. Otherwise we get "choking" when frame rate > refresh rate
								SetEvent(m_hEvtSkip);
								m_bEvtSkip = true;
							}

							REFERENCE_TIME rtRefClockTimeNow = m_llEstVBlankTime;
							if (m_pRefClock) {
								m_pRefClock->GetTime(&rtRefClockTimeNow); // Reference clock time now
							}
							LONG lLastVsyncTime = (LONG)((m_llEstVBlankTime - rtRefClockTimeNow) / 10000); // Last vsync time relative to now
							if (abs(lLastVsyncTime) > lDisplayCycle) {
								lLastVsyncTime = - lDisplayCycle;    // To even out glitches in the beginning
							}

							LONGLONG llNextSampleWait = (LONGLONG)((lLastVsyncTime + GetDisplayCycle() - m_ExtraSets.dTargetSyncOffset) * 10000); // Time from now util next safe time to Paint()
							while ((llRefClockTime + llNextSampleWait) < (m_llSampleTime + m_llHysteresis)) { // While the proposed time is in the past of sample presentation time
								llNextSampleWait = llNextSampleWait + (LONGLONG)(GetDisplayCycle() * 10000); // Try the next possible time, one display cycle ahead
							}
							m_lNextSampleWait = (LONG)(llNextSampleWait / 10000);
							m_lShiftToNearestPrev = m_lShiftToNearest;
							m_lShiftToNearest = (LONG)((llRefClockTime + llNextSampleWait - m_llSampleTime) / 10000); // The adjustment made to get to the sweet point in time, in ms

							// If m_lShiftToNearest is pushed a whole cycle into the future, then we are getting more frames
							// than we can chew and we need to throw one away. We don't want to wait many cycles and skip many
							// frames.
							if (m_lShiftToNearest > (lDisplayCycle + 1)) {
								SetEvent(m_hEvtSkip);
								m_bEvtSkip = true;
							}

							// We need to add a hysteresis to the control of the timing adjustment to avoid judder when
							// presentation time is close to vsync and the renderer couldn't otherwise make up its mind
							// whether to present before the vsync or after. That kind of indecisiveness leads to judder.
							if (m_bSnapToVSync) {

								if ((m_lShiftToNearestPrev - m_lShiftToNearest) > lDisplayCycle2) { // If a step down in the m_lShiftToNearest function. Display slower than video.
									m_bVideoSlowerThanDisplay = false;
									m_llHysteresis = -(LONGLONG)(10000 * lDisplayCycle4);
								} else if ((m_lShiftToNearest - m_lShiftToNearestPrev) > lDisplayCycle2) { // If a step up
									m_bVideoSlowerThanDisplay = true;
									m_llHysteresis = (LONGLONG)(10000 * lDisplayCycle4);
								} else if ((m_lShiftToNearest < (3 * lDisplayCycle4)) && (m_lShiftToNearest > lDisplayCycle4)) {
									m_llHysteresis = 0;    // Reset when between 1/4 and 3/4 of the way either way
								}

								if ((m_lShiftToNearest < lDisplayCycle2) && (m_llHysteresis > 0)) {
									m_llHysteresis = 0;    // Should never really be in this territory.
								}
								if (m_lShiftToNearest < 0) {
									m_llHysteresis = 0;    // A glitch might get us to a sticky state where both these numbers are negative.
								}
								if ((m_lShiftToNearest > lDisplayCycle2) && (m_llHysteresis < 0)) {
									m_llHysteresis = 0;
								}
							}
						}

						if (m_lNextSampleWait < 0) { // Skip late or duplicate sample.
							SetEvent(m_hEvtSkip);
							m_bEvtSkip = true;
						}

						if (m_lNextSampleWait > 1000) {
							m_lNextSampleWait = 1000; // So as to avoid full a full stop when sample is far in the future (shouldn't really happen).
						}
					}
				} // if got new sample
			}
		}
		// Wait for the next presentation time (m_lNextSampleWait) or some other event.
		dwObject = WaitForMultipleObjects(std::size(hEvts), hEvts, FALSE, (DWORD)m_lNextSampleWait);
		switch (dwObject) {
			case WAIT_OBJECT_0: // Quit
				bQuit = true;
				break;

			case WAIT_OBJECT_0 + 1: // Flush
				if (pNewSample) {
					MoveToFreeList(pNewSample, true);
				}
				pNewSample.Release();
				FlushSamples();
				m_bEvtFlush = false;
				ResetEvent(m_hEvtFlush);
				m_bPrerolled = false;
				m_lShiftToNearest = 0;
				stepForward = true;
				break;

			case WAIT_OBJECT_0 + 2: // Skip sample
				m_pcFramesDropped++;
				m_llSampleTime = m_llLastSampleTime; // This sample will never be shown
				m_bEvtSkip = false;
				ResetEvent(m_hEvtSkip);
				stepForward = true;
				break;

			case WAIT_TIMEOUT: // Time to show the sample or something
				if (m_LastSetOutputRange != -1 && m_LastSetOutputRange != m_ExtraSets.iEVROutputRange || m_bPendingRenegotiate) {
					if (pNewSample) {
						MoveToFreeList(pNewSample, true);
					}
					pNewSample.Release();
					FlushSamples();
					RenegotiateMediaType();
					m_bPendingRenegotiate = false;
				}

				if (m_bPendingResetDevice) {
					if (pNewSample) {
						MoveToFreeList(pNewSample, true);
					}
					pNewSample.Release();
					SendResetRequest();
				} else if (m_nStepCount < 0) {
					m_nStepCount = 0;
					m_pcFramesDropped++;
					stepForward = true;
				} else if (pNewSample && (m_nStepCount > 0)) {
					pNewSample->GetUINT32(GUID_SURFACE_INDEX, &m_iCurSurface);
					SubPicSetTime();
					Paint(true);
					CompleteFrameStep(false);
					m_pcFramesDrawn++;
					stepForward = true;
				} else if (pNewSample && !m_bStepping) { // When a stepped frame is shown, a new one is fetched that we don't want to show here while stepping
					pNewSample->GetUINT32(GUID_SURFACE_INDEX, &m_iCurSurface);
					SubPicSetTime();
					Paint(true);
					m_pcFramesDrawn++;
					stepForward = true;
				}
				break;
		} // switch
		if (pNewSample && stepForward) {
			MoveToFreeList(pNewSample, true);
			pNewSample.Release();
		}
	} // while

	if (pNewSample) {
		MoveToFreeList(pNewSample, true);
		pNewSample.Release();
	}

	timeEndPeriod(wTimerRes);
	if (pfAvRevertMmThreadCharacteristics) {
		pfAvRevertMmThreadCharacteristics(hAvrt);
	}
}

STDMETHODIMP_(bool) CSyncAP::ResetDevice()
{
	CAutoLock lock(this);
	CAutoLock lock2(&m_ImageProcessingLock);
	CAutoLock cRenderLock(&m_allocatorLock);
	RemoveAllSamples();

	bool bResult = __super::ResetDevice();

	for (unsigned i = 0; i < m_nSurfaces; i++) {
		CComPtr<IMFSample> pMFSample;
		HRESULT hr = pfMFCreateVideoSampleFromSurface (m_pVideoSurfaces[i], &pMFSample);
		if (SUCCEEDED (hr)) {
			pMFSample->SetUINT32(GUID_SURFACE_INDEX, i);
			m_FreeSamples.emplace_back(pMFSample);
		}
		ASSERT(SUCCEEDED (hr));
	}
	return bResult;
}

void CSyncAP::OnResetDevice()
{
	DLog(L"--> CSyncAP::OnResetDevice on thread: %u", GetCurrentThreadId());
	HRESULT hr;
	hr = m_pD3DManager->ResetDevice(m_pDevice9Ex, m_nResetToken);
	if (m_pSink) {
		m_pSink->Notify(EC_DISPLAY_CHANGED, 0, 0);
	}
	CSize videoSize = m_nativeVideoSize;
	if (m_pSink) {
		m_pSink->Notify(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(videoSize.cx, videoSize.cy), 0);
	}
}

void CSyncAP::RemoveAllSamples()
{
	CAutoLock AutoLock(&m_ImageProcessingLock);
	FlushSamples();
	m_ScheduledSamples.clear();
	m_FreeSamples.clear();
	m_nUsedBuffer = 0;
}

HRESULT CSyncAP::GetFreeSample(IMFSample** ppSample)
{
	CAutoLock lock(&m_SampleQueueLock);
	HRESULT hr = S_OK;

	if (m_FreeSamples.size() > 1) { // <= Cannot use first free buffer (can be currently displayed)
		InterlockedIncrement(&m_nUsedBuffer);
		*ppSample = m_FreeSamples.front().Detach();
		m_FreeSamples.pop_front();
	} else {
		hr = MF_E_SAMPLEALLOCATOR_EMPTY;
	}

	return hr;
}

HRESULT CSyncAP::GetScheduledSample(IMFSample** ppSample, int &_Count)
{
	CAutoLock lock(&m_SampleQueueLock);
	HRESULT hr = S_OK;

	_Count = (int)m_ScheduledSamples.size();
	if (_Count > 0) {
		*ppSample = m_ScheduledSamples.front().Detach();
		m_ScheduledSamples.pop_front();
		--_Count;
	} else {
		hr = MF_E_SAMPLEALLOCATOR_EMPTY;
	}

	return hr;
}

void CSyncAP::MoveToFreeList(IMFSample* pSample, bool bTail)
{
	CAutoLock lock(&m_SampleQueueLock);
	InterlockedDecrement(&m_nUsedBuffer);
	if (m_bPendingMediaFinished && m_nUsedBuffer == 0) {
		m_bPendingMediaFinished = false;
		m_pSink->Notify(EC_COMPLETE, 0, 0);
	}
	if (bTail) {
		m_FreeSamples.emplace_back(pSample);
	} else {
		m_FreeSamples.emplace_front(pSample);
	}
}

void CSyncAP::MoveToScheduledList(IMFSample* pSample, bool _bSorted)
{
	if (_bSorted) {
		CAutoLock lock(&m_SampleQueueLock);
		m_ScheduledSamples.emplace_front(pSample);
	} else {
		CAutoLock lock(&m_SampleQueueLock);
		m_ScheduledSamples.emplace_back(pSample);
	}
}

void CSyncAP::FlushSamples()
{
	CAutoLock lock(this);
	CAutoLock lock2(&m_SampleQueueLock);
	FlushSamplesInternal();
}

void CSyncAP::FlushSamplesInternal()
{
	m_bPrerolled = false;
	while (m_ScheduledSamples.size() > 0) {
		CComPtr<IMFSample> pMFSample;
		pMFSample = m_ScheduledSamples.front();
		m_ScheduledSamples.pop_front();
		MoveToFreeList(pMFSample, true);
	}
}

HRESULT CSyncAP::AdviseSyncClock(ISyncClock* sC)
{
	return m_pGenlock->AdviseSyncClock(sC);
}

HRESULT CSyncAP::BeginStreaming()
{
	m_pcFramesDropped = 0;
	m_pcFramesDrawn = 0;

	CComPtr<IBaseFilter> pEVR;
	FILTER_INFO filterInfo;
	ZeroMemory(&filterInfo, sizeof(filterInfo));
	m_pOuterEVR->QueryInterface (__uuidof(IBaseFilter), (void**)&pEVR);
	pEVR->QueryFilterInfo(&filterInfo); // This addref's the pGraph member

	BeginEnumFilters(filterInfo.pGraph, pEF, pBF)
	if (CComQIPtr<IAMAudioRendererStats> pAS = pBF.p) {
		m_pAudioStats = pAS;
	};
	EndEnumFilters

	pEVR->GetSyncSource(&m_pRefClock);
	if (filterInfo.pGraph) {
		filterInfo.pGraph->Release();
	}
	m_pGenlock->SetMonitor(D3D9Helper::GetAdapter(m_pD3D9Ex, m_hWnd));

	ResetStats();
	EstimateRefreshTimings();
	if (m_dFrameCycle > 0.0) {
		m_dCycleDifference = GetCycleDifference();    // Might have moved to another display
	}
	return S_OK;
}

//
// CSyncRenderer
//

CSyncRenderer::CSyncRenderer(LPCWSTR pName, LPUNKNOWN pUnk, HRESULT& hr, CSyncAP *pAllocatorPresenter): CUnknown(pName, pUnk)
{
	DLog(L"CSyncRenderer::CSyncRenderer()");

	hr = m_pEVR.CoCreateInstance(CLSID_EnhancedVideoRenderer, GetOwner());
	CComQIPtr<IBaseFilter> pEVRBase(m_pEVR);
	m_pEVRBase = pEVRBase; // Don't keep a second reference on the EVR filter
	m_pAllocatorPresenter = pAllocatorPresenter;
}

CSyncRenderer::~CSyncRenderer()
{
	DLog(L"CSyncRenderer::~CSyncRenderer()");
}

STDMETHODIMP CSyncRenderer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	HRESULT hr;

	if (riid == __uuidof(IBaseFilter)) {
		return GetInterface((IBaseFilter*)this, ppv);
	}
	if (riid == __uuidof(IMediaFilter)) {
		return GetInterface((IMediaFilter*)this, ppv);
	}
	if (riid == __uuidof(IPersist)) {
		return GetInterface((IPersist*)this, ppv);
	}

	hr = m_pEVR ? m_pEVR->QueryInterface(riid, ppv) : E_NOINTERFACE;
	if (m_pEVR && FAILED(hr)) {
		hr = m_pAllocatorPresenter ? m_pAllocatorPresenter->QueryInterface(riid, ppv) : E_NOINTERFACE;
	}
	return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CSyncRenderer::EnumPins(__out IEnumPins **ppEnum)
{
	if (m_pEVRBase) {
		return m_pEVRBase->EnumPins(ppEnum);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::FindPin(LPCWSTR Id, __out  IPin **ppPin)
{
	if (m_pEVRBase) {
		return m_pEVRBase->FindPin(Id, ppPin);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::QueryFilterInfo(__out  FILTER_INFO *pInfo)
{
	if (m_pEVRBase) {
		return m_pEVRBase->QueryFilterInfo(pInfo);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::JoinFilterGraph(__in_opt  IFilterGraph *pGraph, __in_opt  LPCWSTR pName)
{
	if (m_pEVRBase) {
		return m_pEVRBase->JoinFilterGraph(pGraph, pName);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::QueryVendorInfo(__out  LPWSTR *pVendorInfo)
{
	if (m_pEVRBase) {
		return m_pEVRBase->QueryVendorInfo(pVendorInfo);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::Stop()
{
	if (m_pEVRBase) {
		return m_pEVRBase->Stop();
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::Pause()
{
	if (m_pEVRBase) {
		return m_pEVRBase->Pause();
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::Run(REFERENCE_TIME tStart)
{
	if (m_pEVRBase) {
		return m_pEVRBase->Run(tStart);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::GetState(DWORD dwMilliSecsTimeout, __out  FILTER_STATE *State)
{
	if (m_pEVRBase) {
		return m_pEVRBase->GetState(dwMilliSecsTimeout, State);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::SetSyncSource(__in_opt IReferenceClock *pClock)
{
	if (m_pEVRBase) {
		return m_pEVRBase->SetSyncSource(pClock);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::GetSyncSource(__deref_out_opt IReferenceClock **pClock)
{
	if (m_pEVRBase) {
		return m_pEVRBase->GetSyncSource(pClock);
	}
	return E_NOTIMPL;
}

STDMETHODIMP CSyncRenderer::GetClassID(__RPC__out CLSID *pClassID)
{
	if (m_pEVRBase) {
		return m_pEVRBase->GetClassID(pClassID);
	}
	return E_NOTIMPL;
}

// ID3DFullscreenControl
STDMETHODIMP CSyncAP::SetD3DFullscreen(bool fEnabled)
{
	m_bIsFullscreen = fEnabled;
	return S_OK;
}

STDMETHODIMP CSyncAP::GetD3DFullscreen(bool* pfEnabled)
{
	CheckPointer(pfEnabled, E_POINTER);
	*pfEnabled = m_bIsFullscreen;
	return S_OK;
}

// IPlaybackNotify
STDMETHODIMP CSyncAP::Stop()
{
	for (unsigned i = 0; i < m_nSurfaces + 2; i++) {
		if (m_pVideoSurfaces[i]) {
			m_pDevice9Ex->ColorFill(m_pVideoSurfaces[i], nullptr, 0);
		}
	}

	return S_OK;
}

//
// CGenlock
//

CGenlock::CGenlock(double target, double limit, double clockD, UINT mon)
	: targetSyncOffset(target) // Target sync offset, typically around 10 ms
	, controlLimit(limit)      // How much sync offset is allowed to drift from target sync offset before control kicks in
	, lowSyncOffset(target - limit)
	, highSyncOffset(target + limit)
	, cycleDelta(clockD)       // Delta used in clock speed adjustment. In fractions of 1.0. Typically around 0.001
	, monitor(mon)             // The monitor to be adjusted if the display refresh rate is the controlled parameter
{
}

CGenlock::~CGenlock()
{
	syncClock.Release();
};

// Reset reference clock speed to nominal.
HRESULT CGenlock::ResetClock()
{
	adjDelta = 0;
	if (syncClock == nullptr) {
		return E_FAIL;
	} else {
		return syncClock->AdjustClock(1.0);
	}
}

HRESULT CGenlock::SetTargetSyncOffset(double targetD)
{
	targetSyncOffset = targetD;
	lowSyncOffset = targetD - controlLimit;
	highSyncOffset = targetD + controlLimit;
	return S_OK;
}

HRESULT CGenlock::GetTargetSyncOffset(double *targetD)
{
	*targetD = targetSyncOffset;
	return S_OK;
}

HRESULT CGenlock::SetControlLimit(double cL)
{
	controlLimit = cL;
	lowSyncOffset = targetSyncOffset - cL;
	highSyncOffset = targetSyncOffset + cL;
	return S_OK;
}

HRESULT CGenlock::GetControlLimit(double *cL)
{
	*cL = controlLimit;
	return S_OK;
}

HRESULT CGenlock::SetDisplayResolution(UINT columns, UINT lines)
{
	visibleColumns = columns;
	visibleLines = lines;
	return S_OK;
}

HRESULT CGenlock::AdviseSyncClock(ISyncClock* sC)
{
	if (!sC) {
		return E_FAIL;
	}
	if (syncClock) {
		syncClock = nullptr;    // Release any outstanding references if this is called repeatedly
	}
	syncClock = sC;
	return S_OK;
}

// Set the monitor to control. This is best done manually as not all monitors can be controlled
// so automatic detection of monitor to control might have unintended effects.
// The PowerStrip API uses zero-based monitor numbers, i.e. the default monitor is 0.
HRESULT CGenlock::SetMonitor(UINT mon)
{
	monitor = mon;
	return S_OK;
}

HRESULT CGenlock::ResetStats()
{
	CAutoLock lock(&csGenlockLock);
	minSyncOffset = 1000000.0;
	maxSyncOffset = -1000000.0;
	minFrameCycle = 1000000.0;
	maxFrameCycle = -1000000.0;
	clockAdjustmentsMade = 0;
	return S_OK;
}

// Synchronize by adjusting reference clock rate (and therefore video FPS).
// Todo: check so that we don't have a live source
HRESULT CGenlock::ControlClock(double syncOffset, double frameCycle)
{
	syncOffsetAvg = syncOffsetFifo.Average(syncOffset);
	expand_range(syncOffset, minSyncOffset, maxSyncOffset);

	frameCycleAvg = frameCycleFifo.Average(frameCycle);
	expand_range(frameCycle, minFrameCycle, maxFrameCycle);

	if (!syncClock) {
		return E_FAIL;
	}
	// Adjust as seldom as possible by checking the current controlState before changing it.
	if ((syncOffsetAvg > highSyncOffset) && (adjDelta != 1))
		// Slow down video stream.
	{
		adjDelta = 1;
		syncClock->AdjustClock(1.0 - cycleDelta); // Makes the clock move slower by providing smaller increments
		clockAdjustmentsMade++;
	} else
		// Speed up video stream.
		if ((syncOffsetAvg < lowSyncOffset) && (adjDelta != -1)) {
			adjDelta = -1;
			syncClock->AdjustClock(1.0 + cycleDelta);
			clockAdjustmentsMade++;
		} else
			// Cruise.
			if ((syncOffsetAvg < targetSyncOffset) && (adjDelta == 1)) {
				adjDelta = 0;
				syncClock->AdjustClock(1.0);
				clockAdjustmentsMade++;
			} else if ((syncOffsetAvg > targetSyncOffset) && (adjDelta == -1)) {
				adjDelta = 0;
				syncClock->AdjustClock(1.0);
				clockAdjustmentsMade++;
			}
	return S_OK;
}

// Don't adjust anything, just update the syncOffset stats
HRESULT CGenlock::UpdateStats(double syncOffset, double frameCycle)
{
	syncOffsetAvg = syncOffsetFifo.Average(syncOffset);
	expand_range(syncOffset, minSyncOffset, maxSyncOffset);

	frameCycleAvg = frameCycleFifo.Average(frameCycle);
	expand_range(frameCycle, minFrameCycle, maxFrameCycle);

	return S_OK;
}
