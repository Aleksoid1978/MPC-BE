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

#pragma once

#include <atlbase.h>
#include <qnetwork.h>
#include <IKeyFrameInfo.h>
#include <IBufferInfo.h>
#include <IBitRateInfo.h>
#include "BaseSplitterFileEx.h"
#include "BaseSplitterInputPin.h"
#include "BaseSplitterOutputPin.h"
#include "BaseSplitterParserOutputPin.h"
#include "AsyncReader.h"
#include "../../../DSUtil/DSMPropertyBag.h"
#include "../../../DSUtil/FontInstaller.h"
#include "../../../DSUtil/MediaDescription.h"
#include <vector>

#define PACKET_PTS_DISCONTINUITY		0x0001
#define PACKET_PTS_VALIDATE_POSITIVE	0x0002

class CBaseSplitterFilter
	: public CBaseFilter
	, public CCritSec
	, public IDSMPropertyBagImpl
	, public IDSMResourceBagImpl
	, public IDSMChapterBagImpl
	, protected CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
	, public IAMOpenProgress
	, public IAMMediaContent
	, public IAMExtendedSeeking
	, public IKeyFrameInfo
	, public IBufferInfo
{
	CCritSec m_csPinMap;
	CAtlMap<DWORD, CBaseSplitterOutputPin*> m_pPinMap;

	CCritSec m_csmtnew;
	CAtlMap<DWORD, CMediaType> m_mtnew;

	CAutoPtrList<CBaseSplitterOutputPin> m_pRetiredOutputs;

	CComQIPtr<ISyncReader>		m_pSyncReader;

protected:
	CHdmvClipInfo::CPlaylist	m_Items;
	CStringW m_fn;

	CAutoPtr<CBaseSplitterInputPin> m_pInput;
	CAutoPtrList<CBaseSplitterOutputPin> m_pOutputs;

	CBaseSplitterOutputPin* GetOutputPin(DWORD TrackNum);
	DWORD GetOutputTrackNum(CBaseSplitterOutputPin* pPin);
	HRESULT AddOutputPin(DWORD TrackNum, CAutoPtr<CBaseSplitterOutputPin> pPin);
	HRESULT RenameOutputPin(DWORD TrackNumSrc, DWORD TrackNumDst, std::vector<CMediaType> mts, BOOL bNeedReconnect = FALSE);
	virtual HRESULT DeleteOutputs();
	virtual HRESULT CreateOutputs(IAsyncReader* pAsyncReader) PURE; // override this ...
	virtual LPCTSTR GetPartFilename(IAsyncReader* pAsyncReader);

	LONGLONG m_nOpenProgress;
	bool m_fAbort;

	REFERENCE_TIME m_rtDuration; // derived filter should set this at the end of CreateOutputs
	REFERENCE_TIME m_rtStart, m_rtStop, m_rtCurrent, m_rtNewStart, m_rtNewStop;
	double m_dRate;

	CAtlList<UINT64> m_bDiscontinuitySent;
	CAtlList<CBaseSplitterOutputPin*> m_pActivePins;

	CAMEvent m_eEndFlush;
	bool m_fFlushing;

	void DeliverBeginFlush();
	void DeliverEndFlush();
	HRESULT DeliverPacket(CAutoPtr<CPacket> p);

	int m_priority;

	CFontInstaller m_fontinst;

	DWORD m_nFlag;

	int m_MaxOutputQueueMs; // 100..10000 ms

protected:
	enum {CMD_EXIT, CMD_SEEK};
	DWORD ThreadProc();

	// ... and also override all these too
	virtual bool DemuxInit() PURE;
	virtual void DemuxSeek(REFERENCE_TIME rt) PURE;
	virtual bool DemuxLoop() PURE;
	virtual bool BuildPlaylist(LPCTSTR pszFileName, CHdmvClipInfo::CPlaylist& Items) { return false; };
	virtual bool BuildChapters(LPCTSTR pszFileName, CHdmvClipInfo::CPlaylist& PlaylistItems, CHdmvClipInfo::CPlaylistChapter& Items) { return false; };

public:
	CBaseSplitterFilter(LPCTSTR pName, LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid);
	virtual ~CBaseSplitterFilter();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT BreakConnect(PIN_DIRECTION dir, CBasePin* pPin);
	HRESULT CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

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

protected:
	friend class CBaseSplitterOutputPin;
	virtual HRESULT SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);

private:
	REFERENCE_TIME m_rtLastStart, m_rtLastStop;
	CAtlList<void*> m_LastSeekers;

public:
	// IAMOpenProgress

	STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
	STDMETHODIMP AbortOperation();

	// IDispatch

	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) {return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) {return E_NOTIMPL;}

	// IAMMediaContent

	STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName);
	STDMETHODIMP get_Title(BSTR* pbstrTitle);
	STDMETHODIMP get_Rating(BSTR* pbstrRating);
	STDMETHODIMP get_Description(BSTR* pbstrDescription);
	STDMETHODIMP get_Copyright(BSTR* pbstrCopyright);
	STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText) {return E_NOTIMPL;}

	// IAMExtendedSeeking

	STDMETHODIMP get_ExSeekCapabilities(long* pExCapabilities);
	STDMETHODIMP get_MarkerCount(long* pMarkerCount);
	STDMETHODIMP get_CurrentMarker(long* pCurrentMarker);
	STDMETHODIMP GetMarkerTime(long MarkerNum, double* pMarkerTime);
	STDMETHODIMP GetMarkerName(long MarkerNum, BSTR* pbstrMarkerName);
	STDMETHODIMP put_PlaybackSpeed(double Speed) {return E_NOTIMPL;}
	STDMETHODIMP get_PlaybackSpeed(double* pSpeed) {return E_NOTIMPL;}

	// IKeyFrameInfo

	STDMETHODIMP_(HRESULT) GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP_(HRESULT) GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);

	// IBufferInfo

	STDMETHODIMP_(int) GetCount();
	STDMETHODIMP GetStatus(int i, int& samples, int& size);
	STDMETHODIMP_(DWORD) GetPriority();

public:
	DWORD GetFlag() { return m_nFlag; }

protected:
	void SortOutputPin();
};
