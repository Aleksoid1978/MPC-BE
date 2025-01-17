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
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/MediaInfo_Config.h"
#include "MediaInfo/TimeCode.h"
#include "ZenLib/ZtringListListF.h"
#if MEDIAINFO_EVENTS
    #include "ZenLib/FileName.h"
#endif //MEDIAINFO_EVENTS
#if MEDIAINFO_IBI || MEDIAINFO_AES
    #include "ThirdParty/base64/base64.h"
#endif //MEDIAINFO_IBI || MEDIAINFO_AES
#include <algorithm>
#if MEDIAINFO_DEMUX
    #include <cmath>
#endif //MEDIAINFO_DEMUX
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Debug
#ifdef MEDIAINFO_DEBUG
    #include <stdio.h>
    #include <windows.h>
    namespace MediaInfo_Config_MediaInfo_Debug
    {
        FILE* F;
        std::string Debug;
        SYSTEMTIME st_In;

        void Debug_Open(bool Out)
        {
            F=fopen("C:\\Temp\\MediaInfo_Debug.txt", "a+t");
            Debug.clear();
            SYSTEMTIME st;
            GetLocalTime( &st );

            char Duration[100];
            if (Out)
            {
                FILETIME ft_In;
                if (SystemTimeToFileTime(&st_In, &ft_In))
                {
                    FILETIME ft_Out;
                    if (SystemTimeToFileTime(&st, &ft_Out))
                    {
                        ULARGE_INTEGER UI_In;
                        UI_In.HighPart=ft_In.dwHighDateTime;
                        UI_In.LowPart=ft_In.dwLowDateTime;

                        ULARGE_INTEGER UI_Out;
                        UI_Out.HighPart=ft_Out.dwHighDateTime;
                        UI_Out.LowPart=ft_Out.dwLowDateTime;

                        ULARGE_INTEGER UI_Diff;
                        UI_Diff.QuadPart=UI_Out.QuadPart-UI_In.QuadPart;

                        FILETIME ft_Diff;
                        ft_Diff.dwHighDateTime=UI_Diff.HighPart;
                        ft_Diff.dwLowDateTime=UI_Diff.LowPart;

                        SYSTEMTIME st_Diff;
                        if (FileTimeToSystemTime(&ft_Diff, &st_Diff))
                        {
                            sprintf(Duration, "%02hd:%02hd:%02hd.%03hd", st_Diff.wHour, st_Diff.wMinute, st_Diff.wSecond, st_Diff.wMilliseconds);
                        }
                        else
                            strcpy(Duration, "            ");
                    }
                    else
                        strcpy(Duration, "            ");

                }
                else
                    strcpy(Duration, "            ");
            }
            else
            {
                st_In=st;
                strcpy(Duration, "            ");
            }

            fprintf(F,"                                       %02hd:%02hd:%02hd.%03hd %s", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, Duration);
        }

        void Debug_Close()
        {
            Debug += "\r\n";
            fwrite(Debug.c_str(), Debug.size(), 1, F); \
            fclose(F);
        }
    }
    using namespace MediaInfo_Config_MediaInfo_Debug;

    #define MEDIAINFO_DEBUG1(_NAME,_TOAPPEND) \
        Debug_Open(false); \
        Debug+=", ";Debug+=_NAME; \
        _TOAPPEND; \
        Debug_Close();

    #define MEDIAINFO_DEBUG2(_NAME,_TOAPPEND) \
        Debug_Open(true); \
        Debug+=", ";Debug+=_NAME; \
        _TOAPPEND; \
        Debug_Close();
#else // MEDIAINFO_DEBUG
    #define MEDIAINFO_DEBUG1(_NAME,_TOAPPEND)
    #define MEDIAINFO_DEBUG2(_NAME,_TOAPPEND)
#endif // MEDIAINFO_DEBUG

namespace MediaInfoLib
{

const size_t Buffer_NormalSize=/*188*7;//*/64*1024;

//***************************************************************************
// Info
//***************************************************************************

const char* DisplayCaptions_Strings[] =
{
    "Command",
    "Content",
    "Stream",
};
static_assert(sizeof(DisplayCaptions_Strings) / sizeof(*DisplayCaptions_Strings) == DisplayCaptions_Max, "");

//***************************************************************************
// Class
//***************************************************************************

MediaInfo_Config_MediaInfo::MediaInfo_Config_MediaInfo()
{
    MediaInfoLib::Config.Init(); //Initialize Configuration

    RequestTerminate=false;
    FileIsSeekable=true;
    FileIsSub=false;
    FileIsDetectingDuration=false;
    FileIsReferenced=false;
    FileTestContinuousFileNames=true;
    FileTestDirectory=true;
    FileKeepInfo=false;
    FileStopAfterFilled=false;
    FileStopSubStreamAfterFilled=false;
    Audio_MergeMonoStreams=false;
    File_Demux_Interleave=false;
    File_ID_OnlyRoot=false;
    File_ExpandSubs_Backup=NULL;
    File_ExpandSubs_Source=NULL;
    #if MEDIAINFO_ADVANCED
        File_IgnoreSequenceFileSize=false;
        File_IgnoreSequenceFilesCount=false;
        File_SequenceFilesSkipFrames=0;
        File_DefaultFrameRate=0;
        File_DefaultTimeCodeDropFrame=(int8u)-1;
        File_Source_List=false;
        File_RiskyBitRateEstimation=false;
        File_MergeBitRateInfo=true;
        File_HighestFormat=true;
        File_ChannelLayout=true;
        File_FrameIsAlwaysComplete=false;
        #if MEDIAINFO_DEMUX
            File_Demux_Unpacketize_StreamLayoutChange_Skip=false;
        #endif //MEDIAINFO_DEMUX
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_MD5
        File_Md5=false;
    #endif //MEDIAINFO_MD5
    #if defined(MEDIAINFO_REFERENCES_YES)
        File_CheckSideCarFiles=false;
    #endif //defined(MEDIAINFO_REFERENCES_YES)
    File_TimeToLive=0;
    File_Buffer_Size_Hint_Pointer=NULL;
    File_Buffer_Read_Size=64*1024*1024;
    #if MEDIAINFO_AES
        Encryption_Format=Encryption_Format_None;
        Encryption_Method=Encryption_Method_None;
        Encryption_Mode=Encryption_Mode_None;
        Encryption_Padding=Encryption_Padding_None;
    #endif //MEDIAINFO_AES
    #if MEDIAINFO_NEXTPACKET
        NextPacket=false;
    #endif //MEDIAINFO_NEXTPACKET
    #if MEDIAINFO_FILTER
        File_Filter_Audio=false;
        File_Filter_HasChanged_=false;
    #endif //MEDIAINFO_FILTER
    #if MEDIAINFO_EVENTS
        Event_CallBackFunction=NULL;
        Event_UserHandler=NULL;
        SubFile_StreamID=(int64u)-1;
        ParseUndecodableFrames=false;
        Events_TimestampShift_Reference_PTS=(int64u)-1;
        Events_TimestampShift_Reference_ID=(int64u)-1;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_ForceIds=false;
        Demux_PCM_20bitTo16bit=false;
        Demux_PCM_20bitTo24bit=false;
        Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10=false;
        Demux_Hevc_Transcode_Iso14496_15_to_AnnexB=false;
        Demux_Unpacketize=false;
        Demux_SplitAudioBlocks=false;
        Demux_Rate=0;
        Demux_FirstDts=(int64u)-1;
        Demux_FirstFrameNumber=(int64u)-1;
        Demux_InitData=0; //In Demux event
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_IBIUSAGE
        Ibi_UseIbiInfoIfAvailable=false;
    #endif //MEDIAINFO_IBIUSAGE
    #if MEDIAINFO_IBIUSAGE
        Ibi_Create=false;
    #endif //MEDIAINFO_IBIUSAGE

    //Specific
    File_MpegTs_ForceMenu=false;
    File_MpegTs_stream_type_Trust=true;
    File_MpegTs_Atsc_transport_stream_id_Trust=true;
    File_MpegTs_RealTime=false;
    File_Mxf_TimeCodeFromMaterialPackage=false;
    File_Mxf_ParseIndex=false;
    File_Bdmv_ParseTargetedFile=true;
    #if defined(MEDIAINFO_DVDIF_YES)
    File_DvDif_DisableAudioIfIsInContainer=false;
    File_DvDif_IgnoreTransmittingFlags=false;
    #endif //defined(MEDIAINFO_DVDIF_YES)
    #if defined(MEDIAINFO_DVDIF_ANALYZE_YES)
        File_DvDif_Analysis=false;
    #endif //defined(MEDIAINFO_DVDIF_ANALYZE_YES)
    #if MEDIAINFO_MACROBLOCKS
        File_Macroblocks_Parse=0;
    #endif //MEDIAINFO_MACROBLOCKS
    File_GrowingFile_Delay=10;
    File_GrowingFile_Force=false;
    #if defined(MEDIAINFO_LIBMMS_YES)
        File_Mmsh_Describe_Only=false;
    #endif //defined(MEDIAINFO_LIBMMS_YES)
    DisplayCaptions=DisplayCaptions_Command;
    File_ProbeCaption_Set({});
    State=0;
    #if defined(MEDIAINFO_AC3_YES)
    File_Ac3_IgnoreCrc=false;
    #endif //defined(MEDIAINFO_AC3_YES)

    //Internal to MediaInfo, not thread safe
    File_Names_Pos=0;
    File_Buffer=NULL;
    File_Buffer_Size_Max=0;
    File_Buffer_Size_ToRead=Buffer_NormalSize;
    File_Buffer_Size=0;
    File_Buffer_Repeat=false;
    File_Buffer_Repeat_IsSupported=false;
    File_IsGrowing=false;
    File_IsNotGrowingAnymore=false;
    File_IsImageSequence=false;
    #if defined(MEDIAINFO_EIA608_YES)
    File_Scte20_IsPresent=false;
    #endif //defined(MEDIAINFO_EIA608_YES)
    #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
    File_DtvccTransport_Stream_IsPresent=false;
    File_DtvccTransport_Descriptor_IsPresent=false;
    #endif //defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
    #if defined(MEDIAINFO_MPEGPS_YES)
    File_MpegPs_PTS_Begin_IsNearZero=false;
    #endif //defined(MEDIAINFO_MPEGPS_YES)
    File_Current_Offset=0;
    File_Current_Size=(int64u)-1;
    File_IgnoreEditsBefore=0;
    File_IgnoreEditsAfter=(int64u)-1;
    File_EditRate=0;
    File_Size=(int64u)-1;
    ParseSpeed=MediaInfoLib::Config.ParseSpeed_Get();
    ParseSpeed_FromFile=false;
    IsFinishing=false;
    #if MEDIAINFO_EVENTS
        Config_PerPackage=NULL;
        Events_TimestampShift_Disabled=false;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_EventWasSent=false;
        Demux_Offset_Frame=(int64u)-1;
        Demux_Offset_DTS=(int64u)-1;
        Demux_Offset_DTS_FromStream=(int64u)-1;
        Events_Delayed_CurrentSource=NULL;
        #if MEDIAINFO_SEEK
           Demux_IsSeeking=false;
        #endif //MEDIAINFO_SEEK
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_SEEK
        File_GoTo_IsFrameOffset=false;
    #endif //MEDIAINFO_SEEK
    #if MEDIAINFO_FIXITY
        TryToFix=false;
    #endif //MEDIAINFO_SEEK
    #if MEDIAINFO_ADVANCED
        TimeCode_Dumps=nullptr;
    #endif //MEDIAINFO_ADVANCED
}

MediaInfo_Config_MediaInfo::~MediaInfo_Config_MediaInfo()
{
    delete[] File_Buffer; //File_Buffer=NULL;
    delete (std::vector<std::vector<ZtringListList> >*)File_ExpandSubs_Backup; //File_ExpandSubs_Backup=NULL

    #if MEDIAINFO_EVENTS
        for (events_delayed::iterator Event=Events_Delayed.begin(); Event!=Events_Delayed.end(); ++Event)
            for (size_t Pos=0; Pos<Event->second.size(); Pos++)
                delete Event->second[Pos]; //Event->second[Pos]=NULL;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_ADVANCED
        delete TimeCode_Dumps;
    #endif //MEDIAINFO_ADVANCED
}

//***************************************************************************
// Info
//***************************************************************************

Ztring MediaInfo_Config_MediaInfo::Option (const String &Option, const String &Value)
{
    #if MEDIAINFO_EVENTS
        SubFile_Config(Option)=Value;
    #endif //MEDIAINFO_EVENTS

    String Option_Lower(Option);
    size_t Egal_Pos=Option_Lower.find(__T('='));
    if (Egal_Pos==string::npos)
        Egal_Pos=Option_Lower.size();
    transform(Option_Lower.begin(), Option_Lower.begin()+Egal_Pos, Option_Lower.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix

    if (Option_Lower==__T("file_requestterminate"))
    {
        RequestTerminate=true;
        return Ztring();
    }
    if (Option_Lower==__T("file_isseekable"))
    {
        File_IsSeekable_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_isseekable_get"))
    {
        return File_IsSeekable_Get()?"1":"0";
    }
    if (Option_Lower==__T("file_issub"))
    {
        File_IsSub_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_issub_get"))
    {
        return File_IsSub_Get()?"1":"0";
    }
    if (Option_Lower==__T("file_parsespeed"))
    {
        File_ParseSpeed_Set(Ztring(Value).To_float32());
        return __T("");
    }
    else if (Option_Lower==__T("file_parsespeed_get"))
    {
        return Ztring::ToZtring(File_ParseSpeed_Get(), 1);
    }
    if (Option_Lower==__T("file_isdetectingduration"))
    {
        File_IsDetectingDuration_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_isdetectingduration_get"))
    {
        return File_IsDetectingDuration_Get()?"1":"0";
    }
    if (Option_Lower==__T("file_isreferenced"))
    {
        File_IsReferenced_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_isreferenced_get"))
    {
        return File_IsReferenced_Get()?"1":"0";
    }
    if (Option_Lower==__T("file_testcontinuousfilenames"))
    {
        File_TestContinuousFileNames_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_testcontinuousfilenames_get"))
    {
        return File_TestContinuousFileNames_Get()?"1":"0";
    }
    if (Option_Lower==__T("file_testdirectory"))
    {
        File_TestDirectory_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_testdirectory_get"))
    {
        return File_TestDirectory_Get()?"1":"0";
    }
    if (Option_Lower==__T("file_keepinfo"))
    {
        File_KeepInfo_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_keepinfo_get"))
    {
        return File_KeepInfo_Get()?"1":"0";
    }
    if (Option_Lower==__T("file_stopafterfilled"))
    {
        File_StopAfterFilled_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_stopafterfilled_get"))
    {
        return File_StopAfterFilled_Get()?"1":"0";
    }
    if (Option_Lower==__T("file_stopsubstreamafterfilled"))
    {
        File_StopSubStreamAfterFilled_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_stopsubstreamafterfilled_get"))
    {
        return File_StopSubStreamAfterFilled_Get()?"1":"0";
    }
    if (Option_Lower==__T("file_audio_mergemonostreams"))
    {
        File_Audio_MergeMonoStreams_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_audio_mergemonostreams_get"))
    {
        return File_Audio_MergeMonoStreams_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_demux_interleave"))
    {
        File_Demux_Interleave_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_demux_interleave_get"))
    {
        return File_Demux_Interleave_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_id_onlyroot"))
    {
        File_ID_OnlyRoot_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_expandsubs"))
    {
        File_ExpandSubs_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_id_onlyroot_get"))
    {
        return File_ID_OnlyRoot_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_ignoresequencefilesize"))
    {
        #if MEDIAINFO_ADVANCED
            File_IgnoreSequenceFileSize_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("Disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_ignoresequencefilescount"))
    {
        #if MEDIAINFO_ADVANCED
            File_IgnoreSequenceFilesCount_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("Disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_sequencefilesskipframes") || Option_Lower==__T("file_sequencefileskipframes"))
    {
        #if MEDIAINFO_ADVANCED
            File_SequenceFilesSkipFrames_Set(Ztring(Value).To_int64u());
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("Disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_defaultframerate"))
    {
        #if MEDIAINFO_ADVANCED
            File_DefaultFrameRate_Set(Ztring(Value).To_float64());
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("File_DefaultFrameRate is disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_defaulttimecode"))
    {
        #if MEDIAINFO_ADVANCED
            File_DefaultTimeCode_Set(Ztring(Value).To_UTF8());
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("File_DefaultTimeCode is disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_defaulttimecodedropframe"))
    {
        #if MEDIAINFO_ADVANCED
            return File_DefaultTimeCodeDropFrame_Set(Value);
        #else //MEDIAINFO_ADVANCED
            return __T("File_DefaultTimeCodeDropFrame is disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_source_list"))
    {
        #if MEDIAINFO_ADVANCED
            File_Source_List_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("File_Source_List_Set is disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_riskybitrateestimation"))
    {
        #if MEDIAINFO_ADVANCED
            File_RiskyBitRateEstimation_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("Advanced features are disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_mergebitrateinfo"))
    {
        #if MEDIAINFO_ADVANCED
            File_MergeBitRateInfo_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("Advanced features are disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_highestformat"))
    {
        #if MEDIAINFO_ADVANCED
            File_HighestFormat_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("Advanced features are disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_channellayout"))
    {
        #if MEDIAINFO_ADVANCED
            File_ChannelLayout_Set(Value==__T("2018"));
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("Advanced features are disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_frameisalwayscomplete"))
    {
        #if MEDIAINFO_ADVANCED
            File_FrameIsAlwaysComplete_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_ADVANCED
            return __T("Advanced features are disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_demux_unpacketize_streamlayoutchange_skip"))
    {
        #if MEDIAINFO_DEMUX
            #if MEDIAINFO_ADVANCED
                File_Demux_Unpacketize_StreamLayoutChange_Skip_Set(!(Value==__T("0") || Value.empty()));
                return Ztring();
            #else //MEDIAINFO_ADVANCED
                return __T("Advanced features disabled due to compilation options");
            #endif //MEDIAINFO_ADVANCED
        #else //MEDIAINFO_ADVANCED
            return __T("Advanced features disabled due to compilation options");
        #endif //MEDIAINFO_ADVANCED
    }
    else if (Option_Lower==__T("file_md5"))
    {
        #if MEDIAINFO_MD5
            File_Md5_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_MD5
            return __T("MD5 is disabled due to compilation options");
        #endif //MEDIAINFO_MD5
    }
    else if (Option_Lower==__T("file_hash"))
    {
        #if MEDIAINFO_HASH
            ZtringList List;
            List.Separator_Set(0, __T(","));
            List.Write(Value);
            ZtringList ToReturn;
            ToReturn.Separator_Set(0, __T(","));
            HashWrapper::HashFunctions Functions;
            for (size_t i=0; i<List.size(); ++i)
            {
                Ztring Value(List[i]);
                Value.MakeLowerCase();
                bool Found=false;
                for (size_t j=0; j<HashWrapper::HashFunction_Max; ++j)
                {
                    Ztring Name;
                    Name.From_UTF8(HashWrapper::Name((HashWrapper::HashFunction)j));
                    Name.MakeLowerCase();
                    if (Value==Name)
                    {
                        Functions.set(j);
                        Found=true;
                    }
                }

                if (!Found)
                    ToReturn.push_back(List[i]);
            }
            File_Hash_Set(Functions);
            if (ToReturn.empty())
                return Ztring();
            else
                return ToReturn.Read()+__T(" hash functions are unknown or disabled due to compilation options");
        #else //MEDIAINFO_HASH
            return __T("Hash functions are disabled due to compilation options");
        #endif //MEDIAINFO_HASH
    }
    else if (Option_Lower==__T("file_hash_get"))
    {
        #if MEDIAINFO_HASH
            ZtringList List;
            HashWrapper::HashFunctions Functions=File_Hash_Get();
            for (size_t i=0; i<HashWrapper::HashFunction_Max; ++i)
                if (Functions[i])
                    List.push_back(Ztring().From_UTF8(HashWrapper::Name((HashWrapper::HashFunction)i)));
            List.Separator_Set(0, __T(","));
            return List.Read();
        #else //MEDIAINFO_HASH
            return __T("Hash functions are disabled due to compilation options");
        #endif //MEDIAINFO_HASH
    }
    else if (Option_Lower==__T("file_checksidecarfiles"))
    {
        #if defined(MEDIAINFO_REFERENCES_YES)
            File_CheckSideCarFiles_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //defined(MEDIAINFO_REFERENCES_YES)
            return __T("Disabled due to compilation options");
        #endif //defined(MEDIAINFO_REFERENCES_YES)
    }
    else if (Option_Lower==__T("file_filename"))
    {
        File_FileName_Set(Value);
        return __T("");
    }
    else if (Option_Lower==__T("file_filename_get"))
    {
        return File_FileName_Get();
    }
    else if (Option_Lower==__T("file_filenameformat"))
    {
        File_FileNameFormat_Set(Value);
        return __T("");
    }
    else if (Option_Lower==__T("file_filenameformat_get"))
    {
        return File_FileNameFormat_Get();
    }
    else if (Option_Lower==__T("file_timetolive"))
    {
        File_TimeToLive_Set(Ztring(Value).To_float64());
        return __T("");
    }
    else if (Option_Lower==__T("file_timetolive_get"))
    {
        return Ztring::ToZtring(File_TimeToLive_Get(), 9);
    }
    else if (Option_Lower==__T("file_partial_begin"))
    {
        File_Partial_Begin_Set(Value);
        return __T("");
    }
    else if (Option_Lower==__T("file_partial_begin_get"))
    {
        return File_Partial_Begin_Get();
    }
    else if (Option_Lower==__T("file_partial_end"))
    {
        File_Partial_End_Set(Value);
        return __T("");
    }
    else if (Option_Lower==__T("file_partial_end_get"))
    {
        return File_Partial_End_Get();
    }
    else if (Option_Lower==__T("file_forceparser"))
    {
        File_ForceParser_Set(Value);
        return __T("");
    }
    else if (Option_Lower==__T("file_forceparser_get"))
    {
        return File_ForceParser_Get();
    }
    else if (Option_Lower==__T("file_forceparser_config"))
    {
        File_ForceParser_Config_Set(Value);
        return __T("");
    }
    else if (Option_Lower==__T("file_buffer_size_hint_pointer"))
    {
        File_Buffer_Size_Hint_Pointer_Set((size_t*)Ztring(Value).To_int64u());
        return __T("");
    }
    else if (Option_Lower==__T("file_buffer_size_hint_pointer_get"))
    {
        return Ztring::ToZtring((size_t)File_Buffer_Size_Hint_Pointer_Get());
    }
    else if (Option_Lower==__T("file_buffer_read_size"))
    {
        File_Buffer_Read_Size_Set((size_t)Ztring(Value).To_int64u());
        return __T("");
    }
    else if (Option_Lower==__T("file_buffer_read_size_get"))
    {
        return Ztring::ToZtring((size_t)File_Buffer_Read_Size_Get());
    }
    else if (Option_Lower==__T("file_filter"))
    {
        #if MEDIAINFO_FILTER
            Ztring ValueLowerCase=Ztring(Value).MakeLowerCase();
            if (ValueLowerCase==__T("audio"))
                File_Filter_Audio_Set(true);
            else
                File_Filter_Set(ValueLowerCase.To_int64u());
            return Ztring();
        #else //MEDIAINFO_FILTER
            return __T("Filter manager is disabled due to compilation options");
        #endif //MEDIAINFO_FILTER
    }
    else if (Option_Lower==__T("file_filter_get"))
    {
        #if MEDIAINFO_FILTER
            return Ztring();//.From_Number(File_Filter_Get());
        #else //MEDIAINFO_FILTER
            return __T("Filter manager is disabled due to compilation options");
        #endif //MEDIAINFO_FILTER
    }
    else if (Option_Lower==__T("file_duplicate"))
    {
        #if MEDIAINFO_DUPLICATE
            return File_Duplicate_Set(Value);
        #else //MEDIAINFO_DUPLICATE
            return __T("Duplicate manager is disabled due to compilation options");
        #endif //MEDIAINFO_DUPLICATE
    }
    else if (Option_Lower==__T("file_duplicate_get"))
    {
        #if MEDIAINFO_DUPLICATE
            //if (File_Duplicate_Get())
                return __T("1");
            //else
            //    return __T("");
        #else //MEDIAINFO_DUPLICATE
            return __T("Duplicate manager is disabled due to compilation options");
        #endif //MEDIAINFO_DUPLICATE
    }
    else if (Option_Lower==__T("file_demux_forceids"))
    {
        #if MEDIAINFO_DEMUX
            if (Ztring(Value).To_int64u()==0)
                Demux_ForceIds_Set(false);
            else
                Demux_ForceIds_Set(true);
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_pcm_20bitto16bit"))
    {
        #if MEDIAINFO_DEMUX
            if (Ztring(Value).To_int64u()==0)
                Demux_PCM_20bitTo16bit_Set(false);
            else
                Demux_PCM_20bitTo16bit_Set(true);
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_pcm_20bitto24bit"))
    {
        #if MEDIAINFO_DEMUX
            if (Ztring(Value).To_int64u()==0)
                Demux_PCM_20bitTo24bit_Set(false);
            else
                Demux_PCM_20bitTo24bit_Set(true);
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_avc_transcode_iso14496_15_to_iso14496_10"))
    {
        #if MEDIAINFO_DEMUX
            if (Ztring(Value).To_int64u()==0)
                Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Set(false);
            else
                Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Set(true);
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_hevc_transcode_iso14496_15_to_annexb"))
    {
        #if MEDIAINFO_DEMUX
            if (Ztring(Value).To_int64u()==0)
                Demux_Hevc_Transcode_Iso14496_15_to_AnnexB_Set(false);
            else
                Demux_Hevc_Transcode_Iso14496_15_to_AnnexB_Set(true);
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_unpacketize"))
    {
        #if MEDIAINFO_DEMUX
            if (Ztring(Value).To_int64u()==0)
                Demux_Unpacketize_Set(false);
            else
                Demux_Unpacketize_Set(true);
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_splitaudioblocks"))
    {
        #if MEDIAINFO_DEMUX
            Demux_SplitAudioBlocks_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_rate"))
    {
        #if 1 //MEDIAINFO_DEMUX
            Demux_Rate_Set(Ztring(Value).To_float64());
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_firstdts"))
    {
        #if MEDIAINFO_DEMUX
            int64u ValueInt64u;
            if (Value.find(__T(':'))!=string::npos)
            {
                Ztring ValueZ=Value;
                ValueInt64u=0;
                size_t Value_Pos=ValueZ.find(__T(':'));
                if (Value_Pos==string::npos)
                    Value_Pos=ValueZ.size();
                ValueInt64u+=Ztring(ValueZ.substr(0, Value_Pos)).To_int64u()*60*60*1000*1000*1000;
                ValueZ.erase(0, Value_Pos+1);
                Value_Pos=ValueZ.find(__T(':'));
                if (Value_Pos==string::npos)
                    Value_Pos=ValueZ.size();
                ValueInt64u+=Ztring(ValueZ.substr(0, Value_Pos)).To_int64u()*60*1000*1000*1000;
                ValueZ.erase(0, Value_Pos+1);
                Value_Pos=ValueZ.find(__T('.'));
                if (Value_Pos==string::npos)
                    Value_Pos=ValueZ.size();
                ValueInt64u+=Ztring(ValueZ.substr(0, Value_Pos)).To_int64u()*1000*1000*1000;
                ValueZ.erase(0, Value_Pos+1);
                if (!ValueZ.empty())
                    ValueInt64u+=Ztring(ValueZ).To_int64u()*1000*1000*1000/(int64u)pow(10.0, (int)ValueZ.size());
            }
            else
                ValueInt64u=Ztring(Value).To_int64u();
            Demux_FirstDts_Set(ValueInt64u);
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_firstframenumber"))
    {
        #if MEDIAINFO_DEMUX
            Demux_FirstFrameNumber_Set(Ztring(Value).To_int64u());
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_demux_initdata"))
    {
        #if MEDIAINFO_DEMUX
            Ztring Value_Lower(Value); Value_Lower.MakeLowerCase();
                 if (Value_Lower==__T("event")) Demux_InitData_Set(0);
            else if (Value_Lower==__T("field")) Demux_InitData_Set(1);
            else return __T("Invalid value");
            return Ztring();
        #else //MEDIAINFO_DEMUX
            return __T("Demux manager is disabled due to compilation options");
        #endif //MEDIAINFO_DEMUX
    }
    else if (Option_Lower==__T("file_ibi"))
    {
        #if MEDIAINFO_IBIUSAGE
            Ibi_Set(Value);
            return Ztring();
        #else //MEDIAINFO_IBIUSAGE
            return __T("IBI support is disabled due to compilation options");
        #endif //MEDIAINFO_IBIUSAGE
    }
    else if (Option_Lower==__T("file_ibi_create"))
    {
        #if MEDIAINFO_IBIUSAGE
            if (Ztring(Value).To_int64u()==0)
                Ibi_Create_Set(false);
            else
                Ibi_Create_Set(true);
            return Ztring();
        #else //MEDIAINFO_IBIUSAGE
            return __T("IBI support is disabled due to compilation options");
        #endif //MEDIAINFO_IBIUSAGE
    }
    else if (Option_Lower==__T("file_ibi_useibiinfoifavailable"))
    {
        #if MEDIAINFO_IBIUSAGE
            if (Ztring(Value).To_int64u()==0)
                Ibi_UseIbiInfoIfAvailable_Set(false);
            else
                Ibi_UseIbiInfoIfAvailable_Set(true);
            return Ztring();
        #else //MEDIAINFO_IBIUSAGE
            return __T("IBI support is disabled due to compilation options");
        #endif //MEDIAINFO_IBIUSAGE
    }
    else if (Option_Lower==__T("file_encryption_format"))
    {
        #if MEDIAINFO_AES
            Encryption_Format_Set(Value);
            return Ztring();
        #else //MEDIAINFO_AES
            return __T("Encryption manager is disabled due to compilation options");
        #endif //MEDIAINFO_AES
    }
    else if (Option_Lower==__T("file_encryption_key"))
    {
        #if MEDIAINFO_AES
            Encryption_Key_Set(Value);
            return Ztring();
        #else //MEDIAINFO_AES
            return __T("Encryption manager is disabled due to compilation options");
        #endif //MEDIAINFO_AES
    }
    else if (Option_Lower==__T("file_encryption_method"))
    {
        #if MEDIAINFO_AES
            Encryption_Method_Set(Value);
            return Ztring();
        #else //MEDIAINFO_AES
            return __T("Encryption manager is disabled due to compilation options");
        #endif //MEDIAINFO_AES
    }
    else if (Option_Lower==__T("file_encryption_mode") || Option_Lower==__T("file_encryption_modeofoperation"))
    {
        #if MEDIAINFO_AES
            Encryption_Mode_Set(Value);
            return Ztring();
        #else //MEDIAINFO_AES
            return __T("Encryption manager is disabled due to compilation options");
        #endif //MEDIAINFO_AES
    }
    else if (Option_Lower==__T("file_encryption_padding"))
    {
        #if MEDIAINFO_AES
            Encryption_Padding_Set(Value);
            return Ztring();
        #else //MEDIAINFO_AES
            return __T("Encryption manager is disabled due to compilation options");
        #endif //MEDIAINFO_AES
    }
    else if (Option_Lower==__T("file_encryption_initializationvector"))
    {
        #if MEDIAINFO_AES
            Encryption_InitializationVector_Set(Value);
            return Ztring();
        #else //MEDIAINFO_AES
            return __T("Encryption manager is disabled due to compilation options");
        #endif //MEDIAINFO_AES
    }
    else if (Option_Lower==__T("file_nextpacket"))
    {
        #if MEDIAINFO_NEXTPACKET
            if (Ztring(Value).To_int64u()==0)
                NextPacket_Set(false);
            else
                NextPacket_Set(true);
            return Ztring();
        #else //MEDIAINFO_NEXTPACKET
            return __T("NextPacket manager is disabled due to compilation options");
        #endif //MEDIAINFO_NEXTPACKET
    }
    else if (Option_Lower==__T("file_subfile_streamid_set"))
    {
        #if MEDIAINFO_EVENTS
            SubFile_StreamID_Set(Value.empty()?(int64u)-1:Ztring(Value).To_int64u());
            return Ztring();
        #else //MEDIAINFO_EVENTS
            return __T("Event manager is disabled due to compilation options");
        #endif //MEDIAINFO_EVENTS
    }
    else if (Option_Lower==__T("file_subfile_ids_set"))
    {
        #if MEDIAINFO_EVENTS
            SubFile_IDs_Set(Value);
            return Ztring();
        #else //MEDIAINFO_EVENTS
            return __T("Event manager is disabled due to compilation options");
        #endif //MEDIAINFO_EVENTS
    }
    else if (Option_Lower==__T("file_parseundecodableframes"))
    {
        #if MEDIAINFO_EVENTS
            ParseUndecodableFrames_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_EVENTS
            return __T("Event manager is disabled due to compilation options");
        #endif //MEDIAINFO_EVENTS
    }
    else if (Option_Lower==__T("file_mpegts_forcemenu"))
    {
        File_MpegTs_ForceMenu_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_mpegts_forcemenu_get"))
    {
        return File_MpegTs_ForceMenu_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_mpegts_stream_type_trust"))
    {
        File_MpegTs_stream_type_Trust_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_mpegts_stream_type_trust_get"))
    {
        return File_MpegTs_stream_type_Trust_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_mpegts_atsc_transport_stream_id_trust"))
    {
        File_MpegTs_Atsc_transport_stream_id_Trust_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_mpegts_atsc_transport_stream_id_trust_get"))
    {
        return File_MpegTs_Atsc_transport_stream_id_Trust_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_mpegts_realtime"))
    {
        File_MpegTs_RealTime_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_mpegts_realtime_get"))
    {
        return File_MpegTs_RealTime_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_mxf_timecodefrommaterialpackage"))
    {
        File_Mxf_TimeCodeFromMaterialPackage_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_mxf_timecodefrommaterialpackage_get"))
    {
        return File_Mxf_TimeCodeFromMaterialPackage_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_mxf_parseindex"))
    {
        File_Mxf_ParseIndex_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_mxf_parseindex_get"))
    {
        return File_Mxf_ParseIndex_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_bdmv_parsetargetedfile"))
    {
        File_Bdmv_ParseTargetedFile_Set(!(Value==__T("0") || Value.empty()));
        return __T("");
    }
    else if (Option_Lower==__T("file_bdmv_parsetargetedfile_get"))
    {
        return File_Bdmv_ParseTargetedFile_Get()?"1":"0";
    }
    else if (Option_Lower==__T("file_dvdif_disableaudioifisincontainer"))
    {
        #if defined(MEDIAINFO_DVDIF_YES)
            File_DvDif_DisableAudioIfIsInContainer_Set(!(Value==__T("0") || Value.empty()));
            return __T("");
        #else //defined(MEDIAINFO_DVDIF_YES)
            return __T("DVDIF is disabled due to compilation options");
        #endif //defined(MEDIAINFO_DVDIF_YES)
    }
    else if (Option_Lower==__T("file_dvdif_disableaudioifisincontainer_get"))
    {
        #if defined(MEDIAINFO_DVDIF_YES)
            return File_DvDif_DisableAudioIfIsInContainer_Get()?"1":"0";
        #else //defined(MEDIAINFO_DVDIF_YES)
            return __T("DVDIF is disabled due to compilation options");
        #endif //defined(MEDIAINFO_DVDIF_YES)
    }
    else if (Option_Lower==__T("file_dvdif_ignoretransmittingflags"))
    {
        #if defined(MEDIAINFO_DVDIF_YES)
            File_DvDif_IgnoreTransmittingFlags_Set(!(Value==__T("0") || Value.empty()));
            return __T("");
        #else //defined(MEDIAINFO_DVDIF_YES)
            return __T("DVDIF is disabled due to compilation options");
        #endif //defined(MEDIAINFO_DVDIF_YES)
    }
    else if (Option_Lower==__T("file_dvdif_ignoretransmittingflags_get"))
    {
        #if defined(MEDIAINFO_DVDIF_YES)
            return File_DvDif_IgnoreTransmittingFlags_Get()?"1":"0";
        #else //defined(MEDIAINFO_DVDIF_YES)
            return __T("DVDIF is disabled due to compilation options");
        #endif //defined(MEDIAINFO_DVDIF_YES)
    }
    else if (Option_Lower==__T("file_dvdif_analysis"))
    {
        #if defined(MEDIAINFO_DVDIF_ANALYZE_YES)
            File_DvDif_Analysis_Set(!(Value==__T("0") || Value.empty()));
            return __T("");
        #else //defined(MEDIAINFO_DVDIF_ANALYZE_YES)
            return __T("DVDIF Analysis is disabled due to compilation options");
        #endif //defined(MEDIAINFO_DVDIF_ANALYZE_YES)
    }
    else if (Option_Lower==__T("file_dvdif_analysis_get"))
    {
        #if defined(MEDIAINFO_DVDIF_ANALYZE_YES)
            return File_DvDif_Analysis_Get()?"1":"0";
        #else //defined(MEDIAINFO_DVDIF_ANALYZE_YES)
            return __T("DVDIF Analysis is disabled due to compilation options");
        #endif //defined(MEDIAINFO_DVDIF_ANALYZE_YES)
    }
    else if (Option_Lower==__T("file_macroblocks_parse"))
    {
        #if MEDIAINFO_MACROBLOCKS
            File_Macroblocks_Parse_Set(Ztring(Value).To_int32s());
            return __T("");
        #else //MEDIAINFO_MACROBLOCKS
            return __T("Macroblock parsing is disabled due to compilation options");
        #endif //MEDIAINFO_MACROBLOCKS
    }
    else if (Option_Lower==__T("file_macroblocks_parse_get"))
    {
        #if MEDIAINFO_MACROBLOCKS
            return Ztring::ToZtring(File_Macroblocks_Parse_Get());
        #else //MEDIAINFO_MACROBLOCKS
            return __T("Macroblock parsing is disabled due to compilation options");
        #endif //MEDIAINFO_MACROBLOCKS
    }
    else if (Option_Lower==__T("file_growingfile_delay"))
    {
        File_GrowingFile_Delay_Set(Ztring(Value).To_float64());
        return Ztring();
    }
    else if (Option_Lower==__T("file_growingfile_delay_get"))
    {
        return Ztring::ToZtring(File_GrowingFile_Delay_Get());
    }
    else if (Option_Lower==__T("file_growingfile_force"))
    {
        File_GrowingFile_Force_Set(Ztring(Value).To_float64());
        return Ztring();
    }
    else if (Option_Lower==__T("file_curl"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            File_Curl_Set(Value);
            return __T("");
        #else //defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif //defined(MEDIAINFO_LIBCURL_YES)
    }
    else if (Option_Lower==__T("file_trytofix"))
    {
        #if MEDIAINFO_FIXITY
            TryToFix_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_FIXITY
            return __T("Fixity support is disabled due to compilation options");
        #endif //MEDIAINFO_FIXITY
    }
    else if (Option_Lower.find(__T("file_curl,"))==0 || Option_Lower.find(__T("file_curl;"))==0)
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            File_Curl_Set(Option.substr(10), Value);
            return __T("");
        #else //defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif //defined(MEDIAINFO_LIBCURL_YES)
    }
    else if (Option_Lower==__T("file_curl_get"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            return File_Curl_Get(Value);
        #else //defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif //defined(MEDIAINFO_LIBCURL_YES)
    }
    else if (Option_Lower==__T("file_mmsh_describe_only"))
    {
        #if defined(MEDIAINFO_LIBMMS_YES)
            File_Mmsh_Describe_Only_Set(!(Value==__T("0") || Value.empty()));
            return __T("");
        #else //defined(MEDIAINFO_LIBMMS_YES)
            return __T("Libmms support is disabled due to compilation options");
        #endif //defined(MEDIAINFO_LIBMMS_YES)
    }
    else if (Option_Lower==__T("file_mmsh_describe_only_get"))
    {
        #if defined(MEDIAINFO_LIBMMS_YES)
            return File_Mmsh_Describe_Only_Get()?"1":"0";
        #else //defined(MEDIAINFO_LIBMMS_YES)
            return __T("Libmms support is disabled due to compilation options");
        #endif //defined(MEDIAINFO_LIBMMS_YES)
    }
    else if (Option_Lower==__T("file_displaycaptions"))
    {
        return File_DisplayCaptions_Set(Value);
    }
    else if (Option_Lower==__T("file_eia708_displayemptystream"))
    {
        return File_DisplayCaptions_Set(Ztring().From_UTF8(DisplayCaptions_Strings[Value==__T("0")?DisplayCaptions_Command:DisplayCaptions_Stream]));
    }
    else if (Option_Lower==__T("file_eia608_displayemptystream"))
    {
        return File_DisplayCaptions_Set(Ztring().From_UTF8(DisplayCaptions_Strings[Value==__T("0")?DisplayCaptions_Command:DisplayCaptions_Stream]));
    }
    else if (Option_Lower==__T("file_commandonlymeansempty"))
    {
        return File_DisplayCaptions_Set(Ztring().From_UTF8(DisplayCaptions_Strings[Value==__T("0")?DisplayCaptions_Command:DisplayCaptions_Content]));
        return __T("");
    }
    else if (Option_Lower==__T("file_probecaption"))
    {
        File_ProbeCaption_Set(Value);
        return __T("");
    }
    else if (Option_Lower==__T("file_probecaption_get"))
    {
        return __T("(Not supported)");
    }
    else if (Option_Lower==__T("file_ac3_ignorecrc"))
    {
        #if defined(MEDIAINFO_AC3_YES)
            File_Ac3_IgnoreCrc_Set(!(Value==__T("0") || Value.empty()));
            return __T("");
        #else //defined(MEDIAINFO_AC3_YES)
            return __T("AC-3 support is disabled due to compilation options");
        #endif //defined(MEDIAINFO_AC3_YES)
    }
    else if (Option_Lower==__T("file_ac3_ignorecrc_get"))
    {
        #if defined(MEDIAINFO_AC3_YES)
            return File_Ac3_IgnoreCrc_Get()?"1":"0";
        #else //defined(MEDIAINFO_AC3_YES)
            return __T("AC-3 support is disabled due to compilation options");
        #endif //defined(MEDIAINFO_AC3_YES)
    }
    else if (Option_Lower==__T("file_event_callbackfunction"))
    {
        #if MEDIAINFO_EVENTS
            return Event_CallBackFunction_Set(Value);
        #else //MEDIAINFO_EVENTS
            return __T("Event manager is disabled due to compilation options");
        #endif //MEDIAINFO_EVENTS
    }
    else
        return __T("Option not known");
}

//***************************************************************************
// File Is Seekable
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_IsSeekable_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    FileIsSeekable=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_IsSeekable_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FileIsSeekable;
}

//***************************************************************************
// File Is Sub
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_IsSub_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    FileIsSub=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_IsSub_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FileIsSub;
}

//***************************************************************************
// File Parse Speed
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_ParseSpeed_Set (float32 NewValue, bool FromGlobal)
{
    CriticalSectionLocker CSL(CS);
    if (ParseSpeed_FromFile && FromGlobal)
        return; //Priority is file settings
    ParseSpeed=NewValue;
    ParseSpeed_FromFile=!FromGlobal;
}

float32 MediaInfo_Config_MediaInfo::File_ParseSpeed_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ParseSpeed;
}

//***************************************************************************
// File Is Detecting Duration
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_IsDetectingDuration_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    FileIsDetectingDuration=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_IsDetectingDuration_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FileIsDetectingDuration;
}

//***************************************************************************
// File Is Referenced
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_IsReferenced_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    FileIsReferenced=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_IsReferenced_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FileIsReferenced;
}

//***************************************************************************
// File Keep Info
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_KeepInfo_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    FileKeepInfo=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_KeepInfo_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FileKeepInfo;
}

//***************************************************************************
// File test continuous file names
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_TestContinuousFileNames_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    FileTestContinuousFileNames=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_TestContinuousFileNames_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FileTestContinuousFileNames;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_TestDirectory_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    FileTestDirectory=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_TestDirectory_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FileTestDirectory;
}

//***************************************************************************
// Stop after filled
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_StopAfterFilled_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    FileStopAfterFilled=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_StopAfterFilled_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FileStopAfterFilled;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_StopSubStreamAfterFilled_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    FileStopSubStreamAfterFilled=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_StopSubStreamAfterFilled_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FileStopSubStreamAfterFilled;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_Audio_MergeMonoStreams_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Audio_MergeMonoStreams=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Audio_MergeMonoStreams_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Audio_MergeMonoStreams;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_Demux_Interleave_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Demux_Interleave=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Demux_Interleave_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Demux_Interleave;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_ID_OnlyRoot_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_ID_OnlyRoot=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_ID_OnlyRoot_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_ID_OnlyRoot;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_ExpandSubs_Set (bool NewValue)
{
    { //for CSL
        CriticalSectionLocker CSL(CS);
        if ((File_ExpandSubs_Backup && NewValue) || (!File_ExpandSubs_Backup && !NewValue))
            return; //No change
        if (File_ExpandSubs_Backup)
        {
            //We want the default
            if (File_ExpandSubs_Source)
            {
                //Config has the backup, Source has the expanded one
                std::vector<std::vector<ZtringListList> >* Stream_More=(std::vector<std::vector<ZtringListList> >*)File_ExpandSubs_Source;
                std::vector<std::vector<ZtringListList> >* Backup=(std::vector<std::vector<ZtringListList> >*)File_ExpandSubs_Backup;
                *Stream_More=*Backup;
                Backup->clear();
            }
            delete (std::vector<std::vector<ZtringListList> > *)File_ExpandSubs_Backup;
            File_ExpandSubs_Backup=NULL;
        }
        else
        {
            //We want the expanded subs
            File_ExpandSubs_Backup=new std::vector<std::vector<ZtringListList> >;
        }
    }

    File_ExpandSubs_Update(NULL);
}

bool MediaInfo_Config_MediaInfo::File_ExpandSubs_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_ExpandSubs_Backup?true:false;
}

void MediaInfo_Config_MediaInfo::File_ExpandSubs_Update(void** Source)
{
    CriticalSectionLocker CSL(CS);
    if (Source)
        File_ExpandSubs_Source=*Source;
    if (!File_ExpandSubs_Source)
        return;
    std::vector<std::vector<ZtringListList> >* Stream_More=(std::vector<std::vector<ZtringListList> >*)File_ExpandSubs_Source;
    std::vector<std::vector<ZtringListList> >* Backup=(std::vector<std::vector<ZtringListList> >*)File_ExpandSubs_Backup;

    //Reset if needed
    if (File_ExpandSubs_Backup && !Backup->empty())
    {
        *Stream_More=*Backup;
        Backup->clear();
    }
    
    if (File_ExpandSubs_Backup)
    {
        //Backup
        *Backup=*Stream_More;

        //Sub-elements
        //for (set<string>::iterator File_ExpandSubs_Item=File_ExpandSubs_Items.begin(); File_ExpandSubs_Item!=File_ExpandSubs_Items.end(); ++File_ExpandSubs_Item)
        for (size_t i=0; i<6; i++)
        {
            //const Ztring Sub=Ztring().From_UTF8(*File_ExpandSubs_Item);
            for (size_t StreamKind=Stream_General; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<(*Stream_More)[StreamKind].size(); StreamPos++)
                {
                    if (i && (*Stream_More)[StreamKind][StreamPos].size()>4*1024)
                        break; // Too big, arbitrary stop

                    ZtringListList Temp;
                    size_t Pos_Max=(*Stream_More)[StreamKind][StreamPos].size();
                    for (size_t Pos=0; Pos<Pos_Max; Pos++)
                        if ((*Stream_More)[StreamKind][StreamPos][Pos].size()>Info_Name_Text)
                        {
                            Ztring& Name=(*Stream_More)[StreamKind][StreamPos][Pos][Info_Name];

                            // Tree
                            size_t Up_Pos=Name.find(__T(" LinkedTo_"));
                            size_t Up_Pos_End=string::npos;
                            if (Up_Pos!=string::npos)
                                Up_Pos_End=Name.find(__T("_Pos"), Name.size()-4);
                            if (Up_Pos!=string::npos && Up_Pos_End==Name.size()-4)
                            {
                                Ztring Up=Name.substr(Up_Pos);
                                auto IsComplementary=Up.FindAndReplace(__T("Complementary"), Ztring());

                                //Hide
                                Ztring ToHide=Name+__T("/String");
                                for (size_t j=Pos+1; j<(*Stream_More)[StreamKind][StreamPos].size(); j++)
                                {
                                    if ((*Stream_More)[StreamKind][StreamPos][j][Info_Name]==ToHide)
                                    {
                                        ZtringList& ToHideList=(*Stream_More)[StreamKind][StreamPos][j];
                                        ToHideList[Info_Options]=__T("N NT");
                                        break;
                                    }
                                }

                                //Expand
                                size_t SpacesCount=1;
                                size_t SpacesTestPos=Name.size()-Up.size();
                                while (SpacesTestPos && (SpacesTestPos=Name.rfind(__T(' '), SpacesTestPos-1))!=string::npos)
                                    SpacesCount++;
                                ZtringList L;
                                L.Separator_Set(0, __T(" + "));
                                L.Write((*Stream_More)[StreamKind][StreamPos][Pos][Info_Text]);
                                for (size_t i=0; i<L.size(); i++)
                                {
                                    Ztring ID=L[i];
                                    size_t ID_DashPos=ID.find(__T('-'));
                                    Ztring ID2;
                                    if (ID_DashPos!=(size_t)-1)
                                    {
                                        ID2=ID.substr(ID_DashPos+1);
                                        ID.resize(ID_DashPos);
                                    }
                                    Ztring ToSearch=Up.substr(10, Up.find('_', 10)-10)+ID;
                                    Ztring ToSearch2=ToSearch;
                                    if (!ID2.empty())
                                    {
                                        ToSearch2+=__T(" Alt");
                                        ToSearch2+=ID2;
                                    }

                                    // Search the linked element
                                    for (size_t j=0; j<(*Stream_More)[StreamKind][StreamPos].size(); j++)
                                    {
                                        if ((*Stream_More)[StreamKind][StreamPos][j][Info_Name]==ToSearch)
                                        {
                                            // Append the sub-elements of the linked element
                                            for (; j<(*Stream_More)[StreamKind][StreamPos].size(); j++)
                                            {
                                                // Stop if not the linked element
                                                if ((*Stream_More)[StreamKind][StreamPos][j][Info_Name].rfind(ToSearch, 0)==string::npos
                                                 && (*Stream_More)[StreamKind][StreamPos][j][Info_Name][0]!=__T(' '))
                                                    break;

                                                // Manage Alt parts when it is inside a list
                                                Ztring ToSearchAlt=ToSearch+__T(" Alt");
                                                if ((*Stream_More)[StreamKind][StreamPos][j][Info_Name].rfind(ToSearchAlt, 0)!=string::npos)
                                                {
                                                    size_t NextChar=(*Stream_More)[StreamKind][StreamPos][j][Info_Name].find_first_not_of(__T("0123456789"), ToSearchAlt.size());
                                                    if (NextChar==string::npos || (*Stream_More)[StreamKind][StreamPos][j][Info_Name][NextChar]==__T(' '))
                                                    {
                                                        if (ID2.empty())
                                                            continue;

                                                        // Check if content is not in main and we have the right Alt
                                                        if ((*Stream_More)[StreamKind][StreamPos][j][Info_Name].rfind(ToSearch2, 0)==0 && (*Stream_More)[StreamKind][StreamPos][j][Info_Name].size()>=ToSearch2.size())
                                                        {
                                                            Ztring Name=ToSearch;
                                                            if ((*Stream_More)[StreamKind][StreamPos][j][Info_Name].size()>ToSearch2.size())
                                                                Name+=__T(' ')+(*Stream_More)[StreamKind][StreamPos][j][Info_Name].substr(ToSearch2.size()+1);
                                                            size_t IsFound=(size_t)-1;
                                                            for (size_t k=Pos+1; k<j; k++)
                                                            {
                                                                if ((*Stream_More)[StreamKind][StreamPos][k][Info_Name]==Name)
                                                                {
                                                                    IsFound=k;
                                                                    break;
                                                                }
                                                            }
                                                            if (IsFound==(size_t)-1)
                                                            {
                                                                Temp.push_back((*Stream_More)[StreamKind][StreamPos][j]);
                                                                Temp.back()[Info_Name]=Name;
                                                                Temp.back()[Info_Name].insert(0, SpacesCount, __T(' '));
                                                            }
                                                        }

                                                        continue;
                                                    }
                                                }

                                                // If Alt link, find the Alt element and replace the main element with the alt value
                                                Ztring Found;
                                                if (!ID2.empty())
                                                {
                                                    size_t Space=(*Stream_More)[StreamKind][StreamPos][j][Info_Name].rfind(__T(' '));
                                                    if (Space!=string::npos || (*Stream_More)[StreamKind][StreamPos][j][Info_Name]==ToSearch)
                                                    {
                                                        Ztring ToSearch3=ToSearch2;
                                                        if (Space!=string::npos)
                                                            ToSearch3+=(*Stream_More)[StreamKind][StreamPos][j][Info_Name].substr(Space);
                                                        for (size_t k=Pos+1; k<(*Stream_More)[StreamKind][StreamPos].size(); k++)
                                                        {
                                                            if ((*Stream_More)[StreamKind][StreamPos][k][Info_Name]==ToSearch3)
                                                            {
                                                                Found=(*Stream_More)[StreamKind][StreamPos][k][Info_Text];
                                                                if (k && !Temp.empty()
                                                                 && Temp[Temp.size()-1][Info_Name].rfind(__T(" ChannelLayout" ))+14==Temp[Temp.size()-1][Info_Name].size()
                                                                 && (*Stream_More)[StreamKind][StreamPos][k  ][Info_Name].rfind(__T(" Position_Polar"))+15==(*Stream_More)[StreamKind][StreamPos][k  ][Info_Name].size()
                                                                 && (*Stream_More)[StreamKind][StreamPos][k-1][Info_Name].rfind(__T(" ChannelLayout" ))+14!=(*Stream_More)[StreamKind][StreamPos][k-1][Info_Name].size())
                                                                {
                                                                    Temp.resize(Temp.size()-1);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                
                                                // Append
                                                Temp.push_back((*Stream_More)[StreamKind][StreamPos][j]);
                                                if (!ID2.empty())
                                                    Temp.back()[Info_Name].insert(ToSearch.size(), __T("-Alt")+ID2);
                                                Temp.back()[Info_Name].insert(0, SpacesCount, __T(' '));
                                                if (IsComplementary)
                                                    Temp.back()[Info_Name].FindAndReplace(__T(" Object"), __T("ComplementaryObject"));
                                                if (!Found.empty())
                                                    Temp.back()[Info_Text]=Found;;
                                            }
                                        }
                                    }
                                }
                            }
                            else if (Up_Pos==string::npos || Name.size()<=11 || Name.find(__T("_Pos/String"), Name.size()-11)==Name.size()-11)
                                Temp.push_back((*Stream_More)[StreamKind][StreamPos][Pos]);

                        }
                    (*Stream_More)[StreamKind][StreamPos]=Temp;
                }
        }
    }

    //Sub-elements
    for (size_t StreamKind=Stream_General; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<(*Stream_More)[StreamKind].size(); StreamPos++)
        {
            const ZtringListList& Track=(*Stream_More)[StreamKind][StreamPos];
            for (size_t Pos=0; Pos<Track.size(); Pos++)
            {
                const ZtringList& Field=Track[Pos];
                if (Field.size()>Info_Name_Text)
                {
                    // Text
                    Ztring Name=Field[Info_Name];
                    size_t Spaces=0;
                    size_t i=0;
                    bool Nested=false;
                    for (;;)
                    {
                        size_t j=Name.find(__T(' '), i);
                        if (!j)
                            Nested=true;
                        if (j==(size_t)-1)
                            break;
                        i=j+1;
                        Spaces++;
                    }
                    //if (Spaces)
                    {
                        size_t j;
                        if (i>=2 && Pos && i-2==Track[Pos-1][Info_Name].size())
                            j=2;
                        else
                            j=1;
                        size_t PosPrevious=Pos;
                        if (PosPrevious)
                            PosPrevious--;
                        while (PosPrevious && Track[PosPrevious][Info_Name][0]==__T(' ')) // avoid injected lines
                            PosPrevious--;
                        if (!Nested && Spaces && Pos && i && i-j<=Track[PosPrevious][Info_Name].size() && Name.substr(0, i-j)==Track[PosPrevious][Info_Name].substr(0, i-j) && Info_Name_Text<Track[PosPrevious].size() && Spaces-1<=Track[PosPrevious][Info_Name_Text].find_first_not_of(__T(' ')))
                            Nested=true;
                        if (Nested)
                            Name.erase(0, i);
                        ZtringList Names;
                        vector<size_t> Numbers;
                        if (!Name.empty() && Pos+1<Track.size())
                        {
                            const ZtringList& Field1=Track[Pos+1];
                            const Ztring& Name1=Field1[Info_Name];
                            size_t Name1_SpacePos=i+Name.size();
                            if (Name1_SpacePos<Name1.size() && Name1[Name1_SpacePos]==__T(' ') && Name==Name1.substr(i, Name.size()))
                            {
                                ZtringList List;
                                List.Separator_Set(0, __T("-"));
                                List.Write(Name);
                                for (size_t i=0; i<List.size(); i++)
                                {
                                    size_t Text_End=List[i].find_last_not_of(__T("0123456789"))+1;
                                    if (Text_End!=List[i].size())
                                    {
                                        Numbers.push_back(Ztring(List[i].substr(Text_End)).To_int64u()+1);
                                        Names.push_back(List[i].substr(0, Text_End));
                                    }
                                }
                            }
                        }
                        if (Names.empty())
                        {
                            Names.push_back(Name);
                            Numbers.push_back(0);
                        }
                        for (size_t i=0; i<Names.size(); i++)
                        {
                            auto& Name=Names[i];
                            Ztring TranslatedName=MediaInfoLib::Config.Language_Get(Name);
                            if (!TranslatedName.empty())
                                Name=TranslatedName;
                            if (Nested && !i)
                                Name.insert(0, Spaces, __T(' '));
                            if (Name.size()>1 && Name.back()==__T('_'))
                            {
                                auto Last2=Name[Name.size()-2];
                                if (Last2>='0' && Last2<='9')
                                    Name.pop_back();
                            }
                            auto& Number=Numbers[i];
                            if (Number)
                                Name+=MediaInfoLib::Config.Language_Get(__T("  Config_Text_NumberTag"))+Ztring::ToZtring(Number);
                        }
                        Names.Separator_Set(0, __T(" "));
                        Names.Quote_Set(__T(""));
                        (*Stream_More)[StreamKind][StreamPos][Pos][Info_Name_Text]=Names.Read();
                    }
                }
            }
        }
}

//---------------------------------------------------------------------------
#if MEDIAINFO_MD5
void MediaInfo_Config_MediaInfo::File_Md5_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Md5=NewValue;
    Hash_Functions.set(HashWrapper::MD5, NewValue);
}

bool MediaInfo_Config_MediaInfo::File_Md5_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Hash_Functions[HashWrapper::MD5];
}
#endif //MEDIAINFO_MD5

//---------------------------------------------------------------------------
#if MEDIAINFO_HASH
void MediaInfo_Config_MediaInfo::File_Hash_Set (HashWrapper::HashFunctions Funtions)
{
    CriticalSectionLocker CSL(CS);
    Hash_Functions=Funtions;
    
    //Legacy
    if (File_Md5)
        Hash_Functions.set(HashWrapper::MD5);
}

HashWrapper::HashFunctions MediaInfo_Config_MediaInfo::File_Hash_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Hash_Functions;
}
#endif //MEDIAINFO_HASH

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_REFERENCES_YES)
void MediaInfo_Config_MediaInfo::File_CheckSideCarFiles_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_CheckSideCarFiles=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_CheckSideCarFiles_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_CheckSideCarFiles;
}
#endif //defined(MEDIAINFO_REFERENCES_YES)

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_IgnoreSequenceFileSize_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_IgnoreSequenceFileSize=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_IgnoreSequenceFileSize_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_IgnoreSequenceFileSize;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_IgnoreSequenceFilesCount_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_IgnoreSequenceFilesCount=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_IgnoreSequenceFilesCount_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_IgnoreSequenceFilesCount;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_SequenceFilesSkipFrames_Set (int64u NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_SequenceFilesSkipFrames=NewValue;
}

int64u MediaInfo_Config_MediaInfo::File_SequenceFilesSkipFrames_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_SequenceFilesSkipFrames;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_DefaultFrameRate_Set (float64 NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_DefaultFrameRate=NewValue;
    #if MEDIAINFO_DEMUX
        Demux_Rate=File_DefaultFrameRate;
    #endif //MEDIAINFO_DEMUX
}

float64 MediaInfo_Config_MediaInfo::File_DefaultFrameRate_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_DefaultFrameRate;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_DefaultTimeCode_Set(string NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_DefaultTimeCode = NewValue;
}

string MediaInfo_Config_MediaInfo::File_DefaultTimeCode_Get()
{
    CriticalSectionLocker CSL(CS);
    return File_DefaultTimeCode;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
Ztring MediaInfo_Config_MediaInfo::File_DefaultTimeCodeDropFrame_Set(const String& NewValue)
{
    int8u NewValueI;
    if (NewValue.empty())
        NewValueI=(int8u)-1;
    else if (NewValue.size()==1 && NewValue[0]>=__T('0') && NewValue[0] <=__T('1'))
        NewValueI=NewValue[0]-__T('0');
    else
        return __T("File_DefaultTimeCodeDropFrame value must be empty, 0 or 1");

    CriticalSectionLocker CSL(CS);
    File_DefaultTimeCodeDropFrame=NewValueI;
    return Ztring();
}

int8u MediaInfo_Config_MediaInfo::File_DefaultTimeCodeDropFrame_Get()
{
    CriticalSectionLocker CSL(CS);
    return File_DefaultTimeCodeDropFrame;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_Source_List_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Source_List=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Source_List_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Source_List;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_RiskyBitRateEstimation_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_RiskyBitRateEstimation=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_RiskyBitRateEstimation_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_RiskyBitRateEstimation;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_MergeBitRateInfo_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_MergeBitRateInfo=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_MergeBitRateInfo_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_MergeBitRateInfo;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_ChannelLayout_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_ChannelLayout =NewValue;
}

bool MediaInfo_Config_MediaInfo::File_ChannelLayout_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_ChannelLayout;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_HighestFormat_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_HighestFormat=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_HighestFormat_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_HighestFormat;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
#if MEDIAINFO_ADVANCED
void MediaInfo_Config_MediaInfo::File_Demux_Unpacketize_StreamLayoutChange_Skip_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Demux_Unpacketize_StreamLayoutChange_Skip=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Demux_Unpacketize_StreamLayoutChange_Skip_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Demux_Unpacketize_StreamLayoutChange_Skip;
}
#endif //MEDIAINFO_ADVANCED
#endif //MEDIAINFO_DEMUX

//***************************************************************************
// File name from somewhere else
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_FileName_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_FileName=NewValue;
}

Ztring MediaInfo_Config_MediaInfo::File_FileName_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_FileName;
}

//***************************************************************************
// File name format
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_FileNameFormat_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_FileNameFormat=NewValue;
}

Ztring MediaInfo_Config_MediaInfo::File_FileNameFormat_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_FileNameFormat;
}

//***************************************************************************
// Time to live
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_TimeToLive_Set (float64 NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_TimeToLive=NewValue;
}

float64 MediaInfo_Config_MediaInfo::File_TimeToLive_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_TimeToLive;
}

//***************************************************************************
// Partial file (begin and end are cut)
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_Partial_Begin_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Partial_Begin=NewValue;
}

Ztring MediaInfo_Config_MediaInfo::File_Partial_Begin_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Partial_Begin;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_Partial_End_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Partial_End=NewValue;
}

Ztring MediaInfo_Config_MediaInfo::File_Partial_End_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Partial_End;
}

//***************************************************************************
// Force Parser
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_ForceParser_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_ForceParser=NewValue;
}

Ztring MediaInfo_Config_MediaInfo::File_ForceParser_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_ForceParser;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_ForceParser_Config_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_ForceParser_Config=NewValue;
}

Ztring MediaInfo_Config_MediaInfo::File_ForceParser_Config_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_ForceParser_Config;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_Buffer_Size_Hint_Pointer_Set (size_t* NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Buffer_Size_Hint_Pointer=NewValue;
}

size_t*  MediaInfo_Config_MediaInfo::File_Buffer_Size_Hint_Pointer_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Buffer_Size_Hint_Pointer;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_Buffer_Read_Size_Set (size_t NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Buffer_Read_Size=NewValue;
}

size_t  MediaInfo_Config_MediaInfo::File_Buffer_Read_Size_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Buffer_Read_Size;
}

//***************************************************************************
// Filter
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_FILTER
void MediaInfo_Config_MediaInfo::File_Filter_Set (int64u NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Filter_16[(int16u)NewValue]=true;
    File_Filter_HasChanged_=true;
}

bool MediaInfo_Config_MediaInfo::File_Filter_Get (const int16u Value)
{
    CriticalSectionLocker CSL(CS);
    //Test
    bool Exists;
    if (File_Filter_16.empty())
        Exists=true;
    else
        Exists=(File_Filter_16.find(Value)!=File_Filter_16.end());
    return Exists;
}

bool MediaInfo_Config_MediaInfo::File_Filter_Get ()
{
    CriticalSectionLocker CSL(CS);
    bool Exist=!File_Filter_16.empty();
    return Exist;
}

void MediaInfo_Config_MediaInfo::File_Filter_Audio_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Filter_Audio=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Filter_Audio_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Filter_Audio;
}

bool MediaInfo_Config_MediaInfo::File_Filter_HasChanged ()
{
    CriticalSectionLocker CSL(CS);
    bool File_Filter_HasChanged_Temp=File_Filter_HasChanged_;
    File_Filter_HasChanged_=false;
    return File_Filter_HasChanged_Temp;
}
#endif //MEDIAINFO_FILTER

//***************************************************************************
// Duplicate
//***************************************************************************

#if MEDIAINFO_DUPLICATE
Ztring MediaInfo_Config_MediaInfo::File_Duplicate_Set (const Ztring &Value_In)
{
    //Preparing for File__Duplicate...
    Ztring ToReturn;
    {
    CriticalSectionLocker CSL(CS);
    File__Duplicate_List.push_back(Value_In);

    //Handling Memory index
    ZtringList List=Value_In;
    for (size_t Pos=0; Pos<List.size(); Pos++)
    {
        //Form= "(-)Data", if "-" the value will be removed
        Ztring &Value=List[Pos];
        bool ToRemove=false;
        if (Value.find(__T('-'))==0)
        {
            Value.erase(Value.begin());
            ToRemove=true;
        }

        //Testing if this is information about a target
        if (List[Pos].find(__T("memory:"))==0 || List[Pos].find(__T("file:"))==0)
        {
            //Searching if already exist
            size_t Memory_Pos=File__Duplicate_Memory_Indexes.Find(List[Pos]);
            if (!ToRemove && Memory_Pos==Error)
            {
                //Does not exist yet (and adding is wanted)
                Memory_Pos=File__Duplicate_Memory_Indexes.Find(__T(""));
                if (Memory_Pos!=Error)
                    File__Duplicate_Memory_Indexes[Memory_Pos]=List[Pos]; //A free place is found
                else
                {
                    //Adding the place at the end
                    Memory_Pos=File__Duplicate_Memory_Indexes.size();
                    File__Duplicate_Memory_Indexes.push_back(List[Pos]);
                }
            }
            else if (ToRemove)
            {
                //Exists yet but Removal is wanted
                File__Duplicate_Memory_Indexes[Memory_Pos].clear();
                Memory_Pos=(size_t)-1;
            }

            ToReturn+=__T(";")+Ztring().From_Number(Memory_Pos);
        }
    }
    if (!ToReturn.empty())
        ToReturn.erase(ToReturn.begin()); //Remove first ";"
    }
    File_IsSeekable_Set(false); //If duplication, we can not seek anymore

    return ToReturn;
}

Ztring MediaInfo_Config_MediaInfo::File_Duplicate_Get (size_t AlreadyRead_Pos)
{
    CriticalSectionLocker CSL(CS);
    if (AlreadyRead_Pos>=File__Duplicate_List.size())
        return Ztring(); //Nothing or not more than the last time
    return File__Duplicate_List[AlreadyRead_Pos];
}

bool MediaInfo_Config_MediaInfo::File_Duplicate_Get_AlwaysNeeded (size_t AlreadyRead_Pos)
{
    CriticalSectionLocker CSL(CS);
    bool Temp=AlreadyRead_Pos>=File__Duplicate_List.size();
    return !Temp; //True if there is something to read
}

size_t MediaInfo_Config_MediaInfo::File__Duplicate_Memory_Indexes_Get (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    return File__Duplicate_Memory_Indexes.Find(Value);
}

void MediaInfo_Config_MediaInfo::File__Duplicate_Memory_Indexes_Erase (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    size_t Pos=File__Duplicate_Memory_Indexes.Find(Value);
    if (Pos!=Error)
        File__Duplicate_Memory_Indexes[Pos].clear();
}
#endif //MEDIAINFO_DUPLICATE

//***************************************************************************
// Demux
//***************************************************************************

#if MEDIAINFO_DEMUX
//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Demux_ForceIds_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_ForceIds=NewValue;
}

bool MediaInfo_Config_MediaInfo::Demux_ForceIds_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_ForceIds;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Demux_PCM_20bitTo16bit_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_PCM_20bitTo16bit=NewValue;
}

bool MediaInfo_Config_MediaInfo::Demux_PCM_20bitTo16bit_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_PCM_20bitTo16bit;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Demux_PCM_20bitTo24bit_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_PCM_20bitTo24bit=NewValue;
}

bool MediaInfo_Config_MediaInfo::Demux_PCM_20bitTo24bit_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_PCM_20bitTo24bit;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10=NewValue;
}

bool MediaInfo_Config_MediaInfo::Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Demux_Hevc_Transcode_Iso14496_15_to_AnnexB_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_Hevc_Transcode_Iso14496_15_to_AnnexB=NewValue;
}

bool MediaInfo_Config_MediaInfo::Demux_Hevc_Transcode_Iso14496_15_to_AnnexB_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_Hevc_Transcode_Iso14496_15_to_AnnexB;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Demux_Unpacketize_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_Unpacketize=NewValue;
}

bool MediaInfo_Config_MediaInfo::Demux_Unpacketize_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_Unpacketize;
}
#endif //MEDIAINFO_DEMUX

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
void MediaInfo_Config_MediaInfo::Demux_SplitAudioBlocks_Set(bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_SplitAudioBlocks = NewValue;
}

bool MediaInfo_Config_MediaInfo::Demux_SplitAudioBlocks_Get()
{
    CriticalSectionLocker CSL(CS);
    return Demux_SplitAudioBlocks;
}
#endif //MEDIAINFO_DEMUX

//---------------------------------------------------------------------------
#if 1 //MEDIAINFO_DEMUX
void MediaInfo_Config_MediaInfo::Demux_Rate_Set (float64 NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_Rate=NewValue;
}

float64 MediaInfo_Config_MediaInfo::Demux_Rate_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_Rate;
}
#endif //MEDIAINFO_DEMUX

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
void MediaInfo_Config_MediaInfo::Demux_FirstDts_Set (int64u NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_FirstDts=NewValue;
}

int64u MediaInfo_Config_MediaInfo::Demux_FirstDts_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_FirstDts;
}
#endif //MEDIAINFO_DEMUX

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
void MediaInfo_Config_MediaInfo::Demux_FirstFrameNumber_Set (int64u NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_FirstFrameNumber=NewValue;
}

int64u MediaInfo_Config_MediaInfo::Demux_FirstFrameNumber_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_FirstFrameNumber;
}
#endif //MEDIAINFO_DEMUX

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
void MediaInfo_Config_MediaInfo::Demux_InitData_Set (int8u NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux_InitData=NewValue;
}

int8u MediaInfo_Config_MediaInfo::Demux_InitData_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux_InitData;
}
#endif //MEDIAINFO_DEMUX

//***************************************************************************
// IBI support
//***************************************************************************

#if MEDIAINFO_IBIUSAGE
//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Ibi_Set (const Ztring &Value)
{
    string Data_Base64=Value.To_UTF8();

    CriticalSectionLocker CSL(CS);
    Ibi=Base64::decode(Data_Base64);
}

string MediaInfo_Config_MediaInfo::Ibi_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ibi;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Ibi_UseIbiInfoIfAvailable_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Ibi_UseIbiInfoIfAvailable=NewValue;
}

bool MediaInfo_Config_MediaInfo::Ibi_UseIbiInfoIfAvailable_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ibi_UseIbiInfoIfAvailable;
}
#endif //MEDIAINFO_IBIUSAGE

#if MEDIAINFO_IBIUSAGE
//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Ibi_Create_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Ibi_Create=NewValue;
}

bool MediaInfo_Config_MediaInfo::Ibi_Create_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ibi_Create;
}
#endif //MEDIAINFO_IBIUSAGE

#if MEDIAINFO_FIXITY
//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::TryToFix_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    TryToFix=NewValue;
}

bool MediaInfo_Config_MediaInfo::TryToFix_Get ()
{
    CriticalSectionLocker CSL(CS);
    return TryToFix || MediaInfoLib::Config.TryToFix_Get();
}
#endif //MEDIAINFO_FIXITY

//***************************************************************************
// Encryption
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_AES
void MediaInfo_Config_MediaInfo::Encryption_Format_Set (const Ztring &Value)
{
    string Data=Value.To_UTF8();
    encryption_format Encryption_Format_Temp=Encryption_Format_None;
    if (Data=="AES")
        Encryption_Format_Temp=Encryption_Format_Aes;

    CriticalSectionLocker CSL(CS);
    Encryption_Format=Encryption_Format_Temp;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Encryption_Format_Set (encryption_format Value)
{
    CriticalSectionLocker CSL(CS);
    Encryption_Format=Value;
}

string MediaInfo_Config_MediaInfo::Encryption_Format_GetS ()
{
    CriticalSectionLocker CSL(CS);
    switch (Encryption_Format)
    {
        case Encryption_Format_Aes: return "AES";
        default: return string();
    }
}

encryption_format MediaInfo_Config_MediaInfo::Encryption_Format_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Encryption_Format;
}
#endif //MEDIAINFO_AES

//---------------------------------------------------------------------------
#if MEDIAINFO_AES
void MediaInfo_Config_MediaInfo::Encryption_Key_Set (const Ztring &Value)
{
    string Data_Base64=Value.To_UTF8();

    CriticalSectionLocker CSL(CS);
    Encryption_Key=Base64::decode(Data_Base64);
}

void MediaInfo_Config_MediaInfo::Encryption_Key_Set (const int8u* Value, size_t Value_Size)
{
    CriticalSectionLocker CSL(CS);
    Encryption_Key=string((const char*)Value, Value_Size);
}

string MediaInfo_Config_MediaInfo::Encryption_Key_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Encryption_Key;
}
#endif //MEDIAINFO_AES

//---------------------------------------------------------------------------
#if MEDIAINFO_AES
void MediaInfo_Config_MediaInfo::Encryption_Method_Set (const Ztring &Value)
{
    string Data=Value.To_UTF8();
    encryption_method Encryption_Method_Temp=Encryption_Method_None;
    if (Data=="Segment")
        Encryption_Method_Temp=Encryption_Method_Segment;

    CriticalSectionLocker CSL(CS);
    Encryption_Method=Encryption_Method_Temp;
}

void MediaInfo_Config_MediaInfo::Encryption_Method_Set (encryption_method Value)
{
    CriticalSectionLocker CSL(CS);
    Encryption_Method=Value;
}

string MediaInfo_Config_MediaInfo::Encryption_Method_GetS ()
{
    CriticalSectionLocker CSL(CS);
    switch (Encryption_Method)
    {
        case Encryption_Method_Segment: return "Segment";
        default: return string();
    }
}

encryption_method MediaInfo_Config_MediaInfo::Encryption_Method_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Encryption_Method;
}
#endif //MEDIAINFO_AES

//---------------------------------------------------------------------------
#if MEDIAINFO_AES
void MediaInfo_Config_MediaInfo::Encryption_Mode_Set (const Ztring &Value)
{
    string Data=Value.To_UTF8();
    encryption_mode Encryption_Mode_Temp=Encryption_Mode_None;
    if (Data=="CBC")
        Encryption_Mode_Temp=Encryption_Mode_Cbc;

    CriticalSectionLocker CSL(CS);
    Encryption_Mode=Encryption_Mode_Temp;
}

void MediaInfo_Config_MediaInfo::Encryption_Mode_Set (encryption_mode Value)
{
    CriticalSectionLocker CSL(CS);
    Encryption_Mode=Value;
}

string MediaInfo_Config_MediaInfo::Encryption_Mode_GetS ()
{
    CriticalSectionLocker CSL(CS);
    switch (Encryption_Mode)
    {
        case Encryption_Mode_Cbc: return "CBC";
        default: return string();
    }
}

encryption_mode MediaInfo_Config_MediaInfo::Encryption_Mode_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Encryption_Mode;
}
#endif //MEDIAINFO_AES

//---------------------------------------------------------------------------
#if MEDIAINFO_AES
void MediaInfo_Config_MediaInfo::Encryption_Padding_Set (const Ztring &Value)
{
    string Data=Value.To_UTF8();
    encryption_padding Encryption_Padding_Temp=Encryption_Padding_None;
    if (Data=="PKCS7")
        Encryption_Padding_Temp=Encryption_Padding_Pkcs7;

    CriticalSectionLocker CSL(CS);
    Encryption_Padding=Encryption_Padding_Temp;
}

void MediaInfo_Config_MediaInfo::Encryption_Padding_Set (encryption_padding Value)
{
    CriticalSectionLocker CSL(CS);
    Encryption_Padding=Value;
}

string MediaInfo_Config_MediaInfo::Encryption_Padding_GetS ()
{
    CriticalSectionLocker CSL(CS);
    switch (Encryption_Padding)
    {
        case Encryption_Padding_Pkcs7: return "PKCS7";
        default: return string();
    }
}

encryption_padding MediaInfo_Config_MediaInfo::Encryption_Padding_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Encryption_Padding;
}
#endif //MEDIAINFO_AES

//---------------------------------------------------------------------------
#if MEDIAINFO_AES
void MediaInfo_Config_MediaInfo::Encryption_InitializationVector_Set (const Ztring &Value)
{
    if (Value==__T("Sequence number"))
    {
        CriticalSectionLocker CSL(CS);
        Encryption_InitializationVector="Sequence number";
    }
    else
    {
        string Data_Base64=Value.To_UTF8();

        CriticalSectionLocker CSL(CS);
        Encryption_InitializationVector=Base64::decode(Data_Base64);
    }
}

string MediaInfo_Config_MediaInfo::Encryption_InitializationVector_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Encryption_InitializationVector;
}
#endif //MEDIAINFO_AES

//***************************************************************************
// NextPacket
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_NEXTPACKET
void MediaInfo_Config_MediaInfo::NextPacket_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    NextPacket=NewValue;
}

bool MediaInfo_Config_MediaInfo::NextPacket_Get ()
{
    CriticalSectionLocker CSL(CS);
    return NextPacket;
}
#endif //MEDIAINFO_NEXTPACKET

//***************************************************************************
// SubFile
//***************************************************************************

#if MEDIAINFO_EVENTS
//---------------------------------------------------------------------------
ZtringListList MediaInfo_Config_MediaInfo::SubFile_Config_Get ()
{
    CriticalSectionLocker CSL(CS);

    return SubFile_Config;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::SubFile_StreamID_Set (int64u Value)
{
    CriticalSectionLocker CSL(CS);

    SubFile_StreamID=Value;
}

//---------------------------------------------------------------------------
int64u MediaInfo_Config_MediaInfo::SubFile_StreamID_Get ()
{
    CriticalSectionLocker CSL(CS);

    return SubFile_StreamID;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::SubFile_IDs_Set (Ztring Value)
{
    CriticalSectionLocker CSL(CS);

    SubFile_IDs=Value;
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Config_MediaInfo::SubFile_IDs_Get ()
{
    CriticalSectionLocker CSL(CS);

    return SubFile_IDs;
}
#endif //MEDIAINFO_EVENTS

//***************************************************************************
// SubFile
//***************************************************************************

#if MEDIAINFO_EVENTS
//---------------------------------------------------------------------------
bool MediaInfo_Config_MediaInfo::ParseUndecodableFrames_Get ()
{
    CriticalSectionLocker CSL(CS);

    return ParseUndecodableFrames;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::ParseUndecodableFrames_Set (bool Value)
{
    CriticalSectionLocker CSL(CS);

    ParseUndecodableFrames=Value;
}
#endif //MEDIAINFO_EVENTS

//***************************************************************************
// Event
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
bool MediaInfo_Config_MediaInfo::Event_CallBackFunction_IsSet ()
{
    CriticalSectionLocker CSL(CS);

    return Event_CallBackFunction?true:false;
}
#endif //MEDIAINFO_EVENTS

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
Ztring MediaInfo_Config_MediaInfo::Event_CallBackFunction_Set (const Ztring &Value)
{
    ZtringList List=Value;

    CriticalSectionLocker CSL(CS);

    if (List.empty())
    {
        Event_CallBackFunction=(MediaInfo_Event_CallBackFunction*)NULL;
        Event_UserHandler=NULL;
    }
    else
        for (size_t Pos=0; Pos<List.size(); Pos++)
        {
            if (List[Pos].find(__T("CallBack=memory://"))==0)
                Event_CallBackFunction=(MediaInfo_Event_CallBackFunction*)Ztring(List[Pos].substr(18, std::string::npos)).To_int64u();
            else if (List[Pos].find(__T("UserHandle=memory://"))==0)
                Event_UserHandler=(void*)Ztring(List[Pos].substr(20, std::string::npos)).To_int64u();
            else if (List[Pos].find(__T("UserHandler=memory://"))==0)
                Event_UserHandler=(void*)Ztring(List[Pos].substr(21, std::string::npos)).To_int64u();
            else
                return("Problem during Event_CallBackFunction value parsing");
        }

    return Ztring();
}
#endif //MEDIAINFO_EVENTS

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
Ztring MediaInfo_Config_MediaInfo::Event_CallBackFunction_Get ()
{
    CriticalSectionLocker CSL(CS);

    return __T("CallBack=memory://")+Ztring::ToZtring((size_t)Event_CallBackFunction)+__T(";UserHandler=memory://")+Ztring::ToZtring((size_t)Event_UserHandler);
}
#endif //MEDIAINFO_EVENTS

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
void MediaInfo_Config_MediaInfo::Event_Send (File__Analyze* Source, const int8u* Data_Content, size_t Data_Size, const Ztring &File_Name)
{
    CriticalSectionLocker CSL(CS);

    if (Source==NULL)
    {
        MediaInfo_Event_Generic* Temp=(MediaInfo_Event_Generic*)Data_Content;

        #if MEDIAINFO_DEMUX
        if (Demux_Offset_Frame!=(int64u)-1)
        {
            if (Temp->FrameNumber!=(int64u)-1)
                Temp->FrameNumber+=Demux_Offset_Frame;
            if (Temp->FrameNumber_PresentationOrder!=(int64u)-1)
                Temp->FrameNumber_PresentationOrder+=Demux_Offset_Frame;
        }
        if (Demux_Offset_DTS!=(int64u)-1)
        {
            if (Temp->DTS!=(int64u)-1)
                Temp->DTS+=Demux_Offset_DTS;
            if (Temp->PTS!=(int64u)-1)
                Temp->PTS+=Demux_Offset_DTS;
            if (Demux_Offset_DTS_FromStream!=(int64u)-1)
            {
                if (Temp->DTS!=(int64u)-1)
                    Temp->DTS-=Demux_Offset_DTS_FromStream;
                if (Temp->PTS!=(int64u)-1)
                    Temp->PTS-=Demux_Offset_DTS_FromStream;
            }
        }
        #endif //MEDIAINFO_DEMUX

        if (File_IgnoreEditsBefore)
        {
            if (Temp->FrameNumber!=(int64u)-1)
            {
                if (Temp->FrameNumber>File_IgnoreEditsBefore)
                    Temp->FrameNumber-=File_IgnoreEditsBefore;
                else
                    Temp->FrameNumber=0;
            }
            if (Temp->DTS!=(int64u)-1)
            {
                if (File_EditRate)
                {
                    int64u TimeOffset=float64_int64s(((float64)File_IgnoreEditsBefore)/File_EditRate*1000000000);
                    if (Temp->DTS>TimeOffset)
                        Temp->DTS-=TimeOffset;
                    else
                        Temp->DTS=0;
                }
            }
            if (Temp->PTS!=(int64u)-1)
            {
                if (File_IgnoreEditsBefore && File_EditRate)
                {
                    int64u TimeOffset=float64_int64s(((float64)File_IgnoreEditsBefore)/File_EditRate*1000000000);
                    if (Temp->PTS>TimeOffset)
                        Temp->PTS-=TimeOffset;
                    else
                        Temp->PTS=0;
                }
            }
        }
    }

    //Adaptation of time stamps
    if (!FileIsReferenced && !Events_TimestampShift_Disabled)
    {
        MediaInfo_Event_Generic* Event=(MediaInfo_Event_Generic*)Data_Content;

        if (Event->StreamIDs_Size>=2 && Event->EventCode!=(MediaInfo_Parser_DvDif<<24 | MediaInfo_Event_DvDif_Analysis_Frame<<8) &&  Event->ParserIDs[0]==MediaInfo_Parser_MpegTs && Event->ParserIDs[1]==MediaInfo_Parser_MpegPs)
        {
            //Catching reference stream
            if (Event->StreamIDs[1]==0xE0 && Events_TimestampShift_Reference_ID==(int64u)-1)
                Events_TimestampShift_Reference_ID=Event->StreamIDs[0];

            //Store the last PTS of the reference stream
            if (Events_TimestampShift_Reference_ID==Event->StreamIDs[0] && Event->PTS!=(int64u)-1)
                Events_TimestampShift_Reference_PTS=Event->PTS;

            //Sending events whose were delayed
            if (!Events_TimestampShift_Delayed.empty() && Events_TimestampShift_Reference_ID!=(int64u)-1 && Event->PTS!=(int64u)-1)
            {
                Events_TimestampShift_Disabled=true; //Disable the sending of events in the next call

                for (size_t Pos=0; Pos<Events_TimestampShift_Delayed.size(); Pos++)
                    if (Events_TimestampShift_Delayed[Pos])
                    {
                        CS.Leave();
                        Event_Send(NULL, Events_TimestampShift_Delayed[Pos]->Data_Content, Events_TimestampShift_Delayed[Pos]->Data_Size, Events_TimestampShift_Delayed[Pos]->File_Name);
                        CS.Enter();

                        int32u EventCode=*((int32u*)Events_TimestampShift_Delayed[Pos]->Data_Content);
                        bool IsSimpleText=(EventCode&0x00FFFF00)==(MediaInfo_Event_Global_SimpleText<<8);
                        if (IsSimpleText)
                        {
                            MediaInfo_Event_Global_SimpleText_0* Old=(MediaInfo_Event_Global_SimpleText_0*)Events_TimestampShift_Delayed[Pos]->Data_Content;
                            delete[] Old->Content; //Old->Content=NULL;
                            if (Old->Row_Values)
                            {
                                for (size_t Row_Pos=0; Row_Pos<Old->Row_Max; Row_Pos++)
                                    delete[] Old->Row_Values[Row_Pos]; // Row_Values[Row_Pos]=NULL;
                                delete[] Old->Row_Values; //Old->Row_Values=NULL;
                            }
                            if (Old->Row_Attributes)
                            {
                                for (size_t Row_Pos=0; Row_Pos<Old->Row_Max; Row_Pos++)
                                    delete[] Old->Row_Attributes[Row_Pos]; // Row_Attributes[Row_Pos]=NULL;
                                delete[] Old->Row_Attributes; //Old->Row_Attributes=NULL;
                            }
                        }

                        delete Events_TimestampShift_Delayed[Pos]; //Events_TimestampShift_Delayed[Pos]=NULL;
                    }
                Events_TimestampShift_Delayed.clear();
                Events_TimestampShift_Disabled=false;
            }

            //MediaInfo_Event_Global_SimpleText
            if ((Event->EventCode &0x00FFFF00)==(MediaInfo_Event_Global_SimpleText<<8)) //If it is MediaInfo_Event_Global_SimpleText
            {
                //Store the event if there is no reference stream
                if (Events_TimestampShift_Reference_ID==(int64u)-1 || Events_TimestampShift_Reference_PTS==(int64u)-1)
                {
                    event_delayed* Event=new event_delayed(Data_Content, Data_Size, File_Name);
                    Events_TimestampShift_Delayed.push_back(Event);

                    // Copying buffers
                    int32u* EventCode=(int32u*)Data_Content;
                    if (((*EventCode)&0x00FFFFFF)==(MediaInfo_Event_Global_SimpleText<<8) && Data_Size==sizeof(MediaInfo_Event_Global_SimpleText_0))
                    {
                        MediaInfo_Event_Global_SimpleText_0* Old=(MediaInfo_Event_Global_SimpleText_0*)Data_Content;
                        MediaInfo_Event_Global_SimpleText_0* New=(MediaInfo_Event_Global_SimpleText_0*)Event->Data_Content;
                        if (New->Content)
                        {
                            size_t Content_Size=wcslen(New->Content);
                            wchar_t* Content=new wchar_t[Content_Size+1];
                            std::memcpy(Content, Old->Content, (Content_Size+1)*sizeof(wchar_t));
                            New->Content=Content;
                        }
                        if (New->Row_Values)
                        {
                            wchar_t** Row_Values=new wchar_t*[New->Row_Max];
                            for (size_t Row_Pos=0; Row_Pos<New->Row_Max; Row_Pos++)
                            {
                                Row_Values[Row_Pos]=new wchar_t[New->Column_Max];
                                std::memcpy(Row_Values[Row_Pos], Old->Row_Values[Row_Pos], New->Column_Max*sizeof(wchar_t));
                            }
                            New->Row_Values=Row_Values;
                        }
                        if (New->Row_Attributes)
                        {
                            int8u** Row_Attributes=new int8u*[New->Row_Max];
                            for (size_t Row_Pos=0; Row_Pos<New->Row_Max; Row_Pos++)
                            {
                                Row_Attributes[Row_Pos]=new int8u[New->Column_Max];
                                std::memcpy(Row_Attributes[Row_Pos], Old->Row_Attributes[Row_Pos], New->Column_Max*sizeof(int8u));
                            }
                            New->Row_Attributes=Row_Attributes;
                        }
                    }
                    return;
                }

                //Shift of the PTS if difference is too huge
                if (Event->PTS>Events_TimestampShift_Reference_PTS+60000000000LL) // difference more than 60 seconds
                {
                    int64u Shift= Event->PTS -Events_TimestampShift_Reference_PTS;
                    if (Shift>=55555555555555LL-10000000000LL && Shift<=55555555555555LL+10000000000LL) //+/- 10 second
                        Shift=55555555555555LL;
                    if (Event->PTS!=(int64u)-1)
                        Event->PTS-=Shift;
                    if (Event->DTS!=(int64u)-1)
                        Event->DTS-=Shift;
                }
            }
        }
    }

    if (Source)
    {
        event_delayed* Event=new event_delayed(Data_Content, Data_Size, File_Name);
        Events_Delayed[Source].push_back(Event);

        // Copying buffers
        int32u* EventCode=(int32u*)Data_Content;
        if (((*EventCode)&0x00FFFFFF)==((MediaInfo_Event_Global_Demux<<8)|4) && Data_Size==sizeof(MediaInfo_Event_Global_Demux_4))
        {
            MediaInfo_Event_Global_Demux_4* Old=(MediaInfo_Event_Global_Demux_4*)Data_Content;
            MediaInfo_Event_Global_Demux_4* New=(MediaInfo_Event_Global_Demux_4*)Event->Data_Content;
            if (New->Content_Size)
            {
                int8u* Content=new int8u[New->Content_Size];
                std::memcpy(Content, Old->Content, New->Content_Size*sizeof(int8u));
                New->Content=Content;
            }
            if (New->Offsets_Size)
            {
                int64u* Offsets_Stream=new int64u[New->Offsets_Size];
                std::memcpy(Offsets_Stream, Old->Offsets_Stream, New->Offsets_Size*sizeof(int64u));
                New->Offsets_Stream=Offsets_Stream;
                int64u* Offsets_Content=new int64u[New->Offsets_Size];
                std::memcpy(Offsets_Content, Old->Offsets_Content, New->Offsets_Size*sizeof(int64u));
                New->Offsets_Content=Offsets_Content;
            }
            if (New->OriginalContent_Size)
            {
                int8u* OriginalContent=new int8u[New->OriginalContent_Size];
                std::memcpy(OriginalContent, Old->OriginalContent, New->OriginalContent_Size*sizeof(int8u));
                New->OriginalContent=OriginalContent;
            }
        }
        if (((*EventCode) & 0x00FFFFFF) == ((MediaInfo_Event_DvDif_Change << 8) | 0) && Data_Size == sizeof(MediaInfo_Event_DvDif_Change_0))
        {
            MediaInfo_Event_DvDif_Change_0* Old = (MediaInfo_Event_DvDif_Change_0*)Data_Content;
            MediaInfo_Event_DvDif_Change_0* New = (MediaInfo_Event_DvDif_Change_0*)Event->Data_Content;
            if (New->MoreData)
            {
                auto MoreData_Size = sizeof(size_t) + *((size_t*)New->MoreData);
                int8u* MoreData = new int8u[MoreData_Size];
                std::memcpy(MoreData, Old->MoreData, MoreData_Size * sizeof(int8u));
                New->MoreData = MoreData;
            }
        }
        if (((*EventCode) & 0x00FFFFFF) == ((MediaInfo_Event_DvDif_Analysis_Frame << 8) | 1) && Data_Size == sizeof(MediaInfo_Event_DvDif_Analysis_Frame_0))
        {
            MediaInfo_Event_DvDif_Analysis_Frame_1* Old = (MediaInfo_Event_DvDif_Analysis_Frame_1*)Data_Content;
            MediaInfo_Event_DvDif_Analysis_Frame_1* New = (MediaInfo_Event_DvDif_Analysis_Frame_1*)Event->Data_Content;
            if (New->MoreData)
            {
                auto MoreData_Size = sizeof(size_t) + *((size_t*)New->MoreData);
                int8u* MoreData = new int8u[MoreData_Size];
                std::memcpy(MoreData, Old->MoreData, MoreData_Size * sizeof(int8u));
                New->MoreData = MoreData;
            }
        }
    }
    else if (Event_CallBackFunction)
    {
        MEDIAINFO_DEBUG1(   "CallBackFunction",
                            Debug+=", EventID=";Debug+=Ztring::ToZtring(LittleEndian2int32u(Data_Content), 16).To_UTF8();)

        Event_CallBackFunction ((unsigned char*)Data_Content, Data_Size, Event_UserHandler);

        MEDIAINFO_DEBUG2(   "CallBackFunction",
                            )
    }
    #if MEDIAINFO_DEMUX
    else if (!File_Name.empty())
    {
        MediaInfo_Event_Generic* Event_Generic=(MediaInfo_Event_Generic*)Data_Content;
        if ((Event_Generic->EventCode&0x00FFFFFF)==((MediaInfo_Event_Global_Demux<<8)|0x04)) //Demux version 4
        {
            if (!MediaInfoLib::Config.Demux_Get())
                return;

            MediaInfo_Event_Global_Demux_4* Event=(MediaInfo_Event_Global_Demux_4*)Data_Content;

            Ztring File_Name_Final(File_Name);
            if (Event->StreamIDs_Size==0)
                File_Name_Final+=__T(".demux");
            else for (size_t Pos=0; Pos<Event->StreamIDs_Size; Pos++)
            {
                if (Event->StreamIDs_Width[Pos]==17)
                {
                    Ztring ID;
                    ID.From_CC4((int32u)Event->StreamIDs[Pos]);
                    File_Name_Final+=__T('.')+ID;
                }
                else if (Event->StreamIDs_Width[Pos])
                {
                    Ztring ID;
                    ID.From_Number(Event->StreamIDs[Pos], 16);
                    while (ID.size()<Event->StreamIDs_Width[Pos])
                        ID.insert(0,  1, __T('0'));
                    if (ID.size()>Event->StreamIDs_Width[Pos])
                        ID.erase(0, ID.size()-Event->StreamIDs_Width[Pos]);
                    File_Name_Final+=__T('.')+ID;
                }
                else
                    File_Name_Final+=__T(".raw");
            }

            std::map<Ztring, File>::iterator F = Demux_Files.find(File_Name_Final);
            if (F == Demux_Files.end())
            {
                Demux_Files[File_Name_Final].Open(File_Name_Final, File::Access_Write_Append);
                F = Demux_Files.find(File_Name_Final);
            }
            F->second.Write(Event->Content, Event->Content_Size);
        }
    }
    #endif //MEDIAINFO_DEMUX
}

void MediaInfo_Config_MediaInfo::Event_Accepted (File__Analyze* Source)
{
    #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
        if (Demux_EventWasSent && NextPacket_Get())
        {
            Events_Delayed_CurrentSource=Source;
            return;
        }
    #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET

    for (events_delayed::iterator Event=Events_Delayed.begin(); Event!=Events_Delayed.end(); ++Event)
        if (Event->first==Source)
        {
            for (size_t Pos=0; Pos<Event->second.size(); Pos++)
                if (Event->second[Pos])
                {
                    Event_Send(NULL, Event->second[Pos]->Data_Content, Event->second[Pos]->Data_Size, Event->second[Pos]->File_Name);

                    int32u EventCode=*((int32u*)Event->second[Pos]->Data_Content);
                    bool IsDemux=(EventCode&0x00FFFF00)==(MediaInfo_Event_Global_Demux<<8);

                    if (IsDemux)
                    {
                        MediaInfo_Event_Global_Demux_4* Old=(MediaInfo_Event_Global_Demux_4*)Event->second[Pos]->Data_Content;
                        delete[] Old->Content; Old->Content=NULL;
                        if (Old->Offsets_Size)
                        {
                            delete[] Old->Offsets_Content; Old->Offsets_Content=NULL;
                        }
                        if (Old->Offsets_Size)
                        {
                            delete[] Old->OriginalContent; Old->OriginalContent=NULL;
                        }
                    }

                    delete Event->second[Pos]; Event->second[Pos]=NULL;

                    #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                        if (IsDemux && NextPacket_Get())
                        {
                            Demux_EventWasSent=true;
                            Event->second.erase(Event->second.begin(), Event->second.begin()+Pos);
                            Events_Delayed_CurrentSource=Source;
                            return;
                        }
                    #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                }

            Events_Delayed.erase(Event->first);
            return;
        }
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Event_SubFile_Start(const Ztring &FileName_Absolute)
{
    Ztring FileName_Relative;
    if (File_Names_RootDirectory.empty())
    {
        FileName FN(FileName_Absolute);
        FileName_Relative=FN.Name_Get();
        if (!FN.Extension_Get().empty())
        {
            FileName_Relative+=__T('.');
            FileName_Relative+=FN.Extension_Get();
        }
    }
    else
    {
        Ztring Root=File_Names_RootDirectory+PathSeparator;
        FileName_Relative=FileName_Absolute;
        if (FileName_Relative.find(Root)==0)
            FileName_Relative.erase(0, Root.size());
    }

    struct MediaInfo_Event_General_SubFile_Start_0 Event;
    memset(&Event, 0xFF, sizeof(struct MediaInfo_Event_Generic));
    Event.EventCode=MediaInfo_EventCode_Create(0, MediaInfo_Event_General_SubFile_Start, 0);
    Event.EventSize=sizeof(struct MediaInfo_Event_General_SubFile_Start_0);
    Event.StreamIDs_Size=0;

    std::string FileName_Relative_Ansi=FileName_Relative.To_UTF8();
    std::wstring FileName_Relative_Unicode=FileName_Relative.To_Unicode();
    std::string FileName_Absolute_Ansi=FileName_Absolute.To_UTF8();
    std::wstring FileName_Absolute_Unicode=FileName_Absolute.To_Unicode();
    Event.FileName_Relative=FileName_Relative_Ansi.c_str();
    Event.FileName_Relative_Unicode=FileName_Relative_Unicode.c_str();
    Event.FileName_Absolute=FileName_Absolute_Ansi.c_str();
    Event.FileName_Absolute_Unicode=FileName_Absolute_Unicode.c_str();

    Event_Send(NULL, (const int8u*)&Event, Event.EventSize);
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Event_SubFile_Missing(const Ztring &FileName_Relative)
{
    struct MediaInfo_Event_General_SubFile_Missing_0 Event;
    memset(&Event, 0xFF, sizeof(struct MediaInfo_Event_Generic));
    Event.EventCode=MediaInfo_EventCode_Create(0, MediaInfo_Event_General_SubFile_Missing, 0);
    Event.EventSize=sizeof(struct MediaInfo_Event_General_SubFile_Start_0);
    Event.StreamIDs_Size=0;

    std::string FileName_Relative_Ansi=FileName_Relative.To_UTF8();
    std::wstring FileName_Relative_Unicode=FileName_Relative.To_Unicode();
    Event.FileName_Relative=FileName_Relative_Ansi.c_str();
    Event.FileName_Relative_Unicode=FileName_Relative_Unicode.c_str();
    Event.FileName_Absolute=NULL;
    Event.FileName_Absolute_Unicode=NULL;

    Event_Send(NULL, (const int8u*)&Event, Event.EventSize);
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::Event_SubFile_Missing_Absolute(const Ztring &FileName_Absolute)
{
    Ztring FileName_Relative;
    if (File_Names_RootDirectory.empty())
    {
        FileName FN(FileName_Absolute);
        FileName_Relative=FN.Name_Get();
        if (!FN.Extension_Get().empty())
        {
            FileName_Relative+=__T('.');
            FileName_Relative+=FN.Extension_Get();
        }
    }
    else
    {
        Ztring Root=File_Names_RootDirectory+PathSeparator;
        FileName_Relative=FileName_Absolute;
        if (FileName_Relative.find(Root)==0)
            FileName_Relative.erase(0, Root.size());
    }

    struct MediaInfo_Event_General_SubFile_Missing_0 Event;
    memset(&Event, 0xFF, sizeof(struct MediaInfo_Event_Generic));
    Event.EventCode=MediaInfo_EventCode_Create(0, MediaInfo_Event_General_SubFile_Missing, 0);
    Event.EventSize=sizeof(struct MediaInfo_Event_General_SubFile_Missing_0);
    Event.StreamIDs_Size=0;

    std::string FileName_Relative_Ansi=FileName_Relative.To_UTF8();
    std::wstring FileName_Relative_Unicode=FileName_Relative.To_Unicode();
    std::string FileName_Absolute_Ansi=FileName_Absolute.To_UTF8();
    std::wstring FileName_Absolute_Unicode=FileName_Absolute.To_Unicode();
    Event.FileName_Relative=FileName_Relative_Ansi.c_str();
    Event.FileName_Relative_Unicode=FileName_Relative_Unicode.c_str();
    Event.FileName_Absolute=FileName_Absolute_Ansi.c_str();
    Event.FileName_Absolute_Unicode=FileName_Absolute_Unicode.c_str();

    Event_Send(NULL, (const int8u*)&Event, Event.EventSize);
}
#endif //MEDIAINFO_EVENTS

//***************************************************************************
// Force Parser
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_MpegTs_ForceMenu_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_MpegTs_ForceMenu=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_MpegTs_ForceMenu_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_MpegTs_ForceMenu;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_MpegTs_stream_type_Trust_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_MpegTs_stream_type_Trust=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_MpegTs_stream_type_Trust_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_MpegTs_stream_type_Trust;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_MpegTs_Atsc_transport_stream_id_Trust_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_MpegTs_Atsc_transport_stream_id_Trust=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_MpegTs_Atsc_transport_stream_id_Trust_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_MpegTs_Atsc_transport_stream_id_Trust;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_MpegTs_RealTime_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_MpegTs_RealTime=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_MpegTs_RealTime_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_MpegTs_RealTime;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_Mxf_TimeCodeFromMaterialPackage_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Mxf_TimeCodeFromMaterialPackage=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Mxf_TimeCodeFromMaterialPackage_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Mxf_TimeCodeFromMaterialPackage;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_Mxf_ParseIndex_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Mxf_ParseIndex=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Mxf_ParseIndex_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Mxf_ParseIndex;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_Bdmv_ParseTargetedFile_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Bdmv_ParseTargetedFile=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Bdmv_ParseTargetedFile_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Bdmv_ParseTargetedFile;
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_DVDIF_YES)
void MediaInfo_Config_MediaInfo::File_DvDif_DisableAudioIfIsInContainer_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_DvDif_DisableAudioIfIsInContainer=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_DvDif_DisableAudioIfIsInContainer_Get ()
{
    CriticalSectionLocker CSL(CS);
    bool Temp=File_DvDif_DisableAudioIfIsInContainer;
    return Temp;
}
#endif //defined(MEDIAINFO_DVDIF_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_DVDIF_YES)
void MediaInfo_Config_MediaInfo::File_DvDif_IgnoreTransmittingFlags_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_DvDif_IgnoreTransmittingFlags=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_DvDif_IgnoreTransmittingFlags_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_DvDif_IgnoreTransmittingFlags;
}
#endif //defined(MEDIAINFO_DVDIF_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_DVDIF_ANALYZE_YES)
void MediaInfo_Config_MediaInfo::File_DvDif_Analysis_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_DvDif_Analysis=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_DvDif_Analysis_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_DvDif_Analysis;
}
#endif //defined(MEDIAINFO_DVDIF_ANALYZE_YES)

//---------------------------------------------------------------------------
#if MEDIAINFO_MACROBLOCKS
void MediaInfo_Config_MediaInfo::File_Macroblocks_Parse_Set (int NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Macroblocks_Parse=NewValue;
}

int MediaInfo_Config_MediaInfo::File_Macroblocks_Parse_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Macroblocks_Parse;
}
#endif //MEDIAINFO_MACROBLOCKS

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_GrowingFile_Delay_Set (float64 NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_GrowingFile_Delay=NewValue;
}

float64 MediaInfo_Config_MediaInfo::File_GrowingFile_Delay_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_GrowingFile_Delay;
}

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::File_GrowingFile_Force_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_GrowingFile_Force=NewValue;
}

//---------------------------------------------------------------------------
bool MediaInfo_Config_MediaInfo::File_GrowingFile_Force_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_GrowingFile_Force;
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_LIBCURL_YES)
void MediaInfo_Config_MediaInfo::File_Curl_Set (const Ztring &NewValue)
{
    size_t Pos=NewValue.find(__T(','));
    if (Pos==string::npos)
        Pos=NewValue.find(__T(';'));
    if (Pos!=string::npos)
    {
        Ztring Field=NewValue.substr(0, Pos); Field.MakeLowerCase();
        Ztring Value=NewValue.substr(Pos+1, string::npos);
        CriticalSectionLocker CSL(CS);
        Curl[Field]=Value;
    }
}

void MediaInfo_Config_MediaInfo::File_Curl_Set (const Ztring &Field_, const Ztring &NewValue)
{
    Ztring Field=Field_; Field.MakeLowerCase();
    CriticalSectionLocker CSL(CS);
    Curl[Field]=NewValue;
}

Ztring MediaInfo_Config_MediaInfo::File_Curl_Get (const Ztring &Field_)
{
    Ztring Field=Field_; Field.MakeLowerCase();
    CriticalSectionLocker CSL(CS);
    std::map<Ztring, Ztring>::iterator Value=Curl.find(Field);
    if (Value==Curl.end())
        return Ztring();
    else
        return Curl[Field];
}
#endif //defined(MEDIAINFO_LIBCURL_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_LIBMMS_YES)
void MediaInfo_Config_MediaInfo::File_Mmsh_Describe_Only_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Mmsh_Describe_Only=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Mmsh_Describe_Only_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Mmsh_Describe_Only;
}
#endif //defined(MEDIAINFO_LIBMMS_YES)

//---------------------------------------------------------------------------
Ztring MediaInfo_Config_MediaInfo::File_DisplayCaptions_Set (const Ztring& NewValue)
{
    auto NewValueS = NewValue.To_UTF8();
    size_t i = 0;
    for (; i < DisplayCaptions_Max; i++) {
        if (NewValueS == DisplayCaptions_Strings[i]) {
            break;
        }
    }
    if (i >= DisplayCaptions_Max)
        return __T("Unknown value");

    CriticalSectionLocker CSL(CS);
    DisplayCaptions = (display_captions)i;

    return {};
}

display_captions MediaInfo_Config_MediaInfo::File_DisplayCaptions_Get ()
{
    CriticalSectionLocker CSL(CS);
    return DisplayCaptions;
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Config_MediaInfo::File_ProbeCaption_Set (const Ztring& NewValue)
{
    static const auto Malformed = __T("File_ProbeCaption option malformed");
    ZtringListList List;
    List.Separator_Set(0, __T(","));
    List.Separator_Set(1, __T("+"));
    List.Write(NewValue);
    CriticalSectionLocker CSL(CS);
    File_ProbeCaption.clear();
    bool HasForAll = false;
    string AllMinus;
    for (const auto& Line : List) {
        config_probe Item;
        for (const auto& Value : Line) {
            auto Pos = Value.find_first_not_of(__T("0123456789"));
            if (Pos == string::npos) {
                Item.Start_Type = config_probe_size;
                Item.Start = Value.To_int64u();
            }
            else if (!Value.empty() && ((Value[0] >= __T('A') && Value[0] <= __T('Z')) || (Value[0] >= __T('a') && Value[0] <= __T('z')))) {
                string Value2(Ztring(Value).MakeUpperCase().To_UTF8());
                bool Negative = false;
                if (Value2.rfind("ALL", 0) == 0) {
                    if (Value2.size() > 3) {
                        if (Value2[3] != __T('-'))
                            return Malformed;
                        Negative = true;
                        Value2 = Value.To_UTF8().substr(4);
                        AllMinus += '-';
                        AllMinus += Value2;
                    }
                }
                if (Value2 != "MP4" && Value2 != "MPEG-4" && Value2 != "MXF" && !Value2.empty()) { //TODO: full list
                    return Malformed;
                }
                Item.Parser = Value2 != "MP4" ? Value2 : "MPEG-4";
                if (Negative) {
                    Item.Parser.insert(Item.Parser.begin(), __T('-'));
                }
            }
            else if (Pos == Value.size() - 1 && Value[Pos] == __T('%')) {
                auto Value_Int = Value.To_int64u();
                if (Value_Int > 100) {
                    return Malformed;
                }
                if (Item.Start_Type == config_probe_none) {
                    Item.Start_Type = config_probe_percent;
                    Item.Start = Value_Int;
                }
                else if (Item.Duration_Type == config_probe_none) {
                    Item.Duration_Type = config_probe_percent;
                    Item.Duration = Value_Int;
                }
                else {
                    return Malformed;
                }
            }
            else if (Pos == Value.size() - 1 && (Value[Pos] == __T('E') || Value[Pos] == __T('G') || Value[Pos] == __T('K') || Value[Pos] == __T('M') || Value[Pos] == __T('P') || Value[Pos] == __T('T') || Value[Pos] == __T('k'))) {
                auto Value_Int = Value.To_int64u();
                switch (Value[Pos])
                {
                case 'E':
                    Value_Int <<= 10;
                    // Fall through
                case 'P':
                    Value_Int <<= 10;
                    // Fall through
                case 'T':
                    Value_Int <<= 10;
                    // Fall through
                case 'G':
                    Value_Int <<= 10;
                    // Fall through
                case 'M':
                    Value_Int <<= 10;
                    // Fall through
                default:
                    Value_Int <<= 10;
                }
                if (Item.Start_Type == config_probe_none) {
                    Item.Start_Type = config_probe_size;
                    Item.Start = Value_Int;
                }
                else if (Item.Duration_Type == config_probe_none) {
                    Item.Duration_Type = config_probe_size;
                    Item.Duration = Value_Int;
                }
                else {
                    return Malformed;
                }
            }
            else {
                TimeCode TC(Value.To_UTF8(), 999);
                if (!TC.IsValid()) {
                    return Malformed;
                }
                auto Seconds = TC.ToSeconds();
                if (Seconds <= 0) {
                    return Malformed;
                }
                if (Item.Start_Type == config_probe_none) {
                    Item.Start_Type = config_probe_dur;
                    Item.Start = Seconds;
                }
                else if (Item.Duration_Type == config_probe_none) {
                    Item.Duration_Type = config_probe_dur;
                    Item.Duration = Seconds;
                }
                else {
                    return Malformed;
                }
            }
        }
        if (Item.Parser.empty()) {
            HasForAll=true;
        }
        if (Item.Duration_Type == config_probe_none) {
            Item.Duration_Type = config_probe_dur;
            Item.Duration = 30;
        }
        File_ProbeCaption.push_back(Item);
    }
    File_ProbeCaption_Pos=0;
    if (!HasForAll) {
        config_probe Probe;
        Probe.Start_Type = config_probe_percent;
        Probe.Start = 50;
        Probe.Duration_Type = config_probe_dur;
        Probe.Duration = 30;
        Probe.Parser = AllMinus;
        File_ProbeCaption.push_back(Probe);
    }

    return {};
}

config_probe MediaInfo_Config_MediaInfo::File_ProbeCaption_Get(const string& ParserName)
{
    if (ParseSpeed<=0 && ParseSpeed>=1)
        return {};

    CriticalSectionLocker CSL(CS);
    for (;;)
    {
        if (File_ProbeCaption_Pos >= File_ProbeCaption.size())
            return {};
        const auto& Item = File_ProbeCaption[File_ProbeCaption_Pos];
        File_ProbeCaption_Pos++;
        if (Item.Parser.empty()) {
            if (Item.Parser[0] == '-') {
                if (Item.Parser.rfind(ParserName, 1) == 1) {
                    continue;
                }
            }
        }
        else if (Item.Parser != ParserName) {
            continue;
        }

        return Item;
    }
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AC3_YES)
void MediaInfo_Config_MediaInfo::File_Ac3_IgnoreCrc_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    File_Ac3_IgnoreCrc=NewValue;
}

bool MediaInfo_Config_MediaInfo::File_Ac3_IgnoreCrc_Get ()
{
    CriticalSectionLocker CSL(CS);
    return File_Ac3_IgnoreCrc;
}
#endif //defined(MEDIAINFO_AC3_YES)

//***************************************************************************
// Analysis internal
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config_MediaInfo::State_Set (float NewValue)
{
    CriticalSectionLocker CSL(CS);
    State=NewValue;
}

float MediaInfo_Config_MediaInfo::State_Get ()
{
    CriticalSectionLocker CSL(CS);
    return State;
}

} //NameSpace
