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

#include "stdafx.h"
#include "HistoryFile.h"

std::list<SessionInfo_t>::iterator CHistoryFile::FindSessionInfo(SessionInfo_t& sesInfo)
{
	if (sesInfo.DVDId) {
		for (auto it = m_SessionInfos.begin(); it != m_SessionInfos.end(); ++it) {
			if (sesInfo.DVDId == (*it).DVDId) {
				return it;
			}
		}
	}
	else if (sesInfo.Path.GetLength()) {
		for (auto it = m_SessionInfos.begin(); it != m_SessionInfos.end(); ++it) {
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

bool CHistoryFile::ReadFile()
{
	if (!::PathFileExistsW(m_filename)) {
		return false;
	}

	FILE* fp;
	int fpStatus;
	do { // Open mpc-be.ini in UNICODE mode, retry if it is already being used by another process
		fp = _wfsopen(m_filename, L"r, ccs=UNICODE", _SH_SECURE);
		if (fp || (GetLastError() != ERROR_SHARING_VIOLATION)) {
			break;
		}
		Sleep(100);
	} while (true);
	if (!fp) {
		ASSERT(0);
		return false;;
	}

	CStdioFile file(fp);

	m_SessionInfos.clear();

	SessionInfo_t sesInfo;
	CStringW section;
	CStringW line;

	while (file.ReadString(line)) {
		int pos = 0;

		if (line[0] == '[') { // new section
			if (section.GetLength()) {
				section.Empty();
				if (sesInfo.Path.GetLength() || sesInfo.DVDId) {
					m_SessionInfos.push_back(sesInfo);
				}
			}
			pos = line.Find(']');
			if (pos > 0) {
				section = line.Mid(1, pos - 1);
				sesInfo = {};
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
				if (param == "Path") {
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
					ULONG dvdTitle;
					int h, m, s;
					if (swscanf_s(value, L"%ul,%02d:%02d:%02d", &dvdTitle, &h, &m, &s) == 4) {
						sesInfo.DVDTitle = dvdTitle;
						sesInfo.DVDTimecode = { (BYTE)h, (BYTE)m, (BYTE)s, 0 };
					}
				}
				else if (param == L"AudioNum") {
					StrToInt32(value, sesInfo.AudioNum);
				}
				else if (param == L"SubtitleNum") {
					StrToInt32(value, sesInfo.SubtitleNum);
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

	fpStatus = fclose(fp);
	ASSERT(fpStatus == 0);

	if (sesInfo.Path.GetLength() || sesInfo.DVDId) {
		m_SessionInfos.push_back(sesInfo);
	}

	return true;
}

bool CHistoryFile::WriteFile()
{
	FILE* fp;
	int fpStatus;
	do { // Open mpc-be.ini, retry if it is already being used by another process
		fp = _wfsopen(m_filename, L"w, ccs=UTF-8", _SH_SECURE);
		if (fp || (GetLastError() != ERROR_SHARING_VIOLATION)) {
			break;
		}
		Sleep(100);
	} while (true);
	if (!fp) {
		ASSERT(FALSE);
		return false;
	}

	bool ret = true;

	CStdioFile file(fp);
	CStringW str;
	try {
		file.WriteString(L"; MPC-BE history file\n");
		int i = 1;
		for (const auto& sesInfo : m_SessionInfos) {
			str.Format(L"[%03d]\n", i++);

			if (sesInfo.Title.GetLength()) {
				str.AppendFormat(L"Title=%s\n", sesInfo.Title);
			}

			if (sesInfo.DVDId) {
				// We do not write the path here, because it may be the same for different DVDs.
				// The path must be recorded in a separate section to be displayed in the recent files menu.
				str.AppendFormat(L"DVDId=%016x\n", sesInfo.DVDId);

				if (sesInfo.DVDTitle) {
					str.AppendFormat(L"DVDPosition=%ul,%02d:%02d:%02d\n",
						sesInfo.DVDTitle,
						sesInfo.DVDTimecode.bHours,
						sesInfo.DVDTimecode.bMinutes,
						sesInfo.DVDTimecode.bSeconds);
				}
			}
			else if (sesInfo.Path.GetLength()) {
				str.AppendFormat(L"Path=%s\n", sesInfo.Path);

				if (sesInfo.Position > UNITS) {
					LONGLONG seconds = sesInfo.Position / UNITS;
					int h = (int)(seconds / 3600);
					int m = (int)(seconds / 60 % 60);
					int s = (int)(seconds % 60);
					str.AppendFormat(L"Position=%02d:%02d:%02d\n", h, m, s);
				}
				if (sesInfo.AudioNum >= 0) {
					str.AppendFormat(L"AudioNum=%d\n", sesInfo.AudioNum);
				}
				if (sesInfo.SubtitleNum >= 0) {
					str.AppendFormat(L"SubtitleNum=%d\n", sesInfo.SubtitleNum);
				}
				if (sesInfo.AudioPath.GetLength()) {
					str.AppendFormat(L"AudioPath=%s\n", sesInfo.AudioPath);
				}
				if (sesInfo.SubtitlePath.GetLength()) {
					str.AppendFormat(L"SubtitlePath=%d\n", sesInfo.SubtitlePath);
				}
			}
			file.WriteString(str);
		}
	}
	catch (CFileException& e) {
		// Fail silently if disk is full
		UNREFERENCED_PARAMETER(e);
		ASSERT(FALSE);
		ret = false;
	}

	fpStatus = fclose(fp);
	ASSERT(fpStatus == 0);

	return ret;
}

void CHistoryFile::SetFilename(CStringW& filename)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	m_filename = filename;
}

bool CHistoryFile::Clear()
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	if (_wremove(m_filename) == 0) {
		m_SessionInfos.clear();
		return true;
	}
	return false;
}

bool CHistoryFile::GetSessionInfo(SessionInfo_t& sesInfo)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	ReadFile();

	bool found = false;
	auto it = FindSessionInfo(sesInfo);

	if (it != m_SessionInfos.end()) {
		auto& si = (*it);

		if (sesInfo.DVDId) {
			sesInfo.DVDTitle    = si.DVDTitle;
			sesInfo.DVDTimecode = si.DVDTimecode;
		}
		else if (sesInfo.Path.GetLength()) {
			sesInfo.Position    = si.Position;
			sesInfo.AudioNum    = si.AudioNum;
			sesInfo.SubtitleNum = si.SubtitleNum;
		}

		m_SessionInfos.erase(it);

		found = true;
	}

	m_SessionInfos.emplace_front(sesInfo);

	while (m_SessionInfos.size() > 100) {
		m_SessionInfos.pop_back();
	}

	WriteFile();

	return found;
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

void CHistoryFile::SetSessionInfo(SessionInfo_t& sesInfo)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	ReadFile();

	auto it = FindSessionInfo(sesInfo);

	if (it != m_SessionInfos.end()) {
		m_SessionInfos.erase(it);
	}

	m_SessionInfos.emplace_front(sesInfo);

	while (m_SessionInfos.size() > 100) {
		m_SessionInfos.pop_back();
	}

	WriteFile();
}
