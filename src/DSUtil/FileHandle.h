/*
 * (C) 2011-2015 see Authors.txt
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

CString GetFileOnly(LPCTSTR Path);
CString GetFolderOnly(LPCTSTR Path);
CString AddSlash(LPCTSTR Path);
CString RemoveSlash(LPCTSTR Path);
CString GetFileExt(LPCTSTR Path);
CString RenameFileExt(LPCTSTR Path, LPCTSTR Ext);
BOOL	GetTemporaryFilePath(CString strExtension, CString& strFileName);
CString CompactPath(LPCTSTR Path, UINT cchMax);

// Get path of specified module
CStringW GetModulePath(HMODULE hModule);
// Get path of the executable file of the current process
CStringW GetProgramPath();
// Get programm directory with slash
CStringW GetProgramDir();
