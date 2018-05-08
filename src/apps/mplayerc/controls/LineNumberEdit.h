/*
 * (C) 2003-2006 Gabest
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

#if !defined(AFX_LINENUMBEREDIT_H__CAB7A465_709C_42B8_80D0_2B0AF6D25AD4__INCLUDED_)
#define AFX_LINENUMBEREDIT_H__CAB7A465_709C_42B8_80D0_2B0AF6D25AD4__INCLUDED_

// CLineNumberStatic window

class CLineNumberStatic : public CStatic
{
// Construction/destruction
public:
	CLineNumberStatic();
	virtual ~CLineNumberStatic();

// Operations
public:
	void SetFgColor( COLORREF col, BOOL redraw );
	void SetBgColor( COLORREF col, BOOL redraw );
	void SetTopAndBottom( int topline, int bottomline );
	void SetTopMargin( int topmargin );
	void SetLineNumberFormat( CString format );

protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	DECLARE_MESSAGE_MAP()

private:
// Attributes
	COLORREF			m_fgcol;
	COLORREF			m_bgcol;
	CString				m_format;

	int m_topmargin;	// Current top margin
	int m_topline;		// Current top line number
	int m_bottomline;	// Current bottom line number
};

// CLineNumberEdit window

class CLineNumberEdit : public CEdit
{
// Construction/destruction
public:
	CLineNumberEdit();
	virtual ~CLineNumberEdit();

// Operations
public:
	void SetMarginForegroundColor( COLORREF col, BOOL redraw = TRUE, BOOL bEnabled = TRUE );
	void SetMarginBackgroundColor( COLORREF col, BOOL redraw = TRUE, BOOL bEnabled = TRUE );
	void SetLineNumberFormat( CString format );
	void SetLineNumberRange( UINT nMin, UINT nMax = 0 );
	void UseSystemColours( BOOL bUseEnabled = TRUE, BOOL bUseDisabled = TRUE );

	int GetLineHeight()
	{
		return m_zero.cy;
	}

protected:
	virtual void PreSubclassWindow();

	virtual afx_msg void OnEnable( BOOL bEnable );
	virtual afx_msg void OnSysColorChange();
	virtual afx_msg void OnChange();
	virtual afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual afx_msg void OnVscroll();
	virtual afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam); // Maps to WM_SETFONT
	virtual afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam); // Maps to WM_SETTEXT
	virtual afx_msg LRESULT OnLineScroll(WPARAM wParam, LPARAM lParam); // Maps to EM_LINESCROLL
	virtual afx_msg LRESULT OnSelectLine(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	void Prepare();
	int CalcLineNumberWidth();
	void UpdateTopAndBottom();

	// Method to set window colour only
	void SetWindowColour( BOOL bEnable = TRUE );

// Attributes
	BOOL				m_bUseEnabledSystemColours;
	COLORREF			m_EnabledFgCol;
	COLORREF			m_EnabledBgCol;
	BOOL				m_bUseDisabledSystemColours;
	COLORREF			m_DisabledFgCol;
	COLORREF			m_DisabledBgCol;

	CLineNumberStatic	m_line;
	CSize				m_zero;
	int					m_maxval;
	CString				m_format;
	int                 m_LineDelta;
};

#endif // !defined(AFX_LINENUMBEREDIT_H__CAB7A465_709C_42B8_80D0_2B0AF6D25AD4__INCLUDED_)
