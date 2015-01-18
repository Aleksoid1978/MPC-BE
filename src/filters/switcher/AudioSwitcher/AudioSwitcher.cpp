/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include <math.h>
#include <MMReg.h>
#include <Ks.h>
#include <KsMedia.h>
#include <moreuuids.h>
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/AudioTools.h"
#include "../../../DSUtil/AudioParser.h"
#include "../../../AudioTools/AudioHelper.h"
#include "../../../ExtLib/ffmpeg/libavutil/channel_layout.h"

#include "AudioSwitcher.h"


#ifdef REGISTER_FILTER

#include "../../filters/ffmpeg_fix.cpp"

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CAudioSwitcherFilter), AudioSwitcherName, MERIT_DO_NOT_USE, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CAudioSwitcherFilter>, NULL, &sudFilter[0]}
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

static struct channel_mode_t {
	const WORD channels;
	const DWORD ch_layout;
	//const LPCTSTR op_value;
}
channel_mode[] = {
	//n  libavcodec                               ID          Name
	{1, AV_CH_LAYOUT_MONO   , /*_T("1.0")*/ }, // SPK_MONO   "Mono"
	{2, AV_CH_LAYOUT_STEREO , /*_T("2.0")*/ }, // SPK_STEREO "Stereo"
	{4, AV_CH_LAYOUT_QUAD   , /*_T("4.0")*/ }, // SPK_4_0    "4.0"
	{5, AV_CH_LAYOUT_5POINT0, /*_T("5.0")*/ }, // SPK_5_0    "5.0"
	{6, AV_CH_LAYOUT_5POINT1, /*_T("5.1")*/ }, // SPK_5_1    "5.1"
	{8, AV_CH_LAYOUT_7POINT1, /*_T("7.1")*/ }, // SPK_7_1    "7.1"
};

static DWORD GetChannelLayout(const WAVEFORMATEX* wfe)
{
	DWORD channel_layout = 0;

	if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		channel_layout = ((WAVEFORMATEXTENSIBLE*)wfe)->dwChannelMask;
	}

	if (channel_layout == 0) {
		channel_layout = GetDefChannelMask(wfe->nChannels);
	}

	return channel_layout;
}

//
// CAudioSwitcherFilter
//

CAudioSwitcherFilter::CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CStreamSwitcherFilter(lpunk, phr, __uuidof(this))
	, m_bMixer(false)
	, m_nMixerLayout(SPK_STEREO)
	, m_bAutoVolumeControl(false)
	, m_bNormBoost(true)
	, m_iNormLevel(75)
	, m_iNormRealeaseTime(8)
	, m_buffer(NULL)
	, m_buf_size(0)
	, m_fGain_dB(0.0f)
	, m_fGainFactor(1.0f)
	, m_rtAudioTimeShift(0)
	, m_rtNextStart(0)
{
	m_AudioNormalizer.SetParam(m_iNormLevel, true, m_iNormRealeaseTime);

	if (phr) {
		if (FAILED(*phr)) {
			return;
		} else {
			*phr = S_OK;
		}
	}
}

CAudioSwitcherFilter::~CAudioSwitcherFilter()
{
	SAFE_DELETE_ARRAY(m_buffer);
}

void CAudioSwitcherFilter::UpdateBufferSize(size_t allsamples)
{
	if (allsamples > m_buf_size) {
		SAFE_DELETE_ARRAY(m_buffer);
		m_buf_size = allsamples;
		m_buffer = DNew float[m_buf_size];
	}
}

STDMETHODIMP CAudioSwitcherFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAudioSwitcherFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAudioSwitcherFilter::CheckMediaType(const CMediaType* pmt)
{
	if (pmt->majortype == MEDIATYPE_Audio && pmt->formattype == FORMAT_WaveFormatEx) {
		WORD wFormatTag = ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag;
		WORD bps = ((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample;

		if (pmt->subtype == MEDIASUBTYPE_PCM) {
			switch (wFormatTag) {
			case WAVE_FORMAT_PCM:
				if (bps == 8 || bps == 16 || bps == 24 || bps == 32) {
					return S_OK;
				}
				break;
			case WAVE_FORMAT_DOLBY_AC3_SPDIF:
				return S_OK;
			case WAVE_FORMAT_EXTENSIBLE:
				GUID& SubFormat = ((WAVEFORMATEXTENSIBLE*)pmt->pbFormat)->SubFormat;
				if (SubFormat == KSDATAFORMAT_SUBTYPE_PCM
						|| SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
						|| SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD
						|| SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS
						|| SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
					return S_OK;
				}
			}
		} else if (pmt->subtype == MEDIASUBTYPE_IEEE_FLOAT && bps == 32
				&& (wFormatTag == WAVE_FORMAT_IEEE_FLOAT || wFormatTag == WAVE_FORMAT_EXTENSIBLE)) {
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CAudioSwitcherFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	HRESULT hr;

	CStreamSwitcherInputPin* pInPin = GetInputPin();
	CStreamSwitcherOutputPin* pOutPin = GetOutputPin();
	if (!pInPin || !pOutPin) {
		return __super::Transform(pIn, pOut);
	}

	const WAVEFORMATEX* in_wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;
	const WAVEFORMATEX* out_wfe = (WAVEFORMATEX*)pOutPin->CurrentMediaType().pbFormat;

	const SampleFormat in_sampleformat = GetSampleFormat(in_wfe);
	if (in_sampleformat == SAMPLE_FMT_NONE) {
		return __super::Transform(pIn, pOut);
	}

	const unsigned in_bytespersample = in_wfe->wBitsPerSample / 8;
	unsigned in_channels       = in_wfe->nChannels;
	unsigned in_samples        = pIn->GetActualDataLength() / (in_channels * in_bytespersample);
	unsigned in_allsamples     = in_samples * in_channels;

	REFERENCE_TIME rtDur = 10000000i64 * in_samples / in_wfe->nSamplesPerSec;

	REFERENCE_TIME rtStart, rtStop;
	if (FAILED(pIn->GetTime(&rtStart, &rtStop))) {
		rtStart = m_rtNextStart;
		rtStop  = m_rtNextStart + rtDur;
	}
	m_rtNextStart = rtStop;

	//if (pIn->IsDiscontinuity() == S_OK) {
	//}

	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;
	if (FAILED(hr = pIn->GetPointer(&pDataIn)) || FAILED(hr = pOut->GetPointer(&pDataOut))) {
		return hr;
	}

	if (!pDataIn || !pDataOut || in_samples < 0) {
		return S_FALSE;
	}

	// in_samples = 0 doesn't mean it's failed, return S_OK otherwise might screw the sound
	if (in_samples == 0) {
		pOut->SetActualDataLength(0);
		return S_OK;
	}

	if (long(in_samples * out_wfe->nChannels * in_bytespersample) > pOut->GetSize()) {
		DbgLog((LOG_TRACE, 3, L"CAudioSwitcherFilter::Transform: %d > %d", in_samples * out_wfe->nChannels * in_bytespersample, pOut->GetSize()));
		pOut->SetActualDataLength(0);
		return S_OK;
	}

	BYTE* data = pDataIn;

	// Mixer
	DWORD in_layout = GetChannelLayout(in_wfe);
	DWORD out_layout = GetChannelLayout(out_wfe);
	if (in_layout != out_layout) {
		BYTE* out;
		if (in_sampleformat == SAMPLE_FMT_FLT) {
			out = pDataOut;
		} else {
			UpdateBufferSize(in_allsamples * out_wfe->nChannels);
			out = (BYTE*)m_buffer;
		}

		m_Mixer.UpdateInput(in_sampleformat, in_layout);
		m_Mixer.UpdateOutput(SAMPLE_FMT_FLT, out_layout);

		in_samples		= m_Mixer.Mixing(out, in_samples, data, in_samples);
		data			= out;
		in_channels		= out_wfe->nChannels;
		in_allsamples	= in_samples * in_channels;
	} else if (in_sampleformat == SAMPLE_FMT_FLT) {
		memcpy(pDataOut, data, in_allsamples * sizeof(float));
		data = pDataOut;
	}

	// Auto volume control
	if (m_bAutoVolumeControl) {
		if (data == pDataIn) {
			UpdateBufferSize(in_allsamples);
			convert_to_float(in_sampleformat, in_channels, in_samples, data, m_buffer);
			data = (BYTE*)m_buffer;
		}
		in_samples		= m_AudioNormalizer.MSteadyHQ32((float*)data, in_samples, in_channels);
		in_allsamples	= in_samples * in_channels;
	}

	// Copy or convert to output
	if (data == pDataIn) {
		memcpy(pDataOut, data, in_allsamples * in_bytespersample);
	} else if (data == (BYTE*)m_buffer) {
		convert_float_to(in_sampleformat, in_channels, in_samples, m_buffer, pDataOut);
	}

	// Gain
	if (!m_bAutoVolumeControl && m_fGainFactor != 1.0f) {
		switch (in_sampleformat) {
		case SAMPLE_FMT_U8:
			gain_uint8(m_fGainFactor, in_allsamples, (uint8_t*)pDataOut);
			break;
		case SAMPLE_FMT_S16:
			gain_int16(m_fGainFactor, in_allsamples, (int16_t*)pDataOut);
			break;
		case SAMPLE_FMT_S24:
			gain_int24(m_fGainFactor, in_allsamples, pDataOut);
			break;
		case SAMPLE_FMT_S32:
			gain_int32(m_fGainFactor, in_allsamples, (int32_t*)pDataOut);
			break;
		case SAMPLE_FMT_FLT:
			gain_float(m_fGainFactor, in_allsamples, (float*)pDataOut);
			break;
		}
	}

	pOut->SetActualDataLength(in_allsamples * get_bytes_per_sample(in_sampleformat));

	rtStart += m_rtAudioTimeShift;
	rtStop  += m_rtAudioTimeShift;
	pOut->SetTime(&rtStart, &rtStop);

	return S_OK;
}

void CAudioSwitcherFilter::TransformMediaType(CMediaType& mt)
{
	if (mt.majortype == MEDIATYPE_Audio && mt.formattype == FORMAT_WaveFormatEx) {
		WORD formattag;
		if (mt.subtype == MEDIASUBTYPE_PCM) {
			formattag = WAVE_FORMAT_PCM;
		} else if (mt.subtype == MEDIASUBTYPE_IEEE_FLOAT) {
			formattag = WAVE_FORMAT_IEEE_FLOAT;
		} else {
			return;
		}

		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;
		WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;

		// exclude spdif/bitstream formats
		if (wfe->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) {
			return;
		}
		if (IsWaveFormatExtensible(wfe)
				&& (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS
					|| wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP
					|| wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD)) {
			return;
		}

		WORD  channels;
		DWORD layout;
		if (m_bMixer) {
			channels = channel_mode[m_nMixerLayout].channels;
			layout   = channel_mode[m_nMixerLayout].ch_layout;
		} else {
			channels = wfe->nChannels;
			if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe->nChannels == CountBits(wfex->dwChannelMask)) {
				layout = wfex->dwChannelMask;
			} else {
				layout = GetDefChannelMask(wfe->nChannels);
			}
		}

		if (channels <= 2
				&& (formattag == WAVE_FORMAT_PCM && (wfe->wBitsPerSample == 8 || wfe->wBitsPerSample == 16)
				|| formattag == WAVE_FORMAT_IEEE_FLOAT && wfe->wBitsPerSample == 32)) {

			if (mt.cbFormat != sizeof(WAVEFORMATEX)) {
				wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX));
			}
			wfe->wFormatTag			= formattag;
			wfe->nChannels			= channels;
			wfe->nBlockAlign		= channels * wfe->wBitsPerSample / 8;
			wfe->nAvgBytesPerSec	= wfe->nBlockAlign * wfe->nSamplesPerSec;
			wfe->cbSize = 0;
		} else {
			if (mt.cbFormat != sizeof(WAVEFORMATEXTENSIBLE)) {
				wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
				wfex->Samples.wValidBitsPerSample = wfex->Format.wBitsPerSample;
			}
			wfex->Format.wFormatTag			= WAVE_FORMAT_EXTENSIBLE;
			wfex->Format.nChannels			= channels;
			wfex->Format.nBlockAlign		= channels * wfex->Format.wBitsPerSample / 8;
			wfex->Format.nAvgBytesPerSec	= wfex->Format.nBlockAlign * wfex->Format.nSamplesPerSec;
			wfex->Format.cbSize				= 22;
			wfex->dwChannelMask				= layout;
			wfex->SubFormat					= mt.subtype;
		}
	}
}

HRESULT CAudioSwitcherFilter::DeliverEndFlush()
{
	DbgLog((LOG_TRACE, 3, L"CAudioSwitcherFilter::DeliverEndFlush()"));

	return __super::DeliverEndFlush();
}

HRESULT CAudioSwitcherFilter::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	DbgLog((LOG_TRACE, 3, L"CAudioSwitcherFilter::DeliverNewSegment()"));

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

// IAudioSwitcherFilter

STDMETHODIMP_(REFERENCE_TIME) CAudioSwitcherFilter::GetAudioTimeShift()
{
	return m_rtAudioTimeShift;
}

STDMETHODIMP CAudioSwitcherFilter::SetAudioTimeShift(REFERENCE_TIME rtAudioTimeShift)
{
	m_rtAudioTimeShift = rtAudioTimeShift;
	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::GetAutoVolumeControl(bool& bAutoVolumeControl, bool& bNormBoost, int& iNormLevel, int& iNormRealeaseTime)
{
	bAutoVolumeControl	= m_bAutoVolumeControl;
	bNormBoost			= m_bNormBoost;
	iNormLevel			= m_iNormLevel;
	iNormRealeaseTime	= m_iNormRealeaseTime;

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetAutoVolumeControl(bool bAutoVolumeControl, bool bNormBoost, int iNormLevel, int iNormRealeaseTime)
{
	m_bAutoVolumeControl	= bAutoVolumeControl;
	m_bNormBoost			= bNormBoost;
	m_iNormLevel				= min(max(0, iNormLevel), 100);
	m_iNormRealeaseTime		= min(max(5, iNormRealeaseTime), 10);

	m_AudioNormalizer.SetParam(m_iNormLevel, bNormBoost, m_iNormRealeaseTime);

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetChannelMixer(bool bMixer, int nLayout)
{
	if (bMixer != m_bMixer || bMixer && nLayout != m_nMixerLayout) {
		m_bOutputFormatChanged = true;
	}

	m_bMixer = bMixer;
	if (nLayout >= SPK_MONO && nLayout <= SPK_7_1) {
		m_nMixerLayout = nLayout;
	}

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetAudioGain(float fGain_dB)
{
	m_fGain_dB = min(max(-3.0f, fGain_dB), 10.0f);
	m_fGainFactor = pow(10.0f, m_fGain_dB/20.0f);

	return S_OK;
}

// IAMStreamSelect

STDMETHODIMP CAudioSwitcherFilter::Enable(long lIndex, DWORD dwFlags)
{
	HRESULT hr = __super::Enable(lIndex, dwFlags);

	return hr;
}
