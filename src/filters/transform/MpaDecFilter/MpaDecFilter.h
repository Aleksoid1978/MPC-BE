/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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

#include <atlcoll.h>
#include "SampleFormat.h"
#include <stdint.h>

#define ENABLE_AC3_ENCODER 1

#include "../DeCSSFilter/DeCSSFilter.h"
#include "IMpaDecFilter.h"
#include "MpaDecFilterSettingsWnd.h"
#include "Mixer.h"
#include "PaddedArray.h"
#include "FFAudioDecoder.h"
#if ENABLE_AC3_ENCODER
#include "AC3Encoder.h"
#endif

#define MPCAudioDecName L"MPC Audio Decoder"

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

//struct DD_stats_t {
//protected:
//	int mode;
//	unsigned int ac3_frames;
//	unsigned int eac3_frames;
//
//public:
//	void Reset();
//	bool Desired(int type);
//};

class __declspec(uuid("3D446B6F-71DE-4437-BE15-8CE47174340F"))
	CMpaDecFilter
	: public CTransformFilter
	, public IMpaDecFilter
	, public ISpecifyPropertyPages2
{
	SampleFormat	m_InternalSampleFormat;
protected:
	// settings
	CCritSec        m_csProps;
	bool            m_fSampleFmt[sfcount];
	bool            m_fDRC;
	bool            m_fSPDIF[etcount];

	CCritSec m_csReceive;
	CPaddedArray    m_buff;

	REFERENCE_TIME  m_rtStart;
	double			m_dStartOffset;

	BOOL            m_bDiscontinuity;
	bool            m_bResync;

	ps2_state_t     m_ps2_state;
//	DD_stats_t      m_DDstats;

	BYTE            m_hdmibuff[61440];
	int             m_hdmicount;
	int             m_hdmisize;
	int             m_truehd_samplerate;
	int             m_truehd_framelength;

	CFFAudioDecoder m_FFAudioDec;

	BOOL			m_bNeedCheck;
	BOOL			m_bHasVideo;

	double			m_dRate;

	BOOL			m_bFlushing;

	enum BitstreamType {
		SPDIF,
		EAC3,
		TRUEHD,
		DTSHD,
		BTCOUNT
	};
	BOOL			m_bBitstreamSupported[BTCOUNT];

	CMixer m_Mixer;
	CAtlArray<float> m_encbuff;
#if ENABLE_AC3_ENCODER
	CAC3Encoder m_AC3Enc;
	HRESULT AC3Encode(BYTE* pBuff, int size, SampleFormat sfmt, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask = 0);
#endif

	BOOL ProcessBitstream(enum AVCodecID nCodecId, HRESULT& hr, BOOL bEOF = FALSE);

	HRESULT ProcessFFmpeg(enum AVCodecID nCodecId, BOOL bEOF = FALSE);

	HRESULT ProcessLPCM();
	HRESULT ProcessHdmvLPCM(bool bAlignOldBuffer);
//	HRESULT ProcessAC3();
	HRESULT ProcessAC3_SPDIF();
	HRESULT ProcessEAC3_SPDIF();
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
	HRESULT Deliver(BYTE* pBuff, size_t size, SampleFormat sfmt, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask = 0);
	HRESULT DeliverBitstream(BYTE* pBuff, int size, WORD type, int sample_rate, int samples);
	HRESULT ReconnectOutput(int nSamples, CMediaType& mt);
	CMediaType CreateMediaType(MPCSampleFormat sf, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask = 0);
	CMediaType CreateMediaTypeSPDIF(DWORD nSamplesPerSec = 48000);
	CMediaType CreateMediaTypeHDMI(WORD type);

	void CalculateDuration(int samples, int sample_rate, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop, BOOL bIsTrueHDBitstream = FALSE);

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

	STDMETHODIMP SetOutputFormat(MPCSampleFormat sf, bool enable);
	STDMETHODIMP_(bool) GetOutputFormat(MPCSampleFormat sf);
	STDMETHODIMP_(MPCSampleFormat) SelectOutputFormat(MPCSampleFormat sf);
	STDMETHODIMP SetDynamicRangeControl(bool fDRC);
	STDMETHODIMP_(bool) GetDynamicRangeControl();
	STDMETHODIMP SetSPDIF(enctype et, bool fSPDIF);
	STDMETHODIMP_(bool) GetSPDIF(enctype et);

	STDMETHODIMP SaveSettings();

	STDMETHODIMP_(CString) GetInformation(MPCAInfo index);
};
