/*
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

#include "stdafx.h"
#include <InitGuid.h>
#include "AllocatorCommon.h"
#include "moreuuids.h"

#include "DXRAllocatorPresenter.h"
#include "madVRAllocatorPresenter.h"
#include "EVRAllocatorPresenter.h"

static bool bIsErrorShowing		= false;
static bool bIsWarningShowing	= false;

bool IsVMR9InGraph(IFilterGraph* pFG)
{
	BeginEnumFilters(pFG, pEF, pBF)
	if (CComQIPtr<IVMRWindowlessControl9>(pBF)) {
		return true;
	}
	EndEnumFilters
	return false;
}

//

HRESULT CreateAP9(const CLSID& clsid, HWND hWnd, bool bFullscreen, ISubPicAllocatorPresenter3** ppAP)
{
	CheckPointer(ppAP, E_POINTER);

	*ppAP = NULL;

	using namespace DSObjects;

	HRESULT hr = E_FAIL;
	CString Error;

	if (IsEqualCLSID(clsid, CLSID_DXRAllocatorPresenter)) {
		*ppAP = DNew CDXRAllocatorPresenter(hWnd, hr, Error);
	}
	else if (IsEqualCLSID(clsid, CLSID_madVRAllocatorPresenter)) {
		*ppAP = DNew CmadVRAllocatorPresenter(hWnd, hr, Error);
	}
	else {
		return E_FAIL;
	}

	if (*ppAP == NULL) {
		return E_OUTOFMEMORY;
	}

	(*ppAP)->AddRef();

	if (FAILED(hr)) {
		if (!bIsErrorShowing) {
			Error += L"\n";
			Error += GetWindowsErrorMessage(hr, NULL);

			MessageBox(hWnd, Error, L"Error creating DX9 allocation presenter", MB_OK | MB_ICONERROR);
		}
		bIsErrorShowing = true;
		(*ppAP)->Release();
		*ppAP = NULL;
	}
	else if (!Error.IsEmpty()) {
		if (!bIsWarningShowing) {
			MessageBox(hWnd, Error, L"Warning creating DX9 allocation presenter", MB_OK | MB_ICONWARNING);
		}
		bIsWarningShowing = true;
	} else {
		bIsErrorShowing = bIsWarningShowing = false;
	}

	return hr;
}

HRESULT CreateEVR(const CLSID& clsid, HWND hWnd, bool bFullscreen, ISubPicAllocatorPresenter3** ppAP)
{
	HRESULT hr = E_FAIL;
	if (clsid == CLSID_EVRAllocatorPresenter) {
		CString Error;
		*ppAP	= DNew DSObjects::CEVRAllocatorPresenter(hWnd, bFullscreen, hr, Error);
		(*ppAP)->AddRef();

		if (FAILED(hr)) {
			if (!bIsErrorShowing) {
				Error += L"\n";
				Error += GetWindowsErrorMessage(hr, NULL);
				MessageBox(hWnd, Error, L"Error creating EVR Custom renderer", MB_OK | MB_ICONERROR);
			}
			bIsErrorShowing = true;
			(*ppAP)->Release();
			*ppAP = NULL;
		} else if (!Error.IsEmpty()) {
			if (!bIsWarningShowing) {
				MessageBox(hWnd, Error, L"Warning creating EVR Custom renderer", MB_OK | MB_ICONWARNING);
			}
			bIsWarningShowing = true;
		} else {
			bIsErrorShowing = bIsWarningShowing = false;
		}
	}

	return hr;
}

CString GetWindowsErrorMessage(HRESULT _Error, HMODULE _Module)
{
#define UNPACK_VALUE(VALUE) case VALUE: return L#VALUE;
	switch (_Error) {
		UNPACK_VALUE(D3DERR_WRONGTEXTUREFORMAT);
		UNPACK_VALUE(D3DERR_UNSUPPORTEDCOLOROPERATION);
		UNPACK_VALUE(D3DERR_UNSUPPORTEDCOLORARG);
		UNPACK_VALUE(D3DERR_UNSUPPORTEDALPHAOPERATION);
		UNPACK_VALUE(D3DERR_UNSUPPORTEDALPHAARG);
		UNPACK_VALUE(D3DERR_TOOMANYOPERATIONS);
		UNPACK_VALUE(D3DERR_CONFLICTINGTEXTUREFILTER);
		UNPACK_VALUE(D3DERR_UNSUPPORTEDFACTORVALUE);
		UNPACK_VALUE(D3DERR_CONFLICTINGRENDERSTATE);
		UNPACK_VALUE(D3DERR_UNSUPPORTEDTEXTUREFILTER);
		UNPACK_VALUE(D3DERR_CONFLICTINGTEXTUREPALETTE);
		UNPACK_VALUE(D3DERR_DRIVERINTERNALERROR);
		UNPACK_VALUE(D3DERR_NOTFOUND);
		UNPACK_VALUE(D3DERR_MOREDATA);
		UNPACK_VALUE(D3DERR_DEVICELOST);
		UNPACK_VALUE(D3DERR_DEVICENOTRESET);
		UNPACK_VALUE(D3DERR_NOTAVAILABLE);
		UNPACK_VALUE(D3DERR_OUTOFVIDEOMEMORY);
		UNPACK_VALUE(D3DERR_INVALIDDEVICE);
		UNPACK_VALUE(D3DERR_INVALIDCALL);
		UNPACK_VALUE(D3DERR_DRIVERINVALIDCALL);
		UNPACK_VALUE(D3DERR_WASSTILLDRAWING);
		UNPACK_VALUE(D3DOK_NOAUTOGEN);
		UNPACK_VALUE(D3DERR_DEVICEREMOVED);
		UNPACK_VALUE(S_NOT_RESIDENT);
		UNPACK_VALUE(S_RESIDENT_IN_SHARED_MEMORY);
		UNPACK_VALUE(S_PRESENT_MODE_CHANGED);
		UNPACK_VALUE(S_PRESENT_OCCLUDED);
		UNPACK_VALUE(D3DERR_DEVICEHUNG);
		UNPACK_VALUE(E_UNEXPECTED);
	}
#undef UNPACK_VALUE

	CString errmsg;
	LPVOID lpMsgBuf;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_HMODULE,
					  _Module, _Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL)) {
		errmsg = (LPCTSTR)lpMsgBuf;
		LocalFree(lpMsgBuf);
	}
	CString Temp;
	Temp.Format(L"0x%08x ", _Error);
	return Temp + errmsg;
}

const wchar_t* D3DFormatToString(D3DFORMAT format)
{
#define UNPACK_VALUE(VALUE) case VALUE: return L#VALUE;
	switch (format) {
		UNPACK_VALUE(D3DFMT_R8G8B8);
		UNPACK_VALUE(D3DFMT_A8R8G8B8);
		UNPACK_VALUE(D3DFMT_X8R8G8B8);
		UNPACK_VALUE(D3DFMT_R5G6B5);
		UNPACK_VALUE(D3DFMT_X1R5G5B5);
		UNPACK_VALUE(D3DFMT_A1R5G5B5);
		UNPACK_VALUE(D3DFMT_A4R4G4B4);
		UNPACK_VALUE(D3DFMT_R3G3B2);
		UNPACK_VALUE(D3DFMT_A8);
		UNPACK_VALUE(D3DFMT_A8R3G3B2);
		UNPACK_VALUE(D3DFMT_X4R4G4B4);
		UNPACK_VALUE(D3DFMT_A2B10G10R10);
		UNPACK_VALUE(D3DFMT_A8B8G8R8);
		UNPACK_VALUE(D3DFMT_X8B8G8R8);
		UNPACK_VALUE(D3DFMT_G16R16);
		UNPACK_VALUE(D3DFMT_A2R10G10B10);
		UNPACK_VALUE(D3DFMT_A16B16G16R16);
		UNPACK_VALUE(D3DFMT_A8P8);
		UNPACK_VALUE(D3DFMT_P8);
		UNPACK_VALUE(D3DFMT_L8);
		UNPACK_VALUE(D3DFMT_A8L8);
		UNPACK_VALUE(D3DFMT_A4L4);
		UNPACK_VALUE(D3DFMT_V8U8);
		UNPACK_VALUE(D3DFMT_L6V5U5);
		UNPACK_VALUE(D3DFMT_X8L8V8U8);
		UNPACK_VALUE(D3DFMT_Q8W8V8U8);
		UNPACK_VALUE(D3DFMT_V16U16);
		UNPACK_VALUE(D3DFMT_A2W10V10U10);
		UNPACK_VALUE(D3DFMT_UYVY);
		UNPACK_VALUE(D3DFMT_R8G8_B8G8);
		UNPACK_VALUE(D3DFMT_YUY2);
		UNPACK_VALUE(D3DFMT_G8R8_G8B8);
		UNPACK_VALUE(D3DFMT_DXT1);
		UNPACK_VALUE(D3DFMT_DXT2);
		UNPACK_VALUE(D3DFMT_DXT3);
		UNPACK_VALUE(D3DFMT_DXT4);
		UNPACK_VALUE(D3DFMT_DXT5);
		UNPACK_VALUE(D3DFMT_D16_LOCKABLE);
		UNPACK_VALUE(D3DFMT_D32);
		UNPACK_VALUE(D3DFMT_D15S1);
		UNPACK_VALUE(D3DFMT_D24S8);
		UNPACK_VALUE(D3DFMT_D24X8);
		UNPACK_VALUE(D3DFMT_D24X4S4);
		UNPACK_VALUE(D3DFMT_D16);
		UNPACK_VALUE(D3DFMT_D32F_LOCKABLE);
		UNPACK_VALUE(D3DFMT_D24FS8);
		UNPACK_VALUE(D3DFMT_D32_LOCKABLE);
		UNPACK_VALUE(D3DFMT_S8_LOCKABLE);
		UNPACK_VALUE(D3DFMT_L16);
		UNPACK_VALUE(D3DFMT_VERTEXDATA);
		UNPACK_VALUE(D3DFMT_INDEX16);
		UNPACK_VALUE(D3DFMT_INDEX32);
		UNPACK_VALUE(D3DFMT_Q16W16V16U16);
		UNPACK_VALUE(D3DFMT_MULTI2_ARGB8);
		UNPACK_VALUE(D3DFMT_R16F);
		UNPACK_VALUE(D3DFMT_G16R16F);
		UNPACK_VALUE(D3DFMT_A16B16G16R16F);
		UNPACK_VALUE(D3DFMT_R32F);
		UNPACK_VALUE(D3DFMT_G32R32F);
		UNPACK_VALUE(D3DFMT_A32B32G32R32F);
		UNPACK_VALUE(D3DFMT_CxV8U8);
		UNPACK_VALUE(D3DFMT_A1);
		UNPACK_VALUE(D3DFMT_A2B10G10R10_XR_BIAS);
		UNPACK_VALUE(D3DFMT_BINARYBUFFER);
	case FCC('NV12'): return L"D3DFMT_NV12";
	case FCC('P010'): return L"D3DFMT_P010";
	};
#undef UNPACK_VALUE

	return L"D3DFMT_UNKNOWN";
}

const wchar_t* GetD3DFormatStr(D3DFORMAT Format)
{
	return D3DFormatToString(Format) + 7;
}
