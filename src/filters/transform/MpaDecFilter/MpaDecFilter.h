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

#pragma once

#include <stdint.h>
#include "../../filters/FilterInterfacesImpl.h"
#include "SampleFormat.h"
#include "../DeCSSFilter/DeCSSFilter.h"
#include "IMpaDecFilter.h"
#include "MpaDecFilterSettingsWnd.h"
#include "Mixer.h"
#include "PaddedBuffer.h"
#include "FFAudioDecoder.h"
#include "AC3Encoder.h"
#include "FloatingAverage.h"

#define MPCAudioDecName L"MPC Audio Decoder"

#define MAX_JITTER 100000i64 // +-10ms jitter is allowed

struct ps2_state_t {
	bool sync;
	int a[2], b[2];
	ps2_state_t() {
		reset();
	}
	void reset() {
		sync = false;
		a[0] = a[1] = b[0] = b[1] = 0;
	}
};

class __declspec(uuid("3D446B6F-71DE-4437-BE15-8CE47174340F"))
	CMpaDecFilter
	: public CTransformFilter
	, public IMpaDecFilter
	, public CExFilterConfigImpl
	, public ISpecifyPropertyPages2
{
	GUID            m_Subtype;
	AVCodecID       m_CodecId;
	SampleFormat    m_InternalSampleFormat;

protected:
	// settings
	CCritSec        m_csProps;
#ifdef REGISTER_FILTER
	bool            m_bSampleFmt[sfcount] = {};
#endif
	bool            m_bAVSync;
	bool            m_bDRC;

	bool            m_bSPDIF[etcount] = {};

	CCritSec        m_csReceive;
	CPaddedBuffer   m_buff;

	REFERENCE_TIME  m_rtStart;
	double          m_dStartOffset;

	BOOL            m_bDiscontinuity;
	BOOL            m_bResync;
	BOOL            m_bResyncTimestamp;

	ps2_state_t     m_ps2_state;

	struct {
		BYTE buf[61440];
		int  size;

		// E-AC3 Bitstreaming
		struct {
			int count;
			int repeat;
			int samples;
			int samplerate;

			bool Ready() const {
				return repeat > 0 && repeat == count;
			}
		} EAC3State;

		// TrueHD Bitstreaming
		struct {
			bool sync;

			int ratebits;

			uint16_t prev_frametime;
			bool prev_frametime_valid;

			uint32_t mat_framesize;
			uint32_t prev_mat_framesize;

			DWORD padding;
		} TrueHDMATState;
	} m_hdmi_bitstream = {};

	CFFAudioDecoder m_FFAudioDec;

	BOOL            m_bNeedCheck;
	BOOL            m_bHasVideo;

	double          m_dRate;

	BOOL            m_bFlushing;
	BOOL            m_bNeedSyncPoint;

	int             m_DTSHDProfile;

	REFERENCE_TIME  m_rtStartInput;
	REFERENCE_TIME  m_rtStopInput;
	REFERENCE_TIME  m_rtStartInputCache;
	REFERENCE_TIME  m_rtStopInputCache;
	BOOL            m_bUpdateTimeCache;

	FloatingAverage<REFERENCE_TIME> m_faJitter{50};
	REFERENCE_TIME m_JitterLimit = MAX_JITTER;


	enum BitstreamType {
		SPDIF,
		EAC3,
		TRUEHD,
		DTSHD,
		BTCOUNT
	};
	bool m_bBitstreamSupported[BTCOUNT] = {};
	bool m_bFallBackToPCM = false;

	void UpdateCacheTimeStamp();
	void ClearCacheTimeStamp();

	CMixer m_Mixer;
	std::vector<float> m_encbuff;
	CAC3Encoder m_AC3Enc;
	HRESULT AC3Encode(BYTE* pBuff, const size_t size, REFERENCE_TIME rtStartInput, const SampleFormat sfmt, const DWORD nSamplesPerSec, const WORD nChannels, const DWORD dwChannelMask);

	BOOL ProcessBitstream(enum AVCodecID nCodecId, HRESULT& hr, BOOL bEOF = FALSE);

	HRESULT ProcessFFmpeg(enum AVCodecID nCodecId, BOOL bEOF = FALSE);

	HRESULT ProcessDvdLPCM();
	HRESULT ProcessHdmvLPCM();
	HRESULT ProcessAC3_SPDIF();
	HRESULT ProcessEAC3_SPDIF(BOOL bEOF = FALSE);

	void MATWriteHeader();
	void MATWritePadding();
	void MATAppendData(const BYTE *p, int size);
	int MATFillDataBuffer(const BYTE *p, int size, bool padding = false);
	HRESULT MATDeliverPacket();
	HRESULT ProcessTrueHD_SPDIF();

	HRESULT ProcessDTS_SPDIF(BOOL bEOF = FALSE);
	HRESULT ProcessPS2PCM();
	HRESULT ProcessPS2ADPCM();
	HRESULT ProcessPCMraw();
	HRESULT ProcessPCMintBE();
	HRESULT ProcessPCMintLE();
	HRESULT ProcessPCMfloatBE();
	HRESULT ProcessPCMfloatLE();

	HRESULT GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData);
	HRESULT Deliver(BYTE* pBuff, const size_t size, const REFERENCE_TIME rtStartInput, const SampleFormat sfmt, const DWORD nSamplesPerSec, const WORD nChannels, DWORD dwChannelMask = 0);
	HRESULT DeliverBitstream(BYTE* pBuff, const int size, const REFERENCE_TIME rtStartInput, const WORD type, const int sample_rate, const int samples);
	HRESULT ReconnectOutput(int nSamples, CMediaType& mt);
	CMediaType CreateMediaType(MPCSampleFormat sf, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask = 0);
	CMediaType CreateMediaTypeSPDIF(DWORD nSamplesPerSec = 48000);
	CMediaType CreateMediaTypeHDMI(WORD type);

	void CalculateDuration(int samples, int sample_rate, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop, BOOL bIsTrueHDBitstream = FALSE);
	MPCSampleFormat SelectOutputFormat(MPCSampleFormat sf);

	friend class CFFAudioDecoder;

public:
	CMpaDecFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMpaDecFilter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT EndOfStream();
	HRESULT BeginFlush();
	HRESULT EndFlush();
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT Receive(IMediaSample* pIn);
	HRESULT CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin);

	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);

	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType *pmt);

	HRESULT BreakConnect(PIN_DIRECTION dir);

	// ISpecifyPropertyPages2
	STDMETHODIMP GetPages(CAUUID* pPages);
	STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// IMpaDecFilter
	STDMETHODIMP SetOutputFormat(MPCSampleFormat sf, bool bEnable);
	STDMETHODIMP_(bool) GetOutputFormat(MPCSampleFormat sf);
	STDMETHODIMP SetAVSyncCorrection(bool bAVSync);
	STDMETHODIMP_(bool) GetAVSyncCorrection();
	STDMETHODIMP SetDynamicRangeControl(bool bDRC);
	STDMETHODIMP_(bool) GetDynamicRangeControl();
	STDMETHODIMP SetSPDIF(enctype et, bool bSPDIF);
	STDMETHODIMP_(bool) GetSPDIF(enctype et);
	STDMETHODIMP SaveSettings();
	STDMETHODIMP_(CString) GetInformation(MPCAInfo index);

	// IExFilterConfig
	STDMETHODIMP SetBool(LPCSTR field, bool value) override;
};
