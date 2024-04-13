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
#include "CSubtitleHelperFilter.h"
#include "clsids.h"
#include "DSUtil/ds_defines.h"

CSubtitleHelperFilter::CSubtitleHelperFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(szName, pUnk, &mInterfaceLock, __uuidof(this))
{
}

CSubtitleHelperFilter::~CSubtitleHelperFilter()
{
	Disconnect();
}

STDMETHODIMP CSubtitleHelperFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

#define TRY_RETURN_AS(_interface) if (riid == __uuidof(_interface)) return GetInterface((_interface *) this, ppv)

	TRY_RETURN_AS(ISubtitleHelper);

#undef TRY_RETURN_AS

	if ((riid != IID_IUnknown) && mpSubtitleConsumer)
	{
		if (SUCCEEDED(mpSubtitleConsumer->NonDelegatingQueryInterface(riid, ppv)))
			return S_OK;
	}

	return __super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CSubtitleHelperFilter::Connect(HWND hWnd)
{
	HRESULT ahRet(S_OK);
	CString aoErrorMessage;

	mpSubtitleConsumer.Release();

	mhOwnerWnd = hWnd;
	if (mhOwnerWnd != nullptr)
		mpSubtitleConsumer = new CSubtitleHelperAllocatorPresenter(mhOwnerWnd, ahRet, aoErrorMessage);

	if (SUCCEEDED(ahRet) && mpSubtitleConsumer)
	{
		auto pVideoRenderer = FindVideoRenderer();
		if (pVideoRenderer)
		{
			ahRet = mpSubtitleConsumer->AttachToRenderer(pVideoRenderer);
		}
		else
		{
			mpSubtitleConsumer.Release();

			ahRet = E_FAIL;
		}
	}

	return ahRet;
}

STDMETHODIMP CSubtitleHelperFilter::Disconnect()
{
	mhOwnerWnd = nullptr;
	mpSubtitleConsumer.Release();

	return S_OK;
}

STDMETHODIMP_(void) CSubtitleHelperFilter::SetPositionFromRenderer()
{
	if (mpSubtitleConsumer)
		mpSubtitleConsumer->SetPositionFromRenderer();
}

CComPtr<IUnknown> CSubtitleHelperFilter::FindVideoRenderer()
{
	CComPtr<IUnknown> apVideoRenderer;
	HRESULT			  ahRes;

    BeginEnumFilters(m_pGraph, pEnumFilters, pBaseFilter)
    {
		CLSID aFilterId;

		ahRes = pBaseFilter->GetClassID(&aFilterId);
		if (SUCCEEDED(ahRes) && (aFilterId == CLSID_MPCVR))
		{
			apVideoRenderer = pBaseFilter;
			break;
		}
    }
    EndEnumFilters;//Add a ; so that my editor can do a correct auto-indent

	return apVideoRenderer;
}

