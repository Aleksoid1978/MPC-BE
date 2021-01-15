/*
 * (C) 2006-2021 see Authors.txt
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

#pragma once

#include <atlbase.h>
#include <d3d9.h>
#include <evr9.h>
#include <mvrInterfaces.h>
#include <HighDPI.h>
#include "../../DSUtil/DSMPropertyBag.h"

#define WM_HIDE			(WM_USER + 1001)
#define WM_OSD_DRAW		(WM_USER + 1002)

enum OSD_COLORS {
	OSD_TRANSPARENT,
	OSD_BACKGROUND,
	OSD_BORDER,
	OSD_TEXT,
	OSD_BAR,
	OSD_CURSOR,
	OSD_DEBUGCLR,
	OSD_LAST
};

enum OSD_MESSAGEPOS {
	OSD_NOMESSAGE,
	OSD_TOPLEFT,
	OSD_TOPRIGHT,
	OSD_DEBUG,
};

enum OSD_TYPE {
	OSD_TYPE_NONE,
	OSD_TYPE_BITMAP,
	OSD_TYPE_MADVR,
	OSD_TYPE_GDI
};

class CMainFrame;

class COSD : public CWnd, public CDPI
{
	enum :int {
		IMG_EXIT    = 0,
		IMG_EXIT_A  = 1,
		IMG_CLOSE   = 23,
		IMG_CLOSE_A = 24,
	};

public:
	COSD(CMainFrame* pMainFrame);
	~COSD();

	HRESULT Create(CWnd* pWnd);

	void Start(CWnd* pWnd, IMFVideoMixerBitmap* pMFVMB);
	void Start(CWnd* pWnd, IMadVRTextOsd* pMVTO);
	void Start(CWnd* pWnd);
	void Stop();

	void DisplayMessage(OSD_MESSAGEPOS nPos, LPCWSTR strMsg, int nDuration = 5000, const bool bPeriodicallyDisplayed = false, const int FontSize = 0, LPCWSTR OSD_Font = nullptr);
	void DebugMessage(LPCWSTR format, ...);
	void ClearMessage(bool hide = false);

	void HideMessage(bool hide);
	void EnableShowMessage(bool bValue = true) { m_bShowMessage = bValue; };

	__int64 GetPos() const;
	__int64 GetRange() const;
	void SetPos(__int64 pos);
	void SetRange(__int64 stop);
	void SetPosAndRange(__int64 pos, __int64 stop);

	void OnSize(UINT nType, int cx, int cy);
	bool OnMouseMove(UINT nFlags, CPoint point);
	void OnMouseLeave();
	bool OnLButtonDown(UINT nFlags, CPoint point);
	bool OnLButtonUp(UINT nFlags, CPoint point);

	void SetWndRect(CRect rc) { m_MainWndRect = rc; };

	OSD_TYPE GetOSDType() { return m_OSDType; };

	void SetChapterBag(CComPtr<IDSMChapterBag>& pCB);
	void RemoveChapters();

	void OverrideDPI(int dpix, int dpiy);
	bool UpdateButtonImages();

	DECLARE_DYNAMIC(COSD)

private:
	CComPtr<IMFVideoMixerBitmap> m_pMFVMB;
	CComPtr<IMadVRTextOsd>       m_pMVTO;

	CMainFrame*			m_pMainFrame;
	CWnd*				m_pWnd;

	CCritSec			m_Lock;
	CDC					m_MemDC;
	MFVideoAlphaBitmap	m_MFVAlphaBitmap;
	BITMAP				m_BitmapInfo;

	CFont	m_MainFont;
	CPen	m_penBorder;
	CPen	m_penCursor;
	CBrush	m_brushBack;
	CBrush	m_brushBar;
	CBrush	m_brushChapter;
	CPen	m_debugPenBorder;
	CBrush	m_debugBrushBack;
	int		m_FontSize;
	CString	m_OSD_Font;

	CRect		m_rectWnd;
	COLORREF	m_Color[OSD_LAST];

	CRect	m_rectSeekBar;
	CRect	m_rectFlyBar;
	CRect	m_rectCloseButton;
	CRect	m_rectExitButton;

	CImageList *m_pButtonImages = nullptr;
	int		m_nButtonHeight;
	int		m_externalFlyBarHeight = 0;

	CRect	m_rectCursor;
	CRect	m_rectBar;
	bool	m_bCursorMoving;
	bool	m_bSeekBarVisible;
	bool	m_bFlyBarVisible;
	bool	m_bMouseOverCloseButton;
	bool	m_bMouseOverExitButton;

	__int64	m_llSeekStop = 0;
	__int64	m_llSeekPos = 0;

	bool	m_bShowMessage;

	CRect	m_MainWndRect;

	const CWnd*		m_pWndInsertAfter;

	OSD_TYPE		m_OSDType;

	CString			m_strMessage;
	OSD_MESSAGEPOS	m_nMessagePos;
	std::list<CString> m_debugMessages;

	CCritSec				m_CBLock;
	CComPtr<IDSMChapterBag> m_pChapterBag;

	UINT m_nDEFFLAGS;

	CRect          m_MainWndRectCashed;
	CString        m_strMessageCashed;
	OSD_MESSAGEPOS m_nMessagePosCashed = OSD_NOMESSAGE;
	CString        m_OSD_FontCashed;
	int            m_FontSizeCashed = 0;
	bool           m_bFontAACashed = false;

	void UpdateBitmap();
	void CalcRect();
	void UpdateSeekBarPos(CPoint point);
	void DrawSlider();
	void DrawFlyBar();
	void DrawRect(CRect& rect, CBrush* pBrush = nullptr, CPen* pPen = nullptr);
	void InvalidateBitmapOSD();
	void DrawMessage();
	void DrawDebug();

	void Reset();

	void DrawWnd();

	void GradientFill(CDC* pDc, CRect* rc);

	int SeekBarHeight      = 0;
	int SliderBarHeight    = 0;
	int SliderCursorHeight = 0;
	int SliderCursorWidth  = 0;
	int SliderChapHeight   = 0;
	int SliderChapWidth    = 0;

	void CreateFontInternal();

	bool m_bPeriodicallyDisplayed = false;
	std::mutex m_mutexTimer;
	HANDLE m_hTimerHandle = nullptr;
	BOOL StartTimer(const DWORD dueTime);
	void EndTimer(const bool bWaitForCallback = true);
	static void CALLBACK TimerCallbackFunc(PVOID lpParameter, BOOLEAN TimerOrWaitFired);

protected:
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	BOOL PreTranslateMessage(MSG* pMsg);
	void OnPaint();
	BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnHide();
	afx_msg void OnDrawWnd();
};
