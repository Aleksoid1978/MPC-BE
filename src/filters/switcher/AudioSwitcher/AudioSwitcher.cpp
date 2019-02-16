/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2019 see Authors.txt
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
#include "../../../DSUtil/SysVersion.h"
#include "../../../AudioTools/AudioHelper.h"
#include "../../../ExtLib/ffmpeg/libavutil/channel_layout.h"
#include "AudioSwitcher.h"

#ifdef REGISTER_FILTER

void* __imp__aligned_malloc  = _aligned_malloc;
void* __imp__aligned_realloc = _aligned_realloc;
void* __imp__aligned_free    = _aligned_free;

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CAudioSwitcherFilter), AudioSwitcherName, MERIT_DO_NOT_USE, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CAudioSwitcherFilter>, nullptr, &sudFilter[0]}
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

#define AUDIO_LEVEL_MAX   10.0
#define AUDIO_LEVEL_MIN  -10.0
#define AUDIO_GAIN_MAX    10.0
#define AUDIO_GAIN_MIN    -3.0

static struct channel_mode_t {
	const WORD channels;
	const DWORD ch_layout;
	//const LPCWSTR op_value;
}
channel_mode[] = {
	//n  libavcodec                               ID          Name
	{1, AV_CH_LAYOUT_MONO   , /*L"1.0"*/ }, // SPK_MONO   "Mono"
	{2, AV_CH_LAYOUT_STEREO , /*L"2.0"*/ }, // SPK_STEREO "Stereo"
	{4, AV_CH_LAYOUT_QUAD   , /*L"4.0"*/ }, // SPK_4_0    "4.0"
	{5, AV_CH_LAYOUT_5POINT0, /*L"5.0"*/ }, // SPK_5_0    "5.0"
	{6, AV_CH_LAYOUT_5POINT1, /*L"5.1"*/ }, // SPK_5_1    "5.1"
	{8, AV_CH_LAYOUT_7POINT1, /*L"7.1"*/ }, // SPK_7_1    "7.1"
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

inline double decibel2factor(double dB) {
	return pow(10.0, dB / 20.0);
}

//
// CAudioSwitcherFilter
//

CAudioSwitcherFilter::CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CStreamSwitcherFilter(lpunk, phr, __uuidof(this))
	, m_bMixer(false)
	, m_nMixerLayout(SPK_STEREO)
	, m_bBassRedirect(false)
	, m_dCenterLevel(1.0)
	, m_dSurroundLevel(1.0)
	, m_dGainFactor(1.0)
	, m_bAutoVolumeControl(false)
	, m_bNormBoost(true)
	, m_iNormLevel(75)
	, m_iNormRealeaseTime(8)
	, m_bInt16(true)
	, m_bInt24(true)
	, m_bInt32(true)
	, m_bFloat(true)
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
}

HRESULT CAudioSwitcherFilter::CheckMediaType(const CMediaType* pmt)
{
	if (pmt->majortype == MEDIATYPE_Audio && pmt->formattype == FORMAT_WaveFormatEx) {
		const WORD wFormatTag = ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag;
		const WORD bps        = ((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample;

		if (pmt->subtype == MEDIASUBTYPE_PCM) {
			switch (wFormatTag) {
				case WAVE_FORMAT_PCM:
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
		} else if (pmt->subtype == MEDIASUBTYPE_IEEE_FLOAT && (bps == 32 || bps == 64)
				&& (wFormatTag == WAVE_FORMAT_IEEE_FLOAT || wFormatTag == WAVE_FORMAT_EXTENSIBLE)) {
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

int get_channel_pos(DWORD layout, DWORD channel)
{
	ASSERT(layout & channel);
	ASSERT(CountBits(channel) == 1);

	int num = 0;
	layout &= channel - 1;
	for (; layout; layout >>= 1) {
		num += layout & 1;
	}
	return num;
}

HRESULT CAudioSwitcherFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	CAutoLock cAutoLock(&m_csTransform);

	HRESULT hr;

	// check input an output buffers
	BYTE* pDataIn = nullptr;
	BYTE* pDataOut = nullptr;
	if (FAILED(hr = pIn->GetPointer(&pDataIn)) || FAILED(hr = pOut->GetPointer(&pDataOut))) {
		return hr;
	}
	if (!pDataIn || !pDataOut) {
		return E_POINTER;
	}

	// check input an output pins
	CStreamSwitcherInputPin* pInPin = GetInputPin();
	CStreamSwitcherOutputPin* pOutPin = GetOutputPin();
	if (!pInPin || !pOutPin) {
		return __super::Transform(pIn, pOut); // hmm
	}

	const WAVEFORMATEX* in_wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;
	SampleFormat audio_sampleformat = GetSampleFormat(in_wfe);

	// bitsreaming
	if (audio_sampleformat == SAMPLE_FMT_NONE) {
		REFERENCE_TIME start, stop;
		if (SUCCEEDED(pIn->GetTime(&start, &stop))) {
			start += m_rtAudioTimeShift;
			stop  += m_rtAudioTimeShift;
			pOut->SetTime(&start, &stop);
		}
		return __super::Transform(pIn, pOut);
	}

	const DWORD  input_layout = GetChannelLayout(in_wfe); // need for BassRedirect

	unsigned     audio_samplerate     = in_wfe->nSamplesPerSec;
	unsigned     audio_channels       = in_wfe->nChannels;
	DWORD        audio_layout         = input_layout;
	BYTE*        audio_data           = pDataIn;
	int          audio_samples        = pIn->GetActualDataLength() / (audio_channels * get_bytes_per_sample(audio_sampleformat));
	unsigned     audio_allsamples     = audio_samples * audio_channels;

	// input samples = 0 doesn't mean it's failed, return S_OK otherwise might screw the sound
	if (audio_samples == 0) {
		pOut->SetActualDataLength(0);
		return S_OK;
	}
	else if (audio_samples < 0) {
		return E_FAIL;
	}

	const WAVEFORMATEX* output_wfe = (WAVEFORMATEX*)pOutPin->CurrentMediaType().pbFormat;
	auto        &output_samplerate    = output_wfe->nSamplesPerSec;
	auto        &output_channels      = output_wfe->nChannels;
	const DWORD  output_layout        = GetChannelLayout(output_wfe);
	SampleFormat output_sampleformat  = GetSampleFormat(output_wfe);
	const int    output_samplesize    = pOut->GetSize() / (output_channels * get_bytes_per_sample(output_sampleformat));

	REFERENCE_TIME delay = 0;

	bool levels = (audio_layout&SPEAKER_FRONT_CENTER) && m_dCenterLevel != 1.0
				|| (audio_layout&(SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_BACK_CENTER|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT)) && m_dSurroundLevel != 1.0;

	// Mixer
	if (audio_layout != output_layout || audio_samplerate != output_samplerate || levels) {
		m_Mixer.UpdateInput(audio_sampleformat, audio_layout, audio_samplerate);
		m_Mixer.UpdateOutput(SAMPLE_FMT_FLT, output_layout, output_samplerate);

		int mix_samples = m_Mixer.CalcOutSamples(audio_samples);

		BYTE* mix_data;
		if (output_sampleformat == SAMPLE_FMT_FLT) {
			if (mix_samples > output_samplesize) {
				return E_FAIL;
			}
			mix_data = pDataOut;
		} else {
			m_buffer.ExpandSize(mix_samples * output_channels);
			mix_data = (BYTE*)m_buffer.Data();
		}

		delay = m_Mixer.GetDelay();
		mix_samples = m_Mixer.Mixing(mix_data, mix_samples, audio_data, audio_samples);

		if (!mix_samples) {
			pOut->SetActualDataLength(0);
			return S_OK;
		}
		audio_data         = mix_data;
		audio_samplerate   = output_samplerate;
		audio_channels     = output_channels;
		audio_layout       = output_layout;
		audio_samples      = mix_samples;
		audio_allsamples   = mix_samples * output_channels;
		audio_sampleformat = SAMPLE_FMT_FLT;
	}

	// Bass redirect (works in place)
	if ((m_bBassRedirect || input_layout == KSAUDIO_SPEAKER_STEREO) && CHL_CONTAINS_ALL(output_layout, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY)) {
		m_BassRedirect.UpdateInput(audio_sampleformat, audio_layout, audio_samplerate);
		m_BassRedirect.Process(audio_data, audio_samples);
	}

	// Auto volume control (works in place, requires float)
	if (m_bAutoVolumeControl) {
		if (audio_sampleformat != SAMPLE_FMT_FLT) {
			m_buffer.ExpandSize(audio_allsamples);
			convert_to_float(audio_sampleformat, audio_channels, audio_samples, audio_data, m_buffer.Data());

			audio_data = (BYTE*)m_buffer.Data();
			audio_sampleformat = SAMPLE_FMT_FLT;
		}
		audio_samples    = m_AudioNormalizer.Process((float*)audio_data, audio_samples, audio_channels);
		audio_allsamples = audio_samples * audio_channels;
	}
	// Gain (works in place)
	else if (m_dGainFactor != 1.0) {
		switch (audio_sampleformat) {
		case SAMPLE_FMT_U8:
			gain_uint8(m_dGainFactor, audio_allsamples, (uint8_t*)audio_data);
			break;
		case SAMPLE_FMT_S16:
			gain_int16(m_dGainFactor, audio_allsamples, (int16_t*)audio_data);
			break;
		case SAMPLE_FMT_S24:
			gain_int24(m_dGainFactor, audio_allsamples, audio_data);
			break;
		case SAMPLE_FMT_S32:
			gain_int32(m_dGainFactor, audio_allsamples, (int32_t*)audio_data);
			break;
		case SAMPLE_FMT_FLT:
			gain_float(m_dGainFactor, audio_allsamples, (float*)audio_data);
			break;
		}
	}

	// Copy or convert to output
	if (audio_data != pDataOut && audio_samples > output_samplesize) {
		return E_FAIL;
	}
	if (audio_data != pDataOut) {
		switch (output_sampleformat) {
		case SAMPLE_FMT_S16:
			hr = convert_to_int16(audio_sampleformat, audio_channels, audio_samples, audio_data, (int16_t*)pDataOut);
			break;
		case SAMPLE_FMT_S24:
			hr = convert_to_int24(audio_sampleformat, audio_channels, audio_samples, audio_data, pDataOut);
			break;
		case SAMPLE_FMT_S32:
			hr = convert_to_int32(audio_sampleformat, audio_channels, audio_samples, audio_data, (int32_t*)pDataOut);
			break;
		case SAMPLE_FMT_FLT:
			hr = convert_to_float(audio_sampleformat, audio_channels, audio_samples, audio_data, (float*)pDataOut);
			break;
		}
		audio_sampleformat = output_sampleformat;
	}

	pOut->SetActualDataLength(audio_allsamples * get_bytes_per_sample(audio_sampleformat));

	REFERENCE_TIME rtDur = 10000000i64 * audio_samples / audio_samplerate;
	REFERENCE_TIME rtStart, rtStop;
	if (FAILED(pIn->GetTime(&rtStart, &rtStop))) {
		rtStart = m_rtNextStart;
		rtStop  = m_rtNextStart + rtDur;
	} else if (delay) {
		rtStart -= delay;
		rtStop   = rtStart + rtDur;
	}
	m_rtNextStart = rtStop;

	rtStart += m_rtAudioTimeShift;
	rtStop  += m_rtAudioTimeShift;
	pOut->SetTime(&rtStart, &rtStop);

	return S_OK;
}

void CAudioSwitcherFilter::TransformMediaType(CMediaType& mt)
{
	if (mt.majortype == MEDIATYPE_Audio
			&& mt.formattype == FORMAT_WaveFormatEx
			&& (mt.subtype == MEDIASUBTYPE_PCM || mt.subtype == MEDIASUBTYPE_IEEE_FLOAT)) {

		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;
		WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;

		SampleFormat sampleformat = GetSampleFormat(wfe);
		if (sampleformat == SAMPLE_FMT_NONE) {
			return; // skip spdif/bitstream formats
		}
		if (m_bMixer || m_bAutoVolumeControl) {
			sampleformat = SAMPLE_FMT_FLT; // some transformations change the sample format to float
		}
		switch (sampleformat) {
		case SAMPLE_FMT_U8:
		case SAMPLE_FMT_S16:
			sampleformat = m_bInt16 ? SAMPLE_FMT_S16
			             : m_bInt32 ? SAMPLE_FMT_S32 // more successful in conversions than Int24
			             : m_bInt24 ? SAMPLE_FMT_S24
			             : m_bFloat ? SAMPLE_FMT_FLT
			             : SAMPLE_FMT_NONE;
			break;
		case SAMPLE_FMT_S24:
			sampleformat = m_bInt24 ? SAMPLE_FMT_S24
			             : m_bInt32 ? SAMPLE_FMT_S32
			             : m_bFloat ? SAMPLE_FMT_FLT
			             : m_bInt16 ? SAMPLE_FMT_S16
			             : SAMPLE_FMT_NONE;
			break;
		case SAMPLE_FMT_S32:
		case SAMPLE_FMT_S64:
			sampleformat = m_bInt32 ? SAMPLE_FMT_S32
			             : m_bFloat ? SAMPLE_FMT_FLT
			             : m_bInt24 ? SAMPLE_FMT_S24
			             : m_bInt16 ? SAMPLE_FMT_S16
			             : SAMPLE_FMT_NONE;
			break;
		case SAMPLE_FMT_FLT:
		case SAMPLE_FMT_DBL:
			sampleformat = m_bFloat ? SAMPLE_FMT_FLT
			             : m_bInt32 ? SAMPLE_FMT_S32
			             : m_bInt24 ? SAMPLE_FMT_S24
			             : m_bInt16 ? SAMPLE_FMT_S16
			             : SAMPLE_FMT_NONE;
			break;
		}
		WORD bitdeph = get_bits_per_sample(sampleformat);

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

		DWORD samplerate = wfe->nSamplesPerSec;
		if (samplerate > 192000) { // 192 kHz is limit for DirectSound
			CLSID clsid = GUID_NULL;
			CComPtr<IPin> pPinTo;
			if (SUCCEEDED(GetOutputPin()->ConnectedTo(&pPinTo)) && pPinTo) {
				clsid = GetCLSID(pPinTo);
			};

			if (clsid != CLSID_MpcAudioRenderer && clsid != CLSID_SanearAudioRenderer) {
				while (samplerate > 96000) { // 96 kHz is optimal maximum for transformations
					samplerate >>= 1; // usually it obtained 96 or 88.2 kHz
				}
			}
		}

		// write new mt.pbFormat
		if (channels <= 2 && (sampleformat == SAMPLE_FMT_U8 || sampleformat == SAMPLE_FMT_S16 || sampleformat == SAMPLE_FMT_FLT)) {
			if (mt.cbFormat != sizeof(WAVEFORMATEX)) {
				wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX));
			}

			wfe->wFormatTag      = (sampleformat == SAMPLE_FMT_FLT) ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
			wfe->nChannels       = channels;
			wfe->nSamplesPerSec  = samplerate;
			wfe->nBlockAlign     = channels * bitdeph / 8;
			wfe->nAvgBytesPerSec = wfe->nBlockAlign * samplerate;
			wfe->wBitsPerSample  = bitdeph;
			wfe->cbSize          = 0;
		}
		else {
			if (mt.cbFormat != sizeof(WAVEFORMATEXTENSIBLE)) {
				wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
			}

			wfex->Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
			wfex->Format.nChannels            = channels;
			wfex->Format.nSamplesPerSec       = samplerate;
			wfex->Format.nBlockAlign          = channels * bitdeph / 8;
			wfex->Format.nAvgBytesPerSec      = wfex->Format.nBlockAlign * samplerate;
			wfex->Format.wBitsPerSample       = bitdeph;
			wfex->Format.cbSize               = 22;
			wfex->Samples.wValidBitsPerSample = bitdeph;
			wfex->dwChannelMask               = layout;
			wfex->SubFormat                   = (sampleformat == SAMPLE_FMT_FLT) ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
		}

		// write new mt.subtype
		mt.subtype = (sampleformat == SAMPLE_FMT_FLT) ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
	}
}


STDMETHODIMP CAudioSwitcherFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAudioSwitcherFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAudioSwitcherFilter

STDMETHODIMP CAudioSwitcherFilter::SetChannelMixer(bool bMixer, int nLayout)
{
	CAutoLock cAutoLock(&m_csTransform);

	if (bMixer != m_bMixer || bMixer && nLayout != m_nMixerLayout) {
		m_bOutputFormatChanged = true;
	}

	m_bMixer = bMixer;
	if (nLayout >= SPK_MONO && nLayout <= SPK_7_1) {
		m_nMixerLayout = nLayout;
	}

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetBassRedirect(bool bBassRedirect)
{
	m_bBassRedirect = bBassRedirect;

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetLevels(double dCenterLevel_dB, double dSurroundLevel_dB)
{
	dCenterLevel_dB = std::clamp(dCenterLevel_dB, AUDIO_LEVEL_MIN, AUDIO_LEVEL_MAX);
	m_dCenterLevel = decibel2factor(dCenterLevel_dB);

	dSurroundLevel_dB = std::clamp(dSurroundLevel_dB, AUDIO_LEVEL_MIN, AUDIO_LEVEL_MAX);
	m_dSurroundLevel = decibel2factor(dSurroundLevel_dB);

	m_Mixer.SetOptions(m_dCenterLevel, m_dSurroundLevel, false);

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetAudioGain(double dGain_dB)
{
	dGain_dB = std::clamp(dGain_dB, AUDIO_GAIN_MIN, AUDIO_GAIN_MAX);
	m_dGainFactor = decibel2factor(dGain_dB);

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetAutoVolumeControl(bool bAutoVolumeControl, bool bNormBoost, int iNormLevel, int iNormRealeaseTime)
{
	m_bAutoVolumeControl	= bAutoVolumeControl;
	m_bNormBoost			= bNormBoost;
	m_iNormLevel			= std::clamp(iNormLevel, 0, 100);
	m_iNormRealeaseTime		= std::clamp(iNormRealeaseTime, 5, 10);

	m_AudioNormalizer.SetParam(m_iNormLevel, bNormBoost, m_iNormRealeaseTime);

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetOutputFormats(int iSampleFormats)
{
	CAutoLock cAutoLock(&m_csTransform);

	if (iSampleFormats & SFMT_MASK) { // at least one format must be enabled
		const bool bInt16 = !!(iSampleFormats & SFMT_INT16);
		const bool bInt24 = !!(iSampleFormats & SFMT_INT24);
		const bool bInt32 = !!(iSampleFormats & SFMT_INT32);
		const bool bFloat = !!(iSampleFormats & SFMT_FLOAT);

		if (bInt16 != m_bInt16
				|| bInt24 != m_bInt24
				|| bInt32 != m_bInt32
				|| bFloat != m_bFloat) {
			m_bOutputFormatChanged = true;
		}

		m_bInt16 = bInt16;
		m_bInt24 = bInt24;
		m_bInt32 = bInt32;
		m_bFloat = bFloat;

		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP_(REFERENCE_TIME) CAudioSwitcherFilter::GetAudioTimeShift()
{
	return m_rtAudioTimeShift;
}

STDMETHODIMP CAudioSwitcherFilter::SetAudioTimeShift(REFERENCE_TIME rtAudioTimeShift)
{
	m_rtAudioTimeShift = rtAudioTimeShift;
	return S_OK;
}

// IAMStreamSelect

STDMETHODIMP CAudioSwitcherFilter::Enable(long lIndex, DWORD dwFlags)
{
	HRESULT hr = __super::Enable(lIndex, dwFlags);

	return hr;
}
