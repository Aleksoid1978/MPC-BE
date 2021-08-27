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
#include "DSUtil/std_helper.h"

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
	m_list.SetRedraw(false);
	m_list.DeleteAllItems();

	CStringW filter;
	m_FilterEdit.GetWindowText(filter);

	for (size_t i = 0; i < m_recentSessions.size(); i++) {
		const auto& sesInfo = m_recentSessions[i];

		if (filter.GetLength()) {
			auto LowerCase = [](CStringW& str) {
				if (!str.IsEmpty()) {
					::CharLowerBuffW(str.GetBuffer(), str.GetLength());
				}
			};

			CStringW path(sesInfo.Path);
			CStringW title(sesInfo.Title);

			LowerCase(filter);
			LowerCase(path);
			LowerCase(title);

			if (path.Find(filter) < 0 && (title.IsEmpty() || title.Find(filter) < 0)) {
				continue;
			}
		}

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

		m_list.SetItemData(n, i);
	}

	m_list.SetRedraw(true);
	m_list.RedrawWindow();
}

void CHistoryDlg::RemoveSelected()
{
	std::list<SessionInfo> selSessions;

	POSITION pos = m_list.GetFirstSelectedItemPosition();

	while (pos) {
		int nItem = m_list.GetNextSelectedItem(pos);

		size_t index = m_list.GetItemData(nItem);
		if (index < m_recentSessions.size()) {
			selSessions.emplace_back(m_recentSessions[index]);
		}
	}

	if (selSessions.size()) {
		if (AfxGetMyApp()->m_HistoryFile.DeleteSessions(selSessions)) {
			AfxGetMyApp()->m_HistoryFile.GetRecentSessions(m_recentSessions, INT_MAX);
			SetupList();
		}
	}
}

int CHistoryDlg::RemoveMissingFiles()
{
	int count = 0;
	auto& historyFile = AfxGetMyApp()->m_HistoryFile;

	std::list<SessionInfo> missingFiles;

	for (auto& sesInfo : m_recentSessions) {
		if (sesInfo.DVDId && sesInfo.Path.GetLength() == 24 && StartsWithNoCase(sesInfo.Path, L":\\VIDEO_TS\\VIDEO_TS.IFO", 1)) {
			// skip DVD-Video at the root of the disc
			continue;
		}
		if (::PathIsURLW(sesInfo.Path)) {
			// skip URL
			continue;
		}
		if (!::PathFileExistsW(sesInfo.Path)) {
			missingFiles.emplace_back(sesInfo);
		}
	}

	if (missingFiles.size()) {
		if (historyFile.DeleteSessions(missingFiles)) {
			count = missingFiles.size();
			historyFile.GetRecentSessions(m_recentSessions, INT_MAX);
			SetupList();
		}
	}

	return count;
}

void CHistoryDlg::ClearHistory()
{
	if (IDYES == AfxMessageBox(ResStr(IDS_RECENT_FILES_QUESTION), MB_ICONQUESTION | MB_YESNO)) {
		if (AfxGetMyApp()->m_HistoryFile.Clear()) {
			m_recentSessions.clear();
			SetupList();
		}
	}
}

void CHistoryDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT1, m_FilterEdit);
	DDX_Control(pDX, IDC_BUTTON2, m_DelSelButton);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CHistoryDlg, CResizableDialog)
	ON_WM_ACTIVATE()
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT1, OnChangeFilterEdit)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedMenu)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedRemoveSel)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

// CHistoryDlg message handlers

BOOL CHistoryDlg::OnInitDialog()
{
	__super::OnInitDialog();

	AddAnchor(IDC_BUTTON1, TOP_LEFT);
	AddAnchor(IDC_EDIT1, TOP_LEFT);
	AddAnchor(IDC_BUTTON2, TOP_LEFT);
	AddAnchor(IDC_LIST1, TOP_LEFT, BOTTOM_RIGHT);

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(COL_PATH, L"Path");
	m_list.InsertColumn(COL_TITLE, L"Title");
	m_list.InsertColumn(COL_POS, L"Position");

	AfxGetMyApp()->m_HistoryFile.GetRecentSessions(m_recentSessions, INT_MAX);

	SetupList();

	CAppSettings& s = AfxGetAppSettings();

	for (int i = 0; i < COL_COUNT; i++) {
		int width = s.HistoryColWidths[i];

		if (width <= 0 || width > 2000) {
			m_list.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
			const int headerW = m_list.GetColumnWidth(i);

			m_list.SetColumnWidth(i, LVSCW_AUTOSIZE);
			width = m_list.GetColumnWidth(i);

			if (headerW > width) {
				width = headerW;
			}
		}
		else {
			if (width < 25) {
				width = 25;
			}
		}
		m_list.SetColumnWidth(i, width);
	}

	EnableSaveRestore(IDS_R_DLG_HISTORY);

	return TRUE;
}

void CHistoryDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_nFilterTimerID) {
		KillTimer(m_nFilterTimerID);
		SetupList();
	} else {
		__super::OnTimer(nIDEvent);
	}
}

void CHistoryDlg::OnChangeFilterEdit()
{
	KillTimer(m_nFilterTimerID);
	m_nFilterTimerID = SetTimer(2, 100, NULL);
}

void CHistoryDlg::OnBnClickedMenu()
{
	enum {
		M_REMOVE_SELECTED = 1,
		M_REMOVE_MISSING,
		M_CLEAR,
	};

	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenuW(MF_STRING | MF_ENABLED, M_REMOVE_SELECTED, ResStr(IDS_HISTORY_REMOVE_SELECTED));
	menu.AppendMenuW(MF_SEPARATOR);
	menu.AppendMenuW(MF_STRING | MF_ENABLED, M_REMOVE_MISSING, ResStr(IDS_HISTORY_REMOVE_MISSING));
	menu.AppendMenuW(MF_STRING | MF_ENABLED, M_CLEAR, ResStr(IDS_HISTORY_CLEAR));

	CRect wrect;
	GetDlgItem(IDC_BUTTON1)->GetWindowRect(&wrect);

	int id = menu.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, wrect.left, wrect.bottom, this);
	switch (id) {
	case M_REMOVE_SELECTED:
		RemoveSelected();
		break;
	case M_REMOVE_MISSING:
		RemoveMissingFiles();
		break;
	case M_CLEAR:
		ClearHistory();
		break;
	}
}

void CHistoryDlg::OnBnClickedRemoveSel()
{
	RemoveSelected();
}

void CHistoryDlg::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
{
	int index = ((NM_LISTVIEW *)pNMHDR)->iItem;
	if (index >= 0 && index < m_recentSessions.size()) {
		const auto& sesInfo = m_recentSessions[index];

		auto pFrame = AfxGetMainFrame();
		if (!pFrame->m_wndPlaylistBar.SelectFileInPlaylist(sesInfo.Path)) {
			pFrame->m_wndPlaylistBar.Open(sesInfo.Path);
		}
		pFrame->OpenCurPlaylistItem();
	}

	*pResult = 0;
}

void CHistoryDlg::OnClose()
{
	CAppSettings& s = AfxGetAppSettings();

	for (int i = 0; i < COL_COUNT; i++) {
		s.HistoryColWidths[i] = m_list.GetColumnWidth(i);
	}

	__super::OnClose();
}
