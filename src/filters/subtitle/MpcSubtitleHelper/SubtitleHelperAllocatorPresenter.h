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

#pragma once

#include "filters/renderer/VideoRenderers/MPCVRAllocatorPresenter.h"

class __declspec(uuid("0D86BBA9-2FD8-42D0-A2A0-1A2090836087")) CSubtitleHelperAllocatorPresenter :
	public DSObjects::CMPCVRAllocatorPresenter
{
public:
	CSubtitleHelperAllocatorPresenter(HWND hWnd, HRESULT& hr, CString& error);
	virtual ~CSubtitleHelperAllocatorPresenter();

	STDMETHODIMP AttachToRenderer(IUnknown* pRenderer);

	// IAllocatorPresenter
	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) override
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP_(CLSID) GetAPCLSID() override
	{
		return __uuidof(this);
	}

	// ISubRenderOptions
	STDMETHODIMP GetString(LPCSTR field, LPWSTR* value, int* chars) override;

	void SetPositionFromRenderer();
};

