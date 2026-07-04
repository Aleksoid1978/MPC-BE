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

#pragma once

#include <afxcmn.h>

// A CTabCtrl that fully owner-draws itself in the dark palette. The native tab
// control ignores SetWindowTheme("DarkMode_*") and always paints its body/pane
// and tab buttons with the light system theme, so we take over WM_PAINT /
// WM_ERASEBKGND and draw the whole control (background, pane frame, tab buttons)
// with DarkTheme colors. Falls back to the stock CTabCtrl when the dark theme is
// disabled. (Drawing approach mirrors MPC-HC's CMPCThemeTabCtrl.)
class CDarkTabCtrl : public CTabCtrl
{
	DECLARE_DYNAMIC(CDarkTabCtrl)

public:
	CDarkTabCtrl();
	virtual ~CDarkTabCtrl();

protected:
	void DrawTabItem(int nItem, CRect rItem, bool selected, CDC* pDC);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
