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
// Get path of specified module
//
CStringW GetModulePath(HMODULE hModule)
{
	CStringW path;
	DWORD bufsize = MAX_PATH;
	DWORD len = 0;
	while (1) {
		len = GetModuleFileNameW(hModule, path.GetBuffer(bufsize), bufsize);
		if (len < bufsize) {
			break;
		}
		bufsize *= 2;
	}
	path.ReleaseBufferSetLength(len);

	ASSERT(path.GetLength());
	return path;
}

//
// Get path of the executable file of the current process.
//
CStringW GetProgramPath()
{
	return GetModulePath(nullptr);
}

//
// Get program directory with backslash
//
CStringW GetProgramDir()
{
	CStringW path = GetModulePath(nullptr);
	path.Truncate(path.ReverseFind('\\') + 1); // do not use this method for random paths

	return path;
}
