/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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

#include "FileDialogs.h"
#include "Subtitles/TextFile.h"

//
// CSaveTextFileDialog
//

class CSaveTextFileDialog : public CSaveFileDialog
{
	DECLARE_DYNAMIC(CSaveTextFileDialog)

private:
	UINT m_e;

public:
	CSaveTextFileDialog(
		UINT e,
		LPCWSTR lpszDefExt = nullptr, LPCWSTR lpszFileName = nullptr,
		LPCWSTR lpszFilter = nullptr, CWnd* pParentWnd = nullptr);
	~CSaveTextFileDialog() = default;

	CComboBox m_encoding;

	UINT GetEncoding() { return m_e; }

protected:
	virtual BOOL OnFileNameOK();
};

//
// CSaveSubtitleFileDialog
//

class CSaveSubtitleFileDialog : public CSaveTextFileDialog
{
	BOOL m_bSaveExternalStyleFile;

	DECLARE_DYNAMIC(CSaveSubtitleFileDialog)

public:
	CSaveSubtitleFileDialog(
		UINT e,
		LPCWSTR lpszDefExt = nullptr, LPCWSTR lpszFileName = nullptr,
		LPCWSTR lpszFilter = nullptr, CWnd* pParentWnd = nullptr,
		BOOL bSaveExternalStyleFile = FALSE);

	BOOL GetSaveExternalStyleFile() const { return m_bSaveExternalStyleFile; }

protected:
	virtual BOOL OnFileNameOK();
};
