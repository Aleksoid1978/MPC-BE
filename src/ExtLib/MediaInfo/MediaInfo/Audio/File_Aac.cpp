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
#if defined(MEDIAINFO_AAC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Aac.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <string>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

extern const char* Aac_audioObjectType(int8u audioObjectType);
extern string Mpeg4_Descriptors_AudioProfileLevelIndicationString(const profilelevel_struct& ProfileLevel);

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Aac::File_Aac()
:File_Usac(), File__Tags_Helper()
{
    //Config
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Aac;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS

    //File__Tags_Helper
    Base=this;

    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=64*1024;
    PTS_DTS_Needed=true;
    StreamSource=IsStream;

    //In
    Frame_Count_Valid=0;
    FrameIsAlwaysComplete=false;
    Mode=Mode_Unknown;
    
    //Conformance
    #if MEDIAINFO_CONFORMANCE
        SamplingRate=0;
    #endif

    audioObjectType=(int8u)-1;
    extensionAudioObjectType=(int8u)-1;
    frame_length=1024;
    extension_sampling_frequency=(int32u)-1;
    aacSpectralDataResilienceFlag=false;
    aacSectionDataResilienceFlag=false;
    aacScalefactorDataResilienceFlag=false;
    FrameSize_Min=(int64u)-1;
    FrameSize_Max=0;
    adts_buffer_fullness_Is7FF=false;
    #if MEDIAINFO_ADVANCED
        aac_frame_length_Total=0;
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_MACROBLOCKS
        ParseCompletely=0;
    #endif //MEDIAINFO_MACROBLOCKS

    //Temp - Main
    muxConfigPresent=true;
    audioMuxVersionA=false;

    //Temp - General Audio
    sbr=NULL;
    ps=NULL;
    raw_data_block_Pos=0;
    ChannelPos_Temp=0;
    ChannelCount_Temp=0;

    //Temp
    CanFill=true;
}

//---------------------------------------------------------------------------
File_Aac::~File_Aac()
{
    for (size_t i=0; i<sbrs.size(); i++)
        delete sbrs[i];
    for (size_t i=0; i<pss.size(); i++)
        delete pss[i];
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Aac::Streams_Accept()
{
    switch (Mode)
    {
         case Mode_ADTS :
                       if (!IsSub)
                            TestContinuousFileNames();
                       break;
        default : ;
    }
    if (Frame_Count_NotParsedIncluded==(int64u)-1)
        Frame_Count_NotParsedIncluded=0;
}

//---------------------------------------------------------------------------
void File_Aac::Streams_Fill()
{
    switch(Mode)
    {
        case Mode_LATM : Fill(Stream_General, 0, General_Format, "LATM"); if (IsSub) Fill(Stream_Audio, 0, Audio_MuxingMode, "LATM"); break;
        default : ;
    }

    for (std::map<std::string, Ztring>::iterator Info=Infos_General.begin(); Info!=Infos_General.end(); ++Info)
        Fill(Stream_General, 0, Info->first.c_str(), Info->second);
    File__Tags_Helper::Stream_Prepare(Stream_Audio);
    for (std::map<std::string, Ztring>::iterator Info_AudioSpecificConfig=Infos_AudioSpecificConfig.begin(); Info_AudioSpecificConfig!=Infos_AudioSpecificConfig.end(); ++Info_AudioSpecificConfig)
        if (Infos.find(Info_AudioSpecificConfig->first)==Infos.end())
            Infos[Info_AudioSpecificConfig->first]=Info_AudioSpecificConfig->second; // Use AudioSpecificConfig info if info not present in stream itself
    for (std::map<std::string, Ztring>::iterator Info=Infos.begin(); Info!=Infos.end(); ++Info)
        Fill(Stream_Audio, StreamPos_Last, Info->first.c_str(), Info->second);

    switch(Mode)
    {
        case Mode_ADTS    : File__Tags_Helper::Streams_Fill(); break;
        default           : ;
    }

    if (Retrieve_Const(Stream_Audio, StreamPos_Last, Audio_SamplesPerFrame).empty())
    {
    int16u frame_length_Multiplier=1;
    if (!MediaInfoLib::Config.LegacyStreamDisplay_Get() && Retrieve_Const(Stream_Audio, StreamPos_Last, Audio_Format).find(__T("AAC"))==0 && Retrieve_Const(Stream_Audio, StreamPos_Last, Audio_Format_Settings_SBR).find(__T("Yes"))==0)
        frame_length_Multiplier=2;
    Fill(Stream_Audio, StreamPos_Last, Audio_SamplesPerFrame, frame_length*frame_length_Multiplier);
    }
}

//---------------------------------------------------------------------------
void File_Aac::Streams_Update()
{
    if (Frame_Count)
    {
        if (Mode==Mode_ADTS)
            Infos["BitRate_Mode"].From_UTF8(adts_buffer_fullness_Is7FF?"VBR":"CBR");

        #if MEDIAINFO_ADVANCED
            switch(Mode)
            {
                case Mode_ADTS    :
                case Mode_LATM    : if (Config->File_RiskyBitRateEstimation_Get() && !adts_buffer_fullness_Is7FF && (Config->ParseSpeed<1.0 || File_Offset+Buffer_Offset<File_Size))
                                    {
                                        float64 BitRate=((float64)Frequency_b)/frame_length;
                                        BitRate*=aac_frame_length_Total*8;
                                        BitRate/=Frame_Count;

                                        Fill(Stream_Audio, 0, Audio_BitRate, BitRate, 10, true);
                                        Fill(Stream_Audio, 0, Audio_Duration, (File_Size-Buffer_TotalBytes_FirstSynched)/BitRate*8*1000, 0, true);
                                    }
                                    break;
                default           : ;
            }
        #endif //MEDIAINFO_ADVANCED
    }
}

//---------------------------------------------------------------------------
void File_Aac::Streams_Finish()
{
    switch(Mode)
    {
        case Mode_ADIF    :
        case Mode_ADTS    : File__Tags_Helper::Streams_Finish(); break;
        default           : ;
    }

    if (FrameSize_Min!=(int32u)-1 && FrameSize_Max)
    {
        if (FrameSize_Max>FrameSize_Min*1.02)
        {
            Fill(Stream_Audio, 0, Audio_BitRate_Mode, "VBR", Unlimited, true, true);
            if (Config->ParseSpeed>=1.0)
            {
                Fill(Stream_Audio, 0, Audio_BitRate_Minimum, ((float64)FrameSize_Min)/frame_length*Frequency_b*8, 0);
                Fill(Stream_Audio, 0, Audio_BitRate_Maximum, ((float64)FrameSize_Max)/frame_length*Frequency_b*8, 0);
                Fill(Stream_Audio, 0, Audio_SamplingCount, Frame_Count*frame_length);
                Fill(Stream_Audio, 0, Audio_Duration, ((float64)Frame_Count)*frame_length/Frequency_b*1000, 0);
            }
        }
        else if (Config->ParseSpeed>=1.0)
        {
            Fill(Stream_Audio, 0, Audio_BitRate_Mode, "CBR");
        }
    }

    if (Mode==Mode_ADTS && !ChannelCount_Temp && ChannelPos_Temp && Retrieve_Const(Stream_Audio, 0, Audio_Channel_s_).empty())
        Fill(Stream_Audio, 0, Audio_Channel_s_, ChannelPos_Temp);

    #if MEDIAINFO_CONFORMANCE
        if (audioObjectType==42 && !ConformanceFlags) {
            ConformanceFlags.set(Usac);
        }
        if (Retrieve_Const(Stream_Audio, 0, "ConformanceErrors").empty() && Retrieve_Const(Stream_Audio, 0, "ConformanceWarnings").empty() && Retrieve_Const(Stream_Audio, 0, "ConformanceInfos").empty()) { //TODO: check why called twice in some cases
        if (ProfileLevel.profile != UnknownAudio && ProfileLevel.profile != UnspecifiedAudio
            && ((audioObjectType == 42 && !ConformanceFlags[BaselineUsac] && !ConformanceFlags[xHEAAC])
             || (audioObjectType != 42 && (ConformanceFlags[BaselineUsac] || ConformanceFlags[xHEAAC])))) {
            string ProfileLevelString = Mpeg4_Descriptors_AudioProfileLevelIndicationString(ProfileLevel);
            auto audioObjectTypeString = to_string(audioObjectType);
            auto audioObjectTypeTemp = Aac_audioObjectType(audioObjectType);
            if (audioObjectTypeTemp && *audioObjectTypeTemp) {
                audioObjectTypeString += " (";
                audioObjectTypeString += audioObjectTypeTemp;
                audioObjectTypeString += ')';
            }
            Fill_Conformance("Crosscheck InitialObjectDescriptor audioProfileLevelIndication", ("MP4 InitialObjectDescriptor audioProfileLevelIndication " + ProfileLevelString + " does not permit MP4 AudioSpecificConfig audioObjectType " + audioObjectTypeString).c_str(), bitset8().set(Usac).set(BaselineUsac).set(xHEAAC));
        }
        Streams_Finish_Conformance();
        }
    #endif
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Aac::FileHeader_Begin()
{
    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.5?128:(Config->ParseSpeed>=0.3?32:8);

    switch(Mode)
    {
        case Mode_AudioSpecificConfig :
        case Mode_ADIF                :
                                        MustSynchronize=false; break;
        default                       : ; //Synchronization is requested, and this is the default
    }

    switch(Mode)
    {
        case Mode_Unknown             :
        case Mode_ADIF                :
        case Mode_ADTS                :
                                        break;
        default                       : return true; //no file header test with other modes
    }

    //Tags
    if (!File__Tags_Helper::FileHeader_Begin())
        return false;

    //Testing
    if (Buffer_Size<4)
        return false;
    if (Buffer[0]==0x41 //"ADIF"
     && Buffer[1]==0x44
     && Buffer[2]==0x49
     && Buffer[3]==0x46)
    {
        Mode=Mode_ADIF;
        File__Tags_Helper::Accept("ADIF");
        MustSynchronize=false;
    }
    else if (Mode==Mode_ADIF)
        File__Tags_Helper::Reject("ADIF");

    return true;
}

//---------------------------------------------------------------------------
void File_Aac::FileHeader_Parse()
{
    switch (Mode)
    {
        case Mode_ADIF    : FileHeader_Parse_ADIF(); break;
        default           : ; //no file header test with other modes
    }
}

//---------------------------------------------------------------------------
void File_Aac::FileHeader_Parse_ADIF()
{
    adif_header();
    Mode=Mode_payload;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Aac::Read_Buffer_Init()
{
    File_Usac::Read_Buffer_Init();
}

//---------------------------------------------------------------------------
void File_Aac::Read_Buffer_Continue()
{
    if (Element_Size==0)
        return;

    if (Frame_Count==0)
    {
        PTS_Begin=FrameInfo.PTS;
        #if MEDIAINFO_MACROBLOCKS
            ParseCompletely=Config->File_Macroblocks_Parse_Get();
        #endif //MEDIAINFO_MACROBLOCKS
    }

    switch(Mode)
    {
        case Mode_AudioSpecificConfig : Read_Buffer_Continue_AudioSpecificConfig(); break;
        case Mode_payload             : Read_Buffer_Continue_payload(); break;
        case Mode_ADIF                :
        case Mode_LATM:
        case Mode_ADTS                : File__Tags_Helper::Read_Buffer_Continue(); break;
        default                       : if (Frame_Count)
                                            File__Tags_Helper::Finish();
    }
}

//---------------------------------------------------------------------------
void File_Aac::Read_Buffer_Continue_AudioSpecificConfig()
{
    File__Analyze::Accept(); //We automaticly trust it

    BS_Begin();
    AudioSpecificConfig(0); //Up to the end of the block
    BS_End();

    Infos_AudioSpecificConfig=Infos;
    Mode=Mode_payload; //Mode_AudioSpecificConfig only once
}

//---------------------------------------------------------------------------
void File_Aac::Read_Buffer_Continue_payload()
{
    BS_Begin();
    payload();
    BS_End();
    if (FrameIsAlwaysComplete && Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "Unknown");

    FILLING_BEGIN();
        //Counting
        Frame_Count++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;
        Element_Info1(Ztring::ToZtring(Frame_Count));

        //Filling
        if (!Status[IsAccepted])
            File__Analyze::Accept();
        if (Frame_Count>=Frame_Count_Valid)
        {
            //No more need data
            if (Mode==Mode_LATM)
                File__Analyze::Accept();
            File__Analyze::Fill();
            if (Config->ParseSpeed<1.0)
            {
                Open_Buffer_Unsynch();
                if (!IsSub && Mode!=Mode_LATM) //Mainly for knowinf it is ADIF, which has sometimes tags at the end
                {
                    Mode=Mode_Unknown;
                    File__Tags_Helper::Finish();
                }
                else
                    File__Analyze::Finish();
            }
        }
    FILLING_ELSE();
        Infos=Infos_AudioSpecificConfig;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Aac::Read_Buffer_Unsynched()
{
    FrameInfo=frame_info();
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Aac::Synchronize()
{
    switch (Mode)
    {
        case Mode_Unknown     : if (Synchronize_LATM()) return true; Buffer_Offset=0; return Synchronize_ADTS();
        case Mode_ADTS        : return Synchronize_ADTS();
        case Mode_LATM        : return Synchronize_LATM();
        default               : return true; //No synchro
    }
}

//---------------------------------------------------------------------------
bool File_Aac::Synchronize_ADTS()
{
    //Tags
    bool Tag_Found;
    if (!File__Tags_Helper::Synchronize(Tag_Found))
        return false;
    if (Tag_Found)
        return true;

    //Synchronizing
    while (Buffer_Offset+6<=Buffer_Size)
    {
         while (Buffer_Offset+6<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0xFF
                                              || (Buffer[Buffer_Offset+1]&0xF6)!=0xF0))
            Buffer_Offset++;

        if (Buffer_Offset+6<=Buffer_Size)//Testing if size is coherant
        {
            //Testing next start, to be sure
            int8u sampling_frequency_index=(CC1(Buffer+Buffer_Offset+2)>>2)&0xF;
            if (sampling_frequency_index>=Aac_sampling_frequency_Size)
            {
                Buffer_Offset++;
                continue;
            }
            int16u aac_frame_length=(CC3(Buffer+Buffer_Offset+3)>>5)&0x1FFF;
            if (IsSub && Buffer_Offset+aac_frame_length==Buffer_Size)
                break;
            if (File_Offset+Buffer_Offset+aac_frame_length!=File_Size-File_EndTagSize)
            {
                //Padding
                while (Buffer_Offset+aac_frame_length+2<=Buffer_Size && Buffer[Buffer_Offset+aac_frame_length]==0x00)
                    aac_frame_length++;

                if (IsSub && Buffer_Offset+aac_frame_length==Buffer_Size)
                    break; //while()
                if (Buffer_Offset+aac_frame_length+2>Buffer_Size)
                    return false; //Need more data

                //Testing
                if (aac_frame_length<=7 || (CC2(Buffer+Buffer_Offset+aac_frame_length)&0xFFF6)!=0xFFF0)
                    Buffer_Offset++;
                else
                {
                    //Testing next start, to be sure
                    if (Buffer_Offset+aac_frame_length+3+3>Buffer_Size)
                        return false; //Need more data
                    sampling_frequency_index=(CC1(Buffer+Buffer_Offset+aac_frame_length+2)>>2)&0xF;
                    if (sampling_frequency_index>=Aac_sampling_frequency_Size)
                    {
                        Buffer_Offset++;
                        continue;
                    }
                    int16u aac_frame_length2=(CC3(Buffer+Buffer_Offset+aac_frame_length+3)>>5)&0x1FFF;
                    if (File_Offset+Buffer_Offset+aac_frame_length+aac_frame_length2!=File_Size-File_EndTagSize)
                    {
                        //Padding
                        while (Buffer_Offset+aac_frame_length+aac_frame_length2+2<=Buffer_Size && Buffer[Buffer_Offset+aac_frame_length+aac_frame_length2]==0x00)
                            aac_frame_length2++;

                        if (IsSub && Buffer_Offset+aac_frame_length+aac_frame_length2==Buffer_Size)
                            break; //while()
                        if (Buffer_Offset+aac_frame_length+aac_frame_length2+2>Buffer_Size)
                            return false; //Need more data

                        //Testing
                        if (aac_frame_length2<=7 || (CC2(Buffer+Buffer_Offset+aac_frame_length+aac_frame_length2)&0xFFF6)!=0xFFF0)
                            Buffer_Offset++;
                        else
                        {
                            //Testing next start, to be sure
                            if (Buffer_Offset+aac_frame_length+aac_frame_length2+3+3>Buffer_Size)
                                return false; //Need more data
                            sampling_frequency_index=(CC1(Buffer+Buffer_Offset+aac_frame_length+aac_frame_length2+2)>>2)&0xF;
                            if (sampling_frequency_index>=Aac_sampling_frequency_Size)
                            {
                                Buffer_Offset++;
                                continue;
                            }
                            int16u aac_frame_length3=(CC3(Buffer+Buffer_Offset+aac_frame_length+aac_frame_length2+3)>>5)&0x1FFF;
                            if (File_Offset+Buffer_Offset+aac_frame_length+aac_frame_length2+aac_frame_length3!=File_Size-File_EndTagSize)
                            {
                                //Padding
                                while (Buffer_Offset+aac_frame_length+aac_frame_length2+aac_frame_length3+2<=Buffer_Size && Buffer[Buffer_Offset+aac_frame_length+aac_frame_length2+aac_frame_length3]==0x00)
                                    aac_frame_length3++;

                                if (IsSub && Buffer_Offset+aac_frame_length+aac_frame_length2+aac_frame_length3==Buffer_Size)
                                    break; //while()
                                if (Buffer_Offset+aac_frame_length+aac_frame_length2+aac_frame_length3+2>Buffer_Size)
                                    return false; //Need more data

                                //Testing
                                if (aac_frame_length3<=7 || (CC2(Buffer+Buffer_Offset+aac_frame_length+aac_frame_length2+aac_frame_length3)&0xFFF6)!=0xFFF0)
                                    Buffer_Offset++;
                                else
                                    break; //while()
                            }
                            else
                                break; //while()
                        }
                    }
                    else
                        break; //while()
                }
            }
            else
                break; //while()
        }
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+6>Buffer_Size)
    {
        if (Buffer_Offset+5==Buffer_Size && (CC2(Buffer+Buffer_Offset)&0xFFF6)!=0xFFF0)
            Buffer_Offset++;
        if (Buffer_Offset+4==Buffer_Size && (CC2(Buffer+Buffer_Offset)&0xFFF6)!=0xFFF0)
            Buffer_Offset++;
        if (Buffer_Offset+3==Buffer_Size && (CC2(Buffer+Buffer_Offset)&0xFFF6)!=0xFFF0)
            Buffer_Offset++;
        if (Buffer_Offset+2==Buffer_Size && (CC2(Buffer+Buffer_Offset)&0xFFF6)!=0xFFF0)
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && CC1(Buffer+Buffer_Offset)!=0xFF)
            Buffer_Offset++;
        return false;
    }

    //Synched is OK
    Mode=Mode_ADTS;
    File__Analyze::Accept();
    return true;
}

//---------------------------------------------------------------------------
bool File_Aac::Synchronize_LATM()
{
    //Synchronizing
    while (Buffer_Offset+3<=Buffer_Size)
    {
         while (Buffer_Offset+3<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0x56
                                              || (Buffer[Buffer_Offset+1]&0xE0)!=0xE0))
            Buffer_Offset++;

        if (Buffer_Offset+3<=Buffer_Size)//Testing if size is coherant
        {
            //Testing next start, to be sure
            int16u audioMuxLengthBytes=CC2(Buffer+Buffer_Offset+1)&0x1FFF;
            if (IsSub && Buffer_Offset+3+audioMuxLengthBytes==Buffer_Size)
                break;
            if (File_Offset+Buffer_Offset+3+audioMuxLengthBytes!=File_Size)
            {
                if (Buffer_Offset+3+audioMuxLengthBytes+3>Buffer_Size)
                    return false; //Need more data

                //Testing
                if ((CC2(Buffer+Buffer_Offset+3+audioMuxLengthBytes)&0xFFE0)!=0x56E0)
                    Buffer_Offset++;
                else
                {
                    //Testing next start, to be sure
                    int16u audioMuxLengthBytes2=CC2(Buffer+Buffer_Offset+3+audioMuxLengthBytes+1)&0x1FFF;
                    if (File_Offset+Buffer_Offset+3+audioMuxLengthBytes+3+audioMuxLengthBytes2!=File_Size)
                    {
                        if (Buffer_Offset+3+audioMuxLengthBytes+3+audioMuxLengthBytes2+3>Buffer_Size)
                            return false; //Need more data

                        //Testing
                        if ((CC2(Buffer+Buffer_Offset+3+audioMuxLengthBytes+3+audioMuxLengthBytes2)&0xFFE0)!=0x56E0)
                            Buffer_Offset++;
                        else
                            break; //while()
                    }
                    else
                        break; //while()
                }
            }
            else
                break; //while()
        }
    }


    //Synchronizing
    while (Buffer_Offset+2<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0x56
                                         || (Buffer[Buffer_Offset+1]&0xE0)!=0xE0))
        Buffer_Offset++;
    if (Buffer_Offset+2>=Buffer_Size)
        return false;

    //Synched is OK
    Mode=Mode_LATM;
    return true;
}

//---------------------------------------------------------------------------
bool File_Aac::Synched_Test()
{
    switch (Mode)
    {
        case Mode_ADTS        : return Synched_Test_ADTS();
        case Mode_LATM        : return Synched_Test_LATM();
        default               : return true; //No synchro
    }
}

//---------------------------------------------------------------------------
bool File_Aac::Synched_Test_ADTS()
{
    //Tags
    if (!File__Tags_Helper::Synched_Test())
        return false;

    //Null padding
    while (Buffer_Offset+2<=Buffer_Size && Buffer[Buffer_Offset]==0x00)
        Buffer_Offset++;

    //Must have enough buffer for having header
    if (Buffer_Offset+2>Buffer_Size)
        return false;

    //Quick test of synchro
    if ((CC2(Buffer+Buffer_Offset)&0xFFF6)!=0xFFF0)
        Synched=false;

    //We continue
    return true;
}

//---------------------------------------------------------------------------
bool File_Aac::Synched_Test_LATM()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+2>Buffer_Size)
        return false;

    //Quick test of synchro
    if ((CC2(Buffer+Buffer_Offset)&0xFFE0)!=0x56E0)
        Synched=false;

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Demux
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
bool File_Aac::Demux_UnpacketizeContainer_Test()
{
    switch (Mode)
    {
        case Mode_ADTS        : return Demux_UnpacketizeContainer_Test_ADTS();
        case Mode_LATM        : return Demux_UnpacketizeContainer_Test_LATM();
        default               : return true; //No header
    }
}
bool File_Aac::Demux_UnpacketizeContainer_Test_ADTS()
{
    int16u aac_frame_length=(BigEndian2int24u(Buffer+Buffer_Offset+3)>>5)&0x1FFF; //13 bits
    Demux_Offset=Buffer_Offset+aac_frame_length;

    if (Demux_Offset>Buffer_Size && File_Offset+Buffer_Size!=File_Size)
        return false; //No complete frame

    Demux_UnpacketizeContainer_Demux();

    return true;
}
bool File_Aac::Demux_UnpacketizeContainer_Test_LATM()
{
    int16u audioMuxLengthBytes=BigEndian2int16u(Buffer+Buffer_Offset+1)&0x1FFF; //13 bits
    Demux_Offset=Buffer_Offset+3+audioMuxLengthBytes;

    if (Demux_Offset>Buffer_Size && File_Offset+Buffer_Size!=File_Size)
        return false; //No complete frame

    Demux_UnpacketizeContainer_Demux();

    return true;
}
#endif //MEDIAINFO_DEMUX

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Aac::Header_Begin()
{
    switch (Mode)
    {
        case Mode_ADTS        : return Header_Begin_ADTS();
        case Mode_LATM        : return Header_Begin_LATM();
        default               : return true; //No header
    }
}

//---------------------------------------------------------------------------
bool File_Aac::Header_Begin_ADTS()
{
    //There is no real header in ADTS, retrieving only the frame length
    if (Buffer_Offset+8>Buffer_Size) //size of adts_fixed_header + adts_variable_header
        return false;

    return true;
}

//---------------------------------------------------------------------------
bool File_Aac::Header_Begin_LATM()
{
    if (Buffer_Offset+3>Buffer_Size) //fixed 24-bit header
        return false;

    return true;
}

//---------------------------------------------------------------------------
void File_Aac::Header_Parse()
{
    switch (Mode)
    {
        case Mode_ADTS        : Header_Parse_ADTS(); break;
        case Mode_LATM        : Header_Parse_LATM(); break;
        default               : ; //No header
    }
}

//---------------------------------------------------------------------------
void File_Aac::Header_Parse_ADTS()
{
    //There is no "header" in ADTS, retrieving only the frame length
    int16u aac_frame_length=(BigEndian2int24u(Buffer+Buffer_Offset+3)>>5)&0x1FFF; //13 bits

    //Filling
    Header_Fill_Size(aac_frame_length);
    Header_Fill_Code(0, "adts_frame");
}

//---------------------------------------------------------------------------
void File_Aac::Header_Parse_LATM()
{
    int16u audioMuxLengthBytes;
    BS_Begin();
    Skip_S2(11,                                                 "syncword");
    Get_S2 (13, audioMuxLengthBytes,                            "audioMuxLengthBytes");
    BS_End();

    //Filling
    Header_Fill_Size(3+audioMuxLengthBytes);
    Header_Fill_Code(0, "LATM");
}

//---------------------------------------------------------------------------
void File_Aac::Data_Parse()
{
    if (FrameSize_Min>Header_Size+Element_Size)
        FrameSize_Min=Header_Size+Element_Size;
    if (FrameSize_Max<Header_Size+Element_Size)
        FrameSize_Max=Header_Size+Element_Size;

    switch (Mode)
    {
        case Mode_ADTS        : Data_Parse_ADTS(); break;
        case Mode_LATM        : Data_Parse_LATM(); break;
        default               : ; //No header
    }

    FILLING_BEGIN();
        //Counting
        if (File_Offset+Buffer_Offset+Element_Size==File_Size)
            Frame_Count_Valid=Frame_Count; //Finish frames in case of there are less than Frame_Count_Valid frames

        #if MEDIAINFO_ADVANCED
            switch(Mode)
            {
                case Mode_LATM    :
                                    aac_frame_length_Total+=Element_Size;
                                    break;
                default           : ;
            }
        #endif //MEDIAINFO_ADVANCED

        if (!Status[IsAccepted])
            File__Analyze::Accept();

        //Filling
        TS_Add(frame_length);
        if (Frame_Count>=Frame_Count_Valid && Config->ParseSpeed<1.0)
        {
            //No more need data
            switch (Mode)
            {
                case Mode_ADTS        :
                case Mode_LATM        :
                                        if (!Status[IsFilled])
                                        {
                                            Fill();
                                            if (File_Offset+Buffer_Offset+Element_Size!=File_Size)
                                                Open_Buffer_Unsynch();
                                            if (!IsSub)
                                                File__Tags_Helper::Finish();
                                        }
                                        break;
                default               : ; //No header
            }

        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Aac::Data_Parse_ADTS()
{
    //Parsing
    BS_Begin();
    adts_frame();
    BS_End();
}

//---------------------------------------------------------------------------
void File_Aac::Data_Parse_LATM()
{
    BS_Begin();
    AudioMuxElement();
    BS_End();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_AAC_YES
