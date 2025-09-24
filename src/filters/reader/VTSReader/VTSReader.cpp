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
#include "filters/transform/DeCSSFilter/IfoFile.h"
#include "VTSReader.h"
#include "DSUtil/DSUtil.h"

// option names
#define OPT_REGKEY_VTSReader		L"Software\\MPC-BE Filters\\VTS Reader"
#define OPT_SECTION_VTSReader		L"Filters\\VTS Reader"
#define OPT_EnableTitleSelection	L"EnableTitleSelection"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_PROGRAM},
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CVTSReader), VTSReaderName, MERIT_NORMAL, std::size(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CVTSReader>, nullptr, &sudFilter[0]}
};

int g_cTemplates = std::size(g_Templates);

STDAPI DllRegisterServer()
{
	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73}",
		L"0", L"0,12,,445644564944454F2D565453"); // "DVDVIDEO-VTS"

	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73}",
		L"0", L"0,12,,445644415544494F2D415453"); // "DVDAUDIO-ATS"

	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73}",
		L"Source Filter", L"{773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73}");

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	DeleteRegKey(L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73}");

	return AMovieDllRegisterServer2(FALSE);
}

#include "filters/filters/Filters.h"

CFilterApp theApp;

#else

#include "DSUtil/Profile.h"

#endif

//
// CVTSReader
//

CVTSReader::CVTSReader(IUnknown* pUnk, HRESULT* phr)
	: CAsyncReader(L"CVTSReader", pUnk, &m_stream, phr, __uuidof(this))
	, m_bEnableTitleSelection(false)
{
	if (phr) {
		*phr = S_OK;
	}

#ifdef REGISTER_FILTER
	CRegKey key;

	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_VTSReader, KEY_READ)) {
		DWORD dw;

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_EnableTitleSelection, dw)) {
			m_bEnableTitleSelection = !!dw;
		}
	}
#else
	AfxGetProfile().ReadBool(OPT_SECTION_VTSReader, OPT_EnableTitleSelection, m_bEnableTitleSelection);
#endif

}

CVTSReader::~CVTSReader()
{
}

STDMETHODIMP CVTSReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IFileSourceFilter)
		QI(ITrackInfo)
		QI(IDSMChapterBag)
		QI(IVTSReader)
		QI(ISpecifyPropertyPages)
		//QI(ISpecifyPropertyPages2)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CVTSReader::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, VTSReaderName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CVTSReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	if (!m_stream.Load(pszFileName, m_bEnableTitleSelection)) {
		return E_FAIL;
	}

	ChapRemoveAll();
	for (size_t i = 0; i < m_stream.GetChaptersCount(); i++) {
		CStringW chap;
		chap.Format(ResStr(IDS_AG_CHAPTER), i + 1);
		ChapAppend(m_stream.GetChapterTime(i), chap);
	}

	m_fn = pszFileName;

	CMediaType mt;
	mt.majortype = MEDIATYPE_Stream;
	mt.subtype = MEDIASUBTYPE_MPEG2_PROGRAM;
	m_mt = mt;

	return S_OK;
}

STDMETHODIMP CVTSReader::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	size_t nCount = m_fn.GetLength() + 1;
	*ppszFileName = (LPOLESTR)CoTaskMemAlloc(nCount * sizeof(WCHAR));
	CheckPointer(*ppszFileName, E_OUTOFMEMORY);

	wcscpy_s(*ppszFileName, nCount, m_fn);
	return S_OK;
}

// ITrackInfo

STDMETHODIMP_(BSTR) CVTSReader::GetTrackName(UINT aTrackIdx)
{
	return m_stream.GetTrackName(aTrackIdx); // return stream's language
}

// ISpecifyPropertyPages2

STDMETHODIMP CVTSReader::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	pPages->pElems[0] = __uuidof(CVTSReaderSettingsWnd);

	return S_OK;
}

STDMETHODIMP CVTSReader::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != nullptr) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CVTSReaderSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CVTSReaderSettingsWnd>(nullptr, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// IVTSReader

STDMETHODIMP CVTSReader::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_VTSReader)) {
		key.SetDWORDValue(OPT_EnableTitleSelection, m_bEnableTitleSelection);
	}
#else
	AfxGetProfile().WriteBool(OPT_SECTION_VTSReader, OPT_EnableTitleSelection, m_bEnableTitleSelection);
#endif

	return S_OK;
}

STDMETHODIMP CVTSReader::SetEnableTitleSelection(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bEnableTitleSelection = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CVTSReader::GetEnableTitleSelection()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bEnableTitleSelection;
}

STDMETHODIMP_(REFERENCE_TIME) CVTSReader::GetDuration()
{
	return m_stream.GetDuration();
}

STDMETHODIMP_(fraction_t) CVTSReader::GetAspectRatio()
{
	return m_stream.GetAspectRatio();
}


// CVTSStream

CVTSStream::CVTSStream() : m_off(0)
{
	m_ifo.reset(DNew CIfoFile());
	m_vob.reset(DNew CVobFile());
}

CVTSStream::~CVTSStream()
{
}

bool CVTSStream::Load(const WCHAR* fnw, bool bEnableTitleSelection)
{
	// TODO bEnableTitleSelection
	return (m_ifo && m_vob && m_ifo->OpenIFO(fnw, 0) && m_ifo->OpenVOB(m_vob.get()));
}

HRESULT CVTSStream::SetPointer(LONGLONG llPos)
{
	m_off = (int)(llPos & 2047);
	int lba = (int)(llPos / 2048);

	return lba == m_vob->Seek(lba) ? S_OK : S_FALSE;
}

HRESULT CVTSStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	CAutoLock lck(&m_csLock);

	DWORD len = dwBytesToRead;
	BYTE* ptr = pbBuffer;

	while (len > 0) {
		BYTE buff[2048];
		if (!m_vob->Read(buff)) {
			break;
		}

		int size = std::min(2048 - m_off, (int)std::min(len, 2048uL));

		memcpy(ptr, &buff[m_off], size);

		m_off = (m_off + size) & 2047;

		if (m_off > 0) {
			m_vob->Seek(m_vob->GetPosition() - 1);
		}

		ptr += size;
		len -= size;
	}

	if (pdwBytesRead) {
		*pdwBytesRead = ptr - pbBuffer;
	}

	return S_OK;
}

LONGLONG CVTSStream::Size(LONGLONG* pSizeAvailable)
{
	LONGLONG len = 2048i64 * m_vob->GetLength();
	if (pSizeAvailable) {
		*pSizeAvailable = len;
	}
	return(len);
}

DWORD CVTSStream::Alignment()
{
	return 1;
}

void CVTSStream::Lock()
{
	m_csLock.Lock();
}

void CVTSStream::Unlock()
{
	m_csLock.Unlock();
}

BSTR CVTSStream::GetTrackName(UINT aTrackIdx)
{
	return m_ifo->GetTrackName(aTrackIdx);
}

UINT CVTSStream::GetChaptersCount()
{
	return m_ifo->GetChaptersCount();
}

REFERENCE_TIME CVTSStream::GetChapterTime(UINT ChapterNumber)
{
	return m_ifo->GetChapterTime(ChapterNumber);
}

REFERENCE_TIME CVTSStream::GetDuration()
{
	return m_ifo->GetDuration();
}

fraction_t CVTSStream::GetAspectRatio()
{
	return m_ifo->GetAspectRatio();
}