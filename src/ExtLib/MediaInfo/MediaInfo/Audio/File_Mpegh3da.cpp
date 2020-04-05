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
#if defined(MEDIAINFO_MPEGH3DA_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Mpegh3da.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <cmath>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

extern const size_t Aac_sampling_frequency_Size_Usac; // USAC expands Aac_sampling_frequency[]
extern const int32u Aac_sampling_frequency[];
struct coreSbrFrameLengthIndex_mapping
{
    int8u    sbrRatioIndex;
    int8u    outputFrameLengthDivided256;
};
extern const size_t coreSbrFrameLengthIndex_Mapping_Size;
extern coreSbrFrameLengthIndex_mapping coreSbrFrameLengthIndex_Mapping[];
extern int8u Aac_Channels_Get(int8u ChannelLayout);
extern string Aac_Channels_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration2_GetString(int8u ChannelLayout);
extern string Aac_ChannelLayout_GetString(int8u ChannelLayout, bool IsMpegh3da=false);
extern string Aac_ChannelLayout_GetString(const vector<Aac_OutputChannel>& OutputChannels);
extern string Aac_ChannelMode_GetString(int8u ChannelLayout, bool IsMpegh3da=false);
extern string Aac_ChannelMode_GetString(const vector<Aac_OutputChannel>& OutputChannels);
extern string Aac_OutputChannelPosition_GetString(int8u OutputChannelPosition);

//---------------------------------------------------------------------------
static const char* const Mpegh3da_Profile[3]=
{
    "Main",
    "High",
    "LC",
};
extern string Mpegh3da_Profile_Get(int8u mpegh3daProfileLevelIndication)
{
    if (!mpegh3daProfileLevelIndication)
        return string();
    if (mpegh3daProfileLevelIndication>=0x10)
        return Ztring::ToZtring(mpegh3daProfileLevelIndication).To_UTF8(); // Raw value
    return string(Mpegh3da_Profile[(mpegh3daProfileLevelIndication-1)/5])+"@L"+char('0'+((mpegh3daProfileLevelIndication-1)%5));
}

//---------------------------------------------------------------------------
static const size_t Mpegh3da_MHASPacketType_Size=19;
static char* Mpegh3da_MHASPacketType[Mpegh3da_MHASPacketType_Size]=
{
    "FILLDATA",
    "MPEGH3DACFG",
    "MPEGH3DAFRAME",
    "AUDIOSCENEINFO",
    "",
    "",
    "SYNC",
    "SYNCGAP",
    "MARKER",
    "CRC16",
    "CRC32",
    "DESCRIPTOR",
    "USERINTERACTION",
    "LOUDNESS_DRC",
    "BUFFERINFO",
    "GLOBAL_CRC16",
    "GLOBAL_CRC32",
    "AUDIOTRUNCATION",
    "GENDATA",
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Mpegh3da::File_Mpegh3da()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Streams_Fill()
{
    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "MPEG-H 3D Audio");
    Fill(Stream_Audio, 0, Audio_Format_Profile, Mpegh3da_Profile_Get(mpegh3daProfileLevelIndication));
    Fill(Stream_Audio, 0, Audio_SamplingRate, usacSamplingFrequency);
    Fill(Stream_Audio, 0, Audio_SamplesPerFrame, coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex].outputFrameLengthDivided256 << 8);
    Streams_Fill_ChannelLayout(string(), referenceLayout);
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Streams_Finish()
{
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Header_Parse()
{
    //Parsing
    int32u MHASPacketType, MHASPacketLabel, MHASPacketLength;
    BS_Begin();
    escapedValue(MHASPacketType, 3, 8, 8,                       "MHASPacketType");
    escapedValue(MHASPacketLabel, 2, 8, 32,                     "MHASPacketLabel");
    escapedValue(MHASPacketLength, 11, 24, 24,                  "MHASPacketLength");
    BS_End();

    FILLING_BEGIN();
        Header_Fill_Code(MHASPacketType, MHASPacketType<Mpegh3da_MHASPacketType_Size?Ztring().From_UTF8(Mpegh3da_MHASPacketType[MHASPacketType]):Ztring().From_CC3(MHASPacketType));
        Header_Fill_Size(Element_Offset+MHASPacketLength);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Data_Parse()
{
    //Parsing
    switch (Element_Code)
    {
        case  1 : mpegh3daConfig(); break;
        case  2 : mpegh3daFrame(); break;
        case  6 : Sync(); break;
        default : Skip_XX(Element_Size-Element_Offset, "Data");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daConfig()
{
    Element_Begin1("mpegh3daConfig");
    BS_Begin();
    int8u usacSamplingFrequencyIndex;
    Get_S1 (8, mpegh3daProfileLevelIndication,                  "mpegh3daProfileLevelIndication");
    Get_S1 (5, usacSamplingFrequencyIndex,                      "usacSamplingFrequencyIndex");
    if (usacSamplingFrequencyIndex==0x1f)
        Get_S3 (24, usacSamplingFrequency,                      "usacSamplingFrequency");
    else
    {
        if (usacSamplingFrequencyIndex<Aac_sampling_frequency_Size_Usac)
            usacSamplingFrequency=Aac_sampling_frequency[usacSamplingFrequencyIndex];
        else
            usacSamplingFrequency=0;
    }
    Get_S1 (3, coreSbrFrameLengthIndex,                         "coreSbrFrameLengthIndex");
    Skip_SB(                                                    "cfg_reserved");
    Skip_SB(                                                    "receiverDelayCompensation");
    SpeakerConfig3d(referenceLayout);
    /*TODO
    FrameworkConfig3d();
    mpegh3daDecoderConfig();
    TEST_SB_SKIP(                                               "usacConfigExtensionPresent");
        mpegh3daConfigExtension();
    TEST_SB_END();
    */
    BS_End();
    Element_End0();

    FILLING_BEGIN();
        //Filling
        if (!Status[IsAccepted])
            Accept("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::SpeakerConfig3d(speaker_layout& Layout)
{
    int8u speakerLayoutType;
    Element_Begin1("SpeakerConfig3d");
    Get_S1(2, speakerLayoutType,                                "speakerLayoutType");
    if (speakerLayoutType==0)
    {
        Get_S1 (6, Layout.ChannelLayout,                        "CICPspeakerLayoutIdx");
    }
    else
    {
        int32u numSpeakers;
        escapedValue(numSpeakers, 5, 8, 16,                     "numSpeakers");
        numSpeakers++;
        Layout.numSpeakers=numSpeakers;

        if (speakerLayoutType==1)
        {
            Layout.CICPspeakerIdxs.resize(numSpeakers);
            for (size_t Pos=0; Pos<numSpeakers; Pos++)
            {
                int8u CICPspeakerIdx;
                Get_S1(7, CICPspeakerIdx,                       "CICPspeakerIdx");
                Layout.CICPspeakerIdxs[Pos]=(Aac_OutputChannel)CICPspeakerIdx;
            }
        }
        else if (speakerLayoutType==2)
        {
            mpegh3daFlexibleSpeakerConfig(Layout);
        }
    }
    Element_End0();

    FILLING_BEGIN();
        //Finish
        if (Status[IsAccepted])
            Finish("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daFlexibleSpeakerConfig(speaker_layout& Layout)
{
    bool angularPrecision;
    Element_Begin1("mpegh3daFlexibleSpeakerConfig");
    Get_SB(angularPrecision,                                    "angularPrecision");
    for (size_t Pos=0; Pos<Layout.numSpeakers; Pos++)
    {
        Layout.SpeakersInfo.push_back(speaker_info(angularPrecision));
        mpegh3daSpeakerDescription(Layout);
        if (Layout.SpeakersInfo.back().AzimuthAngle && Layout.SpeakersInfo.back().AzimuthAngle!=180)
            Skip_SB(                                            "alsoAddSymmetricPair");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daSpeakerDescription(speaker_layout& Layout)
{
    Element_Begin1("mpegh3daSpeakerDescription");
    TESTELSE_SB_SKIP(                                           "isCICPspeakerIdx");
        Skip_S1(7,                                              "CICPspeakerIdx");
    TESTELSE_SB_ELSE(                                           "isCICPspeakerIdx");
        int8u ElevationClass;
        Get_S1(2, ElevationClass,                               "ElevationClass");

        switch (ElevationClass)
        {
        case 0:
            Layout.SpeakersInfo.back().ElevationAngle=0;
            break;
        case 1:
            Layout.SpeakersInfo.back().ElevationAngle=35;
            break;
        case 2:
            Layout.SpeakersInfo.back().ElevationAngle=15;
            Layout.SpeakersInfo.back().ElevationDirection=true;
            break;
        case 3:
            int8u ElevationAngleIdx;
            Get_S1(Layout.SpeakersInfo.back().angularPrecision?7:5, ElevationAngleIdx, "ElevationAngleIdx");
            Layout.SpeakersInfo.back().ElevationAngle=ElevationAngleIdx*Layout.SpeakersInfo.back().angularPrecision?1:5;

            if (Layout.SpeakersInfo.back().ElevationAngle)
                Get_SB(Layout.SpeakersInfo.back().ElevationDirection, "ElevationDirection");
            break;
        }

        int8u AzimuthAngleIdx;
        Get_S1(Layout.SpeakersInfo.back().angularPrecision?8:6, AzimuthAngleIdx, "AzimuthAngleIdx");
        Layout.SpeakersInfo.back().AzimuthAngle=AzimuthAngleIdx*Layout.SpeakersInfo.back().angularPrecision?1:5;

        if (Layout.SpeakersInfo.back().AzimuthAngle && Layout.SpeakersInfo.back().AzimuthAngle!=180)
            Get_SB(Layout.SpeakersInfo.back().AzimuthDirection, "AzimuthDirection");

        Get_SB(Layout.SpeakersInfo.back().isLFE,                "isLFE");
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daFrame()
{
    Element_Begin1("mpegh3daFrame");
    Element_End0();

    FILLING_BEGIN();
        //Filling
        if (Status[IsAccepted])
            Finish("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Sync()
{
    //Parsing
    Skip_B1(                                                    "syncword");
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Streams_Fill_ChannelLayout(const string& Prefix, const speaker_layout& Layout, int8u speakerLayoutType)
{
    if (Aac_Channels_Get(Layout.ChannelLayout))
    {
        Fill(Stream_Audio, 0, (Prefix+"Channel(s)").c_str(), Aac_Channels_GetString(Layout.ChannelLayout));
        if (!Prefix.empty())
            Fill_SetOptions(Stream_Audio, 0, (Prefix + "Channel(s)").c_str(), "N NTY");
        if (!Prefix.empty())
        {
            string ChannelString=MediaInfoLib::Config.Language_Get(Ztring().From_UTF8(Aac_Channels_GetString(Layout.ChannelLayout)), __T(" channel")).To_UTF8();
            string ChannelMode=Aac_ChannelMode_GetString(Layout.ChannelLayout, true);
            if (ChannelMode.size()>3 || (ChannelMode.size()==3 && ChannelMode[2]!='0'))
                ChannelString+=" ("+Aac_ChannelMode_GetString(Layout.ChannelLayout, true)+")";
            Fill(Stream_Audio, 0, (Prefix+"Channel(s)/String").c_str(), ChannelString);
            Fill_SetOptions(Stream_Audio, 0, (Prefix + "Channel(s)/String").c_str(), "Y NTN");
        }
        Fill(Stream_Audio, 0, (Prefix+"ChannelPositions").c_str(), Aac_ChannelConfiguration_GetString(Layout.ChannelLayout));
        if (!Prefix.empty())
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"ChannelPositions").c_str(), "N YTY");
        Fill(Stream_Audio, 0, (Prefix+"ChannelPositions/String2").c_str(), Aac_ChannelConfiguration2_GetString(Layout.ChannelLayout));
        if (!Prefix.empty())
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"ChannelPositions/String2").c_str(), "N YTY");
        Fill(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), Aac_ChannelMode_GetString(Layout.ChannelLayout, true));
        Fill_SetOptions(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), "N NTY");
        Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Aac_ChannelLayout_GetString(Layout.ChannelLayout, true));
    }
    else if (Layout.numSpeakers)
    {
        if (speakerLayoutType==1) // Objects
            Fill(Stream_Audio, 0, (Prefix+"ObjectCount").c_str(), Layout.numSpeakers);
        else
            Fill(Stream_Audio, 0, (Prefix+"Channel(s)").c_str(), Layout.numSpeakers);
        if (!Layout.CICPspeakerIdxs.empty())
        {
            Fill(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), Aac_ChannelMode_GetString(Layout.CICPspeakerIdxs));
            Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Aac_ChannelLayout_GetString(Layout.CICPspeakerIdxs));
        }
    }
    else if (Layout.ChannelLayout)
    {
        Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Layout.ChannelLayout);
    }
}

//---------------------------------------------------------------------------
void File_Mpegh3da::escapedValue(int32u& Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name)
{
    Element_Begin1(Name);
    Get_S4(nBits1, Value, "nBits1");
    if (Value == ((1 << nBits1) - 1))
    {
        int32u ValueAdd;
        Get_S4(nBits2, ValueAdd, "nBits2");
        Value += ValueAdd;
        if (nBits3 && Value == ((1 << nBits2) - 1))
        {
            Get_S4(nBits3, ValueAdd, "nBits3");
            Value += ValueAdd;
        }
    }
    Element_Info1(Value);
    Element_End0();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_MPEGH3DA_YES

