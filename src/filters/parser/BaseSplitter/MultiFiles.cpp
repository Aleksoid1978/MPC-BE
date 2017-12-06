/*
 * (C) 2006-2017 see Authors.txt
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
#include "MultiFiles.h"

IMPLEMENT_DYNAMIC(CMultiFiles, CObject)

CMultiFiles::CMultiFiles()
{
}

void CMultiFiles::Reset()
{
	m_strFiles.clear();
	m_FilesSize.clear();
	m_rtPtsOffsets.clear();
	m_nCurPart = SIZE_T_MAX;
	m_llTotalLength = 0;
	m_pCurrentPTSOffset = nullptr;
}

BOOL CMultiFiles::Open(LPCTSTR lpszFileName)
{
	Reset();
	m_strFiles.emplace_back(lpszFileName);

	return OpenPart(0);
}

BOOL CMultiFiles::OpenFiles(CHdmvClipInfo::CPlaylist& files)
{
	LARGE_INTEGER  llSize = {};
	size_t         nPart  = 0;
	REFERENCE_TIME rtDur  = 0;

	Reset();

	for (const auto& Item : files) {
		m_strFiles.emplace_back(Item.m_strFileName);
		if (!OpenPart(nPart)) {
			return false;
		}

		llSize.QuadPart = 0;
		GetFileSizeEx(m_hFile, &llSize);
		m_llTotalLength += llSize.QuadPart;
		m_FilesSize.emplace_back(llSize.QuadPart);

		m_rtPtsOffsets.emplace_back((Item.m_rtStartTime - Item.m_rtIn) + files.begin()->m_rtIn);
		rtDur += Item.Duration();
		nPart++;
	}

	if (files.size() > 1) {
		ClosePart();
	}

	return TRUE;
}

ULONGLONG CMultiFiles::Seek(LONGLONG lOff, UINT nFrom)
{
	LARGE_INTEGER llNewPos = {};
	LARGE_INTEGER llOff    = {};

	if (m_strFiles.size() == 1) {
		llOff.QuadPart = lOff;
		SetFilePointerEx(m_hFile, llOff, &llNewPos, nFrom);

		return llNewPos.QuadPart;
	} else {
		LONGLONG lAbsolutePos = GetAbsolutePosition(lOff, nFrom);
		size_t   nNewPart     = 0;
		LONGLONG llSum        = 0;

		while (m_FilesSize[nNewPart] + llSum <= lAbsolutePos) {
			llSum += m_FilesSize[nNewPart];
			nNewPart++;
		}

		OpenPart(nNewPart);
		llOff.QuadPart = lAbsolutePos - llSum;
		SetFilePointerEx(m_hFile, llOff, &llNewPos, FILE_BEGIN);

		return llSum + llNewPos.QuadPart;
	}
}

LONGLONG CMultiFiles::GetAbsolutePosition(LONGLONG lOff, UINT nFrom)
{
	LARGE_INTEGER llNoMove = {};
	LARGE_INTEGER llCurPos = {};

	switch (nFrom) {
		case FILE_BEGIN :
			return lOff;
		case FILE_CURRENT :
			SetFilePointerEx(m_hFile, llNoMove, &llCurPos, FILE_CURRENT);
			return llCurPos.QuadPart + lOff;
		case FILE_END :
			return m_llTotalLength - lOff;
		default:
			return 0; // just used to quash "not all control paths return a value" warning
	}
}

ULONGLONG CMultiFiles::GetLength() const
{
	if (m_strFiles.size() == 1) {
		LARGE_INTEGER llSize = {};
		GetFileSizeEx(m_hFile, &llSize);
		return llSize.QuadPart;
	} else {
		return m_llTotalLength;
	}
}

UINT CMultiFiles::Read(BYTE* lpBuf, UINT nCount, DWORD& dwError)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		dwError = ERROR_INVALID_HANDLE;
		return 0;
	}

	dwError         = ERROR_SUCCESS;
	DWORD dwRead    = 0;
	size_t nCurPart = 0;

	do {
		LARGE_INTEGER llNoMove = {};
		LARGE_INTEGER llCurPos = {};
		SetFilePointerEx(m_hFile, llNoMove, &llCurPos, FILE_CURRENT);

again:
		DWORD nNumberOfBytesRead = 0;
		if (!ReadFile(m_hFile, lpBuf, nCount - dwRead, &nNumberOfBytesRead, nullptr)) {
			const DWORD dwLastError = GetLastError();
			if (dwLastError != ERROR_SUCCESS) {
				// TODO - select only the necessary error codes : ERROR_INVALID_HANDLE, ERROR_BAD_NETPATH, ERROR_DEV_NOT_EXIST, ERROR_FILE_INVALID ...
				nCurPart = m_nCurPart;
				ClosePart();
				if (OpenPart(nCurPart)) {
					LARGE_INTEGER llNewPos = {};
					if (SetFilePointerEx(m_hFile, llCurPos, &llNewPos, FILE_BEGIN) && llCurPos.QuadPart == llNewPos.QuadPart) {
						goto again;
					}
				}
			}

			dwError = dwLastError;
			break;
		}

		dwRead   += nNumberOfBytesRead;
		nCurPart = m_nCurPart;

		if (dwRead != nCount && (nCurPart == SIZE_T_MAX || nCurPart < m_strFiles.size() - 1)) {
			OpenPart(nCurPart + 1);
			lpBuf += dwRead;
		}
	} while (nCount != dwRead && (nCurPart == SIZE_T_MAX || nCurPart < m_strFiles.size() - 1));

	return dwRead;
}

void CMultiFiles::Close()
{
	ClosePart();
	Reset();
}

CMultiFiles::~CMultiFiles()
{
	Close();
}

BOOL CMultiFiles::OpenPart(size_t nPart)
{
	if (m_nCurPart == nPart) {
		return TRUE;
	} else {
		ClosePart();

		const CString& lpFileName = m_strFiles[nPart];
		m_hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (m_hFile != INVALID_HANDLE_VALUE) {
			m_nCurPart = nPart;
			if (m_pCurrentPTSOffset) {
				*m_pCurrentPTSOffset = m_rtPtsOffsets[nPart];
			}
		}

		return (m_hFile != INVALID_HANDLE_VALUE);
	}
}

void CMultiFiles::ClosePart()
{
	if (m_hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hFile);
		m_hFile    = INVALID_HANDLE_VALUE;
		m_nCurPart = SIZE_T_MAX;
	}
}
