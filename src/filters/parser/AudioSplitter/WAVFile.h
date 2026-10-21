/*
 * (C) 2014-2026 see Authors.txt
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

class CWAVFile : public CAudioFile
{
protected:
	__int64  m_length      = 0;

	std::unique_ptr<BYTE[]> m_fmtdata;
	DWORD    m_fmtsize     = 0;

	WORD     m_nBlockAlign = 0;
	int      m_blocksize   = 0;

	REFERENCE_TIME m_rtPrev        = 0;
	REFERENCE_TIME m_frameDuration = 0;

	std::map<DWORD, CStringA> m_info;

	struct RiffChapters : public Chapters {
		DWORD id = 0;

		RiffChapters(const CStringW& _name, const REFERENCE_TIME _rt, DWORD _id)
			: Chapters(_name, _rt)
			, id(_id)
		{}
	};
	std::vector<RiffChapters> m_chapters;

	bool ProcessWAVEFORMATEX();
	HRESULT ReadRIFFINFO(const DWORD chunk_size);

	HRESULT ReadADTLTag(const DWORD chunk_size);
	HRESULT ReadCueTag(const DWORD chunk_size);

	bool CheckDTSAC3CD();

public:
	CWAVFile() = default;
	virtual ~CWAVFile();

	bool SetMediaType(CMediaType& mt) override;
	void SetProperties(IBaseFilter* pBF) override;

	HRESULT Open(CBaseSplitterFile* pFile) override;
	REFERENCE_TIME Seek(REFERENCE_TIME rt) override;
	int GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart) override;
	LPCWSTR GetName() const override { return L"WAVE"; };
};
