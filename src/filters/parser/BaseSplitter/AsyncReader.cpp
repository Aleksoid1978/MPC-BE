/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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

//
// CAsyncFileReader
//

CAsyncFileReader::CAsyncFileReader(CString fn, HRESULT& hr)
	: CUnknown(NAME("CAsyncFileReader"), NULL, &hr)
	, m_len(ULONGLONG_ERROR)
	, m_hBreakEvent(NULL)
	, m_lOsError(0)
{
	hr = Open(fn) ? S_OK : E_FAIL;
	if (SUCCEEDED(hr)) {
		m_len = GetLength();
	}
}

CAsyncFileReader::CAsyncFileReader(CHdmvClipInfo::CPlaylist& Items, HRESULT& hr)
	: CUnknown(NAME("CAsyncFileReader"), NULL, &hr)
	, m_len(ULONGLONG_ERROR)
	, m_hBreakEvent(NULL)
	, m_lOsError(0)
{
	hr = OpenFiles(Items) ? S_OK : E_FAIL;
	if (SUCCEEDED(hr)) {
		m_len = GetLength();
	}
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

// IAsyncReader

STDMETHODIMP CAsyncFileReader::SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	do {
		try {
			if ((ULONGLONG)llPosition + lLength > GetLength()) {
				return E_FAIL; // strange, but the Seek below can return llPosition even if the file is not that big (?)
			}
			if ((ULONGLONG)llPosition != Seek(llPosition, FILE_BEGIN)) {
				return E_FAIL;
			}
			DWORD dwError;
			if ((UINT)lLength < Read(pBuffer, lLength, dwError) || dwError != ERROR_SUCCESS) {
				return E_FAIL;
			}

			return S_OK;
		} catch (CFileException* e) {
			m_lOsError = e->m_lOsError;
			e->Delete();
			break;
		}
	} while (m_hBreakEvent && WaitForSingleObject(m_hBreakEvent, 0) == WAIT_TIMEOUT);

	return E_FAIL;
}

STDMETHODIMP CAsyncFileReader::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	if (pTotal) {
		*pTotal = m_len;
	}
	if (pAvailable) {
		*pAvailable = GetLength();
	}
	return S_OK;
}
