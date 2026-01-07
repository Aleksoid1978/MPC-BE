/*
 * (C) 2026 see Authors.txt
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
#include "RF64File.h"

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
// Wave64File
//

CRF64File::CRF64File()
	: CWAVFile()
{
}

HRESULT CRF64File::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;
	m_startpos = 0;

	m_pFile->Seek(0);
	BYTE data[48];
	if (m_pFile->ByteRead(data, sizeof(data)) != S_OK
			|| memcmp(data, RF64_startbytes, sizeof(RF64_startbytes)) != 0) {
		return E_FAIL;
	}

	uint32_t ds64_size    = GETU32(data + 16);
	uint64_t riff_size    = GETU64(data + 20);
	uint64_t data_size    = GETU64(data + 28);
	uint64_t sample_count = GETU64(data + 36);
	uint32_t table_length = GETU32(data + 44);

	m_pFile->Skip(table_length);

	__int64 end = std::min((__int64)(riff_size + 8), m_pFile->GetLength());

	chunk_t Chunk;

	while (m_pFile->ByteRead(Chunk.data, sizeof(Chunk)) == S_OK && m_pFile->GetPos() < end) {
		__int64 pos = m_pFile->GetPos();

		DLog(L"CRF64File::Open() : found '%c%c%c%c' chunk.",
			WCHAR((Chunk.id >> 0) & 0xff),
			WCHAR((Chunk.id >> 8) & 0xff),
			WCHAR((Chunk.id >> 16) & 0xff),
			WCHAR((Chunk.id >> 24) & 0xff));

		switch (Chunk.id) {
		case FCC('fmt '):
			if (m_fmtdata || Chunk.size < sizeof(PCMWAVEFORMAT) || Chunk.size > 65536) {
				DLog(L"CRF64File::Open() : bad format");
				return E_FAIL;
			}
			m_fmtsize = std::max(Chunk.size, (DWORD)sizeof(WAVEFORMATEX)); // PCMWAVEFORMAT to WAVEFORMATEX
			m_fmtdata.reset(DNew BYTE[m_fmtsize]);
			memset(m_fmtdata.get(), 0, m_fmtsize);
			if (m_pFile->ByteRead(m_fmtdata.get(), Chunk.size) != S_OK) {
				DLog(L"CRF64File::Open() : format can not be read.");
				return E_FAIL;
			}
			break;
		case FCC('data'):
			m_startpos = pos;
			m_length = std::min((__int64)data_size, m_pFile->GetLength() - m_startpos);
			break;
		case FCC('LIST'):
			ReadRIFFINFO(Chunk.size);
			break;
		case FCC('wavl'): // not supported
		case FCC('slnt'): // not supported
			DLog(L"CRF64File::Open() : WAVE file is not supported!");
			return E_FAIL;
		case FCC('ID3 '):
		case FCC('id3 '):
			ReadID3Tag(Chunk.size);
			break;
		case FCC('list'):
			ReadADTLTag(Chunk.size);
			break;
		case FCC('cue '):
			ReadCueTag(Chunk.size);
			break;
		default:
			for (int i = 0; i < sizeof(Chunk.id); i++) {
				BYTE ch = Chunk.data[i];
				if (ch != 0x20 && (ch < '0' || ch > '9') && (ch < 'A' || ch > 'Z') && (ch < 'a' || ch > 'z') && ch != '_') {
					DLog(L"CRF64File::Open() : broken file!");
					return E_FAIL;
				}
			}
			break;
		}

		if (Chunk.id == FCC('data')) {
			m_pFile->Seek(pos + data_size);
		} else {
			m_pFile->Seek(pos + Chunk.size);
		}
	}

	if (!m_startpos || !ProcessWAVEFORMATEX()) {
		return E_FAIL;
	}

	m_length    -= m_length % m_nBlockAlign;
	m_endpos     = m_startpos + m_length;
	m_rtduration = 10000000i64 * m_length / m_nAvgBytesPerSec;

	CheckDTSAC3CD();

	return S_OK;
}
