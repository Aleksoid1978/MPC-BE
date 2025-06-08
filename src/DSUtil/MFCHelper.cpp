/*
 * (C) 2016-2025 see Authors.txt
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
#include "MFCHelper.h"

#include <utility>

CString ResStr(UINT nID)
{
	CString id;
	if (!id.LoadStringW(nID)) {
		std::ignore = id.LoadStringW(AfxGetApp()->m_hInstance, nID);
	}

	return id;
}

void SetCursor(HWND m_hWnd, LPCWSTR lpCursorName)
{
	SetClassLongPtrW(m_hWnd, GCLP_HCURSOR, (LONG_PTR)AfxGetApp()->LoadStandardCursor(lpCursorName));
}

void SetCursor(HWND m_hWnd, UINT nID, LPCWSTR lpCursorName)
{
	SetCursor(::GetDlgItem(m_hWnd, nID), lpCursorName);
}

void CorrectComboListWidth(CComboBox& ComboBox)
{
	if (ComboBox.GetCount() <= 0)
		return;

	CString    str;
	CSize      sz;
	TEXTMETRIC tm;
	int        dx		= 0;
	CDC*       pDC		= ComboBox.GetDC();
	CFont*     pFont	= ComboBox.GetFont();

	// Select the listbox font, save the old font
	CFont* pOldFont = pDC->SelectObject(pFont);
	// Get the text metrics for avg char width
	pDC->GetTextMetricsW(&tm);

	// Find the longest string in the combo box.
	for (int i = 0; i < ComboBox.GetCount(); i++) {
		ComboBox.GetLBText(i, str);
		sz = pDC->GetTextExtent(str);

		// Add the avg width to prevent clipping
		sz.cx += tm.tmAveCharWidth;

		if (sz.cx > dx) {
			dx = sz.cx;
		}
	}
	// Select the old font back into the DC
	pDC->SelectObject(pOldFont);
	ComboBox.ReleaseDC(pDC);

	// Get the scrollbar width if it exists
	int min_visible = ComboBox.GetMinVisible();
	int scroll_width = (ComboBox.GetCount() > min_visible) ?
					   ::GetSystemMetrics(SM_CXVSCROLL) : 0;

	// Adjust the width for the vertical scroll bar and the left and right border.
	dx += scroll_width + 2*::GetSystemMetrics(SM_CXEDGE);

	// Set the width of the list box so that every item is completely visible.
	ComboBox.SetDroppedWidth(dx);
}

void CorrectCWndWidth(CWnd* pWnd)
{
	if (!pWnd) {
		return;
	}

	CDC*   pDC = pWnd->GetDC();
	CFont* pFont = pWnd->GetFont();
	CFont* pOldFont = pDC->SelectObject(pFont);

	CString str;
	pWnd->GetWindowTextW(str);
	CSize szText = pDC->GetTextExtent(str);

	TEXTMETRIC tm;
	pDC->GetTextMetricsW(&tm);
	pDC->SelectObject(pOldFont);
	pWnd->ReleaseDC(pDC);

	CRect r;
	pWnd->GetWindowRect(r);
	pWnd->GetOwner()->ScreenToClient(r);

	r.right = r.left + ::GetSystemMetrics(SM_CXMENUCHECK) + szText.cx + tm.tmAveCharWidth;
	pWnd->MoveWindow(r);
}

extern void SetMenuRadioCheck(CCmdUI* pCmdUI, bool bCheck)
{
	if (IsMenu(*pCmdUI->m_pMenu)) {
		MENUITEMINFO mii = { sizeof(mii) };
		mii.fMask = MIIM_FTYPE | MIIM_STATE;
		mii.fType = bCheck ? MFT_RADIOCHECK : MFT_STRING;
		mii.fState = bCheck ? MFS_CHECKED : MFS_UNCHECKED;
		VERIFY(pCmdUI->m_pMenu->SetMenuItemInfo(pCmdUI->m_nID, &mii));
	}
}

inline void AddStringData(CComboBox& ComboBox, LPCWSTR str, DWORD_PTR data)
{
	ComboBox.SetItemData(ComboBox.AddString(str), data);
}

inline void AddStringData(CListBox& ListBox, LPCWSTR str, DWORD_PTR data)
{
	ListBox.SetItemData(ListBox.AddString(str), data);
}

inline DWORD_PTR GetCurItemData(CComboBox& ComboBox)
{
	return ComboBox.GetItemData(ComboBox.GetCurSel());
}

inline DWORD_PTR GetCurItemData(CListBox& ListBox)
{
	return ListBox.GetItemData(ListBox.GetCurSel());
}

void SelectByItemData(CComboBox& ComboBox, DWORD_PTR data)
{
	for (int i = 0; i < ComboBox.GetCount(); i++) {
		if (ComboBox.GetItemData(i) == data) {
			ComboBox.SetCurSel(i);
			break;
		}
	}
}

void SelectByItemData(CListBox& ListBox, DWORD_PTR data)
{
	for (int i = 0; i < ListBox.GetCount(); i++) {
		if (ListBox.GetItemData(i) == data) {
			ListBox.SetCurSel(i);
			ListBox.SetTopIndex(i);
			break;
		}
	}
}

void CopyDataToClipboard(HWND hWnd, const UINT uFormat, const void* data, const size_t size)
{
	HGLOBAL hGlob = ::GlobalAlloc(GMEM_MOVEABLE, size);
	if (hGlob) {
		LPVOID pData = ::GlobalLock(hGlob);
		if (pData) {
			memcpy(pData, data, size);
			::GlobalUnlock(hGlob);

			if (::OpenClipboard(hWnd)) {
				// Place the handle on the clipboard, if the call succeeds
				// the system will take care of the allocated memory
				if (::EmptyClipboard() && ::SetClipboardData(uFormat, hGlob)) {
					hGlob = nullptr;
				}

				::CloseClipboard();
			}
		}

		if (hGlob) {
			::GlobalFree(hGlob);
		}
	}
}

void CopyStringToClipboard(HWND hWnd, const CStringW& str)
{
	CopyDataToClipboard(hWnd, CF_UNICODETEXT, str.GetString(), (str.GetLength() + 1) * sizeof(WCHAR));
}
