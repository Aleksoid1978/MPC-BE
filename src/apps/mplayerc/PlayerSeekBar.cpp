/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
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
	m_ti.lpszText = nullptr;
	m_ti.uId      = (UINT_PTR)m_hWnd;

	m_tooltip.SendMessageW(TTM_ADDTOOLW, 0, (LPARAM)&m_ti);

	ScaleFont();
	SetColor();

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
	if (m_bEnabled != bEnable) {
		m_bEnabled = bEnable;
		Invalidate();
	}
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
			VERIFY(S_OK == m_pMainFrame->m_pTaskbarList->SetProgressValue(m_pMainFrame->m_hWnd, std::max(pos, 1LL), m_stop));
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

	const auto& s = AfxGetAppSettings();

	if (s.bUseDarkTheme && !s.bStatusBarIsVisible && !s.strTimeOnSeekBar.IsEmpty()) {
		m_pos = std::clamp(pos, 0LL, m_stop);
		m_posreal = pos;

		Invalidate();
		return;
	}

	const CRect before = GetThumbRect();
	m_pos = std::clamp(pos, 0LL, m_stop);
	m_posreal = pos;
	const CRect after = GetThumbRect();

	if (before != after) {
		if (s.bUseDarkTheme) {
			Invalidate();
		} else {
			InvalidateRect(before | after);
		}
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

	m_pos_preview = std::clamp(pos, 0LL, m_stop);
}

CRect CPlayerSeekBar::GetChannelRect()
{
	CRect r;
	GetClientRect(&r);

	if (AfxGetAppSettings().bUseDarkTheme) {
		//r.DeflateRect(1,1,1,1);
	} else {
		const int dx  = m_scaleX8;
		const int dy1 = m_scaleY7 + 2;
		const int dy2 = m_scaleY5 + 1;

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
		const int dx = m_scaleY7;
		const int dy = m_scaleY5;

		r.SetRect(x + 1 - dx, r.top - dy, x + dx, r.bottom + dy);
	}

	return r;
}

CRect CPlayerSeekBar::GetInnerThumbRect()
{
	CRect r = GetThumbRect();

	const int dx = m_scaleX4 - 1;
	int dy = m_scaleY5;
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

	const bool bEnabled = m_bEnabled && m_stop > 0;
	const COLORREF repeatAB = COLORREF(RGB(242, 13, 13));
	const CRect channelRect(GetChannelRect());

	if (s.bUseDarkTheme) {
		auto funcMarkChannelTheme = [&](REFERENCE_TIME pos, CDC& memdc, CPen& pen, const CRect& rect) {
			if (pos <= 0 || pos >= m_stop) {
				return;
			}

			const CRect r(channelRect);
			memdc.SelectObject(&pen);
			const int x = r.left + (long)(pos * r.Width() / m_stop);

			// instead of drawing hands can be a marker icon
			// HICON appIcon = (HICON)::LoadImageW(AfxGetResourceHandle(), MAKEINTRESOURCEW(IDR_MARKERS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			// ::DrawIconEx(memdc, x, rc2.top + 10, appIcon, 0, 0, 0, nullptr, DI_NORMAL);
			// ::DestroyIcon(appIcon);

			memdc.MoveTo(x, rect.top + m_scaleY14);
			memdc.LineTo(x, rect.bottom - m_scaleY2);
			memdc.MoveTo(x - m_scaleX1, rect.bottom - m_scaleY2);
			memdc.LineTo(x + m_scaleX1, rect.bottom - m_scaleY2);
		};

		CDC memdc;
		CBitmap bmPaint;
		CRect r;
		GetClientRect(&r);
		memdc.CreateCompatibleDC(&dc);
		bmPaint.CreateCompatibleBitmap(&dc, r.Width(), r.Height());
		CBitmap *bmOld = memdc.SelectObject(&bmPaint);

		GRADIENT_RECT gr = {0, 1};

		if (m_pMainFrame->m_BackGroundGradient.Size()) {
			m_pMainFrame->m_BackGroundGradient.Paint(&memdc, r, 0, s.nThemeBrightness, m_crBackground.R, m_crBackground.G, m_crBackground.B);
		} else {
			tvBackground[0].x = r.left; tvBackground[0].y = r.top;
			tvBackground[1].x = r.right; tvBackground[1].y = r.bottom;
			memdc.GradientFill(tvBackground, 2, &gr, 1, GRADIENT_FILL_RECT_V);
		}

		memdc.SetBkMode(TRANSPARENT);

		CRect rc(channelRect);
		const int nposx = GetThumbRect().right - 2;
		const int nposy = r.top;

		memdc.SelectObject(&m_penPlayed1);
		memdc.MoveTo(rc.left, rc.top);
		memdc.LineTo(rc.right, rc.top);

		memdc.SelectObject(&m_penPlayed2);
		memdc.MoveTo(rc.left - 1, rc.bottom - 1);
		memdc.LineTo(rc.right + 2, rc.bottom - 1);

		// buffer
		m_rLock.SetRect(-1, -1, -1, -1);
		int Progress;
		if (m_pMainFrame->GetBufferingProgress(&Progress)) {
			m_rLock = r;
			const int r_right = r.Width() / 100 * Progress;

			tvBufferingProgress[0].x = r.left; tvBufferingProgress[0].y = r.top;
			tvBufferingProgress[1].x = r_right; tvBufferingProgress[1].y = r.bottom;
			memdc.GradientFill(tvBufferingProgress, 2, &gr, 1, GRADIENT_FILL_RECT_V);

			m_rLock.left = r_right;
		}

		if (bEnabled) {
			if (m_pMainFrame->m_BackGroundGradient.Size()) {
				rc.right = nposx;
				rc.left = rc.left + 1;
				rc.top = rc.top + 1;
				rc.bottom = rc.bottom - 2;

				m_pMainFrame->m_BackGroundGradient.Paint(&memdc, r, 0, s.nThemeBrightness, m_crBackground.R, m_crBackground.G, m_crBackground.B);

				rc = channelRect;
			} else {
				tvBackgroundEnabledLeft[0].x = rc.left; tvBackgroundEnabledLeft[0].y = rc.top;
				tvBackgroundEnabledLeft[1].x = nposx; tvBackgroundEnabledLeft[1].y = rc.bottom - 3;
				memdc.GradientFill(tvBackgroundEnabledLeft, 2, &gr, 1, GRADIENT_FILL_RECT_V);

				CRect rc2;
				rc2.left = nposx - 5;
				rc2.right = nposx;
				rc2.top = rc.top;
				rc2.bottom = rc.bottom;

				tvBackgroundEnabledRight[0].x = rc2.left; tvBackgroundEnabledRight[0].y = rc2.top;
				tvBackgroundEnabledRight[1].x = rc2.right; tvBackgroundEnabledRight[1].y = rc.bottom - 3;
				memdc.GradientFill(tvBackgroundEnabledRight, 2, &gr, 1, GRADIENT_FILL_RECT_V);
			}

			memdc.SelectObject(&m_penPlayed2);
			memdc.MoveTo(rc.left, rc.top);
			memdc.LineTo(nposx, rc.top);

			// draw chapter markers
			if (s.fChapterMarker) {
				CAutoLock lock(&m_CBLock);
				const REFERENCE_TIME stop = m_stop;

				if (stop > 0 && m_pChapterBag && m_pChapterBag->ChapGetCount()) {

					for (DWORD idx = 0; idx < m_pChapterBag->ChapGetCount(); idx++) {
						REFERENCE_TIME rt;
						if (FAILED(m_pChapterBag->ChapGet(idx, &rt, nullptr))) {
							continue;
						}
						funcMarkChannelTheme(rt, memdc, m_penChapters, rc);
					}
				}
			}

			REFERENCE_TIME aPos, bPos;
			bool aEnabled, bEnabled;
			if (m_pMainFrame->CheckABRepeat(aPos, bPos, aEnabled, bEnabled)) {
				if (aEnabled) {
					funcMarkChannelTheme(aPos, memdc, m_penRepeatAB, rc);
				}
				if (bEnabled) {
					funcMarkChannelTheme(bPos, memdc, m_penRepeatAB, rc);
				}
			}
		}

		CString seekbartext = m_pMainFrame->GetTextForBar(s.iSeekBarTextStyle);
		if (seekbartext.GetLength() || !s.bStatusBarIsVisible || !m_strChap.IsEmpty()) {
			memdc.SelectObject(&m_font);
			SetBkMode(memdc, TRANSPARENT);

			LONG xt = s.bStatusBarIsVisible ? 0 : s.strTimeOnSeekBar.GetLength() <= 21 ? 150 : 160;

			if (seekbartext.GetLength() || !m_strChap.IsEmpty()) {
				if (!m_strChap.IsEmpty() && bEnabled) {
					seekbartext = m_strChap;
				}

				// draw filename || chapter name.
				CRect rt = rc;
				rt.left  += 6;
				rt.top   -= 2;
				rt.right -= xt;
				memdc.SetTextColor(m_crText);
				memdc.DrawText(seekbartext, seekbartext.GetLength(), &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

				// Highlighted text
				memdc.SetTextColor(m_crHighlightedText);
				if (nposx > rt.right - 15) {
					memdc.DrawText(seekbartext, seekbartext.GetLength(), &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
				} else {
					rt.right = nposx;
					memdc.DrawText(seekbartext, seekbartext.GetLength(), &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				}
			}

			if (!s.bStatusBarIsVisible) {
				CString str = s.strTimeOnSeekBar;
				CRect rt = rc;
				rt.left  -= xt - 10;
				rt.top   -= 2;
				rt.right -= 6;
				memdc.SetTextColor(m_crTimeText);
				memdc.DrawText(str, str.GetLength(), &rt, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			}
		}

		dc.BitBlt(r.left, r.top, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);
		DeleteObject(memdc.SelectObject(bmOld));
		memdc.DeleteDC();
		bmPaint.DeleteObject();
	} else {
		auto funcMarkChannel = [&](REFERENCE_TIME pos, long verticalPadding, COLORREF markColor) {
			long markPos = channelRect.left + (long)((m_stop > 0) ? channelRect.Width() * pos / m_stop : 0);
			CRect r(markPos, channelRect.top + verticalPadding, markPos + 1, channelRect.bottom - verticalPadding);
			if (r.right < channelRect.right) {
				r.right++;
			}
			ASSERT(r.right <= channelRect.right);
			dc.FillSolidRect(&r, markColor);
			dc.ExcludeClipRect(&r);
		};

		auto CreateThumb = [this](const bool bEnabled, CDC& parentDC) {
			auto& pThumb = bEnabled ? m_pEnabledThumb : m_pDisabledThumb;
			pThumb = std::make_unique<CDC>();
			if (pThumb->CreateCompatibleDC(&parentDC)) {
				const COLORREF white = GetSysColor(COLOR_WINDOW);
				const COLORREF shadow = GetSysColor(COLOR_3DSHADOW);
				const COLORREF light = GetSysColor(COLOR_3DHILIGHT);
				const COLORREF bkg = GetSysColor(COLOR_BTNFACE);

				CBrush b(bkg);

				CRect r(GetThumbRect());
				r.MoveToXY(0, 0);

				CBitmap bmp;
				VERIFY(bmp.CreateCompatibleBitmap(&parentDC, r.Width(), r.Height()));
				VERIFY(pThumb->SelectObject(bmp));

				pThumb->FillRect(&r, &b);

				pThumb->Draw3dRect(&r, light, 0);
				r.DeflateRect(0, 0, 1, 1);
				pThumb->Draw3dRect(&r, light, shadow);
				r.DeflateRect(1, 1, 1, 1);

				pThumb->FrameRect(&r, &b);
				r.DeflateRect(0, 1, 0, 1);
				pThumb->FrameRect(&r, &b);

				r.DeflateRect(1, 1, 0, 0);
				pThumb->Draw3dRect(&r, shadow, bkg);

				if (bEnabled) {
					r.DeflateRect(1, 1, 1, 2);
					CPen whitePen(PS_INSIDEFRAME, 1, white);
					CPen* oldPen = pThumb->SelectObject(&whitePen);
					pThumb->MoveTo(r.left, r.top);
					pThumb->LineTo(r.right, r.top);
					pThumb->MoveTo(r.left, r.bottom);
					pThumb->LineTo(r.right, r.bottom);
					pThumb->SelectObject(oldPen);
					pThumb->SetPixel(r.CenterPoint().x, r.top, 0);
					pThumb->SetPixel(r.CenterPoint().x, r.bottom, 0);
				}
			} else {
				ASSERT(FALSE);
			}
		};

		const COLORREF dark   = GetSysColor(COLOR_GRAYTEXT);
		const COLORREF white  = GetSysColor(COLOR_WINDOW);
		const COLORREF shadow = GetSysColor(COLOR_3DSHADOW);
		const COLORREF light  = GetSysColor(COLOR_3DHILIGHT);
		const COLORREF bkg    = GetSysColor(COLOR_BTNFACE);

		if (!bEnabled) {
			CRect r;
			GetClientRect(&r);
			CBrush b(bkg);
			dc.FillRect(&r, &b);
		}

		// thumb
		auto& pThumb = bEnabled ? m_pEnabledThumb : m_pDisabledThumb;
		if (!pThumb) {
			CreateThumb(bEnabled, dc);
			ASSERT(pThumb);
		}

		CRect r(GetThumbRect());
		CRect ri(GetInnerThumbRect());

		CRgn rgn1, rgn2;
		rgn1.CreateRectRgnIndirect(&r);
		rgn2.CreateRectRgnIndirect(&ri);

		ExtSelectClipRgn(dc, rgn2, RGN_DIFF);
		VERIFY(dc.BitBlt(r.TopLeft().x, r.TopLeft().y, r.Width(), r.Height(), pThumb.get(), 0, 0, SRCCOPY));
		ExtSelectClipRgn(dc, rgn1, RGN_XOR);

		// Chapters
		if (s.fChapterMarker) {
			CAutoLock lock(&m_CBLock);

			const REFERENCE_TIME stop = m_stop;
			if (stop > 0 && m_pChapterBag && m_pChapterBag->ChapGetCount()) {
				for (DWORD i = 0; i < m_pChapterBag->ChapGetCount(); i++) {
					REFERENCE_TIME rtChap;
					if (SUCCEEDED(m_pChapterBag->ChapGet(i, &rtChap, nullptr))) {
						funcMarkChannel(rtChap, 1, dark);
					} else {
						ASSERT(FALSE);
					}
				}
			}
		}

		REFERENCE_TIME aPos, bPos;
		bool aEnabled, bEnabled;
		if (m_pMainFrame->CheckABRepeat(aPos, bPos, aEnabled, bEnabled)) {
			if (aEnabled) {
				funcMarkChannel(aPos, 1, repeatAB);
			}
			if (bEnabled) {
				funcMarkChannel(bPos, 1, repeatAB);
			}
		}

		// channel
		{
			dc.FillSolidRect(&channelRect, bEnabled ? white : bkg);
			CRect r(channelRect);
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
			m_pMainFrame->PostMessageW(WM_HSCROLL, MAKEWPARAM((short)m_pos, SB_THUMBPOSITION), (LPARAM)m_hWnd);
		} else {
			if (!m_pMainFrame->m_bFullScreen && !m_pMainFrame->IsZoomed()) {
				MapWindowPoints(m_pMainFrame, &point, 1);
				m_pMainFrame->PostMessageW(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
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

	if (m_bEnabled && s.bUseDarkTheme && !s.bStatusBarIsVisible) {
		const CRect rc(GetChannelRect());
		CRect rt(rc);
		rt.left  = rc.right - 140;
		rt.right = rc.right - 6;
		CPoint p;
		GetCursorPos(&p);
		ScreenToClient(&p);

		if (rt.PtInRect(p)) {
			s.fRemainingTime = !s.fRemainingTime;
			m_pMainFrame->OnTimer(CMainFrame::TIMER_STREAMPOSPOLLER);
			Invalidate();
		}
	}

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
			m_tooltipTimer = SetTimer(m_tooltipTimer, m_pMainFrame->m_wndPreView.IsWindowVisible() ? 10 : SHOW_DELAY, nullptr);
		}
	} else {
		HideToolTip();
	}

	if (m_tooltipState == TOOLTIP_VISIBLE && m_tooltipPos != m_tooltipLastPos) {
		UpdateToolTipText();

		if (!m_pMainFrame->CanPreviewUse()) {
			UpdateToolTipPosition(point);
		}
		m_tooltipTimer = SetTimer(m_tooltipTimer, m_pMainFrame->CanPreviewUse() ? 10 : AUTOPOP_DELAY, nullptr);
	}
}

void CPlayerSeekBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (AfxGetAppSettings().fUseTimeTooltip) {
		UpdateTooltip(point);
	}

	if (m_CurrentPoint.x == point.x) {
		CDialogBar::OnMouseMove(nFlags, point);
		return;
	}

	if (m_pMainFrame->CanPreviewUse()) {
		UpdateTooltip(point);
	}

	const CWnd* w = GetCapture();
	if (w && w->m_hWnd == m_hWnd && (nFlags & MK_LBUTTON)) {
		const REFERENCE_TIME pos = CalculatePosition(point);
		if (m_pMainFrame->ValidateSeek(pos, m_stop)) {
			MoveThumb(point);

			m_pMainFrame->PostMessageW(WM_HSCROLL, MAKEWPARAM((short)m_pos, SB_THUMBPOSITION), (LPARAM)m_hWnd);

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

void CPlayerSeekBar::ScaleFont()
{
	m_font.DeleteObject();
	m_font.CreateFontW(m_pMainFrame->ScaleY(13), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
					   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
					   L"Tahoma");

	m_scaleX1 = m_pMainFrame->ScaleFloorX(1);
	m_scaleX4 = m_pMainFrame->ScaleFloorX(4);
	m_scaleX8 = m_pMainFrame->ScaleFloorX(8);

	m_scaleY2  = m_pMainFrame->ScaleFloorY(2);
	m_scaleY5  = m_pMainFrame->ScaleFloorY(5);
	m_scaleY7  = m_pMainFrame->ScaleFloorY(7);
	m_scaleY14 = m_pMainFrame->ScaleFloorY(14);

	m_pEnabledThumb.reset();
	m_pDisabledThumb.reset();
}

void CPlayerSeekBar::SetColor()
{
	const auto& s = AfxGetAppSettings();
	if (s.bUseDarkTheme) {
		int R, G, B;

		if (m_pMainFrame->m_BackGroundGradient.Size()) {
			ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, m_crBackground.R, m_crBackground.G, m_crBackground.B);
		} else {
			ThemeRGB(0, 5, 10, R, G, B);
			tvBackground[0] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };
			ThemeRGB(15, 20, 25, R, G, B);
			tvBackground[1] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };
		}

		m_penPlayed1.DeleteObject();
		m_penPlayed1.CreatePen(PS_SOLID, 0, ThemeRGB(30, 35, 40));

		m_penPlayed2.DeleteObject();
		m_penPlayed2.CreatePen(PS_SOLID, 0, ThemeRGB(80, 85, 90));

		m_penChapters.DeleteObject();
		m_penChapters.CreatePen(PS_SOLID, 0, ThemeRGB(255, 255, 255));

		m_penRepeatAB.DeleteObject();
		m_penRepeatAB.CreatePen(PS_SOLID, 0, ThemeRGB(242, 13, 13));

		ThemeRGB(45, 55, 60, R, G, B);
		tvBufferingProgress[0] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };
		ThemeRGB(65, 70, 75, R, G, B);
		tvBufferingProgress[1] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };

		ThemeRGB(0, 5, 10, R, G, B);
		tvBackgroundEnabledLeft[0] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };
		ThemeRGB(105, 110, 115, R, G, B);
		tvBackgroundEnabledLeft[1] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };

		ThemeRGB(0, 5, 10, R, G, B);
		tvBackgroundEnabledRight[0] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };
		ThemeRGB(205, 210, 215, R, G, B);
		tvBackgroundEnabledRight[1] = { 0, 0, COLOR16(R * 256), COLOR16(G * 256), COLOR16(B * 256), 255 * 256 };

		m_crText = ThemeRGB(135, 140, 145);
		m_crHighlightedText = ThemeRGB(205, 210, 215);
		m_crTimeText = ThemeRGB(200, 205, 210);
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
					m_tooltipTimer = SetTimer(m_tooltipTimer, m_pMainFrame->CanPreviewUse() ? 10 : AUTOPOP_DELAY, nullptr);
					m_tooltipPos = CalculatePosition(point);
					UpdateToolTipText();

					if (!m_pMainFrame->CanPreviewUse()) {
						m_tooltip.SendMessageW(TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_ti);
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
		m_tooltip.SendMessageW(TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);
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
		GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

		const auto channelRect_y = GetChannelRect().TopLeft().y;
		point.x -= r_width / 2 - 2;
		point.y = channelRect_y - (r_height + 13);
		ClientToScreen(&point);
		point.x = std::max(mi.rcWork.left + 5, std::min(point.x, mi.rcWork.right - r_width - 5));

		if (point.y <= 5) {
			const CRect r = mi.rcWork;
			if (!r.PtInRect(point)) {
				CPoint p(point);
				p.y = channelRect_y + 30;
				ClientToScreen(&p);

				point.y = p.y;
			}
		}

		m_pMainFrame->m_wndPreView.SetWindowPos(nullptr, point.x, point.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
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

		point.x = std::max(0L, std::min(point.x, r.Width() - size.cx));
		ClientToScreen(&point);

		m_tooltip.SendMessageW(TTM_TRACKPOSITION, 0, MAKELPARAM(point.x, point.y));
	}

	m_tooltipLastPos = m_tooltipPos;
}

void CPlayerSeekBar::UpdateToolTipText()
{
	GUID timeFormat = m_pMainFrame->GetTimeFormat();
	CString tooltipText;
	if (timeFormat == TIME_FORMAT_MEDIA_TIME) {
		tooltipText = ReftimeToString2(m_tooltipPos);
	} else if (timeFormat == TIME_FORMAT_FRAME) {
		tooltipText.Format(L"%I64d", m_tooltipPos);
	} else {
		ASSERT(FALSE);
	}

	if (!m_pMainFrame->CanPreviewUse()) {
		m_ti.lpszText = (LPWSTR)(LPCWSTR)tooltipText;
		m_tooltip.SendMessageW(TTM_SETTOOLINFOW, 0, (LPARAM)&m_ti);
	} else {
		m_pMainFrame->m_wndPreView.SetWindowTextW(tooltipText);
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
	bool needShow = m_pMainFrame->TogglePreview();

	if (m_pMainFrame->m_wndPreView.IsWindowVisible()) {
		if (!needShow) {
			m_pMainFrame->PreviewWindowHide();
			OnMouseMove(nFlags, point);
		}
	}
	else {
		if (needShow) {
			PreviewWindowShow();
		}
	}
}

void CPlayerSeekBar::SetChapterBag(CComPtr<IDSMChapterBag>& pCB)
{
	CAutoLock lock(&m_CBLock);

	if (pCB) {
		m_pChapterBag = pCB;
	}
}

void CPlayerSeekBar::RemoveChapters()
{
	CAutoLock lock(&m_CBLock);
	m_pChapterBag.Release();
}
