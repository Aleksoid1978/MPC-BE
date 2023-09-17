/*
 * (C) 2013-2023 see Authors.txt
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

#define ENTRYNAME(subtype) #subtype
static const struct {
	WORD wFormatTag;
	const CHAR* szName;
}
MPC_g_WaveGuidNames[] = {
	// mmreg.h
	{WAVE_FORMAT_ADPCM,               "MS_ADPCM"},  //
	{WAVE_FORMAT_ALAW,                "ALAW"},
	{WAVE_FORMAT_MULAW,               "MULAW"},
	{WAVE_FORMAT_IMA_ADPCM,           "IMA_ADPCM"}, //
	{WAVE_FORMAT_WMAVOICE9,           "WMSP1"},
	{WAVE_FORMAT_DSPGROUP_TRUESPEECH, "TRUESPEECH"},
	{WAVE_FORMAT_GSM610,              "GSM610"},    //
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
	{WAVE_FORMAT_MPEG_ADTS_AAC,       "MPEG_ADTS_AAC"}, //
	{WAVE_FORMAT_MPEG_RAW_AAC,        "MPEG_RAW_AAC"},  //
	{WAVE_FORMAT_MPEG_LOAS,           "MPEG_LOAS"},     //
	{WAVE_FORMAT_MPEG_HEAAC,          "MPEG_HEAAC"},    //
	{WAVE_FORMAT_WAVPACK_AUDIO,       "WAVPACK4"},
	{WAVE_FORMAT_OPUS,                "OPUS_WAVE"},
	{WAVE_FORMAT_SPEEX_VOICE,         "SPEEX"},
	{WAVE_FORMAT_FLAC,                "FLAC"},
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

/*
// 
static const struct {
	uint32_t fourcc;
	const CHAR* szName;
}
MPC_g_FourCCGuidNames[] = {

}
*/

#define ADDENTRY(subtype) { #subtype, subtype },
static const GUID_STRING_ENTRY MPC_g_GuidNames[] = {
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
	ADDENTRY(MEDIATYPE_Subtitle)
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
	if (guid == GUID_NULL) {
		// to prevent print TIME_FORMAT_NONE for GUID_NULL
		return "GUID_NULL";
	}

	const char* guidStr = GuidNames[guid]; // GUID names from uuids.h
	if (strcmp(guidStr, "Unknown GUID Name") != 0) {
		return guidStr;
	}

	if (memcmp(&guid.Data2, &MEDIASUBTYPE_YUY2.Data2, sizeof(GUID) - sizeof(GUID::Data1)) == 0) {
		// GUID like {xxxxxxxx-0000-0010-8000-00AA00389B71}
		CStringA str = "MEDIASUBTYPE_";

		if ((guid.Data1 & 0x0000FFFF) == guid.Data1) {
			const WORD wFormatTag = (WORD)guid.Data1;
			for (const auto& waveGuidName : MPC_g_WaveGuidNames) {
				if (waveGuidName.wFormatTag == wFormatTag) {
					str.Append(waveGuidName.szName);
					return str;
				}
			}
			str.AppendFormat("0x%04x", wFormatTag);
			return str;
		}

		uint32_t fourcc = guid.Data1;
		for (unsigned i = 0; i < 4; i++) {
			const uint32_t c = fourcc & 0xff;
			str.AppendFormat(c < 32 ? "[%u]" : "%c", c);
			fourcc >>= 8;
		}
		return str;
	}

	for (const auto& guidName : MPC_g_GuidNames) {
		if (guidName.guid == guid) {
			return guidName.szName;
		}
	}

	return "Unknown GUID Name";
}
