/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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
#include "AudioTools/AudioHelper.h"
#include "DSUtil/DSUtil.h"
#include "StreamSwitcher.h"
#include <clsids.h>

#define BLOCKSTREAM

#define PauseGraph \
	CComQIPtr<IMediaControl> _pMC(m_pGraph); \
	OAFilterState _fs = -1; \
	if(_pMC) _pMC->GetState(1000, &_fs); \
	if(_fs == State_Running) \
		_pMC->Pause(); \
 \
	HRESULT _hr = E_FAIL; \
	CComQIPtr<IMediaSeeking> _pMS((IUnknown*)(INonDelegatingUnknown*)m_pGraph); \
	LONGLONG _rtNow = 0; \
	if(_pMS) _hr = _pMS->GetCurrentPosition(&_rtNow); \

#define ResumeGraph \
	if(SUCCEEDED(_hr) && _pMS && _fs != State_Stopped) \
		_hr = _pMS->SetPositions(&_rtNow, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning); \
 \
	if(_fs == State_Running && _pMS) \
		_pMC->Run(); \


//
// CStreamSwitcherPassThru
//

CStreamSwitcherPassThru::CStreamSwitcherPassThru(LPUNKNOWN pUnk, HRESULT* phr, CStreamSwitcherFilter* pFilter)
	: CMediaPosition(L"CStreamSwitcherPassThru", pUnk)
	, m_pFilter(pFilter)
{
}

STDMETHODIMP CStreamSwitcherPassThru::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);
	*ppv = nullptr;

	return
		QI(IMediaSeeking)
		CMediaPosition::NonDelegatingQueryInterface(riid, ppv);
}

template<class T>
HRESULT GetPeer(CStreamSwitcherFilter* pFilter, T** ppT)
{
	*ppT = nullptr;

	CBasePin* pPin = pFilter->GetInputPin();
	if (!pPin) {
		return E_NOTIMPL;
	}

	CComPtr<IPin> pConnected;
	if (FAILED(pPin->ConnectedTo(&pConnected))) {
		return E_NOTIMPL;
	}

	if (CComQIPtr<T> pT = pConnected.p) {
		*ppT = pT.Detach();
		return S_OK;
	}

	return E_NOTIMPL;
}

#define CallPeerSeeking(call) \
	CComPtr<IMediaSeeking> pMS; \
	if(FAILED(GetPeer(m_pFilter, &pMS))) return E_NOTIMPL; \
	return pMS->##call; \

#define CallPeer(call) \
	CComPtr<IMediaPosition> pMP; \
	if(FAILED(GetPeer(m_pFilter, &pMP))) return E_NOTIMPL; \
	return pMP->##call; \

#define CallPeerSeekingAll(call) \
	HRESULT hr = E_NOTIMPL; \
	for (const auto& pPin : m_pFilter->m_pInputs) \
	{ \
		CComPtr<IPin> pConnected; \
		if(FAILED(pPin->ConnectedTo(&pConnected))) \
			continue; \
		if(CComQIPtr<IMediaSeeking> pMS = pConnected.p) \
		{ \
			HRESULT hr2 = pMS->call; \
			if(pPin == m_pFilter->GetInputPin()) \
				hr = hr2; \
		} \
	} \
	return hr; \

#define CallPeerAll(call) \
	HRESULT hr = E_NOTIMPL; \
	for (const auto& pPin : m_pFilter->m_pInputs) \
	{ \
		CComPtr<IPin> pConnected; \
		if(FAILED(pPin->ConnectedTo(&pConnected))) \
			continue; \
		if(CComQIPtr<IMediaPosition> pMP = pConnected.p) \
		{ \
			HRESULT hr2 = pMP->call; \
			if(pPin == m_pFilter->GetInputPin()) \
				hr = hr2; \
		} \
	} \
	return hr; \


// IMediaSeeking

STDMETHODIMP CStreamSwitcherPassThru::GetCapabilities(DWORD* pCaps)
{
	CallPeerSeeking(GetCapabilities(pCaps));
}

STDMETHODIMP CStreamSwitcherPassThru::CheckCapabilities(DWORD* pCaps)
{
	CallPeerSeeking(CheckCapabilities(pCaps));
}

STDMETHODIMP CStreamSwitcherPassThru::IsFormatSupported(const GUID* pFormat)
{
	CallPeerSeeking(IsFormatSupported(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::QueryPreferredFormat(GUID* pFormat)
{
	CallPeerSeeking(QueryPreferredFormat(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::SetTimeFormat(const GUID* pFormat)
{
	CallPeerSeeking(SetTimeFormat(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::GetTimeFormat(GUID* pFormat)
{
	CallPeerSeeking(GetTimeFormat(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::IsUsingTimeFormat(const GUID* pFormat)
{
	CallPeerSeeking(IsUsingTimeFormat(pFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	CallPeerSeeking(ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat));
}

STDMETHODIMP CStreamSwitcherPassThru::SetPositions(LONGLONG* pCurrent, DWORD CurrentFlags, LONGLONG* pStop, DWORD StopFlags)
{
	CallPeerSeekingAll(SetPositions(pCurrent, CurrentFlags, pStop, StopFlags));
}

STDMETHODIMP CStreamSwitcherPassThru::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	CallPeerSeeking(GetPositions(pCurrent, pStop));
}

STDMETHODIMP CStreamSwitcherPassThru::GetCurrentPosition(LONGLONG* pCurrent)
{
	CallPeerSeeking(GetCurrentPosition(pCurrent));
}

STDMETHODIMP CStreamSwitcherPassThru::GetStopPosition(LONGLONG* pStop)
{
	CallPeerSeeking(GetStopPosition(pStop));
}

STDMETHODIMP CStreamSwitcherPassThru::GetDuration(LONGLONG* pDuration)
{
	CallPeerSeeking(GetDuration(pDuration));
}

STDMETHODIMP CStreamSwitcherPassThru::GetPreroll(LONGLONG* pllPreroll)
{
	CallPeerSeeking(GetPreroll(pllPreroll));
}

STDMETHODIMP CStreamSwitcherPassThru::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	CallPeerSeeking(GetAvailable(pEarliest, pLatest));
}

STDMETHODIMP CStreamSwitcherPassThru::GetRate(double* pdRate)
{
	CallPeerSeeking(GetRate(pdRate));
}

STDMETHODIMP CStreamSwitcherPassThru::SetRate(double dRate)
{
	if (0.0 == dRate) {
		return E_INVALIDARG;
	}
	CallPeerSeekingAll(SetRate(dRate));
}

// IMediaPosition

STDMETHODIMP CStreamSwitcherPassThru::get_Duration(REFTIME* plength)
{
	CallPeer(get_Duration(plength));
}

STDMETHODIMP CStreamSwitcherPassThru::get_CurrentPosition(REFTIME* pllTime)
{
	CallPeer(get_CurrentPosition(pllTime));
}

STDMETHODIMP CStreamSwitcherPassThru::put_CurrentPosition(REFTIME llTime)
{
	CallPeerAll(put_CurrentPosition(llTime));
}

STDMETHODIMP CStreamSwitcherPassThru::get_StopTime(REFTIME* pllTime)
{
	CallPeer(get_StopTime(pllTime));
}

STDMETHODIMP CStreamSwitcherPassThru::put_StopTime(REFTIME llTime)
{
	CallPeerAll(put_StopTime(llTime));
}

STDMETHODIMP CStreamSwitcherPassThru::get_PrerollTime(REFTIME * pllTime)
{
	CallPeer(get_PrerollTime(pllTime));
}

STDMETHODIMP CStreamSwitcherPassThru::put_PrerollTime(REFTIME llTime)
{
	CallPeerAll(put_PrerollTime(llTime));
}

STDMETHODIMP CStreamSwitcherPassThru::get_Rate(double* pdRate)
{
	CallPeer(get_Rate(pdRate));
}

STDMETHODIMP CStreamSwitcherPassThru::put_Rate(double dRate)
{
	if (0.0 == dRate) {
		return E_INVALIDARG;
	}
	CallPeerAll(put_Rate(dRate));
}

STDMETHODIMP CStreamSwitcherPassThru::CanSeekForward(LONG* pCanSeekForward)
{
	CallPeer(CanSeekForward(pCanSeekForward));
}

STDMETHODIMP CStreamSwitcherPassThru::CanSeekBackward(LONG* pCanSeekBackward)
{
	CallPeer(CanSeekBackward(pCanSeekBackward));
}

//
// CStreamSwitcherAllocator
//

CStreamSwitcherAllocator::CStreamSwitcherAllocator(CStreamSwitcherInputPin* pPin, HRESULT* phr)
	: CMemAllocator(L"CStreamSwitcherAllocator", nullptr, phr)
	, m_pPin(pPin)
	, m_fMediaTypeChanged(false)
{
	ASSERT(phr);
	ASSERT(pPin);
}

#ifdef _DEBUG
CStreamSwitcherAllocator::~CStreamSwitcherAllocator()
{
	ASSERT(m_bCommitted == FALSE);
}
#endif

STDMETHODIMP_(ULONG) CStreamSwitcherAllocator::NonDelegatingAddRef()
{
	return m_pPin->m_pFilter->AddRef();
}

STDMETHODIMP_(ULONG) CStreamSwitcherAllocator::NonDelegatingRelease()
{
	return m_pPin->m_pFilter->Release();
}

STDMETHODIMP CStreamSwitcherAllocator::GetBuffer(
	IMediaSample** ppBuffer,
	REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime,
	DWORD dwFlags)
{
	HRESULT hr = VFW_E_NOT_COMMITTED;

	if (!m_bCommitted) {
		return hr;
	}

	if (m_fMediaTypeChanged) {
		if (!m_pPin || !m_pPin->m_pFilter) {
			return hr;
		}

		CStreamSwitcherOutputPin* pOut = (static_cast<CStreamSwitcherFilter*>(m_pPin->m_pFilter))->GetOutputPin();
		if (!pOut || !pOut->CurrentAllocator()) {
			return hr;
		}

		ALLOCATOR_PROPERTIES Properties, Actual;
		if (FAILED(pOut->CurrentAllocator()->GetProperties(&Actual))) {
			return hr;
		}
		if (FAILED(GetProperties(&Properties))) {
			return hr;
		}

		if (!m_bCommitted || Properties.cbBuffer < Actual.cbBuffer) {
			Properties.cbBuffer = Actual.cbBuffer;
			if (FAILED(Decommit())) {
				return hr;
			}
			if (FAILED(SetProperties(&Properties, &Actual))) {
				return hr;
			}
			if (FAILED(Commit())) {
				return hr;
			}
			ASSERT(Actual.cbBuffer >= Properties.cbBuffer);
			if (Actual.cbBuffer < Properties.cbBuffer) {
				return hr;
			}
		}
	}

	hr = CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);

	if (m_fMediaTypeChanged && SUCCEEDED(hr)) {
		(*ppBuffer)->SetMediaType(&m_mt);
		m_fMediaTypeChanged = false;
	}

	return hr;
}

void CStreamSwitcherAllocator::NotifyMediaType(const CMediaType& mt)
{
	CopyMediaType(&m_mt, &mt);
	m_fMediaTypeChanged = true;
}

//
// CStreamSwitcherInputPin
//

CStreamSwitcherInputPin::CStreamSwitcherInputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr, LPCWSTR pName)
	: CBaseInputPin(L"CStreamSwitcherInputPin", pFilter, &pFilter->m_csState, phr, pName)
	, m_Allocator(this, phr)
	, m_bSampleSkipped(FALSE)
	, m_bQualityChanged(FALSE)
	, m_bUsingOwnAllocator(FALSE)
	, m_evBlock(TRUE)
	, m_bCanBlock(false)
	, m_hNotifyEvent(nullptr)
	, m_pSSF(static_cast<CStreamSwitcherFilter*>(m_pFilter))
{
	m_bCanReconnectWhenActive = true;
}

class __declspec(uuid("138130AF-A79B-45D5-B4AA-87697457BA87"))
		NeroAudioDecoder {};

STDMETHODIMP CStreamSwitcherInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IStreamSwitcherInputPin)
		IsConnected() && GetCLSID(GetFilterFromPin(GetConnected())) == __uuidof(NeroAudioDecoder) && QI(IPinConnection)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IPinConnection

STDMETHODIMP CStreamSwitcherInputPin::DynamicQueryAccept(const AM_MEDIA_TYPE* pmt)
{
	return QueryAccept(pmt);
}

STDMETHODIMP CStreamSwitcherInputPin::NotifyEndOfStream(HANDLE hNotifyEvent)
{
	if (m_hNotifyEvent) {
		SetEvent(m_hNotifyEvent);
	}
	m_hNotifyEvent = hNotifyEvent;
	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::IsEndPin()
{
	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::DynamicDisconnect()
{
	CAutoLock cAutoLock(&m_csReceive);
	Disconnect();
	return S_OK;
}

// IStreamSwitcherInputPin

STDMETHODIMP_(bool) CStreamSwitcherInputPin::IsActive()
{
	// TODO: lock onto something here
	return(this == m_pSSF->GetInputPin());
}

//

HRESULT CStreamSwitcherInputPin::QueryAcceptDownstream(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = S_OK;

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	if (pOut && pOut->IsConnected()) {
		CMediaType tmp(*pmt);
		(static_cast<CStreamSwitcherFilter*>(m_pFilter))->TransformMediaType(tmp);

		if (CComPtr<IPinConnection> pPC = pOut->CurrentPinConnection()) {
			hr = pPC->DynamicQueryAccept(&tmp);
			if (hr == S_OK) {
				return S_OK;
			}
		}

		hr = pOut->GetConnected()->QueryAccept(&tmp);
	}

	return hr;
}

void CStreamSwitcherInputPin::Block(const bool bBlock)
{
	if (bBlock) {
		m_evBlock.Reset();
	}
	else {
		m_evBlock.Set();
	}
}

HRESULT CStreamSwitcherInputPin::InitializeOutputSample(IMediaSample* pInSample, IMediaSample** ppOutSample)
{
	CheckPointer(ppOutSample, E_POINTER);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	ASSERT(pOut->GetConnected());

	CComPtr<IMediaSample> pOutSample;

	DWORD dwFlags = m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0;

	if (!(m_SampleProps.dwSampleFlags & AM_SAMPLE_SPLICEPOINT)) {
		dwFlags |= AM_GBF_NOTASYNCPOINT;
	}

	HRESULT hr = pOut->GetDeliveryBuffer(&pOutSample
										 , m_SampleProps.dwSampleFlags & AM_SAMPLE_TIMEVALID ? &m_SampleProps.tStart : nullptr
										 , m_SampleProps.dwSampleFlags & AM_SAMPLE_STOPVALID ? &m_SampleProps.tStop : nullptr
										 , dwFlags);

	if (FAILED(hr)) {
		return hr;
	}

	if (!pOutSample) {
		return E_FAIL;
	}

	if (CComQIPtr<IMediaSample2> pOutSample2 = pOutSample.p) {
		AM_SAMPLE2_PROPERTIES OutProps;
		EXECUTE_ASSERT(SUCCEEDED(pOutSample2->GetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, tStart), (PBYTE)&OutProps)));
		OutProps.dwTypeSpecificFlags = m_SampleProps.dwTypeSpecificFlags;
		OutProps.dwSampleFlags =
			(OutProps.dwSampleFlags & AM_SAMPLE_TYPECHANGED) |
			(m_SampleProps.dwSampleFlags & ~AM_SAMPLE_TYPECHANGED);

		OutProps.tStart = m_SampleProps.tStart;
		OutProps.tStop  = m_SampleProps.tStop;
		OutProps.cbData = FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId);

		hr = pOutSample2->SetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId), (PBYTE)&OutProps);
		if (m_SampleProps.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) {
			m_bSampleSkipped = FALSE;
		}
	} else {
		if (m_SampleProps.dwSampleFlags & AM_SAMPLE_TIMEVALID) {
			pOutSample->SetTime(&m_SampleProps.tStart, &m_SampleProps.tStop);
		}

		if (m_SampleProps.dwSampleFlags & AM_SAMPLE_SPLICEPOINT) {
			pOutSample->SetSyncPoint(TRUE);
		}

		if (m_SampleProps.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) {
			pOutSample->SetDiscontinuity(TRUE);
			m_bSampleSkipped = FALSE;
		}

		if (pInSample) {
			LONGLONG MediaStart, MediaEnd;
			if (pInSample->GetMediaTime(&MediaStart, &MediaEnd) == NOERROR) {
				pOutSample->SetMediaTime(&MediaStart, &MediaEnd);
			}
		}
	}

	*ppOutSample = pOutSample.Detach();

	return S_OK;
}

// pure virtual

HRESULT CStreamSwitcherInputPin::CheckMediaType(const CMediaType* pmt)
{
	return m_pSSF->CheckMediaType(pmt);
}

// virtual

HRESULT CStreamSwitcherInputPin::CheckConnect(IPin* pPin)
{
	return (IPin*)m_pSSF->GetOutputPin() == pPin
		   ? E_FAIL
		   : __super::CheckConnect(pPin);
}

HRESULT CStreamSwitcherInputPin::CompleteConnect(IPin* pReceivePin)
{
	HRESULT hr = __super::CompleteConnect(pReceivePin);
	if (FAILED(hr)) {
		return hr;
	}

	m_pSSF->CompleteConnect(PINDIR_INPUT, this, pReceivePin);

	m_bCanBlock = false;
	bool bForkedSomewhere = false;

	CString fileName;
	CString pinName;
	CString trackName;

	IPin* pPin = (IPin*)this;
	IBaseFilter* pBF = (IBaseFilter*)m_pFilter;

	pPin = GetUpStreamPin(pBF, pPin);
	if (pPin) {
		pBF = GetFilterFromPin(pPin);
	}
	while (pPin && pBF) {
		if (IsSplitter(pBF)) {
			pinName = GetPinName(pPin);
		}

		DWORD cStreams = 0;
		if (CComQIPtr<IAMStreamSelect> pSSF = pBF) {
			hr = pSSF->Count(&cStreams);
			if (SUCCEEDED(hr)) {
				for (long i = 0; i < (long)cStreams; i++) {
					WCHAR* pszName = nullptr;
					AM_MEDIA_TYPE* pmt = nullptr;
					hr = pSSF->Info(i, &pmt, nullptr, nullptr, nullptr, &pszName, nullptr, nullptr);
					if (SUCCEEDED(hr) && pmt && pmt->majortype == MEDIATYPE_Audio) {
						trackName = pszName;
						m_pISSF = pSSF;
						DeleteMediaType(pmt);
						if (pszName) {
							CoTaskMemFree(pszName);
						}
						break;
					}
					DeleteMediaType(pmt);
					if (pszName) {
						CoTaskMemFree(pszName);
					}
				}
			}
		}

		CLSID clsid = GetCLSID(pBF);
		if (clsid == CLSID_AviSplitter) {
			m_bCanBlock = true;
		}

		int nIn, nOut, nInC, nOutC;
		CountPins(pBF, nIn, nOut, nInC, nOutC);
		bForkedSomewhere = bForkedSomewhere || nIn > 1 || nOut > 1;

		if (CComQIPtr<IFileSourceFilter> pFSF = pBF) {
			WCHAR* pszName = nullptr;
			AM_MEDIA_TYPE mt;
			if (SUCCEEDED(pFSF->GetCurFile(&pszName, &mt)) && pszName) {
				fileName = pszName;
				CoTaskMemFree(pszName);

				if (::PathIsURLW(fileName)) {
					pinName = GetPinName(pPin);
					if (trackName.GetLength()) {
						fileName = trackName;
					} else if (pinName.GetLength()) {
						fileName = pinName;
					} else {
						fileName = L"Audio";
					}
				} else {
					if (!pinName.IsEmpty()) {
						fileName = pinName;
					/*
					} else {
						fileName.Replace('\\', '/');
						CString fn = fileName.Mid(fileName.ReverseFind('/') + 1);
						if (!fn.IsEmpty()) {
							fileName = fn;
						}
					*/
					}
				}

				if (m_pName) {
					delete[] m_pName;
				}
				const size_t len = fileName.GetLength() + 1;
				m_pName = DNew WCHAR[len];
				wcscpy_s(m_pName, len, fileName);

				if (cStreams == 1) { // Simple external track, no need to use the info from IAMStreamSelect
					m_pISSF.Release();
				}
			}

			break;
		}

		pPin = GetFirstPin(pBF);

		pPin = GetUpStreamPin(pBF, pPin);
		if (pPin) {
			pBF = GetFilterFromPin(pPin);
		}
	}

	if (!bForkedSomewhere) {
		m_bCanBlock = true;
	}

	m_hNotifyEvent = nullptr;

	return S_OK;
}

HRESULT CStreamSwitcherInputPin::Active()
{
	Block(!IsActive());

	return __super::Active();
}

HRESULT CStreamSwitcherInputPin::Inactive()
{
	Block(false);

	return __super::Inactive();
}

// IPin

STDMETHODIMP CStreamSwitcherInputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = __super::QueryAccept(pmt);
	if (S_OK != hr) {
		return hr;
	}

	return QueryAcceptDownstream(pmt);
}

STDMETHODIMP CStreamSwitcherInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	// FIXME: this locked up once
	// CAutoLock cAutoLock(&((CStreamSwitcherFilter*)m_pFilter)->m_csReceive);

	HRESULT hr;
	if (S_OK != (hr = QueryAcceptDownstream(pmt))) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if (m_Connected && m_Connected != pConnector) {
		return VFW_E_ALREADY_CONNECTED;
	}

	if (m_Connected) {
		m_Connected->Release(), m_Connected = nullptr;
	}

	return SUCCEEDED(__super::ReceiveConnection(pConnector, pmt)) ? S_OK : E_FAIL;
}

STDMETHODIMP CStreamSwitcherInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
	CheckPointer(ppAllocator, E_POINTER);

	if (m_pAllocator == nullptr) {
		(m_pAllocator = &m_Allocator)->AddRef();
	}

	(*ppAllocator = m_pAllocator)->AddRef();

	return NOERROR;
}

STDMETHODIMP CStreamSwitcherInputPin::NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly)
{
	HRESULT hr = __super::NotifyAllocator(pAllocator, bReadOnly);
	if (FAILED(hr)) {
		return hr;
	}

	m_bUsingOwnAllocator = (pAllocator == (IMemAllocator*)&m_Allocator);

	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::BeginFlush()
{
	CAutoLock cAutoLock(&m_pSSF->m_csState);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	if (!IsConnected() || !pOut || !pOut->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	HRESULT hr;
	if (FAILED(hr = __super::BeginFlush())) {
		return hr;
	}

	if (IsActive()) {
		return m_pSSF->DeliverBeginFlush();
	} else {
		Block(false);
		return S_OK;
	}
}

STDMETHODIMP CStreamSwitcherInputPin::EndFlush()
{
	CAutoLock cAutoLock(&m_pSSF->m_csState);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	if (!IsConnected() || !pOut || !pOut->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	HRESULT hr;
	if (FAILED(hr = __super::EndFlush())) {
		return hr;
	}

	if (IsActive()) {
		return m_pSSF->DeliverEndFlush();
	} else {
		Block(true);
		return S_OK;
	}
}

STDMETHODIMP CStreamSwitcherInputPin::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	if (!IsConnected() || !pOut || !pOut->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	if (m_hNotifyEvent) {
		SetEvent(m_hNotifyEvent), m_hNotifyEvent = nullptr;
		return S_OK;
	}

	return IsActive() ? m_pSSF->DeliverEndOfStream() : S_OK;
}

// IMemInputPin
STDMETHODIMP CStreamSwitcherInputPin::Receive(IMediaSample* pSample)
{
	bool bFormatChanged = m_pSSF->m_bInputPinChanged || m_pSSF->m_bOutputFormatChanged;

	AM_MEDIA_TYPE* pmt = nullptr;
	if (SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt) {
		const CMediaType mt(*pmt);
		DeleteMediaType(pmt), pmt = nullptr;
		bFormatChanged |= !!(mt != m_mt);
		SetMediaType(&mt);
	}

	// DAMN!!!!!! this doesn't work if the stream we are blocking
	// shares the same thread with another stream, mpeg splitters
	// are usually like that. Our nicely built up multithreaded
	// strategy is useless because of this, ARRRRRRGHHHHHH.

#ifdef BLOCKSTREAM
	if (m_bCanBlock) {
		m_evBlock.Wait();
	}
#endif

	if (!IsActive()) {
#ifdef BLOCKSTREAM
		if (m_bCanBlock) {
			return S_FALSE;
		}
#endif

		return E_FAIL; // a stupid fix for this stupid problem
	}

	CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	ASSERT(pOut->GetConnected());

	HRESULT hr = __super::Receive(pSample);
	if (S_OK != hr) {
		return hr;
	}

	if (m_SampleProps.dwStreamId != AM_STREAM_MEDIA) {
		return pOut->Deliver(pSample);
	}

	ALLOCATOR_PROPERTIES props, actual;
	hr = m_pAllocator->GetProperties(&props);
	hr = pOut->CurrentAllocator()->GetProperties(&actual);

	long cbBuffer = pSample->GetActualDataLength();

	if (bFormatChanged) {
		pOut->SetMediaType(&m_mt);

		CMediaType& out_mt = pOut->CurrentMediaType();
		if (*out_mt.FormatType() == FORMAT_WaveFormatEx) {
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.Format();
			WAVEFORMATEX* out_wfe = (WAVEFORMATEX*)out_mt.Format();

			cbBuffer *= out_wfe->nChannels * out_wfe->wBitsPerSample;
			cbBuffer /= wfe->nChannels * wfe->wBitsPerSample;
		}

		m_pSSF->m_bInputPinChanged = false;
		m_pSSF->m_bOutputFormatChanged = false;
	}

	if (bFormatChanged || cbBuffer > actual.cbBuffer) {
		DLog(L"CStreamSwitcherInputPin::Receive(): %s", bFormatChanged ? L"input or output media type changed" : L"cbBuffer > actual.cbBuffer");

		m_SampleProps.dwSampleFlags |= AM_SAMPLE_TYPECHANGED/* | AM_SAMPLE_DATADISCONTINUITY | AM_SAMPLE_TIMEDISCONTINUITY*/;

		/*
		if (CComQIPtr<IPinConnection> pPC = pOut->CurrentPinConnection()) {
			HANDLE hEOS = CreateEventW(nullptr, FALSE, FALSE, nullptr);
			hr = pPC->NotifyEndOfStream(hEOS);
			hr = pOut->DeliverEndOfStream();
			WaitForSingleObject(hEOS, 3000);
			CloseHandle(hEOS);
			hr = pOut->DeliverBeginFlush();
			hr = pOut->DeliverEndFlush();
		}
		*/

		if (props.cBuffers < 8 && m_mt.majortype == MEDIATYPE_Audio) {
			props.cBuffers = 8;
		}

		//props.cbBuffer = cbBuffer * 3 / 2;
		props.cbBuffer = cbBuffer * 2;

		if (actual.cbAlign != props.cbAlign
				|| actual.cbPrefix != props.cbPrefix
				|| actual.cBuffers < props.cBuffers
				|| actual.cbBuffer < props.cbBuffer) {
			hr = pOut->DeliverBeginFlush();
			hr = pOut->DeliverEndFlush();
			hr = pOut->CurrentAllocator()->Decommit();
			hr = pOut->CurrentAllocator()->SetProperties(&props, &actual);
			hr = pOut->CurrentAllocator()->Commit();
		}
	}

	CComPtr<IMediaSample> pOutSample;
	if (FAILED(InitializeOutputSample(pSample, &pOutSample))) {
		return E_FAIL;
	}

	pmt = nullptr;
	if (bFormatChanged || S_OK == pOutSample->GetMediaType(&pmt)) {
		pOutSample->SetMediaType(&pOut->CurrentMediaType());
		DLog(L"CStreamSwitcherInputPin::Receive(): output media type changed");
	}

	// Transform
	hr = m_pSSF->Transform(pSample, pOutSample);

	if (S_OK == hr) {
		if (pSample->GetActualDataLength() > 0 && pOutSample->GetActualDataLength() == 0) {
			return S_OK;
		}

		hr = pOut->Deliver(pOutSample);
		m_bSampleSkipped = FALSE;
		//ASSERT(SUCCEEDED(hr));
	}
	else if (S_FALSE == hr) {
		hr = S_OK;
		pOutSample = nullptr;
		m_bSampleSkipped = TRUE;

		if (!m_bQualityChanged) {
			m_pFilter->NotifyEvent(EC_QUALITY_CHANGE, 0, 0);
			m_bQualityChanged = TRUE;
		}
	}

	return hr;
}

STDMETHODIMP CStreamSwitcherInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if (!IsConnected()) {
		return S_OK;
	}

	CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherOutputPin* pOut = m_pSSF->GetOutputPin();
	if (!pOut || !pOut->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}

	HRESULT hr = m_pSSF->DeliverNewSegment(tStart, tStop, dRate);
	/*
	if (hr == S_OK) {
		const CLSID clsid = GetCLSID(pOut->GetConnected());
		if (clsid != CLSID_ReClock) {
			// hack - create and send an "empty" packet to make sure that the playback started is 100%
			const WAVEFORMATEX* out_wfe = (WAVEFORMATEX*)pOut->CurrentMediaType().pbFormat;
			const SampleFormat out_sampleformat = GetSampleFormat(out_wfe);
			if (out_sampleformat != SAMPLE_FMT_NONE) {
				CComPtr<IMediaSample> pOutSample;
				if (SUCCEEDED(InitializeOutputSample(nullptr, &pOutSample))
						&& SUCCEEDED(pOutSample->SetActualDataLength(0))) {
					REFERENCE_TIME rtStart = 0, rtStop = 0;
					pOutSample->SetTime(&rtStart, &rtStop);

					pOut->Deliver(pOutSample);
				}
			}
		}
	}
	*/

	return hr;
}

//
// CStreamSwitcherOutputPin
//

CStreamSwitcherOutputPin::CStreamSwitcherOutputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr)
	: CBaseOutputPin(L"CStreamSwitcherOutputPin", pFilter, &pFilter->m_csState, phr, L"Out")
	, m_pSSF(static_cast<CStreamSwitcherFilter*>(m_pFilter))
{
	//m_bCanReconnectWhenActive = true;
}

STDMETHODIMP CStreamSwitcherOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	CheckPointer(ppv,E_POINTER);
	ValidateReadWritePtr(ppv, sizeof(PVOID));
	*ppv = nullptr;

	if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
		if (m_pStreamSwitcherPassThru == nullptr) {
			HRESULT hr = S_OK;
			m_pStreamSwitcherPassThru = (IUnknown*)(INonDelegatingUnknown*)
										DNew CStreamSwitcherPassThru(GetOwner(), &hr, static_cast<CStreamSwitcherFilter*>(m_pFilter));

			if (!m_pStreamSwitcherPassThru) {
				return E_OUTOFMEMORY;
			}
			if (FAILED(hr)) {
				return hr;
			}
		}

		return m_pStreamSwitcherPassThru->QueryInterface(riid, ppv);
	}
	/*
		else if(riid == IID_IStreamBuilder)
		{
			return GetInterface((IStreamBuilder*)this, ppv);
		}
	*/
	return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CStreamSwitcherOutputPin::QueryAcceptUpstream(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = S_FALSE;

	CStreamSwitcherInputPin* pIn = m_pSSF->GetInputPin();

	if (pIn && pIn->IsConnected() && (pIn->IsUsingOwnAllocator() || pIn->CurrentMediaType() == *pmt)) {
		if (CComQIPtr<IPin> pPinTo = pIn->GetConnected()) {
			if (S_OK != (hr = pPinTo->QueryAccept(pmt))) {
				return VFW_E_TYPE_NOT_ACCEPTED;
			}
		} else {
			return E_FAIL;
		}
	}

	return hr;
}

// pure virtual

HRESULT CStreamSwitcherOutputPin::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	CStreamSwitcherInputPin* pIn = m_pSSF->GetInputPin();
	if (!pIn || !pIn->IsConnected()) {
		return E_UNEXPECTED;
	}

	CComPtr<IMemAllocator> pAllocatorIn;
	pIn->GetAllocator(&pAllocatorIn);
	if (!pAllocatorIn) {
		return E_UNEXPECTED;
	}

	HRESULT hr;
	if (FAILED(hr = pAllocatorIn->GetProperties(pProperties))) {
		return hr;
	}

	if (pProperties->cBuffers < 8 && pIn->CurrentMediaType().majortype == MEDIATYPE_Audio) {
		pProperties->cBuffers = 8;
	}

	if (*m_mt.FormatType() == FORMAT_WaveFormatEx) {
		long cbBuffer = pProperties->cbBuffer;
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.Format();
		WAVEFORMATEX* in_wfe = (WAVEFORMATEX*)pIn->CurrentMediaType().Format();

		cbBuffer *= wfe->nChannels * wfe->wBitsPerSample;
		cbBuffer /= in_wfe->nChannels * in_wfe->wBitsPerSample;

		if (cbBuffer > pProperties->cbBuffer) {
			pProperties->cbBuffer = cbBuffer;
		}
	}

	ALLOCATOR_PROPERTIES Actual;
	if (FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) {
		return hr;
	}

	return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		   ? E_FAIL
		   : NOERROR);
}

// virtual

HRESULT CStreamSwitcherOutputPin::CheckConnect(IPin* pPin)
{
	CComPtr<IBaseFilter> pBF = GetFilterFromPin(pPin);
	CLSID clsid = GetCLSID(pBF);

	return
		IsAudioWaveRenderer(pBF)
			|| clsid == CLSID_InfTee
			|| clsid == CLSID_XySubFilter_AutoLoader
			|| clsid == GUIDFromCString(L"{B86F6BEE-E7C0-4D03-8D52-5B4430CF6C88}") // ffdshow Audio Processor
			|| clsid == GUIDFromCString(L"{A753A1EC-973E-4718-AF8E-A3F554D45C44}") // AC3Filter
			|| clsid == GUIDFromCString(L"{B38C58A0-1809-11D6-A458-EDAE78F1DF12}") // DC-DSP Filter
			|| clsid == GUIDFromCString(L"{36F74DF0-12FF-4881-8A55-E7CE4D12688E}") // CyberLink TimeStretch Filter (PDVD10)
			|| clsid == GUIDFromCString(L"{32F1FE49-F8EB-4102-9B2E-E2C016BC049C}") // Acon Digital Media EffectChainer
			|| clsid == GUIDFromCString(L"{94297043-BD82-4DFD-B0DE-8177739C6D20}") // Resampler DMO
			//|| clsid == GUIDFromCString(L"{AEFA5024-215A-4FC7-97A4-1043C86FD0B8}") // MatrixMixer unstable when changing format
		? __super::CheckConnect(pPin)
		: E_FAIL;

	//	return CComQIPtr<IPinConnection>(pPin) ? CBaseOutputPin::CheckConnect(pPin) : E_NOINTERFACE;
	//	return CBaseOutputPin::CheckConnect(pPin);
}

HRESULT CStreamSwitcherOutputPin::BreakConnect()
{
	m_pPinConnection.Release();
	return __super::BreakConnect();
}

HRESULT CStreamSwitcherOutputPin::CompleteConnect(IPin* pReceivePin)
{
	m_pPinConnection = CComQIPtr<IPinConnection>(pReceivePin);
	HRESULT hr = __super::CompleteConnect(pReceivePin);

	CStreamSwitcherInputPin* pIn = m_pSSF->GetInputPin();
	CMediaType mt;
	if (SUCCEEDED(hr) && pIn && pIn->IsConnected()
			&& SUCCEEDED(pIn->GetConnected()->ConnectionMediaType(&mt))) {
		m_pSSF->TransformMediaType(mt, m_bForce16Bit);
		if (m_mt != mt) {
			if (auto clsid = GetCLSID(pReceivePin); clsid != CLSID_MpcAudioRenderer && clsid != CLSID_SanearAudioRenderer) {
				if (pIn->GetConnected()->QueryAccept(&m_mt) == S_OK) {
					hr = m_pFilter->ReconnectPin(pIn->GetConnected(), &m_mt);
				} else {
					hr = VFW_E_TYPE_NOT_ACCEPTED;
				}
			}
		}
	}

	if (SUCCEEDED(hr)) {
		m_bForce16Bit = false;
		m_pSSF->CompleteConnect(PINDIR_OUTPUT, this, pReceivePin);
	}

	return hr;
}

HRESULT CStreamSwitcherOutputPin::SetMediaType(const CMediaType *pmt)
{
	HRESULT hr = m_mt.Set(*pmt);
	if (FAILED(hr)) {
		return hr;
	}

	m_pSSF->TransformMediaType(m_mt, m_bForce16Bit);

	return NOERROR;
}

HRESULT CStreamSwitcherOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return m_pSSF->CheckMediaType(pmt);
}

HRESULT CStreamSwitcherOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	CStreamSwitcherInputPin* pIn = m_pSSF->GetInputPin();
	if (!pIn || !pIn->IsConnected()) {
		return E_UNEXPECTED;
	}

	CComPtr<IEnumMediaTypes> pEM;
	if (FAILED(pIn->GetConnected()->EnumMediaTypes(&pEM))) {
		return VFW_S_NO_MORE_ITEMS;
	}

	if (iPosition > 0 && FAILED(pEM->Skip(iPosition))) {
		return VFW_S_NO_MORE_ITEMS;
	}

	AM_MEDIA_TYPE* tmp = nullptr;
	if (S_OK != pEM->Next(1, &tmp, nullptr) || !tmp) {
		if (mtLastFormat.IsValid()) {
			if (!IsConnected()) {
				m_bForce16Bit = true;
				*pmt = mtLastFormat;

				FreeMediaType(mtLastFormat);
				mtLastFormat.InitMediaType();
				m_pSSF->TransformMediaType(*pmt, true);

				return S_OK;
			}
			FreeMediaType(mtLastFormat);
			mtLastFormat.InitMediaType();
		}

		return VFW_S_NO_MORE_ITEMS;
	}

	m_bForce16Bit = false;

	CopyMediaType(pmt, tmp);
	DeleteMediaType(tmp);

	m_pSSF->TransformMediaType(*pmt);

	if (pmt && pmt->pbFormat && !IsConnected()) {
		auto wfe = (const WAVEFORMATEX*)pmt->pbFormat;
		const auto sampleformat = GetSampleFormat(wfe);
		if (sampleformat != SAMPLE_FMT_NONE && sampleformat != SAMPLE_FMT_S16) {
			mtLastFormat = *pmt;
		}
	}

	return S_OK;
}

// IPin

STDMETHODIMP CStreamSwitcherOutputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = __super::QueryAccept(pmt);
	if (S_OK != hr) {
		return hr;
	}

	return QueryAcceptUpstream(pmt);
}

// IQualityControl

STDMETHODIMP CStreamSwitcherOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	CStreamSwitcherInputPin* pIn = m_pSSF->GetInputPin();
	if (!pIn || !pIn->IsConnected()) {
		return VFW_E_NOT_CONNECTED;
	}
	return pIn->PassNotify(q);
}

// IStreamBuilder

STDMETHODIMP CStreamSwitcherOutputPin::Render(IPin* ppinOut, IGraphBuilder* pGraph)
{
	CComPtr<IBaseFilter> pBF;
	pBF.CoCreateInstance(CLSID_DSoundRender);
	if (!pBF || FAILED(pGraph->AddFilter(pBF, L"Default DirectSound Device"))) {
		return E_FAIL;
	}

	if (FAILED(pGraph->ConnectDirect(ppinOut, GetFirstDisconnectedPin(pBF, PINDIR_INPUT), nullptr))) {
		pGraph->RemoveFilter(pBF);
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CStreamSwitcherOutputPin::Backout(IPin* ppinOut, IGraphBuilder* pGraph)
{
	return S_OK;
}

//
// CStreamSwitcherFilter
//

CStreamSwitcherFilter::CStreamSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr, const CLSID& clsid)
	: CBaseFilter(L"CStreamSwitcherFilter", lpunk, &m_csState, clsid)
{
	if (phr) {
		*phr = S_OK;
	}

	HRESULT hr = S_OK;

	do {
		hr = S_OK;
		std::unique_ptr<CStreamSwitcherInputPin> pInput(DNew CStreamSwitcherInputPin(this, &hr, L"Channel 1"));
		if (!pInput || FAILED(hr)) {
			break;
		}

		hr = S_OK;
		std::unique_ptr<CStreamSwitcherOutputPin> pOutput(DNew CStreamSwitcherOutputPin(this, &hr));
		if (!pOutput || FAILED(hr)) {
			break;
		}

		CAutoLock cAutoLock(&m_csPins);

		m_pInputs.push_front(m_pInput = pInput.release());
		m_pOutput = pOutput.release();

		return;
	} while (false);

	if (phr) {
		*phr = E_FAIL;
	}
}

CStreamSwitcherFilter::~CStreamSwitcherFilter()
{
	CAutoLock cAutoLock(&m_csPins);

	for (auto& pInput : m_pInputs) {
		delete pInput;
	}
	m_pInputs.clear();
	m_pInput = nullptr;

	delete m_pOutput;
	m_pOutput = nullptr;
}

STDMETHODIMP CStreamSwitcherFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

//

int CStreamSwitcherFilter::GetPinCount()
{
	CAutoLock cAutoLock(&m_csPins);

	return(1 + (int)m_pInputs.size());
}

CBasePin* CStreamSwitcherFilter::GetPin(int n)
{
	CAutoLock cAutoLock(&m_csPins);

	if (n < 0 || n >= GetPinCount()) {
		return nullptr;
	}

	if (n == 0) {
		return m_pOutput;
	}

	auto it = m_pInputs.begin();
	for (int i = 1; i < n; ++i) {
		++it;
	}

	return *it;
}

int CStreamSwitcherFilter::GetConnectedInputPinCount()
{
	CAutoLock cAutoLock(&m_csPins);

	int nConnected = 0;

	for (const auto& pInput : m_pInputs) {
		if (pInput->IsConnected()) {
			nConnected++;
		}
	}

	return(nConnected);
}

CStreamSwitcherInputPin* CStreamSwitcherFilter::GetConnectedInputPin(int n)
{
	if (n >= 0) {
		for (const auto& pPin : m_pInputs) {
			if (pPin->IsConnected()) {
				if (n == 0) {
					return pPin;
				}
				n--;
			}
		}
	}

	return nullptr;
}

CStreamSwitcherInputPin* CStreamSwitcherFilter::GetInputPin()
{
	return m_pInput;
}

CStreamSwitcherOutputPin* CStreamSwitcherFilter::GetOutputPin()
{
	return m_pOutput;
}

//

HRESULT CStreamSwitcherFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin, IPin* pReceivePin)
{
	if (dir == PINDIR_INPUT) {
		CAutoLock cAutoLock(&m_csPins);

		int nConnected = GetConnectedInputPinCount();

		if (nConnected == 1) {
			m_pInput = static_cast<CStreamSwitcherInputPin*>(pPin);
		}

		if ((size_t)nConnected == m_pInputs.size()) {
			CStringW name;
			name.Format(L"Channel %d", ++m_PinVersion);

			HRESULT hr = S_OK;
			CStreamSwitcherInputPin* pInputPin = DNew CStreamSwitcherInputPin(this, &hr, name);
			if (!pInputPin || FAILED(hr)) {
				delete pInputPin;
				return E_FAIL;
			}
			m_pInputs.push_back(pInputPin);
		}
	} else if (dir == PINDIR_OUTPUT) {
		CheckSupportedOutputMediaType();
	}

	return S_OK;
}

// this should be very thread safe, I hope it is, it must be... :)

void CStreamSwitcherFilter::SelectInput(CStreamSwitcherInputPin* pInput)
{
	// make sure no input thinks it is active
	m_pInput = nullptr;

	// release blocked GetBuffer in our own allocator & block all Receive
	for (auto pInput: m_pInputs) {
		CStreamSwitcherInputPin* pPin = pInput;
		pPin->Block(false);
		// a few Receive calls can arrive here, but since m_pInput == NULL neighter of them gets delivered
		pPin->Block(true);
	}

	// this will let waiting GetBuffer() calls go on inside our Receive()
	if (m_pOutput) {
		m_pOutput->DeliverBeginFlush();
		m_pOutput->DeliverEndFlush();
	}

	if (!pInput) {
		return;
	}

	// set new input
	m_pInput = pInput;
	m_bInputPinChanged = true;

	// let it go
	m_pInput->Block(false);
}

//

HRESULT CStreamSwitcherFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	BYTE* pDataIn = nullptr;
	BYTE* pDataOut = nullptr;

	HRESULT hr;
	if (FAILED(hr = pIn->GetPointer(&pDataIn))) {
		return hr;
	}
	if (FAILED(hr = pOut->GetPointer(&pDataOut))) {
		return hr;
	}

	long len = pIn->GetActualDataLength();
	long size = pOut->GetSize();

	if (!pDataIn || !pDataOut /*|| len > size || len <= 0*/) {
		return S_FALSE; // FIXME
	}

	memcpy(pDataOut, pDataIn, std::min(len, size));
	pOut->SetActualDataLength(std::min(len, size));

	return S_OK;
}

HRESULT CStreamSwitcherFilter::DeliverEndOfStream()
{
	return m_pOutput ? m_pOutput->DeliverEndOfStream() : E_FAIL;
}

HRESULT CStreamSwitcherFilter::DeliverBeginFlush()
{
	return m_pOutput ? m_pOutput->DeliverBeginFlush() : E_FAIL;
}

HRESULT CStreamSwitcherFilter::DeliverEndFlush()
{
	return m_pOutput ? m_pOutput->DeliverEndFlush() : E_FAIL;
}

HRESULT CStreamSwitcherFilter::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_dRate = dRate > 0.0 ? dRate : 1.0;
	return m_pOutput ? m_pOutput->DeliverNewSegment(tStart, tStop, dRate) : E_FAIL;
}

// IAMStreamSelect

STDMETHODIMP CStreamSwitcherFilter::Count(DWORD* pcStreams)
{
	CheckPointer(pcStreams, E_POINTER);

	CAutoLock cAutoLock(&m_csPins);

	*pcStreams = 0;
	for (const auto& pInputPin : m_pInputs) {

		if (pInputPin->IsConnected()) {
			if (CComPtr<IAMStreamSelect> pSSF = pInputPin->GetStreamSelectionFilter()) {
				DWORD cStreams = 0;
				HRESULT hr = pSSF->Count(&cStreams);
				if (SUCCEEDED(hr)) {
					for (long i = 0; i < (long)cStreams; i++) {
						AM_MEDIA_TYPE* pmt = nullptr;
						hr = pSSF->Info(i, &pmt, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
						if (SUCCEEDED(hr) && pmt && pmt->majortype == MEDIATYPE_Audio) {
							(*pcStreams)++;
						}
						DeleteMediaType(pmt);
					}
				}
			} else {
				(*pcStreams)++;
			}
		}
	}

	return S_OK;
}

STDMETHODIMP CStreamSwitcherFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	CAutoLock cAutoLock(&m_csPins);

	bool bFound = false;

	for (const auto& pInputPin : m_pInputs) {

		if (pInputPin->IsConnected()) {
			if (CComPtr<IAMStreamSelect> pSSF = pInputPin->GetStreamSelectionFilter()) {
				DWORD cStreams = 0;
				HRESULT hr = pSSF->Count(&cStreams);
				if (SUCCEEDED(hr)) {
					for (long i = 0; i < (long)cStreams; i++) {
						AM_MEDIA_TYPE* pmt = nullptr;
						DWORD dwFlags;
						LPWSTR pszName = nullptr;
						hr = pSSF->Info(i, &pmt, &dwFlags, plcid, nullptr, &pszName, nullptr, nullptr);
						if (SUCCEEDED(hr) && pmt && pmt->majortype == MEDIATYPE_Audio) {
							if (lIndex == 0) {

								if (ppmt) {
									*ppmt = pmt;
								} else {
									DeleteMediaType(pmt);
								}

								if (pdwFlags) {
									*pdwFlags = (m_pInput == pInputPin) ? dwFlags : 0;
								}

								if (pdwGroup) {
									*pdwGroup = 0;
								}

								if (ppszName) {
									*ppszName = pszName;
								} else {
									CoTaskMemFree(pszName);
								}

								bFound = true;
								break;
							} else {
								lIndex--;
							}
						}
						DeleteMediaType(pmt);
						CoTaskMemFree(pszName);
					}
				}
			} else if (lIndex == 0) {

				if (ppmt) {
					*ppmt = CreateMediaType(&m_pOutput->CurrentMediaType());
				}

				if (pdwFlags) {
					*pdwFlags = (m_pInput == pInputPin) ? AMSTREAMSELECTINFO_EXCLUSIVE : 0;
				}

				if (plcid) {
					*plcid = 0;
				}

				if (pdwGroup) {
					*pdwGroup = 0;
				}

				if (ppszName) {
					*ppszName = (WCHAR*)CoTaskMemAlloc((wcslen(pInputPin->Name()) + 1) * sizeof(WCHAR));
					if (*ppszName) {
						wcscpy_s(*ppszName, wcslen(pInputPin->Name()) + 1, pInputPin->Name());
					}
				}

				bFound = true;
				break;
			} else {
				lIndex--;
			}
		}

		if (bFound) {
			break;
		}
	}

	if (!bFound) {
		return E_INVALIDARG;
	}

	if (ppUnk) {
		*ppUnk = nullptr;
	}

	return S_OK;
}

STDMETHODIMP CStreamSwitcherFilter::Enable(long lIndex, DWORD dwFlags)
{
	if (dwFlags != AMSTREAMSELECTENABLE_ENABLE) {
		return E_NOTIMPL;
	}

	PauseGraph;

	long i = 0;
	CStreamSwitcherInputPin* pNewInputPin = nullptr;

	for (const auto& pInputPin : m_pInputs) {
		if (pInputPin->IsConnected()) {
			if (CComPtr<IAMStreamSelect> pSSF = pInputPin->GetStreamSelectionFilter()) {
				DWORD cStreams = 0;
				HRESULT hr = pSSF->Count(&cStreams);
				if (SUCCEEDED(hr)) {
					for (i = 0; i < (long)cStreams; i++) {
						AM_MEDIA_TYPE* pmt = nullptr;
						hr = pSSF->Info(i, &pmt, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
						if (SUCCEEDED(hr) && pmt && pmt->majortype == MEDIATYPE_Audio) {
							if (lIndex == 0) {
								DeleteMediaType(pmt);
								pNewInputPin = pInputPin;
								break;
							} else {
								lIndex--;
							}
						}
						DeleteMediaType(pmt);
					}
				}
			} else if (lIndex == 0) {
				pNewInputPin = pInputPin;
				break;
			} else {
				lIndex--;
			}
		}

		if (pNewInputPin) {
			break;
		}
	}

	if (!pNewInputPin) {
		return E_INVALIDARG;
	}

	SelectInput(pNewInputPin);

	if (CComPtr<IAMStreamSelect> pSSF = pNewInputPin->GetStreamSelectionFilter()) {
		pSSF->Enable(i, dwFlags);
	}

	ResumeGraph;

	return S_OK;
}
