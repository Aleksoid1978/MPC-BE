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
#include "PlayerVolumeCtrl.h"

// CVolumeCtrl

IMPLEMENT_DYNAMIC(CVolumeCtrl, CSliderCtrl)

CVolumeCtrl::CVolumeCtrl(bool bSelfDrawn/* = true*/)
	: m_bSelfDrawn(bSelfDrawn)
{
}

CVolumeCtrl::~CVolumeCtrl()
{
}

bool CVolumeCtrl::Create(CWnd* pParentWnd)
{
	VERIFY(CSliderCtrl::Create(WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_HORZ | TBS_TOOLTIPS, CRect(0,0,0,0), pParentWnd, IDC_SLIDER1));

	AppSettings& s = AfxGetAppSettings();

	if (m_BackGroundbm.FileExists(CString(L"background"))) {
		m_BackGroundbm.LoadExternalGradient(L"background");
	}
	if (m_Volumebm.FileExists(CString(L"volume"))) {
		m_Volumebm.LoadExternalGradient(L"volume");
	}

	EnableToolTips(TRUE);
	SetRange(0, 100);
	SetPos(s.nVolume);
	SetPageSize(s.nVolumeStep);
	SetLineSize(0);

	m_nUseDarkTheme		= s.bUseDarkTheme + 1;

	m_nThemeBrightness	= s.nThemeBrightness;
	m_nThemeRed			= s.nThemeRed;
	m_nThemeGreen		= s.nThemeGreen;
	m_nThemeBlue		= s.nThemeBlue;

	return TRUE;
}

void CVolumeCtrl::SetPosInternal(int pos)
{
	SetPos(pos);

	GetParent()->PostMessage(WM_HSCROLL, MAKEWPARAM((short)pos, SB_THUMBPOSITION), (LPARAM)m_hWnd);
}

void CVolumeCtrl::IncreaseVolume()
{
	// align volume up to step. recommend using steps 1, 2, 5 and 10
	SetPosInternal(GetPos() + GetPageSize() - GetPos() % GetPageSize());
}

void CVolumeCtrl::DecreaseVolume()
{
	// align volume down to step. recommend using steps 1, 2, 5 and 10
	int m = GetPos() % GetPageSize();
	SetPosInternal(GetPos() - (m ? m : GetPageSize()));
}

BEGIN_MESSAGE_MAP(CVolumeCtrl, CSliderCtrl)
	ON_WM_ERASEBKGND()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_WM_LBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_HSCROLL_REFLECT()
	ON_WM_SETCURSOR()
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
END_MESSAGE_MAP()

// CVolumeCtrl message handlers

BOOL CVolumeCtrl::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CVolumeCtrl::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	LRESULT lr = CDRF_DODEFAULT;
	AppSettings& s = AfxGetAppSettings();

	int R, G, B, R2, G2, B2;

	GRADIENT_RECT gr[1] = {{0, 1}};

	if (m_bSelfDrawn) {
		switch (pNMCD->dwDrawStage) {
			case CDDS_PREPAINT:
				if (s.bUseDarkTheme && (m_bmUnderCtrl.GetSafeHandle() == NULL
						|| m_nUseDarkTheme == 1
						|| m_nThemeBrightness != s.nThemeBrightness
						|| m_nThemeRed != s.nThemeRed
						|| m_nThemeGreen != s.nThemeGreen
						|| m_nThemeBlue != s.nThemeBlue)) {
					CDC dc;
					dc.Attach(pNMCD->hdc);
					CRect r;
					GetClientRect(&r);
					InvalidateRect(&r);
					CDC memdc;

					int nHeight = ((CMainFrame*)AfxGetMainWnd())->m_wndToolBar.m_nButtonHeight;
					int nBMedian = nHeight - 3 - 0.5 * nHeight - 8;
					int height = r.Height() + nBMedian + 4;

					if (m_BackGroundbm.IsExtGradiendLoading()) {
						ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, R, G, B);
						m_BackGroundbm.PaintExternalGradient(&dc, r, 22, s.nThemeBrightness, R, G, B);
					} else {
						ThemeRGB(50, 55, 60, R, G, B);
						ThemeRGB(20, 25, 30, R2, G2, B2);
						TRIVERTEX tv[2] = {
							{r.left, r.top - nBMedian, R*256, G*256, B*256, 255*256},
							{r.Width(), height, R2*256, G2*256, B2*256, 255*256},
						};
						dc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
					}

					memdc.CreateCompatibleDC(&dc);

					if (m_bmUnderCtrl.GetSafeHandle() != NULL) {
						m_bmUnderCtrl.DeleteObject();
					}

					m_bmUnderCtrl.CreateCompatibleBitmap(&dc, r.Width(), r.Height());
					CBitmap *bmOld = memdc.SelectObject(&m_bmUnderCtrl);

					if (m_nUseDarkTheme == 1) {
						m_nUseDarkTheme++;
					}

					memdc.BitBlt(r.left, r.top, r.Width(), r.Height(), &dc, r.left, r.top, SRCCOPY);

					dc.Detach();
					DeleteObject(memdc.SelectObject(bmOld));
					memdc.DeleteDC();

					m_bItemRedraw = true;
				}

				lr = CDRF_NOTIFYITEMDRAW;
				if (m_bItemRedraw) {
					lr |= CDRF_NOTIFYPOSTPAINT;
				}
				break;

			case CDDS_ITEMPREPAINT:
			case CDDS_POSTPAINT:
				if (s.bUseDarkTheme && m_bmUnderCtrl.GetSafeHandle() != NULL) {
					CDC dc;
					dc.Attach(pNMCD->hdc);
					CRect r;
					GetClientRect(&r);
					InvalidateRect(&r);
					CDC memdc;
					memdc.CreateCompatibleDC(&dc);

					CBitmap *bmOld = memdc.SelectObject(&m_bmUnderCtrl);

					if (m_nUseDarkTheme == 0) {
						m_nUseDarkTheme++;
					}

					m_nThemeBrightness	= s.nThemeBrightness;
					m_nThemeRed			= s.nThemeRed;
					m_nThemeGreen		= s.nThemeGreen;
					m_nThemeBlue		= s.nThemeBlue;

					int pa = 255 * 256;
					unsigned p1 = s.clrOutlineABGR, p2 = s.clrFaceABGR;
					int nVolume = GetPos();

					if (nVolume <= GetPageSize()) {
						nVolume = 0;
					}

					int nVolPos = r.left + (nVolume * 0.43) + 4;

					if (m_Volumebm.IsExtGradiendLoading()) {
						m_Volumebm.PaintExternalGradient(&dc, r, 0);
					} else {
						int ir1 = p1 * 256;
						int ig1 = (p1 >> 8) * 256;
						int ib1 = (p1 >> 16) * 256;
						int ir2 = p2 * 256;
						int ig2 = (p2 >> 8) * 256;
						int ib2 = (p2 >> 16) * 256;

						TRIVERTEX tv[2] = {
							{0, 0, ir1, ig1, ib1, pa},
							{50, 1, ir2, ig2, ib2, pa},
						};
						dc.GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_H);
					}

					unsigned p3 = nVolPos > 30 ? dc.GetPixel(nVolPos, 0) : dc.GetPixel(30, 0);
					CPen penLeft(p2 == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, p3);

					dc.BitBlt(0, 0, r.Width(), r.Height(), &memdc, 0, 0, SRCCOPY);

					DeleteObject(memdc.SelectObject(bmOld));
					memdc.DeleteDC();

					r.DeflateRect(4, 2, 9, 6);
					CopyRect(&pNMCD->rc, &r);

					CPen penRight(p1 == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, p1);
					CPen *penOld = dc.SelectObject(&penRight);

					int nposx, nposy;
					for (int i = 4; i <= 44; i += 4) {

						nposx = r.left + i;
						nposy = r.bottom - (r.Height() * i) / (r.Width() + 6);

						i < nVolPos ? dc.SelectObject(penLeft) : dc.SelectObject(penRight);

						dc.MoveTo(nposx, nposy);			//top_left
						dc.LineTo(nposx + 2, nposy);		//top_right
						dc.LineTo(nposx + 2, r.bottom);		//bottom_right
						dc.LineTo(nposx, r.bottom);			//bottom_left
						dc.LineTo(nposx, nposy);			//top_left

						if (!s.fMute) {
							dc.MoveTo(nposx + 1, nposy - 1);	//top_middle
							dc.LineTo(nposx + 1, r.bottom + 2);	//bottom_middle
						}
					}

					dc.SelectObject(penOld);
					dc.Detach();

					lr = CDRF_SKIPDEFAULT;
					m_bItemRedraw = false;
				} else if (!s.bUseDarkTheme && pNMCD->dwItemSpec == TBCD_CHANNEL) {
					if (m_bmUnderCtrl.GetSafeHandle() != NULL) {
						m_bmUnderCtrl.DeleteObject();
					}

					CDC dc;
					dc.Attach(pNMCD->hdc);

					CRect r;
					GetClientRect(r);
					r.DeflateRect(8, 4, 10, 6);
					CopyRect(&pNMCD->rc, &r);
					CPen shadow(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
					CPen light(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
					CPen* old = dc.SelectObject(&light);
					dc.MoveTo(pNMCD->rc.right, pNMCD->rc.top);
					dc.LineTo(pNMCD->rc.right, pNMCD->rc.bottom);
					dc.LineTo(pNMCD->rc.left, pNMCD->rc.bottom);
					dc.SelectObject(&shadow);
					dc.LineTo(pNMCD->rc.right, pNMCD->rc.top);
					dc.SelectObject(old);

					dc.Detach();
					lr = CDRF_SKIPDEFAULT;
				} else if (!s.bUseDarkTheme && pNMCD->dwItemSpec == TBCD_THUMB) {
					CDC dc;
					dc.Attach(pNMCD->hdc);
					pNMCD->rc.bottom--;
					CRect r(pNMCD->rc);
					r.DeflateRect(0, 0, 1, 0);

					COLORREF shadow = GetSysColor(COLOR_3DSHADOW);
					COLORREF light = GetSysColor(COLOR_3DHILIGHT);
					dc.Draw3dRect(&r, light, 0);
					r.DeflateRect(0, 0, 1, 1);
					dc.Draw3dRect(&r, light, shadow);
					r.DeflateRect(1, 1, 1, 1);
					dc.FillSolidRect(&r, GetSysColor(COLOR_BTNFACE));
					dc.SetPixel(r.left + 7, r.top - 1, GetSysColor(COLOR_BTNFACE));

					dc.Detach();
					lr = CDRF_SKIPDEFAULT;
				}

				if (!s.bUseDarkTheme) {
					m_nUseDarkTheme = 0;
				}

				break;
			default:
				break;
		}
	}

	pNMCD->uItemState &= ~CDIS_FOCUS;

	*pResult = lr;
}

void CVolumeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect r;
	GetChannelRect(&r);
	ASSERT(r.left < r.right);

	int start, stop;
	GetRange(start, stop);
	ASSERT(start < stop);

	r.left += 3;
	r.right -= 4;

	if (point.x < r.left) {
		SetPos(start);
	} else if (point.x >= r.right) {
		SetPos(stop);
	}

	int w = r.right - r.left - 4;
	SetPosInternal(start + ((stop - start) * (point.x - r.left) + (w / 2)) / w);

	CSliderCtrl::OnLButtonDown(nFlags, point);
}

void CVolumeCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CSliderCtrl::OnSetFocus(pOldWnd);

	AfxGetMainWnd()->SetFocus();
}

void CVolumeCtrl::HScroll(UINT nSBCode, UINT nPos)
{
	int nVolMin, nVolMax;
	GetRange(nVolMin, nVolMax);
	int nVol = min(max(nVolMin, (int)nSBCode), nVolMax);

	CRect r;
	GetClientRect(&r);
	InvalidateRect(&r);
	UpdateWindow();

	AfxGetAppSettings().nVolume = GetPos();
	CFrameWnd* pFrame = GetParentFrame();

	if (pFrame && pFrame != GetParent()) {
		pFrame->PostMessage(WM_HSCROLL, MAKEWPARAM((short)nPos, nSBCode), (LPARAM)m_hWnd);
	}
}

BOOL CVolumeCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));

	return TRUE;
}

BOOL CVolumeCtrl::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXT *pTTT = reinterpret_cast<LPTOOLTIPTEXT>(pNMHDR);
	CString str;

	str.AppendFormat(_T("%d%%"), GetPos());

	if (AfxGetAppSettings().fMute) { // TODO: remove i
		CString no_sound_str = ResStr(ID_VOLUME_MUTE_DISABLED);
		int i = no_sound_str.Find('\n');
		if (i > 0) {
			no_sound_str = no_sound_str.Left(i);
		}
		str.AppendFormat(_T(" [%ws]"), no_sound_str);
	}

	_tcscpy_s(pTTT->szText, str);
	pTTT->hinst = NULL;

	*pResult = 0;

	return TRUE;
}
