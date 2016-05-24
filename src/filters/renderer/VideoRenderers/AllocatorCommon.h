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

#include <d3d9.h>
#include <vmr9.h>
#include "../../../SubPic/ISubPic.h"
#include "PixelShaderCompiler.h"

extern CCritSec g_ffdshowReceive;
extern bool queue_ffdshow_support;

extern bool IsVMR9InGraph(IFilterGraph* pFG);
extern CString GetWindowsErrorMessage(HRESULT _Error, HMODULE _Module);
extern const wchar_t *GetD3DFormatStr(D3DFORMAT Format);

extern HRESULT CreateAP9(const CLSID& clsid, HWND hWnd, bool bFullscreen, ISubPicAllocatorPresenter3** ppAP);
extern HRESULT CreateEVR(const CLSID& clsid, HWND hWnd, bool bFullscreen, ISubPicAllocatorPresenter3** ppAP);

// Support ffdshow queuing.
// This interface is used to check version of MPC-BE.
// {A273C7F6-25D4-46b0-B2C8-4F7FADC44E37}
DEFINE_GUID(IID_IVMRffdshow9,
			0xa273c7f6, 0x25d4, 0x46b0, 0xb2, 0xc8, 0x4f, 0x7f, 0xad, 0xc4, 0x4e, 0x37);

MIDL_INTERFACE("A273C7F6-25D4-46b0-B2C8-4F7FADC44E37")
IVMRffdshow9 :
public IUnknown {
public:
	virtual STDMETHODIMP support_ffdshow(void) PURE;
};

// Set and query D3DFullscreen mode.
interface __declspec(uuid("8EA1E899-B77D-4777-9F0E-66421BEA50F8"))
ID3DFullscreenControl :
public IUnknown {
	STDMETHOD(SetD3DFullscreen)(bool fEnabled) PURE;
	STDMETHOD(GetD3DFullscreen)(bool * pfEnabled) PURE;
};
