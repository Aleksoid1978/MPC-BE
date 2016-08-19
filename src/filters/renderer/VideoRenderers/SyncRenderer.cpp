/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include "../SyncClock/ISyncClock.h"
#include <atlbase.h>
#include <atlcoll.h>
#include <strsafe.h> // Required in CGenlock
#include <videoacc.h>
#include <InitGuid.h>
#include <d3d9.h>
#include <dx/d3dx9.h>
#include <vmr9.h>
#include <evr.h>
#include <mfapi.h>
#include <Mferror.h>
#include <vector>
#include "../../../SubPic/DX9SubPic.h"
#include "../../../SubPic/DX9SubPic.h"
#include "../../../SubPic/SubPicQueueImpl.h"
#include <moreuuids.h>
#include "MacrovisionKicker.h"
#include "IPinHook.h"
#include "PixelShaderCompiler.h"
#include "DX9Shaders.h"
#include "SyncRenderer.h"
#include <Version.h>
#include "FocusThread.h"
#include "../DSUtil/D3D9Helper.h"

using namespace GothSync;
using namespace D3D9Helper;

extern bool LoadResource(UINT resid, CStringA& str, LPCTSTR restype);

// CBaseAP

CBaseAP::CBaseAP(HWND hWnd, bool bFullscreen, HRESULT& hr, CString &_Error):
	CSubPicAllocatorPresenterImpl(hWnd, hr, &_Error),
	m_ScreenSize(0, 0),
	m_iRotation(0),
	m_wsResizer(L""), // empty string, not nullptr
	m_nSurface(1),
	m_iCurSurface(0),
	m_bSnapToVSync(false),
	m_bInterlaced(0),
	m_nUsedBuffer(0),
	m_TextScale(1.0),
	m_bNeedCheckSample(true),
	m_hEvtQuit(NULL),
	m_bIsFullscreen(bFullscreen),
	m_uSyncGlitches(0),
	m_pGenlock(NULL),
	m_lAudioLag(0),
	m_lAudioLagMin(10000),
	m_lAudioLagMax(-10000),
	m_pAudioStats(NULL),
	m_nNextJitter(0),
	m_nNextSyncOffset(0),
	m_llLastSyncTime(0),
	m_fAvrFps(0.0),
	m_fJitterStdDev(0.0),
	m_fSyncOffsetStdDev(0.0),
	m_fSyncOffsetAvr(0.0),
	m_llHysteresis(0),
	m_dD3DRefreshCycle(0),
	m_dDetectedScanlineTime(0.0),
	m_dEstRefreshCycle(0.0),
	m_dFrameCycle(0.0),
	m_dOptimumDisplayCycle(0.0),
	m_dCycleDifference(1.0),
	m_llEstVBlankTime(0),
	m_CurrentAdapter(0),
	m_FocusThread(NULL)
{
	if (FAILED(hr)) {
		_Error += L"ISubPicAllocatorPresenterImpl failed\n";
		return;
	}

	HINSTANCE hDll;
	m_pD3DXLoadSurfaceFromMemory = NULL;
	m_pD3DXCreateLine = NULL;
	m_pD3DXCreateFont = NULL;
	m_pD3DXCreateSprite = NULL;
	hDll = GetD3X9Dll();
	if (hDll) {
		(FARPROC &)m_pD3DXLoadSurfaceFromMemory = GetProcAddress(hDll, "D3DXLoadSurfaceFromMemory");
		(FARPROC &)m_pD3DXCreateLine = GetProcAddress(hDll, "D3DXCreateLine");
		(FARPROC &)m_pD3DXCreateFont = GetProcAddress(hDll, "D3DXCreateFontW");
		(FARPROC &)m_pD3DXCreateSprite = GetProcAddress(hDll, "D3DXCreateSprite");
	} else {
		_Error += L"The installed DirectX End-User Runtime is outdated. Please download and install the June 2010 release or newer in order for MPC-BE to function properly.\n";
	}

	m_pDwmIsCompositionEnabled = NULL;
	m_pDwmEnableComposition = NULL;
	m_hDWMAPI = LoadLibrary(L"dwmapi.dll");
	if (m_hDWMAPI) {
		(FARPROC &)m_pDwmIsCompositionEnabled = GetProcAddress(m_hDWMAPI, "DwmIsCompositionEnabled");
		(FARPROC &)m_pDwmEnableComposition = GetProcAddress(m_hDWMAPI, "DwmEnableComposition");
	}

	m_pDirect3DCreate9Ex = NULL;
	m_hD3D9 = LoadLibrary(L"d3d9.dll");
	if (m_hD3D9) {
		(FARPROC &)m_pDirect3DCreate9Ex = GetProcAddress(m_hD3D9, "Direct3DCreate9Ex");
	}

	if (m_pDirect3DCreate9Ex) {
		m_pDirect3DCreate9Ex(D3D_SDK_VERSION, &m_pD3DEx);
		if (!m_pD3DEx) {
			m_pDirect3DCreate9Ex(D3D9b_SDK_VERSION, &m_pD3DEx);
		}
	}

	if (!m_pD3DEx) {
		hr = E_FAIL;
		_Error += L"Failed to create Direct3D 9Ex\n";
		return;
	}

	ZeroMemory(&m_VMR9AlphaBitmap, sizeof(m_VMR9AlphaBitmap));

	CRenderersSettings& rs = GetRenderersSettings();
	if (rs.m_AdvRendSets.bDisableDesktopComposition) {
		m_bDesktopCompositionDisabled = true;
		if (m_pDwmEnableComposition) {
			m_pDwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
		}
	} else {
		m_bDesktopCompositionDisabled = false;
	}

	m_pGenlock = DNew CGenlock(rs.m_AdvRendSets.dTargetSyncOffset, rs.m_AdvRendSets.dControlLimit, rs.m_AdvRendSets.iLineDelta, rs.m_AdvRendSets.iColumnDelta, rs.m_AdvRendSets.dCycleDelta, 0); // Must be done before CreateDXDevice
	hr = CreateDXDevice(_Error);

	// Define the shader profile.
	if (m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0)) {
		m_ShaderProfile = "ps_3_0";
	} else if (m_caps.PixelShaderVersion >= D3DPS_VERSION(2,0)) {
		// http://en.wikipedia.org/wiki/High-level_shader_language

		if (m_caps.PS20Caps.NumTemps >= 22
			&& (m_caps.PS20Caps.Caps & (D3DPS20CAPS_ARBITRARYSWIZZLE | D3DPS20CAPS_GRADIENTINSTRUCTIONS |
			D3DPS20CAPS_PREDICATION | D3DPS20CAPS_NODEPENDENTREADLIMIT | D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT))) {
			m_ShaderProfile = "ps_2_a";
		} else if (m_caps.PS20Caps.NumTemps >= 32
			&& (m_caps.PS20Caps.Caps & D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT)) {
			m_ShaderProfile = "ps_2_b";
		} else {
			m_ShaderProfile = "ps_2_0";
		}
	} else {
		m_ShaderProfile = NULL;
	}

	memset (m_pllJitter, 0, sizeof(m_pllJitter));
	memset (m_pllSyncOffset, 0, sizeof(m_pllSyncOffset));
}

CBaseAP::~CBaseAP()
{
	if (m_bDesktopCompositionDisabled) {
		m_bDesktopCompositionDisabled = false;
		if (m_pDwmEnableComposition) {
			m_pDwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
		}
	}

	m_pFont = NULL;
	m_pLine = NULL;
	m_pD3DDevEx = NULL;
	m_pPSC.Free();
	m_pD3DEx = NULL;
	if (m_hDWMAPI) {
		FreeLibrary(m_hDWMAPI);
		m_hDWMAPI = NULL;
	}
	if (m_hD3D9) {
		FreeLibrary(m_hD3D9);
		m_hD3D9 = NULL;
	}
	m_pAudioStats = NULL;
	SAFE_DELETE(m_pGenlock);

	if (m_FocusThread) {
		m_FocusThread->PostThreadMessage(WM_QUIT, 0, 0);
		if (WaitForSingleObject(m_FocusThread->m_hThread, 10000) == WAIT_TIMEOUT) {
			ASSERT(FALSE);
			TerminateThread(m_FocusThread->m_hThread, 0xDEAD);
		}
	}
}

template<int texcoords>
void CBaseAP::AdjustQuad(MYD3DVERTEX<texcoords>* v, double dx, double dy)
{
	float offset = 0.5;

	for (int i = 0; i < 4; i++) {
		v[i].x -= offset;
		v[i].y -= offset;

		for (int j = 0; j < max(texcoords-1, 1); j++) {
			v[i].t[j].u -= (float)(offset*dx);
			v[i].t[j].v -= (float)(offset*dy);
		}

		if (texcoords > 1) {
			v[i].t[texcoords-1].u -= offset;
			v[i].t[texcoords-1].v -= offset;
		}
	}
}

template<int texcoords>
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

	for (int i = 0; i < texcoords; i++) {
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MAGFILTER, filter);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MINFILTER, filter);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | FVF);

	MYD3DVERTEX<texcoords> tmp = v[2];
	v[2] = v[3];
	v[3] = tmp;
	hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));

	for (int i = 0; i < texcoords; i++) {
		pD3DDev->SetTexture(i, NULL);
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

	MYD3DVERTEX<0> tmp = v[2];
	v[2] = v[3];
	v[3] = tmp;
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

void CBaseAP::ResetStats()
{
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

bool CBaseAP::SettingsNeedResetDevice()
{
	CRenderersSettings& rs = GetRenderersSettings();
	CRenderersSettings::CAdvRendererSettings & New = rs.m_AdvRendSets;
	CRenderersSettings::CAdvRendererSettings & Current = m_LastRendererSettings;

	bool bRet = false;
	if (!m_bIsFullscreen) {
		if (Current.bDisableDesktopComposition) {
			if (!m_bDesktopCompositionDisabled) {
				m_bDesktopCompositionDisabled = true;
				if (m_pDwmEnableComposition) {
					m_pDwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
				}
			}
		} else {
			if (m_bDesktopCompositionDisabled) {
				m_bDesktopCompositionDisabled = false;
				if (m_pDwmEnableComposition) {
					m_pDwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
				}
			}
		}
	}
	bRet = bRet || New.b10BitOutput != Current.b10BitOutput;
	bRet = bRet || New.iSurfaceFormat != Current.iSurfaceFormat;
	m_LastRendererSettings = rs.m_AdvRendSets;
	return bRet;
}

HRESULT CBaseAP::CreateDXDevice(CString &_Error)
{
	TRACE("--> CBaseAP::CreateDXDevice on thread: %d\n", GetCurrentThreadId());

	CAutoLock cRenderLock(&m_allocatorLock);

	CRenderersSettings& rs = GetRenderersSettings();
	m_LastRendererSettings = rs.m_AdvRendSets;
	HRESULT hr = E_FAIL;

	m_pFont = NULL;
	m_pSprite = NULL;
	m_pLine = NULL;

	m_pPSC.Free();
	m_pD3DDevEx = NULL;

	for (int i = 0; i < _countof(m_pResizerPixelShader); i++) {
		m_pResizerPixelShader[i] = NULL;
	}

	POSITION pos = m_pPixelShadersScreenSpace.GetHeadPosition();
	while (pos) {
		CExternalPixelShader &Shader = m_pPixelShadersScreenSpace.GetNext(pos);
		Shader.m_pPixelShader = NULL;
	}
	pos = m_pPixelShaders.GetHeadPosition();
	while (pos) {
		CExternalPixelShader &Shader = m_pPixelShaders.GetNext(pos);
		Shader.m_pPixelShader = NULL;
	}

	UINT currentAdapter = GetAdapter(m_pD3DEx, m_hWnd);
	bool bTryToReset = (currentAdapter == m_CurrentAdapter);

	if (!bTryToReset) {
		m_pD3DDevEx = NULL;
		m_CurrentAdapter = currentAdapter;
	}

	if (!m_pD3DEx) {
		_Error += L"Failed to create Direct3D device\n";
		return E_UNEXPECTED;
	}

	D3DDISPLAYMODE d3ddm;
	ZeroMemory(&d3ddm, sizeof(d3ddm));
	if (FAILED(m_pD3DEx->GetAdapterDisplayMode(m_CurrentAdapter, &d3ddm))) {
		_Error += L"Can not retrieve display mode data\n";
		return E_UNEXPECTED;
	}

	if (FAILED(m_pD3DEx->GetDeviceCaps(m_CurrentAdapter, D3DDEVTYPE_HAL, &m_caps))) {
		if ((m_caps.Caps & D3DCAPS_READ_SCANLINE) == 0) {
			_Error += L"Video card does not have scanline access. Display synchronization is not possible.\n";
			return E_UNEXPECTED;
		}
	}

	m_refreshRate = d3ddm.RefreshRate;
	m_dD3DRefreshCycle = 1000.0 / m_refreshRate; // In ms
	m_ScreenSize.SetSize(d3ddm.Width, d3ddm.Height);
	m_pGenlock->SetDisplayResolution(d3ddm.Width, d3ddm.Height);

	BOOL bCompositionEnabled = false;
	if (m_pDwmIsCompositionEnabled) {
		m_pDwmIsCompositionEnabled(&bCompositionEnabled);
	}
	m_bCompositionEnabled = bCompositionEnabled != 0;

	ZeroMemory(&pp, sizeof(pp));
	if (m_bIsFullscreen) { // Exclusive mode fullscreen
		pp.Windowed = FALSE;
		pp.BackBufferWidth = d3ddm.Width;
		pp.BackBufferHeight = d3ddm.Height;
		pp.hDeviceWindow = m_hWnd;
		DEBUG_ONLY(_tprintf_s(_T("Wnd in CreateDXDevice: %p\n"), m_hWnd));
		pp.BackBufferCount = 3;
		pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		pp.Flags = D3DPRESENTFLAG_VIDEO;
		m_b10BitOutput = rs.m_AdvRendSets.b10BitOutput;
		if (m_b10BitOutput) {
			if (FAILED(m_pD3DEx->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, D3DFMT_A2R10G10B10, false))) {
				m_strStatsMsg[MSG_ERROR] = L"10 bit RGB is not supported by this graphics device in this resolution.";
				m_b10BitOutput = false;
			}
		}

		if (m_b10BitOutput) {
			pp.BackBufferFormat = D3DFMT_A2R10G10B10;
		} else {
			pp.BackBufferFormat = d3ddm.Format;
		}

		if (!m_FocusThread) {
			m_FocusThread = (CFocusThread*)AfxBeginThread(RUNTIME_CLASS(CFocusThread), 0, 0, 0);
		}

		D3DDISPLAYMODEEX DisplayMode;
		ZeroMemory(&DisplayMode, sizeof(DisplayMode));
		DisplayMode.Size = sizeof(DisplayMode);
		m_pD3DEx->GetAdapterDisplayModeEx(m_CurrentAdapter, &DisplayMode, NULL);

		DisplayMode.Format = pp.BackBufferFormat;
		pp.FullScreen_RefreshRateInHz = DisplayMode.RefreshRate;

		bTryToReset = bTryToReset && m_pD3DDevEx && SUCCEEDED(hr = m_pD3DDevEx->ResetEx(&pp, &DisplayMode));

		if (!bTryToReset) {
			m_pD3DDevEx = NULL;

			hr = m_pD3DEx->CreateDeviceEx(
					m_CurrentAdapter, D3DDEVTYPE_HAL, m_FocusThread->GetFocusWindow(),
					D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_ENABLE_PRESENTSTATS | D3DCREATE_NOWINDOWCHANGES,
					&pp, &DisplayMode, &m_pD3DDevEx);
		}

		if (m_pD3DDevEx) {
			m_BackbufferFmt = pp.BackBufferFormat;
			m_DisplayFmt = DisplayMode.Format;
		}
	} else { // Windowed
		pp.Windowed = TRUE;
		pp.hDeviceWindow = m_hWnd;
		pp.SwapEffect = D3DSWAPEFFECT_COPY;
		pp.Flags = D3DPRESENTFLAG_VIDEO;
		pp.BackBufferCount = 1;
		pp.BackBufferWidth = d3ddm.Width;
		pp.BackBufferHeight = d3ddm.Height;
		m_BackbufferFmt = d3ddm.Format;
		m_DisplayFmt = d3ddm.Format;
		m_b10BitOutput = rs.m_AdvRendSets.b10BitOutput;
		if (m_b10BitOutput) {
			if (FAILED(m_pD3DEx->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, D3DFMT_A2R10G10B10, false))) {
				m_strStatsMsg[MSG_ERROR] = L"10 bit RGB is not supported by this graphics device in this resolution.";
				m_b10BitOutput = false;
			}
		}

		if (m_b10BitOutput) {
			m_BackbufferFmt = D3DFMT_A2R10G10B10;
			pp.BackBufferFormat = D3DFMT_A2R10G10B10;
		}
		if (bCompositionEnabled) {
			// Desktop composition presents the whole desktop
			pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		} else {
			pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		}

		bTryToReset = bTryToReset && m_pD3DDevEx && SUCCEEDED(hr = m_pD3DDevEx->ResetEx(&pp, NULL));

		if (!bTryToReset) {
			m_pD3DDevEx = NULL;

			hr = m_pD3DEx->CreateDeviceEx(
					m_CurrentAdapter, D3DDEVTYPE_HAL, m_hWnd,
					D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_ENABLE_PRESENTSTATS,
					&pp, NULL, &m_pD3DDevEx);
		}
	}

	if (m_pD3DDevEx) {
		while (hr == D3DERR_DEVICELOST) {
			TRACE("D3DERR_DEVICELOST. Trying to Reset.\n");
			hr = m_pD3DDevEx->TestCooperativeLevel();
		}
		if (hr == D3DERR_DEVICENOTRESET) {
			TRACE("D3DERR_DEVICENOTRESET\n");
			hr = m_pD3DDevEx->Reset(&pp);
		}

		if (m_pD3DDevEx) {
			m_pD3DDevEx->SetGPUThreadPriority(7);
		}
	}

	if (FAILED(hr)) {
		_Error.AppendFormat(L"CreateDevice() failed: %s\n", GetWindowsErrorMessage(hr, m_hD3D9));
		return hr;
	}

	m_pPSC.Attach(DNew CPixelShaderCompiler(m_pD3DDevEx, true));

	if (m_caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MINFLINEAR && m_caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MAGFLINEAR) {
		m_filter = D3DTEXF_LINEAR;
	} else {
		m_filter = D3DTEXF_NONE;
	}

	InitMaxSubtitleTextureSize(GetRenderersSettings().iSubpicMaxTexWidth, m_ScreenSize);

	if (m_pAllocator) {
		m_pAllocator->ChangeDevice(m_pD3DDevEx);
	} else {
		m_pAllocator = DNew CDX9SubPicAllocator(m_pD3DDevEx, m_maxSubtitleTextureSize, false);
		if (!m_pAllocator) {
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
		m_pSubPicQueue = GetRenderersSettings().nSubpicCount > 0
						 ? (ISubPicQueue*)DNew CSubPicQueue(GetRenderersSettings().nSubpicCount, !GetRenderersSettings().bSubpicAnimationWhenBuffering, GetRenderersSettings().bSubpicAllowDrop, m_pAllocator, &hr)
						 : (ISubPicQueue*)DNew CSubPicQueueNoThread(!GetRenderersSettings().bSubpicAnimationWhenBuffering, m_pAllocator, &hr);
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

	if (m_pD3DXCreateFont) {
		int MinSize = 1600;
		int CurrentSize = min(m_ScreenSize.cx, MinSize);
		double Scale = double(CurrentSize) / double(MinSize);
		m_TextScale = Scale;
		m_pD3DXCreateFont(m_pD3DDevEx, (int)(-24.0*Scale), (UINT)(-11.0*Scale), CurrentSize < 800 ? FW_NORMAL : FW_BOLD, 0, FALSE,
						  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Lucida Console", &m_pFont);
	}
	if (m_pD3DXCreateSprite) {
		m_pD3DXCreateSprite(m_pD3DDevEx, &m_pSprite);
	}
	if (m_pD3DXCreateLine) {
		m_pD3DXCreateLine (m_pD3DDevEx, &m_pLine);
	}
	m_LastAdapterCheck = GetPerfCounter();
	return S_OK;
}

HRESULT CBaseAP::AllocSurfaces(D3DFORMAT Format)
{
	CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_allocatorLock);

	CRenderersSettings& rs = GetRenderersSettings();

	for (int i = 0; i < m_nSurface+2; i++) {
		m_pVideoTextures[i] = NULL;
		m_pVideoSurfaces[i] = NULL;
	}
	m_pRotateTexture = NULL;
	m_pRotateSurface = NULL;

	m_pScreenSizeTextures[0] = NULL;
	m_pScreenSizeTextures[1] = NULL;
	m_SurfaceFmt = Format;

	HRESULT hr;
	if (rs.iSurfaceType == SURFACE_TEXTURE2D || rs.iSurfaceType == SURFACE_TEXTURE3D) {
		int nTexturesNeeded = rs.iSurfaceType == SURFACE_TEXTURE3D ? m_nSurface+2 : 1;

		for (int i = 0; i < nTexturesNeeded; i++) {
			if (FAILED(hr = m_pD3DDevEx->CreateTexture(
								m_nativeVideoSize.cx, m_nativeVideoSize.cy, 1, D3DUSAGE_RENDERTARGET, Format, D3DPOOL_DEFAULT, &m_pVideoTextures[i], NULL))) {
				return hr;
			}

			if (FAILED(hr = m_pVideoTextures[i]->GetSurfaceLevel(0, &m_pVideoSurfaces[i]))) {
				return hr;
			}
		}

		if (rs.iSurfaceType == SURFACE_TEXTURE3D) {
			UINT a = max(m_nativeVideoSize.cx, m_nativeVideoSize.cy);
			if (FAILED(hr = m_pD3DDevEx->CreateTexture(
				a, a, 1, D3DUSAGE_RENDERTARGET, Format, D3DPOOL_DEFAULT, &m_pRotateTexture, NULL))) {
				return hr;
			}
			if (FAILED(hr = m_pRotateTexture->GetSurfaceLevel(0, &m_pRotateSurface))) {
				return hr;
			}
		}

		if (rs.iSurfaceType == SURFACE_TEXTURE2D) {
			for (int i = 0; i < m_nSurface+2; i++) {
				m_pVideoTextures[i] = NULL;
			}
		}
	} else {
		if (FAILED(hr = m_pD3DDevEx->CreateOffscreenPlainSurface(m_nativeVideoSize.cx, m_nativeVideoSize.cy, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pVideoSurfaces[m_iCurSurface], NULL))) {
			return hr;
		}
	}

	hr = m_pD3DDevEx->ColorFill(m_pVideoSurfaces[m_iCurSurface], NULL, 0);
	return S_OK;
}

void CBaseAP::DeleteSurfaces()
{
	CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_allocatorLock);

	for (int i = 0; i < m_nSurface+2; i++) {
		m_pVideoTextures[i] = NULL;
		m_pVideoSurfaces[i] = NULL;
	}
	m_pRotateTexture = NULL;
	m_pRotateSurface = NULL;
}

// ISubPicAllocatorPresenter3

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

HRESULT CBaseAP::InitShaderResizer(int iShader)
{
	if (iShader < 0 || iShader >= shader_count) {
		return E_INVALIDARG;
	}

	if (m_pResizerPixelShader[iShader]) {
		return S_OK;
	}

	if (m_caps.PixelShaderVersion < D3DPS_VERSION(2, 0)) {
		return E_FAIL;
	}

	LPCSTR pSrcData = NULL;
	D3D_SHADER_MACRO ShaderMacros[3] = {
		{ "Ml", m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0) ? "1" : "0" },
		{ NULL, NULL },
		{ NULL, NULL }
	};

	switch (iShader) {
	case shader_smootherstep:
		pSrcData = shader_resizer_smootherstep;
		break;
	case shader_bspline4:
		pSrcData = shader_resizer_bspline4;
		break;
	case shader_mitchell4:
		pSrcData = shader_resizer_mitchell4;
		break;
	case shader_catmull4:
		pSrcData = shader_resizer_catmull4;
		break;
	case shader_bicubic06:
		pSrcData = shader_resizer_bicubic;
		ShaderMacros[1] = { "A", "-0.6" };
		break;
	case shader_bicubic08:
		pSrcData = shader_resizer_bicubic;
		ShaderMacros[1] = { "A", "-0.8" };
		break;
	case shader_bicubic10:
		pSrcData = shader_resizer_bicubic;
		ShaderMacros[1] = { "A", "-1.0" };
		break;
	}

	if (!pSrcData) {
		return E_FAIL;
	}

	HRESULT hr = S_OK;
	CString ErrorMessage;

	hr = m_pPSC->CompileShader(pSrcData, "main", m_ShaderProfile, 0, ShaderMacros, &m_pResizerPixelShader[iShader], &ErrorMessage);

	if (hr == S_OK && m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0) && !m_pResizerPixelShader[shader_downscaling]) {
		hr = m_pPSC->CompileShader(shader_resizer_downscaling, "main", m_ShaderProfile, 0, ShaderMacros, &m_pResizerPixelShader[shader_downscaling], &ErrorMessage);
	}

	if (FAILED(hr)) {
		TRACE("%ws", ErrorMessage.GetString());
		ASSERT(0);
		return hr;
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
	for (int i = 0; i < _countof(v); i++) {
		v[i].x -= 0.5;
		v[i].y -= 0.5;
	}
	hr = m_pD3DDevEx->SetTexture(0, pTexture);
	return TextureBlt(m_pD3DDevEx, v, D3DTEXF_POINT);
}

HRESULT CBaseAP::DrawRect(DWORD _Color, DWORD _Alpha, const CRect &_Rect)
{
	DWORD Color = D3DCOLOR_ARGB(_Alpha, GetRValue(_Color), GetGValue(_Color), GetBValue(_Color));
	MYD3DVERTEX<0> v[] = {
		{float(_Rect.left), float(_Rect.top), 0.5f, 2.0f, Color},
		{float(_Rect.right), float(_Rect.top), 0.5f, 2.0f, Color},
		{float(_Rect.left), float(_Rect.bottom), 0.5f, 2.0f, Color},
		{float(_Rect.right), float(_Rect.bottom), 0.5f, 2.0f, Color},
	};
	for (int i = 0; i < _countof(v); i++) {
		v[i].x -= 0.5;
		v[i].y -= 0.5;
	}
	return DrawRectBase(m_pD3DDevEx, v);
}

HRESULT CBaseAP::TextureResize(IDirect3DTexture9* pTexture, Vector dst[4], const CRect &SrcRect, D3DTEXTUREFILTERTYPE filter)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	float w = (float)desc.Width;
	float h = (float)desc.Height;

	float dx2 = 1.0f/w;
	float dy2 = 1.0f/h;

	MYD3DVERTEX<1> v[] = {
		{dst[0].x, dst[0].y, dst[0].z, 1.0f/dst[0].z,  SrcRect.left * dx2, SrcRect.top * dy2},
		{dst[1].x, dst[1].y, dst[1].z, 1.0f/dst[1].z,  SrcRect.right * dx2, SrcRect.top * dy2},
		{dst[2].x, dst[2].y, dst[2].z, 1.0f/dst[2].z,  SrcRect.left * dx2, SrcRect.bottom * dy2},
		{dst[3].x, dst[3].y, dst[3].z, 1.0f/dst[3].z,  SrcRect.right * dx2, SrcRect.bottom * dy2},
	};
	AdjustQuad(v, 0, 0);

	hr = m_pD3DDevEx->SetTexture(0, pTexture);
	hr = m_pD3DDevEx->SetPixelShader(NULL);
	hr = TextureBlt(m_pD3DDevEx, v, filter);

	return hr;
}

HRESULT CBaseAP::TextureResizeShader(IDirect3DTexture9* pTexture, Vector dst[4], const CRect &srcRect, int iShader)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
		return E_FAIL;
	}

	const float w = sqrt(pow(dst[1].x - dst[0].x, 2) + pow(dst[1].y - dst[0].y, 2) + pow(dst[1].z - dst[0].z, 2));
	const float h = sqrt(pow(dst[2].x - dst[0].x, 2) + pow(dst[2].y - dst[0].y, 2) + pow(dst[2].z - dst[0].z, 2));
	const float rx = srcRect.Width() / w;
	const float ry = srcRect.Height() / h;

	const float dx = 1.0f/(float)desc.Width;
	const float dy = 1.0f/(float)desc.Height;
	const float tx0 = (float)srcRect.left - 0.5f;
	const float tx1 = (float)srcRect.right - 0.5f;
	const float ty0 = (float)srcRect.top - 0.5f;
	const float ty1 = (float)srcRect.bottom - 0.5f;

	MYD3DVERTEX<1> v[] = {
		{dst[0].x - 0.5f, dst[0].y -0.5f, dst[0].z, 1.0f/dst[0].z, { tx0, ty0 } },
		{dst[1].x - 0.5f, dst[1].y -0.5f, dst[1].z, 1.0f/dst[1].z, { tx1, ty0 } },
		{dst[2].x - 0.5f, dst[2].y -0.5f, dst[2].z, 1.0f/dst[2].z, { tx0, ty1 } },
		{dst[3].x - 0.5f, dst[3].y -0.5f, dst[3].z, 1.0f/dst[3].z, { tx1, ty1 } },
	};

	hr = m_pD3DDevEx->SetTexture(0, pTexture);
	if (m_pResizerPixelShader[shader_downscaling] && rx > 2.0f && ry > 2.0f) {
		float fConstData[][4] = {{dx, dy, 0, 0}, {rx, 0, 0, 0}, {ry, 0, 0, 0}};
		hr = m_pD3DDevEx->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));
		hr = m_pD3DDevEx->SetPixelShader(m_pResizerPixelShader[shader_downscaling]);
		m_wsResizer = L"Simple averaging";
	}
	else {
		float fConstData[][4] = { { dx, dy, 0, 0 }, { dx*0.5f, dy*0.5f, 0, 0 }, { dx, 0, 0, 0 }, { 0, dy, 0, 0 } };
		hr = m_pD3DDevEx->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));
		hr = m_pD3DDevEx->SetPixelShader(m_pResizerPixelShader[iShader]);
	}
	hr = TextureBlt(m_pD3DDevEx, v, D3DTEXF_POINT);
	m_pD3DDevEx->SetPixelShader(NULL);

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
			{(float)dst.left, (float)dst.top, 0.5f, 2.0f, (float)src.left / w, (float)src.top / h},
			{(float)dst.right, (float)dst.top, 0.5f, 2.0f, (float)src.right / w, (float)src.top / h},
			{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, (float)src.left / w, (float)src.bottom / h},
			{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / w, (float)src.bottom / h},
		};

		hr = m_pD3DDevEx->SetTexture(0, pTexture);

		// GetRenderState fails for devices created with D3DCREATE_PUREDEVICE
		// so we need to provide default values in case GetRenderState fails
		DWORD abe, sb, db;
		if (FAILED(m_pD3DDevEx->GetRenderState(D3DRS_ALPHABLENDENABLE, &abe)))
			abe = FALSE;
		if (FAILED(m_pD3DDevEx->GetRenderState(D3DRS_SRCBLEND, &sb)))
			sb = D3DBLEND_ONE;
		if (FAILED(m_pD3DDevEx->GetRenderState(D3DRS_DESTBLEND, &db)))
			db = D3DBLEND_ZERO;

		hr = m_pD3DDevEx->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		hr = m_pD3DDevEx->SetRenderState(D3DRS_LIGHTING, FALSE);
		hr = m_pD3DDevEx->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = m_pD3DDevEx->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		hr = m_pD3DDevEx->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src and ...
		hr = m_pD3DDevEx->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA); // ... inverse alpha channel for dst

		hr = m_pD3DDevEx->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		hr = m_pD3DDevEx->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		hr = m_pD3DDevEx->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

		hr = m_pD3DDevEx->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		hr = m_pD3DDevEx->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		hr = m_pD3DDevEx->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		hr = m_pD3DDevEx->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = m_pD3DDevEx->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		hr = m_pD3DDevEx->SetPixelShader(NULL);

		hr = m_pD3DDevEx->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
		hr = m_pD3DDevEx->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

		m_pD3DDevEx->SetTexture(0, NULL);

		m_pD3DDevEx->SetRenderState(D3DRS_ALPHABLENDENABLE, abe);
		m_pD3DDevEx->SetRenderState(D3DRS_SRCBLEND, sb);
		m_pD3DDevEx->SetRenderState(D3DRS_DESTBLEND, db);

		return S_OK;
	} while (0);
	return E_FAIL;
}

// Update the array m_pllJitter with a new vsync period. Calculate min, max and stddev.
void CBaseAP::SyncStats(LONGLONG syncTime)
{
	m_nNextJitter = (m_nNextJitter+1) % NB_JITTER;
	LONGLONG jitter = syncTime - m_llLastSyncTime;
	m_pllJitter[m_nNextJitter] = jitter;
	double syncDeviation = (m_pllJitter[m_nNextJitter] - m_fJitterMean) / 10000.0;
	if (abs(syncDeviation) > (GetDisplayCycle() / 2)) {
		m_uSyncGlitches++;
	}

	LONGLONG llJitterSum = 0;
	LONGLONG llJitterSumAvg = 0;
	for (int i=0; i<NB_JITTER; i++) {
		LONGLONG Jitter = m_pllJitter[i];
		llJitterSum += Jitter;
		llJitterSumAvg += Jitter;
	}
	m_fJitterMean = double(llJitterSumAvg) / NB_JITTER;
	double DeviationSum = 0;

	for (int i=0; i<NB_JITTER; i++) {
		double deviation = m_pllJitter[i] - m_fJitterMean;
		DeviationSum += deviation * deviation;
		LONGLONG deviationInt = std::llround(deviation);
		m_MaxJitter = max(m_MaxJitter, deviationInt);
		m_MinJitter = min(m_MinJitter, deviationInt);
	}

	m_fJitterStdDev = sqrt(DeviationSum/NB_JITTER);
	m_fAvrFps = 10000000.0/(double(llJitterSum)/NB_JITTER);
	m_llLastSyncTime = syncTime;
}

// Collect the difference between periodEnd and periodStart in an array, calculate mean and stddev.
void CBaseAP::SyncOffsetStats(LONGLONG syncOffset)
{
	m_nNextSyncOffset = (m_nNextSyncOffset+1) % NB_JITTER;
	m_pllSyncOffset[m_nNextSyncOffset] = syncOffset;

	LONGLONG AvrageSum = 0;
	for (int i=0; i<NB_JITTER; i++) {
		LONGLONG Offset = m_pllSyncOffset[i];
		AvrageSum += Offset;
		m_MaxSyncOffset = max(m_MaxSyncOffset, Offset);
		m_MinSyncOffset = min(m_MinSyncOffset, Offset);
	}
	double MeanOffset = double(AvrageSum)/NB_JITTER;
	double DeviationSum = 0;
	for (int i=0; i<NB_JITTER; i++) {
		double Deviation = double(m_pllSyncOffset[i]) - MeanOffset;
		DeviationSum += Deviation*Deviation;
	}
	double StdDev = sqrt(DeviationSum/NB_JITTER);

	m_fSyncOffsetAvr = MeanOffset;
	m_fSyncOffsetStdDev = StdDev;
}

void CBaseAP::UpdateAlphaBitmap()
{
	m_VMR9AlphaBitmapData.Free();
	m_pOSDTexture.Release();
	m_pOSDSurface.Release();

	if ((m_VMR9AlphaBitmap.dwFlags & VMRBITMAP_DISABLE) == 0) {
		HBITMAP hBitmap = (HBITMAP)GetCurrentObject (m_VMR9AlphaBitmap.hdc, OBJ_BITMAP);
		if (!hBitmap) {
			return;
		}
		DIBSECTION info = {0};
		if (!::GetObject(hBitmap, sizeof( DIBSECTION ), &info )) {
			return;
		}

		m_VMR9AlphaBitmapRect = CRect(0, 0, info.dsBm.bmWidth, info.dsBm.bmHeight);
		m_VMR9AlphaBitmapWidthBytes = info.dsBm.bmWidthBytes;

		if (m_VMR9AlphaBitmapData.Allocate(info.dsBm.bmWidthBytes * info.dsBm.bmHeight)) {
			memcpy((BYTE *)m_VMR9AlphaBitmapData, info.dsBm.bmBits, info.dsBm.bmWidthBytes * info.dsBm.bmHeight);
		}
	}
}

// Present a sample (frame) using DirectX.
STDMETHODIMP_(bool) CBaseAP::Paint(bool fAll)
{
	if (m_bPendingResetDevice) {
		SendResetRequest();
		return false;
	}

	CRenderersSettings& rs = GetRenderersSettings();
	CRenderersData *pApp = GetRenderersData();
	D3DRASTER_STATUS rasterStatus;
	REFERENCE_TIME llCurRefTime = 0;
	REFERENCE_TIME llSyncOffset = 0;
	double dSyncOffset = 0.0;

	CAutoLock cRenderLock(&m_allocatorLock);

	// Estimate time for next vblank based on number of remaining lines in this frame. This algorithm seems to be
	// accurate within one ms why there should not be any need for a more accurate one. The wiggly line seen
	// when using sync to nearest and sync display is most likely due to inaccuracies in the audio-card-based
	// reference clock. The wiggles are not seen with the perfcounter-based reference clock of the sync to video option.
	m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
	m_uScanLineEnteringPaint = rasterStatus.ScanLine;
	if (m_pRefClock) {
		m_pRefClock->GetTime(&llCurRefTime);
	}
	int dScanLines = max((int)m_ScreenSize.cy - m_uScanLineEnteringPaint, 0);
	dSyncOffset = dScanLines * m_dDetectedScanlineTime; // ms
	llSyncOffset = REFERENCE_TIME(10000.0 * dSyncOffset); // Reference time units (100 ns)
	m_llEstVBlankTime = llCurRefTime + llSyncOffset; // Estimated time for the start of next vblank

	if (m_windowRect.right <= m_windowRect.left || m_windowRect.bottom <= m_windowRect.top
			|| m_nativeVideoSize.cx <= 0 || m_nativeVideoSize.cy <= 0
			|| !m_pVideoSurfaces) {
		return false;
	}

	HRESULT hr;
	CRect rSrcVid(CPoint(0, 0), m_nativeVideoSize);
	CRect rDstVid(m_videoRect);
	CRect rSrcPri(CPoint(0, 0), m_windowRect.Size());
	CRect rDstPri(m_windowRect);

	m_pD3DDevEx->BeginScene();

	CComPtr<IDirect3DSurface9> pBackBuffer;
	m_pD3DDevEx->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	m_pD3DDevEx->SetRenderTarget(0, pBackBuffer);
	hr = m_pD3DDevEx->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	if (!rDstVid.IsRectEmpty()) {
		if (m_pVideoTextures[m_iCurSurface]) {
			CComPtr<IDirect3DTexture9> pVideoTexture = m_pVideoTextures[m_iCurSurface];

			// pre-resize pixel shaders
			if (m_pPixelShaders.GetCount()) {
				static __int64 counter = 0;
				static long start = clock();

				long stop = clock();
				long diff = stop - start;

				if (diff >= 10*60*CLOCKS_PER_SEC) {
					start = stop;    // reset after 10 min (ps float has its limits in both range and accuracy)
				}

				int src = m_iCurSurface, dst = m_nSurface;

				D3DSURFACE_DESC desc;
				m_pVideoTextures[src]->GetLevelDesc(0, &desc);

				float fConstData[][4] = {
					{(float)desc.Width, (float)desc.Height, (float)(counter++), (float)diff / CLOCKS_PER_SEC},
					{1.0f / desc.Width, 1.0f / desc.Height, 0, 0},
				};
				hr = m_pD3DDevEx->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));

				POSITION pos = m_pPixelShaders.GetHeadPosition();
				while (pos) {
					pVideoTexture = m_pVideoTextures[dst];

					hr = m_pD3DDevEx->SetRenderTarget(0, m_pVideoSurfaces[dst]);
					CExternalPixelShader &Shader = m_pPixelShaders.GetNext(pos);
					if (!Shader.m_pPixelShader) {
						Shader.Compile(m_pPSC);
					}
					hr = m_pD3DDevEx->SetPixelShader(Shader.m_pPixelShader);
					TextureCopy(m_pVideoTextures[src]);

					src = dst;
					if (++dst >= m_nSurface+2) {
						dst = m_nSurface;
					}
				}

				hr = m_pD3DDevEx->SetRenderTarget(0, pBackBuffer);
				hr = m_pD3DDevEx->SetPixelShader(NULL);
			}

			Vector dst[4];
			if (m_iRotation) {
				switch (m_iRotation) {
				case 90:
					dst[0].Set((float)rSrcVid.right, (float)rSrcVid.top,    0.5f);
					dst[1].Set((float)rSrcVid.right, (float)rSrcVid.bottom, 0.5f);
					dst[2].Set((float)rSrcVid.left,  (float)rSrcVid.top,    0.5f);
					dst[3].Set((float)rSrcVid.left,  (float)rSrcVid.bottom, 0.5f);
					hr = m_pD3DDevEx->SetRenderTarget(0, m_pRotateSurface);
					break;
				case 180:
					dst[0].Set((float)rSrcVid.right, (float)rSrcVid.bottom, 0.5f);
					dst[1].Set((float)rSrcVid.left,  (float)rSrcVid.bottom, 0.5f);
					dst[2].Set((float)rSrcVid.right, (float)rSrcVid.top,    0.5f);
					dst[3].Set((float)rSrcVid.left,  (float)rSrcVid.top,    0.5f);
					hr = m_pD3DDevEx->SetRenderTarget(0, m_pVideoSurfaces[m_nSurface + 1]);
					break;
				case 270:
					dst[0].Set((float)rSrcVid.left,  (float)rSrcVid.bottom, 0.5f);
					dst[1].Set((float)rSrcVid.left,  (float)rSrcVid.top,    0.5f);
					dst[2].Set((float)rSrcVid.right, (float)rSrcVid.bottom, 0.5f);
					dst[3].Set((float)rSrcVid.right, (float)rSrcVid.top,    0.5f);
					hr = m_pD3DDevEx->SetRenderTarget(0, m_pRotateSurface);
					break;
				}
				MYD3DVERTEX<1> v[] = {
					{ dst[0].x, dst[0].y, 0.5f, 2.0f, 0.0f, 0.0f },
					{ dst[1].x, dst[1].y, 0.5f, 2.0f, 1.0f, 0.0f },
					{ dst[2].x, dst[2].y, 0.5f, 2.0f, 0.0f, 1.0f },
					{ dst[3].x, dst[3].y, 0.5f, 2.0f, 1.0f, 1.0f },
				};
				AdjustQuad(v, 0, 0);
				hr = m_pD3DDevEx->SetTexture(0, pVideoTexture);
				hr = m_pD3DDevEx->SetPixelShader(NULL);
				hr = TextureBlt(m_pD3DDevEx, v, D3DTEXF_LINEAR);

				if (m_iRotation == 180) {
					pVideoTexture = m_pVideoTextures[m_nSurface + 1];
				} else { // 90 and 270
					pVideoTexture = m_pRotateTexture;
				}

				m_pD3DDevEx->SetRenderTarget(0, pBackBuffer);
			}

			Transform(rDstVid, dst);

			// init resizer
			DWORD iResizer = rs.iResizer;
			hr = E_FAIL;

			switch (iResizer) {
			case RESIZER_NEAREST:
			case RESIZER_BILINEAR:
				hr = S_OK;
				break;
			case RESIZER_SHADER_SMOOTHERSTEP: hr = InitShaderResizer(shader_smootherstep); break;
			case RESIZER_SHADER_BSPLINE4:  hr = InitShaderResizer(shader_bspline4);  break;
			case RESIZER_SHADER_MITCHELL4: hr = InitShaderResizer(shader_mitchell4); break;
			case RESIZER_SHADER_CATMULL4:  hr = InitShaderResizer(shader_catmull4);  break;
			case RESIZER_SHADER_BICUBIC06: hr = InitShaderResizer(shader_bicubic06); break;
			case RESIZER_SHADER_BICUBIC08: hr = InitShaderResizer(shader_bicubic08); break;
			case RESIZER_SHADER_BICUBIC10: hr = InitShaderResizer(shader_bicubic10); break;
			}

			if (FAILED(hr)) {
				iResizer = RESIZER_BILINEAR;
			}

			// init post-resize pixel shaders
			bool bScreenSpacePixelShaders = m_pPixelShadersScreenSpace.GetCount() > 0;
			if (bScreenSpacePixelShaders && (!m_pScreenSizeTextures[0] || !m_pScreenSizeTextures[1])) {
				UINT texWidth = min((DWORD)m_ScreenSize.cx, m_caps.MaxTextureWidth);
				UINT texHeight = min((DWORD)m_ScreenSize.cy, m_caps.MaxTextureHeight);

				if (D3D_OK != m_pD3DDevEx->CreateTexture(
							texWidth, texHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
							D3DPOOL_DEFAULT, &m_pScreenSizeTextures[0], NULL)
					|| D3D_OK != m_pD3DDevEx->CreateTexture(
							texWidth, texHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
							D3DPOOL_DEFAULT, &m_pScreenSizeTextures[1], NULL)) {
					ASSERT(0);
					m_pScreenSizeTextures[0] = NULL;
					m_pScreenSizeTextures[1] = NULL;
					bScreenSpacePixelShaders = false; // will do 1 pass then
				}
			}

			if (bScreenSpacePixelShaders) {
				CComPtr<IDirect3DSurface9> pRT;
				hr = m_pScreenSizeTextures[1]->GetSurfaceLevel(0, &pRT);
				if (hr != S_OK) {
					bScreenSpacePixelShaders = false;
				}
				if (bScreenSpacePixelShaders) {
					hr = m_pD3DDevEx->SetRenderTarget(0, pRT);
					if (hr != S_OK) {
						bScreenSpacePixelShaders = false;
					}
					hr = m_pD3DDevEx->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
				}
			}

			// resize
			if (rSrcVid.Size() != rDstVid.Size()) {
				switch (iResizer) {
				case RESIZER_NEAREST:
					m_wsResizer = L"Nearest neighbor";
					hr = TextureResize(pVideoTexture, dst, rSrcVid, D3DTEXF_POINT);
					break;
				case RESIZER_BILINEAR:
					m_wsResizer = L"Bilinear";
					hr = TextureResize(pVideoTexture, dst, rSrcVid, D3DTEXF_LINEAR);
					break;
				case RESIZER_SHADER_SMOOTHERSTEP:
					m_wsResizer = L"Perlin Smootherstep";
					hr = TextureResizeShader(pVideoTexture, dst, rSrcVid, shader_smootherstep);
					break;
				case RESIZER_SHADER_BSPLINE4:
					m_wsResizer = L"B-spline4";
					hr = TextureResizeShader(pVideoTexture, dst, rSrcVid, shader_bspline4);
					break;
				case RESIZER_SHADER_MITCHELL4:
					m_wsResizer = L"Mitchell-Netravali spline4";
					hr = TextureResizeShader(pVideoTexture, dst, rSrcVid, shader_mitchell4);
					break;
				case RESIZER_SHADER_CATMULL4:
					m_wsResizer = L"Catmull-Rom spline4";
					hr = TextureResizeShader(pVideoTexture, dst, rSrcVid, shader_catmull4);
					break;
				case RESIZER_SHADER_BICUBIC06:
					m_wsResizer = L"Bicubic A=-0.6";
					hr = TextureResizeShader(pVideoTexture, dst, rSrcVid, shader_bicubic06);
					break;
				case RESIZER_SHADER_BICUBIC08:
					m_wsResizer = L"Bicubic A=-0.8";
					hr = TextureResizeShader(pVideoTexture, dst, rSrcVid, shader_bicubic08);
					break;
				case RESIZER_SHADER_BICUBIC10:
					m_wsResizer = L"Bicubic A=-1.0";
					hr = TextureResizeShader(pVideoTexture, dst, rSrcVid, shader_bicubic10);
					break;
				}
			} else {
				m_wsResizer = L""; // empty string, not nullptr
				hr = TextureResize(pVideoTexture, dst, rSrcVid, D3DTEXF_POINT);
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

				hr = m_pD3DDevEx->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));

				int src = 1, dst = 0;

				POSITION pos = m_pPixelShadersScreenSpace.GetHeadPosition();
				while (pos) {
					if (m_pPixelShadersScreenSpace.GetTailPosition() == pos) {
						m_pD3DDevEx->SetRenderTarget(0, pBackBuffer);
					} else {
						CComPtr<IDirect3DSurface9> pRT;
						hr = m_pScreenSizeTextures[dst]->GetSurfaceLevel(0, &pRT);
						m_pD3DDevEx->SetRenderTarget(0, pRT);
					}

					CExternalPixelShader &Shader = m_pPixelShadersScreenSpace.GetNext(pos);
					if (!Shader.m_pPixelShader) {
						Shader.Compile(m_pPSC);
					}
					hr = m_pD3DDevEx->SetPixelShader(Shader.m_pPixelShader);
					TextureCopy(m_pScreenSizeTextures[src]);

					std::swap(src, dst);
				}

				hr = m_pD3DDevEx->SetPixelShader(NULL);
			}
		} else {
			if (pBackBuffer) {
				ClipToSurface(pBackBuffer, rSrcVid, rDstVid);
				// rSrcVid has to be aligned on mod2 for yuy2->rgb conversion with StretchRect
				rSrcVid.left &= ~1;
				rSrcVid.right &= ~1;
				rSrcVid.top &= ~1;
				rSrcVid.bottom &= ~1;
				hr = m_pD3DDevEx->StretchRect(m_pVideoSurfaces[m_iCurSurface], rSrcVid, pBackBuffer, rDstVid, m_filter);
				if (FAILED(hr)) {
					return false;
				}
			}
		}
	}

	AlphaBltSubPic(rDstPri, rDstVid);

	if (m_VMR9AlphaBitmap.dwFlags & VMRBITMAP_UPDATE) {
		CAutoLock BitMapLock(&m_VMR9AlphaBitmapLock);
		CRect rcSrc(m_VMR9AlphaBitmap.rSrc);
		m_pOSDTexture.Release();
		m_pOSDSurface.Release();
		if ((m_VMR9AlphaBitmap.dwFlags & VMRBITMAP_DISABLE) == 0 && (BYTE *)m_VMR9AlphaBitmapData) {
			if ( (m_pD3DXLoadSurfaceFromMemory != NULL) &&
					SUCCEEDED(hr = m_pD3DDevEx->CreateTexture(rcSrc.Width(), rcSrc.Height(), 1,
								   D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
								   D3DPOOL_DEFAULT, &m_pOSDTexture, NULL)) ) {
				if (SUCCEEDED (hr = m_pOSDTexture->GetSurfaceLevel(0, &m_pOSDSurface))) {
					hr = m_pD3DXLoadSurfaceFromMemory (m_pOSDSurface, NULL, NULL, (BYTE *)m_VMR9AlphaBitmapData, D3DFMT_A8R8G8B8, m_VMR9AlphaBitmapWidthBytes,
													   NULL, &m_VMR9AlphaBitmapRect, D3DX_FILTER_NONE, m_VMR9AlphaBitmap.clrSrcKey);
				}
				if (FAILED (hr)) {
					m_pOSDTexture.Release();
					m_pOSDSurface.Release();
				}
			}
		}
		m_VMR9AlphaBitmap.dwFlags ^= VMRBITMAP_UPDATE;
	}
	if (pApp->m_iDisplayStats) {
		DrawStats();
	}
	if (m_pOSDTexture) {
		AlphaBlt(rSrcPri, rDstPri, m_pOSDTexture);
	}

	m_pD3DDevEx->EndScene();

	if (m_pD3DDevEx) {
		if (m_bIsFullscreen) {
			hr = m_pD3DDevEx->PresentEx(NULL, NULL, NULL, NULL, NULL);
		} else {
			hr = m_pD3DDevEx->PresentEx(rSrcPri, rDstPri, NULL, NULL, NULL);
		}
	}
	if (FAILED(hr)) {
		DEBUG_ONLY(_tprintf_s(_T("Device lost or something\n")));
	}

	// Calculate timing statistics
	if (m_pRefClock) {
		m_pRefClock->GetTime(&llCurRefTime);    // To check if we called Present too late to hit the right vsync
	}
	m_llEstVBlankTime = max(m_llEstVBlankTime, llCurRefTime); // Sometimes the real value is larger than the estimated value (but never smaller)
	if (pApp->m_iDisplayStats < 3) { // Partial on-screen statistics
		SyncStats(m_llEstVBlankTime);    // Max of estimate and real. Sometimes Present may actually return immediately so we need the estimate as a lower bound
	}
	if (pApp->m_iDisplayStats == 1) { // Full on-screen statistics
		SyncOffsetStats(-llSyncOffset);    // Minus because we want time to flow downward in the graph in DrawStats
	}

	// Adjust sync
	double frameCycle = (m_llSampleTime - m_llLastSampleTime) / 10000.0;
	if (frameCycle < 0) {
		frameCycle = 0.0;    // Happens when searching.
	}

	if (rs.m_AdvRendSets.iSynchronizeMode == SYNCHRONIZE_VIDEO) {
		m_pGenlock->ControlClock(dSyncOffset, frameCycle);
	} else if (rs.m_AdvRendSets.iSynchronizeMode == SYNCHRONIZE_DISPLAY) {
		m_pGenlock->ControlDisplay(dSyncOffset, frameCycle);
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
	if (m_pAudioStats != NULL) {
		m_pAudioStats->GetStatParam(AM_AUDREND_STAT_PARAM_SLAVE_ACCUMERROR, &m_lAudioLag, &tmp);
		m_lAudioLagMin = min((long)m_lAudioLag, m_lAudioLagMin);
		m_lAudioLagMax = max((long)m_lAudioLag, m_lAudioLagMax);
		m_pAudioStats->GetStatParam(AM_AUDREND_STAT_PARAM_SLAVE_MODE, &m_lAudioSlaveMode, &tmp);
	}

	if (pApp->m_bResetStats) {
		ResetStats();
		pApp->m_bResetStats = false;
	}

	bool bResetDevice = m_bPendingResetDevice;
	if (hr == D3DERR_DEVICELOST && m_pD3DDevEx->TestCooperativeLevel() == D3DERR_DEVICENOTRESET || hr == S_PRESENT_MODE_CHANGED) {
		bResetDevice = true;
	}
	if (SettingsNeedResetDevice()) {
		bResetDevice = true;
	}

	BOOL bCompositionEnabled = false;
	if (m_pDwmIsCompositionEnabled) {
		m_pDwmIsCompositionEnabled(&bCompositionEnabled);
	}
	if ((bCompositionEnabled != 0) != m_bCompositionEnabled) {
		if (m_bIsFullscreen) {
			m_bCompositionEnabled = (bCompositionEnabled != 0);
		} else {
			bResetDevice = true;
		}
	}

	if (rs.bResetDevice) {
		LONGLONG time = GetPerfCounter();
		if (time > m_LastAdapterCheck + 20000000) { // check every 2 sec.
			m_LastAdapterCheck = time;
#ifdef _DEBUG
			D3DDEVICE_CREATION_PARAMETERS Parameters;
			if (SUCCEEDED(m_pD3DDevEx->GetCreationParameters(&Parameters))) {
				ASSERT(Parameters.AdapterOrdinal == m_CurrentAdapter);
			}
#endif
			if (m_CurrentAdapter != GetAdapter(m_pD3DEx, m_hWnd)) {
				bResetDevice = true;
			}
#ifdef _DEBUG
			else {
				ASSERT(m_pD3DEx->GetAdapterMonitor(m_CurrentAdapter) == m_pD3DEx->GetAdapterMonitor(GetAdapter(m_pD3DEx, m_hWnd)));
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
	m_pGenlock->SetMonitor(GetAdapter(m_pD3DEx, m_hWnd));
	m_pGenlock->GetTiming();
	OnResetDevice();
	m_bDeviceResetRequested = false;
	m_bPendingResetDevice = false;
	return true;
}

STDMETHODIMP_(bool) CBaseAP::DisplayChange()
{
	return true;
}

// ISubRenderOptions

STDMETHODIMP CBaseAP::GetInt(LPCSTR field, int* value)
{
	CheckPointer(value, E_POINTER);
	if (strcmp(field, "rotation") == 0) {
		*value = m_iRotation;

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


void CBaseAP::DrawText(const RECT &rc, const CString &strText, int _Priority)
{
	if (_Priority < 1) {
		return;
	}
	int Quality = 1;
	//D3DXCOLOR Color1(1.0f, 0.2f, 0.2f, 1.0f); // red
	//D3DXCOLOR Color1(1.0f, 1.0f, 1.0f, 1.0f); // white
	D3DXCOLOR Color1(1.0f, 0.8f, 0.0f, 1.0f); // yellow
	D3DXCOLOR Color0(0.0f, 0.0f, 0.0f, 1.0f); // black
	RECT Rect1 = rc;
	RECT Rect2 = rc;
	if (Quality == 1) {
		OffsetRect(&Rect2 , 2, 2);
	} else {
		OffsetRect(&Rect2 , -1, -1);
	}
	if (Quality > 0) {
		m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	}
	OffsetRect (&Rect2 , 1, 0);
	if (Quality > 3) {
		m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	}
	OffsetRect (&Rect2 , 1, 0);
	if (Quality > 2) {
		m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	}
	OffsetRect (&Rect2 , 0, 1);
	if (Quality > 3) {
		m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	}
	OffsetRect (&Rect2 , 0, 1);
	if (Quality > 1) {
		m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	}
	OffsetRect (&Rect2 , -1, 0);
	if (Quality > 3) {
		m_pFont->DrawText( m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	}
	OffsetRect (&Rect2 , -1, 0);
	if (Quality > 2) {
		m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	}
	OffsetRect (&Rect2 , 0, -1);
	if (Quality > 3) {
		m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
	}
	m_pFont->DrawText( m_pSprite, strText, -1, &Rect1, DT_NOCLIP, Color1);
}

void CBaseAP::DrawStats()
{
	CRenderersSettings& rs = GetRenderersSettings();
	CRenderersData * pApp = GetRenderersData();

	LONGLONG llMaxJitter = m_MaxJitter;
	LONGLONG llMinJitter = m_MinJitter;

	RECT rc = {20, 20, 520, 520 };
	// pApp->m_iDisplayStats = 1 for full stats, 2 for little less, 3 for basic, 0 for no stats
	if (m_pFont && m_pSprite) {
		m_pSprite->Begin(D3DXSPRITE_ALPHABLEND);
		CString strText;
		int TextHeight = (int)(25.0*m_TextScale + 0.5);

		strText.Format(L"Frames drawn from stream start: %d | Sample time stamp: %d ms", m_pcFramesDrawn, (LONG)(m_llSampleTime / 10000));
		DrawText(rc, strText, 1);
		OffsetRect(&rc, 0, TextHeight);

		if (pApp->m_iDisplayStats == 1) {
			strText.Format(L"Frame cycle  : %.3f ms [%.3f ms, %.3f ms]  Actual  %+5.3f ms [%+.3f ms, %+.3f ms]", m_dFrameCycle, m_pGenlock->minFrameCycle, m_pGenlock->maxFrameCycle, m_fJitterMean / 10000.0, (double(llMinJitter)/10000.0), (double(llMaxJitter)/10000.0));
			DrawText(rc, strText, 1);
			OffsetRect(&rc, 0, TextHeight);

			strText.Format(L"Display cycle: Measured closest match %.3f ms   Measured base %.3f ms", m_dOptimumDisplayCycle, m_dEstRefreshCycle);
			DrawText(rc, strText, 1);
			OffsetRect(&rc, 0, TextHeight);

			strText.Format(L"Frame rate   : %.3f fps   Actual frame rate: %.3f fps", m_fps, 10000000.0 / m_fJitterMean);
			DrawText(rc, strText, 1);
			OffsetRect(&rc, 0, TextHeight);

			strText.Format(L"Display      : %d x %d, %d Hz  Cycle %.3f ms", m_ScreenSize.cx, m_ScreenSize.cy, m_refreshRate, m_dD3DRefreshCycle);
			DrawText(rc, strText, 1);
			OffsetRect(&rc, 0, TextHeight);

			if (m_pGenlock->powerstripTimingExists) {
				strText.Format(L"Powerstrip   : Display cycle %.3f ms    Display refresh rate %.3f Hz", 1000.0 / m_pGenlock->curDisplayFreq, m_pGenlock->curDisplayFreq);
				DrawText(rc, strText, 1);
				OffsetRect(&rc, 0, TextHeight);
			}

			if ((m_caps.Caps & D3DCAPS_READ_SCANLINE) == 0) {
				strText = L"Scan line err: Graphics device does not support scan line access. No sync is possible";
				DrawText(rc, strText, 1);
				OffsetRect(&rc, 0, TextHeight);
			}

#ifdef _DEBUG
			if (m_pD3DDevEx) {
				CComPtr<IDirect3DSwapChain9> pSC;
				HRESULT hr = m_pD3DDevEx->GetSwapChain(0, &pSC);
				CComQIPtr<IDirect3DSwapChain9Ex> pSCEx = pSC;
				if (pSCEx) {
					D3DPRESENTSTATS stats;
					hr = pSCEx->GetPresentStats(&stats);
					if (SUCCEEDED(hr)) {
						strText = L"Graphics device present stats:";
						DrawText(rc, strText, 1);
						OffsetRect(&rc, 0, TextHeight);

						strText.Format(L"    PresentCount %d PresentRefreshCount %d SyncRefreshCount %d",
									   stats.PresentCount, stats.PresentRefreshCount, stats.SyncRefreshCount);
						DrawText(rc, strText, 1);
						OffsetRect(&rc, 0, TextHeight);

						LARGE_INTEGER Freq;
						QueryPerformanceFrequency (&Freq);
						Freq.QuadPart /= 1000;
						strText.Format(L"    SyncQPCTime %dms SyncGPUTime %dms",
									   stats.SyncQPCTime.QuadPart / Freq.QuadPart, stats.SyncGPUTime.QuadPart / Freq.QuadPart);
						DrawText(rc, strText, 1);
						OffsetRect(&rc, 0, TextHeight);
					} else {
						strText = L"Graphics device does not support present stats";
						DrawText(rc, strText, 1);
						OffsetRect(&rc, 0, TextHeight);
					}
				}
			}
#endif

			strText.Format(L"Video size   : %d x %d (%d:%d)", m_nativeVideoSize.cx, m_nativeVideoSize.cy, m_aspectRatio.cx, m_aspectRatio.cy);
			CSize videoSize = m_videoRect.Size();
			if (m_nativeVideoSize != videoSize) {
				strText.AppendFormat(L" -> %d x %d %s", videoSize.cx, videoSize.cy, m_wsResizer);
			}
			DrawText(rc, strText, 1);
			OffsetRect(&rc, 0, TextHeight);

			if (rs.m_AdvRendSets.iSynchronizeMode != SYNCHRONIZE_NEAREST) {
				if (rs.m_AdvRendSets.iSynchronizeMode == SYNCHRONIZE_DISPLAY && !m_pGenlock->PowerstripRunning()) {
					strText = L"Sync error   : PowerStrip is not running. No display sync is possible.";
					DrawText(rc, strText, 1);
					OffsetRect(&rc, 0, TextHeight);
				} else {
					strText.Format(L"Sync adjust  : %d | # of adjustments: %d", m_pGenlock->adjDelta, (m_pGenlock->clockAdjustmentsMade + m_pGenlock->displayAdjustmentsMade) / 2);
					DrawText(rc, strText, 1);
					OffsetRect(&rc, 0, TextHeight);
				}
			}
		}

		strText.Format(L"Sync offset  : Average %3.1f ms [%.1f ms, %.1f ms]   Target %3.1f ms", m_pGenlock->syncOffsetAvg, m_pGenlock->minSyncOffset, m_pGenlock->maxSyncOffset, rs.m_AdvRendSets.dTargetSyncOffset);
		DrawText(rc, strText, 1);
		OffsetRect(&rc, 0, TextHeight);

		strText.Format(L"Sync status  : glitches %d,  display-frame cycle mismatch: %7.3f %%,  dropped frames %d", m_uSyncGlitches, 100 * m_dCycleDifference, m_pcFramesDropped);
		DrawText(rc, strText, 1);
		OffsetRect(&rc, 0, TextHeight);

		if (pApp->m_iDisplayStats == 1) {
			if (m_pAudioStats && rs.m_AdvRendSets.iSynchronizeMode == SYNCHRONIZE_VIDEO) {
				strText.Format(L"Audio lag   : %3d ms [%d ms, %d ms] | %s", m_lAudioLag, m_lAudioLagMin, m_lAudioLagMax, (m_lAudioSlaveMode == 4) ? L"Audio renderer is matching rate (for analog sound output)" : L"Audio renderer is not matching rate");
				DrawText(rc, strText, 1);
				OffsetRect(&rc, 0, TextHeight);
			}

			strText.Format(L"Sample time  : waiting %3d ms", m_lNextSampleWait);
			if (rs.m_AdvRendSets.iSynchronizeMode == SYNCHRONIZE_NEAREST) {
				CString temp;
				temp.Format(L"  paint time correction: %3d ms  Hysteresis: %d", m_lShiftToNearest, m_llHysteresis /10000);
				strText += temp;
			}
			DrawText(rc, strText, 1);
			OffsetRect(&rc, 0, TextHeight);

			strText.Format(L"Buffering    : Buffered %3d    Free %3d    Current Surface %3d", m_nUsedBuffer, m_nSurface - m_nUsedBuffer, m_iCurSurface);
			DrawText(rc, strText, 1);
			OffsetRect(&rc, 0, TextHeight);

			strText = L"Settings     : ";

			if (m_bIsFullscreen) {
				strText += L"D3DFS ";
			}
			if (rs.m_AdvRendSets.bDisableDesktopComposition) {
				strText += L"DisDC ";
			}
			if (rs.m_AdvRendSets.iSynchronizeMode == SYNCHRONIZE_VIDEO) {
				strText += L"SyncVideo ";
			} else if (rs.m_AdvRendSets.iSynchronizeMode == SYNCHRONIZE_DISPLAY) {
				strText += L"SyncDisplay ";
			} else if (rs.m_AdvRendSets.iSynchronizeMode == SYNCHRONIZE_NEAREST) {
				strText += L"SyncNearest ";
			}
			if (m_b10BitOutput) {
				strText += L"10 bit ";
			}
			if (rs.m_AdvRendSets.iEVROutputRange == 0) {
				strText += L"0-255 ";
			} else if (rs.m_AdvRendSets.iEVROutputRange == 1) {
				strText += L"16-235 ";
			}

			DrawText(rc, strText, 1);
			OffsetRect(&rc, 0, TextHeight);

			strText.Format(L"%-13s: %s", GetDXVAVersion(), GetDXVADecoderDescription());
			DrawText(rc, strText, 1);
			OffsetRect(&rc, 0, TextHeight);

			for (int i=0; i<6; i++) {
				if (m_strStatsMsg[i][0]) {
					DrawText(rc, m_strStatsMsg[i], 1);
					OffsetRect(&rc, 0, TextHeight);
				}
			}
		}
		OffsetRect(&rc, 0, TextHeight); // Extra "line feed"
		m_pSprite->End();
	}

	if (m_pLine && (pApp->m_iDisplayStats < 3)) {
		D3DXVECTOR2 Points[NB_JITTER];
		int nIndex;

		int DrawWidth = 625;
		int DrawHeight = 250;
		int Alpha = 80;
		int StartX = m_windowRect.Width() - (DrawWidth + 20);
		int StartY = m_windowRect.Height() - (DrawHeight + 20);

		DrawRect(RGB(0, 0, 0), Alpha, CRect(StartX, StartY, StartX + DrawWidth, StartY + DrawHeight));
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
			double Jitter = m_pllJitter[nIndex] - m_fJitterMean;
			Points[i].x  = (FLOAT)(StartX + (i * 5));
			Points[i].y  = (FLOAT)(StartY + (Jitter / 2000.0 + 125.0));
		}
		m_pLine->Draw(Points, NB_JITTER, D3DCOLOR_XRGB(255, 100, 100));

		if (pApp->m_iDisplayStats == 1) { // Full on-screen statistics
			for (int i = 0; i < NB_JITTER; i++) {
				nIndex = (m_nNextSyncOffset + 1 + i) % NB_JITTER;
				if (nIndex < 0) {
					nIndex += NB_JITTER;
				}
				Points[i].x  = (FLOAT)(StartX + (i * 5));
				Points[i].y  = (FLOAT)(StartY + ((m_pllSyncOffset[nIndex]) / 2000 + 125));
			}
			m_pLine->Draw(Points, NB_JITTER, D3DCOLOR_XRGB(100, 200, 100));
		}
		m_pLine->End();
	}
}

double CBaseAP::GetRefreshRate()
{
	if (m_pGenlock->powerstripTimingExists) {
		return m_pGenlock->curDisplayFreq;
	} else {
		return (double)m_refreshRate;
	}
}

double CBaseAP::GetDisplayCycle()
{
	if (m_pGenlock->powerstripTimingExists) {
		return 1000.0 / m_pGenlock->curDisplayFreq;
	} else {
		return (double)m_dD3DRefreshCycle;
	}
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
	if (m_pD3DDevEx) {
		D3DRASTER_STATUS rasterStatus;
		m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
		while (rasterStatus.ScanLine != 0) {
			m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
		}
		while (rasterStatus.ScanLine == 0) {
			m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
		}
		m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
		LONGLONG startTime = GetPerfCounter();
		UINT startLine = rasterStatus.ScanLine;
		LONGLONG endTime = 0;
		LONGLONG time = 0;
		UINT endLine = 0;
		UINT line = 0;
		bool done = false;
		while (!done) { // Estimate time for one scan line
			m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
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
		m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
		while (rasterStatus.ScanLine != 0) {
			m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
		}
		// Now we're at the start of a vsync
		startTime = GetPerfCounter();
		UINT i;
		for (i = 1; i <= 50; i++) {
			m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
			while (rasterStatus.ScanLine == 0) {
				m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
			}
			while (rasterStatus.ScanLine != 0) {
				m_pD3DDevEx->GetRasterStatus(0, &rasterStatus);
			}
			// Now we're at the next vsync
		}
		endTime = GetPerfCounter();
		m_dEstRefreshCycle = (endTime - startTime) / ((i - 1) * 10000.0);
	}
}

bool CBaseAP::ExtractInterlaced(const AM_MEDIA_TYPE* pmt)
{
	if (pmt->formattype==FORMAT_VideoInfo) {
		return false;
	} else if (pmt->formattype==FORMAT_VideoInfo2) {
		return (((VIDEOINFOHEADER2*)pmt->pbFormat)->dwInterlaceFlags & AMINTERLACE_IsInterlaced) != 0;
	} else if (pmt->formattype==FORMAT_MPEGVideo) {
		return false;
	} else if (pmt->formattype==FORMAT_MPEG2Video) {
		return (((MPEG2VIDEOINFO*)pmt->pbFormat)->hdr.dwInterlaceFlags & AMINTERLACE_IsInterlaced) != 0;
	} else {
		return false;
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
	if (FAILED(hr = pSurface->LockRect(&r, NULL, D3DLOCK_READONLY))) {
		pSurface = NULL;
		if (FAILED(hr = m_pD3DDevEx->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pSurface, NULL))
				|| FAILED(hr = m_pD3DDevEx->GetRenderTargetData(m_pVideoSurfaces[m_iCurSurface], pSurface))
				|| FAILED(hr = pSurface->LockRect(&r, NULL, D3DLOCK_READONLY))) {
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

	uint32_t* p = NULL;
	if (m_iRotation) {
		p = DNew uint32_t[bih->biWidth * bih->biHeight];
	}

	BitBltFromRGBToRGB(
		bih->biWidth, bih->biHeight,
		p ? (BYTE*)p : (BYTE*)(bih + 1), bih->biWidth*bih->biBitCount>>3, bih->biBitCount,
		(BYTE*)r.pBits + r.Pitch*(desc.Height-1), -(int)r.Pitch, 32);

	pSurface->UnlockRect();

	if (p) {
		int w = bih->biWidth;
		int h = bih->biHeight;
		uint32_t* out = (uint32_t*)(bih + 1);

		switch (m_iRotation) {
		case 90:
			for (int x = w-1; x >= 0; x--) {
				for (int y = 0; y < h; y++) {
					*out++ = p[x + w*y];
				}
			}
			std::swap(bih->biWidth, bih->biHeight);
			break;
		case 180:
			for (int i = w*h - 1; i >= 0; i--) {
				*out++ = p[i];
			}
			break;
		case 270:
			for (int x = 0; x < w; x++) {
				for (int y = h-1; y >= 0; y--) {
					*out++ = p[x + w*y];
				}
			}
			std::swap(bih->biWidth, bih->biHeight);
			break;
		}

		delete[] p;
	}

	return S_OK;
}

STDMETHODIMP CBaseAP::ClearPixelShaders(int target)
{
	CAutoLock cRenderLock(&m_allocatorLock);

	if (target == TARGET_FRAME) {
		m_pPixelShaders.RemoveAll();
	} else if (target == TARGET_SCREEN) {
		m_pPixelShadersScreenSpace.RemoveAll();
	} else {
		return E_INVALIDARG;
	}
	m_pD3DDevEx->SetPixelShader(NULL);

	return S_OK;
}

STDMETHODIMP CBaseAP::AddPixelShader(int target, LPCSTR sourceCode, LPCSTR profile)
{
	CAutoLock cRenderLock(&m_allocatorLock);

	CAtlList<CExternalPixelShader> *pPixelShaders;
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

	HRESULT hr = Shader.Compile(m_pPSC);
	if (FAILED(hr)) {
		return hr;
	}

	pPixelShaders->AddTail(Shader);
	Paint(true);
	return S_OK;
}

// CSyncAP

CSyncAP::CSyncAP(HWND hWnd, bool bFullscreen, HRESULT& hr, CString &_Error)
	: CBaseAP(hWnd, bFullscreen, hr, _Error)
{
	HMODULE		hLib;
	CRenderersSettings& rs = GetRenderersSettings();

	m_nResetToken   = 0;
	m_hRenderThread = NULL;
	m_hMixerThread  = NULL;
	m_hEvtFlush     = NULL;
	m_hEvtQuit      = NULL;
	m_hEvtSkip      = NULL;
	m_bEvtQuit      = 0;
	m_bEvtFlush     = 0;

	if (FAILED (hr)) {
		_Error += L"SyncAP failed\n";
		return;
	}

	// Load EVR specifics DLLs
	hLib = LoadLibrary (L"dxva2.dll");
	pfDXVA2CreateDirect3DDeviceManager9	= hLib ? (PTR_DXVA2CreateDirect3DDeviceManager9) GetProcAddress (hLib, "DXVA2CreateDirect3DDeviceManager9") : NULL;

	// Load EVR functions
	hLib = LoadLibrary (L"evr.dll");
	pfMFCreateVideoSampleFromSurface = hLib ? (PTR_MFCreateVideoSampleFromSurface)GetProcAddress (hLib, "MFCreateVideoSampleFromSurface") : NULL;
	pfMFCreateVideoMediaType = hLib ? (PTR_MFCreateVideoMediaType)GetProcAddress (hLib, "MFCreateVideoMediaType") : NULL;

	if (!pfDXVA2CreateDirect3DDeviceManager9 || !pfMFCreateVideoSampleFromSurface || !pfMFCreateVideoMediaType) {
		if (!pfDXVA2CreateDirect3DDeviceManager9) {
			_Error += L"Could not find DXVA2CreateDirect3DDeviceManager9 (dxva2.dll)\n";
		}
		if (!pfMFCreateVideoSampleFromSurface) {
			_Error += L"Could not find MFCreateVideoSampleFromSurface (evr.dll)\n";
		}
		if (!pfMFCreateVideoMediaType) {
			_Error += L"Could not find MFCreateVideoMediaType (evr.dll)\n";
		}
		hr = E_FAIL;
		return;
	}

	// Load Vista specific DLLs
	hLib = LoadLibrary (L"avrt.dll");
	pfAvSetMmThreadCharacteristicsW = hLib ? (PTR_AvSetMmThreadCharacteristicsW) GetProcAddress (hLib, "AvSetMmThreadCharacteristicsW") : NULL;
	pfAvSetMmThreadPriority = hLib ? (PTR_AvSetMmThreadPriority) GetProcAddress (hLib, "AvSetMmThreadPriority") : NULL;
	pfAvRevertMmThreadCharacteristics = hLib ? (PTR_AvRevertMmThreadCharacteristics) GetProcAddress (hLib, "AvRevertMmThreadCharacteristics") : NULL;

	// Init DXVA manager
	hr = pfDXVA2CreateDirect3DDeviceManager9(&m_nResetToken, &m_pD3DManager);
	if (SUCCEEDED (hr)) {
		hr = m_pD3DManager->ResetDevice(m_pD3DDevEx, m_nResetToken);
		if (!SUCCEEDED (hr)) {
			_Error += L"m_pD3DManager->ResetDevice failed\n";
		}
	} else {
		_Error += L"DXVA2CreateDirect3DDeviceManager9 failed\n";
	}

	// Bufferize frame only with 3D texture
	if (rs.iSurfaceType == SURFACE_TEXTURE3D) {
		m_nSurface = max(min (rs.nEVRBuffers, MAX_PICTURE_SLOTS-2), 4);
	} else {
		m_nSurface = 1;
	}

	m_nRenderState = Shutdown;
	m_bStepping = false;
	m_bUseInternalTimer = false;
	m_LastSetOutputRange = -1;
	m_bPendingRenegotiate = false;
	m_bPendingMediaFinished = false;
	m_pCurrentDisplaydSample = NULL;
	m_nStepCount = 0;
	m_dwVideoAspectRatioMode = MFVideoARMode_PreservePicture;
	m_dwVideoRenderPrefs = (MFVideoRenderPrefs)0;
	m_BorderColor = RGB (0,0,0);
	m_pOuterEVR = NULL;
	m_bPrerolled = false;
	m_lShiftToNearest = -1; // Illegal value to start with
}

CSyncAP::~CSyncAP(void)
{
	StopWorkerThreads();
	m_pMediaType = NULL;
	m_pClock = NULL;
	m_pD3DManager = NULL;
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
		m_hEvtQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_hEvtFlush = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_hEvtSkip = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_hMixerThread = ::CreateThread(NULL, 0, MixerThreadStatic, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hMixerThread, THREAD_PRIORITY_HIGHEST);
		m_hRenderThread = ::CreateThread(NULL, 0, RenderThreadStatic, (LPVOID)this, 0, &dwThreadId);
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
	CheckPointer(ppRenderer, E_POINTER);
	*ppRenderer = NULL;
	HRESULT hr = E_FAIL;

	do {
		CMacrovisionKicker* pMK = DNew CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;

		CSyncRenderer *pOuterEVR = DNew CSyncRenderer(NAME("CSyncRenderer"), pUnk, hr, &m_VMR9AlphaBitmap, this);
		m_pOuterEVR = pOuterEVR;

		pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)pOuterEVR);
		CComQIPtr<IBaseFilter> pBF = pUnk;

		if (FAILED(hr)) {
			break;
		}

		// Set EVR custom presenter
		CComPtr<IMFVideoPresenter>	pVP;
		CComPtr<IMFVideoRenderer>	pMFVR;
		CComQIPtr<IMFGetService>	pMFGS	= pBF;
		CComQIPtr<IEVRFilterConfig> pConfig	= pBF;

		if (FAILED(pConfig->SetNumberOfStreams(3))) { // TODO - maybe need other number of input stream ...
			DLog(L"IEVRFilterConfig->SetNumberOfStreams(3) fail");
			break;
		}

		hr = pMFGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pMFVR));
		if (SUCCEEDED(hr)) {
			hr = QueryInterface(IID_PPV_ARGS(&pVP));
		}
		if (SUCCEEDED(hr)) {
			hr = pMFVR->InitializeRenderer(NULL, pVP);
		}

		m_bUseInternalTimer = HookNewSegmentAndReceive(GetFirstPin(pBF));

		if (FAILED(hr)) {
			*ppRenderer = NULL;
		} else {
			*ppRenderer = pBF.Detach();
		}
	} while (0);

	return hr;
}

STDMETHODIMP_(bool) CSyncAP::Paint(bool fAll)
{
	return __super::Paint(fAll);
}

STDMETHODIMP CSyncAP::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	HRESULT		hr;
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
	} else if (riid == __uuidof(ID3DFullscreenControl)) {
		hr = GetInterface((ID3DFullscreenControl*)this, ppv);
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
	m_nRenderState	= Started;
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
	HRESULT		hr = S_OK;
	float		fMaxRate = 0.0f;

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
	// pfNearestSupportedRate can be NULL.
	CAutoLock lock(this);
	HRESULT hr = S_OK;
	float   fMaxRate = 0.0f;
	float   fNearestRate = flRate;   // Default.

	CheckPointer (pflNearestSupportedRate, E_POINTER);
	CHECK_HR(CheckShutdown());

	// Find the maximum forward rate.
	fMaxRate = GetMaxRate(fThin);

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
	if (pflNearestSupportedRate != NULL) {
		*pflNearestSupportedRate = fNearestRate;
	}
	return hr;
}

float CSyncAP::GetMaxRate(BOOL bThin)
{
	float fMaxRate = FLT_MAX;  // Default.
	UINT32 fpsNumerator = 0, fpsDenominator = 0;
	UINT MonitorRateHz = 0;

	if (!bThin && (m_pMediaType != NULL)) {
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
			m_pGenlock->ResetTiming();
			m_pRefClock = NULL;
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
			while (*((volatile bool *)&m_bPendingRenegotiate)) {
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
	AM_MEDIA_TYPE* pAMMedia = NULL;
	MFVIDEOFORMAT* VideoFormat;

	CHECK_HR(pMixerType->GetRepresentation(FORMAT_MFVideoFormat, (void**)&pAMMedia));

	VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;
	CHECK_HR(pfMFCreateVideoMediaType(VideoFormat, &m_pMediaType));

	m_pMediaType->SetUINT32(MF_MT_PAN_SCAN_ENABLED, 0);

	const CRenderersSettings& rs = GetRenderersSettings();
	m_pMediaType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, rs.m_AdvRendSets.iEVROutputRange == 1 ? MFNominalRange_16_235 : MFNominalRange_0_255);
	m_LastSetOutputRange = rs.m_AdvRendSets.iEVROutputRange;

	ULARGE_INTEGER ui64FrameSize;
	m_pMediaType->GetUINT64(MF_MT_FRAME_SIZE, &ui64FrameSize.QuadPart);

	CSize videoSize((LONG)ui64FrameSize.HighPart, (LONG)ui64FrameSize.LowPart);
	MFVideoArea Area = GetArea(0, 0, videoSize.cx, videoSize.cy);
	m_pMediaType->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&Area, sizeof(MFVideoArea));

	ULARGE_INTEGER ui64AspectRatio;
	m_pMediaType->GetUINT64(MF_MT_PIXEL_ASPECT_RATIO, &ui64AspectRatio.QuadPart);

	CSize aspectRatio(LONG(ui64AspectRatio.HighPart * videoSize.cx), LONG(ui64AspectRatio.LowPart * videoSize.cy));
	if (aspectRatio.cx > 0 && aspectRatio.cy > 0) {
		ReduceDim(aspectRatio);
	}

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
	AM_MEDIA_TYPE* pAMMedia = NULL;
	CString strTemp;

	CheckPointer(pType, E_POINTER);
	CHECK_HR(pType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pAMMedia));

	hr = InitializeDevice(pAMMedia);
	if (SUCCEEDED(hr)) {
		strTemp = GetMediaTypeName(pAMMedia->subtype);
		strTemp.Replace(L"MEDIASUBTYPE_", L"");
		m_strStatsMsg[MSG_MIXEROUT].Format (L"Mixer output : %s", strTemp);
	}

	pType->FreeRepresentation(FORMAT_VideoInfo2, (void*)pAMMedia);

	return hr;
}

struct D3DFORMAT_TYPE {
	const int Format;
	const LPCTSTR Description;
};

LONGLONG CSyncAP::GetMediaTypeMerit(IMFMediaType *pMediaType)
{
	AM_MEDIA_TYPE *pAMMedia = NULL;
	MFVIDEOFORMAT *VideoFormat;

	HRESULT hr;
	CHECK_HR(pMediaType->GetRepresentation  (FORMAT_MFVideoFormat, (void**)&pAMMedia));
	VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;

	LONGLONG Merit = 0;
	switch (VideoFormat->surfaceInfo.Format) {
		case FCC('NV12'):
			Merit = 90000000;
			break;
		case FCC('YV12'):
			Merit = 80000000;
			break;
		case FCC('YUY2'):
			Merit = 70000000;
			break;
		case FCC('UYVY'):
			Merit = 60000000;
			break;

		case D3DFMT_X8R8G8B8: // Never opt for RGB
		case D3DFMT_A8R8G8B8:
		case D3DFMT_R8G8B8:
		case D3DFMT_R5G6B5:
			Merit = 0;
			break;
		default:
			Merit = 1000;
			break;
	}
	pMediaType->FreeRepresentation(FORMAT_MFVideoFormat, (void*)pAMMedia);
	return Merit;
}

HRESULT CSyncAP::RenegotiateMediaType()
{
	HRESULT hr = S_OK;

	CComPtr<IMFMediaType> pMixerType;
	CComPtr<IMFMediaType> pType;

	if (!m_pMixer) {
		return MF_E_INVALIDREQUEST;
	}

	// Get the mixer's input type
	hr = m_pMixer->GetInputCurrentType(0, &pType);
	if (SUCCEEDED(hr)) {
	    AM_MEDIA_TYPE* pMT;
	    hr = pType->GetRepresentation(FORMAT_VideoInfo2, (void**)&pMT);
	    if (SUCCEEDED(hr)) {
	        m_inputMediaType = *pMT;
	        pType->FreeRepresentation(FORMAT_VideoInfo2, pMT);
	    }
	}

	CInterfaceArray<IMFMediaType> ValidMixerTypes;
	// Loop through all of the mixer's proposed output types.
	DWORD iTypeIndex = 0;
	while ((hr != MF_E_NO_MORE_TYPES)) {
		pMixerType  = NULL;
		pType = NULL;
		m_pMediaType = NULL;

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
			LONGLONG Merit = GetMediaTypeMerit(pType);

			size_t nTypes = ValidMixerTypes.GetCount();
			size_t iInsertPos = 0;
			for (size_t i = 0; i < nTypes; ++i) {
				LONGLONG ThisMerit = GetMediaTypeMerit(ValidMixerTypes[i]);
				if (Merit > ThisMerit) {
					iInsertPos = i;
					break;
				} else {
					iInsertPos = i+1;
				}
			}
			ValidMixerTypes.InsertAt(iInsertPos, pType);
		}
	}

	size_t nValidTypes = ValidMixerTypes.GetCount();
	for (size_t i = 0; i < nValidTypes; ++i) {
		pType = ValidMixerTypes[i];
	}

	for (size_t i = 0; i < nValidTypes; ++i) {
		pType = ValidMixerTypes[i];
		hr = SetMediaType(pType);
		if (SUCCEEDED(hr)) {
			hr = m_pMixer->SetOutputType(0, pType, 0);
			// If something went wrong, clear the media type.
			if (FAILED(hr)) {
				SetMediaType(NULL);
			} else {
				break;
			}
		}
	}

	pMixerType = NULL;
	pType = NULL;
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

	UINT dwSurface;
	bool newSample = false;

	while (SUCCEEDED(hr)) { // Get as many frames as there are and that we have samples for
		CComPtr<IMFSample> pSample;
		CComPtr<IMFSample> pNewSample;
		if (FAILED(GetFreeSample(&pSample))) { // All samples are taken for the moment. Better luck next time
			break;
		}

		memset(&Buffer, 0, sizeof(Buffer));
		Buffer.pSample = pSample;
		pSample->GetUINT32(GUID_SURFACE_INDEX, &dwSurface);
		{
			llClockBefore = GetPerfCounter();
			hr = m_pMixer->ProcessOutput(0 , 1, &Buffer, &dwStatus);
			llClockAfter = GetPerfCounter();
		}

		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) { // There are no samples left in the mixer
			MoveToFreeList(pSample, false);
			break;
		}
		if (m_pSink) {
			llMixerLatency = llClockAfter - llClockBefore;
			m_pSink->Notify (EC_PROCESSING_LATENCY, (LONG_PTR)&llMixerLatency, 0);
		}

		newSample = true;

		if (GetRenderersData()->m_bTearingTest) {
			RECT rcTearing;

			rcTearing.left = m_nTearingPos;
			rcTearing.top = 0;
			rcTearing.right	= rcTearing.left + 4;
			rcTearing.bottom = m_nativeVideoSize.cy;
			m_pD3DDevEx->ColorFill(m_pVideoSurfaces[dwSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));

			rcTearing.left = (rcTearing.right + 15) % m_nativeVideoSize.cx;
			rcTearing.right	= rcTearing.left + 4;
			m_pD3DDevEx->ColorFill(m_pVideoSurfaces[dwSurface], &rcTearing, D3DCOLOR_ARGB (255,255,0,0));
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

	if (m_pMediaType == NULL) {
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
	m_pMixer = NULL;
	m_pSink = NULL;
	m_pClock = NULL;
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
		pszVideo->cx	= m_nativeVideoSize.cx;
		pszVideo->cy	= m_nativeVideoSize.cy;
	}
	if (pszARVideo) {
		pszARVideo->cx	= m_nativeVideoSize.cx * m_aspectRatio.cx;
		pszARVideo->cy	= m_nativeVideoSize.cy * m_aspectRatio.cy;
	}
	return S_OK;
}

STDMETHODIMP CSyncAP::GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax)
{
	if (pszMin) {
		pszMin->cx	= 1;
		pszMin->cy	= 1;
	}

	if (pszMax) {
		D3DDISPLAYMODE	d3ddm;

		ZeroMemory(&d3ddm, sizeof(d3ddm));
		if (SUCCEEDED(m_pD3DEx->GetAdapterDisplayMode(GetAdapter(m_pD3DEx, m_hWnd), &d3ddm))) {
			pszMax->cx	= d3ddm.Width;
			pszMax->cy	= d3ddm.Height;
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
	ASSERT (FALSE);
	return E_NOTIMPL;
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
	HRESULT		hr = m_pD3DManager->ResetDevice (pDevice, resetToken);
	return hr;
}

STDMETHODIMP CSyncAP::OpenDeviceHandle(HANDLE *phDevice)
{
	HRESULT		hr = m_pD3DManager->OpenDeviceHandle (phDevice);
	return hr;
}

STDMETHODIMP CSyncAP::CloseDeviceHandle(HANDLE hDevice)
{
	HRESULT		hr = m_pD3DManager->CloseDeviceHandle(hDevice);
	return hr;
}

STDMETHODIMP CSyncAP::TestDevice(HANDLE hDevice)
{
	HRESULT		hr = m_pD3DManager->TestDevice(hDevice);
	return hr;
}

STDMETHODIMP CSyncAP::LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock)
{
	HRESULT		hr = m_pD3DManager->LockDevice(hDevice, ppDevice, fBlock);
	return hr;
}

STDMETHODIMP CSyncAP::UnlockDevice(HANDLE hDevice, BOOL fSaveState)
{
	HRESULT		hr = m_pD3DManager->UnlockDevice(hDevice, fSaveState);
	return hr;
}

STDMETHODIMP CSyncAP::GetVideoService(HANDLE hDevice, REFIID riid, void **ppService)
{
	HRESULT		hr = m_pD3DManager->GetVideoService(hDevice, riid, ppService);

	if (riid == __uuidof(IDirectXVideoDecoderService)) {
		UINT		nNbDecoder = 5;
		GUID*		pDecoderGuid;
		IDirectXVideoDecoderService*		pDXVAVideoDecoder = (IDirectXVideoDecoderService*) *ppService;
		pDXVAVideoDecoder->GetDecoderDeviceGuids (&nNbDecoder, &pDecoderGuid);
	} else if (riid == __uuidof(IDirectXVideoProcessorService)) {
		IDirectXVideoProcessorService*		pDXVAProcessor = (IDirectXVideoProcessorService*) *ppService;
		UNREFERENCED_PARAMETER(pDXVAProcessor);
	}

	return hr;
}

STDMETHODIMP CSyncAP::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	// This function should be called...
	ASSERT (FALSE);

	if (lpWidth) {
		*lpWidth	= m_nativeVideoSize.cx;
	}
	if (lpHeight)	{
		*lpHeight	= m_nativeVideoSize.cy;
	}
	if (lpARWidth)	{
		*lpARWidth	= m_aspectRatio.cx;
	}
	if (lpARHeight)	{
		*lpARHeight	= m_aspectRatio.cy;
	}
	return S_OK;
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

	for (int i = 0; i < m_nSurface; i++) {
		CComPtr<IMFSample> pMFSample;
		hr = pfMFCreateVideoSampleFromSurface(m_pVideoSurfaces[i], &pMFSample);
		if (SUCCEEDED (hr)) {
			pMFSample->SetUINT32(GUID_SURFACE_INDEX, i);
			m_FreeSamples.AddTail (pMFSample);
		}
		ASSERT (SUCCEEDED (hr));
	}

	{
		// get rotation flag
		CComPtr<IBaseFilter> pBF;
		if (SUCCEEDED(m_pOuterEVR->QueryInterface(IID_PPV_ARGS(&pBF)))) {
			while (pBF = GetUpStreamFilter(pBF)) {
				if (CComQIPtr<IPropertyBag> pPB = pBF) {
					CComVariant var;
					if (SUCCEEDED(pPB->Read(L"ROTATION", &var, NULL)) && var.vt == VT_BSTR) {
						int rotation = _wtoi(var.bstrVal) % 360;
						if (rotation && (rotation % 90 == 0)) {
							if (rotation < 0) {
								rotation += 360;
							}
							m_iRotation = rotation;
						}
						break;
					}
				}
			}
		}
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
	HANDLE hEvts[] = {m_hEvtQuit};
	bool bQuit = false;
	TIMECAPS tc;
	DWORD dwResolution;
	DWORD dwUser = 0;

	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
	dwUser = timeBeginPeriod(dwResolution);

	while (!bQuit) {
		DWORD dwObject = WaitForMultipleObjects (_countof(hEvts), hEvts, FALSE, 1);
		switch (dwObject) {
			case WAIT_OBJECT_0 :
				bQuit = true;
				break;
			case WAIT_TIMEOUT : {
				bool bNewSample = false;
				{
					CAutoLock AutoLock(&m_ImageProcessingLock);
					bNewSample = GetSampleFromMixer();
				}
				if (m_bUseInternalTimer && m_pSubPicQueue) {
					m_pSubPicQueue->SetFPS(m_fps);
				}
			}
			break;
		}
	}
	timeEndPeriod (dwResolution);
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
	TIMECAPS tc;
	DWORD dwResolution;
	LONGLONG llRefClockTime;
	double dTargetSyncOffset;
	MFTIME llSystemTime;
	DWORD dwUser = 0;
	DWORD dwObject;
	int nSamplesLeft;
	CComPtr<IMFSample>pNewSample = NULL; // The sample next in line to be presented

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
	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
	dwUser = timeBeginPeriod(dwResolution);
	pNewSample = NULL;

	while (!bQuit) {
		m_lNextSampleWait = 1; // Default value for running this loop
		nSamplesLeft = 0;
		bool stepForward = false;
		LONG lDisplayCycle = (LONG)(GetDisplayCycle());
		LONG lDisplayCycle2 = (LONG)(GetDisplayCycle() / 2.0); // These are a couple of empirically determined constants used the control the "snap" function
		LONG lDisplayCycle4 = (LONG)(GetDisplayCycle() / 4.0);

		CRenderersSettings& rs = GetRenderersSettings();
		dTargetSyncOffset = rs.m_AdvRendSets.dTargetSyncOffset;

		if ((m_nRenderState == Started || !m_bPrerolled) && !pNewSample) { // If either streaming or the pre-roll sample and no sample yet fetched
			if (SUCCEEDED(GetScheduledSample(&pNewSample, nSamplesLeft))) { // Get the next sample
				m_llLastSampleTime = m_llSampleTime;
				if (!m_bPrerolled) {
					m_bPrerolled = true; // m_bPrerolled is a ticket to show one (1) frame immediately
					m_lNextSampleWait = 0; // Present immediately
				} else if (SUCCEEDED(pNewSample->GetSampleTime(&m_llSampleTime))) { // Get zero-based sample due time
					if (m_llLastSampleTime == m_llSampleTime) { // In the rare case there are duplicate frames in the movie. There really shouldn't be but it happens.
						MoveToFreeList(pNewSample, true);
						pNewSample = NULL;
						m_lNextSampleWait = 0;
					} else {
						m_pClock->GetCorrelatedTime(0, &llRefClockTime, &llSystemTime); // Get zero-based reference clock time. llSystemTime is not used for anything here
						m_lNextSampleWait = (LONG)((m_llSampleTime - llRefClockTime) / 10000); // Time left until sample is due, in ms
						if (m_bStepping) {
							m_lNextSampleWait = 0;
						} else if (rs.m_AdvRendSets.iSynchronizeMode == SYNCHRONIZE_NEAREST) { // Present at the closest "safe" occasion at dTargetSyncOffset ms before vsync to avoid tearing
							if (m_lNextSampleWait < -lDisplayCycle) { // We have to allow slightly negative numbers at this stage. Otherwise we get "choking" when frame rate > refresh rate
								SetEvent(m_hEvtSkip);
								m_bEvtSkip = true;
							}
							REFERENCE_TIME rtRefClockTimeNow;
							if (m_pRefClock) {
								m_pRefClock->GetTime(&rtRefClockTimeNow);    // Reference clock time now
							}
							LONG lLastVsyncTime = (LONG)((m_llEstVBlankTime - rtRefClockTimeNow) / 10000); // Last vsync time relative to now
							if (abs(lLastVsyncTime) > lDisplayCycle) {
								lLastVsyncTime = - lDisplayCycle;    // To even out glitches in the beginning
							}

							LONGLONG llNextSampleWait = (LONGLONG)((lLastVsyncTime + GetDisplayCycle() - dTargetSyncOffset) * 10000); // Time from now util next safe time to Paint()
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
		dwObject = WaitForMultipleObjects(_countof(hEvts), hEvts, FALSE, (DWORD)m_lNextSampleWait);
		switch (dwObject) {
			case WAIT_OBJECT_0: // Quit
				bQuit = true;
				break;

			case WAIT_OBJECT_0 + 1: // Flush
				if (pNewSample) {
					MoveToFreeList(pNewSample, true);
				}
				pNewSample = NULL;
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
				if (m_LastSetOutputRange != -1 && m_LastSetOutputRange != rs.m_AdvRendSets.iEVROutputRange || m_bPendingRenegotiate) {
					if (pNewSample) {
						MoveToFreeList(pNewSample, true);
					}
					pNewSample = NULL;
					FlushSamples();
					RenegotiateMediaType();
					m_bPendingRenegotiate = false;
				}

				if (m_bPendingResetDevice) {
					if (pNewSample) {
						MoveToFreeList(pNewSample, true);
					}
					pNewSample = NULL;
					SendResetRequest();
				} else if (m_nStepCount < 0) {
					m_nStepCount = 0;
					m_pcFramesDropped++;
					stepForward = true;
				} else if (pNewSample && (m_nStepCount > 0)) {
					pNewSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_iCurSurface);
					if (!g_bExternalSubtitleTime) {
						__super::SetTime (g_tSegmentStart + m_llSampleTime);
					}
					Paint(true);
					CompleteFrameStep(false);
					m_pcFramesDrawn++;
					stepForward = true;
				} else if (pNewSample && !m_bStepping) { // When a stepped frame is shown, a new one is fetched that we don't want to show here while stepping
					pNewSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32*)&m_iCurSurface);
					if (!g_bExternalSubtitleTime) {
						__super::SetTime (g_tSegmentStart + m_llSampleTime);
					}
					Paint(true);
					m_pcFramesDrawn++;
					stepForward = true;
				}
				break;
		} // switch
		if (pNewSample && stepForward) {
			MoveToFreeList(pNewSample, true);
			pNewSample = NULL;
		}
	} // while
	if (pNewSample) {
		MoveToFreeList(pNewSample, true);
		pNewSample = NULL;
	}
	timeEndPeriod (dwResolution);
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

	for (int i = 0; i < m_nSurface; i++) {
		CComPtr<IMFSample> pMFSample;
		HRESULT hr = pfMFCreateVideoSampleFromSurface (m_pVideoSurfaces[i], &pMFSample);
		if (SUCCEEDED (hr)) {
			pMFSample->SetUINT32(GUID_SURFACE_INDEX, i);
			m_FreeSamples.AddTail(pMFSample);
		}
		ASSERT(SUCCEEDED (hr));
	}
	return bResult;
}

void CSyncAP::OnResetDevice()
{
	TRACE("--> CSyncAP::OnResetDevice on thread: %d\n", GetCurrentThreadId());
	HRESULT hr;
	hr = m_pD3DManager->ResetDevice(m_pD3DDevEx, m_nResetToken);
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
	m_ScheduledSamples.RemoveAll();
	m_FreeSamples.RemoveAll();
	m_nUsedBuffer = 0;
}

HRESULT CSyncAP::GetFreeSample(IMFSample** ppSample)
{
	CAutoLock lock(&m_SampleQueueLock);
	HRESULT		hr = S_OK;

	if (m_FreeSamples.GetCount() > 1) {	// Cannot use first free buffer (can be currently displayed)
		InterlockedIncrement(&m_nUsedBuffer);
		*ppSample = m_FreeSamples.RemoveHead().Detach();
	} else {
		hr = MF_E_SAMPLEALLOCATOR_EMPTY;
	}

	return hr;
}

HRESULT CSyncAP::GetScheduledSample(IMFSample** ppSample, int &_Count)
{
	CAutoLock lock(&m_SampleQueueLock);
	HRESULT		hr = S_OK;

	_Count = (int)m_ScheduledSamples.GetCount();
	if (_Count > 0) {
		*ppSample = m_ScheduledSamples.RemoveHead().Detach();
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
		m_FreeSamples.AddTail(pSample);
	} else {
		m_FreeSamples.AddHead(pSample);
	}
}

void CSyncAP::MoveToScheduledList(IMFSample* pSample, bool _bSorted)
{
	if (_bSorted) {
		CAutoLock lock(&m_SampleQueueLock);
		m_ScheduledSamples.AddHead(pSample);
	} else {
		CAutoLock lock(&m_SampleQueueLock);
		m_ScheduledSamples.AddTail(pSample);
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
	while (m_ScheduledSamples.GetCount() > 0) {
		CComPtr<IMFSample> pMFSample;
		pMFSample = m_ScheduledSamples.RemoveHead();
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
	if (CComQIPtr<IAMAudioRendererStats> pAS = pBF) {
		m_pAudioStats = pAS;
	};
	EndEnumFilters

	pEVR->GetSyncSource(&m_pRefClock);
	if (filterInfo.pGraph) {
		filterInfo.pGraph->Release();
	}
	m_pGenlock->SetMonitor(GetAdapter(m_pD3DEx, m_hWnd));
	m_pGenlock->GetTiming();

	ResetStats();
	EstimateRefreshTimings();
	if (m_dFrameCycle > 0.0) {
		m_dCycleDifference = GetCycleDifference();    // Might have moved to another display
	}
	return S_OK;
}

HRESULT CreateSyncRenderer(const CLSID& clsid, HWND hWnd, bool bFullscreen, ISubPicAllocatorPresenter3** ppAP)
{
	HRESULT		hr = E_FAIL;
	if (clsid == CLSID_SyncAllocatorPresenter) {
		CString Error;
		*ppAP	= DNew CSyncAP(hWnd, bFullscreen, hr, Error);
		(*ppAP)->AddRef();

		if (FAILED(hr)) {
			Error += L"\n";
			Error += GetWindowsErrorMessage(hr, NULL);
			MessageBox(hWnd, Error, L"Error creating EVR Sync", MB_OK | MB_ICONERROR);
			(*ppAP)->Release();
			*ppAP = NULL;
		} else if (!Error.IsEmpty()) {
			MessageBox(hWnd, Error, L"Warning creating EVR Sync", MB_OK|MB_ICONWARNING);
		}
	}
	return hr;
}

CSyncRenderer::CSyncRenderer(const TCHAR* pName, LPUNKNOWN pUnk, HRESULT& hr, VMR9AlphaBitmap* pVMR9AlphaBitmap, CSyncAP *pAllocatorPresenter): CUnknown(pName, pUnk)
{
	hr = m_pEVR.CoCreateInstance(CLSID_EnhancedVideoRenderer, GetOwner());
	CComQIPtr<IBaseFilter> pEVRBase = m_pEVR;
	m_pEVRBase = pEVRBase; // Don't keep a second reference on the EVR filter
	m_pVMR9AlphaBitmap = pVMR9AlphaBitmap;
	m_pAllocatorPresenter = pAllocatorPresenter;
}

CSyncRenderer::~CSyncRenderer()
{
}

HRESULT STDMETHODCALLTYPE CSyncRenderer::GetState(DWORD dwMilliSecsTimeout, __out  FILTER_STATE *State)
{
	if (m_pEVRBase) {
		return m_pEVRBase->GetState(dwMilliSecsTimeout, State);
	}
	return E_NOTIMPL;
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

STDMETHODIMP CSyncRenderer::GetAlphaBitmapParameters(VMR9AlphaBitmap* pBmpParms)
{
	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
	memcpy (pBmpParms, m_pVMR9AlphaBitmap, sizeof(VMR9AlphaBitmap));
	return S_OK;
}

STDMETHODIMP CSyncRenderer::SetAlphaBitmap(const VMR9AlphaBitmap* pBmpParms)
{
	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
	memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
	m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
	m_pAllocatorPresenter->UpdateAlphaBitmap();
	return S_OK;
}

STDMETHODIMP CSyncRenderer::UpdateAlphaBitmapParameters(const VMR9AlphaBitmap* pBmpParms)
{
	CheckPointer(pBmpParms, E_POINTER);
	CAutoLock BitMapLock(&m_pAllocatorPresenter->m_VMR9AlphaBitmapLock);
	memcpy (m_pVMR9AlphaBitmap, pBmpParms, sizeof(VMR9AlphaBitmap));
	m_pVMR9AlphaBitmap->dwFlags |= VMRBITMAP_UPDATE;
	m_pAllocatorPresenter->UpdateAlphaBitmap();
	return S_OK;
}

STDMETHODIMP CSyncRenderer::support_ffdshow()
{
	queue_ffdshow_support = true;
	return S_OK;
}

STDMETHODIMP CSyncRenderer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	HRESULT hr;

	if (riid == __uuidof(IVMRMixerBitmap9)) {
		return GetInterface((IVMRMixerBitmap9*)this, ppv);
	}

	if (riid == __uuidof(IBaseFilter)) {
		return GetInterface((IBaseFilter*)this, ppv);
	}

	if (riid == __uuidof(IMediaFilter)) {
		return GetInterface((IMediaFilter*)this, ppv);
	}
	if (riid == __uuidof(IPersist)) {
		return GetInterface((IPersist*)this, ppv);
	}
	if (riid == __uuidof(IBaseFilter)) {
		return GetInterface((IBaseFilter*)this, ppv);
	}

	hr = m_pEVR ? m_pEVR->QueryInterface(riid, ppv) : E_NOINTERFACE;
	if (m_pEVR && FAILED(hr)) {
	    hr = m_pAllocatorPresenter ? m_pAllocatorPresenter->QueryInterface(riid, ppv) : E_NOINTERFACE;
	    if (FAILED(hr)) {
	        if (riid == __uuidof(IVMRffdshow9)) { // Support ffdshow queueing. We show ffdshow that this is patched MPC-BE.
	            return GetInterface((IVMRffdshow9*)this, ppv);
	        }
	    }
	}
	return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
}

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

// CGenlock

CGenlock::CGenlock(double target, double limit, int lineD, int colD, double clockD, UINT mon):
	targetSyncOffset(target), // Target sync offset, typically around 10 ms
	controlLimit(limit), // How much sync offset is allowed to drift from target sync offset before control kicks in
	lineDelta(lineD), // Number of rows used in display frequency adjustment, typically 1 (one)
	columnDelta(colD),  // Number of columns used in display frequency adjustment, typically 1 - 2
	cycleDelta(clockD),  // Delta used in clock speed adjustment. In fractions of 1.0. Typically around 0.001
	monitor(mon) // The monitor to be adjusted if the display refresh rate is the controlled parameter
{
	lowSyncOffset = targetSyncOffset - controlLimit;
	highSyncOffset = targetSyncOffset + controlLimit;
	adjDelta = 0;
	displayAdjustmentsMade = 0;
	clockAdjustmentsMade = 0;
	displayFreqCruise = 0;
	displayFreqFaster = 0;
	displayFreqSlower = 0;
	curDisplayFreq = 0;
	psWnd = NULL;
	liveSource = FALSE;
	powerstripTimingExists = FALSE;
	syncOffsetFifo = DNew MovingAverage(64);
	frameCycleFifo = DNew MovingAverage(4);
}

CGenlock::~CGenlock()
{
	ResetTiming();
	if (syncOffsetFifo != NULL) {
		delete syncOffsetFifo;
		syncOffsetFifo = NULL;
	}
	if (frameCycleFifo != NULL) {
		delete frameCycleFifo;
		frameCycleFifo = NULL;
	}
	syncClock = NULL;
};

BOOL CGenlock::PowerstripRunning()
{
	psWnd = FindWindow(L"TPShidden", NULL);
	if (!psWnd) {
		return FALSE;    // Powerstrip is not running
	} else {
		return TRUE;
	}
}

// Get the display timing parameters through PowerStrip (if running).
HRESULT CGenlock::GetTiming()
{
	ATOM getTiming;
	LPARAM lParam = NULL;
	WPARAM wParam = monitor;
	int i = 0;
	int j = 0;
	int params = 0;
	TCHAR tmpStr[MAX_LOADSTRING];

	CAutoLock lock(&csGenlockLock);
	if (!PowerstripRunning()) {
		return E_FAIL;
	}

	getTiming = static_cast<ATOM>(SendMessage(psWnd, UM_GETTIMING, wParam, lParam));
	GlobalGetAtomName(getTiming, savedTiming, MAX_LOADSTRING);

	while (params < TIMING_PARAM_CNT) {
		while (savedTiming[i] != ',' && savedTiming[i] != '\0') {
			tmpStr[j++] = savedTiming[i];
			tmpStr[j] = '\0';
			i++;
		}
		i++; // Skip trailing comma
		j = 0;
		displayTiming[params] = _ttoi(tmpStr);
		displayTimingSave[params] = displayTiming[params];
		params++;
	}

	// The display update frequency is controlled by adding and subtracting pixels form the
	// image. This is done by either subtracting columns or rows or both. Some displays like
	// row adjustments and some column adjustments. One should probably not do both.
	StringCchPrintf(faster, MAX_LOADSTRING, _T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\0"),
					displayTiming[0],
					displayTiming[HFRONTPORCH] - columnDelta,
					displayTiming[2],
					displayTiming[3],
					displayTiming[4],
					displayTiming[VFRONTPORCH] - lineDelta,
					displayTiming[6],
					displayTiming[7],
					displayTiming[PIXELCLOCK],
					displayTiming[9]
				   );

	// Nominal update frequency
	StringCchPrintf(cruise, MAX_LOADSTRING, _T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\0"),
					displayTiming[0],
					displayTiming[HFRONTPORCH],
					displayTiming[2],
					displayTiming[3],
					displayTiming[4],
					displayTiming[VFRONTPORCH],
					displayTiming[6],
					displayTiming[7],
					displayTiming[PIXELCLOCK],
					displayTiming[9]
				   );

	// Lower than nominal update frequency
	StringCchPrintf(slower, MAX_LOADSTRING, _T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\0"),
					displayTiming[0],
					displayTiming[HFRONTPORCH] + columnDelta,
					displayTiming[2],
					displayTiming[3],
					displayTiming[4],
					displayTiming[VFRONTPORCH] + lineDelta,
					displayTiming[6],
					displayTiming[7],
					displayTiming[PIXELCLOCK],
					displayTiming[9]
				   );

	totalColumns = displayTiming[HACTIVE] + displayTiming[HFRONTPORCH] + displayTiming[HSYNCWIDTH] + displayTiming[HBACKPORCH];
	totalLines = displayTiming[VACTIVE] + displayTiming[VFRONTPORCH] + displayTiming[VSYNCWIDTH] + displayTiming[VBACKPORCH];
	pixelClock = 1000 * displayTiming[PIXELCLOCK]; // Pixels/s
	displayFreqCruise = (double)pixelClock / (totalLines * totalColumns); // Frames/s
	displayFreqSlower = (double)pixelClock / ((totalLines + lineDelta) * (totalColumns + columnDelta));
	displayFreqFaster = (double)pixelClock / ((totalLines - lineDelta) * (totalColumns - columnDelta));
	curDisplayFreq = displayFreqCruise;
	GlobalDeleteAtom(getTiming);
	adjDelta = 0;
	powerstripTimingExists = TRUE;
	return S_OK;
}

// Reset display timing parameters to nominal.
HRESULT CGenlock::ResetTiming()
{
	LPARAM lParam = NULL;
	WPARAM wParam = monitor;
	ATOM setTiming;
	LRESULT ret;
	CAutoLock lock(&csGenlockLock);

	if (!PowerstripRunning()) {
		return E_FAIL;
	}

	if (displayAdjustmentsMade > 0) {
		setTiming = GlobalAddAtom(cruise);
		lParam = setTiming;
		ret = SendMessage(psWnd, UM_SETCUSTOMTIMINGFAST, wParam, lParam);
		GlobalDeleteAtom(setTiming);
		curDisplayFreq = displayFreqCruise;
	}
	adjDelta = 0;
	return S_OK;
}

// Reset reference clock speed to nominal.
HRESULT CGenlock::ResetClock()
{
	adjDelta = 0;
	if (syncClock == NULL) {
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
		syncClock = NULL;    // Release any outstanding references if this is called repeatedly
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
	displayAdjustmentsMade = 0;
	clockAdjustmentsMade = 0;
	return S_OK;
}

// Synchronize by adjusting display refresh rate
HRESULT CGenlock::ControlDisplay(double syncOffset, double frameCycle)
{
	LPARAM lParam = NULL;
	WPARAM wParam = monitor;
	ATOM setTiming;

	syncOffsetAvg = syncOffsetFifo->Average(syncOffset);
	minSyncOffset = min(minSyncOffset, syncOffset);
	maxSyncOffset = max(maxSyncOffset, syncOffset);
	frameCycleAvg = frameCycleFifo->Average(frameCycle);
	minFrameCycle = min(minFrameCycle, frameCycle);
	maxFrameCycle = max(maxFrameCycle, frameCycle);

	if (!PowerstripRunning() || !powerstripTimingExists) {
		return E_FAIL;
	}
	// Adjust as seldom as possible by checking the current controlState before changing it.
	if ((syncOffsetAvg > highSyncOffset) && (adjDelta != 1))
		// Speed up display refresh rate by subtracting pixels from the image.
	{
		adjDelta = 1; // Increase refresh rate
		curDisplayFreq = displayFreqFaster;
		setTiming = GlobalAddAtom(faster);
		lParam = setTiming;
		SendMessage(psWnd, UM_SETCUSTOMTIMINGFAST, wParam, lParam);
		GlobalDeleteAtom(setTiming);
		displayAdjustmentsMade++;
	} else
		// Slow down display refresh rate by adding pixels to the image.
		if ((syncOffsetAvg < lowSyncOffset) && (adjDelta != -1)) {
			adjDelta = -1;
			curDisplayFreq = displayFreqSlower;
			setTiming = GlobalAddAtom(slower);
			lParam = setTiming;
			SendMessage(psWnd, UM_SETCUSTOMTIMINGFAST, wParam, lParam);
			GlobalDeleteAtom(setTiming);
			displayAdjustmentsMade++;
		} else
			// Cruise.
			if ((syncOffsetAvg < targetSyncOffset) && (adjDelta == 1)) {
				adjDelta = 0;
				curDisplayFreq = displayFreqCruise;
				setTiming = GlobalAddAtom(cruise);
				lParam = setTiming;
				SendMessage(psWnd, UM_SETCUSTOMTIMINGFAST, wParam, lParam);
				GlobalDeleteAtom(setTiming);
				displayAdjustmentsMade++;
			} else if ((syncOffsetAvg > targetSyncOffset) && (adjDelta == -1)) {
				adjDelta = 0;
				curDisplayFreq = displayFreqCruise;
				setTiming = GlobalAddAtom(cruise);
				lParam = setTiming;
				SendMessage(psWnd, UM_SETCUSTOMTIMINGFAST, wParam, lParam);
				GlobalDeleteAtom(setTiming);
				displayAdjustmentsMade++;
			}
	return S_OK;
}

// Synchronize by adjusting reference clock rate (and therefore video FPS).
// Todo: check so that we don't have a live source
HRESULT CGenlock::ControlClock(double syncOffset, double frameCycle)
{
	syncOffsetAvg = syncOffsetFifo->Average(syncOffset);
	minSyncOffset = min(minSyncOffset, syncOffset);
	maxSyncOffset = max(maxSyncOffset, syncOffset);
	frameCycleAvg = frameCycleFifo->Average(frameCycle);
	minFrameCycle = min(minFrameCycle, frameCycle);
	maxFrameCycle = max(maxFrameCycle, frameCycle);

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
	syncOffsetAvg = syncOffsetFifo->Average(syncOffset);
	minSyncOffset = min(minSyncOffset, syncOffset);
	maxSyncOffset = max(maxSyncOffset, syncOffset);
	frameCycleAvg = frameCycleFifo->Average(frameCycle);
	minFrameCycle = min(minFrameCycle, frameCycle);
	maxFrameCycle = max(maxFrameCycle, frameCycle);
	return S_OK;
}
