/*
 * (C) 2006-2018 see Authors.txt
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

#include <afxwin.h>

#define EDIT_BUTTON_LEFTCLICKED (WM_APP + 842)

// CEditWithButton_Base

class CEditWithButton_Base : public CEdit
{
public:
	CEditWithButton_Base();

	virtual void OnLeftClick();

protected:
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnNcMouseLeave();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	virtual void PreSubclassWindow();
	afx_msg LRESULT OnRecalcNcSize(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg LRESULT OnSetReadOnly(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

	CRect GetButtonRect(const CRect& rectWindow) const;
	int GetButtonThemeState() const;

	virtual void DrawButton(CRect rectButton);
	virtual void DrawButtonContent(CDC& dc, CRect rectButton, HTHEME hButtonTheme) PURE;
	virtual int CalculateButtonWidth() PURE;

private:
	int m_TopBorder;
	int m_BottomBorder;
	int m_LeftBorder;
	int m_RightBorder;

	int m_ButtonWidth;

	bool m_IsButtonPressed;
	bool m_IsMouseActive;

	bool m_IsButtonHot;
};

// CEditWithButton

class CEditWithButton : public CEditWithButton_Base
{
public:
	explicit CEditWithButton(LPCTSTR pszButtonText = L"...");

	CString GetButtonText() const;
	void SetButtonText(LPCTSTR buttonText);

private:
	virtual void DrawButtonContent(CDC& dc, CRect rectButton, HTHEME hButtonTheme);
	virtual int CalculateButtonWidth();

	CString m_ButtonText;
};
