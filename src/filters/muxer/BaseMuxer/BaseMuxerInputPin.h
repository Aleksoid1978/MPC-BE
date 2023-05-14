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

#pragma once

#include <mpc_defines.h>
#include <basestruct.h>
#include "BaseMuxerRelatedPin.h"
#include "DSUtil/DSMPropertyBag.h"

class CBaseMuxerInputPin;

struct MuxerPacket {
	CBaseMuxerInputPin* pPin;
	REFERENCE_TIME rtStart = INVALID_TIME;
	REFERENCE_TIME rtStop = INVALID_TIME;
	std::unique_ptr<BYTE[]> data;
	size_t size = 0;
	enum flag_t {
		empty = 0,
		timevalid = 1,
		syncpoint = 2,
		discontinuity = 4,
		eos = 8,
		bogus = 16
	};
	DWORD flags = empty;
	int index = -1;

	MuxerPacket(CBaseMuxerInputPin* pInputPin, const BYTE* ptr, long len)
		: pPin(pInputPin)
		, size(len)
	{
		ASSERT(len >= 0);
		data.reset(new BYTE[size]);
		memcpy(data.get(), ptr, size);
	}

	bool IsTimeValid() const { return !!(flags & timevalid); }
	bool IsSyncPoint() const { return !!(flags & syncpoint); }
	bool IsDiscontinuity() const { return !!(flags & discontinuity); }
	bool IsEOS() const { return !!(flags & eos); }
	bool IsBogus() const { return !!(flags & bogus); }
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
	std::list<std::unique_ptr<MuxerPacket>> m_queue;
	void PushPacket(std::unique_ptr<MuxerPacket>& pPacket);
	std::unique_ptr<MuxerPacket> PopPacket();
	CAMEvent m_evAcceptPacket;

	friend class CBaseMuxerFilter;

public:
	CBaseMuxerInputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CBaseMuxerInputPin();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	REFERENCE_TIME GetDuration() { return m_rtDuration; }
	int GetID() { return m_iID; }
	CMediaType& CurrentMediaType() { return m_mt; }
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
