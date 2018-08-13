/*
 * (C) 2011-2018 see Authors.txt
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
#include "CUE.h"
#include "text.h"

CString GetCUECommand(CString& ln)
{
	CString c;
	ln.Trim();
	int i = ln.Find(' ');
	if (i < 0) {
		c = ln;
		ln.Empty();
	} else {
		c = ln.Left(i);
		ln.Delete(0, i + 1);
		ln.TrimLeft();
	}
	return c;
}

void MakeCUETitle(CString &Title, const CString& title, const CString& performer, const UINT trackNum/* = UINT_MAX*/)
{
	if (performer.GetLength() > 0 && title.GetLength() > 0) {
		Title.Format(L"%02u. %s - %s", trackNum, performer, title);
	} else if (performer.GetLength() > 0) {
		Title.Format(L"%02u. %s", trackNum, performer);
	} else if (title.GetLength() > 0) {
		Title.Format(L"%02u. %s", trackNum, title);
	}

	if (trackNum == UINT_MAX && Title.GetLength() > 0) {
		Title.Delete(0, Title.Find('.') + 2);
	}
}

bool ParseCUESheet(CString cueData, std::list<Chapters> &ChaptersList, CString& Title, CString& Performer)
{
	BOOL fAudioTrack;
	int track_no = -1, /*index, */index_cnt = 0;
	REFERENCE_TIME rt = _I64_MIN;
	CString TrackTitle;
	CString title, performer;

	Title.Empty();
	Performer.Empty();

	std::list<CString> cuelines;
	Explode(cueData, cuelines, L'\n');

	if (cuelines.size() <= 1) {
		return false;
	}

	for (CString cueLine : cuelines) {
		cueLine.Trim();

		CString cmd = GetCUECommand(cueLine);

		if (cmd == L"TRACK") {
			if (rt != _I64_MIN && track_no != -1 && index_cnt) {
				MakeCUETitle(TrackTitle, title, performer, track_no);
				if (!TrackTitle.IsEmpty()) {
					ChaptersList.push_back(Chapters{TrackTitle, rt});
				}
			}
			rt = _I64_MIN;
			index_cnt = 0;

			WCHAR type[256];
			swscanf_s(cueLine, L"%d %s", &track_no, type, _countof(type));
			fAudioTrack = (wcscmp(type, L"AUDIO") == 0);
			TrackTitle.Format(L"Track %02d", track_no);
		} else if (cmd == L"TITLE") {
			cueLine.Trim(L" \"");
			title = cueLine;

			if (track_no == -1) {
				Title = title;
			}
		} else if (cmd == L"PERFORMER") {
			cueLine.Trim(L" \"");
			performer = cueLine;

			if (track_no == -1) {
				Performer = performer;
			}
		} else if (cmd == L"INDEX") {
			unsigned mm, ss, ff;
			if (3 == swscanf_s(cueLine, L"01 %u:%u:%u", &mm, &ss, &ff)) {
				// "INDEX 01" is required and denotes the start of the track
				if (fAudioTrack) {
					index_cnt++;
					rt = UNITS * (mm*60 + ss) + UNITS * ff / 75;
				}
			}
		}
	}

	if (rt != _I64_MAX && track_no != -1 && index_cnt) {
		MakeCUETitle(TrackTitle, title, performer, track_no);
		if (!TrackTitle.IsEmpty()) {
			ChaptersList.push_back(Chapters{TrackTitle, rt});
		}
	}

	if (ChaptersList.size()) {
		return true;
	} else {
		return false;
	}
}
