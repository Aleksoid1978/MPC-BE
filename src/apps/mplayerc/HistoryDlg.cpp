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
#include "MainFrm.h"
#include "ItemPropertiesDlg.h"
#include "HistoryDlg.h"
#include "../../DSUtil/std_helper.h"

// CHistoryDlg dialog

//IMPLEMENT_DYNAMIC(CHistoryDlg, CResizableDialog)

CHistoryDlg::CHistoryDlg(CWnd* pParent)
	: CResizableDialog(CHistoryDlg::IDD, pParent)
{
}

CHistoryDlg::~CHistoryDlg()
{
}

void CHistoryDlg::SetupList()
{
	m_list.DeleteAllItems();

	std::vector<SessionInfo> recentSessions;
	AfxGetMyApp()->m_HistoryFile.GetRecentSessions(recentSessions, INT_MAX);

	for (const auto& session : recentSessions) {
		int n = m_list.InsertItem(m_list.GetItemCount(), session.Path);
		m_list.SetItemText(n, 1, session.Title);
	}

	UpdateColumnsSizes();
}

void CHistoryDlg::UpdateColumnsSizes()
{
	CRect r;
	m_list.GetClientRect(r);
	m_list.SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_list.SetColumnWidth(1, LVSCW_AUTOSIZE);
	m_list.SetColumnWidth(1, std::max(m_list.GetColumnWidth(1), r.Width() - m_list.GetColumnWidth(0)));
}

void CHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CHistoryDlg, CResizableDialog)
	ON_WM_ACTIVATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

// CHistoryDlg message handlers

BOOL CHistoryDlg::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_list.InsertColumn(0, L"Path");
	m_list.InsertColumn(1, L"Title");
	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	SetupList();

	AddAnchor(IDC_LIST1, TOP_LEFT, BOTTOM_RIGHT);

	EnableSaveRestore(IDS_R_DLG_HISTORY);

	return TRUE;
}

void CHistoryDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (IsWindow(m_list)) {
		m_list.SetRedraw(FALSE);
		m_list.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		UpdateColumnsSizes();
		m_list.SetRedraw();
	}
}
