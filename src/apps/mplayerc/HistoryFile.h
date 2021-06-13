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

struct SessionInfo {
	CStringW Path;
	CStringW Title;

	ULONGLONG DVDId = 0;
	ULONG DVDTitle = 0;
	DVD_HMSF_TIMECODE DVDTimecode = {};
	std::vector<uint8_t> DVDState;

	REFERENCE_TIME Position = 0;
	int AudioNum = -1;
	int SubtitleNum = -1;
	CStringW AudioPath;
	CStringW SubtitlePath;

	void NewPath(CStringW path) {
		*this = {};
		Path = path;
	}

	void CleanPosition() {
		DVDTitle = 0;
		DVDTimecode = {};
		DVDState.clear();

		Position = 0;
		AudioNum = -1;
		SubtitleNum = -1;
		AudioPath.Empty();
		SubtitlePath.Empty();
	}
};

class CHistoryFile
{
private:
	std::mutex m_Mutex;
	DWORD m_LastAccessTick = 0;

	CStringW m_filename;
	std::list<SessionInfo> m_SessionInfos;
	unsigned m_maxCount = 100;

	std::list<SessionInfo>::iterator FindSessionInfo(SessionInfo& sesInfo);
	bool ReadFile();
	bool WriteFile();

public:
	void SetFilename(CStringW& filename);
	void SetMaxCount(unsigned maxcount);

	bool Clear(); // Clear list and delete history file
	bool OpenSessionInfo(SessionInfo& sesInfo, bool bReadPos); // Read or create an entry in the history file
	void SaveSessionInfo(SessionInfo& sesInfo);
	void DeleteSessionInfo(SessionInfo& sesInfo);

	void GetRecentPaths(std::vector<CStringW>& recentPaths, unsigned count);
	void GetRecentSessions(std::vector<SessionInfo>& recentSessions, unsigned count);
};
