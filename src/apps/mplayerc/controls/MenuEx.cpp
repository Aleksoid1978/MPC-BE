/*
 * (C) 2018 see Authors.txt
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
#include "MenuEx.h"
#include "../MainFrm.h"
#include "../../../DSUtil/SysVersion.h"

void CMenuEx::FreeResource()
{
	for (auto& item : m_pMenuItems) {
		SAFE_DELETE(item);
	}

	m_pMenuItems.clear();
}

void CMenuEx::SetMain(CMainFrame* pMainFrame)
{
	m_pMainFrame = pMainFrame;
}

void CMenuEx::ScaleFont()
{
	m_font.DeleteObject();

	NONCLIENTMETRICS nm = { 0 };
	nm.cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, nm.cbSize, &nm, 0));

	if (SysVersion::IsWin8orLater()) {
		nm.lfMenuFont.lfHeight = m_pMainFrame->ScaleSystemToMonitorY(nm.lfMenuFont.lfHeight);
	}
	m_font.CreateFontIndirectW(&nm.lfMenuFont);

	if (SysVersion::IsWin8orLater()) {
		m_CYEDGE = m_pMainFrame->ScaleX(::GetSystemMetrics(SM_CYEDGE));
		m_CYMENU = m_pMainFrame->ScaleX(::GetSystemMetrics(SM_CYMENU));
		m_CXMENUCHECK = m_pMainFrame->ScaleX(::GetSystemMetrics(SM_CXMENUCHECK));
		m_CYMENUCHECK = m_pMainFrame->ScaleX(::GetSystemMetrics(SM_CYMENUCHECK));
	} else {
		m_CYEDGE = ::GetSystemMetrics(SM_CYEDGE);
		m_CYMENU = ::GetSystemMetrics(SM_CYMENU);
		m_CXMENUCHECK = ::GetSystemMetrics(SM_CXMENUCHECK);
		m_CYMENUCHECK = ::GetSystemMetrics(SM_CYMENUCHECK);
	}
}

void CMenuEx::DrawCheckMark(CDC* pDC, CRect rect, const UINT uState, const bool bGrayed, const bool bSelected, CRect& rcMark)
{
	rcMark.SetRectEmpty();

	rect.DeflateRect(2, 0);

	HDC hdcMem = ::CreateCompatibleDC(pDC->GetSafeHdc());
	HBITMAP hbmMono = ::CreateBitmap(m_CXMENUCHECK, m_CYMENUCHECK, 1, 1, nullptr);
	if (hbmMono) {
		HBITMAP hbmPrev = (HBITMAP)::SelectObject(hdcMem, hbmMono);
		if (hbmPrev) {
			RECT rCheck = { 0, 0, m_CXMENUCHECK, m_CYMENUCHECK };
			::DrawFrameControl(hdcMem, &rCheck, DFC_MENU, uState);

			// Draw a white check mark for a selected item.
			HDC hdcMask = ::CreateCompatibleDC(pDC->GetSafeHdc());
			if (hdcMask) {
				HBITMAP hbmMask = ::CreateCompatibleBitmap(pDC->GetSafeHdc(), m_CXMENUCHECK, m_CYMENUCHECK);
				if (hbmMask) {
					HBITMAP hbmPrevMask = (HBITMAP)::SelectObject(hdcMask, hbmMask);
					if (hbmPrevMask) {
						// Set the colour of the check mark to white
						const CAppSettings& s = AfxGetAppSettings();
						bGrayed ? SetBkColor(hdcMask, m_crTGL) : (bSelected ? SetBkColor(hdcMask, s.clrFaceABGR) : SetBkColor(hdcMask, m_crTN));
						::BitBlt(hdcMask, 0, 0, m_CXMENUCHECK, m_CYMENUCHECK, hdcMask, 0, 0, PATCOPY);

						// Invert the check mark bitmap
						::BitBlt(hdcMem, 0, 0, m_CXMENUCHECK, m_CYMENUCHECK, hdcMem, 0, 0, DSTINVERT);

						// Use the mask to copy the check mark to Menu's device context
						::BitBlt(hdcMask, 0, 0, m_CXMENUCHECK, m_CYMENUCHECK, hdcMem, 0, 0, SRCAND);

						const int offset = (rect.Height() - m_CXMENUCHECK) / 2;
						::BitBlt(pDC->GetSafeHdc(), rect.left + offset, rect.top + offset, m_CXMENUCHECK, m_CYMENUCHECK, hdcMask, 0, 0, SRCPAINT);

						rcMark = CRect(CPoint(rect.left + offset, rect.top + offset), CSize(m_CXMENUCHECK, m_CYMENUCHECK));
					}
				}	
			}
			::SelectObject(hdcMem, hbmPrev);
		}
		::DeleteObject(hbmMono);
	}
	::DeleteDC(hdcMem);
}

void CMenuEx::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC	dc;
	CRect rect(lpDIS->rcItem);
	dc.Attach(lpDIS->hDC);
	CFont* pOldFont = dc.SelectObject(&m_font);

	LPMENUITEM lpItem = (LPMENUITEM)lpDIS->itemData;

	const bool bSelected = lpDIS->itemState & ODS_SELECTED;
	const bool bChecked = lpDIS->itemState & ODS_CHECKED;
	const bool bGrayed = lpDIS->itemState & ODS_GRAYED;
	const bool bHotlight = lpDIS->itemState & ODS_HOTLIGHT;

	if (bSelected) {
		dc.SetTextColor(m_crTNL);
	}
	else if (bGrayed || lpDIS->itemState & ODS_DISABLED) {
		dc.SetTextColor(m_crTG);
	}
	else {
		dc.SetTextColor(m_crTN);
	}

	if (lpItem->bMainMenu) { // SYSTEMMENU
		CBrush brush(m_crBkBar);
		dc.FillRect(&rect, &brush);
	}
	else {
		CBrush brush(m_crBN);
		dc.FillRect(&rect, &brush);
	}

	dc.SetBkMode(TRANSPARENT);
	if (bHotlight && lpItem->bMainMenu) {
		dc.Draw3dRect(rect, m_crTG, m_crTG);
		dc.SetTextColor(m_crTNL);
	}

	if (lpItem->uID == 0) { // SEPARATOR
		rect.top = rect.Height() / 2 + rect.top;
		rect.bottom = rect.top + 2;
		rect.left += 2;
		rect.right -= 2;
		dc.Draw3dRect(rect, m_crTGL, m_crBSD);
	}
	else {
		if (lpItem->bMainMenu) { // SYSTEMMENU
			TextMenu(&dc, rect, rect, bSelected, bGrayed, lpItem);
		}
		else {
			const int offset = rect.Height() + m_CXMENUCHECK / 2;
			CRect rcText(rect.left + offset, rect.top, rect.right, rect.bottom);

			TextMenu(&dc, rect, rcText, bSelected, bGrayed, lpItem);
		}

		if (bChecked) {
			MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE };
			::GetMenuItemInfoW(HMENU(lpDIS->hwndItem), lpDIS->itemID, MF_BYCOMMAND, &mii);

			CRect rcMark;
			DrawCheckMark(&dc, rect, mii.fType & MFT_RADIOCHECK ? DFCS_MENUBULLET : DFCS_MENUCHECK, bGrayed, bSelected, rcMark);

			rcMark.left -= 2;
			rcMark.top -= 2;
			rcMark.right += 2;
			rcMark.bottom += 2;
			if (bSelected && !(lpItem->bMainMenu) && !bGrayed) {
				dc.Draw3dRect(&rcMark, m_crBRD, m_crBRL);
			} else if (bSelected && bGrayed) {
				dc.Draw3dRect(&rcMark, m_crBSD, m_crBSL);
			}
		}
	}

	dc.SelectObject(pOldFont);
	dc.Detach();
}

void CMenuEx::TextMenu(CDC *pDC, const CRect &rect, CRect rtText, const bool bSelected, const bool bGrayed, const LPMENUITEM lpItem)
{
	if (bSelected) {
		GRADIENT_RECT gr[1] = { { 0, 1 } };
		const CAppSettings& s = AfxGetAppSettings();
		
		if (bGrayed) {
			pDC->SetTextColor(0);
		} else {
			pDC->SetTextColor(s.clrFaceABGR);
		}

		if (lpItem->bMainMenu) {
			pDC->FillSolidRect(rect.left, rect.top + 1, rect.Width() - 2, rect.Height() - 2, m_crBN);
			pDC->Draw3dRect(rect.left, rect.top + 1, rect.Width() - 2, rect.Height() - 2, m_crBRL, m_crBRL);
		}
		else {
			TRIVERTEX tv[2] = {
				{ rect.left, rect.top, (GetRValue(m_crBR) + 5) * 256, (GetGValue(m_crBR) + 5) * 256, (GetBValue(m_crBR) + 5) * 256, 255 * 256 },
				{ rect.right + 1, rect.bottom - 2, (GetRValue(m_crBR) - 5) * 256, (GetGValue(m_crBR) - 5) * 256, (GetBValue(m_crBR) - 5) * 256, 255 * 256 },
			};
			pDC->GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
			pDC->Draw3dRect(rect.left, rect.top, rect.Width(), rect.Height() - 1, m_crBRL, m_crBRD);
		}
	}

	pDC->SetBkMode(TRANSPARENT);

	CString strR;
	CString str = lpItem->strText;
	const int pos = str.Find('\t');
	if (pos > 0) {
		strR = str.Right(str.GetLength() - pos);
		str = str.Left(pos);
	}

	if (lpItem->bMainMenu) {
		pDC->DrawTextW(str, &rtText, DT_SINGLELINE | DT_EXPANDTABS | DT_CENTER | DT_VCENTER);
	}
	else {
		pDC->DrawTextW(str, &rtText, DT_SINGLELINE | DT_EXPANDTABS | DT_LEFT | DT_VCENTER);
		if (!strR.IsEmpty()) {
			rtText.right -= m_CXMENUCHECK;
			pDC->DrawTextW(strR, &rtText, DT_SINGLELINE | DT_EXPANDTABS | DT_RIGHT | DT_VCENTER);
		}
	}
}

void CMenuEx::ChangeStyle(CMenu *pMenu, const bool bMainMenu/* = false*/)
{
	ASSERT(pMenu);

	for (int i = 0; i < pMenu->GetMenuItemCount(); i++) {
		MENUITEMINFOW mii = { sizeof(mii), MIIM_ID | MIIM_TYPE };
		pMenu->GetMenuItemInfoW(i, &mii, TRUE);
		const auto uID = mii.wID;

		LPMENUITEM lpItem = nullptr;
		auto it = std::find_if(m_pMenuItems.begin(), m_pMenuItems.end(), [&](const MENUITEM* item) {
			return item->uID == uID && item->bMainMenu == bMainMenu;
		});

		if (it != m_pMenuItems.end()) {
			lpItem = *it;
		} else {
			lpItem = DNew MENUITEM;
			m_pMenuItems.push_back(lpItem);
		}

		lpItem->uID = uID;
		lpItem->bMainMenu = bMainMenu;

		if (lpItem->uID > 0) {
			pMenu->GetMenuStringW(i, lpItem->strText, MF_BYPOSITION);

			auto pSubMenu = pMenu->GetSubMenu(i);
			if (pSubMenu) {
				lpItem->bPopupMenu = true;
				ChangeStyle(pSubMenu);
			}
		}

		mii.fMask = MIIM_FTYPE | MIIM_DATA;
        mii.fType |= MFT_OWNERDRAW; 
        mii.dwItemData = (ULONG_PTR)lpItem;
        pMenu->SetMenuItemInfoW(i, &mii, TRUE);
	}
}

void CMenuEx::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	LPMENUITEM lpItem = (LPMENUITEM)lpMIS->itemData;

	if (lpItem->uID > 0) {
		CDC dcScreen;
		dcScreen.Attach(::GetDC(nullptr));
		dcScreen.SaveDC();

		dcScreen.SelectObject(&m_font);

		SIZE size = {};
		CString strText(lpItem->strText);
		strText.Remove(L'&');
		VERIFY(::GetTextExtentPoint32W(dcScreen.GetSafeHdc(), strText.GetString(), strText.GetLength(), &size));
		if (size.cy < m_CYMENU) {
			size.cy = m_CYMENU;
		}

		dcScreen.RestoreDC(-1);
		::ReleaseDC(nullptr, dcScreen.Detach());

		if (lpItem->bMainMenu) {
			lpMIS->itemWidth = size.cx;
		} else {
			lpMIS->itemWidth = size.cx + m_CYMENU * 2;
			if (strText.Find('\t') > 0) {
				lpMIS->itemWidth += m_CYMENU * 2;
			}
			if (lpItem->bPopupMenu) {
				lpMIS->itemWidth += m_CYMENU;
			}
		}

		lpMIS->itemHeight = size.cy + m_CYEDGE * 2;
	} else {
		lpMIS->itemWidth = 1;
		lpMIS->itemHeight = m_CYMENU / 2 - 1;
	}
}

void CMenuEx::SetColorMenu(
	const COLORREF crBkBar,
	const COLORREF crBN, const COLORREF crBNL, const COLORREF crBND,
	const COLORREF crBR, const COLORREF crBRL, const COLORREF crBRD,
	const COLORREF crBS, const COLORREF crBSL, const COLORREF crBSD,
	const COLORREF crTN, const COLORREF crTNL, const COLORREF crTG, const COLORREF crTGL)
{
	m_crBkBar = crBkBar;
	m_crBN  = crBN;
	m_crBNL = crBNL;
	m_crBND = crBND;

	m_crBR = crBR;
	m_crBRL = crBRL;
	m_crBRD = crBRD;

	m_crBS = crBS;
	m_crBSL = crBSL;
	m_crBSD = crBSD;

	m_crTN = crTN;
	m_crTNL = crTNL;
	m_crTG = crTG;
	m_crTGL = crTGL;
}

void CMenuEx::Hook()
{
    m_hookCBT = ::SetWindowsHookExW(WH_CBT, CBTProc, nullptr, ::GetCurrentThreadId());
}

void CMenuEx::UnHook()
{
    ::UnhookWindowsHookEx(m_hookCBT);
}

void CMenuEx::EnableHook(const bool bEnable)
{
	m_bUseDrawHook = bEnable;
}

LPCTSTR g_pszOldMenuProc = L"OldPopupMenuProc";

LRESULT CALLBACK CMenuEx::MenuWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC pfnOldProc = (WNDPROC)::GetPropW(hWnd, g_pszOldMenuProc);
    switch (Msg) {
		case WM_DESTROY:
			::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pfnOldProc);
			::RemovePropW(hWnd, g_pszOldMenuProc);
			break;
		/*
		case WM_ERASEBKGND:
			{
				RECT rc;
				::GetWindowRect(hWnd, &rc);
				::OffsetRect(&rc, -rc.left, -rc.top);
            
				HDC hDC = (HDC)wParam;
            
				HBRUSH hBrush = ::CreateSolidBrush(m_crBN);
				FillRect(hDC, CRect(rc.left, rc.top, rc.right, rc.bottom), hBrush);
				::DeleteObject(hBrush);
			}
			return 1;
		*/
		case WM_PRINT:
			if (m_bUseDrawHook) {
				::CallWindowProcW(pfnOldProc, hWnd, Msg, wParam, lParam);
				if (lParam & PRF_NONCLIENT) {
					GUITHREADINFO tdInfo = { sizeof(tdInfo) };
					if (GetGUIThreadInfo(NULL, &tdInfo)) {
						wchar_t lpClassName[MAX_PATH] = {};
						const int nLength = ::GetClassNameW(tdInfo.hwndActive, lpClassName, MAX_PATH);
						if (nLength == 6 && lstrcmpW(lpClassName, L"MPC-BE") == 0) {
							RECT rc;
							::GetWindowRect(hWnd, &rc);
							::OffsetRect(&rc, -rc.left, -rc.top);
            
							HDC hDC = (HDC)wParam;
            
							HBRUSH hBrush = ::CreateSolidBrush(m_crBN);
							HRGN hrgnPaint = ::CreateRectRgnIndirect(&rc);
							::FrameRgn(hDC, hrgnPaint, hBrush, ::GetSystemMetrics(SM_CXFRAME) - 1, ::GetSystemMetrics(SM_CYFRAME) - 1);
							::DeleteObject(hrgnPaint);

							::FillRect(hDC, CRect(rc.left, rc.top, rc.left + (::GetSystemMetrics(SM_CXFRAME) - 1), rc.bottom), hBrush);
							::DeleteObject(hBrush);
				
							CDC *pDC = CDC::FromHandle(hDC);
							pDC->Draw3dRect(&rc, m_crBNL, m_crBND);
						}
					}
				}
				return 0;
			}
			break;
		case WM_NCPAINT:
			if (m_bUseDrawHook) {
				HDC hDC = ::GetWindowDC(hWnd);
				::SendMessageW(hWnd, WM_PRINT, (WPARAM)hDC, PRF_NONCLIENT);
				::ReleaseDC(hWnd, hDC);
				return 0;
			}
			break;
    }

    return ::CallWindowProcW(pfnOldProc, hWnd, Msg, wParam, lParam);
}

LRESULT CALLBACK CMenuEx::CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HCBT_CREATEWND) {
        HWND hwndWnd = (HWND)wParam;
        LPCBT_CREATEWNDW pS = (LPCBT_CREATEWNDW)lParam;
        
		wchar_t lpClassName[MAX_PATH] = {};
        const int nLength = ::GetClassNameW(hwndWnd, lpClassName, MAX_PATH);
        if (nLength == 6 && lstrcmpW(lpClassName, L"#32768") == 0) {
            WNDPROC pfnOldProc = (WNDPROC)::GetWindowLongPtr(hwndWnd, GWLP_WNDPROC);

            ::SetPropW(hwndWnd, g_pszOldMenuProc, (HANDLE)pfnOldProc);
            ::SetWindowLongPtr(hwndWnd, GWLP_WNDPROC, (LONG_PTR)MenuWndProc);

            // Force menu window to repaint frame.
            ::SetWindowPos(hwndWnd, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            ::RedrawWindow(hwndWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE);
        }
    }
    return ::CallNextHookEx(m_hookCBT, nCode, wParam, lParam);
}
