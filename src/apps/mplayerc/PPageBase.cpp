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
#include "PPageBase.h"
#include "controls/DarkTheme.h"

// CPPageBase dialog

IMPLEMENT_DYNAMIC(CPPageBase, CCmdUIPropertyPage)
CPPageBase::CPPageBase(UINT nIDTemplate, UINT nIDCaption)
	: CCmdUIPropertyPage(nIDTemplate, nIDCaption)
{
}

CPPageBase::~CPPageBase()
{
}

void CPPageBase::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}

void CPPageBase::CreateToolTip()
{
	m_wndToolTip.Create(this);
	m_wndToolTip.Activate(TRUE);
	m_wndToolTip.SetMaxTipWidth(350);
	m_wndToolTip.SetDelayTime(TTDT_AUTOPOP, 10000);

	for (CWnd* pChild = GetWindow(GW_CHILD); pChild; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		CString strToolTip;

		if (strToolTip.LoadString(pChild->GetDlgCtrlID())) {
			m_wndToolTip.AddTool(pChild, strToolTip);
		}
	}
}

BOOL CPPageBase::PreTranslateMessage(MSG* pMsg)
{
	if (IsWindow(m_wndToolTip))
		if (pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST) {
			MSG msg;
			memcpy(&msg, pMsg, sizeof(MSG));

			for (HWND hWndParent = ::GetParent(msg.hwnd);
					hWndParent && hWndParent != m_hWnd;
					hWndParent = ::GetParent(hWndParent)) {
				msg.hwnd = hWndParent;
			}

			if (msg.hwnd) {
				m_wndToolTip.RelayEvent(&msg);
			}
		}

	return __super::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CPPageBase, CCmdUIPropertyPage)
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()

// CPPageBase message handlers

BOOL CPPageBase::OnSetActive()
{
	AfxGetAppSettings().nLastUsedPage = (UINT)(ULONG_PTR)m_pPSP->pszTemplate;

	BOOL bRet = __super::OnSetActive();

	// Re-apply the dark visual style on every activation. It is idempotent (controls
	// already themed are skipped), and doing it each time avoids a race on the page
	// shown first, whose controls may not have been ready the very first time.
	DarkTheme::ApplyThemeToChildren(GetSafeHwnd());
	// Keep group boxes behind the controls they frame so their dark fill never paints over
	// them (which made labels/combos vanish until invalidated, e.g. after Apply).
	DarkTheme::FixGroupBoxes(GetSafeHwnd());

	return bRet;
}

HBRUSH CPPageBase::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = __super::OnCtlColor(pDC, pWnd, nCtlColor);

	if (HBRUSH hbrDark = DarkTheme::OnCtlColor(pDC, nCtlColor)) {
		return hbrDark;
	}

	return hbr;
}

BOOL CPPageBase::OnEraseBkgnd(CDC* pDC)
{
	if (DarkTheme::IsActive()) {
		CRect rc;
		GetClientRect(rc);
		pDC->FillSolidRect(rc, DarkTheme::FaceColor());
		return TRUE;
	}

	return __super::OnEraseBkgnd(pDC);
}

// The horizontal separator lines in the option pages are owner-drawn statics
// (SS_OWNERDRAW) so Windows does not paint its light 3D etched line. We draw them
// here: a flat line in the shared border colour when the dark theme is active, or
// the classic etched edge otherwise.
void CPPageBase::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct && lpDrawItemStruct->CtlType == ODT_STATIC) {
		CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
		CRect rc(lpDrawItemStruct->rcItem);
		if (DarkTheme::IsActive()) {
			pDC->FillSolidRect(rc, DarkTheme::FaceColor());
			pDC->FillSolidRect(rc.left, rc.top + rc.Height() / 2, rc.Width(), 1, DarkTheme::CtrlBorderColor());
		} else {
			pDC->FillSolidRect(rc, GetSysColor(COLOR_3DFACE));
			::DrawEdge(lpDrawItemStruct->hDC, &rc, EDGE_ETCHED, BF_TOP);
		}
		return;
	}

	__super::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

BOOL CPPageBase::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// For NM_CUSTOMDRAW, let any page-specific handler run first (e.g. colour-picker
	// buttons that paint themselves with the selected colour, or list controls). Only
	// when the page does not handle it do we apply the generic dark theming for the
	// controls native dark mode leaves light: trackbar (slider) channels, checkbox/
	// radio-button captions and push buttons. (Group boxes are handled by subclassing,
	// since they emit no NM_CUSTOMDRAW.)
	NMHDR* pNMHDR = reinterpret_cast<NMHDR*>(lParam);
	if (pNMHDR && pNMHDR->code == NM_CUSTOMDRAW) {
		if (__super::OnNotify(wParam, lParam, pResult)) {
			return TRUE;
		}
		if (DarkTheme::TrackbarCustomDraw(pNMHDR, pResult)) {
			return TRUE;
		}
		if (DarkTheme::ButtonCustomDraw(pNMHDR, pResult)) {
			return TRUE;
		}
		return FALSE;
	}

	return __super::OnNotify(wParam, lParam, pResult);
}

void CPPageBase::OnDestroy()
{
	__super::OnDestroy();

	m_wndToolTip.DestroyWindow();
}

BOOL CPPageBase::OnApply()
{
	if (auto pMainFrame = AfxFindMainFrame()) { // need dynamic_cast, because CPPageFormats can be run in a separate process without CMainFrame.
		pMainFrame->PostMessageW(WM_SAVESETTINGS);
	}

	return __super::OnApply();
}

int CPPageBase::ScaleY(int y)
{
	return AfxGetMainFrame()->ScaleY(y);
}
