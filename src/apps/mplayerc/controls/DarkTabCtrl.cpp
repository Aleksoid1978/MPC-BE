/*
 * (C) 2026 see Authors.txt
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
#include "DarkTabCtrl.h"
#include "DarkTheme.h"
#include "../MainFrm.h" // ThemeRGB()

// windowsx.h defines a function-like SubclassWindow(hwnd, lpfn) macro that collides with
// CWnd::SubclassWindow(HWND); undef it so the MFC method call below parses correctly.
#undef SubclassWindow

IMPLEMENT_DYNAMIC(CDarkTabCtrl, CTabCtrl)

BEGIN_MESSAGE_MAP(CDarkTabCtrl, CTabCtrl)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()

CDarkTabCtrl::CDarkTabCtrl()
{
}

CDarkTabCtrl::~CDarkTabCtrl()
{
}

BOOL CDarkTabCtrl::OnEraseBkgnd(CDC* pDC)
{
	if (!DarkTheme::IsActive()) {
		return CTabCtrl::OnEraseBkgnd(pDC);
	}
	// All painting is done (double-buffered) in OnPaint; just claim the erase.
	return TRUE;
}

// Draws one tab button: dark fill + a three-sided border (left/top/right) so the
// button "opens" into the content pane below, plus the caption. The selected tab
// is passed a slightly enlarged rect (see OnPaint) so it overlaps its neighbours
// and merges with the pane frame, exactly like a native themed tab.
void CDarkTabCtrl::DrawTabItem(int nItem, CRect rItem, bool selected, CDC* pDC)
{
	if (nItem < 0) {
		return;
	}

	wchar_t buf[256] = {};
	TCITEMW tci = {};
	tci.mask       = TCIF_TEXT;
	tci.pszText    = buf;
	tci.cchTextMax = _countof(buf) - 1;
	GetItem(nItem, &tci);

	const COLORREF clrBorder = ThemeRGB(50, 56, 62);
	const COLORREF clrBg     = selected ? DarkTheme::FaceColor() : ThemeRGB(16, 20, 25);

	pDC->FillSolidRect(rItem, clrBg);

	// Vertical extents of the left/right border strokes: the selected tab and the
	// first tab connect down onto the pane's horizontal border line.
	const int leftY  = (nItem == 0) ? rItem.bottom - 1 : rItem.bottom - 2;
	const int rightY = selected     ? rItem.bottom - 1 : rItem.bottom;

	CPen pen(PS_SOLID, 1, clrBorder);
	CPen* pOldPen = pDC->SelectObject(&pen);
	pDC->MoveTo(rItem.left, leftY);
	pDC->LineTo(rItem.left, rItem.top);
	pDC->LineTo(rItem.right, rItem.top);
	pDC->LineTo(rItem.right, rightY);
	pDC->SelectObject(pOldPen);

	CRect rText(rItem);
	rText.left += 6;
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextColor(selected ? DarkTheme::TextColor() : RGB(140, 145, 150)); // fixed: text never tints
	pDC->DrawTextW(buf, rText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CDarkTabCtrl::OnPaint()
{
	if (!DarkTheme::IsActive()) {
		CTabCtrl::OnPaint();
		return;
	}

	CPaintDC dc(this);

	CRect rClient;
	GetClientRect(rClient);
	if (rClient.IsRectEmpty()) {
		return;
	}

	// Double-buffer to avoid flicker between the pane frame and the tab buttons.
	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&dc, rClient.Width(), rClient.Height());
	CBitmap* pOldBmp = memDC.SelectObject(&bmp);

	memDC.FillSolidRect(rClient, DarkTheme::FaceColor());

	CFont* pOldFont = memDC.SelectObject(GetFont());

	// Pane frame (the display area below the tab row).
	CRect rContent = rClient;
	AdjustRect(FALSE, rContent);
	rContent.InflateRect(2, 2);
	CBrush borderBrush(ThemeRGB(50, 56, 62));
	memDC.FrameRect(rContent, &borderBrush);

	const int nTab = GetItemCount();
	const int nSel = GetCurSel();

	// Non-selected tabs first, then the selected one on top (enlarged) so it wins
	// the overlap and blends into the pane.
	for (int i = 0; i < nTab; ++i) {
		if (i == nSel) {
			continue;
		}
		CRect r;
		GetItemRect(i, r);
		DrawTabItem(i, r, false, &memDC);
	}
	if (nSel >= 0) {
		CRect r;
		GetItemRect(nSel, r);
		r.top    -= 2;
		r.bottom += 2;
		DrawTabItem(nSel, r, true, &memDC);
	}

	memDC.SelectObject(pOldFont);
	dc.BitBlt(0, 0, rClient.Width(), rClient.Height(), &memDC, 0, 0, SRCCOPY);
	memDC.SelectObject(pOldBmp);
}

// CDarkPropertySheet

IMPLEMENT_DYNAMIC(CDarkPropertySheet, CPropertySheet)

BEGIN_MESSAGE_MAP(CDarkPropertySheet, CPropertySheet)
END_MESSAGE_MAP()

CDarkPropertySheet::CDarkPropertySheet(LPCWSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CDarkPropertySheet::~CDarkPropertySheet()
{
}

BOOL CDarkPropertySheet::OnInitDialog()
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	// Attach the owner-drawn dark tab (the stock tab keeps a light body/frame; CDarkTabCtrl falls
	// back to native when the theme is off), then dark-theme the sheet frame: title bar, background
	// and the OK/Cancel/Apply buttons. Each page dark-themes its own contents in its OnInitDialog.
	if (DarkTheme::IsActive()) {
		if (CTabCtrl* pTab = GetTabControl()) {
			m_darkTab.SubclassWindow(pTab->GetSafeHwnd());
		}
	}
	DarkTheme::ThemeDialog(GetSafeHwnd());

	return bResult;
}
