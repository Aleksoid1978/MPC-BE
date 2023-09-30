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

#include "stdafx.h"
#include <filesystem>
#include "Log.h"
#include "FileHandle.h"
#include "text.h"

//
// Returns the file portion from a path
//
CStringW GetFileOnly(LPCWSTR Path)
{
	CStringW cs = Path;
	::PathStripPathW(cs.GetBuffer(0));
	cs.ReleaseBuffer(-1);
	return cs;
}

//
// Returns the folder portion from a path
//
CStringW GetFolderOnly(LPCWSTR Path)
{
	CStringW cs = Path; // Force CStringW to make a copy
	::PathRemoveFileSpecW(cs.GetBuffer(0));
	cs.ReleaseBuffer(-1);
	return cs;
}

//
// Adds a backslash to the end of a path if it is needed
//
CStringW AddSlash(LPCWSTR Path)
{
	CStringW cs = Path;
	::PathAddBackslashW(cs.GetBuffer(MAX_PATH));
	cs.ReleaseBuffer(-1);
	if(cs.IsEmpty()) {
		cs = L"\\";
	}
	return cs;
}

//
// Removes a backslash from the end of a path if it is there
//
CStringW RemoveSlash(LPCWSTR Path)
{
	CString cs = Path;
	::PathRemoveBackslashW(cs.GetBuffer(MAX_PATH));
	cs.ReleaseBuffer(-1);
	return cs;
}

//
// Returns just the .ext part of the file path
//
CStringW GetFileExt(LPCWSTR Path)
{
	if (::PathIsURLW(Path)) {
		auto q = wcschr(Path, '?');
		if (q) {
			CStringW ext = ::PathFindExtensionW(CStringW(Path, q - Path));
			return ext;
		}
	}
	CStringW ext = ::PathFindExtensionW(Path);
	return ext;
}

//
// Exchanges one file extension for another and returns the new fiel path
//
CStringW RenameFileExt(LPCWSTR Path, LPCWSTR Ext)
{
	CStringW cs = Path;
	::PathRenameExtensionW(cs.GetBuffer(MAX_PATH), Ext);
	cs.ReleaseBuffer(-1);
	return cs;
}

//
// Removes the file name extension from a path, if one is present
//
CStringW RemoveFileExt(LPCWSTR Path)
{
	CStringW cs = Path;
	::PathRemoveExtensionW(cs.GetBuffer(MAX_PATH));
	cs.ReleaseBuffer(-1);
	return cs;
}

CStringW AddExtension(LPCWSTR Path, LPCWSTR Ext)
{
	CStringW cs = Path;
	::PathAddExtensionW(cs.GetBuffer(MAX_PATH), Ext);
	cs.ReleaseBuffer(-1);
	return cs;
}

//
// Generate temporary files with any extension
//
BOOL GetTemporaryFilePath(CStringW strExtension, CStringW& strFileName)
{
	WCHAR lpszTempPath[MAX_PATH] = { 0 };
	if (!GetTempPathW(MAX_PATH, lpszTempPath)) {
		return FALSE;
	}

	WCHAR lpszFilePath[MAX_PATH] = { 0 };
	do {
		if (!GetTempFileNameW(lpszTempPath, L"mpc", 0, lpszFilePath)) {
			return FALSE;
		}

		DeleteFile(lpszFilePath);

		strFileName = lpszFilePath;
		strFileName.Replace(L".tmp", strExtension);

		DeleteFile(strFileName);
	} while (_waccess(strFileName, 00) != -1);

	return TRUE;
}

//
// Compact Path
//
CStringW CompactPath(LPCWSTR Path, UINT cchMax)
{
	CStringW cs = Path;
	WCHAR pathbuf[MAX_PATH] = { 0 };
	if (::PathCompactPathExW(pathbuf, cs, cchMax, 0)) {
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
	auto get_prog_dir = []() {
		CStringW path = GetModulePath(nullptr);
		path.Truncate(path.ReverseFind('\\') + 1); // do not use this method for random paths
		return path;
	};
	static const CStringW progDir = get_prog_dir();

	return progDir;
}

//
// Get application path from "App Paths" subkey
//
CStringW GetRegAppPath(LPCWSTR appFileName, const bool bCurrentUser)
{
	CStringW keyName(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
	keyName.Append(appFileName);

	const HKEY hKeyParent = bCurrentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
	CStringW appPath;
	CRegKey key;

	if (ERROR_SUCCESS == key.Open(hKeyParent, keyName, KEY_READ)) {
		ULONG nChars = 0;
		if (ERROR_SUCCESS == key.QueryStringValue(nullptr, nullptr, &nChars)) {
			if (ERROR_SUCCESS == key.QueryStringValue(nullptr, appPath.GetBuffer(nChars), &nChars)) {
				appPath.ReleaseBuffer(nChars);
			}
		}
		key.Close();
	}

	return appPath;
}

CStringW GetFullExePath(const CStringW exePath, const bool bLookAppPaths)
{
	if (exePath.IsEmpty()) {
		return L"";
	}

	if (StartsWith(exePath, L":\\", 1) || StartsWith(exePath, L"\\\\")) {
		// looks like full path

		if (::PathFileExistsW(exePath)) {
			return exePath;
		} else {
			return L""; // complete the checks anyway
		}
	}

	CStringW apppath;
	// look for in the application folder and in the folders described in the PATH environment variable
	int length = SearchPathW(nullptr, exePath.GetString(), nullptr, MAX_PATH, apppath.GetBuffer(MAX_PATH), nullptr);
	if (length > 0 && length <= MAX_PATH) {
		apppath.ReleaseBufferSetLength(length);
		return apppath;
	}

	if (bLookAppPaths && exePath.Find('\\') < 0) {
		// CreateProcessW and other functions (unlike ShellExecuteExW) does not look for
		// an executable file in the "App Paths", so we will do it manually.
		// see "App Paths" in HKEY_CURRENT_USER
		apppath = GetRegAppPath(exePath, true);
		if (apppath.GetLength() && ::PathFileExistsW(apppath)) {
			return apppath;
		}
		// see "App Paths" in HKEY_LOCAL_MACHINE
		apppath = GetRegAppPath(exePath, false);
		if (apppath.GetLength() && ::PathFileExistsW(apppath)) {
			return apppath;
		}
	}

	return L"";
}

void CleanPath(CStringW& path)
{
	// remove double quotes enclosing path
	path.Trim();
	if (path.GetLength() >= 2 && path[0] == '\"' && path[path.GetLength() - 1] == '\"') {
		path = path.Mid(1, path.GetLength() - 2);
	}
};

bool CFileGetStatus(LPCWSTR lpszFileName, CFileStatus& status)
{
	try {
		return !!CFile::GetStatus(lpszFileName, status);
	}
	catch (CException* e) {
		// MFCBUG: E_INVALIDARG / "Parameter is incorrect" is thrown for certain cds (vs2003)
		// http://groups.google.co.uk/groups?hl=en&lr=&ie=UTF-8&threadm=OZuXYRzWDHA.536%40TK2MSFTNGP10.phx.gbl&rnum=1&prev=/groups%3Fhl%3Den%26lr%3D%26ie%3DISO-8859-1
		DLog(L"CFile::GetStatus() has thrown an exception");
		e->Delete();
		return false;
	}
}

/////

HRESULT FileOperationDelete(const CStringW& path)
{
	CComPtr<IFileOperation> pFileOperation;
	CComPtr<IShellItem> psiItem;

	HRESULT hr = CoCreateInstance(CLSID_FileOperation,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pFileOperation));

	if (SUCCEEDED(hr)) {
		hr = SHCreateItemFromParsingName(path, nullptr, IID_PPV_ARGS(&psiItem));
	}
	if (SUCCEEDED(hr)) {
		hr = pFileOperation->SetOperationFlags(FOF_NOCONFIRMATION | FOF_SILENT | FOF_ALLOWUNDO);
	}
	if (SUCCEEDED(hr)) {
		hr = pFileOperation->DeleteItem(psiItem, nullptr);
	}
	if (SUCCEEDED(hr)) {
		hr = pFileOperation->PerformOperations();
	}
	if (SUCCEEDED(hr)) {
		BOOL opAborted = FALSE;
		pFileOperation->GetAnyOperationsAborted(&opAborted);
		if (opAborted) {
			hr = E_ABORT;
		}
	}

	return hr;
}

HRESULT FileOperation(LPCWSTR source, LPCWSTR target, const UINT func, const DWORD flags)
{
	LPCWSTR pszNewName = PathFindFileNameW(target);
	const CStringW destinationFolder = GetFolderOnly(target);

	return FileOperation(source, destinationFolder, pszNewName, func, flags);
}

HRESULT FileOperation(LPCWSTR source, LPCWSTR destFolder, LPCWSTR newName, const UINT func, const DWORD flags)
{
	CComPtr<IFileOperation> pFileOperation;
	CComPtr<IShellItem> psiItem;
	CComPtr<IShellItem> psiDestinationFolder;

	HRESULT hr = CoCreateInstance(CLSID_FileOperation,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pFileOperation));

	if (SUCCEEDED(hr)) {
		hr = SHCreateItemFromParsingName(source, nullptr, IID_PPV_ARGS(&psiItem));
	}
	if (SUCCEEDED(hr)) {
		hr = SHCreateItemFromParsingName(destFolder, nullptr, IID_PPV_ARGS(&psiDestinationFolder));
	}
	if (SUCCEEDED(hr)) {
		hr = pFileOperation->SetOperationFlags(flags);
	}
	if (SUCCEEDED(hr)) {
		if (func == FO_MOVE) {
			hr = pFileOperation->MoveItem(psiItem, psiDestinationFolder, newName, nullptr);
		}
		else if (func == FO_COPY) {
			hr = pFileOperation->CopyItem(psiItem, psiDestinationFolder, newName, nullptr);
		}
	}
	if (SUCCEEDED(hr)) {
		hr = pFileOperation->PerformOperations();
	}
	if (SUCCEEDED(hr)) {
		BOOL opAborted = FALSE;
		pFileOperation->GetAnyOperationsAborted(&opAborted);
		if (opAborted) {
			hr = E_ABORT;
		}
	}

	return hr;
}
