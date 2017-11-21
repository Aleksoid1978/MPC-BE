/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Matroska files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_MatroskaH
#define MediaInfo_File_MatroskaH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Classe File_Matroska
//***************************************************************************

class File_Mk : public File__Analyze
{
protected :
    //Streams management
    void Streams_Finish();

public :
    File_Mk();
    ~File_Mk();

private :
    //Buffer - Synchro
    bool Synchronize();

    //Buffer - Global
    void Read_Buffer_Continue();

    //Buffer - Global
    void Read_Buffer_Unsynched();
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID);
    #endif //MEDIAINFO_SEEK

    //Buffer
    bool Header_Begin();
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Zero();
    void CRC32();
    void Void();
    void Ebml(){};
    void Ebml_Version(){UInteger_Info();};
    void Ebml_ReadVersion();
    void Ebml_MaxIDLength(){UInteger_Info();};
    void Ebml_MaxSizeLength();
    void Ebml_DocType();
    void Ebml_DocTypeVersion();
    void Ebml_DocTypeReadVersion();
    void Segment();
    void Segment_SeekHead();
    void Segment_SeekHead_Seek();
    void Segment_SeekHead_Seek_SeekID();
    void Segment_SeekHead_Seek_SeekPosition();
    void Segment_Info();
    void Segment_Info_SegmentUID();
    void Segment_Info_SegmentFilename(){UTF8_Info();};
    void Segment_Info_PrevUID(){UInteger_Info();};
    void Segment_Info_PrevFilename(){UTF8_Info();};
    void Segment_Info_NextUID(){UInteger_Info();};
    void Segment_Info_NextFilename(){UTF8_Info();};
    void Segment_Info_SegmentFamily(){Skip_XX(Element_Size, "Data");};
    void Segment_Info_ChapterTranslate(){};
    void Segment_Info_ChapterTranslate_ChapterTranslateEditionUID(){UInteger_Info();};
    void Segment_Info_ChapterTranslate_ChapterTranslateCodec(){UInteger_Info();};
    void Segment_Info_ChapterTranslate_ChapterTranslateID(){Skip_XX(Element_Size, "Data");};
    void Segment_Info_TimecodeScale();
    void Segment_Info_Duration();
    void Segment_Info_DateUTC();
    void Segment_Info_Title();
    void Segment_Info_MuxingApp();
    void Segment_Info_WritingApp();
    void Segment_Cluster();
    void Segment_Cluster_Timecode();
    void Segment_Cluster_SilentTracks(){};
    void Segment_Cluster_SilentTracks_SilentTrackNumber(){UInteger_Info();};
    void Segment_Cluster_Position(){UInteger_Info();};
    void Segment_Cluster_PrevSize(){UInteger_Info();};
    void Segment_Cluster_SimpleBlock();
    void Segment_Cluster_BlockGroup();
    void Segment_Cluster_BlockGroup_Block();
    void Segment_Cluster_BlockGroup_Block_Lace();
    void Segment_Cluster_BlockGroup_BlockVirtual(){Skip_XX(Element_Size, "Data");};
    void Segment_Cluster_BlockGroup_BlockAdditions(){};
    void Segment_Cluster_BlockGroup_BlockAdditions_BlockMore(){};
    void Segment_Cluster_BlockGroup_BlockAdditions_BlockMore_BlockAddID(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_BlockAdditions_BlockMore_BlockAdditional(){Skip_XX(Element_Size, "Data");};
    void Segment_Cluster_BlockGroup_BlockDuration();
    void Segment_Cluster_BlockGroup_ReferencePriority(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_ReferenceBlock(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_ReferenceVirtual(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_CodecState(){Skip_XX(Element_Size, "Data");};
    void Segment_Cluster_BlockGroup_DiscardPadding(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_Slices(){};
    void Segment_Cluster_BlockGroup_Slices_TimeSlice(){};
    void Segment_Cluster_BlockGroup_Slices_TimeSlice_LaceNumber(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_Slices_TimeSlice_FrameNumber(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_Slices_TimeSlice_BlockAdditionID(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_Slices_TimeSlice_Delay(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_Slices_TimeSlice_SliceDuration(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_ReferenceFrame(){};
    void Segment_Cluster_BlockGroup_ReferenceFrame_ReferenceOffset(){UInteger_Info();};
    void Segment_Cluster_BlockGroup_ReferenceFrame_ReferenceTimeCode(){UInteger_Info();};
    void Segment_Cluster_EncryptedBlock(){Skip_XX(Element_Size, "Data");};
    void Segment_Tracks();
    void Segment_Tracks_TrackEntry();
    void Segment_Tracks_TrackEntry_TrackNumber();
    void Segment_Tracks_TrackEntry_TrackUID();
    void Segment_Tracks_TrackEntry_TrackType();
    void Segment_Tracks_TrackEntry_FlagEnabled(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_FlagDefault();
    void Segment_Tracks_TrackEntry_FlagForced();
    void Segment_Tracks_TrackEntry_FlagLacing(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_MinCache(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_MaxCache(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_DefaultDuration();
    void Segment_Tracks_TrackEntry_DefaultDecodedFieldDuration(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrackTimecodeScale(){Float_Info();};
    void Segment_Tracks_TrackEntry_TrackOffset(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_MaxBlockAdditionID(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Name();
    void Segment_Tracks_TrackEntry_Language();
    void Segment_Tracks_TrackEntry_LanguageIETF(){String_Info();};
    void Segment_Tracks_TrackEntry_CodecID();
    void Segment_Tracks_TrackEntry_CodecPrivate();
    void Segment_Tracks_TrackEntry_CodecName(){UTF8_Info();};
    void Segment_Tracks_TrackEntry_AttachmentLink(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_CodecSettings(){UTF8_Info();};
    void Segment_Tracks_TrackEntry_CodecInfoURL(){String_Info();};
    void Segment_Tracks_TrackEntry_CodecDownloadURL(){String_Info();};
    void Segment_Tracks_TrackEntry_CodecDecodeAll(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrackOverlay(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_CodecDelay(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_SeekPreRoll(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrackTranslate(){};
    void Segment_Tracks_TrackEntry_TrackTranslate_TrackTranslateEditionUID(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrackTranslate_TrackTranslateCodec(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrackTranslate_TrackTranslateTrackID(){Skip_XX(Element_Size, "Data");};
    void Segment_Tracks_TrackEntry_Video();
    void Segment_Tracks_TrackEntry_Video_FlagInterlaced(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_FieldOrder(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_StereoMode();
    void Segment_Tracks_TrackEntry_Video_AlphaMode(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_OldStereoMode();
    void Segment_Tracks_TrackEntry_Video_PixelWidth();
    void Segment_Tracks_TrackEntry_Video_PixelHeight();
    void Segment_Tracks_TrackEntry_Video_PixelCropBottom();
    void Segment_Tracks_TrackEntry_Video_PixelCropTop();
    void Segment_Tracks_TrackEntry_Video_PixelCropLeft();
    void Segment_Tracks_TrackEntry_Video_PixelCropRight();
    void Segment_Tracks_TrackEntry_Video_DisplayWidth();
    void Segment_Tracks_TrackEntry_Video_DisplayHeight();
    void Segment_Tracks_TrackEntry_Video_DisplayUnit(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_AspectRatioType(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_ColourSpace(){Skip_XX(Element_Size, "Data");};
    void Segment_Tracks_TrackEntry_Video_GammaValue(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_FrameRate();
    void Segment_Tracks_TrackEntry_Video_Colour(){};
    void Segment_Tracks_TrackEntry_Video_Colour_MatrixCoefficients();
    void Segment_Tracks_TrackEntry_Video_Colour_BitsPerChannel();
    void Segment_Tracks_TrackEntry_Video_Colour_ChromaSubsamplingHorz(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_ChromaSubsamplingVert(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_CbSubsamplingHorz(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_CbSubsamplingVert(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_ChromaSitingHorz(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_ChromaSitingVert(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_Range();
    void Segment_Tracks_TrackEntry_Video_Colour_TransferCharacteristics();
    void Segment_Tracks_TrackEntry_Video_Colour_Primaries();
    void Segment_Tracks_TrackEntry_Video_Colour_MaxCLL(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MaxFALL(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata(){};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_PrimaryRChromaticityX(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_PrimaryRChromaticityY(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_PrimaryGChromaticityX(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_PrimaryGChromaticityY(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_PrimaryBChromaticityX(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_PrimaryBChromaticityY(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_WhitePointChromaticityX(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_WhitePointChromaticityY(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_LuminanceMax(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Colour_MasteringMetadata_LuminanceMin(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Projection(){};
    void Segment_Tracks_TrackEntry_Video_Projection_ProjectionType(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_Video_Projection_ProjectionPrivate(){Skip_XX(Element_Size, "Data");};
    void Segment_Tracks_TrackEntry_Video_Projection_ProjectionPoseYaw(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Projection_ProjectionPosePitch(){Float_Info();};
    void Segment_Tracks_TrackEntry_Video_Projection_ProjectionPoseRoll(){Float_Info();};
    void Segment_Tracks_TrackEntry_Audio(){};
    void Segment_Tracks_TrackEntry_Audio_SamplingFrequency();
    void Segment_Tracks_TrackEntry_Audio_OutputSamplingFrequency();
    void Segment_Tracks_TrackEntry_Audio_Channels();
    void Segment_Tracks_TrackEntry_Audio_ChannelPositions(){Skip_XX(Element_Size, "Data");};
    void Segment_Tracks_TrackEntry_Audio_BitDepth();
    void Segment_Tracks_TrackEntry_TrackOperation(){};
    void Segment_Tracks_TrackEntry_TrackOperation_TrackCombinePlanes(){};
    void Segment_Tracks_TrackEntry_TrackOperation_TrackCombinePlanes_TrackPlane(){};
    void Segment_Tracks_TrackEntry_TrackOperation_TrackCombinePlanes_TrackPlane_TrackPlaneUID(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrackOperation_TrackCombinePlanes_TrackPlane_TrackPlaneType(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrackOperation_TrackJoinBlocks(){};
    void Segment_Tracks_TrackEntry_TrackOperation_TrackJoinBlocks_TrackJoinUID(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrickTrackUID(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrickTrackSegmentUID(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrickTrackFlag(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrickMasterTrackUID(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_TrickMasterTrackSegmentUID(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_ContentEncodings(){};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding(){};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncodingOrder(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncodingScope(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncodingType(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression();
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression_ContentCompAlgo();
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentCompression_ContentCompSettings();
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption(){};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentEncAlgo(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentEncKeyID(){Skip_XX(Element_Size, "Data");};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSignature(){Skip_XX(Element_Size, "Data");};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSigKeyID(){Skip_XX(Element_Size, "Data");};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSigAlgo(){UInteger_Info();};
    void Segment_Tracks_TrackEntry_ContentEncodings_ContentEncoding_ContentEncryption_ContentSigHashAlgo(){UInteger_Info();};
    void Segment_Cues();
    void Segment_Cues_CuePoint();
    void Segment_Cues_CuePoint_CueTime(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions(){};
    void Segment_Cues_CuePoint_CueTrackPositions_CueTrack(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions_CueClusterPosition(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions_CueRelativePosition(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions_CueDuration(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions_CueBlockNumber(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions_CueCodecState(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions_CueReference(){};
    void Segment_Cues_CuePoint_CueTrackPositions_CueReference_CueRefTime(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions_CueReference_CueRefCluster(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions_CueReference_CueRefNumber(){UInteger_Info();};
    void Segment_Cues_CuePoint_CueTrackPositions_CueReference_CueRefCodecState(){UInteger_Info();};
    void Segment_Attachments(){};
    void Segment_Attachments_AttachedFile();
    void Segment_Attachments_AttachedFile_FileDescription();
    void Segment_Attachments_AttachedFile_FileName();
    void Segment_Attachments_AttachedFile_FileMimeType();
    void Segment_Attachments_AttachedFile_FileData();
    void Segment_Attachments_AttachedFile_FileUID(){UInteger_Info();};
    void Segment_Attachments_AttachedFile_FileReferral(){Skip_XX(Element_Size, "Data");};
    void Segment_Attachments_AttachedFile_FileUsedStartTime(){UInteger_Info();};
    void Segment_Attachments_AttachedFile_FileUsedEndTime(){UInteger_Info();};
    void Segment_Chapters(){};
    void Segment_Chapters_EditionEntry();
    void Segment_Chapters_EditionEntry_EditionUID(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_EditionFlagHidden(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_EditionFlagDefault(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_EditionFlagOrdered(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom();
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterUID(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterStringUID(){UTF8_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterTimeStart();
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterTimeEnd(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterFlagHidden(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterFlagEnabled(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterSegmentUID(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterSegmentEditionUID(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterPhysicalEquiv(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterTrack(){};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterTrack_ChapterTrackNumber(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay();
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapString();
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapLanguage();
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapLanguageIETF(){String_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapterDisplay_ChapCountry(){String_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess(){};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCodecID(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessPrivate(){Skip_XX(Element_Size, "Data");};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand(){};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand_ChapProcessTime(){UInteger_Info();};
    void Segment_Chapters_EditionEntry_ChapterAtom_ChapProcess_ChapProcessCommand_ChapProcessData(){Skip_XX(Element_Size, "Data");};
    void Segment_Tags();
    void Segment_Tags_Tag();
    void Segment_Tags_Tag_Targets(){};
    void Segment_Tags_Tag_Targets_TargetTypeValue(){UInteger_Info();};
    void Segment_Tags_Tag_Targets_TargetType(){String_Info();};
    void Segment_Tags_Tag_Targets_TagTrackUID();
    void Segment_Tags_Tag_Targets_TagEditionUID(){UInteger_Info();};
    void Segment_Tags_Tag_Targets_TagChapterUID(){UInteger_Info();};
    void Segment_Tags_Tag_Targets_TagAttachmentUID(){UInteger_Info();};
    void Segment_Tags_Tag_SimpleTag(){};
    void Segment_Tags_Tag_SimpleTag_TagName();
    void Segment_Tags_Tag_SimpleTag_TagLanguage();
    void Segment_Tags_Tag_SimpleTag_TagLanguageIETF(){String_Info();};
    void Segment_Tags_Tag_SimpleTag_TagDefault(){UInteger_Info();};
    void Segment_Tags_Tag_SimpleTag_TagString();
    void Segment_Tags_Tag_SimpleTag_TagBinary(){Skip_XX(Element_Size, "Data");};

    struct stream
    {
        std::vector<int64u>     TimeCodes;
        int64u                  TimeCode_Start;
        int64u                  TrackUID;
        File__Analyze*          Parser;
        stream_t                StreamKind;
        size_t                  StreamPos;
        size_t                  PacketCount;
        int32u                  AvgBytesPerSec; //Only used by x_MS/* codecIDs
        float32                 DisplayAspectRatio;
        float64                 FrameRate;
        bool                    Searching_Payload;
        bool                    Searching_TimeStamps;
        bool                    Searching_TimeStamp_Start;
        bool                    Default;
        bool                    Forced;
        int64u                  ContentCompAlgo;
        size_t                  ContentCompSettings_Buffer_Size;
        int8u*                  ContentCompSettings_Buffer;
        std::map<std::string, Ztring> Infos;
        int64u                  TrackDefaultDuration;
        std::map<int64u, int64u> Segment_Cluster_BlockGroup_BlockDuration_Counts;
        int64u                  PixelCropBottom;
        int64u                  PixelCropLeft;
        int64u                  PixelCropRight;
        int64u                  PixelCropTop;
        #if MEDIAINFO_TRACE
            size_t Trace_Segment_Cluster_Block_Count;
        #endif // MEDIAINFO_TRACE

        stream()
        {
            TimeCode_Start=(int64u)-1;
            TrackUID=(int64u)-1;
            Parser=NULL;
            StreamKind=Stream_Max;
            StreamPos=0;
            PacketCount=0;
            AvgBytesPerSec=0;
            DisplayAspectRatio=0;
            FrameRate=0;
            Searching_Payload=true;
            Searching_TimeStamps=false;
            Searching_TimeStamp_Start=false;
            Default=true;
            Forced=false;
            ContentCompAlgo=(int32u)-1;
            ContentCompSettings_Buffer_Size=0;
            ContentCompSettings_Buffer=NULL;
            TrackDefaultDuration=0;
            PixelCropBottom=0;
            PixelCropLeft=0;
            PixelCropRight=0;
            PixelCropTop=0;
            #if MEDIAINFO_TRACE
                Trace_Segment_Cluster_Block_Count=0;
            #endif // MEDIAINFO_TRACE
        }

        ~stream()
        {
            delete Parser; //Parser=NULL;
            delete[] ContentCompSettings_Buffer; //ContentCompSettings_Buffer=NULL;
        }
    };
    std::map<int64u, stream> Stream;
    size_t                   Stream_Count;

    //Data
    int64u   UInteger_Get();
    int128u  UInteger16_Get();
    void     UInteger_Info();

    float64  Float_Get();
    void     Float_Info();

    Ztring   UTF8_Get();
    void     UTF8_Info();

    Ztring   String_Get();
    void     String_Info();

    //Temp - TrackEntry
    int8u*   CodecPrivate;
    size_t   CodecPrivate_Size;
    void     CodecPrivate_Manage();
    Ztring   CodecID;
    infocodecid_format_t InfoCodecID_Format_Type;
    void     CodecID_Manage();
    int64u   TrackType;

    //Temp
    int8u   InvalidByteMax;
    int64u  Format_Version;
    int64u  TimecodeScale;
    float64 Duration;
    int64u  TrackNumber;
    int64u  TrackVideoDisplayWidth;
    int64u  TrackVideoDisplayHeight;
    int32u  AvgBytesPerSec;
    int64u  Segment_Cluster_TimeCode_Value;
    size_t  Segment_Info_Count;
    size_t  Segment_Tracks_Count;
    size_t  Segment_Cluster_Count;
    typedef std::map<Ztring, Ztring> tagspertrack;
    typedef std::map<int64u, tagspertrack> tags;
    tags    Segment_Tags_Tag_Items;
    int64u  Segment_Tags_Tag_Targets_TagTrackUID_Value;
    bool    CurrentAttachmentIsCover;
    bool    CoverIsSetFromAttachment;
    string  AttachedFile_FileName;
    string  AttachedFile_FileMimeType;
    string  AttachedFile_FileDescription;
    struct crc32
    {
        int64u  Pos;
        int64u  From;
        int64u  UpTo;
        int32u  Computed;
        int32u  Expected;
    };
    std::vector<crc32> CRC32Compute;
    int64u CRC32Compute_SkipUpTo;

    //Chapters
    struct chapterdisplay
    {
        Ztring ChapLanguage;
        Ztring ChapString;
    };
    struct chapteratom
    {
        int64u ChapterTimeStart;
        std::vector<chapterdisplay> ChapterDisplays;

        chapteratom()
        {
            ChapterTimeStart=(int64u)-1;
        }
    };
    struct editionentry
    {
        std::vector<chapteratom> ChapterAtoms;
    };
    std::vector<editionentry> EditionEntries;
    size_t EditionEntries_Pos;
    size_t ChapterAtoms_Pos;
    size_t ChapterDisplays_Pos;
    int64u              Segment_Offset_Begin;
    int64u              Segment_Offset_End;
    int64u              IsParsingSegmentTrack_SeekBackTo;
    int64u              SegmentTrack_Offset_End;
    struct seek
    {
        int64u SeekID;
        int64u SeekPosition;

        seek()
            : SeekID(0)
            , SeekPosition(0)
        {}

        bool operator < (const seek &s) const
        {
            return SeekPosition < s.SeekPosition;
        }
    };
    std::vector<seek>   Segment_Seeks;
    size_t              Segment_Seeks_Pos;
    std::vector<Ztring> Segment_Tag_SimpleTag_TagNames;
    int64u Segment_Cluster_BlockGroup_BlockDuration_Value;
    int64u Segment_Cluster_BlockGroup_BlockDuration_TrackNumber;
    std::vector<int64u> Laces;
    size_t              Laces_Pos;
    #if MEDIAINFO_DEMUX
        int64u              Demux_EventWasSent;
    #endif //MEDIAINFO_DEMUX

    //Hints
    size_t*                 File_Buffer_Size_Hint_Pointer;

    //Helpers
    void Segment_Tracks_TrackEntry_CodecPrivate__Parse();
    void Segment_Tracks_TrackEntry_CodecPrivate_auds();
    void Segment_Tracks_TrackEntry_CodecPrivate_auds_ExtensibleWave(int16u BitsPerSample);
    void Segment_Tracks_TrackEntry_CodecPrivate_vids();
    void JumpTo(int64u GoTo);
    void TestMultipleInstances(size_t* Instances=NULL);
    void CRC32_Check();
    #if MEDIAINFO_TRACE
        bool CRC32_Check_In_Node(const std::string& ToSearchInInfo, const std::string& info, element_details::Element_Node *node);
        size_t Trace_Segment_Cluster_Count;
        size_t Trace_Segment_Cues_CuePoint_Count;
        size_t Trace_Segment_SeekHead_Seek_Count;
    #endif // MEDIAINFO_TRACE
};

} //NameSpace

#endif

