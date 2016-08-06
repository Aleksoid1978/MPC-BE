/*
 * (C) 2016 see Authors.txt
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
#include "../../../DSUtil/AudioParser.h"
#include "DTSHDFile.h"

const uint8_t chk_FILEINFO[8] = {'F', 'I', 'L', 'E', 'I', 'N', 'F', 'O'};
const uint8_t chk_CORESSMD[8] = {'C', 'O', 'R', 'E', 'S', 'S', 'M', 'D'};
const uint8_t chk_EXTSS_MD[8] = {'E', 'X', 'T', 'S', 'S', '_', 'M', 'D'};
const uint8_t chk_AUPR_HDR[8] = {'A', 'U', 'P', 'R', '-', 'H', 'D', 'R'};
const uint8_t chk_AUPRINFO[8] = {'A', 'U', 'P', 'R', 'I', 'N', 'F', 'O'};
const uint8_t chk_NAVI_TBL[8] = {'N', 'A', 'V', 'I', '-', 'T', 'B', 'L'};
const uint8_t chk_BITSHVTB[8] = {'B', 'I', 'T', 'S', 'H', 'V', 'T', 'B'};
const uint8_t chk_STRMDATA[8] = {'S', 'T', 'R', 'M', 'D', 'A', 'T', 'A'};
const uint8_t chk_TIMECODE[8] = {'T', 'I', 'M', 'E', 'C', 'O', 'D', 'E'};
const uint8_t chk_BUILDVER[8] = {'B', 'U', 'I', 'L', 'D', 'V', 'E', 'R'};
const uint8_t chk_BLACKOUT[8] = {'B', 'L', 'A', 'C', 'K', 'O', 'U', 'T'};
const uint8_t chk_BRANCHPT[8] = {'B', 'R', 'A', 'N', 'C', 'H', 'P', 'T'};

#pragma pack(push, 1)
typedef union {
		struct {
			uint8_t id[8];
			uint64_t size;
		};
		BYTE data[16];
	} chunk_t;
#pragma pack(pop)

//
// DTSHDFile
//

CDTSHDFile::CDTSHDFile()
	: CAudioFile()
	, m_length(0)
{
	m_subtype = MEDIASUBTYPE_DTS2;
	m_wFormatTag = WAVE_FORMAT_DTS2;
}

HRESULT CDTSHDFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;
	m_startpos = 0;

	m_pFile->Seek(0);
	chunk_t chunk;

	if (m_pFile->ByteRead(chunk.data, sizeof(chunk)) != S_OK
			|| memcmp(chunk.id, chk_DTSHDHDR, 8) != 0) {
		return E_FAIL;
	}
	chunk.size = _byteswap_uint64(chunk.size);

	__int64 end = m_pFile->GetLength();

	m_pFile->Seek(sizeof(chunk.data) + chunk.size);

	while (m_pFile->ByteRead(chunk.data, sizeof(chunk)) == S_OK && m_pFile->GetPos() < end) {
		chunk.size = _byteswap_uint64(chunk.size);
		__int64 pos = m_pFile->GetPos();


		if (memcmp(chunk.id, chk_AUPR_HDR, 8) == 0) {
			if (chunk.size < 21) {
				DLog("CCDTSHDFile::Open() : broken 'AUPR_HDR' chunk");
				return E_FAIL;
			}
			m_pFile->Skip(3);
			uint32_t MaxSampleRateHz = m_pFile->BitRead(24);
			uint32_t NumFramesTotal = m_pFile->BitRead(32);
			uint32_t SamplesPerFrameAtMaxFs = m_pFile->BitRead(16);
			m_pFile->Skip(5);
			uint32_t ChannelMask = m_pFile->BitRead(16);
			uint32_t CodecDelayAtMaxFs = m_pFile->BitRead(16);

			if (MaxSampleRateHz && ChannelMask && SamplesPerFrameAtMaxFs) {
				m_samplerate = MaxSampleRateHz;
				m_bitdepth = 16;
				m_channels = CountBits((ChannelMask & 0xffff) | ((ChannelMask & 0xae66) << 16));
				m_layout = 0; // not used for MEDIASUBTYPE_DTS/MEDIASUBTYPE_DTS2
				m_rtduration = FractionScale64(UNITS * NumFramesTotal, SamplesPerFrameAtMaxFs, MaxSampleRateHz);
			}
			else {
				return E_FAIL;
			}

		}
		else if (memcmp(chunk.id, chk_STRMDATA, 8) == 0) {
			m_startpos = pos;
			m_length = chunk.size;
			m_endpos = pos + chunk.size;

			if (!m_pFile->IsRandomAccess()) {
				break;
			}
		}
		else if (memcmp(chunk.id, chk_FILEINFO, 8) == 0) {
			if (chunk.size < 32 * KILOBYTE) { // set limit for 'FILEINFO' chunk
				CStringA info;
				if (S_OK == m_pFile->ByteRead((BYTE*)info.GetBufferSetLength(chunk.size), chunk.size)) {
					DLog("CCDTSHDFile::Open() : 'FILEINFO' = %s", info);
				}
			}
		}
		else if (memcmp(chunk.id, chk_CORESSMD, 8) != 0
				&& memcmp(chunk.id, chk_EXTSS_MD, 8) != 0
				&& memcmp(chunk.id, chk_AUPRINFO, 8) != 0
				&& memcmp(chunk.id, chk_NAVI_TBL, 8) != 0
				&& memcmp(chunk.id, chk_BITSHVTB, 8) != 0
				&& memcmp(chunk.id, chk_TIMECODE, 8) != 0
				&& memcmp(chunk.id, chk_BUILDVER, 8) != 0
				&& memcmp(chunk.id, chk_BLACKOUT, 8) != 0
				&& memcmp(chunk.id, chk_BRANCHPT, 8) != 0) {
			DLog("CCDTSHDFile::Open() : bad or unknown chunk id.");
			ASSERT(0);
			if (m_length) {
				break;
			} else {
				return E_FAIL;
			}
		}

		m_pFile->Seek(pos + chunk.size);
	}

	if (!m_startpos) {
		return E_FAIL;
	}

	if (m_endpos > end) {
		// truncated file?
		m_rtduration = SCALE64(m_rtduration, end - m_startpos, m_length);
		m_endpos = end;
		m_length = end - m_startpos;
	}

	return S_OK;
}

REFERENCE_TIME CDTSHDFile::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_pFile->Seek(m_startpos);
		return 0;
	}

	__int64 len = SCALE64(m_length, rt , m_rtduration);
	m_pFile->Seek(m_startpos + len);

	return rt;
}

int CDTSHDFile::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	if (m_pFile->GetPos() >= m_endpos) {
		return 0;
	}

	int size = (int)min(1024, m_endpos - m_pFile->GetPos());

	if (!packet->SetCount(size) || m_pFile->ByteRead(packet->GetData(), size) != S_OK) {
		return 0;
	}

	__int64 len = m_pFile->GetPos() - m_startpos;
	packet->rtStart = SCALE64(m_rtduration, len, m_length);
	packet->rtStop = SCALE64(m_rtduration, (len + size), m_length);

	return size;
}
