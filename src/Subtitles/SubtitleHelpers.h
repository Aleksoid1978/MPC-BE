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

#include <afx.h>
#include <atlcoll.h>
#include <vector>

namespace Subtitle
{
	static const LPCWSTR s_SubFileExts[] = {
		L"srt", L"ass", L"ssa", L"sub", L"idx",
		L"smi", L"sup", L"usf", L"vtt", L"psb",
		L"xss", L"rt",  L"mks", L"lrc"
	};

	enum SubType {
		SRT = 0,
		ASS,
		SSA,
		SUB,
		IDX,
		SMI,
		SUP,
		USF,
		VTT,
		PSB,
		XSS,
		RT,
		MKS,
		LRC
	};

	LPCWSTR GetSubtitleFileExt(SubType type);

	void GetSubFileNames(CString fn, const std::vector<CString>& paths, std::vector<CString>& ret);

	CString GuessSubtitleName(CString fn, CString videoName);
};
