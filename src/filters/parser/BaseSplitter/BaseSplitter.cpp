/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2019 see Authors.txt
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
#include <InitGuid.h>
#include <moreuuids.h>
#include "../../../DSUtil/std_helper.h"
#include "BaseSplitter.h"

//
// CBaseSplitterFilter
//

CBaseSplitterFilter::CBaseSplitterFilter(LPCTSTR pName, LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseFilter(pName, pUnk, this, clsid)
	, m_rtDuration(0), m_rtStart(0), m_rtStop(0), m_rtCurrent(0)
	, m_dRate(1.0)
	, m_nOpenProgress(100)
	, m_fAbort(false)
	, m_rtLastStart(INVALID_TIME)
	, m_rtLastStop(INVALID_TIME)
	, m_priority(THREAD_PRIORITY_NORMAL)
	, m_nFlag(0)
	, m_iQueueDuration(QUEUE_DURATION_DEF)
	//, m_iNetworkTimeout(NETWORK_TIMEOUT_DEF)
{
	if (phr) {
		*phr = S_OK;
	}

	m_pInput.Attach(DNew CBaseSplitterInputPin(L"CBaseSplitterInputPin", this, this, phr));
}

CBaseSplitterFilter::~CBaseSplitterFilter()
{
	CAutoLock cAutoLock(this);

	CAMThread::CallWorker(CMD_EXIT);
	CAMThread::Close();
}

bool CBaseSplitterFilter::IsSomePinDrying()
{
	for (const auto pPin: m_pActivePins) {
		if (!pPin->IsDiscontinuous() && pPin->QueueCount() == 0) {
			return true;
		}
	}

	return false;
}

STDMETHODIMP CBaseSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = nullptr;

	if (m_pInput && riid == __uuidof(IFileSourceFilter)) {
		return E_NOINTERFACE;
	}

	return
		QI(IFileSourceFilter)
		QI(IMediaSeeking)
		QI(IAMOpenProgress)
		QI2(IAMMediaContent)
		QI2(IAMExtendedSeeking)
		QI(IKeyFrameInfo)
		QI(IBufferInfo)
		QI(IExFilterConfig)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		QI(IDSMResourceBag)
		QI(IDSMChapterBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

CBaseSplitterOutputPin* CBaseSplitterFilter::GetOutputPin(DWORD TrackNum)
{
	CAutoLock cAutoLock(&m_csPinMap);

	auto it = m_pPinMap.find(TrackNum);
	if (it != m_pPinMap.end()) {
		return (*it).second;
	}

	return nullptr;
}

DWORD CBaseSplitterFilter::GetOutputTrackNum(CBaseSplitterOutputPin* pPin)
{
	CAutoLock cAutoLock(&m_csPinMap);

	for (const auto& [TrackNum, pPinTmp] : m_pPinMap) {
		if (pPinTmp == pPin) {
			return TrackNum;
		}
	}

	return (DWORD)-1;
}

HRESULT CBaseSplitterFilter::RenameOutputPin(DWORD TrackNumSrc, DWORD TrackNumDst, std::vector<CMediaType> mts, BOOL bNeedReconnect /*= FALSE*/)
{
	CAutoLock cAutoLock(&m_csPinMap);

	auto it = m_pPinMap.find(TrackNumSrc);
	if (it != m_pPinMap.end()) {
		CBaseSplitterOutputPin* pPin = (*it).second;
		AM_MEDIA_TYPE* pmt = nullptr;
		HRESULT hr = S_OK;

		if (CComQIPtr<IPin> pPinTo = pPin->GetConnected()) {
			for (size_t i = 0; i < mts.size(); i++) {
				if (S_OK == pPinTo->QueryAccept(&mts[i])) {
					pmt = &mts[i];
					break;
				}
			}

			if (!pmt) {
				pmt = &mts[0];
			}

			if (bNeedReconnect) {
				hr = ReconnectPin(pPinTo, pmt);
			}
		}

		m_pPinMap.erase(it);
		m_pPinMap[TrackNumDst] = pPin;

		if (pmt) {
			CAutoLock cAutoLock(&m_csmtnew);
			m_mtnew[TrackNumDst] = *pmt;
		}

		return hr;
	}

	return E_FAIL;
}

HRESULT CBaseSplitterFilter::AddOutputPin(DWORD TrackNum, CAutoPtr<CBaseSplitterOutputPin> pPin)
{
	CAutoLock cAutoLock(&m_csPinMap);

	if (!pPin) {
		return E_INVALIDARG;
	}
	m_pPinMap[TrackNum] = pPin;
	m_pOutputs.AddTail(pPin);
	return S_OK;
}

HRESULT CBaseSplitterFilter::DeleteOutputs()
{
	m_rtDuration = 0;

	m_pRetiredOutputs.RemoveAll();

	CAutoLock cAutoLockF(this);
	if (m_State != State_Stopped) {
		return VFW_E_NOT_STOPPED;
	}

	while (m_pOutputs.GetCount()) {
		CAutoPtr<CBaseSplitterOutputPin> pPin = m_pOutputs.RemoveHead();
		if (IPin* pPinTo = pPin->GetConnected()) {
			pPinTo->Disconnect();
		}
		pPin->Disconnect();
		// we can't just let it be deleted now, something might have AddRefed on it (graphedit...)
		m_pRetiredOutputs.AddTail(pPin);
	}

	CAutoLock cAutoLockPM(&m_csPinMap);
	m_pPinMap.clear();

	CAutoLock cAutoLockMT(&m_csmtnew);
	m_mtnew.clear();

	RemoveAll();
	ResRemoveAll();
	ChapRemoveAll();

	m_fontinst.UninstallFonts();

	m_pSyncReader.Release();

	return S_OK;
}

void CBaseSplitterFilter::DeliverBeginFlush()
{
	m_fFlushing = true;
	POSITION pos = m_pOutputs.GetHeadPosition();
	while (pos) {
		m_pOutputs.GetNext(pos)->DeliverBeginFlush();
	}
}

void CBaseSplitterFilter::DeliverEndFlush()
{
	POSITION pos = m_pOutputs.GetHeadPosition();
	while (pos) {
		m_pOutputs.GetNext(pos)->DeliverEndFlush();
	}
	m_fFlushing = false;
	m_eEndFlush.Set();
}

DWORD CBaseSplitterFilter::ThreadProc()
{
	if (m_pSyncReader) {
		m_pSyncReader->SetBreakEvent(GetRequestHandle());
	}

	if (!DemuxInit()) {
		for (;;) {
			DWORD cmd = GetRequest();
			if (cmd == CMD_EXIT) {
				CAMThread::m_hThread = nullptr;
			}
			Reply(S_OK);
			if (cmd == CMD_EXIT) {
				return 0;
			}
		}
	}

	m_eEndFlush.Set();
	m_fFlushing = false;

	for (DWORD cmd = (DWORD)-1; ; cmd = GetRequest()) {
		if (cmd == CMD_EXIT) {
			m_hThread = nullptr;
			Reply(S_OK);
			return 0;
		}

		SetThreadPriority(m_hThread, m_priority);

		m_rtStart = m_rtNewStart;
		m_rtStop = m_rtNewStop;

		DemuxSeek(m_rtStart);

		if (cmd != (DWORD)-1) {
			Reply(S_OK);
		}

		m_eEndFlush.Wait();

		m_pActivePins.clear();

		POSITION pos = m_pOutputs.GetHeadPosition();
		while (pos && !m_fFlushing) {
			CBaseSplitterOutputPin* pPin = m_pOutputs.GetNext(pos);
			if (pPin->IsConnected() && pPin->IsActive()) {
				m_pActivePins.push_back(pPin);
				pPin->DeliverNewSegment(m_rtStart, m_rtStop, m_dRate);
			}
		}

		m_rtOffset = INVALID_TIME;

		do {
			m_bDiscontinuitySent.clear();
		} while (!DemuxLoop());

		for (const auto pPin : m_pActivePins) {
			if (CheckRequest(&cmd)) {
				break;
			}
			pPin->QueueEndOfStream();
		}
	}

	ASSERT(0); // we should only exit via CMD_EXIT

	m_hThread = nullptr;
	return 0;
}

HRESULT CBaseSplitterFilter::DeliverPacket(CAutoPtr<CPacket> p)
{
	HRESULT hr = S_FALSE;

	CBaseSplitterOutputPin* pPin = GetOutputPin(p->TrackNumber);
	if (!pPin || !pPin->IsConnected() || !Contains(m_pActivePins, pPin)) {
		return S_FALSE;
	}

	if (p->rtStart != INVALID_TIME) {
		m_rtCurrent = p->rtStart;

		p->rtStart -= m_rtStart;
		p->rtStop -= m_rtStart;

		ASSERT(p->rtStart <= p->rtStop);
	}

	{
		CAutoLock cAutoLock(&m_csmtnew);

		auto it = m_mtnew.find(p->TrackNumber);
		if (it != m_mtnew.end()) {
			p->pmt = CreateMediaType(&(*it).second);
			m_mtnew.erase(it);
		}
	}

	if (!Contains(m_bDiscontinuitySent, p->TrackNumber)) {
		p->bDiscontinuity = TRUE;
	}

	DWORD TrackNumber = p->TrackNumber;
	BOOL bDiscontinuity = p->bDiscontinuity;

#if defined(DEBUG_OR_LOG) && 0
	DLog(L"[%u]: d%d s%d p%d, b=%Iu, [%20I64d - %20I64d]",
		  p->TrackNumber,
		  p->bDiscontinuity, p->bSyncPoint, p->rtStart != INVALID_TIME && p->rtStart < 0,
		  p->GetCount(), p->rtStart, p->rtStop);
#endif

	hr = pPin->QueuePacket(p);

	if (S_OK != hr) {
		auto it = std::find(m_pActivePins.begin(), m_pActivePins.end(), pPin);
		if (it != m_pActivePins.end()) {
			m_pActivePins.erase(it);
		}

		if (m_pActivePins.size()) { // only die when all pins are down
			hr = S_OK;
		}

		return hr;
	}

	if (bDiscontinuity) {
		m_bDiscontinuitySent.push_back(TrackNumber);
	}

	return hr;
}

HRESULT CBaseSplitterFilter::BreakConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if (dir == PINDIR_INPUT) {
		DeleteOutputs();
	} else if (dir == PINDIR_OUTPUT) {
	} else {
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT CBaseSplitterFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if (dir == PINDIR_INPUT) {
		CBaseSplitterInputPin* pIn = static_cast<CBaseSplitterInputPin*>(pPin);

		HRESULT hr;

		CComPtr<IAsyncReader> pAsyncReader;
		if (FAILED(hr = pIn->GetAsyncReader(&pAsyncReader))
				|| FAILED(hr = DeleteOutputs())
				|| FAILED(hr = CreateOutputs(pAsyncReader))) {
			return hr;
		}

		SortOutputPin();
		ChapSort();

		m_pSyncReader = pAsyncReader;
	} else if (dir == PINDIR_OUTPUT) {
		m_pRetiredOutputs.RemoveAll();
	} else {
		return E_UNEXPECTED;
	}

	return S_OK;
}

int CBaseSplitterFilter::GetPinCount()
{
	return (m_pInput ? 1 : 0) + (int)m_pOutputs.GetCount();
}

CBasePin* CBaseSplitterFilter::GetPin(int n)
{
	CAutoLock cAutoLock(this);

	if (n >= 0 && n < (int)m_pOutputs.GetCount()) {
		if (POSITION pos = m_pOutputs.FindIndex(n)) {
			return m_pOutputs.GetAt(pos);
		}
	}

	if (n == (int)m_pOutputs.GetCount() && m_pInput) {
		return m_pInput;
	}

	return nullptr;
}

STDMETHODIMP CBaseSplitterFilter::Stop()
{
	CAutoLock cAutoLock(this);

	DeliverBeginFlush();
	CallWorker(CMD_EXIT);
	DeliverEndFlush();

	HRESULT hr;
	if (FAILED(hr = __super::Stop())) {
		return hr;
	}

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::Pause()
{
	CAutoLock cAutoLock(this);

	FILTER_STATE fs = m_State;

	HRESULT hr;
	if (FAILED(hr = __super::Pause())) {
		return hr;
	}

	if (fs == State_Stopped) {
		Create();
	}

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;
	if (FAILED(hr = __super::Run(tStart))) {
		return hr;
	}

	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CBaseSplitterFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CheckPointer(pszFileName, E_POINTER);

	m_fn = pszFileName;
	HRESULT hr = E_FAIL;
	CComPtr<IAsyncReader> pAsyncReader;

	if (BuildPlaylist(pszFileName, m_Items)) {
		pAsyncReader = (IAsyncReader*)DNew CAsyncFileReader(m_Items, hr);
	} else {
		pAsyncReader = (IAsyncReader*)DNew CAsyncFileReader(pszFileName, hr, m_nFlag & SOURCE_SUPPORT_URL);
	}

	if (FAILED(hr)
			|| FAILED(hr = DeleteOutputs())
			|| FAILED(hr = CreateOutputs(pAsyncReader))) {
		m_fn.Empty();
		return hr;
	}

	SortOutputPin();

	CHdmvClipInfo::CPlaylistChapter Chapters;
	if (BuildChapters(pszFileName, m_Items, Chapters)) {
		int i = 1;
		for (const auto& chap : Chapters) {
			CString str;
			if (chap.m_nMarkType == CHdmvClipInfo::EntryMark) {
				if (!chap.m_strTitle.IsEmpty()) {
					str = chap.m_strTitle;
				} else {
					str.Format(L"Chapter %d", i);
				}
				ChapAppend(chap.m_rtTimestamp, str);
				i++;
			}
		}
	}

	ChapSort();

	m_pSyncReader = pAsyncReader;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	size_t nCount = m_fn.GetLength() + 1;
	*ppszFileName = (LPOLESTR)CoTaskMemAlloc(nCount * sizeof(WCHAR));
	CheckPointer(*ppszFileName, E_OUTOFMEMORY);

	wcscpy_s(*ppszFileName, nCount, m_fn);
	return S_OK;
}

LPCTSTR CBaseSplitterFilter::GetPartFilename(IAsyncReader* pAsyncReader)
{
	CComQIPtr<IFileHandle>	pFH = pAsyncReader;
	return pFH && pFH->IsValidFileName() ? pFH->GetFileName() : m_fn;
}

// IMediaSeeking

STDMETHODIMP CBaseSplitterFilter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities =
			   AM_SEEKING_CanGetStopPos|
			   AM_SEEKING_CanGetDuration|
			   AM_SEEKING_CanSeekAbsolute|
			   AM_SEEKING_CanSeekForwards|
			   AM_SEEKING_CanSeekBackwards, S_OK : E_POINTER;
}

STDMETHODIMP CBaseSplitterFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	if (*pCapabilities == 0) {
		return S_OK;
	}
	DWORD caps;
	GetCapabilities(&caps);
	if ((caps&*pCapabilities) == 0) {
		return E_FAIL;
	}
	if (caps == *pCapabilities) {
		return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CBaseSplitterFilter::IsFormatSupported(const GUID* pFormat)
{
	return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CBaseSplitterFilter::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterFilter::GetTimeFormat(GUID* pFormat)
{
	return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;
}

STDMETHODIMP CBaseSplitterFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CBaseSplitterFilter::SetTimeFormat(const GUID* pFormat)
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CBaseSplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);

	if (m_rtDuration <= 0) {
		return E_FAIL;
	}

	*pDuration = m_rtDuration;
	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetStopPosition(LONGLONG* pStop)
{
	return GetDuration(pStop);
}

STDMETHODIMP CBaseSplitterFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseSplitterFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseSplitterFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return SetPositionsInternal(this, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CBaseSplitterFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if (pCurrent) {
		*pCurrent = m_rtCurrent;
	}
	if (pStop) {
		*pStop = m_rtStop;
	}
	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if (pEarliest) {
		*pEarliest = 0;
	}
	return GetDuration(pLatest);
}

STDMETHODIMP CBaseSplitterFilter::SetRate(double dRate)
{
	return dRate > 0 ? m_dRate = dRate, S_OK : E_INVALIDARG;
}

STDMETHODIMP CBaseSplitterFilter::GetRate(double* pdRate)
{
	return pdRate ? *pdRate = m_dRate, S_OK : E_POINTER;
}

STDMETHODIMP CBaseSplitterFilter::GetPreroll(LONGLONG* pllPreroll)
{
	return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;
}

HRESULT CBaseSplitterFilter::SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	CAutoLock cAutoLock(this);

	if (!pCurrent && !pStop
			|| (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning
			&& (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning) {
		return S_OK;
	}

	REFERENCE_TIME
	rtCurrent = m_rtCurrent,
	rtStop = m_rtStop;

	if (pCurrent)
		switch (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) {
			case AM_SEEKING_NoPositioning:
				break;
			case AM_SEEKING_AbsolutePositioning:
				rtCurrent = *pCurrent;
				break;
			case AM_SEEKING_RelativePositioning:
			case AM_SEEKING_IncrementalPositioning:
				rtCurrent = rtCurrent + *pCurrent;
				break;
		}

	if (pStop)
		switch (dwStopFlags&AM_SEEKING_PositioningBitsMask) {
			case AM_SEEKING_NoPositioning:
				break;
			case AM_SEEKING_AbsolutePositioning:
				rtStop = *pStop;
				break;
			case AM_SEEKING_RelativePositioning:
				rtStop += *pStop;
				break;
			case AM_SEEKING_IncrementalPositioning:
				rtStop = rtCurrent + *pStop;
				break;
		}

	if (m_rtCurrent == rtCurrent && m_rtStop == rtStop) {
		return S_OK;
	}

	if (m_rtLastStart == rtCurrent && m_rtLastStop == rtStop && !Contains(m_LastSeekers, id)) {
		m_LastSeekers.push_back(id);
		return S_OK;
	}

	m_rtLastStart = rtCurrent;
	m_rtLastStop = rtStop;
	m_LastSeekers.clear();
	m_LastSeekers.push_back(id);

	DLog(L"CBaseSplitterFilter() : Seek Started %I64d", rtCurrent);

	m_rtNewStart = m_rtCurrent = rtCurrent;
	m_rtNewStop = rtStop;

	if (ThreadExists()) {
		DeliverBeginFlush();
		CallWorker(CMD_SEEK);
		DeliverEndFlush();
	}

	DLog(L"CBaseSplitterFilter() : Seek Ended");

	return S_OK;
}

// IAMOpenProgress

STDMETHODIMP CBaseSplitterFilter::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
	CheckPointer(pllTotal, E_POINTER);
	CheckPointer(pllCurrent, E_POINTER);

	*pllTotal = 100;
	*pllCurrent = m_nOpenProgress;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::AbortOperation()
{
	m_fAbort = true;
	return S_OK;
}

// IAMMediaContent

STDMETHODIMP CBaseSplitterFilter::get_AuthorName(BSTR* pbstrAuthorName)
{
	return GetProperty(L"AUTH", pbstrAuthorName);
}

STDMETHODIMP CBaseSplitterFilter::get_Title(BSTR* pbstrTitle)
{
	return GetProperty(L"TITL", pbstrTitle);
}

STDMETHODIMP CBaseSplitterFilter::get_Rating(BSTR* pbstrRating)
{
	return GetProperty(L"RTNG", pbstrRating);
}

STDMETHODIMP CBaseSplitterFilter::get_Description(BSTR* pbstrDescription)
{
	return GetProperty(L"DESC", pbstrDescription);
}

STDMETHODIMP CBaseSplitterFilter::get_Copyright(BSTR* pbstrCopyright)
{
	return GetProperty(L"CPYR", pbstrCopyright);
}

// IAMExtendedSeeking

STDMETHODIMP CBaseSplitterFilter::get_ExSeekCapabilities(long* pExCapabilities)
{
	CheckPointer(pExCapabilities, E_POINTER);
	*pExCapabilities = AM_EXSEEK_CANSEEK;
	if (ChapGetCount()) {
		*pExCapabilities |= AM_EXSEEK_MARKERSEEK;
	}
	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::get_MarkerCount(long* pMarkerCount)
{
	CheckPointer(pMarkerCount, E_POINTER);
	*pMarkerCount = (long)ChapGetCount();
	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::get_CurrentMarker(long* pCurrentMarker)
{
	CheckPointer(pCurrentMarker, E_POINTER);
	REFERENCE_TIME rt = m_rtCurrent;
	long i = ChapLookup(&rt);
	if (i < 0) {
		return E_FAIL;
	}
	*pCurrentMarker = i+1;
	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetMarkerTime(long MarkerNum, double* pMarkerTime)
{
	CheckPointer(pMarkerTime, E_POINTER);
	REFERENCE_TIME rt;
	if (FAILED(ChapGet((int)MarkerNum-1, &rt))) {
		return E_FAIL;
	}
	*pMarkerTime = (double)rt / 10000000;
	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetMarkerName(long MarkerNum, BSTR* pbstrMarkerName)
{
	return ChapGet((int)MarkerNum-1, nullptr, pbstrMarkerName);
}

// IKeyFrameInfo

STDMETHODIMP CBaseSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	return E_NOTIMPL;
}

// IBufferInfo

STDMETHODIMP_(int) CBaseSplitterFilter::GetCount()
{
	CAutoLock cAutoLock(m_pLock);

	return (int)m_pOutputs.GetCount();
}

STDMETHODIMP CBaseSplitterFilter::GetStatus(int i, int& samples, int& size)
{
	CAutoLock cAutoLock(m_pLock);

	if (POSITION pos = m_pOutputs.FindIndex(i)) {
		CBaseSplitterOutputPin* pPin = m_pOutputs.GetAt(pos);
		samples = pPin->QueueCount();
		size = pPin->QueueSize();
		return pPin->IsConnected() ? S_OK : S_FALSE;
	}

	return E_INVALIDARG;
}

STDMETHODIMP_(DWORD) CBaseSplitterFilter::GetPriority()
{
	return m_priority;
}

// CExFilterConfig

STDMETHODIMP CBaseSplitterFilter::GetInt(LPCSTR field, int *value)
{
	CheckPointer(value, E_POINTER);

	if (strcmp(field, "queueDuration") == 0) {
		*value = m_iQueueDuration;
		return S_OK;
	}

	//if (strcmp(field, "networkTimeout") == 0) {
	//	*value = m_iNetworkTimeout;
	//	return S_OK;
	//}

	return E_INVALIDARG;
}

STDMETHODIMP CBaseSplitterFilter::SetInt(LPCSTR field, int value)
{
	if (strcmp(field, "queueDuration") == 0) {
		if (value < QUEUE_DURATION_MIN || value > QUEUE_DURATION_MAX) {
			return E_INVALIDARG;
		}
		m_iQueueDuration = value;
		return S_OK;
	}

	//if (strcmp(field, "networkTimeout") == 0) {
	//	if (value < NETWORK_TIMEOUT_MIN || value > NETWORK_TIMEOUT_MAX) {
	//		return E_INVALIDARG;
	//	}
	//	m_iNetworkTimeout = value;
	//	return S_OK;
	//}

	return E_INVALIDARG;
}

void CBaseSplitterFilter::SortOutputPin()
{
	// Sorting output pin - video at the beginning of the list.

	CAutoPtrList<CBaseSplitterOutputPin> m_pOutputsVideo;
	CAutoPtrList<CBaseSplitterOutputPin> m_pOutputsOther;

	POSITION pos = m_pOutputs.GetHeadPosition();
	while (pos) {
		CAutoPtr<CBaseSplitterOutputPin> pin;
		pin.Attach(m_pOutputs.GetNext(pos).Detach());
		CMediaType mt;
		if (SUCCEEDED(pin->GetMediaType(0, &mt))) {
			if (mt.majortype == MEDIATYPE_Video) {
				m_pOutputsVideo.AddTail(pin);
			} else {
				m_pOutputsOther.AddTail(pin);
			}
		}
	}

	m_pOutputs.RemoveAll();
	pos = m_pOutputsVideo.GetHeadPosition();
	while (pos) {
		CAutoPtr<CBaseSplitterOutputPin> pin;
		pin.Attach(m_pOutputsVideo.GetNext(pos).Detach());
		m_pOutputs.AddTail(pin);
	}
	pos = m_pOutputsOther.GetHeadPosition();
	while (pos) {
		CAutoPtr<CBaseSplitterOutputPin> pin;
		pin.Attach(m_pOutputsOther.GetNext(pos).Detach());
		m_pOutputs.AddTail(pin);
	}
}