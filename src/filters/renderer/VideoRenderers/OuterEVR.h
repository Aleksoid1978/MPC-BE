/*
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

/// === Outer EVR
namespace DSObjects
{
	class COuterEVR
		: public CUnknown
		, public IBaseFilter
	{
		CComPtr<IUnknown> m_pEVR;
		IBaseFilter* m_pEVRBase;
		CEVRAllocatorPresenter* m_pAllocatorPresenter;

	public:
		COuterEVR(const TCHAR* pName, LPUNKNOWN pUnk, HRESULT& hr, CEVRAllocatorPresenter* pAllocatorPresenter);
		~COuterEVR();

		DECLARE_IUNKNOWN;
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

		// IBaseFilter
		STDMETHODIMP EnumPins(__out  IEnumPins** ppEnum);
		STDMETHODIMP FindPin(LPCWSTR Id, __out  IPin** ppPin);
		STDMETHODIMP QueryFilterInfo(__out  FILTER_INFO* pInfo);
		STDMETHODIMP JoinFilterGraph(__in_opt  IFilterGraph* pGraph, __in_opt  LPCWSTR pName);
		STDMETHODIMP QueryVendorInfo(__out  LPWSTR* pVendorInfo);
		STDMETHODIMP Stop();
		STDMETHODIMP Pause();
		STDMETHODIMP Run(REFERENCE_TIME tStart);
		STDMETHODIMP GetState(DWORD dwMilliSecsTimeout, __out  FILTER_STATE* State);
		STDMETHODIMP SetSyncSource(__in_opt  IReferenceClock* pClock);
		STDMETHODIMP GetSyncSource(__deref_out_opt  IReferenceClock** pClock);
		STDMETHODIMP GetClassID(__RPC__out CLSID* pClassID);
	};
}
