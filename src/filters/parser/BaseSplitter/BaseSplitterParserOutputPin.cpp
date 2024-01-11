/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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
#include "BaseSplitterParserOutputPin.h"

#include "DSUtil/AudioParser.h"
#include "DSUtil/H264Nalu.h"
#include "DSUtil/MediaDescription.h"

#define SEQ_START_CODE     0xB3010000
#define PICTURE_START_CODE 0x00010000

#define MOVE_TO_H264_START_CODE(b, e)    while(b <= e - 4  && !((GETU32(b) == 0x01000000) || ((GETU32(b) & 0x00FFFFFF) == 0x00010000))) b++;
#define MOVE_TO_AC3_START_CODE(b, e)     while(b <= e - 8  && (GETU16(b) != AC3_SYNCWORD)) b++;
#define MOVE_TO_AAC_START_CODE(b, e)     while(b <= e - 9  && ((GETU16(b) & AAC_ADTS_SYNCWORD) != AAC_ADTS_SYNCWORD)) b++;
#define MOVE_TO_AACLATM_START_CODE(b, e) while(b <= e - 4  && ((GETU16(b) & 0xe0FF) != 0xe056)) b++;
#define MOVE_TO_DIRAC_START_CODE(b, e)   while(b <= e - 4  && (GETU32(b) != 0x44434242)) b++;
#define MOVE_TO_DTS_START_CODE(b, e)     while(b <= e - 16 && (GETU32(b) != DTS_SYNCWORD_CORE_BE) && GETU32(b) != DTS_SYNCWORD_SUBSTREAM) b++;
#define MOVE_TO_MPEG_START_CODE(b, e)    while(b <= e - 4  && !(GETU32(b) == SEQ_START_CODE || GETU32(b) == PICTURE_START_CODE)) b++;

//
// CBaseSplitterParserOutputPin
//

CBaseSplitterParserOutputPin::CBaseSplitterParserOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
}

CBaseSplitterParserOutputPin::~CBaseSplitterParserOutputPin()
{
	Flush();
}

HRESULT CBaseSplitterParserOutputPin::Flush()
{
	CAutoLock cAutoLock(this);

	m_p.reset();
	m_pl.clear();

	m_bFlushed           = true;
	m_truehd_framelength = 0;

	m_ParseContext.bFrameStartFound = false;
	m_ParseContext.state64          = 0;

	if (m_mt.majortype == MEDIATYPE_Audio && m_mt.pbFormat) {
		const WAVEFORMATEX *wfe = GetFormatHelper<WAVEFORMATEX>(&m_mt);
		m_nChannels             = wfe->nChannels;
		m_nSamplesPerSec        = wfe->nSamplesPerSec;
		m_wBitsPerSample        = wfe->wBitsPerSample;
	}

	if (m_mt.subtype == MEDIASUBTYPE_UTF8) {
		LCID lcid = 0;
		if (m_mt.pbFormat) {
			SUBTITLEINFO* psi = (SUBTITLEINFO*)m_mt.pbFormat;
			lcid = ISO6392ToLcid(psi->IsoLang);
		}

		m_teletext.Flush();
		m_teletext.SetLCID(lcid);
	}

	m_packetFlag = 0;
	m_DTSHDProfile = 0;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	Flush();

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

void CBaseSplitterParserOutputPin::InitPacket(CPacket* pSource)
{
	if (pSource) {
		m_p.reset(DNew CPacket());
		m_p->TrackNumber		= pSource->TrackNumber;
		m_p->bDiscontinuity		= pSource->bDiscontinuity;
		pSource->bDiscontinuity	= FALSE;

		m_p->bSyncPoint			= pSource->bSyncPoint;
		pSource->bSyncPoint		= FALSE;

		m_p->rtStart			= pSource->rtStart;
		pSource->rtStart		= INVALID_TIME;

		m_p->rtStop				= pSource->rtStop;
		pSource->rtStop			= INVALID_TIME;
	}
}

HRESULT CBaseSplitterParserOutputPin::DeliverParsed(const BYTE* start, const size_t size)
{
	std::unique_ptr<CPacket> p2(DNew CPacket());
	p2->TrackNumber    = m_p->TrackNumber;
	p2->bDiscontinuity = m_p->bDiscontinuity;
	p2->bSyncPoint     = m_p->bSyncPoint;
	p2->rtStart        = m_p->rtStart;
	p2->rtStop         = m_p->rtStop;
	p2->pmt            = m_p->pmt;

	m_p->bDiscontinuity = FALSE;
	m_p->bSyncPoint     = FALSE;
	m_p->rtStart        = INVALID_TIME;
	m_p->rtStop         = INVALID_TIME;
	m_p->pmt            = nullptr;

	p2->SetData(start, size);

	if (!p2->pmt && m_bFlushed) {
		p2->pmt = CreateMediaType(&m_mt);
		m_bFlushed = false;
	}

	return __super::DeliverPacket(std::move(p2));
}

void CBaseSplitterParserOutputPin::HandlePacket(std::unique_ptr<CPacket>& p)
{
	if (m_p->pmt) {
		DeleteMediaType(m_p->pmt);
	}
	if (p) {
		if (p->rtStart != INVALID_TIME) {
			m_p->rtStart = p->rtStart;
			m_p->rtStop  = p->rtStop;
			p->rtStart   = INVALID_TIME;
		}
		if (p->bDiscontinuity) {
			m_p->bDiscontinuity = p->bDiscontinuity;
			p->bDiscontinuity   = FALSE;
		}
		if (p->bSyncPoint) {
			m_p->bSyncPoint = p->bSyncPoint;
			p->bSyncPoint   = FALSE;
		}
		m_p->pmt = p->pmt;
		p->pmt   = nullptr;
	}
}

HRESULT CBaseSplitterParserOutputPin::DeliverPacket(std::unique_ptr<CPacket> p)
{
	if (p && p->pmt) {
		if (*((CMediaType*)p->pmt) != m_mt) {
			SetMediaType((CMediaType*)p->pmt);
			Flush();
		}
	}

	CAutoLock cAutoLock(this);

	if (p) {
		m_packetFlag = p->Flag;
	}

	if (m_mt.subtype == MEDIASUBTYPE_RAW_AAC1 && m_packetFlag != PACKET_AAC_RAW) {
		// AAC
		return ParseAAC(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_LATM_AAC) {
		// AAC LATM
		return ParseAACLATM(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_AVC1 || m_mt.subtype == FOURCCMap('1cva')
			   || m_mt.subtype == MEDIASUBTYPE_H264
			   || m_mt.subtype == MEDIASUBTYPE_AMVC) {
		// H.264/AVC/CVMA/CVME
		return ParseAnnexB(p, !!(m_mt.subtype != MEDIASUBTYPE_H264 && m_mt.subtype != MEDIASUBTYPE_AMVC));
	} else if (m_mt.subtype == MEDIASUBTYPE_HEVC) {
		// HEVC
		return ParseHEVC(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_VVC1) {
		// HEVC
		return ParseVVC(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_WVC1 || m_mt.subtype == FOURCCMap('1cvw')
			   || m_mt.subtype == MEDIASUBTYPE_WVC1_CYBERLINK || m_mt.subtype == MEDIASUBTYPE_WVC1_ARCSOFT) {
		// VC1
		return ParseVC1(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
		// HDMV LPCM
		return ParseHDMVLPCM(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_DOLBY_AC3) {
		// Dolby AC3 - core only
		return ParseAC3(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_DOLBY_TRUEHD) {
		// TrueHD only - skip AC3 core
		return ParseTrueHD(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_MLP) {
		// MLP
		return ParseTrueHD(p, FALSE);
	} else if (m_mt.subtype == MEDIASUBTYPE_DRAC) {
		// Dirac
		return ParseDirac(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE || m_mt.subtype == MEDIASUBTYPE_VOBSUB) {
		// DVD(VobSub) Subtitle
		return ParseVobSub(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_ADX_ADPCM) {
		// ADX ADPCM
		return ParseAdxADPCM(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_DTS || m_mt.subtype == MEDIASUBTYPE_DTS2) {
		// DTS
		return ParseDTS(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_UTF8) {
		// Teletext -> UTF8
		return ParseTeletext(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_MPEG2_VIDEO
			|| (m_mt.majortype == MEDIATYPE_Video && m_mt.subtype == MEDIASUBTYPE_MPEG1Payload)) {
		// MPEG1/2 Video
		return ParseMpegVideo(p);
	} else {
		m_p.reset();
		m_pl.clear();
	}

	return m_bEndOfStream ? S_OK : __super::DeliverPacket(std::move(p));
}

#define HandleInvalidPacket(len) if (m_p->size() < len) { return S_OK; } // Should be invalid packet

#define BEGINDATA										\
		BYTE* const base = m_p->data();					\
		BYTE* start = m_p->data();						\
		BYTE* end = start + m_p->size();				\

#define ENDDATA											\
		if (start == end) {								\
			m_p->clear();								\
		} else if (start > base) {						\
			size_t remaining = (size_t)(end - start);	\
			memmove(base, start, remaining);			\
			m_p->resize(remaining);						\
		}

HRESULT CBaseSplitterParserOutputPin::ParseAAC(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(9);

	BEGINDATA;

	for(;;) {
		MOVE_TO_AAC_START_CODE(start, end);
		if (start <= end - 9) {
			audioframe_t aframe;
			int size = ParseADTSAACHeader(start, &aframe);
			if (size == 0) {
				start++;
				continue;
			}
			if (start + size > end) {
				break;
			}

			if (start + size + 9 <= end) {
				int size2 = ParseADTSAACHeader(start + size);
				if (size2 == 0) {
					start++;
					continue;
				}
			}

			HRESULT hr = DeliverParsed(start + aframe.param1, size - aframe.param1);
			if (hr != S_OK) {
				return hr;
			}
			HandlePacket(p);

			start += size;
		} else {
			break;
		}
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseAACLATM(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(4);

	BEGINDATA;

	for(;;) {
		MOVE_TO_AACLATM_START_CODE(start, end);
		if (start <= end - 4) {
			int size = (_byteswap_ushort(GETU16(start + 1)) & 0x1FFF) + 3;
			if (start + size > end) {
				break;
			}

			if (start + size + 4 <= end) {
				if ((GETU16(start + size) & 0xE0FF) != 0xe056) {
					start++;
					continue;
				}
			}

			HRESULT hr = DeliverParsed(start, size);
			if (hr != S_OK) {
				return hr;
			}
			HandlePacket(p);

			start += size;
		} else {
			break;
		}
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseAnnexB(std::unique_ptr<CPacket>& p, bool bConvertToAVCC)
{
	BOOL bTimeStampExists = FALSE;
	if (p) {
		bTimeStampExists = (p->rtStart != INVALID_TIME);
	}

	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) {
		m_p->AppendData(*p);
	}

	if (!m_bEndOfStream) {
		HandleInvalidPacket(6);
	}

	if (m_p->size() >= 6) {
		BEGINDATA;

		MOVE_TO_H264_START_CODE(start, end);

		while (start <= end - 4) {
			BYTE* next = start + 4;

			if (m_bEndOfStream) {
				next = end;
			} else {
				MOVE_TO_H264_START_CODE(next, end);

				if (next >= end - 4) {
					break;
				}
			}

			int size = next - start;

			CH264Nalu Nalu;
			Nalu.SetBuffer(start, size);

			std::unique_ptr<CH264Packet> p2;

			while (Nalu.ReadNext()) {
				std::unique_ptr<CH264Packet> p3(DNew CH264Packet());

				if (bConvertToAVCC) {
					DWORD dwNalLength = Nalu.GetDataLength();
					dwNalLength = _byteswap_ulong(dwNalLength);
					const UINT dwSize = sizeof(dwNalLength);

					p3->resize(Nalu.GetDataLength() + dwSize);
					memcpy(p3->data(), &dwNalLength, dwSize);
					memcpy(p3->data() + dwSize, Nalu.GetDataBuffer(), Nalu.GetDataLength());
				} else {
					p3->resize(Nalu.GetLength());
					memcpy(p3->data(), Nalu.GetNALBuffer(), Nalu.GetLength());
				}

				if (p2 == nullptr) {
					p2 = std::move(p3);
				} else {
					p2->AppendData(*p3);
				}

				if (!p2->bDataExists
					&& (Nalu.GetType() == NALU_TYPE_SLICE || Nalu.GetType() == NALU_TYPE_IDR)) {
					p2->bDataExists = TRUE;
				}
			}
			start = next;

			if (!p2) {
				continue;
			}

			p2->TrackNumber     = m_p->TrackNumber;
			p2->bDiscontinuity  = m_p->bDiscontinuity;
			m_p->bDiscontinuity = FALSE;

			p2->bSyncPoint  = m_p->bSyncPoint;
			m_p->bSyncPoint = FALSE;

			p2->rtStart  = m_p->rtStart;
			m_p->rtStart = INVALID_TIME;
			p2->rtStop   = m_p->rtStop;
			m_p->rtStop  = INVALID_TIME;

			p2->pmt  = m_p->pmt;
			m_p->pmt = nullptr;

			m_pl.emplace_back(std::move(p2));

			if (m_p->pmt) {
				DeleteMediaType(m_p->pmt);
			}

			if (p) {
				if (p->rtStart != INVALID_TIME) {
					m_p->rtStart = p->rtStart;
					m_p->rtStop  = p->rtStop;
					p->rtStart   = INVALID_TIME;
				}
				if (p->bDiscontinuity) {
					m_p->bDiscontinuity = p->bDiscontinuity;
					p->bDiscontinuity   = FALSE;
				}
				if (p->bSyncPoint) {
					m_p->bSyncPoint = p->bSyncPoint;
					p->bSyncPoint   = FALSE;
				}

				m_p->pmt = p->pmt;
				p->pmt   = nullptr;
			}
		}

		ENDDATA;
	}

	if (m_bEndOfStream) {
		if (m_pl.size()) {
			BOOL bDataExists = FALSE;
			std::unique_ptr<CH264Packet> pl = std::move(m_pl.front());
			m_pl.pop_front();
			if (pl->bDataExists) {
				bDataExists = TRUE;
			}

			while (m_pl.size()) {
				std::unique_ptr<CH264Packet> p2 = std::move(m_pl.front());
				m_pl.pop_front();
				if (p2->bDataExists) {
					bDataExists = TRUE;
				}

				pl->AppendData(*p2);
			}

			if (bDataExists) {
				HRESULT hr = __super::DeliverPacket(std::move(pl));
				if (hr != S_OK) {
					return hr;
				}
			}
		}
	} else {
		REFERENCE_TIME rtStart = INVALID_TIME, rtStop = INVALID_TIME;

		for (auto it = m_pl.begin(); it != m_pl.end(); ++it) {
			if (it == m_pl.begin()) {
				continue;
			}

			CH264Packet* pPacket = (*it).get();
			const BYTE* pData = pPacket->data();

			BYTE nut = pData[3 + bConvertToAVCC] & 0x1f;
			if (nut == NALU_TYPE_AUD) {
				m_bHasAccessUnitDelimiters = true;
			}

			if (nut == NALU_TYPE_AUD || (!m_bHasAccessUnitDelimiters && (pPacket->rtStart != INVALID_TIME || !bTimeStampExists))) {
				if (pPacket->rtStart == INVALID_TIME && rtStart != INVALID_TIME) {
					pPacket->rtStart = rtStart;
					pPacket->rtStop  = rtStop;
				}
				rtStart = INVALID_TIME;
				rtStop  = INVALID_TIME;

				BOOL bDataExists = FALSE;
				for (auto it2 = m_pl.begin(); it2 != m_pl.end() && it2 != it && !bDataExists; ++it2) {
					bDataExists = (*it2)->bDataExists;
				}

				if (bDataExists) {
					std::unique_ptr<CH264Packet> packet = std::move(m_pl.front());
					m_pl.pop_front();
					while (it != m_pl.begin()) {
						std::unique_ptr<CH264Packet> p2 = std::move(m_pl.front());
						m_pl.pop_front();
						packet->AppendData(*p2);
					}

					if (!packet->pmt && m_bFlushed) {
						packet->pmt = CreateMediaType(&m_mt);
						m_bFlushed = false;
					}

					HRESULT hr = __super::DeliverPacket(std::move(packet));
					if (hr != S_OK) {
						return hr;
					}
				}
			} else if (rtStart == INVALID_TIME) {
				rtStart	= pPacket->rtStart;
				rtStop	= pPacket->rtStop;
			}
		}
	}

	return S_OK;
}

#define START_CODE    0x000001 // start code prefix, 3 bytes
#define END_NOT_FOUND (-100)
static int hevc_find_frame_end(BYTE* pData, int nSize, MpegParseContext& pc)
{
	for (int i = 0; i < nSize; i++) {
		pc.state64 = (pc.state64 << 8) | pData[i];
		if (((pc.state64 >> 3 * 8) & 0xFFFFFF) != START_CODE) {
			continue;
		}

		int nut = (pc.state64 >> (2 * 8 + 1)) & 0x3F;

		// Beginning of access unit
		if ((nut >= NALU_TYPE_HEVC_VPS && nut <= NALU_TYPE_HEVC_AUD) || nut == NALU_TYPE_HEVC_SEI_PREFIX ||
			(nut >= 41 && nut <= 44) || (nut >= 48 && nut <= 55)) {
			if (pc.bFrameStartFound) {
				pc.bFrameStartFound = 0;
				return i - 5;
			}
		} else if (nut <= NALU_TYPE_HEVC_RASL_R ||
				  (nut >= NALU_TYPE_HEVC_BLA_W_LP && nut <= NALU_TYPE_HEVC_CRA)) {
			int first_slice_segment_in_pic_flag = pData[i] >> 7;
			if (first_slice_segment_in_pic_flag) {
				if (!pc.bFrameStartFound) {
					pc.bFrameStartFound = 1;
				} else { // First slice of next frame found
					pc.bFrameStartFound = 0;
					return i - 5;
				}
			}
		}
	}

	return END_NOT_FOUND;
}

HRESULT CBaseSplitterParserOutputPin::ParseHEVC(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	int nBufferPos = m_p->size();

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(6);

	BEGINDATA;

	MOVE_TO_H264_START_CODE(start, end);
	if (start <= end - 4) {
		if (start > base) {
			m_ParseContext.state64 = 0;
			nBufferPos = std::max(0, nBufferPos - (int)(start - base));
		}

		for (;;) {
			int size = 0;
			if (m_bEndOfStream) {
				size = end - start;
			} else {
				size = hevc_find_frame_end(start + nBufferPos, end - start - nBufferPos, m_ParseContext);
				if (size == END_NOT_FOUND) {
					break;
				}

				size += nBufferPos;
			}

			HRESULT hr = DeliverParsed(start, size);
			if (hr != S_OK) {
				return hr;
			}
			HandlePacket(p);

			start += size;
			nBufferPos = 0;

			if (m_bEndOfStream) {
				break;
			}
		}
	}

	ENDDATA;

	return S_OK;
}

#define IS_H266_SLICE(nut) (nut <= NALU_TYPE_VVC_RASL || (nut >= NALU_TYPE_VVC_IDR_W_RADL && nut <= NALU_TYPE_VVC_GDR))

static int vvc_find_frame_end(BYTE* pData, int nSize, MpegParseContext& pc)
{
	for (int i = 0; i < nSize; i++) {
		pc.state64 = (pc.state64 << 8) | pData[i];
		if (((pc.state64 >> 3 * 8) & 0xFFFFFF) != START_CODE) {
			continue;
		}

		int code_len = ((pc.state64 >> 3 * 8) & 0xFFFFFFFF) == 0x01 ? 4 : 3;

		int nut = (pc.state64 >> (8 + 3)) & 0x1F;

		// 7.4.2.4.3 and 7.4.2.4.4
		if ((nut >= NALU_TYPE_VVC_OPI && nut <= NALU_TYPE_VVC_PREFIX_APS && nut != NALU_TYPE_VVC_PH) || nut == NALU_TYPE_VVC_AUD ||
			(nut == NALU_TYPE_VVC_PREFIX_SEI && !pc.bFrameStartFound) ||
			nut == NALU_TYPE_VVC_RSV_NVCL_26 || nut == NALU_TYPE_VVC_UNSPEC_28 || nut == NALU_TYPE_VVC_UNSPEC_29) {
			if (pc.bFrameStartFound) {
				pc.bFrameStartFound = 0;
				return i - (code_len + 2);
			}
		} else if (nut == NALU_TYPE_VVC_PH || IS_H266_SLICE(nut)) {
			int sh_picture_header_in_slice_header_flag = pData[i] >> 7;

			if (nut == NALU_TYPE_VVC_PH || sh_picture_header_in_slice_header_flag) {
				if (!pc.bFrameStartFound) {
					pc.bFrameStartFound = 1;
				} else { // First slice of next frame found
					pc.bFrameStartFound = 0;
					return i - (code_len + 2);
				}
			}
		}
	}

	return END_NOT_FOUND;
}

HRESULT CBaseSplitterParserOutputPin::ParseVVC(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	int nBufferPos = m_p->size();

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(6);

	BEGINDATA;

	MOVE_TO_H264_START_CODE(start, end);
	if (start <= end - 4) {
		if (start > base) {
			m_ParseContext.state64 = 0;
			nBufferPos = std::max(0, nBufferPos - (int)(start - base));
		}

		for (;;) {
			int size = 0;
			if (m_bEndOfStream) {
				size = end - start;
			} else {
				size = vvc_find_frame_end(start + nBufferPos, end - start - nBufferPos, m_ParseContext);
				if (size == END_NOT_FOUND) {
					break;
				}

				size += nBufferPos;
			}

			HRESULT hr = DeliverParsed(start, size);
			if (hr != S_OK) {
				return hr;
			}
			HandlePacket(p);

			start += size;
			nBufferPos = 0;

			if (m_bEndOfStream) {
				break;
			}
		}
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseVC1(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(5);

	BEGINDATA;

	bool bSeqFound = false;
	while (start <= end - 4) {
		if (GETU32(start) == 0x0D010000) {
			bSeqFound = true;
			break;
		} else if (GETU32(start) == 0x0F010000) {
			break;
		}
		start++;
	}

	while (start <= end - 4) {
		BYTE* next = start + 1;

		if (m_bEndOfStream) {
			next = end;
		} else {
			while (next <= end - 4) {
				if (GETU32(next) == 0x0D010000) {
					if (bSeqFound) {
						break;
					}
					bSeqFound = true;
				} else if (GETU32(next) == 0x0F010000) {
					break;
				}
				next++;
			}

			if (next >= end - 4) {
				break;
			}
		}

		int size = next - start;

		HRESULT hr = DeliverParsed(start, size);
		if (hr != S_OK) {
			return hr;
		}
		HandlePacket(p);

		start		= next;
		bSeqFound	= (GETU32(start) == 0x0D010000);
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseHDMVLPCM(std::unique_ptr<CPacket>& p)
{
	if (!p || p->size() <= 4) {
		return S_OK;
	}

	HRESULT hr = S_OK;

	if (ParseHdmvLPCMHeader(p->data())) {
		p->RemoveHead(4);
		hr = __super::DeliverPacket(std::move(p));
	}

	return hr;
}

HRESULT CBaseSplitterParserOutputPin::ParseAC3(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(8);

	BEGINDATA;

	for(;;) {
		MOVE_TO_AC3_START_CODE(start, end);
		if (start <= end - 8) {
			int size = ParseAC3Header(start);
			if (size == 0) {
				start++;
				continue;
			}
			if (start + size > end) {
				break;
			}

			if (start + size + 8 <= end) {
				int size2 = ParseAC3Header(start + size);
				if (size2 == 0) {
					start++;
					continue;
				}
			}

			HRESULT hr = DeliverParsed(start, size);
			if (hr != S_OK) {
				return hr;
			}
			HandlePacket(p);

			start += size;
		} else {
			break;
		}
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseTrueHD(std::unique_ptr<CPacket>& p, BOOL bCheckAC3/* = TRUE*/)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(16);

	BEGINDATA;

	while (start + 22 <= end) {
		audioframe_t aframe;
		int size = ParseMLPHeader(start, &aframe);
		if (size > 0) {
			// sync frame
			m_truehd_framelength = aframe.samples;
		} else if (bCheckAC3) {
			int ac3size = ParseAC3Header(start);
			if (ac3size == 0) {
				ac3size = ParseEAC3Header(start);
			}
			if (ac3size > 0) {
				if (start + ac3size > end) {
					break;
				}
				start += ac3size;
				continue; // skip ac3 frames
			}
		}

		if (size == 0 && m_truehd_framelength > 0) {
			// get not sync frame size
			size = ((start[0] << 8 | start[1]) & 0xfff) * 2;
		}

		if (size < 8) {
			start++;
			continue;
		}
		if (start + size > end) {
			break;
		}

		HRESULT hr = DeliverParsed(start, size);
		if (hr != S_OK) {
			return hr;
		}
		HandlePacket(p);

		start += size;
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseDirac(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(5);

	BEGINDATA;

	MOVE_TO_DIRAC_START_CODE(start, end);

	while (start <= end - 4) {
		BYTE* next = start + 1;

		if (m_bEndOfStream) {
			next = end;
		} else {
			MOVE_TO_DIRAC_START_CODE(next, end);

			if (next >= end - 4) {
				break;
			}
		}

		int size = next - start;

		HRESULT hr = DeliverParsed(start, size);
		if (hr != S_OK) {
			return hr;
		}
		HandlePacket(p);

		start = next;
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseVobSub(std::unique_ptr<CPacket>& p)
{
	HRESULT hr = S_OK;

	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) {
		m_p->AppendData(*p);
	}

	HandleInvalidPacket(5);

	BYTE* pData = m_p->data();
	int len = (pData[0] << 8) | pData[1];

	if (!len) {
		m_p.reset();
		return hr;
	}

	if ((len > (int)m_p->size())) {
		return hr;
	}

	std::unique_ptr<CPacket> p2(DNew CPacket());
	p2->TrackNumber		= m_p->TrackNumber;
	p2->bDiscontinuity	= m_p->bDiscontinuity;
	p2->bSyncPoint		= m_p->bSyncPoint;
	p2->rtStart			= m_p->rtStart;
	p2->rtStop			= m_p->rtStop;
	p2->pmt				= m_p->pmt;
	p2->SetData(m_p->data(), m_p->size());
	m_p.reset();

	if (!p2->pmt && m_bFlushed) {
		p2->pmt = CreateMediaType(&m_mt);
		m_bFlushed = false;
	}

	hr = __super::DeliverPacket(std::move(p2));

	return hr;
}

HRESULT CBaseSplitterParserOutputPin::ParseAdxADPCM(std::unique_ptr<CPacket>& p)
{
	HRESULT hr = S_OK;

	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->AppendData(*p);

	if (!m_adx_block_size) {
		UINT64 state	= 0;
		BYTE* buf		= m_p->data();
		for (size_t i = 0; i < m_p->size(); i++) {
			state = (state << 8) | buf[i];
			if ((state & 0xFFFF0000FFFFFF00) == 0x8000000003120400ULL) {
				int channels	= state & 0xFF;
				int headersize	= ((state >> 32) & 0xFFFF) + 4;
				if (channels > 0 && headersize >= 8) {
					m_adx_block_size = 18 * channels;

					BYTE* start	= buf + i - 7;
					int size	= headersize + m_adx_block_size;

					HRESULT hr = DeliverParsed(start, size);
					if (hr != S_OK) {
						return hr;
					}
					HandlePacket(p);

					m_p->RemoveHead(i - 7 + headersize + m_adx_block_size);
					break;
				}
			}
		}
	}

	if (!m_adx_block_size) {
		if (m_p->size() > 16) {
			size_t remove_size = m_p->size() - 16;
			m_p->RemoveHead(remove_size);
		}
		return hr;
	}

	while (m_p->size() >= (size_t)m_adx_block_size) {
		BYTE* start	= m_p->data();
		int size	= m_adx_block_size;

		HRESULT hr = DeliverParsed(start, size);
		if (hr != S_OK) {
			return hr;
		}
		HandlePacket(p);

		m_p->RemoveHead(size);
	}

	return hr;
}

HRESULT CBaseSplitterParserOutputPin::ParseDTS(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(16);

	BEGINDATA;

	for(;;) {
		MOVE_TO_DTS_START_CODE(start, end);
		if (start <= end - 16) {
			if (GETU32(start) == DTS_SYNCWORD_CORE_BE) {
				int size = ParseDTSHeader(start);
				if (size == 0) {
					start++;
					continue;
				}

				int sizehd = 0;
				if (start + size + 16 <= end) {
					sizehd = ParseDTSHDHeader(start + size);
				} else if (!m_bEndOfStream) {
					break; // need more data
				}

				if (start + size + sizehd > end) {
					break; // need more data
				}

				if (start + size + sizehd + 16 <= end) {
					if (!ParseDTSHeader(start + size + sizehd)) {
						start++;
						continue;
					}
				}

				size += sizehd;

				HRESULT hr = DeliverParsed(start, size);
				if (hr != S_OK) {
					return hr;
				}
				HandlePacket(p);

				start += size;
			} else {
				const int size = ParseDTSHDHeader(start);
				if (size == 0) {
					start++;
					continue;
				}

				if (start + size > end) {
					break;
				}

				if (!m_DTSHDProfile) {
					audioframe_t dtshdaframe;
					ParseDTSHDHeader(start, size, &dtshdaframe);
					m_DTSHDProfile = dtshdaframe.param2;
				}

				if (m_DTSHDProfile != DCA_PROFILE_EXPRESS) {
					start++;
					continue;
				}

				if (start + size + 16 <= end) {
					if (!ParseDTSHDHeader(start + size)) {
						start++;
						continue;
					}
				}

				HRESULT hr = DeliverParsed(start, size);
				if (hr != S_OK) {
					return hr;
				}
				HandlePacket(p);

				start += size;
			}
		} else {
			break;
		}
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseTeletext(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	HRESULT hr = S_OK;
	if (m_bEndOfStream) {
		m_teletext.ProcessRemainingData();
	} else {
		if (!p || p->size() <= 6) {
			return S_OK;
		}

		m_teletext.ProcessData(p->data(), (uint16_t)p->size(), p->rtStart, p->Flag);
	}

	if (m_teletext.IsOutputPresent()) {
		auto& output = m_teletext.GetOutput();
		for (const auto& tData : output) {
			const CStringA strA = WStrToUTF8(tData.str);

			std::unique_ptr<CPacket> p2(DNew CPacket());
			p2->TrackNumber = m_p->TrackNumber;
			p2->rtStart     = tData.rtStart;
			p2->rtStop      = tData.rtStop;
			p2->bSyncPoint  = TRUE;
			p2->SetData((const void *)(LPCSTR)strA, strA.GetLength());

			hr = __super::DeliverPacket(std::move(p2));
		}

		m_teletext.ClearOutput();
	}

	return hr;
}

HRESULT CBaseSplitterParserOutputPin::ParseMpegVideo(std::unique_ptr<CPacket>& p)
{
	if (!m_p) {
		InitPacket(p.get());
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->AppendData(*p);

	HandleInvalidPacket(6);

	BEGINDATA;

	MOVE_TO_MPEG_START_CODE(start, end);
	while (start <= end - 4) {
		BYTE* next = start + 1;
		if (m_bEndOfStream) {
			next = end;
		} else {
			MOVE_TO_MPEG_START_CODE(next, end);
			if (next >= end - 4) {
				break;
			}
		}

		int size = next - start;

		HRESULT hr = DeliverParsed(start, size);
		if (hr != S_OK) {
			return hr;
		}
		HandlePacket(p);

		start = next;
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::DeliverEndOfStream()
{
	m_bEndOfStream = true;
	DeliverPacket(std::unique_ptr<CPacket>());
	m_bEndOfStream = false;

	return __super::DeliverEndOfStream();
}