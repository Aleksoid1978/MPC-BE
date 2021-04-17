/*
 * (C) 2020-2021 see Authors.txt
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
#include "AIFFFile.h"

#pragma pack(push, 1)
typedef union {
	struct {
		DWORD id;
		DWORD size;
	};
	BYTE data[8];
} chunk_t;
#pragma pack(pop)

//
// CAIFFFile
//

CAIFFFile::CAIFFFile()
	: CAudioFile()
{
	m_subtype = MEDIASUBTYPE_PCM_TWOS;
}

void CAIFFFile::SetProperties(IBaseFilter* pBF)
{
	if (!m_info.empty()) {
		if (CComQIPtr<IDSMPropertyBag> pPB = pBF) {
			for (const auto& [tag, value] : m_info) {
				switch (tag) {
				case FCC('NAME'):
					pPB->SetProperty(L"TITL", value);
					break;
				case FCC('AUTH'):
					pPB->SetProperty(L"AUTH", value);
					break;
				case FCC('(c) '):
					pPB->SetProperty(L"CPYR", value);
					break;
				case FCC('ANNO'):
					pPB->SetProperty(L"DESC", value);
					break;
				}
			}
		}
	}

	__super::SetProperties(pBF);
}

HRESULT CAIFFFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;
	m_startpos = 0;

	m_pFile->Seek(0);
	BYTE data[12];
	if (m_pFile->ByteRead(data, sizeof(data)) != S_OK
			|| GETU32(data+0) != FCC('FORM')
			|| GETU32(data+8) != FCC('AIFF')) {
		return E_FAIL;
	}

	__int64 end = _byteswap_ulong(GETU32(data + 4));
	end -= 4;
	end = std::min(end, m_pFile->GetLength() - 12);

	chunk_t Chunk;

	while (m_pFile->ByteRead(Chunk.data, sizeof(Chunk)) == S_OK && m_pFile->GetPos() < end) {
		const auto pos = m_pFile->GetPos();
		Chunk.size = _byteswap_ulong(Chunk.size);

		DLog(L"CAIFFFile::Open() : found '%c%c%c%c' chunk, size - %lu",
			WCHAR((Chunk.id >> 0)  & 0xff),
			WCHAR((Chunk.id >> 8)  & 0xff),
			WCHAR((Chunk.id >> 16) & 0xff),
			WCHAR((Chunk.id >> 24) & 0xff),
			Chunk.size);

		switch (Chunk.id) {
		case FCC('COMM'):
			if (FAILED(ReadCommonTag(Chunk.size))) {
				return E_FAIL;
			}
			break;
		case FCC('SSND'):
			m_endpos = m_pFile->GetPos() + Chunk.size;

			m_startpos = m_pFile->BitRead(32);
			m_pFile->Skip(4);
			m_startpos += m_pFile->GetPos();
			break;
		case FCC('(c) '):
		case FCC('ANNO'):
		case FCC('AUTH'):
		case FCC('NAME'):
			ReadMetadataTag(Chunk.id, Chunk.size);
			break;
		case FCC('ID3 '):
			ReadID3Tag(Chunk.size);
			break;
		}

		m_pFile->Seek(pos + Chunk.size);
	}

	m_pFile->Seek(m_startpos);

	m_length = m_endpos - m_startpos;
	m_rtduration = SCALE64(UNITS, m_length, m_nAvgBytesPerSec);

	return (m_startpos && m_samplerate) ? S_OK : E_FAIL;
}

REFERENCE_TIME CAIFFFile::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_pFile->Seek(m_startpos);
		return 0;
	}

	auto len = SCALE64(m_length, rt , m_rtduration);
	len -= len % m_block_align;
	m_pFile->Seek(m_startpos + len);

	rt = SCALE64(UNITS, len, m_nAvgBytesPerSec);

	return rt;
}

int CAIFFFile::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	const __int64 start = m_pFile->GetPos();

	if (start + m_block_align > m_endpos) {
		return 0;
	}

	const auto size = std::min((__int64)m_block_size, m_endpos - start);
	if (!packet->SetCount(size) || m_pFile->ByteRead(packet->data(), size) != S_OK) {
		return 0;
	}

	packet->rtStart = rtStart;
	packet->rtStop  = rtStart + SCALE64(m_rtduration, size, m_length);

	return size;
}

HRESULT CAIFFFile::ReadCommonTag(const DWORD chunk_size)
{
	m_channels = m_pFile->BitRead(16);
	m_pFile->Skip(4);
	m_bitdepth = m_pFile->BitRead(16);

	int16_t exp = m_pFile->BitRead(16) - 16383 - 63;
	uint64_t val = m_pFile->BitRead(64);
	if (exp < -63 || exp > 63) {
		DLog(L"CAIFFFile::ReadCommonTag() : exp %d is out of range", exp);
		return E_FAIL;
	}
	m_samplerate = exp >= 0 ? val << exp : (val + (1ULL << (-exp - 1))) >> -exp;

	m_block_align = m_channels * m_bitdepth / 8;
	m_nAvgBytesPerSec = m_samplerate * m_block_align;

	m_block_size = std::max((DWORD)m_block_align, m_nAvgBytesPerSec / 16);
	m_block_size -= m_block_size % m_block_align;

	return S_OK;
}

HRESULT CAIFFFile::ReadMetadataTag(const DWORD chunk_id, const DWORD chunk_size)
{
	CStringA value;
	if (m_pFile->ByteRead((BYTE*)value.GetBufferSetLength(chunk_size), chunk_size) == S_OK) {
		m_info[chunk_id] = UTF8orLocalToWStr(value);

		return S_OK;
	}

	return E_FAIL;
}
