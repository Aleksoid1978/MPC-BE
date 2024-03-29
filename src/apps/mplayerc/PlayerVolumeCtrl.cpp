/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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
#include "DSUtil/FileHandle.h"
#include "WicUtils.h"
#include "PlayerVolumeCtrl.h"

// CVolumeCtrl

IMPLEMENT_DYNAMIC(CVolumeCtrl, CSliderCtrl)

CVolumeCtrl::CVolumeCtrl(bool bSelfDrawn/* = true*/)
	: m_bSelfDrawn(bSelfDrawn)
{
}

bool CVolumeCtrl::Create(CWnd* pParentWnd)
{
	VERIFY(CSliderCtrl::Create(WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_HORZ | TBS_TOOLTIPS, CRect(), pParentWnd, IDC_SLIDER1));

	const auto& s = AfxGetAppSettings();

	CComPtr<IWICBitmap> pBitmap;
	HRESULT hr = WicLoadImage(&pBitmap, false, (GetProgramDir() + L"background.png").GetString());
	if (SUCCEEDED(hr)) {
		m_BackGroundGradient.Create(pBitmap);
	}

	pBitmap.Release();
	hr = WicLoadImage(&pBitmap, false, (GetProgramDir() + L"volume.png").GetString());
	if (SUCCEEDED(hr)) {
		m_VolumeGradient.Create(pBitmap);
	}

	EnableToolTips(TRUE);
	SetRange(0, 100);
	SetPos(s.nVolume);
	SetPageSize(s.nVolumeStep);
	SetLineSize(0);

	m_nUseDarkTheme = (int)s.bUseDarkTheme + 1;

	m_nThemeBrightness = s.nThemeBrightness;
	m_nThemeRed        = s.nThemeRed;
	m_nThemeGreen      = s.nThemeGreen;
	m_nThemeBlue       = s.nThemeBlue;

	m_clrFaceABGR    = s.clrFaceABGR;
	m_clrOutlineABGR = s.clrOutlineABGR;

	m_toolTipHandle = (HWND)SendMessageW(TBM_GETTOOLTIPS);

	return TRUE;
}

void CVolumeCtrl::SetPosInternal(int pos)
{
	m_bRedraw = true;

	SetPos(pos);

	GetParent()->PostMessageW(WM_HSCROLL, MAKEWPARAM((short)pos, SB_THUMBPOSITION), (LPARAM)m_hWnd);
}

void CVolumeCtrl::IncreaseVolume()
{
	// align volume up to step. recommend using steps 1, 2, 5 and 10
	SetPosInternal(IncreaseByGrid(GetPos(), GetPageSize()));
}

void CVolumeCtrl::DecreaseVolume()
{
	// align volume down to step. recommend using steps 1, 2, 5 and 10
	SetPosInternal(DecreaseByGrid(GetPos(), GetPageSize()));
}

BEGIN_MESSAGE_MAP(CVolumeCtrl, CSliderCtrl)
	ON_WM_ERASEBKGND()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_WM_SETFOCUS()
	ON_WM_HSCROLL_REFLECT()
	ON_WM_MOUSEWHEEL()
	ON_WM_SETCURSOR()
	ON_NOTIFY_EX(TTN_NEEDTEXTW, 0, OnToolTipNotify)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
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
	const auto& s = AfxGetAppSettings();

	int R, G, B, R2, G2, B2;

	GRADIENT_RECT gr = {0, 1};

	if (m_bSelfDrawn) {
		switch (pNMCD->dwDrawStage) {
			case CDDS_PREPAINT:
				if (s.bUseDarkTheme && (m_bmUnderCtrl.GetSafeHandle() == nullptr
						|| m_nUseDarkTheme == 1
						|| m_nThemeBrightness != s.nThemeBrightness
						|| m_nThemeRed != s.nThemeRed
						|| m_nThemeGreen != s.nThemeGreen
						|| m_nThemeBlue != s.nThemeBlue
						|| m_clrFaceABGR != s.clrFaceABGR
						|| m_clrOutlineABGR != s.clrOutlineABGR)) {
					CDC dc;
					dc.Attach(pNMCD->hdc);

					CRect r;
					GetClientRect(&r);
					InvalidateRect(&r);

					if (m_BackGroundGradient.Size()) {
						ThemeRGB(s.nThemeRed, s.nThemeGreen, s.nThemeBlue, R, G, B);
						m_BackGroundGradient.Paint(&dc, r, 22, s.nThemeBrightness, R, G, B);
					} else {
						ThemeRGB(50, 55, 60, R, G, B);
						ThemeRGB(20, 25, 30, R2, G2, B2);
						TRIVERTEX tv[2] = {
							{ r.left, r.top, R * 256, G * 256, B * 256, 255 * 256 },
							{ r.Width(), r.Height(), R2 * 256, G2 * 256, B2 * 256, 255 * 256 },
						};
						dc.GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);
					}

					CDC memdc;
					memdc.CreateCompatibleDC(&dc);

					if (m_bmUnderCtrl.GetSafeHandle() != nullptr) {
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
					m_bRedraw = true;
				}

				lr = CDRF_NOTIFYITEMDRAW;
				if (m_bItemRedraw) {
					lr |= CDRF_NOTIFYPOSTPAINT;
				}
				break;

			case CDDS_ITEMPREPAINT:
			case CDDS_POSTPAINT:
				if (s.bUseDarkTheme && m_bmUnderCtrl.GetSafeHandle() != nullptr) {
					CDC dc;
					dc.Attach(pNMCD->hdc);

					CDC imageDC;
					imageDC.CreateCompatibleDC(&dc);
					CBitmap* pOldBitmap = nullptr;

					CRect rc;
					GetClientRect(&rc);
					InvalidateRect(&rc);
					const CSize sz = rc.Size();

					if (m_bRedraw
							|| m_nThemeBrightness != s.nThemeBrightness
							|| m_nThemeRed != s.nThemeRed
							|| m_nThemeGreen != s.nThemeGreen
							|| m_nThemeBlue != s.nThemeBlue
							|| m_clrFaceABGR != s.clrFaceABGR
							|| m_clrOutlineABGR != s.clrOutlineABGR
							|| m_bMute != s.fMute) {
						m_bRedraw = false;

						m_nThemeBrightness = s.nThemeBrightness;
						m_nThemeRed = s.nThemeRed;
						m_nThemeGreen = s.nThemeGreen;
						m_nThemeBlue = s.nThemeBlue;
						m_clrFaceABGR = s.clrFaceABGR;
						m_clrOutlineABGR = s.clrOutlineABGR;
						m_bMute = s.fMute;

						if (m_cashedBitmap.GetSafeHandle() != nullptr) {
							m_cashedBitmap.DeleteObject();
						}
						m_cashedBitmap.CreateCompatibleBitmap(&dc, sz.cx, sz.cy);

						pOldBitmap = imageDC.SelectObject(&m_cashedBitmap);

						if (m_nUseDarkTheme == 0) {
							m_nUseDarkTheme++;
						}

						const COLORREF p1 = s.clrOutlineABGR, p2 = s.clrFaceABGR;
						int nVolume = GetPos();

						if (nVolume <= GetPageSize()) {
							nVolume = 0;
						}

						const CRect DeflateRect(4, 2 + 1, 9, 6 + 5);

						CRect r_volume(rc);
						r_volume.DeflateRect(&DeflateRect);
						const int width_volume = r_volume.Width() - 9;
						const int nVolPos = rc.left + (nVolume * width_volume / 100) + 4;

						if (m_VolumeGradient.Size()) {
							m_VolumeGradient.Paint(&imageDC, rc, 0);
						} else {
							const COLOR16 ir1 = (p1 * 256);
							const COLOR16 ig1 = (p1 >> 8) * 256;
							const COLOR16 ib1 = (p1 >> 16) * 256;
							const COLOR16 ir2 = (p2 * 256);
							const COLOR16 ig2 = (p2 >> 8) * 256;
							const COLOR16 ib2 = (p2 >> 16) * 256;
							const COLOR16 pa = (255 * 256);

							TRIVERTEX tv[2] = {
								{rc.left, rc.top, ir1, ig1, ib1, pa},
								{r_volume.Width(), 1, ir2, ig2, ib2, pa},
							};
							imageDC.GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_H);
						}

						const COLORREF p3 = nVolPos > 30 ? imageDC.GetPixel(nVolPos, 0) : imageDC.GetPixel(30, 0);
						CPen penLeft(p2 == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, p3);

						{
							CDC memdc;
							memdc.CreateCompatibleDC(&dc);

							CBitmap* bmOld = memdc.SelectObject(&m_bmUnderCtrl);

							imageDC.BitBlt(0, 0, sz.cx, sz.cy, &memdc, 0, 0, SRCCOPY);

							DeleteObject(memdc.SelectObject(bmOld));
							memdc.DeleteDC();
						}

						rc.DeflateRect(&DeflateRect);
						CopyRect(&pNMCD->rc, &rc);

						CPen penRight(p1 == 0x00ff00ff ? PS_NULL : PS_SOLID, 0, p1);
						CPen* penOld = imageDC.SelectObject(&penRight);

						int nposx, nposy;
						const int width = rc.Width() - 9;
						const int step = width / 10;

						int i = 4;
						while (i <= width) {
							nposx = rc.left + i;
							nposy = rc.bottom - (rc.Height() * i) / (rc.Width() + 6);

							i < nVolPos ? imageDC.SelectObject(penLeft) : imageDC.SelectObject(penRight);

							imageDC.MoveTo(nposx, nposy);         // top_left
							imageDC.LineTo(nposx + 2, nposy);     // top_right
							imageDC.LineTo(nposx + 2, rc.bottom); // bottom_right
							imageDC.LineTo(nposx, rc.bottom);     // bottom_left
							imageDC.LineTo(nposx, nposy);         // top_left

							if (!s.fMute) {
								imageDC.MoveTo(nposx + 1, nposy - 1);     // top_middle
								imageDC.LineTo(nposx + 1, rc.bottom + 2); // bottom_middle
							}

							i += step;
						}

						imageDC.SelectObject(penOld);
					}
					else {
						pOldBitmap = imageDC.SelectObject(&m_cashedBitmap);
					}

					dc.BitBlt(0, 0, sz.cx, sz.cy, &imageDC, 0, 0, SRCCOPY);

					imageDC.SelectObject(pOldBitmap);
					imageDC.DeleteDC();
					dc.Detach();

					lr = CDRF_SKIPDEFAULT;
					m_bItemRedraw = false;
				} else if (!s.bUseDarkTheme && pNMCD->dwItemSpec == TBCD_CHANNEL) {
					if (m_bmUnderCtrl.GetSafeHandle() != nullptr) {
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

void CVolumeCtrl::SetPosInternal(const CPoint& point, const bool bUpdateToolTip/* = false*/)
{
	CRect r;
	GetChannelRect(&r);
	ASSERT(r.left < r.right);

	int start, stop;
	GetRange(start, stop);
	ASSERT(start < stop);

	r.left += 3;
	r.right -= 4;

	int posX = point.x;
	if (point.x < r.left) {
		SetPos(start);
		posX = r.left;
	} else if (point.x >= r.right) {
		SetPos(stop);
		posX = r.right;
	}

	int w = r.right - r.left - 4;
	SetPosInternal(start + ((stop - start) * (point.x - r.left) + (w / 2)) / w);

	if (bUpdateToolTip && m_toolTipHandle) {
		GetChannelRect(&r);
		ClientToScreen(r);

		CRect tooltipRect;
		::GetWindowRect(m_toolTipHandle, &tooltipRect);

		POINT p = { posX, point.y };
		ClientToScreen(&p);
		p.y = r.top - tooltipRect.Height();

		::SendMessageW(m_toolTipHandle, TTM_TRACKPOSITION, 0, MAKELPARAM(p.x, p.y));
	}
}

void CVolumeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetPosInternal(point);

	if (AfxGetAppSettings().bUseDarkTheme) {
		m_bDrag = true;
		SetCapture();

		if (m_toolTipHandle) {
			TOOLINFOW ti = { sizeof(TOOLINFOW) };
			ti.uFlags = TTF_TRACK | TTF_IDISHWND | TTF_ABSOLUTE;
			ti.hwnd = m_hWnd;
			ti.uId = (UINT_PTR)m_hWnd;
			ti.hinst = AfxGetInstanceHandle();
			ti.lpszText = LPSTR_TEXTCALLBACK;

			::SendMessageW(m_toolTipHandle, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
		}
	} else {
		CSliderCtrl::OnLButtonDown(nFlags, point);
	}
}

void CVolumeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if (AfxGetAppSettings().bUseDarkTheme && m_bDrag) {
		SetPosInternal(point, true);
	} else {
		CSliderCtrl::OnMouseMove(nFlags, point);
	}
}

void CVolumeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (AfxGetAppSettings().bUseDarkTheme) {
		if (m_bDrag) {
			ReleaseCapture();
			m_bDrag = false;
		}
		if (m_toolTipHandle) {
			::SendMessageW(m_toolTipHandle, TTM_TRACKACTIVATE, FALSE, 0);
		}
	} else {
		CSliderCtrl::OnLButtonUp(nFlags, point);
	}
}

void CVolumeCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CSliderCtrl::OnSetFocus(pOldWnd);

	AfxGetMainWnd()->SetFocus();
}

void CVolumeCtrl::HScroll(UINT nSBCode, UINT nPos)
{
	m_bRedraw = true;
	AfxGetAppSettings().nVolume = GetPos();

	CFrameWnd* pFrame = GetParentFrame();
	if (pFrame && pFrame != GetParent()) {
		pFrame->PostMessageW(WM_HSCROLL, MAKEWPARAM((short)nPos, nSBCode), (LPARAM)m_hWnd);
	}
}

BOOL CVolumeCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	if (zDelta > 0) {
		IncreaseVolume();
	} else if (zDelta < 0) {
		DecreaseVolume();
	} else {
		return FALSE;
	}

	return TRUE;
}

BOOL CVolumeCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));

	return TRUE;
}

BOOL CVolumeCtrl::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTW *pTTT = reinterpret_cast<LPTOOLTIPTEXTW>(pNMHDR);
	CString str;

	str.AppendFormat(L"%d%%", GetPos());

	if (AfxGetAppSettings().fMute) { // TODO: remove i
		CString no_sound_str = ResStr(ID_VOLUME_MUTE_DISABLED);
		int i = no_sound_str.Find('\n');
		if (i > 0) {
			no_sound_str = no_sound_str.Left(i);
		}
		str.AppendFormat(L" [%ws]", no_sound_str);
	}

	wcscpy_s(pTTT->szText, str);
	pTTT->hinst = nullptr;

	*pResult = 0;

	return TRUE;
}
