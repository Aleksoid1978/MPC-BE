/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Descriptors part
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
#ifdef MEDIAINFO_MPEG4_YES
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Mpeg4_Descriptors.h"
#include <cstring>
#if defined(MEDIAINFO_OGG_YES)
    #include "MediaInfo/Multiple/File_Ogg.h"
#endif
#if defined(MEDIAINFO_AVC_YES)
    #include "MediaInfo/Video/File_Avc.h"
#endif
#if defined(MEDIAINFO_VC1_YES)
    #include "MediaInfo/Video/File_Vc1.h"
#endif
#if defined(MEDIAINFO_DIRAC_YES)
    #include "MediaInfo/Video/File_Dirac.h"
#endif
#if defined(MEDIAINFO_MPEG4V_YES)
    #include "MediaInfo/Video/File_Mpeg4v.h"
#endif
#if defined(MEDIAINFO_MPEGV_YES)
    #include "MediaInfo/Video/File_Mpegv.h"
#endif
#if defined(MEDIAINFO_JPEG_YES)
    #include "MediaInfo/Image/File_Jpeg.h"
#endif
#if defined(MEDIAINFO_PNG_YES)
    #include "MediaInfo/Image/File_Png.h"
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
#if MEDIAINFO_DEMUX
    #include "MediaInfo/MediaInfo_Config_MediaInfo.h"
    #include "ThirdParty/base64/base64.h"
#endif //MEDIAINFO_DEMUX
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_MPEG4V_YES
    const char* Mpeg4v_Profile_Level(int32u Profile_Level);
#endif //MEDIAINFO_MPEG4V_YES
//---------------------------------------------------------------------------

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Mpeg4_Descriptors_Predefined(int8u ID)
{
    switch (ID)
    {
        case 0x00 : return "Custom";
        case 0x01 : return "null SL packet header";
        case 0x02 : return "Reserved for use in MP4 files";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mpeg4_Descriptors_ObjectTypeIndication(int8u ID)
{
    switch (ID)
    {
        case 0x01 : return "Systems ISO/IEC 14496-1";
        case 0x02 : return "Systems ISO/IEC 14496-1 (v2)";
        case 0x03 : return "Interaction Stream";
        case 0x05 : return "AFX Stream";
        case 0x06 : return "Font Data Stream";
        case 0x07 : return "Synthesized Texture Stream";
        case 0x08 : return "Streaming Text Stream";
        case 0x20 : return "Visual ISO/IEC 14496-2 (MPEG-4 Visual)";
        case 0x21 : return "Visual ISO/IEC 14496-10 (AVC)";
        case 0x22 : return "Parameter Sets for Visual ISO/IEC 14496-10 (AVC)";
        case 0x24 : return "ALS"; //Not sure
        case 0x2B : return "SAOC"; //Not sure
        case 0x40 : return "Audio ISO/IEC 14496-3 (AAC)";
        case 0x60 : return "Visual ISO/IEC 13818-2 Simple Profile (MPEG Video)";
        case 0x61 : return "Visual ISO/IEC 13818-2 Main Profile (MPEG Video)";
        case 0x62 : return "Visual ISO/IEC 13818-2 SNR Profile (MPEG Video)";
        case 0x63 : return "Visual ISO/IEC 13818-2 Spatial Profile (MPEG Video)";
        case 0x64 : return "Visual ISO/IEC 13818-2 High Profile (MPEG Video)";
        case 0x65 : return "Visual ISO/IEC 13818-2 422 Profile (MPEG Video)";
        case 0x66 : return "Audio ISO/IEC 13818-7 Main Profile (AAC)";
        case 0x67 : return "Audio ISO/IEC 13818-7 Low Complexity Profile (AAC)";
        case 0x68 : return "Audio ISO/IEC 13818-7 Scaleable Sampling Rate Profile (AAC)";
        case 0x69 : return "Audio ISO/IEC 13818-3 (MPEG Audio)";
        case 0x6A : return "Visual ISO/IEC 11172-2 (MPEG Video)";
        case 0x6B : return "Audio ISO/IEC 11172-3 (MPEG Audio)";
        case 0x6C : return "Visual ISO/IEC 10918-1 (JPEG)";
        case 0x6D : return "PNG";
        case 0xA0 : return "EVRC";
        case 0xA1 : return "SMV";
        case 0xA2 : return "3GPP2 Compact Multimedia Format (CMF)";
        case 0xA3 : return "VC-1";
        case 0xA4 : return "Dirac";
        case 0xA5 : return "AC-3";
        case 0xA6 : return "E-AC-3";
        case 0xA9 : return "DTS";
        case 0xAA : return "DTS-HD High Resolution Audio";
        case 0xAB : return "DTS-HD Master Audio";
        case 0xAC : return "DTS-HD Express";
        case 0xD1 : return "Private - EVRC";
        case 0xD3 : return "Private - AC-3";
        case 0xD4 : return "Private - DTS";
        case 0xDD : return "Private - Ogg";
        case 0xDE : return "Private - Ogg";
        case 0xE0 : return "Private - VobSub";
        case 0xE1 : return "Private - QCELP";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mpeg4_Descriptors_StreamType(int8u ID)
{
    switch (ID)
    {
        case 0x01 : return "ObjectDescriptorStream";
        case 0x02 : return "ClockReferenceStream";
        case 0x03 : return "SceneDescriptionStream";
        case 0x04 : return "VisualStream";
        case 0x05 : return "AudioStream";
        case 0x06 : return "MPEG7Stream";
        case 0x07 : return "IPMPStream";
        case 0x08 : return "ObjectContentInfoStream";
        case 0x09 : return "MPEGJStream";
        case 0x0A : return "Interaction Stream";
        case 0x0B : return "IPMPToolStream";
        case 0x0C : return "FontDataStream";
        case 0x0D : return "StreamingText";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mpeg4_Descriptors_ODProfileLevelIndication(int8u /*ID*/)
{
    return "";
}

//---------------------------------------------------------------------------
static const char* Mpeg4_Descriptors_SceneProfileLevelIndication(int8u ID)
{
    switch (ID)
    {
        case    1 : return "Simple2D@L1";
        case    2 : return "Simple2D@L2";
        case   11 : return "Basic2D@L1";
        case   12 : return "Core2D@L1";
        case   13 : return "Core2D@L2";
        case   14 : return "Advanced2D@L1";
        case   15 : return "Advanced2D@L2";
        case   16 : return "Advanced2D@L3";
        case   17 : return "Main2D@L1";
        case   18 : return "Main2D@L2";
        case   19 : return "Main2D@L3";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mpeg4_Descriptors_AudioProfileLevelIndication_Profile[]=
{
    nullptr,
    "Main Audio",
    "Scalable Audio",
    "Speech Audio",
    "Synthesis Audio",
    "High Quality Audio",
    "Low Delay Audio",
    "Natural Audio",
    "Mobile Audio Internetworking",
    "AAC",
    "High Efficiency AAC",
    "High Efficiency AAC v2",
    "Low Delay AAC",
    "Low Delay AAC v2",
    "Baseline MPEG Surround",
    "High Definition AAC",
    "ALS Simple",
    "Baseline USAC",
    "Extended HE AAC",
    nullptr,
    nullptr,
};
extern const profilelevel_struct Mpeg4_Descriptors_AudioProfileLevelIndication_Mapping[]=
{
    { UnknownAudio, 0 },
    { Main_Audio, 1 },
    { Main_Audio, 2 },
    { Main_Audio, 3 },
    { Main_Audio, 4 },
    { Scalable_Audio, 1 },
    { Scalable_Audio, 2 },
    { Scalable_Audio, 3 },
    { Scalable_Audio, 4 },
    { Speech_Audio, 1 },
    { Speech_Audio, 2 },
    { Synthesis_Audio, 1 },
    { Synthesis_Audio, 2 },
    { Synthesis_Audio, 3 },
    { High_Quality_Audio, 1 },
    { High_Quality_Audio, 2 },
    { High_Quality_Audio, 3 },
    { High_Quality_Audio, 4 },
    { High_Quality_Audio, 5 },
    { High_Quality_Audio, 6 },
    { High_Quality_Audio, 7 },
    { High_Quality_Audio, 8 },
    { Low_Delay_Audio, 1 },
    { Low_Delay_Audio, 2 },
    { Low_Delay_Audio, 3 },
    { Low_Delay_Audio, 4 },
    { Low_Delay_Audio, 5 },
    { Low_Delay_Audio, 6 },
    { Low_Delay_Audio, 7 },
    { Low_Delay_Audio, 8 },
    { Natural_Audio, 1 },
    { Natural_Audio, 2 },
    { Natural_Audio, 3 },
    { Natural_Audio, 4 },
    { Mobile_Audio_Internetworking, 1 },
    { Mobile_Audio_Internetworking, 2 },
    { Mobile_Audio_Internetworking, 3 },
    { Mobile_Audio_Internetworking, 4 },
    { Mobile_Audio_Internetworking, 5 },
    { Mobile_Audio_Internetworking, 6 },
    { AAC, 1 },
    { AAC, 2 },
    { AAC, 4 },
    { AAC, 5 },
    { High_Efficiency_AAC, 2 },
    { High_Efficiency_AAC, 3 },
    { High_Efficiency_AAC, 4 },
    { High_Efficiency_AAC, 5 },
    { High_Efficiency_AAC_v2, 2 },
    { High_Efficiency_AAC_v2, 3 },
    { High_Efficiency_AAC_v2, 4 },
    { High_Efficiency_AAC_v2, 5 },
    { Low_Delay_AAC, 1 },
    { Baseline_MPEG_Surround, 1 },
    { Baseline_MPEG_Surround, 2 },
    { Baseline_MPEG_Surround, 3 },
    { Baseline_MPEG_Surround, 4 },
    { Baseline_MPEG_Surround, 5 },
    { Baseline_MPEG_Surround, 6 },
    { High_Definition_AAC, 1 },
    { ALS_Simple, 1 },
    { UnknownAudio, 0 },
    { UnknownAudio, 0 },
    { UnknownAudio, 0 },
    { UnknownAudio, 0 },
    { UnknownAudio, 0 },
    { UnknownAudio, 0 },
    { UnknownAudio, 0 },
    { Baseline_USAC, 1 },
    { Baseline_USAC, 2 },
    { Baseline_USAC, 3 },
    { Baseline_USAC, 4 },
    { Extended_HE_AAC, 1 },
    { Extended_HE_AAC, 2 },
    { Extended_HE_AAC, 3 },
    { Extended_HE_AAC, 4 },
    { Low_Delay_AAC_v2, 1 },
    { UnknownAudio, 0 },
    { UnknownAudio, 0 },
    { UnknownAudio, 0 },
    { AAC, 6 },
    { AAC, 7 },
    { High_Efficiency_AAC, 6 },
    { High_Efficiency_AAC, 7 },
    { High_Efficiency_AAC_v2, 6 },
    { High_Efficiency_AAC_v2, 7 },
    { Extended_HE_AAC, 6 },
    { Extended_HE_AAC, 7 },
    // No more in sequence
    { UnspecifiedAudio, 0 },
    { NoAudio, 0 },
};
static constexpr size_t Mpeg4_Descriptors_AudioProfileLevelIndication_Size=sizeof(Mpeg4_Descriptors_AudioProfileLevelIndication_Mapping)/sizeof(profilelevel_struct) - 2;
const profilelevel_struct& Mpeg4_Descriptors_ToProfileLevel(int8u AudioProfileLevelIndication)
{
    if (AudioProfileLevelIndication >= 0xFE)
        AudioProfileLevelIndication -= 0x100 - (Mpeg4_Descriptors_AudioProfileLevelIndication_Size  + 2);
    else if (AudioProfileLevelIndication >= Mpeg4_Descriptors_AudioProfileLevelIndication_Size)
        AudioProfileLevelIndication = 0;
    return Mpeg4_Descriptors_AudioProfileLevelIndication_Mapping[AudioProfileLevelIndication];
};
int8u Mpeg4_Descriptors_ToAudioProfileLevelIndication(const profilelevel_struct& ToMatch)
{
    switch (ToMatch.profile)
    {
        case UnspecifiedAudio             : return 0xFE;
        case NoAudio                      : return 0xFF;
        default:;
    }
    for (size_t i = 0; i < Mpeg4_Descriptors_AudioProfileLevelIndication_Size; i++)
        if (ToMatch == Mpeg4_Descriptors_AudioProfileLevelIndication_Mapping[i])
            return i;
    return 0;
}
string Mpeg4_Descriptors_AudioProfileLevelString(const profilelevel_struct& ProfileLevel)
{
    const auto ProfileString = Mpeg4_Descriptors_AudioProfileLevelIndication_Profile[ProfileLevel.profile];
    if (!ProfileString)
        return {};
    string ToReturn(ProfileString);
    if (ProfileLevel.level)
    {
        ToReturn += "@L";
        ToReturn += to_string(ProfileLevel.level);
    }
    return ToReturn;
}
string Mpeg4_Descriptors_AudioProfileLevelString(int8u AudioProfileLevelIndication)
{
    return Mpeg4_Descriptors_AudioProfileLevelString(Mpeg4_Descriptors_ToProfileLevel(AudioProfileLevelIndication));
}
#if MEDIAINFO_CONFORMANCE
string Mpeg4_Descriptors_AudioProfileLevelIndicationString(const profilelevel_struct& ProfileLevel)
{
    auto AudioProfileLevelIndication = Mpeg4_Descriptors_ToAudioProfileLevelIndication(ProfileLevel);
    string ProfileLevelString;
    ProfileLevelString = to_string(AudioProfileLevelIndication);
    auto Extra = Mpeg4_Descriptors_AudioProfileLevelString(ProfileLevel);
    if (!Extra.empty())
    {
        ProfileLevelString += " (";
        ProfileLevelString += Extra;
        ProfileLevelString += ')';
    }
    return ProfileLevelString;
}
#endif

//---------------------------------------------------------------------------
extern const char* Mpeg4v_Profile_Level(int32u Profile_Level);

//---------------------------------------------------------------------------
static const char* Mpeg4_Descriptors_GraphicsProfileLevelIndication(int8u ID)
{
    switch (ID)
    {
        case    1 : return "Simple2D@L1";
        case    2 : return "Simple2D+Text@L1";
        case    3 : return "Simple2D+Text@L2";
        case    4 : return "Core2D@L1";
        case    5 : return "Core2D@L2";
        case    6 : return "Advanced2D@L1";
        case    7 : return "Advanced2D@L2";
        default   : return "";
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Mpeg4_Descriptors::File_Mpeg4_Descriptors()
:File__Analyze()
{
    //Configuration
    ParserName="MPEG-4 Descriptor";
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Mpeg4_Desc;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    StreamSource=IsStream;

    //In
    KindOfStream=Stream_Max;
    PosOfStream=(size_t)-1;
    TrackID=(int32u)-1;
    Parser_DoNotFreeIt=false;
    SLConfig_DoNotFreeIt=false;

    //Out
    Parser=NULL;
    ES_ID=0x0000;
    SLConfig=NULL;

    //Temp
    ObjectTypeId=0x00;
    
    //Conformance
    #if MEDIAINFO_CONFORMANCE
        SamplingRate=0;
        stss=nullptr;
        sbgp=nullptr;
    #endif
}

//---------------------------------------------------------------------------
File_Mpeg4_Descriptors::~File_Mpeg4_Descriptors()
{
    if (!Parser_DoNotFreeIt)
        delete Parser;// Parser=NULL;
    if (!SLConfig_DoNotFreeIt)
        delete SLConfig;// SLConfig=NULL;
}

//***************************************************************************
// Buffer
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Header_Parse()
{
    //Parsing
    size_t Size=0;
    int8u type, Size_ToAdd;
    Get_B1(type,                                            "type");
    if (type==0)
    {
        Header_Fill_Code(0x00, "Padding");
        Header_Fill_Size(1);
        return;
    }
    do
    {
        Get_B1(Size_ToAdd,                                  "size");
        Size=(Size<<7) | (Size_ToAdd&0x7F);
    }
    while (Size_ToAdd&0x80);

    //Filling
    Header_Fill_Code(type, Ztring().From_CC1(type));
    if (Element_Offset+Size>=Element_Size)
        Size=(size_t)(Element_Size-Element_Offset); //Found one file with too big size but content is OK, cutting the block
    Header_Fill_Size(Element_Offset+Size);
}


//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Data_Parse()
{
    //Preparing
    Status[IsAccepted]=true;

    #define ELEMENT_CASE(_NAME, _DETAIL) \
        case 0x##_NAME : Element_Name(_DETAIL); Descriptor_##_NAME(); break;

    //Parsing
    switch (Element_Code)
    {
        ELEMENT_CASE(00, "Forbidden");
        ELEMENT_CASE(01, "ObjectDescr");
        ELEMENT_CASE(02, "InitialObjectDescr");
        ELEMENT_CASE(03, "ES_Descr");
        ELEMENT_CASE(04, "DecoderConfigDescr");
        ELEMENT_CASE(05, "DecoderSpecificInfo");
        ELEMENT_CASE(06, "SLConfigDescr");
        ELEMENT_CASE(07, "ContentIdentDescr");
        ELEMENT_CASE(08, "SupplContentIdentDescr");
        ELEMENT_CASE(09, "IPI_DescrPointer");
        ELEMENT_CASE(0A, "IPMP_DescrPointer");
        ELEMENT_CASE(0B, "IPMP_Descr");
        ELEMENT_CASE(0C, "QoS_Descr");
        ELEMENT_CASE(0D, "RegistrationDescr");
        ELEMENT_CASE(0E, "ES_ID_Inc");
        ELEMENT_CASE(0F, "ES_ID_Ref");
        ELEMENT_CASE(10, "MP4_IOD_");
        ELEMENT_CASE(11, "MP4_OD_");
        ELEMENT_CASE(12, "IPL_DescrPointerRef");
        ELEMENT_CASE(13, "ExtendedProfileLevelDescr");
        ELEMENT_CASE(14, "profileLevelIndicationIndexDescriptor");
        ELEMENT_CASE(40, "ContentClassificationDescr");
        ELEMENT_CASE(41, "KeyWordDescr");
        ELEMENT_CASE(42, "RatingDescr");
        ELEMENT_CASE(43, "LanguageDescr");
        ELEMENT_CASE(44, "ShortTextualDescr");
        ELEMENT_CASE(45, "ExpandedTextualDescr");
        ELEMENT_CASE(46, "ContentCreatorNameDescr");
        ELEMENT_CASE(47, "ContentCreationDateDescr");
        ELEMENT_CASE(48, "OCICreatorNameDescr");
        ELEMENT_CASE(49, "OCICreationDateDescr");
        ELEMENT_CASE(4A, "SmpteCameraPositionDescr");
        ELEMENT_CASE(4B, "SegmentDescr");
        ELEMENT_CASE(4C, "MediaTimeDescr");
        ELEMENT_CASE(60, "IPMP_ToolsListDescr");
        ELEMENT_CASE(61, "IPMP_Tool");
        ELEMENT_CASE(62, "FLEXmuxTimingDescr");
        ELEMENT_CASE(63, "FLEXmuxCodeTableDescr");
        ELEMENT_CASE(64, "ExtSLConfigDescr");
        ELEMENT_CASE(65, "FLEXmuxBufferSizeDescr");
        ELEMENT_CASE(66, "FLEXmuxIdentDescr");
        ELEMENT_CASE(67, "DependencyPointer");
        ELEMENT_CASE(68, "DependencyMarker");
        ELEMENT_CASE(69, "FLEXmuxChannelDescr");
        default: if (Element_Code>=0xC0)
                    Element_Name("user private");
                 else
                    Element_Name("unknown");
                 Skip_XX(Element_Size,                          "Data");
                 break;
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_01()
{
    //Parsing
    int8u ProfileLevel[5]{};
    bool URL_Flag;
    BS_Begin();
    Skip_S2(10,                                                 "ObjectDescriptorID");
    Get_SB (    URL_Flag,                                       "URL_Flag");
    Skip_SB(                                                    "includeInlineProfileLevelFlag");
    Skip_S1( 4,                                                 "reserved");
    BS_End();
    if (URL_Flag)
    {
        int8u URLlength;
        Get_B1 (URLlength,                                      "URLlength");
        Skip_UTF8(URLlength,                                    "URLstring");
    }
    else if (Element_Code==0x02 || Element_Code==0x10)
    {
        Get_B1 (ProfileLevel[0],                                "ODProfileLevelIndication"); Param_Info1(Mpeg4_Descriptors_ODProfileLevelIndication(ProfileLevel[0]));
        Get_B1 (ProfileLevel[1],                                "sceneProfileLevelIndication"); Param_Info1(Mpeg4_Descriptors_SceneProfileLevelIndication(ProfileLevel[1]));
        Get_B1 (ProfileLevel[2],                                "audioProfileLevelIndication"); Param_Info1(Mpeg4_Descriptors_AudioProfileLevelString(ProfileLevel[2]));
        Get_B1 (ProfileLevel[3],                                "visualProfileLevelIndication"); Param_Info1(Mpeg4v_Profile_Level(ProfileLevel[3]));
        Get_B1 (ProfileLevel[4],                                "graphicsProfileLevelIndication"); Param_Info1(Mpeg4_Descriptors_GraphicsProfileLevelIndication(ProfileLevel[4]));
    }

    FILLING_BEGIN();
        if (Element_Code==0x10)
        {
            //Clear
            ES_ID_Infos.clear();

            //Fill
            int8u ProfileLevel_Count=0;
            for (int8u i=0; i<5; i++)
                if (ProfileLevel[i]!=0xFF)
                    ProfileLevel_Count++;
            es_id_info& ES_ID_Info=ES_ID_Infos[(int32u)-1];
            if (ProfileLevel_Count==1)
            {
                for (int8u i=0; i<5; i++)
                {
                    if (ProfileLevel[i]!=0xFF)
                    {
                        switch (i)
                        {
                            case  2 :   ES_ID_Info.StreamKind=Stream_Audio; 
                                        ES_ID_Info.ProfileLevelString.From_UTF8(Mpeg4_Descriptors_AudioProfileLevelString(ProfileLevel[i]));
                                        break;
                            case  3 :
                                        ES_ID_Info.StreamKind=Stream_Video;
                                        ES_ID_Info.ProfileLevelString=Mpeg4v_Profile_Level(ProfileLevel[i]);
                                        break;
                            default :   ;
                        }
                        if (ES_ID_Info.ProfileLevelString.empty() && ProfileLevel[i]!=0xFE)
                            ES_ID_Info.ProfileLevelString.From_Number(ProfileLevel[i]);
                    }
                }
            }
            memcpy(ES_ID_Info.ProfileLevel, ProfileLevel, sizeof(ProfileLevel));
        }
        Element_ThisIsAList();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_03()
{
    //Parsing
    bool streamDependenceFlag, URL_Flag, OCRstreamFlag;
    Get_B2 (ES_ID,                                              "ES_ID");
    BS_Begin();
    Get_SB (   streamDependenceFlag,                            "streamDependenceFlag");
    Get_SB (   URL_Flag,                                        "URL_Flag");
    Get_SB (   OCRstreamFlag,                                   "OCRstreamFlag");
    Skip_S1(5,                                                  "streamPriority");
    BS_End();
    if (streamDependenceFlag)
        Skip_B2(                                                "dependsOn_ES_ID");
    if (URL_Flag)
    {
        int8u URLlength;
        Get_B1 (URLlength,                                      "URLlength");
        Skip_UTF8(URLlength,                                    "URLstring");
    }
    if (OCRstreamFlag)
        Skip_B2(                                                "OCR_ES_Id");

    FILLING_BEGIN();
        Element_ThisIsAList();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_04()
{
    //Parsing
    int32u bufferSizeDB, MaxBitrate, AvgBitrate;
    int8u  streamType;
    Get_B1 (ObjectTypeId,                                       "objectTypeIndication"); Param_Info1(Mpeg4_Descriptors_ObjectTypeIndication(ObjectTypeId));
    BS_Begin();
    Get_S1 (6, streamType,                                      "streamType"); Param_Info1(Mpeg4_Descriptors_StreamType(streamType));
    Skip_SB(                                                    "upStream");
    Skip_SB(                                                    "reserved");
    BS_End();
    Get_B3 (bufferSizeDB,                                       "bufferSizeDB");
    Get_B4 (MaxBitrate,                                         "maxBitrate");
    Get_B4 (AvgBitrate,                                         "avgBitrate");

    FILLING_BEGIN();
        if (KindOfStream==Stream_Max)
            switch (ObjectTypeId)
            {
                case 0x20 :
                case 0x21 :
                case 0x60 :
                case 0x61 :
                case 0x62 :
                case 0x63 :
                case 0x64 :
                case 0x65 :
                case 0x6A :
                case 0x6C :
                case 0x6D :
                case 0x6E :
                case 0xA3 :
                case 0xA4 :
                            KindOfStream=Stream_Video; break;
                case 0x40 :
                case 0x66 :
                case 0x67 :
                case 0x68 :
                case 0x69 :
                case 0x6B :
                case 0xA0 :
                case 0xA1 :
                case 0xA5 :
                case 0xA6 :
                case 0xA9 :
                case 0xAA :
                case 0xAB :
                case 0xAC :
                case 0xD1 :
                case 0xD3 :
                case 0xD4 :
                case 0xE1 :
                            KindOfStream=Stream_Audio; break;
                case 0x08 :
                case 0xE0 :
                            KindOfStream=Stream_Text; break;
                default: ;
            }
        if (Count_Get(KindOfStream)==0)
            Stream_Prepare(KindOfStream);
        switch (ObjectTypeId)
        {
            case 0x01 : Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), "System", Unlimited, true, true); break;
            case 0x02 : Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), "System Core", Unlimited, true, true); break;
            //case 0x03 Interaction Stream
            //case 0x05 AFX
            //case 0x06 Font Data
            //case 0x07 Synthesized Texture Stream
            case 0x08 : Fill(Stream_Text    , StreamPos_Last, Text_Format, "Streaming Text", Unlimited, true, true); break;
            case 0x20 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "MPEG-4 Visual", Unlimited, true, true); break;
            case 0x21 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "AVC", Unlimited, true, true); break;
            //case 0x22 Parameter Sets for AVC
            case 0x40 : break; //MPEG-4 Audio (several formats are possible)
            case 0x60 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "MPEG Video", Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Profile, "Simple" , Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Version, "Version 2", Unlimited, true, true); break; //MPEG-2V Simple
            case 0x61 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "MPEG Video", Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Profile, "Main"   , Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Version, "Version 2", Unlimited, true, true); break; //MPEG-2V Main
            case 0x62 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "MPEG Video", Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Profile, "SNR"    , Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Version, "Version 2", Unlimited, true, true); break; //MPEG-2V SNR
            case 0x63 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "MPEG Video", Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Profile, "Spatial", Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Version, "Version 2", Unlimited, true, true); break; //MPEG-2V Spatial
            case 0x64 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "MPEG Video", Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Profile, "High"   , Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Version, "Version 2", Unlimited, true, true); break; //MPEG-2V High
            case 0x65 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "MPEG Video", Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Profile, "4:2:2"  , Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Version, "Version 2", Unlimited, true, true); break; //MPEG-2V 4:2:2
            case 0x66 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "AAC", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_Format_Profile, "Main", Unlimited, true, true); break; //MPEG-2 AAC Main
            case 0x67 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "AAC", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_Format_Profile, "LC", Unlimited, true, true); break; //MPEG-2 AAC LC
            case 0x68 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "AAC", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_Format_Profile, "SSR", Unlimited, true, true); break; //MPEG-2 AAC SSR
            case 0x69 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "MPEG Audio", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_Format_Version, "Version 2", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_Format_Profile, "Layer 3", Unlimited, true, true); break;
            case 0x6A : Fill(Stream_Video   , StreamPos_Last, Video_Format, "MPEG Video", Unlimited, true, true); Fill(Stream_Video, StreamPos_Last, Video_Format_Version, "Version 1", Unlimited, true, true); break;
            case 0x6B : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "MPEG Audio", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_Format_Version, "Version 1", Unlimited, true, true); break;
            case 0x6C : Fill(Stream_Video   , StreamPos_Last, Video_Format, "JPEG", Unlimited, true, true); break;
            case 0x6D : Fill(Stream_Video   , StreamPos_Last, Video_Format, "PNG", Unlimited, true, true); break;
            case 0x6E : Fill(Stream_Video   , StreamPos_Last, Video_Format, "MPEG Video", Unlimited, true, true); break;
            case 0xA0 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "EVRC", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, 8000, 10, true); Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, 1, 10, true); break;
            case 0xA1 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "SMV", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, 8000, 10, true); Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, 1, 10, true);  break;
            case 0xA2 : Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), "3GPP2", Unlimited, true, true); break;
            case 0xA3 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "VC-1", Unlimited, true, true); break;
            case 0xA4 : Fill(Stream_Video   , StreamPos_Last, Video_Format, "Dirac", Unlimited, true, true); break;
            case 0xA5 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "AC-3", Unlimited, true, true); break;
            case 0xA6 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "E-AC-3", Unlimited, true, true); break;
            case 0xA9 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "DTS", Unlimited, true, true); break;
            case 0xAA : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "DTS", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_Format_Profile, "HRA", Unlimited, true, true); break; // DTS-HD High Resolution
            case 0xAB : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "DTS", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_Format_Profile, "MA", Unlimited, true, true); break;  // DTS-HD Master Audio
            case 0xAC : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "DTS", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_Format_Profile, "Express", Unlimited, true, true); break;  // DTS Express a.k.a. LBR
            case 0xD1 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "EVRC", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, 8000, 10, true); Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, 1, 10, true);  break;
            case 0xD3 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "AC-3", Unlimited, true, true); break;
            case 0xD4 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "DTS", Unlimited, true, true); break;
            case 0xDD : Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), "Ogg", Unlimited, true, true); break;
            case 0xDE : Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), "Ogg", Unlimited, true, true); break;
            case 0xE0 : Fill(Stream_Text,     StreamPos_Last, Text_Format,  "VobSub", Unlimited, true, true); CodecID_Fill(__T("subp"), Stream_Text, StreamPos_Last, InfoCodecID_Format_Mpeg4); break;
            case 0xE1 : Fill(Stream_Audio   , StreamPos_Last, Audio_Format, "QCELP", Unlimited, true, true); Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, 8000, 10, true); Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, 1, 10, true);  break;
            default: ;
        }
        switch (ObjectTypeId)
        {
            case 0x01 : Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec), "System", Unlimited, true, true); break;
            case 0x02 : Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec), "System Core", Unlimited, true, true); break;
            case 0x20 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-4V", Unlimited, true, true); break;
            case 0x21 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "H264", Unlimited, true, true); break;
            case 0x40 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "AAC", Unlimited, true, true); break; //MPEG-4 AAC
            case 0x60 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-2V", Unlimited, true, true); break; //MPEG-2V Simple
            case 0x61 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-2V", Unlimited, true, true); break; //MPEG-2V Main
            case 0x62 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-2V", Unlimited, true, true); break; //MPEG-2V SNR
            case 0x63 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-2V", Unlimited, true, true); break; //MPEG-2V Spatial
            case 0x64 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-2V", Unlimited, true, true); break; //MPEG-2V High
            case 0x65 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-2V", Unlimited, true, true); break; //MPEG-2V 4:2:2
            case 0x66 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "AAC", Unlimited, true, true); break; //MPEG-2 AAC Main
            case 0x67 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "AAC", Unlimited, true, true); break; //MPEG-2 AAC LC
            case 0x68 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "AAC", Unlimited, true, true); break; //MPEG-2 AAC SSR
            case 0x69 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "MPEG-2A L3", Unlimited, true, true); break;
            case 0x6A : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-1V", Unlimited, true, true); break;
            case 0x6B : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "MPEG-1A", Unlimited, true, true); break;
            case 0x6C : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "JPEG", Unlimited, true, true); break;
            case 0x6D : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "PNG", Unlimited, true, true); break;
            case 0x6E : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-4V", Unlimited, true, true); break;
            case 0xA0 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "EVRC", Unlimited, true, true); break;
            case 0xA1 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "SMV", Unlimited, true, true); break;
            case 0xA2 : Fill(Stream_Video   , StreamPos_Last, Video_Codec, "MPEG-4V", Unlimited, true, true); break;
            case 0xA3 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "VC-1", Unlimited, true, true); break;
            case 0xA4 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "Dirac", Unlimited, true, true); break;
            case 0xA5 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "AC3", Unlimited, true, true); break;
            case 0xA6 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "AC3+", Unlimited, true, true); break;
            case 0xA9 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "DTS", Unlimited, true, true); break;
            case 0xAA :
            case 0xAB : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "DTS-HD", Unlimited, true, true); break;
            case 0xAC : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "DTS Express", Unlimited, true, true); break;
            case 0xD1 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "EVRC", Unlimited, true, true); break;
            case 0xD3 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "AC3", Unlimited, true, true); break;
            case 0xD4 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "DTS", Unlimited, true, true); break;
            case 0xDD : Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec), "Ogg", Unlimited, true, true); break;
            case 0xDE : Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec), "Ogg", Unlimited, true, true); break;
            case 0xE0 : Fill(Stream_Text    , StreamPos_Last, Text_Codec,  "subp", Unlimited, true, true); break;
            case 0xE1 : Fill(Stream_Audio   , StreamPos_Last, Audio_Codec, "QCELP", Unlimited, true, true); break;
            default: ;
        }
        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_CodecID), Ztring().From_CC1(ObjectTypeId), true);
        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec_CC), ObjectTypeId, 16, true);

        //Bitrate mode
        if (AvgBitrate>0
         && !(bufferSizeDB==AvgBitrate && bufferSizeDB==MaxBitrate && bufferSizeDB==0x1000)) //Some buggy data were found
        {
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate_Nominal), AvgBitrate);
            if (!MaxBitrate || (MaxBitrate>=AvgBitrate && MaxBitrate<=AvgBitrate*1.005))
            {
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate_Mode), "CBR");
            }
            else
            {
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate_Mode), "VBR");
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate_Maximum), MaxBitrate);
            }
        }

        //Creating parser
        delete Parser; Parser=NULL;
        switch (ObjectTypeId)
        {
            case 0x01 : switch (streamType)
                        {
                            case 0x01 : Parser=new File_Mpeg4_Descriptors; break;
                            default   : ;
                        }
                        break;
            case 0x20 : //MPEG-4 Visual
                        #if defined(MEDIAINFO_MPEG4V_YES)
                            Parser=new File_Mpeg4v;
                            ((File_Mpeg4v*)Parser)->Frame_Count_Valid=1;
                            ((File_Mpeg4v*)Parser)->FrameIsAlwaysComplete=true;
                        #endif
                        break;
            case 0x21 : //AVC
                        #if defined(MEDIAINFO_AVC_YES)
                            Parser=new File_Avc;
                            ((File_Avc*)Parser)->MustParse_SPS_PPS=true;
                            ((File_Avc*)Parser)->MustSynchronize=false;
                            ((File_Avc*)Parser)->SizedBlocks=true;
                        #endif
                        break;
            case 0x40 : //MPEG-4 AAC
            case 0x66 :
            case 0x67 :
            case 0x68 : //MPEG-2 AAC
                        #if defined(MEDIAINFO_AAC_YES)
                            Parser=new File_Aac;
                            ((File_Aac*)Parser)->Mode=File_Aac::Mode_AudioSpecificConfig;
                            ((File_Aac*)Parser)->FrameIsAlwaysComplete=true;
                            #if MEDIAINFO_CONFORMANCE
                                ((File_Aac*)Parser)->Immediate_FramePos=stss;
                                ((File_Aac*)Parser)->Immediate_FramePos_IsPresent=stss_IsPresent;
                                ((File_Aac*)Parser)->IsCmaf=IsCmaf;
                                ((File_Aac*)Parser)->outputFrameLength=stts;
                                ((File_Aac*)Parser)->FirstOutputtedDecodedSample=FirstOutputtedDecodedSample;
                                ((File_Aac*)Parser)->roll_distance_Values=sgpd_prol;
                                ((File_Aac*)Parser)->roll_distance_FramePos=sbgp;
                                ((File_Aac*)Parser)->roll_distance_FramePos_IsPresent=sbgp_IsPresent;
                                ((File_Aac*)Parser)->SamplingRate=SamplingRate;
                                {
                                    auto const ES_ID_Info=ES_ID_Infos.find(TrackID!=(int32u)-1?TrackID:ES_ID);
                                    if (ES_ID_Info!=ES_ID_Infos.end())
                                        ((File_Aac*)Parser)->SetProfileLevel(ES_ID_Info->second.ProfileLevel[2]);
                                }
                            #endif
                        #endif
                        break;
            case 0x60 :
            case 0x61 :
            case 0x62 :
            case 0x63 :
            case 0x64 :
            case 0x65 :
            case 0x6A : //MPEG Video
                        #if defined(MEDIAINFO_MPEGV_YES)
                            Parser=new File_Mpegv;
                            ((File_Mpegv*)Parser)->FrameIsAlwaysComplete=true;
                        #endif
                        break;
            case 0x69 :
            case 0x6B : //MPEG Audio
                        #if defined(MEDIAINFO_MPEGA_YES)
                            Parser=new File_Mpega;
                        #endif
                        break;
            case 0x6C : //JPEG
                        #if defined(MEDIAINFO_JPEG_YES)
                            Parser=new File_Jpeg;
                            ((File_Jpeg*)Parser)->StreamKind=Stream_Video;
                        #endif
                        break;
            case 0x6D : //PNG
                        #if defined(MEDIAINFO_PNG_YES)
                            Parser=new File_Png;
                        #endif
                        break;
            case 0xA3 : //VC-1
                        #if defined(MEDIAINFO_VC1_YES)
                            Parser=new File_Vc1;
                        #endif
                        break;
            case 0xA4 : //Dirac
                        #if defined(MEDIAINFO_DIRAC_YES)
                            Parser=new File_Dirac;
                        #endif
                        break;
            case 0xA5 : //AC-3
            case 0xA6 : //E-AC-3
            case 0xD3 : //AC-3
                        #if defined(MEDIAINFO_AC3_YES)
                            Parser=new File_Ac3;
                        #endif
                        break;
            case 0xA9 : //DTS
            case 0xAA : //DTS HRA
            case 0xAB : //DTS MA
            case 0xAC : //DTS Express
            case 0xD4 : //DTS
                        #if defined(MEDIAINFO_DTS_YES)
                            Parser=new File_Dts;
                        #endif
                        break;
            case 0xDD :
            case 0xDE : //OGG
                        #if defined(MEDIAINFO_OGG_YES)
                            Parser=new File_Ogg;
                            Parser->MustSynchronize=false;
                            ((File_Ogg*)Parser)->SizedBlocks=true;
                        #endif
                        break;
            default: ;
        }

        Element_Code=(int64u)-1;
        Open_Buffer_Init(Parser);

        Element_ThisIsAList();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_05()
{
    if (ObjectTypeId==0x00 && Parser==NULL) //If no ObjectTypeId detected
    {
        switch (KindOfStream)
        {
            case Stream_Video :
                                #if defined(MEDIAINFO_MPEG4V_YES)
                                    delete Parser; Parser=new File_Mpeg4v;
                                    ((File_Mpeg4v*)Parser)->FrameIsAlwaysComplete=true;
                                #endif
                                break;
            case Stream_Audio :
                                #if defined(MEDIAINFO_AAC_YES)
                                    delete Parser; Parser=new File_Aac;
                                    ((File_Aac*)Parser)->Mode=File_Aac::Mode_AudioSpecificConfig;
                                    #if MEDIAINFO_CONFORMANCE
                                        ((File_Aac*)Parser)->Immediate_FramePos=stss;
                                        ((File_Aac*)Parser)->Immediate_FramePos_IsPresent=stss_IsPresent;
                                        ((File_Aac*)Parser)->IsCmaf=IsCmaf;
                                        ((File_Aac*)Parser)->outputFrameLength=stts;
                                        ((File_Aac*)Parser)->FirstOutputtedDecodedSample=FirstOutputtedDecodedSample;
                                        ((File_Aac*)Parser)->roll_distance_Values=sgpd_prol;
                                        ((File_Aac*)Parser)->roll_distance_FramePos=sbgp;
                                        ((File_Aac*)Parser)->roll_distance_FramePos_IsPresent=sbgp_IsPresent;
                                        ((File_Aac*)Parser)->SamplingRate=SamplingRate;
                                    #endif
                                #endif
                                break;
            default: ;
        }

        Element_Code=(int64u)-1;
        Open_Buffer_Init(Parser);
    }

    if (Parser==NULL)
    {
        Skip_XX(Element_Size,                                   "Unknown");
        return;
    }

    //Parser configuration before the parsing
    switch (ObjectTypeId)
    {
            case 0x60 :
            case 0x61 :
            case 0x62 :
            case 0x63 :
            case 0x64 :
            case 0x65 :
            case 0x6A : //MPEG Video
                    #if defined(MEDIAINFO_MPEGV_YES)
                        ((File_Mpegv*)Parser)->TimeCodeIsNotTrustable=true;
                    #endif
                    break;
        default: ;
    }

    //Parsing
    Open_Buffer_Continue(Parser);

    //Demux
    #if MEDIAINFO_DEMUX
        if (ObjectTypeId!=0x21 || !Config->Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Get()) //0x21 is AVC
            switch (Config->Demux_InitData_Get())
            {
                case 0 :    //In demux event
                            Demux_Level=2; //Container
                            Demux(Buffer+Buffer_Offset, (size_t)Element_Size, ContentType_Header);
                            break;
                case 1 :    //In field
                            {
                            std::string Data_Raw((const char*)(Buffer+Buffer_Offset), (size_t)Element_Size);
                            std::string Data_Base64(Base64::encode(Data_Raw));
                            Parser->Fill(KindOfStream, PosOfStream, "Demux_InitBytes", Data_Base64);
                            Parser->Fill_SetOptions(KindOfStream, PosOfStream, "Demux_InitBytes", "N NT");
                            }
                            break;
                default :   ;
            }
    #endif //MEDIAINFO_DEMUX

    //Parser configuration after the parsing
    switch (ObjectTypeId)
    {
            case 0x60 :
            case 0x61 :
            case 0x62 :
            case 0x63 :
            case 0x64 :
            case 0x65 :
            case 0x6A : //MPEG Video
                    #if defined(MEDIAINFO_MPEGV_YES)
                        ((File_Mpegv*)Parser)->TimeCodeIsNotTrustable=false;
                    #endif
                    break;
        default: ;
    }

    //Positionning
    Element_Offset=Element_Size;
}

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_06()
{
    delete SLConfig; SLConfig=new slconfig;

    //Parsing
    int8u predefined;
    Get_B1 (predefined,                                         "predefined"); Param_Info1(Mpeg4_Descriptors_Predefined(predefined));
    switch (predefined)
    {
        case 0x00 :
            {
                    BS_Begin();
                    Get_SB (SLConfig->useAccessUnitStartFlag,   "useAccessUnitStartFlag");
                    Get_SB (SLConfig->useAccessUnitEndFlag,     "useAccessUnitEndFlag");
                    Get_SB (SLConfig->useRandomAccessPointFlag, "useRandomAccessPointFlag");
                    Get_SB (SLConfig->hasRandomAccessUnitsOnlyFlag, "hasRandomAccessUnitsOnlyFlag");
                    Get_SB (SLConfig->usePaddingFlag,           "usePaddingFlag");
                    Get_SB (SLConfig->useTimeStampsFlag,        "useTimeStampsFlag");
                    Get_SB (SLConfig->useIdleFlag,              "useIdleFlag");
                    Get_SB (SLConfig->durationFlag,             "durationFlag");
                    BS_End();
                    Get_B4 (SLConfig->timeStampResolution,      "timeStampResolution");
                    Get_B4( SLConfig->OCRResolution,            "OCRResolution");
                    Get_B1 (SLConfig->timeStampLength,          "timeStampLength");
                    Get_B1 (SLConfig->OCRLength,                "OCRLength");
                    Get_B1 (SLConfig->AU_Length,                "AU_Length");
                    Get_B1 (SLConfig->instantBitrateLength,     "instantBitrateLength");
                    BS_Begin();
                    Get_S1 (4, SLConfig->degradationPriorityLength, "degradationPriorityLength");
                    Get_S1 (5, SLConfig->AU_seqNumLength,       "AU_seqNumLength");
                    Get_S1 (5, SLConfig->packetSeqNumLength,    "packetSeqNumLength");
                    Skip_S1(2,                                  "reserved");
                    BS_End();
            }
            break;
        case 0x01 :
                    SLConfig->useAccessUnitStartFlag            =false;
                    SLConfig->useAccessUnitEndFlag              =false;
                    SLConfig->useRandomAccessPointFlag          =false;
                    SLConfig->hasRandomAccessUnitsOnlyFlag      =false;
                    SLConfig->usePaddingFlag                    =false;
                    SLConfig->useTimeStampsFlag                 =false;
                    SLConfig->useIdleFlag                       =false;
                    SLConfig->durationFlag                      =false; //-
                    SLConfig->timeStampResolution               =1000;
                    SLConfig->OCRResolution                     =0; //-
                    SLConfig->timeStampLength                   =32;
                    SLConfig->OCRLength                         =0; //-
                    SLConfig->AU_Length                         =0;
                    SLConfig->instantBitrateLength              =0; //-
                    SLConfig->degradationPriorityLength         =0;
                    SLConfig->AU_seqNumLength                   =0;
                    SLConfig->packetSeqNumLength                =0;
                    break;
        case 0x02 :
                    SLConfig->useAccessUnitStartFlag            =false;
                    SLConfig->useAccessUnitEndFlag              =false;
                    SLConfig->useRandomAccessPointFlag          =false;
                    SLConfig->hasRandomAccessUnitsOnlyFlag      =false;
                    SLConfig->usePaddingFlag                    =false;
                    SLConfig->useTimeStampsFlag                 =true;
                    SLConfig->useIdleFlag                       =false;
                    SLConfig->durationFlag                      =false;
                    SLConfig->timeStampResolution               =0; //-
                    SLConfig->OCRResolution                     =0; //-
                    SLConfig->timeStampLength                   =0;
                    SLConfig->OCRLength                         =0;
                    SLConfig->AU_Length                         =0;
                    SLConfig->instantBitrateLength              =0;
                    SLConfig->degradationPriorityLength         =0;
                    SLConfig->AU_seqNumLength                   =0;
                    SLConfig->packetSeqNumLength                =0;
                    break;
        default   :
                    SLConfig->useAccessUnitStartFlag            =false;
                    SLConfig->useAccessUnitEndFlag              =false;
                    SLConfig->useRandomAccessPointFlag          =false;
                    SLConfig->hasRandomAccessUnitsOnlyFlag      =false;
                    SLConfig->usePaddingFlag                    =false;
                    SLConfig->useTimeStampsFlag                 =false;
                    SLConfig->useIdleFlag                       =false;
                    SLConfig->durationFlag                      =false;
                    SLConfig->timeStampResolution               =0;
                    SLConfig->OCRResolution                     =0;
                    SLConfig->timeStampLength                   =0;
                    SLConfig->AU_Length                         =0;
                    SLConfig->instantBitrateLength              =0;
                    SLConfig->degradationPriorityLength         =0;
                    SLConfig->AU_seqNumLength                   =0;
                    SLConfig->packetSeqNumLength                =0;
    }
    if (SLConfig->durationFlag)
    {
        Get_B4 (SLConfig->timeScale,                            "timeScale");
        Get_B2 (SLConfig->accessUnitDuration,                   "accessUnitDuration");
        Get_B2 (SLConfig->compositionUnitDuration,              "compositionUnitDuration");
    }
    else
    {
                SLConfig->timeScale                             =0; //-
                SLConfig->accessUnitDuration                    =0; //-
                SLConfig->compositionUnitDuration               =0; //-
    }
    if (!SLConfig->useTimeStampsFlag)
    {
        BS_Begin();
        Get_S8 (SLConfig->timeStampLength, SLConfig->startDecodingTimeStamp, "startDecodingTimeStamp");
        Get_S8 (SLConfig->timeStampLength, SLConfig->startCompositionTimeStamp, "startCompositionTimeStamp");
        BS_End();
    }
    else
    {
                SLConfig->startDecodingTimeStamp                =0; //-
                SLConfig->startCompositionTimeStamp             =0; //-
    }
}

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_09()
{
    //Parsing
    Skip_B2(                                                    "IPI_ES_Id");
}

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_0E()
{
    //Parsing
    int32u Track_ID;
    Get_B4 (Track_ID,                                           "Track_ID"); //ID of the track to use

    FILLING_BEGIN();
        if (Track_ID!=(int32u)-1)
        {
            es_id_infos::iterator ES_ID_Info=ES_ID_Infos.find((int32u)-1);
            if (ES_ID_Info!=ES_ID_Infos.end())
            {
                ES_ID_Infos[Track_ID]=ES_ID_Info->second;
                ES_ID_Infos.erase(ES_ID_Info);
            }
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_0F()
{
    //Parsing
    Skip_B2(                                                    "ref_index");  //track ref. index of the track to use
}

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_10()
{
    Descriptor_02();
}

//---------------------------------------------------------------------------
void File_Mpeg4_Descriptors::Descriptor_11()
{
    Descriptor_01();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_MPEG4_YES
