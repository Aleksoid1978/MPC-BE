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
#include <MMReg.h>
#include "MatroskaSplitter.h"
#include "../../../DSUtil/AudioParser.h"
#include "../../../DSUtil/VideoParser.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>
#include <basestruct.h>
#include <vector>

// option names
#define OPT_REGKEY_MATROSKASplit	_T("Software\\MPC-BE Filters\\Matroska Splitter")
#define OPT_SECTION_MATROSKASplit	_T("Filters\\Matroska Splitter")
#define OPT_LoadEmbeddedFonts		_T("LoadEmbeddedFonts")
#define OPT_CalcDuration			_T("CalculateDuration")

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_Matroska},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMatroskaSplitterFilter), MatroskaSplitterName, MERIT_NORMAL, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMatroskaSourceFilter), MatroskaSourceName, MERIT_NORMAL, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMatroskaSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMatroskaSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(
		__uuidof(CMatroskaSourceFilter),
		MEDIASUBTYPE_Matroska,
		_T("0,4,,1A45DFA3"),
		NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_Matroska);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CMatroskaSplitterFilter
//

CMatroskaSplitterFilter::CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMatroskaSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_bLoadEmbeddedFonts(true)
	, m_bCalcDuration(false)
	, m_Seek_rt(INVALID_TIME)
	, m_bSupportCueDuration(FALSE)
{
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
	m_bLoadEmbeddedFonts	= !!AfxGetApp()->GetProfileInt(OPT_SECTION_MATROSKASplit, OPT_LoadEmbeddedFonts, m_bLoadEmbeddedFonts);
	m_bCalcDuration			= !!AfxGetApp()->GetProfileInt(OPT_SECTION_MATROSKASplit, OPT_CalcDuration, m_bCalcDuration);
#endif
}

CMatroskaSplitterFilter::~CMatroskaSplitterFilter()
{
}

STDMETHODIMP CMatroskaSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(ITrackInfo)
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

static int compare(const void* a, const void* b)
{
	return (*(INT64*)a - *(INT64*)b);
}

// code from MediaInfo
static double Video_FrameRate_Rounding(double FrameRate)
{
	     if (FrameRate> 9.990 && FrameRate<=10.010)		FrameRate=10.000;
	else if (FrameRate>14.990 && FrameRate<=15.010)		FrameRate=15.000;
	else if (FrameRate>23.964 && FrameRate<=23.988)		FrameRate=23.976;
	else if (FrameRate>23.988 && FrameRate<=24.012)		FrameRate=24.000;
	else if (FrameRate>24.988 && FrameRate<=25.012)		FrameRate=25.000;
	else if (FrameRate>29.955 && FrameRate<=29.985)		FrameRate=29.970;
	else if (FrameRate>29.985 && FrameRate<=30.015)		FrameRate=30.000;
	else if (FrameRate>23.964*2 && FrameRate<=23.988*2)	FrameRate=23.976*2;
	else if (FrameRate>23.988*2 && FrameRate<=24.012*2)	FrameRate=24.000*2;
	else if (FrameRate>24.988*2 && FrameRate<=25.012*2)	FrameRate=25.000*2;
	else if (FrameRate>29.955*2 && FrameRate<=29.985*2)	FrameRate=29.970*2;
	else if (FrameRate>29.985*2 && FrameRate<=30.015*2)	FrameRate=30.000*2;

	return FrameRate;
}


HRESULT CMatroskaSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pTrackEntryMap.RemoveAll();
	m_pOrderedTrackArray.RemoveAll();

	CAtlArray<CMatroskaSplitterOutputPin*> pinOut;
	CAtlArray<TrackEntry*> pinOutTE;

	m_pFile.Attach(DNew CMatroskaFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

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

	POSITION pos = m_pFile->m_segment.Tracks.GetHeadPosition();
	while (pos) {
		Track* pT = m_pFile->m_segment.Tracks.GetNext(pos);

		POSITION pos2 = pT->TrackEntries.GetHeadPosition();
		while (pos2) {
			TrackEntry* pTE = pT->TrackEntries.GetNext(pos2);

			bool isSub = false;

			if (!pTE->Expand(pTE->CodecPrivate, ContentEncoding::TracksPrivateData)) {
				continue;
			}

			CStringA CodecID = pTE->CodecID.ToString();

			CString Name;
			Name.Format(L"Output %I64d", (UINT64)pTE->TrackNumber);

			CMediaType mt;
			CAtlArray<CMediaType> mts;

			if (pTE->TrackType == TrackEntry::TypeVideo) {
				Name.Format(L"Video %d", iVideo++);

				mt.majortype = MEDIATYPE_Video;
				mt.SetSampleSize(1); // variable frame size?
				mt.bFixedSizeSamples = FALSE; // most compressed video has a variable frame size
				mt.bTemporalCompression = TRUE; // stream may contain P or B frames

				if (CodecID == "V_MS/VFW/FOURCC") {
					mt.formattype = FORMAT_VideoInfo;
					VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.GetCount() - sizeof(BITMAPINFOHEADER));
					memset(mt.Format(), 0, mt.FormatLength());
					memcpy(&pvih->bmiHeader, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
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
						mts.Add(mt);
						if (mt.subtype == MEDIASUBTYPE_WVC1) {
							mt.subtype = MEDIASUBTYPE_WVC1_CYBERLINK;
							mts.InsertAt(0, mt);
						}

						if (mt.subtype == MEDIASUBTYPE_HM10) {
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

								__int64 startpos = m_pFile->GetPos();

								if (m_pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
									bgn.Parse(m_pBlock, true);
								} else if (m_pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
									CAutoPtr<BlockGroup> bg(DNew BlockGroup());
									bg->Block.Parse(m_pBlock, true);
									if (!(bg->Block.Lacing & 0x80)) {
										bg->ReferenceBlock.Set(0);    // not a kf
									}

									bgn.AddTail(bg);
								}
								__int64 endpos = m_pFile->GetPos();

								while (bgn.GetCount() && SUCCEEDED(hr)) {
									CAutoPtr<CMatroskaPacket> p(DNew CMatroskaPacket());
									p->bg = bgn.RemoveHead();
									if ((DWORD)p->bg->Block.TrackNumber != pTE->TrackNumber) {
										continue;
									}

									m_pFile->Seek(startpos);
									CBaseSplitterFileEx::hevchdr h;
									CMediaType mt2;
									if (m_pFile->CBaseSplitterFileEx::Read(h, endpos - startpos, &mt2)) {
										mts.InsertAt(0, mt2);
									}

									bIsParse = TRUE;
									break;
								}
							} while (m_pBlock->NextBlock() && SUCCEEDED(hr) && !CheckRequest(NULL) && !bIsParse);

							m_pBlock.Free();
							m_pCluster.Free();

							m_pFile->Seek(pos);
						}
					}
					bHasVideo = true;
				} else if (CodecID == "V_MPEG4/ISO/AVC") {
					if (pTE->CodecPrivate.GetCount() > 9) {
						vc_params_t params;
						AVCParser::ParseSequenceParameterSet(pTE->CodecPrivate.GetData() + 9, pTE->CodecPrivate.GetCount() - 9, params);
						codecAvgTimePerFrame = params.AvgTimePerFrame;
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
					CreateMPEG2VIfromAVC(&mt, &pbmi, 0, aspect, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());

					if (!bHasVideo)
						mts.Add(mt);
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
					CreateMPEG2VISimple(&mt, &pbmi, 0, aspect, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
					if (!bHasVideo)
						mts.Add(mt);
					bHasVideo = true;
				} else if (CodecID == "V_DIRAC") {
					mt.subtype = MEDIASUBTYPE_DiracVideo;
					mt.formattype = FORMAT_DiracVideoInfo;
					DIRACINFOHEADER* dvih = (DIRACINFOHEADER*)mt.AllocFormatBuffer(FIELD_OFFSET(DIRACINFOHEADER, dwSequenceHeader) + pTE->CodecPrivate.GetCount());
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
					memcpy(pSequenceHeader, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
					dvih->cbSequenceHeader = (DWORD)pTE->CodecPrivate.GetCount();

					if (!bHasVideo)
						mts.Add(mt);
					bHasVideo = true;
				} else if (CodecID == "V_MPEG2") {
					BYTE* seqhdr = pTE->CodecPrivate.GetData();
					DWORD len = (DWORD)pTE->CodecPrivate.GetCount();
					int w = (int)pTE->v.PixelWidth;
					int h = (int)pTE->v.PixelHeight;

					if (MakeMPEG2MediaType(mt, seqhdr, len, w, h)) {
						if (!bHasVideo)
							mts.Add(mt);
						bHasVideo = true;

						CAtlArray<BYTE> buf;
						buf.SetCount(len);
						memcpy(buf.GetData(), seqhdr, len);
						CBaseSplitterFileEx::seqhdr h;
						if (m_pFile->CBaseSplitterFileEx::Read(h, buf)) {
							codecAvgTimePerFrame = h.ifps;
						}
					}
				} else if (CodecID == "V_THEORA") {
					BYTE* thdr = pTE->CodecPrivate.GetData() + 3;

					mt.majortype		= MEDIATYPE_Video;
					mt.subtype			= FOURCCMap('OEHT');
					mt.formattype		= FORMAT_MPEG2_VIDEO;
					MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(sizeof(MPEG2VIDEOINFO) + pTE->CodecPrivate.GetCount());
					memset(mt.Format(), 0, mt.FormatLength());
					vih->hdr.bmiHeader.biSize		 = sizeof(vih->hdr.bmiHeader);
					vih->hdr.bmiHeader.biWidth		 = *(WORD*)&thdr[10] >> 4;
					vih->hdr.bmiHeader.biHeight		 = *(WORD*)&thdr[12] >> 4;
					vih->hdr.bmiHeader.biCompression = 'OEHT';
					vih->hdr.bmiHeader.biPlanes		 = 1;
					vih->hdr.bmiHeader.biBitCount	 = 24;
					int nFpsNum	= (thdr[22]<<24)|(thdr[23]<<16)|(thdr[24]<<8)|thdr[25];
					int nFpsDenum	= (thdr[26]<<24)|(thdr[27]<<16)|(thdr[28]<<8)|thdr[29];
					if (nFpsNum) {
						vih->hdr.AvgTimePerFrame = (REFERENCE_TIME)(10000000.0 * nFpsDenum / nFpsNum);
					}
					vih->hdr.dwPictAspectRatioX = (thdr[14]<<16)|(thdr[15]<<8)|thdr[16];
					vih->hdr.dwPictAspectRatioY = (thdr[17]<<16)|(thdr[18]<<8)|thdr[19];

					vih->cbSequenceHeader = (DWORD)pTE->CodecPrivate.GetCount();
					memcpy (&vih->dwSequenceHeader, pTE->CodecPrivate.GetData(), vih->cbSequenceHeader);

					if (!bHasVideo)
						mts.Add(mt);
					bHasVideo = true;
				} else if (CodecID == "V_DSHOW/MPEG1VIDEO" || CodecID == "V_MPEG1") {
					mt.majortype	= MEDIATYPE_Video;
					mt.subtype		= MEDIASUBTYPE_MPEG1Payload;
					mt.formattype	= FORMAT_MPEGVideo;

					MPEG1VIDEOINFO* pm1vi = (MPEG1VIDEOINFO*)mt.AllocFormatBuffer(sizeof(MPEG1VIDEOINFO) + pTE->CodecPrivate.GetCount());
					memset(mt.Format(), 0, mt.FormatLength());
					memcpy(mt.Format() + sizeof(MPEG1VIDEOINFO), pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());

					pm1vi->hdr.bmiHeader.biSize			= sizeof(pm1vi->hdr.bmiHeader);
					pm1vi->hdr.bmiHeader.biWidth		= (LONG)pTE->v.PixelWidth;
					pm1vi->hdr.bmiHeader.biHeight		= (LONG)pTE->v.PixelHeight;
					pm1vi->hdr.bmiHeader.biBitCount		= 12;
					pm1vi->hdr.bmiHeader.biSizeImage	= DIBSIZE(pm1vi->hdr.bmiHeader);

					mt.SetSampleSize(pm1vi->hdr.bmiHeader.biWidth * pm1vi->hdr.bmiHeader.biHeight * 4);
					if (!bHasVideo)
						mts.Add(mt);
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
					CreateMPEG2VISimple(&mt, &pbmi, 0, aspect, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());

					MPEG2VIDEOINFO* pm2vi	= (MPEG2VIDEOINFO*)mt.pbFormat;
					BYTE * extradata		= pTE->CodecPrivate.GetData();
					size_t size				= pTE->CodecPrivate.GetCount();
					vc_params_t params;
					if (HEVCParser::ParseHEVCDecoderConfigurationRecord(extradata, size, params, false)) {
						pm2vi->dwProfile	= params.profile;
						pm2vi->dwLevel		= params.level;
						pm2vi->dwFlags		= params.nal_length_size;
					}

					if (!pm2vi->dwFlags
							&& ((extradata[0] || extradata[1] || extradata[2] > 1 && size > 25))) {
						pm2vi->dwFlags = (extradata[21] & 3) + 1;
					}

					if (!bHasVideo)
						mts.Add(mt);
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
					} else if (CodecID == "V_VP8") {
						fourcc = FCC('VP80');
					} else if (CodecID == "V_VP9") {
						fourcc = FCC('VP90');
					} else if (CodecID == "V_QUICKTIME" && pTE->CodecPrivate.GetCount() >= 8) {
						if (m_pFile->m_ebml.DocTypeReadVersion == 1) {
							fourcc = *(DWORD*)(pTE->CodecPrivate.GetData());
						} else {
							fourcc = *(DWORD*)(pTE->CodecPrivate.GetData() + 4);
						}
					} else if (CodecID.Left(9) == "V_REAL/RV" && CodecID.GetLength() == 11) {
						fourcc = CodecID[7] + (CodecID[8] << 8) + (CodecID[9] << 16) + (CodecID[10] << 24);
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
						VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.GetCount());
						memset(mt.Format(), 0, mt.FormatLength());
						memcpy(mt.Format() + sizeof(VIDEOINFOHEADER), pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
						pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
						pvih->bmiHeader.biWidth = (LONG)pTE->v.PixelWidth;
						pvih->bmiHeader.biHeight = (LONG)pTE->v.PixelHeight;
						pvih->bmiHeader.biPlanes = 1; // must be set to 1.
						pvih->bmiHeader.biBitCount = bitdepth;
						pvih->bmiHeader.biCompression = fourcc;
						pvih->bmiHeader.biSizeImage = DIBSIZE(pvih->bmiHeader);

						if (!bHasVideo) {
							mts.Add(mt);
						}
						bHasVideo = true;
					}
				}

				if (!pTE->DefaultDuration && codecAvgTimePerFrame) {
					pTE->DefaultDuration.Set(codecAvgTimePerFrame * 100);
				}

				REFERENCE_TIME AvgTimePerFrame = 0;
				if (pTE->v.FramePerSec > 0) {
					AvgTimePerFrame = (REFERENCE_TIME)(10000000i64 / pTE->v.FramePerSec);
				} else if (pTE->DefaultDuration > 0) {
					AvgTimePerFrame = (REFERENCE_TIME)pTE->DefaultDuration / 100;
				}

				if (AvgTimePerFrame < 50000 // incorrect fps - calculate avarage value
						&& CodecID != "V_DSHOW/MPEG1VIDEO" && CodecID != "V_MPEG1" && CodecID != "V_MPEG2") {
					DbgLog((LOG_TRACE, 3, L"CMatroskaSplitterFilter::CreateOutputs() : calculate AvgTimePerFrame"));

					CMatroskaNode Root(m_pFile);
					m_pSegment = Root.Child(MATROSKA_ID_SEGMENT);
					m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);

					QWORD lastCueClusterPosition = ULONGLONG_MAX;

					CAtlArray<INT64> timecodes;
					bool readmore = true;

					CNode<Cue>* pCues;
					CNode<Cue>  Cues;
					if (m_pFile->m_segment.Cues.GetCount()) {
						pCues = &m_pFile->m_segment.Cues;
					} else {
						UINT64 TrackNumber = m_pFile->m_segment.GetMasterTrack();

						CAutoPtr<Cue> pCue(DNew Cue());

						do {
							Cluster c;
							c.ParseTimeCode(m_pCluster);

							CAutoPtr<CuePoint> pCuePoint(DNew CuePoint());
							CAutoPtr<CueTrackPosition> pCueTrackPosition(DNew CueTrackPosition());
							pCuePoint->CueTime.Set(c.TimeCode);
							pCueTrackPosition->CueTrack.Set(TrackNumber);
							pCueTrackPosition->CueClusterPosition.Set(m_pCluster->m_filepos - m_pSegment->m_start);
							pCuePoint->CueTrackPositions.AddTail(pCueTrackPosition);
							pCue->CuePoints.AddTail(pCuePoint);
						} while (m_pCluster->Next(true) && pCue->CuePoints.GetCount() < 2);

						Cues.AddTail(pCue);
						pCues = &Cues;
					}

					POSITION pos1 = pCues->GetHeadPosition();
					while (readmore && pos1) {
						Cue* pCue = pCues->GetNext(pos1);
						POSITION pos2 = pCue->CuePoints.GetHeadPosition();
						while (readmore && pos2) {
							CuePoint* pCuePoint = pCue->CuePoints.GetNext(pos2);
							POSITION pos3 = pCuePoint->CueTrackPositions.GetHeadPosition();
							while (readmore && pos3) {
								CueTrackPosition* pCueTrackPositions = pCuePoint->CueTrackPositions.GetNext(pos3);
								if (pCueTrackPositions->CueTrack != pTE->TrackNumber) {
									continue;
								}

								if (lastCueClusterPosition == pCueTrackPositions->CueClusterPosition) {
									continue;
								}
								lastCueClusterPosition = pCueTrackPositions->CueClusterPosition;

								m_pCluster->SeekTo(m_pSegment->m_start + pCueTrackPositions->CueClusterPosition);
								m_pCluster->Parse();

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
											if (!(bg->Block.Lacing & 0x80)) {
												bg->ReferenceBlock.Set(0);    // not a kf
											}
											bgn.AddTail(bg);
										}

										POSITION pos4 = bgn.GetHeadPosition();
										while (pos4) {
											BlockGroup* bg = bgn.GetNext(pos4);
											if (bg->Block.TrackNumber != pTE->TrackNumber) {
												continue;
											}
											INT64 tc = c.TimeCode + bg->Block.TimeCode;

											if (tc < 0) {
												continue;
											}

											timecodes.Add(tc);
											DbgLog((LOG_TRACE, 3, L"	=> Frame: %02d, TimeCode: %5I64d = %10I64d", timecodes.GetCount(), tc, m_pFile->m_segment.GetRefTime(tc)));

											if (timecodes.GetCount() >= 50) {
												readmore = false;
												break;
											}
										}
									} while (readmore && pBlock->NextBlock());
								}
							}
						}
					}

					m_pCluster.Free();

					if (Cues.GetCount()) {
						Cues.RemoveAll();
					}

					if (timecodes.GetCount()) {
						qsort(timecodes.GetData(), timecodes.GetCount(), sizeof(INT64), compare);

						CAtlArray<INT64> timecodes_diff;
						for (size_t i = 1; i < timecodes.GetCount(); i++) {
							if ((timecodes[i] - timecodes[i-1]) > 0) {
								timecodes_diff.Add(timecodes[i] - timecodes[i-1]);
							}
						}

						INT64 matchTC	= 0;
						UINT count		= 0;
						for (size_t i = 0; i < timecodes_diff.GetCount(); i++) {
							if (timecodes_diff[i] == 0 || timecodes_diff[i] == matchTC) {
								continue;
							}
							INT64 tc	= timecodes_diff[i];
							UINT count2	= 0;
							for (size_t j = 0; j < timecodes_diff.GetCount(); j++) {
								if ((timecodes_diff[j] >= tc * 0.9) && (timecodes_diff[j] <= tc * 1.1)) {
									count2++;
								}
							}
							if (count2 > count) {
								matchTC	= tc;
								count	= count2;
							}
						}

						if (matchTC) {

							for (size_t i = 0; i < timecodes_diff.GetCount();) {
								if ((timecodes_diff[i] < matchTC * 0.9) || (timecodes_diff[i] > matchTC * 1.1)) {
									timecodes_diff.RemoveAt(i);
								} else {
									i++;
								}
							}

							INT64 timecode = 0;
							count = 0;
							for (size_t i = 0; i < timecodes_diff.GetCount(); i++) {
								timecode += timecodes_diff[i];
								count++;
								DbgLog((LOG_TRACE, 3, L"	=> TimeCode_Diff: %02d : %5I64d", count, timecodes_diff[i]));
							}

							AvgTimePerFrame = m_pFile->m_segment.SegmentInfo.TimeCodeScale * (timecode) / (count * 100);

							double fps = 10000000.0 / AvgTimePerFrame;
							fps = Video_FrameRate_Rounding(fps);
							AvgTimePerFrame = 10000000.0 / fps;
						}
					}
				}

				if (AvgTimePerFrame < 50000) {
					AvgTimePerFrame = 417082; // set 23.976 as default
				}

				for (size_t i = 0; i < mts.GetCount(); i++) {
					if (mts[i].formattype == FORMAT_VideoInfo
							|| mts[i].formattype == FORMAT_VideoInfo2
							|| mts[i].formattype == FORMAT_MPEG2Video
							|| mts[i].formattype == FORMAT_MPEGVideo) {
						if (pTE->v.PixelWidth && pTE->v.PixelHeight) {
							RECT rect = {(LONG)pTE->v.VideoPixelCropLeft,
										 (LONG)pTE->v.VideoPixelCropTop,
										 (LONG)(pTE->v.PixelWidth - pTE->v.VideoPixelCropRight),
										 (LONG)(pTE->v.PixelHeight - pTE->v.VideoPixelCropBottom)
										};
							VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)mts[i].Format();
							vih->rcSource = vih->rcTarget = rect;
						}

						if (AvgTimePerFrame) {
							if (mts[i].subtype == MEDIASUBTYPE_MPEG1Payload) {
								AvgTimePerFrame *= 2; // Need more testing, but work on all sample that i have :)
							}
							((VIDEOINFOHEADER*)mts[i].Format())->AvgTimePerFrame = AvgTimePerFrame;
						}
					}
				}

				if (pTE->v.DisplayWidth && pTE->v.DisplayHeight) {
					for (size_t i = 0; i < mts.GetCount(); i++) {
						if (mts[i].formattype == FORMAT_VideoInfo) {
							mt = mts[i];
							DWORD vih1 = FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
							DWORD vih2 = FIELD_OFFSET(VIDEOINFOHEADER2, bmiHeader);
							DWORD bmi = mts[i].FormatLength() - FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
							mt.formattype = FORMAT_VideoInfo2;
							mt.AllocFormatBuffer(vih2 + bmi);
							memcpy(mt.Format(), mts[i].Format(), vih1);
							memset(mt.Format() + vih1, 0, vih2 - vih1);
							memcpy(mt.Format() + vih2, mts[i].Format() + vih1, bmi);

							CSize aspect((int)pTE->v.DisplayWidth, (int)pTE->v.DisplayHeight);
							ReduceDim(aspect);
							((VIDEOINFOHEADER2*)mt.Format())->dwPictAspectRatioX = aspect.cx;
							((VIDEOINFOHEADER2*)mt.Format())->dwPictAspectRatioY = aspect.cy;
							mts.InsertAt(i++, mt);
						} else if (mts[i].formattype == FORMAT_MPEG2Video) {
							CSize aspect((int)pTE->v.DisplayWidth, (int)pTE->v.DisplayHeight);
							ReduceDim(aspect);
							((MPEG2VIDEOINFO*)mts[i].Format())->hdr.dwPictAspectRatioX = aspect.cx;
							((MPEG2VIDEOINFO*)mts[i].Format())->hdr.dwPictAspectRatioY = aspect.cy;
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
					mts.Add(mt);
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

					mts.Add(mt);
				} else if (CodecID == "A_AC3") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_DOLBY_AC3);
					mts.Add(mt);
				} else if (CodecID == "A_EAC3") {
					mt.subtype = MEDIASUBTYPE_DOLBY_DDPLUS;
					mts.Add(mt);
				} else if (CodecID == "A_TRUEHD" ||
						   CodecID == "A_MLP") {
					mt.subtype = MEDIASUBTYPE_DOLBY_TRUEHD;
					mts.Add(mt);
				} else if (CodecID == "A_DTS") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_DTS2);
					mts.Add(mt);
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
					mts.Add(mt);
				} else if (CodecID == "A_AAC") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_RAW_AAC1);
					wfe->cbSize = (WORD)pTE->CodecPrivate.GetCount();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.GetCount());
					memcpy(wfe + 1, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
					mts.Add(mt);
				} else if (CodecID == "A_WAVPACK4") {
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_WAVPACK4);
					wfe->cbSize = (WORD)pTE->CodecPrivate.GetCount();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.GetCount());
					memcpy(wfe + 1, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
					mts.Add(mt);
				} else if (CodecID == "A_FLAC") {
					wfe->wFormatTag = WAVE_FORMAT_FLAC;
					wfe->cbSize = (WORD)pTE->CodecPrivate.GetCount();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.GetCount());
					memcpy(wfe + 1, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());

					mt.subtype = MEDIASUBTYPE_FLAC_FRAMED;
					mts.Add(mt);
					mt.subtype = MEDIASUBTYPE_FLAC;
					mts.Add(mt);
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
					mts.Add(mt);
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
					mts.Add(mt);
				} else if (CodecID == "A_MS/ACM") {
					wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(pTE->CodecPrivate.GetCount());
					memcpy(wfe, (WAVEFORMATEX*)pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
					if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe->cbSize == 22) {
						mt.subtype = ((WAVEFORMATEXTENSIBLE*)wfe)->SubFormat;
					}
					else mt.subtype = FOURCCMap(wfe->wFormatTag);
					mts.Add(mt);
				} else if (CodecID == "A_VORBIS") {
					CreateVorbisMediaType(mt, mts,
										  (DWORD)pTE->a.Channels,
										  (DWORD)pTE->a.SamplingFrequency,
										  DWORD(pTE->a.BitDepth),
										  pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
				} else if (CodecID.Find("A_AAC/") == 0) {
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

					mts.Add(mt);

					if (profile < 0) {
						wfe->cbSize = cbSize;
						wfe->nSamplesPerSec *= 2;
						wfe->nAvgBytesPerSec *= 2;

						mts.InsertAt(0, mt);
					}
				} else if (CodecID.Find("A_REAL/") == 0 && CodecID.GetLength() >= 11) {
					mt.subtype = FOURCCMap((DWORD)CodecID[7]|((DWORD)CodecID[8]<<8)|((DWORD)CodecID[9]<<16)|((DWORD)CodecID[10]<<24));
					mt.bTemporalCompression = TRUE;
					wfe->cbSize = (WORD)pTE->CodecPrivate.GetCount();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.GetCount());
					memcpy(wfe + 1, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
					wfe->cbSize = 0; // IMPORTANT: this is screwed, but cbSize has to be 0 and the extra data from codec priv must be after WAVEFORMATEX
					mts.Add(mt);
				} else if (CodecID == "A_QUICKTIME" && pTE->CodecPrivate.GetCount() >= 8) {
					DWORD* type = (DWORD*)(pTE->CodecPrivate.GetData() + 4);
					if (*type == MAKEFOURCC('Q','D','M','2') || *type == MAKEFOURCC('Q','D','M','C')) {
						mt.subtype = FOURCCMap(*type);
						wfe->cbSize = (WORD)pTE->CodecPrivate.GetCount();
						wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.GetCount());
						memcpy(wfe + 1, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
						mts.Add(mt);
					}
				} else if (CodecID == "A_OPUS" || CodecID == "A_OPUS/EXPERIMENTAL") {
					wfe->wFormatTag = WAVE_FORMAT_OPUS;
					mt.subtype = MEDIASUBTYPE_OPUS;
					wfe->cbSize = (WORD)pTE->CodecPrivate.GetCount();
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.GetCount());
					memcpy(wfe + 1, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
					mts.Add(mt);
				} else if (CodecID == "A_ALAC") {
					mt.subtype = MEDIASUBTYPE_ALAC;
					WORD cbSize = (WORD)pTE->CodecPrivate.GetCount() + 12;
					wfe->cbSize = cbSize;
					wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + cbSize);
					BYTE *p = (BYTE *)(wfe + 1);

					memset(p, 0, cbSize);
					memcpy(p + 3,  &cbSize, 1);
					memcpy(p + 4, (const unsigned char *)"alac", 4);
					memcpy(p + 12, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
					mts.Add(mt);
				}
			} else if (pTE->TrackType == TrackEntry::TypeSubtitle) {
				Name.Format(L"Subtitle %d", iSubtitle++);

				mt.SetSampleSize(1);

				if (CodecID == "S_TEXT/ASCII") {
					mt.majortype = MEDIATYPE_Text;
					mt.subtype = MEDIASUBTYPE_NULL;
					mt.formattype = FORMAT_None;
					mts.Add(mt);
					isSub = true;
				} else {
					mt.majortype = MEDIATYPE_Subtitle;
					mt.formattype = FORMAT_SubtitleInfo;
					SUBTITLEINFO* psi = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + pTE->CodecPrivate.GetCount());
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
					memcpy(mt.pbFormat + (psi->dwOffset = sizeof(SUBTITLEINFO)), pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());

					mt.subtype =
						CodecID == "S_TEXT/UTF8" ? MEDIASUBTYPE_UTF8 :
						CodecID == "S_TEXT/SSA" || CodecID == "S_SSA" ? MEDIASUBTYPE_SSA :
						CodecID == "S_TEXT/ASS" || CodecID == "S_ASS" ? MEDIASUBTYPE_ASS :
						CodecID == "S_TEXT/USF" || CodecID == "S_USF" ? MEDIASUBTYPE_USF :
						CodecID == "S_HDMV/PGS" ? MEDIASUBTYPE_HDMVSUB :
						CodecID == "S_DVBSUB" ? MEDIASUBTYPE_DVB_SUBTITLES :
						CodecID == "S_VOBSUB" ? MEDIASUBTYPE_VOBSUB :
						MEDIASUBTYPE_NULL;

					if (mt.subtype != MEDIASUBTYPE_NULL) {
						mts.Add(mt);
						isSub = true;
					}
				}
			}

			if (mts.IsEmpty()) {
				DbgLog((LOG_TRACE, 3, L"CMatroskaSplitterFilter::CreateOutputs() : Unsupported TrackType '%s' (%I64d)", CString(CodecID), (UINT64)pTE->TrackType));
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
				pinOut.InsertAt((iVideo + iAudio - 3), pPinOut, 1);
				pinOutTE.InsertAt((iVideo + iAudio - 3), pTE, 1);
			} else {
				pinOut.Add(pPinOut);
				pinOutTE.Add(pTE);
			}
		}
	}

	for (size_t i = 0; i < pinOut.GetCount(); i++) {
		CAutoPtr<CBaseSplitterOutputPin> pPinOut;
		pPinOut.Attach(pinOut[i]);
		TrackEntry* pTE = pinOutTE[i];

		if (pTE != NULL) {
			AddOutputPin((DWORD)pTE->TrackNumber, pPinOut);
			m_pTrackEntryMap[(DWORD)pTE->TrackNumber] = pTE;
			m_pOrderedTrackArray.Add(pTE);
		}
	}

	Info& info = m_pFile->m_segment.SegmentInfo;
	m_rtDuration = (REFERENCE_TIME)(info.Duration * info.TimeCodeScale / 100);

	if (m_bCalcDuration && bHasVideo && m_pFile->m_segment.Cues.GetCount()) {
		// calculate duration from video track;
		m_pSegment = Root.Child(MATROSKA_ID_SEGMENT);
		m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);
		m_pBlock.Free();

		Segment& s = m_pFile->m_segment;
		UINT64 TrackNumber = s.GetMasterTrack();

		REFERENCE_TIME rtDur = INVALID_TIME;

		POSITION pos1 = s.Cues.GetTailPosition();
		while (pos1 && rtDur == INVALID_TIME) {
			Cue* pCue = s.Cues.GetPrev(pos1);

			POSITION pos2 = pCue->CuePoints.GetTailPosition();
			while (pos2 && rtDur == INVALID_TIME) {
				CuePoint* pCuePoint = pCue->CuePoints.GetPrev(pos2);

				POSITION pos3 = pCuePoint->CueTrackPositions.GetTailPosition();
				while (pos3 && rtDur == INVALID_TIME) {
					CueTrackPosition* pCueTrackPositions = pCuePoint->CueTrackPositions.GetPrev(pos3);

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
									if (!(bg->Block.Lacing & 0x80)) {
										bg->ReferenceBlock.Set(0); // not a kf
									}
									bgn.AddTail(bg);
								}

								POSITION pos4 = bgn.GetHeadPosition();
								while (pos4) {
									BlockGroup* bg = bgn.GetNext(pos4);

									if (bg->Block.TrackNumber == pCueTrackPositions->CueTrack) {
										TrackEntry* pTE = NULL;
										if (!m_pTrackEntryMap.Lookup(TrackNumber, pTE) || !pTE) {
											continue;
										}

										REFERENCE_TIME duration = 1;
										if (bg->BlockDuration.IsValid()) {
											duration = s.GetRefTime(bg->BlockDuration);
										} else if (pTE->DefaultDuration) {
											duration = (pTE->DefaultDuration / 100) * bg->Block.BlockData.GetCount();
										}

										REFERENCE_TIME rt = s.GetRefTime(c.TimeCode + bg->Block.TimeCode) + duration;
										rtDur = max(rtDur, rt);
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
			m_rtDuration = min(m_rtDuration, rtDur);
		}
	}

	m_rtNewStop = m_rtStop = m_rtDuration;

	// fonts
	if (m_bLoadEmbeddedFonts) {
		InstallFonts();
	}

	// resources
	pos = m_pFile->m_segment.Attachments.GetHeadPosition();
	while (pos) {
		Attachment* pA = m_pFile->m_segment.Attachments.GetNext(pos);

		POSITION pos = pA->AttachedFiles.GetHeadPosition();
		while (pos) {
			AttachedFile* pF = pA->AttachedFiles.GetNext(pos);

			CAtlArray<BYTE> pData;
			pData.SetCount((size_t)pF->FileDataLen);
			m_pFile->Seek(pF->FileDataPos);
			if (SUCCEEDED(m_pFile->ByteRead(pData.GetData(), pData.GetCount()))) {
				ResAppend(pF->FileName, pF->FileDescription, CString(pF->FileMimeType), pData.GetData(), (DWORD)pData.GetCount());
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
	SetProperty(L"TITL", info.Title);
	pos = m_pFile->m_segment.Tags.GetHeadPosition();
	while (pos) {
		Tags* Tags = m_pFile->m_segment.Tags.GetNext(pos);

		POSITION pos = Tags->Tag.GetHeadPosition();
		while (pos) {
			Tag* Tag = Tags->Tag.GetNext(pos);

			POSITION pos = Tag->SimpleTag.GetHeadPosition();
			while (pos) {
				SimpleTag* SimpleTag = Tag->SimpleTag.GetNext(pos);
				if (SimpleTag->TagName == _T("TITLE")) {
					DelProperty(L"TITL");
					SetProperty(L"TITL", SimpleTag->TagString);
				} else if (SimpleTag->TagName == _T("ARTIST")) {
					DelProperty(L"AUTH");
					SetProperty(L"AUTH", SimpleTag->TagString);
				} else if (SimpleTag->TagName == _T("DATE_RELEASED")) {
					DelProperty(L"DATE");
					SetProperty(L"DATE", SimpleTag->TagString);
				} else if (SimpleTag->TagName == _T("COPYRIGHT")) {
					DelProperty(L"CPYR");
					SetProperty(L"CPYR", SimpleTag->TagString);
				} else if (SimpleTag->TagName == _T("COMMENT")) {
					DelProperty(L"DESC");
					SetProperty(L"DESC", SimpleTag->TagString);
				}
			}
		}
	}

	BSTR title, date;
	if (SUCCEEDED(GetProperty(L"TITL", &title)) & SUCCEEDED(GetProperty(L"DATE", &date))) {
		if (!CString(title).IsEmpty() && !CString(date).IsEmpty()) {
			CString Title;
			Title.Format(_T("%s (%s)"), title, date);
			SetProperty(L"TITL", Title);
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

void CMatroskaSplitterFilter::SetupChapters(LPCSTR lng, ChapterAtom* parent, int level)
{
	CString tabs('+', level);

	if (!tabs.IsEmpty()) {
		tabs += ' ';
	}

	POSITION pos = parent->ChapterAtoms.GetHeadPosition();
	while (pos) {
		// ChapUID zero not allow by Matroska specs
		UINT64 ChapUID  = parent->ChapterAtoms.GetNext(pos)->ChapterUID;
		ChapterAtom* ca = (ChapUID == 0) ? NULL : m_pFile->m_segment.FindChapterAtom(ChapUID);

		if (ca) {
			CString name, first;

			POSITION pos = ca->ChapterDisplays.GetHeadPosition();
			while (pos) {
				ChapterDisplay* cd = ca->ChapterDisplays.GetNext(pos);
				if (first.IsEmpty()) {
					first = cd->ChapString;
				}
				if (cd->ChapLanguage == lng) {
					name = cd->ChapString;
				}
			}

			name = tabs + (!name.IsEmpty() ? name : first);

			ChapAppend(ca->ChapterTimeStart / 100 - m_pFile->m_rtOffset, name);

			if (!ca->ChapterAtoms.IsEmpty() && level < 5) {
				// level < 5 - hard limit for the number of levels
				SetupChapters(lng, ca, level + 1);
			}
		}
	}
}

void CMatroskaSplitterFilter::InstallFonts()
{
	POSITION pos = m_pFile->m_segment.Attachments.GetHeadPosition();
	while (pos) {
		Attachment* pA = m_pFile->m_segment.Attachments.GetNext(pos);

		POSITION p2 = pA->AttachedFiles.GetHeadPosition();
		while (p2) {
			AttachedFile* pF = pA->AttachedFiles.GetNext(p2);

			if (pF->FileMimeType == "application/x-truetype-font" ||
					pF->FileMimeType == "application/x-font-ttf" ||
					pF->FileMimeType == "application/vnd.ms-opentype") {
				// assume this is a font resource

				if (BYTE* pData = DNew BYTE[(UINT)pF->FileDataLen]) {
					m_pFile->Seek(pF->FileDataPos);

					if (SUCCEEDED(m_pFile->ByteRead(pData, pF->FileDataLen))) {
						//m_fontinst.InstallFont(pData, (UINT)pF->FileDataLen);
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

	POSITION pos = m_pTrackEntryMap.GetStartPosition();
	while (pos) {
		DWORD TrackNumber = 0;
		TrackEntry* pTE = NULL;
		m_pTrackEntryMap.GetNextAssoc(pos, TrackNumber, pTE);

		CBaseSplitterOutputPin* pPin = GetOutputPin(TrackNumber);

		if (!(pTE && pPin && pPin->IsConnected())) {
			continue;
		}

		if (pTE->CodecID.ToString() == "A_VORBIS" && pPin->CurrentMediaType().subtype == MEDIASUBTYPE_Vorbis
				&& pTE->CodecPrivate.GetCount() > 0) {
			BYTE* ptr = pTE->CodecPrivate.GetData();

			CAtlList<int> sizes;
			long last = 0;
			for (BYTE n = *ptr++; n > 0; n--) {
				int size = 0;
				do {
					size += *ptr;
				} while (*ptr++ == 0xff);
				sizes.AddTail(size);
				last += size;
			}
			sizes.AddTail(pTE->CodecPrivate.GetCount() - (ptr - pTE->CodecPrivate.GetData()) - last);

			hr = S_OK;

			POSITION pos = sizes.GetHeadPosition();
			while (pos && SUCCEEDED(hr)) {
				long len = sizes.GetNext(pos);

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
				DbgLog((LOG_TRACE, 3, L"MatroskaSplitterFilter::SendVorbisHeaderSample() - ERROR: Vorbis initialization failed for stream %I64d", TrackNumber));
			}
		}
	}
}

bool CMatroskaSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CMatroskaSplitterFilter");

	CMatroskaNode Root(m_pFile);
	if (!m_pFile
			|| !(m_pSegment = Root.Child(MATROSKA_ID_SEGMENT))
			|| !(m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER))) {
		return false;
	}

	// reindex if needed
	if (m_pFile->IsRandomAccess() && m_pFile->m_segment.Cues.GetCount() == 0) {
		m_nOpenProgress = 0;
		m_pFile->m_segment.SegmentInfo.Duration.Set(0);

		UINT64 TrackNumber = m_pFile->m_segment.GetMasterTrack();

		CAutoPtr<Cue> pCue(DNew Cue());

		do {
			Cluster c;
			c.ParseTimeCode(m_pCluster);

			m_pFile->m_segment.SegmentInfo.Duration.Set((float)c.TimeCode - m_pFile->m_rtOffset / 10000);

			CAutoPtr<CuePoint> pCuePoint(DNew CuePoint());
			CAutoPtr<CueTrackPosition> pCueTrackPosition(DNew CueTrackPosition());
			pCuePoint->CueTime.Set(c.TimeCode);
			pCueTrackPosition->CueTrack.Set(TrackNumber);
			pCueTrackPosition->CueClusterPosition.Set(m_pCluster->m_filepos - m_pSegment->m_start);
			pCuePoint->CueTrackPositions.AddTail(pCueTrackPosition);
			pCue->CuePoints.AddTail(pCuePoint);

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

		m_nOpenProgress = 100;

		if (!m_fAbort) {
			m_pFile->m_segment.Cues.AddTail(pCue);
		}

		m_fAbort = false;

		if (m_pFile->m_segment.Cues.GetCount()) {
			Info& info		= m_pFile->m_segment.SegmentInfo;
			m_rtDuration	= (REFERENCE_TIME)(info.Duration * info.TimeCodeScale / 100);
			m_rtNewStop		= m_rtStop = m_rtDuration;
		}
	}

	m_pCluster.Free();
	m_pBlock.Free();

	POSITION pos1 = m_pFile->m_segment.Cues.GetHeadPosition();
	while (pos1) {
		Cue* pCue = m_pFile->m_segment.Cues.GetNext(pos1);

		POSITION pos2 = pCue->CuePoints.GetTailPosition();
		while (pos2) {
			CuePoint* pCuePoint = pCue->CuePoints.GetPrev(pos2);

			POSITION pos3 = pCuePoint->CueTrackPositions.GetHeadPosition();
			while (pos3) {
				CueTrackPosition* pCueTrackPositions = pCuePoint->CueTrackPositions.GetNext(pos3);

				if (pCueTrackPositions->CueDuration && pCueTrackPositions->CueRelativePosition) {
					m_bSupportCueDuration = TRUE;
					break;
				}
			}
		}
	}

	return true;
}

void CMatroskaSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);
	m_pBlock.Free();

	m_Seek_rt = INVALID_TIME;

	if (rt > 0) {
		rt += m_pFile->m_rtOffset;

		QWORD lastCueClusterPosition = ULONGLONG_MAX;

		Segment& s = m_pFile->m_segment;

		UINT64 TrackNumber = s.GetMasterTrack();

		REFERENCE_TIME seek_rt = 0;

		POSITION pos1 = s.Cues.GetHeadPosition();
		while (pos1) {
			Cue* pCue = s.Cues.GetNext(pos1);

			POSITION pos2 = pCue->CuePoints.GetTailPosition();
			while (pos2) {
				CuePoint* pCuePoint = pCue->CuePoints.GetPrev(pos2);

				if (rt < s.GetRefTime(pCuePoint->CueTime)) {
					continue;
				}

				POSITION pos3 = pCuePoint->CueTrackPositions.GetHeadPosition();
				while (pos3) {
					CueTrackPosition* pCueTrackPositions = pCuePoint->CueTrackPositions.GetNext(pos3);

					if (TrackNumber != pCueTrackPositions->CueTrack) {
						continue;
					}

					if (lastCueClusterPosition == pCueTrackPositions->CueClusterPosition) {
						continue;
					}

					lastCueClusterPosition = pCueTrackPositions->CueClusterPosition;

					m_pCluster->SeekTo(m_pSegment->m_start + pCueTrackPositions->CueClusterPosition);
					if (FAILED(m_pCluster->Parse())) {
						continue;
					}

					{
						Cluster c;
						c.ParseTimeCode(m_pCluster);
						seek_rt = s.GetRefTime(c.TimeCode);

						bool fPassedCueTime = false;
						if (CAutoPtr<CMatroskaNode> pBlock = m_pCluster->GetFirstBlock()) {

							do {
								CBlockGroupNode bgn;

								if (pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
									bgn.Parse(pBlock, true);
								} else if (pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
									CAutoPtr<BlockGroup> bg(DNew BlockGroup());
									bg->Block.Parse(pBlock, true);
									if (!(bg->Block.Lacing & 0x80)) {
										bg->ReferenceBlock.Set(0); // not a kf
									}
									bgn.AddTail(bg);
								}

								POSITION pos4 = bgn.GetHeadPosition();
								while (!fPassedCueTime && pos4) {
									BlockGroup* bg = bgn.GetNext(pos4);
									seek_rt = s.GetRefTime(c.TimeCode + bg->Block.TimeCode);

									if ((bg->Block.TrackNumber == pCueTrackPositions->CueTrack && rt < seek_rt) || (abs(seek_rt - rt) <= 5000000i64)) {
										fPassedCueTime = true;
									}
								}
							} while (!fPassedCueTime && pBlock->NextBlock());
						}

						if (fPassedCueTime && seek_rt > 0) {
							DbgLog((LOG_TRACE, 3, L"CMatroskaSplitterFilter::DemuxSeek() : Seek One - %s => %s, [%10I64d - %10I64d]", ReftimeToString(rt), ReftimeToString(seek_rt), rt, seek_rt));
							goto end;
						}
					}
				}

				Cluster c;
				c.ParseTimeCode(m_pCluster);
				REFERENCE_TIME seek_rt2 = s.GetRefTime(c.TimeCode);

				if (seek_rt2 > 0) {
					DbgLog((LOG_TRACE, 3, L"CMatroskaSplitterFilter::DemuxSeek() : Seek Two - %s => %s, [%10I64d - %10I64d]", ReftimeToString(rt), ReftimeToString(seek_rt2), rt, seek_rt2));
					goto end;
				}
			}

			if (seek_rt > 0 && seek_rt < rt) {
				DbgLog((LOG_TRACE, 3, L"CMatroskaSplitterFilter::DemuxSeek() : Seek Three - %s => %s, [%10I64d - %10I64d]", ReftimeToString(rt), ReftimeToString(seek_rt), rt, seek_rt));
				goto end;
			}
		}

		{
			// Plan B
			m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);
			seek_rt = 0;

			do {
				Cluster c;
				if (FAILED(c.ParseTimeCode(m_pCluster))) {
					continue;
				}
				REFERENCE_TIME seek_rt2 = s.GetRefTime(c.TimeCode);

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
					REFERENCE_TIME seek_rt2 = s.GetRefTime(c.TimeCode);

					if (s.GetRefTime(c.TimeCode) == seek_rt) {
						DbgLog((LOG_TRACE, 3, L"CMatroskaSplitterFilter::DemuxSeek(), plan B : %s => %s, [%10I64d - %10I64d]", ReftimeToString(rt), ReftimeToString(seek_rt), rt, seek_rt));
						goto end;
					}
				} while (m_pCluster->Next());
			}
		}

		// epic fail ...
		m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);
		DbgLog((LOG_TRACE, 3, L"CMatroskaSplitterFilter::DemuxSeek(), epic fail ... start from begin"));

	end:
		Cluster c;
		c.ParseTimeCode(m_pCluster);
		m_Seek_rt = s.GetRefTime(c.TimeCode);
		DbgLog((LOG_TRACE, 3, L"CMatroskaSplitterFilter::DemuxSeek() : Final(Cluster timecode) - %s => %s, [%10I64d - %10I64d]", ReftimeToString(rt), ReftimeToString(m_Seek_rt), rt, m_Seek_rt));
	}
}

#define SetBlockTime																				\
	REFERENCE_TIME duration = 0;																	\
	if (p->bg->BlockDuration.IsValid()) {															\
		duration = m_pFile->m_segment.GetRefTime(p->bg->BlockDuration);								\
	} else if (pTE->DefaultDuration) {																\
		duration = (pTE->DefaultDuration / 100) * p->bg->Block.BlockData.GetCount();				\
	}																								\
	if (pTE->TrackType == TrackEntry::TypeSubtitle && !duration) {									\
		duration = 1;																				\
	}																								\
	\
	p->rtStart = m_pFile->m_segment.GetRefTime((REFERENCE_TIME)c.TimeCode + p->bg->Block.TimeCode);	\
	p->rtStop = p->rtStart + duration;																\
	\
	p->rtStart -= m_pFile->m_rtOffset;																\
	p->rtStop -= m_pFile->m_rtOffset;																\
	\
	p->TrackType = pTE->TrackType;																	\

bool CMatroskaSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	SendVorbisHeaderSample(); // HACK: init vorbis decoder with the headers

	if (m_Seek_rt > 0 && m_bSupportCueDuration) {
		CAtlList<UINT64> TrackNumbers;

		Segment& s = m_pFile->m_segment;
		POSITION pos1 = s.Tracks.GetHeadPosition();
		while (pos1) {
			Track* pT = s.Tracks.GetNext(pos1);

			POSITION pos2 = pT->TrackEntries.GetHeadPosition();
			while (pos2) {
				TrackEntry* pTE = pT->TrackEntries.GetNext(pos2);

				if (pTE->TrackType == TrackEntry::TypeSubtitle) {
					TrackNumbers.AddTail(pTE->TrackNumber);
				}
			}
		}

		if (TrackNumbers.GetCount()) {
			CMatroskaNode Root(m_pFile);
			CAutoPtr<CMatroskaNode> pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);

			QWORD lastCueClusterPosition = ULONGLONG_MAX;
			BOOL bBreak = FALSE;

			if (pCluster && !bBreak) {
				pos1 = s.Cues.GetHeadPosition();
				while (pos1) {
					Cue* pCue = s.Cues.GetNext(pos1);

					POSITION pos2 = pCue->CuePoints.GetTailPosition();
					while (pos2 && !bBreak) {
						CuePoint* pCuePoint = pCue->CuePoints.GetPrev(pos2);

						REFERENCE_TIME cueTime = s.GetRefTime(pCuePoint->CueTime);
						if (cueTime > m_Seek_rt) {
							continue;
						}

						POSITION pos3 = pCuePoint->CueTrackPositions.GetHeadPosition();
						while (pos3 && !bBreak) {
							CueTrackPosition* pCueTrackPositions = pCuePoint->CueTrackPositions.GetNext(pos3);

							if (m_bSupportCueDuration && !TrackNumbers.Find(pCueTrackPositions->CueTrack)) {
								continue;
							}

							if (m_bSupportCueDuration && (!pCueTrackPositions->CueDuration || !pCueTrackPositions->CueRelativePosition)) {
								continue;
							}

							if (lastCueClusterPosition == pCueTrackPositions->CueClusterPosition) {
								continue;
							}
							lastCueClusterPosition = pCueTrackPositions->CueClusterPosition;

							if (m_bSupportCueDuration) {
								REFERENCE_TIME cueDuration = s.GetRefTime(pCueTrackPositions->CueDuration);
								if (cueTime + cueDuration > m_Seek_rt) {
									pCluster->SeekTo(m_pSegment->m_start + pCueTrackPositions->CueClusterPosition);
									if (FAILED(pCluster->Parse())) {
										continue;
									}

									QWORD pos = pCluster->GetPos();

									Cluster c;
									c.ParseTimeCode(pCluster);

									pCluster->SeekTo(pos + pCueTrackPositions->CueRelativePosition);
									CAutoPtr<CMatroskaNode> pBlock(DNew CMatroskaNode(pCluster));

									if (!pBlock) {
										continue;
									}

									CBlockGroupNode bgn;
									if (pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
										bgn.Parse(pBlock, true);
									} else if (pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
										CAutoPtr<BlockGroup> bg(DNew BlockGroup());
										bg->Block.Parse(pBlock, true);
										if (!(bg->Block.Lacing & 0x80)) {
											bg->ReferenceBlock.Set(0);    // not a kf
										}
										bgn.AddTail(bg);
									}

									while (bgn.GetCount()) {
										CAutoPtr<CMatroskaPacket> p(DNew CMatroskaPacket());
										p->bg = bgn.RemoveHead();

										if (!TrackNumbers.Find(p->bg->Block.TrackNumber)) {
											continue;
										}

										p->bSyncPoint = !p->bg->ReferenceBlock.IsValid();
										p->TrackNumber = (DWORD)p->bg->Block.TrackNumber;

										TrackEntry* pTE = NULL;
										if (!m_pTrackEntryMap.Lookup(p->TrackNumber, pTE) || !pTE) {
											continue;
										}

										SetBlockTime;

										if (p->rtStart >= m_Seek_rt || p->rtStop < m_Seek_rt) {
											continue;
										}

										POSITION pos = p->bg->Block.BlockData.GetHeadPosition();
										while (pos) {
											CBinary* pb = p->bg->Block.BlockData.GetNext(pos);
											pTE->Expand(*pb, ContentEncoding::AllFrameContents);
										}

										hr = DeliverPacket(p);
									};
								}
							} else if (cueTime < m_Seek_rt - 60 * 10000000i64) {
								bBreak = TRUE;
								break;
							} else { // look no earlier than 1 minute ...
								pCluster->SeekTo(m_pSegment->m_start + pCueTrackPositions->CueClusterPosition);
								if (FAILED(pCluster->Parse())) {
									continue;
								}

								Cluster c;
								c.ParseTimeCode(pCluster);

								CAutoPtrList<CBlockGroupNode> bgnList;
								if (CAutoPtr<CMatroskaNode> pBlock = pCluster->GetFirstBlock()) {
									do {
										CAutoPtr<CBlockGroupNode> bgn(DNew CBlockGroupNode());

										if (pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
											bgn->Parse(pBlock, true);
										} else if (pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
											CAutoPtr<BlockGroup> bg(DNew BlockGroup());
											bg->Block.Parse(pBlock, true);
											if (!(bg->Block.Lacing & 0x80)) {
												bg->ReferenceBlock.Set(0); // not a kf
											}
											bgn->AddTail(bg);
										}

										BOOL bIsValid = FALSE;
										POSITION pos = bgn->GetHeadPosition();
										while (pos) {
											BlockGroup* bg = bgn->GetNext(pos);

											if (!TrackNumbers.Find(bg->Block.TrackNumber)) {
												continue;
											}

											if (bg->BlockDuration.IsValid() && (cueTime + s.GetRefTime(bg->BlockDuration) > m_Seek_rt)) {
												bIsValid = TRUE;
												break;
											}
										}

										if (bIsValid) {
											bgnList.AddTail(bgn);
										}
									} while (pBlock->NextBlock());
								}

								if (bgnList.GetCount()) {
									while (bgnList.GetCount()) {
										CAutoPtr<CBlockGroupNode> bgn = bgnList.RemoveHead();

										while (bgn->GetCount() && SUCCEEDED(hr)) {
											CAutoPtr<CMatroskaPacket> p(DNew CMatroskaPacket());
											p->bg = bgn->RemoveHead();

											if (!TrackNumbers.Find(p->bg->Block.TrackNumber)) {
												continue;
											}

											p->bSyncPoint = !p->bg->ReferenceBlock.IsValid();
											p->TrackNumber = (DWORD)p->bg->Block.TrackNumber;

											TrackEntry* pTE = NULL;
											if (!m_pTrackEntryMap.Lookup(p->TrackNumber, pTE) || !pTE) {
												continue;
											}

											SetBlockTime;

											if (p->rtStart >= m_Seek_rt || p->rtStop < m_Seek_rt) {
												continue;
											}

											POSITION pos = p->bg->Block.BlockData.GetHeadPosition();
											while (pos) {
												CBinary* pb = p->bg->Block.BlockData.GetNext(pos);
												pTE->Expand(*pb, ContentEncoding::AllFrameContents);
											}

											hr = DeliverPacket(p);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	do {
		Cluster c;
		c.ParseTimeCode(m_pCluster);

		if (!m_pBlock) {
			m_pBlock = m_pCluster->GetFirstBlock();
		}
		if (!m_pBlock) {
			continue;
		}

		do {
			CBlockGroupNode bgn;

			if (m_pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
				bgn.Parse(m_pBlock, true);
			} else if (m_pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
				CAutoPtr<BlockGroup> bg(DNew BlockGroup());
				bg->Block.Parse(m_pBlock, true);
				if (!(bg->Block.Lacing & 0x80)) {
					bg->ReferenceBlock.Set(0);    // not a kf
				}
				bgn.AddTail(bg);
			}

			while (bgn.GetCount() && SUCCEEDED(hr)) {
				CAutoPtr<CMatroskaPacket> p(DNew CMatroskaPacket());
				p->bg = bgn.RemoveHead();

				p->bSyncPoint = !p->bg->ReferenceBlock.IsValid();
				p->TrackNumber = (DWORD)p->bg->Block.TrackNumber;

				TrackEntry* pTE = NULL;
				if (!m_pTrackEntryMap.Lookup(p->TrackNumber, pTE) || !pTE) {
					continue;
				}

				SetBlockTime;

				POSITION pos = p->bg->Block.BlockData.GetHeadPosition();
				while (pos) {
					CBinary* pb = p->bg->Block.BlockData.GetNext(pos);
					pTE->Expand(*pb, ContentEncoding::AllFrameContents);
				}

				hr = DeliverPacket(p);
			}
		} while (m_pBlock->NextBlock() && SUCCEEDED(hr) && !CheckRequest(NULL));

		m_pBlock.Free();
	} while (m_pFile->GetPos() < (__int64)(m_pFile->m_segment.pos + m_pFile->m_segment.len)
			 && m_pCluster->Next(true) && SUCCEEDED(hr) && !CheckRequest(NULL));

	m_pCluster.Free();

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CMatroskaSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);

	nKFs				= 0;
	Segment& s			= m_pFile->m_segment;
	UINT64 TrackNumber	= s.GetMasterTrack();

	POSITION pos = s.Cues.GetHeadPosition();
	while (pos) {
		Cue* pCue = s.Cues.GetNext(pos);
		if (pCue) {
			POSITION pos2 = pCue->CuePoints.GetHeadPosition();
			while (pos2) {
				CuePoint* pCuePoint = pCue->CuePoints.GetNext(pos2);
				if (pCuePoint && pCuePoint->CueTrackPositions.GetCount()) {
					CueTrackPosition* pCueTrackPositions = pCuePoint->CueTrackPositions.GetHead();
					if (pCueTrackPositions->CueTrack == TrackNumber) {
						nKFs++;
					}
				}
			}
		}
	}

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

	UINT nKFsTmp		= 0;
	Segment& s			= m_pFile->m_segment;
	UINT64 TrackNumber	= s.GetMasterTrack();

	POSITION pos = s.Cues.GetHeadPosition();
	while (pos) {
		Cue* pCue = s.Cues.GetNext(pos);
		if (pCue) {
			POSITION pos2 = pCue->CuePoints.GetHeadPosition();
			while (pos2) {
				CuePoint* pCuePoint = pCue->CuePoints.GetNext(pos2);
				if (pCuePoint && pCuePoint->CueTrackPositions.GetCount()) {
					CueTrackPosition* pCueTrackPositions = pCuePoint->CueTrackPositions.GetHead();
					if (pCueTrackPositions->CueTrack == TrackNumber) {
						pKFs[nKFsTmp++] = s.GetRefTime(pCuePoint->CueTime);
					}
				}
			}
		}
	}

	nKFs = nKFsTmp;

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
	if (aTrackIdx >= m_pOrderedTrackArray.GetCount()) {
		return NULL;
	}
	return m_pOrderedTrackArray[aTrackIdx];
}

STDMETHODIMP_(UINT) CMatroskaSplitterFilter::GetTrackCount()
{
	return (UINT)m_pTrackEntryMap.GetCount();
}

STDMETHODIMP_(BOOL) CMatroskaSplitterFilter::GetTrackInfo(UINT aTrackIdx, struct TrackElement* pStructureToFill)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == NULL) {
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
	if (pTE == NULL) {
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
	if (pTE == NULL) {
		return NULL;
	}
	return pTE->Name.AllocSysString();
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetTrackCodecID(UINT aTrackIdx)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == NULL) {
		return NULL;
	}
	return pTE->CodecID.ToString().AllocSysString();
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetTrackCodecName(UINT aTrackIdx)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == NULL) {
		return NULL;
	}
	return pTE->CodecName.AllocSysString();
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetTrackCodecInfoURL(UINT aTrackIdx)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == NULL) {
		return NULL;
	}
	return pTE->CodecInfoURL.AllocSysString();
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetTrackCodecDownloadURL(UINT aTrackIdx)
{
	TrackEntry* pTE = GetTrackEntryAt(aTrackIdx);
	if (pTE == NULL) {
		return NULL;
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

	if (*ppPage != NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMatroskaSplitterSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMatroskaSplitterSettingsWnd>(NULL, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// IMpegSplitterFilter
STDMETHODIMP CMatroskaSplitterFilter::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_MATROSKASplit)) {
		key.SetDWORDValue(OPT_LoadEmbeddedFonts, m_bLoadEmbeddedFonts);
		key.SetDWORDValue(OPT_CalcDuration, m_bCalcDuration);
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MATROSKASplit, OPT_LoadEmbeddedFonts, m_bLoadEmbeddedFonts);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_MATROSKASplit, OPT_CalcDuration, m_bCalcDuration);
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

//
// CMatroskaSplitterOutputPin
//

CMatroskaSplitterOutputPin::CMatroskaSplitterOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
}

CMatroskaSplitterOutputPin::~CMatroskaSplitterOutputPin()
{
}

HRESULT CMatroskaSplitterOutputPin::DeliverEndFlush()
{
	{
		CAutoLock cAutoLock(&m_csQueue);
		m_packets.RemoveAll();
	}

	return __super::DeliverEndFlush();
}

HRESULT CMatroskaSplitterOutputPin::DeliverEndOfStream()
{
	CAutoLock cAutoLock(&m_csQueue);

	while (m_packets.GetCount()) {
		HRESULT hr = DeliverMatroskaBlock(m_packets.RemoveHead());
		if (hr != S_OK) {
			return hr;
		}
	}

	return __super::DeliverEndOfStream();
}

HRESULT CMatroskaSplitterOutputPin::DeliverPacket(CAutoPtr<CPacket> p)
{
	CMatroskaPacket* mp = dynamic_cast<CMatroskaPacket*>(p.m_p);
	if (!mp) {
		return __super::DeliverPacket(p);
	}

	CAutoLock cAutoLock(&m_csQueue);

	CAutoPtr<CMatroskaPacket> p2;
	p.Detach();
	p2.Attach(mp);

	HRESULT hr = S_OK;
	if (p2->TrackType == TrackEntry::TypeSubtitle) {
		hr = DeliverMatroskaBlock(p2);
	} else {
		m_packets.AddTail(p2);

		if (m_packets.GetCount() == 2) {
			REFERENCE_TIME rtBlockDuration = m_packets.GetTail()->rtStart - m_packets.GetHead()->rtStart;
			hr = DeliverMatroskaBlock(m_packets.RemoveHead(), rtBlockDuration);
		}
	}

	return hr;
}

static WORD RL16(BYTE* p)
{
	return ((WORD)p[0] | (WORD)p[1] << 8);
}

static void WL16(BYTE* p, WORD d)
{
	p[0] = (d);
	p[1] = (d) >> 8;
}

static void WL32(BYTE* p, DWORD d)
{
	p[0] = (d);
	p[1] = (d) >> 8;
	p[2] = (d) >> 16;
	p[3] = (d) >> 24;
}

// reconstruct full wavpack blocks from mangled matroska ones.
// From LAV's ffmpeg
static bool ParseWavpack(CMediaType* mt, CBinary* Data, CAutoPtr<CPacket>& p)
{
	CheckPointer(mt, false);
	CheckPointer(mt->pbFormat, false);
	CheckPointer(Data, false);

	CGolombBuffer gb(Data->GetData(), Data->GetCount());

	if (gb.GetSize() < 12) {
		return false;
	}

	DWORD samples	= gb.ReadDwordLE();
	WORD ver		= 0;
	int dstlen		= 0;
	int offset		= 0;

	if (mt->formattype == FORMAT_WaveFormatEx) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt->Format();
		if (wfe->cbSize >= 2) {
			ver = RL16(mt->pbFormat);
		}
	}

	CAutoPtr<CBinary> ptr(DNew CBinary());

	while (gb.RemainingSize() >= 8) {
		DWORD flags		= gb.ReadDwordLE();
		DWORD crc		= gb.ReadDwordLE();
		DWORD blocksize	= gb.RemainingSize();

		int multiblock	= (flags & 0x1800) != 0x1800;
		if (multiblock) {
			if (gb.RemainingSize() < 4) {
				return false;
			}
			blocksize = gb.ReadDwordLE();
		}

		if (blocksize > (DWORD)gb.RemainingSize()) {
			return false;
		}

		ptr->SetCount(dstlen + blocksize + 32);
		BYTE *dst = ptr->GetData();

		dstlen += blocksize + 32;

		WL32(dst + offset, MAKEFOURCC('w', 'v', 'p', 'k'));			// tag
		WL32(dst + offset + 4,  blocksize + 24);					// blocksize - 8
		WL16(dst + offset + 8,  ver);								// version
		WL16(dst + offset + 10, 0);									// track/index_no
		WL32(dst + offset + 12, 0);									// total samples
		WL32(dst + offset + 16, 0);									// block index
		WL32(dst + offset + 20, samples);							// number of samples
		WL32(dst + offset + 24, flags);								// flags
		WL32(dst + offset + 28, crc);								// crc
		memcpy(dst + offset + 32, gb.GetBufferPos(), blocksize);	// block data

		gb.SkipBytes(blocksize);
		offset += blocksize + 32;
	}

	p->Copy(*ptr);

	return true;
}

HRESULT CMatroskaSplitterOutputPin::DeliverMatroskaBlock(CMatroskaPacket* p, REFERENCE_TIME rtBlockDuration/* = 0*/)
{
	HRESULT hr = S_FALSE;

	size_t BlockCount = p->bg->Block.BlockData.GetCount();

	REFERENCE_TIME rtStart		= p->rtStart;
	REFERENCE_TIME rtDuration	= 0;
	if (p->rtStop != p->rtStart) {
		rtDuration				= (p->rtStop - p->rtStart) / BlockCount;
	} else if (rtBlockDuration > 0) {
		rtDuration				= rtBlockDuration / BlockCount;
	} else if (m_rtLastDuration) {
		rtDuration				= m_rtLastDuration;
	}
	REFERENCE_TIME rtStop		= rtStart + rtDuration;

	m_rtLastDuration = rtDuration;

	POSITION pos = p->bg->Block.BlockData.GetHeadPosition();
	while (pos) {
		CAutoPtr<CPacket> tmp(DNew CPacket());

		tmp->TrackNumber	= p->TrackNumber;
		tmp->bDiscontinuity	= p->bDiscontinuity;
		tmp->bSyncPoint		= p->bSyncPoint;
		tmp->rtStart		= rtStart;
		tmp->rtStop			= rtStop;

		if (m_mt.subtype == MEDIASUBTYPE_DVB_SUBTITLES) {
			// Add DBV subtitle missing start code - 0x20 0x00 (in Matroska DVB packets start with 0x0F ...)
			CAutoPtr<CBinary> ptr(DNew CBinary());
			ptr->SetCount(2);
			BYTE *pData = ptr->GetData();
			pData[0] = 0x20;
			pData[1] = 0x00;
			tmp->Copy(*ptr);
			tmp->Append(*p->bg->Block.BlockData.GetNext(pos));
		} else if (m_mt.subtype == MEDIASUBTYPE_WAVPACK4) {
			if (!ParseWavpack(&m_mt, p->bg->Block.BlockData.GetNext(pos), tmp)) {
				continue;
			}
		} else if (m_mt.subtype == MEDIASUBTYPE_icpf) {
			CAutoPtr<CBinary> ptr(DNew CBinary());
			ptr->SetCount(2 * sizeof(DWORD));
			DWORD* pData = (DWORD*)ptr->GetData();
			pData[0] = p->bg->Block.BlockData.GetCount();
			pData[1] = FCC('icpf');
			tmp->Copy(*ptr);
			tmp->Append(*p->bg->Block.BlockData.GetNext(pos));
		} else {
			tmp->Copy(*p->bg->Block.BlockData.GetNext(pos));
		}

		if (S_OK != (hr = DeliverPacket(tmp))) {
			break;
		}

		rtStart += rtDuration;
		rtStop += rtDuration;

		p->bSyncPoint		= false;
		p->bDiscontinuity	= false;
	}

	if (m_mt.subtype == MEDIASUBTYPE_WAVPACK4) {
		POSITION pos = p->bg->ba.bm.GetHeadPosition();
		while (pos) {
			BlockMore* bm = p->bg->ba.bm.GetNext(pos);
			CAutoPtr<CPacket> tmp(DNew CPacket());

			tmp->TrackNumber	= p->TrackNumber;
			tmp->bDiscontinuity	= false;
			tmp->bSyncPoint		= false;
			tmp->rtStart		= p->rtStart;
			tmp->rtStop			= p->rtStop;

			if (!ParseWavpack(&m_mt, &bm->BlockAdditional, tmp)) {
				continue;
			}

			if (S_OK != (hr = DeliverPacket(tmp))) {
				break;
			}
		}
	}

	return hr;
}