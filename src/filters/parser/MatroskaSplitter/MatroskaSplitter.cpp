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
#include <MMReg.h>
#include "MatroskaSplitter.h"
#include "../BaseSplitter/TimecodeAnalyzer.h"
#include "../../../DSUtil/AudioParser.h"
#include "../../../DSUtil/MP4AudioDecoderConfig.h"
#include "../../../DSUtil/VideoParser.h"
#include "../../../DSUtil/GolombBuffer.h"
#include "../../../DSUtil/std_helper.h"
#include <IMediaSideData.h>

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>
#include <basestruct.h>
#include <cmath>
#include <list>
#include <libavutil/pixfmt.h>
#include <libavutil/intreadwrite.h>

// option names
#define OPT_REGKEY_MATROSKASplit	L"Software\\MPC-BE Filters\\Matroska Splitter"
#define OPT_SECTION_MATROSKASplit	L"Filters\\Matroska Splitter"
#define OPT_LoadEmbeddedFonts		L"LoadEmbeddedFonts"
#define OPT_CalcDuration			L"CalculateDuration"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_Matroska},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMatroskaSplitterFilter), MatroskaSplitterName, MERIT_NORMAL, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMatroskaSourceFilter), MatroskaSourceName, MERIT_NORMAL, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMatroskaSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMatroskaSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(
		__uuidof(CMatroskaSourceFilter),
		MEDIASUBTYPE_Matroska,
		L"0,4,,1A45DFA3",
		nullptr);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_Matroska);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#else

#include "../../../DSUtil/Profile.h"

#endif

//
// CMatroskaSplitterFilter
//

CMatroskaSplitterFilter::CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"CMatroskaSplitterFilter", pUnk, phr, __uuidof(this))
	, m_bLoadEmbeddedFonts(true)
	, m_bCalcDuration(false)
	, m_hasHdmvDvbSubPin(false)
	, m_Cluster_seek_rt(INVALID_TIME)
	, m_Cluster_seek_pos(0)
	, m_bSupportSubtitlesCueDuration(FALSE)
	, m_MasterDataHDR(nullptr)
	, m_HDRContentLightLevel(nullptr)
	, m_ColorSpace(nullptr)
	, m_profile(-1)
	, m_pix_fmt(-1)
	, m_bits(8)
	, m_interlaced(-1)
{
	m_nFlag |= SOURCE_SUPPORT_URL;
#ifdef REGISTER_FILTER
	CRegKey key;

	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_MATROSKASplit, KEY_READ)) {
		DWORD dw;

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_LoadEmbeddedFonts, dw)) {
			m_bLoadEmbeddedFonts = !!dw;
		}

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_CalcDuration, dw)) {
			m_bCalcDuration = !!dw;
		}
	}
#else
	CProfile& profile = AfxGetProfile();
	profile.ReadBool(OPT_SECTION_MATROSKASplit, OPT_LoadEmbeddedFonts, m_bLoadEmbeddedFonts);
	profile.ReadBool(OPT_SECTION_MATROSKASplit, OPT_CalcDuration, m_bCalcDuration);
#endif
}

CMatroskaSplitterFilter::~CMatroskaSplitterFilter()
{
	SAFE_DELETE(m_MasterDataHDR);
	SAFE_DELETE(m_HDRContentLightLevel);
	SAFE_DELETE(m_ColorSpace);
}

bool CMatroskaSplitterFilter::IsHdmvDvbSubPinDrying()
{
	if (m_hasHdmvDvbSubPin) {
		for (const auto pPin : m_pActivePins) {
			if (pPin->QueueCount() == 0 && ((CMatroskaSplitterOutputPin*)pPin)->NeedNextSubtitle()) {
				return true;
			}
		}
	}

	return false;
}

STDMETHODIMP CMatroskaSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(ITrackInfo)
		QI(IExFilterInfo)
		QI(IMatroskaSplitterFilter)
		QI(ISpecifyPropertyPages)
		QI(ISpecifyPropertyPages2)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CMatroskaSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, MatroskaSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

#define MATROSKA_VIDEO_STEREOMODE 15
static const LPWSTR matroska_stereo_mode[MATROSKA_VIDEO_STEREOMODE] = {
	L"mono",
	L"sbs_lr",
	L"tb_rl",
	L"tb_lr",
	L"checkerboard_rl",
	L"checkerboard_lr",
	L"row_interleaved_rl",
	L"row_interleaved_lr",
	L"col_interleaved_rl",
	L"col_interleaved_lr",
	L"anaglyph_cyan_red",
	L"sbs_rl",
	L"anaglyph_green_magenta",
	L"mvc_lr",
	L"mvc_rl",
};

static int enum_to_chroma_pos(int *xpos, int *ypos, int pos)
{
	if (pos <= AVCHROMA_LOC_UNSPECIFIED || pos >= AVCHROMA_LOC_NB)
		return -1;
	pos--;

	*xpos = (pos & 1) * 128;
	*ypos = ((pos >> 1) ^ (pos < 4)) * 128;

	return 0;
}

static int chroma_pos_to_enum(int xpos, int ypos)
{
	for (int pos = AVCHROMA_LOC_UNSPECIFIED + 1; pos < AVCHROMA_LOC_NB; pos++) {
		int xout, yout;
		if (enum_to_chroma_pos(&xout, &yout, pos) == 0 && xout == xpos && yout == ypos)
			return pos;
	}

	return AVCHROMA_LOC_UNSPECIFIED;
}

bool CMatroskaSplitterFilter::ReadFirtsBlock(std::vector<byte>& pData, TrackEntry* pTE)
{
	const __int64 pos = m_pFile->GetPos();

	CMatroskaNode Root(m_pFile);
	m_pSegment = Root.Child(MATROSKA_ID_SEGMENT);
	m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);

	Cluster c;
	c.ParseTimeCode(m_pCluster);

	if (!m_pBlock) {
		m_pBlock = m_pCluster->GetFirstBlock();
	}

	do {
		CBlockGroupNode bgn;

		if (m_pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
			bgn.Parse(m_pBlock, true);
		}
		else if (m_pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
			CAutoPtr<BlockGroup> bg(DNew BlockGroup());
			bg->Block.Parse(m_pBlock, true);
			bgn.emplace_back(bg);
		}

		for (const auto& bg : bgn) {
			if (bg->Block.TrackNumber != pTE->TrackNumber) {
				continue;
			}

			for (const auto& pb : bg->Block.BlockData) {
				pTE->Expand(*pb, ContentEncoding::AllFrameContents);
				pData.insert(pData.end(), pb->cbegin(), pb->cend());
			}

			break;
		}
	} while (m_pBlock->NextBlock() && !CheckRequest(nullptr) && pData.empty());

	m_pBlock.Free();
	m_pCluster.Free();

	m_pFile->Seek(pos);

	return !pData.empty();
}

HRESULT CMatroskaSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pTrackEntryMap.clear();
	m_pOrderedTrackArray.clear();

	std::vector<CMatroskaSplitterOutputPin*> pinOut;
	std::vector<TrackEntry*> pinOutTE;

	m_pFile.Attach(DNew CMatroskaFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	CMatroskaNode Root(m_pFile);
	if (!m_pFile
			|| !(m_pSegment = Root.Child(MATROSKA_ID_SEGMENT))
			|| !(m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER))) {
		return E_FAIL;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	int iVideo = 1, iAudio = 1, iSubtitle = 1;
	bool bHasVideo = false;

	REFERENCE_TIME codecAvgTimePerFrame = 0;
	BOOL bInterlaced = FALSE;

	CString videoTrackName;

	for (const auto& pT : m_pFile->m_segment.Tracks) {
		for (const auto& pTE : pT->TrackEntries) {
			bool isSub = false;

			if (!pTE->Expand(pTE->CodecPrivate, ContentEncoding::TracksPrivateData)) {
				continue;
			}

			CStringA CodecID = pTE->CodecID.ToString();

			CString Name;
			Name.Format(L"Output %I64u", (UINT64)pTE->TrackNumber);

			CMediaType mt;
			std::vector<CMediaType> mts;

			if (pTE->TrackType == TrackEntry::TypeVideo && !bHasVideo) {
				Name.Format(L"Video %d", iVideo++);

				mt.majortype = MEDIATYPE_Video;
				mt.SetSampleSize(1); // variable frame size?
				mt.bFixedSizeSamples = FALSE; // most compressed video has a variable frame size
				mt.bTemporalCompression = TRUE; // stream may contain P or B frames

				videoTrackName = pTE->Name;

				if (CodecID == "V_MS/VFW/FOURCC") {
					mt.formattype = FORMAT_VideoInfo;
					VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.size() - sizeof(BITMAPINFOHEADER));
					memset(mt.Format(), 0, mt.FormatLength());
					memcpy(&pvih->bmiHeader, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					mt.subtype = FOURCCMap(pvih->bmiHeader.biCompression);
					switch (pvih->bmiHeader.biCompression) {
					case BI_RGB:
					case BI_BITFIELDS:
						mt.subtype =
							pvih->bmiHeader.biBitCount == 1 ? MEDIASUBTYPE_RGB1 :
							pvih->bmiHeader.biBitCount == 4 ? MEDIASUBTYPE_RGB4 :
							pvih->bmiHeader.biBitCount == 8 ? MEDIASUBTYPE_RGB8 :
							pvih->bmiHeader.biBitCount == 16 ? pvih->bmiHeader.biCompression == BI_RGB ? MEDIASUBTYPE_RGB555 : MEDIASUBTYPE_RGB565 :
							pvih->bmiHeader.biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
							pvih->bmiHeader.biBitCount == 32 ? MEDIASUBTYPE_ARGB32 :
							MEDIASUBTYPE_NULL;
						mt.bTemporalCompression = FALSE;
						pvih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); // fix mkvmerge bug (http://msdn.microsoft.com/en-us/library/windows/desktop/dd318229%28v=vs.85%29.aspx)
						break;
					//case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
					//case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
					case FCC('v210'):
						pvih->bmiHeader.biBitCount = 20; // fixed incorrect bitdepth (ffmpeg bug)
						mt.bTemporalCompression = FALSE;
						break;
					}

					pvih->bmiHeader.biSizeImage = DIBSIZE(pvih->bmiHeader);
					mt.SetSampleSize(pvih->bmiHeader.biSizeImage); // fixed frame size

					if (!bHasVideo) {
						mts.push_back(mt);
						if (mt.subtype == MEDIASUBTYPE_WVC1) {
							mt.subtype = MEDIASUBTYPE_WVC1_CYBERLINK;
							mts.insert(mts.cbegin(), mt);
						}

						if (mt.subtype == MEDIASUBTYPE_HM10) {
							std::vector<BYTE> pData;
							if (ReadFirtsBlock(pData, pTE)) {
								CBaseSplitterFileEx::hevchdr h;
								CMediaType mt2;
								if (m_pFile->CBaseSplitterFileEx::Read(h, pData, &mt2)) {
									mts.insert(mts.cbegin(), mt2);
								}
							}
						}

						if (mt.subtype == MEDIASUBTYPE_H264 || mt.subtype == MEDIASUBTYPE_h264) {
							m_dtsonly = (pTE->CodecPrivate.empty() || pTE->CodecPrivate.data()[0] != 1);
						}
					}
					bHasVideo = true;
				} else if (CodecID == "V_MPEG4/ISO/AVC") {
					if (pTE->CodecPrivate.size() > 9) {
						vc_params_t params;
						AVCParser::ParseSequenceParameterSet(pTE->CodecPrivate.data() + 9, pTE->CodecPrivate.size() - 9, params);
						codecAvgTimePerFrame = params.AvgTimePerFrame;
						bInterlaced = params.interlaced;
					}

					BITMAPINFOHEADER pbmi;
					memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
					pbmi.biSize			= sizeof(pbmi);
					pbmi.biWidth		= (LONG)pTE->v.PixelWidth;
					pbmi.biHeight		= (LONG)pTE->v.PixelHeight;
					pbmi.biCompression	= '1CVA';
					pbmi.biPlanes		= 1;
					pbmi.biBitCount		= 24;
					pbmi.biSizeImage	= DIBSIZE(pbmi);

					CSize aspect(pbmi.biWidth, pbmi.biHeight);
					ReduceDim(aspect);
					CreateMPEG2VIfromAVC(&mt, &pbmi, 0, aspect, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					if (!bHasVideo) {
						mts.push_back(mt);

						if (SUCCEEDED(CreateMPEG2VIfromMVC(&mt, &pbmi, 0, aspect, pTE->CodecPrivate.data(), pTE->CodecPrivate.size()))) {
							mts.insert(mts.cbegin(), mt);
						} else if (pTE->CodecPrivate.empty()) {
							std::vector<BYTE> pData;
							if (ReadFirtsBlock(pData, pTE)) {
								CBaseSplitterFileEx::avchdr h;
								CMediaType mt2;
								if (m_pFile->CBaseSplitterFileEx::Read(h, pData, &mt2)) {
									mts.insert(mts.cbegin(), mt2);
								}
							}
						}
					}
					bHasVideo = true;
				} else if (CodecID.Left(12) == "V_MPEG4/ISO/") {
					BITMAPINFOHEADER pbmi;
					memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
					pbmi.biSize			= sizeof(pbmi);
					pbmi.biWidth		= (LONG)pTE->v.PixelWidth;
					pbmi.biHeight		= (LONG)pTE->v.PixelHeight;
					pbmi.biCompression	= FCC('MP4V');
					pbmi.biPlanes		= 1;
					pbmi.biBitCount		= 24;
					pbmi.biSizeImage	= DIBSIZE(pbmi);

					CSize aspect(pbmi.biWidth, pbmi.biHeight);
					ReduceDim(aspect);
					CreateMPEG2VISimple(&mt, &pbmi, 0, aspect, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					if (!bHasVideo)
						mts.push_back(mt);
					bHasVideo = true;
				} else if (CodecID == "V_DIRAC") {
					mt.subtype = MEDIASUBTYPE_DiracVideo;
					mt.formattype = FORMAT_DiracVideoInfo;
					DIRACINFOHEADER* dvih = (DIRACINFOHEADER*)mt.AllocFormatBuffer(FIELD_OFFSET(DIRACINFOHEADER, dwSequenceHeader) + pTE->CodecPrivate.size());
					memset(mt.Format(), 0, mt.FormatLength());
					dvih->hdr.bmiHeader.biSize = sizeof(dvih->hdr.bmiHeader);
					dvih->hdr.bmiHeader.biWidth = (LONG)pTE->v.PixelWidth;
					dvih->hdr.bmiHeader.biHeight = (LONG)pTE->v.PixelHeight;
					dvih->hdr.bmiHeader.biPlanes = 1;
					dvih->hdr.bmiHeader.biBitCount = 24;
					dvih->hdr.bmiHeader.biSizeImage = DIBSIZE(dvih->hdr.bmiHeader);
					dvih->hdr.dwPictAspectRatioX = dvih->hdr.bmiHeader.biWidth;
					dvih->hdr.dwPictAspectRatioY = dvih->hdr.bmiHeader.biHeight;

					BYTE* pSequenceHeader = (BYTE*)dvih->dwSequenceHeader;
					memcpy(pSequenceHeader, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					dvih->cbSequenceHeader = (DWORD)pTE->CodecPrivate.size();

					if (!bHasVideo)
						mts.push_back(mt);
					bHasVideo = true;
				} else if (CodecID == "V_MPEG2") {
					BYTE* seqhdr = pTE->CodecPrivate.data();
					DWORD len = (DWORD)pTE->CodecPrivate.size();
					int w = (int)pTE->v.PixelWidth;
					int h = (int)pTE->v.PixelHeight;

					if (MakeMPEG2MediaType(mt, seqhdr, len, w, h)) {
						if (!bHasVideo)
							mts.push_back(mt);
						bHasVideo = true;

						std::vector<BYTE> buf;
						buf.resize(len);
						memcpy(buf.data(), seqhdr, len);
						CBaseSplitterFileEx::seqhdr h;
						if (m_pFile->CBaseSplitterFileEx::Read(h, buf)) {
							codecAvgTimePerFrame = h.hdr.ifps;
							bInterlaced = TRUE;
						}
					}
				} else if (CodecID == "V_DSHOW/MPEG1VIDEO" || CodecID == "V_MPEG1") {
					mt.majortype	= MEDIATYPE_Video;
					mt.subtype		= MEDIASUBTYPE_MPEG1Payload;
					mt.formattype	= FORMAT_MPEGVideo;

					MPEG1VIDEOINFO* pm1vi = (MPEG1VIDEOINFO*)mt.AllocFormatBuffer(sizeof(MPEG1VIDEOINFO) + pTE->CodecPrivate.size());
					memset(mt.Format(), 0, mt.FormatLength());
					memcpy(mt.Format() + sizeof(MPEG1VIDEOINFO), pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					pm1vi->hdr.bmiHeader.biSize			= sizeof(pm1vi->hdr.bmiHeader);
					pm1vi->hdr.bmiHeader.biWidth		= (LONG)pTE->v.PixelWidth;
					pm1vi->hdr.bmiHeader.biHeight		= (LONG)pTE->v.PixelHeight;
					pm1vi->hdr.bmiHeader.biBitCount		= 12;
					pm1vi->hdr.bmiHeader.biSizeImage	= DIBSIZE(pm1vi->hdr.bmiHeader);

					mt.SetSampleSize(pm1vi->hdr.bmiHeader.biWidth * pm1vi->hdr.bmiHeader.biHeight * 4);
					if (!bHasVideo)
						mts.push_back(mt);
					bHasVideo = true;
				} else if (CodecID == "V_MPEGH/ISO/HEVC") {
					BITMAPINFOHEADER pbmi;
					memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
					pbmi.biSize			= sizeof(pbmi);
					pbmi.biWidth		= (LONG)pTE->v.PixelWidth;
					pbmi.biHeight		= (LONG)pTE->v.PixelHeight;
					pbmi.biCompression	= FCC('HVC1');
					pbmi.biPlanes		= 1;
					pbmi.biBitCount		= 24;

					CSize aspect(pbmi.biWidth, pbmi.biHeight);
					ReduceDim(aspect);
					CreateMPEG2VISimple(&mt, &pbmi, 0, aspect, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					MPEG2VIDEOINFO* pm2vi	= (MPEG2VIDEOINFO*)mt.pbFormat;
					BYTE * extradata		= pTE->CodecPrivate.data();
					size_t size				= pTE->CodecPrivate.size();
					vc_params_t params;
					if (HEVCParser::ParseHEVCDecoderConfigurationRecord(extradata, size, params, false)) {
						pm2vi->dwProfile	= params.profile;
						pm2vi->dwLevel		= params.level;
						pm2vi->dwFlags		= params.nal_length_size;
					}

					if (!pm2vi->dwFlags
							&& (extradata[0] || extradata[1] || extradata[2] > 1 && size > 25)) {
						pm2vi->dwFlags = (extradata[21] & 3) + 1;
					}

					if (!bHasVideo) {
						mts.push_back(mt);
						if (pTE->CodecPrivate.empty()) {
							std::vector<BYTE> pData;
							if (ReadFirtsBlock(pData, pTE)) {
								CBaseSplitterFileEx::hevchdr h;
								CMediaType mt2;
								if (m_pFile->CBaseSplitterFileEx::Read(h, pData, &mt2)) {
									mts.insert(mts.cbegin(), mt2);
								}
							}
						}
					}
					bHasVideo = true;
				} else {
					DWORD fourcc = 0;
					WORD bitdepth = 0;
					if (CodecID == "V_MJPEG") {
						fourcc = FCC('MJPG');
						mt.bTemporalCompression = FALSE; // Motion JPEG has only I frames
					} else if (CodecID == "V_MPEG4/MS/V3") {
						fourcc = FCC('MP43');
					} else if (CodecID == "V_PRORES") {
						fourcc = FCC('icpf');
						mt.subtype = MEDIASUBTYPE_icpf;
					} else if (CodecID == "V_SNOW") {
						fourcc = FCC('SNOW');
					} else if (CodecID == "V_THEORA") {
						fourcc = FCC('THEO');
					} else if (CodecID == "V_VP8") {
						fourcc = FCC('VP80');
					} else if (CodecID == "V_VP9") {
						fourcc = FCC('VP90');
					} else if (CodecID == "V_AV1") {
						fourcc = FCC('AV01');
					} else if (CodecID == "V_QUICKTIME" && pTE->CodecPrivate.size() >= 8) {
						if (m_pFile->m_ebml.DocTypeReadVersion == 1) {
							fourcc = GETU32(pTE->CodecPrivate.data());
						} else {
							fourcc = GETU32(pTE->CodecPrivate.data() + 4);
						}
					} else if (CodecID.Left(9) == "V_REAL/RV" && CodecID.GetLength() >= 11) {
						fourcc = CodecID[7] + (CodecID[8] << 8) + (CodecID[9] << 16) + (CodecID[10] << 24);
					} else if (CodecID == "V_FFV1") {
						fourcc = FCC('FFV1');
					} else if (CodecID == "V_UNCOMPRESSED") {
						fourcc = FCC((DWORD)pTE->v.ColourSpace);
						switch (fourcc) {
						case FCC('Y8  '):
						case FCC('Y800'):
							bitdepth = 8;
							break;
						case FCC('YVU9'):
							bitdepth = 9;
							break;
						case FCC('I420'):
						case FCC('Y41B'):
						case FCC('NV12'):
						case FCC('YV12'):
							bitdepth = 12;
							break;
						case FCC('HDYC'):
						case FCC('UYVY'):
						case FCC('Y42B'):
						case FCC('YUY2'):
						case FCC('YV16'):
						case FCC('yuv2'):
							bitdepth = 16;
							break;
						case FCC('YV24'):
							bitdepth = 24;
							break;
						default:
							fourcc = 0; // unknown FourCC
							break;
							// TODO: bitdepth = 8 * framesize / (width * height)
						}

						mt.SetSampleSize(pTE->v.PixelWidth * (LONG)pTE->v.PixelHeight * bitdepth / 8); // fixed frame size
						mt.bTemporalCompression = FALSE;
					}

					if (fourcc) {
						mt.formattype = FORMAT_VideoInfo;
						mt.subtype = FOURCCMap(fourcc);
						VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.size());
						memset(mt.Format(), 0, mt.FormatLength());
						memcpy(mt.Format() + sizeof(VIDEOINFOHEADER), pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
						pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
						pvih->bmiHeader.biWidth = (LONG)pTE->v.PixelWidth;
						pvih->bmiHeader.biHeight = (LONG)pTE->v.PixelHeight;
						pvih->bmiHeader.biPlanes = 1; // must be set to 1.
						pvih->bmiHeader.biBitCount = bitdepth;
						pvih->bmiHeader.biCompression = fourcc;
						pvih->bmiHeader.biSizeImage = DIBSIZE(pvih->bmiHeader);

						if (!bHasVideo) {
							mts.push_back(mt);

							if (CodecID.Left(9) == "V_REAL/RV" && pTE->CodecPrivate.size() > 26) {
								const BYTE* extra = pTE->CodecPrivate.data() + 26;
								const size_t extralen = pTE->CodecPrivate.size() - 26;

								VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER) + extralen);
								memcpy(pvih + 1, extra, extralen);
								mts.insert(mts.cbegin(), mt);
							}

							if (mt.subtype == MEDIASUBTYPE_AV01) {
								std::vector<BYTE> pData;
								if (ReadFirtsBlock(pData, pTE)) {
									const auto size = pData.size();
									pvih = (VIDEOINFOHEADER*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER) + size);
									memcpy(pvih + 1, pData.data(), size);
									mts.insert(mts.cbegin(), mt);
								}
							} else if (mt.subtype == MEDIASUBTYPE_VP90) {
								std::vector<BYTE> pData;
								if (ReadFirtsBlock(pData, pTE)) {
									CGolombBuffer gb(pData.data(), pData.size());
									const BYTE marker = gb.BitRead(2);
									if (marker == 0x2) {
										BYTE profile = gb.BitRead(1);
										profile |= gb.BitRead(1) << 1;
										if (profile == 3) {
											profile += gb.BitRead(1);
										}

										#define VP9_SYNCCODE 0x498342

										AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
										int bits = 0;
										if (!gb.BitRead(1)) {
											const BYTE keyframe = !gb.BitRead(1);
											gb.BitRead(1);
											gb.BitRead(1);

											if (keyframe) {
												if (VP9_SYNCCODE == gb.BitRead(24)) {
													static const enum AVColorSpace colorspaces[8] = {
														AVCOL_SPC_UNSPECIFIED, AVCOL_SPC_BT470BG, AVCOL_SPC_BT709, AVCOL_SPC_SMPTE170M,
														AVCOL_SPC_SMPTE240M, AVCOL_SPC_BT2020_NCL, AVCOL_SPC_RESERVED, AVCOL_SPC_RGB,
													};

													bits = profile <= 1 ? 0 : 1 + gb.BitRead(1); // 0:8, 1:10, 2:12
													const AVColorSpace colorspace = colorspaces[gb.BitRead(3)];
													if (colorspace == AVCOL_SPC_RGB) {
														static const enum AVPixelFormat pix_fmt_rgb[3] = {
															AV_PIX_FMT_GBRP, AV_PIX_FMT_GBRP10, AV_PIX_FMT_GBRP12
														};
														pix_fmt = pix_fmt_rgb[bits];
													} else {
														static const enum AVPixelFormat pix_fmt_for_ss[3][2 /* v */][2 /* h */] = {
															{ { AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUV422P },
																{ AV_PIX_FMT_YUV440P, AV_PIX_FMT_YUV420P } },
															{ { AV_PIX_FMT_YUV444P10, AV_PIX_FMT_YUV422P10 },
																{ AV_PIX_FMT_YUV440P10, AV_PIX_FMT_YUV420P10 } },
															{ { AV_PIX_FMT_YUV444P12, AV_PIX_FMT_YUV422P12 },
																{ AV_PIX_FMT_YUV440P12, AV_PIX_FMT_YUV420P12 } }
														};

														gb.BitRead(1);
														if (profile & 1) {
															const BYTE h = gb.BitRead(1);
															const BYTE v = gb.BitRead(1);
															pix_fmt = pix_fmt_for_ss[bits][v][h];
														} else {
															pix_fmt = pix_fmt_for_ss[bits][1][1];
														}
													}
												}
											}
										}

										m_profile = profile;
										m_pix_fmt = pix_fmt;
										m_bits    = bits == 0 ? 8 : bits == 1 ? 10 : 12;
									}
								}
							}
						}

						bHasVideo = true;
					}
				}

				double framerate = 0.0;
				if (pTE->v.FrameRate > 0) {
					framerate = pTE->v.FrameRate;
					DLog(L"CMatroskaSplitterFilter::CreateOutputs(): FrameRate = %.6f (by pTE->v.FrameRate)", framerate);
				} else if (pTE->DefaultDuration > 0) {
					framerate = 100.0 * UNITS / pTE->DefaultDuration;
					DLog(L"CMatroskaSplitterFilter::CreateOutputs(): FrameRate = %.6f (by pTE->DefaultDuration = %I64d)", framerate, (INT64)pTE->DefaultDuration);
				}

				if (CodecID == "V_DSHOW/MPEG1VIDEO" || CodecID == "V_MPEG1") {
					DLog(L"CMatroskaSplitterFilter::CreateOutputs(): HACK: MPEG1 fps = %.6f overwritten to %.6f", framerate, framerate / 2);
					framerate /= 2;
				}

				if (framerate > 200.0 && CodecID != "V_MPEG2") { // incorrect fps - calculate avarage value
					CMatroskaNode Root(m_pFile);
					m_pSegment = Root.Child(MATROSKA_ID_SEGMENT);
					m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);

					std::vector<REFERENCE_TIME> timecodes;
					timecodes.reserve(TimecodeAnalyzer::DefaultFrameNum);

					bool readmore = true;

					do {
						Cluster c;
						c.ParseTimeCode(m_pCluster);

						if (CAutoPtr<CMatroskaNode> pBlock = m_pCluster->GetFirstBlock()) {
							do {
								CBlockGroupNode bgn;

								if (pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
									bgn.Parse(pBlock, true);
								}
								else if (pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
									CAutoPtr<BlockGroup> bg(DNew BlockGroup());
									bg->Block.Parse(pBlock, true);
									bgn.emplace_back(bg);
								}

								for (const auto& bg : bgn) {
									if (bg->Block.TrackNumber != pTE->TrackNumber) {
										continue;
									}
									timecodes.push_back(c.TimeCode + bg->Block.TimeCode);

									if (timecodes.size() >= TimecodeAnalyzer::DefaultFrameNum) {
										readmore = false;
										break;
									}
								}
							} while (readmore && pBlock->NextBlock());
						}
					} while (readmore && m_pCluster->Next(true));

					m_pCluster.Free();

					framerate = TimecodeAnalyzer::CalculateFPS(timecodes, 1000000000ui64 / m_pFile->m_segment.SegmentInfo.TimeCodeScale);
				}

				if (bInterlaced && codecAvgTimePerFrame) {
					// hack for interlaced video
					double factor = 10000000.0 / (framerate * codecAvgTimePerFrame);
					if (factor > 0.4 && factor < 0.6) {
						framerate = 10000000.0 / codecAvgTimePerFrame;
					}
				}

				if (std::isnan(framerate) || framerate <= 0) {
					DLog(L"CMatroskaSplitterFilter::CreateOutputs() : Very damaged file? Set fps as 23.976.");
					framerate = 24/1.001; // set 23.976 as default
				}

				if (!pTE->DefaultDuration) {
					// manual fill DefaultDuration only if it is empty
					DLog(L"CMatroskaSplitterFilter::CreateOutputs() : Empty DefaultDuration! Set calculated value.");
					pTE->DefaultDuration.Set((UINT64)((100.0 * UNITS) / framerate + 0.001));
				}

				for (auto& item : mts) {
					if (item.formattype == FORMAT_VideoInfo
							|| item.formattype == FORMAT_VideoInfo2
							|| item.formattype == FORMAT_MPEG2Video
							|| item.formattype == FORMAT_MPEGVideo) {
						if (pTE->v.PixelWidth && pTE->v.PixelHeight) {
							RECT rect = {(LONG)pTE->v.VideoPixelCropLeft,
										 (LONG)pTE->v.VideoPixelCropTop,
										 (LONG)(pTE->v.PixelWidth - pTE->v.VideoPixelCropRight),
										 (LONG)(pTE->v.PixelHeight - pTE->v.VideoPixelCropBottom)
										};
							VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)item.Format();
							vih->rcSource = vih->rcTarget = rect;
						}

						((VIDEOINFOHEADER*)item.Format())->AvgTimePerFrame = (UINT64)(UNITS / framerate + 0.001);
					}
				}

				if (pTE->v.DisplayWidth && pTE->v.DisplayHeight) {
					for (auto& item : mts) {
						if (item.formattype == FORMAT_VideoInfo) {
							DWORD vih1 = FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
							DWORD vih2 = FIELD_OFFSET(VIDEOINFOHEADER2, bmiHeader);
							DWORD bmi = item.FormatLength() - FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);

							item.formattype = FORMAT_VideoInfo2;
							item.ReallocFormatBuffer(vih2 + bmi);
							memmove(item.Format() + vih2, item.Format() + vih1, bmi);
							memset(item.Format() + vih1, 0, vih2 - vih1);

							CSize aspect((int)pTE->v.DisplayWidth, (int)pTE->v.DisplayHeight);
							ReduceDim(aspect);
							((VIDEOINFOHEADER2*)item.Format())->dwPictAspectRatioX = aspect.cx;
							((VIDEOINFOHEADER2*)item.Format())->dwPictAspectRatioY = aspect.cy;
						} else if (item.formattype == FORMAT_MPEG2Video) {
							CSize aspect((int)pTE->v.DisplayWidth, (int)pTE->v.DisplayHeight);
							ReduceDim(aspect);
							((MPEG2VIDEOINFO*)item.Format())->hdr.dwPictAspectRatioX = aspect.cx;
							((MPEG2VIDEOINFO*)item.Format())->hdr.dwPictAspectRatioY = aspect.cy;
						}
					}
				}

				if (pTE->v.FlagInterlaced == 1) {
					switch (pTE->v.FieldOrder) {
					case 0: // progressive
						m_interlaced = 0;
						break;
					case 1:
					case 9: // top field stored first
						m_interlaced = 1;
						break;
					case 6:
					case 14: // bottom field stored first
						m_interlaced = 2;
						break;
					default:
						m_interlaced = -1; // not supported
					}
				}

				if (pTE->v.StereoMode && pTE->v.StereoMode < MATROSKA_VIDEO_STEREOMODE) {
					SetProperty(L"STEREOSCOPIC3DMODE", matroska_stereo_mode[pTE->v.StereoMode]);
				}

				if (pTE->v.VideoColorInformation.bValid) {
					m_ColorSpace = DNew(ColorSpace);
					ZeroMemory(m_ColorSpace, sizeof(ColorSpace));

					if (pTE->v.VideoColorInformation.MatrixCoefficients != AVCOL_SPC_RESERVED) {
						m_ColorSpace->MatrixCoefficients = pTE->v.VideoColorInformation.MatrixCoefficients;
					}
					if (pTE->v.VideoColorInformation.Primaries != AVCOL_PRI_RESERVED
							&& pTE->v.VideoColorInformation.Primaries != AVCOL_PRI_RESERVED0) {
						m_ColorSpace->Primaries = pTE->v.VideoColorInformation.Primaries;
					}
					if (pTE->v.VideoColorInformation.Range != AVCOL_RANGE_UNSPECIFIED
							&& pTE->v.VideoColorInformation.Range <= AVCOL_RANGE_JPEG) {
						m_ColorSpace->Range = pTE->v.VideoColorInformation.Range;
					}
					if (pTE->v.VideoColorInformation.TransferCharacteristics != AVCOL_TRC_RESERVED
							&& pTE->v.VideoColorInformation.TransferCharacteristics != AVCOL_TRC_RESERVED0) {
						m_ColorSpace->TransferCharacteristics = pTE->v.VideoColorInformation.TransferCharacteristics;
					}

					enum MatroskaColourChromaSitingHorz {
						MATROSKA_COLOUR_CHROMASITINGHORZ_UNDETERMINED = 0,
						MATROSKA_COLOUR_CHROMASITINGHORZ_LEFT         = 1,
						MATROSKA_COLOUR_CHROMASITINGHORZ_HALF         = 2,
						MATROSKA_COLOUR_CHROMASITINGHORZ_NB
					};

					enum MatroskaColourChromaSitingVert {
						MATROSKA_COLOUR_CHROMASITINGVERT_UNDETERMINED = 0,
						MATROSKA_COLOUR_CHROMASITINGVERT_TOP          = 1,
						MATROSKA_COLOUR_CHROMASITINGVERT_HALF         = 2,
						MATROSKA_COLOUR_CHROMASITINGVERT_NB
					};

					if (pTE->v.VideoColorInformation.ChromaSitingHorz != MATROSKA_COLOUR_CHROMASITINGHORZ_UNDETERMINED
							&& pTE->v.VideoColorInformation.ChromaSitingVert != MATROSKA_COLOUR_CHROMASITINGVERT_UNDETERMINED
							&& pTE->v.VideoColorInformation.ChromaSitingHorz < MATROSKA_COLOUR_CHROMASITINGHORZ_NB
							&& pTE->v.VideoColorInformation.ChromaSitingVert < MATROSKA_COLOUR_CHROMASITINGVERT_NB) {
						m_ColorSpace->ChromaLocation =
							chroma_pos_to_enum((pTE->v.VideoColorInformation.ChromaSitingHorz - 1) << 7,
											   (pTE->v.VideoColorInformation.ChromaSitingVert - 1) << 7);
					}

				}

				if (pTE->v.VideoColorInformation.MaxCLL.IsValid() && pTE->v.VideoColorInformation.MaxFALL.IsValid()) {
					m_HDRContentLightLevel = DNew(MediaSideDataHDRContentLightLevel);
					ZeroMemory(m_HDRContentLightLevel, sizeof(MediaSideDataHDRContentLightLevel));

					m_HDRContentLightLevel->MaxCLL  = pTE->v.VideoColorInformation.MaxCLL;
					m_HDRContentLightLevel->MaxFALL = pTE->v.VideoColorInformation.MaxFALL;
				}

				if (pTE->v.VideoColorInformation.SMPTE2086MasteringMetadata.bValid) {
					MasteringMetadata& metadata = pTE->v.VideoColorInformation.SMPTE2086MasteringMetadata;

					m_MasterDataHDR = DNew(MediaSideDataHDR);
					ZeroMemory(m_MasterDataHDR, sizeof(MediaSideDataHDR));

					m_MasterDataHDR->display_primaries_x[0] = metadata.PrimaryGChromaticityX;
					m_MasterDataHDR->display_primaries_y[0] = metadata.PrimaryGChromaticityY;
					m_MasterDataHDR->display_primaries_x[1] = metadata.PrimaryBChromaticityX;
					m_MasterDataHDR->display_primaries_y[1] = metadata.PrimaryBChromaticityY;
					m_MasterDataHDR->display_primaries_x[2] = metadata.PrimaryRChromaticityX;
					m_MasterDataHDR->display_primaries_y[2] = metadata.PrimaryRChromaticityY;

					m_MasterDataHDR->white_point_x = metadata.WhitePointChromaticityX;
					m_MasterDataHDR->white_point_y = metadata.WhitePointChromaticityY;
					m_MasterDataHDR->max_display_mastering_luminance = metadata.LuminanceMax;
					m_MasterDataHDR->min_display_mastering_luminance = metadata.LuminanceMin;
				}

				if (mt.subtype == MEDIASUBTYPE_VP90 && m_profile != -1 && m_pix_fmt != -1) {
					for (const auto& item : mts) {
						if (item.formattype == FORMAT_VideoInfo2) {
							mt = item;
							VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2) + 16);
							BYTE *extra = (BYTE*)(vih2 + 1);
							memcpy(extra, "vpcC", 4);
							// use code from LAV
							AV_WB8 (extra +  4, 1); // version
							AV_WB24(extra +  5, 0); // flags
							AV_WB8 (extra +  8, m_profile);
							AV_WB8 (extra +  9, 0);
							AV_WB8 (extra + 10, m_bits << 4 | (m_ColorSpace ? m_ColorSpace->ChromaLocation : 0 ) << 1 | (m_ColorSpace ? m_ColorSpace->Range == AVCOL_RANGE_JPEG : 0));
							AV_WB8 (extra + 11, m_ColorSpace ? m_ColorSpace->Primaries : AVCOL_PRI_UNSPECIFIED);
							AV_WB8 (extra + 12, m_ColorSpace ? m_ColorSpace->TransferCharacteristics : AVCOL_TRC_UNSPECIFIED);
							AV_WB8 (extra + 13, m_ColorSpace ? m_ColorSpace->MatrixCoefficients : AVCOL_SPC_UNSPECIFIED);
							AV_WB16(extra + 14, 0); // no codec init data

							mts.insert(mts.cbegin(), mt);
							break;
						}
					}
				}
			} else if (pTE->TrackType == TrackEntry::TypeAudio) {
				Name.Format(L"Audio %d", iAudio++);

				mt.majortype = MEDIATYPE_Audio;
				mt.formattype = FORMAT_WaveFormatEx;
				WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
				memset(wfe, 0, mt.FormatLength());
				wfe->nChannels = (WORD)pTE->a.Channels;
				wfe->nSamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
				wfe->wBitsPerSample = (WORD)pTE->a.BitDepth;
				wfe->nBlockAlign = (WORD)((wfe->nChannels * wfe->wBitsPerSample) / 8);
				wfe->nAvgBytesPerSec = wfe->nSamplesPerSec * wfe->nBlockAlign;
				mt.SetSampleSize(256000);

				if (CodecID == "A_MPEG/L3") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MPEGLAYER3);
					mts.push_back(mt);
				} else if (CodecID == "A_MPEG/L2" ||
						   CodecID == "A_MPEG/L1") {
					WORD layer = CodecID == "A_MPEG/L1" ? 1 : 2;

					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MPEG);
					if (layer == 2) {
						mt.subtype = MEDIASUBTYPE_MPEG2_AUDIO;
					}
					MPEG1WAVEFORMAT* f = (MPEG1WAVEFORMAT*)mt.ReallocFormatBuffer(sizeof(MPEG1WAVEFORMAT));
					f->fwHeadMode		= 1 << wfe->nChannels;
					f->fwHeadLayer		= 1 << (layer - 1);
					f->dwHeadBitrate	= 0;
					f->dwPTSHigh		= 0;
					f->dwPTSLow			= 0;
					f->fwHeadFlags		= 0;
					f->fwHeadModeExt	= 0;
					f->wHeadEmphasis	= 0;

					mts.push_back(mt);
				} else if (CodecID.Left(5) == "A_AC3") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_DOLBY_AC3);

					std::vector<BYTE> pData;
					if (ReadFirtsBlock(pData, pTE)) {
						audioframe_t aframe;
						const int size = ParseAC3Header(pData.data(), &aframe);
						if (size && aframe.param1) {
							wfe->nAvgBytesPerSec = aframe.param1 / 8;
						}
					}

					mts.push_back(mt);
				} else if (CodecID == "A_EAC3") {
					mt.subtype = MEDIASUBTYPE_DOLBY_DDPLUS;

					std::vector<BYTE> pData;
					if (ReadFirtsBlock(pData, pTE)) {
						audioframe_t aframe;
						int size = ParseEAC3Header(pData.data(), &aframe);
						if (!size || aframe.param1 == EAC3_FRAME_TYPE_DEPENDENT) {
							size = ParseAC3Header(pData.data(), &aframe);
						}
						if (size) {
							wfe->nChannels = aframe.channels;
							wfe->nAvgBytesPerSec = size * aframe.samplerate / aframe.samples;
							if (size + 8 <= (int)pData.size()) {
								int size2 = ParseEAC3Header(pData.data() + size, &aframe);
								if (size2 && aframe.param1 == EAC3_FRAME_TYPE_DEPENDENT) {
									wfe->nChannels += aframe.channels - 2;
									wfe->nAvgBytesPerSec = (size + size2) * aframe.samplerate / aframe.samples;
								}
							}
						}
					}

					mts.push_back(mt);
				} else if (CodecID == "A_TRUEHD") {
					mt.subtype = MEDIASUBTYPE_DOLBY_TRUEHD;
					mts.push_back(mt);
				} else if (CodecID == "A_MLP") {
					mt.subtype = MEDIASUBTYPE_MLP;
					mts.push_back(mt);
				} else if (CodecID == "A_DTS") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_DTS2);

					BYTE profile = 0;

					__int64 pos = m_pFile->GetPos();

					CMatroskaNode Root(m_pFile);
					m_pSegment = Root.Child(MATROSKA_ID_SEGMENT);
					m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);

					Cluster c;
					c.ParseTimeCode(m_pCluster);

					if (!m_pBlock) {
						m_pBlock = m_pCluster->GetFirstBlock();
					}

					BOOL bIsParse = FALSE;
					do {
						CBlockGroupNode bgn;

						if (m_pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
							bgn.Parse(m_pBlock, true);
						} else if (m_pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
							CAutoPtr<BlockGroup> bg(DNew BlockGroup());
							bg->Block.Parse(m_pBlock, true);
							bgn.emplace_back(bg);
						}

						for (const auto& bg : bgn) {
							if (bg->Block.TrackNumber != pTE->TrackNumber) {
								continue;
							}

							if (!bg->Block.BlockData.empty()) {
								const auto& pb = bg->Block.BlockData.cbegin()->m_p;
								pTE->Expand(*pb, ContentEncoding::AllFrameContents);

								BYTE* start	= pb->data();
								BYTE* end	= start + pb->size();
								audioframe_t aframe;
								int size = ParseDTSHeader(start, &aframe);
								if (size) {
									if (aframe.samples) {
										wfe->nAvgBytesPerSec = size * aframe.samplerate / aframe.samples;
									}
									if (start + size + 40 <= end) {
										if (ParseDTSHDHeader(start + size, end - start - size, &aframe)) {
											profile = aframe.param2;

											wfe->nSamplesPerSec = aframe.samplerate;
											wfe->nChannels = aframe.channels;
											if (aframe.param1) {
												wfe->wBitsPerSample = aframe.param1;
											}
											wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample / 8;
											if (aframe.param2 == DCA_PROFILE_HD_HRA) {
												wfe->nAvgBytesPerSec += CalcBitrate(aframe) / 8;
											} else {
												wfe->nAvgBytesPerSec = 0;
											}
										}
									}
								} else if (ParseDTSHDHeader(start, end - start, &aframe) && aframe.param2 == DCA_PROFILE_EXPRESS) {
									profile = aframe.param2;

									wfe->nSamplesPerSec = aframe.samplerate;
									wfe->nChannels = aframe.channels;
									wfe->wBitsPerSample = aframe.param1;
									wfe->nBlockAlign = wfe->nChannels * wfe->wBitsPerSample / 8;
									wfe->nAvgBytesPerSec = CalcBitrate(aframe) / 8;
								}
							}

							bIsParse = TRUE;
							break;
						}
					} while (m_pBlock->NextBlock() && SUCCEEDED(hr) && !CheckRequest(nullptr) && !bIsParse);

					m_pBlock.Free();
					m_pCluster.Free();

					m_pFile->Seek(pos);

					if (profile) {
						wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 1);
						wfe->cbSize = 1;
						((BYTE *)(wfe + 1))[0] = profile;
					}

					mts.push_back(mt);
				} else if (CodecID == "A_TTA1") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_TTA1);
					wfe->cbSize = 30;
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 30);
					BYTE *p = (BYTE *)(wfe + 1);
					memcpy(p, (const unsigned char *)"TTA1\x01\x00", 6);
					memcpy(p + 6,  &wfe->nChannels, 2);
					memcpy(p + 8,  &wfe->wBitsPerSample, 2);
					memcpy(p + 10, &wfe->nSamplesPerSec, 4);
					memset(p + 14, 0, 30 - 14);
					mts.push_back(mt);
				} else if (CodecID == "A_AAC") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_RAW_AAC1);
					wfe->cbSize = (WORD)pTE->CodecPrivate.size();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.size());
					memcpy(wfe + 1, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					if (!pTE->CodecPrivate.empty()) {
						CMP4AudioDecoderConfig MP4AudioDecoderConfig;
						if (MP4AudioDecoderConfig.Parse(pTE->CodecPrivate.data(), pTE->CodecPrivate.size())) {
							if (MP4AudioDecoderConfig.m_ChannelCount > wfe->nChannels) {
								wfe->nChannels = MP4AudioDecoderConfig.m_ChannelCount;
								wfe->nBlockAlign = (WORD)((wfe->nChannels * wfe->wBitsPerSample) / 8);
							}
							if (MP4AudioDecoderConfig.m_SamplingFrequency > wfe->nSamplesPerSec) {
								wfe->nSamplesPerSec = MP4AudioDecoderConfig.m_SamplingFrequency;
							}
						}
					}

					wfe->nAvgBytesPerSec = 0;
					mts.push_back(mt);
				} else if (CodecID.Left(6) == "A_AAC/") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_RAW_AAC1);
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 5);
					wfe->cbSize = 2;

					int profile;
					if (CodecID.Find("/MAIN") > 0) {
						profile = 0;
					} else if (CodecID.Find("/SBR") > 0) {
						profile = -1;
					} else if (CodecID.Find("/LC") > 0) {
						profile = 1;
					} else if (CodecID.Find("/SSR") > 0) {
						profile = 2;
					} else if (CodecID.Find("/LTP") > 0) {
						profile = 3;
					} else {
						continue;
					}

					WORD cbSize = MakeAACInitData((BYTE*)(wfe + 1), profile, wfe->nSamplesPerSec, (int)pTE->a.Channels);
					mts.push_back(mt);

					if (profile < 0) {
						wfe->cbSize = cbSize;
						wfe->nSamplesPerSec *= 2;
						wfe->nAvgBytesPerSec *= 2;

						mts.insert(mts.cbegin(), mt);
					}
				} else if (CodecID == "A_WAVPACK4") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_WAVPACK4);
					wfe->cbSize = (WORD)pTE->CodecPrivate.size();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.size());
					memcpy(wfe + 1, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					mts.push_back(mt);
				} else if (CodecID == "A_FLAC") {
					wfe->wFormatTag = WAVE_FORMAT_FLAC;
					wfe->cbSize = (WORD)pTE->CodecPrivate.size();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.size());
					memcpy(wfe + 1, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					mt.subtype = MEDIASUBTYPE_FLAC_FRAMED;
					mts.push_back(mt);
					mt.subtype = MEDIASUBTYPE_FLAC;
					mts.push_back(mt);
				} else if (CodecID == "A_PCM/INT/LIT") {
					mt.subtype = MEDIASUBTYPE_PCM;
					mt.SetSampleSize(wfe->nBlockAlign);
					if (pTE->a.Channels <= 2 && pTE->a.BitDepth <= 16) {
						wfe->wFormatTag = WAVE_FORMAT_PCM;
					} else {
						WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
						if (pTE->a.BitDepth&7) {
							wfex->Format.wBitsPerSample = (WORD)(pTE->a.BitDepth + 7) & 0xFFF8;
							wfex->Format.nBlockAlign = wfex->Format.nChannels * wfex->Format.wBitsPerSample / 8;
							wfex->Format.nAvgBytesPerSec = wfex->Format.nSamplesPerSec * wfex->Format.nBlockAlign;
							mt.SetSampleSize(wfex->Format.nBlockAlign);
						}
						wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
						wfex->Format.cbSize = 22;
						wfex->Samples.wValidBitsPerSample = (WORD)pTE->a.BitDepth;
						wfex->dwChannelMask = GetDefChannelMask(wfex->Format.nChannels);
						wfex->SubFormat = MEDIASUBTYPE_PCM;
					}
					mts.push_back(mt);
				} else if (CodecID == "A_PCM/INT/BIG") {
					switch (pTE->a.BitDepth) {
					case 16: mt.subtype = MEDIASUBTYPE_PCM_TWOS; break;
					case 24: mt.subtype = MEDIASUBTYPE_PCM_IN24; break;
					case 32: mt.subtype = MEDIASUBTYPE_PCM_IN32; break;
					default:
						ASSERT(0);
						break;
					}
					mt.SetSampleSize(wfe->nBlockAlign);
					mts.push_back(mt);
				} else if (CodecID == "A_PCM/FLOAT/IEEE") {
					mt.subtype = MEDIASUBTYPE_IEEE_FLOAT;
					mt.SetSampleSize(wfe->nBlockAlign);
					if (pTE->a.Channels <= 2) {
						wfe->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
					} else {
						WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
						wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
						wfex->Format.cbSize = 22;
						wfex->Samples.wValidBitsPerSample = (WORD)pTE->a.BitDepth;
						wfex->dwChannelMask = GetDefChannelMask(wfex->Format.nChannels);
						wfex->SubFormat = MEDIASUBTYPE_IEEE_FLOAT;
					}
					mts.push_back(mt);
				} else if (CodecID == "A_MS/ACM") {
					wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(pTE->CodecPrivate.size());
					memcpy(wfe, (WAVEFORMATEX*)pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe->cbSize == 22) {
						mt.subtype = ((WAVEFORMATEXTENSIBLE*)wfe)->SubFormat;
					}
					else mt.subtype = FOURCCMap(wfe->wFormatTag);
					mts.push_back(mt);
				} else if (CodecID == "A_VORBIS") {
					CreateVorbisMediaType(mt, mts,
										  (DWORD)pTE->a.Channels,
										  (DWORD)pTE->a.SamplingFrequency,
										  DWORD(pTE->a.BitDepth),
										  pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
				} else if (CodecID.Left(7) == "A_REAL/" && CodecID.GetLength() >= 11) {
					mt.subtype = FOURCCMap((DWORD)CodecID[7]|((DWORD)CodecID[8]<<8)|((DWORD)CodecID[9]<<16)|((DWORD)CodecID[10]<<24));
					mt.bTemporalCompression = TRUE;
					wfe->cbSize = (WORD)pTE->CodecPrivate.size();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.size());
					memcpy(wfe + 1, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					wfe->cbSize = 0; // IMPORTANT: this is screwed, but cbSize has to be 0 and the extra data from codec priv must be after WAVEFORMATEX
					mts.push_back(mt);
				} else if (CodecID == "A_QUICKTIME" && pTE->CodecPrivate.size() >= 8) {
					DWORD type = GETU32(pTE->CodecPrivate.data() + 4);
					// 'QDM2', 'QDMC', 'ima4'
					mt.subtype = FOURCCMap(type);
					wfe->cbSize = (WORD)pTE->CodecPrivate.size();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.size());
					memcpy(wfe + 1, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					mts.push_back(mt);
				} else if (CodecID == "A_QUICKTIME/QDM2") {
					mt.subtype = MEDIASUBTYPE_QDM2;
					wfe->cbSize = (WORD)pTE->CodecPrivate.size();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.size());
					memcpy(wfe + 1, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					mts.push_back(mt);
				} else if (CodecID.Left(6) == "A_OPUS") {
					wfe->wFormatTag = WAVE_FORMAT_OPUS;
					mt.subtype = MEDIASUBTYPE_OPUS;
					wfe->cbSize = (WORD)pTE->CodecPrivate.size();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.size());
					memcpy(wfe + 1, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					mts.push_back(mt);
				} else if (CodecID == "A_ALAC") {
					wfe->wFormatTag = WAVE_FORMAT_ALAC;
					mt.subtype = MEDIASUBTYPE_ALAC;
					WORD cbSize = (WORD)pTE->CodecPrivate.size() + 12;
					wfe->cbSize = cbSize;
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + cbSize);
					BYTE *p = (BYTE *)(wfe + 1);

					memset(p, 0, cbSize);
					memcpy(p + 3, &cbSize, 1);
					memcpy(p + 4, (const unsigned char *)"alac", 4);
					memcpy(p + 12, pTE->CodecPrivate.data(), pTE->CodecPrivate.size());
					mts.push_back(mt);
				}
			} else if (pTE->TrackType == TrackEntry::TypeSubtitle) {
				Name.Format(L"Subtitle %d", iSubtitle++);

				mt.SetSampleSize(1);

				if (CodecID == "S_TEXT/ASCII") {
					mt.majortype = MEDIATYPE_Text;
					mt.subtype = MEDIASUBTYPE_NULL;
					mt.formattype = FORMAT_None;
					mts.push_back(mt);
					isSub = true;
				} else {
					if (CodecID == "S_TEXT/WEBVTT") {
						pTE->CodecPrivate.clear();
					}

					mt.majortype = MEDIATYPE_Subtitle;
					mt.formattype = FORMAT_SubtitleInfo;
					SUBTITLEINFO* psi = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + pTE->CodecPrivate.size());
					memset(psi, 0, mt.FormatLength());
					strncpy_s(psi->IsoLang, pTE->Language, _countof(psi->IsoLang) - 1);
					CString subtitle_Name = pTE->Name;
					if (pTE->FlagForced && pTE->FlagDefault) {
						subtitle_Name += L" [Default, Forced]";
					} else if (pTE->FlagForced) {
						subtitle_Name += L" [Forced]";
					} else if (pTE->FlagDefault) {
						subtitle_Name += L" [Default]";
					}
					subtitle_Name = subtitle_Name.Trim();

					wcsncpy_s(psi->TrackName, subtitle_Name, _countof(psi->TrackName) - 1);
					memcpy(mt.pbFormat + (psi->dwOffset = sizeof(SUBTITLEINFO)), pTE->CodecPrivate.data(), pTE->CodecPrivate.size());

					mt.subtype =
						CodecID == "S_TEXT/UTF8" || CodecID == "D_WEBVTT/SUBTITLES" || CodecID == "S_TEXT/WEBVTT" ? MEDIASUBTYPE_UTF8 :
						CodecID == "S_TEXT/SSA" || CodecID == "S_SSA" ? MEDIASUBTYPE_SSA :
						CodecID == "S_TEXT/ASS" || CodecID == "S_ASS" ? MEDIASUBTYPE_ASS :
						CodecID == "S_TEXT/USF" || CodecID == "S_USF" ? MEDIASUBTYPE_USF :
						CodecID == "S_HDMV/PGS" ? MEDIASUBTYPE_HDMVSUB :
						CodecID == "S_DVBSUB" ? MEDIASUBTYPE_DVB_SUBTITLES :
						CodecID == "S_VOBSUB" ? MEDIASUBTYPE_VOBSUB :
						CodecID == "S_KATE" ? FOURCCMap(FCC('KATE')) : // TODO: use the correct subtype for KATE subtitles
						MEDIASUBTYPE_NULL;

					if (mt.subtype != MEDIASUBTYPE_NULL) {
						mts.push_back(mt);
						isSub = true;

						if (mt.subtype == MEDIASUBTYPE_HDMVSUB || mt.subtype == MEDIASUBTYPE_DVB_SUBTITLES) {
							m_hasHdmvDvbSubPin = true;
						}
					}
				}
			}

			if (mts.empty()) {
				DLog(L"CMatroskaSplitterFilter::CreateOutputs() : Unsupported or multiple TrackType '%S' (%I64u)", CodecID, (UINT64)pTE->TrackType);
				continue;
			}

			CString Language = CString(pTE->Language.IsEmpty() ? L"English" : CString(ISO6392ToLanguage(pTE->Language))).Trim();

			Name = Language
				   + (pTE->Name.IsEmpty() ? L"" : (Language.IsEmpty() ? L"" : L", ") + pTE->Name)
				   + (L" (" + Name + L")");

			if (pTE->FlagForced && pTE->FlagDefault) {
				Name = Name + L" [Default, Forced]";
			} else if (pTE->FlagForced) {
				Name = Name + L" [Forced]";
			} else if (pTE->FlagDefault) {
				Name = Name + L" [Default]";
			}

			HRESULT hr;

			CMatroskaSplitterOutputPin* pPinOut = DNew CMatroskaSplitterOutputPin(mts, Name, this, this, &hr);
			if (!pTE->Name.IsEmpty()) {
				pPinOut->SetProperty(L"NAME", pTE->Name);
			}
			if (pTE->Language.GetLength() == 3) {
				pPinOut->SetProperty(L"LANG", CString(pTE->Language));
			}

			if (!isSub) {
				pinOut.insert(pinOut.begin() + (iVideo + iAudio - 3), pPinOut);
				pinOutTE.insert(pinOutTE.begin() + (iVideo + iAudio - 3), pTE);
			} else {
				pinOut.push_back(pPinOut);
				pinOutTE.push_back(pTE);
			}
		}
	}

	for (size_t i = 0; i < pinOut.size(); i++) {
		CAutoPtr<CBaseSplitterOutputPin> pPinOut;
		pPinOut.Attach(pinOut[i]);
		TrackEntry* pTE = pinOutTE[i];

		if (pTE != nullptr) {
			AddOutputPin((DWORD)pTE->TrackNumber, pPinOut);
			m_pTrackEntryMap[(DWORD)pTE->TrackNumber] = pTE;
			m_pOrderedTrackArray.push_back(pTE);

			if (pTE->TrackType == TrackType::TypeSubtitle) {
				m_subtitlesTrackNumbers.push_back(pTE->TrackNumber);
			}
		}
	}

	Info& info = m_pFile->m_segment.SegmentInfo;
	m_rtDuration = (REFERENCE_TIME)(info.Duration * info.TimeCodeScale / 100);

	if (m_bCalcDuration && bHasVideo && !m_pFile->m_segment.Cues.empty()) {
		// calculate duration from video track;
		m_pSegment = Root.Child(MATROSKA_ID_SEGMENT);
		m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);
		m_pBlock.Free();

		Segment& s = m_pFile->m_segment;
		UINT64 TrackNumber = s.GetMasterTrack();

		REFERENCE_TIME rtDur = INVALID_TIME;

		for (auto& it = s.Cues.crbegin(); it != s.Cues.crend() && rtDur == INVALID_TIME; it++) {
			const auto& pCue = *it;

			for (auto& it2 = pCue->CuePoints.crbegin(); it2 != pCue->CuePoints.crend() && rtDur == INVALID_TIME; it2++) {
				const auto& pCuePoint = *it2;

				for (auto& it3 = pCuePoint->CueTrackPositions.crbegin(); it3 != pCuePoint->CueTrackPositions.crend() && rtDur == INVALID_TIME; it3++) {
					const auto& pCueTrackPositions = *it3;

					if (TrackNumber != pCueTrackPositions->CueTrack) {
						continue;
					}

					m_pCluster->SeekTo(m_pSegment->m_start + pCueTrackPositions->CueClusterPosition);
					if (FAILED(m_pCluster->Parse())) {
						continue;
					}

					do {
						Cluster c;
						c.ParseTimeCode(m_pCluster);

						if (CAutoPtr<CMatroskaNode> pBlock = m_pCluster->GetFirstBlock()) {

							do {
								CBlockGroupNode bgn;

								if (pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
									bgn.Parse(pBlock, true);
								} else if (pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
									CAutoPtr<BlockGroup> bg(DNew BlockGroup());
									bg->Block.Parse(pBlock, true);
									bgn.emplace_back(bg);
								}

								for (const auto& bg : bgn) {
									if (bg->Block.TrackNumber == pCueTrackPositions->CueTrack) {
										auto it = m_pTrackEntryMap.find(TrackNumber);
										if (it == m_pTrackEntryMap.end() || !(*it).second) {
											continue;
										}
										TrackEntry* pTE = (*it).second;

										REFERENCE_TIME duration = 1;
										if (bg->BlockDuration.IsValid()) {
											duration = s.GetRefTime(bg->BlockDuration);
										} else if (pTE->DefaultDuration) {
											duration = (pTE->DefaultDuration / 100) * bg->Block.BlockData.size();
										}

										REFERENCE_TIME rt = s.GetRefTime((INT64)c.TimeCode + bg->Block.TimeCode) + duration;
										rtDur = std::max(rtDur, rt);
									}
								}
							} while (pBlock->NextBlock());
						}
					} while (m_pCluster->Next(true));
				}
			}
		}
		m_pCluster.Free();
		m_pBlock.Free();

		if (rtDur != INVALID_TIME) {
			m_rtDuration = std::min(m_rtDuration, rtDur);
		}
	}

	m_rtNewStop = m_rtStop = m_rtDuration;

	// fonts
	if (m_bLoadEmbeddedFonts) {
		InstallFonts();
	}

	// resources
	for (const auto& pA : m_pFile->m_segment.Attachments) {
		for (const auto& pF : pA->AttachedFiles) {
			std::vector<BYTE> pData;
			pData.resize((size_t)pF->FileDataLen);
			m_pFile->Seek(pF->FileDataPos);
			if (SUCCEEDED(m_pFile->ByteRead(pData.data(), pData.size()))) {
				ResAppend(pF->FileName, pF->FileDescription, CString(pF->FileMimeType), pData.data(), (DWORD)pData.size());
			}
		}
	}

	// chapters
	if (ChapterAtom* caroot = m_pFile->m_segment.FindChapterAtom(0)) {
		CStringA str;
		str.ReleaseBufferSetLength(GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, str.GetBuffer(3), 3));
		CStringA ChapLanguage = CStringA(ISO6391To6392(str));
		if (ChapLanguage.GetLength() < 3) {
			ChapLanguage = "eng";
		}

		SetupChapters(ChapLanguage, caroot);
	}

	// Tags
	if (!info.Title.IsEmpty()) {
		SetProperty(L"TITL", info.Title);
	}

	std::list<int> pg_offsets;

	for (const auto& Tags : m_pFile->m_segment.Tags) {
		for (const auto& Tag : Tags->Tag) {
			for (const auto& SimpleTag : Tag->SimpleTag) {
				if (!SimpleTag->TagString.IsEmpty()) {
					if (SimpleTag->TagName == L"TITLE") {
						SetProperty(L"TITL", SimpleTag->TagString);
					} else if (SimpleTag->TagName == L"ARTIST") {
						SetProperty(L"AUTH", SimpleTag->TagString);
					} else if (SimpleTag->TagName == L"DATE_RELEASED") {
						SetProperty(L"DATE", SimpleTag->TagString);
					} else if (SimpleTag->TagName == L"COPYRIGHT") {
						SetProperty(L"CPYR", SimpleTag->TagString);
					} else if (SimpleTag->TagName == L"COMMENT" || SimpleTag->TagName == L"DESCRIPTION") {
						SetProperty(L"DESC", SimpleTag->TagString);
					} else if (SimpleTag->TagName == L"RATING") {
						SetProperty(L"RTNG", SimpleTag->TagString);
					} else if (SimpleTag->TagName == L"ALBUM") {
						SetProperty(L"ALBUM", SimpleTag->TagString);
					} else if (SimpleTag->TagName == L"3d-plane") {
						pg_offsets.push_back(_wtoi(SimpleTag->TagString));
					} else if (SimpleTag->TagName == L"ROTATE" || SimpleTag->TagName == L"ROTATION") {
						SetProperty(L"ROTATION", SimpleTag->TagString);
					}
				}
			}
		}
	}

	if (pg_offsets.size()) {
		CString offsets;

		pg_offsets.sort();
		pg_offsets.unique();
		for (auto it = pg_offsets.begin(); it != pg_offsets.end(); it++) {
			if (offsets.IsEmpty()) {
				offsets.Format(L"%d", *it);
			} else {
				offsets.AppendFormat(L",%d", *it);
			}
		}

		SetProperty(L"stereo_subtitle_offset_ids", offsets);
	}

	CComBSTR title;
	if (!videoTrackName.IsEmpty()
			&& (FAILED(GetProperty(L"TITL", &title)) || !title.Length())) {
		SetProperty(L"TITL", videoTrackName);
	}
	title.Empty();

	CComBSTR date;
	if (SUCCEEDED(GetProperty(L"TITL", &title)) & SUCCEEDED(GetProperty(L"DATE", &date))
			&& title.Length() && date.Length()) {
		CString Title;
		Title.Format(L"%s (%s)", title, date);
		SetProperty(L"TITL", Title);
	}

	if (!m_pOutputs.IsEmpty() && !m_pFile->m_segment.Cues.empty()) {
		auto& s = m_pFile->m_segment;
		const UINT64 TrackNumber = s.GetMasterTrack();
		QWORD lastCueClusterPosition = ULONGLONG_MAX;

		for (const auto& pCue : s.Cues) {
			for (const auto& pCuePoint : pCue->CuePoints) {
				for (const auto& pCueTrackPositions : pCuePoint->CueTrackPositions) {
					if (TrackNumber != pCueTrackPositions->CueTrack) {
						continue;
					}

					// prevent processing the same position
					if (lastCueClusterPosition == pCueTrackPositions->CueClusterPosition) {
						continue;
					}
					lastCueClusterPosition = pCueTrackPositions->CueClusterPosition;

					const auto cueTime = s.GetRefTime(pCuePoint->CueTime);
					const auto rtOffset = (cueTime >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;

					m_sps.push_back(SyncPoint{cueTime - rtOffset, __int64(m_pSegment->m_start + pCueTrackPositions->CueClusterPosition)});
				}
			}
		}

		if (!m_sps.empty()) {
			std::sort(m_sps.begin(), m_sps.end(), [](const SyncPoint& a, const SyncPoint& b) {
				return (a.rt < b.rt);
			});
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

void CMatroskaSplitterFilter::SetupChapters(LPCSTR lng, ChapterAtom* parent, int level)
{
	CString tabs(L'+', level);

	if (!tabs.IsEmpty()) {
		tabs += L' ';
	}

	for (const auto& ca : parent->ChapterAtoms) {
		// ChapUID zero not allow by Matroska specs
		const UINT64 ChapUID  = ca->ChapterUID;
		ChapterAtom* ca = (ChapUID == 0) ? nullptr : m_pFile->m_segment.FindChapterAtom(ChapUID);

		if (ca) {
			CString name, first;

			for (const auto& cd : ca->ChapterDisplays) {
				if (first.IsEmpty()) {
					first = cd->ChapString;
				}
				if (cd->ChapLanguage == lng) {
					name = cd->ChapString;
				}
			}

			name = tabs + (!name.IsEmpty() ? name : first);

			const auto chapterTime = (REFERENCE_TIME)ca->ChapterTimeStart / 100;
			const auto rtOffset = (chapterTime >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;
			ChapAppend(chapterTime - rtOffset, name);

			if (!ca->ChapterAtoms.empty() && level < 5) {
				// level < 5 - hard limit for the number of levels
				SetupChapters(lng, ca, level + 1);
			}
		}
	}
}

void CMatroskaSplitterFilter::InstallFonts()
{
	for (const auto& pA : m_pFile->m_segment.Attachments) {
		for (const auto& pF : pA->AttachedFiles) {
			if (pF->FileMimeType == "application/x-truetype-font" ||
					pF->FileMimeType == "application/x-font-ttf" ||
					pF->FileMimeType == "application/vnd.ms-opentype") {
				// assume this is a font resource

				if (BYTE* pData = DNew BYTE[(UINT)pF->FileDataLen]) {
					m_pFile->Seek(pF->FileDataPos);

					if (SUCCEEDED(m_pFile->ByteRead(pData, pF->FileDataLen))) {
						m_fontinst.InstallFontMemory(pData, (UINT)pF->FileDataLen);
					}

					delete [] pData;
				}
			}
		}
	}
}

void CMatroskaSplitterFilter::SendVorbisHeaderSample()
{
	HRESULT hr;
	for (const auto& item : m_pTrackEntryMap) {
		DWORD TrackNumber = item.first;
		TrackEntry* pTE   = item.second;

		CBaseSplitterOutputPin* pPin = GetOutputPin(TrackNumber);

		if (!(pTE && pPin && pPin->IsConnected())) {
			continue;
		}

		if (pTE->CodecID.ToString() == "A_VORBIS" && pPin->CurrentMediaType().subtype == MEDIASUBTYPE_Vorbis
				&& pTE->CodecPrivate.size() > 0) {
			BYTE* ptr = pTE->CodecPrivate.data();

			std::list<int> sizes;
			long last = 0;
			for (BYTE n = *ptr++; n > 0; n--) {
				int size = 0;
				do {
					size += *ptr;
				} while (*ptr++ == 0xff);
				sizes.push_back(size);
				last += size;
			}
			sizes.push_back(pTE->CodecPrivate.size() - (ptr - pTE->CodecPrivate.data()) - last);

			hr = S_OK;

			for (const long len : sizes) {

				CAutoPtr<CPacket> p(DNew CPacket());
				p->TrackNumber	= (DWORD)pTE->TrackNumber;
				p->rtStart		= 0;
				p->rtStop		= 1;
				p->bSyncPoint	= FALSE;

				p->SetData(ptr, len);
				ptr += len;

				hr = DeliverPacket(p);
			}

			if (FAILED(hr)) {
				DLog(L"MatroskaSplitterFilter::SendVorbisHeaderSample() - ERROR: Vorbis initialization failed for stream %u", TrackNumber);
			}
		}
	}
}

bool CMatroskaSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CMatroskaSplitterFilter");

	CMatroskaNode Root(m_pFile);
	if (!m_pFile) {
		return false;
	}

	auto& s = m_pFile->m_segment;

	// reindex if needed
	if (m_pFile->IsRandomAccess() && m_pFile->m_segment.Cues.empty()) {
		m_pSegment = Root.Child(MATROSKA_ID_SEGMENT);
		m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);

		const UINT64 TrackNumber = s.GetMasterTrack();

		m_nOpenProgress = 0;
		s.SegmentInfo.Duration.Set(0);

		CAutoPtr<Cue> pCue(DNew Cue());

		do {
			Cluster c;
			c.ParseTimeCode(m_pCluster);

			const auto clusterTime = s.GetRefTime(c.TimeCode);
			const auto rtOffset = (clusterTime >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;

			s.SegmentInfo.Duration.Set((float)c.TimeCode - rtOffset / 10000);

			CAutoPtr<CuePoint> pCuePoint(DNew CuePoint());
			CAutoPtr<CueTrackPosition> pCueTrackPosition(DNew CueTrackPosition());
			pCuePoint->CueTime.Set(c.TimeCode);
			pCueTrackPosition->CueTrack.Set(TrackNumber);
			pCueTrackPosition->CueClusterPosition.Set(m_pCluster->m_filepos - m_pSegment->m_start);
			pCuePoint->CueTrackPositions.emplace_back(pCueTrackPosition);
			pCue->CuePoints.emplace_back(pCuePoint);

			m_nOpenProgress = m_pFile->GetPos() * 100 / m_pFile->GetLength();

			DWORD cmd;
			if (CheckRequest(&cmd)) {
				if (cmd == CMD_EXIT) {
					m_fAbort = true;
				} else {
					Reply(S_OK);
				}
			}
		} while (!m_fAbort && m_pCluster->Next(true));

		m_pCluster.Free();

		m_nOpenProgress = 100;

		if (!m_fAbort) {
			s.Cues.emplace_back(pCue);
		}

		m_fAbort = false;

		if (!s.Cues.empty()) {
			m_rtDuration = (REFERENCE_TIME)(s.SegmentInfo.Duration * s.SegmentInfo.TimeCodeScale / 100);
			m_rtNewStop  = m_rtStop = m_rtDuration;

			QWORD lastCueClusterPosition = ULONGLONG_MAX;

			for (const auto& pCue : s.Cues) {
				for (const auto& pCuePoint : pCue->CuePoints) {
					for (const auto& pCueTrackPositions : pCuePoint->CueTrackPositions) {
						if (TrackNumber != pCueTrackPositions->CueTrack) {
							continue;
						}

						// prevent processing the same position
						if (lastCueClusterPosition == pCueTrackPositions->CueClusterPosition) {
							continue;
						}
						lastCueClusterPosition = pCueTrackPositions->CueClusterPosition;

						const auto cueTime = s.GetRefTime(pCuePoint->CueTime);
						const auto rtOffset = (cueTime >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;

						m_sps.push_back(SyncPoint{cueTime - rtOffset, __int64(m_pSegment->m_start + pCueTrackPositions->CueClusterPosition)});
					}
				}
			}

			if (!m_sps.empty()) {
				std::sort(m_sps.begin(), m_sps.end(), [](const SyncPoint& a, const SyncPoint& b) {
					return (a.rt < b.rt);
				});
			}
		}
	}

	if (!m_subtitlesTrackNumbers.empty()) {
		for (const auto& pCue : s.Cues) {
			for (const auto& pCuePoint : pCue->CuePoints) {
				for (const auto& pCueTrackPositions : pCuePoint->CueTrackPositions) {
					if (pCueTrackPositions->CueDuration && pCueTrackPositions->CueRelativePosition) {
						m_bSupportSubtitlesCueDuration = TRUE;
						break;
					}
				}

				if (m_bSupportSubtitlesCueDuration) {
					break;
				}
			}

			if (m_bSupportSubtitlesCueDuration) {
				break;
			}
		}
	}

	return true;
}

void CMatroskaSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);
	m_pBlock.Free();

	if (!m_pCluster) {
		return;
	}

	m_Cluster_seek_rt = INVALID_TIME;
	m_Cluster_seek_pos = 0;

	if (rt > 0) {
		Segment& s = m_pFile->m_segment;

		// Plan A
		for (auto it = m_sps.rbegin(); it != m_sps.rend(); ++it) {
			if (rt < it->rt) {
				continue;
			}

			m_pCluster->SeekTo(it->fp);
			if (FAILED(m_pCluster->Parse())) {
				continue;
			}

			Cluster c;
			c.ParseTimeCode(m_pCluster);

			const auto clusterTime = s.GetRefTime(c.TimeCode);
			const auto rtOffset = (clusterTime >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;
			const REFERENCE_TIME seek_rt = clusterTime - rtOffset;
			if (seek_rt <= rt) {
				DLog(L"CMatroskaSplitterFilter::DemuxSeek() : plan A - %s => %s, [%10I64d - %10I64d]", ReftimeToString(rt), ReftimeToString(seek_rt), rt, seek_rt);
				goto end;
			}
		}
		{
			// Plan B
			m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);
			REFERENCE_TIME seek_rt = 0;

			do {
				Cluster c;
				if (FAILED(c.ParseTimeCode(m_pCluster))) {
					continue;
				}

				const auto clusterTime = s.GetRefTime(c.TimeCode);
				const auto rtOffset = (clusterTime >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;
				const REFERENCE_TIME seek_rt2 = clusterTime - rtOffset;
				if (seek_rt2 > rt) {
					break;
				}

				seek_rt = seek_rt2;
			} while (m_pCluster->Next());

			if (seek_rt > 0 && seek_rt < m_rtDuration) {
				m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);

				do {
					Cluster c;
					if (FAILED(c.ParseTimeCode(m_pCluster))) {
						continue;
					}

					const auto clusterTime = s.GetRefTime(c.TimeCode);
					const auto rtOffset = (clusterTime >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;
					if ((clusterTime - rtOffset) == seek_rt) {
						DLog(L"CMatroskaSplitterFilter::DemuxSeek(), plan B : %s => %s, [%10I64d - %10I64d]", ReftimeToString(rt), ReftimeToString(seek_rt), rt, seek_rt);
						goto end;
					}
				} while (m_pCluster->Next());
			}
		}

		// epic fail ...
		m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);
		DLog(L"CMatroskaSplitterFilter::DemuxSeek(), epic fail ... start from begin");

	end:
		Cluster c;
		c.ParseTimeCode(m_pCluster);
		m_Cluster_seek_rt = s.GetRefTime(c.TimeCode);
		const auto rtOffset = (m_Cluster_seek_rt >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;
		m_Cluster_seek_pos = m_pCluster->m_filepos;
		DLog(L"CMatroskaSplitterFilter::DemuxSeek() : Final(Cluster timecode) - %s => %s, [%10I64d - %10I64d], pos - %I64d", ReftimeToString(rt), ReftimeToString(m_Cluster_seek_rt - rtOffset), rt, m_Cluster_seek_rt - rtOffset, m_Cluster_seek_pos);
	}
}

#define SetBlockTime																\
	REFERENCE_TIME duration = 0;													\
	if (p->bg->BlockDuration.IsValid()) {											\
		duration = m_pFile->m_segment.GetRefTime(p->bg->BlockDuration);				\
	} else if (pTE->DefaultDuration) {												\
		duration = (pTE->DefaultDuration / 100) * p->bg->Block.BlockData.size();	\
	}																				\
	if (pTE->TrackType == TrackEntry::TypeSubtitle && !duration) {					\
		duration = 1;																\
	}																				\
	\
	p->rtStart = clusterTime + s.GetRefTime(p->bg->Block.TimeCode);					\
	p->rtStop = p->rtStart + duration;												\
	\
	p->rtStart -= rtOffset;															\
	p->rtStop -= rtOffset;															\

bool CMatroskaSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	if (!m_pCluster) {
		return true;
	}

	SendVorbisHeaderSample(); // HACK: init vorbis decoder with the headers

	const auto& s = m_pFile->m_segment;

	if (m_Cluster_seek_rt > 0 && m_bSupportSubtitlesCueDuration) {
		CAutoPtr<CMatroskaNode> pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);
		if (pCluster) {
			bool bBreak = false;
			QWORD lastCueRelativePosition = ULONGLONG_MAX;
			for (const auto& pCue : s.Cues) {
				for (auto it = pCue->CuePoints.crbegin(); it != pCue->CuePoints.crend(); it++) {
					const auto& pCuePoint = *it;
					REFERENCE_TIME cueTime = s.GetRefTime(pCuePoint->CueTime);
					if (cueTime > m_Cluster_seek_rt) {
						continue;
					}

					if (cueTime < m_Cluster_seek_rt - 30 * UNITS) {
						bBreak = true;
						break;
					}

					for (const auto& pCueTrackPositions : pCuePoint->CueTrackPositions) {
						if (!Contains(m_subtitlesTrackNumbers, (UINT64)pCueTrackPositions->CueTrack)) {
							continue;
						}

						if (!pCueTrackPositions->CueDuration || !pCueTrackPositions->CueRelativePosition) {
							continue;
						}

						const REFERENCE_TIME cueDuration = s.GetRefTime(pCueTrackPositions->CueDuration);

						if (cueTime + cueDuration > m_Cluster_seek_rt
								&& (m_pSegment->m_start + pCueTrackPositions->CueClusterPosition) < m_Cluster_seek_pos) {
							pCluster->SeekTo(m_pSegment->m_start + pCueTrackPositions->CueClusterPosition);
							if (FAILED(pCluster->Parse())) {
								continue;
							}

							const QWORD pos = pCluster->GetPos();
							// prevent processing the same position
							if (lastCueRelativePosition == pos + pCueTrackPositions->CueRelativePosition) {
								continue;
							}
							lastCueRelativePosition = pos + pCueTrackPositions->CueRelativePosition;

							pCluster->SeekTo(pos + pCueTrackPositions->CueRelativePosition);
							CAutoPtr<CMatroskaNode> pBlock(DNew CMatroskaNode(pCluster));
							if (!pBlock) {
								continue;
							}

							Cluster c;
							c.ParseTimeCode(pCluster);
							const auto clusterTime = s.GetRefTime(c.TimeCode);
							const auto rtOffset = (clusterTime >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;

							CBlockGroupNode bgn;
							if (pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
								bgn.Parse(pBlock, true);
							} else if (pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
								CAutoPtr<BlockGroup> bg(DNew BlockGroup());
								bg->Block.Parse(pBlock, true);
								if (!(bg->Block.Lacing & 0x80)) {
									bg->ReferenceBlock.Set(0); // not a kf
								}
								bgn.emplace_back(bg);
							}

							for (auto &bg : bgn) {
								CAutoPtr<CMatroskaPacket> p(DNew CMatroskaPacket());
								p->bg = bg;

								if (!Contains(m_subtitlesTrackNumbers, (UINT64)p->bg->Block.TrackNumber)) {
									continue;
								}

								p->bSyncPoint = !p->bg->ReferenceBlock.IsValid();
								p->TrackNumber = (DWORD)p->bg->Block.TrackNumber;

								auto it = m_pTrackEntryMap.find(p->TrackNumber);
								if (it == m_pTrackEntryMap.end() || !(*it).second) {
									continue;
								}
								TrackEntry* pTE = (*it).second;

								SetBlockTime;

								if (p->rtStart >= m_Cluster_seek_rt || p->rtStop < m_Cluster_seek_rt) {
									continue;
								}

								hr = DeliverMatroskaPacket(pTE, p);
							}
						}
					}
				}

				if (bBreak) {
					break;
				}
			}
		}
	}

	do {
		if (!m_pBlock) {
			m_pBlock = m_pCluster->GetFirstBlock();
		}
		if (!m_pBlock) {
			continue;
		}

		Cluster c;
		c.ParseTimeCode(m_pCluster);
		const auto clusterTime = s.GetRefTime(c.TimeCode);
		const auto rtOffset = (clusterTime >= m_pFile->m_rtOffset) ? m_pFile->m_rtOffset : 0LL;

		do {
			CBlockGroupNode bgn;

			if (m_pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
				bgn.Parse(m_pBlock, true);
			} else if (m_pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
				CAutoPtr<BlockGroup> bg(DNew BlockGroup());
				bg->Block.Parse(m_pBlock, true);
				if (!(bg->Block.Lacing & 0x80)) {
					bg->ReferenceBlock.Set(0); // not a kf
				}
				bgn.emplace_back(bg);
			}

			for (auto &bg : bgn) {
				CAutoPtr<CMatroskaPacket> p(DNew CMatroskaPacket());
				p->bg = bg;

				p->bSyncPoint = !p->bg->ReferenceBlock.IsValid();
				p->TrackNumber = (DWORD)p->bg->Block.TrackNumber;

				auto it = m_pTrackEntryMap.find(p->TrackNumber);
				if (it == m_pTrackEntryMap.end() || !(*it).second) {
					continue;
				}
				TrackEntry* pTE = (*it).second;

				SetBlockTime;

				hr = DeliverMatroskaPacket(pTE, p);

				if (FAILED(hr)) {
					break;
				}
			}
		} while (m_pBlock->NextBlock() && SUCCEEDED(hr) && !CheckRequest(nullptr));

		m_pBlock.Free();
	} while (m_pFile->GetPos() < (__int64)(m_pFile->m_segment.pos + m_pFile->m_segment.len)
			 && m_pCluster->Next(true) && SUCCEEDED(hr) && !CheckRequest(nullptr));

	m_pCluster.Free();

	CAutoLock cAutoLock(&m_csPackets);
	for (auto& [TrackNumber, packets] : m_packets) {
		for (auto& p : packets) {
			DeliverMatroskaPacket(p);
		}

		packets.clear();
	}

	return true;
}

HRESULT CMatroskaSplitterFilter::DeliverMatroskaPacket(TrackEntry* pTE, CAutoPtr<CMatroskaPacket> p)
{
	for (const auto& pb : p->bg->Block.BlockData) {
		pTE->Expand(*pb, ContentEncoding::AllFrameContents);
	}

	HRESULT hr = S_OK;
	if (pTE->TrackType == TrackEntry::TypeSubtitle) {
		hr = DeliverMatroskaPacket(p);
	} else {
		CAutoLock cAutoLock(&m_csPackets);
		auto& packets = m_packets[p->TrackNumber];
		packets.emplace_back(p);

		if (packets.size() == 2) {
			const auto rtBlockDuration = packets.back()->rtStart - packets.front()->rtStart;
			hr = DeliverMatroskaPacket(packets.front(), rtBlockDuration);
			packets.pop_front();
		}
	}

	return hr;
}

// reconstruct full wavpack blocks from mangled matroska ones.
// From LAV's ffmpeg
static bool ParseWavpack(const CMediaType* mt, CBinary* Data, CAutoPtr<CPacket>& p)
{
	CheckPointer(mt->pbFormat, false);

	if (Data->size() < 12) {
		return false;
	}

	CGolombBuffer gb(Data->data(), Data->size());

	DWORD samples = gb.ReadDwordLE();
	WORD ver      = 0;
	int dstlen    = 0;
	int offset    = 0;

	if (mt->formattype == FORMAT_WaveFormatEx) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt->Format();
		if (wfe->cbSize >= 2) {
			ver = AV_RL16(mt->pbFormat);
		}
	}

	CAutoPtr<CBinary> ptr(DNew CBinary());

	while (gb.RemainingSize() >= 8) {
		DWORD flags     = gb.ReadDwordLE();
		DWORD crc       = gb.ReadDwordLE();
		DWORD blocksize = gb.RemainingSize();

		int multiblock = (flags & 0x1800) != 0x1800;
		if (multiblock) {
			if (gb.RemainingSize() < 4) {
				return false;
			}
			blocksize = gb.ReadDwordLE();
		}

		if (blocksize > (DWORD)gb.RemainingSize()) {
			return false;
		}

		ptr->resize(dstlen + blocksize + 32);
		BYTE *dst = ptr->data();

		dstlen += blocksize + 32;

		AV_WL32(dst + offset, MAKEFOURCC('w', 'v', 'p', 'k'));    // tag
		AV_WL32(dst + offset + 4,  blocksize + 24);               // blocksize - 8
		AV_WL16(dst + offset + 8,  ver);                          // version
		AV_WL16(dst + offset + 10, 0);                            // track/index_no
		AV_WL32(dst + offset + 12, 0);                            // total samples
		AV_WL32(dst + offset + 16, 0);                            // block index
		AV_WL32(dst + offset + 20, samples);                      // number of samples
		AV_WL32(dst + offset + 24, flags);                        // flags
		AV_WL32(dst + offset + 28, crc);                          // crc
		memcpy (dst + offset + 32, gb.GetBufferPos(), blocksize); // block data

		gb.SkipBytes(blocksize);
		offset += blocksize + 32;
	}

	p->SetData(ptr->data(), ptr->size());

	return true;
}

HRESULT CMatroskaSplitterFilter::DeliverMatroskaPacket(CAutoPtr<CMatroskaPacket> p, REFERENCE_TIME rtBlockDuration/* = 0*/)
{
	const auto pPin = GetOutputPin(p->TrackNumber);
	CheckPointer(pPin, E_FAIL);

	HRESULT hr = S_FALSE;

	auto& rtLastDuration = m_lastDuration[p->TrackNumber];
	const auto& mt = pPin->CurrentMediaType();

	const size_t BlockCount = p->bg->Block.BlockData.size();

	REFERENCE_TIME rtStart    = p->rtStart;
	REFERENCE_TIME rtDuration = 0;
	if (p->rtStop != p->rtStart) {
		rtDuration = (p->rtStop - p->rtStart) / BlockCount;
	} else if (rtBlockDuration > 0) {
		rtDuration = rtBlockDuration / BlockCount;
	} else if (rtLastDuration) {
		rtDuration = rtLastDuration;
	}
	REFERENCE_TIME rtStop = rtStart + rtDuration;

	rtLastDuration = rtDuration;

	for (const auto& pb : p->bg->Block.BlockData) {
		CAutoPtr<CPacket> pOutput(DNew CPacket());

		pOutput->TrackNumber    = p->TrackNumber;
		pOutput->bDiscontinuity = p->bDiscontinuity;
		pOutput->bSyncPoint     = p->bSyncPoint;
		pOutput->rtStart        = rtStart;
		pOutput->rtStop         = rtStop;

		if (mt.subtype == MEDIASUBTYPE_DVB_SUBTITLES) {
			// Add DBV subtitle missing start code - 0x20 0x00 (in Matroska DVB packets start with 0x0F ...)
			static BYTE start_code[2] = {0x20, 0x00};
			pOutput->SetData(start_code, sizeof(start_code));
			pOutput->AppendData(pb->data(), pb->size());
		} else if (mt.subtype == MEDIASUBTYPE_WAVPACK4) {
			if (!ParseWavpack(&mt, pb, pOutput)) {
				continue;
			}
		} else if (mt.subtype == MEDIASUBTYPE_icpf) {
			const DWORD data[2] = {pb->size(), FCC('icpf')};
			pOutput->SetData(&data[0], sizeof(data));
			pOutput->AppendData(pb->data(), pb->size());
		} else if (mt.subtype == MEDIASUBTYPE_VP90) {
			REFERENCE_TIME rtStartTmp = rtStart;
			REFERENCE_TIME rtStopTmp = rtStop;

			const BYTE* pData = pb->data();
			size_t size = pb->size();

			const BYTE marker = pData[size - 1];
			if ((marker & 0xe0) == 0xc0) {
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

						CAutoPtr<CPacket> pPacket(DNew CPacket());
						pPacket->SetData(pData, sz);

						pPacket->TrackNumber    = pOutput->TrackNumber;
						pPacket->bDiscontinuity = pOutput->bDiscontinuity;
						pPacket->bSyncPoint     = pOutput->bSyncPoint;

						if (pData[0] & 0x2) {
							pPacket->rtStart = rtStartTmp;
							pPacket->rtStop  = rtStopTmp;
							rtStartTmp      = INVALID_TIME;
							rtStopTmp       = INVALID_TIME;
						}
						if (S_OK != (hr = DeliverPacket(pPacket))) {
							break;
						}

						pData += sz;
						size -= sz;
					}
				}

				rtStart += rtDuration;
				rtStop += rtDuration;

				p->bSyncPoint     = FALSE;
				p->bDiscontinuity = FALSE;

				continue;
			}

			pOutput->SetData(pb->data(), pb->size());
		} else {
			pOutput->SetData(pb->data(), pb->size());
		}

		if (S_OK != (hr = DeliverPacket(pOutput))) {
			break;
		}

		rtStart += rtDuration;
		rtStop += rtDuration;

		p->bSyncPoint     = false;
		p->bDiscontinuity = false;
	}

	if (mt.subtype == MEDIASUBTYPE_WAVPACK4) {
		for (const auto& bm : p->bg->ba.bm) {
			CAutoPtr<CPacket> pOutput(DNew CPacket());

			pOutput->TrackNumber = p->TrackNumber;
			pOutput->rtStart     = p->rtStart;
			pOutput->rtStop      = p->rtStop;

			if (!ParseWavpack(&mt, &bm->BlockAdditional, pOutput)) {
				continue;
			}

			if (S_OK != (hr = DeliverPacket(pOutput))) {
				break;
			}
		}
	}

	return hr;
}

// IKeyFrameInfo

STDMETHODIMP CMatroskaSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);
	nKFs = m_sps.size();

	return S_OK;
}

STDMETHODIMP CMatroskaSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);

	if (*pFormat != TIME_FORMAT_MEDIA_TIME) {
		return E_INVALIDARG;
	}

	nKFs = 0;
	for (const auto& sps : m_sps) {
		pKFs[nKFs++] = sps.rt;
	}

	return S_OK;
}

//
// CMatroskaSourceFilter
//

CMatroskaSourceFilter::CMatroskaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMatroskaSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

// ITrackInfo

TrackEntry* CMatroskaSplitterFilter::GetTrackEntryAt(UINT aTrackIdx)
{
	if (aTrackIdx >= m_pOrderedTrackArray.size()) {
		return nullptr;
	}
	return m_pOrderedTrackArray[aTrackIdx];
}

STDMETHODIMP_(UINT) CMatroskaSplitterFilter::GetTrackCount()
{
	return (UINT)m_pTrackEntryMap.size();
}

STDMETHODIMP_(BOOL) CMatroskaSplitterFilter::GetTrackInfo(UINT aTrackIdx, struct TrackElement* pStructureToFill)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == nullptr) {
		return FALSE;
	}

	pStructureToFill->FlagDefault = !!pTE->FlagDefault;
	pStructureToFill->FlagForced = !!pTE->FlagForced;
	pStructureToFill->FlagLacing = !!pTE->FlagLacing;
	strncpy_s(pStructureToFill->Language, pTE->Language, 3);
	if (pStructureToFill->Language[0] == '\0') {
		strncpy_s(pStructureToFill->Language, "eng", 3);
	}
	pStructureToFill->Language[3] = '\0';
	pStructureToFill->MaxCache = (UINT)pTE->MaxCache;
	pStructureToFill->MinCache = (UINT)pTE->MinCache;
	pStructureToFill->Type = (BYTE)pTE->TrackType;
	return TRUE;
}

STDMETHODIMP_(BOOL) CMatroskaSplitterFilter::GetTrackExtendedInfo(UINT aTrackIdx, void* pStructureToFill)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == nullptr) {
		return FALSE;
	}

	if (pTE->TrackType == TrackEntry::TypeVideo) {
		TrackExtendedInfoVideo* pTEIV = (TrackExtendedInfoVideo*)pStructureToFill;
		pTEIV->AspectRatioType = (BYTE)pTE->v.AspectRatioType;
		pTEIV->DisplayUnit = (BYTE)pTE->v.DisplayUnit;
		pTEIV->DisplayWidth = (UINT)pTE->v.DisplayWidth;
		pTEIV->DisplayHeight = (UINT)pTE->v.DisplayHeight;
		pTEIV->Interlaced = !!pTE->v.FlagInterlaced;
		pTEIV->PixelWidth = (UINT)pTE->v.PixelWidth;
		pTEIV->PixelHeight = (UINT)pTE->v.PixelHeight;
	} else if (pTE->TrackType == TrackEntry::TypeAudio) {
		TrackExtendedInfoAudio* pTEIA = (TrackExtendedInfoAudio*)pStructureToFill;
		pTEIA->BitDepth = (UINT)pTE->a.BitDepth;
		pTEIA->Channels = (UINT)pTE->a.Channels;
		pTEIA->OutputSamplingFrequency = (FLOAT)pTE->a.OutputSamplingFrequency;
		pTEIA->SamplingFreq = (FLOAT)pTE->a.SamplingFrequency;
	} else {
		return FALSE;
	}

	return TRUE;
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetTrackName(UINT aTrackIdx)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == nullptr) {
		return nullptr;
	}
	return pTE->Name.AllocSysString();
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetTrackCodecID(UINT aTrackIdx)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == nullptr) {
		return nullptr;
	}
	return pTE->CodecID.ToString().AllocSysString();
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetTrackCodecName(UINT aTrackIdx)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == nullptr) {
		return nullptr;
	}
	return pTE->CodecName.AllocSysString();
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetTrackCodecInfoURL(UINT aTrackIdx)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == nullptr) {
		return nullptr;
	}
	return pTE->CodecInfoURL.AllocSysString();
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetTrackCodecDownloadURL(UINT aTrackIdx)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == nullptr) {
		return nullptr;
	}
	return pTE->CodecDownloadURL.AllocSysString();
}

// ISpecifyPropertyPages2

STDMETHODIMP CMatroskaSplitterFilter::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	pPages->pElems[0] = __uuidof(CMatroskaSplitterSettingsWnd);

	return S_OK;
}

STDMETHODIMP CMatroskaSplitterFilter::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != nullptr) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMatroskaSplitterSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMatroskaSplitterSettingsWnd>(nullptr, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// IMatroskaSplitterFilter

STDMETHODIMP CMatroskaSplitterFilter::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_MATROSKASplit)) {
		key.SetDWORDValue(OPT_LoadEmbeddedFonts, m_bLoadEmbeddedFonts);
		key.SetDWORDValue(OPT_CalcDuration, m_bCalcDuration);
	}
#else
	CProfile& profile = AfxGetProfile();
	profile.WriteBool(OPT_SECTION_MATROSKASplit, OPT_LoadEmbeddedFonts, m_bLoadEmbeddedFonts);
	profile.WriteBool(OPT_SECTION_MATROSKASplit, OPT_CalcDuration, m_bCalcDuration);
#endif

	return S_OK;
}

STDMETHODIMP CMatroskaSplitterFilter::SetLoadEmbeddedFonts(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bLoadEmbeddedFonts = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMatroskaSplitterFilter::GetLoadEmbeddedFonts()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bLoadEmbeddedFonts;
}

STDMETHODIMP CMatroskaSplitterFilter::SetCalcDuration(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bCalcDuration = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CMatroskaSplitterFilter::GetCalcDuration()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bCalcDuration;
}

// IExFilterInfo

STDMETHODIMP CMatroskaSplitterFilter::GetInt(LPCSTR field, int *value)
{
	CheckPointer(value, E_POINTER);

	if (!strcmp(field, "VIDEO_PROFILE")) {
		if (m_profile != -1) {
			*value = m_profile;
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
	else if (!strcmp(field, "VIDEO_INTERLACED")) {
		if (m_interlaced != -1) {
			*value = m_interlaced;
			return S_OK;
		}
		return E_ABORT;
	}
	else if (!strcmp(field, "VIDEO_FLAG_ONLY_DTS")) {
		*value = m_dtsonly;
		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP CMatroskaSplitterFilter::GetBin(LPCSTR field, LPVOID *value, unsigned *size)
{
	CheckPointer(value, E_POINTER);
	CheckPointer(size, E_POINTER);

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

	if (!strcmp(field, "VIDEO_COLOR_SPACE")) {
		if (m_ColorSpace) {
			*size = sizeof(*m_ColorSpace);
			*value = (LPVOID)LocalAlloc(LPTR, *size);
			memcpy(*value, m_ColorSpace, *size);

			return S_OK;
		}
		return E_ABORT;
	}

	return E_INVALIDARG;
}

//
// CMatroskaSplitterOutputPin
//

CMatroskaSplitterOutputPin::CMatroskaSplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
	if (mts[0].subtype == MEDIASUBTYPE_HDMVSUB) {
		m_SubtitleType = hdmvsub;
	}
	else if (mts[0].subtype == MEDIASUBTYPE_DVB_SUBTITLES) {
		m_SubtitleType = dvbsub;
	}
}

CMatroskaSplitterOutputPin::~CMatroskaSplitterOutputPin()
{
}

HRESULT CMatroskaSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_bNeedNextSubtitle = false;

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CMatroskaSplitterOutputPin::QueuePacket(CAutoPtr<CPacket> p)
{
	if (!ThreadExists()) {
		return S_FALSE;
	}

	bool force_packet = false;

	if (m_SubtitleType == hdmvsub) {
		if (p && p->size() >= 3) {
			const BYTE segtype = p->data()[0];
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
		if (p && p->size() >= 6) {
			BYTE* pos = p->data();
			BYTE* end = pos + p->size();

			while (pos + 6 < end) {
				if (*pos++ == 0x0F) {
					const WORD segtype = *pos++;
					pos += 2;
					const WORD seglength = _byteswap_ushort(GETU16(pos));
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

	if (S_OK == m_hrDeliver && (force_packet || ((CMatroskaSplitterFilter*)m_pSplitter)->IsHdmvDvbSubPinDrying())) {
		m_queue.Add(p);
		return m_hrDeliver;
	}

	return __super::QueuePacket(p);
}
