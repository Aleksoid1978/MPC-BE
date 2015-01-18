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

#pragma once

//#include <winver.h>

struct FullFileInfo {
	CString strCompanyName;
	CString strFileDescription;
	CString strFileVersion;
	CString strInternalName;
	CString strLegalCopyright;
	CString strOriginalFileName;
	CString strProductName;
	CString strProductVersion;
	CString strComments;
	CString strLegalTrademarks;
	CString strPrivateBuild;
	CString strSpecialBuild;
};

class CFileVersionInfo
{
public:
	CFileVersionInfo();
	virtual ~CFileVersionInfo();

	static BOOL		Create(LPCTSTR lpszFileName, VS_FIXEDFILEINFO& FileInfo);
	static BOOL		Create(LPCTSTR lpszFileName, VS_FIXEDFILEINFO& FileInfo, FullFileInfo& fullFileInfo);

	static CString	GetFileVersionEx(LPCTSTR lpszFileName);
	static CString	GetFileVersionExShort(LPCTSTR lpszFileName);
	static QWORD	GetFileVersion(LPCTSTR lpszFileName);

protected:
	static BOOL	GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD &dwId, BOOL bPrimaryEnough = FALSE);
};
