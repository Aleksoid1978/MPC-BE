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

#pragma once

#include <qnetwork.h>
#include <IKeyFrameInfo.h>
#include <IBufferInfo.h>
#include <IBitRateInfo.h>
#include "filters/filters/FilterInterfacesImpl.h"
#include "BaseSplitterFileEx.h"
#include "BaseSplitterInputPin.h"
#include "BaseSplitterOutputPin.h"
#include "AsyncReader.h"
#include "DSUtil/DSMPropertyBag.h"
#include "DSUtil/FontInstaller.h"
#include "DSUtil/MediaDescription.h"

#define PACKET_PTS_DISCONTINUITY 0x0001

#define SOURCE_SUPPORT_URL       0x0002

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
	, public CExFilterConfigImpl
{
	CCritSec m_csPinMap;
	std::map<DWORD, CBaseSplitterOutputPin*> m_pPinMap;

	CCritSec m_csmtnew;
	std::map<DWORD, CMediaType> m_mtnew;

	std::list<std::unique_ptr<CBaseSplitterOutputPin>> m_pRetiredOutputs;

protected:
	CHdmvClipInfo::CPlaylist m_Items;
	CStringW m_fn;

	std::unique_ptr<CBaseSplitterInputPin> m_pInput;
	std::list<std::unique_ptr<CBaseSplitterOutputPin>> m_pOutputs;

	CBaseSplitterOutputPin* GetOutputPin(DWORD TrackNum);
	DWORD GetOutputTrackNum(CBaseSplitterOutputPin* pPin);
	HRESULT AddOutputPin(DWORD TrackNum, std::unique_ptr<CBaseSplitterOutputPin>& pPin);
	HRESULT RenameOutputPin(DWORD TrackNumSrc, DWORD TrackNumDst, std::vector<CMediaType> mts, BOOL bNeedReconnect = FALSE);
	virtual HRESULT DeleteOutputs();
	virtual HRESULT CreateOutputs(IAsyncReader* pAsyncReader) PURE; // override this ...
	virtual LPCWSTR GetPartFilename(IAsyncReader* pAsyncReader);

	LONGLONG m_nOpenProgress = 100;
	bool m_fAbort = false;

	REFERENCE_TIME m_rtDuration = 0; // derived filter should set this at the end of CreateOutputs
	REFERENCE_TIME m_rtStart    = 0;
	REFERENCE_TIME m_rtStop     = 0;
	REFERENCE_TIME m_rtCurrent  = 0;
	REFERENCE_TIME m_rtNewStart = 0;
	REFERENCE_TIME m_rtNewStop  = 0;
	double m_dRate = 1.0;

	std::list<DWORD> m_bDiscontinuitySent;
	std::list<CBaseSplitterOutputPin*> m_pActivePins;

	CAMEvent m_eEndFlush;
	bool m_fFlushing = false;

	void DeliverBeginFlush();
	void DeliverEndFlush();
	HRESULT DeliverPacket(std::unique_ptr<CPacket> p);

	int m_priority = THREAD_PRIORITY_NORMAL;

	CFontInstaller m_fontinst;

	DWORD m_nFlag = 0;

	int m_iQueueDuration = QUEUE_DURATION_DEF; //  100..15000 ms
	//int m_iNetworkTimeout = NETWORK_TIMEOUT_DEF; // 2000..20000 ms

	REFERENCE_TIME m_rtOffset = INVALID_TIME;

protected:
	enum {CMD_EXIT, CMD_SEEK};
	DWORD ThreadProc();

	// ... and also override all these too
	virtual bool DemuxInit() PURE;
	virtual void DemuxSeek(REFERENCE_TIME rt) PURE;
	virtual bool DemuxLoop() PURE;
	virtual bool BuildPlaylist(LPCWSTR pszFileName, CHdmvClipInfo::CPlaylist& Items, BOOL bReadMVCExtension = TRUE) { return false; };
	virtual bool BuildChapters(LPCWSTR pszFileName, CHdmvClipInfo::CPlaylist& PlaylistItems, CHdmvClipInfo::CPlaylistChapter& Items) { return false; };

public:
	CBaseSplitterFilter(LPCWSTR pName, LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid);
	virtual ~CBaseSplitterFilter();

	bool IsSomePinDrying();

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
	REFERENCE_TIME m_rtLastStart = INVALID_TIME;
	REFERENCE_TIME m_rtLastStop  = INVALID_TIME;
	std::list<void*> m_LastSeekers;

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

	// IExFilterConfig

	STDMETHODIMP Flt_GetInt(LPCSTR field, int *value) override;
	STDMETHODIMP Flt_GetInt64(LPCSTR field, __int64* value) override;
	STDMETHODIMP Flt_SetInt(LPCSTR field, int value) override;

public:
	CComQIPtr<ISyncReader> m_pSyncReader;

	DWORD GetFlag() { return m_nFlag; }

protected:
	void SortOutputPin();
};
