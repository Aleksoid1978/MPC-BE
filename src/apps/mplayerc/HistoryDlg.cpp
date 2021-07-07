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

	for (const auto& sesInfo : recentSessions) {
		CStringW str;
		int n = m_list.InsertItem(m_list.GetItemCount(), sesInfo.Path);

		m_list.SetItemText(n, COL_TITLE, sesInfo.Title);

		if (sesInfo.DVDId) {
			if (sesInfo.DVDTitle) {
				str.Format(L"%02u,%02u:%02u:%02u\n",
					(unsigned)sesInfo.DVDTitle,
					(unsigned)sesInfo.DVDTimecode.bHours,
					(unsigned)sesInfo.DVDTimecode.bMinutes,
					(unsigned)sesInfo.DVDTimecode.bSeconds);
				m_list.SetItemText(n, COL_POS, str);
			}
		}
		else {
			if (sesInfo.Position > UNITS) {
				LONGLONG seconds = sesInfo.Position / UNITS;
				int h = (int)(seconds / 3600);
				int m = (int)(seconds / 60 % 60);
				int s = (int)(seconds % 60);
				str.Format(L"%02d:%02d:%02d\n", h, m, s);
				m_list.SetItemText(n, COL_POS, str);
			}
		}
	}
}

void CHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CHistoryDlg, CResizableDialog)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

// CHistoryDlg message handlers

BOOL CHistoryDlg::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(COL_PATH, L"Path");
	m_list.InsertColumn(COL_TITLE, L"Title");
	m_list.InsertColumn(COL_POS, L"Position");

	SetupList();

	for (int nCol = COL_PATH; nCol < COL_COUNT; nCol++) {
		m_list.SetColumnWidth(nCol, LVSCW_AUTOSIZE_USEHEADER);
		const int headerWidth = m_list.GetColumnWidth(nCol);
		m_list.SetColumnWidth(nCol, LVSCW_AUTOSIZE);
		const int contentWidth = m_list.GetColumnWidth(nCol);

		if (headerWidth > contentWidth) {
			m_list.SetColumnWidth(nCol, headerWidth);
		}
	}

	AddAnchor(IDC_LIST1, TOP_LEFT, BOTTOM_RIGHT);

	EnableSaveRestore(IDS_R_DLG_HISTORY);

	return TRUE;
}
