/*
 * (C) 2020-2026 see Authors.txt
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

#define AIFF_C_VERSION1 FCC(0xA2805140)

union chunk_t {
	struct {
		DWORD id;
		DWORD size;
	};
	BYTE data[8];
};

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
			|| (GETU32(data+8) != FCC('AIFF') && (GETU32(data+8) != FCC('AIFC')))) {
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
		case FCC('FVER'):
			if (m_pFile->ByteRead(&m_version, 4) != S_OK || m_version != AIFF_C_VERSION1) {
				return E_FAIL;
			}
			break;
		case FCC('SSND'):
			m_endpos = m_pFile->GetPos() + Chunk.size;

			if (m_pFile->ByteRead(data, 4) != S_OK) {
				return E_FAIL;
			}
			m_startpos = _byteswap_ushort(GETU32(data));

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

		m_pFile->Seek(pos + ALIGN(Chunk.size, 2));
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
	uint16_t data[9];

	if (chunk_size < 18 || m_pFile->ByteRead(&data, 18) != S_OK) {
		return E_FAIL;
	};

	m_channels = _byteswap_ushort(data[0]);
	m_bitdepth = _byteswap_ushort(data[3]);

	int16_t exp = _byteswap_ushort(data[4]) - 16383 - 63;
	uint64_t val = _byteswap_uint64(GETU64(&data[5]));
	if (exp < -63 || exp > 63) {
		DLog(L"CAIFFFile::ReadCommonTag() : exp %d is out of range", exp);
		return E_FAIL;
	}
	m_samplerate = exp >= 0 ? val << exp : (val + (1ULL << (-exp - 1))) >> -exp;

	m_block_align = m_channels * m_bitdepth / 8;
	m_nAvgBytesPerSec = m_samplerate * m_block_align;

	if (m_version == AIFF_C_VERSION1) {
		if (chunk_size < 18+4 || m_pFile->ByteRead(&data, 4) != S_OK) {
			return E_FAIL;
		};

		uint32_t fourcc = GETU32(&data);
		switch (fourcc) {
		case FCC('NONE'):
		case FCC('raw '):
		case FCC('twos'):
		case FCC('sowt'):
		case FCC('in24'):
		case FCC('in32'):
		case FCC('fl32'):
		case FCC('fl64'):
			m_subtype = GetMediaSubtypeGUID(fourcc);
			break;
		case FCC('alaw'):
			m_wFormatTag = WAVE_FORMAT_ALAW;
			m_subtype = MEDIASUBTYPE_ALAW;
			break;
		case FCC('ulaw'):
			m_wFormatTag = WAVE_FORMAT_MULAW;
			m_subtype = MEDIASUBTYPE_MULAW;
			break;
		case FCC('ima4'):
			m_subtype = MEDIASUBTYPE_IMA4;
			m_block_align = m_channels * 34;
			m_block_align_samples = 64;
			break;
		default:
			return E_FAIL;
		}
	}

	m_block_size = std::max((DWORD)m_block_align, m_nAvgBytesPerSec / 16);
	m_block_size -= m_block_size % m_block_align;

	return S_OK;
}

HRESULT CAIFFFile::ReadMetadataTag(const DWORD chunk_id, const DWORD chunk_size)
{
	CStringA value;
	if (m_pFile->ByteRead(value.GetBufferSetLength(chunk_size), chunk_size) == S_OK) {
		m_info[chunk_id] = UTF8orLocalToWStr(value);

		return S_OK;
	}

	return E_FAIL;
}
