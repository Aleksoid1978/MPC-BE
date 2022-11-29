/*
 * (C) 2022 see Authors.txt
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

#include "SubPic/ISubPic.h"

#define TARGET_FRAME 0
#define TARGET_SCREEN 1

//
// IAllocatorPresenter (MPC-BE internal interface)
//

interface __declspec(uuid("AD863F43-83F9-4B8E-962C-426F2BDBEAEF"))
IAllocatorPresenter :
public IUnknown {
	STDMETHOD (CreateRenderer) (IUnknown** ppRenderer) PURE;

	STDMETHOD_(CLSID, GetAPCLSID) () PURE;

	STDMETHOD_(SIZE, GetVideoSize) () PURE;
	STDMETHOD_(SIZE, GetVideoSizeAR) () PURE;
	STDMETHOD_(void, SetPosition) (RECT w, RECT v) PURE;
	STDMETHOD (SetRotation) (int rotation) PURE;
	STDMETHOD_(int, GetRotation) () PURE;
	STDMETHOD (SetFlip) (bool flip) PURE;
	STDMETHOD_(bool, GetFlip) () PURE;
	STDMETHOD_(bool, Paint) (bool fAll) PURE;

	STDMETHOD_(void, SetTime) (REFERENCE_TIME rtNow) PURE;
	STDMETHOD_(void, SetSubtitleDelay) (int delay_ms) PURE;
	STDMETHOD_(int, GetSubtitleDelay) () PURE;
	STDMETHOD_(double, GetFPS) () PURE;

	STDMETHOD_(void, SetSubPicProvider) (ISubPicProvider* pSubPicProvider) PURE;
	STDMETHOD_(void, Invalidate) (REFERENCE_TIME rtInvalidate = -1) PURE;

	STDMETHOD (GetDIB) (BYTE* lpDib, DWORD* size) PURE; // may be deleted in the future
	STDMETHOD (GetVideoFrame) (BYTE* lpDib, DWORD* size) PURE;
	STDMETHOD (GetDisplayedImage) (LPVOID* dibImage) PURE;

	STDMETHOD_(int, GetPixelShaderMode) () PURE;
	STDMETHOD (ClearPixelShaders) (int target) PURE;
	STDMETHOD (AddPixelShader) (int target, LPCWSTR name, LPCSTR profile, LPCSTR sourceCode) PURE;

	STDMETHOD_(bool, ResizeDevice) () PURE;
	STDMETHOD_(bool, ResetDevice) () PURE;
	STDMETHOD_(bool, DisplayChange) () PURE;
	STDMETHOD_(void, ResetStats) () PURE;

	STDMETHOD_(bool, IsRendering)() PURE;
};
