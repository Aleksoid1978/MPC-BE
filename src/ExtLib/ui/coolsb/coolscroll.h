#ifndef _COOLSBLIB_INCLUDED
#define _COOLSBLIB_INCLUDED

#ifdef __cplusplus
extern "C"{
#endif

#include <windows.h>

// To complement the exisiting SB_HORZ, SB_VERT, SB_BOTH
// scrollbar identifiers
#define COOLSB_NONE (-1)
#define SB_INSBUT	(-2)

//
//	Arrow size defines
//
#define SYSTEM_METRIC (-1)


//
// general scrollbar styles
//
// use the standard ESB_DISABLE_xxx flags to represent the
// enabled / disabled states. (defined in winuser.h)
//
#define CSBS_THUMBALWAYS		4
#define CSBS_VISIBLE			8

//cool scrollbar styles for Flat scrollbars
#define CSBS_NORMAL			0
#define CSBS_FLAT			1
#define CSBS_HOTTRACKED		2

//
//	Button mask flags for indicating which members of SCROLLBUT
//	to use during a button insertion / modification
//	
#define SBBF_TYPE			0x0001
#define SBBF_ID				0x0002
#define SBBF_PLACEMENT		0x0004
#define SBBF_SIZE			0x0008
#define SBBF_BITMAP			0x0010
#define SBBF_ENHMETAFILE	0x0020
//#define SBBF_OWNERDRAW		0x0040	//unused at present
#define SBBF_CURSOR			0x0080
#define SBBF_BUTMINMAX		0x0100
#define SBBF_STATE			0x0200

//button styles (states)
#define SBBS_NORMAL			0
#define SBBS_PUSHED			1
#define SBBS_CHECKED		SBBS_PUSHED

//
// scrollbar button types
//
#define SBBT_PUSHBUTTON		1	//standard push button
#define SBBT_TOGGLEBUTTON	2	//toggle button
#define SBBT_FIXED			3	//fixed button (non-clickable)
#define SBBT_FLAT			4	//blank area (flat, with border)
#define SBBT_BLANK			5	//blank area (flat, no border)
#define SBBT_DARK			6	//dark blank area (flat)
#define SBBT_OWNERDRAW		7	//user draws the button via a WM_NOTIFY

#define SBBT_MASK			0x1f	//mask off low 5 bits

//button type modifiers
#define SBBM_RECESSED		0x0020	//recessed when clicked (like Word 97)
#define SBBM_LEFTARROW		0x0040
#define SBBM_RIGHTARROW		0x0080
#define SBBM_UPARROW		0x0100
#define SBBM_DOWNARROW		0x0200
#define SBBM_RESIZABLE		0x0400
#define SBBM_TYPE2			0x0800
#define SBBM_TYPE3			0x1000
#define SBBM_TOOLTIPS		0x2000	//currently unused (define COOLSB_TOOLTIPS in userdefs.h)

//button placement flags
#define SBBP_LEFT	1
#define SBBP_RIGHT  2
#define SBBP_TOP	1	//3
#define SBBP_BOTTOM 2	//4


//
//	Button command notification codes
//	for sending with a WM_COMMAND message
//
#define CSBN_BASE	0
#define CSBN_CLICKED (1 + CSBN_BASE)
#define CSBN_HILIGHT (2 + CSBN_BASE)

//
//	Minimum size in pixels of a scrollbar thumb
//
#define MINTHUMBSIZE_NT4   8
#define MINTHUMBSIZE_2000  6

//define some more hittest values for our cool-scrollbar
#define HTSCROLL_LEFT		(SB_LINELEFT)
#define HTSCROLL_RIGHT		(SB_LINERIGHT)
#define HTSCROLL_UP			(SB_LINEUP)
#define HTSCROLL_DOWN		(SB_LINEDOWN)
#define HTSCROLL_THUMB		(SB_THUMBTRACK)
#define HTSCROLL_PAGEGUP	(SB_PAGEUP)
#define HTSCROLL_PAGEGDOWN	(SB_PAGEDOWN)
#define HTSCROLL_PAGELEFT	(SB_PAGELEFT)
#define HTSCROLL_PAGERIGHT	(SB_PAGERIGHT)

#define HTSCROLL_NONE		(-1)
#define HTSCROLL_NORMAL		(-1)

#define HTSCROLL_INSERTED	(128)
#define HTSCROLL_PRE		(32 | HTSCROLL_INSERTED)
#define HTSCROLL_POST		(64 | HTSCROLL_INSERTED)

/*

	Public interface to the Cool Scrollbar library


*/

typedef COLORREF (*ptr_themeRGB)(const int, const int, const int);

BOOL	WINAPI InitializeCoolSB(HWND hwnd, ptr_themeRGB ThemeRGB);
HRESULT WINAPI UninitializeCoolSB	(HWND hwnd);

BOOL WINAPI CoolSB_SetMinThumbSize(HWND hwnd, UINT wBar, UINT size);
BOOL WINAPI CoolSB_IsThumbTracking(HWND hwnd);
BOOL WINAPI CoolSB_IsCoolScrollEnabled(HWND hwnd);

//
BOOL WINAPI CoolSB_EnableScrollBar	(HWND hwnd, int wSBflags, UINT wArrows);
BOOL WINAPI CoolSB_GetScrollInfo	(HWND hwnd, int fnBar, LPSCROLLINFO lpsi);
int	 WINAPI CoolSB_GetScrollPos	(HWND hwnd, int nBar);
BOOL WINAPI CoolSB_GetScrollRange	(HWND hwnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos);

//
int	 WINAPI CoolSB_SetScrollInfo	(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fRedraw);
int  WINAPI CoolSB_SetScrollPos	(HWND hwnd, int nBar, int nPos, BOOL fRedraw);
int  WINAPI CoolSB_SetScrollRange	(HWND hwnd, int nBar, int nMinPos, int nMaxPos, BOOL fRedraw);
BOOL WINAPI CoolSB_ShowScrollBar	(HWND hwnd, int wBar, BOOL fShow);

ptr_themeRGB fThemeRGB;

//
// Scrollbar dimension functions
//
BOOL WINAPI CoolSB_SetSize			(HWND hwnd, int wBar, int nLength, int nWidth);

//
// Set the visual nature of a scrollbar (flat, normal etc)
//
BOOL WINAPI CoolSB_SetStyle		(HWND hwnd, int wBar, UINT nStyle);
BOOL WINAPI CoolSB_SetThumbAlways	(HWND hwnd, int wBar, BOOL fThumbAlways);

//
//	Scrollbar button structure, for inserted buttons only
//
typedef struct 
{
	UINT			fMask;			//which members are in use
	UINT			uPlacement;		//is this button to the left/right (above/below) of the scrollbar??
	UINT			uCmdId;			//command identifier (WM_COMMAND value to send)
	UINT			uButType;		//
	UINT			uState;			//toggled etc
	int				nSize;			//size in pixels. -1 for autosize
	
	HBITMAP			hBmp;			//handle to a bitmap to use as the button face
	HENHMETAFILE	hEmf;			//handle to an enhanced metafile
	
	HCURSOR			hCurs;			//handle to a user-supplied mouse cursor to apply
									//to this button

	int				nSizeReserved;	//internal variable used for resizing
	int				nMinSize;		//min size
	int				nMaxSize;		//max size

} SCROLLBUT;

BOOL WINAPI CoolSB_InsertButton(HWND hwnd, int wSBflags, UINT nPos,  SCROLLBUT *psb);
BOOL WINAPI CoolSB_ModifyButton(HWND hwnd, int wSBflags, UINT uItem, BOOL fByCmd, SCROLLBUT *psb);
BOOL WINAPI CoolSB_RemoveButton(HWND hwnd, int wSBflags, UINT uItem, BOOL fByCmd);
BOOL WINAPI CoolSB_GetButton   (HWND hwnd, int wSBflags, UINT uItem, BOOL fByCmd, SCROLLBUT *psb);

void WINAPI CoolSB_SetESBProc(void *proc);

typedef struct
{
	NMHDR	hdr;
	DWORD   dwDrawStage;
	HDC		hdc;
	RECT	rect;
	UINT	uItem;
	UINT	uState;
	UINT	nBar;
	
} NMCSBCUSTOMDRAW;

typedef struct
{
	NMHDR	hdr;
	RECT	rect;
	POINT	pt;
	UINT	uCmdId;
	UINT	uState;
	int		nBar;
} NMCOOLBUTMSG;

/*
typedef struct
{
	NMHDR	hdr;
	DWORD   dwDrawStage;
	HDC		hdc;
	RECT	rect;
	UINT	uCmdId;
	UINT	uState;

} NMCOOLBUTTON_CUSTOMDRAW;
*/


//
//	Define the WM_NOTIFY code value for cool-scrollbar custom drawing
//
#define NM_COOLSB_CUSTOMDRAW (0-0xfffU)

#ifdef __cplusplus
}
#endif

#endif
