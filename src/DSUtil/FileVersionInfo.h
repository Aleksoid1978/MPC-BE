/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2026 see Authors.txt
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

//#include <winver.h>

struct FullFileInfo {
	CStringW strCompanyName;
	CStringW strFileDescription;
	CStringW strFileVersion;
	CStringW strInternalName;
	CStringW strLegalCopyright;
	CStringW strOriginalFileName;
	CStringW strProductName;
	CStringW strProductVersion;
	CStringW strComments;
	CStringW strLegalTrademarks;
	CStringW strPrivateBuild;
	CStringW strSpecialBuild;
};

class CFileVersionInfo
{
public:
	CFileVersionInfo();
	virtual ~CFileVersionInfo();

	static BOOL     Create(LPCWSTR lpszFileName, VS_FIXEDFILEINFO& FileInfo);
	static BOOL     Create(LPCWSTR lpszFileName, VS_FIXEDFILEINFO& FileInfo, FullFileInfo& fullFileInfo);

	static CStringW GetFileVersionEx(LPCWSTR lpszFileName);
	static CStringW GetFileVersionExShort(LPCWSTR lpszFileName);
	static UINT64   GetFileVersion(LPCWSTR lpszFileName);

protected:
	static BOOL GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD &dwId, BOOL bPrimaryEnough = FALSE);
};
