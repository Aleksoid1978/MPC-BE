/*
 * (C) 2024-2025 see Authors.txt
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
#include "FileDialogs.h"
#include "DSUtil/FileHandle.h"

//
// COpenFileDialog
//

IMPLEMENT_DYNAMIC(COpenFileDialog, CFileDialog)
COpenFileDialog::COpenFileDialog(
		LPCWSTR lpszDefExt,
		LPCWSTR lpszFileName,
		DWORD dwFlags,
		LPCWSTR lpszFilter,
		CWnd* pParentWnd)
	: CFileDialog(TRUE, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	CAppSettings& s = AfxGetAppSettings();
	CStringW path(lpszFileName);
	if (s.bKeepHistory && (path.IsEmpty() || ::PathIsURLW(path))) {
		path = s.strLastOpenFile;
	}

	m_strInitialDir = ::GetFolderPath(path);
	m_pOFN->lpstrInitialDir = m_strInitialDir.GetString();

	if (dwFlags & OFN_ALLOWMULTISELECT) {
		// https://learn.microsoft.com/en-us/cpp/mfc/reference/cfiledialog-class?view=msvc-170#remarks
		const int bufferlen = 100000; // needed to receive a large number of files
		m_pOFN->lpstrFile = m_strFileBuffer.GetBuffer(bufferlen);
		m_pOFN->nMaxFile  = bufferlen;
	}
}

CStringW COpenFileDialog::GetPathName()
{
	CStringW filepath;

	CComPtr<IFileOpenDialog> pFileOpenDialog = GetIFileOpenDialog();
	if (pFileOpenDialog) {
		CComPtr<IShellItem> pShellItem;
		HRESULT hr = pFileOpenDialog->GetResult(&pShellItem);
		if (SUCCEEDED(hr)) {
			LPWSTR pszName = nullptr;
			hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
			if (SUCCEEDED(hr)) {
				filepath = pszName;
				CoTaskMemFree(pszName);
			}
		}
	}

	return filepath;
}

void COpenFileDialog::GetFilePaths(std::list<CStringW>& filepaths)
{
	filepaths.clear();

	CComPtr<IFileOpenDialog> pFileOpenDialog = GetIFileOpenDialog();
	if (pFileOpenDialog) {
		CComPtr<IShellItemArray> pItemArray;
		HRESULT hr = pFileOpenDialog->GetResults(&pItemArray);
		if (SUCCEEDED(hr)) {
			DWORD dwNumItems = 0;
			hr = pItemArray->GetCount(&dwNumItems);
			if (SUCCEEDED(hr)) {
				for (DWORD i = 0; i < dwNumItems; ++i) {
					CComPtr<IShellItem> pItem;
					hr = pItemArray->GetItemAt(i, &pItem);
					if (SUCCEEDED(hr)) {
						LPWSTR pszName = nullptr;
						hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
						if (SUCCEEDED(hr)) {
							filepaths.emplace_back(pszName);
							CoTaskMemFree(pszName);
						}
					}
				}
			}
		}
	}
}

//
// CSaveFileDialog
//

IMPLEMENT_DYNAMIC(CSaveFileDialog, CFileDialog)
CSaveFileDialog::CSaveFileDialog(
		LPCWSTR lpszDefExt,
		LPCWSTR lpszFileName,
		DWORD dwFlags,
		LPCWSTR lpszFilter,
		CWnd* pParentWnd)
	: CFileDialog(FALSE, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	m_strInitialDir = ::GetFolderPath(lpszFileName);
	m_pOFN->lpstrInitialDir = m_strInitialDir.GetString();
}

CStringW CSaveFileDialog::GetPathName()
{
	CStringW filepath;

	CComPtr<IFileSaveDialog> pFileSaveDialog = GetIFileSaveDialog();
	if (pFileSaveDialog) {
		CComPtr<IShellItem> pItem;
		HRESULT hr = pFileSaveDialog->GetResult(&pItem);
		if (SUCCEEDED(hr)) {
			LPWSTR pszName = nullptr;
			hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
			if (SUCCEEDED(hr)) {
				filepath = pszName;
				CoTaskMemFree(pszName);
			}
		}
	}

	return filepath;
}
