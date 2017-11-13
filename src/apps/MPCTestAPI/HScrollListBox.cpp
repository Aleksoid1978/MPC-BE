/*
 * (C) 2008-2017 see Authors.txt
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
#include "HScrollListBox.h"

BEGIN_MESSAGE_MAP(CHScrollListBox, CListBox)
	//{{AFX_MSG_MAP(CHScrollListBox)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_MESSAGE(LB_ADDSTRING, OnAddString)
	ON_MESSAGE(LB_INSERTSTRING, OnInsertString)
	ON_MESSAGE(LB_DELETESTRING, OnDeleteString)
	ON_MESSAGE(LB_DIR, OnDir)
	ON_MESSAGE(LB_RESETCONTENT, OnResetContent)
END_MESSAGE_MAP()

// CHScrollListBox

CHScrollListBox::CHScrollListBox()
{
}

CHScrollListBox::~CHScrollListBox()
{
}

void CHScrollListBox::PreSubclassWindow()
{
	CListBox::PreSubclassWindow();

#ifdef _DEBUG
	// NOTE: this list box is designed to work as a single column, system-drawn
	//		 list box. The asserts below will ensure of that.
	DWORD dwStyle = GetStyle();
	ASSERT((dwStyle & LBS_MULTICOLUMN) == 0);
	ASSERT((dwStyle & LBS_OWNERDRAWFIXED) == 0);
	ASSERT((dwStyle & LBS_OWNERDRAWVARIABLE) == 0);
#endif
}

// CHScrollListBox message handlers

int CHScrollListBox::GetTextLen(LPCWSTR lpszText)
{
	ASSERT(AfxIsValidString(lpszText));

	CDC *pDC = GetDC();
	ASSERT(pDC);

	CSize size;
	CFont* pOldFont = pDC->SelectObject(GetFont());
	if ((GetStyle() & LBS_USETABSTOPS) == 0) {
		size = pDC->GetTextExtent(lpszText, (int) wcslen(lpszText));
		size.cx += 3;
	} else {
		// Expand tabs as well
		size = pDC->GetTabbedTextExtentW(lpszText, (int) wcslen(lpszText), 0, nullptr);
		size.cx += 2;
	}
	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	return size.cx;
}

void CHScrollListBox::ResetHExtent()
{
	if (GetCount() == 0) {
		SetHorizontalExtent(0);
		return;
	}

	CWaitCursor cwc;
	int iMaxHExtent = 0;
	for (int i = 0; i < GetCount(); i++) {
		CString csText;
		GetText(i, csText);
		int iExt = GetTextLen(csText);
		if (iExt > iMaxHExtent) {
			iMaxHExtent = iExt;
		}
	}
	SetHorizontalExtent(iMaxHExtent);
}

void CHScrollListBox::SetNewHExtent(LPCWSTR lpszNewString)
{
	int iExt = GetTextLen(lpszNewString);
	if (iExt > GetHorizontalExtent()) {
		SetHorizontalExtent(iExt);
	}
}

// OnAddString: wParam - none, lParam - string, returns - int

LRESULT CHScrollListBox::OnAddString(WPARAM /*wParam*/, LPARAM lParam)
{
	LRESULT lResult = Default();
	if (!((lResult == LB_ERR) || (lResult == LB_ERRSPACE))) {
		SetNewHExtent((LPCWSTR) lParam);
	}
	return lResult;
}

// OnInsertString: wParam - index, lParam - string, returns - int

LRESULT CHScrollListBox::OnInsertString(WPARAM /*wParam*/, LPARAM lParam)
{
	LRESULT lResult = Default();
	if (!((lResult == LB_ERR) || (lResult == LB_ERRSPACE))) {
		SetNewHExtent((LPCWSTR) lParam);
	}
	return lResult;
}

// OnDeleteString: wParam - index, lParam - none, returns - int

LRESULT CHScrollListBox::OnDeleteString(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	LRESULT lResult = Default();
	if (!((lResult == LB_ERR) || (lResult == LB_ERRSPACE))) {
		ResetHExtent();
	}
	return lResult;
}

// OnDir: wParam - attr, lParam - wildcard, returns - int

LRESULT CHScrollListBox::OnDir(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	LRESULT lResult = Default();
	if (!((lResult == LB_ERR) || (lResult == LB_ERRSPACE))) {
		ResetHExtent();
	}
	return lResult;
}

// OnResetContent: wParam - none, lParam - none, returns - int

LRESULT CHScrollListBox::OnResetContent(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	LRESULT lResult = Default();
	SetHorizontalExtent(0);
	return lResult;
}
