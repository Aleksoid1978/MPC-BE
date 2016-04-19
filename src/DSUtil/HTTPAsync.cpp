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

#include "stdafx.h"
#include "HTTPAsync.h"

#pragma comment(lib, "WinInet.Lib")

void CALLBACK CHTTPAsync::Callback(_In_ HINTERNET hInternet,
								   __in_opt DWORD_PTR dwContext,
								   __in DWORD dwInternetStatus,
								   __in_opt LPVOID lpvStatusInformation,
								   __in DWORD dwStatusInformationLength)
{
	auto* pContext = (CHTTPAsync*)dwContext;
	auto* pRes     = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;
	switch (pContext->m_context) {
		case Context::CONTEXT_CONNECT:
			if (dwInternetStatus == INTERNET_STATUS_HANDLE_CREATED) {
				pContext->m_hConnect = (HINTERNET)pRes->dwResult;
				SetEvent(pContext->m_hConnectedEvent);
			}
			break;
		case Context::CONTEXT_REQUEST:
			{
				switch (dwInternetStatus) {
					case INTERNET_STATUS_HANDLE_CREATED:
						{
							pContext->m_hRequest = (HINTERNET)pRes->dwResult;
							pContext->m_bRequestComplete = TRUE;
							SetEvent(pContext->m_hRequestOpenedEvent);
						}
						break;
					case INTERNET_STATUS_REQUEST_SENT:
						{
							DWORD* lpBytesSent = (DWORD*)lpvStatusInformation;
							UNREFERENCED_PARAMETER(lpBytesSent);
						}
						break;
					case INTERNET_STATUS_REQUEST_COMPLETE:
						{
							pContext->m_bRequestComplete = TRUE;
							SetEvent(pContext->m_hRequestCompleteEvent);
						}
						break;
					case INTERNET_STATUS_REDIRECT:
						{
							CString strNewAddr = (LPCTSTR)lpvStatusInformation;
							UNREFERENCED_PARAMETER(strNewAddr);
						}
						break;
					case INTERNET_STATUS_RESPONSE_RECEIVED:
						{
							DWORD* dwBytesReceived = (DWORD*)lpvStatusInformation;
							UNREFERENCED_PARAMETER(dwBytesReceived);
						}
						break;
					}
			}
	}
}

CString CHTTPAsync::QueryInfoStr(DWORD dwInfoLevel) const
{
	CheckPointer(m_hRequest, L"");

	CString queryInfo;
	DWORD   dwLen = 0;
	if (!HttpQueryInfo(m_hRequest, dwInfoLevel, NULL, &dwLen, 0) && dwLen) {
		const DWORD dwError = GetLastError();
		if (dwError == ERROR_INSUFFICIENT_BUFFER
				&& HttpQueryInfo(m_hRequest, dwInfoLevel, (LPVOID)queryInfo.GetBuffer(dwLen), &dwLen, 0)) {
			queryInfo.ReleaseBuffer(dwLen);
		}
	}

	return queryInfo;
}

DWORD CHTTPAsync::QueryInfoDword(DWORD dwInfoLevel) const
{
	CheckPointer(m_hRequest, 0);

	DWORD dwStatusCode = 0;
	DWORD dwStatusLen  = sizeof(dwStatusCode);
	HttpQueryInfo(m_hRequest, HTTP_QUERY_FLAG_NUMBER | dwInfoLevel, &dwStatusCode, &dwStatusLen, 0);

	return dwStatusCode;
}

CHTTPAsync::CHTTPAsync()
{
	m_hConnectedEvent       = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRequestOpenedEvent   = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRequestCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CHTTPAsync::~CHTTPAsync()
{
	Close();

	if (m_hConnectedEvent) {
		CloseHandle(m_hConnectedEvent);
	}
	if (m_hRequestOpenedEvent) {
		CloseHandle(m_hRequestOpenedEvent);
	}
	if (m_hRequestCompleteEvent) {
		CloseHandle(m_hRequestCompleteEvent);
	}
}

static CString FormatErrorMessage(DWORD dwError)
{
	CString errMsg;

	LPVOID lpMsgBuf = NULL;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS |
					  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
					  GetModuleHandle(L"wininet"), dwError,
					  MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
					  (LPTSTR)&lpMsgBuf, 0, NULL) > 0) {
		errMsg = (LPTSTR)lpMsgBuf;
		errMsg.TrimRight(L"\r\n");
		LocalFree(lpMsgBuf);
	}

	if (dwError == ERROR_INTERNET_EXTENDED_ERROR) {
		CString internetInfo;
		DWORD   dwLen = 0;
		if (!InternetGetLastResponseInfo(&dwError, NULL, &dwLen) && dwLen) {
			const DWORD dwLastError = GetLastError();
			if (dwLastError == ERROR_INSUFFICIENT_BUFFER
					&& InternetGetLastResponseInfo(&dwError, internetInfo.GetBuffer(dwLen), &dwLen)) {
				internetInfo.ReleaseBuffer(dwLen);
				if (!internetInfo.IsEmpty()) {
					errMsg += L", " + internetInfo;
					errMsg.TrimRight(L"\r\n");
				}
			}
		}
	}
	
	return errMsg;
}

#define SAFE_INTERNET_CLOSE_HANDLE(p) { if (p) { VERIFY(InternetCloseHandle(p)); (p) = NULL; } }
#define CheckLastError(lpszFunction, ret) \
{ \
	const DWORD dwError = GetLastError(); \
	if (dwError != ERROR_IO_PENDING) { \
		DbgLog((LOG_TRACE, 3, L"CHTTPAsync() error : Function '%s' failed with error %d - '%s', line %i", CString(lpszFunction), dwError, FormatErrorMessage(dwError), __LINE__)); \
		return (ret); \
	} \
} \

void CHTTPAsync::Close()
{
	ResetEvent(m_hConnectedEvent);
	ResetEvent(m_hRequestOpenedEvent);
	ResetEvent(m_hRequestCompleteEvent);

	if (m_hInstance) {
		InternetSetStatusCallback(m_hInstance, NULL);
	}

	SAFE_INTERNET_CLOSE_HANDLE(m_hRequest);
	SAFE_INTERNET_CLOSE_HANDLE(m_hConnect);
	SAFE_INTERNET_CLOSE_HANDLE(m_hInstance);

	m_url.Clear();
	m_url_str.Empty();
	m_host.Empty();
	m_path.Empty();

	m_nPort   = INTERNET_DEFAULT_HTTP_PORT;
	m_nScheme = ATL_URL_SCHEME_HTTP;

	m_header.Empty();
	m_contentType.Empty();
	m_lenght = 0;

	m_bRequestComplete = TRUE;
}

HRESULT CHTTPAsync::Connect(LPCTSTR lpszURL, DWORD dwTimeOut/* = INFINITE*/, LPCTSTR lpszAgent/* = L"MPC-BE"*/, BOOL bSendRequest/* = TRUE*/)
{
	Close();

	if (!m_url.CrackUrl(lpszURL)) {
		return E_INVALIDARG;
	}
	if (m_url.GetScheme() != ATL_URL_SCHEME_HTTP && m_url.GetScheme() != ATL_URL_SCHEME_HTTPS) {
		return E_FAIL;
	}
	if (m_url.GetPortNumber() == ATL_URL_INVALID_PORT_NUMBER) {
		m_url.SetPortNumber(ATL_URL_DEFAULT_HTTP_PORT);
	}

	m_url_str = lpszURL;
	if (m_url.GetUrlPathLength() == 0) {
		m_url.SetUrlPath(L"/");
		m_url_str += '/';
	}

	m_host    = m_url.GetHostName();
	m_path    = CString(m_url.GetUrlPath()) + CString(m_url.GetExtraInfo());
	m_nPort   = m_url.GetPortNumber();
	m_nScheme = m_url.GetScheme();

	m_hInstance = InternetOpen(lpszAgent, 
							   INTERNET_OPEN_TYPE_PRECONFIG,
							   NULL,
							   NULL,
							   INTERNET_FLAG_ASYNC);
	CheckPointer(m_hInstance, E_FAIL);

	if (InternetSetStatusCallback(m_hInstance, (INTERNET_STATUS_CALLBACK)&Callback) == INTERNET_INVALID_STATUS_CALLBACK) {
		return E_FAIL;
	}

	m_context = Context::CONTEXT_CONNECT;

	m_hConnect = InternetConnect(m_hInstance, 
								 m_host,
								 m_nPort,
								 NULL,
								 NULL,
								 INTERNET_SERVICE_HTTP,
								 INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE,
								 (DWORD_PTR)this);
	if (m_hConnect == NULL) {
		CheckLastError(L"InternetConnect()", E_FAIL);

		if (WaitForSingleObject(m_hConnectedEvent, dwTimeOut) == WAIT_TIMEOUT) {
			return E_FAIL;
		}
	}

	CheckPointer(m_hConnect, E_FAIL);

	if (bSendRequest) {
		if (SendRequest(L"", dwTimeOut) != S_OK) {
			return E_FAIL;
		}

		m_header = QueryInfoStr(HTTP_QUERY_RAW_HEADERS_CRLF);
		m_contentType = QueryInfoStr(HTTP_QUERY_CONTENT_TYPE);

		const CString queryInfo = QueryInfoStr(HTTP_QUERY_CONTENT_LENGTH);
		if (!queryInfo.IsEmpty()) {
			QWORD val = 0;
			if (1 == swscanf_s(queryInfo, L"%I64u", &val)) {
				m_lenght = val;
			}
		}
	}

	return S_OK;
}

HRESULT CHTTPAsync::SendRequest(LPCTSTR customHeader/* = L""*/, DWORD dwTimeOut/* = INFINITE*/)
{
	CheckPointer(m_hConnect, E_FAIL);

	std::unique_lock<std::mutex> lock(m_mutexRequest);

	if (!m_bRequestComplete) {
		DbgLog((LOG_TRACE, 3, L"CHTTPAsync::SendRequest() : previous request has not completed, exit"));
		return S_FALSE;
	}

	ResetEvent(m_hRequestOpenedEvent);
	ResetEvent(m_hRequestCompleteEvent);

	SAFE_INTERNET_CLOSE_HANDLE(m_hRequest);

	m_context = Context::CONTEXT_REQUEST;

	DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_KEEP_CONNECTION;
	if (m_nScheme == ATL_URL_SCHEME_HTTPS) {
		dwFlags |= (INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
	}

	m_hRequest = HttpOpenRequest(m_hConnect,
								 L"GET",
								 m_path,
								 NULL,
								 NULL,
								 NULL,
								 dwFlags,
								 (DWORD_PTR)this);
	if (m_hRequest == NULL) {
		CheckLastError(L"HttpOpenRequest()", E_FAIL);
		
		if (WaitForSingleObject(m_hRequestOpenedEvent, dwTimeOut) == WAIT_TIMEOUT) {
			DbgLog((LOG_TRACE, 3, L"CHTTPAsync::SendRequest() : HttpOpenRequest() - %d ms time out reached, exit", dwTimeOut));
			m_bRequestComplete = FALSE;
			return E_FAIL;
		}
	}

	CheckPointer(m_hRequest, E_FAIL);

	CString lpszHeaders = L"Accept: */*\r\n";
	lpszHeaders += customHeader;
	for (;;) {
		if (!HttpSendRequest(m_hRequest,
							 lpszHeaders,
							 lpszHeaders.GetLength(),
							 NULL,
							 0)) {
			CheckLastError(L"HttpSendRequest()", E_FAIL);

			if (WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT) {
				DbgLog((LOG_TRACE, 3, L"CHTTPAsync::SendRequest() : HttpSendRequest() - %d ms time out reached, exit", dwTimeOut));
				m_bRequestComplete = FALSE;
				return S_FALSE;
			}
		}

		const DWORD dwStatusCode = QueryInfoDword(HTTP_QUERY_STATUS_CODE);
		if (dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ) {
			DWORD dwFlags = FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA;
			const DWORD ret = InternetErrorDlg(GetDesktopWindow(),
											   m_hRequest,
											   ERROR_INTERNET_INCORRECT_PASSWORD,
											   dwFlags,
											   NULL);
			if (ret == ERROR_INTERNET_FORCE_RETRY) {
				continue;
			}

			return E_FAIL;
		} else if (dwStatusCode != HTTP_STATUS_OK && dwStatusCode != HTTP_STATUS_PARTIAL_CONTENT) {
			return E_FAIL;
		}
			
		break;
	}

	return S_OK;
}

HRESULT CHTTPAsync::Read(PBYTE pBuffer, DWORD dwSizeToRead, LPDWORD dwSizeRead, DWORD dwTimeOut/* = INFINITE*/)
{
	CheckPointer(m_hRequest, E_FAIL);

	std::unique_lock<std::mutex> lock(m_mutexRequest);

	if (!m_bRequestComplete) {
		DbgLog((LOG_TRACE, 3, L"CHTTPAsync::Read() : previous request has not completed, exit"));
		return S_FALSE;
	}

	INTERNET_BUFFERSA InetBuff = { sizeof(InetBuff) };
	InetBuff.lpvBuffer         = pBuffer;
	InetBuff.dwBufferLength    = dwSizeToRead;

	m_context = Context::CONTEXT_REQUEST;

	if (!InternetReadFileExA(m_hRequest,
							 &InetBuff,
							 IRF_ASYNC,
							 (DWORD_PTR)this)) {
		CheckLastError(L"InternetReadFileExA()", E_FAIL);

		if (WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT) {
			DbgLog((LOG_TRACE, 3, L"CHTTPAsync::Read() : InternetReadFileExA() - %d ms time out reached, exit", dwTimeOut));
			m_bRequestComplete = FALSE;
			return S_FALSE;
		}
	}

	if (dwSizeRead) {
		*dwSizeRead = InetBuff.dwBufferLength;
	}

	return S_OK;
}

CString CHTTPAsync::GetHeader() const
{
	return m_header;
}

CString CHTTPAsync::GetContentType() const
{
	return m_contentType;
}

QWORD CHTTPAsync::GetLenght() const
{
	return m_lenght;
}
