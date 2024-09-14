/*
 * (C) 2024 see Authors.txt
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
#include "SaveFileDialog.h"
#include "DSUtil/FileHandle.h"

// CSaveFileDialog

IMPLEMENT_DYNAMIC(CSaveFileDialog, CFileDialog)
CSaveFileDialog::CSaveFileDialog(
		LPCWSTR lpszDefExt,
		LPCWSTR lpszFileName,
		DWORD dwFlags,
		LPCWSTR lpszFilter,
		CWnd* pParentWnd)
	: CFileDialog(FALSE, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	CStringW dir = ::GetFolderPath(lpszFileName);
	int size = std::max(MAX_PATH, dir.GetLength() + 1);
	m_pstrInitialDir.reset(new WCHAR[size]);
	memset(m_pstrInitialDir.get(), 0, size * sizeof(WCHAR));
	wcscpy_s(m_pstrInitialDir.get(), size, dir.GetString());
	m_pOFN->lpstrInitialDir = m_pstrInitialDir.get();

	size += 500;
	m_pstrFile.reset(new WCHAR[size]);
	memset(m_pstrFile.get(), 0, size * sizeof(WCHAR));
	m_pOFN->lpstrFile = m_pstrFile.get();
	m_pOFN->nMaxFile = size;
}
