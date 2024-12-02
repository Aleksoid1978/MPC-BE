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

#include "../BaseSplitter/BaseSplitter.h"

/* top-level master-IDs */
#define EBML_ID_HEADER			0x1A45DFA3

/* toplevel segment */
#define MATROSKA_ID_SEGMENT		0x18538067

/* Matroska top-level master IDs */
#define MATROSKA_ID_INFO		0x1549A966
#define MATROSKA_ID_TRACKS		0x1654AE6B
#define MATROSKA_ID_CUES		0x1C53BB6B
#define MATROSKA_ID_TAGS		0x1254C367
#define MATROSKA_ID_SEEKHEAD	0x114D9B74
#define MATROSKA_ID_ATTACHMENTS	0x1941A469
#define MATROSKA_ID_CLUSTER		0x1F43B675
#define MATROSKA_ID_CHAPTERS	0x1043A770

/* IDs in the cluster master */
#define MATROSKA_ID_BLOCKGROUP	0xA0
#define MATROSKA_ID_SIMPLEBLOCK	0xA3

namespace MatroskaReader
{
	class CMatroskaNode;

	class CANSI : public CStringA
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN);
	};
	class CUTF8 : public CStringW
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN);
	};

	template<class T, class BASE>
	class CSimpleVar
	{
	protected:
		T m_val;
		bool m_fValid;
	public:
		CSimpleVar(T val = 0) : m_val(val), m_fValid(false) {}
		BASE& operator = (const BASE& v) {
			m_val = v.m_val;
			m_fValid = true;
			return *this;
		}
		BASE& operator = (T val) {
			m_val = val;
			m_fValid = true;
			return *this;
		}
		operator T() const {
			return m_val;
		}
		BASE& Set(T val) {
			m_val = val;
			m_fValid = true;
			return (*(BASE*)this);
		}
		bool IsValid() const {
			return m_fValid;
		}
		virtual HRESULT Parse(CMatroskaNode* pMN);
	};

	class CUInt : public CSimpleVar<UINT64, CUInt>
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN);
	};
	class CInt : public CSimpleVar<INT64, CInt>
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN);
	};
	class CByte : public CSimpleVar<BYTE, CByte> {};
	class CShort : public CSimpleVar<short, CShort> {};
	class CFloat : public CSimpleVar<double, CFloat>
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN);
	};
	class CID : public CSimpleVar<DWORD, CID>
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN);
	};
	class CLength : public CSimpleVar<UINT64, CLength>
	{
		bool m_fSigned;
	public:
		CLength(bool fSigned = false) : m_fSigned(fSigned) {} HRESULT Parse(CMatroskaNode* pMN);
	};
	class CSignedLength : public CLength
	{
	public:
		CSignedLength() : CLength(true) {}
	};

	class ContentCompression;

	class CBinary : public std::vector<BYTE>
	{
	public:
		CStringA ToString() {
			return CStringA((LPCSTR)data(), (int)size());
		}
		bool Compress(ContentCompression& cc);
		bool Decompress(ContentCompression& cc);
		HRESULT Parse(CMatroskaNode* pMN);
	};

	template<class T>
	class CNode : public std::list<std::unique_ptr<T>>
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class EBML
	{
	public:
		CUInt EBMLVersion, EBMLReadVersion;
		CUInt EBMLMaxIDLength, EBMLMaxSizeLength;
		CANSI DocType;
		CUInt DocTypeVersion, DocTypeReadVersion;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Info
	{
	public:
		CBinary SegmentUID, PrevUID, NextUID;
		CUTF8 SegmentFilename, PrevFilename, NextFilename;
		CUInt TimeCodeScale; // [ns], default: 1.000.000
		CFloat Duration;
		CInt DateUTC;
		CUTF8 Title, MuxingApp, WritingApp;

		Info() {
			TimeCodeScale.Set(1000000ui64);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class SeekHead
	{
	public:
		CID SeekID;
		CUInt SeekPosition;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Seek
	{
	public:
		CNode<SeekHead> SeekHeads;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class TimeSlice
	{
	public:
		CUInt LaceNumber, FrameNumber;
		CUInt Delay, Duration;

		TimeSlice() {
			LaceNumber.Set(0);
			FrameNumber.Set(0);
			Delay.Set(0);
			Duration.Set(0);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class SimpleBlock
	{
	public:
		CLength TrackNumber;
		CInt TimeCode;
		CByte Lacing;
		std::list<std::unique_ptr<CBinary>> BlockData;

		HRESULT Parse(CMatroskaNode* pMN, bool fFull);
	};

	class BlockMore
	{
	public:
		CInt BlockAddID;
		CBinary BlockAdditional;

		BlockMore() {
			BlockAddID.Set(1);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class BlockAdditions
	{
	public:
		CNode<BlockMore> bm;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class BlockGroup
	{
	public:
		SimpleBlock Block;
		//				BlockVirtual
		CUInt BlockDuration;
		CUInt ReferencePriority;
		CInt ReferenceBlock;
		CInt ReferenceVirtual;
		CBinary CodecState;
		CNode<TimeSlice> TimeSlices;
		BlockAdditions ba;

		HRESULT Parse(CMatroskaNode* pMN, bool fFull);
	};

	class CBlockGroupNode : public CNode<BlockGroup>
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN, bool fFull);
	};

	class CSimpleBlockNode : public CNode<SimpleBlock>
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN, bool fFull);
	};

	class Cluster
	{
	public:
		CUInt TimeCode, Position, PrevSize;
		CBlockGroupNode BlockGroups;
		CSimpleBlockNode SimpleBlocks;

		HRESULT Parse(CMatroskaNode* pMN);
		HRESULT ParseTimeCode(CMatroskaNode* pMN);
	};

	class CMasteringMetadata
	{
		bool m_bValid = false;
	public:
		CFloat PrimaryRChromaticityX, PrimaryRChromaticityY;
		CFloat PrimaryGChromaticityX, PrimaryGChromaticityY;
		CFloat PrimaryBChromaticityX, PrimaryBChromaticityY;
		CFloat WhitePointChromaticityX, WhitePointChromaticityY;
		CFloat LuminanceMax, LuminanceMin;

		CMasteringMetadata() {
			PrimaryRChromaticityX.Set(0.0); PrimaryRChromaticityY.Set(0.0);
			PrimaryGChromaticityX.Set(0.0); PrimaryGChromaticityY.Set(0.0);
			PrimaryBChromaticityX.Set(0.0); PrimaryBChromaticityX.Set(0.0);
			WhitePointChromaticityX.Set(0.0); WhitePointChromaticityY.Set(0.0);
			LuminanceMax.Set(0.0); LuminanceMin.Set(0.0);
		}
		bool IsValid() const {
			return m_bValid;
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CColour
	{
		bool m_bValid = false;
	public:
		CUInt MatrixCoefficients;
		CUInt ChromaSitingHorz;
		CUInt ChromaSitingVert;
		CUInt Range;
		CUInt TransferCharacteristics;
		CUInt Primaries;
		CUInt MaxCLL;
		CUInt MaxFALL;
		CMasteringMetadata MasteringMetadata;

		CColour() {
			MatrixCoefficients.Set(0);
			ChromaSitingHorz.Set(0); ChromaSitingHorz.Set(0);
			Range.Set(0);
			TransferCharacteristics.Set(0);
			Primaries.Set(0);
		}
		bool IsValid() const {
			return m_bValid;
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CProjection
	{
	public:
		CUInt   ProjectionType;
		CBinary ProjectionPrivate;
		CFloat  ProjectionPoseYaw;
		CFloat  ProjectionPosePitch;
		CFloat  ProjectionPoseRoll;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Video
	{
	public:
		CUInt FlagInterlaced, FieldOrder, StereoMode;
		CUInt PixelWidth, PixelHeight, DisplayWidth, DisplayHeight, DisplayUnit;
		CUInt VideoPixelCropBottom, VideoPixelCropTop, VideoPixelCropLeft, VideoPixelCropRight;
		CUInt AspectRatioType;
		CUInt ColourSpace;
		CFloat GammaValue;
		CFloat FrameRate; // Number of frames per second. Informational only.

		CColour Colour;
		CProjection Projection;

		Video() {
			FlagInterlaced.Set(0);
			FieldOrder.Set(-1);
			StereoMode.Set(0);
			DisplayUnit.Set(0);
			AspectRatioType.Set(0);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Audio
	{
	public:
		CFloat SamplingFrequency;
		CFloat OutputSamplingFrequency;
		CUInt Channels;
		CBinary ChannelPositions;
		CUInt BitDepth;

		Audio() {
			SamplingFrequency.Set(8000.0);
			Channels.Set(1);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class ContentCompression
	{
	public:
		CUInt ContentCompAlgo;
		enum {ZLIB, BZLIB, LZO1X, HDRSTRIP};
		CBinary ContentCompSettings;

		ContentCompression() {
			ContentCompAlgo.Set(ZLIB);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class ContentEncryption
	{
	public:
		CUInt ContentEncAlgo;
		enum {UNKE, DES, THREEDES, TWOFISH, BLOWFISH, AES};
		CBinary ContentEncKeyID, ContentSignature, ContentSigKeyID;
		CUInt ContentSigAlgo;
		enum {UNKS, RSA};
		CUInt ContentSigHashAlgo;
		enum {UNKSH, SHA1_160, MD5};

		ContentEncryption() {
			ContentEncAlgo.Set(0);
			ContentSigAlgo.Set(0);
			ContentSigHashAlgo.Set(0);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class ContentEncoding
	{
	public:
		CUInt ContentEncodingOrder;
		CUInt ContentEncodingScope;
		enum {AllFrameContents = 1, TracksPrivateData = 2};
		CUInt ContentEncodingType;
		enum {Compression, Encryption};
		ContentCompression cc;
		ContentEncryption ce;

		ContentEncoding() {
			ContentEncodingOrder.Set(0);
			ContentEncodingScope.Set(AllFrameContents);
			ContentEncodingType.Set(Compression);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class ContentEncodings
	{
	public:
		CNode<ContentEncoding> ce;

		ContentEncodings() {}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class BlockAdditionMapping
	{
	public:
		CUInt Value;
		CANSI Name;
		CUInt Type;
		CBinary ExtraData;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class TrackEntry
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
		CUInt FlagEnabled, FlagDefault, FlagLacing, FlagForced;
		CUInt MinCache, MaxCache;
		CUTF8 Name;
		CANSI Language{ "eng" };
		CANSI LanguageBCP47;
		CBinary CodecID;
		CBinary CodecPrivate;
		CUTF8 CodecName;
		CUTF8 CodecSettings;
		CANSI CodecInfoURL;
		CANSI CodecDownloadURL;
		CUInt CodecDecodeAll;
		CUInt TrackOverlay;
		CUInt DefaultDuration;
		CUInt MaxBlockAdditionID;
		CFloat TrackTimecodeScale;
		enum {NoDesc = 0, DescVideo = 1, DescAudio = 2};
		unsigned int DescType;
		Video v;
		Audio a;
		ContentEncodings ces;
		CNode<BlockAdditionMapping> BlockAdditionMappings;

		TrackEntry() {
			DescType = NoDesc;
			FlagEnabled.Set(1);
			FlagDefault.Set(1);
			FlagLacing.Set(1);
			FlagForced.Set(0);
			MinCache.Set(0);
			TrackTimecodeScale.Set(1.0f);
			MaxBlockAdditionID.Set(0);
			CodecDecodeAll.Set(1);
		}
		HRESULT Parse(CMatroskaNode* pMN);

		bool Expand(CBinary& data, UINT64 Scope);
	};

	class Track
	{
	public:
		CNode<TrackEntry> TrackEntries;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CueReference
	{
	public:
		CUInt CueRefTime, CueRefCluster, CueRefNumber, CueRefCodecState;

		CueReference() {
			CueRefNumber.Set(1);
			CueRefCodecState.Set(0);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CueTrackPosition
	{
	public:
		CUInt CueTrack, CueDuration, CueRelativePosition, CueClusterPosition, CueBlockNumber, CueCodecState;
		CNode<CueReference> CueReferences;

		CueTrackPosition() {
			CueBlockNumber.Set(1);
			CueCodecState.Set(0);
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CuePoint
	{
	public:
		CUInt CueTime;
		CNode<CueTrackPosition> CueTrackPositions;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Cue
	{
	public:
		CNode<CuePoint> CuePoints;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class AttachedFile
	{
	public:
		CUTF8 FileDescription;
		CUTF8 FileName;
		CANSI FileMimeType;
		UINT64 FileDataPos, FileDataLen; // BYTE* FileData
		CUInt FileUID;

		AttachedFile() {
			FileDataPos = FileDataLen = 0;
		}
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Attachment
	{
	public:
		CNode<AttachedFile> AttachedFiles;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class ChapterDisplay
	{
	public:
		CUTF8 ChapString;
		CANSI ChapLanguage{ "eng" };
		CANSI ChapLanguageBCP47;
		CANSI ChapCountry;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class ChapterAtom
	{
	public:
		CUInt ChapterUID;
		CUInt ChapterTimeStart, ChapterTimeEnd, ChapterFlagHidden, ChapterFlagEnabled;
		//CNode<CUInt> ChapterTracks; // TODO
		CNode<ChapterDisplay> ChapterDisplays;
		CNode<ChapterAtom> ChapterAtoms;

		ChapterAtom() {
			ChapterUID.Set(0);// 0 = not set (ChapUID zero not allow by Matroska specs)
			ChapterFlagHidden.Set(0);
			ChapterFlagEnabled.Set(1);
		}
		HRESULT Parse(CMatroskaNode* pMN);
		ChapterAtom* FindChapterAtom(UINT64 id);
	};

	class EditionEntry : public ChapterAtom
	{
	public:
		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Chapter
	{
	public:
		CNode<EditionEntry> EditionEntries;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Targets
	{
	public:
		CUInt TargetTypeValue;
		CANSI TargetType;
		CUInt TrackUID;
		CUInt EditionUID;
		CUInt ChapterUID;
		CUInt AttachmentUID;

		Targets() {
			TargetTypeValue.Set(50);
		}

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class SimpleTag
	{
	public:
		CUTF8 TagName;
		CANSI TagLanguage{ "und" };
		CANSI TagLanguageBCP47;
		CUTF8 TagString;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Tag
	{
	public:
		CNode<SimpleTag> SimpleTag;
		CNode<Targets> Targets;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Tags
	{
	public:
		CNode<Tag> Tag;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class Segment
	{
	public:
		UINT64 pos, len;
		Info SegmentInfo;
		CNode<Seek> MetaSeekInfo;
		CNode<Cluster> Clusters;
		CNode<Track> Tracks;
		CNode<Cue> Cues;
		CNode<Attachment> Attachments;
		CNode<Chapter> Chapters;
		CNode<Tags> Tags;

		HRESULT Parse(CMatroskaNode* pMN);
		HRESULT ParseMinimal(CMatroskaNode* pMN);

		UINT64 GetMasterTrack();

		REFERENCE_TIME GetRefTime(INT64 t) const {
			return llMulDiv(t, SegmentInfo.TimeCodeScale, 100, 0);
		}
		ChapterAtom* FindChapterAtom(UINT64 id, int nEditionEntry = 0);
	};

	class CMatroskaFile : public CBaseSplitterFileEx
	{
	public:
		CMatroskaFile(IAsyncReader* pAsyncReader, HRESULT& hr);
		virtual ~CMatroskaFile() {}

		HRESULT Init();

		//using CBaseSplitterFile::Read;
		template <class T> HRESULT Read(T& var);

		EBML m_ebml;
		Segment m_segment;
		REFERENCE_TIME m_rtOffset;

		HRESULT Parse(CMatroskaNode* pMN);
	};

	class CMatroskaNode
	{
		CMatroskaNode* m_pParent;
		CMatroskaFile* m_pMF;

		bool Resync();

	public:
		CID m_id;
		CLength m_len;
		UINT64 m_filepos, m_start;

		HRESULT Parse();

	public:
		CMatroskaNode(CMatroskaFile* pMF); // creates the root
		CMatroskaNode(CMatroskaNode* pParent);

		CMatroskaNode* Parent() {
			return m_pParent;
		}
		std::unique_ptr<CMatroskaNode> Child(DWORD id = 0, bool fSearch = true);
		bool Next(bool fSame = false);
		bool Find(DWORD id, bool fSearch = true);

		UINT64 FindPos(DWORD id, UINT64 start = 0);

		void SeekTo(UINT64 pos);
		UINT64 GetPos(), GetLength();
		template <class T> HRESULT Read(T& var);
		HRESULT Read(BYTE* pData, UINT64 len);

		std::unique_ptr<CMatroskaNode> Copy();

		std::unique_ptr<CMatroskaNode> GetFirstBlock();
		bool NextBlock();

		bool IsRandomAccess() {
			return m_pMF->IsRandomAccess();
		}
	};
}
