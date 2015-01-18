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

void MakeCUETitle(CString &Title, CString title, CString performer, UINT trackNum)
{
	if (performer.GetLength() > 0 && title.GetLength() > 0) {
		Title.Format(_T("%02u. %s - %s"), trackNum, performer, title);
	} else if (performer.GetLength() > 0) {
		Title.Format(_T("%02u. %s"), trackNum, performer);
	} else if (title.GetLength() > 0) {
		Title.Format(_T("%02u. %s"), trackNum, title);
	}

	if (trackNum == UINT_MAX && Title.GetLength() > 0) {
		Title.Delete(0, Title.Find('.') + 2);
	}
}

bool ParseCUESheet(CString cueData, CAtlList<Chapters> &ChaptersList, CString& Title, CString& Performer)
{
	BOOL fAudioTrack;
	int track_no = -1, /*index, */index_cnt = 0;
	REFERENCE_TIME rt = _I64_MIN;
	CString TrackTitle;
	CString title, performer;

	Title.Empty();
	Performer.Empty();

	CAtlList<CString> cuelines;
	Explode(cueData, cuelines, '\n');

	if (cuelines.GetCount() <= 1) {
		return false;
	}

	while (cuelines.GetCount()) {
		CString cueLine	= cuelines.RemoveHead().Trim();
		CString cmd		= GetCUECommand(cueLine);

		if (cmd == _T("TRACK")) {
			if (rt != _I64_MIN && track_no != -1 && index_cnt) {
				MakeCUETitle(TrackTitle, title, performer, track_no);
				if (!TrackTitle.IsEmpty()) {
					ChaptersList.AddTail(Chapters(TrackTitle, rt));
				}
			}
			rt = _I64_MIN;
			index_cnt = 0;

			TCHAR type[256];
			swscanf_s(cueLine, _T("%d %s"), &track_no, type, _countof(type)-1);
			fAudioTrack = (wcscmp(type, _T("AUDIO")) == 0);
			TrackTitle.Format(_T("Track %02d"), track_no);
		} else if (cmd == _T("TITLE")) {
			cueLine.Trim(_T(" \""));
			title = cueLine;

			if (track_no == -1) {
				Title = title;
			}
		} else if (cmd == _T("PERFORMER")) {
			cueLine.Trim(_T(" \""));
			performer = cueLine;

			if (track_no == -1) {
				Performer = performer;
			}
		} else if (cmd == _T("INDEX")) {
			int idx, mm, ss, ff;
			swscanf_s(cueLine, _T("%d %d:%d:%d"), &idx, &mm, &ss, &ff);

			if (fAudioTrack) {
				index_cnt++;

				rt = MILLISECONDS_TO_100NS_UNITS((mm * 60 + ss) * 1000);
			}
		}
	}

	if (rt != _I64_MAX && track_no != -1 && index_cnt) {
		MakeCUETitle(TrackTitle, title, performer, track_no);
		if (!TrackTitle.IsEmpty()) {
			ChaptersList.AddTail(Chapters(TrackTitle, rt));
		}
	}

	if (ChaptersList.GetCount()) {
		return true;
	} else {
		return false;
	}
}
