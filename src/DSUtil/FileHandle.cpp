/*
 * (C) 2011-2017 see Authors.txt
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
#include "FileHandle.h"

//
// Returns the file portion from a path
//
CString GetFileOnly(LPCTSTR Path)
{
	// Strip off the path and return just the filename part
	CString temp = (LPCTSTR) Path; // Force CString to make a copy
	::PathStripPath(temp.GetBuffer(0));
	temp.ReleaseBuffer(-1);
	return temp;
}

//
// Returns the folder portion from a path
//
CString GetFolderOnly(LPCTSTR Path)
{
	// Strip off the filename and return only path part
	CString temp = (LPCTSTR) Path; // Force CString to make a copy
	::PathRemoveFileSpec(temp.GetBuffer(0));
	temp.ReleaseBuffer(-1);
	return temp;
}

//
// Adds a backslash to the end of a path if it is needed
//
CString AddSlash(LPCTSTR Path)
{
	CString cs = Path;
	::PathAddBackslash(cs.GetBuffer(MAX_PATH));
	cs.ReleaseBuffer(-1);
	if(cs.IsEmpty()) {
		cs = _T("\\");
	}
	return cs;
}

//
// Removes a backslash from the end of a path if it is there
//
CString RemoveSlash(LPCTSTR Path)
{
	CString cs = Path;
	::PathRemoveBackslash(cs.GetBuffer(MAX_PATH));
	cs.ReleaseBuffer(-1);
	return cs;
}

//
// Returns just the .ext part of the file path
//
CString GetFileExt(LPCTSTR Path)
{
	CString cs;
	cs = ::PathFindExtension(Path);
	return cs;
}

//
// Exchanges one file extension for another and returns the new fiel path
//
CString RenameFileExt(LPCTSTR Path, LPCTSTR Ext)
{
	CString cs = Path;
	::PathRenameExtension(cs.GetBuffer(MAX_PATH), Ext);
	return cs;
}

//
// Generate temporary files with any extension
//
BOOL GetTemporaryFilePath(CString strExtension, CString& strFileName)
{
	TCHAR lpszTempPath[MAX_PATH] = { 0 };
	if (!GetTempPath(MAX_PATH, lpszTempPath)) {
		return FALSE;
	}

	TCHAR lpszFilePath[MAX_PATH] = { 0 };
	do {
		if (!GetTempFileName(lpszTempPath, _T("mpc"), 0, lpszFilePath)) {
			return FALSE;
		}

		DeleteFile(lpszFilePath);

		strFileName = lpszFilePath;
		strFileName.Replace(_T(".tmp"), strExtension);

		DeleteFile(strFileName);
	} while (_taccess(strFileName, 00) != -1);

	return TRUE;
}

//
// Compact Path
//
CString CompactPath(LPCTSTR Path, UINT cchMax)
{
	CString cs = Path;
	WCHAR pathbuf[MAX_PATH] = { 0 };
	if (::PathCompactPathEx(pathbuf, cs, cchMax, 0)) {
		cs = pathbuf;
	}

	return cs;
}

//
// Get Module Path
//
CString GetModulePath(HMODULE hModule)
{
	CString ret;
	int pos, len = MAX_PATH - 1;
	for (;;) {
		pos = GetModuleFileName(hModule, ret.GetBuffer(len), len);
		if (pos == len) {
			// buffer was too small, enlarge it and try again
			len *= 2;
			ret.ReleaseBuffer(0);
			continue;
		}
		ret.ReleaseBuffer(pos);
		break;
	}

	ASSERT(!ret.IsEmpty());
	return ret;
}

//
// Get path of the executable file of the current process.
//
CString GetProgramPath()
{
	return GetModulePath(nullptr);
}

//
// Get program directory
//
CString GetProgramDir()
{
	return AddSlash(GetFolderOnly(GetModulePath(nullptr)));
}
