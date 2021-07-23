/*
 * (C) 2012-2020 see Authors.txt
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
	const int r = r1 + MulDiv(i, r2 - r1, k);
	const int g = g1 + MulDiv(i, g2 - g1, k);
	const int b = b1 + MulDiv(i, b2 - b1, k);

	return RGB(r, g, b);
}

// CPrevView

CPreView::CPreView(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
{
}

BOOL CPreView::SetWindowTextW(LPCWSTR lpString)
{
	m_tooltipstr = lpString;

	CRect rect;
	GetClientRect(&rect);

	rect.bottom = m_caption;
	rect.left += 10;
	rect.right -= 10;

	InvalidateRect(rect);

	return ::SetWindowTextW(m_hWnd, lpString);
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

	m_caption = m_pMainFrame->ScaleY(20);

	CRect rc;
	GetClientRect(&rc);

	m_videorect.left   = (m_border + 1);
	m_videorect.top    = (m_caption + 1);
	m_videorect.right  = rc.right  - (m_border + 1);
	m_videorect.bottom = rc.bottom - (m_border + 1);

	if (!m_view.Create(nullptr, nullptr, WS_CHILD | WS_VISIBLE, m_videorect, this, 0)) {
		return -1;
	}

	ScaleFont();
	SetColor();

	return 0;
}

void CPreView::OnPaint()
{
	CPaintDC dc(this);

	CRect rcBar;
	GetClientRect(&rcBar);
	const int w = rcBar.Width();
	const int h = rcBar.Height();

	CDC mdc;
	mdc.CreateCompatibleDC(&dc);

	CBitmap bm;
	bm.CreateCompatibleBitmap(&dc, w, h);
	CBitmap* pOldBm = mdc.SelectObject(&bm);

	TRIVERTEX vert[2] = {
		{ 0, 0, COLOR16(m_cr1.R1 * 256), COLOR16(m_cr1.G1 * 256), COLOR16(m_cr1.B1 * 256), 0 },
		{ w, h, COLOR16(m_cr1.R2 * 256), COLOR16(m_cr1.G2 * 256), COLOR16(m_cr1.B2 * 256), 0 }
	};
	GRADIENT_RECT GradientRect = { 0 , 1 };
	mdc.GradientFill(vert, 2, &GradientRect, 1, GRADIENT_FILL_RECT_V);

	int i, k;

	k = w;
	for(i = 0; i < k; i++) {
		mdc.SetPixelV(i, 0, RGBFill(m_cr2.R1, m_cr2.G1, m_cr2.B1, m_cr2.R2, m_cr2.G2, m_cr2.B2, i, k));
	}

	k = w;
	for(i = rcBar.left + m_border; i < k - m_border; i++) {
		mdc.SetPixelV(i, m_caption, RGBFill(m_cr3.R1, m_cr3.G1, m_cr3.B1, m_cr3.R2, m_cr3.G2, m_cr3.B2, i, k));
	}

	k = w;
	for(i = rcBar.left + m_border; i < k - m_border; i++) {
		mdc.SetPixelV(i, rcBar.bottom - m_border - 1, RGBFill(m_cr4.R1, m_cr4.G1, m_cr4.B1, m_cr4.R2, m_cr4.G2, m_cr4.B2, i, k));
	}

	k = w;
	for(i = 0; i < k; i++) {
		mdc.SetPixelV(i, rcBar.bottom - 1, RGBFill(m_cr5.R1, m_cr5.G1, m_cr5.B1, m_cr5.R2, m_cr5.G2, m_cr5.B2, i, k));
	}

	k = h;
	for(i = 0; i < k - 1; i++) {
		mdc.SetPixelV(0, i, RGBFill(m_cr6.R1, m_cr6.G1, m_cr6.B1, m_cr6.R2, m_cr6.G2, m_cr6.B2, i, k));
	}

	k = h;
	for(i = m_caption; i < k - m_border; i++) {
		mdc.SetPixelV(m_border, i, RGBFill(m_cr7.R1, m_cr7.G1, m_cr7.B1, m_cr7.R2, m_cr7.G2, m_cr7.B2, i, k));
	}

	k = h;
	for(i = m_caption; i < k - m_border; i++) {
		mdc.SetPixelV(rcBar.right - m_border - 1, i, RGBFill(m_cr8.R1, m_cr8.G1, m_cr8.B1, m_cr8.R2, m_cr8.G2, m_cr8.B2, i, k));
	}

	k = h;
	for(i = 0 ; i < k; i++) {
		mdc.SetPixelV(rcBar.right - 1, i, RGBFill(m_cr9.R1, m_cr9.G1, m_cr9.B1, m_cr9.R2, m_cr9.G2, m_cr9.B2, i, k));
	}

	// text
	mdc.SelectObject(&m_font);
	CRect rtime(rcBar);
	rtime.top = 0;
	rtime.bottom = m_caption;
	mdc.SetBkMode(TRANSPARENT);
	mdc.SetTextColor(m_crText);
	mdc.DrawTextW(m_tooltipstr, m_tooltipstr.GetLength(), &rtime, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	dc.ExcludeClipRect(m_videorect);
	dc.BitBlt(0, 0, w, h, &mdc, 0, 0, SRCCOPY);

	mdc.SelectObject(pOldBm);
	bm.DeleteObject();
	mdc.DeleteDC();
}

void CPreView::OnShowWindow(BOOL bShow, UINT nStatus)
{
	if (bShow) {
		m_pMainFrame->SetPreviewVideoPosition();
	}

	__super::OnShowWindow(bShow, nStatus);
}

void CPreView::SetWindowSize()
{
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfoW(MonitorFromWindow(GetParent()->GetSafeHwnd(), MONITOR_DEFAULTTONEAREST), &mi);

	CRect wr;
	GetParent()->GetClientRect(&wr);

	int w = (mi.rcWork.right - mi.rcWork.left) * m_relativeSize / 100;
	// the preview size should not be larger than half size of the main window, but not less than 160
	w = std::max(160, std::min(w, wr.Width() / 2));

	int h = (w - ((m_border + 1) * 2)) * 9 / 16;
	h += (m_caption + 1);
	h += (m_border + 1);

	CRect rc;
	GetClientRect(&rc);
	if (rc.Width() != w || rc.Height() != h) {
		SetWindowPos(nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		GetClientRect(&rc);
		m_videorect.right = rc.right - (m_border + 1);
		m_videorect.bottom = rc.bottom - (m_border + 1);

		m_view.SetWindowPos(nullptr, 0, 0, m_videorect.Width(), m_videorect.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void CPreView::ScaleFont()
{
	m_font.DeleteObject();
	m_font.CreateFontW(m_pMainFrame->ScaleY(13), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
					   OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH | FF_MODERN,
					   L"Tahoma");
}

void CPreView::SetColor()
{
	const COLORREF bg = GetSysColor(COLOR_BTNFACE);
	const COLORREF light = RGB(255, 255, 255);
	const COLORREF shadow = GetSysColor(COLOR_BTNSHADOW);

	const auto bUseDarkTheme = AfxGetAppSettings().bUseDarkTheme;

	if (bUseDarkTheme) {
		ThemeRGB(95, 100, 105, m_cr1.R1, m_cr1.G1, m_cr1.B1);
		ThemeRGB(25, 30, 35, m_cr1.R2, m_cr1.G2, m_cr1.B2);
	} else {
		m_cr1.R1 = m_cr1.R2 = GetRValue(bg);
		m_cr1.G1 = m_cr1.G2 = GetGValue(bg);
		m_cr1.B1 = m_cr1.B2 = GetBValue(bg);
	}

	if (bUseDarkTheme) {
		ThemeRGB(145, 140, 145, m_cr2.R1, m_cr2.G1, m_cr2.B1);
		ThemeRGB(115, 120, 125, m_cr2.R2, m_cr2.G2, m_cr2.B2);
	} else {
		m_cr2.R1 = m_cr2.R2 = GetRValue(light);
		m_cr2.G1 = m_cr2.G2 = GetGValue(light);
		m_cr2.B1 = m_cr2.B2 = GetBValue(light);
	}

	if (bUseDarkTheme) {
		ThemeRGB(15, 20, 25, m_cr3.R1, m_cr3.G1, m_cr3.B1);
		ThemeRGB(55, 60, 65, m_cr3.R2, m_cr3.G2, m_cr3.B2);
	} else {
		m_cr3.R1 = m_cr3.R2 = GetRValue(shadow);
		m_cr3.G1 = m_cr3.G2 = GetGValue(shadow);
		m_cr3.B1 = m_cr3.B2 = GetBValue(shadow);
	}

	if (bUseDarkTheme) {
		ThemeRGB(35, 40, 45, m_cr4.R1, m_cr4.G1, m_cr4.B1);
		ThemeRGB(55, 60, 65, m_cr4.R2, m_cr4.G2, m_cr4.B2);
	} else {
		m_cr4.R1 = m_cr4.R2 = GetRValue(light);
		m_cr4.G1 = m_cr4.G2 = GetGValue(light);
		m_cr4.B1 = m_cr4.B2 = GetBValue(light);
	}

	if (bUseDarkTheme) {
		ThemeRGB(0, 5, 10, m_cr5.R1, m_cr5.G1, m_cr5.B1);
		ThemeRGB(10, 15, 20, m_cr5.R2, m_cr5.G2, m_cr5.B2);
	} else {
		m_cr5.R1 = m_cr5.R2 = GetRValue(shadow);
		m_cr5.G1 = m_cr5.G2 = GetGValue(shadow);
		m_cr5.B1 = m_cr5.B2 = GetBValue(shadow);
	}

	if (bUseDarkTheme) {
		ThemeRGB(145, 150, 155, m_cr6.R1, m_cr6.G1, m_cr6.B1);
		ThemeRGB(45, 50, 55, m_cr6.R2, m_cr6.G2, m_cr6.B2);
	} else {
		m_cr6.R1 = m_cr6.R2 = GetRValue(light);
		m_cr6.G1 = m_cr6.G2 = GetGValue(light);
		m_cr6.B1 = m_cr6.B2 = GetBValue(light);
	}

	if (bUseDarkTheme) {
		ThemeRGB(55, 60, 65, m_cr7.R1, m_cr7.G1, m_cr7.B1);
		ThemeRGB(15, 20, 25, m_cr7.R2, m_cr7.G2, m_cr7.B2);
	} else {
		m_cr7.R1 = m_cr7.R2 = GetRValue(shadow);
		m_cr7.G1 = m_cr7.G2 = GetGValue(shadow);
		m_cr7.B1 = m_cr7.B2 = GetBValue(shadow);
	}

	if (bUseDarkTheme) {
		ThemeRGB(105, 110, 115, m_cr8.R1, m_cr8.G1, m_cr8.B1);
		ThemeRGB(55, 60, 65, m_cr8.R2, m_cr8.G2, m_cr8.B2);
	} else {
		m_cr8.R1 = m_cr8.R2 = GetRValue(light);
		m_cr8.G1 = m_cr8.G2 = GetGValue(light);
		m_cr8.B1 = m_cr8.B2 = GetBValue(light);
	}

	if (bUseDarkTheme) {
		ThemeRGB(65, 70, 75, m_cr9.R1, m_cr9.G1, m_cr9.B1);
		ThemeRGB(5, 10, 15, m_cr9.R2, m_cr9.G2, m_cr9.B2);
	} else {
		m_cr9.R1 = m_cr9.R2 = GetRValue(shadow);
		m_cr9.G1 = m_cr9.G2 = GetGValue(shadow);
		m_cr9.B1 = m_cr9.B2 = GetBValue(shadow);
	}

	m_crText = bUseDarkTheme ? ThemeRGB(255, 255, 255) : 0;
}