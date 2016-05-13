/*
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
#include "EVRAllocatorPresenter.h"
#include "OuterEVR.h"
#include <Mferror.h>
#include "IPinHook.h"
#include "MacrovisionKicker.h"

#if (0)		// Set to 1 to activate EVR traces
	#define TRACE_EVR	TRACE
#else
	#define TRACE_EVR	__noop
#endif

#define MIN_FRAME_TIME 15000

enum EVR_STATS_MSG {
	MSG_MIXERIN,
	MSG_MIXEROUT
};

// Guid to tag IMFSample with DirectX surface index
static const GUID GUID_SURFACE_INDEX = { 0x30c8e9f6, 0x415, 0x4b81, { 0xa3, 0x15, 0x1, 0xa, 0xc6, 0xa9, 0xda, 0x19 } };

MFOffset MakeOffset(float v)
{
	MFOffset offset;
	offset.value = short(v);
	offset.fract = WORD(65536 * (v-offset.value));
	return offset;
}

MFVideoArea MakeArea(float x, float y, DWORD width, DWORD height)
{
	MFVideoArea area;
	area.OffsetX = MakeOffset(x);
	area.OffsetY = MakeOffset(y);
	area.Area.cx = width;
	area.Area.cy = height;
	return area;
}

using namespace DSObjects;

CEVRAllocatorPresenter::CEVRAllocatorPresenter(HWND hWnd, bool bFullscreen, HRESULT& hr, CString &_Error)
	: CDX9AllocatorPresenter(hWnd, bFullscreen, hr, true, _Error)
	, m_nResetToken(0)
	, m_hRenderThread(NULL)
	, m_hGetMixerThread(NULL)
	, m_hVSyncThread(NULL)
	, m_hEvtFlush(NULL)
	, m_hEvtQuit(NULL)
	, m_hEvtRenegotiate(NULL)
	, m_bEvtQuit(false)
	, m_bEvtFlush(false)
	, m_ModeratedTime(0)
	, m_ModeratedTimeLast(-1)
	, m_ModeratedClockLast(-1)
	, m_ModeratedTimer(0)
	, m_LastClockState(MFCLOCK_STATE_INVALID)
	, m_nRenderState(Shutdown)
	, m_fUseInternalTimer(false)
	, m_LastSetOutputRange(-1)
	, m_bPendingMediaFinished(false)
	, m_bWaitingSample(false)
	, m_pCurrentDisplaydSample(NULL)
	, m_nStepCount(0)
	, m_dwVideoAspectRatioMode(MFVideoARMode_PreservePicture)
	, m_dwVideoRenderPrefs((MFVideoRenderPrefs)0)
	, m_BorderColor(RGB (0,0,0))
	, m_bSignaledStarvation(false)
	, m_StarvationClock(0)
	, m_pOuterEVR(NULL)
	, m_LastScheduledSampleTime(-1)
	, m_LastScheduledUncorrectedSampleTime(-1)
	, m_MaxSampleDuration(0)
	, m_LastSampleOffset(0)
	, m_LastPredictedSync(0)
	, m_VSyncOffsetHistoryPos(0)
	, m_bLastSampleOffsetValid(false)
	, m_bChangeMT(false)
	, m_LastScheduledSampleTimeFP(-1)
	, pfDXVA2CreateDirect3DDeviceManager9(NULL)
	, pfMFCreateDXSurfaceBuffer(NULL)
	, pfMFCreateVideoSampleFromSurface(NULL)
	, pfMFCreateVideoMediaType(NULL)
	, pfAvSetMmThreadCharacteristicsW(NULL)
	, pfAvSetMmThreadPriority(NULL)
	, pfAvRevertMmThreadCharacteristics(NULL)
	, m_pMediaType(NULL)
	, m_bStreamChanged(TRUE)
{
	HMODULE hLib = NULL;
	CRenderersSettings& rs = GetRenderersSettings();

	ZeroMemory(m_VSyncOffsetHistory, sizeof(m_VSyncOffsetHistory));
	ResetStats();

	if (FAILED(hr)) {
		_Error += L"DX9AllocatorPresenter failed\n";
		return;
	}

	// Load EVR specifics DLLs
	hLib = LoadLibrary(L"dxva2.dll");
	pfDXVA2CreateDirect3DDeviceManager9	= hLib ? (PTR_DXVA2CreateDirect3DDeviceManager9) GetProcAddress(hLib, "DXVA2CreateDirect3DDeviceManager9") : NULL;

	// Load EVR functions
	hLib = LoadLibrary(L"evr.dll");
	pfMFCreateDXSurfaceBuffer			= hLib ? (PTR_MFCreateDXSurfaceBuffer)			GetProcAddress(hLib, "MFCreateDXSurfaceBuffer") : NULL;
	pfMFCreateVideoSampleFromSurface	= hLib ? (PTR_MFCreateVideoSampleFromSurface)	GetProcAddress(hLib, "MFCreateVideoSampleFromSurface") : NULL;
	pfMFCreateVideoMediaType			= hLib ? (PTR_MFCreateVideoMediaType)			GetProcAddress(hLib, "MFCreateVideoMediaType") : NULL;

	if (!pfDXVA2CreateDirect3DDeviceManager9 || !pfMFCreateDXSurfaceBuffer || !pfMFCreateVideoSampleFromSurface || !pfMFCreateVideoMediaType) {
		if (!pfDXVA2CreateDirect3DDeviceManager9) {
			_Error += L"Could not find DXVA2CreateDirect3DDeviceManager9 (dxva2.dll)\n";
		}
		if (!pfMFCreateDXSurfaceBuffer) {
			_Error += L"Could not find MFCreateDXSurfaceBuffer (evr.dll)\n";
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

	// Load mfplat fuctions
#if 0
	hLib = LoadLibrary (L"mfplat.dll");
	(FARPROC &)pMFCreateMediaType = GetProcAddress(hLib, "MFCreateMediaType");
	(FARPROC &)pMFInitMediaTypeFromAMMediaType = GetProcAddress(hLib, "MFInitMediaTypeFromAMMediaType");
	(FARPROC &)pMFInitAMMediaTypeFromMFMediaType = GetProcAddress(hLib, "MFInitAMMediaTypeFromMFMediaType");

	if (!pMFCreateMediaType || !pMFInitMediaTypeFromAMMediaType || !pMFInitAMMediaTypeFromMFMediaType) {
		hr = E_FAIL;
		return;
	}
#endif

	// Load Vista specifics DLLs
	hLib = LoadLibrary(L"avrt.dll");
	pfAvSetMmThreadCharacteristicsW		= hLib ? (PTR_AvSetMmThreadCharacteristicsW)	GetProcAddress(hLib, "AvSetMmThreadCharacteristicsW") : NULL;
	pfAvSetMmThreadPriority				= hLib ? (PTR_AvSetMmThreadPriority)			GetProcAddress(hLib, "AvSetMmThreadPriority") : NULL;
	pfAvRevertMmThreadCharacteristics	= hLib ? (PTR_AvRevertMmThreadCharacteristics)	GetProcAddress(hLib, "AvRevertMmThreadCharacteristics") : NULL;

	// Init DXVA manager
	hr = pfDXVA2CreateDirect3DDeviceManager9(&m_nResetToken, &m_pD3DManager);
	if (SUCCEEDED(hr)) {
		hr = m_pD3DManager->ResetDevice(m_pD3DDev, m_nResetToken);
		if (!SUCCEEDED(hr)) {
			_Error += L"m_pD3DManager->ResetDevice() failed\n";
		}
	} else {
		_Error += L"DXVA2CreateDirect3DDeviceManager9() failed\n";
	}

	// Bufferize frame only with 3D texture!
	if (rs.iSurfaceType == SURFACE_TEXTURE3D) {
		m_nNbDXSurface = clamp(rs.nEVRBuffers, 4, MAX_VIDEO_SURFACES);
	} else {
		m_nNbDXSurface = 1;
	}
}

CEVRAllocatorPresenter::~CEVRAllocatorPresenter(void)
{
	StopWorkerThreads();	// If not already done...
	m_pMediaType	= NULL;
	m_pClock		= NULL;

	m_pD3DManager	= NULL;
}

void CEVRAllocatorPresenter::ResetStats()
{
	m_pcFrames			= 0;
	m_nDroppedUpdate	= 0;
	m_pcFramesDrawn		= 0;
	m_piAvg				= 0;
	m_piDev				= 0;
}

HRESULT CEVRAllocatorPresenter::CheckShutdown() const
{
	if (m_nRenderState == Shutdown) {
		return MF_E_SHUTDOWN;
	} else {
		return S_OK;
	}
}

void CEVRAllocatorPresenter::StartWorkerThreads()
{
	DWORD dwThreadId;

	if (m_nRenderState == Shutdown) {
		m_hEvtQuit			= CreateEvent (NULL, TRUE, FALSE, NULL);
		m_hEvtFlush			= CreateEvent (NULL, TRUE, FALSE, NULL);

		m_hRenderThread		= ::CreateThread(NULL, 0, PresentThread, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hRenderThread, THREAD_PRIORITY_TIME_CRITICAL);
		m_hGetMixerThread	= ::CreateThread(NULL, 0, GetMixerThreadStatic, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hGetMixerThread, THREAD_PRIORITY_HIGHEST);
		m_hVSyncThread		= ::CreateThread(NULL, 0, VSyncThreadStatic, (LPVOID)this, 0, &dwThreadId);
		SetThreadPriority(m_hVSyncThread, THREAD_PRIORITY_HIGHEST);

		m_nRenderState		= Stopped;
		m_bChangeMT			= true;
		TRACE_EVR("EVR: Worker threads started...\n");
	}
}

void CEVRAllocatorPresenter::StopWorkerThreads()
{
	if (m_nRenderState != Shutdown) {
		if (m_pClock) {// if m_pClock is active, everything has been initialized from the above InitServicePointers()
			SetEvent(m_hEvtFlush);
			m_bEvtFlush = true;
			SetEvent(m_hEvtQuit);
			m_bEvtQuit = true;
			if (m_hRenderThread && WaitForSingleObject(m_hRenderThread, 1000) == WAIT_TIMEOUT) {
				ASSERT(FALSE);
				TerminateThread(m_hRenderThread, 0xDEAD);
			}
			if (m_hGetMixerThread && WaitForSingleObject(m_hGetMixerThread, 1000) == WAIT_TIMEOUT) {
				ASSERT(FALSE);
				TerminateThread(m_hGetMixerThread, 0xDEAD);
			}
			if (m_hVSyncThread && WaitForSingleObject(m_hVSyncThread, 1000) == WAIT_TIMEOUT) {
				ASSERT(FALSE);
				TerminateThread(m_hVSyncThread, 0xDEAD);
			}
		}

		SAFE_CLOSE_HANDLE(m_hRenderThread);
		SAFE_CLOSE_HANDLE(m_hGetMixerThread);
		SAFE_CLOSE_HANDLE(m_hVSyncThread);
		SAFE_CLOSE_HANDLE(m_hEvtFlush);
		SAFE_CLOSE_HANDLE(m_hEvtQuit);

		m_bEvtFlush	= false;
		m_bEvtQuit	= false;

		TRACE_EVR("EVR: Worker threads stopped...\n");
	}

	m_nRenderState = Shutdown;
}

STDMETHODIMP CEVRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
	CheckPointer(ppRenderer, E_POINTER);

	*ppRenderer = NULL;

	HRESULT hr = E_FAIL;

	do {
		CMacrovisionKicker*	pMK  = DNew CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
		CComPtr<IUnknown>	pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;

		COuterEVR *pOuterEVR = DNew COuterEVR(NAME("COuterEVR"), pUnk, hr, &m_VMR9AlphaBitmap, this);
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
		CComQIPtr<IEVRFilterConfig>	pConfig	= pBF;

		if (FAILED(pConfig->SetNumberOfStreams(3))) { // TODO - maybe need other number of input stream ...
			DbgLog((LOG_TRACE, 3, L"IEVRFilterConfig->SetNumberOfStreams(3) fail"));
			break;
		}

		hr = pMFGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pMFVR));
		if (SUCCEEDED(hr)) {
			hr = QueryInterface(IID_PPV_ARGS(&pVP));
		}
		if (SUCCEEDED(hr)) {
			hr = pMFVR->InitializeRenderer(NULL, pVP);
		}

		m_fUseInternalTimer = HookNewSegmentAndReceive(GetFirstPin(pBF));

		if (FAILED(hr)) {
			*ppRenderer = NULL;
		} else {
			*ppRenderer = pBF.Detach();
		}

	} while (0);

	return hr;
}

STDMETHODIMP_(bool) CEVRAllocatorPresenter::Paint(bool fAll)
{
	return __super::Paint(fAll);
}

STDMETHODIMP CEVRAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
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
	} else if (riid == __uuidof(IEVRTrustedVideoPlugin)) {
		hr = GetInterface((IEVRTrustedVideoPlugin*)this, ppv);
	} else if (riid == IID_IQualProp) {
		hr = GetInterface((IQualProp*)this, ppv);
	} else if (riid == __uuidof(IMFRateSupport)) {
		hr = GetInterface((IMFRateSupport*)this, ppv);
	} else if (riid == __uuidof(IDirect3DDeviceManager9)) {
		//hr = GetInterface((IDirect3DDeviceManager9*)this, ppv);
		hr = m_pD3DManager->QueryInterface(__uuidof(IDirect3DDeviceManager9), (void**) ppv);
	} else if (riid == __uuidof(ID3DFullscreenControl)) {
		hr = GetInterface((ID3DFullscreenControl*)this, ppv);
	} else {
		hr = __super::NonDelegatingQueryInterface(riid, ppv);
	}

	return hr;
}

// IMFClockStateSink
STDMETHODIMP CEVRAllocatorPresenter::OnClockStart(MFTIME hnsSystemTime,  LONGLONG llClockStartOffset)
{
	TRACE_EVR("EVR: OnClockStart  hnsSystemTime = %I64d,   llClockStartOffset = %I64d\n", hnsSystemTime, llClockStartOffset);

	m_nRenderState			= Started;
	m_ModeratedTimeLast		= -1;
	m_ModeratedClockLast	= -1;

	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockStop(MFTIME hnsSystemTime)
{
	TRACE_EVR("EVR: OnClockStop  hnsSystemTime = %I64d\n", hnsSystemTime);

	m_nRenderState			= Stopped;
	m_ModeratedClockLast	= -1;
	m_ModeratedTimeLast		= -1;

	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockPause(MFTIME hnsSystemTime)
{
	TRACE_EVR("EVR: OnClockPause  hnsSystemTime = %I64d\n", hnsSystemTime);

	if (!m_bSignaledStarvation) {
		m_nRenderState		= Paused;
	}
	m_ModeratedTimeLast		= -1;
	m_ModeratedClockLast	= -1;

	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockRestart(MFTIME hnsSystemTime)
{
	TRACE_EVR("EVR: OnClockRestart  hnsSystemTime = %I64d\n", hnsSystemTime);

	m_nRenderState			= Started;
	m_ModeratedTimeLast		= -1;
	m_ModeratedClockLast	= -1;

	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
	ASSERT(FALSE);
	return E_NOTIMPL;
}

// IBaseFilter delegate
bool CEVRAllocatorPresenter::GetState( DWORD dwMilliSecsTimeout, FILTER_STATE *State, HRESULT &_ReturnValue)
{
	CAutoLock lock(&m_SampleQueueLock);

	if (m_bSignaledStarvation) {
		size_t nSamples = max(m_nNbDXSurface / 2, 1);
		if ((m_ScheduledSamples.GetCount() < nSamples || m_LastSampleOffset < -m_rtTimePerFrame*2) && !g_bNoDuration) {
			*State = (FILTER_STATE)Paused;
			_ReturnValue = VFW_S_STATE_INTERMEDIATE;
			return true;
		}
		m_bSignaledStarvation = false;
	}
	return false;
}

// IQualProp
STDMETHODIMP CEVRAllocatorPresenter::get_FramesDroppedInRenderer(int *pcFrames)
{
	*pcFrames = m_pcFrames;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_FramesDrawn(int *pcFramesDrawn)
{
	*pcFramesDrawn = m_pcFramesDrawn;
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_AvgFrameRate(int *piAvgFrameRate)
{
	*piAvgFrameRate = (int)(m_fAvrFps * 100);
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_Jitter(int *iJitter)
{
	*iJitter = (int)((m_fJitterStdDev/10000.0) + 0.5);
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_AvgSyncOffset(int *piAvg)
{
	*piAvg = (int)((m_fSyncOffsetAvr/10000.0) + 0.5);
	return S_OK;
}
STDMETHODIMP CEVRAllocatorPresenter::get_DevSyncOffset(int *piDev)
{
	*piDev = (int)((m_fSyncOffsetStdDev/10000.0) + 0.5);
	return S_OK;
}

// IMFRateSupport

STDMETHODIMP CEVRAllocatorPresenter::GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate)
{
	// TODO : not finished...
	*pflRate = 0;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::GetFastestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate)
{
	HRESULT hr = S_OK;
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

STDMETHODIMP CEVRAllocatorPresenter::IsRateSupported(BOOL fThin, float flRate, float *pflNearestSupportedRate)
{
	// fRate can be negative for reverse playback.
	// pfNearestSupportedRate can be NULL.

	CAutoLock lock(this);

	HRESULT hr = S_OK;
	float   fMaxRate = 0.0f;
	float   fNearestRate = flRate;   // Default.

	CheckPointer(pflNearestSupportedRate, E_POINTER);
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

float CEVRAllocatorPresenter::GetMaxRate(BOOL bThin)
{
	float   fMaxRate		= FLT_MAX;  // Default.
	UINT32  fpsNumerator	= 0, fpsDenominator = 0;

	if (!bThin && (m_pMediaType != NULL)) {
		// Non-thinned: Use the frame rate and monitor refresh rate.

		// Frame rate:
		MFGetAttributeRatio(m_pMediaType, MF_MT_FRAME_RATE,
							&fpsNumerator, &fpsDenominator);

		// Monitor refresh rate:
		UINT MonitorRateHz = m_refreshRate; // D3DDISPLAYMODE

		if (fpsDenominator && fpsNumerator && MonitorRateHz) {
			// Max Rate = Refresh Rate / Frame Rate
			fMaxRate = (float)MulDiv(MonitorRateHz, fpsDenominator, fpsNumerator);
		}
	}
	return fMaxRate;
}

void CEVRAllocatorPresenter::CompleteFrameStep(bool bCancel)
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
STDMETHODIMP CEVRAllocatorPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
	HRESULT hr = S_OK;

	switch (eMessage) {
		case MFVP_MESSAGE_BEGINSTREAMING :			// The EVR switched from stopped to paused. The presenter should allocate resources
			TRACE_EVR("EVR: MFVP_MESSAGE_BEGINSTREAMING\n");
			ResetStats();
			m_bStreamChanged = TRUE;
			break;

		case MFVP_MESSAGE_CANCELSTEP :				// Cancels a frame step
			TRACE_EVR("EVR: MFVP_MESSAGE_CANCELSTEP\n");
			CompleteFrameStep(true);
			break;

		case MFVP_MESSAGE_ENDOFSTREAM :				// All input streams have ended.
			TRACE_EVR("EVR: MFVP_MESSAGE_ENDOFSTREAM\n");
			m_bPendingMediaFinished = true;
			m_bStreamChanged = TRUE;
			break;

		case MFVP_MESSAGE_ENDSTREAMING :			// The EVR switched from running or paused to stopped. The presenter should free resources
			TRACE_EVR("EVR: MFVP_MESSAGE_ENDSTREAMING\n");
			break;

		case MFVP_MESSAGE_FLUSH :					// The presenter should discard any pending samples
			TRACE_EVR("EVR: MFVP_MESSAGE_FLUSH\n");
			SetEvent(m_hEvtFlush);
			m_bEvtFlush = true;
			while (WaitForSingleObject(m_hEvtFlush, 1) == WAIT_OBJECT_0);
			break;

		case MFVP_MESSAGE_INVALIDATEMEDIATYPE :		// The mixer's output format has changed. The EVR will initiate format negotiation, as described previously
			TRACE_EVR("EVR: MFVP_MESSAGE_INVALIDATEMEDIATYPE\n");
			/*
				1) The EVR sets the media type on the reference stream.
				2) The EVR calls IMFVideoPresenter::ProcessMessage on the presenter with the MFVP_MESSAGE_INVALIDATEMEDIATYPE message.
				3) The presenter sets the media type on the mixer's output stream.
				4) The EVR sets the media type on the substreams.
			*/
			if (!m_hRenderThread) {// handle things here
				CAutoLock lock2(&m_ImageProcessingLock);
				CAutoLock cRenderLock(&m_RenderLock);
				RenegotiateMediaType();
			} else {// leave it to the other thread
				m_hEvtRenegotiate = CreateEvent(NULL, TRUE, FALSE, NULL);
				EXECUTE_ASSERT(WAIT_OBJECT_0 == WaitForSingleObject(m_hEvtRenegotiate, INFINITE));
				EXECUTE_ASSERT(CloseHandle(m_hEvtRenegotiate));
				m_hEvtRenegotiate = NULL;
			}

			break;

		case MFVP_MESSAGE_PROCESSINPUTNOTIFY :		// One input stream on the mixer has received a new sample
			//		GetImageFromMixer();
			break;

		case MFVP_MESSAGE_STEP :					// Requests a frame step.
			TRACE_EVR("EVR: MFVP_MESSAGE_STEP\n");
			m_nStepCount = (int)ulParam;
			hr = S_OK;
			break;

		default:
			ASSERT(FALSE);
			break;
	}
	return hr;
}

HRESULT CEVRAllocatorPresenter::IsMediaTypeSupported(IMFMediaType* pMixerType)
{
	HRESULT hr;

	// We support only video types
	GUID MajorType;
	hr = pMixerType->GetMajorType(&MajorType);

	if (SUCCEEDED(hr)) {
		if (MajorType != MFMediaType_Video) {
			hr = MF_E_INVALIDMEDIATYPE;
		}
	}

	// We support only progressive formats
	MFVideoInterlaceMode InterlaceMode = MFVideoInterlace_Unknown;

	if (SUCCEEDED(hr)) {
		hr = pMixerType->GetUINT32(MF_MT_INTERLACE_MODE, (UINT32*)&InterlaceMode);
	}

	if (SUCCEEDED(hr)) {
		if (InterlaceMode != MFVideoInterlace_Progressive) {
			hr = MF_E_INVALIDMEDIATYPE;
		}
	}

	// Check whether we support the surface format
	int Merit = 0;

	if (SUCCEEDED(hr)) {
		hr = GetMixerMediaTypeMerit(pMixerType, &Merit);
	}

	return hr;
}

HRESULT CEVRAllocatorPresenter::CreateProposedOutputType(IMFMediaType* pMixerType, IMFMediaType** pType)
{
	HRESULT			hr;
	AM_MEDIA_TYPE*	pAMMedia = NULL;
	LARGE_INTEGER	i64Size;
	MFVIDEOFORMAT*	VideoFormat;

	CHECK_HR(pMixerType->GetRepresentation(FORMAT_MFVideoFormat, (LPVOID*)&pAMMedia));

	VideoFormat = (MFVIDEOFORMAT*)pAMMedia->pbFormat;
	hr = pfMFCreateVideoMediaType(VideoFormat, &m_pMediaType);

#if 0
	// This code doesn't work, use same method as VMR9 instead
	if (VideoFormat->videoInfo.FramesPerSecond.Numerator != 0) {
		switch (VideoFormat->videoInfo.InterlaceMode) {
			case MFVideoInterlace_Progressive:
			case MFVideoInterlace_MixedInterlaceOrProgressive:
			default: {
				m_rtTimePerFrame = (10000000I64*VideoFormat->videoInfo.FramesPerSecond.Denominator)/VideoFormat->videoInfo.FramesPerSecond.Numerator;
				m_bInterlaced = false;
			}
			break;
			case MFVideoInterlace_FieldSingleUpper:
			case MFVideoInterlace_FieldSingleLower:
			case MFVideoInterlace_FieldInterleavedUpperFirst:
			case MFVideoInterlace_FieldInterleavedLowerFirst: {
				m_rtTimePerFrame = (20000000I64*VideoFormat->videoInfo.FramesPerSecond.Denominator)/VideoFormat->videoInfo.FramesPerSecond.Numerator;
				m_bInterlaced = true;
			}
			break;
		}
	}
#endif

	m_aspectRatio.cx = VideoFormat->videoInfo.PixelAspectRatio.Numerator;
	m_aspectRatio.cy = VideoFormat->videoInfo.PixelAspectRatio.Denominator;

	if (SUCCEEDED(hr)) {
		i64Size.HighPart = VideoFormat->videoInfo.dwWidth;
		i64Size.LowPart	 = VideoFormat->videoInfo.dwHeight;
		m_pMediaType->SetUINT64(MF_MT_FRAME_SIZE, i64Size.QuadPart);
		m_pMediaType->SetUINT32(MF_MT_PAN_SCAN_ENABLED, 0);

		CRenderersSettings& rs = GetRenderersSettings();

		if (rs.m_AdvRendSets.iEVROutputRange == 1) {
			m_pMediaType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_16_235);
		} else {
			m_pMediaType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_0_255);
		}

		m_LastSetOutputRange = rs.m_AdvRendSets.iEVROutputRange;

		i64Size.HighPart = m_aspectRatio.cx;
		i64Size.LowPart  = m_aspectRatio.cy;
		m_pMediaType->SetUINT64(MF_MT_PIXEL_ASPECT_RATIO, i64Size.QuadPart);

		MFVideoArea Area = MakeArea(0, 0, VideoFormat->videoInfo.dwWidth, VideoFormat->videoInfo.dwHeight);
		m_pMediaType->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&Area, sizeof(MFVideoArea));

	}

	m_aspectRatio.cx *= VideoFormat->videoInfo.dwWidth;
	m_aspectRatio.cy *= VideoFormat->videoInfo.dwHeight;

	if (m_aspectRatio.cx >= 1 && m_aspectRatio.cy >= 1) {
		ReduceDim(m_aspectRatio);
	}

	AfxGetApp()->m_pMainWnd->PostMessage(WM_REARRANGERENDERLESS);

	pMixerType->FreeRepresentation(FORMAT_MFVideoFormat, (LPVOID)pAMMedia);
	m_pMediaType->QueryInterface(IID_PPV_ARGS(pType));

	return hr;
}

HRESULT CEVRAllocatorPresenter::SetMediaType(IMFMediaType* pType)
{
	CheckPointer(pType, E_POINTER);

	HRESULT hr;
	AM_MEDIA_TYPE* pAMMedia = NULL;

	CHECK_HR(pType->GetRepresentation(FORMAT_VideoInfo2, (LPVOID*)&pAMMedia));

	if (SUCCEEDED(hr == InitializeDevice(pType))) {
		D3DFORMAT format = D3DFMT_UNKNOWN;
		GetMediaTypeFourCC(pType, (DWORD*)&format);
		m_strStatsMsg[MSG_MIXEROUT] = GetD3DFormatStr(format);
	}

	pType->FreeRepresentation(FORMAT_VideoInfo2, (LPVOID)pAMMedia);

	return hr;
}

HRESULT CEVRAllocatorPresenter::GetMediaTypeFourCC(IMFMediaType* pType, DWORD* pFourCC)
{
	if (pFourCC == NULL) {
		return E_POINTER;
	}

	GUID guidSubType = GUID_NULL;
	HRESULT hr = pType->GetGUID(MF_MT_SUBTYPE, &guidSubType);

	if (SUCCEEDED(hr)) {
		*pFourCC = guidSubType.Data1;
	}

	return hr;
}

HRESULT CEVRAllocatorPresenter::GetMixerMediaTypeMerit(IMFMediaType* pType, int* pMerit)
{
	DWORD mix_fmt;
	HRESULT hr = GetMediaTypeFourCC(pType, &mix_fmt);

	if (SUCCEEDED(hr)) {
		// Information about actual YUV formats - http://msdn.microsoft.com/en-us/library/windows/desktop/dd206750%28v=vs.85%29.aspx
		// DirectShow EVR not support 10 and 16-bit format

		// Test result
		// EVR input formats: NV12, YV12, YUY2, AYUV, RGB32, ARGB32, AI44 and P010 (GTX 960).
		// EVR mixer formats
		// Intel: YUY2, X8R8G8B8, A8R8G8B8 (HD 4000).
		// Nvidia: NV12, YUY2, X8R8G8B8 (GTX 660Ti, GTX 960).
		// ATI/AMD: NV12, X8R8G8B8 (HD 5770)

		switch (mix_fmt) {
			case D3DFMT_A8R8G8B8:// an accepted format, but fails on most surface types
			case D3DFMT_A8B8G8R8:
			case D3DFMT_X8B8G8R8:
			case D3DFMT_R8G8B8:
			case D3DFMT_R5G6B5:
			case D3DFMT_X1R5G5B5:
			case D3DFMT_A1R5G5B5:
			case D3DFMT_A4R4G4B4:
			case D3DFMT_R3G3B2:
			case D3DFMT_A8R3G3B2:
			case D3DFMT_X4R4G4B4:
			case D3DFMT_A8P8:
			case D3DFMT_P8:
				*pMerit = 0;
				return MF_E_INVALIDMEDIATYPE;
		}

		*pMerit = 2;

		if (m_inputMediaType.subtype == MEDIASUBTYPE_NV12 || m_inputMediaType.subtype == MEDIASUBTYPE_YV12) {
			switch (mix_fmt) {
				case FCC('NV12'): *pMerit = 90; break;
				case FCC('YUY2'): *pMerit = 80; break;
				case D3DFMT_X8R8G8B8: *pMerit = 70; break;
				case D3DFMT_A2R10G10B10: *pMerit = 60; break;
			}
		}
		else if (m_inputMediaType.subtype == MEDIASUBTYPE_YUY2) {
			switch (mix_fmt) {
				case FCC('YUY2'): *pMerit = 90; break;
				case D3DFMT_X8R8G8B8: *pMerit = 80; break;
				case D3DFMT_A2R10G10B10: *pMerit = 70; break;
				case FCC('NV12'): *pMerit = 60; break; // colour degradation
			}
		}
		else if (m_inputMediaType.subtype == MEDIASUBTYPE_P010) {
			switch (mix_fmt) {
				case D3DFMT_A2R10G10B10: *pMerit = 90; break;
				case D3DFMT_X8R8G8B8: *pMerit = 80; break; // bit depth degradation
				case FCC('YUY2'): *pMerit = 70; break; // bit depth degradation
				case FCC('NV12'): *pMerit = 60; break; // bit depth degradation
			}
		}
		else if (m_inputMediaType.subtype == MEDIASUBTYPE_AYUV
				|| m_inputMediaType.subtype == MEDIASUBTYPE_RGB32
				|| m_inputMediaType.subtype == MEDIASUBTYPE_ARGB32) {
			switch (mix_fmt) {
				case D3DFMT_X8R8G8B8: *pMerit = 90; break;
				case D3DFMT_A2R10G10B10: *pMerit = 80; break;
				case FCC('YUY2'): *pMerit = 70; break; // colour degradation
				case FCC('NV12'): *pMerit = 60; break; // colour degradation
			}
		}
	}

	return hr;
}

LPCTSTR FindD3DFormat(const D3DFORMAT Format);

LPCTSTR CEVRAllocatorPresenter::GetMediaTypeFormatDesc(IMFMediaType* pMediaType)
{
	D3DFORMAT Format = D3DFMT_UNKNOWN;
	GetMediaTypeFourCC(pMediaType, (DWORD*)&Format);
	return FindD3DFormat(Format);
}

HRESULT CEVRAllocatorPresenter::RenegotiateMediaType()
{
	HRESULT hr = S_OK;

	CComPtr<IMFMediaType> pMixerType;
	CComPtr<IMFMediaType> pType;

	if (!m_pMixer) {
		return MF_E_INVALIDREQUEST;
	}

	CInterfaceArray<IMFMediaType> ValidMixerTypes;

	// Get the mixer's input type
	hr = m_pMixer->GetInputCurrentType(0, &pType);
	if (SUCCEEDED(hr)) {
		AM_MEDIA_TYPE* pMT;
		hr = pType->GetRepresentation(FORMAT_VideoInfo2, (LPVOID*)&pMT);
		if (SUCCEEDED(hr)) {
			m_inputMediaType = *pMT;
			pType->FreeRepresentation(FORMAT_VideoInfo2, (LPVOID)pMT);
		}
	}

	// Loop through all of the mixer's proposed output types.
	DWORD iTypeIndex = 0;
	while ((hr != MF_E_NO_MORE_TYPES)) {
		pMixerType.Release();
		pType.Release();
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

		int Merit;
		if (SUCCEEDED(hr)) {
			hr = GetMixerMediaTypeMerit(pType, &Merit);
		}

		if (SUCCEEDED(hr)) {
			size_t nTypes = ValidMixerTypes.GetCount();
			size_t iInsertPos = 0;
			for (size_t i = 0; i < nTypes; ++i) {
				int ThisMerit;
				GetMixerMediaTypeMerit(ValidMixerTypes[i], &ThisMerit);

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
#ifdef _DEBUG
	for (size_t i = 0; i < nValidTypes; ++i) {
		// Step 3. Adjust the mixer's type to match our requirements.
		pType = ValidMixerTypes[i];
		DbgLog((LOG_TRACE, 3, L"EVR: Valid mixer output type: %s", GetMediaTypeFormatDesc(pType)));
	}
#endif
	for (size_t i = 0; i < nValidTypes; ++i) {
		// Step 3. Adjust the mixer's type to match our requirements.
		pType = ValidMixerTypes[i];

		TRACE_EVR("EVR: Trying mixer output type: %ws\n", GetMediaTypeFormatDesc(pType));

		// Step 5. Try to set the media type on ourselves.
		hr = SetMediaType(pType);

		// Step 6. Set output media type on mixer.
		if (SUCCEEDED(hr)) {
			hr = m_pMixer->SetOutputType(0, pType, 0);

			// If something went wrong, clear the media type.
			if (FAILED(hr)) {
				SetMediaType(NULL);
			} else {
				m_bStreamChanged = FALSE;
				break;
			}
		}
	}

	if (m_nRenderState == Started || m_nRenderState == Paused) {
		m_bChangeMT = true;
	}

	return hr;
}

bool CEVRAllocatorPresenter::GetImageFromMixer()
{
	MFT_OUTPUT_DATA_BUFFER		Buffer;
	HRESULT						hr = S_OK;
	DWORD						dwStatus;
	REFERENCE_TIME				nsSampleTime;
	LONGLONG					llClockBefore = 0;
	LONGLONG					llClockAfter  = 0;
	LONGLONG					llMixerLatency;
	UINT						dwSurface;

	bool bDoneSomething = false;

	while (SUCCEEDED(hr)) {
		CComPtr<IMFSample> pSample;

		if (FAILED(GetFreeSample(&pSample))) {
			m_bWaitingSample = true;
			break;
		}

		memset(&Buffer, 0, sizeof(Buffer));
		Buffer.pSample	= pSample;
		pSample->GetUINT32(GUID_SURFACE_INDEX, &dwSurface);

		{
			llClockBefore = GetPerfCounter();
			hr = m_pMixer->ProcessOutput(0 , 1, &Buffer, &dwStatus);
			llClockAfter = GetPerfCounter();
		}

		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
			MoveToFreeList (pSample, false);
			break;
		}

		if (m_pSink) {
			//CAutoLock autolock(this); We shouldn't need to lock here, m_pSink is thread safe
			llMixerLatency = llClockAfter - llClockBefore;
			m_pSink->Notify(EC_PROCESSING_LATENCY, (LONG_PTR)&llMixerLatency, 0);
		}

		pSample->GetSampleTime(&nsSampleTime);
		REFERENCE_TIME nsDuration;
		pSample->GetSampleDuration(&nsDuration);

		if (GetRenderersData()->m_bTearingTest) {
			RECT rcTearing;

			rcTearing.left		= m_nTearingPos;
			rcTearing.top		= 0;
			rcTearing.right		= rcTearing.left + 4;
			rcTearing.bottom	= m_nativeVideoSize.cy;
			m_pD3DDev->ColorFill(m_pVideoSurface[dwSurface], &rcTearing, D3DCOLOR_ARGB(255, 255, 0, 0));

			rcTearing.left		= (rcTearing.right + 15) % m_nativeVideoSize.cx;
			rcTearing.right		= rcTearing.left + 4;
			m_pD3DDev->ColorFill(m_pVideoSurface[dwSurface], &rcTearing, D3DCOLOR_ARGB(255, 255, 0, 0));
			m_nTearingPos		= (m_nTearingPos + 7) % m_nativeVideoSize.cx;
		}

		TRACE_EVR("EVR: Get from Mixer : %d  (%I64d) (%I64d)\n", dwSurface, nsSampleTime, m_rtTimePerFrame ? nsSampleTime / m_rtTimePerFrame : 0);

		MoveToScheduledList (pSample, false);
		bDoneSomething = true;
		if (m_rtTimePerFrame == 0) {
			break;
		}
	}

	return bDoneSomething;
}

STDMETHODIMP CEVRAllocatorPresenter::GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType)
{
	HRESULT hr = S_OK;
	CAutoLock lock(this);  // Hold the critical section.

	CheckPointer(ppMediaType, E_POINTER);
	CHECK_HR(CheckShutdown());

	if (m_pMediaType == NULL) {
		CHECK_HR(MF_E_NOT_INITIALIZED);
	}

	CHECK_HR(m_pMediaType->QueryInterface(__uuidof(IMFVideoMediaType), (void**)&ppMediaType));

	return hr;
}

// IMFTopologyServiceLookupClient
STDMETHODIMP CEVRAllocatorPresenter::InitServicePointers(/* [in] */ __in  IMFTopologyServiceLookup *pLookup)
{
	TRACE_EVR("EVR: CEVRAllocatorPresenter::InitServicePointers\n");

	HRESULT hr;
	DWORD	dwObjects = 1;

	ASSERT(!m_pMixer);
	hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_MIXER_SERVICE, __uuidof(IMFTransform), (void**)&m_pMixer, &dwObjects);
	if (FAILED(hr)) {
		ASSERT(0);
		return hr;
	}

	ASSERT(!m_pSink);
	hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE, __uuidof(IMediaEventSink ), (void**)&m_pSink, &dwObjects);
	if (FAILED(hr)) {
		ASSERT(0);
		m_pMixer.Release();
		return hr;
	}

	ASSERT(!m_pClock);
	hr = pLookup->LookupService(MF_SERVICE_LOOKUP_GLOBAL, 0, MR_VIDEO_RENDER_SERVICE, __uuidof(IMFClock ), (void**)&m_pClock, &dwObjects);
	if (FAILED(hr)) {	// IMFClock can't be guaranteed to exist during first initialization. After negotiating the media type, it should initialize okay.
		return S_OK;
	}

	StartWorkerThreads();
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::ReleaseServicePointers()
{
	TRACE_EVR("EVR: CEVRAllocatorPresenter::ReleaseServicePointers\n");
	StopWorkerThreads();
	m_pMixer.Release();
	m_pSink.Release();
	m_pClock.Release();
	return S_OK;
}

// IMFVideoDeviceID
STDMETHODIMP CEVRAllocatorPresenter::GetDeviceID(/* [out] */ __out  IID *pDeviceID)
{
	CheckPointer(pDeviceID, E_POINTER);
	*pDeviceID = IID_IDirect3DDevice9;
	return S_OK;
}

// IMFGetService
STDMETHODIMP CEVRAllocatorPresenter::GetService (/* [in] */ __RPC__in REFGUID guidService,
		/* [in] */ __RPC__in REFIID riid,
		/* [iid_is][out] */ __RPC__deref_out_opt LPVOID *ppvObject)
{
	if (guidService == MR_VIDEO_RENDER_SERVICE) {
		return NonDelegatingQueryInterface (riid, ppvObject);
	} else if (guidService == MR_VIDEO_ACCELERATION_SERVICE) {
		return m_pD3DManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppvObject);
	}

	return E_NOINTERFACE;
}

// IMFAsyncCallback
STDMETHODIMP CEVRAllocatorPresenter::GetParameters(	/* [out] */ __RPC__out DWORD *pdwFlags, /* [out] */ __RPC__out DWORD *pdwQueue)
{
	return E_NOTIMPL;
}

STDMETHODIMP CEVRAllocatorPresenter::Invoke		 (	/* [in] */ __RPC__in_opt IMFAsyncResult *pAsyncResult)
{
	return E_NOTIMPL;
}

// IMFVideoDisplayControl
STDMETHODIMP CEVRAllocatorPresenter::GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo)
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

STDMETHODIMP CEVRAllocatorPresenter::GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax)
{
	if (pszMin) {
		pszMin->cx	= 1;
		pszMin->cy	= 1;
	}

	if (pszMax) {
		D3DDISPLAYMODE	d3ddm;

		ZeroMemory(&d3ddm, sizeof(d3ddm));
		if (SUCCEEDED(m_pD3D->GetAdapterDisplayMode(GetAdapter(m_pD3D), &d3ddm))) {
			pszMax->cx	= d3ddm.Width;
			pszMax->cy	= d3ddm.Height;
		}
	}

	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest)
{
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest)
{
	// Always all source rectangle ?
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

STDMETHODIMP CEVRAllocatorPresenter::SetAspectRatioMode(DWORD dwAspectRatioMode)
{
	m_dwVideoAspectRatioMode = (MFVideoAspectRatioMode)dwAspectRatioMode;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::GetAspectRatioMode(DWORD *pdwAspectRatioMode)
{
	CheckPointer(pdwAspectRatioMode, E_POINTER);
	*pdwAspectRatioMode = m_dwVideoAspectRatioMode;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::SetVideoWindow(HWND hwndVideo)
{
	CAutoLock lock(this);
	CAutoLock lock2(&m_ImageProcessingLock);
	CAutoLock cRenderLock(&m_RenderLock);

	if (m_hWnd != hwndVideo) {
		m_hWnd = hwndVideo;
		m_bPendingResetDevice = true;
		SendResetRequest();
	}
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::GetVideoWindow(HWND *phwndVideo)
{
	CheckPointer(phwndVideo, E_POINTER);
	*phwndVideo = m_hWnd;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::RepaintVideo()
{
	Paint (true);
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp)
{
	ASSERT(FALSE);
	return E_NOTIMPL;
}

STDMETHODIMP CEVRAllocatorPresenter::SetBorderColor(COLORREF Clr)
{
	m_BorderColor = Clr;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::GetBorderColor(COLORREF *pClr)
{
	CheckPointer(pClr, E_POINTER);
	*pClr = m_BorderColor;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::SetRenderingPrefs(DWORD dwRenderFlags)
{
	m_dwVideoRenderPrefs = (MFVideoRenderPrefs)dwRenderFlags;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::GetRenderingPrefs(DWORD *pdwRenderFlags)
{
	CheckPointer(pdwRenderFlags, E_POINTER);
	*pdwRenderFlags = m_dwVideoRenderPrefs;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::SetFullscreen(BOOL fFullscreen)
{
	ASSERT(FALSE);
	return E_NOTIMPL;
}

STDMETHODIMP CEVRAllocatorPresenter::GetFullscreen(BOOL *pfFullscreen)
{
	ASSERT(FALSE);
	return E_NOTIMPL;
}

// IEVRTrustedVideoPlugin
STDMETHODIMP CEVRAllocatorPresenter::IsInTrustedVideoMode(BOOL *pYes)
{
	CheckPointer(pYes, E_POINTER);
	*pYes = TRUE;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::CanConstrict(BOOL *pYes)
{
	CheckPointer(pYes, E_POINTER);
	*pYes = TRUE;
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::SetConstriction(DWORD dwKPix)
{
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::DisableImageExport(BOOL bDisable)
{
	return S_OK;
}

// IDirect3DDeviceManager9
STDMETHODIMP CEVRAllocatorPresenter::ResetDevice(IDirect3DDevice9 *pDevice, UINT resetToken)
{
	HRESULT hr = m_pD3DManager->ResetDevice(pDevice, resetToken);
	return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::OpenDeviceHandle(HANDLE *phDevice)
{
	HRESULT hr = m_pD3DManager->OpenDeviceHandle(phDevice);
	return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::CloseDeviceHandle(HANDLE hDevice)
{
	HRESULT hr = m_pD3DManager->CloseDeviceHandle(hDevice);
	return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::TestDevice(HANDLE hDevice)
{
	HRESULT hr = m_pD3DManager->TestDevice(hDevice);
	return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::LockDevice(HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock)
{
	HRESULT hr = m_pD3DManager->LockDevice(hDevice, ppDevice, fBlock);
	return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::UnlockDevice(HANDLE hDevice, BOOL fSaveState)
{
	HRESULT hr = m_pD3DManager->UnlockDevice(hDevice, fSaveState);
	return hr;
}

STDMETHODIMP CEVRAllocatorPresenter::GetVideoService(HANDLE hDevice, REFIID riid, void **ppService)
{
	HRESULT hr = m_pD3DManager->GetVideoService(hDevice, riid, ppService);

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

STDMETHODIMP CEVRAllocatorPresenter::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	// This function should be called...
	ASSERT(FALSE);

	if (lpWidth) {
		*lpWidth	= m_nativeVideoSize.cx;
	}
	if (lpHeight) {
		*lpHeight	= m_nativeVideoSize.cy;
	}
	if (lpARWidth) {
		*lpARWidth	= m_aspectRatio.cx;
	}
	if (lpARHeight) {
		*lpARHeight	= m_aspectRatio.cy;
	}
	return S_OK;
}

STDMETHODIMP CEVRAllocatorPresenter::InitializeDevice(IMFMediaType* pMediaType)
{
	HRESULT hr;
	CAutoLock lock(this);
	CAutoLock lock2(&m_ImageProcessingLock);
	CAutoLock cRenderLock(&m_RenderLock);

	// Retrieve the surface size and format
	UINT32 width;
	UINT32 height;
	hr = MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);

	D3DFORMAT Format;
	if (SUCCEEDED(hr)) {
		CSize frameSize(width, height);
		m_bStreamChanged |= m_nativeVideoSize != frameSize;
		m_nativeVideoSize = frameSize;
		hr = GetMediaTypeFourCC(pMediaType, (DWORD*)&Format);
	}

	if (m_bStreamChanged && SUCCEEDED(hr)) {
		DeleteSurfaces();
		RemoveAllSamples();
		hr = AllocSurfaces();
	}

	if (m_bStreamChanged && SUCCEEDED(hr)) {
		for (int i = 0; i < m_nNbDXSurface; i++) {
			CComPtr<IMFSample> pMFSample;
			hr = pfMFCreateVideoSampleFromSurface(m_pVideoSurface[i], &pMFSample);

			if (SUCCEEDED(hr)) {
				pMFSample->SetUINT32(GUID_SURFACE_INDEX, i);
				m_FreeSamples.AddTail(pMFSample);
			}
			ASSERT(SUCCEEDED(hr));
		}
	}

	return hr;
}

DWORD WINAPI CEVRAllocatorPresenter::GetMixerThreadStatic(LPVOID lpParam)
{
	SetThreadName(DWORD_MAX, "CEVRPresenter::MixerThread");
	CEVRAllocatorPresenter*	pThis = (CEVRAllocatorPresenter*)lpParam;
	pThis->GetMixerThread();
	return 0;
}

DWORD WINAPI CEVRAllocatorPresenter::PresentThread(LPVOID lpParam)
{
	SetThreadName(DWORD_MAX, "CEVRPresenter::PresentThread");
	CEVRAllocatorPresenter* pThis = (CEVRAllocatorPresenter*)lpParam;
	pThis->RenderThread();
	return 0;
}

void CEVRAllocatorPresenter::CheckWaitingSampleFromMixer()
{
	if (m_bWaitingSample) {
		m_bWaitingSample = false;
		//GetImageFromMixer(); // Do this in processing thread instead
	}
}

bool ExtractInterlaced(const AM_MEDIA_TYPE* pmt)
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

void CEVRAllocatorPresenter::GetMixerThread()
{
	bool		bQuit	= false;
	TIMECAPS	tc;
	DWORD		dwResolution;

	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
	timeBeginPeriod(dwResolution);

	while (!bQuit) {
		DWORD dwObject = WaitForSingleObject(m_hEvtQuit, 1);
		switch (dwObject) {
			case WAIT_OBJECT_0 :
				bQuit = true;
				break;
			case WAIT_TIMEOUT : {
				if (m_nRenderState == Stopped) {
					continue; // nothing sensible to do here
				}

				bool bDoneSomething = false;
				{
					CAutoLock AutoLock(&m_ImageProcessingLock);
					bDoneSomething = GetImageFromMixer();
				}
				if ((m_rtTimePerFrame == 0 && bDoneSomething) || m_bChangeMT) {
					//CAutoLock lock(this);
					//CAutoLock lock2(&m_ImageProcessingLock);
					//CAutoLock cRenderLock(&m_RenderLock);

					// Use the code from VMR9 to get the movie fps, as this method is reliable.
					CComPtr<IPin>	pPin;
					CMediaType		mt;
					if (SUCCEEDED(m_pOuterEVR->FindPin(L"EVR Input0", &pPin)) &&
						SUCCEEDED(pPin->ConnectionMediaType(&mt)) ) {

						FillAddingField(pPin, &mt);
					}
					// If framerate not set by Video Decoder - choose 23.976
					if (m_rtTimePerFrame == 1) {
						m_rtTimePerFrame = 417083;
					}

					// Update internal subtitle clock
					if (m_fUseInternalTimer && m_pSubPicQueue) {
						m_fps = 10000000.0 / m_rtTimePerFrame;
						m_pSubPicQueue->SetFPS(m_fps);
					}

					m_bChangeMT = false;
				}
			}
			break;
		}
	}

	timeEndPeriod(dwResolution);
}

void ModerateFloat(double& Value, double Target, double& ValuePrim, double ChangeSpeed)
{
	double xbiss = (-(ChangeSpeed)*(ValuePrim) - (Value-Target)*(ChangeSpeed*ChangeSpeed)*0.25f);
	ValuePrim += xbiss;
	Value += ValuePrim;
}

LONGLONG CEVRAllocatorPresenter::GetClockTime(LONGLONG PerformanceCounter)
{
	LONGLONG			llClockTime;
	MFTIME				nsCurrentTime;
	m_pClock->GetCorrelatedTime(0, &llClockTime, &nsCurrentTime);
	DWORD Characteristics = 0;
	m_pClock->GetClockCharacteristics(&Characteristics);
	MFCLOCK_STATE State;
	m_pClock->GetState(0, &State);

	if (!(Characteristics & MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ)) {
		MFCLOCK_PROPERTIES Props;
		if (m_pClock->GetProperties(&Props) == S_OK) {
			llClockTime = (llClockTime * 10000000) / Props.qwClockFrequency;    // Make 10 MHz
		}

	}
	LONGLONG llPerf = PerformanceCounter;
	//	return llClockTime + (llPerf - nsCurrentTime);
	double Target = llClockTime + (llPerf - nsCurrentTime) * m_ModeratedTimeSpeed;

	bool bReset = false;
	if (m_ModeratedTimeLast < 0 || State != m_LastClockState || m_ModeratedClockLast < 0) {
		bReset = true;
		m_ModeratedTimeLast = llPerf;
		m_ModeratedClockLast = llClockTime;
	}

	m_LastClockState = State;

	LONGLONG TimeChangeM = llPerf - m_ModeratedTimeLast;
	LONGLONG ClockChangeM = llClockTime - m_ModeratedClockLast;
	UNREFERENCED_PARAMETER(ClockChangeM);

	m_ModeratedTimeLast = llPerf;
	m_ModeratedClockLast = llClockTime;

#if 1

	if (bReset) {
		m_ModeratedTimeSpeed = 1.0;
		m_ModeratedTimeSpeedPrim = 0.0;
		ZeroMemory(m_TimeChangeHistory, sizeof(m_TimeChangeHistory));
		ZeroMemory(m_ClockChangeHistory, sizeof(m_ClockChangeHistory));
		m_ClockTimeChangeHistoryPos = 0;
	}
	if (TimeChangeM) {
		int Pos = m_ClockTimeChangeHistoryPos % 100;
		int nHistory = min(m_ClockTimeChangeHistoryPos, 100);
		++m_ClockTimeChangeHistoryPos;
		if (nHistory > 50) {
			int iLastPos = (Pos - (nHistory)) % 100;
			if (iLastPos < 0) {
				iLastPos += 100;
			}

			double TimeChange = llPerf - m_TimeChangeHistory[iLastPos];
			double ClockChange = llClockTime - m_ClockChangeHistory[iLastPos];

			double ClockSpeedTarget = ClockChange / TimeChange;
			double ChangeSpeed = 0.1;
			if (ClockSpeedTarget > m_ModeratedTimeSpeed) {
				if (ClockSpeedTarget / m_ModeratedTimeSpeed > 0.1) {
					ChangeSpeed = 0.1;
				} else {
					ChangeSpeed = 0.01;
				}
			} else {
				if (m_ModeratedTimeSpeed / ClockSpeedTarget > 0.1) {
					ChangeSpeed = 0.1;
				} else {
					ChangeSpeed = 0.01;
				}
			}
			ModerateFloat(m_ModeratedTimeSpeed, ClockSpeedTarget, m_ModeratedTimeSpeedPrim, ChangeSpeed);
			//m_ModeratedTimeSpeed = TimeChange / ClockChange;
		}
		m_TimeChangeHistory[Pos] = (double)llPerf;
		m_ClockChangeHistory[Pos] = (double)llClockTime;
	}

	return (LONGLONG)Target;
#else
	double EstimateTime = m_ModeratedTime + TimeChange * m_ModeratedTimeSpeed + m_ClockDiffCalc;
	double Diff = Target - EstimateTime;

	// > 5 ms just set it
	if ((fabs(Diff) > 50000.0 || bReset)) {

		//TRACE_EVR("EVR: Reset clock at diff: %f ms\n", (m_ModeratedTime - Target) /10000.0);
		if (State == MFCLOCK_STATE_RUNNING) {
			if (bReset) {
				m_ModeratedTimeSpeed = 1.0;
				m_ModeratedTimeSpeedPrim = 0.0;
				m_ClockDiffCalc = 0;
				m_ClockDiffPrim = 0;
				m_ModeratedTime = Target;
				m_ModeratedTimer = llPerf;
			} else {
				EstimateTime = m_ModeratedTime + TimeChange * m_ModeratedTimeSpeed;
				Diff = Target - EstimateTime;
				m_ClockDiffCalc = Diff;
				m_ClockDiffPrim = 0;
			}
		} else {
			m_ModeratedTimeSpeed = 0.0;
			m_ModeratedTimeSpeedPrim = 0.0;
			m_ClockDiffCalc = 0;
			m_ClockDiffPrim = 0;
			m_ModeratedTime = Target;
			m_ModeratedTimer = llPerf;
		}
	}

	{
		LONGLONG ModerateTime = 10000;
		double ChangeSpeed = 1.00;
		/*		if (m_ModeratedTimeSpeedPrim != 0.0)
				{
					if (m_ModeratedTimeSpeedPrim < 0.1)
						ChangeSpeed = 0.1;
				}*/

		int nModerate = 0;
		double Change = 0;
		while (m_ModeratedTimer < llPerf - ModerateTime) {
			m_ModeratedTimer += ModerateTime;
			m_ModeratedTime += double(ModerateTime) * m_ModeratedTimeSpeed;

			double TimerDiff = llPerf - m_ModeratedTimer;

			double Diff = (double)(m_ModeratedTime - (Target - TimerDiff));

			double TimeSpeedTarget;
			double AbsDiff = fabs(Diff);
			TimeSpeedTarget = 1.0 - (Diff / 1000000.0);
			//			TimeSpeedTarget = m_ModeratedTimeSpeed - (Diff / 100000000000.0);
			//if (AbsDiff > 20000.0)
			//				TimeSpeedTarget = 1.0 - (Diff / 1000000.0);
			/*else if (AbsDiff > 5000.0)
				TimeSpeedTarget = 1.0 - (Diff / 100000000.0);
			else
				TimeSpeedTarget = 1.0 - (Diff / 500000000.0);*/
			double StartMod = m_ModeratedTimeSpeed;
			ModerateFloat(m_ModeratedTimeSpeed, TimeSpeedTarget, m_ModeratedTimeSpeedPrim, ChangeSpeed);
			m_ModeratedTimeSpeed = TimeSpeedTarget;
			++nModerate;
			Change += m_ModeratedTimeSpeed - StartMod;
		}
		if (nModerate) {
			m_ModeratedTimeSpeedDiff = Change / nModerate;
		}

		double Ret = m_ModeratedTime + double(llPerf - m_ModeratedTimer) * m_ModeratedTimeSpeed;
		double Diff = Target - Ret;
		ModerateFloat(m_ClockDiffCalc, Diff, m_ClockDiffPrim, ChangeSpeed*0.1);

		Ret += m_ClockDiffCalc;
		Diff = Target - Ret;
		m_ClockDiff = Diff;
		return LONGLONG(Ret + 0.5);
	}

	return Target;
	return LONGLONG(m_ModeratedTime + 0.5);
#endif
}

void CEVRAllocatorPresenter::OnVBlankFinished(bool fAll, LONGLONG PerformanceCounter)
{
	if (!m_pCurrentDisplaydSample || !m_OrderedPaint || !fAll) {
		return;
	}

	LONGLONG			llClockTime;
	LONGLONG			nsSampleTime;
	LONGLONG SampleDuration = 0;
	if (!m_bSignaledStarvation) {
		llClockTime = GetClockTime(PerformanceCounter);
		m_StarvationClock = llClockTime;
	} else {
		llClockTime = m_StarvationClock;
	}
	if (FAILED(m_pCurrentDisplaydSample->GetSampleDuration(&SampleDuration))) {
		SampleDuration = 0;
	}

	if (FAILED(m_pCurrentDisplaydSample->GetSampleTime(&nsSampleTime))) {
		nsSampleTime = llClockTime;
	}
	LONGLONG TimePerFrame = m_rtTimePerFrame;
	if (!TimePerFrame) {
		return;
	}
	if (SampleDuration > 1) {
		TimePerFrame = SampleDuration;
	}
	{
		m_nNextSyncOffset = (m_nNextSyncOffset+1) % NB_JITTER;
		LONGLONG SyncOffset = nsSampleTime - llClockTime;

		m_pllSyncOffset[m_nNextSyncOffset] = SyncOffset;
		//TRACE_EVR("EVR: SyncOffset(%d, %d): %8I64d     %8I64d     %8I64d \n", m_nCurSurface, m_VSyncMode, m_LastPredictedSync, -SyncOffset, m_LastPredictedSync - (-SyncOffset));

		m_MaxSyncOffset = MINLONG64;
		m_MinSyncOffset = MAXLONG64;

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
		m_bSyncStatsAvailable = true;
		m_fSyncOffsetStdDev = StdDev;
	}
}

STDMETHODIMP_(bool) CEVRAllocatorPresenter::ResetDevice()
{
	DbgLog((LOG_TRACE, 3, L"CEVRAllocatorPresenter::ResetDevice()"));

	StopWorkerThreads();

	CAutoLock lock(this);
	CAutoLock lock2(&m_ImageProcessingLock);
	CAutoLock cRenderLock(&m_RenderLock);

	RemoveAllSamples();

	bool bResult = __super::ResetDevice();

	for (int i = 0; i < m_nNbDXSurface; i++) {
		CComPtr<IMFSample> pMFSample;
		HRESULT hr = pfMFCreateVideoSampleFromSurface(m_pVideoSurface[i], &pMFSample);

		if (SUCCEEDED(hr)) {
			pMFSample->SetUINT32(GUID_SURFACE_INDEX, i);
			m_FreeSamples.AddTail(pMFSample);
		}
		ASSERT(SUCCEEDED(hr));
	}

	if (bResult) {
		StartWorkerThreads();
	}

	return bResult;
}

STDMETHODIMP_(bool) CEVRAllocatorPresenter::DisplayChange()
{
	CAutoLock lock(this);
	CAutoLock lock2(&m_ImageProcessingLock);
	CAutoLock cRenderLock(&m_RenderLock);

	return __super::DisplayChange();
}

void CEVRAllocatorPresenter::RenderThread()
{
	HANDLE		hEvts[]		= { m_hEvtQuit, m_hEvtFlush };
	bool		bQuit		= false, bForcePaint = false;
	TIMECAPS	tc;
	DWORD		dwResolution;
	MFTIME		nsSampleTime;
	LONGLONG	llClockTime;
	DWORD		dwObject;

	// Tell Multimedia Class Scheduler we are a playback thread (increase priority)
	HANDLE hAvrt = 0;
	if (pfAvSetMmThreadCharacteristicsW) {
		DWORD dwTaskIndex	= 0;
		hAvrt = pfAvSetMmThreadCharacteristicsW (L"Playback", &dwTaskIndex);
		if (pfAvSetMmThreadPriority) {
			pfAvSetMmThreadPriority (hAvrt, AVRT_PRIORITY_HIGH /*AVRT_PRIORITY_CRITICAL*/);
		}
	}

	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
	timeBeginPeriod(dwResolution);
	CRenderersSettings& rs = GetRenderersSettings();

	int NextSleepTime = 1;
	while (!bQuit) {
		LONGLONG	llPerf = GetPerfCounter();
		UNREFERENCED_PARAMETER(llPerf);
		if (!rs.m_AdvRendSets.bVSyncAccurate && NextSleepTime == 0) {
			NextSleepTime = 1;
		}
		dwObject = WaitForMultipleObjects(_countof(hEvts), hEvts, FALSE, max(NextSleepTime < 0 ? 1 : NextSleepTime, 0));
		if (m_hEvtRenegotiate) {
			CAutoLock Lock(&m_csExternalMixerLock);
			CAutoLock cRenderLock(&m_RenderLock);
			RenegotiateMediaType();
			SetEvent(m_hEvtRenegotiate);
		}
		if (NextSleepTime > 1) {
			NextSleepTime = 0;
		} else if (NextSleepTime == 0) {
			NextSleepTime = -1;
		}
		switch (dwObject) {
			case WAIT_OBJECT_0 :
				bQuit = true;
				break;
			case WAIT_OBJECT_0 + 1 :
				// Flush pending samples!
				FlushSamples();
				m_bEvtFlush = false;
				ResetEvent(m_hEvtFlush);
				bForcePaint = true;
				TRACE_EVR("EVR: Flush done!\n");
				break;

			case WAIT_TIMEOUT :

				if (m_LastSetOutputRange != -1 && m_LastSetOutputRange != rs.m_AdvRendSets.iEVROutputRange) {
					{
						CAutoLock Lock(&m_csExternalMixerLock);
						CAutoLock cRenderLock(&m_RenderLock);
						FlushSamples();
						RenegotiateMediaType();
					}
				}
				if (m_bPendingResetDevice) {
					SendResetRequest();
				}

				// Discard timer events if playback stop
				//			if ((dwObject == WAIT_OBJECT_0 + 3) && (m_nRenderState != Started)) continue;

				//			TRACE_EVR("EVR: RenderThread ==>> Waiting buffer\n");

				//			if (WaitForMultipleObjects(_countof(hEvtsBuff), hEvtsBuff, FALSE, INFINITE) == WAIT_OBJECT_0+2)
				{
					CComPtr<IMFSample> pMFSample;
					LONGLONG	llPerf = GetPerfCounter();
					UNREFERENCED_PARAMETER(llPerf);
					int nSamplesLeft = 0;
					if (SUCCEEDED(GetScheduledSample(&pMFSample, nSamplesLeft))) {
						//pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32*)&m_nCurSurface);
						m_pCurrentDisplaydSample = pMFSample;

						bool bValidSampleTime = true;
						HRESULT hGetSampleTime = pMFSample->GetSampleTime (&nsSampleTime);
						if (hGetSampleTime != S_OK || nsSampleTime == 0) {
							bValidSampleTime = false;
						}
						// We assume that all samples have the same duration
						LONGLONG SampleDuration = 0;
						pMFSample->GetSampleDuration(&SampleDuration);

						//TRACE_EVR("EVR: RenderThread ==>> Presenting surface %d  (%I64d)\n", m_nCurSurface, nsSampleTime);

						bool bStepForward = false;

						if (m_nStepCount < 0) {
							// Drop frame
							TRACE_EVR("EVR: Dropped frame\n");
							m_pcFrames++;
							bStepForward = true;
							m_nStepCount = 0;
							/*
							} else if (m_nStepCount > 0) {
								pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_nCurSurface);
								++m_OrderedPaint;
								if (!g_bExternalSubtitleTime) {
									__super::SetTime (g_tSegmentStart + nsSampleTime);
								}
								Paint(true);
								m_nDroppedUpdate = 0;
								CompleteFrameStep (false);
								bStepForward = true;
							*/
						} else if (m_nRenderState == Started) {
							LONGLONG CurrentCounter = GetPerfCounter();
							// Calculate wake up timer
							if (!m_bSignaledStarvation) {
								llClockTime = GetClockTime(CurrentCounter);
								m_StarvationClock = llClockTime;
							} else {
								llClockTime = m_StarvationClock;
							}

							if (!bValidSampleTime) {
								// Just play as fast as possible
								bStepForward = true;
								pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_nCurSurface);
								++m_OrderedPaint;
								if (!g_bExternalSubtitleTime) {
									__super::SetTime (g_tSegmentStart + nsSampleTime);
								}
								Paint(true);
							} else {
								LONGLONG TimePerFrame = (LONGLONG)(GetFrameTime() * 10000000.0);
								LONGLONG DrawTime = m_PaintTime * 9 / 10 - 20000; // 2 ms offset (= m_PaintTime * 0.9 - 20000)
								//if (!s.bVSync)
								DrawTime = 0;

								LONGLONG SyncOffset = 0;
								LONGLONG VSyncTime = 0;
								LONGLONG TimeToNextVSync = -1;
								bool bVSyncCorrection = false;
								double DetectedRefreshTime;
								double DetectedScanlinesPerFrame;
								double DetectedScanlineTime;
								int DetectedRefreshRatePos;
								{
									CAutoLock Lock(&m_RefreshRateLock);
									DetectedRefreshTime = m_DetectedRefreshTime;
									DetectedRefreshRatePos = m_DetectedRefreshRatePos;
									DetectedScanlinesPerFrame = m_DetectedScanlinesPerFrame;
									DetectedScanlineTime = m_DetectedScanlineTime;
								}

								if (DetectedRefreshRatePos < 20 || !DetectedRefreshTime || !DetectedScanlinesPerFrame) {
									DetectedRefreshTime = 1.0/m_refreshRate;
									DetectedScanlinesPerFrame = m_ScreenSize.cy;
									DetectedScanlineTime = DetectedRefreshTime / double(m_ScreenSize.cy);
								}

								if (rs.m_AdvRendSets.bVSync) {
									bVSyncCorrection = true;
									double TargetVSyncPos = GetVBlackPos();
									double RefreshLines = DetectedScanlinesPerFrame;
									double ScanlinesPerSecond = 1.0/DetectedScanlineTime;
									double CurrentVSyncPos = fmod(double(m_VBlankStartMeasure) + ScanlinesPerSecond * ((CurrentCounter - m_VBlankStartMeasureTime) / 10000000.0), RefreshLines);
									double LinesUntilVSync = 0;
									//TargetVSyncPos -= ScanlinesPerSecond * (DrawTime/10000000.0);
									//TargetVSyncPos -= 10;
									TargetVSyncPos = fmod(TargetVSyncPos, RefreshLines);
									if (TargetVSyncPos < 0) {
										TargetVSyncPos += RefreshLines;
									}
									if (TargetVSyncPos > CurrentVSyncPos) {
										LinesUntilVSync = TargetVSyncPos - CurrentVSyncPos;
									} else {
										LinesUntilVSync = (RefreshLines - CurrentVSyncPos) + TargetVSyncPos;
									}
									double TimeUntilVSync = LinesUntilVSync * DetectedScanlineTime;
									TimeToNextVSync = (LONGLONG)(TimeUntilVSync * 10000000.0);
									VSyncTime = (LONGLONG)(DetectedRefreshTime * 10000000.0);

									LONGLONG ClockTimeAtNextVSync = llClockTime + (LONGLONG)(TimeUntilVSync * 10000000.0 * m_ModeratedTimeSpeed);

									SyncOffset = (nsSampleTime - ClockTimeAtNextVSync);

									//if (SyncOffset < 0)
									//	TRACE_EVR("EVR: SyncOffset(%d): %I64d     %I64d     %I64d\n", m_nCurSurface, SyncOffset, TimePerFrame, VSyncTime);
								} else {
									SyncOffset = (nsSampleTime - llClockTime);
								}

								//LONGLONG SyncOffset = nsSampleTime - llClockTime;
								TRACE_EVR("EVR: SyncOffset: %I64d SampleFrame: %I64d ClockFrame: %I64d\n", SyncOffset, TimePerFrame!=0 ? nsSampleTime/TimePerFrame : 0, TimePerFrame!=0 ? llClockTime /TimePerFrame : 0);
								if (SampleDuration > 1 && !m_DetectedLock) {
									TimePerFrame = SampleDuration;
								}

								LONGLONG MinMargin;
								if (m_FrameTimeCorrection == 0) {
									MinMargin = MIN_FRAME_TIME;
								} else {
									MinMargin = MIN_FRAME_TIME + min(LONGLONG(m_DetectedFrameTimeStdDev), 20000);
								}
								LONGLONG TimePerFrameMargin = min(max(TimePerFrame*2/100, MinMargin), TimePerFrame*11/100); // (0.02..0.11)TimePerFrame
								LONGLONG TimePerFrameMargin0 = TimePerFrameMargin/2;
								LONGLONG TimePerFrameMargin1 = 0;

								if (m_DetectedLock && TimePerFrame < VSyncTime) {
									VSyncTime = TimePerFrame;
								}

								if (m_VSyncMode == 1) {
									TimePerFrameMargin1 = -TimePerFrameMargin;
								} else if (m_VSyncMode == 2) {
									TimePerFrameMargin1 = TimePerFrameMargin;
								}

								m_LastSampleOffset = SyncOffset;
								m_bLastSampleOffsetValid = true;

								LONGLONG VSyncOffset0 = 0;
								bool bDoVSyncCorrection = false;
								if ((SyncOffset < -(TimePerFrame + TimePerFrameMargin0 - TimePerFrameMargin1)) && nSamplesLeft > 0) { // Only drop if we have something else to display at once
									// Drop frame
									TRACE_EVR("EVR: Dropped frame\n");
									m_pcFrames++;
									bStepForward = true;
									++m_nDroppedUpdate;
									NextSleepTime = 0;
									//VSyncOffset0 = (-SyncOffset) - VSyncTime;
									//VSyncOffset0 = (-SyncOffset) - VSyncTime + TimePerFrameMargin1;
									//m_LastPredictedSync = VSyncOffset0;
									bDoVSyncCorrection = false;
								} else if (SyncOffset < TimePerFrameMargin1) {

									if (bVSyncCorrection) {
										VSyncOffset0 = -SyncOffset;
										bDoVSyncCorrection = true;
									}

									// Paint and prepare for next frame
									TRACE_EVR("EVR: Normalframe\n");
									m_nDroppedUpdate = 0;
									bStepForward = true;
									pMFSample->GetUINT32(GUID_SURFACE_INDEX, (UINT32 *)&m_nCurSurface);
									m_LastFrameDuration = nsSampleTime - m_LastSampleTime;
									m_LastSampleTime = nsSampleTime;
									m_LastPredictedSync = VSyncOffset0;

									if (m_nStepCount > 0) {
										CompleteFrameStep (false);
									}

									++m_OrderedPaint;

									if (!g_bExternalSubtitleTime) {
										__super::SetTime (g_tSegmentStart + nsSampleTime);
									}
									Paint(true);

									NextSleepTime = 0;
									m_pcFramesDrawn++;
								} else {
									if (TimeToNextVSync >= 0 && SyncOffset > 0) {
										NextSleepTime = (int)(TimeToNextVSync/10000 - 2);
									} else {
										NextSleepTime = (int)(SyncOffset/10000 - 2);
									}

									if (NextSleepTime > TimePerFrame) {
										NextSleepTime = 1;
									}

									if (NextSleepTime < 0) {
										NextSleepTime = 0;
									}
									NextSleepTime = 1;
									//TRACE_EVR("EVR: Delay\n");
								}

								if (bDoVSyncCorrection) {
									//LONGLONG VSyncOffset0 = (((SyncOffset) % VSyncTime) + VSyncTime) % VSyncTime;
									LONGLONG Margin = TimePerFrameMargin;

									LONGLONG VSyncOffsetMin = 30000000000000;
									LONGLONG VSyncOffsetMax = -30000000000000;
									for (int i = 0; i < 5; ++i) {
										VSyncOffsetMin = min(m_VSyncOffsetHistory[i], VSyncOffsetMin);
										VSyncOffsetMax = max(m_VSyncOffsetHistory[i], VSyncOffsetMax);
									}

									m_VSyncOffsetHistory[m_VSyncOffsetHistoryPos] = VSyncOffset0;
									m_VSyncOffsetHistoryPos = (m_VSyncOffsetHistoryPos + 1) % 5;

									//LONGLONG VSyncTime2 = VSyncTime2 + (VSyncOffsetMax - VSyncOffsetMin);
									//VSyncOffsetMin; = (((VSyncOffsetMin) % VSyncTime) + VSyncTime) % VSyncTime;
									//VSyncOffsetMax = (((VSyncOffsetMax) % VSyncTime) + VSyncTime) % VSyncTime;

									//TRACE_EVR("EVR: SyncOffset(%d, %d): %8I64d     %8I64d     %8I64d     %8I64d\n", m_nCurSurface, m_VSyncMode,VSyncOffset0, VSyncOffsetMin, VSyncOffsetMax, VSyncOffsetMax - VSyncOffsetMin);

									if (m_VSyncMode == 0) {
										// 23.976 in 60 Hz
										if (VSyncOffset0 < Margin && VSyncOffsetMax > (VSyncTime - Margin)) {
											m_VSyncMode = 2;
										} else if (VSyncOffset0 > (VSyncTime - Margin) && VSyncOffsetMin < Margin) {
											m_VSyncMode = 1;
										}
									} else if (m_VSyncMode == 2) {
										if (VSyncOffsetMin > (Margin)) {
											m_VSyncMode = 0;
										}
									} else if (m_VSyncMode == 1) {
										if (VSyncOffsetMax < (VSyncTime - Margin)) {
											m_VSyncMode = 0;
										}
									}
								}
							}
							} else if (m_nRenderState == Paused) {
								if (bForcePaint) {
									bStepForward = true;
									// Ensure that the renderer is properly updated after seeking when paused
									if (!g_bExternalSubtitleTime) {
										__super::SetTime(g_tSegmentStart + nsSampleTime);
									}
									Paint(false);
								}
								NextSleepTime = int(SampleDuration / 10000 - 2);
							}

						m_pCurrentDisplaydSample = NULL;
						if (bStepForward) {
							MoveToFreeList(pMFSample, true);
							CheckWaitingSampleFromMixer();
							m_MaxSampleDuration = max(SampleDuration, m_MaxSampleDuration);
						} else {
							MoveToScheduledList(pMFSample, true);
						}

						bForcePaint = false;
					} else if (m_bLastSampleOffsetValid && m_LastSampleOffset < -10000000) { // Only starve if we are 1 seconds behind
						if (m_nRenderState == Started && !g_bNoDuration) {
							m_pSink->Notify(EC_STARVATION, 0, 0);
							m_bSignaledStarvation = true;
						}
					}
					//GetImageFromMixer();
				}
				//else
				//{
				//	TRACE_EVR("EVR: RenderThread ==>> Flush before rendering frame!\n");
				//}

				break;
		}
	}

	timeEndPeriod(dwResolution);
	if (pfAvRevertMmThreadCharacteristics) {
		pfAvRevertMmThreadCharacteristics(hAvrt);
	}
}

void CEVRAllocatorPresenter::VSyncThread()
{
	bool		bQuit	= false;
	TIMECAPS	tc;
	DWORD		dwResolution;

	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	dwResolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
	timeBeginPeriod(dwResolution);

	CRenderersSettings& rs	= GetRenderersSettings();

	while (!bQuit) {
		DWORD dwObject = WaitForSingleObject(m_hEvtQuit, 1);
		switch (dwObject) {
			case WAIT_OBJECT_0 :
				bQuit = true;
				break;
			case WAIT_TIMEOUT : {
				// Do our stuff
				if (m_pD3DDev && rs.m_AdvRendSets.bVSync) {
					if (m_nRenderState == Started) {
						int VSyncPos = GetVBlackPos();
						int WaitRange = max(m_ScreenSize.cy / 40, 5);
						int MinRange = max(min(int(0.003 * double(m_ScreenSize.cy) * double(m_refreshRate) + 0.5), m_ScreenSize.cy / 3), 5); // 1.8  ms or max 33 % of Time

						VSyncPos += MinRange + WaitRange;

						VSyncPos = VSyncPos % m_ScreenSize.cy;
						if (VSyncPos < 0) {
							VSyncPos += m_ScreenSize.cy;
						}

						int ScanLine = (VSyncPos + 1) % m_ScreenSize.cy;
						if (ScanLine < 0) {
							ScanLine += m_ScreenSize.cy;
						}
						int ScanLineMiddle = ScanLine + m_ScreenSize.cy / 2;
						ScanLineMiddle = ScanLineMiddle % m_ScreenSize.cy;
						if (ScanLineMiddle < 0) {
							ScanLineMiddle += m_ScreenSize.cy;
						}

						int ScanlineStart = ScanLine;
						bool bTakenLock;
						WaitForVBlankRange(ScanlineStart, 5, true, true, false, bTakenLock);
						LONGLONG TimeStart = GetPerfCounter();

						WaitForVBlankRange(ScanLineMiddle, 5, true, true, false, bTakenLock);
						LONGLONG TimeMiddle = GetPerfCounter();

						int ScanlineEnd = ScanLine;
						WaitForVBlankRange(ScanlineEnd, 5, true, true, false, bTakenLock);
						LONGLONG TimeEnd = GetPerfCounter();

						double nSeconds = double(TimeEnd - TimeStart) / 10000000.0;
						LONGLONG DiffMiddle = TimeMiddle - TimeStart;
						LONGLONG DiffEnd = TimeEnd - TimeMiddle;
						double DiffDiff;
						if (DiffEnd > DiffMiddle) {
							DiffDiff = double(DiffEnd) / double(DiffMiddle);
						} else {
							DiffDiff = double(DiffMiddle) / double(DiffEnd);
						}
						if (nSeconds > 0.003 && DiffDiff < 1.3) {
							double ScanLineSeconds;
							double nScanLines;
							if (ScanLineMiddle > ScanlineEnd) {
								ScanLineSeconds = double(TimeMiddle - TimeStart) / 10000000.0;
								nScanLines = ScanLineMiddle - ScanlineStart;
							} else {
								ScanLineSeconds = double(TimeEnd - TimeMiddle) / 10000000.0;
								nScanLines = ScanlineEnd - ScanLineMiddle;
							}

							double ScanLineTime = ScanLineSeconds / nScanLines;

							int iPos = m_DetectedRefreshRatePos % 100;
							m_ldDetectedScanlineRateList[iPos] = ScanLineTime;
							if (m_DetectedScanlineTime && ScanlineStart != ScanlineEnd) {
								int Diff = ScanlineEnd - ScanlineStart;
								nSeconds -= double(Diff) * m_DetectedScanlineTime;
							}
							m_ldDetectedRefreshRateList[iPos] = nSeconds;
							double Average = 0;
							double AverageScanline = 0;
							int nPos = min(iPos + 1, 100);
							for (int i = 0; i < nPos; ++i) {
								Average += m_ldDetectedRefreshRateList[i];
								AverageScanline += m_ldDetectedScanlineRateList[i];
							}

							if (nPos) {
								Average /= double(nPos);
								AverageScanline /= double(nPos);
							} else {
								Average = 0;
								AverageScanline = 0;
							}

							double ThisValue = Average;

							if (Average > 0.0 && AverageScanline > 0.0) {
								CAutoLock Lock(&m_RefreshRateLock);
								++m_DetectedRefreshRatePos;
								if (m_DetectedRefreshTime == 0 || m_DetectedRefreshTime / ThisValue > 1.01 || m_DetectedRefreshTime / ThisValue < 0.99) {
									m_DetectedRefreshTime = ThisValue;
									m_DetectedRefreshTimePrim = 0;
								}
								if (_isnan(m_DetectedRefreshTime)) {
									m_DetectedRefreshTime = 0.0;
								}
								if (_isnan(m_DetectedRefreshTimePrim)) {
									m_DetectedRefreshTimePrim = 0.0;
								}

								ModerateFloat(m_DetectedRefreshTime, ThisValue, m_DetectedRefreshTimePrim, 1.5);
								if (m_DetectedRefreshTime > 0.0) {
									m_DetectedRefreshRate = 1.0/m_DetectedRefreshTime;
								} else {
									m_DetectedRefreshRate = 0.0;
								}

								if (m_DetectedScanlineTime == 0 || m_DetectedScanlineTime / AverageScanline > 1.01 || m_DetectedScanlineTime / AverageScanline < 0.99) {
									m_DetectedScanlineTime = AverageScanline;
									m_DetectedScanlineTimePrim = 0;
								}
								ModerateFloat(m_DetectedScanlineTime, AverageScanline, m_DetectedScanlineTimePrim, 1.5);
								if (m_DetectedScanlineTime > 0.0) {
									m_DetectedScanlinesPerFrame = m_DetectedRefreshTime / m_DetectedScanlineTime;
								} else {
									m_DetectedScanlinesPerFrame = 0;
								}
							}
						}
					}
				} else {
					m_DetectedRefreshRate = 0.0;
					m_DetectedScanlinesPerFrame = 0.0;
				}
			}
			break;
		}
	}

	timeEndPeriod(dwResolution);
}

DWORD WINAPI CEVRAllocatorPresenter::VSyncThreadStatic(LPVOID lpParam)
{
	SetThreadName(DWORD_MAX, "CEVRAllocatorPresenter::VSyncThread");
	CEVRAllocatorPresenter* pThis = (CEVRAllocatorPresenter*)lpParam;
	pThis->VSyncThread();
	return 0;
}

void CEVRAllocatorPresenter::OnResetDevice()
{
	CAutoLock cRenderLock(&m_RenderLock);

	// Reset DXVA Manager, and get new buffers
	HRESULT hr = m_pD3DManager->ResetDevice(m_pD3DDev, m_nResetToken);

	// Not necessary, but Microsoft documentation say Presenter should send this message...
	if (m_pSink) {
		EXECUTE_ASSERT(S_OK == (hr = m_pSink->Notify(EC_DISPLAY_CHANGED, 0, 0)));
	}
}

void CEVRAllocatorPresenter::RemoveAllSamples()
{
	CAutoLock AutoLock(&m_ImageProcessingLock);
	CAutoLock Lock(&m_csExternalMixerLock);

	FlushSamples();
	m_ScheduledSamples.RemoveAll();
	m_FreeSamples.RemoveAll();
	m_LastScheduledSampleTime = -1;
	m_LastScheduledUncorrectedSampleTime = -1;
	m_nUsedBuffer = 0;
}

HRESULT CEVRAllocatorPresenter::GetFreeSample(IMFSample** ppSample)
{
	CAutoLock lock(&m_SampleQueueLock);
	HRESULT hr = S_OK;

	if (m_FreeSamples.GetCount() > 1) {	// <= Cannot use first free buffer (can be currently displayed)
		InterlockedIncrement (&m_nUsedBuffer);
		*ppSample = m_FreeSamples.RemoveHead().Detach();
	} else {
		hr = MF_E_SAMPLEALLOCATOR_EMPTY;
	}

	return hr;
}

HRESULT CEVRAllocatorPresenter::GetScheduledSample(IMFSample** ppSample, int &_Count)
{
	CAutoLock lock(&m_SampleQueueLock);
	HRESULT hr = S_OK;

	_Count = (int)m_ScheduledSamples.GetCount();
	if (_Count > 0) {
		*ppSample = m_ScheduledSamples.RemoveHead().Detach();
		--_Count;
	} else {
		hr = MF_E_SAMPLEALLOCATOR_EMPTY;
	}

	return hr;
}

void CEVRAllocatorPresenter::MoveToFreeList(IMFSample* pSample, bool bTail)
{
	CAutoLock lock(&m_SampleQueueLock);
	InterlockedDecrement (&m_nUsedBuffer);
	if (m_bPendingMediaFinished && m_nUsedBuffer == 0) {
		m_bPendingMediaFinished = false;
		m_pSink->Notify(EC_COMPLETE, 0, 0);
	}
	if (bTail) {
		m_FreeSamples.AddTail (pSample);
	} else {
		m_FreeSamples.AddHead(pSample);
	}
}

void CEVRAllocatorPresenter::MoveToScheduledList(IMFSample* pSample, bool _bSorted)
{
	if (_bSorted) {
		CAutoLock lock(&m_SampleQueueLock);
		// Insert sorted
		/*POSITION Iterator = m_ScheduledSamples.GetHeadPosition();

		LONGLONG NewSampleTime;
		pSample->GetSampleTime(&NewSampleTime);

		while (Iterator != NULL)
		{
			POSITION CurrentPos = Iterator;
			IMFSample *pIter = m_ScheduledSamples.GetNext(Iterator);
			LONGLONG SampleTime;
			pIter->GetSampleTime(&SampleTime);
			if (NewSampleTime < SampleTime)
			{
				m_ScheduledSamples.InsertBefore(CurrentPos, pSample);
				return;
			}
		}*/

		m_ScheduledSamples.AddHead(pSample);
	} else {

		CAutoLock lock(&m_SampleQueueLock);

		CRenderersSettings& rs = GetRenderersSettings();
		/*double ForceFPS = 0.0;
		//double ForceFPS = 59.94;
		//double ForceFPS = 23.976;
		if (ForceFPS != 0.0) {
			m_rtTimePerFrame = (REFERENCE_TIME)(10000000.0 / ForceFPS);
		}*/
		LONGLONG Duration = m_rtTimePerFrame;
		UNREFERENCED_PARAMETER(Duration);
		LONGLONG PrevTime = m_LastScheduledUncorrectedSampleTime;
		LONGLONG Time;
		LONGLONG SetDuration;
		pSample->GetSampleDuration(&SetDuration);
		pSample->GetSampleTime(&Time);
		m_LastScheduledUncorrectedSampleTime = Time;

		m_bCorrectedFrameTime = false;

		LONGLONG Diff2 = PrevTime - (LONGLONG)(m_LastScheduledSampleTimeFP * 10000000.0);
		LONGLONG Diff = Time - PrevTime;
		if (PrevTime == -1) {
			Diff = 0;
		}
		if (Diff < 0) {
			Diff = -Diff;
		}
		if (Diff2 < 0) {
			Diff2 = -Diff2;
		}
		if (Diff < m_rtTimePerFrame*8 && m_rtTimePerFrame && Diff2 < m_rtTimePerFrame*8) { // Detect seeking
			int iPos = (m_DetectedFrameTimePos++) % 60;
			LONGLONG Diff = Time - PrevTime;
			if (PrevTime == -1) {
				Diff = 0;
			}
			m_DetectedFrameTimeHistory[iPos] = Diff;

			if (m_DetectedFrameTimePos >= 10) {
				int nFrames = min(m_DetectedFrameTimePos, 60);
				LONGLONG DectedSum = 0;
				for (int i = 0; i < nFrames; ++i) {
					DectedSum += m_DetectedFrameTimeHistory[i];
				}

				double Average = double(DectedSum) / double(nFrames);
				double DeviationSum = 0.0;
				for (int i = 0; i < nFrames; ++i) {
					double Deviation = m_DetectedFrameTimeHistory[i] - Average;
					DeviationSum += Deviation*Deviation;
				}

				double StdDev = sqrt(DeviationSum/double(nFrames));

				m_DetectedFrameTimeStdDev = StdDev;

				double DetectedRate = 1.0/ (double(DectedSum) / (nFrames * 10000000.0) );

				double AllowedError = 0.0003;

				static double AllowedValues[] = {/*75, 72/1.001,*/ 60.0, 60/1.001, 50.0, 48.0, 48/1.001, 30.0, 30/1.001, 25.0, 24.0, 24/1.001};

				int nAllowed = _countof(AllowedValues);
				for (int i = 0; i < nAllowed; ++i) {
					if (fabs(1.0 - DetectedRate / AllowedValues[i]) < AllowedError) {
						DetectedRate = AllowedValues[i];
						break;
					}
				}

				m_DetectedFrameTimeHistoryHistory[m_DetectedFrameTimePos % 500] = DetectedRate;

				class CAutoInt
				{
				public:

					int m_Int;

					CAutoInt() {
						m_Int = 0;
					}
					CAutoInt(int _Other) {
						m_Int = _Other;
					}

					operator int () const {
						return m_Int;
					}

					CAutoInt &operator ++ () {
						++m_Int;
						return *this;
					}
				};


				CMap<double, double, CAutoInt, CAutoInt> Map;

				for (int i = 0; i < 500; ++i) {
					++Map[m_DetectedFrameTimeHistoryHistory[i]];
				}

				POSITION Pos = Map.GetStartPosition();
				double BestVal = 0.0;
				int BestNum = 5;
				while (Pos) {
					double Key;
					CAutoInt Value;
					Map.GetNextAssoc(Pos, Key, Value);
					if (Value.m_Int > BestNum && Key != 0.0) {
						BestNum = Value.m_Int;
						BestVal = Key;
					}
				}

				m_DetectedLock = false;
				for (int i = 0; i < nAllowed; ++i) {
					if (BestVal == AllowedValues[i]) {
						m_DetectedLock = true;
						break;
					}
				}
				if (BestVal != 0.0) {
					m_DetectedFrameRate = BestVal;
					m_DetectedFrameTime = 1.0 / BestVal;
				}
			}

			LONGLONG PredictedNext = PrevTime + m_rtTimePerFrame;
			LONGLONG PredictedDiff = PredictedNext - Time;
			if (PredictedDiff < 0) {
				PredictedDiff = -PredictedDiff;
			}

			if (m_DetectedFrameTime != 0.0
					//&& PredictedDiff > MIN_FRAME_TIME
					&& m_DetectedLock && rs.m_AdvRendSets.bEVRFrameTimeCorrection) {
				double CurrentTime = Time / 10000000.0;
				double LastTime = m_LastScheduledSampleTimeFP;
				double PredictedTime = LastTime + m_DetectedFrameTime;
				if (fabs(PredictedTime - CurrentTime) > 0.0015) { // 1.5 ms wrong, lets correct
					CurrentTime = PredictedTime;
					Time = (LONGLONG)(CurrentTime * 10000000.0);
					pSample->SetSampleTime(Time);
					pSample->SetSampleDuration(LONGLONG(m_DetectedFrameTime * 10000000.0));
					m_bCorrectedFrameTime = true;
					m_FrameTimeCorrection = 30;
				}
				m_LastScheduledSampleTimeFP = CurrentTime;
			} else {
				m_LastScheduledSampleTimeFP = Time / 10000000.0;
			}
		} else {
			m_LastScheduledSampleTimeFP = Time / 10000000.0;
			if (Diff > m_rtTimePerFrame*8) {
				// Seek
				m_bSignaledStarvation = false;
				m_DetectedFrameTimePos = 0;
				m_DetectedLock = false;
			}
		}

		//TRACE_EVR("EVR: Time: %f %f %f\n", Time / 10000000.0, SetDuration / 10000000.0, m_DetectedFrameRate);
		if (!m_bCorrectedFrameTime && m_FrameTimeCorrection) {
			--m_FrameTimeCorrection;
		}

#if 0
		if (Time <= m_LastScheduledUncorrectedSampleTime && m_LastScheduledSampleTime >= 0) {
			PrevTime = m_LastScheduledSampleTime;
		}

		m_bCorrectedFrameTime = false;
		if (PrevTime != -1 && (Time >= PrevTime - ((Duration*20)/9) || Time == 0) || ForceFPS != 0.0) {
			if (Time - PrevTime > ((Duration*20)/9) && Time - PrevTime < Duration * 8 || Time == 0 || ((Time - PrevTime) < (Duration / 11)) || ForceFPS != 0.0) {
				// Error!!!!
				Time = PrevTime + Duration;
				pSample->SetSampleTime(Time);
				pSample->SetSampleDuration(Duration);
				m_bCorrectedFrameTime = true;
				TRACE_EVR("EVR: Corrected invalid sample time\n");
			}
		}
		if (Time+Duration*10 < m_LastScheduledSampleTime) {
			// Flush when repeating movie
			FlushSamplesInternal();
		}
#endif

#if 0
		static LONGLONG LastDuration = 0;
		LONGLONG SetDuration = m_rtTimePerFrame;
		pSample->GetSampleDuration(&SetDuration);
		if (SetDuration != LastDuration) {
			TRACE_EVR("EVR: Old duration: %I64d New duration: %I64d\n", LastDuration, SetDuration);
		}
		LastDuration = SetDuration;
#endif
		m_LastScheduledSampleTime = Time;

		m_ScheduledSamples.AddTail(pSample);
	}
}

void CEVRAllocatorPresenter::FlushSamples()
{
	CAutoLock lock(this);
	CAutoLock lock2(&m_SampleQueueLock);

	FlushSamplesInternal();
	m_LastScheduledSampleTime = -1;
}

void CEVRAllocatorPresenter::FlushSamplesInternal()
{
	CAutoLock Lock(&m_csExternalMixerLock);

	while (!m_ScheduledSamples.IsEmpty()) {
		CComPtr<IMFSample> pMFSample;

		pMFSample = m_ScheduledSamples.RemoveHead();
		MoveToFreeList (pMFSample, true);
	}

	m_LastSampleOffset			= 0;
	m_bLastSampleOffsetValid	= false;
	m_bSignaledStarvation		= false;
}
