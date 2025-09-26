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

#include <ExtLib/ui/ResizableLib/ResizableDialog.h>

//
// COpenDlg dialog
//

class COpenDlg : public CResizableDialog
{
	//DECLARE_DYNAMIC(COpenDlg)

public:
	COpenDlg(CWnd* pParent = nullptr);
	virtual ~COpenDlg();

	bool m_bMultipleFiles = false;
	std::list<CStringW> m_fns;

	enum { IDD = IDD_OPEN_DLG };
	BOOL m_bPasteClipboardURL = FALSE;
	CComboBox m_mrucombo;
	CStringW m_path;
	CComboBox m_mrucombo2;
	CStringW m_path2;
	BOOL m_bAppendPlaylist = FALSE;

	HICON m_hIcon;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedBrowsebutton();
	afx_msg void OnBnClickedBrowsebutton2();
	afx_msg void OnBnClickedOk();
	afx_msg void OnUpdateDub(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
};
