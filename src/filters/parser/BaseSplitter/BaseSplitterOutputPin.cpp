/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include "../../../DSUtil/DSUtil.h"
#include <moreuuids.h>
#include "../../switcher/AudioSwitcher/AudioSwitcher.h"
#include "BaseSplitterOutputPin.h"
#include "BaseSplitter.h"
#include "../../parser/MpegSplitter/MpegSplitter.h"

//
// CBaseSplitterOutputPin
//

CBaseSplitterOutputPin::CBaseSplitterOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, double dFactor/* = 1.0*/)
	: CBaseOutputPin(NAME("CBaseSplitterOutputPin"), pFilter, pLock, phr, GetMediaTypeDesc(mts, pName, pFilter))
	, pSplitter(static_cast<CBaseSplitterFilter*>(m_pFilter))
	, m_hrDeliver(S_OK) // just in case it were asked before the worker thread could be created and reset it
	, m_fFlushing(false)
	, m_fFlushed(false)
	, m_eEndFlush(TRUE)
	, m_rtPrev(0)
	, m_rtOffset(0)
	, m_MinQueuePackets(max(10, pSplitter->GetMinQueuePackets() * (dFactor < 1.0 ? dFactor : 1)))
	, m_MaxQueuePackets(max(m_MinQueuePackets * 2, pSplitter->GetMaxQueuePackets() * dFactor))
	, m_MinQueueSize(pSplitter->GetMinQueueSize())
	, m_MaxQueueSize(pSplitter->GetMaxQueueSize())
{
	m_mts.Copy(mts);
	memset(&m_brs, 0, sizeof(m_brs));
	m_brs.rtLastDeliverTime = INVALID_TIME;
}

CBaseSplitterOutputPin::CBaseSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, double dFactor/* = 1.0*/)
	: CBaseOutputPin(NAME("CBaseSplitterOutputPin"), pFilter, pLock, phr, pName)
	, pSplitter(static_cast<CBaseSplitterFilter*>(m_pFilter))
	, m_hrDeliver(S_OK) // just in case it were asked before the worker thread could be created and reset it
	, m_fFlushing(false)
	, m_fFlushed(false)
	, m_eEndFlush(TRUE)
	, m_MinQueuePackets(max(10, pSplitter->GetMinQueuePackets() * (dFactor < 1.0 ? dFactor : 1)))
	, m_MaxQueuePackets(max(m_MinQueuePackets * 2, pSplitter->GetMaxQueuePackets() * dFactor))
	, m_MinQueueSize(pSplitter->GetMinQueueSize())
	, m_MaxQueueSize(pSplitter->GetMaxQueueSize())

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
	SAFE_DELETE_ARRAY(m_pName);

	size_t len = wcslen(pName) + 1;
	m_pName = DNew WCHAR[len];
	CheckPointer(m_pName, E_OUTOFMEMORY);
	wcscpy_s(m_pName, len, pName);
	return S_OK;
}

HRESULT CBaseSplitterOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	ASSERT(pAlloc);
	ASSERT(pProperties);

	HRESULT hr = NOERROR;

	pProperties->cBuffers = max(pProperties->cBuffers, m_nBuffers);
	pProperties->cbBuffer = max(m_mt.lSampleSize, 1);

	// TODO: is this still needed ?
	if (m_mt.subtype == MEDIASUBTYPE_Vorbis && m_mt.formattype == FORMAT_VorbisFormat) {
		// oh great, the oggds vorbis decoder assumes there will be two at least, stupid thing...
		pProperties->cBuffers = max(pProperties->cBuffers, 2);
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
	for (size_t i = 0; i < m_mts.GetCount(); i++) {
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
	if ((size_t)iPosition >= m_mts.GetCount()) {
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

	m_rtPrev	= 0;
	m_rtOffset	= 0;

	if (m_fFlushing) {
		return S_FALSE;
	}
	m_rtStart = tStart;
	if (!ThreadExists()) {
		return S_FALSE;
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
	return QueuePacket(CAutoPtr<CPacket>()); // NULL means EndOfStream
}

HRESULT CBaseSplitterOutputPin::QueuePacket(CAutoPtr<CPacket> p)
{
	if (!ThreadExists()) {
		return S_FALSE;
	}

	while (S_OK == m_hrDeliver
			&& (m_queue.GetSize() > m_MaxQueueSize
				|| m_queue.GetCount() > (m_MaxQueuePackets * 3 / 2)
				|| (m_queue.GetCount() > m_MaxQueuePackets && !pSplitter->IsAnyPinDrying(m_MaxQueuePackets)))) {
		Sleep(10);
	}

	if (S_OK != m_hrDeliver) {
		return m_hrDeliver;
	}

	m_queue.Add(p);

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
		pTmp = NULL;
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
			m_hThread = NULL;
			cmd = GetRequest();
			Reply(S_OK);
			ASSERT(cmd == CMD_EXIT);
			return 0;
		}

		int cnt = 0;
		do {
			CAutoPtr<CPacket> p;

			{
				CAutoLock cAutoLock(&m_queue);
				if ((cnt = m_queue.GetCount()) > 0) {
					p = m_queue.Remove();
				}
			}

			if (S_OK == m_hrDeliver && cnt > 0) {
				ASSERT(!m_fFlushing);

				m_fFlushed = false;

				// flushing can still start here, to release a blocked deliver call

				HRESULT hr = p
							 ? DeliverPacket(p)
							 : DeliverEndOfStream();

				m_eEndFlush.Wait(); // .. so we have to wait until it is done

				if (hr != S_OK && !m_fFlushed) { // and only report the error in m_hrDeliver if we didn't flush the stream
					// CAutoLock cAutoLock(&m_csQueueLock);
					m_hrDeliver = hr;
					break;
				}
			}
		} while (--cnt > 0);
	}
}

#define MAX_PTS_SHIFT 50000000i64
HRESULT CBaseSplitterOutputPin::DeliverPacket(CAutoPtr<CPacket> p)
{
	HRESULT hr;

	long nBytes = (long)p->GetCount();

	if (nBytes == 0) {
		return S_OK;
	}

	DWORD nFlag = pSplitter->GetFlag();

	if (p->rtStart != INVALID_TIME && (nFlag & PACKET_PTS_DISCONTINUITY)) {
		// Filter invalid PTS value (if too different from previous packet)
		if (!IsDiscontinuous() && !((nFlag & PACKET_PTS_VALIDATE_POSITIVE) && p->rtStart < 0)) {
			REFERENCE_TIME rt = p->rtStart + m_rtOffset;
			if (abs(rt - m_rtPrev) > MAX_PTS_SHIFT) {
				m_rtOffset += m_rtPrev - rt;
				DbgLog((LOG_TRACE, 3, L"CBaseSplitterOutputPin::DeliverPacket() : Packet discontinuity detected, adjusting offset to %I64d", m_rtOffset));
			}
		}

		p->rtStart	+= m_rtOffset;
		p->rtStop	+= m_rtOffset;

		m_rtPrev	= p->rtStart;
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
						TRACE(_T("[%d] c: %d kbps, a: %d kbps\n"),
							p->TrackNumber,
							(m_brs.nCurrentBitRate+500)/1000,
							(m_brs.nAverageBitRate+500)/1000);
			*/
		}

		double dRate = 1.0;
		if (SUCCEEDED(pSplitter->GetRate(&dRate))) {
			p->rtStart = (REFERENCE_TIME)((double)p->rtStart / dRate);
			p->rtStop = (REFERENCE_TIME)((double)p->rtStop / dRate);
		}
	}

	do {
		CComPtr<IMediaSample> pSample;
		if (S_OK != (hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0))) {
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
			if (S_OK != (hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0))) {
				break;
			}
		}

		if (p->pmt) {
			pSample->SetMediaType(p->pmt);
			p->bDiscontinuity = true;

			// CAutoLock cAutoLock(m_pLock); // this can cause the lock
			m_mts.RemoveAll();
			m_mts.Add(*p->pmt);
		}

		bool fTimeValid = p->rtStart != INVALID_TIME;

#if defined(_DEBUG) && 0
		TRACE(_T("[%d]: d%d s%d p%d, b=%d, [%20I64d - %20I64d]\n"),
			  p->TrackNumber,
			  p->bDiscontinuity, p->bSyncPoint, fTimeValid && p->rtStart < 0,
			  nBytes, p->rtStart, p->rtStop);
#endif

		ASSERT(!p->bSyncPoint || fTimeValid);

		BYTE* pData = NULL;
		if (S_OK != (hr = pSample->GetPointer(&pData)) || !pData) {
			break;
		}
		memcpy(pData, p->GetData(), nBytes);
		if (S_OK != (hr = pSample->SetActualDataLength(nBytes))) {
			break;
		}
		if (S_OK != (hr = pSample->SetTime(fTimeValid ? &p->rtStart : NULL, fTimeValid ? &p->rtStop : NULL))) {
			break;
		}
		if (S_OK != (hr = pSample->SetMediaTime(NULL, NULL))) {
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
		pTmp = NULL;

		CComPtr<IBaseFilter> pBF = GetFilterFromPin(pPinTo);

		if (GetCLSID(pBF) == GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}"))) { // ISCR
			CAutoPtr<CPacket> p(DNew CPacket());
			p->TrackNumber = (DWORD)-1;
			p->rtStart = -1;
			p->rtStop = 0;
			p->bSyncPoint = FALSE;
			p->SetData(" ", 2);
			QueuePacket(p);
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
	return pSplitter->GetCapabilities(pCapabilities);
}

STDMETHODIMP CBaseSplitterOutputPin::CheckCapabilities(DWORD* pCapabilities)
{
	return pSplitter->CheckCapabilities(pCapabilities);
}

STDMETHODIMP CBaseSplitterOutputPin::IsFormatSupported(const GUID* pFormat)
{
	return pSplitter->IsFormatSupported(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::QueryPreferredFormat(GUID* pFormat)
{
	return pSplitter->QueryPreferredFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::GetTimeFormat(GUID* pFormat)
{
	return pSplitter->GetTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::IsUsingTimeFormat(const GUID* pFormat)
{
	return pSplitter->IsUsingTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::SetTimeFormat(const GUID* pFormat)
{
	return pSplitter->SetTimeFormat(pFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::GetDuration(LONGLONG* pDuration)
{
	return pSplitter->GetDuration(pDuration);
}

STDMETHODIMP CBaseSplitterOutputPin::GetStopPosition(LONGLONG* pStop)
{
	return pSplitter->GetStopPosition(pStop);
}

STDMETHODIMP CBaseSplitterOutputPin::GetCurrentPosition(LONGLONG* pCurrent)
{
	return pSplitter->GetCurrentPosition(pCurrent);
}

STDMETHODIMP CBaseSplitterOutputPin::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return pSplitter->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
}

STDMETHODIMP CBaseSplitterOutputPin::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return pSplitter->SetPositionsInternal(this, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CBaseSplitterOutputPin::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	return pSplitter->GetPositions(pCurrent, pStop);
}

STDMETHODIMP CBaseSplitterOutputPin::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	return pSplitter->GetAvailable(pEarliest, pLatest);
}

STDMETHODIMP CBaseSplitterOutputPin::SetRate(double dRate)
{
	return pSplitter->SetRate(dRate);
}

STDMETHODIMP CBaseSplitterOutputPin::GetRate(double* pdRate)
{
	return pSplitter->GetRate(pdRate);
}

STDMETHODIMP CBaseSplitterOutputPin::GetPreroll(LONGLONG* pllPreroll)
{
	return pSplitter->GetPreroll(pllPreroll);
}

double CBaseSplitterOutputPin::CalcQueryFactor(const CMediaType* pmt)
{
	if (pmt->majortype == MEDIATYPE_Audio) {
		double dFactor = 4.0;
		if (pmt->subtype == MEDIASUBTYPE_DOLBY_TRUEHD) {
			dFactor *= 5.0;
		} else if (pmt->subtype == MEDIASUBTYPE_PCM || pmt->subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO) {
			dFactor *= 3.0;
		} else if (pmt->subtype == MEDIASUBTYPE_DTS || pmt->subtype == MEDIASUBTYPE_DTS2) {
			dFactor *= 2.0;
		}

		return dFactor;
	}
	
	return 1.0;
}