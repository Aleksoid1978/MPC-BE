/*
 * (C) 2011-2014 see Authors.txt
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

#include <atlcoll.h>

struct Chapters {
	REFERENCE_TIME	rt;
	CString			name;

	Chapters() {
		rt		= 0;
	}

	Chapters(CString s, REFERENCE_TIME t) {
		rt		= t;
		name	= s;
	}
};

CString GetCUECommand(CString& ln);
void MakeCUETitle(CString &Title, CString title, CString performer, UINT trackNum = UINT_MAX);
bool ParseCUESheet(CString cueData, CAtlList<Chapters> &ChaptersList, CString& Title, CString& Performer);

