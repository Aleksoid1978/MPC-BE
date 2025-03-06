/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
//
// Information about VP9 files
// http://downloads.webmproject.org/docs/vp9/vp9-bitstream_superframe-and-uncompressed-header_v1.0.pdf
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
#if defined(MEDIAINFO_VP9_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Vp9.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "ZenLib/BitStream.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace MediaInfoLib
{
//---------------------------------------------------------------------------

//***************************************************************************
// Info
//***************************************************************************

static const char* Vp9_Feature[] = 
{
    "Profile",
    "Level",
    "Bit Depth",
    "Chroma Subsampling",
};

//---------------------------------------------------------------------------
const char* Mpegv_matrix_coefficients_ColorSpace(int8u matrix_coefficients);
const char* Mpegv_matrix_coefficients(int8u transfer_characteristics);
static const int8u Vp9_ColorSpace[8] =
{
    2,
    5,
    1,
    6,
    7,
    9,
    2,
    0,
};

//---------------------------------------------------------------------------
static const char* Vp9_ColorRange[] =
{
    "Limited",
    "Full",
};

//---------------------------------------------------------------------------
static const char* Vp9_ChromaSubsampling[] =
{
    "",
    "4:4:0",
    "4:2:2",
    "4:2:0",
};

//---------------------------------------------------------------------------
static const int8u Vp9_ChromaSubsampling_OutOfBand[] =
{
    3,
    3,
    2,
    0,
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Vp9::File_Vp9()
:File__Analyze()
{
    //Configuration
    ParserName="VP9";
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
File_Vp9::~File_Vp9()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vp9::Streams_Accept()
{
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "VP9");
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
// https://www.webmproject.org/docs/container/#vp9-codec-feature-metadata-codecprivate
void File_Vp9::Read_Buffer_OutOfBand()
{
    Accept();
    while (Element_Offset<Element_Size)
    {
        Element_Begin1("Feature");
        int8u ID, Size;
        Element_Begin1("Header");
            Get_B1 (ID,                                         "ID");
            Get_B1 (Size,                                       "Size");
        Element_End0();
        int8u Value;
        if (Size==1 && ID && ID<=4)
        {
            Element_Name(Vp9_Feature[ID-1]);
            Get_B1 (Value,                                      "Value");
            switch (ID)
            {
                case 1: Fill(Stream_Video, 0, Video_Format_Profile, Value); break;
                case 2: Fill(Stream_Video, 0, Video_Format_Level, (float)Value/10, 1); break;
                case 3: Fill(Stream_Video, 0, Video_BitDepth, Value); break;
                case 4: if (Value<4)
                        {
                            Fill(Stream_Video, 0, Video_ChromaSubsampling, Vp9_ChromaSubsampling[Vp9_ChromaSubsampling_OutOfBand[Value]]);
                            if (Value<2)
                                Fill(Stream_Video, 0, Video_ChromaSubsampling_Position, "Type "+std::to_string(Value));
                        }
                        break;
            }
        }
        else
        {
            Element_Name(Ztring::ToZtring(ID));
            Skip_XX(Size,                                       "Unknown");
        }
        Element_End0();
    }
}
 
//---------------------------------------------------------------------------
void File_Vp9::Read_Buffer_Continue()
{
    if (!Status[IsAccepted])
        Accept();

    Element_Begin1("uncompressed_header");
    BS_Begin();
    int16u width_minus_one{}, height_minus_one{};
    int8u FRAME_MARKER, profile, bit_depth{}, colorspace{}, subsampling{};
    bool version0, version1, version2, show_existing_frame, frame_type, show_frame, error_resilient_mode, yuv_range_flag{};
    Get_S1(2, FRAME_MARKER,                                     "FRAME_MARKER (0b10)");
    if (FRAME_MARKER!=0x2)
        Trusted_IsNot("FRAME_MARKER is wrong");
    Get_SB(   version0,                                         "version");
    Get_SB(   version1,                                         "high");
    profile=(version1<<1)+version0;
    if (profile==3)
    {
        Get_SB(   version2,                                     "RESERVED_ZERO");
        profile+=version2<<2;
    }
    if (profile>3)
    {
        Skip_BS(Data_BS_Remain(),                               "Unknown");
        BS_End();
        Element_End0();
        return;
    }
    Get_SB(   show_existing_frame,                              "show_existing_frame");
    if (show_existing_frame)
    {
        Skip_S1(3,                                              "index_of_frame_to_show");
        BS_End();
        Element_End0();
        return;
    }
    Get_SB (    frame_type,                                     "frame_type");
    Get_SB (    show_frame,                                     "show_frame");
    Get_SB (    error_resilient_mode,                           "error_resilient_mode");
    int Has_SyncCode_Color_Refresh; // bit 0 = SyncCode, bit 1 = Color, bit 2 = refresh
    if (!frame_type) //I-Frame
        Has_SyncCode_Color_Refresh=3;
    else
    {
        if (show_frame)
        {
            bool intra_only;
            Get_SB (   intra_only,                              "intra_only");
            if (intra_only)
                Has_SyncCode_Color_Refresh=profile>0?7:5;
            else
                Has_SyncCode_Color_Refresh=0;
            if (!error_resilient_mode)
                Skip_SB(                                        "reset_frame_context");

        }
        else
            Has_SyncCode_Color_Refresh=0;
    }
    if (Has_SyncCode_Color_Refresh)
    {
        int32u SYNC_CODE;
        Get_S3 (24, SYNC_CODE,                                  "SYNC_CODE (0x498342)");
        if (SYNC_CODE!=0x498342)
            Trusted_IsNot("SYNC_CODE is wrong");
        if (Has_SyncCode_Color_Refresh&2)
        {
            Element_Begin1("bitdepth_colorspace_sampling");
            if (profile>1)
            {
                bool bit_depth_flag;
                Get_SB (   bit_depth_flag,                      "bit_depth_flag");
                bit_depth=bit_depth_flag?12:10;
                Param_Info2(bit_depth, " bits");
            }
            else
                bit_depth=8;
            Get_S1 (3, colorspace,                              "colorspace");
            colorspace=Vp9_ColorSpace[colorspace];
            Param_Info1(Mpegv_matrix_coefficients_ColorSpace(colorspace));
            if (colorspace) // not sRGB
            {
                Get_SB (   yuv_range_flag,                      "yuv_range_flag");
                switch (profile)
                {
                    case 1:
                    case 3:
                        {
                        bool subsampling_x, subsampling_y;
                        Get_SB (subsampling_x,                  "subsampling_x");
                        Get_SB (subsampling_y,                  "subsampling_y");
                        subsampling=(subsampling_x<<1)+subsampling_y;
                        Skip_SB(                                "reserved");
                        }
                        break;
                    default:
                        subsampling=3;
                }
            }
            Element_End0();
        }
        else
            Skip_SB(                                            "reserved");
    }
    if (Has_SyncCode_Color_Refresh&4)
        Skip_S1( 8,                                             "refresh_frame_flags");
    if (Has_SyncCode_Color_Refresh)
    {
        Element_Begin1("frame_size");
        bool has_scaling;
        Get_S2 (16, width_minus_one,                            "width_minus_one");
        Get_S2 (16, height_minus_one,                           "height_minus_one");
        Get_SB (    has_scaling,                                "has_scaling");
        if (has_scaling)
        {
            Get_S2 (16, width_minus_one,                        "render_width_minus_one");
            Get_S2 (16, height_minus_one,                       "render_height_minus_one");
        }
        Element_End0();
    }
    Skip_BS(Data_BS_Remain(),                                   "(Not parsed)");
    BS_End();
    Element_End0();

    FILLING_BEGIN()
        if (!Frame_Count)
        {
            if (Has_SyncCode_Color_Refresh)
            {
                if (Has_SyncCode_Color_Refresh>>1)
                {
                    Fill(Stream_Video, 0, Video_Format_Profile, profile, 10, true);
                    Fill(Stream_Video, 0, Video_BitDepth, bit_depth, 10, true);
                    Fill(Stream_Video, 0, Video_ColorSpace, Mpegv_matrix_coefficients_ColorSpace(colorspace));
                    Fill(Stream_Video, 0, Video_matrix_coefficients, Mpegv_matrix_coefficients(colorspace));
                    if (colorspace)
                    {
                        Fill(Stream_Video, 0, Video_ChromaSubsampling, Vp9_ChromaSubsampling[subsampling], Unlimited, true, true);
                        Fill(Stream_Video, 0, Video_colour_range, Vp9_ColorRange[yuv_range_flag]);
                    }
                }
            }
            Fill(Stream_Video, 0, Video_Width, (int32u)width_minus_one+1);
            Fill(Stream_Video, 0, Video_Height, (int32u)height_minus_one+1);
        }
    FILLING_END()
    Frame_Count++;
    Finish();
}

} //NameSpace

#endif //MEDIAINFO_VP9_YES
