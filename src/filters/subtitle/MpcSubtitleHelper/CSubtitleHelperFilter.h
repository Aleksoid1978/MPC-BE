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

#include "ISubtitleHelper.h"
#include "SubtitleHelperAllocatorPresenter.h"

class __declspec(uuid("9075FBA6-E312-434C-A8D1-2996794A3B0A")) CSubtitleHelperFilter
	: public CBaseFilter
	, public ISubtitleHelper
{
public:
	CSubtitleHelperFilter(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CSubtitleHelperFilter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    virtual int GetPinCount() override
	{
		return 0;
	}

    virtual CBasePin *GetPin(int n) override
	{
		return nullptr;
	}

	virtual STDMETHODIMP Connect(HWND hWnd) override;
	virtual STDMETHODIMP Disconnect() override;
	virtual STDMETHODIMP_(void) SetPositionFromRenderer() override;

	static constexpr LPCTSTR szName = _T("MPC Subtitle Helper");

private:
	CComPtr<IUnknown> FindVideoRenderer();

private:
	CCritSec								   mInterfaceLock; // Critical section for interfaces
	HWND									   mhOwnerWnd = nullptr;
	CComPtr<CSubtitleHelperAllocatorPresenter> mpSubtitleConsumer;
};

