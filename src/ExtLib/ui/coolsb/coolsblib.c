/*
	
	Cool Scrollbar Library Version 1.2

	Module: coolsblib.c
	Copyright (c) J Brown 2001

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

	 V1.0: Apr 2001: Initial Version

*/

#define STRICT

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "coolscroll.h"
#include "userdefs.h"
#include "coolsb_internal.h"

static TCHAR szPropStr[] = _T("CoolSBSubclassPtr");

LRESULT CALLBACK CoolSBWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

SCROLLWND *GetScrollWndFromHwnd(HWND hwnd)
{
	return (SCROLLWND *)GetProp(hwnd, szPropStr);
}

SCROLLBAR *GetScrollBarFromHwnd(HWND hwnd, UINT nBar)
{
	SCROLLWND *sw = GetScrollWndFromHwnd(hwnd);
	
	if(!sw) return 0;
	
	if(nBar == SB_HORZ)
		return &sw->sbarHorz;
	else if(nBar == SB_VERT)
		return &sw->sbarVert;
	else
		return 0;
}

BOOL WINAPI CoolSB_IsCoolScrollEnabled(HWND hwnd)
{
	if(GetScrollWndFromHwnd(hwnd))
		return TRUE;
	else
		return FALSE;
}

BOOL GetScrollRect(SCROLLWND *sw, UINT nBar, HWND hwnd, RECT *rect);

//
//	Special support for USER32.DLL patching (using Detours library)
//	The only place we call a real scrollbar API is in InitializeCoolSB,
//	where we call EnableScrollbar.
//	
//	We HAVE to call the origial EnableScrollbar function, 
//	so we need to be able to set a pointer to this func when using
//	using Detours (or any other LIB??)
//

static BOOL (WINAPI * pEnableScrollBar) (HWND, UINT, UINT) = 0;

void WINAPI CoolSB_SetESBProc(void *proc)
{
	pEnableScrollBar = proc;
}
//
//

static void RedrawNonClient(HWND hwnd, BOOL fFrameChanged)
{
	if(fFrameChanged == FALSE)
	{
		/*
		RECT rect;
		HRGN hrgn1, hrgn2;
		
		SCROLLWND *sw = GetScrollWndFromHwnd(hwnd);
		
		GetScrollRect(sw, SB_HORZ, hwnd, &rect);
		hrgn1 = CreateRectRgnIndirect(&rect);
		
		GetScrollRect(sw, SB_VERT, hwnd, &rect);
		hrgn2 = CreateRectRgnIndirect(&rect);
		
		CombineRgn(hrgn1, hrgn2, hrgn1, RGN_OR);
		
		SendMessage(hwnd, WM_NCPAINT, (WPARAM)hrgn1, 0);
		
		DeleteObject(hrgn1);
		DeleteObject(hrgn2);*/

		SendMessage(hwnd, WM_NCPAINT, (WPARAM)1, 0);
	}
	else
	{
		SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
			| SWP_FRAMECHANGED | SWP_DRAWFRAME);
	}
}

//
//	return the default minimum size of a scrollbar thumb
//
int WINAPI CoolSB_GetDefaultMinThumbSize(void)
{
	DWORD dwVersion = GetVersion();

	// set the minimum thumb size for a scrollbar. This
	// differs between NT4 and 2000, so need to check to see
	// which platform we are running under
	if(dwVersion < 0x80000000)              // Windows NT/2000
	{
		if(LOBYTE(LOWORD(dwVersion)) >= 5)
			return MINTHUMBSIZE_2000;
		else
			return MINTHUMBSIZE_NT4;
	}
	else
	{
		return MINTHUMBSIZE_NT4;
	}
}

static SCROLLINFO *GetScrollInfoFromHwnd(HWND hwnd, int fnBar)
{
	SCROLLBAR *sb = GetScrollBarFromHwnd(hwnd, fnBar);

	if(sb == 0)
		return FALSE;

	if(fnBar == SB_HORZ)
	{
		return &sb->scrollInfo;
	}
	else if(fnBar == SB_VERT)
	{
		return &sb->scrollInfo;
	}
	else
		return NULL;
}
//
//	Initialize the cool scrollbars for a window by subclassing it
//	and using the coolsb window procedure instead
//
BOOL WINAPI InitializeCoolSB(HWND hwnd, ptr_themeRGB ThemeRGB)
{
	SCROLLWND *sw;
	SCROLLINFO *si;
	INITCOMMONCONTROLSEX ice;
	TOOLINFO ti;
	RECT rect;
	DWORD dwCurStyle;
	//BOOL fDisabled;

	if(pEnableScrollBar == 0)
		pEnableScrollBar = EnableScrollBar;

	GetClientRect(hwnd, &rect);

	//if we have already initialized Cool Scrollbars for this window,
	//then stop the user from doing it again
	if(GetScrollWndFromHwnd(hwnd) != 0)
	{
		return FALSE;
	}

	fThemeRGB = ThemeRGB;

	//allocate a private scrollbar structure which we 
	//will use to keep track of the scrollbar data
	sw = (SCROLLWND *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCROLLWND));

	si = &sw->sbarHorz.scrollInfo;
	si->cbSize = sizeof(SCROLLINFO);
	si->fMask  = SIF_ALL;
	GetScrollInfo(hwnd, SB_HORZ, si);

	si = &sw->sbarVert.scrollInfo;
	si->cbSize = sizeof(SCROLLINFO);
	si->fMask  = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, si);

	//check to see if the window has left-aligned scrollbars
	if(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LEFTSCROLLBAR)
		sw->fLeftScrollbar = TRUE;
	else
		sw->fLeftScrollbar = FALSE;

	dwCurStyle = GetWindowLong(hwnd, GWL_STYLE);

	SetProp(hwnd, szPropStr, (HANDLE)sw);

	//try to enable the scrollbar arrows - if the return value is
	//non-zero, then the scrollbars were previously disabled
	//fDisabled = pEnableScrollBar(hwnd, SB_HORZ, ESB_ENABLE_BOTH);

	//scrollbars will automatically get enabled, even if
	//they aren't to start with....sorry, but there isn't an
	//easy alternative.
	if(dwCurStyle & WS_HSCROLL)
		sw->sbarHorz.fScrollFlags = CSBS_VISIBLE;

	if(dwCurStyle & WS_VSCROLL)
		sw->sbarVert.fScrollFlags = CSBS_VISIBLE;

	//need to be able to distinguish between horizontal and vertical
	//scrollbars in some instances
	sw->sbarHorz.nBarType	     = SB_HORZ;
	sw->sbarVert.nBarType	     = SB_VERT;

	sw->sbarHorz.fFlatScrollbar  = CSBS_NORMAL;
	sw->sbarVert.fFlatScrollbar  = CSBS_NORMAL;

	//set the default arrow sizes for the scrollbars
	sw->sbarHorz.nArrowLength	 = SYSTEM_METRIC;
	sw->sbarHorz.nArrowWidth	 = SYSTEM_METRIC;
	sw->sbarVert.nArrowLength	 = SYSTEM_METRIC;
	sw->sbarVert.nArrowWidth	 = SYSTEM_METRIC;

	sw->bPreventStyleChange		 = FALSE;
	
	sw->oldproc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CoolSBWndProc);

	CoolSB_SetMinThumbSize(hwnd, SB_BOTH, CoolSB_GetDefaultMinThumbSize());

#ifdef COOLSB_TOOLTIPS
	ice.dwSize = sizeof(ice);
	ice.dwICC  = ICC_BAR_CLASSES;
	InitCommonControlsEx(&ice);

	sw->hwndToolTip = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, TOOLTIPS_CLASS, _T(""),
                            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            hwnd, NULL, GetModuleHandle(0),
                            NULL);

	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND;
	ti.hwnd   = hwnd;
	ti.uId    = (UINT)hwnd;
	ti.lpszText = LPSTR_TEXTCALLBACK;
	ti.hinst   = GetModuleHandle(0);

	SendMessage(sw->hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
	
#else

	UNREFERENCED_PARAMETER(ice);
	UNREFERENCED_PARAMETER(ti);
	sw->hwndToolTip = 0;

#endif
	
	//send the window a frame changed message to update the scrollbars
	RedrawNonClient(hwnd, TRUE);

	return TRUE;
}

BOOL WINAPI CoolSB_EnableScrollBar	(HWND hwnd, int wSBflags, UINT wArrows)
{
	SCROLLBAR *sbar;
	UINT oldstate;
	BOOL bFailed = FALSE;

	if(!CoolSB_IsCoolScrollEnabled(hwnd))
		return EnableScrollBar(hwnd, wSBflags, wArrows);

	if((wSBflags == SB_HORZ || wSBflags == SB_BOTH) && 
		(sbar = GetScrollBarFromHwnd(hwnd, SB_HORZ)))
	{
		oldstate = sbar->fScrollFlags;
		
		//clear any existing state, and OR in the disabled flags
		sbar->fScrollFlags = (sbar->fScrollFlags & ~ESB_DISABLE_BOTH) | wArrows;

		if(oldstate == sbar->fScrollFlags)
			bFailed = TRUE;

	}

	if((wSBflags == SB_VERT || wSBflags == SB_BOTH) && 
		(sbar = GetScrollBarFromHwnd(hwnd, SB_VERT)))
	{
		oldstate = sbar->fScrollFlags;
		
		//clear any existing state, and OR in the disabled flags
		sbar->fScrollFlags = (sbar->fScrollFlags & ~ESB_DISABLE_BOTH) | wArrows;

		if(oldstate == sbar->fScrollFlags)
			bFailed = TRUE;
	}

	return !bFailed;
}

BOOL WINAPI CoolSB_GetScrollBarInfo(HWND hwnd)
{
//	SCROLLBARINFO sbi; not defined in winuser.h
	return FALSE;	
}

BOOL WINAPI CoolSB_GetScrollInfo (HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
	SCROLLINFO *mysi;
	BOOL copied = FALSE;
	
	if(!lpsi)
		return FALSE;

	if(!(mysi = GetScrollInfoFromHwnd(hwnd, fnBar)))
	{
		return GetScrollInfo(hwnd, fnBar, lpsi);
	}
	
	if(lpsi->fMask & SIF_PAGE)
	{
		lpsi->nPage = mysi->nPage;
		copied = TRUE;
	}

	if(lpsi->fMask & SIF_POS)
	{
		lpsi->nPos = mysi->nPos;
		copied = TRUE;
	}

	if(lpsi->fMask & SIF_TRACKPOS)
	{
		lpsi->nTrackPos = mysi->nTrackPos;
		copied = TRUE;
	}

	if(lpsi->fMask & SIF_RANGE)
	{
		lpsi->nMin = mysi->nMin;
		lpsi->nMax = mysi->nMax;
		copied = TRUE;
	}

	return copied;
}

int	WINAPI CoolSB_GetScrollPos (HWND hwnd, int nBar)
{
	SCROLLINFO *mysi;
	
	if(!(mysi = GetScrollInfoFromHwnd(hwnd, nBar)))
		return GetScrollPos(hwnd, nBar);

	return mysi->nPos;
}

BOOL WINAPI CoolSB_GetScrollRange (HWND hwnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos)
{
	SCROLLINFO *mysi;
	
	if(!lpMinPos || !lpMaxPos)
		return FALSE;

	if(!(mysi = GetScrollInfoFromHwnd(hwnd, nBar)))
		return GetScrollRange(hwnd, nBar, lpMinPos, lpMaxPos);

	*lpMinPos = mysi->nMin;
	*lpMaxPos = mysi->nMax;

	return TRUE;
}

int	WINAPI CoolSB_SetScrollInfo (HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fRedraw)
{
	SCROLLINFO *mysi;
	SCROLLBAR *sbar;
	BOOL       fRecalcFrame = FALSE;

	if(!lpsi)
		return FALSE;

	if(!(mysi = GetScrollInfoFromHwnd(hwnd, fnBar)))
		return SetScrollInfo(hwnd, fnBar, lpsi, fRedraw);

	//if(CoolSB_IsThumbTracking(hwnd))
	//	return mysi->nPos;

	if(lpsi->fMask & SIF_RANGE)
	{
		mysi->nMin = lpsi->nMin;
		mysi->nMax = lpsi->nMax;
	}

	//The nPage member must specify a value from 0 to nMax - nMin +1. 
	if(lpsi->fMask & SIF_PAGE)
	{
		UINT t = (UINT)(mysi->nMax - mysi->nMin + 1);
		mysi->nPage = min(max(0, lpsi->nPage), t);
	}

	//The nPos member must specify a value between nMin and nMax - max(nPage - 1, 0).
	if(lpsi->fMask & SIF_POS)
	{
		mysi->nPos = max(lpsi->nPos, mysi->nMin);
		mysi->nPos = min((UINT)mysi->nPos, mysi->nMax - max(mysi->nPage - 1, 0));
	}

	sbar = GetScrollBarFromHwnd(hwnd, fnBar);

	if((lpsi->fMask & SIF_DISABLENOSCROLL) || (sbar->fScrollFlags & CSBS_THUMBALWAYS))
	{
		if(!sbar->fScrollVisible)
		{
			CoolSB_ShowScrollBar(hwnd, fnBar, TRUE);
			fRecalcFrame = TRUE;
		}
	}
	else
	{
		if(    mysi->nPage >  (UINT)mysi->nMax 
			|| mysi->nPage == (UINT)mysi->nMax && mysi->nMax == 0
			|| mysi->nMax  <= mysi->nMin)
		{
			if(sbar->fScrollVisible)
			{
				CoolSB_ShowScrollBar(hwnd, fnBar, FALSE);
				fRecalcFrame = TRUE;
			}
		}
		else
		{
			if(!sbar->fScrollVisible)
			{
				CoolSB_ShowScrollBar(hwnd, fnBar, TRUE);
				fRecalcFrame = TRUE;
			}

		}

	}

	if(fRedraw && !CoolSB_IsThumbTracking(hwnd))
		RedrawNonClient(hwnd, fRecalcFrame);
	
	return mysi->nPos;
}


int WINAPI CoolSB_SetScrollPos(HWND hwnd, int nBar, int nPos, BOOL fRedraw)
{
	SCROLLINFO *mysi;
	int oldpos;
	
	if(!(mysi = GetScrollInfoFromHwnd(hwnd, nBar)))
	{
		return SetScrollPos(hwnd, nBar, nPos, fRedraw);
	}

	//this is what should happen, but real scrollbars don't work like this..
	//if(CoolSB_IsThumbTracking(hwnd))
	//	return mysi->nPos;

	//validate and set the scollbar position
	oldpos = mysi->nPos;
	mysi->nPos = max(nPos, mysi->nMin);
	mysi->nPos = min((UINT)mysi->nPos, mysi->nMax - max(mysi->nPage - 1, 0));

	if(fRedraw && !CoolSB_IsThumbTracking(hwnd))
		RedrawNonClient(hwnd, FALSE);

	return oldpos;
}

int WINAPI CoolSB_SetScrollRange (HWND hwnd, int nBar, int nMinPos, int nMaxPos, BOOL fRedraw)
{
	SCROLLINFO *mysi;
	
	if(!(mysi = GetScrollInfoFromHwnd(hwnd, nBar)))
		return SetScrollRange(hwnd, nBar, nMinPos, nMaxPos, fRedraw);

	if(CoolSB_IsThumbTracking(hwnd))
		return mysi->nPos;

	//hide the scrollbar if nMin == nMax
	//nMax-nMin must not be greater than MAXLONG
	mysi->nMin = nMinPos;
	mysi->nMax = nMaxPos;
	
	if(fRedraw)
		RedrawNonClient(hwnd, FALSE);

	return TRUE;
}

//
//	Show or hide the specified scrollbars
//
BOOL WINAPI CoolSB_ShowScrollBar (HWND hwnd, int wBar, BOOL fShow)
{
	SCROLLBAR *sbar;
	BOOL bFailed = FALSE;
	DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

	if(!CoolSB_IsCoolScrollEnabled(hwnd))
		return ShowScrollBar(hwnd, wBar, fShow);

	if((wBar == SB_HORZ || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_HORZ)))
	{
		sbar->fScrollFlags  =  sbar->fScrollFlags & ~CSBS_VISIBLE;
		sbar->fScrollFlags |= (fShow == TRUE ? CSBS_VISIBLE : 0);
		//bFailed = TRUE;

		if(fShow)	SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_HSCROLL);
		else		SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_HSCROLL);
	}

	if((wBar == SB_VERT || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_VERT)))
	{
		sbar->fScrollFlags  =  sbar->fScrollFlags & ~CSBS_VISIBLE;
		sbar->fScrollFlags |= (fShow == TRUE ? CSBS_VISIBLE : 0);
		//bFailed = TRUE;

		if(fShow)	SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_VSCROLL);
		else		SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_VSCROLL);
	}

	if(bFailed)
	{
		return FALSE;
	}
	else
	{
		//DWORD style = GetWindowLong(hwnd, GWL_STYLE);
		//style |= WS_VSCROLL;
		
		//if(s
		//SetWindowLong(hwnd, GWL_STYLE, style);

		SetWindowPos(hwnd, 0, 0, 0, 0, 0, 
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | 
			SWP_NOACTIVATE | SWP_FRAMECHANGED);
		
		return TRUE;
	}
}

//
//	Remove cool scrollbars from the specified window.
//
HRESULT WINAPI UninitializeCoolSB(HWND hwnd)
{
	int i = 0;
	SCROLLWND *sw = GetScrollWndFromHwnd(hwnd);
	if(!sw) return E_FAIL;

	//restore the window procedure with the original one
	SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)sw->oldproc);

	RemoveProp(hwnd, szPropStr);
	//SetWindowLong(hwnd, GWL_USERDATA, 0);

	//finally, release the memory needed for the cool scrollbars
	HeapFree(GetProcessHeap(), 0, sw);

    //Force WM_NCCALCSIZE and WM_NCPAINT so the original scrollbars can kick in
    RedrawNonClient(hwnd, TRUE);

	fThemeRGB = NULL;

	return S_OK;
}

#ifdef INCLUDE_BUTTONS

//
//	Cool scrollbar specific interface (BUTTON support)
//

//
//	Insert a button into the scrollbar area
//
//	wSBflags - SB_HORZ / SB_VERT only
//	uPos     - position into which to insert.
//				can be 0 to insert at the start, or -1 to insert
//				at the end of previously inserted buttons
//

BOOL WINAPI CoolSB_InsertButton(HWND hwnd, int wSBflags, UINT nPos, SCROLLBUT *psb)
{
	SCROLLBAR *sbar;
	SCROLLBUT *sbut;
	UINT i;

	if(!psb) return FALSE;

	if(!(sbar = GetScrollBarFromHwnd(hwnd, wSBflags)))
		return FALSE;
	
	//check that we havn't reached the maximum allowed buttons yet
	if(sbar->nButtons == MAX_COOLSB_BUTS)
		return FALSE;

	//insert at end
	if(nPos == -1)
	{
		sbut = &sbar->sbButtons[sbar->nButtons];
	}
	//otherwise, need to make room
	else if((int)nPos < 0 || (int)nPos > (UINT)sbar->nButtons)
	{
		return FALSE;
	}
	else
	{
		//insert space for the button at the specified position
		for(i = sbar->nButtons; i > nPos; i--)
		{
			sbar->sbButtons[i] = sbar->sbButtons[i-1];
		}

		sbut = &sbar->sbButtons[nPos];
	}

	//only set the button's properties if they are
	//specified by the SCROLLBUT->fMask. 
	//Otherwise, use a default property value

	if(psb->fMask & SBBF_TYPE)
		sbut->uButType   = psb->uButType;
	else
		sbut->uButType	 = SBBT_PUSHBUTTON;

	if(psb->fMask & SBBF_STATE)
		sbut->uState	 = psb->uState;
	else
		sbut->uState	 = 0;

	if(psb->fMask & SBBF_ID)
		sbut->uCmdId     = psb->uCmdId;
	else
		sbut->uCmdId	 = 0;

	if(psb->fMask & SBBF_SIZE)
		sbut->nSize		 = psb->nSize;
	else
		sbut->nSize		 = -1;

	if(psb->fMask & SBBF_PLACEMENT)
		sbut->uPlacement = psb->uPlacement;
	else
		sbut->uPlacement = SBBP_LEFT;

	if(psb->fMask & SBBF_BITMAP)
		sbut->hBmp		 = psb->hBmp;
	else
		sbut->hBmp		 = 0;

	if(psb->fMask & SBBF_ENHMETAFILE)
		sbut->hEmf		 = psb->hEmf;
	else
		sbut->hEmf		 = 0;

	if(psb->fMask & SBBF_CURSOR)
		sbut->hCurs = psb->hCurs;
	else
		sbut->hCurs = 0;

	/*
		We don't use the callback function anymore. The uButType
		member must now specify SBBT_OWNERDRAW, and a WM_NOTIFY will
		be sent when a button must be drawn
	if((psb->fMask & SBBF_OWNERDRAW) && ((psb->uButType & SBBT_MASK) == SBBT_OWNERDRAW))
		pDrawProc	 = psb->pDrawProc;
	else
		pDrawProc	 = 0;*/

	sbar->nButtons++;
	sbut->nSizeReserved = sbut->nSize;

	//MAKE SURE that any resizable buttons are only to the left / above
	//a scrollbar. We don't support resize operations to the right of a scrollbar
	if((sbut->uButType & SBBM_RESIZABLE) &&	sbut->uPlacement == SBBP_RIGHT)
		sbut->uButType &= ~SBBM_RESIZABLE;

	if(psb->fMask & SBBF_BUTMINMAX)
	{
		sbut->nMinSize = psb->nMinSize;
		sbut->nMaxSize = psb->nMaxSize;
	}
	else
	{
		sbut->nMinSize = 0;
		sbut->nMaxSize = -1;
	}

	return TRUE;
}

static SCROLLBUT *GetButtonFromId(SCROLLBAR *sbar, UINT uCmdId)
{
	int i;
	for(i = 0; i < sbar->nButtons; i++)
	{
		if(sbar->sbButtons[i].uCmdId == uCmdId)
			return &sbar->sbButtons[i];
	}

	return 0;
}

//
//	Modify the properties of the specified scrollbar button.
//	wSBflags - SB_HORZ / SB_VERT only
//	uItem    - the command identifier specified when the button was created,
//			   or a non-negative position of the button, depending on if
//			   fByCmd is FALSE or TRUE, respectively
//
BOOL WINAPI CoolSB_ModifyButton (HWND hwnd, int wSBflags, UINT uItem, BOOL fByCmd, SCROLLBUT *psb)
{
	SCROLLBAR *sbar;
	SCROLLBUT *sbut;

	if(!psb) return FALSE;

	//find if this window is CoolScroll enabled
	if(!(sbar = GetScrollBarFromHwnd(hwnd, wSBflags)))
		return FALSE;

	//find the button to modify, depending on if we
	//are modifying by position or command id
	if(fByCmd == FALSE)
	{
		//button from position
		if((int)uItem < 0 || (int)uItem >= (UINT)sbar->nButtons)
			return FALSE;
		else
			sbut = &sbar->sbButtons[uItem];
	}
	else if(fByCmd == TRUE)
	{
		//button from command identifier
		if(!(sbut = GetButtonFromId(sbar, uItem)))
			return FALSE;
	}

	if(psb->fMask & SBBF_TYPE)			sbut->uButType   = psb->uButType;
	if(psb->fMask & SBBF_STATE)			sbut->uState	 = psb->uState;
	if(psb->fMask & SBBF_ID)			sbut->uCmdId     = psb->uCmdId;
	if(psb->fMask & SBBF_SIZE)			sbut->nSize		 = psb->nSize;
	if(psb->fMask & SBBF_PLACEMENT)		sbut->uPlacement = psb->uPlacement;
	if(psb->fMask & SBBF_BITMAP)		sbut->hBmp		 = psb->hBmp;
	if(psb->fMask & SBBF_ENHMETAFILE)	sbut->hEmf		 = psb->hEmf;
	if(psb->fMask & SBBF_CURSOR)		sbut->hCurs		 = psb->hCurs;
	
	if(psb->fMask & SBBF_BUTMINMAX)
	{
		sbut->nMinSize = psb->nMinSize;
		sbut->nMaxSize = psb->nMaxSize;
	}

	return TRUE;
}

BOOL WINAPI CoolSB_RemoveButton(HWND hwnd, int wSBflags, UINT uItem, BOOL fByCmd)
{
	int i;
	SCROLLBAR *sbar;

	//find if this window is CoolScroll enabled
	if(!(sbar = GetScrollBarFromHwnd(hwnd, wSBflags)))
		return FALSE;

	//find the button to modify, depending on if we
	//are modifying by position or command id
	if(fByCmd == FALSE && ((int)uItem < 0 || (int)uItem >= (UINT)sbar->nButtons))
	{
		return FALSE;
	}
	else if(fByCmd == TRUE)
	{
		//find the button with the specified command id
		for(i = 0; i < sbar->nButtons; i++)
		{
			if(sbar->sbButtons[i].uCmdId == uItem)
			{
				//change the id to an index
				uItem = i;
				break;
			}
		}

		//if we failed to find the button...
		if(i == sbar->nButtons) return FALSE;
	}

	//remove the button!
	for(i = uItem; i < sbar->nButtons - 1; i++)
	{
		sbar->sbButtons[i] = sbar->sbButtons[i+1];
	}

	sbar->nButtons--;
	
	RedrawNonClient(hwnd, TRUE);

	return TRUE;
}

//
//	fill in the supplied SCROLLBUT structure
//
BOOL WINAPI CoolSB_GetButton(HWND hwnd, int wSBflags, UINT uItem, BOOL fByCmd, SCROLLBUT *psb)
{
	SCROLLBAR *sbar;
	SCROLLBUT *sbut;

	if(!psb) return FALSE;

	//find if this window is CoolScroll enabled
	if(!(sbar = GetScrollBarFromHwnd(hwnd, wSBflags)))
		return FALSE;

	//find the button to modify, depending on if we
	//are modifying by position or command id
	if(fByCmd == FALSE)
	{
		//button from position
		if((int)uItem < 0 || (int)uItem >= (UINT)sbar->nButtons)
			return FALSE;
		else
			sbut = &sbar->sbButtons[uItem];
	}
	else if(fByCmd == TRUE)
	{
		//button from command identifier
		if(!(sbut = GetButtonFromId(sbar, uItem)))
			return FALSE;
	}

	//copy them across
	*psb = *sbut;

	return FALSE; 
}

#else

BOOL WINAPI CoolSB_InsertButton(HWND hwnd, int wSBflags, UINT nPos,  SCROLLBUT *psb)				{	return FALSE; }
BOOL WINAPI CoolSB_ModifyButton(HWND hwnd, int wSBflags, UINT uItem, BOOL fByCmd, SCROLLBUT *psb)	{	return FALSE; }
BOOL WINAPI CoolSB_RemoveButton(HWND hwnd, int wSBflags, UINT uItem, BOOL fByCmd)					{	return FALSE; }
BOOL WINAPI CoolSB_GetButton   (HWND hwnd, int wSBflags, UINT uItem, BOOL fByCmd, SCROLLBUT *psb)	{	return FALSE; }

#endif //INCLUDE_BUTTONS

//
//	Set the size of the scrollbars
//
BOOL WINAPI CoolSB_SetSize	(HWND hwnd, int wBar, int nLength, int nWidth)
{
	SCROLLBAR *sbar;
	
	if(nLength == 0 || nWidth == 0)
		return FALSE;

	if(nLength < -8 || nWidth < -8)
		return FALSE;

	if(nLength > 256 || nWidth > 256)
		return FALSE;

	if(!GetScrollWndFromHwnd(hwnd))
		return FALSE;

	int bRedraw = 0;
	if((wBar == SB_HORZ || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_HORZ)))
	{
		bRedraw = (sbar->nArrowLength != nLength) || (sbar->nArrowWidth != nWidth);
		sbar->nArrowLength = nLength;
		sbar->nArrowWidth  = nWidth;
	}

	if((wBar == SB_VERT || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_VERT)))
	{
		bRedraw = bRedraw || (sbar->nArrowLength != nLength) || (sbar->nArrowWidth != nWidth);
		sbar->nArrowLength = nLength;
		sbar->nArrowWidth  = nWidth;
	}

	if (bRedraw)
		RedrawNonClient(hwnd, TRUE);

	return TRUE;
}


//
//	Alter the display mode of the scrollbars
//	wBar   - SB_HORZ / SB_VERT / SB_BOTH
//	nStyle - CSBF_NORMAL / CSBF_FLAT / CSBF_HOTTRACKED
//
BOOL WINAPI CoolSB_SetStyle(HWND hwnd, int wBar, UINT nStyle)
{
	SCROLLBAR *sbar;

	if(!GetScrollWndFromHwnd(hwnd))
		return FALSE;

	int bRedraw = 0;
	if((wBar == SB_HORZ || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_HORZ)))
	{
		bRedraw = sbar->fFlatScrollbar != nStyle;
		sbar->fFlatScrollbar = nStyle;
	}

	if((wBar == SB_VERT || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_VERT)))
	{
		bRedraw = bRedraw || (sbar->fFlatScrollbar != nStyle);
		sbar->fFlatScrollbar = nStyle;
	}

	if (bRedraw)
		RedrawNonClient(hwnd, FALSE);

	return TRUE;
}

//
//	Set if the thumb is always visible, even if there is no data to
//  scroll. Setting this keeps the scrollbar enabled, but the thumb
//  covers the whole area
//	
BOOL WINAPI CoolSB_SetThumbAlways(HWND hwnd, int wBar, BOOL fThumbAlways)
{
	SCROLLBAR *sbar;

	if(!GetScrollWndFromHwnd(hwnd))
		return FALSE;

	if((wBar == SB_HORZ || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_HORZ)))
	{
		if(fThumbAlways)
			sbar->fScrollFlags |=  CSBS_THUMBALWAYS;
		else
			sbar->fScrollFlags &= ~CSBS_THUMBALWAYS;
	}

	if((wBar == SB_VERT || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_VERT)))
	{
		if(fThumbAlways)
			sbar->fScrollFlags |=  CSBS_THUMBALWAYS;
		else
			sbar->fScrollFlags &= ~CSBS_THUMBALWAYS;
	}

	RedrawNonClient(hwnd, FALSE);

	return TRUE;
}

//
//	Set the minimum size, in pixels, that the thumb box will shrink to.
//
BOOL WINAPI CoolSB_SetMinThumbSize(HWND hwnd, UINT wBar, UINT size)
{
	SCROLLBAR *sbar;

	if(!GetScrollWndFromHwnd(hwnd))
		return FALSE;

	if(size == -1)
		size = CoolSB_GetDefaultMinThumbSize();

	if((wBar == SB_HORZ || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_HORZ)))
	{
		sbar->nMinThumbSize = size;
	}

	if((wBar == SB_VERT || wBar == SB_BOTH) && 
	   (sbar = GetScrollBarFromHwnd(hwnd, SB_VERT)))
	{
		sbar->nMinThumbSize = size;
	}

	return TRUE;
}
