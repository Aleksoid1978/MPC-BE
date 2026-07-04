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

// A CCheckListBox that paints its item text with the dark palette when the dark
// theme is active. Only the text foreground/background differ from the base class;
// the check-box glyph is still drawn by CCheckListBox (themed), so it stays in sync
// with the rest of the UI. Falls back to the stock rendering when the theme is off.
class CDarkCheckListBox : public CCheckListBox
{
	DECLARE_DYNAMIC(CDarkCheckListBox)

public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	DECLARE_MESSAGE_MAP()
};
