/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2022 see Authors.txt
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
#include "../BaseSplitter/BaseSplitter.h"
#include "filters/transform/BaseVideoFilter/BaseVideoFilter.h"

#define RMSplitterName     L"MPC RealMedia Splitter"
#define RMSourceName       L"MPC RealMedia Source"

#pragma pack(push, 1)

namespace RMFF
{
	struct ChunkHdr {
		union {
			char id[4];
			UINT32 object_id;
		};
		UINT32 size;
		UINT16 object_version;
	};
	struct FileHdr {
		UINT32 version, nHeaders;
	};
	struct Properies {
		UINT32 maxBitRate, avgBitRate;
		UINT32 maxPacketSize, avgPacketSize, nPackets;
		UINT32 tDuration, tPreroll;
		UINT32 ptrIndex, ptrData;
		UINT16 nStreams;
		enum flags_t {
			PN_SAVE_ENABLED = 1,
			PN_PERFECT_PLAY_ENABLED = 2,
			PN_LIVE_BROADCAST = 4
		} flags;
	};
	struct MediaProperies {
		UINT16 stream;
		UINT32 maxBitRate, avgBitRate;
		UINT32 maxPacketSize, avgPacketSize;
		UINT32 tStart, tPreroll, tDuration;
		CStringA name, mime;
		std::vector<BYTE> typeSpecData;
		UINT32 width, height;
		bool interlaced, top_field_first;
	};
	struct ContentDesc {
		CStringA title, author, copyright, comment;
	};
	struct DataChunk {
		UINT64 pos;
		UINT32 nPackets, ptrNext;
	};
	struct MediaPacketHeader {
		UINT16 len, stream;
		UINT32 tStart;
		UINT8 reserved;
		enum flag_t {
			PN_RELIABLE_FLAG = 1,
			PN_KEYFRAME_FLAG = 2
		} flags; // UINT8
		std::vector<BYTE> pData;
	};
	struct IndexChunkHeader {
		UINT32 nIndices;
		UINT16 stream;
		UINT32 ptrNext;
	};
	struct IndexRecord {
		UINT32 tStart, ptrFilePos, packet;
	};
}

struct rvinfo {
	DWORD dwSize, fcc1, fcc2;
	WORD w, h, bpp;
	DWORD unk1, fps, type1, type2;
	BYTE morewh[14];
	void bswap();
};

struct rainfo3 {
	DWORD fourcc;           // Header signature ('.', 'r', 'a', 0xfd)
	WORD  version;          // Version (always 3)
	WORD  header_size;      // Header size, not including first 8 bytes
	BYTE  unknown[10];      // Unknown
	DWORD data_size;        // Data size
	void bswap();
};

struct rainfo {
	DWORD fourcc1;          // '.', 'r', 'a', 0xfd
	WORD  version1;         // 4 or 5
	WORD  unknown1;         // 00 000
	DWORD fourcc2;          // .ra4 or .ra5
	DWORD unknown2;         // ???
	WORD  version2;         // 4 or 5
	DWORD header_size;      // == 0x4e
	WORD  flavor;           // codec flavor id
	DWORD coded_frame_size; // coded frame size
	DWORD unknown3;         // big number
	DWORD unknown4;         // bigger number
	DWORD unknown5;         // yet another number
	WORD  sub_packet_h;
	WORD  frame_size;
	WORD  sub_packet_size;
	WORD  unknown6;         // 00 00
	void bswap();
};

struct rainfo4 : rainfo {
	WORD  sample_rate;
	WORD  unknown8;         // 0
	WORD  sample_size;
	WORD  channels;
	void bswap();
};

struct rainfo5 : rainfo {
	BYTE  unknown7[6];      // 0, srate, 0
	WORD  sample_rate;
	WORD  unknown8;         // 0
	WORD  sample_size;
	WORD  channels;
	DWORD genr;             // "genr"
	DWORD fourcc3;          // fourcc
	void bswap();
};

#pragma pack(pop)

class CRMFile : public CBaseSplitterFile
{
	// using CBaseSplitterFile::Read;

	HRESULT Init();
	void GetDimensions();

public:
	CRMFile(IAsyncReader* pAsyncReader, HRESULT& hr);

	template<typename T> HRESULT Read(T& var);
	HRESULT Read(RMFF::ChunkHdr& hdr);
	HRESULT Read(RMFF::MediaPacketHeader& mph, bool fFull = true);

	RMFF::FileHdr m_fh;
	RMFF::ContentDesc m_cd;
	RMFF::Properies m_p;
	CAutoPtrList<RMFF::MediaProperies> m_mps;
	CAutoPtrList<RMFF::DataChunk> m_dcs;
	CAutoPtrList<RMFF::IndexRecord> m_irs;

	struct subtitle {
		CStringA name, data;
	};
	std::list<subtitle> m_subs;

	int GetMasterStream();
};

class CRealMediaSplitterOutputPin : public CBaseSplitterOutputPin
{
private:
	struct segment {
		std::vector<BYTE> data;
		DWORD offset;
	};

	class CSegments : public CAutoPtrList<segment>, public CCritSec
	{
	public:
		REFERENCE_TIME rtStart;
		bool fDiscontinuity, fSyncPoint, fMerged;
		void Clear() {
			CAutoLock cAutoLock(this);
			rtStart = 0;
			fDiscontinuity = fSyncPoint = fMerged = false;
			RemoveAll();
		}
	} m_segments;

	CCritSec m_csQueue;

	HRESULT DeliverSegments();

protected:
	HRESULT DeliverPacket(CAutoPtr<CPacket> p);

public:
	CRealMediaSplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CRealMediaSplitterOutputPin();

	HRESULT DeliverEndFlush();
};

class __declspec(uuid("E21BE468-5C18-43EB-B0CC-DB93A847D769"))
	CRealMediaSplitterFilter : public CBaseSplitterFilter
{
protected:
	std::unique_ptr<CRMFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	POSITION m_seekpos;
	UINT32 m_seekpacket;
	UINT64 m_seekfilepos;

public:
	CRealMediaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CRealMediaSplitterFilter();

	// CBaseFilter

	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);

	// IKeyFrameInfo

	STDMETHODIMP_(HRESULT) GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP_(HRESULT) GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);
};

class __declspec(uuid("765035B3-5944-4A94-806B-20EE3415F26F"))
	CRealMediaSourceFilter : public CRealMediaSplitterFilter
{
public:
	CRealMediaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
