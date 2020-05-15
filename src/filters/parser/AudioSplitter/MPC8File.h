/*
 * (C) 2020 see Authors.txt
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

#pragma once

#include "AudioFile.h"

class CMPC8File : public CAudioFile
{
	struct chunk_t {
		uint16_t id = 0;
		uint64_t pos = 0;
		uint64_t size = 0;
	};

	uint64_t m_header_pos = 0;
	uint64_t m_samples = 0;

	uint32_t m_block_pwr = 0;
	uint32_t m_seek_pwr = 0;

	uint32_t m_currentframe = 0;
	uint32_t m_framesamples = 0;

	std::vector<uint64_t> m_index;

	CAPETag* m_APETag;

	std::vector<Chapters> m_chapters;

	bool ReadChunkHeader(chunk_t& chunk);

	uint64_t ReadVarLen();
	int32_t ReadGolomb(const int bits);

	bool ReadStreamHeader();
	bool ReadSeekTable();
	void ReadChapter(const uint64_t size);

public:
	CMPC8File();
	~CMPC8File();

	void SetProperties(IBaseFilter* pBF);

	HRESULT Open(CBaseSplitterFile* pFile);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	int GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart);
	CString GetName() const { return L"MusePack 8"; };
};
