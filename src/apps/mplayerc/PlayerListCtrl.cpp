/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
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
#include "PlayerListCtrl.h"
#include "DSUtil/SysVersion.h"

// CInPlaceHotKey

CInPlaceWinHotkey::CInPlaceWinHotkey(int iItem, int iSubItem, CString sInitText)
	: m_sInitText( sInitText )
{
	m_iItem = iItem;
	m_iSubItem = iSubItem;
	m_bESC = FALSE;
}

CInPlaceWinHotkey::~CInPlaceWinHotkey()
{
}

BEGIN_MESSAGE_MAP(CInPlaceWinHotkey, CWinHotkeyCtrl)
	ON_WM_KILLFOCUS()
	ON_WM_NCDESTROY()
	ON_WM_CHAR()
	ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceHotKey message handlers

BOOL CInPlaceWinHotkey::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == VK_RETURN
				|| pMsg->wParam == VK_DELETE
				|| pMsg->wParam == VK_ESCAPE
				|| GetKeyState(VK_CONTROL)) {
			::TranslateMessage(pMsg);
			::DispatchMessageW(pMsg);
			return TRUE;				// DO NOT process further
		}
	}

	return CWinHotkeyCtrl::PreTranslateMessage(pMsg);
}

void CInPlaceWinHotkey::OnKillFocus(CWnd* pNewWnd)
{
	CWinHotkeyCtrl::OnKillFocus(pNewWnd);

	CString str;
	GetWindowTextW(str);

	NMLVDISPINFOW dispinfo;
	dispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDITW;
	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.iItem = m_iItem;
	dispinfo.item.iSubItem = m_iSubItem;
	dispinfo.item.pszText = m_bESC ? nullptr : LPWSTR((LPCWSTR)str);
	dispinfo.item.cchTextMax = str.GetLength();
	GetParent()->GetParent()->SendMessageW(WM_NOTIFY, GetParent()->GetDlgCtrlID(), (LPARAM)&dispinfo);

	DestroyWindow();
}

void CInPlaceWinHotkey::OnNcDestroy()
{
	CWinHotkeyCtrl::OnNcDestroy();

	delete this;
}

void CInPlaceWinHotkey::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_ESCAPE || nChar == VK_RETURN) {
		if (nChar == VK_ESCAPE) {
			m_bESC = TRUE;
		}
		GetParent()->SetFocus();
		return;
	}

	CWinHotkeyCtrl::OnChar(nChar, nRepCnt, nFlags);
}

int CInPlaceWinHotkey::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWinHotkeyCtrl::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	// Set the proper font
	CFont* font = GetParent()->GetFont();
	SetFont(font);

	SetWindowTextW(m_sInitText);
	SetFocus();
	SetSel(0, -1);
	return 0;
}

// CInPlaceEdit

CInPlaceEdit::CInPlaceEdit(int iItem, int iSubItem, CString sInitText)
	: m_sInitText( sInitText )
{
	m_iItem = iItem;
	m_iSubItem = iSubItem;
	m_bESC = FALSE;
}

CInPlaceEdit::~CInPlaceEdit()
{
}

BEGIN_MESSAGE_MAP(CInPlaceEdit, CEdit)
	//{{AFX_MSG_MAP(CInPlaceEdit)
	ON_WM_KILLFOCUS()
	ON_WM_NCDESTROY()
	ON_WM_CHAR()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit message handlers

BOOL CInPlaceEdit::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == VK_RETURN
				|| pMsg->wParam == VK_DELETE
				|| pMsg->wParam == VK_ESCAPE
				|| GetKeyState(VK_CONTROL)) {
			::TranslateMessage(pMsg);
			::DispatchMessageW(pMsg);
			return TRUE;				// DO NOT process further
		}
	}

	return CEdit::PreTranslateMessage(pMsg);
}

void CInPlaceEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);

	CString str;
	GetWindowTextW(str);

	NMLVDISPINFOW dispinfo;
	dispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDITW;
	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.iItem = m_iItem;
	dispinfo.item.iSubItem = m_iSubItem;
	dispinfo.item.pszText = m_bESC ? nullptr : LPWSTR((LPCWSTR)str);
	dispinfo.item.cchTextMax = str.GetLength();
	GetParent()->GetParent()->SendMessageW(WM_NOTIFY, GetParent()->GetDlgCtrlID(), (LPARAM)&dispinfo);

	DestroyWindow();
}

void CInPlaceEdit::OnNcDestroy()
{
	CEdit::OnNcDestroy();

	delete this;
}

void CInPlaceEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_ESCAPE || nChar == VK_RETURN) {
		if (nChar == VK_ESCAPE) {
			m_bESC = TRUE;
		}
		GetParent()->SetFocus();
		return;
	}

	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

int CInPlaceEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CEdit::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	// Set the proper font
	CFont* font = GetParent()->GetFont();
	SetFont(font);

	SetWindowTextW(m_sInitText);
	SetFocus();
	SetSel(0, -1);
	return 0;
}

// CInPlaceFloatEdit

CInPlaceFloatEdit::CInPlaceFloatEdit(int iItem, int iSubItem, CString sInitText)
	: CInPlaceEdit (iItem, iSubItem, sInitText) {}

CInPlaceFloatEdit::~CInPlaceFloatEdit()
{
}

BEGIN_MESSAGE_MAP(CInPlaceFloatEdit, CInPlaceEdit)
	ON_WM_CHAR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceFloatEdit message handlers

void CInPlaceFloatEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_ESCAPE || nChar == VK_RETURN) {
		if (nChar == VK_ESCAPE) {
			m_bESC = TRUE;
		}
		GetParent()->SetFocus();
		return;
	}

	if (nChar == ',') {
		nChar = '.';
	}

	if (!(nChar >= '0' && nChar <= '9' || nChar == '.' || nChar == '\b')) {
		return;
	}

	CString str;
	GetWindowTextW(str);

	if ((nChar == '.')  && str.Find('.') >= 0) {
		int nStartChar, nEndChar;
		GetSel(nStartChar, nEndChar);
		if (!(nStartChar < nEndChar && str.Mid(nStartChar, nEndChar-nStartChar).Find('.') >= 0)) {
			return;
		}
	}

	//CEdit::OnChar(nChar, nRepCnt, nFlags);
	DefWindowProcW(WM_CHAR, nChar, MAKELONG(nRepCnt, nFlags));
}

// CInPlaceComboBox

CInPlaceComboBox::CInPlaceComboBox(int iItem, int iSubItem, std::list<CString>& lstItems, int nSel)
{
	m_iItem = iItem;
	m_iSubItem = iSubItem;

	m_lstItems = lstItems;
	m_nSel = nSel;
	m_bESC = FALSE;
}

CInPlaceComboBox::~CInPlaceComboBox()
{
}

BEGIN_MESSAGE_MAP(CInPlaceComboBox, CComboBox)
	//{{AFX_MSG_MAP(CInPlaceComboBox)
	ON_WM_CREATE()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_WM_NCDESTROY()
	ON_CONTROL_REFLECT(CBN_CLOSEUP, OnCloseup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceComboBox message handlers

int CInPlaceComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	// Set the proper font
	CFont* font = GetParent()->GetFont();
	SetFont(font);

	for (const auto& lstItem : m_lstItems) {
		AddString(lstItem);
	}

	SetFocus();
	SetCurSel(m_nSel);
	return 0;
}

BOOL CInPlaceComboBox::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == VK_RETURN
				|| pMsg->wParam == VK_ESCAPE) {
			::TranslateMessage(pMsg);
			::DispatchMessageW(pMsg);
			return TRUE;				// DO NOT process further
		}
	}

	return CComboBox::PreTranslateMessage(pMsg);
}

void CInPlaceComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);

	CString str;
	GetWindowTextW(str);

	NMLVDISPINFOW dispinfo;
	dispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDITW;
	dispinfo.item.mask = LVIF_TEXT|LVIF_PARAM;
	dispinfo.item.iItem = m_iItem;
	dispinfo.item.iSubItem = m_iSubItem;
	dispinfo.item.pszText = m_bESC ? nullptr : LPWSTR((LPCWSTR)str);
	dispinfo.item.cchTextMax = str.GetLength();
	dispinfo.item.lParam = GetCurSel();
	GetParent()->GetParent()->SendMessageW(WM_NOTIFY, GetParent()->GetDlgCtrlID(), (LPARAM)&dispinfo);

	PostMessageW(WM_CLOSE);
}

void CInPlaceComboBox::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_ESCAPE || nChar == VK_RETURN) {
		if (nChar == VK_ESCAPE) {
			m_bESC = TRUE;
		}
		GetParent()->SetFocus();
		return;
	}

	CComboBox::OnChar(nChar, nRepCnt, nFlags);
}

void CInPlaceComboBox::OnNcDestroy()
{
	CComboBox::OnNcDestroy();

	delete this;
}

void CInPlaceComboBox::OnCloseup()
{
	GetParent()->SetFocus();
}

// CInPlaceListBox

CInPlaceListBox::CInPlaceListBox(int iItem, int iSubItem, std::list<CString>& lstItems, int nSel)
{
	m_iItem = iItem;
	m_iSubItem = iSubItem;

	m_lstItems = lstItems;
	m_nSel = nSel;
	m_bESC = FALSE;
}

CInPlaceListBox::~CInPlaceListBox()
{
}

BEGIN_MESSAGE_MAP(CInPlaceListBox, CListBox)
	//{{AFX_MSG_MAP(CInPlaceListBox)
	ON_WM_CREATE()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_WM_NCDESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceListBox message handlers

int CInPlaceListBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListBox::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	// Set the proper font
	CFont* font = GetParent()->GetFont();
	SetFont(font);

	for (const auto& lstItem : m_lstItems) {
		AddString(lstItem);
	}
	SetCurSel( m_nSel );
	SetFocus();
	return 0;
}

BOOL CInPlaceListBox::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == VK_RETURN
				|| pMsg->wParam == VK_ESCAPE) {
			::TranslateMessage(pMsg);
			::DispatchMessageW(pMsg);
			return TRUE;				// DO NOT process further
		}
	}

	return CListBox::PreTranslateMessage(pMsg);
}

void CInPlaceListBox::OnKillFocus(CWnd* pNewWnd)
{
	CListBox::OnKillFocus(pNewWnd);

	CString str;
	GetWindowTextW(str);

	NMLVDISPINFOW dispinfo;
	dispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDITW;
	dispinfo.item.mask = LVIF_TEXT|LVIF_PARAM;
	dispinfo.item.iItem = m_iItem;
	dispinfo.item.iSubItem = m_iSubItem;
	dispinfo.item.pszText = m_bESC ? nullptr : LPWSTR((LPCWSTR)str);
	dispinfo.item.cchTextMax = str.GetLength();
	dispinfo.item.lParam = GetCurSel();
	GetParent()->GetParent()->SendMessageW(WM_NOTIFY, GetParent()->GetDlgCtrlID(), (LPARAM)&dispinfo);

	PostMessageW(WM_CLOSE);
}

void CInPlaceListBox::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_ESCAPE || nChar == VK_RETURN) {
		if (nChar == VK_ESCAPE) {
			m_bESC = TRUE;
		}
		GetParent()->SetFocus();
		return;
	}

	CListBox::OnChar(nChar, nRepCnt, nFlags);
}

void CInPlaceListBox::OnNcDestroy()
{
	CListBox::OnNcDestroy();

	delete this;
}


// CPlayerListCtrl

IMPLEMENT_DYNAMIC(CPlayerListCtrl, CListCtrl)
CPlayerListCtrl::CPlayerListCtrl(int tStartEditingDelay)
	: m_tStartEditingDelay(tStartEditingDelay)
	, m_nItemClicked(-1)
	, m_nTimerID(0)
	, m_nSubItemClicked(-1)
	, m_fInPlaceDirty(false)
{
}

CPlayerListCtrl::~CPlayerListCtrl()
{
}

void CPlayerListCtrl::PreSubclassWindow()
{
	EnableToolTips(TRUE);

	CListCtrl::PreSubclassWindow();
}

int CPlayerListCtrl::HitTestEx(CPoint& point, int* col) const
{
	if (col) {
		*col = 0;
	}

	int row = HitTest(CPoint(0, point.y), nullptr);

	if ((GetWindowLongPtrW(m_hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT) {
		return row;
	}

	int nColumnCount = ((CHeaderCtrl*)GetDlgItem(0))->GetItemCount();

	for (int top = GetTopIndex(), bottom = GetBottomIndex(); top <= bottom; top++) {
		CRect r;
		GetItemRect(top, &r, LVIR_BOUNDS);

		if (r.top <= point.y && point.y < r.bottom) {
			for (int colnum = 0; colnum < nColumnCount; colnum++) {
				int colwidth = GetColumnWidth(colnum);

				if (point.x >= r.left && point.x <= (r.left + colwidth)) {
					if (col) {
						*col = colnum;
					}
					return top;
				}

				r.left += colwidth;
			}
		}
	}

	return -1;
}

int CPlayerListCtrl::GetBottomIndex() const
{
	CRect r;
	GetClientRect(r);

	int nBottomIndex = GetTopIndex() + GetCountPerPage() - 1;

	if (nBottomIndex >= GetItemCount()) {
		nBottomIndex = GetItemCount() - 1;
	} else if (nBottomIndex < GetItemCount()) {
		CRect br;
		GetItemRect(nBottomIndex, br, LVIR_BOUNDS);

		if (br.bottom < r.bottom) {
			nBottomIndex++;
		}
	}

	return(nBottomIndex);
}

CImageList* CPlayerListCtrl::CreateDragImageEx(LPPOINT lpPoint)
{
	if (GetSelectedCount() <= 0) {
		return nullptr;
	}

	CRect cSingleRect, cCompleteRect(0, 0, 0, 0);
	GetClientRect(cSingleRect);
	int nWidth = cSingleRect.Width();

	// Start and Stop index in view area
	int nIndex = GetTopIndex() - 1;
	int nBottomIndex = GetBottomIndex();

	// Determine the size of the drag image (limited for
	// rows visible and Client width)
	while ((nIndex = GetNextItem(nIndex, LVNI_SELECTED)) != -1 && nIndex <= nBottomIndex) {
		GetItemRect(nIndex, cSingleRect, LVIR_BOUNDS);
		/*
				CRect r;
				GetItemRect(nIndex, r, LVIR_LABEL);
				cSingleRect.left = r.left;
		*/
		if (cSingleRect.left < 0) {
			cSingleRect.left = 0;
		}
		if (cSingleRect.right > nWidth) {
			cSingleRect.right = nWidth;
		}

		cCompleteRect |= cSingleRect;
	}

	//
	// Create bitmap in memory DC
	//
	CClientDC cDc(this);
	CDC cMemDC;
	CBitmap cBitmap;

	if (!cMemDC.CreateCompatibleDC(&cDc)) {
		return nullptr;
	}

	if (!cBitmap.CreateCompatibleBitmap(&cDc, cCompleteRect.Width(), cCompleteRect.Height())) {
		return nullptr;
	}

	CBitmap* pOldMemDCBitmap = cMemDC.SelectObject(&cBitmap);
	// Here green is used as mask color
	cMemDC.FillSolidRect(0, 0, cCompleteRect.Width(), cCompleteRect.Height(), RGB(0, 255, 0));

	// Paint each DragImage in the DC
	nIndex = GetTopIndex() - 1;
	while ((nIndex = GetNextItem(nIndex, LVNI_SELECTED)) != -1 && nIndex <= nBottomIndex) {
		CPoint pt;
		CImageList* pSingleImageList = CreateDragImage(nIndex, &pt);

		if (pSingleImageList) {
			GetItemRect(nIndex, cSingleRect, LVIR_BOUNDS);

			pSingleImageList->Draw(&cMemDC,
								   0,
								   CPoint(cSingleRect.left - cCompleteRect.left, cSingleRect.top - cCompleteRect.top),
								   ILD_MASK);

			pSingleImageList->DeleteImageList();
			delete pSingleImageList;
		}
	}

	cMemDC.SelectObject(pOldMemDCBitmap);

	//
	// Create the image list with the merged drag images
	//
	CImageList* pCompleteImageList = DNew CImageList;

	pCompleteImageList->Create(cCompleteRect.Width(),
							   cCompleteRect.Height(),
							   ILC_COLOR | ILC_MASK, 0, 1);

	// Here green is used as mask color
	pCompleteImageList->Add(&cBitmap, RGB(0, 255, 0));

	//
	// as an optional service:
	// Find the offset of the current mouse cursor to the image list
	// this we can use in BeginDrag()
	//
	if (lpPoint) {
		lpPoint->x = cCompleteRect.left;
		lpPoint->y = cCompleteRect.top;
	}

	return(pCompleteImageList);
}

bool CPlayerListCtrl::PrepareInPlaceControl(int nRow, int nCol, CRect& rect)
{
	if (!EnsureVisible(nRow, TRUE)) {
		return false;
	}

	int nColumnCount = ((CHeaderCtrl*)GetDlgItem(0))->GetItemCount();
	if (nCol >= nColumnCount || GetColumnWidth(nCol) < 5) {
		return false;
	}

	int offset = 0;
	for (int i = 0; i < nCol; i++) {
		offset += GetColumnWidth(i);
	}

	GetItemRect(nRow, &rect, LVIR_BOUNDS);

	CRect rcClient;
	GetClientRect(&rcClient);
	if (offset + rect.left < 0 || offset + rect.left > rcClient.right) {
		CSize size(offset + rect.left, 0);
		Scroll(size);
		rect.left -= size.cx;
	}

	rect.left += offset;
	rect.right = rect.left + GetColumnWidth(nCol);
	if (rect.right > rcClient.right) {
		rect.right = rcClient.right;
	}

	rect.DeflateRect(1, 0, 0, 1);

	if (nCol == 0) {
		CRect r;
		GetItemRect(nRow, r, LVIR_LABEL);
		rect.left = r.left-1;
	}

	return true;
}

CWinHotkeyCtrl* CPlayerListCtrl::ShowInPlaceWinHotkey(int nItem, int nCol)
{
	CRect rect;
	if (!PrepareInPlaceControl(nItem, nCol, rect)) {
		return(nullptr);
	}

	DWORD dwStyle = /*WS_BORDER|*/WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT;

	CWinHotkeyCtrl* pWinHotkey = DNew CInPlaceWinHotkey(nItem, nCol, GetItemText(nItem, nCol));
	pWinHotkey->Create(dwStyle, rect, this, IDC_WINHOTKEY1);

	m_fInPlaceDirty = false;

	return pWinHotkey;
}

void CPlayerListCtrl::OnEnChangeWinHotkey1()
{
	m_fInPlaceDirty = true;
}

CEdit* CPlayerListCtrl::ShowInPlaceEdit(int nItem, int nCol)
{
	CRect rect;
	if (!PrepareInPlaceControl(nItem, nCol, rect)) {
		return(nullptr);
	}

	DWORD dwStyle = /*WS_BORDER|*/WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;

	LV_COLUMN lvcol;
	lvcol.mask = LVCF_FMT;
	GetColumn(nCol, &lvcol);
	dwStyle |= (lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT ? ES_LEFT
			   : (lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT ? ES_RIGHT
			   : ES_CENTER;

	CEdit* pEdit = DNew CInPlaceEdit(nItem, nCol, GetItemText(nItem, nCol));
	pEdit->Create(dwStyle, rect, this, IDC_EDIT1);

	m_fInPlaceDirty = false;

	return pEdit;
}

CEdit* CPlayerListCtrl::ShowInPlaceFloatEdit(int nItem, int nCol)
{
	CRect rect;
	if (!PrepareInPlaceControl(nItem, nCol, rect)) {
		return(nullptr);
	}

	DWORD dwStyle = /*WS_BORDER|*/WS_CHILD | WS_VISIBLE|ES_AUTOHSCROLL|ES_RIGHT;

	CEdit* pFloatEdit = DNew CInPlaceFloatEdit(nItem, nCol, GetItemText(nItem, nCol));
	pFloatEdit->Create(dwStyle, rect, this, IDC_EDIT1);

	m_fInPlaceDirty = false;

	return pFloatEdit;
}

CComboBox* CPlayerListCtrl::ShowInPlaceComboBox(int nItem, int nCol, std::list<CString>& lstItems, int nSel, bool bShowDropDown)
{
	CRect rect;
	if (!PrepareInPlaceControl(nItem, nCol, rect)) {
		return(nullptr);
	}

	DWORD dwStyle = /*WS_BORDER|*/WS_CHILD | WS_VISIBLE | WS_VSCROLL /*| WS_HSCROLL*/
					| CBS_DROPDOWNLIST /*|CBS_NOINTEGRALHEIGHT*/;
	CComboBox* pComboBox = DNew CInPlaceComboBox(nItem, nCol, lstItems, nSel);
	pComboBox->Create(dwStyle, rect, this, IDC_COMBO1);

	CorrectComboListWidth(*pComboBox);

	int width = GetColumnWidth(nCol);
	if (pComboBox->GetDroppedWidth() < width) {
		pComboBox->SetDroppedWidth(width);
	}

	if (bShowDropDown) {
		pComboBox->ShowDropDown();
	}

	m_fInPlaceDirty = false;

	return pComboBox;
}

CListBox* CPlayerListCtrl::ShowInPlaceListBox(int nItem, int nCol, std::list<CString>& lstItems, int nSel)
{
	CRect rect;
	if (!PrepareInPlaceControl(nItem, nCol, rect)) {
		return(nullptr);
	}

	DWORD dwStyle = WS_BORDER | WS_CHILD | WS_VISIBLE | WS_VSCROLL/* | WS_HSCROLL*/ | LBS_NOTIFY;
	CListBox* pListBox = DNew CInPlaceListBox(nItem, nCol, lstItems, nSel);
	pListBox->Create(dwStyle, rect, this, IDC_LIST1);

	CRect ir;
	GetItemRect(m_nItemClicked, &ir, LVIR_BOUNDS);

	pListBox->SetItemHeight(-1, ir.Height());

	CDC* pDC = pListBox->GetDC();
	CFont* pWndFont = GetFont();
	pDC->SelectObject(pWndFont);
	int width = GetColumnWidth(nCol);

	for (const auto& lstItem : lstItems) {
		int w = pDC->GetTextExtent(lstItem).cx + 16;
		if (width < w) {
			width = w;
		}
	}
	ReleaseDC(pDC);

	CRect r;
	pListBox->GetWindowRect(r);
	ScreenToClient(r);
	r.top = ir.bottom;
	r.bottom = r.top + pListBox->GetItemHeight(0) * (pListBox->GetCount() + 1);
	r.right = r.left + width;
	pListBox->MoveWindow(r);

	m_fInPlaceDirty = false;

	return pListBox;
}

BEGIN_MESSAGE_MAP(CPlayerListCtrl, CListCtrl)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_NOTIFY_REFLECT(LVN_MARQUEEBEGIN, OnLvnMarqueeBegin)
	ON_NOTIFY_REFLECT(LVN_INSERTITEM, OnLvnInsertitem)
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnLvnDeleteitem)
	ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
	ON_EN_CHANGE(IDC_WINHOTKEY1, OnEnChangeWinHotkey1)
	ON_CBN_DROPDOWN(IDC_COMBO1, OnCbnDropdownCombo1)
	ON_CBN_SELENDOK(IDC_COMBO1, OnCbnSelendokCombo1)
	ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelChangeList1)
	ON_NOTIFY_EX(HDN_ITEMCHANGINGW, 0, OnHdnItemchanging)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
END_MESSAGE_MAP()

// CPlayerListCtrl message handlers

void CPlayerListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (GetFocus() != this) {
		SetFocus();
	}

	if (nSBCode == SB_THUMBTRACK) {
		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
		GetScrollInfo(SB_VERT, &si);

		const auto newPos = (int)nPos;
		if (newPos != si.nTrackPos) {
			CRect rc;
			GetClientRect(&rc);
			const int cy = si.nPage == 0 ? rc.bottom : rc.bottom * (si.nMax + 1) / si.nPage;
			const CSize size(0, cy * (newPos - si.nPos) / (si.nMax + 1));
			Scroll(size);
		}
	}

	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CPlayerListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (GetFocus() != this) {
		SetFocus();
	}
	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CPlayerListCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (GetFocus() != this) {
		SetFocus();
	}
	return CListCtrl::OnMouseWheel(nFlags, zDelta, pt);
}

void CPlayerListCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	CListCtrl::OnLButtonDown(nFlags, point);

	if (GetFocus() != this) {
		SetFocus();
	}

	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = 0;
	}

	int nItemClickedNow, nSubItemClickedNow;

	if ((nItemClickedNow = HitTestEx(point, &nSubItemClickedNow)) < 0) {
		m_nItemClicked = -1;
	} else if (m_nItemClicked == nItemClickedNow /*&& m_nSubItemClicked == m_nSubItemClickedNow*/) {
		m_nSubItemClicked = nSubItemClickedNow;

		NMLVDISPINFOW dispinfo;
		dispinfo.hdr.hwndFrom = m_hWnd;
		dispinfo.hdr.idFrom = GetDlgCtrlID();
		dispinfo.hdr.code = LVN_BEGINLABELEDITW;
		dispinfo.item.mask = 0;
		dispinfo.item.iItem = m_nItemClicked;
		dispinfo.item.iSubItem = m_nSubItemClicked;
		if (GetParent()->SendMessageW(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo)) {
			if (m_tStartEditingDelay > 0) {
				m_nTimerID = SetTimer(1, m_tStartEditingDelay, nullptr);
			} else {
				dispinfo.hdr.code = LVN_DOLABELEDIT;
				GetParent()->SendMessageW(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
			}
		}
	} else {
		m_nItemClicked = nItemClickedNow;
		m_nSubItemClicked = nSubItemClickedNow;

		SetItemState(m_nItemClicked, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	}
}

void CPlayerListCtrl::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = 0;

		UINT flag = LVIS_FOCUSED;
		if ((GetItemState(m_nItemClicked, flag) & flag) == flag && m_nSubItemClicked >= 0) {
			NMLVDISPINFOW dispinfo;
			dispinfo.hdr.hwndFrom = m_hWnd;
			dispinfo.hdr.idFrom = GetDlgCtrlID();
			dispinfo.hdr.code = LVN_DOLABELEDIT;
			dispinfo.item.mask = 0;
			dispinfo.item.iItem = m_nItemClicked;
			dispinfo.item.iSubItem = m_nSubItemClicked;
			GetParent()->SendMessageW(WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&dispinfo);
		}
	} else if (nIDEvent == 43) {
		// CListCtrl does really strange things on this timer.
		// For example, when using mouse scroll right after mouse left button was clicked,
		// this timer scrolls the control back to the clicked item after a while.
		// There is no known problems with simply killing this timer.
		VERIFY(KillTimer(nIDEvent));
	} else {
		__super::OnTimer(nIDEvent);
	}
}

void CPlayerListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	KillTimer(1);

	CListCtrl::OnLButtonDblClk(nFlags, point);
}

void CPlayerListCtrl::OnLvnMarqueeBegin(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	UNREFERENCED_PARAMETER(pNMLV);
	*pResult = 1;
}

void CPlayerListCtrl::OnLvnInsertitem(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	UNREFERENCED_PARAMETER(pNMLV);
	m_nItemClicked = -1;
	*pResult = 0;
}

void CPlayerListCtrl::OnLvnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	UNREFERENCED_PARAMETER(pNMLV);
	m_nItemClicked = -1;
	*pResult = 0;
}

void CPlayerListCtrl::OnEnChangeEdit1()
{
	m_fInPlaceDirty = true;
}

void CPlayerListCtrl::OnCbnDropdownCombo1()
{
	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO1);

	CRect ir;
	GetItemRect(m_nItemClicked, &ir, LVIR_BOUNDS);

	CRect r;
	pCombo->GetWindowRect(r);
	ScreenToClient(r);
	r.bottom = r.top + ir.Height() + pCombo->GetItemHeight(0)*10;
	pCombo->MoveWindow(r);
}

void CPlayerListCtrl::OnCbnSelendokCombo1()
{
	m_fInPlaceDirty = true;
}

void CPlayerListCtrl::OnLbnSelChangeList1()
{
	m_fInPlaceDirty = true;
	SetFocus();
}

BOOL CPlayerListCtrl::OnHdnItemchanging(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMHEADERW phdr = reinterpret_cast<LPNMHEADERW>(pNMHDR);
	UNREFERENCED_PARAMETER(phdr);
	//SetFocus();
	*pResult = 0;
	return FALSE;
}

INT_PTR CPlayerListCtrl::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	int col;
	int row = HitTestEx(point, &col);
	if (row == -1) {
		return -1;
	}

	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();

	CRect rect;
	GetItemRect(row, &rect, LVIR_BOUNDS);

	for (int colnum = 0; colnum < nColumnCount; colnum++) {
		int colwidth = GetColumnWidth(colnum);

		if (colnum == col) {
			rect.right = rect.left + colwidth;
			break;
		}

		rect.left += colwidth;
	}

	pTI->hwnd = m_hWnd;
	pTI->uId = (UINT_PTR)((row<<10)+(col&0x3ff)+1);
	pTI->lpszText = LPSTR_TEXTCALLBACKW;
	pTI->rect = rect;

	return pTI->uId;
}

BOOL CPlayerListCtrl::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTW* pTTT = (TOOLTIPTEXTW*)pNMHDR;

	UINT_PTR nID = pNMHDR->idFrom;

	if (pTTT->uFlags & TTF_IDISHWND) {
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (nID == 0) {   // Notification in NT from automatically
		return FALSE; // created tooltip
	}

	pTTT->lParam = (LPARAM)m_hWnd;

	*pResult = 0;

	return GetParent()->SendMessageW(WM_NOTIFY, id, (LPARAM)pNMHDR);
}

int CPlayerListCtrl::InsertColumn(_In_ int nCol, _In_z_ LPCWSTR lpszColumnHeading,
		_In_ int nFormat, _In_ int nWidth, _In_ int nSubItem, _In_ int nMinWidth)
{
	nCol = __super::InsertColumn(nCol, lpszColumnHeading, nFormat, nWidth, nSubItem);
	if (nCol != -1 && nMinWidth > 0) {
		LVCOLUMNW col;
		col.mask = LVCF_MINWIDTH;
		col.cxMin = nMinWidth;
		SetColumn(nCol, &col);
		SetExtendedStyle(GetExtendedStyle() | LVS_EX_COLUMNSNAPPOINTS);
	}

	return nCol;
}
