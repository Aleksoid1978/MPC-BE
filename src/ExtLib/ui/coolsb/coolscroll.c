/*
    Cool Scrollbar Library Version 1.2

    Module: coolscroll.c
	Copyright (c) J Brown 2001

	http://www.codeproject.com/KB/dialog/coolscroll.aspx
	  
	This code is freeware, however, you may not publish
	this code elsewhere or charge any money for it. This code
	is supplied as-is. I make no guarantees about the suitability
	of this code - use at your own risk.
	
	It would be nice if you credited me, in the event
	that you use this code in a product.
	
	VERSION HISTORY:

	 V1.2: TreeView problem fixed by Diego Tartara
		   Small problem in thumbsize calculation also fixed (thanks Diego!)

	 V1.1: Added support for Right-left windows
	       Changed calling convention of APIs to WINAPI (__stdcall)
		   Completely standalone (no need for c-runtime)
		   Now supports ALL windows with appropriate USER32.DLL patching
		    (you provide!!)

	 V1.0: Apr 2001: Initial Version

  IMPORTANT:
	 This whole library is based around code for a horizontal scrollbar.
	 All "vertical" scrollbar drawing / mouse interaction uses the
	 horizontal scrollbar functions, but uses a trick to convert the vertical
	 scrollbar coordinates into horizontal equivelants. When I started this project,
	 I quickly realised that the code for horz/vert bars was IDENTICAL, apart
	 from the fact that horizontal code uses left/right coords, and vertical code
	 uses top/bottom coords. On entry to a "vertical" drawing function, for example,
	 the coordinates are "rotated" before the horizontal function is called, and
	 then rotated back once the function has completed. When something needs to
	 be drawn, the coords are converted back again before drawing.
	 
     This trick greatly reduces the amount of code required, and makes
	 maintanence much simpler. This way, only one function is needed to draw
	 a scrollbar, but this can be used for both horizontal and vertical bars
	 with careful thought.
*/
#define STRICT
//#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "coolscroll.h"
#include "userdefs.h"
#include "coolsb_internal.h"

//define some values if the new version of common controls
//is not available.
#ifndef NM_CUSTOMDRAW
#define NM_CUSTOMDRAW (NM_FIRST-12)
#define CDRF_DODEFAULT		0x0000
#define CDRF_SKIPDEFAULT	0x0004
#define CDDS_PREPAINT		0x0001
#define CDDS_POSTPAINT		0x0002
#endif

//
//	Special thumb-tracking variables
//
//
static UINT uCurrentScrollbar = COOLSB_NONE;	//SB_HORZ / SB_VERT
static UINT uCurrentScrollPortion = HTSCROLL_NONE;
static UINT uCurrentButton = 0;

static RECT rcThumbBounds;		//area that the scroll thumb can travel in
static int  nThumbSize;			//(pixels)
static int  nThumbPos;			//(pixels)
static int  nThumbMouseOffset;	//(pixels)
static int  nLastPos = -1;		//(scrollbar units)
static int  nThumbPos0;			//(pixels) initial thumb position

//
//	Temporary state used to auto-generate timer messages
//
static UINT uMouseOverId = 0;
static UINT uMouseOverScrollbar = COOLSB_NONE;
static UINT uHitTestPortion = HTSCROLL_NONE;
static UINT uLastHitTestPortion = HTSCROLL_NONE;
static RECT MouseOverRect;

static UINT uScrollTimerMsg = 0;
static UINT uScrollTimerPortion = HTSCROLL_NONE;
static UINT uScrollTimerId = 0;
static HWND hwndCurCoolSB = 0;

static inline COLORREF crThemeRGB(const int r, const int g, const int b)
{
	return fThemeRGB ? (*fThemeRGB)(r, g ,b) : RGB(r, g ,b);
}

#define crNormal       crThemeRGB(100, 105, 110)
#define crBackground   crThemeRGB(60, 65, 70)
#define crMouseOver    crThemeRGB(150, 155, 160)
#define crMouseClicked crThemeRGB(210, 215, 220)

BOOL bMouseClicked = FALSE;

BOOL WINAPI CoolSB_IsThumbTracking(HWND hwnd)	
{ 
	SCROLLWND *sw;

	if((sw = GetScrollWndFromHwnd(hwnd)) == NULL)
		return FALSE;
	else
		return sw->fThumbTracking; 
}

//
//	swap the rectangle's x coords with its y coords
//
static void __stdcall RotateRect(RECT *rect)
{
	int temp;
	temp = rect->left;
	rect->left = rect->top;
	rect->top = temp;

	temp = rect->right;
	rect->right = rect->bottom;
	rect->bottom = temp;
}

//
//	swap the coords if the scrollbar is a SB_VERT
//
static void __stdcall RotateRect0(SCROLLBAR *sb, RECT *rect)
{
	if(sb->nBarType == SB_VERT)
		RotateRect(rect);
}

//
//	Calculate if the SCROLLINFO members produce
//  an enabled or disabled scrollbar
//
static BOOL IsScrollInfoActive(SCROLLINFO *si)
{
	if((si->nPage > (UINT)si->nMax
		|| si->nMax <= si->nMin || si->nMax == 0))
		return FALSE;
	else
		return TRUE;
}

//
//	Return if the specified scrollbar is enabled or not
//
static BOOL IsScrollbarActive(SCROLLBAR *sb)
{
	SCROLLINFO *si = &sb->scrollInfo;
	if(((sb->fScrollFlags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH) ||
		!(sb->fScrollFlags & CSBS_THUMBALWAYS) && !IsScrollInfoActive(si))
		return FALSE;
	else
		return TRUE;
}

//
//	Draw a standard scrollbar arrow
//
static int DrawScrollArrow(SCROLLBAR *sbar, HDC hdc, RECT *rect, UINT arrow, BOOL fMouseDown, BOOL fMouseOver)
{
	UINT ret;
	UINT flags = arrow;

	//HACKY bit so this routine can be called by vertical and horizontal code
	if(sbar->nBarType == SB_VERT)
	{
		if(flags & DFCS_SCROLLLEFT)		flags = flags & ~DFCS_SCROLLLEFT  | DFCS_SCROLLUP;
		if(flags & DFCS_SCROLLRIGHT)	flags = flags & ~DFCS_SCROLLRIGHT | DFCS_SCROLLDOWN;
	}

	if(fMouseDown) flags |= (DFCS_FLAT | DFCS_PUSHED);

#ifdef FLAT_SCROLLBARS
	if(sbar->fFlatScrollbar != CSBS_NORMAL)
	{
		HDC hdcmem1, hdcmem2;
		HBITMAP hbm1, oldbm1;
		HBITMAP hbm2, oldbm2;
		RECT rc;
		int width, height;

		rc = *rect;
		width  = rc.right-rc.left;
		height = rc.bottom-rc.top;
		SetRect(&rc, 0, 0, width, height);

		//MONOCHROME bitmap to convert the arrow to black/white mask
		hdcmem1 = CreateCompatibleDC(hdc);
		hbm1    = CreateBitmap(width, height, 1, 1, NULL);
		UnrealizeObject(hbm1);
		oldbm1  = SelectObject(hdcmem1, hbm1);
		

		//NORMAL bitmap to draw the arrow into
		hdcmem2 = CreateCompatibleDC(hdc);
		hbm2    = CreateCompatibleBitmap(hdc, width, height);
		UnrealizeObject(hbm2);
		oldbm2  = SelectObject(hdcmem2, hbm2);
		

		flags = flags & ~DFCS_PUSHED | DFCS_FLAT;	//just in case
		DrawFrameControl(hdcmem2, &rc, DFC_SCROLL, flags);


#ifndef HOT_TRACKING
		if(fMouseDown)
		{
			//uncomment these to make the cool scrollbars
			//look like the common controls flat scrollbars
			//fMouseDown = FALSE;
			//fMouseOver = TRUE;
		}
#endif
		//draw a flat monochrome version of a scrollbar arrow (dark)
		if(fMouseDown)
		{
			SetBkColor(hdcmem2, GetSysColor(COLOR_BTNTEXT));
			BitBlt(hdcmem1, 0, 0, width, height, hdcmem2, 0, 0, SRCCOPY);
			SetBkColor(hdc, crMouseClicked);
			SetTextColor(hdc, crBackground);
			BitBlt(hdc, rect->left, rect->top, width, height, hdcmem1, 0, 0, SRCCOPY);
		}
		//draw a flat monochrome version of a scrollbar arrow (grey)
		else if(fMouseOver)
		{
			SetBkColor(hdcmem2, GetSysColor(COLOR_BTNTEXT));
			BitBlt(hdcmem1, 0, 0, width, height, hdcmem2, 0, 0, SRCINVERT);

			SetBkColor(hdc, crMouseOver);
			SetTextColor(hdc, crBackground);
			BitBlt(hdc, rect->left, rect->top, width, height, hdcmem1, 0, 0, SRCCOPY);
		}
		//draw the arrow normally
		else
		{
			SetBkColor(hdcmem2, GetSysColor(COLOR_BTNTEXT));
			BitBlt(hdcmem1, 0, 0, width, height, hdcmem2, 0, 0, SRCCOPY);
			SetBkColor(hdc, crNormal);
			SetTextColor(hdc, crBackground);
			BitBlt(hdc, rect->left, rect->top, width, height, hdcmem1, 0, 0, SRCCOPY);
		}

		SelectObject(hdcmem1, oldbm1);
		SelectObject(hdcmem2, oldbm2);
		DeleteObject(hbm1);
		DeleteObject(hbm2);
		DeleteDC(hdcmem1);
		DeleteDC(hdcmem2);

		ret = 0;
	}
	else
#endif
	ret = DrawFrameControl(hdc, rect, DFC_SCROLL, flags);

	return ret;
}

//
//	Return the size in pixels for the specified scrollbar metric,
//  for the specified scrollbar
//
static int GetScrollMetric(SCROLLBAR *sbar, int metric)
{
	if(sbar->nBarType == SB_HORZ)
	{
		if(metric == SM_CXHORZSB)
		{
			if(sbar->nArrowLength < 0)
				return -sbar->nArrowLength * GetSystemMetrics(SM_CXHSCROLL);
			else
				return sbar->nArrowLength;
		}
		else
		{
			if(sbar->nArrowWidth < 0)
				return -sbar->nArrowWidth * GetSystemMetrics(SM_CYHSCROLL);
			else
				return sbar->nArrowWidth;
		}
	}
	else if(sbar->nBarType == SB_VERT)
	{
		if(metric == SM_CYVERTSB)
		{
			if(sbar->nArrowLength < 0)
				return -sbar->nArrowLength * GetSystemMetrics(SM_CYVSCROLL);
			else
				return sbar->nArrowLength;
		}
		else
		{
			if(sbar->nArrowWidth < 0)
				return -sbar->nArrowWidth * GetSystemMetrics(SM_CXVSCROLL);
			else
				return sbar->nArrowWidth;
		}
	}

	return 0;
}

//
//	
//
static COLORREF GetSBForeColor(void)
{
	return crBackground;
}

static COLORREF GetSBBackColor(void)
{
	return crBackground;
}

//
//	Paint a checkered rectangle, with each alternate
//	pixel being assigned a different colour
//
static void DrawCheckedRect(HDC hdc, RECT *rect, COLORREF fg, COLORREF bg)
{
	static WORD wCheckPat[8] = 
	{ 
		0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555 
	};

	HBITMAP hbmp;
	HBRUSH  hbr, hbrold;
	COLORREF fgold, bgold;

	hbmp = CreateBitmap(8, 8, 1, 1, wCheckPat);
	hbr  = CreatePatternBrush(hbmp);

	UnrealizeObject(hbr);
	SetBrushOrgEx(hdc, rect->left, rect->top, 0);

	hbrold = (HBRUSH)SelectObject(hdc, hbr);

	fgold = SetTextColor(hdc, fg);
	bgold = SetBkColor(hdc, bg);
	
	PatBlt(hdc, rect->left, rect->top, 
				rect->right - rect->left, 
				rect->bottom - rect->top, 
				PATCOPY);
	
	SetBkColor(hdc, bgold);
	SetTextColor(hdc, fgold);
	
	SelectObject(hdc, hbrold);
	DeleteObject(hbr);
	DeleteObject(hbmp);
}

//
//	Fill the specifed rectangle using a solid colour
//
static void PaintRect(HDC hdc, RECT *rect, COLORREF color)
{
	RECT rc = *rect;
	rc.left += 4;
	rc.right -= 4;

	COLORREF oldcol = SetBkColor(hdc, bMouseClicked ? crMouseClicked : crMouseOver);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, _T(""), 0, 0);
	SetBkColor(hdc, oldcol);
}

//
//	Draw a simple blank scrollbar push-button. Can be used
//	to draw a push button, or the scrollbar thumb
//	drawflag - could set to BF_FLAT to make flat scrollbars
//
void DrawBlankButton(HDC hdc, const RECT *rect, UINT drawflag)
{
	RECT rc = *rect;
		
#ifndef FLAT_SCROLLBARS
	drawflag &= ~BF_FLAT;
#endif

	HBRUSH hBrushBackground = CreateSolidBrush(crBackground);
	FillRect(hdc, &rc, hBrushBackground);
	rc.left += 4;
	rc.right -= 4;
	HBRUSH hBrushNormal = CreateSolidBrush(crNormal);
	FillRect(hdc, &rc, hBrushNormal);

	DeleteObject(hBrushBackground);
	DeleteObject(hBrushNormal);
}

//
//	Send a WM_VSCROLL or WM_HSCROLL message
//
static void SendScrollMessage(HWND hwnd, UINT scrMsg, UINT scrId, UINT pos)
{
	SendMessage(hwnd, scrMsg, MAKEWPARAM(scrId, pos), 0);
}

//
//	Calculate the screen coordinates of the area taken by
//  the horizontal scrollbar. Take into account the size
//  of the window borders
//
static BOOL GetHScrollRect(SCROLLWND *sw, HWND hwnd, RECT *rect)
{
	GetWindowRect(hwnd, rect);
	
	if(sw->fLeftScrollbar)
	{
		rect->left  += sw->cxLeftEdge + (sw->sbarVert.fScrollVisible ? 
					GetScrollMetric(&sw->sbarVert, SM_CXVERTSB) : 0);
		rect->right -= sw->cxRightEdge;
	}
	else
	{
		rect->left   += sw->cxLeftEdge;					//left window edge
	
		rect->right  -= sw->cxRightEdge +				//right window edge
					(sw->sbarVert.fScrollVisible ? 
					GetScrollMetric(&sw->sbarVert, SM_CXVERTSB) : 0);
	}
	
	rect->bottom -= sw->cyBottomEdge;				//bottom window edge
	
	rect->top	  = rect->bottom -
					(sw->sbarHorz.fScrollVisible ?
					GetScrollMetric(&sw->sbarHorz, SM_CYHORZSB) : 0);
	
	return TRUE;
}

//
//	Calculate the screen coordinates of the area taken by the
//  vertical scrollbar
//
static BOOL GetVScrollRect(SCROLLWND *sw, HWND hwnd, RECT *rect)
{
	GetWindowRect(hwnd, rect);
	rect->top	 += sw->cyTopEdge;						//top window edge
	
	rect->bottom -= sw->cyBottomEdge + 
					(sw->sbarHorz.fScrollVisible ?		//bottom window edge
					GetScrollMetric(&sw->sbarHorz, SM_CYHORZSB) : 0);

	if(sw->fLeftScrollbar)
	{
		rect->left	+= sw->cxLeftEdge;
		rect->right = rect->left + (sw->sbarVert.fScrollVisible ?
					GetScrollMetric(&sw->sbarVert, SM_CXVERTSB) : 0);
	}
	else
	{
		rect->right  -= sw->cxRightEdge;
		rect->left    = rect->right - (sw->sbarVert.fScrollVisible ?	
					GetScrollMetric(&sw->sbarVert, SM_CXVERTSB) : 0);
	}

	return TRUE;
}

//	Depending on what type of scrollbar nBar refers to, call the
//  appropriate Get?ScrollRect function
//
BOOL GetScrollRect(SCROLLWND *sw, UINT nBar, HWND hwnd, RECT *rect)
{
	if(nBar == SB_HORZ)
		return GetHScrollRect(sw, hwnd, rect);
	else if(nBar == SB_VERT)
		return GetVScrollRect(sw, hwnd, rect);
	else
		return FALSE;
}

//
//	This code is a prime candidate for splitting out into a separate
//  file at some stage
//
#ifdef INCLUDE_BUTTONS

//
//	Calculate the size in pixels of the specified button
//
static int GetSingleButSize(SCROLLBAR *sbar, SCROLLBUT *sbut)
{
	//multiple of the system button size
	//or a specific button size
	if(sbut->nSize < 0)
	{
		if(sbar->nBarType == SB_HORZ)
			return -sbut->nSize * GetSystemMetrics(SM_CXHSCROLL);
		else 
			return -sbut->nSize * GetSystemMetrics(SM_CYVSCROLL);
	}
	else
		return  sbut->nSize;
}

//
//	Find the size in pixels of all the inserted buttons,
//	either before or after the specified scrollbar
//
static int GetButtonSize(SCROLLBAR *sbar, HWND hwnd, UINT uBeforeAfter)
{
	int i;
	int nPixels = 0;

	SCROLLBUT *sbut = sbar->sbButtons;
	
	for(i = 0; i < sbar->nButtons; i++)
	{
		//only consider those buttons on the same side as nTopBottom says
		if(sbut[i].uPlacement == uBeforeAfter)
		{
			nPixels += GetSingleButSize(sbar, &sbut[i]);
		}
	}

	return nPixels;
}
#endif //INCLUDE_BUTTONS

//
//	Work out the scrollbar width/height for either type of scrollbar (SB_HORZ/SB_VERT)
//	rect - coords of the scrollbar.
//	store results into *thumbsize and *thumbpos
//
static int CalcThumbSize(SCROLLBAR *sbar, const RECT *rect, int *pthumbsize, int *pthumbpos)
{
	SCROLLINFO *si;
	int scrollsize;			//total size of the scrollbar including arrow buttons
	int workingsize;		//working area (where the thumb can slide)
	int siMaxMin;
	int butsize;
	int startcoord;
	int thumbpos = 0, thumbsize = 0;

	int adjust=0;
	static int count=0;

	//work out the width (for a horizontal) or the height (for a vertical)
	//of a standard scrollbar button
	butsize = GetScrollMetric(sbar, SM_SCROLL_LENGTH);

	if(1) //sbar->nBarType == SB_HORZ)
	{
		scrollsize = rect->right - rect->left;
		startcoord = rect->left;
	}
	/*else if(sbar->nBarType == SB_VERT)
	{
		scrollsize = rect->bottom - rect->top;
		startcoord = rect->top;
	}
	else
	{
		return 0;
	}*/

	si = &sbar->scrollInfo;
	siMaxMin = si->nMax - si->nMin + 1;
	workingsize = scrollsize - butsize * 2;

	//
	// Work out the scrollbar thumb SIZE
	//
	if(si->nPage == 0)
	{
		thumbsize = butsize;
	}
	else if(siMaxMin > 0)
	{
		thumbsize = MulDiv(si->nPage, workingsize, siMaxMin);

		if(thumbsize < sbar->nMinThumbSize)
			thumbsize = sbar->nMinThumbSize;
	}

	//
	// Work out the scrollbar thumb position
	//
	if(siMaxMin > 0)
	{
		int pagesize = max(1, si->nPage);
		thumbpos = MulDiv(si->nPos - si->nMin, workingsize-thumbsize, siMaxMin - pagesize);
		
		if(thumbpos < 0)						
			thumbpos = 0;

		if(thumbpos >= workingsize-thumbsize)	
			thumbpos = workingsize-thumbsize;
	}

	thumbpos += startcoord + butsize;

	*pthumbpos  = thumbpos;
	*pthumbsize = thumbsize;

	return 1;
}

//
//	return a hit-test value for whatever part of the scrollbar x,y is located in
//	rect, x, y: SCREEN coordinates
//	the rectangle must not include space for any inserted buttons 
//	(i.e, JUST the scrollbar area)
//
static UINT GetHorzScrollPortion(SCROLLBAR *sbar, HWND hwnd, const RECT *rect, int x, int y)
{
	int thumbwidth, thumbpos;
	int butwidth = GetScrollMetric(sbar, SM_SCROLL_LENGTH);
	int scrollwidth  = rect->right-rect->left;
	int workingwidth = scrollwidth - butwidth*2;

	if(y < rect->top || y >= rect->bottom)
		return HTSCROLL_NONE;

	CalcThumbSize(sbar, rect, &thumbwidth, &thumbpos);

	//if we have had to scale the buttons to fit in the rect,
	//then adjust the button width accordingly
	if(scrollwidth <= butwidth * 2)
	{
		butwidth = scrollwidth / 2;	
	}

	//check for left button click
	if(x >= rect->left && x < rect->left + butwidth)
	{
		return HTSCROLL_LEFT;	
	}
	//check for right button click
	else if(x >= rect->right-butwidth && x < rect->right)
	{
		return HTSCROLL_RIGHT;
	}
	
	//if the thumb is too big to fit (i.e. it isn't visible)
	//then return a NULL scrollbar area
	if(thumbwidth >= workingwidth)
		return HTSCROLL_NONE;
	
	//check for point in the thumbbar
	if(x >= thumbpos && x < thumbpos+thumbwidth)
	{
		return HTSCROLL_THUMB;
	}	
	//check for left margin
	else if(x >= rect->left+butwidth && x < thumbpos)
	{
		return HTSCROLL_PAGELEFT;
	}
	else if(x >= thumbpos+thumbwidth && x < rect->right-butwidth)
	{
		return HTSCROLL_PAGERIGHT;
	}
	
	return HTSCROLL_NONE;
}

//
//	For vertical scrollbars, rotate all coordinates by -90 degrees
//	so that we can use the horizontal version of this function
//
static UINT GetVertScrollPortion(SCROLLBAR *sb, HWND hwnd, RECT *rect, int x, int y)
{
	UINT r;
	
	RotateRect(rect);
	r = GetHorzScrollPortion(sb, hwnd, rect, y, x);
	RotateRect(rect);
	return r;
}

//
//	CUSTOM DRAW support
//	
static LRESULT PostCustomPrePostPaint0(HWND hwnd, HDC hdc, SCROLLBAR *sb, UINT dwStage)
{
#ifdef CUSTOM_DRAW
	NMCSBCUSTOMDRAW	nmcd;

	CoolSB_ZeroMemory(&nmcd, sizeof nmcd);
	nmcd.hdr.hwndFrom = hwnd;
	nmcd.hdr.idFrom   = GetWindowLong(hwnd, GWL_ID);
	nmcd.hdr.code     = NM_COOLSB_CUSTOMDRAW;
	nmcd.nBar		  = sb->nBarType;
	nmcd.dwDrawStage  = dwStage;
	nmcd.hdc		  = hdc;

	hwnd = GetParent(hwnd);
	return SendMessage(hwnd, WM_NOTIFY, 0, (LPARAM)&nmcd);
#else
	return 0;
#endif
}

static LRESULT PostCustomDrawNotify0(HWND hwnd, HDC hdc, UINT nBar, RECT *prect, UINT nItem, BOOL fMouseDown, BOOL fMouseOver, BOOL fInactive)
{
#ifdef CUSTOM_DRAW
	NMCSBCUSTOMDRAW	nmcd;

	//fill in the standard header
	nmcd.hdr.hwndFrom = hwnd;
	nmcd.hdr.idFrom   = GetWindowLong(hwnd, GWL_ID);
	nmcd.hdr.code     = NM_COOLSB_CUSTOMDRAW;

	nmcd.dwDrawStage  = CDDS_ITEMPREPAINT;
	nmcd.nBar		  = nBar;
	nmcd.rect		  = *prect;
	nmcd.uItem		  = nItem;
	nmcd.hdc		  = hdc;

	if(fMouseDown) 
		nmcd.uState		  = CDIS_SELECTED;
	else if(fMouseOver)
		nmcd.uState		  = CDIS_HOT;
	else if(fInactive)
		nmcd.uState		  = CDIS_DISABLED;
	else
		nmcd.uState		  = CDIS_DEFAULT;

	hwnd = GetParent(hwnd);
	return SendMessage(hwnd, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
#else
	return 0;
#endif
}

// Depending on if we are supporting custom draw, either define
// a macro to the function name, or to nothing at all. If custom draw
// is turned off, then we can save ALOT of code space by binning all 
// calls to the custom draw support.
//
#ifdef CUSTOM_DRAW
#define PostCustomDrawNotify	PostCustomDrawNotify0
#define PostCustomPrePostPaint	PostCustomPrePostPaint0
#else
#define PostCustomDrawNotify	1 ? (void)0 : PostCustomDrawNotify0
#define PostCustomPrePostPaint	1 ? (void)0 : PostCustomPrePostPaint0
#endif

static LRESULT PostMouseNotify0(HWND hwnd, UINT msg, UINT nBar, RECT *prect, UINT nCmdId, POINT pt)
{
#ifdef NOTIFY_MOUSE
	NMCOOLBUTMSG	nmcb;

	//fill in the standard header
	nmcb.hdr.hwndFrom	= hwnd;
	nmcb.hdr.idFrom		= GetWindowLong(hwnd, GWL_ID);
	nmcb.hdr.code		= NM_CLICK;

	nmcb.nBar			= nBar;
	nmcb.uCmdId			= nCmdId;
	nmcb.uState			= 0;
	nmcb.rect			= *prect;
	nmcb.pt				= pt;

	hwnd = GetParent(hwnd);
	return SendMessage(hwnd, WM_NOTIFY, nmcb.hdr.idFrom, (LPARAM)&nmcb);
#else
	return 0;
#endif
}

#ifdef NOTIFY_MOUSE
#define PostMouseNotify			PostMouseNotify0
#else
#define PostMouseNotify			1 ? (void)0 : PostMouseNotify0
#endif



//
//	Draw a complete HORIZONTAL scrollbar in the given rectangle
//	Don't draw any inserted buttons in this procedure
//	
//	uDrawFlags - hittest code, to say if to draw the
//  specified portion in an active state or not.
//
//
static LRESULT NCDrawHScrollbar(SCROLLBAR *sb, HWND hwnd, HDC hdc, const RECT *rect, UINT uDrawFlags)
{
	SCROLLINFO *si;
	RECT ctrl, thumb;
	RECT sbm;
	int butwidth	 = GetScrollMetric(sb, SM_SCROLL_LENGTH);
	int scrollwidth  = rect->right-rect->left;
	int workingwidth = scrollwidth - butwidth*2;
	int thumbwidth   = 0, thumbpos = 0;
	int siMaxMin;

	BOOL fCustomDraw = 0;

	BOOL fMouseDownL = 0, fMouseOverL = 0, fBarHot = 0;
	BOOL fMouseDownR = 0, fMouseOverR = 0;

	COLORREF crCheck1   = GetSBForeColor();
	COLORREF crCheck2   = GetSBBackColor();
	COLORREF crInverse1 = GetSBBackColor();
	COLORREF crInverse2 = GetSBBackColor();

	UINT uDFCFlat = sb->fFlatScrollbar ? DFCS_FLAT : 0;
	UINT uDEFlat  = sb->fFlatScrollbar ? BF_FLAT   : 0;

	//drawing flags to modify the appearance of the scrollbar buttons
	UINT uLeftButFlags  = DFCS_SCROLLLEFT;
	UINT uRightButFlags = DFCS_SCROLLRIGHT;

	if(scrollwidth <= 0)
		return 0;

	si = &sb->scrollInfo;
	siMaxMin = si->nMax - si->nMin;

	if(hwnd != hwndCurCoolSB)
		uDrawFlags = HTSCROLL_NONE;
	//
	// work out the thumb size and position
	//
	CalcThumbSize(sb, rect, &thumbwidth, &thumbpos);
	
	if(sb->fScrollFlags & ESB_DISABLE_LEFT)		uLeftButFlags  |= DFCS_INACTIVE;
	if(sb->fScrollFlags & ESB_DISABLE_RIGHT)	uRightButFlags |= DFCS_INACTIVE;

	//if we need to grey the arrows because there is no data to scroll
	if(!IsScrollInfoActive(si) && !(sb->fScrollFlags & CSBS_THUMBALWAYS))
	{
		uLeftButFlags  |= DFCS_INACTIVE;
		uRightButFlags |= DFCS_INACTIVE;
	}

	if(hwnd == hwndCurCoolSB)
	{
#ifdef FLAT_SCROLLBARS	
		BOOL ldis = !(uLeftButFlags & DFCS_INACTIVE);
		BOOL rdis = !(uRightButFlags & DFCS_INACTIVE);

		fBarHot = (sb->nBarType == (int)uMouseOverScrollbar && sb->fFlatScrollbar == CSBS_HOTTRACKED);
		
		fMouseOverL = uHitTestPortion == HTSCROLL_LEFT && fBarHot && ldis;		
		fMouseOverR = uHitTestPortion == HTSCROLL_RIGHT && fBarHot && rdis;
#endif
		fMouseDownL = (uDrawFlags == HTSCROLL_LEFT);
		fMouseDownR = (uDrawFlags == HTSCROLL_RIGHT);
	}


#ifdef CUSTOM_DRAW
	fCustomDraw = ((PostCustomPrePostPaint(hwnd, hdc, sb, CDDS_PREPAINT)) == CDRF_SKIPDEFAULT);
#endif

	//
	// Draw the scrollbar now
	//
	if(scrollwidth > butwidth*2)
	{
		//LEFT ARROW
		SetRect(&ctrl, rect->left, rect->top, rect->left + butwidth, rect->bottom);

		RotateRect0(sb, &ctrl);

		if(fCustomDraw)
			PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &ctrl, SB_LINELEFT, fMouseDownL, fMouseOverL, uLeftButFlags & DFCS_INACTIVE);
		else
			DrawScrollArrow(sb, hdc, &ctrl, uLeftButFlags, fMouseDownL, fMouseOverL);

		RotateRect0(sb, &ctrl);

		//MIDDLE PORTION
		//if we can fit the thumbbar in, then draw it
		if(thumbwidth > 0 && thumbwidth <= workingwidth
			&& IsScrollInfoActive(si) && ((sb->fScrollFlags & ESB_DISABLE_BOTH) != ESB_DISABLE_BOTH))
		{	
			//Draw the scrollbar margin above the thumb
			SetRect(&sbm, rect->left + butwidth, rect->top, thumbpos, rect->bottom);
			
			RotateRect0(sb, &sbm);
			
			if(fCustomDraw)
			{
				PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &sbm, SB_PAGELEFT, uDrawFlags == HTSCROLL_PAGELEFT, FALSE, FALSE);
			}
			else
			{
				if(uDrawFlags == HTSCROLL_PAGELEFT)
					DrawCheckedRect(hdc, &sbm, crInverse1, crInverse2);
				else
					DrawCheckedRect(hdc, &sbm, crCheck1, crCheck2);

			}

			RotateRect0(sb, &sbm);			
			
			//Draw the margin below the thumb
			sbm.left = thumbpos+thumbwidth;
			sbm.right = rect->right - butwidth;
			
			RotateRect0(sb, &sbm);
			if(fCustomDraw)
			{
				PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &sbm, SB_PAGERIGHT, uDrawFlags == HTSCROLL_PAGERIGHT, 0, 0);
			}
			else
			{
				if(uDrawFlags == HTSCROLL_PAGERIGHT)
					DrawCheckedRect(hdc, &sbm, crInverse1, crInverse2);
				else
					DrawCheckedRect(hdc, &sbm, crCheck1, crCheck2);
			
			}
			RotateRect0(sb, &sbm);
			
			//Draw the THUMB finally
			SetRect(&thumb, thumbpos, rect->top, thumbpos+thumbwidth, rect->bottom);

			RotateRect0(sb, &thumb);			

			if(fCustomDraw)
			{
				PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &thumb, SB_THUMBTRACK, uDrawFlags==HTSCROLL_THUMB, uHitTestPortion == HTSCROLL_THUMB && fBarHot, FALSE);
			}
			else
			{

#ifdef FLAT_SCROLLBARS	
				if(hwnd == hwndCurCoolSB && sb->fFlatScrollbar && (uDrawFlags == HTSCROLL_THUMB || 
				(uHitTestPortion == HTSCROLL_THUMB && fBarHot)))
				{	
					PaintRect(hdc, &thumb, GetSysColor(COLOR_3DSHADOW));
				}
				else
#endif
				{
					DrawBlankButton(hdc, &thumb, uDEFlat);
				}
			}
			RotateRect0(sb, &thumb);

		}
		//otherwise, just leave that whole area blank
		else
		{
			OffsetRect(&ctrl, butwidth, 0);
			ctrl.right = rect->right - butwidth;

			//if we always show the thumb covering the whole scrollbar,
			//then draw it that way
			if(!IsScrollInfoActive(si)	&& (sb->fScrollFlags & CSBS_THUMBALWAYS) 
				&& ctrl.right - ctrl.left > sb->nMinThumbSize)
			{
				//leave a 1-pixel gap between the thumb + right button
				ctrl.right --;
				RotateRect0(sb, &ctrl);

				if(fCustomDraw)
					PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &ctrl, SB_THUMBTRACK, fMouseDownL, FALSE, FALSE);
				else
				{
#ifdef FLAT_SCROLLBARS	
					if(sb->fFlatScrollbar == CSBS_HOTTRACKED && uDrawFlags == HTSCROLL_THUMB)
						PaintRect(hdc, &ctrl, GetSysColor(COLOR_3DSHADOW));
					else
#endif
						DrawBlankButton(hdc, &ctrl, uDEFlat);

				}
				RotateRect0(sb, &ctrl);

				//draw the single-line gap
				ctrl.left = ctrl.right;
				ctrl.right += 1;
				
				RotateRect0(sb, &ctrl);
				
				if(fCustomDraw)
					PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &ctrl, SB_PAGERIGHT, 0, 0, 0);
				else
					PaintRect(hdc, &ctrl, GetSysColor(COLOR_SCROLLBAR));

				RotateRect0(sb, &ctrl);
			}
			//otherwise, paint a blank if the thumb doesn't fit in
			else
			{
				RotateRect0(sb, &ctrl);
	
				if(fCustomDraw)
					PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &ctrl, SB_PAGERIGHT, 0, 0, 0);
				else
					DrawCheckedRect(hdc, &ctrl, crCheck1, crCheck2);
				
				RotateRect0(sb, &ctrl);
			}
		}

		//RIGHT ARROW
		SetRect(&ctrl, rect->right - butwidth, rect->top, rect->right, rect->bottom);

		RotateRect0(sb, &ctrl);

		if(fCustomDraw)
			PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &ctrl, SB_LINERIGHT, fMouseDownR, fMouseOverR, uRightButFlags & DFCS_INACTIVE);
		else
			DrawScrollArrow(sb, hdc, &ctrl, uRightButFlags, fMouseDownR, fMouseOverR);

		RotateRect0(sb, &ctrl);
	}
	//not enough room for the scrollbar, so just draw the buttons (scaled in size to fit)
	else
	{
		butwidth = scrollwidth / 2;

		//LEFT ARROW
		SetRect(&ctrl, rect->left, rect->top, rect->left + butwidth, rect->bottom);

		RotateRect0(sb, &ctrl);
		if(fCustomDraw)
			PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &ctrl, SB_LINELEFT, fMouseDownL, fMouseOverL, uLeftButFlags & DFCS_INACTIVE);
		else	
			DrawScrollArrow(sb, hdc, &ctrl, uLeftButFlags, fMouseDownL, fMouseOverL);
		RotateRect0(sb, &ctrl);

		//RIGHT ARROW
		OffsetRect(&ctrl, scrollwidth - butwidth, 0);
		
		RotateRect0(sb, &ctrl);
		if(fCustomDraw)
			PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &ctrl, SB_LINERIGHT, fMouseDownR, fMouseOverR, uRightButFlags & DFCS_INACTIVE);
		else
			DrawScrollArrow(sb, hdc, &ctrl, uRightButFlags, fMouseDownR, fMouseOverR);		
		RotateRect0(sb, &ctrl);

		//if there is a gap between the buttons, fill it with a solid color
		//if(butwidth & 0x0001)
		if(ctrl.left != rect->left + butwidth)
		{
			ctrl.left --;
			ctrl.right -= butwidth;
			RotateRect0(sb, &ctrl);
			
			if(fCustomDraw)
				PostCustomDrawNotify(hwnd, hdc, sb->nBarType, &ctrl, SB_PAGERIGHT, 0, 0, 0);
			else
				DrawCheckedRect(hdc, &ctrl, crCheck1, crCheck2);

			RotateRect0(sb, &ctrl);
		}
			
	}

#ifdef CUSTOM_DRAW
	PostCustomPrePostPaint(hwnd, hdc, sb, CDDS_POSTPAINT);
#endif

	return fCustomDraw;
}

//
//	Draw a vertical scrollbar using the horizontal draw routine, but
//	with the coordinates adjusted accordingly
//
static LRESULT NCDrawVScrollbar(SCROLLBAR *sb, HWND hwnd, HDC hdc, const RECT *rect, UINT uDrawFlags)
{
	LRESULT ret;
	RECT rc;

	rc = *rect;
	RotateRect(&rc);
	ret = NCDrawHScrollbar(sb, hwnd, hdc, &rc, uDrawFlags);
	RotateRect(&rc);
	
	return ret;
}

//
//	Generic wrapper function for the scrollbar drawing
//
static LRESULT NCDrawScrollbar(SCROLLBAR *sb, HWND hwnd, HDC hdc, const RECT *rect, UINT uDrawFlags)
{
	if(sb->nBarType == SB_HORZ)
		return NCDrawHScrollbar(sb, hwnd, hdc, rect, uDrawFlags);
	else
		return NCDrawVScrollbar(sb, hwnd, hdc, rect, uDrawFlags);
}

#ifdef INCLUDE_BUTTONS

//
//	Draw the specified bitmap centered in the rectangle
//
static void DrawImage(HDC hdc, HBITMAP hBitmap, RECT *rc)
{
	BITMAP bm;
	int cx;
	int cy;   
	HDC memdc;
	HBITMAP hOldBM;
	RECT  rcDest = *rc;   
	POINT p;
	SIZE  delta;
	COLORREF colorOld;

	if(hBitmap == NULL) 
		return;

	// center bitmap in caller's rectangle   
	GetObject(hBitmap, sizeof bm, &bm);   
	
	cx = bm.bmWidth;
	cy = bm.bmHeight;

	delta.cx = (rc->right-rc->left - cx) / 2;
	delta.cy = (rc->bottom-rc->top - cy) / 2;
	
	if(rc->right-rc->left > cx)
	{
		SetRect(&rcDest, rc->left+delta.cx, rc->top + delta.cy, 0, 0);   
		rcDest.right = rcDest.left + cx;
		rcDest.bottom = rcDest.top + cy;
		p.x = 0;
		p.y = 0;
	}
	else
	{
		p.x = -delta.cx;   
		p.y = -delta.cy;
	}
   
	// select checkmark into memory DC
	memdc = CreateCompatibleDC(hdc);
	hOldBM = (HBITMAP)SelectObject(memdc, hBitmap);
   
	// set BG color based on selected state   
	colorOld = SetBkColor(hdc, GetSysColor(COLOR_3DFACE));

	BitBlt(hdc, rcDest.left, rcDest.top, rcDest.right-rcDest.left, rcDest.bottom-rcDest.top, memdc, p.x, p.y, SRCCOPY);

	// restore
	SetBkColor(hdc, colorOld);
	SelectObject(memdc, hOldBM);
	DeleteDC(memdc);
}

//
// Draw the specified metafile
//
static void DrawMetaFile(HDC hdc, HENHMETAFILE hemf, RECT *rect)
{
	RECT rc;
	POINT pt;

	SetRect(&rc, 0, 0, rect->right-rect->left, rect->bottom-rect->top);
	SetWindowOrgEx(hdc, -rect->left, -rect->top, &pt);
	PlayEnhMetaFile(hdc, hemf, &rc);
	SetWindowOrgEx(hdc, pt.x, pt.y, 0);
}

//
//	Draw a single scrollbar inserted button, in whatever style
//	it has been defined to use.	
//
static UINT DrawScrollButton(SCROLLBUT *sbut, HDC hdc, const RECT *pctrl, UINT flags)
{
	NMCSBCUSTOMDRAW	nmcd;
	HWND hwnd;
	RECT rect = *pctrl;
	UINT f;

	switch(sbut->uButType & SBBT_MASK)
	{
	case SBBT_OWNERDRAW:

		hwnd = WindowFromDC(hdc);

		//fill in the standard header
		nmcd.hdr.hwndFrom = hwnd;
		nmcd.hdr.idFrom   = GetWindowLong(hwnd, GWL_ID);
		nmcd.hdr.code     = NM_COOLSB_CUSTOMDRAW;

		nmcd.dwDrawStage  = CDDS_ITEMPREPAINT;
		nmcd.nBar		  = SB_INSBUT;
		nmcd.rect		  = *pctrl;
		nmcd.uItem		  = sbut->uCmdId;
		nmcd.hdc		  = hdc;
		nmcd.uState		  = flags;

		IntersectClipRect(hdc, rect.left, rect.top, rect.right, rect.bottom);
		SendMessage(GetParent(hwnd), WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
		SelectClipRgn(hdc, NULL);

		break;

	case SBBT_FIXED:
		flags &= ~SBBS_PUSHED;

	case SBBT_TOGGLEBUTTON:
		if(sbut->uState != SBBS_NORMAL)
			flags |= SBBS_PUSHED;

		//intentionally fall through here...

	case SBBT_PUSHBUTTON: 
		
		f = flags & SBBS_PUSHED ? DFCS_PUSHED | DFCS_FLAT : 0;
		if(sbut->uButType & SBBM_LEFTARROW)
		{
			DrawFrameControl(hdc, &rect, DFC_SCROLL, DFCS_SCROLLLEFT | f);
		}
		else if(sbut->uButType & SBBM_RIGHTARROW)
		{
			DrawFrameControl(hdc, &rect, DFC_SCROLL, DFCS_SCROLLRIGHT | f);
		}
		else if(sbut->uButType & SBBM_UPARROW)
		{
			DrawFrameControl(hdc, &rect, DFC_SCROLL, DFCS_SCROLLUP | f);
		}
		else if(sbut->uButType & SBBM_DOWNARROW)
		{
			DrawFrameControl(hdc, &rect, DFC_SCROLL, DFCS_SCROLLDOWN | f);
		}
		else
		{
			//
			if(flags & SBBS_PUSHED)
			{
				if(sbut->uButType & SBBM_RECESSED)
				{
					InflateRect(&rect, -1, -1);
					DrawEdge(hdc, &rect, EDGE_SUNKEN, BF_RECT|BF_FLAT);
					InflateRect(&rect, 1, 1);

					FrameRect(hdc, &rect, GetSysColorBrush(COLOR_3DDKSHADOW));
					InflateRect(&rect, -2, -2);
				}
				else
				{
					DrawEdge(hdc, &rect, EDGE_SUNKEN, BF_RECT | BF_FLAT | BF_ADJUST);
					InflateRect(&rect, 1, 1);
				}
			}
			else
			{
				// draw the button borders
				if(sbut->uButType & SBBM_TYPE2)
				{
					DrawFrameControl(hdc, &rect, DFC_BUTTON, DFCS_BUTTONPUSH);
					InflateRect(&rect, -2, -2);
				}

				else if(sbut->uButType & SBBM_TYPE3)
				{
					DrawFrameControl(hdc, &rect, DFC_BUTTON, DFCS_BUTTONPUSH);
					InflateRect(&rect, -1, -1);
				}
				else
				{
					DrawEdge(hdc, &rect, EDGE_RAISED, BF_RECT | BF_ADJUST);
					rect.bottom++;
					rect.right++;
				}
			
				OffsetRect(&rect, -1, -1);
				rect.top++;	rect.left++;
			}

			if(sbut->hBmp)
			{
				PaintRect(hdc, &rect, GetSysColor(COLOR_3DFACE));

				if(flags & SBBS_PUSHED)	
				{
					rect.top++; rect.left++;
				}

				IntersectClipRect(hdc, rect.left, rect.top, rect.right,rect.bottom);
				DrawImage(hdc, sbut->hBmp, &rect);
				SelectClipRgn(hdc, 0);
			}
			else if(sbut->hEmf)
			{
				PaintRect(hdc, &rect, GetSysColor(COLOR_3DFACE));
				InflateRect(&rect, -1, -1);		

				if(flags & SBBS_PUSHED)	
				{
					rect.top++; rect.left++;
				}

				IntersectClipRect(hdc, rect.left, rect.top, rect.right,rect.bottom);
				DrawMetaFile(hdc, sbut->hEmf, &rect);
				SelectClipRgn(hdc, 0);
			}
			else
			{
				PaintRect(hdc, &rect, GetSysColor(COLOR_3DFACE));
			}
		}
			

		break;

	case SBBT_BLANK:
		PaintRect(hdc, &rect, GetSysColor(COLOR_3DFACE));
		break;

	case SBBT_FLAT:
		DrawBlankButton(hdc, &rect, BF_FLAT);
		break;

	case SBBT_DARK:
		PaintRect(hdc, &rect, GetSysColor(COLOR_3DDKSHADOW));
		break;
	}
	
	return 0;
}

//
//	Draw any buttons inserted into the horizontal scrollbar
//	assume that the button widths have already been calculated
//	Note: RECT *rect is the rectangle of the scrollbar
//	      leftright: 1 = left, 2 = right, 3 = both
//
static LRESULT DrawHorzButtons(SCROLLBAR *sbar, HDC hdc, const RECT *rect, int leftright)
{
	int i;
	int xposl, xposr;
	RECT ctrl;
	SCROLLBUT *sbut = sbar->sbButtons;
	
	xposl = rect->left - sbar->nButSizeBefore;
	xposr = rect->right;
	
	for(i = 0; i < sbar->nButtons; i++)
	{
		if((leftright & SBBP_LEFT) && sbut[i].uPlacement == SBBP_LEFT)
		{
			int butwidth = GetSingleButSize(sbar, &sbut[i]);
			SetRect(&ctrl, xposl, rect->top, xposl + butwidth, rect->bottom);
			RotateRect0(sbar, &ctrl);
			DrawScrollButton(&sbut[i], hdc, &ctrl, SBBS_NORMAL);
			
			xposl += butwidth;
		}

		if((leftright & SBBP_RIGHT) && sbut[i].uPlacement == SBBP_RIGHT)
		{
			int butwidth = GetSingleButSize(sbar, &sbut[i]);
			SetRect(&ctrl, xposr, rect->top, xposr + butwidth, rect->bottom);
			RotateRect0(sbar, &ctrl);
			DrawScrollButton(&sbut[i], hdc, &ctrl, SBBS_NORMAL);
			xposr += butwidth;
		}
	}
	return 0;
}

static LRESULT DrawVertButtons(SCROLLBAR *sbar, HDC hdc, const RECT *rect, int leftright)
{
	RECT rc = *rect;
	RotateRect(&rc);
	DrawHorzButtons(sbar, hdc, &rc, leftright);
	return 0;
}
#endif	// INCLUDE_BUTTONS

//
//	Define these two for proper processing of NCPAINT
//	NOT needed if we don't bother to mask the scrollbars we draw
//	to prevent the old window procedure from accidently drawing over them
//
HDC CoolSB_GetDC(HWND hwnd, WPARAM wParam)
{
	// I just can't figure out GetDCEx, so I'll just use this:
	return GetWindowDC(hwnd);

	/*
	RECT rc;
	DWORD flags = 0x10000;
	HRGN hrgn = (HRGN)wParam;

	if(hrgn == (HRGN)1)
	{
		GetWindowRect(hwnd, &rc);
		OffsetRect(&rc, -rc.left, -rc.top);
		hrgn = CreateRectRgnIndirect(&rc);
	}

	if(GetWindowLong(hwnd, GWL_STYLE) & WS_CLIPCHILDREN)
		flags |= DCX_CLIPCHILDREN;

	if(GetWindowLong(hwnd, GWL_STYLE) & WS_CLIPSIBLINGS)
		flags |= DCX_CLIPSIBLINGS;

	return GetDCEx(hwnd, hrgn, flags | DCX_CACHE|DCX_NORESETATTRS|DCX_WINDOW | DCX_INTERSECTUPDATE);
	*/

	//return GetDCEx(hwnd, NULL, flags | DCX_WINDOW| DCX_NORESETATTRS);
}

static LRESULT NCPaint(SCROLLWND *sw, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	SCROLLBAR *sb;
	HDC hdc;
	HRGN hrgn;
	RECT winrect, rect;
	HRGN clip;
	BOOL fUpdateAll = ((LONG)wParam == 1);
	BOOL fCustomDraw = FALSE;
	UINT ret;
	DWORD dwStyle;

	GetWindowRect(hwnd, &winrect);
	
	//if entire region needs painting, then make a region to cover the entire window
	if(fUpdateAll)
		hrgn = (HRGN)wParam;
	else
		hrgn = (HRGN)wParam;
	
	//hdc = GetWindowDC(hwnd);
	hdc = CoolSB_GetDC(hwnd, wParam);

	//
	//	Only draw the horizontal scrollbar if the window is tall enough
	//
	sb = &sw->sbarHorz;
	if(sb->fScrollVisible)
	{
		int hbarwidth = 0, leftright = 0;

		//get the screen coordinates of the whole horizontal scrollbar area
		GetHScrollRect(sw, hwnd, &rect);

		//make the coordinates relative to the window for drawing
		OffsetRect(&rect, -winrect.left, -winrect.top);

#ifdef INCLUDE_BUTTONS

		//work out the size of any inserted buttons so we can dra them
		sb->nButSizeBefore  = GetButtonSize(sb, hwnd, SBBP_LEFT);
		sb->nButSizeAfter   = GetButtonSize(sb, hwnd, SBBP_RIGHT);

		//make sure there is room for the buttons
		hbarwidth = rect.right - rect.left;

		//check that we can fit any left/right buttons in the available space
		if(sb->nButSizeAfter < (hbarwidth - MIN_COOLSB_SIZE))
		{
			//adjust the scrollbar rectangle to fit the buttons into
			sb->fButVisibleAfter = TRUE;
			rect.right -= sb->nButSizeAfter;
			leftright |= SBBP_RIGHT;
			
			//check that there is enough space for the right buttons
			if(sb->nButSizeBefore + sb->nButSizeAfter < (hbarwidth - MIN_COOLSB_SIZE))
			{
				sb->fButVisibleBefore = TRUE;
				rect.left += sb->nButSizeBefore;
				leftright |= SBBP_LEFT;
			}
			else
				sb->fButVisibleBefore = FALSE;
		}	
		else
			sb->fButVisibleAfter = FALSE;
		
		
		DrawHorzButtons(sb, hdc, &rect, leftright);
#endif// INCLUDE_BUTTONS		

		if(uCurrentScrollbar == SB_HORZ)
			fCustomDraw |= NCDrawHScrollbar(sb, hwnd, hdc, &rect, uScrollTimerPortion);
		else
			fCustomDraw |= NCDrawHScrollbar(sb, hwnd, hdc, &rect, HTSCROLL_NONE);
	}

	//
	// Only draw the vertical scrollbar if the window is wide enough to accomodate it
	//
	sb = &sw->sbarVert;
	if(sb->fScrollVisible)
	{
		int vbarheight = 0, updown = 0;

		//get the screen cooridinates of the whole horizontal scrollbar area
		GetVScrollRect(sw, hwnd, &rect);

		//make the coordinates relative to the window for drawing
		OffsetRect(&rect, -winrect.left, -winrect.top);

#ifdef INCLUDE_BUTTONS

		//work out the size of any inserted buttons so we can dra them
		sb->nButSizeBefore  = GetButtonSize(sb, hwnd, SBBP_LEFT);
		sb->nButSizeAfter   = GetButtonSize(sb, hwnd, SBBP_RIGHT);

		//make sure there is room for the buttons
		vbarheight = rect.bottom - rect.top;

		//check that we can fit any left/right buttons in the available space
		if(sb->nButSizeAfter < (vbarheight - MIN_COOLSB_SIZE))
		{
			//adjust the scrollbar rectangle to fit the buttons into
			sb->fButVisibleAfter = TRUE;
			rect.bottom -= sb->nButSizeAfter;
			updown |= SBBP_BOTTOM;
			
			//check that there is enough space for the right buttons
			if(sb->nButSizeBefore + sb->nButSizeAfter < (vbarheight - MIN_COOLSB_SIZE))
			{
				sb->fButVisibleBefore = TRUE;
				rect.top += sb->nButSizeBefore;
				updown |= SBBP_TOP;
			}
			else
				sb->fButVisibleBefore = FALSE;
		}	
		else
			sb->fButVisibleAfter = FALSE;
		
	
		DrawVertButtons(sb, hdc, &rect, updown);
#endif // INCLUDE_BUTTONS

		if(uCurrentScrollbar == SB_VERT)
			fCustomDraw |= NCDrawVScrollbar(sb, hwnd, hdc, &rect, uScrollTimerPortion);
		else
			fCustomDraw |= NCDrawVScrollbar(sb, hwnd, hdc, &rect, HTSCROLL_NONE);
	}

	//Call the default window procedure for WM_NCPAINT, with the
	//new window region. ** region must be in SCREEN coordinates **
	dwStyle = GetWindowLong(hwnd, GWL_STYLE);

    // If the window has WS_(H-V)SCROLL bits set, we should reset them
    // to avoid windows taking the scrollbars into account.
    // We temporarily set a flag preventing the subsecuent 
    // WM_STYLECHANGING/WM_STYLECHANGED to be forwarded to 
    // the original window procedure
    if ( dwStyle & (WS_VSCROLL|WS_HSCROLL) )
    {
        sw->bPreventStyleChange = TRUE;
        SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~(WS_VSCROLL|WS_HSCROLL));
    }
	
	ret = CallWindowProc(sw->oldproc, hwnd, WM_NCPAINT, (WPARAM)hrgn, lParam);
	
    if ( dwStyle & (WS_VSCROLL|WS_HSCROLL) )
    {
        SetWindowLong(hwnd, GWL_STYLE, dwStyle);
        sw->bPreventStyleChange = FALSE;
    }


	// DRAW THE DEAD AREA
	// only do this if the horizontal and vertical bars are visible
	if(sw->sbarHorz.fScrollVisible && sw->sbarVert.fScrollVisible)
	{
		GetWindowRect(hwnd, &rect);
		OffsetRect(&rect, -winrect.left, -winrect.top);

		rect.bottom -= sw->cyBottomEdge;
		rect.top  = rect.bottom - GetScrollMetric(&sw->sbarHorz, SM_CYHORZSB);

		if(sw->fLeftScrollbar)
		{
			rect.left += sw->cxLeftEdge;
			rect.right = rect.left + GetScrollMetric(&sw->sbarVert, SM_CXVERTSB);
		}
		else
		{
			rect.right -= sw->cxRightEdge;
			rect.left = rect.right  - GetScrollMetric(&sw->sbarVert, SM_CXVERTSB);
		}
		
		if(fCustomDraw)
			PostCustomDrawNotify(hwnd, hdc, SB_BOTH, &rect, 32, 0, 0, 0);
		else
		{
			//calculate the position of THIS window's dead area
			//with the position of the PARENT window's client rectangle.
			//if THIS window has been positioned such that its bottom-right
			//corner sits in the parent's bottom-right corner, then we should
			//show the sizing-grip.
			//Otherwise, assume this window is not in the right place, and
			//just draw a blank rectangle
			RECT parent;
			RECT rect2;
			HWND hwndParent = GetParent(hwnd);

			GetClientRect(hwndParent, &parent);
			MapWindowPoints(hwndParent, 0, (POINT *)&parent, 2);

			CopyRect(&rect2, &rect);
			OffsetRect(&rect2, winrect.left, winrect.top);

			if(!sw->fLeftScrollbar && parent.right == rect2.right+sw->cxRightEdge && parent.bottom == rect2.bottom+sw->cyBottomEdge
			 || sw->fLeftScrollbar && parent.left  == rect2.left -sw->cxLeftEdge  && parent.bottom == rect2.bottom+sw->cyBottomEdge)
				DrawFrameControl(hdc, &rect, DFC_SCROLL, sw->fLeftScrollbar ? DFCS_SCROLLSIZEGRIPRIGHT : DFCS_SCROLLSIZEGRIP );
			else
				PaintRect(hdc, &rect, GetSysColor(COLOR_3DFACE));
		}
	}

	UNREFERENCED_PARAMETER(clip);

	ReleaseDC(hwnd, hdc);
	return ret;
}

//
// Need to detect if we have clicked in the scrollbar region or not
//
static LRESULT NCHitTest(SCROLLWND *sw, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	RECT hrect;
	RECT vrect;
	POINT pt;

	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	
	//work out exactly where the Horizontal and Vertical scrollbars are
	GetHScrollRect(sw, hwnd, &hrect);
	GetVScrollRect(sw, hwnd, &vrect);
	
	//Clicked in the horizontal scrollbar area
	if(sw->sbarHorz.fScrollVisible && PtInRect(&hrect, pt))
	{
		return HTHSCROLL;
	}
	//Clicked in the vertical scrollbar area
	else if(sw->sbarVert.fScrollVisible && PtInRect(&vrect, pt))
	{
		return HTVSCROLL;
	}
	//clicked somewhere else
	else
	{
		return CallWindowProc(sw->oldproc, hwnd, WM_NCHITTEST, wParam, lParam);
	}
}

//
//	Return a HT* value indicating what part of the scrollbar was clicked
//	Rectangle is not adjusted
//
static UINT GetHorzPortion(SCROLLBAR *sb, HWND hwnd, RECT *rect, int x, int y)
{
	RECT rc = *rect;

	if(y < rc.top || y >= rc.bottom) return HTSCROLL_NONE;

#ifdef INCLUDE_BUTTONS

	if(sb->fButVisibleBefore) 
	{
		//clicked on the buttons to the left of the scrollbar
		if(x >= rc.left && x < rc.left + sb->nButSizeBefore)
			return HTSCROLL_INSERTED;

		//adjust the rectangle to exclude the left-side buttons, now that we
		//know we havn't clicked on them
		rc.left  += sb->nButSizeBefore;
	}

	if(sb->fButVisibleAfter)
	{
		//clicked on the buttons to the right of the scrollbar
		if(x >= rc.right - sb->nButSizeAfter && x < rc.right)
			return HTSCROLL_INSERTED;

		//adjust the rectangle to exclude the right-side buttons, now that we
		//know we havn't clicked on them
		rc.right -= sb->nButSizeAfter;
	}

#endif INCLUDE_BUTTONS

	//Now we have the rectangle for the scrollbar itself, so work out
	//what part we clicked on.
	return GetHorzScrollPortion(sb, hwnd, &rc, x, y);
}

//
//	Just call the horizontal version, with adjusted coordinates
//
static UINT GetVertPortion(SCROLLBAR *sb, HWND hwnd, RECT *rect, int x, int y)
{
	UINT ret;
	RotateRect(rect);
	ret = GetHorzPortion(sb, hwnd, rect, y, x);
	RotateRect(rect);
	return ret;
}

//
//	Wrapper function for GetHorzPortion and GetVertPortion
//
static UINT GetPortion(SCROLLBAR *sb, HWND hwnd, RECT *rect, int x, int y)
{
	if(sb->nBarType == SB_HORZ)
		return GetHorzPortion(sb, hwnd, rect, x, y);
	else if(sb->nBarType == SB_VERT)
		return GetVertPortion(sb, hwnd, rect, x, y);
	else
		return HTSCROLL_NONE;
}

//
//	Input: rectangle of the total scrollbar area
//	Output: adjusted to take the inserted buttons into account
//
static void GetRealHorzScrollRect(SCROLLBAR *sb, RECT *rect)
{
	if(sb->fButVisibleBefore) rect->left += sb->nButSizeBefore;
	if(sb->fButVisibleAfter)  rect->right -= sb->nButSizeAfter;
}

//
//	Input: rectangle of the total scrollbar area
//	Output: adjusted to take the inserted buttons into account
//
static void GetRealVertScrollRect(SCROLLBAR *sb, RECT *rect)
{
	if(sb->fButVisibleBefore) rect->top += sb->nButSizeBefore;
	if(sb->fButVisibleAfter)  rect->bottom -= sb->nButSizeAfter;
}

//
//	Decide which type of scrollbar we have before calling
//  the real function to do the job
//
static void GetRealScrollRect(SCROLLBAR *sb, RECT *rect)
{
	if(sb->nBarType == SB_HORZ)
	{
		GetRealHorzScrollRect(sb, rect);
	}
	else if(sb->nBarType == SB_VERT)
	{
		GetRealVertScrollRect(sb, rect);
	}
}

//
//	All button code shoule be collected together
//	
//
#ifdef INCLUDE_BUTTONS

//
//	Return the index of the button covering the specified point
//	rect		- rectangle of the whole scrollbar area
//	pt			- screen coords of the mouse
//	fReturnRect - do/don't modify the rect to return the button's area
//
static UINT GetHorzButtonFromPt(SCROLLBAR *sb, RECT *rect, POINT pt, BOOL fReturnRect)
{
	int leftpos = rect->left, rightpos = rect->right;
	int i;
	int butwidth;
	SCROLLBUT *sbut = sb->sbButtons;

	if(!PtInRect(rect, pt))
		return -1;

	if(sb->fButVisibleAfter)
		rightpos -= sb->nButSizeAfter;

	for(i = 0; i < sb->nButtons; i++)
	{
		if(sb->fButVisibleBefore && sbut[i].uPlacement == SBBP_LEFT)
		{
			butwidth = GetSingleButSize(sb, &sbut[i]);
			
			//if the current button is under the specified point
			if(pt.x >= leftpos && pt.x < leftpos + butwidth)
			{
				//if the caller wants us to return the rectangle of the button
				if(fReturnRect)
				{
					rect->left  = leftpos;
					rect->right = leftpos + butwidth;
				}

				return i;
			}

			leftpos += butwidth;
		}
		else if(sb->fButVisibleAfter && sbut[i].uPlacement == SBBP_RIGHT)
		{
			butwidth = GetSingleButSize(sb, &sbut[i]);

			//if the current button is under the specified point
			if(pt.x >= rightpos && pt.x < rightpos + butwidth)
			{
				//if the caller wants us to return the rectangle of the button
				if(fReturnRect)
				{
					rect->left  = rightpos;
					rect->right = rightpos + butwidth;
				}
				return i;
			}

			rightpos += butwidth;
		}
	}

	return -1;
}


static UINT GetVertButtonFromPt(SCROLLBAR *sb, RECT *rect, POINT pt, BOOL fReturnRect)
{
	UINT ret;
	int temp;
	
	//swap the X/Y coords
	temp = pt.x;
	pt.x = pt.y;
	pt.y = temp;

	//swap the rectangle
	RotateRect(rect);
	
	ret = GetHorzButtonFromPt(sb, rect, pt, fReturnRect);

	RotateRect(rect);
	return ret;
}

//
//
//
static UINT GetButtonFromPt(SCROLLBAR *sb, RECT *rect, POINT pt, BOOL fReturnRect)
{
	if(sb->nBarType == SB_HORZ)
	{
		return GetHorzButtonFromPt(sb, rect, pt, fReturnRect);
	}
	else
	{
		return GetVertButtonFromPt(sb, rect, pt, fReturnRect);
	}
}

//
//	Find the coordinates (in RECT format) of the specified button index
//
static UINT GetHorzButtonRectFromId(SCROLLBAR *sb, RECT *rect, UINT index)
{
	UINT i;
	SCROLLBUT *sbut = sb->sbButtons;
	int leftpos = rect->left, rightpos = rect->right;

	if(sb->fButVisibleAfter)
		rightpos -= sb->nButSizeAfter;

	//find the particular button in question
	for(i = 0; i < index; i++)
	{
		if(sb->fButVisibleBefore && sbut[i].uPlacement == SBBP_LEFT)
		{
			leftpos += GetSingleButSize(sb, &sbut[i]);
		}
		else if(sb->fButVisibleAfter && sbut[i].uPlacement == SBBP_RIGHT)
		{
			rightpos += GetSingleButSize(sb, &sbut[i]);
		}
	}

	//now return the rectangle
	if(sbut[i].uPlacement == SBBP_LEFT)
	{
		rect->left  = leftpos;
		rect->right = leftpos + GetSingleButSize(sb, &sbut[i]);
	}
	else
	{
		rect->left  = rightpos;
		rect->right = rightpos + GetSingleButSize(sb, &sbut[i]);
	}

	return 0;
}

static UINT GetVertButtonRectFromId(SCROLLBAR *sb, RECT *rect, UINT index)
{
	UINT ret;
	RotateRect(rect);
	ret = GetHorzButtonRectFromId(sb, rect, index);
	RotateRect(rect);
	return ret;
}

static UINT GetButtonRectFromId(SCROLLBAR *sb, RECT *rect, UINT index)
{
	if(sb->nBarType == SB_HORZ)
	{
		return GetHorzButtonRectFromId(sb, rect, index);
	}
	else
	{
		return GetVertButtonRectFromId(sb, rect, index);
	}
}
#endif //INCLUDE_BUTTONS	

//
//	Left button click in the non-client area
//
static LRESULT NCLButtonDown(SCROLLWND *sw, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	RECT rect, winrect;
	HDC hdc;
	SCROLLBAR *sb;
	SCROLLBUT *sbut = 0;
	POINT pt;

	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);

	hwndCurCoolSB = hwnd;

	//
	//	HORIZONTAL SCROLLBAR PROCESSING
	//
	if(wParam == HTHSCROLL)
	{
		uScrollTimerMsg = WM_HSCROLL;
		uCurrentScrollbar = SB_HORZ;
		sb = &sw->sbarHorz;

		//get the total area of the normal Horz scrollbar area
		GetHScrollRect(sw, hwnd, &rect);
		uCurrentScrollPortion = GetHorzPortion(sb, hwnd, &rect, LOWORD(lParam), HIWORD(lParam));
	}
	//
	//	VERTICAL SCROLLBAR PROCESSING
	//
	else if(wParam == HTVSCROLL)
	{
		uScrollTimerMsg = WM_VSCROLL;
		uCurrentScrollbar = SB_VERT;
		sb = &sw->sbarVert;

		//get the total area of the normal Horz scrollbar area
		GetVScrollRect(sw, hwnd, &rect);
		uCurrentScrollPortion = GetVertPortion(sb, hwnd, &rect, LOWORD(lParam), HIWORD(lParam));
	}
	//
	//	NORMAL PROCESSING
	//
	else
	{
		uCurrentScrollPortion = HTSCROLL_NONE;
		return CallWindowProc(sw->oldproc, hwnd, WM_NCLBUTTONDOWN, wParam, lParam);
	}

	//
	// we can now share the same code for vertical
	// and horizontal scrollbars
	//
	switch(uCurrentScrollPortion)
	{
	//inserted buttons to the left/right
#ifdef INCLUDE_BUTTONS
	case HTSCROLL_INSERTED:  

#ifdef HOT_TRACKING
		KillTimer(hwnd, uMouseOverId);
		uMouseOverId = 0;
		uMouseOverScrollbar = COOLSB_NONE;
#endif

		//find the index of the button that has been clicked
		//adjust the rectangle to give the button's rectangle
		uCurrentButton = GetButtonFromPt(sb, &rect, pt, TRUE);

		sbut = &sb->sbButtons[uCurrentButton];
		
		//post a notification message
		PostMouseNotify(hwnd, NM_CLICK, sb->nBarType, &rect, sbut->uCmdId, pt);

		GetWindowRect(hwnd, &winrect);
		OffsetRect(&rect, -winrect.left, -winrect.top);
		hdc = GetWindowDC(hwnd);
			
		DrawScrollButton(sbut, hdc, &rect, SBBS_PUSHED);

		ReleaseDC(hwnd, hdc);
	
		break;
#endif	//INCLUDE_BUTTONS

	case HTSCROLL_THUMB: 

		//if the scrollbar is disabled, then do no further processing
		if(!IsScrollbarActive(sb))
			return 0;
		
		GetRealScrollRect(sb, &rect);
		RotateRect0(sb, &rect);
		CalcThumbSize(sb, &rect, &nThumbSize, &nThumbPos);
		RotateRect0(sb, &rect);
		
		//remember the bounding rectangle of the scrollbar work area
		rcThumbBounds = rect;
		
		sw->fThumbTracking = TRUE;
		sb->scrollInfo.nTrackPos = sb->scrollInfo.nPos;
		
		if(wParam == HTVSCROLL) 
			nThumbMouseOffset = pt.y - nThumbPos;
		else
			nThumbMouseOffset = pt.x - nThumbPos;

		nLastPos = -sb->scrollInfo.nPos;
		nThumbPos0 = nThumbPos;
	
		//if(sb->fFlatScrollbar)
		//{
			GetWindowRect(hwnd, &winrect);
			OffsetRect(&rect, -winrect.left, -winrect.top);
			hdc = GetWindowDC(hwnd);
			NCDrawScrollbar(sb, hwnd, hdc, &rect, HTSCROLL_THUMB);
			ReleaseDC(hwnd, hdc);
		//}

		break;

		//Any part of the scrollbar
	case HTSCROLL_LEFT:  
		if(sb->fScrollFlags & ESB_DISABLE_LEFT)		return 0;
		else										goto target1;
	
	case HTSCROLL_RIGHT: 
		if(sb->fScrollFlags & ESB_DISABLE_RIGHT)	return 0;
		else										goto target1;

		goto target1;	

	case HTSCROLL_PAGELEFT:  case HTSCROLL_PAGERIGHT:

		target1:

		//if the scrollbar is disabled, then do no further processing
		if(!IsScrollbarActive(sb))
			break;

		//ajust the horizontal rectangle to NOT include
		//any inserted buttons
		GetRealScrollRect(sb, &rect);

		SendScrollMessage(hwnd, uScrollTimerMsg, uCurrentScrollPortion, 0);

		// Check what area the mouse is now over :
		// If the scroll thumb has moved under the mouse in response to 
		// a call to SetScrollPos etc, then we don't hilight the scrollbar margin
		if(uCurrentScrollbar == SB_HORZ)
			uScrollTimerPortion = GetHorzScrollPortion(sb, hwnd, &rect, pt.x, pt.y);
		else
			uScrollTimerPortion = GetVertScrollPortion(sb, hwnd, &rect, pt.x, pt.y);

		GetWindowRect(hwnd, &winrect);
		OffsetRect(&rect, -winrect.left, -winrect.top);
		hdc = GetWindowDC(hwnd);
			
#ifndef HOT_TRACKING
		//if we aren't hot-tracking, then don't highlight 
		//the scrollbar thumb unless we click on it
		if(uScrollTimerPortion == HTSCROLL_THUMB)
			uScrollTimerPortion = HTSCROLL_NONE;
#endif
		NCDrawScrollbar(sb, hwnd, hdc, &rect, uScrollTimerPortion);
		ReleaseDC(hwnd, hdc);

		//Post the scroll message!!!!
		uScrollTimerPortion = uCurrentScrollPortion;

		//set a timer going on the first click.
		//if this one expires, then we can start off a more regular timer
		//to generate the auto-scroll behaviour
		uScrollTimerId = SetTimer(hwnd, COOLSB_TIMERID1, COOLSB_TIMERINTERVAL1, 0);
		break;
	default:
		return CallWindowProc(sw->oldproc, hwnd, WM_NCLBUTTONDOWN, wParam, lParam);
		//return 0;
	}
		
	SetCapture(hwnd);
	return 0;
}

//
//	Left button released
//
static LRESULT LButtonUp(SCROLLWND *sw, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	RECT rect;
	//UINT thisportion;
	HDC hdc;
	POINT pt;
	RECT winrect;
	UINT buttonIdx = 0;
	
	bMouseClicked = FALSE;
	//current scrollportion is the button that we clicked down on
	if(uCurrentScrollPortion != HTSCROLL_NONE)
	{
		SCROLLBAR *sb = &sw->sbarHorz;
		lParam = GetMessagePos();
		ReleaseCapture();

		GetWindowRect(hwnd, &winrect);
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);

		//emulate the mouse input on a scrollbar here...
		if(uCurrentScrollbar == SB_HORZ)
		{
			//get the total area of the normal Horz scrollbar area
			sb = &sw->sbarHorz;
			GetHScrollRect(sw, hwnd, &rect);
		}
		else if(uCurrentScrollbar == SB_VERT)
		{
			//get the total area of the normal Horz scrollbar area
			sb = &sw->sbarVert;
			GetVScrollRect(sw, hwnd, &rect);
		}

		//we need to do different things depending on if the
		//user is activating the scrollbar itself, or one of
		//the inserted buttons
		switch(uCurrentScrollPortion)
		{
#ifdef INCLUDE_BUTTONS
		//inserted buttons are being clicked
		case HTSCROLL_INSERTED:
			
			//get the rectangle of the ACTIVE button 
			buttonIdx = GetButtonFromPt(sb, &rect, pt, FALSE);
			GetButtonRectFromId(sb, &rect, uCurrentButton);
	
			OffsetRect(&rect, -winrect.left, -winrect.top);

			//Send the notification BEFORE we redraw, so the
			//bitmap can be changed smoothly by the user if they require
			if(uCurrentButton == buttonIdx)
			{
				SCROLLBUT *sbut = &sb->sbButtons[buttonIdx];
				UINT cmdid = sbut->uCmdId;
				
				if((sbut->uButType & SBBT_MASK) == SBBT_TOGGLEBUTTON)
					sbut->uState ^= 1;

				//send a notify??				
				//only post a message if the command id is valid
				if(cmdid != -1 && cmdid > 0)
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(cmdid, CSBN_CLICKED), 0);
			
				//user might have deleted this button, so redraw whole area
				NCPaint(sw, hwnd, 1, 0);
			}
			else
			{
				//otherwise, just redraw the button in its new state
				hdc = GetWindowDC(hwnd);
				DrawScrollButton(&sb->sbButtons[uCurrentButton], hdc, &rect, SBBS_NORMAL);
				ReleaseDC(hwnd, hdc);
			}
	
			break;
#endif	// INCLUDE_BUTTONS

		//The scrollbar is active
		case HTSCROLL_LEFT:  case HTSCROLL_RIGHT: 
		case HTSCROLL_PAGELEFT:  case HTSCROLL_PAGERIGHT: 
		case HTSCROLL_NONE:
			
			KillTimer(hwnd, uScrollTimerId);

		case HTSCROLL_THUMB: 
	
			//In case we were thumb tracking, make sure we stop NOW
			if(sw->fThumbTracking == TRUE)
			{
				SendScrollMessage(hwnd, uScrollTimerMsg, SB_THUMBPOSITION, nLastPos);
				sw->fThumbTracking = FALSE;
			}

			//send the SB_ENDSCROLL message now that scrolling has finished
			SendScrollMessage(hwnd, uScrollTimerMsg, SB_ENDSCROLL, 0);

			//adjust the total scroll area to become where the scrollbar
			//really is (take into account the inserted buttons)
			GetRealScrollRect(sb, &rect);
			OffsetRect(&rect, -winrect.left, -winrect.top);
			hdc = GetWindowDC(hwnd);
			
			//draw whichever scrollbar sb is
			NCDrawScrollbar(sb, hwnd, hdc, &rect, HTSCROLL_NORMAL);

			ReleaseDC(hwnd, hdc);
			break;
		}

		//reset our state to default
		uCurrentScrollPortion = HTSCROLL_NONE;
		uScrollTimerPortion	  = HTSCROLL_NONE;
		uScrollTimerId		  = 0;

		uScrollTimerMsg       = 0;
		uCurrentScrollbar     = COOLSB_NONE;

		return 0;
	}
	else
	{
		/*
		// Can't remember why I did this!
		if(GetCapture() == hwnd)
		{
			ReleaseCapture();
		}*/
	}

	return CallWindowProc(sw->oldproc, hwnd, WM_LBUTTONUP, wParam, lParam);
}

//
//	This function is called whenever the mouse is moved and 
//  we are dragging the scrollbar thumb about.
//
static LRESULT ThumbTrackHorz(SCROLLBAR *sbar, HWND hwnd, int x, int y)
{
	POINT pt;
	RECT rc, winrect, rc2;
	COLORREF crCheck1 = GetSBForeColor();
	COLORREF crCheck2 = GetSBBackColor();
	HDC hdc;
	int thumbpos = nThumbPos;
	int pos;
	int siMaxMin = 0;
	UINT flatflag = sbar->fFlatScrollbar ? BF_FLAT : 0;
	BOOL fCustomDraw = FALSE;

	SCROLLINFO *si;
	si = &sbar->scrollInfo;

	pt.x = x;
	pt.y = y;

	//draw the thumb at whatever position
	rc = rcThumbBounds;

	SetRect(&rc2, rc.left -  THUMBTRACK_SNAPDIST*2, rc.top -    THUMBTRACK_SNAPDIST, 
				  rc.right + THUMBTRACK_SNAPDIST*2, rc.bottom + THUMBTRACK_SNAPDIST);

	rc.left +=  GetScrollMetric(sbar, SM_CXHORZSB);
	rc.right -= GetScrollMetric(sbar, SM_CXHORZSB);

	//if the mouse is not in a suitable distance of the scrollbar,
	//then "snap" the thumb back to its initial position
#ifdef SNAP_THUMB_BACK
	if(!PtInRect(&rc2, pt))
	{
		thumbpos = nThumbPos0;
	}
	//otherwise, move the thumb to where the mouse is
	else
#endif //SNAP_THUMB_BACK
	{
		//keep the thumb within the scrollbar limits
		thumbpos = pt.x - nThumbMouseOffset;
		if(thumbpos < rc.left) thumbpos = rc.left;
		if(thumbpos > rc.right - nThumbSize) thumbpos = rc.right - nThumbSize;
	}

	GetWindowRect(hwnd, &winrect);

	if(sbar->nBarType == SB_VERT)
		RotateRect(&winrect);
	
	hdc = GetWindowDC(hwnd);
		
#ifdef CUSTOM_DRAW
	fCustomDraw = PostCustomPrePostPaint(hwnd, hdc, sbar, CDDS_PREPAINT) == CDRF_SKIPDEFAULT;
#endif

	OffsetRect(&rc, -winrect.left, -winrect.top);
	thumbpos -= winrect.left;

	//draw the margin before the thumb
	SetRect(&rc2, rc.left, rc.top, thumbpos, rc.bottom);
	RotateRect0(sbar, &rc2);

	if(fCustomDraw)
		PostCustomDrawNotify(hwnd, hdc, sbar->nBarType, &rc2, SB_PAGELEFT, 0, 0, 0);
	else
		DrawCheckedRect(hdc, &rc2, crCheck1, crCheck2);
	
	RotateRect0(sbar, &rc2);

	//draw the margin after the thumb 
	SetRect(&rc2, thumbpos+nThumbSize, rc.top, rc.right, rc.bottom);
	
	RotateRect0(sbar, &rc2);
	
	if(fCustomDraw)
		PostCustomDrawNotify(hwnd, hdc, sbar->nBarType, &rc2, SB_PAGERIGHT, 0, 0, 0);
	else
		DrawCheckedRect(hdc, &rc2, crCheck1, crCheck2);
	
	RotateRect0(sbar, &rc2);
	
	//finally draw the thumb itelf. This is how it looks on win2000, anyway
	SetRect(&rc2, thumbpos, rc.top, thumbpos+nThumbSize, rc.bottom);
	
	RotateRect0(sbar, &rc2);

	if(fCustomDraw)
		PostCustomDrawNotify(hwnd, hdc, sbar->nBarType, &rc2, SB_THUMBTRACK, TRUE, TRUE, FALSE);
	else
	{

#ifdef FLAT_SCROLLBARS	
		if(sbar->fFlatScrollbar)
			PaintRect(hdc, &rc2, GetSysColor(COLOR_3DSHADOW));
		else
#endif
		{
				DrawBlankButton(hdc, &rc2, flatflag);
		}
	}

	RotateRect0(sbar, &rc2);
	ReleaseDC(hwnd, hdc);

	//post a SB_TRACKPOS message!!!
	siMaxMin = si->nMax - si->nMin;

	if(siMaxMin > 0)
		pos = MulDiv(thumbpos-rc.left, siMaxMin-si->nPage + 1, rc.right-rc.left-nThumbSize);
	else
		pos = thumbpos - rc.left;

	if(pos != nLastPos)
	{
		si->nTrackPos = pos;	
		SendScrollMessage(hwnd, uScrollTimerMsg, SB_THUMBTRACK, pos);
	}

	nLastPos = pos;

#ifdef CUSTOM_DRAW
	PostCustomPrePostPaint(hwnd, hdc, sbar, CDDS_POSTPAINT);
#endif
	
	return 0;
}

//
//	remember to rotate the thumb bounds rectangle!!
//
static LRESULT ThumbTrackVert(SCROLLBAR *sb, HWND hwnd, int x, int y)
{
	//sw->swapcoords = TRUE;
	RotateRect(&rcThumbBounds);
	ThumbTrackHorz(sb, hwnd, y, x);
	RotateRect(&rcThumbBounds);
	//sw->swapcoords = FALSE;

	return 0;
}

//
//	Called when we have set the capture from the NCLButtonDown(...)
//	
static LRESULT MouseMove(SCROLLWND *sw, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	RECT rect;
	UINT thisportion;
	HDC hdc;
	static UINT lastportion = 0;
	static UINT lastbutton = 0;
	POINT pt;
	RECT winrect;
	UINT buttonIdx = 0;

	if(sw->fThumbTracking == TRUE)
	{
		int x, y;
		lParam = GetMessagePos();
		x = LOWORD(lParam);
		y = HIWORD(lParam);

		bMouseClicked = TRUE;

		if(uCurrentScrollbar == SB_HORZ)
			return ThumbTrackHorz(&sw->sbarHorz, hwnd, x,y);


		else if(uCurrentScrollbar == SB_VERT)
			return ThumbTrackVert(&sw->sbarVert, hwnd, x,y);
	}

	if(uCurrentScrollPortion == HTSCROLL_NONE)
	{
		return CallWindowProc(sw->oldproc, hwnd, WM_MOUSEMOVE, wParam, lParam);
	}
	else
	{
		LPARAM nlParam;
		SCROLLBAR *sb = &sw->sbarHorz;
		SCROLLBUT *sbut = 0;

		nlParam = GetMessagePos();

		GetWindowRect(hwnd, &winrect);

		pt.x = LOWORD(nlParam);
		pt.y = HIWORD(nlParam);

		//emulate the mouse input on a scrollbar here...
		if(uCurrentScrollbar == SB_HORZ)
		{
			sb = &sw->sbarHorz;
		}
		else if(uCurrentScrollbar == SB_VERT)
		{
			sb = &sw->sbarVert;
		}

		//get the total area of the normal scrollbar area
		GetScrollRect(sw, sb->nBarType, hwnd, &rect);
		
		//see if we clicked in the inserted buttons / normal scrollbar
		//thisportion = GetPortion(sb, hwnd, &rect, LOWORD(lParam), HIWORD(lParam));
		thisportion = GetPortion(sb, hwnd, &rect, pt.x, pt.y);
		
		//we need to do different things depending on if the
		//user is activating the scrollbar itself, or one of
		//the inserted buttons
		switch(uCurrentScrollPortion)
		{
#ifdef INCLUDE_BUTTONS
		//inserted buttons are being clicked
		case HTSCROLL_INSERTED:
			
			//find the index of the button that has been clicked
			//Don't adjust the rectangle though
			buttonIdx = GetButtonFromPt(sb, &rect, pt, FALSE);
						
			//Get the rectangle of the active button
			GetButtonRectFromId(sb, &rect, uCurrentButton);

			//if the button to the LEFT of the current 
			//button is resizable, then resize it
#ifdef RESIZABLE_BUTTONS
			if(uCurrentButton > 0)
			{
				sbut = &sb->sbButtons[uCurrentButton - 1];
			
				//only resize if BOTH buttons are on same side of scrollbar
				if(sbut->uPlacement == (sbut+1)->uPlacement && (sbut->uButType & SBBM_RESIZABLE))
				{
					int oldsize = sbut->nSize;
					int butsize1, butsize2;
					RECT rect2;
					int scrollsize;

					if(uCurrentScrollbar == SB_HORZ)
					{
						rect.left -= GetSingleButSize(sb, sbut);
						sbut->nSize = pt.x - rect.left;
					}
					else
					{
						rect.top -= GetSingleButSize(sb, sbut);
						sbut->nSize = pt.y - rect.top;
					}

					//if(sbut->nSize < 0)	sbut->nSize = 0;
					if(sbut->nSize < (int)sbut->nMinSize)
						sbut->nSize = sbut->nMinSize;

					if((UINT)sbut->nSize > (UINT)sbut->nMaxSize)
						sbut->nSize = sbut->nMaxSize;
					
					GetScrollRect(sw, uCurrentScrollbar, hwnd, &rect2);
					
					if(uCurrentScrollbar == SB_HORZ)
						scrollsize = rect2.right-rect2.left;
					else
						scrollsize = rect2.bottom-rect2.top;

					butsize1 = GetButtonSize(sb, hwnd, SBBP_LEFT);
					butsize2 = GetButtonSize(sb, hwnd, SBBP_RIGHT);

					//adjust the button size if it gets too big
					if(butsize1 + butsize2 > scrollsize  - MINSCROLLSIZE)
					{
						sbut->nSize -= (butsize1+butsize2) - (scrollsize - MINSCROLLSIZE);
					}
					
					//remember what size the USER set the button to
					sbut->nSizeReserved = sbut->nSize;
					NCPaint(sw, hwnd, (WPARAM)1, (LPARAM)0);
					return 0;
				}
			}
#endif	//RESIZABLE_BUTTONS			
			
			OffsetRect(&rect, -winrect.left, -winrect.top);

			hdc = GetWindowDC(hwnd);
			
			//if the button under the mouse is not the active button,
			//then display the active button in its normal state
			if(buttonIdx != uCurrentButton 
				//include this if toggle buttons always stay depressed
				//if they are being activated
				&& (sb->sbButtons[uCurrentButton].uButType & SBBT_MASK) != SBBT_TOGGLEBUTTON)
			{
				if(lastbutton != buttonIdx)
					DrawScrollButton(&sb->sbButtons[uCurrentButton], hdc, &rect, SBBS_NORMAL);
			}
			//otherwise, depress the active button if the mouse is over
			//it (just like a normal scroll button works)
			else
			{
				if(lastbutton != buttonIdx)
					DrawScrollButton(&sb->sbButtons[uCurrentButton], hdc, &rect, SBBS_PUSHED);
			}

			ReleaseDC(hwnd, hdc);
			return CallWindowProc(sw->oldproc, hwnd, WM_MOUSEMOVE, wParam, lParam);
			//break;

#endif	//INCLUDE_BUTTONS

		//The scrollbar is active
		case HTSCROLL_LEFT:		 case HTSCROLL_RIGHT:case HTSCROLL_THUMB: 
		case HTSCROLL_PAGELEFT:  case HTSCROLL_PAGERIGHT: 
		case HTSCROLL_NONE:
			
			//adjust the total scroll area to become where the scrollbar
			//really is (take into account the inserted buttons)
			GetRealScrollRect(sb, &rect);

			OffsetRect(&rect, -winrect.left, -winrect.top);
			hdc = GetWindowDC(hwnd);
		
			if(thisportion != uCurrentScrollPortion)
			{
				uScrollTimerPortion = HTSCROLL_NONE;

				if(lastportion != thisportion)
					NCDrawScrollbar(sb, hwnd, hdc, &rect, HTSCROLL_NORMAL);
			}
			//otherwise, draw the button in its depressed / clicked state
			else
			{
				uScrollTimerPortion = uCurrentScrollPortion;

				if(lastportion != thisportion)
					NCDrawScrollbar(sb, hwnd, hdc, &rect, thisportion);
			}

			ReleaseDC(hwnd, hdc);

			break;
		}


		lastportion = thisportion;
		lastbutton  = buttonIdx;

		//must return zero here, because we might get cursor anomilies
		//CallWindowProc(sw->oldproc, hwnd, WM_MOUSEMOVE, wParam, lParam);
		return 0;
		
	}
}

#ifdef INCLUDE_BUTTONS
#ifdef RESIZABLE_BUTTONS
//
//	Any resizable buttons must be shrunk to fit if the window is made too small
//
static void ResizeButtonsToFit(SCROLLWND *sw, SCROLLBAR *sbar, HWND hwnd)
{
	int butsize1, butsize2;
	RECT rc;
	int scrollsize;
	int i;
	SCROLLBUT *sbut;

	//make sure that the scrollbar can fit into space, by
	//shrinking any resizable buttons
	GetScrollRect(sw, sbar->nBarType, hwnd, &rc);

	if(sbar->nBarType == SB_HORZ)
		scrollsize = rc.right-rc.left;
	else
		scrollsize = rc.bottom-rc.top;

	//restore any resizable buttons to their user-defined sizes,
	//before shrinking them to fit. This means when we make the window
	//bigger, the buttons will restore to their initial sizes
	for(i = 0; i < sbar->nButtons; i++)
	{
		sbut = &sbar->sbButtons[i];
		if(sbut->uButType & SBBM_RESIZABLE)
		{
			sbut->nSize = sbut->nSizeReserved;
		}
	}
	
	butsize1 = GetButtonSize(sbar, hwnd, SBBP_LEFT);
	butsize2 = GetButtonSize(sbar, hwnd, SBBP_RIGHT);

	if(butsize1 + butsize2 > scrollsize - MINSCROLLSIZE)
	{
		i = 0;
		while(i < sbar->nButtons && 
			butsize1 + butsize2 > scrollsize - MINSCROLLSIZE)
		{
			sbut = &sbar->sbButtons[i++];
			if(sbut->uButType & SBBM_RESIZABLE)
			{
				int oldsize = sbut->nSize;
				sbut->nSize -= (butsize1+butsize2) - (scrollsize-MINSCROLLSIZE);

				if(sbut->nSize < (int)sbut->nMinSize)
					sbut->nSize = sbut->nMinSize;

				if((UINT)sbut->nSize > (UINT)sbut->nMaxSize)
					sbut->nSize = sbut->nMaxSize;

				
				butsize1 -= (oldsize - sbut->nSize);
			}
		}
	}

}
#endif
#endif

//
//	We must allocate from in the non-client area for our scrollbars
//	Call the default window procedure first, to get the borders (if any)
//	allocated some space, then allocate the space for the scrollbars
//	if they fit
//
static LRESULT NCCalcSize(SCROLLWND *sw, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	NCCALCSIZE_PARAMS *nccsp;
	RECT *rect;
	RECT oldrect;
	BOOL fCalcValidRects = (wParam == TRUE);
	SCROLLBAR *sb;
	UINT ret;
	DWORD dwStyle;

	//Regardless of the value of fCalcValidRects, the first rectangle 
	//in the array specified by the rgrc structure member of the 
	//NCCALCSIZE_PARAMS structure contains the coordinates of the window,
	//so we can use the exact same code to modify this rectangle, when
	//wParam is TRUE and when it is FALSE.
	nccsp = (NCCALCSIZE_PARAMS *)lParam;
	rect = &nccsp->rgrc[0];
	oldrect = *rect;

	dwStyle = GetWindowLong(hwnd, GWL_STYLE);

	// TURN OFF SCROLL-STYLES.
    if ( dwStyle & (WS_VSCROLL|WS_HSCROLL) )
    {
        sw->bPreventStyleChange = TRUE;
        SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~(WS_VSCROLL|WS_HSCROLL));
    }
	
	//call the default procedure to get the borders allocated
	ret = CallWindowProc(sw->oldproc, hwnd, WM_NCCALCSIZE, wParam, lParam);

	// RESTORE PREVIOUS STYLES (if present at all)
    if ( dwStyle & (WS_VSCROLL|WS_HSCROLL) )
    {
        SetWindowLong(hwnd, GWL_STYLE, dwStyle);
        sw->bPreventStyleChange = FALSE;
    }

	// calculate what the size of each window border is,
	sw->cxLeftEdge   = rect->left     - oldrect.left;
	sw->cxRightEdge  = oldrect.right  - rect->right;
	sw->cyTopEdge    = rect->top      - oldrect.top;
	sw->cyBottomEdge = oldrect.bottom - rect->bottom;

	sb = &sw->sbarHorz;

	//if there is room, allocate some space for the horizontal scrollbar
	//NOTE: Change the ">" to a ">=" to make the horz bar totally fill the
	//window before disappearing
	if((sb->fScrollFlags & CSBS_VISIBLE) && 
#ifdef COOLSB_FILLWINDOW
		rect->bottom - rect->top >= GetScrollMetric(sb, SM_CYHORZSB))
#else
		rect->bottom - rect->top > GetScrollMetric(sb, SM_CYHORZSB))
#endif
	{
		rect->bottom -= GetScrollMetric(sb, SM_CYHORZSB);
		sb->fScrollVisible = TRUE;
	}
	else
		sb->fScrollVisible = FALSE;

	sb = &sw->sbarVert;

	//if there is room, allocate some space for the vertical scrollbar
	if((sb->fScrollFlags & CSBS_VISIBLE) && 
		rect->right - rect->left >= GetScrollMetric(sb, SM_CXVERTSB))
	{
		if(sw->fLeftScrollbar)
			rect->left  += GetScrollMetric(sb, SM_CXVERTSB);
		else
			rect->right -= GetScrollMetric(sb, SM_CXVERTSB);

		sb->fScrollVisible = TRUE;
	}
	else
		sb->fScrollVisible = FALSE;

#ifdef INCLUDE_BUTTONS
#ifdef RESIZABLE_BUTTONS
	ResizeButtonsToFit(sw, &sw->sbarHorz, hwnd);
	ResizeButtonsToFit(sw, &sw->sbarVert, hwnd);
#endif
#endif

	//don't return a value unless we actually modify the other rectangles
	//in the NCCALCSIZE_PARAMS structure. In this case, we return 0
	//no matter what the value of fCalcValidRects is
	return ret;//FALSE;
}

//
//	used for hot-tracking over the scroll buttons
//
static LRESULT NCMouseMove(SCROLLWND *sw, HWND hwnd, WPARAM wHitTest, LPARAM lParam)
{
	//install a timer for the mouse-over events, if the mouse moves
	//over one of the scrollbars
#ifdef HOT_TRACKING
	hwndCurCoolSB = hwnd;
	if(wHitTest == HTHSCROLL)
	{
		if(uMouseOverScrollbar == SB_HORZ)
			return CallWindowProc(sw->oldproc, hwnd, WM_NCMOUSEMOVE, wHitTest, lParam);

		uLastHitTestPortion = HTSCROLL_NONE;
		uHitTestPortion     = HTSCROLL_NONE;
		GetScrollRect(sw, SB_HORZ, hwnd, &MouseOverRect);
		uMouseOverScrollbar = SB_HORZ;
		uMouseOverId = SetTimer(hwnd, COOLSB_TIMERID3, COOLSB_TIMERINTERVAL3, 0);

		NCPaint(sw, hwnd, 1, 0);
	}
	else if(wHitTest == HTVSCROLL)
	{
		if(uMouseOverScrollbar == SB_VERT)
			return CallWindowProc(sw->oldproc, hwnd, WM_NCMOUSEMOVE, wHitTest, lParam);

		uLastHitTestPortion = HTSCROLL_NONE;
		uHitTestPortion     = HTSCROLL_NONE;
		GetScrollRect(sw, SB_VERT, hwnd, &MouseOverRect);
		uMouseOverScrollbar = SB_VERT;
		uMouseOverId = SetTimer(hwnd, COOLSB_TIMERID3, COOLSB_TIMERINTERVAL3, 0);

		NCPaint(sw, hwnd, 1, 0);
	}

#endif //HOT_TRACKING
	return CallWindowProc(sw->oldproc, hwnd, WM_NCMOUSEMOVE, wHitTest, lParam);
}

//
//	Timer routine to generate scrollbar messages
//
static LRESULT CoolSB_Timer(SCROLLWND *swnd, HWND hwnd, WPARAM wTimerId, LPARAM lParam)
{
	//let all timer messages go past if we don't have a timer installed ourselves
	if(uScrollTimerId == 0 && uMouseOverId == 0)
	{
		return CallWindowProc(swnd->oldproc, hwnd, WM_TIMER, wTimerId, lParam);
	}

#ifdef HOT_TRACKING
	//mouse-over timer
	if(wTimerId == COOLSB_TIMERID3)
	{
		POINT pt;
		RECT rect, winrect;
		HDC hdc;
		SCROLLBAR *sbar;

		if(swnd->fThumbTracking)
			return 0;

		//if the mouse moves outside the current scrollbar,
		//then kill the timer..
		GetCursorPos(&pt);

		if(!PtInRect(&MouseOverRect, pt))
		{
			KillTimer(hwnd, uMouseOverId);
			uMouseOverId = 0;
			uMouseOverScrollbar = COOLSB_NONE;
			uLastHitTestPortion = HTSCROLL_NONE;

			uHitTestPortion = HTSCROLL_NONE;
			NCPaint(swnd, hwnd, 1, 0);
		}
		else
		{
			if(uMouseOverScrollbar == SB_HORZ)
			{
				sbar = &swnd->sbarHorz;
				uHitTestPortion = GetHorzPortion(sbar, hwnd, &MouseOverRect, pt.x, pt.y);
			}
			else
			{
				sbar = &swnd->sbarVert;
				uHitTestPortion = GetVertPortion(sbar, hwnd, &MouseOverRect, pt.x, pt.y);
			}

			if(uLastHitTestPortion != uHitTestPortion)
			{
				rect = MouseOverRect;
				GetRealScrollRect(sbar, &rect);

				GetWindowRect(hwnd, &winrect);
				OffsetRect(&rect, -winrect.left, -winrect.top);

				hdc = GetWindowDC(hwnd);
				NCDrawScrollbar(sbar, hwnd, hdc, &rect, HTSCROLL_NONE);
				ReleaseDC(hwnd, hdc);
			}
			
			uLastHitTestPortion = uHitTestPortion;
		}

		return 0;
	}
#endif // HOT_TRACKING

	//if the first timer goes off, then we can start a more
	//regular timer interval to auto-generate scroll messages
	//this gives a slight pause between first pressing the scroll arrow, and the
	//actual scroll starting
	if(wTimerId == COOLSB_TIMERID1)
	{
		KillTimer(hwnd, uScrollTimerId);
		uScrollTimerId = SetTimer(hwnd, COOLSB_TIMERID2, COOLSB_TIMERINTERVAL2, 0);
		return 0;
	}
	//send the scrollbar message repeatedly
	else if(wTimerId == COOLSB_TIMERID2)
	{
		//need to process a spoof WM_MOUSEMOVE, so that
		//we know where the mouse is each time the scroll timer goes off.
		//This is so we can stop sending scroll messages if the thumb moves
		//under the mouse.
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hwnd, &pt);
		
		MouseMove(swnd, hwnd, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));

		if(uScrollTimerPortion != HTSCROLL_NONE)
			SendScrollMessage(hwnd, uScrollTimerMsg, uScrollTimerPortion, 0);
		
		return 0;
	}
	else
	{
		return CallWindowProc(swnd->oldproc, hwnd, WM_TIMER, wTimerId, lParam);
	}
}

//
//	We must intercept any calls to SetWindowLong, to check if
//  left-scrollbars are taking effect or not
//
static LRESULT CoolSB_StyleChange(SCROLLWND *swnd, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	STYLESTRUCT *ss = (STYLESTRUCT *)lParam;

	if(wParam == GWL_EXSTYLE)
	{
		if(ss->styleNew & WS_EX_LEFTSCROLLBAR)
			swnd->fLeftScrollbar = TRUE;
		else
			swnd->fLeftScrollbar = FALSE;
	}

	return CallWindowProc(swnd->oldproc, hwnd, msg, wParam, lParam);
}

static UINT curTool = -1;
static LRESULT CoolSB_Notify(SCROLLWND *swnd, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
#ifdef COOLSB_TOOLTIPS

	NMTTDISPINFO *nmdi = (NMTTDISPINFO *)lParam;

	if(nmdi->hdr.hwndFrom == swnd->hwndToolTip &&
		nmdi->hdr.code == TTN_GETDISPINFO)
	{
		//convert the tooltip notify from a "ISHWND" style
		//request to an id-based request. 
		//We do this because our tooltip is a window-style
		//tip, with no tools, and the GETDISPINFO request must
		//indicate which button to retrieve the text for
		//nmdi->hdr.idFrom = curTool;
		nmdi->hdr.idFrom = curTool;
		nmdi->hinst = GetModuleHandle(0);
		nmdi->uFlags &= ~TTF_IDISHWND;
	}
#endif	//COOLSB_TOOLTIPS

	return CallWindowProc(swnd->oldproc, hwnd, WM_NOTIFY, wParam, lParam);	
}

static LRESULT SendToolTipMessage0(HWND hwndTT, UINT message, WPARAM wParam, LPARAM lParam)
{
	return SendMessage(hwndTT, message, wParam, lParam);
}

#ifdef COOLSB_TOOLTIPS
#define SendToolTipMessage		SendToolTipMessage0
#else
#define SendToolTipMessage		1 ? (void)0 : SendToolTipMessage0
#endif


//
//	We must intercept any calls to SetWindowLong, to make sure that
//	the user does not set the WS_VSCROLL or WS_HSCROLL styles
//
static LRESULT CoolSB_SetCursor(SCROLLWND *swnd, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
#ifdef INCLUDE_BUTTONS
	UINT lo = LOWORD(lParam);
	UINT hi = HIWORD(lParam);
	UINT xy;
	RECT rect;
	SCROLLBAR *sbar;
	SCROLLBUT *sbut;
	POINT pt;
	UINT id;
	static UINT lastid;

#ifdef HIDE_CURSOR_AFTER_MOUSEUP
	static UINT lastmsg;
	if(lastmsg == WM_LBUTTONDOWN)
	{
		lastmsg =  hi;
		return CallWindowProc(swnd->oldproc, hwnd, WM_SETCURSOR, wParam, lParam);	
	}
	else
		lastmsg =  hi;
#endif
	
	//if we are over either or our scrollbars
	if(lo == HTHSCROLL || lo == HTVSCROLL)
	{
		xy = GetMessagePos();
		pt.x = LOWORD(xy);
		pt.y = HIWORD(xy);

		if(lo == HTHSCROLL)
		{
			sbar = &swnd->sbarHorz;
			GetScrollRect(swnd, SB_HORZ, hwnd, &rect);
			id = GetHorzPortion(sbar, hwnd, &rect, pt.x, pt.y);
		}
		else
		{
			sbar = &swnd->sbarVert;
			GetScrollRect(swnd, SB_VERT, hwnd, &rect);
			id = GetVertPortion(sbar, hwnd, &rect, pt.x, pt.y);
		}

		if(id != HTSCROLL_INSERTED)
		{
			if(swnd->hwndToolTip != 0)
			{
				SendToolTipMessage(swnd->hwndToolTip, TTM_ACTIVATE, FALSE, 0);
				SendToolTipMessage(swnd->hwndToolTip, TTM_POP, 0, 0);
			}

			return CallWindowProc(swnd->oldproc, hwnd, WM_SETCURSOR, wParam, lParam);
		}
		
		if(swnd->hwndToolTip != 0)
		{
			SendToolTipMessage(swnd->hwndToolTip, TTM_ACTIVATE, TRUE, 0);
		}

		//set the cursor if one has been specified
		if((id = GetButtonFromPt(sbar, &rect, pt, TRUE)) != -1)
		{
			sbut = &sbar->sbButtons[id];
			curTool = sbut->uCmdId;

			if(lastid != id && swnd->hwndToolTip != 0)
			{
				if(IsWindowVisible(swnd->hwndToolTip))
					SendToolTipMessage(swnd->hwndToolTip, TTM_UPDATE, TRUE, 0);
			}

			lastid = id;

			if(sbut->hCurs != 0)
			{
				SetCursor(sbut->hCurs);
				return 0;
			}
		}
		else
		{
			curTool = -1;
			lastid = -1;
		}
	}
	else if(swnd->hwndToolTip != 0)
	{
		SendToolTipMessage(swnd->hwndToolTip, TTM_ACTIVATE, FALSE, 0);
		SendToolTipMessage(swnd->hwndToolTip, TTM_POP, 0, 0);
	}

#endif	//INCLUDE_BUTTONS
	return CallWindowProc(swnd->oldproc, hwnd, WM_SETCURSOR, wParam, lParam);
}


//
//	Send the specified message to the tooltip control
//
static void __stdcall RelayMouseEvent(HWND hwnd, HWND hwndToolTip, UINT event)
{
#ifdef COOLSB_TOOLTIPS
	MSG msg;

	CoolSB_ZeroMemory(&msg, sizeof(MSG));
	msg.hwnd = hwnd;
	msg.message = event;

	SendMessage(hwndToolTip, TTM_RELAYEVENT, 0, (LONG)&msg);
#else
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hwndToolTip);
	UNREFERENCED_PARAMETER(event);
#endif
}


//
//  CoolScrollbar subclass procedure.
//	Handle all messages needed to mimick normal windows scrollbars
//
LRESULT CALLBACK CoolSBWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC oldproc;
	SCROLLWND *swnd = GetScrollWndFromHwnd(hwnd);
	static int count;

	switch(message)
	{
	case WM_NCDESTROY:
		//this should NEVER be called, because the user
		//should have called Uninitialize() themselves.

		//However, if the user tries to call Uninitialize().. 
		//after this window is destroyed, this window's entry in the lookup
		//table will not be there, and the call will fail
		oldproc = swnd->oldproc;
		UninitializeCoolSB(hwnd);
		
		//we must call the original window procedure, otherwise it
		//will never get the WM_NCDESTROY message, and it wouldn't
		//be able to clean up etc.
		return CallWindowProc(oldproc, hwnd, message, wParam, lParam);

	case WM_NCCALCSIZE:
		return NCCalcSize(swnd, hwnd, wParam, lParam);

	case WM_NCPAINT:
		return NCPaint(swnd, hwnd, wParam, lParam);	

	case WM_NCHITTEST:
		return NCHitTest(swnd, hwnd, wParam, lParam);

	case WM_NCRBUTTONDOWN: case WM_NCRBUTTONUP: 
	case WM_NCMBUTTONDOWN: case WM_NCMBUTTONUP: 
		RelayMouseEvent(hwnd, swnd->hwndToolTip, (WM_MOUSEMOVE-WM_NCMOUSEMOVE) + (message));
		if(wParam == HTHSCROLL || wParam == HTVSCROLL) 
			return 0;
		else 
			break;

	case WM_NCLBUTTONDBLCLK:
		//TRACE("WM_NCLBUTTONDBLCLK %d\n", count++);
		if(wParam == HTHSCROLL || wParam == HTVSCROLL)
			return NCLButtonDown(swnd, hwnd, wParam, lParam);
		else
			break;

	case WM_NCLBUTTONDOWN:
		//TRACE("WM_NCLBUTTONDOWN%d\n", count++);
		RelayMouseEvent(hwnd, swnd->hwndToolTip, WM_LBUTTONDOWN);
		return NCLButtonDown(swnd, hwnd, wParam, lParam);


	case WM_LBUTTONUP:
		//TRACE("WM_LBUTTONUP %d\n", count++);
		RelayMouseEvent(hwnd, swnd->hwndToolTip, WM_LBUTTONUP);
		return LButtonUp(swnd, hwnd, wParam, lParam);

	case WM_NOTIFY:
		return CoolSB_Notify(swnd, hwnd, wParam, lParam);

	//Mouse moves are received when we set the mouse capture,
	//even when the mouse moves over the non-client area
	case WM_MOUSEMOVE: 
		//TRACE("WM_MOUSEMOVE %d\n", count++);
		return MouseMove(swnd, hwnd, wParam, lParam);
	
	case WM_TIMER:
		return CoolSB_Timer(swnd, hwnd, wParam, lParam);

	//case WM_STYLECHANGING:
	//	return CoolSB_StyleChange(swnd, hwnd, WM_STYLECHANGING, wParam, lParam);
	case WM_STYLECHANGED:

		if(swnd->bPreventStyleChange)
		{
			// the NCPAINT handler has told us to eat this message!
			return 0;
		}
		else
		{
            if (message == WM_STYLECHANGED) 
				return CoolSB_StyleChange(swnd, hwnd, WM_STYLECHANGED, wParam, lParam);
			else
				break;
		}

	case WM_NCMOUSEMOVE: 
		{
			static LONG lastpos = -1;

			//TRACE("WM_NCMOUSEMOVE %d\n", count++);

			//The problem with NCMOUSEMOVE is that it is sent continuously
			//even when the mouse is stationary (under win2000 / win98)
			//
			//Tooltips don't like being sent a continous stream of mouse-moves
			//if the cursor isn't moving, because they will think that the mouse
			//is moving position, and the internal timer will never expire
			//
			if(lastpos != lParam)
			{
				RelayMouseEvent(hwnd, swnd->hwndToolTip, WM_MOUSEMOVE);
				lastpos = lParam;
			}
		}

		return NCMouseMove(swnd, hwnd, wParam, lParam);


	case WM_SETCURSOR:
		return CoolSB_SetCursor(swnd, hwnd, wParam, lParam);

	case WM_CAPTURECHANGED:
		break;

	default:
		break;
	}
	
	return CallWindowProc(swnd->oldproc, hwnd, message, wParam, lParam);
}



