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
	#include "../../filters/ffmpeg_fix.cpp"
#endif

#include <moreuuids.h>
#include <basestruct.h>
#include <vector>

#include <ffmpeg/libavcodec/avcodec.h>

// option names
#define OPT_REGKEY_MpaDec   _T("Software\\MPC-BE Filters\\MPC Audio Decoder")
#define OPT_SECTION_MpaDec  _T("Filters\\MPC Audio Decoder")
#define OPTION_SFormat_i16  _T("SampleFormat_int16")
#define OPTION_SFormat_i24  _T("SampleFormat_int24")
#define OPTION_SFormat_i32  _T("SampleFormat_int32")
#define OPTION_SFormat_flt  _T("SampleFormat_float")
#define OPTION_DRC          _T("DRC")
#define OPTION_SPDIF_ac3    _T("SPDIF_ac3")
#define OPTION_SPDIF_eac3   _T("HDMI_eac3")
#define OPTION_SPDIF_truehd _T("HDMI_truehd")
#define OPTION_SPDIF_dts    _T("SPDIF_dts")
#define OPTION_SPDIF_dtshd  _T("HDMI_dtshd")
#if ENABLE_AC3_ENCODER
#define OPTION_SPDIF_ac3enc _T("SPDIF_ac3enc")
#endif

#define MAX_JITTER          1000000i64 // +-100ms jitter is allowed
#define MAX_DTS_JITTER      1400000i64 // +-140ms jitter is allowed for DTS

#define PADDING_SIZE        FF_INPUT_BUFFER_PADDING_SIZE

#define BS_HEADER_SIZE          8
#define BS_AC3_SIZE          6144
#define BS_EAC3_SIZE        24576 // 6144 for DD Plus * 4 for IEC 60958 frames
#define BS_MAT_SIZE         61424 // max length of MAT data
#define BS_MAT_OFFSET        2560
#define BS_TRUEHD_SIZE      61440 // 8 header bytes + 61424 of MAT data + 8 zero byte
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
	// QDM2
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
};

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_IEEE_FLOAT},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMpaDecFilter), MPCAudioDecName, MERIT_NORMAL + 1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, &__uuidof(CMpaDecFilter), CreateInstance<CMpaDecFilter>, NULL, &sudFilter[0]},
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

#pragma warning(push)
#pragma warning(disable : 4245)
static struct scmap_t {
	const WORD nChannels;
	const BYTE ch[8];
	const DWORD dwChannelMask;
}
// dshow: left, right, center, LFE, left surround, right surround
// lets see how we can map these things to dshow (oh the joy!)

s_scmap_hdmv[] = {
	//   FL FR FC LFe BL BR FLC FRC
	{0, {-1,-1,-1,-1,-1,-1,-1,-1 }, 0}, // INVALID
	{1, { 0,-1,-1,-1,-1,-1,-1,-1 }, SPEAKER_FRONT_CENTER}, // Mono    M1, 0
	{0, {-1,-1,-1,-1,-1,-1,-1,-1 }, 0}, // INVALID
	{2, { 0, 1,-1,-1,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT}, // Stereo  FL, FR
	{4, { 0, 1, 2,-1,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER},															// 3/0      FL, FR, FC
	{4, { 0, 1, 2,-1,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER},															// 2/1      FL, FR, Surround
	{4, { 0, 1, 2, 3,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER},										// 3/1      FL, FR, FC, Surround
	{4, { 0, 1, 2, 3,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT},											// 2/2      FL, FR, BL, BR
	{6, { 0, 1, 2, 3, 4,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT},						// 3/2      FL, FR, FC, BL, BR
	{6, { 0, 1, 2, 5, 3, 4,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT},// 3/2+LFe  FL, FR, FC, BL, BR, LFe
	{8, { 0, 1, 2, 3, 6, 4, 5,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},// 3/4  FL, FR, FC, BL, Bls, Brs, BR
	{8, { 0, 1, 2, 7, 4, 5, 3, 6 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},// 3/4+LFe  FL, FR, FC, BL, Bls, Brs, BR, LFe
};
#pragma warning(pop)

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
	SF_PCM24, // <-- SAMPLE_FMT_S24
//  SF_PCM24  // <-- SAMPLE_FMT_S24P
};

static const SampleFormat MPCtoSamplefmt[sfcount] = {
	SAMPLE_FMT_S16, // <-- SF_PCM16
	SAMPLE_FMT_S24, // <-- SF_PCM24
	SAMPLE_FMT_S32, // <-- SF_PCM32
	SAMPLE_FMT_FLT  // <-- SF_FLOAT
};

//void DD_stats_t::Reset()
//{
//	mode = AV_CODEC_ID_NONE;
//	ac3_frames  = 0;
//	eac3_frames = 0;
//}
//
//bool DD_stats_t::Desired(int type)
//{
//	if (mode != AV_CODEC_ID_NONE) {
//		return (mode == type);
//	};
//	// unknown mode
//	if (type == AV_CODEC_ID_AC3) {
//		++ac3_frames;
//	} else if (type == AV_CODEC_ID_EAC3) {
//		++eac3_frames;
//	}
//
//	if (ac3_frames + eac3_frames >= 4) {
//		if (eac3_frames > 2 * ac3_frames) {
//			mode = AV_CODEC_ID_EAC3; // EAC3
//		} else {
//			mode = AV_CODEC_ID_AC3;  // AC3 or mixed AC3+EAC3
//		}
//		return (mode == type);
//	}
//
//	return true;
//}

CMpaDecFilter::CMpaDecFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CTransformFilter(NAME("CMpaDecFilter"), lpunk, __uuidof(this))
	, m_InternalSampleFormat(SAMPLE_FMT_NONE)
	, m_rtStart(0)
	, m_dStartOffset(0.0)
	, m_bDiscontinuity(FALSE)
	, m_bResync(false)
	, m_buff(PADDING_SIZE)
	, m_hdmicount(0)
	, m_hdmisize(0)
	, m_truehd_samplerate(0)
	, m_truehd_framelength(0)
	, m_bHasVideo(TRUE)
	, m_bDoAdditionalCheck(FALSE)
{
	if (phr) {
		*phr = S_OK;
	}

	m_pInput = DNew CDeCSSInputPin(NAME("CDeCSSInputPin"), this, phr, L"In");
	if (!m_pInput) {
		*phr = E_OUTOFMEMORY;
	}
	if (FAILED(*phr)) {
		return;
	}

	m_pOutput = DNew CTransformOutputPin(NAME("CTransformOutputPin"), this, phr, L"Out");
	if (!m_pOutput) {
		*phr = E_OUTOFMEMORY;
	}
	if (FAILED(*phr)) {
		delete m_pInput, m_pInput = NULL;
		return;
	}

	memset(&m_bBitstreamSupported, 0, sizeof(m_bBitstreamSupported));

//	m_DDstats.Reset();

	// default settings
	m_fSampleFmt[SF_PCM16] = true;
	m_fSampleFmt[SF_PCM24] = false;
	m_fSampleFmt[SF_PCM32] = false;
	m_fSampleFmt[SF_FLOAT] = false;
	m_fDRC                 = false;
	m_fSPDIF[ac3]          = false;
	m_fSPDIF[eac3]         = false;
	m_fSPDIF[truehd]       = false;
	m_fSPDIF[dts]          = false;
	m_fSPDIF[dtshd]        = false;
#if ENABLE_AC3_ENCODER
	m_fSPDIF[ac3enc]       = false;
#endif

	// read settings
	CString layout_str;
#ifdef REGISTER_FILTER
	CRegKey key;
	ULONG len = 8;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_MpaDec, KEY_READ)) {
		DWORD dw;
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SFormat_i16, dw)) {
			m_fSampleFmt[SF_PCM16] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SFormat_i24, dw)) {
			m_fSampleFmt[SF_PCM24] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SFormat_i32, dw)) {
			m_fSampleFmt[SF_PCM32] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SFormat_flt, dw)) {
			m_fSampleFmt[SF_FLOAT] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_DRC, dw)) {
			m_fDRC = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_ac3, dw)) {
			m_fSPDIF[ac3] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_eac3, dw)) {
			m_fSPDIF[eac3] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_truehd, dw)) {
			m_fSPDIF[truehd] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_dts, dw)) {
			m_fSPDIF[dts] = !!dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_dtshd, dw)) {
			m_fSPDIF[dtshd] = !!dw;
		}
#if ENABLE_AC3_ENCODER
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPTION_SPDIF_ac3enc, dw)) {
			m_fSPDIF[ac3enc] = !!dw;
		}
#endif
	}
#else
	m_fSampleFmt[SF_PCM16] = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SFormat_i16, m_fSampleFmt[SF_PCM16]);
	m_fSampleFmt[SF_PCM24] = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SFormat_i24, m_fSampleFmt[SF_PCM24]);
	m_fSampleFmt[SF_PCM32] = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SFormat_i32, m_fSampleFmt[SF_PCM32]);
	m_fSampleFmt[SF_FLOAT] = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SFormat_flt, m_fSampleFmt[SF_FLOAT]);
	m_fDRC                 = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_DRC, m_fDRC);
	m_fSPDIF[ac3]          = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_ac3, m_fSPDIF[ac3]);
	m_fSPDIF[eac3]         = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_eac3, m_fSPDIF[eac3]);
	m_fSPDIF[truehd]       = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_truehd, m_fSPDIF[truehd]);
	m_fSPDIF[dts]          = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_dts, m_fSPDIF[dts]);
	m_fSPDIF[dtshd]        = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_dtshd, m_fSPDIF[dtshd]);
#if ENABLE_AC3_ENCODER
	m_fSPDIF[ac3enc]       = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_ac3enc, m_fSPDIF[ac3enc]);
#endif
#endif
	if (!(m_fSampleFmt[SF_PCM16] || m_fSampleFmt[SF_PCM24] || m_fSampleFmt[SF_PCM32] || m_fSampleFmt[SF_FLOAT])) {
		m_fSampleFmt[SF_PCM16] = true;
	}
}

CMpaDecFilter::~CMpaDecFilter()
{
	StopStreaming();
}

STDMETHODIMP CMpaDecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMpaDecFilter)
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
	return __super::BeginFlush();
}

HRESULT CMpaDecFilter::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_buff.RemoveAll();
	m_FFAudioDec.FlushBuffers();
#if ENABLE_AC3_ENCODER
	m_Mixer.FlushBuffers();
	m_encbuff.RemoveAll();
#endif
	return __super::EndFlush();
}

HRESULT CMpaDecFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	m_ps2_state.sync = false;
	m_hdmicount = 0;
	m_hdmisize  = 0;
	m_truehd_samplerate  = 0;
	m_truehd_framelength = 0;
	m_bResync = true;
	m_rtStart = 0; // LOOKATTHIS // reset internal timer?

	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CMpaDecFilter::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

	AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA) {
		return m_pOutput->Deliver(pIn);
	}

	AM_MEDIA_TYPE* pmt;
	if (SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt) {
		CMediaType mt(*pmt);
		m_pInput->SetMediaType(&mt);
		DeleteMediaType(pmt);
		pmt = NULL;
	}

	BYTE* pDataIn = NULL;
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
		DbgLog((LOG_TRACE, 3, L"CMpaDecFilter::Receive(): [%10I64d => %10I64d], %d", rtStart, rtStop, rtStop - rtStart));
	} else {
		DbgLog((LOG_TRACE, 3, L"CMpaDecFilter::Receive(): frame without timestamp"));
	}
#endif

	if (pIn->IsDiscontinuity() == S_OK
			|| (m_FFAudioDec.GetNeedSyncPoint() && S_OK == pIn->IsSyncPoint())) {
		m_bDiscontinuity = TRUE;
		m_buff.RemoveAll();
#if ENABLE_AC3_ENCODER
		m_encbuff.RemoveAll();
#endif
		m_bResync = true;
		if (FAILED(hr)) {
			DbgLog((LOG_TRACE, 3, L"CMpaDecFilter::Receive() : Discontinuity without timestamp"));
			// LOOKATTHIS // return S_OK;
		}
	}

	const GUID& subtype = m_pInput->CurrentMediaType().subtype;
	enum AVCodecID nCodecId = FindCodec(subtype);

	REFERENCE_TIME jitterLimit = MAX_JITTER;
	if (!m_bHasVideo || m_FFAudioDec.GetIgnoreJitterChecking()) {
		jitterLimit = INT64_MAX;
	} else if (nCodecId == AV_CODEC_ID_DTS) {
		jitterLimit = MAX_DTS_JITTER;
	}

	if (rtStart != INVALID_TIME && abs(m_rtStart - rtStart) > jitterLimit) {
		DbgLog((LOG_TRACE, 3, L"CMpaDecFilter::Receive() : jitter limit is exceeded - %I64d", abs(m_rtStart - rtStart)));
		m_bResync = true;
	}

	if (m_bResync && SUCCEEDED(hr)) {
		DbgLog((LOG_TRACE, 3, L"CMpaDecFilter::Receive() : Resync Request - [%I64d -> %I64d], buffer : %u", m_rtStart, rtStart, m_buff.GetCount()));

		m_buff.RemoveAll();
#if ENABLE_AC3_ENCODER
		m_encbuff.RemoveAll();
#endif
		if (m_rtStart != INVALID_TIME && rtStart != m_rtStart) {
			m_bDiscontinuity = TRUE;
		}
		
		m_rtStart = rtStart;
		m_dStartOffset = 0.0;
		m_bResync = false;
	}

	size_t bufflen = m_buff.GetCount();
	m_buff.SetCount(bufflen + len, 4096);
	memcpy(m_buff.GetData() + bufflen, pDataIn, len);
	len += (long)bufflen;

//	if (subtype == MEDIASUBTYPE_DOLBY_AC3 || subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3 || subtype == MEDIASUBTYPE_DNET) {
//		return ProcessAC3();
//	}

	if (nCodecId != AV_CODEC_ID_NONE) {
		if (ProcessBitstream(nCodecId, hr)) {
			return hr;
		}
		return ProcessFFmpeg(nCodecId);
	}

	if (subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
		hr = ProcessLPCM();
	} else if (subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
		// TODO: check if the test is really correct
		hr = ProcessHdmvLPCM(pIn->IsSyncPoint() == S_FALSE);
	} else if (subtype == MEDIASUBTYPE_PS2_PCM) {
		hr = ProcessPS2PCM();
	} else if (subtype == MEDIASUBTYPE_PS2_ADPCM) {
		hr = ProcessPS2ADPCM();
	} else if (subtype == MEDIASUBTYPE_PCM_NONE ||
			   subtype == MEDIASUBTYPE_PCM_RAW) {
		hr = ProcessPCMraw();
	} else if (subtype == MEDIASUBTYPE_PCM_TWOS ||
			   subtype == MEDIASUBTYPE_PCM_IN24 ||
			   subtype == MEDIASUBTYPE_PCM_IN32) {
		hr = ProcessPCMintBE();
	} else if (subtype == MEDIASUBTYPE_PCM_SOWT) {
		hr = ProcessPCMintLE();
	} else if (subtype == MEDIASUBTYPE_PCM_FL32 ||
			   subtype == MEDIASUBTYPE_PCM_FL64) {
		hr = ProcessPCMfloatBE();
	} else if (subtype == MEDIASUBTYPE_IEEE_FLOAT) {
		hr = ProcessPCMfloatLE();
	}

	return hr;
}

BOOL CMpaDecFilter::ProcessBitstream(enum AVCodecID nCodecId, HRESULT& hr, BOOL bEOF/* = FALSE*/)
{
	if (m_bBitstreamSupported[SPDIF]) {
		if (GetSPDIF(ac3) && nCodecId == AV_CODEC_ID_AC3) {
			hr = bEOF ? S_OK : ProcessAC3_SPDIF();
			return TRUE;
		}
		if (GetSPDIF(dts) && nCodecId == AV_CODEC_ID_DTS) {
			hr = ProcessDTS_SPDIF(bEOF);
			return TRUE;
		}
	}

	if (m_bBitstreamSupported[EAC3] && GetSPDIF(eac3) && nCodecId == AV_CODEC_ID_EAC3) {
		hr = bEOF ? S_OK : ProcessEAC3_SPDIF();
		return TRUE;
	}

	if (m_bBitstreamSupported[TRUEHD] && GetSPDIF(truehd) && nCodecId == AV_CODEC_ID_TRUEHD) {
		hr = bEOF ? S_OK : ProcessTrueHD_SPDIF();
		return TRUE;
	}

	return FALSE;
}

#define BEGINDATA									\
		BYTE* const base = m_buff.GetData();		\
		BYTE* end = base + m_buff.GetCount();		\
		BYTE* p = base;								\

#define ENDDATA										\
		if (p == end) {								\
			m_buff.RemoveAll();						\
		} else if (p > base) {						\
			size_t remaining = (size_t)(end - p);	\
			memmove(base, p, remaining);			\
			m_buff.SetCount(remaining);				\
		}

HRESULT CMpaDecFilter::ProcessLPCM()
{
	WAVEFORMATEX* wfein = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();

	WORD nChannels = wfein->nChannels;
	if (nChannels < 1 || nChannels > 8) {
		return ERROR_NOT_SUPPORTED;
	}

	BEGINDATA

	unsigned int blocksize = nChannels * 2 * wfein->wBitsPerSample / 8;
	size_t nSamples = (m_buff.GetCount() / blocksize) * 2 * nChannels;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	size_t outSize = nSamples * (wfein->wBitsPerSample <= 16 ? 2 : 4); // convert to 16 and 32-bit
	CAtlArray<BYTE> outBuff;
	outBuff.SetCount(outSize);

	switch (wfein->wBitsPerSample) {
		case 16: {
			out_sf = SAMPLE_FMT_S16;
			uint16_t* pDataOut = (uint16_t*)outBuff.GetData();

			for (size_t i = 0; i < nSamples; i++) {
				pDataOut[i] = (uint16_t)(*p) << 8 | (uint16_t)(*(p + 1));
				p += 2;
			}
		}
		break;
		case 24 : {
			out_sf = SAMPLE_FMT_S32;
			uint32_t* pDataOut = (uint32_t*)outBuff.GetData();

			size_t m = nChannels * 2;
			for (size_t k = 0, n = nSamples / m; k < n; k++) {
				BYTE* q = p + m * 2;
				for (size_t i = 0; i < m; i++) {
					pDataOut[i] = (uint32_t)(*p) << 24 | (uint32_t)(*(p + 1)) << 16 | (uint32_t)(*q) << 8;
					p += 2;
					q++;
				}
				p += m;
				pDataOut += m;
			}
		}
		break;
		case 20 : {
			out_sf = SAMPLE_FMT_S32;
			uint32_t* pDataOut = (uint32_t*)outBuff.GetData();

			size_t m = nChannels * 2;
			for (size_t k = 0, n = nSamples / m; k < n; k++) {
				BYTE* q = p + m * 2;
				for (size_t i = 0; i < m; i++) {
					uint32_t u32 = (uint32_t)(*p) << 24 | (uint32_t)(*(p+1)) << 16;
					if (i & 1) {
						u32 |= (*(uint8_t*)q & 0x0F) << 12;
						q++;
					} else {
						u32 |= (*(uint8_t*)q & 0xF0) << 8;
					}
					pDataOut[i] = u32;
					p += 2;
				}
				p += nChannels;
				pDataOut += m;
			}
		}
		break;
	}

	ENDDATA

	return Deliver(outBuff.GetData(), outSize, out_sf, wfein->nSamplesPerSec, wfein->nChannels);
}

HRESULT CMpaDecFilter::ProcessHdmvLPCM(bool bAlignOldBuffer) // Blu ray LPCM
{
	WAVEFORMATEX_HDMV_LPCM* wfein = (WAVEFORMATEX_HDMV_LPCM*)m_pInput->CurrentMediaType().Format();

	scmap_t* remap     = &s_scmap_hdmv [wfein->channel_conf];
	int nChannels      = wfein->nChannels;
	int xChannels      = nChannels + (nChannels & 1);
	int BytesPerSample = (wfein->wBitsPerSample + 7) / 8;
	int BytesPerFrame  = BytesPerSample * xChannels;

	BYTE* pDataIn      = m_buff.GetData();
	int len            = (int)m_buff.GetCount();
	len -= len % BytesPerFrame;
	if (bAlignOldBuffer) {
		m_buff.SetCount(len);
	}
	int nFrames = len / BytesPerFrame;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	int outSize = nFrames * nChannels * (wfein->wBitsPerSample <= 16 ? 2 : 4); // convert to 16 and 32-bit
	CAtlArray<BYTE> outBuff;
	outBuff.SetCount(outSize);

	switch (wfein->wBitsPerSample) {
		case 16: {
			out_sf = SAMPLE_FMT_S16;
			int16_t* pDataOut = (int16_t*)outBuff.GetData();

			for (int i=0; i<nFrames; i++) {
				for (int j = 0; j < nChannels; j++) {
					BYTE nRemap = remap->ch[j];
					*pDataOut = (int16_t)(pDataIn[nRemap * 2] << 8 | pDataIn[nRemap * 2 + 1]);
					pDataOut++;
				}
				pDataIn += BytesPerFrame;
			}
		}
		break;
		case 24 :
		case 20: {
			out_sf = SAMPLE_FMT_S32; // convert to 32-bit
			int32_t* pDataOut = (int32_t*)outBuff.GetData();

			for (int i=0; i<nFrames; i++) {
				for (int j = 0; j < nChannels; j++) {
					BYTE nRemap = remap->ch[j];
					*pDataOut = (int32_t)(pDataIn[nRemap * 3] << 24 | pDataIn[nRemap * 3 + 1] << 16 | pDataIn[nRemap * 3 + 2] << 8);
					pDataOut++;
				}
				pDataIn += BytesPerFrame;
			}
		}
		break;
	}
	memmove(m_buff.GetData(), pDataIn, m_buff.GetCount() - len );
	m_buff.SetCount(m_buff.GetCount() - len);

	return Deliver(outBuff.GetData(), outSize, out_sf, wfein->nSamplesPerSec, wfein->nChannels, remap->dwChannelMask);
}

HRESULT CMpaDecFilter::ProcessFFmpeg(enum AVCodecID nCodecId, BOOL bEOF/* = FALSE*/)
{
	HRESULT hr;
	enum AVCodecID ffCodecId = m_FFAudioDec.GetCodecId();

	if (bEOF) {
		for (;;) {
			int size = 0;
			CAtlArray<BYTE> output;
			SampleFormat samplefmt = SAMPLE_FMT_NONE;

			hr = m_FFAudioDec.Decode(ffCodecId, NULL, 0, size, output, samplefmt);
			if (hr == S_OK && output.GetCount() > 0) {
				hr = Deliver(output.GetData(), output.GetCount(), samplefmt, m_FFAudioDec.GetSampleRate(), m_FFAudioDec.GetChannels(), m_FFAudioDec.GetChannelMask());
			} else {
				break;
			}
		}
		return S_OK;
	}


	BEGINDATA

	if (ffCodecId == AV_CODEC_ID_NONE) {
		m_FFAudioDec.Init(nCodecId, m_pInput);
		m_FFAudioDec.SetDRC(GetDynamicRangeControl());
	}

	// RealAudio
	CPaddedArray buffRA(FF_INPUT_BUFFER_PADDING_SIZE);
	bool isRA = false;
	if (nCodecId == AV_CODEC_ID_ATRAC3 || nCodecId == AV_CODEC_ID_COOK || nCodecId == AV_CODEC_ID_SIPR) {
		HRESULT hrRA = m_FFAudioDec.RealPrepare(p, int(end - p), buffRA);
		if (hrRA == S_OK) {
			p = buffRA.GetData();
			end = p + buffRA.GetCount();
			isRA = true;
		} else if (hrRA == E_FAIL) {
			return S_OK;
		} else {
			// trying continue decoding without any pre-processing ...
		}
	}

	while (p < end) {
		int size = 0;
		CAtlArray<BYTE> output;
		SampleFormat samplefmt = SAMPLE_FMT_NONE;

		hr = m_FFAudioDec.Decode(nCodecId, p, int(end - p), size, output, samplefmt);
		if (FAILED(hr)) {
			m_buff.RemoveAll();
			m_bResync = true;
			return S_OK;
		} else if (hr == S_FALSE) {
			m_bResync = true;
			p += size;
			continue;
		} else if (output.GetCount() > 0) { // && SUCCEEDED(hr)
			hr = Deliver(output.GetData(), output.GetCount(), samplefmt, m_FFAudioDec.GetSampleRate(), m_FFAudioDec.GetChannels(), m_FFAudioDec.GetChannelMask());
		} else if (size == 0) { // && pBuffOut.GetCount() == 0
			break;
		}

		p += size;
	}

	if (isRA) { // RealAudio
		p = base + buffRA.GetCount();
		end = base + m_buff.GetCount();
	}

	ENDDATA

	return S_OK;
}

//HRESULT CMpaDecFilter::ProcessAC3()
//{
//	HRESULT hr;
//
//	BEGINDATA
//
//	while (p + 8 <= end) {
//		if (*(WORD*)p != 0x770b) {
//			p++;
//			continue;
//		}
//
//		AVCodecID ftype;
//		int size;
//		if ((size = ParseAC3Header(p)) > 0) {
//			ftype = AV_CODEC_ID_AC3;
//		} else if ((size = ParseEAC3Header(p)) > 0) {
//			ftype = AV_CODEC_ID_EAC3;
//		} else {
//			p += 2;
//			continue;
//		}
//
//		if (p + size > end) {
//			break;
//		}
//
//		if (m_DDstats.Desired(ftype)) {
//			if (m_FFAudioDec.GetCodecId() != ftype) {
//				m_FFAudioDec.Init(ftype, m_pInput);
//				m_FFAudioDec.SetDRC(GetDynamicRangeControl());
//			}
//
//			CAtlArray<BYTE> output;
//			SampleFormat samplefmt = SAMPLE_FMT_NONE;
//
//			hr = m_FFAudioDec.Decode(ftype, p, size, size, output, samplefmt);
//			if (FAILED(hr)) {
//				m_buff.RemoveAll();
//				m_bResync = true;
//				return S_OK;
//			} else if (hr == S_FALSE) {
//				m_bResync = true;
//				p += size;
//				continue;
//			} else if (!output.IsEmpty()) { // && SUCCEEDED(hr)
//				hr = Deliver(output.GetData(), output.GetCount(), samplefmt, m_FFAudioDec.GetSampleRate(), m_FFAudioDec.GetChannels(), m_FFAudioDec.GetChannelMask());
//			} else if (size == 0) { // && pBuffOut.IsEmpty()
//				break;
//			}
//		}
//		p += size;
//	}
//
//	ENDDATA
//
//	return S_OK;
//}

HRESULT CMpaDecFilter::ProcessAC3_SPDIF()
{
	HRESULT hr;

	BEGINDATA

	while (p + 8 <= end) { // 8 =  AC3 header size + 1
		audioframe_t aframe;
		int size = ParseAC3Header(p, &aframe);

		if (size == 0) {
			p++;
			continue;
		}
		if (p + size > end) {
			break;
		}

		if (FAILED(hr = DeliverBitstream(p, size, IEC61937_AC3, aframe.samplerate, 1536))) {
			return hr;
		}

		p += size;
	}

	ENDDATA

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessEAC3_SPDIF()
{
	HRESULT hr;

	BEGINDATA

	while (p + 8 <= end) {
		audioframe_t aframe;
		int size = ParseEAC3Header(p, &aframe);

		if (size == 0) {
			p++;
			continue;
		}
		if (p + size > end) {
			break;
		}

		static const uint8_t eac3_repeat[4] = {6, 3, 2, 1};
		int repeat = 1;
		if ((p[4] & 0xc0) != 0xc0) { /* fscod */
			repeat = eac3_repeat[(p[4] & 0x30) >> 4]; /* numblkscod */
		}
		m_hdmicount++;
		if (m_hdmisize + size <= BS_EAC3_SIZE - BS_HEADER_SIZE) {
			memcpy(m_hdmibuff + m_hdmisize, p, size);
			m_hdmisize += size;
		} else {
			ASSERT(0);
		}
		p += size;

		if (m_hdmicount < repeat) {
			break;
		}

		hr = DeliverBitstream(m_hdmibuff, m_hdmisize, IEC61937_EAC3, aframe.samplerate, aframe.samples * repeat);
		m_hdmicount = 0;
		m_hdmisize  = 0;
		if (FAILED(hr)) {
			return hr;
		}
	}

	ENDDATA

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessTrueHD_SPDIF()
{
	const BYTE mat_start_code[20]  = { 0x07, 0x9E, 0x00, 0x03, 0x84, 0x01, 0x01, 0x01, 0x80, 0x00, 0x56, 0xA5, 0x3B, 0xF4, 0x81, 0x83, 0x49, 0x80, 0x77, 0xE0 };
	const BYTE mat_middle_code[12] = { 0xC3, 0xC1, 0x42, 0x49, 0x3B, 0xFA, 0x82, 0x83, 0x49, 0x80, 0x77, 0xE0 };
	const BYTE mat_end_code[16]    = { 0xC3, 0xC2, 0xC0, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x11 };

	HRESULT hr;

	BEGINDATA

	while (p + 16 <= end) {
		audioframe_t aframe;
		int size = ParseMLPHeader(p, &aframe);
		if (size > 0) {
			// sync frame
			m_truehd_samplerate  = aframe.samplerate;
			m_truehd_framelength = aframe.samples;
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

		if (size == 0 && m_truehd_framelength > 0) {
			// get not sync frame size
			size = ((p[0] << 8 | p[1]) & 0xfff) * 2;
		}

		if (size < 8) {
			p++;
			continue;
		}
		if (p + size > end) {
			break;
		}

		m_hdmicount++;
		if (m_hdmicount == 1) {
			// skip 8 header bytes and write MAT start code
			memcpy(m_hdmibuff + BS_HEADER_SIZE, mat_start_code, sizeof(mat_start_code));
			m_hdmisize = BS_HEADER_SIZE + sizeof(mat_start_code);
		} else if (m_hdmicount == 13) {
			memcpy(m_hdmibuff + (BS_HEADER_SIZE + BS_MAT_SIZE) / 2, mat_middle_code, sizeof(mat_middle_code));
			m_hdmisize = (BS_HEADER_SIZE + BS_MAT_SIZE) / 2 + sizeof(mat_middle_code);
		}

		if (m_hdmisize + size <= m_hdmicount * BS_MAT_OFFSET) {
			memcpy(m_hdmibuff + m_hdmisize, p, size);
			m_hdmisize += size;
			memset(m_hdmibuff + m_hdmisize, 0, m_hdmicount * BS_MAT_OFFSET - m_hdmisize);
			m_hdmisize = m_hdmicount * BS_MAT_OFFSET;
		} else {
			ASSERT(0);
		}
		p += size;

		if (m_hdmicount < 24) {
			break;
		}

		memcpy(m_hdmibuff + (BS_HEADER_SIZE + BS_MAT_SIZE) - sizeof(mat_end_code), mat_end_code, sizeof(mat_end_code));
		m_hdmisize = (BS_HEADER_SIZE + BS_MAT_SIZE);

		hr = DeliverBitstream(m_hdmibuff + BS_HEADER_SIZE, m_hdmisize - BS_HEADER_SIZE, IEC61937_TRUEHD, m_truehd_samplerate, m_truehd_framelength * 24);
		m_hdmicount = 0;
		m_hdmisize  = 0;
		if (FAILED(hr)) {
			return hr;
		}
	}


	ENDDATA

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessDTS_SPDIF(BOOL bEOF/* = FALSE*/)
{
	HRESULT hr;

	BEGINDATA

	BOOL bUseDTSHDBitstream = GetSPDIF(dtshd) && m_bBitstreamSupported[DTSHD];

	while (p + 16 <= end) {
		audioframe_t aframe;
		int size  = ParseDTSHeader(p, &aframe);
		if (size == 0) {
			p++;
			continue;
		}

		int sizehd = 0;
		if (p + size + 16 <= end) {
			sizehd = ParseDTSHDHeader(p + size);
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
			if (FAILED(hr = DeliverBitstream(p, size + sizehd, IEC61937_DTSHD, aframe.samplerate, aframe.samples))) {
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
					DbgLog((LOG_TRACE, 3, L"CMpaDecFilter:ProcessDTS_SPDIF() - framelength is not supported"));
					return E_FAIL;
			}
			if (FAILED(hr = DeliverBitstream(p, size, type, aframe.samplerate >> aframe.param2, aframe.samples))) {
				return hr;
			}
		}

		p += (size + sizehd);
	}

	ENDDATA

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMraw() //'raw '
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t size       = m_buff.GetCount();
	size_t nSamples   = size * 8 / wfe->wBitsPerSample;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	CAtlArray<BYTE> outBuff;
	outBuff.SetCount(size);

	switch (wfe->wBitsPerSample) {
		case 8: // unsigned 8-bit
			out_sf = SAMPLE_FMT_U8;
			memcpy(outBuff.GetData(), m_buff.GetData(), size);
		break;
		case 16: { // signed big-endian 16-bit
			out_sf = SAMPLE_FMT_S16;
			uint16_t* pIn  = (uint16_t*)m_buff.GetData();
			uint16_t* pOut = (uint16_t*)outBuff.GetData();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = bswap_16(pIn[i]);
			}
		}
		break;
	}

	HRESULT hr;
	if (S_OK != (hr = Deliver(outBuff.GetData(), size, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.RemoveAll();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMintBE() // 'twos', big-endian 'in24' and 'in32'
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t nSamples   = m_buff.GetCount() * 8 / wfe->wBitsPerSample;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	size_t outSize = nSamples * (wfe->wBitsPerSample <= 16 ? 2 : 4); // convert to 16 and 32-bit
	CAtlArray<BYTE> outBuff;
	outBuff.SetCount(outSize);

	switch (wfe->wBitsPerSample) {
		case 8: { //signed 8-bit
			out_sf = SAMPLE_FMT_S16;
			int8_t*  pIn  = (int8_t*)m_buff.GetData();
			int16_t* pOut = (int16_t*)outBuff.GetData();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = (int16_t)pIn[i] << 8;
			}
		}
		break;
		case 16: { // signed big-endian 16-bit
			out_sf = SAMPLE_FMT_S16;
			uint16_t* pIn  = (uint16_t*)m_buff.GetData(); // signed take as an unsigned to shift operations.
			uint16_t* pOut = (uint16_t*)outBuff.GetData();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = bswap_16(pIn[i]);
			}
		}
		break;
		case 24: { // signed big-endian 24-bit
			out_sf = SAMPLE_FMT_S32;
			uint8_t*  pIn  = (uint8_t*)m_buff.GetData();
			uint32_t* pOut = (uint32_t*)outBuff.GetData();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = (uint32_t)pIn[3 * i]     << 24 |
						  (uint32_t)pIn[3 * i + 1] << 16 |
						  (uint32_t)pIn[3 * i + 2] << 8;
			}
		}
		break;
		case 32: { // signed big-endian 32-bit
			out_sf = SAMPLE_FMT_S32;
			uint32_t* pIn  = (uint32_t*)m_buff.GetData(); // signed take as an unsigned to shift operations.
			uint32_t* pOut = (uint32_t*)outBuff.GetData();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = bswap_32(pIn[i]);
			}
		}
		break;
	}

	HRESULT hr;
	if (S_OK != (hr = Deliver(outBuff.GetData(), outSize, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.RemoveAll();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMintLE() // 'sowt', little-endian 'in24' and 'in32'
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t nSamples   = m_buff.GetCount() * 8 / wfe->wBitsPerSample;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	size_t outSize = nSamples * (wfe->wBitsPerSample <= 16 ? 2 : 4); // convert to 16 and 32-bit
	CAtlArray<BYTE> outBuff;
	outBuff.SetCount(outSize);

	switch (wfe->wBitsPerSample) {
		case 8: { //signed 8-bit
			out_sf = SAMPLE_FMT_S16;
			int8_t*  pIn  = (int8_t*)m_buff.GetData();
			int16_t* pOut = (int16_t*)outBuff.GetData();

			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = (int16_t)pIn[i] << 8;
			}
		}
		break;
		case 16: // signed little-endian 16-bit
			out_sf = SAMPLE_FMT_S16;
			memcpy(outBuff.GetData(), m_buff.GetData(), outSize);
			break;
		case 24: // signed little-endian 24-bit
			out_sf = SAMPLE_FMT_S32;
			convert_int24_to_int32(nSamples, (uint8_t*)m_buff.GetData(), (int32_t*)outBuff.GetData());
			break;
		case 32: // signed little-endian 32-bit
			out_sf = SAMPLE_FMT_S32;
			memcpy(outBuff.GetData(), m_buff.GetData(), outSize);
			break;
	}

	HRESULT hr;
	if (S_OK != (hr = Deliver(outBuff.GetData(), outSize, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.RemoveAll();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMfloatBE() // big-endian 'fl32' and 'fl64'
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t size       = m_buff.GetCount();
	size_t nSamples   = size * 8 / wfe->wBitsPerSample;

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	CAtlArray<BYTE> outBuff;
	outBuff.SetCount(size);

	switch (wfe->wBitsPerSample) {
		case 32: {
			out_sf = SAMPLE_FMT_FLT;
			uint32_t* pIn  = (uint32_t*)m_buff.GetData();
			uint32_t* pOut = (uint32_t*)outBuff.GetData();
			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = bswap_32(pIn[i]);
			}
		}
		break;
		case 64: {
			out_sf = SAMPLE_FMT_DBL;
			uint64_t* pIn  = (uint64_t*)m_buff.GetData();
			uint64_t* pOut = (uint64_t*)outBuff.GetData();
			for (size_t i = 0; i < nSamples; i++) {
				pOut[i] = bswap_64(pIn[i]);
			}
		}
		break;
	}

	HRESULT hr;
	if (S_OK != (hr = Deliver(outBuff.GetData(), size, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.RemoveAll();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPCMfloatLE() // little-endian 'fl32' and 'fl64'
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	size_t size = m_buff.GetCount();

	SampleFormat out_sf = SAMPLE_FMT_NONE;
	CAtlArray<BYTE> outBuff;
	outBuff.SetCount(size);

	switch (wfe->wBitsPerSample) {
		case 32:
			out_sf = SAMPLE_FMT_FLT;
			break;
		case 64:
			out_sf = SAMPLE_FMT_DBL;
			break;
	}
	memcpy(outBuff.GetData(), m_buff.GetData(), size);

	HRESULT hr;
	if (S_OK != (hr = Deliver(outBuff.GetData(), size, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
		return hr;
	}

	m_buff.RemoveAll();
	return S_OK;
}

HRESULT CMpaDecFilter::ProcessPS2PCM()
{
	BEGINDATA

	WAVEFORMATEXPS2* wfe = (WAVEFORMATEXPS2*)m_pInput->CurrentMediaType().Format();
	size_t size = wfe->dwInterleave * wfe->nChannels;

	SampleFormat out_sf = SAMPLE_FMT_S16P;
	CAtlArray<BYTE> outBuff;
	outBuff.SetCount(size);

	while (p + size <= end) {
		DWORD* dw = (DWORD*)p;

		if (dw[0] == 'dhSS') {
			p += dw[1] + 8;
		} else if (dw[0] == 'dbSS') {
			p += 8;
			m_ps2_state.sync = true;
		} else {
			if (m_ps2_state.sync) {
				memcpy(outBuff.GetData(), p, size);
			} else {
				memset(outBuff.GetData(), 0, size);
			}

			HRESULT hr;
			if (S_OK != (hr = Deliver(outBuff.GetData(), size, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
				return hr;
			}

			p += size;
		}
	}

	ENDDATA

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

		*pout++ = (int16_t)min(max(INT16_MIN, output), INT16_MAX);
	}
}

HRESULT CMpaDecFilter::ProcessPS2ADPCM()
{
	BEGINDATA

	WAVEFORMATEXPS2* wfe = (WAVEFORMATEXPS2*)m_pInput->CurrentMediaType().Format();
	size_t size  = wfe->dwInterleave * wfe->nChannels;
	int samples = wfe->dwInterleave * 14 / 16 * 2;
	int channels = wfe->nChannels;

	SampleFormat out_sf = SAMPLE_FMT_S16P;
	size_t outSize = samples * channels * sizeof(int16_t);
	CAtlArray<BYTE> outBuff;
	outBuff.SetCount(outSize);
	int16_t* pOut = (int16_t*)outBuff.GetData();

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
				memset(outBuff.GetData(), 0, outSize);
			}

			HRESULT hr;
			if (S_OK != (hr = Deliver(outBuff.GetData(), outSize, out_sf, wfe->nSamplesPerSec, wfe->nChannels))) {
				return hr;
			}

			p += size;
		}
	}

	ENDDATA

	return S_OK;
}

HRESULT CMpaDecFilter::GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData)
{
	HRESULT hr;
	*pData = NULL;

	if (FAILED(hr = m_pOutput->GetDeliveryBuffer(pSample, NULL, NULL, 0))
			|| FAILED(hr = (*pSample)->GetPointer(pData))) {
		return hr;
	}

	AM_MEDIA_TYPE* pmt = NULL;
	if (SUCCEEDED((*pSample)->GetMediaType(&pmt)) && pmt) {
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
		pmt = NULL;
	}

	return S_OK;
}

HRESULT CMpaDecFilter::Deliver(BYTE* pBuff, size_t size, SampleFormat sfmt, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask)
{
	if (dwChannelMask == 0) {
		dwChannelMask = GetDefChannelMask(nChannels);
	}
	ASSERT(nChannels == av_popcount(dwChannelMask));

	m_InternalSampleFormat = sfmt;

#if ENABLE_AC3_ENCODER
	if (m_bBitstreamSupported[SPDIF] && GetSPDIF(ac3enc) && nChannels > 2) { // do not encode mono and stereo
		return AC3Encode(pBuff, size, sfmt, nSamplesPerSec, nChannels, dwChannelMask);
	}
#endif

	int nSamples = size / (nChannels * get_bytes_per_sample(sfmt));

	REFERENCE_TIME rtStart, rtStop;
	CalculateDuration(nSamples, nSamplesPerSec, rtStart, rtStop);

	if (rtStart < 0) {
		return S_OK;
	}

	MPCSampleFormat out_mpcsf = SelectOutputFormat(SamplefmtToMPC[sfmt]);
	const SampleFormat out_sf = MPCtoSamplefmt[out_mpcsf];

	BYTE*  pDataIn  = pBuff;
	CAtlArray<float> mixData;

	CMediaType mt = CreateMediaType(out_mpcsf, nSamplesPerSec, nChannels, dwChannelMask);
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	HRESULT hr;
	if (FAILED(hr = ReconnectOutput(nSamples, mt))) {
		return hr;
	}

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;
	if (FAILED(GetDeliveryBuffer(&pOut, &pDataOut))) {
		return E_FAIL;
	}

	if (hr == S_OK) {
		m_pOutput->SetMediaType(&mt);
		pOut->SetMediaType(&mt);
	}

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(NULL, NULL);

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

HRESULT CMpaDecFilter::DeliverBitstream(BYTE* pBuff, int size, WORD type, int sample_rate, int samples)
{
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
			length = BS_TRUEHD_SIZE;
			isHDMI = true;
			break;
		default:
			DbgLog((LOG_TRACE, 3, L"CMpaDecFilter::DeliverBitstream() - type is not supported"));
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
	BYTE* pDataOut = NULL;
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
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetPreroll(FALSE);
	pOut->SetDiscontinuity(m_bDiscontinuity);
	m_bDiscontinuity = FALSE;
	pOut->SetSyncPoint(TRUE);

	pOut->SetActualDataLength(length);

	return m_pOutput->Deliver(pOut);
}

#if ENABLE_AC3_ENCODER
HRESULT CMpaDecFilter::AC3Encode(BYTE* pBuff, int size, SampleFormat sfmt, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask)
{
	DWORD new_layout     = m_AC3Enc.SelectLayout(dwChannelMask);
	WORD  new_channels   = av_popcount(new_layout);
	DWORD new_samplerate = m_AC3Enc.SelectSamplerate(nSamplesPerSec);

	if (!m_AC3Enc.OK()) {
		m_AC3Enc.Init(new_samplerate, new_layout);
		m_encbuff.RemoveAll();
	}

	int nSamples = size / (nChannels * get_bytes_per_sample(sfmt));

	size_t remain = m_encbuff.GetCount();
	float* p;

	if (new_layout == dwChannelMask && new_samplerate == nSamplesPerSec) {
		int added  = size / get_bytes_per_sample(sfmt);
		m_encbuff.SetCount(remain + added);
		p = m_encbuff.GetData() + remain;

		convert_to_float(sfmt, nChannels, nSamples, pBuff, p);
	} else {
		m_Mixer.UpdateInput(sfmt, dwChannelMask, nSamplesPerSec);
		m_Mixer.UpdateOutput(SAMPLE_FMT_FLT, new_layout, new_samplerate);
		int added = m_Mixer.CalcOutSamples(nSamples) * new_channels;
		m_encbuff.SetCount(remain + added);
		p = m_encbuff.GetData() + remain;

		added = m_Mixer.Mixing((BYTE*)p, added, pBuff, nSamples) * new_channels;
		if (added <= 0) {
			m_encbuff.SetCount(remain);
			return S_OK;
		}
		m_encbuff.SetCount(remain + added);
	}

	CAtlArray<BYTE> output;

	while (m_AC3Enc.Encode(m_encbuff, output) == S_OK) {
		DeliverBitstream(output.GetData(), output.GetCount(), IEC61937_AC3, nSamplesPerSec, 1536);
	}

	return S_OK;
}
#endif

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
		rtStart = m_rtStart, m_rtStart = rtStop = m_rtStart + 200000;
	} else {
		// Length of the sample
		double dDuration = (double)samples / sample_rate * 10000000.0;
		m_dStartOffset += fmod(dDuration, 1.0);

		// Delivery Timestamps
		rtStart = m_rtStart, rtStop = m_rtStart + (REFERENCE_TIME)(dDuration + 0.5);

		// Compute next start time
		m_rtStart += (REFERENCE_TIME)dDuration;
		// If the offset reaches one (100ns), add it to the next frame
		if (m_dStartOffset > 0.5) {
			m_rtStart++;
			m_dStartOffset -= 1.0;
		}
	}
}

HRESULT CMpaDecFilter::CheckInputType(const CMediaType* mtIn)
{
	if (mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mtIn->Format();
		if (wfe->nChannels < 1 || wfe->nChannels > 8 || (wfe->wBitsPerSample != 16 && wfe->wBitsPerSample != 20 && wfe->wBitsPerSample != 24)) {
			return VFW_E_TYPE_NOT_ACCEPTED;
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

	CMediaType& mt		= m_pInput->CurrentMediaType();
	WAVEFORMATEX* wfe	= (WAVEFORMATEX*)mt.Format();
	UNREFERENCED_PARAMETER(wfe);

	pProperties->cBuffers = 4;
	// pProperties->cbBuffer = 1;
	pProperties->cbBuffer = 48000*6*(32/8)/10; // 48KHz 6ch 32bps 100ms
	pProperties->cbAlign = 1;
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
	if (wfe == NULL) {
		return E_INVALIDARG;
	}

	if (iPosition == 0) {
		const GUID& subtype = mt.subtype;
		if (GetSPDIF(ac3) && (subtype == MEDIASUBTYPE_DOLBY_AC3 || subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3) ||
			GetSPDIF(dts) && (subtype == MEDIASUBTYPE_DTS || subtype == MEDIASUBTYPE_DTS2)
#if ENABLE_AC3_ENCODER
			|| GetSPDIF(ac3enc) /*&& wfe->nChannels > 2*/
#endif
		) {
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
	DWORD out_layout;

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

		if (mt.subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
			WAVEFORMATEX_HDMV_LPCM* wfelpcm = (WAVEFORMATEX_HDMV_LPCM*)mt.Format();
			out_layout = s_scmap_hdmv[wfelpcm->channel_conf].dwChannelMask;
		} else {
			out_layout = GetDefChannelMask(wfe->nChannels);
		}
	}

	*pmt = CreateMediaType(out_mpcsf, out_samplerate, out_channels, out_layout);

	return S_OK;
}

HRESULT CMpaDecFilter::StartStreaming()
{
	HRESULT hr = __super::StartStreaming();
	if (FAILED(hr)) {
		return hr;
	}

	m_ps2_state.reset();

	if (!m_bDoAdditionalCheck) {
		m_bDoAdditionalCheck = TRUE;
		memset(&m_bBitstreamSupported, FALSE, sizeof(m_bBitstreamSupported));

		CComPtr<IPin> pPin = m_pOutput;
		CComPtr<IPin> pPinRenderer;
		for (CComPtr<IBaseFilter> pBF = this; pBF = GetDownStreamFilter(pBF, pPin); pPin = GetFirstPin(pBF, PINDIR_OUTPUT)) {
			if (IsAudioWaveRenderer(pBF) && SUCCEEDED(pPin->ConnectedTo(&pPinRenderer)) && pPinRenderer) {
				break;
			}
		}

		if (pPinRenderer) {
			CMediaType mtOutput;
			m_pOutput->ConnectionMediaType(&mtOutput);

			CMediaType mt;
			mt = CreateMediaTypeSPDIF();
			m_bBitstreamSupported[SPDIF]		= pPinRenderer->QueryAccept(&mt) == S_OK;

			if (IsWinVistaOrLater()) {
				mt = CreateMediaTypeHDMI(IEC61937_EAC3);
				m_bBitstreamSupported[EAC3]		= pPinRenderer->QueryAccept(&mt) == S_OK;

				mt = CreateMediaTypeHDMI(IEC61937_TRUEHD);
				m_bBitstreamSupported[TRUEHD]	= pPinRenderer->QueryAccept(&mt) == S_OK;

				mt = CreateMediaTypeHDMI(IEC61937_DTSHD);
				m_bBitstreamSupported[DTSHD]	= pPinRenderer->QueryAccept(&mt) == S_OK;
			}

			pPinRenderer->QueryAccept(&mtOutput);
		}

		BOOL b_HasVideo = FALSE;
		if (CComPtr<IBaseFilter> pFilter = GetFilterFromPin(m_pInput->GetConnected())) {
			if (GetCLSID(pFilter) == GUIDFromCString(_T("{09144FD6-BB29-11DB-96F1-005056C00008}"))) { // PBDA DTFilter
				b_HasVideo = TRUE;
			} else {
				BeginEnumPins(pFilter, pEP, pPin) {
					BeginEnumMediaTypes(pPin, pEM, pmt) {
						if (pmt->majortype == MEDIATYPE_Video || pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO) {
							b_HasVideo = TRUE;
							break;
						}
					}
					EndEnumMediaTypes(pmt)
					if (b_HasVideo) {
						break;
					}
				}
				EndEnumFilters
			}
		}

		m_bHasVideo = b_HasVideo;
	}

	return S_OK;
}

HRESULT CMpaDecFilter::StopStreaming()
{
	m_FFAudioDec.StreamFinish();

	return __super::StopStreaming();
}

HRESULT CMpaDecFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType *pmt)
{
	if (dir == PINDIR_INPUT) {
		if (m_FFAudioDec.GetCodecId() != AV_CODEC_ID_NONE) {
			m_FFAudioDec.StreamFinish();
		}

		enum AVCodecID nCodecId = FindCodec(pmt->subtype);
		if (nCodecId != AV_CODEC_ID_NONE) {
			if (m_FFAudioDec.Init(nCodecId, m_pInput)) {
				m_FFAudioDec.SetDRC(GetDynamicRangeControl());
			} else {
				return VFW_E_TYPE_NOT_ACCEPTED;
			}
		}
	}

	return __super::SetMediaType(dir, pmt);
}

// IMpaDecFilter

STDMETHODIMP CMpaDecFilter::SetOutputFormat(MPCSampleFormat mpcsf, bool enable)
{
	CAutoLock cAutoLock(&m_csProps);
	if (mpcsf >= 0 && mpcsf < sfcount) {
		m_fSampleFmt[mpcsf] = enable;
	} else {
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetOutputFormat(MPCSampleFormat mpcsf)
{
	CAutoLock cAutoLock(&m_csProps);
	if (mpcsf >= 0 && mpcsf < sfcount) {
		return m_fSampleFmt[mpcsf];
	}
	return false;
}

STDMETHODIMP_(MPCSampleFormat) CMpaDecFilter::SelectOutputFormat(MPCSampleFormat mpcsf)
{
	CAutoLock cAutoLock(&m_csProps);
	if (mpcsf >= 0 && mpcsf < sfcount && m_fSampleFmt[mpcsf]) {
		return mpcsf;
	}

	if (m_fSampleFmt[SF_FLOAT]) {
		return SF_FLOAT;
	}
	if (m_fSampleFmt[SF_PCM24]) {
		return SF_PCM24;
	}
	if (m_fSampleFmt[SF_PCM16]) {
		return SF_PCM16;
	}
	if (m_fSampleFmt[SF_PCM32]) {
		return SF_PCM32;
	}
	return SF_PCM16;
}

STDMETHODIMP CMpaDecFilter::SetDynamicRangeControl(bool fDRC)
{
	CAutoLock cAutoLock(&m_csProps);
	m_fDRC = fDRC;

	m_FFAudioDec.SetDRC(fDRC);

	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetDynamicRangeControl()
{
	CAutoLock cAutoLock(&m_csProps);

	return m_fDRC;
}

STDMETHODIMP CMpaDecFilter::SetSPDIF(enctype et, bool fSPDIF)
{
	CAutoLock cAutoLock(&m_csProps);
	if (et < 0 || et >= etcount) {
		return E_INVALIDARG;
	}

	m_fSPDIF[et] = fSPDIF;
	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetSPDIF(enctype et)
{
	CAutoLock cAutoLock(&m_csProps);
	if (et < 0 || et >= etcount) {
		return false;
	}
	if (et == dtshd && !m_fSPDIF[dts]) {
		return false;
	}

	return m_fSPDIF[et];
}

STDMETHODIMP CMpaDecFilter::SaveSettings()
{
	CAutoLock cAutoLock(&m_csProps);

#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_MpaDec)) {
		key.SetDWORDValue(OPTION_SFormat_i16, m_fSampleFmt[SF_PCM16]);
		key.SetDWORDValue(OPTION_SFormat_i24, m_fSampleFmt[SF_PCM24]);
		key.SetDWORDValue(OPTION_SFormat_i32, m_fSampleFmt[SF_PCM32]);
		key.SetDWORDValue(OPTION_SFormat_flt, m_fSampleFmt[SF_FLOAT]);
		key.SetDWORDValue(OPTION_DRC, m_fDRC);
		key.SetDWORDValue(OPTION_SPDIF_ac3, m_fSPDIF[ac3]);
		key.SetDWORDValue(OPTION_SPDIF_eac3, m_fSPDIF[eac3]);
		key.SetDWORDValue(OPTION_SPDIF_truehd, m_fSPDIF[truehd]);
		key.SetDWORDValue(OPTION_SPDIF_dts, m_fSPDIF[dts]);
		key.SetDWORDValue(OPTION_SPDIF_dtshd, m_fSPDIF[dtshd]);
#if ENABLE_AC3_ENCODER
		key.SetDWORDValue(OPTION_SPDIF_ac3enc, m_fSPDIF[ac3enc]);
#endif
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SFormat_i16, m_fSampleFmt[SF_PCM16]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SFormat_i24, m_fSampleFmt[SF_PCM24]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SFormat_i32, m_fSampleFmt[SF_PCM32]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SFormat_flt, m_fSampleFmt[SF_FLOAT]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_DRC, m_fDRC);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_ac3, m_fSPDIF[ac3]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_eac3, m_fSPDIF[eac3]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_truehd, m_fSPDIF[truehd]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_dts, m_fSPDIF[dts]);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_dtshd, m_fSPDIF[dtshd]);
#if ENABLE_AC3_ENCODER
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MpaDec, OPTION_SPDIF_ac3enc, m_fSPDIF[ac3enc]);
#endif
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

	return NULL;
}

STDMETHODIMP_(CString) CMpaDecFilter::GetInformation(MPCAInfo index)
{
	CAutoLock cAutoLock(&m_csProps);
	CString infostr;

	if (index == AINFO_MPCVersion) {
		infostr.Format(_T("v%d.%d.%d (build %d)"), MPC_VERSION_MAJOR, MPC_VERSION_MINOR, MPC_VERSION_PATCH, MPC_VERSION_REV);

		return infostr;
	}

	if (index == AINFO_DecoderInfo) {
		// Input
		DWORD samplerate = 0;
		WORD channels = 0;
		DWORD layout = 0;

		if (m_FFAudioDec.GetCodecId() != AV_CODEC_ID_NONE) {
			infostr.Format(L"Codec: %hS", m_FFAudioDec.GetCodecName());
			WORD bitdeph = m_FFAudioDec.GetCoddedBitdepth();
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
				infostr += "DVD LPCM";
			}
			else if (subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
				infostr += "HDMV LPCM";
			}
			else if (subtype == MEDIASUBTYPE_PS2_PCM) {
				infostr += "PS2 PCM";
			}
			else if (subtype == MEDIASUBTYPE_PS2_ADPCM) {
				infostr += "PS2 ADPCM";
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
				infostr += "PCM";
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
				infostr.AppendFormat(L"Channels: %u  [%hS]\r\n", channels, str);
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
						infostr += L" -> HDMI E-AC3";
						// wfeout samplerate, channels and bit depth is not actual
					}
					else if (wfexout->SubFormat == KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_MLP) {
						infostr += " -> HDMI TrueHD";
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

	return NULL;
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

	if (*ppPage != NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMpaDecSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpaDecSettingsWnd>(NULL, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}
