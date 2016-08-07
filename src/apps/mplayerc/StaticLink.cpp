/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include "StaticLink.h"

// CStaticLink

COLORREF CStaticLink::g_colorUnvisited = RGB(0,0,255);
COLORREF CStaticLink::g_colorVisited   = RGB(128,0,128);

IMPLEMENT_DYNAMIC(CStaticLink, CStatic)

BEGIN_MESSAGE_MAP(CStaticLink, CStatic)
	ON_WM_NCHITTEST()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

CStaticLink::CStaticLink(LPCTSTR lpText, bool bDeleteOnDestroy)
{
	m_link = lpText;
	m_color = g_colorUnvisited;
	m_bDeleteOnDestroy = bDeleteOnDestroy;
}

LRESULT CStaticLink::OnNcHitTest(CPoint point)
{
	return HTCLIENT;
}

HBRUSH CStaticLink::CtlColor(CDC* pDC, UINT nCtlColor)
{
	ASSERT(nCtlColor == CTLCOLOR_STATIC);
	DWORD dwStyle = GetStyle();

	HBRUSH hbr = NULL;
	if ((dwStyle & 0xFF) <= SS_RIGHT) {

		if (!(HFONT)m_font) {

			LOGFONT lf;
			GetFont()->GetObject(sizeof(lf), &lf);
			lf.lfUnderline = TRUE;
			m_font.CreateFontIndirect(&lf);
		}

		pDC->SelectObject(&m_font);
		pDC->SetTextColor(m_color);
		pDC->SetBkMode(TRANSPARENT);

		hbr = (HBRUSH)::GetStockObject(HOLLOW_BRUSH);
	}
	return hbr;
}

void CStaticLink::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_link.IsEmpty()) {

		m_link.LoadString(GetDlgCtrlID()) || (GetWindowText(m_link),1);
		if (m_link.IsEmpty()) {
			return;
		}
	}

	HINSTANCE h = m_link.Navigate();
	if ((UINT_PTR)h > 32) {
		m_color = g_colorVisited;
		Invalidate();
	} else {
		MessageBeep(0);
		DLog(L"*** WARNING: CStaticLink: unable to navigate link %s",
			  (LPCTSTR)m_link);
	}
}

BOOL CStaticLink::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));

	return TRUE;
}

void CStaticLink::PostNcDestroy()
{
	if (m_bDeleteOnDestroy) {
		delete this;
	}
}
