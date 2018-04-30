/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include "MainFrm.h"
#include "Misc.h"
#include "../../Subtitles/TextFile.h"
#include "WebClient.h"
#include "DIB.h"
#include "../../DSUtil/WinAPIUtils.h"

CWebClientSocket::CWebClientSocket(CWebServer* pWebServer, CMainFrame* pMainFrame)
	: m_pWebServer(pWebServer)
	, m_pMainFrame(pMainFrame)
{
}

CWebClientSocket::~CWebClientSocket()
{
}

bool CWebClientSocket::SetCookie(CString name, CString value, __time64_t expire, CString path, CString domain)
{
	if (name.IsEmpty()) {
		return false;
	}

	if (value.IsEmpty()) {
		m_cookie.RemoveKey(name);
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
	m_hdrlines.RemoveAll();
	m_data.Empty();

	m_cmd.Empty();
	m_path.Empty();
	m_ver.Empty();
	m_get.RemoveAll();
	m_post.RemoveAll();
	m_cookie.RemoveAll();
	m_request.RemoveAll();
}

void CWebClientSocket::Header()
{
	if (m_cmd.IsEmpty()) {
		if (m_hdr.IsEmpty()) {
			return;
		}

		CAtlList<CString> lines;
		Explode(m_hdr, lines, L'\n');
		CString str = lines.RemoveHead();

		CAtlList<CString> sl;
		ExplodeMin(str, sl, ' ', 3);
		m_cmd = sl.RemoveHead().MakeUpper();
		m_path = sl.RemoveHead();
		m_ver = sl.RemoveHead().MakeUpper();
		ASSERT(sl.GetCount() == 0);

		POSITION pos = lines.GetHeadPosition();
		while (pos) {
			Explode(lines.GetNext(pos), sl, L':', 2);
			if (sl.GetCount() == 2) {
				m_hdrlines[sl.GetHead().MakeLower()] = sl.GetTail();
			}
		}
	}

	// remember new cookies

	CString value;
	if (m_hdrlines.Lookup(L"cookie", value)) {
		CAtlList<CString> sl;
		Explode(value, sl, L';');
		POSITION pos = sl.GetHeadPosition();
		while (pos) {
			CAtlList<CString> sl2;
			Explode(sl.GetNext(pos), sl2, L'=', 2);
			m_cookie[sl2.GetHead()] = sl2.GetCount() == 2 ? sl2.GetTail() : L"";
		}
	}

	// start new session

	if (!m_cookie.Lookup(L"MPCSESSIONID", m_sessid)) {
		srand((unsigned int)time(nullptr));
		m_sessid.Format(L"%08x", rand()*0x12345678);
		SetCookie(L"MPCSESSIONID", m_sessid);
	} else {
		// TODO: load session
	}

	CStringA reshdr, resbody;

	if (m_cmd == L"POST") {
		CString str;
		if (m_hdrlines.Lookup(L"content-length", str)) {

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
						CAtlList<CString> sl;
						Explode(str, sl, L'&');
						POSITION pos = sl.GetHeadPosition();
						while (pos) {
							CAtlList<CString> sl2;
							Explode(sl.GetNext(pos), sl2, L'=', 2);
							if (sl2.GetCount() == 2) {
								m_post[sl2.GetHead().MakeLower()] = UTF8To16(UrlDecode(TToA(sl2.GetTail())));
							} else {
								m_post[sl2.GetHead().MakeLower()] = L"";
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
			int k = m_query.Find('#');
			if (k >= 0) {
				m_query.Truncate(k);
			}

			CAtlList<CString> sl;
			Explode(m_query, sl, L'&');
			POSITION pos = sl.GetHeadPosition();
			while (pos) {
				CAtlList<CString> sl2;
				Explode(sl.GetNext(pos), sl2, L'=', 2);
				if (sl2.GetCount() == 2) {
					m_get[sl2.GetHead()] = UTF8To16(UrlDecode(TToA(sl2.GetTail())));
				} else {
					m_get[sl2.GetHead()] = L"";
				}
			}
		}

		// m_request <-- m_get+m_post+m_cookie
		{
			CString key, value;
			POSITION pos;
			pos = m_get.GetStartPosition();
			while (pos) {
				m_get.GetNextAssoc(pos, key, value);
				m_request[key] = value;
			}
			pos = m_post.GetStartPosition();
			while (pos) {
				m_post.GetNextAssoc(pos, key, value);
				m_request[key] = value;
			}
			pos = m_cookie.GetStartPosition();
			while (pos) {
				m_cookie.GetNextAssoc(pos, key, value);
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
			POSITION pos = m_cookie.GetStartPosition();
			while (pos) {
				CString key, value;
				m_cookie.GetNextAssoc(pos, key, value);
				reshdr += "Set-Cookie: " + key + "=" + value;
				POSITION pos2 = m_cookieattribs.GetStartPosition();
				while (pos2) {
					CString key;
					cookie_attribs value;
					m_cookieattribs.GetNextAssoc(pos2, key, value);
					if (!value.path.IsEmpty()) {
						reshdr += "; path=" + value.path;
					}
					if (!value.expire.IsEmpty()) {
						reshdr += "; expire=" + value.expire;
					}
					if (!value.domain.IsEmpty()) {
						reshdr += "; domain=" + value.domain;
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

		CString connection = L"close";
		m_hdrlines.Lookup(L"connection", connection);

		Clear();

		// if (connection == L"close")
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
	CString arg;
	if (m_request.Lookup(L"wm_command", arg)) {
		int id = _wtol(arg);

		if (id > 0) {
			if (id == ID_FILE_EXIT) {
				m_pMainFrame->PostMessageW(WM_COMMAND, id);
			} else {
				m_pMainFrame->SendMessageW(WM_COMMAND, id);
			}
		} else {
			if (arg == CMD_SETPOS && m_request.Lookup(L"position", arg)) {
				int h, m, s, ms = 0;
				WCHAR c;
				if (swscanf_s(arg, L"%d%c%d%c%d%c%d", &h, &c, sizeof(WCHAR),
							   &m, &c, sizeof(WCHAR), &s, &c, sizeof(WCHAR), &ms) >= 5) {
					REFERENCE_TIME rtPos = 10000i64*(((h*60+m)*60+s)*1000+ms);
					m_pMainFrame->SeekTo(rtPos);
					for (int retries = 20; retries-- > 0; Sleep(50)) {
						if (abs((int)((rtPos - m_pMainFrame->GetPos())/10000)) < 100) {
							break;
						}
					}
				}
			} else if (arg == CMD_SETPOS && m_request.Lookup(L"percent", arg)) {
				float percent = 0;
				if (swscanf_s(arg, L"%f", &percent) == 1) {
					m_pMainFrame->SeekTo((REFERENCE_TIME)(percent / 100 * m_pMainFrame->GetDur()));
				}
			} else if (arg == CMD_SETVOLUME && m_request.Lookup(L"volume", arg)) {
				int volume = wcstol(arg, nullptr, 10);
				m_pMainFrame->m_wndToolBar.Volume = std::clamp(volume, 0, 100);
				m_pMainFrame->OnPlayVolume(0);
			}
		}
	}

	CString ref;
	if (!m_hdrlines.Lookup(L"referer", ref)) {
		return true;
	}

	hdr =
		"HTTP/1.1 302 Found\r\n"
		"Location: " + CStringA(ref) + "\r\n";

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
	CPath file(m_pMainFrame->m_wndPlaylistBar.GetCurFileName());
	file.StripPath();
	file.RemoveExtension();

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
	CAtlList<CStringA> rootdrives;
	for (WCHAR drive[] = L"A:"; drive[0] <= 'Z'; drive[0]++)
		if (GetDriveType(drive) != DRIVE_NO_ROOT_DIR) {
			rootdrives.AddTail(CStringA(drive) + '\\');
		}

	// process GET

	CString path;
	CFileStatus fs;
	if (m_get.Lookup(L"path", path)) {

		if (CFileGetStatus(path, fs) && !(fs.m_attribute&CFile::directory)) {

			CAtlList<CString> cmdln;

			cmdln.AddTail(path);

			CString focus;
			if (m_get.Lookup(L"focus", focus) && !focus.CompareNoCase(L"no")) {
				cmdln.AddTail(L"/nofocus");
			}

			int len = 0;

			POSITION pos = cmdln.GetHeadPosition();
			while (pos) {
				CString& str = cmdln.GetNext(pos);
				len += (str.GetLength()+1)*sizeof(WCHAR);
			}

			CAutoVectorPtr<BYTE> buff;
			if (buff.Allocate(4+len)) {
				BYTE* p = buff;
				*(DWORD*)p = cmdln.GetCount();
				p += sizeof(DWORD);

				POSITION pos = cmdln.GetHeadPosition();
				while (pos) {
					CString& str = cmdln.GetNext(pos);
					len = (str.GetLength()+1)*sizeof(WCHAR);
					memcpy(p, (LPCTSTR)str, len);
					p += len;
				}

				COPYDATASTRUCT cds;
				cds.dwData = 0x6ABE51;
				cds.cbData = p - buff;
				cds.lpData = (void*)(BYTE*)buff;
				m_pMainFrame->SendMessageW(WM_COPYDATA, (WPARAM)nullptr, (LPARAM)&cds);
			}

			CPath p(path);
			p.RemoveFileSpec();
			path = (LPCTSTR)p;
		}
	} else {
		path = m_pMainFrame->m_wndPlaylistBar.GetCurFileName();

		if (CFileGetStatus(path, fs) && !(fs.m_attribute&CFile::directory)) {
			CPath p(path);
			p.RemoveFileSpec();
			path = (LPCTSTR)p;
		}
	}

	if (path.Find(L"://") >= 0) {
		path.Empty();
	}

	if (CFileGetStatus(path, fs) && (fs.m_attribute&CFile::directory) || path.Find(L"\\") == 0) {
		CPath p(path);
		p.Canonicalize();
		p.MakePretty();
		p.AddBackslash();
		path = (LPCTSTR)p;
	}

	CStringA files;

	if (path.IsEmpty()) {
		POSITION pos = rootdrives.GetHeadPosition();
		while (pos) {
			CStringA& drive = rootdrives.GetNext(pos);

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
		CString parent;

		if (path.GetLength() > 3) {
			CPath p(path + "..");
			p.Canonicalize();
			p.AddBackslash();
			parent = (LPCTSTR)p;
		}

		files += "<tr class=\"dir\">\r\n";
		files +=
			"<td class=\"dirname\"><a href=\"[path]?path=" + parent + "\">..</a></td>"
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
		CPath p(path);
		p.RemoveFileSpec();
		dir = (LPCTSTR)p;
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
	CString path = m_pMainFrame->m_wndPlaylistBar.GetCurFileName();
	CString dir;

	if (!path.IsEmpty()) {
		CPath p(path);
		p.RemoveFileSpec();
		dir = (LPCTSTR)p;
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

	m_pWebServer->LoadPage(IDR_HTML_VARIABLES, body, m_path);
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

bool CWebClientSocket::OnStatus(CStringA& hdr, CStringA& body, CStringA& mime)
{
	/*
	CString path = m_pMainFrame->m_wndPlaylistBar.GetCur(), dir;
	if (!path.IsEmpty()) {
		CPath p(path);
		p.RemoveFileSpec();
		dir = (LPCTSTR)p;
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

	CPath file(m_pMainFrame->m_wndPlaylistBar.GetCurFileName());

	CString status;// = m_pMainFrame->GetStatusMessage();
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

	return true;
}

bool CWebClientSocket::OnSnapShotJpeg(CStringA& hdr, CStringA& body, CStringA& mime)
{
	bool fRet = false;

	BYTE *jpeg = nullptr;
	long size = 0;
	size_t jpeg_size = 0;

	std::vector<BYTE> dib;
	CString errmsg;
	if (S_OK == m_pMainFrame->GetDisplayedImage(dib, errmsg) || S_OK == m_pMainFrame->GetCurrentFrame(dib, errmsg)) {
		if (BMPDIB(0, dib.data(), L"image/jpeg", AfxGetAppSettings().nWebServerQuality, 1, &jpeg, &jpeg_size)) {
			hdr +=
				"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
				"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
				"Pragma: no-cache\r\n";
			body = CStringA((char*)jpeg, jpeg_size);
			mime = "image/jpeg";
			fRet = true;
		}
	}

	return fRet;
}
