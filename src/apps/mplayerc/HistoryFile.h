/*
 * (C) 2021 see Authors.txt
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

struct SessionInfo_t {
	CStringW Path;
	CStringW Title;
	REFERENCE_TIME Position = 0;
	ULONGLONG DVDId = 0;
	ULONG DVDTitle = 0;
	DVD_HMSF_TIMECODE DVDTimecode = {};
	int AudioNum = -1;
	int SubtitleNum = -1;
	CStringW AudioPath;
	CStringW SubtitlePath;

	void NewDVD(ULONGLONG dvdId) {
		*this = {};
		DVDId = dvdId;
	}

	void NewPath(CStringW path) {
		*this = {};
		Path = path;
	}
};

class CHistoryFile
{
private:
	std::mutex m_Mutex;

	CStringW m_filename;
	std::list<SessionInfo_t> m_SessionInfos;

	std::list<SessionInfo_t>::iterator FindSessionInfo(SessionInfo_t& sesInfo);
	bool ReadFile();
	bool WriteFile();

public:
	void SetFilename(CStringW& filename);

	bool Clear();
	bool GetSessionInfo(SessionInfo_t& sesInfo);
	void GetRecentPaths(std::vector<CStringW>& recentPaths, unsigned count);
	void SetSessionInfo(SessionInfo_t& sesInfo);
};
