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
#if defined(MEDIAINFO_WEBP_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_WebP.h"
#if defined(MEDIAINFO_ICC_YES)
    #include "MediaInfo/Tag/File_Icc.h"
#endif
#if defined(MEDIAINFO_XMP_YES)
    #include "MediaInfo/Tag/File_Xmp.h"
#endif
#if defined(MEDIAINFO_VP8_YES)
    #include "MediaInfo/Video/File_Vp8.h"
#endif
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Const
//***************************************************************************

namespace Elements
{
    const int32u RIFF = 0x52494646;
    const int32u WEBP = 0x57454250;
    const int32u WEBP_ALPH = 0x414C5048;
    const int32u WEBP_ANIM = 0x414E494D;
    const int32u WEBP_ANMF = 0x414E4D46;
    const int32u WEBP_EXIF = 0x45584946;
    const int32u WEBP_ICCP = 0x49434350;
    const int32u WEBP_VP8_ = 0x56503820;
    const int32u WEBP_VP8L = 0x5650384C;
    const int32u WEBP_VP8X = 0x56503858;
    const int32u WEBP_XMP_ = 0x584D5020;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_WebP::File_WebP()
{
    HasAlpha = false;
}

//---------------------------------------------------------------------------
File_WebP::~File_WebP()
{
    delete ICC_Parser;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_WebP::Streams_Finish()
{
    if (StreamKind_Last == Stream_Video) {
        Fill(Stream_Video, 0, Video_FrameCount, Frame_Count);
        Fill(Stream_Video, 0, Video_FrameRate_Mode, FrameDuration ? "CFR" : "VFR");
        Fill(Stream_Video, 0, Video_Duration, TotalDuration);
    }
    if (HasAlpha) {
        auto ColorSpace = Retrieve(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_ColorSpace));
        if (!ColorSpace.empty() && ColorSpace.back() != __T('A')) {
            ColorSpace += __T('A');
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_ColorSpace), ColorSpace, true);
        }
    }
    if (StreamKind_Last == Stream_Image && ICC_Parser) {
        Merge(*ICC_Parser, Stream_Image, 0, 0); // Merged here because we don't know the stream kind in advance
    }
}

//***************************************************************************
// Buffer
//***************************************************************************

//---------------------------------------------------------------------------
bool File_WebP::FileHeader_Begin()
{
    if (Buffer_Size < 12)
        return false;

    if (CC4(Buffer)      != Elements::RIFF
     || CC4(Buffer + 8)  != Elements::WEBP) {
        Reject();
        return false;
    }

    Accept();
    Fill(Stream_General, 0, General_Format, "WebP");
    return true;
}

//---------------------------------------------------------------------------
void File_WebP::Header_Parse()
{
    //Parsing
    int32u Size, Name;
    Get_C4 (Name,                                               "Name");
    Get_L4 (Size,                                               "Size");

    //Alignment
    Alignment_ExtraByte = Size % 2 && Size < File_Size - (File_Offset + Buffer_Offset + Element_Offset);
    Size += Alignment_ExtraByte;

    //Top level chunks
    if (Name == Elements::RIFF) {
        Get_C4 (Name,                                           "Real Name");
    }

    FILLING_BEGIN();
        Header_Fill_Size(8 + (int64u)Size);
        Header_Fill_Code(Name, Ztring().From_CC4(Name));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_WebP::Data_Parse()
{
    //Alignment
    if (Alignment_ExtraByte) {
        Element_Size--;
    }

    DATA_BEGIN
    LIST(WEBP)
        ATOM_BEGIN
        ATOM(WEBP_ALPH)
        ATOM(WEBP_ANIM)
        LIST(WEBP_ANMF)
            ATOM_BEGIN
            ATOM(WEBP_ALPH)
            ATOM(WEBP_VP8_)
            ATOM(WEBP_VP8L)
        ATOM_END
        ATOM(WEBP_EXIF)
        ATOM(WEBP_ICCP)
        ATOM(WEBP_VP8_)
        ATOM(WEBP_VP8L)
        ATOM(WEBP_VP8X)
        ATOM(WEBP_XMP_)
        ATOM_END
    DATA_END

    //Alignment
    if (Alignment_ExtraByte) {
        bool HasAlignment = Element_Offset == Element_Size;
        Element_Size++;
        if (HasAlignment) {
            Skip_B1(                                            "Alignment");
        }
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_WebP::WEBP_ALPH()
{
    Skip_XX(Element_Size,                                       "(Not parsed)"); 

    HasAlpha = true;
}

//---------------------------------------------------------------------------
void File_WebP::WEBP_ANIM()
{
    //Parsing
    Skip_L4(                                                    "Background color");
    Skip_L2(                                                    "Loop count");

    FILLING_BEGIN();
        //Ztring bg;
        //bg.From_Number(bgColor, 16);
        //bg.MakeUpperCase();
        //bg.insert(0, __T("0x"));
        //Fill(Stream_Image, 0, "Animation_BackgroundColor", bg);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_WebP::WEBP_ANMF()
{
    //Parsing
    if (Element_Size < 16 && !Element_IsComplete_Get()) {
        Element_WaitForMoreData();
        return;
    }
    int32u Duration;
    Skip_L3(                                                    "Frame X");
    Skip_L3(                                                    "Frame Y");
    Skip_L3(                                                    "Frame Width minus 1");
    Skip_L3(                                                    "Frame Height minus 1");
    Get_L3 (Duration,                                           "Frame Duration");
    BS_Begin();
    Skip_S1(6,                                                  "Reserved");
    Skip_SB(                                                    "Blending method");
    Skip_SB(                                                    "Disposal method");
    BS_End();

    FILLING_BEGIN();
        if (!Frame_Count) {
            Stream_Prepare(Stream_Video);
            FrameDuration = Duration;
        }
        if (Frame_Count && Duration != FrameDuration) {
            FrameDuration = 0; // VFR
        }
        Frame_Count++;
        TotalDuration += Duration;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_WebP::WEBP_EXIF()
{
    Skip_XX(Element_Size,                                       "EXIF metadata"); 

    //Fill(Stream_Image, 0, "Metadata_Exif", "Yes"); 
}

//---------------------------------------------------------------------------
void File_WebP::WEBP_ICCP()
{
    if (ICC_Parser)
        delete ICC_Parser;
    ICC_Parser = new File_Icc;
    ICC_Parser->StreamKind = Stream_Image;
    ICC_Parser->IsAdditional = true;
    Open_Buffer_Init(ICC_Parser);
    Open_Buffer_Continue(ICC_Parser);
    Open_Buffer_Finalize(ICC_Parser);
}

//---------------------------------------------------------------------------
void File_WebP::WEBP_VP8_()
{
    if (!Frame_Count) {
        Stream_Prepare(Stream_Image);
    }

    #if defined(MEDIAINFO_VP8_YES)
    File_Vp8 MI;
    MI.StreamKind = StreamKind_Last;
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);

    FILLING_BEGIN();
        if (Frame_Count <= 1) { // Using first frame data
            Merge(MI, StreamKind_Last, 0, 0);
        }
    FILLING_END();
    #else
    Skip_XX(Element_Size,                                       "(Data)");
    Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "VP8");
    #endif
}

//---------------------------------------------------------------------------
void File_WebP::WEBP_VP8L()
{
    if (!Frame_Count) {
        Stream_Prepare(Stream_Image);
    }

    //Parsing
    int16u w, h;
    int8u signature, version_number;
    bool alpha_is_used;
    Get_B1 (signature,                                          "signature");
    if (signature != 0x2F)
    {
        Trusted_IsNot("Invalid VP8L signature");
        return;
    }
    BS_Begin_LE();
    Get_T2 (14, w,                                              "image_width minus 1");
    Get_T2 (14, h,                                              "image_height minus 1");
    Get_TB (    alpha_is_used,                                  "alpha_is_used");
    Get_T1 ( 3, version_number,                                 "version_number");
    BS_End_LE();
    Skip_XX(Element_Size - Element_Offset,                      version_number ? "(Unsupported)" : "(Not parsed)");

    FILLING_BEGIN();
        if (Frame_Count <= 1) { // Using first frame data
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "WebP");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format_Version), "Version " + to_string(version_number));
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Width), ((int32u)w) + 1);
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Height), ((int32u)h) + 1);
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_BitDepth), 8);
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Compression_Mode), "Lossless");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_ColorSpace), alpha_is_used ? "RGBA" : "RGB");
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_WebP::WEBP_VP8X()
{
    //Parsing
    BS_Begin();
    Skip_S1(2,                                                  "Reserved");
    Skip_SB(                                                    "ICC profile");
    Skip_SB(                                                    "Alpha");
    Skip_SB(                                                    "EXIF metadata");
    Skip_SB(                                                    "XMP metadata");
    Skip_SB(                                                    "Animation");
    BS_End();
    Skip_L3(                                                    "Reserved");
    Skip_L3(                                                    "Canvas Width minus 1");
    Skip_L3(                                                    "Canvas Height minus 1");
}

//---------------------------------------------------------------------------
void File_WebP::WEBP_XMP_()
{
    //Parsing
    #if defined(MEDIAINFO_XMP_YES)
    File_Xmp MI;
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
    Merge(MI, Stream_General, 0, 0);
    #else
    Skip_UTF8(Element_Size,                                     "XMP metadata");
    #endif
}

} // namespace MediaInfoLib

#endif