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

#include "../BaseSplitter/BaseSplitter.h"
#include "MpegSplitterFile.h"
#include "MpegSplitterSettingsWnd.h"
#include "../../../DSUtil/AudioParser.h"
#include <ITrackInfo.h>

#define MpegSplitterName L"MPC MPEG Splitter"
#define MpegSourceName   L"MPC MPEG Source"

class __declspec(uuid("DC257063-045F-4BE2-BD5B-E12279C464F0"))
	CMpegSplitterFilter
	: public CBaseSplitterFilter
	, public IAMStreamSelect
	, public ISpecifyPropertyPages2
	, public IMpegSplitterFilter
{
	REFERENCE_TIME	m_rtStartOffset;
	bool			m_pPipoBimbo;
	CHdmvClipInfo	m_ClipInfo;

	CAutoPtr<CMpegSplitterFile> m_pFile;
	CComQIPtr<ITrackInfo> m_pTI;

	REFERENCE_TIME m_rtPlaylistDuration;
	REFERENCE_TIME m_rtMin, m_rtMax;

	CAtlMap<DWORD, CAutoPtr<CPacket>> pPackets; 

	CString m_AudioLanguageOrder, m_SubtitlesLanguageOrder;
	bool m_ForcedSub, m_SubEmptyPin;
	int m_AC3CoreOnly;
	CCritSec m_csProps;

	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);
	void	ReadClipInfo(LPCOLESTR pszFileName);

	STDMETHODIMP GetDuration(LONGLONG* pDuration);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();
	bool BuildPlaylist(LPCTSTR pszFileName, CHdmvClipInfo::CPlaylist& files);
	bool BuildChapters(LPCTSTR pszFileName, CHdmvClipInfo::CPlaylist& PlaylistItems, CHdmvClipInfo::CPlaylistChapter& Items);

	HRESULT DemuxNextPacket(REFERENCE_TIME rtStartOffset);

	void HandleStream(CMpegSplitterFile::stream& s, CString fName, DWORD dwPictAspectRatioX, DWORD dwPictAspectRatioY, CStringA& palette);

	CString FormatStreamName(CMpegSplitterFile::stream& s, CMpegSplitterFile::stream_type type);

	__int64 SeekBD(REFERENCE_TIME rt);

public:
	CMpegSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid = __uuidof(CMpegSplitterFilter));
	void SetPipo(bool bPipo) {
		m_pPipoBimbo = bPipo;
	};

	void GetMediaTypes(CMpegSplitterFile::stream_type sType, CAtlArray<CMediaType>& mts);

	bool m_hasHDMVSubPin;
	bool IsHDMVSubPinDrying();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP GetClassID(CLSID* pClsID);
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);

	// CBaseFilter

	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);

	// IAMStreamSelect

	STDMETHODIMP Count(DWORD* pcStreams);
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
	STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);

	// ISpecifyPropertyPages2

	STDMETHODIMP GetPages(CAUUID* pPages);
	STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// IMpegSplitterFilter
	STDMETHODIMP Apply();

	STDMETHODIMP SetForcedSub(BOOL nValue);
	STDMETHODIMP_(BOOL) GetForcedSub();

	STDMETHODIMP SetAudioLanguageOrder(WCHAR *nValue);
	STDMETHODIMP_(WCHAR *) GetAudioLanguageOrder();

	STDMETHODIMP SetSubtitlesLanguageOrder(WCHAR *nValue);
	STDMETHODIMP_(WCHAR *) GetSubtitlesLanguageOrder();

	STDMETHODIMP SetVC1_GuidFlag(int nValue);
	STDMETHODIMP_(int) GetVC1_GuidFlag();

	STDMETHODIMP SetTrueHD(int nValue);
	STDMETHODIMP_(int) GetTrueHD();

	STDMETHODIMP SetSubEmptyPin(BOOL nValue);
	STDMETHODIMP_(BOOL) GetSubEmptyPin();

	STDMETHODIMP_(int) GetMPEGType();
};

class __declspec(uuid("1365BE7A-C86A-473C-9A41-C0A6E82C9FA3"))
	CMpegSourceFilter : public CMpegSplitterFilter
{
public:
	CMpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid = __uuidof(CMpegSourceFilter));
};

class CMpegSplitterOutputPin : public CBaseSplitterParserOutputPin
{
	CMpegSplitterFile::stream_type m_type;
public:
	CMpegSplitterOutputPin(CAtlArray<CMediaType>& mts, CMpegSplitterFile::stream_type type, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	int m_iHDMVSub;

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT QueuePacket(CAutoPtr<CPacket> p);

	STDMETHODIMP Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt);
};
