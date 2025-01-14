/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
#include <basestruct.h>
#include "DSUtil/GolombBuffer.h"
#include "DSUtil/AudioParser.h"
#include "DSUtil/MP4AudioDecoderConfig.h"
#include "DSUtil/BitsWriter.h"
#include "MP4Splitter.h"

#include <ExtLib/Bento4/Core/Ap4.h>
#include <ExtLib/Bento4/Core/Ap4File.h>
#include <ExtLib/Bento4/Core/Ap4CttsAtom.h>
#include <ExtLib/Bento4/Core/Ap4SttsAtom.h>
#include <ExtLib/Bento4/Core/Ap4StssAtom.h>
#include <ExtLib/Bento4/Core/Ap4StsdAtom.h>
#include <ExtLib/Bento4/Core/Ap4IsmaCryp.h>
#include <ExtLib/Bento4/Core/Ap4ChplAtom.h>
#include <ExtLib/Bento4/Core/Ap4FtabAtom.h>
#include <ExtLib/Bento4/Core/Ap4DataAtom.h>
#include <ExtLib/Bento4/Core/Ap4PaspAtom.h>
#include <ExtLib/Bento4/Core/Ap4ChapAtom.h>
#include <ExtLib/Bento4/Core/Ap4Dvc1Atom.h>
#include <ExtLib/Bento4/Core/Ap4DataInfoAtom.h>

#include <libavutil/intreadwrite.h>
#include <libavutil/pixfmt.h>

#define MOV_TKHD_FLAG_ENABLED       0x0001

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MP4},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{(LPWSTR)L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesIn), sudPinTypesIn},
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMP4SplitterFilter), MP4SplitterName, MERIT_NORMAL, std::size(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMP4SourceFilter), MP4SourceName, MERIT_NORMAL, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMP4SplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMP4SourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = std::size(g_Templates);

STDAPI DllRegisterServer()
{
	DeleteRegKey(L"Media Type\\Extensions\\", L".mp4");
	DeleteRegKey(L"Media Type\\Extensions\\", L".mov");

	const std::list<CString> chkbytes = {
		L"4,4,,66747970", // '....ftyp'
		L"4,4,,6d6f6f76", // '....moov'
		L"4,4,,6d646174", // '....mdat'
		L"4,4,,77696465", // '....wide'
		L"4,4,,736b6970", // '....skip'
		L"4,4,,66726565", // '....free'
		L"4,4,,706e6f74", // '....pnot'
	};

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MP4, chkbytes, nullptr);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_MP4);

	return AMovieDllRegisterServer2(FALSE);
}

#include "filters/filters/Filters.h"

CFilterApp theApp;

#endif

//
// CMP4SplitterFilter
//

CMP4SplitterFilter::CMP4SplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"CMP4SplitterFilter", pUnk, phr, __uuidof(this))
{
	m_nFlag |= SOURCE_SUPPORT_URL;
}

CMP4SplitterFilter::~CMP4SplitterFilter()
{
	SAFE_DELETE(m_ColorSpace);
	SAFE_DELETE(m_MasterDataHDR);
	SAFE_DELETE(m_HDRContentLightLevel);
}

STDMETHODIMP CMP4SplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IExFilterInfo)
		__super::NonDelegatingQueryInterface(riid, ppv);
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

static void SetPictAR(CSize& pictAR, LONG width, LONG height, LONG codec_width, LONG codec_height, VIDEOINFOHEADER2* vih2)
{
	if (!pictAR.cx || !pictAR.cy) {
		if (width && height) {
			pictAR.cx = width;
			pictAR.cy = height;
		} else {
			pictAR.cx = codec_width;
			pictAR.cy = codec_height;
		}
		ReduceDim(pictAR);
	}

	if (vih2) {
		vih2->dwPictAspectRatioX = pictAR.cx;
		vih2->dwPictAspectRatioY = pictAR.cy;
	}
}

static const DWORD GetFourcc(AP4_VisualSampleEntry* vse)
{
	DWORD fourcc = -1;

	const AP4_Atom::Type type = vse->GetType();
	const CString tname = UTF8ToWStr(vse->GetCompressorName());

	switch (type) {
	// RAW video
	case AP4_ATOM_TYPE_RAW:
		fourcc = BI_RGB;
		break;
	case AP4_ATOM_TYPE_2vuy:
	case AP4_ATOM_TYPE_2Vuy:
		fourcc = FCC('UYVY');
		break;
	case AP4_ATOM_TYPE_DVOO:
	case AP4_ATOM_TYPE_yuvs:
		fourcc = FCC('YUY2');
		break;
	case AP4_ATOM_TYPE_v410:
		fourcc = FCC('V410');
		break;
	// DivX 3
	case AP4_ATOM_TYPE_DIV3:
	case AP4_ATOM_TYPE_3IVD:
		fourcc = FCC('DIV3');
		break;
	// H.263
	case AP4_ATOM_TYPE_H263:
		if (tname == L"Sorenson H263") {
			fourcc = FCC('FLV1');
		} else {
			fourcc = FCC('H263');
		}
		break;
	// Motion-JPEG
	case AP4_ATOM_TYPE_MJPG:
	case AP4_ATOM_TYPE_AVDJ: // uncommon fourcc
	case AP4_ATOM_TYPE_DMB1: // uncommon fourcc
		fourcc = FCC('MJPG');
		break;
	// JPEG2000
	case AP4_ATOM_TYPE_MJP2:
	case AP4_ATOM_TYPE_AVj2: // uncommon fourcc
		fourcc = FCC('MJP2');
		break;
	// VC-1
	case AP4_ATOM_TYPE_OVC1:
	case AP4_ATOM_TYPE_VC1:
		fourcc = FCC('WVC1');
		break;
	// DV Video (http://msdn.microsoft.com/en-us/library/windows/desktop/dd388646%28v=vs.85%29.aspx)
	case AP4_ATOM_TYPE_DVC:
	case AP4_ATOM_TYPE_DVCP:
		fourcc = FCC('dvsd'); // MEDIASUBTYPE_dvsd (DV Video Decoder, ffdshow, LAV)
		break;
	case AP4_ATOM_TYPE_DVPP:
		fourcc = FCC('dv25'); // MEDIASUBTYPE_dv25 (ffdshow, LAV)
		break;
	case AP4_ATOM_TYPE_DV5N:
	case AP4_ATOM_TYPE_DV5P:
		fourcc = FCC('dv50'); // MEDIASUBTYPE_dv50 (ffdshow, LAV)
		break;
	case AP4_ATOM_TYPE_DVH2:
	case AP4_ATOM_TYPE_DVH3:
	case AP4_ATOM_TYPE_DVH4:
	case AP4_ATOM_TYPE_DVH5:
	case AP4_ATOM_TYPE_DVH6:
	case AP4_ATOM_TYPE_DVHQ:
	case AP4_ATOM_TYPE_DVHP:
		fourcc = FCC('CDVH'); // MEDIASUBTYPE_CDVH (LAV)
		break;
	// MagicYUV
	case AP4_ATOM_TYPE_M8RG:
	case AP4_ATOM_TYPE_M8RA:
	case AP4_ATOM_TYPE_M8G0:
	case AP4_ATOM_TYPE_M8Y0:
	case AP4_ATOM_TYPE_M8Y2:
	case AP4_ATOM_TYPE_M8Y4:
	case AP4_ATOM_TYPE_M8YA:
	case AP4_ATOM_TYPE_M0RG:
	case AP4_ATOM_TYPE_M0RA:
	case AP4_ATOM_TYPE_M0R0:
	case AP4_ATOM_TYPE_M0Y2:
	case AP4_ATOM_TYPE_M2RG:
	case AP4_ATOM_TYPE_M2RA:
		fourcc = FCC('MAGY');
		break;
	case AP4_ATOM_TYPE_vp08:
		fourcc = FCC('VP80');
		break;
	case AP4_ATOM_TYPE_vp09:
		fourcc = FCC('VP90');
		break;
	case AP4_ATOM_TYPE_av01:
		fourcc = FCC('AV01');
		break;
	default:
		fourcc = _byteswap_ulong(type);
		break;
	}

	return fourcc;
}

HRESULT CMP4SplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_trackpos.clear();

	m_pFile.reset(DNew CMP4SplitterFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.reset();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;
	REFERENCE_TIME rtVideoDuration = 0;

	CSize videoSize;

	int nRotation = 0;
	AP4_Array<AP4_UI32> ChapterTrackEntries;

	if (AP4_Movie* movie = m_pFile->GetMovie()) {
		int iAudio = 0;
		int iSubtitle = 0;

		// looking for main video track (skip tracks with motionless frames)
		AP4_UI32 mainvideoID = 0;
		for (AP4_List<AP4_Track>::Item* item = movie->GetTracks().FirstItem();
				item;
				item = item->GetNext()) {
			AP4_Track* track = item->GetData();

			if (track->GetType() != AP4_Track::TYPE_VIDEO) {
				continue;
			}
			if (track->GetSampleCount() == 1 && track->GetDuration() == 0) {
				// broken video track that can't be played
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

			if (track->GetType() == AP4_Track::TYPE_VIDEO && track->GetId() != mainvideoID) {
				continue;
			}

			AP4_Sample sample;

			if (!AP4_SUCCEEDED(track->GetSample(0, sample)) || sample.GetDescriptionIndex() == 0xFFFFFFFF) {
				continue;
			}

			if (AP4_ChapAtom* chap = dynamic_cast<AP4_ChapAtom*>(track->GetTrakAtom()->FindChild("tref/chap"))) {
				ChapterTrackEntries = chap->GetChapterTrackEntries();
			} else {
				bool bFoundChapterTrack = false;
				for (AP4_Cardinal i = 0; i < ChapterTrackEntries.ItemCount(); i++) {
					const AP4_UI32& ChapterTrackId = ChapterTrackEntries[i];
					if (ChapterTrackId == track->GetId()) {
						bFoundChapterTrack = true;
						break;
					}
				}
				if (bFoundChapterTrack) {
					continue;
				}
			}

			CString TrackName;
			// We do not use the "mdia/hdlr" atom, because it is useless information ("SoundHandler" and so on).
			const auto nameAtom = dynamic_cast<AP4_DataInfoAtom*>(track->GetTrakAtom()->FindChild("udta/name"));
			if (nameAtom) {
				auto name_data = nameAtom->GetData();
				if (name_data->GetDataSize() > 0) {
					CStringA tmp((char*)name_data->GetData(), name_data->GetDataSize());
					TrackName = AltUTF8ToWStr(tmp);
				}
			}
			TrackName.Trim();

			CStringA TrackLanguage = track->GetTrackLanguage().c_str();
			CStringW outputDesc;

			CSize PictAR;
			AP4_UI32 width = 0;
			AP4_UI32 height = 0;
			REFERENCE_TIME AvgTimePerFrame = 0;
			AP4_UI32 tkhd_flags = 0;

			if (track->GetType() == AP4_Track::TYPE_VIDEO) {
				if (AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD))) {
					width = tkhd->GetWidth() >> 16;
					height = tkhd->GetHeight() >> 16;

					tkhd->GetPictAR(PictAR.cx, PictAR.cy);

					if (!nRotation) {
						nRotation = tkhd->GetRotation();
						if (nRotation) {
							CString prop;
							prop.Format(L"%d", nRotation);
							SetProperty(L"ROTATION", prop);
						}
					}
				}

				if (!PictAR.cx || !PictAR.cy) {
					if (AP4_DataInfoAtom* clef = dynamic_cast<AP4_DataInfoAtom*>(track->GetTrakAtom()->FindChild("tapt/clef"))) {
						const AP4_DataBuffer* clef_data = clef->GetData();
						if (clef_data->GetDataSize() == 12) { // 20 bytes(size) - 8 bytes(header)
							const uint32_t* data = (uint32_t*)clef_data->GetData();
							if (data[1] && data[2]) {
								uint32_t clef_x = _byteswap_ulong(data[1]) >> 16;
								uint32_t clef_y = _byteswap_ulong(data[2]) >> 16;
								if (clef_x > 0 && clef_y > 0) {
									ReduceDim(clef_x, clef_y);
									PictAR = { (long)clef_x, (long)clef_y };
								}
							}
						}
					}
				}

				AvgTimePerFrame = track->GetSampleCount() ? REFERENCE_TIME(track->GetDurationHighPrecision() * 10000.0 / (track->GetSampleCount())) : 0;
				if (AP4_SttsAtom* stts = dynamic_cast<AP4_SttsAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stts"))) {
					AP4_Duration totalDuration = stts->GetTotalDuration();
					AP4_UI32 totalFrames       = stts->GetTotalFrames();
					if (totalDuration && totalFrames) {
						AvgTimePerFrame = 10000000.0 / track->GetMediaTimeScale() * totalDuration / totalFrames;
					}
				}

				if (!movie->HasFragments()
						&& track->GetTrakAtom()->FindChild("mdia/minf/stbl/ctts") == nullptr) {
					m_dtsonly = 1;
				}
			}
			else if (track->GetType() == AP4_Track::TYPE_TEXT) {
				if (AP4_TkhdAtom* tkhd = dynamic_cast<AP4_TkhdAtom*>(track->GetTrakAtom()->GetChild(AP4_ATOM_TYPE_TKHD))) {
					tkhd_flags = tkhd->GetFlags();
				}
			}

			std::vector<CMediaType> mts;

			CMediaType mt;
			mt.SetSampleSize(1);

			VIDEOINFOHEADER2* vih2 = nullptr;
			WAVEFORMATEX* wfe      = nullptr;

			AP4_DataBuffer empty;

			if (AP4_SampleDescription* desc = track->GetSampleDescription(sample.GetDescriptionIndex())) {
				AP4_MpegSampleDescription* mpeg_desc = nullptr;

				if (desc->GetType() == AP4_SampleDescription::TYPE_MPEG) {
					mpeg_desc = dynamic_cast<AP4_MpegSampleDescription*>(desc);
				} else if (desc->GetType() == AP4_SampleDescription::TYPE_ISMACRYP) {
					AP4_IsmaCrypSampleDescription* isma_desc = dynamic_cast<AP4_IsmaCrypSampleDescription*>(desc);
					mpeg_desc = isma_desc->GetOriginalSampleDescription();
				}

				// AP4_MpegVideoSampleDescription
				if (auto* video_desc = dynamic_cast<AP4_MpegVideoSampleDescription*>(mpeg_desc)) {
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

					vih2 = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2) + di->GetDataSize());
					memset(vih2, 0, mt.FormatLength());
					vih2->dwBitRate            = video_desc->GetAvgBitrate()/8;
					vih2->bmiHeader.biSize     = sizeof(vih2->bmiHeader);
					vih2->bmiHeader.biWidth    = biWidth;
					vih2->bmiHeader.biHeight   = biHeight;
					vih2->bmiHeader.biBitCount = video_desc->GetDepth();
					vih2->rcSource             = vih2->rcTarget = CRect(0, 0, biWidth, biHeight);
					vih2->AvgTimePerFrame      = AvgTimePerFrame;

					SetPictAR(PictAR, width, height, vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight, vih2);

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

								CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, PictAR, data, size);
								mt.SetSampleSize(pbmi.biSizeImage);
								mts.push_back(mt);

								MPEG2VIDEOINFO* mvih	= (MPEG2VIDEOINFO*)mt.pbFormat;
								mt.subtype				= FOURCCMap(mvih->hdr.bmiHeader.biCompression = 'V4PM');
								mts.push_back(mt);
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

								CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, PictAR, data, size);
								mt.SetSampleSize(pbmi.biSizeImage);
								mts.push_back(mt);
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
							mts.push_back(mt);
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
							mts.push_back(mt);
							break;
						case AP4_TSC2_OTI:
							vih2->bmiHeader.biCompression = FCC('TSC2');
							mt.subtype = MEDIASUBTYPE_TSCC2;
							mts.push_back(mt);
							break;
						case AP4_PNG_OTI:
							vih2->bmiHeader.biCompression = FCC('PNG ');
							mt.subtype = MEDIASUBTYPE_PNG;
							mts.push_back(mt);
							break;
					}

					if (mt.subtype != GUID_NULL) {
						outputDesc.SetString(L"Video 1");
					}
					DLogIf(mt.subtype == GUID_NULL, L"Unknown video OBI: %02x", video_desc->GetObjectTypeId());
				}
				// AP4_MpegAudioSampleDescription
				else if (auto audio_desc = dynamic_cast<AP4_MpegAudioSampleDescription*>(mpeg_desc)) {
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

					AP4_UI08 Mpeg4AudioObjectType = 0;

					switch (audio_desc->GetObjectTypeId()) {
						case AP4_MPEG4_AUDIO_OTI:
							{
								const AP4_MpegAudioSampleDescription::Mpeg4AudioObjectType objectType = audio_desc->GetMpeg4AudioObjectType();
								if (objectType == AOT_AAC_LC
										|| objectType == AOT_AAC_MAIN
										|| objectType == AOT_SBR
										|| objectType == AOT_PS
										|| objectType == AOT_USAC) {
									CMP4AudioDecoderConfig MP4AudioDecoderConfig;
									if (MP4AudioDecoderConfig.Parse(di->GetData(), di->GetDataSize())) {
										if (MP4AudioDecoderConfig.m_ChannelCount != wfe->nChannels) {
											wfe->nChannels = MP4AudioDecoderConfig.m_ChannelCount;
											wfe->nBlockAlign = (WORD)((wfe->nChannels * wfe->wBitsPerSample) / 8);
										}
										if (MP4AudioDecoderConfig.m_SamplingFrequency != wfe->nSamplesPerSec) {
											wfe->nSamplesPerSec = MP4AudioDecoderConfig.m_SamplingFrequency;
										}
									}
								}

								if (objectType == AOT_ALS) {
									wfe->wFormatTag = WAVE_FORMAT_UNKNOWN;
									mt.subtype = MEDIASUBTYPE_ALS;
									mts.push_back(mt);
									break;
								}
							}
						case AP4_MPEG2_AAC_AUDIO_MAIN_OTI:
						case AP4_MPEG2_AAC_AUDIO_LC_OTI:
						case AP4_MPEG2_AAC_AUDIO_SSRP_OTI:
							mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_RAW_AAC1);
							mts.push_back(mt);
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
							mts.push_back(mt);
							break;
						case AP4_DTSC_AUDIO_OTI:
						case AP4_DTSH_AUDIO_OTI:
						case AP4_DTSL_AUDIO_OTI:
							mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_DTS2);
							{
								m_pFile->Seek(sample.GetOffset());
								CBaseSplitterFileEx::dtshdr h;
								CMediaType mt2;
								if (m_pFile->Read(h, sample.GetSize(), &mt2, false)) {
									mt = mt2;

									AP4_DataBuffer data;
									if (AP4_SUCCEEDED(sample.ReadData(data))) {
										const BYTE* start = data.GetData();
										const BYTE* end = start + data.GetDataSize();
										int size = ParseDTSHeader(start);
										if (size) {
											if (start + size + 40 <= end) {
												audioframe_t aframe;
												int sizehd = ParseDTSHDHeader(start + size, end - start - size, &aframe);
												if (sizehd) {
													wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 1);
													wfe->cbSize = 1;
													((BYTE *)(wfe + 1))[0] = (BYTE)aframe.param2;

													wfe->nSamplesPerSec = aframe.samplerate;
													wfe->nChannels = aframe.channels;
													if (aframe.param1) {
														wfe->wBitsPerSample = aframe.param1;
													}
													wfe->nBlockAlign = (wfe->nChannels * wfe->wBitsPerSample) / 8;
													if (aframe.param2 == DCA_PROFILE_HD_HRA || aframe.param2 == DCA_PROFILE_HD_HRA_X || aframe.param2 == DCA_PROFILE_HD_HRA_X_IMAX) {
														wfe->nAvgBytesPerSec += CalcBitrate(aframe) / 8;
													} else {
														wfe->nAvgBytesPerSec = 0;
													}
												}
											}
										}
									}
								}
							}
							mts.push_back(mt);
							break;
						case AP4_VORBIS_OTI:
							CreateVorbisMediaType(mt, mts,
												  (DWORD)audio_desc->GetChannelCount(),
												  (DWORD)(audio_desc->GetSampleRate() ? audio_desc->GetSampleRate() : track->GetMediaTimeScale()),
												  (DWORD)audio_desc->GetSampleSize(), di->GetData(), di->GetDataSize());
							break;
					}

					if (mt.subtype != GUID_NULL) {
						iAudio++;
						outputDesc.Format(L"Audio %d", iAudio);
					}
					DLogIf(mt.subtype == GUID_NULL, L"Unknown audio OBI: %02x", audio_desc->GetObjectTypeId());
				}
				// AP4_MpegSystemSampleDescription
				else if (auto system_desc = dynamic_cast<AP4_MpegSystemSampleDescription*>(desc)) {
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
								std::list<CStringA> sl;
								for (int i = 0; i < 16 * 4; i += 4) {
									BYTE y = (pal[i + 1] - 16) * 255 / 219;
									BYTE u = pal[i + 2];
									BYTE v = pal[i + 3];
									BYTE r = (BYTE)std::clamp(1.0 * y + 1.4022 * (v - 128), 0.0, 255.0);
									BYTE g = (BYTE)std::clamp(1.0 * y - 0.3456 * (u - 128) - 0.7145 * (v - 128), 0.0, 255.0);
									BYTE b = (BYTE)std::clamp(1.0 * y + 1.7710 * (u - 128), 0.0, 255.0);
									CStringA str;
									str.Format("%02x%02x%02x", r, g, b);
									sl.push_back(str);
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
								strncpy_s(psi->IsoLang, TrackLanguage, std::size(psi->IsoLang) - 1);
								wcsncpy_s(psi->TrackName, TrackName, std::size(psi->TrackName) - 1);
								memcpy(psi + 1, (LPCSTR)hdr, hdr.GetLength());
								mts.push_back(mt);
							}
							break;
					}

					if (mt.subtype != GUID_NULL) {
						iSubtitle++;
						outputDesc.Format(L"Subtitle %d", iSubtitle);
					}
					DLogIf(mt.subtype == GUID_NULL, L"Unknown system OBI: %02x", system_desc->GetObjectTypeId());
				}
				// AP4_UnknownSampleDescription
				else if (auto unknown_desc = dynamic_cast<AP4_UnknownSampleDescription*>(desc)) { // TEMP
					AP4_SampleEntry* sample_entry = unknown_desc->GetSampleEntry();

					if (dynamic_cast<AP4_TextSampleEntry*>(sample_entry)
							|| dynamic_cast<AP4_Tx3gSampleEntry*>(sample_entry)) {
						mt.majortype = MEDIATYPE_Subtitle;
						mt.subtype = MEDIASUBTYPE_UTF8;
						mt.formattype = FORMAT_SubtitleInfo;
						SUBTITLEINFO* si = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO));
						memset(si, 0, mt.FormatLength());
						si->dwOffset = sizeof(SUBTITLEINFO);
						strcpy_s(si->IsoLang, std::size(si->IsoLang), CStringA(TrackLanguage));
						BOOL forced = 0;
						if (AP4_Tx3gSampleEntry* tx3g = dynamic_cast<AP4_Tx3gSampleEntry*>(sample_entry)) {
							const AP4_Tx3gSampleEntry::AP4_Tx3gDescription& description = tx3g->GetDescription();
							forced = description.DisplayFlags & 0x80000000;
						}

						if (tkhd_flags & MOV_TKHD_FLAG_ENABLED) {
							if (forced) {
								TrackName += L" [Default, Forced]";
							} else {
								TrackName += L" [Default]";
							}
						}
						else if (forced) {
							TrackName += L" [Forced]";
						}

						wcscpy_s(si->TrackName, std::size(si->TrackName), TrackName);
						mts.push_back(mt);

						iSubtitle++;
						outputDesc.Format(L"Subtitle %d", iSubtitle);
					}
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

					// AP4_VisualSampleEntry
					if (auto vse = dynamic_cast<AP4_VisualSampleEntry*>(atom)) {
						const DWORD fourcc = GetFourcc(vse);
						AP4_DataBuffer* pData = (AP4_DataBuffer*)&db;

						char buff[5] = { 0 };
						memcpy(buff, &fourcc, 4);

						if ((buff[0] == 'x' || buff[0] == 'h') && buff[1] == 'd') {
							// Apple HDV/XDCAM
							m_pFile->Seek(sample.GetOffset());
							CBaseSplitterFileEx::seqhdr h;
							if (m_pFile->Read(h, sample.GetSize(), &mt)) {
								mts.push_back(mt);
							}
							break;
						}

						if (fourcc == FCC('WVC1')) {
							if (AP4_Dvc1Atom* Dvc1 = dynamic_cast<AP4_Dvc1Atom*>(vse->GetChild(AP4_ATOM_TYPE_DVC1))) {
								pData = (AP4_DataBuffer*)Dvc1->GetDecoderInfo();
							}
						}

						AP4_Size ExtraSize = pData->GetDataSize();
						if (type == AP4_ATOM_TYPE_FFV1
								|| type == AP4_ATOM_TYPE_H263) {
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

						if (!PictAR.cx || !PictAR.cy) {
							if (AP4_PaspAtom* pasp = dynamic_cast<AP4_PaspAtom*>(vse->GetChild(AP4_ATOM_TYPE_PASP))) {
								if (pasp->GetHSpacing() > 0 && pasp->GetVSpacing() > 0) {
									auto pictAR_x = (uint64_t)vse->GetWidth() * pasp->GetHSpacing();
									auto pictAR_y = (uint64_t)vse->GetHeight() * pasp->GetVSpacing();
									ReduceDim(pictAR_x, pictAR_y);
									PictAR = { (long)pictAR_x, (long)pictAR_y };
								}
							}
						}

						SetPictAR(PictAR, width, height, vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight, vih2);

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
								break; // incorrect or unsupported
							}
							mts.push_back(mt);
							outputDesc.SetString(L"Video 1");
							break;
						}

						mt.subtype = FOURCCMap(fourcc);
						mts.push_back(mt);
						outputDesc.SetString(L"Video 1");

						if (fourcc == FCC('yuv2')
								|| fourcc == FCC('b16g')
								|| fourcc == FCC('b48r')
								|| fourcc == FCC('b64a')
							) {
							mt.subtype = MEDIASUBTYPE_LAV_RAWVIDEO;
							mts.push_back(mt);
							break;
						}

						if (mt.subtype == MEDIASUBTYPE_WVC1) {
							mt.subtype = MEDIASUBTYPE_WVC1_CYBERLINK;
							mts.insert(mts.cbegin(), mt);
						}

						_strlwr_s(buff);
						DWORD typelwr = GETU32(buff);
						if (typelwr != fourcc) {
							mt.subtype = FOURCCMap(vih2->bmiHeader.biCompression = typelwr);
							mts.push_back(mt);
						}

						_strupr_s(buff);
						DWORD typeupr = GETU32(buff);
						if (typeupr != fourcc) {
							mt.subtype = FOURCCMap(vih2->bmiHeader.biCompression = typeupr);
							mts.push_back(mt);
						}

						if (vse->m_hasPalette) {
							m_bHasPalette = vse->m_hasPalette;
							memcpy(m_Palette, vse->GetPalette(), 1024);
						}

						if (AP4_DataInfoAtom* fiel = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_FIEL))) {
							const AP4_DataBuffer* fiel_data = fiel->GetData();
							if (fiel_data->GetDataSize() >= 2) {
								const BYTE fields = fiel_data->GetData()[0];
								const BYTE detail = fiel_data->GetData()[1];
								if (fields == 0x01) {
									m_interlaced = 0;
								} else if (fields == 0x02) {
									m_interlaced = (detail == 1 || detail == 9) ? 1 : 2;
								}
							}
						}

						switch (type) {
							case AP4_ATOM_TYPE_AVC1:
							case AP4_ATOM_TYPE_AVC2:
							case AP4_ATOM_TYPE_AVC3:
							case AP4_ATOM_TYPE_AVC4:
							case AP4_ATOM_TYPE_DVAV:
							case AP4_ATOM_TYPE_DVA1:
							case AP4_ATOM_TYPE_AVLG:
							case AP4_ATOM_TYPE_XALG:
							case AP4_ATOM_TYPE_AI1P:
							case AP4_ATOM_TYPE_AI1Q:
							case AP4_ATOM_TYPE_AI12:
							case AP4_ATOM_TYPE_AI13:
							case AP4_ATOM_TYPE_AI15:
							case AP4_ATOM_TYPE_AI16:
							case AP4_ATOM_TYPE_AI5P:
							case AP4_ATOM_TYPE_AI5Q:
							case AP4_ATOM_TYPE_AI52:
							case AP4_ATOM_TYPE_AI53:
							case AP4_ATOM_TYPE_AI55:
							case AP4_ATOM_TYPE_AI56:
							case AP4_ATOM_TYPE_AIVX:
							case AP4_ATOM_TYPE_AVIN:
								{ // H.264/AVC
									const AP4_DataBuffer* di = nullptr;
									if (AP4_DataInfoAtom* avcC = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_AVCC))) {
										di = avcC->GetData();
									}
									if (!di) {
										di = &empty;
									}

									auto data = (BYTE*)di->GetData();
									const auto size = (const size_t)di->GetDataSize();

									if (size > 9) {
										vc_params_t params;
										AVCParser::ParseSequenceParameterSet(data + 9, size - 9, params);
										const auto& codecAvgTimePerFrame = params.AvgTimePerFrame;
										const auto& bInterlaced = params.interlaced;

										if (bInterlaced && codecAvgTimePerFrame) {
											// hack for interlaced video
											const auto factor = (double)AvgTimePerFrame / (double)codecAvgTimePerFrame;
											if (factor > 0.4 && factor < 0.6) {
												AvgTimePerFrame = codecAvgTimePerFrame;
											}
										}
									}

									BITMAPINFOHEADER pbmi = {};
									pbmi.biSize        = sizeof(pbmi);
									pbmi.biWidth       = (LONG)vse->GetWidth();
									pbmi.biHeight      = (LONG)vse->GetHeight();
									pbmi.biCompression = FCC('AVC1');
									pbmi.biPlanes      = 1;
									pbmi.biBitCount    = 24;
									pbmi.biSizeImage   = DIBSIZE(pbmi);

									CreateMPEG2VIfromAVC(&mt, &pbmi, AvgTimePerFrame, PictAR, data, size);
									mt.SetSampleSize(pbmi.biSizeImage);

									mts.clear();
									mts.push_back(mt);

									if (SUCCEEDED(CreateMPEG2VIfromMVC(&mt, &pbmi, AvgTimePerFrame, PictAR, data, size))) {
										mts.insert(mts.cbegin(), mt);
									} else if (!di->GetDataSize()) {
										switch (type) {
											case AP4_ATOM_TYPE_AI1P:
											case AP4_ATOM_TYPE_AI1Q:
											case AP4_ATOM_TYPE_AI12:
											case AP4_ATOM_TYPE_AI13:
											case AP4_ATOM_TYPE_AI15:
											case AP4_ATOM_TYPE_AI16:
											case AP4_ATOM_TYPE_AI5P:
											case AP4_ATOM_TYPE_AI5Q:
											case AP4_ATOM_TYPE_AI52:
											case AP4_ATOM_TYPE_AI53:
											case AP4_ATOM_TYPE_AI55:
											case AP4_ATOM_TYPE_AI56:
											case AP4_ATOM_TYPE_AIVX:
											case AP4_ATOM_TYPE_AVIN: {
													// code from ffmpeg/libavformat/utils.c -> ff_generate_avci_extradata()
													static const uint8_t avci100_1080p_extradata[] = {
														// SPS
														0x00, 0x00, 0x00, 0x01, 0x67, 0x7a, 0x10, 0x29,
														0xb6, 0xd4, 0x20, 0x22, 0x33, 0x19, 0xc6, 0x63,
														0x23, 0x21, 0x01, 0x11, 0x98, 0xce, 0x33, 0x19,
														0x18, 0x21, 0x02, 0x56, 0xb9, 0x3d, 0x7d, 0x7e,
														0x4f, 0xe3, 0x3f, 0x11, 0xf1, 0x9e, 0x08, 0xb8,
														0x8c, 0x54, 0x43, 0xc0, 0x78, 0x02, 0x27, 0xe2,
														0x70, 0x1e, 0x30, 0x10, 0x10, 0x14, 0x00, 0x00,
														0x03, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0xca,
														0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
														// PPS
														0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x33, 0x48,
														0xd0
													};
													static const uint8_t avci100_1080i_extradata[] = {
														// SPS
														0x00, 0x00, 0x00, 0x01, 0x67, 0x7a, 0x10, 0x29,
														0xb6, 0xd4, 0x20, 0x22, 0x33, 0x19, 0xc6, 0x63,
														0x23, 0x21, 0x01, 0x11, 0x98, 0xce, 0x33, 0x19,
														0x18, 0x21, 0x03, 0x3a, 0x46, 0x65, 0x6a, 0x65,
														0x24, 0xad, 0xe9, 0x12, 0x32, 0x14, 0x1a, 0x26,
														0x34, 0xad, 0xa4, 0x41, 0x82, 0x23, 0x01, 0x50,
														0x2b, 0x1a, 0x24, 0x69, 0x48, 0x30, 0x40, 0x2e,
														0x11, 0x12, 0x08, 0xc6, 0x8c, 0x04, 0x41, 0x28,
														0x4c, 0x34, 0xf0, 0x1e, 0x01, 0x13, 0xf2, 0xe0,
														0x3c, 0x60, 0x20, 0x20, 0x28, 0x00, 0x00, 0x03,
														0x00, 0x08, 0x00, 0x00, 0x03, 0x01, 0x94, 0x20,
														// PPS
														0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x33, 0x48,
														0xd0
													};
													static const uint8_t avci50_1080p_extradata[] = {
														// SPS
														0x00, 0x00, 0x00, 0x01, 0x67, 0x6e, 0x10, 0x28,
														0xa6, 0xd4, 0x20, 0x32, 0x33, 0x0c, 0x71, 0x18,
														0x88, 0x62, 0x10, 0x19, 0x19, 0x86, 0x38, 0x8c,
														0x44, 0x30, 0x21, 0x02, 0x56, 0x4e, 0x6f, 0x37,
														0xcd, 0xf9, 0xbf, 0x81, 0x6b, 0xf3, 0x7c, 0xde,
														0x6e, 0x6c, 0xd3, 0x3c, 0x05, 0xa0, 0x22, 0x7e,
														0x5f, 0xfc, 0x00, 0x0c, 0x00, 0x13, 0x8c, 0x04,
														0x04, 0x05, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00,
														0x00, 0x03, 0x00, 0x32, 0x84, 0x00, 0x00, 0x00,
														// PPS
														0x00, 0x00, 0x00, 0x01, 0x68, 0xee, 0x31, 0x12,
														0x11
													};
													static const uint8_t avci50_1080i_extradata[] = {
														// SPS
														0x00, 0x00, 0x00, 0x01, 0x67, 0x6e, 0x10, 0x28,
														0xa6, 0xd4, 0x20, 0x32, 0x33, 0x0c, 0x71, 0x18,
														0x88, 0x62, 0x10, 0x19, 0x19, 0x86, 0x38, 0x8c,
														0x44, 0x30, 0x21, 0x02, 0x56, 0x4e, 0x6e, 0x61,
														0x87, 0x3e, 0x73, 0x4d, 0x98, 0x0c, 0x03, 0x06,
														0x9c, 0x0b, 0x73, 0xe6, 0xc0, 0xb5, 0x18, 0x63,
														0x0d, 0x39, 0xe0, 0x5b, 0x02, 0xd4, 0xc6, 0x19,
														0x1a, 0x79, 0x8c, 0x32, 0x34, 0x24, 0xf0, 0x16,
														0x81, 0x13, 0xf7, 0xff, 0x80, 0x02, 0x00, 0x01,
														0xf1, 0x80, 0x80, 0x80, 0xa0, 0x00, 0x00, 0x03,
														0x00, 0x20, 0x00, 0x00, 0x06, 0x50, 0x80, 0x00,
														// PPS
														0x00, 0x00, 0x00, 0x01, 0x68, 0xee, 0x31, 0x12,
														0x11
													};
													static const uint8_t avci100_720p_extradata[] = {
														// SPS
														0x00, 0x00, 0x00, 0x01, 0x67, 0x7a, 0x10, 0x29,
														0xb6, 0xd4, 0x20, 0x2a, 0x33, 0x1d, 0xc7, 0x62,
														0xa1, 0x08, 0x40, 0x54, 0x66, 0x3b, 0x8e, 0xc5,
														0x42, 0x02, 0x10, 0x25, 0x64, 0x2c, 0x89, 0xe8,
														0x85, 0xe4, 0x21, 0x4b, 0x90, 0x83, 0x06, 0x95,
														0xd1, 0x06, 0x46, 0x97, 0x20, 0xc8, 0xd7, 0x43,
														0x08, 0x11, 0xc2, 0x1e, 0x4c, 0x91, 0x0f, 0x01,
														0x40, 0x16, 0xec, 0x07, 0x8c, 0x04, 0x04, 0x05,
														0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x03,
														0x00, 0x64, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00,
														// PPS
														0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x31, 0x12,
														0x11
													};
													static const uint8_t avci50_720p_extradata[] = {
														// SPS
														0x00, 0x00, 0x00, 0x01, 0x67, 0x6e, 0x10, 0x20,
														0xa6, 0xd4, 0x20, 0x32, 0x33, 0x0c, 0x71, 0x18,
														0x88, 0x62, 0x10, 0x19, 0x19, 0x86, 0x38, 0x8c,
														0x44, 0x30, 0x21, 0x02, 0x56, 0x4e, 0x6f, 0x37,
														0xcd, 0xf9, 0xbf, 0x81, 0x6b, 0xf3, 0x7c, 0xde,
														0x6e, 0x6c, 0xd3, 0x3c, 0x0f, 0x01, 0x6e, 0xff,
														0xc0, 0x00, 0xc0, 0x01, 0x38, 0xc0, 0x40, 0x40,
														0x50, 0x00, 0x00, 0x03, 0x00, 0x10, 0x00, 0x00,
														0x06, 0x48, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
														// PPS
														0x00, 0x00, 0x00, 0x01, 0x68, 0xee, 0x31, 0x12,
														0x11
													};

													bool field_order_progressive = true;
													switch (type) {
														case AP4_ATOM_TYPE_AI15:
														case AP4_ATOM_TYPE_AI16:
														case AP4_ATOM_TYPE_AI55:
														case AP4_ATOM_TYPE_AI56:
														case AP4_ATOM_TYPE_AVIN:
															field_order_progressive = false;
													}

													AP4_UI16 width = vse->GetWidth();

													if (AP4_DataInfoAtom* ares = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_ARES))) {
														const AP4_DataBuffer* ares_data = ares->GetData();
														if (ares_data->GetDataSize() > 11) {
															const WORD cid = _byteswap_ushort(GETU16(ares_data->GetData() + 10));
															if (cid == 0xd4d || cid == 0xd4e) {
																width = 1440;
															}
														}
													}

													BYTE* data  = nullptr;
													size_t size = 0;
													switch (width) {
														case 1920:
															if (field_order_progressive) {
																data = (BYTE*)avci100_1080p_extradata;
																size = sizeof(avci100_1080p_extradata);
															} else {
																data = (BYTE*)avci100_1080i_extradata;
																size = sizeof(avci100_1080i_extradata);
															}
															break;
														case 1440:
															if (field_order_progressive) {
																data = (BYTE*)avci50_1080p_extradata;
																size = sizeof(avci50_1080p_extradata);
															} else {
																data = (BYTE*)avci50_1080i_extradata;
																size = sizeof(avci50_1080i_extradata);
															}
															break;
														case 1280:
														case  960:
															data = (BYTE*)avci50_720p_extradata;
															size = sizeof(avci50_720p_extradata);
															break;
													}

													if (!size) {
														break;
													}

													BITMAPINFOHEADER pbmi;
													memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
													pbmi.biSize        = sizeof(pbmi);
													pbmi.biWidth       = (LONG)width;
													pbmi.biHeight      = (LONG)vse->GetHeight();
													pbmi.biCompression = FCC('H264');
													pbmi.biPlanes      = 1;
													pbmi.biBitCount    = 24;
													pbmi.biSizeImage   = DIBSIZE(pbmi);

													CreateMPEG2VIfromAVC(&mt, &pbmi, AvgTimePerFrame, PictAR, data, size);
													mt.SetSampleSize(pbmi.biSizeImage);

													mts.clear();
													mts.push_back(mt);
												}
												break;
											default: {
													m_pFile->Seek(sample.GetOffset());
													CBaseSplitterFileEx::avchdr h;
													CMediaType mt2;
													if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
														mts.insert(mts.cbegin(), mt2);
													}
												}
										}
									}

									// code from libavformat/mov.c
									#define MAX_REORDER_DELAY 16
									AP4_SI64 pts_buf[MAX_REORDER_DELAY + 1];
									for (size_t i = 0; i < MAX_REORDER_DELAY + 1; i++) {
										pts_buf[i] = INT64_MIN;
									}

									m_video_delay = 0;

									int j, r, buf_start = 0;
									AP4_Sample sample_tmp;
									for (AP4_Ordinal index = 0; index < 150; index++) {
										if (!AP4_SUCCEEDED(track->GetSample(index, sample_tmp)) || sample_tmp.GetDescriptionIndex() == 0xFFFFFFFF) {
											break;
										}

										j = buf_start;
										buf_start++;
										if (buf_start == MAX_REORDER_DELAY + 1) {
											buf_start = 0;
										}

										pts_buf[j] = sample_tmp.GetCts();

										int num_swaps = 0;
										while (j != buf_start) {
											r = j - 1;
											if (r < 0) r = MAX_REORDER_DELAY;
											if (pts_buf[j] < pts_buf[r]) {
												std::swap(pts_buf[j], pts_buf[r]);
												++num_swaps;
											} else {
												break;
											}
											j = r;
										}

										m_video_delay = std::max(m_video_delay, num_swaps);
									}
								}
								break;
							case AP4_ATOM_TYPE_HVC1:
							case AP4_ATOM_TYPE_HEV1:
							case AP4_ATOM_TYPE_DVHE:
							case AP4_ATOM_TYPE_DVH1:
								{ // HEVC/H.265
									const AP4_DataBuffer* di = nullptr;
									if (AP4_DataInfoAtom* hvcC = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_HVCC))) {
										di = hvcC->GetData();
									}
									if (!di) {
										di = &empty;
									}

									BYTE* data         = (BYTE*)di->GetData();
									size_t size        = (size_t)di->GetDataSize();

									BITMAPINFOHEADER pbmi;
									memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
									pbmi.biSize        = sizeof(pbmi);
									pbmi.biWidth       = (LONG)vse->GetWidth();
									pbmi.biHeight      = (LONG)vse->GetHeight();
									pbmi.biCompression = FCC('HVC1');
									pbmi.biPlanes      = 1;
									pbmi.biBitCount    = 24;
									pbmi.biSizeImage   = DIBSIZE(pbmi);

									vc_params_t params = { 0 };
									HEVCParser::ParseHEVCDecoderConfigurationRecord(data, size, params, false);

									CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, PictAR, data, size, params.profile, params.level, params.nal_length_size);
									mt.SetSampleSize(pbmi.biSizeImage);

									mts.clear();
									mts.push_back(mt);

									if (!di->GetDataSize()) {
										m_pFile->Seek(sample.GetOffset());
										CBaseSplitterFileEx::hevchdr h;
										CMediaType mt2;
										if (m_pFile->Read(h, sample.GetSize(), &mt2)) {
											mts.insert(mts.cbegin(), mt2);
										}
									} else if (di->GetDataSize() == 23) {
										m_pFile->Seek(sample.GetOffset());
										AP4_DataBuffer data;
										if (AP4_SUCCEEDED(sample.ReadData(data))) {
											std::vector<uint8_t> new_record;
											if (HEVCParser::ReconstructHEVCDecoderConfigurationRecord(data.GetData(), data.GetDataSize(), params.nal_length_size,
																									  di->GetData(), di->GetDataSize(),
																									  new_record)) {
												CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, PictAR, new_record.data(), new_record.size(),
																	params.profile, params.level, params.nal_length_size);
												mts.clear();
												mts.push_back(mt);
											}
										}
									}
								}
								break;
							case AP4_ATOM_TYPE_VVC1:
							case AP4_ATOM_TYPE_VVI1:
								{ // H.266/VVC
									const AP4_DataBuffer* di = nullptr;
									if (AP4_DataInfoAtom* vvcC = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_VVCC))) {
										di = vvcC->GetData();
									}
									if (!di) {
										di = &empty;
									}

									BYTE* data  = (BYTE*)di->GetData();
									size_t size = (size_t)di->GetDataSize();
									if (size > 4) {
										size -= 4;
										data += 4;

										BITMAPINFOHEADER pbmi = {};
										pbmi.biSize        = sizeof(pbmi);
										pbmi.biWidth       = (LONG)vse->GetWidth();
										pbmi.biHeight      = (LONG)vse->GetHeight();
										pbmi.biCompression = FCC('VVC1');
										pbmi.biPlanes      = 1;
										pbmi.biBitCount    = 24;
										pbmi.biSizeImage   = DIBSIZE(pbmi);

										CreateMPEG2VISimple(&mt, &pbmi, AvgTimePerFrame, PictAR, data, size);
										mt.SetSampleSize(pbmi.biSizeImage);

										mts.clear();
										mts.push_back(mt);
									}
								}
								break;
							case AP4_ATOM_TYPE_vp08:
							case AP4_ATOM_TYPE_vp09:
								if (AP4_DataInfoAtom* vpcC = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_VPCC))) {
									const AP4_DataBuffer* di = vpcC->GetData();
									if (di->GetDataSize() >= 12) {
										vih2 = (VIDEOINFOHEADER2*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2) + di->GetDataSize() + 4);
										BYTE *extra = (BYTE*)(vih2 + 1);
										memcpy(extra, "vpcC", 4);
										memcpy(extra + 4, di->GetData(), di->GetDataSize());

										mts.clear();
										mt.subtype = FOURCCMap(vih2->bmiHeader.biCompression = fourcc);
										mts.push_back(mt);
									}
								}
								break;
							case AP4_ATOM_TYPE_av01:
								{
									bool bReadAV1C = false;
									// https://aomediacodec.github.io/av1-isobmff/#av1codecconfigurationbox-section
									if (AP4_DataInfoAtom* av1C = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_AV1C))) {
										const AP4_DataBuffer* di = av1C->GetData();
										if (di->GetDataSize() >= 4) {
											const auto version = di->GetData()[0];
											if (version != 0x81) { // marker = 1(1), version = 1(7)
												DLog(L"CMP4SplitterFilter::CreateOutputs() : Unknown AV1 Codec Configuration Box version - 0x%02x", version);
											} else {
												vih2 = (VIDEOINFOHEADER2*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2) + di->GetDataSize());
												BYTE* extra = (BYTE*)(vih2 + 1);
												memcpy(extra, di->GetData(), di->GetDataSize());

												mt.subtype = FOURCCMap(vih2->bmiHeader.biCompression = fourcc);
												mts.insert(mts.cbegin(), mt);

												bReadAV1C = true;
											}
										}
									}

									if (!bReadAV1C) {
										AP4_DataBuffer data;
										if (AP4_SUCCEEDED(sample.ReadData(data))) {
											AV1Parser::AV1SequenceParameters seq_params;
											std::vector<uint8_t> obu_sequence_header;
											if (AV1Parser::ParseOBU(data.GetData(), data.GetDataSize(), seq_params, obu_sequence_header)) {
												vih2 = (VIDEOINFOHEADER2*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2) + 4 + obu_sequence_header.size());
												BYTE* extra = (BYTE*)(vih2 + 1);

												CBitsWriter bw(extra, 4);
												bw.writeBits(1, 1); // marker
												bw.writeBits(7, 1); // version
												bw.writeBits(3, seq_params.profile);
												bw.writeBits(5, seq_params.level);
												bw.writeBits(1, seq_params.tier);
												bw.writeBits(1, seq_params.bitdepth > 8);
												bw.writeBits(1, seq_params.bitdepth == 12);
												bw.writeBits(1, seq_params.monochrome);
												bw.writeBits(1, seq_params.chroma_subsampling_x);
												bw.writeBits(1, seq_params.chroma_subsampling_y);
												bw.writeBits(2, seq_params.chroma_sample_position);
												bw.writeBits(8, 0); // padding

												memcpy(extra + 4, obu_sequence_header.data(), obu_sequence_header.size());

												mts.insert(mts.cbegin(), mt);
											}
										}
									}
								}
								break;
							case AP4_ATOM_TYPE_AVdn:
							case AP4_ATOM_TYPE_AVdh:
								{
									AP4_DataBuffer data;
									if (AP4_SUCCEEDED(sample.ReadData(data))) {
										const auto size = data.GetDataSize();
										if (size >= 0x280) {
											const auto buf = data.GetData();
											uint8_t bitdepth = 0;
											switch (buf[0x21] >> 5) {
												case 1: bitdepth = 8;  break;
												case 2: bitdepth = 10; break;
												case 3: bitdepth = 12; break;
											}
											if (bitdepth) {
												const bool b444Format = (buf[0x2C] >> 6) & 1;
												if (b444Format) {
													const bool act = buf[0x2C] & 1;
													if (bitdepth == 10) {
														m_pix_fmt = act ? AV_PIX_FMT_YUV444P10 : AV_PIX_FMT_GBRP10;
													} else if (bitdepth == 12) {
														m_pix_fmt = act ? AV_PIX_FMT_YUV444P12 : AV_PIX_FMT_GBRP12;
													}
												} else if (bitdepth == 12) {
													m_pix_fmt = AV_PIX_FMT_YUV422P12;
												} else if (bitdepth == 10) {
													m_pix_fmt = AV_PIX_FMT_YUV422P10;
												} else {
													m_pix_fmt = AV_PIX_FMT_YUV422P;
												}
											}
										}
									}
								}

								break;
						}

						if (AP4_DataInfoAtom* clap = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_CLAP))) {
							const AP4_DataBuffer* clap_data = clap->GetData();
							if (clap_data->GetDataSize() == 32) { // 40 bytes(size) - 8 bytes(header)
								const uint32_t* data = (uint32_t*)clap_data->GetData();
								if (data[1] && data[3] && data[5] && data[7]) {
									const double apertureWidth  = (double)_byteswap_ulong(data[0]) / _byteswap_ulong(data[1]);
									const double apertureHeight = (double)_byteswap_ulong(data[2]) / _byteswap_ulong(data[3]);
									const double horizOff = (double)_byteswap_ulong(data[4]) / _byteswap_ulong(data[5]);
									const double vertOff  = (double)_byteswap_ulong(data[6]) / _byteswap_ulong(data[7]);

									const double x = ((double)vse->GetWidth() - apertureWidth)/2 + horizOff;
									const double y = ((double)vse->GetHeight() - apertureHeight)/2 + vertOff;
									for (auto& item : mts) {
										if (item.formattype == FORMAT_VideoInfo
												|| item.formattype == FORMAT_VideoInfo2
												|| item.formattype == FORMAT_MPEG2Video
												|| item.formattype == FORMAT_MPEGVideo) {
											auto vih = (VIDEOINFOHEADER*)item.Format();
											vih->rcSource = vih->rcTarget = {
												(LONG)std::round(x),
												(LONG)std::round(y),
												(LONG)std::round(x + apertureWidth),
												(LONG)std::round(y + apertureHeight)
											};
										}
									}
								} else {
									ASSERT(false);
								}
							}
						}

						if (AP4_DataInfoAtom* colr = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_COLR))) {
							const AP4_DataBuffer* colr_data = colr->GetData();
							if (colr_data->GetDataSize() > 4) {
								const auto color_parameter_type = GETU32(colr_data->GetData());
								if (color_parameter_type == FCC('nclc') || color_parameter_type == FCC('nclx')) {
									if (colr_data->GetDataSize() >= 10) {
										const auto color_primaries = _byteswap_ushort(GETU16(colr_data->GetData() + 4));
										const auto color_transfer_function = _byteswap_ushort(GETU16(colr_data->GetData() + 6));
										const auto color_matrix = _byteswap_ushort(GETU16(colr_data->GetData() + 8));
										auto color_range = AVCOL_RANGE_MPEG;
										if (color_parameter_type == FCC('nclx') && colr_data->GetDataSize() >= 11) {
											const auto range = colr_data->GetData()[10] >> 7;
											if (range) {
												color_range = AVCOL_RANGE_JPEG;
											}
										}

										m_ColorSpace = DNew(ColorSpace);
										ZeroMemory(m_ColorSpace, sizeof(ColorSpace));
										if (color_matrix != AVCOL_SPC_RESERVED) {
											m_ColorSpace->MatrixCoefficients = color_matrix;
										}
										if (color_primaries != AVCOL_PRI_RESERVED
												&& color_primaries != AVCOL_PRI_RESERVED0) {
											m_ColorSpace->Primaries = color_primaries;
										}
										m_ColorSpace->Range = color_range;
										if (color_transfer_function != AVCOL_TRC_RESERVED
												&& color_transfer_function != AVCOL_TRC_RESERVED0) {
											m_ColorSpace->TransferCharacteristics = color_transfer_function;
										}
									}
								}
							}
						}

						if (AP4_DataInfoAtom* mdcv = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_MDCV))) {
							const AP4_DataBuffer* mdcv_data = mdcv->GetData();
							if (mdcv_data->GetDataSize() == 24) {
								m_MasterDataHDR = DNew(MediaSideDataHDR);
								ZeroMemory(m_MasterDataHDR, sizeof(MediaSideDataHDR));

								auto data = mdcv_data->GetData();

								m_MasterDataHDR->display_primaries_x[0] = AV_RB16(data)     * (1.0/50000);
								m_MasterDataHDR->display_primaries_y[0] = AV_RB16(data + 2) * (1.0/50000);
								m_MasterDataHDR->display_primaries_x[1] = AV_RB16(data + 4) * (1.0/50000);
								m_MasterDataHDR->display_primaries_y[1] = AV_RB16(data + 6) * (1.0/50000);
								m_MasterDataHDR->display_primaries_x[2] = AV_RB16(data + 8) * (1.0/50000);
								m_MasterDataHDR->display_primaries_y[2] = AV_RB16(data + 10)* (1.0/50000);

								m_MasterDataHDR->white_point_x = AV_RB16(data + 12) * (1.0/50000);
								m_MasterDataHDR->white_point_y = AV_RB16(data + 14) * (1.0/50000);
								m_MasterDataHDR->max_display_mastering_luminance = AV_RB32(data + 16) * (1.0/10000);
								m_MasterDataHDR->min_display_mastering_luminance = AV_RB32(data + 20) * (1.0/10000);
							}
						}

						if (AP4_DataInfoAtom* clli = dynamic_cast<AP4_DataInfoAtom*>(vse->GetChild(AP4_ATOM_TYPE_CLLI))) {
							const AP4_DataBuffer* clli_data = clli->GetData();
							if (clli_data->GetDataSize() == 4) {
								m_HDRContentLightLevel = DNew(MediaSideDataHDRContentLightLevel);
								ZeroMemory(m_HDRContentLightLevel, sizeof(MediaSideDataHDRContentLightLevel));

								auto data = clli_data->GetData();

								m_HDRContentLightLevel->MaxCLL  = AV_RB16(data);
								m_HDRContentLightLevel->MaxFALL = AV_RB16(data + 2);
							}
						}

						break;
					}
					// AP4_AudioSampleEntry
					else if (auto ase = dynamic_cast<AP4_AudioSampleEntry*>(atom)) {
						DWORD fourcc        = _byteswap_ulong(ase->GetType());
						DWORD samplerate    = ase->GetSampleRate();
						WORD  channels      = ase->GetChannelCount();
						WORD  bitspersample = ase->GetSampleSize();

						// overwrite audio fourc
						if ((type & 0xffff0000) == AP4_ATOM_TYPE('m', 's', 0, 0)) {
							fourcc = type & 0xffff;
						} else if (type == AP4_ATOM_TYPE_ALAW) {
							fourcc = WAVE_FORMAT_ALAW;
						} else if (type == AP4_ATOM_TYPE_ULAW) {
							fourcc = WAVE_FORMAT_MULAW;
						} else if (type == AP4_ATOM_TYPE__MP3) {
							fourcc = WAVE_FORMAT_MPEGLAYER3;
						} else if (type == AP4_ATOM_TYPE_ac_3 || type == AP4_ATOM_TYPE_AC_3 || type == AP4_ATOM_TYPE_SAC3) {
							fourcc = WAVE_FORMAT_DOLBY_AC3;
						} else if (type == AP4_ATOM_TYPE_EAC3) {
							fourcc = WAVE_FORMAT_UNKNOWN;
						} else if (type == AP4_ATOM_TYPE_MP4A) {
							fourcc = WAVE_FORMAT_RAW_AAC1;
						} else if (type == AP4_ATOM_TYPE_NMOS) {
							fourcc = MAKEFOURCC('N','E','L','L');
						} else if (type == AP4_ATOM_TYPE_ALAC) {
							fourcc = MAKEFOURCC('a','l','a','c');
						} else if (type == AP4_ATOM_TYPE_SPEX) {
							fourcc = 0xa109;
						} else if ((type == AP4_ATOM_TYPE_NONE || type == AP4_ATOM_TYPE_RAW) && bitspersample == 8 ||
								    type == AP4_ATOM_TYPE_SOWT && bitspersample == 16 ||
								   (type == AP4_ATOM_TYPE_IN24 || type == AP4_ATOM_TYPE_IN32) && ase->GetEndian() == ENDIAN_LITTLE) {
							if (type == AP4_ATOM_TYPE_IN24) {
								bitspersample = 24;
							} else if (type == AP4_ATOM_TYPE_IN32) {
								bitspersample = 32;
							}
							fourcc = type = WAVE_FORMAT_PCM;
						} else if ((type == AP4_ATOM_TYPE_FL32 || type == AP4_ATOM_TYPE_FL64) && ase->GetEndian() == ENDIAN_LITTLE) {
							fourcc = type = WAVE_FORMAT_IEEE_FLOAT;
						} else if (type == AP4_ATOM_TYPE_LPCM || type == AP4_ATOM_TYPE_IPCM) {
							DWORD flags = ase->GetFormatSpecificFlags();
							const bool floating = (type == AP4_ATOM_TYPE_LPCM) && (flags & 1); // floating point
							const bool bBigEndian = (flags & 2) || (type == AP4_ATOM_TYPE_IPCM && ase->GetEndian() != ENDIAN_LITTLE);
							if (bBigEndian) {
								if (floating) {
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
							} else {
								if (floating) {
									fourcc = type = WAVE_FORMAT_IEEE_FLOAT;
								} else {
									fourcc = type = WAVE_FORMAT_PCM;
								}
							}
						} else if (type == AP4_ATOM_TYPE_FLAC) {
							fourcc = WAVE_FORMAT_FLAC;
						} else if (type == AP4_ATOM_TYPE_Opus) {
							fourcc = WAVE_FORMAT_OPUS;
						} else if (type == AP4_ATOM_TYPE_DTSC || type == AP4_ATOM_TYPE_DTSE
								|| type == AP4_ATOM_TYPE_DTSH || type == AP4_ATOM_TYPE_DTSL) {
							fourcc = type = WAVE_FORMAT_DTS2;
						} else if (type == AP4_ATOM_TYPE_MP2) {
							fourcc = WAVE_FORMAT_MPEG;
						} else if (type == AP4_ATOM_TYPE_mlpa) {
							fourcc = WAVE_FORMAT_UNKNOWN;
						}

						mt.majortype = MEDIATYPE_Audio;
						mt.formattype = FORMAT_WaveFormatEx;
						wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
						memset(wfe, 0, mt.FormatLength());
						if (!(fourcc & 0xffff0000)) {
							wfe->wFormatTag = (WORD)fourcc;
						}
						wfe->nSamplesPerSec  = samplerate;
						wfe->nChannels       = channels;
						wfe->wBitsPerSample  = bitspersample;
						wfe->nBlockAlign     = ase->GetBytesPerFrame();
						wfe->nAvgBytesPerSec = ase->GetSamplesPerPacket() ? wfe->nSamplesPerSec * wfe->nBlockAlign / ase->GetSamplesPerPacket() : 0;

						mt.subtype = FOURCCMap(fourcc);
						if (type == AP4_ATOM_TYPE_EAC3) {
							mt.subtype = MEDIASUBTYPE_DOLBY_DDPLUS;

							m_pFile->Seek(sample.GetOffset());
							AP4_DataBuffer data;
							if (AP4_SUCCEEDED(sample.ReadData(data)) && data.GetDataSize() >= 12) {
								audioframe_t aframe;
								if (ParseEAC3Header(data.GetData(), &aframe, static_cast<int>(data.GetDataSize())) && aframe.param2) {
									wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 1);
									wfe->cbSize = 1;
									(reinterpret_cast<BYTE*>(wfe + 1))[0] = 1;
								}
							}
						} else if (type == AP4_ATOM_TYPE_mlpa) {
							mt.subtype = MEDIASUBTYPE_DOLBY_TRUEHD;

							m_pFile->Seek(sample.GetOffset());
							AP4_DataBuffer data;
							if (AP4_SUCCEEDED(sample.ReadData(data)) && data.GetDataSize() >= 22) {
								audioframe_t aframe;
								if (ParseMLPHeader(data.GetData(), &aframe) && aframe.param3) {
									wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 1);
									wfe->cbSize = 1;
									(reinterpret_cast<BYTE*>(wfe + 1))[0] = 1;
								}
							}
						} else if (type == AP4_ATOM_TYPE('m', 's', 0x00, 0x02)) {
							const WORD numcoef = 7;
							static ADPCMCOEFSET coef[] = { {256, 0}, {512, -256}, {0,0}, {192,64}, {240,0}, {460, -208}, {392,-232} };
							const ULONG size = sizeof(ADPCMWAVEFORMAT) + (numcoef * sizeof(ADPCMCOEFSET));
							ADPCMWAVEFORMAT* format = (ADPCMWAVEFORMAT*)mt.ReallocFormatBuffer(size);
							if (format != nullptr) {
								format->wfx.wFormatTag = WAVE_FORMAT_ADPCM;
								format->wfx.wBitsPerSample = 4;
								format->wfx.cbSize = (WORD)(size - sizeof(WAVEFORMATEX));
								format->wSamplesPerBlock = format->wfx.nBlockAlign * 2 / format->wfx.nChannels - 12;
								format->wNumCoef = numcoef;
								memcpy( format->aCoef, coef, sizeof(coef) );
							}
						} else if (type == AP4_ATOM_TYPE('m', 's', 0x00, 0x11)) {
							IMAADPCMWAVEFORMAT* format = (IMAADPCMWAVEFORMAT*)mt.ReallocFormatBuffer(sizeof(IMAADPCMWAVEFORMAT));
							if (format != nullptr) {
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
								if ((GETU32(data) == 0x24000000) && (GETU32(data+4) == 0x63616c61)) {
									break;
								}
								size--;
								data++;
							}

							if (size >= 36) {
								channels   = AV_RB8(data + 21);
								samplerate = AV_RB32(data + 32);

								wfe->nSamplesPerSec  = samplerate;
								wfe->nChannels       = channels;
								wfe->nAvgBytesPerSec = ase->GetSamplesPerPacket() ? wfe->nSamplesPerSec * wfe->nBlockAlign / ase->GetSamplesPerPacket() : 0;

								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 36);
								wfe->cbSize = 36;
								memcpy(wfe+1, data, 36);
							}
						} else if (type == WAVE_FORMAT_PCM) {
							mt.SetSampleSize(wfe->nBlockAlign);
							if (channels > 2 || bitspersample > 16) {
								WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
								if (wfex != nullptr) {
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
								if (wfex != nullptr) {
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
						} else if (type == AP4_ATOM_TYPE_FLAC) {
							mt.subtype = MEDIASUBTYPE_FLAC_FRAMED;
							if (AP4_FLACSampleEntry* flacSample = dynamic_cast<AP4_FLACSampleEntry*>(atom)) {
								const AP4_DataBuffer* extraData = flacSample->GetData();
								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + extraData->GetDataSize());
								wfe->cbSize = extraData->GetDataSize();
								memcpy(wfe + 1, extraData->GetData(), extraData->GetDataSize());
							}
							mts.push_back(mt);

							mt.subtype = MEDIASUBTYPE_FLAC;
						} else if (type == WAVE_FORMAT_DTS2) {
							BYTE profile = 0;

							m_pFile->Seek(sample.GetOffset());
							AP4_DataBuffer data;
							if (AP4_SUCCEEDED(sample.ReadData(data))) {
								const BYTE* start = data.GetData();
								const BYTE* end = start + data.GetDataSize();
								audioframe_t aframe;
								const int size = ParseDTSHeader(start);
								if (size) {
									if (start + size + 40 <= end) {
										const int sizehd = ParseDTSHDHeader(start + size, end - start - size, &aframe);
										if (sizehd) {
											profile = aframe.param2;

											wfe->nSamplesPerSec = aframe.samplerate;
											wfe->nChannels = aframe.channels;
											if (aframe.param1) {
												wfe->wBitsPerSample = aframe.param1;
											}
											wfe->nBlockAlign = (wfe->nChannels * wfe->wBitsPerSample) / 8;
											if (aframe.param2 == DCA_PROFILE_HD_HRA || aframe.param2 == DCA_PROFILE_HD_HRA_X || aframe.param2 == DCA_PROFILE_HD_HRA_X_IMAX) {
												// DTS-HD High Resolution Audio
												wfe->nAvgBytesPerSec += CalcBitrate(aframe) / 8;
											} else {
												// DTS-HD Master Audio
												wfe->nAvgBytesPerSec = 0;
											}
										}
									}
								} else if (ParseDTSHDHeader(start, end - start, &aframe) && aframe.param2 == DCA_PROFILE_EXPRESS) {
									// DTS Express
									profile = aframe.param2;

									wfe->nSamplesPerSec = aframe.samplerate;
									wfe->nChannels = aframe.channels;
									wfe->wBitsPerSample = aframe.param1;
									wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample / 8;
									wfe->nAvgBytesPerSec = CalcBitrate(aframe) / 8;
								}
							}

							if (profile) {
								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 1);
								wfe->cbSize = 1;
								((BYTE *)(wfe + 1))[0] = profile;
							}
						} else if (type == AP4_ATOM_TYPE_Opus) {
							mt.subtype = MEDIASUBTYPE_OPUS;

							if (AP4_DataInfoAtom* dOpsAtom = dynamic_cast<AP4_DataInfoAtom*>(ase->GetChild(AP4_ATOM_TYPE_DOPS))) {
								const AP4_DataBuffer* di = dOpsAtom->GetData();
								if (di->GetDataSize() >= 11 && di->GetData()[0] == 0) {
									const BYTE* src = di->GetData() + 1;
									const size_t size = di->GetDataSize() + 8;
									std::vector<BYTE> extradata;
									extradata.resize(size);
									BYTE* dst = extradata.data();

									memcpy(dst, "OpusHead", 8);
									AV_WB8(dst + 8, 1); // OpusHead version
									memcpy(dst + 9, src, size - 9);
									AV_WL16(dst + 10, AV_RB16(dst + 10));
									AV_WL32(dst + 12, AV_RB32(dst + 12));
									AV_WL16(dst + 16, AV_RB16(dst + 16));

									wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + size);
									wfe->cbSize = (WORD)size;
									memcpy(wfe + 1, dst, size);
								}
							}
						} else if (type == AP4_ATOM_TYPE_MP2) {
							mt.subtype = MEDIASUBTYPE_MPEG2_AUDIO;

							m_pFile->Seek(sample.GetOffset());
							AP4_DataBuffer data;
							if (AP4_SUCCEEDED(sample.ReadData(data)) && data.GetDataSize() >= 4) {
								MPEG1WAVEFORMAT mpeg1wf = {};
								if (ParseMPEG1Header(data.GetData(), &mpeg1wf)) {
									MPEG1WAVEFORMAT* f = (MPEG1WAVEFORMAT*)mt.ReallocFormatBuffer(sizeof(MPEG1WAVEFORMAT));
									memcpy(f, &mpeg1wf, sizeof(MPEG1WAVEFORMAT));
								}
							}
						} else if (db.GetDataSize() > 0) {
							//always needed extra data for QDM2
							wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + db.GetDataSize());
							wfe->cbSize = db.GetDataSize();
							memcpy(wfe + 1, db.GetData(), db.GetDataSize());
						}

						iAudio++;
						outputDesc.Format(L"Audio %d", iAudio);
						mts.push_back(mt);

						if (AP4_DataInfoAtom* WfexAtom = dynamic_cast<AP4_DataInfoAtom*>(ase->GetChild(AP4_ATOM_TYPE_WFEX))) {
							const AP4_DataBuffer* di = WfexAtom->GetData();
							if (di->GetDataSize()) {
								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(di->GetDataSize());
								memcpy(wfe, di->GetData(), di->GetDataSize());

								mt.subtype = FOURCCMap(wfe->wFormatTag);
								mts.insert(mts.cbegin(), mt);
							}
						}

						if (AP4_DataInfoAtom* glblAtom = dynamic_cast<AP4_DataInfoAtom*>(ase->GetChild(AP4_ATOM_TYPE_GLBL))) {
							const AP4_DataBuffer* di = glblAtom->GetData();
							if (di->GetDataSize()) {
								wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + di->GetDataSize());
								wfe->cbSize = di->GetDataSize();
								memcpy(wfe + 1, di->GetData(), di->GetDataSize());

								mts.insert(mts.cbegin(), mt);
							}
						}

						break;
					}
					else {
						DLog(L"Unknow MP4 Stream %x" , type);
					}
				}
			}

			if (mts.empty()) {
				continue;
			}

			REFERENCE_TIME rtDuration = 10000i64 * track->GetDurationHighPrecision();
			if (m_rtDuration < rtDuration) {
				m_rtDuration = rtDuration;
			}
			if (rtVideoDuration < rtDuration && AP4_Track::TYPE_VIDEO == track->GetType())
				rtVideoDuration = rtDuration; // get the max video duration

			DWORD id = track->GetId();

			CStringW pinName;

			if (TrackLanguage.GetLength()) {
				if (TrackLanguage != "und") {
					pinName = ISO6392ToLanguage(TrackLanguage).Trim();
					CStringW lng(TrackLanguage);
					if (!StartsWithNoCase(pinName, lng)) {
						pinName.AppendFormat(L" (%s)", lng);
					}
					pinName.Append(L", ");
				}
			}

			if (TrackName.GetLength()) {
				pinName.Append(TrackName);
				if (outputDesc.GetLength()) {
					pinName.AppendFormat(L" (%s)", outputDesc);
				}
			} else {
				if (outputDesc.GetLength()) {
					pinName.Append(outputDesc);
				} else {
					pinName.AppendFormat(L"Output %d", id);
				}
			}

			if (AP4_Track::TYPE_VIDEO == track->GetType() && videoSize == CSize(0, 0)) {
				for (int i = 0, j = mts.size(); i < j; ++i) {
					BITMAPINFOHEADER bih;
					if (ExtractBIH(&mts[i], &bih)) {
						videoSize.cx = bih.biWidth;
						videoSize.cy = abs(bih.biHeight);
						break;
					}
				}
			}

			std::unique_ptr<CBaseSplitterOutputPin> pPinOut(DNew CMP4SplitterOutputPin(mts, pinName, this, this, &hr));

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

				CString ChapterName = UTF8orLocalToWStr(chapter.Name.c_str());
				ChapAppend(chapter.Time, ChapterName);
			}
		} else if (ChapterTrackEntries.ItemCount()) {
			for (AP4_List<AP4_Track>::Item* item = movie->GetTracks().FirstItem();
					item;
					item = item->GetNext()) {
				AP4_Track* track = item->GetData();
				if (track->GetType() != AP4_Track::TYPE_TEXT) {
					continue;
				}

				bool bFoundChapterTrack = false;
				for (AP4_Cardinal i = 0; i < ChapterTrackEntries.ItemCount(); i++) {
					const AP4_UI32& ChapterTrackId = ChapterTrackEntries[i];
					if (ChapterTrackId == track->GetId()) {
						bFoundChapterTrack = true;
						break;
					}
				}

				if (bFoundChapterTrack) {
					char* buff = nullptr;
					for (AP4_Cardinal i = 0; i < track->GetSampleCount(); i++) {
						AP4_Sample sample;
						AP4_DataBuffer data;
						track->ReadSample(i, sample, data);

						const AP4_Byte* ptr  = data.GetData();
						const AP4_Size avail = data.GetDataSize();
						CString ChapterName;

						if (avail > 2) {
							size_t size = (ptr[0] << 8) | ptr[1];
							if (size > avail - 2) {
								size = avail - 2;
							}

							bool bUTF16LE = false;
							bool bUTF16BE = false;
							if (avail > 4) {
								const WORD bom = (ptr[2] << 8) | ptr[3];
								if (bom == 0xfffe) {
									bUTF16LE = true;
								} else if (bom == 0xfeff) {
									bUTF16BE = true;
								}
							}

							if (bUTF16LE || bUTF16BE) {
								memcpy((BYTE*)ChapterName.GetBufferSetLength(size / 2), &ptr[2], size);
								if (bUTF16BE) {
									for (int i = 0, j = ChapterName.GetLength(); i < j; i++) {
										ChapterName.SetAt(i, (ChapterName[i] << 8) | (ChapterName[i] >> 8));
									}
								}
							} else {
								buff = DNew char[size + 1];
								memcpy(buff, &ptr[2], size);
								buff[size] = 0;

								ChapterName = UTF8orLocalToWStr(buff);

								SAFE_DELETE_ARRAY(buff);
							}

							const REFERENCE_TIME rtStart = RescaleI64x32(sample.GetCts(), UNITS, track->GetMediaTimeScale());
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
					if (atom->GetType() == AP4_ATOM_TYPE_COVR) {
						for (AP4_List<AP4_Atom>::Item* itemChild = atom->GetChildren().FirstItem();
							itemChild;
							itemChild = itemChild->GetNext()) {
							if (AP4_DataAtom* data = dynamic_cast<AP4_DataAtom*>(itemChild->GetData())) {
								const AP4_DataBuffer* db = data->GetData();

								if (db->GetDataSize() > 10) {
									const auto dataType = data->GetDataType();
									if (dataType == 0x1b) {
										ResAppend(L"cover.bmp", L"cover", L"image/bmp", (BYTE*)db->GetData(), (DWORD)db->GetDataSize());
									} else {
										DWORD sync = GETU32(db->GetData());
										if ((sync & 0x00ffffff) == 0x00FFD8FF) { // SOI segment + first byte of next segment
											ResAppend(L"cover.jpg", L"cover", L"image/jpeg", (BYTE*)db->GetData(), (DWORD)db->GetDataSize());
										} else if (sync == MAKEFOURCC(0x89, 'P', 'N', 'G')) {
											ResAppend(L"cover.png", L"cover", L"image/png", (BYTE*)db->GetData(), (DWORD)db->GetDataSize());
										}
									}
								}
							}
						}
					} else if (AP4_DataAtom* data = dynamic_cast<AP4_DataAtom*>(atom->GetChild(AP4_ATOM_TYPE_DATA))) {
						auto db = data->GetData();

						if (db->GetDataSize() > 0) {
							if (atom->GetType() == AP4_ATOM_TYPE_TRKN) {
								if (db->GetDataSize() >= 4) {
									unsigned short n = (db->GetData()[2] << 8) | db->GetData()[3];
									if (n > 0 && n < 100) {
										track.Format(L"%02d", n);
									} else if (n >= 100) {
										track.Format(L"%d", n);
									}
								}
							} else {
								CStringW str = UTF8ToWStr(CStringA((LPCSTR)db->GetData(), db->GetDataSize()));

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

		const AP4_Duration fragmentdurationms = movie->GetFragmentsDurationMs();
		if (fragmentdurationms) {
			// override duration from 'sidx' atom
			const REFERENCE_TIME rtDuration = 10000i64 * fragmentdurationms;
			m_rtDuration = rtVideoDuration = rtDuration;
		}
	}

	if (rtVideoDuration > 0 && rtVideoDuration < m_rtDuration / 2) {
		m_rtDuration = rtVideoDuration; // fix incorrect duration
	}

	m_rtNewStop = m_rtStop = m_rtDuration;

	if (m_pOutputs.size()) {
		AP4_Movie* movie = m_pFile->GetMovie();

		for (auto& [id, tp] : m_trackpos) {
			AP4_Track* track = movie->GetTrack(id);

			AP4_Sample sample;
			if (AP4_SUCCEEDED(track->GetSample(0, sample))) {
				const auto rt = RescaleI64x32(sample.GetCts(), UNITS, track->GetMediaTimeScale());
				m_rtMovieOffset = std::min(m_rtMovieOffset, std::max(rt, 0i64));
			}
		}
		if (m_rtMovieOffset == MAXLONGLONG) {
			m_rtMovieOffset = 0;
		}

		if (movie->HasFragmentsIndex()) {
			const AP4_Array<AP4_IndexTableEntry>& entries = movie->GetFragmentsIndexEntries();
			for (AP4_Cardinal i = 0; i < entries.ItemCount(); ++i) {
				const SyncPoint sp = { entries[i].m_rt - m_rtMovieOffset, __int64(entries[i].m_offset) };
				m_sps.push_back(sp);
			}
		} else {
			for (const auto& tp : m_trackpos) {
				AP4_Track* track = movie->GetTrack(tp.first);

				if (!track->HasIndex()) {
					continue;
				}

				const AP4_Array<AP4_IndexTableEntry>& entries = track->GetIndexEntries();
				for (AP4_Cardinal i = 0; i < entries.ItemCount(); ++i) {
					const SyncPoint sp = { entries[i].m_rt - m_rtMovieOffset, __int64(entries[i].m_offset) };
					m_sps.push_back(sp);
				}

				break;
			}
		}
	}

	SetID3TagProperties(this, m_pFile->m_pID3Tag);

	return m_pOutputs.size() > 0 ? S_OK : E_FAIL;
}

bool CMP4SplitterFilter::DemuxInit()
{
	AP4_Movie* movie = m_pFile->GetMovie();

	for (auto& [id, tp] : m_trackpos) {
		tp.index = 0;
		tp.ts = 0;
		tp.offset = 0;

		AP4_Track* track = movie->GetTrack(id);

		AP4_Sample sample;
		if (AP4_SUCCEEDED(track->GetSample(0, sample))) {
			tp.ts = sample.GetCts();
			tp.offset = sample.GetOffset();
		}
	}

	return true;
}

void CMP4SplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (m_pFile->IsStreaming() || m_rtDuration <= 0) {
		return;
	}

	AP4_Movie* movie = m_pFile->GetMovie();

	if (movie->HasFragmentsIndex()
			&& (AP4_FAILED(movie->SelectMoof(rt)))) {
		bSelectMoofSuccessfully = FALSE;
		return;
	}
	bSelectMoofSuccessfully = TRUE;

	if (rt <= 0 || movie->HasFragmentsIndex()) {
		DemuxInit();
		return;
	}

	for (auto& [id, tp] : m_trackpos) {
		AP4_Track* track = movie->GetTrack(id);

		if (track->HasIndex() && track->GetIndexEntries().ItemCount()) {
			if (AP4_FAILED(track->GetIndexForRefTime(rt + m_rtMovieOffset, tp.index, tp.ts, tp.offset))) {
				continue;
			}
		} else {
			if (AP4_FAILED(track->GetSampleIndexForRefTime(rt, tp.index))) {
				continue;
			}

			AP4_Sample sample;
			if (AP4_SUCCEEDED(track->GetSample(tp.index, sample))) {
				tp.ts = sample.GetCts();
				tp.offset = sample.GetOffset();
			}
		}
	}
}

bool CMP4SplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	if (!bSelectMoofSuccessfully) {
		return true;
	}

	m_pFile->Seek(0);
	AP4_Movie* movie = m_pFile->GetMovie();

	const auto rtDiffMaximum = MILLISECONDS_TO_100NS_UNITS(m_iQueueDuration);

	while (SUCCEEDED(hr) && !CheckRequest(nullptr)) {
		std::pair<const DWORD, trackpos>* pNext = nullptr;
		REFERENCE_TIME rtNext = _I64_MAX;
		ULONGLONG nextOffset = 0;

		for (auto& tp : m_trackpos) {
			AP4_Track* track = movie->GetTrack(tp.first);

			CBaseSplitterOutputPin* pPin = GetOutputPin((DWORD)track->GetId());
			if (!pPin->IsConnected()) {
				continue;
			}

			if (tp.second.index < track->GetSampleCount()) {
				const REFERENCE_TIME rt = RescaleI64x32(tp.second.ts, UNITS, track->GetMediaTimeScale());
				auto const rtDiff = llabs(rtNext - rt);
				if (!pNext
						|| (rtDiff <= rtDiffMaximum && tp.second.offset < nextOffset)
						|| (rtDiff > rtDiffMaximum && rt < rtNext)) {
					pNext = &tp;
					nextOffset = tp.second.offset;
					rtNext = rt;
				}
			}
		}

		if (!pNext) {
			if (movie->HasFragmentsIndex()
					&& (AP4_SUCCEEDED(movie->SwitchNextMoof()))) {
				DemuxInit();
				continue;
			} else {
				break;
			}
		}

		AP4_Track* track = movie->GetTrack(pNext->first);

		CBaseSplitterOutputPin* pPin = GetOutputPin((DWORD)track->GetId());

		AP4_Sample sample;
		AP4_DataBuffer data;

		if (pPin && pPin->IsConnected() && AP4_SUCCEEDED(track->ReadSample(pNext->second.index, sample, data))) {
			const CMediaType& mt = pPin->CurrentMediaType();

			std::unique_ptr<CPacket> p(DNew CPacket());
			p->TrackNumber = (DWORD)track->GetId();
			p->rtStart = RescaleI64x32(sample.GetCts(), UNITS, track->GetMediaTimeScale());
			p->rtStop = RescaleI64x32(sample.GetCts() + sample.GetDuration(), UNITS, track->GetMediaTimeScale());
			if (p->rtStop == p->rtStart && p->rtStart != INVALID_TIME) {
				p->rtStop++;
			}
			p->bSyncPoint = sample.IsSync();

			REFERENCE_TIME duration = p->rtStop - p->rtStart;

			if (track->GetType() == AP4_Track::TYPE_AUDIO
					&& mt.subtype != MEDIASUBTYPE_RAW_AAC1
					&& mt.subtype != MEDIASUBTYPE_Vorbis2
					&& mt.subtype != MEDIASUBTYPE_OPUS
					&& duration < 100000) { // duration < 10 ms (hack for PCM, ADPCM, Law and other)

				p->SetData(data.GetData(), data.GetDataSize());

				while (duration < 500000 && AP4_SUCCEEDED(track->ReadSample(pNext->second.index + 1, sample, data))) {
					size_t size = p->size();
					p->resize(size + data.GetDataSize());
					memcpy(p->data() + size, data.GetData(), data.GetDataSize());

					p->rtStop = RescaleI64x32(sample.GetCts() + sample.GetDuration(), UNITS, track->GetMediaTimeScale());

					duration = p->rtStop - p->rtStart;

					pNext->second.index++;
				}
			}
			else if (track->GetType() == AP4_Track::TYPE_TEXT) {
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
							str = WStrToUTF8(wstr);
						} else {
							str = CStringA((LPCSTR)&ptr[2], size);
						}

						CStringA dlgln = str;
						dlgln.Replace("\r", "");
						dlgln.Replace("\n", "\\N");

						p->SetData((LPCSTR)dlgln, dlgln.GetLength());
					}
				}
			}
			else {
				p->SetData(data.GetData(), data.GetDataSize());
			}

			p->rtStart -= m_rtMovieOffset;
			p->rtStop -= m_rtMovieOffset;
			hr = DeliverPacket(std::move(p));
		}

		{
			AP4_Sample sample;
			if (AP4_SUCCEEDED(track->GetSample(++pNext->second.index, sample))) {
				pNext->second.ts = sample.GetCts();
				pNext->second.offset = sample.GetOffset();
			}
		}

		if (FAILED(m_pFile->GetLastReadError())) {
			break;
		}
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CMP4SplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);

	nKFs = m_sps.size();
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

	for (nKFs = 0; nKFs < m_sps.size(); nKFs++) {
		pKFs[nKFs] = m_sps[nKFs].rt;
	}

	return S_OK;
}

// IExFilterInfo

STDMETHODIMP CMP4SplitterFilter::GetPropertyInt(LPCSTR field, int *value)
{
	CheckPointer(value, E_POINTER);

	if (!strcmp(field, "VIDEO_INTERLACED")) {
		if (m_interlaced != -1) {
			*value = m_interlaced;
			return S_OK;
		}
		return E_ABORT;
	}
	else if (!strcmp(field, "VIDEO_PIXEL_FORMAT")) {
		if (m_pix_fmt != -1) {
			*value = m_pix_fmt;
			return S_OK;
		}
		return E_ABORT;
	}
	else if (!strcmp(field, "VIDEO_FLAG_ONLY_DTS")) {
		*value = m_dtsonly;
		return S_OK;
	} else if (!strcmp(field, "VIDEO_DELAY")) {
		if (m_video_delay != 0) {
			*value = m_video_delay;
			return S_OK;
		}
		return E_ABORT;
	}

	return E_INVALIDARG;
}

STDMETHODIMP CMP4SplitterFilter::GetPropertyBin(LPCSTR field, LPVOID *value, unsigned *size)
{
	CheckPointer(value, E_POINTER);
	CheckPointer(size, E_POINTER);

	if (!strcmp(field, "PALETTE")) {
		if (m_bHasPalette) {
			*size = sizeof(m_Palette);
			*value = (LPVOID)LocalAlloc(LPTR, *size);
			memcpy(*value, m_Palette, *size);

			return S_OK;
		}
		return E_ABORT;
	}

	if (!strcmp(field, "VIDEO_COLOR_SPACE")) {
		if (m_ColorSpace) {
			*size = sizeof(*m_ColorSpace);
			*value = (LPVOID)LocalAlloc(LPTR, *size);
			memcpy(*value, m_ColorSpace, *size);

			return S_OK;
		}
		return E_ABORT;
	}

	if (!strcmp(field, "HDR_MASTERING_METADATA")) {
		if (m_MasterDataHDR) {
			*size = sizeof(*m_MasterDataHDR);
			*value = (LPVOID)LocalAlloc(LPTR, *size);
			memcpy(*value, m_MasterDataHDR, *size);

			return S_OK;
		}
		return E_ABORT;
	}

	if (!strcmp(field, "HDR_CONTENT_LIGHT_LEVEL")) {
		if (m_HDRContentLightLevel) {
			*size = sizeof(*m_HDRContentLightLevel);
			*value = (LPVOID)LocalAlloc(LPTR, *size);
			memcpy(*value, m_HDRContentLightLevel, *size);

			return S_OK;
		}
		return E_ABORT;
	}

	return E_INVALIDARG;
}

//
// CMP4SourceFilter
//

CMP4SourceFilter::CMP4SourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMP4SplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.reset();
}

//
// CMP4SplitterOutputPin
//

CMP4SplitterOutputPin::CMP4SplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
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

HRESULT CMP4SplitterOutputPin::DeliverPacket(std::unique_ptr<CPacket> p)
{
	CAutoLock cAutoLock(this);

	if (p && m_mt.subtype == MEDIASUBTYPE_VP90) {
		REFERENCE_TIME rtStartTmp = p->rtStart;
		REFERENCE_TIME rtStopTmp  = p->rtStop;

		const BYTE* pData = p->data();
		size_t size = p->size();

		const BYTE marker = pData[size - 1];
		if ((marker & 0xe0) == 0xc0) {
			HRESULT hr = S_OK;

			const BYTE nbytes = 1 + ((marker >> 3) & 0x3);
			BYTE n_frames = 1 + (marker & 0x7);
			const size_t idx_sz = 2 + n_frames * nbytes;
			if (size >= idx_sz && pData[size - idx_sz] == marker && nbytes >= 1 && nbytes <= 4) {
				const BYTE *idx = pData + size + 1 - idx_sz;

				while (n_frames--) {
					size_t sz = 0;
					switch(nbytes) {
						case 1: sz = (BYTE)*idx; break;
						case 2: sz = AV_RL16(idx); break;
						case 3: sz = AV_RL24(idx); break;
						case 4: sz = AV_RL32(idx); break;
					}

					idx += nbytes;
					if (sz > size || !sz) {
						break;
					}

					std::unique_ptr<CPacket> packet(DNew CPacket());
					packet->SetData(pData, sz);

					packet->TrackNumber    = p->TrackNumber;
					packet->bDiscontinuity = p->bDiscontinuity;
					packet->bSyncPoint     = p->bSyncPoint;

					if (pData[0] & 0x2) {
						packet->rtStart = rtStartTmp;
						packet->rtStop  = rtStopTmp;
						rtStartTmp      = INVALID_TIME;
						rtStopTmp       = INVALID_TIME;
					}
					hr = __super::DeliverPacket(std::move(packet));
					if (S_OK != hr) {
						break;
					}

					pData += sz;
					size -= sz;
				}
			}

			return hr;
		}
	}

	return __super::DeliverPacket(std::move(p));
}
