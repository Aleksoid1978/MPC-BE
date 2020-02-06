/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include "PlayerStatusBar.h"
#include "OpenImage.h"
#include "../../DSUtil/SysVersion.h"

// CPlayerStatusBar

IMPLEMENT_DYNAMIC(CPlayerStatusBar, CDialogBar)

CPlayerStatusBar::CPlayerStatusBar(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
	, m_status(false, false)
	, m_time(true, false)
	, m_bmid(0)
	, m_time_rect(-1, -1, -1, -1)
	, m_time_rect2(-1, -1, -1, -1)
{
	m_font.m_hObject = nullptr;
}

CPlayerStatusBar::~CPlayerStatusBar()
{
}

BOOL CPlayerStatusBar::Create(CWnd* pParentWnd)
{
	if (!__super::Create(pParentWnd, IDD_PLAYERSTATUSBAR, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_BOTTOM, IDD_PLAYERSTATUSBAR)) {
		return FALSE;
	}

	ScaleFontInternal();

	return TRUE;
}

BOOL CPlayerStatusBar::PreCreateWindow(CREATESTRUCT& cs)
{
	VERIFY(CDialogBar::PreCreateWindow(cs));

	m_dwStyle &= ~CBRS_BORDER_TOP;
	m_dwStyle &= ~CBRS_BORDER_BOTTOM;

	return TRUE;
}

CSize CPlayerStatusBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CSize ret = __super::CalcFixedLayout(bStretch, bHorz);
	ret.cy = m_pMainFrame->ScaleSystemToMonitorY(ret.cy);
	return ret;
}

void CPlayerStatusBar::ScaleFont()
{
	m_status.ScaleFont();
	m_time.ScaleFont();

	ScaleFontInternal();
}

void CPlayerStatusBar::ScaleFontInternal()
{
	m_font.DeleteObject();

	m_font.CreateFontW(m_pMainFrame->ScaleY(13), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
					  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
					  L"Tahoma");
}

int CPlayerStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogBar::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	if (m_BackGroundbm.FileExists(L"background")) {
		m_BackGroundbm.LoadExternalGradient(L"background");
	}

	CRect r;
	r.SetRectEmpty();

	m_type.Create(L"", WS_CHILD | WS_VISIBLE | SS_ICON, r, this, IDC_STATIC1);
	m_status.Create(L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, r, this, IDC_PLAYERSTATUS);
	m_status.SetWindowPos(&m_time, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	m_time.Create(L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, r, this, IDC_PLAYERTIME);

	Relayout();

	return 0;
}

void CPlayerStatusBar::Relayout()
{
	CAppSettings& s = AfxGetAppSettings();

	if (!s.bUseDarkTheme) {
		m_type.ShowWindow(/*SW_SHOW*/SW_HIDE);
		m_status.ShowWindow(SW_SHOW);
		m_time.ShowWindow(SW_SHOW);

		BITMAP bm;
		memset(&bm, 0, sizeof(bm));
		if (m_bm.m_hObject) {
			m_bm.GetBitmap(&bm);
		}

		CRect r, r2;
		GetClientRect(r);

		r.DeflateRect(/*2*/7, 5, bm.bmWidth + 8, 4);
		int div = r.right - (m_time.IsWindowVisible() ? 140 : 0);

		CString str;
		m_time.GetWindowTextW(str);
		if (CDC* pDC = m_time.GetDC()) {
			CFont* pOld = pDC->SelectObject(&m_time.GetFont());
			div = r.right - pDC->GetTextExtent(str).cx;
			pDC->SelectObject(pOld);
			m_time.ReleaseDC(pDC);
		}

		r2 = r;
		r2.right = div - 2;
		m_status.MoveWindow(&r2);

		r2 = r;
		r2.left = div;
		m_time.MoveWindow(&r2);
		m_time_rect = r2;

		/*
		GetClientRect(r);
		r.SetRect(6, r.top+4, 22, r.bottom-4);
		m_type.MoveWindow(r);
		*/
	} else {
		m_type.ShowWindow(SW_HIDE);
		m_status.ShowWindow(SW_HIDE);
		m_time.ShowWindow(SW_HIDE);
		s.strTimeOnSeekBar = GetStatusTimer();
	}
}

void CPlayerStatusBar::Clear()
{
	m_status.SetWindowTextW(L"");
	m_time.SetWindowTextW(L"");

	SetStatusBitmap(0);

	Relayout();

	Invalidate();
}

void CPlayerStatusBar::SetStatusBitmap(UINT id)
{
	if (m_bmid == id) {
		return;
	}

	if (m_bm.m_hObject) {
		m_bm.DeleteObject();
	}

	if (id) {
		CImage img;
		img.LoadFromResource(AfxGetInstanceHandle(), id);
		m_bm.Attach(img.Detach());
	}

	m_bmid = id;

	Relayout();

	Invalidate();
}

void CPlayerStatusBar::SetStatusMessage(CString str)
{
	str.Trim();
	str.Replace(L"&", L"&&");
	m_status.SetWindowTextW(str);

	Relayout();

	Invalidate();
}

CString CPlayerStatusBar::GetStatusTimer()
{
	CString strResult;
	m_time.GetWindowTextW(strResult);

	return strResult;
}

CString CPlayerStatusBar::GetStatusMessage()
{
	CString strResult;
	m_status.GetWindowTextW(strResult);

	return strResult;
}

void CPlayerStatusBar::SetStatusTimer(CString str)
{
	if (GetStatusTimer() != str) {
		m_time.SetRedraw(FALSE);
		str.Trim();
		m_time.SetWindowTextW(str);
		m_time.SetRedraw(TRUE);

		Relayout();
		Invalidate();
	}
}

void CPlayerStatusBar::SetStatusTimer(REFERENCE_TIME rtNow, REFERENCE_TIME rtDur, bool fHighPrecision, const GUID& timeFormat/* = TIME_FORMAT_MEDIA_TIME*/)
{
	CString str;
	CString posstr, durstr, rstr;

	if (timeFormat == TIME_FORMAT_MEDIA_TIME) {
		DVD_HMSF_TIMECODE tcNow, tcDur, tcRt;

		if (fHighPrecision) {
			tcNow = RT2HMSF(rtNow);
			tcDur = RT2HMSF(rtDur);
			tcRt  = RT2HMSF(rtDur - rtNow);
		} else {
			tcNow = RT2HMS_r(rtNow);
			tcDur = RT2HMS_r(rtDur);
			tcRt  = RT2HMS_r(rtDur - rtNow);
		}

#if 0
		if (tcDur.bHours > 0 || (rtNow >= rtDur && tcNow.bHours > 0)) {
			posstr.Format(L"%02u:%02u:%02u", tcNow.bHours, tcNow.bMinutes, tcNow.bSeconds);
			rstr.Format(L"%02u:%02u:%02u", tcRt.bHours, tcRt.bMinutes, tcRt.bSeconds);
		} else {
			posstr.Format(L"%02u:%02u", tcNow.bMinutes, tcNow.bSeconds);
			rstr.Format(L"%02u:%02u", tcRt.bMinutes, tcRt.bSeconds);
		}

		if (tcDur.bHours > 0) {
			durstr.Format(L"%02u:%02u:%02u", tcDur.bHours, tcDur.bMinutes, tcDur.bSeconds);
		} else {
			durstr.Format(L"%02u:%02u", tcDur.bMinutes, tcDur.bSeconds);
		}
#else
		posstr.Format(L"%02u:%02u:%02u", tcNow.bHours, tcNow.bMinutes, tcNow.bSeconds);
		rstr.Format(L"%02u:%02u:%02u", tcRt.bHours, tcRt.bMinutes, tcRt.bSeconds);
		durstr.Format(L"%02u:%02u:%02u", tcDur.bHours, tcDur.bMinutes, tcDur.bSeconds);
#endif

		if (fHighPrecision) {
			posstr.AppendFormat(L".%03d", (rtNow / 10000) % 1000);
			durstr.AppendFormat(L".%03d", (rtDur / 10000) % 1000);
			rstr.AppendFormat(L".%03d", ((rtDur - rtNow) / 10000) % 1000);
		}
	} else if (timeFormat == TIME_FORMAT_FRAME) {
		posstr.Format(L"%I64d", rtNow);
		durstr.Format(L"%I64d", rtDur);
		rstr.Format(L"%I64d", rtDur - rtNow);
	}

	if (!AfxGetAppSettings().fRemainingTime) {
		str = ((rtDur <= 0) || (rtDur < rtNow)) ? posstr : posstr + L" / " + durstr;
	} else {
		str = ((rtDur <= 0) || (rtDur < rtNow)) ? posstr : L"- " + rstr + L" / " + durstr;
	}

	SetStatusTimer(str);
}

void CPlayerStatusBar::ShowTimer(bool fShow)
{
	m_time.ShowWindow(fShow ? SW_SHOW : SW_HIDE);

	Relayout();

	Invalidate();
}

BEGIN_MESSAGE_MAP(CPlayerStatusBar, CDialogBar)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETCURSOR()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

// CPlayerStatusBar message handlers

BOOL CPlayerStatusBar::OnEraseBkgnd(CDC* pDC)
{
	CAppSettings& s = AfxGetAppSettings();
	CRect r;

	if (!s.bUseDarkTheme) {
		for (CWnd* pChild = GetWindow(GW_CHILD); pChild; pChild = pChild->GetNextWindow()) {
			if (!pChild->IsWindowVisible()) {
				continue;
			}

			pChild->GetClientRect(&r);
			pChild->MapWindowPoints(this, &r);
			pDC->ExcludeClipRect(&r);
		}

		GetClientRect(&r);

		if (m_pMainFrame->m_pLastBar != this || m_pMainFrame->m_bFullScreen) {
			r.InflateRect(0, 0, 0, 1);
		}

		if (m_pMainFrame->m_bFullScreen) {
			r.InflateRect(1, 0, 1, 0);
		}

		pDC->Draw3dRect(&r, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));
		r.DeflateRect(1, 1);
		pDC->FillSolidRect(&r, 0);
	}

	return TRUE;
}

void CPlayerStatusBar::OnPaint()
{
	CPaintDC dc(this);

	if (GetSafeHwnd()) {
		Relayout();
	}

	CAppSettings& s = AfxGetAppSettings();

	if (!s.bUseDarkTheme) {
		if (m_bm.m_hObject) {
			BITMAP bm;
			m_bm.GetBitmap(&bm);
			CDC memdc;
			memdc.CreateCompatibleDC(&dc);
			memdc.SelectObject(&m_bm);

			CRect r;
			GetClientRect(&r);
			dc.BitBlt(r.right - bm.bmWidth - 1, (r.Height() - bm.bmHeight) / 2, bm.bmWidth, bm.bmHeight, &memdc, 0, 0, SRCCOPY);
		}
	} else {
		CRect r;
		GetClientRect(&r);

		CDC memdc;
		memdc.CreateCompatibleDC(&dc);

		CBitmap m_bmPaint;
		m_bmPaint.CreateCompatibleBitmap(&dc, r.Width(), r.Height());
		memdc.SelectObject(&m_bmPaint);

		// background
		int R, G, B;

		if (m_BackGroundbm.IsExtGradiendLoading()) {
			ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, R, G, B);
			m_BackGroundbm.PaintExternalGradient(&dc, r, 55, s.nThemeBrightness, R, G, B);
		} else {
			int R2, G2, B2;
			ThemeRGB(30, 35, 40, R, G, B);
			ThemeRGB(0, 5, 10, R2, G2, B2);
			GRADIENT_RECT gr = {0, 1};
			TRIVERTEX tv[2] = {
				{r.left, r.top, R * 256, G * 256, B * 256, 255 * 256},
				{r.right, r.bottom, R2 * 256, G2 * 256, B2 * 256, 255 * 256},
			};
			memdc.GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);
		}

		memdc.SetBkMode(TRANSPARENT);

		CPen penPlayed1(PS_SOLID, 0, COLORREF(0));
		memdc.SelectObject(&penPlayed1);
		memdc.MoveTo(r.left, r.top + 1);
		memdc.LineTo(r.right, r.top + 1);
		memdc.MoveTo(r.left, r.top + 2);
		memdc.LineTo(r.right, r.top + 2);

		CPen penPlayed2(PS_SOLID, 0, ThemeRGB(50, 55, 60));
		memdc.SelectObject(&penPlayed2);
		memdc.MoveTo(r.left, r.top + 3);
		memdc.LineTo(r.right, r.top + 3);

		memdc.SetTextColor(ThemeRGB(165, 170, 175));

		// texts
		memdc.SelectObject(&m_font);

		CString str = GetStatusTimer();
		int strlen = str.GetLength();

		s.strTimeOnSeekBar = str;

		const LONG xOffset = 6;
		const LONG yOffset = 3;

		m_time_rect2.SetRectEmpty();
		if (strlen > 0) {
			const LONG textWidth = memdc.GetTextExtent(str).cx;

			CRect rt = r;
			m_time_rect2        = rt;
			m_time_rect2.right  = rt.right;
			m_time_rect2.top    = rt.top;
			m_time_rect2.left   = rt.right - textWidth;
			m_time_rect2.bottom = rt.bottom;

			rt.right -= xOffset;
			rt.top   += yOffset;

			memdc.DrawText(str, strlen, &rt, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		}

		str = GetStatusMessage();
		strlen = str.GetLength();

		if (strlen > 0) {
			CRect rt = r;
			rt.left += xOffset;
			rt.top  += yOffset;

			memdc.DrawText(str, strlen, &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		}

		dc.BitBlt(r.left, r.top, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
	}
}

void CPlayerStatusBar::OnSize(UINT nType, int cx, int cy)
{
	CDialogBar::OnSize(nType, cx, cy);

	Relayout();

	Invalidate();
}

void CPlayerStatusBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	CAppSettings& s = AfxGetAppSettings();

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	m_pMainFrame->GetWindowPlacement(&wp);

	if (m_time_rect.PtInRect(point) || m_time_rect2.PtInRect(point)) {
		s.fRemainingTime = !s.fRemainingTime;
		m_pMainFrame->OnTimer(2);
		return;
	}

	if (!m_pMainFrame->m_bFullScreen && wp.showCmd != SW_SHOWMAXIMIZED) {
		CRect r;
		GetClientRect(r);
		CPoint p = point;

		MapWindowPoints(m_pMainFrame, &point, 1);

		m_pMainFrame->PostMessageW(WM_NCLBUTTONDOWN,
							(p.x >= r.Width()-r.Height() && !m_pMainFrame->IsCaptionHidden()) ? HTBOTTOMRIGHT :
							HTCAPTION,
							MAKELPARAM(point.x, point.y));
	}
}

BOOL CPlayerStatusBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	m_pMainFrame->GetWindowPlacement(&wp);

	CPoint p;
	GetCursorPos(&p);
	ScreenToClient(&p);

	if (m_time_rect.PtInRect(p) || m_time_rect2.PtInRect(p)) {
		SetCursor(LoadCursorW(nullptr, IDC_HAND));
		return TRUE;
	}

	if (!m_pMainFrame->m_bFullScreen && wp.showCmd != SW_SHOWMAXIMIZED) {
		CRect r;
		GetClientRect(r);
		if (p.x >= r.Width()-r.Height() && !m_pMainFrame->IsCaptionHidden()) {
			SetCursor(LoadCursorW(nullptr, IDC_SIZENWSE));
			return TRUE;
		}
	}

	return CDialogBar::OnSetCursor(pWnd, nHitTest, message);
}

HBRUSH CPlayerStatusBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogBar::OnCtlColor(pDC, pWnd, nCtlColor);

	if (!AfxGetAppSettings().bUseDarkTheme) {
		if (*pWnd == m_type) {
			hbr = GetStockBrush(BLACK_BRUSH);
		}
	}

	return hbr;
}
