/********************************************************************
*
* Copyright (c) 2002 Sven Wiegand <mail@sven-wiegand.de>
*
* You can use this and modify this in any way you want,
* BUT LEAVE THIS HEADER INTACT.
*
* Redistribution is appreciated.
*
* $Workfile:$
* $Revision:$
* $Modtime:$
* $Author:$
*
* Revision History:
*	$History:$
*
*********************************************************************/

#include "stdafx.h"
#include "PropPageFrameDefault.h"
// <MPC-BE Custom Code>
#include "../../../DSUtil/SysVersion.h"
// </MPC-BE Custom Code>


namespace TreePropSheet
{


//uncomment the following line, if you don't have installed the
//new platform SDK
#define XPSUPPORT

#ifdef XPSUPPORT
#include <uxtheme.h>
#include <vssym32.h>
#endif

//-------------------------------------------------------------------
// class CThemeLib
//-------------------------------------------------------------------

#define THEMEAPITYPE(f)					typedef HRESULT (__stdcall *_##f)
#define THEMEAPITYPE_(t, f)			typedef t (__stdcall *_##f)
#define THEMEAPIPTR(f)					_##f m_p##f

#ifdef XPSUPPORT
	#define THEMECALL(f)						return (*m_p##f)
	#define GETTHEMECALL(f)					m_p##f = (_##f)GetProcAddress(m_hThemeLib, #f)
#else
	void ThemeDummy(...) {ASSERT(FALSE);}
	#define HTHEME									void*
	#define TABP_PANE								0
	#define THEMECALL(f)						return 0; ThemeDummy
	#define GETTHEMECALL(f)					m_p##f = NULL
#endif


/**
Helper class for loading the uxtheme DLL and providing their
functions.

One global object of this class exists.

@author Sven Wiegand
*/
class CThemeLib
{
// construction/destruction
public:
	CThemeLib();
	~CThemeLib();

// operations
public:
	/**
	Returns TRUE if the call wrappers are available, FALSE otherwise.
	*/
	BOOL IsAvailable() const;

// call wrappers
public:
	BOOL IsThemeActive() const
	{THEMECALL(IsThemeActive)();}

	HTHEME OpenThemeData(HWND hwnd, LPCWSTR pszClassList) const
	{THEMECALL(OpenThemeData)(hwnd, pszClassList);}

	HRESULT CloseThemeData(HTHEME hTheme) const
	{THEMECALL(CloseThemeData)(hTheme);}

	HRESULT GetThemeBackgroundContentRect(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId,  const RECT *pBoundingRect, OUT RECT *pContentRect) const
	{THEMECALL(GetThemeBackgroundContentRect)(hTheme, hdc, iPartId, iStateId, pBoundingRect, pContentRect);}

	HRESULT DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, OPTIONAL const RECT *pClipRect) const
	{THEMECALL(DrawThemeBackground)(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);}

	HRESULT DrawThemeEdge(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, UINT uEdge, UINT uFlags, OPTIONAL const RECT * pContentRect) const
	{THEMECALL(DrawThemeEdge)(hTheme, hdc, iPartId, iStateId, pRect, uEdge, uFlags, pContentRect);}

// function pointers
private:
#ifdef XPSUPPORT
	THEMEAPITYPE_(BOOL, IsThemeActive)();
	THEMEAPIPTR(IsThemeActive);

	THEMEAPITYPE_(HTHEME, OpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
	THEMEAPIPTR(OpenThemeData);

	THEMEAPITYPE(CloseThemeData)(HTHEME hTheme);
	THEMEAPIPTR(CloseThemeData);

	THEMEAPITYPE(GetThemeBackgroundContentRect)(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId,  const RECT *pBoundingRect, OUT RECT *pContentRect);
	THEMEAPIPTR(GetThemeBackgroundContentRect);

	THEMEAPITYPE(DrawThemeBackground)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, OPTIONAL const RECT *pClipRect);
	THEMEAPIPTR(DrawThemeBackground);

	THEMEAPITYPE(DrawThemeEdge)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pRect, UINT uEdge, UINT uFlags, OPTIONAL const RECT* pContentRect);
	THEMEAPIPTR(DrawThemeEdge);
#endif

// properties
private:
	/** instance handle to the library or NULL. */
	HINSTANCE m_hThemeLib;
};

/**
One and only instance of CThemeLib.
*/
static CThemeLib g_ThemeLib;


CThemeLib::CThemeLib()
:	m_hThemeLib(NULL)
{
#ifdef XPSUPPORT
	m_hThemeLib = LoadLibrary(_T("uxtheme.dll"));
	if (!m_hThemeLib)
		return;

	GETTHEMECALL(IsThemeActive);
	GETTHEMECALL(OpenThemeData);
	GETTHEMECALL(CloseThemeData);
	GETTHEMECALL(GetThemeBackgroundContentRect);
	GETTHEMECALL(DrawThemeBackground);
	GETTHEMECALL(DrawThemeEdge);
#endif
}


CThemeLib::~CThemeLib()
{
	if (m_hThemeLib)
		FreeLibrary(m_hThemeLib);
}


BOOL CThemeLib::IsAvailable() const
{
	return m_hThemeLib!=NULL;
}


//-------------------------------------------------------------------
// class CPropPageFrameDefault
//-------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CPropPageFrameDefault, CWnd)
	//{{AFX_MSG_MAP(CPropPageFrameDefault)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CPropPageFrameDefault::CPropPageFrameDefault()
{
}


CPropPageFrameDefault::~CPropPageFrameDefault()
{
	if (m_Images.GetSafeHandle())
		m_Images.DeleteImageList();
}


/////////////////////////////////////////////////////////////////////
// Overridings

BOOL CPropPageFrameDefault::Create(DWORD dwWindowStyle, const RECT &rect, CWnd *pwndParent, UINT nID)
{
	return CWnd::Create(
		AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW, AfxGetApp()->LoadStandardCursor(IDC_ARROW), GetSysColorBrush(COLOR_3DFACE)),
		_T("Page Frame"),
		dwWindowStyle, rect, pwndParent, nID);
}


CWnd* CPropPageFrameDefault::GetWnd()
{
	return static_cast<CWnd*>(this);
}


void CPropPageFrameDefault::SetCaption(LPCTSTR lpszCaption, HICON hIcon /*= NULL*/)
{
	CPropPageFrame::SetCaption(lpszCaption, hIcon);

	// build image list
	if (m_Images.GetSafeHandle())
		m_Images.DeleteImageList();
	if (hIcon)
	{
		ICONINFO	ii;
		if (!GetIconInfo(hIcon, &ii))
			return;

		CBitmap	bmMask;
		bmMask.Attach(ii.hbmMask);
		if (ii.hbmColor) DeleteObject(ii.hbmColor);

		BITMAP	bm;
		bmMask.GetBitmap(&bm);

		if (!m_Images.Create(bm.bmWidth, bm.bmHeight, ILC_COLOR32|ILC_MASK, 0, 1))
			return;

		if (m_Images.Add(hIcon) == -1)
			m_Images.DeleteImageList();
	}
}


CRect CPropPageFrameDefault::CalcMsgArea()
{
	CRect	rect;
	GetClientRect(rect);
	if (g_ThemeLib.IsAvailable() && g_ThemeLib.IsThemeActive())
	{
		HTHEME	hTheme = g_ThemeLib.OpenThemeData(m_hWnd, L"Tab");
		if (hTheme)
		{
			CRect	rectContent;
			CDC		*pDc = GetDC();
			g_ThemeLib.GetThemeBackgroundContentRect(hTheme, pDc->m_hDC, TABP_PANE, 0, rect, rectContent);
			ReleaseDC(pDc);
			g_ThemeLib.CloseThemeData(hTheme);

			if (GetShowCaption())
				rectContent.top = rect.top+GetCaptionHeight()+1;
			rect = rectContent;
		}
	}
	else if (GetShowCaption())
		rect.top+= GetCaptionHeight()+1;

	return rect;
}


CRect CPropPageFrameDefault::CalcCaptionArea()
{
	CRect	rect;
	GetClientRect(rect);
	if (g_ThemeLib.IsAvailable() && g_ThemeLib.IsThemeActive())
	{
		HTHEME	hTheme = g_ThemeLib.OpenThemeData(m_hWnd, L"Tab");
		if (hTheme)
		{
			CRect	rectContent;
			CDC		*pDc = GetDC();
			g_ThemeLib.GetThemeBackgroundContentRect(hTheme, pDc->m_hDC, TABP_PANE, 0, rect, rectContent);
			ReleaseDC(pDc);
			g_ThemeLib.CloseThemeData(hTheme);

			if (GetShowCaption())
				rectContent.bottom = rect.top + GetCaptionHeight();
			else
				rectContent.bottom = rectContent.top;

			rect = rectContent;
		}
	}
	else
	{
		if (GetShowCaption())
			rect.bottom = rect.top+GetCaptionHeight();
		else
			rect.bottom = rect.top;
	}

	return rect;
}

void CPropPageFrameDefault::DrawCaption(CDC *pDc, CRect rect, LPCTSTR lpszCaption, HICON hIcon)
{
	// <MPC-BE Custom Code>
	COLORREF	clrLeft = GetSysColor(COLOR_ACTIVECAPTION);
	// </MPC-BE Custom Code>
	COLORREF	clrRight = pDc->GetPixel(rect.right-1, rect.top);
	FillGradientRectH(pDc, rect, clrLeft, clrRight);

	// draw icon
	if (hIcon && m_Images.GetSafeHandle() && m_Images.GetImageCount() == 1)
	{
		IMAGEINFO	ii;
		m_Images.GetImageInfo(0, &ii);
		CPoint		pt(3, rect.CenterPoint().y - (ii.rcImage.bottom-ii.rcImage.top)/2);
		m_Images.Draw(pDc, 0, pt, ILD_TRANSPARENT);
		rect.left+= (ii.rcImage.right-ii.rcImage.left) + 3;
	}

	// draw text
	rect.left += 2;

	COLORREF	clrPrev = pDc->SetTextColor(GetSysColor(COLOR_CAPTIONTEXT));
	int				nBkStyle = pDc->SetBkMode(TRANSPARENT);
	CFont			*pFont = (CFont*)pDc->SelectStockObject(SYSTEM_FONT);

	// <MPC-BE Custom Code>
	auto GetNonClientMetrics = [](NONCLIENTMETRICSW* ncm) {
		ncm->cbSize = sizeof(NONCLIENTMETRICSW);
		VERIFY(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm->cbSize, ncm, 0));
	};

	NONCLIENTMETRICSW ncm{};
	GetNonClientMetrics(&ncm);
	auto& lf = ncm.lfMessageFont;
	lf.lfHeight = static_cast<long>(-.8f * rect.Height());
	lf.lfWeight = FW_BOLD;

	CFont f;
	f.CreateFontIndirectW(&lf);
	pFont = pDc->SelectObject(&f);

	TEXTMETRICW GDIMetrics;
	GetTextMetricsW(pDc->GetSafeHdc(), &GDIMetrics);
	while (GDIMetrics.tmHeight > rect.Height() && abs(lf.lfHeight) > 10) {
		pDc->SelectObject(pFont);
		f.DeleteObject();
		lf.lfHeight++;
		f.CreateFontIndirectW(&lf);
		pDc->SelectObject(&f);
		GetTextMetricsW(pDc->GetSafeHdc(), &GDIMetrics);
	}
	rect.top -= GDIMetrics.tmDescent - 1;
	// <MPC-BE Custom Code>

	pDc->DrawTextW(lpszCaption, rect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

	pDc->SetTextColor(clrPrev);
	pDc->SetBkMode(nBkStyle);
	pDc->SelectObject(pFont);
}


/////////////////////////////////////////////////////////////////////
// Implementation helpers

void CPropPageFrameDefault::FillGradientRectH(CDC *pDc, const RECT &rect, COLORREF clrLeft, COLORREF clrRight)
{
	// pre calculation
	int	nSteps = rect.right-rect.left;
	int	nRRange = GetRValue(clrRight)-GetRValue(clrLeft);
	int	nGRange = GetGValue(clrRight)-GetGValue(clrLeft);
	int	nBRange = GetBValue(clrRight)-GetBValue(clrLeft);

	double	dRStep = (double)nRRange/(double)nSteps;
	double	dGStep = (double)nGRange/(double)nSteps;
	double	dBStep = (double)nBRange/(double)nSteps;

	double	dR = (double)GetRValue(clrLeft);
	double	dG = (double)GetGValue(clrLeft);
	double	dB = (double)GetBValue(clrLeft);

	CPen	*pPrevPen = NULL;
	for (int x = rect.left; x <= rect.right; ++x)
	{
		CPen	Pen(PS_SOLID, 1, RGB((BYTE)dR, (BYTE)dG, (BYTE)dB));
		pPrevPen = pDc->SelectObject(&Pen);
		pDc->MoveTo(x, rect.top);
		pDc->LineTo(x, rect.bottom);
		pDc->SelectObject(pPrevPen);

		dR+= dRStep;
		dG+= dGStep;
		dB+= dBStep;
	}
}


/////////////////////////////////////////////////////////////////////
// message handlers

void CPropPageFrameDefault::OnPaint()
{
	CPaintDC dc(this);
	Draw(&dc);
}


BOOL CPropPageFrameDefault::OnEraseBkgnd(CDC* pDC)
{
	if (g_ThemeLib.IsAvailable() && g_ThemeLib.IsThemeActive())
	{
		HTHEME	hTheme = g_ThemeLib.OpenThemeData(m_hWnd, L"Tab");
		if (hTheme)
		{
			CRect	rect;
			GetClientRect(rect);
			g_ThemeLib.DrawThemeBackground(hTheme, pDC->m_hDC, TABP_BODY, 0, rect, NULL);
			g_ThemeLib.DrawThemeEdge(hTheme, pDC->m_hDC, TABP_BODY, 0, rect, BDR_SUNKENOUTER, BF_FLAT | BF_RECT, NULL);

			g_ThemeLib.CloseThemeData(hTheme);
		}
		return TRUE;
	}
	else
	{
		return CWnd::OnEraseBkgnd(pDC);
	}
}



} //namespace TreePropSheet
