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

#pragma once

#include <wininet.h>
#include <mutex>
#include "UrlParser.h"
#include "mpc_defines.h"

namespace http {
	inline CStringW userAgent = L"Mozilla/5.0";
	inline DWORD connectTimeout = 10000;
	inline DWORD readTimeout = 10000;
}

class CHTTPAsync
{
protected:
	std::mutex m_mutexRequest;

	enum Context {
		CONTEXT_INITIAL = -1,
		CONTEXT_CONNECT,
		CONTEXT_REQUEST
	};
	Context m_context = CONTEXT_INITIAL;

	HANDLE m_hConnectedEvent       = INVALID_HANDLE_VALUE;
	HANDLE m_hRequestOpenedEvent   = INVALID_HANDLE_VALUE;
	HANDLE m_hRequestCompleteEvent = INVALID_HANDLE_VALUE;
	BOOL m_bRequestComplete        = TRUE;

	HINTERNET m_hInstance = nullptr;
	HINTERNET m_hConnect  = nullptr;
	HINTERNET m_hRequest  = nullptr;

	CStringW m_url_str;
	CStringW m_host;
	CStringW m_path;
	CStringW m_schemeName;

	CStringW m_url_redirect_str;

	INTERNET_PORT m_nPort     = 0;
	INTERNET_SCHEME m_nScheme = INTERNET_SCHEME_HTTP;

	CStringA m_header;
	CStringA m_contentType;
	CStringA m_contentEncoding;
	UINT64 m_lenght = 0;

	bool m_bClosing = true;

	bool m_bIsCompressed = false;

	bool m_bSupportsRanges = false;
	bool m_bIsGoogleMedia = false;

	struct http_chunk_t {
		bool use;
		UINT64 size;
		UINT64 start;
		UINT64 end;
		UINT64 read;
	} m_http_chunk = {};
	constexpr static uint64_t googlemedia_maximum_chunk_size = 10ull * MEGABYTE - 1;

	static void CALLBACK Callback(__in HINTERNET hInternet,
								  __in_opt DWORD_PTR dwContext,
								  __in DWORD dwInternetStatus,
								  __in_opt LPVOID lpvStatusInformation,
								  __in DWORD dwStatusInformationLength);

	CStringA QueryInfoStr(DWORD dwInfoLevel) const;
	DWORD QueryInfoDword(DWORD dwInfoLevel) const;

	HRESULT SeekInternal(UINT64 position);
	HRESULT RangeInternal(UINT64 start, UINT64 end);

	HRESULT ReadInternal(PBYTE pBuffer, DWORD dwSizeToRead, DWORD& dwSizeRead, DWORD dwTimeOut);

public:
	CHTTPAsync();
	virtual ~CHTTPAsync();

	void Close();

	HRESULT Connect(LPCWSTR lpszURL, DWORD dwTimeOut = INFINITE, LPCWSTR lpszCustomHeader = L"");
	HRESULT SendRequest(LPCWSTR lpszCustomHeader = L"", DWORD dwTimeOut = INFINITE, bool bNoAutoRedirect = false);
	HRESULT Read(PBYTE pBuffer, DWORD dwSizeToRead, DWORD& dwSizeRead, DWORD dwTimeOut = INFINITE);

	HRESULT Seek(UINT64 position);

	const CStringA& GetHeader() const;

	// get content type in lowercase
	const CStringA& GetContentType() const;
	// get content encoding in lowercase
	const CStringA& GetContentEncoding() const;

	const bool IsSupportsRanges() const;
	const bool IsGoogleMedia() const;

	const bool IsCompressed() const;
	bool GetUncompressed(std::vector<BYTE>& buffer);

	UINT64 GetLenght() const;

	const CStringW& GetRedirectURL() const;
};

