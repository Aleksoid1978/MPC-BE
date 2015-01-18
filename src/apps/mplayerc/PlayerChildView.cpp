/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include "PlayerChildView.h"
#include "OpenImage.h"

// CChildView

CChildView::CChildView() : m_vrect(0,0,0,0)
{
	m_lastlmdowntime = 0;
	m_lastlmdownpoint.SetPoint(0, 0);

	LoadLogo();
}

CChildView::~CChildView()
{
}

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), HBRUSH(COLOR_WINDOW+1), NULL);

	return TRUE;
}

BOOL CChildView::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MYMOUSELAST) {
		CWnd* pParent = GetParent();
		CPoint p(pMsg->lParam);
		::MapWindowPoints(pMsg->hwnd, pParent->m_hWnd, &p, 1);

		bool fDblClick = false;

		bool fInteractiveVideo = AfxGetMainFrame()->IsInteractiveVideo();
		/*
					if (fInteractiveVideo)
					{
						if (pMsg->message == WM_LBUTTONDOWN)
						{
							if ((pMsg->time - m_lastlmdowntime) <= GetDoubleClickTime()
							&& abs(pMsg->pt.x - m_lastlmdownpoint.x) <= GetSystemMetrics(SM_CXDOUBLECLK)
							&& abs(pMsg->pt.y - m_lastlmdownpoint.y) <= GetSystemMetrics(SM_CYDOUBLECLK))
							{
								fDblClick = true;
								m_lastlmdowntime = 0;
								m_lastlmdownpoint.SetPoint(0, 0);
							}
							else
							{
								m_lastlmdowntime = pMsg->time;
								m_lastlmdownpoint = pMsg->pt;
							}
						}
						else if (pMsg->message == WM_LBUTTONDBLCLK)
						{
							m_lastlmdowntime = pMsg->time;
							m_lastlmdownpoint = pMsg->pt;
						}
					}
		*/
		if ((pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP || pMsg->message == WM_MOUSEMOVE)
				&& fInteractiveVideo) {
			if (pMsg->message == WM_MOUSEMOVE) {
				pParent->PostMessage(pMsg->message, pMsg->wParam, MAKELPARAM(p.x, p.y));
			}

			if (fDblClick) {
				pParent->PostMessage(WM_LBUTTONDOWN, pMsg->wParam, MAKELPARAM(p.x, p.y));
				pParent->PostMessage(WM_LBUTTONDBLCLK, pMsg->wParam, MAKELPARAM(p.x, p.y));
			}
		} else {
			pParent->PostMessage(pMsg->message, pMsg->wParam, MAKELPARAM(p.x, p.y));
			return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CChildView::SetVideoRect(CRect r)
{
	m_vrect = r;

	Invalidate();
}

void CChildView::LoadLogo()
{
	AppSettings& s = AfxGetAppSettings();
	bool bHaveLogo = false;

	CAutoLock cAutoLock(&m_csLogo);

	m_logo.Destroy();

	CString logoFName = _T("logo");

	if (m_logo.FileExists(logoFName, true)) {
		m_logo.Attach(OpenImage(logoFName));
	} else {
		if (s.fLogoExternal) {
			m_logo.Attach(OpenImage(s.strLogoFileName));
			if (m_logo) {
				bHaveLogo = true;
			}
		}

		if (!bHaveLogo) {
			s.fLogoExternal = false;
			s.strLogoFileName = "";

			if (!m_logo.LoadFromResource(s.nLogoId)) {
				m_logo.LoadFromResource(s.nLogoId = DEF_LOGO);
			}
		}
	}

	if (m_hWnd) {
		Invalidate();
	}
}

CSize CChildView::GetLogoSize() const
{
	CSize ret(0,0);

	if (!m_logo.IsNull()) {
		ret.SetSize(m_logo.GetWidth(), m_logo.GetHeight());
	}

	return ret;
}

IMPLEMENT_DYNAMIC(CChildView, CWnd)

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	//{{AFX_MSG_MAP(CChildView)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
	//	ON_WM_NCHITTEST()
	ON_WM_NCHITTEST()
	ON_WM_NCLBUTTONDOWN()
END_MESSAGE_MAP()

// CChildView message handlers

void CChildView::OnPaint()
{
	CPaintDC dc(this);

	if (auto pFrame = AfxGetMainFrame()) {
		pFrame->RepaintVideo();
	}
}

BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	CAutoLock cAutoLock(&m_csLogo);

	CRect r;
	GetClientRect(r);

	BITMAP bm;
	int x, y, w, h, oldmode;

	COLORREF bkcolor = 0;

	BLENDFUNCTION bf;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 0xFF;

	auto pFrame = AfxGetMainFrame();
	if (pFrame->IsSomethingLoaded()) {

		if (GetObject(pFrame->m_InternalImage, sizeof(bm), &bm)) {

			h = min(abs(bm.bmHeight), r.Height());
			w = min(r.Width(), MulDiv(h, bm.bmWidth, abs(bm.bmHeight)));
			h = MulDiv(w, abs(bm.bmHeight), bm.bmWidth);
			x = (r.Width() - w) / 2;
			y = (r.Height() - h) / 2;
			r = CRect(CPoint(x, y), CSize(w, h));

			oldmode = pDC->SetStretchBltMode(STRETCH_HALFTONE);
			pFrame->m_InternalImage.StretchBlt(*pDC, r, CRect(0, 0, bm.bmWidth, abs(bm.bmHeight)));
			pDC->SetStretchBltMode(oldmode);
			pDC->AlphaBlend(x, y, w, h, pDC, x, y, w, h, bf);

			pDC->ExcludeClipRect(r);
		} else {
			pDC->ExcludeClipRect(m_vrect);
		}
		UNREFERENCED_PARAMETER(bm);

	} else if (!m_logo.IsNull() /*&& (pFrame->IsPlaylistEmpty()*/) {

		if (GetObject(m_logo, sizeof(bm), &bm)) {

			h = min(abs(bm.bmHeight), r.Height());
			w = min(r.Width(), MulDiv(h, bm.bmWidth, abs(bm.bmHeight)));
			h = MulDiv(w, abs(bm.bmHeight), bm.bmWidth);
			x = (r.Width() - w) / 2;
			y = (r.Height() - h) / 2;
			r = CRect(CPoint(x, y), CSize(w, h));

			bkcolor = m_logo.GetPixel(0,0);

			oldmode = pDC->SetStretchBltMode(STRETCH_HALFTONE);
			m_logo.StretchBlt(*pDC, r, CRect(0, 0, bm.bmWidth, abs(bm.bmHeight)));
			pDC->SetStretchBltMode(oldmode);
			pDC->AlphaBlend(x, y, w, h, pDC, x, y, w, h, bf);

			pDC->ExcludeClipRect(r);
		}
	}

	GetClientRect(r);
	pDC->FillSolidRect(r, bkcolor);

	return TRUE;
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (auto pFrame = AfxGetMainFrame()) {
		pFrame->MoveVideoWindow();
		pFrame->UpdateThumbnailClip();
	}
}

BOOL CChildView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	auto pFrame = AfxGetMainFrame();

	if (pFrame->m_fHideCursor) {
		SetCursor(NULL);
		return TRUE;
	}

	if (pFrame->IsSomethingLoaded() && (nHitTest == HTCLIENT)) {
		if (pFrame->GetPlaybackMode() == PM_DVD) {
			return FALSE;
		}

		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));

		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

LRESULT CChildView::OnNcHitTest(CPoint point)
{
	UINT nHitTest = CWnd::OnNcHitTest(point);

	auto pFrame = AfxGetMainFrame();

	WINDOWPLACEMENT wp;
	pFrame->GetWindowPlacement(&wp);

	if (!pFrame->m_fFullScreen && wp.showCmd != SW_SHOWMAXIMIZED && AfxGetAppSettings().iCaptionMenuMode == MODE_BORDERLESS) {
		CRect rcClient, rcFrame;
		GetWindowRect(&rcFrame);
		rcClient = rcFrame;

		CSize sizeBorder(GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER));

		rcClient.InflateRect(-(5 * sizeBorder.cx), -(5 * sizeBorder.cy));
		rcFrame.InflateRect(sizeBorder.cx, sizeBorder.cy);

		if (rcFrame.PtInRect(point)) {
			if (point.x > rcClient.right) {
				if (point.y < rcClient.top) {
					nHitTest = HTTOPRIGHT;
				} else if (point.y > rcClient.bottom) {
					nHitTest = HTBOTTOMRIGHT;
				} else {
					nHitTest = HTRIGHT;
				}
			} else if (point.x < rcClient.left) {
				if (point.y < rcClient.top) {
					nHitTest = HTTOPLEFT;
				} else if (point.y > rcClient.bottom) {
					nHitTest = HTBOTTOMLEFT;
				} else {
					nHitTest = HTLEFT;
				}
			} else if (point.y < rcClient.top) {
				nHitTest = HTTOP;
			} else if (point.y > rcClient.bottom) {
				nHitTest = HTBOTTOM;
			}
		}
	}

	return nHitTest;
}

void CChildView::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	auto pFrame = AfxGetMainFrame();
	bool fLeftMouseBtnUnassigned = !AssignedToCmd(wmcmd::LDOWN);

	if (!pFrame->m_fFullScreen && (pFrame->IsCaptionHidden() || fLeftMouseBtnUnassigned)) {
		BYTE bFlag = 0;
		switch (nHitTest) {
			case HTTOP:
				bFlag = WMSZ_TOP;
				break;
			case HTTOPLEFT:
				bFlag = WMSZ_TOPLEFT;
				break;
			case HTTOPRIGHT:
				bFlag = WMSZ_TOPRIGHT;
				break;
			case HTLEFT:
				bFlag = WMSZ_LEFT;
				break;
			case HTRIGHT:
				bFlag = WMSZ_RIGHT;
				break;
			case HTBOTTOM:
				bFlag = WMSZ_BOTTOM;
				break;
			case HTBOTTOMLEFT:
				bFlag = WMSZ_BOTTOMLEFT;
				break;
			case HTBOTTOMRIGHT:
				bFlag = WMSZ_BOTTOMRIGHT;
				break;
		}

		if (bFlag) {
			pFrame->PostMessage(WM_SYSCOMMAND, (SC_SIZE | bFlag), (LPARAM)POINTTOPOINTS(point));
		} else {
			CWnd::OnNcLButtonDown(nHitTest, point);
		}
	}
}
