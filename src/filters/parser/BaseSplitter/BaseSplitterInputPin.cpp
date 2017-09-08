/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include "BaseSplitterInputPin.h"
#include "BaseSplitter.h"

//
// CBaseSplitterInputPin
//

CBaseSplitterInputPin::CBaseSplitterInputPin(TCHAR* pName, CBaseSplitterFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBasePin(pName, pFilter, pLock, phr, L"Input", PINDIR_INPUT)
{
}

CBaseSplitterInputPin::~CBaseSplitterInputPin()
{
}

HRESULT CBaseSplitterInputPin::GetAsyncReader(IAsyncReader** ppAsyncReader)
{
	CheckPointer(ppAsyncReader, E_POINTER);
	*ppAsyncReader = nullptr;
	CheckPointer(m_pAsyncReader, VFW_E_NOT_CONNECTED);
	(*ppAsyncReader = m_pAsyncReader)->AddRef();
	return S_OK;
}

STDMETHODIMP CBaseSplitterInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CBaseSplitterInputPin::CheckMediaType(const CMediaType* pmt)
{
	return S_OK;
}

HRESULT CBaseSplitterInputPin::CheckConnect(IPin* pPin)
{
	HRESULT hr;
	if (FAILED(hr = __super::CheckConnect(pPin))) {
		return hr;
	}

	return CComQIPtr<IAsyncReader>(pPin) ? S_OK : E_NOINTERFACE;
}

HRESULT CBaseSplitterInputPin::BreakConnect()
{
	HRESULT hr;

	if (FAILED(hr = __super::BreakConnect())) {
		return hr;
	}

	if (FAILED(hr = (static_cast<CBaseSplitterFilter*>(m_pFilter))->BreakConnect(PINDIR_INPUT, this))) {
		return hr;
	}

	m_pAsyncReader.Release();

	return S_OK;
}

HRESULT CBaseSplitterInputPin::CompleteConnect(IPin* pPin)
{
	HRESULT hr;

	if (FAILED(hr = __super::CompleteConnect(pPin))) {
		return hr;
	}

	CheckPointer(pPin, E_POINTER);
	m_pAsyncReader = pPin;
	CheckPointer(m_pAsyncReader, E_NOINTERFACE);

	if (FAILED(hr = (static_cast<CBaseSplitterFilter*>(m_pFilter))->CompleteConnect(PINDIR_INPUT, this))) {
		return hr;
	}

	return S_OK;
}

STDMETHODIMP CBaseSplitterInputPin::BeginFlush()
{
	return E_UNEXPECTED;
}

STDMETHODIMP CBaseSplitterInputPin::EndFlush()
{
	return E_UNEXPECTED;
}
