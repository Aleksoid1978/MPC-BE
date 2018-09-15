/*
 * (C) 2003-2006 Gabest
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

#include "stdafx.h"
#include "DSMSplitter.h"
#include "../../../DSUtil/DSUtil.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_DirectShowMedia},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CDSMSplitterFilter), DSMSplitterName, MERIT_NORMAL, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CDSMSourceFilter), DSMSourceName, MERIT_NORMAL, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDSMSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CDSMSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	CString str;
	str.Format(L"0,%d,,%%0%dI64x", DSMSW_SIZE, DSMSW_SIZE*2);
	str.Format(CString(str), DSMSW);

	RegisterSourceFilter(
		CLSID_AsyncReader,
		MEDIASUBTYPE_DirectShowMedia,
		str, nullptr);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_DirectShowMedia);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CDSMSplitterFilter
//

CDSMSplitterFilter::CDSMSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"CDSMSplitterFilter", pUnk, phr, __uuidof(this))
{
}

CDSMSplitterFilter::~CDSMSplitterFilter()
{
}

STDMETHODIMP CDSMSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, DSMSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

HRESULT CDSMSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pFile.Attach(DNew CDSMSplitterFile(pAsyncReader, hr, *this, *this));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->m_rtDuration;

	std::vector<BYTE> ids;
	ids.reserve(m_pFile->m_mts.size());

	for (const auto& [id, mt] : m_pFile->m_mts) {
		ids.push_back(id);
	}

	std::sort(ids.begin(), ids.end());

	for (size_t i = 0; i < ids.size(); i++) {
		BYTE id = ids[i];
		CMediaType& mt = m_pFile->m_mts[id];

		CStringW name, lang;
		name.Format(L"Output %02d", id);

		std::vector<CMediaType> mts;
		mts.push_back(mt);

		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, name, this, this, &hr));

		name.Empty();

		for (const auto& [key, value] : m_pFile->m_sim[id]) {
			pPinOut->SetProperty(CStringW(key), value);

			if (key == "NAME") {
				name = value;
			}
			if (key == "LANG") if ((lang = ISO6392ToLanguage(CStringA(value))).IsEmpty()) {
					lang = value;
				}
		}

		if (!name.IsEmpty() || !lang.IsEmpty()) {
			if (!name.IsEmpty()) {
				if (!lang.IsEmpty()) {
					name += L" (" + lang + L")";
				}
			} else if (!lang.IsEmpty()) {
				name = lang;
			}
			pPinOut->SetName(name);
		}

		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pPinOut)));
	}

	for (const auto& [key, value] : m_pFile->m_fim) {
		SetProperty(CStringW(key), value);
	}

	for (size_t i = 0; i < m_resources.size(); i++) {
		const CDSMResource& r = m_resources[i];
		if (r.mime == L"application/x-truetype-font" ||
				r.mime == L"application/x-font-ttf" ||
				r.mime == L"application/vnd.ms-opentype") {
			m_fontinst.InstallFontMemory(r.data.data(), r.data.size());
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CDSMSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CDSMSplitterFilter");
	return true;
}

void CDSMSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	m_pFile->Seek(m_pFile->FindSyncPoint(rt));
}

bool CDSMSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	while (SUCCEEDED(hr) && !CheckRequest(nullptr) && m_pFile->GetRemaining()) {
		dsmp_t type;
		UINT64 len;

		if (!m_pFile->Sync(type, len)) {
			continue;
		}

		__int64 pos = m_pFile->GetPos();

		if (type == DSMP_SAMPLE) {
			CAutoPtr<CPacket> p(DNew CPacket());
			if (m_pFile->Read(len, p)) {
				if (p->rtStart != INVALID_TIME) {
					p->rtStart -= m_pFile->m_rtFirst;
					p->rtStop -= m_pFile->m_rtFirst;
				}

				hr = DeliverPacket(p);
			}
		}

		m_pFile->Seek(pos + len);
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CDSMSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);
	nKFs = m_pFile->m_sps.size();
	return S_OK;
}

STDMETHODIMP CDSMSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_pFile, E_UNEXPECTED);

	if (*pFormat != TIME_FORMAT_MEDIA_TIME) {
		return E_INVALIDARG;
	}

	// these aren't really the keyframes, but quicky accessable points in the stream
	for (nKFs = 0; nKFs < m_pFile->m_sps.size(); nKFs++) {
		pKFs[nKFs] = m_pFile->m_sps[nKFs].rt;
	}

	return S_OK;
}

//
// CDSMSourceFilter
//

CDSMSourceFilter::CDSMSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CDSMSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
