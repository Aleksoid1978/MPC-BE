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
#include "MainFrm.h"
#include "FullscreenWnd.h"

// CFullscreenWnd

IMPLEMENT_DYNAMIC(CFullscreenWnd, CWnd)

CFullscreenWnd::CFullscreenWnd(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
	, m_hCursor(::LoadCursor(NULL, IDC_ARROW))
	, m_bCursorVisible(false)
	, m_bTrackingMouseLeave(false)
{
}

CFullscreenWnd::~CFullscreenWnd()
{
}

BEGIN_MESSAGE_MAP(CFullscreenWnd, CWnd)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

LRESULT CFullscreenWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_COMMAND :
			m_pMainFrame->PostMessage(message, wParam, lParam);
			break;
		case WM_NCACTIVATE :
			return 0;
	}

	return CWnd::WindowProc(message, wParam, lParam);
}

BOOL CFullscreenWnd::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
		case WM_SYSKEYDOWN :
		case WM_SYSKEYUP :
		case WM_SYSCHAR :
		case WM_SYSCOMMAND :

		case WM_KEYDOWN :
		case WM_KEYUP :
		case WM_CHAR :

		case WM_LBUTTONDBLCLK :
		case WM_MBUTTONDOWN :
		case WM_MBUTTONUP :
		case WM_MBUTTONDBLCLK :
		case WM_RBUTTONDOWN :
		case WM_RBUTTONUP :
		case WM_RBUTTONDBLCLK :
		case WM_XBUTTONDOWN :

		case WM_MOUSEWHEEL :

			m_pMainFrame->PostMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
			return TRUE;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

BOOL CFullscreenWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_NOCLOSE,
									   m_hCursor, HBRUSH(COLOR_WINDOW + 1));

	return TRUE;
}

// CFullscreenWnd message handlers

void CFullscreenWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_pMainFrame) {
		if (!(m_pMainFrame->IsD3DFullScreenMode() && m_pMainFrame->m_OSD.OnLButtonDown(nFlags, point))) {
			m_pMainFrame->PostMessage(WM_LBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
		}
		return;
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CFullscreenWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_pMainFrame) {
		if (!(m_pMainFrame->IsD3DFullScreenMode() && m_pMainFrame->m_OSD.OnLButtonUp(nFlags, point))) {
			m_pMainFrame->PostMessage(WM_RBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
		}
		return;
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CFullscreenWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bTrackingMouseLeave) {
		TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, this->m_hWnd };
		if (TrackMouseEvent(&tme)) {
			m_bTrackingMouseLeave = true;
		}
	}

	if (m_pMainFrame) {
		if (m_pMainFrame->IsD3DFullScreenMode() && m_pMainFrame->m_OSD.OnMouseMove(nFlags, point)) {
			m_pMainFrame->KillTimer(CMainFrame::TIMER_FULLSCREENMOUSEHIDER);
		} else {
			m_pMainFrame->PostMessage(WM_MOUSEMOVE, nFlags, MAKELPARAM(point.x, point.y));
		}
		return;
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CFullscreenWnd::OnMouseLeave()
{
	m_bTrackingMouseLeave = false;

	if (m_pMainFrame) {
		m_pMainFrame->m_OSD.OnMouseLeave();
		return;
	}

	CWnd::OnMouseLeave();
}

BOOL CFullscreenWnd::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

BOOL CFullscreenWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bCursorVisible) {
		::SetCursor(m_hCursor);
	} else {
		::SetCursor(NULL);
	}

	return FALSE;
}

void CFullscreenWnd::ShowCursor(bool bVisible)
{
	if (m_bCursorVisible != bVisible) {
		m_bCursorVisible = bVisible;
		PostMessage(WM_SETCURSOR, 0, 0);
	}
}

void CFullscreenWnd::SetCursor(LPCWSTR lpCursorName)
{
	m_hCursor = ::LoadCursor(NULL, lpCursorName);
	m_bCursorVisible = true;
	PostMessage(WM_SETCURSOR, 0, 0);
}

bool CFullscreenWnd::IsWindow() const
{
	return !!m_hWnd;
}
