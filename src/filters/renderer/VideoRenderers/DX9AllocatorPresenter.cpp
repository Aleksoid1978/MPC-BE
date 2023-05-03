/*
 * (C) 2003-2006 Gabest
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
#include "DX9AllocatorPresenter.h"
#include <utility>
#include <clsids.h>
#include "SubPic/DX9SubPic.h"
#include "SubPic/SubPicQueueImpl.h"
#include "IPinHook.h"
#include "DSUtil/SysVersion.h"
#include "DSUtil/WinAPIUtils.h"
#include "DSUtil/D3D9Helper.h"
#include "DSUtil/DXVAState.h"
#include "IDirectVobSub.h"
#include "FocusThread.h"
#include "D3DHook.h"
#include "Variables.h"
#include "Utils.h"
#include "apps/mplayerc/mpc_messages.h"

#define FRAMERATE_MAX_DELTA 3000

using namespace DSObjects;

static const LPCSTR s_nominalrange[8] = { nullptr, "0-255", "16-235", "48-208", nullptr, nullptr, nullptr, nullptr };
static const LPCSTR s_transfermatrix[8] = { nullptr, "BT.709", "BT.601", "SMPTE 240M", "BT.2020", nullptr, nullptr, "YCgCo" };
static const LPCSTR s_transferfunction[32] = {nullptr, "Linear RGB", "1.8 gamma", "2.0 gamma", "2.2 gamma", "BT.709", "SMPTE 240M", "sRGB",
	"2.8 gamma", "Log100", "Log316", "Symmetric BT.709", "Constant luminance BT.2020", "Non-constant luminance BT.2020",
	"2.6 gamma", "SMPTE ST 2084 (PQ)", "ARIB STD-B67 (HLG)"};

static const wchar_t g_szClassName[] = L"MPCVRWindow";

// CDX9AllocatorPresenter

CDX9AllocatorPresenter::CDX9AllocatorPresenter(HWND hWnd, bool bFullscreen, HRESULT& hr, CString &_Error)
	: CDX9RenderingEngine(hWnd, hr, &_Error)
	, m_bIsFullscreen(bFullscreen)
{
	DLog(L"CDX9AllocatorPresenter::CDX9AllocatorPresenter()");

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

CDX9AllocatorPresenter::~CDX9AllocatorPresenter()
{
	DLog(L"CDX9AllocatorPresenter::~CDX9AllocatorPresenter()");

	if (SysVersion::IsWin8orLater()) {
		D3DHook::UnHook();
	}

	DwmEnableMMCSS(FALSE);

	m_Font3D.InvalidateDeviceObjects();
	m_pLine.Release();
	m_pAlphaBitmapTexture.Release();
	CleanupRenderingEngine();

	m_pDevice9Ex.Release();
	m_pDevice9ExRefresh.Release();
	m_pD3D9Ex.Release();

	if (m_hD3D9) {
		FreeLibrary(m_hD3D9);
		m_hD3D9 = nullptr;
	}

	if (m_FocusThread) {
		m_FocusThread->PostThreadMessageW(WM_QUIT, 0, 0);
		if (WaitForSingleObject(m_FocusThread->m_hThread, 10000) == WAIT_TIMEOUT) {
			ASSERT(FALSE);
			TerminateThread(m_FocusThread->m_hThread, 0xDEAD);
		}
	}

	BOOL ret;

	if (m_hWndVR) {
		ret = DestroyWindow(m_hWndVR);
		DLogIf(ret == 0, L"DestroyWindow failed (%s)!", HR2Str(HRESULT_FROM_WIN32(GetLastError())));
	}

	ret = UnregisterClassW(g_szClassName, AfxGetApp()->m_hInstance);
	DLog(L"Unregistering '%s' class %s.", g_szClassName, ret ? L"completed successfully" : L"failed");
}

HRESULT CDX9AllocatorPresenter::RegisterWindowClass()
{
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = ::DefWindowProcW;
	wc.hInstance = AfxGetApp()->m_hInstance;
	wc.lpszClassName = g_szClassName;

	if (!RegisterClassExW(&wc)) {
		DWORD dwError = GetLastError();
		HRESULT hr = HRESULT_FROM_WIN32(dwError);
		DLog(L"Registering '%s' class failed (%s)!", g_szClassName, HR2Str(hr));
		return hr;
	}

	DLog(L"Registering '%s' class completed successfully.", g_szClassName);
	return S_OK;
}

#if 0
class CRandom31
{
public:

	CRandom31() {
		m_Seed = 12164;
	}

	void f_SetSeed(int32_t _Seed) {
		m_Seed = _Seed;
	}
	int32_t f_GetSeed() {
		return m_Seed;
	}
	/*
	Park and Miller's psuedo-random number generator.
	*/
	int32_t m_Seed;
	int32_t f_Get() {
		static const int32_t A = 16807;
		static const int32_t M = 2147483647; // 2^31 - 1
		static const int32_t q = M / A;      // M / A
		static const int32_t r = M % A;      // M % A
		m_Seed = A * (m_Seed % q) - r * (m_Seed / q);
		if (m_Seed < 0) {
			m_Seed += M;
		}
		return m_Seed;
	}

	static int32_t fs_Max() {
		return 2147483646;
	}

	double f_GetFloat() {
		return double(f_Get()) * (1.0 / double(fs_Max()));
	}
};

class CVSyncEstimation
{
private:
	class CHistoryEntry
	{
	public:
		CHistoryEntry() {
			m_Time = 0;
			m_ScanLine = -1;
		}
		LONGLONG m_Time;
		int m_ScanLine;
	};

	class CSolution
	{
	public:
		CSolution() {
			m_ScanLines = 1000;
			m_ScanLinesPerSecond = m_ScanLines * 100;
		}
		int m_ScanLines;
		double m_ScanLinesPerSecond;
		double m_SqrSum;

		void f_Mutate(double _Amount, CRandom31 &_Random, int _MinScans) {
			int ToDo = _Random.f_Get() % 10;
			if (ToDo == 0) {
				m_ScanLines = m_ScanLines / 2;
			} else if (ToDo == 1) {
				m_ScanLines = m_ScanLines * 2;
			}

			m_ScanLines = m_ScanLines * (1.0 + (_Random.f_GetFloat() * _Amount) - _Amount * 0.5);
			m_ScanLines = std::max(m_ScanLines, _MinScans);

			if (ToDo == 2) {
				m_ScanLinesPerSecond /= (_Random.f_Get() % 4) + 1;
			} else if (ToDo == 3) {
				m_ScanLinesPerSecond *= (_Random.f_Get() % 4) + 1;
			}

			m_ScanLinesPerSecond *= 1.0 + (_Random.f_GetFloat() * _Amount) - _Amount * 0.5;
		}

		void f_SpawnInto(CSolution &_Other, CRandom31 &_Random, int _MinScans) {
			_Other = *this;
			_Other.f_Mutate(_Random.f_GetFloat() * 0.1, _Random, _MinScans);
		}

		static int fs_Compare(const void *_pFirst, const void *_pSecond) {
			const CSolution *pFirst = (const CSolution *)_pFirst;
			const CSolution *pSecond = (const CSolution *)_pSecond;
			if (pFirst->m_SqrSum < pSecond->m_SqrSum) {
				return -1;
			} else if (pFirst->m_SqrSum > pSecond->m_SqrSum) {
				return 1;
			}
			return 0;
		}
	};

	const static int NumHistory = 128;

	CHistoryEntry m_History[NumHistory];
	int m_iHistory;
	CSolution m_OldSolutions[2];

	CRandom31 m_Random;

	double fp_GetSquareSum(double _ScansPerSecond, double _ScanLines) {
		double SquareSum = 0;
		int nHistory = std::min(m_nHistory, NumHistory);
		int iHistory = m_iHistory - nHistory;
		if (iHistory < 0) {
			iHistory += NumHistory;
		}
		for (int i = 1; i < nHistory; ++i) {
			int iHistory0 = iHistory + i - 1;
			int iHistory1 = iHistory + i;
			if (iHistory0 < 0) {
				iHistory0 += NumHistory;
			}
			iHistory0 = iHistory0 % NumHistory;
			iHistory1 = iHistory1 % NumHistory;
			ASSERT(m_History[iHistory0].m_Time != 0);
			ASSERT(m_History[iHistory1].m_Time != 0);

			double DeltaTime = (m_History[iHistory1].m_Time - m_History[iHistory0].m_Time)/10000000.0;
			double PredictedScanLine = m_History[iHistory0].m_ScanLine + DeltaTime * _ScansPerSecond;
			PredictedScanLine = fmod(PredictedScanLine, _ScanLines);
			double Delta = (m_History[iHistory1].m_ScanLine - PredictedScanLine);
			double DeltaSqr = Delta * Delta;
			SquareSum += DeltaSqr;
		}
		return SquareSum;
	}

	int m_nHistory;
public:

	CVSyncEstimation() {
		m_iHistory = 0;
		m_nHistory = 0;
	}

	void f_AddSample(int _ScanLine, LONGLONG _Time) {
		m_History[m_iHistory].m_ScanLine = _ScanLine;
		m_History[m_iHistory].m_Time = _Time;
		++m_nHistory;
		m_iHistory = (m_iHistory + 1) % NumHistory;
	}

	void f_GetEstimation(double &_RefreshRate, int &_ScanLines, int _ScreenSizeY, int _WindowsRefreshRate) {
		_RefreshRate = 0;
		_ScanLines = 0;

		int iHistory = m_iHistory;
		// We have a full history
		if (m_nHistory > 10) {
			for (int l = 0; l < 5; ++l) {
				const int nSol = 3+5+5+3;
				CSolution Solutions[nSol];

				Solutions[0] = m_OldSolutions[0];
				Solutions[1] = m_OldSolutions[1];
				Solutions[2].m_ScanLines = _ScreenSizeY;
				Solutions[2].m_ScanLinesPerSecond = _ScreenSizeY * _WindowsRefreshRate;

				int iStart = 3;
				for (int i = iStart; i < iStart + 5; ++i) {
					Solutions[0].f_SpawnInto(Solutions[i], m_Random, _ScreenSizeY);
				}
				iStart += 5;
				for (int i = iStart; i < iStart + 5; ++i) {
					Solutions[1].f_SpawnInto(Solutions[i], m_Random, _ScreenSizeY);
				}
				iStart += 5;
				for (int i = iStart; i < iStart + 3; ++i) {
					Solutions[2].f_SpawnInto(Solutions[i], m_Random, _ScreenSizeY);
				}

				int Start = 2;
				if (l == 0) {
					Start = 0;
				}
				for (int i = Start; i < nSol; ++i) {
					Solutions[i].m_SqrSum = fp_GetSquareSum(Solutions[i].m_ScanLinesPerSecond, Solutions[i].m_ScanLines);
				}

				qsort(Solutions, nSol, sizeof(Solutions[0]), &CSolution::fs_Compare);
				for (int i = 0; i < 2; ++i) {
					m_OldSolutions[i] = Solutions[i];
				}
			}

			_ScanLines = m_OldSolutions[0].m_ScanLines + 0.5;
			_RefreshRate = 1.0 / (m_OldSolutions[0].m_ScanLines / m_OldSolutions[0].m_ScanLinesPerSecond);
		} else {
			m_OldSolutions[0].m_ScanLines = _ScreenSizeY;
			m_OldSolutions[1].m_ScanLines = _ScreenSizeY;
		}
	}
};
#endif

HANDLE CDX9AllocatorPresenter::LockD3DDevice()
{
	if (m_pDevice9Ex) {
		_RTL_CRITICAL_SECTION *pCritSec = (_RTL_CRITICAL_SECTION *)((size_t)m_pDevice9Ex.p + sizeof(size_t));

		if (!IsBadReadPtr(pCritSec, sizeof(*pCritSec)) && !IsBadWritePtr(pCritSec, sizeof(*pCritSec))
			&& !IsBadReadPtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))) && !IsBadWritePtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo)))) {
			if (pCritSec->DebugInfo->CriticalSection == pCritSec) {
				EnterCriticalSection(pCritSec);
				return pCritSec->OwningThread;
			}
		}
	}
	return nullptr;
}

void CDX9AllocatorPresenter::UnlockD3DDevice(HANDLE& lockOwner)
{
	if (m_pDevice9Ex) {
		_RTL_CRITICAL_SECTION *pCritSec = (_RTL_CRITICAL_SECTION *)((size_t)m_pDevice9Ex.p + sizeof(size_t));

		if (!IsBadReadPtr(pCritSec, sizeof(*pCritSec)) && !IsBadWritePtr(pCritSec, sizeof(*pCritSec))
			&& !IsBadReadPtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))) && !IsBadWritePtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo)))) {
			if (pCritSec->DebugInfo->CriticalSection == pCritSec && pCritSec->OwningThread == lockOwner) {
				LeaveCriticalSection(pCritSec);
			}
		}
	}
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

HRESULT CDX9AllocatorPresenter::CreateDevice(CString &_Error)
{
	DLog(L"CDX9AllocatorPresenter::CreateDevice()");

	// extern variable
	g_nFrameType = PICT_NONE;

	CAutoLock lock(&m_RenderLock);

	ZeroMemory(m_DetectedFrameTimeHistory, sizeof(m_DetectedFrameTimeHistory));
	ZeroMemory(m_DetectedFrameTimeHistoryHistory, sizeof(m_DetectedFrameTimeHistoryHistory));

	ZeroMemory(m_ldDetectedRefreshRateList, sizeof(m_ldDetectedRefreshRateList));
	ZeroMemory(m_ldDetectedScanlineRateList, sizeof(m_ldDetectedScanlineRateList));

	ZeroMemory(m_Jitters, sizeof(m_Jitters));
	ZeroMemory(m_llSyncOffsets, sizeof(m_llSyncOffsets));

	ZeroMemory(m_TimeChangeHistory, sizeof(m_TimeChangeHistory));
	ZeroMemory(m_ClockChangeHistory, sizeof(m_ClockChangeHistory));

	m_DetectedFrameRate				= 0.0;
	m_DetectedFrameTime				= 0.0;
	m_DetectedFrameTimeStdDev		= 0.0;
	m_DetectedLock					= false;
	m_DetectedFrameTimePos			= 0;

	m_DetectedRefreshRatePos		= 0;
	m_DetectedRefreshTimePrim		= 0.0;
	m_DetectedScanlineTime			= 0.0;
	m_DetectedScanlineTimePrim		= 0.0;
	m_DetectedRefreshRate			= 0.0;

	m_nNextJitter					= 0;
	m_nNextSyncOffset				= 0;
	m_llLastPerf					= 0;
	m_fAvrFps						= 0.0;
	m_fJitterStdDev					= 0.0;
	m_fSyncOffsetStdDev				= 0.0;
	m_fSyncOffsetAvr				= 0.0;
	m_bSyncStatsAvailable			= false;

	m_VBlankEndWait					= 0;
	m_VBlankMin						= 300000;
	m_VBlankMinCalc					= 300000;
	m_VBlankMax						= 0;
	m_VBlankStartWait				= 0;
	m_VBlankWaitTime				= 0;
	m_VBlankLockTime				= 0;
	m_PresentWaitTime				= 0;
	m_PresentWaitTimeMin			= 3000000000;
	m_PresentWaitTimeMax			= 0;

	m_VBlankEndPresent				= -100000;
	m_VBlankStartMeasureTime		= 0;
	m_VBlankStartMeasure			= 0;

	m_PaintTime						= 0;
	m_PaintTimeMin					= 3000000000;
	m_PaintTimeMax					= 0;

	m_RasterStatusWaitTime			= 0;
	m_RasterStatusWaitTimeMin		= 3000000000;
	m_RasterStatusWaitTimeMax		= 0;
	m_RasterStatusWaitTimeMaxCalc	= 0;

	m_ClockDiff						= 0.0;
	m_ClockDiffPrim					= 0.0;
	m_ClockDiffCalc					= 0.0;

	m_ModeratedTimeSpeed			= 1.0;
	m_ModeratedTimeSpeedDiff		= 0.0;
	m_ModeratedTimeSpeedPrim		= 0.0;

	m_ClockTimeChangeHistoryPos		= 0;

	m_bDisplayChanged				= false;

	m_pLine.Release();

	m_bAlphaBitmapEnable = false;
	m_pAlphaBitmapTexture.Release();
	ZeroMemory(&m_AlphaBitmapParams, sizeof(m_AlphaBitmapParams));

	CleanupRenderingEngine();

	if (!m_pD3D9Ex) {
		_Error += L"Failed to create D3D9 Ex\n";
		return E_UNEXPECTED;
	}

	m_AdapterCount = m_pD3D9Ex->GetAdapterCount();

	UINT currentAdapter = GetAdapter(m_pD3D9Ex);
	bool bTryToReset = (currentAdapter == m_CurrentAdapter);

	if (m_bNeedCreateWindow || !bTryToReset) {
		m_bNeedCreateWindow = false;

		if (m_hWndVR) {
			DestroyWindow(m_hWndVR);
			m_hWndVR = nullptr;

			if (m_bIsFullscreen) {
				bTryToReset = false;
			}
		}

		if (!m_bIsFullscreen && m_ExtraSets.iPresentMode) {
			m_hWndVR = CreateWindowExW(
				0,
				g_szClassName,
				nullptr,
				WS_VISIBLE | WS_DISABLED | WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
				0, 0, 1, 1,
				m_hWnd,
				nullptr,
				AfxGetApp()->m_hInstance,
				nullptr
			);

			if (m_hWndVR && !m_windowRect.IsRectEmpty()) {
				SetWindowPos(m_hWndVR, nullptr, m_windowRect.left, m_windowRect.top, m_windowRect.Width(), m_windowRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}
	}

	if (!bTryToReset) {
		m_pDevice9Ex.Release();
		m_pDevice9ExRefresh.Release();
		m_CurrentAdapter = currentAdapter;

		m_GPUUsage.Init(m_D3D9DeviceName, m_D3D9Device);
	}

	HRESULT hr = S_OK;

	m_bCompositionEnabled = FALSE;
	DwmIsCompositionEnabled(&m_bCompositionEnabled);

	// detect FP 16-bit textures support
	m_bFP16Support = SUCCEEDED(m_pD3D9Ex->CheckDeviceFormat(m_CurrentAdapter, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_QUERY_FILTER, D3DRTYPE_VOLUMETEXTURE, D3DFMT_A16B16G16R16F));

	// set surface formats
	switch (m_ExtraSets.iSurfaceFormat) {
	case D3DFMT_A16B16G16R16F:
		if (m_bFP16Support) {
			m_SurfaceFmt = D3DFMT_A16B16G16R16F;
			break;
		}
	case D3DFMT_A2R10G10B10: {
		bool b10bitSupport = SUCCEEDED(m_pD3D9Ex->CheckDeviceFormat(m_CurrentAdapter, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, D3DFMT_A2R10G10B10));
		if (b10bitSupport) {
			m_SurfaceFmt = D3DFMT_A2R10G10B10;
			break;
		}
	}
	default:
		m_SurfaceFmt = D3DFMT_X8R8G8B8;
	}

	D3DDISPLAYMODEEX d3ddmEx = { sizeof(d3ddmEx) };
	m_pD3D9Ex->GetAdapterDisplayModeEx(m_CurrentAdapter, &d3ddmEx, nullptr);
	m_ScreenSize.SetSize(d3ddmEx.Width, d3ddmEx.Height);

	CSize backBufferSize;
	GetMaxResolution(m_pD3D9Ex, backBufferSize);

	if (m_bIsFullscreen && SysVersion::IsWin8orLater() && !SysVersion::IsWin10v1809orLater()) {
		D3DHook::Hook(m_D3D9Device, d3ddmEx.RefreshRate);
	}

	bool b10BitOutput = m_ExtraSets.b10BitOutput;

	{
		HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFOEXW mi = { sizeof(mi) };
		GetMonitorInfoW(hMonitor, (MONITORINFO*)&mi);
		DisplayConfig_t dc = {};
		bool ret = GetDisplayConfig(mi.szDevice, dc);

		if (dc.refreshRate.Numerator && dc.refreshRate.Denominator) {
			m_dRefreshRate = (double)dc.refreshRate.Numerator / (double)dc.refreshRate.Denominator;
		} else {
			m_dRefreshRate = d3ddmEx.RefreshRate;
		}

		if (b10BitOutput) {
			b10BitOutput = (dc.bitsPerChannel >= 10);
		}
	}

	ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));

	if (m_bIsFullscreen) {
		if (b10BitOutput) {
			b10BitOutput = SUCCEEDED(m_pD3D9Ex->CheckDeviceType(m_CurrentAdapter, D3DDEVTYPE_HAL, D3DFMT_A2R10G10B10, D3DFMT_A2R10G10B10, FALSE));
		}

		if (b10BitOutput) {
			m_d3dpp.BackBufferFormat = D3DFMT_A2R10G10B10;
		} else {
			m_d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
		}
		m_d3dpp.Windowed = FALSE;
		m_d3dpp.hDeviceWindow = m_hWnd;
		m_d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
		m_d3dpp.BackBufferCount = 3;
		m_d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		m_d3dpp.FullScreen_RefreshRateInHz = m_refreshRate = d3ddmEx.RefreshRate;
		m_d3dpp.BackBufferWidth = m_ScreenSize.cx;
		m_d3dpp.BackBufferHeight = m_ScreenSize.cy;

		if (!m_FocusThread) {
			m_FocusThread = (CFocusThread*)AfxBeginThread(RUNTIME_CLASS(CFocusThread), 0, 0, 0);
		}

		d3ddmEx.Format = m_d3dpp.BackBufferFormat;

		bTryToReset = bTryToReset && m_pDevice9Ex;
		if (bTryToReset) {
			bTryToReset = SUCCEEDED(hr = m_pDevice9Ex->ResetEx(&m_d3dpp, &d3ddmEx));
			DLog(L"    => ResetEx(fullscreen) : %s", S_OK == hr ? L"S_OK" : GetWindowsErrorMessage(hr, m_hD3D9).GetString());
		}

		if (!bTryToReset) {
			m_pDevice9Ex.Release();
			hr = m_pD3D9Ex->CreateDeviceEx(
					m_CurrentAdapter, D3DDEVTYPE_HAL, m_FocusThread->GetFocusWindow(),
					GetVertexProcessing() | D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_ENABLE_PRESENTSTATS | D3DCREATE_NOWINDOWCHANGES, //D3DCREATE_MANAGED
					&m_d3dpp, &d3ddmEx, &m_pDevice9Ex);
			DLog(L"    => CreateDeviceEx(fullscreen) : %s", S_OK == hr ? L"S_OK" : GetWindowsErrorMessage(hr, m_hD3D9).GetString());
		}

		if (m_pDevice9Ex) {
			m_BackbufferFmt = m_d3dpp.BackBufferFormat;
			m_DisplayFmt = d3ddmEx.Format;
		}
	} else {
		m_d3dpp.Windowed = TRUE;
		m_d3dpp.hDeviceWindow = m_hWndVR ? m_hWndVR : m_hWnd;
		if (m_ExtraSets.iPresentMode == 0 || m_bPreviewMode) {
			m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
		}
		else {
			m_d3dpp.SwapEffect = D3DSWAPEFFECT_FLIPEX;
		}
		m_d3dpp.BackBufferCount = m_d3dpp.SwapEffect == D3DSWAPEFFECT_COPY ? 1 : 3;
		m_d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
		// Desktop composition takes care of the VSYNC
		if (m_bCompositionEnabled) {
			m_d3dpp.PresentationInterval = m_ExtraSets.bVSync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
		} else {
			m_d3dpp.PresentationInterval = m_ExtraSets.bVSync ? D3DPRESENT_INTERVAL_IMMEDIATE : D3DPRESENT_INTERVAL_ONE;
		}

		m_refreshRate = d3ddmEx.RefreshRate;
		m_d3dpp.BackBufferWidth = m_d3dpp.SwapEffect == D3DSWAPEFFECT_COPY ? backBufferSize.cx : m_windowRect.Width();
		m_d3dpp.BackBufferHeight = m_d3dpp.SwapEffect == D3DSWAPEFFECT_COPY ? backBufferSize.cy : m_windowRect.Height();

		bTryToReset = bTryToReset && m_pDevice9Ex;
		if (bTryToReset) {
			bTryToReset = SUCCEEDED(hr = m_pDevice9Ex->ResetEx(&m_d3dpp, nullptr));
			DLog(L"    => ResetEx(window) : %s", S_OK == hr ? L"S_OK" : GetWindowsErrorMessage(hr, m_hD3D9).GetString());
		}

		if (!bTryToReset) {
			m_pDevice9Ex.Release();
			// We can get 0x8876086a here when switching from two displays to one display using Win + P (Windows 7)
			// Cause: We might not reinitialize dx correctly during the switch
			hr = m_pD3D9Ex->CreateDeviceEx(
					m_CurrentAdapter, D3DDEVTYPE_HAL, m_hWndVR ? m_hWndVR : m_hWnd,
					GetVertexProcessing() | D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_ENABLE_PRESENTSTATS, //D3DCREATE_MANAGED
					&m_d3dpp, nullptr, &m_pDevice9Ex);
			DLog(L"    => CreateDeviceEx(window) : %s", S_OK == hr ? L"S_OK" : GetWindowsErrorMessage(hr, m_hD3D9).GetString());
		}

		if (m_pDevice9Ex) {
			m_DisplayFmt = d3ddmEx.Format;
		}

		m_BackbufferFmt = m_d3dpp.BackBufferFormat;
	}

	if (m_pDevice9Ex) {
		while (hr == D3DERR_DEVICELOST) {
			DLog(L"    => D3DERR_DEVICELOST. Trying to Reset.");
			hr = m_pDevice9Ex->CheckDeviceState(m_hWndVR ? m_hWndVR : m_hWnd);
		}
		if (hr == D3DERR_DEVICENOTRESET) {
			DLog(L"    => D3DERR_DEVICENOTRESET");
			hr = m_pDevice9Ex->ResetEx(&m_d3dpp, m_bIsFullscreen ? &d3ddmEx : nullptr);
		}

		if (m_pDevice9Ex) {
			m_pDevice9Ex->SetGPUThreadPriority(7);
		}
	}

	DLog(L"    => CreateDevice() : %s", HR2Str(hr));

	if (FAILED(hr)) {
		_Error += L"CreateDevice() failed\n";
		_Error.AppendFormat(L"Error code: %s\n", HR2Str(hr));

		return hr;
	}

	if (!bTryToReset) {
#if DXVAHDVP
		m_pDXVAHD_VP.Release();
		m_pDXVAHD_Device.Release();
#endif
#if DXVA2VP
		m_pDXVA2_VP.Release();
		m_pDXVA2_VPService.Release();
#endif
	}

	m_MainThreadId = GetCurrentThreadId();

	// Initialize the rendering engine
	InitRenderingEngine();

	if (m_bEnableSubPic) {
		hr = InitializeISR(_Error, m_bIsFullscreen ? m_ScreenSize : backBufferSize);
		if (FAILED(hr)) {
			return hr;
		}
	}

	m_LastAdapterCheck = GetPerfCounter();

	m_MonitorName.Empty();
	m_nMonitorHorRes = m_nMonitorVerRes = 0;

	HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFOEXW mi;
	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);
	if (GetMonitorInfoW(hMonitor, &mi)) {
		ReadDisplay(mi.szDevice, &m_MonitorName, &m_nMonitorHorRes, &m_nMonitorVerRes);
		m_rcMonitor = mi.rcMonitor;
	}

	m_strProcessingFmt = GetD3DFormatStr(m_SurfaceFmt);
	m_strBackbufferFmt = GetD3DFormatStr(m_BackbufferFmt);

	if (!m_pDevice9ExRefresh) {
		D3DPRESENT_PARAMETERS d3dpp = { 0 };
		d3dpp.Windowed = TRUE;
		d3dpp.BackBufferWidth = 640;
		d3dpp.BackBufferHeight = 480;
		d3dpp.BackBufferCount = 0;
		d3dpp.BackBufferFormat = m_d3dpp.BackBufferFormat;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.Flags = D3DPRESENTFLAG_VIDEO;

		hr = m_pD3D9Ex->CreateDeviceEx(
				m_CurrentAdapter, D3DDEVTYPE_HAL, GetShellWindow(),
				D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
				&d3dpp, nullptr, &m_pDevice9ExRefresh);
		DLog(L"    => CreateDeviceEx(m_pDevice9ExRefresh) : %s", S_OK == hr ? L"S_OK" : GetWindowsErrorMessage(hr, m_hD3D9).GetString());
	}

	if (!bTryToReset) {
		m_Font3D.InvalidateDeviceObjects();
		HRESULT hr2 = m_Font3D.InitDeviceObjects(m_pDevice9Ex);
		DLogIf(FAILED(hr2), L"m_Font3D initialization failed with error %s", HR2Str(hr2));
		m_bFont3DUpdate = true;

		OnResetDevice();
	}

	return S_OK;
}

HRESULT CDX9AllocatorPresenter::AllocSurfaces()
{
	CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);

	return CreateVideoSurfaces();
}

HRESULT CDX9AllocatorPresenter::ResetD3D9Device()
{
	DLog(L"CDX9AllocatorPresenter::ResetD3D9Device()");

	CAutoLock cRenderLock(&m_RenderLock);

	CSize backBufferSize;
	GetMaxResolution(m_pD3D9Ex, backBufferSize);

	D3DDISPLAYMODEEX d3ddmEx = { sizeof(d3ddmEx) };
	m_pD3D9Ex->GetAdapterDisplayModeEx(m_CurrentAdapter, &d3ddmEx, nullptr);

	if (m_ScreenSize.cx != d3ddmEx.Width || m_ScreenSize.cy != d3ddmEx.Height) {
		m_ScreenSize.SetSize(d3ddmEx.Width, d3ddmEx.Height);
		FreeScreenTextures();
	}

	HRESULT hr = E_FAIL;

	if (m_bIsFullscreen) {
		d3ddmEx.Format = m_d3dpp.BackBufferFormat;
		m_d3dpp.FullScreen_RefreshRateInHz = m_refreshRate = d3ddmEx.RefreshRate;
		m_d3dpp.BackBufferWidth = m_ScreenSize.cx;
		m_d3dpp.BackBufferHeight = m_ScreenSize.cy;

		hr = m_pDevice9Ex->ResetEx(&m_d3dpp, &d3ddmEx);
	} else {
		m_refreshRate = d3ddmEx.RefreshRate;
		m_d3dpp.BackBufferWidth = m_d3dpp.SwapEffect == D3DSWAPEFFECT_COPY ? backBufferSize.cx : m_windowRect.Width();
		m_d3dpp.BackBufferHeight = m_d3dpp.SwapEffect == D3DSWAPEFFECT_COPY ? backBufferSize.cy : m_windowRect.Height();

		hr = m_pDevice9Ex->ResetEx(&m_d3dpp, nullptr);
	}

	DLog(L"    => ResetEx() return : %s", S_OK == hr ? L"S_OK" : GetWindowsErrorMessage(hr, m_hD3D9).GetString());

	while (hr == D3DERR_DEVICELOST) {
		DLog(L"    => D3DERR_DEVICELOST. Trying to Reset.");
		hr = m_pDevice9Ex->CheckDeviceState(m_hWndVR ? m_hWndVR : m_hWnd);
	}

	if (SUCCEEDED(hr) && m_bEnableSubPic) {
		CString _Error;
		hr = InitializeISR(_Error, m_bIsFullscreen ? m_ScreenSize : backBufferSize);
	}

	return hr;
}

HRESULT CDX9AllocatorPresenter::InitializeISR(CString& _Error, const CSize& desktopSize)
{
	CComPtr<ISubPicProvider> pSubPicProvider;
	if (m_pSubPicQueue) {
		m_pSubPicQueue->GetSubPicProvider(&pSubPicProvider);
	}

	InitMaxSubtitleTextureSize(m_SubpicSets.iMaxTexWidth, desktopSize);

	if (m_pSubPicAllocator) {
		m_pSubPicAllocator->ChangeDevice(m_pDevice9Ex);
	} else {
		m_pSubPicAllocator = DNew CDX9SubPicAllocator(m_pDevice9Ex, m_maxSubtitleTextureSize, false);
	}
	if (!m_pSubPicAllocator) {
		_Error += L"CDX9SubPicAllocator failed\n";
		return E_FAIL;
	}

	HRESULT hr = S_OK;
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

	return hr;
}

void CDX9AllocatorPresenter::DeleteSurfaces()
{
	CAutoLock cAutoLock(this);
	CAutoLock cRenderLock(&m_RenderLock);

	FreeVideoSurfaces();
}

UINT CDX9AllocatorPresenter::GetAdapter(IDirect3D9* pD3D)
{
	if (m_hWnd == nullptr || pD3D == nullptr) {
		return D3DADAPTER_DEFAULT;
	}

	m_D3D9Device.Empty();
	m_D3D9DeviceName.Empty();
	m_D3D9VendorId = 0;

	if (pD3D->GetAdapterCount() > 1 && m_ExtraSets.sD3DRenderDevice.GetLength() > 0) {
		WCHAR strGUID[50] = { 0 };
		D3DADAPTER_IDENTIFIER9 adapterIdentifier;

		for (UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp) {
			if (pD3D->GetAdapterIdentifier(adp, 0, &adapterIdentifier) == S_OK) {
				if ((::StringFromGUID2(adapterIdentifier.DeviceIdentifier, strGUID, 50) > 0) && (m_ExtraSets.sD3DRenderDevice == strGUID)) {
					m_D3D9Device     = adapterIdentifier.Description;
					m_D3D9DeviceName = adapterIdentifier.DeviceName;
					m_D3D9VendorId   = adapterIdentifier.VendorId;

					m_D3D9Device.Trim();
					m_D3D9DeviceName.Trim();

					return adp;
				}
			}
		}
	}

	HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor == nullptr) {
		return D3DADAPTER_DEFAULT;
	}

	for (UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp) {
		HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
		if (hAdpMon == hMonitor) {
			D3DADAPTER_IDENTIFIER9 adapterIdentifier;
			if (pD3D->GetAdapterIdentifier(adp, 0, &adapterIdentifier) == S_OK) {
				m_D3D9Device     = adapterIdentifier.Description;
				m_D3D9DeviceName = adapterIdentifier.DeviceName;
				m_D3D9VendorId   = adapterIdentifier.VendorId;

				m_D3D9Device.Trim();
				m_D3D9DeviceName.Trim();
			}
			return adp;
		}
	}

	return D3DADAPTER_DEFAULT;
}

DWORD CDX9AllocatorPresenter::GetVertexProcessing()
{
	HRESULT hr;
	D3DCAPS9 caps;

	hr = m_pD3D9Ex->GetDeviceCaps(m_CurrentAdapter, D3DDEVTYPE_HAL, &caps);
	if (FAILED(hr)) {
		return D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
			caps.VertexShaderVersion < D3DVS_VERSION(2, 0)) {
		return D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	return D3DCREATE_HARDWARE_VERTEXPROCESSING;
}

// IAllocatorPresenter

void CDX9AllocatorPresenter::CalculateJitter(LONGLONG PerfCounter)
{
	// Calculate the jitter!
	LONGLONG curJitter = PerfCounter - m_llLastPerf;
	if (llabs(curJitter) <= (INT_MAX / NB_JITTER)) { // filter out very large jetter values
		m_nNextJitter = (m_nNextJitter + 1) % NB_JITTER;
		m_Jitters[m_nNextJitter] = (int)curJitter;

		m_MaxJitter = MINLONG64;
		m_MinJitter = MAXLONG64;

		// Calculate the real FPS
		int JitterSum = 0;
		int OneSecSum = 0;
		int OneSecCount = 0;
		for (unsigned i = 0; i < NB_JITTER; i++) {
			JitterSum += m_Jitters[i];
			if (OneSecSum < 10000000) {
				unsigned index = (NB_JITTER + m_nNextJitter - i) % NB_JITTER;
				OneSecSum += m_Jitters[index];
				OneSecCount++;
			}
		}

		int FrameTimeMean = JitterSum / NB_JITTER;
		__int64 DeviationSum = 0;
		for (const auto& jitter : m_Jitters) {
			int Deviation = jitter - FrameTimeMean;
			DeviationSum += (__int64)Deviation * Deviation;
			if (m_MaxJitter < Deviation) m_MaxJitter = Deviation;
			if (m_MinJitter > Deviation) m_MinJitter = Deviation;
		}

		m_iJitterMean = FrameTimeMean;
		m_fJitterStdDev = sqrt(DeviationSum / NB_JITTER);
		m_fAvrFps = 10000000.0 * OneSecCount / OneSecSum;
	}

	m_llLastPerf = PerfCounter;
}

bool CDX9AllocatorPresenter::GetVBlank(int &_ScanLine, int &_bInVBlank, bool _bMeasureTime)
{
	LONGLONG llPerf = 0;
	if (_bMeasureTime) {
		llPerf = GetPerfCounter();
	}

	int ScanLine = 0;
	_ScanLine = 0;
	_bInVBlank = 0;
	D3DRASTER_STATUS RasterStatus;
	if (m_pDevice9Ex->GetRasterStatus(0, &RasterStatus) != S_OK) {
		return false;
	}
	ScanLine = RasterStatus.ScanLine;
	_bInVBlank = RasterStatus.InVBlank;

	if (_bMeasureTime) {
		m_VBlankMax = std::max(m_VBlankMax, ScanLine);
		if (ScanLine != 0 && !_bInVBlank) {
			m_VBlankMinCalc = std::min(m_VBlankMinCalc, ScanLine);
		}
		m_VBlankMin = m_VBlankMax - m_ScreenSize.cy;
	}
	if (_bInVBlank) {
		_ScanLine = 0;
	} else if (m_VBlankMin != 300000) {
		_ScanLine = ScanLine - m_VBlankMin;
	} else {
		_ScanLine = ScanLine;
	}

	if (_bMeasureTime) {
		LONGLONG Time = GetPerfCounter() - llPerf;
		if (Time > 5000000) { // 0.5 sec
			DLog(L"GetVBlank too long (%f sec)", Time / 10000000.0);
		}
		m_RasterStatusWaitTimeMaxCalc = std::max(m_RasterStatusWaitTimeMaxCalc, Time);
	}

	return true;
}

bool CDX9AllocatorPresenter::WaitForVBlankRange(int &_RasterStart, int _RasterSize, bool _bWaitIfInside, bool _bNeedAccurate, bool _bMeasure, HANDLE& lockOwner)
{
	if (_bMeasure) {
		m_RasterStatusWaitTimeMaxCalc = 0;
	}
	bool bWaited = false;
	int ScanLine = 0;
	int InVBlank = 0;
	LONGLONG llPerf = 0;
	if (_bMeasure) {
		llPerf = GetPerfCounter();
	}
	GetVBlank(ScanLine, InVBlank, _bMeasure);
	if (_bMeasure) {
		m_VBlankStartWait = ScanLine;
	}

	static bool bOneWait = true;
	if (bOneWait && _bMeasure) {
		bOneWait = false;
		// If we are already in the wanted interval we need to wait until we aren't, this improves sync when for example you are playing 23.976 Hz material on a 24 Hz refresh rate
		int nInVBlank = 0;
		for (int i = 0; i < 50; i++) { // to prevent infinite loop
			if (!GetVBlank(ScanLine, InVBlank, _bMeasure)) {
				break;
			}

			if (InVBlank && nInVBlank == 0) {
				nInVBlank = 1;
			} else if (!InVBlank && nInVBlank == 1) {
				nInVBlank = 2;
			} else if (InVBlank && nInVBlank == 2) {
				nInVBlank = 3;
			} else if (!InVBlank && nInVBlank == 3) {
				break;
			}
		}
	}
	if (_bWaitIfInside) {
		int ScanLineDiff = long(ScanLine) - _RasterStart;
		if (ScanLineDiff > m_ScreenSize.cy / 2) {
			ScanLineDiff -= m_ScreenSize.cy;
		} else if (ScanLineDiff < -m_ScreenSize.cy / 2) {
			ScanLineDiff += m_ScreenSize.cy;
		}

		if (ScanLineDiff >= 0 && ScanLineDiff <= _RasterSize) {
			bWaited = true;
			// If we are already in the wanted interval we need to wait until we aren't, this improves sync when for example you are playing 23.976 Hz material on a 24 Hz refresh rate
			int LastLineDiff = ScanLineDiff;
			for (int i = 0; i < 50; i++) { // to prevent infinite loop
				if (!GetVBlank(ScanLine, InVBlank, _bMeasure)) {
					break;
				}
				int ScanLineDiff = long(ScanLine) - _RasterStart;
				if (ScanLineDiff > m_ScreenSize.cy / 2) {
					ScanLineDiff -= m_ScreenSize.cy;
				} else if (ScanLineDiff < -m_ScreenSize.cy / 2) {
					ScanLineDiff += m_ScreenSize.cy;
				}
				if (!(ScanLineDiff >= 0 && ScanLineDiff <= _RasterSize) || (LastLineDiff < 0 && ScanLineDiff > 0)) {
					break;
				}
				LastLineDiff = ScanLineDiff;
				Sleep(1); // Just sleep
			}
		}
	}
	double RefreshRate = GetRefreshRate();
	LONG ScanLines = GetScanLines();
	int MinRange = std::clamp(long(0.0015 * double(ScanLines) * RefreshRate + 0.5), 5L, ScanLines/3); // 1.5 ms or max 33 % of Time
	int NoSleepStart = _RasterStart - MinRange;
	int NoSleepRange = MinRange;
	if (NoSleepStart < 0) {
		NoSleepStart += m_ScreenSize.cy;
	}

	int MinRange2 = std::clamp(long(0.0050 * double(ScanLines) * RefreshRate + 0.5), 5L, ScanLines/3); // 5 ms or max 33 % of Time
	int D3DDevLockStart = _RasterStart - MinRange2;
	int D3DDevLockRange = MinRange2;
	if (D3DDevLockStart < 0) {
		D3DDevLockStart += m_ScreenSize.cy;
	}

	int ScanLineDiff = ScanLine - _RasterStart;
	if (ScanLineDiff > m_ScreenSize.cy / 2) {
		ScanLineDiff -= m_ScreenSize.cy;
	} else if (ScanLineDiff < -m_ScreenSize.cy / 2) {
		ScanLineDiff += m_ScreenSize.cy;
	}
	int LastLineDiff = ScanLineDiff;


	int ScanLineDiffSleep = long(ScanLine) - NoSleepStart;
	if (ScanLineDiffSleep > m_ScreenSize.cy / 2) {
		ScanLineDiffSleep -= m_ScreenSize.cy;
	} else if (ScanLineDiffSleep < -m_ScreenSize.cy / 2) {
		ScanLineDiffSleep += m_ScreenSize.cy;
	}
	int LastLineDiffSleep = ScanLineDiffSleep;


	int ScanLineDiffLock = long(ScanLine) - D3DDevLockStart;
	if (ScanLineDiffLock > m_ScreenSize.cy / 2) {
		ScanLineDiffLock -= m_ScreenSize.cy;
	} else if (ScanLineDiffLock < -m_ScreenSize.cy / 2) {
		ScanLineDiffLock += m_ScreenSize.cy;
	}
	int LastLineDiffLock = ScanLineDiffLock;

	LONGLONG llPerfLock = 0;

	for (int i = 0; i < 50; i++) { // to prevent infinite loop
		if (!GetVBlank(ScanLine, InVBlank, _bMeasure)) {
			break;
		}
		int ScanLineDiff = long(ScanLine) - _RasterStart;
		if (ScanLineDiff > m_ScreenSize.cy / 2) {
			ScanLineDiff -= m_ScreenSize.cy;
		} else if (ScanLineDiff < -m_ScreenSize.cy / 2) {
			ScanLineDiff += m_ScreenSize.cy;
		}
		if ((ScanLineDiff >= 0 && ScanLineDiff <= _RasterSize) || (LastLineDiff < 0 && ScanLineDiff > 0)) {
			break;
		}

		LastLineDiff = ScanLineDiff;

		bWaited = true;

		int ScanLineDiffLock = long(ScanLine) - D3DDevLockStart;
		if (ScanLineDiffLock > m_ScreenSize.cy / 2) {
			ScanLineDiffLock -= m_ScreenSize.cy;
		} else if (ScanLineDiffLock < -m_ScreenSize.cy / 2) {
			ScanLineDiffLock += m_ScreenSize.cy;
		}

		if ((ScanLineDiffLock >= 0 && ScanLineDiffLock <= D3DDevLockRange) || (LastLineDiffLock < 0 && ScanLineDiffLock > 0)) {
			if (!lockOwner && _bMeasure) {
				llPerfLock = GetPerfCounter();
				lockOwner = LockD3DDevice();
			}
		}
		LastLineDiffLock = ScanLineDiffLock;


		int ScanLineDiffSleep = long(ScanLine) - NoSleepStart;
		if (ScanLineDiffSleep > m_ScreenSize.cy / 2) {
			ScanLineDiffSleep -= m_ScreenSize.cy;
		} else if (ScanLineDiffSleep < -m_ScreenSize.cy / 2) {
			ScanLineDiffSleep += m_ScreenSize.cy;
		}

		if (!((ScanLineDiffSleep >= 0 && ScanLineDiffSleep <= NoSleepRange) || (LastLineDiffSleep < 0 && ScanLineDiffSleep > 0)) || !_bNeedAccurate) {
			//TRACE("%d\n", RasterStatus.ScanLine);
			Sleep(1); // Don't sleep for the last 1.5 ms scan lines, so we get maximum precision
		}
		LastLineDiffSleep = ScanLineDiffSleep;
	}
	_RasterStart = ScanLine;
	if (_bMeasure) {
		m_VBlankEndWait = ScanLine;
		m_VBlankWaitTime = GetPerfCounter() - llPerf;

		if (lockOwner) {
			m_VBlankLockTime = GetPerfCounter() - llPerfLock;
		} else {
			m_VBlankLockTime = 0;
		}

		m_RasterStatusWaitTime = m_RasterStatusWaitTimeMaxCalc;
		expand_range(m_RasterStatusWaitTime, m_RasterStatusWaitTimeMin, m_RasterStatusWaitTimeMax);
	}

	return bWaited;
}

int CDX9AllocatorPresenter::GetVBlackPos()
{
	const int WaitRange = std::max(m_ScreenSize.cy / 40, 5L);
	if (!m_bCompositionEnabled) {
		const int MinRange = std::clamp(long(0.005 * double(m_ScreenSize.cy) * GetRefreshRate() + 0.5), 5L, m_ScreenSize.cy / 3); // 5  ms or max 33 % of Time
		const int WaitFor = m_ScreenSize.cy - (MinRange + WaitRange);
		return WaitFor;
	} else {
		const int WaitFor = m_ScreenSize.cy / 2;
		return WaitFor;
	}
}

bool CDX9AllocatorPresenter::WaitForVBlank(bool &_Waited, HANDLE& lockOwner)
{
	if (!m_ExtraSets.bVSyncInternal) {
		_Waited = true;
		m_VBlankWaitTime = 0;
		m_VBlankLockTime = 0;
		m_VBlankEndWait = 0;
		m_VBlankStartWait = 0;
		return true;
	}

	const auto bCompositionEnabled = m_bCompositionEnabled;
	int WaitFor = GetVBlackPos();

	if (!bCompositionEnabled) {
		_Waited = WaitForVBlankRange(WaitFor, 0, false, true, true, lockOwner);
		return true;
	} else {
		// Instead we wait for VBlack after the present, this seems to fix the stuttering problem. It's also possible to fix by removing the Sleep above, but that isn't an option.
		WaitForVBlankRange(WaitFor, 0, false, true, true, lockOwner);
		return false;
	}
}

STDMETHODIMP CDX9AllocatorPresenter::DisableSubPicInitialization()
{
	if (m_pDevice9Ex) {
		// must be called before calling CreateRenderer
		return E_ILLEGAL_METHOD_CALL;
	}

	m_bEnableSubPic = false;

	return S_OK;
}

STDMETHODIMP CDX9AllocatorPresenter::EnablePreviewModeInitialization()
{
	if (m_pDevice9Ex) {
		// must be called before calling CreateRenderer
		return E_ILLEGAL_METHOD_CALL;
	}

	m_bPreviewMode = true;
	m_bNeedCreateWindow = false;

	return S_OK;
}

STDMETHODIMP_(bool) CDX9AllocatorPresenter::Paint(bool fAll)
{
	if (m_bPendingResetDevice) {
		SendResetRequest();
		return false;
	}

	if (m_bResizingDevice) {
		return false;
	}

#if 0
	if (TryEnterCriticalSection (&(CRITICAL_SECTION &)(*((CCritSec *)this)))) {
		LeaveCriticalSection((&(CRITICAL_SECTION &)(*((CCritSec *)this))));
	} else {
		__debugbreak();
	}
#endif

	LONGLONG StartPaint = GetPerfCounter();
	CAutoLock cRenderLock(&m_RenderLock);

	if (m_windowRect.right <= m_windowRect.left || m_windowRect.bottom <= m_windowRect.top
			|| m_nativeVideoSize.cx <= 0 || m_nativeVideoSize.cy <= 0) {
		if (m_OrderedPaint) {
			--m_OrderedPaint;
		}

		return false;
	}

	HRESULT hr = S_OK;
	const bool bStereo3DTransform = (m_Stereo3DSets.iTransform != STEREO3D_AsIs && !m_bPreviewMode);

	m_pDevice9Ex->BeginScene();

	CComPtr<IDirect3DSurface9> pBackBuffer;
	m_pDevice9Ex->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

	// Clear the backbuffer
	m_pDevice9Ex->SetRenderTarget(0, pBackBuffer);
	hr = m_pDevice9Ex->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);

	CRect rSrcVid(CPoint(0, 0), m_nativeVideoSize);
	CRect rDstVid(m_videoRect);

	CRect rSrcPri(CPoint(0, 0), m_windowRect.Size());
	CRect rDstPri(m_windowRect);

	// Render the current video frame
	if (bStereo3DTransform) {
		hr = RenderVideo(nullptr, rSrcVid, rDstVid);
	} else {
		hr = RenderVideo(pBackBuffer, rSrcVid, rDstVid);
	}

	// paint subtitles on the backbuffer
	if (m_bEnableSubPic) {
		MediaOffset3D offset3D = { INVALID_TIME };
		{
			std::unique_lock<std::mutex> lock(m_mutexOffsetQueue);
			if (!m_mediaOffsetQueue.empty()) {
				auto selected = m_mediaOffsetQueue.begin();
				for (auto it = m_mediaOffsetQueue.begin(); it != m_mediaOffsetQueue.end(); it++) {
					if (it->timestamp > m_rtNow) {
						break;
					}

					offset3D = *it;
					selected = it;
				}
				m_mediaOffsetQueue.erase(m_mediaOffsetQueue.begin(), ++selected);
			}
		}

		int xOffsetInPixels = 0;
		if (m_Stereo3DSets.iMode == SUBPIC_STEREO_SIDEBYSIDE
				|| m_Stereo3DSets.iMode == SUBPIC_STEREO_TOPANDBOTTOM
				|| m_Stereo3DSets.iTransform == STEREO3D_HalfOverUnder_to_Interlace) {
			if (offset3D.timestamp != INVALID_TIME) {
				int idx = m_nCurrentSubtitlesStream;
				if (!m_stereo_subtitle_offset_ids.empty() && (size_t)m_nCurrentSubtitlesStream < m_stereo_subtitle_offset_ids.size()) {
					idx = m_stereo_subtitle_offset_ids[m_nCurrentSubtitlesStream];
				}
				if (idx < offset3D.offset.offset_count && offset3D.offset.offset[idx]) {
					m_nStereoOffsetInPixels = offset3D.offset.offset[idx];
				}
			}

			xOffsetInPixels = (m_bMVC_Base_View_R_flag != m_Stereo3DSets.bSwapLR) ? -m_nStereoOffsetInPixels : m_nStereoOffsetInPixels;
		}
		AlphaBltSubPic(rSrcPri, rDstVid, xOffsetInPixels);
	}

	if (m_bAlphaBitmapEnable && m_pAlphaBitmapTexture) {
		const int xOffsetInPixels = 4;

		CRect rcDst(rSrcPri);
		if (m_Stereo3DSets.iMode == SUBPIC_STEREO_SIDEBYSIDE) {
			CRect rcTemp(rcDst);
			rcTemp.right -= rcTemp.Width() / 2;
			rcTemp.OffsetRect(-xOffsetInPixels, 0);

			AlphaBlt(rSrcPri, rcTemp, m_pAlphaBitmapTexture);

			rcDst.left += rcDst.Width() / 2;
			rcTemp.OffsetRect(xOffsetInPixels * 2, 0);
		} else if (m_Stereo3DSets.iMode == SUBPIC_STEREO_TOPANDBOTTOM || m_Stereo3DSets.iTransform == STEREO3D_HalfOverUnder_to_Interlace) {
			CRect rcTemp(rcDst);
			rcTemp.bottom -= rcTemp.Height() / 2;
			rcTemp.OffsetRect(-xOffsetInPixels, 0);

			AlphaBlt(rSrcPri, rcTemp, m_pAlphaBitmapTexture);

			rcDst.top += rcDst.Height() / 2;
			rcTemp.OffsetRect(xOffsetInPixels * 2, 0);
		}

		AlphaBlt(rSrcPri, rcDst, m_pAlphaBitmapTexture);
	}

	if (bStereo3DTransform) {
		Stereo3DTransform(pBackBuffer, rDstVid);
	}

	if (m_ExtraSets.iDisplayStats) {
		DrawStats();
	}

	m_pDevice9Ex->EndScene();

	bool bDoVSyncInPresent = !m_bCompositionEnabled || !m_ExtraSets.bVSyncInternal;

	LONGLONG PresentWaitTime = 0;

	CComPtr<IDirect3DQuery9> pEventQuery;

	m_pDevice9Ex->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);
	if (pEventQuery) {
		pEventQuery->Issue(D3DISSUE_END);
	}

	if (m_ExtraSets.bFlushGPUBeforeVSync && pEventQuery) {
		LONGLONG llPerf = GetPerfCounter();
		BOOL Data;
		//Sleep(5);
		LONGLONG FlushStartTime = GetPerfCounter();
		while (S_FALSE == pEventQuery->GetData( &Data, sizeof(Data), D3DGETDATA_FLUSH )) {
			if (!m_ExtraSets.bFlushGPUWait) {
				break;
			}
			Sleep(1);
			if (GetPerfCounter() - FlushStartTime > 500000) {
				break;    // timeout after 50 ms
			}
		}
		if (m_ExtraSets.bFlushGPUWait) {
			m_WaitForGPUTime = GetPerfCounter() - llPerf;
		} else {
			m_WaitForGPUTime = 0;
		}
	} else {
		m_WaitForGPUTime = 0;
	}

	if (fAll) {
		m_PaintTime = (GetPerfCounter() - StartPaint);
		expand_range(m_PaintTime, m_PaintTimeMin, m_PaintTimeMax);
	}

	bool bWaited = false;
	HANDLE lockOwner = nullptr;
	if (fAll) {
		// Only sync to refresh when redrawing all
		bool bTest = WaitForVBlank(bWaited, lockOwner);
		ASSERT(bTest == bDoVSyncInPresent);
		if (!bDoVSyncInPresent) {
			LONGLONG Time = GetPerfCounter();
			OnVBlankFinished(fAll, Time);
			if (m_OrderedPaint) {
				CalculateJitter(Time);
			}
		}
	}

	// Create a device pointer m_pd3dDevice

	// Create a query object
	{
		CComPtr<IDirect3DQuery9> pEventQuery;
		m_pDevice9Ex->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);

		LONGLONG llPerf = GetPerfCounter();
		if (m_d3dpp.SwapEffect == D3DSWAPEFFECT_COPY) {
			hr = m_pDevice9Ex->PresentEx(rSrcPri, rDstPri, nullptr, nullptr, 0);
		} else {
			hr = m_pDevice9Ex->PresentEx(nullptr, nullptr, nullptr, nullptr, 0);
		}

		// Issue an End event
		if (pEventQuery) {
			pEventQuery->Issue(D3DISSUE_END);
		}

		BOOL Data;

		if (m_ExtraSets.bFlushGPUAfterPresent && pEventQuery) {
			LONGLONG FlushStartTime = GetPerfCounter();
			while (S_FALSE == pEventQuery->GetData( &Data, sizeof(Data), D3DGETDATA_FLUSH )) {
				if (!m_ExtraSets.bFlushGPUWait) {
					break;
				}
				if (GetPerfCounter() - FlushStartTime > 500000) {
					break;    // timeout after 50 ms
				}
			}
		}

		int ScanLine;
		int bInVBlank;
		GetVBlank(ScanLine, bInVBlank, false);

		if (fAll && m_OrderedPaint) {
			m_VBlankEndPresent = ScanLine;
		}

		for (int i = 0; i < 50 && (ScanLine == 0 || bInVBlank); i++) { // to prevent infinite loop
			if (!GetVBlank(ScanLine, bInVBlank, false)) {
				break;
			}
		}

		m_VBlankStartMeasureTime = GetPerfCounter();
		m_VBlankStartMeasure = ScanLine;

		if (fAll && bDoVSyncInPresent) {
			m_PresentWaitTime = (GetPerfCounter() - llPerf) + PresentWaitTime;
		} else {
			m_PresentWaitTime = 0;
		}
		expand_range(m_PresentWaitTime, m_PresentWaitTimeMin, m_PresentWaitTimeMax);
	}

	if (bDoVSyncInPresent) {
		LONGLONG Time = GetPerfCounter();
		if (m_OrderedPaint) {
			CalculateJitter(Time);
		}
		OnVBlankFinished(fAll, Time);
	}

	if (lockOwner) {
		UnlockD3DDevice(lockOwner);
	}

	/*if (!bWaited)
	{
		bWaited = true;
		WaitForVBlank(bWaited);
		TRACE("Double VBlank\n");
		ASSERT(bWaited);
		if (!bDoVSyncInPresent)
		{
			CalculateJitter();
			OnVBlankFinished(fAll);
		}
	}*/

	if (!m_bPendingResetDevice) {
		bool bResetDevice = false;

		if (hr == S_PRESENT_OCCLUDED && m_bIsFullscreen) {
			// hmm. previous device reset was bad?
			DLog(L"CDX9AllocatorPresenter::Paint() : S_PRESENT_OCCLUDED - need Reset Device");
			bResetDevice = true;
		}

		if (hr == D3DERR_DEVICELOST && m_pDevice9Ex->CheckDeviceState(m_hWndVR ? m_hWndVR : m_hWnd) == D3DERR_DEVICENOTRESET) {
			DLog(L"CDX9AllocatorPresenter::Paint() : D3DERR_DEVICELOST - need Reset Device");
			bResetDevice = true;
		}

		//if (hr == S_PRESENT_MODE_CHANGED)
		//{
		//	DLog(L"Reset Device: D3D Device mode changed");
		//	bResetDevice = true;
		//}

		if (m_bNeedResetDevice) {
			m_bNeedResetDevice = false;
			DLog(L"CDX9AllocatorPresenter::Paint() : Settings Changed - need Reset Device");
			bResetDevice = true;
		}

		if (!bResetDevice) {
			BOOL bCompositionEnabled = FALSE;
			DwmIsCompositionEnabled(&bCompositionEnabled);

			if (bCompositionEnabled != m_bCompositionEnabled) {
				if (m_bIsFullscreen) {
					m_bCompositionEnabled = bCompositionEnabled;
				} else {
					DLog(L"CDX9AllocatorPresenter::Paint() : DWM Composition Changed - need Reset Device");
					bResetDevice = true;
				}
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
				if (m_CurrentAdapter != GetAdapter(m_pD3D9Ex)) {
					DLog(L"CDX9AllocatorPresenter::Paint() : D3D adapter changed - need Reset Device");
					bResetDevice = true;
				}
#ifdef _DEBUG
				else {
					ASSERT(m_pD3D9Ex->GetAdapterMonitor(m_CurrentAdapter) == m_pD3D9Ex->GetAdapterMonitor(GetAdapter(m_pD3D9Ex)));
				}
#endif
			}
		}

		if (bResetDevice) {
			m_bPendingResetDevice = true;
			SendResetRequest();
		}
	}

	if (m_OrderedPaint) {
		--m_OrderedPaint;
	} else {
		//TRACE("UNORDERED PAINT!!!!!!\n");
	}
	return true;
}

double CDX9AllocatorPresenter::GetFrameTime()
{
	if (m_DetectedLock) {
		return m_DetectedFrameTime;
	}

	return m_rtTimePerFrame / 10000000.0;
}

double CDX9AllocatorPresenter::GetFrameRate()
{
	if (m_DetectedLock) {
		return m_DetectedFrameRate;
	}

	return m_rtTimePerFrame ? (10000000.0 / m_rtTimePerFrame) : 0;
}

void CDX9AllocatorPresenter::SendResetRequest()
{
	if (!m_bDeviceResetRequested) {
		m_bDeviceResetRequested = true;
		AfxGetApp()->m_pMainWnd->PostMessage(WM_RESET_DEVICE);
	}
}

STDMETHODIMP_(bool) CDX9AllocatorPresenter::ResizeDevice()
{
	DLog(L"CDX9AllocatorPresenter::ResizeDevice()");

	ASSERT(m_MainThreadId == GetCurrentThreadId());

	if (FAILED(ResetD3D9Device())) {
		DLog(L"CDX9AllocatorPresenter::ResizeDevice() - ResetD3D9Device() FAILED, send reset request");

		m_bPendingResetDevice = true;
		SendResetRequest();
	}

	m_bResizingDevice = false;
	Paint(false);

	return true;
}

STDMETHODIMP_(bool) CDX9AllocatorPresenter::ResetDevice()
{
	DLog(L"CDX9AllocatorPresenter::ResetDevice()");

	ASSERT(m_MainThreadId == GetCurrentThreadId());

	DeleteSurfaces();

	HRESULT hr;
	CString Error;
	if (FAILED(hr = CreateDevice(Error)) || FAILED(hr = AllocSurfaces())) {
#ifdef DEBUG_OR_LOG
		Error += GetWindowsErrorMessage(hr, nullptr);
		DLog(L"CDX9AllocatorPresenter::ResetDevice() - Error:\n%s", Error);
#endif
		m_bDeviceResetRequested = false;
		return false;
	}
	m_bDeviceResetRequested = false;
	m_bPendingResetDevice = false;

	return true;
}

STDMETHODIMP_(bool) CDX9AllocatorPresenter::DisplayChange()
{
	DLog(L"CDX9AllocatorPresenter::DisplayChange()");

	ASSERT(m_MainThreadId == GetCurrentThreadId());

	CComPtr<IDirect3D9Ex> pD3DEx;

	if (m_pfDirect3DCreate9Ex) {
		m_pfDirect3DCreate9Ex(D3D_SDK_VERSION, &pD3DEx);
		if (!pD3DEx) {
			m_pfDirect3DCreate9Ex(D3D9b_SDK_VERSION, &pD3DEx);
		}
	}

	if (pD3DEx) {
		const UINT adapterCount = pD3DEx->GetAdapterCount();
		if (adapterCount != m_AdapterCount) {
			m_AdapterCount = adapterCount;
			m_pD3D9Ex = pD3DEx;
		}
	}

	if (FAILED(ResetD3D9Device())) {
		DLog(L"CDX9AllocatorPresenter::DisplayChange() - ResetD3D9Device() FAILED, send reset request");

		m_bPendingResetDevice = true;
		SendResetRequest();
	}

	m_bDisplayChanged = true;

	return true;
}

STDMETHODIMP_(void) CDX9AllocatorPresenter::ResetStats()
{
	CAutoLock cRenderLock(&m_RenderLock);

	LONGLONG Time = GetPerfCounter();

	m_PaintTime = 0;
	m_PaintTimeMin = 0;
	m_PaintTimeMax = 0;

	m_RasterStatusWaitTime = 0;
	m_RasterStatusWaitTimeMin = 0;
	m_RasterStatusWaitTimeMax = 0;

	m_MinSyncOffset = 0;
	m_MaxSyncOffset = 0;
	m_fSyncOffsetAvr = 0;
	m_fSyncOffsetStdDev = 0;

	CalculateJitter(Time);
}

STDMETHODIMP_(void) CDX9AllocatorPresenter::SetPosition(RECT w, RECT v)
{
	const auto bWindowSizeChanged = m_windowRect.Size() != CRect(w).Size();

	__super::SetPosition(w, v);

	if (m_hWndVR) {
		SetWindowPos(m_hWndVR, nullptr, m_windowRect.left, m_windowRect.top, m_windowRect.Width(), m_windowRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	}

	if (bWindowSizeChanged && !m_bIsFullscreen && m_d3dpp.SwapEffect != D3DSWAPEFFECT_COPY) {
		m_bResizingDevice = true;
		AfxGetApp()->m_pMainWnd->PostMessage(WM_RESIZE_DEVICE);
	}

	if (bWindowSizeChanged) {
		m_bFont3DUpdate = true; // do not set "false" here
	}
}

STDMETHODIMP_(void) CDX9AllocatorPresenter::SetExtraSettings(ExtraRendererSettings* pExtraSets)
{
	if (pExtraSets) {
		CAutoLock cRenderLock(&m_RenderLock);

		if (m_pDevice9Ex) {
			if ((!m_bIsFullscreen && pExtraSets->bVSync != m_ExtraSets.bVSync)
				|| (!m_bIsFullscreen && pExtraSets->iPresentMode != m_ExtraSets.iPresentMode)
				|| pExtraSets->b10BitOutput != m_ExtraSets.b10BitOutput
				|| pExtraSets->iSurfaceFormat != m_ExtraSets.iSurfaceFormat) {
				m_bNeedResetDevice = true;
			}

			m_bNeedCreateWindow = (!m_bPreviewMode && !m_bIsFullscreen && pExtraSets->iPresentMode != m_ExtraSets.iPresentMode);
		}

		m_ExtraSets = *pExtraSets;
		m_nSurfaces = std::clamp(m_ExtraSets.nEVRBuffers, 2, MAX_VIDEO_SURFACES);
	}
}

void CDX9AllocatorPresenter::DrawStats()
{
	int iDetailedStats = 2;
	switch (m_ExtraSets.iDisplayStats) {
		case 1:
			iDetailedStats = 2;
			break;
		case 2:
			iDetailedStats = 1;
			break;
		case 3:
			iDetailedStats = 0;
			break;
	}

	const LONGLONG llMaxJitter     = m_MaxJitter;
	const LONGLONG llMinJitter     = m_MinJitter;
	const LONGLONG llMaxSyncOffset = m_MaxSyncOffset;
	const LONGLONG llMinSyncOffset = m_MinSyncOffset;

	if (m_bFont3DUpdate) {
		const UINT fontH = std::max(m_windowRect.Height() / 35, 6); // must be equal to 5 or more
		const UINT fontW = std::max(m_windowRect.Width() / 130, 4); // 0 = auto
		const UINT fontFlags = (m_rcMonitor.Width() - m_windowRect.Width() > 100) ? 0 : D3DFONT_BOLD;

		HRESULT hr2 = m_Font3D.CreateFontBitmap(L"Lucida Console", fontH, fontW, fontFlags);
		DLogIf(FAILED(hr2), L"m_Font3D font creation failed with error %s", HR2Str(hr2));

		m_bFont3DUpdate = false;
	}

	if (!m_pLine && m_pfD3DXCreateLine) {
		m_pfD3DXCreateLine(m_pDevice9Ex, &m_pLine);
	}

	{
		CString strText;
		strText.Preallocate(1200);

		if (iDetailedStats > 1) {
			double rtMS, rtFPS;
			rtMS = rtFPS = 0.0;
			if (m_rtTimePerFrame) {
				rtMS  = double(m_rtTimePerFrame) / 10000.0;
				rtFPS = 10000000.0 / (double)(m_rtTimePerFrame);
			}

			if (g_nFrameType != PICT_NONE) {
				strText.Format(L"Frame rate   : %7.03f   (%7.3f ms = %.03f, %s)   (%7.3f ms = %.03f%s, %2.03f StdDev)  Clock: %1.4f %%",
					m_fAvrFps,
					rtMS,
					rtFPS,
					g_nFrameType == PICT_FRAME ? L"P" : L"I",
					GetFrameTime() * 1000.0,
					GetFrameRate(),
					m_DetectedLock ? L" L" : L"",
					m_DetectedFrameTimeStdDev / 10000.0,
					m_ModeratedTimeSpeed*100.0);
			} else {
				strText.Format(L"Frame rate   : %7.03f   (%7.3f ms = %.03f)   (%7.3f ms = %.03f%s, %2.03f StdDev)  Clock: %1.4f %%",
					m_fAvrFps,
					rtMS,
					rtFPS,
					GetFrameTime() * 1000.0,
					GetFrameRate(),
					m_DetectedLock ? L" L" : L"",
					m_DetectedFrameTimeStdDev / 10000.0,
					m_ModeratedTimeSpeed*100.0);
			}
		} else {
			strText.Format(L"Frame rate   : %7.03f   (%.03f%s)",
				m_fAvrFps,
				GetFrameRate(),
				m_DetectedLock ? L" L" : L"");
		}

		if (g_nFrameType != PICT_NONE) {
			strText.AppendFormat(L"\nFrame type   : %s",
							g_nFrameType == PICT_FRAME ? L"Progressive" :
							g_nFrameType == PICT_BOTTOM_FIELD ? L"Interlaced : Bottom field first" :
							L"Interlaced : Top field first");
		}

		if (iDetailedStats > 1) {
			strText.Append(L"\nSettings     :");

			if (m_bIsFullscreen) {
				strText += L" FS";
			}

			switch (m_d3dpp.SwapEffect) {
				case D3DSWAPEFFECT_COPY   : strText += L" Copy";   break;
				case D3DSWAPEFFECT_FLIP   : strText += L" Flip";   break;
				case D3DSWAPEFFECT_FLIPEX : strText += L" FlipEx"; break;
			}

			if (!m_bCompositionEnabled) {
				strText += L" DisDC";
			}

			if (m_ExtraSets.bVSync) {
				strText += L" VSync";
			}
			if (m_ExtraSets.bVSyncInternal) {
				strText += m_ExtraSets.bVSync ? L"+Internal" : L" IntVSync";
			}

			if (m_ExtraSets.bFlushGPUBeforeVSync) {
				strText += L" GPUFlushBV";
			}
			if (m_ExtraSets.bFlushGPUAfterPresent) {
				strText += L" GPUFlushAP";
			}
			if (m_ExtraSets.bFlushGPUWait) {
				strText += L" GPUFlushWt";
			}

			if (m_ExtraSets.bEVRFrameTimeCorrection) {
				strText += L" FTC";
			}
			if (m_ExtraSets.iEVROutputRange == 0) {
				strText += L" 0-255";
			} else if (m_ExtraSets.iEVROutputRange == 1) {
				strText += L" 16-235";
			}

			strText.AppendFormat(L"\nInput format : %s", m_strInputFmt);

			strText.Append(L"\nSurface fmts | Mixer output  | Processing    | Backbuffer/Display |");
			ASSERT(m_BackbufferFmt == m_DisplayFmt);
			strText.AppendFormat(L"\n             | %-13s | %-13s | %-18s |"
				, m_strMixerOutputFmt
				, m_strProcessingFmt
				, m_strBackbufferFmt);

			strText.AppendFormat(L"\nRefresh rate : %.05f Hz (%.3f Hz)    SL: %4d    Last Duration: %8.4f    Corrected Frame Time: %s",
				m_DetectedRefreshRate,
				m_dRefreshRate,
				int(m_DetectedScanlinesPerFrame + 0.5),
				double(m_LastFrameDuration) / 10000.0,
				m_bCorrectedFrameTime ? L"Yes" : L"No");
		}

		if (m_bSyncStatsAvailable) {
			if (iDetailedStats > 1) {
				strText.AppendFormat(L"\nSync offset  : Min = %+8.3f ms, Max = %+8.3f ms, StdDev = %7.3f ms, Avr = %7.3f ms, Mode = %d", (double(llMinSyncOffset)/10000.0), (double(llMaxSyncOffset)/10000.0), m_fSyncOffsetStdDev/10000.0, m_fSyncOffsetAvr/10000.0, m_VSyncMode);
			} else {
				strText.AppendFormat(L"\nSync offset  : Mode = %d", m_VSyncMode);
			}
		}

		if (iDetailedStats > 1) {
			strText.AppendFormat(L"\nJitter       : Min = %+8.3f ms, Max = %+8.3f ms, StdDev = %7.3f ms", (double(llMinJitter)/10000.0), (double(llMaxJitter)/10000.0), m_fJitterStdDev/10000.0);
		}

		if (m_pSubPicAllocator && m_pSubPicProvider && iDetailedStats > 1) {
			CDX9SubPicAllocator *pAlloc = (CDX9SubPicAllocator *)m_pSubPicAllocator.p;
			int nFree = 0;
			int nAlloc = 0;
			int nSubPic = 0;
			REFERENCE_TIME QueueStart = 0;
			REFERENCE_TIME QueueEnd = 0;

			if (m_pSubPicQueue) {
				REFERENCE_TIME QueueNow = 0;
				m_pSubPicQueue->GetStats(nSubPic, QueueNow, QueueStart, QueueEnd);

				if (QueueStart) {
					QueueStart -= QueueNow;
				}

				if (QueueEnd) {
					QueueEnd -= QueueNow;
				}
			}

			pAlloc->GetStats(nFree, nAlloc);
			strText.AppendFormat(L"\nSubtitles    : Free %d     Allocated %d     Buffered %d     QueueStart %7.3f     QueueEnd %7.3f", nFree, nAlloc, nSubPic, (double(QueueStart)/10000000.0), (double(QueueEnd)/10000000.0));
		}

		if (m_ExtraSets.bVSyncInternal) {
			if (iDetailedStats > 1) {
				if (m_VBlankEndPresent == -100000) {
					strText.AppendFormat(L"\nVBlank Wait  : Start %4d   End %4d   Wait %7.3f ms   Lock %7.3f ms   Offset %4d   Max %4d", m_VBlankStartWait, m_VBlankEndWait, (double(m_VBlankWaitTime)/10000.0), (double(m_VBlankLockTime)/10000.0), m_VBlankMin, m_VBlankMax - m_VBlankMin);
				} else {
					strText.AppendFormat(L"\nVBlank Wait  : Start %4d   End %4d   Wait %7.3f ms   Lock %7.3f ms   Offset %4d   Max %4d   EndPresent %4d", m_VBlankStartWait, m_VBlankEndWait, (double(m_VBlankWaitTime)/10000.0), (double(m_VBlankLockTime)/10000.0), m_VBlankMin, m_VBlankMax - m_VBlankMin, m_VBlankEndPresent);
				}
			} else {
				if (m_VBlankEndPresent == -100000) {
					strText.AppendFormat(L"\nVBlank Wait  : Start %4d   End %4d", m_VBlankStartWait, m_VBlankEndWait);
				} else {
					strText.AppendFormat(L"\nVBlank Wait  : Start %4d   End %4d   EP %4d", m_VBlankStartWait, m_VBlankEndWait, m_VBlankEndPresent);
				}
			}
		}

		bool bDoVSyncInPresent = !m_bCompositionEnabled || !m_ExtraSets.bVSyncInternal;
		if (iDetailedStats > 1 && bDoVSyncInPresent) {
			strText.AppendFormat(L"\nPresent Wait : Wait %7.3f ms   Min %7.3f ms   Max %7.3f ms", (double(m_PresentWaitTime)/10000.0), (double(m_PresentWaitTimeMin)/10000.0), (double(m_PresentWaitTimeMax)/10000.0));
		}

		if (iDetailedStats > 1) {
			if (m_WaitForGPUTime) {
				strText.AppendFormat(L"\nPaint Time   : Draw %7.3f ms   Min %7.3f ms   Max %7.3f ms   GPU %7.3f ms", (double(m_PaintTime-m_WaitForGPUTime)/10000.0), (double(m_PaintTimeMin)/10000.0), (double(m_PaintTimeMax)/10000.0), (double(m_WaitForGPUTime)/10000.0));
			} else {
				strText.AppendFormat(L"\nPaint Time   : Draw %7.3f ms   Min %7.3f ms   Max %7.3f ms", (double(m_PaintTime-m_WaitForGPUTime)/10000.0), (double(m_PaintTimeMin)/10000.0), (double(m_PaintTimeMax)/10000.0));
			}
		} else {
			if (m_WaitForGPUTime) {
				strText.AppendFormat(L"\nPaint Time   : Draw %7.3f ms   GPU %7.3f ms", (double(m_PaintTime - m_WaitForGPUTime)/10000.0), (double(m_WaitForGPUTime)/10000.0));
			} else {
				strText.AppendFormat(L"\nPaint Time   : Draw %7.3f ms", (double(m_PaintTime - m_WaitForGPUTime)/10000.0));
			}
		}

		if (iDetailedStats > 1 && m_ExtraSets.bVSyncInternal) {
			strText.AppendFormat(L"\nRaster Status: Wait %7.3f ms   Min %7.3f ms   Max %7.3f ms", (double(m_RasterStatusWaitTime)/10000.0), (double(m_RasterStatusWaitTimeMin)/10000.0), (double(m_RasterStatusWaitTimeMax)/10000.0));
		}

		if (iDetailedStats > 1) {
			strText.AppendFormat(L"\nBuffering    : Buffered %3d    Free %3d    Current Surface %3u", m_nUsedBuffer, m_nSurfaces - m_nUsedBuffer, m_iCurSurface);
		} else {
			strText.AppendFormat(L"\nBuffered     : %3d", m_nUsedBuffer);
		}

		if (iDetailedStats > 1) {
			if (m_pVideoTextures[0] || m_pVideoSurfaces[0]) {
				D3DSURFACE_DESC desc = {};
				if (m_pVideoTextures[0]) {
					m_pVideoTextures[0]->GetLevelDesc(0, &desc);
				} else if (m_pVideoSurfaces[0]) {
					m_pVideoSurfaces[0]->GetDesc(&desc);
				}

				if (desc.Width != (UINT)m_nativeVideoSize.cx || desc.Height != (UINT)m_nativeVideoSize.cy) {
					strText.AppendFormat(L"\nTexture size : %d x %d", desc.Width, desc.Height);
				}
			}

			if (m_Decoder.GetLength()) {
				strText.AppendFormat(L"\nDecoder      : " + m_Decoder + ", " + DXVAState::GetDescription());
			}

			if (m_D3D9Device.GetLength()) {
				strText.AppendFormat(L"\nRender device: " + m_D3D9Device);
			}

			if (m_MonitorName.GetLength()) {
				strText.AppendFormat(L"\nMonitor      : %s, Native resolution %ux%u", m_MonitorName, m_nMonitorHorRes, m_nMonitorVerRes);
			}

			strText.AppendFormat(L"\nVideo size   : %d x %d (%d:%d)", m_nativeVideoSize.cx, m_nativeVideoSize.cy, m_aspectRatio.cx, m_aspectRatio.cy);
			CSize videoSize = m_videoRect.Size();
			if (m_nativeVideoSize != videoSize) {
				strText.AppendFormat(L" -> %d x %d", videoSize.cx, videoSize.cy);
				if (m_wsResizer) {
					strText.AppendFormat(L" %s", m_wsResizer);
					if (m_wsResizer2) {
						strText.AppendFormat(L":X + %s:Y", m_wsResizer2);
					}
				}
			}

			if (m_wsCorrection || m_strFinalPass.GetLength()) {
				strText.Append(L"\nProcessing   : ");
				if (m_wsCorrection) {
					strText.Append(m_wsCorrection);
				}
				if (m_strFinalPass.GetLength()) {
					if (m_wsCorrection) {
						strText.Append(L", ");
					}
					strText.Append(m_strFinalPass);
				}
			}

			{
				strText.AppendFormat(L"\nPerformance  : CPU:%3d%%", m_CPUUsage.GetUsage());

				if (m_GPUUsage.GetType() != CGPUUsage::UNKNOWN_GPU) {
					CGPUUsage::statistic gpu_statistic;
					m_GPUUsage.GetUsage(gpu_statistic);

					if (m_GPUUsage.IsUseStatistics() || m_GPUUsage.GetType() != CGPUUsage::INTEL_GPU) {
						strText.AppendFormat(L", GPU:%3u%%", gpu_statistic.gpu);
					}

					if (gpu_statistic.clock) {
						strText.AppendFormat(L" (%4u MHz)", gpu_statistic.clock);
					}

					if (gpu_statistic.bUseDecode) {
						if (gpu_statistic.bUseProcessing) {
							strText.AppendFormat(L", Video Decode/Processing:%3u%%/%3u%%", gpu_statistic.decode, gpu_statistic.processing);
						} else {
							strText.AppendFormat(L", Video Decode:%3u%%", gpu_statistic.decode);
						}
					}

					if (gpu_statistic.memUsageTotal) {
						strText.AppendFormat(L", GPU Memory:%4llu/%llu MB", gpu_statistic.memUsageTotal, gpu_statistic.memUsageCurrent);
					}
				}

				if (const size_t mem_usage = m_MemUsage.GetUsage()) {
					strText.AppendFormat(L", Memory:%4zu MB", mem_usage);
				}
			}
		}

		if (iDetailedStats && !m_pfD3DXCreateLine) {
			strText.Append(L"\nERROR: The file d3dx9_43.dll is not loaded, so the graph will not be drawn.");
		}

		m_Font3D.Draw2DText(42, 42, D3DCOLOR_XRGB(0, 0, 0), strText);
		m_Font3D.Draw2DText(40, 40, D3DCOLOR_XRGB(255, 204, 0), strText);
	}

	if (iDetailedStats && m_pLine) {
		D3DXVECTOR2 Points[NB_JITTER];

		const int defwidth  = 810;
		const int defheight = 300;

		const float ScaleX = m_windowRect.Width() / 1920.0f;
		const float ScaleY = m_windowRect.Height() / 1080.0f;

		const float DrawWidth = defwidth * ScaleX;
		const float DrawHeight = defheight * ScaleY;
		const float StartX = m_windowRect.Width() - (DrawWidth + 20 * ScaleX);
		const float StartY = m_windowRect.Height() - (DrawHeight + 20 * ScaleX);
		const float StepX = DrawWidth / NB_JITTER;
		const float StepY = DrawHeight / 24.0f;
		const D3DCOLOR rectColor = D3DCOLOR_ARGB(80, 0, 0, 0);

		DrawRect(rectColor, CRect(StartX, StartY, StartX + DrawWidth, StartY + DrawHeight));

		m_pLine->SetWidth(2.5f * ScaleX);
		m_pLine->SetAntialias(TRUE);
		m_pLine->Begin();

		// draw grid lines
		for (unsigned i = 1; i < 24; ++i) {
			Points[0].x = StartX;
			Points[0].y = StartY + i * StepY;
			Points[1].y = Points[0].y;

			float lineLength;
			if ((i - 1) % 4) {
				lineLength = 0.07f;
			} else if (i == 13) {
				lineLength = 1.0f;
			} else {
				lineLength = 0.93f;
			}
			Points[1].x = StartX + DrawWidth * lineLength;
			m_pLine->Draw(Points, 2, D3DCOLOR_XRGB(100, 100, 255));
		}

		// draw jitter curve
		if (m_rtTimePerFrame) {
			for (unsigned i = 0; i < NB_JITTER; i++) {
				unsigned index = (m_nNextJitter + 1 + i) % NB_JITTER;
				float jitter = float(m_Jitters[index] - m_iJitterMean);

				Points[i].x = StartX + i * StepX;
				Points[i].y = StartY + (jitter * ScaleY / 5000.0f + DrawHeight / 2.0f);
			}
			m_pLine->Draw(Points, NB_JITTER, D3DCOLOR_XRGB(255, 100, 100));

			if (m_bSyncStatsAvailable) {
				// draw sync offset
				for (unsigned i = 0; i < NB_JITTER; i++) {
					unsigned index = (m_nNextSyncOffset + 1 + i) % NB_JITTER;
					Points[i].x = StartX + i * StepX;
					Points[i].y = StartY + (m_llSyncOffsets[index] * ScaleY / 5000.0f + DrawHeight / 2.0f);
				}
				m_pLine->Draw(Points, NB_JITTER, D3DCOLOR_XRGB(100, 200, 100));
			}
		}
		m_pLine->End();
	}
}

STDMETHODIMP CDX9AllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
	CheckPointer(size, E_POINTER);

	CSize framesize = GetVideoSize();
	const CSize dar = GetVideoSizeAR();
	const int rotation = GetRotation();

	if (dar.cx > 0 && dar.cy > 0) {
		framesize.cx = MulDiv(framesize.cy, dar.cx, dar.cy);
	}
	if (rotation == 90 || rotation == 270) {
		std::swap(framesize.cx, framesize.cy);
	}

	const DWORD required = sizeof(BITMAPINFOHEADER) + (framesize.cx * framesize.cy * 4);
	if (!lpDib) {
		*size = required;
		return S_OK;
	}
	if (*size < required) {
		return E_OUTOFMEMORY;
	}
	*size = required;

	CAutoLock lock(&m_RenderLock);

	HRESULT hr;
	CComPtr<IDirect3DSurface9> pRGB32Surface;
	if (FAILED(hr = m_pDevice9Ex->CreateRenderTarget(framesize.cx, framesize.cy, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pRGB32Surface, nullptr))) {
		return hr;
	}

	CComPtr<IDirect3DTexture9> pVideoTexture = m_pVideoTextures[m_iCurSurface];
	if (m_pPSCorrection) {
		D3DSURFACE_DESC videoDesc;
		m_pVideoTextures[m_iCurSurface]->GetLevelDesc(0, &videoDesc);

		const float fConstData[4] = { 1.0f / videoDesc.Width, 1.0f / videoDesc.Height, 0, 0 };
		hr = m_pDevice9Ex->SetPixelShaderConstantF(0, fConstData, 1);

		CComPtr<IDirect3DSurface9> pTemporarySurface;
		hr = m_pFrameTextures[0]->GetSurfaceLevel(0, &pTemporarySurface);
		hr = m_pDevice9Ex->SetRenderTarget(0, pTemporarySurface);
		hr = m_pDevice9Ex->SetPixelShader(m_pPSCorrection);
		TextureCopy(m_pVideoTextures[m_iCurSurface]);
		pVideoTexture = m_pFrameTextures[0];
	}

	D3DLOCKED_RECT r = {};
	if (FAILED(hr = m_pDevice9Ex->SetRenderTarget(0, pRGB32Surface))
			|| FAILED(hr = Resize(pVideoTexture, { CPoint(0, 0), m_nativeVideoSize }, { CPoint(0, 0), framesize }))
			|| FAILED(hr = pRGB32Surface->LockRect(&r, nullptr, D3DLOCK_READONLY))) {
		return hr;
	}

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)lpDib;
	memset(bih, 0, sizeof(BITMAPINFOHEADER));
	bih->biSize           = sizeof(BITMAPINFOHEADER);
	bih->biWidth          = framesize.cx;
	bih->biHeight         = framesize.cy;
	bih->biBitCount       = 32;
	bih->biPlanes         = 1;
	bih->biSizeImage      = DIBSIZE(*bih);

	uint32_t* p = nullptr;
	if (rotation) {
		p = DNew uint32_t[bih->biWidth * bih->biHeight];
	}

	RetrieveBitmapData(framesize.cx, framesize.cy, 32, p ? (BYTE*)p : (BYTE*)(bih + 1), (BYTE*)r.pBits, r.Pitch);

	pRGB32Surface->UnlockRect();

	if (p) {
		int w = bih->biWidth;
		int h = bih->biHeight;
		uint32_t* out = (uint32_t*)(bih + 1);

		switch (rotation) {
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

		delete [] p;
	}

	return S_OK;
}

STDMETHODIMP CDX9AllocatorPresenter::GetDisplayedImage(LPVOID* dibImage)
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

STDMETHODIMP CDX9AllocatorPresenter::ClearPixelShaders(int target)
{
	CAutoLock cRenderLock(&m_RenderLock);

	return ClearCustomPixelShaders(target);
}

STDMETHODIMP CDX9AllocatorPresenter::AddPixelShader(int target, LPCWSTR name, LPCSTR profile, LPCSTR sourceCode)
{
	CAutoLock cRenderLock(&m_RenderLock);

	return AddCustomPixelShader(target, sourceCode, profile);
}

void CDX9AllocatorPresenter::OnChangeInput(CComPtr<IPin> pPin)
{
	CMediaType input_mt;

	if (pPin->ConnectionMediaType(&input_mt) == S_OK) {
		// set framerate
		ExtractAvgTimePerFrame(&input_mt, m_rtTimePerFrame);
		if (m_rtTimePerFrame == 1) { // if framerate not set by Video Decoder - choose 23.976
			m_rtTimePerFrame = 417083;
		}

		// set mixer format string
		CString subtypestr = GetGUIDString(input_mt.subtype);
		if (StartsWith(subtypestr, L"MEDIASUBTYPE_")) {
			m_strInputFmt = subtypestr.Mid(13);
		} else {
			BITMAPINFOHEADER bih;
			if (ExtractBIH(&input_mt, &bih)) {
				m_strInputFmt.Format(L"%C%C%C%C",
					((char*)&bih.biCompression)[0],
					((char*)&bih.biCompression)[1],
					((char*)&bih.biCompression)[2],
					((char*)&bih.biCompression)[3]);
			}
		}

		if (input_mt.formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)input_mt.pbFormat;
			if (vih2->dwControlFlags & (AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT)) {
				DXVA2_ExtendedFormat exfmt;
				exfmt.value = vih2->dwControlFlags;
				if (s_nominalrange[exfmt.NominalRange]) {
					m_strInputFmt.AppendFormat(L", %S", s_nominalrange[exfmt.NominalRange]);
				}
				if (s_transfermatrix[exfmt.VideoTransferMatrix]) {
					m_strInputFmt.AppendFormat(L", %S", s_transfermatrix[exfmt.VideoTransferMatrix]);
				}
				if (s_transferfunction[exfmt.VideoTransferFunction]) {
					m_strInputFmt.AppendFormat(L", %S", s_transferfunction[exfmt.VideoTransferFunction]);
				}
			}
		}

		// set input DXVA2_ExtendedFormat and init shaders for image correction
		InitCorrectionPass(input_mt);
	}

	// set decoder string
	m_Decoder.Empty();
	CComPtr<IPin> pPinTo;
	if (pPin && SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo) {
		IBaseFilter* pBF = nullptr;

		do {
			pBF = GetFilterFromPin(pPinTo);
			CLSID clsid = GetCLSID(pBF);
			CComQIPtr<IDirectVobSub> pDVS = pBF;

			if (!pDVS // VSFilter, xy-VSFilter
				&& clsid != CLSID_ffdshowRawVideoFilter // ffdshow raw video filter
				&& clsid != GUIDFromCString(L"{FCC4769C-1E45-4E60-8CBF-B159B2584587}") // Bluesky Frame Rate Converter
				&& clsid != GUIDFromCString(L"{848E3162-C71A-4191-A0BE-0FFC449A7A35}")) { // DmitriRender
				// skip some filters (not decoders)
				break;
			}

			pPinTo = GetUpStreamPin(pBF, GetFirstPin(pBF));
		} while (pPinTo);

		if (pBF) {
			m_Decoder = GetFilterName(pBF);
		}
	}
}

// ID3DFullscreenControl
STDMETHODIMP CDX9AllocatorPresenter::SetD3DFullscreen(bool fEnabled)
{
	m_bIsFullscreen = fEnabled;
	return S_OK;
}

STDMETHODIMP CDX9AllocatorPresenter::GetD3DFullscreen(bool* pfEnabled)
{
	CheckPointer(pfEnabled, E_POINTER);
	*pfEnabled = m_bIsFullscreen;
	return S_OK;
}
