/*
 * (C) 2012-2015 see Authors.txt
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
#include "MainFrm.h"
#include "PlayerPreView.h"

static COLORREF RGBFill(int r1, int g1, int b1, int r2, int g2, int b2, int i, int k)
{
	int r, g, b;

	r = r1 + (i * (r2 - r1) / k);
	g = g1 + (i * (g2 - g1) / k);
	b = b1 + (i * (b2 - b1) / k);

	return RGB(r, g, b);
}


// CPrevView

CPreView::CPreView()
{
}

CPreView::~CPreView()
{
}

BOOL CPreView::SetWindowText(LPCWSTR lpString)
{
	m_tooltipstr = lpString;

	CRect rect;
	GetClientRect(rect);

	rect.bottom = m_caption;
	rect.left += 10;
	rect.right -= 10;

	InvalidateRect(rect);

	return ::SetWindowText(m_hWnd, lpString);
}

void CPreView::GetVideoRect(LPRECT lpRect)
{
	m_view.GetClientRect(lpRect);
}

HWND CPreView::GetVideoHWND()
{
	return m_view.GetSafeHwnd();
}

IMPLEMENT_DYNAMIC(CPreView, CWnd)

BEGIN_MESSAGE_MAP(CPreView, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// CPreView message handlers

BOOL CPreView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.style &= ~WS_BORDER;

	return TRUE;
}

int CPreView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	m_caption = ScaleY(20);

	CRect rc;
	GetClientRect(rc);

	m_videorect.left   = (m_border + 1);
	m_videorect.top    = (m_caption + 1);
	m_videorect.right  = rc.right  - (m_border + 1);
	m_videorect.bottom = rc.bottom - (m_border + 1);

	if (!m_view.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, m_videorect, this, 0)) {
		return -1;
	}

	return 0;
}

void CPreView::OnPaint()
{
	CPaintDC dc(this);

	CRect rcBar;
	GetClientRect(rcBar);

	CDC mdc;
	mdc.CreateCompatibleDC(&dc);

	CBitmap bm;
	bm.CreateCompatibleBitmap(&dc, rcBar.Width(), rcBar.Height());
	CBitmap* pOldBm = mdc.SelectObject(&bm);
	mdc.SetBkMode(TRANSPARENT);

	int r1, g1, b1, r2, g2, b2, i, k;
	COLORREF bg = GetSysColor(COLOR_BTNFACE);
	COLORREF light = RGB(255,255,255);
	COLORREF shadow = GetSysColor(COLOR_BTNSHADOW);

	CAppSettings& s = AfxGetAppSettings();

	if (s.bUseDarkTheme) {
		ThemeRGB(95, 100, 105, r1, g1, b1);
		ThemeRGB(25, 30, 35, r2, g2, b2);
	} else {
		r1 = r2 = GetRValue(bg);
		g1 = g2 = GetGValue(bg);
		b1 = b2 = GetBValue(bg);
	}
	k = rcBar.Height();
	for(i=0;i<k;i++) {
		mdc.FillSolidRect(0,i,rcBar.Width(),1,RGBFill(r1, g1, b1, r2, g2, b2, i, k));
	}

	if (s.bUseDarkTheme) {
		ThemeRGB(145, 140, 145, r1, g1, b1);
		ThemeRGB(115, 120, 125, r2, g2, b2);
	} else {
		r1 = r2 = GetRValue(light);
		g1 = g2 = GetGValue(light);
		b1 = b2 = GetBValue(light);
	}
	k = rcBar.Width();
	for(i=0;i<k;i++) {
		mdc.FillSolidRect(i,0,1,1,RGBFill(r1, g1, b1, r2, g2, b2, i, k));
	}

	if (s.bUseDarkTheme) {
		ThemeRGB(15, 20, 25, r1, g1, b1);
		ThemeRGB(55, 60, 65, r2, g2, b2);
	} else {
		r1 = r2 = GetRValue(shadow);
		g1 = g2 = GetGValue(shadow);
		b1 = b2 = GetBValue(shadow);
	}
	k = rcBar.Width();
	for(i=rcBar.left+m_border;i<k-m_border;i++) {
		mdc.FillSolidRect(i,m_caption,1,1,RGBFill(r1, g1, b1, r2, g2, b2, i, k));
	}

	if (s.bUseDarkTheme) {
		ThemeRGB(35, 40, 45, r1, g1, b1);
		ThemeRGB(55, 60, 65, r2, g2, b2);
	} else {
		r1 = r2 = GetRValue(light);
		g1 = g2 = GetGValue(light);
		b1 = b2 = GetBValue(light);
	}
	k = rcBar.Width();
	for(i=rcBar.left+m_border;i<k-m_border;i++) {
		mdc.FillSolidRect(i,rcBar.bottom-m_border-1,1,1,RGBFill(r1, g1, b1, r2, g2, b2, i, k));
	}

	if (s.bUseDarkTheme) {
		ThemeRGB(0, 5, 10, r1, g1, b1);
		ThemeRGB(10, 15, 20, r2, g2, b2);
	} else {
		r1 = r2 = GetRValue(shadow);
		g1 = g2 = GetGValue(shadow);
		b1 = b2 = GetBValue(shadow);
	}
	k = rcBar.Width();
	for(i=0;i<k;i++) {
		mdc.FillSolidRect(i,rcBar.bottom-1,1,1,RGBFill(r1, g1, b1, r2, g2, b2, i, k));
	}

	if (s.bUseDarkTheme) {
		ThemeRGB(145, 150, 155, r1, g1, b1);
		ThemeRGB(45, 50, 55, r2, g2, b2);
	} else {
		r1 = r2 = GetRValue(light);
		g1 = g2 = GetGValue(light);
		b1 = b2 = GetBValue(light);
	}
	k = rcBar.Height();
	for(i=0;i<k-1;i++) {
		mdc.FillSolidRect(0,i,1,1,RGBFill(r1, g1, b1, r2, g2, b2, i, k));
	}

	if (s.bUseDarkTheme) {
		ThemeRGB(55, 60, 65, r1, g1, b1);
		ThemeRGB(15, 20, 25, r2, g2, b2);
	} else {
		r1 = r2 = GetRValue(shadow);
		g1 = g2 = GetGValue(shadow);
		b1 = b2 = GetBValue(shadow);
	}
	k = rcBar.Height();
	for(i=m_caption;i<k-m_border;i++) {
		mdc.FillSolidRect(m_border,i,1,1,RGBFill(r1, g1, b1, r2, g2, b2, i, k));
	}

	if (s.bUseDarkTheme) {
		ThemeRGB(105, 110, 115, r1, g1, b1);
		ThemeRGB(55, 60, 65, r2, g2, b2);
	} else {
		r1 = r2 = GetRValue(light);
		g1 = g2 = GetGValue(light);
		b1 = b2 = GetBValue(light);
	}
	k = rcBar.Height();
	for(i=m_caption;i<k-m_border;i++) {
		mdc.FillSolidRect(rcBar.right-m_border-1,i,1,1,RGBFill(r1, g1, b1, r2, g2, b2, i, k));
	}

	if (s.bUseDarkTheme) {
		ThemeRGB(65, 70, 75, r1, g1, b1);
		ThemeRGB(5, 10, 15, r2, g2, b2);
	} else {
		r1 = r2 = GetRValue(shadow);
		g1 = g2 = GetGValue(shadow);
		b1 = b2 = GetBValue(shadow);
	}
	k = rcBar.Height();
	for(i=0;i<k;i++) {
		mdc.FillSolidRect(rcBar.right-1,i,1,1,RGBFill(r1, g1, b1, r2, g2, b2, i, k));
	}

	// text (time)
	CFont font;

	if (s.bUseDarkTheme) {
		ThemeRGB(255, 255, 255, r1, g1, b1);
	} else {
		r1 = GetRValue(0);
		g1 = GetGValue(0);
		b1 = GetBValue(0);
	}

	mdc.SetTextColor(RGB(r1,g1,b1));

	font.CreateFont(ScaleY(13), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
									OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH | FF_MODERN,
									_T("Tahoma"));

	mdc.SelectObject(&font);
	CRect rtime = rcBar;
	rtime.top = 0;
	rtime.bottom = m_caption;
	mdc.DrawText(m_tooltipstr, m_tooltipstr.GetLength(), &rtime, DT_CENTER|DT_VCENTER|DT_SINGLELINE);

	dc.ExcludeClipRect(m_videorect);
	dc.BitBlt(0, 0, rcBar.Width(), rcBar.Height(), &mdc, 0, 0, SRCCOPY);

	mdc.SelectObject(pOldBm);
	bm.DeleteObject();
	mdc.DeleteDC();
}

void CPreView::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow) {
		auto pFrame = AfxGetMainFrame();
		if (pFrame) {
			pFrame->SetPreviewVideoPosition();
		}
	}

	__super::OnShowWindow(bShow, nStatus);
}

void CPreView::SetWindowSize()
{
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(MonitorFromWindow(GetParent()->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST), &mi);

	CRect wr;
	GetParent()->GetClientRect(wr);

	int w = (mi.rcWork.right - mi.rcWork.left) * m_relativeSize / 100;
	// the preview size should not be larger than half size of the main window, but not less than 160
	w = max(160, min(w, wr.Width() / 2));

	int h = (w - ((m_border + 1) * 2)) * 9 / 16;
	h += (m_caption + 1);
	h += (m_border + 1);

	CRect rc;
	GetClientRect(&rc);
	if (rc.Width() != w || rc.Height() != h) {
		SetWindowPos(NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		GetClientRect(&rc);
		m_videorect.right = rc.right - (m_border + 1);
		m_videorect.bottom = rc.bottom - (m_border + 1);

		m_view.SetWindowPos(NULL, 0, 0, m_videorect.Width(), m_videorect.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}
