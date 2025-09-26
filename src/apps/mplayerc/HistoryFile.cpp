/*
 * (C) 2021-2025 see Authors.txt
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
#include "HistoryFile.h"
#include "DSUtil/CryptoUtils.h"

//
// CMpcLstFile
//

FILE* CMpcLstFile::CheckOpenFileForRead(bool& valid)
{
	if (!::PathFileExistsW(m_filename)) {
		valid = false;
		return nullptr;
	}

	const ULONGLONG tick = GetTickCount64();
	if (m_LastAccessTick && tick - m_LastAccessTick < 100u) {
		valid = true;
		return nullptr;
	}

	FILE* pFile;

	do { // Open mpc-be.ini in UNICODE mode, retry if it is already being used by another process
		pFile = _wfsopen(m_filename, L"r, ccs=UNICODE", _SH_SECURE);
		if (pFile || GetLastError() != ERROR_SHARING_VIOLATION) {
			break;
		}
		Sleep(100);
	} while (true);

	ASSERT(pFile);
	valid = pFile != nullptr;

	return pFile;
}

FILE* CMpcLstFile::OpenFileForWrite()
{
	FILE* pFile;

	do { // Open file, retry if it is already being used by another process
		pFile = _wfsopen(m_filename, L"w, ccs=UTF-8", _SH_SECURE);
		if (pFile || (GetLastError() != ERROR_SHARING_VIOLATION)) {
			break;
		}
		Sleep(100);
	} while (true);

	return pFile;
}

void CMpcLstFile::CloseFile(FILE*& pFile)
{
	int fpStatus = fclose(pFile);
	ASSERT(fpStatus == 0);
	m_LastAccessTick = GetTickCount64();
	pFile = nullptr;
}

bool CMpcLstFile::Clear()
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	if (_wremove(m_filename) == 0 || errno == ENOENT) {
		IntClearEntries();
		return true;
	}
	return false;
}

void CMpcLstFile::SetFilename(const CStringW& filename)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	m_filename = filename;
}

void CMpcLstFile::SetMaxCount(unsigned maxcount)
{
	m_maxCount = std::clamp(maxcount, 10u, 999u);
}

//
// CSessionFile
//

bool CSessionFile::ReadFile()
{
	bool valid = false;
	FILE* pFile = CheckOpenFileForRead(valid);
	if (!pFile) {
		return valid;
	}

	CStdioFile file(pFile);

	IntClearEntries();

	SessionInfo sesInfo;
	CStringW section;
	CStringW line;

	while (file.ReadString(line)) {
		int pos = 0;

		if (line[0] == '[') { // new section
			if (section.GetLength()) {
				section.Empty();
				IntAddEntry(sesInfo);
				sesInfo = {};
			}

			pos = line.Find(']');
			if (pos > 0) {
				section = line.Mid(1, pos - 1);
			}
			continue;
		}

		if (line[0] == ';' || section.IsEmpty()) {
			continue;
		}

		pos = line.Find('=');
		if (pos > 0 && pos + 1 < line.GetLength()) {
			CStringW param = line.Mid(0, pos).Trim();
			CStringW value = line.Mid(pos + 1).Trim();
			if (value.GetLength()) {
				if (param == L"Path") {
					sesInfo.Path = value;
				}
				else if (param == L"Position") {
					int h, m, s;
					if (swscanf_s(value, L"%02d:%02d:%02d", &h, &m, &s) == 3) {
						sesInfo.Position = (((h * 60) + m) * 60 + s) * UNITS;
					}
				}
				else if (param == L"DVDId") {
					StrHexToUInt64(value, sesInfo.DVDId);
				}
				else if (param == L"DVDPosition") {
					unsigned dvdTitle, h, m, s;
					if (swscanf_s(value, L"%02u,%02u:%02u:%02u", &dvdTitle, &h, &m, &s) == 4) {
						sesInfo.DVDTitle = dvdTitle;
						sesInfo.DVDTimecode = { (BYTE)h, (BYTE)m, (BYTE)s, 0 };
					}
				}
				else if (param == L"DVDState") {
					unsigned ret = Base64ToBynary(value, sesInfo.DVDState);
					if (!ret) {
						sesInfo.DVDState.clear();
					}
				}
				else if (param == L"AudioNum") {
					int32_t i32val;
					if (StrToInt32(value, i32val) && i32val >= 1) {
						sesInfo.AudioNum = i32val - 1;
					}
				}
				else if (param == L"SubtitleNum") {
					int32_t i32val;
					if (StrToInt32(value, i32val) && i32val >= 1) {
						sesInfo.SubtitleNum = i32val - 1;
					}
				}
				else if (param == L"AudioPath") {
					sesInfo.AudioPath = value;
				}
				else if (param == L"SubtitlePath") {
					sesInfo.SubtitlePath = value;
				}
				else if (param == L"Title") {
					sesInfo.Title = value;
				}
			}
		}
	}

	CloseFile(pFile);

	if (section.GetLength()) {
		IntAddEntry(sesInfo);
	}

	return true;
}

//
// CHistoryFile
//

void CHistoryFile::IntAddEntry(const SessionInfo& sesInfo)
{
	if (sesInfo.Path.GetLength()) {
		m_SessionInfos.emplace_back(sesInfo);
	}
}

void CHistoryFile::IntClearEntries()
{
	m_SessionInfos.clear();
}

std::list<SessionInfo>::iterator CHistoryFile::FindSessionInfo(const SessionInfo& sesInfo, std::list<SessionInfo>::iterator begin)
{
	if (sesInfo.DVDId) {
		for (auto it = begin; it != m_SessionInfos.end(); ++it) {
			if (sesInfo.DVDId == (*it).DVDId) {
				return it;
			}
		}
	}
	else if (sesInfo.Path.GetLength()) {
		for (auto it = begin; it != m_SessionInfos.end(); ++it) {
			if (sesInfo.Path.CompareNoCase((*it).Path) == 0) {
				return it;
			}
		}
	}
	else {
		ASSERT(0);
	}

	return m_SessionInfos.end();
}

bool CHistoryFile::WriteFile()
{
	FILE* pFile = OpenFileForWrite();
	if (!pFile) {
		return false;
	}

	bool ret = true;

	CStdioFile file(pFile);
	CStringW str;
	try {
		file.WriteString(L"; MPC-BE History File 0.1\n");
		int i = 1;
		for (const auto& sesInfo : m_SessionInfos) {
			if (sesInfo.Path.GetLength()) {
				str.Format(L"\n[%03d]\n", i++);

				str.AppendFormat(L"Path=%s\n", sesInfo.Path);

				if (sesInfo.Title.GetLength()) {
					str.AppendFormat(L"Title=%s\n", sesInfo.Title);
				}

				if (sesInfo.DVDId) {
					str.AppendFormat(L"DVDId=%016I64x\n", sesInfo.DVDId);
					if (sesInfo.DVDTitle) {
						str.AppendFormat(L"DVDPosition=%02u,%02u:%02u:%02u\n",
							(unsigned)sesInfo.DVDTitle,
							(unsigned)sesInfo.DVDTimecode.bHours,
							(unsigned)sesInfo.DVDTimecode.bMinutes,
							(unsigned)sesInfo.DVDTimecode.bSeconds);
					}
					if (sesInfo.DVDState.size()) {
						CStringW base64 = BynaryToBase64W(sesInfo.DVDState.data(), sesInfo.DVDState.size());
						if (base64.GetLength()) {
							str.AppendFormat(L"DVDState=%s\n", base64);
						}
					}
				}
				else {
					if (sesInfo.Position > UNITS) {
						LONGLONG seconds = sesInfo.Position / UNITS;
						int h = (int)(seconds / 3600);
						int m = (int)(seconds / 60 % 60);
						int s = (int)(seconds % 60);
						str.AppendFormat(L"Position=%02d:%02d:%02d\n", h, m, s);
					}
					if (sesInfo.AudioNum >= 0) {
						str.AppendFormat(L"AudioNum=%d\n", sesInfo.AudioNum + 1);
					}
					if (sesInfo.SubtitleNum >= 0) {
						str.AppendFormat(L"SubtitleNum=%d\n", sesInfo.SubtitleNum + 1);
					}
					if (sesInfo.AudioPath.GetLength()) {
						str.AppendFormat(L"AudioPath=%s\n", sesInfo.AudioPath);
					}
					if (sesInfo.SubtitlePath.GetLength()) {
						str.AppendFormat(L"SubtitlePath=%s\n", sesInfo.SubtitlePath);
					}
				}
				file.WriteString(str);
			}
		}
	}
	catch (CFileException& e) {
		// Fail silently if disk is full
		UNREFERENCED_PARAMETER(e);
		ASSERT(FALSE);
		ret = false;
	}

	CloseFile(pFile);

	return ret;
}

bool CHistoryFile::OpenSessionInfo(SessionInfo& sesInfo, bool bReadPos)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	ReadFile();

	bool found = false;
	auto it = FindSessionInfo(sesInfo, m_SessionInfos.begin());

	if (it != m_SessionInfos.end()) {
		found = true;
		auto& si = (*it);

		if (bReadPos) {
			if (sesInfo.DVDId) {
				sesInfo.DVDTitle    = si.DVDTitle;
				sesInfo.DVDTimecode = si.DVDTimecode;
				sesInfo.DVDState    = si.DVDState;
			}
			else if (sesInfo.Path.GetLength()) {
				sesInfo.Position     = si.Position;
				sesInfo.AudioNum     = si.AudioNum;
				sesInfo.SubtitleNum  = si.SubtitleNum;
				sesInfo.AudioPath    = si.AudioPath;
				sesInfo.SubtitlePath = si.SubtitlePath;
			}
		}

		if (sesInfo.Title.IsEmpty()) {
			sesInfo.Title = si.Title;
		}
	}

	if (it != m_SessionInfos.begin() || !found) { // not first entry or empty list
		if (found) {
			m_SessionInfos.erase(it);
		}

		m_SessionInfos.emplace_front(sesInfo);

		while (m_SessionInfos.size() > m_maxCount) {
			m_SessionInfos.pop_back();
		}
		WriteFile();
	}

	return found;
}

void CHistoryFile::SaveSessionInfo(const SessionInfo& sesInfo)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	ReadFile();

	if (m_SessionInfos.size()) {
		auto it = FindSessionInfo(sesInfo, m_SessionInfos.begin());

		if (it == m_SessionInfos.begin() && sesInfo.Equals(*it)) {
			return;
		}

		if (it != m_SessionInfos.end()) {
			m_SessionInfos.erase(it);
		}
	}

	m_SessionInfos.emplace_front(sesInfo); // Writing new data

	while (m_SessionInfos.size() > m_maxCount) {
		m_SessionInfos.pop_back();
	}

	WriteFile();
}

bool CHistoryFile::DeleteSessions(const std::list<SessionInfo>& sessions)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	if (!ReadFile()) {
		return false;
	}

	bool changed = false;

	for (const auto& sesInfo : sessions) {
		auto it = FindSessionInfo(sesInfo, m_SessionInfos.begin());

		// delete what was found and all unexpected duplicates (for example, after manual editing)
		while (it != m_SessionInfos.end()) {
			m_SessionInfos.erase(it++);
			changed = true;
			it = FindSessionInfo(sesInfo, it);
		}
	}

	if (changed) {
		return WriteFile();
	}

	return true; // already deleted
}

void CHistoryFile::GetRecentPaths(std::vector<CStringW>& recentPaths, unsigned count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	recentPaths.clear();
	ReadFile();

	if (count > m_SessionInfos.size()) {
		count = m_SessionInfos.size();
	}

	if (count) {
		recentPaths.reserve(count);
		auto it = m_SessionInfos.cbegin();
		for (unsigned i = 0; i < count; i++) {
			recentPaths.emplace_back((*it++).Path);
		}
	}
}

void CHistoryFile::GetRecentSessions(std::vector<SessionInfo>& recentSessions, unsigned count)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	recentSessions.clear();
	ReadFile();

	if (count > m_SessionInfos.size()) {
		count = m_SessionInfos.size();
	}

	if (count) {
		recentSessions.reserve(count);
		auto it = m_SessionInfos.cbegin();
		for (unsigned i = 0; i < count; i++) {
			recentSessions.emplace_back(*it++);
		}
	}
}

unsigned CHistoryFile::GetSessionsCount()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	ReadFile();

	return m_SessionInfos.size();
}

void CHistoryFile::TrunkFile(unsigned maxcount)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	ReadFile();

	SetMaxCount(maxcount);

	while (m_SessionInfos.size() > m_maxCount) {
		m_SessionInfos.pop_back();
	}

	WriteFile();
}

//
// CFavoritesFile
//

void CFavoritesFile::IntAddEntry(const SessionInfo& sesInfo)
{
	if (sesInfo.Path.GetLength()) {
		if (sesInfo.DVDId) {
			m_DVDs.emplace_back(sesInfo);
		} else {
			m_Files.emplace_back(sesInfo);
		}
	}
}

void CFavoritesFile::IntClearEntries()
{
	m_Files.clear();
	m_DVDs.clear();
}

bool CFavoritesFile::WriteFile()
{
	FILE* pFile = OpenFileForWrite();
	if (!pFile) {
		return false;
	}

	bool ret = true;

	CStdioFile file(pFile);
	CStringW str;
	try {
		file.WriteString(L"; MPC-BE Favorites File 0.1\n");
		int i = 1;
		for (const auto& sesInfo : m_Files) {
			if (sesInfo.Path.GetLength()) {
				str.Format(L"\n[%03d]\n", i++);

				str.AppendFormat(L"Path=%s\n", sesInfo.Path);

				if (sesInfo.Title.GetLength()) {
					str.AppendFormat(L"Title=%s\n", sesInfo.Title);
				}
				if (sesInfo.Position > UNITS) {
					LONGLONG seconds = sesInfo.Position / UNITS;
					int h = (int)(seconds / 3600);
					int m = (int)(seconds / 60 % 60);
					int s = (int)(seconds % 60);
					str.AppendFormat(L"Position=%02d:%02d:%02d\n", h, m, s);
				}
				if (sesInfo.AudioNum >= 0) {
					str.AppendFormat(L"AudioNum=%d\n", sesInfo.AudioNum + 1);
				}
				if (sesInfo.SubtitleNum >= 0) {
					str.AppendFormat(L"SubtitleNum=%d\n", sesInfo.SubtitleNum + 1);
				}
				if (sesInfo.AudioPath.GetLength()) {
					str.AppendFormat(L"AudioPath=%s\n", sesInfo.AudioPath);
				}
				if (sesInfo.SubtitlePath.GetLength()) {
					str.AppendFormat(L"SubtitlePath=%s\n", sesInfo.SubtitlePath);
				}

				file.WriteString(str);
			}
		}

		for (const auto& sesDvd : m_DVDs) {
			if (sesDvd.Path.GetLength() && sesDvd.DVDId) {
				str.Format(L"\n[%03d]\n", i++);

				str.AppendFormat(L"Path=%s\n", sesDvd.Path);

				if (sesDvd.Title.GetLength()) {
					str.AppendFormat(L"Title=%s\n", sesDvd.Title);
				}

				str.AppendFormat(L"DVDId=%016I64x\n", sesDvd.DVDId);

				if (sesDvd.DVDTitle) {
					str.AppendFormat(L"DVDPosition=%02u,%02u:%02u:%02u\n",
						(unsigned)sesDvd.DVDTitle,
						(unsigned)sesDvd.DVDTimecode.bHours,
						(unsigned)sesDvd.DVDTimecode.bMinutes,
						(unsigned)sesDvd.DVDTimecode.bSeconds);
				}
				if (sesDvd.DVDState.size()) {
					CStringW base64 = BynaryToBase64W(sesDvd.DVDState.data(), sesDvd.DVDState.size());
					if (base64.GetLength()) {
						str.AppendFormat(L"DVDState=%s\n", base64);
					}
				}

				file.WriteString(str);
			}
		}
	}
	catch (CFileException& e) {
		// Fail silently if disk is full
		UNREFERENCED_PARAMETER(e);
		ASSERT(FALSE);
		ret = false;
	}

	CloseFile(pFile);

	return ret;
}

void CFavoritesFile::GetFavorites(std::list<SessionInfo>& favFiles, std::list<SessionInfo>& favDVDs)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	bool ok = ReadFile();
	if (!ok) {
		IntClearEntries();
	}

	favFiles = m_Files;
	favDVDs = m_DVDs;
}

void CFavoritesFile::AppendFavorite(const SessionInfo& fav)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	bool ok = ReadFile();
	if (!ok) {
		IntClearEntries();
	}

	if (fav.DVDId) {
		m_DVDs.emplace_back(fav);
	} else {
		m_Files.emplace_back(fav);
	}

	WriteFile();
}

void CFavoritesFile::SaveFavorites(const std::list<SessionInfo>& favFiles, const std::list<SessionInfo>& favDVDs)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	m_Files = favFiles;
	m_DVDs = favDVDs;

	WriteFile();
}

//
// CPlaylistConfigFile
//

bool CPlaylistConfigFile::BasicPlsContains(LPCWSTR filename)
{
	for (const auto&pli : m_PlaylistInfos) {
		if (pli.Type == PLS_Basic && pli.Path.CompareNoCase(filename) == 0) {
			return true;
		}
	}
	return false;
}

void CPlaylistConfigFile::IntClearEntries()
{
	m_PlaylistInfos.clear();
}

void CPlaylistConfigFile::IntAddEntry(PlaylistInfo& plsInfo)
{
	if (plsInfo.Path.IsEmpty() && plsInfo.Type != PLS_Explorer) {
		return;
	}

	if (plsInfo.Type == PLS_Basic) {
		if (BasicPlsContains(plsInfo.Path)) {
			return;
		}
		if (!::PathFileExistsW(m_PlaylistFolder + plsInfo.Path)) {
			return;
		}
		plsInfo.Sorting = PLS_SORT_None;
	}

	m_PlaylistInfos.emplace_back(plsInfo);
}

bool CPlaylistConfigFile::ReadFile()
{
	bool valid = false;
	FILE* pFile = CheckOpenFileForRead(valid);
	if (!pFile) {
		return valid;
	}

	CStdioFile file(pFile);

	IntClearEntries();

	PlaylistInfo plsInfo;
	CStringW section;
	CStringW line;

	while (file.ReadString(line)) {
		int pos = 0;

		if (line[0] == '[') { // new section
			if (section.GetLength()) {
				section.Empty();
				IntAddEntry(plsInfo);
				plsInfo = {};
			}

			pos = line.Find(']');
			if (pos > 0) {
				section = line.Mid(1, pos - 1);
			}
			continue;
		}

		if (line[0] == ';' || section.IsEmpty()) {
			continue;
		}

		pos = line.Find('=');
		if (pos > 0 && pos + 1 < line.GetLength()) {
			CStringW param = line.Mid(0, pos).Trim();
			CStringW value = line.Mid(pos + 1).Trim();
			if (value.GetLength()) {
				if (param == L"Type") {
					if (value == "basic") {
						plsInfo.Type = PLS_Basic;
					}
					else if (value == "explorer") {
						plsInfo.Type = PLS_Explorer;
					}
					else if (value == "link") {
						plsInfo.Type = PLS_Link;
					}
				}
				else if (param == L"Path") {
					plsInfo.Path = value;
				}
				else if (param == L"Title") {
					plsInfo.Title = value;
				}
				else if (param == L"CurItem") {
					plsInfo.CurItem = value;
				}
				else if (param == L"CurIndex") {
					int32_t i32val;
					if (StrToInt32(value, i32val) && i32val >= 1) {
						plsInfo.CurIndex = i32val;
					}
				}
				else if (param == L"Sorting") {
					int32_t i32val;
					if (StrToInt32(value, i32val) && i32val >= 1) {
						plsInfo.Sorting = discard<PlaylistSort>((PlaylistSort)i32val, PLS_SORT_None, PLS_SORT_None, PLS_SORT_DateCreated);
					}
				}
			}
		}
	}

	CloseFile(pFile);

	if (section.GetLength()) {
		IntAddEntry(plsInfo);
	}

	return true;
}

bool CPlaylistConfigFile::WriteFile()
{
	FILE* pFile = OpenFileForWrite();
	if (!pFile) {
		return false;
	}

	bool ret = true;

	CStdioFile file(pFile);
	CStringW str;
	try {
		file.WriteString(L"; MPC-BE Playlist Config File 0.1\n");
		int i = 1;
		for (const auto& plsInfo : m_PlaylistInfos) {
			if (plsInfo.Path.GetLength() || plsInfo.Type == PLS_Explorer) {

				CStringW type;
				switch (plsInfo.Type) {
				case PLS_Basic:    type = L"basic";    break;
				case PLS_Explorer: type = L"explorer"; break;
				case PLS_Link:     type = L"link";    break;
				default:
					continue;
				}

				str.Format(L"\n[%02d]\n", i++);

				str.AppendFormat(L"Type=%s\n", type);
				str.AppendFormat(L"Path=%s\n", plsInfo.Path);

				if (plsInfo.Title.GetLength()) {
					str.AppendFormat(L"Title=%s\n", plsInfo.Title);
				}
				if (plsInfo.CurItem.GetLength()) {
					str.AppendFormat(L"CurItem=%s\n", plsInfo.CurItem);
				}
				if (plsInfo.CurIndex > 0) {
					str.AppendFormat(L"CurIndex=%d\n", plsInfo.CurIndex);
				}
				if (plsInfo.Sorting > 0) {
					str.AppendFormat(L"CurIndex=%d\n", plsInfo.CurIndex);
				}

				file.WriteString(str);
			}
		}
	}
	catch (CFileException& e) {
		// Fail silently if disk is full
		UNREFERENCED_PARAMETER(e);
		ASSERT(FALSE);
		ret = false;
	}

	CloseFile(pFile);

	return ret;
}

void CPlaylistConfigFile::AddLostBasicPlaylists()
{
	WIN32_FIND_DATAW wfd;
	HANDLE hFile = FindFirstFileW(m_PlaylistFolder + L"*.mpcpl", &wfd);
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			if (!BasicPlsContains(wfd.cFileName)) {
				CStringW title(wfd.cFileName, wcslen(wfd.cFileName) - 6);
				PlaylistInfo plsInfo = { PLS_Basic, wfd.cFileName, title };
				m_PlaylistInfos.emplace_back(plsInfo);
			}
		} while (FindNextFileW(hFile, &wfd));
		FindClose(hFile);
	}
}

void CPlaylistConfigFile::SetFilename(const CStringW& filename)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	m_filename = filename;
	m_PlaylistFolder = filename.Left(filename.ReverseFind(L'\\')+1) + L"Playlists\\";
}

void CPlaylistConfigFile::OpenPlaylists(std::list<PlaylistInfo>& playlistInfos)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	bool ok = ReadFile();
	if (!ok) {
		IntClearEntries();
	}

	// TODO
	//AddLostBasicPlaylists();

	playlistInfos = m_PlaylistInfos;
}

void CPlaylistConfigFile::SavePlaylists(const std::list<PlaylistInfo>& playlistInfos)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	m_PlaylistInfos = playlistInfos;

	WriteFile();
}
