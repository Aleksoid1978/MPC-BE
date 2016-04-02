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
#include "OggFile.h"
#include "../BaseSplitter/BaseSplitter.h"

#define OggSplitterName L"MPC Ogg Splitter"
#define OggSourceName   L"MPC Ogg Source"

class COggSplitterOutputPin : public CBaseSplitterOutputPin
{
	class CComment
	{
	public:
		CStringW m_key, m_value;
		CComment(CStringW key, CStringW value) : m_key(key), m_value(value) {
			m_key.MakeUpper();
		}
	};

	CAutoPtrList<CComment> m_pComments;

protected:
	CCritSec m_csPackets;
	CAutoPtrList<CPacket> m_packets;
	CAtlArray<BYTE> m_lastPacketData;
	DWORD m_lastseqnum;
	REFERENCE_TIME m_rtLast;
	bool m_fSetKeyFrame;

	void ResetState(DWORD seqnum = -1);

public:
	COggSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	void AddComment(BYTE* p, int len);
	CStringW GetComment(CStringW key);

	void HandlePacket(DWORD TrackNumber, BYTE* pData, int len);
	HRESULT UnpackPage(OggPage& page);
	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len) PURE;
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position) PURE;
	CAutoPtr<CPacket> GetPacket();

	HRESULT DeliverEndFlush();
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};

class COggVorbisOutputPin : public COggSplitterOutputPin
{
	CAutoPtrList<CPacket> m_initpackets;

	DWORD m_audio_sample_rate;
	DWORD m_blocksize[2], m_lastblocksize;
	CAtlArray<bool> m_blockflags;

	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

	HRESULT DeliverPacket(CAutoPtr<CPacket> p);
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

public:
	COggVorbisOutputPin(OggVorbisIdHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT UnpackInitPage(OggPage& page);
	bool IsInitialized() {
		return m_initpackets.GetCount() >= 3;
	}
};

class COggFlacOutputPin : public COggSplitterOutputPin
{
	CAutoPtrList<CPacket> m_initpackets;

	int		m_nSamplesPerSec;
	int		m_nChannels;
	WORD	m_wBitsPerSample;
	int		m_nAvgBytesPerSec;

	DWORD m_blocksize[2], m_lastblocksize;
	CAtlArray<bool> m_blockflags;

	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

public:
	COggFlacOutputPin(BYTE* h, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	bool IsInitialized() {
		return m_initpackets.GetCount() >= 3;
	}
};

class COggDirectShowOutputPin : public COggSplitterOutputPin
{
	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggDirectShowOutputPin(AM_MEDIA_TYPE* pmt, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggStreamOutputPin : public COggSplitterOutputPin
{
	__int64 m_time_unit, m_samples_per_unit;
	DWORD m_default_len;

	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggStreamOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggVideoOutputPin : public COggStreamOutputPin
{
public:
	COggVideoOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggAudioOutputPin : public COggStreamOutputPin
{
public:
	COggAudioOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggTextOutputPin : public COggStreamOutputPin
{
public:
	COggTextOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggKateOutputPin : public COggStreamOutputPin
{
public:
	COggKateOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggTheoraOutputPin : public COggSplitterOutputPin
{
	CAutoPtrList<CPacket>	m_initpackets;
	LONG					m_KfgShift;
	LONG					m_KfgMask;
	UINT					m_nVersion;
	REFERENCE_TIME			m_rtAvgTimePerFrame;

	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggTheoraOutputPin(OggPage& page, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT UnpackInitPage(OggPage& page);
	bool IsInitialized() {
		return m_initpackets.GetCount() >= 3;
	}
};

class COggDiracOutputPin : public COggSplitterOutputPin
{
	REFERENCE_TIME	m_rtAvgTimePerFrame;
	bool			m_bOldDirac;
	bool			m_IsInitialized;

	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggDiracOutputPin(BYTE* p, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT UnpackInitPage(OggPage& page);
	bool IsInitialized() {
		return m_IsInitialized;
	}
	HRESULT InitDirac(BYTE* p, int nCount);
};

class COggOpusOutputPin : public COggSplitterOutputPin
{
	int  m_SampleRate;
	WORD m_Preskip;

	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggOpusOutputPin(BYTE* h, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggSpeexOutputPin : public COggSplitterOutputPin
{
	int m_SampleRate;

	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggSpeexOutputPin(BYTE* h, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggVP8OutputPin : public COggSplitterOutputPin
{
	REFERENCE_TIME m_rtAvgTimePerFrame;

	virtual HRESULT UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggVP8OutputPin(BYTE* h, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class __declspec(uuid("9FF48807-E133-40AA-826F-9B2959E5232D"))
	COggSplitterFilter : public CBaseSplitterFilter
{
protected:
	CAutoPtr<COggFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	DWORD m_bitstream_serial_number_start, m_bitstream_serial_number_last;
	DWORD m_bitstream_serial_number_Video = DWORD_MAX;

	BOOL bIsTheoraPresent;
	BOOL bIsVP8Present;

public:
	COggSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~COggSplitterFilter();

	// CBaseFilter

	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);
};

class __declspec(uuid("6D3688CE-3E9D-42F4-92CA-8A11119D25CD"))
	COggSourceFilter : public COggSplitterFilter
{
public:
	COggSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
