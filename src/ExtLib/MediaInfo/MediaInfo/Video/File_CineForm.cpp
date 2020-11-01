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
#if defined(MEDIAINFO_CINEFORM_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/Video/File_CineForm.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Tags[]=
{
    "Padding",
    "SampleType",
    "SampleIndexTable",
    "SampleIndexEntry",
    "BitstreamMarker",
    "VersionMajor",
    "VersionMinor",
    "VersionRevision",
    "VersionEdit",
    "SequenceFlags",
    "TransformType",
    "NumFrames",
    "NumChannels",
    "NumWavelets",
    "NumSubbands",
    "NumSpatial",
    "FirstWavelet",
    "ChannelByteSize",
    "GroupTrailer",
    "ImageType",
    "ImageWidth",
    "ImageHeight",
    "ImageFormat",
    "ImageIndex",
    "ImageTrailer",
    "LowpassSubband",
    "NumLevels",
    "LowpassWidth",
    "LowpassHeight",
    "MarginTop",
    "MarginBottom",
    "MarginLeft",
    "MarginRight",
    "PixelOffset",
    "Quantization",
    "PixelDepth",
    "LowpassTrailer",
    "WaveletType",
    "WaveletNumber",
    "WaveletLevel",
    "NumBands",
    "HighpassWidth",
    "HighpassHeight",
    "LowpassBorder",
    "HighpassBorder",
    "LowpassScale",
    "LowpassDivisor",
    "HighpassTrailer",
    "BandNum",
    "BandWidth",
    "BandHeight",
    "BandSubband",
    "BandEncoding",
    "BandQuantization",
    "BandScale",
    "BandHeader",
    "BandTrailer",
    "NumZeroValues",
    "NumZeroTrees",
    "NumPositives",
    "NumNegative",
    "ZeroNodes",
    "ChannelNumber",
    "InterlacedFlags",
    "ProtectionFlags",
    "PixtureAspectRatioX",
    "PixtureAspectRatioY",
    "SubBand", //Unused
    "SampleFlags",
    "FrameNumber",
    "Precision",
    "InputFormat",
    "BandCodingFlags",
    "InputColorspace",
    "PeakLevel",
    "PeakTableOffsetLow",
    "PeakTableOffsetHigh",
    "TagSampleEnd",
    "TagCount",
    "TagVersion", // 4 version, 4 subversion, 8 subsubversion
    "QualityLow",
    "QualityHigh",
    "BandSecondPass",
    "PrescaleTable",
    "EncodedFormat",
    "DisplayHeight",
    "DisplayWidth",
    "DisplayOffsetX",
    "DisplayOffsetY",
    "ColorSpaceOld",
    "ColorSpaceOld3pt3",
    "EncodedChannels",
    "EncodedChannelNum",
    "EncodedChannelQuality",
    "Skip",
    "PresentationHeight",
    "PresentationWidth",
    NULL,
    NULL,
    NULL,
    NULL,
    "BitsPerComponent",
    "MaxBitsPerComponent",
    NULL,
    "ChannelWidth?",
    "ChannelHeight?",
    "PatternWidth",
    "PatternHeight",
    "ComponentsPerSample",
    "PrescaleShift/AlphaSampling",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "LayerCount",
    "LayerNumber",
    "LayerPattern",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "NOP",
};

static Ztring Tag_String(int16u Value)
{
    if (Value>=sizeof(Tags)/sizeof(const char*) || !Tags[Value])
        return Ztring::ToZtring(Value);
    return Ztring().From_UTF8(Tags[Value]);
}

//---------------------------------------------------------------------------
static const char* color_spaces[] =
{
    "RGB",
    "YUV",
    "RGGB",
};
enum color_space_enum
{
    ColorSpace_Unkn, // Unknown
    ColorSpace_RGB,
    ColorSpace_YUV,
    ColorSpace_RGGB,
};
enum pattern_enum
{
    Pattern_Unk = 0x00, // Unknown
    Pattern_422 = 0x21,
    Pattern_420 = 0x22,
    Pattern_411 = 0x41,
    Pattern_410 = 0x44,
};

struct input_format
{
    int16u                      InputFormat;
    int8u                       ColorSpace; // color_space_enum
    int8u                       Pattern; // (PatternWidth<<4)|PatternHeight
    int16u                      ChannelCount;
    int16u                      BitsPerComponent;
};
static const input_format InputFormatInfo[] =
{
    {  0, ColorSpace_Unkn, Pattern_Unk, 0,  0},
    {  1, ColorSpace_YUV , Pattern_422, 3,  8},
    {  2, ColorSpace_YUV , Pattern_422, 3,  8},
    {  3, ColorSpace_YUV , Pattern_422, 3,  8},
    {  7, ColorSpace_RGB , Pattern_Unk, 3,  8},
    {  8, ColorSpace_RGB , Pattern_Unk, 3,  8},
    {  9, ColorSpace_RGB , Pattern_Unk, 3,  8},
    { 10, ColorSpace_YUV , Pattern_422, 3, 10},
    { 16, ColorSpace_YUV , Pattern_420, 3,  8},
    { 17, ColorSpace_YUV , Pattern_420, 3,  8},
    { 30, ColorSpace_RGB , Pattern_Unk, 4, 16},
    { 32, ColorSpace_RGB , Pattern_Unk, 4,  8},
    { 33, ColorSpace_YUV , Pattern_422, 3,  8},
    { 35, ColorSpace_YUV , Pattern_Unk, 4,  8},
    { 36, ColorSpace_YUV , Pattern_Unk, 4,  8},
    {100, ColorSpace_RGGB, Pattern_Unk, 4,  0},
    {101, ColorSpace_RGGB, Pattern_Unk, 4,  8},
    {102, ColorSpace_RGGB, Pattern_Unk, 4, 10},
    {103, ColorSpace_RGGB, Pattern_Unk, 4, 10},
    {104, ColorSpace_RGGB, Pattern_Unk, 4, 16},
    {105, ColorSpace_RGGB, Pattern_Unk, 4, 12},
    {120, ColorSpace_RGB , Pattern_Unk, 3, 16},
    {121, ColorSpace_RGB , Pattern_Unk, 4, 16},
    {122, ColorSpace_RGB , Pattern_Unk, 3, 10},
    {123, ColorSpace_RGB , Pattern_Unk, 3, 10},
    {128, ColorSpace_RGB , Pattern_Unk, 3, 10},
    {129, ColorSpace_YUV , Pattern_422, 3, 10},
    {130, ColorSpace_YUV , Pattern_Unk, 4, 16},
    {131, ColorSpace_YUV , Pattern_Unk, 4, 10},
    {132, ColorSpace_RGB , Pattern_Unk, 4, 16},
};

static void InputFormat_Fill(input_format& ToFill)
{
    if (!ToFill.InputFormat)
        return;

    const size_t Size=sizeof(InputFormatInfo)/sizeof(InputFormatInfo[0]);
    for (size_t i=0; i<Size; i++)
        if (ToFill.InputFormat==InputFormatInfo[i].InputFormat)
        {
            if (!ToFill.ColorSpace)
                ToFill.ColorSpace=InputFormatInfo[i].ColorSpace;
            if (!ToFill.Pattern)
                ToFill.Pattern=InputFormatInfo[i].Pattern;
            if (!ToFill.ChannelCount)
                ToFill.ChannelCount=InputFormatInfo[i].ChannelCount;
            if (!ToFill.BitsPerComponent)
                ToFill.BitsPerComponent=InputFormatInfo[i].BitsPerComponent;
            return;
        }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_CineForm::File_CineForm()
:File__Analyze()
{
}


//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_CineForm::Streams_Fill()
{
    //Filling
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "CineForm");
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_CineForm::Read_Buffer_Continue()
{
    input_format InputFormat;
    InputFormat.InputFormat=0;
    InputFormat.ColorSpace=ColorSpace_Unkn; // Does not really exist
    InputFormat.Pattern=Pattern_Unk; // Does not really exist
    InputFormat.ChannelCount=0;
    InputFormat.BitsPerComponent=0;
    int16u ImageWidth=0;
    int16u ImageHeight=0;
    int32u SampleFlags=(int32u)-1;
    int16u EncodedFormat=0;
    int16u PatternWidth=0;
    int16u PatternHeight=0;
    int16u InterlacedFlags=0;
    int16u PixtureAspectRatioX=0;
    int16u PixtureAspectRatioY=0;

    while (Element_Size-Element_Offset>=4)// && Mask!=Mask_Total)
    {
        // Tag
        // 0x0000 - 0x1FFF: Tag-Value (2+2)
        // 0x2000 - 0x3FFF: Large (1+3)
        // 0x4000 - 0x5FFF: Small (2+2)
        // 0x6000 - 0x7FFF: Large (1+3)
        // 0x8000 - 0xFFFF: optional values, real value is absolute of the 2-complement
        Element_Begin0();
        int16u Tag, Value;
        Get_B2 (Tag,                                            "Tag");
        Get_B2 (Value,                                          "Value");
        if (Tag&0x8000)
            Tag=(int16u)(-((int16s)Tag)); // Optional tags are "negative"

        int32u Size=0;
        if (Tag&0x2000) //Large
        {
            Element_Name("Large");
            Element_Info1(Ztring::ToZtring(Tag>>8));
            Size=Tag&0xFF;
            Size<<=16;
            Size|=Value;
            Size*=4;
            Skip_XX(Size,                                       "Data");
        }
        else if (Tag&0x4000) //Small
        {
            Element_Name("Small");
            Element_Info1(Ztring::ToZtring(Tag));
            int32u Size=Value;
            Size*=4;
            Skip_XX(Size,                                       "Data");
        }
        else //Tag-Value (2+2)
        {
            Element_Name(Tag_String(Tag));
            Element_Info1(Value);

            switch (Tag)
            {
                case   2: //SampleIndexTable
                    for (int16u i=0; i<Value; i++)
                        Skip_B4(                                "Offset");
                    break;
                case  12: InputFormat.ChannelCount=Value; break;
                case  20: ImageWidth=Value; break;
                case  21: if (!ImageHeight) ImageHeight=Value; break; //CroppedHeight has priority
                case  68: SampleFlags=Value; break;
                case  65: PixtureAspectRatioX=Value; break;
                case  66: PixtureAspectRatioY=Value; break;
                case  70: if (!InputFormat.BitsPerComponent) InputFormat.BitsPerComponent=Value; break; // BitsPerComponent has the priority
                case  71: InputFormat.InputFormat=Value; break;
                case  84: EncodedFormat=Value; break;
                case  85: ImageHeight=Value; break; //CroppedHeight
                case 101: InputFormat.BitsPerComponent=Value; break;
                case 106: PatternWidth=Value; break;
                case 107: PatternHeight=Value; break;
                default:;
            }
        }
        Element_End0();
    }
    Element_Offset=Element_Size;

    FILLING_BEGIN();
        Accept("CineForm");
        if (ImageWidth)
            Fill(Stream_Video, 0, Video_Width, ImageWidth);
        if (ImageHeight)
            Fill(Stream_Video, 0, Video_Height, ImageHeight);
        if (SampleFlags!=(int32u)-1)
            Fill(Stream_Video, 0, Video_ScanType, (SampleFlags&1)?"Progressive":"Interlaced");
        switch (EncodedFormat)
        {
            case 1:
                    InputFormat.ColorSpace=ColorSpace_YUV;
                    if (!PatternWidth && !PatternHeight)
                    {
                        PatternWidth=2;
                        PatternHeight=1;
                    };
                    if (!InputFormat.BitsPerComponent)
                        InputFormat.BitsPerComponent=10;
                    break;
            case 2:
                    InputFormat.ColorSpace=ColorSpace_RGGB;
                    if (!InputFormat.BitsPerComponent)
                        InputFormat.BitsPerComponent=16;
                    break;
            case 3:
                    InputFormat.ColorSpace=ColorSpace_RGB;
                    if (!InputFormat.BitsPerComponent)
                        InputFormat.BitsPerComponent=12;
                    break;
            case 4:
                    InputFormat.ColorSpace=ColorSpace_RGB;
                    if (!InputFormat.ChannelCount)
                        InputFormat.ChannelCount=4;
                    break;
            case 5:
                    InputFormat.ColorSpace=ColorSpace_YUV;
                    if (!InputFormat.ChannelCount)
                        InputFormat.ChannelCount=4;
                    break;
            default:;
        }
        if (PatternWidth<=4 && PatternHeight<=4)
            InputFormat.Pattern=(PatternWidth<<4)|PatternHeight;
        InputFormat_Fill(InputFormat);
        string ColorSpace;
        if (InputFormat.ColorSpace)
        {
            ColorSpace=color_spaces[InputFormat.ColorSpace-1];
            if (InputFormat.ChannelCount==4 && InputFormat.ColorSpace!=ColorSpace_RGGB)
                ColorSpace+='A';
        }
        Fill(Stream_Video, 0, Video_ColorSpace, ColorSpace);
        if (InputFormat.BitsPerComponent)
            Fill(Stream_Video, 0, Video_BitDepth, InputFormat.BitsPerComponent);
        string ChromaSubsampling;
        switch (InputFormat.Pattern)
        {
            case 0x21:ChromaSubsampling="4:2:2"; break;
            case 0x22:ChromaSubsampling="4:2:0"; break;
            case 0x41:ChromaSubsampling="4:1:1"; break;
            case 0x44:ChromaSubsampling="4:1:0"; break;
            default:;
        }
        Fill(Stream_Video, 0, Video_ChromaSubsampling, ChromaSubsampling);
        Fill(Stream_Video, 0, Video_ScanType, (InterlacedFlags&0x01)?"Interlaced":"Progressive");
        if (InterlacedFlags&0x01)
            Fill(Stream_Video, 0, Video_ScanType, (InterlacedFlags&0x02)?"TFF":"BFF");
        //TODO: bit 2 field 1 only, bit 3 field 2 only, bit 4 Dominance (?)
        if (PixtureAspectRatioX && PixtureAspectRatioY)
            Fill(Stream_Video, 0, Video_DisplayAspectRatio, ((float32)PixtureAspectRatioX)/PixtureAspectRatioY);
        Fill();
    FILLING_END();
    Finish();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_CINEFORM_*
