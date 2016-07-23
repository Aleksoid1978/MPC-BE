/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include <aviriff.h>
#include <atlpath.h>
#include "BaseMuxer.h"
#include <InitGuid.h>
#include <moreuuids.h>
#include <basestruct.h>

#define MAXQUEUESIZE 100

//
// CBaseMuxerFilter
//

CBaseMuxerFilter::CBaseMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseFilter(NAME("CBaseMuxerFilter"), pUnk, this, clsid)
	, m_rtCurrent(0)
{
	if (phr) {
		*phr = S_OK;
	}
	m_pOutput.Attach(DNew CBaseMuxerOutputPin(L"Output", this, this, phr));
	AddInput();
}

CBaseMuxerFilter::~CBaseMuxerFilter()
{
}

STDMETHODIMP CBaseMuxerFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return
		QI(IMediaSeeking)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		QI(IDSMResourceBag)
		QI(IDSMChapterBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

//

void CBaseMuxerFilter::AddInput()
{
	POSITION pos = m_pInputs.GetHeadPosition();
	while (pos) {
		CBasePin* pPin = m_pInputs.GetNext(pos);
		if (!pPin->IsConnected()) {
			return;
		}
	}

	CStringW name;

	name.Format(L"Input %d", m_pInputs.GetCount()+1);

	CBaseMuxerInputPin* pInputPin = NULL;
	if (FAILED(CreateInput(name, &pInputPin)) || !pInputPin) {
		ASSERT(0);
		return;
	}
	CAutoPtr<CBaseMuxerInputPin> pAutoPtrInputPin(pInputPin);

	name.Format(L"~Output %d", m_pRawOutputs.GetCount()+1);

	CBaseMuxerRawOutputPin* pRawOutputPin = NULL;
	if (FAILED(CreateRawOutput(name, &pRawOutputPin)) || !pRawOutputPin) {
		ASSERT(0);
		return;
	}
	CAutoPtr<CBaseMuxerRawOutputPin> pAutoPtrRawOutputPin(pRawOutputPin);

	pInputPin->SetRelatedPin(pRawOutputPin);
	pRawOutputPin->SetRelatedPin(pInputPin);

	m_pInputs.AddTail(pAutoPtrInputPin);
	m_pRawOutputs.AddTail(pAutoPtrRawOutputPin);
}

HRESULT CBaseMuxerFilter::CreateInput(CStringW name, CBaseMuxerInputPin** ppPin)
{
	CheckPointer(ppPin, E_POINTER);
	HRESULT hr = S_OK;
	*ppPin = DNew CBaseMuxerInputPin(name, this, this, &hr);
	return hr;
}

HRESULT CBaseMuxerFilter::CreateRawOutput(CStringW name, CBaseMuxerRawOutputPin** ppPin)
{
	CheckPointer(ppPin, E_POINTER);
	HRESULT hr = S_OK;
	*ppPin = DNew CBaseMuxerRawOutputPin(name, this, this, &hr);
	return hr;
}

//

DWORD CBaseMuxerFilter::ThreadProc()
{
	SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

	POSITION pos;

	for (;;) {
		DWORD cmd = GetRequest();

		switch (cmd) {
			default:
			case CMD_EXIT:
				CAMThread::m_hThread = NULL;
				Reply(S_OK);
				return 0;

			case CMD_RUN:
				m_pActivePins.RemoveAll();
				m_pPins.RemoveAll();

				pos = m_pInputs.GetHeadPosition();
				while (pos) {
					CBaseMuxerInputPin* pPin = m_pInputs.GetNext(pos);
					if (pPin->IsConnected()) {
						m_pActivePins.AddTail(pPin);
						m_pPins.AddTail(pPin);
					}
				}

				m_rtCurrent = 0;

				Reply(S_OK);

				MuxInit();

				try {
					MuxHeaderInternal();

					while (!CheckRequest(NULL) && m_pActivePins.GetCount()) {
						if (m_State == State_Paused) {
							Sleep(10);
							continue;
						}

						CAutoPtr<MuxerPacket> pPacket = GetPacket();
						if (!pPacket) {
							Sleep(1);
							continue;
						}

						if (pPacket->IsTimeValid()) {
							m_rtCurrent = pPacket->rtStart;
						}

						if (pPacket->IsEOS()) {
							m_pActivePins.RemoveAt(m_pActivePins.Find(pPacket->pPin));
						}

						MuxPacketInternal(pPacket);
					}

					MuxFooterInternal();
				} catch (HRESULT hr) {
					CComQIPtr<IMediaEventSink>(m_pGraph)->Notify(EC_ERRORABORT, hr, 0);
				}

				m_pOutput->DeliverEndOfStream();

				pos = m_pRawOutputs.GetHeadPosition();
				while (pos) {
					m_pRawOutputs.GetNext(pos)->DeliverEndOfStream();
				}

				m_pActivePins.RemoveAll();
				m_pPins.RemoveAll();

				break;
		}
	}

	ASSERT(0); // this function should only return via CMD_EXIT

	CAMThread::m_hThread = NULL;
	return 0;
}

void CBaseMuxerFilter::MuxHeaderInternal()
{
	TRACE(_T("MuxHeader\n"));

	if (CComQIPtr<IBitStream> pBitStream = m_pOutput->GetBitStream()) {
		MuxHeader(pBitStream);
	}

	MuxHeader();

	//

	POSITION pos = m_pPins.GetHeadPosition();
	while (pos) {
		if (CBaseMuxerInputPin* pInput = m_pPins.GetNext(pos))
			if (CBaseMuxerRawOutputPin* pOutput = dynamic_cast<CBaseMuxerRawOutputPin*>(pInput->GetRelatedPin())) {
				pOutput->MuxHeader(pInput->CurrentMediaType());
			}
	}
}

void CBaseMuxerFilter::MuxPacketInternal(const MuxerPacket* pPacket)
{
	TRACE(_T("MuxPacket pPin=%x, size=%d, s%d e%d b%d, rt=(%I64d-%I64d)\n"),
		  pPacket->pPin->GetID(),
		  pPacket->pData.GetCount(),
		  !!(pPacket->flags & MuxerPacket::syncpoint),
		  !!(pPacket->flags & MuxerPacket::eos),
		  !!(pPacket->flags & MuxerPacket::bogus),
		  pPacket->rtStart/10000, pPacket->rtStop/10000);

	if (CComQIPtr<IBitStream> pBitStream = m_pOutput->GetBitStream()) {
		MuxPacket(pBitStream, pPacket);
	}

	MuxPacket(pPacket);

	if (CBaseMuxerInputPin* pInput = pPacket->pPin)
		if (CBaseMuxerRawOutputPin* pOutput = dynamic_cast<CBaseMuxerRawOutputPin*>(pInput->GetRelatedPin())) {
			pOutput->MuxPacket(pInput->CurrentMediaType(), pPacket);
		}
}

void CBaseMuxerFilter::MuxFooterInternal()
{
	TRACE(_T("MuxFooter\n"));

	if (CComQIPtr<IBitStream> pBitStream = m_pOutput->GetBitStream()) {
		MuxFooter(pBitStream);
	}

	MuxFooter();

	//

	POSITION pos = m_pPins.GetHeadPosition();
	while (pos) {
		if (CBaseMuxerInputPin* pInput = m_pPins.GetNext(pos))
			if (CBaseMuxerRawOutputPin* pOutput = dynamic_cast<CBaseMuxerRawOutputPin*>(pInput->GetRelatedPin())) {
				pOutput->MuxFooter(pInput->CurrentMediaType());
			}
	}
}

CAutoPtr<MuxerPacket> CBaseMuxerFilter::GetPacket()
{
	REFERENCE_TIME rtMin = _I64_MAX;
	CBaseMuxerInputPin* pPinMin = NULL;
	int i = int(m_pActivePins.GetCount());

	POSITION pos = m_pActivePins.GetHeadPosition();
	while (pos) {
		CBaseMuxerInputPin* pPin = m_pActivePins.GetNext(pos);

		CAutoLock cAutoLock(&pPin->m_csQueue);
		if (!pPin->m_queue.GetCount()) {
			continue;
		}

		MuxerPacket* p = pPin->m_queue.GetHead();

		if (p->IsBogus() || !p->IsTimeValid() || p->IsEOS()) {
			pPinMin = pPin;
			i = 0;
			break;
		}

		if (p->rtStart < rtMin) {
			rtMin = p->rtStart;
			pPinMin = pPin;
		}

		i--;
	}

	CAutoPtr<MuxerPacket> pPacket;

	if (pPinMin && i == 0) {
		pPacket = pPinMin->PopPacket();
	} else {
		pos = m_pActivePins.GetHeadPosition();
		while (pos) {
			m_pActivePins.GetNext(pos)->m_evAcceptPacket.Set();
		}
	}

	return pPacket;
}

//

int CBaseMuxerFilter::GetPinCount()
{
	return int(m_pInputs.GetCount()) + (m_pOutput ? 1 : 0) + int(m_pRawOutputs.GetCount());
}

CBasePin* CBaseMuxerFilter::GetPin(int n)
{
	CAutoLock cAutoLock(this);

	if (n >= 0 && n < (int)m_pInputs.GetCount()) {
		if (POSITION pos = m_pInputs.FindIndex(n)) {
			return m_pInputs.GetAt(pos);
		}
	}

	n -= int(m_pInputs.GetCount());

	if (n == 0 && m_pOutput) {
		return m_pOutput;
	}

	n--;

	if (n >= 0 && n < (int)m_pRawOutputs.GetCount()) {
		if (POSITION pos = m_pRawOutputs.FindIndex(n)) {
			return m_pRawOutputs.GetAt(pos);
		}
	}

	n -= int(m_pRawOutputs.GetCount());

	return NULL;
}

STDMETHODIMP CBaseMuxerFilter::Stop()
{
	CAutoLock cAutoLock(this);

	HRESULT hr = __super::Stop();
	if (FAILED(hr)) {
		return hr;
	}

	CallWorker(CMD_EXIT);

	return hr;
}

STDMETHODIMP CBaseMuxerFilter::Pause()
{
	CAutoLock cAutoLock(this);

	FILTER_STATE fs = m_State;

	HRESULT hr = __super::Pause();
	if (FAILED(hr)) {
		return hr;
	}

	if (fs == State_Stopped && m_pOutput) {
		CAMThread::Create();
		CallWorker(CMD_RUN);
	}

	return hr;
}

STDMETHODIMP CBaseMuxerFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cAutoLock(this);

	HRESULT hr = __super::Run(tStart);
	if (FAILED(hr)) {
		return hr;
	}

	return hr;
}

// IMediaSeeking

STDMETHODIMP CBaseMuxerFilter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities = AM_SEEKING_CanGetDuration|AM_SEEKING_CanGetCurrentPos, S_OK : E_POINTER;
}

STDMETHODIMP CBaseMuxerFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	if (*pCapabilities == 0) {
		return S_OK;
	}
	DWORD caps;
	GetCapabilities(&caps);
	caps &= *pCapabilities;
	return caps == 0 ? E_FAIL : caps == *pCapabilities ? S_OK : S_FALSE;
}

STDMETHODIMP CBaseMuxerFilter::IsFormatSupported(const GUID* pFormat)
{
	return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CBaseMuxerFilter::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CBaseMuxerFilter::GetTimeFormat(GUID* pFormat)
{
	return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;
}

STDMETHODIMP CBaseMuxerFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CBaseMuxerFilter::SetTimeFormat(const GUID* pFormat)
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CBaseMuxerFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	*pDuration = 0;
	POSITION pos = m_pInputs.GetHeadPosition();
	while (pos) {
		REFERENCE_TIME rt = m_pInputs.GetNext(pos)->GetDuration();
		if (rt > *pDuration) {
			*pDuration = rt;
		}
	}
	return S_OK;
}

STDMETHODIMP CBaseMuxerFilter::GetStopPosition(LONGLONG* pStop)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	CheckPointer(pCurrent, E_POINTER);
	*pCurrent = m_rtCurrent;
	return S_OK;
}

STDMETHODIMP CBaseMuxerFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	FILTER_STATE fs;

	if (SUCCEEDED(GetState(0, &fs)) && fs == State_Stopped) {
		POSITION pos = m_pInputs.GetHeadPosition();
		while (pos) {
			CBasePin* pPin = m_pInputs.GetNext(pos);
			CComQIPtr<IMediaSeeking> pMS = pPin->GetConnected();
			if (!pMS) {
				pMS = GetFilterFromPin(pPin->GetConnected());
			}
			if (pMS) {
				pMS->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
			}
		}

		return S_OK;
	}

	return VFW_E_WRONG_STATE;
}

STDMETHODIMP CBaseMuxerFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::SetRate(double dRate)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::GetRate(double* pdRate)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::GetPreroll(LONGLONG* pllPreroll)
{
	return E_NOTIMPL;
}

//
// CBitStream
//

CBitStream::CBitStream(IStream* pStream, bool fThrowError)
	: CUnknown(_T("CBitStream"), NULL)
	, m_pStream(pStream)
	, m_fThrowError(fThrowError)
	, m_bitbuff(0)
	, m_bitlen(0)
{
	ASSERT(m_pStream);

	LARGE_INTEGER li = {0};
	m_pStream->Seek(li, STREAM_SEEK_SET, NULL);

	ULARGE_INTEGER uli = {0};
	m_pStream->SetSize(uli); // not that it worked...

	m_pStream->Commit(S_OK); // also seems to have no effect, but maybe in the future...
}

CBitStream::~CBitStream()
{
	BitFlush();
}

STDMETHODIMP CBitStream::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return
		QI(IBitStream)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IBitStream

STDMETHODIMP_(UINT64) CBitStream::GetPos()
{
	ULARGE_INTEGER pos = {0, 0};
	m_pStream->Seek(*(LARGE_INTEGER*)&pos, STREAM_SEEK_CUR, &pos);
	return pos.QuadPart;
}

STDMETHODIMP_(UINT64) CBitStream::Seek(UINT64 pos)
{
	BitFlush();

	LARGE_INTEGER li;
	li.QuadPart = pos;
	ULARGE_INTEGER linew;
	linew.QuadPart = (ULONGLONG)-1;
	m_pStream->Seek(li, STREAM_SEEK_SET, &linew);
	ASSERT(li.QuadPart == (LONGLONG)linew.QuadPart);
	return linew.QuadPart;
}

STDMETHODIMP CBitStream::ByteWrite(const void* pData, int len)
{
	HRESULT hr = S_OK;

	BitFlush();

	if (len > 0) {
		ULONG cbWritten = 0;
		hr = m_pStream->Write(pData, len, &cbWritten);

		ASSERT(SUCCEEDED(hr));
		if (m_fThrowError && FAILED(hr)) {
			throw hr;
		}
	}

	return hr;
}

STDMETHODIMP CBitStream::BitWrite(UINT64 data, int len)
{
	HRESULT hr = S_OK;

	ASSERT(len >= 0 && len <= 64);

	if (len > 56) {
		BitWrite(data >> 56, len - 56);
		len = 56;
	}

	m_bitbuff <<= len;
	m_bitbuff |= data & ((1ui64 << len) - 1);
	m_bitlen += len;

	while (m_bitlen >= 8) {
		BYTE b = (BYTE)(m_bitbuff >> (m_bitlen - 8));
		hr = m_pStream->Write(&b, 1, NULL);
		m_bitlen -= 8;

		ASSERT(SUCCEEDED(hr));
		if (m_fThrowError && FAILED(hr)) {
			throw E_FAIL;
		}
	}

	return hr;
}

STDMETHODIMP CBitStream::BitFlush()
{
	HRESULT hr = S_OK;

	if (m_bitlen > 0) {
		ASSERT(m_bitlen < 8);
		BYTE b = (BYTE)(m_bitbuff << (8 - m_bitlen));
		hr = m_pStream->Write(&b, 1, NULL);
		m_bitlen = 0;

		ASSERT(SUCCEEDED(hr));
		if (m_fThrowError && FAILED(hr)) {
			throw E_FAIL;
		}
	}

	return hr;
}

STDMETHODIMP CBitStream::StrWrite(LPCSTR pData, BOOL bFixNewLine)
{
	CStringA str = pData;

	if (bFixNewLine) {
		str.Replace("\r", "");
		str.Replace("\n", "\r\n");
	}

	return ByteWrite((LPCSTR)str, str.GetLength());
}

//
// CBaseMuxerRelatedPin
//

CBaseMuxerRelatedPin::CBaseMuxerRelatedPin()
	: m_pRelatedPin(NULL)
{
}

CBaseMuxerRelatedPin::~CBaseMuxerRelatedPin()
{
}

// IBaseMuxerRelatedPin

STDMETHODIMP CBaseMuxerRelatedPin::SetRelatedPin(CBasePin* pPin)
{
	m_pRelatedPin = pPin;
	return S_OK;
}

STDMETHODIMP_(CBasePin*) CBaseMuxerRelatedPin::GetRelatedPin()
{
	return m_pRelatedPin;
}

//
// CBaseMuxerInputPin
//

CBaseMuxerInputPin::CBaseMuxerInputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseInputPin(NAME("CBaseMuxerInputPin"), pFilter, pLock, phr, pName)
	, m_rtDuration(0)
	, m_evAcceptPacket(TRUE)
	, m_fEOS(false)
	, m_iPacketIndex(0)
{
	static int s_iID = 0;
	m_iID = s_iID++;
}

CBaseMuxerInputPin::~CBaseMuxerInputPin()
{
}

STDMETHODIMP CBaseMuxerInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IBaseMuxerRelatedPin)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

bool CBaseMuxerInputPin::IsSubtitleStream()
{
	return m_mt.majortype == MEDIATYPE_Subtitle || m_mt.majortype == MEDIATYPE_Text;
}

void CBaseMuxerInputPin::PushPacket(CAutoPtr<MuxerPacket> pPacket)
{
	for (int i = 0; m_pFilter->IsActive() && !m_bFlushing
			&& !m_evAcceptPacket.Wait(1)
			&& i < 1000;
			i++) {
		;
	}

	if (!m_pFilter->IsActive() || m_bFlushing) {
		return;
	}

	CAutoLock cAutoLock(&m_csQueue);

	m_queue.AddTail(pPacket);

	if (m_queue.GetCount() >= MAXQUEUESIZE) {
		m_evAcceptPacket.Reset();
	}
}

CAutoPtr<MuxerPacket> CBaseMuxerInputPin::PopPacket()
{
	CAutoPtr<MuxerPacket> pPacket;

	CAutoLock cAutoLock(&m_csQueue);

	if (m_queue.GetCount()) {
		pPacket = m_queue.RemoveHead();
	}

	if (m_queue.GetCount() < MAXQUEUESIZE) {
		m_evAcceptPacket.Set();
	}

	return pPacket;
}

HRESULT CBaseMuxerInputPin::CheckMediaType(const CMediaType* pmt)
{
	if (pmt->formattype == FORMAT_WaveFormatEx) {
		WORD wFormatTag = ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag;
		if ((wFormatTag == WAVE_FORMAT_PCM
				|| wFormatTag == WAVE_FORMAT_EXTENSIBLE
				|| wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
				&& pmt->subtype != FOURCCMap(wFormatTag)
				&& !(pmt->subtype == MEDIASUBTYPE_PCM && wFormatTag == WAVE_FORMAT_EXTENSIBLE)
				&& !(pmt->subtype == MEDIASUBTYPE_PCM && wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
				&& pmt->subtype != MEDIASUBTYPE_DVD_LPCM_AUDIO
				&& pmt->subtype != MEDIASUBTYPE_DOLBY_AC3
				&& pmt->subtype != MEDIASUBTYPE_DTS) {
			return E_INVALIDARG;
		}
	}

	return pmt->majortype == MEDIATYPE_Video
		   || pmt->majortype == MEDIATYPE_Audio && pmt->formattype != FORMAT_VorbisFormat
		   || pmt->majortype == MEDIATYPE_Text && pmt->subtype == MEDIASUBTYPE_NULL && pmt->formattype == FORMAT_None
		   || pmt->majortype == MEDIATYPE_Subtitle
		   ? S_OK
		   : E_INVALIDARG;
}

HRESULT CBaseMuxerInputPin::BreakConnect()
{
	HRESULT hr = __super::BreakConnect();
	if (FAILED(hr)) {
		return hr;
	}

	RemoveAll();

	// TODO: remove extra disconnected pins, leave one

	return hr;
}

HRESULT CBaseMuxerInputPin::CompleteConnect(IPin* pReceivePin)
{
	HRESULT hr = __super::CompleteConnect(pReceivePin);
	if (FAILED(hr)) {
		return hr;
	}

	// duration

	m_rtDuration = 0;
	CComQIPtr<IMediaSeeking> pMS;
	if ((pMS = GetFilterFromPin(pReceivePin)) || (pMS = pReceivePin)) {
		pMS->GetDuration(&m_rtDuration);
	}

	// properties

	for (CComPtr<IPin> pPin = pReceivePin; pPin; pPin = GetUpStreamPin(GetFilterFromPin(pPin))) {
		if (CComQIPtr<IDSMPropertyBag> pPB = pPin) {
			ULONG cProperties = 0;
			if (SUCCEEDED(pPB->CountProperties(&cProperties)) && cProperties > 0) {
				for (ULONG iProperty = 0; iProperty < cProperties; iProperty++) {
					PROPBAG2 PropBag;
					memset(&PropBag, 0, sizeof(PropBag));
					ULONG cPropertiesReturned = 0;
					if (FAILED(pPB->GetPropertyInfo(iProperty, 1, &PropBag, &cPropertiesReturned))) {
						continue;
					}

					HRESULT hr;
					CComVariant var;
					if (SUCCEEDED(pPB->Read(1, &PropBag, NULL, &var, &hr)) && SUCCEEDED(hr)) {
						SetProperty(PropBag.pstrName, &var);
					}

					CoTaskMemFree(PropBag.pstrName);
				}
			}
		}
	}

	(static_cast<CBaseMuxerFilter*>(m_pFilter))->AddInput();

	return S_OK;
}

HRESULT CBaseMuxerInputPin::Active()
{
	m_rtMaxStart = INVALID_TIME;
	m_fEOS = false;
	m_iPacketIndex = 0;
	m_evAcceptPacket.Set();
	return __super::Active();
}

HRESULT CBaseMuxerInputPin::Inactive()
{
	CAutoLock cAutoLock(&m_csQueue);
	m_queue.RemoveAll();
	return __super::Inactive();
}

STDMETHODIMP CBaseMuxerInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	return __super::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CBaseMuxerInputPin::Receive(IMediaSample* pSample)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr = __super::Receive(pSample);
	if (FAILED(hr)) {
		return hr;
	}

	CAutoPtr<MuxerPacket> pPacket(DNew MuxerPacket(this));

	long len = pSample->GetActualDataLength();

	BYTE* pData = NULL;
	if (FAILED(pSample->GetPointer(&pData)) || !pData) {
		return S_OK;
	}

	pPacket->pData.SetCount(len);
	memcpy(pPacket->pData.GetData(), pData, len);

	if (S_OK == pSample->IsSyncPoint() || m_mt.majortype == MEDIATYPE_Audio && !m_mt.bTemporalCompression) {
		pPacket->flags |= MuxerPacket::syncpoint;
	}

	if (S_OK == pSample->GetTime(&pPacket->rtStart, &pPacket->rtStop)) {
		pPacket->flags |= MuxerPacket::timevalid;

		pPacket->rtStart += m_tStart;
		pPacket->rtStop += m_tStart;

		if ((pPacket->flags & MuxerPacket::syncpoint) && pPacket->rtStart < m_rtMaxStart) {
			pPacket->flags &= ~MuxerPacket::syncpoint;
			pPacket->flags |= MuxerPacket::bogus;
		}

		m_rtMaxStart = max(m_rtMaxStart,  pPacket->rtStart);
	} else if (pPacket->flags & MuxerPacket::syncpoint) {
		pPacket->flags &= ~MuxerPacket::syncpoint;
		pPacket->flags |= MuxerPacket::bogus;
	}

	if (S_OK == pSample->IsDiscontinuity()) {
		pPacket->flags |= MuxerPacket::discontinuity;
	}

	pPacket->index = m_iPacketIndex++;

	PushPacket(pPacket);

	return S_OK;
}

STDMETHODIMP CBaseMuxerInputPin::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr = __super::EndOfStream();
	if (FAILED(hr)) {
		return hr;
	}

	ASSERT(!m_fEOS);

	CAutoPtr<MuxerPacket> pPacket(DNew MuxerPacket(this));
	pPacket->flags |= MuxerPacket::eos;
	PushPacket(pPacket);

	m_fEOS = true;

	return hr;
}

//
// CBaseMuxerOutputPin
//

CBaseMuxerOutputPin::CBaseMuxerOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(_T("CBaseMuxerOutputPin"), pFilter, pLock, phr, pName)
{
}

IBitStream* CBaseMuxerOutputPin::GetBitStream()
{
	if (!m_pBitStream) {
		if (CComQIPtr<IStream> pStream = GetConnected()) {
			m_pBitStream = DNew CBitStream(pStream, true);
		}
	}

	return m_pBitStream;
}

HRESULT CBaseMuxerOutputPin::BreakConnect()
{
	m_pBitStream = NULL;

	return __super::BreakConnect();
}

HRESULT CBaseMuxerOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	ASSERT(pAlloc);
	ASSERT(pProperties);

	HRESULT hr = NOERROR;

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1;

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

HRESULT CBaseMuxerOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Stream && pmt->subtype == MEDIASUBTYPE_NULL
		   ? S_OK
		   : E_INVALIDARG;
}

HRESULT CBaseMuxerOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	CAutoLock cAutoLock(m_pLock);

	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}

	pmt->ResetFormatBuffer();
	pmt->InitMediaType();
	pmt->majortype = MEDIATYPE_Stream;
	pmt->subtype = MEDIASUBTYPE_NULL;
	pmt->formattype = FORMAT_None;

	return S_OK;
}

HRESULT CBaseMuxerOutputPin::DeliverEndOfStream()
{
	m_pBitStream = NULL;

	return __super::DeliverEndOfStream();
}

STDMETHODIMP CBaseMuxerOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	return E_NOTIMPL;
}

//
// CBaseMuxerRawOutputPin
//

CBaseMuxerRawOutputPin::CBaseMuxerRawOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseMuxerOutputPin(pName, pFilter, pLock, phr)
{
}

STDMETHODIMP CBaseMuxerRawOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IBaseMuxerRelatedPin)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CBaseMuxerRawOutputPin::MuxHeader(const CMediaType& mt)
{
	CComQIPtr<IBitStream> pBitStream = GetBitStream();
	if (!pBitStream) {
		return;
	}

	const BYTE utf8bom[3] = {0xef, 0xbb, 0xbf};

	if ((mt.subtype == FOURCCMap('1CVA') || mt.subtype == FOURCCMap('1cva')) && mt.formattype == FORMAT_MPEG2_VIDEO) {
		MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.Format();

		for (DWORD i = 0; i < vih->cbSequenceHeader-2; i += 2) {
			pBitStream->BitWrite(0x00000001, 32);
			WORD size = (((BYTE*)vih->dwSequenceHeader)[i+0]<<8) | ((BYTE*)vih->dwSequenceHeader)[i+1];
			pBitStream->ByteWrite(&((BYTE*)vih->dwSequenceHeader)[i+2], size);
			i += size;
		}
	} else if (mt.subtype == MEDIASUBTYPE_UTF8) {
		pBitStream->ByteWrite(utf8bom, sizeof(utf8bom));
	} else if (mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2) {
		SUBTITLEINFO* si = (SUBTITLEINFO*)mt.Format();
		BYTE* p = (BYTE*)si + si->dwOffset;

		if (memcmp(utf8bom, p, 3) != 0) {
			pBitStream->ByteWrite(utf8bom, sizeof(utf8bom));
		}

		CStringA str((char*)p, mt.FormatLength() - (p - mt.Format()));
		pBitStream->StrWrite(str + '\n', true);

		if (str.Find("[Events]") < 0) {
			pBitStream->StrWrite("\n\n[Events]\n", true);
		}
	} else if (mt.subtype == MEDIASUBTYPE_VOBSUB) {
		m_idx.RemoveAll();
	} else if (mt.majortype == MEDIATYPE_Audio
			   && (mt.subtype == MEDIASUBTYPE_PCM
				   || mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
				   || mt.subtype == FOURCCMap(WAVE_FORMAT_EXTENSIBLE)
				   || mt.subtype == FOURCCMap(WAVE_FORMAT_IEEE_FLOAT))
			   && mt.formattype == FORMAT_WaveFormatEx) {
		pBitStream->BitWrite('RIFF', 32);
		pBitStream->BitWrite(0, 32); // file length - 8, set later
		pBitStream->BitWrite('WAVE', 32);

		pBitStream->BitWrite('fmt ', 32);
		pBitStream->ByteWrite(&mt.cbFormat, 4);
		pBitStream->ByteWrite(mt.pbFormat, mt.cbFormat);

		pBitStream->BitWrite('data', 32);
		pBitStream->BitWrite(0, 32); // data length, set later
	}
}

void CBaseMuxerRawOutputPin::MuxPacket(const CMediaType& mt, const MuxerPacket* pPacket)
{
	CComQIPtr<IBitStream> pBitStream = GetBitStream();
	if (!pBitStream) {
		return;
	}

	const BYTE* pData = pPacket->pData.GetData();
	const int DataSize = int(pPacket->pData.GetCount());

	if (mt.subtype == MEDIASUBTYPE_RAW_AAC1 && mt.formattype == FORMAT_WaveFormatEx) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

		int profile = 0;

		int srate_idx = 11;
		if (92017 <= wfe->nSamplesPerSec) {
			srate_idx = 0;
		} else if (75132 <= wfe->nSamplesPerSec) {
			srate_idx = 1;
		} else if (55426 <= wfe->nSamplesPerSec) {
			srate_idx = 2;
		} else if (46009 <= wfe->nSamplesPerSec) {
			srate_idx = 3;
		} else if (37566 <= wfe->nSamplesPerSec) {
			srate_idx = 4;
		} else if (27713 <= wfe->nSamplesPerSec) {
			srate_idx = 5;
		} else if (23004 <= wfe->nSamplesPerSec) {
			srate_idx = 6;
		} else if (18783 <= wfe->nSamplesPerSec) {
			srate_idx = 7;
		} else if (13856 <= wfe->nSamplesPerSec) {
			srate_idx = 8;
		} else if (11502 <= wfe->nSamplesPerSec) {
			srate_idx = 9;
		} else if (9391 <= wfe->nSamplesPerSec) {
			srate_idx = 10;
		}

		int channels = wfe->nChannels;

		if (wfe->cbSize >= 2) {
			BYTE* p = (BYTE*)(wfe+1);
			profile = (p[0]>>3)-1;
			srate_idx = ((p[0]&7)<<1)|((p[1]&0x80)>>7);
			channels = (p[1]>>3)&15;
		}

		int len = (DataSize + 7) & 0x1fff;

		BYTE hdr[7] = {0xff, 0xf9};
		hdr[2] = (profile<<6) | (srate_idx<<2) | ((channels&4)>>2);
		hdr[3] = ((channels&3)<<6) | (len>>11);
		hdr[4] = (len>>3)&0xff;
		hdr[5] = ((len&7)<<5) | 0x1f;
		hdr[6] = 0xfc;

		pBitStream->ByteWrite(hdr, sizeof(hdr));
	} else if ((mt.subtype == FOURCCMap('1CVA') || mt.subtype == FOURCCMap('1cva')) && mt.formattype == FORMAT_MPEG2_VIDEO) {
		const BYTE* p = pData;
		int i = DataSize;

		while (i >= 4) {
			DWORD len = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];

			i -= len + 4;
			p += len + 4;
		}

		if (i == 0) {
			p = pData;
			i = DataSize;

			while (i >= 4) {
				DWORD len = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];

				pBitStream->BitWrite(0x00000001, 32);

				p += 4;
				i -= 4;

				if (len > (DWORD)i || len == 1) {
					len = i;
					ASSERT(0);
				}

				pBitStream->ByteWrite(p, len);

				p += len;
				i -= len;
			}

			return;
		}
	} else if (mt.subtype == MEDIASUBTYPE_UTF8 || mt.majortype == MEDIATYPE_Text) {
		CStringA str((char*)pData, DataSize);
		str.Trim();
		if (str.IsEmpty()) {
			return;
		}

		DVD_HMSF_TIMECODE start = RT2HMSF(pPacket->rtStart, 25);
		DVD_HMSF_TIMECODE stop = RT2HMSF(pPacket->rtStop, 25);

		str.Format("%d\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\n%s\n\n",
				   pPacket->index+1,
				   start.bHours, start.bMinutes, start.bSeconds, (int)((pPacket->rtStart/10000)%1000),
				   stop.bHours, stop.bMinutes, stop.bSeconds, (int)((pPacket->rtStop/10000)%1000),
				   CStringA(str));

		pBitStream->StrWrite(str, true);

		return;
	} else if (mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2) {
		CStringA str((char*)pData, DataSize);
		str.Trim();
		if (str.IsEmpty()) {
			return;
		}

		DVD_HMSF_TIMECODE start = RT2HMSF(pPacket->rtStart, 25);
		DVD_HMSF_TIMECODE stop = RT2HMSF(pPacket->rtStop, 25);

		size_t fields = mt.subtype == MEDIASUBTYPE_ASS2 ? 10 : 9;

		CAtlList<CStringA> sl;
		Explode(str, sl, ',', fields);
		if (sl.GetCount() < fields) {
			return;
		}

		CStringA readorder = sl.RemoveHead(); // TODO
		CStringA layer = sl.RemoveHead();
		CStringA style = sl.RemoveHead();
		CStringA actor = sl.RemoveHead();
		CStringA left = sl.RemoveHead();
		CStringA right = sl.RemoveHead();
		CStringA top = sl.RemoveHead();
		if (fields == 10) {
			top += ',' + sl.RemoveHead();    // bottom
		}
		CStringA effect = sl.RemoveHead();
		str = sl.RemoveHead();

		if (mt.subtype == MEDIASUBTYPE_SSA) {
			layer = "Marked=0";
		}

		str.Format("Dialogue: %s,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s,%s,%s,%s,%s,%s,%s\n",
				   layer,
				   start.bHours, start.bMinutes, start.bSeconds, (int)((pPacket->rtStart/100000)%100),
				   stop.bHours, stop.bMinutes, stop.bSeconds, (int)((pPacket->rtStop/100000)%100),
				   style, actor, left, right, top, effect,
				   CStringA(str));

		pBitStream->StrWrite(str, true);

		return;
	} else if (mt.subtype == MEDIASUBTYPE_VOBSUB) {
		bool fTimeValid = pPacket->IsTimeValid();

		if (fTimeValid) {
			idx_t i;
			i.rt = pPacket->rtStart;
			i.fp = pBitStream->GetPos();
			m_idx.AddTail(i);
		}

		int DataSizeLeft = DataSize;

		while (DataSizeLeft > 0) {
			int BytesAvail = 0x7ec - (fTimeValid ? 9 : 4);
			int Size = min(BytesAvail, DataSizeLeft);
			int Padding = 0x800 - Size - 20 - (fTimeValid ? 9 : 4);

			pBitStream->BitWrite(0x000001ba, 32);
			pBitStream->BitWrite(0x440004000401ui64, 48);
			pBitStream->BitWrite(0x000003f8, 32);
			pBitStream->BitWrite(0x000001bd, 32);

			if (fTimeValid) {
				pBitStream->BitWrite(Size+9, 16);
				pBitStream->BitWrite(0x8180052100010001ui64, 64);
			} else {
				pBitStream->BitWrite(Size+4, 16);
				pBitStream->BitWrite(0x810000, 24);
			}

			pBitStream->BitWrite(0x20, 8);

			pBitStream->ByteWrite(pData, Size);

			pData += Size;
			DataSizeLeft -= Size;

			if (Padding > 0) {
				Padding -= 6;
				ASSERT(Padding >= 0);
				pBitStream->BitWrite(0x000001be, 32);
				pBitStream->BitWrite(Padding, 16);
				while (Padding-- > 0) {
					pBitStream->BitWrite(0xff, 8);
				}
			}

			fTimeValid = false;
		}

		return;
	} else if (mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

		// This code is probably totally broken for anything but 16 bits
		for (int i = 0, bps = wfe->wBitsPerSample/8; i < DataSize; i += bps)
			for (int j = bps-1; j >= 0; j--) {
				pBitStream->BitWrite(pData[i+j], 8);
			}

		return;
	}
	// else // TODO: restore more streams (vorbis to ogg)

	pBitStream->ByteWrite(pData, DataSize);
}

void CBaseMuxerRawOutputPin::MuxFooter(const CMediaType& mt)
{
	CComQIPtr<IBitStream> pBitStream = GetBitStream();
	if (!pBitStream) {
		return;
	}

	if (mt.majortype == MEDIATYPE_Audio
			&& (mt.subtype == MEDIASUBTYPE_PCM
				|| mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
				|| mt.subtype == FOURCCMap(WAVE_FORMAT_EXTENSIBLE)
				|| mt.subtype == FOURCCMap(WAVE_FORMAT_IEEE_FLOAT))
			&& mt.formattype == FORMAT_WaveFormatEx) {
		pBitStream->BitFlush();

		ASSERT(pBitStream->GetPos() <= 0xffffffff);
		UINT32 size = (UINT32)pBitStream->GetPos();

		size -= 8;
		pBitStream->Seek(4);
		pBitStream->ByteWrite(&size, 4);

		size -= sizeof(RIFFLIST) + sizeof(RIFFCHUNK) + mt.FormatLength();
		pBitStream->Seek(sizeof(RIFFLIST) + sizeof(RIFFCHUNK) + mt.FormatLength() + 4);
		pBitStream->ByteWrite(&size, 4);
	} else if (mt.subtype == MEDIASUBTYPE_VOBSUB) {
		if (CComQIPtr<IFileSinkFilter> pFSF = GetFilterFromPin(GetConnected())) {
			WCHAR* fn = NULL;
			if (SUCCEEDED(pFSF->GetCurFile(&fn, NULL))) {
				CPathW p(fn);
				p.RenameExtension(L".idx");
				CoTaskMemFree(fn);

				FILE* f;
				if (!_tfopen_s(&f, CString((LPCWSTR)p), _T("w"))) {
					SUBTITLEINFO* si = (SUBTITLEINFO*)mt.Format();

					_ftprintf_s(f, _T("%s\n"), _T("# VobSub index file, v7 (do not modify this line!)"));

					fwrite(mt.Format() + si->dwOffset, mt.FormatLength() - si->dwOffset, 1, f);

					CString iso6391 = ISO6392To6391(si->IsoLang);
					if (iso6391.IsEmpty()) {
						iso6391 = _T("--");
					}
					_ftprintf_s(f, _T("\nlangidx: 0\n\nid: %s, index: 0\n"), iso6391);

					CString alt = CString(CStringW(si->TrackName));
					if (!alt.IsEmpty()) {
						_ftprintf_s(f, _T("alt: %s\n"), alt);
					}

					POSITION pos = m_idx.GetHeadPosition();
					while (pos) {
						const idx_t& i = m_idx.GetNext(pos);
						DVD_HMSF_TIMECODE start = RT2HMSF(i.rt, 25);
						_ftprintf_s(f, _T("timestamp: %02d:%02d:%02d:%03d, filepos: %09I64x\n"),
								  start.bHours, start.bMinutes, start.bSeconds, (int)((i.rt/10000)%1000),
								  i.fp);
					}

					fclose(f);
				}
			}
		}
	}
}
