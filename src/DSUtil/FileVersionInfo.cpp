/*
 * (C) 2003-2006 Gabest
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
#include "FileVersionInfo.h"

CFileVersionInfo::CFileVersionInfo()
{
}

CFileVersionInfo::~CFileVersionInfo()
{
}

BOOL CFileVersionInfo::GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD &dwId, BOOL bPrimaryEnough/*= FALSE*/)
{
	LPWORD lpwData;

	for (lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData) + unBlockSize; lpwData += 2) {
		if (*lpwData == wLangId) {
			dwId = *((DWORD*)lpwData);
			return TRUE;
		}
	}

	if (!bPrimaryEnough) {
		return FALSE;
	}

	for (lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData) + unBlockSize; lpwData += 2) {
		if (((*lpwData)&0x00FF) == (wLangId&0x00FF)) {
			dwId = *((DWORD*)lpwData);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CFileVersionInfo::Create(LPCWSTR lpszFileName, VS_FIXEDFILEINFO& FileInfo)
{
	FullFileInfo fullFileInfo;

	return Create(lpszFileName, FileInfo, fullFileInfo);
}

BOOL CFileVersionInfo::Create(LPCWSTR lpszFileName, VS_FIXEDFILEINFO& FileInfo, FullFileInfo& fullFileInfo)
{
	DWORD dwHandle;
	DWORD dwFileVersionInfoSize = GetFileVersionInfoSizeW(lpszFileName, &dwHandle);

	ZeroMemory(&FileInfo, sizeof(FileInfo));

	if (!dwFileVersionInfoSize) {
		return FALSE;
	}

	LPVOID lpData = (LPVOID)DNew BYTE[dwFileVersionInfoSize];

	if (!lpData) {
		return FALSE;
	}

	try
	{
		if (!GetFileVersionInfoW(lpszFileName, dwHandle, dwFileVersionInfoSize, lpData)) {
			throw FALSE;
		}

		LPVOID lpInfo;
		UINT   unInfoLen;
		if (VerQueryValueW(lpData, L"\\", &lpInfo, &unInfoLen)) {
			ASSERT(unInfoLen == sizeof(FileInfo));
			if (unInfoLen == sizeof(FileInfo)) {
				memcpy(&FileInfo, lpInfo, unInfoLen);
			}
		}

		VerQueryValueW(lpData, L"\\VarFileInfo\\Translation", &lpInfo, &unInfoLen);

		DWORD dwLangCode = 0;
		if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, FALSE)) {
			if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, TRUE)) {
				if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), dwLangCode, TRUE)) {
					if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), dwLangCode, TRUE))
						// use the first one we can get
						dwLangCode = *((DWORD*)lpInfo);
				}
			}
		}

		CString strSubBlock;
		strSubBlock.Format(L"\\StringFileInfo\\%04X%04X\\", dwLangCode&0x0000FFFF, (dwLangCode&0xFFFF0000)>>16);

		if (VerQueryValueW(lpData, strSubBlock + L"CompanyName", &lpInfo, &unInfoLen)) {
			fullFileInfo.strCompanyName = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"FileDescription", &lpInfo, &unInfoLen)) {
			fullFileInfo.strFileDescription = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"FileVersion", &lpInfo, &unInfoLen)) {
			fullFileInfo.strFileVersion = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"InternalName", &lpInfo, &unInfoLen)) {
			fullFileInfo.strInternalName = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"LegalCopyright", &lpInfo, &unInfoLen)) {
			fullFileInfo.strLegalCopyright = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"OriginalFileName", &lpInfo, &unInfoLen)) {
			fullFileInfo.strOriginalFileName = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"ProductName", &lpInfo, &unInfoLen)) {
			fullFileInfo.strProductName = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"ProductVersion", &lpInfo, &unInfoLen)) {
			fullFileInfo.strProductVersion = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"Comments", &lpInfo, &unInfoLen)) {
			fullFileInfo.strComments = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"LegalTrademarks", &lpInfo, &unInfoLen)) {
			fullFileInfo.strLegalTrademarks = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"PrivateBuild", &lpInfo, &unInfoLen)) {
			fullFileInfo.strPrivateBuild = CString((LPCWSTR)lpInfo);
		}
		if (VerQueryValueW(lpData, strSubBlock + L"SpecialBuild", &lpInfo, &unInfoLen)) {
			fullFileInfo.strSpecialBuild = CString((LPCWSTR)lpInfo);
		}

		delete[] lpData;
	}
	catch (BOOL) {
		delete[] lpData;
		return FALSE;
	}

	return TRUE;
}

/*
WORD CFileVersionInfo::GetFileVersion(int nIndex)
{
	switch(nIndex) {
		case 0:
			return (WORD)(m_FileInfo.dwFileVersionLS & 0x0000FFFF);
		case 1:
			return (WORD)((m_FileInfo.dwFileVersionLS & 0xFFFF0000) >> 16);
		case 2:
			return (WORD)(m_FileInfo.dwFileVersionMS & 0x0000FFFF);
		case 3:
			return (WORD)((m_FileInfo.dwFileVersionMS & 0xFFFF0000) >> 16);
		default:
			return 0;
	}
}

WORD CFileVersionInfo::GetProductVersion(int nIndex)
{
	switch(nIndex) {
		case 0:
			return (WORD)(m_FileInfo.dwProductVersionLS & 0x0000FFFF);
		case 1:
			return (WORD)((m_FileInfo.dwProductVersionLS & 0xFFFF0000) >> 16);
		case 2:
			return (WORD)(m_FileInfo.dwProductVersionMS & 0x0000FFFF);
		case 3:
			return (WORD)((m_FileInfo.dwProductVersionMS & 0xFFFF0000) >> 16);
		default:
			return 0;
	}
}
*/

CString CFileVersionInfo::GetFileVersionEx(LPCWSTR lpszFileName)
{
	CString          strFileVersion;
	VS_FIXEDFILEINFO FileInfo;
	if (Create(lpszFileName, FileInfo)) {
		strFileVersion.Format(L"%d.%d.%d.%d",
							 (FileInfo.dwFileVersionMS & 0xFFFF0000) >> 16,
							 (FileInfo.dwFileVersionMS & 0x0000FFFF),
							 (FileInfo.dwFileVersionLS & 0xFFFF0000) >> 16,
							 (FileInfo.dwFileVersionLS & 0x0000FFFF));
	}

	return strFileVersion;
}

CString CFileVersionInfo::GetFileVersionExShort(LPCWSTR lpszFileName)
{
	CString          strFileVersion;
	VS_FIXEDFILEINFO FileInfo;
	if (Create(lpszFileName, FileInfo)) {
		strFileVersion.Format(L"%d.%d.%d",
							 (FileInfo.dwFileVersionMS & 0xFFFF0000) >> 16,
							 (FileInfo.dwFileVersionMS & 0x0000FFFF),
							 (FileInfo.dwFileVersionLS & 0xFFFF0000) >> 16);
	}

	return strFileVersion;
}

QWORD CFileVersionInfo::GetFileVersion(LPCWSTR lpszFileName)
{
	QWORD            qwFileVersion = 0;
	VS_FIXEDFILEINFO FileInfo;
	if (Create(lpszFileName, FileInfo)) {
		qwFileVersion = ((QWORD)FileInfo.dwFileVersionMS << 32) | FileInfo.dwFileVersionLS;
	}

	return qwFileVersion;
}
