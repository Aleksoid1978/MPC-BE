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
#include "DarkCheckListBox.h"
#include "DarkTheme.h"

IMPLEMENT_DYNAMIC(CDarkCheckListBox, CCheckListBox)

BEGIN_MESSAGE_MAP(CDarkCheckListBox, CCheckListBox)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// Paint the whole client dark before the items are drawn, so any area an item row
// does not cover (e.g. behind a tab-control body) shows the dark background instead
// of white.
BOOL CDarkCheckListBox::OnEraseBkgnd(CDC* pDC)
{
	if (DarkTheme::IsActive()) {
		CRect rc;
		GetClientRect(&rc);
		pDC->FillSolidRect(rc, DarkTheme::FaceColor());
		return TRUE;
	}

	return CCheckListBox::OnEraseBkgnd(pDC);
}

// This mirrors MFC's CCheckListBox::DrawItem (which only draws the item text; the
// check-box glyph is drawn separately by the non-virtual PreDrawItem), but replaces
// the hard-coded COLOR_WINDOW / COLOR_WINDOWTEXT with the dark theme palette.
void CDarkCheckListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (!DarkTheme::IsActive()) {
		CCheckListBox::DrawItem(lpDrawItemStruct);
		return;
	}

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	if (!pDC) {
		return;
	}

	if (static_cast<LONG>(lpDrawItemStruct->itemID) >= 0
			&& (lpDrawItemStruct->itemAction & (ODA_DRAWENTIRE | ODA_SELECT))) {
		const int cyItem = GetItemHeight(lpDrawItemStruct->itemID);
		const BOOL fDisabled = !IsWindowEnabled() || !IsEnabled(lpDrawItemStruct->itemID);

		COLORREF clrText = fDisabled ? RGB(120, 125, 130) : DarkTheme::TextColor();
		COLORREF clrBk   = DarkTheme::FaceColor();
		if (!fDisabled && (lpDrawItemStruct->itemState & ODS_SELECTED)) {
			clrText = RGB(255, 255, 255);
			clrBk   = RGB(45, 80, 120); // subtle dark-blue selection
		}
		pDC->SetTextColor(clrText);
		pDC->SetBkColor(clrBk);

		CString strText;
		GetText(lpDrawItemStruct->itemID, strText);

		int yText = lpDrawItemStruct->rcItem.top;
		if (cyItem > static_cast<int>(m_cyText)) {
			yText += (cyItem - static_cast<int>(m_cyText)) / 2;
		}

		// Fill to the full client width: for some lists the item rect (and the DC's
		// clip region) does not span the whole control, which would leave the right
		// side unpainted (white).
		CRect rcFill(lpDrawItemStruct->rcItem);
		CRect rcClient;
		GetClientRect(&rcClient);
		if (rcClient.right > rcFill.right) {
			rcFill.right = rcClient.right;
		}
		if (rcFill.bottom > rcClient.bottom) {
			rcFill.bottom = rcClient.bottom;
		}
		// Clip to the client rect (not "no clip"): the list box reuses one DC for every row and does
		// not reset the clip per row, so clearing it let a partially-visible last row spill below the
		// box — both our text (bounded above + ETO_CLIPPED) AND the checkbox glyph the base
		// CCheckListBox::PreDrawItem paints (which our clamp can't reach). A client-rect clip still
		// lets the opaque fill reach the right edge but keeps every row's paint — glyph included —
		// inside the box, so the next item's checkbox no longer peeks out the bottom.
		CRgn rgnClient;
		rgnClient.CreateRectRgnIndirect(&rcClient);
		pDC->SelectClipRgn(&rgnClient);

		pDC->ExtTextOutW(lpDrawItemStruct->rcItem.left, yText, ETO_OPAQUE | ETO_CLIPPED,
			rcFill, strText, strText.GetLength(), nullptr);
	}

	if (lpDrawItemStruct->itemAction & ODA_FOCUS) {
		pDC->DrawFocusRect(&lpDrawItemStruct->rcItem);
	}
}
