/*
 * (C) 2016-2024 see Authors.txt
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

#include "pch.h"
#include "SubtitleHelperAllocatorPresenter.h"
#include "FilterInterfaces.h"
#include "filters/renderer/VideoRenderers/IPinHook.h"
#include "DSUtil/DSUtil.h"

CSubtitleHelperAllocatorPresenter::CSubtitleHelperAllocatorPresenter(HWND hWnd, HRESULT& hr, CString& error)
	: CMPCVRAllocatorPresenter(hWnd, hr, error)
{
}

CSubtitleHelperAllocatorPresenter::~CSubtitleHelperAllocatorPresenter()
{
}

STDMETHODIMP CSubtitleHelperAllocatorPresenter::AttachToRenderer(IUnknown* pRenderer)
{
	CheckPointer(pRenderer, E_POINTER);

	if (m_pMPCVR)
		return E_FAIL; // can only attach once

	m_pMPCVR = pRenderer;

	__super::AttachToRenderer();

	return S_OK;
}

// ISubRenderOptions
STDMETHODIMP CSubtitleHelperAllocatorPresenter::GetString(LPCSTR field, LPWSTR* value, int* chars)
{
	CheckPointer(value, E_POINTER);
	CheckPointer(chars, E_POINTER);

	CString ret;

	if (strcmp(field, "name") == 0) 
		ret = L"MPC SubtitleHelper";

	if (ret.IsEmpty() == false) 
	{
		const int len = ret.GetLength();
		const size_t sz = (len + 1) * sizeof(WCHAR);
		LPWSTR buf = (LPWSTR)LocalAlloc(LPTR, sz);

		if (!buf) {
			return E_OUTOFMEMORY;
		}

		wcscpy_s(buf, len + 1, ret);
		*chars = len;
		*value = buf;

		return S_OK;
	}

	return __super::GetString(field, value, chars);
}

void CSubtitleHelperAllocatorPresenter::SetPositionFromRenderer()
{
	RECT w = {}, v = {};

	if (CComQIPtr<IBasicVideo> pBasicVideo = m_pMPCVR)
	{
		long left, top, width, height;

		if (SUCCEEDED(pBasicVideo->GetDestinationPosition(&left, &top, &width, &height)))
			v = { left, top, left + width, top + height };
	}

	if (CComQIPtr<IVideoWindow> pVideoWindow = m_pMPCVR)
	{
		long left, top, width, height;

		if (SUCCEEDED(pVideoWindow->GetWindowPosition(&left, &top, &width, &height)))
			w = { left, top, left + width, top + height };
	}

	// Note: We're intentionally calling the grandparent, since we should not update the renderer
	CAllocatorPresenterImpl::SetPosition(w, v);
}
