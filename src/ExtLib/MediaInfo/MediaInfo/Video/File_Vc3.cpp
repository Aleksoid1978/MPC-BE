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
#if defined(MEDIAINFO_VC3_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/Video/File_Vc3.h"
#if defined(MEDIAINFO_CDP_YES)
    #include "MediaInfo/Text/File_Cdp.h"
#endif
#include <sstream>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Vc3_Profile[2]=
{
    "HD",
    "RI",
};

//---------------------------------------------------------------------------
static const char* Vc3_FromCID_Profile(int32u CompressionID)
{
    if (CompressionID>=1235 && CompressionID<=1260)
        return Vc3_Profile[0];
    if (CompressionID>=1270 && CompressionID<=1275)
        return Vc3_Profile[1];
    return "";
}

//---------------------------------------------------------------------------
static const char* Vc3_Level[6]=
{
    "444",
    "HQX",
    "HQ",
    "SQ",
    "LB",
    "TR",
};

//---------------------------------------------------------------------------
static const char* Vc3_FromCID_Level(int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1256 :
        case 1270 :
                    return Vc3_Level[0];
        case 1235 :
        case 1241 :
        case 1250 :
        case 1271 :
                    return Vc3_Level[1];
        case 1238 :
        case 1243 :
        case 1251 :
        case 1272 :
                    return Vc3_Level[2];
        case 1237 :
        case 1242 :
        case 1252 :
        case 1273 :
                    return Vc3_Level[3];
        case 1253 :
        case 1274 :
                    return Vc3_Level[4];
        case 1244 :
        case 1258 :
        case 1259 :
        case 1260 :
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const bool Vc3_FromCID_IsSupported (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1235 :
        case 1237 :
        case 1238 :
        case 1241 :
        case 1242 :
        case 1243 :
        case 1250 :
        case 1251 :
        case 1252 :
        case 1253 :
        case 1256 :
        case 1258 :
        case 1259 :
        case 1260 :
                    return true;
        default   : return false;
    }
}

//---------------------------------------------------------------------------
static const int32u Vc3_CompressedFrameSize_RI(int64u Size, int16u Width, int16u Height)
{
    int32u WidthBlock = Width / 16;
    if (Width % 16)
        WidthBlock++; // Additional block with padding
    int32u HeightBlock = Height / 16;
    if (Height % 16)
        HeightBlock++; // Additional block with padding
    Size *= WidthBlock * HeightBlock;
    if (false) //TODO: Alpha
        Size /= 12240;
    else
        Size /= 8160;

    int32u Remaining = Size % 4096;
    if (Remaining >= 2048) // Round-up limit
        Size += 4096 - Remaining; // Round up
    else
        Size -= Remaining; // Round down
    if (Size < 8192) // Lower size limit
        Size = 8192;

    return (int32u)Size;
}

//---------------------------------------------------------------------------
static const int32u Vc3_CompressedFrameSize(int32u CompressionID, int16u Width, int16u Height)
{
    int32u Size;
    switch (CompressionID)
    {
        case 1253 : 
        case 1274 :
                    Size= 188416; break;
        case 1258 : 
                    Size= 212992; break;
        case 1252 :
                    Size= 303104; break;
        case 1259 : 
        case 1260 :
                    Size= 417792; break;
        case 1250 : 
        case 1251 :
                    Size= 458752; break;
        case 1237 : 
        case 1242 : 
        case 1244 :
        case 1273 :
                    Size= 606208; break;
        case 1235 :
        case 1238 : 
        case 1241 : 
        case 1243 :
        case 1271 :
        case 1272 :
                    Size= 917504; break;
        case 1256 : 
        case 1270 :
                    Size=1835008; break;
        default   : return 0;
    }

    if (CompressionID >= 1270)
        return Vc3_CompressedFrameSize_RI(Size, Width, Height); // Adaptative
    
    return Size;
};

//---------------------------------------------------------------------------
static const int8u Vc3_SBD(int32u SBD) //Sample Bit Depth
{
    switch (SBD)
    {
        case 1 : return  8;
        case 2 : return 10;
        case 3 : return 12;
        default: return  0;
    }
};

//---------------------------------------------------------------------------
static const int8u Vc3_SBD_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1237 :
        case 1238 :
        case 1242 :
        case 1243 :
        case 1251 :
        case 1252 :
        case 1253 :
        case 1258 :
        case 1259 :
        case 1260 :
                    return 8;
        case 1235 :
        case 1241 :
        case 1250 :
        case 1256 :
                    return 10;
        default   : return 0;
    }
}

//---------------------------------------------------------------------------
static const char* Vc3_FFC[4]=
{
    "",
    "Progressive",
    "Interlaced",
    "Interlaced",
};

//---------------------------------------------------------------------------
static const char* Vc3_FFC_ScanOrder[4]=
{
    "",
    "",
    "TFF",
    "BFF",
};

//---------------------------------------------------------------------------
static const char* Vc3_FFE[2]=
{
    "Interlaced",
    "Progressive",
};

//---------------------------------------------------------------------------
static const char* Vc3_SST[2]=
{
    "Progressive",
    "Interlaced",
};

//---------------------------------------------------------------------------
static const char* Vc3_SST_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1235 :
        case 1237 :
        case 1238 :
        case 1250 :
        case 1251 :
        case 1252 :
        case 1253 :
        case 1256 :
        case 1258 :
        case 1259 :
        case 1270:
        case 1271:
        case 1272:
        case 1273:
        case 1274:
                    return Vc3_SST[0];
        case 1241 :
        case 1242 :
        case 1243 :
        case 1260 :
                    return Vc3_SST[1];
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const int16u Vc3_SPL_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1258 :
                    return 960;
        case 1250 :
        case 1251 :
        case 1252 :
                    return 1280;
        case 1259:
        case 1260:
        case 1244:
                    return 1440;
        case 1235 :
        case 1237 :
        case 1238 :
        case 1241 :
        case 1242 :
        case 1243 :
        case 1253 :
                    return 1920;
        default   : return 0;
    }
}

//---------------------------------------------------------------------------
static const int16u Vc3_ALPF_PerFrame_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1250 :
        case 1251 :
        case 1252 :
        case 1258 :
                    return 720;
        case 1235 :
        case 1237 :
        case 1238 :
        case 1241 :
        case 1242 :
        case 1243 :
        case 1253 :
        case 1256 :
        case 1259 :
        case 1260 :
                    return 1080;
        default   : return 0;
    }
}

//---------------------------------------------------------------------------
static const char* Vc3_CLV[4]=
{
    "BT.709",
    "BT.2020 non-constant",
    "BT.2020 constant",
    "",                         //Out of band
};

//---------------------------------------------------------------------------
static const char* Vc3_CLF[2]=
{
    "YUV",
    "RGB",
};

//---------------------------------------------------------------------------
static const char* Vc3_CLR_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1235 :
        case 1237 :
        case 1238 :
        case 1241 :
        case 1242 :
        case 1243 :
        case 1250 :
        case 1251 :
        case 1252 :
        case 1253 :
        case 1258 :
        case 1259 :
        case 1260 :
        case 1271:
        case 1272:
        case 1273:
        case 1274:
                    return Vc3_CLF[0];
        case 1256 :
        case 1270 :
                    return Vc3_CLF[1];
        default   : return "";
    }
};

//---------------------------------------------------------------------------
static const char* Vc3_SSC[4]=
{
    "4:2:2",
    "4:2:0",
    "4:4:4",
    "",
};

//---------------------------------------------------------------------------
static const char* Vc3_SSC_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1235 :
        case 1237 :
        case 1238 :
        case 1241 :
        case 1242 :
        case 1243 :
        case 1250 :
        case 1251 :
        case 1252 :
        case 1253 :
        case 1258 :
        case 1259 :
        case 1260 :
        case 1271:
        case 1272:
        case 1273:
        case 1274:
                    return Vc3_SSC[0];
        case 1256 :
        case 1270:
                    return Vc3_SSC[2];
        default   : return "";
    }
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Vc3::File_Vc3()
:File__Analyze()
{
    //In
    Frame_Count_Valid=2;
    FrameRate=0;

    //Parsers
    #if defined(MEDIAINFO_CDP_YES)
        Cdp_Parser=NULL;
    #endif //defined(MEDIAINFO_CDP_YES)

    //Temp
    FFC_FirstFrame=(int8u)-1;
}

//---------------------------------------------------------------------------
File_Vc3::~File_Vc3()
{
    #if defined(MEDIAINFO_CDP_YES)
        delete Cdp_Parser; //Cdp_Parser=NULL;
    #endif //defined(MEDIAINFO_CDP_YES)
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vc3::Streams_Fill()
{
    //Filling
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "VC-3");
    Fill(Stream_Video, 0, Video_BitRate_Mode, VBR?"VBR":"CBR");
    if (!VBR && FrameRate && Vc3_CompressedFrameSize(CID, SPL, ALPF*(SST?2:1)))
        Fill(Stream_Video, 0, Video_BitRate, Vc3_CompressedFrameSize(CID, SPL, ALPF*(SST?2:1))*8*FrameRate, 0);
    Fill(Stream_Video, 0, Video_Format_Version, __T("Version ")+Ztring::ToZtring(HVN));
    Fill(Stream_Video, 0, Video_Format_Profile, string(Vc3_FromCID_Profile(CID))+'@'+Vc3_FromCID_Level(CID));
    if (FFC_FirstFrame!=(int8u)-1)
        Fill(Stream_Video, 0, Video_ScanOrder, Vc3_FFC_ScanOrder[FFC_FirstFrame]);
    if (Vc3_FromCID_IsSupported(CID))
    {
        if (Vc3_SPL_FromCID(CID))
            Fill(Stream_Video, 0, Video_Width, Vc3_SPL_FromCID(CID));
        if (Vc3_ALPF_PerFrame_FromCID(CID))
            Fill(Stream_Video, 0, Video_Height, Vc3_ALPF_PerFrame_FromCID(CID));
        if (Vc3_SBD_FromCID(CID))
            Fill(Stream_Video, 0, Video_BitDepth, Vc3_SBD_FromCID(CID));
        Fill(Stream_Video, 0, Video_ScanType, Vc3_SST_FromCID(CID));
        Fill(Stream_Video, 0, Video_ColorSpace, Vc3_CLR_FromCID(CID));
        if (!strcmp(Vc3_CLR_FromCID(CID), "YUV")) // YUV
            Fill(Stream_Video, 0, Video_ChromaSubsampling, Vc3_SSC_FromCID(CID));
        Fill(Stream_Video, 0, Video_PixelAspectRatio, Video_Width==1440?1.333:1.0);
    }
    else
    {
        Fill(Stream_Video, 0, Video_Width, SPL);
        Fill(Stream_Video, 0, Video_Height, ALPF*(SST?2:1));
        Fill(Stream_Video, 0, Video_BitDepth, Vc3_SBD(SBD));
        Fill(Stream_Video, 0, Video_ScanType, Vc3_SST[SST]);
        Fill(Stream_Video, 0, Video_ColorSpace, (string(Vc3_CLF[CLF])+(ALP?"A":"")));
        if (!CLF) // YUV
            Fill(Stream_Video, 0, Video_ChromaSubsampling, (string(Vc3_SSC[SSC])+(ALP?"4":"")));
        if (PARC && PARN)
            Fill(Stream_Video, 0, Video_PixelAspectRatio, ((float64)PARC)/PARN);
    }

    if (!TimeCode_FirstFrame.empty())
        Fill(Stream_Video, 0, Video_TimeCode_FirstFrame, TimeCode_FirstFrame);

    if (FrameInfo.DUR!=(int64u)-1)
        Fill(Stream_Video, 0, Video_FrameRate, 1000000000.0/FrameInfo.DUR, 3);
}

//---------------------------------------------------------------------------
void File_Vc3::Streams_Finish()
{
    #if defined(MEDIAINFO_CDP_YES)
        if (Cdp_Parser && !Cdp_Parser->Status[IsFinished] && Cdp_Parser->Status[IsAccepted])
        {
            Finish(Cdp_Parser);
            for (size_t StreamPos=0; StreamPos<Cdp_Parser->Count_Get(Stream_Text); StreamPos++)
            {
                Merge(*Cdp_Parser, Stream_Text, StreamPos, StreamPos);
                Ztring MuxingMode=Cdp_Parser->Retrieve(Stream_Text, StreamPos, "MuxingMode");
                Fill(Stream_Text, StreamPos, "MuxingMode", __T("VC-3 / Nexio user data / ")+MuxingMode, true);
            }

            Ztring LawRating=Cdp_Parser->Retrieve(Stream_General, 0, General_LawRating);
            if (!LawRating.empty())
                Fill(Stream_General, 0, General_LawRating, LawRating, true);
            Ztring Title=Cdp_Parser->Retrieve(Stream_General, 0, General_Title);
            if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
                Fill(Stream_General, 0, General_Title, Title);
        }
    #endif //defined(MEDIAINFO_CDP_YES)
}

//***************************************************************************
// Buffer - Demux
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
bool File_Vc3::Demux_UnpacketizeContainer_Test()
{
    //TODO: handling of the extra 4 bytes in a MOV container having 2 frames in a sample (see "Frame size?" part)

    if (Buffer_Offset+0x2C>Buffer_Size)
        return false;

    ALPF= BigEndian2int16u(Buffer+Buffer_Offset+0x18);
    SPL = BigEndian2int16u(Buffer+Buffer_Offset+0x1A);
    SST =(BigEndian2int16u(Buffer+Buffer_Offset+0x22)&(1<<2)?true:false);
    CID = BigEndian2int32u(Buffer+Buffer_Offset+0x28);
    size_t Size=Vc3_CompressedFrameSize(CID, SPL, ALPF*(SST?2:1));
    if (!Size)
    {
        if (!IsSub)
        {
            Reject();
            return false;
        }
        Size=Buffer_Size; //Hoping that the packet is complete. TODO: add a flag in the container parser saying if the packet is complete
    }
    Demux_Offset=Buffer_Offset+Size;

    if (Demux_Offset>Buffer_Size && !Config->IsFinishing)
        return false; //No complete frame

    Demux_UnpacketizeContainer_Demux();

    return true;
}
#endif //MEDIAINFO_DEMUX

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vc3::Read_Buffer_Unsynched()
{
    #if defined(MEDIAINFO_CDP_YES)
        if (Cdp_Parser)
            Cdp_Parser->Open_Buffer_Unsynch();
    #endif //defined(MEDIAINFO_CDP_YES)
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Vc3::Header_Begin()
{
    if (IsSub && Buffer_Offset+4==Buffer_Size && BigEndian2int32u(Buffer+Buffer_Offset)*Frame_Count_InThisBlock==Buffer_Offset)
    {
        Skip_B4(                                                "Frame size?");
        Buffer_Offset+=4;
    }

    if (Buffer_Offset+0x00000280>Buffer_Size)
        return false;

    return true;
}

//---------------------------------------------------------------------------
void File_Vc3::Header_Parse()
{
    ALPF= BigEndian2int16u(Buffer+Buffer_Offset+0x18);
    SPL = BigEndian2int16u(Buffer+Buffer_Offset+0x1A);
    SST =(BigEndian2int16u(Buffer+Buffer_Offset+0x22)&(1<<2)?true:false);
    CID = BigEndian2int32u(Buffer+Buffer_Offset+0x28);

    Header_Fill_Code(0, "Frame");
    size_t Size=Vc3_CompressedFrameSize(CID, SPL, ALPF*(SST?2:1));
    if (!Size)
    {
        if (!IsSub)
        {
            Reject();
            return;
        }
        Size=Buffer_Size; //Hoping that the packet is complete. TODO: add a flag in the container parser saying if the packet is complete
    }
    Header_Fill_Size(Size);
}

//---------------------------------------------------------------------------
void File_Vc3::Data_Parse()
{
    //Parsing
    if (Status[IsFilled])
    {
        Skip_XX(Element_Size,                                   "Data");
    }
    else
    {
    Element_Info1(Frame_Count);
    Element_Begin1("Header");
    HeaderPrefix();
    if (HVN <= 3)
    {
        CodingControlA();
        Skip_XX(16,                                             "Reserved");
        ImageGeometry();
        Skip_XX( 5,                                             "Reserved");
        CompressionID();
        CodingControlB();
        Skip_XX( 3,                                             "Reserved");
        TimeCode();
        Skip_XX(38,                                             "Reserved");
        UserData();
        Skip_XX( 3,                                             "Reserved");
        MacroblockScanIndices();
        Element_End0();
        Element_Begin1("Payload");
        Skip_XX(Element_Size-Element_Offset-4,                  "Data");
        Element_End0();
        Element_Begin1("EOF");
        Skip_B4(                                                CRCF?"CRC":"Signature");
        Element_End0();
    }
    else
    {
        Element_End0();
        Skip_XX(Element_Size-Element_Offset,                    "Data");
    }
    }

    FILLING_BEGIN();
        Frame_Count++;
        Frame_Count_InThisBlock++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;
        if (FrameRate)
        {
            FrameInfo.PTS=FrameInfo.DTS+=float64_int64s(1000000000/FrameRate);
            FrameInfo.DUR=float64_int64s(1000000000/FrameRate);
        }
        else if (FrameInfo.DUR!=(int64u)-1)
        {
            if (Frame_Count_InThisBlock==1)
                FrameInfo.DUR/=Buffer_Size/Element_Size;
            FrameInfo.PTS=FrameInfo.DTS+=FrameInfo.DUR;
        }
        else
        {
            FrameInfo.PTS=FrameInfo.DTS=FrameInfo.DUR=(int64u)-1;
        }
        if (!Status[IsAccepted])
            Accept("VC-3");
        if (!Status[IsFilled] && Frame_Count>=Frame_Count_Valid)
        {
            Fill("VC-3");

            if (!IsSub && Config->ParseSpeed<1.0)
                Finish("VC-3");
        }
    FILLING_END();
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vc3::HeaderPrefix()
{
    //Parsing
    Element_Begin1("Header Prefix");
    Get_B4 (HS,                                                 "HS, Header Size");
    Get_B1 (HVN,                                                "HVN, Header Version Number");
    Element_End0();

    FILLING_BEGIN();
        if (HS<0x00000280)
            Reject("VC-3");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Vc3::CodingControlA()
{
    //Parsing
    Element_Begin1("Coding Control A");
    BS_Begin();

    int8u FFC;
    Mark_0();
    Mark_0();
    Mark_0();
    Get_SB (   VBR,                                             "VBR, Variable Bitrate Encoding");
    Mark_0();
    Mark_0();
    Get_S1 (2, FFC,                                             "FFC, Field/Frame Count"); Param_Info1(Vc3_FFC[FFC]);

    Mark_1();
    Mark_0();
    Skip_SB(                                                    "MACF, Macroblock Adaptive Control flag");
    Get_SB (   CRCF,                                            "CRCF, CRC flag");
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();

    Mark_1();
    Mark_0();
    Mark_1();
    Mark_0();
    Mark_0();
    Get_SB (   PMA,                                             "PMA, Pre-multiplied Alpha");
    Get_SB (   LLA,                                             "LLA, Lossless Alpha flag");
    Get_SB (   ALP,                                             "ALP, Alpha flag");

    BS_End();
    Element_End0();

    FILLING_BEGIN();
        if (FFC_FirstFrame==(int8u)-1)
            FFC_FirstFrame=FFC;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Vc3::ImageGeometry()
{
    //Parsing
    int8u PARC0, PARC1, PARN0, PARN1;
    Element_Begin1("Image Geometry");
    Get_B2 (ALPF,                                               "Active lines-per-frame");
    Get_B2 (SPL,                                                "Samples-per-line");
    BS_Begin();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Get_S1 (2, PARC1,                                           "PARC1, Pixel Aspect Ratio C1");
    Get_S1 (2, PARN1,                                           "PARN1, Pixel Aspect Ratio N1");
    BS_End();
    Skip_B2(                                                    "Number of active lines");
    Get_B1 (PARC0,                                              "PARC0, Pixel Aspect Ratio C0");
    Get_B1 (PARN0,                                              "PARN0, Pixel Aspect Ratio N0");
    PARC=(((int16u)PARC1)<<8)|PARC0;
    PARN=(((int16u)PARN1)<<8)|PARN0;

    BS_Begin();

    Get_S1 (3, SBD,                                             "Sample bit depth"); Param_Info1(Vc3_SBD(SBD));
    Mark_1();
    Mark_1();
    Mark_0();
    Mark_0();
    Mark_0();

    Mark_1();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_1();
    Get_SB (   SST,                                             "Source scan type"); Param_Info1(Vc3_SST[SST]);
    Mark_0();
    Mark_0();

    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::CompressionID()
{
    //Parsing
    Element_Begin1("Compression ID");
    int32u Data;
    Get_B4 (Data,                                               "Compression ID");
    Element_End0();

    FILLING_BEGIN();
        CID=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Vc3::CodingControlB()
{
    //Parsing
    Element_Begin1("Coding Control B");
    BS_Begin();

    Info_SB(   FFE,                                             "FFE, Field/Frame Count"); Param_Info1(Vc3_FFE[FFE]);
    Get_SB (   SSC,                                             "SSC, Sub Sampling Control"); Param_Info1(Vc3_SSC[SSC]);
    Mark_0();
    Mark_0();
    Get_S1 (2, CLV,                                             "CLR, Color Volume"); Param_Info1(Vc3_CLV[CLV]);
    Get_SB (   CLF,                                             "CLF, Color Format"); Param_Info1(Vc3_CLF[CLF]);

    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::TimeCode()
{
    //Parsing
    Element_Begin1("Time Code");
    bool TCP;

    BS_Begin();
    Get_SB (   TCP,                                             "TCP, Time Code Present");
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    if (TCP)
        Mark_0();
    else
    {
        TCP=Peek_SB();
        if (TCP)
            Skip_SB(                                            "TCP, Time Code Present (wrong order)");
        else
            Mark_0();
    }

    if (TCP)
    {
        Element_Begin1("Time Code");
            int8u HHT, HHU, MMT, MMU, SST, SSU, FFT, FFU;
            bool DF;
            Skip_S1(4,                                          "Binary Group 1");
            Get_S1 (4, FFU,                                     "Units of Frames");
            Skip_S1(4,                                          "Binary Group 2");
            Skip_SB(                                            "Color Frame");
            Get_SB (   DF,                                      "Drop Frame");
            Get_S1 (2, FFT,                                     "Tens of Frames");
            Skip_S1(4,                                          "Binary Group 3");
            Get_S1 (4, SSU,                                     "Units of Seconds");
            Skip_S1(4,                                          "Binary Group 4");
            Skip_SB(                                            "Field ID");
            Get_S1 (3, SST,                                     "Tens of Seconds");
            Skip_S1(4,                                          "Binary Group 5");
            Get_S1 (4, MMU,                                     "Units of Minutes");
            Skip_S1(4,                                          "Binary Group 6");
            Skip_SB(                                            "X");
            Get_S1 (3, MMT,                                     "Tens of Minutes");
            Skip_S1(4,                                          "Binary Group 7");
            Get_S1 (4, HHU,                                     "Units of Hours");
            Skip_S1(4,                                          "Binary Group 8");
            Skip_SB(                                            "X");
            Skip_SB(                                            "X");
            Get_S1 (2, HHT,                                     "Tens of Hours");
            FILLING_BEGIN();
                if (TimeCode_FirstFrame.empty() && FFU<10 && SSU<10 && SST<6 && MMU<10 && MMT<6 && HHU<10)
                {
                    std::ostringstream S;
                    S<<(size_t)HHT<<(size_t)HHU<<':'<<(size_t)MMT<<(size_t)MMU<<':'<<(size_t)SST<<(size_t)SSU<<(DF?';':':')<<(size_t)FFT<<(size_t)FFU;
                    TimeCode_FirstFrame=S.str();
                }
            FILLING_END();
        Element_End0();
        BS_End();
    }
    else
    {
        BS_End();
        Skip_B8(                                                "Junk");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::UserData()
{
    //Parsing
    int8u UserDataLabel;

    Element_Begin1("User Data Control");
    BS_Begin();
    Get_S1 (4, UserDataLabel,                                   "User Data Label");
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_1();
    BS_End();
    Element_End0();

    Element_Begin1("User Data Payload");
    switch (UserDataLabel)
    {
        case 0x00: Skip_XX(260,                                 "Reserved"); break;
        case 0x08: UserData_8(); break;
        default  : Skip_XX(260,                                 "Reserved for future use");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::MacroblockScanIndices()
{
    Element_Begin1("Macroblock Scan Indices Control");
        Skip_XX(9,                                              "ToDo");
    Element_End0();
    Element_Begin1("Macroblock Scan Indices Payload");
        Skip_XX(HS-Element_Offset,                              "ToDo");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::UserData_8()
{
    if (Element_Offset + 0x104 < Element_Size
        && Buffer[Buffer_Offset + (size_t)Element_Offset + 0xBA] == 0x96
        && Buffer[Buffer_Offset + (size_t)Element_Offset + 0xBB] == 0x69)
    {
        Skip_XX(0xBA,                                           "Nexio private data?");
        #if defined(MEDIAINFO_CDP_YES)
            if (Cdp_Parser==NULL)
            {
                Cdp_Parser=new File_Cdp;
                Open_Buffer_Init(Cdp_Parser);
                Frame_Count_Valid=300;
            }
            if (!Cdp_Parser->Status[IsFinished])
            {
                ((File_Cdp*)Cdp_Parser)->AspectRatio=16.0/9.0;
                Open_Buffer_Continue(Cdp_Parser, Buffer + Buffer_Offset + (size_t)Element_Offset, 0x49);
            }
            Element_Offset+=0x49;
            Skip_B1(                                            "Nexio private data?");
        #else //MEDIAINFO_CDP_YES
            Skip_XX(0x4A                                        "CDP data");
        #endif //MEDIAINFO_CDP_YES
    }
    else
        Skip_XX(260,                                            "Nexio private data?");

}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_VC3_*
