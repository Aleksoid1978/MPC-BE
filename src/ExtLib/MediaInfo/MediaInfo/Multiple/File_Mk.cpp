/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_MK_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Mk.h"
#if defined(MEDIAINFO_OGG_YES)
    #include "MediaInfo/Multiple/File_Ogg.h"
#endif
#if defined(MEDIAINFO_RM_YES)
    #include "MediaInfo/Multiple/File_Rm.h"
#endif
#if defined(MEDIAINFO_MPEG4V_YES)
    #include "MediaInfo/Video/File_Mpeg4v.h"
#endif
#if defined(MEDIAINFO_AVC_YES)
    #include "MediaInfo/Video/File_Avc.h"
#endif
#if defined(MEDIAINFO_HEVC_YES)
    #include "MediaInfo/Video/File_Hevc.h"
#endif
#if defined(MEDIAINFO_FFV1_YES)
    #include "MediaInfo/Video/File_Ffv1.h"
#endif
#if defined(MEDIAINFO_HUFFYUV_YES)
    #include "MediaInfo/Video/File_HuffYuv.h"
#endif
#if defined(MEDIAINFO_VC1_YES)
    #include "MediaInfo/Video/File_Vc1.h"
#endif
#if defined(MEDIAINFO_DIRAC_YES)
    #include "MediaInfo/Video/File_Dirac.h"
#endif
#if defined(MEDIAINFO_MPEGV_YES)
    #include "MediaInfo/Video/File_Mpegv.h"
#endif
#if defined(MEDIAINFO_PRORES_YES)
    #include "MediaInfo/Video/File_ProRes.h"
#endif
#if defined(MEDIAINFO_VP8_YES)
    #include "MediaInfo/Video/File_Vp8.h"
#endif
#if defined(MEDIAINFO_AAC_YES)
    #include "MediaInfo/Audio/File_Aac.h"
#endif
#if defined(MEDIAINFO_AC3_YES)
    #include "MediaInfo/Audio/File_Ac3.h"
#endif
#if defined(MEDIAINFO_DTS_YES)
    #include "MediaInfo/Audio/File_Dts.h"
#endif
#if defined(MEDIAINFO_MPEGA_YES)
    #include "MediaInfo/Audio/File_Mpega.h"
#endif
#if defined(MEDIAINFO_FLAC_YES)
    #include "MediaInfo/Audio/File_Flac.h"
#endif
#if defined(MEDIAINFO_OPUS_YES)
    #include "MediaInfo/Audio/File_Opus.h"
#endif
#if defined(MEDIAINFO_WVPK_YES)
    #include "MediaInfo/Audio/File_Wvpk.h"
#endif
#if defined(MEDIAINFO_TTA_YES)
    #include "MediaInfo/Audio/File_Tta.h"
#endif
#if defined(MEDIAINFO_PCM_YES)
    #include "MediaInfo/Audio/File_Pcm.h"
#endif
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif //MEDIAINFO_EVENTS
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include "ThirdParty/base64/base64.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_TRACE
    static const size_t MaxCountSameElementInTrace=10;
#endif // MEDIAINFO_TRACE

//---------------------------------------------------------------------------
namespace Elements
{
    //Common
    const int64u Zero=(int32u)-1; //Should be (int64u)-1 but Borland C++ does not like this
    const int64u CRC32=0x3F;
    const int64u Void=0x6C;

    //EBML
    const int64u Ebml=0xA45DFA3;
    const int64u Ebml_Version=0x286;
    const int64u Ebml_ReadVersion=0x2F7;
    const int64u Ebml_MaxIDLength=0x2F2;
    const int64u Ebml_MaxSizeLength=0x2F3;
    const int64u Ebml_DocType=0x282;
    const int64u Ebml_DocTypeVersion=0x287;
    const int64u Ebml_DocTypeReadVersion=0x285;

    //Segment
    const int64u Segment=0x8538067;
    const int64u Segment_Attachments=0x0941A469;
    const int64u Segment_Attachments_AttachedFile=0x21A7;
    const int64u Segment_Attachments_AttachedFile_FileDescription=0x067E;
    const int64u Segment_Attachments_AttachedFile_FileName=0x066E;
    const int64u Segment_Attachments_AttachedFile_FileMimeType=0x0660;
    const int64u Segment_Attachments_AttachedFile_FileData=0x065C;
    const int64u Segment_Attachments_AttachedFile_FileUID=0x06AE;
    const int64u Segment_Attachments_AttachedFile_FileReferral=0x0675;
    const int64u Segment_Attachments_AttachedFile_FileUsedStartTime=0x0661;
    const int64u Segment_Attachments_AttachedFile_FileUsedEndTime=0x0662;
    const int64u Segment_Chapters=0x43A770;
    const int64u Segment_Chapters_EditionEntry=0x05B9;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom=0x36;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess=0x2944;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCodecID=0x2955;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand=0x2911;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand_ChapProcessData=0x2933;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand_ChapProcessTime=0x2922;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessPrivate=0x050D;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay=0x00;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapCountry=0x037E;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapLanguage=0x037C;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapString=0x05;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterFlagEnabled=0x0598;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterFlagHidden=0x18;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterPhysicalEquiv=0x23C3;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterSegmentEditionUID=0x2EBC;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterSegmentUID=0x2E67;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterTimeStart=0x11;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterTimeEnd=0x12;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterTrack=0x0F;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterTrack_ChapterTrackNumber=0x09;
    const int64u Segment_Chapters_EditionEntry_ChapterAtom_ChapterUID=0x33C4;
    const int64u Segment_Chapters_EditionEntry_EditionFlagDefault=0x05DB;
    const int64u Segment_Chapters_EditionEntry_EditionFlagHidden=0x05BD;
    const int64u Segment_Chapters_EditionEntry_EditionFlagOrdered=0x05DD;
    const int64u Segment_Chapters_EditionEntry_EditionUID=0x05BC;
    const int64u Segment_Cluster=0xF43B675;
    const int64u Segment_Cluster_BlockGroup=0x20;
    const int64u Segment_Cluster_BlockGroup_Block=0x21;
    const int64u Segment_Cluster_BlockGroup_Block_Lace=0xFFFFFFFFFFFFFFFELL; //Fake one
    const int64u Segment_Cluster_BlockGroup_BlockVirtual=0x22;
    const int64u Segment_Cluster_BlockGroup_BlockAdditions=0x35A1;
    const int64u Segment_Cluster_BlockGroup_BlockAdditions_BlockMore=0x26;
    const int64u Segment_Cluster_BlockGroup_BlockAdditions_BlockMore_BlockAddID=0x6E;
    const int64u Segment_Cluster_BlockGroup_BlockAdditions_BlockMore_BlockAdditional=0x25;
    const int64u Segment_Cluster_BlockGroup_BlockDuration=0x1B;
    const int64u Segment_Cluster_BlockGroup_ReferencePriority=0x7A;
    const int64u Segment_Cluster_BlockGroup_ReferenceBlock=0x7B;
    const int64u Segment_Cluster_BlockGroup_ReferenceVirtual=0x7D;
    const int64u Segment_Cluster_BlockGroup_CodecState=0x24;
    const int64u Segment_Cluster_BlockGroup_DiscardPadding=0x35A2;
    const int64u Segment_Cluster_BlockGroup_Slices=0xE;
    const int64u Segment_Cluster_BlockGroup_Slices_TimeSlice=0x68;
    const int64u Segment_Cluster_BlockGroup_Slices_TimeSlice_Duration=0x4F;
    const int64u Segment_Cluster_BlockGroup_Slices_TimeSlice_LaceNumber=0x4C;
    const int64u Segment_Cluster_Position=0x27;
    const int64u Segment_Cluster_PrevSize=0x2B;
    const int64u Segment_Cluster_SilentTracks=0x1854;
    const int64u Segment_Cluster_SilentTracks_SilentTrackNumber=0x18D7;
    const int64u Segment_Cluster_SimpleBlock=0x23;
    const int64u Segment_Cluster_Timecode=0x67;
    const int64u Segment_Cues=0xC53BB6B;
    const int64u Segment_Cues_CuePoint=0x3B;
    const int64u Segment_Cues_CuePoint_CueTime=0x33;
    const int64u Segment_Cues_CuePoint_CueTrackPositions=0x37;
    const int64u Segment_Cues_CuePoint_CueTrackPositions_CueTrack=0x77;
    const int64u Segment_Cues_CuePoint_CueTrackPositions_CueClusterPosition=0x71;
    const int64u Segment_Cues_CuePoint_CueTrackPositions_CueRelativePosition=0x70;
    const int64u Segment_Cues_CuePoint_CueTrackPositions_CueDuration=0x32;
    const int64u Segment_Cues_CuePoint_CueTrackPositions_CueBlockNumber=0x1378;
    const int64u Segment_Info=0x549A966;
    const int64u Segment_Info_ChapterTranslate=0x2924;
    const int64u Segment_Info_ChapterTranslate_ChapterTranslateCodec=0x29BF;
    const int64u Segment_Info_ChapterTranslate_ChapterTranslateEditionUID=0x29FC;
    const int64u Segment_Info_ChapterTranslate_ChapterTranslateID=0x29A5;
    const int64u Segment_Info_DateUTC=0x461;
    const int64u Segment_Info_Duration=0x489;
    const int64u Segment_Info_MuxingApp=0xD80;
    const int64u Segment_Info_NextFilename=0x1E83BB;
    const int64u Segment_Info_NextUID=0x1EB923;
    const int64u Segment_Info_PrevFilename=0x1C83AB;
    const int64u Segment_Info_PrevUID=0x1CB923;
    const int64u Segment_Info_SegmentFamily=0x444;
    const int64u Segment_Info_SegmentFilename=0x3384;
    const int64u Segment_Info_SegmentUID=0x33A4;
    const int64u Segment_Info_TimecodeScale=0xAD7B1;
    const int64u Segment_Info_Title=0x3BA9;
    const int64u Segment_Info_WritingApp=0x1741;
    const int64u Segment_SeekHead=0x14D9B74;
    const int64u Segment_SeekHead_Seek=0xDBB;
    const int64u Segment_SeekHead_Seek_SeekID=0x13AB;
    const int64u Segment_SeekHead_Seek_SeekPosition=0x13AC;
    const int64u Segment_Tags=0x0254C367;
    const int64u Segment_Tags_Tag=0x3373;
    const int64u Segment_Tags_Tag_SimpleTag=0x27C8;
    const int64u Segment_Tags_Tag_SimpleTag_TagBinary=0x485;
    const int64u Segment_Tags_Tag_SimpleTag_TagDefault=0x484;
    const int64u Segment_Tags_Tag_SimpleTag_TagLanguage=0x47A;
    const int64u Segment_Tags_Tag_SimpleTag_TagName=0x5A3;
    const int64u Segment_Tags_Tag_SimpleTag_TagString=0x487;
    const int64u Segment_Tags_Tag_Targets=0x23C0;
    const int64u Segment_Tags_Tag_Targets_TargetTypeValue=0x28CA;
    const int64u Segment_Tags_Tag_Targets_TargetType=0x23CA;
    const int64u Segment_Tags_Tag_Targets_TagTrackUID=0x23C5;
    const int64u Segment_Tags_Tag_Targets_TagEditionUID=0x23C9;
    const int64u Segment_Tags_Tag_Targets_TagChapterUID=0x23C4;
    const int64u Segment_Tags_Tag_Targets_TagAttachmentUID=0x23C6;
    const int64u Segment_Tracks=0x654AE6B;
    const int64u Segment_Tracks_TrackEntry=0x2E;
    const int64u Segment_Tracks_TrackEntry_AttachmentLink=0x3446;
    const int64u Segment_Tracks_TrackEntry_Audio=0x61;
    const int64u Segment_Tracks_TrackEntry_Audio_BitDepth=0x2264;
    const int64u Segment_Tracks_TrackEntry_Audio_Channels=0x1F;
    const int64u Segment_Tracks_TrackEntry_Audio_OutputSamplingFrequency=0x38B5;
    const int64u Segment_Tracks_TrackEntry_Audio_SamplingFrequency=0x35;
    const int64u Segment_Tracks_TrackEntry_CodecSettings=0x1A9697;
    const int64u Segment_Tracks_TrackEntry_CodecInfoURL=0x1B4040;
    const int64u Segment_Tracks_TrackEntry_CodecDownloadURL=0x06B240;
    const int64u Segment_Tracks_TrackEntry_CodecDecodeAll=0x2A;
    const int64u Segment_Tracks_TrackEntry_CodecID=0x6;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings=0x2D80;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding=0x2240;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncodingOrder=0x1031;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncodingScope=0x1032;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncodingType=0x1033;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression=0x1034;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression_ContentCompAlgo=0x0254;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression_ContentCompSettings=0x0255;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption=0x1035;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentEncAlgo=0x07E1;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentEncKeyID=0x07E2;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSignature=0x07E3;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSigKeyID=0x07E4;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSigAlgo=0x07E5;
    const int64u Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSigHashAlgo=0x07E6;
    const int64u Segment_Tracks_TrackEntry_CodecName=0x58688;
    const int64u Segment_Tracks_TrackEntry_CodecPrivate=0x23A2;
    const int64u Segment_Tracks_TrackEntry_DefaultDuration=0x3E383;
    const int64u Segment_Tracks_TrackEntry_FlagDefault=0x8;
    const int64u Segment_Tracks_TrackEntry_FlagEnabled=0x39;
    const int64u Segment_Tracks_TrackEntry_FlagForced=0x15AA;
    const int64u Segment_Tracks_TrackEntry_FlagLacing=0x1C;
    const int64u Segment_Tracks_TrackEntry_Language=0x2B59C;
    const int64u Segment_Tracks_TrackEntry_MaxBlockAdditionID=0x15EE;
    const int64u Segment_Tracks_TrackEntry_MaxCache=0x2DF8;
    const int64u Segment_Tracks_TrackEntry_MinCache=0x2DE7;
    const int64u Segment_Tracks_TrackEntry_Name=0x136E;
    const int64u Segment_Tracks_TrackEntry_TrackNumber=0x57;
    const int64u Segment_Tracks_TrackEntry_TrackTimecodeScale=0x3314F;
    const int64u Segment_Tracks_TrackEntry_TrackOffset=0x137F;
    const int64u Segment_Tracks_TrackEntry_TrackType=0x3;
    const int64u Segment_Tracks_TrackEntry_TrackUID=0x33C5;
    const int64u Segment_Tracks_TrackEntry_Video=0x60;
    const int64u Segment_Tracks_TrackEntry_Video_AspectRatioType=0x14B3;
    const int64u Segment_Tracks_TrackEntry_Video_ColourSpace=0xEB524;
    const int64u Segment_Tracks_TrackEntry_Video_DisplayHeight=0x14BA;
    const int64u Segment_Tracks_TrackEntry_Video_DisplayUnit=0x14B2;
    const int64u Segment_Tracks_TrackEntry_Video_DisplayWidth=0x14B0;
    const int64u Segment_Tracks_TrackEntry_Video_FlagInterlaced=0x1A;
    const int64u Segment_Tracks_TrackEntry_Video_FieldOrder=0x1D;
    const int64u Segment_Tracks_TrackEntry_Video_FrameRate=0x383E3;
    const int64u Segment_Tracks_TrackEntry_Video_Colour=0x15B0;
    const int64u Segment_Tracks_TrackEntry_Video_Colour_MatrixCoefficients=0x15B1;
    const int64u Segment_Tracks_TrackEntry_Video_Colour_BitsPerChannel=0x15B2;
    const int64u Segment_Tracks_TrackEntry_Video_Colour_Range=0x15B9;
    const int64u Segment_Tracks_TrackEntry_Video_Colour_TransferCharacteristics=0x15BA;
    const int64u Segment_Tracks_TrackEntry_Video_Colour_Primaries=0x15BB;
    const int64u Segment_Tracks_TrackEntry_Video_PixelCropBottom=0x14AA;
    const int64u Segment_Tracks_TrackEntry_Video_PixelCropLeft=0x14CC;
    const int64u Segment_Tracks_TrackEntry_Video_PixelCropRight=0x14DD;
    const int64u Segment_Tracks_TrackEntry_Video_PixelCropTop=0x14BB;
    const int64u Segment_Tracks_TrackEntry_Video_PixelHeight=0x3A;
    const int64u Segment_Tracks_TrackEntry_Video_PixelWidth=0x30;
    const int64u Segment_Tracks_TrackEntry_Video_StereoMode=0x13B8;
    const int64u Segment_Tracks_TrackEntry_Video_OldStereoMode=0x13B9;
    const int64u Segment_Tracks_TrackEntry_TrackOverlay=0x2FAB;
    const int64u Segment_Tracks_TrackEntry_TrackTranslate=0x2624;
    const int64u Segment_Tracks_TrackEntry_TrackTranslate_Codec=0x26BF;
    const int64u Segment_Tracks_TrackEntry_TrackTranslate_EditionUID=0x26FC;
    const int64u Segment_Tracks_TrackEntry_TrackTranslate_TrackID=0x26A5;
}

//---------------------------------------------------------------------------
// CRC_32_Table (Little Endian bitstream, )
// The CRC in use is the IEEE-CRC-32 algorithm as used in the ISO 3309 standard and in section 8.1.1.6.2 of ITU-T recommendation V.42, with initial value of 0xFFFFFFFF. The CRC value MUST be computed on a little endian bitstream and MUST use little endian storage.
// A CRC is computed like this:
// Init: int32u CRC32 ^= 0;
// for each data byte do
//     CRC32=(CRC32>>8) ^ Mk_CRC32_Table[(CRC32&0xFF)^*Buffer_Current++];
// End: CRC32 ^= 0;
static const int32u Mk_CRC32_Table[256] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02Ef8D,
};

//---------------------------------------------------------------------------
static void Matroska_CRC32_Compute(int32u &CRC32, const int8u* Buffer_Current, const int8u* Buffer_End)
{
    while (Buffer_Current<Buffer_End)
        CRC32 = (CRC32 >> 8) ^ Mk_CRC32_Table[(CRC32 & 0xFF) ^ *Buffer_Current++];
}

//---------------------------------------------------------------------------
#if MEDIAINFO_FIXITY
static size_t Matroska_TryToFixCRC(int8u* Buffer, size_t Buffer_Size, int32u CRCExpected, int8u& Modified)
{
    //looking for a bit flip
    vector<size_t> BitPositions;
    size_t BitPosition_Max=Buffer_Size*8;
    for (size_t BitPosition=0; BitPosition<BitPosition_Max; BitPosition++)
    {
        size_t BytePosition=BitPosition>>3;
        size_t BitInBytePosition=BitPosition&0x7;
        Buffer[BytePosition]^=1<<BitInBytePosition;
        int32u CRC32Computed=0xFFFFFFFF;
        Matroska_CRC32_Compute(CRC32Computed, Buffer, Buffer+Buffer_Size);
        CRC32Computed ^= 0xFFFFFFFF;
        if (CRC32Computed==CRCExpected)
        {
            BitPositions.push_back(BitPosition);
        }
        Buffer[BytePosition]^=1<<BitInBytePosition;
    }

    if (BitPositions.size()!=1)
        return (size_t)-1;

    Modified=Buffer[BitPositions[0]>>3]; //Save the byte here as we already have the content
    return BitPositions[0];
}
#endif //MEDIAINFO_FIXITY

//---------------------------------------------------------------------------
static const char* Mk_ContentCompAlgo(int64u Algo)
{
    switch (Algo)
    {
        case 0x00 : return "zlib";
        case 0x01 : return "bzlib";
        case 0x02 : return "lzo1x";
        case 0x03 : return "Header stripping";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mk_StereoMode(int64u StereoMode)
{
    switch (StereoMode)
    {
        case 0x00 : return ""; //Mono (default)
        case 0x01 : return "Side by Side (left eye first)";
        case 0x02 : return "Top-Bottom (right eye first)";
        case 0x03 : return "Top-Bottom (left eye first)";
        case 0x04 : return "Checkboard (right eye first)";
        case 0x05 : return "Checkboard (left eye first)";
        case 0x06 : return "Row Interleaved (right eye first)";
        case 0x07 : return "Row Interleaved (left eye first)";
        case 0x08 : return "Column Interleaved (right eye first)";
        case 0x09 : return "Column Interleaved (left eye first)";
        case 0x0A : return "Anaglyph (cyan/red)";
        case 0x0B : return "Side by Side (right eye first)";
        case 0x0C : return "Anaglyph (green/magenta)";
        case 0x0D : return "Both Eyes laced in one block (left eye first)";
        case 0x0E : return "Both Eyes laced in one block (right eye first)";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mk_OldStereoMode(int64u StereoMode)
{
    switch (StereoMode)
    {
        case 0x00 : return ""; //Mono (default)
        case 0x01 : return "Right Eye";
        case 0x02 : return "Left Eye";
        case 0x03 : return "Both Eye";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mk_OriginalSourceMedium_From_Source_ID (const Ztring &Value)
{
    if (Value.size()==6 && Value[0] == __T('0') && Value[1] == __T('0'))
        return "Blu-ray";
    if (Value.size()==6 && Value[0] == __T('0') && Value[1] == __T('1'))
        return "DVD-Video";
    return "";
}

//---------------------------------------------------------------------------
static Ztring Mk_ID_From_Source_ID (const Ztring &Value)
{
    if (Value.size()==6 && Value[0] == __T('0') && Value[1] == __T('0'))
    {
        // Blu-ray
        int16u ValueI=0;
        for (size_t Pos=2; Pos<Value.size(); Pos++)
        {
            ValueI*=16;
            if (Value[Pos]>=__T('0') && Value[Pos]<=__T('9'))
                ValueI+=Value[Pos]-__T('0');
            else if (Value[Pos]>=__T('A') && Value[Pos]<=__T('F'))
                ValueI+=10+Value[Pos]-__T('A');
            else if (Value[Pos]>=__T('a') && Value[Pos]<=__T('f'))
                ValueI+=10+Value[Pos]-__T('a');
            else
                return Value;
        }
        return Ztring::ToZtring(ValueI);
    }

    if (Value.size()==6 && Value[0] == __T('0') && Value[1] == __T('1'))
    {
        // DVD-Video
        int16u ValueI=0;
        for (size_t Pos=2; Pos<Value.size(); Pos++)
        {
            ValueI*=16;
            if (Value[Pos]>=__T('0') && Value[Pos]<=__T('9'))
                ValueI+=Value[Pos]-__T('0');
            else if (Value[Pos]>=__T('A') && Value[Pos]<=__T('F'))
                ValueI+=10+Value[Pos]-__T('A');
            else if (Value[Pos]>=__T('a') && Value[Pos]<=__T('f'))
                ValueI+=10+Value[Pos]-__T('a');
            else
                return Value;
        }
        int8u ID1 = ValueI&0xFF;
        int8u ID2 = 0;
        ValueI-=ID1;
        if (ValueI)
            ID2=ValueI>>8;

        return Ztring::ToZtring(ID1) + (ID2?(__T('-') + Ztring::ToZtring(ID2)):Ztring());
    }

    return Value;
}

//---------------------------------------------------------------------------
static Ztring Mk_ID_String_From_Source_ID (const Ztring &Value)
{
    if (Value.size()==6 && Value[0] == __T('0') && Value[1] == __T('0'))
    {
        // Blu-ray
        int16u ValueI=0;
        for (size_t Pos=2; Pos<Value.size(); Pos++)
        {
            ValueI*=16;
            if (Value[Pos]>=__T('0') && Value[Pos]<=__T('9'))
                ValueI+=Value[Pos]-__T('0');
            else if (Value[Pos]>=__T('A') && Value[Pos]<=__T('F'))
                ValueI+=10+Value[Pos]-__T('A');
            else if (Value[Pos]>=__T('a') && Value[Pos]<=__T('f'))
                ValueI+=10+Value[Pos]-__T('a');
            else
                return Value;
        }

        return Get_Hex_ID(ValueI);
    }

    if (Value.size()==6 && Value[0] == __T('0') && Value[1] == __T('1'))
    {
        // DVD-Video
        int16u ValueI=0;
        for (size_t Pos=2; Pos<Value.size(); Pos++)
        {
            ValueI*=16;
            if (Value[Pos]>=__T('0') && Value[Pos]<=__T('9'))
                ValueI+=Value[Pos]-__T('0');
            else if (Value[Pos]>=__T('A') && Value[Pos]<=__T('F'))
                ValueI+=10+Value[Pos]-__T('A');
            else if (Value[Pos]>=__T('a') && Value[Pos]<=__T('f'))
                ValueI+=10+Value[Pos]-__T('a');
            else
                return Value;
        }
        int8u ID1 = ValueI&0xFF;
        int8u ID2 = 0;
        ValueI-=ID1;
        if (ValueI)
            ID2=ValueI>>8;

        return Get_Hex_ID(ID1) + (ID2? Get_Hex_ID(ID2):Ztring());
    }

    return Value;
}

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
extern std::string ExtensibleWave_ChannelMask (int32u ChannelMask);

//---------------------------------------------------------------------------
extern std::string ExtensibleWave_ChannelMask2 (int32u ChannelMask);

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Mk::File_Mk()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Matroska;
        StreamIDs_Width[0]=16;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_Level=2; //Container
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(0); //Container1
    #endif //MEDIAINFO_TRACE
    DataMustAlwaysBeComplete=false;
    MustSynchronize=true;

    //Temp
    InvalidByteMax=0; //Default is max size of 8 bytes
    Format_Version=0;
    TimecodeScale=1000000; //Default value
    Duration=0;
    Segment_Info_Count=0;
    Segment_Tracks_Count=0;
    Segment_Cluster_Count=0;
    CurrentAttachmentIsCover=false;
    CoverIsSetFromAttachment=false;
    Laces_Pos=0;
    #if MEDIAINFO_DEMUX
        Demux_EventWasSent=(int64u)-1;
    #endif //MEDIAINFO_DEMUX
    CRC32Compute_SkipUpTo=0;

    //Hints
    File_Buffer_Size_Hint_Pointer=NULL;

    //Helpers
    CodecPrivate=NULL;
}

//---------------------------------------------------------------------------
File_Mk::~File_Mk()
{
    delete[] CodecPrivate; //CodecPrivate=NULL;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mk::Streams_Finish()
{
    if (Duration!=0 && TimecodeScale!=0)
        Fill(Stream_General, 0, General_Duration, Duration*int64u_float64(TimecodeScale)/1000000.0, 0);

    //Tags (General)
    for (tags::iterator Item=Segment_Tags_Tag_Items.begin(); Item!=Segment_Tags_Tag_Items.end(); ++Item)
        if (!Item->first || Item->first == (int64u)-1)
        {
            for (tagspertrack::iterator Tag=Item->second.begin(); Tag!=Item->second.end(); ++Tag)
                if ((Tag->first!=__T("Encoded_Library") || Retrieve(Stream_General, 0, "Encoded_Library")!=Tag->second) // Avoid repetition if Info block is same as tags
                 && (Tag->first!=__T("Encoded_Date") || Retrieve(StreamKind_Last, StreamPos_Last, "Encoded_Date")!=Tag->second)
                 && (Tag->first!=__T("Title") || Retrieve(StreamKind_Last, StreamPos_Last, "Title")!=Tag->second))
                    Fill(Stream_General, 0, Tag->first.To_UTF8().c_str(), Tag->second);
        }

    for (std::map<int64u, stream>::iterator Temp=Stream.begin(); Temp!=Stream.end(); ++Temp)
    {
        StreamKind_Last=Temp->second.StreamKind;
        StreamPos_Last=Temp->second.StreamPos;

        //Tags (per track)
        if (Temp->second.TrackUID && Temp->second.TrackUID!=(int64u)-1)
        {
            tags::iterator Item=Segment_Tags_Tag_Items.find(Temp->second.TrackUID);
            if (Item != Segment_Tags_Tag_Items.end())
            {
                for (tagspertrack::iterator Tag=Item->second.begin(); Tag!=Item->second.end(); ++Tag)
                    if ((Tag->first!=__T("Language") || Retrieve(StreamKind_Last, StreamPos_Last, "Language").empty())) // Prioritize Tracks block over tags
                        Fill(StreamKind_Last, StreamPos_Last, Tag->first.To_UTF8().c_str(), Tag->second);
            }
        }

        //Tags
        //Ztring Duration_Temp;
        float64 FrameRate_FromTags = 0.0;
        Ztring TagsList=Retrieve(StreamKind_Last, StreamPos_Last, "_STATISTICS_TAGS");
        if (TagsList.size())
        {
            bool Tags_Verified=false;
            {
                Ztring Happ = Retrieve(Stream_General, 0, "Encoded_Application");
                Ztring Hutc = Retrieve(Stream_General, 0, "Encoded_Date");
                Ztring App = Retrieve(StreamKind_Last, StreamPos_Last, "_STATISTICS_WRITING_APP");
                Ztring Utc = Retrieve(StreamKind_Last, StreamPos_Last, "_STATISTICS_WRITING_DATE_UTC");
                Clear(StreamKind_Last, StreamPos_Last, "_STATISTICS_WRITING_APP");
                Clear(StreamKind_Last, StreamPos_Last, "_STATISTICS_WRITING_DATE_UTC");
                Clear(StreamKind_Last, StreamPos_Last, "_STATISTICS_TAGS");
                Hutc.FindAndReplace(__T("UTC "), Ztring());
                if (App==Happ && Utc==Hutc) Tags_Verified=true;
                else Fill(StreamKind_Last, StreamPos_Last, "Statistics Tags Issue", App + __T(' ') + Utc + __T(" / ") + Happ + __T(' ') + Hutc);
            }
            Ztring TempTag;

            float64 Statistics_Duration=0;
            float64 Statistics_FrameCount=0;
            for (Ztring::iterator Back = TagsList.begin();;++Back)
            {
                if ((Back == TagsList.end()) || (*Back == ' ') || (*Back == '\0'))
                {
                    if (TempTag.size())
                    {
                        Ztring TagValue = Retrieve(StreamKind_Last, StreamPos_Last, TempTag.To_Local().c_str());
                        if (TagValue.size())
                        {
                            Clear(StreamKind_Last, StreamPos_Last, TempTag.To_Local().c_str());
                            bool Set=false;
                            if (TempTag==__T("BPS"))
                            {
                                if (Tags_Verified)
                                { Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate), TagValue, true); Set=true; }
                                else
                                    TempTag="BitRate";
                            }
                            else if (TempTag==__T("DURATION"))
                            {
                                if (Tags_Verified)
                                {
                                    ZtringList Parts;
                                    Parts.Separator_Set(0, ":");
                                    Parts.Write(TagValue);
                                    Statistics_Duration=0;
                                    if (Parts.size()>=1)
                                        Statistics_Duration += Parts[0].To_float64()*60*60;
                                    if (Parts.size()>=2)
                                        Statistics_Duration += Parts[1].To_float64()*60;
                                    int8u Rounding=0; //Default is rounding to milliseconds, TODO: rounding to less than millisecond when "Duration" field is changed to seconds.
                                    if (Parts.size()>=3)
                                    {
                                        Statistics_Duration += Parts[2].To_float64();
                                        if (Parts[2].size()>6) //More than milliseconds
                                            Rounding=Parts[2].size()-6;
                                    }
                                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Duration), Statistics_Duration*1000, Rounding, true);
                                    Set=true;
                                    //Duration_Temp = TagValue;
                                }
                                if (!Set) TempTag="Duration";
                            }
                            else if (TempTag==__T("NUMBER_OF_FRAMES"))
                            {
                                if (Tags_Verified)
                                {
                                    Statistics_FrameCount=TagValue.To_float64();
                                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameCount), TagValue, true);
                                    if (StreamKind_Last==Stream_Text)
                                    {
                                        const Ztring &Format=Retrieve(Stream_Text, StreamPos_Last, "Format");
                                        if (Format.find(__T("608"))==string::npos && Format.find(__T("708"))==string::npos)
                                            Fill(Stream_Text, StreamPos_Last, Text_ElementCount, TagValue, true); // if not 608 or 708, this is used to be also the count of elements
                                    }
                                    Set=true;
                                }
                                else
                                    TempTag="FrameCount";
                            }
                            else if (TempTag==__T("NUMBER_OF_BYTES"))
                            {
                                if (Tags_Verified)
                                {
                                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize), TagValue, true);
                                    Set=true;
                                }
                                else
                                    TempTag="StreamSize";
                            }
                            else if (TempTag==__T("NUMBER_OF_BYTES_UNCOMPRESSED"))
                            {
                                if (Tags_Verified)
                                {
                                    Fill(StreamKind_Last, StreamPos_Last, "StreamSize_Demuxed", TagValue, true);
                                    Set=true;
                                }
                                else
                                    TempTag="Stream Size (Uncompressed)";
                            }
                            else if (TempTag==__T("SOURCE_ID"))
                            {
                                if (Tags_Verified)
                                {
                                    Fill(StreamKind_Last, StreamPos_Last, "OriginalSourceMedium_ID", Mk_ID_From_Source_ID(TagValue), true);
                                    Fill(StreamKind_Last, StreamPos_Last, "OriginalSourceMedium_ID/String", Mk_ID_String_From_Source_ID(TagValue), true);
                                    Fill(Stream_General, 0, General_OriginalSourceMedium, Mk_OriginalSourceMedium_From_Source_ID(TagValue), Unlimited, true, true);
                                    Set=true;
                                }
                                else
                                    TempTag="OriginalSourceMedium_ID";
                            }
                            if (!Set)
                            {
                                TempTag.insert(0, __T("FromStats_"));
                                Fill(StreamKind_Last, StreamPos_Last, TempTag.To_Local().c_str(), TagValue.To_Local().c_str());
                            }
                        }
                        if (Back == TagsList.end())
                            break;
                        TempTag.clear();
                    }
                }
                else
                    TempTag+=*Back;
            }
            if (Statistics_Duration && Statistics_FrameCount)
            {
                FrameRate_FromTags = Statistics_FrameCount/Statistics_Duration;
                if (float64_int64s(FrameRate_FromTags) - FrameRate_FromTags*1.001 > -0.0001
                 && float64_int64s(FrameRate_FromTags) - FrameRate_FromTags*1.001 < +0.0001)
                {
                    // Checking 1.001 frame rates, Statistics_Duration is has often only a 1 ms precision so we test between -1ms and +1ms
                    float64 Duration_1001 = Statistics_FrameCount / float64_int64s(FrameRate_FromTags) * 1.001000;
                    float64 Duration_1000 = Statistics_FrameCount / float64_int64s(FrameRate_FromTags) * 1.001001;
                    bool CanBe1001 = false;
                    bool CanBe1000 = false;
                    if (std::fabs((Duration_1000 - Duration_1001) * 10000) >= 15)
                    {
                        Ztring DurationS; DurationS.From_Number(Statistics_Duration, 3);
                        Ztring DurationS_1001; DurationS_1001.From_Number(Duration_1001, 3);
                        Ztring DurationS_1000; DurationS_1000.From_Number(Duration_1000, 3);

                        CanBe1001=DurationS==DurationS_1001?true:false;
                        CanBe1000=DurationS==DurationS_1000?true:false;
                        if (CanBe1001 && !CanBe1000)
                            FrameRate_FromTags = float64_int64s(FrameRate_FromTags) / 1.001;
                        if (CanBe1000 && !CanBe1001)
                            FrameRate_FromTags = float64_int64s(FrameRate_FromTags) / 1.001001;
                    }

                    // Duration from tags not reliable, checking TrackDefaultDuration
                    if (CanBe1000 == CanBe1001 && Temp->second.TrackDefaultDuration)
                    {
                        const float64 Duration_Default=((float64)1000000000)/Temp->second.TrackDefaultDuration;
                        if (float64_int64s(Duration_Default) - Duration_Default*1.001000 > -0.000002
                         && float64_int64s(Duration_Default) - Duration_Default*1.001000 < +0.000002) // Detection of precise 1.001 (e.g. 24000/1001) taking into account precision of 32-bit float
                        {
                            FrameRate_FromTags = float64_int64s(FrameRate_FromTags) / 1.001;
                        }
                        if (float64_int64s(Duration_Default) - Duration_Default*1.001001 > -0.000002
                         && float64_int64s(Duration_Default) - Duration_Default*1.001001 < +0.000002) // Detection of rounded 1.001 (e.g. 23976/1000) taking into account precision of 32-bit float
                        {
                            FrameRate_FromTags = float64_int64s(FrameRate_FromTags) / 1.001001;
                        }
                    }
                }

                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameRate), FrameRate_FromTags, 3, true);
            }
        }

        if (Temp->second.DisplayAspectRatio!=0)
        {
            //Corrections
            if (Temp->second.DisplayAspectRatio>=1.777 && Temp->second.DisplayAspectRatio<=1.778)
                Temp->second.DisplayAspectRatio=((float32)16)/9;
            if (Temp->second.DisplayAspectRatio>=1.333 && Temp->second.DisplayAspectRatio<=1.334)
                Temp->second.DisplayAspectRatio=((float32)4)/3;
            Fill(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio, Temp->second.DisplayAspectRatio, 3, true);
            int64u Width=Retrieve(Stream_Video, StreamPos_Last, Video_Width).To_int64u();
            int64u Height=Retrieve(Stream_Video, StreamPos_Last, Video_Height).To_int64u();
            if (Width)
                Fill(Stream_Video, StreamPos_Last, Video_PixelAspectRatio, Temp->second.DisplayAspectRatio*Height/Width, 3, true);
        }

        if (Temp->second.Parser)
        {
            Fill(Temp->second.Parser);
            if (Config->ParseSpeed<=1.0)
                Temp->second.Parser->Open_Buffer_Unsynch();
        }

        //Video specific
        if (StreamKind_Last==Stream_Video)
        {
            //FrameRate
            bool IsVfr=false;
            if (Temp->second.Segment_Cluster_BlockGroup_BlockDuration_Counts.size()>2)
                IsVfr=true;
            else if (Temp->second.TimeCodes.size()>1)
            {
                //Trying to detect VFR
                std::vector<int64s> FrameRate_Between;
                std::sort(Temp->second.TimeCodes.begin(), Temp->second.TimeCodes.end()); //This is PTS, no DTS --> Some frames are out of order
                size_t FramesToAdd=0;
                for (size_t Pos=1; Pos<Temp->second.TimeCodes.size(); Pos++)
                {
                    int64u Duration=Temp->second.TimeCodes[Pos]-Temp->second.TimeCodes[Pos-1];
                    if (Duration)
                        FrameRate_Between.push_back(Duration);
                    else
                        FramesToAdd++;
                }
                if (FrameRate_Between.size()>=60+32) //Minimal 1 seconds (@60 fps)
                    FrameRate_Between.resize(FrameRate_Between.size()-16); //We remove the last ones, because this is PTS, no DTS --> Some frames are out of order
                std::sort(FrameRate_Between.begin(), FrameRate_Between.end());
                if (FrameRate_Between.size()>2)
                {
                    //Looking for 3 consecutive same values, in order to remove some missing frames from stats
                    size_t i=FrameRate_Between.size()-1;
                    int64s Previous = FrameRate_Between[i];
                    do
                    {
                        i--;
                        if (FrameRate_Between[i]==Previous && FrameRate_Between[i-1]==Previous)
                            break;
                        Previous=FrameRate_Between[i];
                    }
                    while (i>2);
                    if (i>FrameRate_Between.size()/2)
                        FrameRate_Between.resize(i+2);
                }

                if (FrameRate_Between.size()>=40)
                    FrameRate_Between.resize(FrameRate_Between.size()-FrameRate_Between.size()/10); //We remove the last ones, in order to ignore skipped frames (bug of the muxer?)
                else if (FrameRate_Between.size()>=7)
                    FrameRate_Between.resize(FrameRate_Between.size()-4); //We remove the last ones, in order to ignore skipped frames (bug of the muxer?)
                if (FrameRate_Between.size()>2
                 && FrameRate_Between[0]*0.9<FrameRate_Between[FrameRate_Between.size()-1]
                 && FrameRate_Between[0]*1.1>FrameRate_Between[FrameRate_Between.size()-1]
                 && TimecodeScale)
                {
                    float Time=0;
                    for (size_t i=0; i<FrameRate_Between.size(); i++)
                        Time += FrameRate_Between[i];
                    Time /= FrameRate_Between.size();

                    if (Temp->second.TrackDefaultDuration && Time>=Temp->second.TrackDefaultDuration/TimecodeScale*0.95 && Time<=Temp->second.TrackDefaultDuration/TimecodeScale*1.05)
                        Time=(float)Temp->second.TrackDefaultDuration/TimecodeScale; //TrackDefaultDuration is maybe more precise than the time code

                    if (Time)
                    {
                        float32 FrameRate_FromCluster=1000000000/Time/TimecodeScale;
                        if (Temp->second.Parser)
                        {
                            float32 FrameRate_FromParser=Temp->second.Parser->Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate).To_float32();
                            if (FrameRate_FromParser
                             && FrameRate_FromParser*2>FrameRate_FromCluster*0.9
                             && FrameRate_FromParser*2<FrameRate_FromCluster*1.1) //TODO: awfull method to detect interlaced content with one field per block
                                FrameRate_FromCluster/=2;
                        }
                        if (FrameRate_FromTags)
                        {
                                if (FrameRate_FromCluster < FrameRate_FromTags - (FrameRate_FromTags*(TimecodeScale*0.0000000010021)) || FrameRate_FromCluster > FrameRate_FromTags + (FrameRate_FromTags*(TimecodeScale*0.0000000010021)))
                                    IsVfr=true;
                        }
                        else
                            Fill(Stream_Video, StreamPos_Last, Video_FrameRate, FrameRate_FromCluster);
                    }
                }
                else if (FrameRate_Between.size()>2)
                    IsVfr=true;
            }

            else if (Temp->second.TrackDefaultDuration)
            {
                float32 FrameRate_FromCluster=1000000000/(float32)Temp->second.TrackDefaultDuration;
                if (Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate).empty())
                    Fill(Stream_Video, StreamPos_Last, Video_FrameRate, FrameRate_FromCluster);
            }

            Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Mode, IsVfr?"VFR":"CFR");
        }

        if (Temp->second.Parser)
        {
            //Delay
            if (Temp->second.TimeCode_Start!=(int64u)-1 && TimecodeScale)
            {
                //From TimeCode
                float64 Delay=Temp->second.TimeCode_Start*int64u_float64(TimecodeScale)/1000000.0;

                //From stream format
                if (StreamKind_Last==Stream_Audio && Count_Get(Stream_Video)==1 && Temp->second.Parser->Count_Get(Stream_General)>0)
                {
                         if (Temp->second.Parser->Buffer_TotalBytes_FirstSynched==0)
                        ;
                    else if (Temp->second.AvgBytesPerSec!=0)
                        Delay+=((float64)Temp->second.Parser->Buffer_TotalBytes_FirstSynched)*1000/Temp->second.AvgBytesPerSec;
                    else
                    {
                        int64u BitRate = Temp->second.Parser->Retrieve(Stream_Audio, 0, Audio_BitRate).To_int64u();
                        if (BitRate == 0)
                            BitRate = Temp->second.Parser->Retrieve(Stream_Audio, 0, Audio_BitRate_Nominal).To_int64u();
                        if (BitRate)
                            Delay += ((float64)Temp->second.Parser->Buffer_TotalBytes_FirstSynched) * 1000 / BitRate;
                    }
                }

                //Filling
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay), Delay, 0, true);
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay_Source), "Container");
            }

            Ztring Codec_Temp=Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec)); //We want to keep the 4CC;
            //if (Duration_Temp.empty()) Duration_Temp=Retrieve(StreamKind_Last, Temp->second.StreamPos, Fill_Parameter(StreamKind_Last, Generic_Duration)); //Duration from stream is sometimes false
            //else Duration_Temp.clear();

            Finish(Temp->second.Parser);
            Merge(*Temp->second.Parser, StreamKind_Last, 0, StreamPos_Last);
            //if (!Duration_Temp.empty()) Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Duration), Duration_Temp, true);
            if (Temp->second.StreamKind==Stream_Video && !Codec_Temp.empty())
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec), Codec_Temp, true);

            //Special case: AAC
            if (StreamKind_Last==Stream_Audio
             && (Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("AAC")
              || Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("MPEG Audio")
              || Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("Vorbis")))
                Clear(Stream_Audio, StreamPos_Last, Audio_BitDepth); //Resolution is not valid for AAC / MPEG Audio / Vorbis

            //Special case: 5.1
            if (StreamKind_Last==Stream_Audio
             && (Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("AC-3")
              || Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("E-AC-3")
              || Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("DTS"))
             && Retrieve(Stream_Audio, StreamPos_Last, Audio_Channel_s__Original)==__T("6")
             && Retrieve(Stream_Audio, StreamPos_Last, Audio_Channel_s_)==__T("5"))
            {
                Clear(Stream_Audio, StreamPos_Last, Audio_Channel_s__Original);
                Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, 6, 10, true); //Some muxers do not count LFE in the channel count, let's say it is normal
            }

            //VFR
            if (Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate_Mode)==__T("VFR") && Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate_Original).empty())
            {
                Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Original, Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate));
                Clear(Stream_Video, StreamPos_Last, Video_FrameRate);
            }

            //Crop
            if (Temp->second.PixelCropLeft || Temp->second.PixelCropRight)
            {
                Fill(Stream_Video, StreamPos_Last, Video_Width_Original, Retrieve(Stream_Video, StreamPos_Last, Video_Width), true);
                Fill(Stream_Video, StreamPos_Last, Video_Width, Retrieve(Stream_Video, StreamPos_Last, Video_Width).To_int64u()-Temp->second.PixelCropLeft-Temp->second.PixelCropRight, 10, true);
                Fill(Stream_Video, StreamPos_Last, Video_Width_Offset, Temp->second.PixelCropLeft, 10, true);
            }
            if (Temp->second.PixelCropTop || Temp->second.PixelCropBottom)
            {
                Fill(Stream_Video, StreamPos_Last, Video_Height_Original, Retrieve(Stream_Video, StreamPos_Last, Video_Height), true);
                Fill(Stream_Video, StreamPos_Last, Video_Height, Retrieve(Stream_Video, StreamPos_Last, Video_Height).To_int64u()-Temp->second.PixelCropTop-Temp->second.PixelCropBottom, 10, true);
                Fill(Stream_Video, StreamPos_Last, Video_Height_Offset, Temp->second.PixelCropTop, 10, true);
            }
        }

        if (Temp->second.FrameRate!=0 && Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate).empty())
            Fill(Stream_Video, StreamPos_Last, Video_FrameRate, Temp->second.FrameRate, 3);

        //Flags
        Fill(StreamKind_Last, StreamPos_Last, "Default", Temp->second.Default?"Yes":"No");
        Fill(StreamKind_Last, StreamPos_Last, "Forced", Temp->second.Forced?"Yes":"No");
    }

    //Chapters
    if (TimecodeScale)
    {
        for (EditionEntries_Pos=0; EditionEntries_Pos<EditionEntries.size(); EditionEntries_Pos++)
        {
            Stream_Prepare(Stream_Menu);
            Fill(Stream_Menu, StreamPos_Last, Menu_Chapters_Pos_Begin, Count_Get(Stream_Menu, StreamPos_Last), 10, true);
            for (ChapterAtoms_Pos=0; ChapterAtoms_Pos<EditionEntries[EditionEntries_Pos].ChapterAtoms.size(); ChapterAtoms_Pos++)
            {
                if (EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterTimeStart!=(int64u)-1)
                {
                    Ztring Text;
                    for (ChapterDisplays_Pos=0; ChapterDisplays_Pos<EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterDisplays.size(); ChapterDisplays_Pos++)
                    {
                        Ztring PerLanguage;
                        if (!EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterDisplays[ChapterDisplays_Pos].ChapLanguage.empty())
                            PerLanguage=MediaInfoLib::Config.Iso639_1_Get(EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterDisplays[ChapterDisplays_Pos].ChapLanguage)+__T(':');
                        PerLanguage+=EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterDisplays[ChapterDisplays_Pos].ChapString;
                        Text+=PerLanguage+__T(" - ");
                    }
                    if (Text.size())
                        Text.resize(Text.size()-3);
                    Fill(Stream_Menu, StreamPos_Last, Ztring().Duration_From_Milliseconds(EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterTimeStart/1000000).To_UTF8().c_str(), Text);
                }
            }
            Fill(Stream_Menu, StreamPos_Last, Menu_Chapters_Pos_End, Count_Get(Stream_Menu, StreamPos_Last), 10, true);
        }
    }

    //Purge what is not needed anymore
    if (!File_Name.empty()) //Only if this is not a buffer, with buffer we can have more data
        Stream.clear();
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mk::Read_Buffer_Unsynched()
{
    Laces_Pos=0;
    Laces.clear();
    if (!File_GoTo)
        Element_Level=0;

    for (std::map<int64u, stream>::iterator streamItem=Stream.begin(); streamItem!=Stream.end(); streamItem++)
    {
        if (!File_GoTo)
            streamItem->second.PacketCount=0;
        if (streamItem->second.Parser)
            streamItem->second.Parser->Open_Buffer_Unsynch();
    }
}   

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_Mk::Read_Buffer_Seek(size_t Method, int64u Value, int64u ID)
{
    //Currently stupidely go back to 0 //TODO: 
    GoTo(Buffer_TotalBytes_FirstSynched);
    Open_Buffer_Unsynch();
    return 1;
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Mk::Synchronize()
{
    //Synchronizing
    while (Buffer_Offset+4<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0x1A
                                         || Buffer[Buffer_Offset+1]!=0x45
                                         || Buffer[Buffer_Offset+2]!=0xDF
                                         || Buffer[Buffer_Offset+3]!=0xA3))
    {
        Buffer_Offset++;
        while (Buffer_Offset<Buffer_Size && Buffer[Buffer_Offset]!=0x1A)
            Buffer_Offset++;
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+4>Buffer_Size)
    {
        if (Buffer_Offset+3==Buffer_Size && CC3(Buffer+Buffer_Offset)!=0x1A45DF)
            Buffer_Offset++;
        if (Buffer_Offset+2==Buffer_Size && CC2(Buffer+Buffer_Offset)!=0x1A45)
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && CC1(Buffer+Buffer_Offset)!=0x1A)
            Buffer_Offset++;
        return false;
    }

    //Synched is OK
    MustSynchronize=false; //We need synchro only once (at the beginning, in case of junk bytes before EBML)
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mk::Read_Buffer_Continue()
{
    //Handling CRC32 computing when there is no need of the data (data not parsed, only needed for CRC32)
    if (CRC32Compute_SkipUpTo>File_Offset)
    {
        int64u Size=CRC32Compute_SkipUpTo-File_Offset;
        if (Element_Size>Size)
            Element_Size=Size;
        Element_Offset=Element_Size;
        CRC32_Check();
    }
}

//***************************************************************************
// Buffer
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Mk::Header_Begin()
{
    #if MEDIAINFO_DEMUX
        //Handling of multiple frames in one block
        if (Config->Demux_Unpacketize_Get() && Demux_EventWasSent!=(int64u)-1)
        {
            stream &Stream_Temp=Stream[Demux_EventWasSent];
            Frame_Count_NotParsedIncluded=Stream_Temp.Parser->Frame_Count_NotParsedIncluded;
            FrameInfo.PTS=Stream_Temp.Parser->FrameInfo.PTS;
            Open_Buffer_Continue(Stream_Temp.Parser, Buffer + Buffer_Offset, 0);
            if (Config->Demux_EventWasSent)
                return false;
            Demux_EventWasSent=(int64u)-1;
        }
    #endif //MEDIAINFO_DEMUX

    return true;
}

//---------------------------------------------------------------------------
void File_Mk::Header_Parse()
{
    //Handling of laces
    if (!Laces.empty())
    {
        Header_Fill_Code(Elements::Segment_Cluster_BlockGroup_Block_Lace, "Data");
        Header_Fill_Size(Laces[Laces_Pos]);
        return;
    }

    //Test of zero padding
    int8u Null;
    Peek_B1(Null);
    if (Null<=InvalidByteMax)
    {
        if (Buffer_Offset_Temp==0)
            Buffer_Offset_Temp=Buffer_Offset+1;

        while (Buffer_Offset_Temp<Buffer_Size)
        {
            if (Buffer[Buffer_Offset_Temp]>InvalidByteMax)
                break;
            Buffer_Offset_Temp++;
        }
        if (Buffer_Offset_Temp>=Buffer_Size)
        {
            Element_WaitForMoreData();
            return;
        }

        Header_Fill_Code((int32u)-1); //Should be (int64u)-1 but Borland C++ does not like this
        Header_Fill_Size(Buffer_Offset_Temp-Buffer_Offset);
        Buffer_Offset_Temp=0;

        return;
    }

    //Parsing
    int64u Name = 0, Size = 0;
    bool NameIsValid=true;
    if (Element_Offset+1<Element_Size)
    {
        int8u NamePeek;
        Peek_B1(NamePeek);
        if (NamePeek<0x10)
        {
            Skip_B1(                                            "Invalid");
            #if MEDIAINFO_TRACE
            Element_Level--;
            Element_Info("NOK");
            Element_Level++;
            #endif //MEDIAINFO_TRACE
            NameIsValid=false;

            Header_Fill_Code(0, "Junk");
            Header_Fill_Size(1);
        }
    }
    if (NameIsValid)
    {
    Get_EB (Name,                                               "Name");
    Get_EB (Size,                                               "Size");

    //Detection of 0-sized Segment expected to be -1-sized (unlimited)
    if (Name==Elements::Segment && Size==0)
    {
        Param_Info1("Incoherent, changed to unlimited");
        Size=0xFFFFFFFFFFFFFFLL; //Unlimited
        Fill(Stream_General, 0, "SegmentSizeIsZero", "Yes");

        #if MEDIAINFO_FIXITY
            if (Config->TryToFix_Get())
            {
                size_t Pos=(size_t)(Element_Offset-1);
                while (!Buffer[Buffer_Offset+Pos])
                    Pos--;
                size_t ToWrite_Size=Element_Offset-Pos;
                if (ToWrite_Size<=8)
                {
                    int8u ToWrite[8];
                    int64u2BigEndian(ToWrite, ((int64u)-1)>>(ToWrite_Size-1));
                    FixFile(File_Offset+Buffer_Offset+Pos, ToWrite, ToWrite_Size)?Param_Info1("Fixed"):Param_Info1("Not fixed");
                }
            }
        #endif //MEDIAINFO_FIXITY
    }

    //Filling
    Header_Fill_Code(Name, Ztring().From_Number(Name, 16));
    Header_Fill_Size(Element_Offset+Size);
    }

    if ((Name==Elements::Segment_Cluster_BlockGroup_Block || Name==Elements::Segment_Cluster_SimpleBlock) && Buffer_Offset+Element_Offset+Size>Buffer_Size && File_Buffer_Size_Hint_Pointer)
    {
        int64u Buffer_Size_Target = (size_t)(Buffer_Offset + Element_Offset + Size - Buffer_Size + Element_Offset); //+Element_Offset for next packet header

        if (Buffer_Size_Target<128 * 1024)
            Buffer_Size_Target = 128 * 1024;
        (*File_Buffer_Size_Hint_Pointer) = (size_t)Buffer_Size_Target;

        Element_WaitForMoreData();
        return;
    }

    //Incoherencies
    if (Element_Level<=2 && File_Offset+Buffer_Offset+Element_Offset+Size>File_Size)
        Fill(Stream_General, 0, "IsTruncated", "Yes");
}

//---------------------------------------------------------------------------
void File_Mk::Data_Parse()
{
    #define LIS2(_ATOM, _NAME) \
        case Elements::_ATOM : \
                if (Level==Element_Level) \
                { \
                    Element_Name(_NAME); \
                    _ATOM(); \
                    Element_ThisIsAList(); \
                } \

    #define ATO2(_ATOM, _NAME) \
                case Elements::_ATOM : \
                        if (Level==Element_Level) \
                        { \
                            if (Element_IsComplete_Get()) \
                            { \
                                Element_Name(_NAME); \
                                _ATOM(); \
                            } \
                            else \
                            { \
                                Element_WaitForMoreData(); \
                                return; \
                            } \
                        } \
                        break; \

    #define ATOM_END_MK \
        ATOM(Zero) \
        ATOM(CRC32) \
        ATOM(Void) \
        ATOM_END

    //Parsing
    DATA_BEGIN
    LIST(Ebml)
        ATOM_BEGIN
        ATOM(Ebml_Version)
        ATOM(Ebml_ReadVersion)
        ATOM(Ebml_MaxIDLength)
        ATOM(Ebml_MaxSizeLength)
        ATOM(Ebml_DocType)
        ATOM(Ebml_DocTypeVersion)
        ATOM(Ebml_DocTypeReadVersion)
        ATOM_END_MK
    LIST(Segment)
        ATOM_BEGIN
        LIST(Segment_Attachments)
            ATOM_BEGIN
            LIST(Segment_Attachments_AttachedFile)
                ATOM_BEGIN
                ATOM(Segment_Attachments_AttachedFile_FileDescription)
                ATOM(Segment_Attachments_AttachedFile_FileName)
                ATOM(Segment_Attachments_AttachedFile_FileMimeType)
                LIST_SKIP(Segment_Attachments_AttachedFile_FileData) //This is ATOM, but some ATOMs are too big
                ATOM(Segment_Attachments_AttachedFile_FileUID)
                ATOM(Segment_Attachments_AttachedFile_FileReferral)
                ATOM(Segment_Attachments_AttachedFile_FileUsedStartTime)
                ATOM(Segment_Attachments_AttachedFile_FileUsedEndTime)
                ATOM_END_MK
            ATOM_END_MK
        LIST(Segment_Chapters)
            ATOM_BEGIN
            LIST(Segment_Chapters_EditionEntry)
                ATOM_BEGIN
                LIST(Segment_Chapters_EditionEntry_ChapterAtom)
                    ATOM_BEGIN
                    LIST(Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess)
                        ATOM_BEGIN
                        ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCodecID)
                        LIST(Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand)
                            ATOM_BEGIN
                            ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand_ChapProcessData)
                            ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand_ChapProcessTime)
                            ATOM_END_MK
                        ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessPrivate)
                        ATOM_END_MK
                    LIST(Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay)
                        ATOM_BEGIN
                        ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapCountry)
                        ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapLanguage)
                        ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapString)
                        ATOM_END_MK
                    ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterFlagEnabled)
                    ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterFlagHidden)
                    ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterPhysicalEquiv)
                    ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterSegmentEditionUID)
                    ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterSegmentUID)
                    ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterTimeEnd)
                    ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterTimeStart)
                    LIST(Segment_Chapters_EditionEntry_ChapterAtom_ChapterTrack)
                        ATOM_BEGIN
                        ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterTrack_ChapterTrackNumber)
                        ATOM_END_MK
                    ATOM(Segment_Chapters_EditionEntry_ChapterAtom_ChapterUID)
                    ATOM_END_MK
                ATOM(Segment_Chapters_EditionEntry_EditionFlagDefault)
                ATOM(Segment_Chapters_EditionEntry_EditionFlagHidden)
                ATOM(Segment_Chapters_EditionEntry_EditionFlagOrdered)
                ATOM(Segment_Chapters_EditionEntry_EditionUID)
                ATOM_END_MK
            ATOM_END_MK
        LIST(Segment_Cluster)
            ATOM_BEGIN
            LIST(Segment_Cluster_BlockGroup)
                ATOM_BEGIN
                LIST(Segment_Cluster_BlockGroup_Block)
                    ATOM_BEGIN
                    ATOM(Segment_Cluster_BlockGroup_Block_Lace)
                    ATOM_END_MK
                LIST(Segment_Cluster_BlockGroup_BlockAdditions)
                    ATOM_BEGIN
                    LIST(Segment_Cluster_BlockGroup_BlockAdditions_BlockMore)
                        ATOM_BEGIN
                        ATOM(Segment_Cluster_BlockGroup_BlockAdditions_BlockMore_BlockAddID)
                        ATOM(Segment_Cluster_BlockGroup_BlockAdditions_BlockMore_BlockAdditional)
                        ATOM_END_MK
                    ATOM_END_MK
                ATOM(Segment_Cluster_BlockGroup_BlockDuration)
                ATOM(Segment_Cluster_BlockGroup_ReferencePriority)
                ATOM(Segment_Cluster_BlockGroup_ReferenceBlock)
                ATOM(Segment_Cluster_BlockGroup_ReferenceVirtual)
                ATOM(Segment_Cluster_BlockGroup_CodecState)
                LIST(Segment_Cluster_BlockGroup_Slices)
                    ATOM_BEGIN
                    LIST(Segment_Cluster_BlockGroup_Slices_TimeSlice)
                        ATOM_BEGIN
                        ATOM(Segment_Cluster_BlockGroup_Slices_TimeSlice_Duration)
                        ATOM(Segment_Cluster_BlockGroup_Slices_TimeSlice_LaceNumber)
                        ATOM_END_MK
                    ATOM_END_MK
                ATOM_END_MK
            ATOM(Segment_Cluster_Position)
            ATOM(Segment_Cluster_PrevSize)
            LIST(Segment_Cluster_SilentTracks)
                ATOM_BEGIN
                ATOM(Segment_Cluster_SilentTracks_SilentTrackNumber)
                ATOM_END_MK
            LIST(Segment_Cluster_SimpleBlock)
                ATOM_BEGIN
                ATOM(Segment_Cluster_BlockGroup_Block_Lace)
                ATOM_END_MK
            ATOM(Segment_Cluster_Timecode)
            ATOM_END_MK
        LIST(Segment_Cues)
            ATOM_BEGIN
            LIST(Segment_Cues_CuePoint)
                ATOM_BEGIN
                ATOM(Segment_Cues_CuePoint_CueTime)
                LIST(Segment_Cues_CuePoint_CueTrackPositions)
                    ATOM_BEGIN
                    ATOM(Segment_Cues_CuePoint_CueTrackPositions_CueTrack)
                    ATOM(Segment_Cues_CuePoint_CueTrackPositions_CueClusterPosition)
                    ATOM(Segment_Cues_CuePoint_CueTrackPositions_CueRelativePosition)
                    ATOM(Segment_Cues_CuePoint_CueTrackPositions_CueDuration)
                    ATOM(Segment_Cues_CuePoint_CueTrackPositions_CueBlockNumber)
                    ATOM_END_MK
                ATOM_END_MK
            ATOM_END_MK
        LIST(Segment_Info)
            ATOM_BEGIN
            LIST(Segment_Info_ChapterTranslate)
                ATOM_BEGIN
                ATOM(Segment_Info_ChapterTranslate_ChapterTranslateCodec)
                ATOM(Segment_Info_ChapterTranslate_ChapterTranslateEditionUID)
                ATOM(Segment_Info_ChapterTranslate_ChapterTranslateID)
                ATOM_END_MK
            ATOM(Segment_Info_DateUTC)
            ATOM(Segment_Info_Duration)
            ATOM(Segment_Info_MuxingApp)
            ATOM(Segment_Info_NextFilename)
            ATOM(Segment_Info_NextUID)
            ATOM(Segment_Info_PrevFilename)
            ATOM(Segment_Info_PrevUID)
            ATOM(Segment_Info_SegmentFamily)
            ATOM(Segment_Info_SegmentFilename)
            ATOM(Segment_Info_SegmentUID)
            ATOM(Segment_Info_TimecodeScale)
            ATOM(Segment_Info_Title)
            ATOM(Segment_Info_WritingApp)
            ATOM_END_MK
        LIST(Segment_SeekHead)
            ATOM_BEGIN
            LIST(Segment_SeekHead_Seek)
                ATOM_BEGIN
                ATOM(Segment_SeekHead_Seek_SeekID)
                ATOM(Segment_SeekHead_Seek_SeekPosition)
                ATOM_END_MK
            ATOM_END_MK
        LIST(Segment_Tags)
            ATOM_BEGIN
            LIST(Segment_Tags_Tag)
                ATOM_BEGIN
                LIST(Segment_Tags_Tag_SimpleTag)
                    ATOM_BEGIN
                    LIST(Segment_Tags_Tag_SimpleTag)
                        ATOM_BEGIN
                        LIST(Segment_Tags_Tag_SimpleTag)
                            ATOM_BEGIN
                            ATOM(Segment_Tags_Tag_SimpleTag_TagBinary)
                            ATOM(Segment_Tags_Tag_SimpleTag_TagDefault)
                            ATOM(Segment_Tags_Tag_SimpleTag_TagLanguage)
                            ATOM(Segment_Tags_Tag_SimpleTag_TagName)
                            ATOM(Segment_Tags_Tag_SimpleTag_TagString)
                            ATOM_END_MK
                        ATOM(Segment_Tags_Tag_SimpleTag_TagBinary)
                        ATOM(Segment_Tags_Tag_SimpleTag_TagDefault)
                        ATOM(Segment_Tags_Tag_SimpleTag_TagLanguage)
                        ATOM(Segment_Tags_Tag_SimpleTag_TagName)
                        ATOM(Segment_Tags_Tag_SimpleTag_TagString)
                        ATOM_END_MK
                    ATOM(Segment_Tags_Tag_SimpleTag_TagBinary)
                    ATOM(Segment_Tags_Tag_SimpleTag_TagDefault)
                    ATOM(Segment_Tags_Tag_SimpleTag_TagLanguage)
                    ATOM(Segment_Tags_Tag_SimpleTag_TagName)
                    ATOM(Segment_Tags_Tag_SimpleTag_TagString)
                    ATOM_END_MK
                LIST(Segment_Tags_Tag_Targets)
                    ATOM_BEGIN
                    ATOM(Segment_Tags_Tag_Targets_TargetTypeValue)
                    ATOM(Segment_Tags_Tag_Targets_TargetType)
                    ATOM(Segment_Tags_Tag_Targets_TagTrackUID)
                    ATOM(Segment_Tags_Tag_Targets_TagEditionUID)
                    ATOM(Segment_Tags_Tag_Targets_TagChapterUID)
                    ATOM(Segment_Tags_Tag_Targets_TagAttachmentUID)
                    ATOM_END_MK
                ATOM_END_MK
            ATOM_END_MK
        LIST(Segment_Tracks)
            ATOM_BEGIN
            LIST(Segment_Tracks_TrackEntry)
                ATOM_BEGIN
                ATOM(Segment_Tracks_TrackEntry_AttachmentLink)
                LIST(Segment_Tracks_TrackEntry_Audio)
                    ATOM_BEGIN
                    ATOM(Segment_Tracks_TrackEntry_Audio_BitDepth)
                    ATOM(Segment_Tracks_TrackEntry_Audio_Channels)
                    ATOM(Segment_Tracks_TrackEntry_Audio_OutputSamplingFrequency)
                    ATOM(Segment_Tracks_TrackEntry_Audio_SamplingFrequency)
                    ATOM_END_MK
                ATOM(Segment_Tracks_TrackEntry_CodecSettings)
                ATOM(Segment_Tracks_TrackEntry_CodecInfoURL)
                ATOM(Segment_Tracks_TrackEntry_CodecDownloadURL)
                ATOM(Segment_Tracks_TrackEntry_CodecDecodeAll)
                ATOM(Segment_Tracks_TrackEntry_CodecID)
                LIS2(Segment_Tracks_TrackEntry_ContentEncodings, "ContentEncodings")
                    ATOM_BEGIN
                    LIS2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding, "ContentEncoding")
                    ATOM_BEGIN
                        ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncodingOrder, "ContentEncodingOrder")
                        ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncodingScope, "ContentEncodingScope")
                        ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncodingType, "ContentEncodingType")
                        LIS2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression, "ContentCompression")
                            ATOM_BEGIN
                            ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression_ContentCompAlgo, "ContentCompAlgo")
                            ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression_ContentCompSettings, "ContentCompSettings")
                            ATOM_END_MK
                        LIS2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption, "ContentEncryption")
                            ATOM_BEGIN
                            ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentEncAlgo, "ContentAlgo")
                            ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentEncKeyID, "ContentKeyID")
                            ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSignature, "ContentSignature")
                            ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSigKeyID, "ContentSigKeyID")
                            ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSigAlgo, "ContentSigAlgo")
                            ATO2(Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSigHashAlgo, "ContentSigHashAlgo")
                            ATOM_END_MK
                        ATOM_END_MK
                    ATOM_END_MK
                ATOM(Segment_Tracks_TrackEntry_CodecName)
                ATOM(Segment_Tracks_TrackEntry_CodecPrivate)
                ATOM(Segment_Tracks_TrackEntry_DefaultDuration)
                ATOM(Segment_Tracks_TrackEntry_FlagDefault)
                ATOM(Segment_Tracks_TrackEntry_FlagEnabled)
                ATOM(Segment_Tracks_TrackEntry_FlagForced)
                ATOM(Segment_Tracks_TrackEntry_FlagLacing)
                ATOM(Segment_Tracks_TrackEntry_Language)
                ATOM(Segment_Tracks_TrackEntry_MaxBlockAdditionID)
                ATOM(Segment_Tracks_TrackEntry_MaxCache)
                ATOM(Segment_Tracks_TrackEntry_MinCache)
                ATOM(Segment_Tracks_TrackEntry_Name)
                ATOM(Segment_Tracks_TrackEntry_TrackNumber)
                ATOM(Segment_Tracks_TrackEntry_TrackTimecodeScale)
                ATOM(Segment_Tracks_TrackEntry_TrackOffset)
                ATOM(Segment_Tracks_TrackEntry_TrackType)
                ATOM(Segment_Tracks_TrackEntry_TrackUID)
                LIST(Segment_Tracks_TrackEntry_Video)
                    ATOM_BEGIN
                    ATOM(Segment_Tracks_TrackEntry_Video_AspectRatioType)
                    ATOM(Segment_Tracks_TrackEntry_Video_ColourSpace)
                    ATOM(Segment_Tracks_TrackEntry_Video_DisplayHeight)
                    ATOM(Segment_Tracks_TrackEntry_Video_DisplayUnit)
                    ATOM(Segment_Tracks_TrackEntry_Video_DisplayWidth)
                    ATOM(Segment_Tracks_TrackEntry_Video_FlagInterlaced)
                    ATOM(Segment_Tracks_TrackEntry_Video_FieldOrder)
                    ATOM(Segment_Tracks_TrackEntry_Video_FrameRate)
                    LIST(Segment_Tracks_TrackEntry_Video_Colour)
                        ATOM_BEGIN
                        ATOM(Segment_Tracks_TrackEntry_Video_Colour_MatrixCoefficients)
                        ATOM(Segment_Tracks_TrackEntry_Video_Colour_BitsPerChannel)
                        ATOM(Segment_Tracks_TrackEntry_Video_Colour_Range)
                        ATOM(Segment_Tracks_TrackEntry_Video_Colour_TransferCharacteristics)
                        ATOM(Segment_Tracks_TrackEntry_Video_Colour_Primaries)
                        ATOM_END_MK
                    ATOM(Segment_Tracks_TrackEntry_Video_PixelCropBottom)
                    ATOM(Segment_Tracks_TrackEntry_Video_PixelCropLeft)
                    ATOM(Segment_Tracks_TrackEntry_Video_PixelCropRight)
                    ATOM(Segment_Tracks_TrackEntry_Video_PixelCropTop)
                    ATOM(Segment_Tracks_TrackEntry_Video_PixelHeight)
                    ATOM(Segment_Tracks_TrackEntry_Video_PixelWidth)
                    ATOM(Segment_Tracks_TrackEntry_Video_StereoMode)
                    ATOM_END_MK
                ATOM(Segment_Tracks_TrackEntry_TrackOverlay)
                LIST(Segment_Tracks_TrackEntry_TrackTranslate)
                    ATOM_BEGIN
                    ATOM(Segment_Tracks_TrackEntry_TrackTranslate_Codec)
                    ATOM(Segment_Tracks_TrackEntry_TrackTranslate_EditionUID)
                    ATOM(Segment_Tracks_TrackEntry_TrackTranslate_TrackID)
                    ATOM_END_MK
                ATOM_END_MK
            ATOM_END_MK
        ATOM_END_MK
    DATA_END

    if (!Element_IsWaitingForMoreData() && !CRC32Compute.empty())
        CRC32_Check();
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mk::Zero()
{
    Element_Name("Junk");

    Skip_XX(Element_Size,                                       "Junk");
}

//---------------------------------------------------------------------------
void File_Mk::CRC32()
{
    Element_Name("CRC-32");

    //Parsing
    if (Element_Size!=4)
        UInteger_Info(); //Something is wrong, 4-byte integer is expected
    else
    {
        if (CRC32Compute.empty())
            Fill(Stream_General, 0, "ErrorDetectionType", Element_Level==3?"Per level 1":"Custom", Unlimited, true, true);

        if (CRC32Compute.size()<Element_Level)
            CRC32Compute.resize(Element_Level);
        
        Get_L4(CRC32Compute[Element_Level-1].Expected,          "Value");

        {
            Param_Info1(__T("Not tested ")+Ztring::ToZtring(Element_Level-1)+__T(' ')+Ztring::ToZtring(CRC32Compute[Element_Level-1].Expected));
            CRC32Compute[Element_Level-1].Computed=0xFFFFFFFF;
            CRC32Compute[Element_Level-1].Pos = File_Offset + Buffer_Offset;
            CRC32Compute[Element_Level-1].From = File_Offset + Buffer_Offset + Element_Size;
            CRC32Compute[Element_Level-1].UpTo = File_Offset + Buffer_Offset + Element_TotalSize_Get(1);
        }
    }
}

//---------------------------------------------------------------------------
void File_Mk::Void()
{
    Element_Name("Void");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Ebml()
{
    Element_Name("EBML");
}

//---------------------------------------------------------------------------
void File_Mk::Ebml_Version()
{
    Element_Name("EBMLVersion");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Ebml_ReadVersion()
{
    Element_Name("EBMLReadVersion");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Ebml_MaxIDLength()
{
    Element_Name("EBMLMaxIDLength");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Ebml_MaxSizeLength()
{
    Element_Name("EBMLMaxSizeLength");

    //Parsing
    int64u Value = UInteger_Get();

    //Filling
    FILLING_BEGIN();
        if (Value > 8)
            Value = 8; //Not expected, considerating it as if it is 8 for the moment
        InvalidByteMax = (int8u)((1 << (8-Value))-1);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Ebml_DocType()
{
    Element_Name("DocType");

    //Parsing
    Ztring Data;
    Get_Local(Element_Size, Data,                               "Data"); Element_Info1(Data);

    //Filling
    FILLING_BEGIN();
        if (Data==__T("matroska"))
        {
            Accept("Matroska");
            Fill(Stream_General, 0, General_Format, "Matroska");
            Buffer_MaximumSize = 64 * 1024 * 1024; //Testing with huge lossless 4K frames
            File_Buffer_Size_Hint_Pointer = Config->File_Buffer_Size_Hint_Pointer_Get();
        }
        else if (Data==__T("webm"))
        {
            Accept("Matroska");
            Fill(Stream_General, 0, General_Format, "WebM");
        }
        else
        {
            Reject("Matroska");
            return;
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Ebml_DocTypeVersion()
{
    Element_Name("DocTypeVersion");

    //Parsing
    Format_Version=UInteger_Get();

    //Filling
    FILLING_BEGIN();
        Fill(Stream_General, 0, General_Format_Version, __T("Version ")+Ztring::ToZtring(Format_Version));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Ebml_DocTypeReadVersion()
{
    Element_Name("DocTypeReadVersion");

    //Parsing
    int64u UInteger=UInteger_Get();

    //Filling
    FILLING_BEGIN();
        if (UInteger!=Format_Version)
            Fill(Stream_General, 0, General_Format_Version, __T("Version ")+Ztring::ToZtring(UInteger)); //Adding compatible version for info about legacy decoders
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment()
{
    Element_Name("Segment");

    if (!Status[IsAccepted])
    {
        Accept("Matroska");
        Fill(Stream_General, 0, General_Format, "Matroska"); //Default is Matroska
    }

    Segment_Offset_Begin=File_Offset+Buffer_Offset;
    Segment_Offset_End=File_Offset+Buffer_Offset+Element_TotalSize_Get();

    #if MEDIAINFO_TRACE
        Trace_Segment_Cluster_Count=0;
    #endif // MEDIAINFO_TRACE
}

void File_Mk::Segment_Attachments()
{
    Element_Name("Attachments");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Attachments_AttachedFile()
{
    Element_Name("AttachedFile");

    AttachedFile_FileName.clear();
    AttachedFile_FileMimeType.clear();
    AttachedFile_FileDescription.clear();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Attachments_AttachedFile_FileDescription()
{
    Element_Name("FileDescription");

    //Parsing
    Ztring Data=UTF8_Get();

    AttachedFile_FileDescription=Data.To_UTF8();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Attachments_AttachedFile_FileName()
{
    Element_Name("FileName");

    //Parsing
    Ztring Data=UTF8_Get();

    Fill(Stream_General, 0, "Attachments", Data);
    
    //Cover is in the first file which name contains "cover"
    if (!CoverIsSetFromAttachment && Data.MakeLowerCase().find(__T("cover")) != string::npos)
        CurrentAttachmentIsCover=true;

    AttachedFile_FileName=Data.To_UTF8();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Attachments_AttachedFile_FileMimeType()
{
    Element_Name("FileMimeType");

    //Parsing
    Ztring Data=Local_Get();

    AttachedFile_FileMimeType=Data.To_UTF8();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Attachments_AttachedFile_FileData()
{
    Element_Name("FileData");

    bool Attachments_Demux=true;

    //Parsing
    if ((Attachments_Demux || !CoverIsSetFromAttachment && CurrentAttachmentIsCover) && Element_Size<=16*1024*1024) //TODO: option for setting the acceptable maximum size of the attachment
    {
        if (!Element_IsComplete_Get())
        {
            Element_WaitForMoreData();
            return;
        }

        std::string Data_Raw;
        Peek_String(Element_TotalSize_Get(), Data_Raw);

        if (!CoverIsSetFromAttachment && CurrentAttachmentIsCover)
        {
            std::string Data_Base64(Base64::encode(Data_Raw));

            //Filling
            Fill(Stream_General, 0, General_Cover_Data, Data_Base64);
            Fill(Stream_General, 0, General_Cover, "Yes");
            CoverIsSetFromAttachment=true;
        }

        #if MEDIAINFO_EVENTS
            if (Attachments_Demux)
            {
                EVENT_BEGIN(Global, AttachedFile, 0)
                    Event.Content_Size=Data_Raw.size();
                    Event.Content=(const int8u*)Data_Raw.c_str();
                    Event.Flags=0;
                    Event.Name=AttachedFile_FileName.c_str();
                    Event.MimeType=AttachedFile_FileMimeType.c_str();
                    Event.Description=AttachedFile_FileDescription.c_str();
                EVENT_END()
            }
        #endif //MEDIAINFO_EVENTS
    }
    
    Skip_XX(Element_TotalSize_Get(),                            "Data");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Attachments_AttachedFile_FileUID()
{
    Element_Name("FileUID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Attachments_AttachedFile_FileReferral()
{
    Element_Name("FileReferral");

    //Parsing
    Skip_XX(Element_Size,                                       "Data");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Attachments_AttachedFile_FileUsedStartTime()
{
    Element_Name("FileUsedStartTime");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Attachments_AttachedFile_FileUsedEndTime()
{
    Element_Name("FileUsedEndTime");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters()
{
    Element_Name("Chapters");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry()
{
    Element_Name("EditionEntry");

    //Filling
    EditionEntries_Pos=EditionEntries.size();
    EditionEntries.resize(EditionEntries_Pos+1);
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom()
{
    Element_Name("ChapterAtom");

    //Filling
    ChapterAtoms_Pos=EditionEntries[EditionEntries_Pos].ChapterAtoms.size();
    EditionEntries[EditionEntries_Pos].ChapterAtoms.resize(ChapterAtoms_Pos+1);
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess()
{
    Element_Name("ChapProcess");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCodecID()
{
    Element_Name("ChapProcessCodecID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand()
{
    Element_Name("ChapProcessCommand");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand_ChapProcessData()
{
    Element_Name("ChapProcessData");

    //Parsing
    Skip_XX(Element_Size,                                       "Data");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand_ChapProcessTime()
{
    Element_Name("ChapProcessTime");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessPrivate()
{
    Element_Name("ChapProcessPrivate");

    //Parsing
    Skip_XX(Element_Size,                                       "Data");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay()
{
    Element_Name("ChapterDisplay");

    //Filling
    ChapterDisplays_Pos=EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterDisplays.size();
    EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterDisplays.resize(ChapterDisplays_Pos+1);
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapCountry()
{
    Element_Name("ChapCountry");

    //Parsing
    Local_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapLanguage()
{
    Element_Name("ChapLanguage");

    //Parsing
    Ztring Data=Local_Get();

    FILLING_BEGIN();
        EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterDisplays[ChapterDisplays_Pos].ChapLanguage=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapString()
{
    Element_Name("ChapString");

    //Parsing
    Ztring Data=UTF8_Get();

    FILLING_BEGIN();
        EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterDisplays[ChapterDisplays_Pos].ChapString=Data;
        //if (TimecodeScale!=0 && ChapterTimeStart!=(int64u)-1)
        //    Fill(StreamKind_Last, StreamPos_Last, Ztring::ToZtring(Chapter_Pos).To_Local().c_str(), Ztring().Duration_From_Milliseconds(ChapterTimeStart/TimecodeScale)+__T(" - ")+ChapterString, true);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterFlagHidden()
{
    Element_Name("ChapterFlagHidden");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterFlagEnabled()
{
    Element_Name("ChapterFlagEnabled");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterPhysicalEquiv()
{
    Element_Name("ChapterPhysicalEquiv");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterSegmentEditionUID()
{
    Element_Name("ChapterSegmentEditionUID");

    //Parsing
    Skip_XX(Element_Size,                                       "Data");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterSegmentUID()
{
    Element_Name("ChapterSegmentUID");

    //Parsing
    Skip_XX(Element_Size,                                       "Data");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterTimeEnd()
{
    Element_Name("ChapterTimeEnd");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterTimeStart()
{
    Element_Name("ChapterTimeStart");

    //Parsing
    int64u Data=UInteger_Get();

    FILLING_BEGIN();
        EditionEntries[EditionEntries_Pos].ChapterAtoms[ChapterAtoms_Pos].ChapterTimeStart=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterTrack()
{
    Element_Name("ChapterTrack");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterTrack_ChapterTrackNumber()
{
    Element_Name("ChapterTrackNumber");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_ChapterAtom_ChapterUID()
{
    Element_Name("ChapterUID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_EditionFlagDefault()
{
    Element_Name("EditionFlagDefault");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_EditionFlagHidden()
{
    Element_Name("EditionFlagHidden");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_EditionFlagOrdered()
{
    Element_Name("EditionFlagOrdered");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Chapters_EditionEntry_EditionUID()
{
    Element_Name("EditionUID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster()
{
    Element_Name("Cluster");

    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            if (Trace_Segment_Cluster_Count<MaxCountSameElementInTrace)
                Trace_Segment_Cluster_Count++;
            else
                Element_Set_Remove_Children_IfNoErrors();
        }
    #endif // MEDIAINFO_TRACE

    //For each stream
    std::map<int64u, stream>::iterator Temp=Stream.begin();
    if (!Segment_Cluster_Count)
    {
        Stream_Count=0;
        while (Temp!=Stream.end())
        {
            if (Temp->second.Parser)
                Temp->second.Searching_Payload=true;
            if (Temp->second.StreamKind==Stream_Video || Temp->second.StreamKind==Stream_Audio)
                Temp->second.Searching_TimeStamp_Start=true;
            if (Temp->second.StreamKind==Stream_Video)
                Temp->second.Searching_TimeStamps=true;
            if (Temp->second.Searching_Payload
             || Temp->second.Searching_TimeStamp_Start
             || Temp->second.Searching_TimeStamps)
                Stream_Count++;

            //Specific cases
            #ifdef MEDIAINFO_AAC_YES
                if (Retrieve(Temp->second.StreamKind, Temp->second.StreamPos, Audio_CodecID).find(__T("A_AAC/"))==0)
                    ((File_Aac*)Stream[Temp->first].Parser)->Mode=File_Aac::Mode_raw_data_block; //In case AudioSpecificConfig is not present
            #endif //MEDIAINFO_AAC_YES

            ++Temp;
        }

        //We must parse moov?
        if (Stream_Count==0)
        {
            //Jumping
            std::sort(Segment_Seeks.begin(), Segment_Seeks.end());
            for (size_t Pos=0; Pos<Segment_Seeks.size(); Pos++)
                if (Segment_Seeks[Pos]>File_Offset+Buffer_Offset+Element_Size)
                {
                    JumpTo(Segment_Seeks[Pos]);
                    break;
                }
            if (File_GoTo==(int64u)-1)
                JumpTo(Segment_Offset_End);
            return;
        }
    }
    Segment_Cluster_Count++;
    Segment_Cluster_TimeCode_Value=0; //Default
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup()
{
    Element_Name("BlockGroup");

    Segment_Cluster_BlockGroup_BlockDuration_Value=(int64u)-1;
    Segment_Cluster_BlockGroup_BlockDuration_TrackNumber=(int64u)-1;
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_Block()
{
    if (!Element_IsComplete_Get())
        return;

    Element_Name((Element_Level==3)?"SimpleBlock":"Block");

    //Parsing
    Get_EB (TrackNumber,                                        "TrackNumber"); Element_Info1(TrackNumber);

    //Finished?
    stream& streamItem = Stream[TrackNumber];
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            if (streamItem.Trace_Segment_Cluster_Block_Count<=MaxCountSameElementInTrace)
                streamItem.Trace_Segment_Cluster_Block_Count++;
            //else
            //    Element_Set_Remove_Children_IfNoErrors();
        }
    #endif // MEDIAINFO_TRACE
    streamItem.PacketCount++;
    if (streamItem.Searching_Payload || streamItem.Searching_TimeStamps || streamItem.Searching_TimeStamp_Start)
    {
        //Parsing
        int16u TimeCodeU;
        Get_B2 (TimeCodeU,                                      "TimeCode"); // Should be signed, but we don't have signed integer reader
        int16s TimeCode = (int16s)TimeCodeU;
        Element_Info1(TimeCodeU);
        #if MEDIAINFO_DEMUX
        FrameInfo.PTS=(Segment_Cluster_TimeCode_Value+TimeCode)*1000000;
        #endif //MEDIAINFO_DEMUX

        FILLING_BEGIN();
            if (Segment_Cluster_TimeCode_Value+TimeCode<streamItem.TimeCode_Start) //Does not work well: streamItem.Searching_TimeStamp_Start)
            {
                FILLING_BEGIN();
                      streamItem.TimeCode_Start=Segment_Cluster_TimeCode_Value+TimeCode;
                    //streamItem.Searching_TimeStamp_Start=false;
                FILLING_END();
            }
            if (streamItem.Searching_TimeStamps)
            {
                streamItem.TimeCodes.push_back(Segment_Cluster_TimeCode_Value+TimeCode);
                if (streamItem.TimeCodes.size()>128)
                    streamItem.Searching_TimeStamps=false;
            }

            if (Segment_Cluster_BlockGroup_BlockDuration_Value!=(int64u)-1)
            {
                streamItem.Segment_Cluster_BlockGroup_BlockDuration_Counts[Segment_Cluster_BlockGroup_BlockDuration_Value]++;
                Segment_Cluster_BlockGroup_BlockDuration_Value=(int64u)-1;
            }
        FILLING_END();

        if (streamItem.Searching_Payload)
        {
            int32u Lacing;
            Element_Begin1("Flags");
                BS_Begin();
                Skip_BS(1,                                      "KeyFrame");
                Skip_BS(3,                                      "Reserved");
                Skip_BS(1,                                      "Invisible");
                Get_BS (2, Lacing,                              "Lacing");
                Skip_BS(1,                                      "Discardable");
                BS_End();
            Element_End0();
            if (Lacing>0)
            {
                Element_Begin1("Lacing");
                    int8u FrameCountMinus1;
                    Get_B1(FrameCountMinus1,                    "Frame count minus 1");
                    switch (Lacing)
                    {
                        case 1 : //Xiph lacing
                                {
                                    int64u Element_Offset_Virtual=0;
                                    for (int8u Pos=0; Pos<FrameCountMinus1; Pos++)
                                    {
                                        int32u Size=0;
                                        int8u Size8;
                                        do
                                        {
                                            Get_B1 (Size8,      "Size");
                                            Size+=Size8;
                                        }
                                        while (Size8==0xFF);
                                        Param_Info1(Size);
                                        Element_Offset_Virtual+=Size;
                                        Laces.push_back(Size);
                                    }
                                    if (Element_Offset+Element_Offset_Virtual>Element_Size)
                                    {
                                        //Problem
                                        Laces.clear();
                                        Laces.push_back(Element_Size - Element_Offset);
                                    }
                                    else
                                        Laces.push_back(Element_Size-Element_Offset-Element_Offset_Virtual); //last lace
                                }
                                break;
                        case 2 : //Fixed-size lacing - No more data
                                {
                                    int64u Size=(Element_Size-Element_Offset)/(FrameCountMinus1+1);
                                    Laces.resize(FrameCountMinus1+1, Size);
                                }
                                break;
                        case 3 : //EBML lacing
                                {
                                    int64u Element_Offset_Virtual=0, Size;
                                    Get_EB (Size,                "Size");
                                    Laces.push_back(Size);
                                    Element_Offset_Virtual+=Size;
                                    for (int8u Pos=1; Pos<FrameCountMinus1; Pos++)
                                    {
                                        int64s Diff;
                                        Get_ES (Diff,           "Difference");
                                        Size+=Diff; Param_Info1(Size);
                                        Element_Offset_Virtual+=Size;
                                        Laces.push_back(Size);
                                    }
                                    if (Element_Offset+Element_Offset_Virtual>Element_Size)
                                    {
                                        //Problem
                                        Laces.clear();
                                        Laces.push_back(Element_Size - Element_Offset);
                                    }
                                    else
                                        Laces.push_back(Element_Size-Element_Offset-Element_Offset_Virtual); Param_Info1(Size); //last lace
                                }
                                break;
                        default : ; //Should never be here
                    }
                Element_End0();
            }
            else
                Laces.push_back(Element_Size-Element_Offset);
        }
        else
        {
            Laces.push_back(Element_Size-Element_Offset);
        }
    }
    else
    {
        Laces.push_back(Element_Size-Element_Offset);
    }

    if (Laces.size()==1)
    {
        Element_Begin1("Data");
        Segment_Cluster_BlockGroup_Block_Lace();
        Element_End0();
    }

    #if MEDIAINFO_TRACE
        if (Trace_Activated && (Trace_Segment_Cluster_Count>MaxCountSameElementInTrace || streamItem.Trace_Segment_Cluster_Block_Count>MaxCountSameElementInTrace))
            Element_Children_IfNoErrors();
    #endif // MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_Block_Lace()
{
    stream& streamItem=Stream[TrackNumber];

    //Content compression
    if (streamItem.ContentCompAlgo!=(int32u)-1 && streamItem.ContentCompAlgo!=3)
        streamItem.Searching_Payload=false; //Unsupported

    if (streamItem.Searching_Payload)
    {
        Element_Parser(streamItem.Parser->ParserName.c_str());

        Element_Code=TrackNumber;

        //Content compression
        /* Old method, does not support all needs e.g. 1 complete frame per demux packet
        if (streamItem.ContentCompAlgo==3) //Header Stripping
        {
            Element_Offset-=(size_t)streamItem.ContentCompSettings_Buffer_Size; //This is an extra array, not in the stream
            Open_Buffer_Continue(streamItem.Parser, streamItem.ContentCompSettings_Buffer, (size_t)streamItem.ContentCompSettings_Buffer_Size);
            Element_Offset+=(size_t)streamItem.ContentCompSettings_Buffer_Size;
            Demux(streamItem.ContentCompSettings_Buffer, (size_t)streamItem.ContentCompSettings_Buffer_Size, ContentType_MainStream);
        }
        */
        int8u* Buffer_Stripping=NULL;
        const int8u* Save_Buffer=Buffer;
        int64u Save_File_Offset=File_Offset;
        size_t Save_Buffer_Offset=Buffer_Offset;
        int64u Save_Element_Size=Element_Size;
        size_t Save_Element_Offset=(size_t)Element_Offset;
        if (streamItem.ContentCompAlgo==3) //Header Stripping
        {
            Element_Size=streamItem.ContentCompSettings_Buffer_Size+Save_Element_Size-Save_Element_Offset;
            File_Offset+=Buffer_Offset+Element_Offset-streamItem.ContentCompSettings_Buffer_Size;
            Buffer_Offset=0;
            Element_Offset=0;
            Buffer_Stripping=new int8u[(size_t)Element_Size];
            std::memcpy(Buffer_Stripping, streamItem.ContentCompSettings_Buffer, streamItem.ContentCompSettings_Buffer_Size);
            std::memcpy(Buffer_Stripping+streamItem.ContentCompSettings_Buffer_Size, Save_Buffer+Save_Buffer_Offset+Save_Element_Offset, Save_Element_Size-Save_Element_Offset);
            Buffer=Buffer_Stripping;
        }

        //Parsing
        if(Laces_Pos)
            FrameInfo.PTS=streamItem.Parser->FrameInfo.PTS;
        else
            streamItem.Parser->FrameInfo.PTS=FrameInfo.PTS;
        Frame_Count_NotParsedIncluded=(streamItem.PacketCount==1 && !Laces_Pos)?0:streamItem.Parser->Frame_Count_NotParsedIncluded;
        #if MEDIAINFO_DEMUX
            int8u Demux_Level_old=Demux_Level;
            if (streamItem.Parser && streamItem.Parser->Demux_Level==2)
                Demux_Level=4;
            Demux(Buffer+Buffer_Offset+(size_t)Element_Offset, (size_t)(Element_Size-Element_Offset), ContentType_MainStream);
            Demux_Level=Demux_Level_old;
            streamItem.Parser->FrameInfo.PTS=FrameInfo.PTS;
        #endif //MEDIAINFO_DEMUX
            Open_Buffer_Continue(streamItem.Parser, (size_t)(Element_Size-Element_Offset));
        if (streamItem.Parser->Status[IsFinished]
            || (streamItem.PacketCount>=300 && MediaInfoLib::Config.ParseSpeed_Get()<1))
        {
            streamItem.Searching_Payload=false;
            if (!streamItem.Searching_TimeStamps && !streamItem.Searching_TimeStamp_Start)
                Stream_Count--;
        }
        FrameInfo.PTS=(int64u)-1;
        Frame_Count_NotParsedIncluded=(int64u)-1;

        #if MEDIAINFO_DEMUX
            if (Config->Demux_EventWasSent && Config->Demux_Unpacketize_Get())
                Demux_EventWasSent=Element_Code;
        #endif //MEDIAINFO_DEMUX

        if (Save_Buffer!=Buffer)
        {
            //We must change the buffer for keeping out
            Element_Offset=Save_Element_Size;
            Element_Size=Save_Element_Size;
            File_Offset=Save_File_Offset;
            Buffer_Offset=Save_Buffer_Offset;
            delete[] Buffer; Buffer=Save_Buffer;
        }
    }
    else
        Skip_XX(Element_Size-Element_Offset,                    "Data");

    //Filling
    Frame_Count++;
    if (!Status[IsFilled] && ((Frame_Count>6 && (Stream_Count==0 ||Config->ParseSpeed==0.0)) || Frame_Count>512*Stream.size()))
    {
        Fill();
        if (MediaInfoLib::Config.ParseSpeed_Get()<1)
        {
            //Jumping
            std::sort(Segment_Seeks.begin(), Segment_Seeks.end());
            for (size_t Pos=0; Pos<Segment_Seeks.size(); Pos++)
                if (Segment_Seeks[Pos]>File_Offset+Buffer_Offset+Element_Size)
                {
                    JumpTo(Segment_Seeks[Pos]);
                    Open_Buffer_Unsynch();
                    break;
                }
            if (File_GoTo==(int64u)-1)
            {
                JumpTo(Segment_Offset_End);
                Open_Buffer_Unsynch();
            }
        }

        Laces.clear();
    }

    Laces_Pos++;
    if (Laces_Pos>=Laces.size())
    {
        Laces.clear();
        Laces_Pos=0;
    }

    Element_Show();

    #if MEDIAINFO_TRACE
        if (Trace_Activated && (Trace_Segment_Cluster_Count>MaxCountSameElementInTrace || streamItem.Trace_Segment_Cluster_Block_Count>MaxCountSameElementInTrace))
            Element_Children_IfNoErrors();
    #endif // MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_BlockAdditions()
{
    Element_Name("BlockAdditions");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_BlockAdditions_BlockMore()
{
    Element_Name("BlockMore");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_BlockAdditions_BlockMore_BlockAddID()
{
    Element_Name("BlockAddID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_BlockAdditions_BlockMore_BlockAdditional()
{
    Element_Name("BlockAdditional");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_BlockDuration()
{
    Element_Name("BlockDuration");

    //Parsing
    int64u Segment_Cluster_TimeCode_Value=UInteger_Get();

    FILLING_BEGIN();
        if (Segment_Cluster_BlockGroup_BlockDuration_TrackNumber!=(int64u)-1)
        {
            Stream[Segment_Cluster_BlockGroup_BlockDuration_TrackNumber].Segment_Cluster_BlockGroup_BlockDuration_Counts[Segment_Cluster_TimeCode_Value]++;
            Segment_Cluster_BlockGroup_BlockDuration_TrackNumber=(int64u)-1;
        }
        else
            Segment_Cluster_BlockGroup_BlockDuration_Value=Segment_Cluster_TimeCode_Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_ReferencePriority()
{
    Element_Name("ReferencePriority");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_ReferenceBlock()
{
    Element_Name("ReferenceBlock");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_ReferenceVirtual()
{
    Element_Name("ReferenceVirtual");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_CodecState()
{
    Element_Name("CodecState");

    //Parsing
    Skip_XX(Element_Size,                                       "Data");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_Slices()
{
    Element_Name("Slices");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_Slices_TimeSlice()
{
    Element_Name("TimeSlice");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_Slices_TimeSlice_Duration()
{
    Element_Name("Duration");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_BlockGroup_Slices_TimeSlice_LaceNumber()
{
    Element_Name("LaceNumber");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_Position()
{
    Element_Name("Position");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_PrevSize()
{
    Element_Name("PrevSize");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_SilentTracks()
{
    Element_Name("SilentTracks");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_SilentTracks_SilentTrackNumber()
{
    Element_Name("SilentTrackNumber");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_SimpleBlock()
{
    Segment_Cluster_BlockGroup_BlockDuration_Value=(int64u)-1;
    Segment_Cluster_BlockGroup_BlockDuration_TrackNumber=(int64u)-1;

    Segment_Cluster_BlockGroup_Block();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cluster_Timecode()
{
    Element_Name("Timecode");

    //Parsing
    Segment_Cluster_TimeCode_Value=UInteger_Get();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cues()
{
    Element_Name("Cues");

    //Skipping Cues, we don't need of them
    TestMultipleInstances();

    #if MEDIAINFO_TRACE
        Trace_Segment_Cues_CuePoint_Count=0;
    #endif // MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cues_CuePoint()
{
    Element_Name("CuePoint");

    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            if (Trace_Segment_Cues_CuePoint_Count<MaxCountSameElementInTrace)
                Trace_Segment_Cues_CuePoint_Count++;
            else
                Element_Set_Remove_Children_IfNoErrors();
        }
    #endif // MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cues_CuePoint_CueTime()
{
    Element_Name("CueTime");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cues_CuePoint_CueTrackPositions()
{
    Element_Name("CueTrackPositions");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cues_CuePoint_CueTrackPositions_CueTrack()
{
    Element_Name("CueTrack");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cues_CuePoint_CueTrackPositions_CueClusterPosition()
{
    Element_Name("CueClusterPosition");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cues_CuePoint_CueTrackPositions_CueRelativePosition()
{
    Element_Name("CueRelativePosition");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cues_CuePoint_CueTrackPositions_CueDuration()
{
    Element_Name("CueDuration");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Cues_CuePoint_CueTrackPositions_CueBlockNumber()
{
    Element_Name("CueBlockNumber");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info()
{
    Element_Name("Info");

    TestMultipleInstances(&Segment_Info_Count);
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_ChapterTranslate()
{
    Element_Name("ChapterTranslate");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_ChapterTranslate_ChapterTranslateCodec()
{
    Element_Name("ChapterTranslateCodec");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_ChapterTranslate_ChapterTranslateEditionUID()
{
    Element_Name("ChapterTranslateEditionUID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_ChapterTranslate_ChapterTranslateID()
{
    Element_Name("ChapterTranslateID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_DateUTC()
{
    Element_Name("DateUTC");

    //Parsing
    int64u Data;
    Get_B8(Data,                                                "Data"); Element_Info1(Data/1000000000+978307200); //From Beginning of the millenium, in nanoseconds

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_General, 0, "Encoded_Date", Ztring().Date_From_Seconds_1970((int32u)(Data/1000000000+978307200))); //978307200s between beginning of the millenium and 1970
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_Duration()
{
    Element_Name("Duration");

    //Parsing
    float64 Float=Float_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Duration=Float;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_MuxingApp()
{
    Element_Name("MuxingApp");

    //Parsing
    Ztring Data=UTF8_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_General, 0, "Encoded_Library", Data);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_NextFilename()
{
    Element_Name("NextFilename");

    //Parsing
    UTF8_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_NextUID()
{
    Element_Name("NextUID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_PrevFilename()
{
    Element_Name("PrevFilename");

    //Parsing
    UTF8_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_PrevUID()
{
    Element_Name("PrevUID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_SegmentFamily()
{
    Element_Name("SegmentFamily");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_SegmentFilename()
{
    Element_Name("SegmentFilename");

    //Parsing
    UTF8_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_SegmentUID()
{
    Element_Name("SegmentUID");

    //Parsing
    int128u Data;
    Data=UInteger16_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_General, 0, General_UniqueID, Ztring().From_Local(Data.toString(10)));
        Fill(Stream_General, 0, General_UniqueID_String, Ztring().From_Local(Data.toString(10))+__T(" (0x")+Ztring().From_Local(Data.toString(16))+__T(')'));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_TimecodeScale()
{
    Element_Name("TimecodeScale");

    //Parsing
    int64u UInteger=UInteger_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        TimecodeScale=UInteger;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_Title()
{
    Element_Name("Title");

    //Parsing
    Ztring Data=UTF8_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_General, 0, "Title", Data);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Info_WritingApp()
{
    Element_Name("WritingApp");

    //Parsing
    Ztring Data=UTF8_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_General, 0, "Encoded_Application", Data);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_SeekHead()
{
    Element_Name("SeekHead");

    Segment_Seeks.clear();

    #if MEDIAINFO_TRACE
        Trace_Segment_SeekHead_Seek_Count=0;
    #endif // MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Mk::Segment_SeekHead_Seek()
{
    Element_Name("Seek");

    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            if (Trace_Segment_SeekHead_Seek_Count<MaxCountSameElementInTrace)
                Trace_Segment_SeekHead_Seek_Count++;
            else
                Element_Set_Remove_Children_IfNoErrors();
        }
    #endif // MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Mk::Segment_SeekHead_Seek_SeekID()
{
    Element_Name("SeekID");

    //Parsing
    int64u Data;
    Get_EB (Data,                                               "Data");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_SeekHead_Seek_SeekPosition()
{
    Element_Name("SeekPosition");

    //Parsing
    int64u Data=UInteger_Get();

    Segment_Seeks.push_back(Segment_Offset_Begin+Data);
    Element_Info1(Ztring::ToZtring(Segment_Offset_Begin+Data, 16));
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags()
{
    Element_Name("Tags");

    Segment_Tag_SimpleTag_TagNames.clear();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag()
{
    Element_Name("Tag");

    //Previous tags
    tags::iterator Items0 = Segment_Tags_Tag_Items.find((int64u)-1);
    if (Items0 != Segment_Tags_Tag_Items.end())
    {
        tagspertrack &Items = Segment_Tags_Tag_Items[0]; // Creates it if not yet present, else take the previous one

        //Change the key of the current tag
        for (tagspertrack::iterator Item=Items0->second.begin(); Item!=Items0->second.end(); ++Item)
            Items[Item->first] = Item->second;
        Segment_Tags_Tag_Items.erase(Items0);
    }

    //Init
    Segment_Tags_Tag_Targets_TagTrackUID_Value=0; // Default is all tracks
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_SimpleTag()
{
    Element_Name("SimpleTag");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_SimpleTag_TagBinary()
{
    Element_Name("TagBinary");

    //Parsing
    Skip_XX(Element_Size,                                       "Data");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_SimpleTag_TagDefault()
{
    Element_Name("TagDefault");

    //Parsing
    UInteger_Get();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_SimpleTag_TagLanguage()
{
    Element_Name("TagLanguage");

    //Parsing
    Ztring Data;
    Get_Local(Element_Size, Data,                              "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        //Fill(StreamKind_Last, StreamPos_Last, "Language", Data);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_SimpleTag_TagName()
{
    Element_Name("TagName");

    //Parsing
    Ztring TagName=UTF8_Get();

    Segment_Tag_SimpleTag_TagNames.resize(Element_Level-5); //5 is the first level of a tag
    Segment_Tag_SimpleTag_TagNames.push_back(TagName);
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_SimpleTag_TagString()
{
    Element_Name("TagString");

    //Parsing
    Ztring TagString;
    TagString=UTF8_Get();

    if (Segment_Tag_SimpleTag_TagNames.empty())
        return;
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("AERMS_OF_USE")) Segment_Tag_SimpleTag_TagNames[0]=__T("TermsOfUse"); //Typo in the source file
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("BITSPS")) return; //Useless
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("COMPATIBLE_BRANDS")) return; //QuickTime techinical info, useless
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("CONTENT_TYPE")) Segment_Tag_SimpleTag_TagNames[0]=__T("ContentType");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("COPYRIGHT")) Segment_Tag_SimpleTag_TagNames[0]=__T("Copyright");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("CREATION_TIME")) {Segment_Tag_SimpleTag_TagNames[0]=__T("Encoded_Date"); TagString.insert(0, __T("UTC "));}
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("DATE_DIGITIZED")) {Segment_Tag_SimpleTag_TagNames[0]=__T("Mastered_Date"); TagString.insert(0, __T("UTC "));}
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("DATE_RELEASE")) Segment_Tag_SimpleTag_TagNames[0]=__T("Released_Date");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("DATE_RELEASED")) Segment_Tag_SimpleTag_TagNames[0]=__T("Released_Date");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("DESCRIPTION")) Segment_Tag_SimpleTag_TagNames[0]=__T("Description");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("ENCODED_BY")) Segment_Tag_SimpleTag_TagNames[0]=__T("EncodedBy");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("ENCODER")) Segment_Tag_SimpleTag_TagNames[0]=__T("Encoded_Library");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("FPS")) return; //Useless
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("LANGUAGE")) Segment_Tag_SimpleTag_TagNames[0]=__T("Language");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("MAJOR_BRAND")) return; //QuickTime techinical info, useless
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("MINOR_VERSION")) return; //QuickTime techinical info, useless
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("PART_NUMBER")) Segment_Tag_SimpleTag_TagNames[0]=__T("Track/Position");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("ORIGINAL_MEDIA_TYPE")) Segment_Tag_SimpleTag_TagNames[0]=__T("OriginalSourceForm");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("SAMPLE") && Segment_Tag_SimpleTag_TagNames.size()==2 && Segment_Tag_SimpleTag_TagNames[1]==__T("PART_NUMBER")) return; //Useless
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("SAMPLE") && Segment_Tag_SimpleTag_TagNames.size()==2 && Segment_Tag_SimpleTag_TagNames[1]==__T("TITLE")) {Segment_Tag_SimpleTag_TagNames.resize(1); Segment_Tag_SimpleTag_TagNames[0]=__T("Title_More");}
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("STEREO_MODE")) return; //Useless
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("TERMS_OF_USE")) Segment_Tag_SimpleTag_TagNames[0]=__T("TermsOfUse");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("TITLE")) Segment_Tag_SimpleTag_TagNames[0]=__T("Title");
    if (Segment_Tag_SimpleTag_TagNames[0]==__T("TOTAL_PARTS")) Segment_Tag_SimpleTag_TagNames[0]=__T("Track/Position_Total");
    for (size_t Pos=0; Pos<Segment_Tag_SimpleTag_TagNames.size(); Pos++)
    {
        if (Segment_Tag_SimpleTag_TagNames[Pos]==__T("BARCODE")) Segment_Tag_SimpleTag_TagNames[Pos]=__T("BarCode");
        if (Segment_Tag_SimpleTag_TagNames[Pos]==__T("COMMENT")) Segment_Tag_SimpleTag_TagNames[Pos]=__T("Comment");
        if (Segment_Tag_SimpleTag_TagNames[Pos]==__T("ORIGINAL")) Segment_Tag_SimpleTag_TagNames[Pos]=__T("Original");
        if (Segment_Tag_SimpleTag_TagNames[Pos]==__T("URL")) Segment_Tag_SimpleTag_TagNames[Pos]=__T("Url");
    }

    Ztring TagName;
    for (size_t Pos=0; Pos<Segment_Tag_SimpleTag_TagNames.size(); Pos++)
    {
        TagName+=Segment_Tag_SimpleTag_TagNames[Pos];
        if (Pos+1<Segment_Tag_SimpleTag_TagNames.size())
            TagName+=__T('/');
    }

    Segment_Tags_Tag_Items[Segment_Tags_Tag_Targets_TagTrackUID_Value][TagName]=TagString;
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_Targets()
{
    Element_Name("Targets");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_Targets_TargetTypeValue()
{
    Element_Name("TargetTypeValue");

    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_Targets_TargetType()
{
    Element_Name("TargetType");

    UTF8_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_Targets_TagTrackUID()
{
    Element_Name("TagTrackUID");

    //Parsing
    Segment_Tags_Tag_Targets_TagTrackUID_Value=UInteger_Get();

    FILLING_BEGIN();
        tags::iterator Items0 = Segment_Tags_Tag_Items.find((int64u)-1);
        if (Items0 != Segment_Tags_Tag_Items.end())
        {
            tagspertrack &Items = Segment_Tags_Tag_Items[Segment_Tags_Tag_Targets_TagTrackUID_Value]; // Creates it if not yet present, else take the previous one

            //Change the key of the current tag
            for (tagspertrack::iterator Item=Items0->second.begin(); Item!=Items0->second.end(); ++Item)
                Items[Item->first] = Item->second;
            Segment_Tags_Tag_Items.erase(Items0);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_Targets_TagEditionUID()
{
    Element_Name("TagEditionUID");

    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_Targets_TagChapterUID()
{
    Element_Name("TagChapterUID");

    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tags_Tag_Targets_TagAttachmentUID()
{
    Element_Name("TagAttachmentUID");

    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks()
{
    Element_Name("Tracks");

    TestMultipleInstances(&Segment_Tracks_Count);
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry()
{
    Element_Name("TrackEntry");

    if (Segment_Info_Count>1)
        return; //First element has the priority

    //Clearing
    CodecID.clear();
    InfoCodecID_Format_Type=InfoCodecID_Format_Matroska;
    TrackType=(int64u)-1;
    TrackNumber=(int64u)-1;
    TrackVideoDisplayWidth=0;
    TrackVideoDisplayHeight=0;
    AvgBytesPerSec=0;

    //Preparing
    Stream_Prepare(Stream_Max);

    //Default values
    Fill_Flush();
    Fill(StreamKind_Last, StreamPos_Last, "Language", "eng");
    Fill(StreamKind_Last, StreamPos_Last, General_StreamOrder, Stream.size());
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_AttachmentLink()
{
    Element_Name("AttachmentLink");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Audio()
{
    Element_Name("Audio");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Audio_BitDepth()
{
    Element_Name("BitDepth");

    //Parsing
    int64u UInteger=UInteger_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        if (UInteger)
            Fill(StreamKind_Last, StreamPos_Last, "BitDepth", UInteger, 10, true);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Audio_Channels()
{
    Element_Name("Channels");

    //Parsing
    int64u UInteger=UInteger_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, UInteger, 10, true);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Audio_OutputSamplingFrequency()
{
    Element_Name("OutputSamplingFrequency");

    //Parsing
    float64 Float=Float_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, Float, 0, true);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Audio_SamplingFrequency()
{
    Element_Name("SamplingFrequency");

    //Parsing
    float64 Float=Float_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, Float, 0, true);
        #ifdef MEDIAINFO_AAC_YES
            if (Retrieve(Stream_Audio, StreamPos_Last, Audio_CodecID).find(__T("A_AAC/"))==0)
                ((File_Aac*)Stream[TrackNumber].Parser)->AudioSpecificConfig_OutOfBand(float64_int64s(Float));
        #endif //MEDIAINFO_AAC_YES
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecSettings()
{
    Element_Name("CodecSettings");

    //Parsing
    UTF8_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecInfoURL()
{
    Element_Name("CodecInfoURL");

    //Parsing
    Local_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecDownloadURL()
{
    Element_Name("CodecDecodeAll");

    //Parsing
    Local_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecDecodeAll()
{
    Element_Name("CodecDecodeAll");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecID()
{
    Element_Name("CodecID");

    //Parsing
    Ztring Data;
    Get_Local(Element_Size, Data,                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        CodecID=Data;
        CodecID_Manage();
        CodecPrivate_Manage();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression()
{
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].ContentCompAlgo=0; //0 is default
        Fill(StreamKind_Last, StreamPos_Last, "MuxingMode", Mk_ContentCompAlgo(0), Unlimited, true, true);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression_ContentCompAlgo()
{
    //Parsing
    int64u Algo=UInteger_Get(); Param_Info1(Mk_ContentCompAlgo(Algo));

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].ContentCompAlgo=Algo;
        Fill(StreamKind_Last, StreamPos_Last, "MuxingMode", Mk_ContentCompAlgo(Algo), Unlimited, true, true);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression_ContentCompSettings()
{
    //Parsing
    Skip_XX(Element_Size,                                       "Data");

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        stream& streamItem = Stream[TrackNumber];
        streamItem.ContentCompSettings_Buffer=new int8u[(size_t)Element_Size];
        std::memcpy(streamItem.ContentCompSettings_Buffer, Buffer+Buffer_Offset, (size_t)Element_Size);
        streamItem.ContentCompSettings_Buffer_Size=(size_t)Element_Size;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecName()
{
    Element_Name("CodecName");

    //Parsing
    UTF8_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecPrivate()
{
    Element_Name("CodecPrivate");

    if (Segment_Info_Count>1)
    {
        Skip_XX(Element_Size,                                   "Data (not parsed)");  
        return; //First element has the priority
    }
    if (TrackNumber==(int64u)-1 || TrackType==(int64u)-1 || Retrieve(Stream[TrackNumber].StreamKind, Stream[TrackNumber].StreamPos, "CodecID").empty())
    {
        //Codec not already known, saving CodecPrivate
        if (CodecPrivate)
            delete[] CodecPrivate; //CodecPrivate=NULL.
        CodecPrivate_Size=(size_t)Element_Size;
        CodecPrivate=new int8u[CodecPrivate_Size];
        std::memcpy(CodecPrivate, Buffer+Buffer_Offset, CodecPrivate_Size);
        return;
    }

    Segment_Tracks_TrackEntry_CodecPrivate__Parse();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecPrivate__Parse()
{
    //Creating the parser
    stream& streamItem = Stream[TrackNumber];
    if (streamItem.Parser==NULL)
    {
        if (streamItem.StreamKind==Stream_Audio && Retrieve(Stream_Audio, streamItem.StreamPos, Audio_CodecID)==__T("A_MS/ACM"))
            Segment_Tracks_TrackEntry_CodecPrivate_auds();
        else if (streamItem.StreamKind==Stream_Video && Retrieve(Stream_Video, streamItem.StreamPos, Video_CodecID)==__T("V_MS/VFW/FOURCC"))
            Segment_Tracks_TrackEntry_CodecPrivate_vids();
        else if (Element_Size>0)
            Skip_XX(Element_Size,                 "Unknown");
        return;
    }

    #if MEDIAINFO_DEMUX
        switch (Config->Demux_InitData_Get())
        {
            case 0 :    //In demux event
                        {
                        Demux_Level=2; //Container
                        int64u Element_Code_Old=Element_Code;
                        Element_Code=TrackNumber;
                        Demux(Buffer+Buffer_Offset, (size_t)Element_Size, ContentType_Header);
                        Element_Code=Element_Code_Old;
                        }
                        break;
            case 1 :    //In field
                        {
                        std::string Data_Raw((const char*)(Buffer+Buffer_Offset), (size_t)Element_Size);
                        std::string Data_Base64(Base64::encode(Data_Raw));
                        Fill(StreamKind_Last, StreamPos_Last, "Demux_InitBytes", Data_Base64);
                        Fill_SetOptions(StreamKind_Last, StreamPos_Last, "Demux_InitBytes", "N NT");
                        }
                        break;
            default :   ;
        }
    #endif // MEDIAINFO_DEMUX

    //Parsing
    Open_Buffer_OutOfBand(streamItem.Parser);

    //Filling
    if (streamItem.Parser->Status[IsFinished]) //Can be finnished here...
    {
        streamItem.Searching_Payload=false;
        Stream_Count--;
    }

    //In case of problem
    Element_Show();
}

//--------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecPrivate_auds()
{
    Element_Info1("Copy of auds");

    //Parsing
    int32u SamplesPerSec;
    int16u FormatTag, Channels, BitsPerSample;
    Get_L2 (FormatTag,                                          "FormatTag");
    Get_L2 (Channels,                                           "Channels");
    Get_L4 (SamplesPerSec,                                      "SamplesPerSec");
    Get_L4 (AvgBytesPerSec,                                     "AvgBytesPerSec");
    Skip_L2(                                                    "BlockAlign");
    Get_L2 (BitsPerSample,                                      "BitsPerSample");

    //Filling
    FILLING_BEGIN();
        InfoCodecID_Format_Type=InfoCodecID_Format_Riff;
        CodecID.From_Number(FormatTag, 16);
        CodecID_Fill(CodecID, Stream_Audio, StreamPos_Last, InfoCodecID_Format_Riff);
        Fill(Stream_Audio, StreamPos_Last, Audio_Codec, CodecID, true); //May be replaced by codec parser
        Fill(Stream_Audio, StreamPos_Last, Audio_Codec_CC, CodecID);
        Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, Channels!=5?Channels:6, 10, true);
        Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, SamplesPerSec, 10, true);
        Fill(Stream_Audio, StreamPos_Last, Audio_BitRate, AvgBytesPerSec*8, 10, true);
        if (BitsPerSample)
            Fill(Stream_Audio, StreamPos_Last, Audio_BitDepth, BitsPerSample);

        CodecID_Manage();
        if (TrackNumber!=(int64u)-1)
            Stream[TrackNumber].AvgBytesPerSec=AvgBytesPerSec;
    FILLING_END();

    //Options
    if (Element_Offset+2>Element_Size)
        return; //No options

    //Parsing
    int16u Option_Size;
    Get_L2 (Option_Size,                                        "cbSize");

    //Filling
    if (Option_Size>0)
    {
             if (FormatTag==0xFFFE) //Extensible Wave
            Segment_Tracks_TrackEntry_CodecPrivate_auds_ExtensibleWave();
        else Skip_XX(Option_Size,                               "Unknown");
    }
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecPrivate_auds_ExtensibleWave()
{
    //Parsing
    int128u SubFormat;
    int32u ChannelMask;
    Skip_L2(                                                    "ValidBitsPerSample / SamplesPerBlock");
    Get_L4 (ChannelMask,                                        "ChannelMask");
    Get_GUID(SubFormat,                                         "SubFormat");

    FILLING_BEGIN();
        if ((SubFormat.hi&0xFFFFFFFFFFFF0000LL)==0x0010000000000000LL && SubFormat.lo==0x800000AA00389B71LL)
        {
            CodecID_Fill(Ztring().From_Number((int16u)SubFormat.hi, 16), Stream_Audio, StreamPos_Last, InfoCodecID_Format_Riff);
            Fill(Stream_Audio, StreamPos_Last, Audio_CodecID, Ztring().From_GUID(SubFormat), true);
            Fill(Stream_Audio, StreamPos_Last, Audio_Codec, MediaInfoLib::Config.Codec_Get(Ztring().From_Number((int16u)SubFormat.hi, 16)), true);

            //Creating the parser
            #if defined(MEDIAINFO_PCM_YES)
            if (MediaInfoLib::Config.CodecID_Get(Stream_Audio, InfoCodecID_Format_Riff, Ztring().From_Number((int16u)SubFormat.hi, 16))==__T("PCM"))
            {
                //Creating the parser
                File_Pcm MI;
                MI.Frame_Count_Valid=0;
                MI.Codec=Ztring().From_Number((int16u)SubFormat.hi, 16);

                //Parsing
                Open_Buffer_Init(&MI);
                Open_Buffer_Continue(&MI, 0);

                //Filling
                Finish(&MI);
                Merge(MI, StreamKind_Last, 0, StreamPos_Last);
            }
            #endif
        }
        else
        {
            CodecID_Fill(Ztring().From_GUID(SubFormat), Stream_Audio, StreamPos_Last, InfoCodecID_Format_Riff);
        }
        Fill(Stream_Audio, StreamPos_Last, Audio_ChannelPositions, ExtensibleWave_ChannelMask(ChannelMask));
        Fill(Stream_Audio, StreamPos_Last, Audio_ChannelPositions_String2, ExtensibleWave_ChannelMask2(ChannelMask));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_CodecPrivate_vids()
{
    Element_Info1("Copy of vids");

    //Parsing
    int32u Width, Height, Compression;
    int16u Resolution;
    Skip_L4(                                                    "Size");
    Get_L4 (Width,                                              "Width");
    Get_L4 (Height,                                             "Height");
    Skip_L2(                                                    "Planes");
    Get_L2 (Resolution,                                         "BitCount");
    Get_C4 (Compression,                                        "Compression");
    Skip_L4(                                                    "SizeImage");
    Skip_L4(                                                    "XPelsPerMeter");
    Skip_L4(                                                    "YPelsPerMeter");
    Skip_L4(                                                    "ClrUsed");
    Skip_L4(                                                    "ClrImportant");

    FILLING_BEGIN();
        Ztring Codec;
        if (((Compression&0x000000FF)>=0x00000020 && (Compression&0x000000FF)<=0x0000007E
          && (Compression&0x0000FF00)>=0x00002000 && (Compression&0x0000FF00)<=0x00007E00
          && (Compression&0x00FF0000)>=0x00200000 && (Compression&0x00FF0000)<=0x007E0000
          && (Compression&0xFF000000)>=0x20000000 && (Compression&0xFF000000)<=0x7E000000)
         ||   Compression==0x00000000
           ) //Sometimes this value is wrong, we have to test this
        {
            //Filling
            InfoCodecID_Format_Type=InfoCodecID_Format_Riff;
            CodecID.From_CC4(Compression);
            if (Compression==0x00000000)
            {
                Fill(Stream_Video, StreamPos_Last, Video_Format, "RGB", Unlimited, true, true);
                Fill(Stream_Video, StreamPos_Last, Video_Codec, "RGB", Unlimited, true, true); //Raw RGB, not handled by automatic codec mapping
            }
            else
            {
                CodecID_Fill(CodecID, Stream_Video, StreamPos_Last, InfoCodecID_Format_Riff);
                Fill(Stream_Video, StreamPos_Last, Video_Codec, CodecID, true); //FormatTag, may be replaced by codec parser
                Fill(Stream_Video, StreamPos_Last, Video_Codec_CC, CodecID); //FormatTag
            }
            Fill(Stream_Video, StreamPos_Last, Video_Width, Width, 10, true);
            Fill(Stream_Video, StreamPos_Last, Video_Height, Height, 10, true);
            if (Resolution==32 && Compression==0x74736363) //tscc
                Fill(StreamKind_Last, StreamPos_Last, "BitDepth", 8);
            else if (Compression==0x44495633) //DIV3
                Fill(StreamKind_Last, StreamPos_Last, "BitDepth", 8);
            else if (Compression==0x44585342) //DXSB
                Fill(StreamKind_Last, StreamPos_Last, "BitDepth", Resolution);
            else if (Resolution>16 && MediaInfoLib::Config.CodecID_Get(StreamKind_Last, InfoCodecID_Format_Riff, Ztring().From_CC4(Compression), InfoCodecID_ColorSpace).find(__T("RGBA"))!=std::string::npos) //RGB codecs
                Fill(StreamKind_Last, StreamPos_Last, "BitDepth", Resolution/4);
            else if (Compression==0x00000000 //RGB
                  || MediaInfoLib::Config.CodecID_Get(StreamKind_Last, InfoCodecID_Format_Riff, Ztring().From_CC4(Compression), InfoCodecID_ColorSpace).find(__T("RGB"))!=std::string::npos) //RGB codecs
            {
                if (Resolution==32)
                {
                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), "RGBA", Unlimited, true, true);
                    Fill(StreamKind_Last, StreamPos_Last, "BitDepth", Resolution/4); //With Alpha
                }
                else
                    Fill(StreamKind_Last, StreamPos_Last, "BitDepth", Resolution<=16?8:(Resolution/3)); //indexed or normal
            }
            else if (Compression==0x56503632 //VP62
                  || MediaInfoLib::Config.CodecID_Get(StreamKind_Last, InfoCodecID_Format_Riff, Ztring().From_CC4(Compression), InfoCodecID_Format)==__T("H.263") //H.263
                  || MediaInfoLib::Config.CodecID_Get(StreamKind_Last, InfoCodecID_Format_Riff, Ztring().From_CC4(Compression), InfoCodecID_Format)==__T("VC-1")) //VC-1
                Fill(StreamKind_Last, StreamPos_Last, "BitDepth", Resolution/3);
        }

        //Creating the parser
        CodecID_Manage();

    FILLING_END();

    if (Data_Remain())
    {
        Element_Begin1("Private data");
        Open_Buffer_OutOfBand(Stream[TrackNumber].Parser);
        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_DefaultDuration()
{
    Element_Name("DefaultDuration");

    //Parsing
    int64u UInteger=UInteger_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].TrackDefaultDuration=UInteger;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_FlagDefault()
{
    Element_Name("FlagDefault");

    //Parsing
    int64u UInteger=UInteger_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].Default=UInteger?true:false;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_FlagEnabled()
{
    Element_Name("FlagEnabled");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_FlagForced()
{
    Element_Name("FlagForced");

    //Parsing
    int64u UInteger=UInteger_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].Forced=UInteger?true:false;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_FlagLacing()
{
    Element_Name("FlagLacing");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Language()
{
    Element_Name("Language");

    //Parsing
    Ztring Data;
    Get_Local(Element_Size, Data,                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(StreamKind_Last, StreamPos_Last, "Language", Data, true);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_MaxBlockAdditionID()
{
    Element_Name("MaxBlockAdditionID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_MaxCache()
{
    Element_Name("MaxCache");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_MinCache()
{
    Element_Name("MinCache");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Name()
{
    Element_Name("Name");

    //Parsing
    Ztring Data;
    Get_UTF8(Element_Size, Data,                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(StreamKind_Last, StreamPos_Last, "Title", Data);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackNumber()
{
    Element_Name("TrackNumber");

    //Parsing
    TrackNumber=UInteger_Get();

    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(StreamKind_Last, StreamPos_Last, General_ID, TrackNumber);
        stream& streamItem = Stream[TrackNumber];
        if (StreamKind_Last!=Stream_Max)
        {
            streamItem.StreamKind=StreamKind_Last;
            streamItem.StreamPos=StreamPos_Last;
        }
        if (TrackVideoDisplayWidth && TrackVideoDisplayHeight)
            streamItem.DisplayAspectRatio=((float)TrackVideoDisplayWidth)/(float)TrackVideoDisplayHeight;
        if (AvgBytesPerSec)
            streamItem.AvgBytesPerSec=AvgBytesPerSec;

        CodecID_Manage();
        CodecPrivate_Manage();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackTimecodeScale()
{
    Element_Name("TrackTimecodeScale");

    Float_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackOffset()
{
    Element_Name("TrackOffset");

    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackType()
{
    Element_Name("TrackType");

    //Parsing
    int64u UInteger=UInteger_Get();

    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        TrackType=UInteger;
        switch(UInteger)
        {
            case 0x01 :
                        Stream_Prepare(Stream_Video);
                        break;
            case 0x02 :
                        Stream_Prepare(Stream_Audio);
                        break;
            case 0x11 :
                        Stream_Prepare(Stream_Text );
                        break;
            default   : ;
        }

        if (TrackNumber!=(int64u)-1 && StreamKind_Last!=Stream_Max)
        {
            stream& streamItem = Stream[TrackNumber];
            streamItem.StreamKind=StreamKind_Last;
            streamItem.StreamPos=StreamPos_Last;
        }

        CodecID_Manage();
        CodecPrivate_Manage();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackUID()
{
    Element_Name("TrackUID");

    //Parsing
    int64u UInteger=UInteger_Get();

    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].TrackUID=UInteger;
        Fill(StreamKind_Last, StreamPos_Last, General_UniqueID, UInteger);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video()
{
    Element_Name("Video");

    //Preparing
    if (Segment_Info_Count>1)
        return; //First element has the priority
    TrackVideoDisplayWidth=0;
    TrackVideoDisplayHeight=0;
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_AspectRatioType()
{
    Element_Name("AspectRatioType");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_ColourSpace()
{
    Element_Name("ColourSpace");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_DisplayHeight()
{
    Element_Name("DisplayHeight");

    //Parsing
    int64u UInteger=UInteger_Get();

    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        TrackVideoDisplayHeight=UInteger;
        if (TrackNumber!=(int64u)-1 && TrackVideoDisplayWidth && TrackVideoDisplayHeight)
            Stream[TrackNumber].DisplayAspectRatio=((float)TrackVideoDisplayWidth)/(float)TrackVideoDisplayHeight;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_DisplayUnit()
{
    Element_Name("DisplayUnit");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_DisplayWidth()
{
    Element_Name("DisplayWidth");

    //Parsing
    int64u UInteger=UInteger_Get();

    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        TrackVideoDisplayWidth=UInteger;
        if (TrackNumber!=(int64u)-1 && TrackVideoDisplayWidth && TrackVideoDisplayHeight)
            Stream[TrackNumber].DisplayAspectRatio=((float)TrackVideoDisplayWidth)/(float)TrackVideoDisplayHeight;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_FlagInterlaced()
{
    Element_Name("FlagInterlaced");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_FieldOrder()
{
    Element_Name("FieldOrder");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_FrameRate()
{
    Element_Name("FrameRate");

    //Parsing
   float64 Value=Float_Get();

    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].FrameRate=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_Colour()
{
    Element_Name("Colour");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_Colour_MatrixCoefficients()
{
    Element_Name("MatrixCoefficients");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_Colour_BitsPerChannel()
{
    Element_Name("BitsPerChannel");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_Colour_Range()
{
    Element_Name("Range");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_Colour_TransferCharacteristics()
{
    Element_Name("TransferCharacteristics");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_Colour_Primaries()
{
    Element_Name("Primaries");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_PixelCropBottom()
{
    Element_Name("PixelCropBottom");

    //Parsing
    int64u UInteger=UInteger_Get();
    
    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].PixelCropBottom=UInteger;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_PixelCropLeft()
{
    Element_Name("PixelCropLeft");

    //Parsing
    int64u UInteger=UInteger_Get();
    
    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].PixelCropLeft=UInteger;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_PixelCropRight()
{
    Element_Name("PixelCropRight");

    //Parsing
    int64u UInteger=UInteger_Get();
    
    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].PixelCropRight=UInteger;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_PixelCropTop()
{
    Element_Name("PixelCropTop");

    //Parsing
    int64u UInteger=UInteger_Get();
    
    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Stream[TrackNumber].PixelCropTop=UInteger;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_PixelHeight()
{
    Element_Name("PixelHeight");

    //Parsing
    int64u UInteger=UInteger_Get();

    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_Video, StreamPos_Last, Video_Height, UInteger, 10, true);
        if (!TrackVideoDisplayHeight)
            TrackVideoDisplayHeight=UInteger; //Default value of DisplayHeight is PixelHeight

        //In case CodecID was defined before this item, some decoders are not initialized with the correct values, filling it now
        #if defined(MEDIAINFO_FFV1_YES)
            const Ztring &Format=Retrieve(Stream_Video, StreamPos_Last, Video_Format);
            stream& streamItem = Stream[TrackNumber];
            if (0);
        #endif
        #if defined(MEDIAINFO_FFV1_YES)
        else if (Format==__T("FFV1"))
        {
            File_Ffv1* parser = (File_Ffv1*)streamItem.Parser;
            parser->Height=UInteger;
        }
        #endif
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_PixelWidth()
{
    Element_Name("PixelWidth");

    //Parsing
    int64u UInteger=UInteger_Get();

    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_Video, StreamPos_Last, Video_Width, UInteger, 10, true);
        if (!TrackVideoDisplayWidth)
            TrackVideoDisplayWidth=UInteger; //Default value of DisplayWidth is PixelWidth

        //In case CodecID was defined before this item, some decoders are not initialized with the correct values, filling it now
        #if defined(MEDIAINFO_FFV1_YES)
            const Ztring &Format=Retrieve(Stream_Video, StreamPos_Last, Video_Format);
            stream& streamItem = Stream[TrackNumber];
            if (0);
        #endif
        #if defined(MEDIAINFO_FFV1_YES)
        else if (Format==__T("FFV1"))
        {
            File_Ffv1* parser = (File_Ffv1*)streamItem.Parser;
            parser->Width=UInteger;
        }
        #endif
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_StereoMode()
{
    Element_Name("StereoMode");

    //Parsing
    int64u UInteger=UInteger_Get(); Element_Info1(Mk_StereoMode(UInteger));

    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_Video, StreamPos_Last, Video_MultiView_Count, 2); //Matroska seems to be limited to 2 views
        Fill(Stream_Video, StreamPos_Last, Video_MultiView_Layout, Mk_StereoMode(UInteger));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_Video_OldStereoMode()
{
    Element_Name("StereoMode");

    //Parsing
    int64u UInteger=UInteger_Get(); Element_Info1(Mk_StereoMode(UInteger));

    //Filling
    FILLING_BEGIN();
        if (Segment_Info_Count>1)
            return; //First element has the priority
        Fill(Stream_Video, StreamPos_Last, Video_MultiView_Count, 2); //Matroska seems to be limited to 2 views
        Fill(Stream_Video, StreamPos_Last, Video_MultiView_Layout, Mk_StereoMode(UInteger));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackOverlay()
{
    Element_Name("TrackOverlay");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackTranslate()
{
    Element_Name("TrackTranslate");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackTranslate_Codec()
{
    Element_Name("Codec");
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackTranslate_EditionUID()
{
    Element_Name("EditionUID");

    //Parsing
    UInteger_Info();
}

//---------------------------------------------------------------------------
void File_Mk::Segment_Tracks_TrackEntry_TrackTranslate_TrackID()
{
    Element_Name("TrackID");

    //Parsing
    UInteger_Info();
}

//***************************************************************************
// Data
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mk::UInteger_Info()
{
    switch (Element_Size)
    {
        case 1 :
                {
                    Info_B1(Data,                               "Data"); Element_Info1(Data);
                    return;
                }
        case 2 :
                {
                    Info_B2(Data,                               "Data"); Element_Info1(Data);
                    return;
                }
        case 3 :
                {
                    Info_B3(Data,                               "Data"); Element_Info1(Data);
                    return;
                }
        case 4 :
                {
                    Info_B4(Data,                               "Data"); Element_Info1(Data);
                    return;
                }
        case 5 :
                {
                    Info_B5(Data,                               "Data"); Element_Info1(Data);
                    return;
                }
        case 6 :
                {
                    Info_B6(Data,                               "Data"); Element_Info1(Data);
                    return;
                }
        case 7 :
                {
                    Info_B7(Data,                               "Data"); Element_Info1(Data);
                    return;
                }
        case 8 :
                {
                    Info_B8(Data,                               "Data"); Element_Info1(Data);
                    return;
                }
        case 16:
                {
                    Info_B16(Data,                              "Data"); Element_Info1(Data);
                    return;
                }
        default : Skip_XX(Element_Size,                         "Data");
    }
}

//---------------------------------------------------------------------------
int64u File_Mk::UInteger_Get()
{
    switch (Element_Size)
    {
        case 1 :
                {
                    int8u Data;
                    Get_B1 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 2 :
                {
                    int16u Data;
                    Get_B2 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 3 :
                {
                    int32u Data;
                    Get_B3 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 4 :
                {
                    int32u Data;
                    Get_B4 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 5 :
                {
                    int64u Data;
                    Get_B5 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 6 :
                {
                    int64u Data;
                    Get_B6 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 7 :
                {
                    int64u Data;
                    Get_B7 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 8 :
                {
                    int64u Data;
                    Get_B8 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        default :   Skip_XX(Element_Size,                       "Data");
                    return 0;
    }
}

//---------------------------------------------------------------------------
int128u File_Mk::UInteger16_Get()
{
    switch (Element_Size)
    {
        case 1 :
                {
                    int8u Data;
                    Get_B1 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 2 :
                {
                    int16u Data;
                    Get_B2 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 3 :
                {
                    int32u Data;
                    Get_B3 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 4 :
                {
                    int32u Data;
                    Get_B4 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 5 :
                {
                    int64u Data;
                    Get_B5 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 6 :
                {
                    int64u Data;
                    Get_B6 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 7 :
                {
                    int64u Data;
                    Get_B7 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 8 :
                {
                    int64u Data;
                    Get_B8 (Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 16:
                {
                    int128u Data;
                    Get_B16(Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        default :   Skip_XX(Element_Size,                       "Data");
                    return 0;
    }
}

//---------------------------------------------------------------------------
float64 File_Mk::Float_Get()
{
    switch (Element_Size)
    {
        case 4 :
                {
                    float32 Data;
                    Get_BF4(Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        case 8 :
                {
                    float64 Data;
                    Get_BF8(Data,                               "Data"); Element_Info1(Data);
                    return Data;
                }
        default :   Skip_XX(Element_Size,                       "Data");
                    return 0.0;
    }
}

//---------------------------------------------------------------------------
void File_Mk::Float_Info()
{
    switch (Element_Size)
    {
        case 4 :
                {
                    Info_BF4(Data,                              "Data"); Element_Info1(Data);
                    return;
                }
        case 8 :
                {
                    Info_BF8(Data,                              "Data"); Element_Info1(Data);
                    return;
                }
        default :   Skip_XX(Element_Size,                       "Data");
                    return;
    }
}

//---------------------------------------------------------------------------
Ztring File_Mk::UTF8_Get()
{
    Ztring Data;
    Get_UTF8(Element_Size, Data,                                "Data"); Element_Info1(Data);
    return Data;
}

//---------------------------------------------------------------------------
void File_Mk::UTF8_Info()
{
    Info_UTF8(Element_Size, Data,                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
Ztring File_Mk::Local_Get()
{
    Ztring Data;
    Get_Local(Element_Size, Data,                               "Data"); Element_Info1(Data);
    return Data;
}

//---------------------------------------------------------------------------
void File_Mk::Local_Info()
{
    Info_Local(Element_Size, Data,                              "Data"); Element_Info1(Data);
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mk::CodecID_Manage()
{
    if (TrackType==(int64u)-1 || TrackNumber==(int64u)-1 || CodecID.empty() || Stream[TrackNumber].Parser)
        return; //Not ready (or not needed)

    if (Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_CodecID)).empty())
    {
        CodecID_Fill(CodecID, StreamKind_Last, StreamPos_Last, InfoCodecID_Format_Matroska);
        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec), CodecID);
    }
    stream& streamItem = Stream[TrackNumber];

    //Creating the parser
    #if defined(MEDIAINFO_MPEG4V_YES) || defined(MEDIAINFO_AVC_YES) || defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_VC1_YES) || defined(MEDIAINFO_DIRAC_YES) || defined(MEDIAINFO_MPEGV_YES) || defined(MEDIAINFO_VP8_YES) || defined(MEDIAINFO_OGG_YES) || defined(MEDIAINFO_DTS_YES)
        const Ztring &Format=MediaInfoLib::Config.CodecID_Get(StreamKind_Last, InfoCodecID_Format_Type, CodecID, InfoCodecID_Format);
    #endif
        if (0);
    #if defined(MEDIAINFO_MPEG4V_YES)
    else if (Format==__T("MPEG-4 Visual"))
    {
        streamItem.Parser=new File_Mpeg4v;
        ((File_Mpeg4v*)streamItem.Parser)->FrameIsAlwaysComplete=true;
    }
    #endif
    #if defined(MEDIAINFO_AVC_YES)
    else if (Format==__T("AVC"))
    {
        File_Avc* parser = new File_Avc;
        streamItem.Parser= parser;
        ((File_Avc*)streamItem.Parser)->FrameIsAlwaysComplete=true;
        if (InfoCodecID_Format_Type==InfoCodecID_Format_Matroska)
        {
            parser->MustSynchronize=false;
            parser->MustParse_SPS_PPS=true;
            parser->SizedBlocks=true;
            #if MEDIAINFO_DEMUX
                if (Config->Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Get())
                {
                    streamItem.Parser->Demux_Level=2; //Container
                    streamItem.Parser->Demux_UnpacketizeContainer=true;
                }
            #endif //MEDIAINFO_DEMUX
        }
    }
    #endif
    #if defined(MEDIAINFO_HEVC_YES)
    else if (Format==__T("HEVC"))
    {
        File_Hevc* parser = new File_Hevc;
        streamItem.Parser = parser;
        parser->FrameIsAlwaysComplete=true;
        if (InfoCodecID_Format_Type==InfoCodecID_Format_Matroska)
        {
            parser->MustSynchronize=false;
            parser->MustParse_VPS_SPS_PPS=true;
            parser->MustParse_VPS_SPS_PPS_FromMatroska=true;
            parser->SizedBlocks=true;
            #if MEDIAINFO_DEMUX
                if (Config->Demux_Hevc_Transcode_Iso14496_15_to_AnnexB_Get())
                {
                    streamItem.Parser->Demux_Level=2; //Container
                    streamItem.Parser->Demux_UnpacketizeContainer=true;
                }
            #endif //MEDIAINFO_DEMUX
        }
    }
    #endif
    #if defined(MEDIAINFO_FFV1_YES)
    else if (Format==__T("FFV1"))
    {
        File_Ffv1* parser = new File_Ffv1;
        streamItem.Parser = parser;
        parser->Width=Retrieve(Stream_Video, StreamPos_Last, Video_Width).To_int32u();
        parser->Height=Retrieve(Stream_Video, StreamPos_Last, Video_Height).To_int32u();
    }
    #endif
    #if defined(MEDIAINFO_HUFFYUV_YES)
    else if (Format==__T("HuffYUV"))
    {
        streamItem.Parser=new File_HuffYuv;
    }
    #endif
    #if defined(MEDIAINFO_VC1_YES)
    else if (Format==__T("VC-1"))
    {
        File_Vc1* parser = new File_Vc1;
        streamItem.Parser= parser;
        parser->FrameIsAlwaysComplete=true;
    }
    #endif
    #if defined(MEDIAINFO_DIRAC_YES)
    else if (Format==__T("Dirac"))
    {
        streamItem.Parser=new File_Dirac;
    }
    #endif
    #if defined(MEDIAINFO_MPEGV_YES)
    else if (Format==__T("MPEG Video"))
    {
        File_Mpegv* parser = new File_Mpegv;
        streamItem.Parser = parser;
        parser->FrameIsAlwaysComplete=true;
    }
    #endif
    #if defined(MEDIAINFO_PRORES_YES)
    else if (Format==__T("ProRes"))
    {
        streamItem.Parser=new File_ProRes;
    }
    #endif
    #if defined(MEDIAINFO_VP8_YES)
    else if (Format==__T("VP8"))
    {
        streamItem.Parser=new File_Vp8;
    }
    #endif
    #if defined(MEDIAINFO_OGG_YES)
    else if (Format==__T("Theora")  || Format==__T("Vorbis"))
    {
        File_Ogg* parser = new File_Ogg;
        streamItem.Parser = parser;
        streamItem.Parser->MustSynchronize=false;
        parser->XiphLacing=true;
    }
    #endif
    #if defined(MEDIAINFO_RM_YES)
    else if (CodecID.find(__T("V_REAL/"))==0)
    {
        File_Rm* parser = new File_Rm;
        streamItem.Parser = parser;
        parser->FromMKV_StreamType=Stream_Video;
    }
    #endif
    #if defined(MEDIAINFO_AC3_YES)
    else if (Format==__T("AC-3") || Format==__T("E-AC-3") || Format==__T("TrueHD"))
    {
        streamItem.Parser=new File_Ac3;
    }
    #endif
    #if defined(MEDIAINFO_DTS_YES)
    else if (Format==__T("DTS"))
    {
        streamItem.Parser=new File_Dts;
    }
    #endif
    #if defined(MEDIAINFO_AAC_YES)
    else if (CodecID==(__T("A_AAC")))
    {
        File_Aac* parser = new File_Aac;
        streamItem.Parser = parser;
        parser->Mode=File_Aac::Mode_AudioSpecificConfig;
    }
    #endif
    #if defined(MEDIAINFO_AAC_YES)
    else if (CodecID.find(__T("A_AAC/"))==0)
    {
        Ztring Profile;
        int8u audioObjectType=0, Version=0, SBR=2, PS=2;
             if (CodecID==__T("A_AAC/MPEG2/MAIN"))     {Version=2; Profile=__T("Main");                   audioObjectType=1;}
        else if (CodecID==__T("A_AAC/MPEG2/LC"))       {Version=2; Profile=__T("LC");                     audioObjectType=2; SBR=0;}
        else if (CodecID==__T("A_AAC/MPEG2/LC/SBR"))   {Version=2; Profile=__T("HE-AAC / LC");            audioObjectType=2; SBR=1;}
        else if (CodecID==__T("A_AAC/MPEG2/SSR"))      {Version=2; Profile=__T("SSR");                    audioObjectType=3;}
        else if (CodecID==__T("A_AAC/MPEG4/MAIN"))     {Version=4; Profile=__T("Main");                   audioObjectType=1;}
        else if (CodecID==__T("A_AAC/MPEG4/LC"))       {Version=4; Profile=__T("LC");                     audioObjectType=2; SBR=0;}
        else if (CodecID==__T("A_AAC/MPEG4/LC/SBR"))   {Version=4; Profile=__T("HE-AAC / LC");            audioObjectType=2; SBR=1; PS=0;}
        else if (CodecID==__T("A_AAC/MPEG4/LC/SBR/PS")){Version=4; Profile=__T("HE-AACv2 / HE-AAC / LC"); audioObjectType=2; SBR=1; PS=1;}
        else if (CodecID==__T("A_AAC/MPEG4/SSR"))      {Version=4; Profile=__T("SSR");                    audioObjectType=3;}
        else if (CodecID==__T("A_AAC/MPEG4/LTP"))      {Version=4; Profile=__T("LTP");                    audioObjectType=4;}
        else if (CodecID==__T("raac"))                 {           Profile=__T("LC");                     audioObjectType=2;}
        else if (CodecID==__T("racp"))                 {           Profile=__T("HE-AAC / LC");            audioObjectType=2; SBR=1; PS=0;}

        if (Version>0)
            Fill(Stream_Audio, StreamPos_Last, Audio_Format_Version, Version==2?"Version 2":"Version 4");
        Fill(Stream_Audio, StreamPos_Last, Audio_Format_Profile, Profile);
        if (SBR!=2)
            Fill(Stream_Audio, StreamPos_Last, Audio_Format_Settings_SBR, SBR?"Yes":"No");
        if (PS!=2)
            Fill(Stream_Audio, StreamPos_Last, Audio_Format_Settings_PS, PS?"Yes":"No");
        int64s sampling_frequency=Retrieve(Stream_Audio, StreamPos_Last, Audio_SamplingRate).To_int64s();

        File_Aac* parser = new File_Aac;
        streamItem.Parser = parser;
        parser->Mode=File_Aac::Mode_AudioSpecificConfig;
        parser->AudioSpecificConfig_OutOfBand(sampling_frequency, audioObjectType, SBR==1?true:false, PS==1?true:false, SBR==1?true:false, PS==1?true:false);
    }
    #endif
    #if defined(MEDIAINFO_AAC_YES)
    else if (Format==(__T("AAC")))
    {
        File_Aac* parser = new File_Aac;
        streamItem.Parser = parser;
        parser->Mode=File_Aac::Mode_ADTS;
    }
    #endif
    #if defined(MEDIAINFO_MPEGA_YES)
    else if (Format==__T("MPEG Audio"))
    {
        streamItem.Parser=new File_Mpega;
    }
    #endif
    #if defined(MEDIAINFO_FLAC_YES)
    else if (Format==__T("Flac"))
    {
        streamItem.Parser=new File_Flac;
    }
    #endif
    #if defined(MEDIAINFO_OPUS_YES)
    else if (CodecID.find(__T("A_OPUS"))==0) //http://wiki.xiph.org/MatroskaOpus
    {
        streamItem.Parser=new File_Opus;
    }
    #endif
    #if defined(MEDIAINFO_WVPK_YES)
    else if (Format==__T("WavPack"))
    {
        File_Wvpk* parser = new File_Wvpk;
        streamItem.Parser = parser;
        parser->FromMKV=true;
    }
    #endif
    #if defined(MEDIAINFO_TTA_YES)
    else if (Format==__T("TTA"))
    {
        //streamItem.Parser=new File_Tta; //Parser is not needed, because header is useless and dropped (the parser analyses only the header)
    }
    #endif
    #if defined(MEDIAINFO_PCM_YES)
    else if (Format==__T("PCM"))
    {
        File_Pcm* parser = new File_Pcm;
        streamItem.Parser = parser;
        parser->Codec=CodecID;
    }
    #endif
    #if defined(MEDIAINFO_RM_YES)
    else if (CodecID.find(__T("A_REAL/"))==0)
    {
        File_Rm* parser = new File_Rm;
        streamItem.Parser = parser;
        parser->FromMKV_StreamType=Stream_Audio;
    }
    #endif
    Element_Code=TrackNumber;
    Open_Buffer_Init(streamItem.Parser);

    CodecID.clear();
}

//---------------------------------------------------------------------------
void File_Mk::CodecPrivate_Manage()
{
    if (CodecPrivate==NULL || TrackNumber==(int64u)-1 || TrackType==(int64u)-1 || Retrieve(Stream[TrackNumber].StreamKind, Stream[TrackNumber].StreamPos, "CodecID").empty())
        return; //Not ready (or not needed)

    //Codec Private is already here, so we can parse it now
    const int8u* Buffer_Save=Buffer;
    size_t Buffer_Offset_Save=Buffer_Offset;
    size_t Buffer_Size_Save=Buffer_Size;
    int64u Element_Size_Save=Element_Size;
    Buffer=CodecPrivate;
    Buffer_Offset=0;
    Buffer_Size=CodecPrivate_Size;
    Element_Offset=0;
    Element_Size=Buffer_Size;
    Segment_Tracks_TrackEntry_CodecPrivate__Parse();
    Buffer=Buffer_Save;
    Buffer_Offset=Buffer_Offset_Save;
    Buffer_Size=Buffer_Size_Save;
    Element_Size=Element_Size_Save;
    Element_Offset=Element_Size_Save;
    delete[] CodecPrivate; CodecPrivate=NULL;
    CodecPrivate_Size=0;
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mk::JumpTo (int64u GoToValue)
{
    //Clearing CRC data
    for (size_t i = 0; i<CRC32Compute.size(); i++)
        if (CRC32Compute[i].UpTo)
        {
            //Searching and replacing CRC-32 information
            //TODO: better implementation without this ugly hack
            #if MEDIAINFO_TRACE
            element_details::Element_Node *node = Get_Trace_Node(i);
            if (node)
            {
                std::string ToSearchInInfo=std::string("Not tested ")+Ztring::ToZtring(i).To_UTF8()+' '+Ztring::ToZtring(CRC32Compute[i].Expected).To_UTF8();
                CRC32_Check_In_Node(ToSearchInInfo, "Not tested", node);
            }
            #endif //MEDIAINFO_TRACE

            CRC32Compute[i].UpTo=0;
        }
    CRC32Compute.clear();

    //GoTo
    GoTo(GoToValue);
}

//---------------------------------------------------------------------------
//We want to parse more than the 1st element only if one of the following:
//-trace is activated
//-request for parsing all and CRC-32 is present
//TODO: if trace is not activated and CRC-32 is present, we don't need to parse all elements, we only need to do the CRC-32 check
void File_Mk::TestMultipleInstances (size_t* Instances)
{
    #if MEDIAINFO_TRACE
    bool ParseAll=false;
    if (Trace_Activated)
        ParseAll=true;
    #else //MEDIAINFO_TRACE
    static bool ParseAll = false;
    #endif //MEDIAINFO_TRACE
    if (!ParseAll && Config->ParseSpeed >= 1.0)
    {
        //Probing, checking if CRC-32 is present
        if (Element_Size < 1)
        {
            Element_WaitForMoreData();
            return;
        }
        #if MEDIAINFO_TRACE
        if (Buffer[Buffer_Offset] == 0xBF) //CRC-32 element
            ParseAll=true;
        #endif //MEDIAINFO_TRACE
    }

    if ((!Instances || *Instances) && !ParseAll)
        Skip_XX(Element_TotalSize_Get(),                    "No need, skipping");

    if (Instances)
        (*Instances)++;
}

//---------------------------------------------------------------------------
#if MEDIAINFO_TRACE
bool File_Mk::CRC32_Check_In_Node(const std::string& ToSearchInInfo, const std::string& info, element_details::Element_Node *node)
{
    //Check in the current node
    for (size_t i = 0; i < node->Infos.size(); ++i)
    {
        if (node->Infos[i]->data == ToSearchInInfo)
        {
            node->Infos[i]->data = info;
            return true;
        }
    }

    //Check in the children of the current node
    for (size_t i = 0; i < node->Children.size(); ++i)
    {
        if (CRC32_Check_In_Node(ToSearchInInfo, info, node->Children[i]))
            return true;
    }

    return false;
}
#endif // MEDIAINFO_TRACE

//---------------------------------------------------------------------------
void File_Mk::CRC32_Check ()
{
    for (size_t i = 0; i<CRC32Compute.size(); i++)
        if (CRC32Compute[i].UpTo && File_Offset + Buffer_Offset - (size_t)Header_Size >= CRC32Compute[i].From)
        {
            //Handling of case when data is not completetely loaded because it is not needed by the parser
            const size_t Offset = Buffer_Offset + (size_t)((Element_WantNextLevel && Element_Offset <= Element_Size) ? Element_Offset : Element_Size);
            if (Element_Offset > Element_Size)
            {
                CRC32Compute_SkipUpTo = File_Offset + Element_Offset;
                Element_Offset = Element_Size;
            }
            
            Matroska_CRC32_Compute(CRC32Compute[i].Computed, Buffer + Buffer_Offset - (size_t)Header_Size, Buffer + Offset);
            if (File_Offset + Offset >= CRC32Compute[i].UpTo)
            {
                CRC32Compute[i].Computed ^= 0xFFFFFFFF;

                #if MEDIAINFO_TRACE
                    if (Trace_Activated)
                    {
                        //Searching and replacing CRC-32 information
                        //TODO: better implementation without this ugly hack

                        element_details::Element_Node *node = Get_Trace_Node(i);
                        if (node)
                        {
                            std::string ToSearchInInfo=std::string("Not tested ")+Ztring::ToZtring(i).To_UTF8()+' '+Ztring::ToZtring(CRC32Compute[i].Expected).To_UTF8();
                            CRC32_Check_In_Node(ToSearchInInfo, CRC32Compute[i].Computed == CRC32Compute[i].Expected?"OK":"NOK", node);

                            #if MEDIAINFO_FIXITY
                                if (Config->TryToFix_Get() && CRC32Compute[i].Computed!=CRC32Compute[i].Expected)
                                {
                                    size_t NewBuffer_Size=(size_t)(CRC32Compute[i].UpTo-CRC32Compute[i].From);
                                    int8u* NewBuffer=new int8u[NewBuffer_Size];
                                    File F;
                                    if (F.Open(File_Name))
                                    {
                                        F.GoTo(CRC32Compute[i].From);
                                        F.Read(NewBuffer, NewBuffer_Size);
                                        int8u Modified=0;
                                        size_t BitPosition=Matroska_TryToFixCRC(NewBuffer, NewBuffer_Size, CRC32Compute[i].Expected, Modified);
                                        if (BitPosition!=(size_t)-1)
                                        {
                                            size_t BytePosition=BitPosition>>3;
                                            size_t BitInBytePosition=BitPosition&0x7;
                                            Modified^=1<<BitInBytePosition;
                                            FixFile(CRC32Compute[i].From+BytePosition, &Modified, 1)?Param_Info1("Fixed"):Param_Info1("Not fixed");
                                        }
                                    }
                                    delete[] NewBuffer; //NewBuffer=NULL;
                                }
                            #endif //MEDIAINFO_FIXITY

                        }

                        //Debug
                        //size_t Element_Level_Save=Element_Level;
                        //Element_Level=i;
                        //Element_Info(CRC32Compute[i].Computed == CRC32Compute[i].Expected ? __T("CRC32 check OK"):__T("CRC32 check NOK"));
                        //Element_Level=Element_Level_Save;
                    }
                #endif //MEDIAINFO_TRACE

                if (CRC32Compute[i].Computed != CRC32Compute[i].Expected)
                {
                    Fill(Stream_General, 0, "CRC_Error_Pos", CRC32Compute[i].Pos);
                }

                CRC32Compute[i].UpTo=0;
            }
        }
}

} //NameSpace

#endif //MEDIAINFO_MK_YES



