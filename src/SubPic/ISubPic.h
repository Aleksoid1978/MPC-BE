/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include <atlcoll.h>
#include "CoordGeom.h"

enum SUBTITLE_TYPE {
	ST_TEXT,
	ST_VOBSUB,
	ST_DVB,
	ST_HDMV,
	ST_XSUB,
	ST_XYSUBPIC
};

// flag for display only forced subtitles (PGS/VOBSUB)
extern bool g_bForcedSubtitle;

#pragma pack(push, 1)
struct SubPicDesc {
	int type;
	int w, h, bpp, pitch, pitchUV;
	void* bits;
	BYTE* bitsU;
	BYTE* bitsV;
	RECT vidrect; // video rectangle

	struct SubPicDesc() {
		type = 0;
		w = h = bpp = pitch = pitchUV = 0;
		bits = NULL;
		bitsU = bitsV = NULL;
		vidrect = CRect(0, 0, 0, 0);
	}
};
#pragma pack(pop)

//
// ISubPic
//

interface __declspec(uuid("449E11F3-52D1-4a27-AA61-E2733AC92CC0"))
ISubPic :
public IUnknown {
	static const REFERENCE_TIME INVALID_SUBPIC_TIME = -1;

	STDMETHOD_(void*, GetObject) () PURE;

	STDMETHOD_(REFERENCE_TIME, GetStart) () PURE;
	STDMETHOD_(REFERENCE_TIME, GetStop) () PURE;
	STDMETHOD_(void, SetStart) (REFERENCE_TIME rtStart) PURE;
	STDMETHOD_(void, SetStop) (REFERENCE_TIME rtStop) PURE;

	STDMETHOD (GetDesc) (SubPicDesc& spd /*[out]*/) PURE;
	STDMETHOD (CopyTo) (ISubPic* pSubPic /*[in]*/) PURE;

	STDMETHOD (ClearDirtyRect) (DWORD color /*[in]*/) PURE;
	STDMETHOD (GetDirtyRect) (RECT* pDirtyRect /*[out]*/) PURE;
	STDMETHOD (SetDirtyRect) (RECT* pDirtyRect /*[in]*/) PURE;

	STDMETHOD (GetMaxSize) (SIZE* pMaxSize /*[out]*/) PURE;
	STDMETHOD (SetSize) (SIZE pSize /*[in]*/, RECT vidrect /*[in]*/) PURE;
	STDMETHOD (GetSize) (SIZE* pSize /*[out]*/) PURE;

	STDMETHOD (Lock) (SubPicDesc& spd /*[out]*/) PURE;
	STDMETHOD (Unlock) (RECT* pDirtyRect /*[in]*/) PURE;

	STDMETHOD (AlphaBlt) (RECT* pSrc, RECT* pDst, SubPicDesc* pTarget = NULL /*[in]*/) PURE;
	STDMETHOD (GetSourceAndDest) (RECT rcWindow /*[in]*/, RECT rcVideo /*[in]*/, BOOL bPositionRelative /*[in]*/, CPoint ShiftPos /*[in]*/, RECT* pRcSource /*[out]*/, RECT* pRcDest /*[out]*/, int xOffsetInPixels /*[in]*/, const BOOL bUseSpecialCase/*[in]*/) const PURE;
	STDMETHOD (SetVirtualTextureSize) (const SIZE pSize, const POINT pTopLeft) PURE;

	STDMETHOD_(REFERENCE_TIME, GetSegmentStart) () PURE;
	STDMETHOD_(REFERENCE_TIME, GetSegmentStop) () PURE;
	STDMETHOD_(void, SetSegmentStart) (REFERENCE_TIME rtStart) PURE;
	STDMETHOD_(void, SetSegmentStop) (REFERENCE_TIME rtStop) PURE;

	STDMETHOD (SetType) (SUBTITLE_TYPE subtitleType /*[in]*/) PURE;
	STDMETHOD (GetType) (SUBTITLE_TYPE* pSubtitleType /*[out]*/) PURE;

	STDMETHOD_(bool, GetInverseAlpha)() const PURE;
	STDMETHOD_(void, SetInverseAlpha)(bool bInverted) PURE;
};

//
// ISubPicAllocator
//

interface __declspec(uuid("CF7C3C23-6392-4a42-9E72-0736CFF793CB"))
ISubPicAllocator :
public IUnknown {
	STDMETHOD (SetCurSize) (SIZE size /*[in]*/) PURE;
	STDMETHOD (SetCurVidRect) (RECT curvidrect) PURE;

	STDMETHOD (GetStatic) (ISubPic** ppSubPic /*[out]*/) PURE;
	STDMETHOD (AllocDynamic) (ISubPic** ppSubPic /*[out]*/) PURE;

	STDMETHOD_(bool, IsDynamicWriteOnly) () PURE;

	STDMETHOD (ChangeDevice) (IUnknown* pDev) PURE;
	STDMETHOD (SetMaxTextureSize) (SIZE MaxTextureSize) PURE;

	STDMETHOD (Reset) () PURE;
};

//
// ISubPicProvider
//

interface __declspec(uuid("D62B9A1A-879A-42db-AB04-88AA8F243CFD"))
ISubPicProvider :
public IUnknown {
	static const REFERENCE_TIME UNKNOWN_TIME = _I64_MAX;

	STDMETHOD (Lock) () PURE;
	STDMETHOD (Unlock) () PURE;

	STDMETHOD_(POSITION, GetStartPosition) (REFERENCE_TIME rt, double fps, bool CleanOld = false) PURE;
	STDMETHOD_(POSITION, GetNext) (POSITION pos) PURE;

	STDMETHOD_(REFERENCE_TIME, GetStart) (POSITION pos, double fps) PURE;
	STDMETHOD_(REFERENCE_TIME, GetStop) (POSITION pos, double fps) PURE;

	STDMETHOD_(bool, IsAnimated) (POSITION pos) PURE;

	STDMETHOD (Render) (SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox) PURE;
	STDMETHOD (GetTextureSize) (POSITION pos, SIZE& MaxTextureSize, SIZE& VirtualSize, POINT& VirtualTopLeft) PURE;

	STDMETHOD_(SUBTITLE_TYPE, GetType) () PURE;
};

//
// ISubPicQueue
//

interface __declspec(uuid("C8334466-CD1E-4ad1-9D2D-8EE8519BD180"))
ISubPicQueue :
public IUnknown {
	STDMETHOD (SetSubPicProvider) (ISubPicProvider* pSubPicProvider /*[in]*/) PURE;
	STDMETHOD (GetSubPicProvider) (ISubPicProvider** pSubPicProvider /*[out]*/) PURE;

	STDMETHOD (SetFPS) (double fps /*[in]*/) PURE;
	STDMETHOD (SetTime) (REFERENCE_TIME rtNow /*[in]*/) PURE;

	STDMETHOD (Invalidate) (REFERENCE_TIME rtInvalidate = -1) PURE;
	STDMETHOD_(bool, LookupSubPic) (REFERENCE_TIME rtNow /*[in]*/, CComPtr<ISubPic> &pSubPic /*[out]*/) PURE;

	STDMETHOD (GetStats) (int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop /*[out]*/) PURE;
	STDMETHOD (GetStats) (int nSubPic /*[in]*/, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop /*[out]*/) PURE;

	STDMETHOD_(bool, LookupSubPic)(REFERENCE_TIME rtNow /*[in]*/, bool bAdviseBlocking, CComPtr<ISubPic>& pSubPic /*[out]*/) PURE;
};

//
// ISubPicAllocatorPresenter3 (under development)
//
#define TARGET_FRAME 0
#define TARGET_SCREEN 1

interface __declspec(uuid("AD863F43-83F9-4B8E-962C-426F2BDBEAEF"))
ISubPicAllocatorPresenter3 :
public IUnknown {
	STDMETHOD (CreateRenderer) (IUnknown** ppRenderer) PURE;

	STDMETHOD_(SIZE, GetVideoSize) () PURE;
	STDMETHOD_(SIZE, GetVideoSizeAR) () PURE;
	STDMETHOD_(void, SetPosition) (RECT w, RECT v) PURE;
	STDMETHOD_(bool, Paint) (bool fAll) PURE;

	STDMETHOD_(void, SetTime) (REFERENCE_TIME rtNow) PURE;
	STDMETHOD_(void, SetSubtitleDelay) (int delay_ms) PURE;
	STDMETHOD_(int, GetSubtitleDelay) () PURE;
	STDMETHOD_(double, GetFPS) () PURE;

	STDMETHOD_(void, SetSubPicProvider) (ISubPicProvider* pSubPicProvider) PURE;
	STDMETHOD_(void, Invalidate) (REFERENCE_TIME rtInvalidate = -1) PURE;

	STDMETHOD (GetDIB) (BYTE* lpDib, DWORD* size) PURE;

	STDMETHOD (SetVideoAngle) (Vector v) PURE;
	STDMETHOD (ClearPixelShaders) (int target) PURE;
	STDMETHOD (AddPixelShader) (int target, LPCSTR sourceCode, LPCSTR profile) PURE;

	STDMETHOD_(bool, ResetDevice) () PURE;
	STDMETHOD_(bool, DisplayChange) () PURE;

	STDMETHOD_(bool, IsRendering)() PURE;
};

//
// ISubStream
//

interface __declspec(uuid("DE11E2FB-02D3-45e4-A174-6B7CE2783BDB"))
ISubStream :
public IPersist {
	STDMETHOD_(int, GetStreamCount) () PURE;
	STDMETHOD (GetStreamInfo) (int i, WCHAR** ppName, LCID* pLCID) PURE;
	STDMETHOD_(int, GetStream) () PURE;
	STDMETHOD (SetStream) (int iStream) PURE;
	STDMETHOD (Reload) () PURE;
	STDMETHOD (SetSourceTargetInfo)(CString yuvMatrix, CString inputRange, CString outpuRange) { return E_NOTIMPL; };
	// TODO: get rid of IPersist to identify type and use only
	// interface functions to modify the settings of the substream
};
