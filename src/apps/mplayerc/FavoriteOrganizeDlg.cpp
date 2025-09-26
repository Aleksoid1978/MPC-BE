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

#include "stdafx.h"
#include "MainFrm.h"
#include "ItemPropertiesDlg.h"
#include "FavoriteOrganizeDlg.h"
#include "DSUtil/std_helper.h"

// CFavoriteOrganizeDlg dialog

//IMPLEMENT_DYNAMIC(CFavoriteOrganizeDlg, CResizableDialog)

CFavoriteOrganizeDlg::CFavoriteOrganizeDlg(CWnd* pParent)
	: CResizableDialog(CFavoriteOrganizeDlg::IDD, pParent)
{
}

CFavoriteOrganizeDlg::~CFavoriteOrganizeDlg()
{
}

void CFavoriteOrganizeDlg::SetupList()
{
	m_list.DeleteAllItems();

	const int curTab = m_tab.GetCurSel();

	if (curTab == 0) {
		for (const auto& favFile : m_FavFiles) {
			auto& favname = favFile.Title.GetLength() ? favFile.Title : favFile.Path;
			int n = m_list.InsertItem(m_list.GetItemCount(), favname);

			m_list.SetItemData(n, (DWORD_PTR)&(favFile));

			if (favFile.Position > UNITS) {
				CStringW str;
				LONGLONG seconds = favFile.Position / UNITS;
				int h = (int)(seconds / 3600);
				int m = (int)(seconds / 60 % 60);
				int s = (int)(seconds % 60);
				str.Format(L"[%02d:%02d:%02d]", h, m, s);
				m_list.SetItemText(n, 1, str);
			}
		}
	}
	else if (curTab == 1) {
		for (const auto& favDvd : m_FavDVDs) {
			auto& favname = favDvd.Title.GetLength() ? favDvd.Title : favDvd.Path;
			int n = m_list.InsertItem(m_list.GetItemCount(), favDvd.Title);

			m_list.SetItemData(n, (DWORD_PTR)&(favDvd));

			if (favDvd.DVDTitle) {
				CStringW str;
				str.Format(L"[%02u,%02u:%02u:%02u]",
					(unsigned)favDvd.DVDTitle,
					(unsigned)favDvd.DVDTimecode.bHours,
					(unsigned)favDvd.DVDTimecode.bMinutes,
					(unsigned)favDvd.DVDTimecode.bSeconds);
				m_list.SetItemText(n, 1, str);
			}
		}
	}

	UpdateColumnsSizes();
}

void CFavoriteOrganizeDlg::SaveList()
{
	auto sync_favlist = [&](std::list<SessionInfo>& favlist) {
		std::list<SessionInfo> newfavlist;
		for (int i = 0; i < m_list.GetItemCount(); i++) {
			auto it = FindInListByPointer(favlist, (SessionInfo*)m_list.GetItemData(i));
			if (it == favlist.end()) {
				ASSERT(0);
				break;
			}
			newfavlist.emplace_back(*it);
		}
		favlist = newfavlist;
	};

	const int curTab = m_tab.GetCurSel();

	if (curTab == 0) {
		sync_favlist(m_FavFiles);
	}
	else if (curTab == 1) {
		sync_favlist(m_FavDVDs);
	}
}

void CFavoriteOrganizeDlg::UpdateColumnsSizes()
{
	CRect r;
	m_list.GetClientRect(r);
	m_list.SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_list.SetColumnWidth(1, LVSCW_AUTOSIZE);
	m_list.SetColumnWidth(1, std::max(m_list.GetColumnWidth(1), r.Width() - m_list.GetColumnWidth(0)));
}

void CFavoriteOrganizeDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TAB1, m_tab);
	DDX_Control(pDX, IDC_LIST2, m_list);
}

BEGIN_MESSAGE_MAP(CFavoriteOrganizeDlg, CResizableDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnTcnSelChangeTab1)
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_BUTTON1, OnEditBnClicked)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON1, OnUpdateEditBn)
	ON_BN_CLICKED(IDC_BUTTON2, OnDeleteBnClicked)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateDeleteBn)
	ON_BN_CLICKED(IDC_BUTTON3, OnUpBnClicked)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON3, OnUpdateUpBn)
	ON_BN_CLICKED(IDC_BUTTON4, OnDownBnClicked)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON4, OnUpdateDownBn)
	ON_NOTIFY(TCN_SELCHANGING, IDC_TAB1, OnTcnSelChangingTab1)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_WM_ACTIVATE()
	ON_NOTIFY(LVN_ENDLABELEDITW, IDC_LIST2, OnLvnEndLabelEditList2)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST2, OnPlayFavorite)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST2, OnKeyPressed)
	ON_NOTIFY(LVN_GETINFOTIPW, IDC_LIST2, OnLvnGetInfoTipList)
	ON_WM_SIZE()
END_MESSAGE_MAP()

// CFavoriteOrganizeDlg message handlers

BOOL CFavoriteOrganizeDlg::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_tab.InsertItem(0, ResStr(IDS_FAVFILES));
	m_tab.InsertItem(1, ResStr(IDS_FAVDVDS));
	// m_tab.InsertItem(2, ResStr(IDS_FAVDEVICES));
	m_tab.SetCurSel(0);

	m_list.InsertColumn(0, L"");
	m_list.InsertColumn(1, L"");
	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	AfxGetMyApp()->m_FavoritesFile.GetFavorites(m_FavFiles, m_FavDVDs);

	SetupList();

	AddAnchor(IDC_TAB1, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LIST2, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON1, TOP_RIGHT);
	AddAnchor(IDC_BUTTON2, TOP_RIGHT);
	AddAnchor(IDC_BUTTON3, TOP_RIGHT);
	AddAnchor(IDC_BUTTON4, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);

	EnableSaveRestore(IDS_R_DLG_ORGANIZE_FAV);

	return TRUE;
}

BOOL CFavoriteOrganizeDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && pMsg->hwnd == m_list.GetSafeHwnd() && m_list.GetSelectedCount() > 0) {
		return FALSE;
	}

	return __super::PreTranslateMessage(pMsg);
}

void CFavoriteOrganizeDlg::OnTcnSelChangeTab1(NMHDR* pNMHDR, LRESULT* pResult)
{
	m_list.SetRedraw(FALSE);
	m_list.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetupList();
	m_list.SetRedraw();

	*pResult = 0;
}

void CFavoriteOrganizeDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl != IDC_LIST2) {
		return;
	}

	int nItem = lpDrawItemStruct->itemID;
	CRect rcItem = lpDrawItemStruct->rcItem;

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	CBrush b;
	if (!!m_list.GetItemState(nItem, LVIS_SELECTED)) {
		b.CreateSolidBrush(0xf1dacc);
		pDC->FillRect(rcItem, &b);
		b.DeleteObject();
		b.CreateSolidBrush(0xc56a31);
		pDC->FrameRect(rcItem, &b);
	} else {
		b.CreateSysColorBrush(COLOR_WINDOW);
		pDC->FillRect(rcItem, &b);
	}

	CStringW str;
	pDC->SetTextColor(0);

	str = m_list.GetItemText(nItem, 0);
	pDC->TextOut(rcItem.left + 3, (rcItem.top + rcItem.bottom - pDC->GetTextExtent(str).cy) / 2, str);
	str = m_list.GetItemText(nItem, 1);

	if (!str.IsEmpty()) {
		pDC->TextOut(rcItem.right - pDC->GetTextExtent(str).cx - 3, (rcItem.top + rcItem.bottom - pDC->GetTextExtent(str).cy) / 2, str);
	}
}

void CFavoriteOrganizeDlg::OnEditBnClicked()
{
	if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
		int nItem = m_list.GetNextSelectedItem(pos);

		auto edit_favitem = [&](std::list<SessionInfo>& favlist) {
			auto it = FindInListByPointer(favlist, (SessionInfo*)m_list.GetItemData(nItem));
			if (it == favlist.end()) {
				ASSERT(0);
				return;
			}

			CItemPropertiesDlg ipd((*it).Title, (*it).Path);
			if (ipd.DoModal() == IDOK) {
				m_list.SetItemText(nItem, 0, ipd.GetPropertyName());
				(*it).Title = ipd.GetPropertyName();
				(*it).Path = ipd.GetPropertyPath();
			}
		};

		const int curTab = m_tab.GetCurSel();

		if (curTab == 0) {
			edit_favitem(m_FavFiles);
		}
		else if (curTab == 1) {
			edit_favitem(m_FavDVDs);
		}
	}
}

void CFavoriteOrganizeDlg::OnUpdateEditBn(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_list.GetSelectedCount() == 1);
}

void CFavoriteOrganizeDlg::OnLvnEndLabelEditList2(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	if (pDispInfo->item.iItem >= 0 && pDispInfo->item.pszText) {
		m_list.SetItemText(pDispInfo->item.iItem, 0, pDispInfo->item.pszText);
	}

	UpdateColumnsSizes();

	*pResult = 0;
}

void CFavoriteOrganizeDlg::PlayFavorite(int nItem)
{
	if (nItem < 0 || nItem >= m_list.GetItemCount()) {
		return;
	}

	const int curTab = m_tab.GetCurSel();

	if (curTab == 0) {
		auto it = FindInListByPointer(m_FavFiles, (SessionInfo*)m_list.GetItemData(nItem));
		if (it != m_FavFiles.end()) {
			AfxGetMainFrame()->PlayFavoriteFile(*it);
		}
	}
	else if (curTab == 1) {
		auto it = FindInListByPointer(m_FavDVDs, (SessionInfo*)m_list.GetItemData(nItem));
		if (it != m_FavDVDs.end()) {
			AfxGetMainFrame()->PlayFavoriteDVD(*it);
		}
	}
}

void CFavoriteOrganizeDlg::OnPlayFavorite(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	PlayFavorite(pItemActivate->iItem);
}

void CFavoriteOrganizeDlg::OnKeyPressed(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

	switch (pLVKeyDow->wVKey) {
		case VK_DELETE:
			OnDeleteBnClicked();
			*pResult = 1;
			break;
		case VK_RETURN:
			if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
				int nItem = m_list.GetNextSelectedItem(pos);
				PlayFavorite(nItem);
			}
			*pResult = 1;
			break;
		case 'A':
			if (GetKeyState(VK_CONTROL) < 0) { // If the high-order bit is 1, the key is down; otherwise, it is up.
				m_list.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
			}
			*pResult = 1;
			break;
		case 'I':
			if (GetKeyState(VK_CONTROL) < 0) { // If the high-order bit is 1, the key is down; otherwise, it is up.
				for (int nItem = 0; nItem < m_list.GetItemCount(); nItem++) {
					m_list.SetItemState(nItem, ~m_list.GetItemState(nItem, LVIS_SELECTED), LVIS_SELECTED);
				}
			}
			*pResult = 1;
			break;
		default:
			*pResult = 0;
	}
}

void CFavoriteOrganizeDlg::OnDeleteBnClicked()
{
	int nItem = 0;

	auto erase_favitem = [&](std::list<SessionInfo>& favlist) {
		POSITION pos;
		while (pos = m_list.GetFirstSelectedItemPosition()) {
			nItem = m_list.GetNextSelectedItem(pos);
			if (nItem < 0 || nItem >= m_list.GetItemCount()) {
				return;
			}

			auto it = FindInListByPointer(favlist, (SessionInfo*)m_list.GetItemData(nItem));
			if (it != favlist.end()) {
				favlist.erase(it);
			}

			m_list.DeleteItem(nItem);
		}
	};

	const int curTab = m_tab.GetCurSel();

	if (curTab == 0) {
		erase_favitem(m_FavFiles);
	}
	else if (curTab == 1) {
		erase_favitem(m_FavDVDs);
	}

	nItem = std::min(nItem, m_list.GetItemCount() - 1);
	m_list.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
}

void CFavoriteOrganizeDlg::OnUpdateDeleteBn(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_list.GetSelectedCount() > 0);
}

void CFavoriteOrganizeDlg::MoveItem(int nItem, int offset)
{
	DWORD_PTR data = m_list.GetItemData(nItem);
	CStringW strName = m_list.GetItemText(nItem, 0);
	CStringW strPos = m_list.GetItemText(nItem, 1);

	m_list.DeleteItem(nItem);

	nItem += offset;

	m_list.InsertItem(nItem, strName);
	m_list.SetItemData(nItem, data);
	m_list.SetItemText(nItem, 1, strPos);
	m_list.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
}

void CFavoriteOrganizeDlg::OnUpBnClicked()
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int nItem;

	while (pos) {
		nItem = m_list.GetNextSelectedItem(pos);

		if (nItem <= 0 || nItem >= m_list.GetItemCount()) {
			return;
		}

		MoveItem(nItem, -1);
	}
}

void CFavoriteOrganizeDlg::OnUpdateUpBn(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_list.GetSelectedCount() > 0 && !m_list.GetItemState(0, LVIS_SELECTED));
}

void CFavoriteOrganizeDlg::OnDownBnClicked()
{
	std::vector<int> selectedItems;

	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos) {
		int nItem = m_list.GetNextSelectedItem(pos);

		if (nItem < 0 || nItem >= m_list.GetItemCount() - 1) {
			return;
		}

		selectedItems.emplace_back(nItem);
	}

	for (int i = selectedItems.size() - 1; i >= 0; i--) {
		MoveItem(selectedItems[i], +1);
	}
}

void CFavoriteOrganizeDlg::OnUpdateDownBn(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_list.GetSelectedCount() > 0 && !m_list.GetItemState(m_list.GetItemCount() - 1, LVIS_SELECTED));
}

void CFavoriteOrganizeDlg::OnTcnSelChangingTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	SaveList();

	*pResult = 0;
}

void CFavoriteOrganizeDlg::OnBnClickedOk()
{
	SaveList();

	CAppSettings& s = AfxGetAppSettings();

	AfxGetMyApp()->m_FavoritesFile.SaveFavorites(m_FavFiles, m_FavDVDs);

	OnOK();
}

void CFavoriteOrganizeDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (IsWindow(m_list)) {
		m_list.SetRedraw(FALSE);
		m_list.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		UpdateColumnsSizes();
		m_list.SetRedraw();
	}
}

void CFavoriteOrganizeDlg::OnLvnGetInfoTipList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVGETINFOTIPW pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIPW>(pNMHDR);

	auto setTipText = [&](std::list<SessionInfo>& favlist) {
		auto it = FindInListByPointer(favlist, (SessionInfo*)m_list.GetItemData(pGetInfoTip->iItem));
		if (it != favlist.end()) {
			StringCchCopyW(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, (*it).Path);
			*pResult = 0;
		}
	};

	const int curTab = m_tab.GetCurSel();

	if (curTab == 0) {
		setTipText(m_FavFiles);
	}
	else if (curTab == 1) {
		setTipText(m_FavDVDs);
	}
}
