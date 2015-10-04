/*
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
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CRawVideoSplitterFilter), RawVideoSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CRawVideoSourceFilter), RawVideoSourceName, MERIT_NORMAL+1, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CRawVideoSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CRawVideoSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	CAtlList<CString> chkbytes;
	chkbytes.AddTail(_T("0,9,,595556344D50454732"));	// YUV4MPEG2
	chkbytes.AddTail(_T("0,3,,000001"));				// MPEG1/2, VC-1
	chkbytes.AddTail(_T("0,4,,00000001"));				// H.264/AVC, H.265/HEVC

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_NULL, chkbytes, NULL);

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

static const BYTE SYNC_MPEG1[4]		= {0x00, 0x00, 0x01, 0xB3};
static const BYTE SYNC_H264[4]		= {0x00, 0x00, 0x00, 0x01};
static const BYTE SYNC_VC1_F[4]		= {0x00, 0x00, 0x01, 0x0F};
static const BYTE SYNC_VC1_D[4]		= {0x00, 0x00, 0x01, 0x0D};
static const BYTE SYNC_HEVC_VPS[5]	= {0x00, 0x00, 0x00, 0x01, 0x40};
static const BYTE SYNC_HEVC_AUD[5]	= {0x00, 0x00, 0x00, 0x01, 0x46};
static const BYTE SYNC_MPEG4[4]		= {0x00, 0x00, 0x01, 0xB0};

//
// CRawVideoSplitterFilter
//

CRawVideoSplitterFilter::CRawVideoSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CRawVideoSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

STDMETHODIMP CRawVideoSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
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
		return E_ABORT; // hack
	}
	if (duration < 100 || duration > 10000) {
		return E_INVALIDARG;
	}

	m_iBufferDuration = duration;
	return S_OK;
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

HRESULT CRawVideoSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
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
	m_pFile->SetBreakHandle(GetRequestHandle());

	BYTE buf[256] = {0};
	if (FAILED(m_pFile->ByteRead(buf, 255))) {
		return E_FAIL;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	CAtlArray<CMediaType> mts;
	CMediaType mt;
	CString pName;

	if (memcmp(buf, YUV4MPEG2_, sizeof(YUV4MPEG2_)) == 0) {
		CStringA params = CStringA(buf + sizeof(YUV4MPEG2_));
		params.Truncate(params.Find(0x0A));

		int firstframepos = sizeof(YUV4MPEG2_) + params.GetLength() + 1;
		if (firstframepos + sizeof(FRAME_) > 255 || memcmp(buf + firstframepos, FRAME_, sizeof(FRAME_)) != 0) {
			return E_FAIL; // incorrect or unsuppurted YUV4MPEG2 file
		}

		int    width		= 0;
		int    height		= 0;
		int    fpsnum		= 24;
		int    fpsden		= 1;
		LONG   sar_x		= 1;
		LONG   sar_y		= 1;
		FOURCC fourcc		= FCC('I420'); // 4:2:0 - I420 by default
		FOURCC fourcc_2		= 0;
		WORD   bpp			= 12;
		DWORD  interl		= 0;

		int k;
		CAtlList<CStringA> sl;
		Explode(params, sl, 0x20);
		POSITION pos = sl.GetHeadPosition();
		while (pos) {
			CStringA& str = sl.GetNext(pos);
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
					switch (str[1]) {
					case 'p': interl = 0; break;
					case 't': interl = 1; break;
					case 'b': interl = 2; break;
					case 'm': interl = 3; break;
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
				str = str.Mid(1);
				// 8-bit
				if (str == "mono") {
					fourcc		= FCC('Y800');
					bpp			= 8;
				}
				else if (str == "420" || str == "420jpeg" || str == "420mpeg2" || str == "420paldv") {
					fourcc		= FCC('I420');
					bpp			= 12;
				}
				else if (str == "411") {
					fourcc		= FCC('Y41B');
					bpp			= 12;
				}
				else if (str == "422") {
					fourcc		= FCC('Y42B');
					bpp			= 16;
				}
				else if (str == "444") {
					fourcc		= FCC('444P'); // for libavcodec
					fourcc_2	= FCC('I444'); // for madVR
					bpp			= 24;
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
		m_AvgTimePerFrame = 10000000i64 * fpsden / fpsnum;
		m_framesize = width * height * bpp >> 3;

		mt.majortype  = MEDIATYPE_Video;
		mt.formattype = FORMAT_VIDEOINFO2;

		VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));

		memset(vih2, 0, sizeof(VIDEOINFOHEADER2));

		vih2->bmiHeader.biSize        = sizeof(vih2->bmiHeader);
		vih2->bmiHeader.biWidth       = width;
		vih2->bmiHeader.biHeight      = height;
		vih2->bmiHeader.biPlanes      = 1;
		vih2->bmiHeader.biBitCount    = bpp;
		vih2->bmiHeader.biSizeImage   = m_framesize;
		//vih2->rcSource = vih2->rcTarget = CRect(0, 0, width, height);
		//vih2->dwBitRate      = m_framesize * 8 * fpsnum / fpsden;
		vih2->AvgTimePerFrame  = m_AvgTimePerFrame;
		// always tell DirectShow it's interlaced (progressive flags set in IMediaSample struct)
		vih2->dwInterlaceFlags = AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOrWeave;

		sar_x *= width;
		sar_y *= height;
		ReduceDim(sar_x, sar_y);
		vih2->dwPictAspectRatioX = sar_x;
		vih2->dwPictAspectRatioY = sar_y;

		m_rtDuration      = (m_pFile->GetLength() - m_startpos) / (sizeof(FRAME_) + m_framesize) * 10000000i64 * fpsden / fpsnum;
		mt.SetSampleSize(m_framesize);

		vih2->bmiHeader.biCompression = fourcc;
		mt.subtype = FOURCCMap(fourcc);
		mts.Add(mt);

		if (fourcc_2) {
			vih2->bmiHeader.biCompression = fourcc_2;
			mt.subtype = FOURCCMap(fourcc_2);
			mts.Add(mt);
		}

		m_RAWType   = RAW_Y4M;
		pName = L"YUV4MPEG2 Video Output";
	}

	if (m_RAWType == RAW_NONE && memcmp(buf, SYNC_MPEG1, sizeof(SYNC_MPEG1)) == 0) {
		m_pFile->Seek(0);
		CBaseSplitterFileEx::seqhdr h;
		if (m_pFile->Read(h, min(KILOBYTE, m_pFile->GetLength()), &mt, false)) {
			mts.Add(mt);

			if (mt.subtype == MEDIASUBTYPE_MPEG1Payload) {
				m_RAWType = RAW_MPEG1;
				pName = L"MPEG1 Video Output";
			} else {
				m_RAWType = RAW_MPEG2;
				pName = L"MPEG2 Video Output";
			}

			m_AvgTimePerFrame	= h.ifps;

			REFERENCE_TIME rtStart	= INVALID_TIME;
			REFERENCE_TIME rtStop	= INVALID_TIME;

			__int64 posMin			= 0;
			__int64 posMax			= 0;
			// find start PTS
			{
				BYTE id = 0x00;
				m_pFile->Seek(0);
				while (m_pFile->GetPos() < min(MEGABYTE, m_pFile->GetLength()) && rtStart == INVALID_TIME) {
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
							rtStart	= rt;
							posMin	= pos;
						}
					}
				}
			}

			// find end PTS
			{
				BYTE id = 0x00;
				m_pFile->Seek(m_pFile->GetLength() - min(MEGABYTE, m_pFile->GetLength()));
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
				double rate = (double)(posMax - posMin) / (rtStop - rtStart);
				m_rtNewStop = m_rtStop = m_rtDuration = (REFERENCE_TIME)((double)m_pFile->GetLength() / rate);
			}
		}
	}

	if (m_RAWType == RAW_NONE && memcmp(buf, SYNC_H264, sizeof(SYNC_H264)) == 0) {
		m_pFile->Seek(0);

		CBaseSplitterFileEx::avchdr h;
		if (m_pFile->Read(h, min(MEGABYTE, m_pFile->GetLength()), &mt)) {
			mts.Add(mt);
			if (mt.subtype == MEDIASUBTYPE_H264 && SUCCEEDED(CreateAVCfromH264(&mt))) {
				mts.Add(mt);
			}

			m_RAWType = RAW_H264;
			pName = L"H.264/AVC1 Video Output";
		}
	}

	if (m_RAWType == RAW_NONE
			&& (memcmp(buf, SYNC_VC1_F, sizeof(SYNC_VC1_F)) == 0 || memcmp(buf, SYNC_VC1_D, sizeof(SYNC_VC1_D)) == 0)) {
		BYTE id = 0x00;
		m_pFile->Seek(0);
		while (m_pFile->GetPos() < min(MEGABYTE, m_pFile->GetLength()) && m_RAWType == RAW_NONE) {
			if (!m_pFile->NextMpegStartCode(id)) {
				continue;
			}

			if (id == 0x0F) {	// sequence header
				m_pFile->Seek(m_pFile->GetPos() - 4);

				CBaseSplitterFileEx::vc1hdr h;
				if (m_pFile->Read(h, min(KILOBYTE, m_pFile->GetRemaining()), &mt)) {
					mts.Add(mt);
					mt.subtype = MEDIASUBTYPE_WVC1_CYBERLINK;
					mts.Add(mt);
					mt.subtype = MEDIASUBTYPE_WVC1_ARCSOFT;
					mts.Add(mt);

					m_RAWType = RAW_VC1;
					pName = L"VC-1 Video Output";
				}
			}
		}
	}

	if (m_RAWType == RAW_NONE
			&& (memcmp(buf, SYNC_HEVC_VPS, sizeof(SYNC_HEVC_VPS)) == 0 || memcmp(buf, SYNC_HEVC_AUD, sizeof(SYNC_HEVC_AUD)) == 0)) {
		m_pFile->Seek(0);

		CBaseSplitterFileEx::hevchdr h;
		if (m_pFile->Read(h, min(MEGABYTE, m_pFile->GetLength()), &mt)) {
			mts.Add(mt);
			m_RAWType = RAW_HEVC;
			pName = L"H.265/HEVC Video Output";
		}
	}

	if (m_RAWType == RAW_NONE && memcmp(buf, SYNC_MPEG4, sizeof(SYNC_MPEG4)) == 0) {
		m_pFile->Seek(0);

		DWORD width = 0;
		DWORD height = 0;
		BYTE parx = 1;
		BYTE pary = 1;
		__int64 seqhdrsize = 0;

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

				m_pFile->BitByteAlign();
				while (m_pFile->BitRead(32, true) == 0x000001b2) {
					while (m_pFile->BitRead(24, true) != 0x000001) {
						m_pFile->BitRead(8);
					}
				}

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
				}
			} else if (id == 0xb6) {
				seqhdrsize = m_pFile->GetPos() - 4;
			}
		}

		if (width && height) {
			if (m_AvgTimePerFrame < 50000) {
				m_AvgTimePerFrame = 400000;
			};

			mt.majortype = MEDIATYPE_Video;
			mt.subtype = FOURCCMap('v4pm');
			mt.formattype = FORMAT_MPEG2Video;

			MPEG2VIDEOINFO* mvih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + seqhdrsize);
			memset(mvih, 0, mt.FormatLength());
			mvih->hdr.bmiHeader.biSize = sizeof(mvih->hdr.bmiHeader);
			mvih->hdr.bmiHeader.biWidth = width;
			mvih->hdr.bmiHeader.biHeight = height;
			mvih->hdr.bmiHeader.biCompression = 'v4pm';
			mvih->hdr.bmiHeader.biPlanes = 1;
			mvih->hdr.bmiHeader.biBitCount = 24;
			mvih->hdr.bmiHeader.biSizeImage = DIBSIZE(mvih->hdr.bmiHeader);
			mvih->hdr.AvgTimePerFrame = m_AvgTimePerFrame;
			mvih->hdr.dwPictAspectRatioX = width*parx;
			mvih->hdr.dwPictAspectRatioY = height*pary;
			mvih->cbSequenceHeader = seqhdrsize;
			m_pFile->Seek(0);
			m_pFile->ByteRead((BYTE*)mvih->dwSequenceHeader, seqhdrsize);
			mts.Add(mt);
			mt.subtype = FOURCCMap(mvih->hdr.bmiHeader.biCompression = 'V4PM');
			mts.Add(mt);

			m_startpos = seqhdrsize;

			m_RAWType = RAW_MPEG4;
			pName = L"MPEG-4 Visual Video Output";
		}
	}

	if (m_RAWType != RAW_NONE) {
		for (size_t i = 0; i < mts.GetCount(); i++) {
			VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)mts[i].Format();
			if (!vih->AvgTimePerFrame) {
				// set 25 fps as default value.
				vih->AvgTimePerFrame = 400000;
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
	if (m_RAWType == RAW_Y4M) {
		if (rt <= 0) {
			m_pFile->Seek(m_startpos);
		} else {
			__int64 framenum = rt / m_AvgTimePerFrame;
			m_pFile->Seek(m_startpos + framenum * (sizeof(FRAME_) + m_framesize));
		}
		return;
	}

	if (rt <= 0 || m_rtDuration <= 0) {
		m_pFile->Seek(0);
	} else {
		m_pFile->Seek((__int64)((double)rt / m_rtDuration * m_pFile->GetLength()));
	}
}

bool CRawVideoSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	REFERENCE_TIME rt = 0;
	CAutoPtr<CPacket> mpeg4packet;

	while (SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetRemaining()) {

		if (m_RAWType == RAW_Y4M) {
			if (m_pFile->GetRemaining() < m_framesize + (int)sizeof(FRAME_)) {
				break;
			}

			BYTE buf[sizeof(FRAME_)];
			if (m_pFile->ByteRead(buf, sizeof(FRAME_)) != S_OK || memcmp(buf, FRAME_, sizeof(FRAME_)) != 0) {
				ASSERT(0);
			}
			__int64 framenum = (m_pFile->GetPos() - m_startpos) / (sizeof(FRAME_) + m_framesize);

			CAutoPtr<CPacket> p(DNew CPacket());
			p->TrackNumber	= 0;
			p->bSyncPoint	= FALSE;

			p->SetCount(m_framesize);
			m_pFile->ByteRead(p->GetData(), m_framesize);

			p->rtStart     = framenum * m_AvgTimePerFrame;
			p->rtStop      = p->rtStart + m_AvgTimePerFrame;

			hr = DeliverPacket(p);
			continue;
		}

		if (m_RAWType == RAW_MPEG4) {
			for (int i = 0; i < 65536; ++i) { // don't call CheckRequest so often
				bool eof = !m_pFile->GetRemaining();

				if (mpeg4packet && !mpeg4packet->IsEmpty() && (m_pFile->BitRead(32, true) == 0x000001b6 || eof)) {
					hr = DeliverPacket(mpeg4packet);
				}

				if (eof) {
					break;
				}

				if (!mpeg4packet) {
					mpeg4packet.Attach(DNew CPacket());
					mpeg4packet->SetCount(0, 1024);
					mpeg4packet->TrackNumber = 0;
					mpeg4packet->rtStart = rt;
					mpeg4packet->rtStop = rt + m_AvgTimePerFrame;
					mpeg4packet->bSyncPoint = FALSE;
					rt += m_AvgTimePerFrame;
					// rt = INVALID_TIME;
				}

				BYTE b;
				m_pFile->ByteRead(&b, 1);
				mpeg4packet->Add(b);
			}
			continue;
		}


		if (size_t size = min(64 * KILOBYTE, m_pFile->GetRemaining())) {
			CAutoPtr<CPacket> p(DNew CPacket());
			p->TrackNumber = 0;
			p->bSyncPoint = FALSE;

			p->SetCount(size);
			m_pFile->ByteRead(p->GetData(), size);

			p->rtStart = INVALID_TIME;
			p->rtStop = INVALID_TIME;

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
