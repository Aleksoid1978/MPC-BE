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

void CAddCommandDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CAddCommandDlg, CDialog)
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

	for (int i = 0; i < s.wmcmds.size(); i++) {
		const wmcmd& wc = s.wmcmds[i];

		m_list.InsertItem(i, wc.GetName());
		CString str_id;
		str_id.Format(L"%d", wc.cmd);
		m_list.SetItemText(i, COL_ID, str_id);
		m_list.SetItemData(i, wc.cmd);
	}

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

void CAddCommandDlg::OnBnClickedOk()
{
	UpdateData();

	int selected = m_list.GetSelectionMark();
	if (selected != -1) {
		selectedID = m_list.GetItemData(selected);
	}

	OnOK();
}
