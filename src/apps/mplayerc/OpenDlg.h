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

#include <afxwin.h>
#include <ResizableLib/ResizableDialog.h>

// COpenDlg dialog

class COpenDlg : public CResizableDialog
{
	//DECLARE_DYNAMIC(COpenDlg)

public:
	COpenDlg(CWnd* pParent = nullptr);
	virtual ~COpenDlg();

	bool m_bMultipleFiles;
	CAtlList<CString> m_fns;

	enum { IDD = IDD_OPEN_DLG };
	BOOL m_bPasteClipboardURL;
	CComboBox m_mrucombo;
	CString m_path;
	CComboBox m_mrucombo2;
	CString m_path2;
	CStatic m_label2;
	BOOL m_bAppendPlaylist;

	HICON	m_hIcon;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedBrowsebutton();
	afx_msg void OnBnClickedBrowsebutton2();
	afx_msg void OnBnClickedOk();
	afx_msg void OnUpdateDub(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAppendToPlaylist(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
};

// COpenFileDlg

class COpenFileDlg : public CFileDialog
{
	DECLARE_DYNAMIC(COpenFileDlg)

private:
	WCHAR* m_buff;
	WCHAR* m_InitialDir;
	CAtlArray<CString>& m_mask;

public:
	COpenFileDlg(CAtlArray<CString>& mask, bool fAllowDirSelection,
				 LPCWSTR lpszDefExt = nullptr,
				 LPCWSTR lpszFileName = nullptr,
				 DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				 LPCWSTR lpszFilter = nullptr,
				 CWnd* pParentWnd = nullptr);
	virtual ~COpenFileDlg();

	static bool m_fAllowDirSelection;
	static WNDPROC m_wndProc;
	static LRESULT CALLBACK WindowProcNew(HWND hwnd,UINT message, WPARAM wParam, LPARAM lParam);

	virtual BOOL OnInitDialog();

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnIncludeItem(OFNOTIFYEX* pOFNEx, LRESULT* pResult);

public:
	afx_msg void OnDestroy();
};
