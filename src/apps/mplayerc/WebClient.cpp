/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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
#include <atlutil.h>
#include "MainFrm.h"
#include "Misc.h"
#include "Subtitles/TextFile.h"
#include "WebClient.h"
#include "DIB.h"
#include "DSUtil/FileHandle.h"

#include "Version.h"

CWebClientSocket::CWebClientSocket(CWebServer* pWebServer, CMainFrame* pMainFrame)
	: m_pWebServer(pWebServer)
	, m_pMainFrame(pMainFrame)
{
}

CWebClientSocket::~CWebClientSocket()
{
}

bool CWebClientSocket::SetCookie(const CString& name, const CString& value, __time64_t expire, const CString& path, const CString& domain)
{
	if (name.IsEmpty()) {
		return false;
	}

	if (value.IsEmpty()) {
		m_cookie.erase(name);
		return true;
	}

	m_cookie[name] = value;

	m_cookieattribs[name].path = path;
	m_cookieattribs[name].domain = domain;

	if (expire >= 0) {
		CTime t(expire);
		SYSTEMTIME st;
		t.GetAsSystemTime(st);
		CStringA str;
		SystemTimeToHttpDate(st, str);
		m_cookieattribs[name].expire = str;
	}

	return true;
}

void CWebClientSocket::Clear()
{
	m_hdr.Empty();
	m_hdrlines.clear();
	m_data.Empty();

	m_cmd.Empty();
	m_path.Empty();
	m_ver.Empty();
	m_get.clear();
	m_post.clear();
	m_cookie.clear();
	m_request.clear();
}

void CWebClientSocket::Header()
{
	if (m_cmd.IsEmpty()) {
		if (m_hdr.IsEmpty()) {
			return;
		}

		std::list<CString> lines;
		Explode(m_hdr, lines, L'\n');
		CString str = lines.front();

		std::list<CString> sl;
		ExplodeMin(str, sl, ' ', 3);
		ASSERT(sl.size() == 3);
		if (sl.size() == 3) {
			auto it = sl.begin();
			m_cmd  = (*it++).MakeUpper();
			m_path = (*it++);
			m_ver  = (*it++).MakeUpper();

			lines.pop_front();
			for (const auto& line : lines) {
				Explode(line, sl, L':', 2);
				if (sl.size() == 2) {
					m_hdrlines[sl.front().MakeLower()] = sl.back();
				}
			}
		}
	}

	// remember new cookies

	if (const auto it = m_hdrlines.find(L"cookie"); it != m_hdrlines.end()) {
		CString value = (*it).second;
		std::list<CString> sl;
		Explode(value, sl, L';');

		for (const auto& item : sl) {
			std::list<CString> sl2;
			Explode(item, sl2, L'=', 2);
			m_cookie[sl2.front()] = (sl2.size() == 2) ? sl2.back() : CString();
		}
	}

	// start new session

	if (const auto it = m_cookie.find(L"MPCSESSIONID"); it != m_cookie.end()) {
		m_sessid = (*it).second;
		// TODO: load session
	} else {
		srand((unsigned int)time(nullptr));
		m_sessid.Format(L"%08x", rand()*0x12345678);
		SetCookie(L"MPCSESSIONID", m_sessid);
	}

	CStringA reshdr, resbody;

	if (m_cmd == L"POST") {
		if (const auto it = m_hdrlines.find(L"content-length"); it != m_hdrlines.end()) {
			CString str = (*it).second;
			int len = wcstol(str, nullptr, 10);
			str.Empty();

			int err = 0;
			char c;

			int timeout = 1000;

			do {
				for (; len > 0 && (err = Receive(&c, 1)) > 0; len--) {
					m_data += c;
					if (c == '\r') {
						continue;
					}
					str += c;
					if (c == '\n' || len == 1) {
						std::list<CString> sl;
						Explode(str, sl, L'&');

						for (const auto& item : sl) {
							std::list<CString> sl2;
							Explode(item, sl2, L'=', 2);
							if (sl2.size() == 2) {
								m_post[sl2.front().MakeLower()] = UTF8ToWStr(UrlDecode(TToA(sl2.back())));
							} else {
								m_post[sl2.front().MakeLower()].Empty();
							}
						}
						str.Empty();
					}
				}

				if (err == SOCKET_ERROR) {
					Sleep(1);
				}
			} while (err == SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK && timeout-- > 0);

			Receive(&c, 1);
			Receive(&c, 1);
		}
	}

	if (m_cmd == L"GET" || m_cmd == L"HEAD" || m_cmd == L"POST") {
		int k = m_path.Find('?');
		if (k >= 0) {
			m_query = m_path.Mid(k + 1);
			m_path.Truncate(k);
		}

		if (m_query.GetLength() > 0) {
			k = m_query.Find('#');
			if (k >= 0) {
				m_query.Truncate(k);
			}

			std::list<CString> sl;
			Explode(m_query, sl, L'&');

			for (const auto& item : sl) {
				std::list<CString> sl2;
				Explode(item, sl2, L'=', 2);
				if (sl2.size() == 2) {
					m_get[sl2.front()] = UTF8ToWStr(UrlDecode(TToA(sl2.back())));
				} else {
					m_get[sl2.front()].Empty();
				}
			}
		}

		// m_request <-- m_get+m_post+m_cookie
		{
			for (const auto&[key, value] : m_get) {
				m_request[key] = value;
			}
			for (const auto&[key, value] : m_post) {
				m_request[key] = value;
			}
			for (const auto&[key, value] : m_cookie) {
				m_request[key] = value;
			}
		}

		m_pWebServer->OnRequest(this, reshdr, resbody);
	} else {
		reshdr = "HTTP/1.1 400 Bad Request\r\n";
	}

	if (!reshdr.IsEmpty()) {
		// cookies
		{
			for (const auto&[key, value] : m_cookie) {
				reshdr += "Set-Cookie: " + key + "=" + value;
				for (const auto&[cookiekey, cookieattrib] : m_cookieattribs) {
					if (cookieattrib.path.GetLength()) {
						reshdr += "; path=" + cookieattrib.path;
					}
					if (cookieattrib.expire.GetLength()) {
						reshdr += "; expire=" + cookieattrib.expire;
					}
					if (cookieattrib.domain.GetLength()) {
						reshdr += "; domain=" + cookieattrib.domain;
					}
				}
				reshdr += "\r\n";
			}
		}

		reshdr +=
			"Server: MPC-BE WebServer\r\n"
			"Connection: close\r\n"
			"\r\n";

		Send(reshdr, reshdr.GetLength());

		if (m_cmd != L"HEAD" && reshdr.Find("HTTP/1.1 200 OK") == 0 && !resbody.IsEmpty()) {
			Send(resbody, resbody.GetLength());
		}

		Clear();
		OnClose(0);
	}
}

void CWebClientSocket::OnReceive(int nErrorCode)
{
	if (nErrorCode == 0) {
		char c;
		while (Receive(&c, 1) > 0) {
			if (c == '\r') {
				continue;
			} else {
				m_hdr += c;
			}

			int len = m_hdr.GetLength();
			if (len >= 2 && m_hdr[len-2] == '\n' && m_hdr[len-1] == '\n') {
				Header();
				return;
			}
		}
	}

	__super::OnReceive(nErrorCode);
}

void CWebClientSocket::OnClose(int nErrorCode)
{
	m_pWebServer->OnClose(this);

	__super::OnClose(nErrorCode);
}

bool CWebClientSocket::OnCommand(CStringA& hdr, CStringA& body, CStringA& mime)
{
	if (auto it = m_request.find(L"wm_command"); it != m_request.end()) {
		const int id = _wtol((*it).second);

		switch (id) {
		case CMD_SETPOS:
			if (it = m_request.find(L"position"); it != m_request.end()) {
				int h, m, s, ms = 0;
				if (swscanf_s((*it).second, L"%d:%d:%d.%d", &h, &m, &s, &ms) >= 3) {
					REFERENCE_TIME rtPos = 10000i64*(((h * 60 + m) * 60 + s) * 1000 + ms);
					m_pMainFrame->SeekTo(rtPos);
					for (int retries = 20; retries-- > 0; Sleep(50)) {
						if (abs((int)((rtPos - m_pMainFrame->GetPos()) / 10000)) < 100) {
							break;
						}
					}
				}
			}
			else if (it = m_request.find(L"percent"); it != m_request.end()) {
				double percent;
				if (StrToDouble((*it).second, percent)) {
					m_pMainFrame->SeekTo((REFERENCE_TIME)(percent / 100 * m_pMainFrame->GetDur()));
				}
			}
			break;
		case CMD_SETVOLUME:
			if (it = m_request.find(L"volume"); it != m_request.end()) {
				int volume;
				if (StrToInt32((*it).second, volume)) {
					m_pMainFrame->m_wndToolBar.Volume = std::clamp(volume, 0, 100);
					m_pMainFrame->OnPlayVolume(0);
				}
			}
			break;
		case ID_FILE_EXIT:
			m_pMainFrame->PostMessageW(WM_COMMAND, id);
			break;
		default:
			if (id > 0) {
				m_pMainFrame->SendMessageW(WM_COMMAND, id);
			}
			break;
		}
	}

	if (const auto it = m_hdrlines.find(L"referer"); it != m_hdrlines.end()) {
		hdr = "HTTP/1.1 302 Found\r\nLocation: " + CStringA((*it).second) + "\r\n";
	}

	return true;
}

bool CWebClientSocket::OnIndex(CStringA& hdr, CStringA& body, CStringA& mime)
{
	CStringA wmcoptions;

	// generate page

	CAppSettings& s = AfxGetAppSettings();

	for (const auto& wc : s.wmcmds) {
		CStringA str;
		str.Format("%d", wc.cmd);
		CStringA valueName(UTF8(wc.GetName()));
		valueName.Replace("&", "&amp;");
		wmcoptions += "<option value=\"" + str + "\">" + valueName + "</option>\r\n";
	}

	m_pWebServer->LoadPage(IDR_HTML_INDEX, body, m_path);
	body.Replace("[wmcoptions]", wmcoptions);

	return true;
}

bool CWebClientSocket::OnInfo(CStringA& hdr, CStringA& body, CStringA& mime)
{
	int pos = (int)(m_pMainFrame->GetPos()/10000);
	int dur = (int)(m_pMainFrame->GetDur()/10000);

	CString positionstring, durationstring, versionstring, sizestring;
	versionstring.Format(L"%s", MPC_VERSION_WSTR);

	CStringW file = GetFileName(m_pMainFrame->m_wndPlaylistBar.GetCurFileName());
	RemoveFileExt(file);

	positionstring.Format(L"%02d:%02d:%02d", (pos/3600000), (pos/60000)%60, (pos/1000)%60);
	durationstring.Format(L"%02d:%02d:%02d", (dur/3600000), (dur/60000)%60, (dur/1000)%60);

	WIN32_FIND_DATAW wfd;
	HANDLE hFind = FindFirstFileW(m_pMainFrame->m_wndPlaylistBar.GetCurFileName(), &wfd);
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
		__int64 size = (__int64(wfd.nFileSizeHigh)<<32)|wfd.nFileSizeLow;
		const int MAX_FILE_SIZE_BUFFER = 65;
		WCHAR szFileSize[MAX_FILE_SIZE_BUFFER];
		StrFormatByteSizeW(size, szFileSize, MAX_FILE_SIZE_BUFFER);
		sizestring.Format(L"%s", szFileSize);
	}

	m_pWebServer->LoadPage(IDR_HTML_INFO, body, m_path);
	body.Replace("[version]", UTF8(versionstring));
	body.Replace("[file]", UTF8(file));
	body.Replace("[position]", UTF8(positionstring));
	body.Replace("[duration]", UTF8(durationstring));
	body.Replace("[size]", UTF8(sizestring));

	return true;
}

bool CWebClientSocket::OnBrowser(CStringA& hdr, CStringA& body, CStringA& mime)
{
	std::list<CStringA> rootdrives;
	for (WCHAR drive[] = L"A:"; drive[0] <= 'Z'; drive[0]++)
		if (GetDriveTypeW(drive) != DRIVE_NO_ROOT_DIR) {
			rootdrives.emplace_back(CStringA(drive) + '\\');
		}

	// process GET

	CString path;
	CFileStatus fs;
	if (auto it = m_get.find(L"path"); it != m_get.end()) {
		path = (*it).second;

		if (CFileGetStatus(path, fs) && !(fs.m_attribute&CFile::directory)) {

			std::list<CString> cmdln;

			cmdln.emplace_back(path);

			if (it = m_get.find(L"focus"); it != m_get.end() && (*it).second.CompareNoCase(L"no") == 0) {
				cmdln.emplace_back(L"/nofocus");
			}

			int len = 0;

			for (const auto& str : cmdln) {
				len += (str.GetLength()+1)*sizeof(WCHAR);
			}

			std::unique_ptr<BYTE[]> buff(new(std::nothrow) BYTE[4 + len]);
			if (buff) {
				BYTE* p = buff.get();
				*(DWORD*)p = cmdln.size();
				p += sizeof(DWORD);

				for (const auto& str : cmdln) {
					len = (str.GetLength()+1)*sizeof(WCHAR);
					memcpy(p, (LPCWSTR)str, len);
					p += len;
				}

				COPYDATASTRUCT cds;
				cds.dwData = 0x6ABE51;
				cds.cbData = p - buff.get();
				cds.lpData = (void*)buff.get();
				m_pMainFrame->SendMessageW(WM_COPYDATA, (WPARAM)nullptr, (LPARAM)&cds);
			}

			RemoveFileSpec(path);
		}
	} else {
		path = m_pMainFrame->m_wndPlaylistBar.GetCurFileName();

		if (CFileGetStatus(path, fs) && !(fs.m_attribute&CFile::directory)) {
			RemoveFileSpec(path);
		}
	}

	if (path.Find(L"://") >= 0) {
		path.Empty();
	}

	if (CFileGetStatus(path, fs) && (fs.m_attribute&CFile::directory) || path.Find(L"\\") == 0) {
		path = GetFullCannonFilePath(path);
		::PathMakePrettyW(path.GetBuffer());
		AddSlash(path);
	}

	CStringA files;

	if (path.IsEmpty()) {
		for (const auto& drive : rootdrives) {
			files += "<tr class=\"dir\">\r\n";
			files +=
				"<td class=\"dirname\"><a href=\"[path]?path=" + UrlEncode(drive) + "\">" + drive + "</a></td>"
				"<td class=\"dirtype\">Directory</td>"
				"<td class=\"dirsize\">&nbsp;</td>\r\n"
				"<td class=\"dirdate\">&nbsp;</td>";
			files += "</tr>\r\n";
		}

		path = "Root";
	} else {
		CStringW parent;

		if (path.GetLength() > 3) {
			parent = GetFullCannonFilePath(path + "..");
			AddSlash(parent);
		}

		files += "<tr class=\"dir\">\r\n";
		files +=
			"<td class=\"dirname\"><a href=\"[path]?path=" + UTF8Arg(parent) + "\">..</a></td>"
			"<td class=\"dirtype\">Directory</td>"
			"<td class=\"dirsize\">&nbsp;</td>\r\n"
			"<td class=\"dirdate\">&nbsp;</td>";
		files += "</tr>\r\n";

		WIN32_FIND_DATAW fd = {0};

		HANDLE hFind = FindFirstFileW(path + "*.*", &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.') {
					continue;
				}

				CString fullpath = path + fd.cFileName;

				files += "<tr class=\"dir\">\r\n";
				files +=
					"<td class=\"dirname\"><a href=\"[path]?path=" + UTF8Arg(fullpath) + "\">" + UTF8(fd.cFileName) + "</a></td>"
					"<td class=\"dirtype\">Directory</td>"
					"<td class=\"dirsize\">&nbsp;</td>\r\n"
					"<td class=\"dirdate\"><span class=\"nobr\">" + CStringA(CTime(fd.ftLastWriteTime).Format(L"%Y.%m.%d %H:%M")) + "</span></td>";
				files += "</tr>\r\n";
			} while (FindNextFileW(hFind, &fd));

			FindClose(hFind);
		}

		hFind = FindFirstFileW(path + "*.*", &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
					continue;
				}

				CString fullpath = path + fd.cFileName;
				WCHAR *ext = wcsrchr(fd.cFileName, '.');
				if (ext != nullptr) {
					ext++;
				}

				CStringA size;
				size.Format("%I64dK", ((UINT64)fd.nFileSizeHigh<<22)|(fd.nFileSizeLow>>10));

				CString type(L"&nbsp;");
				LoadType(fullpath, type);

				if (ext != nullptr) {
					files += "<tr class=\"" + UTF8(ext) + "\">\r\n";
				} else {
					files += "<tr class=\"noext\">\r\n";
				}
				files +=
					"<td class=\"filename\"><a href=\"[path]?path=" + UTF8Arg(fullpath) + "\">" + UTF8(fd.cFileName) + "</a></td>"
					"<td class=\"filetype\"><span class=\"nobr\">" + UTF8(type) + "</span></td>"
					"<td class=\"filesize\" align=\"right\"><span class=\"nobr\">" + size + "</span></td>\r\n"
					"<td class=\"filedate\"><span class=\"nobr\">" + CStringA(CTime(fd.ftLastWriteTime).Format(L"%Y.%m.%d %H:%M")) + "</span></td>";
				files += "</tr>\r\n";
			} while (FindNextFileW(hFind, &fd));

			FindClose(hFind);
		}
	}

	m_pWebServer->LoadPage(IDR_HTML_BROWSER, body, m_path);
	body.Replace("[currentdir]", UTF8(path));
	body.Replace("[currentfiles]", files);

	return true;
}

bool CWebClientSocket::OnControls(CStringA& hdr, CStringA& body, CStringA& mime)
{
	CString path = m_pMainFrame->m_wndPlaylistBar.GetCurFileName();
	CString dir;

	if (!path.IsEmpty()) {
		dir = GetFolderPath(path);
	}

	OAFilterState fs = m_pMainFrame->GetMediaState();
	CString state;
	state.Format(L"%d", fs);
	CString statestring;
	switch (fs) {
		case State_Stopped:
			statestring = ResStr(IDS_CONTROLS_STOPPED);
			break;
		case State_Paused:
			statestring = ResStr(IDS_CONTROLS_PAUSED);
			break;
		case State_Running:
			statestring = ResStr(IDS_CONTROLS_PLAYING);
			break;
		default:
			statestring = L"n/a";
			break;
	}

	int pos = (int)(m_pMainFrame->GetPos()/10000);
	int dur = (int)(m_pMainFrame->GetDur()/10000);

	CString position, duration;
	position.Format(L"%d", pos);
	duration.Format(L"%d", dur);

	CString positionstring, durationstring, playbackrate;
	//positionstring.Format(L"%02d:%02d:%02d.%03d", (pos/3600000), (pos/60000)%60, (pos/1000)%60, pos%1000);
	//durationstring.Format(L"%02d:%02d:%02d.%03d", (dur/3600000), (dur/60000)%60, (dur/1000)%60, dur%1000);
	positionstring.Format(L"%02d:%02d:%02d", (pos/3600000), (pos/60000)%60, (pos/1000)%60);
	durationstring.Format(L"%02d:%02d:%02d", (dur/3600000), (dur/60000)%60, (dur/1000)%60);
	playbackrate = L"1";

	CString volumelevel, muted;
	volumelevel.Format(L"%d", m_pMainFrame->m_wndToolBar.m_volctrl.GetPos());
	muted.Format(L"%d", m_pMainFrame->m_wndToolBar.Volume == -10000 ? 1 : 0);

	CString reloadtime(L"0");

	m_pWebServer->LoadPage(IDR_HTML_CONTROLS, body, m_path);
	body.Replace("[filepatharg]", UTF8Arg(path));
	body.Replace("[filepath]", UTF8(path));
	body.Replace("[filedirarg]", UTF8Arg(dir));
	body.Replace("[filedir]", UTF8(dir));
	body.Replace("[state]", UTF8(state));
	body.Replace("[statestring]", UTF8(statestring));
	body.Replace("[position]", UTF8(position));
	body.Replace("[positionstring]", UTF8(positionstring));
	body.Replace("[duration]", UTF8(duration));
	body.Replace("[durationstring]", UTF8(durationstring));
	body.Replace("[volumelevel]", UTF8(volumelevel));
	body.Replace("[muted]", UTF8(muted));
	body.Replace("[playbackrate]", UTF8(playbackrate));
	body.Replace("[reloadtime]", UTF8(reloadtime));

	return true;
}

bool CWebClientSocket::OnVariables(CStringA& hdr, CStringA& body, CStringA& mime)
{
	const CString path = m_pMainFrame->m_wndPlaylistBar.GetCurFileName();
	CString dir;
	CString file;
	CString sizestring;

	if (!path.IsEmpty()) {
		dir = GetFolderPath(path);
		file = GetFileName(path);

		WIN32_FIND_DATAW wfd;
		HANDLE hFind = FindFirstFileW(path, &wfd);
		if (hFind != INVALID_HANDLE_VALUE) {
			FindClose(hFind);
			__int64 size = (__int64(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;
			const int MAX_FILE_SIZE_BUFFER = 65;
			WCHAR szFileSize[MAX_FILE_SIZE_BUFFER];
			StrFormatByteSizeW(size, szFileSize, MAX_FILE_SIZE_BUFFER);
			sizestring.Format(L"%s", szFileSize);
		}
	}

	OAFilterState fs = m_pMainFrame->GetMediaState();
	CString state;
	state.Format(L"%d", fs);
	CString statestring;
	switch (fs) {
		case State_Stopped:
			statestring = ResStr(IDS_CONTROLS_STOPPED);
			break;
		case State_Paused:
			statestring = ResStr(IDS_CONTROLS_PAUSED);
			break;
		case State_Running:
			statestring = ResStr(IDS_CONTROLS_PLAYING);
			break;
		default:
			statestring = L"n/a";
			break;
	}

	int pos = (int)(m_pMainFrame->GetPos()/10000);
	int dur = (int)(m_pMainFrame->GetDur()/10000);

	CString position, duration;
	position.Format(L"%d", pos);
	duration.Format(L"%d", dur);

	CString positionstring, durationstring, playbackrate;
	//positionstring.Format(L"%02d:%02d:%02d.%03d", (pos/3600000), (pos/60000)%60, (pos/1000)%60, pos%1000);
	//durationstring.Format(L"%02d:%02d:%02d.%03d", (dur/3600000), (dur/60000)%60, (dur/1000)%60, dur%1000);
	positionstring.Format(L"%02d:%02d:%02d", (pos/3600000), (pos/60000)%60, (pos/1000)%60);
	durationstring.Format(L"%02d:%02d:%02d", (dur/3600000), (dur/60000)%60, (dur/1000)%60);
	playbackrate = L"1";

	CString volumelevel, muted;
	volumelevel.Format(L"%d", m_pMainFrame->m_wndToolBar.m_volctrl.GetPos());
	muted.Format(L"%d", m_pMainFrame->m_wndToolBar.Volume == -10000 ? 1 : 0);

	CString reloadtime(L"0"); // TODO

	CString HDR(L"n/a");
	if (fs != -1) {
		BeginEnumFilters(m_pMainFrame->m_pGB, pEF, pBF) {
			// Checks if any Video Renderer is in the graph
			if (IsVideoRenderer(pBF)) {
				BeginEnumPins(pBF, pEP, pPin) {
					CMediaType mt;
					if (SUCCEEDED(pPin->ConnectionMediaType(&mt))) {
						if (mt.majortype == MEDIATYPE_Video && (mt.formattype == FORMAT_VideoInfo2 || mt.formattype == FORMAT_MPEG2_VIDEO) && mt.pbFormat) {
							const auto vih2 = (VIDEOINFOHEADER2*)mt.pbFormat;
							HDR = L"Unknown";
							if (vih2->dwControlFlags & (AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT)) {
								DXVA2_ExtendedFormat exfmt;
								exfmt.value = vih2->dwControlFlags;
								switch (exfmt.VideoTransferFunction) {
									case MFVideoTransFunc_2084: HDR = L"HDR";      break;
									case MFVideoTransFunc_HLG:  HDR = L"HDR(HLG)"; break;
									default:                    HDR = L"SDR";      break;
								}
							}

							break;
						}
					}
				}
				EndEnumPins;

				break;
			}
		}
		EndEnumFilters;
	}

	m_pWebServer->LoadPage(IDR_HTML_VARIABLES, body, m_path);
	body.Replace("[file]", UTF8(file));
	body.Replace("[filepatharg]", UTF8Arg(path));
	body.Replace("[filepath]", UTF8(path));
	body.Replace("[filedirarg]", UTF8Arg(dir));
	body.Replace("[filedir]", UTF8(dir));
	body.Replace("[state]", UTF8(state));
	body.Replace("[statestring]", UTF8(statestring));
	body.Replace("[position]", UTF8(position));
	body.Replace("[positionstring]", UTF8(positionstring));
	body.Replace("[duration]", UTF8(duration));
	body.Replace("[durationstring]", UTF8(durationstring));
	body.Replace("[volumelevel]", UTF8(volumelevel));
	body.Replace("[muted]", UTF8(muted));
	body.Replace("[playbackrate]", UTF8(playbackrate));
	body.Replace("[size]", UTF8(sizestring));
	body.Replace("[reloadtime]", UTF8(reloadtime));
	body.Replace("[hdr]", UTF8(HDR));
	body.Replace("[version]", UTF8(MPC_VERSION_FULL_WSTR));

	return true;
}

bool CWebClientSocket::OnStatus(CStringA& hdr, CStringA& body, CStringA& mime)
{
	/*
	CString path = m_pMainFrame->m_wndPlaylistBar.GetCur(), dir;
	if (!path.IsEmpty()) {
		dir = GetFolderOnly(path);
	}
	path.Replace(L"'", L"\\'");
	dir.Replace(L"'", L"\\'");

	CString volumelevel, muted;
	volumelevel.Format(L"%d", m_pMainFrame->m_wndToolBar.m_volctrl.GetPos());
	muted.Format(L"%d", m_pMainFrame->m_wndToolBar.Volume == -10000 ? 1 : 0);
	body.Replace("[volumelevel]", UTF8(volumelevel));
	body.Replace("[muted]", UTF8(muted));
	*/

	CString title;
	m_pMainFrame->GetWindowTextW(title);

	CStringW file(m_pMainFrame->m_wndPlaylistBar.GetCurFileName());

	CString status;
	OAFilterState fs = m_pMainFrame->GetMediaState();
	switch (fs) {
		case State_Stopped:
			status = ResStr(IDS_CONTROLS_STOPPED);
			break;
		case State_Paused:
			status = ResStr(IDS_CONTROLS_PAUSED);
			break;
		case State_Running:
			status = ResStr(IDS_CONTROLS_PLAYING);
			break;
		default:
			status = L"n/a";
			break;
	}

	int pos = (int)(m_pMainFrame->GetPos()/10000);
	int dur = (int)(m_pMainFrame->GetDur()/10000);

	CString posstr, durstr;
	posstr.Format(L"%02d:%02d:%02d", (pos/3600000), (pos/60000)%60, (pos/1000)%60);
	durstr.Format(L"%02d:%02d:%02d", (dur/3600000), (dur/60000)%60, (dur/1000)%60);

	title.Replace(L"'", L"\\'");
	status.Replace(L"'", L"\\'");

	body.Format("OnStatus('%s', '%s', %d, '%s', %d, '%s', %d, %d, '%s')", // , '%s'
				UTF8(title), UTF8(status),
				pos, UTF8(posstr), dur, UTF8(durstr),
				m_pMainFrame->IsMuted(), m_pMainFrame->GetVolume(),
				UTF8(file)/*, UTF8(dir)*/);

	return true;
}

bool CWebClientSocket::OnError404(CStringA& hdr, CStringA& body, CStringA& mime)
{
	m_pWebServer->LoadPage(IDR_HTML_404, body, m_path);

	return true;
}

bool CWebClientSocket::OnPlayer(CStringA& hdr, CStringA& body, CStringA& mime)
{
	m_pWebServer->LoadPage(IDR_HTML_PLAYER, body, m_path);
	if (AfxGetAppSettings().bWebUIEnablePreview) {
		body.Replace("[preview]",
					 "<img src=\"logo.png\" id=\"snapshot\" alt=\"snapshot\" onload=\"OnLoadSnapShot()\" onabort=\"OnAbortErrorSnapShot()\" onerror=\"OnAbortErrorSnapShot()\">");
	} else {
		body.Replace("[preview]", UTF8(ResStr(IDS_WEBUI_DISABLED_PREVIEW_MSG)));
	}
	return true;
}

bool CWebClientSocket::OnSnapShotJpeg(CStringA& hdr, CStringA& body, CStringA& mime)
{
	if (!AfxGetAppSettings().bWebUIEnablePreview) {
		hdr = "HTTP/1.0 403 Forbidden\r\n";
		return true;
	}

	CSimpleBlock<BYTE> dib;
	CString errmsg;

	if (S_OK == m_pMainFrame->GetDisplayedImage(dib, errmsg) || S_OK == m_pMainFrame->GetCurrentFrame(dib, errmsg)) {
		const int maxJpegSize = std::max<int>(32*KILOBYTE, dib.Size());
		if (body.GetAllocLength() < maxJpegSize) {
			body.Preallocate(maxJpegSize);
		}
		size_t dstLen = body.GetAllocLength();

		if (SaveDIB_WIC(L".jpg", dib.Data(), AfxGetAppSettings().nWebServerQuality, (BYTE*)body.GetBuffer(), dstLen)) {
			std::ignore = body.GetBufferSetLength(dstLen);

			hdr +=
				"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
				"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
				"Pragma: no-cache\r\n";
			mime = "image/jpeg";

			return true;
		}
	}

	return false;
}
