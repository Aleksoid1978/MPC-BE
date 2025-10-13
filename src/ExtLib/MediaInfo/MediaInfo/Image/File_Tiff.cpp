/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// TIFF Format
//
// From
// http://partners.adobe.com/public/developer/en/tiff/TIFF6.pdf
// http://partners.adobe.com/public/developer/en/tiff/TIFFphotoshop.pdf
// http://www.fileformat.info/format/tiff/
// http://en.wikipedia.org/wiki/Tagged_Image_File_Format
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
#if defined(MEDIAINFO_TIFF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_Tiff.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
namespace Tiff_Tag
{
    const int16u SubFileType                = 254;
    const int16u ImageWidth                 = 256;
    const int16u ImageLength                = 257;
    const int16u BitsPerSample              = 258;
    const int16u Compression                = 259;
    const int16u PhotometricInterpretation  = 262;
    const int16u Make                       = 271;
    const int16u Model                      = 272;
    const int16u StripOffsets               = 273;
    const int16u SamplesPerPixel            = 277;
    const int16u RowsPerStrip               = 278;
    const int16u StripByteCounts            = 279;
    const int16u XResolution                = 282;
    const int16u YResolution                = 283;
    const int16u PlanarConfiguration        = 284;
    const int16u ResolutionUnit             = 296;
    const int16u Software                   = 305;
    const int16u ExtraSamples               = 338;
    const int16u JPEGTables                 = 347;
    const int16u JPEGProc                   = 512;
    const int16u JPEGInterchangeFormat      = 513;
    const int16u JPEGInterchangeFormatLngth = 514;
    const int16u JPEGRestartInterval        = 515;
    const int16u JPEGLosslessPredictors     = 517;
    const int16u JPEGPointTransforms        = 518;
    const int16u JPEGQTables                = 519;
    const int16u JPEGDCTables               = 520;
    const int16u JPEGACTables               = 521;
}

//---------------------------------------------------------------------------
const char* Tiff_Compression_Name(int16u compression)
{
    switch (compression)
    {
    case 1: return "Raw";
    case 2: return "CCITT T.4/Group 3 1D Fax";
    case 3: return "CCITT T.4/Group 3 Fax";
    case 4: return "CCITT T.6/Group 4 Fax";
    case 5: return "LZW";
    case 6:
    case 7:
    case 99: return "JPEG";
    case 8: return "Adobe Deflate";
    case 9: return "JBIG B&W";
    case 10: return "JBIG Color";
    case 262: return "Kodak 262";
    case 32766: return "Next";
    case 32767: return "Sony ARW";
    case 32769: return "Packed RAW";
    case 32770: return "Samsung SRW";
    case 32771: return "CCIRLEW";
    case 32772: return "Samsung SRW 2";
    case 32773: return "PackBits";
    case 32809: return "Thunderscan";
    case 32867: return "Kodak KDC";
    case 32895: return "IT8CTPAD";
    case 32896: return "IT8LW";
    case 32897: return "IT8MP";
    case 32898: return "IT8BL";
    case 32908: return "PixarFilm";
    case 32909: return "PixarLog";
    case 32946: return "Deflate";
    case 32947: return "DCS";
    case 33003: return "Aperio JPEG 2000 YCbCr";
    case 33005: return "Aperio JPEG 2000 RGB";
    case 34661: return "JBIG";
    case 34676: return "SGILog";
    case 34677: return "SGILog24";
    case 34712: return "JPEG 2000";
    case 34713: return "Nikon NEF";
    case 34715: return "JBIG2 TIFF FX";
    case 34718: return "Microsoft Document Imaging (MDI) Binary Level Codec";
    case 34719: return "Microsoft Document Imaging (MDI) Progressive Transform Codec";
    case 34720: return "Microsoft Document Imaging (MDI) Vector";
    case 34887: return "ESRI Lerc";
    case 34892: return "Lossy JPEG";
    case 34925: return "LZMA2";
    case 34926:
    case 50000: return "Zstd";
    case 34927:
    case 50001: return "WebP";
    case 34933: return "PNG";
    case 34934: return "JPEG XR";
    case 50002:
    case 52546: return "JPEG XL";
    case 65000: return "Kodak DCR";
    case 65535: return "Pentax PEF";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Tiff_Compression_Mode(int32u Compression)
{
    switch (Compression)
    {
        case     1 :
        case     2 :
        case     3 :
        case     5 :
        case     8 :
        case 32773 : return "Lossless";
        default    : return ""; //Unknown or depends of the compresser (e.g. JPEG can be lossless or lossy)
    }
}

//---------------------------------------------------------------------------
static const char* Tiff_PhotometricInterpretation(int32u PhotometricInterpretation)
{
    switch (PhotometricInterpretation)
    {
        case     0 :
        case     1 : return "B/W or Grey scale";
        case     2 : return "RGB";
        case     3 : return "Palette";
        case     4 : return "Transparency mask";
        case     5 : return "CMYK";
        case     6 : return "YCbCr";
        case     8 : return "CIELAB";
        default    : return ""; //Unknown
    }
}

//---------------------------------------------------------------------------
static const char* Tiff_PhotometricInterpretation_ColorSpace (int32u PhotometricInterpretation)
{
    switch (PhotometricInterpretation)
    {
        case     0 :
        case     1 : return "Y";
        case     2 : return "RGB";
        case     3 : return "RGB"; //Palette
        case     4 : return "A"; //Transparency mask;
        case     5 : return "CMYK";
        case     6 : return "YUV"; //YCbCr
        case     8 : return "CIELAB"; //What is it?
        default    : return ""; //Unknown
    }
}

static const char* Tiff_ExtraSamples_ColorSpace(int32u ExtraSamples)
{
    switch (ExtraSamples)
    {
        case     1 : return "A";
        default    : return ""; //Unknown
    }
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Tiff::Streams_Finish()
{
    if (Infos.find(0) != Infos.end() && Infos[0].find(0xC612) != Infos[0].end()) {
        Fill(Stream_General, 0, General_Format, "DNG");
    }
    if (Retrieve_Const(Stream_General, 0, General_Format).empty()) {
        Fill(Stream_General, 0, General_Format, "TIFF");
    }

    infos::iterator Info;
    size_t StreamPos_Current{};

    for (int i = 0; i < 5; ++i) {
        if (i == 0) {
            StreamPos_Current = 0;
        }
        else {
            if (Infos.find(i) == Infos.end()) {
                continue;
            }
            Stream_Prepare(Stream_Image);
            StreamPos_Current = StreamPos_Last;
        }

        Fill(Stream_Image, StreamPos_Current, Image_Format_Settings, LittleEndian ? "Little" : "Big");
        Fill(Stream_Image, StreamPos_Current, Image_Format_Settings_Endianness, LittleEndian ? "Little" : "Big");

        //Type
        Info = Infos[i].find(Tiff_Tag::SubFileType);
        if (Info != Infos[i].end() && Info->second.Read().To_int64u() & 1)
            Fill(Stream_Image, StreamPos_Current, Image_Type, "Thumbnail");

        //Width
        Info = Infos[i].find(Tiff_Tag::ImageWidth);
        if (Info != Infos[i].end())
            Fill(Stream_Image, StreamPos_Current, Image_Width, Info->second.Read());

        //Height
        Info = Infos[i].find(Tiff_Tag::ImageLength);
        if (Info != Infos[i].end())
            Fill(Stream_Image, StreamPos_Current, Image_Height, Info->second.Read());

        //BitsPerSample
        Info = Infos[i].find(Tiff_Tag::BitsPerSample);
        if (Info != Infos[i].end())
        {
            if (Info->second.size() > 1)
            {
                bool IsOk = true;
                for (size_t Pos = 1; Pos < Info->second.size(); ++Pos)
                    if (Info->second[Pos] != Info->second[0])
                        IsOk = false;
                if (IsOk)
                    Info->second.resize(1); //They are all same, we display 1 piece of information
            }

            Fill(Stream_Image, StreamPos_Current, Image_BitDepth, Info->second.Read());
        }

        //Compression
        Info = Infos[i].find(Tiff_Tag::Compression);
        if (Info != Infos[i].end())
        {
            int32u Value = Info->second.Read().To_int32u();
            Fill(Stream_Image, StreamPos_Current, Image_Format, Tiff_Compression_Name(Value));
            Fill(Stream_Image, StreamPos_Current, Image_Codec, Tiff_Compression_Name(Value));
            Fill(Stream_Image, StreamPos_Current, Image_Compression_Mode, Tiff_Compression_Mode(Value));
        }

        //PhotometricInterpretation
        Info = Infos[i].find(Tiff_Tag::PhotometricInterpretation);
        if (Info != Infos[i].end())
        {
            int32u Value = Info->second.Read().To_int32u();
            Fill(Stream_Image, StreamPos_Current, Image_ColorSpace, Tiff_PhotometricInterpretation_ColorSpace(Value));
            //Note: should we differeniate between raw RGB and palette (also RGB actually...)
        }

        //XResolution
        Info = Infos[i].find(Tiff_Tag::XResolution);
        if (Info != Infos[i].end())
        {
            Fill(Stream_Image, StreamPos_Current, "Density_X", Info->second.Read());
            Fill_SetOptions(Stream_Image, StreamPos_Current, "Density_X", "N NT");
        }

        //YResolution
        Info = Infos[i].find(Tiff_Tag::YResolution);
        if (Info != Infos[i].end())
        {
            Fill(Stream_Image, StreamPos_Current, "Density_Y", Info->second.Read());
            Fill_SetOptions(Stream_Image, StreamPos_Current, "Density_Y", "N NT");
        }

        //ResolutionUnit
        Info = Infos[i].find(Tiff_Tag::ResolutionUnit);
        if (Info != Infos[i].end())
        {
            switch (Info->second.Read().To_int32u())
            {
            case 0: break;
            case 1: Fill(Stream_Image, StreamPos_Current, "Density_Unit", "dpcm"); Fill_SetOptions(Stream_Image, 0, "Density_Unit", "N NT"); break;
            case 2: Fill(Stream_Image, StreamPos_Current, "Density_Unit", "dpi"); Fill_SetOptions(Stream_Image, 0, "Density_Unit", "N NT"); break;
            default: Fill(Stream_Image, StreamPos_Current, "Density_Unit", Info->second.Read()); Fill_SetOptions(Stream_Image, 0, "Density_Unit", "N NT");
            }
        }
        else if (Infos[i].find(Tiff_Tag::XResolution) != Infos[i].end() || Infos[i].find(Tiff_Tag::YResolution) != Infos[i].end())
        {
            Fill(Stream_Image, StreamPos_Current, "Density_Unit", "dpi");
            Fill_SetOptions(Stream_Image, StreamPos_Current, "Density_Unit", "N NT");
        }

        //XResolution or YResolution 
        if (Infos[i].find(Tiff_Tag::XResolution) != Infos[i].end() || Infos[i].find(Tiff_Tag::YResolution) != Infos[i].end())
        {
            Ztring X = Retrieve(Stream_Image, StreamPos_Current, "Density_X");
            if (X.empty())
                X.assign(1, __T('?'));
            Ztring Y = Retrieve(Stream_Image, StreamPos_Current, "Density_Y");
            if (Y.empty())
                Y.assign(1, __T('?'));
            if (X != Y)
            {
                X += __T('x');
                X += Y;
            }
            Y = Retrieve(Stream_Image, StreamPos_Current, "Density_Unit");
            if (!Y.empty())
            {
                X += __T(' ');
                X += Y;
                Fill(Stream_Image, StreamPos_Current, "Density/String", X);
            }
        }

        if (Retrieve_Const(Stream_General, 0, General_Format) == __T("TIFF")) {
            //Software
            Info = Infos[0].find(Tiff_Tag::Software);
            if (Info != Infos[0].end())
                Fill(Stream_General, 0, General_Encoded_Application_Name, Info->second.Read()); // TODO: if this is removed, we lose some info in the displayed string when there are several sources for application name (TIFF, Exif, XMP...)
        }

        //ExtraSamples
        Info = Infos[i].find(Tiff_Tag::ExtraSamples);
        if (Info != Infos[i].end())
        {
            Ztring ColorSpace = Retrieve(Stream_Image, StreamPos_Current, Image_ColorSpace);
            ColorSpace += Ztring().From_UTF8(Tiff_ExtraSamples_ColorSpace(Info->second.Read().To_int32u()));
            Fill(Stream_Image, StreamPos_Current, Image_ColorSpace, ColorSpace, true);
        }
    }

    File_Exif::Streams_Finish();

    //FileSize
    Info = Infos[0].find(Tiff_Tag::StripOffsets);
    if (Info != Infos[0].end())
    {
        size_t ExpectedFileSize_Pos=static_cast<size_t>(-1);
        for (size_t i=0; i<Info->second.size(); i++)
        {
            auto Offset=Info->second[i].To_int64u();
            if (ExpectedFileSize<Offset)
            {
                ExpectedFileSize=Offset;
                ExpectedFileSize_Pos=i;
            }
        }
        if (ExpectedFileSize!=static_cast<int64u>(-1))
        {
            Info = Infos[0].find(Tiff_Tag::StripByteCounts);
            if (Info != Infos[0].end() && ExpectedFileSize_Pos<Info->second.size())
            {
                ExpectedFileSize+=Info->second[ExpectedFileSize_Pos].To_int64u();
            }
        }
        
        if (ExpectedFileSize>File_Size)
            IsTruncated(ExpectedFileSize, false, "TIFF");
    }
}

} //NameSpace

#endif

