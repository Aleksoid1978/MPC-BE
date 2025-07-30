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
#if defined(MEDIAINFO_AV1_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Av1.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
extern const char* Mpegv_colour_primaries(int8u colour_primaries);
extern const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
extern const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
extern const char* Mpegv_matrix_coefficients_ColorSpace(int8u matrix_coefficients);
extern const char* Avc_video_full_range[];

//---------------------------------------------------------------------------
const char* Av1_obu_type(int8u obu_type)
{
    switch (obu_type)
    {
        case  0x1 : return "sequence_header";
        case  0x2 : return "temporal_delimiter";
        case  0x3 : return "frame_header";
        case  0x4 : return "tile_group";
        case  0x5 : return "metadata";
        case  0x6 : return "frame";
        case  0x7 : return "redundant_frame_header";
        case  0x8 : return "tile_list";
        case  0xF : return "padding";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
const char* Av1_seq_profile(int8u seq_profile)
{
    switch (seq_profile)
    {
        case  0x0 : return "Main";
        case  0x1 : return "High";
        case  0x2 : return "Professional";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
const char* Av1_metadata_type(int8u metadata_type)
{
    switch (metadata_type)
    {
    case  0x1: return "METADATA_TYPE_HDR_CLL";
    case  0x2: return "METADATA_TYPE_HDR_MDCV";
    case  0x3: return "METADATA_TYPE_SCALABILITY";
    case  0x4: return "METADATA_TYPE_ITUT_T35";
    case  0x5: return "METADATA_TYPE_TIMECODE";
    default: return "";
    }
}

//---------------------------------------------------------------------------
const char* Av1_frame_type[4] =
{
    "Key",
    "Inter",
    "Intra Only",
    "Switch",
};

//---------------------------------------------------------------------------
static const char* Av1_chroma_sample_position[3] =
{
    "Type 0",
    "Type 2",
    "3",
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Av1::File_Av1()
{
    //Config
    #if MEDIAINFO_EVENTS
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;

    //In
    Frame_Count_Valid=0;
    FrameIsAlwaysComplete=false;

    //Temp
    sequence_header_Parsed=false;
    SeenFrameHeader=false;
}

//---------------------------------------------------------------------------
File_Av1::~File_Av1()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av1::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "AV1");

    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "AV1");

    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?8:(IsSub?1:2);
}

//---------------------------------------------------------------------------
void File_Av1::Streams_Fill()
{
}

//---------------------------------------------------------------------------
void File_Av1::Streams_Finish()
{
    Fill(Stream_Video, 0, Video_Format_Settings_GOP, GOP_Detect(GOP));

    //Merge info about different HDR formats
    auto HDR_Format = HDR.find(Video_HDR_Format);
    if (HDR_Format != HDR.end())
    {
        std::bitset<HdrFormat_Max> HDR_Present;
        size_t HDR_FirstFormatPos = (size_t)-1;
        for (size_t i = 0; i < HdrFormat_Max; ++i)
            if (!HDR_Format->second[i].empty())
            {
                if (HDR_FirstFormatPos == (size_t)-1)
                    HDR_FirstFormatPos = i;
                HDR_Present[i] = true;
            }
        bool LegacyStreamDisplay = MediaInfoLib::Config.LegacyStreamDisplay_Get();
        for (const auto& HDR_Item : HDR)
        {
            size_t i = HDR_FirstFormatPos;
            size_t HDR_FirstFieldNonEmpty = (size_t)-1;
            if (HDR_Item.first > Video_HDR_Format_Compatibility)
            {
                for (; i < HdrFormat_Max; ++i)
                {
                    if (!HDR_Present[i])
                        continue;
                    if (HDR_FirstFieldNonEmpty == (size_t)-1 && !HDR_Item.second[i].empty())
                        HDR_FirstFieldNonEmpty = i;
                    if (!HDR_Item.second[i].empty() && HDR_FirstFieldNonEmpty < HdrFormat_Max && HDR_Item.second[i] != HDR_Item.second[HDR_FirstFieldNonEmpty])
                        break;
                }
            }
            if (i == HdrFormat_Max && HDR_FirstFieldNonEmpty != (size_t)-1)
                Fill(Stream_Video, 0, HDR_Item.first, HDR_Item.second[HDR_FirstFieldNonEmpty]);
            else
            {
                ZtringList Value;
                Value.Separator_Set(0, __T(" / "));
                if (i != HdrFormat_Max)
                    for (i = HDR_FirstFormatPos; i < HdrFormat_Max; ++i)
                    {
                        if (!LegacyStreamDisplay && HDR_FirstFormatPos != HdrFormat_SmpteSt2086 && i >= HdrFormat_SmpteSt2086)
                            break;
                        if (!HDR_Present[i])
                            continue;
                        Value.push_back(HDR_Item.second[i]);
                    }
                auto Value_Flat = Value.Read();
                if (!Value.empty() && Value_Flat.size() > (Value.size() - 1) * 3)
                    Fill(Stream_Video, 0, HDR_Item.first, Value.Read());
            }
        }
    }

    if (!maximum_content_light_level.empty())
        Fill(Stream_Video, 0, Video_MaxCLL, maximum_content_light_level);
    if (!maximum_frame_average_light_level.empty())
        Fill(Stream_Video, 0, Video_MaxFALL, maximum_frame_average_light_level);
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av1::Read_Buffer_OutOfBand()
{
    //Parsing
    bool initial_presentation_delay_present;
    BS_Begin();
    Mark_1 ();
    Skip_S1(7,                                                  "version");
    Skip_S1(3,                                                  "seq_profile");
    Skip_S1(5,                                                  "seq_level_idx_0");
    Skip_SB(                                                    "seq_tier_0");
    Skip_SB(                                                    "high_bitdepth");
    Skip_SB(                                                    "twelve_bit");
    Skip_SB(                                                    "monochrome");
    Skip_SB(                                                    "chroma_subsampling_x");
    Skip_SB(                                                    "chroma_subsampling_y");
    Skip_S1(2,                                                  "chroma_sample_position");
    Skip_S1(3,                                                  "reserved");
    Get_SB (   initial_presentation_delay_present,              "initial_presentation_delay_present");
    Skip_S1(4,                                                  initial_presentation_delay_present?"initial_presentation_delay_minus_one":"reserved");
    BS_End();

    Open_Buffer_Continue(Buffer, Buffer_Size);
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av1::Header_Parse()
{
    //Parsing
    int8u obu_type;
    bool obu_extension_flag;
    BS_Begin();
    Mark_0 ();
    Get_S1 ( 4, obu_type,                                       "obu_type");
    Get_SB (    obu_extension_flag,                             "obu_extension_flag");
    Skip_SB(                                                    "obu_has_size_field");
    Skip_SB(                                                    "obu_reserved_1bit");
    if (obu_extension_flag)
    {
        Skip_S1(3,                                              "temporal_id");
        Skip_S1(2,                                              "spatial_id");
        Skip_S1(3,                                              "extension_header_reserved_3bits");
    }
    BS_End();

    int64u obu_size;
    Get_leb128 (obu_size,                                       "obu_size");

    FILLING_BEGIN();
    Header_Fill_Size(Element_Offset+obu_size);
    FILLING_END();

    if (FrameIsAlwaysComplete && (Element_IsWaitingForMoreData() || Element_Offset+obu_size>Element_Size))
    {
        // Trashing the remaining bytes, as the the frame is always complete so remaining bytes should not be prepending the next frame
        Buffer_Offset=Buffer_Size;
        Element_Offset=0;
        return;
    }

    FILLING_BEGIN();
        Header_Fill_Code(obu_type, Av1_obu_type(obu_type));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::Data_Parse()
{
    //Probing mode in case of raw stream //TODO: better reject of bad files
    if (!IsSub && !Status[IsAccepted] && (!Element_Code || Element_Code>6))
    {
        Reject();
        return;
    }

    //Parsing
    switch (Element_Code)
    {
        case  0x1 : sequence_header(); break;
        case  0x2 : temporal_delimiter(); break;
        case  0x3 : frame_header(); break;
        case  0x4 : tile_group(); break;
        case  0x5 : metadata(); break;
        case  0x6 : frame(); break;
        case  0xF : padding(); break;
        default   : Skip_XX(Element_Size-Element_Offset,        "Data");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av1::sequence_header()
{
    //Parsing
    int32u max_frame_width_minus_1, max_frame_height_minus_1;
    int8u seq_profile, seq_level_idx[33]{}, operating_points_cnt_minus_1, buffer_delay_length_minus_1, frame_width_bits_minus_1, frame_height_bits_minus_1, seq_force_screen_content_tools, BitDepth, color_primaries, transfer_characteristics, matrix_coefficients, chroma_sample_position;
    bool reduced_still_picture_header, seq_tier[33], timing_info_present_flag, decoder_model_info_present_flag, seq_choose_screen_content_tools, mono_chrome, color_range, color_description_present_flag, subsampling_x, subsampling_y;
    BS_Begin();
    Get_S1 ( 3, seq_profile,                                    "seq_profile"); Param_Info1(Av1_seq_profile(seq_profile));
    Skip_SB(                                                    "still_picture");
    Get_SB (    reduced_still_picture_header,                   "reduced_still_picture_header");
    if (reduced_still_picture_header)
    {
        Get_S1 ( 5, seq_level_idx[0],                           "seq_level_idx[0]");
        decoder_model_info_present_flag=false;
        seq_tier[0]=false;
    }
    else
    {
        TEST_SB_GET(timing_info_present_flag,                   "timing_info_present_flag");
            bool equal_picture_interval;
            Skip_S4(32,                                         "num_units_in_tick");
            Skip_S4(32,                                         "time_scale");
            Get_SB (equal_picture_interval,                     "equal_picture_interval");
            if (equal_picture_interval)
                Skip_UE(                                        "num_ticks_per_picture_minus1");
            TEST_SB_GET (decoder_model_info_present_flag,       "decoder_model_info_present_flag");
                Get_S1 ( 5, buffer_delay_length_minus_1,        "buffer_delay_length_minus_1");
                Skip_S4(32,                                     "num_units_in_decoding_tick");
                Skip_S1( 5,                                     "buffer_removal_time_length_minus_1");
                Skip_S1( 5,                                     "frame_presentation_time_length_minus_1");
            TEST_SB_END();
        TEST_SB_END();
        Skip_SB(                                                "initial_display_delay_present_flag");
        Get_S1 ( 5, operating_points_cnt_minus_1,               "operating_points_cnt_minus_1");
        for (int8u i=0; i<=operating_points_cnt_minus_1; i++)
        {
            Element_Begin1("operating_point");
            Skip_S2(12,                                         "operating_point_idc[i]");
            Get_S1(5, seq_level_idx[i],                         "seq_level_idx[i]");
            if (seq_level_idx[i]>7)
                Get_SB(seq_tier[i],                             "seq_tier[i]");
            if (timing_info_present_flag && decoder_model_info_present_flag)
            {
                TEST_SB_SKIP(                                   "decoder_model_present_for_this_op[i]");
                    Skip_S5(buffer_delay_length_minus_1+1,      "decoder_buffer_delay[op]");
                    Skip_S5(buffer_delay_length_minus_1+1,      "encoder_buffer_delay[op]");
                    Skip_SB(                                    "low_delay_mode_flag[op]");
                TEST_SB_END();

            }
            Element_End0();
        }
    }
    Get_S1 ( 4, frame_width_bits_minus_1,                       "frame_width_bits_minus_1");
    Get_S1 ( 4, frame_height_bits_minus_1,                      "frame_height_bits_minus_1");
    Get_S4 (frame_width_bits_minus_1+1, max_frame_width_minus_1, "max_frame_width_minus_1");
    Get_S4 (frame_height_bits_minus_1+1, max_frame_height_minus_1, "max_frame_height_minus_1");
    if (!reduced_still_picture_header)
    {
        TEST_SB_SKIP(                                           "frame_id_numbers_present_flag");
            Skip_S1(4,                                          "delta_frame_id_length_minus2");
            Skip_S1(3,                                          "frame_id_length_minus1");
        TEST_SB_END();
    }
    Skip_SB(                                                    "use_128x128_superblock");
    Skip_SB(                                                    "enable_dual_filter");
    Skip_SB(                                                    "enable_intra_edge_filter");
    if (!reduced_still_picture_header)
    {
        bool enable_order_hint;
        Skip_SB(                                                "enable_interintra_compound");
        Skip_SB(                                                "enable_masked_compound");
        Skip_SB(                                                "enable_warped_motion");
        Skip_SB(                                                "enable_dual_filter");
        TEST_SB_GET (enable_order_hint,                         "enable_order_hint");
            Skip_SB(                                            "enable_jnt_comp");
            Skip_SB(                                            "enable_ref_frame_mvs");
        TEST_SB_END();
        Get_SB (seq_choose_screen_content_tools,                "seq_choose_screen_content_tools");
        if (seq_choose_screen_content_tools)
            seq_force_screen_content_tools=2;
        else
            Get_S1 (1, seq_force_screen_content_tools,          "seq_force_screen_content_tools");
        if (seq_force_screen_content_tools)
        {
            bool seq_choose_integer_mv;
            Get_SB(seq_choose_integer_mv,                       "seq_choose_integer_mv");
            if (!seq_choose_integer_mv)
                Skip_S1(1,                                      "seq_force_integer_mv");
        }
        if (enable_order_hint)
            Skip_S1(3,                                          "order_hint_bits_minus_1");
    }
    Skip_SB(                                                    "enable_superres");
    Skip_SB(                                                    "enable_cdef");
    Skip_SB(                                                    "enable_restoration");
    Element_Begin1("color_config");
        bool high_bitdepth;
        Get_SB (high_bitdepth,                                  "high_bitdepth");
        BitDepth=high_bitdepth?10:8;
        if (seq_profile>=2 && high_bitdepth)
        {
            bool twelve_bit;
            Get_SB (twelve_bit,                                 "twelve_bit");
            if (twelve_bit)
                BitDepth+=2;
        }
        if (seq_profile==1)
            mono_chrome=false;
        else
            Get_SB (mono_chrome,                                "mono_chrome");
        TESTELSE_SB_GET (color_description_present_flag,        "color_description_present_flag");
            Get_S1 (8, color_primaries,                         "color_primaries"); Param_Info1(Mpegv_colour_primaries(color_primaries));
            Get_S1 (8, transfer_characteristics,                "transfer_characteristics"); Param_Info1(Mpegv_transfer_characteristics(transfer_characteristics));
            Get_S1 (8, matrix_coefficients,                     "matrix_coefficients"); Param_Info1(Mpegv_matrix_coefficients(matrix_coefficients));
        TESTELSE_SB_ELSE(                                       "color_description_present_flag");
            color_primaries=2;
            transfer_characteristics=2;
            matrix_coefficients=2;
        TESTELSE_SB_END();
        if (mono_chrome)
        {
            color_range=true;
            subsampling_x=true;
            subsampling_y=true;
        }
        else if (color_primaries==1 && transfer_characteristics==13 && matrix_coefficients==0)
        {
            subsampling_x=false;
            subsampling_y=false;
        }
        else
        {
            Get_SB(color_range,                                 "color_range"); Param_Info1(Avc_video_full_range[color_range]);
            if (seq_profile==0)
            {
                subsampling_x=true;
                subsampling_y=true;
            }
            else if (seq_profile==1)
            {
                subsampling_x=false;
                subsampling_y=false;
            }
            else
            {
                if ( BitDepth == 12 )
                {
                    Get_SB(subsampling_x,                       "subsampling_x");
                    if (subsampling_x)
                        Get_SB(subsampling_y,                   "subsampling_y");
                    else
                        subsampling_y=false;
                }
                else
                {
                    subsampling_x=true;
                    subsampling_y=false;
                }
            } 
            if (subsampling_x && subsampling_y)
                Get_S1 ( 2, chroma_sample_position,             "chroma_sample_position");
        }
        Skip_SB(                                                "separate_uv_delta_q");
    Element_End0();
    Skip_SB(                                                    "film_grain_params_present");
    Mark_1();
    if (Data_BS_Remain()<8)
        while (Data_BS_Remain())
            Mark_0();
    BS_End();

    FILLING_BEGIN_PRECISE();
        if (!sequence_header_Parsed)
        {
            if (IsSub)
                Accept();
            Fill(Stream_Video, 0, Video_Format_Profile, Ztring().From_UTF8(Av1_seq_profile(seq_profile))+(seq_level_idx[0]==31?Ztring():(__T("@L")+Ztring().From_Number(2+(seq_level_idx[0]>>2))+__T(".")+Ztring().From_Number(seq_level_idx[0]&3))));
            Fill(Stream_Video, 0, Video_Width, max_frame_width_minus_1+1);
            Fill(Stream_Video, 0, Video_Height, max_frame_height_minus_1+1);
            Fill(Stream_Video, 0, Video_BitDepth, BitDepth);
            Fill(Stream_Video, 0, Video_ColorSpace, mono_chrome?"Y":((color_primaries==1 && transfer_characteristics==13 && matrix_coefficients==0)?"RGB":"YUV"));
            if (Retrieve(Stream_Video, 0, Video_ColorSpace)==__T("YUV"))
            {
                Fill(Stream_Video, 0, Video_ChromaSubsampling, subsampling_x?(subsampling_y?"4:2:0":"4:2:2"):"4:4:4"); // "!subsampling_x && subsampling_y" (4:4:0) not possible
                if (subsampling_x && subsampling_y && chroma_sample_position)
                    Fill(Stream_Video, 0, Video_ChromaSubsampling_Position, Av1_chroma_sample_position[chroma_sample_position-1]);
            }
            if (color_description_present_flag)
            {
                Fill(Stream_Video, 0, Video_colour_description_present, "Yes");
                Fill(Stream_Video, 0, Video_colour_primaries, Mpegv_colour_primaries(color_primaries));
                Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(transfer_characteristics));
                Fill(Stream_Video, 0, Video_matrix_coefficients, Mpegv_matrix_coefficients(matrix_coefficients));
            }
            if (mono_chrome ||  !(color_primaries==1 && transfer_characteristics==13 && matrix_coefficients==0))
                Fill(Stream_Video, 0, Video_colour_range, Avc_video_full_range[color_range]);

            sequence_header_Parsed=true;
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::temporal_delimiter()
{
    SeenFrameHeader=false;

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::frame_header()
{
    /* bitstream conformance requires SeenFrameHeader is only 1 when OBU type is OBU_REDUNDANT_FRAME_HEADER
     * since tile_group is not parsed, SeenFrameHeader cannot be relied upon as it is supposed to be reset by tile_group
    if (SeenFrameHeader)
    {
        Skip_XX(Element_Size,                                   "Duplicated data");
        return;
    }
    */
    SeenFrameHeader=1;

    if (!sequence_header_Parsed)
    {
        Skip_XX(Element_Size,                                   "Data");
        return;
    }

    //Parsing
    BS_Begin();
    Element_Begin1("uncompressed_header");
        int8u frame_type;
        TEST_SB_SKIP(                                           "show_existing_frame");
            BS_End();
            SeenFrameHeader = 0;
            Skip_XX(Element_Size-Element_Offset,                "Data");
            return;
        TEST_SB_END();
        Get_S1 (2, frame_type,                                  "frame_type"); Param_Info1(Av1_frame_type[frame_type]);

        FILLING_BEGIN();
            GOP.push_back((frame_type&1)?'P':'I');
        FILLING_ELSE();
            GOP.push_back(' ');
        FILLING_END();
        if (GOP.size()>=512)
            GOP.resize(384);
    Element_End0();
    BS_End();

    FILLING_BEGIN();
        if (!Status[IsAccepted])
            Accept();
        Frame_Count++;
        if (Frame_Count>=Frame_Count_Valid)
            Finish();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::tile_group()
{
    Skip_XX(Element_Size - Element_Offset,                      "Data");
}

//---------------------------------------------------------------------------
void File_Av1::metadata()
{
    //Parsing
    int64u metadata_type;
    Get_leb128 (metadata_type,                                  "metadata_type");
    Param_Info1(Av1_metadata_type(metadata_type));

    switch (metadata_type)
    {
        case    1 : metadata_hdr_cll(); break;
        case    2 : metadata_hdr_mdcv(); break;
        case    4 : metadata_itu_t_t35(); break;
        default   : Skip_XX(Element_Size-Element_Offset,        "Data");
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_hdr_cll()
{
    //Parsing
    Get_LightLevel(maximum_content_light_level, maximum_frame_average_light_level);
}

//---------------------------------------------------------------------------
void File_Av1::metadata_hdr_mdcv()
{
    //Parsing
    auto& HDR_Format = HDR[Video_HDR_Format][HdrFormat_SmpteSt2086];
    if (HDR_Format.empty())
    {
        HDR_Format = __T("SMPTE ST 2086");
        HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt2086] = "HDR10";
    }
    Get_MasteringDisplayColorVolume(HDR[Video_MasteringDisplay_ColorPrimaries][HdrFormat_SmpteSt2086], HDR[Video_MasteringDisplay_Luminance][HdrFormat_SmpteSt2086], true);
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35()
{
    //Parsing
    int8u itu_t_t35_country_code;
    Get_B1(itu_t_t35_country_code,                              "itu_t_t35_country_code");
    if (itu_t_t35_country_code == 0xFF)
        Skip_B1(                                                "itu_t_t35_country_code_extension_byte");

    switch (itu_t_t35_country_code)
    {
    case 0xB5: metadata_itu_t_t35_B5(); break; // USA
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5()
{
    int16u itu_t_t35_terminal_provider_code;
    Get_B2(itu_t_t35_terminal_provider_code,                    "itu_t_t35_terminal_provider_code");

    switch (itu_t_t35_terminal_provider_code)
    {
    case 0x003C: metadata_itu_t_t35_B5_003C(); break;
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5_003C()
{
    int16u itu_t_t35_terminal_provider_oriented_code;
    Get_B2(itu_t_t35_terminal_provider_oriented_code,           "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
    case 0x0001: metadata_itu_t_t35_B5_003C_0001(); break;
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5_003C_0001()
{
    int8u application_identifier;
    Get_B1(application_identifier,                              "application_identifier");

    switch (application_identifier)
    {
    case 0x04: metadata_itu_t_t35_B5_003C_0001_04(); break;
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5_003C_0001_04()
{
    Element_Info1("SMPTE ST 2094 App 4");

    int8u application_version{};
    bool IsHDRplus{ false }, tone_mapping_flag{};
    Get_SMPTE_ST_2094_40(application_version, IsHDRplus, tone_mapping_flag);

    FILLING_BEGIN();
        auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_SmpteSt209440];
        if (HDR_Format.empty())
        {
            HDR_Format=__T("SMPTE ST 2094 App 4");
            HDR[Video_HDR_Format_Version][HdrFormat_SmpteSt209440].From_Number(application_version);
            if (IsHDRplus)
                HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt209440]=tone_mapping_flag?__T("HDR10+ Profile B"):__T("HDR10+ Profile A");
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::frame()
{
    //Parsing
    Element_Begin1("frame_header");
    frame_header();
    Element_End0();

    Element_Begin1("tile_group");
    tile_group();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av1::padding()
{
    Skip_XX(Element_Size,                                       "Padding");
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
//TODO generic code
std::string File_Av1::GOP_Detect (std::string PictureTypes)
{
    //Finding a string without blanks
    size_t PictureTypes_Limit=PictureTypes.find(' ');
    if (PictureTypes_Limit!=string::npos)
    {
        if (PictureTypes_Limit>PictureTypes.size()/2)
            PictureTypes.resize(PictureTypes_Limit);
        else
        {
            //Trim
            size_t TrimPos;
            TrimPos=PictureTypes.find_first_not_of(' ');
            if (TrimPos!=string::npos)
                PictureTypes.erase(0, TrimPos);
            TrimPos=PictureTypes.find_last_not_of(' ');
            if (TrimPos!=string::npos)
                PictureTypes.erase(TrimPos+1);

            //Finding the longest string
            ZtringList List; List.Separator_Set(0, __T(" "));
            List.Write(Ztring().From_UTF8(PictureTypes));
            size_t MaxLength=0;
            size_t MaxLength_Pos=0;
            for (size_t Pos=0; Pos<List.size(); Pos++)
                if (List[Pos].size()>MaxLength)
                {
                    MaxLength=List[Pos].size();
                    MaxLength_Pos=Pos;
                }
            PictureTypes=List[MaxLength_Pos].To_Local();

        }
    }

    //Creating all GOP values
    std::vector<Ztring> GOPs;
    size_t GOP_Frame_Count=0;
    size_t GOP_BFrames_Max=0;
    size_t I_Pos1=PictureTypes.find('I');
    while (I_Pos1!=std::string::npos)
    {
        size_t I_Pos2=PictureTypes.find('I', I_Pos1+1);
        if (I_Pos2!=std::string::npos)
        {
            std::vector<size_t> P_Positions;
            size_t P_Position=I_Pos1;
            do
            {
                P_Position=PictureTypes.find('P', P_Position+1);
                if (P_Position<I_Pos2)
                    P_Positions.push_back(P_Position);
            }
            while (P_Position<I_Pos2);
            if (P_Positions.size()>1 && P_Positions[0]>I_Pos1+1 && P_Positions[P_Positions.size()-1]==I_Pos2-1)
                P_Positions.resize(P_Positions.size()-1); //Removing last P-Frame for next test, this is often a terminating P-Frame replacing a B-Frame
            Ztring GOP;
            bool IsOK=true;
            if (!P_Positions.empty())
            {
                size_t Delta=P_Positions[0]-I_Pos1;
                for (size_t Pos=1; Pos<P_Positions.size(); Pos++)
                    if (P_Positions[Pos]-P_Positions[Pos-1]!=Delta)
                    {
                        IsOK=false;
                        break;
                    }
                if (IsOK)
                {
                    GOP+=__T("M=")+Ztring::ToZtring(P_Positions[0]-I_Pos1)+__T(", ");
                    if (P_Positions[0]-I_Pos1>GOP_BFrames_Max)
                        GOP_BFrames_Max=P_Positions[0]-I_Pos1;
                }
            }
            if (IsOK)
            {
                GOP+=__T("N=")+Ztring::ToZtring(I_Pos2-I_Pos1);
                GOPs.push_back(GOP);
            }
            else
                GOPs.push_back(Ztring()); //There is a problem, blank
            GOP_Frame_Count+=I_Pos2-I_Pos1;
        }
        I_Pos1=I_Pos2;
    }

    //Some clean up
    if (GOP_Frame_Count+GOP_BFrames_Max>Frame_Count && !GOPs.empty())
        GOPs.resize(GOPs.size()-1); //Removing the last one, there may have uncomplete B-frame filling
    if (GOPs.size()>4)
        GOPs.erase(GOPs.begin()); //Removing the first one, it is sometime different and we have enough to deal with

    //Filling
    if (GOPs.size()>=4)
    {
        bool IsOK=true;
        for (size_t Pos=1; Pos<GOPs.size(); Pos++)
            if (GOPs[Pos]!=GOPs[0])
            {
                IsOK=false;
                break;
            }
        if (IsOK)
            return GOPs[0].To_Local();
    }

    return string();
}

void File_Av1::Get_leb128(int64u& Info, const char* Name)
{
    Info=0;
    for (int8u i=0; i<8; i++)
    {
        if (Element_Offset>=Element_Size)
            break; // End of stream reached, not normal
        int8u leb128_byte=BigEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset);
        Element_Offset++;
        Info|=(static_cast<int64u>(leb128_byte&0x7f)<<(i*7));
        if (!(leb128_byte&0x80))
        {
            #if MEDIAINFO_TRACE
                if (Trace_Activated)
                {
                    Param(Name, Info, i+1);
                    Param_Info(__T("(")+Ztring::ToZtring(i+1)+__T(" bytes)"));
                }
            #endif //MEDIAINFO_TRACE
            return;
        }
    }
    Trusted_IsNot("Size is wrong");
    Info=0;
}

//---------------------------------------------------------------------------
} //NameSpace

#endif //MEDIAINFO_AV1_YES
