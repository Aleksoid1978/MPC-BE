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

#include "stdafx.h"
#include "DSUtil/DSUtil.h"
#include "MPCStreamReader.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_TRANSPORT},
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CUDPReader), StreamReaderName, MERIT_NORMAL, std::size(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CUDPReader>, nullptr, &sudFilter[0]}
};

int g_cTemplates = std::size(g_Templates);

STDAPI DllRegisterServer()
{
	SetRegKeyValue(L"udp", 0, L"Source Filter", CStringFromGUID(__uuidof(CUDPReader)));
	SetRegKeyValue(L"http", 0, L"Source Filter", CStringFromGUID(__uuidof(CUDPReader)));
	SetRegKeyValue(L"https", 0, L"Source Filter", CStringFromGUID(__uuidof(CUDPReader)));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	// TODO

	return AMovieDllRegisterServer2(FALSE);
}

#include "filters/filters/Filters.h"

CFilterApp theApp;

#endif

//
// CUDPReader
//

CUDPReader::CUDPReader(IUnknown* pUnk, HRESULT* phr)
	: CAsyncReader(L"CUDPReader", pUnk, &m_stream, phr, __uuidof(this))
{
	if (phr) {
		*phr = S_OK;
	}
}

STDMETHODIMP CUDPReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IFileSourceFilter)
		QI2(IAMMediaContent)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CUDPReader::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	const wchar_t* readerName;
	switch (m_stream.GetProtocol()) {
		case CLiveStream::protocol::PR_UDP:
			readerName = StreamReaderName L" [UDP]";
			break;
		case CLiveStream::protocol::PR_HTTP:
			readerName = StreamReaderName L" [HTTP]";
			break;
		case CLiveStream::protocol::PR_PIPE:
			readerName = StreamReaderName L" [stdin]";
			break;
		case CLiveStream::protocol::PR_HLS:
			readerName = StreamReaderName L" [HLS-Live]";
			break;
		default:
			readerName = StreamReaderName;
			break;
	}

	wcscpy_s(pInfo->achName, readerName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

STDMETHODIMP CUDPReader::Stop()
{
	m_stream.CallWorker(CLiveStream::CMD::CMD_STOP);
	return S_OK;
}

STDMETHODIMP CUDPReader::Pause()
{
	return S_OK;
}

STDMETHODIMP CUDPReader::Run(REFERENCE_TIME tStart)
{
	m_stream.CallWorker(CLiveStream::CMD::CMD_RUN);
	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CUDPReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	if (!m_stream.Load(pszFileName)) {
		return E_FAIL;
	}

	m_fn = pszFileName;

	m_mt.majortype = MEDIATYPE_Stream;
	m_mt.subtype   = m_stream.GetSubtype();

	return S_OK;
}

STDMETHODIMP CUDPReader::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	size_t nCount = m_fn.GetLength() + 1;
	*ppszFileName = (LPOLESTR)CoTaskMemAlloc(nCount * sizeof(WCHAR));
	CheckPointer(*ppszFileName, E_OUTOFMEMORY);
	wcscpy_s(*ppszFileName, nCount, m_fn);

	return S_OK;
}

// IAMMediaContent

STDMETHODIMP CUDPReader::get_Title(BSTR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);

	if (m_stream.GetTitle().GetLength()) {
		*pbstrTitle = m_stream.GetTitle().AllocSysString();
		return S_OK;
	}

	return E_UNEXPECTED;
}

STDMETHODIMP CUDPReader::get_Description(BSTR* pbstrDescription)
{
	CheckPointer(pbstrDescription, E_POINTER);

	if (m_stream.GetDescription().GetLength()) {
		*pbstrDescription = m_stream.GetDescription().AllocSysString();
		return S_OK;
	}

	return E_UNEXPECTED;
}

STDMETHODIMP CUDPReader::get_BaseURL(BSTR* pbstrBaseURL)
{
	CheckPointer(pbstrBaseURL, E_POINTER);

	if (m_stream.GetUrl().GetLength()) {
		*pbstrBaseURL = m_stream.GetUrl().AllocSysString();
		return S_OK;
	}

	return E_UNEXPECTED;
}
