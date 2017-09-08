/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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

#include "../../Subtitles/TextFile.h"

// CSaveTextFileDialog

class CSaveTextFileDialog : public CFileDialog
{
	BOOL m_bDisableExternalStyleCheckBox;
	BOOL m_bSaveExternalStyleFile;

	DECLARE_DYNAMIC(CSaveTextFileDialog)

private:
	CTextFile::enc m_e;

public:
	CSaveTextFileDialog(
		CTextFile::enc e,
		LPCWSTR lpszDefExt = nullptr, LPCWSTR lpszFileName = nullptr,
		LPCWSTR lpszFilter = nullptr, CWnd* pParentWnd = nullptr,
		BOOL bDisableExternalStyleCheckBox = TRUE, BOOL bSaveExternalStyleFile = FALSE);
	virtual ~CSaveTextFileDialog();

	CComboBox m_encoding;

	CTextFile::enc GetEncoding() { return m_e; }
	BOOL GetSaveExternalStyleFile() const { return m_bSaveExternalStyleFile; }

protected:
	virtual BOOL OnFileNameOK();
};
