/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
	static const std::vector<LPCTSTR> subTypesExt = {
		_T("srt"), _T("sub"), _T("smi"), _T("psb"),
		_T("ssa"), _T("ass"), _T("idx"), _T("usf"),
		_T("xss"), _T("txt"), _T("rt"),  _T("sup"),
		_T("vtt"), _T("mks")
	};

	enum SubType {
		SRT = 0,
		SUB,
		SMI,
		PSB,
		SSA,
		ASS,
		IDX,
		USF,
		XSS,
		TXT,
		RT,
		SUP,
		VTT
	};

	LPCTSTR GetSubtitleFileExt(SubType type);

	struct SubFile {
		CString fn; /*SubType type;*/
	};

	void GetSubFileNames(CString fn, const CAtlArray<CString>& paths, CAtlArray<SubFile>& ret);

	CString GuessSubtitleName(CString fn, CString videoName);
};
