/*
 * (C) 2018 see Authors.txt
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

#define DVRSplitterName L"MPC DVR Splitter"
#define DVRSourceName   L"MPC DVR Source"

class __declspec(uuid("69D5E2F8-55D8-41B9-A95E-24A240F5F127"))
CDVRSplitterFilter : public CBaseSplitterFilter
{
	CAutoPtr<CBaseSplitterFileEx> m_pFile;

	__int64 m_startpos = 0;
	__int64 m_endpos   = 0;

	REFERENCE_TIME m_AvgTimePerFrame = 0;
	REFERENCE_TIME m_rtOffsetVideo = INVALID_TIME;
	REFERENCE_TIME m_rtOffsetAudio = INVALID_TIME;

	bool m_bHXVS = false;
	bool m_bDHAV = false;
	bool m_bCCTV = false;

	struct HXVSHeader {
		DWORD sync, size, pts, dummy;

		REFERENCE_TIME rt = 0;
		bool key_frame = false;
	};
	bool HXVSSync();
	bool HXVSReadHeader(HXVSHeader& hdr);

	struct DHAVHeader {
		DWORD sync;
		BYTE type, subtype, channel, frame_subnumber;
		DWORD frame_number, size, date;
		WORD pts;
		BYTE ext_size, checksum;

		bool key_frame = false;

		struct {
			SHORT width = 0;
			SHORT height = 0;
			BYTE codec = 0;
			BYTE frame_rate = 0;
		} video;
		struct {
			BYTE channels = 0;
			BYTE codec = 0;
			UINT sample_rate = 0;
		} audio;
	};
	REFERENCE_TIME m_rt_Seek = 0;
	bool DHAVSync(__int64& pos);
	bool DHAVReadHeader(DHAVHeader& hdr, const bool bParseExt = false);

	std::vector<SyncPoint> m_sps;

#pragma pack(push, 1)
	struct CCTVHeader {
		uint32_t sync;
		uint32_t stream_id;
		uint32_t width;
		uint32_t height;
		uint32_t fps;
		uint8_t unknown1[16];
		uint32_t key;
		uint8_t unknown2[4 + 4 + 4];
		uint64_t frame_num;
		uint32_t size;
		uint8_t unknown3[60];
		uint32_t end_sync;
	};
#pragma pack(pop)

	bool CCTVSync(__int64& pos);
	bool CCTVReadHeader(CCTVHeader& hdr);

protected:
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CDVRSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	// CBaseFilter

	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);

	// IKeyFrameInfo

	STDMETHODIMP GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);
};

class __declspec(uuid("E85619F1-2A32-4CF2-84E0-2F888E458EE1"))
CDVRSourceFilter : public CDVRSplitterFilter
{
public:
	CDVRSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);

	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);
};
