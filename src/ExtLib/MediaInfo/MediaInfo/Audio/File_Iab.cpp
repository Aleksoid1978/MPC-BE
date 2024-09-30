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
#if defined(MEDIAINFO_IAB_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Iab.h"
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
const int32u Iab_SampleRate[4]=
{
    48000,
    96000,
    0,
    0,
};

//---------------------------------------------------------------------------
const int8u Iab_BitDepth[4]=
{
    16,
    24,
    0,
    0,
};

//---------------------------------------------------------------------------
const float32 Iab_FrameRate[16]=
{
    24,
    25,
    30,
    48,
    50,
    60,
    96,
    100,
    120,
    24000.0/1001.0,
    0,
    0,
    0,
    0,
    0,
    0,
};

//---------------------------------------------------------------------------
const char* Iab_Channel(int32u Code)
{
    static const char* Iab_Channel_Values[]=
    {
        "L",
        "Lc",
        "C",
        "Rc",
        "R",
        "Lss",
        "Ls",
        "Lb",
        "Rb",
        "Rss",
        "Rs",
        "Tsl",
        "Tsr",
        "LFE",
        "Left Height",
        "Right Height",
        "Center Height",
        "Left Surround Height",
        "Right Surround Height",
        "Left Side Surround Height",
        "Right Side Surround Height",
        "Left Rear Surround Height",
        "Right Rear Surround Height",
        "Tc",
        //0x18-0x7F reserved
        "Tfl",
        "Tfr",
        "Tbl",
        "Tbr",
        "Tsl",
        "Tsr",
        "LFE1",
        "LFE2",
        "Lw",
        "Rw",
    };
    if (Code<0x18)
        return Iab_Channel_Values[Code];
    if (Code>=0x80 && Code<sizeof(Iab_Channel_Values)/sizeof(const char*)+0x68)
        return Iab_Channel_Values[Code-0x68];
    return "";
}
//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Iab::File_Iab()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;

    //In
    Frame_Count_Valid=1;

    //Temp
    SampleRate=(int8u)-1;
    BitDepth=(int8u)-1;
    FrameRate=(int8u)-1;
}

//---------------------------------------------------------------------------
File_Iab::~File_Iab()
{
}

//***************************************************************************
// Streams management extra
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iab::Streams_Fill_ForAdm()
{
    if (Frame.Objects.empty())
        return;
    Fill(Stream_Audio, 0, "NumberOfProgrammes", 1);
    Fill(Stream_Audio, 0, "NumberOfContents", 1);
    Fill(Stream_Audio, 0, "NumberOfObjects", Frame.Objects.size());
    Fill(Stream_Audio, 0, "NumberOfPackFormats", Frame.Objects.size());
    size_t numberOfChannelFormats=0;
    for (const auto& Object : Frame.Objects)
    {
        numberOfChannelFormats+=Object.ChannelLayout.empty()?1:Object.ChannelLayout.size();
    }
    Fill(Stream_Audio, 0, "NumberOfChannelFormats", numberOfChannelFormats);
    Fill(Stream_Audio, 0, "NumberOfTrackUIDs", numberOfChannelFormats);
    Fill(Stream_Audio, 0, "NumberOfTrackFormats", numberOfChannelFormats);
    Fill(Stream_Audio, 0, "NumberOfStreamFormats", numberOfChannelFormats);

    {
        auto O="Programme0";
        Fill(Stream_Audio, 0, O, "Yes");
        Fill(Stream_Audio, 0, (string(O) + " Pos").c_str(), 0);
        Fill_SetOptions(Stream_Audio, 0, (string(O) + " Pos").c_str(), "N NIY");
        auto Content="Programme0 LinkedTo_Content_Pos";
        Fill(Stream_Audio, 0, Content, 0);
        Fill_SetOptions(Stream_Audio, 0, Content, "N NIY");
        auto Content_String="Programme0 LinkedTo_Content_Pos/String";
        Fill(Stream_Audio, 0, Content_String, 1);
        Fill_SetOptions(Stream_Audio, 0, Content_String, "Y NIN");
    }
    {
        auto O="Content0";
        Fill(Stream_Audio, 0, O, "Yes");
        Fill(Stream_Audio, 0, (string(O) + " Pos").c_str(), 0);
        Fill_SetOptions(Stream_Audio, 0, (string(O) + " Pos").c_str(), "N NIY");
        ZtringList SubstreamPos, SubstreamNum;
        for (size_t i=0; i<Frame.Objects.size(); i++)
        {
            SubstreamPos.push_back(Ztring::ToZtring(i));
            SubstreamNum.push_back(Ztring::ToZtring(i+1));
        }
        auto Object="Content0 LinkedTo_Object_Pos";
        SubstreamPos.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, Object, SubstreamPos.Read());
        Fill_SetOptions(Stream_Audio, 0, Object, "N NIY");
        auto Object_String="Content0 LinkedTo_Object_Pos/String";
        SubstreamNum.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, Object_String, SubstreamNum.Read());
        Fill_SetOptions(Stream_Audio, 0, Object_String, "Y NIN");
    }

    numberOfChannelFormats=0;
    for (size_t i=0; i<Frame.Objects.size(); i++)
    {
        const auto& Object=Frame.Objects[i];
        auto O="Object"+to_string(i);
        Fill(Stream_Audio, 0, O.c_str(), "Yes");
        Fill(Stream_Audio, 0, (O + " Pos").c_str(), i);
        Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
        auto PackFormat=O+" LinkedTo_PackFormat_Pos";
        Fill(Stream_Audio, 0, PackFormat.c_str(), i);
        Fill_SetOptions(Stream_Audio, 0, PackFormat.c_str(), "N NIY");
        Fill(Stream_Audio, 0, (PackFormat+"/String").c_str(), i+1);
        Fill_SetOptions(Stream_Audio, 0, (PackFormat+"/String").c_str(), "Y NIN");
        ZtringList SubstreamPos, SubstreamNum;
        if (Object.ChannelLayout.empty())
        {
            SubstreamPos.push_back(Ztring::ToZtring(numberOfChannelFormats));
            SubstreamNum.push_back(Ztring::ToZtring(numberOfChannelFormats+1));
            numberOfChannelFormats++;
        }
        else
        {
            for (size_t j=0; j<Object.ChannelLayout.size(); j++)
            {
                SubstreamPos.push_back(Ztring::ToZtring(numberOfChannelFormats));
                SubstreamNum.push_back(Ztring::ToZtring(numberOfChannelFormats+1));
                numberOfChannelFormats++;
            }
        }
        O+=" LinkedTo_TrackUID_Pos";
        SubstreamPos.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, O.c_str(), SubstreamPos.Read());
        Fill_SetOptions(Stream_Audio, 0, O.c_str(), "N NIY");
        SubstreamNum.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (O+"/String").c_str(), SubstreamNum.Read());
        Fill_SetOptions(Stream_Audio, 0, (O+"/String").c_str(), "Y NIN");
    }

    numberOfChannelFormats = 0;
    for (size_t i=0; i<Frame.Objects.size(); i++)
    {
        const auto& Object=Frame.Objects[i];
        auto O="PackFormat"+to_string(i);
        Fill(Stream_Audio, 0, O.c_str(), "Yes");
        Fill(Stream_Audio, 0, (O + " Pos").c_str(), i);
        Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
        ZtringList SubstreamPos, SubstreamNum, ChannelLayout;
        if (Object.ChannelLayout.empty())
        {
            SubstreamPos.push_back(Ztring::ToZtring(numberOfChannelFormats));
            SubstreamNum.push_back(Ztring::ToZtring(numberOfChannelFormats+1));
            numberOfChannelFormats++;
        }
        else
        {
            for (size_t j=0; j<Object.ChannelLayout.size(); j++)
            {
                SubstreamPos.push_back(Ztring::ToZtring(numberOfChannelFormats));
                SubstreamNum.push_back(Ztring::ToZtring(numberOfChannelFormats+1));
                numberOfChannelFormats++;
                ChannelLayout.push_back(Iab_Channel(Object.ChannelLayout[j]));
            }
        }
        Fill(Stream_Audio, 0, (O+" TypeDefinition").c_str(), Object.ChannelLayout.empty()?"Objects":"DirectSpeakers");
        ChannelLayout.Separator_Set(0, __T(" "));
        Fill(Stream_Audio, 0, (O+" ChannelLayout").c_str(), ChannelLayout.Read());
        O+=" LinkedTo_ChannelFormat_Pos";
        SubstreamPos.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, O.c_str(), SubstreamPos.Read());
        Fill_SetOptions(Stream_Audio, 0, O.c_str(), "N NIY");
        SubstreamNum.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (O+"/String").c_str(), SubstreamNum.Read());
        Fill_SetOptions(Stream_Audio, 0, (O+"/String").c_str(), "Y NIN");
    }

    numberOfChannelFormats=0;
    for (size_t i=0; i<Frame.Objects.size(); i++)
    {
        const auto& Object=Frame.Objects[i];
        auto O_Base="ChannelFormat";
        if (Object.ChannelLayout.empty())
        {
            auto O=O_Base+to_string(numberOfChannelFormats);
            Fill(Stream_Audio, 0, O.c_str(), "Yes");
            Fill(Stream_Audio, 0, (O + " Pos").c_str(), numberOfChannelFormats);
            Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
            Fill(Stream_Audio, 0, (O+" TypeDefinition").c_str(), "Objects");
            numberOfChannelFormats++;
        }
        else
        {
            for (size_t j=0; j<Object.ChannelLayout.size(); j++)
            {
                auto O=O_Base+to_string(numberOfChannelFormats);
                Fill(Stream_Audio, 0, O.c_str(), "Yes");
                Fill(Stream_Audio, 0, (O + " Pos").c_str(), numberOfChannelFormats);
                Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
                Fill(Stream_Audio, 0, (O+" TypeDefinition").c_str(), "DirectSpeakers");
                Fill(Stream_Audio, 0, (O+" ChannelLayout").c_str(), Iab_Channel(Object.ChannelLayout[j]));
                numberOfChannelFormats++;
            }
        }
    }

    numberOfChannelFormats=0;
    for (size_t i=0; i<Frame.Objects.size(); i++)
    {
        const auto& Object=Frame.Objects[i];
        auto O_Base="TrackUID";
        if (Object.ChannelLayout.empty())
        {
            auto O=O_Base+to_string(numberOfChannelFormats);
            Fill(Stream_Audio, 0, O.c_str(), "Yes");
            Fill(Stream_Audio, 0, (O + " Pos").c_str(), numberOfChannelFormats);
            Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
            if (Iab_SampleRate[SampleRate])
                Fill(Stream_Audio, 0, (O+" SamplingRate").c_str(), Iab_SampleRate[SampleRate]);
            if (Iab_BitDepth[BitDepth])
                Fill(Stream_Audio, 0, (O+" BitDepth").c_str(), Iab_BitDepth[BitDepth]);
            auto PackFormat=O+" LinkedTo_PackFormat_Pos";
            Fill(Stream_Audio, 0, PackFormat.c_str(), i);
            Fill_SetOptions(Stream_Audio, 0, PackFormat.c_str(), "N NIY");
            Fill(Stream_Audio, 0, (PackFormat+"/String").c_str(), i+1);
            Fill_SetOptions(Stream_Audio, 0, (PackFormat+"/String").c_str(), "Y NIN");
            auto TrackFormat=O+" LinkedTo_TrackFormat_Pos";
            Fill(Stream_Audio, 0, TrackFormat.c_str(), numberOfChannelFormats);
            Fill_SetOptions(Stream_Audio, 0, TrackFormat.c_str(), "N NIY");
            Fill(Stream_Audio, 0, (TrackFormat+"/String").c_str(), numberOfChannelFormats+1);
            Fill_SetOptions(Stream_Audio, 0, (TrackFormat+"/String").c_str(), "Y NIN");
            numberOfChannelFormats++;
        }
        else
        {
            for (size_t j=0; j<Object.ChannelLayout.size(); j++)
            {
                auto O=O_Base+to_string(numberOfChannelFormats);
                Fill(Stream_Audio, 0, O.c_str(), "Yes");
                Fill(Stream_Audio, 0, (O + " Pos").c_str(), numberOfChannelFormats);
                Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
                if (Iab_SampleRate[SampleRate])
                    Fill(Stream_Audio, 0, (O+" SamplingRate").c_str(), Iab_SampleRate[SampleRate]);
                if (Iab_BitDepth[BitDepth])
                    Fill(Stream_Audio, 0, (O+" BitDepth").c_str(), Iab_BitDepth[BitDepth]);
                auto PackFormat=O+" LinkedTo_PackFormat_Pos";
                Fill(Stream_Audio, 0, PackFormat.c_str(), i);
                Fill_SetOptions(Stream_Audio, 0, PackFormat.c_str(), "N NIY");
                Fill(Stream_Audio, 0, (PackFormat+"/String").c_str(), i+1);
                Fill_SetOptions(Stream_Audio, 0, (PackFormat+"/String").c_str(), "Y NIN");
                auto TrackFormat=O+" LinkedTo_TrackFormat_Pos";
                Fill(Stream_Audio, 0, TrackFormat.c_str(), numberOfChannelFormats);
                Fill_SetOptions(Stream_Audio, 0, TrackFormat.c_str(), "N NIY");
                Fill(Stream_Audio, 0, (TrackFormat+"/String").c_str(), numberOfChannelFormats+1);
                Fill_SetOptions(Stream_Audio, 0, (TrackFormat+"/String").c_str(), "Y NIN");
                numberOfChannelFormats++;
            }
        }
    }

    numberOfChannelFormats=0;
    for (size_t i=0; i<Frame.Objects.size(); i++)
    {
        const auto& Object=Frame.Objects[i];
        auto O_Base="TrackFormat";
        if (Object.ChannelLayout.empty())
        {
            auto O=O_Base+to_string(numberOfChannelFormats);
            Fill(Stream_Audio, 0, O.c_str(), "Yes");
            Fill(Stream_Audio, 0, (O + " Pos").c_str(), numberOfChannelFormats);
            Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
            Fill(Stream_Audio, 0, (O+" FormatDefinition").c_str(), "PCM");
            auto StreamFormat=O+" LinkedTo_StreamFormat_Pos";
            Fill(Stream_Audio, 0, StreamFormat.c_str(), numberOfChannelFormats);
            Fill_SetOptions(Stream_Audio, 0, StreamFormat.c_str(), "N NIY");
            Fill(Stream_Audio, 0, (StreamFormat+"/String").c_str(), numberOfChannelFormats+1);
            Fill_SetOptions(Stream_Audio, 0, (StreamFormat+"/String").c_str(), "Y NIN");
            numberOfChannelFormats++;
        }
        else
        {
            for (size_t j=0; j<Object.ChannelLayout.size(); j++)
            {
                auto O=O_Base+to_string(numberOfChannelFormats);
                Fill(Stream_Audio, 0, O.c_str(), "Yes");
                Fill(Stream_Audio, 0, (O + " Pos").c_str(), numberOfChannelFormats);
                Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
                Fill(Stream_Audio, 0, (O+" FormatDefinition").c_str(), "PCM");
                auto StreamFormat=O+" LinkedTo_StreamFormat_Pos";
                Fill(Stream_Audio, 0, StreamFormat.c_str(), numberOfChannelFormats);
                Fill_SetOptions(Stream_Audio, 0, StreamFormat.c_str(), "N NIY");
                Fill(Stream_Audio, 0, (StreamFormat+"/String").c_str(), numberOfChannelFormats+1);
                Fill_SetOptions(Stream_Audio, 0, (StreamFormat+"/String").c_str(), "Y NIN");
                numberOfChannelFormats++;
            }
        }
    }

    numberOfChannelFormats=0;
    for (size_t i=0; i<Frame.Objects.size(); i++)
    {
        const auto& Object=Frame.Objects[i];
        auto O_Base="StreamFormat";
        if (Object.ChannelLayout.empty())
        {
            auto O=O_Base+to_string(numberOfChannelFormats);
            Fill(Stream_Audio, 0, O.c_str(), "Yes");
            Fill(Stream_Audio, 0, (O + " Pos").c_str(), numberOfChannelFormats);
            Fill(Stream_Audio, 0, (O + " Format").c_str(), "PCM");
            Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
            auto ChannelFormat=O+" LinkedTo_ChannelFormat_Pos";
            Fill(Stream_Audio, 0, ChannelFormat.c_str(), numberOfChannelFormats);
            Fill_SetOptions(Stream_Audio, 0, ChannelFormat.c_str(), "N NIY");
            Fill(Stream_Audio, 0, (ChannelFormat+"/String").c_str(), numberOfChannelFormats+1);
            Fill_SetOptions(Stream_Audio, 0, (ChannelFormat+"/String").c_str(), "Y NIN");
            auto PackFormat=O+" LinkedTo_PackFormat_Pos";
            Fill(Stream_Audio, 0, PackFormat.c_str(), i);
            Fill_SetOptions(Stream_Audio, 0, PackFormat.c_str(), "N NIY");
            Fill(Stream_Audio, 0, (PackFormat+"/String").c_str(), i+1);
            Fill_SetOptions(Stream_Audio, 0, (PackFormat+"/String").c_str(), "Y NIN");
            auto TrackFormat=O+" LinkedTo_TrackFormat_Pos";
            Fill(Stream_Audio, 0, TrackFormat.c_str(), numberOfChannelFormats);
            Fill_SetOptions(Stream_Audio, 0, TrackFormat.c_str(), "N NIY");
            Fill(Stream_Audio, 0, (TrackFormat+"/String").c_str(), numberOfChannelFormats+1);
            Fill_SetOptions(Stream_Audio, 0, (TrackFormat+"/String").c_str(), "Y NIN");
            numberOfChannelFormats++;
        }
        else
        {
            for (size_t j=0; j<Object.ChannelLayout.size(); j++)
            {
                auto O=O_Base+to_string(numberOfChannelFormats);
                Fill(Stream_Audio, 0, O.c_str(), "Yes");
                Fill(Stream_Audio, 0, (O + " Pos").c_str(), numberOfChannelFormats);
                Fill_SetOptions(Stream_Audio, 0, (O + " Pos").c_str(), "N NIY");
                auto ChannelFormat=O+" LinkedTo_ChannelFormat_Pos";
                Fill(Stream_Audio, 0, ChannelFormat.c_str(), numberOfChannelFormats);
                Fill_SetOptions(Stream_Audio, 0, ChannelFormat.c_str(), "N NIY");
                Fill(Stream_Audio, 0, (ChannelFormat+"/String").c_str(), numberOfChannelFormats+1);
                Fill_SetOptions(Stream_Audio, 0, (ChannelFormat+"/String").c_str(), "Y NIN");
                auto PackFormat=O+" LinkedTo_PackFormat_Pos";
                Fill(Stream_Audio, 0, PackFormat.c_str(), i);
                Fill_SetOptions(Stream_Audio, 0, PackFormat.c_str(), "N NIY");
                Fill(Stream_Audio, 0, (PackFormat+"/String").c_str(), i+1);
                Fill_SetOptions(Stream_Audio, 0, (PackFormat+"/String").c_str(), "Y NIN");
                auto TrackFormat=O+" LinkedTo_TrackFormat_Pos";
                Fill(Stream_Audio, 0, TrackFormat.c_str(), numberOfChannelFormats);
                Fill_SetOptions(Stream_Audio, 0, TrackFormat.c_str(), "N NIY");
                Fill(Stream_Audio, 0, (TrackFormat+"/String").c_str(), numberOfChannelFormats+1);
                Fill_SetOptions(Stream_Audio, 0, (TrackFormat+"/String").c_str(), "Y NIN");
                numberOfChannelFormats++;
            }
        }
    }
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iab::Streams_Fill()
{
    //Filling
    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "IAB");
    Fill(Stream_Audio, 0, Audio_Format_Info, "Immersive Audio Bitstream");
    Fill(Stream_Audio, 0, Audio_Format_Version, __T("Version ")+Ztring::ToZtring(Version));
    if (Iab_SampleRate[SampleRate])
        Fill(Stream_Audio, 0, Audio_SamplingRate, Iab_SampleRate[SampleRate]);
    if (Iab_BitDepth[BitDepth])
        Fill(Stream_Audio, 0, Audio_BitDepth, Iab_BitDepth[BitDepth]);
    if (Iab_FrameRate[FrameRate])
        Fill(Stream_Audio, 0, Audio_FrameRate, Iab_FrameRate[FrameRate]);
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iab::Header_Parse()
{
    //Parsing
    int32u PreambleLength, IAFrameLength, ElementID, ElementSize;
    if (Element_Level==2)
    {
        int8u PreambleTag, IAFrameTag;
        Get_B1 (PreambleTag,                                    "PreambleTag");
        Get_B4 (PreambleLength,                                 "PreambleLength");
        Skip_XX(PreambleLength,                                 "PreambleValue");
        Get_B1 (IAFrameTag,                                     "IAFrameTag");
        Get_B4 (IAFrameLength,                                  "IAFrameLength");

        FILLING_BEGIN();
            if (!Status[IsAccepted] && PreambleTag==1 && IAFrameTag==2)
                Accept();
        FILLING_END();

        //Filling
        Header_Fill_Size(Element_Offset+IAFrameLength);
        Header_Fill_Code(0, "IAB");
    }
    else
    {
        Get_Plex8 (ElementID,                                   "ElementID");
        Get_Plex8 (ElementSize,                                 "ElementSize");

        //Filling
        Header_Fill_Size(Element_Offset+ElementSize);
        Header_Fill_Code(ElementID, "IAElement");
    }
}

//---------------------------------------------------------------------------
void File_Iab::Data_Parse()
{
    if (Element_Level==1)
    {
        Element_Info1(Frame_Count);
        Element_ThisIsAList();
        return;
    }

    //Parsing
    switch (Element_Code)
    {
        case 0x00000008: Element_Name("IAFrame"); IAFrame(); break;
        case 0x00000010: Element_Name("Bed Definition"); BedDefinition(); break;
        case 0x00000020: Element_Name("Bed Remap"); BedRemap(); break;
        case 0x00000040: Element_Name("Object Definition"); ObjectDefinition(); break;
        case 0x00000400: Element_Name("Audio Data PCM"); AudioDataPCM(); break;
        default: Element_Name(Ztring().From_CC4((int32u)Element_Code)); Skip_XX(Element_Size, "Data");
    }

    if ((Element_Code!=0x00000008 || Element_Offset==Element_Size) && Element_Size>=Element_TotalSize_Get(Element_Level-1))
    {
        Frame.Objects=std::move(F.Objects);
        Frame_Count++;
        if (!Status[IsFilled] && Frame_Count>=Frame_Count_Valid)
            Finish();
    }
}

//---------------------------------------------------------------------------
void File_Iab::IAFrame()
{
    //Parsing
    int32u MaxRendered, SubElementCount;
    Get_B1 (Version,                                                "Version");
    if (Version==1)
    {
        BS_Begin();
        Get_S1 (2, SampleRate,                                      "SampleRate"); Param_Info2C(Iab_SampleRate[SampleRate], Iab_SampleRate[SampleRate], " Hz");
        Get_S1 (2, BitDepth,                                        "BitDepth"); Param_Info2C(Iab_BitDepth[BitDepth], Iab_BitDepth[BitDepth], " bits");
        Get_S1 (4, FrameRate,                                       "FrameRate"); Param_Info2C(Iab_FrameRate[FrameRate], Iab_FrameRate[FrameRate], " FPS");
        BS_End();
        Get_Plex8 (MaxRendered,                                     "MaxRendered");
        Get_Plex8 (SubElementCount,                                 "SubElementCount");
        Element_ThisIsAList();
        Frame.Objects=std::move(F.Objects);
    }
    else
        Skip_XX(Element_Size-Element_Offset,                        "Unknown");
}

//---------------------------------------------------------------------------
void File_Iab::BedDefinition()
{
    F.Objects.resize(F.Objects.size()+1);

    //Parsing
    int32u ChannelCount;
    int8u AudioDescription;
    bool ConditionalBed;
    Skip_Plex8(                                                 "MetaID");
    BS_Begin();
    Get_SB (ConditionalBed,                                     "ConditionalBed");
    if (ConditionalBed)
    {
        Skip_S1(8,                                              "BedUseCase");
    }
    Get_Plex(4, ChannelCount,                                   "ChannelCount");
    for (int32u n=0; n<ChannelCount; n++)
    {
        Element_Begin1("Channel");
        int32u ChannelID;
        int8u ChannelGainPrefix;
        bool ChannelDecorInfoExists;
        Get_Plex (4, ChannelID,                                 "ChannelID"); Element_Info1(Iab_Channel(ChannelID));
        Skip_Plex(8,                                            "AudioDataID");
        Get_S1 (2, ChannelGainPrefix,                           "ChannelGainPrefix");
        if (ChannelGainPrefix>1)
            Skip_S1(10,                                         "ChannelGain");
        Get_SB (ChannelDecorInfoExists,                         "ChannelDecorInfoExists");
        if (ChannelDecorInfoExists)
        {
            int8u ChannelDecorCoefPrefix;
            Skip_S2(2,                                          "Reserved");
            Get_S1 (2, ChannelDecorCoefPrefix,                  "ChannelDecorCoefPrefix");
            if (ChannelDecorCoefPrefix>1)
                Skip_S1(10,                                     "ChannelDecorCoef");
        }
        Element_End0();
        F.Objects.back().ChannelLayout.push_back(ChannelID);
    }
    Skip_S2(10,                                                 "0x180");
    if (Data_BS_Remain()%8)
        Skip_S1(Data_BS_Remain()%8,                             "AlignBits");
    BS_End();
    Get_B1 (AudioDescription,                                   "AudioDescription");
    if (AudioDescription&0x80)
    {
        size_t Pos=(size_t)Element_Offset+1;
        while (Pos<Element_Size-1 && Buffer[Buffer_Offset+Pos])
            Pos++;
        Skip_XX(Pos-Element_Offset,                             "AudioDescriptionText");
    }
    Skip_B1(                                                    "SubElementCount");
    Element_ThisIsAList();
}

//---------------------------------------------------------------------------
void File_Iab::ObjectDefinition()
{
    F.Objects.resize(F.Objects.size()+1);

    //Parsing
    int8u AudioDescription;
    bool ConditionalBed;
    Skip_Plex8(                                                 "MetaID");
    Skip_Plex8(                                                 "AudioDataID");
    BS_Begin();
    Get_SB (ConditionalBed,                                     "ConditionalBed");
    if (ConditionalBed)
    {
        Skip_SB(                                                "1");
        Skip_S1(8,                                              "ObjectUseCase");
    }
    Skip_SB(                                                    "0");
    int32u NumPanSubBlocks=8;
    for (int32u sb=0; sb<NumPanSubBlocks; sb++)
    {
        Element_Begin1("PanSubBlock");
        bool PanInfoExists;
        if (!sb)
            PanInfoExists=true;
        else
            Get_SB (PanInfoExists,                              "PanInfoExists");
        if (PanInfoExists)
        {
            int8u ObjectGainPrefix;
            bool ObjectSnap, ObjectZoneControl;
            Get_S1 (2, ObjectGainPrefix,                        "ObjectGainPrefix");
            if (ObjectGainPrefix>1)
                Skip_S1(10,                                     "ObjectGainPrefix");
            Skip_S1( 3,                                         "b001");
            Skip_S2(16,                                         "ObjectPosX");
            Skip_S2(16,                                         "ObjectPosY");
            Skip_S2(16,                                         "ObjectPosZ");
            Get_SB (ObjectSnap,                                 "ObjectSnap");
            if (ObjectSnap)
            {
                bool ObjectSnapTolExists;
                Get_SB (ObjectSnapTolExists,                    "ObjectSnapTolExists");
                if (ObjectSnapTolExists)
                {
                    Skip_S2(12,                                 "ObjectSnapTolerance");
                }
                Skip_SB(                                        "0");
            }
            Get_SB (ObjectZoneControl,                          "ObjectZoneControl");
            if (ObjectZoneControl)
            {
                for (int n=0; n<9; n++)
                {
                    int8u ZoneGainPrefix;
                    Get_S1 (2, ZoneGainPrefix,                  "ZoneGainPrefix");
                    if (ZoneGainPrefix>1)
                        Skip_S1(10,                             "ZoneGain");
                }
            }
            int8u ObjectSpreadMode;
            Get_S1 (2, ObjectSpreadMode,                        "ObjectSpreadMode");
            switch (ObjectSpreadMode)
            {
                case 0:
                case 2:
                {
                    Skip_S1( 8,                                 "ObjectSpread");
                }
                break;
                case 3:
                {
                    Skip_S2(12,                                 "ObjectSpreadX");
                    Skip_S2(12,                                 "ObjectSpreadY");
                    Skip_S2(12,                                 "ObjectSpreadZ");
                }
                break;
                default: ;
            }
            Skip_S1(4,                                          "0");
            int8u ObjectDecorCoefPrefix;
            Get_S1 (2, ObjectDecorCoefPrefix,                   "ObjectDecorCoefPrefix");
            if (ObjectDecorCoefPrefix>1)
                Skip_S1(8,                                      "ObjectDecorCoefPrefix");
        }
        Element_End0();
    }
    BS_End();
    Get_B1 (AudioDescription,                                   "AudioDescription");
    if (AudioDescription&0x80)
    {
        size_t Pos=(size_t)Element_Offset+1;
        while (Pos<Element_Size-1 && Buffer[Buffer_Offset+Pos])
            Pos++;
        Skip_XX(Pos-Element_Offset,                             "AudioDescriptionText");
    }
    Skip_B1(                                                    "SubElementCount");
    Element_ThisIsAList();
}

//---------------------------------------------------------------------------
void File_Iab::BedRemap()
{
    int32u SourceChannels, DestinationChannels;
    Skip_Plex8(                                                 "MetaID");
    Skip_B1 (                                                   "RemapUseCase");
    BS_Begin();
    Get_Plex (4, SourceChannels,                                "SourceChannels");
    Get_Plex (4, DestinationChannels,                           "DestinationChannels");
    int32u NumPanSubBlocks=8;
    for (int32u sb=0; sb<NumPanSubBlocks; sb++)
    {
        Element_Begin1("PanSubBlock");
        bool RemapInfoExists;
        if (!sb)
            RemapInfoExists=true;
        else
            Get_SB (RemapInfoExists,                            "RemapInfoExists");
        if (RemapInfoExists)
        {
            for(int32u oChan=0; oChan<DestinationChannels; oChan++)
            {
                Skip_Plex(4,                                    "DestinationChannelID");
                for (int32u iChan=0; iChan<SourceChannels; iChan++)
                {
                    int8u RemapGainPrefix;
                    Get_S1 (2, RemapGainPrefix,                 "RemapGainPrefix");
                    if (RemapGainPrefix>1)
                        Skip_S1(10,                             "RemapGain");
                }
            }
        }
        Element_End0();
    }
    BS_End();
}

//---------------------------------------------------------------------------
void File_Iab::AudioDataPCM()
{
    //Parsing
    Skip_Plex8(                                                 "MetaID");
    Skip_XX(Element_Size-Element_Offset,                        "PCMData");
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iab::Get_Plex(int8u Bits, int32u& Info, const char* Name)
{
    for (;;)
    {
        Peek_BS(Bits, Info);
        if (Info!=(1<<Bits)-1 || Bits>=32)
        {
            Get_BS(Bits, Info, Name);
            return;
        }
        BS->Skip(Bits);
        Bits<<=1;
    }
}

//---------------------------------------------------------------------------
void File_Iab::Get_Plex8(int32u& Info, const char* Name)
{
    //Element size
    int8u Info8;
    Peek_B1(Info8);
    if (Info8!=0xFF)
    {
        Get_B1(Info8, Name);
        Info=Info8;
        return;
    }
    Element_Offset++;
    int16u Info16;
    Peek_B2(Info16);
    if (Info16!=0xFFFF)
    {
        Get_B2(Info16, Name);
        Info=Info16;
        return;
    }
    Element_Offset+=2;
    Get_B4(Info, Name);
}

} //NameSpace

#endif //MEDIAINFO_IAB_YES
