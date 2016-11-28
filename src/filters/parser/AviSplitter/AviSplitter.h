/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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

#include <atlbase.h>
#include <atlcoll.h>
#include "AviSplitterSettingsWnd.h"
#include "../BaseSplitter/BaseSplitter.h"

#define AviSplitterName L"MPC AVI Splitter"
#define AviSourceName   L"MPC AVI Source"

class CAviSplitterOutputPin : public CBaseSplitterOutputPin
{
public:
	CAviSplitterOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT CheckConnect(IPin* pPin);
};

class CAviFile;

class __declspec(uuid("9736D831-9D6C-4E72-B6E7-560EF9181001"))
	CAviSplitterFilter
	: public CBaseSplitterFilter
	, public ISpecifyPropertyPages2
	, public IAviSplitterFilter
{
	CAutoVectorPtr<DWORD> m_tFrame;

private:
	bool m_bBadInterleavedSuport, m_bSetReindex;

protected:
	CCritSec m_csProps;
	CAutoPtr<CAviFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	HRESULT ReIndex(__int64 end, UINT64& Size, DWORD TrackNumber);

	REFERENCE_TIME m_maxTimeStamp;

public:
	CAviSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter

	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);

	// IKeyFrameInfo

	STDMETHODIMP GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);

	// ISpecifyPropertyPages2

	STDMETHODIMP GetPages(CAUUID* pPages);
	STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// IAviSplitterFilter

	STDMETHODIMP Apply();
	STDMETHODIMP SetBadInterleavedSuport(BOOL nValue);
	STDMETHODIMP_(BOOL) GetBadInterleavedSuport();
	STDMETHODIMP SetReindex(BOOL nValue);
	STDMETHODIMP_(BOOL) GetReindex();
};

class __declspec(uuid("CEA8DEFF-0AF7-4DB9-9A38-FB3C3AEFC0DE"))
	CAviSourceFilter : public CAviSplitterFilter
{
public:
	CAviSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
