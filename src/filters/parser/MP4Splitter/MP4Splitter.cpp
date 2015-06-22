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
#include <MMreg.h>
#include "MP4Splitter.h"
#include "../../../DSUtil/GolombBuffer.h"
#include "../../../DSUtil/AudioParser.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>
#include <basestruct.h>

#include <Bento4/Core/Ap4.h>
#include <Bento4/Core/Ap4File.h>
#include <Bento4/Core/Ap4SttsAtom.h>
#include <Bento4/Core/Ap4StssAtom.h>
#include <Bento4/Core/Ap4StsdAtom.h>
#include <Bento4/Core/Ap4IsmaCryp.h>
#include <Bento4/Core/Ap4AvcCAtom.h>
#include <Bento4/Core/Ap4HvcCAtom.h>
#include <Bento4/Core/Ap4ChplAtom.h>
#include <Bento4/Core/Ap4FtabAtom.h>
#include <Bento4/Core/Ap4DataAtom.h>
#include <Bento4/Core/Ap4PaspAtom.h>
#include <Bento4/Core/Ap4ChapAtom.h>
#include <Bento4/Core/Ap4Dvc1Atom.h>
#include <Bento4/Core/Ap4WfexAtom.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MP4},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMP4SplitterFilter), MP4SplitterName, MERIT_NORMAL, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMP4SourceFilter), MP4SourceName, MERIT_NORMAL, 0, NULL, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMPEG4VideoSplitterFilter), L"MPC MPEG4 Video Splitter", MERIT_NORMAL, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMPEG4VideoSourceFilter), L"MPC MPEG4 Video Source", MERIT_NORMAL, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMP4SplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMP4SourceFilter>, NULL, &sudFilter[1]},
	{sudFilter[2].strName, sudFilter[2].clsID, CreateInstance<CMPEG4VideoSplitterFilter>, NULL, &sudFilter[2]},
	{sudFilter[3].strName, sudFilter[3].clsID, CreateInstance<CMPEG4VideoSourceFilter>, NULL, &sudFilter[3]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	DeleteRegKey(_T("Media Type\\Extensions\\"), _T(".mp4"));
	DeleteRegKey(_T("Media Type\\Extensions\\"), _T(".mov"));

	CAtlList<CString> chkbytes;

	// mov, mp4
	chkbytes.AddTail(_T("4,4,,66747970")); // '....ftyp'
	chkbytes.AddTail(_T("4,4,,6d6f6f76")); // '....moov'
	chkbytes.AddTail(_T("4,4,,6d646174")); // '....mdat'
	chkbytes.AddTail(_T("4,4,,77696465")); // '....wide'
	chkbytes.AddTail(_T("4,4,,736b6970")); // '....skip'
	chkbytes.AddTail(_T("4,4,,66726565")); // '....free'
	chkbytes.AddTail(_T("4,4,,706e6f74")); // '....pnot'
	// raw mpeg4 video
	chkbytes.AddTail(_T("3,3,,000001"));

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MP4, chkbytes, NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_MP4);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CMP4SplitterFilter
//

CMP4SplitterFilter::CMP4SplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMP4SplitterFilter"), pUnk, phr, __uuidof(this))
{
}

CMP4SplitterFilter::~CMP4SplitterFilter()
{
}

STDMETHODIMP CMP4SplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, MP4SourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

void SetTrackName(CString *TrackName, CString Suffix)
{
	if (TrackName->IsEmpty()) {
		*TrackName = Suffix;
	} else {
		*TrackName += _T(" - ");
		*TrackName += Suffix;
	}
}

#define FormatTrackName(name, append)			\
	if (!tname.IsEmpty() && tname[0] < 0x20) {	\
		tname.Delete(0);						\
	}											\
	if (tname.IsEmpty()) {						\
		tname = name;							\
	} else if (append) {						\
		tname.AppendFormat(_T("(%s)"), name);	\
	}											\
	SetTrackName(&TrackName, tname);			\

#define HandlePASP(SampleEntry)																			\
	if (AP4_PaspAtom* pasp = dynamic_cast<AP4_PaspAtom*>(SampleEntry->GetChild(AP4_ATOM_TYPE_PASP))) {	\
		if (!Aspect.cx && pasp->GetNum() > 0 && pasp->GetDen() > 0) {									\
			Aspect.SetSize(pasp->GetNum(), pasp->GetDen());												\
		}																								\
	}																									\

static void SetAspect(CSize& Aspect, LONG width, LONG height, LONG codec_width, LONG codec_height, VIDEOINFOHEADER2* vih2)
{
	if (!Aspect.cx || !Aspect.cy) {
		if (width && height) {
			Aspect.SetSize(width, height);
		} else {
			Aspect.SetSize(codec_width, codec_height);
		}
	} else {
		Aspect.cx *= codec_width;
		Aspect.cy *= codec_height;
	}
	ReduceDim(Aspect);

	if (vih2) {
		vih2->dwPictAspectRatioX = Aspect.cx;
		vih2->dwPictAspectRatioY = Aspect.cy;
	}
}

static CString ConvertStr(const char* S)
{
	CString str = AltUTF8To16(S);
	if (str.IsEmpty()) {
		str = ConvertToUTF16(S, CP_ACP); //Trying Local...
	}

	return str;
}

HRESULT CMP4SplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	//bool b_HasVideo = false;

	m_trackpos.RemoveAll();

	m_pFile.Free();
	m_pFile.Attach(DNew CMP4SplitterFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;
	REFERENCE_TIME rtVideoDuration = 0;

	CSize videoSize(0, 0);

	int nRotation		= 0;
	int ChapterTrackId	= INT_MIN;

	if (AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie()) {
		// looking for main video track (skip tracks with motionless frames)
		AP4_UI32 mainvideoID = 0;
		for (AP4_List<AP4_Track>::Item* item = movie->GetTracks().FirstItem();
				item;
				item = item->GetNext()) {
			AP4_Track* track = item->GetData();

			if (track->GetType() != AP4_Track::TYPE_VIDEO) {
				continue;
			}
			if (!mainvideoID) {
				mainvideoID = track->GetId();
			}
			if (AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss"))) {
				const AP4_Array<AP4_UI32>& entries = stss->GetEntries();
				if (entries.ItemCount() > 0) {
					mainvideoID = track->GetId();
					break;
				}
			}
		}
		// process the tracks
		for (AP4_List<AP4_Track>::Item* item = movie->GetTracks().FirstItem();
				item;
				item = item->GetNext()) {
			AP4_Track* track = item->GetData();

			if (track->GetType() != AP4_Track::TYPE_VIDEO
					&& track->GetType() != AP4_Track::TYPE_AUDIO
					&& track->GetType() != AP4_Track::TYPE_TEXT
					&& track->GetType() != AP4_Track::TYPE_SUBP) {
				continue;
			}

			//if (b_HasVideo && track->GetType() == AP4_Track::TYPE_VIDEO) {
			if (track->GetType() == AP4_Track::TYPE_VIDEO && track->GetId() != mainvideoID) {
				continue;
			}

			AP4_Sample sample;

			if (!AP4_SUCCEEDED(track->GetSample(0, sample)) || sample.GetDescriptionIndex() == 0xFFFFFFFF) {
				continue;
			}

			if (AP4_ChapAtom* chap = dynamic_cast<AP4_ChapAtom*>(track->GetTrakAtom()->FindChild("tref/chap"))) {
				ChapterTrackId = chap->GetChapterTrackId();
			}

			if (ChapterTrackId == track->GetId()) {
				continue;
			}

			if (track->GetType() == AP4_Track::TYPE_VIDEO && !nRotation) {
				if (AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD))) {
					nRotation = tkhd->GetRotation();
					if (nRotation) {
						CString prop;
						prop.Format(_T("%d"), nRotation);
						SetProperty(L"ROTATION", prop);
					}
				}
			}

			CSize Aspect(0, 0);
			AP4_UI32 width = 0;
			AP4_UI32 height = 0;

			CString TrackName = ConvertStr(track->GetTrackName().c_str());
			if (TrackName.GetLength() && TrackName[0] < 0x20) {
				TrackName.Delete(0);
			}
			TrackName.Trim();

			CStringA TrackLanguage = track->GetTrackLanguage().c_str();

			REFERENCE_TIME AvgTimePerFrame = 0;
			if (track->GetType() == AP4_Track::TYPE_VIDEO) {
				if (AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD))) {
					width = tkhd->GetWidth() >> 16;
					height = tkhd->GetHeight() >> 16;
					double num = 0;
					double den = 0;
					tkhd->GetAspect(num, den);
					if (num > 0 && den > 0) {
						Aspect = ReduceDim(num/den);
					}
				}

				AvgTimePerFrame = track->GetSampleCount() ? track->GetDurationMs() * 10000 / (track->GetSampleCount()) : 0;
				if (AP4_SttsAtom* stts = dynamic_cast<AP4_SttsAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stts"))) {
					AP4_Duration totalDuration	= stts->GetTotalDuration();
					AP4_UI32 totalFrames		= stts->GetTotalFrames();
					if (totalDuration && totalFrames) {
						AvgTimePerFrame = 10000000.0 / track->GetMediaTimeScale() * totalDuration / totalFrames;
					}
				}
			}

			CAtlArray<CMediaType> mts;

			CMediaType mt;
			mt.SetSampleSize(1);

			VIDEOINFOHEADER2* vih2	= NULL;
			WAVEFORMATEX* wfe		= NULL;

			AP4_DataBuffer empty;

			if (AP4_SampleDescription* desc = track->GetSampleDescription(sample.GetDescriptionIndex())) {
				AP4_MpegSampleDescription* mpeg_desc = NULL;

				if (desc->GetType() == AP4_SampleDescription::TYPE_MPEG) {
					mpeg_desc = dynamic_cast<AP4_MpegSampleDescription*>(desc);
				} else if (desc->GetType() == AP4_SampleDescription::TYPE_ISMACRYP) {
					AP4_IsmaCrypSampleDescription* isma_desc = dynamic_cast<AP4_IsmaCrypSampleDescription*>(desc);
					mpeg_desc = isma_desc->GetOriginalSampleDescription();
				}

				if (mpeg_desc) {
					CStringW TypeString = CStringW(mpeg_desc->GetObjectTypeString(mpeg_desc->GetObjectTypeId()));
					if ((TypeString.Find(_T("UNKNOWN")) == -1) && (TypeString.Find(_T("INVALID")) == -1)) {
						SetTrackName(&TrackName, TypeString);
					}
				}

				if (AP4_MpegVideoSampleDescription* video_desc =
							dynamic_cast<AP4_MpegVideoSampleDescription*>(mpeg_desc)) {
					const AP4_DataBuffer* di = video_desc->GetDecoderInfo();
					if (!di) {
						di = &empty;
					}

					LONG biWidth = (LONG)video_desc->GetWidth();
					LONG biHeight = (LONG)video_desc->GetHeight();

					if (!biWidth || !biHeight) {
						biWidth = width;
						biHeight = height;
					}

					if (!biWidth || !biHeight) {
						continue;
					}

					mt.majortype	= MEDIATYPE_Video;
					mt.formattype	= FORMAT_VideoInfo2;

					vih2						= (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2) + di->GetDataSize());
					memset(vih2, 0, mt.FormatLength());
					vih2->dwBitRate				= video_desc->GetAvgBitrate()/8;
					vih2->bmiHeader.biSize		= sizeof(vih2->bmiHeader);
					vih2->bmiHeader.biWidth		= biWidth;
					vih2->bmiHeader.biHeight	= biHeight;
					vih2->rcSource				= vih2->rcTarget = CRect(0, 0, biWidth, biHeight);
					vih2->AvgTimePerFrame		= AvgTimePerFrame;

					SetAspect(Aspect, width, height, vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight, vih2);

					memcpy(vih2 + 1, di->GetData(), di->GetDataSize());

					switch (video_desc->GetObjectTypeId()) {
						case AP4_MPEG4_VISUAL_OTI:
							{
								BYTE* data			= (BYTE*)di->GetData();
								size_t size			= (size_t)di->GetDataSize();

								BITMAPINFOHEADER pbmi;
								memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
								pbmi.biSize			= sizeof(pbmi);
								pbmi.biWidth		= biWidth;
								pbmi.biHeight		= biHeight;
								pbmi.biCompression	= FCC('mp4v');
								pbmi.biPlanes		= 1;
								pbmi.biBitCount		= 24;
								pbmi.biSizeImage	= DIBSIZE(pbmi);

								CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, Aspect, data, size);
								mt.SetSampleSize(pbmi.biSizeImage);
								mts.Add(mt);

								MPEG2VIDEOINFO* mvih	= (MPEG2VIDEOINFO*)mt.pbFormat;
								mt.subtype				= FOURCCMap(mvih->hdr.bmiHeader.biCompression = 'V4PM');
								mts.Add(mt);
							}
							break;
						case AP4_JPEG_OTI:
							{
								BYTE* data			= (BYTE*)di->GetData();
								size_t size			= (size_t)di->GetDataSize();

								BITMAPINFOHEADER pbmi;
								memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
								pbmi.biSize			= sizeof(pbmi);
								pbmi.biWidth		= biWidth;
								pbmi.biHeight		= biHeight;
								pbmi.biCompression	= FCC('jpeg');
								pbmi.biPlanes		= 1;
								pbmi.biBitCount		= 24;
								pbmi.biSizeImage	= DIBSIZE(pbmi);

								CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, Aspect, data, size);
								mt.SetSampleSize(pbmi.biSizeImage);
								mts.Add(mt);
							}
							break;
						case AP4_MPEG2_VISUAL_SIMPLE_OTI:
						case AP4_MPEG2_VISUAL_MAIN_OTI:
						case AP4_MPEG2_VISUAL_SNR_OTI:
						case AP4_MPEG2_VISUAL_SPATIAL_OTI:
						case AP4_MPEG2_VISUAL_HIGH_OTI:
						case AP4_MPEG2_VISUAL_422_OTI:
							mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
							{
								m_pFile->Seek(sample.GetOffset());
								CBaseSplitterFileEx::seqhdr h;
								CMediaType mt2;
								if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
									mt = mt2;
								}
							}
							mts.Add(mt);
							break;
						case AP4_MPEG1_VISUAL_OTI:
							mt.subtype = MEDIASUBTYPE_MPEG1Payload;
							{
								m_pFile->Seek(sample.GetOffset());
								CBaseSplitterFileEx::seqhdr h;
								CMediaType mt2;
								if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
									mt = mt2;
								}
							}
							mts.Add(mt);
							break;
					}

					if (mt.subtype == GUID_NULL) {
						TRACE(_T("Unknown video OBI: %02x\n"), video_desc->GetObjectTypeId());
					}
				} else if (AP4_MpegAudioSampleDescription* audio_desc =
							   dynamic_cast<AP4_MpegAudioSampleDescription*>(mpeg_desc)) {
					const AP4_DataBuffer* di = audio_desc->GetDecoderInfo();
					if (!di) {
						di = &empty;
					}

					mt.majortype	= MEDIATYPE_Audio;
					mt.formattype	= FORMAT_WaveFormatEx;

					wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + di->GetDataSize());
					memset(wfe, 0, mt.FormatLength());
					wfe->nSamplesPerSec		= audio_desc->GetSampleRate();
					if (!wfe->nSamplesPerSec) {
						wfe->nSamplesPerSec	= track->GetMediaTimeScale();
					}
					wfe->nAvgBytesPerSec	= audio_desc->GetAvgBitrate()/8;
					wfe->nChannels			= audio_desc->GetChannelCount();
					wfe->wBitsPerSample		= audio_desc->GetSampleSize();
					wfe->cbSize				= (WORD)di->GetDataSize();
					wfe->nBlockAlign		= (WORD)((wfe->nChannels * wfe->wBitsPerSample) / 8);

					memcpy(wfe + 1, di->GetData(), di->GetDataSize());

					switch (audio_desc->GetObjectTypeId()) {
						case AP4_MPEG4_AUDIO_OTI:
						case AP4_MPEG2_AAC_AUDIO_MAIN_OTI:
						case AP4_MPEG2_AAC_AUDIO_LC_OTI:
						case AP4_MPEG2_AAC_AUDIO_SSRP_OTI:
							if (di->GetDataSize() > 10) {
								if (GETDWORD(di->GetData()+6) == 0x00534c41) { // 'ALS\0' sync word
									wfe->wFormatTag = WAVE_FORMAT_UNKNOWN;
									mt.subtype = FOURCCMap(MAKEFOURCC('A','L','S',' ')); // create our own GUID - {20534C41-0000-0010-8000-00AA00389B71}
									mts.Add(mt);
									break;
								}
							}
							mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_RAW_AAC1);
							if (wfe->cbSize >= 2) {
								WORD Channels = (((BYTE*)(wfe+1))[1]>>3) & 0xf;
								if (Channels) {
									wfe->nChannels		= Channels;
									wfe->nBlockAlign	= (WORD)((wfe->nChannels * wfe->wBitsPerSample) / 8);
								}
							}
							mts.Add(mt);
							break;
						case AP4_MPEG2_PART3_AUDIO_OTI:
						case AP4_MPEG1_AUDIO_OTI:
							mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MPEGLAYER3);
							{
								m_pFile->Seek(sample.GetOffset());
								CBaseSplitterFileEx::mpahdr h;
								CMediaType mt2;
								if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
									mt = mt2;
								}
							}
							mts.Add(mt);
							break;
						case AP4_DTSC_AUDIO_OTI:
						case AP4_DTSH_AUDIO_OTI:
						case AP4_DTSL_AUDIO_OTI:
							mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_DTS2);
							{
								m_pFile->Seek(sample.GetOffset());
								CBaseSplitterFileEx::dtshdr h;
								CMediaType mt2;
								if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
									mt = mt2;
								}
							}
							mts.Add(mt);
							break;
						case AP4_VORBIS_OTI:
							CreateVorbisMediaType(mt, mts,
												  (DWORD)audio_desc->GetChannelCount(),
												  (DWORD)(audio_desc->GetSampleRate() ? audio_desc->GetSampleRate() : track->GetMediaTimeScale()),
												  (DWORD)audio_desc->GetSampleSize(), di->GetData(), di->GetDataSize());
							break;
					}

					if (mt.subtype == GUID_NULL) {
						TRACE(_T("Unknown audio OBI: %02x\n"), audio_desc->GetObjectTypeId());
					}
				} else if (AP4_MpegSystemSampleDescription* system_desc =
							   dynamic_cast<AP4_MpegSystemSampleDescription*>(desc)) {
					const AP4_DataBuffer* di = system_desc->GetDecoderInfo();
					if (!di) {
						di = &empty;
					}

					switch (system_desc->GetObjectTypeId()) {
						case AP4_NERO_VOBSUB:
							if (di->GetDataSize() >= 16 * 4) {
								CSize size(videoSize);
								if (AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD))) {
									if (tkhd->GetWidth() && tkhd->GetHeight()) {
										size.cx = tkhd->GetWidth() >> 16;
										size.cy = tkhd->GetHeight() >> 16;
									}
								}

								if (size == CSize(0, 0)) {
									size = CSize(720, 576);
								}

								const AP4_Byte* pal = di->GetData();
								CAtlList<CStringA> sl;
								for (int i = 0; i < 16 * 4; i += 4) {
									BYTE y = (pal[i + 1] - 16) * 255 / 219;
									BYTE u = pal[i + 2];
									BYTE v = pal[i + 3];
									BYTE r = (BYTE)min(max(1.0 * y + 1.4022 * (v - 128), 0), 255);
									BYTE g = (BYTE)min(max(1.0 * y - 0.3456 * (u - 128) - 0.7145 * (v - 128), 0), 255);
									BYTE b = (BYTE)min(max(1.0 * y + 1.7710 * (u - 128), 0) , 255);
									CStringA str;
									str.Format("%02x%02x%02x", r, g, b);
									sl.AddTail(str);
								}

								CStringA hdr;
								hdr.Format(
									"# VobSub index file, v7 (do not modify this line!)\n"
									"size: %dx%d\n"
									"palette: %s\n",
									size.cx, size.cy,
									Implode(sl, ','));

								mt.majortype	= MEDIATYPE_Subtitle;
								mt.subtype		= MEDIASUBTYPE_VOBSUB;
								mt.formattype	= FORMAT_SubtitleInfo;

								SUBTITLEINFO* psi	= (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + hdr.GetLength());
								memset(psi, 0, mt.FormatLength());
								psi->dwOffset		= sizeof(SUBTITLEINFO);
								strncpy_s(psi->IsoLang, TrackLanguage, _countof(psi->IsoLang) - 1);
								wcsncpy_s(psi->TrackName, TrackName, _countof(psi->TrackName) - 1);
								memcpy(psi + 1, (LPCSTR)hdr, hdr.GetLength());
								mts.Add(mt);
							}
							break;
					}

					if (mt.subtype == GUID_NULL) {
						TRACE(_T("Unknown audio OBI: %02x\n"), system_desc->GetObjectTypeId());
					}
				} else if (AP4_UnknownSampleDescription* unknown_desc =
							   dynamic_cast<AP4_UnknownSampleDescription*>(desc)) { // TEMP
					AP4_SampleEntry* sample_entry = unknown_desc->GetSampleEntry();

					if (dynamic_cast<AP4_TextSampleEntry*>(sample_entry)
							|| dynamic_cast<AP4_Tx3gSampleEntry*>(sample_entry)) {
						mt.majortype = MEDIATYPE_Subtitle;
						mt.subtype = MEDIASUBTYPE_UTF8;
						mt.formattype = FORMAT_SubtitleInfo;
						SUBTITLEINFO* si = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO));
						memset(si, 0, mt.FormatLength());
						si->dwOffset = sizeof(SUBTITLEINFO);
						strcpy_s(si->IsoLang, _countof(si->IsoLang), CStringA(TrackLanguage));
						wcscpy_s(si->TrackName, _countof(si->TrackName), TrackName);
						mts.Add(mt);
					}
				}
			} else if (AP4_Avc1SampleEntry* avc1 = dynamic_cast<AP4_Avc1SampleEntry*>(
					track->GetTrakAtom()->FindChild("mdia/minf/stbl/stsd/avc1"))) {
				if (AP4_AvcCAtom* avcC = dynamic_cast<AP4_AvcCAtom*>(avc1->GetChild(AP4_ATOM_TYPE_AVCC))) {
					SetTrackName(&TrackName, _T("MPEG4 Video (H264)"));

					const AP4_DataBuffer* di = avcC->GetDecoderInfo();
					if (!di) {
						di = &empty;
					}

					BYTE* data			= (BYTE*)di->GetData();
					size_t size			= (size_t)di->GetDataSize();

					BITMAPINFOHEADER pbmi;
					memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
					pbmi.biSize			= sizeof(pbmi);
					pbmi.biWidth		= (LONG)avc1->GetWidth();
					pbmi.biHeight		= (LONG)avc1->GetHeight();
					pbmi.biCompression	= '1CVA';
					pbmi.biPlanes		= 1;
					pbmi.biBitCount		= 24;
					pbmi.biSizeImage	= DIBSIZE(pbmi);

					HandlePASP(avc1);
					SetAspect(Aspect, width, height, pbmi.biWidth, pbmi.biHeight, vih2);

					CreateMPEG2VIfromAVC(&mt, &pbmi, AvgTimePerFrame, Aspect, data, size);
					mt.SetSampleSize(pbmi.biSizeImage);

					mts.Add(mt);
					//b_HasVideo = true;
				}
			} else if (AP4_Hvc1SampleEntry* hvc1 = dynamic_cast<AP4_Hvc1SampleEntry*>(
					track->GetTrakAtom()->FindChild("mdia/minf/stbl/stsd/hvc1"))) {
				if (AP4_HvcCAtom* hvcC = dynamic_cast<AP4_HvcCAtom*>(hvc1->GetChild(AP4_ATOM_TYPE_HVCC))) {
					SetTrackName(&TrackName, _T("HEVC Video (H.265)"));

					const AP4_DataBuffer* di = hvcC->GetDecoderInfo();
					if (!di) {
						di = &empty;
					}

					BYTE* data			= (BYTE*)di->GetData();
					size_t size			= (size_t)di->GetDataSize();

					BITMAPINFOHEADER pbmi;
					memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
					pbmi.biSize			= sizeof(pbmi);
					pbmi.biWidth		= (LONG)hvc1->GetWidth();
					pbmi.biHeight		= (LONG)hvc1->GetHeight();
					pbmi.biCompression	= FCC('HVC1');
					pbmi.biPlanes		= 1;
					pbmi.biBitCount		= 24;
					pbmi.biSizeImage	= DIBSIZE(pbmi);

					HandlePASP(hvc1);
					SetAspect(Aspect, width, height, pbmi.biWidth, pbmi.biHeight, vih2);

					CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, Aspect, data, size);
					mt.SetSampleSize(pbmi.biSizeImage);

					vc_params_t params;
					if (HEVCParser::ParseHEVCDecoderConfigurationRecord(data, size, params, false)) {
						MPEG2VIDEOINFO* pm2vi	= (MPEG2VIDEOINFO*)mt.pbFormat;
						pm2vi->dwProfile		= params.profile;
						pm2vi->dwLevel			= params.level;
						pm2vi->dwFlags			= params.nal_length_size;
					}
					mts.Add(mt);
				}
			} else if (AP4_StsdAtom* stsd = dynamic_cast<AP4_StsdAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stsd"))) {
				const AP4_DataBuffer& db = stsd->GetDataBuffer();

				int k = 0;
				for (AP4_List<AP4_Atom>::Item* items = stsd->GetChildren().FirstItem();
						items;
						items = items->GetNext(), ++k) {

					AP4_Atom* atom = items->GetData();
					AP4_Atom::Type type = atom->GetType();

					if (k == 0 && stsd->GetChildren().ItemCount() > 1 && type == AP4_ATOM_TYPE_JPEG) {
						continue; // Multiple fourcc, we skip first JPEG.
					}

					DWORD fourcc =
						((type >> 24) & 0x000000ff) |
						((type >>  8) & 0x0000ff00) |
						((type <<  8) & 0x00ff0000) |
						((type << 24) & 0xff000000);

					if (AP4_VisualSampleEntry* vse = dynamic_cast<AP4_VisualSampleEntry*>(atom)) {
						AP4_DataBuffer* pData = (AP4_DataBuffer*)&db;


						char buff[5];
						memcpy(buff, &fourcc, 4);
						buff[4] = 0;

						CString tname = UTF8To16(vse->GetCompressorName());
						if ((buff[0] == 'x' || buff[0] == 'h') && buff[1] == 'd') {
							// Apple HDV/XDCAM
							FormatTrackName(_T("HDV/XDV MPEG2"), 0);

							m_pFile->Seek(sample.GetOffset());
							CBaseSplitterFileEx::seqhdr h;
							CMediaType mt2;
							if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
								mt = mt2;
							}
							mts.Add(mt);

							break;
						} else if (type == AP4_ATOM_TYPE_MJPA || type == AP4_ATOM_TYPE_MJPB || type == AP4_ATOM_TYPE_MJPG) {
							FormatTrackName(_T("M-Jpeg"), 1);
						} else if (type == AP4_ATOM_TYPE_MJP2) {
							FormatTrackName(_T("M-Jpeg 2000"), 1);
						} else if (type == AP4_ATOM_TYPE_APCN ||
								   type == AP4_ATOM_TYPE_APCH ||
								   type == AP4_ATOM_TYPE_APCO ||
								   type == AP4_ATOM_TYPE_APCS ||
								   type == AP4_ATOM_TYPE_AP4H) {
							FormatTrackName(_T("Apple ProRes"), 0);
						} else if (type == AP4_ATOM_TYPE_SVQ1 ||
								   type == AP4_ATOM_TYPE_SVQ2 ||
								   type == AP4_ATOM_TYPE_SVQ3) {
							FormatTrackName(_T("Sorenson"), 0);
						} else if (type == AP4_ATOM_TYPE_CVID) {
							FormatTrackName(_T("Cinepack"), 0);
						} else if (type == AP4_ATOM_TYPE_OVC1 || type == AP4_ATOM_TYPE_VC1) {
							fourcc = FCC('WVC1');
							FormatTrackName(_T("VC-1"), 0);
							if (AP4_Dvc1Atom* Dvc1 = dynamic_cast<AP4_Dvc1Atom*>(vse->GetChild(AP4_ATOM_TYPE_DVC1))) {
								pData = (AP4_DataBuffer*)Dvc1->GetDecoderInfo();
							}
						} else if (type == AP4_ATOM_TYPE_RAW) {
							fourcc = BI_RGB;
						}

						AP4_Size ExtraSize = pData->GetDataSize();
						if (type == AP4_ATOM_TYPE_FFV1) {
							ExtraSize = 0;
						}

						BOOL bVC1ExtraData = FALSE;
						if (type == AP4_ATOM_TYPE_OVC1 && ExtraSize > 198) {
							ExtraSize -= 198;
							bVC1ExtraData = TRUE;
						}

						mt.majortype	= MEDIATYPE_Video;
						mt.formattype	= FORMAT_VideoInfo2;

						vih2							= (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2) + ExtraSize);
						memset(vih2, 0, mt.FormatLength());
						vih2->bmiHeader.biSize			= sizeof(vih2->bmiHeader);
						vih2->bmiHeader.biWidth			= (LONG)vse->GetWidth();
						vih2->bmiHeader.biHeight		= (LONG)vse->GetHeight();
						vih2->bmiHeader.biCompression	= fourcc;
						vih2->bmiHeader.biBitCount		= (LONG)vse->GetDepth();
						vih2->bmiHeader.biSizeImage		= DIBSIZE(vih2->bmiHeader);
						vih2->rcSource					= vih2->rcTarget = CRect(0, 0, vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight);
						vih2->AvgTimePerFrame			= AvgTimePerFrame;

						HandlePASP(vse);
						SetAspect(Aspect, width, height, vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight, vih2);

						mt.SetSampleSize(vih2->bmiHeader.biSizeImage);

						if (ExtraSize) {
							if (bVC1ExtraData) {
								memcpy(vih2 + 1, pData->GetData() + 198, ExtraSize);
							} else {
								memcpy(vih2 + 1, pData->GetData(), ExtraSize);
							}
						}

						if (fourcc == BI_RGB) {
							WORD &bitcount = vih2->bmiHeader.biBitCount;
							if (bitcount == 16) {
								mt.subtype = MEDIASUBTYPE_RGB555;
							} else if (bitcount == 24) {
								mt.subtype = MEDIASUBTYPE_RGB24;
							} else if (bitcount == 32) {
								mt.subtype = MEDIASUBTYPE_ARGB32;
							} else {
								break;
							}
							mts.Add(mt);
							break;
						}
						mt.subtype = FOURCCMap(fourcc);
						mts.Add(mt);

						if (mt.subtype == MEDIASUBTYPE_WVC1) {
							mt.subtype = MEDIASUBTYPE_WVC1_CYBERLINK;
							mts.InsertAt(0, mt);
						}

						_strlwr_s(buff);
						AP4_Atom::Type typelwr = *(AP4_Atom::Type*)buff;

						if (typelwr != fourcc) {
							mt.subtype = FOURCCMap(vih2->bmiHeader.biCompression = typelwr);
							mts.Add(mt);
							//b_HasVideo = true;
						}

						_strupr_s(buff);
						AP4_Atom::Type typeupr = *(AP4_Atom::Type*)buff;

						if (typeupr != fourcc) {
							mt.subtype = FOURCCMap(vih2->bmiHeader.biCompression = typeupr);
							mts.Add(mt);
							//b_HasVideo = true;
						}

						if (vse->m_hasPalette) {
							track->SetPalette(vse->GetPalette());
						}

						break;
					} else if (AP4_AudioSampleEntry* ase = dynamic_cast<AP4_AudioSampleEntry*>(atom)) {
						DWORD samplerate    = ase->GetSampleRate();
						WORD  channels      = ase->GetChannelCount();
						WORD  bitspersample = ase->GetSampleSize();

						// overwrite audio fourc
						if ((type & 0xffff0000) == AP4_ATOM_TYPE('m', 's', 0, 0)) {
							fourcc = type & 0xffff;
						} else if (type == AP4_ATOM_TYPE_ALAW) {
							fourcc = WAVE_FORMAT_ALAW;
							SetTrackName(&TrackName, _T("PCM A-law"));
						} else if (type == AP4_ATOM_TYPE_ULAW) {
							fourcc = WAVE_FORMAT_MULAW;
							SetTrackName(&TrackName, _T("PCM mu-law"));
						} else if (type == AP4_ATOM_TYPE__MP3) {
							SetTrackName(&TrackName, _T("MPEG Audio (MP3)"));
							fourcc = WAVE_FORMAT_MPEGLAYER3;
						} else if ((type == AP4_ATOM_TYPE__AC3) || (type == AP4_ATOM_TYPE_SAC3) || (type == AP4_ATOM_TYPE_EAC3)) {
							if (type == AP4_ATOM_TYPE_EAC3) {
								SetTrackName(&TrackName, _T("Enhanced AC-3 audio"));
								fourcc = WAVE_FORMAT_UNKNOWN;
							} else {
								fourcc = WAVE_FORMAT_DOLBY_AC3;
								SetTrackName(&TrackName, _T("AC-3 Audio"));
							}
						} else if (type == AP4_ATOM_TYPE_MP4A) {
							fourcc = WAVE_FORMAT_RAW_AAC1;
							SetTrackName(&TrackName, _T("MPEG-2 Audio AAC"));
						} else if (type == AP4_ATOM_TYPE_NMOS) {
							fourcc = MAKEFOURCC('N','E','L','L');
							SetTrackName(&TrackName, _T("NellyMoser Audio"));
						} else if (type == AP4_ATOM_TYPE_ALAC) {
							fourcc = MAKEFOURCC('a','l','a','c');
							SetTrackName(&TrackName, _T("ALAC Audio"));
						} else if (type == AP4_ATOM_TYPE_QDM2) {
							SetTrackName(&TrackName, _T("QDesign Music 2"));
						} else if (type == AP4_ATOM_TYPE_SPEX) {
							fourcc = 0xa109;
							SetTrackName(&TrackName, _T("Speex"));
						} else if ((type == AP4_ATOM_TYPE_NONE || type == AP4_ATOM_TYPE_RAW) && bitspersample == 8 ||
								    type == AP4_ATOM_TYPE_SOWT && bitspersample == 16 ||
								   (type == AP4_ATOM_TYPE_IN24 || type == AP4_ATOM_TYPE_IN32) && ase->GetEndian()==ENDIAN_LITTLE) {
							fourcc = type = WAVE_FORMAT_PCM;
						} else if ((type == AP4_ATOM_TYPE_FL32 || type == AP4_ATOM_TYPE_FL64) && ase->GetEndian()==ENDIAN_LITTLE) {
							fourcc = type = WAVE_FORMAT_IEEE_FLOAT;
						} else if (type == AP4_ATOM_TYPE_LPCM) {
							SetTrackName(&TrackName, _T("LPCM"));
							DWORD flags = ase->GetFormatSpecificFlags();
							if (flags & 2) { // big endian
								if (flags & 1) { // floating point
									if      (bitspersample == 32) type = AP4_ATOM_TYPE_FL32;
									else if (bitspersample == 64) type = AP4_ATOM_TYPE_FL64;
								} else {
									if      (bitspersample == 16) type = AP4_ATOM_TYPE_TWOS;
									else if (bitspersample == 24) type = AP4_ATOM_TYPE_IN24;
									else if (bitspersample == 32) type = AP4_ATOM_TYPE_IN32;
								}
								fourcc = ((type >> 24) & 0x000000ff) |
										 ((type >>  8) & 0x0000ff00) |
										 ((type <<  8) & 0x00ff0000) |
										 ((type << 24) & 0xff000000);
							} else {         // little endian
								if (flags & 1) { // floating point
									fourcc = type = WAVE_FORMAT_IEEE_FLOAT;
								} else {
									fourcc = type = WAVE_FORMAT_PCM;
								}
							}
						}

						if (type == AP4_ATOM_TYPE_NONE ||
							type == AP4_ATOM_TYPE_TWOS ||
							type == AP4_ATOM_TYPE_SOWT ||
							type == AP4_ATOM_TYPE_IN24 ||
							type == AP4_ATOM_TYPE_IN32 ||
							type == AP4_ATOM_TYPE_FL32 ||
							type == AP4_ATOM_TYPE_FL64 ||
							type == WAVE_FORMAT_PCM    ||
							type == WAVE_FORMAT_IEEE_FLOAT) {
							SetTrackName(&TrackName, _T("PCM"));
						}

						DWORD nAvgBytesPerSec = 0;
						if (type == AP4_ATOM_TYPE_EAC3) {
							AP4_Sample sample;
							AP4_DataBuffer sample_data;

							if (track->GetSampleCount() && AP4_SUCCEEDED(track->ReadSample(1, sample, sample_data)) && sample_data.GetDataSize() >= 7) {
								audioframe_t aframe;
								if (ParseEAC3Header(sample_data.GetData(), &aframe)) {
									samplerate		= aframe.samplerate;
									channels		= aframe.channels;
									nAvgBytesPerSec	= (aframe.size * 8i64 * samplerate / aframe.samples + 4) /8;
								}
							}
						}

						mt.majortype = MEDIATYPE_Audio;
						mt.formattype = FORMAT_WaveFormatEx;
						wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
						memset(wfe, 0, mt.FormatLength());
						if (!(fourcc & 0xffff0000)) {
							wfe->wFormatTag = (WORD)fourcc;
						}
						wfe->nSamplesPerSec		= samplerate;
						wfe->nChannels			= channels;
						wfe->wBitsPerSample		= bitspersample;
						wfe->nBlockAlign		= ase->GetBytesPerFrame();
						wfe->nAvgBytesPerSec	= nAvgBytesPerSec ? nAvgBytesPerSec : (ase->GetSamplesPerPacket() ? wfe->nSamplesPerSec * wfe->nBlockAlign / ase->GetSamplesPerPacket() : 0);

						mt.subtype = FOURCCMap(fourcc);
						if (type == AP4_ATOM_TYPE_EAC3) {
							mt.subtype = MEDIASUBTYPE_DOLBY_DDPLUS;
						}

						if (type == AP4_ATOM_TYPE('m', 's', 0x00, 0x02)) {
							const WORD numcoef = 7;
							static ADPCMCOEFSET coef[] = { {256, 0}, {512, -256}, {0,0}, {192,64}, {240,0}, {460, -208}, {392,-232} };
							const ULONG size = sizeof(ADPCMWAVEFORMAT) + (numcoef * sizeof(ADPCMCOEFSET));
							ADPCMWAVEFORMAT* format = (ADPCMWAVEFORMAT*)mt.ReallocFormatBuffer(size);
							if (format != NULL) {
								format->wfx.wFormatTag = WAVE_FORMAT_ADPCM;
								format->wfx.wBitsPerSample = 4;
								format->wfx.cbSize = (WORD)(size - sizeof(WAVEFORMATEX));
								format->wSamplesPerBlock = format->wfx.nBlockAlign * 2 / format->wfx.nChannels - 12;
								format->wNumCoef = numcoef;
								memcpy( format->aCoef, coef, sizeof(coef) );
							}
						} else if (type == AP4_ATOM_TYPE('m', 's', 0x00, 0x11)) {
							IMAADPCMWAVEFORMAT* format = (IMAADPCMWAVEFORMAT*)mt.ReallocFormatBuffer(sizeof(IMAADPCMWAVEFORMAT));
							if (format != NULL) {
								format->wfx.wFormatTag = WAVE_FORMAT_IMA_ADPCM;
								format->wfx.wBitsPerSample = 4;
								format->wfx.cbSize = (WORD)(sizeof(IMAADPCMWAVEFORMAT) - sizeof(WAVEFORMATEX));
								int X = (format->wfx.nBlockAlign - (4 * format->wfx.nChannels)) * 8;
								int Y = format->wfx.wBitsPerSample * format->wfx.nChannels;
								format->wSamplesPerBlock = (X / Y) + 1;
							}
						} else if (type == AP4_ATOM_TYPE_ALAC) {
							const AP4_Byte* data = db.GetData();
							AP4_Size size = db.GetDataSize();

							while (size >= 36) {
								if ((GETDWORD(data) == 0x24000000) && (GETDWORD(data+4) == 0x63616c61)) {
									break;
								}
								size--;
								data++;
							}

							if (size >= 36) {
								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 36);
								wfe->cbSize = 36;
								memcpy(wfe+1, data, 36);
							}
						} else if (type == WAVE_FORMAT_PCM) {
							mt.SetSampleSize(wfe->nBlockAlign);
							if (channels > 2 || bitspersample > 16) {
								WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
								if (wfex != NULL) {
									wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
									wfex->Format.cbSize = 22;
									wfex->Samples.wValidBitsPerSample = bitspersample;
									wfex->dwChannelMask = GetDefChannelMask(channels); // TODO: get correct channel mask from mov file
									wfex->SubFormat = MEDIASUBTYPE_PCM;
								}
							}
						} else if (type == WAVE_FORMAT_IEEE_FLOAT) {
							mt.SetSampleSize(wfe->nBlockAlign);
							if (channels > 2) {
								WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
								if (wfex != NULL) {
									wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
									wfex->Format.cbSize = 22;
									wfex->Samples.wValidBitsPerSample = bitspersample;
									wfex->dwChannelMask = GetDefChannelMask(channels); // TODO: get correct channel mask from mov file
									wfex->SubFormat = MEDIASUBTYPE_IEEE_FLOAT;
								}
							}
						} else if (type == AP4_ATOM_TYPE_MP4A ||
								   type == AP4_ATOM_TYPE_ALAW ||
								   type == AP4_ATOM_TYPE_ULAW) {
							// not need any extra data for ALAW, ULAW
							// also extra data is not required for IMA4, MAC3, MAC6
						} else if (type == AP4_ATOM_TYPE_OWMA && db.GetDataSize() > 36) {
							// skip unknown first 36 bytes
							const AP4_Size size = db.GetDataSize() - 36;
							const AP4_Byte* pData = db.GetData() + 36;

							wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(size);
							memcpy(wfe, pData, size);
								
							mt.subtype = FOURCCMap(wfe->wFormatTag);
						} else if (db.GetDataSize() > 0) {
							//always needed extra data for QDM2
							wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + db.GetDataSize());
							wfe->cbSize = db.GetDataSize();
							memcpy(wfe + 1, db.GetData(), db.GetDataSize());
						}

						mts.Add(mt);

						if (AP4_WfexAtom* WfexAtom = dynamic_cast<AP4_WfexAtom*>(ase->GetChild(AP4_ATOM_TYPE_WFEX))) {
							const AP4_DataBuffer* di = WfexAtom->GetDecoderInfo();
							if (di && di->GetDataSize()) {
								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(di->GetDataSize());
								memcpy(wfe, di->GetData(), di->GetDataSize());
								
								mt.subtype = FOURCCMap(wfe->wFormatTag);
								mts.InsertAt(0, mt);
							}
						}

						break;
					} else {
						TRACE(_T("Unknow MP4 Stream %x\n") , fourcc);
					}
				}
			}

			if (mts.IsEmpty()) {
				continue;
			}

			REFERENCE_TIME rtDuration = 10000i64 * track->GetDurationMs();
			if (m_rtDuration < rtDuration) {
				m_rtDuration = rtDuration;
			}
			if (rtVideoDuration < rtDuration && AP4_Track::TYPE_VIDEO == track->GetType())
				rtVideoDuration = rtDuration; // get the max video duration

			DWORD id = track->GetId();

			CStringW name, lang;
			name.Format(L"Output %d", id);

			if (!TrackName.IsEmpty()) {
				name = TrackName;
			}

			if (!TrackLanguage.IsEmpty()) {
				if (TrackLanguage != L"und") {
					name += " (" + TrackLanguage + ")";
				}
			}

			if (AP4_Track::TYPE_VIDEO == track->GetType() && videoSize == CSize(0, 0)) {
				for (int i = 0, j = mts.GetCount(); i < j; ++i) {
					BITMAPINFOHEADER bih;
					if (ExtractBIH(&mts[i], &bih)) {
						videoSize.cx = bih.biWidth;
						videoSize.cy = abs(bih.biHeight);
						break;
					}
				}
			}

			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CMP4SplitterOutputPin(mts, name, this, this, &hr));

			if (!TrackName.IsEmpty()) {
				pPinOut->SetProperty(L"NAME", TrackName);
			}
			if (!TrackLanguage.IsEmpty()) {
				pPinOut->SetProperty(L"LANG", CStringW(TrackLanguage));
			}

			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pPinOut)));

			m_trackpos[id] = trackpos();
		}

		if (AP4_ChplAtom* chpl = dynamic_cast<AP4_ChplAtom*>(movie->GetMoovAtom()->FindChild("udta/chpl"))) {
			AP4_Array<AP4_ChplAtom::AP4_Chapter>& chapters = chpl->GetChapters();

			for (AP4_Cardinal i = 0; i < chapters.ItemCount(); ++i) {
				AP4_ChplAtom::AP4_Chapter& chapter = chapters[i];

				CString ChapterName = ConvertStr(chapter.Name.c_str());

				ChapAppend(chapter.Time, ChapterName);
			}
		} else if (ChapterTrackId != INT_MIN) {
			for (AP4_List<AP4_Track>::Item* item = movie->GetTracks().FirstItem();
					item;
					item = item->GetNext()) {

				AP4_Track* track = item->GetData();

				if (ChapterTrackId == track->GetId()) {

					char buff[256] = { 0 };
					for (AP4_Cardinal i = 0; i < track->GetSampleCount(); i++) {
						AP4_Sample sample;
						AP4_DataBuffer data;
						track->ReadSample(i, sample, data);

						const AP4_Byte* ptr	= data.GetData();
						AP4_Size avail		= data.GetDataSize();
						CString ChapterName;

						if (avail > 2) {
							size_t size = (ptr[0] << 8) | ptr[1];
							if (size > avail) size = avail;
							memcpy(buff, &ptr[2], size);
							buff[size] = 0;

							ChapterName = ConvertStr(buff);
							REFERENCE_TIME rtStart	= (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * sample.GetCts());

							ChapAppend(rtStart, ChapterName);
						}
					}

					break;
				}
			}
		}

		if (ChapGetCount()) {
			ChapSort();
		}

		if (AP4_ContainerAtom* ilst = dynamic_cast<AP4_ContainerAtom*>(movie->GetMoovAtom()->FindChild("udta/meta/ilst"))) {
			CStringW title, artist, writer, album, year, appl, desc, gen, track, copyright;

			for (AP4_List<AP4_Atom>::Item* item = ilst->GetChildren().FirstItem();
					item;
					item = item->GetNext()) {
				if (AP4_ContainerAtom* atom = dynamic_cast<AP4_ContainerAtom*>(item->GetData())) {
					if (AP4_DataAtom* data = dynamic_cast<AP4_DataAtom*>(atom->GetChild(AP4_ATOM_TYPE_DATA))) {
						const AP4_DataBuffer* db = data->GetData();

						if (atom->GetType() == AP4_ATOM_TYPE_TRKN) {
							if (db->GetDataSize() >= 4) {
								unsigned short n = (db->GetData()[2] << 8) | db->GetData()[3];
								if (n > 0 && n < 100) {
									track.Format(L"%02d", n);
								} else if (n >= 100) {
									track.Format(L"%d", n);
								}
							}
						} else if (atom->GetType() == AP4_ATOM_TYPE_COVR) {
							if (db->GetDataSize() > 10) {
								DWORD sync = GETDWORD(db->GetData()+6);
								// check for JFIF or Exif sync ...
								if (sync == MAKEFOURCC('J', 'F', 'I', 'F') || sync == MAKEFOURCC('E', 'x', 'i', 'f')) {
									ResAppend(_T("cover.jpg"), _T("cover"), _T("image/jpeg"), (BYTE*)db->GetData(), (DWORD)db->GetDataSize());
								} else {
									sync = GETDWORD(db->GetData());
									// check for PNG sync ...
									if (sync == MAKEFOURCC(0x89, 'P', 'N', 'G')) {
										ResAppend(_T("cover.png"), _T("cover"), _T("image/png"), (BYTE*)db->GetData(), (DWORD)db->GetDataSize());
									}
								}
							}
						} else {
							CStringW str = UTF8To16(CStringA((LPCSTR)db->GetData(), db->GetDataSize()));

							switch (atom->GetType()) {
								case AP4_ATOM_TYPE_NAM:
									title = str;
									break;
								case AP4_ATOM_TYPE_ART:
									artist = str;
									break;
								case AP4_ATOM_TYPE_WRT:
									writer = str;
									break;
								case AP4_ATOM_TYPE_ALB:
									album = str;
									break;
								case AP4_ATOM_TYPE_DAY:
									year = str;
									break;
								case AP4_ATOM_TYPE_TOO:
									appl = str;
									break;
								case AP4_ATOM_TYPE_CMT:
									desc = str;
									break;
								case AP4_ATOM_TYPE_DESC:
									if (desc.IsEmpty()) {
										desc = str;
									}
									break;
								case AP4_ATOM_TYPE_GEN:
									gen = str;
									break;
								case AP4_ATOM_TYPE_CPRT:
									copyright = str;
									break;
							}
						}
					}
				}
			}

			if (!title.IsEmpty()) {
				SetProperty(L"TITL", title);
			}

			if (!album.IsEmpty()) {
				SetProperty(L"ALBUM", album);
			}

			if (!artist.IsEmpty()) {
				SetProperty(L"AUTH", artist);
			} else if (!writer.IsEmpty()) {
				SetProperty(L"AUTH", writer);
			}

			if (!appl.IsEmpty()) {
				SetProperty(L"APPL", appl);
			}

			if (!desc.IsEmpty()) {
				SetProperty(L"DESC", desc);
			}

			if (!copyright.IsEmpty()) {
				SetProperty(L"CPYR", copyright);
			}
		}
	}

	if (rtVideoDuration > 0 && rtVideoDuration < m_rtDuration/2)
		m_rtDuration = rtVideoDuration; // fix incorrect duration

	m_rtNewStop = m_rtStop = m_rtDuration;

	if (m_pOutputs.GetCount()) {
		AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

		POSITION pos = m_trackpos.GetStartPosition();
		while (pos) {
			CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);
			AP4_Track* track = movie->GetTrack(pPair->m_key);

			if (!track->HasIndex()) {
				continue;
			}

			const AP4_Array<AP4_IndexTableEntry>& entries = track->GetIndexEntries();
			for (AP4_Cardinal i = 0; i < entries.ItemCount(); ++i) {
				SyncPoint sp = { entries[i].m_rt, __int64(entries[i].m_offset) };
				m_sps.Add(sp);
			}

			break;
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CMP4SplitterFilter::DemuxInit()
{
	AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

	POSITION pos = m_trackpos.GetStartPosition();
	while (pos) {
		CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

		pPair->m_value.index = 0;
		pPair->m_value.ts = 0;

		AP4_Track* track = movie->GetTrack(pPair->m_key);

		AP4_Sample sample;
		if (AP4_SUCCEEDED(track->GetSample(0, sample))) {
			pPair->m_value.ts = sample.GetCts();
		}
	}

	return true;
}

void CMP4SplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

	POSITION pos = m_trackpos.GetStartPosition();
	while (pos) {
		CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

		AP4_Track* track = movie->GetTrack(pPair->m_key);

		if (track->HasIndex()) {
			if (AP4_FAILED(track->GetIndexForRefTime(rt, pPair->m_value.index, pPair->m_value.ts))) {
				continue;
			}
		} else {
			if (AP4_FAILED(track->GetSampleIndexForRefTime(rt, pPair->m_value.index))) {
				continue;
			}

			AP4_Sample sample;
			if (AP4_SUCCEEDED(track->GetSample(pPair->m_value.index, sample))) {
				pPair->m_value.ts = sample.GetCts();
			}
		}
	}
}

bool CMP4SplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	m_pFile->Seek(0);
	AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

	while (SUCCEEDED(hr) && !CheckRequest(NULL) && (!m_pFile->IsStreaming() || SUCCEEDED(m_pFile->WaitAvailable()))) {

		CAtlMap<DWORD, trackpos>::CPair* pPairNext = NULL;
		REFERENCE_TIME rtNext = 0;

		POSITION pos = m_trackpos.GetStartPosition();
		while (pos) {
			CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

			AP4_Track* track = movie->GetTrack(pPair->m_key);

			CBaseSplitterOutputPin* pPin = GetOutputPin((DWORD)track->GetId());
			if (!pPin->IsConnected()) {
				continue;
			}

			REFERENCE_TIME rt = (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * pPair->m_value.ts);

			if (pPair->m_value.index < track->GetSampleCount() && (!pPairNext || rt < rtNext)) {
				pPairNext = pPair;
				rtNext = rt;
			}
		}

		if (!pPairNext) {
			break;
		}

		AP4_Track* track = movie->GetTrack(pPairNext->m_key);

		CBaseSplitterOutputPin* pPin = GetOutputPin((DWORD)track->GetId());

		AP4_Sample sample;
		AP4_DataBuffer data;

		if (pPin && pPin->IsConnected() && AP4_SUCCEEDED(track->ReadSample(pPairNext->m_value.index, sample, data))) {
			const CMediaType& mt = pPin->CurrentMediaType();

			CAutoPtr<CPacket> p(DNew CPacket());
			p->TrackNumber = (DWORD)track->GetId();
			p->rtStart = (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * sample.GetCts());
			p->rtStop = p->rtStart + (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * sample.GetDuration());
			p->bSyncPoint = sample.IsSync();

			if (track->GetType() == AP4_Track::TYPE_AUDIO && data.GetDataSize() >= 1 && data.GetDataSize() <= 16) {
				WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

				int nBlockAlign;
				if (wfe->nBlockAlign == 0) {
					nBlockAlign = 1200;
				} else if (wfe->nBlockAlign <= 16) { // for PCM (from 8bit mono to 64bit stereo), A-Law, u-Law
					nBlockAlign = wfe->nBlockAlign * (wfe->nSamplesPerSec >> 4); // 1/16s=62.5ms
				} else {
					nBlockAlign = wfe->nBlockAlign;
					//pPairNext->m_value.index -= pPairNext->m_value.index % wfe->nBlockAlign; // if this code is necessary - we need explanations and examples
				}

				p->rtStop = p->rtStart;
				int fFirst = true;

				while (AP4_SUCCEEDED(track->ReadSample(pPairNext->m_value.index, sample, data))) {
					AP4_Size size = data.GetDataSize();
					const AP4_Byte* ptr = data.GetData();

					if (fFirst) {
						p->SetData(ptr, size);
						p->rtStart = p->rtStop = (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * sample.GetCts());
						fFirst = false;
					} else {
						for (int i = 0; i < size; ++i) p->Add(ptr[i]);
					}

					p->rtStop += (REFERENCE_TIME)(10000000.0 / track->GetMediaTimeScale() * sample.GetDuration());

					if (pPairNext->m_value.index + 1 >= track->GetSampleCount() || (int)p->GetCount() >= nBlockAlign) {
						break;
					}

					pPairNext->m_value.index++;
				}
			} else if (track->GetType() == AP4_Track::TYPE_TEXT) {
				const AP4_Byte* ptr = data.GetData();
				AP4_Size avail = data.GetDataSize();

				if (avail > 2) {
					AP4_UI16 size = (ptr[0] << 8) | ptr[1];

					if (size <= avail-2) {
						CStringA str;

						if (size >= 2 && ptr[2] == 0xfe && ptr[3] == 0xff) {
							CStringW wstr = CStringW((LPCWSTR)&ptr[2], size/2);
							for (int i = 0; i < wstr.GetLength(); ++i) {
								wstr.SetAt(i, ((WORD)wstr[i] >> 8) | ((WORD)wstr[i] << 8));
							}
							str = UTF16To8(wstr);
						} else {
							str = CStringA((LPCSTR)&ptr[2], size);
						}

						CStringA dlgln = str;
						dlgln.Replace("\r", "");
						dlgln.Replace("\n", "\\N");

						p->SetData((LPCSTR)dlgln, dlgln.GetLength());
					}
				}
			} else {
				p->SetData(data.GetData(), data.GetDataSize());

				if (track->m_hasPalette) {
					track->m_hasPalette = false;
					CAutoPtr<CPacket> p2(DNew CPacket());
					p2->SetData(track->GetPalette(), 1024);
					p->Append(*p2);

					CAutoPtr<CPacket> p3(DNew CPacket());
					static BYTE add[13] = {0x00, 0x00, 0x04, 0x00, 0x80, 0x8C, 0x4D, 0x9D, 0x10, 0x8E, 0x25, 0xE9, 0xFE};
					p3->SetData(add, _countof(add));
					p->Append(*p3);
				}
			}

			hr = DeliverPacket(p);
		}

		{
			AP4_Sample sample;
			if (AP4_SUCCEEDED(track->GetSample(++pPairNext->m_value.index, sample))) {
				pPairNext->m_value.ts = sample.GetCts();
			}
		}

	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CMP4SplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);

	nKFs = m_sps.GetCount();
	return S_OK;
}

STDMETHODIMP CMP4SplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_pFile, E_UNEXPECTED);

	if (*pFormat != TIME_FORMAT_MEDIA_TIME) {
		return E_INVALIDARG;
	}

	for (nKFs = 0; nKFs < m_sps.GetCount(); nKFs++) {
		pKFs[nKFs] = m_sps[nKFs].rt;
	}

	return S_OK;
}

//
// CMP4SourceFilter
//

CMP4SourceFilter::CMP4SourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMP4SplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

//
// CMPEG4VideoSplitterFilter
//

CMPEG4VideoSplitterFilter::CMPEG4VideoSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMPEG4VideoSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

void CMPEG4VideoSplitterFilter::SkipUserData()
{
	m_pFile->BitByteAlign();
	while (m_pFile->BitRead(32, true) == 0x000001b2)
		while (m_pFile->BitRead(24, true) != 0x000001) {
			m_pFile->BitRead(8);
		}
}

HRESULT CMPEG4VideoSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pFile.Attach(DNew CBaseSplitterFileEx(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	// TODO

	DWORD width = 0;
	DWORD height = 0;
	BYTE parx = 1;
	BYTE pary = 1;
	REFERENCE_TIME atpf = 400000;

	if (m_pFile->BitRead(24, true) != 0x000001) {
		return E_FAIL;
	}

	BYTE id;
	while (m_pFile->NextMpegStartCode(id, 1024 - m_pFile->GetPos())) {
		if (id == 0xb5) {
			BYTE is_visual_object_identifier = (BYTE)m_pFile->BitRead(1);

			if (is_visual_object_identifier) {
				BYTE visual_object_verid = (BYTE)m_pFile->BitRead(4);
				BYTE visual_object_priority = (BYTE)m_pFile->BitRead(3);
			}

			BYTE visual_object_type = (BYTE)m_pFile->BitRead(4);

			if (visual_object_type == 1 || visual_object_type == 2) {
				BYTE video_signal_type = (BYTE)m_pFile->BitRead(1);

				if (video_signal_type) {
					BYTE video_format = (BYTE)m_pFile->BitRead(3);
					BYTE video_range = (BYTE)m_pFile->BitRead(1);
					BYTE colour_description = (BYTE)m_pFile->BitRead(1);

					if (colour_description) {
						BYTE colour_primaries = (BYTE)m_pFile->BitRead(8);
						BYTE transfer_characteristics = (BYTE)m_pFile->BitRead(8);
						BYTE matrix_coefficients = (BYTE)m_pFile->BitRead(8);
					}
				}
			}

			SkipUserData();

			if (visual_object_type == 1) {
				if (m_pFile->BitRead(24) != 0x000001) {
					break;
				}

				BYTE video_object_start_code = (BYTE)m_pFile->BitRead(8);
				if (video_object_start_code < 0x00 || video_object_start_code > 0x1f) {
					break;
				}

				if (m_pFile->BitRead(24) != 0x000001) {
					break;
				}

				BYTE video_object_layer_start_code = (DWORD)m_pFile->BitRead(8);
				if (video_object_layer_start_code < 0x20 || video_object_layer_start_code > 0x2f) {
					break;
				}

				BYTE random_accessible_vol = (BYTE)m_pFile->BitRead(1);
				BYTE video_object_type_indication = (BYTE)m_pFile->BitRead(8);

				if (video_object_type_indication == 0x12) { // Fine Granularity Scalable
					break;    // huh
				}

				BYTE is_object_layer_identifier = (BYTE)m_pFile->BitRead(1);

				BYTE video_object_layer_verid = 0;

				if (is_object_layer_identifier) {
					video_object_layer_verid = (BYTE)m_pFile->BitRead(4);
					BYTE video_object_layer_priority = (BYTE)m_pFile->BitRead(3);
				}

				BYTE aspect_ratio_info = (BYTE)m_pFile->BitRead(4);

				switch (aspect_ratio_info) {
					default:
						ASSERT(0);
						break;
					case 1:
						parx = 1;
						pary = 1;
						break;
					case 2:
						parx = 12;
						pary = 11;
						break;
					case 3:
						parx = 10;
						pary = 11;
						break;
					case 4:
						parx = 16;
						pary = 11;
						break;
					case 5:
						parx = 40;
						pary = 33;
						break;
					case 15:
						parx = (BYTE)m_pFile->BitRead(8);
						pary = (BYTE)m_pFile->BitRead(8);
						break;
				}

				BYTE vol_control_parameters = (BYTE)m_pFile->BitRead(1);

				if (vol_control_parameters) {
					BYTE chroma_format = (BYTE)m_pFile->BitRead(2);
					BYTE low_delay = (BYTE)m_pFile->BitRead(1);
					BYTE vbv_parameters = (BYTE)m_pFile->BitRead(1);

					if (vbv_parameters) {
						WORD first_half_bit_rate = (WORD)m_pFile->BitRead(15);
						if (!m_pFile->BitRead(1)) {
							break;
						}
						WORD latter_half_bit_rate = (WORD)m_pFile->BitRead(15);
						if (!m_pFile->BitRead(1)) {
							break;
						}
						WORD first_half_vbv_buffer_size = (WORD)m_pFile->BitRead(15);
						if (!m_pFile->BitRead(1)) {
							break;
						}

						BYTE latter_half_vbv_buffer_size = (BYTE)m_pFile->BitRead(3);
						WORD first_half_vbv_occupancy = (WORD)m_pFile->BitRead(11);
						if (!m_pFile->BitRead(1)) {
							break;
						}
						WORD latter_half_vbv_occupancy = (WORD)m_pFile->BitRead(15);
						if (!m_pFile->BitRead(1)) {
							break;
						}
					}
				}

				BYTE video_object_layer_shape = (BYTE)m_pFile->BitRead(2);

				if (video_object_layer_shape == 3 && video_object_layer_verid != 1) {
					BYTE video_object_layer_shape_extension = (BYTE)m_pFile->BitRead(4);
				}

				if (!m_pFile->BitRead(1)) {
					break;
				}
				WORD vop_time_increment_resolution = (WORD)m_pFile->BitRead(16);
				if (!m_pFile->BitRead(1)) {
					break;
				}
				BYTE fixed_vop_rate = (BYTE)m_pFile->BitRead(1);

				if (fixed_vop_rate) {
					int bits = 0;
					for (WORD i = vop_time_increment_resolution; i; i /= 2) {
						++bits;
					}

					WORD fixed_vop_time_increment = m_pFile->BitRead(bits);

					if (fixed_vop_time_increment) {
						atpf = UNITS * fixed_vop_time_increment / vop_time_increment_resolution;
					}
				}

				if (video_object_layer_shape != 2) {
					if (video_object_layer_shape == 0) {
						if (!m_pFile->BitRead(1)) {
							break;
						}
						width = (WORD)m_pFile->BitRead(13);
						if (!m_pFile->BitRead(1)) {
							break;
						}
						height = (WORD)m_pFile->BitRead(13);
						if (!m_pFile->BitRead(1)) {
							break;
						}
					}

					BYTE interlaced = (BYTE)m_pFile->BitRead(1);
					BYTE obmc_disable = (BYTE)m_pFile->BitRead(1);

					// ...
				}
			}
		} else if (id == 0xb6) {
			m_seqhdrsize = m_pFile->GetPos() - 4;
		}
	}

	if (!width || !height) {
		return E_FAIL;
	}

	CAtlArray<CMediaType> mts;

	CMediaType mt;
	mt.SetSampleSize(1);

	mt.majortype = MEDIATYPE_Video;
	mt.subtype = FOURCCMap('v4pm');
	mt.formattype = FORMAT_MPEG2Video;
	MPEG2VIDEOINFO* mvih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + m_seqhdrsize);
	memset(mvih, 0, mt.FormatLength());
	mvih->hdr.bmiHeader.biSize = sizeof(mvih->hdr.bmiHeader);
	mvih->hdr.bmiHeader.biWidth = width;
	mvih->hdr.bmiHeader.biHeight = height;
	mvih->hdr.bmiHeader.biCompression = 'v4pm';
	mvih->hdr.bmiHeader.biPlanes = 1;
	mvih->hdr.bmiHeader.biBitCount = 24;
	mvih->hdr.bmiHeader.biSizeImage = DIBSIZE(mvih->hdr.bmiHeader);
	mvih->hdr.AvgTimePerFrame = atpf;
	mvih->hdr.dwPictAspectRatioX = width*parx;
	mvih->hdr.dwPictAspectRatioY = height*pary;
	mvih->cbSequenceHeader = m_seqhdrsize;
	m_pFile->Seek(0);
	m_pFile->ByteRead((BYTE*)mvih->dwSequenceHeader, m_seqhdrsize);
	mts.Add(mt);
	mt.subtype = FOURCCMap(mvih->hdr.bmiHeader.biCompression = 'V4PM');
	mts.Add(mt);

	CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Video", this, this, &hr));
	EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));

	m_rtNewStop = m_rtStop = m_rtDuration;

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CMPEG4VideoSplitterFilter::DemuxInit()
{
	return true;
}

void CMPEG4VideoSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	ASSERT(rt == 0);

	m_pFile->Seek(m_seqhdrsize);
}

bool CMPEG4VideoSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	CAutoPtr<CPacket> p;

	REFERENCE_TIME rt = 0;
	REFERENCE_TIME atpf = ((MPEG2VIDEOINFO*)GetOutputPin(0)->CurrentMediaType().Format())->hdr.AvgTimePerFrame;

	DWORD sync = ~0;

	while (SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetRemaining()) {
		for (int i = 0; i < 65536; ++i) { // don't call CheckRequest so often
			bool eof = !m_pFile->GetRemaining();

			if (p && !p->IsEmpty() && (m_pFile->BitRead(32, true) == 0x000001b6 || eof)) {
				hr = DeliverPacket(p);
			}

			if (eof) {
				break;
			}

			if (!p) {
				p.Attach(DNew CPacket());
				p->SetCount(0, 1024);
				p->TrackNumber = 0;
				p->rtStart = rt;
				p->rtStop = rt + atpf;
				p->bSyncPoint = FALSE;
				rt += atpf;
				// rt = INVALID_TIME;
			}

			BYTE b;
			m_pFile->ByteRead(&b, 1);
			p->Add(b);
		}
	}

	return true;
}

//
// CMPEG4VideoSourceFilter
//

CMPEG4VideoSourceFilter::CMPEG4VideoSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMPEG4VideoSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

//
// CMP4SplitterOutputPin
//

CMP4SplitterOutputPin::CMP4SplitterOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
}

CMP4SplitterOutputPin::~CMP4SplitterOutputPin()
{
}

HRESULT CMP4SplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(this);
	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CMP4SplitterOutputPin::DeliverEndFlush()
{
	CAutoLock cAutoLock(this);
	return __super::DeliverEndFlush();
}

HRESULT CMP4SplitterOutputPin::DeliverPacket(CAutoPtr<CPacket> p)
{
	CAutoLock cAutoLock(this);
	return __super::DeliverPacket(p);
}
