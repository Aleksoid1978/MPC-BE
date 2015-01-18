/*
 * (C) 2009-2015 see Authors.txt
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
#include "MpcAudioRenderer.h"

// option names
#define OPT_REGKEY_AudRend			_T("Software\\MPC-BE Filters\\MPC Audio Renderer")
#define OPT_SECTION_AudRend			_T("Filters\\MPC Audio Renderer")
#define OPT_DeviceMode				_T("UseWasapi")
#define OPT_AudioDevice				_T("SoundDevice")
#define OPT_UseBitExactOutput		_T("UseBitExactOutput")
#define OPT_UseSystemLayoutChannels	_T("UseSystemLayoutChannels")
// TODO: rename option values

// set to 1(or more) to enable more detail debug log
#define DBGLOG_LEVEL 0

#ifdef REGISTER_FILTER

#include "../../filters/ffmpeg_fix.cpp"

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_IEEE_FLOAT}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMpcAudioRenderer), MpcAudioRendererName, /*0x40000001*/MERIT_UNLIKELY + 1, _countof(sudpPins), sudpPins, CLSID_AudioRendererCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, &__uuidof(CMpcAudioRenderer), CreateInstance<CMpcAudioRenderer>, NULL, &sudFilter[0]},
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

static GUID lpSoundGUID = {0xdef00000, 0x9c6d, 0x47ed, {0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03}}; //DSDEVID_DefaultPlayback from dsound.h

bool CALLBACK DSEnumProc2(LPGUID lpGUID,
						  LPCTSTR lpszDesc,
						  LPCTSTR lpszDrvName,
						  LPVOID lpContext )
{
	CString *pStr = (CString *)lpContext;
	ASSERT ( pStr );
	CString strGUID = *pStr;

	if (lpGUID != NULL) { // NULL only for "Primary Sound Driver".
		if (strGUID == lpszDesc) {
			memcpy((VOID *)&lpSoundGUID, lpGUID, sizeof(GUID));
		}
	}

	return TRUE;
}

static void DumpWaveFormatEx(WAVEFORMATEX* pwfx)
{
	DbgLog((LOG_TRACE, 3, L"		=> wFormatTag		= 0x%04x",	pwfx->wFormatTag));
	DbgLog((LOG_TRACE, 3, L"		=> nChannels		= %d",	pwfx->nChannels));
	DbgLog((LOG_TRACE, 3, L"		=> nSamplesPerSec	= %d",	pwfx->nSamplesPerSec));
	DbgLog((LOG_TRACE, 3, L"		=> nAvgBytesPerSec	= %d",	pwfx->nAvgBytesPerSec));
	DbgLog((LOG_TRACE, 3, L"		=> nBlockAlign		= %d",	pwfx->nBlockAlign));
	DbgLog((LOG_TRACE, 3, L"		=> wBitsPerSample	= %d",	pwfx->wBitsPerSample));
	DbgLog((LOG_TRACE, 3, L"		=> cbSize			= %d",	pwfx->cbSize));
	if (IsWaveFormatExtensible(pwfx)) {
		WAVEFORMATEXTENSIBLE* wfe = (WAVEFORMATEXTENSIBLE*)pwfx;
		DbgLog((LOG_TRACE, 3, L"		WAVEFORMATEXTENSIBLE:"));
		DbgLog((LOG_TRACE, 3, L"			=> wValidBitsPerSample	= %d", wfe->Samples.wValidBitsPerSample));
		DbgLog((LOG_TRACE, 3, L"			=> dwChannelMask		= 0x%x", wfe->dwChannelMask));
		DbgLog((LOG_TRACE, 3, L"			=> SubFormat			= %s", CStringFromGUID(wfe->SubFormat)));
	}
}

CMpcAudioRenderer::CMpcAudioRenderer(LPUNKNOWN punk, HRESULT *phr)
	: CBaseRenderer(__uuidof(this), NAME("CMpcAudioRenderer"), punk, phr)
	, m_dRate(1.0)
	, m_pReferenceClock(NULL)
	, m_pWaveFileFormat(NULL)
	, m_pWaveFileFormatOutput(NULL)
	, m_pMMDevice(NULL)
	, m_pAudioClient(NULL)
	, m_pRenderClient(NULL)
	, m_WASAPIMode(MODE_WASAPI_EXCLUSIVE)
	, m_nFramesInBuffer(0)
	, m_hnsPeriod(0)
	, m_hTask(NULL)
	, m_bIsAudioClientStarted(false)
	, m_lVolume(DSBVOLUME_MIN)
	, m_dVolumeFactor(0.0)
	, m_nThreadId(0)
	, m_hRenderThread(NULL)
	, m_bThreadPaused(FALSE)
	, m_hDataEvent(NULL)
	, m_hPauseEvent(NULL)
	, m_hWaitPauseEvent(NULL)
	, m_hResumeEvent(NULL)
	, m_hWaitResumeEvent(NULL)
	, m_hStopRenderThreadEvent(NULL)
	, m_hReinitializeEvent(NULL)
	, m_bIsBitstream(FALSE)
	, m_BitstreamMode(BITSTREAM_NONE)
	, m_hModule(NULL)
	, pfAvSetMmThreadCharacteristicsW(NULL)
	, pfAvRevertMmThreadCharacteristics(NULL)
	, m_bUseBitExactOutput(TRUE)
	, m_bUseSystemLayoutChannels(TRUE)
	, m_filterState(State_Stopped)
	, m_hRendererNeedMoreData(NULL)
	, m_hStopWaitingRenderer(NULL)
	, m_CurrentPacket(NULL)
	, m_rtStartTime(0)
	, m_rtNextSampleTime(0)
	, m_nSampleNum(0)
	, m_bEndOfStream(FALSE)
	, m_bUseDefaultDevice(FALSE)
	, m_eEndReceive(TRUE)
	, m_eReinitialize(TRUE)
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CMpcAudioRenderer()"));

#ifdef REGISTER_FILTER
	CRegKey key;
	TCHAR buff[256];
	ULONG len;

	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_AudRend, KEY_READ)) {
		DWORD dw;
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_DeviceMode, dw)) {
			m_WASAPIMode = (WASAPI_MODE)dw;
		}
		len = _countof(buff);
		memset(buff, 0, sizeof(buff));
		if (ERROR_SUCCESS == key.QueryStringValue(OPT_AudioDevice, buff, &len)) {
			m_DeviceName = CString(buff);
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_UseBitExactOutput, dw)) {
			m_bUseBitExactOutput = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_UseSystemLayoutChannels, dw)) {
			m_bUseSystemLayoutChannels = !!dw;
		}
	}
#else
	m_WASAPIMode				= (WASAPI_MODE)AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_DeviceMode, m_WASAPIMode);
	m_DeviceName				= AfxGetApp()->GetProfileString(OPT_SECTION_AudRend, OPT_AudioDevice, m_DeviceName);
	m_bUseBitExactOutput		= !!AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_UseBitExactOutput, m_bUseBitExactOutput);
	m_bUseSystemLayoutChannels	= !!AfxGetApp()->GetProfileInt(OPT_SECTION_AudRend, OPT_UseSystemLayoutChannels, m_bUseSystemLayoutChannels);
#endif

	m_WASAPIMode				= min(max(m_WASAPIMode, MODE_WASAPI_EXCLUSIVE), MODE_WASAPI_SHARED);
	m_WASAPIModeAfterRestart	= m_WASAPIMode;

	if (phr) {
		*phr = E_FAIL;
	}

	if (IsWinVistaOrLater()) {
		// Load Vista & above specifics DLLs
		m_hModule = LoadLibrary(L"AVRT.dll");
		if (m_hModule != NULL) {
			pfAvSetMmThreadCharacteristicsW		= (PTR_AvSetMmThreadCharacteristicsW)	GetProcAddress(m_hModule, "AvSetMmThreadCharacteristicsW");
			pfAvRevertMmThreadCharacteristics	= (PTR_AvRevertMmThreadCharacteristics)	GetProcAddress(m_hModule, "AvRevertMmThreadCharacteristics");
		} else {
			// Wasapi not available
			return;
		}
	} else {
		// Wasapi not available below Vista
		return;
	}

	m_hDataEvent				= CreateEvent(NULL, FALSE, FALSE, NULL);

	m_hPauseEvent				= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hWaitPauseEvent			= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hResumeEvent				= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hWaitResumeEvent			= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hStopRenderThreadEvent	= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hReinitializeEvent		= CreateEvent(NULL, FALSE, FALSE, NULL);

	m_hRendererNeedMoreData		= CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hStopWaitingRenderer		= CreateEvent(NULL, FALSE, FALSE, NULL);

	HRESULT hr = S_OK;
	// CBaseRenderer is using a lazy initialization for the CRendererPosPassThru - we need it always
	CBasePin *pPin = GetPin(0);
	m_pPosition = new CRendererPosPassThru(NAME("CRendererPosPassThru"), CBaseFilter::GetOwner(), &hr, pPin);
	if (m_pPosition == NULL) {
		hr = E_OUTOFMEMORY;
	} else if (FAILED(hr)) {
		delete m_pPosition;
		m_pPosition = NULL;
		hr = E_NOINTERFACE;
	}

	if (phr) {
		*phr = hr;
	}

	if (SUCCEEDED(hr)) {
		CComPtr<IMMDeviceEnumerator> enumerator;
		if (SUCCEEDED(enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator)))) {
			enumerator->RegisterEndpointNotificationCallback(this);
		}
	}

	m_eEndReceive.Set();
}

CMpcAudioRenderer::~CMpcAudioRenderer()
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::~CMpcAudioRenderer()"));

	{
		CComPtr<IMMDeviceEnumerator> enumerator;
		if (SUCCEEDED(enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator)))) {
			enumerator->UnregisterEndpointNotificationCallback(this);
		}
	}

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
	SAFE_CLOSE_HANDLE(m_hReinitializeEvent);

	SAFE_CLOSE_HANDLE(m_hRendererNeedMoreData);
	SAFE_CLOSE_HANDLE(m_hStopWaitingRenderer);

	SAFE_RELEASE(m_pRenderClient);
	SAFE_RELEASE(m_pAudioClient);
	SAFE_RELEASE(m_pMMDevice);

	if (m_pReferenceClock) {
		SetSyncSource(NULL);
		SAFE_RELEASE(m_pReferenceClock);
	}

	SAFE_DELETE_ARRAY(m_pWaveFileFormat);
	SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);

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
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckMediaType()"));

	if ((pmt->majortype != MEDIATYPE_Audio) || (pmt->formattype != FORMAT_WaveFormatEx)) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckMediaType() - Not supported"));
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	HRESULT hr = S_OK;

	if (pmt->subtype != MEDIASUBTYPE_PCM && pmt->subtype != MEDIASUBTYPE_IEEE_FLOAT) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckMediaType() - allow only PCM/IEEE_FLOAT input"));
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if (pmt->subtype == MEDIASUBTYPE_PCM) {
		// Check S/PDIF & Bistream ...
		BOOL m_bIsBitstreamInput = FALSE;
		WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX*)pmt->pbFormat;

		if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
			m_bIsBitstreamInput = TRUE;
		} else if (IsWaveFormatExtensible(pWaveFormatEx)) {
			WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;
			m_bIsBitstreamInput = (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS
								   || wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD
								   || wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP);
		}

		if (m_bIsBitstreamInput) {
			if (!m_pAudioClient) {
				hr = CheckAudioClient();
				if (FAILED(hr)) {
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckMediaType() - Error on check audio client"));
					return hr;
				}
				if (!m_pAudioClient) {
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckMediaType() - Error, audio client not loaded"));
					return VFW_E_CANNOT_CONNECT;
				}
			}

			hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pWaveFormatEx, NULL);
		}
	}

	return hr;
}

HRESULT CMpcAudioRenderer::Receive(IMediaSample* pSample)
{
	ASSERT(pSample);

	// It may return VFW_E_SAMPLE_REJECTED code to say don't bother
	HRESULT hr = PrepareReceive(pSample);
	ASSERT(m_bInReceive == SUCCEEDED(hr));
	if (FAILED(hr)) {
		if (hr == VFW_E_SAMPLE_REJECTED) {
			return NOERROR;
		}

		return hr;
	}

	if (m_State == State_Paused) {
		{
			CAutoLock cRendererLock(&m_InterfaceLock);
			if (m_State == State_Stopped) {
				m_bInReceive = FALSE;
				return NOERROR;
			}
		}
		Ready();
	}

	// http://blogs.msdn.com/b/mediasdkstuff/archive/2008/09/19/custom-directshow-audio-renderer-hangs-playback-in-windows-media-player-11.aspx
	DoRenderSampleWasapi(pSample);
	m_eEndReceive.Set();

	m_bInReceive = FALSE;

	CAutoLock cRendererLock(&m_InterfaceLock);
	if (m_State == State_Stopped) {
		return NOERROR;
	}

	CAutoLock cSampleLock(&m_RendererLock);

	ClearPendingSample();
	SendEndOfStream();
	CancelNotification();

	return NOERROR;
}


STDMETHODIMP CMpcAudioRenderer::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IReferenceClock) {
		return GetReferenceClockInterface(riid, ppv);
	} else if (riid == IID_IDispatch) {
		return GetInterface(static_cast<IDispatch*>(this), ppv);
	} else if (riid == IID_IBasicAudio) {
		return GetInterface(static_cast<IBasicAudio*>(this), ppv);
	} else if (riid == IID_IMediaSeeking) {
        return GetInterface(static_cast<IMediaSeeking*>(this), ppv);
	} else if (riid == __uuidof(IMMNotificationClient)) {
		return GetInterface(static_cast<IMMNotificationClient*>(this), ppv);
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

	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::SetMediaType()"));

	HRESULT hr = S_OK;

	if (!m_pAudioClient) {
		hr = CheckAudioClient();
		if (FAILED(hr)) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::SetMediaType() - Error on check audio client"));
			return hr;
		}
		if (!m_pAudioClient) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::SetMediaType() - Error, audio client not loaded"));
			return VFW_E_CANNOT_CONNECT;
		}
	}

	if (m_pRenderClient) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::SetMediaType() : Render client already initialized. Reinitialization..."));
	} else {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::SetMediaType() : Render client initialization"));
	}

	hr = CheckAudioClient(pwf);

	EXIT_ON_ERROR(hr);

	CopyWaveFormat(pwf, &m_pWaveFileFormat);

	return CBaseRenderer::SetMediaType(pmt);
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
		m_hRenderThread = NULL;
	}

	return S_OK;
}

HRESULT CMpcAudioRenderer::StartRendererThread()
{
	if (!m_hRenderThread) {
		m_hRenderThread = ::CreateThread(NULL, 0, RenderThreadEntryPoint, (LPVOID)this, 0, &m_nThreadId);
		if (m_hRenderThread) {
			//SetThreadPriority(m_hRenderThread, THREAD_PRIORITY_HIGHEST);
			return S_OK;
		} else {
			return E_FAIL;
		}
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
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderThread() - start, thread ID = 0x%x", m_nThreadId));

	HRESULT hr = S_OK;

	// These are wait handles for the thread stopping, new sample arrival and pausing redering
	HANDLE renderHandles[4] = {
		m_hStopRenderThreadEvent,
		m_hPauseEvent,
		m_hReinitializeEvent,
		m_hDataEvent
	};

	// These are wait handles for resuming or exiting the thread
	HANDLE resumeHandles[2] = {
		m_hStopRenderThreadEvent,
		m_hResumeEvent
	};

	EnableMMCSS();

	while (true) {
		CheckBufferStatus();

		// 1) Waiting for the next WASAPI buffer to be available to be filled
		// 2) Exit requested for the thread
		// 3) For a pause request
		DWORD result = WaitForMultipleObjects(4, renderHandles, FALSE, INFINITE);
		switch (result) {
			case WAIT_OBJECT_0:
				DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderThread() - exit events"));

				RevertMMCSS();
				return 0;
			case WAIT_OBJECT_0 + 1:
				{
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderThread() - pause events"));

					m_bThreadPaused = TRUE;
					ResetEvent(m_hResumeEvent);
					SetEvent(m_hWaitPauseEvent);

					DWORD resultResume = WaitForMultipleObjects(2, resumeHandles, FALSE, INFINITE);
					if (resultResume == WAIT_OBJECT_0) { // exit event
						DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderThread() - exit events"));

						RevertMMCSS();
						return 0;
					}

					if (resultResume == WAIT_OBJECT_0 + 1) { // resume event
						DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderThread() - resume events"));
						m_bThreadPaused = FALSE;
						SetEvent(m_hWaitResumeEvent);
					}
				}
				break;
			case WAIT_OBJECT_0 + 2:
				ReinitializeAudioDevice();
				break;
			case WAIT_OBJECT_0 + 3:
				{
#if defined(_DEBUG) && DBGLOG_LEVEL > 1
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderThread() - Data Event, Audio client state = %s", m_bIsAudioClientStarted ? L"Started" : L"Stoped"));
#endif
					HRESULT hr = RenderWasapiBuffer();
					if (hr == AUDCLNT_E_DEVICE_INVALIDATED ) {
						SetEvent(m_hReinitializeEvent);
					}
				}
				break;
			default:
#if defined(_DEBUG) && DBGLOG_LEVEL > 1
				DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderThread() - WaitForMultipleObjects() failed: %d", GetLastError()));
#endif
				break;
		}
	}

	RevertMMCSS();

	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderThread() - close, thread ID = 0x%x", m_nThreadId));

	return 0;
}

STDMETHODIMP CMpcAudioRenderer::Run(REFERENCE_TIME rtStart)
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::Run()"));

	HRESULT hr = NOERROR;

	if (m_State == State_Running) {
		return hr;
	}

	if (IsEndOfStream()) {
		NotifyEvent(EC_COMPLETE, S_OK, 0);
		// I don't know why, but requires double calling NotifyEvent(EC_COMPLETE, ...);
		return NotifyEvent(EC_COMPLETE, S_OK, 0);
	}

	m_bEndOfStream = FALSE;
	m_nSampleNum = 0;

	m_filterState = State_Running;
	m_rtStartTime = rtStart;

	return CBaseRenderer::Run(rtStart);
}

STDMETHODIMP CMpcAudioRenderer::Stop()
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::Stop()"));

	m_filterState = State_Stopped;
	if (m_hStopWaitingRenderer) {
		SetEvent(m_hStopWaitingRenderer);
	}

	return CBaseRenderer::Stop();
};

STDMETHODIMP CMpcAudioRenderer::Pause()
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::Pause()"));

	m_filterState = State_Paused;
	if (m_hStopWaitingRenderer) {
		SetEvent(m_hStopWaitingRenderer);
	}

	return CBaseRenderer::Pause();
};

// === IBasicAudio
STDMETHODIMP CMpcAudioRenderer::put_Volume(long lVolume)
{
	m_lVolume = lVolume;

	if (m_lVolume <= DSBVOLUME_MIN) {
		m_lVolume = DSBVOLUME_MIN;
		m_dVolumeFactor = 0.0;
	}
	else if (m_lVolume >= DSBVOLUME_MAX) {
		m_lVolume = DSBVOLUME_MAX;
		m_dVolumeFactor = 1.0;
	} else {
		m_dVolumeFactor = pow(10.0, m_lVolume / 2000.0);
	}

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::get_Volume(long *plVolume)
{
	*plVolume = m_lVolume;

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

	if (m_strDeviceId == pwstrDeviceId) {
		SetEvent(m_hReinitializeEvent);
	}

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
{
	CheckPointer(m_hRenderThread, S_OK);

	if (m_bUseDefaultDevice
			&& flow == eRender
			&& role == eConsole) {
		SetEvent(m_hReinitializeEvent);
	}

	return S_OK;
}

// === ISpecifyPropertyPages2
STDMETHODIMP CMpcAudioRenderer::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	BOOL bShowStatusPage	= m_pInputPin && m_pInputPin->IsConnected();

	pPages->cElems			= bShowStatusPage ? 2 : 1;
	pPages->pElems			= (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	CheckPointer(pPages->pElems, E_OUTOFMEMORY);

	pPages->pElems[0]		= __uuidof(CMpcAudioRendererSettingsWnd);
	if (bShowStatusPage) {
		pPages->pElems[1]	= __uuidof(CMpcAudioRendererStatusWnd);
	}

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMpcAudioRendererSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpcAudioRendererSettingsWnd>(NULL, &hr))->AddRef();
	} else if (guid == __uuidof(CMpcAudioRendererStatusWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpcAudioRendererStatusWnd>(NULL, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// === IMpcAudioRendererFilter
STDMETHODIMP CMpcAudioRenderer::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_AudRend)) {
		key.SetDWORDValue(OPT_DeviceMode, (DWORD)m_WASAPIModeAfterRestart);
		key.SetStringValue(OPT_AudioDevice, m_DeviceName);
		key.SetDWORDValue(OPT_UseBitExactOutput, m_bUseBitExactOutput);
		key.SetDWORDValue(OPT_UseSystemLayoutChannels, m_bUseSystemLayoutChannels);
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_DeviceMode, (int)m_WASAPIModeAfterRestart);
	AfxGetApp()->WriteProfileString(OPT_SECTION_AudRend, OPT_AudioDevice, m_DeviceName);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_UseBitExactOutput, m_bUseBitExactOutput);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_AudRend, OPT_UseSystemLayoutChannels, m_bUseSystemLayoutChannels);
#endif

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::SetWasapiMode(INT nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_WASAPIModeAfterRestart = (WASAPI_MODE)nValue;
	return S_OK;
}
STDMETHODIMP_(INT) CMpcAudioRenderer::GetWasapiMode()
{
	CAutoLock cAutoLock(&m_csProps);
	return (INT)m_WASAPIModeAfterRestart;
}

STDMETHODIMP CMpcAudioRenderer::SetSoundDevice(CString nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_DeviceName = nValue;
	return S_OK;
}

STDMETHODIMP_(CString) CMpcAudioRenderer::GetSoundDevice()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_DeviceName;
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

	*ppWfxIn	= m_pWaveFileFormat;
	*ppWfxOut	= m_pWaveFileFormatOutput;

	return S_OK;
}

STDMETHODIMP CMpcAudioRenderer::SetBitExactOutput(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bUseBitExactOutput = nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpcAudioRenderer::GetBitExactOutput()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bUseBitExactOutput;
}

STDMETHODIMP CMpcAudioRenderer::SetSystemLayoutChannels(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bUseSystemLayoutChannels = nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpcAudioRenderer::GetSystemLayoutChannels()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bUseSystemLayoutChannels;
}

STDMETHODIMP_(BITSTREAM_MODE) CMpcAudioRenderer::GetBitstreamMode()
{
	CAutoLock cAutoLock(&m_csProps);
	return (m_pGraph && m_bIsBitstream) ? m_BitstreamMode : BITSTREAM_NONE;
}

STDMETHODIMP_(CString) CMpcAudioRenderer::GetCurrentPlaybackDevice()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_strDeviceName;
}

HRESULT CMpcAudioRenderer::GetReferenceClockInterface(REFIID riid, void **ppv)
{
	HRESULT hr = S_OK;

	if (m_pReferenceClock) {
		return m_pReferenceClock->NonDelegatingQueryInterface(riid, ppv);
	}

	m_pReferenceClock = DNew CBaseReferenceClock(NAME("Mpc Audio Clock"), NULL, &hr);
	if (!m_pReferenceClock) {
		return E_OUTOFMEMORY;
	}

	m_pReferenceClock->AddRef();

	hr = SetSyncSource(m_pReferenceClock);
	if (FAILED(hr)) {
		SetSyncSource(NULL);
		SAFE_RELEASE(m_pReferenceClock);
		return hr;
	}

	return GetReferenceClockInterface(riid, ppv);
}

HRESULT CMpcAudioRenderer::EndOfStream(void)
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::EndOfStream()"));

	if (m_hStopWaitingRenderer) {
		SetEvent(m_hStopWaitingRenderer);
	}

	m_bEndOfStream = TRUE;

	return CBaseRenderer::EndOfStream();
}


#pragma region
// ==== WASAPI

static const TCHAR *GetSampleFormatString(const SampleFormat value)
{
#define UNPACK_VALUE(VALUE) case VALUE: return _T( #VALUE );
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

HRESULT CMpcAudioRenderer::DoRenderSampleWasapi(IMediaSample *pMediaSample)
{
#if defined(_DEBUG) && DBGLOG_LEVEL > 1
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::DoRenderSampleWasapi()"));
#endif

	if (m_filterState == State_Stopped
			|| !m_hRenderThread
			|| m_eReinitialize.Check()) {
		return S_FALSE;
	}

	CheckBufferStatus();

	HANDLE handles[3] = {
		m_hStopRenderThreadEvent,
		m_hRendererNeedMoreData,
		m_hStopWaitingRenderer
	};

	DWORD result = WaitForMultipleObjects(3, handles, FALSE, INFINITE);
	if (result == WAIT_OBJECT_0) {
		// m_hStopRenderThreadEvent
		return S_FALSE;
	}

	if (m_pWaveFileFormat == NULL
			|| m_pWaveFileFormatOutput == NULL) {
		return S_FALSE;
	}

	m_eEndReceive.Reset();

	long lSize = pMediaSample->GetActualDataLength();
	if (!lSize) {
		return S_FALSE;
	}

	HRESULT	hr					= S_OK;
	BYTE *pMediaBuffer			= NULL;
	BYTE *pInputBufferPointer	= NULL;

	SampleFormat in_sf;
	DWORD in_layout;
	int   in_channels;
	int   in_samplerate;
	int   in_samples;

	SampleFormat out_sf;
	DWORD out_layout;
	int   out_channels;
	int   out_samplerate;

	BYTE* buff    = NULL;
	BYTE* out_buf = NULL;

	AM_MEDIA_TYPE *pmt;
	if (SUCCEEDED(pMediaSample->GetMediaType(&pmt)) && pmt != NULL) {
		CMediaType mt(*pmt);
		hr = CheckAudioClient((WAVEFORMATEX*)mt.Format());

		DeleteMediaType(pmt);
		if (FAILED(hr)) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 1
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::DoRenderSampleWasapi() - Error while checking audio client with input media type"));
#endif
			return hr;
		}
	}

	bool bFormatChanged = !m_bIsBitstream && IsFormatChanged(m_pWaveFileFormat, m_pWaveFileFormatOutput);

	if (bFormatChanged) {
		// prepare for resample ... if needed
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pWaveFileFormat;
		in_channels   = wfe->nChannels;
		in_samplerate = wfe->nSamplesPerSec;
		in_samples    = lSize / wfe->nBlockAlign;
		bool isfloat  = false;
		if (IsWaveFormatExtensible(wfe)) {
			WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)m_pWaveFileFormat;
			in_layout = wfex->dwChannelMask;
			isfloat   = !!(wfex->SubFormat == MEDIASUBTYPE_IEEE_FLOAT);
		} else {
			in_layout = GetDefChannelMask(wfe->nChannels);
			isfloat   = !!(wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
		}
		if (isfloat) {
			switch (wfe->wBitsPerSample) {
				case 32:
					in_sf = SAMPLE_FMT_FLT;
					break;
				case 64:
					in_sf = SAMPLE_FMT_DBL;
					break;
				default:
					return E_INVALIDARG;
			}
		} else {
			switch (wfe->wBitsPerSample) {
				case 8:
					in_sf = SAMPLE_FMT_U8;
					break;
				case 16:
					in_sf = SAMPLE_FMT_S16;
					break;
				case 24:
					in_sf = SAMPLE_FMT_S24;
					break;
				case 32:
					in_sf = SAMPLE_FMT_S32;
					break;
				default:
					return E_INVALIDARG;
			}
		}

		WAVEFORMATEX* wfeOutput = (WAVEFORMATEX*)m_pWaveFileFormatOutput;
		out_channels   = wfeOutput->nChannels;;
		out_samplerate = wfeOutput->nSamplesPerSec;
		isfloat        = false;
		if (IsWaveFormatExtensible(wfeOutput)) {
			WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)m_pWaveFileFormatOutput;
			out_layout = wfex->dwChannelMask;
			isfloat    = !!(wfex->SubFormat == MEDIASUBTYPE_IEEE_FLOAT);
		} else {
			out_layout = GetDefChannelMask(wfeOutput->nChannels);
			isfloat    = !!(wfeOutput->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
		}
		if (isfloat) {
			if (wfeOutput->wBitsPerSample == 32) {
				out_sf = SAMPLE_FMT_FLT;
			} else {
				return E_INVALIDARG;
			}
		} else {
			switch (wfeOutput->wBitsPerSample) {
				case 16:
					out_sf = SAMPLE_FMT_S16;
					break;
				case 24:
					out_sf = SAMPLE_FMT_S24;
					break;
				case 32:
					out_sf = SAMPLE_FMT_S32;
					break;
				default:
					return E_INVALIDARG;
			}
		}
	}

	hr = pMediaSample->GetPointer(&pMediaBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	if (bFormatChanged) {
		BYTE* in_buff = &pMediaBuffer[0];

		bool bUseMixer		= false;
		int out_samples		= in_samples;
		SampleFormat sfmt	= in_sf;

		if (in_layout != out_layout || in_samplerate != out_samplerate) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 2
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::DoRenderSampleWasapi() - use Mixer"));
			DbgLog((LOG_TRACE, 3, L"	input:"));
			DbgLog((LOG_TRACE, 3, L"		layout		= 0x%x", in_layout));
			DbgLog((LOG_TRACE, 3, L"		channels	= %d", in_channels));
			DbgLog((LOG_TRACE, 3, L"		samplerate	= %d", in_samplerate));

			DbgLog((LOG_TRACE, 3, L"	output:"));
			DbgLog((LOG_TRACE, 3, L"		layout		= 0x%x", out_layout));
			DbgLog((LOG_TRACE, 3, L"		channels	= %d", out_channels));
			DbgLog((LOG_TRACE, 3, L"		samplerate	= %d", out_samplerate));
#endif

			m_Resampler.UpdateInput(in_sf, in_layout, in_samplerate);
			m_Resampler.UpdateOutput(out_sf, out_layout, out_samplerate);
			out_samples = m_Resampler.CalcOutSamples(in_samples);
			buff        = DNew BYTE[out_samples * out_channels * get_bytes_per_sample(out_sf)];
			if (!buff) {
				return E_OUTOFMEMORY;
			}
			out_samples = m_Resampler.Mixing(buff, out_samples, in_buff, in_samples);
			bUseMixer   = true;
			sfmt        = out_sf;
		} else {
			buff = in_buff;
		}

		{
			WAVEFORMATEX* wfeOutput				= (WAVEFORMATEX*)m_pWaveFileFormatOutput;
			WAVEFORMATEXTENSIBLE* wfexOutput	= (WAVEFORMATEXTENSIBLE*)wfeOutput;

			WORD tag	= wfeOutput->wFormatTag;
			bool fPCM	= tag == WAVE_FORMAT_PCM || (tag == WAVE_FORMAT_EXTENSIBLE && wfexOutput->SubFormat == KSDATAFORMAT_SUBTYPE_PCM);
			bool fFloat	= tag == WAVE_FORMAT_IEEE_FLOAT || (tag == WAVE_FORMAT_EXTENSIBLE && wfexOutput->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

			if (fPCM) {
				if (wfeOutput->wBitsPerSample == 16) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 2
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::DoRenderSampleWasapi() - convert from '%s' to 16bit PCM", GetSampleFormatString(sfmt)));
#endif
					lSize	= out_samples * out_channels * sizeof(int16_t);
					out_buf	= DNew BYTE[lSize];
					convert_to_int16(sfmt, out_channels, out_samples, buff, (int16_t*)out_buf);
				} else if (wfeOutput->wBitsPerSample == 24) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 2
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::DoRenderSampleWasapi() - convert from '%s' to 24bit PCM", GetSampleFormatString(sfmt)));
#endif
					lSize	= out_samples * out_channels * sizeof(BYTE) * 3;
					out_buf	= DNew BYTE[lSize];
					convert_to_int24(sfmt, out_channels, out_samples, buff, out_buf);
				} else if (wfeOutput->wBitsPerSample == 32) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 2
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::DoRenderSampleWasapi() - convert from '%s' to 32bit PCM", GetSampleFormatString(sfmt)));
#endif
					lSize	= out_samples * out_channels * sizeof(int32_t);
					out_buf	= DNew BYTE[lSize];
					convert_to_int32(sfmt, out_channels, out_samples, buff, (int32_t*)out_buf);
				} else {
#if defined(_DEBUG) && DBGLOG_LEVEL > 2
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::DoRenderSampleWasapi() - unsupported format"));
#endif
				}
			} else if (fFloat) {
				if (wfeOutput->wBitsPerSample == 32) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 2
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::DoRenderSampleWasapi() - convert from '%s' to 32bit FLOAT", GetSampleFormatString(sfmt)));
#endif
					lSize	= out_samples * out_channels * sizeof(float);
					out_buf	= DNew BYTE[lSize];
					convert_to_float(sfmt, out_channels, out_samples, buff, (float*)out_buf);
				} else {
#if defined(_DEBUG) && DBGLOG_LEVEL > 2
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::DoRenderSampleWasapi() - unsupported format"));
#endif
				}
			}

			if (bUseMixer) {
				SAFE_DELETE_ARRAY(buff);
			}

			if (!out_buf) {
				return E_INVALIDARG;
			}

		}

		pInputBufferPointer	= &out_buf[0];
	} else {
		pInputBufferPointer	= &pMediaBuffer[0];
	}

	{
		CAutoLock cRenderLock(&m_csRender);

		REFERENCE_TIME rtStart, rtStop;
		if (FAILED(pMediaSample->GetTime(&rtStart, &rtStop))) {
			rtStart = rtStop = INVALID_TIME;
		}

		CAutoPtr<CPacket> p(DNew CPacket());
		p->rtStart	= rtStart;
		p->rtStop	= rtStop;
		p->SetData(pInputBufferPointer, lSize);

		if (m_bIsBitstream && m_BitstreamMode == BITSTREAM_NONE) {
			if (m_pWaveFileFormatOutput->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
				m_BitstreamMode = BITSTREAM_DTS;
				// AC3/DTS
				if (lSize > 8) {
					BYTE IEC61937_type = pInputBufferPointer[4];
					if (IEC61937_type == IEC61937_AC3) {
						m_BitstreamMode = BITSTREAM_AC3;
					}
				}
			} else if (IsWaveFormatExtensible(m_pWaveFileFormatOutput)) {
				WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE*)m_pWaveFileFormatOutput;
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
				&& (m_Filter.IsInitialized() || SUCCEEDED(m_Filter.Init(m_dRate, m_pWaveFileFormatOutput)) )) {
			if (SUCCEEDED(m_Filter.Push(p))) {
				while (SUCCEEDED(m_Filter.Pull(p))) {
					m_WasapiQueue.Add(p);
				}
			}
		} else {
			m_WasapiQueue.Add(p);
		}
	}

	SAFE_DELETE_ARRAY(out_buf);

	if (!m_bIsAudioClientStarted) {
		StartAudioClient();
	}

	return S_OK;
}
#pragma endregion

HRESULT CMpcAudioRenderer::CheckAudioClient(WAVEFORMATEX *pWaveFormatEx/* = NULL*/)
{
	HRESULT hr = S_OK;
	CAutoLock cAutoLock(&m_csCheck);
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient()"));
	if (m_pMMDevice == NULL) {
		hr = GetAudioDevice();
		if (FAILED(hr)) {
			return hr;
		}
	}

	// If no WAVEFORMATEX structure provided and client already exists, return it
	if (m_pAudioClient != NULL && pWaveFormatEx == NULL) {
		return hr;
	}

	// Just create the audio client if no WAVEFORMATEX provided
	if (m_pAudioClient == NULL && pWaveFormatEx == NULL) {
		if (SUCCEEDED(hr)) {
			hr = CreateAudioClient();
		}
		return hr;
	}

	BOOL bInitNeed = TRUE;

	// Compare the exisiting WAVEFORMATEX with the one provided
	if (CheckFormatChanged(pWaveFormatEx, &m_pWaveFileFormat) || !m_pWaveFileFormatOutput) {

		// Format has changed, audio client has to be reinitialized
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient() - Format changed, reinitialize the audio client"));

		CopyWaveFormat(m_pWaveFileFormat, &m_pWaveFileFormatOutput);

		WAVEFORMATEX *pFormat		= NULL;
		WAVEFORMATEX* pDeviceFormat	= NULL;

		WAVEFORMATEX *sharedClosestMatch = NULL;

		IPropertyStore *pProps = NULL;
		PROPVARIANT varConfig;

		WAVEFORMATEXTENSIBLE wfexAsIs = {0};

		if (m_WASAPIMode == MODE_WASAPI_EXCLUSIVE || IsBitstream(pWaveFormatEx)) { // EXCLUSIVE/BITSTREAM
			if (IsBitstream(pWaveFormatEx)) {
				pFormat = pWaveFormatEx;
			} else {
				if (m_bUseBitExactOutput) {
					if (!m_bUseSystemLayoutChannels
							&& SUCCEEDED(SelectFormat(pWaveFormatEx, wfexAsIs))) {
						StopAudioClient();
						SAFE_RELEASE(m_pRenderClient);
						SAFE_RELEASE(m_pAudioClient);
						hr = CreateAudioClient();
						if (SUCCEEDED(hr)) {
							WAVEFORMATEXTENSIBLE wfex = { 0 };
							DWORD dwChannelMask	= GetDefChannelMask(pWaveFormatEx->nChannels);
							if (IsWaveFormatExtensible(pWaveFormatEx)) {
								dwChannelMask	= ((WAVEFORMATEXTENSIBLE*)pWaveFormatEx)->dwChannelMask;
							}
							CreateFormat(wfex, wfexAsIs.Format.wBitsPerSample, pWaveFormatEx->nChannels, dwChannelMask, pWaveFormatEx->nSamplesPerSec);
							hr = InitAudioClient((WAVEFORMATEX*)&wfex, FALSE);
							if (S_OK == hr) {
								CopyWaveFormat((WAVEFORMATEX*)&wfex, &m_pWaveFileFormatOutput);
								bInitNeed = FALSE;
							}
						}
					}

					if(bInitNeed
							&& SUCCEEDED(SelectFormat(pWaveFormatEx, wfexAsIs))) {
						hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfexAsIs, NULL);
						if (S_OK == hr) {
							pFormat = (WAVEFORMATEX*)&wfexAsIs;
						}
					}
				}

				if (!pFormat && bInitNeed) {
					m_pMMDevice->OpenPropertyStore(STGM_READ, &pProps);
					PropVariantInit(&varConfig);
					hr = pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &varConfig);
					if (SUCCEEDED(hr) && varConfig.vt == VT_BLOB && varConfig.blob.pBlobData != NULL) {
						pFormat = (WAVEFORMATEX*)varConfig.blob.pBlobData;
					}
				}
			}

			if (bInitNeed) {
				hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pFormat, NULL);
				if (S_OK == hr) {
					CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
				}
			}
		} else if (m_WASAPIMode == MODE_WASAPI_SHARED) { // SHARED
			pFormat = pWaveFormatEx;
			hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
			if (FAILED(hr)) {
				hr = m_pAudioClient->GetMixFormat(&pDeviceFormat);
				if (FAILED(hr)) {
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient() - GetMixFormat() failed"));
					return hr;
				}

				pFormat = pDeviceFormat;
				hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat, &sharedClosestMatch);
				if (S_OK == hr) {
					CopyWaveFormat(pFormat, &m_pWaveFileFormatOutput);
				}
			}
		}

		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient() - IsFormatSupported()"));
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
		DbgLog((LOG_TRACE, 3, L"	Input format:"));
		DumpWaveFormatEx(pWaveFormatEx);

		DbgLog((LOG_TRACE, 3, L"	Output format:"));
		DumpWaveFormatEx(m_pWaveFileFormatOutput);
#endif

		if (pProps) {
			PropVariantClear(&varConfig);
			SAFE_RELEASE(pProps);
		}

		if (pDeviceFormat) {
			CoTaskMemFree(pDeviceFormat);
		}

		if (S_OK == hr) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient() - WASAPI client accepted the format"));
			if (bInitNeed) {
				StopAudioClient();
				SAFE_RELEASE(m_pRenderClient);
				SAFE_RELEASE(m_pAudioClient);
				hr = CreateAudioClient();
			}
		} else if (S_FALSE == hr) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient() - WASAPI client refused the format with a closest match"));
#if defined(_DEBUG) && DBGLOG_LEVEL > 0
			DbgLog((LOG_TRACE, 3, L"	=> ppClosestMatch:"));
			DumpWaveFormatEx(sharedClosestMatch);
#endif

			CopyWaveFormat(sharedClosestMatch, &m_pWaveFileFormatOutput);
			CoTaskMemFree(sharedClosestMatch);

			hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, m_pWaveFileFormatOutput, &sharedClosestMatch);
			if (S_OK == hr) {
				DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient() - WASAPI client accepted the closest match format"));
				StopAudioClient();
				SAFE_RELEASE(m_pRenderClient);
				SAFE_RELEASE(m_pAudioClient);
				hr = CreateAudioClient();
			}

			if (hr != S_OK) {
				if (sharedClosestMatch) {
					CoTaskMemFree(sharedClosestMatch);
				}
				SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);

				return hr;
			}
		} else if (AUDCLNT_E_UNSUPPORTED_FORMAT == hr) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient() - WASAPI client refused the format"));
			SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);
			return hr;
		} else {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient() - WASAPI failed = 0x%08x", hr));
			SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);
			return hr;
		}
	} else if (m_pRenderClient == NULL) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CheckAudioClient() - First initialization of the audio renderer"));
	} else {
		return hr;
	}

	if (SUCCEEDED(hr) && bInitNeed) {
		SAFE_RELEASE(m_pRenderClient);
		hr = InitAudioClient(m_pWaveFileFormatOutput);
	}

	if (SUCCEEDED(hr)) {
		m_Filter.Flush();
	}

	return hr;
}

HRESULT CMpcAudioRenderer::GetAudioDevice()
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - Target end point: '%s'", m_DeviceName));

	m_bUseDefaultDevice = FALSE;
	m_strDeviceName.Empty();

	CComPtr<IMMDeviceEnumerator> enumerator;
	HRESULT hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
	if (hr != S_OK) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - failed to create MMDeviceEnumerator: (0x%08x)", hr));
		return hr;
	}

	IMMDeviceCollection* devices;
	hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);
	if (hr != S_OK) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - enumerator->EnumAudioEndpoints() failed: (0x%08x)", hr));
		return hr;
	}

	UINT count = 0;
	hr = devices->GetCount(&count);
	if (hr != S_OK) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - devices->GetCount() failed: (0x%08x)", hr));
		SAFE_RELEASE(devices);
		return hr;
	}

	if (hr == S_OK && devices) {
		IPropertyStore* pProps = NULL;

		for (UINT i = 0 ; i < count ; i++) {
			LPWSTR pwszID = NULL;
			IMMDevice *endpoint = NULL;
			hr = devices->Item(i, &endpoint);
			if (hr == S_OK) {
				hr = endpoint->GetId(&pwszID);
				if (hr == S_OK) {
					if (endpoint->OpenPropertyStore(STGM_READ, &pProps) == S_OK) {
						PROPVARIANT varName;
						PropVariantInit(&varName);

						// Found the configured audio endpoint
						if ((pProps->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK) && (m_DeviceName == varName.pwszVal)) {
							DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - devices->GetId() OK, num: (%d), pwszVal: '%s', pwszID: '%s'", i, varName.pwszVal, pwszID));

							m_strDeviceName = varName.pwszVal;
							if (m_strDeviceId.IsEmpty()) {
								m_strDeviceId = pwszID;
							}

							enumerator->GetDevice(pwszID, &m_pMMDevice);
							SAFE_RELEASE(devices);
							m_pMMDevice = endpoint;
							CoTaskMemFree(pwszID);
							pwszID = NULL;
							PropVariantClear(&varName);
							SAFE_RELEASE(pProps);
							return S_OK;
						} else {
							PropVariantClear(&varName);
							SAFE_RELEASE(pProps);
							SAFE_RELEASE(endpoint);
							CoTaskMemFree(pwszID);
							pwszID = NULL;
						}
					}
				} else {
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - devices->GetId() failed: (0x%08x)", hr));
				}
			} else {
				DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - devices->Item() failed: (0x%08x)", hr));
			}

			CoTaskMemFree(pwszID);
			pwszID = NULL;
		}
	}

	SAFE_RELEASE(devices);
	m_bUseDefaultDevice = TRUE;

	hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pMMDevice);
	if (hr == S_OK) {
		LPWSTR pwszID = NULL;
		hr = m_pMMDevice->GetId(&pwszID);
		if (hr == S_OK) {
			if (m_strDeviceId.IsEmpty()) {
				m_strDeviceId = pwszID;
			}

			IPropertyStore* pProps = NULL;
			if (m_pMMDevice->OpenPropertyStore(STGM_READ, &pProps) == S_OK) {
				PROPVARIANT varName;
				PropVariantInit(&varName);
				if ((pProps->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK)) {
					DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetAudioDevice() - Unable to find selected audio device, using the default end point : pwszVal: '%s', pwszID: '%s'", varName.pwszVal, pwszID));
					m_strDeviceName = varName.pwszVal;
				}

				PropVariantClear(&varName);
				SAFE_RELEASE(pProps);
			}
		}

		CoTaskMemFree(pwszID);
	}

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

	WAVEFORMATEXTENSIBLE* wfex		= NULL;
	WAVEFORMATEXTENSIBLE* wfexNew	= NULL;
	if (IsWaveFormatExtensible(pWaveFormatEx)) {
		wfex = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;
	}
	if (IsWaveFormatExtensible(pNewWaveFormatEx)) {
		wfexNew	= (WAVEFORMATEXTENSIBLE*)pNewWaveFormatEx;
	}

	if (wfex && wfexNew &&
			(wfex->SubFormat != wfexNew->SubFormat
			|| wfex->dwChannelMask != wfexNew->dwChannelMask
			/*|| wfex->Samples.wValidBitsPerSample != wfexNew->Samples.wValidBitsPerSample*/)) {

		if (wfex->SubFormat == wfexNew->SubFormat
				&& (wfex->dwChannelMask == KSAUDIO_SPEAKER_5POINT1 && wfexNew->dwChannelMask == KSAUDIO_SPEAKER_5POINT1_SURROUND)) {
			return false;
		}
		return true;
	}

	return false;
}

bool CMpcAudioRenderer::CheckFormatChanged(WAVEFORMATEX *pWaveFormatEx, WAVEFORMATEX **ppNewWaveFormatEx)
{
	bool formatChanged = false;
	if (m_pWaveFileFormat == NULL) {
		formatChanged = true;
	} else {
		formatChanged = IsFormatChanged(pWaveFormatEx, m_pWaveFileFormat);
	}

	if (!formatChanged) {
		return false;
	}

	return CopyWaveFormat(pWaveFormatEx, ppNewWaveFormatEx);
}

bool CMpcAudioRenderer::CopyWaveFormat(WAVEFORMATEX *pSrcWaveFormatEx, WAVEFORMATEX **ppDestWaveFormatEx)
{
	CheckPointer(pSrcWaveFormatEx, false);

	SAFE_DELETE_ARRAY(*ppDestWaveFormatEx);

	size_t size = sizeof(WAVEFORMATEX) + pSrcWaveFormatEx->cbSize; // Always true, even for WAVEFORMATEXTENSIBLE and WAVEFORMATEXTENSIBLE_IEC61937
	*ppDestWaveFormatEx = (WAVEFORMATEX *)DNew BYTE[size];
	if (!(*ppDestWaveFormatEx)) {
		return false;
	}
	memcpy(*ppDestWaveFormatEx, pSrcWaveFormatEx, size);

	return true;
}

BOOL CMpcAudioRenderer::IsBitstream(WAVEFORMATEX *pWaveFormatEx)
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

HRESULT CMpcAudioRenderer::SelectFormat(WAVEFORMATEX* pwfx, WAVEFORMATEXTENSIBLE& wfex)
{
	// first - check variables ...
	if (m_wBitsPerSampleList.GetSize() == 0
			|| m_AudioParamsList.GetSize() == 0
			|| m_nChannelsList.GetSize() == 0) {
		return E_FAIL;
	}

	// trying to create an output media type similar to input media type ...

	DWORD nSamplesPerSec	= pwfx->nSamplesPerSec;
	BOOL bExists			= FALSE;
	for (int i = 0; i < m_AudioParamsList.GetSize(); i++) {
		if (m_AudioParamsList[i].nSamplesPerSec == nSamplesPerSec) {
			bExists			= TRUE;
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
		nSamplesPerSec		= FindClosestInArray(array, nSamplesPerSec);
	}

	WORD wBitsPerSample		= pwfx->wBitsPerSample;
	AudioParams ap(wBitsPerSample, nSamplesPerSec);
	if (m_AudioParamsList.Find(ap) == -1) {
		wBitsPerSample		= 0;
		for (int i = m_AudioParamsList.GetSize() - 1; i >= 0; i--) {
			if (m_AudioParamsList[i].nSamplesPerSec == nSamplesPerSec) {
				wBitsPerSample = m_AudioParamsList[i].wBitsPerSample;
				break;
			}
		}
	}

	WORD nChannels			= 0;
	DWORD dwChannelMask		= 0;

	if (m_bUseSystemLayoutChannels) {
		// to get the number of channels and channel mask quite simple call IAudioClient::GetMixFormat()
		WAVEFORMATEX *pDeviceFormat = NULL;
		if (SUCCEEDED(m_pAudioClient->GetMixFormat(&pDeviceFormat)) && pDeviceFormat) {
			nChannels		= pDeviceFormat->nChannels;
			dwChannelMask	= ((WAVEFORMATEXTENSIBLE*)pDeviceFormat)->dwChannelMask;

			CoTaskMemFree(pDeviceFormat);
		}
	}

	if (!nChannels) {
		nChannels			= pwfx->nChannels;
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

		dwChannelMask		= GetDefChannelMask(nChannels);
		if (IsWaveFormatExtensible(pwfx)) {
			dwChannelMask	= ((WAVEFORMATEXTENSIBLE*)pwfx)->dwChannelMask;
		}

		if (m_nChannelsList.Find(nChannels) == -1) {
			nChannels			= m_nChannelsList[m_nChannelsList.GetSize() - 1];
			dwChannelMask		= m_dwChannelMaskList[m_nChannelsList.Find(nChannels)];
		}

		if (m_dwChannelMaskList.Find(dwChannelMask) == -1) {
			dwChannelMask		= m_dwChannelMaskList[m_nChannelsList.Find(nChannels)];
		}
	}

	WAVEFORMATEX& wfe		= wfex.Format;
	wfe.nChannels			= nChannels;
	wfe.nSamplesPerSec		= nSamplesPerSec;
	wfe.wBitsPerSample		= wBitsPerSample;
	wfe.nBlockAlign			= nChannels * wfe.wBitsPerSample / 8;
	wfe.nAvgBytesPerSec		= nSamplesPerSec * wfe.nBlockAlign;

	wfex.Format.wFormatTag					= WAVE_FORMAT_EXTENSIBLE;
	wfex.Format.cbSize						= sizeof(wfex) - sizeof(wfex.Format);
	wfex.SubFormat							= MEDIASUBTYPE_PCM;
	wfex.dwChannelMask						= dwChannelMask;
	wfex.Samples.wValidBitsPerSample		= wfex.Format.wBitsPerSample;
	if (wfex.Samples.wValidBitsPerSample == 32 && wfe.wBitsPerSample == 32) {
		wfex.Samples.wValidBitsPerSample	= 24;
	}

	return S_OK;
}

void CMpcAudioRenderer::CreateFormat(WAVEFORMATEXTENSIBLE& wfex, WORD wBitsPerSample, WORD nChannels, DWORD dwChannelMask, DWORD nSamplesPerSec)
{
	memset(&wfex, 0, sizeof(wfex));

	WAVEFORMATEX& wfe		= wfex.Format;
	wfe.nChannels			= nChannels;
	wfe.nSamplesPerSec		= nSamplesPerSec;
	wfe.wBitsPerSample		= wBitsPerSample;

	wfe.nBlockAlign			= nChannels * wfe.wBitsPerSample / 8;
	wfe.nAvgBytesPerSec		= nSamplesPerSec * wfe.nBlockAlign;

	wfex.Format.wFormatTag					= WAVE_FORMAT_EXTENSIBLE;
	wfex.Format.cbSize						= sizeof(wfex) - sizeof(wfex.Format);
	wfex.Samples.wValidBitsPerSample		= wfex.Format.wBitsPerSample;
	if (wfex.Samples.wValidBitsPerSample == 32 && wfe.wBitsPerSample == 32) {
		wfex.Samples.wValidBitsPerSample	= 24;
	}
	wfex.dwChannelMask						= dwChannelMask;
	wfex.SubFormat							= MEDIASUBTYPE_PCM;
}

HRESULT CMpcAudioRenderer::StartAudioClient()
{
	if (m_eReinitialize.Check()) {
		return S_OK;
	}

	HRESULT hr = S_OK;

	if (!m_bIsAudioClientStarted && m_pAudioClient) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::StartAudioClient()"));

		// To reduce latency, load the first buffer with data
		// from the audio source before starting the stream.
		RenderWasapiBuffer();

		if (FAILED(hr = m_pAudioClient->Start())) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::StartAudioClient() - start audio client failed"));
			return hr;
		}
		m_bIsAudioClientStarted = true;
	} else {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::StartAudioClient() - already started"));
	}

	return hr;
}

HRESULT CMpcAudioRenderer::StopAudioClient()
{
	HRESULT hr = S_OK;

	if (m_bIsAudioClientStarted) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::StopAudioClient"));

		m_bIsAudioClientStarted = false;

		PauseRendererThread();

		if (m_pAudioClient) {
			if (FAILED(hr = m_pAudioClient->Stop())) {
				DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::StopAudioClient() - stopp audio client failed"));
				DbgLog((LOG_TRACE, 3, L"Stopping audio client failed"));
				return hr;
			}
			if (FAILED(hr = m_pAudioClient->Reset())) {
				DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::StopAudioClient() - reset audio client failed"));
				return hr;
			}
		}
	}

	return hr;
}

HRESULT CMpcAudioRenderer::ReinitializeAudioDevice()
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::ReinitializeAudioDevice()"));

	m_eReinitialize.Set();
	m_eEndReceive.Wait();

	HRESULT hr = E_FAIL;

	m_strDeviceName.Empty();

	if (m_bIsAudioClientStarted && m_pAudioClient) {
		m_pAudioClient->Stop();
	}
	m_bIsAudioClientStarted = false;

	SAFE_RELEASE(m_pRenderClient);
	SAFE_RELEASE(m_pAudioClient);
	SAFE_RELEASE(m_pMMDevice);

	SAFE_DELETE_ARRAY(m_pWaveFileFormat);
	SAFE_DELETE_ARRAY(m_pWaveFileFormatOutput);

	CMediaType mt;
	if (SUCCEEDED(m_pInputPin->ConnectionMediaType(&mt)) && mt.pbFormat) {
		hr = CheckMediaType(&mt);
		if (SUCCEEDED(hr)) {
			hr = SetMediaType(&mt);
		} else {
			IPin* pPin = m_pInputPin->GetConnected();
			if (pPin) {
				CComQIPtr<IMediaControl> pMC(m_pGraph);
				OAFilterState fs = -1;
				if (pMC) {
					pMC->GetState(100, &fs);
					pMC->Stop();
				}

				BeginEnumMediaTypes(pPin, pEMT, pmt) {
					if (SUCCEEDED(pPin->QueryAccept(pmt))) {
						mt = *pmt;
						hr = CheckMediaType(&mt);
						if (SUCCEEDED(hr)) {
							hr = ReconnectPin(pPin, pmt);
							if (SUCCEEDED(hr)) {
								break;
							}
						}
					}
				}
				EndEnumMediaTypes(pmt)

				if (pMC && fs != State_Stopped) {
					if(fs == State_Running) {
						pMC->Run();
					} else if (fs == State_Paused) {
						pMC->Pause();
					}
				}
			}
		}
	}

	if (FAILED(hr)) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::ReinitializeAudioDevice() failed: (0x%08x)", hr));
	}

	m_eReinitialize.Reset();
	return hr;
}

#define GetDuration(size, wfex) (size / wfex->nBlockAlign * UNITS / wfex->nSamplesPerSec)
HRESULT CMpcAudioRenderer::RenderWasapiBuffer()
{
	CheckPointer(m_pRenderClient, S_OK);
	CheckPointer(m_pWaveFileFormatOutput, S_OK);

	HRESULT hr = S_OK;

	UINT32 numFramesPadding = 0;
	if (m_WASAPIMode == MODE_WASAPI_SHARED && !m_bIsBitstream) { // SHARED
		m_pAudioClient->GetCurrentPadding(&numFramesPadding);
		if (FAILED(hr)) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 1
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderWasapiBuffer() - GetCurrentPadding() failed (0x%08x)", hr));
#endif
			return hr;
		}
	}

	UINT32 numFramesAvailable	= m_nFramesInBuffer - numFramesPadding;
	UINT32 nAvailableBytes		= numFramesAvailable * m_pWaveFileFormatOutput->nBlockAlign;

	BYTE* pData = NULL;
	hr = m_pRenderClient->GetBuffer(numFramesAvailable, &pData);
	if (FAILED(hr)) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 1
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderWasapiBuffer() - GetBuffer() failed with size %ld (0x%08x)", numFramesAvailable, hr));
#endif
		return hr;
	}

	size_t WasapiBufLen = 0;
	{
		CAutoLock cRenderLock(&m_csRender);
		WasapiBufLen = m_WasapiQueue.GetSize() + (m_CurrentPacket ? m_CurrentPacket->GetCount() : 0);
	}

	DWORD bufferFlags = 0;
	if (!pData || !WasapiBufLen || (m_filterState != State_Running && !m_bEndOfStream)) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 1
		if (m_filterState != State_Running) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderWasapiBuffer() - not running"));
		} else if (!WasapiBufLen) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderWasapiBuffer() - data queue is empty"));
		}
#endif
		bufferFlags = AUDCLNT_BUFFERFLAGS_SILENT;
	} else {
#if defined(_DEBUG) && DBGLOG_LEVEL > 1
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderWasapiBuffer() - requested: %u[%u], available: %u", nAvailableBytes, numFramesAvailable, WasapiBufLen));
#endif
		{
			CAutoLock cRenderLock(&m_csRender);

			REFERENCE_TIME rtRefClock = INVALID_TIME;
			UINT32 nWritenBytes = 0;
			do {
				if (!m_CurrentPacket && m_WasapiQueue.GetCount()) {
					m_CurrentPacket = m_WasapiQueue.Remove();
				}
				if (!m_CurrentPacket) {
					break;
				}

				REFERENCE_TIME rWaitRenderTime = INVALID_TIME;

				rtRefClock = GetRefClockTime();
				if (rtRefClock != INVALID_TIME) {
					if (m_nSampleNum == 0) {
						m_rtNextSampleTime = m_CurrentPacket->rtStart;
					}

					if (m_CurrentPacket->rtStart < rtRefClock - m_hnsPeriod) {
						m_nSampleNum = 0;
						m_CurrentPacket.Free();
						continue;
					}

					BOOL bReSync = FALSE;
					if (abs(m_CurrentPacket->rtStart - m_rtNextSampleTime) > 20000) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 2
						DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderWasapiBuffer() - Discontinuity detected : %u[%u], %I64d -> %I64d(%s -> %s), %I64d)",
								nWritenBytes, nAvailableBytes,
								m_rtNextSampleTime, m_CurrentPacket->rtStart,
								ReftimeToString(m_rtNextSampleTime), ReftimeToString(m_CurrentPacket->rtStart),
								m_CurrentPacket->rtStart - m_rtNextSampleTime));
#endif

						bReSync = TRUE;
					}

					REFERENCE_TIME rtDuration = GetDuration(m_CurrentPacket->GetCount(), m_pWaveFileFormatOutput);
					m_rtNextSampleTime = m_CurrentPacket->rtStart + rtDuration;

					if ((m_nSampleNum == 0 && m_CurrentPacket->rtStart > rtRefClock) || bReSync) {
						rWaitRenderTime = m_CurrentPacket->rtStart;
					}
				}

				rtRefClock = GetRefClockTime();
				if (rWaitRenderTime != INVALID_TIME
						&& rWaitRenderTime > rtRefClock) {
					REFERENCE_TIME rtSilenceDuration = rWaitRenderTime - rtRefClock;
					UINT32 nSilenceFrames = rtSilenceDuration / (UNITS / m_pWaveFileFormatOutput->nSamplesPerSec);
					UINT32 nSilenceBytes = min(nSilenceFrames * m_pWaveFileFormatOutput->nBlockAlign, nAvailableBytes - nWritenBytes);
					if (nSilenceBytes == (nAvailableBytes - nWritenBytes)) {
						bufferFlags = AUDCLNT_BUFFERFLAGS_SILENT;
						break;
					} else {
						memset(pData, 0, nSilenceBytes);
					}
					nWritenBytes += nSilenceBytes;
				} else {
					UINT32 nFilledBytes = min(m_CurrentPacket->GetCount(), nAvailableBytes - nWritenBytes);
					memcpy(&pData[nWritenBytes], m_CurrentPacket->GetData(), nFilledBytes);
					m_CurrentPacket->RemoveAt(0, nFilledBytes);
					nWritenBytes += nFilledBytes;

					if (m_CurrentPacket->IsEmpty()) {
						m_CurrentPacket.Free();
					} else if (m_CurrentPacket->rtStart != INVALID_TIME) {
						REFERENCE_TIME rtDuration = GetDuration(nFilledBytes, m_pWaveFileFormatOutput);
						m_CurrentPacket->rtStart += rtDuration;
					}
				}

				m_nSampleNum++;
			} while (nWritenBytes < nAvailableBytes && bufferFlags != AUDCLNT_BUFFERFLAGS_SILENT);

			if (bufferFlags != AUDCLNT_BUFFERFLAGS_SILENT) {
				if (!nWritenBytes) {
					bufferFlags = AUDCLNT_BUFFERFLAGS_SILENT;
				} else if (nWritenBytes < nAvailableBytes) {
					memset(&pData[nWritenBytes], 0, nAvailableBytes - nWritenBytes);
				}
			}
		}

		if (m_lVolume <= DSBVOLUME_MIN) {
			bufferFlags = AUDCLNT_BUFFERFLAGS_SILENT;
		}
		else if (m_lVolume < DSBVOLUME_MAX && !IsBitstream(m_pWaveFileFormat)) {
			// Adjusting volume ...
			WAVEFORMATEX* wfeOutput = (WAVEFORMATEX*)m_pWaveFileFormatOutput;
			SampleFormat sf = GetSampleFormat(wfeOutput);
			size_t samples = nAvailableBytes / (wfeOutput->wBitsPerSample >> 3);

			switch (sf) {
			case SAMPLE_FMT_U8:
				gain_uint8(m_dVolumeFactor, samples, (uint8_t*)pData);
				break;
			case SAMPLE_FMT_S16:
				gain_int16(m_dVolumeFactor, samples, (int16_t*)pData);
				break;
			case SAMPLE_FMT_S24:
				gain_int24(m_dVolumeFactor, samples, pData);
				break;
			case SAMPLE_FMT_S32:
				gain_int32(m_dVolumeFactor, samples, (int32_t*)pData);
				break;
			case SAMPLE_FMT_FLT:
				gain_float(m_dVolumeFactor, samples, (float*)pData);
				break;
			case SAMPLE_FMT_DBL:
				gain_double(m_dVolumeFactor, samples, (double*)pData);
				break;
			}
		}
	}

	hr = m_pRenderClient->ReleaseBuffer(numFramesAvailable, bufferFlags);
	if (FAILED(hr)) {
#if defined(_DEBUG) && DBGLOG_LEVEL > 1
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::RenderWasapiBuffer() - ReleaseBuffer() failed with size %ld (0x%08x)", numFramesAvailable, hr));
#endif
	}

	return hr;
}

void CMpcAudioRenderer::CheckBufferStatus()
{
	if (!m_pWaveFileFormatOutput) {
		return;
	}

	UINT32 nAvailableBytes = m_nFramesInBuffer * m_pWaveFileFormatOutput->nBlockAlign;
	size_t WasapiBufLen = 0;
	{
		CAutoLock cRenderLock(&m_csRender);
		WasapiBufLen = m_WasapiQueue.GetSize() + (m_CurrentPacket ? m_CurrentPacket->GetCount() : 0);
	}

	if (m_bEndOfStream && !WasapiBufLen) {
		m_bEndOfStream = FALSE;
	}

	if (WasapiBufLen < (nAvailableBytes * 10)) {
		SetEvent(m_hRendererNeedMoreData);
	} else {
		ResetEvent(m_hRendererNeedMoreData);
	}
}

HRESULT CMpcAudioRenderer::GetBufferSize(WAVEFORMATEX *pWaveFormatEx, REFERENCE_TIME *pHnsBufferPeriod)
{
	CheckPointer(pWaveFormatEx, S_OK);

	UINT32 nBufferSize = 0;

	if (pWaveFormatEx->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
		if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
			nBufferSize = 6144;
		} else {
			return S_OK;
		}
	} else {
		WAVEFORMATEXTENSIBLE *wfext = (WAVEFORMATEXTENSIBLE*)pWaveFormatEx;

		if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
			nBufferSize = 61440;
		} else if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD) {
			nBufferSize = 32768;
		} else if (wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS) {
			nBufferSize = 24576;
		} else {
			return S_OK;
		}
	}

	*pHnsBufferPeriod = (REFERENCE_TIME)((REFERENCE_TIME)nBufferSize * 10000 * 8 / ((REFERENCE_TIME)pWaveFormatEx->nChannels * pWaveFormatEx->wBitsPerSample *
						1.0 * pWaveFormatEx->nSamplesPerSec) /*+ 0.5*/);
	*pHnsBufferPeriod *= 1000;

	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::GetBufferSize() - set a %I64u period for a %u buffer size", *pHnsBufferPeriod, nBufferSize));

	return S_OK;
}

HRESULT CMpcAudioRenderer::InitAudioClient(WAVEFORMATEX *pWaveFormatEx, BOOL bCheckFormat/* = TRUE*/)
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient()"));
	HRESULT hr = S_OK;

	m_hnsPeriod = 0;
	m_BitstreamMode = BITSTREAM_NONE;

	REFERENCE_TIME hnsDefaultDevicePeriod = 0;
	REFERENCE_TIME hnsMinimumDevicePeriod = 0;
	hr = m_pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, &hnsMinimumDevicePeriod);
	if (SUCCEEDED(hr)) {
		m_hnsPeriod = hnsDefaultDevicePeriod;
	}
	m_hnsPeriod = max(500000, m_hnsPeriod); // 50 ms - minimal size of buffer

	if (bCheckFormat) {
		WAVEFORMATEX *pClosestMatch = NULL;
		hr = m_pAudioClient->IsFormatSupported((m_WASAPIMode == MODE_WASAPI_EXCLUSIVE || IsBitstream(pWaveFormatEx)) ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
												pWaveFormatEx,
												&pClosestMatch);
		if (pClosestMatch) {
			CoTaskMemFree(pClosestMatch);
		}

		if (S_OK == hr) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - WASAPI client accepted the format"));
		} else if (S_FALSE == hr) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - WASAPI client refused the format with a closest match"));
			return hr;
		} else if (AUDCLNT_E_UNSUPPORTED_FORMAT == hr) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - WASAPI client refused the format"));
			return hr;
		} else {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - WASAPI failed = 0x%08x", hr));
			return hr;
		}
	}

	GetBufferSize(pWaveFormatEx, &m_hnsPeriod);

	if (SUCCEEDED(hr)) {
		hr = m_pAudioClient->Initialize((m_WASAPIMode == MODE_WASAPI_EXCLUSIVE || IsBitstream(pWaveFormatEx)) ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
										 AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
										 m_hnsPeriod,
										 (m_WASAPIMode == MODE_WASAPI_EXCLUSIVE || IsBitstream(pWaveFormatEx)) ? m_hnsPeriod : 0,
										 pWaveFormatEx, NULL);
	}

	if (FAILED(hr) && hr != AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - FAILED(0x%08x)", hr));
		return hr;
	}

	if (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED == hr) {
		// if the buffer size was not aligned, need to do the alignment dance
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - Buffer size not aligned. Realigning"));

		// get the buffer size, which will be aligned
		hr = m_pAudioClient->GetBufferSize(&m_nFramesInBuffer);

		// throw away this IAudioClient
		StopAudioClient();
		SAFE_RELEASE(m_pRenderClient);
		SAFE_RELEASE(m_pAudioClient);
		if (SUCCEEDED(hr)) {
			hr = CreateAudioClient();
		}

		// calculate the new aligned periodicity
		m_hnsPeriod = // hns =
			(REFERENCE_TIME)(
				10000.0 * // (hns / ms) *
				1000 * // (ms / s) *
				m_nFramesInBuffer / // frames /
				pWaveFormatEx->nSamplesPerSec  // (frames / s)
				+ 0.5 // rounding
			);

		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - Trying again with periodicity of %I64u hundred-nanoseconds, or %u frames.", m_hnsPeriod, m_nFramesInBuffer));
		if (SUCCEEDED(hr)) {
			hr = m_pAudioClient->Initialize((m_WASAPIMode == MODE_WASAPI_EXCLUSIVE || IsBitstream(pWaveFormatEx)) ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
											 AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
											 m_hnsPeriod,
											 (m_WASAPIMode == MODE_WASAPI_EXCLUSIVE || IsBitstream(pWaveFormatEx)) ? m_hnsPeriod : 0,
											 pWaveFormatEx, NULL);
		}

		if (FAILED(hr)) {
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - Failed to reinitialize the audio client"));
			return hr;
		}
	}

	EXIT_ON_ERROR(hr);

	// get the buffer size, which is aligned
	hr = m_pAudioClient->GetBufferSize(&m_nFramesInBuffer);
	EXIT_ON_ERROR(hr);

	// enables a client to write output data to a rendering endpoint buffer
	hr = m_pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)(&m_pRenderClient));

	if (FAILED(hr)) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - service initialization FAILED(0x%08x)", hr));
	} else {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::InitAudioClient() - service initialization success, with periodicity of %I64u hundred-nanoseconds = %u frames", m_hnsPeriod, m_nFramesInBuffer));
		m_bIsBitstream = IsBitstream(pWaveFormatEx);
	}
	EXIT_ON_ERROR(hr);

	WasapiFlush();
	hr = m_pAudioClient->SetEventHandle(m_hDataEvent);
	EXIT_ON_ERROR(hr);

	hr = StartRendererThread();
	EXIT_ON_ERROR(hr);

	return hr;
}

HRESULT CMpcAudioRenderer::CreateAudioClient()
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CreateAudioClient()"));

	HRESULT hr	= S_OK;
	m_hnsPeriod	= 0;

	if (m_pMMDevice == NULL) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CreateAudioClient() - failed, device not loaded"));
		return E_FAIL;
	}

	if (m_pAudioClient) {
		StopAudioClient();
		SAFE_RELEASE(m_pAudioClient);
	}

	hr = m_pMMDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&m_pAudioClient));
	if (FAILED(hr)) {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CreateAudioClient() - activation FAILED(0x%08x)", hr));
	} else {
		DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::CreateAudioClient() - success"));
		if (!m_wBitsPerSampleList.GetSize()) {
			// get list of supported output formats - wBitsPerSample, nChannels(dwChannelMask), nSamplesPerSec
			WORD wBitsPerSampleValues[]		= {16, 24, 32};
			DWORD nSamplesPerSecValues[]	= {44100, 48000, 88200, 96000, 176400, 192000};
			WORD nChannelsValues[]			= {2, 4, 4, 6, 6, 8, 8};
			DWORD dwChannelMaskValues[]		= {
				KSAUDIO_SPEAKER_STEREO,
				KSAUDIO_SPEAKER_QUAD, KSAUDIO_SPEAKER_SURROUND,
				KSAUDIO_SPEAKER_5POINT1_SURROUND, KSAUDIO_SPEAKER_5POINT1,
				KSAUDIO_SPEAKER_7POINT1_SURROUND, KSAUDIO_SPEAKER_7POINT1
			};

			WAVEFORMATEXTENSIBLE wfex;

			// 1 - wBitsPerSample
			for (int i = 0; i < _countof(wBitsPerSampleValues); i++) {
				for (int k = 0; k < _countof(nSamplesPerSecValues); k++) {
					CreateFormat(wfex, wBitsPerSampleValues[i], 2, KSAUDIO_SPEAKER_STEREO, nSamplesPerSecValues[k]);
					if (S_OK == m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfex, NULL)
							&& m_wBitsPerSampleList.Find(wBitsPerSampleValues[i]) == -1) {
						m_wBitsPerSampleList.Add(wBitsPerSampleValues[i]);
					}
				}
			}
			if (m_wBitsPerSampleList.GetSize() == 0) {
				m_wBitsPerSampleList.RemoveAll();
				m_nChannelsList.RemoveAll();
				m_dwChannelMaskList.RemoveAll();
				m_AudioParamsList.RemoveAll();

				return hr;
			}

			// 2 - m_nSamplesPerSec
			for (int k = 0; k < m_wBitsPerSampleList.GetSize(); k++) {
				for (int i = 0; i < _countof(nSamplesPerSecValues); i++) {
					CreateFormat(wfex, m_wBitsPerSampleList[k], nChannelsValues[0], dwChannelMaskValues[0], nSamplesPerSecValues[i]);
					if (S_OK == m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfex, NULL)) {
						AudioParams ap(m_wBitsPerSampleList[k], nSamplesPerSecValues[i]);
						m_AudioParamsList.Add(ap);
					}
				}
			}
			if (m_AudioParamsList.GetSize() == 0) {
				m_wBitsPerSampleList.RemoveAll();
				m_nChannelsList.RemoveAll();
				m_dwChannelMaskList.RemoveAll();
				m_AudioParamsList.RemoveAll();

				return hr;
			}

			// 3 - nChannels(dwChannelMask)
			AudioParams ap = m_AudioParamsList[0];
			for (int i = 0; i < _countof(nChannelsValues); i++) {
				CreateFormat(wfex, ap.wBitsPerSample, nChannelsValues[i], dwChannelMaskValues[i], ap.nSamplesPerSec);
				if (S_OK == m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfex, NULL)) {
					m_nChannelsList.Add(nChannelsValues[i]);
					m_dwChannelMaskList.Add(dwChannelMaskValues[i]);
				}
			}
			if (m_nChannelsList.GetSize() == 0) {
				m_wBitsPerSampleList.RemoveAll();
				m_nChannelsList.RemoveAll();
				m_dwChannelMaskList.RemoveAll();
				m_AudioParamsList.RemoveAll();

				return hr;
			}

#ifdef _DEBUG
			DbgLog((LOG_TRACE, 3, L"	List of supported output formats:"));
			DbgLog((LOG_TRACE, 3, L"		BitsPerSample:"));
			for (int i = 0; i < m_wBitsPerSampleList.GetSize(); i++) {
				DbgLog((LOG_TRACE, 3, L"			%d", m_wBitsPerSampleList[i]));
				DbgLog((LOG_TRACE, 3, L"			SamplesPerSec:"));
				for (int k = 0; k < m_AudioParamsList.GetSize(); k++) {
					if (m_AudioParamsList[k].wBitsPerSample == m_wBitsPerSampleList[i]) {
						DbgLog((LOG_TRACE, 3, L"				%d", m_AudioParamsList[k].nSamplesPerSec));
					}
				}
			}

			#define ADDENTRY(mode) ChannelMaskStr[mode] = _T(#mode)
			CAtlMap<DWORD, CString> ChannelMaskStr;
			ADDENTRY(KSAUDIO_SPEAKER_STEREO);
			ADDENTRY(KSAUDIO_SPEAKER_QUAD);
			ADDENTRY(KSAUDIO_SPEAKER_SURROUND);
			ADDENTRY(KSAUDIO_SPEAKER_5POINT1_SURROUND);
			ADDENTRY(KSAUDIO_SPEAKER_5POINT1);
			ADDENTRY(KSAUDIO_SPEAKER_7POINT1_SURROUND);
			ADDENTRY(KSAUDIO_SPEAKER_7POINT1);
			#undef ADDENTRY
			DbgLog((LOG_TRACE, 3, L"		Channels:"));
			for (int i = 0; i < m_nChannelsList.GetSize(); i++) {
				DbgLog((LOG_TRACE, 3, L"			%d/0x%x	[%s]", m_nChannelsList[i], m_dwChannelMaskList[i], ChannelMaskStr[m_dwChannelMaskList[i]]));
			}
#endif
		}
	}
	return hr;
}

HRESULT CMpcAudioRenderer::EnableMMCSS()
{
	if (m_hTask == NULL) {
		if (pfAvSetMmThreadCharacteristicsW) {
			// Ask MMCSS to temporarily boost the thread priority
			// to reduce glitches while the low-latency stream plays.
			DWORD taskIndex = 0;

			m_hTask = pfAvSetMmThreadCharacteristicsW(_T("Pro Audio"), &taskIndex);
			DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::EnableMMCSS - Putting thread 0x%x in higher priority for Wasapi mode (lowest latency)", ::GetCurrentThreadId()));
			if (m_hTask == NULL) {
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}
	}
	return S_OK;
}

HRESULT CMpcAudioRenderer::RevertMMCSS()
{
	HRESULT hr = S_OK;

	if (m_hTask != NULL && pfAvRevertMmThreadCharacteristics != NULL) {
		if (pfAvRevertMmThreadCharacteristics(m_hTask)) {
			return hr;
		} else {
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	return S_FALSE;
}

void CMpcAudioRenderer::WasapiFlush()
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::WasapiFlush()"));

	CAutoLock cRenderLock(&m_csRender);

	m_Resampler.FlushBuffers();
	m_WasapiQueue.RemoveAll();
	m_CurrentPacket.Free();
}

HRESULT CMpcAudioRenderer::BeginFlush()
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::BeginFlush()"));

	if (m_hStopWaitingRenderer) {
		SetEvent(m_hStopWaitingRenderer);
	}

	return CBaseRenderer::BeginFlush();
}

HRESULT	CMpcAudioRenderer::EndFlush()
{
	DbgLog((LOG_TRACE, 3, L"CMpcAudioRenderer::EndFlush()"));

	if (!m_bEndOfStream) {
		WasapiFlush();
		m_Filter.Flush();
	}

	return CBaseRenderer::EndFlush();
}

inline REFERENCE_TIME CMpcAudioRenderer::GetRefClockTime()
{
	REFERENCE_TIME rt = INVALID_TIME;
	if (m_pReferenceClock) {
		m_pReferenceClock->GetTime(&rt);
		rt -= m_rtStartTime;
	}

	return rt;
}
