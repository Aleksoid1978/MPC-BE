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
#include "StatusLabel.h"
#include "MainFrm.h"
#include "../../DSUtil/SysVersion.h"

// CStatusLabel

IMPLEMENT_DYNAMIC(CStatusLabel, CStatic)
CStatusLabel::CStatusLabel(bool fRightAlign, bool fAddEllipses)
	: m_fRightAlign(fRightAlign)
	, m_fAddEllipses(fAddEllipses)
{
	m_font.m_hObject = NULL;
}

CStatusLabel::~CStatusLabel()
{
}

BOOL CStatusLabel::Create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	if (!__super::Create(lpszText, dwStyle, rect, pParentWnd, nID)) {
		return FALSE;
	}

	ScaleFont();

	return TRUE;
}

void CStatusLabel::ScaleFont()
{
	m_font.DeleteObject();

	if (AfxGetAppSettings().bUseDarkTheme) {
		m_font.CreateFont(AfxGetMainFrame()->ScaleY(13), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
						  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
						  _T("Tahoma"));
	} else {
		m_font.CreateFont(AfxGetMainFrame()->ScaleY(16), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
						  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
						  _T("Segoe UI"));
	}
}

BEGIN_MESSAGE_MAP(CStatusLabel, CStatic)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// CStatusLabel message handlers

BOOL CStatusLabel::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_LBUTTONDOWN) {
		GetParent()->SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
	}

	return __super::PreTranslateMessage(pMsg);
}

void CStatusLabel::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	CString str;
	GetWindowText(str);
	CRect r;
	int R, G, B;
	GetClientRect(&r);
	CFont* old = dc.SelectObject(&m_font);

	CAppSettings& s = AfxGetAppSettings();

	if (s.bUseDarkTheme) {
		ThemeRGB(165, 170, 175, R, G, B);
		dc.SetTextColor(RGB(R,G,B));
		ThemeRGB(5, 10, 15, R, G, B);
		dc.SetBkColor(RGB(R,G,B));
	} else {
		dc.SetTextColor(0xffffff);
		dc.SetBkColor(0);
	}

	CSize size = dc.GetTextExtent(str);
	CPoint p = CPoint(m_fRightAlign ? r.Width() - size.cx : 0, (r.Height()-size.cy)/2);

	if (m_fAddEllipses) {
		while (size.cx > r.Width()-3 && str.GetLength() > 3) {
			str = str.Left(str.GetLength()-4) + _T("...");
			size = dc.GetTextExtent(str);
		}
	}

	if (s.bUseDarkTheme) {
		dc.SelectObject(&old);
		ThemeRGB(5, 10, 15, R, G, B);
		dc.FillSolidRect(&r, RGB(R, G, B));
		dc.TextOut(p.x, p.y, str);
	} else {
		dc.TextOut(p.x, p.y, str);
		dc.ExcludeClipRect(CRect(p, size));
		dc.SelectObject(&old);
		dc.FillSolidRect(&r, 0);
	}

	dc.Detach();
}

BOOL CStatusLabel::OnEraseBkgnd(CDC* pDC)
{
	CRect r;
	GetClientRect(&r);
	int R, G, B;

	if (AfxGetAppSettings().bUseDarkTheme) {
		ThemeRGB(5, 10, 15, R, G, B);
		pDC->FillSolidRect(&r, RGB(R,G,B));
		pDC->FillSolidRect(&r, 0);
	} else {
		pDC->FillSolidRect(&r, 0);
	}

	return TRUE;
}
