/*
 * (C) 2006-2018 see Authors.txt
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
#include "RawVideoSplitter.h"
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Video},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CRawVideoSplitterFilter), RawVideoSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CRawVideoSourceFilter), RawVideoSourceName, MERIT_NORMAL+1, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CRawVideoSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CRawVideoSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	const std::list<CString> chkbytes = {
		L"0,9,,595556344D50454732",      // YUV4MPEG2
		L"0,6,,444B49460000",            // 'DKIF\0\0'
		L"0,3,,000001",                  // MPEG1/2, VC-1
		L"0,4,,00000001",                // H.264/AVC, H.265/HEVC
		L"0,4,,434D5331,20,4,,50445652", // 'CMS1................PDVR'
		L"0,5,,3236344456",              // '264DV'
		L"0,4,,44484156",                // 'DHAV'
		L"0,4,,FFFFFF88",
	};

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_NULL, chkbytes, nullptr);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_MPEG1Video);
	UnRegisterSourceFilter(MEDIASUBTYPE_NULL);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

static const BYTE YUV4MPEG2_[10]	= {'Y', 'U', 'V', '4', 'M', 'P', 'E', 'G', '2', 0x20};
static const BYTE FRAME_[6]			= {'F', 'R', 'A', 'M', 'E', 0x0A};

static const BYTE SHORT_START_CODE[3] = {0x00, 0x00, 0x01};
static const BYTE LONG_START_CODE[4]  = {0x00, 0x00, 0x00, 0x01};

//
// CRawVideoSplitterFilter
//

CRawVideoSplitterFilter::CRawVideoSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"CRawVideoSplitterFilter", pUnk, phr, __uuidof(this))
{
}

STDMETHODIMP CRawVideoSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IExFilterInfo)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CRawVideoSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, RawVideoSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

STDMETHODIMP CRawVideoSplitterFilter::SetBufferDuration(int duration)
{
	if (m_RAWType == RAW_MPEG1 || m_RAWType == RAW_MPEG2 || m_RAWType == RAW_H264 || m_RAWType == RAW_VC1 || m_RAWType == RAW_HEVC) {
		return E_ABORT; // hack. hard-coded in CreateOutputs()
	}
	if (duration < BUFFER_DURATION_MIN || duration > BUFFER_DURATION_MAX) {
		return E_INVALIDARG;
	}
	if (duration > 5000) {
		// limit the maximum buffer, because RAW video has very large bitrates, and for single video streams there is no sense in a long queue.
		duration = 5000;
	}

	m_iBufferDuration = duration;
	return S_OK;
}

// IExFilterInfo

STDMETHODIMP CRawVideoSplitterFilter::GetInt(LPCSTR field, int *value)
{
	CheckPointer(value, E_POINTER);

	if (!strcmp(field, "VIDEO_INTERLACED")) {
		switch (y4m_interl) {
		case 'p': *value = 0; break;
		case 't': *value = 1; break;
		case 'b': *value = 2; break;
		default: return E_ABORT;
		}
		return S_OK;
	}

	return E_INVALIDARG;
}

bool CRawVideoSplitterFilter::ReadGOP(REFERENCE_TIME& rt)
{
	m_pFile->BitRead(1);
	BYTE hour		= m_pFile->BitRead(5);
	BYTE minute		= m_pFile->BitRead(6);
	m_pFile->BitRead(1);
	BYTE second		= m_pFile->BitRead(6);
	BYTE frame		= m_pFile->BitRead(6);
	BYTE closedGOP	= m_pFile->BitRead(1);
	BYTE brokenGOP	= m_pFile->BitRead(1);
	if (brokenGOP || !frame) {
		return false;
	}

	rt = (((hour * 60) + minute) * 60 + second) * UNITS;

	return true;
}

#define COMPARE(V) (memcmp(buf, V, sizeof(V)) == 0)
HRESULT CRawVideoSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;
	m_pFile.Free();

	m_pFile.Attach(DNew CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE | FM_FILE_DL | FM_STREAM));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	BYTE buf[256] = {0};
	if (FAILED(m_pFile->ByteRead(buf, 255))) {
		return E_FAIL;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	std::vector<CMediaType> mts;
	CMediaType mt;
	CString pName;

	if (COMPARE(YUV4MPEG2_)) {
		CStringA params = CStringA(buf + sizeof(YUV4MPEG2_));
		params.Truncate(params.Find(0x0A));

		int firstframepos = sizeof(YUV4MPEG2_) + params.GetLength() + 1;
		if (firstframepos + sizeof(FRAME_) > 255 || memcmp(buf + firstframepos, FRAME_, sizeof(FRAME_)) != 0) {
			return E_FAIL; // incorrect or unsuppurted YUV4MPEG2 file
		}

		int    width       = 0;
		int    height      = 0;
		int    fpsnum      = 24;
		int    fpsden      = 1;
		LONG   sar_x       = 1;
		LONG   sar_y       = 1;
		FOURCC fourcc      = FCC('I420'); // 4:2:0 - I420 by default
		FOURCC fourcc_2    = 0;
		WORD   bpp         = 12;
		DWORD  interlFlags = 0;

		int k;
		std::list<CStringA> sl;
		Explode(params, sl, 0x20);
		for (const auto& str : sl) {
			if (str.GetLength() < 2) {
				continue;
			}

			switch (str[0]) {
			case 'W':
				width = atoi(str.Mid(1));
				break;
			case 'H':
				height = atoi(str.Mid(1));
				break;
			case 'F':
				k = str.Find(':');
				fpsnum = atoi(str.Mid(1, k - 1));
				fpsden = atoi(str.Mid(k + 1));
				break;
			case 'I':
				if (str.GetLength() == 2) {
					y4m_interl = str[1];
					switch (y4m_interl) {
					default:
						y4m_interl = 'p';
						DLog(L"YUV4MPEG2: incorrect interlace flag, output as progressive");
					case 'p': // progressive
						interlFlags = 0;
						break;
					case 't': // top field first
						interlFlags = AMINTERLACE_IsInterlaced|AMINTERLACE_Field1First|AMINTERLACE_DisplayModeBobOrWeave;
						break;
					case 'b': // bottom field first
						interlFlags = AMINTERLACE_IsInterlaced|AMINTERLACE_DisplayModeBobOrWeave;
						break;
					case 'm': // mixed modes (detailed in FRAME headers, but not yet supported)
						// tell DirectShow it's interlaced, actual flags for frames set in IMediaSample
						interlFlags = AMINTERLACE_IsInterlaced|AMINTERLACE_DisplayModeBobOrWeave;;
						break;
					}
				}
				break;
			case 'A':
				k = str.Find(':');
				sar_x = atoi(str.Mid(1, k - 1));
				sar_y = atoi(str.Mid(k + 1));
				if (sar_x <= 0 || sar_y <= 0) { // if 'A0:0' = unknown or bad value
					sar_x = sar_y = 1; // then force 'A1:1' = square pixels
				}
				break;
			case 'C':
				CStringA cs = str.Mid(1);
				// 8-bit
				if (cs == "mono") {
					fourcc		= FCC('Y800');
					bpp			= 8;
				}
				else if (cs == "420" || cs == "420jpeg" || cs == "420mpeg2" || cs == "420paldv") {
					fourcc		= FCC('I420');
					bpp			= 12;
				}
				else if (cs == "411") {
					fourcc		= FCC('Y41B');
					bpp			= 12;
				}
				else if (cs == "422") {
					fourcc		= FCC('Y42B');
					bpp			= 16;
				}
				else if (cs == "444") {
					fourcc		= FCC('444P'); // for libavcodec
					fourcc_2	= FCC('I444'); // for madVR
					bpp			= 24;
				}
				// 10-bit
				else if (cs == "420p10") {
					fourcc		= MAKEFOURCC('Y', '3', 11 , 10 );
					bpp			= 24;
					mt.subtype	= MEDIASUBTYPE_LAV_RAWVIDEO;
				}
				else { // unsuppurted colour space
					fourcc		= 0;
					bpp			= 0;
				}
				break;
			//case 'X':
			//	break;
			}
		}

		if (width <= 0 || height <= 0 || fpsnum <= 0 || fpsden <= 0 || fourcc == 0) {
			return E_FAIL; // incorrect or unsuppurted YUV4MPEG2 file
		}

		m_startpos  = firstframepos;
		m_AvgTimePerFrame = UNITS * fpsden / fpsnum;
		m_framesize = width * height * bpp >> 3;

		mt.majortype  = MEDIATYPE_Video;
		mt.formattype = FORMAT_VIDEOINFO2;

		VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));

		memset(vih2, 0, sizeof(VIDEOINFOHEADER2));

		vih2->bmiHeader.biSize      = sizeof(vih2->bmiHeader);
		vih2->bmiHeader.biWidth     = width;
		vih2->bmiHeader.biHeight    = height;
		vih2->bmiHeader.biPlanes    = 1;
		vih2->bmiHeader.biBitCount  = bpp;
		vih2->bmiHeader.biSizeImage = m_framesize;
		//vih2->rcSource = vih2->rcTarget = CRect(0, 0, width, height);
		//vih2->dwBitRate = m_framesize * 8 * fpsnum / fpsden;
		vih2->AvgTimePerFrame       = m_AvgTimePerFrame;
		vih2->dwInterlaceFlags      = interlFlags;

		sar_x *= width;
		sar_y *= height;
		ReduceDim(sar_x, sar_y);
		vih2->dwPictAspectRatioX = sar_x;
		vih2->dwPictAspectRatioY = sar_y;

		if (!m_pFile->IsStreaming()) {
			__int64 num_frames = (m_pFile->GetLength() - m_startpos) / (sizeof(FRAME_) + m_framesize);
			m_rtDuration = FractionScale64(UNITS * num_frames, fpsden, fpsnum);
			m_rtNewStop = m_rtStop = m_rtDuration;
		}
		mt.SetSampleSize(m_framesize);

		vih2->bmiHeader.biCompression = fourcc;
		if (mt.subtype != MEDIASUBTYPE_LAV_RAWVIDEO) {
			mt.subtype = FOURCCMap(fourcc);
		}
		mts.push_back(mt);

		if (fourcc_2) {
			vih2->bmiHeader.biCompression = fourcc_2;
			mt.subtype = FOURCCMap(fourcc_2);
			mts.push_back(mt);
		}

		m_RAWType   = RAW_Y4M;
		pName = L"YUV4MPEG2 Video Output";
	}

	if (m_RAWType == RAW_NONE) {
		// https://wiki.multimedia.cx/index.php/IVF
		if (GETU32(buf) == FCC('DKIF')) {
			if (GETU16(buf + 4) != 0 || GETU16(buf + 6) < 32) {
				return E_FAIL; // incorrect or unsuppurted IVF file
			}

			m_startpos = GETU16(buf + 6);
			DWORD fourcc = GETU32(buf + 8);
			int width  = GETU16(buf + 12);
			int height = GETU16(buf + 14);
			unsigned fpsnum = GETU32(buf + 16);
			unsigned fpsden = GETU32(buf + 20);
			unsigned num_frames = GETU32(buf + 24);

			if (width <= 0 || height <= 0 || !fpsnum || !fpsden) {
				return E_FAIL; // incorrect IVF file
			}
			DLogIf(!num_frames, L"IVF: unknown number of frames");

			m_AvgTimePerFrame = UNITS * fpsden / fpsnum;
			m_rtDuration = FractionScale64(UNITS * num_frames, fpsden, fpsnum);

			mt.majortype = MEDIATYPE_Video;
			mt.formattype = FORMAT_VIDEOINFO2;
			mt.subtype = FOURCCMap(fourcc);

			VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
			memset(vih2, 0, sizeof(VIDEOINFOHEADER2));

			vih2->bmiHeader.biSize = sizeof(vih2->bmiHeader);
			vih2->bmiHeader.biWidth = width;
			vih2->bmiHeader.biHeight = height;
			vih2->bmiHeader.biPlanes = 1;
			vih2->bmiHeader.biBitCount = 12;
			vih2->bmiHeader.biCompression = fourcc;
			vih2->bmiHeader.biSizeImage = width * height * 12 / 8;
			vih2->AvgTimePerFrame = m_AvgTimePerFrame;
			mts.push_back(mt);

			m_RAWType = RAW_IVF;
		}
	}

	if (m_RAWType == RAW_NONE) {
		// chinese DVRs with H.264 RAW
		int maxpos = 0;
		DWORD value;

		// Polyvision PVDR ?
		if (GETU32(buf) == FCC('CMS1') && GETU32(buf + 20) == FCC('PDVR')) {
			m_pFile->Seek(24);
			if (S_OK != m_pFile->ByteRead((BYTE*)&value, 4)) {
				return E_FAIL;
			}
			m_pFile->Skip(value);
			maxpos = 1536;
		}
		// LTV-DVR ?
		else if ((GETU32(buf) == 0x88FFFFFF && GETU32(buf + 12) == GETU32(buf + 20))
				|| GETU32(buf) == FCC('DHAV')) {
			m_pFile->Seek(28);
			maxpos = 512;
		}
		// ?
		else if (GETU32(buf) == FCC('264D') && buf[4] == 'V') {
			m_pFile->Seek(0x400);
			maxpos = 2 * KILOBYTE;
		}

		if (maxpos) {
			// simple search first AnnexB Nal
			// TODO: to alter in accordance with the specification
			__int64 pos = m_pFile->GetPos();
			while (pos < maxpos && S_OK == m_pFile->ByteRead((BYTE*)&value, 4)) {
				if ((value & 0x00FFFFFF) == 0x00010000) {
					m_pFile->Seek(pos);
					if (S_OK != m_pFile->ByteRead(buf, 255)) {
						return E_FAIL;
					}
					m_startpos = pos;
					break;
				}
				m_pFile->Seek(++pos);
			}
		}
	}

	if (m_RAWType == RAW_NONE && COMPARE(SHORT_START_CODE)) {
		// MPEG-PS probe
		m_pFile->Seek(0);
		int cnt = 0, limit = 5;
		int ps = 0;

		m_pFile->Seek(0);
		BYTE id = 0x00;
		while (cnt < limit && m_pFile->GetPos() < std::min(64I64 * KILOBYTE, m_pFile->GetLength())) {
			if (!m_pFile->NextMpegStartCode(id)) {
				continue;
			}
			if (id == 0xba) { // program stream header
				const BYTE b = (BYTE)m_pFile->BitRead(8, true);
				if (((b & 0xf1) == 0x21) || ((b & 0xc4) == 0x44)) {
					ps++;
				}
			} else if (id == 0xbb) { // program stream system header
				const WORD len = (WORD)m_pFile->BitRead(16);
				m_pFile->Seek(m_pFile->GetPos() + len);
				if (m_pFile->BitRead(24, true) == 0x000001) {
					cnt++;
				}
			} else if ((id >= 0xbd && id < 0xf0) || (id == 0xfd)) { // pes packet
				const WORD len = (WORD)m_pFile->BitRead(16);
				m_pFile->Seek(m_pFile->GetPos() + len);
				if (m_pFile->BitRead(24, true) == 0x000001) {
					cnt++;
				}
			}
		}

		if (ps > 0 && cnt == limit) {
			return E_FAIL;
		}
	}

	if (m_RAWType == RAW_NONE && COMPARE(SHORT_START_CODE)) {
		m_pFile->Seek(0);
		CBaseSplitterFileEx::seqhdr h;
		if (m_pFile->Read(h, (int)std::min((__int64)KILOBYTE, m_pFile->GetLength()), &mt, false)) {
			mts.push_back(mt);

			if (mt.subtype == MEDIASUBTYPE_MPEG1Payload) {
				m_RAWType = RAW_MPEG1;
				pName = L"MPEG1 Video Output";
			} else {
				m_RAWType = RAW_MPEG2;
				pName = L"MPEG2 Video Output";
			}

			m_AvgTimePerFrame = h.hdr.ifps;
			if (m_pFile->IsRandomAccess()) {
				REFERENCE_TIME rtStart = INVALID_TIME;
				REFERENCE_TIME rtStop  = INVALID_TIME;

				__int64 posMin = 0;
				__int64 posMax = 0;
				// find start PTS
				{
					BYTE id = 0x00;
					m_pFile->Seek(0);
					while (m_pFile->GetPos() < std::min((__int64)MEGABYTE, m_pFile->GetLength()) && rtStart == INVALID_TIME) {
						if (!m_pFile->NextMpegStartCode(id)) {
							continue;
						}

						if (id == 0xb8) {	// GOP
							REFERENCE_TIME rt = 0;
							__int64 pos = m_pFile->GetPos();
							if (!ReadGOP(rt)) {
								continue;
							}

							if (rtStart == INVALID_TIME) {
								rtStart = rt;
								posMin  = pos;
							}
						}
					}
				}

				// find end PTS
				{
					BYTE id = 0x00;
					m_pFile->Seek(m_pFile->GetLength() - std::min((__int64)MEGABYTE, m_pFile->GetLength()));
					while (m_pFile->GetPos() < m_pFile->GetLength()) {
						if (!m_pFile->NextMpegStartCode(id)) {
							continue;
						}

						if (id == 0xb8) {	// GOP
							__int64 pos = m_pFile->GetPos();
							if (ReadGOP(rtStop)) {
								posMax = pos;
							}
						}
					}
				}

				if (rtStart != INVALID_TIME && rtStop != INVALID_TIME && rtStop > rtStart) {
					const double rate = (double)(posMax - posMin) / (rtStop - rtStart);
					m_rtNewStop = m_rtStop = m_rtDuration = (REFERENCE_TIME)((double)m_pFile->GetLength() / rate);
				}
			}
		}
	}

	if (m_RAWType == RAW_NONE && COMPARE(SHORT_START_CODE)) {
		BYTE id = 0x00;
		m_pFile->Seek(0);
		while (m_pFile->GetPos() < std::min((__int64)MEGABYTE, m_pFile->GetLength()) && m_RAWType == RAW_NONE) {
			if (!m_pFile->NextMpegStartCode(id)) {
				continue;
			}

			if (id == 0x0F) {	// sequence header
				m_pFile->Seek(m_pFile->GetPos() - 4);

				CBaseSplitterFileEx::vc1hdr h;
				if (m_pFile->Read(h, (int)std::min((__int64)KILOBYTE, m_pFile->GetRemaining()), &mt)) {
					mts.push_back(mt);
					mt.subtype = MEDIASUBTYPE_WVC1_CYBERLINK;
					mts.push_back(mt);
					mt.subtype = MEDIASUBTYPE_WVC1_ARCSOFT;
					mts.push_back(mt);

					m_RAWType = RAW_VC1;
					pName = L"VC-1 Video Output";
				}
			}
		}
	}

	if (m_RAWType == RAW_NONE && COMPARE(SHORT_START_CODE)) {
		m_pFile->Seek(0);

		DWORD width = 0;
		DWORD height = 0;
		BYTE parx = 1;
		BYTE pary = 1;
		__int64 extrasize = 0;

		BYTE id;
		while (m_pFile->NextMpegStartCode(id, 1024 - m_pFile->GetPos())) {
			if (id == 0x00) {
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
						m_AvgTimePerFrame = UNITS * fixed_vop_time_increment / vop_time_increment_resolution;
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
			} else if (id == 0xb6) {
				extrasize = m_pFile->GetPos() - 4;
			}
		}

		if (width && height) {
			if (m_AvgTimePerFrame < 50000) {
				m_AvgTimePerFrame = 400000;
			};

			mt.SetSampleSize(0);
			mt.SetTemporalCompression(TRUE);
			mt.majortype = MEDIATYPE_Video;
			mt.subtype = MEDIASUBTYPE_FMP4;
			mt.formattype = FORMAT_VIDEOINFO2;

			VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2) + extrasize);
			memset(vih2, 0, mt.FormatLength());
			vih2->bmiHeader.biSize = sizeof(vih2->bmiHeader) + extrasize;
			vih2->bmiHeader.biWidth = width;
			vih2->bmiHeader.biHeight = height;
			vih2->bmiHeader.biPlanes = 1;
			vih2->bmiHeader.biBitCount = 12;
			vih2->bmiHeader.biCompression = FCC('FMP4');
			vih2->bmiHeader.biSizeImage = width * height * 12 / 8;
			vih2->AvgTimePerFrame = m_AvgTimePerFrame;
			vih2->dwInterlaceFlags = 0;
			vih2->rcSource = vih2->rcTarget = CRect(0, 0, width, height);
			long par_x = width * parx;
			long par_y = height * pary;
			ReduceDim(par_x, par_y);
			vih2->dwPictAspectRatioX = par_x;
			vih2->dwPictAspectRatioY = par_y;
			m_pFile->Seek(0);
			m_pFile->ByteRead((BYTE*)(vih2 + 1), extrasize);

			mts.push_back(mt);

			m_startpos = 0;

			m_RAWType = RAW_MPEG4;
			pName = L"MPEG-4 Visual Video Output";
		}
	}

	if (m_RAWType == RAW_NONE
			&& (COMPARE(SHORT_START_CODE) || COMPARE(LONG_START_CODE))) {
		m_pFile->Seek(m_startpos);

		CBaseSplitterFileEx::avchdr h;
		if (m_pFile->Read(h, (int)std::min((__int64)MEGABYTE, m_pFile->GetLength()), &mt)) {
			mts.push_back(mt);
			if (mt.subtype == MEDIASUBTYPE_H264 && SUCCEEDED(CreateAVCfromH264(&mt))) {
				mts.push_back(mt);
			}

			m_RAWType = RAW_H264;
			pName = L"H.264/AVC1 Video Output";
		}
	}

	if (m_RAWType == RAW_NONE
			&& (COMPARE(SHORT_START_CODE) || COMPARE(LONG_START_CODE))) {
		m_pFile->Seek(0);

		CBaseSplitterFileEx::hevchdr h;
		if (m_pFile->Read(h, (int)std::min((__int64)MEGABYTE, m_pFile->GetLength()), &mt)) {
			mts.push_back(mt);
			m_RAWType = RAW_HEVC;
			pName = L"H.265/HEVC Video Output";
		}
	}

	if (m_RAWType != RAW_NONE) {
		for (size_t i = 0; i < mts.size(); i++) {
			VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2*)mts[i].Format();
			if (!vih2->AvgTimePerFrame) {
				// set 25 fps as default value.
				vih2->AvgTimePerFrame = 400000;
			}
		}
		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterParserOutputPin(mts, pName, this, this, &hr));
		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));
	}

	if (m_RAWType == RAW_MPEG1 || m_RAWType == RAW_MPEG2 || m_RAWType == RAW_H264 || m_RAWType == RAW_VC1 || m_RAWType == RAW_HEVC) {
		m_iBufferDuration = 200; // hack. equivalent to 240 packets
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CRawVideoSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CRawVideoSplitterFilter");
	if (!m_pFile) {
		return false;
	}

	m_pFile->Seek(0);

	return true;
}

void CRawVideoSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (rt <= 0 || m_rtDuration <= 0) {
		m_pFile->Seek(m_startpos);
		return;
	}

	if (m_RAWType == RAW_Y4M) {
		const __int64 framenum = rt / m_AvgTimePerFrame;;
		m_pFile->Seek(m_startpos + framenum * (sizeof(FRAME_) + m_framesize));
		return;
	}

	if (m_RAWType == RAW_IVF) {
		m_pFile->Seek(m_startpos);
		return;
	}

	m_pFile->Seek((__int64)((double)rt / m_rtDuration * m_pFile->GetLength()));
}

bool CRawVideoSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	REFERENCE_TIME rt = 0;
	CAutoPtr<CPacket> mpeg4packet;

	while (SUCCEEDED(hr) && !CheckRequest(nullptr) && m_pFile->GetRemaining()) {

		if (m_RAWType == RAW_Y4M) {
			static BYTE buf[sizeof(FRAME_)];
			if ((hr = m_pFile->ByteRead(buf, sizeof(FRAME_))) != S_OK || !COMPARE(FRAME_)) {
				break;
			}
			const __int64 framenum = (m_pFile->GetPos() - m_startpos) / (sizeof(FRAME_) + m_framesize);

			CAutoPtr<CPacket> p(DNew CPacket());
			p->rtStart = framenum * m_AvgTimePerFrame;
			p->rtStop  = p->rtStart + m_AvgTimePerFrame;

			p->resize(m_framesize);
			if ((hr = m_pFile->ByteRead(p->data(), m_framesize)) != S_OK) {
				break;
			}

			hr = DeliverPacket(p);
			continue;
		}

		if (m_RAWType == RAW_IVF) {
			BYTE header[12];
			if ((hr = m_pFile->ByteRead(header, sizeof(header))) != S_OK) {
				break;
			}

			const int framesize = GETU32(header);
			const __int64 framenum = GETU64(header + 4);

			CAutoPtr<CPacket> p(DNew CPacket());
			p->rtStart = framenum * m_AvgTimePerFrame;
			p->rtStop = p->rtStart + m_AvgTimePerFrame;

			p->resize(framesize);
			if ((hr = m_pFile->ByteRead(p->data(), framesize)) != S_OK) {
				break;
			}

			hr = DeliverPacket(p);
			continue;
		}

		if (m_RAWType == RAW_MPEG4) {
			for (int i = 0; i < 65536; ++i) { // don't call CheckRequest so often
				const bool eof = !m_pFile->GetRemaining();

				if (mpeg4packet && !mpeg4packet->empty() && (m_pFile->BitRead(32, true) == 0x000001b6 || eof)) {
					hr = DeliverPacket(mpeg4packet);
				}

				if (eof) {
					break;
				}

				if (!mpeg4packet) {
					mpeg4packet.Attach(DNew CPacket());
					mpeg4packet->clear();
					mpeg4packet->rtStart = rt;
					mpeg4packet->rtStop  = rt + m_AvgTimePerFrame;
					rt += m_AvgTimePerFrame;
					// rt = INVALID_TIME;
				}

				BYTE b;
				m_pFile->ByteRead(&b, 1);
				mpeg4packet->push_back(b);
			}
			continue;
		}


		if (const size_t size = std::min(64LL * KILOBYTE, m_pFile->GetRemaining())) {
			CAutoPtr<CPacket> p(DNew CPacket());
			p->resize(size);
			if ((hr = m_pFile->ByteRead(p->data(), size)) != S_OK) {
				break;
			}

			hr = DeliverPacket(p);
		}
	}

	return true;
}

//
// CRawVideoSourceFilter
//

CRawVideoSourceFilter::CRawVideoSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CRawVideoSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
