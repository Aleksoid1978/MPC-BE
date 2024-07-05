/*
 * (C) 2021-2024 see Authors.txt
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

void CHistoryDlg::CopySelectedPaths()
{
	CStringW paths;

	POSITION pos = m_list.GetFirstSelectedItemPosition();

	while (pos) {
		int nItem = m_list.GetNextSelectedItem(pos);

		size_t index = m_list.GetItemData(nItem);
		if (index < m_recentSessions.size()) {
			paths.Append(m_recentSessions[index].Path);
			paths.Append(L"\n");
		}
	}

	if (paths.GetLength()) {
		CopyStringToClipboard(this->m_hWnd, paths);
	}
}

void CHistoryDlg::RemoveFromJumpList(const std::list<SessionInfo>& sessions)
{
	CComPtr<IApplicationDestinations> pDests;
	HRESULT hr = pDests.CoCreateInstance(CLSID_ApplicationDestinations, nullptr, CLSCTX_INPROC_SERVER);
	if (hr == S_OK) {
		CComPtr<IApplicationDocumentLists> pDocLists;
		hr = pDocLists.CoCreateInstance(CLSID_ApplicationDocumentLists, nullptr, CLSCTX_INPROC_SERVER);
		if (hr == S_OK) {
			CComPtr<IObjectArray> pItems;
			hr = pDocLists->GetList(ADLT_RECENT, 0, IID_PPV_ARGS(&pItems));
			if (hr == S_OK) {
				UINT cObjects = 0;
				hr = pItems->GetCount(&cObjects);
				if (hr == S_OK) {
					for (UINT i = 0; i < cObjects; i++) {
						CComPtr<IShellItem> pShellItem;
						hr = pItems->GetAt(i, IID_PPV_ARGS(&pShellItem));
						if (hr == S_OK) {
							LPWSTR pszName = nullptr;
							hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
							if (hr == S_OK) {
								for (auto& ses : sessions) {
									if (ses.Path.CompareNoCase(pszName) == 0) {
										pDests->RemoveDestination(pShellItem);
										break;
									}
								}
								CoTaskMemFree(pszName);
							}
						}
					}
				}
			}
		}
	}
}

void CHistoryDlg::RemoveSelected()
{
	auto& historyFile = AfxGetMyApp()->m_HistoryFile;

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
		if (historyFile.DeleteSessions(selSessions)) {
			RemoveFromJumpList(selSessions);
			historyFile.GetRecentSessions(m_recentSessions, INT_MAX);
			SetupList();

			auto& strLastOpenFile = AfxGetAppSettings().strLastOpenFile;
			for (auto& ses : selSessions) {
				if (ses.Path.CompareNoCase(strLastOpenFile) == 0) {
					strLastOpenFile.Empty();
					break;
				}
			}
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
			RemoveFromJumpList(missingFiles);
			count = missingFiles.size();
			historyFile.GetRecentSessions(m_recentSessions, INT_MAX);
			SetupList();

			auto& strLastOpenFile = AfxGetAppSettings().strLastOpenFile;
			if (!::PathFileExistsW(strLastOpenFile)) {
				strLastOpenFile.Empty();
			}
		}
	}

	return count;
}

void CHistoryDlg::ClearHistory()
{
	if (IDYES == AfxMessageBox(ResStr(IDS_RECENT_FILES_QUESTION), MB_ICONQUESTION | MB_YESNO)) {
		if (AfxGetMyApp()->m_HistoryFile.Clear()) {
			CComPtr<IApplicationDestinations> pDests;
			HRESULT hr = pDests.CoCreateInstance(CLSID_ApplicationDestinations, nullptr, CLSCTX_INPROC_SERVER);
			if (SUCCEEDED(hr)) {
				hr = pDests->RemoveAllDestinations();
			}
			m_recentSessions.clear();
			SetupList();

			AfxGetAppSettings().strLastOpenFile.Empty();
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
	ON_WM_SHOWWINDOW()
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT1, OnChangeFilterEdit)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedMenu)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedRemoveSel)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST1, OnKeydownList)
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
	m_list.InsertColumn(COL_PATH, ResStr(IDS_HISTORY_PATH));
	m_list.InsertColumn(COL_TITLE, ResStr(IDS_HISTORY_TITLE));
	m_list.InsertColumn(COL_POS, ResStr(IDS_HISTORY_POSITION));

	EnableSaveRestore(IDS_R_DLG_HISTORY);

	return TRUE;
}

void CHistoryDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow) {
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
	}

	__super::OnShowWindow(bShow, nStatus);
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
	m_nFilterTimerID = SetTimer(2, 100, nullptr);
}

void CHistoryDlg::OnBnClickedMenu()
{
	enum {
		M_COPY_SELECTED = 1,
		M_REMOVE_SELECTED,
		M_REMOVE_MISSING,
		M_CLEAR,
	};

	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenuW(MF_STRING | MF_ENABLED, M_COPY_SELECTED, ResStr(IDS_COPY_TO_CLIPBOARD) + L"\tCtrl+C");
	menu.AppendMenuW(MF_STRING | MF_ENABLED, M_REMOVE_SELECTED, ResStr(IDS_HISTORY_REMOVE_SELECTED)+L"\tDelete");
	menu.AppendMenuW(MF_SEPARATOR);
	menu.AppendMenuW(MF_STRING | MF_ENABLED, M_REMOVE_MISSING, ResStr(IDS_HISTORY_REMOVE_MISSING));
	menu.AppendMenuW(MF_STRING | MF_ENABLED, M_CLEAR, ResStr(IDS_HISTORY_CLEAR));

	CRect wrect;
	GetDlgItem(IDC_BUTTON1)->GetWindowRect(&wrect);

	int id = menu.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, wrect.left, wrect.bottom, this);
	switch (id) {
	case M_COPY_SELECTED:
		CopySelectedPaths();
		break;
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
	int nItem = ((NM_LISTVIEW *)pNMHDR)->iItem;
	size_t index = m_list.GetItemData(nItem);

	if (index < m_recentSessions.size()) {
		const auto& sesInfo = m_recentSessions[index];

		auto pFrame = AfxGetMainFrame();
		if (!pFrame->m_wndPlaylistBar.SelectFileInPlaylist(sesInfo.Path)) {
			pFrame->m_wndPlaylistBar.Open(sesInfo.Path);
		}
		pFrame->OpenCurPlaylistItem();
	}

	*pResult = 0;
}

void CHistoryDlg::OnKeydownList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVKEYDOWN* pLVKeyDow = (NMLVKEYDOWN*)pNMHDR;

	if ((pLVKeyDow->flags & KF_EXTENDED) && pLVKeyDow->wVKey == VK_DELETE) {
		CString str;
		str.Format(ResStr(IDS_REMOVEFROMLISTQUESTION), (int)m_list.GetSelectedCount());
		if (IDYES == AfxMessageBox(str, MB_ICONQUESTION | MB_YESNO)) {
			RemoveSelected();
		}
	}
	else if (pLVKeyDow->wVKey == 'C' && GetKeyState(VK_CONTROL) < 0) {
		CopySelectedPaths();
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
