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
#if defined(MEDIAINFO_H263_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_H263.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "ZenLib/BitStream.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace MediaInfoLib
{
//---------------------------------------------------------------------------

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* H263_Source_Format[]=
{
    nullptr,
    "sub-QCIF",
    "QCIF",
    "CIF",
    "4CIF",
    "16CIF",
    nullptr,
    nullptr,
};

//---------------------------------------------------------------------------
static const int16u H263_Source_Format_Width[]=
{
    0,
    128,
    176,
    352,
    704,
    1408,
    0,
    0,
};

//---------------------------------------------------------------------------
static const int16u H263_Source_Format_Height[]=
{
    0,
    96,
    144,
    288,
    576,
    1152,
    0,
    0,
};

//---------------------------------------------------------------------------
static const int8u H263_PAR_W[]=
{
    0,
    12,
    10,
    16,
    40,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

static const int8u H263_PAR_H[]=
{
    0,
    11,
    11,
    11,
    33,
    0,
    0,
    0,
    0,
    0,
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
File_H263::File_H263()
:File__Analyze()
{
    //Configuration
    ParserName="H.263";
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    StreamSource=IsStream;
    Buffer_TotalBytes_FirstSynched_Max=1024*1024;
    Trusted_Multiplier=2;

    //In
    Frame_Count_Valid=0;
    FrameIsAlwaysComplete=false;
}

//---------------------------------------------------------------------------
File_H263::~File_H263()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_H263::Streams_Accept()
{
    Stream_Prepare(Stream_Video);
}

//---------------------------------------------------------------------------
void File_H263::Streams_Update()
{
}

//---------------------------------------------------------------------------
void File_H263::Streams_Fill()
{
    Fill(Stream_General, 0, General_Format_Version, "H.263");
    Fill(Stream_Video, 0, Video_Format, "H.263");
    Fill(Stream_Video, 0, Video_Codec, "H.263");

    if (H263_Source_Format_Width[Source_Format])
        Fill(Stream_Video, 0, Video_Width, H263_Source_Format_Width[Source_Format]);
    if (H263_Source_Format_Height[Source_Format])
        Fill(Stream_Video, 0, Video_Height, H263_Source_Format_Height[Source_Format]);
    Fill(Stream_Video, 0, Video_ColorSpace, "YUV");
    Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:2:0");
    Fill(Stream_Video, 0, Video_BitDepth, 8);
    if (PAR_W && PAR_H)
        Fill(Stream_Video, 0, Video_PixelAspectRatio, ((float32)PAR_W)/PAR_H, 3);
}

//---------------------------------------------------------------------------
void File_H263::Streams_Finish()
{
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_H263::Synchronize()
{
    //Synchronizing
    while(Buffer_Offset+5<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                        || Buffer[Buffer_Offset+1]!=0x00
                                        || (Buffer[Buffer_Offset+2]&0xFC)!=0x80
                                        || (Buffer[Buffer_Offset+3]&0x03)!=0x02
                                        || (Buffer[Buffer_Offset+4]&0x1C)==0x00))
    {
        Buffer_Offset+=2;
        while(Buffer_Offset<Buffer_Size && Buffer[Buffer_Offset]!=0x00)
            Buffer_Offset+=2;
        if ((Buffer_Offset<Buffer_Size && Buffer[Buffer_Offset-1]==0x00) || Buffer_Offset>=Buffer_Size)
            Buffer_Offset--;
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+4==Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                      || Buffer[Buffer_Offset+1]!=0x00
                                      || (Buffer[Buffer_Offset+2]&0xFC)!=0x80
                                      || (Buffer[Buffer_Offset+3]&0x03)!=0x02))
        Buffer_Offset++;
    if (Buffer_Offset+3==Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                      || Buffer[Buffer_Offset+1]!=0x00
                                      || (Buffer[Buffer_Offset+2]&0xFC)!=0x80))
        Buffer_Offset++;
    if (Buffer_Offset+2==Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                      || Buffer[Buffer_Offset+1]!=0x00))
        Buffer_Offset++;
    if (Buffer_Offset+1==Buffer_Size &&  Buffer[Buffer_Offset  ]!=0x00)
        Buffer_Offset++;

    if (Buffer_Offset+5>Buffer_Size)
    {
        if (Frame_Count==0 && Buffer_TotalBytes+Buffer_Offset>Buffer_TotalBytes_FirstSynched_Max)
            Reject();
        return false;
    }

    //Synched is OK
    Synched=true;
    return true;
}

//---------------------------------------------------------------------------
bool File_H263::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Size-Buffer_Offset<=4)
        return false;

    //Quick test of synchro
    if (Buffer[Buffer_Offset  ]!=0x00
     || Buffer[Buffer_Offset+1]!=0x00
     || (Buffer[Buffer_Offset+2]&0xFC)!=0x80
     || (Buffer[Buffer_Offset+3]&0x03)!=0x02
     || (Buffer[Buffer_Offset+4]&0x1C)==0x00
     || (Buffer_Size>=0x100000 && Header_Parser_GetSize()==(size_t)-1)) //Preventing waiting too much after a false positive sync
    {
        if (Frame_Count==0 && Buffer_TotalBytes>Buffer_TotalBytes_FirstSynched_Max)
            Trusted=0;
        if (!Status[IsFilled])
            Frame_Count=0; // Back to start of sync
        Trusted_IsNot("Sync issue");
        Buffer_Offset++;
        Synched=false;
        return true;
    }

    //We continue
    return true;
}

//---------------------------------------------------------------------------
void File_H263::Synched_Init()
{
    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?8:(IsSub?1:2);

    //Temp
    PAR_W=12;
    PAR_H=11;
    Temporal_Reference_IsValid=false;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_H263::Read_Buffer_Unsynched()
{
    Temporal_Reference_IsValid=false;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_H263::Header_Parse()
{
    Header_Fill_Code(0x00, "Frame");
    auto Data_Size=Header_Parser_GetSize();
    if (Data_Size!=(size_t)-1)
        Header_Fill_Size(Data_Size);
}

//---------------------------------------------------------------------------
size_t File_H263::Header_Parser_GetSize()
{
    //Look for next Sync word
    if (Buffer_Offset_Temp==0) //Buffer_Offset_Temp is not 0 if Header_Parse_Fill_Size() has already parsed first frames
        Buffer_Offset_Temp=Buffer_Offset+3;
    while (Buffer_Offset_Temp+3<=Buffer_Size
        && (Buffer[Buffer_Offset_Temp  ]!=0x00
         || Buffer[Buffer_Offset_Temp+1]!=0x00
         || (Buffer[Buffer_Offset_Temp+2]&0xFC)!=0x80))
    {
        Buffer_Offset_Temp+=2;
        while(Buffer_Offset_Temp<Buffer_Size && Buffer[Buffer_Offset_Temp]!=0x00)
            Buffer_Offset_Temp+=2;
        if (Buffer_Offset_Temp>=Buffer_Size || Buffer[Buffer_Offset_Temp-1]==0x00)
            Buffer_Offset_Temp--;
    }

    //Must wait more data?
    if (Buffer_Offset_Temp+3>Buffer_Size)
    {
        if (FrameIsAlwaysComplete || Config->IsFinishing)
            Buffer_Offset_Temp=Buffer_Size; //We are sure that the next bytes are a start
        else
            return false;
    }

    //OK, we continue
    auto Data_Size=Buffer_Offset_Temp-Buffer_Offset;
    Buffer_Offset_Temp=0;
    return Data_Size;

}

//---------------------------------------------------------------------------
void File_H263::Data_Parse()
{
    //Parsing
    int8u Temporal_Reference_Temp;
    BS_Begin();
    Skip_S3(22,                                                 "Picture Start Code (PSC)");
    Get_S1 ( 8, Temporal_Reference_Temp,                        "Temporal Reference (TR)");
    if (!Temporal_Reference_IsValid)
    {
        Temporal_Reference=Temporal_Reference_Temp;
        Temporal_Reference_IsValid=true;
    }
    if (Temporal_Reference_Temp==Temporal_Reference || ((int8u)(Temporal_Reference_Temp-Temporal_Reference)>8 && (int8u)(Temporal_Reference-Temporal_Reference_Temp)>8))
    {
        if (Frame_Count==0 && Buffer_TotalBytes>Buffer_TotalBytes_FirstSynched_Max)
            Trusted=0;
        if (!Status[IsFilled])
            Frame_Count=0; // Back to start of sync
        Trusted_IsNot("Out of Order");
        return;
    }
    Temporal_Reference=Temporal_Reference_Temp;
    Element_Begin1("Type Information (PTYPE)");
        Mark_1();
        Mark_0();
        Skip_SB(                                                "Split screen indicator");
        Skip_SB(                                                "Document camera indicator");
        Skip_SB(                                                "Full Picture Freeze Release");
        Get_S1 (3, Source_Format,                               "Source Format"); Param_Info1C(H263_Source_Format[Source_Format], H263_Source_Format[Source_Format]);
        if (Source_Format!=7)
        {
            Skip_SB(                                            "Picture Coding Type");
            Skip_SB(                                            "Unrestricted Motion Vector mode");
            Skip_SB(                                            "Syntax-based Arithmetic Coding mode");
            Skip_SB(                                            "Advanced Prediction mode");
            Skip_SB(                                            "PB-frames mode");
        }
    Element_End0();
    if (Source_Format==7) // Extended PTYPE
    {
        Element_Begin1("Plus PTYPE (PLUSPTYPE)");
            int8u Ufep, PixelAspectRatioCode=0, Width=0, Height=0;
            Get_S1 ( 3, Ufep,                                   "Update Full Extended PTYPE (UFEP)");
            switch (Ufep)
            {
                case 0  :
                            break;
                case 1  :
                            Element_Begin1("Optional Part of PLUSPTYPE (OPPTYPE)");
                            Get_S1 (3, Source_Format,           "Source Format"); Param_Info1C(H263_Source_Format[Source_Format], H263_Source_Format[Source_Format]);
                            Skip_SB(                            "Custom PCF");
                            Skip_SB(                            "Unrestricted Motion Vector (UMV) mode");
                            Skip_SB(                            "Syntax-based Arithmetic Coding (SAC) mode");
                            Skip_SB(                            "Advanced Prediction (AP) mode");
                            Skip_SB(                            "Advanced INTRA Coding (AIC) mode");
                            Skip_SB(                            "Deblocking Filter (DF) mode");
                            Skip_SB(                            "Slice Structured (SS) mode");
                            Skip_SB(                            "Reference Picture Selection (RPS) mode");
                            Skip_SB(                            "Independent Segment Decoding (ISD) mode");
                            Skip_SB(                            "Alternative INTER VLC (AIV) mode");
                            Skip_SB(                            "Modified Quantization (MQ) mode");
                            Mark_1();
                            Mark_0();
                            Mark_0();
                            Mark_0();
                            Element_End0();
                            break;
                default :
                            BS_End();
                            Skip_XX(Element_Size-Element_Offset, "Unknown");
                            return; //TODO: frame count...
            }
            Element_Begin1("mandatory part of PLUSPTYPE when PLUSPTYPE present (MPPTYPE)");
                Skip_S1(3,                                      "Picture Type Code");
                Skip_SB(                                        "Reference Picture Resampling (RPR) mode");
                Skip_SB(                                        "Reduced-Resolution Update (RRU) mode");
                Skip_SB(                                        "Rounding Type (RTYPE)");
                Mark_0();
                Mark_0();
                Mark_1();
            Element_End0();
        Element_End0();
        Skip_SB(                                                "CPM");
        Skip_S1(2,                                              "PSBI");
        Element_Begin1("Custom Picture Format (CPFMT)");
            Get_S1 (4, PixelAspectRatioCode,                    "Pixel Aspect Ratio Code");
            Get_S1 (4, Width,                                   "Picture Width Indication");
            Width++; Width<<=2; Param_Info2(Width, " pixels");
            Mark_1();
            Get_S1 (4, Height,                                  "Picture Height Indication");
            Height<<=2; Param_Info2(Height, " pixels");
        Element_End0();
        if (PixelAspectRatioCode==0xF)
        {
            Element_Begin1("Extended Pixel Aspect Ratio (EPAR)");
            Get_S1 (8, PAR_W,                                   "PAR Width");
            Get_S1 (8, PAR_H,                                   "PAR Height");
            Element_End0();
        }
        else
        {
            PAR_W=H263_PAR_W[PixelAspectRatioCode];
            PAR_H=H263_PAR_H[PixelAspectRatioCode];
        }
    }
    BS_End();
    Skip_XX(Element_Size-Element_Offset,                        "Other data");

    FILLING_BEGIN();
        if (!H263_Source_Format[Source_Format])
        {
            if (Frame_Count==0 && Buffer_TotalBytes>Buffer_TotalBytes_FirstSynched_Max)
                Trusted=0;
            if (!Status[IsFilled])
                Frame_Count=0; // Back to start of sync
            Trusted_IsNot("Source_Format");
            return;
        }

        Element_Info1(Frame_Count);
        Frame_Count++;

        //Filling
        if (!Status[IsFilled] && Frame_Count>=Frame_Count_Valid)
        {
            Accept("H.263");
            Finish("H.263");
        }
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_H263_YES
