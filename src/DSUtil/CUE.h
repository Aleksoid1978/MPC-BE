/*
 * (C) 2011-2020 see Authors.txt
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

struct Chapters {
	CString        name;
	REFERENCE_TIME rt = 0;

	Chapters::Chapters(const CString& _name, const REFERENCE_TIME _rt)
		: name(_name)
		, rt(_rt)
	{}
};

CString GetCUECommand(CString& ln);
void MakeCUETitle(CString &Title, const CString& title, const CString& performer, const UINT trackNum = UINT_MAX);
bool ParseCUESheet(const CString& cueData, std::list<Chapters> &ChaptersList, CString& Title, CString& Performer);
