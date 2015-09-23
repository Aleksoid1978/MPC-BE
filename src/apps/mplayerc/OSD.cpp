/*
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
#include "OSD.h"
#include "../../DSUtil/SysVersion.h"
#include "MainFrm.h"
#include "FullscreenWnd.h"

#define SEEKBAR_HEIGHT			60
#define SLIDER_BAR_HEIGHT		10
#define SLIDER_CURSOR_HEIGHT	30
#define SLIDER_CURSOR_WIDTH		15
#define SLIDER_CHAP_WIDTH		4
#define SLIDER_CHAP_HEIGHT		10

COSD::COSD(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
	, m_nMessagePos(OSD_NOMESSAGE)
	, m_bSeekBarVisible(false)
	, m_bFlyBarVisible(false)
	, m_bCursorMoving(false)
	, m_pVMB(NULL)
	, m_pMVTO(NULL)
	, m_pWnd(NULL)
	, m_FontSize(0)
	, m_bMouseOverExitButton(false)
	, m_bMouseOverCloseButton(false)
	, m_bShowMessage(true)
	, m_OSDType(OSD_TYPE_NONE)
	, m_pChapterBag(NULL)
	, m_pWndInsertAfter(&wndNoTopMost)
	, m_nDEFFLAGS(SWP_NOACTIVATE | SWP_NOREDRAW | SWP_ASYNCWINDOWPOS | SWP_NOZORDER)
{
	if (IsWinEightOrLater()) {
		// remove SWP_NOZORDER for Win 8 and later - it's use WS_CHILD style
		m_nDEFFLAGS &= ~SWP_NOZORDER;
		m_pWndInsertAfter = &wndTop;
	}

	m_Color[OSD_TRANSPARENT]	= RGB(  0,   0,   0);
	m_Color[OSD_BACKGROUND]		= RGB( 32,  40,  48);
	m_Color[OSD_BORDER]			= RGB( 48,  56,  62);
	m_Color[OSD_TEXT]			= RGB(224, 224, 224);
	m_Color[OSD_BAR]			= RGB( 64,  72,  80);
	m_Color[OSD_CURSOR]			= RGB(192, 200, 208);
	m_Color[OSD_DEBUGCLR]		= RGB(128, 136, 144);

	m_penBorder.CreatePen(PS_SOLID, 1, m_Color[OSD_BORDER]);
	m_penCursor.CreatePen(PS_SOLID, 4, m_Color[OSD_CURSOR]);
	m_brushBack.CreateSolidBrush(m_Color[OSD_BACKGROUND]);
	m_brushBar.CreateSolidBrush (m_Color[OSD_BAR]);
	m_brushChapter.CreateSolidBrush(m_Color[OSD_CURSOR]);
	m_debugBrushBack.CreateSolidBrush(m_Color[OSD_DEBUGCLR]);
	m_debugPenBorder.CreatePen(PS_SOLID, 1, m_Color[OSD_BORDER]);

	ZeroMemory(&m_BitmapInfo, sizeof(m_BitmapInfo));

	HBITMAP hBmp = CMPCPngImage::LoadExternalImage(L"flybar", IDB_PLAYERFLYBAR_PNG, IMG_TYPE::UNDEF);
	BITMAP bm = { 0 };
	::GetObject(hBmp, sizeof(bm), &bm);

	if (CMPCPngImage::FileExists(CString(L"flybar")) && bm.bmWidth != bm.bmHeight * 25) {
		hBmp = CMPCPngImage::LoadExternalImage("", IDB_PLAYERFLYBAR_PNG, IMG_TYPE::UNDEF);
		::GetObject(hBmp, sizeof(bm), &bm);
	}

	if (NULL != hBmp) {
		CBitmap *bmp = DNew CBitmap();

		if (bmp) {
			bmp->Attach(hBmp);

			if (bm.bmWidth == bm.bmHeight * 25) {
				m_pButtonsImages = DNew CImageList();
				m_pButtonsImages->Create(bm.bmHeight, bm.bmHeight, ILC_COLOR32 | ILC_MASK, 1, 0);
				m_pButtonsImages->Add(bmp, static_cast<CBitmap*>(0));

				m_nButtonHeight = bm.bmHeight;
			}

			delete bmp;
		}

		DeleteObject(hBmp);
	}

	// GDI+ handling
	// Gdiplus::GdiplusStartup(&m_gdiplusToken, &m_gdiplusStartupInput, NULL);
}

COSD::~COSD()
{
	Stop();

	if (m_MemDC) {
		m_MemDC.DeleteDC();
	}

	if (m_pButtonsImages) {
		delete m_pButtonsImages;
	}

	// GDI+ handling
	// Gdiplus::GdiplusShutdown(m_gdiplusToken);
}

HRESULT COSD::Create(CWnd* pWnd)
{
	DWORD dwStyleEx	= WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED;
	DWORD dwStyle	= WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	if (IsWinEightOrLater()) {
		dwStyleEx	= WS_EX_TRANSPARENT | WS_EX_LAYERED;
		dwStyle		= WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	}
	if (!CreateEx(dwStyleEx, AfxRegisterWndClass(0), NULL, dwStyle, CRect(0, 0, 0, 0), pWnd, 0, NULL)) {
		DbgLog((LOG_TRACE, 3, L"Failed to create OSD Window"));
		return E_FAIL;
	}

	const AppSettings& s = AfxGetAppSettings();
	SetLayeredWindowAttributes(RGB(255, 0, 255), 255 - s.nOSDTransparent, LWA_ALPHA | LWA_COLORKEY);
	if (s.iShowOSD & OSD_ENABLE) {
		Start(pWnd);
	}

	return S_OK;
}

IMPLEMENT_DYNAMIC(COSD, CWnd)

BEGIN_MESSAGE_MAP(COSD, CWnd)
	ON_MESSAGE_VOID(WM_HIDE, OnHide)
	ON_MESSAGE_VOID(WM_OSD_DRAW, OnDrawWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void COSD::OnHide()
{
	ShowWindow(SW_HIDE);
}

void COSD::OnDrawWnd()
{
	DrawWnd();
}

void COSD::OnSize(UINT nType, int cx, int cy)
{
	if (m_pWnd && m_pVMB) {
		if (m_bSeekBarVisible || m_bFlyBarVisible) {
			m_bCursorMoving		= false;
			m_bSeekBarVisible	= false;
			m_bFlyBarVisible	= false;
		}

		CalcRect();
		InvalidateVMROSD();
		UpdateBitmap();
	} else if (m_pWnd) {
		//PostMessage(WM_OSD_DRAW);
		DrawWnd();
	}
}

void COSD::UpdateBitmap()
{
	CAutoLock Lock(&m_Lock);

	CWindowDC dc(m_pWnd);

	if (m_MemDC) {
		m_MemDC.DeleteDC();
	}

	ZeroMemory(&m_BitmapInfo, sizeof(m_BitmapInfo));

	if (m_MemDC.CreateCompatibleDC(&dc)) {
		BITMAPINFO	bmi = {0};
		HBITMAP		hbmpRender;

		ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
		bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth		= m_rectWnd.Width();
		bmi.bmiHeader.biHeight		= -m_rectWnd.Height(); // top-down
		bmi.bmiHeader.biPlanes		= 1;
		bmi.bmiHeader.biBitCount	= 32;
		bmi.bmiHeader.biCompression	= BI_RGB;

		hbmpRender = CreateDIBSection(m_MemDC, &bmi, DIB_RGB_COLORS, NULL, NULL, NULL);
		m_MemDC.SelectObject(hbmpRender);

		if (::GetObject(hbmpRender, sizeof(BITMAP), &m_BitmapInfo) != 0) {
			// Configure the VMR's bitmap structure
			if (m_pVMB) {
				ZeroMemory(&m_VMR9AlphaBitmap, sizeof(m_VMR9AlphaBitmap));
				m_VMR9AlphaBitmap.dwFlags		= VMRBITMAP_HDC | VMRBITMAP_SRCCOLORKEY;
				m_VMR9AlphaBitmap.hdc			= m_MemDC;
				m_VMR9AlphaBitmap.rSrc			= m_rectWnd;
				m_VMR9AlphaBitmap.rDest.left	= 0;
				m_VMR9AlphaBitmap.rDest.top		= 0;
				m_VMR9AlphaBitmap.rDest.right	= 1.0;
				m_VMR9AlphaBitmap.rDest.bottom	= 1.0;
				m_VMR9AlphaBitmap.fAlpha		= 1.0;
				m_VMR9AlphaBitmap.clrSrcKey		= m_Color[OSD_TRANSPARENT];
			}
			m_MemDC.SetTextColor(m_Color[OSD_TEXT]);
			m_MemDC.SetBkMode(TRANSPARENT);
		}

		if (m_MainFont.GetSafeHandle()) {
			m_MemDC.SelectObject(m_MainFont);
		}

		DeleteObject(hbmpRender);
	}
}

void COSD::Reset()
{
	m_bShowMessage = true;

	m_MainWndRect.SetRectEmpty();
	m_strMessage.Empty();

	CalcRect();
}

void COSD::Start(CWnd* pWnd, IVMRMixerBitmap9* pVMB)
{
	m_pVMB		= pVMB;
	m_pMVTO		= NULL;
	m_pWnd		= pWnd;
	m_OSDType	= OSD_TYPE_BITMAP;

	Reset();

	UpdateBitmap();
}

void COSD::Start(CWnd* pWnd, IMadVRTextOsd* pMVTO)
{
	m_pVMB		= NULL;
	m_pMVTO		= pMVTO;
	m_pWnd		= pWnd;
	m_OSDType	= OSD_TYPE_MADVR;

	Reset();
}

void COSD::Start(CWnd* pWnd)
{
	m_pVMB		= NULL;
	m_pMVTO		= NULL;
	m_pWnd		= pWnd;
	m_OSDType	= OSD_TYPE_GDI;

	Reset();
}

void COSD::Stop()
{
	if (m_pWnd) {
		m_pWnd->KillTimer((UINT_PTR)this);
	}

	m_bCursorMoving			= false;
	m_bSeekBarVisible		= false;
	m_bFlyBarVisible		= false;
	m_bMouseOverExitButton	= false;
	m_bMouseOverCloseButton	= false;

	ClearMessage();

	m_pVMB.Release();
	m_pMVTO.Release();
	m_pWnd = NULL;

	Reset();
}

void COSD::CalcRect()
{
	if (m_pWnd) {
		m_pWnd->GetClientRect(&m_rectWnd);

		m_rectSeekBar.left			= m_rectWnd.left		+ 10;
		m_rectSeekBar.right			= m_rectWnd.right		- 10;
		m_rectSeekBar.top			= m_rectWnd.bottom		- SEEKBAR_HEIGHT;
		m_rectSeekBar.bottom		= m_rectSeekBar.top		+ SEEKBAR_HEIGHT;

		m_rectFlyBar.left			= m_rectWnd.left;
		m_rectFlyBar.right			= m_rectWnd.right;
		m_rectFlyBar.top			= m_rectWnd.top;
		m_rectFlyBar.bottom			= m_rectWnd.top			+ 100;

		m_rectExitButton.left		= m_rectWnd.right		- 34;
		m_rectExitButton.right		= m_rectWnd.right		- 10;
		m_rectExitButton.top		= m_rectWnd.top			- 10;
		m_rectExitButton.bottom		= m_rectWnd.top			+ 34;

		m_rectCloseButton.left		= m_rectExitButton.left	- 28;
		m_rectCloseButton.right		= m_rectExitButton.left	- 4;
		m_rectCloseButton.top		= m_rectWnd.top			- 10;
		m_rectCloseButton.bottom	= m_rectWnd.top			+ 34;
	}
}

void COSD::DrawRect(CRect* rect, CBrush* pBrush, CPen* pPen)
{
	if (pPen) {
		m_MemDC.SelectObject(pPen);
	} else {
		m_MemDC.SelectStockObject(NULL_PEN);
	}

	if (pBrush) {
		m_MemDC.SelectObject(pBrush);
	} else {
		m_MemDC.SelectStockObject(HOLLOW_BRUSH);
	}

	m_MemDC.Rectangle(rect);
}

void COSD::DrawSlider(CRect* rect, __int64 llMin, __int64 llMax, __int64 llPos)
{
	m_rectBar.left		= rect->left  + 10;
	m_rectBar.right		= rect->right - 10;
	m_rectBar.top		= rect->top   + (rect->Height() - SLIDER_BAR_HEIGHT) / 2;
	m_rectBar.bottom	= m_rectBar.top + SLIDER_BAR_HEIGHT;

	if (llMax == llMin) {
		m_rectCursor.left	= m_rectBar.left;
	} else {
		m_rectCursor.left	= m_rectBar.left + (long)((m_rectBar.Width() - SLIDER_CURSOR_WIDTH) * llPos / (llMax-llMin));
	}

	m_rectCursor.right		= m_rectCursor.left + SLIDER_CURSOR_WIDTH;
	m_rectCursor.top		= rect->top   + (rect->Height() - SLIDER_CURSOR_HEIGHT) / 2;
	m_rectCursor.bottom		= m_rectCursor.top + SLIDER_CURSOR_HEIGHT;

	DrawRect(rect, &m_brushBack, &m_penBorder);
	DrawRect(&m_rectBar, &m_brushBar);

	if (AfxGetAppSettings().fChapterMarker) {
		CAutoLock lock(&m_CBLock);

		if (m_pChapterBag && m_pChapterBag->ChapGetCount() > 1 && llMax != llMin) {
			REFERENCE_TIME rt;
			for (DWORD i = 0; i < m_pChapterBag->ChapGetCount(); ++i) {
				if (SUCCEEDED(m_pChapterBag->ChapGet(i, &rt, NULL))) {
					__int64 pos = m_rectBar.Width() * rt / (llMax - llMin);
					if (pos < 0) {
						continue;
					}

					CRect r;
					r.left		= m_rectBar.left + (LONG)pos - SLIDER_CHAP_WIDTH / 2;
					r.top		= rect->top + (rect->Height() - SLIDER_CHAP_HEIGHT) / 2;
					r.right		= r.left + SLIDER_CHAP_WIDTH;
					r.bottom	= r.top + SLIDER_CHAP_HEIGHT;

					DrawRect(&r, &m_brushChapter);
				}
			}
		}
	}

	DrawRect(&m_rectCursor, NULL, &m_penCursor);
}

void COSD::DrawFlyBar(CRect* rect)
{
	icoExit = m_pButtonsImages->ExtractIcon(0);
	DrawIconEx(m_MemDC, m_rectWnd.right - 34, 10, icoExit, 0, 0, 0, NULL, DI_NORMAL);
	DestroyIcon(icoExit);

	icoClose = m_pButtonsImages->ExtractIcon(23);
	DrawIconEx(m_MemDC, m_rectWnd.right - 62, 10, icoClose, 0, 0, 0, NULL, DI_NORMAL);
	DestroyIcon(icoClose);

	if (m_bMouseOverExitButton) {
		icoExit_a = m_pButtonsImages->ExtractIcon(1);
		DrawIconEx(m_MemDC, m_rectWnd.right - 34, 10, icoExit_a, 0, 0, 0, NULL, DI_NORMAL);
		DestroyIcon(icoExit_a);
	}
	if (m_bMouseOverCloseButton) {
		icoClose_a = m_pButtonsImages->ExtractIcon(24);
		DrawIconEx(m_MemDC, m_rectWnd.right - 62, 10, icoClose_a, 0, 0, 0, NULL, DI_NORMAL);
		DestroyIcon(icoClose_a);
	}
}

void COSD::DrawMessage()
{
	if (m_BitmapInfo.bmWidth * m_BitmapInfo.bmHeight * (m_BitmapInfo.bmBitsPixel >> 3) == 0) {
		return;
	}

	if (m_nMessagePos != OSD_NOMESSAGE) {
		CRect rectText(0, 0, 0, 0);
		CRect rectMessages;

		m_MemDC.DrawText(m_strMessage, &rectText, DT_CALCRECT);
		rectText.InflateRect(20, 10);
		switch (m_nMessagePos) {
			case OSD_TOPLEFT :
				rectMessages = CRect(10, 10, min((rectText.right + 10), (m_rectWnd.right - 10)), (rectText.bottom + 12));
				break;
			case OSD_TOPRIGHT :
			default :
				rectMessages = CRect(max(10, m_rectWnd.right - 10 - rectText.Width()), 10, m_rectWnd.right-10, rectText.bottom + 10);
				break;
		}

		//m_MemDC.BeginPath();
		//m_MemDC.RoundRect(rectMessages.left, rectMessages.top, rectMessages.right, rectMessages.bottom, 10, 10);
		//m_MemDC.EndPath();
		//m_MemDC.SelectClipPath(RGN_COPY);

		GradientFill(&m_MemDC, &rectMessages);

		DWORD uFormat = DT_LEFT|DT_VCENTER|DT_NOPREFIX;

		if (rectText.right + 10 >= (rectMessages.right)) {
			uFormat = uFormat|DT_END_ELLIPSIS;
		}

		const CAppSettings& s = AfxGetAppSettings();

		CRect r;
		if (s.fFontShadow) {
			r		= rectMessages;
			r.left	+= 12;
			r.top	+= 7;
			m_MemDC.SetTextColor(RGB(16, 24, 32));
			m_MemDC.DrawText(m_strMessage, &r, uFormat);
		}

		r		= rectMessages;
		r.left	+= 10;
		r.top	+= 5;
		m_MemDC.SetTextColor(s.clrFontABGR);
		m_MemDC.DrawText(m_strMessage, &r, uFormat);
	}
}

void COSD::DrawDebug()
{
	if (!m_debugMessages.IsEmpty()) {
		CString msg, tmp;
		POSITION pos;
		pos = m_debugMessages.GetHeadPosition();
		msg.Format(_T("%s"), m_debugMessages.GetNext(pos));

		while (pos) {
			tmp = m_debugMessages.GetNext(pos);
			if (!tmp.IsEmpty()) {
				msg.AppendFormat(_T("\r\n%s"), tmp);
			}
		}

		CRect rectText(0, 0, 0, 0);
		CRect rectMessages;
		m_MemDC.DrawText(msg, &rectText, DT_CALCRECT);
		rectText.InflateRect(20, 10);

		int l, r, t, b;
		l = (m_rectWnd.Width() >> 1) - (rectText.Width() >> 1) - 10;
		r = (m_rectWnd.Width() >> 1) + (rectText.Width() >> 1) + 10;
		t = (m_rectWnd.Height() >> 1) - (rectText.Height() >> 1) - 10;
		b = (m_rectWnd.Height() >> 1) + (rectText.Height() >> 1) + 10;
		rectMessages = CRect(l, t, r, b);
		DrawRect(&rectMessages, &m_debugBrushBack, &m_debugPenBorder);
		m_MemDC.DrawText(msg, &rectMessages, DT_CENTER | DT_VCENTER);
	}
}

void COSD::InvalidateVMROSD()
{
	CAutoLock Lock(&m_Lock);

	if (!m_BitmapInfo.bmWidth || !m_BitmapInfo.bmHeight || !m_BitmapInfo.bmBitsPixel || !m_pVMB) {
		return;
	}

	memsetd(m_BitmapInfo.bmBits, 0xff000000, m_BitmapInfo.bmWidth * m_BitmapInfo.bmHeight * (m_BitmapInfo.bmBitsPixel >> 3));

	if (m_bSeekBarVisible) {
		DrawSlider(&m_rectSeekBar, m_llSeekMin, m_llSeekMax, m_llSeekPos);
	}
	if (m_bFlyBarVisible) {
		DrawFlyBar(&m_rectFlyBar);
	}

	DrawMessage();
	DrawDebug();

	m_VMR9AlphaBitmap.dwFlags &= ~VMRBITMAP_DISABLE;
	m_pVMB->SetAlphaBitmap(&m_VMR9AlphaBitmap);

	m_pMainFrame->RepaintVideo();
}

void COSD::UpdateSeekBarPos(CPoint point)
{
	m_llSeekPos = (point.x - m_rectBar.left) * (m_llSeekMax - m_llSeekMin) / (m_rectBar.Width() - SLIDER_CURSOR_WIDTH);
	m_llSeekPos = max (m_llSeekPos, m_llSeekMin);
	m_llSeekPos = min (m_llSeekPos, m_llSeekMax);

	if (AfxGetAppSettings().fFastSeek ^ (GetKeyState(VK_SHIFT) < 0)) {
		m_pMainFrame->GetClosestKeyFrame(m_llSeekPos);
	}

	if (m_pWnd) {
		m_pMainFrame->PostMessage(WM_HSCROLL, MAKEWPARAM((short)m_llSeekPos, SB_THUMBTRACK), (LPARAM)m_pWnd->m_hWnd);
	}
}

bool COSD::OnMouseMove(UINT nFlags, CPoint point)
{
	bool bRet = false;

	if (m_pVMB) {
		if (m_bCursorMoving) {
			bRet = true;
			UpdateSeekBarPos(point);
			InvalidateVMROSD();
		} else if (m_rectSeekBar.PtInRect(point)) {
			bRet = true;
			if (!m_bSeekBarVisible) {
				m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_HAND);
				m_bSeekBarVisible = true;
				InvalidateVMROSD();
			}
		} else if (m_rectFlyBar.PtInRect(point)) {
			bRet = true;
			if (!m_bFlyBarVisible) {
				m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_ARROW);
				m_bFlyBarVisible = true;
				InvalidateVMROSD();
			} else {
				if (!m_bMouseOverExitButton && m_rectExitButton.PtInRect(point)) {
					m_bMouseOverExitButton	= true;
					m_bMouseOverCloseButton	= false;
					InvalidateVMROSD();
				} else if (!m_bMouseOverCloseButton && m_rectCloseButton.PtInRect(point)) {
					m_bMouseOverExitButton	= false;
					m_bMouseOverCloseButton	= true;
					InvalidateVMROSD();
				} else if ((m_bMouseOverCloseButton && !m_rectCloseButton.PtInRect(point)) || (m_bMouseOverExitButton && !m_rectExitButton.PtInRect(point))) {
					m_bMouseOverExitButton	= false;
					m_bMouseOverCloseButton	= false;
					InvalidateVMROSD();
				}

				if (m_rectCloseButton.PtInRect(point) || m_rectExitButton.PtInRect(point)) {
					m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_HAND);
				} else {
					m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_ARROW);
				}
			}
		} else if (m_bSeekBarVisible || m_bFlyBarVisible) {
			OnMouseLeave();
		}
	}

	return bRet;
}

void COSD::OnMouseLeave()
{
	m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_ARROW);

	const bool bHideBars = (m_pVMB && (m_bSeekBarVisible || m_bFlyBarVisible));

	m_bCursorMoving			= false;
	m_bSeekBarVisible		= false;
	m_bFlyBarVisible		= false;
	m_bMouseOverExitButton	= false;
	m_bMouseOverCloseButton	= false;

	if (bHideBars) {
		// Add new timer for removing any messages
		if (m_pWnd) {
			m_pWnd->KillTimer((UINT_PTR)this);
			m_pWnd->SetTimer((UINT_PTR)this, 1000, TimerFunc);
		}
		InvalidateVMROSD();
	}
}

bool COSD::OnLButtonDown(UINT nFlags, CPoint point)
{
	bool bRet = false;

	if (m_pVMB) {
		if (m_rectCursor.PtInRect (point)) {
			m_bCursorMoving		= true;
			bRet				= true;
		} else if (m_rectExitButton.PtInRect(point) || m_rectCloseButton.PtInRect(point)) {
			bRet				= true;
		} else if (m_rectSeekBar.PtInRect(point)) {
			m_bSeekBarVisible	= true;
			bRet				= true;
			UpdateSeekBarPos(point);
			InvalidateVMROSD();
		}
	}

	return bRet;
}

bool COSD::OnLButtonUp(UINT nFlags, CPoint point)
{
	bool bRet = false;

	if (m_pVMB) {
		m_bCursorMoving = false;

		if (m_rectFlyBar.PtInRect(point)) {
			if (m_rectExitButton.PtInRect(point)) {
				m_pMainFrame->PostMessage(WM_COMMAND, ID_FILE_EXIT);
			}

			if (m_rectCloseButton.PtInRect(point)) {
				m_pMainFrame->PostMessage(WM_COMMAND, ID_FILE_CLOSEPLAYLIST);
			}
		}

		bRet = (m_rectCursor.PtInRect(point) || m_rectSeekBar.PtInRect(point));
	}

	return bRet;
}

__int64 COSD::GetPos() const
{
	return m_llSeekPos;
}

void COSD::SetPos(__int64 pos)
{
	m_llSeekPos = pos;
	if (m_bSeekBarVisible) {
		InvalidateVMROSD();
	}
}

void COSD::SetRange(__int64 start,  __int64 stop)
{
	m_llSeekMin	= start;
	m_llSeekMax = stop;
}

void COSD::GetRange(__int64& start, __int64& stop)
{
	start	= m_llSeekMin;
	stop	= m_llSeekMax;
}

void COSD::TimerFunc(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
	COSD* pOSD = (COSD*)nIDEvent;
	if (pOSD) {
		pOSD->ClearMessage();
	}

	::KillTimer(hWnd, nIDEvent);
}

void COSD::ClearMessage(bool hide)
{
	CAutoLock Lock(&m_Lock);

	if (m_bSeekBarVisible || m_bFlyBarVisible) {
		return;
	}

	if (!hide) {
		m_nMessagePos = OSD_NOMESSAGE;
	}

	BOOL bRepaint = FALSE;
	if (m_pVMB) {
		bRepaint = TRUE;
		DWORD dwBackup				= (m_VMR9AlphaBitmap.dwFlags | VMRBITMAP_DISABLE);
		m_VMR9AlphaBitmap.dwFlags	= VMRBITMAP_DISABLE;
		m_pVMB->SetAlphaBitmap(&m_VMR9AlphaBitmap);
		m_VMR9AlphaBitmap.dwFlags	= dwBackup;
	} else if (m_pMVTO) {
		m_pMVTO->OsdClearMessage();
	} else if (::IsWindow(m_hWnd) && IsWindowVisible()) {
		PostMessage(WM_HIDE);
	}

	if (bRepaint) {
		m_pMainFrame->RepaintVideo();
	}
}

void COSD::DisplayMessage(OSD_MESSAGEPOS nPos, LPCTSTR strMsg, int nDuration, int FontSize, CString OSD_Font)
{
	if (!m_bShowMessage) {
		return;
	}

	const CAppSettings& s = AfxGetAppSettings();

	if (m_pVMB) {
		if (nPos != OSD_DEBUG) {
			m_nMessagePos	= nPos;
			m_strMessage	= strMsg;
		} else {
			m_debugMessages.AddTail(strMsg);
			if (m_debugMessages.GetCount() > 20) {
				m_debugMessages.RemoveHead();
			}
			nDuration = -1;
		}

		m_FontSize = FontSize ? FontSize : s.nOSDSize;

		if (m_FontSize < 10 || m_FontSize > 26) {
			m_FontSize = 20;
		}

		m_OSD_Font = OSD_Font.IsEmpty() ? s.strOSDFont : OSD_Font;

		if (m_MainFont.GetSafeHandle()) {
			m_MainFont.DeleteObject();
		}

		LOGFONT lf;
		ZeroMemory(&lf, sizeof(lf));
		lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;
		lf.lfHeight			= m_FontSize * 10;
		lf.lfQuality		= s.fFontAA ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, m_OSD_Font.GetBuffer(0));

		m_MainFont.CreatePointFontIndirect(&lf, &m_MemDC);
		m_MemDC.SelectObject(m_MainFont);

		if (m_pWnd) {
			m_pWnd->KillTimer((UINT_PTR)this);
			if (nDuration != -1) {
				m_pWnd->SetTimer((UINT_PTR)this, nDuration, (TIMERPROC)TimerFunc);
			}
		}

		InvalidateVMROSD();
	} else if (m_pMVTO) {
		m_pMVTO->OsdDisplayMessage(strMsg, nDuration);
	} else if (m_pWnd) {
		if (nPos != OSD_DEBUG) {
			m_nMessagePos	= nPos;
			m_strMessage	= strMsg;
		}

		m_FontSize = FontSize ? FontSize : s.nOSDSize;

		if (m_FontSize < 10 || m_FontSize > 26) {
			m_FontSize = 20;
		}

		m_OSD_Font = OSD_Font.IsEmpty() ? s.strOSDFont : OSD_Font;

		if (m_pWnd) {
			m_pWnd->KillTimer((UINT_PTR)this);
			if (nDuration != -1) {
				m_pWnd->SetTimer((UINT_PTR)this, nDuration, (TIMERPROC)TimerFunc);
			}

			SetWindowPos(m_pWndInsertAfter, 0, 0, 0, 0, m_nDEFFLAGS | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			//PostMessage(WM_OSD_DRAW);
			DrawWnd();
		}
	}
}

void COSD::DebugMessage(LPCTSTR format, ...)
{
	CString tmp;
	va_list argList;
	va_start(argList, format);
	tmp.FormatV(format, argList);
	va_end(argList);

	DisplayMessage(OSD_DEBUG, tmp);
}

void COSD::HideMessage(bool hide)
{
	if (m_pVMB) {
		if (hide) {
			ClearMessage(true);
		} else {
			InvalidateVMROSD();
		}
	} else {
		if (hide) {
			ClearMessage(true);
		} else {
			if (!m_strMessage.IsEmpty()) {
				SetWindowPos(m_pWndInsertAfter, 0, 0, 0, 0, m_nDEFFLAGS | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			}
			//PostMessage(WM_OSD_DRAW);
			DrawWnd();
		}
	}
}

BOOL COSD::PreTranslateMessage(MSG* pMsg)
{
	if (m_pWnd) {
		switch (pMsg->message) {
			case WM_LBUTTONDOWN :
			case WM_LBUTTONDBLCLK :
			case WM_MBUTTONDOWN :
			case WM_MBUTTONUP :
			case WM_MBUTTONDBLCLK :
			case WM_RBUTTONDOWN :
			case WM_RBUTTONUP :
			case WM_RBUTTONDBLCLK :
				m_pWnd->SetFocus();
				break;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

BOOL COSD::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	cs.style &= ~WS_BORDER;

	return TRUE;
}

BOOL COSD::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void COSD::OnPaint()
{
	CPaintDC dc(this);

	//PostMessage(WM_OSD_DRAW);
	DrawWnd();
}

void COSD::DrawWnd()
{
	CAutoLock Lock(&m_Lock);

	if (!IsWindowVisible() || !m_pWnd || m_OSDType != OSD_TYPE_GDI || m_strMessage.IsEmpty()) {
		return;
	}

	if (m_nMessagePos == OSD_NOMESSAGE) {
		if (IsWindowVisible()) {
			ShowWindow(SW_HIDE);
		}
		return;
	}

	CClientDC dc(this);

	CDC temp_DC;
	temp_DC.CreateCompatibleDC(&dc);
	CBitmap temp_BM;
	temp_BM.CreateCompatibleBitmap(&temp_DC, m_MainWndRect.Width(), m_MainWndRect.Height());
	CBitmap* temp_pOldBmt = temp_DC.SelectObject(&temp_BM);

	if (m_MainFont.GetSafeHandle()) {
		m_MainFont.DeleteObject();
	}

	const CAppSettings& s = AfxGetAppSettings();

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;
	lf.lfHeight			= m_FontSize * 10;
	lf.lfQuality		= s.fFontAA ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, m_OSD_Font.GetBuffer(0));

	m_MainFont.CreatePointFontIndirect(&lf, &temp_DC);
	temp_DC.SelectObject(m_MainFont);

	CRect rectText;
	CRect rectMessages;
	temp_DC.DrawText(m_strMessage, &rectText, DT_CALCRECT);
	rectText.InflateRect(0, 0, 10, 10);

	switch (m_nMessagePos) {
		case OSD_TOPLEFT :
			rectMessages = CRect(0, 0, min((rectText.right + 10), m_MainWndRect.Width() - 20), min((rectText.bottom + 2), m_MainWndRect.Height() - 20));
			break;
		case OSD_TOPRIGHT :
		default :
			int imax = max(0, m_MainWndRect.Width() - rectText.Width() - 30);
			rectMessages = CRect(imax, 0, (m_MainWndRect.Width() - 20) + imax, min((rectText.bottom + 2), m_MainWndRect.Height() - 20));
			break;
	}

	temp_DC.SelectObject(temp_pOldBmt);
	temp_BM.DeleteObject();
	temp_DC.DeleteDC();

	CRect wr(m_MainWndRect.left + 10 + rectMessages.left, m_MainWndRect.top + 10, rectMessages.Width() - rectMessages.left, rectMessages.Height());
	if (IsWinEightOrLater()) {
		wr.left	-= m_MainWndRect.left;
		wr.top	-= m_MainWndRect.top;
	}
	SetWindowPos(NULL, wr.left, wr.top, wr.right, wr.bottom, m_nDEFFLAGS | SWP_NOZORDER);

	CRect rcBar;
	GetClientRect(&rcBar);

	CDC mdc;
	mdc.CreateCompatibleDC(&dc);
	CBitmap bm;
	bm.CreateCompatibleBitmap(&dc, rcBar.Width(), rcBar.Height());
	CBitmap* pOldBm = mdc.SelectObject(&bm);
	mdc.SetBkMode(TRANSPARENT);

	if (m_MainFont.GetSafeHandle()) {
		m_MainFont.DeleteObject();
	}

	m_MainFont.CreatePointFontIndirect(&lf, &mdc);
	mdc.SelectObject(m_MainFont);

	GradientFill(&mdc, &rcBar);

	DWORD uFormat = DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_NOPREFIX;

	CRect r;

	if (s.fFontShadow) {
		r			= rcBar;
		r.left		= 12;
		r.top		= 7;
		r.bottom	+= rectText.Height();

		mdc.SetTextColor(RGB(16, 24, 32));
		mdc.DrawText(m_strMessage, &r, uFormat);
	}

	r			= rcBar;
	r.left		= 10;
	r.top		= 5;
	r.bottom	+= rectText.Height();

	mdc.SetTextColor(s.clrFontABGR);
	mdc.DrawText(m_strMessage, m_strMessage.GetLength(), &r, uFormat);

	/*
	// GDI+ handling

	using namespace Gdiplus;
	Graphics graphics(mdc.GetSafeHdc());
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);
	graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

	CString ss(message);
	CString f(m_OSD_Font);
	Font font(mdc);

	FontFamily fontFamily;
	font.GetFamily(&fontFamily);

	StringFormat strformat;
	//strformat.SetAlignment((StringAlignment) 0);
	//strformat.SetLineAlignment(StringAlignmentCenter);
	strformat.SetFormatFlags(StringFormatFlagsNoWrap);
	strformat.SetTrimming(StringTrimmingEllipsisCharacter);//SetFormatFlags(StringFormatFlagsLineLimit );
	RectF p(r.left+10, r.top, r.Width(), r.Height());

	REAL enSize = font.GetSize();

	GraphicsPath path;
	path.AddString(ss.AllocSysString(), ss.GetLength(), &fontFamily, FontStyleRegular, enSize, p, &strformat);

	Pen pen(Color(76,80,86), 5);
	pen.SetLineJoin(LineJoinRound);
	graphics.DrawPath(&pen, &path);

	//for(int i=1; i<8; ++i)
	//{
	//	Pen pen(Color(32, 32, 28, 30), i);
	//	pen.SetLineJoin(LineJoinRound);
	//	graphics.DrawPath(&pen, &path);
	//}


	LinearGradientBrush brush(Gdiplus::Rect(r.left, r.top, r.Width(), r.Height()), Color(255, 255, 255), Color(217, 229, 247), LinearGradientModeVertical);
	graphics.FillPath(&brush, &path);
	*/

	dc.BitBlt(0, 0, rcBar.Width(), rcBar.Height(), &mdc, 0, 0, SRCCOPY);

	mdc.SelectObject(pOldBm);
	bm.DeleteObject();
	mdc.DeleteDC();
}

void COSD::SetChapterBag(CComPtr<IDSMChapterBag>& pCB)
{
	CAutoLock lock(&m_CBLock);

	if (pCB) {
		m_pChapterBag.Release();
		pCB.CopyTo(&m_pChapterBag);
	}
}

void COSD::GradientFill(CDC* pDc, CRect* rc)
{
	const AppSettings& s = AfxGetAppSettings();

	int R, G, B, R1, G1, B1, R_, G_, B_, R1_, G1_, B1_;
	R	= GetRValue(s.clrGrad1ABGR);
	G	= GetGValue(s.clrGrad1ABGR);
	B	= GetBValue(s.clrGrad1ABGR);
	R1	= GetRValue(s.clrGrad2ABGR);
	G1	= GetGValue(s.clrGrad2ABGR);
	B1	= GetBValue(s.clrGrad2ABGR);
	R_	= min(R + 32, 255);
	R1_	= min(R1 + 32, 255);
	G_	= min(G + 32, 255);
	G1_	= min(G1 + 32, 255);
	B_	= min(B + 32, 255);
	B1_	= min(B1 + 32, 255);

	int nOSDTransparent	= s.nOSDTransparent;
	int nOSDBorder		= s.nOSDBorder;

	GRADIENT_RECT gr[1] = {{0, 1}};
	TRIVERTEX tv[2] = {
		{rc->left, rc->top, R * 256, G * 256, B * 256, nOSDTransparent * 256},
		{rc->right, rc->bottom, R1 * 256, G1 * 256, B1 * 256, nOSDTransparent * 256},
	};
	pDc->GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);

	if (nOSDBorder > 0) {
		GRADIENT_RECT gr2[1] = {{0, 1}};
		TRIVERTEX tv2[2] = {
			{rc->left, rc->top, R_ * 256, G_ * 256, B_ * 256, nOSDTransparent * 256},
			{rc->left + nOSDBorder, rc->bottom, R1_ * 256, G1_ * 256, B1_ * 256, nOSDTransparent * 256},
		};
		pDc->GradientFill(tv2, 2, gr2, 1, GRADIENT_FILL_RECT_V);

		GRADIENT_RECT gr3[1] = {{0, 1}};
		TRIVERTEX tv3[2] = {
			{rc->right, rc->top, R_ * 256, G_ * 256, B_ * 256, nOSDTransparent * 256},
			{rc->right - nOSDBorder, rc->bottom, R1_ * 256, G1_ * 256, B1_ * 256, nOSDTransparent * 256},
		};
		pDc->GradientFill(tv3, 2, gr3, 1, GRADIENT_FILL_RECT_V);

		GRADIENT_RECT gr4[1] = {{0, 1}};
		TRIVERTEX tv4[2] = {
			{rc->left, rc->top, R_ * 256, G_ * 256, B_ * 256, nOSDTransparent * 256},
			{rc->right, rc->top + nOSDBorder, R_ * 256, G_ * 256, B_ * 256, nOSDTransparent * 256},
		};
		pDc->GradientFill(tv4, 2, gr4, 1, GRADIENT_FILL_RECT_V);

		GRADIENT_RECT gr5[1] = {{0, 1}};
		TRIVERTEX tv5[2] = {
			{rc->left, rc->bottom, R1_ * 256, G1_ * 256, B1_ * 256, nOSDTransparent * 256},
			{rc->right, rc->bottom - nOSDBorder, R1_ * 256, G1_ * 256, B1_ * 256, nOSDTransparent * 256},
		};
		pDc->GradientFill(tv5, 2, gr5, 1, GRADIENT_FILL_RECT_V);
	}
}