/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include "MatroskaFile.h"
#include "MatroskaSplitterSettingsWnd.h"
#include "../BaseSplitter/BaseSplitter.h"
#include <basestruct.h>
#include <IMediaSideData.h>
#include <ITrackInfo.h>
#include "../../filters/FilterInterfacesImpl.h"

#define MatroskaSplitterName L"MPC Matroska Splitter"
#define MatroskaSourceName   L"MPC Matroska Source"

using namespace MatroskaReader;

class CMatroskaSplitterOutputPin
	: public CBaseSplitterOutputPin
	, public CSubtitleStatus
{

public:
	CMatroskaSplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMatroskaSplitterOutputPin();

	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT QueuePacket(CAutoPtr<CPacket> p);
};

class __declspec(uuid("149D2E01-C32E-4939-80F6-C07B81015A7A"))
	CMatroskaSplitterFilter
	: public CBaseSplitterFilter
	, public ITrackInfo
	, public CExFilterInfoImpl
	, public IMatroskaSplitterFilter
	, public ISpecifyPropertyPages2
{
	class CMatroskaPacket : public CPacket
	{
	public:
		CAutoPtr<BlockGroup> bg;
	};

	void SetupChapters(LPCSTR lng, ChapterAtom* parent, int level = 0);
	void InstallFonts();
	void SendVorbisHeaderSample();

	CAutoPtr<CMatroskaNode> m_pSegment, m_pCluster, m_pBlock;

	REFERENCE_TIME m_Seek_rt;
	BOOL m_bSupportCueDuration;

	MediaSideDataHDR* m_MasterDataHDR;
	MediaSideDataHDRContentLightLevel* m_HDRContentLightLevel;
	ColorSpace* m_ColorSpace;

	int m_profile;
	int m_pix_fmt;
	int m_bits;
	int m_interlaced;
	int m_dtsonly = 0; // 0 - no, 1 - yes

	std::vector<SyncPoint> m_sps;

	std::map<DWORD, REFERENCE_TIME> m_lastDuration;
	std::map<DWORD, std::deque<CAutoPtr<CMatroskaPacket>>> m_packets;

	CCritSec m_csPackets;

	CCritSec m_csProps;
	bool m_bLoadEmbeddedFonts, m_bCalcDuration;

protected:
	CAutoPtr<CMatroskaFile> m_pFile;

	bool ReadFirtsBlock(std::vector<byte>& pData, TrackEntry* pTE);
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	std::map<DWORD, TrackEntry*> m_pTrackEntryMap;
	std::vector<TrackEntry* > m_pOrderedTrackArray;
	TrackEntry* GetTrackEntryAt(UINT aTrackIdx);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	HRESULT DeliverMatroskaPacket(TrackEntry* pTE, CAutoPtr<CMatroskaPacket> p);
	HRESULT DeliverMatroskaPacket(CAutoPtr<CMatroskaPacket> p, REFERENCE_TIME rtBlockDuration = 0);

public:
	CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CMatroskaSplitterFilter();

	bool m_hasHdmvDvbSubPin;
	bool IsHdmvDvbSubPinDrying();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter

	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);

	// IKeyFrameInfo

	STDMETHODIMP GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);

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

	// IMatroskaSplitterFilter

	STDMETHODIMP Apply();
	STDMETHODIMP SetLoadEmbeddedFonts(BOOL nValue);
	STDMETHODIMP_(BOOL) GetLoadEmbeddedFonts();
	STDMETHODIMP SetCalcDuration(BOOL nValue);
	STDMETHODIMP_(BOOL) GetCalcDuration();

	// IExFilterInfo
	STDMETHODIMP GetInt(LPCSTR field, int *value) override;
	STDMETHODIMP GetBin(LPCSTR field, LPVOID *value, unsigned *size) override;
};

class __declspec(uuid("0A68C3B5-9164-4a54-AFAF-995B2FF0E0D4"))
	CMatroskaSourceFilter : public CMatroskaSplitterFilter
{
public:
	CMatroskaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
