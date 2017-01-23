/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include "DSUtil.h"
#include "MediaTypeEx.h"

#include <MMReg.h>
#include <InitGuid.h>
#include <moreuuids.h>
#include <basestruct.h>
#include <d3d9types.h>
#include <dxva.h>
#include <dxva2api.h>

#include "GUIDString.h"

CMediaTypeEx::CMediaTypeEx()
{
}

CString CMediaTypeEx::ToString(IPin* pPin)
{
	CString packing, type, codec, dim, rate, dur;

	// TODO

	if (majortype == MEDIATYPE_DVD_ENCRYPTED_PACK) {
		packing = L"Encrypted MPEG2 Pack";
	} else if (majortype == MEDIATYPE_MPEG2_PACK) {
		packing = L"MPEG2 Pack";
	} else if (majortype == MEDIATYPE_MPEG2_PES) {
		packing = L"MPEG2 PES";
	}

	if (majortype == MEDIATYPE_Video || subtype == MEDIASUBTYPE_MPEG2_VIDEO) {
		type = L"Video";

		BITMAPINFOHEADER bih;
		bool fBIH = ExtractBIH(this, &bih);

		int w, h, arx, ary;
		bool fDim = ExtractDim(this, w, h, arx, ary);

		if (fBIH) {
			codec = GetVideoCodecName(subtype, bih.biCompression);
		}

		if (codec.IsEmpty()) {
			if (formattype == FORMAT_MPEGVideo) {
				codec = L"MPEG1 Video";
			} else if (formattype == FORMAT_MPEG2_VIDEO) {
				codec = L"MPEG2 Video";
			} else if (formattype == FORMAT_DiracVideoInfo) {
				codec = L"Dirac Video";
			}
		}

		if (fDim) {
			dim.Format(L"%dx%d", w, h);
			if (w*ary != h*arx) {
				dim.AppendFormat(L" (%d:%d)", arx, ary);
			}
		}

		if (formattype == FORMAT_VideoInfo || formattype == FORMAT_MPEGVideo) {
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pbFormat;
			if (vih->AvgTimePerFrame) {
				rate.Format(L"%0.3f", 10000000.0f / vih->AvgTimePerFrame);
				rate.TrimRight('0'); // remove trailing zeros
				rate.TrimRight('.'); // remove the trailing dot
				rate += L"fps ";
			}
			if (vih->dwBitRate) {
				rate.AppendFormat(L"%ukbps", vih->dwBitRate/1000);
			}
		} else if (formattype == FORMAT_VideoInfo2 || formattype == FORMAT_MPEG2_VIDEO || formattype == FORMAT_DiracVideoInfo) {
			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pbFormat;
			if (vih->AvgTimePerFrame) {
				rate.Format(L"%0.3f", 10000000.0f / vih->AvgTimePerFrame);
				rate.TrimRight('0'); // remove trailing zeros
				rate.TrimRight('.'); // remove the trailing dot
				rate += L"fps ";
			}
			if (vih->dwBitRate) {
				rate.AppendFormat(L"%ukbps", vih->dwBitRate/1000);
			}
		}

		rate.TrimRight();

		if (subtype == MEDIASUBTYPE_DVD_SUBPICTURE) {
			type = L"Subtitle";
			codec = L"DVD Subpicture";
		}
	} else if (majortype == MEDIATYPE_Audio || subtype == MEDIASUBTYPE_DOLBY_AC3) {
		type = L"Audio";

		if (formattype == FORMAT_WaveFormatEx) {
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)Format();

			if (wfe->wFormatTag/* > WAVE_FORMAT_PCM && wfe->wFormatTag < WAVE_FORMAT_EXTENSIBLE
			&& wfe->wFormatTag != WAVE_FORMAT_IEEE_FLOAT*/
					|| subtype != GUID_NULL) {
				codec = GetAudioCodecName(subtype, wfe->wFormatTag);
				dim.Format(L"%uHz", wfe->nSamplesPerSec);
				if (wfe->nChannels == 1) {
					dim += L" mono";
				} else if (wfe->nChannels == 2) {
					dim += L" stereo";
				} else {
					dim.AppendFormat(L" %uch", wfe->nChannels);
				}
				if (wfe->nAvgBytesPerSec) {
					rate.Format(L"%ukbps", wfe->nAvgBytesPerSec*8/1000);
				}
			}
		} else if (formattype == FORMAT_VorbisFormat) {
			VORBISFORMAT* vf = (VORBISFORMAT*)Format();

			codec = GetAudioCodecName(subtype, 0);
			dim.Format(L"%uHz", vf->nSamplesPerSec);
			if (vf->nChannels == 1) {
				dim += L" mono";
			} else if (vf->nChannels == 2) {
				dim += L" stereo";
			} else {
				dim.AppendFormat(L" %uch", vf->nChannels);
			}
			if (vf->nAvgBitsPerSec) {
				rate.Format(L"%ukbps", vf->nAvgBitsPerSec/1000);
			}
		} else if (formattype == FORMAT_VorbisFormat2) {
			VORBISFORMAT2* vf = (VORBISFORMAT2*)Format();

			codec = GetAudioCodecName(subtype, 0);
			dim.Format(L"%uHz", vf->SamplesPerSec);
			if (vf->Channels == 1) {
				dim += L" mono";
			} else if (vf->Channels == 2) {
				dim += L" stereo";
			} else {
				dim.AppendFormat(L" %uch", vf->Channels);
			}
		}
	} else if (majortype == MEDIATYPE_Text) {
		type = L"Text";
	} else if (majortype == MEDIATYPE_Subtitle || subtype == MEDIASUBTYPE_DVD_SUBPICTURE) {
		type = L"Subtitle";
		codec = GetSubtitleCodecName(subtype);
	} else {
		type = L"Unknown";
	}

	if (CComQIPtr<IMediaSeeking> pMS = pPin) {
		REFERENCE_TIME rtDur = 0;
		if (SUCCEEDED(pMS->GetDuration(&rtDur)) && rtDur) {
			rtDur /= 10000000;
			int s = rtDur%60;
			rtDur /= 60;
			int m = rtDur%60;
			rtDur /= 60;
			int h = (int)rtDur;
			if (h) {
				dur.Format(L"%d:%02d:%02d", h, m, s);
			} else if (m) {
				dur.Format(L"%02d:%02d", m, s);
			} else if (s) {
				dur.Format(L"%ds", s);
			}
		}
	}

	CString str;
	if (!codec.IsEmpty()) {
		str += codec + L" ";
	}
	if (!dim.IsEmpty()) {
		str += dim + L" ";
	}
	if (!rate.IsEmpty()) {
		str += rate + L" ";
	}
	if (!dur.IsEmpty()) {
		str += dur + L" ";
	}
	str.Trim(L" ,");

	if (!str.IsEmpty()) {
		str = type + L": " + str;
	} else {
		str = type;
	}

	return str;
}

CString CMediaTypeEx::GetVideoCodecName(const GUID& subtype, DWORD biCompression)
{
	CString str;

	static CAtlMap<DWORD, CString> names;
	if (names.IsEmpty()) {
		names['WMV1'] = L"Windows Media Video 7";
		names['WMV2'] = L"Windows Media Video 8";
		names['WMV3'] = L"Windows Media Video 9";
		names['DIV3'] = L"DivX 3";
		names['MP43'] = L"MSMPEG4v3";
		names['MP42'] = L"MSMPEG4v2";
		names['MP41'] = L"MSMPEG4v1";
		names['DX30'] = L"DivX 3";
		names['DX50'] = L"DivX 5";
		names['DIVX'] = L"DivX 6";
		names['XVID'] = L"Xvid";
		names['MP4V'] = L"MPEG4 Video";
		names['AVC1'] = L"H.264/AVC";
		names['H264'] = L"H.264/AVC";
		names['RV10'] = L"RealVideo 1";
		names['RV20'] = L"RealVideo 2";
		names['RV30'] = L"RealVideo 3";
		names['RV40'] = L"RealVideo 4";
		names['FLV1'] = L"Flash Video 1";
		names['FLV4'] = L"Flash Video 4";
		names['VP50'] = L"On2 VP5";
		names['VP60'] = L"On2 VP6";
		names['SVQ3'] = L"SVQ3";
		names['SVQ1'] = L"SVQ1";
		names['H263'] = L"H263";
		names['DRAC'] = L"Dirac";
		names['WVC1'] = L"VC-1";
		names['THEO'] = L"Theora";
		names['HVC1'] = L"HEVC";
		names['HM91'] = L"HEVC(HM9.1)";
		names['HM10'] = L"HEVC(HM10)";
		names['HM12'] = L"HEVC(HM12)";
	}

	if (biCompression != BI_RGB && biCompression != BI_BITFIELDS) {
		DWORD fourcc = FCC(biCompression);
		BYTE* b = (BYTE*)&fourcc;

		for (size_t i = 0; i < 4; i++) {
			if (b[i] >= 'a' && b[i] <= 'z') {
				b[i] = b[i] + 32;
			}
		}

		if (!names.Lookup(fourcc, str)) {
			if (subtype == MEDIASUBTYPE_DiracVideo) {
				str = L"Dirac Video";
			} else if (subtype == MEDIASUBTYPE_apch ||
					   subtype == MEDIASUBTYPE_apcn ||
					   subtype == MEDIASUBTYPE_apcs ||
					   subtype == MEDIASUBTYPE_apco ||
					   subtype == MEDIASUBTYPE_ap4h) {
				str.Format(L"ProRes Video (%4.4hs)", &biCompression);
			} else if (biCompression < 256) {
				str.Format(L"%u", biCompression);
			} else {
				str.Format(L"%4.4hs", &biCompression);
			}
		}
	} else {
		if (subtype == MEDIASUBTYPE_RGB32)
			str = L"RGB32";
		else if (subtype == MEDIASUBTYPE_RGB24)
			str = L"RGB24";
		else if (subtype == MEDIASUBTYPE_RGB555)
			str = L"RGB555";
		else if (subtype == MEDIASUBTYPE_RGB565)
			str = L"RGB565";
		else if (subtype == MEDIASUBTYPE_ARGB32)
			str = L"ARGB32";
		else if (subtype == MEDIASUBTYPE_RGB8)
			str = L"RGB8";
	}

	return str;
}

CString CMediaTypeEx::GetAudioCodecName(const GUID& subtype, WORD wFormatTag)
{
	CString str;

	static CAtlMap<WORD, CString> names;
	if (names.IsEmpty()) {
		// MMReg.h
		names[WAVE_FORMAT_ADPCM]                 = L"MS ADPCM";
		names[WAVE_FORMAT_IEEE_FLOAT]            = L"IEEE Float";
		names[WAVE_FORMAT_ALAW]                  = L"aLaw";
		names[WAVE_FORMAT_MULAW]                 = L"muLaw";
		names[WAVE_FORMAT_DTS]                   = L"DTS";
		names[WAVE_FORMAT_DRM]                   = L"DRM";
		names[WAVE_FORMAT_WMAVOICE9]             = L"WMA Voice";
		names[WAVE_FORMAT_WMAVOICE10]            = L"WMA Voice";
		names[WAVE_FORMAT_OKI_ADPCM]             = L"OKI ADPCM";
		names[WAVE_FORMAT_IMA_ADPCM]             = L"IMA ADPCM";
		names[WAVE_FORMAT_MEDIASPACE_ADPCM]      = L"Mediaspace ADPCM";
		names[WAVE_FORMAT_SIERRA_ADPCM]          = L"Sierra ADPCM";
		names[WAVE_FORMAT_G723_ADPCM]            = L"G723 ADPCM";
		names[WAVE_FORMAT_DIALOGIC_OKI_ADPCM]    = L"Dialogic OKI ADPCM";
		names[WAVE_FORMAT_MEDIAVISION_ADPCM]     = L"Media Vision ADPCM";
		names[WAVE_FORMAT_YAMAHA_ADPCM]          = L"Yamaha ADPCM";
		names[WAVE_FORMAT_DSPGROUP_TRUESPEECH]   = L"DSP Group Truespeech";
		names[WAVE_FORMAT_DOLBY_AC2]             = L"Dolby AC2";
		names[WAVE_FORMAT_GSM610]                = L"GSM610";
		names[WAVE_FORMAT_MSNAUDIO]              = L"MSN Audio";
		names[WAVE_FORMAT_ANTEX_ADPCME]          = L"Antex ADPCME";
		names[WAVE_FORMAT_CS_IMAADPCM]           = L"Crystal Semiconductor IMA ADPCM";
		names[WAVE_FORMAT_ROCKWELL_ADPCM]        = L"Rockwell ADPCM";
		names[WAVE_FORMAT_ROCKWELL_DIGITALK]     = L"Rockwell Digitalk";
		names[WAVE_FORMAT_G721_ADPCM]            = L"G721";
		names[WAVE_FORMAT_G728_CELP]             = L"G728";
		names[WAVE_FORMAT_MSG723]                = L"MSG723";
		names[WAVE_FORMAT_MPEG]                  = L"MPEG Audio";
		names[WAVE_FORMAT_MPEGLAYER3]            = L"MP3";
		names[WAVE_FORMAT_LUCENT_G723]           = L"Lucent G723";
		names[WAVE_FORMAT_VOXWARE]               = L"Voxware";
		names[WAVE_FORMAT_G726_ADPCM]            = L"G726";
		names[WAVE_FORMAT_G722_ADPCM]            = L"G722";
		names[WAVE_FORMAT_G729A]                 = L"G729A";
		names[WAVE_FORMAT_MEDIASONIC_G723]       = L"MediaSonic G723";
		names[WAVE_FORMAT_ZYXEL_ADPCM]           = L"ZyXEL ADPCM";
		names[WAVE_FORMAT_RAW_AAC1]              = L"AAC";
		names[WAVE_FORMAT_RHETOREX_ADPCM]        = L"Rhetorex ADPCM";
		names[WAVE_FORMAT_VIVO_G723]             = L"Vivo G723";
		names[WAVE_FORMAT_VIVO_SIREN]            = L"Vivo Siren";
		names[WAVE_FORMAT_DIGITAL_G723]          = L"Digital G723";
		names[WAVE_FORMAT_SANYO_LD_ADPCM]        = L"Sanyo LD ADPCM";
		names[WAVE_FORMAT_MSAUDIO1]              = L"WMA 1";
		names[WAVE_FORMAT_WMAUDIO2]              = L"WMA 2";
		names[WAVE_FORMAT_WMAUDIO3]              = L"WMA Pro";
		names[WAVE_FORMAT_WMAUDIO_LOSSLESS]      = L"WMA Lossless";
		names[WAVE_FORMAT_CREATIVE_ADPCM]        = L"Creative ADPCM";
		names[WAVE_FORMAT_CREATIVE_FASTSPEECH8]  = L"Creative Fastspeech 8";
		names[WAVE_FORMAT_CREATIVE_FASTSPEECH10] = L"Creative Fastspeech 10";
		names[WAVE_FORMAT_UHER_ADPCM]            = L"UHER ADPCM";
		names[WAVE_FORMAT_DTS2]                  = L"DTS";
		names[WAVE_FORMAT_DOLBY_AC3_SPDIF]       = L"S/PDIF";
		// other
		names[WAVE_FORMAT_DOLBY_AC3]             = L"Dolby AC3";
		names[WAVE_FORMAT_LATM_AAC]              = L"AAC(LATM)";
		names[WAVE_FORMAT_FLAC]                  = L"FLAC";
		names[WAVE_FORMAT_TTA1]                  = L"TTA";
		names[WAVE_FORMAT_WAVPACK4]              = L"WavPack";
		names[WAVE_FORMAT_14_4]                  = L"RealAudio 14.4";
		names[WAVE_FORMAT_28_8]                  = L"RealAudio 28.8";
		names[WAVE_FORMAT_ATRC]                  = L"RealAudio ATRC";
		names[WAVE_FORMAT_COOK]                  = L"RealAudio COOK";
		names[WAVE_FORMAT_DNET]                  = L"RealAudio DNET";
		names[WAVE_FORMAT_RAAC]                  = L"RealAudio RAAC";
		names[WAVE_FORMAT_RACP]                  = L"RealAudio RACP";
		names[WAVE_FORMAT_SIPR]                  = L"RealAudio SIPR";
		names[WAVE_FORMAT_PS2_PCM]               = L"PS2 PCM";
		names[WAVE_FORMAT_PS2_ADPCM]             = L"PS2 ADPCM";
		names[WAVE_FORMAT_SPEEX]                 = L"Speex";
		names[WAVE_FORMAT_ADX_ADPCM]             = L"ADX ADPCM";
	}

	if (!names.Lookup(wFormatTag, str)) {
		// for wFormatTag equal to WAVE_FORMAT_UNKNOWN, WAVE_FORMAT_PCM, WAVE_FORMAT_EXTENSIBLE and other.
		static CAtlMap<GUID, CString> guidnames;
		if (guidnames.IsEmpty()) {
			guidnames[MEDIASUBTYPE_PCM]				= L"PCM";
			guidnames[MEDIASUBTYPE_IEEE_FLOAT]		= L"IEEE Float";
			guidnames[MEDIASUBTYPE_DVD_LPCM_AUDIO]	= L"PCM";
			guidnames[MEDIASUBTYPE_HDMV_LPCM_AUDIO]	= L"LPCM";
			guidnames[MEDIASUBTYPE_Vorbis]			= L"Vorbis (deprecated)";
			guidnames[MEDIASUBTYPE_Vorbis2]			= L"Vorbis";
			guidnames[MEDIASUBTYPE_MP4A]			= L"MPEG4 Audio";
			guidnames[MEDIASUBTYPE_FLAC_FRAMED]		= L"FLAC (framed)";
			guidnames[MEDIASUBTYPE_DOLBY_AC3]		= L"Dolby AC3";
			guidnames[MEDIASUBTYPE_DOLBY_DDPLUS]	= L"DD+";
			guidnames[MEDIASUBTYPE_DOLBY_TRUEHD]	= L"TrueHD";
			guidnames[MEDIASUBTYPE_DTS]				= L"DTS";
			guidnames[MEDIASUBTYPE_MLP]				= L"MLP";
			guidnames[MEDIASUBTYPE_ALAC]			= L"ALAC";
			guidnames[MEDIASUBTYPE_PCM_NONE]		= L"QT PCM";
			guidnames[MEDIASUBTYPE_PCM_RAW]			= L"QT PCM";
			guidnames[MEDIASUBTYPE_PCM_TWOS]		= L"PCM";
			guidnames[MEDIASUBTYPE_PCM_SOWT]		= L"QT PCM";
			guidnames[MEDIASUBTYPE_PCM_IN24]		= L"PCM";
			guidnames[MEDIASUBTYPE_PCM_IN32]		= L"PCM";
			guidnames[MEDIASUBTYPE_PCM_FL32]		= L"QT PCM";
			guidnames[MEDIASUBTYPE_PCM_FL64]		= L"QT PCM";
			guidnames[MEDIASUBTYPE_IMA4]			= L"ADPCM";
			guidnames[MEDIASUBTYPE_ADPCM_SWF]		= L"ADPCM";
			guidnames[MEDIASUBTYPE_IMA_AMV]			= L"ADPCM";
			guidnames[MEDIASUBTYPE_ALS]				= L"ALS";
			guidnames[MEDIASUBTYPE_QDMC]			= L"QDMC";
			guidnames[MEDIASUBTYPE_QDM2]			= L"QDM2";
			guidnames[MEDIASUBTYPE_RoQA]			= L"ROQA";
			guidnames[MEDIASUBTYPE_APE]				= L"APE";
			guidnames[MEDIASUBTYPE_AMR]				= L"AMR";
			guidnames[MEDIASUBTYPE_SAMR]			= L"AMR";
			guidnames[MEDIASUBTYPE_SAWB]			= L"AMR";
			guidnames[MEDIASUBTYPE_OPUS]			= L"Opus";
			guidnames[MEDIASUBTYPE_BINKA_DCT]		= L"BINK DCT";
			guidnames[MEDIASUBTYPE_AAC_ADTS]		= L"AAC";
			guidnames[MEDIASUBTYPE_DSD1]			= L"DSD";
			guidnames[MEDIASUBTYPE_DSD8]			= L"DSD";
			guidnames[MEDIASUBTYPE_DSDL]			= L"DSD";
			guidnames[MEDIASUBTYPE_DSDM]			= L"DSD";
			guidnames[MEDIASUBTYPE_NELLYMOSER]		= L"Nelly Moser";
		}

		if (!guidnames.Lookup(subtype, str)) {
			str.Format(L"0x%04x", wFormatTag);
		}
	}

	return str;
}

CString CMediaTypeEx::GetSubtitleCodecName(const GUID& subtype)
{
	CString str;

	static CAtlMap<GUID, CString> names;
	if (names.IsEmpty()) {
		names[MEDIASUBTYPE_UTF8]			= L"UTF-8";
		names[MEDIASUBTYPE_SSA]				= L"SubStation Alpha";
		names[MEDIASUBTYPE_ASS]				= L"Advanced SubStation Alpha";
		names[MEDIASUBTYPE_ASS2]			= L"Advanced SubStation Alpha";
		names[MEDIASUBTYPE_USF]				= L"Universal Subtitle Format";
		names[MEDIASUBTYPE_VOBSUB]			= L"VobSub";
		names[MEDIASUBTYPE_DVB_SUBTITLES]	= L"DVB";
		names[MEDIASUBTYPE_DVD_SUBPICTURE]	= L"DVD Subtitles";
		names[MEDIASUBTYPE_HDMVSUB]			= L"PGS";
	}

	if (names.Lookup(subtype, str)) {

	}

	return str;
}

#define ADDENTRY(mode) DXVA_names[mode] = #mode
CString GetGUIDString(const GUID& guid)
{
	static CAtlMap<GUID, CHAR*> DXVA_names;
	{
		// GUID names from dxva.h which are synonyms in dxva2api.h
		ADDENTRY(DXVA_ModeNone);
		ADDENTRY(DXVA_ModeMPEG1_VLD);
		ADDENTRY(DXVA_ModeMPEG2and1_VLD);
		ADDENTRY(DXVA_ModeH264_MoComp_NoFGT);
		ADDENTRY(DXVA_ModeH264_MoComp_FGT);
		ADDENTRY(DXVA_ModeH264_IDCT_NoFGT);
		ADDENTRY(DXVA_ModeH264_IDCT_FGT);
		ADDENTRY(DXVA_ModeH264_VLD_NoFGT);
		ADDENTRY(DXVA_ModeH264_VLD_FGT);
		ADDENTRY(DXVA_ModeH264_VLD_WithFMOASO_NoFGT);
		ADDENTRY(DXVA_ModeH264_VLD_Stereo_Progressive_NoFGT);
		ADDENTRY(DXVA_ModeH264_VLD_Stereo_NoFGT);
		ADDENTRY(DXVA_ModeH264_VLD_Multiview_NoFGT);
		ADDENTRY(DXVA_ModeWMV8_PostProc);
		ADDENTRY(DXVA_ModeWMV8_MoComp);
		ADDENTRY(DXVA_ModeWMV9_PostProc);
		ADDENTRY(DXVA_ModeWMV9_MoComp);
		ADDENTRY(DXVA_ModeWMV9_IDCT);
		ADDENTRY(DXVA_ModeVC1_PostProc);
		ADDENTRY(DXVA_ModeVC1_MoComp);
		ADDENTRY(DXVA_ModeVC1_IDCT);
		ADDENTRY(DXVA_ModeVC1_VLD);
		ADDENTRY(DXVA_ModeVC1_D2010);
		ADDENTRY(DXVA_NoEncrypt);
		ADDENTRY(DXVA_ModeMPEG4pt2_VLD_Simple);
		ADDENTRY(DXVA_ModeMPEG4pt2_VLD_AdvSimple_NoGMC);
		ADDENTRY(DXVA_ModeMPEG4pt2_VLD_AdvSimple_GMC);
		ADDENTRY(DXVA_ModeHEVC_VLD_Main);
		ADDENTRY(DXVA_ModeHEVC_VLD_Main10);

		// GUID names from dxva2api.h
		ADDENTRY(DXVA2_ModeMPEG2_MoComp);
		ADDENTRY(DXVA2_ModeMPEG2_IDCT);
		ADDENTRY(DXVA2_ModeMPEG2_VLD);
	}

	// to prevent print TIME_FORMAT_NONE for GUID_NULL
	if (guid == GUID_NULL) {
		return L"GUID_NULL";
	}

	CHAR* guidStr = GuidNames[guid]; // GUID names from uuids.h
	if (strcmp(guidStr, "Unknown GUID Name") == 0) {
		guidStr = m_GuidNames[guid]; // GUID names from moreuuids.h
	}
	if (strcmp(guidStr, "Unknown GUID Name") == 0) {
		CHAR* str;
		if (DXVA_names.Lookup(guid, str)) {
			guidStr = str;
		} else if (memcmp(&guid.Data2, &MEDIASUBTYPE_YUY2.Data2, sizeof(GUID)- sizeof(GUID::Data1)) == 0 // GUID like {xxxxxxxx-0000-0010-8000-00AA00389B71}
				&& (guid.Data1 & 0xE0E0E0E0)) { // and FOURCC with chars >= 0x20
			static CHAR s_guidstr[] = "MEDIASUBTYPE_XXXX";
			memcpy(&s_guidstr[13], &guid.Data1, sizeof(guid.Data1));
			guidStr = s_guidstr;
		}
	}

	return CString(guidStr);
}

void CMediaTypeEx::Dump(CAtlList<CString>& sl)
{
	CString str;

	sl.RemoveAll();

	ULONG fmtsize = 0;

	CString major = CStringFromGUID(majortype);
	CString sub = CStringFromGUID(subtype);
	CString format = CStringFromGUID(formattype);

	sl.AddTail(ToString());
	sl.AddTail(L"");

	sl.AddTail(L"AM_MEDIA_TYPE: ");
	str.Format(L"majortype: %s %s", GetGUIDString(majortype), major);
	sl.AddTail(str);
	if (majortype == MEDIATYPE_Video && subtype == FOURCCMap(BI_RLE8)) { // fake subtype for RLE 8-bit
		str.Format(L"subtype: BI_RLE8 %s", sub);
	} else if (majortype == MEDIATYPE_Video && subtype == FOURCCMap(BI_RLE4)) { // fake subtype for RLE 4-bit
		str.Format(L"subtype: BI_RLE4 %s", sub);
	} else {
		str.Format(L"subtype: %s %s", GetGUIDString(subtype), sub);
	}
	sl.AddTail(str);
	str.Format(L"formattype: %s %s", GetGUIDString(formattype), format);
	sl.AddTail(str);
	str.Format(L"bFixedSizeSamples: %d", bFixedSizeSamples);
	sl.AddTail(str);
	str.Format(L"bTemporalCompression: %d", bTemporalCompression);
	sl.AddTail(str);
	str.Format(L"lSampleSize: %u", lSampleSize);
	sl.AddTail(str);
	str.Format(L"cbFormat: %u", cbFormat);
	sl.AddTail(str);

	sl.AddTail(L"");

	if (formattype == FORMAT_VideoInfo || formattype == FORMAT_VideoInfo2
			|| formattype == FORMAT_MPEGVideo || formattype == FORMAT_MPEG2_VIDEO) {

		fmtsize =
			formattype == FORMAT_VideoInfo   ? sizeof(VIDEOINFOHEADER)    :
			formattype == FORMAT_VideoInfo2  ? sizeof(VIDEOINFOHEADER2)   :
			formattype == FORMAT_MPEGVideo   ? sizeof(MPEG1VIDEOINFO) - 1 :
			formattype == FORMAT_MPEG2_VIDEO ? sizeof(MPEG2VIDEOINFO) - 4 :
			0;

		VIDEOINFOHEADER& vih = *(VIDEOINFOHEADER*)pbFormat;
		BITMAPINFOHEADER* bih = &vih.bmiHeader;

		sl.AddTail(L"VIDEOINFOHEADER:");
		str.Format(L"rcSource: (%d,%d)-(%d,%d)", vih.rcSource.left, vih.rcSource.top, vih.rcSource.right, vih.rcSource.bottom);
		sl.AddTail(str);
		str.Format(L"rcTarget: (%d,%d)-(%d,%d)", vih.rcTarget.left, vih.rcTarget.top, vih.rcTarget.right, vih.rcTarget.bottom);
		sl.AddTail(str);
		str.Format(L"dwBitRate: %u", vih.dwBitRate);
		sl.AddTail(str);
		str.Format(L"dwBitErrorRate: %u", vih.dwBitErrorRate);
		sl.AddTail(str);
		str.Format(L"AvgTimePerFrame: %I64d", vih.AvgTimePerFrame);
		if (vih.AvgTimePerFrame > 0) {
			str.AppendFormat(L" (%.3f fps)", 10000000.0 / vih.AvgTimePerFrame);
		}
		sl.AddTail(str);

		sl.AddTail(L"");

		if (formattype == FORMAT_VideoInfo2 || formattype == FORMAT_MPEG2_VIDEO) {
			VIDEOINFOHEADER2& vih2 = *(VIDEOINFOHEADER2*)pbFormat;
			bih = &vih2.bmiHeader;

			sl.AddTail(L"VIDEOINFOHEADER2:");
			str.Format(L"dwInterlaceFlags: 0x%08x", vih2.dwInterlaceFlags);
			sl.AddTail(str);
			str.Format(L"dwCopyProtectFlags: 0x%08x", vih2.dwCopyProtectFlags);
			sl.AddTail(str);
			str.Format(L"dwPictAspectRatioX: %u", vih2.dwPictAspectRatioX);
			sl.AddTail(str);
			str.Format(L"dwPictAspectRatioY: %u", vih2.dwPictAspectRatioY);
			sl.AddTail(str);
			str.Format(L"dwControlFlags: 0x%08x", vih2.dwControlFlags);
			sl.AddTail(str);
			if (vih2.dwControlFlags & (AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT)) {
				// http://msdn.microsoft.com/en-us/library/windows/desktop/ms698715%28v=vs.85%29.aspx
				const LPCSTR nominalrange[] = { NULL, "0-255", "16-235", "48-208" };
				const LPCSTR transfermatrix[] = { NULL, "BT.709", "BT.601", "SMPTE 240M", "BT.2020", NULL, NULL, "YCgCo" };
				const LPCSTR lighting[] = { NULL, "bright", "office", "dim", "dark" };
				const LPCSTR primaries[] = { NULL, "Reserved", "BT.709", "BT.470-4 System M", "BT.470-4 System B,G",
					"SMPTE 170M", "SMPTE 240M", "EBU Tech. 3213", "SMPTE", "BT.2020" };
				const LPCSTR transfunc[] = { NULL, "Linear RGB", "1.8 gamma", "2.0 gamma", "2.2 gamma", "BT.709", "SMPTE 240M",
					"sRGB", "2.8 gamma", "Log100", "Log316", "Symmetric BT.709", NULL, NULL, NULL, NULL, "SMPTE ST 2084" };

#define ADD_PARAM_DESC(str, parameter, descs) if (parameter < _countof(descs) && descs[parameter]) str.AppendFormat(L" (%hS)", descs[parameter])

				DXVA2_ExtendedFormat exfmt;
				exfmt.value = vih2.dwControlFlags;

				str.Format(L"- VideoChromaSubsampling: %u", exfmt.VideoChromaSubsampling);
				sl.AddTail(str);

				str.Format(L"- NominalRange          : %u", exfmt.NominalRange);
				ADD_PARAM_DESC(str, exfmt.NominalRange, nominalrange);
				sl.AddTail(str);

				str.Format(L"- VideoTransferMatrix   : %u", exfmt.VideoTransferMatrix);
				ADD_PARAM_DESC(str, exfmt.VideoTransferMatrix, transfermatrix);
				sl.AddTail(str);

				str.Format(L"- VideoLighting         : %u", exfmt.VideoLighting);
				ADD_PARAM_DESC(str, exfmt.VideoLighting, lighting);
				sl.AddTail(str);

				str.Format(L"- VideoPrimaries        : %u", exfmt.VideoPrimaries);
				ADD_PARAM_DESC(str, exfmt.VideoPrimaries, primaries);
				sl.AddTail(str);

				str.Format(L"- VideoTransferFunction : %u", exfmt.VideoTransferFunction);
				ADD_PARAM_DESC(str, exfmt.VideoTransferFunction, transfunc);
				sl.AddTail(str);
			}
			str.Format(L"dwReserved2: 0x%08x", vih2.dwReserved2);
			sl.AddTail(str);

			sl.AddTail(L"");
		}

		if (formattype == FORMAT_MPEGVideo) {
			MPEG1VIDEOINFO& mvih = *(MPEG1VIDEOINFO*)pbFormat;

			sl.AddTail(L"MPEG1VIDEOINFO:");
			str.Format(L"dwStartTimeCode: %u", mvih.dwStartTimeCode);
			sl.AddTail(str);
			str.Format(L"cbSequenceHeader: %u", mvih.cbSequenceHeader);
			sl.AddTail(str);

			sl.AddTail(L"");
		} else if (formattype == FORMAT_MPEG2_VIDEO) {
			MPEG2VIDEOINFO& mvih = *(MPEG2VIDEOINFO*)pbFormat;

			sl.AddTail(L"MPEG2VIDEOINFO:");
			str.Format(L"dwStartTimeCode: %d", mvih.dwStartTimeCode);
			sl.AddTail(str);
			str.Format(L"cbSequenceHeader: %d", mvih.cbSequenceHeader);
			sl.AddTail(str);
			str.Format(L"dwProfile: 0x%08x", mvih.dwProfile);
			sl.AddTail(str);
			str.Format(L"dwLevel: 0x%08x", mvih.dwLevel);
			sl.AddTail(str);
			str.Format(L"dwFlags: 0x%08x", mvih.dwFlags);
			sl.AddTail(str);

			sl.AddTail(L"");
		}

		sl.AddTail(L"BITMAPINFOHEADER:");
		str.Format(L"biSize: %u", bih->biSize);
		sl.AddTail(str);
		str.Format(L"biWidth: %d", bih->biWidth);
		sl.AddTail(str);
		str.Format(L"biHeight: %d", bih->biHeight);
		sl.AddTail(str);
		str.Format(L"biPlanes: %u", bih->biPlanes);
		sl.AddTail(str);
		str.Format(L"biBitCount: %u", bih->biBitCount);
		sl.AddTail(str);
		if (bih->biCompression < 256) {
			str.Format(L"biCompression: %u", bih->biCompression);
		} else {
			str.Format(L"biCompression: %4.4hs", &bih->biCompression);
		}
		sl.AddTail(str);
		str.Format(L"biSizeImage: %d", bih->biSizeImage);
		sl.AddTail(str);
		str.Format(L"biXPelsPerMeter: %d", bih->biXPelsPerMeter);
		sl.AddTail(str);
		str.Format(L"biYPelsPerMeter: %d", bih->biYPelsPerMeter);
		sl.AddTail(str);
		str.Format(L"biClrUsed: %u", bih->biClrUsed);
		sl.AddTail(str);
		str.Format(L"biClrImportant: %u", bih->biClrImportant);
		sl.AddTail(str);
	} else if (formattype == FORMAT_WaveFormatEx || formattype == FORMAT_WaveFormatExFFMPEG) {
		WAVEFORMATEX *pWfe = NULL;
		if (formattype == FORMAT_WaveFormatExFFMPEG) {
			fmtsize = sizeof(WAVEFORMATEXFFMPEG);

			WAVEFORMATEXFFMPEG *wfeff = (WAVEFORMATEXFFMPEG*)pbFormat;
			pWfe = &wfeff->wfex;

			sl.AddTail(L"WAVEFORMATEXFFMPEG:");
			str.Format(L"nCodecId: 0x%04x", wfeff->nCodecId);
			sl.AddTail(str);
			sl.AddTail(L"");
		} else {
			fmtsize = sizeof(WAVEFORMATEX);
			pWfe = (WAVEFORMATEX*)pbFormat;
		}

		WAVEFORMATEX& wfe = *pWfe;

		sl.AddTail(L"WAVEFORMATEX:");
		str.Format(L"wFormatTag: 0x%04x", wfe.wFormatTag);
		sl.AddTail(str);
		str.Format(L"nChannels: %u", wfe.nChannels);
		sl.AddTail(str);
		str.Format(L"nSamplesPerSec: %u", wfe.nSamplesPerSec);
		sl.AddTail(str);
		str.Format(L"nAvgBytesPerSec: %u", wfe.nAvgBytesPerSec);
		sl.AddTail(str);
		str.Format(L"nBlockAlign: %u", wfe.nBlockAlign);
		sl.AddTail(str);
		str.Format(L"wBitsPerSample: %u", wfe.wBitsPerSample);
		sl.AddTail(str);
		str.Format(L"cbSize: %u (extra bytes)", wfe.cbSize);
		sl.AddTail(str);

		if (wfe.wFormatTag != WAVE_FORMAT_PCM && wfe.cbSize > 0 && formattype == FORMAT_WaveFormatEx) {
			if (wfe.wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe.cbSize == sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)) {
				sl.AddTail(L"");

				fmtsize = sizeof(WAVEFORMATEXTENSIBLE);

				WAVEFORMATEXTENSIBLE& wfex = *(WAVEFORMATEXTENSIBLE*)pbFormat;

				sl.AddTail(L"WAVEFORMATEXTENSIBLE:");
				if (wfex.Format.wBitsPerSample != 0) {
					str.Format(L"wValidBitsPerSample: %u", wfex.Samples.wValidBitsPerSample);
				} else {
					str.Format(L"wSamplesPerBlock: %u", wfex.Samples.wSamplesPerBlock);
				}
				sl.AddTail(str);
				str.Format(L"dwChannelMask: 0x%08x", wfex.dwChannelMask);
				sl.AddTail(str);
				str.Format(L"SubFormat: %s", CStringFromGUID(wfex.SubFormat));
				sl.AddTail(str);
			} else if (wfe.wFormatTag == WAVE_FORMAT_DOLBY_AC3 && wfe.cbSize == sizeof(DOLBYAC3WAVEFORMAT)-sizeof(WAVEFORMATEX)) {
				sl.AddTail(L"");

				fmtsize = sizeof(DOLBYAC3WAVEFORMAT);

				DOLBYAC3WAVEFORMAT& ac3wf = *(DOLBYAC3WAVEFORMAT*)pbFormat;

				sl.AddTail(L"DOLBYAC3WAVEFORMAT:");
				str.Format(L"bBigEndian: %u", ac3wf.bBigEndian);
				sl.AddTail(str);
				str.Format(L"bsid: %u", ac3wf.bsid);
				sl.AddTail(str);
				str.Format(L"lfeon: %u", ac3wf.lfeon);
				sl.AddTail(str);
				str.Format(L"copyrightb: %u", ac3wf.copyrightb);
				sl.AddTail(str);
				str.Format(L"nAuxBitsCode: %u", ac3wf.nAuxBitsCode);
				sl.AddTail(str);
			}
		}
	} else if (formattype == FORMAT_VorbisFormat) {
		fmtsize = sizeof(VORBISFORMAT);

		VORBISFORMAT& vf = *(VORBISFORMAT*)pbFormat;

		sl.AddTail(L"VORBISFORMAT:");
		str.Format(L"nChannels: %u", vf.nChannels);
		sl.AddTail(str);
		str.Format(L"nSamplesPerSec: %u", vf.nSamplesPerSec);
		sl.AddTail(str);
		str.Format(L"nMinBitsPerSec: %u", vf.nMinBitsPerSec);
		sl.AddTail(str);
		str.Format(L"nAvgBitsPerSec: %u", vf.nAvgBitsPerSec);
		sl.AddTail(str);
		str.Format(L"nMaxBitsPerSec: %u", vf.nMaxBitsPerSec);
		sl.AddTail(str);
		str.Format(L"fQuality: %.3f", vf.fQuality);
		sl.AddTail(str);
	} else if (formattype == FORMAT_VorbisFormat2) {
		fmtsize = sizeof(VORBISFORMAT2);

		VORBISFORMAT2& vf = *(VORBISFORMAT2*)pbFormat;

		sl.AddTail(L"VORBISFORMAT:");
		str.Format(L"Channels: %u", vf.Channels);
		sl.AddTail(str);
		str.Format(L"SamplesPerSec: %u", vf.SamplesPerSec);
		sl.AddTail(str);
		str.Format(L"BitsPerSample: %u", vf.BitsPerSample);
		sl.AddTail(str);
		str.Format(L"HeaderSize: {%u, %u, %u}", vf.HeaderSize[0], vf.HeaderSize[1], vf.HeaderSize[2]);
		sl.AddTail(str);
	} else if (formattype == FORMAT_SubtitleInfo) {
		fmtsize = sizeof(SUBTITLEINFO);

		SUBTITLEINFO& si = *(SUBTITLEINFO*)pbFormat;

		sl.AddTail(L"SUBTITLEINFO:");
		str.Format(L"dwOffset: %u", si.dwOffset);
		sl.AddTail(str);
		str.Format(L"IsoLang: %s", CString(si.IsoLang, _countof(si.IsoLang) - 1));
		sl.AddTail(str);
		str.Format(L"TrackName: %s", CString(si.TrackName, _countof(si.TrackName) - 1));
		sl.AddTail(str);
	}

	if (fmtsize < cbFormat) { // extra and unknown data
		sl.AddTail(L"");

		ULONG extrasize = cbFormat - fmtsize;
		str.Format(L"Extradata: %u", extrasize);
		sl.AddTail(str);
		for (ULONG i = 0, j = (extrasize + 15) & ~15; i < j; i += 16) {
			str.Format(L"%04x:", i);
			ULONG line_end = min(i + 16, extrasize);

			for (ULONG k = i; k < line_end; k++) {
				str.AppendFormat(L" %02x", pbFormat[fmtsize + k]);
			}

			for (ULONG k = line_end, l = i + 16; k < l; k++) {
				str += L"   ";
			}

			str += ' ';

			CString ch;
			for (size_t k = i; k < line_end; k++) {
				unsigned char c = (unsigned char)pbFormat[fmtsize + k];
				ch.AppendFormat(L"%C", c >= 0x20 ? c : '.');
			}
			str += ch;

			sl.AddTail(str);
		}
	}
}

bool CMediaTypeEx::ValidateSubtitle()
{
	return majortype == MEDIATYPE_Text
			|| majortype == MEDIATYPE_ScriptCommand
			|| majortype == MEDIATYPE_Subtitle
			|| subtype == MEDIASUBTYPE_DVD_SUBPICTURE
			|| subtype == MEDIASUBTYPE_CVD_SUBPICTURE
			|| subtype == MEDIASUBTYPE_SVCD_SUBPICTURE
			|| subtype == MEDIASUBTYPE_XSUB
			|| subtype == MEDIASUBTYPE_DVB_SUBTITLES
			|| subtype == MEDIASUBTYPE_HDMVSUB;
}
