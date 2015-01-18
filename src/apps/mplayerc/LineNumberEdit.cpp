/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include "LineNumberEdit.h"

UINT urm_SELECTLINE = ::RegisterWindowMessage( _T("_LINE_NUMBER_EDIT_SELECTLINE_") );

// CLineNumberEdit

CLineNumberEdit::CLineNumberEdit()
{
	m_hWnd = NULL;
	m_line.m_hWnd = NULL;
	m_zero.cx = 0;
	m_zero.cy = 0;
	m_format = _T( "%03i" );
	m_LineDelta = 1;

	// Could default m_maxval to 99,999, but may cause problems
	// if m_format is changed and m_maxval is not...
	m_maxval = 998;

	// Setting up so by defult the original hard-coded colour
	// scheme is used when enabled and the system colours are
	// used when disabled.
	m_bUseEnabledSystemColours = FALSE;
	m_bUseDisabledSystemColours = TRUE;
	m_EnabledFgCol = RGB( 0, 0, 0 );
	m_EnabledBgCol = RGB( 200, 200, 200 );
	m_DisabledFgCol = GetSysColor( COLOR_GRAYTEXT );
	m_DisabledBgCol = GetSysColor( COLOR_3DFACE );

	SetWindowColour();
}

CLineNumberEdit::~CLineNumberEdit()
{
}

BEGIN_MESSAGE_MAP(CLineNumberEdit, CEdit)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	ON_WM_VSCROLL()
	ON_CONTROL_REFLECT(EN_VSCROLL, OnVscroll)
	ON_MESSAGE(WM_SETFONT, OnSetFont)
	ON_WM_SIZE()
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	ON_WM_SYSCOLORCHANGE()
	ON_WM_ENABLE()
	ON_MESSAGE(EM_LINESCROLL, OnLineScroll)
	ON_REGISTERED_MESSAGE(urm_SELECTLINE, OnSelectLine)
END_MESSAGE_MAP()

void CLineNumberEdit::PreSubclassWindow()
{
	// Unfortunately, we can't change to ES_MULTILINE
	// during run-time.
	ASSERT( GetStyle() & ES_MULTILINE );

	// Creating the line number control
	SetLineNumberFormat( m_format );
}

// CLineNumberEdit message handlers

void CLineNumberEdit::OnSysColorChange()
{
	CEdit::OnSysColorChange();

	// update the CStatic with the new colours
	SetWindowColour( IsWindowEnabled() );
}

LRESULT CLineNumberEdit::OnSetText( WPARAM wParam, LPARAM lParam )
{
	// Default processing
	LRESULT retval = DefWindowProc( WM_SETTEXT, wParam, lParam );
	UpdateTopAndBottom();
	return retval;
}

void CLineNumberEdit::OnChange()
{
	UpdateTopAndBottom();
}

void CLineNumberEdit::OnVscroll()
{
	UpdateTopAndBottom();
}

void CLineNumberEdit::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	CEdit::OnVScroll( nSBCode, nPos, pScrollBar );
	UpdateTopAndBottom();
}

LRESULT CLineNumberEdit::OnLineScroll( WPARAM wParam, LPARAM lParam )
{
	// Default processing
	LRESULT retval = DefWindowProc( EM_LINESCROLL, wParam, lParam );
	UpdateTopAndBottom();
	return retval;
}

LRESULT CLineNumberEdit::OnSetFont( WPARAM wParam, LPARAM lParam )
{
	DefWindowProc( WM_SETFONT, wParam, lParam );
	// We resize the line-number
	// field
	Prepare();
	return 0;
}

void CLineNumberEdit::OnSize( UINT nType, int cx, int cy )
{
	CEdit::OnSize( nType, cx, cy );

	// If we have the line-number
	// control, it must be resized
	// as well.
	if( m_line.m_hWnd )
		Prepare();
}

void CLineNumberEdit::OnEnable( BOOL bEnable )
{
	CEdit::OnEnable( bEnable );
	SetWindowColour( bEnable );
}

LRESULT CLineNumberEdit::OnSelectLine(WPARAM wParam, LPARAM /*lParam*/ )
{
	// Calc start and end position of the line
	int lineno = static_cast<int>(wParam) + GetScrollPos( SB_VERT );
	int start = LineIndex( lineno );
	int end = LineIndex( lineno + 1 );
	SetSel( start, end - 1 );
	return 0;
}

void CLineNumberEdit::SetWindowColour( BOOL bEnable /*= TRUE*/ )
{
	if (m_bUseEnabledSystemColours)
	{
		// re-query the system colours in case they have changed.
		m_EnabledFgCol = GetSysColor( COLOR_WINDOWTEXT );
		m_EnabledBgCol = GetSysColor( COLOR_WINDOW );
	}

	if (m_bUseDisabledSystemColours)
	{
		// re-query the system colours in case they have changed.
		m_DisabledFgCol = GetSysColor( COLOR_GRAYTEXT );
		m_DisabledBgCol = GetSysColor( COLOR_3DFACE );
	}

	// change the colour based on bEnable
	if (bEnable)
	{
		m_line.SetFgColor( m_EnabledFgCol, TRUE );
		m_line.SetBgColor( m_EnabledBgCol, TRUE );
	}
	else
	{
		m_line.SetFgColor( m_DisabledFgCol, TRUE );
		m_line.SetBgColor( m_DisabledBgCol, TRUE );
	}
}

void CLineNumberEdit::UseSystemColours( BOOL bUseEnabled /*= TRUE*/, BOOL bUseDisabled /*= TRUE*/ )
{
	m_bUseEnabledSystemColours = bUseEnabled;
	m_bUseDisabledSystemColours = bUseDisabled;
	BOOL bEnable = TRUE;
	if (::IsWindow(m_hWnd))
		bEnable = IsWindowEnabled();

	SetWindowColour( bEnable );
}

// CLineNumberEdit private implementation

void CLineNumberEdit::Prepare()
{
	// Calc sizes
	int width = CalcLineNumberWidth();
	CRect rect;
	GetClientRect( &rect );
	CRect rectEdit( rect );
	rect.right = width;
	rectEdit.left = rect.right + 3;

	// Setting the edit rect and
	// creating or moving child control
	SetRect( &rectEdit );
	if( m_line.m_hWnd )
		m_line.MoveWindow( 0, 0, width, rect.Height() );
	else
		m_line.Create(NULL,WS_CHILD | WS_VISIBLE | SS_NOTIFY, rect, this, 1 );

	GetRect( &rectEdit );

	// Update line number control data
	m_line.SetTopMargin( rectEdit.top );
	UpdateTopAndBottom();
}

int CLineNumberEdit::CalcLineNumberWidth()
{
	CClientDC dc( this );

	// If a new font is set during runtime,
	// we must explicitly select the font into
	// the CClientDC to measure it.
	CFont* font = GetFont();
	CFont* oldFont = dc.SelectObject( font );

	m_zero=dc.GetTextExtent( _T( "0" ) );
	CString format;

	// GetLimitText returns the number of bytes the edit box may contain,
	// not the max number of lines...
	//... which is the max number of lines, given one character per d:o :-)
	int maxval = GetLimitText();
	if (m_maxval > 0)
		maxval = m_maxval + m_LineDelta;

	format.Format( m_format, maxval );
	CSize fmt = dc.GetTextExtent( format );
	dc.SelectObject( oldFont );

	// Calculate the size of the line-
	// number field. We add a 5 pixel margin
	// to the max size of the format string
	return fmt.cx + 5;
}

void CLineNumberEdit::UpdateTopAndBottom()
{
	CRect rect;
	GetClientRect( &rect );
	int maxline = GetLineCount() + m_LineDelta;

	// Height for individual lines
	int lineheight = m_zero.cy;

	// Calculate the number of lines to draw
	int topline = GetFirstVisibleLine() + m_LineDelta;
	if( ( topline + ( rect.Height() / lineheight ) ) < maxline )
		maxline = topline + ( rect.Height() / lineheight );

	if ( m_maxval > 0 && maxline > m_maxval + m_LineDelta )
		maxline = m_maxval + m_LineDelta;

	m_line.SetTopAndBottom( topline, maxline );
}

// CLineNumberEdit public implementation

void CLineNumberEdit::SetMarginForegroundColor( COLORREF col, BOOL redraw, BOOL bEnabled /*= TRUE*/ )
{
	m_line.SetFgColor( col, redraw );
	if (bEnabled)
	{
		m_bUseEnabledSystemColours = FALSE;
		m_EnabledFgCol = col;
	}
	else
	{
		m_bUseDisabledSystemColours = FALSE;
		m_DisabledFgCol = col;
	}
}

void CLineNumberEdit::SetMarginBackgroundColor( COLORREF col, BOOL redraw, BOOL bEnabled /*= TRUE*/ )
{
	m_line.SetBgColor( col, redraw );
	if (bEnabled)
	{
		m_bUseEnabledSystemColours = FALSE;
		m_EnabledBgCol = col;
	}
	else
	{
		m_bUseDisabledSystemColours = FALSE;
		m_DisabledBgCol = col;
	}
}

void CLineNumberEdit::SetLineNumberFormat( CString format )
{
	m_format = format;
	m_line.SetLineNumberFormat( format );
	if( m_hWnd )
		Prepare();
}

void CLineNumberEdit::SetLineNumberRange( UINT nMin, UINT nMax /*= 0*/ )
{
	m_LineDelta = ( int ) nMin;
	m_maxval = ( int ) nMax;
}

// CLineNumberStatic

CLineNumberStatic::CLineNumberStatic()
{
	m_bgcol = RGB( 255, 255, 248 );
	m_fgcol = RGB( 0, 0, 0 );
	m_format = _T( "%05i" );
	m_topline = 0;
	m_bottomline = 0;
}

CLineNumberStatic::~CLineNumberStatic()
{
}

BEGIN_MESSAGE_MAP(CLineNumberStatic, CStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

// CLineNumberStatic message handlers

void CLineNumberStatic::OnPaint()
{
	CPaintDC dcPaint( this );

	CRect rect;
	GetClientRect( &rect );

	// We double buffer the drawing -
	// preparing the memory CDC
	CDC dc;
	dc.CreateCompatibleDC( &dcPaint );
	int saved = dc.SaveDC();

	// Create GDI and select objects
	CBitmap bmp;
	CPen pen;
	bmp.CreateCompatibleBitmap( &dcPaint, rect.Width(), rect.Height() );
	pen.CreatePen( PS_SOLID, 1, m_fgcol );
	dc.SelectObject( &bmp );
	dc.SelectObject( &pen );

	// Painting the background
	dc.FillSolidRect( &rect, m_bgcol );
	dc.MoveTo( rect.right - 1, 0 );
	dc.LineTo( rect.right - 1, rect.bottom );

	// Setting other attributes
	dc.SetTextColor( m_fgcol );
	dc.SetBkColor( m_bgcol );
	dc.SelectObject( GetParent()->GetFont() );

	// Output the line numbers
	if( m_bottomline )
	{
		int lineheight = dc.GetTextExtent( _T( "0" ) ).cy;
		for( int t = m_topline ; t < m_bottomline ; t++ )
		{
			CString output;
			output.Format( m_format, t );
			int topposition = m_topmargin + lineheight * ( t - m_topline );
			dc.TextOut( 2, topposition, output );
		}
	}

	dcPaint.BitBlt( 0, 0, rect. right, rect.bottom, &dc, 0, 0, SRCCOPY );
	dc.RestoreDC( saved );
}

BOOL CLineNumberStatic::OnEraseBkgnd( CDC* )
{
	return TRUE;
}

void CLineNumberStatic::OnLButtonDown( UINT nFlags, CPoint point )
{
	// Find the line clicked on
	CClientDC	dc( this );
	dc.SelectObject( GetParent()->GetFont() );
	int lineheight = dc.GetTextExtent( _T( "0" ) ).cy;
	int lineno = ( int ) ( ( double ) point.y / ( double ) lineheight );

	// Select this line in the edit control
	GetParent()->SendMessage( urm_SELECTLINE, lineno );

	CStatic::OnLButtonDown( nFlags, point );
}

// CLineNumberStatic public implementation

void CLineNumberStatic::SetBgColor( COLORREF col, BOOL redraw )
{
	m_bgcol = col;
	if( m_hWnd && redraw )
		RedrawWindow();
}

void CLineNumberStatic::SetFgColor( COLORREF col, BOOL redraw )
{
	m_fgcol = col;
	if( m_hWnd && redraw )
		RedrawWindow();
}

void CLineNumberStatic::SetTopAndBottom( int topline, int bottomline )
{
	m_topline = topline;
	m_bottomline = bottomline;
	if( m_hWnd )
		RedrawWindow();
}

void CLineNumberStatic::SetTopMargin( int topmargin )
{
	m_topmargin = topmargin;
}

void CLineNumberStatic::SetLineNumberFormat( CString format )
{
	m_format = format;
	if( m_hWnd )
		RedrawWindow();
}
