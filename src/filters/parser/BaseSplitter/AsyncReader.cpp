/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include <afxsock.h>
#include <afxinet.h>
#include "../../../DSUtil/DSUtil.h"

//
// CAsyncFileReader
//

CAsyncFileReader::CAsyncFileReader(CString fn, HRESULT& hr)
	: CUnknown(NAME("CAsyncFileReader"), NULL, &hr)
	, m_len((ULONGLONG)-1)
	, m_hBreakEvent(NULL)
	, m_lOsError(0)
{
	hr = Open(fn, modeRead|shareDenyNone|typeBinary|osSequentialScan) ? S_OK : E_FAIL;
	if (SUCCEEDED(hr)) {
		m_len = GetLength();
	}
}

CAsyncFileReader::CAsyncFileReader(CHdmvClipInfo::CPlaylist& Items, HRESULT& hr)
	: CUnknown(NAME("CAsyncFileReader"), NULL, &hr)
	, m_len((ULONGLONG)-1)
	, m_hBreakEvent(NULL)
	, m_lOsError(0)
{
	hr = OpenFiles(Items, modeRead|shareDenyNone|typeBinary|osSequentialScan) ? S_OK : E_FAIL;
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
			if ((ULONGLONG)llPosition+lLength > GetLength()) {
				return E_FAIL;    // strange, but the Seek below can return llPosition even if the file is not that big (?)
			}
			if ((ULONGLONG)llPosition != Seek(llPosition, begin)) {
				return E_FAIL;
			}
			if ((UINT)lLength < Read(pBuffer, lLength)) {
				return E_FAIL;
			}

#if 0 // def DEBUG
			static __int64 s_total = 0, s_laststoppos = 0;
			s_total += lLength;
			if (s_laststoppos > llPosition) {
				TRACE(_T("[%I64d - %I64d] %d (%I64d)\n"), llPosition, llPosition + lLength, lLength, s_total);
			}
			s_laststoppos = llPosition + lLength;
#endif

			return S_OK;
		} catch (CFileException* e) {
			m_lOsError = e->m_lOsError;
			e->Delete();
			Sleep(1);
			CString fn = m_strFileName;
			try {
				Close();
			} catch (CFileException* e) {
				e->Delete();
			}
			try {
				Open(fn, modeRead|shareDenyNone|typeBinary|osSequentialScan);
			} catch (CFileException* e) {
				e->Delete();
			}
			m_strFileName = fn;
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

// IFileHandle

STDMETHODIMP_(HANDLE) CAsyncFileReader::GetFileHandle()
{
	return m_hFile;
}

STDMETHODIMP_(LPCTSTR) CAsyncFileReader::GetFileName()
{
	return m_nCurPart != -1 ? m_strFiles[m_nCurPart] : m_strFiles[0];
}

STDMETHODIMP_(bool) CAsyncFileReader::IsValidFilename()
{
	return m_strFiles.IsEmpty() ? false : true;
}
