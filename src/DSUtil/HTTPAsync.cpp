/*
 * (C) 2016-2025 see Authors.txt
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
#include "Log.h"
#include "text.h"
#include <ExtLib/zlib/zlib.h>

void CALLBACK CHTTPAsync::Callback(_In_ HINTERNET hInternet,
								   __in_opt DWORD_PTR dwContext,
								   __in DWORD dwInternetStatus,
								   __in_opt LPVOID lpvStatusInformation,
								   __in DWORD dwStatusInformationLength)
{
	if (dwInternetStatus == INTERNET_STATUS_HANDLE_CLOSING) {
		// there is no point in processing further.
		// also dwContext may be incorrect on Windows 7 if there is no network.
		return;
	}
	if (!lpvStatusInformation) {
		return;
	}

	auto pHTTPAsync   = reinterpret_cast<CHTTPAsync*>(dwContext);
	auto pAsyncResult = reinterpret_cast<INTERNET_ASYNC_RESULT*>(lpvStatusInformation);

	if (pHTTPAsync->m_bClosing) {
		return;
	}

	switch (pHTTPAsync->m_context) {
		case Context::CONTEXT_CONNECT:
			if (dwInternetStatus == INTERNET_STATUS_HANDLE_CREATED) {
				pHTTPAsync->m_hConnect = reinterpret_cast<HINTERNET>(pAsyncResult->dwResult);
				SetEvent(pHTTPAsync->m_hConnectedEvent);
			}
			break;
		case Context::CONTEXT_REQUEST:
			{
				switch (dwInternetStatus) {
					case INTERNET_STATUS_HANDLE_CREATED:
						pHTTPAsync->m_hRequest = reinterpret_cast<HINTERNET>(pAsyncResult->dwResult);
						pHTTPAsync->m_bRequestComplete = TRUE;
						SetEvent(pHTTPAsync->m_hRequestOpenedEvent);
						break;
					case INTERNET_STATUS_REQUEST_COMPLETE:
						pHTTPAsync->m_bRequestComplete = TRUE;
						SetEvent(pHTTPAsync->m_hRequestCompleteEvent);
						break;
					case INTERNET_STATUS_REDIRECT:
						pHTTPAsync->m_url_redirect_str.SetString(reinterpret_cast<LPCWSTR>(lpvStatusInformation), dwStatusInformationLength);
						break;
					}
			}
	}
}

CStringA CHTTPAsync::QueryInfoStr(DWORD dwInfoLevel) const
{
	CheckPointer(m_hRequest, "");

	CStringA queryInfo;
	DWORD   dwLen = 0;
	if (!HttpQueryInfoA(m_hRequest, dwInfoLevel, nullptr, &dwLen, nullptr) && dwLen) {
		const DWORD dwError = GetLastError();
		if (dwError == ERROR_INSUFFICIENT_BUFFER
				&& HttpQueryInfoA(m_hRequest, dwInfoLevel, (LPVOID)queryInfo.GetBuffer(dwLen), &dwLen, nullptr)) {
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
	HttpQueryInfoW(m_hRequest, HTTP_QUERY_FLAG_NUMBER | dwInfoLevel, &dwStatusCode, &dwStatusLen, 0);

	return dwStatusCode;
}

CHTTPAsync::CHTTPAsync()
{
	m_hConnectedEvent       = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	m_hRequestOpenedEvent   = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	m_hRequestCompleteEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
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

static CStringW FormatErrorMessage(DWORD dwError)
{
	CStringW errMsg;

	LPVOID lpMsgBuf = nullptr;
	if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS |
					   FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
					   GetModuleHandleW(L"wininet"), dwError,
					   MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
					   (LPWSTR)&lpMsgBuf, 0, nullptr) > 0) {
		errMsg = (LPWSTR)lpMsgBuf;
		errMsg.TrimRight(L"\r\n");
		LocalFree(lpMsgBuf);
	}

	if (dwError == ERROR_INTERNET_EXTENDED_ERROR) {
		CStringW internetInfo;
		DWORD   dwLen = 0;
		if (!InternetGetLastResponseInfoW(&dwError, nullptr, &dwLen) && dwLen) {
			const DWORD dwLastError = GetLastError();
			if (dwLastError == ERROR_INSUFFICIENT_BUFFER
					&& InternetGetLastResponseInfoW(&dwError, internetInfo.GetBuffer(dwLen), &dwLen)) {
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

#define SAFE_INTERNET_CLOSE_HANDLE(p) { if (p) { VERIFY(InternetCloseHandle(p)); (p) = nullptr; } }
#define CheckLastError(lpszFunction, ret) \
{ \
	const DWORD dwError = GetLastError(); \
	if (dwError != ERROR_IO_PENDING) { \
		DLog(L"CHTTPAsync() error : Function '%s' failed with error %d - '%s', line %i", lpszFunction, dwError, FormatErrorMessage(dwError).GetString(), __LINE__); \
		return (ret); \
	} \
} \

void CHTTPAsync::Close()
{
	m_bClosing = true;

	ResetEvent(m_hConnectedEvent);
	ResetEvent(m_hRequestOpenedEvent);
	ResetEvent(m_hRequestCompleteEvent);

	if (m_hInstance) {
		InternetSetStatusCallbackW(m_hInstance, nullptr);
	}

	SAFE_INTERNET_CLOSE_HANDLE(m_hRequest);
	SAFE_INTERNET_CLOSE_HANDLE(m_hConnect);
	SAFE_INTERNET_CLOSE_HANDLE(m_hInstance);

	m_url_str.Empty();
	m_host.Empty();
	m_path.Empty();

	m_nPort   = 0;
	m_nScheme = INTERNET_SCHEME_HTTP;

	m_header.Empty();
	m_contentType.Empty();
	m_lenght = 0;

	m_bRequestComplete = TRUE;
}

HRESULT CHTTPAsync::Connect(LPCWSTR lpszURL, DWORD dwTimeOut/* = INFINITE*/, LPCWSTR lpszCustomHeader/* = L""*/)
{
	m_url_redirect_str.Empty();

	for (;;) {
		Close();
		m_bClosing = false;

		auto url = !m_url_redirect_str.IsEmpty() ? m_url_redirect_str.GetString() : lpszURL;

		CUrlParser urlParser;
		if (!urlParser.Parse(url)) {
			return E_INVALIDARG;
		}
		if (urlParser.GetScheme() != INTERNET_SCHEME_HTTP && urlParser.GetScheme() != INTERNET_SCHEME_HTTPS) {
			return E_FAIL;
		}

		m_url_str = url;
		m_host = urlParser.GetHostName();
		m_path = CStringW(urlParser.GetUrlPath()) + CStringW(urlParser.GetExtraInfo());
		m_nPort = urlParser.GetPortNumber();
		m_nScheme = urlParser.GetScheme();
		m_schemeName = urlParser.GetSchemeName();

		m_hInstance = InternetOpenW(http::userAgent.GetString(),
									INTERNET_OPEN_TYPE_PRECONFIG,
									nullptr,
									nullptr,
									INTERNET_FLAG_ASYNC);
		CheckPointer(m_hInstance, E_FAIL);

		if (InternetSetStatusCallbackW(m_hInstance, (INTERNET_STATUS_CALLBACK)&Callback) == INTERNET_INVALID_STATUS_CALLBACK) {
			return E_FAIL;
		}

		static bool bSetMaxConnections = false;
		if (!bSetMaxConnections) {
			bSetMaxConnections = true;

			DWORD value = 0;
			DWORD size = sizeof(DWORD);
			if (InternetQueryOptionW(nullptr, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &value, &size) && value < 10) {
				value = 10;
				InternetSetOptionW(nullptr, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &value, size);
			}

			if (InternetQueryOptionW(nullptr, INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER, &value, &size) && value < 10) {
				value = 10;
				InternetSetOptionW(nullptr, INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER, &value, size);
			}
		}

		m_context = Context::CONTEXT_CONNECT;

		m_hConnect = InternetConnectW(m_hInstance,
									  m_host,
									  m_nPort,
									  urlParser.GetUserName(),
									  urlParser.GetPassword(),
									  INTERNET_SERVICE_HTTP,
									  INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE,
									  (DWORD_PTR)this);
		if (m_hConnect == nullptr) {
			CheckLastError(L"InternetConnectW()", E_FAIL);

			if (WaitForSingleObject(m_hConnectedEvent, dwTimeOut) == WAIT_TIMEOUT) {
				return E_FAIL;
			}
		}

		CheckPointer(m_hConnect, E_FAIL);

		auto hr = SendRequest(lpszCustomHeader, dwTimeOut, true);
		if (hr != S_OK) {
			if (hr == E_CHANGED_STATE && !m_url_redirect_str.IsEmpty()) {
				continue;
			}

			return E_FAIL;
		}

		break;
	}

	m_header = QueryInfoStr(HTTP_QUERY_RAW_HEADERS_CRLF);
	m_header.Trim("\r\n ");
#if 0
	DLog(L"CHTTPAsync::Connect() : return header:\n%s", UTF8orLocalToWStr(m_header));
#endif

	m_contentType = QueryInfoStr(HTTP_QUERY_CONTENT_TYPE).MakeLower();
	m_contentEncoding = QueryInfoStr(HTTP_QUERY_CONTENT_ENCODING).MakeLower();
	m_bSupportsRanges = QueryInfoStr(HTTP_QUERY_ACCEPT_RANGES).MakeLower() == L"bytes";

	m_bIsCompressed = !m_contentEncoding.IsEmpty() && (StartsWith(m_contentEncoding, "gzip") || StartsWith(m_contentEncoding, "deflate"));

	const CStringA queryInfo = QueryInfoStr(HTTP_QUERY_CONTENT_LENGTH);
	if (!queryInfo.IsEmpty()) {
		UINT64 val = 0;
		if (1 == sscanf_s(queryInfo, "%I64u", &val)) {
			m_lenght = val;
		}
	}

	m_bIsGoogleMedia = EndsWith(m_host, L"googlevideo.com") && StartsWith(m_path, L"/videoplayback?") &&
					   (StartsWith(m_contentType, "video") || StartsWith(m_contentType, "audio"));

	if (m_lenght && m_bSupportsRanges && m_bIsGoogleMedia) {
		m_http_chunk.end = m_http_chunk.size = std::min(m_lenght, googlemedia_maximum_chunk_size);
		if (RangeInternal(m_http_chunk.start, m_http_chunk.end - 1) == S_OK) {
			m_http_chunk.use = true;
		}
	}

	return S_OK;
}

HRESULT CHTTPAsync::SendRequest(LPCWSTR lpszCustomHeader/* = L""*/, DWORD dwTimeOut/* = INFINITE*/, bool bNoAutoRedirect/* = false*/)
{
	CheckPointer(m_hConnect, E_FAIL);

	std::unique_lock<std::mutex> lock(m_mutexRequest);

	if (!m_bRequestComplete) {
		DLog(L"CHTTPAsync::SendRequest() : previous request has not completed, exit");
		return S_FALSE;
	}

	for (;;) {
		ResetEvent(m_hRequestOpenedEvent);
		ResetEvent(m_hRequestCompleteEvent);

		SAFE_INTERNET_CLOSE_HANDLE(m_hRequest);

		m_context = Context::CONTEXT_REQUEST;

		DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_KEEP_CONNECTION;
		if (bNoAutoRedirect) {
			dwFlags |= INTERNET_FLAG_NO_AUTO_REDIRECT;
		}
		if (m_nScheme == INTERNET_SCHEME_HTTPS) {
			dwFlags |= (INTERNET_FLAG_SECURE | SECURITY_IGNORE_ERROR_MASK);
		}

		m_hRequest = HttpOpenRequestW(m_hConnect,
									  L"GET",
									  m_path,
									  nullptr,
									  nullptr,
									  nullptr,
									  dwFlags,
									  (DWORD_PTR)this);
		if (m_hRequest == nullptr) {
			CheckLastError(L"HttpOpenRequestW()", E_FAIL);

			if (WaitForSingleObject(m_hRequestOpenedEvent, dwTimeOut) == WAIT_TIMEOUT) {
				DLog(L"CHTTPAsync::SendRequest() : HttpOpenRequestW() - %u ms time out reached, exit", dwTimeOut);
				m_bRequestComplete = FALSE;
				return E_FAIL;
			}
		}

		CheckPointer(m_hRequest, E_FAIL);

		bool bNewRequestPath = false;

		CStringW lpszHeaders = L"Accept: */*\r\n";
		lpszHeaders += lpszCustomHeader;
		for (;;) {
			if (!HttpSendRequestW(m_hRequest,
								  lpszHeaders,
								  lpszHeaders.GetLength(),
								  nullptr,
								  0)) {
				CheckLastError(L"HttpSendRequestW()", E_FAIL);

				if (WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT) {
					DLog(L"CHTTPAsync::SendRequest() : HttpSendRequestW() - %u ms time out reached, exit", dwTimeOut);
					m_bRequestComplete = FALSE;
					return S_FALSE;
				}
			}

			const DWORD dwStatusCode = QueryInfoDword(HTTP_QUERY_STATUS_CODE);
			if (dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ || dwStatusCode == HTTP_STATUS_DENIED) {
				dwFlags = FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA;
				const DWORD ret = InternetErrorDlg(GetDesktopWindow(),
												   m_hRequest,
												   ERROR_INTERNET_INCORRECT_PASSWORD,
												   dwFlags,
												   nullptr);
				if (ret == ERROR_INTERNET_FORCE_RETRY) {
					continue;
				}

				return E_FAIL;
			} else if (dwStatusCode != HTTP_STATUS_OK && dwStatusCode != HTTP_STATUS_PARTIAL_CONTENT) {
				if (dwStatusCode == HTTP_STATUS_MOVED
						|| dwStatusCode == HTTP_STATUS_REDIRECT
						|| dwStatusCode == HTTP_STATUS_REDIRECT_METHOD
						|| dwStatusCode == HTTP_STATUS_REDIRECT_KEEP_VERB
						|| dwStatusCode == HTTP_STATUS_PERMANENT_REDIRECT) {
					m_url_redirect_str = QueryInfoStr(HTTP_QUERY_LOCATION);
					if (!m_url_redirect_str.IsEmpty()) {
						CUrlParser urlParser(m_url_redirect_str.GetString());
						if (!urlParser.IsValid()) {
							m_path = m_url_redirect_str;
							m_url_redirect_str = CUrlParser::CombineUrl(m_schemeName + L"://" + m_host, m_url_redirect_str);

							bNewRequestPath = true;
							break;
						}

						return E_CHANGED_STATE;
					}
				}

				return E_FAIL;
			}

			break;
		}

		if (bNewRequestPath) {
			continue;
		}

		break;
	}

	return S_OK;
}

HRESULT CHTTPAsync::ReadInternal(PBYTE pBuffer, DWORD dwSizeToRead, DWORD& dwSizeRead, DWORD dwTimeOut)
{
	CheckPointer(m_hRequest, E_FAIL);

	std::unique_lock<std::mutex> lock(m_mutexRequest);

	if (!m_bRequestComplete) {
		DLog(L"CHTTPAsync::ReadInternal() : previous request has not completed, exit");
		return E_FAIL;
	}

	m_context = Context::CONTEXT_REQUEST;

	DWORD _dwSizeRead = 0;
	DWORD _dwSizeToRead = dwSizeToRead;

	while (_dwSizeToRead) {
		INTERNET_BUFFERS InetBuff = { sizeof(InetBuff) };
		InetBuff.lpvBuffer = &pBuffer[_dwSizeRead];
		InetBuff.dwBufferLength = _dwSizeToRead;

		if (!InternetReadFileExW(m_hRequest,
			&InetBuff,
			IRF_ASYNC,
			(DWORD_PTR)this)) {
			CheckLastError(L"InternetReadFileExW()", E_FAIL);

			if (WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT) {
				DLog(L"CHTTPAsync::ReadInternal() : InternetReadFileExW() - %u ms time out reached, exit", dwTimeOut);
				m_bRequestComplete = FALSE;
				return E_FAIL;
			}
		}

		if (!InetBuff.dwBufferLength) {
			break;
		}

		_dwSizeRead += InetBuff.dwBufferLength;
		_dwSizeToRead -= InetBuff.dwBufferLength;
	};

	dwSizeRead = _dwSizeRead;

	return _dwSizeRead ? S_OK : S_FALSE;
}

HRESULT CHTTPAsync::Read(PBYTE pBuffer, DWORD dwSizeToRead, DWORD& dwSizeRead, DWORD dwTimeOut/* = INFINITE*/)
{
	dwSizeRead = 0;

	if (m_http_chunk.use) {
		HRESULT hr = S_OK;
		auto begin = pBuffer;
		DWORD sizeRead = 0;
		for (;;) {
			auto size = std::min<DWORD>(dwSizeToRead - dwSizeRead, m_http_chunk.end - m_http_chunk.read);
			hr = ReadInternal(begin, size, sizeRead, dwTimeOut);
			if (hr != S_OK) {
				break;
			}

			dwSizeRead += sizeRead;
			m_http_chunk.read += sizeRead;

			if (m_http_chunk.read == m_http_chunk.end && m_lenght > m_http_chunk.end) {
				hr = Seek(m_http_chunk.end);
				if (hr != S_OK) {
					break;
				}

				if (dwSizeRead < dwSizeToRead) {
					begin += sizeRead;
					continue;
				}
			}

			break;
		}

		return hr;
	} else {
		return ReadInternal(pBuffer, dwSizeToRead, dwSizeRead, dwTimeOut);
	}
}

HRESULT CHTTPAsync::SeekInternal(UINT64 position)
{
	if (!m_lenght || position > m_lenght) {
		return E_FAIL;
	}

	CStringW customHeader; customHeader.Format(L"Range: bytes=%I64u-\r\n", position);
	return SendRequest(customHeader);
}

HRESULT CHTTPAsync::RangeInternal(UINT64 start, UINT64 end)
{
	if (start >= end) {
		return E_FAIL;
	}

	if (!m_lenght || end > m_lenght) {
		return E_FAIL;
	}

	CStringW customHeader; customHeader.Format(L"Range: bytes=%I64u-%I64u\r\n", start, end);
	return SendRequest(customHeader);
}

HRESULT CHTTPAsync::Seek(UINT64 position)
{
	if (m_http_chunk.use) {
		m_http_chunk.size = std::min(m_lenght - position, googlemedia_maximum_chunk_size);
		m_http_chunk.start = m_http_chunk.read = position;
		m_http_chunk.end = m_http_chunk.start + m_http_chunk.size;

		return RangeInternal(m_http_chunk.start, m_http_chunk.end - 1);
	} else {
		return SeekInternal(position);
	}
}

constexpr size_t decompressBlockSize = 1024;
bool CHTTPAsync::GetUncompressed(std::vector<BYTE>& buffer)
{
	if (!m_bIsCompressed) {
		return false;
	}

	if (!m_lenght) {
		return false;
	}

	buffer.clear();

	int ret = {};
	z_stream stream = {};
	if ((ret = inflateInit2(&stream, 32 + 15)) != Z_OK) {
		return false;
	}

	std::vector<BYTE> compressedData(m_lenght);
	DWORD dwSizeRead = 0;
	if (Read(compressedData.data(), m_lenght, dwSizeRead) != S_OK) {
		inflateEnd(&stream);
		return false;
	}

	stream.next_in = compressedData.data();
	stream.avail_in = static_cast<uInt>(compressedData.size());

	size_t n = 0;
	do {
		buffer.resize(++n * decompressBlockSize);
		auto dst = buffer.data();
		stream.next_out = &dst[(n - 1) * decompressBlockSize];
		stream.avail_out = decompressBlockSize;
		if ((ret = inflate(&stream, Z_NO_FLUSH)) != Z_OK && ret != Z_STREAM_END) {
			buffer.clear();
			break;
		}
	} while (stream.avail_out == 0 && stream.avail_in != 0 && ret != Z_STREAM_END);

	inflateEnd(&stream);

	if (!buffer.empty()) {
		buffer.resize(static_cast<size_t>(stream.total_out));
	}

	return !buffer.empty();
}

const CStringA& CHTTPAsync::GetHeader() const
{
	return m_header;
}

const CStringA& CHTTPAsync::GetContentType() const
{
	return m_contentType;
}

const CStringA& CHTTPAsync::GetContentEncoding() const
{
	return m_contentEncoding;
}

const bool CHTTPAsync::IsSupportsRanges() const
{
	return m_bSupportsRanges;
}

const bool CHTTPAsync::IsGoogleMedia() const
{
	return m_bIsGoogleMedia;
}

const bool CHTTPAsync::IsCompressed() const
{
	return m_bIsCompressed;
}

UINT64 CHTTPAsync::GetLenght() const
{
	return m_lenght;
}

const CStringW& CHTTPAsync::GetRedirectURL() const
{
	return m_url_redirect_str;
}
