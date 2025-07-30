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
#include "MediaInfo/Image/File_GainMap.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if defined(MEDIAINFO_JPEG_YES)
    #include "MediaInfo/Image/File_Jpeg.h"
#endif
#if defined(MEDIAINFO_PSD_YES)
    #include "MediaInfo/Image/File_Psd.h"
#endif
#if defined(MEDIAINFO_C2PA_YES)
    #include "MediaInfo/Tag/File_C2pa.h"
#endif
#if defined(MEDIAINFO_EXIF_YES)
    #include "MediaInfo/Tag/File_Exif.h"
#endif
#if defined(MEDIAINFO_ICC_YES)
    #include "MediaInfo/Tag/File_Icc.h"
#endif
#if defined(MEDIAINFO_IIM_YES)
    #include "MediaInfo/Tag/File_Iim.h"
#endif
#if defined(MEDIAINFO_XMP_YES)
    #include "MediaInfo/Tag/File_Xmp.h"
#endif
#include <zlib.h>
#include <memory>
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
static string Png_Colour_type_Settings(int8u Colour_type, int8u Bit_depth)
{
    switch (Colour_type)
    {
        case 0 :
        case 2 :
        case 4 :
        case 6 : return "Linear";
        case 3 : return "Indexed-"+std::to_string(Bit_depth);
        default: return "";
    }
}

//---------------------------------------------------------------------------
const char* Mpegv_colour_primaries(int8u colour_primaries);
const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
const char* Mpegv_matrix_coefficients_ColorSpace(int8u matrix_coefficients);
const char* Mk_Video_Colour_Range(int8u range);

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
namespace Elements
{
    const int32u IDAT=0x49444154;
    const int32u IEND=0x49454E44;
    const int32u IHDR=0x49484452;
    const int32u JDAT=0x4A444154;
    const int32u JHDR=0x4A484452;
    const int32u MEND=0x4D454E44;
    const int32u MHDR=0x4D484452;
    const int32u PLTE=0x504C5445;
    const int32u acTL=0x6163544C;
    const int32u bKGD=0x624B4744;
    const int32u caBX=0x63614258;
    const int32u caNv=0x63614E76;
    const int32u cHRM=0x6348524D; 
    const int32u cICP=0x63494350;
    const int32u cLLI=0x634C4C49;
    const int32u cLLi=0x634C4C69;
    const int32u eXIf=0x65584966;
    const int32u fcTL=0x6663544C;
    const int32u fdAT=0x66644154;
    const int32u gAMA=0x67414D41;
    const int32u gdAT=0x67644154;
    const int32u gmAP=0x676D4150;
    const int32u hIST=0x68495354;
    const int32u iCCP=0x69434350;
    const int32u iTXt=0x69545874;
    const int32u mDCV=0x6D444356;
    const int32u mDCv=0x6D444376;
    const int32u pHYs=0x70485973;
    const int32u sBIT=0x73424954;
    const int32u sPLT=0x73504C54;
    const int32u sRGB=0x73524742;
    const int32u tEXt=0x74455874;
    const int32u tIME=0x74494D45;
    const int32u tRNS=0x74524E53;
    const int32u vpAg=0x76704167;
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
File_Png::File_Png() : Data_Size{}, Signature{}
{
    //Config
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;

    //In
    StreamKind=Stream_Image;
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
        if (Config->File_Names.size() > 1 || Config->File_IsReferenced_Get())
            StreamKind = Stream_Video;
        Stream_Prepare(StreamKind);
        if (Config->File_Names.size() > 1)
            Fill(Stream_Video, StreamPos_Last, Video_FrameCount, Config->File_Names.size());
    }
    else
        Stream_Prepare(StreamKind);
}

//---------------------------------------------------------------------------
void File_Png::Streams_Finish()
{
    if (Data_Size != (int64u)-1) {
        if (StreamKind == Stream_Video && !IsSub && File_Size != (int64u)-1 && !Config->File_Sizes.empty())
            Fill(Stream_Video, 0, Video_StreamSize, File_Size - (Config->File_Sizes.front() - Data_Size) * Config->File_Sizes.size()); //We guess that the metadata part has a fixed size
        if (StreamKind == Stream_Image && (IsSub || File_Size != (int64u)-1)) {
            Fill(Stream_Image, 0, Image_StreamSize, Data_Size);
        }
    }
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
    Data_Size = 8;

    auto Signature_Temp = CC4(Buffer);
    const char* Format;
    switch (Signature_Temp)
    {
        case 0x89504E47 :
            Format = "PNG";
            break;
        case 0x8A4D4E47 :
            StreamKind = Stream_Video;
            Format = "MNG";
            break;
        case 0x8B4A4E47 :
            Format = "JNG";
            break;
        default:
            Reject("PNG");
            return true;
    }

    //All should be OK...
    Signature = Signature_Temp;
    Accept("PNG");
    Fill(Stream_General, 0, General_Format, Format);
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

//---------------------------------------------------------------------------
void File_Png::Read_Buffer_AfterParsing()
{
    //TODO: should be after the parsing of a file
    if (Config->ParseSpeed < 1.0 && !Config->File_Sizes.empty() && File_Offset >= Config->File_Sizes.front()) {
        Finish();
    }
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
        CASE_INFO(JHDR,                                         "JNG header");
        CASE_INFO(JDAT,                                         "JNG data");
        CASE_INFO(MEND,                                         "MNG trailer");
        CASE_INFO(MHDR,                                         "MNG header");
        CASE_INFO(PLTE,                                         "Palette table");
        CASE_INFO(acTL,                                         "Animation Control Chunk");
        CASE_INFO(bKGD,                                         "Background color"); 
        CASE_INFO(caBX,                                         "JUMBF");
        CASE_INFO(caNv,                                         "Canvas");
        CASE_INFO(cHRM,                                         "Primary chromaticities and white point"); 
        CASE_INFO(cICP,                                         "Coding-independent code points");
        CASE_INFO(cLLI,                                         "Content Light Level Information");
        CASE_INFO(cLLi,                                         "Content Light Level Information");
        CASE_INFO(eXIf,                                         "Exchangeable Image File (Exif) Profile"); 
        CASE_INFO(fcTL,                                         "Frame Control Chunk"); 
        CASE_INFO(fdAT,                                         "Frame Data Chunk"); 
        CASE_INFO(gAMA,                                         "Gamma");
        CASE_INFO(gdAT,                                         "Gain map image data");
        CASE_INFO(gmAP,                                         "Gain map metadata");
        CASE_INFO(hIST,                                         "Image histogram"); 
        CASE_INFO(iCCP,                                         "Embedded ICC profile");
        CASE_INFO(iTXt,                                         "International textual data");
        CASE_INFO(mDCV,                                         "Mastering Display Color Volume");
        CASE_INFO(mDCv,                                         "Mastering Display Color Volume");
        CASE_INFO(pHYs,                                         "Physical pixel dimensions");
        CASE_INFO(sBIT,                                         "Significant bits");
        CASE_INFO(sPLT,                                         "Suggested palette");
        CASE_INFO(sRGB,                                         "Standard RGB color space");
        CASE_INFO(tEXt,                                         "Textual data");
        CASE_INFO(tIME,                                         "Time stamp information");
        CASE_INFO(tRNS,                                         "Transparency");
        CASE_INFO(vpAg,                                         "Virtual page");
        CASE_INFO(zTXt,                                         "Compressed textual data");
        default :
            if (Element_Code & 0x20000000) {
                Skip_XX(Element_Size,                           "(Unknown)"); 
                Data_Size = (int64u)-1;
            }
            else {
                Data_Common();
            }
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
    Data_Common();
    if (Retrieve_Const(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format)).empty()) {
        Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "PNG");
        Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), "PNG");
    }
    if (Config->ParseSpeed < 1.0 && Signature != 0x8B4A4E47 && Config->File_Names_Pos > 1) {
        if (Data_Size != (int64u)-1) {
            if (File_Size == (int64u)-1) {
                Data_Size = (int64u)-1;
            }
            else {
                Data_Size += IsSub ? (Buffer_Size - (Buffer_Offset + Element_TotalSize_Get())) : (File_Size - (File_Offset + Buffer_Offset + Element_TotalSize_Get())); // We bet that there is no metadata at the end of the file
            }
        }
        Finish();
    }
}

//---------------------------------------------------------------------------
void File_Png::JDAT()
{
    //Parsing
    #if defined(MEDIAINFO_JPEG_YES)
        File_Jpeg MI;
        MI.StreamKind = StreamKind;
        Open_Buffer_Init(&MI);
        Open_Buffer_Continue(&MI);
        Open_Buffer_Finalize(&MI);
        Merge(MI, StreamKind, 0, 0);
        const auto& StreamSize = Retrieve_Const(StreamKind, 0, Fill_Parameter(StreamKind_Last, Generic_StreamSize));
        if (StreamSize.empty()) {
            Data_Size = (int64u)-1;
        }
        else {
            Data_Size += Header_Size + Element_TotalSize_Get() - Element_Size + StreamSize.To_int64u();
        }
        Clear(StreamKind, 0, Fill_Parameter(StreamKind_Last, Generic_StreamSize));
    #else
        Skip_XX(Element_Size,                                   "JPEG data");
        Data_Common();
    #endif
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
            auto Packing=Png_Colour_type_Settings(Colour_type, Bit_depth);
            Fill(StreamKind_Last, 0, "Format_Settings_Packing", Packing);
            Fill(StreamKind_Last, 0, "Format_Settings", Packing);
            if (Signature != 0x8A4D4E47) { // Not MNG
                Fill(StreamKind_Last, 0, "Width", Width);
                Fill(StreamKind_Last, 0, "Height", Height);
            }
            switch (Colour_type)
            {
                case 3:
                    Bit_depth=8; // From spec: "indexed-colour PNG images (colour type 3), in which the sample depth is always 8 bits" (sample depth is our bit depth
                    [[fallthrough]];
                case 0 :
                case 2:
                case 4:
                case 6:
                    string ColorSpace=(Colour_type&(1<<1))?"RGB":"Y";
                    if (Colour_type&(1<<2))
                        ColorSpace+='A';
                    Fill(StreamKind_Last, 0, "ColorSpace", ColorSpace);
                    break;
            }
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

    Data_Common();
}


//---------------------------------------------------------------------------
void File_Png::MHDR()
{
    //Parsing
    int32u Width, Height, Frequency, FrameCount, Duration;
    Get_B4 (Width,                                              "Width");
    Get_B4 (Height,                                             "Height");
    Get_B4 (Frequency,                                          "Ticks per second");
    Skip_B4(                                                    "Nominal layer count");
    Get_B4 (FrameCount,                                         "Nominal frame count:");
    Get_B4 (Duration,                                           "Nominal play time:");
    Skip_B4(                                                    "Simplicity profile");

    FILLING_BEGIN_PRECISE();
        {
            Fill(Stream_Video, 0, Video_Width, Width);
            Fill(Stream_Video, 0, Video_Height, Height);
            if (Frequency && Duration && Duration != (int32u)-1) {
                Fill(Stream_Video, 0, Video_Height, ((float32)Duration) / Frequency);
            }
            if (FrameCount) {
                Fill(Stream_Video, 0, Video_FrameCount, FrameCount);
            }
        }
    FILLING_END();

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Png::caBX()
{
    //Parsing
    #if defined(MEDIAINFO_C2PA_YES)
        File_C2pa MI;
        Open_Buffer_Init(&MI);
        Open_Buffer_Continue(&MI);
        Open_Buffer_Finalize(&MI);
        Merge(MI, Stream_General, 0, 0, false);
        Merge(MI);
    #endif
}

//---------------------------------------------------------------------------
void File_Png::cICP()
{
    //Parsing
    int8u ColourPrimaries, TransferFunction, MatrixCoefficients, VideoFullRangeFlag;
    Get_B1 (ColourPrimaries,                                    "Colour Primaries"); Param_Info1(Mpegv_colour_primaries(ColourPrimaries));
    Get_B1 (TransferFunction,                                   "Transfer Function"); Param_Info1(Mpegv_transfer_characteristics(TransferFunction));
    Get_B1 (MatrixCoefficients,                                 "Matrix Coefficients"); Param_Info1(Mpegv_matrix_coefficients(MatrixCoefficients));
    Get_B1 (VideoFullRangeFlag,                                 "Video Full Range Flag"); Param_Info1(Mk_Video_Colour_Range(VideoFullRangeFlag + 1));

    FILLING_BEGIN()
        Fill(StreamKind_Last, StreamPos_Last, "colour_description_present", "Yes");
        auto colour_primaries=Mpegv_colour_primaries(ColourPrimaries);
        Fill(StreamKind_Last, StreamPos_Last, "colour_primaries", (*colour_primaries)?colour_primaries:std::to_string(ColourPrimaries).c_str());
        auto transfer_characteristics=Mpegv_transfer_characteristics(TransferFunction);
        Fill(StreamKind_Last, StreamPos_Last, "transfer_characteristics", (*transfer_characteristics)?transfer_characteristics:std::to_string(TransferFunction).c_str());
        auto matrix_coefficients=Mpegv_matrix_coefficients(MatrixCoefficients);
        Fill(StreamKind_Last, StreamPos_Last, "matrix_coefficients", (*matrix_coefficients)?matrix_coefficients:std::to_string(MatrixCoefficients).c_str());
        Ztring ColorSpace=Mpegv_matrix_coefficients_ColorSpace(MatrixCoefficients);
        if (!ColorSpace.empty() && ColorSpace!=Retrieve_Const(StreamKind_Last, StreamPos_Last, "ColorSpace"))
            Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", Mpegv_matrix_coefficients_ColorSpace(MatrixCoefficients));
        Fill(StreamKind_Last, StreamPos_Last, "colour_range", Mk_Video_Colour_Range(VideoFullRangeFlag+1));
    FILLING_END()

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Png::cLLI()
{
    //Parsing
    Ztring MaxCLL, MaxFALL;
    Get_LightLevel(MaxCLL, MaxFALL, 10000);

    FILLING_BEGIN();
        Fill(StreamKind_Last, StreamPos_Last, "MaxCLL", MaxCLL);
        Fill(StreamKind_Last, StreamPos_Last, "MaxFALL", MaxFALL);
    FILLING_END();

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Png::eXIf()
{
    Element_Info1("Exif");

    //Parsing
    #if defined(MEDIAINFO_EXIF_YES)
    File_Exif MI;
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
    Merge(MI, Stream_General, 0, 0, false);
    Merge(MI, Stream_Image, 0, 0, false);
    size_t Count = MI.Count_Get(Stream_Image);
    for (size_t i = 1; i < Count; ++i) {
        Merge(MI, Stream_Image, i, StreamPos_Last + 1, false);
    }
    #else
    Skip_UTF8(Element_Size - Element_Offset,                    "EXIF Tags");
    #endif
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

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Png::gdAT()
{
    Stream_Prepare(Stream_Image);
    Fill(Stream_Image, StreamPos_Last, Image_Type, "Gain map");

    //Parsing
    File_Png MI;
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
    Merge(MI, Stream_Image, 0, StreamPos_Last);
    if (MI.GainMap_metadata_ISO) {
        GainMap_metadata_ISO.reset(new GainMap_metadata());
        *static_cast<GainMap_metadata*>(GainMap_metadata_ISO.get()) = *static_cast<GainMap_metadata*>(MI.GainMap_metadata_ISO.get());
    }
}

//---------------------------------------------------------------------------
void File_Png::gmAP()
{
    //Parsing
    File_GainMap MI;
    GainMap_metadata_ISO.reset(new GainMap_metadata());
    MI.output = static_cast<GainMap_metadata*>(GainMap_metadata_ISO.get());
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
}

//---------------------------------------------------------------------------
void File_Png::iCCP()
{
    //Parsing
    int8u Compression;
    size_t Zero=(size_t)Element_Offset;
    for (; Zero<Element_Size; Zero++)
        if (!Buffer[Buffer_Offset+Zero])
            break;
    if (Zero>=Element_Size)
    {
        Skip_XX(Element_Size-Element_Offset,                    "(Problem)");
        return;
    }
    Skip_XX(Zero-Element_Offset,                                "Profile name");
    Skip_B1(                                                    "Null separator");
    Get_B1 (Compression,                                        "Compression method");

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
        strm.avail_out=0x1000000; //Blocks of 64 KiB, arbitrary chosen, as a begin //TEMP increase
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
            delete[](strm.next_out - strm.total_out); strm.next_out=UncompressedData_New;
            strm.next_out=strm.next_out+strm.total_out;
            strm.avail_out=UncompressedData_NewMaxSize-strm.total_out;
        }
        auto Buffer=(const char*)strm.next_out-strm.total_out;
        auto Buffer_Size=(size_t)strm.total_out;
        inflateEnd(&strm);
        #if defined(MEDIAINFO_ICC_YES)
            File_Icc ICC_Parser;
            ICC_Parser.StreamKind=StreamKind_Last;
            ICC_Parser.IsAdditional=true;
            Open_Buffer_Init(&ICC_Parser);
            Open_Buffer_Continue(&ICC_Parser, (const int8u*)Buffer, Buffer_Size);
            Open_Buffer_Finalize(&ICC_Parser);
            Merge(ICC_Parser, StreamKind_Last, 0, 0);
            delete[] Buffer;
        #else
            Skip_XX(Element_Size-Element_Offset,                "ICC profile");
        #endif
    }
    else
        Skip_XX(Element_Size-Element_Offset,                    "ICC profile");

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Png::mDCV()
{
    Ztring MasteringDisplay_ColorPrimaries, MasteringDisplay_Luminance;
    Get_MasteringDisplayColorVolume(MasteringDisplay_ColorPrimaries, MasteringDisplay_Luminance);

    FILLING_BEGIN();
        Fill(StreamKind_Last, StreamPos_Last, "HDR_Format", "SMPTE ST 2086");
        Fill(StreamKind_Last, StreamPos_Last, "HDR_Format_Compatibility", "HDR10");
        Fill(StreamKind_Last, StreamPos_Last, "MasteringDisplay_ColorPrimaries", MasteringDisplay_ColorPrimaries);
        Fill(StreamKind_Last, StreamPos_Last, "MasteringDisplay_Luminance", MasteringDisplay_Luminance);
    FILLING_END();

    Data_Common();
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

    Data_Common();
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

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Png::tIME()
{
    //Parsing
    int16u YY;
    int8u MM, DD, hh, mm, ss;
    Get_B2 (YY,                                             "Year");
    Get_B1 (MM,                                             "Month");
    Get_B1 (DD,                                             "Day");
    Get_B1 (hh,                                             "Hour");
    Get_B1 (mm,                                             "Minute");
    Get_B1 (ss,                                             "Second");

    FILLING_BEGIN()
        if (MM && MM <= 12 && DD && DD < 32 && hh < 24 && mm < 61 && ss <= 61) {
            auto MM1 = MM / 10;
            auto MM0 = MM % 10;
            auto DD1 = DD / 10;
            auto DD0 = DD % 10;
            auto hh1 = hh / 10;
            auto hh0 = hh % 10;
            auto mm1 = mm / 10;
            auto mm0 = mm % 10;
            auto ss1 = ss / 10;
            auto ss0 = ss % 10;
            Fill(Stream_General, 0, General_Encoded_Date,
                std::to_string(YY) + '-' +
                char('0' + MM1) + char('0' + MM0) + '-' +
                char('0' + DD1) + char('0' + DD0) + 'T' +
                char('0' + hh1) + char('0' + hh0) + ':' +
                char('0' + mm1) + char('0' + mm0) + ':' +
                char('0' + ss1) + char('0' + ss0) + 'Z');
        }
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
    int8u Compression{};
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
                delete[](strm.next_out - strm.total_out); strm.next_out=UncompressedData_New;
                strm.next_out=strm.next_out+strm.total_out;
                strm.avail_out=UncompressedData_NewMaxSize-strm.total_out;
            }
            auto Buffer=(const char*)strm.next_out-strm.total_out;
            auto Buffer_Size=(size_t)strm.total_out;
            inflateEnd(&strm);
            if (Method[IsUTF8])
                Text.From_UTF8(Buffer, Buffer_Size);
            else
                Text.From_ISO_8859_1(Buffer, Buffer_Size);
            inflateEnd(&strm);
            delete[](strm.next_out - strm.total_out);
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
            #if defined(MEDIAINFO_XMP_YES)
            auto Text_UTF8=Text.To_UTF8();
            File_Xmp MI;
            Open_Buffer_Init(&MI, Text_UTF8.size());
            Open_Buffer_Continue(&MI, (const int8u*)Text_UTF8.c_str(), Text_UTF8.size());
            Open_Buffer_Finalize(&MI);
            Element_Show(); //TODO: why is it needed?
            Merge(MI, Stream_General, 0, 0, false);
            Text.clear();
            #endif
        }
        else if (Keyword_UTF8.rfind("Raw profile type ", 0) == 0)
        {
            Decode_RawProfile(Text.To_UTF8().c_str(), Text.To_UTF8().size(), Keyword_UTF8.substr(17));
            Text.clear();
        }
        else if (!Language.empty())
            Text.insert(0, __T('(')+Language+__T(')'));
        Fill(Stream_General, 0, Keyword_UTF8.c_str(), Text);
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Png::Decode_RawProfile(const char* in, size_t in_len, const string& type)
{
#if defined(MEDIAINFO_EXIF_YES) || defined(MEDIAINFO_ICC_YES) || defined(MEDIAINFO_IIM_YES)
    auto HexStringToBytes{
        [](const char* src, size_t len, size_t expected_length) -> string {
            string to_return;
            auto end = src + len;
            to_return.resize(expected_length);
            size_t actual_length = 0;
            bool is_even = true;
            for (auto dst = &to_return[0]; actual_length < expected_length && src < end; ++src) {
                if (*src == '\n') {
                    continue;
                }
                auto c = *src;
                if (c >= '0' && c <= '9')
                    c -= '0';
                else if (c >= 'A' && c <= 'F')
                    c -= 'A' - 10;
                else if (c >= 'a' && c <= 'f')
                    c -= 'a' - 10;
                else {
                    return {};
                }
                *dst += c << (is_even << 2);
                if (!is_even) {
                    ++dst;
                    ++actual_length;
                }
                is_even = !is_even;
            }
            if (actual_length != expected_length) {
                return {};
            }
            return to_return;
        }
    };

    if (!in || !in_len) {
        return;
    }

    // '\n<profile name>\n<length>(%8lu)\n<hex payload>\n'
    if (*in != '\n') {
        return;
    }
    auto src = in + 1;

    // skip the profile name and extract the length.
    while (*src != '\0' && *src++ != '\n') {
    }
    char* end;
    auto expected_length = strtoul(src, &end, 10);
    if (*end != '\n') {
        return;
    }
    ++end;

    // 'end' now points to the profile payload.
    auto data = HexStringToBytes(end, in_len - (end - in), expected_length);
    if (data.empty())
        return;
    
    // Parsing
    size_t Pos = 0;
    if (!data.compare(0, 6, "Exif\0\0", 6))
        Pos = 6;

    std::unique_ptr<File__Analyze> MI;
#if defined(MEDIAINFO_PSD_YES)
    if (type == "8bim" || (type == "iptc" && !data.compare(0, 4, "8bim", 4))) {
        auto Parser = new File_Psd;
        Parser->Step = File_Psd::Step_ImageResourcesBlock;
        MI.reset(Parser);
    }
#endif //MEDIAINFO_PSD_YES
#if defined(MEDIAINFO_EXIF_YES)
    if (type == "APP1" || type == "exif") {
        MI.reset(new File_Exif);
    }
#endif //MEDIAINFO_EXIF_YES
#if defined(MEDIAINFO_ICC_YES)
    if (type == "icc" || type == "icm") {
        MI.reset(new File_Icc);
    }
#endif //MEDIAINFO_ICC_YES
#if defined(MEDIAINFO_IIM_YES)
    if (type == "iptc" && data.compare(0, 4, "8bim", 4)) {
        MI.reset(new File_Iim);
    }
#endif //MEDIAINFO_IIM_YES
#if defined(MEDIAINFO_XMP_YES)
    if (type == "xmp") {
        MI.reset(new File_Xmp);
    }
#endif //MEDIAINFO_ICC_YES
    if (!MI) {
        return;
    }

    auto Buffer_Save = Buffer;
    auto Buffer_Offset_Save = Buffer_Offset;
    auto Buffer_Size_Save = Buffer_Size;
    auto Element_Offset_Save = Element_Offset;
    auto Element_Size_Save = Element_Size;
    Buffer = (const int8u*)data.c_str() + Pos;
    Buffer_Offset = 0;
    Buffer_Size = data.size() - Pos;
    Element_Offset = 0;
    Element_Size = Buffer_Size;

    Open_Buffer_Init(MI.get());
    Open_Buffer_Continue(MI.get());
    Open_Buffer_Finalize(MI.get());
    Merge(*MI, Stream_General, 0, 0, false);
    Merge(*MI, Stream_Image, 0, 0, false);
    size_t Count = MI->Count_Get(Stream_Image);
    for (size_t i = 1; i < Count; ++i) {
        Merge(*MI, Stream_Image, i, StreamPos_Last + 1, false);
    }

    Buffer = Buffer_Save;
    Buffer_Offset = Buffer_Offset_Save;
    Buffer_Size = Buffer_Size_Save;
    Element_Offset = Element_Offset_Save;
    Element_Size = Element_Size_Save;
#endif
}

//---------------------------------------------------------------------------
void File_Png::Data_Common()
{
    Skip_XX(Element_Size - Element_Offset,                      "(Unknown)");
    if (Data_Size != (int64u)-1) {
        Data_Size += Header_Size + Element_TotalSize_Get();
    }
}

} //NameSpace

#endif
