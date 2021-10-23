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
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

const int32u Iab_SampleRate[4]=
{
    48000,
    96000,
    0,
    0,
};

const int8u Iab_BitDepth[4]=
{
    16,
    24,
    0,
    0,
};

const float32 Iab_FrameRate[16] =
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
    Frame_Count_Valid=2;

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
        Header_Fill_Code(0, "IAFrame");
    }
    else
    {
        Get_Flex8 (ElementID,                                   "ElementID");
        Get_Flex8 (ElementSize,                                 "ElementSize");

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
        case 0x00000008: Element_Name("Frame"); FrameHeader(); break;
        case 0x00000010: Element_Name("Bed Definition"); BedDefinition(); break;
        case 0x00000040: Element_Name("Bed Remap"); BedRemap(); break;
        default: Element_Name(Ztring().From_CC4((int32u)Element_Code)); Skip_XX(Element_Size, "Data");
    }

    //Finish();
}

//---------------------------------------------------------------------------
void File_Iab::FrameHeader()
{
    //Parsing
    Element_Begin1("Frame Header");
    int32u MaxRendered, SubElementCount;
    Get_B1 (Version,                                            "Version");
    if (Version==1)
    {
        BS_Begin();
        Get_S1 (2, SampleRate,                                      "SampleRate"); Param_Info2C(Iab_SampleRate[SampleRate], Iab_SampleRate[SampleRate], " Hz");
        Get_S1 (2, BitDepth,                                        "BitDepth"); Param_Info2C(Iab_BitDepth[BitDepth], Iab_BitDepth[BitDepth], " bits");
        Get_S1 (4, FrameRate,                                       "FrameRate"); Param_Info2C(Iab_FrameRate[FrameRate], Iab_FrameRate[FrameRate], " FPS");
        BS_End();
        Get_Flex8(MaxRendered,                                      "MaxRendered");
        Get_Flex8(SubElementCount,                                  "SubElementCount");
        Element_End0();
        Element_ThisIsAList();
    }
    else
        Skip_XX(Element_Size-Element_Offset,                        "Unknown");

    FILLING_BEGIN();
        Frame_Count++;
        if (!Status[IsFilled] && Frame_Count>=Frame_Count_Valid)
            Finish();
    FILLING_END();
}


//---------------------------------------------------------------------------
void File_Iab::BedDefinition()
{
}

//---------------------------------------------------------------------------
void File_Iab::BedRemap()
{
}

//---------------------------------------------------------------------------
void File_Iab::Get_Flex8(int32u& Info, const char* Name)
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
