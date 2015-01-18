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

#pragma once

#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/DSMPropertyBag.h"

interface __declspec(uuid("30AB78C7-5259-4594-AEFE-9C0FC2F08A5E"))
IBitStream :
public IUnknown {
	STDMETHOD_(UINT64, GetPos) () PURE;
	STDMETHOD_(UINT64, Seek) (UINT64 pos) PURE; // it's a _stream_, please don't seek if you don't have to
	STDMETHOD(ByteWrite) (const void* pData, int len) PURE;
	STDMETHOD(BitWrite) (UINT64 data, int len) PURE;
	STDMETHOD(BitFlush) () PURE;
	STDMETHOD(StrWrite) (LPCSTR pData, BOOL bFixNewLine) PURE;
};

class CBitStream : public CUnknown, public IBitStream
{
	CComPtr<IStream> m_pStream;
	bool m_fThrowError;
	UINT64 m_bitbuff;
	int m_bitlen;

public:
	CBitStream(IStream* pStream, bool m_fThrowError = false);
	virtual ~CBitStream();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IBitStream

	STDMETHODIMP_(UINT64) GetPos();
	STDMETHODIMP_(UINT64) Seek(UINT64 pos);
	STDMETHODIMP ByteWrite(const void* pData, int len);
	STDMETHODIMP BitWrite(UINT64 data, int len);
	STDMETHODIMP BitFlush();
	STDMETHODIMP StrWrite(LPCSTR pData, BOOL bFixNewLine);
};

interface __declspec(uuid("EE6F2741-7DB4-4AAD-A3CB-545208EE4C0A"))
IBaseMuxerRelatedPin :
public IUnknown {
	STDMETHOD(SetRelatedPin) (CBasePin* pPin) PURE;
	STDMETHOD_(CBasePin*, GetRelatedPin) () PURE;
};

class CBaseMuxerRelatedPin : public IBaseMuxerRelatedPin
{
	CBasePin* m_pRelatedPin; // should not hold a reference because it would be circular

public:
	CBaseMuxerRelatedPin();
	virtual ~CBaseMuxerRelatedPin();

	// IBaseMuxerRelatedPin

	STDMETHODIMP SetRelatedPin(CBasePin* pPin);
	STDMETHODIMP_(CBasePin*) GetRelatedPin();
};

class CBaseMuxerInputPin;

struct MuxerPacket {
	CBaseMuxerInputPin* pPin;
	REFERENCE_TIME rtStart, rtStop;
	CAtlArray<BYTE> pData;
	enum flag_t {empty = 0, timevalid = 1, syncpoint = 2, discontinuity = 4, eos = 8, bogus = 16};
	DWORD flags;
	int index;
	struct MuxerPacket(CBaseMuxerInputPin* pPin) {
		this->pPin = pPin;
		rtStart = rtStop = INVALID_TIME;
		flags = empty;
		index = -1;
	}
	bool IsTimeValid() const {
		return !!(flags & timevalid);
	}
	bool IsSyncPoint() const {
		return !!(flags & syncpoint);
	}
	bool IsDiscontinuity() const {
		return !!(flags & discontinuity);
	}
	bool IsEOS() const {
		return !!(flags & eos);
	}
	bool IsBogus() const {
		return !!(flags & bogus);
	}
};

class CBaseMuxerInputPin : public CBaseInputPin, public CBaseMuxerRelatedPin, public IDSMPropertyBagImpl
{
private:
	int m_iID;

	CCritSec m_csReceive;
	REFERENCE_TIME m_rtMaxStart, m_rtDuration;
	bool m_fEOS;
	int m_iPacketIndex;

	CCritSec m_csQueue;
	CAutoPtrList<MuxerPacket> m_queue;
	void PushPacket(CAutoPtr<MuxerPacket> pPacket);
	CAutoPtr<MuxerPacket> PopPacket();
	CAMEvent m_evAcceptPacket;

	friend class CBaseMuxerFilter;

public:
	CBaseMuxerInputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CBaseMuxerInputPin();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	REFERENCE_TIME GetDuration() {
		return m_rtDuration;
	}
	int GetID() {
		return m_iID;
	}
	CMediaType& CurrentMediaType() {
		return m_mt;
	}
	bool IsSubtitleStream();

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin* pReceivePin);

	HRESULT Active();
	HRESULT Inactive();

	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	STDMETHODIMP Receive(IMediaSample* pSample);
	STDMETHODIMP EndOfStream();
};

class CBaseMuxerOutputPin : public CBaseOutputPin
{
	CComPtr<IBitStream> m_pBitStream;

public:
	CBaseMuxerOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CBaseMuxerOutputPin() {}

	IBitStream* GetBitStream();

	HRESULT BreakConnect();

	HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);

	HRESULT DeliverEndOfStream();

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
};

class CBaseMuxerRawOutputPin : public CBaseMuxerOutputPin, public CBaseMuxerRelatedPin
{
	struct idx_t {
		REFERENCE_TIME rt;
		__int64 fp;
	};
	CAtlList<idx_t> m_idx;

public:
	CBaseMuxerRawOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CBaseMuxerRawOutputPin() {}

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	virtual void MuxHeader(const CMediaType& mt);
	virtual void MuxPacket(const CMediaType& mt, const MuxerPacket* pPacket);
	virtual void MuxFooter(const CMediaType& mt);
};

class CBaseMuxerFilter
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IMediaSeeking
	, public IDSMPropertyBagImpl
	, public IDSMResourceBagImpl
	, public IDSMChapterBagImpl
{
private:
	CAutoPtrList<CBaseMuxerInputPin> m_pInputs;
	CAutoPtr<CBaseMuxerOutputPin> m_pOutput;
	CAutoPtrList<CBaseMuxerRawOutputPin> m_pRawOutputs;

	enum {CMD_EXIT, CMD_RUN};
	DWORD ThreadProc();

	REFERENCE_TIME m_rtCurrent;
	CAtlList<CBaseMuxerInputPin*> m_pActivePins;

	CAutoPtr<MuxerPacket> GetPacket();

	void MuxHeaderInternal();
	void MuxPacketInternal(const MuxerPacket* pPacket);
	void MuxFooterInternal();

protected:
	CAtlList<CBaseMuxerInputPin*> m_pPins;
	CBaseMuxerOutputPin* GetOutputPin() {
		return m_pOutput;
	}

	virtual void MuxInit() PURE;

	// only called when the output pin is connected
	virtual void MuxHeader(IBitStream* pBS) {}
	virtual void MuxPacket(IBitStream* pBS, const MuxerPacket* pPacket) {}
	virtual void MuxFooter(IBitStream* pBS) {}

	// always called (useful if the derived class wants to write somewhere else than downstream)
	virtual void MuxHeader() {}
	virtual void MuxPacket(const MuxerPacket* pPacket) {}
	virtual void MuxFooter() {}

	// allows customized pins in derived classes
	virtual HRESULT CreateInput(CStringW name, CBaseMuxerInputPin** ppPin);
	virtual HRESULT CreateRawOutput(CStringW name, CBaseMuxerRawOutputPin** ppPin);

public:
	CBaseMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid);
	virtual ~CBaseMuxerFilter();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	void AddInput();

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

	// IMediaSeeking

	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);
};
