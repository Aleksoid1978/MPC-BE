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
#include <moreuuids.h>
#include "BaseSplitterParserOutputPin.h"

#include "../../../DSUtil/AudioParser.h"
#include "../../../DSUtil/MediaDescription.h"

#define MOVE_TO_H264_START_CODE(b, e)		while(b <= e - 4  && !((GETDWORD(b) == 0x01000000) || ((GETDWORD(b) & 0x00FFFFFF) == 0x00010000))) b++; if((b <= e - 4) && GETDWORD(b) == 0x01000000) b++;
#define MOVE_TO_AC3_START_CODE(b, e)		while(b <= e - 8  && (GETWORD(b) != AC3_SYNC_WORD)) b++;
#define MOVE_TO_AAC_START_CODE(b, e)		while(b <= e - 9  && ((GETWORD(b) & 0xf0ff) != 0xf0ff)) b++;
#define MOVE_TO_AACLATM_START_CODE(b, e)	while(b <= e - 4  && ((GETWORD(b) & 0xe0FF) != 0xe056)) b++;
#define MOVE_TO_DIRAC_START_CODE(b, e)		while(b <= e - 4  && (GETDWORD(b) != 0x44434242)) b++;
#define MOVE_TO_DTS_START_CODE(b, e)		while(b <= e - 16 && (GETDWORD(b) != DTS_SYNC_WORD)) b++;

//
// CBaseSplitterParserOutputPin
//

CBaseSplitterParserOutputPin::CBaseSplitterParserOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
	, m_bHasAccessUnitDelimiters(false)
	, m_bFlushed(false)
	, m_truehd_framelength(0)
	, m_nChannels(0)
	, m_nSamplesPerSec(0)
	, m_wBitsPerSample(0)
	, m_adx_block_size(0)
	, m_bEndOfStream(false)
{
}

CBaseSplitterParserOutputPin::~CBaseSplitterParserOutputPin()
{
	Flush();
}

HRESULT CBaseSplitterParserOutputPin::Flush()
{
	CAutoLock cAutoLock(this);

	m_p.Free();
	m_pl.RemoveAll();

	m_bFlushed				= true;
	m_truehd_framelength	= 0;

	m_ParseContext.bFrameStartFound	= FALSE;
	m_ParseContext.state64			= 0;

	InitAudioParams();

	m_teletext.Flush();

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::DeliverEndFlush()
{
	CAutoLock cAutoLock(this);

	Flush();

	return __super::DeliverEndFlush();
}

void CBaseSplitterParserOutputPin::InitPacket(CPacket* pSource)
{
	if (pSource) {
		m_p.Attach(DNew CPacket());
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

void CBaseSplitterParserOutputPin::InitAudioParams()
{
	if (m_mt.pbFormat) {
		const WAVEFORMATEX *wfe	= GetFormatHelper(wfe, &m_mt);
		m_nChannels				= wfe->nChannels;
		m_nSamplesPerSec		= wfe->nSamplesPerSec;
		m_wBitsPerSample		= wfe->wBitsPerSample;
	}
}

#define HandlePacket(offset) \
	CAutoPtr<CPacket> p2(DNew CPacket());		\
	p2->TrackNumber		= m_p->TrackNumber;		\
	p2->bDiscontinuity	= m_p->bDiscontinuity;	\
	m_p->bDiscontinuity	= FALSE;				\
	\
	p2->bSyncPoint	= m_p->bSyncPoint;			\
	m_p->bSyncPoint	= FALSE;					\
	\
	p2->rtStart		= m_p->rtStart;				\
	m_p->rtStart	= INVALID_TIME;				\
	\
	p2->rtStop		= m_p->rtStop;				\
	m_p->rtStop		= INVALID_TIME;				\
	\
	p2->pmt		= m_p->pmt;						\
	m_p->pmt	= NULL;							\
	\
	p2->SetData(start + offset, size - offset);	\
	\
	if (!p2->pmt && m_bFlushed) {				\
		p2->pmt		= CreateMediaType(&m_mt);	\
		m_bFlushed	= false;					\
	}											\
	\
	HRESULT hr = __super::DeliverPacket(p2);	\
	if (hr != S_OK) {							\
		return hr;								\
	}											\
	\
	if (m_p->pmt) {								\
		DeleteMediaType(m_p->pmt);				\
	}											\
	if (p) {									\
		if (p->rtStart != INVALID_TIME) {		\
			m_p->rtStart	= p->rtStart;		\
			m_p->rtStop		= p->rtStop;		\
			p->rtStart		= INVALID_TIME;		\
		}										\
		if (p->bDiscontinuity) {				\
			m_p->bDiscontinuity	= p->bDiscontinuity;\
			p->bDiscontinuity	= FALSE;			\
		}											\
		if (p->bSyncPoint) {					\
			m_p->bSyncPoint	= p->bSyncPoint;	\
			p->bSyncPoint	= FALSE;			\
		}										\
		m_p->pmt	= p->pmt;					\
		p->pmt		= NULL;						\
	}


HRESULT CBaseSplitterParserOutputPin::DeliverPacket(CAutoPtr<CPacket> p)
{
	CAutoLock cAutoLock(this);

	if (p && p->pmt) {
		if (*((CMediaType*)p->pmt) != m_mt) {
			SetMediaType((CMediaType*)p->pmt);
			Flush();
		}
	}

	if (m_mt.majortype == MEDIATYPE_Audio && (!m_nChannels || !m_nSamplesPerSec)) {
		InitAudioParams();
	}

	if (m_mt.subtype == MEDIASUBTYPE_RAW_AAC1) {
		// AAC
		return ParseAAC(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_LATM_AAC) {
		// AAC LATM
		return ParseAACLATM(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_AVC1 || m_mt.subtype == FOURCCMap('1cva')
			   || m_mt.subtype == MEDIASUBTYPE_H264) {
		// H.264/AVC/CVMA/CVME
		return ParseAnnexB(p, !!(m_mt.subtype != MEDIASUBTYPE_H264));
	} else if (m_mt.subtype == MEDIASUBTYPE_HEVC) {
		// HEVC
		return ParseHEVC(p);
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
	} else if (m_mt.subtype == MEDIASUBTYPE_DTS) {
		// DTS
		return ParseDTS(p);
	} else if (m_mt.subtype == MEDIASUBTYPE_UTF8) {
		// Teletext -> UTF8
		return ParseTeletext(p);
	} else {
		m_p.Free();
		m_pl.RemoveAll();
	}

	return m_bEndOfStream ? S_OK : __super::DeliverPacket(p);
}

#define HandleInvalidPacket(size) if (m_p->GetCount() < size) { return S_OK; } // Should be invalid packet

#define BEGINDATA										\
		BYTE* const base = m_p->GetData();				\
		BYTE* start = m_p->GetData();					\
		BYTE* end = start + m_p->GetCount();			\

#define ENDDATA											\
		if (start == end) {								\
			m_p->RemoveAll();							\
		} else if (start > base) {						\
			size_t remaining = (size_t)(end - start);	\
			memmove(base, start, remaining);			\
			m_p->SetCount(remaining);					\
		}

HRESULT CBaseSplitterParserOutputPin::ParseAAC(CAutoPtr<CPacket> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

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
				audioframe_t aframe2;
				int size2 = ParseADTSAACHeader(start + size, &aframe2);
				if (size2 == 0) {
					start++;
					continue;
				}
			}

			HandlePacket(aframe.param1);

			start += size;
		} else {
			break;
		}
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseAACLATM(CAutoPtr<CPacket> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

	HandleInvalidPacket(4);

	BEGINDATA;

	for(;;) {
		MOVE_TO_AACLATM_START_CODE(start, end);
		if (start <= end - 4) {
			int size = (_byteswap_ushort(GETWORD(start + 1)) & 0x1FFF) + 3;
			if (start + size > end) {
				break;
			}

			if (start + size + 4 <= end) {
				if ((GETWORD(start + size) & 0xE0FF) != 0xe056) {
					start++;
					continue;
				}
			}

			HandlePacket(0);

			start += size;
		} else {
			break;
		}
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseAnnexB(CAutoPtr<CPacket> p, bool bConvertToAVCC)
{
	BOOL bTimeStampExists = FALSE;
	if (p) {
		bTimeStampExists = (p->rtStart != INVALID_TIME);
	}

	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

	HandleInvalidPacket(6);

	BEGINDATA;

	MOVE_TO_H264_START_CODE(start, end);

	while (start <= end - 4) {
		BYTE* next = start + 1;

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

		CAutoPtr<CH264Packet> p2;

		while (Nalu.ReadNext()) {
			CAutoPtr<CH264Packet> p3(DNew CH264Packet());

			if (bConvertToAVCC) {
				DWORD dwNalLength	= Nalu.GetDataLength();
				dwNalLength			= _byteswap_ulong(dwNalLength);
				const UINT dwSize	= sizeof(dwNalLength);

				p3->SetCount(Nalu.GetDataLength() + dwSize);
				memcpy(p3->GetData(), &dwNalLength, dwSize);
				memcpy(p3->GetData() + dwSize, Nalu.GetDataBuffer(), Nalu.GetDataLength());
			} else {
				static const BYTE start_code[]		= { 0, 0, 0, 1 };
				static const UINT start_code_size	= sizeof(start_code);

				p3->SetCount(Nalu.GetDataLength() + start_code_size);
				memcpy(p3->GetData(), start_code, start_code_size);
				memcpy(p3->GetData() + start_code_size, Nalu.GetDataBuffer(), Nalu.GetDataLength());
			}

			if (p2 == NULL) {
				p2 = p3;
			} else {
				p2->Append(*p3);
			}

			if (Nalu.GetType() == NALU_TYPE_SLICE
					|| Nalu.GetType() == NALU_TYPE_IDR) {
				p2->bSliceExist = TRUE;
			}
		}
		start = next;

		if (!p2) {
			continue;
		}

		p2->TrackNumber		= m_p->TrackNumber;
		p2->bDiscontinuity	= m_p->bDiscontinuity;
		m_p->bDiscontinuity	= FALSE;

		p2->bSyncPoint	= m_p->bSyncPoint;
		m_p->bSyncPoint	= FALSE;

		p2->rtStart		= m_p->rtStart;
		m_p->rtStart	= INVALID_TIME;
		p2->rtStop		= m_p->rtStop;
		m_p->rtStop		= INVALID_TIME;

		p2->pmt		= m_p->pmt;
		m_p->pmt	= NULL;

		m_pl.AddTail(p2);

		if (m_p->pmt) {
			DeleteMediaType(m_p->pmt);
		}

		if (p) {
			if (p->rtStart != INVALID_TIME) {
				m_p->rtStart	= p->rtStart;
				m_p->rtStop		= p->rtStop;
				p->rtStart		= INVALID_TIME;
			}
			if (p->bDiscontinuity) {
				m_p->bDiscontinuity	= p->bDiscontinuity;
				p->bDiscontinuity	= FALSE;
			}
			if (p->bSyncPoint) {
				m_p->bSyncPoint	= p->bSyncPoint;
				p->bSyncPoint	= FALSE;
			}

			m_p->pmt = p->pmt;
			p->pmt = NULL;
		}
	}

	ENDDATA;

	if (m_bEndOfStream) {
		if (m_pl.GetCount()) {
			BOOL bSliceExist = FALSE;
			CAutoPtr<CH264Packet> pl = m_pl.RemoveHead();
			if (pl->bSliceExist) {
				bSliceExist = TRUE;
			}

			while (m_pl.GetCount()) {
				CAutoPtr<CH264Packet> p2 = m_pl.RemoveHead();
				if (p2->bSliceExist) {
					bSliceExist = TRUE;
				}

				pl->Append(*p2);
			}

			if (bSliceExist) {
				HRESULT hr = __super::DeliverPacket(pl);
				if (hr != S_OK) {
					return hr;
				}
			}
		}
	} else {
		REFERENCE_TIME rtStart = INVALID_TIME, rtStop = INVALID_TIME;

		for (POSITION pos = m_pl.GetHeadPosition(); pos; m_pl.GetNext(pos)) {
			if (pos == m_pl.GetHeadPosition()) {
				continue;
			}

			CH264Packet* pPacket = m_pl.GetAt(pos);
			const BYTE* pData = pPacket->GetData();

			BYTE nut = pData[4] & 0x1f;
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

				BOOL bSliceExist = FALSE;
				CAutoPtr<CH264Packet> pl = m_pl.RemoveHead();
				if (pl->bSliceExist) {
					bSliceExist = TRUE;
				}

				while (pos != m_pl.GetHeadPosition()) {
					CAutoPtr<CH264Packet> p2 = m_pl.RemoveHead();
					if (p2->bSliceExist) {
						bSliceExist = TRUE;
					}

					pl->Append(*p2);
				}

				if (!pl->pmt && m_bFlushed) {
					pl->pmt = CreateMediaType(&m_mt);
					m_bFlushed = false;
				}

				if (bSliceExist) {
					HRESULT hr = __super::DeliverPacket(pl);
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

#define HEVC_NAL_RASL_R		9
#define HEVC_NAL_BLA_W_LP	16
#define HEVC_NAL_CRA_NUT	21

#define HEVC_NAL_VPS		32
#define HEVC_NAL_AUD		35
#define HEVC_NAL_SEI_PREFIX	39

#define START_CODE			0x000001	///< start_code_prefix_one_3bytes
#define END_NOT_FOUND		(-100)
static int hevc_find_frame_end(BYTE* pData, int nSize, MpegParseContext& pc)
{
	for (int i = 0; i < nSize; i++) {
		pc.state64 = (pc.state64 << 8) | pData[i];
		if (((pc.state64 >> 3 * 8) & 0xFFFFFF) != START_CODE) {
			continue;
		}

		int nut = (pc.state64 >> (2 * 8 + 1)) & 0x3F;

		// Beginning of access unit
		if ((nut >= HEVC_NAL_VPS && nut <= HEVC_NAL_AUD) || nut == HEVC_NAL_SEI_PREFIX ||
			(nut >= 41 && nut <= 44) || (nut >= 48 && nut <= 55)) {
			if (pc.bFrameStartFound) {
				pc.bFrameStartFound = 0;
				return i - 5;
			}
		} else if (nut <= HEVC_NAL_RASL_R ||
				  (nut >= HEVC_NAL_BLA_W_LP && nut <= HEVC_NAL_CRA_NUT)) {
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

HRESULT CBaseSplitterParserOutputPin::ParseHEVC(CAutoPtr<CPacket> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	int nBufferPos = m_p->GetCount();

	if (p) m_p->Append(*p);

	HandleInvalidPacket(6);

	BEGINDATA;

	int size = 0;

	MOVE_TO_H264_START_CODE(start, end);
	if (start <= end - 4) {
		if (start > m_p->GetData()) {
			m_ParseContext.state64 = 0;
			nBufferPos = max(0, nBufferPos - (start - m_p->GetData()));
		}

		for (;;) {
			if (m_bEndOfStream) {
				size = end - start;
			} else {
				size = hevc_find_frame_end(start + nBufferPos, end - start - nBufferPos, m_ParseContext);
				if (size == END_NOT_FOUND) {
					break;
				}

				size += nBufferPos;
			}

			HandlePacket(0);

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

HRESULT CBaseSplitterParserOutputPin::ParseVC1(CAutoPtr<CPacket> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

	HandleInvalidPacket(5);

	BEGINDATA;

	bool bSeqFound = false;
	while (start <= end - 4) {
		if (GETDWORD(start) == 0x0D010000) {
			bSeqFound = true;
			break;
		} else if (GETDWORD(start) == 0x0F010000) {
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
				if (GETDWORD(next) == 0x0D010000) {
					if (bSeqFound) {
						break;
					}
					bSeqFound = true;
				} else if (GETDWORD(next) == 0x0F010000) {
					break;
				}
				next++;
			}

			if (next >= end - 4) {
				break;
			}
		}

		int size = next - start;

		HandlePacket(0);

		start		= next;
		bSeqFound	= (GETDWORD(start) == 0x0D010000);
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseHDMVLPCM(CAutoPtr<CPacket> p)
{
	if (!p || p->GetCount() <= 4) {
		return S_OK;
	}

	HRESULT hr = S_OK;

	audioframe_t aframe;
	if (ParseHdmvLPCMHeader(p->GetData(), &aframe)) {
		p->RemoveAt(0, 4);
		hr = __super::DeliverPacket(p);
	}

	return hr;
}

HRESULT CBaseSplitterParserOutputPin::ParseAC3(CAutoPtr<CPacket> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

	HandleInvalidPacket(8);

	BEGINDATA;

	for(;;) {
		MOVE_TO_AC3_START_CODE(start, end);
		if (start <= end - 8) {
			audioframe_t aframe;
			int size = ParseAC3Header(start, &aframe);
			if (size == 0) {
				start++;
				continue;
			}
			if (start + size > end) {
				break;
			}

			if (start + size + 8 <= end) {
				int size2 = ParseAC3Header(start + size, &aframe);
				if (size2 == 0) {
					start++;
					continue;
				}
			}

			HandlePacket(0);

			start += size;
		} else {
			break;
		}
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseTrueHD(CAutoPtr<CPacket> p, BOOL bCheckAC3/* = TRUE*/)
{
	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

	HandleInvalidPacket(16);

	BEGINDATA;

	while (start + 16 <= end) {
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

		HandlePacket(0);

		start += size;
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseDirac(CAutoPtr<CPacket> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

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

		HandlePacket(0);

		start = next;
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseVobSub(CAutoPtr<CPacket> p)
{
	HRESULT hr = S_OK;

	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

	HandleInvalidPacket(5);

	BYTE* pData = m_p->GetData();
	int len = (pData[0] << 8) | pData[1];

	if (!len) {
		if (m_p) {
			m_p.Free();
		}
		return hr;

	}

	if ((len > (int)m_p->GetCount())) {
		return hr;
	}

	if (m_p) {
		CAutoPtr<CPacket> p2(DNew CPacket());
		p2->TrackNumber		= m_p->TrackNumber;
		p2->bDiscontinuity	= m_p->bDiscontinuity;
		p2->bSyncPoint		= m_p->bSyncPoint;
		p2->rtStart			= m_p->rtStart;
		p2->rtStop			= m_p->rtStop;
		p2->pmt				= m_p->pmt;
		p2->SetData(m_p->GetData(), m_p->GetCount());
		m_p.Free();

		if (!p2->pmt && m_bFlushed) {
			p2->pmt = CreateMediaType(&m_mt);
			m_bFlushed = false;
		}

		hr = __super::DeliverPacket(p2);
	}

	return hr;
}

HRESULT CBaseSplitterParserOutputPin::ParseAdxADPCM(CAutoPtr<CPacket> p)
{
	HRESULT hr = S_OK;

	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

	if (!m_adx_block_size) {
		UINT64 state	= 0;
		BYTE* buf		= m_p->GetData();
		for (size_t i = 0; i < m_p->GetCount(); i++) {
			state = (state << 8) | buf[i];
			if ((state & 0xFFFF0000FFFFFF00) == 0x8000000003120400ULL) {
				int channels	= state & 0xFF;
				int headersize	= ((state >> 32) & 0xFFFF) + 4;
				if (channels > 0 && headersize >= 8) {
					m_adx_block_size = 18 * channels;

					BYTE* start	= buf + i - 7;
					int size	= headersize + m_adx_block_size;
					HandlePacket(0);

					m_p->RemoveAt(0, i - 7 + headersize + m_adx_block_size);
					break;
				}
			}
		}
	}

	if (!m_adx_block_size) {
		if (m_p->GetCount() > 16) {
			size_t remove_size = m_p->GetCount() - 16;
			m_p->RemoveAt(0, remove_size);
		}
		return hr;
	}

	while (m_p->GetCount() >= (size_t)m_adx_block_size) {
		BYTE* start	= m_p->GetData();
		int size	= m_adx_block_size;

		HandlePacket(0);

		m_p->RemoveAt(0, size);
	}

	return hr;
}

HRESULT CBaseSplitterParserOutputPin::ParseDTS(CAutoPtr<CPacket> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	if (p) m_p->Append(*p);

	HandleInvalidPacket(16);

	BEGINDATA;

	for(;;) {
		MOVE_TO_DTS_START_CODE(start, end);
		if (start <= end - 16) {
			audioframe_t aframe;
			int size = ParseDTSHeader(start, &aframe);
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
				if (!ParseDTSHeader(start + size + sizehd, &aframe)) {
					start++;
					continue;
				}
			}

			size += sizehd;

			HandlePacket(0);

			start += size;
		} else {
			break;
		}
	}

	ENDDATA;

	return S_OK;
}

HRESULT CBaseSplitterParserOutputPin::ParseTeletext(CAutoPtr<CPacket> p)
{
	if (!m_p) {
		InitPacket(p);
	}

	if (!m_p) {
		return S_OK;
	}

	HRESULT hr = S_OK;
	std::vector<TeletextData> output;

	if (m_bEndOfStream) {
		if (m_teletext.ProcessRemainingData()) {
			m_teletext.GetOutput(output);
			m_teletext.EraseOutput();
		}
	} else {
		if (!p || p->GetCount() <= 6) {
			return S_OK;
		}

		m_teletext.ProcessData(p->GetData(), p->GetCount(), p->rtStart);
		if (m_teletext.IsOutputPresent()) {
			m_teletext.GetOutput(output);
			m_teletext.EraseOutput();
		}	
	}

	if (!output.empty()) {
		for (size_t i = 0; i < output.size(); i++) {
			TeletextData tData = output[i];

			CStringA strA = UTF16To8(tData.str);

			CAutoPtr<CPacket> p2(DNew CPacket());
			p2->TrackNumber	= m_p->TrackNumber;
			p2->rtStart		= tData.rtStart;
			p2->rtStop		= tData.rtStop;
			p2->bSyncPoint	= TRUE;
			p2->SetData((VOID*)(LPCSTR)strA, strA.GetLength());

			hr = __super::DeliverPacket(p2);
		}
	}

	return hr;
}

HRESULT CBaseSplitterParserOutputPin::DeliverEndOfStream()
{
	m_bEndOfStream = true;
	DeliverPacket(CAutoPtr<CPacket>());
	m_bEndOfStream = false;

	return __super::DeliverEndOfStream();
}