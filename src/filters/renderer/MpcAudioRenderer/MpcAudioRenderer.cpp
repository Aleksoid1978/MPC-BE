/*
 * (C) 2009-2018 see Authors.txt
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
#include <map>

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>
#include <math.h>
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/AudioParser.h"
#include "../../../DSUtil/AudioTools.h"
#include "../../../DSUtil/SysVersion.h"
#include "AudioHelper.h"
#include "AudioDevice.h"
#include "MpcAudioRenderer.h"

// option names
#define OPT_REGKEY_AudRend          L"Software\\MPC-BE Filters\\MPC Audio Renderer"
#define OPT_SECTION_AudRend         L"Filters\\MPC Audio Renderer"
#define OPT_DeviceMode              L"UseWasapi"
#define OPT_AudioDeviceId           L"SoundDeviceId"
#define OPT_UseBitExactOutput       L"UseBitExactOutput"
#define OPT_UseSystemLayoutChannels L"UseSystemLayoutChannels"
#define OPT_ReleaseDeviceIdle       L"ReleaseDeviceIdle"
#define OPT_UseCrossFeed            L"CrossFeed"
// TODO: rename option values

// set to 1(or more) to enable more detail debug log
#define DBGLOG_LEVEL 1

#ifdef REGISTER_FILTER

#include "../../filters/ffmpeg_fix.cpp"

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_IEEE_FLOAT}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMpcAudioRenderer), MpcAudioRendererName, /*0x40000001*/MERIT_UNLIKELY + 1, _countof(sudpPins), sudpPins, CLSID_AudioRendererCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, &__uuidof(CMpcAudioRenderer), CreateInstance<CMpcAudioRenderer>, nullptr, &sudFilter[0]},
	{L"CMpcAudioRendererPropertyPage", &__uuidof(CMpcAudioRendererSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CMpcAudioRendererSettingsWnd> >},
	{L"CMpcAudioRendererPropertyPageStatus", &__uuidof(CMpcAudioRendererStatusWnd), CreateInstance<CInternalPropertyPageTempl<CMpcAudioRendererStatusWnd> >},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

static void DumpWaveFormatEx(const WAVEFORMATEX* pwfx)
{
	DLog(L"        => wFormatTag      = 0x%04x", pwfx->wFormatTag);
	DLog(L"        => nChannels       = %d", pwfx->nChannels);
	DLog(L"        => nSamplesPerSec  = %d", pwfx->nSamplesPerSec);
	DLog(L"        => nAvgBytesPerSec = %d", pwfx->nAvgBytesPerSec);
	DLog(L"        => nBlockAlign     = %d", pwfx->nBlockAlign);
	DLog(L"        => wBitsPerSample  = %d", pwfx->wBitsPerSample);
	DLog(L"        => cbSize          = %d", pwfx->cbSize);
	if (IsWaveFormatExtensible(pwfx)) {
		WAVEFORMATEXTENSIBLE* wfe = (WAVEFORMATEXTENSIBLE*)pwfx;
		DLog(L"        WAVEFORMATEXTENSIBLE:");
		DLog(L"            => wValidBitsPerSample = %d", wfe->Samples.wValidBitsPerSample);
		DLog(L"            => dwChannelMask       = 0x%x", wfe->dwChannelMask);
		DLog(L"            => SubFormat           = %s", CStringFromGUID(wfe->SubFormat));
	}
}

#define SamplesToTime(samples, wfex) (FractionScale64(samples, UNITS, wfex->nSamplesPerSec))
#define TimeToSamples(time, wfex)    (FractionScale64(time, wfex->nSamplesPerSec, UNITS))

#define IsExclusive(wfex)            (m_WASAPIMode == MODE_WASAPI_EXCLUSIVE || IsBitstream(wfex))
#define IsExclusiveMode()            (m_WASAPIMode == MODE_WASAPI_EXCLUSIVE || m_bIsBitstream)

CMpcAudioRenderer::CMpcAudioRenderer(LPUNKNOWN punk, HRESULT *phr)
	: CBaseRenderer(__uuidof(this), L"CMpcAudioRenderer", punk, phr)
	, m_dRate(1.0)
	, m_pSyncClock(nullptr)
	, m_pWaveFormatExInput(nullptr)
	, m_pWaveFormatExOutput(nullptr)
	, m_pMMDevice(nullptr)
	, m_pAudioClient(nullptr)
	, m_pRenderClient(nullptr)
	, m_pAudioClock(nullptr)
	, m_WASAPIMode(MODE_WASAPI_EXCLUSIVE)
	, m_nFramesInBuffer(0)
	, m_nMaxWasapiQueueSize(0)
	, m_hnsPeriod(0)
	, m_bIsAudioClientStarted(false)
	, m_lVolume(DSBVOLUME_MAX)
	, m_lBalance(DSBPAN_CENTER)
	, m_dVolumeFactor(1.0)
	, m_dBalanceFactor(1.0)
	, m_dwBalanceMask(0)
	, m_bUpdateBalanceMask(true)
	, m_nThreadId(0)
	, m_hRenderThread(nullptr)
	, m_bThreadPaused(FALSE)
	, m_hDataEvent(nullptr)
	, m_hPauseEvent(nullptr)
	, m_hWaitPauseEvent(nullptr)
	, m_hResumeEvent(nullptr)
	, m_hWaitResumeEvent(nullptr)
	, m_hStopRenderThreadEvent(nullptr)
	, m_bIsBitstream(FALSE)
	, m_BitstreamMode(BITSTREAM_NONE)
	, m_hModule(nullptr)
	, pfAvSetMmThreadCharacteristicsW(nullptr)
	, pfAvRevertMmThreadCharacteristics(nullptr)
	, m_bUseBitExactOutput(TRUE)
	, m_bUseSystemLayoutChannels(TRUE)
	, m_bReleaseDeviceIdle(FALSE)
	, m_filterState(State_Stopped)
	, m_hRendererNeedMoreData(nullptr)
	, m_CurrentPacket(nullptr)
	, m_rtStartTime(0)
	, m_rtNextSampleTime(0)
	, m_rtLastSampleTimeEnd(0)
	, m_rtEstimateSlavingJitter(0)
	, m_bUseDefaultDevice(FALSE)
	, m_nSampleOffset(0)
	, m_bUseCrossFeed(FALSE)
	, m_bNeedReinitialize(FALSE)
	, m_bNeedReinitializeFull(FALSE)
	, m_FlushEvent(TRUE)
	, m_ReceiveEvent(TRUE)
	, m_bReal32bitSupport(FALSE)
	, m_bDVDPlayback(FALSE)
{
	DLog(L"CMpcAudioRenderer::CMpcAudioRenderer()");

#ifdef REGISTER_FILTER
	CRegKey key;
	WCHAR buff[256];
	ULONG len;

	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_AudRend, KEY_READ)) {
		DWORD dw;
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_DeviceMode, dw)) {
			m_WASAPIMode = (WASAPI_MODE)dw;
		}
		len = _countof(buff);
		memset(buff, 0, sizeof(buff));
		if (ERROR_SUCCESS == key.QueryStringValue(OPT_AudioDeviceId, buff, &len)) {
			m_DeviceId = CString(buff);
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_UseBitExactOutput, dw)) {
			m_bUseBitExactOutput = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_UseSystemLayoutChannels, dw)) {
			m_bUseSystemLayoutChannels = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_ReleaseDeviceIdle, dw)) {
			m_bReleaseDeviceIdle = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_UseCrossFeed, dw)) {
			m_bUseCrossFeed = !!dw;
		}
	}
#else
	m_WASAPIMode               = (WASAPI_MODE)AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_DeviceMode, m_WASAPIMode);
	m_DeviceId                 = AfxGetApp()->GetProfileString(OPT_SECTION_AudRend, OPT_AudioDeviceId, m_DeviceId);
	m_bUseBitExactOutput       = !!AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_UseBitExactOutput, m_bUseBitExactOutput);
	m_bUseSystemLayoutChannels = !!AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_UseSystemLayoutChannels, m_bUseSystemLayoutChannels);
	m_bReleaseDeviceIdle       = !!AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_ReleaseDeviceIdle, m_bReleaseDeviceIdle);
	m_bUseCrossFeed            = !!AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_UseCrossFeed, m_bUseCrossFeed);
#endif

	m_WASAPIMode = std::clamp(m_WASAPIMode, MODE_WASAPI_EXCLUSIVE, MODE_WASAPI_SHARED);

	if (phr) {
		*phr = E_FAIL;
	}

	m_hModule = LoadLibraryW(L"AVRT.dll");
	if (m_hModule) {
		pfAvSetMmThreadCharacteristicsW   = (PTR_AvSetMmThreadCharacteristicsW)GetProcAddress(m_hModule, "AvSetMmThreadCharacteristicsW");
		pfAvRevertMmThreadCharacteristics = (PTR_AvRevertMmThreadCharacteristics)GetProcAddress(m_hModule, "AvRevertMmThreadCharacteristics");
	} else {
		return;
	}

	m_hDataEvent             = CreateEventW(nullptr, FALSE, FALSE, nullptr);

	m_hPauseEvent            = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	m_hWaitPauseEvent        = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	m_hResumeEvent           = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	m_hWaitResumeEvent       = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	m_hStopRenderThreadEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

	m_hRendererNeedMoreData  = CreateEventW(nullptr, TRUE, FALSE, nullptr);

	HRESULT hr = S_OK;
	m_pInputPin = DNew CMpcAudioRendererInputPin(this, &hr);
	if (!m_pInputPin) {
		hr = E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		if (phr) {
			*phr = hr;
		}
		return;
	}

	// CBaseRenderer is using a lazy initialization for the CRendererPosPassThru - we need it always
	m_pPosition = DNew CRendererPosPassThru(L"CRendererPosPassThru", GetOwner(), &hr, m_pInputPin);
	if (m_pPosition == nullptr) {
		hr = E_OUTOFMEMORY;
	} else if (FAILED(hr)) {
		delete m_pPosition;
		m_pPosition = nullptr;
		hr = E_NOINTERFACE;
	}
	if (FAILED(hr)) {
		if (phr) {
			*phr = hr;
		}
		return;
	}

	m_pSyncClock = DNew CAudioSyncClock(GetOwner(), &hr);
	if (phr) {
		*phr = hr;
	}

	if (SUCCEEDED(hr)) {
		CComPtr<IMMDeviceEnumerator> enumerator;
		if (SUCCEEDED(enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator)))) {
			enumerator->RegisterEndpointNotificationCallback(this);
		}
	}
}

CMpcAudioRenderer::~CMpcAudioRenderer()
{
	DLog(L"CMpcAudioRenderer::~CMpcAudioRenderer()");

	CComPtr<IMMDeviceEnumerator> enumerator;
	if (SUCCEEDED(enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator)))) {
		enumerator->UnregisterEndpointNotificationCallback(this);
	}

	EndReleaseTimer();

	m_pSyncClock->UnSlave();
	SAFE_DELETE(m_pSyncClock);

	StopRendererThread();
	if (m_bIsAudioClientStarted && m_pAudioClient) {
		m_pAudioClient->Stop();
	}

	WasapiFlush();

	SAFE_CLOSE_HANDLE(m_hStopRenderThreadEvent);
	SAFE_CLOSE_HANDLE(m_hDataEvent);
	SAFE_CLOSE_HANDLE(m_hPauseEvent);
	SAFE_CLOSE_HANDLE(m_hWaitPauseEvent);
	SAFE_CLOSE_HANDLE(m_hResumeEvent);
	SAFE_CLOSE_HANDLE(m_hWaitResumeEvent);

	SAFE_CLOSE_HANDLE(m_hRendererNeedMoreData);

	SAFE_RELEASE(m_pRenderClient);
	SAFE_RELEASE(m_pAudioClock);
	SAFE_RELEASE(m_pAudioClient);
	SAFE_RELEASE(m_pMMDevice);

	SAFE_DELETE_ARRAY(m_pWaveFormatExInput);
	SAFE_DELETE_ARRAY(m_pWaveFormatExOutput);

	if (m_hModule) {
		FreeLibrary(m_hModule);
	}
}

HRESULT CMpcAudioRenderer::CheckInputType(const CMediaType *pmt)
{
	return CheckMediaType(pmt);
}

HRESULT	CMpcAudioRenderer::CheckMediaType(const CMediaType *pmt)
{
	CAutoLock cAutoLock(&m_csCheck);

	CheckPointer(pmt, E_POINTER);
	CheckPointer(pmt->pbFormat, VFW_E_TYPE_NOT_ACCEPTED);

	if ((pmt->majortype != MEDIATYPE_Audio) || (pmt->formattype != FORMAT_WaveFormatEx)) {
		DLog(L"CMpcAudioRenderer::CheckMediaType() - Not supported");
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if (pmt->subtype != MEDIASUBTYPE_PCM && pmt->subtype != MEDIASUBTYPE_IEEE_FLOAT) {
		DLog(L"CMpcAudioRenderer::CheckMediaType() - allow only PCM/IEEE_FLOAT input");
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	HRESULT hr = S_OK;
	if (pmt->subtype == MEDIASUBTYPE_PCM) {
		// Check S/PDIF & HDMI Bitstream
		const WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX*)pmt->pbFormat;
		const BOOL bIsBitstreamInput = IsBitstream(pWaveFormatEx);
		if (bIsBitstreamInput) {
			hr = CreateAudioClient();
			if (FAILED(hr)) {
				DLog(L"CMpcAudioRenderer::CheckMediaType() - audio client initialization error");
				return VFW_E_CANNOT_CONNECT;
			}

			hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pWaveFormatEx, nullptr);
		}
	}

	return hr;
}

HRESULT CMpcAudioRenderer::Receive(IMediaSample* pSample)
{
	auto ReceiveInternal = [&](IMediaSample* pSample) {
		ASSERT(pSample);

		if (m_FlushEvent.Check()) {
			return S_OK;
		}

		m_ReceiveEvent.Set();

		// It may return VFW_E_SAMPLE_REJECTED code to say don't bother
		HRESULT hr = PrepareReceive(pSample);
		ASSERT(m_bInReceive == SUCCEEDED(hr));
		if (FAILED(hr)) {
			if (hr == VFW_E_SAMPLE_REJECTED) {
				hr = S_OK;
			}
			return hr;
		}

		if (m_State == State_Paused) {
			{
				CAutoLock cRendererLock(&m_InterfaceLock);
				if (m_State == State_Stopped) {
					m_bInReceive = FALSE;
					return S_OK;
				}
			}
			Ready();
		}

		Transform(pSample);

		m_bInReceive = FALSE;

		CAutoLock cRendererLock(&m_InterfaceLock);
		if (m_State == State_Stopped) {
			return S_OK;
		}

		CAutoLock cSampleLock(&m_RendererLock);

		ClearPendingSample();
		SendEndOfStream();
		CancelNotification();

		return S_OK;

	};

	HRESULT hr = ReceiveInternal(pSample);
	m_ReceiveEvent.Reset();

	return hr;
}

STDMETHODIMP CMpcAudioRenderer::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IReferenceClock) {
		return m_pSyncClock->NonDelegatingQueryInterface(riid, ppv);
	} else if (riid == IID_IDispatch) {
		return GetInterface(static_cast<IDispatch*>(this), ppv);
	} else if (riid == IID_IBasicAudio) {
		return GetInterface(static_cast<IBasicAudio*>(this), ppv);
	} else if (riid == IID_IMediaSeeking) {
		return GetInterface(static_cast<IMediaSeeking*>(this), ppv);
	} else if (riid == __uuidof(IMMNotificationClient)) {
		return GetInterface(static_cast<IMMNotificationClient*>(this), ppv);
	} else if (riid == __uuidof(IAMStreamSelect)) {
		return GetInterface(static_cast<IAMStreamSelect*>(this), ppv);
	} else if (riid == __uuidof(ISpecifyPropertyPages)) {
		return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
	} else if (riid == __uuidof(ISpecifyPropertyPages2)) {
		return GetInterface(static_cast<ISpecifyPropertyPages2*>(this), ppv);
	} else if (riid == __uuidof(IMpcAudioRendererFilter)) {
		return GetInterface(static_cast<IMpcAudioRendererFilter*>(this), ppv);
	}

	return CBaseRenderer::NonDelegatingQueryInterface (riid, ppv);
}

HRESULT CMpcAudioRenderer::SetMediaType(const CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);

	WAVEFORMATEX *pwf = (WAVEFORMATEX *)pmt->Format();
	CheckPointer(pwf, VFW_E_TYPE_NOT_ACCEPTED);

	DLog(L"CMpcAudioRenderer::SetMediaType()");

	HRESULT hr = S_OK;

	if (!m_pAudioClient) {
		hr = CreateAudioClient();
		if (FAILED(hr)) {
			DLog(L"CMpcAudioRenderer::SetMediaType() - audio client initialization error");
			return VFW_E_CANNOT_CONNECT;
		}
	}

	if (m_pRenderClient) {
		DLog(L"CMpcAudioRenderer::SetMediaType() : Render client already initialized. Reinitialization...");
	} else {
		DLog(L"CMpcAudioRenderer::SetMediaType() : Render client initialization");
	}

	hr = CheckAudioClient(pwf);

	EXIT_ON_ERROR(hr);

	CopyWaveFormat(pwf, &m_pWaveFormatExInput);

	return CBaseRenderer::SetMediaType(pmt);
}

HRESULT CMpcAudioRenderer::CompleteConnect(IPin *pReceivePin)
{
	DLog(L"CMpcAudioRenderer::CompleteConnect()");

	m_bDVDPlayback = FindFilter(CLSID_DVDNavigator, m_pGraph) != nullptr;

	return CBaseRenderer::CompleteConnect(pReceivePin);
}

DWORD WINAPI CMpcAudioRenderer::RenderThreadEntryPoint(LPVOID lpParameter)
{
	return ((CMpcAudioRenderer*)lpParameter)->RenderThread();
}

HRESULT CMpcAudioRenderer::StopRendererThread()
{
	if (m_hRenderThread) {
		SetEvent(m_hStopRenderThreadEvent);
		if (WaitForSingleObject(m_hRenderThread, 1000) == WAIT_TIMEOUT) {
			TerminateThread(m_hRenderThread, 0xDEAD);
		}

		CloseHandle(m_hRenderThread);
		m_hRenderThread = nullptr;
	}

	return S_OK;
}

HRESULT CMpcAudioRenderer::StartRendererThread()
{
	if (!m_hRenderThread) {
		m_hRenderThread = ::CreateThread(nullptr, 0, RenderThreadEntryPoint, (LPVOID)this, 0, &m_nThreadId);
		return m_hRenderThread ? S_OK : E_FAIL;
	} else if (m_bThreadPaused) {
		SetEvent(m_hResumeEvent);
		WaitForSingleObject(m_hWaitResumeEvent, INFINITE);
	}

	return S_OK;
}

HRESULT CMpcAudioRenderer::PauseRendererThread()
{
	if (m_bThreadPaused) {
		return S_OK;
	}

	// Pause the render thread
	if (m_hRenderThread) {
		SetEvent(m_hPauseEvent);
		WaitForSingleObject(m_hWaitPauseEvent, INFINITE);
		return S_OK;
	}

	return E_FAIL;
}

DWORD CMpcAudioRenderer::RenderThread()
{
	DLog(L"CMpcAudioRenderer::RenderThread() - start, thread ID = 0x%x", m_nThreadId);

	HRESULT hr = S_OK;

	// These are wait handles for the thread stopping, pausing redering and new sample arrival
	HANDLE renderHandles[3] = {
		m_hStopRenderThreadEvent,
		m_hPauseEvent,
		m_hDataEvent
	};

	// These are wait handles for resuming or exiting the thread
	HANDLE resumeHandles[2] = {
		m_hStopRenderThreadEvent,
		m_hResumeEvent
	};

	HANDLE hTaskHandle = nullptr;
	if (pfAvSetMmThreadCharacteristicsW && pfAvRevertMmThreadCharacteristics) {
		// Ask MMCSS to temporarily boost the thread priority
		// to reduce glitches while the low-latency stream plays.
		DWORD taskIndex = 0;

		hTaskHandle = pfAvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);
		if (hTaskHandle != nullptr) {
			DLog(L"CMpcAudioRenderer::EnableMMCSS() - Putting thread 0x%x in higher priority for Wasapi mode (lowest latency)", ::GetCurrentThreadId());
		}
	}
	if (hTaskHandle == nullptr) {
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	}

	bool bExit = false;
	while (!bExit) {
		CheckBufferStatus();

		const DWORD result = WaitForMultipleObjects(3, renderHandles, FALSE, 1000);
		switch (result) {
			case WAIT_TIMEOUT: // timed out after a 1 second wait
				if (m_pRenderClient) {
					SetReinitializeAudioDevice(TRUE);
				}
				break;
			case WAIT_OBJECT_0: // exit event
				DLog(L"CMpcAudioRenderer::RenderThread() - exit events");
				bExit = true;
				break;
			case WAIT_OBJECT_0 + 1: // pause event
				{
					DLog(L"CMpcAudioRenderer::RenderThread() - pause events");

					m_bThreadPaused = TRUE;
					ResetEvent(m_hResumeEvent);
					SetEvent(m_hWaitPauseEvent);

					const DWORD resultResume = WaitForMultipleObjects(2, resumeHandles, FALSE, INFINITE);
					switch (resultResume) {
						case WAIT_OBJECT_0: // exit event
							DLog(L"CMpcAudioRenderer::RenderThread() - exit events");
							bExit = true;
							break;
						case WAIT_OBJECT_0 + 1: // resume event
							DLog(L"CMpcAudioRenderer::RenderThread() - resume events");
							m_bThreadPaused = FALSE;
							SetEvent(m_hWaitResumeEvent);
					}
				}
				break;
			case WAIT_OBJECT_0 + 2: // render event
				{
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 1
					DLog(L"CMpcAudioRenderer::RenderThread() - Data Event, Audio client state = %s", m_bIsAudioClientStarted ? L"Started" : L"Stoped");
#endif
					HRESULT hr = RenderWasapiBuffer();
					if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
						SetReinitializeAudioDevice(TRUE);
					}
				}
				break;
			default:
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 1
				DLog(L"CMpcAudioRenderer::RenderThread() - WaitForMultipleObjects() failed: %u", GetLastError());
#endif
				break;
		}
	}

	if (hTaskHandle != nullptr) {
		pfAvRevertMmThreadCharacteristics(hTaskHandle);
	}

	DLog(L"CMpcAudioRenderer::RenderThread() - close, thread ID = 0x%x", m_nThreadId);

	return 0;
}

STDMETHODIMP CMpcAudioRenderer::Run(REFERENCE_TIME rtStart)
{
	DLog(L"CMpcAudioRenderer::Run()");

	if (m_State == State_Running) {
		return NOERROR;
	}

	m_filterState = State_Running;
	m_rtStartTime = rtStart;

	if (m_bEOS) {
		NotifyEvent(EC_COMPLETE, S_OK, 0);
	}

	EndReleaseTimer();

	if (m_hRendererNeedMoreData) {
		SetEvent(m_hRendererNeedMoreData);
	}

	if (m_pAudioClock) {
		m_pSyncClock->Slave(m_pAudioClock, m_rtStartTime + m_rtNextSampleTime);
	}

	return CBaseRenderer::Run(rtStart);
}

STDMETHODIMP CMpcAudioRenderer::Stop()
{
	DLog(L"CMpcAudioRenderer::Stop()");

	m_pSyncClock->UnSlave();

	m_filterState = State_Stopped;
	if (m_hRendererNeedMoreData) {
		SetEvent(m_hRendererNeedMoreData);
	}

	StartReleaseTimer();

	return CBaseRenderer::Stop();
};

STDMETHODIMP CMpcAudioRenderer::Pause()
{
	DLog(L"CMpcAudioRenderer::Pause()");

	m_pSyncClock->UnSlave();

	m_filterState = State_Paused;
	if (m_hRendererNeedMoreData) {
		SetEvent(m_hRendererNeedMoreData);
	}

	StartReleaseTimer();

	return CBaseRenderer::Pause();
};

// === IBasicAudio
STDMETHODIMP CMpcAudioRenderer::put_Volume(long lVolume)
{
	if (lVolume <= DSBVOLUME_MIN) {
		m_lVolume = DSBVOLUME_MIN;
		m_dVolumeFactor = 0.0;
	}
	else if (lVolume >= DSBVOLUME_MAX) {
		m_lVolume = DSBVOLUME_MAX;
		m_dVolumeFactor = 1.0;
	}
	else {
		m_lVolume = lVolume;
		m_dVolumeFactor = pow(10.0, m_lVolume / 2000.0);
	}

	if (m_lBalance > DSBPAN_LEFT && m_lBalance < DSBPAN_RIGHT) {
		m_dBalanceFactor = m_dVolumeFactor * pow(10.0, -labs(m_lBalance) / 2000.0);
	}

	m_bUpdateBalanceMask = true;

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::get_Volume(long *plVolume)
{
	*plVolume = m_lVolume;

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::put_Balance(long lBalance)
{
	if (lBalance <= DSBPAN_LEFT) {
		m_lBalance = DSBPAN_LEFT;
		m_dBalanceFactor = 0.0;
	}
	else if (lBalance >= DSBPAN_RIGHT) {
		m_lBalance = DSBPAN_RIGHT;
		m_dBalanceFactor = 0.0;
	}
	else {
		m_lBalance = lBalance;
		m_dBalanceFactor = m_dVolumeFactor * pow(10.0, -labs(m_lBalance) / 2000.0);
	}

	m_bUpdateBalanceMask = true;

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::get_Balance(long *plBalance)
{
	*plBalance = m_lBalance;

	return S_OK;
}

// === IMediaSeeking
STDMETHODIMP CMpcAudioRenderer::IsFormatSupported(const GUID* pFormat)
{
	return m_pPosition->IsFormatSupported(pFormat);
}

STDMETHODIMP CMpcAudioRenderer::QueryPreferredFormat(GUID* pFormat)
{
	return m_pPosition->QueryPreferredFormat(pFormat);
}

STDMETHODIMP CMpcAudioRenderer::SetTimeFormat(const GUID* pFormat)
{
	return m_pPosition->SetTimeFormat(pFormat);
}

STDMETHODIMP CMpcAudioRenderer::IsUsingTimeFormat(const GUID* pFormat)
{
	return m_pPosition->IsUsingTimeFormat(pFormat);
}

STDMETHODIMP CMpcAudioRenderer::GetTimeFormat(GUID* pFormat)
{
	return m_pPosition->GetTimeFormat(pFormat);
}

STDMETHODIMP CMpcAudioRenderer::GetDuration(LONGLONG* pDuration)
{
	return m_pPosition->GetDuration(pDuration);
}

STDMETHODIMP CMpcAudioRenderer::GetStopPosition(LONGLONG* pStop)
{
	return m_pPosition->GetStopPosition(pStop);
}

STDMETHODIMP CMpcAudioRenderer::GetCurrentPosition(LONGLONG* pCurrent)
{
	return m_pPosition->GetCurrentPosition(pCurrent);
}

STDMETHODIMP CMpcAudioRenderer::GetCapabilities(DWORD* pCapabilities)
{
	return m_pPosition->GetCapabilities(pCapabilities);
}

STDMETHODIMP CMpcAudioRenderer::CheckCapabilities(DWORD* pCapabilities)
{
	return m_pPosition->CheckCapabilities(pCapabilities);
}

STDMETHODIMP CMpcAudioRenderer::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return m_pPosition->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
}

STDMETHODIMP CMpcAudioRenderer::SetPositions(LONGLONG* pCurrent, DWORD CurrentFlags, LONGLONG* pStop, DWORD StopFlags)
{
	return m_pPosition->SetPositions(pCurrent, CurrentFlags, pStop, StopFlags);
}

STDMETHODIMP CMpcAudioRenderer::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	return m_pPosition->GetPositions(pCurrent, pStop);
}

STDMETHODIMP CMpcAudioRenderer::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	return m_pPosition->GetAvailable(pEarliest, pLatest);
}

STDMETHODIMP CMpcAudioRenderer::SetRate(double dRate)
{
	CAutoLock cInterfaceLock(&m_InterfaceLock);

	if (dRate < 0.25 || dRate > 4.0 || m_bIsBitstream) {
		return VFW_E_UNSUPPORTED_AUDIO;
	}

	m_dRate = dRate;
	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::GetRate(double* pdRate)
{
	return m_pPosition->GetRate(pdRate);
}

STDMETHODIMP CMpcAudioRenderer::GetPreroll(LONGLONG* pPreroll)
{
	return m_pPosition->GetPreroll(pPreroll);
}

// === IMMNotificationClient
STDMETHODIMP CMpcAudioRenderer::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
	CheckPointer(m_hRenderThread, S_OK);

	if (m_strCurrentDeviceId == pwstrDeviceId) {
		SetReinitializeAudioDevice(TRUE);
	}

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
{
	CheckPointer(m_hRenderThread, S_OK);

	if (m_bUseDefaultDevice
			&& flow == eRender
			&& role == eConsole) {
		SetReinitializeAudioDevice(TRUE);
	}

	return S_OK;
}

// === IAMStreamSelect
STDMETHODIMP CMpcAudioRenderer::Count(DWORD* pcStreams)
{
	CheckPointer(pcStreams, E_POINTER);

	return AudioDevices::GetActiveAudioDevices(NULL, (UINT*)pcStreams, FALSE);
}


STDMETHODIMP CMpcAudioRenderer::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	AudioDevices::devicesList devicesList;
	if (S_OK != AudioDevices::GetActiveAudioDevices(&devicesList, NULL, FALSE)
			|| !devicesList.size()) {
		return E_FAIL;
	}

	if (lIndex >= (long)devicesList.size()) {
		return S_FALSE;
	}

	const auto& [deviceName, deviceId] = devicesList[lIndex];

	if (ppmt) {
		*ppmt = nullptr;
	}
	if (plcid) {
		*plcid = 0;
	}
	if (pdwGroup) {
		*pdwGroup = 0;
	}
	if (ppObject) {
		*ppObject = nullptr;
	}
	if (ppUnk) {
		*ppUnk = nullptr;
	}

	if (pdwFlags) {
		if (deviceId == GetCurrentDeviceId()) {
			*pdwFlags = AMSTREAMSELECTINFO_ENABLED;
		} else {
			*pdwFlags = 0;
		}
	}

	if (ppszName) {
		*ppszName = (WCHAR*)CoTaskMemAlloc((deviceName.GetLength() + 1) * sizeof(WCHAR));
		if (*ppszName) {
			wcscpy_s(*ppszName, deviceName.GetLength() + 1, deviceName);
		}
	}

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::Enable(long lIndex, DWORD dwFlags)
{
	if (dwFlags != AMSTREAMSELECTENABLE_ENABLE) {
		return E_NOTIMPL;
	}

	AudioDevices::devicesList devicesList;
	if (S_OK != AudioDevices::GetActiveAudioDevices(&devicesList, NULL, FALSE)
			|| !devicesList.size()) {
		return E_FAIL;
	}

	if (lIndex >= (long)devicesList.size()) {
		return S_FALSE;
	}

	return SetDeviceId(devicesList[lIndex].second);
}


// === ISpecifyPropertyPages2
STDMETHODIMP CMpcAudioRenderer::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	BOOL bShowStatusPage = m_pInputPin && m_pInputPin->IsConnected();

	pPages->cElems = bShowStatusPage ? 2 : 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	CheckPointer(pPages->pElems, E_OUTOFMEMORY);

	pPages->pElems[0] = __uuidof(CMpcAudioRendererSettingsWnd);
	if (bShowStatusPage) {
		pPages->pElems[1] = __uuidof(CMpcAudioRendererStatusWnd);
	}

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != nullptr) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMpcAudioRendererSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpcAudioRendererSettingsWnd>(nullptr, &hr))->AddRef();
	} else if (guid == __uuidof(CMpcAudioRendererStatusWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpcAudioRendererStatusWnd>(nullptr, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// === IMpcAudioRendererFilter
STDMETHODIMP CMpcAudioRenderer::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_AudRend)) {
		key.SetDWORDValue(OPT_DeviceMode, (DWORD)m_WASAPIMode);
		key.SetStringValue(OPT_AudioDeviceId, m_DeviceId);
		key.SetDWORDValue(OPT_UseBitExactOutput, m_bUseBitExactOutput);
		key.SetDWORDValue(OPT_UseSystemLayoutChannels, m_bUseSystemLayoutChannels);
		key.SetDWORDValue(OPT_ReleaseDeviceIdle, m_bReleaseDeviceIdle);
		key.SetDWORDValue(OPT_UseCrossFeed, m_bUseCrossFeed);
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_DeviceMode, (int)m_WASAPIMode);
	AfxGetApp()->WriteProfileString(OPT_SECTION_AudRend, OPT_AudioDeviceId, m_DeviceId);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_UseBitExactOutput, m_bUseBitExactOutput);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_UseSystemLayoutChannels, m_bUseSystemLayoutChannels);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_ReleaseDeviceIdle, m_bReleaseDeviceIdle);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_UseCrossFeed, m_bUseCrossFeed);
#endif

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::SetWasapiMode(INT nValue)
{
	CAutoLock cAutoLock(&m_csProps);

	if (m_pAudioClient && m_WASAPIMode != nValue) {
		SetReinitializeAudioDevice();
	}

	m_WASAPIMode = (WASAPI_MODE)nValue;
	return S_OK;
}
STDMETHODIMP_(INT) CMpcAudioRenderer::GetWasapiMode()
{
	CAutoLock cAutoLock(&m_csProps);
	return (INT)m_WASAPIMode;
}

STDMETHODIMP CMpcAudioRenderer::SetDeviceId(CString pDeviceId)
{
	CAutoLock cAutoLock(&m_csProps);

	if (m_pAudioClient) {
		CString deviceIdSrc = m_strCurrentDeviceId;
		CString deviceIdDst = pDeviceId;

		if (deviceIdSrc != deviceIdDst
				&& (deviceIdSrc.IsEmpty() || deviceIdDst.IsEmpty())) {
			AudioDevices::device device;
			AudioDevices::GetDefaultAudioDevice(device);

			if (deviceIdSrc.IsEmpty()) {
				deviceIdSrc = device.second;
			}
			if (deviceIdDst.IsEmpty()) {
				deviceIdDst = device.second;
			}
		}

		if (deviceIdSrc != deviceIdDst) {
			SetReinitializeAudioDevice(TRUE);
		}
	}

	m_DeviceId = pDeviceId;
	return S_OK;
}

STDMETHODIMP_(CString) CMpcAudioRenderer::GetDeviceId()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_DeviceId;
}

STDMETHODIMP_(UINT) CMpcAudioRenderer::GetMode()
{
	CAutoLock cAutoLock(&m_csProps);

	if (!m_pGraph) {
		return MODE_NONE;
	}

	if (m_bIsBitstream) {
		return MODE_WASAPI_EXCLUSIVE_BITSTREAM;
	}

	return (UINT)m_WASAPIMode;
}

STDMETHODIMP CMpcAudioRenderer::GetStatus(WAVEFORMATEX** ppWfxIn, WAVEFORMATEX** ppWfxOut)
{
	CAutoLock cAutoLock(&m_csProps);

	*ppWfxIn  = m_pWaveFormatExInput;
	*ppWfxOut = m_pWaveFormatExOutput;

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::SetBitExactOutput(BOOL bValue)
{
	CAutoLock cAutoLock(&m_csProps);

	if (m_pAudioClient && m_bUseBitExactOutput != bValue) {
		SetReinitializeAudioDevice();
	}

	m_bUseBitExactOutput = bValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpcAudioRenderer::GetBitExactOutput()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bUseBitExactOutput;
}

STDMETHODIMP CMpcAudioRenderer::SetSystemLayoutChannels(BOOL bValue)
{
	CAutoLock cAutoLock(&m_csProps);

	if (m_pAudioClient && m_bUseSystemLayoutChannels != bValue) {
		SetReinitializeAudioDevice();
	}

	m_bUseSystemLayoutChannels = bValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpcAudioRenderer::GetSystemLayoutChannels()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bUseSystemLayoutChannels;
}

STDMETHODIMP CMpcAudioRenderer::SetReleaseDeviceIdle(BOOL bValue)
{
	CAutoLock cAutoLock(&m_csProps);

	m_bReleaseDeviceIdle = bValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpcAudioRenderer::GetReleaseDeviceIdle()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bReleaseDeviceIdle;
}

STDMETHODIMP_(BITSTREAM_MODE) CMpcAudioRenderer::GetBitstreamMode()
{
	CAutoLock cAutoLock(&m_csProps);
	return (m_pGraph && m_bIsBitstream) ? m_BitstreamMode : BITSTREAM_NONE;
}

STDMETHODIMP_(CString) CMpcAudioRenderer::GetCurrentDeviceName()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_strCurrentDeviceName;
}

STDMETHODIMP_(CString) CMpcAudioRenderer::GetCurrentDeviceId()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_strCurrentDeviceId;
}

STDMETHODIMP CMpcAudioRenderer::SetCrossFeed(BOOL bValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bUseCrossFeed = bValue;

	if (m_bUseCrossFeed && m_pAudioClient && !m_bs2b_active) {
		if (m_output_params.channels == 2
				&& m_output_params.samplerate >= BS2B_MINSRATE
				&& m_output_params.samplerate <= BS2B_MAXSRATE) {
			m_bs2b.clear();
			m_bs2b.set_srate(m_output_params.samplerate);
			m_bs2b.set_level_fcut(700); // 700 Hz
			m_bs2b.set_level_feed(60);  // 6 dB
			m_bs2b_active = true;
			return S_OK;
		}
	}

	m_bs2b_active = false;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpcAudioRenderer::GetCrossFeed()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bUseCrossFeed;
}

void CMpcAudioRenderer::SetBalanceMask(const DWORD output_layout)
{
	m_dwBalanceMask = 0;
	DWORD quiet_layout = 0;

	if (m_lBalance < DSBPAN_CENTER) { // left louder, right quieter
		quiet_layout = SPEAKER_FRONT_RIGHT|SPEAKER_BACK_RIGHT|SPEAKER_FRONT_RIGHT_OF_CENTER|SPEAKER_SIDE_RIGHT|SPEAKER_TOP_FRONT_RIGHT|SPEAKER_TOP_BACK_RIGHT;
	}
	else if (m_lBalance > DSBPAN_CENTER) { // right louder, left quieter
		quiet_layout = SPEAKER_FRONT_LEFT|SPEAKER_BACK_LEFT|SPEAKER_FRONT_LEFT_OF_CENTER|SPEAKER_SIDE_LEFT|SPEAKER_TOP_FRONT_LEFT|SPEAKER_TOP_BACK_LEFT;
	}

	unsigned channel_num = 0;
	for (unsigned i = 0; i < 32; i++) {
		DWORD ch = 1 << i;

		if (ch & output_layout) {
			if (ch & quiet_layout) {
				m_dwBalanceMask |= 1 << channel_num;
			}
			channel_num++;
		}
	}
}

#pragma region
// ==== WASAPI

static const WCHAR* GetSampleFormatString(const SampleFormat value)
{
#define UNPACK_VALUE(VALUE) case VALUE: return L#VALUE;
	switch (value) {
		UNPACK_VALUE(SAMPLE_FMT_U8);
		UNPACK_VALUE(SAMPLE_FMT_S16);
		UNPACK_VALUE(SAMPLE_FMT_S24);
		UNPACK_VALUE(SAMPLE_FMT_S32);
		UNPACK_VALUE(SAMPLE_FMT_FLT);
		UNPACK_VALUE(SAMPLE_FMT_DBL);
	};
#undef UNPACK_VALUE
	return L"Error value";
};

HRESULT CMpcAudioRenderer::Transform(IMediaSample *pMediaSample)
{
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 1
	DLog(L"CMpcAudioRenderer::Transform()");
#endif

	if (m_filterState == State_Stopped
			|| !m_hRenderThread) {
		return S_FALSE;
	}

	if (m_bNeedReinitializeFull) {
		ReinitializeAudioDevice(TRUE);
	}
	if (m_bNeedReinitialize) {
		ReinitializeAudioDevice();
	}

	CheckBufferStatus();

	HANDLE handles[3] = {
		m_hStopRenderThreadEvent,
		m_FlushEvent,
		m_hRendererNeedMoreData,
	};

	const DWORD result = WaitForMultipleObjects(3, handles, FALSE, INFINITE);
	if (result == WAIT_OBJECT_0 || result == WAIT_OBJECT_0 + 1) {
		// m_hStopRenderThreadEvent, m_FlushEvent
		return S_FALSE;
	}

	long lSize = pMediaSample->GetActualDataLength();
	if (!lSize) {
		return S_FALSE;
	}

	REFERENCE_TIME rtStart, rtStop;
	if (FAILED(pMediaSample->GetTime(&rtStart, &rtStop))) {
		rtStart = rtStop = INVALID_TIME;
	}

	if (rtStart < 0 && rtStart != INVALID_TIME) {
		return S_FALSE;
	}

	m_rtLastSampleTimeEnd = rtStart + SamplesToTime(lSize / m_pWaveFormatExInput->nBlockAlign, m_pWaveFormatExInput) / m_dRate;

	if (!m_pRenderClient) {
		if (!m_pAudioClient) {
			InitAudioClient();
		}
		if (!m_pRenderClient && m_pAudioClient) {
			CheckAudioClient(m_pWaveFormatExInput);
		}
		
		if (!m_pRenderClient) {
			m_rtNextSampleTime = rtStart + SamplesToTime(lSize / m_pWaveFormatExInput->nBlockAlign, m_pWaveFormatExInput) / m_dRate;

			REFERENCE_TIME graphTime;
			m_pSyncClock->GetTime(&graphTime);

			const REFERENCE_TIME duration = m_rtStartTime + rtStart - graphTime - m_hnsPeriod;
			if (duration >= OneMillisecond) {
				const DWORD dwMilliseconds = duration / OneMillisecond;
				WaitForMultipleObjects(2, handles, FALSE, dwMilliseconds);
			}

			return S_FALSE;
		}
	}

	CheckPointer(m_pWaveFormatExInput, S_FALSE);
	CheckPointer(m_pWaveFormatExOutput, S_FALSE);

	if (m_input_params.sf == SAMPLE_FMT_NONE || m_output_params.sf == SAMPLE_FMT_NONE) {
		return S_FALSE;
	}

	if (m_rtEstimateSlavingJitter > 0) {
		const REFERENCE_TIME duration = m_rtEstimateSlavingJitter / m_dRate;
		const UINT32 nSilenceFrames   = TimeToSamples(duration, m_pWaveFormatExOutput);
		const UINT32 nSilenceBytes    = nSilenceFrames * m_pWaveFormatExOutput->nBlockAlign;
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL
		DLog(L"CMpcAudioRenderer::Transform() - Pad silence %.2f ms to minimize slaving jitter", duration / 10000.0);
#endif
		CAutoPtr<CPacket> pSilent(DNew CPacket());
		pSilent->rtStart = rtStart - duration;
		pSilent->rtStop  = rtStart;
		pSilent->SetCount(nSilenceBytes);
		ZeroMemory(pSilent->data(), nSilenceBytes);

		m_rtNextSampleTime = pSilent->rtStart;
		m_rtEstimateSlavingJitter = 0;

		CAutoLock cQueueLock(&m_csQueue);
		m_WasapiQueue.Add(pSilent);

		StartAudioClient();
	}

	BYTE *pMediaBuffer = nullptr;
	HRESULT hr = pMediaSample->GetPointer(&pMediaBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	BYTE *pInputBufferPointer = nullptr;

	BYTE* out_buf = nullptr;

	if (m_bUpdateBalanceMask) {
		SetBalanceMask(m_output_params.layout);
		m_bUpdateBalanceMask = false;
	}

	int in_samples = lSize / m_pWaveFormatExInput->nBlockAlign;
	int out_samples = in_samples;
	UNREFERENCED_PARAMETER(out_samples);

	const bool bFormatChanged = !m_bIsBitstream && (m_input_params.layout != m_output_params.layout || m_input_params.samplerate != m_output_params.samplerate || m_input_params.sf != m_output_params.sf);
	if (bFormatChanged) {
		BYTE* in_buff = &pMediaBuffer[0];

		if (m_input_params.layout != m_output_params.layout || m_input_params.samplerate != m_output_params.samplerate) {
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 2
			CString dbgLog = L"CMpcAudioRenderer::Transform() - use Mixer:\n";
			dbgLog.Append      (L"    input:\n");
			dbgLog.AppendFormat(L"        layout     = 0x%x\n", m_input_params.layout);
			dbgLog.AppendFormat(L"        channels   = %d\n",   m_input_params.channels);
			dbgLog.AppendFormat(L"        samplerate = %d\n",   m_input_params.samplerate);
			dbgLog.AppendFormat(L"        format     = %s\n",   GetSampleFormatString(m_input_params.sf));
			dbgLog.Append      (L"    output:\n");
			dbgLog.AppendFormat(L"        layout     = 0x%x\n", m_output_params.layout);
			dbgLog.AppendFormat(L"        channels   = %d\n",   m_output_params.channels);
			dbgLog.AppendFormat(L"        samplerate = %d\n",   m_output_params.samplerate);
			dbgLog.AppendFormat(L"        format     = %s",     GetSampleFormatString(m_output_params.sf));
			DLog(dbgLog);
#endif

			CAutoLock cResamplerLock(&m_csResampler);

			//m_Resampler.SetOptions(true);
			m_Resampler.UpdateInput(m_input_params.sf, m_input_params.layout, m_input_params.samplerate);
			m_Resampler.UpdateOutput(m_output_params.sf, m_output_params.layout, m_output_params.samplerate);
			out_samples = m_Resampler.CalcOutSamples(in_samples);
			REFERENCE_TIME delay = m_Resampler.GetDelay();
			out_buf     = DNew BYTE[out_samples * m_output_params.channels * get_bytes_per_sample(m_output_params.sf)];
			out_samples = m_Resampler.Mixing(out_buf, out_samples, in_buff, in_samples);
			if (!out_samples) {
				delete [] out_buf;
				return E_INVALIDARG;
			}
			lSize = out_samples * m_output_params.channels * get_bytes_per_sample(m_output_params.sf);

			if (delay && rtStart != INVALID_TIME) {
				rtStart -= delay;
				rtStop  -= delay;
			}
		} else {
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 2
			DLog(L"CMpcAudioRenderer::Transform() - convert: '%s' -> '%s'", GetSampleFormatString(m_input_params.sf), GetSampleFormatString(m_output_params.sf));
#endif

			lSize   = out_samples * m_output_params.channels * get_bytes_per_sample(m_output_params.sf);
			out_buf = DNew BYTE[lSize];

			HRESULT hr = E_INVALIDARG;
			switch (m_output_params.sf) {
				case SAMPLE_FMT_S16:
					hr = convert_to_int16(m_input_params.sf, m_output_params.channels, out_samples, in_buff, (int16_t*)out_buf);
					break;
				case SAMPLE_FMT_S24:
					hr = convert_to_int24(m_input_params.sf, m_output_params.channels, out_samples, in_buff, out_buf);
					break;
				case SAMPLE_FMT_S32:
					hr = convert_to_int32(m_input_params.sf, m_output_params.channels, out_samples, in_buff, (int32_t*)out_buf);
					break;
				case SAMPLE_FMT_FLT:
					hr = convert_to_float(m_input_params.sf, m_output_params.channels, out_samples, in_buff, (float*)out_buf);
					break;
			}

			if (FAILED(hr)) {
				return E_INVALIDARG;
			}
		}

		pInputBufferPointer	= &out_buf[0];
	} else {
		pInputBufferPointer	= &pMediaBuffer[0];
	}

	if (m_bs2b_active) {
		switch (m_output_params.sf) {
		case SAMPLE_FMT_S16:
			m_bs2b.cross_feed((int16_t*)pInputBufferPointer, out_samples);
			break;
		case SAMPLE_FMT_S24:
			m_bs2b.cross_feed((bs2b_int24_t*)pInputBufferPointer, out_samples);
			break;
		case SAMPLE_FMT_S32:
			m_bs2b.cross_feed((int32_t*)pInputBufferPointer, out_samples);
			break;
		case SAMPLE_FMT_FLT:
			m_bs2b.cross_feed((float*)pInputBufferPointer, out_samples);
			break;
		}
	}

	CAutoPtr<CPacket> p(DNew CPacket());
	p->rtStart = rtStart;
	p->rtStop  = rtStop;
	p->SetData(pInputBufferPointer, lSize);

	if (m_bIsBitstream && m_BitstreamMode == BITSTREAM_NONE) {
		if (m_pWaveFormatExOutput->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
			m_BitstreamMode = BITSTREAM_DTS;
			// AC3/DTS
			if (lSize > 8) {
				const BYTE IEC61937_type = pInputBufferPointer[4];
				if (IEC61937_type == IEC61937_AC3) {
					m_BitstreamMode = BITSTREAM_AC3;
				}
			}
		} else if (IsWaveFormatExtensible(m_pWaveFormatExOutput)) {
			const WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE*)m_pWaveFormatExOutput;
			if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS) {
				m_BitstreamMode = BITSTREAM_EAC3;
			} else if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD) {
				m_BitstreamMode = BITSTREAM_DTSHD;
			} else if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
				m_BitstreamMode = BITSTREAM_TRUEHD;
			}
		}
	}

	if (m_dRate != 1.0
			&& !m_bIsBitstream
			&& (m_Filter.IsInitialized() || SUCCEEDED(m_Filter.Init(m_dRate, m_pWaveFormatExOutput, p->rtStart)) )) {
		if (SUCCEEDED(m_Filter.Push(p))) {
			while (SUCCEEDED(m_Filter.Pull(p))) {
				PushToQueue(p);
			}
		}
	} else {
		PushToQueue(p);
	}

	SAFE_DELETE_ARRAY(out_buf);

	if (!m_bIsAudioClientStarted) {
		StartAudioClient();
	}

	return S_OK;
}

HRESULT CMpcAudioRenderer::PushToQueue(CAutoPtr<CPacket> p)
{
	if (m_FlushEvent.Check()) {
		return S_FALSE;
	}

	{
		CAutoLock cQueueLock(&m_csQueue);
		if (p && !m_rtNextSampleTime && !m_WasapiQueue.GetCount()
				&& p->rtStart > 0 && p->rtStart <= 10 * UNITS) {
			const UINT32 nSilenceFrames = TimeToSamples(p->rtStart, m_pWaveFormatExOutput);
			const UINT32 nSilenceBytes  = nSilenceFrames * m_pWaveFormatExOutput->nBlockAlign;
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL
			DLog(L"CMpcAudioRenderer::PushToQueue() - Pad silence %.2f ms", p->rtStart / 10000.0);
#endif
			CAutoPtr<CPacket> pSilent(DNew CPacket());
			pSilent->rtStart = 0;
			pSilent->rtStop  = p->rtStart;
			pSilent->SetCount(nSilenceBytes);
			ZeroMemory(pSilent->data(), nSilenceBytes);

			m_WasapiQueue.Add(pSilent);
		}
	}

	CAutoLock cQueueLock(&m_csQueue);
	m_WasapiQueue.Add(p);

	return S_OK;
}
#pragma endregion

HRESULT CMpcAudioRenderer::GetAudioDevice(const BOOL bForceUseDefaultDevice)
{
	DLog(L"CMpcAudioRenderer::GetAudioDevice() - Target end point device Id: '%s'", m_DeviceId);

	m_bUseDefaultDevice = FALSE;
	m_strCurrentDeviceName.Empty();

	CComPtr<IMMDeviceEnumerator> enumerator;
	HRESULT hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
	if (hr != S_OK) {
		DLog(L"CMpcAudioRenderer::GetAudioDevice() - IMMDeviceEnumerator::CoCreateInstance() failed: (0x%08x)", hr);
		return hr;
	}

	if (!bForceUseDefaultDevice && !m_DeviceId.IsEmpty()) {
		CComPtr<IMMDeviceCollection> devices;
		hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);
		if (hr != S_OK) {
			DLog(L"CMpcAudioRenderer::GetAudioDevice() - IMMDeviceEnumerator::EnumAudioEndpoints() failed: (0x%08x)", hr);
			return hr;
		}

		UINT count = 0;
		hr = devices->GetCount(&count);
		if (hr != S_OK) {
			DLog(L"CMpcAudioRenderer::GetAudioDevice() - IMMDeviceCollection::GetCount() failed: (0x%08x)", hr);
			return hr;
		}

		IPropertyStore* pProps = nullptr;

		for (UINT i = 0; i < count; i++) {
			LPWSTR pwszID = nullptr;
			IMMDevice *endpoint = nullptr;
			hr = devices->Item(i, &endpoint);
			if (hr == S_OK) {
				hr = endpoint->GetId(&pwszID);
				if (hr == S_OK) {
					hr = endpoint->OpenPropertyStore(STGM_READ, &pProps);
					if (hr == S_OK) {
						PROPVARIANT eventDriven;
						PropVariantInit(&eventDriven);
						BOOL bEventDrivenModeSupport = (pProps->GetValue(PKEY_AudioEndpoint_Supports_EventDriven_Mode, &eventDriven) == S_OK);
						PropVariantClear(&eventDriven);

						PROPVARIANT varName;
						PropVariantInit(&varName);

						// Found the configured audio endpoint
						if (bEventDrivenModeSupport
								&& (pProps->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK) && (m_DeviceId == CString(pwszID))) {
							DLog(L"CMpcAudioRenderer::GetAudioDevice() - devices->GetId() OK, num: (%u), pwszVal: '%s', pwszID: '%s'", i, varName.pwszVal, pwszID);

							m_strCurrentDeviceId = pwszID;
							m_strCurrentDeviceName = varName.pwszVal;

							enumerator->GetDevice(pwszID, &m_pMMDevice);
							m_pMMDevice = endpoint;
							CoTaskMemFree(pwszID);
							pwszID = nullptr;
							PropVariantClear(&varName);
							SAFE_RELEASE(pProps);
							return S_OK;
						} else {
							PropVariantClear(&varName);
							SAFE_RELEASE(pProps);
							SAFE_RELEASE(endpoint);
							CoTaskMemFree(pwszID);
							pwszID = nullptr;
							hr = E_FAIL;
						}
					}
				} else {
					DLog(L"CMpcAudioRenderer::GetAudioDevice() - devices->GetId() failed: (0x%08x)", hr);
				}
			} else {
				DLog(L"CMpcAudioRenderer::GetAudioDevice() - devices->Item() failed: (0x%08x)", hr);
			}

			CoTaskMemFree(pwszID);
			pwszID = nullptr;
		}
	}

	m_bUseDefaultDevice = TRUE;

	hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pMMDevice);
	if (hr == S_OK) {
		LPWSTR pwszID = nullptr;
		hr = m_pMMDevice->GetId(&pwszID);
		if (hr == S_OK) {
			IPropertyStore* pProps = nullptr;
			hr = m_pMMDevice->OpenPropertyStore(STGM_READ, &pProps);
			if (hr == S_OK) {
				hr = E_FAIL;

				PROPVARIANT eventDriven;
				PropVariantInit(&eventDriven);
				BOOL bEventDrivenModeSupport = (pProps->GetValue(PKEY_AudioEndpoint_Supports_EventDriven_Mode, &eventDriven) == S_OK);
				PropVariantClear(&eventDriven);

				PROPVARIANT varName;
				PropVariantInit(&varName);
				if (bEventDrivenModeSupport
						&& (pProps->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK)) {
					DLog(L"CMpcAudioRenderer::GetAudioDevice() - Unable to find selected audio device, using the default end point : pwszVal: '%s', pwszID: '%s'", varName.pwszVal, pwszID);

					m_strCurrentDeviceId = pwszID;
					m_strCurrentDeviceName = varName.pwszVal;

					hr = S_OK;
				}

				PropVariantClear(&varName);
				SAFE_RELEASE(pProps);
			}
		}

		CoTaskMemFree(pwszID);
	}

	return hr;
}

HRESULT CMpcAudioRenderer::InitAudioClient()
{
	DLog(L"CMpcAudioRenderer::InitAudioClient()");

	HRESULT hr	= S_OK;
	m_hnsPeriod	= 0;

	if (m_pMMDevice == nullptr) {
		DLog(L"CMpcAudioRenderer::InitAudioClient() - failed, device not loaded");
		return E_FAIL;
	}

	if (m_pAudioClient) {
		PauseRendererThread();
		m_bIsAudioClientStarted = false;

		SAFE_RELEASE(m_pAudioClient);
	}

	hr = m_pMMDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&m_pAudioClient));
	if (FAILED(hr)) {
		DLog(L"CMpcAudioRenderer::InitAudioClient() - IMMDevice::Activate() failed: (0x%08x)", hr);
	} else {
		DLog(L"CMpcAudioRenderer::InitAudioClient() - success");
		if (!m_wBitsPerSampleList.GetSize()) {
			// get list of supported output formats - wBitsPerSample, nChannels(dwChannelMask), nSamplesPerSec
			const WORD  wBitsPerSampleValues[] = {16, 24, 32};
			const DWORD nSamplesPerSecValues[] = {44100, 48000, 88200, 96000, 176400, 192000};
			const WORD  nChannelsValues[]      = {2, 4, 4, 6, 6, 8, 8};
			const DWORD dwChannelMaskValues[]  = {
				KSAUDIO_SPEAKER_STEREO,
				KSAUDIO_SPEAKER_QUAD, KSAUDIO_SPEAKER_SURROUND,
				KSAUDIO_SPEAKER_5POINT1_SURROUND, KSAUDIO_SPEAKER_5POINT1,
				KSAUDIO_SPEAKER_7POINT1_SURROUND, KSAUDIO_SPEAKER_7POINT1
			};

			auto RemoveAll = [&]() {
				m_wBitsPerSampleList.RemoveAll();
				m_nChannelsList.RemoveAll();
				m_dwChannelMaskList.RemoveAll();
				m_AudioParamsList.RemoveAll();
				m_bReal32bitSupport = FALSE;
			};

			WAVEFORMATEXTENSIBLE wfex;

			// 1 - wBitsPerSample
			for (int i = 0; i < _countof(wBitsPerSampleValues); i++) {
				for (int k = 0; k < _countof(nSamplesPerSecValues); k++) {
					CreateFormat(wfex, wBitsPerSampleValues[i], 2, KSAUDIO_SPEAKER_STEREO, nSamplesPerSecValues[k]);
					if (S_OK == m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfex, nullptr)) {
						if (m_wBitsPerSampleList.Find(wBitsPerSampleValues[i]) == -1) {
							m_wBitsPerSampleList.Add(wBitsPerSampleValues[i]);
						}

						if (wBitsPerSampleValues[i] == 32) {
							CreateFormat(wfex, wBitsPerSampleValues[i], 2, KSAUDIO_SPEAKER_STEREO, nSamplesPerSecValues[k], 32);
							m_bReal32bitSupport = (S_OK == m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfex, nullptr));
						}
					}
				}
			}
			if (m_wBitsPerSampleList.GetSize() == 0) {
				RemoveAll();
				return hr;
			}

			// 2 - m_nSamplesPerSec
			for (int k = 0; k < m_wBitsPerSampleList.GetSize(); k++) {
				for (int i = 0; i < _countof(nSamplesPerSecValues); i++) {
					CreateFormat(wfex, m_wBitsPerSampleList[k], nChannelsValues[0], dwChannelMaskValues[0], nSamplesPerSecValues[i]);
					if (S_OK == m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfex, nullptr)) {
						AudioParams ap(m_wBitsPerSampleList[k], nSamplesPerSecValues[i]);
						m_AudioParamsList.Add(ap);
					}
				}
			}
			if (m_AudioParamsList.GetSize() == 0) {
				RemoveAll();
				return hr;
			}

			// 3 - nChannels(dwChannelMask)
			AudioParams ap = m_AudioParamsList[0];
			for (int i = 0; i < _countof(nChannelsValues); i++) {
				CreateFormat(wfex, ap.wBitsPerSample, nChannelsValues[i], dwChannelMaskValues[i], ap.nSamplesPerSec);
				if (S_OK == m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfex, nullptr)) {
					m_nChannelsList.Add(nChannelsValues[i]);
					m_dwChannelMaskList.Add(dwChannelMaskValues[i]);
				}
			}
			if (m_nChannelsList.GetSize() == 0) {
				RemoveAll();
				return hr;
			}

#ifdef DEBUG_OR_LOG
			DLog(L"    List of supported output formats:");
			DLog(L"        BitsPerSample:");
			for (int i = 0; i < m_wBitsPerSampleList.GetSize(); i++) {
				if (m_wBitsPerSampleList[i] == 32 && !m_bReal32bitSupport) {
					DLog(L"            24 padded to 32");
				} else {
					DLog(L"            %d", m_wBitsPerSampleList[i]);
				}
				DLog(L"            SamplesPerSec:");
				for (int k = 0; k < m_AudioParamsList.GetSize(); k++) {
					if (m_AudioParamsList[k].wBitsPerSample == m_wBitsPerSampleList[i]) {
						DLog(L"                %d", m_AudioParamsList[k].nSamplesPerSec);
					}
				}
			}

			#define ADDENTRY(mode) ChannelMaskStr[mode] = L#mode
			std::map<DWORD, CString> ChannelMaskStr;
			ADDENTRY(KSAUDIO_SPEAKER_STEREO);
			ADDENTRY(KSAUDIO_SPEAKER_QUAD);
			ADDENTRY(KSAUDIO_SPEAKER_SURROUND);
			ADDENTRY(KSAUDIO_SPEAKER_5POINT1_SURROUND);
			ADDENTRY(KSAUDIO_SPEAKER_5POINT1);
			ADDENTRY(KSAUDIO_SPEAKER_7POINT1_SURROUND);
			ADDENTRY(KSAUDIO_SPEAKER_7POINT1);
			#undef ADDENTRY

			DLog(L"        Channels:");
			for (int i = 0; i < m_nChannelsList.GetSize(); i++) {
				DLog(L"            %d/0x%x  [%s]", m_nChannelsList[i], m_dwChannelMaskList[i], ChannelMaskStr[m_dwChannelMaskList[i]]);
			}
#endif
		}
	}
	return hr;
}

HRESULT CMpcAudioRenderer::CreateAudioClient(const BOOL bForceUseDefaultDevice/* = FALSE*/)
{
	HRESULT hr = S_OK;
	if (!m_pMMDevice) {
		hr = GetAudioDevice(bForceUseDefaultDevice);
		if (FAILED(hr)) {
			return hr;
		}
	}

	if (!m_pAudioClient) {
		hr = InitAudioClient();
	}

	return hr;
}

HRESULT CMpcAudioRenderer::CheckAudioClient(const WAVEFORMATEX *pWaveFormatEx)
{
	CAutoLock cAutoLock(&m_csCheck);
	DLog(L"CMpcAudioRenderer::CheckAudioClient()");

	BOOL bForceUseDefaultDevice = FALSE;

again:
	HRESULT hr = CreateAudioClient(bForceUseDefaultDevice);
	if (FAILED(hr)) {
		DLog(L"CMpcAudioRenderer::CheckAudioClient() - audio client initialization error");
		return hr;
	}

	auto CheckFormatChanged = [&](const WAVEFORMATEX *pWaveFormatEx, WAVEFORMATEX **ppNewWaveFormatEx) {
		if (m_pWaveFormatExInput == nullptr || IsFormatChanged(pWaveFormatEx, m_pWaveFormatExInput)) {
			return CopyWaveFormat(pWaveFormatEx, ppNewWaveFormatEx);
		}

		return false;
	};

	BOOL bInitNeed = TRUE;
	// Compare the existing WAVEFORMATEX with the one provided
	if (CheckFormatChanged(pWaveFormatEx, &m_pWaveFormatExInput) || !m_pWaveFormatExOutput) {

		// Format has changed, audio client has to be reinitialized
		DLog(L"CMpcAudioRenderer::CheckAudioClient() - Format changed, re-initialize the audio client");

		CopyWaveFormat(m_pWaveFormatExInput, &m_pWaveFormatExOutput);

		if (IsExclusive(pWaveFormatEx)) { // EXCLUSIVE/BITSTREAM
			WAVEFORMATEX *pFormat = nullptr;
			IPropertyStore *pProps = nullptr;
			PROPVARIANT varConfig;

			if (IsBitstream(pWaveFormatEx)) {
				pFormat = (WAVEFORMATEX*)pWaveFormatEx;
			} else {
				WAVEFORMATEXTENSIBLE wfexAsIs = { 0 };

				if (m_bUseBitExactOutput) {
					if (!m_bUseSystemLayoutChannels
							&& SUCCEEDED(SelectFormat(pWaveFormatEx, wfexAsIs))) {
						PauseRendererThread();
						m_bIsAudioClientStarted = false;

						SAFE_RELEASE(m_pRenderClient);
						m_pSyncClock->UnSlave();
						SAFE_RELEASE(m_pAudioClock);
						SAFE_RELEASE(m_pAudioClient);
						hr = InitAudioClient();
						if (SUCCEEDED(hr)) {
							WAVEFORMATEXTENSIBLE wfex = { 0 };
							DWORD dwChannelMask = GetDefChannelMask(pWaveFormatEx->nChannels);
							if (IsWaveFormatExtensible(pWaveFormatEx)) {
								dwChannelMask = ((WAVEFORMATEXTENSIBLE*)pWaveFormatEx)->dwChannelMask;
							}
							CreateFormat(wfex, wfexAsIs.Format.wBitsPerSample, pWaveFormatEx->nChannels, dwChannelMask, pWaveFormatEx->nSamplesPerSec);
							hr = CreateRenderClient((WAVEFORMATEX*)&wfex, FALSE);
							if (S_OK == hr) {
								CopyWaveFormat((WAVEFORMATEX*)&wfex, &m_pWaveFormatExOutput);
								bInitNeed = FALSE;
							}
						}
					}

					if(bInitNeed
							&& SUCCEEDED(SelectFormat(pWaveFormatEx, wfexAsIs))) {
						hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfexAsIs, nullptr);
						if (S_OK == hr) {
							pFormat = (WAVEFORMATEX*)&wfexAsIs;
						}
					}
				}

				if (!pFormat && bInitNeed) {
					hr = m_pMMDevice->OpenPropertyStore(STGM_READ, &pProps);
					if (SUCCEEDED(hr)) {
						PropVariantInit(&varConfig);
						hr = pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &varConfig);
						if (SUCCEEDED(hr) && varConfig.vt == VT_BLOB && varConfig.blob.pBlobData != nullptr) {
							pFormat = (WAVEFORMATEX*)varConfig.blob.pBlobData;
						}
					}
				}
			}

			if (bInitNeed) {
				hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pFormat, nullptr);
				if (S_OK == hr) {
					CopyWaveFormat(pFormat, &m_pWaveFormatExOutput);
				}
			}

			if (pProps) {
				PropVariantClear(&varConfig);
				SAFE_RELEASE(pProps);
			}
		} else if (m_WASAPIMode == MODE_WASAPI_SHARED) { // SHARED
			WAVEFORMATEX* pDeviceFormat = nullptr;
			hr = m_pAudioClient->GetMixFormat(&pDeviceFormat);
			if (SUCCEEDED(hr) && pDeviceFormat) {
				CopyWaveFormat(pDeviceFormat, &m_pWaveFormatExOutput);
				CoTaskMemFree(pDeviceFormat);
			}
		}

#ifdef DEBUG_OR_LOG
		DLog(L"    Input format:");
		DumpWaveFormatEx(pWaveFormatEx);

		DLog(L"    Output format:");
		DumpWaveFormatEx(m_pWaveFormatExOutput);
#endif

		if (bInitNeed) {
			m_pSyncClock->UnSlave();

			PauseRendererThread();
			m_bIsAudioClientStarted = false;

			SAFE_RELEASE(m_pRenderClient);
			m_pSyncClock->UnSlave();
			SAFE_RELEASE(m_pAudioClock);
			SAFE_RELEASE(m_pAudioClient);
		}

		if (S_OK == hr) {
			DLog(L"CMpcAudioRenderer::CheckAudioClient() - WASAPI client accepted the format");
			if (bInitNeed) {
				hr = InitAudioClient();
			}
		} else if (AUDCLNT_E_UNSUPPORTED_FORMAT == hr) {
			DLog(L"CMpcAudioRenderer::CheckAudioClient() - WASAPI client refused the format");
			SAFE_DELETE_ARRAY(m_pWaveFormatExOutput);
			return hr;
		} else {
			DLog(L"CMpcAudioRenderer::CheckAudioClient() - WASAPI failed: (0x%08x)", hr);
			SAFE_DELETE_ARRAY(m_pWaveFormatExOutput);
			return hr;
		}
	} else if (m_pRenderClient == nullptr) {
		DLog(L"CMpcAudioRenderer::CheckAudioClient() - First initialization of the audio renderer");
	} else {
		return hr;
	}

	if (SUCCEEDED(hr) && bInitNeed) {
		SAFE_RELEASE(m_pRenderClient);
		m_pSyncClock->UnSlave();
		SAFE_RELEASE(m_pAudioClock);
		hr = CreateRenderClient(m_pWaveFormatExOutput);

		if (hr == AUDCLNT_E_DEVICE_IN_USE && !m_bUseDefaultDevice) {
			SAFE_RELEASE(m_pRenderClient);
			m_pSyncClock->UnSlave();
			SAFE_RELEASE(m_pAudioClock);
			SAFE_RELEASE(m_pAudioClient);
			SAFE_RELEASE(m_pMMDevice);

			SAFE_DELETE_ARRAY(m_pWaveFormatExOutput);

			bForceUseDefaultDevice = TRUE;
			goto again;
		}
	}

	if (SUCCEEDED(hr)) {
		m_Filter.Flush();
	}

	m_bs2b_active = false;

	if (SUCCEEDED(hr)) {
		auto FillFormats = [](const WAVEFORMATEX* pwfe, AudioFormats& format, bool bInput) {
			ZeroMemory(&format, sizeof(format));
			format.sf = SAMPLE_FMT_NONE;

			format.channels   = pwfe->nChannels;
			format.samplerate = pwfe->nSamplesPerSec;

			bool bFloat = false;
			if (IsWaveFormatExtensible(pwfe)) {
				const WAVEFORMATEXTENSIBLE* pwfex = (WAVEFORMATEXTENSIBLE*)pwfe;
				format.layout = pwfex->dwChannelMask;
				bFloat        = !!(pwfex->SubFormat == MEDIASUBTYPE_IEEE_FLOAT);
			} else {
				format.layout = GetDefChannelMask(pwfe->nChannels);
				bFloat        = !!(pwfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
			}

			switch (pwfe->wBitsPerSample) {
				case 8:
					if (bInput) {
						format.sf = SAMPLE_FMT_U8;
					}
					break;
				case 16:
					format.sf = SAMPLE_FMT_S16;
					break;
				case 24:
					format.sf = SAMPLE_FMT_S24;
					break;
				case 32:
					format.sf = bFloat ? SAMPLE_FMT_FLT : SAMPLE_FMT_S32;
					break;
				case 64:
					if (bInput && bFloat) {
						format.sf = SAMPLE_FMT_DBL;
					}
					break;
			}
		};

		FillFormats(m_pWaveFormatExInput, m_input_params, true);
		FillFormats(m_pWaveFormatExOutput, m_output_params, false);

		if (m_bUseCrossFeed) {
			if (!m_bIsBitstream
					&& m_output_params.channels == 2
					&& m_output_params.samplerate >= BS2B_MINSRATE
					&& m_output_params.samplerate <= BS2B_MAXSRATE) {
				m_bs2b.clear();
				m_bs2b.set_srate(m_output_params.samplerate);
				m_bs2b.set_level_fcut(700); // 700 Hz
				m_bs2b.set_level_feed(60);  // 6 dB
				m_bs2b_active = true;
			}
		}
	}

	return hr;
}

static HRESULT CalcBitstreamBufferPeriod(WAVEFORMATEX *pWaveFormatEx, REFERENCE_TIME *pHnsBufferPeriod)
{
	CheckPointer(pWaveFormatEx, E_FAIL);

	UINT32 nBufferSize = 0;

	if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
		nBufferSize = 6144;
	} else if (IsWaveFormatExtensible(pWaveFormatEx)) {
		const WAVEFORMATEXTENSIBLE *wfext = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;

		if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
			nBufferSize = 61440;
		} else if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD) {
			nBufferSize = 32768;
		} else if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS) {
			nBufferSize = 24576;
		}
	}

	if (nBufferSize) {
		*pHnsBufferPeriod = llMulDiv(nBufferSize << 3, UNITS, pWaveFormatEx->nChannels * pWaveFormatEx->wBitsPerSample * pWaveFormatEx->nSamplesPerSec, 0);

		DLog(L"CalcBitstreamBufferPeriod() - set period of %I64d hundred-nanoseconds for a %u buffer size", *pHnsBufferPeriod, nBufferSize);
	}

	return S_OK;
}

HRESULT CMpcAudioRenderer::CreateRenderClient(WAVEFORMATEX *pWaveFormatEx, const BOOL bCheckFormat/* = TRUE*/)
{
	DLog(L"CMpcAudioRenderer::CreateRenderClient()");
	HRESULT hr = S_OK;

	m_hnsPeriod = 0;
	m_BitstreamMode = BITSTREAM_NONE;

	REFERENCE_TIME hnsDefaultDevicePeriod = 0;
	REFERENCE_TIME hnsMinimumDevicePeriod = 0;
	hr = m_pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, &hnsMinimumDevicePeriod);
	if (SUCCEEDED(hr)) {
		DLog(L"CMpcAudioRenderer::CreateRenderClient() - DefaultDevicePeriod = %.2f ms, MinimumDevicePeriod = %.2f ms", hnsDefaultDevicePeriod / 10000.0, hnsMinimumDevicePeriod / 10000.0);
		m_hnsPeriod = IsExclusive(pWaveFormatEx) ? hnsMinimumDevicePeriod : hnsDefaultDevicePeriod;
	}
	m_hnsPeriod = std::max(500000LL, m_hnsPeriod); // 50 ms - minimal duration of buffer, TODO - optional ???

	const AUDCLNT_SHAREMODE ShareMode = IsExclusive(pWaveFormatEx) ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED;

	if (bCheckFormat) {
		WAVEFORMATEX *pClosestMatch = nullptr;
		hr = m_pAudioClient->IsFormatSupported(ShareMode,
											   pWaveFormatEx,
											   &pClosestMatch);
		if (pClosestMatch) {
			CoTaskMemFree(pClosestMatch);
		}

		if (S_OK == hr) {
			DLog(L"CMpcAudioRenderer::CreateRenderClient() - WASAPI client accepted the format");
		} else if (S_FALSE == hr) {
			DLog(L"CMpcAudioRenderer::CreateRenderClient() - WASAPI client refused the format with a closest match");
			return hr;
		} else if (AUDCLNT_E_UNSUPPORTED_FORMAT == hr) {
			DLog(L"CMpcAudioRenderer::CreateRenderClient() - WASAPI client refused the format");
			return hr;
		} else {
			DLog(L"CMpcAudioRenderer::CreateRenderClient() - IAudioClient::IsFormatSupported() failed: (0x%08x)", hr);
			return hr;
		}
	}

	CalcBitstreamBufferPeriod(pWaveFormatEx, &m_hnsPeriod);
	const REFERENCE_TIME hnsPeriodicity = IsExclusive(pWaveFormatEx) ? m_hnsPeriod : 0;

	if (SUCCEEDED(hr)) {
		hr = m_pAudioClient->Initialize(ShareMode,
										AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
										m_hnsPeriod,
										hnsPeriodicity,
										pWaveFormatEx, nullptr);
	}

	if (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED == hr && IsExclusive(pWaveFormatEx)) {
		// if the buffer size was not aligned, need to do the alignment dance
		DLog(L"CMpcAudioRenderer::CreateRenderClient() - Buffer size not aligned. Realigning");

		// get the buffer size, which will be aligned
		hr = m_pAudioClient->GetBufferSize(&m_nFramesInBuffer);

		// throw away this IAudioClient
		PauseRendererThread();
		m_bIsAudioClientStarted = false;

		SAFE_RELEASE(m_pRenderClient);
		m_pSyncClock->UnSlave();
		SAFE_RELEASE(m_pAudioClock);
		SAFE_RELEASE(m_pAudioClient);
		if (SUCCEEDED(hr)) {
			hr = InitAudioClient();
		}

		// calculate the new aligned periodicity
		m_hnsPeriod = SamplesToTime(m_nFramesInBuffer, pWaveFormatEx);

		DLog(L"CMpcAudioRenderer::CreateRenderClient() - Trying again with periodicity of %I64d hundred-nanoseconds, or %u frames.", m_hnsPeriod, m_nFramesInBuffer);
		if (SUCCEEDED(hr)) {
			hr = m_pAudioClient->Initialize(ShareMode,
											AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
											m_hnsPeriod,
											m_hnsPeriod,
											pWaveFormatEx, nullptr);
		}
	}

	if (FAILED(hr)) {
		DLog(L"CMpcAudioRenderer::CreateRenderClient() - IAudioClient::Initialize() failed: (0x%08x)", hr);
		return hr;
	}

	EXIT_ON_ERROR(hr);

	// get the buffer size, which is aligned
	hr = m_pAudioClient->GetBufferSize(&m_nFramesInBuffer);
	EXIT_ON_ERROR(hr);
	DLog(L"CMpcAudioRenderer::CreateRenderClient() - NumBufferFrames = %u (%.2f ms)", m_nFramesInBuffer, m_nFramesInBuffer * 1000.0 / pWaveFormatEx->nSamplesPerSec);

	// enables a client to write output data to a rendering endpoint buffer
	hr = m_pAudioClient->GetService(IID_PPV_ARGS(&m_pRenderClient));

	if (FAILED(hr)) {
		DLog(L"CMpcAudioRenderer::CreateRenderClient() - IAudioClient::GetService(IAudioRenderClient) failed: (0x%08x)", hr);
	} else {
		DLog(L"CMpcAudioRenderer::CreateRenderClient() - service initialization success, with periodicity of %I64d hundred-nanoseconds = %u frames", m_hnsPeriod, m_nFramesInBuffer);
		m_bIsBitstream = IsBitstream(pWaveFormatEx);
	}
	EXIT_ON_ERROR(hr);

	hr = m_pAudioClient->GetService(IID_PPV_ARGS(&m_pAudioClock));
	if (FAILED(hr)) {
		DLog(L"CMpcAudioRenderer::CreateRenderClient() - IAudioClient::GetService(IAudioClock) failed: (0x%08x)", hr);
	}

	if (!m_bReleased) {
		WasapiFlush();
	}

	auto GetAudioPosition = [&] {
		UINT64 deviceClockFrequency, deviceClockPosition;
		if (SUCCEEDED(m_pAudioClock->GetFrequency(&deviceClockFrequency))
				&& SUCCEEDED(m_pAudioClock->GetPosition(&deviceClockPosition, nullptr))) {
			return llMulDiv(deviceClockPosition, UNITS, deviceClockFrequency, 0);
		}

		return 0LL;
	};

	m_rtEstimateSlavingJitter = 0;
	if (m_filterState == State_Running) {
		if (!m_bReleased) {
			m_rtEstimateSlavingJitter = m_rtLastSampleTimeEnd - (m_pSyncClock->GetPrivateTime() - m_rtStartTime) + GetAudioPosition();
			if (m_rtEstimateSlavingJitter < 0 || m_rtEstimateSlavingJitter > 200 * OneMillisecond) {
				m_rtEstimateSlavingJitter = 0;
			}
			m_pSyncClock->Slave(m_pAudioClock, m_rtStartTime + m_rtLastSampleTimeEnd - m_rtEstimateSlavingJitter);
		} else {
			const REFERENCE_TIME rtEstimateSlavingJitter = m_rtNextSampleTime - (m_pSyncClock->GetPrivateTime() - m_rtStartTime) + GetAudioPosition();
			if (rtEstimateSlavingJitter >= OneMillisecond
					&& rtEstimateSlavingJitter <= 200 * OneMillisecond) {
				const DWORD dwMilliseconds = rtEstimateSlavingJitter / OneMillisecond;
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL
				DLog(L"CMpcAudioRenderer::CreateRenderClient() - Sleep %u ms to minimize slaving jitter", dwMilliseconds);
				Sleep(dwMilliseconds);
#endif
			}
			m_pSyncClock->Slave(m_pAudioClock, m_rtStartTime + m_rtNextSampleTime);
		}
	}

	m_bReleased = false;

	m_nMaxWasapiQueueSize = TimeToSamples(2000000LL, m_pWaveFormatExOutput) * m_pWaveFormatExOutput->nBlockAlign; // 200 ms

	hr = m_pAudioClient->SetEventHandle(m_hDataEvent);
	EXIT_ON_ERROR(hr);

	hr = StartRendererThread();
	EXIT_ON_ERROR(hr);

	return hr;
}

bool CMpcAudioRenderer::IsFormatChanged(const WAVEFORMATEX *pWaveFormatEx, const WAVEFORMATEX *pNewWaveFormatEx)
{
	if (!pWaveFormatEx || !pNewWaveFormatEx) {
		return true;
	}

	if (pWaveFormatEx->wFormatTag != pNewWaveFormatEx->wFormatTag
			|| pWaveFormatEx->nChannels != pNewWaveFormatEx->nChannels
			|| pWaveFormatEx->wBitsPerSample != pNewWaveFormatEx->wBitsPerSample
			|| pWaveFormatEx->nSamplesPerSec != pNewWaveFormatEx->nSamplesPerSec) {
		return true;
	}

	WAVEFORMATEXTENSIBLE* wfex    = nullptr;
	WAVEFORMATEXTENSIBLE* wfexNew = nullptr;
	if (IsWaveFormatExtensible(pWaveFormatEx)) {
		wfex = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;
	}
	if (IsWaveFormatExtensible(pNewWaveFormatEx)) {
		wfexNew	= (WAVEFORMATEXTENSIBLE*)pNewWaveFormatEx;
	}

	if (wfex && wfexNew
			&& (wfex->SubFormat != wfexNew->SubFormat || wfex->dwChannelMask != wfexNew->dwChannelMask)) {
		if (wfex->SubFormat == wfexNew->SubFormat
				&& (wfex->dwChannelMask == KSAUDIO_SPEAKER_5POINT1 && wfexNew->dwChannelMask == KSAUDIO_SPEAKER_5POINT1_SURROUND)) {
			return false;
		}
		return true;
	}

	return false;
}

bool CMpcAudioRenderer::CopyWaveFormat(const WAVEFORMATEX *pSrcWaveFormatEx, WAVEFORMATEX **ppDestWaveFormatEx)
{
	CheckPointer(pSrcWaveFormatEx, false);

	SAFE_DELETE_ARRAY(*ppDestWaveFormatEx);

	size_t size = sizeof(WAVEFORMATEX) + pSrcWaveFormatEx->cbSize;
	*ppDestWaveFormatEx = (WAVEFORMATEX *)DNew BYTE[size];
	if (!(*ppDestWaveFormatEx)) {
		return false;
	}
	memcpy(*ppDestWaveFormatEx, pSrcWaveFormatEx, size);

	return true;
}

BOOL CMpcAudioRenderer::IsBitstream(const WAVEFORMATEX *pWaveFormatEx)
{
	CheckPointer(pWaveFormatEx, FALSE);

	if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
		return TRUE;
	}

	if (IsWaveFormatExtensible(pWaveFormatEx)) {
		WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;
		return (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS
				|| wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD
				|| wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP);
	}

	return FALSE;
}

static DWORD FindClosestInArray(CSimpleArray<DWORD> array, DWORD val)
{
	LONG diff = abs(LONG(val - array[0]));
	DWORD Num = array[0];
	for (int idx = 0; idx < array.GetSize(); idx++) {
		DWORD val1 = array[idx];
		LONG diff1 = abs(LONG(val - array[idx]));
		if (diff > abs(LONG(val - array[idx]))) {
			diff = abs(LONG(val - array[idx]));
			Num = array[idx];
		}
	}

	return Num;
}

HRESULT CMpcAudioRenderer::SelectFormat(const WAVEFORMATEX* pwfx, WAVEFORMATEXTENSIBLE& wfex)
{
	// first - check variables ...
	if (m_wBitsPerSampleList.GetSize() == 0
			|| m_AudioParamsList.GetSize() == 0
			|| m_nChannelsList.GetSize() == 0) {
		return E_FAIL;
	}

	// trying to create an output media type similar to input media type ...

	DWORD nSamplesPerSec = pwfx->nSamplesPerSec;
	BOOL bExists = FALSE;
	for (int i = 0; i < m_AudioParamsList.GetSize(); i++) {
		if (m_AudioParamsList[i].nSamplesPerSec == nSamplesPerSec) {
			bExists = TRUE;
			break;
		}
	}

	if (!bExists) {
		CSimpleArray<DWORD> array;
		for (int i = 0; i < m_AudioParamsList.GetSize(); i++) {
			if (array.Find(m_AudioParamsList[i].nSamplesPerSec) == -1) {
				array.Add(m_AudioParamsList[i].nSamplesPerSec);
			}
		}
		nSamplesPerSec = FindClosestInArray(array, nSamplesPerSec);
	}

	WORD wBitsPerSample = pwfx->wBitsPerSample;
	AudioParams ap(wBitsPerSample, nSamplesPerSec);
	if (m_AudioParamsList.Find(ap) == -1) {
		wBitsPerSample = 0;
		for (int i = m_AudioParamsList.GetSize() - 1; i >= 0; i--) {
			if (m_AudioParamsList[i].nSamplesPerSec == nSamplesPerSec) {
				wBitsPerSample = m_AudioParamsList[i].wBitsPerSample;
				break;
			}
		}
	}

	WORD nChannels      = 0;
	DWORD dwChannelMask = 0;

	if (m_bUseSystemLayoutChannels) {
		// to get the number of channels and channel mask quite simple call IAudioClient::GetMixFormat()
		WAVEFORMATEX *pDeviceFormat = nullptr;
		if (SUCCEEDED(m_pAudioClient->GetMixFormat(&pDeviceFormat)) && pDeviceFormat) {
			nChannels = pDeviceFormat->nChannels;
			dwChannelMask = ((WAVEFORMATEXTENSIBLE*)pDeviceFormat)->dwChannelMask;

			CoTaskMemFree(pDeviceFormat);
		}
	}

	if (!nChannels) {
		nChannels = pwfx->nChannels;
		switch (nChannels) {
			case 1:
			case 3:
				nChannels = 2;
				break;
			case 5:
				nChannels = 6;
				break;
			case 7:
				nChannels = 8;
				break;
		}

		dwChannelMask = GetDefChannelMask(nChannels);
		if (IsWaveFormatExtensible(pwfx)) {
			dwChannelMask = ((WAVEFORMATEXTENSIBLE*)pwfx)->dwChannelMask;
		}

		if (m_nChannelsList.Find(nChannels) == -1) {
			nChannels     = m_nChannelsList[m_nChannelsList.GetSize() - 1];
			dwChannelMask = m_dwChannelMaskList[m_nChannelsList.Find(nChannels)];
		}

		if (m_dwChannelMaskList.Find(dwChannelMask) == -1) {
			dwChannelMask = m_dwChannelMaskList[m_nChannelsList.Find(nChannels)];
		}
	}

	CreateFormat(wfex, wBitsPerSample, nChannels, dwChannelMask, nSamplesPerSec);

	return S_OK;
}

void CMpcAudioRenderer::CreateFormat(WAVEFORMATEXTENSIBLE& wfex, WORD wBitsPerSample, WORD nChannels, DWORD dwChannelMask, DWORD nSamplesPerSec, WORD wValidBitsPerSample/* = 0*/)
{
	ZeroMemory(&wfex, sizeof(wfex));

	WAVEFORMATEX& wfe   = wfex.Format;
	wfe.nChannels       = nChannels;
	wfe.nSamplesPerSec  = nSamplesPerSec;
	wfe.wBitsPerSample  = wBitsPerSample;
	wfe.nBlockAlign     = nChannels * wfe.wBitsPerSample / 8;
	wfe.nAvgBytesPerSec = nSamplesPerSec * wfe.nBlockAlign;

	wfex.Format.wFormatTag               = WAVE_FORMAT_EXTENSIBLE;
	wfex.Format.cbSize                   = sizeof(wfex) - sizeof(wfex.Format);
	wfex.SubFormat                       = MEDIASUBTYPE_PCM;
	wfex.dwChannelMask                   = dwChannelMask;
	wfex.Samples.wValidBitsPerSample     = wValidBitsPerSample ? wValidBitsPerSample : wBitsPerSample;
	if (wfex.Samples.wValidBitsPerSample == 32
			&& !wValidBitsPerSample
			&& !m_bReal32bitSupport) {
		wfex.Samples.wValidBitsPerSample = 24;
	}
}

HRESULT CMpcAudioRenderer::StartAudioClient()
{
	HRESULT hr = S_OK;

	if (m_pAudioClient) {
		if (!m_bIsAudioClientStarted) {
			DLog(L"CMpcAudioRenderer::StartAudioClient()");

			// To reduce latency, load the first buffer with data
			// from the audio source before starting the stream.
			RenderWasapiBuffer();

			if (FAILED(hr = m_pAudioClient->Start())) {
				DLog(L"CMpcAudioRenderer::StartAudioClient() - start audio client failed (0x%08x)", hr);
				return hr;
			}
			m_bIsAudioClientStarted = true;
		} else {
			DLog(L"CMpcAudioRenderer::StartAudioClient() - already started");
		}
	}

	return hr;
}

void CMpcAudioRenderer::SetReinitializeAudioDevice(BOOL bFullInitialization/* = FALSE*/)
{
	if (bFullInitialization) {
		m_bNeedReinitializeFull = TRUE;
	} else {
		m_bNeedReinitialize = TRUE;
	}

	SetEvent(m_hRendererNeedMoreData);
}

HRESULT CMpcAudioRenderer::ReinitializeAudioDevice(BOOL bFullInitialization/* = FALSE*/)
{
	DLog(L"CMpcAudioRenderer::ReinitializeAudioDevice()");

	PauseRendererThread();

	CAutoLock cRenderLock(&m_csRender);

	WAVEFORMATEX* pWaveFormatEx = nullptr;
	CopyWaveFormat(m_pWaveFormatExInput, &pWaveFormatEx);

	if (m_bIsAudioClientStarted && m_pAudioClient) {
		m_pAudioClient->Stop();
	}
	m_bIsAudioClientStarted = false;

	if (bFullInitialization) {
		SAFE_RELEASE(m_pRenderClient);
		SAFE_RELEASE(m_pAudioClient);
		SAFE_RELEASE(m_pMMDevice);

		SAFE_DELETE_ARRAY(m_pWaveFormatExInput);

		m_wBitsPerSampleList.RemoveAll();
		m_nChannelsList.RemoveAll();
		m_dwChannelMaskList.RemoveAll();
		m_AudioParamsList.RemoveAll();

		m_bReal32bitSupport = FALSE;
	}
	SAFE_DELETE_ARRAY(m_pWaveFormatExOutput);

	HRESULT hr = S_OK;

	if (bFullInitialization) {
		hr = CreateAudioClient();
		if (SUCCEEDED(hr)) {
			hr = CheckAudioClient(pWaveFormatEx);
		}
	} else {
		hr = CheckAudioClient(pWaveFormatEx);
	}

	if (FAILED(hr)) {
		DLog(L"CMpcAudioRenderer::ReinitializeAudioDevice() failed: (0x%08x)", hr);
	}

	SAFE_DELETE_ARRAY(pWaveFormatEx);

	m_bNeedReinitialize = m_bNeedReinitializeFull = FALSE;

	return hr;
}

HRESULT CMpcAudioRenderer::RenderWasapiBuffer()
{
	CheckPointer(m_pRenderClient, S_OK);
	CheckPointer(m_pWaveFormatExOutput, S_OK);

	CAutoLock cRenderLock(&m_csRender);

	HRESULT hr = S_OK;

	UINT32 numFramesPadding = 0;
	if (m_WASAPIMode == MODE_WASAPI_SHARED && !m_bIsBitstream) { // SHARED
		m_pAudioClient->GetCurrentPadding(&numFramesPadding);
		if (FAILED(hr)) {
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 1
			DLog(L"CMpcAudioRenderer::RenderWasapiBuffer() - GetCurrentPadding() failed (0x%08x)", hr);
#endif
			return hr;
		}
	}

	const UINT32 numFramesAvailable = m_nFramesInBuffer - numFramesPadding;
	const UINT32 nAvailableBytes = numFramesAvailable * m_pWaveFormatExOutput->nBlockAlign;

	BYTE* pData = nullptr;
	hr = m_pRenderClient->GetBuffer(numFramesAvailable, &pData);
	if (FAILED(hr)) {
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 1
		DLog(L"CMpcAudioRenderer::RenderWasapiBuffer() - GetBuffer() failed with size %u (0x%08x)", numFramesAvailable, hr);
#endif
		return hr;
	}

	const size_t nWasapiQueueSize = WasapiQueueSize();
	DWORD dwFlags = 0;

	const BOOL bFlushing = m_FlushEvent.Check();
	if (!pData || !nWasapiQueueSize || m_filterState != State_Running || bFlushing) {
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 1
		if (m_filterState != State_Running) {
			DLog(L"CMpcAudioRenderer::RenderWasapiBuffer() - not running");
		} else if (!nWasapiQueueSize) {
			DLog(L"CMpcAudioRenderer::RenderWasapiBuffer() - data queue is empty");
		} else if (bFlushing) {
			DLog(L"CMpcAudioRenderer::RenderWasapiBuffer() - flushing");
		}
#endif
		dwFlags = AUDCLNT_BUFFERFLAGS_SILENT;

		if (!nWasapiQueueSize && m_filterState == State_Running && m_bDVDPlayback) {
			m_rtNextSampleTime = m_rtLastSampleTimeEnd = std::max(m_rtNextSampleTime, m_pSyncClock->GetPrivateTime() - m_rtStartTime + m_hnsPeriod);
		}
	} else {
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 1
		DLog(L"CMpcAudioRenderer::RenderWasapiBuffer() - requested: %u[%u], available: %Iu", nAvailableBytes, numFramesAvailable, nWasapiQueueSize);
#endif
		UINT32 nWritenBytes = 0;

		do {
			{
				CAutoLock cQueueLock(&m_csQueue);
				if (!m_CurrentPacket && m_WasapiQueue.GetCount()) {
					m_CurrentPacket = m_WasapiQueue.Remove();
					m_nSampleOffset = 0;
				}
			}
			if (!m_CurrentPacket) {
				break;
			}

			if (!m_nSampleOffset) {
				const REFERENCE_TIME rtTimeDelta = m_CurrentPacket->rtStart - m_rtNextSampleTime;
				if (std::abs(rtTimeDelta) > 200) {
					m_pSyncClock->OffsetAudioClock(rtTimeDelta);
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL
					DLog(L"CMpcAudioRenderer::RenderWasapiBuffer() - Discontinuity detected, Correct reference clock by %.2f ms", rtTimeDelta / 10000.0);
#endif
				}
				m_rtNextSampleTime = m_CurrentPacket->rtStart + SamplesToTime(m_CurrentPacket->size() / m_pWaveFormatExOutput->nBlockAlign, m_pWaveFormatExOutput);
			}

			const UINT32 nFilledBytes = std::min((UINT32)m_CurrentPacket->size(), nAvailableBytes - nWritenBytes);
			memcpy(&pData[nWritenBytes], m_CurrentPacket->data(), nFilledBytes);
			if (nFilledBytes == m_CurrentPacket->size()) {
				m_CurrentPacket.Free();
			} else {
				m_CurrentPacket->RemoveHead(nFilledBytes);
			}
			nWritenBytes += nFilledBytes;
			m_nSampleOffset += nFilledBytes;
		} while (nWritenBytes < nAvailableBytes && dwFlags != AUDCLNT_BUFFERFLAGS_SILENT);

		if (dwFlags != AUDCLNT_BUFFERFLAGS_SILENT) {
			if (!nWritenBytes) {
				dwFlags = AUDCLNT_BUFFERFLAGS_SILENT;
			} else if (nWritenBytes < nAvailableBytes) {
				memset(&pData[nWritenBytes], 0, nAvailableBytes - nWritenBytes);
			}
		}

		if (m_lVolume <= DSBVOLUME_MIN) {
			dwFlags = AUDCLNT_BUFFERFLAGS_SILENT;
		}
		else if (!m_bIsBitstream && (m_lVolume < DSBVOLUME_MAX || m_lBalance != DSBPAN_CENTER)) {
			ApplyVolumeBalance(pData, nAvailableBytes);
		}
	}

	hr = m_pRenderClient->ReleaseBuffer(numFramesAvailable, dwFlags);
#if defined(DEBUG_OR_LOG) && DBGLOG_LEVEL > 1
	if (FAILED(hr)) {
		DLog(L"CMpcAudioRenderer::RenderWasapiBuffer() - ReleaseBuffer() failed with size %u (0x%08x)", numFramesAvailable, hr);
	}
#endif

	return hr;
}

void CMpcAudioRenderer::CheckBufferStatus()
{
	if (!m_pWaveFormatExOutput) {
		return;
	}

	const size_t nWasapiQueueSize = WasapiQueueSize();
	if (nWasapiQueueSize < m_nMaxWasapiQueueSize || !m_pRenderClient) {
		SetEvent(m_hRendererNeedMoreData);
	} else {
		ResetEvent(m_hRendererNeedMoreData);
	}
}

void CMpcAudioRenderer::WasapiFlush()
{
	DLog(L"CMpcAudioRenderer::WasapiFlush()");

	{
		CAutoLock cQueueLock(&m_csQueue);
		m_WasapiQueue.RemoveAll();
		m_CurrentPacket.Free();

		m_nSampleOffset = 0;
	}

	{
		CAutoLock cResamplerLock(&m_csResampler);
		m_Resampler.FlushBuffers();
	}
}

HRESULT CMpcAudioRenderer::EndFlush()
{
	WasapiFlush();
	m_Filter.Flush();

	HRESULT hr = CBaseRenderer::EndFlush();
	m_FlushEvent.Reset();

	if (!m_bDVDPlayback) {
		m_rtNextSampleTime = 0;
		m_rtLastSampleTimeEnd = 0;
		m_rtEstimateSlavingJitter = 0;
	}

	return hr;
}

void CMpcAudioRenderer::NewSegment()
{
	if (m_bDVDPlayback) {
		m_rtNextSampleTime = 0;
		m_rtLastSampleTimeEnd = 0;
		m_rtEstimateSlavingJitter = 0;
	}
}

void CMpcAudioRenderer::Flush()
{
	m_FlushEvent.Set();

#if DEBUG
	ULONGLONG start = 0;
	if (m_ReceiveEvent.Check()) {
		start = GetPerfCounter();
	}
#endif
	while (m_ReceiveEvent.Check()) {
		Sleep(1);
	};
#if DEBUG
	if (start) {
		const ULONGLONG end = GetPerfCounter();
		DLog(L"CMpcAudioRenderer::Flush() : Waiting for the end of data receive - %0.3f ms", (end - start) / 10000.0);
	}
#endif
}

size_t CMpcAudioRenderer::WasapiQueueSize()
{
	CAutoLock cQueueLock(&m_csQueue);
	return m_WasapiQueue.GetSize() + (m_CurrentPacket ? m_CurrentPacket->size() : 0);
}

void CMpcAudioRenderer::WaitFinish()
{
	if (!m_bIsBitstream && m_input_params.samplerate != m_output_params.samplerate) {
		int out_samples = m_Resampler.CalcOutSamples(0);
		if (out_samples) {
			REFERENCE_TIME rtStart = m_rtNextSampleTime;
			for (;;) {
				BYTE* buff = DNew BYTE[out_samples * m_output_params.channels * get_bytes_per_sample(m_output_params.sf)];
				out_samples = m_Resampler.Receive(buff, out_samples);
				if (out_samples) {
					CAutoPtr<CPacket> p(DNew CPacket());
					p->rtStart = rtStart;
					p->rtStop  = rtStart + SamplesToTime(out_samples, m_pWaveFormatExOutput);
					p->SetData(buff, out_samples * m_output_params.channels * get_bytes_per_sample(m_output_params.sf));

					rtStart = p->rtStop;

					if (m_dRate != 1.0
							&& (m_Filter.IsInitialized() || SUCCEEDED(m_Filter.Init(m_dRate, m_pWaveFormatExOutput, p->rtStart)) )) {
						if (SUCCEEDED(m_Filter.Push(p))) {
							while (SUCCEEDED(m_Filter.Pull(p))) {
								PushToQueue(p);
							}
						}
					} else {
						PushToQueue(p);
					}
				}

				delete [] buff;

				if (!out_samples) {
					break;
				}

				out_samples = m_Resampler.CalcOutSamples(0);
			}
		}
	}

	for (;;) {
		if (m_filterState != State_Running
				|| m_bNeedReinitialize || m_bNeedReinitializeFull
				|| !WasapiQueueSize()) {
			break;
		}

		Sleep(m_hnsPeriod / 10000);
	}
}

static VOID CALLBACK TimerCallbackFunc(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	if (auto pRenderer = (CMpcAudioRenderer*)lpParameter) {
		pRenderer->ReleaseDevice();
	}
}

BOOL CMpcAudioRenderer::StartReleaseTimer()
{
	BOOL ret = FALSE;
	if (m_bReleaseDeviceIdle && IsExclusiveMode() && !m_hReleaseTimerHandle) {
		ret = CreateTimerQueueTimer(
				&m_hReleaseTimerHandle,
				nullptr,
				TimerCallbackFunc,
				this,
				3000,
				0,
				WT_EXECUTEINTIMERTHREAD);
	}

	return ret;
}

void CMpcAudioRenderer::EndReleaseTimer()
{
	if (IsExclusiveMode() && m_hReleaseTimerHandle) {
		DeleteTimerQueueTimer(nullptr, m_hReleaseTimerHandle, INVALID_HANDLE_VALUE);
		m_hReleaseTimerHandle = nullptr;
	}
}

void CMpcAudioRenderer::ReleaseDevice()
{
	if (IsExclusiveMode()) {
		DLog(L"CMpcAudioRenderer::ReleaseDevice()");

		m_bReleased = true;
		m_eReleaseEvent.Set();

		PauseRendererThread();
		m_bIsAudioClientStarted = false;

		SAFE_RELEASE(m_pRenderClient);
		SAFE_RELEASE(m_pAudioClock);
		SAFE_RELEASE(m_pAudioClient);

		m_eReleaseEvent.Reset();
	}
}

void CMpcAudioRenderer::ApplyVolumeBalance(BYTE* pData, UINT32 size)
{
	const SampleFormat sf   = m_output_params.sf;
	const size_t channels   = m_output_params.channels;
	const size_t allsamples = size / (m_pWaveFormatExOutput->wBitsPerSample / 8);
	const size_t samples    = allsamples / channels;

	if (m_lBalance == DSBPAN_CENTER) {
		switch (sf) {
		case SAMPLE_FMT_U8:
			gain_uint8(m_dVolumeFactor, allsamples, (uint8_t*)pData);
			break;
		case SAMPLE_FMT_S16:
			gain_int16(m_dVolumeFactor, allsamples, (int16_t*)pData);
			break;
		case SAMPLE_FMT_S24:
			gain_int24(m_dVolumeFactor, allsamples, pData);
			break;
		case SAMPLE_FMT_S32:
			gain_int32(m_dVolumeFactor, allsamples, (int32_t*)pData);
			break;
		case SAMPLE_FMT_FLT:
			gain_float(m_dVolumeFactor, allsamples, (float*)pData);
			break;
		case SAMPLE_FMT_DBL:
			gain_double(m_dVolumeFactor, allsamples, (double*)pData);
			break;
		}
	} else {
		// volume and balance
		// do not use limiter, because  m_dBalanceFactor and m_dVolumeFactor are always less than or equal to 1.0
		switch (sf) {
		case SAMPLE_FMT_U8: {
			uint8_t* p = (uint8_t*)pData;
			for (size_t i = 0; i < samples; i++) {
				for (size_t ch = 0; ch < channels; ch++) {
					int8_t sample = (int8_t)(*p ^ 0x80);
					sample = (int8_t)((m_dwBalanceMask & (1 << ch) ? m_dBalanceFactor : m_dVolumeFactor) * sample);
					*p++ = (uint8_t)sample ^ 0x80;
				}
			}
			break; }
		case SAMPLE_FMT_S16: {
			int16_t* p = (int16_t*)pData;
			for (size_t i = 0; i < samples; i++) {
				for (size_t ch = 0; ch < channels; ch++) {
					*p++ = (int16_t)((m_dwBalanceMask & (1 << ch) ? m_dBalanceFactor : m_dVolumeFactor) * *p);
				}
			}
			break; }
		case SAMPLE_FMT_S24: {
			for (size_t i = 0; i < samples; i++) {
				for (size_t ch = 0; ch < channels; ch++) {
					int32_t sample = 0;
					((BYTE*)(&sample))[1] = *(pData);
					((BYTE*)(&sample))[2] = *(pData + 1);
					((BYTE*)(&sample))[3] = *(pData + 2);
					sample = (int32_t)((m_dwBalanceMask & (1 << ch) ? m_dBalanceFactor : m_dVolumeFactor) * sample);
					*pData++ = ((BYTE*)(&sample))[1];
					*pData++ = ((BYTE*)(&sample))[2];
					*pData++ = ((BYTE*)(&sample))[3];
				}
			}
			break; }
		case SAMPLE_FMT_S32: {
			int32_t* p = (int32_t*)pData;
			for (size_t i = 0; i < samples; i++) {
				for (size_t ch = 0; ch < channels; ch++) {
					*p++ = (int32_t)((m_dwBalanceMask & (1 << ch) ? m_dBalanceFactor : m_dVolumeFactor) * *p);
				}
			}
			break; }
		case SAMPLE_FMT_FLT: {
			float* p = (float*)pData;
			for (size_t i = 0; i < samples; i++) {
				for (size_t ch = 0; ch < channels; ch++) {
					*p++ = (float)((m_dwBalanceMask & (1 << ch) ? m_dBalanceFactor : m_dVolumeFactor) * *p);
				}
			}
			break; }
		case SAMPLE_FMT_DBL: {
			double* p = (double*)pData;
			for (size_t i = 0; i < samples; i++) {
				for (size_t ch = 0; ch < channels; ch++) {
					*p++ = (m_dwBalanceMask & (1 << ch) ? m_dBalanceFactor : m_dVolumeFactor) * *p;
				}
			}
			break; }
		}
	}
}

CMpcAudioRendererInputPin::CMpcAudioRendererInputPin(CBaseRenderer* pRenderer, HRESULT* phr)
	: CRendererInputPin(pRenderer, phr, L"In")
	, m_pRenderer(static_cast<CMpcAudioRenderer*>(pRenderer))
{
}

STDMETHODIMP CMpcAudioRendererInputPin::NewSegment(REFERENCE_TIME startTime, REFERENCE_TIME stopTime, double rate)
{
	DLog(L"CMpcAudioRendererInputPin::NewSegment()");

	CAutoLock cReceiveLock(&m_csReceive);

	m_pRenderer->NewSegment();
	return CRendererInputPin::NewSegment(startTime, stopTime, rate);
}

STDMETHODIMP CMpcAudioRendererInputPin::Receive(IMediaSample* pSample)
{
	CAutoLock cReceiveLock(&m_csReceive);
	return CRendererInputPin::Receive(pSample);
}

STDMETHODIMP CMpcAudioRendererInputPin::EndOfStream()
{
	DLog(L"CMpcAudioRendererInputPin::EndOfStream()");

	CAutoLock cReceiveLock(&m_csReceive);

	m_pRenderer->WaitFinish();
	m_pRenderer->StartReleaseTimer();

	return CRendererInputPin::EndOfStream();
}

STDMETHODIMP CMpcAudioRendererInputPin::BeginFlush()
{
	DLog(L"CMpcAudioRendererInputPin::BeginFlush()");

	m_pRenderer->Flush();

	return CRendererInputPin::BeginFlush();
}

STDMETHODIMP CMpcAudioRendererInputPin::EndFlush()
{
	DLog(L"CMpcAudioRendererInputPin::EndFlush()");

	return CRendererInputPin::EndFlush();
}
