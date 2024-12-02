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

#include "stdafx.h"
#include "MatroskaFile.h"
#include <ExtLib/zlib/zlib.h>

#define DOCTYPE        "matroska"
#define DOCTYPE_WEBM   "webm"
#define DOCTYPEVERSION 2

using namespace MatroskaReader;

#define BeginChunk	\
	CheckPointer(pMN0, E_POINTER); \
\
	std::unique_ptr<CMatroskaNode> pMNChild = pMN0->Child(); \
	CMatroskaNode* pMN = pMNChild.get(); \
	if (!pMN) return S_FALSE; \
\
	do \
	{ \
		switch (pMN->m_id) \
		{ \

#define EndChunk \
		} \
	} \
	while (pMN->Next()); \
\
	return S_OK; \

static void bswap(BYTE* s, int len)
{
	for (BYTE* d = s + len - 1; s < d; s++, d--) {
		*s ^= *d, *d ^= *s, *s ^= *d;
	}
}

//
// CMatroskaFile
//

CMatroskaFile::CMatroskaFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE | FM_FILE_DL)
	, m_rtOffset(0)
{
	if (FAILED(hr)) {
		return;
	}
	hr = Init();
}

HRESULT CMatroskaFile::Init()
{
	DWORD dw;
	if (FAILED(Read(dw)) || dw != EBML_ID_HEADER) {
		return E_FAIL;
	}

	CMatroskaNode Root(this);
	if (FAILED(Parse(&Root))) {
		return E_FAIL;
	}

	const auto& s = m_segment;

	BOOL bCanPlayback = FALSE;
	for (const auto& pT : s.Tracks) {
		for (const auto& pTE : pT->TrackEntries) {
			if (pTE->TrackType == TrackEntry::TypeVideo || pTE->TrackType == TrackEntry::TypeAudio) {
				bCanPlayback = TRUE;
				break;
			}
		}

		if (bCanPlayback) {
			break;
		}
	}

	if (bCanPlayback) {
		std::unique_ptr<CMatroskaNode> pSegment = Root.Child(MATROSKA_ID_SEGMENT);
		if (pSegment) {
			std::unique_ptr<CMatroskaNode> pCluster = pSegment->Child(MATROSKA_ID_CLUSTER);
			if (pCluster) {
				Cluster c0;
				c0.ParseTimeCode(pCluster.get());
				m_rtOffset = m_segment.GetRefTime(c0.TimeCode);
			}
		}
	}

	return S_OK;
}

template <class T>
HRESULT CMatroskaFile::Read(T& var)
{
	HRESULT hr = ByteRead((BYTE*)&var, sizeof(var));
	if (S_OK == hr) {
		bswap((BYTE*)&var, sizeof(var));
	}
	return hr;
}

HRESULT CMatroskaFile::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
case EBML_ID_HEADER:
	m_ebml.Parse(pMN);
	if ((m_ebml.DocType != DOCTYPE || m_ebml.DocTypeReadVersion > DOCTYPEVERSION) &&  m_ebml.DocType != DOCTYPE_WEBM) {
		return E_FAIL;
	}
	break;
case MATROSKA_ID_SEGMENT:
	if (m_segment.SegmentInfo.SegmentUID.empty()) {
		m_segment.ParseMinimal(pMN);
	}
	break;
	EndChunk
}

//

HRESULT EBML::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x4286:
		EBMLVersion.Parse(pMN);
		break;
	case 0x42F7:
		EBMLReadVersion.Parse(pMN);
		break;
	case 0x42F2:
		EBMLMaxIDLength.Parse(pMN);
		break;
	case 0x42F3:
		EBMLMaxSizeLength.Parse(pMN);
		break;
	case 0x4282:
		DocType.Parse(pMN);
		break;
	case 0x4287:
		DocTypeVersion.Parse(pMN);
		break;
	case 0x4285:
		DocTypeReadVersion.Parse(pMN);
		break;
	EndChunk
}

HRESULT Segment::Parse(CMatroskaNode* pMN0)
{
	pos = pMN0->GetPos();

	BeginChunk
	case MATROSKA_ID_INFO:
		SegmentInfo.Parse(pMN);
		break;
	case MATROSKA_ID_SEEKHEAD:
		MetaSeekInfo.Parse(pMN);
		break;
	case MATROSKA_ID_TRACKS:
		Tracks.Parse(pMN);
		break;
	case MATROSKA_ID_CLUSTER:
		Clusters.Parse(pMN);
		break;
	case MATROSKA_ID_CUES:
		Cues.Parse(pMN);
		break;
	case MATROSKA_ID_ATTACHMENTS:
		Attachments.Parse(pMN);
		break;
	case MATROSKA_ID_CHAPTERS:
		Chapters.Parse(pMN);
		break;
	case MATROSKA_ID_TAGS:
		Tags.Parse(pMN);
		break;
	EndChunk
}

HRESULT Segment::ParseMinimal(CMatroskaNode* pMN0)
{
	CheckPointer(pMN0, E_POINTER);

	pos = pMN0->GetPos();
	len = pMN0->m_len;

	std::unique_ptr<CMatroskaNode> pMN = pMN0->Child();
	if (!pMN) {
		return S_FALSE;
	}

	// read sequentially all the elements to the first cluster
	do {
		switch (pMN->m_id) {
			case MATROSKA_ID_INFO:
				SegmentInfo.Parse(pMN.get());
				break;
			case MATROSKA_ID_SEEKHEAD:
				MetaSeekInfo.Parse(pMN.get());
				break;
			case MATROSKA_ID_TRACKS:
				Tracks.Parse(pMN.get());
				break;
			case MATROSKA_ID_CUES:
				Cues.Parse(pMN.get());
				break;
			case MATROSKA_ID_CHAPTERS:
				Chapters.Parse(pMN.get());
				break;
			case MATROSKA_ID_ATTACHMENTS:
				Attachments.Parse(pMN.get());
				break;
			case MATROSKA_ID_TAGS:
				Tags.Parse(pMN.get());
				break;
			default:
				break;
		}
	} while (pMN->m_id != MATROSKA_ID_CLUSTER && pMN->Next());

	if (!pMN->IsRandomAccess()) {
		return S_OK;
	}

	// parse SeekHead (MetaSeekInfo) to get the position of the missing elements
	while (UINT64 pos = pMN->FindPos(MATROSKA_ID_SEEKHEAD, pMN->GetPos())) {
		pMN->SeekTo(pos);
		if (FAILED(pMN->Parse()) || (pMN->m_filepos + pMN->m_len) > pMN->GetLength()) {
			break; // a broken file
		}
		MetaSeekInfo.Parse(pMN.get());
	}

	// trying to read items that have not been read before
	if (Tracks.empty() && (pMN = pMN0->Child(MATROSKA_ID_TRACKS, false))) {
		Tracks.Parse(pMN.get());
	}
	if (Cues.empty() && (pMN = pMN0->Child(MATROSKA_ID_CUES, false))) {
		do {
			Cues.Parse(pMN.get());
		} while (pMN->Next(true));
	}
	if (Chapters.empty() && (pMN = pMN0->Child(MATROSKA_ID_CHAPTERS, false))) {
		Chapters.Parse(pMN.get());
	}
	if (Attachments.empty() && (pMN = pMN0->Child(MATROSKA_ID_ATTACHMENTS, false))) {
		Attachments.Parse(pMN.get());
	}
	if (Tags.empty() && (pMN = pMN0->Child(MATROSKA_ID_TAGS, false))) {
		Tags.Parse(pMN.get());
	}

	return S_OK;
}

UINT64 Segment::GetMasterTrack()
{
	UINT64 TrackNumber = 0, AltTrackNumber = 0;

	for (const auto& pT : Tracks) {
		for (const auto& pTE : pT->TrackEntries) {
			if (pTE->TrackType == TrackEntry::TypeVideo) {
				TrackNumber = pTE->TrackNumber;
				break;
			} else if (pTE->TrackType == TrackEntry::TypeAudio && AltTrackNumber == 0) {
				AltTrackNumber = pTE->TrackNumber;
			}
		}

		if (TrackNumber) {
			break;
		}
	}

	if (TrackNumber == 0) {
		TrackNumber = AltTrackNumber;
	}
	if (TrackNumber == 0) {
		TrackNumber = 1;
	}

	return TrackNumber;
}

ChapterAtom* ChapterAtom::FindChapterAtom(UINT64 id)
{
	if (ChapterUID == id) {
		return this;
	}

	for (const auto& ca : ChapterAtoms) {
		const auto& caFind = ca->FindChapterAtom(id);
		if (caFind) {
			return caFind;
		}
	}

	return nullptr;
}

ChapterAtom* Segment::FindChapterAtom(UINT64 id, int nEditionEntry)
{
	for (const auto& ch : Chapters) {
		for (const auto& ee : ch->EditionEntries) {
			if (nEditionEntry-- == 0) {
				return id == 0 ? ee.get() : ee->FindChapterAtom(id);
			}
		}
	}

	return nullptr;
}

HRESULT Info::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x73A4:
		SegmentUID.Parse(pMN);
		break;
	case 0x7384:
		SegmentFilename.Parse(pMN);
		break;
	case 0x3CB923:
		PrevUID.Parse(pMN);
		break;
	case 0x3C83AB:
		PrevFilename.Parse(pMN);
		break;
	case 0x3EB923:
		NextUID.Parse(pMN);
		break;
	case 0x3E83BB:
		NextFilename.Parse(pMN);
		break;
	case 0x2AD7B1:
		TimeCodeScale.Parse(pMN);
		break;
	case 0x4489:
		Duration.Parse(pMN);
		break;
	case 0x4461:
		DateUTC.Parse(pMN);
		break;
	case 0x7BA9:
		Title.Parse(pMN);
		break;
	case 0x4D80:
		MuxingApp.Parse(pMN);
		break;
	case 0x5741:
		WritingApp.Parse(pMN);
		break;
	EndChunk
}

HRESULT Seek::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x4DBB:
		SeekHeads.Parse(pMN);
		break;
	EndChunk
}

HRESULT SeekHead::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x53AB:
		SeekID.Parse(pMN);
		break;
	case 0x53AC:
		SeekPosition.Parse(pMN);
		break;
	EndChunk
}

HRESULT Track::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xAE:
		TrackEntries.Parse(pMN);
		break;
	EndChunk
}


HRESULT BlockAdditionMapping::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x41F0:
		Value.Parse(pMN);
		break;
	case 0x41A4:
		Name.Parse(pMN);
		break;
	case 0x41E7:
		Type.Parse(pMN);
		break;
	case 0x41ED:
		ExtraData.Parse(pMN);
		break;
	EndChunk
}

HRESULT TrackEntry::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xD7:
		TrackNumber.Parse(pMN);
		break;
	case 0x73C5:
		TrackUID.Parse(pMN);
		break;
	case 0x83:
		TrackType.Parse(pMN);
		break;
	case 0xB9:
		FlagEnabled.Parse(pMN);
		break;
	case 0x88:
		FlagDefault.Parse(pMN);
		break;
	case 0x9C:
		FlagLacing.Parse(pMN);
		break;
	case 0x55AA:
		FlagForced.Parse(pMN);
		break;
	case 0x6DE7:
		MinCache.Parse(pMN);
		break;
	case 0x6DF8:
		MaxCache.Parse(pMN);
		break;
	case 0x536E:
		Name.Parse(pMN);
		break;
	case 0x22B59C:
		Language.Parse(pMN);
		break;
	case 0x22B59D:
		LanguageBCP47.Parse(pMN);
		break;
	case 0x86:
		CodecID.Parse(pMN);
		break;
	case 0x63A2:
		CodecPrivate.Parse(pMN);
		break;
	case 0x258688:
		CodecName.Parse(pMN);
		break;
	case 0x3A9697:
		CodecSettings.Parse(pMN);
		break;
	case 0x3B4040:
		CodecInfoURL.Parse(pMN);
		break;
	case 0x26B240:
		CodecDownloadURL.Parse(pMN);
		break;
	case 0xAA:
		CodecDecodeAll.Parse(pMN);
		break;
	case 0x6FAB:
		TrackOverlay.Parse(pMN);
		break;
	case 0x23E383:
	case 0x2383E3:
		DefaultDuration.Parse(pMN);
		break;
	case 0x23314F:
		TrackTimecodeScale.Parse(pMN);
		break;
	case 0xE0:
		if (S_OK == v.Parse(pMN)) {
			DescType |= DescVideo;
		}
		break;
	case 0xE1:
		if (S_OK == a.Parse(pMN)) {
			DescType |= DescAudio;
		}
		break;
	case 0x6D80:
		ces.Parse(pMN);
		break;
	case 0x41E4:
		BlockAdditionMappings.Parse(pMN);
		break;
	EndChunk
}

bool TrackEntry::Expand(CBinary& data, UINT64 Scope)
{
	if (ces.ce.empty()) {
		return true;
	}

	std::vector<ContentEncoding*> cearray;
	for (const auto& ce : ces.ce) {
		cearray.push_back(ce.get());
	}

	std::sort(cearray.begin(), cearray.end(), [&](const ContentEncoding* left, const ContentEncoding* right) {
		return left->ContentEncodingOrder < right->ContentEncodingOrder;
	});

	for (const auto ce : cearray) {
		if (!(ce->ContentEncodingScope & Scope)) {
			continue;
		}

		if (ce->ContentEncodingType == ContentEncoding::Compression) {
			if (!data.Decompress(ce->cc)) {
				return false;
			}
		} else if (ce->ContentEncodingType == ContentEncoding::Encryption) {
			// TODO
			return false;
		}
	}

	return true;
}

HRESULT CMasteringMetadata::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x55D1:
		PrimaryRChromaticityX.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55D2:
		PrimaryRChromaticityY.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55D3:
		PrimaryGChromaticityX.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55D4:
		PrimaryGChromaticityY.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55D5:
		PrimaryBChromaticityX.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55D6:
		PrimaryBChromaticityY.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55D7:
		WhitePointChromaticityX.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55D8:
		WhitePointChromaticityY.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55D9:
		LuminanceMax.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55DA:
		LuminanceMin.Parse(pMN);
		m_bValid = true;
		break;
	EndChunk
}

HRESULT CColour::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x55B1:
		MatrixCoefficients.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55B7:
		ChromaSitingHorz.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55B8:
		ChromaSitingVert.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55B9:
		Range.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55BA:
		TransferCharacteristics.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55BB:
		Primaries.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55BC:
		MaxCLL.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55BD:
		MaxFALL.Parse(pMN);
		m_bValid = true;
		break;
	case 0x55D0:
		MasteringMetadata.Parse(pMN);
		break;
	EndChunk
}

HRESULT CProjection::Parse(CMatroskaNode* pMN0)
{
	CUInt  uValue;
	CFloat fValue;

	BeginChunk
	case 0x7671:
		if (SUCCEEDED(uValue.Parse(pMN)) && uValue >= 0 && uValue <= 3) {
			ProjectionType = uValue;
		}
		break;
	case 0x7672:
		ProjectionPrivate.Parse(pMN);
		break;
	case 0x7673:
		if (SUCCEEDED(fValue.Parse(pMN)) && fValue >= -180.0 && fValue <= 180.0) {
			ProjectionPoseYaw = fValue;
		}
		break;
	case 0x7674:
		if (SUCCEEDED(fValue.Parse(pMN)) && fValue >= -90.0 && fValue <= 90.0) {
			ProjectionPosePitch = fValue;
		}
		break;
	case 0x7675:
		if (SUCCEEDED(fValue.Parse(pMN)) && fValue >= -180.0 && fValue <= 180.0) {
			ProjectionPoseRoll = fValue;
		}
		break;
	EndChunk
}

HRESULT Video::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x9A:
		FlagInterlaced.Parse(pMN);
		break;
	case 0x9D:
		FieldOrder.Parse(pMN);
		break;
	case 0x53B8:
		StereoMode.Parse(pMN);
		break;
	case 0xB0:
		PixelWidth.Parse(pMN);
		break;
	case 0xBA:
		PixelHeight.Parse(pMN);
		break;
	case 0x54B0:
		DisplayWidth.Parse(pMN);
		break;
	case 0x54BA:
		DisplayHeight.Parse(pMN);
		break;
	case 0x54B2:
		DisplayUnit.Parse(pMN);
		break;
	case 0x54B3:
		AspectRatioType.Parse(pMN);
		break;
	case 0x54AA:
		VideoPixelCropBottom.Parse(pMN);
		if ((INT64)VideoPixelCropBottom < 0) VideoPixelCropBottom.Set(0); // fix bad value
		break;
	case 0x54BB:
		VideoPixelCropTop.Parse(pMN);
		if ((INT64)VideoPixelCropTop < 0) VideoPixelCropTop.Set(0); // fix bad value
		break;
	case 0x54CC:
		VideoPixelCropLeft.Parse(pMN);
		if ((INT64)VideoPixelCropLeft < 0) VideoPixelCropLeft.Set(0); // fix bad value
		break;
	case 0x54DD:
		VideoPixelCropRight.Parse(pMN);
		if ((INT64)VideoPixelCropRight < 0) VideoPixelCropRight.Set(0); // fix bad value
		break;
	case 0x2EB524:
		ColourSpace.Parse(pMN);
		break;
	case 0x2FB523:
		GammaValue.Parse(pMN);
		break;
	case 0x2383E3:
		FrameRate.Parse(pMN);
		break;
	case 0x55B0:
		Colour.Parse(pMN);
		break;
	case 0x7670:
		Projection.Parse(pMN);
		break;
	}
	} while (pMN->Next());

	if (VideoPixelCropLeft + VideoPixelCropRight >= PixelWidth) {
		VideoPixelCropLeft.Set(0);
		VideoPixelCropRight.Set(0);
	}

	if (VideoPixelCropTop + VideoPixelCropBottom >= PixelHeight) {
		VideoPixelCropTop.Set(0);
		VideoPixelCropBottom.Set(0);
	}

	if (DisplayUnit && (!DisplayWidth || !DisplayHeight)) {
		DisplayWidth.Set(0);
		DisplayHeight.Set(0);
		DisplayUnit.Set(0);
	}

	if (!DisplayWidth) {
		DisplayWidth.Set(PixelWidth - VideoPixelCropLeft - VideoPixelCropRight);
	}

	if (!DisplayHeight) {
		DisplayHeight.Set(PixelHeight - VideoPixelCropTop - VideoPixelCropBottom);
	}

	return S_OK;
}

HRESULT Audio::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xB5:
		SamplingFrequency.Parse(pMN);
		if (!OutputSamplingFrequency) {
			OutputSamplingFrequency.Set(SamplingFrequency);
		}
		break;
	case 0x78B5:
		OutputSamplingFrequency.Parse(pMN);
		break;
	case 0x9F:
		Channels.Parse(pMN);
		break;
	case 0x7D7B:
		ChannelPositions.Parse(pMN);
		break;
	case 0x6264:
		BitDepth.Parse(pMN);
		break;
	EndChunk
}

HRESULT ContentEncodings::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x6240:
		ce.Parse(pMN);
		break;
	EndChunk
}

HRESULT ContentEncoding::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x5031:
		ContentEncodingOrder.Parse(pMN);
		break;
	case 0x5032:
		ContentEncodingScope.Parse(pMN);
		break;
	case 0x5033:
		ContentEncodingType.Parse(pMN);
		break;
	case 0x5034:
		cc.Parse(pMN);
		break;
	case 0x5035:
		ce.Parse(pMN);
		break;
	EndChunk
}

HRESULT ContentCompression::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x4254:
		ContentCompAlgo.Parse(pMN);
		break;
	case 0x4255:
		ContentCompSettings.Parse(pMN);
		break;
	EndChunk
}

HRESULT ContentEncryption::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x47e1:
		ContentEncAlgo.Parse(pMN);
		break;
	case 0x47e2:
		ContentEncKeyID.Parse(pMN);
		break;
	case 0x47e3:
		ContentSignature.Parse(pMN);
		break;
	case 0x47e4:
		ContentSigKeyID.Parse(pMN);
		break;
	case 0x47e5:
		ContentSigAlgo.Parse(pMN);
		break;
	case 0x47e6:
		ContentSigHashAlgo.Parse(pMN);
		break;
	EndChunk
}

HRESULT Cluster::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xE7:
		TimeCode.Parse(pMN);
		break;
	case 0xA7:
		Position.Parse(pMN);
		break;
	case 0xAB:
		PrevSize.Parse(pMN);
		break;
	case MATROSKA_ID_BLOCKGROUP:
		BlockGroups.Parse(pMN, true);
		break;
	case MATROSKA_ID_SIMPLEBLOCK:
		SimpleBlocks.Parse(pMN, true);
		break;
	EndChunk
}

HRESULT Cluster::ParseTimeCode(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xE7:
		TimeCode.Parse(pMN);
		return S_OK;
	EndChunk
}

HRESULT BlockGroup::Parse(CMatroskaNode* pMN0, bool fFull)
{
	BeginChunk
	case 0xA1:
		Block.Parse(pMN, fFull);
		break;
	case 0xA2: /* TODO: multiple virt blocks? */
		;
		break;
	case 0x9B:
		BlockDuration.Parse(pMN);
		break;
	case 0xFA:
		ReferencePriority.Parse(pMN);
		break;
	case 0xFB:
		ReferenceBlock.Parse(pMN);
		break;
	case 0xFD:
		ReferenceVirtual.Parse(pMN);
		break;
	case 0xA4:
		CodecState.Parse(pMN);
		break;
	case 0xE8:
		TimeSlices.Parse(pMN);
		break;
	case 0x75A1:
		if (fFull) {
			ba.Parse(pMN);
		}
		break;
	EndChunk
}

HRESULT SimpleBlock::Parse(CMatroskaNode* pMN, bool fFull)
{
	pMN->SeekTo(pMN->m_start);

	TrackNumber.Parse(pMN);
	CShort s;
	s.Parse(pMN);
	TimeCode.Set(s);
	Lacing.Parse(pMN);

	if (!fFull) {
		return S_OK;
	}

	std::list<UINT64> lens;
	UINT64 tlen = 0;
	UINT64 FrameSize;
	BYTE FramesInLaceLessOne;

	switch ((Lacing & 0x06) >> 1) {
		case 0:
			// No lacing
			lens.push_back((pMN->m_start + pMN->m_len) - (pMN->GetPos() + tlen));
			break;
		case 1:
			// Xiph lacing
			BYTE n;
			pMN->Read(n);
			while (n-- > 0) {
				BYTE b;
				UINT64 len = 0;
				do {
					pMN->Read(b);
					len += b;
				} while (b == 0xff);
				lens.push_back(len);
				tlen += len;
			}
			lens.push_back((pMN->m_start + pMN->m_len) - (pMN->GetPos() + tlen));
			break;
		case 2:
			// Fixed-size lacing
			pMN->Read(FramesInLaceLessOne);
			FramesInLaceLessOne++;
			FrameSize = ((pMN->m_start + pMN->m_len) - (pMN->GetPos() + tlen)) / FramesInLaceLessOne;
			while (FramesInLaceLessOne-- > 0) {
				lens.push_back(FrameSize);
			}
			break;
		case 3:
			// EBML lacing
			pMN->Read(FramesInLaceLessOne);

			CLength FirstFrameSize;
			FirstFrameSize.Parse(pMN);
			lens.push_back(FirstFrameSize);
			FramesInLaceLessOne--;
			tlen = FirstFrameSize;

			CSignedLength DiffSize;
			FrameSize = FirstFrameSize;
			while (FramesInLaceLessOne--) {
				DiffSize.Parse(pMN);
				FrameSize += DiffSize;
				if ((FrameSize > (UINT64)INT_MAX) || (pMN->GetPos() + FrameSize > pMN->GetLength())) {
					DLog(L"SimpleBlock::Parse() : invalid EBML block size %I64u at %I64u for length %I64u", FrameSize, pMN->GetPos(), pMN->GetLength());
					return E_FAIL;
				}
				lens.push_back(FrameSize);
				tlen += FrameSize;
			}
			lens.push_back((pMN->m_start + pMN->m_len) - (pMN->GetPos() + tlen));
			break;
	}

	for (const UINT64 len : lens) {
		if ((__int64)len < 0) {
			continue;
		}
		std::unique_ptr<CBinary> p(DNew CBinary());
		p->resize((size_t)len);
		pMN->Read(p->data(), len);
		BlockData.emplace_back(std::move(p));
	}

	return S_OK;
}

HRESULT BlockAdditions::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xA6:
		bm.Parse(pMN);
		break;
	EndChunk
}

HRESULT BlockMore::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xEE:
		BlockAddID.Parse(pMN);
		break;
	case 0xA5:
		BlockAdditional.Parse(pMN);
		break;
	EndChunk
}

HRESULT TimeSlice::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xCC:
		LaceNumber.Parse(pMN);
		break;
	case 0xCD:
		FrameNumber.Parse(pMN);
		break;
	case 0xCE:
		Delay.Parse(pMN);
		break;
	case 0xCF:
		Duration.Parse(pMN);
		break;
	EndChunk
}

HRESULT Cue::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xBB:
		CuePoints.Parse(pMN);
		break;
	EndChunk
}

HRESULT CuePoint::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xB3:
		CueTime.Parse(pMN);
		break;
	case 0xB7:
		CueTrackPositions.Parse(pMN);
		break;
	EndChunk
}

HRESULT CueTrackPosition::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xF7:
		CueTrack.Parse(pMN);
		break;
	case 0xB2:
		CueDuration.Parse(pMN);
		break;
	case 0xF0:
		CueRelativePosition.Parse(pMN);
		break;
	case 0xF1:
		CueClusterPosition.Parse(pMN);
		break;
	case 0x5387:
		CueBlockNumber.Parse(pMN);
		break;
	case 0xEA:
		CueCodecState.Parse(pMN);
		break;
	case 0xDB:
		CueReferences.Parse(pMN);
		break;
	EndChunk
}

HRESULT CueReference::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x96:
		CueRefTime.Parse(pMN);
		break;
	case 0x97:
		CueRefCluster.Parse(pMN);
		break;
	case 0x535F:
		CueRefNumber.Parse(pMN);
		break;
	case 0xEB:
		CueRefCodecState.Parse(pMN);
		break;
	EndChunk
}

HRESULT Attachment::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x61A7:
		AttachedFiles.Parse(pMN);
		break;
	EndChunk
}

HRESULT AttachedFile::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x467E:
		FileDescription.Parse(pMN);
		break;
	case 0x466E:
		FileName.Parse(pMN);
		break;
	case 0x4660:
		FileMimeType.Parse(pMN);
		break;
	case 0x465C: // binary
		FileDataLen = pMN->m_len;
		FileDataPos = pMN->m_start;
		break;
	case 0x46AE:
		FileUID.Parse(pMN);
		break;
	EndChunk
}

HRESULT Chapter::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x45B9:
		EditionEntries.Parse(pMN);
		break;
	EndChunk
}

HRESULT EditionEntry::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0xB6:
		ChapterAtoms.Parse(pMN);
		break;
	EndChunk
}

HRESULT ChapterAtom::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x73C4:
		ChapterUID.Parse(pMN);
		break;
	case 0x91:
		ChapterTimeStart.Parse(pMN);
		break;
	case 0x92:
		ChapterTimeEnd.Parse(pMN);
		break;
		//	case 0x8F: // TODO
	case 0x80:
		ChapterDisplays.Parse(pMN);
		break;
	case 0xB6:
		ChapterAtoms.Parse(pMN);
		break;
	case 0x98:
		ChapterFlagHidden.Parse(pMN);
		break;
	case 0x4598:
		ChapterFlagEnabled.Parse(pMN);
		break;
	EndChunk
}

HRESULT ChapterDisplay::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x85:
		ChapString.Parse(pMN);
		break;
	case 0x437C:
		ChapLanguage.Parse(pMN);
		break;
	case 0x437D:
		ChapLanguageBCP47.Parse(pMN);
		break;
	case 0x437E:
		ChapCountry.Parse(pMN);
		break;
	EndChunk
}

HRESULT Tags::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x7373:
		Tag.Parse(pMN);
		break;
	EndChunk
}

HRESULT Tag::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x67C8:
		SimpleTag.Parse(pMN);
		break;
	case 0x63C0:
		Targets.Parse(pMN);
		break;
	EndChunk
}

HRESULT SimpleTag::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x45A3:
		TagName.Parse(pMN);
		break;
	case 0x447A:
		TagLanguage.Parse(pMN);
		break;
	case 0x447B:
		TagLanguageBCP47.Parse(pMN);
		break;
	case 0x4487:
		TagString.Parse(pMN);
		break;
	EndChunk
}

HRESULT Targets::Parse(CMatroskaNode* pMN0)
{
	BeginChunk
	case 0x68CA:
		TargetTypeValue.Parse(pMN);
		break;
	case 0x63CA:
		TargetType.Parse(pMN);
		break;
	case 0x63C5:
		TrackUID.Parse(pMN);
		break;
	case 0x63C9:
		EditionUID.Parse(pMN);
		break;
	case 0x63C4:
		ChapterUID.Parse(pMN);
		break;
	case 0x63C6:
		AttachmentUID.Parse(pMN);
		break;
	EndChunk
}

//

HRESULT CBinary::Parse(CMatroskaNode* pMN)
{
	ASSERT(pMN->m_len <= INT_MAX);
	resize((size_t)pMN->m_len);
	return pMN->Read(data(), pMN->m_len);
}

bool CBinary::Compress(ContentCompression& cc)
{
	if (cc.ContentCompAlgo == ContentCompression::ZLIB) {
		int res;
		z_stream c_stream;

		c_stream.zalloc = (alloc_func)0;
		c_stream.zfree = (free_func)0;
		c_stream.opaque = (voidpf)0;

		if (Z_OK != (res = deflateInit(&c_stream, 9))) {
			return false;
		}

		c_stream.next_in = data();
		c_stream.avail_in = (uInt)size();

		BYTE* dst = nullptr;
		int n = 0;
		do {
			dst = (BYTE*)realloc(dst, ++n * 10);
			c_stream.next_out = &dst[(n - 1) * 10];
			c_stream.avail_out = 10;
			if (Z_OK != (res = deflate(&c_stream, Z_FINISH)) && Z_STREAM_END != res) {
				free(dst);
				return false;
			}
		} while (0 == c_stream.avail_out && Z_STREAM_END != res);

		deflateEnd(&c_stream);

		resize(c_stream.total_out);
		memcpy(data(), dst, size());

		free(dst);

		return true;
	}

	return false;
}

bool CBinary::Decompress(ContentCompression& cc)
{
	if (cc.ContentCompAlgo == ContentCompression::ZLIB) {
		int res;
		z_stream d_stream;

		d_stream.zalloc = (alloc_func)0;
		d_stream.zfree = (free_func)0;
		d_stream.opaque = (voidpf)0;

		if (Z_OK != (res = inflateInit(&d_stream))) {
			return false;
		}

		d_stream.next_in = data();
		d_stream.avail_in = (uInt)size();

		BYTE* dst = nullptr;
		int n = 0;
		do {
			dst = (BYTE*)realloc(dst, ++n * 1000);
			d_stream.next_out = &dst[(n - 1) * 1000];
			d_stream.avail_out = 1000;
			if (Z_OK != (res = inflate(&d_stream, Z_NO_FLUSH)) && Z_STREAM_END != res) {
				free(dst);
				return false;
			}
		} while (0 == d_stream.avail_out && 0 != d_stream.avail_in && Z_STREAM_END != res);

		inflateEnd(&d_stream);

		resize(d_stream.total_out);
		memcpy(data(), dst, size());

		free(dst);

		return true;
	} else if (cc.ContentCompAlgo == ContentCompression::HDRSTRIP) {
		insert(begin(), cc.ContentCompSettings.begin(), cc.ContentCompSettings.end());
	}

	return false;
}

HRESULT CANSI::Parse(CMatroskaNode* pMN)
{
	Empty();

	UINT64 len = pMN->m_len;
	CHAR c;
	while (len-- > 0 && SUCCEEDED(pMN->Read(c))) {
		*this += c;
	}

	return (len == -1 ? S_OK : E_FAIL);
}

HRESULT CUTF8::Parse(CMatroskaNode* pMN)
{
	Empty();

	std::unique_ptr<BYTE[]> buff(new(std::nothrow) BYTE[pMN->m_len + 1]);
	if (!buff || S_OK != pMN->Read(buff.get(), pMN->m_len)) {
		return E_FAIL;
	}

	buff[pMN->m_len] = 0;
	CString::operator = (UTF8ToWStr((LPCSTR)buff.get()));
	return S_OK;
}

HRESULT CUInt::Parse(CMatroskaNode* pMN)
{
	m_val = 0;
	for (UINT64 i = 0; i < pMN->m_len; i++) {
		m_val <<= 8;
		HRESULT hr = pMN->Read(*(BYTE*)&m_val);
		if (FAILED(hr)) {
			return hr;
		}
	}
	m_fValid = true;
	return S_OK;
}

HRESULT CInt::Parse(CMatroskaNode* pMN)
{
	m_val = 0;
	for (UINT64 i = 0; i < pMN->m_len; i++) {
		HRESULT hr = pMN->Read(*((BYTE*)&m_val + 7 - i));
		if (FAILED(hr)) {
			return hr;
		}
	}
	m_val >>= (8 - pMN->m_len) * 8;
	m_fValid = true;
	return S_OK;
}

HRESULT CFloat::Parse(CMatroskaNode* pMN)
{
	HRESULT hr = E_FAIL;
	m_val = 0;

	if (pMN->m_len == 4) {
		float val = 0;
		hr = pMN->Read(val);
		m_val = val;
	} else if (pMN->m_len == 8) {
		hr = pMN->Read(m_val);
	}
	if (SUCCEEDED(hr)) {
		m_fValid = true;
	}
	return hr;
}


template<class T, class BASE>
HRESULT CSimpleVar<T, BASE>::Parse(CMatroskaNode* pMN)
{
	m_val = 0;
	m_fValid = true;
	return pMN->Read(m_val);
}

HRESULT CID::Parse(CMatroskaNode* pMN)
{
	m_val = 0;

	BYTE b = 0;
	HRESULT hr = pMN->Read(b);
	if (FAILED(hr)) {
		return hr;
	}

	int nMoreBytes = 0;

	if ((b&0x80) == 0x80) {
		m_val = b&0xff;
		nMoreBytes = 0;
	} else if ((b&0xc0) == 0x40) {
		m_val = b&0x7f;
		nMoreBytes = 1;
	} else if ((b&0xe0) == 0x20) {
		m_val = b&0x3f;
		nMoreBytes = 2;
	} else if ((b&0xf0) == 0x10) {
		m_val = b&0x1f;
		nMoreBytes = 3;
	} else {
		return E_FAIL;
	}

	while (nMoreBytes-- > 0) {
		m_val <<= 8;
		hr = pMN->Read(*(BYTE*)&m_val);
		if (FAILED(hr)) {
			return hr;
		}
	}

	m_fValid = true;

	return S_OK;
}

HRESULT CLength::Parse(CMatroskaNode* pMN)
{
	m_val = 0;

	BYTE b = 0;
	HRESULT hr = pMN->Read(b);
	if (FAILED(hr)) {
		return hr;
	}

	int nMoreBytes = 0;

	if ((b&0x80) == 0x80) {
		m_val = b&0x7f;
		nMoreBytes = 0;
	} else if ((b&0xc0) == 0x40) {
		m_val = b&0x3f;
		nMoreBytes = 1;
	} else if ((b&0xe0) == 0x20) {
		m_val = b&0x1f;
		nMoreBytes = 2;
	} else if ((b&0xf0) == 0x10) {
		m_val = b&0x0f;
		nMoreBytes = 3;
	} else if ((b&0xf8) == 0x08) {
		m_val = b&0x07;
		nMoreBytes = 4;
	} else if ((b&0xfc) == 0x04) {
		m_val = b&0x03;
		nMoreBytes = 5;
	} else if ((b&0xfe) == 0x02) {
		m_val = b&0x01;
		nMoreBytes = 6;
	} else if ((b&0xff) == 0x01) {
		m_val = b&0x00;
		nMoreBytes = 7;
	} else {
		return E_FAIL;
	}

	//int nMoreBytesTmp = nMoreBytes;

	const UINT64 UnknownSize = (1i64 << (7 * (nMoreBytes + 1))) - 1;

	while (nMoreBytes-- > 0) {
		m_val <<= 8;
		hr = pMN->Read(*(BYTE*)&m_val);
		if (FAILED(hr)) {
			return hr;
		}
	}

	if (m_val == UnknownSize) {
		DLog(L"CLength::Parse() : Unspecified chunk size 0x%016I64x at %I64u", m_val, pMN->GetPos());
		m_val = UINT64_MAX;
	}

	if (m_fSigned) {
		m_val -= (UnknownSize >> 1);
	}

	m_fValid = true;

	return S_OK;
}

/*
HRESULT CSignedLength::Parse(CMatroskaNode* pMN)
{
//	HRESULT hr = __super::Parse(pMN);
//	if (FAILED(hr)) return hr;

	m_val = 0;

	BYTE b = 0;
	HRESULT hr = pMN->Read(b);
	if (FAILED(hr)) return hr;

	int nMoreBytes = 0;

	if ((b&0x80) == 0x80) {m_val = b&0x7f; nMoreBytes = 0;}
	else if ((b&0xc0) == 0x40) {m_val = b&0x3f; nMoreBytes = 1;}
	else if ((b&0xe0) == 0x20) {m_val = b&0x1f; nMoreBytes = 2;}
	else if ((b&0xf0) == 0x10) {m_val = b&0x0f; nMoreBytes = 3;}
	else if ((b&0xf8) == 0x08) {m_val = b&0x07; nMoreBytes = 4;}
	else if ((b&0xfc) == 0x04) {m_val = b&0x03; nMoreBytes = 5;}
	else if ((b&0xfe) == 0x02) {m_val = b&0x01; nMoreBytes = 6;}
	else if ((b&0xff) == 0x01) {m_val = b&0x00; nMoreBytes = 7;}
	else return E_FAIL;

	//int nMoreBytesTmp = nMoreBytes;

	UINT64 UnknownSize = (1i64<<(7*(nMoreBytes+1)))-1;

	while (nMoreBytes-- > 0)
	{
		m_val <<= 8;
		hr = pMN->Read(*(BYTE*)&m_val);
		if (FAILED(hr)) return hr;
	}

	if (m_val == UnknownSize)
	{
		m_val = pMN->GetLength() - pMN->GetPos();
		DLog(L"CLength: Unspecified chunk size at %I64u (corrected to %I64u)", pMN->GetPos(), m_val);
	}

	m_val -= (UnknownSize >> 1);

	m_fValid = true;

	return S_OK;
}
*/

template<class T>
HRESULT CNode<T>::Parse(CMatroskaNode* pMN)
{
	std::unique_ptr<T> p(DNew T());
	HRESULT hr = E_OUTOFMEMORY;
	if (!p || FAILED(hr = p->Parse(pMN))) {
		return hr;
	}
	this->emplace_back(std::move(p));
	return S_OK;
}

HRESULT CBlockGroupNode::Parse(CMatroskaNode* pMN, bool fFull)
{
	std::unique_ptr<BlockGroup> p(DNew BlockGroup());
	HRESULT hr = E_OUTOFMEMORY;
	if (!p || FAILED(hr = p->Parse(pMN, fFull))) {
		return hr;
	}
	emplace_back(std::move(p));
	return S_OK;
}

HRESULT CSimpleBlockNode::Parse(CMatroskaNode* pMN, bool fFull)
{
	std::unique_ptr<SimpleBlock> p(DNew SimpleBlock());
	HRESULT hr = E_OUTOFMEMORY;
	if (!p || FAILED(hr = p->Parse(pMN, fFull))) {
		return hr;
	}
	emplace_back(std::move(p));
	return S_OK;
}

///////////////////////////////

CMatroskaNode::CMatroskaNode(CMatroskaFile* pMF)
	: m_pMF(pMF)
	, m_pParent(nullptr)
{
	ASSERT(m_pMF);
	m_start = m_filepos = 0;
	m_len.Set(m_pMF ? m_pMF->GetLength() : 0);
}

CMatroskaNode::CMatroskaNode(CMatroskaNode* pParent)
	: m_pMF(pParent->m_pMF)
	, m_pParent(pParent)
{
	Parse();
}

#define MAXFAILEDCOUNT (20 * 1024 * 1024)
HRESULT CMatroskaNode::Parse()
{
	m_filepos = GetPos();
	if (FAILED(m_id.Parse(this)) || FAILED(m_len.Parse(this))) {
		return E_FAIL;
	}

	m_start = GetPos();

	if (m_len == UINT64_MAX) {
		if (m_id == MATROSKA_ID_SEGMENT) {
			m_len.Set(m_pParent->m_start + m_pParent->m_len - GetPos());
		} else {
			// find next Node with the same id
			CID id;
			UINT64 byteCount = 0;
			while (byteCount < MAXFAILEDCOUNT && GetPos() < (m_pParent->m_start + m_pParent->m_len)) {
				SeekTo(m_start + byteCount);
				if (SUCCEEDED(id.Parse(this))
					&& id == m_id) {
					CLength len;
					if (SUCCEEDED(len.Parse(this))) {
						SeekTo(m_start);
						m_len.Set(byteCount);

						break;
					}
				}

				byteCount++;
			}

			if (m_len == UINT64_MAX) {
				return E_FAIL;
			}
		}
	}

	return S_OK;
}

std::unique_ptr<CMatroskaNode> CMatroskaNode::Child(DWORD id, bool fSearch)
{
	if (m_len == 0) {
		return std::unique_ptr<CMatroskaNode>();
	}
	SeekTo(m_start);
	std::unique_ptr<CMatroskaNode> pMN(DNew CMatroskaNode(this));
	if (id && !pMN->Find(id, fSearch)) {
		pMN.reset();
	}
	return pMN;
}

bool CMatroskaNode::Next(bool fSame)
{
	if (!m_pParent) {
		return false;
	}

	CID id = m_id;

	for (;;) {
		if (/*m_pParent->m_id == MATROSKA_ID_SEGMENT && */m_len > m_pParent->m_len) {
			DLog(L"CMatroskaNode::Next() : skip invalid element : len = %I64u, parent element len = %I64u", (UINT64)m_len, (UINT64)m_pParent->m_len);
			SeekTo(m_start + 1);
		} else if (m_start + m_len >= m_pParent->m_start + m_pParent->m_len) {
			break;
		} else {
			SeekTo(m_start + m_len);
		}

		if (FAILED(Parse())) {
			if (!Resync()) {
				return false;
			}
		}

		if (!fSame || m_id == id) {
			return true;
		}
	}

	return false;
}

bool CMatroskaNode::Find(DWORD id, bool fSearch)
{
	UINT64 pos = m_pParent && m_pParent->m_id == MATROSKA_ID_SEGMENT
				? FindPos(id)
				: 0;

	if (pos) {
		SeekTo(pos);
		Parse();
	} else if (fSearch) {
		while (m_id != id && Next()) {
			;
		}
	}

	return (m_id == id);
}

void CMatroskaNode::SeekTo(UINT64 pos)
{
	m_pMF->Seek(pos);
}

UINT64 CMatroskaNode::GetPos()
{
	return m_pMF->GetPos();
}

UINT64 CMatroskaNode::GetLength()
{
	return m_pMF->GetLength();
}

template <class T>
HRESULT CMatroskaNode::Read(T& var)
{
	return m_pMF->Read(var);
}

HRESULT CMatroskaNode::Read(BYTE* pData, UINT64 len)
{
	return m_pMF->ByteRead(pData, len);
}

UINT64 CMatroskaNode::FindPos(DWORD id, UINT64 start)
{
	Segment& sm = m_pMF->m_segment;

	for (const auto& s : sm.MetaSeekInfo) {
		for (const auto& sh : s->SeekHeads) {
			if (sh->SeekID == id && sh->SeekPosition+sm.pos >= start) {
				return sh->SeekPosition + sm.pos;
			}
		}
	}

	return 0;
}

std::unique_ptr<CMatroskaNode> CMatroskaNode::Copy()
{
	std::unique_ptr<CMatroskaNode> pNewNode(DNew CMatroskaNode(m_pMF));
	pNewNode->m_pParent = m_pParent;
	pNewNode->m_id.Set(m_id);
	pNewNode->m_len.Set(m_len);
	pNewNode->m_filepos = m_filepos;
	pNewNode->m_start = m_start;
	return pNewNode;
}

std::unique_ptr<CMatroskaNode> CMatroskaNode::GetFirstBlock()
{
	std::unique_ptr<CMatroskaNode> pNode = Child();
	do {
		if (pNode->m_id == MATROSKA_ID_BLOCKGROUP || pNode->m_id == MATROSKA_ID_SIMPLEBLOCK) {
			return pNode;
		}
	} while (pNode->Next());
	return std::unique_ptr<CMatroskaNode>();
}

bool CMatroskaNode::NextBlock()
{
	if (!m_pParent) {
		return false;
	}

	CID id = m_id;

	for (;;) {
		if (m_len > m_pParent->m_len) {
			DLog(L"CMatroskaNode::NextBlock() : skip invalid element : len = %I64u, parent element len = %I64u", (UINT64)m_len, (UINT64)m_pParent->m_len);
			SeekTo(m_start + 1);
		} else if (m_start + m_len >= m_pParent->m_start + m_pParent->m_len) {
			break;
		} else {
			SeekTo(m_start + m_len);
		}

		if (FAILED(Parse())) {
			if (!Resync()) {
				return false;
			}
		}

		if (m_id == MATROSKA_ID_BLOCKGROUP || m_id == MATROSKA_ID_SIMPLEBLOCK) {
			return true;
		}
	}

	return false;
}

bool CMatroskaNode::Resync()
{
	if (m_pParent->m_id == MATROSKA_ID_SEGMENT) {
		SeekTo(m_filepos);

		int failedCount = 0;
		for (BYTE b = 0; S_OK == Read(b) && failedCount < MAXFAILEDCOUNT; b = 0, failedCount++) {
			if ((b&0xf0) != 0x10) {
				continue;
			}

			DWORD dw = b;
			Read((BYTE*)&dw + 1, 3);
			bswap((BYTE*)&dw, 4);

			switch (dw) {
				case MATROSKA_ID_INFO:			// SegmentInfo
				case MATROSKA_ID_SEEKHEAD:		// MetaSeekInfo
				case MATROSKA_ID_TRACKS:		// Tracks
				case MATROSKA_ID_CLUSTER:		// Clusters
				case MATROSKA_ID_CUES:			// Cues
				case MATROSKA_ID_ATTACHMENTS:	// Attachments
				case MATROSKA_ID_CHAPTERS:		// Chapters
				case MATROSKA_ID_TAGS:			// Tags
					SeekTo(GetPos() - 4);
					return SUCCEEDED(Parse());
			}

			SeekTo(GetPos() - 3);
		}
	}

	return false;
}
