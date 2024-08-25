/*
 * (C) 2013-2024 see Authors.txt
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
#include <wmcodecdsp.h>
#include <moreuuids.h>
#include <encdec.h> // MEDIASUBTYPE_CPFilters_Processed, FORMATTYPE_CPFilters_Processed
#include "GUIDString.h"

static const WaveStringEntry MPC_g_WaveGuidNames[] = {
	// mmreg.h
	{WAVE_FORMAT_ADPCM,               "MS_ADPCM"},
	{WAVE_FORMAT_ALAW,                "ALAW"},
	{WAVE_FORMAT_MULAW,               "MULAW"},
	{WAVE_FORMAT_IMA_ADPCM,           "IMA_ADPCM"},
	{WAVE_FORMAT_WMAVOICE9,           "WMSP1"},
	{WAVE_FORMAT_DSPGROUP_TRUESPEECH, "TRUESPEECH"},
	{WAVE_FORMAT_GSM610,              "GSM610"},
	{WAVE_FORMAT_SHARP_G726,          "G726_ADPCM"},
	{WAVE_FORMAT_MPEGLAYER3,          "MP3"},
	{WAVE_FORMAT_VOXWARE_RT29,        "VOXWARE_RT29"},
	{WAVE_FORMAT_RAW_AAC1,            "RAW_AAC1"},
	{WAVE_FORMAT_MSAUDIO1,            "MSAUDIO1"},
	{WAVE_FORMAT_WMAUDIO2,            "WMAUDIO2"},
	{WAVE_FORMAT_WMAUDIO3,            "WMAUDIO3"},
	{WAVE_FORMAT_WMAUDIO_LOSSLESS,    "WMAUDIO_LOSSLESS"},
	{WAVE_FORMAT_INTEL_MUSIC_CODER,   "INTEL_MUSIC"},
	{WAVE_FORMAT_INDEO_AUDIO,         "INDEO_AUDIO"},
	{WAVE_FORMAT_DTS2,                "DTS2"},
	{WAVE_FORMAT_MPEG_ADTS_AAC,       "MPEG_ADTS_AAC"},
	{WAVE_FORMAT_MPEG_RAW_AAC,        "MPEG_RAW_AAC"},
	{WAVE_FORMAT_MPEG_LOAS,           "MPEG_LOAS"},
	{WAVE_FORMAT_MPEG_HEAAC,          "MPEG_HEAAC"},
	{WAVE_FORMAT_WAVPACK_AUDIO,       "WAVPACK4"},
	{WAVE_FORMAT_OPUS,                "OPUS_WAVE"},
	{WAVE_FORMAT_SPEEX_VOICE,         "SPEEX"},
	{WAVE_FORMAT_FLAC,                "FLAC"},
	{WAVE_FORMAT_EXTENSIBLE,          "WAVEFORMATEXTENSIBLE"}, // invalid media subtype from "AVI/WAV File Source"
	// moreuuids.h
	{WAVE_FORMAT_SIPR,                "SIPR_WAVE"},      //0x0130
	{WAVE_FORMAT_ATRC,                "ATRAC3"},         //0x0270
	{WAVE_FORMAT_LATM_AAC,            "LATM_AAC"},       //0x01FF
	{WAVE_FORMAT_DOLBY_AC3,           "WAVE_DOLBY_AC3"}, //0x2000
	{WAVE_FORMAT_ADPCM_SWF,           "ADPCM_SWF"},      //0x5346
	{WAVE_FORMAT_TTA1 ,               "TTA1"},           //0x77A1
	{WAVE_FORMAT_PS2_PCM,             "PS2_PCM"},        //0xF521
	{WAVE_FORMAT_PS2_ADPCM,           "PS2_ADPCM"},      //0xF522
};

#define ADDENTRY(subtype) { subtype, #subtype },
static const GuidStringEntry MPC_g_GuidNames[] = {
	ADDENTRY(MEDIASUBTYPE_FLAC_FRAMED)
	ADDENTRY(MEDIASUBTYPE_TAK)
	ADDENTRY(MEDIASUBTYPE_WavpackHybrid)
	ADDENTRY(MEDIASUBTYPE_SVCD_SUBPICTURE)
	ADDENTRY(MEDIASUBTYPE_CVD_SUBPICTURE)
	ADDENTRY(MEDIASUBTYPE_MPEG2_PVA)
	ADDENTRY(MEDIASUBTYPE_DirectShowMedia)
	ADDENTRY(MEDIASUBTYPE_Dirac)
	ADDENTRY(FORMAT_DiracVideoInfo)
	ADDENTRY(MEDIASUBTYPE_MP4)
	ADDENTRY(MEDIASUBTYPE_FLV)
	ADDENTRY(MEDIASUBTYPE_RealMedia)
	ADDENTRY(MEDIASUBTYPE_ATRAC3plus)
	ADDENTRY(MEDIASUBTYPE_ATRAC9)
	ADDENTRY(MEDIASUBTYPE_PS2_SUB)
	ADDENTRY(MEDIASUBTYPE_Ogg)
	ADDENTRY(MEDIASUBTYPE_Vorbis)
	ADDENTRY(FORMAT_VorbisFormat)
	ADDENTRY(MEDIASUBTYPE_Vorbis2)
	ADDENTRY(FORMAT_VorbisFormat2)
	ADDENTRY(MEDIASUBTYPE_Matroska)
	ADDENTRY(MEDIASUBTYPE_UTF8)
	ADDENTRY(MEDIASUBTYPE_SSA)
	ADDENTRY(MEDIASUBTYPE_ASS)
	ADDENTRY(MEDIASUBTYPE_ASS2)
	ADDENTRY(MEDIASUBTYPE_SSF)
	ADDENTRY(MEDIASUBTYPE_USF)
	ADDENTRY(MEDIASUBTYPE_VOBSUB)
	ADDENTRY(FORMAT_SubtitleInfo)
	ADDENTRY(MEDIASUBTYPE_HDMVSUB)
	ADDENTRY(MEDIASUBTYPE_ArcsoftH264)
	ADDENTRY(MEDIASUBTYPE_H264_bis)
	ADDENTRY(MEDIASUBTYPE_WVC1_ARCSOFT)
	ADDENTRY(FORMAT_RLTheora)
	ADDENTRY(MEDIASUBTYPE_RoQ)
	ADDENTRY(MEDIASUBTYPE_BD_LPCM_AUDIO)
	ADDENTRY(MEDIASUBTYPE_MUSEPACK_Stream)
	ADDENTRY(MEDIASUBTYPE_WAVPACK_Stream)
	ADDENTRY(MEDIASUBTYPE_TTA1_Stream)
	ADDENTRY(MEDIASUBTYPE_HDMV_LPCM_AUDIO)
	ADDENTRY(MEDIASUBTYPE_DOLBY_DDPLUS_ARCSOFT)
	ADDENTRY(MEDIASUBTYPE_FFMPEG_AUDIO)
	ADDENTRY(FORMAT_WaveFormatExFFMPEG)
	ADDENTRY(MEDIASUBTYPE_TAK_Stream)
	ADDENTRY(MEDIASUBTYPE_DOLBY_DDPLUS)
	ADDENTRY(MEDIASUBTYPE_DOLBY_TRUEHD)
	ADDENTRY(MEDIASUBTYPE_DTS_HD)
	ADDENTRY(MEDIASUBTYPE_CPFilters_Processed)
	ADDENTRY(FORMATTYPE_CPFilters_Processed)
	ADDENTRY(MEDIASUBTYPE_LAV_RAWVIDEO)
	ADDENTRY(MEDIASUBTYPE_WEBVTT)
};
#undef ADDENTRY

CStringA GetGUIDName(const GUID& guid)
{
	return GuidNames.GetString(guid).c_str();
}

void SetExtraGuidStrings()
{
	GuidNames.SetExtraGuidStrings(
		&MPC_g_GuidNames[0], std::size(MPC_g_GuidNames),
		&MPC_g_WaveGuidNames[0], std::size(MPC_g_WaveGuidNames)
	);
}
