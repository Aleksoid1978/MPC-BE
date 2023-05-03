/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
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
#include <ExtLib/zlib/zlib.h>
#include "DSUtil/FileHandle.h"
#include "WebServer.h"
#include "WebClient.h"

CWebServerSocket::CWebServerSocket(CWebServer* pWebServer, int port)
	: m_pWebServer(pWebServer)
{
	Create(port);
	Listen();
}

CWebServerSocket::~CWebServerSocket()
{
}

void CWebServerSocket::OnAccept(int nErrorCode)
{
	if (nErrorCode == 0 && m_pWebServer) {
		m_pWebServer->OnAccept(this);
	}

	__super::OnAccept(nErrorCode);
}

const std::map<CString, CWebServer::RequestHandler> CWebServer::m_internalpages = {
	{ L"/",               &CWebClientSocket::OnIndex        },
	{ L"/index.html",     &CWebClientSocket::OnIndex        },
	{ L"/info.html",      &CWebClientSocket::OnInfo         },
	{ L"/browser.html",   &CWebClientSocket::OnBrowser      },
	{ L"/controls.html",  &CWebClientSocket::OnControls     },
	{ L"/command.html",   &CWebClientSocket::OnCommand      },
	{ L"/status.html",    &CWebClientSocket::OnStatus       },
	{ L"/player.html",    &CWebClientSocket::OnPlayer       },
	{ L"/variables.html", &CWebClientSocket::OnVariables    },
	{ L"/snapshot.jpg",   &CWebClientSocket::OnSnapShotJpeg },
	{ L"/404.html",       &CWebClientSocket::OnError404     },
};

const std::map<CString, UINT> CWebServer::m_downloads = {
	{L"/default.css",                  IDF_DEFAULT_CSS},
	{L"/favicon.png",                  IDF_FAVICON},
	{L"/vbg.png",                      IDF_VBR_PNG},
	{L"/vbs.png",                      IDF_VBS_PNG},
	{L"/sliderbar.gif",                IDF_SLIDERBAR_GIF},
	{L"/slidergrip.gif",               IDF_SLIDERGRIP_GIF},
	{L"/sliderback.png",               IDF_SLIDERBACK_PNG},
	{L"/1pix.gif",                     IDF_1PIX_GIF},
	{L"/headericon.png",               IDF_HEADERICON_PNG},
	{L"/headerback.png",               IDF_HEADERBACK_PNG},
	{L"/headerclose.png",              IDF_HEADERCLOSE_PNG},
	{L"/leftside.png",                 IDF_LEFTSIDE_PNG},
	{L"/rightside.png",                IDF_RIGHTSIDE_PNG},
	{L"/bottomside.png",               IDF_BOTTOMSIDE_PNG},
	{L"/leftbottomside.png",           IDF_LEFTBOTTOMSIDE_PNG},
	{L"/rightbottomside.png",          IDF_RIGHTBOTTOMSIDE_PNG},
	{L"/seekbarleft.png",              IDF_SEEKBARLEFT_PNG},
	{L"/seekbarmid.png",               IDF_SEEKBARMID_PNG},
	{L"/seekbarright.png",             IDF_SEEKBARRIGHT_PNG},
	{L"/seekbargrip.png",              IDF_SEEKBARGRIP_PNG},
	{L"/logo.png",                     IDF_LOGO1},
	{L"/controlback.png",              IDF_CONTROLBACK_PNG},
	{L"/controlbuttonplay.png",        IDF_CONTROLBUTTONPLAY_PNG},
	{L"/controlbuttonpause.png",       IDF_CONTROLBUTTONPAUSE_PNG},
	{L"/controlbuttonstop.png",        IDF_CONTROLBUTTONSTOP_PNG},
	{L"/controlbuttonskipback.png",    IDF_CONTROLBUTTONSKIPBACK_PNG},
	{L"/controlbuttondecrate.png",     IDF_CONTROLBUTTONDECRATE_PNG},
	{L"/controlbuttonincrate.png",     IDF_CONTROLBUTTONINCRATE_PNG},
	{L"/controlbuttonskipforward.png", IDF_CONTROLBUTTONSKIPFORWARD_PNG},
	{L"/controlbuttonstep.png",        IDF_CONTROLBUTTONSTEP_PNG},
	{L"/controlvolumeon.png",          IDF_CONTROLVOLUMEON_PNG},
	{L"/controlvolumeoff.png",         IDF_CONTROLVOLUMEOFF_PNG},
	{L"/controlvolumebar.png",         IDF_CONTROLVOLUMEBAR_PNG},
	{L"/controlvolumegrip.png",        IDF_CONTROLVOLUMEGRIP_PNG},
};

std::map<CStringA, CStringA> CWebServer::m_mimes;

CWebServer::CWebServer(CMainFrame* pMainFrame, int nPort)
	: m_pMainFrame(pMainFrame)
	, m_nPort(nPort)
{
	CRegKey key;
	CString str(L"MIME\\Database\\Content Type");
	if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, str, KEY_READ)) {
		WCHAR buff[256];
		DWORD len = std::size(buff);
		for (int i = 0; ERROR_SUCCESS == key.EnumKey(i, buff, &len); i++, len = std::size(buff)) {
			CRegKey mime;
			WCHAR ext[64];
			ULONG extlen = std::size(ext);
			if (ERROR_SUCCESS == mime.Open(HKEY_CLASSES_ROOT, str + L"\\" + buff, KEY_READ)
					&& ERROR_SUCCESS == mime.QueryStringValue(L"Extension", ext, &extlen)) {
				m_mimes[CStringA(ext).MakeLower()] = CStringA(buff).MakeLower();
			}
		}
	}

	m_mimes[".html"] = "text/html";
	m_mimes[".txt"] = "text/plain";
	m_mimes[".css"] = "text/css";
	m_mimes[".gif"] = "image/gif";
	m_mimes[".jpeg"] = "image/jpeg";
	m_mimes[".jpg"] = "image/jpeg";
	m_mimes[".png"] = "image/png";
	m_mimes[".js"] = "text/javascript";

	m_webroot = CPath(GetProgramDir());

	CString WebRoot = AfxGetAppSettings().strWebRoot;
	WebRoot.Replace('/', '\\');
	WebRoot.Trim();
	CPath p(WebRoot);
	if (WebRoot.Find(L":\\") < 0 && WebRoot.Find(L"\\\\") < 0) {
		m_webroot.Append(WebRoot);
	} else {
		m_webroot = p;
	}
	m_webroot.Canonicalize();
	m_webroot.MakePretty();
	if (!m_webroot.IsDirectory()) {
		m_webroot = CPath();
	}

	std::list<CString> sl;
	Explode(AfxGetAppSettings().strWebServerCGI, sl, L';');
	for (const auto& item : sl) {
		std::list<CString> sl2;
		CString ext = Explode(item, sl2, L'=', 2);
		if (sl2.size() < 2) {
			continue;
		}
		m_cgi[ext] = sl2.back();
	}

	m_ThreadId = 0;
	m_hThread = ::CreateThread(nullptr, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);
}

CWebServer::~CWebServer()
{
	if (m_hThread != nullptr) {
		PostThreadMessageW(m_ThreadId, WM_QUIT, 0, 0);
		if (WaitForSingleObject(m_hThread, 10000) == WAIT_TIMEOUT) {
			TerminateThread (m_hThread, 0xDEAD);
		}
		EXECUTE_ASSERT(CloseHandle(m_hThread));
	}
}

DWORD WINAPI CWebServer::StaticThreadProc(LPVOID lpParam)
{
	return static_cast<CWebServer*>(lpParam)->ThreadProc();
}

DWORD CWebServer::ThreadProc()
{
	if (!AfxSocketInit(nullptr)) {
		return DWORD_ERROR;
	}

	CWebServerSocket s(this, m_nPort);

	MSG msg;
	while ((int)GetMessage(&msg, nullptr, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return 0;
}

static void PutFileContents(LPCWSTR fn, const CStringA& data)
{
	FILE* f = nullptr;
	if (_wfopen_s(&f, fn, L"wb") == 0) {
		fwrite((LPCSTR)data, 1, data.GetLength(), f);
		fclose(f);
	}
}

void CWebServer::Deploy(CString dir)
{
	CStringA data;
	if (LoadResource(IDR_HTML_INDEX, data, RT_HTML)) {
		PutFileContents(dir + L"index.html", data);
	}
	if (LoadResource(IDR_HTML_INFO, data, RT_HTML)) {
		PutFileContents(dir + L"info.html", data);
	}
	if (LoadResource(IDR_HTML_BROWSER, data, RT_HTML)) {
		PutFileContents(dir + L"browser.html", data);
	}
	if (LoadResource(IDR_HTML_CONTROLS, data, RT_HTML)) {
		PutFileContents(dir + L"controls.html", data);
	}
	if (LoadResource(IDR_HTML_VARIABLES, data, RT_HTML)) {
		PutFileContents(dir + L"variables.html", data);
	}
	if (LoadResource(IDR_HTML_404, data, RT_HTML)) {
		PutFileContents(dir + L"404.html", data);
	}
	if (LoadResource(IDR_HTML_PLAYER, data, RT_HTML)) {
		PutFileContents(dir + L"player.html", data);
	}

	for (auto& [fn, id] : m_downloads) {
		if (LoadResource(id, data, L"FILE")) {
			PutFileContents(dir + fn, data);
		}
	}
}

bool CWebServer::ToLocalPath(CString& path, CString& redir)
{
	if (!path.IsEmpty() && m_webroot.IsDirectory()) {
		CString tmp = path;
		tmp.Replace('/', '\\');
		tmp.TrimLeft('\\');

		CPath p;
		p.Combine(m_webroot, tmp);
		p.Canonicalize();

		if (p.IsDirectory()) {
			std::list<CString> sl;
			Explode(AfxGetAppSettings().strWebDefIndex, sl, L';');
			for (const auto& str : sl) {
				CPath p2 = p;
				p2.Append(str);
				if (p2.FileExists()) {
					p = p2;
					redir = path;
					if (redir.GetAt(redir.GetLength()-1) != L'/') {
						redir += L'/';
					}
					redir += str;
					break;
				}
			}
		}

		if (wcslen(p) > wcslen(m_webroot) && p.FileExists()) {
			path = (LPCWSTR)p;
			return true;
		}
	}

	return false;
}

bool CWebServer::LoadPage(UINT resid, CStringA& str, CString path)
{
	CString redir;
	if (ToLocalPath(path, redir)) {
		FILE* f = nullptr;
		if (_wfopen_s(&f, path, L"rb") == 0) {
			fseek(f, 0, 2);
			char* buff = str.GetBufferSetLength(ftell(f));
			fseek(f, 0, 0);
			int len = (int)fread(buff, 1, str.GetLength(), f);
			fclose(f);
			return len == str.GetLength();
		}
	}

	return LoadResource(resid, str, RT_HTML);
}

void CWebServer::OnAccept(CWebServerSocket* pServer)
{
	std::unique_ptr<CWebClientSocket> p(DNew CWebClientSocket(this, m_pMainFrame));
	if (pServer->Accept(*p)) {
		CString name;
		UINT port;
		if (AfxGetAppSettings().fWebServerLocalhostOnly && p->GetPeerName(name, port) && name != L"127.0.0.1") {
			p->Close();
			return;
		}

		m_clients.emplace_back(std::move(p));
	}
}

void CWebServer::OnClose(CWebClientSocket* pClient)
{
	for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
		if ((*it).get() == pClient) {
			m_clients.erase(it);
			break;
		}
	}
}

void CWebServer::OnRequest(CWebClientSocket* pClient, CStringA& hdr, CStringA& body)
{
	CPath p(pClient->m_path);
	CStringA ext(p.GetExtension().MakeLower());
	CStringA mime;
	if (ext.IsEmpty()) {
		mime = "text/html";
	} else {
		auto it = m_mimes.find(ext);
		if (it != m_mimes.end()) {
			mime = (*it).second;
		}
	}

	hdr = "HTTP/1.1 200 OK\r\n";

	bool fHandled = false, fCGI = false;

	if (m_webroot.IsDirectory()) {
		CStringA tmphdr;
		fHandled = fCGI = CallCGI(pClient, tmphdr, body, mime);

		if (fHandled) {
			tmphdr.Replace("\r\n", "\n");
			std::list<CStringA> hdrlines;
			ExplodeMin(tmphdr, hdrlines, '\n');

			for (auto it = hdrlines.begin(); it != hdrlines.end();) {
				auto cur = it++;
				std::list<CStringA> sl;
				CStringA key = Explode(*cur, sl, ':', 2);
				if (sl.size() < 2) {
					continue;
				}
				key.Trim().MakeLower();
				if (key == "content-type") {
					mime = sl.back().Trim();
					hdrlines.erase(cur);
				} else if (key == "content-length") {
					hdrlines.erase(cur);
				}
			}
			tmphdr = Implode(hdrlines, "\r\n");
			hdr += tmphdr + "\r\n";
		}
	}

	RequestHandler rh = nullptr;
	if (!fHandled) {
		auto it = m_internalpages.find(pClient->m_path);
		if (it != m_internalpages.end() && (pClient->*(*it).second)(hdr, body, mime)) {
			if (mime.IsEmpty()) {
				mime = "text/html";
			}

			auto it2 = pClient->m_get.find(L"redir");
			if (it2 != pClient->m_get.end() || (it2 = pClient->m_post.find(L"redir")) != pClient->m_post.end()) {
				CString redir = (*it2).second;
				if (redir.IsEmpty()) {
					redir = '/';
				}

				hdr = "HTTP/1.1 302 Found\r\nLocation: " + CStringA(redir) + "\r\n";
				return;
			}

			fHandled = true;
		}
	}

	if (!fHandled && m_webroot.IsDirectory()) {
		fHandled = LoadPage(0, body, pClient->m_path);
	}

	if (!fHandled) {
		auto it = m_downloads.find(pClient->m_path);
		if (it != m_downloads.end() && LoadResource((*it).second, body, L"FILE")) {
			if (mime.IsEmpty()) {
				mime = "application/octet-stream";
			}
			fHandled = true;
		}
	}

	if (!fHandled) {
		hdr = mime == "text/html"
			  ? "HTTP/1.1 301 Moved Permanently\r\n" "Location: /404.html\r\n"
			  : "HTTP/1.1 404 Not Found\r\n";
		return;
	}

	if (mime == "text/html" && !fCGI) {
		hdr +=
			"Expires: Thu, 19 Nov 1981 08:52:00 GMT\r\n"
			"Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
			"Pragma: no-cache\r\n";

		CStringA debug;
		if (AfxGetAppSettings().fWebServerPrintDebugInfo) {
			debug += "<hr>\r\n";

			for (const auto& [key, value] : pClient->m_hdrlines) {
				debug += "HEADER[" + key + "] = " + value + "<br>\r\n";
			}
			debug += "cmd: " + pClient->m_cmd + "<br>\r\n";
			debug += "path: " + pClient->m_path + "<br>\r\n";
			debug += "ver: " + pClient->m_ver + "<br>\r\n";

			for (const auto&[key, value] : pClient->m_get) {
				debug += "GET[" + key + "] = " + value + "<br>\r\n";
			}
			for (const auto&[key, value] : pClient->m_post) {
				debug += "POST[" + key + "] = " + value + "<br>\r\n";
			}
			for (const auto&[key, value] : pClient->m_cookie) {
				debug += "COOKIE[" + key + "] = " + value + "<br>\r\n";
			}
			for (const auto&[key, value] : pClient->m_request) {
				debug += "REQUEST[" + key + "] = " + value + "<br>\r\n";
			}
		}
		body.Replace("[debug]", debug);

		body.Replace("[browserpath]", "/browser.html");
		body.Replace("[commandpath]", "/command.html");
		body.Replace("[controlspath]", "/controls.html");
		body.Replace("[indexpath]", "/index.html");
		body.Replace("[path]", CStringA(pClient->m_path));
		body.Replace("[setposcommand]", _CRT_STRINGIZE(CMD_SETPOS));
		body.Replace("[setvolumecommand]", _CRT_STRINGIZE(CMD_SETVOLUME));
		body.Replace("[wmcname]", "wm_command");
	}

	// gzip
	if (AfxGetAppSettings().fWebServerUseCompression && !body.IsEmpty()
		&& hdr.Find("Content-Encoding:") < 0 && ext != ".png" && ext != ".jpg" && ext != ".gif")
		do {
			std::list<CString> sl;
			CString accept_encoding;
			if (const auto it = pClient->m_hdrlines.find(L"accept-encoding"); it != pClient->m_hdrlines.end()) {
				accept_encoding = (*it).second;
				accept_encoding.MakeLower();
				ExplodeMin(accept_encoding, sl, ',');
			}
			if (std::find(sl.cbegin(), sl.cend(), L"gzip") == sl.cend()) {
				break;
			}

			// Allocate deflate state
			z_stream strm;

			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			int ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
			if (ret != Z_OK) {
				ASSERT(0);
				break;
			}

			int gzippedBuffLen = body.GetLength()+1;
			BYTE* gzippedBuff = DNew BYTE[gzippedBuffLen];

			// Compress
			strm.avail_in = body.GetLength();
			strm.next_in = (Bytef*)(LPCSTR)body;

			strm.avail_out = gzippedBuffLen;
			strm.next_out = gzippedBuff;

			ret = deflate(&strm, Z_FINISH);
			if (ret != Z_STREAM_END || strm.avail_in != 0) {
				ASSERT(0);
				deflateEnd(&strm);
				delete [] gzippedBuff;
				break;
			}
			gzippedBuffLen -= strm.avail_out;
			memcpy(body.GetBufferSetLength(gzippedBuffLen), gzippedBuff, gzippedBuffLen);

			// Clean up
			deflateEnd(&strm);
			delete [] gzippedBuff;

			hdr += "Content-Encoding: gzip\r\n";
		} while (0);

	CStringA content;
	content.Format(
		"Content-Type: %s\r\n"
		"Content-Length: %d\r\n",
		mime, body.GetLength());
	hdr += content;
}

static DWORD WINAPI KillCGI(LPVOID lParam)
{
	HANDLE hProcess = (HANDLE)lParam;
	if (WaitForSingleObject(hProcess, 30000) == WAIT_TIMEOUT) {
		TerminateProcess(hProcess, 0);
	}
	return 0;
}

bool CWebServer::CallCGI(CWebClientSocket* pClient, CStringA& hdr, CStringA& body, CStringA& mime)
{
	CString path = pClient->m_path, redir = path;
	if (!ToLocalPath(path, redir)) {
		return false;
	}
	CString ext = CPath(path).GetExtension().MakeLower();
	CPath dir(path);
	dir.RemoveFileSpec();

	CString cgi;
	auto it = m_cgi.find(ext);
	if (it == m_cgi.end() || !CPath((*it).second).FileExists()) {
		return false;
	}
	cgi = (*it).second;

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hChildStdinRd, hChildStdinWr, hChildStdinWrDup = nullptr;
	HANDLE hChildStdoutRd, hChildStdoutWr, hChildStdoutRdDup = nullptr;

	SECURITY_ATTRIBUTES saAttr;
	ZeroMemory(&saAttr, sizeof(saAttr));
	saAttr.nLength = sizeof(saAttr);
	saAttr.bInheritHandle = TRUE;

	if (CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
		BOOL fSuccess = DuplicateHandle(hProcess, hChildStdoutRd, hProcess, &hChildStdoutRdDup, 0, FALSE, DUPLICATE_SAME_ACCESS);
		UNREFERENCED_PARAMETER(fSuccess);
		CloseHandle(hChildStdoutRd);
	}

	if (CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) {
		BOOL fSuccess = DuplicateHandle(hProcess, hChildStdinWr, hProcess, &hChildStdinWrDup, 0, FALSE, DUPLICATE_SAME_ACCESS);
		UNREFERENCED_PARAMETER(fSuccess);
		CloseHandle(hChildStdinWr);
	}

	STARTUPINFO siStartInfo;
	ZeroMemory(&siStartInfo, sizeof(siStartInfo));
	siStartInfo.cb = sizeof(siStartInfo);
	siStartInfo.hStdError = hChildStdoutWr;
	siStartInfo.hStdOutput = hChildStdoutWr;
	siStartInfo.hStdInput = hChildStdinRd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	siStartInfo.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(piProcInfo));

	CStringA envstr;

	LPVOID lpvEnv = GetEnvironmentStringsW();
	if (lpvEnv) {
		CString str;

		std::list<CString> env;
		for (LPWSTR lpszVariable = (LPWSTR)lpvEnv; *lpszVariable; lpszVariable += wcslen(lpszVariable)+1)
			if (lpszVariable != (LPWSTR)lpvEnv) {
				env.emplace_back(lpszVariable);
			}

		env.emplace_back(L"GATEWAY_INTERFACE=CGI/1.1");
		env.emplace_back(L"SERVER_SOFTWARE=MPC-BE/6.4.x.y");
		env.emplace_back(L"SERVER_PROTOCOL=" + pClient->m_ver);
		env.emplace_back(L"REQUEST_METHOD=" + pClient->m_cmd);
		env.emplace_back(L"PATH_INFO=" + redir);
		env.emplace_back(L"PATH_TRANSLATED=" + path);
		env.emplace_back(L"SCRIPT_NAME=" + redir);
		env.emplace_back(L"QUERY_STRING=" + pClient->m_query);

		if (const auto it2 = pClient->m_hdrlines.find(L"content-type"); it2 != pClient->m_hdrlines.end()) {
			env.emplace_back(L"CONTENT_TYPE=" + (*it2).second);
		}
		if (const auto it2 = pClient->m_hdrlines.find(L"content-length"); it2 != pClient->m_hdrlines.end()) {
			env.emplace_back(L"CONTENT_LENGTH=" + (*it2).second);
		}

		for (const auto& [key, value] : pClient->m_hdrlines) {
			CString Key = key;
			Key.Replace(L"-", L"_");
			Key.MakeUpper();
			env.emplace_back(L"HTTP_" + Key + L"=" + value);
		}

		CString name;
		UINT port;

		if (pClient->GetPeerName(name, port)) {
			str.Format(L"%u", port);
			env.emplace_back(L"REMOTE_ADDR=" + name);
			env.emplace_back(L"REMOTE_HOST=" + name);
			env.emplace_back(L"REMOTE_PORT=" + str);
		}

		if (pClient->GetSockName(name, port)) {
			str.Format(L"%u", port);
			env.emplace_back(L"SERVER_NAME=" + name);
			env.emplace_back(L"SERVER_PORT=" + str);
		}

		env.emplace_back(L"\0");

		str = Implode(env, '\0');
		envstr = CStringA(str, str.GetLength());

		FreeEnvironmentStringsW((LPWSTR)lpvEnv);
	}

	WCHAR* cmdln = DNew WCHAR[32768];
	_snwprintf_s(cmdln, 32768, 32768, L"\"%s\" \"%s\"", cgi.GetString(), path.GetString());

	if (hChildStdinRd && hChildStdoutWr)
		if (CreateProcess(
					nullptr, cmdln, nullptr, nullptr, TRUE, 0,
					envstr.GetLength() ? (LPVOID)(LPCSTR)envstr : nullptr,
					dir, &siStartInfo, &piProcInfo)) {
			DWORD ThreadId;
			VERIFY(CreateThread(nullptr, 0, KillCGI, (LPVOID)piProcInfo.hProcess, 0, &ThreadId));

			static const int BUFFSIZE = 1024;
			DWORD dwRead, dwWritten = 0;

			int i = 0, len = pClient->m_data.GetLength();
			for (; i < len; i += dwWritten)
				if (!WriteFile(hChildStdinWrDup, (LPCSTR)pClient->m_data + i, std::min(len - i, BUFFSIZE), &dwWritten, nullptr)) {
					break;
				}

			CloseHandle(hChildStdinWrDup);
			CloseHandle(hChildStdoutWr);

			body.Empty();

			CStringA buff;
			while (i == len && ReadFile(hChildStdoutRdDup, buff.GetBuffer(BUFFSIZE), BUFFSIZE, &dwRead, nullptr) && dwRead) {
				buff.ReleaseBufferSetLength(dwRead);
				body += buff;
			}

			int hdrend = body.Find("\r\n\r\n");
			if (hdrend >= 0) {
				hdr = body.Left(hdrend + 2);
				body = body.Mid(hdrend + 4);
			}

			CloseHandle(hChildStdinRd);
			CloseHandle(hChildStdoutRdDup);

			CloseHandle(piProcInfo.hProcess);
			CloseHandle(piProcInfo.hThread);
		} else {
			body = L"CGI Error";
		}

	delete [] cmdln;

	return true;
}
