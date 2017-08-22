/*
 * (C) 2009-2017 see Authors.txt
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
	, public IAMStreamSelect
	, public ISpecifyPropertyPages2
	, public IMpcAudioRendererFilter
{
	CCritSec			m_csQueue;
	CCritSec			m_csResampler;
	CCritSec			m_csRender;
	CCritSec			m_csProps;
	CCritSec			m_csCheck;

	CMixer				m_Resampler;

	CPacketQueue		m_WasapiQueue;
	CAutoPtr<CPacket>	m_CurrentPacket;

	REFERENCE_TIME		m_rtStartTime;
	REFERENCE_TIME		m_rtNextSampleTime;

	BOOL				m_bFirstSampleToRender;

	BOOL				m_bUseDefaultDevice;

	CString				m_strCurrentDeviceId;
	CString				m_strCurrentDeviceName;

	BOOL				m_bHasVideo;

public:
	CMpcAudioRenderer(LPUNKNOWN punk, HRESULT *phr);
	~CMpcAudioRenderer();

	HRESULT CheckInputType(const CMediaType* mtIn);

	HRESULT Receive(IMediaSample* pSample) override;
	HRESULT CheckMediaType(const CMediaType *pmt) override;
	HRESULT SetMediaType(const CMediaType *pmt) override;
	HRESULT CompleteConnect(IPin *pReceivePin) override;
	HRESULT DoRenderSample(IMediaSample *pMediaSample) override { return E_NOTIMPL; }

	HRESULT EndFlush() override;

	void Flush();

	size_t WasapiQueueSize();
	void WaitFinish();

	CAMEvent m_eReleaseEvent{TRUE};
	bool m_bReleased             = false;
	HANDLE m_hReleaseTimerHandle = NULL;

	BOOL StartReleaseTimer();
	void EndReleaseTimer();
	void ReleaseDevice();

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
	STDMETHODIMP put_Balance(long lBalance) override;
	STDMETHODIMP get_Balance(long *plBalance) override;

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

	// === IAMStreamSelect
	STDMETHODIMP Count(DWORD* pcStreams);
	STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags);

	// === ISpecifyPropertyPages2
	STDMETHODIMP GetPages(CAUUID* pPages) override;
	STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage) override;

	// === IMpcAudioRendererFilter
	STDMETHODIMP					Apply() override;
	STDMETHODIMP					SetWasapiMode(INT nValue) override;
	STDMETHODIMP_(INT)				GetWasapiMode() override;
	STDMETHODIMP					SetDeviceId(CString pDeviceId) override;
	STDMETHODIMP_(CString)			GetDeviceId() override;
	STDMETHODIMP_(UINT)				GetMode() override;
	STDMETHODIMP					GetStatus(WAVEFORMATEX** ppWfxIn, WAVEFORMATEX** ppWfxOut) override;
	STDMETHODIMP					SetBitExactOutput(BOOL nValue) override;
	STDMETHODIMP_(BOOL)				GetBitExactOutput() override;
	STDMETHODIMP					SetSystemLayoutChannels(BOOL nValue) override;
	STDMETHODIMP_(BOOL)				GetSystemLayoutChannels() override;
	STDMETHODIMP					SetReleaseDeviceIdle(BOOL nValue) override;
	STDMETHODIMP_(BOOL)				GetReleaseDeviceIdle() override;
	STDMETHODIMP_(BITSTREAM_MODE)	GetBitstreamMode() override;
	STDMETHODIMP_(CString)			GetCurrentDeviceName() override;
	STDMETHODIMP_(CString)			GetCurrentDeviceId() override;
	STDMETHODIMP					SetSyncMethod(INT nValue) override;
	STDMETHODIMP_(INT)				GetSyncMethod() override;

	// CMpcAudioRenderer
private:
	HRESULT					GetReferenceClockInterface(REFIID riid, void **ppv);

	WAVEFORMATEX			*m_pWaveFormatExInput;
	WAVEFORMATEX			*m_pWaveFormatExOutput;
	CBaseReferenceClock*	m_pReferenceClock;
	double					m_dRate;
	long					m_lVolume;
	long					m_lBalance;
	double					m_dVolumeFactor;
	double					m_dBalanceFactor;
	DWORD					m_dwBalanceMask;
	bool					m_bUpdateBalanceMask; // TODO: remove it

	void					SetBalanceMask(const DWORD output_layout);
	void					ApplyVolumeBalance(BYTE* pData, UINT32 size);

	CFilter					m_Filter;

	// CMpcAudioRenderer WASAPI methods
	HRESULT					GetAudioDevice(const BOOL bForceUseDefaultDevice);
	HRESULT					CreateAudioClient();
	HRESULT					InitAudioClient(WAVEFORMATEX *pWaveFormatEx, BOOL bCheckFormat = TRUE);
	HRESULT					CheckAudioClient(WAVEFORMATEX *pWaveFormatEx = NULL);
	HRESULT					Transform(IMediaSample *pMediaSample);
	HRESULT					PushToQueue(CAutoPtr<CPacket> p);

	bool					IsFormatChanged(const WAVEFORMATEX *pWaveFormatEx, const WAVEFORMATEX *pNewWaveFormatEx);
	bool					CopyWaveFormat(WAVEFORMATEX *pSrcWaveFormatEx, WAVEFORMATEX **ppDestWaveFormatEx);

	BOOL					IsBitstream(WAVEFORMATEX *pWaveFormatEx);
	HRESULT					SelectFormat(WAVEFORMATEX* pwfx, WAVEFORMATEXTENSIBLE& wfex);
	void					CreateFormat(WAVEFORMATEXTENSIBLE& wfex, WORD wBitsPerSample, WORD nChannels, DWORD dwChannelMask, DWORD nSamplesPerSec, WORD wValidBitsPerSample = 0);

	HRESULT					StartAudioClient();

	void					SetReinitializeAudioDevice(BOOL bFullInitialization = FALSE);
	HRESULT					ReinitializeAudioDevice(BOOL bFullInitialization = FALSE);

	HRESULT					RenderWasapiBuffer();
	void					CheckBufferStatus();
	void					WasapiFlush();

	inline REFERENCE_TIME	GetRefClockTime();

	// WASAPI variables
	HMODULE					m_hModule;
	HANDLE					m_hTask;
	WASAPI_MODE				m_WASAPIMode;
	CString					m_DeviceId;
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
	BOOL					m_bReleaseDeviceIdle;
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

	HANDLE					m_hRendererNeedMoreData;

	CAMEvent				m_FlushEvent;
	CAMEvent				m_ReceiveEvent;

	BOOL					m_bNeedReinitialize;
	BOOL					m_bNeedReinitializeFull;

	CSimpleArray<WORD>		m_wBitsPerSampleList;
	CSimpleArray<WORD>		m_nChannelsList;
	CSimpleArray<DWORD>		m_dwChannelMaskList;

	BOOL					m_bReal32bitSupport;

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

	struct AudioFormats {
		SampleFormat sf = SAMPLE_FMT_NONE;
		DWORD layout    = 0;
		int channels    = 0;
		int samplerate  = 0;
	};
	AudioFormats m_input_params, m_output_params;
};

class CMpcAudioRendererInputPin final
	: public CRendererInputPin
{
	CMpcAudioRenderer* m_pRenderer;

	CCritSec m_csReceive;

public:
	CMpcAudioRendererInputPin(CBaseRenderer* pRenderer, HRESULT* phr);

	STDMETHODIMP NewSegment(REFERENCE_TIME startTime, REFERENCE_TIME stopTime, double rate) override;
	STDMETHODIMP Receive(IMediaSample* pSample) override;

	STDMETHODIMP EndOfStream() override;
	STDMETHODIMP BeginFlush() override;
	STDMETHODIMP EndFlush() override;
};
