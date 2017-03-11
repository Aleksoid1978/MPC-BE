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
#include "MainFrm.h"
#include "PlayerSeekBar.h"

#define SHOW_DELAY    200
#define AUTOPOP_DELAY 1000

// CPlayerSeekBar

IMPLEMENT_DYNAMIC(CPlayerSeekBar, CDialogBar)

CPlayerSeekBar::CPlayerSeekBar(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
{
}

CPlayerSeekBar::~CPlayerSeekBar()
{
}

BOOL CPlayerSeekBar::Create(CWnd* pParentWnd)
{
	VERIFY(CDialogBar::Create(pParentWnd, IDD_PLAYERSEEKBAR, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_BOTTOM, IDD_PLAYERSEEKBAR));

	ModifyStyleEx(WS_EX_LAYOUTRTL, WS_EX_NOINHERITLAYOUT);

	m_tooltip.Create(this, TTS_NOPREFIX | TTS_ALWAYSTIP);

	m_tooltip.SetMaxTipWidth(SHRT_MAX);

	m_tooltip.SetDelayTime(TTDT_AUTOPOP, SHRT_MAX);
	m_tooltip.SetDelayTime(TTDT_INITIAL, 0);
	m_tooltip.SetDelayTime(TTDT_RESHOW, 0);

	ZeroMemory(&m_ti, sizeof(TOOLINFO));
	m_ti.cbSize   = sizeof(TOOLINFO);
	m_ti.uFlags   = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	m_ti.hwnd     = m_hWnd;
	m_ti.hinst    = AfxGetInstanceHandle();
	m_ti.lpszText = NULL;
	m_ti.uId      = (UINT)m_hWnd;

	m_tooltip.SendMessage(TTM_ADDTOOL, 0, (LPARAM)&m_ti);

	if (m_BackGroundbm.FileExists(CString(L"background"))) {
		m_BackGroundbm.LoadExternalGradient(L"background");
	}

	return TRUE;
}

BOOL CPlayerSeekBar::PreCreateWindow(CREATESTRUCT& cs)
{
	VERIFY(CDialogBar::PreCreateWindow(cs));

	m_dwStyle &= ~CBRS_BORDER_TOP;
	m_dwStyle &= ~CBRS_BORDER_BOTTOM;
	m_dwStyle |= CBRS_SIZE_FIXED;

	return TRUE;
}

CSize CPlayerSeekBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CSize ret = __super::CalcFixedLayout(bStretch, bHorz);
	ret.cy = m_pMainFrame->ScaleSystemToMonitorY(ret.cy);
	return ret;
}

void CPlayerSeekBar::Enable(bool bEnable)
{
	m_bEnabled = bEnable;

	Invalidate();
}

void CPlayerSeekBar::GetRange(REFERENCE_TIME& stop)
{
	stop = m_stop;
}

void CPlayerSeekBar::SetRange(const REFERENCE_TIME stop)
{
	if (stop > 0) {
		m_stop = stop;
	} else {
		m_stop = 0;
	}

	if (m_pos < 0 || m_pos >= m_stop) {
		SetPos(0);
	}
}

void CPlayerSeekBar::SetPos(const REFERENCE_TIME pos)
{
	const CWnd* w = GetCapture();

	if (w && w->m_hWnd == m_hWnd) {
		return;
	}

	SetPosInternal(pos);

	if (HasDuration() && AfxGetAppSettings().fUseWin7TaskBar) {
		if (m_pMainFrame->m_pTaskbarList) {
			VERIFY(S_OK == m_pMainFrame->m_pTaskbarList->SetProgressValue(m_pMainFrame->m_hWnd, max(pos, 1ll), m_stop));
		}
	}
}

REFERENCE_TIME CPlayerSeekBar::CalculatePosition(const CPoint point)
{
	REFERENCE_TIME pos = -1;
	const CRect r = GetChannelRect();

	if (point.x < r.left) {
		pos = 0;
	} else if (point.x >= r.right) {
		pos = m_stop;
	} else if (m_stop > 0) {
		const LONG w = r.right - r.left;
		pos = (m_stop * (point.x - r.left) + (w / 2)) / w;
	}

	return pos;
}

void CPlayerSeekBar::MoveThumb(const CPoint point)
{
	REFERENCE_TIME pos = CalculatePosition(point);

	if (pos >= 0) {
		if (AfxGetAppSettings().fFastSeek ^ (GetKeyState(VK_SHIFT) < 0)) {
			pos = m_pMainFrame->GetClosestKeyFrame(pos);
		}

		SetPosInternal(pos);
	}
}

void CPlayerSeekBar::SetPosInternal(const REFERENCE_TIME pos)
{
	if (m_pos == pos) {
		return;
	}

	const CRect before = GetThumbRect();
	m_pos = clamp(pos, 0LL, m_stop);
	m_posreal = pos;
	const CRect after = GetThumbRect();

	if (before != after && !AfxGetAppSettings().bUseDarkTheme) {
		InvalidateRect(before | after);
	}
}

void CPlayerSeekBar::MoveThumbPreview(const CPoint point)
{
	REFERENCE_TIME pos = CalculatePosition(point);

	if (pos >= 0) {
		if (AfxGetAppSettings().fFastSeek ^ (GetKeyState(VK_SHIFT) < 0)) {
			pos = m_pMainFrame->GetClosestKeyFrame(pos);
		}

		SetPosInternalPreview(pos);
	}
}

void CPlayerSeekBar::SetPosInternalPreview(const REFERENCE_TIME pos)
{
	if (m_pos_preview == pos) {
		return;
	}

	m_pos_preview = clamp(pos, 0LL, m_stop);
}

CRect CPlayerSeekBar::GetChannelRect()
{
	CRect r;
	GetClientRect(&r);

	if (AfxGetAppSettings().bUseDarkTheme) {
		//r.DeflateRect(1,1,1,1);
	} else {
		const int dx  = m_pMainFrame->ScaleFloorX(8);
		const int dy1 = m_pMainFrame->ScaleFloorY(7) + 2;
		const int dy2 = m_pMainFrame->ScaleFloorY(5) + 1;

		r.DeflateRect(dx, dy1, dx, dy2);
	}

	return r;
}

CRect CPlayerSeekBar::GetThumbRect()
{
	CRect r = GetChannelRect();

	const int x = r.left + (int)((m_stop > 0) ? (REFERENCE_TIME)r.Width() * m_pos / m_stop : 0);
	const int y = r.CenterPoint().y;

	if (AfxGetAppSettings().bUseDarkTheme) {
		r.SetRect(x, y - 2, x + 3, y + 3);
	} else {
		const int dx = m_pMainFrame->ScaleFloorY(7);
		const int dy = m_pMainFrame->ScaleFloorY(5);

		r.SetRect(x + 1 - dx, r.top - dy, x + dx, r.bottom + dy);
	}

	return r;
}

CRect CPlayerSeekBar::GetInnerThumbRect()
{
	CRect r = GetThumbRect();

	const int dx = m_pMainFrame->ScaleFloorX(4) - 1;
	int dy = m_pMainFrame->ScaleFloorY(5);
	if (!m_bEnabled || m_stop <= 0) {
		dy -= 1;
	}

	r.DeflateRect(dx, dy, dx, dy);

	return r;
}

BEGIN_MESSAGE_MAP(CPlayerSeekBar, CDialogBar)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
	ON_WM_TIMER()
	ON_WM_RBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_COMMAND_EX(ID_PLAY_STOP, OnPlayStop)
END_MESSAGE_MAP()

// CPlayerSeekBar message handlers

void CPlayerSeekBar::OnPaint()
{
	CPaintDC dc(this);

	const CAppSettings& s = AfxGetAppSettings();

	int R, G, B, R2, G2, B2;

	const bool bEnabled = m_bEnabled && m_stop > 0;

	if (s.bUseDarkTheme) {
		CString str = m_pMainFrame->GetStrForTitle();
		CDC memdc;
		CBitmap m_bmPaint;
		CRect r;
		GetClientRect(&r);
		memdc.CreateCompatibleDC(&dc);
		m_bmPaint.CreateCompatibleBitmap(&dc, r.Width(), r.Height());
		CBitmap *bmOld = memdc.SelectObject(&m_bmPaint);

		GRADIENT_RECT gr[1] = {{0, 1}};
		int pa = 255 * 256;

		if (m_BackGroundbm.IsExtGradiendLoading()) {
			ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, R, G, B);
			m_BackGroundbm.PaintExternalGradient(&memdc, r, 0, s.nThemeBrightness, R, G, B);
		} else {
			ThemeRGB(0, 5, 10, R, G, B);
			ThemeRGB(15, 20, 25, R2, G2, B2);
			TRIVERTEX tv[2] = {
				{r.left, r.top, R * 256, G * 256, B * 256, pa},
				{r.right, r.bottom, R2 * 256, G2 * 256, B2 * 256, pa},
			};
			memdc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
		}

		memdc.SetBkMode(TRANSPARENT);

		CPen penPlayed(s.clrFaceABGR == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, s.clrFaceABGR);
		CPen penPlayedOutline(s.clrOutlineABGR == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, s.clrOutlineABGR);

		CRect rc = GetChannelRect();
		const int nposx = GetThumbRect().right - 2;
		const int nposy = r.top;

		ThemeRGB(30, 35, 40, R, G, B);
		CPen penPlayed1(PS_SOLID, 0, RGB(R,G,B));
		memdc.SelectObject(&penPlayed1);
		memdc.MoveTo(rc.left, rc.top);
		memdc.LineTo(rc.right, rc.top);

		ThemeRGB(80, 85, 90, R, G, B);
		CPen penPlayed2(PS_SOLID, 0, RGB(R,G,B));
		memdc.SelectObject(&penPlayed2);
		memdc.MoveTo(rc.left - 1, rc.bottom - 1);
		memdc.LineTo(rc.right + 2, rc.bottom - 1);

		// buffer
		m_rLock.SetRect(-1, -1, -1, -1);
		int Progress;
		if (m_pMainFrame->GetBufferingProgress(&Progress)) {
			m_rLock = r;
			const int r_right = r.Width() / 100 * Progress;
			ThemeRGB(45, 55, 60, R, G, B);
				ThemeRGB(65, 70, 75, R2, G2, B2);
				TRIVERTEX tvb[2] = {
					{r.left, r.top, R * 256, G * 256, B * 256, pa},
					{r_right, r.bottom - 3, R2 * 256, G2 * 256, B2 * 256, pa},
				};
				memdc.GradientFill(tvb, 2, gr, 1, GRADIENT_FILL_RECT_V);
				m_rLock.left = r_right;
		}

		if (bEnabled) {
			if (m_BackGroundbm.IsExtGradiendLoading()) {
				rc.right = nposx;
				rc.left = rc.left + 1;
				rc.top = rc.top + 1;
				rc.bottom = rc.bottom - 2;

				ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, R, G, B);
				m_BackGroundbm.PaintExternalGradient(&memdc, r, 0, s.nThemeBrightness, R, G, B);

				rc = GetChannelRect();
			} else {
				ThemeRGB(0, 5, 10, R, G, B);
				ThemeRGB(105, 110, 115, R2, G2, B2);
				TRIVERTEX tv[2] = {
					{rc.left, rc.top, R * 256, G * 256, B * 256, pa},
					{nposx, rc.bottom - 3, R2 * 256, G2 * 256, B2 * 256, pa},
				};
				memdc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);

				CRect rc2;
				rc2.left = nposx - 5;
				rc2.right = nposx;
				rc2.top = rc.top;
				rc2.bottom = rc.bottom;

				ThemeRGB(0, 5, 10, R, G, B);
				ThemeRGB(205, 210, 215, R2, G2, B2);
				TRIVERTEX tv2[2] = {
					{rc2.left, rc2.top, R * 256, G * 256, B * 256, pa},
					{rc2.right, rc.bottom - 3, R2 * 256, G2 * 256, B2 * 256, pa},
				};
				memdc.GradientFill(tv2, 2, gr, 1, GRADIENT_FILL_RECT_V);
			}

			ThemeRGB(80, 85, 90, R, G, B);
			CPen penPlayed3(PS_SOLID, 0, RGB(R,G,B));
			memdc.SelectObject(&penPlayed3);
			memdc.MoveTo(rc.left, rc.top);//active_top
			memdc.LineTo(nposx, rc.top);

			// draw chapter markers
			if (s.fChapterMarker) {
				CAutoLock lock(&m_CBLock);

				if (m_pChapterBag && m_pChapterBag->ChapGetCount()) {
					const CRect rc2 = rc;
					for (DWORD idx = 0; idx < m_pChapterBag->ChapGetCount(); idx++) {
						const CRect r = GetChannelRect();
						REFERENCE_TIME rt;

						if (FAILED(m_pChapterBag->ChapGet(idx, &rt, NULL))) {
							continue;
						}

						if (rt <= 0 || (rt >= m_stop)) {
							continue;
						}

						const int x = r.left + (int)((m_stop > 0) ? (REFERENCE_TIME)r.Width() * rt / m_stop : 0);

						// instead of drawing hands can be a marker icon
						// HICON appIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MARKERS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
						// ::DrawIconEx(memdc, x, rc2.top + 10, appIcon, 0,0, 0, NULL, DI_NORMAL);
						// ::DestroyIcon(appIcon);

						ThemeRGB(255, 255, 255, R, G, B);
						CPen penPlayed2(PS_SOLID, 0, RGB(R,G,B));
						memdc.SelectObject(&penPlayed2);

						memdc.MoveTo(x, rc2.top + 14);
						memdc.LineTo(x, rc2.bottom - 2);
						memdc.MoveTo(x - 1, rc2.bottom - 2);
						memdc.LineTo(x + 2, rc2.bottom - 2);
					}
				}
			}
		}

		if (s.fFileNameOnSeekBar || !s.bStatusBarIsVisible || !m_strChap.IsEmpty()) {
			ThemeRGB(135, 140, 145, R, G, B);
			memdc.SetTextColor(RGB(R,G,B));

			CFont font;
			font.CreateFont(m_pMainFrame->ScaleY(13), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
							OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");

			CFont* pOldFont = memdc.SelectObject(&font);
			SetBkMode(memdc, TRANSPARENT);

			LONG xt = s.bStatusBarIsVisible ? 0 : s.strTimeOnSeekBar.GetLength() <= 21 ? 150 : 160;

			if (s.fFileNameOnSeekBar || !m_strChap.IsEmpty()) {
				if (!m_strChap.IsEmpty() && bEnabled) {
					str = m_strChap;
				}

				// draw filename || chapter name.
				CRect rt = rc;
				rt.left  += 6;
				rt.top   -= 2;
				rt.right -= xt;
				memdc.DrawText(str, str.GetLength(), &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

				// highlights string
				ThemeRGB(205, 210, 215, R, G, B);
				memdc.SetTextColor(RGB(R,G,B));
				if (nposx > rt.right - 15) {
					memdc.DrawText(str, str.GetLength(), &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
				} else {
					rt.right = nposx;
					memdc.DrawText(str, str.GetLength(), &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				}
			}

			if (!s.bStatusBarIsVisible) {
				str = s.strTimeOnSeekBar;
				CRect rt = rc;
				rt.left  -= xt - 10;
				rt.top   -= 2;
				rt.right -= 6;
				ThemeRGB(200, 205, 210, R, G, B);
				memdc.SetTextColor(RGB(R,G,B));
				memdc.DrawText(str, str.GetLength(), &rt, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			}
		}

		dc.BitBlt(r.left, r.top, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
		DeleteObject(memdc.SelectObject(bmOld));
		memdc.DeleteDC();
		m_bmPaint.DeleteObject();
	} else {
		const COLORREF white  = GetSysColor(COLOR_WINDOW);
		const COLORREF shadow = GetSysColor(COLOR_3DSHADOW);
		const COLORREF light  = GetSysColor(COLOR_3DHILIGHT);
		const COLORREF bkg    = GetSysColor(COLOR_BTNFACE);

		// thumb
		{
			CRect r(GetThumbRect());
			CRect ri(GetInnerThumbRect());
			CRect rt = r, rit = ri;

			dc.Draw3dRect(&r, light, 0);
			r.DeflateRect(0, 0, 1, 1);
			dc.Draw3dRect(&r, light, shadow);
			r.DeflateRect(1, 1, 1, 1);

			CBrush b(bkg);

			dc.FrameRect(&r, &b);
			r.DeflateRect(0, 1, 0, 1);
			dc.FrameRect(&r, &b);

			r.DeflateRect(1, 1, 0, 0);
			dc.Draw3dRect(&r, shadow, bkg);

			if (bEnabled) {
				r.DeflateRect(1, 1, 1, 2);
				CPen white(PS_INSIDEFRAME, 1, white);
				CPen* old = dc.SelectObject(&white);
				dc.MoveTo(r.left, r.top);
				dc.LineTo(r.right, r.top);
				dc.MoveTo(r.left, r.bottom);
				dc.LineTo(r.right, r.bottom);
				dc.SelectObject(old);
				dc.SetPixel(r.CenterPoint().x, r.top, 0);
				dc.SetPixel(r.CenterPoint().x, r.bottom, 0);
			}

			//dc.SetPixel(r.CenterPoint().x + 5, r.top - 4, bkg); //?

			{
				CRgn rgn1, rgn2;
				rgn1.CreateRectRgnIndirect(&rt);
				rgn2.CreateRectRgnIndirect(&rit);
				ExtSelectClipRgn(dc, rgn1, RGN_DIFF);
				ExtSelectClipRgn(dc, rgn2, RGN_OR);
			}
		}

		// channel
		{
			CRect r = GetChannelRect();

			dc.FillSolidRect(&r, bEnabled ? white : bkg);
			r.InflateRect(1, 1);
			dc.Draw3dRect(&r, shadow, light);
			dc.ExcludeClipRect(&r);
		}

		// background
		{
			CRect r;
			GetClientRect(&r);
			CBrush b(bkg);
			dc.FillRect(&r, &b);
		}
	}
}

void CPlayerSeekBar::OnSize(UINT nType, int cx, int cy)
{
	HideToolTip();

	CDialogBar::OnSize(nType, cx, cy);

	Invalidate();
}

void CPlayerSeekBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	const REFERENCE_TIME pos = CalculatePosition(point);
	if (m_pMainFrame->ValidateSeek(pos, m_stop)) {
		if (m_bEnabled && (GetChannelRect() | GetThumbRect()).PtInRect(point)) {
			SetCapture();
			MoveThumb(point);
			m_pMainFrame->PostMessage(WM_HSCROLL, MAKEWPARAM((short)m_pos, SB_THUMBPOSITION), (LPARAM)m_hWnd);
		} else {
			if (!m_pMainFrame->m_bFullScreen) {
				MapWindowPoints(m_pMainFrame, &point, 1);
				m_pMainFrame->PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
			}
		}
	}

	CDialogBar::OnLButtonDown(nFlags, point);
}

void CPlayerSeekBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	CDialogBar::OnLButtonUp(nFlags, point);
}

void CPlayerSeekBar::OnRButtonDown(UINT nFlags, CPoint point)
{
	CAppSettings& s = AfxGetAppSettings();

	if (!s.bStatusBarIsVisible) {
		const CRect rc(GetChannelRect());
		CRect rt(rc);
		rt.left  = rc.right - 140;
		rt.right = rc.right - 6;
		CPoint p;
		GetCursorPos(&p);
		ScreenToClient(&p);

		if (rt.PtInRect(p)) {
			s.fRemainingTime = !s.fRemainingTime;
		}
	}

	m_pMainFrame->PostMessage(WM_COMMAND, ID_PLAY_GOTO);

	CDialogBar::OnRButtonDown(nFlags, point);
}


void CPlayerSeekBar::UpdateTooltip(CPoint point)
{
	m_tooltipPos = CalculatePosition(point);
	if (m_bEnabled && m_stop > 0 && (GetChannelRect() | GetThumbRect()).PtInRect(point)) {
		if (m_tooltipState == TOOLTIP_HIDDEN && m_tooltipPos != m_tooltipLastPos) {

			TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
			tme.hwndTrack = m_hWnd;
			tme.dwFlags   = TME_LEAVE;
			TrackMouseEvent(&tme);

			m_tooltipState = TOOLTIP_TRIGGERED;
			m_tooltipTimer = SetTimer(m_tooltipTimer, m_pMainFrame->m_wndPreView.IsWindowVisible() ? 10 : SHOW_DELAY, NULL);
		}
	} else {
		HideToolTip();
	}

	if (m_tooltipState == TOOLTIP_VISIBLE && m_tooltipPos != m_tooltipLastPos) {
		UpdateToolTipText();

		if (!m_pMainFrame->CanPreviewUse()) {
			UpdateToolTipPosition(point);
		}
		m_tooltipTimer = SetTimer(m_tooltipTimer, m_pMainFrame->CanPreviewUse() ? 10 : AUTOPOP_DELAY, NULL);
	}
}

void CPlayerSeekBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_CurrentPoint.x == point.x) {
		CDialogBar::OnMouseMove(nFlags, point);
		return;
	}

	if (AfxGetAppSettings().fUseTimeTooltip || m_pMainFrame->CanPreviewUse()) {
		UpdateTooltip(point);
	}

	const CWnd* w = GetCapture();
	if (w && w->m_hWnd == m_hWnd && (nFlags & MK_LBUTTON)) {
		const REFERENCE_TIME pos = CalculatePosition(point);
		if (m_pMainFrame->ValidateSeek(pos, m_stop)) {
			MoveThumb(point);

			m_pMainFrame->PostMessage(WM_HSCROLL, MAKEWPARAM((short)m_pos, SB_THUMBPOSITION), (LPARAM)m_hWnd);

			Invalidate();
			UpdateWindow();
		}
	}

	m_CurrentPoint = point;

	const OAFilterState fs = m_pMainFrame->GetMediaState();
	if (fs != -1) {
		if (m_pMainFrame->CanPreviewUse()) {
			MoveThumbPreview(point);
			UpdateToolTipPosition(point);
		}
	} else {
		m_pMainFrame->PreviewWindowHide();
	}

	CDialogBar::OnMouseMove(nFlags, point);
}

void CPlayerSeekBar::OnMouseLeave()
{
	HideToolTip();
	m_pMainFrame->PreviewWindowHide();
	m_strChap.Empty();
	Invalidate();
}

BOOL CPlayerSeekBar::PreTranslateMessage(MSG* pMsg)
{
	POINT ptWnd(pMsg->pt);
	this->ScreenToClient(&ptWnd);

	if (m_bEnabled && AfxGetAppSettings().fUseTimeTooltip && m_stop > 0 && (GetChannelRect() | GetThumbRect()).PtInRect(ptWnd)) {
		m_tooltip.RelayEvent(pMsg);
	}

	return CDialogBar::PreTranslateMessage(pMsg);
}

BOOL CPlayerSeekBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint p;
	GetCursorPos(&p);
	ScreenToClient(&p);
	if (m_rLock.PtInRect(p)) {
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
		return TRUE;
	}


	if (m_bEnabled && m_stop > 0 && m_stop != 100) {
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CPlayerSeekBar::OnPlayStop(UINT nID)
{
	SetPos(0);
	Invalidate();

	return FALSE;
}

void CPlayerSeekBar::PreviewWindowShow()
{
	if (m_pMainFrame->CanPreviewUse()) {
		if (!m_pMainFrame->m_wndPreView.IsWindowVisible()) {
			CPoint point;
			GetCursorPos(&point);
			ScreenToClient(&point);
			MoveThumbPreview(point);
		}
		m_pMainFrame->PreviewWindowShow(m_pos_preview);
	}
}

void CPlayerSeekBar::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_tooltipTimer) {
		switch (m_tooltipState) {
			case TOOLTIP_TRIGGERED:
			{
				CPoint point;

				GetCursorPos(&point);
				ScreenToClient(&point);

				if (m_bEnabled && m_stop > 0 && (GetChannelRect() | GetThumbRect()).PtInRect(point)) {
					m_tooltipTimer = SetTimer(m_tooltipTimer, m_pMainFrame->CanPreviewUse() ? 10 : AUTOPOP_DELAY, NULL);
					m_tooltipPos = CalculatePosition(point);
					UpdateToolTipText();

					if (!m_pMainFrame->CanPreviewUse()) {
						m_tooltip.SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_ti);
					}

					m_tooltipState = TOOLTIP_VISIBLE;
				}
			}
			break;
			case TOOLTIP_VISIBLE:
			{
				HideToolTip();
				PreviewWindowShow();
			}
			break;
		}
	}

	CWnd::OnTimer(nIDEvent);
}

void CPlayerSeekBar::HideToolTip()
{
	if (m_tooltipState > TOOLTIP_HIDDEN) {
		KillTimer(m_tooltipTimer);
		m_tooltip.SendMessage(TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);
		m_tooltipState = TOOLTIP_HIDDEN;
	}
}

void CPlayerSeekBar::UpdateToolTipPosition(CPoint point)
{
	if (m_pMainFrame->CanPreviewUse()) {
		if (!m_pMainFrame->m_wndPreView.IsWindowVisible()) {
			m_pMainFrame->m_wndPreView.SetWindowSize();
		}

		CRect rc;
		m_pMainFrame->m_wndPreView.GetWindowRect(&rc);
		const int r_width  = rc.Width();
		const int r_height = rc.Height();

		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

		point.x -= r_width / 2 - 2;
		point.y = GetChannelRect().TopLeft().y - (r_height + 13);
		ClientToScreen(&point);
		point.x = max(mi.rcWork.left + 5, min(point.x, mi.rcWork.right - r_width - 5));

		if (point.y <= 5) {
			const CRect r = mi.rcWork;
			if (!r.PtInRect(point)) {
				CPoint p(point);
				p.y = GetChannelRect().TopLeft().y + 30;
				ClientToScreen(&p);

				point.y = p.y;
			}
		}

		m_pMainFrame->m_wndPreView.SetWindowPos(NULL, point.x, point.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	} else {
		CSize size = m_tooltip.GetBubbleSize(&m_ti);
		CRect r;
		GetWindowRect(r);

		if (AfxGetAppSettings().nTimeTooltipPosition == TIME_TOOLTIP_ABOVE_SEEKBAR) {
			point.x -= size.cx / 2 - 2;
			point.y = GetChannelRect().TopLeft().y - (size.cy + 13);
		} else {
			point.x += 10;
			point.y += 20;
		}

		point.x = max(0, min(point.x, r.Width() - size.cx));
		ClientToScreen(&point);

		m_tooltip.SendMessage(TTM_TRACKPOSITION, 0, MAKELPARAM(point.x, point.y));
	}

	m_tooltipLastPos = m_tooltipPos;
}

void CPlayerSeekBar::UpdateToolTipText()
{
	GUID timeFormat = m_pMainFrame->GetTimeFormat();
	CString tooltipText;
	if (timeFormat == TIME_FORMAT_MEDIA_TIME) {
		DVD_HMSF_TIMECODE tcNow = RT2HMS_r(m_tooltipPos);
		tooltipText.Format(L"%02u:%02u:%02u", tcNow.bHours, tcNow.bMinutes, tcNow.bSeconds);
	} else if (timeFormat == TIME_FORMAT_FRAME) {
		tooltipText.Format(L"%I64d", m_tooltipPos);
	} else {
		ASSERT(FALSE);
	}

	if (!m_pMainFrame->CanPreviewUse()) {
		m_ti.lpszText = (LPTSTR)(LPCTSTR)tooltipText;
		m_tooltip.SendMessage(TTM_SETTOOLINFO, 0, (LPARAM)&m_ti);
	} else {
		m_pMainFrame->m_wndPreView.SetWindowText(tooltipText);
	}

	{
		CAutoLock lock(&m_CBLock);

		m_strChap.Empty();
		if (AfxGetAppSettings().bUseDarkTheme
				&& AfxGetAppSettings().fChapterMarker
				&& m_pChapterBag && m_pChapterBag->ChapGetCount()) {

			CComBSTR chapterName;
			REFERENCE_TIME rt = m_tooltipPos;
			m_pChapterBag->ChapLookup(&rt, &chapterName);

			if (chapterName.Length() > 0) {
				m_strChap.Format(L"%s%s", ResStr(IDS_AG_CHAPTER2), chapterName);
				Invalidate();
			}
		}
	}
}

void CPlayerSeekBar::OnMButtonDown(UINT nFlags, CPoint point)
{
	if (m_pMainFrame->m_wndPreView.IsWindowVisible()) {
		m_pMainFrame->PreviewWindowHide();
		AfxGetAppSettings().fSmartSeek = !AfxGetAppSettings().fSmartSeek;
		OnMouseMove(nFlags,point);
	}
}

void CPlayerSeekBar::SetChapterBag(CComPtr<IDSMChapterBag>& pCB)
{
	CAutoLock lock(&m_CBLock);

	if (pCB) {
		m_pChapterBag.Release();
		pCB.CopyTo(&m_pChapterBag);
	}
}
