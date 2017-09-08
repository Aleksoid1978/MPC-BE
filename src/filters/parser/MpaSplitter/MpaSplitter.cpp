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
#include <MMReg.h>
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include "MpaSplitter.h"
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Audio},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CMpaSplitterFilter), MpaSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CMpaSourceFilter), MpaSourceName, MERIT_NORMAL+1, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMpaSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMpaSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	CAtlList<CString> chkbytes;
	chkbytes.AddTail(L"0,2,FFE0,FFE0");
	chkbytes.AddTail(L"0,10,FFFFFF00000080808080,49443300000000000000");
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG1Audio, chkbytes, nullptr);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CMpaSplitterFilter
//

CMpaSplitterFilter::CMpaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMpaSplitterFilter"), pUnk, phr, __uuidof(this))
{
	m_nFlag |= SOURCE_SUPPORT_URL;
}

STDMETHODIMP CMpaSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CMpaSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, MpaSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

HRESULT CMpaSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();

	m_pFile.Attach(DNew CMpaSplitterFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	CAtlArray<CMediaType> mts;
	mts.Add(m_pFile->GetMediaType());

	CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Audio", this, this, &hr));
	AddOutputPin(0, pPinOut);

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->IsStreaming() ? 0 : m_pFile->GetDuration();

	SetID3TagProperties(this, m_pFile->m_pID3Tag);
	SetAPETagProperties(this, m_pFile->m_pAPETag);

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CMpaSplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CheckPointer(m_pFile, VFW_E_NOT_CONNECTED);

	*pDuration = m_pFile->IsStreaming() ? m_rtDuration : m_pFile->GetDuration();

	return S_OK;
}

bool CMpaSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CMpaSplitterFilter");
	if (!m_pFile) {
		return false;
	}

	// TODO

	return true;
}

void CMpaSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	__int64 startpos = m_pFile->GetStartPos();
	__int64 endpos = m_pFile->GetLength();

	if (rt <= 0 || m_pFile->GetDuration() <= 0) {
		m_pFile->Seek(startpos);
		m_rtime = 0;
	} else {
		m_pFile->Seek(startpos + (__int64)((1.0 * rt / m_pFile->GetDuration()) * (endpos - startpos)));
		m_rtime = rt;
	}
}

bool CMpaSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	int FrameSize;
	REFERENCE_TIME rtDuration;

	BOOL bFirst = TRUE;

	while (SUCCEEDED(hr) && !CheckRequest(nullptr) && (m_pFile->GetRemaining() > 9 || m_pFile->IsStreaming())) {
		if (!m_pFile->Sync(FrameSize, rtDuration, DEF_SYNC_SIZE, bFirst)) {
			continue;
		}

		CAutoPtr<CPacket> p(DNew CPacket());

		FrameSize = min(FrameSize, m_pFile->GetRemaining());
		p->SetCount(FrameSize);
		m_pFile->ByteRead(p->GetData(), FrameSize);

		p->TrackNumber = 0;
		p->rtStart = m_rtime;
		p->rtStop  = m_rtime + rtDuration;
		p->bSyncPoint = TRUE;

		hr = DeliverPacket(p);

		m_rtime += rtDuration;

		bFirst = FALSE;
	}

	return true;
}

//
// CMpaSourceFilter
//

CMpaSourceFilter::CMpaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMpaSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
