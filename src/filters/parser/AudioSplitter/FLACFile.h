/*
 * (C) 2020-2025 see Authors.txt
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

class CFLACFile : public CAudioFile
{
	void* m_pDecoder = nullptr;

	uint64_t m_offset = 0;
	uint64_t m_curPos = 0;
	uint64_t m_totalsamples = 0;

	struct {
		CString title;
		CString artist;
		CString comment;
		CString year;
		CString album;
		bool got_vorbis_comments = false;
	} m_info;

	std::list<Chapters> m_chapters;

	struct CoverInfo_t {
		CStringW name;
		CStringW desc;
		CStringW mime;
		std::vector<BYTE> picture;
	};
	std::vector<CoverInfo_t> m_covers;

public:
	CFLACFile();
	~CFLACFile();

	void SetProperties(IBaseFilter* pBF);

	HRESULT Open(CBaseSplitterFile* pFile);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	int GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart);
	CString GetName() const { return L"Flac"; }

	void UpdateFromMetadata(void* pBuffer);
	auto* GetFile() { return m_pFile; }
	bool m_bIsEOF = false;
};
