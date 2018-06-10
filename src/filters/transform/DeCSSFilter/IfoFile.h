/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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

#include <map>
#include <basestruct.h>
#include "VobFile.h"

// CIfoFile

class CIfoFile
{
	struct chapter_t {
		REFERENCE_TIME rtime;
		UINT32 first_sector;
		UINT32 last_sector;
		UINT32 title:16, track:8;
	};

	std::map<DWORD, CString> m_pStream_Lang;
	std::vector<chapter_t> m_pChapters;

	REFERENCE_TIME	m_rtDuration;
	fraction_t		m_Aspect;

public:
	CIfoFile();

	bool OpenIFO(CString fn, CVobFile* vobfile, ULONG nProgNum = 0); // vts ifo

	BSTR			GetTrackName(UINT aTrackIdx) const;
	UINT			GetChaptersCount() const { return (UINT)m_pChapters.size(); }
	REFERENCE_TIME	GetChapterTime(UINT ChapterNumber) const;
	__int64			GetChapterPos(UINT ChapterNumber) const;
	REFERENCE_TIME	GetDuration() const { return m_rtDuration; }
	fraction_t		GetAspectRatio() const { return m_Aspect; }

	static bool GetTitleInfo(LPCTSTR fn, ULONG nTitleNum, ULONG& VTSN /* out */, ULONG& TTN /* out */); // video_ts.ifo

private:
	CFile		m_ifoFile;
	BYTE		ReadByte();
	WORD		ReadWord();
	DWORD		ReadDword();
};
