/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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

#include <d3d9.h>
#include "../../../SubPic/ISubPic.h"

HRESULT CreateAP9(const CLSID& clsid, HWND hWnd, bool bFullscreen, ISubPicAllocatorPresenter3** ppAP);
HRESULT CreateEVR(const CLSID& clsid, HWND hWnd, bool bFullscreen, ISubPicAllocatorPresenter3** ppAP);

CString GetWindowsErrorMessage(HRESULT _Error, HMODULE _Module);
const wchar_t* D3DFormatToString(D3DFORMAT format);
const wchar_t* GetD3DFormatStr(D3DFORMAT Format);

// Set and query D3DFullscreen mode.
interface __declspec(uuid("8EA1E899-B77D-4777-9F0E-66421BEA50F8"))
ID3DFullscreenControl :
public IUnknown {
	STDMETHOD(SetD3DFullscreen)(bool fEnabled) PURE;
	STDMETHOD(GetD3DFullscreen)(bool * pfEnabled) PURE;
};

// Notify playback events
interface __declspec(uuid("7DB66F45-A3CB-4B06-8219-916C33E53E2D"))
IPlaybackNotify :
public IUnknown {
	STDMETHOD(Stop)() PURE;
};

