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

#include <atlbase.h>
#include <AsyncReader/asyncio.h>
#include <AsyncReader/asyncrdr.h>

#include <ITrackInfo.h>
#include "VTSReaderSettingsWnd.h"
#include "../../../DSUtil/DSMPropertyBag.h"

#include "IVTSReader.h"

#define VTSReaderName L"MPC VTS Reader"

class CIfoFile;
class CVobFile;

class CVTSStream : public CAsyncStream
{
private:
	CCritSec m_csLock;

	CAutoPtr<CIfoFile> m_ifo;
	CAutoPtr<CVobFile> m_vob;
	int m_off;

public:
	CVTSStream();
	virtual ~CVTSStream();

	bool Load(const WCHAR* fnw, bool bEnableTitleSelection);

	HRESULT SetPointer(LONGLONG llPos);
	HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
	LONGLONG Size(LONGLONG* pSizeAvailable);
	DWORD Alignment();
	void Lock();
	void Unlock();

	BSTR			GetTrackName(UINT aTrackIdx);
	UINT			GetChaptersCount();
	REFERENCE_TIME	GetChapterTime(UINT ChapterNumber);

	REFERENCE_TIME	GetDuration();

	fraction_t		GetAspectRatio();
};

class __declspec(uuid("773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73"))
	CVTSReader
	: public CAsyncReader
	, public IFileSourceFilter
	, public ITrackInfo
	, public IDSMChapterBagImpl
	, public IVTSReader
	, public ISpecifyPropertyPages2
{
	CVTSStream m_stream;
	CStringW m_fn;

private:
	CCritSec m_csProps;
	bool m_bEnableTitleSelection;

public:
	CVTSReader(IUnknown* pUnk, HRESULT* phr);
	~CVTSReader();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);

	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// ITrackInfo
	STDMETHODIMP_(UINT) GetTrackCount();
	STDMETHODIMP_(BOOL) GetTrackInfo(UINT aTrackIdx, struct TrackElement* pStructureToFill);
	STDMETHODIMP_(BOOL) GetTrackExtendedInfo(UINT aTrackIdx, void* pStructureToFill);
	STDMETHODIMP_(BSTR) GetTrackName(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecID(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecName(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecInfoURL(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecDownloadURL(UINT aTrackIdx);

	// ISpecifyPropertyPages2
	STDMETHODIMP GetPages(CAUUID* pPages);
	STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// IVTSReader
	STDMETHODIMP Apply();

	STDMETHODIMP SetEnableTitleSelection(BOOL nValue);
	STDMETHODIMP_(BOOL) GetEnableTitleSelection();

	STDMETHODIMP_(REFERENCE_TIME) GetDuration();
	STDMETHODIMP_(fraction_t) GetAspectRatio();
};
