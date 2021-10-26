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

	bool Equals(const SessionInfo& b) const {
		bool ret = Path.CompareNoCase(b.Path) == 0
			&& Title.Compare(b.Title) == 0
			&& DVDId == b.DVDId
			&& DVDTitle == b.DVDTitle
			&& memcmp(&DVDTimecode, &b.DVDTimecode, sizeof(DVD_HMSF_TIMECODE)) == 0
			&& DVDState == b.DVDState
			&& Position == b.Position
			&& AudioNum == b.SubtitleNum
			&& AudioPath.CompareNoCase(b.AudioPath) == 0
			&& SubtitlePath.CompareNoCase(b.SubtitlePath) == 0;
		return ret;
	}
};

//
// CMpcLstFile
//

class CMpcLstFile
{
private:
	DWORD m_LastAccessTick = 0;

protected:
	std::mutex m_Mutex;
	CStringW m_filename;
	unsigned m_maxCount = 100;

	FILE* CheckOpenFileForRead(bool& valid);
	FILE* OpenFileForWrite();
	void CloseFile(FILE*& pFile);

	virtual void IntClearEntries() = 0;

public:
	bool Clear(); // Clear list and delete file

	void SetFilename(const CStringW& filename);
	void SetMaxCount(unsigned maxcount);
};

//
// CSessionFile
//

class CSessionFile : public CMpcLstFile
{
protected:
	virtual void IntAddEntry(const SessionInfo& sesInfo) = 0;
	bool ReadFile();
};

//
// CHistoryFile
//

class CHistoryFile : public CSessionFile
{
private:
	std::list<SessionInfo> m_SessionInfos;

	void IntAddEntry(const SessionInfo& sesInfo) override;
	void IntClearEntries() override;

	std::list<SessionInfo>::iterator FindSessionInfo(const SessionInfo& sesInfo, std::list<SessionInfo>::iterator begin);
	bool WriteFile();

public:
	bool OpenSessionInfo(SessionInfo& sesInfo, bool bReadPos); // Read or create an entry in the history file
	void SaveSessionInfo(const SessionInfo& sesInfo);
	bool DeleteSessions(const std::list<SessionInfo>& sessions);

	void GetRecentPaths(std::vector<CStringW>& recentPaths, unsigned count);
	void GetRecentSessions(std::vector<SessionInfo>& recentSessions, unsigned count);
};

//
// CFavoritesFile
//

class CFavoritesFile : public CSessionFile
{
private:
	void IntAddEntry(const SessionInfo& sesInfo) override;
	void IntClearEntries() override;

	bool WriteFile();

public:
	std::list<SessionInfo> m_FavFiles;
	std::list<SessionInfo> m_FavDVDs;

	void OpenFavorites();
	void SaveFavorites();
};

//
// CPlaylistListFile
//

enum PlaylistType {
	PLS_Unknown = 0,
	PLS_Basic,
	PLS_Explorer,
	PLS_Link
};

struct PlaylistInfo
{
	PlaylistType PlsType = PLS_Unknown;
	CStringW Path;
	CStringW Title;
	CStringW CurItem;
};

class CPlaylistListFile : public CMpcLstFile
{
	CStringW& m_PlaylistFolder;

	void IntClearEntries() override;
	void IntAddEntry(const PlaylistInfo& plsInfo);
	bool ReadFile();
	bool WriteFile();

public:
	std::list<PlaylistInfo> m_PlaylistInfos;

	void OpenPlaylists();
	void SavePlaylists();
};
