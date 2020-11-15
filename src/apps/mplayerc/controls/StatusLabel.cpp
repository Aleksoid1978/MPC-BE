/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2020 see Authors.txt
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
#include "../MainFrm.h"
#include "../../DSUtil/SysVersion.h"

// CStatusLabel

IMPLEMENT_DYNAMIC(CStatusLabel, CStatic)
CStatusLabel::CStatusLabel(bool fRightAlign, bool fAddEllipses)
	: m_fRightAlign(fRightAlign)
	, m_fAddEllipses(fAddEllipses)
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
		m_font.CreateFontW(AfxGetMainFrame()->ScaleY(13), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
						   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
						   L"Tahoma");
	} else {
		m_font.CreateFontW(AfxGetMainFrame()->ScaleY(16), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
						   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
						   L"Segoe UI");
	}
}

BEGIN_MESSAGE_MAP(CStatusLabel, CStatic)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// CStatusLabel message handlers

BOOL CStatusLabel::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_LBUTTONDOWN) {
		GetParent()->SendMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
	}

	return __super::PreTranslateMessage(pMsg);
}

void CStatusLabel::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	CString str;
	GetWindowTextW(str);
	CRect rc;
	GetClientRect(&rc);
	CFont* old = dc.SelectObject(&m_font);

	const CAppSettings& s = AfxGetAppSettings();

	if (s.bUseDarkTheme) {
		dc.SetTextColor(ThemeRGB(165, 170, 175));
		dc.SetBkColor(ThemeRGB(5, 10, 15));
	} else {
		dc.SetTextColor(0xffffff);
		dc.SetBkColor(0);
	}

	UINT format = DT_VCENTER | DT_SINGLELINE;
	if (m_fAddEllipses) {
		format |= DT_END_ELLIPSIS;
	}
	if (m_fRightAlign) {
		format |= DT_RIGHT;
	} else {
		format |= DT_LEFT;
	}

	dc.SelectObject(&old);
	dc.FillSolidRect(&rc, s.bUseDarkTheme ? ThemeRGB(5, 10, 15) : 0);
	dc.DrawTextW(str, &rc, format);

	dc.Detach();
}

BOOL CStatusLabel::OnEraseBkgnd(CDC* pDC)
{
	/*
	CRect r;
	GetClientRect(&r);
	if (AfxGetAppSettings().bUseDarkTheme) {
		pDC->FillSolidRect(&r, ThemeRGB(5, 10, 15));
		pDC->FillSolidRect(&r, 0);
	} else {
		pDC->FillSolidRect(&r, 0);
	}
	*/

	return TRUE;
}
