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

#include "stdafx.h"
#include <atlbase.h>
#include <MMReg.h>
#include <Ks.h>
#include <KsMedia.h>
#include <sys/timeb.h>

#include "MpaDecFilter.h"
#include "AudioHelper.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/AudioParser.h"
#include "../../../DSUtil/SysVersion.h"
#include "Version.h"

#ifdef REGISTER_FILTER
	#include <InitGuid.h>
#endif

#include <moreuuids.h>
#include <basestruct.h>
#include <vector>

#include <ffmpeg/libavcodec/avcodec.h>
#include "AudioDecoders.h"

// option names
#define OPT_REGKEY_MpaDec   L"Software\\MPC-BE Filters\\MPC Audio Decoder"
#define OPT_SECTION_MpaDec  L"Filters\\MPC Audio Decoder"
#define OPTION_SFormat_i16  L"SampleFormat_int16"
#define OPTION_SFormat_i24  L"SampleFormat_int24"
#define OPTION_SFormat_i32  L"SampleFormat_int32"
#define OPTION_SFormat_flt  L"SampleFormat_float"
#define OPTION_AVSYNC       L"AVSync"
#define OPTION_DRC          L"DRC"
#define OPTION_SPDIF_ac3    L"SPDIF_ac3"
#define OPTION_SPDIF_eac3   L"HDMI_eac3"
#define OPTION_SPDIF_truehd L"HDMI_truehd"
#define OPTION_SPDIF_dts    L"SPDIF_dts"
#define OPTION_SPDIF_dtshd  L"HDMI_dtshd"
#define OPTION_SPDIF_ac3enc L"SPDIF_ac3enc"

#define PADDING_SIZE        AV_INPUT_BUFFER_PADDING_SIZE
/** ffmpeg\libavcodec\avcodec.h - AV_INPUT_BUFFER_PADDING_SIZE
* @ingroup lavc_decoding
* Required number of additionally allocated bytes at the end of the input bitstream for decoding.
* This is mainly needed because some optimized bitstream readers read
* 32 or 64 bit at once and could read over the end.
* Note: If the first 23 bits of the additional bytes are not 0, then damaged
* MPEG bitstreams could cause overread and segfault.
*/

#define BS_HEADER_SIZE          8
#define BS_AC3_SIZE          6144
#define BS_EAC3_SIZE        24576                       // 6144 for DD Plus * 4 for IEC 60958 frames
#define BS_MAT_FRAME_SIZE   61424                       // MAT frame size
#define BS_MAT_TRUEHD_SIZE  (BS_MAT_FRAME_SIZE + 8 + 8) // 8 header bytes + 61424 of MAT data + 8 zero byte
#define BS_MAT_TRUEHD_LIMIT (BS_MAT_TRUEHD_SIZE - 24)   // IEC total frame size - MAT end code size
#define BS_MAT_POS_MIDDLE   (BS_HEADER_SIZE + 30708)    // middle point + 8 header bytes
#define BS_DTSHD_SIZE       32768

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	// MPEG Audio
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MP3},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MPEG1AudioPayload},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MPEG1Payload},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MPEG1Packet},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MPEG2_AUDIO},
	// AC3, E-AC3, TrueHD
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_DOLBY_DDPLUS},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_DOLBY_DDPLUS},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DOLBY_DDPLUS},
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_DOLBY_TRUEHD},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_DOLBY_TRUEHD},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DOLBY_TRUEHD},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_WAVE_DOLBY_AC3},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MLP},
	// DTS
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_DTS},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_DTS},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DTS},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DTS2},
	// LPCM
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_HDMV_LPCM_AUDIO},
	// AAC
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_RAW_AAC1},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_RAW_AAC1},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_RAW_AAC1},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_LATM_AAC},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_AAC_ADTS},
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_mp4a},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_mp4a},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_mp4a},
	// AMR
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_AMR},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_SAMR},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_SAWB},
	// PS2Audio
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_PS2_PCM},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_PS2_PCM},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PS2_PCM},
	{&MEDIATYPE_MPEG2_PACK,			&MEDIASUBTYPE_PS2_ADPCM},
	{&MEDIATYPE_MPEG2_PES,			&MEDIASUBTYPE_PS2_ADPCM},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PS2_ADPCM},
	// Vorbis
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_Vorbis2},
	// Flac
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_FLAC_FRAMED},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_FLAC},
	// NellyMoser
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_NELLYMOSER},
	// PCM
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PCM_NONE},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PCM_RAW},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PCM_TWOS},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PCM_SOWT},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PCM_IN24},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PCM_IN32},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PCM_FL32},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_PCM_FL64},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_IEEE_FLOAT}, // only for 64-bit float PCM
	// ADPCM
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_IMA4},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_ADPCM_SWF},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_IMA_AMV},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_ADX_ADPCM},
	// RealAudio
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_14_4},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_28_8},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_ATRC},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_COOK},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DNET},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_SIPR},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_SIPR_WAVE},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_RAAC},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_RACP},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_RALF},
	// ALAC
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_ALAC},
	// ALS
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_ALS},
	// ATRAC3
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_ATRAC3},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_ATRAC3plus},
	// QDesign Music
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_QDMC},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_QDM2},
	// WavPack
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_WAVPACK4},
	// MusePack
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MPC7},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MPC8},
	// APE
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_APE},
	// TAK
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_TAK},
	// TTA
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_TTA1},
	// Shorten
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_Shorten},
	// DSP Group TrueSpeech
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_TRUESPEECH},
	// Voxware MetaSound
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_VOXWARE_RT29},
	// Windows Media Audio 9 Professional
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_WMAUDIO3},
	// Windows Media Audio Lossless
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_WMAUDIO_LOSSLESS},
	// Windows Media Audio 1, 2
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MSAUDIO1},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_WMAUDIO2},
	// Windows Media Audio Voice
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_WMSP1},
	// Bink Audio
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_BINKA_DCT},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_BINKA_RDFT},
	// Indeo Audio
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_IAC},
	// Opus Audio
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_OPUS},
	// Speex Audio
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_SPEEX},
	// DSD
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DSD},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DSDL},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DSDM},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DSD1},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DSD8},
	// DST
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_DST},
	// A-law/mu-law
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_ALAW},
	{&MEDIATYPE_Audio,				&MEDIASUBTYPE_MULAW},
};

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_IEEE_FLOAT},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMpaDecFilter), MPCAudioDecName, MERIT_NORMAL + 1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, &__uuidof(CMpaDecFilter), CreateInstance<CMpaDecFilter>, nullptr, &sudFilter[0]},
	{L"CMpaDecPropertyPage", &__uuidof(CMpaDecSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CMpaDecSettingsWnd> >},
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

//

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

static const MPCSampleFormat SamplefmtToMPC[SAMPLE_FMT_NB] = {
	SF_PCM16, // <-- SAMPLE_FMT_U8
	SF_PCM16, // <-- SAMPLE_FMT_S16
	SF_PCM32, // <-- SAMPLE_FMT_S32
	SF_FLOAT, // <-- SAMPLE_FMT_FLT
	SF_FLOAT, // <-- SAMPLE_FMT_DBL
	SF_PCM16, // <-- SAMPLE_FMT_U8P
	SF_PCM16, // <-- SAMPLE_FMT_S16P
	SF_PCM32, // <-- SAMPLE_FMT_S32P
	SF_FLOAT, // <-- SAMPLE_FMT_FLTP
	SF_FLOAT, // <-- SAMPLE_FMT_DBLP
	SF_PCM32, // <-- SAMPLE_FMT_S64
	SF_PCM32, // <-- SAMPLE_FMT_S64P
	SF_PCM24, // <-- SAMPLE_FMT_S24
//  SF_PCM24  // <-- SAMPLE_FMT_S24P
};

static const SampleFormat MPCtoSamplefmt[sfcount] = {
	SAMPLE_FMT_S16, // <-- SF_PCM16
	SAMPLE_FMT_S24, // <-- SF_PCM24
	SAMPLE_FMT_S32, // <-- SF_PCM32
	SAMPLE_FMT_FLT  // <-- SF_FLOAT
};

CMpaDecFilter::CMpaDecFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CTransformFilter(L"CMpaDecFilter", lpunk, __uuidof(this))
	, m_Subtype(GUID_NULL)
	, m_CodecId(AV_CODEC_ID_NONE)
	, m_InternalSampleFormat(SAMPLE_FMT_NONE)
	, m_rtStart(0)
	, m_dStartOffset(0.0)
	, m_bDiscontinuity(FALSE)
	, m_bResync(FALSE)
	, m_bResyncTimestamp(FALSE)
	, m_buff(PADDING_SIZE)
	, m_bNeedCheck(TRUE)
	, m_bHasVideo(FALSE)
	, m_dRate(1.0)
	, m_bFlushing(FALSE)
	, m_bNeedSyncPoint(FALSE)
	, m_DTSHDProfile(0)
	, m_rtStartInput(INVALID_TIME)
	, m_rtStopInput(INVALID_TIME)
	, m_rtStartInputCache(INVALID_TIME)
	, m_rtStopInputCache(INVALID_TIME)
	, m_bUpdateTimeCache(TRUE)
	, m_FFAudioDec(this)
	, m_bAVSync(true)
	, m_bDRC(false)
{
	if (phr) {
		*phr = S_OK;
	}

	m_pInput = DNew CDeCSSInputPin(L"CDeCSSInputPin", this, phr, L"In");
	if (!m_pInput) {
		*phr = E_OUTOFMEMORY;
	}
	if (FAILED(*phr)) {
		return;
	}

	m_pOutput = DNew CTransformOutputPin(L"CTransformOutputPin", this, phr, L"Out");
	if (!m_pOutput) {
		*phr = E_OUTOFMEMORY;
	}
	if (FAILED(*phr)) {
		delete m_pInput, m_pInput = nullptr;
		return;
	}

	// read settings
#ifdef REGISTER_FILTER
	m_bSampleFmt[SF_PCM16] = true;
	m_bSampleFmt[SF_PCM24] = false;
	m_bSampleFmt[SF_PCM32] = false;
	m_bSampleFmt[SF_FLOAT] = false;

	CRegKey key;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_MpaDec, KEY_READ)) {
		DWORD dw;
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SFormat_i16, dw)) {
			m_bSampleFmt[SF_PCM16] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SFormat_i24, dw)) {
			m_bSampleFmt[SF_PCM24] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SFormat_i32, dw)) {
			m_bSampleFmt[SF_PCM32] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SFormat_flt, dw)) {
			m_bSampleFmt[SF_FLOAT] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_AVSYNC, dw)) {
			m_bAVSync = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_DRC, dw)) {
			m_bDRC = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_ac3, dw)) {
			m_bSPDIF[ac3] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_eac3, dw)) {
			m_bSPDIF[eac3] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_truehd, dw)) {
			m_bSPDIF[truehd] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_dts, dw)) {
			m_bSPDIF[dts] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_dtshd, dw)) {
			m_bSPDIF[dtshd] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_ac3enc, dw)) {
			m_bSPDIF[ac3enc] = !!dw;
		}
	}

	if (!(m_bSampleFmt[SF_PCM16] || m_bSampleFmt[SF_PCM24] || m_bSampleFmt[SF_PCM32] || m_bSampleFmt[SF_FLOAT])) {
		m_bSampleFmt[SF_PCM16] = true;
	}
#else
	m_bAVSync              = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_AVSYNC, m_bAVSync);
	m_bDRC                 = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_DRC, m_bDRC);
	m_bSPDIF[ac3]          = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_ac3, m_bSPDIF[ac3]);
	m_bSPDIF[eac3]         = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_eac3, m_bSPDIF[eac3]);
	m_bSPDIF[truehd]       = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_truehd, m_bSPDIF[truehd]);
	m_bSPDIF[dts]          = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_dts, m_bSPDIF[dts]);
	m_bSPDIF[dtshd]        = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_dtshd, m_bSPDIF[dtshd]);
	m_bSPDIF[ac3enc]       = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_ac3enc, m_bSPDIF[ac3enc]);
#endif
}

CMpaDecFilter::~CMpaDecFilter()
{
	m_FFAudioDec.StreamFinish();
}

STDMETHODIMP CMpaDecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMpaDecFilter)
		QI(IExFilterConfig)
		QI(ISpecifyPropertyPages)
		QI(ISpecifyPropertyPages2)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMpaDecFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	enum AVCodecID nCodecId = m_FFAudioDec.GetCodecId();
	if (nCodecId != AV_CODEC_ID_NONE) {
		HRESULT hr = S_OK;
		if (!ProcessBitstream(nCodecId, hr, TRUE)) {
			ProcessFFmpeg(nCodecId, TRUE);
		}
	}

	return __super::EndOfStream();
}

HRESULT CMpaDecFilter::BeginFlush()
{
	m_bFlushing = TRUE;
	return __super::BeginFlush();
}

HRESULT CMpaDecFilter::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_buff.Clear();
	m_FFAudioDec.FlushBuffers();
	m_Mixer.FlushBuffers();
	m_encbuff.clear();

	HRESULT hr = __super::EndFlush();
	m_bFlushing = FALSE;
	return hr;
}

HRESULT CMpaDecFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	m_ps2_state.sync = false;
	ZeroMemory(&m_hdmi_bitstream, sizeof(m_hdmi_bitstream));

	m_bResync = TRUE;
	m_rtStart = 0; // LOOKATTHIS // reset internal timer?

	m_bNeedSyncPoint = m_FFAudioDec.NeedSyncPoint();

	m_dRate = dRate > 0.0 ? dRate : 1.0;

	m_DTSHDProfile = 0;

	m_bUpdateTimeCache = TRUE;

	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CMpaDecFilter::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	if (m_bNeedCheck) {
		m_bNeedCheck = FALSE;

		memset(&m_bBitstreamSupported, true, sizeof(m_bBitstreamSupported));

		CComPtr<IPin> pPinRenderer;
		CComPtr<IPin> pPin = m_pOutput;
		for (CComPtr<IBaseFilter> pBF = this; pBF = GetDownStreamFilter(pBF, pPin); pPin = GetFirstPin(pBF, PINDIR_OUTPUT)) {
			if (IsAudioWaveRenderer(pBF) && SUCCEEDED(pPin->ConnectedTo(&pPinRenderer)) && pPinRenderer) {
				break;
			}
		}

		if (pPinRenderer) {
			CMediaType mtRenderer;
			if (SUCCEEDED(pPinRenderer->ConnectionMediaType(&mtRenderer)) && mtRenderer.pbFormat) {
				memset(&m_bBitstreamSupported, false, sizeof(m_bBitstreamSupported));

				CMediaType mt = CreateMediaTypeSPDIF();
				m_bBitstreamSupported[SPDIF]  = pPinRenderer->QueryAccept(&mt) == S_OK;

				mt = CreateMediaTypeHDMI(IEC61937_EAC3);
				m_bBitstreamSupported[EAC3]   = pPinRenderer->QueryAccept(&mt) == S_OK;

				mt = CreateMediaTypeHDMI(IEC61937_TRUEHD);
				m_bBitstreamSupported[TRUEHD] = pPinRenderer->QueryAccept(&mt) == S_OK;

				mt = CreateMediaTypeHDMI(IEC61937_DTSHD);
				m_bBitstreamSupported[DTSHD]  = pPinRenderer->QueryAccept(&mt) == S_OK;

				pPinRenderer->QueryAccept(&mtRenderer);
			}
		}
	}

	HRESULT hr;

	AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA) {
		return m_pOutput->Deliver(pIn);
	}

	AM_MEDIA_TYPE* pmt;
	if (SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt) {
		CMediaType mt(*pmt);
		m_pInput->SetMediaType(&mt);
		if (m_Subtype != pmt->subtype) {
			m_Subtype = pmt->subtype;
			m_CodecId = FindCodec(m_Subtype);
			m_bFallBackToPCM = false;
		}
		DeleteMediaType(pmt);
		pmt = nullptr;
	}

	BYTE* pDataIn = nullptr;
	if (FAILED(hr = pIn->GetPointer(&pDataIn))) {
		return hr;
	}

	long len = pIn->GetActualDataLength();
	// skip empty packet (StreamBufferSource can produce empty data)
	if (len == 0) {
		return S_OK;
	}

	(static_cast<CDeCSSInputPin*>(m_pInput))->StripPacket(pDataIn, len);

	REFERENCE_TIME rtStart = INVALID_TIME, rtStop = INVALID_TIME;
	hr = pIn->GetTime(&rtStart, &rtStop);

#if 0
	if (SUCCEEDED(hr)) {
		DLog(L"CMpaDecFilter::Receive(): [%10I64d => %10I64d], %I64d", rtStart, rtStop, rtStop - rtStart);
	} else {
		DLog(L"CMpaDecFilter::Receive(): frame without timestamp");
	}
#endif

	if (pIn->IsDiscontinuity() == S_OK
			|| (m_bNeedSyncPoint && S_OK == pIn->IsSyncPoint())) {
		DLog(L"CMpaDecFilter::Receive() : Discontinuity, flushing decoder");
		m_buff.Clear();
		m_FFAudioDec.FlushBuffers();
		m_Mixer.FlushBuffers();
		m_encbuff.clear();
		m_bDiscontinuity = TRUE;
		m_bResync = TRUE;
		if (FAILED(hr)) {
			DLog(L"    -> Discontinuity without timestamp");
		}

		if (m_bNeedSyncPoint && pIn->IsSyncPoint() == S_OK) {
			DLog(L"    -> Got SyncPoint, resuming decoding");
			m_bNeedSyncPoint = FALSE;
		}
	}

	if (m_bResync && SUCCEEDED(hr)) {
		DLog(L"CMpaDecFilter::Receive() : Resync Request - [%I64d -> %I64d], buffer : %Iu", m_rtStart, rtStart, m_buff.Size());

		if (m_rtStart != INVALID_TIME && rtStart != m_rtStart) {
			m_bDiscontinuity = TRUE;
		}

		m_rtStart = rtStart;
		m_rtStartInputCache = INVALID_TIME;
		m_dStartOffset = 0.0;
		m_bResync = FALSE;
		m_bResyncTimestamp = TRUE;
	}

	m_rtStartInput = SUCCEEDED(hr) ? rtStart : INVALID_TIME;
	m_rtStopInput = SUCCEEDED(hr) ? rtStop : INVALID_TIME;

	m_buff.Append(pDataIn, len);

	if (m_CodecId != AV_CODEC_ID_NONE) {
		if (ProcessBitstream(m_CodecId, hr)) {
			return hr;
		}
		return ProcessFFmpeg(m_CodecId);
	}

	if (m_Subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
		hr = ProcessDvdLPCM();
	} else if (m_Subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
		hr = ProcessHdmvLPCM();
	} else if (m_Subtype == MEDIASUBTYPE_PS2_PCM) {
		hr = ProcessPS2PCM();
	} else if (m_Subtype == MEDIASUBTYPE_PS2_ADPCM) {
		hr = ProcessPS2ADPCM();
	} else if (m_Subtype == MEDIASUBTYPE_PCM_NONE ||
			   m_Subtype == MEDIASUBTYPE_PCM_RAW) {
		hr = ProcessPCMraw();
	} else if (m_Subtype == MEDIASUBTYPE_PCM_TWOS ||
			   m_Subtype == MEDIASUBTYPE_PCM_IN24 ||
			   m_Subtype == MEDIASUBTYPE_PCM_IN32) {
		hr = ProcessPCMintBE();
	} else if (m_Subtype == MEDIASUBTYPE_PCM_SOWT) {
		hr = ProcessPCMintLE();
	} else if (m_Subtype == MEDIASUBTYPE_PCM_FL32 ||
			   m_Subtype == MEDIASUBTYPE_PCM_FL64) {
		hr = ProcessPCMfloatBE();
	} else if (m_Subtype == MEDIASUBTYPE_IEEE_FLOAT) {
		hr = ProcessPCMfloatLE();
	}

	return hr;
}

HRESULT CMpaDecFilter::CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin)
{
	if (direction == PINDIR_OUTPUT) {
		m_bHasVideo = HasMediaType(m_pGraph, MEDIATYPE_Video) || HasMediaType(m_pGraph, MEDIASUBTYPE_MPEG2_VIDEO);
	}
	else if (direction == PINDIR_INPUT) {
		CMediaType mt;
		if (S_OK == pReceivePin->ConnectionMediaType(&mt)) {
			m_Subtype = mt.subtype;
			m_CodecId = FindCodec(m_Subtype);
		}
	}

	return __super::CompleteConnect(direction, pReceivePin);
}

void CMpaDecFilter::UpdateCacheTimeStamp()
{
	m_rtStartInputCache = m_rtStartInput;
	m_rtStopInputCache = m_rtStopInput;
	m_rtStartInput = m_rtStopInput = INVALID_TIME;
	m_bUpdateTimeCache = FALSE;
}

void CMpaDecFilter::ClearCacheTimeStamp()
{
	m_rtStartInputCache = INVALID_TIME;
	m_bUpdateTimeCache = TRUE;
}

BOOL CMpaDecFilter::ProcessBitstream(enum AVCodecID nCodecId, HRESULT& hr, BOOL bEOF/* = FALSE*/)
{
	if (m_bFallBackToPCM) {
		return FALSE;
	}

	if (m_bBitstreamSupported[SPDIF]) {
		if (GetSPDIF(ac3) && nCodecId == AV_CODEC_ID_AC3) {
			hr = bEOF ? S_OK : ProcessAC3_SPDIF();
			return TRUE;
		}
		if (GetSPDIF(dts) && nCodecId == AV_CODEC_ID_DTS) {
			hr = ProcessDTS_SPDIF(bEOF);
			return m_bFallBackToPCM ? FALSE : TRUE;
		}
	}

	if (m_bBitstreamSupported[EAC3] && GetSPDIF(eac3) && nCodecId == AV_CODEC_ID_EAC3) {
		hr = ProcessEAC3_SPDIF(bEOF);
		return TRUE;
	}

	if (m_bBitstreamSupported[TRUEHD] && GetSPDIF(truehd) && nCodecId == AV_CODEC_ID_TRUEHD) {
		hr = bEOF ? S_OK : ProcessTrueHD_SPDIF();
		return TRUE;
	}

	return FALSE;
}

HRESULT CMpaDecFilter::ProcessDvdLPCM()
{
	const WAVEFORMATEX* wfein = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	unsigned channels = wfein->nChannels;
	if (channels < 1 || channels > 8) {
		return E_FAIL;
	}

	unsigned src_size = m_buff.Size();
	unsigned dst_size = 0;
	SampleFormat out_sf = SAMPLE_FMT_NONE;

	if (wfein->cbSize == 8) {
		const DVDALPCMFORMAT* fmt = (DVDALPCMFORMAT*)wfein;
		if (fmt->GroupAssignment > 20) {
			return E_FAIL;
		}
		DVDA_INFO a;
		a.groupassign = fmt->GroupAssignment;
		a.channels1   = CountBits(s_scmap_dvda[a.groupassign].layout1);
		a.samplerate1 = fmt->wfe.nSamplesPerSec;
		a.bitdepth1   = fmt->wfe.wBitsPerSample;
		a.channels2   = CountBits(s_scmap_dvda[a.groupassign].layout2);
		a.samplerate2 = fmt->nSamplesPerSec2;
		a.bitdepth2   = fmt->wBitsPerSample2;
		ASSERT(a.channels1 + a.channels2 == channels);
		DWORD layout = s_scmap_dvda[a.groupassign].layout1 | s_scmap_dvda[a.groupassign].layout2;

		auto dst = DecodeDvdaLPCM(dst_size, out_sf, m_buff.Data(), src_size, a);
		if (out_sf == SAMPLE_FMT_NONE) {
			return E_FAIL;
		}
		if (!dst) {
			return S_FALSE;
		}

		m_buff.RemoveHead(m_buff.Size() - src_size);
		DLogIf(src_size > 0, L"ProcessDvdLPCM(): %u bytes not processed", src_size);

		return Deliver(dst.get(), dst_size, m_rtStartInput, out_sf, wfein->nSamplesPerSec, channels, layout);
	}
	else {
		auto dst = DecodeDvdLPCM(dst_size, out_sf, m_buff.Data(), src_size, channels, wfein->wBitsPerSample);
		if (out_sf == SAMPLE_FMT_NONE) {
			return E_FAIL;
		}
		if (!dst) {
			return S_FALSE;
		}

		m_buff.RemoveHead(m_buff.Size() - src_size);
		DLogIf(src_size > 0, L"ProcessDvdLPCM(): %u bytes not processed", src_size);

		return Deliver(dst.get(), dst_size, m_rtStartInput, out_sf, wfein->nSamplesPerSec, channels);
	}
}

HRESULT CMpaDecFilter::ProcessHdmvLPCM() // Blu ray LPCM
{
	WAVEFORMATEX_HDMV_LPCM* wfein = (WAVEFORMATEX_HDMV_LPCM*)m_pInput->CurrentMediaType().Format();
	if (wfein->channel_conf >= _countof(s_scmap_hdmv) || !s_scmap_hdmv[wfein->channel_conf].layout) {
		return E_FAIL;
	}

	unsigned src_size = m_buff.Size();
	unsigned dst_size = 0;
	SampleFormat out_sf = SAMPLE_FMT_NONE;
	auto dst = DecodeHdmvLPCM(dst_size, out_sf, m_buff.Data(), src_size, wfein->nChannels, wfein->wBitsPerSample, wfein->channel_conf);
	if (out_sf == SAMPLE_FMT_NONE) {
		return E_FAIL;
	}
	if (!dst) {
		return S_FALSE;
	}

	m_buff.RemoveHead(m_buff.Size() - src_size);
	DLogIf(src_size > 0, L"ProcessHdmvLPCM(): %u bytes not processed", src_size);

	return Deliver(dst.get(), dst_size, m_rtStartInput, out_sf, wfein->nSamplesPerSec, wfein->nChannels, s_scmap_hdmv[wfein->channel_conf].layout);
}

HRESULT CMpaDecFilter::ProcessFFmpeg(enum AVCodecID nCodecId, BOOL bEOF/* = FALSE*/)
{
	HRESULT hr = S_OK;

	if (m_FFAudioDec.GetCodecId() == AV_CODEC_ID_NONE) {
		if (m_FFAudioDec.Init(nCodecId, &m_pInput->CurrentMediaType())) {
			m_FFAudioDec.SetDRC(GetDynamicRangeControl());
		} else {
			return S_OK;
		}
	} else if (m_FFAudioDec.NeedReinit()) {
		if (!m_FFAudioDec.Init(nCodecId, nullptr)) {
			return S_OK;
		}
	}

	if (bEOF) {
		hr = m_FFAudioDec.SendData(nullptr, 0);
		if (hr == S_OK) {
			std::vector<BYTE> output;
			SampleFormat samplefmt = SAMPLE_FMT_NONE;

			REFERENCE_TIME rtStart = INVALID_TIME;
			while (S_OK == (hr = m_FFAudioDec.ReceiveData(output, samplefmt, rtStart))) {
				if (output.size()) {
					hr = Deliver(output.data(), output.size(), rtStart, samplefmt, m_FFAudioDec.GetSampleRate(), m_FFAudioDec.GetChannels(), m_FFAudioDec.GetChannelMask());
					output.clear();
				}
			}
		}

		return S_OK;
	}

	BYTE* const base = m_buff.Data();
	BYTE* end = base + m_buff.Size();
	BYTE* p = base;

	// RealAudio
	CPaddedBuffer buffRA(AV_INPUT_BUFFER_PADDING_SIZE);
	bool isRA = false;
	if (nCodecId == AV_CODEC_ID_ATRAC3 || nCodecId == AV_CODEC_ID_COOK || nCodecId == AV_CODEC_ID_SIPR) {
		hr = m_FFAudioDec.RealPrepare(p, int(end - p), buffRA);
		if (hr == S_OK) {
			p = buffRA.Data();
			end = p + buffRA.Size();
			isRA = true;

			m_rtStartInput = m_rtStartInputCache;
			m_rtStartInputCache = AV_NOPTS_VALUE;
		} else if (hr == E_FAIL) {
			if (m_rtStartInputCache == INVALID_TIME) {
				m_rtStartInputCache = m_rtStartInput;
			}
			return S_OK;
		} else {
			// trying continue decoding without any pre-processing ...
		}
	}

	while (p < end) {
		int out_size = 0;

		hr = m_FFAudioDec.SendData(p, int(end - p), &out_size);
		if (S_OK == hr) {
			std::vector<BYTE> output;
			SampleFormat samplefmt = SAMPLE_FMT_NONE;

			REFERENCE_TIME rtStart = INVALID_TIME;
			while (S_OK == (hr = m_FFAudioDec.ReceiveData(output, samplefmt, rtStart))) {
				if (output.size()) {
					hr = Deliver(output.data(), output.size(), rtStart, samplefmt, m_FFAudioDec.GetSampleRate(), m_FFAudioDec.GetChannels(), m_FFAudioDec.GetChannelMask());
					output.clear();
				}
			}
		} else {
			m_bResync = TRUE;
			if (!out_size) {
				m_buff.Clear();
				return S_OK;
			}
		}

		p += out_size;
	}

	if (isRA) { // RealAudio
		p = base + buffRA.Size();
		end = base + m_buff.Size();
	}

	m_buff.RemoveHead(p - base);

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessAC3_SPDIF()
{
	HRESULT hr;

	BYTE* const base = m_buff.Data();
	BYTE* end = base + m_buff.Size();
	BYTE* p = base;

	while (p + 8 <= end) { // 8 =  AC3 header size + 1
		audioframe_t aframe;
		int size = ParseAC3Header(p, &aframe);

		if (size == 0) {
			p++;
			continue;
		}

		if (m_bUpdateTimeCache) {
			UpdateCacheTimeStamp();
		}

		if (p + size > end) {
			break;
		}

		if (FAILED(hr = DeliverBitstream(p, size, m_rtStartInputCache, IEC61937_AC3, aframe.samplerate, 1536))) {
			return hr;
		}

		p += size;
	}

	m_buff.RemoveHead(p - base);

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessEAC3_SPDIF(BOOL bEOF/* = FALSE*/)
{
	HRESULT hr = S_OK;

	auto DeliverPacket = [&] {
		hr = DeliverBitstream(m_hdmi_bitstream.buf, m_hdmi_bitstream.size, m_rtStartInputCache, IEC61937_EAC3, m_hdmi_bitstream.EAC3State.samplerate, m_hdmi_bitstream.EAC3State.samples * m_hdmi_bitstream.EAC3State.repeat);
		m_hdmi_bitstream.size = 0;

		m_hdmi_bitstream.EAC3State.count      = 0;
		m_hdmi_bitstream.EAC3State.repeat     = 0;
		m_hdmi_bitstream.EAC3State.samples    = 0;
		m_hdmi_bitstream.EAC3State.samplerate = 0;
	};

	if (bEOF) {
		if (m_hdmi_bitstream.EAC3State.Ready()) {
			DeliverPacket();
		}
		return hr;
	}

	BYTE* const base = m_buff.Data();
	BYTE* end = base + m_buff.Size();
	BYTE* p = base;

	while (p + 8 <= end) {
		audioframe_t aframe;
		int size = ParseEAC3Header(p, &aframe);

		bool bAC3Frame = false;
		if (size == 0) {
			size = ParseAC3Header(p, &aframe);
			if (size == 0) {
				p++;
				continue;
			}

			bAC3Frame = true;
			aframe.param1 = EAC3_FRAME_TYPE_INDEPENDENT;
		}

		if (aframe.param1 == EAC3_FRAME_TYPE_INDEPENDENT && m_hdmi_bitstream.EAC3State.Ready()) {
			DeliverPacket();
			if (FAILED(hr)) {
				return hr;
			}
		}

		if (m_hdmi_bitstream.EAC3State.count == 0 && aframe.param1 == EAC3_FRAME_TYPE_DEPENDENT) {
			DLog(L"CMpaDecFilter::ProcessEAC3_SPDIF() : Ignoring dependent frame without independent frame.");
			p += size;
			continue;
		}

		if (m_bUpdateTimeCache) {
			UpdateCacheTimeStamp();
		}

		if (p + size > end) {
			break;
		}

		static const uint8_t eac3_repeat[4] = {6, 3, 2, 1};
		int repeat = 1;
		if (!bAC3Frame && (p[4] & 0xc0) != 0xc0) { /* fscod */
			repeat = eac3_repeat[(p[4] & 0x30) >> 4]; /* numblkscod */
		}

		if (aframe.param1 == EAC3_FRAME_TYPE_INDEPENDENT) {
			m_hdmi_bitstream.EAC3State.count++;
			if (!m_hdmi_bitstream.EAC3State.repeat) {
				m_hdmi_bitstream.EAC3State.repeat = repeat;
				m_hdmi_bitstream.EAC3State.samples = aframe.samples;
				m_hdmi_bitstream.EAC3State.samplerate = aframe.samplerate;
			}
		}

		if (m_hdmi_bitstream.size + size <= BS_EAC3_SIZE - BS_HEADER_SIZE) {
			memcpy(m_hdmi_bitstream.buf + m_hdmi_bitstream.size, p, size);
			m_hdmi_bitstream.size += size;
		} else {
			ASSERT(0);
		}
		p += size;
	}

	m_buff.RemoveHead(p - base);

	return S_OK;
}

static const BYTE mat_start_code[20]  = { 0x07, 0x9E, 0x00, 0x03, 0x84, 0x01, 0x01, 0x01, 0x80, 0x00, 0x56, 0xA5, 0x3B, 0xF4, 0x81, 0x83, 0x49, 0x80, 0x77, 0xE0 };
static const BYTE mat_middle_code[12] = { 0xC3, 0xC1, 0x42, 0x49, 0x3B, 0xFA, 0x82, 0x83, 0x49, 0x80, 0x77, 0xE0 };
static const BYTE mat_end_code[24]    = { 0xC3, 0xC2, 0xC0, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void CMpaDecFilter::MATWriteHeader()
{
	ASSERT(m_hdmi_bitstream.size == 0);

	// skip 8 header bytes and write MAT start code
	memcpy(m_hdmi_bitstream.buf + BS_HEADER_SIZE, mat_start_code, sizeof(mat_start_code));
	m_hdmi_bitstream.size = BS_HEADER_SIZE + sizeof(mat_start_code);

	// unless the start code falls into the padding,  its considered part of the current MAT frame
	// Note that audio frames are not always aligned with MAT frames, so we might already have a partial frame at this point
	DWORD dwSize = BS_HEADER_SIZE + sizeof(mat_start_code);
	m_hdmi_bitstream.TrueHDMATState.mat_framesize += dwSize;

	// The MAT metadata counts as padding, if we're scheduled to write any, which mean the start bytes should reduce any further padding.
	if (m_hdmi_bitstream.TrueHDMATState.padding > 0) {
		// if the header fits into the padding of the last frame, just reduce the amount of needed padding
		if (m_hdmi_bitstream.TrueHDMATState.padding > dwSize) {
			m_hdmi_bitstream.TrueHDMATState.padding -= dwSize;
			m_hdmi_bitstream.TrueHDMATState.mat_framesize = 0;
		} else { // otherwise, consume all padding and set the size of the next MAT frame to the remaining data
			m_hdmi_bitstream.TrueHDMATState.mat_framesize = (dwSize - m_hdmi_bitstream.TrueHDMATState.padding);
			m_hdmi_bitstream.TrueHDMATState.padding = 0;
		}
	}
}

void CMpaDecFilter::MATWritePadding()
{
	if (m_hdmi_bitstream.TrueHDMATState.padding > 0) {
		static BYTE padding[5120] = {};

		memset(padding, 0, m_hdmi_bitstream.TrueHDMATState.padding);
		int remaining = MATFillDataBuffer(padding, m_hdmi_bitstream.TrueHDMATState.padding, true);
		ASSERT(remaining <= 5120);

		// not all padding could be written to the buffer, write it later
		if (remaining >= 0) {
			m_hdmi_bitstream.TrueHDMATState.padding = remaining;
			m_hdmi_bitstream.TrueHDMATState.mat_framesize = 0;
		} else {// more padding then requested was written, eg. there was a MAT middle/end marker that needed to be written
			m_hdmi_bitstream.TrueHDMATState.padding = 0;
			m_hdmi_bitstream.TrueHDMATState.mat_framesize = -remaining;
		}
	}
}

void CMpaDecFilter::MATAppendData(const BYTE *p, int size)
{
	memcpy(m_hdmi_bitstream.buf + m_hdmi_bitstream.size, p, size);
	m_hdmi_bitstream.size += size;
	m_hdmi_bitstream.TrueHDMATState.mat_framesize += size;
}

int CMpaDecFilter::MATFillDataBuffer(const BYTE *p, int size, bool padding)
{
	if (m_hdmi_bitstream.size >= BS_MAT_TRUEHD_LIMIT) {
		return size;
	}

	int remaining = size;

	// Write MAT middle marker, if needed
	// The MAT middle marker always needs to be in the exact same spot, any audio data will be split.
	// If we're currently writing padding, then the marker will be considered as padding data and reduce the amount of padding still required.
	if (m_hdmi_bitstream.size <= BS_MAT_POS_MIDDLE && m_hdmi_bitstream.size + size > BS_MAT_POS_MIDDLE) {
		// write as much data before the middle code as we can
		int nBytesBefore = BS_MAT_POS_MIDDLE - m_hdmi_bitstream.size;
		MATAppendData(p, nBytesBefore);
		remaining -= nBytesBefore;

		// write the MAT middle code
		MATAppendData(mat_middle_code, sizeof(mat_middle_code));

		// if we're writing padding, deduct the size of the code from it
		if (padding) {
			remaining -= sizeof(mat_middle_code);
		}

		// write remaining data after the MAT marker
		if (remaining > 0) {
			MATAppendData(p + nBytesBefore, remaining);
			remaining = 0;
		}

		return remaining;
	}

	// not enough room in the buffer to write all the data, write as much as we can and add the MAT footer
	if (m_hdmi_bitstream.size + size > BS_MAT_TRUEHD_LIMIT) {
		// write as much data before the middle code as we can
		int nBytesBefore = BS_MAT_TRUEHD_LIMIT - m_hdmi_bitstream.size;
		MATAppendData(p, nBytesBefore);
		remaining -= nBytesBefore;

		// write the MAT end code
		MATAppendData(mat_end_code, sizeof(mat_end_code));

		// MAT markers don't displace padding, so reduce the amount of padding
		if (padding) {
			remaining -= sizeof(mat_end_code);
		}

		// any remaining data will be written in future calls
		return remaining;
	}

	MATAppendData(p, size);

	return 0;
}

HRESULT CMpaDecFilter::MATDeliverPacket()
{
	HRESULT hr = S_OK;
	if (m_hdmi_bitstream.size > 0) {
		// Deliver MAT packet
		hr = DeliverBitstream(m_hdmi_bitstream.buf + BS_HEADER_SIZE, BS_MAT_FRAME_SIZE, m_rtStartInputCache, IEC61937_TRUEHD, 0, 0);
		m_hdmi_bitstream.size = 0;
	}

	return hr;
}

HRESULT CMpaDecFilter::ProcessTrueHD_SPDIF()
{
	HRESULT hr;

	BYTE* const base = m_buff.Data();
	BYTE* end = base + m_buff.Size();
	BYTE* p = base;

	while (p + 16 <= end) {
		audioframe_t aframe;
		int size = ParseMLPHeader(p, &aframe);
		if (size > 0) {
			// sync frame
			m_hdmi_bitstream.TrueHDMATState.sync = true;

			// get the ratebits from the sync frame
			m_hdmi_bitstream.TrueHDMATState.ratebits = p[8] >> 4;
		} else {
			int ac3size = ParseAC3Header(p);
			if (ac3size == 0) {
				ac3size = ParseEAC3Header(p);
			}
			if (ac3size > 0) {
				if (p + ac3size > end) {
					break;
				}
				p += ac3size;
				continue; // skip ac3 frames
			}
		}

		if (size == 0 && m_hdmi_bitstream.TrueHDMATState.sync) {
			// get not sync frame size
			size = ((p[0] << 8 | p[1]) & 0xfff) * 2;
		}

		if (size < 8) {
			p++;
			continue;
		}

		if (m_bUpdateTimeCache) {
			UpdateCacheTimeStamp();
		}

		if (p + size > end) {
			break;
		}

		uint16_t frame_time = (p[2] << 8 | p[3]);
		uint32_t space_size = 0;

		// compute final padded size for the previous frame, if any
		if (m_hdmi_bitstream.TrueHDMATState.prev_frametime_valid) {
			space_size = ((frame_time - m_hdmi_bitstream.TrueHDMATState.prev_frametime) & 0xff) * (64 >> (m_hdmi_bitstream.TrueHDMATState.ratebits & 7));
		}

		// compute padding (ie. difference to the size of the previous frame)
		m_hdmi_bitstream.TrueHDMATState.padding += (space_size - m_hdmi_bitstream.TrueHDMATState.prev_mat_framesize) & 0xffff;
		ASSERT((space_size - m_hdmi_bitstream.TrueHDMATState.prev_mat_framesize) <= 0xffff);

		// store frame time of the previous frame
		m_hdmi_bitstream.TrueHDMATState.prev_frametime = frame_time;
		m_hdmi_bitstream.TrueHDMATState.prev_frametime_valid = true;

		// if the buffer is as full as its going to be, add the MAT footer and flush it
		if (m_hdmi_bitstream.size >= BS_MAT_TRUEHD_LIMIT) {
			// write footer and remove it from the padding
			MATAppendData(mat_end_code, sizeof(mat_end_code));
			m_hdmi_bitstream.TrueHDMATState.padding -= sizeof(mat_end_code);

			// flush packet out
			hr = MATDeliverPacket();
			if (FAILED(hr)) {
				return hr;
			}
		}

		// Write the MAT header into the fresh buffer
		if (m_hdmi_bitstream.size == 0) {
			MATWriteHeader();
		}

		// write padding of the previous frame (if any)
		MATWritePadding();

		// Buffer is full, submit it
		if (m_hdmi_bitstream.size >= (BS_MAT_TRUEHD_SIZE - BS_HEADER_SIZE)) {
			hr = MATDeliverPacket();
			if (FAILED(hr)) {
				return hr;
			}

			// and setup a new buffer
			MATWriteHeader();
			MATWritePadding();
		}

		// write actual audio data to the buffer
		int remaining = MATFillDataBuffer(p, size);

		// not all data could be written
		if (remaining) {
			// flush out old data
			hr = MATDeliverPacket();
			if (FAILED(hr)) {
				return hr;
			}

			// .. setup a new buffer
			MATWriteHeader();

			// and write the remaining data
			MATFillDataBuffer(p + (size - remaining), remaining);
		}

		// store the size of the current MAT frame, so we can add padding later
		m_hdmi_bitstream.TrueHDMATState.prev_mat_framesize = m_hdmi_bitstream.TrueHDMATState.mat_framesize;
		m_hdmi_bitstream.TrueHDMATState.mat_framesize = 0;

		p += size;
	}

	m_buff.RemoveHead(p - base);

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessDTS_SPDIF(BOOL bEOF/* = FALSE*/)
{
	HRESULT hr;

	BYTE* const base = m_buff.Data();
	BYTE* end = base + m_buff.Size();
	BYTE* p = base;

	BOOL bUseDTSHDBitstream = GetSPDIF(dtshd) && m_bBitstreamSupported[DTSHD];

	while (p + 16 <= end) {
		audioframe_t aframe;
		int size = ParseDTSHeader(p, &aframe);
		if (size == 0) {
			if (!m_DTSHDProfile) {
				const auto sizehd = ParseDTSHDHeader(p);
				if (sizehd) {
					if (p + sizehd <= end) {
						audioframe_t dtshdaframe;
						ParseDTSHDHeader(p, sizehd, &dtshdaframe);
						m_DTSHDProfile = dtshdaframe.param2;
						if (m_DTSHDProfile == DCA_PROFILE_EXPRESS) {
							m_bFallBackToPCM = true;
							return S_OK;
						}
					} else {
						break;
					}
				}
			}

			p++;
			continue;
		}

		if (m_bUpdateTimeCache) {
			UpdateCacheTimeStamp();
		}

		int sizehd = 0;
		if (p + size + 16 <= end) {
			sizehd = ParseDTSHDHeader(p + size);
			if (sizehd
					&& !m_DTSHDProfile
					&& (p + size + sizehd <= end)) {
				audioframe_t dtshdaframe;
				ParseDTSHDHeader(p + size, sizehd, &dtshdaframe);
				m_DTSHDProfile = dtshdaframe.param2;
			}
		} else if (!bEOF){
			break; // need more data for check DTS-HD syncword
		}

		if (p + size + sizehd > end) {
			if (bEOF && !bUseDTSHDBitstream && p + size <= end) {
				sizehd = 0;
			} else {
				break;
			}
		}

		bool usehdmi = sizehd && bUseDTSHDBitstream;
		if (usehdmi) {
			if (FAILED(hr = DeliverBitstream(p, size + sizehd, m_rtStartInputCache, IEC61937_DTSHD, aframe.samplerate, aframe.samples))) {
				return hr;
			}
		} else {
			BYTE type;
			switch (aframe.samples) {
				case  512:
					type = IEC61937_DTS1;
					break;
				case 1024:
					type = IEC61937_DTS2;
					break;
				case 2048:
					type = IEC61937_DTS3;
					break;
				default:
					DLog(L"CMpaDecFilter:ProcessDTS_SPDIF() - framelength is not supported");
					return E_FAIL;
			}
			if (FAILED(hr = DeliverBitstream(p, size, m_rtStartInputCache, type, aframe.samplerate >> aframe.param2, aframe.samples))) {
				return hr;
			}
		}

		p += (size + sizehd);
	}

	m_buff.RemoveHead(p - base);

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMraw() //'raw '
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t size       = m_buff.Size();
	size_t nSamples   = size * 8 / wfe->wBitsPerSample;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	std::vector<NoInitByte> outBuff;
	outBuff.resize(size);

	switch (wfe->wBitsPerSample) {
		case 8: // unsigned 8-bit
			out_sf = SAMPLE_FMT_U8;
			memcpy(outBuff.data(), m_buff.Data(), size);
		break;
		case 16: { // signed big-endian 16-bit
			out_sf = SAMPLE_FMT_S16;
			uint16_t* pIn  = (uint16_t*)m_buff.Data();
			uint16_t* pOut = (uint16_t*)outBuff.data();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = _byteswap_ushort(pIn[i]);
			}
		}
		break;
	}

	HRESULT hr;
	if (S_OK != (hr = Deliver(&outBuff.front().value, size, m_rtStartInput, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.Clear();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMintBE() // 'twos', big-endian 'in24' and 'in32'
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t nSamples   = m_buff.Size() * 8 / wfe->wBitsPerSample;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	size_t outSize = nSamples * (wfe->wBitsPerSample <= 16 ? 2 : 4); // convert to 16 and 32-bit
	std::vector<NoInitByte> outBuff;
	outBuff.resize(outSize);

	switch (wfe->wBitsPerSample) {
		case 8: { //signed 8-bit
			out_sf = SAMPLE_FMT_S16;
			int8_t*  pIn  = (int8_t*)m_buff.Data();
			int16_t* pOut = (int16_t*)outBuff.data();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = (int16_t)pIn[i] << 8;
			}
		}
		break;
		case 16: { // signed big-endian 16-bit
			out_sf = SAMPLE_FMT_S16;
			uint16_t* pIn  = (uint16_t*)m_buff.Data(); // signed take as an unsigned to shift operations.
			uint16_t* pOut = (uint16_t*)outBuff.data();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = _byteswap_ushort(pIn[i]);
			}
		}
		break;
		case 24: { // signed big-endian 24-bit
			out_sf = SAMPLE_FMT_S32;
			uint8_t*  pIn  = (uint8_t*)m_buff.Data();
			uint32_t* pOut = (uint32_t*)outBuff.data();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = (uint32_t)pIn[3 * i]     << 24 |
						  (uint32_t)pIn[3 * i + 1] << 16 |
						  (uint32_t)pIn[3 * i + 2] << 8;
			}
		}
		break;
		case 32: { // signed big-endian 32-bit
			out_sf = SAMPLE_FMT_S32;
			uint32_t* pIn  = (uint32_t*)m_buff.Data(); // signed take as an unsigned to shift operations.
			uint32_t* pOut = (uint32_t*)outBuff.data();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = _byteswap_ulong(pIn[i]);
			}
		}
		break;
	}

	HRESULT hr;
	if (S_OK != (hr = Deliver(&outBuff.front().value, outSize, m_rtStartInput, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.Clear();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMintLE() // 'sowt', little-endian 'in24' and 'in32'
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t nSamples   = m_buff.Size() * 8 / wfe->wBitsPerSample;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	size_t outSize = nSamples * (wfe->wBitsPerSample <= 16 ? 2 : 4); // convert to 16 and 32-bit
	std::vector<NoInitByte> outBuff;
	outBuff.resize(outSize);

	switch (wfe->wBitsPerSample) {
		case 8: { //signed 8-bit
			out_sf = SAMPLE_FMT_S16;
			int8_t*  pIn  = (int8_t*)m_buff.Data();
			int16_t* pOut = (int16_t*)outBuff.data();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = (int16_t)pIn[i] << 8;
			}
		}
		break;
		case 16: // signed little-endian 16-bit
			out_sf = SAMPLE_FMT_S16;
			memcpy(outBuff.data(), m_buff.Data(), outSize);
			break;
		case 24: // signed little-endian 24-bit
			out_sf = SAMPLE_FMT_S32;
			convert_int24_to_int32((int32_t*)outBuff.data(), (uint8_t*)m_buff.Data(), nSamples);
			break;
		case 32: // signed little-endian 32-bit
			out_sf = SAMPLE_FMT_S32;
			memcpy(outBuff.data(), m_buff.Data(), outSize);
			break;
	}

	HRESULT hr;
	if (S_OK != (hr = Deliver(&outBuff.front().value, outSize, m_rtStartInput, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.Clear();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMfloatBE() // big-endian 'fl32' and 'fl64'
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t size       = m_buff.Size();
	size_t nSamples   = size * 8 / wfe->wBitsPerSample;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	std::vector<NoInitByte> outBuff;
	outBuff.resize(size);

	switch (wfe->wBitsPerSample) {
		case 32: {
			out_sf = SAMPLE_FMT_FLT;
			uint32_t* pIn  = (uint32_t*)m_buff.Data();
			uint32_t* pOut = (uint32_t*)outBuff.data();
			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = _byteswap_ulong(pIn[i]);
			}
		}
		break;
		case 64: {
			out_sf = SAMPLE_FMT_DBL;
			uint64_t* pIn  = (uint64_t*)m_buff.Data();
			uint64_t* pOut = (uint64_t*)outBuff.data();
			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = _byteswap_uint64(pIn[i]);
			}
		}
		break;
	}

	HRESULT hr;
	if (S_OK != (hr = Deliver(&outBuff.front().value, size, m_rtStartInput, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.Clear();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMfloatLE() // little-endian 'fl32' and 'fl64'
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t size = m_buff.Size();

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	std::vector<NoInitByte> outBuff;
	outBuff.resize(size);

	switch (wfe->wBitsPerSample) {
		case 32:
			out_sf = SAMPLE_FMT_FLT;
			break;
		case 64:
			out_sf = SAMPLE_FMT_DBL;
			break;
	}
	memcpy(outBuff.data(), m_buff.Data(), size);

	HRESULT hr;
	if (S_OK != (hr = Deliver(&outBuff.front().value, size, m_rtStartInput, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.Clear();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPS2PCM()
{
	BYTE* const base = m_buff.Data();
	BYTE* end = base + m_buff.Size();
	BYTE* p = base;

	WAVEFORMATEXPS2* wfe = (WAVEFORMATEXPS2*)m_pInput->CurrentMediaType().Format();
	size_t size = wfe->dwInterleave * wfe->nChannels;

	SampleFormat out_sf = SAMPLE_FMT_S16P;
	std::vector<NoInitByte> outBuff;
	outBuff.resize(size);

	while (p + size <= end) {
		DWORD* dw = (DWORD*)p;

		if (dw[0] == 'dhSS') {
			p += dw[1] + 8;
		} else if (dw[0] == 'dbSS') {
			p += 8;
			m_ps2_state.sync = true;
		} else {
			if (m_ps2_state.sync) {
				memcpy(outBuff.data(), p, size);
			} else {
				memset(outBuff.data(), 0, size);
			}

			HRESULT hr;
			if (S_OK != (hr = Deliver(&outBuff.front().value, size, m_rtStartInput, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
				return hr;
			}
			m_rtStartInput = INVALID_TIME;

			p += size;
		}
	}

	m_buff.RemoveHead(p - base);

	return S_OK;
}

static void decodeps2adpcm(ps2_state_t& s, int channel, BYTE* pin, int16_t* pout)
{
	int tbl_index = pin[0]>>4;
	int shift     = pin[0]&0xf;
	int unk       = pin[1]; // ?
	UNREFERENCED_PARAMETER(unk);

	if (tbl_index >= 10) {
		ASSERT(0);
		return;
	}
	// if (unk == 7) {ASSERT(0); return;} // ???

	static int s_tbl[] = {
		0, 0,  60, 0,  115, -52,  98, -55,  122, -60,
		0, 0, -60, 0, -115,  52, -98,  55, -122,  60
	};

	int* tbl = &s_tbl[tbl_index*2];
	int& a = s.a[channel];
	int& b = s.b[channel];

	for (int i = 0; i < 28; i++) {
		short input = (short)(((pin[2+i/2] >> ((i&1) << 2)) & 0xf) << 12) >> shift;
		int output = (a * tbl[1] + b * tbl[0]) / 64 + input;

		a = b;
		b = output;

		*pout++ = (int16_t)std::clamp(output, INT16_MIN, (int)INT16_MAX);
	}
}

HRESULT CMpaDecFilter::ProcessPS2ADPCM()
{
	BYTE* const base = m_buff.Data();
	BYTE* end = base + m_buff.Size();
	BYTE* p = base;

	WAVEFORMATEXPS2* wfe = (WAVEFORMATEXPS2*)m_pInput->CurrentMediaType().Format();
	size_t size  = wfe->dwInterleave * wfe->nChannels;
	int samples = wfe->dwInterleave * 14 / 16 * 2;
	int channels = wfe->nChannels;

	SampleFormat out_sf = SAMPLE_FMT_S16P;
	size_t outSize = samples * channels * sizeof(int16_t);
	std::vector<NoInitByte> outBuff;
	outBuff.resize(outSize);
	int16_t* pOut = (int16_t*)outBuff.data();

	while (p + size <= end) {
		DWORD* dw = (DWORD*)p;

		if (dw[0] == 'dhSS') {
			p += dw[1] + 8;
		} else if (dw[0] == 'dbSS') {
			p += 8;
			m_ps2_state.sync = true;
		} else {
			if (m_ps2_state.sync) {
				for (int ch = 0, j = 0, k = 0; ch < channels; ch++, j += wfe->dwInterleave) {
					for (DWORD i = 0; i < wfe->dwInterleave; i += 16, k += 28) {
						decodeps2adpcm(m_ps2_state, ch, p + i + j, pOut + k);
					}
				}
			} else {
				memset(outBuff.data(), 0, outSize);
			}

			HRESULT hr;
			if (S_OK != (hr = Deliver(&outBuff.front().value, outSize, m_rtStartInput, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
				return hr;
			}
			m_rtStartInput = INVALID_TIME;

			p += size;
		}
	}

	m_buff.RemoveHead(p - base);

	return S_OK;
}

HRESULT CMpaDecFilter::GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData)
{
	HRESULT hr;
	*pData = nullptr;

	if (FAILED(hr = m_pOutput->GetDeliveryBuffer(pSample, nullptr, nullptr, 0))
			|| FAILED(hr = (*pSample)->GetPointer(pData))) {
		return hr;
	}

	AM_MEDIA_TYPE* pmt = nullptr;
	if (SUCCEEDED((*pSample)->GetMediaType(&pmt)) && pmt) {
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
		pmt = nullptr;
	}

	return S_OK;
}

HRESULT CMpaDecFilter::Deliver(BYTE* pBuff, const size_t size, const REFERENCE_TIME rtStartInput, const SampleFormat sfmt, const DWORD nSamplesPerSec, const WORD nChannels, DWORD dwChannelMask/* = 0*/)
{
	if (m_bFlushing) {
		return S_FALSE;
	}

	if (dwChannelMask == 0) {
		dwChannelMask = GetDefChannelMask(nChannels);
	}
	ASSERT(nChannels == av_popcount(dwChannelMask));

	m_InternalSampleFormat = sfmt;

	if (m_bBitstreamSupported[SPDIF] && GetSPDIF(ac3enc) && nChannels > 2) { // do not encode mono and stereo
		return AC3Encode(pBuff, size, rtStartInput, sfmt, nSamplesPerSec, nChannels, dwChannelMask);
	}

	int nSamples = size / (nChannels * get_bytes_per_sample(sfmt));

	if (m_bResyncTimestamp && rtStartInput != INVALID_TIME) {
		m_rtStart = rtStartInput;
		m_bResyncTimestamp = FALSE;
	}

	if (rtStartInput != INVALID_TIME) {
		const REFERENCE_TIME rtJitter = m_rtStart - rtStartInput;
		m_faJitter.Sample(rtJitter);
		const REFERENCE_TIME rtJitterMin = m_faJitter.AbsMinimum();
		if (m_bAVSync && m_bHasVideo && abs(rtJitterMin)> m_JitterLimit) {
			DLog(L"CMpaDecFilter::Deliver() : corrected A/V sync by %I64d", rtJitterMin);
			m_rtStart -= rtJitterMin;
			m_faJitter.OffsetValues(-rtJitterMin);
			m_bDiscontinuity = TRUE;
		}
	}

	REFERENCE_TIME rtStart, rtStop;
	CalculateDuration(nSamples, nSamplesPerSec, rtStart, rtStop);

	if (rtStart < 0) {
		return S_OK;
	}

	MPCSampleFormat out_mpcsf = SelectOutputFormat(SamplefmtToMPC[sfmt]);
	const SampleFormat out_sf = MPCtoSamplefmt[out_mpcsf];

	BYTE*  pDataIn  = pBuff;

	CMediaType mt = CreateMediaType(out_mpcsf, nSamplesPerSec, nChannels, dwChannelMask);
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	HRESULT hr;
	if (FAILED(hr = ReconnectOutput(nSamples, mt))) {
		return hr;
	}

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = nullptr;
	if (FAILED(GetDeliveryBuffer(&pOut, &pDataOut))) {
		return E_FAIL;
	}

	if (hr == S_OK) {
		m_pOutput->SetMediaType(&mt);
		pOut->SetMediaType(&mt);
	}

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(nullptr, nullptr);

	pOut->SetPreroll(FALSE);
	pOut->SetDiscontinuity(m_bDiscontinuity);
	m_bDiscontinuity = FALSE;
	pOut->SetSyncPoint(TRUE);

	pOut->SetActualDataLength(nSamples * nChannels * wfe->wBitsPerSample / 8);

	WAVEFORMATEX* wfeout = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();
	ASSERT(wfeout->nChannels == wfe->nChannels);
	ASSERT(wfeout->nSamplesPerSec == wfe->nSamplesPerSec);
	UNREFERENCED_PARAMETER(wfeout);

	switch (out_mpcsf) {
		case SF_PCM16:
			convert_to_int16(sfmt, nChannels, nSamples, pDataIn, (int16_t*)pDataOut);
			break;
		case SF_PCM24:
			convert_to_int24(sfmt, nChannels, nSamples, pDataIn, pDataOut);
			break;
		case SF_PCM32:
			convert_to_int32(sfmt, nChannels, nSamples, pDataIn, (int32_t*)pDataOut);
			break;
		case SF_FLOAT:
			convert_to_float(sfmt, nChannels, nSamples, pDataIn, (float*)pDataOut);
			break;
	}

	return m_pOutput->Deliver(pOut);
}

HRESULT CMpaDecFilter::DeliverBitstream(BYTE* pBuff, const int size, const REFERENCE_TIME rtStartInput, const WORD type, const int sample_rate, const int samples)
{
	ClearCacheTimeStamp();

	HRESULT hr;
	WORD subtype  = 0;
	bool isDTSWAV = false;
	bool isHDMI   = false;
	int  length   = 0;

	switch (type) {
		case IEC61937_AC3:
			length = BS_AC3_SIZE;
			break;
		case IEC61937_DTS1:
		case IEC61937_DTS2:
		case IEC61937_DTS3:
			if (sample_rate == 44100 && size == samples * 4) { // DTSWAV
				length = size;
				isDTSWAV = true;
			} else while (length < size + 16) {
				length += 2048;
			}
			break;
		case IEC61937_DTSHD:
			length  = BS_DTSHD_SIZE;
			subtype = 4;
			isHDMI  = true;
			break;
		case IEC61937_EAC3:
			length = BS_EAC3_SIZE;
			isHDMI = true;
			break;
		case IEC61937_TRUEHD:
			length = BS_MAT_TRUEHD_SIZE;
			isHDMI = true;
			break;
		default:
			DLog(L"CMpaDecFilter::DeliverBitstream() - type is not supported");
			return E_INVALIDARG;
	}

	CMediaType mt;
	if (isHDMI) {
		mt = CreateMediaTypeHDMI(type);
	} else {
		mt = CreateMediaTypeSPDIF(sample_rate);
	}

	if (FAILED(hr = ReconnectOutput(length, mt))) {
		return hr;
	}

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = nullptr;
	if (FAILED(GetDeliveryBuffer(&pOut, &pDataOut))) {
		return E_FAIL;
	}

	if (isDTSWAV) {
		memcpy(pDataOut, pBuff, size);
	} else {
		memset(pDataOut + BS_HEADER_SIZE + size, 0, length - (BS_HEADER_SIZE + size)); // Fill after the input buffer with zeros if any extra bytes

		int index = 0;
		// Fill the 8 bytes (4 words) of IEC header
		WORD* pDataOutW = (WORD*)pDataOut;
		pDataOutW[index++] = 0xf872;
		pDataOutW[index++] = 0x4e1f;
		pDataOutW[index++] = type | subtype << 8;
		if (type == IEC61937_DTSHD) {
			pDataOutW[index++] = (size & ~0xf) + 0x18; // (size without 12 extra bytes) & 0xf + 0x18
			// begin dts-hd start code
			pDataOutW[index++] = 0x0100;
			pDataOutW[index++] = 0;
			pDataOutW[index++] = 0;
			pDataOutW[index++] = 0;
			pDataOutW[index++] = 0xfefe;
			// end dts-hd start code
			pDataOutW[index++] = size;
		} else if (type == IEC61937_EAC3 || type == IEC61937_TRUEHD) {
			pDataOutW[index++] = size;
		} else {
			pDataOutW[index++] = size * 8;
		}
		_swab((char*)pBuff, (char*)&pDataOutW[index], size & ~1);
		if (size & 1) { // _swab doesn't like odd number.
			pDataOut[index * 2 + size - 1] = 0;
			pDataOut[index * 2 + size] = pBuff[size - 1];
		}
	}

	if (m_bResyncTimestamp && rtStartInput != INVALID_TIME) {
		m_rtStart = rtStartInput;
		m_bResyncTimestamp = FALSE;
	}

	if (rtStartInput != INVALID_TIME) {
		const REFERENCE_TIME rtJitter = m_rtStart - rtStartInput;
		m_faJitter.Sample(rtJitter);
		const REFERENCE_TIME rtJitterMin = m_faJitter.AbsMinimum();
		if (m_bAVSync && m_bHasVideo && abs(rtJitterMin)> m_JitterLimit) {
			DLog(L"CMpaDecFilter::DeliverBitstream() : corrected A/V sync by %I64d", rtJitterMin);
			m_rtStart -= rtJitterMin;
			m_faJitter.OffsetValues(-rtJitterMin);
			m_bDiscontinuity = TRUE;
		}
	}

	REFERENCE_TIME rtStart, rtStop;
	CalculateDuration(samples, sample_rate, rtStart, rtStop, type == IEC61937_TRUEHD);

	if (rtStart < 0) {
		return S_OK;
	}

	if (hr == S_OK) {
		m_pOutput->SetMediaType(&mt);
		pOut->SetMediaType(&mt);
	}

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(nullptr, nullptr);

	pOut->SetPreroll(FALSE);
	pOut->SetDiscontinuity(m_bDiscontinuity);
	m_bDiscontinuity = FALSE;
	pOut->SetSyncPoint(TRUE);

	pOut->SetActualDataLength(length);

	return m_pOutput->Deliver(pOut);
}

HRESULT CMpaDecFilter::AC3Encode(BYTE* pBuff, const size_t size, REFERENCE_TIME rtStartInput, const SampleFormat sfmt, const DWORD nSamplesPerSec, const WORD nChannels, const DWORD dwChannelMask)
{
	DWORD new_layout     = m_AC3Enc.SelectLayout(dwChannelMask);
	WORD  new_channels   = av_popcount(new_layout);
	DWORD new_samplerate = m_AC3Enc.SelectSamplerate(nSamplesPerSec);

	if (!m_AC3Enc.OK()) {
		m_AC3Enc.Init(new_samplerate, new_layout);
		m_encbuff.clear();
	}

	int nSamples = size / (nChannels * get_bytes_per_sample(sfmt));

	size_t remain = m_encbuff.size();
	float* p;

	if (new_layout == dwChannelMask && new_samplerate == nSamplesPerSec) {
		int added  = size / get_bytes_per_sample(sfmt);
		m_encbuff.resize(remain + added);
		p = m_encbuff.data() + remain;

		convert_to_float(sfmt, nChannels, nSamples, pBuff, p);
	} else {
		m_Mixer.UpdateInput(sfmt, dwChannelMask, nSamplesPerSec);
		m_Mixer.UpdateOutput(SAMPLE_FMT_FLT, new_layout, new_samplerate);
		int added = m_Mixer.CalcOutSamples(nSamples) * new_channels;
		m_encbuff.resize(remain + added);
		p = m_encbuff.data() + remain;

		added = m_Mixer.Mixing((BYTE*)p, added, pBuff, nSamples) * new_channels;
		if (!added) {
			m_encbuff.resize(remain);
			return S_OK;
		}
		m_encbuff.resize(remain + added);
	}

	std::vector<BYTE> output;

	while (m_AC3Enc.Encode(m_encbuff, output) == S_OK) {
		DeliverBitstream(output.data(), output.size(), rtStartInput, IEC61937_AC3, new_samplerate, 1536);
		rtStartInput = INVALID_TIME;
	}

	return S_OK;
}

HRESULT CMpaDecFilter::ReconnectOutput(int nSamples, CMediaType& mt)
{
	HRESULT hr;

	CComQIPtr<IMemInputPin> pPin = m_pOutput->GetConnected();
	if (!pPin) {
		return E_NOINTERFACE;
	}

	CComPtr<IMemAllocator> pAllocator;
	if (FAILED(hr = pPin->GetAllocator(&pAllocator)) || !pAllocator) {
		return hr;
	}

	ALLOCATOR_PROPERTIES props, actual;
	if (FAILED(hr = pAllocator->GetProperties(&props))) {
		return hr;
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();
	long cbBuffer = nSamples * wfe->nBlockAlign;

	if (mt != m_pOutput->CurrentMediaType() || cbBuffer > props.cbBuffer) {
		if (cbBuffer > props.cbBuffer) {
			props.cBuffers = 4;
			props.cbBuffer = cbBuffer*3/2;

			if (FAILED(hr = m_pOutput->DeliverBeginFlush())
					|| FAILED(hr = m_pOutput->DeliverEndFlush())
					|| FAILED(hr = pAllocator->Decommit())
					|| FAILED(hr = pAllocator->SetProperties(&props, &actual))
					|| FAILED(hr = pAllocator->Commit())) {
				return hr;
			}

			if (props.cBuffers > actual.cBuffers || props.cbBuffer > actual.cbBuffer) {
				NotifyEvent(EC_ERRORABORT, hr, 0);
				return E_FAIL;
			}
		}

		return S_OK;
	}

	return S_FALSE;
}

CMediaType CMpaDecFilter::CreateMediaType(MPCSampleFormat mpcsf, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask)
{
	CMediaType mt;

	mt.majortype  = MEDIATYPE_Audio;
	mt.subtype    = mpcsf == SF_FLOAT ? MEDIASUBTYPE_IEEE_FLOAT : MEDIASUBTYPE_PCM;
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEXTENSIBLE wfex;
	//memset(&wfex, 0, sizeof(wfex));

	WAVEFORMATEX& wfe  = wfex.Format;
	wfe.nChannels      = nChannels;
	wfe.nSamplesPerSec = nSamplesPerSec;
	switch (mpcsf) {
		default:
		case SF_PCM16:
			wfe.wBitsPerSample = 16;
			break;
		case SF_PCM24:
			wfe.wBitsPerSample = 24;
			break;
		case SF_PCM32:
		case SF_FLOAT:
			wfe.wBitsPerSample = 32;
			break;
	}
	wfe.nBlockAlign     = nChannels * wfe.wBitsPerSample / 8;
	wfe.nAvgBytesPerSec = nSamplesPerSec * wfe.nBlockAlign;

	if (nChannels <= 2 && dwChannelMask <= 0x4 && (mpcsf == SF_PCM16 || mpcsf == SF_FLOAT)) {
		// WAVEFORMATEX
		wfe.wFormatTag = (mpcsf == SF_FLOAT) ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
		wfe.cbSize = 0;

		mt.SetFormat((BYTE*)&wfe, sizeof(wfe));
	} else {
		// WAVEFORMATEXTENSIBLE
		wfex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wfex.Format.cbSize = sizeof(wfex) - sizeof(wfex.Format); // 22
		wfex.Samples.wValidBitsPerSample = wfex.Format.wBitsPerSample;
		if (dwChannelMask == 0) {
			dwChannelMask = GetDefChannelMask(nChannels);
		}
		wfex.dwChannelMask = dwChannelMask;
		wfex.SubFormat = mt.subtype;

		mt.SetFormat((BYTE*)&wfex, sizeof(wfex));
	}
	mt.SetSampleSize(wfex.Format.nBlockAlign);

	return mt;
}

CMediaType CMpaDecFilter::CreateMediaTypeSPDIF(DWORD nSamplesPerSec)
{
	if (nSamplesPerSec % 11025 == 0) {
		nSamplesPerSec = 44100;
	} else {
		nSamplesPerSec = 48000;
	}
	CMediaType mt = CreateMediaType(SF_PCM16, nSamplesPerSec, 2, SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
	((WAVEFORMATEX*)mt.pbFormat)->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
	return mt;
}

CMediaType CMpaDecFilter::CreateMediaTypeHDMI(WORD type)
{
	// some info here - http://msdn.microsoft.com/en-us/library/windows/desktop/dd316761%28v=vs.85%29.aspx
	// but we use WAVEFORMATEXTENSIBLE structure
	CMediaType mt;
	mt.majortype  = MEDIATYPE_Audio;
	mt.subtype    = MEDIASUBTYPE_PCM;
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEXTENSIBLE wfex;
	memset(&wfex, 0, sizeof(wfex));

	GUID subtype = GUID_NULL;

	switch(type) {
	case IEC61937_DTSHD:
		wfex.Format.nChannels = 8;
		wfex.dwChannelMask    = KSAUDIO_SPEAKER_7POINT1_SURROUND;
		subtype = KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD;
		break;
	case IEC61937_EAC3:
		wfex.Format.nChannels = 2;
		wfex.dwChannelMask    = KSAUDIO_SPEAKER_STEREO;
		subtype = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS;
		break;
	case IEC61937_TRUEHD:
		wfex.Format.nChannels = 8;
		wfex.dwChannelMask    = KSAUDIO_SPEAKER_7POINT1_SURROUND;
		subtype = KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP;
		break;
	default:
		ASSERT(0);
		break;
	}

	if (subtype != GUID_NULL) {
		wfex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
		wfex.Format.nSamplesPerSec       = 192000;
		wfex.Format.wBitsPerSample       = 16;
		wfex.Format.nBlockAlign          = wfex.Format.nChannels * wfex.Format.wBitsPerSample / 8;
		wfex.Format.nAvgBytesPerSec      = wfex.Format.nSamplesPerSec * wfex.Format.nBlockAlign;
		wfex.Format.cbSize               = sizeof(wfex) - sizeof(wfex.Format);
		wfex.Samples.wValidBitsPerSample = wfex.Format.wBitsPerSample;
		wfex.SubFormat = subtype;
	}

	mt.SetSampleSize(1);
	mt.SetFormat((BYTE*)&wfex, sizeof(wfex.Format) + wfex.Format.cbSize);

	return mt;
}

void CMpaDecFilter::CalculateDuration(int samples, int sample_rate, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop, BOOL bIsTrueHDBitstream/* = FALSE*/)
{
	if (bIsTrueHDBitstream) {
		// Delivery Timestamps
		// TrueHD frame size, 24 * 0.83333ms
		const REFERENCE_TIME duration = 200000i64;
		rtStart = m_rtStart, rtStop = m_rtStart + duration;
		m_rtStart += (REFERENCE_TIME)(duration / m_dRate);
	} else {
		// Length of the sample
		const double dDuration = (double)samples / sample_rate * 10000000.0;
		m_dStartOffset += fmod(dDuration, 1.0);

		// Delivery Timestamps
		rtStart = m_rtStart, rtStop = m_rtStart + (REFERENCE_TIME)(dDuration + 0.5);

		// Compute next start time
		m_rtStart += (REFERENCE_TIME)(dDuration / m_dRate);
		// If the offset reaches one (100ns), add it to the next frame
		if (m_dStartOffset > 0.5) {
			m_rtStart++;
			m_dStartOffset -= 1.0;
		}
	}
}

#ifdef REGISTER_FILTER

const MPCSampleFormat SFmtPrority[sfcount][sfcount] = {
	{ SF_PCM16, SF_PCM32, SF_PCM24, SF_FLOAT }, // int16
	{ SF_PCM24, SF_PCM32, SF_FLOAT, SF_PCM16 }, // int24
	{ SF_PCM32, SF_PCM24, SF_FLOAT, SF_PCM16 }, // int32
	{ SF_FLOAT, SF_PCM32, SF_PCM24, SF_PCM16 }, // float
};

MPCSampleFormat CMpaDecFilter::SelectOutputFormat(MPCSampleFormat mpcsf)
{
	CAutoLock cAutoLock(&m_csProps);

	if (mpcsf >= 0 && mpcsf < sfcount) {
		for (int i = 0; i < sfcount; i++) {
			if (m_bSampleFmt[SFmtPrority[mpcsf][i]]) {
				return SFmtPrority[mpcsf][i];
			}
		}
	}

	return SF_PCM16;
}

#else

MPCSampleFormat CMpaDecFilter::SelectOutputFormat(MPCSampleFormat mpcsf)
{
	if (mpcsf >= 0 && mpcsf < sfcount) {
		return mpcsf;
	}

	return SF_PCM16;
}

#endif

HRESULT CMpaDecFilter::CheckInputType(const CMediaType* mtIn)
{
	if (mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
		if (mtIn->FormatLength() >= sizeof(WAVEFORMATEX)) {
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)mtIn->Format();
			if (wfe->nChannels < 1 || wfe->nChannels > 8
				|| (wfe->wBitsPerSample != 16 && wfe->wBitsPerSample != 20 && wfe->wBitsPerSample != 24)) {
				return VFW_E_TYPE_NOT_ACCEPTED;
			}
			if (mtIn->FormatLength() == sizeof(DVDALPCMFORMAT)) {
				auto fmt = (DVDALPCMFORMAT*)mtIn->Format();
				if (fmt->GroupAssignment > 20) {
					return VFW_E_TYPE_NOT_ACCEPTED;
				}
			}
		}
	} else if (mtIn->subtype == MEDIASUBTYPE_PS2_ADPCM) {
		WAVEFORMATEXPS2* wfe = (WAVEFORMATEXPS2*)mtIn->Format();
		if (wfe->dwInterleave & 0xf) { // has to be a multiple of the block size (16 bytes)
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	} else if (mtIn->subtype == MEDIASUBTYPE_IEEE_FLOAT) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mtIn->Format();
		if (wfe->wBitsPerSample != 64) {    // only for 64-bit float PCM
			return VFW_E_TYPE_NOT_ACCEPTED; // not needed any decoders for 32-bit float
		}
	}

	for (int i = 0; i < _countof(sudPinTypesIn); i++) {
		if (*sudPinTypesIn[i].clsMajorType == mtIn->majortype
				&& *sudPinTypesIn[i].clsMinorType == mtIn->subtype) {
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpaDecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn))
		   && mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_PCM
		   || mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_IEEE_FLOAT
		   ? S_OK
		   : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpaDecFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	/*
	CMediaType& mt    = m_pInput->CurrentMediaType();
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();
	UNREFERENCED_PARAMETER(wfe);
	*/

	pProperties->cBuffers = 4;
	pProperties->cbBuffer = 192000 * 8 * (32/8) / 10; // 192KHz 8ch 32bps 100ms
	pProperties->cbAlign  = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
	if (FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) {
		return hr;
	}

	return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		   ? E_FAIL
		   : NOERROR;
}

HRESULT CMpaDecFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition > 1) {
		return VFW_S_NO_MORE_ITEMS;
	}

	CMediaType mt = m_pInput->CurrentMediaType();
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();
	if (wfe == nullptr) {
		return E_INVALIDARG;
	}

	if (iPosition == 0) {
		const GUID& subtype = mt.subtype;
		if (GetSPDIF(ac3) && (subtype == MEDIASUBTYPE_DOLBY_AC3 || subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3)
				|| GetSPDIF(dts) && (subtype == MEDIASUBTYPE_DTS || subtype == MEDIASUBTYPE_DTS2)
				|| GetSPDIF(ac3enc) && wfe->nChannels > 2) { // do not encode mono and stereo
			*pmt = CreateMediaTypeSPDIF(wfe->nSamplesPerSec);
			return S_OK;
		}
		if (GetSPDIF(eac3) && subtype == MEDIASUBTYPE_DOLBY_DDPLUS) {
			*pmt = CreateMediaTypeHDMI(IEC61937_EAC3);
			return S_OK;
		}
		if (GetSPDIF(truehd) && subtype == MEDIASUBTYPE_DOLBY_TRUEHD) {
			*pmt = CreateMediaTypeHDMI(IEC61937_TRUEHD);
			return S_OK;
		}
	}

	MPCSampleFormat out_mpcsf;
	DWORD out_samplerate;
	WORD  out_channels;
	DWORD out_layout = 0;

	AVCodecID codecID = m_FFAudioDec.GetCodecId();
	if (codecID != AV_CODEC_ID_NONE) {
		out_mpcsf      = SelectOutputFormat(SamplefmtToMPC[m_FFAudioDec.GetSampleFmt()]);
		out_samplerate = m_FFAudioDec.GetSampleRate();
		out_channels   = m_FFAudioDec.GetChannels();
		out_layout     = m_FFAudioDec.GetChannelMask();
	} else {
		if (wfe->wFormatTag == WAVE_FORMAT_PCM && wfe->wBitsPerSample > 16) {
			out_mpcsf = SF_PCM32;
		} else if (wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
			out_mpcsf = SF_FLOAT;
		} else {
			out_mpcsf = SF_PCM16;
		}
		out_mpcsf      = SelectOutputFormat(out_mpcsf);

		out_samplerate = wfe->nSamplesPerSec;
		out_channels   = wfe->nChannels;

		if (mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO && wfe->cbSize == 8) {
			const DVDALPCMFORMAT* fmt = (DVDALPCMFORMAT*)wfe;
			if (fmt->GroupAssignment > 20) {
				return E_FAIL;
			}
			out_layout = s_scmap_dvda[fmt->GroupAssignment].layout1 | s_scmap_dvda[fmt->GroupAssignment].layout2;
			out_channels = CountBits(out_layout);
		}
		else if (mt.subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
			WAVEFORMATEX_HDMV_LPCM* wfelpcm = (WAVEFORMATEX_HDMV_LPCM*)mt.Format();
			out_layout = s_scmap_hdmv[wfelpcm->channel_conf].layout;
		}

		if (!out_layout) {
			out_layout = GetDefChannelMask(out_channels);
		}
	}

	*pmt = CreateMediaType(out_mpcsf, out_samplerate, out_channels, out_layout);

	return S_OK;
}

HRESULT CMpaDecFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType *pmt)
{
	if (dir == PINDIR_INPUT) {
		if (m_FFAudioDec.GetCodecId() != AV_CODEC_ID_NONE) {
			m_FFAudioDec.StreamFinish();
		}

		enum AVCodecID nCodecId = FindCodec(pmt->subtype);
		if (nCodecId != AV_CODEC_ID_NONE) {
			if (m_FFAudioDec.Init(nCodecId, &m_pInput->CurrentMediaType())) {
				m_FFAudioDec.SetDRC(GetDynamicRangeControl());
			} else {
				return VFW_E_TYPE_NOT_ACCEPTED;
			}

			if (nCodecId == AV_CODEC_ID_DTS || nCodecId == AV_CODEC_ID_TRUEHD) {
				m_faJitter.SetNumSamples(200);
				m_JitterLimit = MAX_JITTER * 10;
			} else {
				m_faJitter.SetNumSamples(50);
				m_JitterLimit = MAX_JITTER;
			}
		} else {
			m_faJitter.SetNumSamples(50);
			m_JitterLimit = MAX_JITTER;
		}
	}

	return __super::SetMediaType(dir, pmt);
}

HRESULT CMpaDecFilter::BreakConnect(PIN_DIRECTION dir)
{
	DLog(L"CMpaDecFilter::BreakConnect()");

	if (dir == PINDIR_INPUT) {
		m_FFAudioDec.StreamFinish();
		m_bNeedCheck = TRUE;
	}

	return __super::BreakConnect(dir);
}

// IMpaDecFilter

#ifdef REGISTER_FILTER

STDMETHODIMP CMpaDecFilter::SetOutputFormat(MPCSampleFormat mpcsf, bool bEnable)
{
	CAutoLock cAutoLock(&m_csProps);
	if (mpcsf >= 0 && mpcsf < sfcount) {
		m_bSampleFmt[mpcsf] = bEnable;
	} else {
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetOutputFormat(MPCSampleFormat mpcsf)
{
	CAutoLock cAutoLock(&m_csProps);
	if (mpcsf >= 0 && mpcsf < sfcount) {
		return m_bSampleFmt[mpcsf];
	}
	return false;
}

#else

STDMETHODIMP CMpaDecFilter::SetOutputFormat(MPCSampleFormat mpcsf, bool bEnable)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetOutputFormat(MPCSampleFormat mpcsf)
{
	return true;
}

#endif

STDMETHODIMP CMpaDecFilter::SetAVSyncCorrection(bool bAVSync)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bAVSync = bAVSync;

	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetAVSyncCorrection()
{
	CAutoLock cAutoLock(&m_csProps);

	return m_bAVSync;
}

STDMETHODIMP CMpaDecFilter::SetDynamicRangeControl(bool bDRC)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bDRC = bDRC;

	m_FFAudioDec.SetDRC(bDRC);

	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetDynamicRangeControl()
{
	CAutoLock cAutoLock(&m_csProps);

	return m_bDRC;
}

STDMETHODIMP CMpaDecFilter::SetSPDIF(enctype et, bool bSPDIF)
{
	CAutoLock cAutoLock(&m_csProps);
	if (et < 0 || et >= etcount) {
		return E_INVALIDARG;
	}

	m_bSPDIF[et] = bSPDIF;
	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetSPDIF(enctype et)
{
	CAutoLock cAutoLock(&m_csProps);
	if (et < 0 || et >= etcount) {
		return false;
	}
	if (et == dtshd && !m_bSPDIF[dts]) {
		return false;
	}

	return m_bSPDIF[et];
}

STDMETHODIMP CMpaDecFilter::SaveSettings()
{
	CAutoLock cAutoLock(&m_csProps);

#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_MpaDec)) {
		key.SetDWORDValue(OPTION_SFormat_i16, m_bSampleFmt[SF_PCM16]);
		key.SetDWORDValue(OPTION_SFormat_i24, m_bSampleFmt[SF_PCM24]);
		key.SetDWORDValue(OPTION_SFormat_i32, m_bSampleFmt[SF_PCM32]);
		key.SetDWORDValue(OPTION_SFormat_flt, m_bSampleFmt[SF_FLOAT]);
		key.SetDWORDValue(OPTION_AVSYNC, m_bAVSync);
		key.SetDWORDValue(OPTION_DRC, m_bDRC);
		key.SetDWORDValue(OPTION_SPDIF_ac3, m_bSPDIF[ac3]);
		key.SetDWORDValue(OPTION_SPDIF_eac3, m_bSPDIF[eac3]);
		key.SetDWORDValue(OPTION_SPDIF_truehd, m_bSPDIF[truehd]);
		key.SetDWORDValue(OPTION_SPDIF_dts, m_bSPDIF[dts]);
		key.SetDWORDValue(OPTION_SPDIF_dtshd, m_bSPDIF[dtshd]);
		key.SetDWORDValue(OPTION_SPDIF_ac3enc, m_bSPDIF[ac3enc]);
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_AVSYNC, m_bAVSync);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_DRC, m_bDRC);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_ac3, m_bSPDIF[ac3]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_eac3, m_bSPDIF[eac3]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_truehd, m_bSPDIF[truehd]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_dts, m_bSPDIF[dts]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_dtshd, m_bSPDIF[dtshd]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_ac3enc, m_bSPDIF[ac3enc]);
#endif

	return S_OK;
}

static const char* SampleFormatToStringA(SampleFormat sf)
{
	switch (sf) {
		case SAMPLE_FMT_U8:
		case SAMPLE_FMT_U8P:
			return "8-bit";
		case SAMPLE_FMT_S16:
		case SAMPLE_FMT_S16P:
			return "16-bit";
		case SAMPLE_FMT_S24:
			return "24-bit";
		case SAMPLE_FMT_S32:
		case SAMPLE_FMT_S32P:
			return "32-bit";
		case SAMPLE_FMT_FLT:
		case SAMPLE_FMT_FLTP:
			return "32-bit float";
		case SAMPLE_FMT_DBL:
		case SAMPLE_FMT_DBLP:
			return "64-bit float";
	}

	return nullptr;
}

STDMETHODIMP_(CString) CMpaDecFilter::GetInformation(MPCAInfo index)
{
	CAutoLock cAutoLock(&m_csProps);
	CString infostr;

	if (index == AINFO_MPCVersion) {
		infostr.Format(L"v%d.%d.%d (build %d)", MPC_VERSION_MAJOR, MPC_VERSION_MINOR, MPC_VERSION_PATCH, MPC_VERSION_REV);

		return infostr;
	}

	if (index == AINFO_DecoderInfo) {
		// Input
		DWORD samplerate = 0;
		WORD channels = 0;
		DWORD layout = 0;

		const AVCodecID codecId = m_FFAudioDec.GetCodecId();

		if (codecId != AV_CODEC_ID_NONE) {
			LPCSTR codecName = m_FFAudioDec.GetCodecName();

			if (codecId == AV_CODEC_ID_DTS && m_DTSHDProfile != 0) {
				switch (m_DTSHDProfile) {
					case DCA_PROFILE_HD_HRA :
						codecName = "dts-hd hra";
						break;
					case DCA_PROFILE_HD_MA :
						codecName = "dts-hd ma";
						break;
					case DCA_PROFILE_EXPRESS :
						codecName = "dts express";
						break;
				}
			}

			infostr.Format(L"Codec: %hS", codecName);
			const WORD bitdeph = m_FFAudioDec.GetCoddedBitdepth();
			if (bitdeph) {
				infostr.AppendFormat(L", %u-bit", bitdeph);
			}
			infostr += L"\r\n";
			samplerate = m_FFAudioDec.GetSampleRate();
			channels = m_FFAudioDec.GetChannels();
			layout = m_FFAudioDec.GetChannelMask();
		}
		else if (m_pInput->CurrentMediaType().IsValid()) {
			const GUID& subtype = m_pInput->CurrentMediaType().subtype;
			infostr = L"Input: ";
			bool enable_bitdeph = true;
			if (subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
				infostr += L"DVD LPCM";
			}
			else if (subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
				infostr += L"HDMV LPCM";
			}
			else if (subtype == MEDIASUBTYPE_PS2_PCM) {
				infostr += L"PS2 PCM";
			}
			else if (subtype == MEDIASUBTYPE_PS2_ADPCM) {
				infostr += L"PS2 ADPCM";
			}
			else if (subtype == MEDIASUBTYPE_PCM_NONE
					|| subtype == MEDIASUBTYPE_PCM_RAW
					|| subtype == MEDIASUBTYPE_PCM_TWOS
					|| subtype == MEDIASUBTYPE_PCM_SOWT
					|| subtype == MEDIASUBTYPE_PCM_IN24
					|| subtype == MEDIASUBTYPE_PCM_IN32
					|| subtype == MEDIASUBTYPE_PCM_FL32
					|| subtype == MEDIASUBTYPE_PCM_FL64
					|| subtype == MEDIASUBTYPE_IEEE_FLOAT) {
				infostr += L"PCM";
			}
			else {
				// bitstream ac3, eac3, dts, truehd, mlp
				enable_bitdeph = false;
				infostr += GetCodecDescriptorName(FindCodec(subtype));
			}

			if (WAVEFORMATEX* wfein = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format()) {
				if (enable_bitdeph) {
					infostr.AppendFormat(L", %u-bit", wfein->wBitsPerSample);
					if (subtype == MEDIASUBTYPE_PCM_FL32
						|| subtype == MEDIASUBTYPE_PCM_FL64
						|| subtype == MEDIASUBTYPE_IEEE_FLOAT) {
						infostr += L" float";
					}
				}
				samplerate = wfein->nSamplesPerSec;
				channels = wfein->nChannels;
			}
			infostr += L"\r\n";
		}
		else {
			return ResStr(IDS_MPADEC_NOTACTIVE);
		}

		if (samplerate) {
			infostr.AppendFormat(L"Sample rate: %u Hz\r\n", samplerate);
		}
		if (channels) {
			if (channels > 2 && layout) {
				const char* spks[] = { "L", "R", "C", "LFE", "BL", "BR", "FLC", "FRC", "BC", "SL", "SR", "TC" };
				CStringA str;
				for (int i = 0; i < _countof(spks); i++) {
					if (layout & (1 << i)) {
						if (str.GetLength()) {
							str.Append(",");
						}
						str.Append(spks[i]);
					}
				}
				BYTE lfe = 0;
				if (layout & SPEAKER_LOW_FREQUENCY) {
					channels--;
					lfe = 1;
				}
				infostr.AppendFormat(L"Channels: %u.%u  [%hS]\r\n", channels, lfe, str);
			}
			else {
				infostr.AppendFormat(L"Channels: %u\r\n", channels);
			}
		}

		// Output
		infostr += L"Output:";
		if (m_InternalSampleFormat != SAMPLE_FMT_NONE) {
			infostr.AppendFormat(L" %hS", SampleFormatToStringA(m_InternalSampleFormat));
		}

		if (WAVEFORMATEX* wfeout = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format()) {
			const GUID& subtype = m_pOutput->CurrentMediaType().subtype;
			bool outPCM = false;

			if (subtype == MEDIASUBTYPE_PCM) {
				switch (wfeout->wFormatTag) {
				case WAVE_FORMAT_DOLBY_AC3_SPDIF:
					{
						enum AVCodecID nCodecId = m_FFAudioDec.GetCodecId();
						CString codec = L"AC3";
						if (nCodecId == AV_CODEC_ID_DTS) {
							codec = L"DTS";
						}
						infostr.AppendFormat(L" -> %s SPDIF, %u Hz", codec, wfeout->nSamplesPerSec);
						// wfeout channels and bit depth is not actual
					}
					break;
				case WAVE_FORMAT_EXTENSIBLE: {
					WAVEFORMATEXTENSIBLE* wfexout = (WAVEFORMATEXTENSIBLE*)wfeout;

					if (wfexout->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
						outPCM = true;
					}
					else if (wfexout->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DTS_HD) {
						infostr += L" -> HDMI DTS-HD";
						// wfeout samplerate, channels and bit depth is not actual
					}
					else if (wfexout->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS) {
						infostr += L" -> HDMI E-AC-3";
						// wfeout samplerate, channels and bit depth is not actual
					}
					else if (wfexout->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
						infostr += L" -> HDMI TrueHD";
						// wfeout samplerate, channels and bit depth is not actual
					}
					break;
				}
				case WAVE_FORMAT_PCM:
					outPCM = true;
					break;
				}
			}
			else if (subtype == MEDIASUBTYPE_IEEE_FLOAT) {
				outPCM = true;
			}

			if (outPCM) {
				const char* outsf = SampleFormatToStringA(GetSampleFormat(wfeout));
				if (outsf != SampleFormatToStringA(m_InternalSampleFormat)) {
					infostr.AppendFormat(L" -> %hS", outsf);
				}
			}

		}

		return infostr;
	}

	return nullptr;
}

// ISpecifyPropertyPages2

STDMETHODIMP CMpaDecFilter::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	HRESULT hr = S_OK;

	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	CheckPointer(pPages->pElems, E_OUTOFMEMORY);
	pPages->pElems[0] = __uuidof(CMpaDecSettingsWnd);

	return hr;
}

STDMETHODIMP CMpaDecFilter::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != nullptr) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMpaDecSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpaDecSettingsWnd>(nullptr, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// IExFilterConfig

STDMETHODIMP CMpaDecFilter::SetBool(LPCSTR field, bool value)
{
	if (strcmp(field, "stereodownmix") == 0) {
		m_FFAudioDec.SetStereoDownmix(value);
		return S_OK;
	}

	return E_INVALIDARG;
}
