/*
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

#pragma once

#include "../BaseSplitter/BaseSplitter.h"
#include "../../filters/FilterInterfacesImpl.h"

#define RawVideoSplitterName	L"MPC RAW Video Splitter"
#define RawVideoSourceName		L"MPC RAW Video Source"

class __declspec(uuid("486AA463-EE67-4F75-B941-F1FAB217B342"))
	CRawVideoSplitterFilter
	: public CBaseSplitterFilter
	, public CExFilterInfoImpl
{
	enum {
		RAW_NONE,
		RAW_MPEG1,
		RAW_MPEG2,
		RAW_H264,
		RAW_VC1,
		RAW_HEVC,
		RAW_Y4M,
		RAW_MPEG4,
	} m_RAWType = RAW_NONE;

	int y4m_interl = -1;

	__int64			m_startpos			= 0;
	REFERENCE_TIME	m_AvgTimePerFrame	= 0;
	int				m_framesize			= 0; // for YUV4MPEG2 only

protected:
	CAutoPtr<CBaseSplitterFileEx> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	bool ReadGOP(REFERENCE_TIME& rt);
public:
	CRawVideoSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);

	// IBufferControl
	STDMETHODIMP SetBufferDuration(int duration);

	// IExFilterInfo
	STDMETHODIMP GetInt(LPCSTR field, int *value) override;
};

class __declspec(uuid("E32A3501-04A9-486B-898B-F5A4C8A4AAAC"))
	CRawVideoSourceFilter : public CRawVideoSplitterFilter
{
public:
	CRawVideoSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
