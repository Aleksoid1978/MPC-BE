/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Configuration of MediaInfo (per MediaInfo block)
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_Config_MediaInfoH
#define MediaInfo_Config_MediaInfoH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Internal_Const.h"
#include "MediaInfo/HashWrapper.h" //For MEDIAINFO_HASH
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Config.h"
    #include "MediaInfo/MediaInfo_Config_PerPackage.h"
    #include "MediaInfo/MediaInfo_Events.h"
    #include "ZenLib/File.h"
#endif //MEDIAINFO_EVENTS
#include "ZenLib/CriticalSection.h"
#include "ZenLib/Translation.h"
#include "ZenLib/InfoMap.h"
#if MEDIAINFO_ADVANCED
    #include "MediaInfo/TimeCode.h"
#endif //MEDIAINFO_ADVANCED
using namespace ZenLib;
using std::string;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

#if MEDIAINFO_EVENTS
    class File__Analyze;
#endif //MEDIAINFO_EVENTS

#if MEDIAINFO_AES
enum encryption_format
{
    Encryption_Format_None,
    Encryption_Format_Aes,
};
enum encryption_method
{
     Encryption_Method_None,
     Encryption_Method_Segment,
};
enum encryption_mode
{
     Encryption_Mode_None,
     Encryption_Mode_Cbc,
};
enum encryption_padding
{
     Encryption_Padding_None,
     Encryption_Padding_Pkcs7,
};
#endif //MEDIAINFO_AES

enum config_probe_type
{
    config_probe_none,
    config_probe_size,
    config_probe_dur,
    config_probe_percent,
};
struct config_probe
{
    config_probe_type   Start_Type=config_probe_none;
    config_probe_type   Duration_Type=config_probe_none;
    int64u              Start=0;
    int64u              Duration=0;
    string              Parser;
};

enum display_captions
{
    DisplayCaptions_Command,
    DisplayCaptions_Content,
    DisplayCaptions_Stream,
    DisplayCaptions_Max
};

//***************************************************************************
// Class MediaInfo_Config_MediaInfo
//***************************************************************************

class MediaInfo_Config_MediaInfo
{
public :
    //Constructor/Destructor
    MediaInfo_Config_MediaInfo();
    ~MediaInfo_Config_MediaInfo();

    bool          RequestTerminate;

    //General
    Ztring Option (const String &Option, const String &Value=Ztring());

    void          File_IsSeekable_Set (bool NewValue);
    bool          File_IsSeekable_Get ();

    void          File_IsSub_Set (bool NewValue);
    bool          File_IsSub_Get ();

    void          File_ParseSpeed_Set(float32 NewValue, bool FromGlobal=false);
    float32       File_ParseSpeed_Get();

    void          File_IsDetectingDuration_Set (bool NewValue);
    bool          File_IsDetectingDuration_Get ();

    void          File_IsReferenced_Set (bool NewValue);
    bool          File_IsReferenced_Get ();

    void          File_TestContinuousFileNames_Set (bool NewValue);
    bool          File_TestContinuousFileNames_Get ();

    void          File_TestDirectory_Set (bool NewValue);
    bool          File_TestDirectory_Get ();

    void          File_KeepInfo_Set (bool NewValue);
    bool          File_KeepInfo_Get ();

    void          File_StopAfterFilled_Set (bool NewValue);
    bool          File_StopAfterFilled_Get ();

    void          File_StopSubStreamAfterFilled_Set (bool NewValue);
    bool          File_StopSubStreamAfterFilled_Get ();

    void          File_Audio_MergeMonoStreams_Set (bool NewValue);
    bool          File_Audio_MergeMonoStreams_Get ();

    void          File_Demux_Interleave_Set (bool NewValue);
    bool          File_Demux_Interleave_Get ();

    void          File_ID_OnlyRoot_Set (bool NewValue);
    bool          File_ID_OnlyRoot_Get ();

    void          File_ExpandSubs_Set(bool NewValue);
    bool          File_ExpandSubs_Get();
    void          File_ExpandSubs_Update(void** Source);

    #if MEDIAINFO_ADVANCED
        void          File_IgnoreSequenceFileSize_Set (bool NewValue);
        bool          File_IgnoreSequenceFileSize_Get ();
    #endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_ADVANCED
        void          File_IgnoreSequenceFilesCount_Set (bool NewValue);
        bool          File_IgnoreSequenceFilesCount_Get ();
    #endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_ADVANCED
        void          File_SequenceFilesSkipFrames_Set (int64u NewValue);
        int64u        File_SequenceFilesSkipFrames_Get ();
    #endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_ADVANCED
        void          File_DefaultFrameRate_Set (float64 NewValue);
        float64       File_DefaultFrameRate_Get ();
        void          File_DefaultTimeCode_Set (string NewValue);
        string        File_DefaultTimeCode_Get ();
        Ztring        File_DefaultTimeCodeDropFrame_Set (const String& NewValue);
        int8u         File_DefaultTimeCodeDropFrame_Get ();
    #endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_ADVANCED
        void          File_Source_List_Set (bool NewValue);
        bool          File_Source_List_Get ();
    #endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_ADVANCED
        void          File_RiskyBitRateEstimation_Set (bool NewValue);
        bool          File_RiskyBitRateEstimation_Get ();
        void          File_MergeBitRateInfo_Set (bool NewValue);
        bool          File_MergeBitRateInfo_Get ();
        void          File_HighestFormat_Set (bool NewValue);
        bool          File_HighestFormat_Get ();
        void          File_ChannelLayout_Set(bool NewValue);
        bool          File_ChannelLayout_Get();
        void          File_FrameIsAlwaysComplete_Set(bool NewValue) { File_FrameIsAlwaysComplete = NewValue; }
        bool          File_FrameIsAlwaysComplete_Get() { return File_FrameIsAlwaysComplete; }
#endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_DEMUX
        #if MEDIAINFO_ADVANCED
            void          File_Demux_Unpacketize_StreamLayoutChange_Skip_Set (bool NewValue);
            bool          File_Demux_Unpacketize_StreamLayoutChange_Skip_Get ();
        #endif //MEDIAINFO_ADVANCED
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_HASH
        void         File_Hash_Set (HashWrapper::HashFunctions Funtions);
        HashWrapper::HashFunctions File_Hash_Get ();
    #endif //MEDIAINFO_HASH
    #if MEDIAINFO_MD5
        void          File_Md5_Set (bool NewValue);
        bool          File_Md5_Get ();
    #endif //MEDIAINFO_MD5

    #if defined(MEDIAINFO_REFERENCES_YES)
        void          File_CheckSideCarFiles_Set (bool NewValue);
        bool          File_CheckSideCarFiles_Get ();
    #endif //defined(MEDIAINFO_REFERENCES_YES)

    void          File_FileName_Set (const Ztring &NewValue);
    Ztring        File_FileName_Get ();

    void          File_FileNameFormat_Set (const Ztring &NewValue);
    Ztring        File_FileNameFormat_Get ();

    void          File_TimeToLive_Set (float64 NewValue);
    float64       File_TimeToLive_Get ();

    void          File_Partial_Begin_Set (const Ztring &NewValue);
    Ztring        File_Partial_Begin_Get ();
    void          File_Partial_End_Set (const Ztring &NewValue);
    Ztring        File_Partial_End_Get ();

    void          File_ForceParser_Set (const Ztring &NewValue);
    Ztring        File_ForceParser_Get ();
    void          File_ForceParser_Config_Set (const Ztring &NewValue);
    Ztring        File_ForceParser_Config_Get ();

    void          File_Buffer_Size_Hint_Pointer_Set (size_t* NewValue);
    size_t*       File_Buffer_Size_Hint_Pointer_Get ();

    void          File_Buffer_Read_Size_Set (size_t NewValue);
    size_t        File_Buffer_Read_Size_Get ();

    #if MEDIAINFO_AES
    void          Encryption_Format_Set (const Ztring &Value);
    void          Encryption_Format_Set (encryption_format Value);
    string        Encryption_Format_GetS ();
    encryption_format Encryption_Format_Get ();
    void          Encryption_Key_Set (const Ztring &Value);
    void          Encryption_Key_Set (const int8u* Value, size_t Value_Size);
    string        Encryption_Key_Get ();
    void          Encryption_Method_Set (const Ztring &Value);
    void          Encryption_Method_Set (encryption_method Value);
    string        Encryption_Method_GetS ();
    encryption_method Encryption_Method_Get ();
    void          Encryption_Mode_Set (const Ztring &Value);
    void          Encryption_Mode_Set (encryption_mode Value);
    string        Encryption_Mode_GetS ();
    encryption_mode Encryption_Mode_Get ();
    void          Encryption_Padding_Set (const Ztring &Value);
    void          Encryption_Padding_Set (encryption_padding Value);
    string        Encryption_Padding_GetS ();
    encryption_padding  Encryption_Padding_Get ();
    void          Encryption_InitializationVector_Set (const Ztring &Value);
    string        Encryption_InitializationVector_Get ();
    #endif //MEDIAINFO_AES

    #if MEDIAINFO_NEXTPACKET
    void          NextPacket_Set (bool NewValue);
    bool          NextPacket_Get ();
    #endif //MEDIAINFO_NEXTPACKET

    #if MEDIAINFO_FILTER
    void          File_Filter_Set     (int64u NewValue);
    bool          File_Filter_Get     (const int16u  Value);
    bool          File_Filter_Get     ();
    void          File_Filter_Audio_Set (bool NewValue);
    bool          File_Filter_Audio_Get ();
    bool          File_Filter_HasChanged();
    #endif //MEDIAINFO_FILTER

    #if MEDIAINFO_DUPLICATE
    Ztring        File_Duplicate_Set  (const Ztring &Value);
    Ztring        File_Duplicate_Get  (size_t AlreadyRead_Pos); //Requester must say how many Get() it already read
    bool          File_Duplicate_Get_AlwaysNeeded (size_t AlreadyRead_Pos); //Requester must say how many Get() it already read
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_DUPLICATE
    size_t        File__Duplicate_Memory_Indexes_Get (const Ztring &ToFind);
    void          File__Duplicate_Memory_Indexes_Erase (const Ztring &ToFind);
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_EVENTS
    ZtringListList SubFile_Config_Get ();
    void          SubFile_StreamID_Set(int64u Value);
    int64u        SubFile_StreamID_Get();
    void          SubFile_IDs_Set(Ztring Value);
    Ztring        SubFile_IDs_Get();
    #endif //MEDIAINFO_EVENTS

    #if MEDIAINFO_EVENTS
    bool          ParseUndecodableFrames_Get ();
    void          ParseUndecodableFrames_Set (bool Value);
    #endif //MEDIAINFO_EVENTS

    #if MEDIAINFO_EVENTS
    bool          Event_CallBackFunction_IsSet ();
    Ztring        Event_CallBackFunction_Set (const Ztring &Value);
    Ztring        Event_CallBackFunction_Get ();
    void          Event_Send(File__Analyze* Source, const int8u* Data_Content, size_t Data_Size, const Ztring &File_Name=Ztring());
    void          Event_Accepted(File__Analyze* Source);
    void          Event_SubFile_Start(const Ztring &FileName_Absolute);
    void          Event_SubFile_Missing(const Ztring &FileName_Relative);
    void          Event_SubFile_Missing_Absolute(const Ztring &FileName_Absolute);
    #endif //MEDIAINFO_EVENTS

    void          Demux_Rate_Set (float64 NewValue);
    float64       Demux_Rate_Get ();
    #if MEDIAINFO_DEMUX
    void          Demux_ForceIds_Set (bool NewValue);
    bool          Demux_ForceIds_Get ();
    void          Demux_PCM_20bitTo16bit_Set (bool NewValue);
    bool          Demux_PCM_20bitTo16bit_Get ();
    void          Demux_PCM_20bitTo24bit_Set (bool NewValue);
    bool          Demux_PCM_20bitTo24bit_Get ();
    void          Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Set (bool NewValue);
    bool          Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Get ();
    void          Demux_Hevc_Transcode_Iso14496_15_to_AnnexB_Set (bool NewValue);
    bool          Demux_Hevc_Transcode_Iso14496_15_to_AnnexB_Get ();
    void          Demux_Unpacketize_Set (bool NewValue);
    bool          Demux_Unpacketize_Get ();
    void          Demux_FirstDts_Set (int64u NewValue);
    int64u        Demux_FirstDts_Get ();
    void          Demux_FirstFrameNumber_Set (int64u NewValue);
    int64u        Demux_FirstFrameNumber_Get ();
    void          Demux_InitData_Set (int8u NewValue);
    int8u         Demux_InitData_Get ();
    void          Demux_SplitAudioBlocks_Set(bool NewValue);
    bool          Demux_SplitAudioBlocks_Get();
    std::map<Ztring, File> Demux_Files;
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_IBIUSAGE
    void          Ibi_Set (const Ztring &NewValue);
    std::string   Ibi_Get ();
    void          Ibi_UseIbiInfoIfAvailable_Set (bool NewValue);
    bool          Ibi_UseIbiInfoIfAvailable_Get ();
    #endif //MEDIAINFO_IBIUSAGE
    #if MEDIAINFO_IBIUSAGE
    void          Ibi_Create_Set (bool NewValue);
    bool          Ibi_Create_Get ();
    #endif //MEDIAINFO_IBIUSAGE
    #if MEDIAINFO_FIXITY
    void          TryToFix_Set (bool NewValue);
    bool          TryToFix_Get ();
    #endif //MEDIAINFO_FIXITY

    //Specific
    void          File_MpegTs_ForceMenu_Set (bool NewValue);
    bool          File_MpegTs_ForceMenu_Get ();
    void          File_MpegTs_stream_type_Trust_Set (bool NewValue);
    bool          File_MpegTs_stream_type_Trust_Get ();
    void          File_MpegTs_Atsc_transport_stream_id_Trust_Set (bool NewValue);
    bool          File_MpegTs_Atsc_transport_stream_id_Trust_Get ();
    void          File_MpegTs_RealTime_Set (bool NewValue);
    bool          File_MpegTs_RealTime_Get ();
    void          File_Mxf_TimeCodeFromMaterialPackage_Set (bool NewValue);
    bool          File_Mxf_TimeCodeFromMaterialPackage_Get ();
    void          File_Mxf_ParseIndex_Set (bool NewValue);
    bool          File_Mxf_ParseIndex_Get ();
    void          File_Bdmv_ParseTargetedFile_Set (bool NewValue);
    bool          File_Bdmv_ParseTargetedFile_Get ();
    #if defined(MEDIAINFO_DVDIF_YES)
    void          File_DvDif_DisableAudioIfIsInContainer_Set (bool NewValue);
    bool          File_DvDif_DisableAudioIfIsInContainer_Get ();
    void          File_DvDif_IgnoreTransmittingFlags_Set (bool NewValue);
    bool          File_DvDif_IgnoreTransmittingFlags_Get ();
    #endif //defined(MEDIAINFO_DVDIF_YES)
    #if defined(MEDIAINFO_DVDIF_ANALYZE_YES)
    void          File_DvDif_Analysis_Set (bool NewValue);
    bool          File_DvDif_Analysis_Get ();
    #endif //defined(MEDIAINFO_DVDIF_ANALYZE_YES)
    #if MEDIAINFO_MACROBLOCKS
    void          File_Macroblocks_Parse_Set (int NewValue);
    int           File_Macroblocks_Parse_Get ();
    #endif //MEDIAINFO_MACROBLOCKS
    void          File_GrowingFile_Delay_Set(float64 Value);
    float64       File_GrowingFile_Delay_Get();
    void          File_GrowingFile_Force_Set(bool Value);
    bool          File_GrowingFile_Force_Get();
    #if defined(MEDIAINFO_LIBCURL_YES)
    void          File_Curl_Set (const Ztring &NewValue);
    void          File_Curl_Set (const Ztring &Field, const Ztring &NewValue);
    Ztring        File_Curl_Get (const Ztring &Field);
    #endif //defined(MEDIAINFO_LIBCURL_YES)
    #if defined(MEDIAINFO_LIBMMS_YES)
    void          File_Mmsh_Describe_Only_Set (bool NewValue);
    bool          File_Mmsh_Describe_Only_Get ();
    #endif //defined(MEDIAINFO_LIBMMS_YES)
    Ztring        File_DisplayCaptions_Set (const Ztring& NewValue);
    display_captions File_DisplayCaptions_Get ();
    Ztring        File_ProbeCaption_Set(const Ztring& NewValue);
    config_probe  File_ProbeCaption_Get(const string& Parser);
    #if defined(MEDIAINFO_AC3_YES)
    void          File_Ac3_IgnoreCrc_Set (bool NewValue);
    bool          File_Ac3_IgnoreCrc_Get ();
    #endif //defined(MEDIAINFO_AC3_YES)

    //Analysis internal
    void          State_Set (float State);
    float         State_Get ();

    //Internal to MediaInfo, not thread safe
    ZtringList    File_Names;
    std::vector<int64u> File_Sizes;
    size_t        File_Names_Pos;
    size_t        File_Buffer_Size_Max;
    size_t        File_Buffer_Size_ToRead;
    size_t        File_Buffer_Size;
    int8u*        File_Buffer;
    bool          File_Buffer_Repeat;
    bool          File_Buffer_Repeat_IsSupported;
    bool          File_IsGrowing;
    bool          File_IsNotGrowingAnymore;
    bool          File_IsImageSequence;
    #if defined(MEDIAINFO_EIA608_YES)
    bool          File_Scte20_IsPresent;
    #endif //defined(MEDIAINFO_EIA608_YES)
    #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
    bool          File_DtvccTransport_Stream_IsPresent;
    bool          File_DtvccTransport_Descriptor_IsPresent;
    #endif //defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
    #if defined(MEDIAINFO_MPEGPS_YES)
    bool          File_MpegPs_PTS_Begin_IsNearZero;
    #endif //defined(MEDIAINFO_MPEGPS_YES)
    int64u        File_Current_Offset;
    int64u        File_Current_Size;
    int64u        File_IgnoreEditsBefore;
    int64u        File_IgnoreEditsAfter;
    float64       File_EditRate;
    int64u        File_Size;
    float32       ParseSpeed;
    bool          ParseSpeed_FromFile;
    bool          IsFinishing;
    #if MEDIAINFO_EVENTS
    MediaInfo_Config_PerPackage* Config_PerPackage;
    bool          Events_TimestampShift_Disabled;
    Ztring        File_Names_RootDirectory;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
    bool          Demux_EventWasSent;
    int64u          Demux_Offset_Frame;
    int64u          Demux_Offset_DTS;
    int64u          Demux_Offset_DTS_FromStream;
    File__Analyze*  Events_Delayed_CurrentSource;
        #if MEDIAINFO_SEEK
        bool      Demux_IsSeeking;
        #endif //MEDIAINFO_SEEK
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_SEEK
    bool      File_GoTo_IsFrameOffset;
    #endif //MEDIAINFO_SEEK

    //Logs
    #if MEDIAINFO_ADVANCED
        struct timecode_dump
        {
            std::string List;
            TimeCode LastTC;
            int32u FramesMax=0;
            int64u FrameCount=0;
            string Attributes_First;
            string Attributes_Last;
        };
        std::map<std::string, timecode_dump>* TimeCode_Dumps;
    #endif //MEDIAINFO_ADVANCED

private :
    bool                    FileIsSeekable;
    bool                    FileIsSub;
    bool                    FileIsDetectingDuration;
    bool                    FileIsReferenced;
    bool                    FileTestContinuousFileNames;
    bool                    FileTestDirectory;
    bool                    FileKeepInfo;
    bool                    FileStopAfterFilled;
    bool                    FileStopSubStreamAfterFilled;
    bool                    Audio_MergeMonoStreams;
    bool                    File_Demux_Interleave;
    bool                    File_ID_OnlyRoot;
    void*                   File_ExpandSubs_Backup;
    void*                   File_ExpandSubs_Source;
    #if MEDIAINFO_ADVANCED
        bool                File_IgnoreSequenceFileSize;
        bool                File_IgnoreSequenceFilesCount;
        int64u              File_SequenceFilesSkipFrames;
        float64             File_DefaultFrameRate;
        string              File_DefaultTimeCode;
        int8u               File_DefaultTimeCodeDropFrame;
        bool                File_Source_List;
        bool                File_RiskyBitRateEstimation;
        bool                File_MergeBitRateInfo;
        bool                File_HighestFormat;
        bool                File_ChannelLayout;
        bool                File_FrameIsAlwaysComplete;
        #if MEDIAINFO_DEMUX
            bool                File_Demux_Unpacketize_StreamLayoutChange_Skip;
        #endif //MEDIAINFO_DEMUX
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_HASH
        HashWrapper::HashFunctions Hash_Functions;
    #endif //MEDIAINFO_HASH
    #if MEDIAINFO_MD5
        bool                File_Md5;
    #endif //MEDIAINFO_MD5
    #if defined(MEDIAINFO_REFERENCES_YES)
        bool                File_CheckSideCarFiles;
    #endif //defined(MEDIAINFO_REFERENCES_YES)
    Ztring                  File_FileName;
    Ztring                  File_FileNameFormat;
    float64                 File_TimeToLive;
    Ztring                  File_Partial_Begin;
    Ztring                  File_Partial_End;
    Ztring                  File_ForceParser;
    Ztring                  File_ForceParser_Config;
    size_t*                 File_Buffer_Size_Hint_Pointer;
    size_t                  File_Buffer_Read_Size;

    //Extra
    #if MEDIAINFO_AES
    encryption_format       Encryption_Format;
    string                  Encryption_Key;
    encryption_method       Encryption_Method;
    encryption_mode         Encryption_Mode;
    encryption_padding      Encryption_Padding;
    string                  Encryption_InitializationVector;
    #endif //MEDIAINFO_AES

    #if MEDIAINFO_NEXTPACKET
    bool                    NextPacket;
    #endif //MEDIAINFO_NEXTPACKET

    #if MEDIAINFO_FILTER
    std::map<int16u, bool>  File_Filter_16;
    bool                    File_Filter_Audio;
    bool                    File_Filter_HasChanged_;
    #endif //MEDIAINFO_FILTER

    #if MEDIAINFO_DUPLICATE
    std::vector<Ztring>     File__Duplicate_List;
    ZtringList              File__Duplicate_Memory_Indexes;
    #endif //MEDIAINFO_DUPLICATE

    //Event
    #if MEDIAINFO_EVENTS
    MediaInfo_Event_CallBackFunction* Event_CallBackFunction; //void Event_Handler(unsigned char* Data_Content, size_t Data_Size, void* UserHandler)
    struct event_delayed
    {
        int8u* Data_Content;
        size_t Data_Size;
        Ztring File_Name;

        event_delayed (const int8u* Data_Content_, size_t Data_Size_, const Ztring &File_Name_)
        {
            File_Name=File_Name_;
            Data_Size=Data_Size_;
            Data_Content=new int8u[Data_Size];
            std::memcpy(Data_Content, Data_Content_, Data_Size);
        }

        ~event_delayed ()
        {
            delete[] Data_Content; //Data_Content=NULL;
        }
    };
    typedef std::map<File__Analyze*, std::vector<event_delayed*> > events_delayed;
    events_delayed Events_Delayed;
    void*                   Event_UserHandler;
    ZtringListList          SubFile_Config;
    int64u                  SubFile_StreamID;
    bool                    ParseUndecodableFrames;
    Ztring                  SubFile_IDs;
    int64u                      Events_TimestampShift_Reference_PTS;
    int64u                      Events_TimestampShift_Reference_ID;
    std::vector<event_delayed*> Events_TimestampShift_Delayed;
    #endif //MEDIAINFO_EVENTS

    float64                 Demux_Rate;
    #if MEDIAINFO_DEMUX
    bool                    Demux_ForceIds;
    bool                    Demux_PCM_20bitTo16bit;
    bool                    Demux_PCM_20bitTo24bit;
    bool                    Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10;
    bool                    Demux_Hevc_Transcode_Iso14496_15_to_AnnexB;
    bool                    Demux_Unpacketize;
    bool                    Demux_SplitAudioBlocks;
    int64u                  Demux_FirstDts;
    int64u                  Demux_FirstFrameNumber;
    int8u                   Demux_InitData;
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_IBIUSAGE
    std::string             Ibi;
    bool                    Ibi_UseIbiInfoIfAvailable;
    #endif //MEDIAINFO_IBIUSAGE
    #if MEDIAINFO_IBIUSAGE
    bool                    Ibi_Create;
    #endif //MEDIAINFO_IBIUSAGE
    #if MEDIAINFO_FIXITY
    bool                    TryToFix;
    #endif //MEDIAINFO_SEEK

    //Specific
    bool                    File_MpegTs_ForceMenu;
    bool                    File_MpegTs_stream_type_Trust;
    bool                    File_MpegTs_Atsc_transport_stream_id_Trust;
    bool                    File_MpegTs_RealTime;
    bool                    File_Mxf_TimeCodeFromMaterialPackage;
    bool                    File_Mxf_ParseIndex;
    bool                    File_Bdmv_ParseTargetedFile;
    #if defined(MEDIAINFO_DVDIF_YES)
    bool                    File_DvDif_DisableAudioIfIsInContainer;
    bool                    File_DvDif_IgnoreTransmittingFlags;
    #endif //defined(MEDIAINFO_DVDIF_ANALYZE_YES)
    #if defined(MEDIAINFO_DVDIF_ANALYZE_YES)
    bool                    File_DvDif_Analysis;
    #endif //defined(MEDIAINFO_DVDIF_ANALYZE_YES)
    #if MEDIAINFO_MACROBLOCKS
    int                     File_Macroblocks_Parse;
    #endif //MEDIAINFO_MACROBLOCKS
    float64                 File_GrowingFile_Delay;
    bool                    File_GrowingFile_Force;
    #if defined(MEDIAINFO_LIBMMS_YES)
    bool                    File_Mmsh_Describe_Only;
    #endif //defined(MEDIAINFO_LIBMMS_YES)
    display_captions        DisplayCaptions;
    std::vector<config_probe> File_ProbeCaption;
    size_t                  File_ProbeCaption_Pos;
    #if defined(MEDIAINFO_AC3_YES)
    bool                    File_Ac3_IgnoreCrc;
    #endif //defined(MEDIAINFO_AC3_YES)

    //Analysis internal
    float                   State;

    //Generic
    #if defined(MEDIAINFO_LIBCURL_YES)
    std::map<Ztring, Ztring> Curl;
    #endif //defined(MEDIAINFO_LIBCURL_YES)

    ZenLib::CriticalSection CS;

    //Constructor
    MediaInfo_Config_MediaInfo (const MediaInfo_Config_MediaInfo&);             // Prevent copy-construction
    MediaInfo_Config_MediaInfo& operator=(const MediaInfo_Config_MediaInfo&);   // Prevent assignment
};

} //NameSpace

#endif
