/*
 * (C) 2021 see Authors.txt
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
#include "AddCommandDlg.h"

// CAddCommandDlg dialog

IMPLEMENT_DYNAMIC(CAddCommandDlg, CDialog)
CAddCommandDlg::CAddCommandDlg(CWnd* pParent)
	: CDialog(CAddCommandDlg::IDD, pParent)
{
}

CAddCommandDlg::~CAddCommandDlg()
{
}

void CAddCommandDlg::FillList()
{
	CAppSettings& s = AfxGetAppSettings();

	for (int i = 0; i < (int)s.wmcmds.size(); i++) {
		const wmcmd& wc = s.wmcmds[i];

		m_list.InsertItem(i, wc.GetName());
		CString str_id;
		str_id.Format(L"%d", wc.cmd);
		m_list.SetItemText(i, COL_ID, str_id);
		m_list.SetItemData(i, wc.cmd);
	}
}

void CAddCommandDlg::FilterList()
{
	CString filter;
	m_FilterEdit.GetWindowText(filter);

	m_list.SetRedraw(false);
	m_list.DeleteAllItems();

	CAppSettings& s = AfxGetAppSettings();

	if (filter.IsEmpty()) {
		FillList();
	}
	else {
		auto LowerCase = [](CString& str) {
			if (!str.IsEmpty()) {
				::CharLowerBuffW(str.GetBuffer(), str.GetLength());
			}
		};
		LowerCase(filter);
		int n = 0;

		for (int i = 0; i < (int)s.wmcmds.size(); i++) {
			const wmcmd& wc = s.wmcmds[i];

			CString name = wc.GetName();
			LowerCase(name);
			CString str_id;
			str_id.Format(L"%d", wc.cmd);

			if (name.Find(filter) >= 0 || str_id.Find(filter) >= 0) {
				m_list.InsertItem(n, wc.GetName());
				m_list.SetItemText(n, COL_ID, str_id);
				m_list.SetItemData(n, wc.cmd);
			}
		}
	}

	m_list.SetRedraw(true);
	m_list.RedrawWindow();
}

void CAddCommandDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT1, m_FilterEdit);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CAddCommandDlg, CDialog)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT1, OnChangeFilterEdit)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

// CAddCommandDlg message handlers

BOOL CAddCommandDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_COLUMNSNAPPOINTS | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_list.InsertColumn(COL_CMD, ResStr(IDS_MOUSE_COMMAND));
	m_list.InsertColumn(COL_ID, L"ID");

	CAppSettings& s = AfxGetAppSettings();

	FillList();

	for (int nCol = 0; nCol < COL_COUNT; nCol++) {
		m_list.SetColumnWidth(nCol, LVSCW_AUTOSIZE_USEHEADER);
		const int headerSize = m_list.GetColumnWidth(nCol);

		LVCOLUMNW col;
		col.mask = LVCF_MINWIDTH;
		col.cxMin = headerSize;
		m_list.SetColumn(nCol, &col);
	}

	return TRUE;
}

void CAddCommandDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_nFilterTimerID) {
		KillTimer(m_nFilterTimerID);
		FilterList();
	} else {
		__super::OnTimer(nIDEvent);
	}
}

void CAddCommandDlg::OnChangeFilterEdit()
{
	KillTimer(m_nFilterTimerID);
	m_nFilterTimerID = SetTimer(2, 100, NULL);
}

void CAddCommandDlg::OnBnClickedOk()
{
	UpdateData();

	int selected = m_list.GetSelectionMark();
	if (selected != -1) {
		selectedID = m_list.GetItemData(selected);
	}

	OnOK();
}
