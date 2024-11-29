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

namespace MatroskaWriter
{
	class CID
	{
	protected:
		DWORD m_id;
		UINT64 HeaderSize(UINT64 len);
		HRESULT HeaderWrite(IStream* pStream);

	public:
		CID(DWORD id);
		DWORD GetID() const {
			return m_id;
		}
		virtual UINT64 Size(bool fWithHeader = true);
		virtual HRESULT Write(IStream* pStream);
	};

	class CLength final
	{
		UINT64 m_len;
	public:
		CLength(UINT64 len = 0) : m_len(len) {}
		operator UINT64() {
			return m_len;
		}
		UINT64 Size();
		HRESULT Write(IStream* pStream);
	};

	class CBinary : public std::vector<BYTE>, public CID
	{
	public:
		CBinary(DWORD id) : CID(id) {}
		CBinary& operator = (const CBinary& b) {
			assign(b.cbegin(), b.cend());
			return *this;
		}
		operator BYTE* () {
			return (BYTE*)data();
		}
		CBinary& Set(CStringA str) {
			resize(str.GetLength() + 1);
			strcpy_s((char*)data(), str.GetLength() + 1, str);
			return *this;
		}
		//CBinary& Set(CStringA str) {SetCount(str.GetLength()); memcpy((char*)GetData(), str, str.GetLength()); return *this;}
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class CANSI : public CStringA, public CID
	{
	public:
		CANSI(DWORD id) : CID(id) {}
		CANSI& Set(CStringA str) {
			CStringA::operator = (str);
			return *this;
		}
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class CUTF8 : public CStringW, public CID
	{
	public:
		CUTF8(DWORD id) : CID(id) {}
		CUTF8& Set(CStringW str) {
			CStringW::operator = (str);
			return *this;
		}
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	template<class T, class BASE>
	class CSimpleVar : public CID
	{
	protected:
		T m_val;
		bool m_fSet;
	public:
		explicit CSimpleVar(DWORD id, T val = 0) : CID(id), m_val(val) {
			m_fSet = !!val;
		}
		operator T() {
			return m_val;
		}
		BASE& Set(T val) {
			m_val = val;
			m_fSet = true;
			return (*(BASE*)this);
		}
		void UnSet() {
			m_fSet = false;
		}
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class CUInt : public CSimpleVar<UINT64, CUInt>
	{
	public:
		explicit CUInt(DWORD id, UINT64 val = 0) : CSimpleVar<UINT64, CUInt>(id, val) {}
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class CInt : public CSimpleVar<INT64, CInt>
	{
	public:
		explicit CInt(DWORD id, INT64 val = 0) : CSimpleVar<INT64, CInt>(id, val) {}
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class CByte : public CSimpleVar<BYTE, CByte>
	{
	public:
		explicit CByte(DWORD id, BYTE val = 0) : CSimpleVar<BYTE, CByte>(id, val) {}
	};

	class CShort : public CSimpleVar<short, CShort>
	{
	public:
		explicit CShort(DWORD id, short val = 0) : CSimpleVar<short, CShort>(id, val) {}
	};

	class CFloat : public CSimpleVar<float, CFloat>
	{
	public:
		explicit CFloat(DWORD id, float val = 0) : CSimpleVar<float, CFloat>(id, val) {}
	};

	template<class T>
	class CNode : public std::list<std::unique_ptr<T>>
	{
	public:
		UINT64 Size(bool fWithHeader = true) {
			UINT64 len = 0;
			for (const auto& item : *this) {
				len += item->Size(fWithHeader);
			}
			return len;
		}
		HRESULT Write(IStream* pStream) {
			for (const auto& item : *this) {
				HRESULT hr = item->Write(pStream);
				if (FAILED(hr)) {
					return hr;
				}
			}
			return S_OK;
		}
	};

	class EBML : public CID
	{
	public:
		CUInt EBMLVersion, EBMLReadVersion;
		CUInt EBMLMaxIDLength, EBMLMaxSizeLength;
		CANSI DocType;
		CUInt DocTypeVersion, DocTypeReadVersion;

		EBML(DWORD id = 0x1A45DFA3);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Info : public CID
	{
	public:
		CBinary SegmentUID, PrevUID, NextUID;
		CUTF8 SegmentFilename, PrevFilename, NextFilename;
		CUInt TimeCodeScale; // [ns], default: 1.000.000
		CFloat Duration;
		CInt DateUTC;
		CUTF8 Title, MuxingApp, WritingApp;

		Info(DWORD id = 0x1549A966);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Video : public CID
	{
	public:
		CUInt FlagInterlaced, StereoMode;
		CUInt PixelWidth, PixelHeight, DisplayWidth, DisplayHeight, DisplayUnit;
		CUInt AspectRatioType;
		CUInt ColourSpace;
		CFloat GammaValue;
		CFloat FramePerSec;

		Video(DWORD id = 0xE0);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Audio : public CID
	{
	public:
		CFloat SamplingFrequency;
		CFloat OutputSamplingFrequency;
		CUInt Channels;
		CBinary ChannelPositions;
		CUInt BitDepth;

		Audio(DWORD id = 0xE1);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class TrackEntry : public CID
	{
	public:
		enum {
			TypeVideo = 1,
			TypeAudio = 2,
			TypeComplex = 3,
			TypeLogo = 0x10,
			TypeSubtitle = 0x11,
			TypeControl = 0x20
		};
		CUInt TrackNumber, TrackUID, TrackType;
		CUInt FlagEnabled, FlagDefault, FlagLacing;
		CUInt MinCache, MaxCache;
		CUTF8 Name;
		CANSI Language;
		CBinary CodecID;
		CBinary CodecPrivate;
		CUTF8 CodecName;
		CUTF8 CodecSettings;
		CANSI CodecInfoURL;
		CANSI CodecDownloadURL;
		CUInt CodecDecodeAll;
		CUInt TrackOverlay;
		CUInt DefaultDuration;
		enum { NoDesc = 0, DescVideo = 1, DescAudio = 2 };
		int DescType;
		Video v;
		Audio a;

		TrackEntry(DWORD id = 0xAE);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Track : public CID
	{
	public:
		CNode<TrackEntry> TrackEntries;

		Track(DWORD id = 0x1654AE6B);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class CBlock : public CID
	{
	public:
		CLength TrackNumber;
		REFERENCE_TIME TimeCode, TimeCodeStop;
		CNode<CBinary> BlockData;

		CBlock(DWORD id = 0xA1);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class BlockGroup : public CID
	{
	public:
		CUInt BlockDuration;
		CUInt ReferencePriority;
		CInt ReferenceBlock;
		CInt ReferenceVirtual;
		CBinary CodecState;
		CBlock Block;
		//CNode<TimeSlice> TimeSlices;

		BlockGroup(DWORD id = 0xA0);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Cluster : public CID
	{
	public:
		CUInt TimeCode, Position, PrevSize;
		CNode<BlockGroup> BlockGroups;

		Cluster(DWORD id = 0x1F43B675);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	/*class CueReference : public CID
	{
	public:
		CUInt CueRefTime, CueRefCluster, CueRefNumber, CueRefCodecState;

		CueReference(DWORD id = 0xDB);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};*/

	class CueTrackPosition : public CID
	{
	public:
		CUInt CueTrack, CueClusterPosition, CueBlockNumber, CueCodecState;
		//CNode<CueReference> CueReferences;

		CueTrackPosition(DWORD id = 0xB7);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class CuePoint : public CID
	{
	public:
		CUInt CueTime;
		CNode<CueTrackPosition> CueTrackPositions;

		CuePoint(DWORD id = 0xBB);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Cue : public CID
	{
	public:
		CNode<CuePoint> CuePoints;

		Cue(DWORD id = 0x1C53BB6B);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class SeekID : public CID
	{
		CID m_cid;
	public:
		SeekID(DWORD id = 0x53AB);
		void Set(DWORD id) {
			m_cid = id;
		}
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class SeekHead : public CID
	{
	public:
		SeekID ID;
		CUInt Position;

		SeekHead(DWORD id = 0x4DBB);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Seek : public CID
	{
	public:
		CNode<SeekHead> SeekHeads;

		Seek(DWORD id = 0x114D9B74);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Segment : public CID
	{
	public:
		Segment(DWORD id = 0x18538067);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Tags : public CID
	{
	public:
		// TODO

		Tags(DWORD id = 0x1254C367);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};

	class Void : public CID
	{
		UINT64 m_len;
	public:
		Void(UINT64 len, DWORD id = 0xEC);
		UINT64 Size(bool fWithHeader = true) override;
		HRESULT Write(IStream* pStream) override;
	};
}
