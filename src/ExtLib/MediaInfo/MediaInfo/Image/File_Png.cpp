/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about PNG files
//
// From http://www.fileformat.info/format/png/
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
#if defined(MEDIAINFO_PNG_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_Png.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/Tag/File_Xmp.h"
#include <zlib.h>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Png_Colour_type(int8u Colour_type)
{
    switch (Colour_type)
    {
        case 0 : return "Greyscale";
        case 2 : return "Truecolour";
        case 3 : return "Indexed-colour";
        case 4 : return "Greyscale with alpha";
        case 6 : return "Truecolour with alpha";
    default: return "";
    }
}

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
namespace Elements
{
    const int32u IDAT=0x49444154;
    const int32u IEND=0x49454E44;
    const int32u IHDR=0x49484452;
    const int32u PLTE=0x506C5445;
    const int32u gAMA=0x67414D41;
    const int32u iTXt=0x69545874;
    const int32u pHYs=0x70485973;
    const int32u sBIT=0x73424954;
    const int32u tEXt=0x74455874;
    const int32u zTXt=0x7A545874;
}

//---------------------------------------------------------------------------
static const char* Keywords_Mapping[][2]= 
{
    { "Author", "Performer" },
    { "Creation Time", "Encoded_Date" },
    { "Software", "Encoded_Application" },
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Png::File_Png()
{
    //Config
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;

    //In
    StreamKind=Stream_Max;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Png::Streams_Accept()
{
    if (!IsSub)
    {
        TestContinuousFileNames();

        Stream_Prepare((Config->File_Names.size()>1 || Config->File_IsReferenced_Get())?Stream_Video:Stream_Image);
        if (File_Size!=(int64u)-1)
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize), File_Size);
        if (StreamKind_Last==Stream_Video)
            Fill(Stream_Video, StreamPos_Last, Video_FrameCount, Config->File_Names.size());
    }
    else
        Stream_Prepare(StreamKind==Stream_Max?StreamKind_Last:StreamKind);
}

//***************************************************************************
// Header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Png::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<8)
        return false; //Must wait for more data

    if (CC4(Buffer+4)!=0x0D0A1A0A) //Byte order
    {
        Reject("PNG");
        return false;
    }

    switch (CC4(Buffer)) //Signature
    {
        case 0x89504E47 :
            Accept("PNG");

            Fill(Stream_General, 0, General_Format, "PNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "PNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), "PNG");

            break;

        case 0x8A4E4E47 :
            Accept("PNG");

            Fill(Stream_General, 0, General_Format, "MNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "MNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), "MNG");

            Finish("PNG");
            break;

        case 0x8B4A4E47 :
            Accept("PNG");

            Fill(Stream_General, 0, General_Format, "JNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "JNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), "JNG");

            Finish("PNG");
            break;

        default:
            Reject("PNG");
    }

    //All should be OK...
    return true;
}

//---------------------------------------------------------------------------
void File_Png::FileHeader_Parse()
{
    Skip_B4(                                                    "Signature");
    Skip_B4(                                                    "ByteOrder");
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Png::Read_Buffer_Unsynched()
{
    Read_Buffer_Unsynched_OneFramePerFile();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Png::Header_Parse()
{
    //Parsing
    int32u Length, Chunk_Type;
    Get_B4 (Length,                                             "Length");
    Get_C4 (Chunk_Type,                                         "Chunk Type");

    //Filling
    if (Chunk_Type==Elements::IDAT)
        Element_ThisIsAList(); // No need of the content
    Header_Fill_Size(12+Length); //+4 for CRC
    Header_Fill_Code(Chunk_Type, Ztring().From_CC4(Chunk_Type));
}

//---------------------------------------------------------------------------
void File_Png::Data_Parse()
{
    Element_Size-=4; //For CRC

    #define CASE_INFO(_NAME, _DETAIL) \
        case Elements::_NAME : Element_Info1(_DETAIL); _NAME(); break;

    //Parsing
    switch (Element_Code)
    {
        CASE_INFO(IDAT,                                         "Image data");
        CASE_INFO(IEND,                                         "Image trailer");
        CASE_INFO(IHDR,                                         "Image header");
        CASE_INFO(PLTE,                                         "Palette table");
        CASE_INFO(gAMA,                                         "Gamma");
        CASE_INFO(iTXt,                                         "International textual data");
        CASE_INFO(pHYs,                                         "Physical pixel dimensions");
        CASE_INFO(sBIT,                                         "Significant bits");
        CASE_INFO(tEXt,                                         "Textual data");
        CASE_INFO(zTXt,                                         "Compressed textual data");
        default : Skip_XX(Element_Size,                         "Unknown");
    }

    Element_Size+=4; //For CRC
    if (Element_Code!=Elements::IDAT)
        Skip_B4(                                                "CRC"); 
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Png::IDAT()
{
    //Parsing
    Skip_XX(Element_TotalSize_Get()-4,                          "Data");
    Param2("CRC",                                               "(Skipped) (4 bytes)");
    if (Config->ParseSpeed<1.0)
        Finish();
}

//---------------------------------------------------------------------------
void File_Png::IHDR()
{
    //Parsing
    int32u Width, Height;
    int8u  Bit_depth, Colour_type, Compression_method, Interlace_method;
    Get_B4 (Width,                                              "Width");
    Get_B4 (Height,                                             "Height");
    Get_B1 (Bit_depth,                                          "Bit depth");
    Get_B1 (Colour_type,                                        "Colour type"); Param_Info1(Png_Colour_type(Colour_type));
    Get_B1 (Compression_method,                                 "Compression method");
    Skip_B1(                                                    "Filter method");
    Get_B1 (Interlace_method,                                   "Interlace method");

    FILLING_BEGIN_PRECISE();
        if (!Status[IsFilled])
        {
            Fill(StreamKind_Last, 0, "Width", Width);
            Fill(StreamKind_Last, 0, "Height", Height);
            string ColorSpace=(Colour_type&(1<<1))?"RGB":"Y";
            if (Colour_type&(1<<2))
                ColorSpace+='A';
            Fill(StreamKind_Last, 0, "ColorSpace", ColorSpace);
            Fill(StreamKind_Last, 0, "BitDepth", Bit_depth);
            if (Retrieve_Const(StreamKind_Last, 0, "PixelAspectRatio").empty())
                Fill(StreamKind_Last, 0, "PixelAspectRatio", 1.0, 3);
            switch (Compression_method)
            {
                case 0 :
                    Fill(StreamKind_Last, 0, "Format_Compression", "Deflate");
                    break;
                default: ;
            }
            switch (Interlace_method)
            {
                case 0 :
                    break;
                case 1 :
                    break;
                default: ;
            }

            Fill();
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Png::gAMA()
{
    //Parsing
    int32u Gamma;
    Get_B4 (Gamma,                                              "Gamma");

    FILLING_BEGIN()
        Fill(StreamKind_Last, 0, "Gamma", Gamma/100000.0);
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Png::pHYs()
{
    //Parsing
    int32u X, Y;
    Get_B4 (X,                                                  "Pixels per unit, X axis");
    Get_B4 (Y,                                                  "Pixels per unit, Y axis");
    Skip_B1(                                                    "Unit specifier");

    FILLING_BEGIN()
        if (X && Y)
        {
            Clear(StreamKind_Last, 0, "DisplayAspectRatio");
            Fill(StreamKind_Last, 0, "PixelAspectRatio", ((float32)Y) / X, 3, true);
        }
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Png::sBIT()
{
    //Parsing
    map<int8u, int64u> Bits;
    for (int64u i=0; i<Element_Size; i++)
    {
        int8u Bit;
        Get_B1 (Bit,                                            "Significant bits");
        Bits[Bit]++;
    }

    FILLING_BEGIN()
        if (Bits.size()==1)
            Fill(StreamKind_Last, 0, "BitDepth", Bits.begin()->first, 10, true);
    FILLING_END()
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Png::Textual(bitset8 Method)
{
    //Parsing
    Ztring Keyword, Language, Text;
    size_t Zero=(size_t)Element_Offset;
    for (; Zero<Element_Size; Zero++)
        if (!Buffer[Buffer_Offset+Zero])
            break;
    if (Zero>=Element_Size)
    {
        Skip_XX(Element_Size-Element_Offset,                    "(Problem)");
        return;
    }
    Get_ISO_8859_1(Zero-Element_Offset, Keyword,                "Keyword");
    Skip_B1(                                                    "Null separator");
    int8u Compression;
    if (Method[IsCompressed])
    {
        if (Method[IsUTF8])
        {
            int8u Compressed;
            Get_B1(Compressed,                                  "Compression flag");
            if (!Compressed)
                Method.set(IsCompressed, 0);
        }
        Get_B1(Compression,                                     "Compression method");
    }
    if (Method[IsUTF8])
    {
        Zero=(size_t)Element_Offset;
        for (; Zero<Element_Size; Zero++)
            if (!Buffer[Buffer_Offset+Zero])
                break;
        if (Zero>=Element_Size)
        {
            Skip_XX(Element_Size-Element_Offset,                "(Problem)");
            return;
        }
        Get_ISO_8859_1(Zero-Element_Offset, Language,           "Language tag");
        Skip_B1(                                                "Null separator");
        Zero=(size_t)Element_Offset;
        for (; Zero<Element_Size; Zero++)
            if (!Buffer[Buffer_Offset+Zero])
                break;
        if (Zero>=Element_Size)
        {
            Skip_XX(Element_Size-Element_Offset,                "(Problem)");
            return;
        }
        Skip_ISO_8859_1(Zero-Element_Offset,                    "Translated keyword");
        Skip_B1(                                                "Null separator");
    }
    if (Method[IsCompressed])
    {
        if (!Compression)
        {
            //Uncompress init
            z_stream strm;
            strm.next_in=(Bytef*)Buffer+Buffer_Offset+(size_t)Element_Offset;
            strm.avail_in=(int)(Element_Size-Element_Offset);
            strm.next_out=NULL;
            strm.avail_out=0;
            strm.total_out=0;
            strm.zalloc=Z_NULL;
            strm.zfree=Z_NULL;
            inflateInit(&strm);

            //Prepare out
            strm.avail_out=0x10000; //Blocks of 64 KiB, arbitrary chosen, as a begin
            strm.next_out=(Bytef*)new Bytef[strm.avail_out];

            //Parse compressed data, with handling of the case the output buffer is not big enough
            for (;;)
            {
                //inflate
                int inflate_Result=inflate(&strm, Z_NO_FLUSH);
                if (inflate_Result<0)
                    break;

                //Check if we need to stop
                if (strm.avail_out || inflate_Result)
                    break;

                //Need to increase buffer
                size_t UncompressedData_NewMaxSize=strm.total_out*4;
                int8u* UncompressedData_New=new int8u[UncompressedData_NewMaxSize];
                memcpy(UncompressedData_New, strm.next_out-strm.total_out, strm.total_out);
                delete[] strm.next_out; strm.next_out=UncompressedData_New;
                strm.next_out=strm.next_out+strm.total_out;
                strm.avail_out=UncompressedData_NewMaxSize-strm.total_out;
            }
            auto Buffer=(const char*)strm.next_out-strm.total_out;
            auto Buffer_Size=(size_t)strm.total_out;
            if (Method[IsUTF8])
                Text.From_UTF8(Buffer, Buffer_Size);
            else
                Text.From_ISO_8859_1(Buffer, Buffer_Size);
        }
        Skip_XX(Element_Size-Element_Offset,                    "(Compressed)");
        if (!Text.empty())
            Param_Info1(Text);
    }
    else
    {
        auto Size=(size_t)(Element_Size-Element_Offset);
        const auto Trace="Text string";
        if (Method[IsUTF8])
            Get_UTF8(Size, Text,                                Trace);
        else
            Get_ISO_8859_1(Size, Text,                          Trace);
    }

    FILLING_BEGIN()
        auto Keyword_UTF8=Keyword.To_UTF8();
        for (const auto& Key : Keywords_Mapping)
        {
            if (Keyword_UTF8==Key[0])
                Keyword_UTF8=Key[1];
        }
        if (Keyword_UTF8=="Comment" && Text.rfind(__T("Created with "), 0)==0) //Some tools e.g. GIMP do that
        {
            Keyword_UTF8="Encoded_Application";
            Text.erase(0, 13);
        }
        if (Keyword_UTF8=="XML:com.adobe.xmp")
        {
            auto Text_UTF8=Text.To_UTF8();
            File_Xmp MI;
            Open_Buffer_Init(&MI, Text_UTF8.size());
            Open_Buffer_Continue(&MI, (const int8u*)Text_UTF8.c_str(), Text_UTF8.size());
            Skip_XX(Text_UTF8.size(),                           "Stream, Data");
            Open_Buffer_Finalize(&MI);
            Merge(MI, Stream_General, 0, 0);
            Text.clear();
        }
        else if (!Language.empty())
            Text.insert(0, __T('(')+Language+__T(')'));
        Fill(Stream_General, 0, Keyword_UTF8.c_str(), Text);
    FILLING_END()
}

} //NameSpace

#endif
