/*
 * (C) 2020-2024 see Authors.txt
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
#include "MPC7File.h"

#define MPC_FRAMESAMPLES 1152
#define ID3v1_TAG_SIZE    128

//
// CMPC7File
//

CMPC7File::CMPC7File()
	: CAudioFile()
{
	m_channels = 2;
	m_bitdepth = 16;
	m_subtype = MEDIASUBTYPE_MPC7;
	m_wFormatTag = 0x504d;
}

CMPC7File::~CMPC7File()
{
}

void CMPC7File::SetProperties(IBaseFilter* pBF)
{
	__super::SetProperties(pBF);
	if (!m_pAPETag && m_pID3Tag) {
		SetID3TagProperties(pBF, m_pID3Tag);
	}
}

static const int freq[] = { 44100, 48000, 37800, 32000 };
HRESULT CMPC7File::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;

	m_pFile->Seek(0);
	if (m_pFile->GetLength() <= 24) {
		return E_FAIL;
	}

	DWORD id = 0;
	if (m_pFile->ByteRead((BYTE*)&id, 4) != S_OK || (id & 0x0fffffff) != MAKEFOURCC('M','P','+',7)) {
		return E_FAIL;
	}

	if (m_pFile->ByteRead((BYTE*)&m_frames_cnt, 4) != S_OK || !m_frames_cnt) {
		return E_FAIL;
	}

	if ((uint64_t)m_frames_cnt * sizeof(mpc_frame_t) >= UINT_MAX) {
		return E_FAIL;
	}

	m_extradata.SetSize(16);
	if (m_pFile->ByteRead(m_extradata.Data(), m_extradata.Bytes()) != S_OK) {
		return E_FAIL;
	}

	const auto samplerateidx = m_extradata[2] & 3;
	m_samplerate = freq[samplerateidx];

	m_rtduration = llMulDiv(UNITS, (int64_t)m_frames_cnt * MPC_FRAMESAMPLES, m_samplerate, 0);

	m_startpos = m_pFile->GetPos();
	m_endpos = m_pFile->GetLength();

	size_t tag_size;
	if (ReadApeTag(tag_size)) {
		m_endpos -= tag_size;
	} else if (m_startpos + ID3v1_TAG_SIZE < m_endpos) {
		m_pFile->Seek(m_endpos - ID3v1_TAG_SIZE);

		if (m_pFile->BitRead(24) == 'TAG') {
			m_pID3Tag = DNew CID3Tag();
			tag_size = ID3v1_TAG_SIZE - 3;
			std::unique_ptr<BYTE[]> ptr(new(std::nothrow) BYTE[tag_size]);
			if (ptr && m_pFile->ByteRead(ptr.get(), tag_size) == S_OK) {
				m_pID3Tag->ReadTagsV1(ptr.get(), tag_size);
			}

			m_endpos -= ID3v1_TAG_SIZE;

			if (m_pID3Tag->TagItems.empty()) {
				SAFE_DELETE(m_pID3Tag);
			}
		}
	}

	m_pFile->Seek(m_startpos);
	m_frames.resize(m_frames_cnt);

	uint32_t stream_curbits = 8;
	for (auto& frame : m_frames) {
		const auto pos = m_pFile->GetPos();
		uint32_t tmp, size2, curbits;
		if (m_pFile->ByteRead((BYTE*)&tmp, 4) != S_OK) {
			return E_FAIL;
		}
		curbits = stream_curbits;
		if (curbits <= 12) {
			size2 = (tmp >> (12 - curbits)) & 0xFFFFF;
		} else {
			uint32_t tmp2;
			if (m_pFile->ByteRead((BYTE*)&tmp2, 4) != S_OK) {
				return E_FAIL;
			}
			size2 = (tmp << (curbits - 12) | tmp2 >> (44 - curbits)) & 0xFFFFF;
		}
		curbits += 20;
		m_pFile->Seek(pos);

		uint32_t size = ((size2 + curbits + 31) & ~31) >> 3;
		stream_curbits = (curbits + size2) & 0x1F;

		frame.pos = pos;
		frame.size = size + 4;
		frame.curbits = curbits;

		m_pFile->Seek(pos + size);
		if (stream_curbits) {
			m_pFile->Seek(m_pFile->GetPos() - 4);
		}
	}

	m_pFile->Seek(m_startpos);

	return S_OK;
}

REFERENCE_TIME CMPC7File::Seek(REFERENCE_TIME rt)
{
	if (rt <= UNITS) {
		m_currentframe = 0;
		m_pFile->Seek(m_frames.front().size);
		return 0;
	}

	const auto pos = llMulDiv(rt - UNITS, m_samplerate, UNITS, 0);
	m_currentframe = pos / MPC_FRAMESAMPLES;
	if (m_currentframe >= m_frames.size()) {
		m_currentframe = m_frames.size() - 1;
	}

	rt = llMulDiv(UNITS, (uint64_t)m_currentframe * MPC_FRAMESAMPLES, m_samplerate, 0);

	return rt;
}

int CMPC7File::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	if (m_currentframe >= m_frames.size()) {
		return 0;
	}

	const auto& frame = m_frames[m_currentframe];
	packet->SetCount(frame.size);
	packet->data()[0] = frame.curbits;
	if (m_currentframe == m_frames.size() - 1) {
		packet->data()[1] = 1;
	}

	m_pFile->Seek(frame.pos);
	if (m_pFile->ByteRead(packet->data() + 4, frame.size - 4) != S_OK) {
		return 0;
	}

	packet->rtStart = llMulDiv(UNITS, (uint64_t)m_currentframe * MPC_FRAMESAMPLES, m_samplerate, 0);
	m_currentframe++;
	packet->rtStop = llMulDiv(UNITS, (uint64_t)m_currentframe * MPC_FRAMESAMPLES, m_samplerate, 0);

	return frame.size;
}
