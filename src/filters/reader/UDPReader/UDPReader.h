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

#pragma once

#include "UDPStream.h"
#include <AsyncReader/asyncrdr.h>


#define UDPReaderName   L"MPC UDP/HTTP Reader"
#define STDInReaderName L"MPC Std input Reader"

class __declspec(uuid("0E4221A9-9718-48D5-A5CF-4493DAD4A015"))
	CUDPReader
	: public CAsyncReader
	, public IFileSourceFilter
{
	CUDPStream m_stream;
	CString    m_fn;

public:
	CUDPReader(IUnknown* pUnk, HRESULT* phr);
	~CUDPReader();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);
};
