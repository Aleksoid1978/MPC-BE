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

#pragma once

struct LyricWord {
	REFERENCE_TIME start;
	REFERENCE_TIME end;
	int textStart;
	int textLength;
};

struct LyricLine {
	REFERENCE_TIME start;
	REFERENCE_TIME end;
	CString text;
	std::vector<LyricWord> words;
};

struct LyricTrack {
	CString name;
	CString language;
	bool external = false;
	std::vector<LyricLine> lines;
};

bool ParseLyricJson(const CStringA& json, std::vector<LyricLine>& lines);
