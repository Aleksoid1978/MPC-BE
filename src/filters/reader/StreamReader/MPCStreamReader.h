/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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

#include <qnetwork.h>
#include "LiveStream.h"
#include <ExtLib/AsyncReader/asyncrdr.h>

#define StreamReaderName L"MPC Stream Reader"

class __declspec(uuid("0E49B128-9547-4423-88F9-897837E298F5"))
	CUDPReader
	: public CAsyncReader
	, public IFileSourceFilter
	, public IAMMediaContent
{
	CLiveStream m_stream;
	CStringW    m_fn;

public:
	CUDPReader(IUnknown* pUnk, HRESULT* phr);
	~CUDPReader() = default;

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

	// IAMMediaContent
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) { return E_NOTIMPL; }
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) { return E_NOTIMPL; }
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) { return E_NOTIMPL; }
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) { return E_NOTIMPL; }
	STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName) { return E_NOTIMPL; }
	STDMETHODIMP get_Title(BSTR* pbstrTitle);
	STDMETHODIMP get_Rating(BSTR* pbstrRating) { return E_NOTIMPL; }
	STDMETHODIMP get_Description(BSTR* pbstrDescription);
	STDMETHODIMP get_Copyright(BSTR* pbstrCopyright) { return E_NOTIMPL; }
	STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL);
	STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText) { return E_NOTIMPL; }
};
