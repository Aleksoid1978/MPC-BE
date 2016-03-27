/*
 * (C) 2016 see Authors.txt
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

#include <wininet.h>
#include <atlutil.h>

class CHTTPAsync
{
protected:
	enum Context {
		CONTEXT_INITIAL = -1,
		CONTEXT_CONNECT,
		CONTEXT_REQUEST
	};
	Context m_context = CONTEXT_INITIAL;

	HANDLE m_hConnectedEvent       = INVALID_HANDLE_VALUE;
	HANDLE m_hRequestOpenedEvent   = INVALID_HANDLE_VALUE;
	HANDLE m_hRequestCompleteEvent = INVALID_HANDLE_VALUE;

	HINTERNET m_hInstance = NULL;
	HINTERNET m_hConnect  = NULL;
	HINTERNET m_hRequest  = NULL;

	CUrl m_url;
	CString m_url_str;
	CString m_host;
	CString m_path;

	ATL_URL_PORT m_nPort     = INTERNET_DEFAULT_HTTP_PORT;
	ATL_URL_SCHEME m_nScheme = ATL_URL_SCHEME_HTTP;

	QWORD m_lenght = 0;

	static void CALLBACK Callback(__in HINTERNET hInternet,
								  __in_opt DWORD_PTR dwContext,
								  __in DWORD dwInternetStatus,
								  __in_opt LPVOID lpvStatusInformation,
								  __in DWORD dwStatusInformationLength);

	CString const QueryInfo(DWORD dwInfoLevel);

public:
	CHTTPAsync();
	virtual ~CHTTPAsync();

	void Close();

	HRESULT Connect(LPCTSTR lpszURL, DWORD dwTimeOut = INFINITE, LPCTSTR lpszAgent = L"MPC-BE", BOOL bSendRequest = TRUE);
	DWORD Read(PBYTE pBuffer, DWORD dwSize, DWORD dwTimeOut = INFINITE);

	CString const GetHeader() { return QueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF); }
	QWORD const GetLenght() { return m_lenght; }
};

