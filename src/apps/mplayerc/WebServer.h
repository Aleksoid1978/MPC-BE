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

#pragma once

#include <afxsock.h>
#include <atlpath.h>

#define UTF8(str)    WStrToUTF8(str)
#define UTF8Arg(str) UrlEncode(UTF8(str), true)

#define CMD_SETPOS    -1
#define CMD_SETVOLUME -2

class CWebServer;

class CWebServerSocket : public CAsyncSocket
{
	CWebServer* m_pWebServer;

protected:
	void OnAccept(int nErrorCode);

public:
	CWebServerSocket(CWebServer* pWebServer, int port = 13579);
	virtual ~CWebServerSocket();
};

class CWebServerSocket;
class CWebClientSocket;
class CMainFrame;

class CWebServer
{
	CMainFrame* m_pMainFrame;
	int m_nPort;

	DWORD ThreadProc();
	static DWORD WINAPI StaticThreadProc(LPVOID lpParam);
	DWORD m_ThreadId;
	HANDLE m_hThread;

	CAutoPtrList<CWebClientSocket> m_clients;

	typedef bool (CWebClientSocket::*RequestHandler)(CStringA& hdr, CStringA& body, CStringA& mime);
	static const std::map<CString, CWebServer::RequestHandler> m_internalpages;
	static const std::map<CString, UINT> m_downloads;
	static std::map<CStringA, CStringA> m_mimes;
	CPath m_webroot;

	std::map<CString, CString> m_cgi;
	bool CallCGI(CWebClientSocket* pClient, CStringA& hdr, CStringA& body, CStringA& mime);

public:
	CWebServer(CMainFrame* pMainFrame, int nPort = 13579);
	~CWebServer();

	static void Deploy(CString dir);

	bool ToLocalPath(CString& path, CString& redir);
	bool LoadPage(UINT resid, CStringA& str, CString path = L"");

	void OnAccept(CWebServerSocket* pServer);
	void OnClose(CWebClientSocket* pClient);
	void OnRequest(CWebClientSocket* pClient, CStringA& reshdr, CStringA& resbody);
};
