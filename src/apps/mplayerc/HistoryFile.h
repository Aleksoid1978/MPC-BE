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

//
// CMpcLstFile
//

class CMpcLstFile
{
protected:
	std::mutex m_Mutex;
	DWORD m_LastAccessTick = 0;

	CStringW m_filename;
	unsigned m_maxCount = 100;
	LPCWSTR m_Header = L"";

	virtual void IntAddEntry(SessionInfo& sesInfo) = 0;
	virtual void IntClearEntries() = 0;

	bool ReadFile();

public:
	bool Clear(); // Clear list and delete file

	void SetFilename(CStringW& filename);
	void SetMaxCount(unsigned maxcount);
};

//
// CHistoryFile
//

class CHistoryFile : public CMpcLstFile
{
private:
	std::list<SessionInfo> m_SessionInfos;

	void IntAddEntry(SessionInfo& sesInfo) override;
	void IntClearEntries() override;

	std::list<SessionInfo>::iterator FindSessionInfo(SessionInfo& sesInfo);
	bool WriteFile();

public:
	bool OpenSessionInfo(SessionInfo& sesInfo, bool bReadPos); // Read or create an entry in the history file
	void SaveSessionInfo(SessionInfo& sesInfo);
	void DeleteSessionInfo(SessionInfo& sesInfo);

	void GetRecentPaths(std::vector<CStringW>& recentPaths, unsigned count);
	void GetRecentSessions(std::vector<SessionInfo>& recentSessions, unsigned count);
};

//
// CFavoritesFile
//

class CFavoritesFile : public CMpcLstFile
{
private:
	void IntAddEntry(SessionInfo& sesInfo) override;
	void IntClearEntries() override;

	bool WriteFile();

public:
	std::list<SessionInfo> m_FavFiles;
	std::list<SessionInfo> m_FavDVDs;

	void OpenFavorites();
	void SaveFavorites();
};
