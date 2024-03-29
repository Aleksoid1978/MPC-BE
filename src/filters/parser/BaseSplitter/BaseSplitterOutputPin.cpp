/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
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
#include <clsids.h>
#include <moreuuids.h>
#include "filters/switcher/AudioSwitcher/StreamSwitcher.h" // for IStreamSwitcherInputPin
#include "BaseSplitterOutputPin.h"
#include "BaseSplitter.h"

//
// CBaseSplitterOutputPin
//

CBaseSplitterOutputPin::CBaseSplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(L"CBaseSplitterOutputPin", pFilter, pLock, phr, GetMediaTypeDesc(mts, pName, pFilter))
	, m_pSplitter(static_cast<CBaseSplitterFilter*>(m_pFilter))
{
	m_mts = mts;
	memset(&m_brs, 0, sizeof(m_brs));
	m_brs.rtLastDeliverTime = INVALID_TIME;
}

CBaseSplitterOutputPin::CBaseSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(L"CBaseSplitterOutputPin", pFilter, pLock, phr, pName)
	, m_pSplitter(static_cast<CBaseSplitterFilter*>(m_pFilter))
{
	memset(&m_brs, 0, sizeof(m_brs));
	m_brs.rtLastDeliverTime = INVALID_TIME;
}

CBaseSplitterOutputPin::~CBaseSplitterOutputPin()
{
}

STDMETHODIMP CBaseSplitterOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IMediaSeeking)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		QI(IBitRateInfo)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CBaseSplitterOutputPin::SetName(LPCWSTR pName)
{
	CheckPointer(pName, E_POINTER);

	if (m_pName) {
		delete[] m_pName;
	}
	const size_t len = wcslen(pName) + 1;
	m_pName = DNew WCHAR[len];
	wcscpy_s(m_pName, len, pName);

	return S_OK;
}

HRESULT CBaseSplitterOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	ASSERT(pAlloc);
	ASSERT(pProperties);

	HRESULT hr = NOERROR;

	pProperties->cBuffers = std::max(1L, pProperties->cBuffers);
	pProperties->cbBuffer = std::max(1uL, m_mt.lSampleSize);

	// TODO: is this still needed ?
	if (m_mt.subtype == MEDIASUBTYPE_Vorbis && m_mt.formattype == FORMAT_VorbisFormat) {
		// oh great, the oggds vorbis decoder assumes there will be two at least, stupid thing...
		pProperties->cBuffers = std::max(pProperties->cBuffers, 2L);
	}

	ALLOCATOR_PROPERTIES Actual;
	if (FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) {
		return hr;
	}

	if (Actual.cbBuffer < pProperties->cbBuffer) {
		return E_FAIL;
	}
	ASSERT(Actual.cBuffers == pProperties->cBuffers);

	return NOERROR;
}

HRESULT CBaseSplitterOutputPin::CheckMediaType(const CMediaType* pmt)
{
	for (size_t i = 0; i < m_mts.size(); i++) {
		if (*pmt == m_mts[i]) {
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

HRESULT CBaseSplitterOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	CAutoLock cAutoLock(m_pLock);

	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if ((size_t)iPosition >= m_mts.size()) {
		return VFW_S_NO_MORE_ITEMS;
	}

	*pmt = m_mts[iPosition];

	return S_OK;
}

//

HRESULT CBaseSplitterOutputPin::Active()
{
	CAutoLock cAutoLock(m_pLock);

	if (m_Connected) {
		Create();
	}

	return __super::Active();
}

HRESULT CBaseSplitterOutputPin::Inactive()
{
	CAutoLock cAutoLock(m_pLock);

	if (ThreadExists()) {
		CallWorker(CMD_EXIT);
	}

	return __super::Inactive();
}

HRESULT CBaseSplitterOutputPin::DeliverBeginFlush()
{
	m_eEndFlush.Reset();
	m_fFlushed = false;
	m_fFlushing = true;
	m_hrDeliver = S_FALSE;
	m_queue.RemoveAll();
	HRESULT hr = IsConnected() ? GetConnected()->BeginFlush() : S_OK;
	if (S_OK != hr) {
		m_eEndFlush.Set();
	}
	return hr;
}

HRESULT CBaseSplitterOutputPin::DeliverEndFlush()
{
	if (!ThreadExists()) {
		return S_FALSE;
	}
	HRESULT hr = IsConnected() ? GetConnected()->EndFlush() : S_OK;
	m_hrDeliver = S_OK;
	m_fFlushing = false;
	m_fFlushed = true;
	m_eEndFlush.Set();
	return hr;
}

HRESULT CBaseSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_brs.rtLastDeliverTime = INVALID_TIME;

	m_rtPrev = INVALID_TIME;

	if (m_fFlushing) {
		return S_FALSE;
	}
	m_rtStart = tStart;
	if (!ThreadExists()) {
		return S_FALSE;
	}
	if (m_pSplitter->m_iQueueDuration >= QUEUE_DURATION_MIN && m_pSplitter->m_iQueueDuration <= QUEUE_DURATION_MAX) {
		m_maxQueueDuration = m_pSplitter->m_iQueueDuration * 10000;
		m_maxQueueCount = m_pSplitter->m_iQueueDuration * 12 / 10;
	}
	HRESULT hr = __super::DeliverNewSegment(tStart, tStop, dRate);
	if (S_OK != hr) {
		return hr;
	}
	MakeISCRHappy();
	return hr;
}

size_t CBaseSplitterOutputPin::QueueCount()
{
	return m_queue.GetCount();
}

size_t CBaseSplitterOutputPin::QueueSize()
{
	return m_queue.GetSize();
}

HRESULT CBaseSplitterOutputPin::QueueEndOfStream()
{
	return QueuePacket(std::unique_ptr<CPacket>()); // nullptr means EndOfStream
}

HRESULT CBaseSplitterOutputPin::QueuePacket(std::unique_ptr<CPacket> p)
{
	if (!ThreadExists()) {
		return S_FALSE;
	}

	CBaseSplitterFilter *pSplitter = static_cast<CBaseSplitterFilter*>(m_pFilter);

	while (S_OK == m_hrDeliver) {
		const auto count = m_queue.GetCount();
		const auto duration = m_queue.GetDuration();

		if (duration < m_maxQueueDuration && count < m_maxQueueCount // the buffer is not full
				|| duration < 60*10000000 && count < 60*1200 && pSplitter->IsSomePinDrying() // some pins should not be empty, but to a certain limit
				) {
			break;
		}

		Sleep(10);
	}

	if (S_OK == m_hrDeliver) {
		m_queue.Add(p);
	}

	return m_hrDeliver;
}

bool CBaseSplitterOutputPin::IsDiscontinuous()
{
	return CMediaTypeEx(m_mt).ValidateSubtitle();
}

bool CBaseSplitterOutputPin::IsActive()
{
	CComPtr<IPin> pPin = this;
	do {
		CComPtr<IPin> pPinTo;
		CComQIPtr<IStreamSwitcherInputPin> pSSIP;
		if (S_OK == pPin->ConnectedTo(&pPinTo) && (pSSIP = pPinTo) && !pSSIP->IsActive()) {
			return false;
		}
		pPin = GetFirstPin(GetFilterFromPin(pPinTo), PINDIR_OUTPUT);
	} while (pPin);

	return true;
}

DWORD CBaseSplitterOutputPin::ThreadProc()
{
	SetThreadName((DWORD)-1, "CBaseSplitterOutputPin");
	m_hrDeliver = S_OK;
	m_fFlushing = m_fFlushed = false;
	m_eEndFlush.Set();

	// fix for Microsoft DTV-DVD Video Decoder - video freeze after STOP/PLAY
	bool iHaaliRenderConnect = false;
	CComPtr<IPin> pPinTo = this, pTmp;
	while (pPinTo && SUCCEEDED(pPinTo->ConnectedTo(&pTmp)) && (pPinTo = pTmp)) {
		pTmp = nullptr;
		CComPtr<IBaseFilter> pBF = GetFilterFromPin(pPinTo);
		if (GetCLSID(pBF) == CLSID_DXR) { // Haali Renderer
			iHaaliRenderConnect = true;
			break;
		}
		pPinTo = GetFirstPin(pBF, PINDIR_OUTPUT);
	}
	if (IsConnected() && !iHaaliRenderConnect) {
		GetConnected()->BeginFlush();
		GetConnected()->EndFlush();
	}

	for (;;) {
		Sleep(1);

		DWORD cmd;
		if (CheckRequest(&cmd)) {
			m_hThread = nullptr;
			cmd = GetRequest();
			Reply(S_OK);
			ASSERT(cmd == CMD_EXIT);
			return 0;
		}

		size_t cnt = 0;
		do {
			std::unique_ptr<CPacket> p;
			m_queue.RemoveSafe(p, cnt);

			if (S_OK == m_hrDeliver && cnt > 0) {
				ASSERT(!m_fFlushing);

				m_fFlushed = false;

				// flushing can still start here, to release a blocked deliver call

				HRESULT hr = p
							 ? DeliverPacket(std::move(p))
							 : DeliverEndOfStream();

				m_eEndFlush.Wait(); // .. so we have to wait until it is done

				if (hr != S_OK && !m_fFlushed) { // and only report the error in m_hrDeliver if we didn't flush the stream
					// CAutoLock cAutoLock(&m_csQueueLock);
					m_hrDeliver = hr;
					break;
				}
			}
		} while (cnt > 1);
	}
}

#define MAX_PTS_SHIFT 50000000i64
HRESULT CBaseSplitterOutputPin::DeliverPacket(std::unique_ptr<CPacket> p)
{
	HRESULT hr;

	long nBytes = (long)p->size();

	if (nBytes == 0) {
		return S_OK;
	}

	if (p->rtStart != INVALID_TIME && (m_pSplitter->GetFlag() & PACKET_PTS_DISCONTINUITY)) {
		if (m_rtPrev == INVALID_TIME && m_pSplitter->m_rtOffset == INVALID_TIME) {
			m_rtPrev = 0;
			m_pSplitter->m_rtOffset = 0;
		}

		// Filter invalid PTS value (if too different from previous packet)
		if (!IsDiscontinuous()) {
			if (m_rtPrev != INVALID_TIME) {
				const REFERENCE_TIME rt = p->rtStart + m_pSplitter->m_rtOffset;
				if (llabs(rt - m_rtPrev) > MAX_PTS_SHIFT) {
					m_pSplitter->m_rtOffset += m_rtPrev - rt;
					DLog(L"CBaseSplitterOutputPin::DeliverPacket() : [%u] packet discontinuity detected, adjusting offset to %I64d", p->TrackNumber, m_pSplitter->m_rtOffset);
				}
			}
		}

		p->rtStart += m_pSplitter->m_rtOffset;
		p->rtStop += m_pSplitter->m_rtOffset;

		m_rtPrev = p->rtStart;
	}

	m_brs.nBytesSinceLastDeliverTime += nBytes;

	if (p->rtStart != INVALID_TIME) {
		if (m_brs.rtLastDeliverTime == INVALID_TIME) {
			m_brs.rtLastDeliverTime = p->rtStart;
			m_brs.nBytesSinceLastDeliverTime = 0;
		}

		if (m_brs.rtLastDeliverTime + 10000000 < p->rtStart) {
			REFERENCE_TIME rtDiff = p->rtStart - m_brs.rtLastDeliverTime;

			double secs, bits;

			secs = (double)rtDiff / 10000000;
			bits = 8.0 * m_brs.nBytesSinceLastDeliverTime;
			m_brs.nCurrentBitRate = (DWORD)(bits / secs);

			m_brs.rtTotalTimeDelivered += rtDiff;
			m_brs.nTotalBytesDelivered += m_brs.nBytesSinceLastDeliverTime;

			secs = (double)m_brs.rtTotalTimeDelivered / 10000000;
			bits = 8.0 * m_brs.nTotalBytesDelivered;
			m_brs.nAverageBitRate = (DWORD)(bits / secs);

			m_brs.rtLastDeliverTime = p->rtStart;
			m_brs.nBytesSinceLastDeliverTime = 0;
			/*
			DLog(L"[%u] c: %u kbps, a: %u kbps",
				p->TrackNumber,
				(m_brs.nCurrentBitRate+500)/1000,
				(m_brs.nAverageBitRate+500)/1000);
			*/
		}

		double dRate = 1.0;
		if (SUCCEEDED(m_pSplitter->GetRate(&dRate))) {
			p->rtStart = (REFERENCE_TIME)((double)p->rtStart / dRate);
			p->rtStop = (REFERENCE_TIME)((double)p->rtStop / dRate);
		}
	}

	do {
		CComPtr<IMediaSample> pSample;
		if (S_OK != (hr = GetDeliveryBuffer(&pSample, nullptr, nullptr, 0))) {
			break;
		}

		if (nBytes > pSample->GetSize()) {
			pSample.Release();

			ALLOCATOR_PROPERTIES props, actual;
			if (S_OK != (hr = m_pAllocator->GetProperties(&props))) {
				break;
			}
			props.cbBuffer = nBytes*3/2;

			if (props.cBuffers > 1) {
				if (S_OK != (hr = __super::DeliverBeginFlush())) {
					break;
				}
				if (S_OK != (hr = __super::DeliverEndFlush())) {
					break;
				}
			}

			if (S_OK != (hr = m_pAllocator->Decommit())) {
				break;
			}
			if (S_OK != (hr = m_pAllocator->SetProperties(&props, &actual))) {
				break;
			}
			if (S_OK != (hr = m_pAllocator->Commit())) {
				break;
			}
			if (S_OK != (hr = GetDeliveryBuffer(&pSample, nullptr, nullptr, 0))) {
				break;
			}
		}

		if (p->pmt) {
			pSample->SetMediaType(p->pmt);
			p->bDiscontinuity = true;

			// CAutoLock cAutoLock(m_pLock); // this can cause the lock
			m_mts.clear();
			m_mts.emplace_back(*p->pmt);
		}

		bool fTimeValid = p->rtStart != INVALID_TIME;

#if defined(DEBUG_OR_LOG) && 0
		DLog(L"[%u]: d%d s%d p%d, b=%d, [%20I64d - %20I64d]",
			  p->TrackNumber,
			  p->bDiscontinuity, p->bSyncPoint, fTimeValid && p->rtStart < 0,
			  nBytes, p->rtStart, p->rtStop);
#endif

		ASSERT(!p->bSyncPoint || fTimeValid);

		BYTE* pData = nullptr;
		if (S_OK != (hr = pSample->GetPointer(&pData)) || !pData) {
			break;
		}
		memcpy(pData, p->data(), nBytes);
		if (S_OK != (hr = pSample->SetActualDataLength(nBytes))) {
			break;
		}
		if (S_OK != (hr = pSample->SetTime(fTimeValid ? &p->rtStart : nullptr, fTimeValid ? &p->rtStop : nullptr))) {
			break;
		}
		if (S_OK != (hr = pSample->SetMediaTime(nullptr, nullptr))) {
			break;
		}
		if (S_OK != (hr = pSample->SetDiscontinuity(p->bDiscontinuity))) {
			break;
		}
		if (S_OK != (hr = pSample->SetSyncPoint(p->bSyncPoint))) {
			break;
		}
		if (S_OK != (hr = pSample->SetPreroll(fTimeValid && p->rtStart < 0))) {
			break;
		}
		if (S_OK != (hr = Deliver(pSample))) {
			break;
		}
	} while (false);

	return hr;
}

void CBaseSplitterOutputPin::MakeISCRHappy()
{
	CComPtr<IPin> pPinTo = this, pTmp;
	while (pPinTo && SUCCEEDED(pPinTo->ConnectedTo(&pTmp)) && (pPinTo = pTmp)) {
		pTmp = nullptr;

		CComPtr<IBaseFilter> pBF = GetFilterFromPin(pPinTo);

		if (GetCLSID(pBF) == GUIDFromCString(L"{48025243-2D39-11CE-875D-00608CB78066}")) { // ISCR
			std::unique_ptr<CPacket> p(DNew CPacket());
			p->TrackNumber = (DWORD)-1;
			p->rtStart = -1;
			p->rtStop = 0;
			p->bSyncPoint = FALSE;
			p->SetData(" ", 2);
			QueuePacket(std::move(p));
			break;
		}

		pPinTo = GetFirstPin(pBF, PINDIR_OUTPUT);
	}
}

HRESULT CBaseSplitterOutputPin::GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	return __super::GetDeliveryBuffer(ppSample, pStartTime, pEndTime, dwFlags);
}

HRESULT CBaseSplitterOutputPin::Deliver(IMediaSample* pSample)
{
	return __super::Deliver(pSample);
}

// IMediaSeeking

STDMETHODIMP CBaseSplitterOutputPin::GetCapabilities(DWORD* pCapabilities)
{
	return m_pSplitter->GetCapabilities(pCapabilities);
}

STDMETHODIMP CBaseSplitterOutputPin::CheckCapabilities(DWORD* pCapabilities)
{
	return m_pSplitter->CheckCapabilities(pCapabilities);
}

STDMETHODIMP CBaseSplitterOutputPin::IsFormatSupported(const GUID* pFormat)
{
	return m_pSplitter->IsFormatSupported(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::QueryPreferredFormat(GUID* pFormat)
{
	return m_pSplitter->QueryPreferredFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::GetTimeFormat(GUID* pFormat)
{
	return m_pSplitter->GetTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::IsUsingTimeFormat(const GUID* pFormat)
{
	return m_pSplitter->IsUsingTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::SetTimeFormat(const GUID* pFormat)
{
	return m_pSplitter->SetTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::GetDuration(LONGLONG* pDuration)
{
	return m_pSplitter->GetDuration(pDuration);
}

STDMETHODIMP CBaseSplitterOutputPin::GetStopPosition(LONGLONG* pStop)
{
	return m_pSplitter->GetStopPosition(pStop);
}

STDMETHODIMP CBaseSplitterOutputPin::GetCurrentPosition(LONGLONG* pCurrent)
{
	return m_pSplitter->GetCurrentPosition(pCurrent);
}

STDMETHODIMP CBaseSplitterOutputPin::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return m_pSplitter->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return m_pSplitter->SetPositionsInternal(this, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CBaseSplitterOutputPin::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	return m_pSplitter->GetPositions(pCurrent, pStop);
}

STDMETHODIMP CBaseSplitterOutputPin::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	return m_pSplitter->GetAvailable(pEarliest, pLatest);
}

STDMETHODIMP CBaseSplitterOutputPin::SetRate(double dRate)
{
	return m_pSplitter->SetRate(dRate);
}

STDMETHODIMP CBaseSplitterOutputPin::GetRate(double* pdRate)
{
	return m_pSplitter->GetRate(pdRate);
}

STDMETHODIMP CBaseSplitterOutputPin::GetPreroll(LONGLONG* pllPreroll)
{
	return m_pSplitter->GetPreroll(pllPreroll);
}
