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
#include "DSUtil.h"
#include "MediaTypeEx.h"
#include "AudioParser.h"

#include <MMReg.h>
#include <InitGuid.h>
#include <moreuuids.h>
#include <basestruct.h>
#include <d3d9types.h>
#include <dxva.h>
#include <dxva2api.h>
#include <map>
#include "MediaDescription.h"
#include "std_helper.h"

#include "GUIDString.h"

static const std::map<DWORD, LPCSTR> vfourcs = {
	{FCC('WMV1'), "Windows Media Video 7"},
	{FCC('WMV2'), "Windows Media Video 8"},
	{FCC('WMV3'), "Windows Media Video 9"},
	{FCC('DIV3'), "DivX 3"},
	{FCC('MP43'), "MSMPEG4v3"},
	{FCC('MP42'), "MSMPEG4v2"},
	{FCC('MP41'), "MSMPEG4v1"},
	{FCC('DX30'), "DivX 3"},
	{FCC('DX50'), "DivX 5"},
	{FCC('DIVX'), "DivX 6"},
	{FCC('XVID'), "Xvid"},
	{FCC('MP4V'), "MPEG4 Video"},
	{FCC('AVC1'), "H.264/AVC"},
	{FCC('H264'), "H.264/AVC"},
	{FCC('RV10'), "RealVideo 1"},
	{FCC('RV20'), "RealVideo 2"},
	{FCC('RV30'), "RealVideo 3"},
	{FCC('RV40'), "RealVideo 4"},
	{FCC('FLV1'), "Flash Video 1"},
	{FCC('FLV4'), "Flash Video 4"},
	{FCC('VP50'), "On2 VP5"},
	{FCC('VP60'), "On2 VP6"},
	{FCC('SVQ3'), "SVQ3"},
	{FCC('SVQ1'), "SVQ1"},
	{FCC('H263'), "H263"},
	{FCC('DRAC'), "Dirac"},
	{FCC('WVC1'), "VC-1"},
	{FCC('THEO'), "Theora"},
	{FCC('HVC1'), "HEVC"},
	{FCC('HM91'), "HEVC(HM9.1)"},
	{FCC('HM10'), "HEVC(HM10)"},
	{FCC('HM12'), "HEVC(HM12)"},
};

static const std::map<WORD, LPCSTR> aformattags = {
	// MMReg.h
	{WAVE_FORMAT_ADPCM,                 "MS ADPCM"},
	{WAVE_FORMAT_IEEE_FLOAT,            "IEEE Float"},
	{WAVE_FORMAT_ALAW,                  "aLaw"},
	{WAVE_FORMAT_MULAW,                 "muLaw"},
	{WAVE_FORMAT_DTS,                   "DTS"},
	{WAVE_FORMAT_DRM,                   "DRM"},
	{WAVE_FORMAT_WMAVOICE9,             "WMA Voice"},
	{WAVE_FORMAT_WMAVOICE10,            "WMA Voice"},
	{WAVE_FORMAT_OKI_ADPCM,             "OKI ADPCM"},
	{WAVE_FORMAT_IMA_ADPCM,             "IMA ADPCM"},
	{WAVE_FORMAT_MEDIASPACE_ADPCM,      "Mediaspace ADPCM"},
	{WAVE_FORMAT_SIERRA_ADPCM,          "Sierra ADPCM"},
	{WAVE_FORMAT_G723_ADPCM,            "G723 ADPCM"},
	{WAVE_FORMAT_DIALOGIC_OKI_ADPCM,    "Dialogic OKI ADPCM"},
	{WAVE_FORMAT_MEDIAVISION_ADPCM,     "Media Vision ADPCM"},
	{WAVE_FORMAT_YAMAHA_ADPCM,          "Yamaha ADPCM"},
	{WAVE_FORMAT_DSPGROUP_TRUESPEECH,   "DSP Group Truespeech"},
	{WAVE_FORMAT_DOLBY_AC2,             "Dolby AC2"},
	{WAVE_FORMAT_GSM610,                "GSM610"},
	{WAVE_FORMAT_MSNAUDIO,              "MSN Audio"},
	{WAVE_FORMAT_ANTEX_ADPCME,          "Antex ADPCME"},
	{WAVE_FORMAT_CS_IMAADPCM,           "Crystal Semiconductor IMA ADPCM"},
	{WAVE_FORMAT_ROCKWELL_ADPCM,        "Rockwell ADPCM"},
	{WAVE_FORMAT_ROCKWELL_DIGITALK,     "Rockwell Digitalk"},
	{WAVE_FORMAT_G721_ADPCM,            "G721"},
	{WAVE_FORMAT_G728_CELP,             "G728"},
	{WAVE_FORMAT_MSG723,                "MSG723"},
	{WAVE_FORMAT_MPEG,                  "MPEG Audio"},
	{WAVE_FORMAT_MPEGLAYER3,            "MP3"},
	{WAVE_FORMAT_LUCENT_G723,           "Lucent G723"},
	{WAVE_FORMAT_VOXWARE,               "Voxware"},
	{WAVE_FORMAT_G726_ADPCM,            "G726"},
	{WAVE_FORMAT_G722_ADPCM,            "G722"},
	{WAVE_FORMAT_G729A,                 "G729A"},
	{WAVE_FORMAT_MEDIASONIC_G723,       "MediaSonic G723"},
	{WAVE_FORMAT_ZYXEL_ADPCM,           "ZyXEL ADPCM"},
	{WAVE_FORMAT_RAW_AAC1,              "AAC"},
	{WAVE_FORMAT_RHETOREX_ADPCM,        "Rhetorex ADPCM"},
	{WAVE_FORMAT_VIVO_G723,             "Vivo G723"},
	{WAVE_FORMAT_VIVO_SIREN,            "Vivo Siren"},
	{WAVE_FORMAT_DIGITAL_G723,          "Digital G723"},
	{WAVE_FORMAT_SANYO_LD_ADPCM,        "Sanyo LD ADPCM"},
	{WAVE_FORMAT_MSAUDIO1,              "WMA 1"},
	{WAVE_FORMAT_WMAUDIO2,              "WMA 2"},
	{WAVE_FORMAT_WMAUDIO3,              "WMA Pro"},
	{WAVE_FORMAT_WMAUDIO_LOSSLESS,      "WMA Lossless"},
	{WAVE_FORMAT_CREATIVE_ADPCM,        "Creative ADPCM"},
	{WAVE_FORMAT_CREATIVE_FASTSPEECH8,  "Creative Fastspeech 8"},
	{WAVE_FORMAT_CREATIVE_FASTSPEECH10, "Creative Fastspeech 10"},
	{WAVE_FORMAT_UHER_ADPCM,            "UHER ADPCM"},
	{WAVE_FORMAT_DTS2,                  "DTS"},
	{WAVE_FORMAT_DOLBY_AC3_SPDIF,       "S/PDIF"},
	// other
	{WAVE_FORMAT_DOLBY_AC3,             "Dolby AC3"},
	{WAVE_FORMAT_LATM_AAC,              "AAC(LATM)"},
	{WAVE_FORMAT_FLAC,                  "FLAC"},
	{WAVE_FORMAT_TTA1,                  "TTA"},
	{WAVE_FORMAT_WAVPACK4,              "WavPack"},
	{WAVE_FORMAT_14_4,                  "RealAudio 14.4"},
	{WAVE_FORMAT_28_8,                  "RealAudio 28.8"},
	{WAVE_FORMAT_ATRC,                  "RealAudio ATRC"},
	{WAVE_FORMAT_COOK,                  "RealAudio COOK"},
	{WAVE_FORMAT_DNET,                  "RealAudio DNET"},
	{WAVE_FORMAT_RAAC,                  "RealAudio RAAC"},
	{WAVE_FORMAT_RACP,                  "RealAudio RACP"},
	{WAVE_FORMAT_SIPR,                  "RealAudio SIPR"},
	{WAVE_FORMAT_PS2_PCM,               "PS2 PCM"},
	{WAVE_FORMAT_PS2_ADPCM,             "PS2 ADPCM"},
	{WAVE_FORMAT_SPEEX,                 "Speex"},
	{WAVE_FORMAT_ADX_ADPCM,             "ADX ADPCM"},
};

static const std::map<GUID, LPCSTR> audioguids = {
	{MEDIASUBTYPE_PCM,             "PCM"},
	{MEDIASUBTYPE_IEEE_FLOAT,      "IEEE Float"},
	{MEDIASUBTYPE_DVD_LPCM_AUDIO,  "PCM"},
	{MEDIASUBTYPE_HDMV_LPCM_AUDIO, "LPCM"},
	{MEDIASUBTYPE_Vorbis,          "Vorbis (deprecated)"},
	{MEDIASUBTYPE_Vorbis2,         "Vorbis"},
	{MEDIASUBTYPE_MP4A,            "MPEG4 Audio"},
	{MEDIASUBTYPE_FLAC_FRAMED,     "FLAC (framed)"},
	{MEDIASUBTYPE_DOLBY_AC3,       "Dolby AC3"},
	{MEDIASUBTYPE_DOLBY_DDPLUS,    "DD+"},
	{MEDIASUBTYPE_DOLBY_TRUEHD,    "TrueHD"},
	{MEDIASUBTYPE_DTS,             "DTS"},
	{MEDIASUBTYPE_MLP,             "MLP"},
	{MEDIASUBTYPE_ALAC,            "ALAC"},
	{MEDIASUBTYPE_PCM_NONE,        "QT PCM"},
	{MEDIASUBTYPE_PCM_RAW,         "QT PCM"},
	{MEDIASUBTYPE_PCM_TWOS,        "PCM"},
	{MEDIASUBTYPE_PCM_SOWT,        "QT PCM"},
	{MEDIASUBTYPE_PCM_IN24,        "PCM"},
	{MEDIASUBTYPE_PCM_IN32,        "PCM"},
	{MEDIASUBTYPE_PCM_FL32,        "QT PCM"},
	{MEDIASUBTYPE_PCM_FL64,        "QT PCM"},
	{MEDIASUBTYPE_IMA4,            "ADPCM"},
	{MEDIASUBTYPE_ADPCM_SWF,       "ADPCM"},
	{MEDIASUBTYPE_IMA_AMV,         "ADPCM"},
	{MEDIASUBTYPE_ALS,             "ALS"},
	{MEDIASUBTYPE_QDMC,            "QDMC"},
	{MEDIASUBTYPE_QDM2,            "QDM2"},
	{MEDIASUBTYPE_RoQA,            "ROQA"},
	{MEDIASUBTYPE_APE,             "APE"},
	{MEDIASUBTYPE_AMR,             "AMR"},
	{MEDIASUBTYPE_SAMR,            "AMR"},
	{MEDIASUBTYPE_SAWB,            "AMR"},
	{MEDIASUBTYPE_OPUS,            "Opus"},
	{MEDIASUBTYPE_BINKA_DCT,       "BINK DCT"},
	{MEDIASUBTYPE_AAC_ADTS,        "AAC"},
	{MEDIASUBTYPE_DSD,             "DSD"},
	{MEDIASUBTYPE_DSD1,            "DSD"},
	{MEDIASUBTYPE_DSD8,            "DSD"},
	{MEDIASUBTYPE_DSDL,            "DSD"},
	{MEDIASUBTYPE_DSDM,            "DSD"},
	{MEDIASUBTYPE_DST,             "DST"},
	{MEDIASUBTYPE_NELLYMOSER,      "Nelly Moser"},
	{MEDIASUBTYPE_TAK,             "TAK"},
	{MEDIASUBTYPE_BINKA_DCT,       "BINK"},
	{MEDIASUBTYPE_BINKA_RDFT,      "BINK"},
};

static const std::map<GUID, LPCSTR> subtitleguids = {
	{MEDIASUBTYPE_UTF8,           "UTF-8"},
	{MEDIASUBTYPE_SSA,            "SubStation Alpha"},
	{MEDIASUBTYPE_ASS,            "Advanced SubStation Alpha"},
	{MEDIASUBTYPE_ASS2,           "Advanced SubStation Alpha"},
	{MEDIASUBTYPE_USF,            "Universal Subtitle Format"},
	{MEDIASUBTYPE_VOBSUB,         "VobSub"},
	{MEDIASUBTYPE_DVB_SUBTITLES,  "DVB"},
	{MEDIASUBTYPE_DVD_SUBPICTURE, "DVD Subtitles"},
	{MEDIASUBTYPE_HDMVSUB,        "PGS"},
};

#define ADDENTRY(mode) { mode, #mode },
static const std::map<GUID, LPCSTR> dxvaguids = {
	// GUID name from dxva.h
	ADDENTRY(DXVA_ModeNone)
	// GUID names from dxva2api.h
	ADDENTRY(DXVA2_ModeMPEG2_MoComp)
	ADDENTRY(DXVA2_ModeMPEG2_IDCT)
	ADDENTRY(DXVA2_ModeMPEG2_VLD)
	ADDENTRY(DXVA2_ModeMPEG1_VLD)
	ADDENTRY(DXVA2_ModeMPEG2and1_VLD)
	ADDENTRY(DXVA2_ModeH264_MoComp_NoFGT)
	ADDENTRY(DXVA2_ModeH264_MoComp_FGT)
	ADDENTRY(DXVA2_ModeH264_IDCT_NoFGT)
	ADDENTRY(DXVA2_ModeH264_IDCT_FGT)
	ADDENTRY(DXVA2_ModeH264_VLD_NoFGT)
	ADDENTRY(DXVA2_ModeH264_VLD_FGT)
	ADDENTRY(DXVA2_ModeH264_VLD_WithFMOASO_NoFGT)
	ADDENTRY(DXVA2_ModeH264_VLD_Stereo_Progressive_NoFGT)
	ADDENTRY(DXVA2_ModeH264_VLD_Stereo_NoFGT)
	ADDENTRY(DXVA2_ModeH264_VLD_Multiview_NoFGT)
	ADDENTRY(DXVA2_ModeWMV8_PostProc)
	ADDENTRY(DXVA2_ModeWMV8_MoComp)
	ADDENTRY(DXVA2_ModeWMV9_PostProc)
	ADDENTRY(DXVA2_ModeWMV9_MoComp)
	ADDENTRY(DXVA2_ModeWMV9_IDCT)
	ADDENTRY(DXVA2_ModeVC1_PostProc)
	ADDENTRY(DXVA2_ModeVC1_MoComp)
	ADDENTRY(DXVA2_ModeVC1_IDCT)
	ADDENTRY(DXVA2_ModeVC1_VLD)
	ADDENTRY(DXVA2_ModeVC1_D2010)
	ADDENTRY(DXVA2_NoEncrypt)
	ADDENTRY(DXVA2_ModeMPEG4pt2_VLD_Simple)
	ADDENTRY(DXVA2_ModeMPEG4pt2_VLD_AdvSimple_NoGMC)
	ADDENTRY(DXVA2_ModeMPEG4pt2_VLD_AdvSimple_GMC)
	ADDENTRY(DXVA2_ModeHEVC_VLD_Main)
	ADDENTRY(DXVA2_ModeHEVC_VLD_Main10)
	ADDENTRY(DXVA2_ModeVP9_VLD_Profile0)
	ADDENTRY(DXVA2_ModeVP9_VLD_10bit_Profile2)
	ADDENTRY(DXVA2_ModeVP8_VLD)
};
#undef ADDENTRY

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
				rate += FormatBitrate(vih->dwBitRate);
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
				rate += FormatBitrate(vih->dwBitRate);
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
				if (codec == L"DTS" && wfe->cbSize == 1) {
					const auto profile = ((BYTE *)(wfe + 1))[0];
					switch (profile) {
						case DCA_PROFILE_HD_HRA:
							codec = L"DTS-HD HRA";
							break;
						case DCA_PROFILE_HD_MA:
							codec = L"DTS-HD MA";
							break;
						case DCA_PROFILE_EXPRESS:
							codec = L"DTS Express";
							break;
					}
				}
				dim.Format(L"%uHz", wfe->nSamplesPerSec);
				if (wfe->nChannels) {
					DWORD layout = 0;
					if (IsWaveFormatExtensible(wfe)) {
						const WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;
						layout = wfex->dwChannelMask;
					}
					else {
						layout = GetDefChannelMask(wfe->nChannels);
					}
					BYTE lfe = 0;
					WORD nChannels = wfe->nChannels;
					if (layout & SPEAKER_LOW_FREQUENCY) {
						nChannels--;
						lfe = 1;
					}
					dim.AppendFormat(L" %u.%u chn", nChannels, lfe);
				}
				if (wfe->nAvgBytesPerSec && wfe->wFormatTag != WAVE_FORMAT_OPUS) {
					rate += FormatBitrate(wfe->nAvgBytesPerSec * 8);
				}
			}
		} else if (formattype == FORMAT_VorbisFormat) {
			VORBISFORMAT* vf = (VORBISFORMAT*)Format();

			codec = GetAudioCodecName(subtype, 0);
			dim.Format(L"%uHz", vf->nSamplesPerSec);
			if (vf->nChannels) {
				const DWORD layout = GetDefChannelMask(vf->nChannels);
				BYTE lfe = 0;
				WORD nChannels = vf->nChannels;
				if (layout & SPEAKER_LOW_FREQUENCY) {
					nChannels--;
					lfe = 1;
				}
				dim.AppendFormat(L" %u.%u chn", nChannels, lfe);
			}
			if (vf->nAvgBitsPerSec) {
				rate += FormatBitrate(vf->nAvgBitsPerSec * 8);
			}
		} else if (formattype == FORMAT_VorbisFormat2) {
			VORBISFORMAT2* vf = (VORBISFORMAT2*)Format();

			codec = GetAudioCodecName(subtype, 0);
			dim.Format(L"%uHz", vf->SamplesPerSec);
			if (vf->Channels) {
				const DWORD layout = GetDefChannelMask(vf->Channels);
				BYTE lfe = 0;
				WORD nChannels = vf->Channels;
				if (layout & SPEAKER_LOW_FREQUENCY) {
					nChannels--;
					lfe = 1;
				}
				dim.AppendFormat(L" %u.%u chn", nChannels, lfe);
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

	if (biCompression != BI_RGB && biCompression != BI_BITFIELDS) {
		DWORD fourcc = biCompression;
		BYTE* b = (BYTE*)&fourcc;

		for (size_t i = 0; i < 4; i++) {
			if (b[i] >= 'a' && b[i] <= 'z') {
				b[i] = b[i] + 32;
			}
		}

		auto it = vfourcs.find(fourcc);
		if (it != vfourcs.cend()) {
			str = (*it).second;
		} else {
			if (subtype == MEDIASUBTYPE_DiracVideo) {
				str = L"Dirac Video";
			} else if (subtype == MEDIASUBTYPE_MP4V ||
					   subtype == MEDIASUBTYPE_mp4v) {
				str = L"MPEG4 Video";
			} else if (subtype == MEDIASUBTYPE_apch ||
					   subtype == MEDIASUBTYPE_apcn ||
					   subtype == MEDIASUBTYPE_apcs ||
					   subtype == MEDIASUBTYPE_apco ||
					   subtype == MEDIASUBTYPE_ap4h ||
					   subtype == MEDIASUBTYPE_ap4x) {
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

	auto it1 = aformattags.find(wFormatTag);
	if (it1 != aformattags.cend()) {
		str = (*it1).second;
	} else {
		auto it2 = audioguids.find(subtype);
		if (it2 != audioguids.cend()) {
			str = (*it2).second;
		} else {
			str.Format(L"0x%04x", wFormatTag);
		}
	}

	return str;
}

CString CMediaTypeEx::GetSubtitleCodecName(const GUID& subtype)
{
	CString str;

	auto it = subtitleguids.find(subtype);
	if (it != subtitleguids.cend()) {
		str = (*it).second;
	}

	return str;
}

CString GetGUIDString(const GUID& guid)
{
	// to prevent print TIME_FORMAT_NONE for GUID_NULL
	if (guid == GUID_NULL) {
		return L"GUID_NULL";
	}

	const CHAR* guidStr = GuidNames[guid]; // GUID names from uuids.h

	if (strcmp(guidStr, "Unknown GUID Name") == 0) {
		guidStr = m_GuidNames[guid]; // GUID names from moreuuids.h
	}

	if (strcmp(guidStr, "Unknown GUID Name") == 0) {

		auto it = dxvaguids.find(guid);
		if (it != dxvaguids.cend()) {
			guidStr = (*it).second;
		}
		else if (memcmp(&guid.Data2, &MEDIASUBTYPE_YUY2.Data2, sizeof(GUID)- sizeof(GUID::Data1)) == 0) {
			// GUID like {xxxxxxxx-0000-0010-8000-00AA00389B71}

			switch (guid.Data1) {
			case WAVE_FORMAT_ADPCM:         guidStr = "MEDIASUBTYPE_MS_ADPCM"; break;
			case WAVE_FORMAT_GSM610:        guidStr = "MEDIASUBTYPE_GSM610"; break;
			case WAVE_FORMAT_MPEG_ADTS_AAC: guidStr = "MEDIASUBTYPE_MPEG_ADTS_AAC"; break;
			case WAVE_FORMAT_MPEG_RAW_AAC:  guidStr = "MEDIASUBTYPE_MPEG_RAW_AAC"; break;
			case WAVE_FORMAT_MPEG_LOAS:     guidStr = "MEDIASUBTYPE_MPEG_LOAS"; break;
			case WAVE_FORMAT_MPEG_HEAAC:    guidStr = "MEDIASUBTYPE_MPEG_HEAAC"; break;
			default:
				return L"MEDIASUBTYPE_" + FourccToWStr(guid.Data1);
			}
		}
	}

	return CString(guidStr);;
}

CString GetGUIDString2(const GUID& guid)
{
	CString ret = GetGUIDString(guid);
	if (ret == L"Unknown GUID Name") {
		ret.AppendFormat(L" %s", CStringFromGUID(guid));
	}

	return ret;
}

void CMediaTypeEx::Dump(std::list<CString>& sl)
{
	CString str;

	sl.clear();

	ULONG fmtsize = 0;

	CString major = CStringFromGUID(majortype);
	CString sub = CStringFromGUID(subtype);
	CString format = CStringFromGUID(formattype);

	sl.emplace_back(ToString());
	sl.emplace_back(L"");

	sl.emplace_back(L"AM_MEDIA_TYPE: ");
	str.Format(L"majortype: %s %s", GetGUIDString(majortype), major);
	sl.emplace_back(str);
	if (majortype == MEDIATYPE_Video && subtype == FOURCCMap(BI_RLE8)) { // fake subtype for RLE 8-bit
		str.Format(L"subtype: BI_RLE8 %s", sub);
	} else if (majortype == MEDIATYPE_Video && subtype == FOURCCMap(BI_RLE4)) { // fake subtype for RLE 4-bit
		str.Format(L"subtype: BI_RLE4 %s", sub);
	} else {
		str.Format(L"subtype: %s %s", GetGUIDString(subtype), sub);
	}
	sl.emplace_back(str);
	str.Format(L"formattype: %s %s", GetGUIDString(formattype), format);
	sl.emplace_back(str);
	str.Format(L"bFixedSizeSamples: %d", bFixedSizeSamples);
	sl.emplace_back(str);
	str.Format(L"bTemporalCompression: %d", bTemporalCompression);
	sl.emplace_back(str);
	str.Format(L"lSampleSize: %u", lSampleSize);
	sl.emplace_back(str);
	str.Format(L"cbFormat: %u", cbFormat);
	sl.emplace_back(str);

	sl.emplace_back(L"");

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

		sl.emplace_back(L"VIDEOINFOHEADER:");
		str.Format(L"rcSource: (%d,%d)-(%d,%d)", vih.rcSource.left, vih.rcSource.top, vih.rcSource.right, vih.rcSource.bottom);
		sl.emplace_back(str);
		str.Format(L"rcTarget: (%d,%d)-(%d,%d)", vih.rcTarget.left, vih.rcTarget.top, vih.rcTarget.right, vih.rcTarget.bottom);
		sl.emplace_back(str);
		str.Format(L"dwBitRate: %u", vih.dwBitRate);
		sl.emplace_back(str);
		str.Format(L"dwBitErrorRate: %u", vih.dwBitErrorRate);
		sl.emplace_back(str);
		str.Format(L"AvgTimePerFrame: %I64d", vih.AvgTimePerFrame);
		if (vih.AvgTimePerFrame > 0) {
			str.AppendFormat(L" (%.3f fps)", 10000000.0 / vih.AvgTimePerFrame);
		}
		sl.emplace_back(str);

		sl.emplace_back(L"");

		if (formattype == FORMAT_VideoInfo2 || formattype == FORMAT_MPEG2_VIDEO) {
			VIDEOINFOHEADER2& vih2 = *(VIDEOINFOHEADER2*)pbFormat;
			bih = &vih2.bmiHeader;

			sl.emplace_back(L"VIDEOINFOHEADER2:");
			str.Format(L"dwInterlaceFlags: 0x%08x", vih2.dwInterlaceFlags);
			sl.emplace_back(str);
			str.Format(L"dwCopyProtectFlags: 0x%08x", vih2.dwCopyProtectFlags);
			sl.emplace_back(str);
			str.Format(L"dwPictAspectRatioX: %u", vih2.dwPictAspectRatioX);
			sl.emplace_back(str);
			str.Format(L"dwPictAspectRatioY: %u", vih2.dwPictAspectRatioY);
			sl.emplace_back(str);
			str.Format(L"dwControlFlags: 0x%08x", vih2.dwControlFlags);
			sl.emplace_back(str);
			if (vih2.dwControlFlags & (AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT)) {
				// http://msdn.microsoft.com/en-us/library/windows/desktop/ms698715%28v=vs.85%29.aspx
				const LPCSTR nominalrange[] = { nullptr, "0-255", "16-235", "48-208" };
				const LPCSTR transfermatrix[] = { nullptr, "BT.709", "BT.601", "SMPTE 240M", "BT.2020", nullptr, nullptr, "YCgCo" };
				const LPCSTR lighting[] = { nullptr, "bright", "office", "dim", "dark" };
				const LPCSTR primaries[] = { nullptr, "Reserved", "BT.709", "BT.470-4 System M", "BT.470-4 System B,G",
					"SMPTE 170M", "SMPTE 240M", "EBU Tech. 3213", "SMPTE C", "BT.2020" };
				const LPCSTR transfunc[] = { nullptr, "Linear RGB", "1.8 gamma", "2.0 gamma", "2.2 gamma", "BT.709", "SMPTE 240M",
					"sRGB", "2.8 gamma", "Log100", "Log316", "Symmetric BT.709", "Constant luminance BT.2020", "Non-constant luminance BT.2020",
					"2.6 gamma", "SMPTE ST 2084 (PQ)", "ARIB STD-B67 (HLG)"};

#define ADD_PARAM_DESC(str, parameter, descs) if (parameter < _countof(descs) && descs[parameter]) str.AppendFormat(L" (%hS)", descs[parameter])

				DXVA2_ExtendedFormat exfmt;
				exfmt.value = vih2.dwControlFlags;

				str.Format(L"- VideoChromaSubsampling: %u", exfmt.VideoChromaSubsampling);
				sl.emplace_back(str);

				str.Format(L"- NominalRange          : %u", exfmt.NominalRange);
				ADD_PARAM_DESC(str, exfmt.NominalRange, nominalrange);
				sl.emplace_back(str);

				str.Format(L"- VideoTransferMatrix   : %u", exfmt.VideoTransferMatrix);
				ADD_PARAM_DESC(str, exfmt.VideoTransferMatrix, transfermatrix);
				sl.emplace_back(str);

				str.Format(L"- VideoLighting         : %u", exfmt.VideoLighting);
				ADD_PARAM_DESC(str, exfmt.VideoLighting, lighting);
				sl.emplace_back(str);

				str.Format(L"- VideoPrimaries        : %u", exfmt.VideoPrimaries);
				ADD_PARAM_DESC(str, exfmt.VideoPrimaries, primaries);
				sl.emplace_back(str);

				str.Format(L"- VideoTransferFunction : %u", exfmt.VideoTransferFunction);
				ADD_PARAM_DESC(str, exfmt.VideoTransferFunction, transfunc);
				sl.emplace_back(str);
			}
			str.Format(L"dwReserved2: 0x%08x", vih2.dwReserved2);
			sl.emplace_back(str);

			sl.emplace_back(L"");
		}

		if (formattype == FORMAT_MPEGVideo) {
			MPEG1VIDEOINFO& mvih = *(MPEG1VIDEOINFO*)pbFormat;

			sl.emplace_back(L"MPEG1VIDEOINFO:");
			str.Format(L"dwStartTimeCode: %u", mvih.dwStartTimeCode);
			sl.emplace_back(str);
			str.Format(L"cbSequenceHeader: %u", mvih.cbSequenceHeader);
			sl.emplace_back(str);

			sl.emplace_back(L"");
		} else if (formattype == FORMAT_MPEG2_VIDEO) {
			MPEG2VIDEOINFO& mvih = *(MPEG2VIDEOINFO*)pbFormat;

			sl.emplace_back(L"MPEG2VIDEOINFO:");
			str.Format(L"dwStartTimeCode: %d", mvih.dwStartTimeCode);
			sl.emplace_back(str);
			str.Format(L"cbSequenceHeader: %d", mvih.cbSequenceHeader);
			sl.emplace_back(str);
			str.Format(L"dwProfile: 0x%08x", mvih.dwProfile);
			sl.emplace_back(str);
			str.Format(L"dwLevel: 0x%08x", mvih.dwLevel);
			sl.emplace_back(str);
			str.Format(L"dwFlags: 0x%08x", mvih.dwFlags);
			sl.emplace_back(str);

			sl.emplace_back(L"");
		}

		sl.emplace_back(L"BITMAPINFOHEADER:");
		str.Format(L"biSize: %u", bih->biSize);
		sl.emplace_back(str);
		str.Format(L"biWidth: %d", bih->biWidth);
		sl.emplace_back(str);
		str.Format(L"biHeight: %d", bih->biHeight);
		sl.emplace_back(str);
		str.Format(L"biPlanes: %u", bih->biPlanes);
		sl.emplace_back(str);
		str.Format(L"biBitCount: %u", bih->biBitCount);
		sl.emplace_back(str);
		str.Format(L"biCompression: %s", FourccToWStr(bih->biCompression));
		sl.emplace_back(str);
		str.Format(L"biSizeImage: %d", bih->biSizeImage);
		sl.emplace_back(str);
		str.Format(L"biXPelsPerMeter: %d", bih->biXPelsPerMeter);
		sl.emplace_back(str);
		str.Format(L"biYPelsPerMeter: %d", bih->biYPelsPerMeter);
		sl.emplace_back(str);
		str.Format(L"biClrUsed: %u", bih->biClrUsed);
		sl.emplace_back(str);
		str.Format(L"biClrImportant: %u", bih->biClrImportant);
		sl.emplace_back(str);
	} else if (formattype == FORMAT_WaveFormatEx || formattype == FORMAT_WaveFormatExFFMPEG) {
		WAVEFORMATEX *pWfe = nullptr;
		if (formattype == FORMAT_WaveFormatExFFMPEG) {
			fmtsize = sizeof(WAVEFORMATEXFFMPEG);

			WAVEFORMATEXFFMPEG *wfeff = (WAVEFORMATEXFFMPEG*)pbFormat;
			pWfe = &wfeff->wfex;

			sl.emplace_back(L"WAVEFORMATEXFFMPEG:");
			str.Format(L"nCodecId: 0x%04x", wfeff->nCodecId);
			sl.emplace_back(str);
			sl.emplace_back(L"");
		} else {
			fmtsize = sizeof(WAVEFORMATEX);
			pWfe = (WAVEFORMATEX*)pbFormat;
		}

		WAVEFORMATEX& wfe = *pWfe;

		sl.emplace_back(L"WAVEFORMATEX:");
		str.Format(L"wFormatTag: 0x%04x", wfe.wFormatTag);
		sl.emplace_back(str);
		str.Format(L"nChannels: %u", wfe.nChannels);
		sl.emplace_back(str);
		str.Format(L"nSamplesPerSec: %u", wfe.nSamplesPerSec);
		sl.emplace_back(str);
		str.Format(L"nAvgBytesPerSec: %u", wfe.nAvgBytesPerSec);
		sl.emplace_back(str);
		str.Format(L"nBlockAlign: %u", wfe.nBlockAlign);
		sl.emplace_back(str);
		str.Format(L"wBitsPerSample: %u", wfe.wBitsPerSample);
		sl.emplace_back(str);
		str.Format(L"cbSize: %u (extra bytes)", wfe.cbSize);
		sl.emplace_back(str);

		if (wfe.wFormatTag != WAVE_FORMAT_PCM && wfe.cbSize > 0 && formattype == FORMAT_WaveFormatEx) {
			if (wfe.wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe.cbSize == sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)) {
				sl.emplace_back(L"");

				fmtsize = sizeof(WAVEFORMATEXTENSIBLE);

				WAVEFORMATEXTENSIBLE& wfex = *(WAVEFORMATEXTENSIBLE*)pbFormat;

				sl.emplace_back(L"WAVEFORMATEXTENSIBLE:");
				if (wfex.Format.wBitsPerSample != 0) {
					str.Format(L"wValidBitsPerSample: %u", wfex.Samples.wValidBitsPerSample);
				} else {
					str.Format(L"wSamplesPerBlock: %u", wfex.Samples.wSamplesPerBlock);
				}
				sl.emplace_back(str);
				str.Format(L"dwChannelMask: 0x%08x", wfex.dwChannelMask);
				sl.emplace_back(str);
				str.Format(L"SubFormat: %s", CStringFromGUID(wfex.SubFormat));
				sl.emplace_back(str);
			} else if (wfe.wFormatTag == WAVE_FORMAT_DOLBY_AC3 && wfe.cbSize == sizeof(DOLBYAC3WAVEFORMAT)-sizeof(WAVEFORMATEX)) {
				sl.emplace_back(L"");

				fmtsize = sizeof(DOLBYAC3WAVEFORMAT);

				DOLBYAC3WAVEFORMAT& ac3wf = *(DOLBYAC3WAVEFORMAT*)pbFormat;

				sl.emplace_back(L"DOLBYAC3WAVEFORMAT:");
				str.Format(L"bBigEndian: %u", ac3wf.bBigEndian);
				sl.emplace_back(str);
				str.Format(L"bsid: %u", ac3wf.bsid);
				sl.emplace_back(str);
				str.Format(L"lfeon: %u", ac3wf.lfeon);
				sl.emplace_back(str);
				str.Format(L"copyrightb: %u", ac3wf.copyrightb);
				sl.emplace_back(str);
				str.Format(L"nAuxBitsCode: %u", ac3wf.nAuxBitsCode);
				sl.emplace_back(str);
			}
		}
	} else if (formattype == FORMAT_VorbisFormat) {
		fmtsize = sizeof(VORBISFORMAT);

		VORBISFORMAT& vf = *(VORBISFORMAT*)pbFormat;

		sl.emplace_back(L"VORBISFORMAT:");
		str.Format(L"nChannels: %u", vf.nChannels);
		sl.emplace_back(str);
		str.Format(L"nSamplesPerSec: %u", vf.nSamplesPerSec);
		sl.emplace_back(str);
		str.Format(L"nMinBitsPerSec: %u", vf.nMinBitsPerSec);
		sl.emplace_back(str);
		str.Format(L"nAvgBitsPerSec: %u", vf.nAvgBitsPerSec);
		sl.emplace_back(str);
		str.Format(L"nMaxBitsPerSec: %u", vf.nMaxBitsPerSec);
		sl.emplace_back(str);
		str.Format(L"fQuality: %.3f", vf.fQuality);
		sl.emplace_back(str);
	} else if (formattype == FORMAT_VorbisFormat2) {
		fmtsize = sizeof(VORBISFORMAT2);

		VORBISFORMAT2& vf = *(VORBISFORMAT2*)pbFormat;

		sl.emplace_back(L"VORBISFORMAT:");
		str.Format(L"Channels: %u", vf.Channels);
		sl.emplace_back(str);
		str.Format(L"SamplesPerSec: %u", vf.SamplesPerSec);
		sl.emplace_back(str);
		str.Format(L"BitsPerSample: %u", vf.BitsPerSample);
		sl.emplace_back(str);
		str.Format(L"HeaderSize: {%u, %u, %u}", vf.HeaderSize[0], vf.HeaderSize[1], vf.HeaderSize[2]);
		sl.emplace_back(str);
	} else if (formattype == FORMAT_SubtitleInfo) {
		fmtsize = sizeof(SUBTITLEINFO);

		SUBTITLEINFO& si = *(SUBTITLEINFO*)pbFormat;

		sl.emplace_back(L"SUBTITLEINFO:");
		str.Format(L"dwOffset: %u", si.dwOffset);
		sl.emplace_back(str);
		str.Format(L"IsoLang: %s", CString(si.IsoLang, _countof(si.IsoLang) - 1));
		sl.emplace_back(str);
		str.Format(L"TrackName: %s", CString(si.TrackName, _countof(si.TrackName) - 1));
		sl.emplace_back(str);
	}

	if (fmtsize < cbFormat) { // extra and unknown data
		sl.emplace_back(L"");

		ULONG extrasize = cbFormat - fmtsize;
		str.Format(L"Extradata: %u", extrasize);
		sl.emplace_back(str);
		for (ULONG i = 0, j = (extrasize + 15) & ~15; i < j; i += 16) {
			str.Format(L"%04x:", i);
			ULONG line_end = std::min(i + 16, extrasize);

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

			sl.emplace_back(str);
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
