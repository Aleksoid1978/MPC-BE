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
#include "AsyncReader.h"
#include "DSUtil/UrlParser.h"

//
// CAsyncFileReader
//

CAsyncFileReader::CAsyncFileReader(CString fn, HRESULT& hr, BOOL bSupportURL)
	: CUnknown(L"CAsyncFileReader", nullptr, &hr)
	, m_bSupportURL(bSupportURL)
	, m_hBreakEvent(nullptr)
	, m_lOsError(0)
{
	hr = Open(fn) ? S_OK : E_FAIL;
}

CAsyncFileReader::CAsyncFileReader(CHdmvClipInfo::CPlaylist& Items, HRESULT& hr)
	: CUnknown(L"CAsyncFileReader", nullptr, &hr)
	, m_hBreakEvent(nullptr)
	, m_lOsError(0)
{
	hr = OpenFiles(Items) ? S_OK : E_FAIL;
}

STDMETHODIMP CAsyncFileReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IAsyncReader)
		QI(ISyncReader)
		QI(IFileHandle)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

BOOL CAsyncFileReader::Open(LPCWSTR lpszFileName)
{
	if (::PathIsURLW(lpszFileName)) {
		CUrlParser urlParser;
		if (m_bSupportURL
				&& urlParser.Parse(lpszFileName)
				&& m_HTTPAsync.Connect(lpszFileName, 10000) == S_OK) {
			const UINT64 ContentLength = m_HTTPAsync.GetLenght();
			if (ContentLength == 0) {
				return FALSE;
			}

			m_total = ContentLength;
			m_url = lpszFileName;
			m_sourcetype = SourceType::HTTP;

			if (m_total > http::googlemedia_maximum_chunk_size
					&& m_HTTPAsync.IsSupportsRanges() && m_HTTPAsync.IsGoogleMedia()) {
				m_http_chunk.end = m_http_chunk.size = std::min(m_total, http::googlemedia_maximum_chunk_size);
				if (m_HTTPAsync.Range(m_http_chunk.start, m_http_chunk.end - 1) == S_OK) {
					m_http_chunk.use = true;
				}
			}

			return TRUE;
		}

		return FALSE;
	}

	return __super::Open(lpszFileName);
}

ULONGLONG CAsyncFileReader::GetLength()
{
	return m_total ? m_total : __super::GetLength();
}

// IAsyncReader

STDMETHODIMP CAsyncFileReader::SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	if ((ULONGLONG)llPosition + lLength > GetLength()) {
		return E_FAIL;
	}

	if (m_url.GetLength()) {
		auto HTTPSeek = [&](UINT64 position) {
			if (m_http_chunk.use) {
				m_http_chunk.size = std::min(m_total - position, http::googlemedia_maximum_chunk_size);
				m_http_chunk.start = m_http_chunk.read = position;
				m_http_chunk.end = m_http_chunk.start + m_http_chunk.size;

				return m_HTTPAsync.Range(m_http_chunk.start, m_http_chunk.end - 1);
			} else {
				return m_HTTPAsync.Seek(position);
			}
		};

		auto HTTPRead = [&](PBYTE buffer, DWORD sizeToRead, DWORD& dwSizeRead) {
			if (m_http_chunk.use) {
				HRESULT hr = S_OK;
				auto begin = buffer;
				DWORD sizeRead = 0;
				for (;;) {
					auto size = std::min<DWORD>(sizeToRead - dwSizeRead, m_http_chunk.end - m_http_chunk.read);
					hr = m_HTTPAsync.Read(begin, size, &sizeRead);
					if (hr != S_OK) {
						break;
					}

					dwSizeRead += sizeRead;
					m_http_chunk.read += sizeRead;

					if (m_http_chunk.read == m_http_chunk.end && m_total > m_http_chunk.end) {
						hr = HTTPSeek(llPosition + sizeRead);
						if (hr != S_OK) {
							break;
						}

						if (dwSizeRead < sizeToRead) {
							begin += sizeRead;
							continue;
						}
					}

					break;
				}

				return hr;
			} else {
				return m_HTTPAsync.Read(buffer, sizeToRead, &dwSizeRead);
			}
		};

		auto RetryOnError = [&] {
			const DWORD dwError = GetLastError();
			if (dwError == ERROR_INTERNET_CONNECTION_RESET
					|| dwError == ERROR_HTTP_INVALID_SERVER_RESPONSE) {
				if (S_OK == HTTPSeek(llPosition)) {
					return true;
				}
			}
			return false;
		};

		for (;;) {
			if (m_pos != llPosition) {
				if (llPosition > m_pos && (llPosition - m_pos) <= 64 * KILOBYTE) {
					static std::vector<BYTE> pBufferTmp(64 * KILOBYTE);
					const DWORD lenght = llPosition - m_pos;

					DWORD dwSizeRead = 0;
					HRESULT hr = HTTPRead(pBufferTmp.data(), lenght, dwSizeRead);
					if (hr != S_OK || dwSizeRead != lenght) {
						if (RetryOnError()) {
							continue;
						}
						return E_FAIL;
					}
				} else {
					HRESULT hr = HTTPSeek(llPosition);
#ifdef DEBUG_OR_LOG
					DLog(L"CAsyncFileReader::SyncRead() : do HTTP seeking from %I64d to %I64d, hr = 0x%08x", m_pos, llPosition, hr);
#endif
					if (hr != S_OK) {
						return hr;
					}
				}

				m_pos = llPosition;
			}

			DWORD dwSizeRead = 0;
			HRESULT hr = HTTPRead(pBuffer, lLength, dwSizeRead);
			if (hr != S_OK || dwSizeRead != lLength) {
				if (RetryOnError()) {
					continue;
				}
				return E_FAIL;
			}
			m_pos += dwSizeRead;

			return S_OK;
		}

		return E_FAIL;
	}

	try {
		if ((ULONGLONG)llPosition != Seek(llPosition, FILE_BEGIN)) {
			return E_FAIL;
		}
		DWORD dwError;
		UINT readed = Read(pBuffer, lLength, dwError);
		if (readed < (UINT)lLength || dwError != ERROR_SUCCESS) {
			return E_FAIL;
		}

		return S_OK;
	}
	catch (CFileException* e) {
		m_lOsError = e->m_lOsError;
		e->Delete();

		return E_FAIL;
	}
}

STDMETHODIMP CAsyncFileReader::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	const LONGLONG len = GetLength();

	if (pTotal) {
		*pTotal = len;
	}
	if (pAvailable) {
		*pAvailable = len;
	}
	return S_OK;
}
