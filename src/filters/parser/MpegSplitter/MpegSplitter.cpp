/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include <MMReg.h>
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <dmodshow.h>
#include "MpegSplitter.h"
#include <moreuuids.h>
#include <basestruct.h>
#include <list>

#include "../../reader/VTSReader/VTSReader.h"
#include "../apps/mplayerc/SettingsDefines.h"

// option names
#define OPT_REGKEY_MPEGSplit  L"Software\\MPC-BE Filters\\MPEG Splitter"
#define OPT_SECTION_MPEGSplit L"Filters\\MPEG Splitter"
#define OPT_ForcedSub         L"ForcedSub"
#define OPT_AudioLangOrder    L"AudioLanguageOrder"
#define OPT_SubLangOrder      L"SubtitlesLanguageOrder"
#define OPT_AC3CoreOnly       L"AC3CoreOnly"
#define OPT_SubEmptyOutput    L"SubtitleEmptyOutput"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_PROGRAM},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_TRANSPORT},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_PVA},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL},
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMpegSplitterFilter), MpegSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMpegSourceFilter), MpegSourceName, MERIT_UNLIKELY, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMpegSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMpegSourceFilter>, NULL, &sudFilter[1]},
	{L"CMpegSplitterPropertyPage", &__uuidof(CMpegSplitterSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CMpegSplitterSettingsWnd> >},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	DeleteRegKey(L"Media Type\\Extensions\\", L".ts");

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG1System, L"0,16,FFFFFFFFF100010001800001FFFFFFFF,000001BA2100010001800001000001BB", NULL);
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_PROGRAM, L"0,5,FFFFFFFFC0,000001BA40", NULL);
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_PVA, L"0,8,fffffc00ffe00000,4156000055000000", NULL);

	CAtlList<CString> chkbytes;
	chkbytes.AddTail(L"0,1,,47,188,1,,47,376,1,,47");
	chkbytes.AddTail(L"4,1,,47,196,1,,47,388,1,,47");
	chkbytes.AddTail(L"0,4,,54467263,1660,1,,47"); // TFrc

	chkbytes.AddTail(L"0,8,,4D504C5330323030"); // MPLS0200
	chkbytes.AddTail(L"0,8,,4D504C5330313030"); // MPLS0100
	chkbytes.AddTail(L"0,4,,494D4B48");			// IMKH

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_TRANSPORT, chkbytes, NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

static CString GetMediaTypeDesc(const CMediaType *pMediaType, const CHdmvClipInfo::Stream *pClipInfo, PES_STREAM_TYPE pesStreamType, CStringA lang)
{
	const WCHAR *pPresentationDesc = NULL;

	if (pClipInfo) {
		pPresentationDesc = StreamTypeToName(pClipInfo->m_Type);
	} else {
		pPresentationDesc = StreamTypeToName(pesStreamType);
	}

	CString MajorType;
	CAtlList<CString> Infos;

	if (pMediaType->majortype == MEDIATYPE_Video) {
		MajorType = L"Video";
		if (pMediaType->subtype == MEDIASUBTYPE_DVD_SUBPICTURE
			|| pMediaType->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE
			|| pMediaType->subtype == MEDIASUBTYPE_CVD_SUBPICTURE) {
			MajorType = L"Subtitle";
		}

		if (pClipInfo) {
			CString name = ISO6392ToLanguage(pClipInfo->m_LanguageCode);
			if (!name.IsEmpty()) {
				Infos.AddTail(name);
			}
		} else {
			if (!lang.IsEmpty()) {
				CString name = ISO6392ToLanguage(lang);
				if (!name.IsEmpty()) {
					Infos.AddTail(name);
				}
			}
		}

		const VIDEOINFOHEADER *pVideoInfo = NULL;
		const VIDEOINFOHEADER2 *pVideoInfo2 = NULL;

		if (pMediaType->formattype == FORMAT_VideoInfo) {
			pVideoInfo = GetFormatHelper(pVideoInfo, pMediaType);

			if (pVideoInfo->bmiHeader.biCompression == FCC('drac')) {
				Infos.AddTail(L"DIRAC");
			}

		} else if (pMediaType->formattype == FORMAT_MPEGVideo) {
			Infos.AddTail(L"MPEG");

			const MPEG1VIDEOINFO *pInfo = GetFormatHelper(pInfo, pMediaType);

			pVideoInfo = &pInfo->hdr;

		} else if (pMediaType->formattype == FORMAT_MPEG2_VIDEO) {
			const MPEG2VIDEOINFO *pInfo = GetFormatHelper(pInfo, pMediaType);

			pVideoInfo2 = &pInfo->hdr;

			bool bIsAVC		= false;
			bool bIsHEVC	= false;
			bool bIsMPEG2	= false;

			if (pInfo->hdr.bmiHeader.biCompression == FCC('AVC1') || pInfo->hdr.bmiHeader.biCompression == FCC('H264')) {
				bIsAVC = true;
				Infos.AddTail(L"AVC (H.264)");
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('AMVC')) {
				bIsAVC = true;
				Infos.AddTail(L"MVC (Full)");
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('EMVC')) {
				bIsAVC = true;
				Infos.AddTail(L"MVC (Subset)");
			} else if (pInfo->hdr.bmiHeader.biCompression == 0) {
				Infos.AddTail(L"MPEG2");
				bIsMPEG2 = true;
			} else if (pInfo->hdr.bmiHeader.biCompression == FCC('HEVC') || pInfo->hdr.bmiHeader.biCompression == FCC('HVC1')) {
				Infos.AddTail(L"HEVC (H.265)");
				bIsHEVC = true;
			} else {
				WCHAR Temp[5];
				memset(Temp, 0, sizeof(Temp));
				Temp[0] = (pInfo->hdr.bmiHeader.biCompression >> 0) & 0xFF;
				Temp[1] = (pInfo->hdr.bmiHeader.biCompression >> 8) & 0xFF;
				Temp[2] = (pInfo->hdr.bmiHeader.biCompression >> 16) & 0xFF;
				Temp[3] = (pInfo->hdr.bmiHeader.biCompression >> 24) & 0xFF;
				Infos.AddTail(Temp);
			}

			if (bIsMPEG2) {
				Infos.AddTail(MPEG2_Profile[pInfo->dwProfile]);
			} else if (pInfo->dwProfile) {
				if (bIsAVC) {
					switch (pInfo->dwProfile) {
						case 44:
							Infos.AddTail(L"CAVLC Profile");
							break;
						case 66:
							Infos.AddTail(L"Baseline Profile");
							break;
						case 77:
							Infos.AddTail(L"Main Profile");
							break;
						case 88:
							Infos.AddTail(L"Extended Profile");
							break;
						case 100:
							Infos.AddTail(L"High Profile");
							break;
						case 110:
							Infos.AddTail(L"High 10 Profile");
							break;
						case 118:
							Infos.AddTail(L"Multiview High Profile");
							break;
						case 122:
							Infos.AddTail(L"High 4:2:2 Profile");
							break;
						case 244:
							Infos.AddTail(L"High 4:4:4 Profile");
							break;
						case 128:
							Infos.AddTail(L"Stereo High Profile");
							break;
						default:
							Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile));
							break;
					}
				} else if (bIsHEVC) {
					switch (pInfo->dwProfile) {
						case 1:
							Infos.AddTail(L"Main Profile");
							break;
						case 2:
							Infos.AddTail(L"Main/10 Profile");
							break;
						default:
							Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile));
							break;
					}
				} else {
					Infos.AddTail(FormatString(L"Profile %d", pInfo->dwProfile));
				}
			}

			if (bIsMPEG2) {
				Infos.AddTail(MPEG2_Level[pInfo->dwLevel]);
			} else if (pInfo->dwLevel) {
				if (bIsAVC) {
					Infos.AddTail(FormatString(L"Level %1.1f", double(pInfo->dwLevel) / 10.0));
				} else if (bIsHEVC) {
					Infos.AddTail(FormatString(L"Level %d.%d", pInfo->dwLevel / 30, (pInfo->dwLevel % 30) / 3));
				} else {
					Infos.AddTail(FormatString(L"Level %d", pInfo->dwLevel));
				}
			}
		} else if (pMediaType->formattype == FORMAT_VIDEOINFO2) {
			const VIDEOINFOHEADER2 *pInfo = GetFormatHelper(pInfo, pMediaType);
			pVideoInfo2 = pInfo;

			DWORD CodecType = pInfo->bmiHeader.biCompression;
			if (CodecType == FCC('WVC1')) {
				Infos.AddTail(L"VC-1");
			} else if (CodecType) {
				WCHAR Temp[5];
				memset(Temp, 0, sizeof(Temp));
				Temp[0] = (CodecType >> 0) & 0xFF;
				Temp[1] = (CodecType >> 8) & 0xFF;
				Temp[2] = (CodecType >> 16) & 0xFF;
				Temp[3] = (CodecType >> 24) & 0xFF;
				Infos.AddTail(Temp);
			}
		} else if (pMediaType->subtype == MEDIASUBTYPE_DVD_SUBPICTURE) {
			Infos.AddTail(L"DVD Sub Picture");
		} else if (pMediaType->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE) {
			Infos.AddTail(L"SVCD Sub Picture");
		} else if (pMediaType->subtype == MEDIASUBTYPE_CVD_SUBPICTURE) {
			Infos.AddTail(L"CVD Sub Picture");
		}

		if (pVideoInfo2) {
			if (pVideoInfo2->bmiHeader.biWidth && pVideoInfo2->bmiHeader.biHeight) {
				Infos.AddTail(FormatString(L"%dx%d", pVideoInfo2->bmiHeader.biWidth, pVideoInfo2->bmiHeader.biHeight));
			}
			if (pVideoInfo2->AvgTimePerFrame) {
				Infos.AddTail(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo2->AvgTimePerFrame)));
			}
			if (pVideoInfo2->dwBitRate) {
				Infos.AddTail(FormatBitrate(pVideoInfo2->dwBitRate));
			}
		} else if (pVideoInfo) {
			if (pVideoInfo->bmiHeader.biWidth && pVideoInfo->bmiHeader.biHeight) {
				Infos.AddTail(FormatString(L"%dx%d", pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight));
			}
			if (pVideoInfo->AvgTimePerFrame) {
				Infos.AddTail(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo->AvgTimePerFrame)));
			}
			if (pVideoInfo->dwBitRate) {
				Infos.AddTail(FormatBitrate(pVideoInfo->dwBitRate));
			}
		}

	} else if (pMediaType->majortype == MEDIATYPE_Audio) {
		MajorType = L"Audio";
		if (pClipInfo) {
			CString name = ISO6392ToLanguage(pClipInfo->m_LanguageCode);
			if (!name.IsEmpty()) {
				Infos.AddTail(name);
			}
		} else {
			if (!lang.IsEmpty()) {
				CString name = ISO6392ToLanguage(lang);
				if (!name.IsEmpty()) {
					Infos.AddTail(name);
				}
			}
		}
		if (pMediaType->formattype == FORMAT_WaveFormatEx) {
			const WAVEFORMATEX *pInfo = GetFormatHelper(pInfo, pMediaType);

			if (pMediaType->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
				Infos.AddTail(L"DVD LPCM");
			} else if (pMediaType->subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
				const WAVEFORMATEX_HDMV_LPCM *pInfoHDMV = GetFormatHelper(pInfoHDMV, pMediaType);
				UNREFERENCED_PARAMETER(pInfoHDMV);
				Infos.AddTail(L"HDMV LPCM");
			}
			if (pMediaType->subtype == MEDIASUBTYPE_DOLBY_DDPLUS) {
				Infos.AddTail(L"Dolby Digital Plus");
			} else if (pMediaType->subtype == MEDIASUBTYPE_DOLBY_TRUEHD) {
				Infos.AddTail(L"Dolby TrueHD");
			} else if (pMediaType->subtype == MEDIASUBTYPE_MLP) {
				Infos.AddTail(L"MLP");
			} else {
				switch (pInfo->wFormatTag) {
					case WAVE_FORMAT_PS2_PCM: {
						Infos.AddTail(L"PS2 PCM");
					}
					break;
					case WAVE_FORMAT_PS2_ADPCM: {
						Infos.AddTail(L"PS2 ADPCM");
					}
					break;
					case WAVE_FORMAT_ADX_ADPCM: {
						Infos.AddTail(L"ADX ADPCM");
					}
					break;
					case WAVE_FORMAT_DTS2: {
						if (pPresentationDesc) {
							Infos.AddTail(pPresentationDesc);
						} else {
							Infos.AddTail(L"DTS");
						}
					}
					break;
					case WAVE_FORMAT_DOLBY_AC3: {
						Infos.AddTail(L"Dolby Digital");
					}
					break;
					case WAVE_FORMAT_RAW_AAC1: {
						Infos.AddTail(L"AAC");
					}
					break;
					case WAVE_FORMAT_LATM_AAC: {
						Infos.AddTail(L"AAC (LATM)");
					}
					break;
					case WAVE_FORMAT_MPEGLAYER3: {
						Infos.AddTail(L"MP3");
					}
					break;
					case WAVE_FORMAT_MPEG: {
						const MPEG1WAVEFORMAT* pInfoMPEG1 = GetFormatHelper(pInfoMPEG1, pMediaType);

						int layer = GetHighestBitSet32(pInfoMPEG1->fwHeadLayer) + 1;
						Infos.AddTail(FormatString(L"MPEG1 - Layer %d", layer));
					}
					break;
					case WAVE_FORMAT_ALAW:
						Infos.AddTail(L"A-law PCM");
						break;
					case WAVE_FORMAT_OPUS:
						Infos.AddTail(L"Opus");
						break;
				}
			}

			if (pClipInfo && (pClipInfo->m_SampleRate == BDVM_SampleRate_48_192 || pClipInfo->m_SampleRate == BDVM_SampleRate_48_96)) {
				switch (pClipInfo->m_SampleRate) {
					case BDVM_SampleRate_48_192:
						Infos.AddTail(FormatString(L"192(48) kHz"));
						break;
					case BDVM_SampleRate_48_96:
						Infos.AddTail(FormatString(L"96(48) kHz"));
						break;
				}
			} else if (pInfo->nSamplesPerSec) {
				Infos.AddTail(FormatString(L"%.1f kHz", double(pInfo->nSamplesPerSec)/1000.0));
			}
			if (pInfo->nChannels) {
				Infos.AddTail(FormatString(L"%d chn", pInfo->nChannels));
			}
			if (pInfo->wBitsPerSample) {
				Infos.AddTail(FormatString(L"%d bit", pInfo->wBitsPerSample));
			}
			if (pInfo->nAvgBytesPerSec) {
				if (pesStreamType != AUDIO_STREAM_DTS_HD_MASTER_AUDIO) {
					Infos.AddTail(FormatBitrate(pInfo->nAvgBytesPerSec * 8));
				}
			}

		}
	} else if (pMediaType->majortype == MEDIATYPE_Subtitle) {
		MajorType = L"Subtitle";

		if (pPresentationDesc == NULL) {
			if (pMediaType->subtype == MEDIASUBTYPE_DVB_SUBTITLES) {
				pPresentationDesc = L"DVB";
			} else if (pMediaType->subtype == MEDIASUBTYPE_UTF8) {
				pPresentationDesc = L"Teletext";
			}
		}

		if (pPresentationDesc) {
			Infos.AddTail(pPresentationDesc);
		}

		if (pClipInfo) {
			CString name = ISO6392ToLanguage(pClipInfo->m_LanguageCode);
			if (!name.IsEmpty()) {
				Infos.AddHead(name);
			} else if (!lang.IsEmpty()) {
				CString name = ISO6392ToLanguage(lang);
				if (!name.IsEmpty()) {
					Infos.AddHead(name);
				}
			}
		} else if (pMediaType->cbFormat == sizeof(SUBTITLEINFO)) {
			const SUBTITLEINFO *pInfo = GetFormatHelper(pInfo, pMediaType);
			CString name = ISO6392ToLanguage(pInfo->IsoLang);

			if (!lang.IsEmpty()) {
				CString name = ISO6392ToLanguage(lang);
				if (!name.IsEmpty()) {
					Infos.AddHead(name);
				}
			} else if (!name.IsEmpty()) {
				Infos.AddHead(name);
			}
			if (pInfo->TrackName[0]) {
				Infos.AddTail(pInfo->TrackName);
			}
		} else if (!lang.IsEmpty()) {
			CString name = ISO6392ToLanguage(lang);
			if (!name.IsEmpty()) {
				Infos.AddHead(name);
			}
		}
	}

	if (!Infos.IsEmpty()) {
		CString Ret;

		Ret += MajorType;
		Ret += L" - ";

		bool bFirst = true;

		for (POSITION pos = Infos.GetHeadPosition(); pos; Infos.GetNext(pos)) {
			CString& String = Infos.GetAt(pos);

			if (bFirst) {
				Ret += String;
			} else {
				Ret += L", " + String;
			}

			bFirst = false;
		}

		return Ret;
	}
	return L"";
}

//
// CMpegSplitterFilter
//

CMpegSplitterFilter::CMpegSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseSplitterFilter(NAME("CMpegSplitterFilter"), pUnk, phr, clsid)
	, m_pPipoBimbo(false)
	, m_rtPlaylistDuration(0)
	, m_rtStartOffset(0)
	, m_rtSeekOffset(INVALID_TIME)
	, m_rtMin(0)
	, m_rtMax(0)
	, m_rtGlobalPCRTimeStamp(INVALID_TIME)
	, m_length(0)
	, m_ForcedSub(false)
	, m_AC3CoreOnly(0)
	, m_SubEmptyPin(false)
	, m_hasHdmvDvbSubPin(false)
	, m_bIsBD(false)
{
	m_nFlag |= SOURCE_SUPPORT_URL;
	m_nFlag |= PACKET_PTS_DISCONTINUITY;

#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_MPEGSplit, KEY_READ)) {
		TCHAR buff[256] = { 0 };
		ULONG len = _countof(buff);
		if (ERROR_SUCCESS == key.QueryStringValue(OPT_AudioLangOrder, buff, &len)) {
			m_AudioLanguageOrder = CString(buff);
		}

		len = _countof(buff);
		memset(buff, 0, sizeof(buff));
		if (ERROR_SUCCESS == key.QueryStringValue(OPT_SubLangOrder, buff, &len)) {
			m_SubtitlesLanguageOrder = CString(buff);
		}

		DWORD dw;
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_ForcedSub, dw)) {
			m_ForcedSub = !!dw;
		}

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_AC3CoreOnly, dw)) {
			m_AC3CoreOnly = dw;
		}

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_SubEmptyOutput, dw)) {
			m_SubEmptyPin = !!dw;
		}
	}
#else
	m_ForcedSub   = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MPEGSplit, OPT_ForcedSub, m_ForcedSub);
	m_AC3CoreOnly = AfxGetApp()->GetProfileInt(OPT_SECTION_MPEGSplit, OPT_AC3CoreOnly, m_AC3CoreOnly);
	m_SubEmptyPin = !!AfxGetApp()->GetProfileInt(OPT_SECTION_MPEGSplit, OPT_SubEmptyOutput, m_SubEmptyPin);
#endif
}

void CMpegSplitterFilter::GetMediaTypes(CMpegSplitterFile::stream_type sType, CAtlArray<CMediaType>& mts)
{
	for (int type = CMpegSplitterFile::stream_type::video; type <= CMpegSplitterFile::stream_type::subpic; type++) {
		if (type == sType) {
			POSITION pos = m_pFile->m_streams[type].GetHeadPosition();
			while (pos) {
				CMpegSplitterFile::stream& s = m_pFile->m_streams[type].GetNext(pos);
				for(size_t i = 0; i < s.mts.size(); i++) {
					mts.Add(s.mts[i]);
				}
			}

			return;
		}
	}
}

bool CMpegSplitterFilter::IsHdmvDvbSubPinDrying()
{
	if (m_hasHdmvDvbSubPin) {
		POSITION pos = m_pActivePins.GetHeadPosition();
		while (pos) {
			CBaseSplitterOutputPin* pPin = m_pActivePins.GetNext(pos);
			if (((CMpegSplitterOutputPin*)pPin)->NeedNextSubtitle()) {
				return true;
			}
		}
	}

	return false;
}

STDMETHODIMP CMpegSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IMpegSplitterFilter)
		QI(ISpecifyPropertyPages)
		QI(ISpecifyPropertyPages2)
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CMpegSplitterFilter::GetClassID(CLSID* pClsID)
{
	CheckPointer (pClsID, E_POINTER);

	if (m_pPipoBimbo) {
		memcpy(pClsID, &CLSID_WMAsfReader, sizeof(GUID));
		return S_OK;
	} else {
		return __super::GetClassID(pClsID);
	}
}

STDMETHODIMP CMpegSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, MpegSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

void CMpegSplitterFilter::ReadClipInfo(LPCOLESTR pszFileName)
{
	if (wcslen(pszFileName) > 0) {
		WCHAR Drive[_MAX_DRIVE] = { 0 };
		WCHAR Dir[_MAX_PATH] = { 0 };
		WCHAR Filename[_MAX_PATH] = { 0 };
		WCHAR Ext[_MAX_EXT] = { 0 };

		if (_wsplitpath_s(pszFileName, Drive, _countof(Drive), Dir, _countof(Dir), Filename, _countof(Filename), Ext, _countof(Ext)) == 0) {
			CString	strClipInfo;

			_wcslwr_s(Ext, _countof(Ext));

			if (wcscmp(Ext, L".ssif") == 0) {
				if (Drive[0]) {
					strClipInfo.Format(L"%s\\%s\\..\\..\\CLIPINF\\%s.clpi", Drive, Dir, Filename);
				} else {
					strClipInfo.Format(L"%s\\..\\..\\CLIPINF\\%s.clpi", Dir, Filename);
				}
			} else {
				if (Drive[0]) {
					strClipInfo.Format(L"%s\\%s\\..\\CLIPINF\\%s.clpi", Drive, Dir, Filename);
				} else {
					strClipInfo.Format(L"%s\\..\\CLIPINF\\%s.clpi", Dir, Filename);
				}
			}

			m_ClipInfo.ReadInfo(strClipInfo);
		}
	}
}

STDMETHODIMP CMpegSplitterFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CPath path(pszFileName);
	const CString ext = path.GetExtension().MakeLower();

	if (ext == L".iso" || ext == L".mdf") { // ignore the disk images without signature
		return E_ABORT;
	}

	return __super::Load(pszFileName, pmt);
}

HRESULT CMpegSplitterFilter::DeliverPacket(CAutoPtr<CPacket> p)
{
	const DWORD TrackNumber = p->TrackNumber;

	if (m_bUseMVCExtension) {
		if (TrackNumber == m_dwMVCExtensionTrackNumber) {
			m_MVCExtensionQueue.AddTail(p);
		} else if (TrackNumber == m_dwMasterH264TrackNumber) {
			m_MVCBaseQueue.AddTail(p);
			if (m_MVCExtensionQueue.IsEmpty()) {
				return S_OK;
			}

			POSITION pos = m_MVCBaseQueue.GetHeadPosition();
			while (pos) {
				CAutoPtr<CPacket>& pMVCBasePacket = m_MVCBaseQueue.GetAt(pos);

				POSITION pos_remove = NULL;
				while (!m_MVCExtensionQueue.IsEmpty()) {
					CAutoPtr<CPacket>& pMVCExtensionPacket = m_MVCExtensionQueue.GetHead();
					if (pMVCExtensionPacket->rtStart == pMVCBasePacket->rtStart) {
						pMVCBasePacket->Append(*pMVCExtensionPacket);
						m_MVCExtensionQueue.RemoveHeadNoReturn();

						__super::DeliverPacket(pMVCBasePacket);
						pos_remove = pos;
						m_MVCBaseQueue.GetNext(pos);
						m_MVCBaseQueue.RemoveAt(pos_remove);
						break;
					} else if (pMVCExtensionPacket->rtStart < pMVCBasePacket->rtStart) {
						DLog(L"CMpegSplitterFilter::DeliverPacket() : Dropping MVC extension %I64d, base is %I64d", pMVCExtensionPacket->rtStart, pMVCBasePacket->rtStart);
						m_MVCExtensionQueue.RemoveHeadNoReturn();
					} else {
						DLog(L"CMpegSplitterFilter::DeliverPacket() : Dropping base %I64d, next MVC extension is %I64d", pMVCBasePacket->rtStart, pMVCExtensionPacket->rtStart);
						pos_remove = pos;
						m_MVCBaseQueue.GetNext(pos);
						m_MVCBaseQueue.RemoveAt(pos_remove);
						break;
					}
				}

				if (!pos_remove) {
					m_MVCBaseQueue.GetNext(pos);
				}
			}
		} else {
			return __super::DeliverPacket(p);
		}

		return S_OK;
	}

	return __super::DeliverPacket(p);
}

template<typename T>
inline HRESULT CMpegSplitterFilter::HandleMPEGPacket(DWORD TrackNumber, __int64 nBytes, T& h, REFERENCE_TIME rtStartOffset, BOOL bStreamUsePTS)
{
	HRESULT hr = S_OK;

	if (nBytes > 0) {
		if (bStreamUsePTS) {
			const BOOL bPacketStart = h.fpts;

			CAutoPtr<CPacket>& p = pPackets[TrackNumber];
			if (bPacketStart && p) {
				if (p->bSyncPoint) {
					if (m_rtSeekOffset != INVALID_TIME && p->rtStart < m_rtSeekOffset) {
						DLog(L"CMpegSplitterFilter::HandleMPEGPacket() : [%u] Dropping packet %I64d, seek offset is %I64d", p->TrackNumber, p->rtStart, m_rtSeekOffset);
					} else {
						hr = DeliverPacket(p);
					}
				}
				p.Free();
			}

			if (!p) {
				if (!bPacketStart) {
					DLog(L"CMpegSplitterFilter::HandleMPEGPacket() : [%u] Dropping incomplete packet, size %I64d", TrackNumber, nBytes);
					return S_OK;
				}

				p.Attach(DNew CPacket());
				p->TrackNumber	= TrackNumber;
				p->bSyncPoint	= bPacketStart;
				p->rtStart		= h.fpts ? (h.pts - rtStartOffset) : INVALID_TIME;
				p->rtStop		= (p->rtStart == INVALID_TIME) ? INVALID_TIME : p->rtStart + 1;
			}

			size_t oldSize = p->GetCount();
			size_t newSize = p->GetCount() + nBytes;
			p->SetCount(newSize, max(1024, newSize));
			hr = m_pFile->ByteRead(p->GetData() + oldSize, nBytes);
		} else {
			REFERENCE_TIME rtStart = INVALID_TIME;
			if (h.fpts) {
				rtStart = h.pts - rtStartOffset;
			} else if (m_rtGlobalPCRTimeStamp != INVALID_TIME) {
				rtStart = m_rtGlobalPCRTimeStamp - rtStartOffset;
			}

			CAutoPtr<CPacket> p(DNew CPacket());
			p->TrackNumber	= TrackNumber;
			p->rtStart		= rtStart;
			p->rtStop		= (p->rtStart == INVALID_TIME) ? INVALID_TIME : p->rtStart + 1;
			p->bSyncPoint	= p->rtStart != INVALID_TIME;
			p->SetCount(nBytes);
			m_pFile->ByteRead(p->GetData(), nBytes);
			hr = DeliverPacket(p);
		}
	}

	return hr;
}

HRESULT CMpegSplitterFilter::DemuxNextPacket(REFERENCE_TIME rtStartOffset)
{
	if (!m_pFile->IsStreaming() && !m_pFile->GetRemaining()) {
		return E_FAIL;
	}

	HRESULT hr = S_OK;
	BYTE b;

	if (m_pFile->m_type == MPEG_TYPES::mpeg_ps) {
		if (!m_pFile->NextMpegStartCode(b)) {
			return S_FALSE;
		}

		if (b == 0xba) { // program stream header
			CMpegSplitterFile::pshdr h;
			if (!m_pFile->ReadPS(h)) {
				return S_FALSE;
			}
		} else if (b == 0xbb) { // program stream system header
			CMpegSplitterFile::pssyshdr h;
			if (!m_pFile->ReadPSS(h)) {
				return S_FALSE;
			}
		} else if ((b >= 0xbd && b < 0xf0) || (b == 0xfd)) { // pes packet
			CMpegSplitterFile::peshdr peshdr;
			if (!m_pFile->ReadPES(peshdr, b) || !peshdr.len) {
				return S_FALSE;
			}

			if (peshdr.type == CMpegSplitterFile::mpeg2 && peshdr.scrambling) {
				m_pFile->Seek(m_pFile->GetPos() + peshdr.len);
				return S_FALSE;
			}

			__int64 pos = m_pFile->GetPos();

			DWORD TrackNumber = m_pFile->AddStream(0, b, peshdr.id_ext, peshdr.len);
			if (GetOutputPin(TrackNumber)) {
				const __int64 nBytes = peshdr.len - (m_pFile->GetPos() - pos);
				hr = HandleMPEGPacket(TrackNumber, nBytes, peshdr, rtStartOffset, m_pFile->m_streamData[TrackNumber].usePTS);
			}
			m_pFile->Seek(pos + peshdr.len);
		}
	} else if (m_pFile->m_type == MPEG_TYPES::mpeg_ts) {
		CMpegSplitterFile::trhdr h;
		if (!m_pFile->ReadTR(h)) {
			return S_FALSE;
		}

		if (h.fPCR) {
			m_rtGlobalPCRTimeStamp = h.PCR;
		}

		__int64 pos = m_pFile->GetPos();

		if (h.payload && ISVALIDPID(h.pid)) {
			DWORD TrackNumber = h.pid;
			if (GetOutputPin(TrackNumber) || TrackNumber == m_dwMVCExtensionTrackNumber) {
				CMpegSplitterFile::peshdr peshdr;
				if (h.payloadstart) {
					if (m_pFile->NextMpegStartCode(b, 4)) { // pes packet
						if (m_pFile->ReadPES(peshdr, b)) {
							if (peshdr.type == CMpegSplitterFile::mpeg2 && peshdr.scrambling) {
								m_pFile->Seek(h.next);
								return S_OK;
							}
							TrackNumber = m_pFile->AddStream(h.pid, b, 0, (DWORD)(h.bytes - (m_pFile->GetPos() - pos)));
						}
					} else {
						m_pFile->Seek(pos);
					}
				}

				if (h.bytes > (m_pFile->GetPos() - pos)) {
					const __int64 nBytes = h.bytes - (m_pFile->GetPos() - pos);
					hr = HandleMPEGPacket(TrackNumber, nBytes, peshdr, rtStartOffset, m_pFile->m_streamData[TrackNumber].usePTS);
				}
			}
		}

		m_pFile->Seek(h.next);
	} else if (m_pFile->m_type == MPEG_TYPES::mpeg_pva) {
		CMpegSplitterFile::pvahdr pvahdr;
		if (!m_pFile->ReadPVA(pvahdr)) {
			return S_FALSE;
		}

		__int64 pos = m_pFile->GetPos();

		DWORD TrackNumber = pvahdr.streamid;
		if (GetOutputPin(TrackNumber)) {
			hr = HandleMPEGPacket(TrackNumber, pvahdr.length, pvahdr, rtStartOffset, m_pFile->m_streamData[TrackNumber].usePTS);
		}

		m_pFile->Seek(pos + pvahdr.length);
	}

	return hr;
}

#define ReadBEdw(var) \
	f.Read(&((BYTE*)&var)[3], 1); \
	f.Read(&((BYTE*)&var)[2], 1); \
	f.Read(&((BYTE*)&var)[1], 1); \
	f.Read(&((BYTE*)&var)[0], 1); \

void CMpegSplitterFilter::HandleStream(CMpegSplitterFile::stream& s, CString fName, DWORD dwPictAspectRatioX, DWORD dwPictAspectRatioY, CStringA& palette)
{
	CMediaType mt = s.mt;

	// correct Aspect Ratio for DVD structure.
	if (s.mt.subtype == MEDIASUBTYPE_MPEG2_VIDEO && dwPictAspectRatioX && dwPictAspectRatioY) {
		VIDEOINFOHEADER2& vih2 = *(VIDEOINFOHEADER2*)s.mt.pbFormat;
		vih2.dwPictAspectRatioX = dwPictAspectRatioX;
		vih2.dwPictAspectRatioY = dwPictAspectRatioY;
	}

	// Add addition GUID for compatible with Cyberlink & Arcsoft VC1 Decoder
	if (mt.subtype == MEDIASUBTYPE_WVC1) {
		mt.subtype = MEDIASUBTYPE_WVC1_CYBERLINK;
		s.mts.push_back(mt);
		mt.subtype = MEDIASUBTYPE_WVC1_ARCSOFT;
		s.mts.push_back(mt);
	}

	// Add addition VobSub type
	if (mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE) {
		if (palette.IsEmpty()) {
			for (;;) {
				if (::PathFileExists(fName)) {
					CPath fname(fName);
					fname.StripPath();
					if (!CString(fname).Find(L"VTS_")) {
						fName = fName.Left(fName.ReverseFind('.') + 1);
						fName.TrimRight(L".0123456789") += L"0.ifo";

						if (::PathFileExists(fName)) {
							// read palette from .ifo file, code from CVobSubFile::ReadIfo()
							CFile f;
							if (!f.Open(fName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone)) {
								break;
							}

							/* PGC1 */
							f.Seek(0xc0 + 0x0c, SEEK_SET);

							DWORD pos;
							ReadBEdw(pos);

							f.Seek(pos * 0x800 + 0x0c, CFile::begin);

							DWORD offset;
							ReadBEdw(offset);

							/* Subpic palette */
							f.Seek(pos * 0x800 + offset + 0xa4, CFile::begin);

							CAtlList<CStringA> sl;
							for (size_t i = 0; i < 16; i++) {
								BYTE y, u, v, tmp;

								f.Read(&tmp, 1);
								f.Read(&y, 1);
								f.Read(&u, 1);
								f.Read(&v, 1);

								y = (y - 16) * 255 / 219;

								BYTE r = (BYTE)clamp(1.0 * y + 1.4022 * (v - 128), 0.0, 255.0);
								BYTE g = (BYTE)clamp(1.0 * y - 0.3456 * (u - 128) - 0.7145 * (v - 128), 0.0, 255.0);
								BYTE b = (BYTE)clamp(1.0 * y + 1.7710 * (u - 128), 0.0, 255.0);

								CStringA str;
								str.Format("%02x%02x%02x", r, g, b);
								sl.AddTail(str);
							}
							palette = Implode(sl, ',');

							f.Close();
						}
					}
				}

				break;
			}
		}

		// Get resolution for first video track
		int vid_width = 0;
		int vid_height = 0;
		POSITION pos = m_pFile->m_streams[CMpegSplitterFile::stream_type::video].GetHeadPosition();
		if (pos) {
			CMpegSplitterFile::stream& sVideo = m_pFile->m_streams[CMpegSplitterFile::stream_type::video].GetHead();
			int arx, ary;
			ExtractDim(&sVideo.mt, vid_width, vid_height, arx, ary);
			UNREFERENCED_PARAMETER(arx);
			UNREFERENCED_PARAMETER(ary);
		}

		CStringA hdr		= VobSubDefHeader(vid_width ? vid_width : 720, vid_height ? vid_height : 576, palette);

		mt.majortype		= MEDIATYPE_Subtitle;
		mt.subtype			= MEDIASUBTYPE_VOBSUB;
		mt.formattype		= FORMAT_SubtitleInfo;
		SUBTITLEINFO* si	= (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + hdr.GetLength());
		memset(si, 0, mt.FormatLength());
		si->dwOffset		= sizeof(SUBTITLEINFO);
		strncpy_s(si->IsoLang, m_pTI ? CStringA(m_pTI->GetTrackName(s.ps1id)) : "eng", _countof(si->IsoLang) - 1);
		memcpy(si + 1, (LPCSTR)hdr, hdr.GetLength());

		s.mts.push_back(mt);
	}

	if (mt.subtype == MEDIASUBTYPE_H264) {
		if (m_pFile->m_streams[CMpegSplitterFile::stream_type::video].GetCount() == 1
				&& m_pFile->m_streams[CMpegSplitterFile::stream_type::stereo].GetCount() == 1) {
			CMpegSplitterFile::stream& stereo = m_pFile->m_streams[CMpegSplitterFile::stream_type::stereo].GetHead();
			s.mts.push_back(stereo.mt);

			m_bUseMVCExtension          = TRUE;
			m_dwMasterH264TrackNumber   = s;
			m_dwMVCExtensionTrackNumber = stereo;
		}

		if (SUCCEEDED(CreateAVCfromH264(&mt))) {
			s.mts.push_back(mt);
		}
	}

	s.mts.push_back(s.mt);
}

CString CMpegSplitterFilter::FormatStreamName(CMpegSplitterFile::stream& s, CMpegSplitterFile::stream_type type)
{
	CString name = CMpegSplitterFile::CStreamList::ToString(type);
	CString str;

	int nProgram;
	const CHdmvClipInfo::Stream *pClipInfo;
	const CMpegSplitterFile::program * pProgram = m_pFile->FindProgram(s.pid, &nProgram, &pClipInfo);
	const wchar_t *pStreamName	= NULL;
	PES_STREAM_TYPE StreamType	= pClipInfo ? pClipInfo->m_Type : pProgram ? pProgram->streams[nProgram].type : INVALID;
	pStreamName					= StreamTypeToName((PES_STREAM_TYPE)StreamType);

	CStringA lang_name	= s.lang;
	lang_name			= m_pTI ? CStringA(m_pTI->GetTrackName(s.ps1id)) : lang_name;

	CString FormatDesc = GetMediaTypeDesc(s.mts.empty() ? &s.mt : &s.mts[0], pClipInfo, StreamType, lang_name);

	if (!FormatDesc.IsEmpty()) {
		str.Format(L"%s (%04x,%02x,%02x)", FormatDesc, s.pid, s.pesid, s.ps1id);
	} else if (pStreamName) {
		str.Format(L"%s - %s (%04x,%02x,%02x)", name, pStreamName, s.pid, s.pesid, s.ps1id);
	} else {
		str.Format(L"%s (%04x,%02x,%02x)", name, s.pid, s.pesid, s.ps1id);
	}

	return str;
}

__int64 CMpegSplitterFilter::SeekBD(REFERENCE_TIME rt)
{
	if (m_Items.GetCount()) {
		POSITION pos = m_Items.GetHeadPosition();
		while (pos) {
			CHdmvClipInfo::PlaylistItem* Item = m_Items.GetNext(pos);
			if (Item->m_sps.GetCount()
					&& rt >= Item->m_rtStartTime && rt <= (Item->m_rtStartTime + Item->Duration())) {
				REFERENCE_TIME _rt = rt - Item->m_rtStartTime + Item->m_rtIn;
				for (size_t idx = 0; idx < Item->m_sps.GetCount() - 1; idx++) {
					if (_rt < Item->m_sps[idx].rt) {
						if (idx > 0) {
							idx--;
						}

						return Item->m_sps[idx].fp + Item->m_SizeIn;
					}
				}
			}
		}
	}

	return -1;
}

HRESULT CMpegSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();

	ReadClipInfo(GetPartFilename(pAsyncReader));

	m_pFile.Attach(DNew CMpegSplitterFile(pAsyncReader, hr, m_ClipInfo, m_bIsBD, m_ForcedSub, m_AC3CoreOnly, m_SubEmptyPin));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	if (m_rtMin && m_rtMax && m_rtMax > m_rtMin) {
		m_pFile->m_rtMin = m_rtMin;
		m_pFile->m_rtMax = m_rtMax;
	}

	REFERENCE_TIME	rt_IfoDuration	= 0;
	fraction_t		IfoASpect		= {0, 0};
	if (m_pFile->m_type == MPEG_TYPES::mpeg_ps) {
		if (m_pInput && m_pInput->IsConnected() && (GetCLSID(m_pInput->GetConnected()) == __uuidof(CVTSReader))) { // MPC VTS Reader
			CComPtr<IBaseFilter> pBF = GetFilterFromPin(m_pInput->GetConnected());
			m_pTI = pBF;
			if (CComQIPtr<IVTSReader> VTSReader = pBF) {
				rt_IfoDuration	= VTSReader->GetDuration();
				IfoASpect		= VTSReader->GetAspectRatio();
			}
		}
	}

	if (m_ClipInfo.IsHdmv()) {
		for (size_t i = 0; i < m_ClipInfo.GetStreamCount(); i++) {
			CHdmvClipInfo::Stream* stream = m_ClipInfo.GetStreamByIndex (i);
			if (stream->m_Type == PRESENTATION_GRAPHICS_STREAM) {
				m_pFile->AddHdmvPGStream(stream->m_PID, stream->m_LanguageCode);
			}
		}
	}

	int audio_sel	= 0;
	int subpic_sel	= 0;

#ifdef REGISTER_FILTER
	CString lang;
	CAtlList<CString> audioLangList;
	CAtlList<CString> subpicLangList;

	if (!m_AudioLanguageOrder.IsEmpty()) {
		int tPos = 0;
		lang = m_AudioLanguageOrder.Tokenize(L",; ", tPos);
		while (tPos != -1) {
			if (!lang.IsEmpty()) {
				audioLangList.AddTail(lang);
			}
			lang = m_AudioLanguageOrder.Tokenize(L",; ", tPos);
		}
		if (!lang.IsEmpty()) {
			audioLangList.AddTail(lang);
		}
	}

	if (!m_SubtitlesLanguageOrder.IsEmpty()) {
		int tPos = 0;
		lang = m_SubtitlesLanguageOrder.Tokenize(L",; ", tPos);
		while (tPos != -1) {
			if (!lang.IsEmpty()) {
				subpicLangList.AddTail(lang);
			}
			lang = m_SubtitlesLanguageOrder.Tokenize(L",; ", tPos);
		}
		if (!lang.IsEmpty()) {
			subpicLangList.AddTail(lang);
		}
	}

	for (int type = CMpegSplitterFile::stream_type::video; type <= CMpegSplitterFile::stream_type::subpic; type++) {
		int stream_idx	= 0;
		int Idx_audio	= 99;
		int Idx_subpic	= 99;

		POSITION pos = m_pFile->m_streams[type].GetHeadPosition();
		while (pos) {
			CMpegSplitterFile::stream& s = m_pFile->m_streams[type].GetNext(pos);
			if (type == CMpegSplitterFile::stream_type::subpic && s.pid == NO_SUBTITLE_PID) {
				continue;
			};

			CString str = FormatStreamName(s, (CMpegSplitterFile::stream_type)type);
			CString str_tmp(str);
			str_tmp.MakeLower();
			if (type == CMpegSplitterFile::stream_type::audio) {
				CString audioStreamSelected;
				if (!audioLangList.IsEmpty()) {
					int idx = 0;
					POSITION pos = audioLangList.GetHeadPosition();
					while (pos) {
						lang = audioLangList.GetNext(pos).MakeLower();
						if (-1 != str_tmp.Find(lang)) {
							if (idx < Idx_audio) {
								audioStreamSelected	= str;
								Idx_audio			= idx;
								break;
							}
						}
						idx++;
					}
				}
				if (!Idx_audio && !audioStreamSelected.IsEmpty()) {
					audio_sel = stream_idx;
					break;
				}
			} else if (type == CMpegSplitterFile::stream_type::subpic) {
				CString subpicStreamSelected;
				if (!subpicLangList.IsEmpty()) {
					int idx = 0;
					POSITION pos = subpicLangList.GetHeadPosition();
					while (pos) {
						lang = subpicLangList.GetNext(pos).MakeLower();
						if (-1 != str_tmp.Find(lang)) {
							if (idx < Idx_subpic) {
								subpicStreamSelected	= str;
								Idx_subpic				= idx;
								break;
							}
						}
						idx++;
					}
				}
				if (!Idx_subpic && !subpicStreamSelected.IsEmpty()) {
					subpic_sel = stream_idx;
					break;
				}
			}

			stream_idx++;
		}
	}
#endif

	CString fullName = GetPartFilename(pAsyncReader);
	if (fullName.IsEmpty()) {
		// trying to get file name from FileSource
		BeginEnumFilters(m_pGraph, pEF, pBF) {
			CComQIPtr<IFileSourceFilter> pFSF = pBF;
			if (pFSF) {
				LPOLESTR pFN = NULL;
				AM_MEDIA_TYPE mt;
				if (SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN) {
					fullName = CString(pFN);
					CoTaskMemFree(pFN);
				}
				break;
			}
		}
		EndEnumFilters
	}

	CStringA palette;
	for (int type = CMpegSplitterFile::stream_type::video; type <= CMpegSplitterFile::stream_type::subpic; type++) {
		POSITION pos = m_pFile->m_streams[type].GetHeadPosition();
		while (pos) {
			CMpegSplitterFile::stream& s = m_pFile->m_streams[type].GetNext(pos);
			HandleStream(s, fullName, IfoASpect.num, IfoASpect.den, palette);
		}
	}

	for (int type = CMpegSplitterFile::stream_type::video; type <= CMpegSplitterFile::stream_type::subpic; type++) {
		int stream_idx = 0;

		POSITION pos = m_pFile->m_streams[type].GetHeadPosition();
		while (pos) {
			CMpegSplitterFile::stream& s = m_pFile->m_streams[type].GetNext(pos);

			CString str;

			if (type == CMpegSplitterFile::stream_type::subpic && s.pid == NO_SUBTITLE_PID) {
				str	= NO_SUBTITLE_NAME;
			} else {
				str = FormatStreamName(s, (CMpegSplitterFile::stream_type)type);
			}

			CAtlArray<CMediaType> mts;
			for(size_t i = 0; i < s.mts.size(); i++) {
				mts.Add(s.mts[i]);
			}
			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CMpegSplitterOutputPin(mts, (CMpegSplitterFile::stream_type)type, str, this, this, &hr));

			if (type == CMpegSplitterFile::stream_type::audio) {
				if (audio_sel == stream_idx && (S_OK == AddOutputPin(s, pPinOut))) {
					break;
				}
			} else if (type == CMpegSplitterFile::stream_type::subpic) {
				if (subpic_sel == stream_idx && (S_OK == AddOutputPin(s, pPinOut))) {
					break;
				}
			} else if (S_OK == AddOutputPin(s, pPinOut)) {
				break;
			}

			stream_idx++;
		}
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	m_length = m_pFile->GetLength();

	if (m_rtPlaylistDuration) {
		m_rtDuration = m_rtPlaylistDuration;
		m_nFlag &= ~PACKET_PTS_DISCONTINUITY;
	} else if (rt_IfoDuration) {
		m_rtDuration = rt_IfoDuration;
	} else if (!m_pFile->IsStreaming() && m_pFile->m_rate) {
		m_rtDuration = UNITS * m_pFile->GetLength() / m_pFile->m_rate;
	}

	m_rtNewStop = m_rtStop = m_rtDuration;

	if (m_bUseMVCExtension && !m_Items.IsEmpty()) {
		SetProperty(L"STEREOSCOPIC3DMODE", m_MVC_Base_View_R_flag ? L"mvc_rl" : L"mvc_lr");

		if (m_Items.GetCount()) {
			POSITION pos = m_Items.GetHeadPosition();
			while (pos) {
				CHdmvClipInfo::PlaylistItem* Item = m_Items.GetNext(pos);
				Item->m_sps.RemoveAll();
			}
		}

		// PG offsets
		const CHdmvClipInfo::PlaylistItem* Item = m_Items.GetHead();
		if (Item->m_pg_offset_sequence_id.size()) {
			std::list<BYTE> pg_offsets;
			for (auto it = Item->m_pg_offset_sequence_id.begin(); it != Item->m_pg_offset_sequence_id.end(); it++) {
				if (*it != 0xff) {
					pg_offsets.push_back(*it);

					CString offset; offset.Format(L"%u", *it);
					SetProperty(L"stereo_subtitle_offset_id", offset);
				}
			}
			if (pg_offsets.size()) {
				CString offsets;

				pg_offsets.sort();
				pg_offsets.unique();
				for (auto it = pg_offsets.begin(); it != pg_offsets.end(); it++) {
					if (offsets.IsEmpty()) {
						offsets.Format(L"%u", *it);
					} else {
						offsets.AppendFormat(L",%u", *it);
					}
				}

				SetProperty(L"stereo_subtitle_offset_ids", offsets);
			}
		}

		// IG offsets
		if (Item->m_ig_offset_sequence_id.size()) {
			std::list<BYTE> ig_offsets;
			for (auto it = Item->m_ig_offset_sequence_id.begin(); it != Item->m_ig_offset_sequence_id.end(); it++) {
				if (*it != 0xff) {
					ig_offsets.push_back(*it);
				}
			}
			if (ig_offsets.size()) {
				CString offsets;

				ig_offsets.sort();
				ig_offsets.unique();
				for (auto it = ig_offsets.begin(); it != ig_offsets.end(); it++) {
					if (offsets.IsEmpty()) {
						offsets.Format(L"%u", *it);
					} else {
						offsets.AppendFormat(L",%u", *it);
					}
				}

				SetProperty(L"stereo_interactive_offset_ids", offsets);
			}
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CMpegSplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(m_pFile, VFW_E_NOT_CONNECTED);

	if (m_pFile->IsVariableSize() && m_length != m_pFile->GetLength()) {
		const REFERENCE_TIME rtDuration = UNITS * m_pFile->GetLength() / m_pFile->m_rate;
		if (llabs(rtDuration - m_rtDuration) >= UNITS) {
			m_rtNewStop = m_rtStop = m_rtDuration = rtDuration;
			NotifyEvent(EC_LENGTH_CHANGED, 0, 0);
		}

		m_length = m_pFile->GetLength();
	}

	return __super::GetDuration(pDuration);
}

bool CMpegSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CMpegSplitterFilter");
	if (!m_pFile) {
		return false;
	}

	if (m_bUseMVCExtension) {
		if (IPin* pPin = dynamic_cast<IPin*>(GetOutputPin(m_dwMasterH264TrackNumber))) {
			CMediaType mt;
			if (SUCCEEDED(pPin->ConnectionMediaType(&mt))) {
				if (mt.subtype != MEDIASUBTYPE_AMVC) {
					m_bUseMVCExtension          = FALSE;
					m_dwMasterH264TrackNumber   = DWORD_MAX;
					m_dwMVCExtensionTrackNumber = DWORD_MAX;
				}
			}
		}
	}

	return true;
}

#define SimpleSeek																		\
	__int64 nextPos;																	\
	REFERENCE_TIME rtPTS = m_pFile->NextPTS(masterStream, masterStream.codec, nextPos);	\
	if (rtPTS != INVALID_TIME) {														\
		m_rtStartOffset = m_pFile->m_rtMin + rtPTS - rt;								\
	}																					\
	if (m_rtStartOffset > m_pFile->m_rtMax) {											\
		m_rtStartOffset = 0;															\
	}																					\

#define SeekPos(t) (__int64)(1.0 * t / m_rtDuration * len)

void CMpegSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (m_pFile->IsStreaming() || m_rtDuration <= 0) {
		return;
	}

	CMpegSplitterFile::CStreamList* pMasterStream = m_pFile->GetMasterStream();
	if (!pMasterStream) {
		ASSERT(0);
		return;
	}

	m_rtSeekOffset  = INVALID_TIME;
	m_rtStartOffset = 0;

	m_MVCExtensionQueue.RemoveAll();
	m_MVCBaseQueue.RemoveAll();

	CMpegSplitterFile::stream& masterStream = pMasterStream->GetHead();
	DWORD TrackNum = masterStream;

	if (rt == 0) {
		m_pFile->Seek(m_pFile->m_posMin);
	} else {
		const REFERENCE_TIME rtmax = rt - UNITS;

		const __int64 pos = SeekBD(rt);
		if (pos >= 0 && pos < (m_pFile->GetLength() - 4)) {
			m_pFile->Seek(pos + 4);

			__int64 nextPos;
			REFERENCE_TIME rtPTS     = m_pFile->NextPTS(TrackNum, masterStream.codec, nextPos);
			REFERENCE_TIME minseekrt = rtPTS;
			__int64 minseekpos       = m_pFile->GetPos();

			while (m_pFile->GetRemaining()) {
				m_pFile->Seek(nextPos);
				rtPTS = m_pFile->NextPTS(TrackNum, masterStream.codec, nextPos);
				if (rtPTS > rtmax || rtPTS == INVALID_TIME) {
					break;
				}

				minseekrt = rtPTS;
				minseekpos = m_pFile->GetPos();
			}

			DLog(L"CMpegSplitterFilter::DemuxSeek() : BD seek, %I64d -> %I64d, position - %I64d", rt, minseekrt, minseekpos);

			m_rtSeekOffset = minseekrt;
			return;
		}

		const __int64 len          = m_pFile->GetLength();
		__int64 seekpos            = SeekPos(rt);
		__int64 minseekpos         = _I64_MIN;
		REFERENCE_TIME minseekrt   = INVALID_TIME;

		const REFERENCE_TIME rtmin = rtmax - UNITS/2;

		if (m_pFile->m_bPESPTSPresent) {
			POSITION pos = pMasterStream->GetHeadPosition();
			while (pos) {
				CMpegSplitterFile::stream& stream = pMasterStream->GetNext(pos);
				CMpegSplitterFile::stream_codec codec = stream.codec;
				TrackNum = stream;

				CBaseSplitterOutputPin* pPin = GetOutputPin(TrackNum);
				if (pPin && pPin->IsConnected()) {
					if (TrackNum == m_dwMasterH264TrackNumber) {
						TrackNum = m_dwMVCExtensionTrackNumber;
						codec = CMpegSplitterFile::stream_codec::MVC;
					}

					m_pFile->Seek(seekpos);
					__int64 curpos = seekpos;

					double div = 1.0;
					__int64 nextPos;
					for (;;) {
						REFERENCE_TIME rtPTS = m_pFile->NextPTS(TrackNum, codec, nextPos);
						if (rtPTS != INVALID_TIME
								&& rtmin <= rtPTS && rtPTS < rtmax) {
							minseekrt = rtPTS;
							minseekpos = m_pFile->GetPos();

							if (codec == CMpegSplitterFile::stream_codec::MPEG
									|| codec == CMpegSplitterFile::stream_codec::H264) {
								const REFERENCE_TIME rtLimit = codec == CMpegSplitterFile::stream_codec::MPEG ? rt : rt - UNITS/2;
								while (m_pFile->GetRemaining()) {
									m_pFile->Seek(nextPos);
									rtPTS = m_pFile->NextPTS(TrackNum, codec, nextPos, TRUE, rtLimit);
									if (rtPTS > rtLimit || rtPTS == INVALID_TIME) {
										break;
									}
									minseekrt = rtPTS;
									minseekpos = m_pFile->GetPos();
								}
							}

							break;
						}

						REFERENCE_TIME dt = rtPTS == INVALID_TIME ? rtPTS : rtPTS - rtmax;
						if (rtPTS < 0) {
							dt = 20 * UNITS / div;
						}
						dt /= div;
						div += 0.05;

						if (div >= 5.0) {
							break;
						}

						curpos -= SeekPos(dt);
						m_pFile->Seek(curpos);
					}
				}
			}
		}

		if (minseekpos != _I64_MIN) {
			DLog(L"CMpegSplitterFilter::DemuxSeek() : seek by timestamp, %I64d -> %I64d, position - %I64d", rt, minseekrt, minseekpos);
			seekpos = minseekpos;
			if (m_bIsBD) {
				m_rtSeekOffset = minseekrt;
			}
		} else {
			// simple seek by bitrate

			seekpos	= SeekPos(rt);
			m_pFile->Seek(seekpos);

			if (m_pFile->m_bPESPTSPresent) {
				SimpleSeek;
				if (rtPTS != INVALID_TIME) {
					seekpos = m_pFile->GetPos();
				}

				DLog(L"CMpegSplitterFilter::DemuxSeek() : seek by bitrate, %I64d -> %I64d, position - %I64d", rt, rtPTS, seekpos);
			}
		}

		m_pFile->Seek(seekpos);
	}
}

bool CMpegSplitterFilter::DemuxLoop()
{
	CMpegSplitterFile::CStreamList* pMasterStream = m_pFile->GetMasterStream();
	if (!pMasterStream) {
		ASSERT(0);
		return false;
	}

	REFERENCE_TIME rtStartOffset = m_rtStartOffset ? m_rtStartOffset : m_pFile->m_rtMin;
	m_rtGlobalPCRTimeStamp = INVALID_TIME;

	HRESULT hr = S_OK;
	while (SUCCEEDED(hr) && !CheckRequest(NULL)) {
		hr = DemuxNextPacket(rtStartOffset);

		if (FAILED(m_pFile->GetLastReadError())) {
			break;
		}
	}

	POSITION pos = pPackets.GetStartPosition();
	while (pos) {
		if (CAutoPtr<CPacket>& p = pPackets.GetNextValue(pos)) {
			if (p->bSyncPoint && GetOutputPin(p->TrackNumber)) {
				DeliverPacket(p);
			}
			p.Free();
		}
	}
	pPackets.RemoveAll();

	return true;
}

bool CMpegSplitterFilter::BuildPlaylist(LPCTSTR pszFileName, CHdmvClipInfo::CPlaylist& Items)
{
	m_rtPlaylistDuration = 0;

	bool res = SUCCEEDED(m_ClipInfo.ReadPlaylist(pszFileName, m_rtPlaylistDuration, Items, TRUE, &m_MVC_Base_View_R_flag));
	if (res) {
		m_rtMin = Items.GetHead()->m_rtIn;
		REFERENCE_TIME rtDur = 0;

		POSITION pos = Items.GetHeadPosition();
		while (pos) {
			rtDur += Items.GetNext(pos)->Duration();
		}

		m_rtMax = m_rtMin + rtDur;
	}

	m_bIsBD = res;

	return res;
}

bool CMpegSplitterFilter::BuildChapters(LPCTSTR pszFileName, CHdmvClipInfo::CPlaylist& PlaylistItems, CHdmvClipInfo::CPlaylistChapter& Items)
{
	return SUCCEEDED(m_ClipInfo.ReadChapters(pszFileName, PlaylistItems, Items)) ? true : false;
}

// IAMStreamSelect

STDMETHODIMP CMpegSplitterFilter::Count(DWORD* pcStreams)
{
	CheckPointer(pcStreams, E_POINTER);

	*pcStreams = 0;

	for (int i = CMpegSplitterFile::stream_type::video; i <= CMpegSplitterFile::stream_type::subpic; i++) {
		(*pcStreams) += m_pFile->m_streams[i].GetCount();
	}

	if (m_pFile->m_programs.GetValidCount() > 1) {
		(*pcStreams) += m_pFile->m_programs.GetValidCount();
	}

	return S_OK;
}

STDMETHODIMP CMpegSplitterFilter::Enable(long lIndex, DWORD dwFlags)
{
	if (!(dwFlags & AMSTREAMSELECTENABLE_ENABLE)) {
		return E_NOTIMPL;
	}

	if (m_pFile->m_programs.GetValidCount() > 1) {
		if (lIndex < (long)m_pFile->m_programs.GetValidCount()) {
			POSITION pos = m_pFile->m_programs.GetStartPosition();
			int j = 0;
			while (pos) {
				CMpegSplitterFile::program* p = &m_pFile->m_programs.GetNextValue(pos);
				if (!p->streamCount(m_pFile->m_streams)) {
					continue;
				}
				if (j == lIndex) {
					for (auto stream = p->streams.begin(); stream != p->streams.end(); stream++) {
						if (stream->pid) {
							long index = m_pFile->m_programs.GetValidCount();
							for (int type = CMpegSplitterFile::stream_type::video; type < CMpegSplitterFile::stream_type::subpic; type++) {
								POSITION pos2 = m_pFile->m_streams[type].GetHeadPosition();
								while (pos2) {
									CMpegSplitterFile::stream& s = m_pFile->m_streams[type].GetNext(pos2);
									if (s.pid == stream->pid) {
										Enable(index, dwFlags);
									}
									index++;
								}
							}
						}
					}
					break;
				}
				j++;
			}

			return S_OK;
		}

		lIndex -= m_pFile->m_programs.GetValidCount();
	}

	for (int type = CMpegSplitterFile::stream_type::video, j = 0; type <= CMpegSplitterFile::stream_type::subpic; type++) {
		int cnt = m_pFile->m_streams[type].GetCount();

		if (lIndex >= j && lIndex < j + cnt) {
			lIndex -= j;

			POSITION pos = m_pFile->m_streams[type].FindIndex(lIndex);
			if (!pos) {
				return E_UNEXPECTED;
			}

			CMpegSplitterFile::stream& to = m_pFile->m_streams[type].GetAt(pos);

			pos = m_pFile->m_streams[type].GetHeadPosition();
			while (pos) {
				CMpegSplitterFile::stream& from = m_pFile->m_streams[type].GetNext(pos);
				if (!GetOutputPin(from)) {
					continue;
				}

				if (from == to) {
					return S_OK;
				}

				CComQIPtr<IMediaControl> pMC(m_pGraph);
				OAFilterState fs = -1;
				if(pMC) {
					pMC->GetState(100, &fs);
					pMC->Stop();
				}
				Lock();

				if (type == CMpegSplitterFile::stream_type::subpic && to.mts.size() == 1 && to.mts[0].subtype == MEDIASUBTYPE_DVD_SUBPICTURE) {
					POSITION pos2 = m_pFile->m_streams[type].GetHeadPosition();
					while (pos2) {
						CMpegSplitterFile::stream& s_src = m_pFile->m_streams[type].GetNext(pos2);
						if (s_src != to && s_src.mts.size() == 2 && s_src.mts[0].subtype == MEDIASUBTYPE_VOBSUB) {
							to.mts.insert(to.mts.begin(), s_src.mts[0]);
							break;
						}
					}
				}

				HRESULT hr = RenameOutputPin(from, to, to.mts, type == CMpegSplitterFile::stream_type::subpic);

				Unlock();
				if (pMC) {
					if(fs == State_Running) {
						pMC->Run();
					} else if (fs == State_Paused) {
						pMC->Pause();
					}
				}

				if (FAILED(hr)) {
					return hr;
				}

				return S_OK;
			}
		}

		j += cnt;
	}

	return S_FALSE;
}

STDMETHODIMP CMpegSplitterFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	if (m_pFile->m_programs.GetValidCount() > 1) {
		if (lIndex < (long)m_pFile->m_programs.GetValidCount()) {
			int type = -1;
			for (type = CMpegSplitterFile::stream_type::video; type <= CMpegSplitterFile::stream_type::subpic; type++) {
				if (m_pFile->m_streams[type].GetCount()) {
					break;
				}
			}

			WORD pidEnabled = -1;
			if (type == CMpegSplitterFile::stream_type::video
					|| type == CMpegSplitterFile::stream_type::audio) {
				CMpegSplitterFile::CStreamList& lst = m_pFile->m_streams[type];
				POSITION pos = lst.GetHeadPosition();
				while (pos) {
					CMpegSplitterFile::stream& s = m_pFile->m_streams[type].GetNext(pos);
					if (GetOutputPin(s)) {
						pidEnabled = s;
						break;
					}
				}
			}

			POSITION pos = m_pFile->m_programs.GetStartPosition();
			int j = 0;
			while (pos) {
				CMpegSplitterFile::program* p = &m_pFile->m_programs.GetNextValue(pos);
				if (!p->streamCount(m_pFile->m_streams)) {
					continue;
				}

				if (j == lIndex) {
					if (ppmt) {
						*ppmt = NULL;
					}
					if (pdwFlags) {
						*pdwFlags = 0;

						for (auto stream = p->streams.begin(); stream != p->streams.end(); stream++) {
							if (stream->pid == pidEnabled) {
								*pdwFlags = AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE;
								break;
							}
						}
					}
					if (plcid) {
						*plcid = 0;
					}
					if (pdwGroup) {
						*pdwGroup = 0x67458F;
					}
					if (ppObject) {
						*ppObject = NULL;
					}
					if (ppUnk) {
						*ppUnk = NULL;
					}
					if (ppszName) {
						CString str;
						if (p->name.IsEmpty()) {
							str.Format(L"Program - %d", p->program_number);
						} else {
							str = p->name;
						}

						*ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength() + 1) * sizeof(WCHAR));
						if (*ppszName == NULL) {
							return E_OUTOFMEMORY;
						}

						wcscpy_s(*ppszName, str.GetLength() + 1, str);
					}
					return S_OK;
				}
				j++;
			}

			return S_OK;
		}
		lIndex -= m_pFile->m_programs.GetValidCount();
	}

	for (int type = CMpegSplitterFile::stream_type::video, j = 0; type <= CMpegSplitterFile::stream_type::subpic; type++) {
		int cnt = m_pFile->m_streams[type].GetCount();

		if (lIndex >= j && lIndex < j+cnt) {
			lIndex -= j;

			POSITION pos = m_pFile->m_streams[type].FindIndex(lIndex);
			if (!pos) {
				return E_UNEXPECTED;
			}

			CMpegSplitterFile::stream& s	= m_pFile->m_streams[type].GetAt(pos);
			CHdmvClipInfo::Stream* pStream	= m_ClipInfo.FindStream(s.pid);

			if (ppmt) {
				*ppmt = CreateMediaType(&s.mts[0]);
			}
			if (pdwFlags) {
				*pdwFlags = GetOutputPin(s) ? (AMSTREAMSELECTINFO_ENABLED | AMSTREAMSELECTINFO_EXCLUSIVE) : 0;
			}
			if (plcid) {
				CStringA lang_name	= s.lang;
				LCID lcid			= !lang_name.IsEmpty() ? ISO6392ToLcid(lang_name) : 0;

				*plcid = pStream ? pStream->m_LCID : lcid;
			}
			if (pdwGroup) {
				*pdwGroup = type;
			}
			if (ppObject) {
				*ppObject = NULL;
			}
			if (ppUnk) {
				*ppUnk = NULL;
			}

			if (ppszName) {
				CString str;

				if (type == CMpegSplitterFile::stream_type::subpic && s.pid == NO_SUBTITLE_PID) {
					str = NO_SUBTITLE_NAME;
					if (plcid) {
						*plcid = (LCID)LCID_NOSUBTITLES;
					}
				} else {
					str = FormatStreamName(s, (CMpegSplitterFile::stream_type)type);
				}

				*ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength() + 1) * sizeof(WCHAR));
				if (*ppszName == NULL) {
					return E_OUTOFMEMORY;
				}

				wcscpy_s(*ppszName, str.GetLength() + 1, str);
			}

			return S_OK;
		}

		j += cnt;
	}

	return S_FALSE;
}

// ISpecifyPropertyPages2

STDMETHODIMP CMpegSplitterFilter::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	pPages->pElems[0] = __uuidof(CMpegSplitterSettingsWnd);

	return S_OK;
}

STDMETHODIMP CMpegSplitterFilter::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMpegSplitterSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMpegSplitterSettingsWnd>(NULL, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// IMpegSplitterFilter
STDMETHODIMP CMpegSplitterFilter::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_MPEGSplit)) {
		key.SetStringValue(OPT_AudioLangOrder, m_AudioLanguageOrder);
		key.SetStringValue(OPT_SubLangOrder, m_SubtitlesLanguageOrder);
		key.SetDWORDValue(OPT_ForcedSub, m_ForcedSub);
		key.SetDWORDValue(OPT_AC3CoreOnly, m_AC3CoreOnly);
		key.SetDWORDValue(OPT_SubEmptyOutput, m_SubEmptyPin);
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MPEGSplit, OPT_ForcedSub, m_ForcedSub);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MPEGSplit, OPT_AC3CoreOnly, m_AC3CoreOnly);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MPEGSplit, OPT_SubEmptyOutput, m_SubEmptyPin);
#endif

	return S_OK;
}

#ifdef REGISTER_FILTER
STDMETHODIMP CMpegSplitterFilter::SetAudioLanguageOrder(WCHAR *nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_AudioLanguageOrder = nValue;
	return S_OK;
}

STDMETHODIMP_(WCHAR *) CMpegSplitterFilter::GetAudioLanguageOrder()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_AudioLanguageOrder.GetBuffer();
}

STDMETHODIMP CMpegSplitterFilter::SetSubtitlesLanguageOrder(WCHAR *nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_SubtitlesLanguageOrder = nValue;
	return S_OK;
}

STDMETHODIMP_(WCHAR *) CMpegSplitterFilter::GetSubtitlesLanguageOrder()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_SubtitlesLanguageOrder.GetBuffer();
}
#endif

STDMETHODIMP CMpegSplitterFilter::SetForcedSub(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_ForcedSub = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpegSplitterFilter::GetForcedSub()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_ForcedSub;
}

STDMETHODIMP CMpegSplitterFilter::SetTrueHD(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_AC3CoreOnly = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMpegSplitterFilter::GetTrueHD()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_AC3CoreOnly;
}

STDMETHODIMP CMpegSplitterFilter::SetSubEmptyPin(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_SubEmptyPin = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMpegSplitterFilter::GetSubEmptyPin()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_SubEmptyPin;
}

//
// CMpegSourceFilter
//

CMpegSourceFilter::CMpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CMpegSplitterFilter(pUnk, phr, clsid)
{
	m_pInput.Free();
}

//
// CMpegSplitterOutputPin
//

CMpegSplitterOutputPin::CMpegSplitterOutputPin(CAtlArray<CMediaType>& mts, CMpegSplitterFile::stream_type type, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterParserOutputPin(mts, pName, pFilter, pLock, phr)
	, m_type(type)
{
}

HRESULT CMpegSplitterOutputPin::CheckMediaType(const CMediaType* pmt)
{
	CAtlArray<CMediaType> mts;
	(static_cast<CMpegSplitterFilter*>(m_pFilter))->GetMediaTypes(m_type, mts);
	for (size_t i = 0; i < mts.GetCount(); i++) {
		if (mts[i] == *pmt) {
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

HRESULT CMpegSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if (m_SubtitleType) {
		m_bNeedNextSubtitle = false;
	}

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CMpegSplitterOutputPin::QueuePacket(CAutoPtr<CPacket> p)
{
	if (!ThreadExists()) {
		return S_FALSE;
	}

	bool force_packet = false;

	if (m_SubtitleType == hdmvsub) {
		if (p && p->GetCount() >= 3) {
			int segtype = p->GetData()[0];
			//int unitsize = p->GetData()[1] << 8 | p->GetData()[2];
			//if (segtype == 22) Log2File(L"");
			//Log2File(L"segtype %3d, unitsize %5d, time %4.3f", segtype, unitsize, p->rtStart/10000000.0);

			if (segtype == 22) {
				// this is first packet of HDMV sub, set standart mode
				m_bNeedNextSubtitle = false;
				force_packet = true; // but send this packet anyway
			}
			else if (segtype == 21) {
				// this is picture packet, force next HDMV sub
				m_bNeedNextSubtitle = true;
			}
		}
	}
	else if (m_SubtitleType == dvbsub) {
		if (p && p->GetCount() >= 6) {
			BYTE* pos = p->GetData();
			BYTE* end = pos + p->GetCount();

			while (pos + 6 < end) {
				if (*pos++ == 0x0F) {
					int segtype = *pos++;
					pos += 2;
					int seglength = _byteswap_ushort(GETWORD(pos));
					pos += 2 + seglength;

					if (segtype == 0x14) { // first "display" segment
						force_packet = m_bNeedNextSubtitle;
						m_bNeedNextSubtitle = false;
					}
					else if (segtype == 0x13) { // "object" segment
						m_bNeedNextSubtitle = true;
					}
				}
			}
		}
	}

	if (S_OK == m_hrDeliver && (force_packet || ((CMpegSplitterFilter*)pSplitter)->IsHdmvDvbSubPinDrying())) {
		m_queue.Add(p);
		return m_hrDeliver;
	}

	return __super::QueuePacket(p);
}

STDMETHODIMP CMpegSplitterOutputPin::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt)
{
	HRESULT		hr;
	PIN_INFO	PinInfo;

	if (SUCCEEDED(pReceivePin->QueryPinInfo(&PinInfo))) {
		GUID FilterClsid;
		if (SUCCEEDED(PinInfo.pFilter->GetClassID(&FilterClsid))) {
			if (FilterClsid == CLSID_DMOWrapperFilter) {
				(static_cast<CMpegSplitterFilter*>(m_pFilter))->SetPipo(true);
			}
		}
		PinInfo.pFilter->Release();
	}

	hr = __super::Connect(pReceivePin, pmt);

	if (S_OK == hr && m_mt.majortype == MEDIATYPE_Subtitle) {
		if (m_mt.subtype == MEDIASUBTYPE_HDMVSUB) {
			m_SubtitleType = hdmvsub;
			(static_cast<CMpegSplitterFilter*>(m_pFilter))->m_hasHdmvDvbSubPin = true;
		}
		else if (m_mt.subtype == MEDIASUBTYPE_DVB_SUBTITLES) {
			m_SubtitleType = dvbsub;
			(static_cast<CMpegSplitterFilter*>(m_pFilter))->m_hasHdmvDvbSubPin = true;
		}
		else {
			m_SubtitleType = not_hdmv_dvbsub;
			(static_cast<CMpegSplitterFilter*>(m_pFilter))->m_hasHdmvDvbSubPin = false; // only one output pin for subtitles
		}
	}

	(static_cast<CMpegSplitterFilter*>(m_pFilter))->SetPipo(false);

	return hr;
}
