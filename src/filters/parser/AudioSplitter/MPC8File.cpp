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
#include "MPC8File.h"

#define MPCTAG(a, b) ((a) | ((b) << 8))

static constexpr auto MPCTAG_STREAMHDR   = MPCTAG('S', 'H');
static constexpr auto MPCTAG_STREAMEND   = MPCTAG('S', 'E');

static constexpr auto MPCTAG_SEEKTBLOFF  = MPCTAG('S', 'O');
static constexpr auto MPCTAG_SEEKTABLE   = MPCTAG('S', 'T');
static constexpr auto MPCTAG_CHAPTERS    = MPCTAG('C', 'T');

static constexpr auto MPCTAG_AUDIOPACKET = MPCTAG('A', 'P');

//
// CMPC8File
//

CMPC8File::CMPC8File()
	: CAudioFile()
{
	m_bitdepth = 16;
	m_subtype = MEDIASUBTYPE_MPC8;
	m_wFormatTag = 0x504D;
}

void CMPC8File::SetProperties(IBaseFilter* pBF)
{
	__super::SetProperties(pBF);

	if (!m_chapters.empty()) {
		if (CComQIPtr<IDSMChapterBag> pCB = pBF) {
			pCB->ChapRemoveAll();
			for (const auto& chap : m_chapters) {
				pCB->ChapAppend(chap.rt, chap.name);
			}
		}
	}
}

HRESULT CMPC8File::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;

	m_pFile->Seek(0);
	m_header_pos = m_pFile->GetPos();
	DWORD id = 0;
	if (m_pFile->ByteRead((BYTE*)&id, 4) != S_OK || id != FCC('MPCK')) {
		return E_FAIL;
	}

	uint64_t seek_table_pos = 0;
	uint64_t seek_table_size = 0;

	for (;;) {
		chunk_t chunk;
		if (!ReadChunkHeader(chunk)) {
			return E_FAIL;
		}

		DLog(L"CMPC8File::Open() : read '%c%c' chunk",
			WCHAR((chunk.id >> 0) & 0xff),
			WCHAR((chunk.id >> 8) & 0xff));

		const auto pos = m_pFile->GetPos();

		switch (chunk.id) {
			case MPCTAG_STREAMHDR:
				if (!ReadStreamHeader()) {
					return E_FAIL;
				}
				break;
			case MPCTAG_SEEKTBLOFF:
				seek_table_pos = ReadVarLen() + chunk.pos;
				break;
			case MPCTAG_AUDIOPACKET:
				m_startpos = chunk.pos;
				break;
			case MPCTAG_STREAMEND:
				return E_FAIL;
		}

		if (m_startpos) {
			break;
		}

		m_pFile->Seek(pos + chunk.size);
	}

	if (!seek_table_pos || !m_startpos) {
		return E_FAIL;
	}

	m_endpos = m_pFile->GetLength();

	size_t tag_size;
	if (ReadApeTag(tag_size)) {
		m_endpos -= tag_size;
	}

	m_pFile->Seek(seek_table_pos);
	if (!ReadSeekTable()) {
		return E_FAIL;
	}

	m_pFile->Seek(m_startpos);

	return S_OK;
}

uint64_t CMPC8File::ReadVarLen()
{
	uint64_t ret = 0;
	uint8_t tmp;
	do {
		tmp = m_pFile->BitRead(8);
		ret = (ret << 7) + (tmp & 0x7f);
	} while (tmp & 0x80);

	return ret;
}

int32_t CMPC8File::ReadGolomb(const int bits)
{
	int32_t n = -1;
	for (uint8_t b = 0; !b; n++) {
		b = (uint8_t)m_pFile->BitRead(1);
	}

	return (n << bits) + m_pFile->BitRead(bits);
}

bool CMPC8File::ReadChunkHeader(chunk_t& chunk)
{
	chunk.pos = m_pFile->GetPos();

	if (m_pFile->ByteRead((BYTE*)&chunk.id, 2) != S_OK) {
		return false;
	}

	chunk.size = ReadVarLen();

	const uint64_t chunk_header_size = m_pFile->GetPos() - chunk.pos;
	if (chunk.size < chunk_header_size) {
		return false;
	}

	chunk.size -= chunk_header_size;
	return true;
}

static const int freq[] = { 44100, 48000, 37800, 32000 };
bool CMPC8File::ReadStreamHeader()
{
	m_pFile->Skip(4); // CRC
	const uint8_t ver = m_pFile->BitRead(8);
	if (ver != 8) {
		DLog(L"CMPC8File::ReadStreamHeader() : wrong %u version", ver);
		return false;
	}

	m_samples = ReadVarLen();
	ReadVarLen();

	m_extradata.SetSize(2);
	if (m_pFile->ByteRead(m_extradata.Data(), m_extradata.Bytes()) != S_OK) {
		return false;
	}

	m_channels = (m_extradata[1] >> 4) + 1;
	const auto samplerateidx = m_extradata[0] >> 5;
	m_samplerate = freq[samplerateidx];
	m_block_pwr = (m_extradata[1] & 3) * 2;
	m_framesamples = 1152 << m_block_pwr;

	m_rtduration = llMulDiv(UNITS, m_samples, m_samplerate, 0);

	return true;
}

bool CMPC8File::ReadSeekTable()
{
	chunk_t chunk;
	if (!ReadChunkHeader(chunk) || chunk.id != MPCTAG_SEEKTABLE) {
		return false;
	}
	const auto pos = m_pFile->GetPos();

	uint64_t size = ReadVarLen();
	m_seek_pwr = m_block_pwr + m_pFile->BitRead(4);
	if (size < 2 || size > (1 + m_samples / (1152ull << m_seek_pwr))) {
		return false;
	}
	m_index.resize(size);

	m_index[0] = ReadVarLen() + m_header_pos;
	m_index[1] = ReadVarLen() + m_header_pos;
	for (unsigned i = 2; i < size; i++) {
		int32_t code = ReadGolomb(12);
		if (code & 1) {
			code = -(code & ~1);
		}
		m_index[i] = (code >> 1) + 2 * m_index[i - 1] - m_index[i - 2];
	}

	m_pFile->Seek(pos + chunk.size);

	for (;;) {
		chunk_t chunk;
		if (!ReadChunkHeader(chunk) || chunk.id != MPCTAG_CHAPTERS) {
			break;
		}

		ReadChapter(chunk.size);
	}

	return true;
}

void CMPC8File::ReadChapter(const uint64_t size)
{
	const auto pos = m_pFile->GetPos();
	const auto end = m_pFile->GetPos() + size;

	auto offset = ReadVarLen();
	m_pFile->BitRead(16); m_pFile->BitRead(16);

	const auto footer_size = APE_TAG_FOOTER_BYTES - 8;
	if (uint64_t(m_pFile->GetPos() + footer_size) >= end) {
		m_pFile->Seek(end);
		return;
	}
	const auto buf_size = end - m_pFile->GetPos();
	std::unique_ptr<BYTE[]> ptr(new(std::nothrow) BYTE[buf_size]);
	if (ptr && m_pFile->ByteRead(ptr.get(), buf_size) == S_OK) {
		CAPEChapters APEChapters;
		if (APEChapters.ReadFooter(ptr.get(), footer_size) && APEChapters.GetTagSize()) {
			const auto tag_size = APEChapters.GetTagSize();
			if (tag_size == (buf_size - footer_size) && APEChapters.ReadTags(ptr.get() + footer_size, tag_size)) {
				CString track, title;
				for (const auto& [type, key, value] : APEChapters.TagItems) {
					if (type == CAPETag::APE_TYPE_STRING) {
						CString tagKey(key); tagKey.MakeLower();
						if (tagKey == L"track") {
							track = std::get<CString>(value);
						} else if (tagKey == L"title") {
							title = std::get<CString>(value);
						}
					}
				}

				CString chapterTitle;
				if (!track.IsEmpty() && !title.IsEmpty()) {
					chapterTitle.Format(L"%s - %s", title.GetString(), track.GetString());
				} else if (!title.IsEmpty()) {
					chapterTitle.Format(L"%s", title.GetString());
				} else if (!track.IsEmpty()) {
					chapterTitle.Format(L"%s", track.GetString());
				}

				if (!chapterTitle.IsEmpty()) {
					const auto rt = llMulDiv(UNITS, offset, m_samplerate, 0);
					m_chapters.emplace_back(chapterTitle, rt);
				}
			}
		}
	}

	m_pFile->Seek(end);
}

REFERENCE_TIME CMPC8File::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_currentframe = 0;
		m_pFile->Seek(m_index.front());
		return 0;
	}

	const auto pos = llMulDiv(rt, m_samplerate, UNITS, 0);
	uint32_t index = (pos / m_framesamples) >> (m_seek_pwr - m_block_pwr);
	if (index >= m_index.size()) {
		index = m_index.size() - 1;
	}

	m_pFile->Seek(m_index[index]);
	m_currentframe = index << (m_seek_pwr - m_block_pwr);
	rt = llMulDiv(UNITS, m_currentframe * m_framesamples, m_samplerate, 0);

	return rt;
}

int CMPC8File::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	for (;;) {
		chunk_t chunk;
		if (!ReadChunkHeader(chunk)) {
			return 0;
		}

		const auto pos = m_pFile->GetPos();
		switch (chunk.id) {
			case MPCTAG_STREAMEND:
				return 0;
			case MPCTAG_AUDIOPACKET:
				packet->rtStart = llMulDiv(UNITS, m_currentframe * m_framesamples, m_samplerate, 0);
				m_currentframe++;
				packet->rtStop = llMulDiv(UNITS, m_currentframe * m_framesamples, m_samplerate, 0);

				if (!packet->SetCount(chunk.size) || m_pFile->ByteRead(packet->data(), chunk.size) != S_OK) {
					return 0;
				}

				return chunk.size;
			default:
				m_pFile->Seek(pos + chunk.size);
				continue;
		}
	}
}
