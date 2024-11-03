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
#include "WicUtils.h"
#include "DSUtil/FileHandle.h"
#include "PlayerChildView.h"

// CChildView

CChildView::CChildView(CMainFrame* pMainFrame)
	: m_hCursor(::LoadCursorW(nullptr, IDC_ARROW))
	, m_pMainFrame(pMainFrame)
{
	LoadLogo();
}

int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1) {
		return -1;
	}

	int digitizerStatus = GetSystemMetrics(SM_DIGITIZER);
	if ((digitizerStatus & (NID_READY + NID_MULTI_INPUT))) {
		DLog(L"CChildView::OnCreate() : touch is ready for input + support multiple inputs");
		if (!RegisterTouchWindow()) {
			DLog(L"CChildView::OnCreate() : RegisterTouchWindow() failed");
		}
	}

	return 0;
}

void CChildView::OnMouseLeave()
{
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(&p);

	CRect r;
	GetClientRect(r);
	if (!r.PtInRect(p)) {
		m_pMainFrame->StopAutoHideCursor();
	}

	m_bTrackingMouseLeave = false;
	CWnd::OnMouseLeave();
}

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::LoadCursorW(nullptr, IDC_ARROW), HBRUSH(COLOR_WINDOW + 1), nullptr);

	return TRUE;
}

BOOL CChildView::OnTouchInput(CPoint pt, int nInputNumber, int nInputsCount, PTOUCHINPUT pInput)
{
	return m_pMainFrame->OnTouchInput(pt, nInputNumber, nInputsCount, pInput);
}

BOOL CChildView::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MYMOUSELAST) {
		CPoint point(pMsg->lParam);
		if (pMsg->message == WM_MOUSEMOVE) {
			if (m_lastMousePoint == point) {
				return FALSE;
			}

			m_lastMousePoint = point;

			if (!m_bTrackingMouseLeave) {
				TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, this->m_hWnd };
				if (TrackMouseEvent(&tme)) {
					m_bTrackingMouseLeave = true;
				}
			}
		}

		CWnd* pParent = GetParent();
		::MapWindowPoints(pMsg->hwnd, pParent->m_hWnd, &point, 1);

		const bool fInteractiveVideo = m_pMainFrame->IsInteractiveVideo();
		if (fInteractiveVideo &&
				(pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP || pMsg->message == WM_MOUSEMOVE)) {
			if (pMsg->message == WM_MOUSEMOVE) {
				VERIFY(pParent->PostMessageW(pMsg->message, pMsg->wParam, MAKELPARAM(point.x, point.y)));
			}
		} else {
			VERIFY(pParent->PostMessageW(pMsg->message, pMsg->wParam, MAKELPARAM(point.x, point.y)));
			return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CChildView::SetVideoRect(CRect r)
{
	m_videoRect = r;
	Invalidate();
}

void CChildView::LoadLogo()
{
	CAppSettings& s = AfxGetAppSettings();

	CAutoLock cAutoLock(&m_csLogo);

	m_pBitmapResized.Release();
	m_logoRect.SetRectEmpty();

	HRESULT hr = E_FAIL;

	if (s.bLogoExternal) {
		// load external logo
		m_pBitmap.Release();
		hr = WicLoadImage(&m_pBitmap, true, s.strLogoFileName.GetString());
	}

	if (FAILED(hr)) {
		// load logo from program folder
		std::vector<LPCWSTR> logoExts = { L"png", L"bmp", L"jpg", L"jpeg", L"gif" };
		if (WicMatchDecoderFileExtension(L".webp")) {
			logoExts.emplace_back(L"webp");
		}
		if (WicMatchDecoderFileExtension(L".jxl")) {
			logoExts.emplace_back(L"jxl");
		}
		if (WicMatchDecoderFileExtension(L".heic")) {
			logoExts.emplace_back(L"heic");
		}

		const CStringW path = GetProgramDir();

		WIN32_FIND_DATAW wfd;
		HANDLE hFile = FindFirstFileW(path + L"logo.*", &wfd);
		if (hFile != INVALID_HANDLE_VALUE) {
			do {
				if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
					const CStringW filename = wfd.cFileName;
					const CStringW ext = filename.Mid(filename.ReverseFind('.') + 1).MakeLower();

					if (std::find(logoExts.cbegin(), logoExts.cend(), ext) != logoExts.cend()) {
						m_pBitmap.Release();
						hr = WicLoadImage(&m_pBitmap, true, (path+filename).GetString());
					}
				}
			} while (FAILED(hr) && FindNextFileW(hFile, &wfd));
			FindClose(hFile);
		}
	}

	if (FAILED(hr)) {
		// load internal logo
		BYTE* data;
		UINT size;
		hr = LoadResourceFile(s.nLogoId, &data, size);
		if (FAILED(hr)) {
			s.nLogoId = DEF_LOGO;
			hr = LoadResourceFile(s.nLogoId, &data, size);
		}

		if (SUCCEEDED(hr)) {
			m_pBitmap.Release();
			hr = WicLoadImage(&m_pBitmap, true, data, size);
		}
	}

	if (m_hWnd) {
		Invalidate();
	}
}

CSize CChildView::GetLogoSize() const
{
	if (m_pBitmap) {
		UINT w, h;
		HRESULT hr = m_pBitmap->GetSize(&w, &h);
		if (SUCCEEDED(hr)) {
			return { (LONG)w, (LONG)h };
		}
	}

	return { 0, 0 };
}

void CChildView::ClearResizedImage()
{
	CAutoLock cAutoLock(&m_csLogo);

	m_pBitmapResized.Release();
}

IMPLEMENT_DYNAMIC(CChildView, CWnd)

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_WM_NCHITTEST()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_CREATE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

// CChildView message handlers

void CChildView::OnPaint()
{
	CPaintDC dc(this);

	if (!m_pMainFrame->m_bDesktopCompositionEnabled) {
		m_pMainFrame->RepaintVideo();
	}
}

BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	CAutoLock cAutoLock(&m_csLogo);

	CRect r;
	GetClientRect(r);

	COLORREF bkcolor = 0;
	HRESULT hr = S_FALSE;
	CComPtr<IWICBitmapSource> pBitmapSource;

	if (m_pMainFrame->IsD3DFullScreenMode() ||
			((m_pMainFrame->m_eMediaLoadState != MLS_LOADED || m_pMainFrame->m_bAudioOnly) && !m_pMainFrame->m_bNextIsOpened)) {

		if (m_pMainFrame->m_pMainBitmap) {
			pBitmapSource = m_pMainFrame->m_pMainBitmap;
		}
		else if (m_pBitmap) {
			const WICRect wicrect = { 0,0,1,1 };
			hr = m_pBitmap->CopyPixels(&wicrect, 4, 4, (BYTE*)&bkcolor);
			bkcolor &= 0x00FFFFFF;
			pBitmapSource = m_pBitmap;
		}
	} else {
		pDC->ExcludeClipRect(m_videoRect);
	}

	if (pBitmapSource) {
		UINT width, height;
		hr = pBitmapSource->GetSize(&width, &height);

		if (SUCCEEDED(hr)) {
			int h = std::min((int)height, r.Height());
			int w = std::min(r.Width(), MulDiv(h, width, height));
			h = MulDiv(w, height, width);
			int x = std::lround(((double)r.Width() - w) / 2.0);
			int y = std::lround(((double)r.Height() - h) / 2.0);
			m_logoRect = CRect(CPoint(x, y), CSize(w, h));

			if (!m_logoRect.IsRectEmpty()) {
				if (m_pBitmapResized) {
					UINT rwidth, rheight;
					hr = m_pBitmapResized->GetSize(&rwidth, &rheight);
					if (FAILED(hr) || w != (int)rwidth || h != (int)rheight) {
						m_pBitmapResized.Release();
					}
				}

				if (!m_pBitmapResized) {
					hr = WicCreateBitmapScaled(&m_pBitmapResized, w, h, pBitmapSource);
				}

				if (SUCCEEDED(hr)) {
					HBITMAP hBitmap = nullptr;
					BYTE* data = nullptr;
					BITMAPINFO bminfo;
					hr = WicCreateDibSecton(hBitmap, &data, bminfo, m_pBitmapResized);
					::SetDIBitsToDevice(*pDC, x, y, w, h, 0, 0, 0, h, data, &bminfo, DIB_RGB_COLORS);
					pDC->ExcludeClipRect(m_logoRect);
					DeleteObject(hBitmap);
				} else {
					m_logoRect.SetRectEmpty();
				}
			}
		}
	}

	pDC->FillSolidRect(r, bkcolor);

	return TRUE;
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	m_pMainFrame->MoveVideoWindow();
	m_pMainFrame->UpdateThumbnailClip();
}

BOOL CChildView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_pMainFrame->m_bHideCursor) {
		::SetCursor(nullptr);
		return TRUE;
	}

	if (m_pMainFrame->IsSomethingLoaded() && (nHitTest == HTCLIENT)) {
		if (m_pMainFrame->m_bIsMPCVRExclusiveMode || m_pMainFrame->GetPlaybackMode() == PM_DVD) {
			return FALSE;
		}

		::SetCursor(m_hCursor);

		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

LRESULT CChildView::OnNcHitTest(CPoint point)
{
	UINT nHitTest = CWnd::OnNcHitTest(point);

	WINDOWPLACEMENT wp;
	m_pMainFrame->GetWindowPlacement(&wp);

	if (!m_pMainFrame->m_bFullScreen && wp.showCmd != SW_SHOWMAXIMIZED && AfxGetAppSettings().iCaptionMenuMode == MODE_BORDERLESS) {
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
	if (!m_pMainFrame->m_bFullScreen && (m_pMainFrame->IsCaptionHidden() || !AssignedMouseToCmd(MOUSE_CLICK_LEFT, 0))) {
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
			m_pMainFrame->PostMessageW(WM_SYSCOMMAND, (SC_SIZE | bFlag), (LPARAM)POINTTOPOINTS(point));
		} else {
			CWnd::OnNcLButtonDown(nHitTest, point);
		}
	}
}
