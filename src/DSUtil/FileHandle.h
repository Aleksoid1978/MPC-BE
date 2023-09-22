/*
 * (C) 2011-2023 see Authors.txt
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

CStringW GetFileOnly(LPCWSTR Path);
CStringW GetFolderOnly(LPCWSTR Path);
CStringW AddSlash(LPCWSTR Path);
CStringW RemoveSlash(LPCWSTR Path);
CStringW GetFileExt(LPCWSTR Path);
CStringW RenameFileExt(LPCWSTR Path, LPCWSTR Ext);
CStringW RemoveFileExt(LPCWSTR Path);
CStringW AddExtension(LPCWSTR Path, LPCWSTR Ext);
BOOL     GetTemporaryFilePath(CStringW strExtension, CStringW& strFileName);
CStringW CompactPath(LPCWSTR Path, UINT cchMax);

// Get path of specified module
CStringW GetModulePath(HMODULE hModule);

// Get path of the executable file of the current process
CStringW GetProgramPath();

// Get programm directory with slash
CStringW GetProgramDir();

// Get application path from "App Paths" subkey
CStringW GetRegAppPath(LPCWSTR appFileName, const bool bUser);

// Searches and checks for the presence of an executable file
// in the application folder,
// in the folders described in the PATH environment variable,
// in "App paths" (if bLookAppPaths is true).
// Returns the full path to the existing executable file, otherwise an empty string.
CStringW GetFullExePath(const CStringW exePath, const bool bLookAppPaths);

void CleanPath(CStringW& path);

bool CFileGetStatus(LPCWSTR lpszFileName, CFileStatus& status);

/////

HRESULT FileOperationDelete(const CStringW& path);

// Copy or move file.
// 'func' can be FO_MOVE or FO_COPY
HRESULT FileOperation(LPCWSTR source, LPCWSTR target, const UINT func, const DWORD flags);

// Copy or move file or folder.
// If 'newName' is nullptr then the original name does not change
// func can be FO_MOVE or FO_COPY
HRESULT FileOperation(LPCWSTR source, LPCWSTR destFolder, LPCWSTR newName, const UINT func, const DWORD flags);
