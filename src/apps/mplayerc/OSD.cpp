/*
 * (C) 2006-2025 see Authors.txt
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
#include "DSUtil/SysVersion.h"
#include "DSUtil/FileHandle.h"
#include "WicUtils.h"
#include "OSD.h"

#define SEEKBAR_HEIGHT       60
#define SLIDER_BAR_HEIGHT    10
#define SLIDER_CURSOR_HEIGHT 20
#define SLIDER_CURSOR_WIDTH  8
#define SLIDER_CHAP_HEIGHT   10
#define SLIDER_CHAP_WIDTH    4

#define OSD_COLOR_TRANSPARENT RGB(  0,   0,   0)
#define OSD_COLOR_BACKGROUND  RGB( 32,  40,  48)
#define OSD_COLOR_BORDER      RGB( 48,  56,  62)
#define OSD_COLOR_TEXT        RGB(224, 224, 224)
#define OSD_COLOR_BAR         RGB( 64,  72,  80)
#define OSD_COLOR_BAR2        RGB(  0, 123, 167)
#define OSD_COLOR_CURSOR      RGB(192, 200, 208)
#define OSD_COLOR_DEBUGCLR    RGB(128, 136, 144)

COSD::COSD(CMainFrame* pMainFrame)
	: m_pMainFrame(pMainFrame)
	, m_pWndInsertAfter(&wndNoTopMost)
	, m_nDEFFLAGS(SWP_NOACTIVATE | SWP_NOREDRAW | SWP_ASYNCWINDOWPOS | SWP_NOZORDER)
{
	if (SysVersion::IsWin8orLater()) {
		// remove SWP_NOZORDER for Win 8 and later - it's use WS_CHILD style
		m_nDEFFLAGS &= ~SWP_NOZORDER;
		m_pWndInsertAfter = &wndTop;
	}

	m_penBorder.CreatePen(PS_SOLID, 1, OSD_COLOR_BORDER);
	m_brushCursor.CreateSolidBrush(OSD_COLOR_CURSOR);
	m_brushBack.CreateSolidBrush(OSD_COLOR_BACKGROUND);
	m_brushBar.CreateSolidBrush (OSD_COLOR_BAR);
	m_brushBar2.CreateSolidBrush (OSD_COLOR_BAR2);
	m_brushChapter.CreateSolidBrush(OSD_COLOR_CURSOR);
	m_debugBrushBack.CreateSolidBrush(OSD_COLOR_DEBUGCLR);
	m_debugPenBorder.CreatePen(PS_SOLID, 1, OSD_COLOR_BORDER);

	ZeroMemory(&m_BitmapInfo, sizeof(m_BitmapInfo));
}

COSD::~COSD()
{
	EndTimer();

	if (m_MemDC) {
		m_MemDC.DeleteDC();
	}

	if (m_pButtonImages) {
		delete m_pButtonImages;
	}
}

HRESULT COSD::Create(CWnd* pWnd)
{
	DWORD dwStyle	= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	DWORD dwStyleEx	= WS_EX_TRANSPARENT | WS_EX_LAYERED;

	if (SysVersion::IsWin8orLater()) {
		dwStyle		|= WS_CHILD;
	} else {
		dwStyle		|= WS_POPUP;
		dwStyleEx	|= WS_EX_TOPMOST;
	}

	if (!CreateEx(dwStyleEx, AfxRegisterWndClass(0), nullptr, dwStyle, RECT{0,0,0,0}, pWnd, 0, nullptr)) {
		DLog(L"Failed to create OSD Window");
		return E_FAIL;
	}

	const CAppSettings& s = AfxGetAppSettings();
	SetLayeredWindowAttributes(RGB(255, 0, 255), 255 - s.nOSDTransparent, LWA_ALPHA | LWA_COLORKEY);
	if (s.ShowOSD.Enable) {
		Start(pWnd);
	}

	return S_OK;
}

IMPLEMENT_DYNAMIC(COSD, CWnd)

BEGIN_MESSAGE_MAP(COSD, CWnd)
	ON_MESSAGE_VOID(WM_OSD_HIDE, OnHide)
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
	if (m_pWnd && m_pMFVMB) {
		if (m_bSeekBarVisible || m_bFlyBarVisible) {
			m_bCursorMoving		= false;
			m_bSeekBarVisible	= false;
			m_bFlyBarVisible	= false;
		}

		CalcSeekbar();
		CalcFlybar();

		InvalidateBitmapOSD();
		UpdateBitmap();
	}
	else if (m_pWnd) {
		//PostMessageW(WM_OSD_DRAW);
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

		hbmpRender = CreateDIBSection(m_MemDC, &bmi, DIB_RGB_COLORS, nullptr, nullptr, 0);
		m_MemDC.SelectObject(hbmpRender);

		if (::GetObjectW(hbmpRender, sizeof(BITMAP), &m_BitmapInfo) != 0) {
			if (m_pMFVMB) {
				ZeroMemory(&m_MFVAlphaBitmap, sizeof(m_MFVAlphaBitmap));
				m_MFVAlphaBitmap.GetBitmapFromDC  = TRUE;
				m_MFVAlphaBitmap.bitmap.hdc       = m_MemDC;
				m_MFVAlphaBitmap.params.dwFlags   = MFVideoAlphaBitmap_SrcColorKey;
				m_MFVAlphaBitmap.params.clrSrcKey = OSD_COLOR_TRANSPARENT;
				m_MFVAlphaBitmap.params.rcSrc     = m_rectWnd;
				m_MFVAlphaBitmap.params.nrcDest   = { 0, 0, 1, 1 };
				m_MFVAlphaBitmap.params.fAlpha    = 1.0;
			}

			m_MemDC.SetTextColor(OSD_COLOR_TEXT);
			m_MemDC.SetBkMode(TRANSPARENT);
		}

		DeleteObject(hbmpRender);
	}
}

void COSD::Reset()
{
	m_bShowMessage = true;
	m_bPeriodicallyDisplayed = false;

	m_MainWndRect.SetRectEmpty();
	m_strMessage.Empty();

	m_SeekbarFont.DeleteObject();

	CalcSeekbar();
	CalcFlybar();
}

void COSD::Start(CWnd* pWnd, IMFVideoMixerBitmap* pMFVMB)
{
	m_pMFVMB	= pMFVMB;
	m_pMVTO		= nullptr;
	m_pWnd		= pWnd;
	m_OSDType	= OSD_TYPE_BITMAP;

	UseCurentMonitorDPI(pWnd->GetSafeHwnd());
	UpdateButtonImages();
	CreateFontInternal();

	Reset();

	UpdateBitmap();
}

void COSD::Start(CWnd* pWnd, IMadVRTextOsd* pMVTO)
{
	m_pMFVMB	= nullptr;
	m_pMVTO		= pMVTO;
	m_pWnd		= pWnd;
	m_OSDType	= OSD_TYPE_MADVR;

	Reset();
}

void COSD::Start(CWnd* pWnd)
{
	m_pMFVMB	= nullptr;
	m_pMVTO		= nullptr;
	m_pWnd		= pWnd;
	m_OSDType	= OSD_TYPE_GDI;

	UseCurentMonitorDPI(pWnd->GetSafeHwnd());
	UpdateButtonImages();
	CreateFontInternal();

	Reset();
}

void COSD::Stop()
{
	EndTimer();

	m_bCursorMoving			= false;
	m_bSeekBarVisible		= false;
	m_bFlyBarVisible		= false;
	m_bMouseOverExitButton	= false;
	m_bMouseOverCloseButton	= false;

	ClearMessage();

	m_pMFVMB.Release();
	m_pMVTO.Release();
	m_pWnd = nullptr;

	Reset();
}

void COSD::CalcSeekbar()
{
	if (m_pWnd && m_pMFVMB) {
		SliderCursorHeight = ScaleY(SLIDER_CURSOR_HEIGHT);
		SliderCursorWidth  = ScaleX(SLIDER_CURSOR_WIDTH);
		SliderChapHeight   = ScaleY(SLIDER_CHAP_HEIGHT);
		SliderChapWidth    = ScaleY(SLIDER_CHAP_WIDTH);

		int SeekBarHeight   = ScaleY(SEEKBAR_HEIGHT);
		int SliderBarHeight = ScaleY(SLIDER_BAR_HEIGHT);
		int hor8 = ScaleX(8);

		m_pWnd->GetClientRect(&m_rectWnd);

		m_rectSeekBar.left   = m_rectWnd.left    + hor8;
		m_rectSeekBar.right  = m_rectWnd.right   - hor8;
		m_rectSeekBar.top    = m_rectWnd.bottom  - SeekBarHeight;
		m_rectSeekBar.bottom = m_rectSeekBar.top + SeekBarHeight;

		m_rectSlider.left   = m_rectSeekBar.left  + hor8;
		m_rectSlider.right  = m_rectSeekBar.right - hor8;
		m_rectSlider.top    = m_rectSeekBar.top   + (m_rectSeekBar.Height() - SliderBarHeight) / 2;
		m_rectSlider.bottom = m_rectSlider.top + SliderBarHeight;

		m_rectPosText.SetRect(m_rectSlider.left + m_rectSlider.Width() / 2, m_rectSeekBar.top, m_rectSlider.right, m_rectSlider.top);


		if (m_SeekbarTextHeight != m_rectPosText.Height() || !m_SeekbarFont.GetSafeHandle()) {
			m_SeekbarFont.DeleteObject();

			m_SeekbarTextHeight = m_rectPosText.Height();

			LOGFONTW lf = {};
			lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;
			lf.lfHeight = -(m_SeekbarTextHeight * 72 / 96);
			wcscpy_s(lf.lfFaceName, LF_FACESIZE, AfxGetAppSettings().strOSDFont);

			m_SeekbarFont.CreateFontIndirectW(&lf);
		}
	}
}

void COSD::CalcFlybar()
{
	if (m_pWnd) {
		m_pWnd->GetClientRect(&m_rectWnd);

		m_rectFlyBar.left        = m_rectWnd.left;
		m_rectFlyBar.right       = m_rectWnd.right;
		m_rectFlyBar.top         = m_rectWnd.top;
		m_rectFlyBar.bottom      = m_rectWnd.top           + ScaleY(100);

		m_rectExitButton.right   = m_rectWnd.right         - ScaleX(10);
		m_rectExitButton.top     = m_rectWnd.top           + ScaleY(10);
		m_rectExitButton.left    = m_rectExitButton.right  - m_nButtonHeight;
		m_rectExitButton.bottom  = m_rectExitButton.top    + m_nButtonHeight;

		m_rectCloseButton.right  = m_rectExitButton.left   - ScaleX(4);
		m_rectCloseButton.top    = m_rectExitButton.top;
		m_rectCloseButton.left   = m_rectCloseButton.right - m_nButtonHeight;
		m_rectCloseButton.bottom = m_rectCloseButton.top   + m_nButtonHeight;
	}
}

void COSD::DrawRect(CRect& rect, CBrush* pBrush, CPen* pPen)
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

void COSD::DrawSeekbar()
{
	m_rectCursor.left = m_rectSlider.left;
	if (m_llSeekStop > 0) {
		m_rectCursor.left += (long)((m_rectSlider.Width() - SliderCursorWidth) * m_llSeekPos / m_llSeekStop);
	}
	m_rectCursor.right  = m_rectCursor.left + SliderCursorWidth;
	m_rectCursor.top    = m_rectSeekBar.top + (m_rectSeekBar.Height() - SliderCursorHeight) / 2;
	m_rectCursor.bottom = m_rectCursor.top + SliderCursorHeight;

	DrawRect(m_rectSeekBar, &m_brushBack, &m_penBorder);
	CRect rect(m_rectSlider);
	rect.right = m_rectCursor.left+1;
	DrawRect(rect, &m_brushBar2);
	rect.left = m_rectCursor.right-1;
	rect.right = m_rectSlider.right;
	DrawRect(rect, &m_brushBar);

	if (m_SeekbarFont.GetSafeHandle()) {
		CStringW text = ReftimeToString2(m_llSeekPos, false);
		if (m_llSeekStop > 0) {
			text.Append(L" / ");
			text.Append(ReftimeToString2(m_llSeekStop, false));
		}

		m_MemDC.SelectObject(m_SeekbarFont);
		m_MemDC.SetTextColor(OSD_COLOR_CURSOR);
		m_MemDC.DrawText(text, &m_rectPosText, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	if (AfxGetAppSettings().fChapterMarker) {
		CAutoLock lock(&m_CBLock);

		if (m_pChapterBag && m_pChapterBag->ChapGetCount() > 1 && m_llSeekStop != 0) {
			REFERENCE_TIME rt;
			for (DWORD i = 0; i < m_pChapterBag->ChapGetCount(); ++i) {
				if (SUCCEEDED(m_pChapterBag->ChapGet(i, &rt, nullptr))) {
					__int64 pos = m_rectSlider.Width() * rt / m_llSeekStop;
					if (pos < 0) {
						continue;
					}

					CRect r;
					r.left   = m_rectSlider.left + (LONG)pos - SliderChapWidth / 2;
					r.top    = m_rectSeekBar.top + (m_rectSeekBar.Height() - SliderChapHeight) / 2;
					r.right  = r.left + SliderChapWidth;
					r.bottom = r.top + SliderChapHeight;

					DrawRect(r, &m_brushChapter);
				}
			}
		}
	}

	DrawRect(m_rectCursor, &m_brushCursor);
}

void COSD::DrawFlybar()
{
	const int nImageExit = m_bMouseOverExitButton ? IMG_EXIT_A : IMG_EXIT;
	const int nImageClose = m_bMouseOverCloseButton ? IMG_CLOSE_A : IMG_CLOSE;

	m_pButtonImages->Draw(&m_MemDC, nImageExit, m_rectExitButton.TopLeft(), ILD_NORMAL);
	m_pButtonImages->Draw(&m_MemDC, nImageClose, m_rectCloseButton.TopLeft(), ILD_NORMAL);
}

void COSD::DrawMessage()
{
	if (m_BitmapInfo.bmWidth * m_BitmapInfo.bmHeight * (m_BitmapInfo.bmBitsPixel >> 3) == 0) {
		return;
	}

	if (m_nMessagePos != OSD_NOMESSAGE) {
		CRect rectText;
		CRect rectMessages;

		m_MemDC.SelectObject(m_MainFont);

		m_MemDC.DrawText(m_strMessage, &rectText, DT_CALCRECT | DT_NOPREFIX);
		rectText.InflateRect(20, 10);
		switch (m_nMessagePos) {
			case OSD_TOPLEFT :
				rectMessages = CRect(10, 10, std::min((rectText.right + 10), (m_rectWnd.right - 10)), (rectText.bottom + 12));
				break;
			case OSD_TOPRIGHT :
			default :
				rectMessages = CRect(std::max(10L, m_rectWnd.right - 10 - rectText.Width()), 10, m_rectWnd.right-10, rectText.bottom + 10);
				break;
		}

		//m_MemDC.BeginPath();
		//m_MemDC.RoundRect(rectMessages.left, rectMessages.top, rectMessages.right, rectMessages.bottom, 10, 10);
		//m_MemDC.EndPath();
		//m_MemDC.SelectClipPath(RGN_COPY);

		GradientFill(&m_MemDC, &rectMessages);

		UINT uFormat = DT_LEFT|DT_VCENTER|DT_NOPREFIX;

		if (rectText.right + 10 >= (rectMessages.right)) {
			uFormat |= DT_END_ELLIPSIS;
		}

		const CAppSettings& s = AfxGetAppSettings();

		CRect r;
		if (s.bOSDFontShadow) {
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
	if (!m_debugMessages.empty()) {
		CString msg;
		auto it = m_debugMessages.cbegin();
		msg.Format(L"%s", *it++);

		while (it != m_debugMessages.cend()) {
			const CString& tmp = *it++;
			if (!tmp.IsEmpty()) {
				msg.AppendFormat(L"\r\n%s", tmp);
			}
		}

		m_MemDC.SelectObject(m_MainFont);

		CRect rectText;
		CRect rectMessages;
		m_MemDC.DrawText(msg, &rectText, DT_CALCRECT | DT_NOPREFIX);
		rectText.InflateRect(20, 10);

		int l, r, t, b;
		l = (m_rectWnd.Width() >> 1) - (rectText.Width() >> 1) - 10;
		r = (m_rectWnd.Width() >> 1) + (rectText.Width() >> 1) + 10;
		t = (m_rectWnd.Height() >> 1) - (rectText.Height() >> 1) - 10;
		b = (m_rectWnd.Height() >> 1) + (rectText.Height() >> 1) + 10;
		rectMessages = CRect(l, t, r, b);
		DrawRect(rectMessages, &m_debugBrushBack, &m_debugPenBorder);
		m_MemDC.DrawText(msg, &rectMessages, DT_CENTER | DT_VCENTER | DT_NOPREFIX);
	}
}

void COSD::InvalidateBitmapOSD()
{
	CAutoLock Lock(&m_Lock);

	if (!m_pMFVMB || !m_BitmapInfo.bmWidth || !m_BitmapInfo.bmHeight || !m_BitmapInfo.bmBitsPixel) {
		return;
	}

	fill_u32(m_BitmapInfo.bmBits, 0xff000000, m_BitmapInfo.bmWidth * m_BitmapInfo.bmHeight);

	if (m_bSeekBarVisible) {
		DrawSeekbar();
	}
	if (m_bFlyBarVisible) {
		DrawFlybar();
	}

	DrawMessage();
	DrawDebug();

	m_pMFVMB->SetAlphaBitmap(&m_MFVAlphaBitmap);

	m_pMainFrame->RepaintVideo(m_llSeekPos == m_llSeekStop);
}

void COSD::UpdateSeekBarPos(CPoint point)
{
	auto llSeekPos = m_llSeekStop * ((__int64)point.x - m_rectSlider.left) / ((__int64)m_rectSlider.Width() - SliderCursorWidth);
	llSeekPos = std::clamp(llSeekPos, 0ll, m_llSeekStop);

	if (llSeekPos != m_llSeekPos) {
		if (AfxGetAppSettings().fFastSeek ^ (GetKeyState(VK_SHIFT) < 0)) {
			m_pMainFrame->GetClosestKeyFrame(llSeekPos);
		}

		if (m_pWnd) {
			m_pMainFrame->SeekTo(llSeekPos);
		}
	}
}

bool COSD::OnMouseMove(UINT nFlags, CPoint point)
{
	bool bRet = false;

	if (m_pMFVMB) {
		if (m_bCursorMoving) {
			bRet = true;
			UpdateSeekBarPos(point);
			InvalidateBitmapOSD();
		} else if (m_rectSeekBar.PtInRect(point)) {
			bRet = true;
			if (!m_bSeekBarVisible) {
				if (m_pMainFrame->IsD3DFullScreenMode()) {
					m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_HAND);
				} else {
					SetCursor(LoadCursorW(nullptr, IDC_HAND));
				}
				m_bSeekBarVisible = true;
				InvalidateBitmapOSD();
			}
		} else if (m_rectFlyBar.PtInRect(point)) {
			bRet = true;
			if (!m_bFlyBarVisible) {
				if (m_pMainFrame->IsD3DFullScreenMode()) {
					m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_ARROW);
				} else {
					SetCursor(LoadCursorW(nullptr, IDC_ARROW));
				}
				m_bFlyBarVisible = true;
				InvalidateBitmapOSD();
			} else {
				if (!m_bMouseOverExitButton && m_rectExitButton.PtInRect(point)) {
					m_bMouseOverExitButton	= true;
					m_bMouseOverCloseButton	= false;
					InvalidateBitmapOSD();
				} else if (!m_bMouseOverCloseButton && m_rectCloseButton.PtInRect(point)) {
					m_bMouseOverExitButton	= false;
					m_bMouseOverCloseButton	= true;
					InvalidateBitmapOSD();
				} else if ((m_bMouseOverCloseButton && !m_rectCloseButton.PtInRect(point)) || (m_bMouseOverExitButton && !m_rectExitButton.PtInRect(point))) {
					m_bMouseOverExitButton	= false;
					m_bMouseOverCloseButton	= false;
					InvalidateBitmapOSD();
				}

				if (m_rectCloseButton.PtInRect(point) || m_rectExitButton.PtInRect(point)) {
					if (m_pMainFrame->IsD3DFullScreenMode()) {
						m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_HAND);
					} else {
						SetCursor(LoadCursorW(nullptr, IDC_HAND));
					}
				} else {
					if (m_pMainFrame->IsD3DFullScreenMode()) {
						m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_ARROW);
					} else {
						SetCursor(LoadCursorW(nullptr, IDC_ARROW));
					}
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
	if (m_pMainFrame->IsD3DFullScreenMode()) {
		m_pMainFrame->m_pFullscreenWnd->SetCursor(IDC_ARROW);
	} else {
		SetCursor(LoadCursorW(nullptr, IDC_ARROW));
	}

	const bool bHideBars = (m_pMFVMB && (m_bSeekBarVisible || m_bFlyBarVisible));

	m_bCursorMoving			= false;
	m_bSeekBarVisible		= false;
	m_bFlyBarVisible		= false;
	m_bMouseOverExitButton	= false;
	m_bMouseOverCloseButton	= false;

	if (bHideBars) {
		// Add new timer for removing any messages
		if (m_pWnd) {
			StartTimer(1000);
		}
		InvalidateBitmapOSD();
	}
}

bool COSD::OnLButtonDown(UINT nFlags, CPoint point)
{
	bool bRet = false;

	if (m_pMFVMB) {
		if (m_rectCursor.PtInRect(point)) {
			m_bCursorMoving		= true;
			bRet				= true;
		} else if (m_rectExitButton.PtInRect(point) || m_rectCloseButton.PtInRect(point)) {
			bRet				= true;
		} else if (m_rectSeekBar.PtInRect(point)) {
			m_bSeekBarVisible	= true;
			bRet				= true;
			UpdateSeekBarPos(point);
		}
	}

	return bRet;
}

bool COSD::OnLButtonUp(UINT nFlags, CPoint point)
{
	bool bRet = false;

	if (m_pMFVMB) {
		m_bCursorMoving = false;

		if (m_rectFlyBar.PtInRect(point)) {
			if (m_rectExitButton.PtInRect(point)) {
				m_pMainFrame->PostMessageW(WM_COMMAND, ID_FILE_EXIT);
			}

			if (m_rectCloseButton.PtInRect(point)) {
				m_pMainFrame->PostMessageW(WM_COMMAND, ID_FILE_CLOSEPLAYLIST);
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

__int64 COSD::GetRange() const
{
	return m_llSeekStop;
}

void COSD::SetPos(__int64 pos)
{
	SetPosAndRange(pos, m_llSeekStop);
}

void COSD::SetRange(__int64 stop)
{
	SetPosAndRange(m_llSeekPos, stop);
}

void COSD::SetPosAndRange(__int64 pos, __int64 stop)
{
	const auto bUpdateSeekBar = (m_bSeekBarVisible && (m_llSeekPos != pos || m_llSeekStop != stop));
	m_llSeekPos = pos;
	m_llSeekStop = stop;
	if (bUpdateSeekBar) {
		InvalidateBitmapOSD();
	}
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

	if (m_pMFVMB) {
		m_pMFVMB->ClearAlphaBitmap();
		DLog(L"IMFVideoMixerBitmap::ClearAlphaBitmap");
		m_pMainFrame->RepaintVideo(); //???
	} else if (m_pMVTO) {
		m_pMVTO->OsdClearMessage();
	} else if (::IsWindow(m_hWnd) && IsWindowVisible()) {
		PostMessageW(WM_OSD_HIDE);
	}
}

void COSD::DisplayMessage(
	OSD_MESSAGEPOS nPos,
	LPCWSTR strMsg,
	int nDuration/* = 5000*/,
	const bool bPeriodicallyDisplayed/* = false*/,
	const int FontSize/* = 0*/,
	LPCWSTR OSD_Font/* = nullptr*/)
{
	if (!m_bShowMessage) {
		return;
	}

	const CAppSettings& s = AfxGetAppSettings();

	if (m_pMFVMB) {
		if (bPeriodicallyDisplayed && !m_bPeriodicallyDisplayed && m_bTimerStarted) {
			return;
		}

		if (nPos != OSD_DEBUG) {
			m_nMessagePos = nPos;
			m_strMessage  = strMsg;
		} else {
			m_debugMessages.emplace_back(strMsg);
			if (m_debugMessages.size() > 20) {
				m_debugMessages.pop_front();
			}
			nDuration = -1;
		}

		m_FontSize = FontSize ? std::clamp(FontSize, 8, 40) : s.nOSDSize;
		if (OSD_Font) {
			m_OSD_Font = OSD_Font;
		} else {
			m_OSD_Font = s.strOSDFont;
		}

		if (m_OSD_FontCashed != m_OSD_Font
				|| m_FontSizeCashed != m_FontSize
				|| m_bFontAACashed != s.bOSDFontAA
				|| !m_MainFont.GetSafeHandle()) {
			CreateFontInternal();
		}

		m_OSD_FontCashed = m_OSD_Font;
		m_FontSizeCashed = m_FontSize;
		m_bFontAACashed = s.bOSDFontAA;

		if (m_pWnd) {
			EndTimer();
			if (nDuration != -1) {
				StartTimer(nDuration);
			}
		}

		m_bPeriodicallyDisplayed = bPeriodicallyDisplayed;

		InvalidateBitmapOSD();
	} else if (m_pMVTO) {
		m_pMVTO->OsdDisplayMessage(strMsg, nDuration);
	} else if (m_pWnd) {
		if (bPeriodicallyDisplayed && !m_bPeriodicallyDisplayed && m_bTimerStarted) {
			return;
		}

		if (nPos != OSD_DEBUG) {
			m_nMessagePos = nPos;
			m_strMessage  = strMsg;
		}

		m_FontSize = FontSize ? std::clamp(FontSize, 8, 40) : s.nOSDSize;
		if (OSD_Font) {
			m_OSD_Font = OSD_Font;
		} else {
			m_OSD_Font = s.strOSDFont;
		}

		EndTimer();
		if (nDuration != -1) {
			StartTimer(nDuration);
		}

		SetWindowPos(m_pWndInsertAfter, 0, 0, 0, 0, m_nDEFFLAGS | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		//PostMessageW(WM_OSD_DRAW);
		DrawWnd();

		m_bPeriodicallyDisplayed = bPeriodicallyDisplayed;
	}
}

void COSD::DebugMessage(LPCWSTR format, ...)
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
	if (m_pMFVMB) {
		if (hide) {
			ClearMessage(true);
		} else {
			InvalidateBitmapOSD();
		}
	} else {
		if (hide) {
			ClearMessage(true);
		} else {
			if (!m_strMessage.IsEmpty()) {
				SetWindowPos(m_pWndInsertAfter, 0, 0, 0, 0, m_nDEFFLAGS | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			}
			//PostMessageW(WM_OSD_DRAW);
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

	//PostMessageW(WM_OSD_DRAW);
	DrawWnd();
}

void COSD::DrawWnd()
{
	CAutoLock Lock(&m_Lock);

	if (m_nMessagePos == OSD_NOMESSAGE) {
		if (IsWindowVisible()) {
			ShowWindow(SW_HIDE);
		}
		return;
	}

	if (!IsWindowVisible() || !m_pWnd || m_OSDType != OSD_TYPE_GDI || m_strMessage.IsEmpty()) {
		return;
	}

	const CAppSettings& s = AfxGetAppSettings();

	if (m_MainWndRectCashed == m_MainWndRect
			&& m_strMessageCashed == m_strMessage
			&& m_nMessagePosCashed == m_nMessagePos
			&& m_OSD_FontCashed == m_OSD_Font
			&& m_FontSizeCashed == m_FontSize
			&& m_bFontAACashed == s.bOSDFontAA) {
		return;
	}

	m_MainWndRectCashed = m_MainWndRect;
	m_strMessageCashed = m_strMessage;
	m_nMessagePosCashed = m_nMessagePos;

	if (m_OSD_FontCashed != m_OSD_Font
			|| m_FontSizeCashed != m_FontSize
			|| m_bFontAACashed != s.bOSDFontAA
			|| !m_MainFont.GetSafeHandle()) {
		CreateFontInternal();
	}

	m_OSD_FontCashed = m_OSD_Font;
	m_FontSizeCashed = m_FontSize;
	m_bFontAACashed = s.bOSDFontAA;

	CClientDC dc(this);

	CDC temp_DC;
	temp_DC.CreateCompatibleDC(&dc);
	CBitmap temp_BM;
	temp_BM.CreateCompatibleBitmap(&temp_DC, m_MainWndRect.Width(), m_MainWndRect.Height());
	CBitmap* temp_pOldBmt = temp_DC.SelectObject(&temp_BM);

	temp_DC.SelectObject(m_MainFont);

	LONG messageWidth = 0;
	if (m_strMessage.Find(L'\n') == -1) {
		messageWidth = temp_DC.GetTextExtent(m_strMessage).cx;
	} else {
		std::list<CString> strings;
		ExplodeEsc(m_strMessage, strings, L'\n');
		for (const auto& str : strings) {
			messageWidth = std::max(messageWidth, temp_DC.GetTextExtent(str).cx);
		}
	}

	CRect rectText;
	temp_DC.DrawText(m_strMessage, &rectText, DT_CALCRECT | DT_NOPREFIX);
	rectText.right = messageWidth;
	rectText.InflateRect(0, 0, 10, 10);

	CRect rectMessages;
	switch (m_nMessagePos) {
		case OSD_TOPLEFT :
			rectMessages = CRect(0, 0, std::min((rectText.right + 10), (LONG)m_MainWndRect.Width() - 20), std::min((rectText.bottom + 2), (LONG)m_MainWndRect.Height() - 20));
			break;
		case OSD_TOPRIGHT :
		default :
			const int imax = std::max(0, m_MainWndRect.Width() - rectText.Width() - 30);
			rectMessages = CRect(imax, 0, (m_MainWndRect.Width() - 20) + imax, std::min((rectText.bottom + 2), (LONG)m_MainWndRect.Height() - 20));
			break;
	}

	temp_DC.SelectObject(temp_pOldBmt);
	temp_BM.DeleteObject();
	temp_DC.DeleteDC();

	CRect wr(m_MainWndRect.left + 10 + rectMessages.left, m_MainWndRect.top + 10, rectMessages.Width() - rectMessages.left, rectMessages.Height());
	if (SysVersion::IsWin8orLater()) {
		wr.left	-= m_MainWndRect.left;
		wr.top	-= m_MainWndRect.top;
	}
	SetWindowPos(nullptr, wr.left, wr.top, wr.right, wr.bottom, m_nDEFFLAGS | SWP_NOZORDER);

	CRect rcBar;
	GetClientRect(&rcBar);

	CDC mdc;
	mdc.CreateCompatibleDC(&dc);
	CBitmap bm;
	bm.CreateCompatibleBitmap(&dc, rcBar.Width(), rcBar.Height());
	CBitmap* pOldBm = mdc.SelectObject(&bm);
	mdc.SetBkMode(TRANSPARENT);

	mdc.SelectObject(m_MainFont);

	GradientFill(&mdc, &rcBar);

	const UINT uFormat = DT_LEFT | DT_TOP | DT_END_ELLIPSIS | DT_NOPREFIX;

	CRect r;

	if (s.bOSDFontShadow) {
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
	mdc.DrawText(m_strMessage, &r, uFormat);

	dc.BitBlt(0, 0, rcBar.Width(), rcBar.Height(), &mdc, 0, 0, SRCCOPY);

	mdc.SelectObject(pOldBm);
	bm.DeleteObject();
	mdc.DeleteDC();
}

void COSD::SetChapterBag(CComPtr<IDSMChapterBag>& pCB)
{
	CAutoLock lock(&m_CBLock);

	if (pCB) {
		m_pChapterBag = pCB;
	}
}

void COSD::RemoveChapters()
{
	CAutoLock lock(&m_CBLock);
	m_pChapterBag.Release();
}

void COSD::OverrideDPI(int dpix, int dpiy)
{
	CDPI::OverrideDPI(dpix, dpiy);

	UpdateButtonImages();

	m_FontSizeCashed = 0;
	DrawWnd();
}

bool COSD::UpdateButtonImages()
{
	if (m_pMainFrame->m_svgFlybar.IsLoad()) {
		int w = 0;
		int h = ScaleY(24);
		if (h != m_externalFlyBarHeight) {
			m_externalFlyBarHeight = h;
			if (HBITMAP hBitmap = m_pMainFrame->m_svgFlybar.Rasterize(w, h)) {
				if (w == h * 25) {
					SAFE_DELETE(m_pButtonImages);
					m_pButtonImages = DNew CImageList();
					m_pButtonImages->Create(h, h, ILC_COLOR32 | ILC_MASK, 1, 0);
					ImageList_Add(m_pButtonImages->GetSafeHandle(), hBitmap, nullptr);

					m_nButtonHeight = h;
				}
				DeleteObject(hBitmap);
			}

			if (m_pButtonImages) {
				return true;
			}
		}
	}

	return false;
}

void COSD::GradientFill(CDC* pDc, CRect* rc)
{
	const CAppSettings& s = AfxGetAppSettings();

	int R, G, B, R1, G1, B1, R_, G_, B_, R1_, G1_, B1_;
	R   = GetRValue(s.clrGrad1ABGR);
	G   = GetGValue(s.clrGrad1ABGR);
	B   = GetBValue(s.clrGrad1ABGR);
	R1  = GetRValue(s.clrGrad2ABGR);
	G1  = GetGValue(s.clrGrad2ABGR);
	B1  = GetBValue(s.clrGrad2ABGR);
	R_  = std::min(R + 32, 255);
	R1_ = std::min(R1 + 32, 255);
	G_  = std::min(G + 32, 255);
	G1_ = std::min(G1 + 32, 255);
	B_  = std::min(B + 32, 255);
	B1_ = std::min(B1 + 32, 255);

	int nOSDTransparent	= s.nOSDTransparent;
	int nOSDBorder		= s.nOSDBorder;

	GRADIENT_RECT gr = {0, 1};
	TRIVERTEX tv[2] = {
		{rc->left, rc->top, R * 256, G * 256, B * 256, nOSDTransparent * 256},
		{rc->right, rc->bottom, R1 * 256, G1 * 256, B1 * 256, nOSDTransparent * 256},
	};
	pDc->GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);

	if (nOSDBorder > 0) {
		TRIVERTEX tv2[2] = {
			{rc->left, rc->top, R_ * 256, G_ * 256, B_ * 256, nOSDTransparent * 256},
			{rc->left + nOSDBorder, rc->bottom, R1_ * 256, G1_ * 256, B1_ * 256, nOSDTransparent * 256},
		};
		pDc->GradientFill(tv2, 2, &gr, 1, GRADIENT_FILL_RECT_V);

		TRIVERTEX tv3[2] = {
			{rc->right, rc->top, R_ * 256, G_ * 256, B_ * 256, nOSDTransparent * 256},
			{rc->right - nOSDBorder, rc->bottom, R1_ * 256, G1_ * 256, B1_ * 256, nOSDTransparent * 256},
		};
		pDc->GradientFill(tv3, 2, &gr, 1, GRADIENT_FILL_RECT_V);

		TRIVERTEX tv4[2] = {
			{rc->left, rc->top, R_ * 256, G_ * 256, B_ * 256, nOSDTransparent * 256},
			{rc->right, rc->top + nOSDBorder, R_ * 256, G_ * 256, B_ * 256, nOSDTransparent * 256},
		};
		pDc->GradientFill(tv4, 2, &gr, 1, GRADIENT_FILL_RECT_V);

		TRIVERTEX tv5[2] = {
			{rc->left, rc->bottom, R1_ * 256, G1_ * 256, B1_ * 256, nOSDTransparent * 256},
			{rc->right, rc->bottom - nOSDBorder, R1_ * 256, G1_ * 256, B1_ * 256, nOSDTransparent * 256},
		};
		pDc->GradientFill(tv5, 2, &gr, 1, GRADIENT_FILL_RECT_V);
	}
}

void COSD::CreateFontInternal()
{
	if (m_MainFont.GetSafeHandle()) {
		m_MainFont.DeleteObject();
	}

	LOGFONTW lf = {};
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;
	lf.lfHeight = -PointsToPixels(m_FontSize);
	lf.lfQuality = AfxGetAppSettings().bOSDFontAA ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, m_OSD_Font);

	m_MainFont.CreateFontIndirectW(&lf);
}

void COSD::StartTimer(const UINT nTimerDurarion)
{
	EndTimer();

	if (m_pWnd) {
		m_pWnd->SetTimer((UINT_PTR)this, nTimerDurarion,
			[](HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime) {
				if (auto pOSD = reinterpret_cast<COSD*>(nIDEvent)) {
					pOSD->ClearMessage();
					pOSD->EndTimer();
				}
			});
		m_bTimerStarted = true;
	}
}

void COSD::EndTimer()
{
	if (m_bTimerStarted) {
		if (m_pWnd) {
			m_pWnd->KillTimer(reinterpret_cast<UINT_PTR>(this));
		}
		m_bTimerStarted = false;
	}
}
