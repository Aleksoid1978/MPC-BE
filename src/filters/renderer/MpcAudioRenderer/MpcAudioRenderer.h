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

#pragma once

#include <BaseClasses/streams.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <FunctionDiscoveryKeys_devpkey.h>
#include "MpcAudioRendererSettingsWnd.h"
#include "Mixer.h"
#include "Filter.h"
#include "../../../DSUtil/Packet.h"

#define MpcAudioRendererName L"MPC Audio Renderer"

// if you get a compilation error on AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED,
// uncomment the #define below
// #define AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED	AUDCLNT_ERR(0x019)

class __declspec(uuid("601D2A2B-9CDE-40bd-8650-0485E3522727"))
	CMpcAudioRenderer final
	: public CBaseRenderer
	, public IBasicAudio
	, public IMediaSeeking
	, public IMMNotificationClient
	, public ISpecifyPropertyPages2
	, public IMpcAudioRendererFilter
{
	CCritSec			m_csRender;
	CCritSec			m_csProps;
	CCritSec			m_csCheck;

	CAMEvent			m_eEndReceive;
	CAMEvent			m_eReinitialize;

	CMixer				m_Resampler;

	CPacketQueue		m_WasapiQueue;
	CAutoPtr<CPacket>	m_CurrentPacket;

	REFERENCE_TIME		m_rtStartTime;
	REFERENCE_TIME		m_rtNextSampleTime;

	ULONGLONG			m_nSampleNum;

	BOOL				m_bUseDefaultDevice;

	CString				m_strDeviceId;
	CString				m_strDeviceName;

	BOOL				m_bHasVideo;

public:
	CMpcAudioRenderer(LPUNKNOWN punk, HRESULT *phr);
	~CMpcAudioRenderer();

	HRESULT CheckInputType(const CMediaType* mtIn);

	HRESULT Receive(IMediaSample* pSample) override;
	HRESULT CheckMediaType(const CMediaType *pmt) override;
	HRESULT SetMediaType(const CMediaType *pmt) override;
	HRESULT DoRenderSample(IMediaSample *pMediaSample) override { return E_NOTIMPL; }

	HRESULT BeginFlush() override;
	HRESULT EndFlush() override;

	DECLARE_IUNKNOWN

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv) override;

	// === IMediaFilter
	STDMETHODIMP Run(REFERENCE_TIME rtStart) override;
	STDMETHODIMP Stop() override;
	STDMETHODIMP Pause() override;

	// === IDispatch (pour IBasicAudio)
	STDMETHODIMP GetTypeInfoCount(UINT * pctinfo) override { return E_NOTIMPL; };
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo) override { return E_NOTIMPL; };
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) override { return E_NOTIMPL; };
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) override { return E_NOTIMPL; };

	// === IBasicAudio
	STDMETHODIMP put_Volume(long lVolume) override;
	STDMETHODIMP get_Volume(long *plVolume) override;
	STDMETHODIMP put_Balance(long lBalance) override { return E_NOTIMPL; }
	STDMETHODIMP get_Balance(long *plBalance) override { return E_NOTIMPL; }

	// === IMediaSeeking
	STDMETHODIMP GetCapabilities(DWORD* pCapabilities) override;
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities) override;
	STDMETHODIMP IsFormatSupported(const GUID* pFormat) override;
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat) override;
	STDMETHODIMP GetTimeFormat(GUID* pFormat) override;
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat) override;
	STDMETHODIMP SetTimeFormat(const GUID* pFormat) override;
	STDMETHODIMP GetDuration(LONGLONG* pDuration) override;
	STDMETHODIMP GetStopPosition(LONGLONG* pStop) override;
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent) override;
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) override;
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags) override;
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop) override;
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest) override;
	STDMETHODIMP SetRate(double dRate) override;
	STDMETHODIMP GetRate(double* pdRate) override;
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll) override;

	// === IMMNotificationClient
	STDMETHODIMP OnDeviceStateChanged(__in LPCWSTR pwstrDeviceId, __in DWORD dwNewState) override;
	STDMETHODIMP OnDeviceAdded(__in LPCWSTR pwstrDeviceId) override { return E_NOTIMPL; }
	STDMETHODIMP OnDeviceRemoved(__in LPCWSTR pwstrDeviceId) override { return E_NOTIMPL; }
	STDMETHODIMP OnDefaultDeviceChanged(__in EDataFlow flow, __in ERole role, __in LPCWSTR pwstrDefaultDeviceId) override;
	STDMETHODIMP OnPropertyValueChanged(__in LPCWSTR pwstrDeviceId, __in const PROPERTYKEY key) override { return E_NOTIMPL; }

	// === ISpecifyPropertyPages2
	STDMETHODIMP GetPages(CAUUID* pPages) override;
	STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage) override;

	// === IMpcAudioRendererFilter
	STDMETHODIMP					Apply() override;
	STDMETHODIMP					SetWasapiMode(INT nValue) override;
	STDMETHODIMP_(INT)				GetWasapiMode() override;
	STDMETHODIMP					SetSoundDevice(CString nValue) override;
	STDMETHODIMP_(CString)			GetSoundDevice() override;
	STDMETHODIMP_(UINT)				GetMode() override;
	STDMETHODIMP					GetStatus(WAVEFORMATEX** ppWfxIn, WAVEFORMATEX** ppWfxOut) override;
	STDMETHODIMP					SetBitExactOutput(BOOL nValue) override;
	STDMETHODIMP_(BOOL)				GetBitExactOutput() override;
	STDMETHODIMP					SetSystemLayoutChannels(BOOL nValue) override;
	STDMETHODIMP_(BOOL)				GetSystemLayoutChannels() override;
	STDMETHODIMP_(BITSTREAM_MODE)	GetBitstreamMode() override;
	STDMETHODIMP_(CString)			GetCurrentPlaybackDevice() override;
	STDMETHODIMP					SetSyncMethod(INT nValue) override;
	STDMETHODIMP_(INT)				GetSyncMethod() override;

	// CMpcAudioRenderer
private:
	HRESULT					GetReferenceClockInterface(REFIID riid, void **ppv);

	WAVEFORMATEX			*m_pWaveFileFormat;
	WAVEFORMATEX			*m_pWaveFileFormatOutput;
	CBaseReferenceClock*	m_pReferenceClock;
	double					m_dRate;
	long					m_lVolume;
	double					m_dVolumeFactor;

	CFilter					m_Filter;

	// CMpcAudioRenderer WASAPI methods
	HRESULT					GetAudioDevice();
	HRESULT					CreateAudioClient();
	HRESULT					InitAudioClient(WAVEFORMATEX *pWaveFormatEx, BOOL bCheckFormat = TRUE);
	HRESULT					CheckAudioClient(WAVEFORMATEX *pWaveFormatEx = NULL);
	HRESULT					DoRenderSampleWasapi(IMediaSample *pMediaSample);
	HRESULT					GetBufferSize(WAVEFORMATEX *pWaveFormatEx, REFERENCE_TIME *pHnsBufferPeriod);

	bool					IsFormatChanged(const WAVEFORMATEX *pWaveFormatEx, const WAVEFORMATEX *pNewWaveFormatEx);
	bool					CheckFormatChanged(WAVEFORMATEX *pWaveFormatEx, WAVEFORMATEX **ppNewWaveFormatEx);
	bool					CopyWaveFormat(WAVEFORMATEX *pSrcWaveFormatEx, WAVEFORMATEX **ppDestWaveFormatEx);

	BOOL					IsBitstream(WAVEFORMATEX *pWaveFormatEx);
	HRESULT					SelectFormat(WAVEFORMATEX* pwfx, WAVEFORMATEXTENSIBLE& wfex);
	void					CreateFormat(WAVEFORMATEXTENSIBLE& wfex, WORD wBitsPerSample, WORD nChannels, DWORD dwChannelMask, DWORD nSamplesPerSec);

	HRESULT					StartAudioClient();
	HRESULT					StopAudioClient();

	HRESULT					ReinitializeAudioDevice();

	HRESULT					RenderWasapiBuffer();
	void					CheckBufferStatus();
	void					WasapiFlush();

	inline REFERENCE_TIME	GetRefClockTime();

	// WASAPI variables
	HMODULE					m_hModule;
	HANDLE					m_hTask;
	WASAPI_MODE				m_WASAPIMode;
	CString					m_DeviceName;
	IMMDevice				*m_pMMDevice;
	IAudioClient			*m_pAudioClient;
	IAudioRenderClient		*m_pRenderClient;
	UINT32					m_nFramesInBuffer;
	REFERENCE_TIME			m_hnsPeriod;
	bool					m_bIsAudioClientStarted;
	BOOL					m_bIsBitstream;
	BITSTREAM_MODE			m_BitstreamMode;
	BOOL					m_bUseBitExactOutput;
	BOOL					m_bUseSystemLayoutChannels;
	SYNC_METHOD				m_SyncMethod;
	FILTER_STATE			m_filterState;

	UINT32					m_nSampleOffset;

	typedef HANDLE			(__stdcall *PTR_AvSetMmThreadCharacteristicsW)(LPCWSTR TaskName, LPDWORD TaskIndex);
	typedef BOOL			(__stdcall *PTR_AvRevertMmThreadCharacteristics)(HANDLE AvrtHandle);

	PTR_AvSetMmThreadCharacteristicsW	pfAvSetMmThreadCharacteristicsW;
	PTR_AvRevertMmThreadCharacteristics	pfAvRevertMmThreadCharacteristics;

	HRESULT					EnableMMCSS();
	HRESULT					RevertMMCSS();

	// Rendering thread
	static DWORD WINAPI		RenderThreadEntryPoint(LPVOID lpParameter);
	DWORD					RenderThread();
	DWORD					m_nThreadId;

	BOOL					m_bThreadPaused;

	HRESULT					StopRendererThread();
	HRESULT					StartRendererThread();
	HRESULT					PauseRendererThread();

	HANDLE					m_hRenderThread;

	HANDLE					m_hDataEvent;
	HANDLE					m_hPauseEvent;
	HANDLE					m_hResumeEvent;
	HANDLE					m_hWaitPauseEvent;
	HANDLE					m_hWaitResumeEvent;
	HANDLE					m_hStopRenderThreadEvent;
	HANDLE					m_hReinitializeEvent;

	HANDLE					m_hRendererNeedMoreData;
	HANDLE					m_hStopWaitingRenderer;

	CSimpleArray<WORD>		m_wBitsPerSampleList;
	CSimpleArray<WORD>		m_nChannelsList;
	CSimpleArray<DWORD>		m_dwChannelMaskList;

	struct AudioParams {
		WORD  wBitsPerSample;
		DWORD nSamplesPerSec;

		AudioParams(WORD v1, DWORD v2) {
			wBitsPerSample = v1;
			nSamplesPerSec = v2;
		}

		bool operator == (const struct AudioParams& ap) const {
			return (this->wBitsPerSample == ap.wBitsPerSample
					&& this->nSamplesPerSec == ap.nSamplesPerSec);
		}
	};
	CSimpleArray<AudioParams> m_AudioParamsList;
};

class CMpcAudioRendererInputPin final
	: public CRendererInputPin
{
public:
	CMpcAudioRendererInputPin(CBaseRenderer* pRenderer, HRESULT* phr);

	STDMETHODIMP EndOfStream() override;
};
