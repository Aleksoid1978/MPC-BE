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
#if defined(MEDIAINFO_AV2_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Av2.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if defined(MEDIAINFO_ICC_YES)
    #include "MediaInfo/Tag/File_Icc.h"
#endif
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

#define GLOBAL_XLAYER_ID 31

#define SIMPLE     0
#define INTERINTRA 1
#define LOCALWARP  2
#define DELTAWARP  3
#define EXTENDWARP 4

#define MOTION_MODES 5

#define SELECT_SCREEN_CONTENT_TOOLS 2

#define SEG_LVL_MAX 3

#define MAXQ_8_BITS 255
#define MAXQ_OFFSET 24
#define MAXQ_10_BITS MAXQ_8_BITS + 2 * MAXQ_OFFSET
#define MAXQ_12_BITS MAXQ_8_BITS + 4 * MAXQ_OFFSET

#define BLOCK_64X64   12
#define BLOCK_128X128 15
#define BLOCK_256X256 18

#define BLOCK_SIZES 31

#define MAX_TILE_WIDTH 4096
#define MAX_TILE_AREA  4096 * 2304
#define MAX_TILE_ROWS  64
#define MAX_TILE_COLS  64

//---------------------------------------------------------------------------
static const char* Av2_obu_type(int8u obu_type)
{
    switch (obu_type)
    {
    case  1 : return "OBU_SEQUENCE_HEADER";
    case  2 : return "OBU_TEMPORAL_DELIMITER";
    case  3 : return "OBU_MULTI_FRAME_HEADER";
    case  4 : return "OBU_CLOSED_LOOP_KEY";
    case  5 : return "OBU_OPEN_LOOP_KEY";
    case  6 : return "OBU_LEADING_TILE_GROUP";
    case  7 : return "OBU_REGULAR_TILE_GROUP";
    case  8 : return "OBU_METADATA_SHORT";
    case  9 : return "OBU_METADATA_GROUP";
    case 10 : return "OBU_SWITCH";
    case 11 : return "OBU_LEADING_SEF";
    case 12 : return "OBU_REGULAR_SEF";
    case 13 : return "OBU_LEADING_TIP";
    case 14 : return "OBU_REGULAR_TIP";
    case 15 : return "OBU_BUFFER_REMOVAL_TIMING";
    case 16 : return "OBU_LAYER_CONFIGURATION_RECORD";
    case 17 : return "OBU_ATLAS_SEGMENT";
    case 18 : return "OBU_OPERATING_POINT_SET";
    case 19 : return "OBU_BRIDGE_FRAME";
    case 20 : return "OBU_MSDO";
    case 21 : return "OBU_RAS_FRAME";
    case 22 : return "OBU_QUANTIZATION_MATRIX";
    case 23 : return "OBU_FILM_GRAIN";
    case 24 : return "OBU_CONTENT_INTERPRETATION";
    case 25 : return "OBU_PADDING";
    default : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Av2_scan_type_idc(int8u scan_type_idc)
{
    switch (scan_type_idc)
    {
    case 0: return "Unspecified";
    case 1: return "Progressive frame";
    case 2: return "Interlace field";
    case 3: return "Interlace complementary field-pair";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Av2_color_description_idc(int8u color_description_idc)
{
    switch (color_description_idc)
    {
    case 0: return "Explicitly signaled";
    case 1: return "BT.709 SDR";
    case 2: return "BT.2100 PQ";
    case 3: return "BT.2100 HLG";
    case 4: return "sRGB";
    case 5: return "sYCC";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Av2_metadata_type(int8u metadata_type)
{
    switch (metadata_type)
    {
    case 1: return "METADATA_TYPE_HDR_CLL";
    case 2: return "METADATA_TYPE_HDR_MDCV";
    case 3: return "METADATA_TYPE_ITUT_T35";
    case 4: return "METADATA_TYPE_TIMECODE";
    case 6: return "METADATA_TYPE_BANDING_HINTS";
    case 7: return "METADATA_TYPE_ICC_PROFILE";
    case 8: return "METADATA_TYPE_SCAN_TYPE";
    case 9: return "METADATA_TYPE_TEMPORAL_POINT_INFO";
    default: return "";
    }
}

//***************************************************************************
// Math utils
//***************************************************************************

//---------------------------------------------------------------------------
static int FloorLog2(unsigned int n) {
    if (n == 0) return -1;
    int res = -1;
    while (n > 0) {
        n >>= 1;
        res++;
    }
    return res;
}

//---------------------------------------------------------------------------
static int CeilLog2(unsigned int n) {
    if (n <= 1) return 0;
    return FloorLog2(n - 1) + 1;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Av2::File_Av2()
{
    StreamSource = IsStream;
    FrameIsAlwaysComplete = false;
}

//---------------------------------------------------------------------------
File_Av2::~File_Av2()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av2::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "AV2");

    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "AV2");
}

//---------------------------------------------------------------------------
void File_Av2::Streams_Fill()
{
    string ColorSpace = chroma_format_idc == 1 ? "Y" : (ColorPrimaries == 1 && TransferCharacteristics == 13 && MatrixCoefficients == 0) ? "RGB" : "YUV";
    Fill(Stream_Video, 0, Video_ColorSpace, ColorSpace);
    if (ColorSpace == "YUV")
    {
        Fill(Stream_Video, 0, Video_ChromaSubsampling, chroma_format_idc == 0 ? "4:2:0" : chroma_format_idc == 2 ? "4:4:4" : chroma_format_idc == 3 ? "4:2:2" : "");
    }
    if (ColorDescriptionPresent) {
        Fill(Stream_Video, 0, Video_colour_description_present, "Yes");
        Fill(Stream_Video, 0, Video_colour_primaries, Mpegv_colour_primaries(ColorPrimaries));
        Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(TransferCharacteristics));
        Fill(Stream_Video, 0, Video_matrix_coefficients, Mpegv_matrix_coefficients(MatrixCoefficients));
        Fill(Stream_Video, 0, Video_colour_range, Avc_video_full_range[FullRange]);
    }
}

//---------------------------------------------------------------------------
void File_Av2::Streams_Finish()
{
    
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av2::Read_Buffer_OutOfBand()
{
    //Parsing
    bool initial_presentation_delay_present;
    BS_Begin();
    Mark_1 ();
    Skip_S1(7,                                                  "version");
    Skip_S1(3,                                                  "seq_profile");
    Skip_S1(5,                                                  "seq_level_idx_0");
    Skip_SB(                                                    "seq_tier_0");
    Skip_S1(2,                                                  "bitdepth_idx");
    Skip_SB(                                                    "monochrome");
    Skip_SB(                                                    "chroma_subsampling_x");
    Skip_SB(                                                    "chroma_subsampling_y");
    Skip_S1(3,                                                  "chroma_sample_position");
    Skip_S1(2,                                                  "reserved");
    Get_SB (   initial_presentation_delay_present,              "initial_presentation_delay_present");
    Skip_S1(4,                                                  initial_presentation_delay_present?"initial_presentation_delay_minus_one":"reserved");
    BS_End();

    Open_Buffer_Continue(Buffer, Buffer_Size);
}

//---------------------------------------------------------------------------
void File_Av2::Read_Buffer_Init()
{
    if (!Frame_Count_Valid)
        Frame_Count_Valid = IsSub ? 1 : 2;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av2::Header_Parse()
{
    //Parsing
    int64u obu_size;
    if (IsAnnexB) {
        Element_DoNotShow();
        Element_End0();
        Get_leb128(obu_size,                                    "num_bytes_in_obu");
        Element_Begin0();
        obu_size += Element_Offset;
    }
    else {
        obu_size = Element_Size;
    }

    Element_Name("obu_header");
    bool obu_header_extension_flag;
    int8u obu_type;
    BS_Begin();
    Get_SB  (   obu_header_extension_flag,                      "obu_header_extension_flag");
    Get_S1  (5, obu_type,                                       "obu_type");
    Skip_S1 (2,                                                 "obu_tlayer_id");
    if (obu_header_extension_flag) {
        Get_S1  (3, obu_mlayer_id,                              "obu_mlayer_id");
        Get_S1  (5, obu_xlayer_id,                              "obu_xlayer_id");
    }
    else {
        obu_mlayer_id = 0;
        obu_xlayer_id = (obu_type == 20) ? GLOBAL_XLAYER_ID : 0;
    }
    BS_End();

    FILLING_BEGIN();
    Header_Fill_Size(obu_size);
    Header_Fill_Code(obu_type, Av2_obu_type(obu_type));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av2::Data_Parse()
{
    //Probing mode in case of raw stream //TODO: better reject of bad files
    if (!IsSub && !Status[IsAccepted]
        && !sequence_header_Parsed && Element_Code != 1)
    {
        Reject();
        return;
    }

    //Parsing
    switch (Element_Code)
    {
    case  1: sequence_header_obu(); break;
    case  2: temporal_delimiter_obu(); break;
    case 20: multi_stream_decoder_operation_obu(); break;
    case  3: multi_frame_header_obu(); break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 19: BS_Begin(); frame_header(true); BS_End(); break;
    case  8: metadata_short_obu(); break;
    case  9: metadata_group_obu(); break;
    case  6:
    case  7:
    case  4:
    case  5:
    case 10:
    case 21: tile_group_obu(Element_Size); break;
    case 16: layer_config_record_obu(); break;
    case 17: atlas_segment_info_obu(); break;
    case 18: operating_point_set_obu(); break;
    case 15: buffer_removal_timing_obu(); break;
    case 22: quantizer_matrix_obu(); break;
    case 23: film_grain_obu(); break;
    case 24: content_interpretation_obu(); break;
    case 25: padding_obu(); break;
    default: reserved_obu(); break;
    }

    if (!Status[IsAccepted] && Frame_Count && !IsSub && File_Offset + Buffer_Offset + Element_TotalSize_Get() >= File_Size) {
        Accept();
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av2::obu_extension_data(int64u sz)
{
    Element_Begin1("obu_extension_data");
    Skip_BS(sz,                                                 "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::trailing_bits(int64u nbBits)
{
    Element_Begin1("trailing_bits");
    Mark_1(); // trailing_one_bit
    if (nbBits < 1) return; // prevent underflow
    --nbBits;
    while (nbBits > 0) {
        Mark_0(); // trailing_zero_bit
        --nbBits;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::byte_alignment()
{
    Element_Begin1("byte_alignment");
    while (BS->Remain() & 7) {
        Mark_0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::reserved_obu()
{
    Element_Begin1("reserved_obu");
    Skip_XX(Element_Size,                                       "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::sequence_header_obu()
{
    //Parsing
    BS_Begin();

    Element_Begin1("sequence_header_obu");
    Skip_uvlc(                                                  "seq_header_id");
    Skip_S1(5,                                                  "seq_profile_idc");
    TESTELSE_SB_GET(single_picture_header_flag,                 "single_picture_header_flag");
        // seq_lcr_id = 0 	
        // still_picture = 1
    TESTELSE_SB_ELSE(                                           "single_picture_header_flag");
        Skip_S1(3,                                              "seq_lcr_id");
        Skip_SB(                                                "still_picture");
    TESTELSE_SB_END();
    int8u seq_level_idx;
    Get_S1(5, seq_level_idx,                                    "seq_level_idx");
    if (seq_level_idx > 7 && !single_picture_header_flag) {
        Skip_SB(                                                "seq_tier");
    }
    else {
        // seq_tier = 0
    }
    int8u frame_width_bits_minus_1, frame_height_bits_minus_1;
    Get_S1(4, frame_width_bits_minus_1,                         "frame_width_bits_minus_1");
    Get_S1(4, frame_height_bits_minus_1,                        "frame_height_bits_minus_1 ");
    int32u max_frame_width_minus_1, max_frame_height_minus_1;
    Get_S4(frame_width_bits_minus_1 + 1, max_frame_width_minus_1, "max_frame_width_minus_1");
    Get_S4(frame_height_bits_minus_1 + 1, max_frame_height_minus_1, "max_frame_height_minus_1");
    TESTELSE_SB_SKIP(                                           "seq_cropping_window_present_flag");
        Skip_uvlc(                                              "seq_cropping_win_left_offset");
        Skip_uvlc(                                              "seq_cropping_win_right_offset");
        Skip_uvlc(                                              "seq_cropping_win_top_offset");
        Skip_uvlc(                                              "seq_cropping_win_bottom_offset");
    TESTELSE_SB_ELSE(                                           "seq_cropping_window_present_flag");
        // seq_cropping_win_left_offset = 0
        // seq_cropping_win_right_offset = 0
        // seq_cropping_win_top_offset = 0
        // seq_cropping_win_bottom_offset = 0
    TESTELSE_SB_END();
    Element_Begin1("color_config");
        int32u bit_depth_idc;
        Get_uvlc(chroma_format_idc,                             "chroma_format_idc"); Param_Info1(chroma_format_idc == 0 ? "4:2:0" : chroma_format_idc == 2 ? "4:4:4" : chroma_format_idc == 3 ? "4:2:2" : "");
        Get_uvlc(bit_depth_idc,                                 "bit_depth_idc"); Param_Info2(bit_depth_idc == 0 ? 10 : (bit_depth_idc == 1 ? 8 : 12), " bits");
        Monochrome = (chroma_format_idc == 1);
    Element_End0();
    int8u max_tlayer_id, max_mlayer_id;
    if (single_picture_header_flag) {
        // decoder_model_info_present_flag = 0
        max_tlayer_id = 0;
        max_mlayer_id = 0;
        // seq_max_mlayer_cnt = 1
    }
    else {
        TESTELSE_SB_SKIP(                                       "max_display_model_info_present_flag");
            Skip_S1(4,                                          "max_initial_display_delay_minus_1");
        TESTELSE_SB_ELSE(                                       "max_display_model_info_present_flag");
            // max_initial_display_delay_minus_1 = BUFFER_POOL_MAX_SIZE - 1
        TESTELSE_SB_END();
        TEST_SB_SKIP(                                           "decoder_model_info_present_flag");
            Element_Begin1("decoder_model_info");
                Skip_S4(32,                                     "num_units_in_decoding_tick");
            Element_End0();
            TEST_SB_SKIP(                                       "max_display_model_info_present_flag");
                Element_Begin1("operating_parameters_info");
                Skip_uvlc(                                      "decoder_buffer_delay");
                Skip_uvlc(                                      "encoder_buffer_delay");
                Skip_SB(                                        "low_delay_mode_flag");
                Element_End0();
            TEST_SB_END();
        TEST_SB_END();
        Get_S1(2, max_tlayer_id,                                "max_tlayer_id");
        Get_S1(3, max_mlayer_id,                                "max_mlayer_id");
        if (max_mlayer_id > 0) {
            Skip_S4(CeilLog2(max_mlayer_id + 1),                "seq_max_mlayer_cnt");
        }
    }
    if (max_tlayer_id > 0) {
        TEST_SB_SKIP(                                           "tlayer_dependency_present_flag");
            for (int currLayer = 1; currLayer <= max_tlayer_id; ++currLayer) {
                for (int refLayer = currLayer; refLayer >= 0; --refLayer) {
                    Skip_SB(                                    "tlayer_dependency_map");
                }
            }
        TEST_SB_END();
    }
    if (max_mlayer_id > 0) {
        TEST_SB_SKIP(                                           "mlayer_dependency_present_flag ");
            for (int currLayer = 1; currLayer <= max_mlayer_id; ++currLayer) {
                for (int refLayer = currLayer; refLayer >= 0; --refLayer) {
                    Skip_SB(                                    "mlayer_dependency_map");
                }
            }
        TEST_SB_END();
    }
    sequence_partition_config();
    sequence_segment_config();
    sequence_intra_config();
    sequence_inter_config();
    sequence_scc_config();
    sequence_transform_quant_entropy_config();
    sequence_filter_config();
    TEST_SB_SKIP(                                               "seq_tile_info_present_flag ");
        Skip_SB(                                                "allow_tile_info_change");
        auto seqSbSize = get_seq_sb_size();
        tile_params(max_frame_width_minus_1 + 1, max_frame_height_minus_1 + 1, seqSbSize, seqSbSize, false);
    TEST_SB_END();
    bool film_grain_params_present;
    Get_SB(film_grain_params_present,                           "film_grain_params_present");
    Element_End0();

    obu_end();
    BS_End();

    //Filling
    FILLING_BEGIN_PRECISE();
    if (!sequence_header_Parsed)
    {
        Fill(Stream_Video, 0, Video_Width, max_frame_width_minus_1 + 1);
        Fill(Stream_Video, 0, Video_Height, max_frame_height_minus_1 + 1);
        Fill(Stream_Video, 0, Video_BitDepth, bit_depth_idc == 0 ? 10 : (bit_depth_idc == 1 ? 8 : 12));
        if (film_grain_params_present)
            Fill(Stream_Video, 0, Video_Format_Settings, "Film Grain Synthesis");
        if (max_mlayer_id > 0 || max_tlayer_id > 0)
            Fill(Stream_Video, 0, Video_Format_Settings, "Scalable Video Coding");

        sequence_header_Parsed = true;
    }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av2::sequence_partition_config() 
{
    Element_Begin1("sequence_partition_config");
    TESTELSE_SB_GET(use_256x256_superblock,                     "use_256x256_superblock");
    TESTELSE_SB_ELSE(                                           "use_256x256_superblock");
        Get_SB(use_128x128_superblock,                          "use_128x128_superblock");
    TESTELSE_SB_END();
    bool enable_sdp;
    if (Monochrome) {
        enable_sdp = 0;
    }
    else {
        Get_SB(enable_sdp,                                      "enable_sdp");
    }
    if (enable_sdp && !single_picture_header_flag) {
        Skip_SB(                                                "enable_extended_sdp");
    }
    else {
        // enable_extended_sdp = 0
    }
    TESTELSE_SB_SKIP(                                           "enable_ext_partitions");
        Skip_SB(                                                "enable_uneven_4way_partitions");
    TESTELSE_SB_ELSE(                                           "enable_ext_partitions");
        // enable_uneven_4way_partitions = 0
    TESTELSE_SB_END();
    TESTELSE_SB_SKIP(                                           "reduce_pb_aspect_ratio");
        Skip_SB(                                                "max_pb_aspect_ratio_log2_minus1");
        // MaxPbAspectRatio = 1 << (max_pb_aspect_ratio_log2_minus1 + 1)
    TESTELSE_SB_ELSE(                                           "reduce_pb_aspect_ratio");
        // MaxPbAspectRatio = 8
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::sequence_segment_config() 
{
    Element_Begin1("sequence_segment_config");
    bool enable_ext_seg;
    Get_SB(enable_ext_seg,                                      "enable_ext_seg");
    int8u MaxSegments = enable_ext_seg ? 16 : 8;
    TEST_SB_SKIP(                                               "seq_seg_info_present_flag");
        Skip_SB(                                                "seq_allow_seg_info_change");
        seg_info(MaxSegments);
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::sequence_intra_config() 
{
    Element_Begin1("sequence_intra_config");
    Skip_SB(                                                    "enable_dip");
    Skip_SB(                                                    "enable_intra_edge_filter");
    Skip_SB(                                                    "enable_mrls");
    Skip_SB(                                                    "enable_cfl_intra");
    if (Monochrome) {
        // cfl_ds_filter_index = 0
    }
    else {
        Skip_S1(2,                                              "cfl_ds_filter_index");
    }
    Skip_SB(                                                    "enable_mhccp");
    Skip_SB(                                                    "enable_ibp");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::sequence_inter_config() 
{
    Element_Begin1("sequence_inter_config");
    if (single_picture_header_flag) {
        Skip_SB(                                                "enable_refmvbank");
        TESTELSE_SB_SKIP(                                       "disable_drl_reorder");
        TESTELSE_SB_ELSE(                                       "disable_drl_reorder");
            Skip_SB(                                            "constrain_drl_reorder");
        TESTELSE_SB_END();
        Skip_ns(4 - 1,                                          "seq_max_bvp_drl_bits_minus1");
        Skip_SB(                                                "allow_frame_max_bvp_drl_bits");
        Skip_SB(                                                "enable_bawp");
    }
    else {
        bool motionModeEnabled = false;
        bool seq_enabled_motion_modes[MOTION_MODES]{};
        for (int mode = INTERINTRA; mode < MOTION_MODES; ++mode) {
            Get_SB(seq_enabled_motion_modes[mode],              "seq_enabled_motion_modes[ mode ]");
            motionModeEnabled |= seq_enabled_motion_modes[mode];
        }
        if (motionModeEnabled)
            Skip_SB(                                            "seq_frame_motion_modes_present_flag");
        if (seq_enabled_motion_modes[DELTAWARP])
            Skip_SB(                                            "enable_six_param_warp_delta");
        Skip_SB(                                                "enable_masked_compound");
        TEST_SB_SKIP(                                           "enable_ref_frame_mvs");
        Skip_SB(                                                "reduced_ref_frame_mvs_mode");
        TEST_SB_END();
        Skip_S1(3,                                              "order_hint_bits_minus_1");
        Skip_SB(                                                "enable_refmvbank");
        TESTELSE_SB_SKIP(                                       "disable_drl_reorder");
        TESTELSE_SB_ELSE(                                       "disable_drl_reorder");
            Skip_SB(                                            "constrain_drl_reorder");
        TESTELSE_SB_END();
        Skip_SB(                                                "explicit_ref_frame_map");
        TEST_SB_SKIP(                                           "use_extra_ref_frames");
            Skip_S1(4,                                          "num_ref_frames_minus_1");
        TEST_SB_END();
        Skip_S1(3,                                              "long_term_frame_id_bits");
        Skip_ns(6 - 1,                                          "seq_max_drl_bits_minus1");
        Skip_SB(                                                "allow_frame_max_drl_bits");
        Skip_ns(4 - 1,                                          "seq_max_bvp_drl_bits_minus1");
        Skip_SB(                                                "allow_frame_max_bvp_drl_bits");
        Skip_S1(2,                                              "num_same_ref_compound");
        bool enable_tip, EnableTipOutput;
        TESTELSE_SB_GET(enable_tip,                             "enable_tip");
            bool disable_tip_output;
            Get_SB(disable_tip_output,                          "disable_tip_output");
            EnableTipOutput = !disable_tip_output;
            Skip_SB(                                            "enable_tip_hole_fill");
        TESTELSE_SB_ELSE(                                       "enable_tip");
            EnableTipOutput = false;
        TESTELSE_SB_END();
        Skip_SB(                                                "enable_mv_traj");
        Skip_SB(                                                "enable_bawp");
        Skip_SB(                                                "enable_cwp");
        Skip_SB(                                                "enable_imp_msk_bld");
        bool enable_lf_sub_pu;
        Get_SB(enable_lf_sub_pu,                                "enable_lf_sub_pu");
        if (EnableTipOutput && enable_lf_sub_pu)
            Skip_SB(                                            "enable_tip_explicit_qp");
        int8u enable_opfl_refine;
        Get_S1(2, enable_opfl_refine,                           "enable_opfl_refine");
        bool enable_refinemv;
        Get_SB(enable_refinemv,                                 "enable_refinemv");
        if (enable_tip && (enable_opfl_refine != 0 || enable_refinemv))
            Skip_SB(                                            "enable_tip_refinemv");
        Skip_SB(                                                "enable_bru");
        Skip_SB(                                                "enable_adaptive_mvd");
        Skip_SB(                                                "enable_mvd_sign_derive");
        Skip_SB(                                                "enable_flex_mvres");
        if (single_picture_header_flag) {
        }
        else {
            Skip_SB(                                            "enable_global_motion");
        }
        Skip_SB(                                                "enable_short_refresh_frame_flags");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::sequence_scc_config() 
{
    Element_Begin1("sequence_scc_config");
    int8u seq_force_screen_content_tools;
    if (single_picture_header_flag) {
        seq_force_screen_content_tools = SELECT_SCREEN_CONTENT_TOOLS;
    }
    else {
        TESTELSE_SB_SKIP(                                       "seq_choose_screen_content_tools");
            seq_force_screen_content_tools = SELECT_SCREEN_CONTENT_TOOLS;
        TESTELSE_SB_ELSE(                                       "seq_choose_screen_content_tools");
            Get_S1(1, seq_force_screen_content_tools,           "seq_force_screen_content_tools");
        TESTELSE_SB_END();
        if (seq_force_screen_content_tools > 0) {
            TESTELSE_SB_SKIP(                                   "seq_choose_integer_mv");
            TESTELSE_SB_ELSE(                                   "seq_choose_integer_mv");
                Skip_S1(1,                                      "seq_force_integer_mv");
            TESTELSE_SB_END();
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::sequence_transform_quant_entropy_config() 
{
    Element_Begin1("sequence_transform_quant_entropy_config");
    TESTELSE_SB_SKIP(                                           "enable_fsc");
    TESTELSE_SB_ELSE(                                           "enable_fsc");
        Skip_SB(                                                "enable_idtx_intra");
    TESTELSE_SB_END();
    Skip_SB(                                                    "enable_intra_ist");
    Skip_SB(                                                    "enable_inter_ist");
    if (Monochrome) {
    }
    else {
        Skip_SB(                                                "enable_chroma_dctonly");
    }
    if (!single_picture_header_flag)
        Skip_SB(                                                "enable_inter_ddt");
    Skip_SB(                                                    "reduced_tx_part_set");
    if (Monochrome) {
    }
    else {
        Skip_SB(                                                "enable_cctx");
    }
    bool enable_tcq, choose_tcq_per_frame;
    Get_SB(enable_tcq,                                          "enable_tcq");
    if (enable_tcq && !single_picture_header_flag)
        Get_SB(choose_tcq_per_frame,                            "choose_tcq_per_frame");
    else
        choose_tcq_per_frame = false;
    if (enable_tcq && !choose_tcq_per_frame) {
    }
    else {
        Skip_SB(                                                "enable_parity_hiding");
    }
    if (single_picture_header_flag) {
    }
    else {
        TEST_SB_SKIP(                                           "enable_avg_cdf");
            Skip_SB(                                            "avg_cdf_type");
        TEST_SB_END();
    }
    if (Monochrome) {
    }
    else {
        Skip_SB(                                                "separate_uv_delta_q");
    }
    bool equal_ac_dc_q;
    TESTELSE_SB_GET(equal_ac_dc_q,                              "equal_ac_dc_q");
    TESTELSE_SB_ELSE(                                           "equal_ac_dc_q");
        Skip_S1(5,                                              "base_y_dc_delta_q");
        Skip_SB(                                                "y_dc_delta_q_enabled");
    TESTELSE_SB_END();
    if (!Monochrome) {
        if (!equal_ac_dc_q) {
            Skip_S1(5,                                          "base_uv_dc_delta_q");
            Skip_SB(                                            "uv_dc_delta_q_enabled");
        }
        Skip_S1(5,                                              "base_uv_ac_delta_q");
        Skip_SB(                                                "uv_ac_delta_q_enabled");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::seg_info(int8u numSegments)
{
    const int8u Segmentation_Feature_Bits[SEG_LVL_MAX] = { 9, 0, 0 };
    const int8u Segmentation_Feature_Signed[SEG_LVL_MAX] = { 1, 0, 0 };

    Element_Begin1("seg_info");
    for (int8u i = 0; i < numSegments; ++i) {
        for (int8u j = 0; j < SEG_LVL_MAX; ++j) {
            TEST_SB_SKIP(                                       "feature_enabled");
                auto bitsToRead = Segmentation_Feature_Bits[j];
                if (Segmentation_Feature_Signed[j] == 1)
                    Skip_S2(1 + bitsToRead,                     "feature_value");
                else
                    Skip_S2(bitsToRead,                         "feature_value");
            TEST_SB_END();
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::sequence_filter_config() 
{
    Element_Begin1("sequence_filter_config");
    Skip_SB(                                                    "disable_loopfilters_across_tiles");
    Skip_SB(                                                    "enable_cdef");
    Skip_SB(                                                    "enable_gdf");
    TEST_SB_SKIP(                                               "enable_restoration");
        Skip_SB(                                                "lr_tools_disable[ 0 ][ RESTORE_PC_WIENER ]");
        Skip_SB(                                                "lr_tools_disable[ 0 ][ RESTORE_WIENER_NONSEP ]");
        TEST_SB_SKIP(                                           "enable_restoration");
            Skip_SB(                                            "lr_tools_disable[ 1 ][ RESTORE_WIENER_NONSEP ]");
        TEST_SB_END();
    TEST_SB_END();
    Skip_SB(                                                    "enable_ccso");
    if (single_picture_header_flag) {
    }
    else {
        TESTELSE_SB_SKIP(                                       "cdef_on_skip_txfm_always_on");
        TESTELSE_SB_ELSE(                                       "cdef_on_skip_txfm_always_on");
            Skip_SB(                                            "cdef_on_skip_txfm_disabled");
        TESTELSE_SB_END();
    }
    Skip_S1(2,                                                  "df_par_bits_minus2");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::temporal_delimiter_obu()
{
    Element_Begin1("temporal_delimiter_obu");
    SeenFrameHeader = false;
    Element_End0();

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av2::multi_stream_decoder_operation_obu()
{
#if MEDIAINFO_TRACE
    if (Trace_Activated)
    {
        //Parsing
        BS_Begin();

        Element_Begin1("multi_stream_decoder_operation_obu");
        int8u num_streams_minus2;
        Get_S1 (3, num_streams_minus2,                          "num_streams_minus2");
        Skip_S1(6,                                              "multi_config_idc");
        Skip_S1(5,                                              "multi_level_idx");
        Skip_SB(                                                "multi_tier");
        Skip_S1(4,                                              "multi_interop");
        TESTELSE_SB_SKIP(                                       "multi_even_allocation_flag");
        TESTELSE_SB_ELSE(                                       "multi_even_allocation_flag");
        Skip_S1(3,                                              "multi_large_picture_idc");
        TESTELSE_SB_END();
        for (int8u i = 0; i < num_streams_minus2 + 2; i++) {
            Skip_S1(5,                                          "sub_xlayer_id[ i ]");
            Skip_S1(5,                                          "sub_profile[ i ]");
            Skip_S1(5,                                          "sub_level[ i ]");
            Skip_SB(                                            "sub_tier[ i ]");
            Skip_S1(4,                                          "sub_mlayer_count[ i ]");
        }
        Element_End0();

        obu_end();
        BS_End();
    }
    else
#endif //MEDIAINFO_TRACE
        Skip_XX(Element_Size,                                   "Data");

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av2::multi_frame_header_obu()
{
    Element_Begin1("multi_frame_header_obu");
    Skip_XX(Element_Size,                                       "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::layer_config_record_obu()
{
    Element_Begin1("layer_config_record_obu");
    Skip_XX(Element_Size,                                       "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::atlas_segment_info_obu()
{
    Element_Begin1("atlas_segment_info_obu");
    Skip_XX(Element_Size,                                       "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::operating_point_set_obu()
{
    Element_Begin1("operating_point_set_obu");
    Skip_XX(Element_Size,                                       "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::buffer_removal_timing_obu()
{
    Element_Begin1("buffer_removal_timing_obu");
    Skip_XX(Element_Size,                                       "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::quantizer_matrix_obu()
{
    Element_Begin1("quantizer_matrix_obu");
    Skip_XX(Element_Size,                                       "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::film_grain_obu()
{
    Element_Begin1("film_grain_obu");
    Skip_XX(Element_Size,                                       "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::content_interpretation_obu()
{
    //Parsing
    BS_Begin();

    Element_Begin1("content_interpretation_obu");
    int8u scan_type_idc;
    bool color_description_present_flag, chroma_sample_position_present_flag, aspect_ratio_info_present_flag, timing_info_present_flag;
    Get_S1(2, scan_type_idc,                                    "scan_type_idc");           Param_Info1(Av2_scan_type_idc(scan_type_idc));
    Get_SB(color_description_present_flag,                      "color_description_present_flag");
    Get_SB(chroma_sample_position_present_flag,                 "chroma_sample_position_present_flag");
    Get_SB(aspect_ratio_info_present_flag,                      "aspect_ratio_info_present_flag");
    Get_SB(timing_info_present_flag,                            "timing_info_present_flag");
    Skip_S1(2,                                                  "reserved_2bit");
    int8u color_primaries{ 2 };
    int8u transfer_characteristics{ 2 };
    int8u matrix_coefficients{ 2 };
    bool full_range_flag{};
    if (color_description_present_flag) {
        int64u color_description_idc;
        Get_rg(2, color_description_idc,                        "color_description_idc");    Param_Info1(Av2_color_description_idc(static_cast<int8u>(color_description_idc)));
        if (color_description_idc == 0) { // Explicitly signaled
            Get_S1(8, color_primaries,                          "color_primaries");          Param_Info1(Mpegv_colour_primaries(color_primaries));
            Get_S1(8, transfer_characteristics,                 "transfer_characteristics"); Param_Info1(Mpegv_transfer_characteristics(transfer_characteristics));
            Get_S1(8, matrix_coefficients,                      "matrix_coefficients");      Param_Info1(Mpegv_matrix_coefficients(matrix_coefficients));
        }
        else {
            switch (color_description_idc) {
            case 1: // BT.709 SDR
                color_primaries = 1; transfer_characteristics = 1; matrix_coefficients = 1; break;
            case 2: // BT.2100 PQ
                color_primaries = 9; transfer_characteristics = 16; matrix_coefficients = 9; break;
            case 3: // BT.2100 HLG
                color_primaries = 9; transfer_characteristics = 18; matrix_coefficients = 9; break;
            case 4: // sRGB
                color_primaries = 1; transfer_characteristics = 13; matrix_coefficients = 0; break;
            case 5: // sYCC
                color_primaries = 1; transfer_characteristics = 13; matrix_coefficients = 6; break;
            }
        }
        Get_SB(full_range_flag,                                 "full_range_flag");
    }
    if (chroma_sample_position_present_flag) {
        Skip_uvlc(                                              "chroma_sample_position_top");
        if (scan_type_idc != 1)
            Skip_uvlc(                                          "chroma_sample_position_bottom");
    }
    if (aspect_ratio_info_present_flag) {
        int8u aspect_ratio_idc;
        Get_S1(8, aspect_ratio_idc,                             "aspect_ratio_idc");
        if (aspect_ratio_idc == 255) {
            Skip_uvlc(                                          "sar_width");
            Skip_uvlc(                                          "sar_height");
        }
    }
    float64 framerate{};
    bool equal_picture_interval;
    if (timing_info_present_flag) {
        Element_Begin1("timing_info");
        int32u num_units_in_display_tick, time_scale;
        Get_S4(32, num_units_in_display_tick,                   "num_units_in_display_tick");
        Get_S4(32, time_scale,                                  "time_scale");
        framerate = static_cast<float64>(time_scale) / num_units_in_display_tick;
        TEST_SB_GET(equal_picture_interval,                     "equal_picture_interval");
        int32u num_ticks_per_picture_minus_1;
            Get_uvlc(num_ticks_per_picture_minus_1,             "num_ticks_per_picture_minus_1");
            framerate /= num_ticks_per_picture_minus_1 + 1;
        TEST_SB_END();
        Element_End0();
    }
    Element_End0();

    obu_end();
    BS_End();

    //Filling
    FILLING_BEGIN_PRECISE();
    if (color_description_present_flag)
    {
        ColorDescriptionPresent = color_description_present_flag;
        ColorPrimaries = color_primaries;
        TransferCharacteristics = transfer_characteristics;
        MatrixCoefficients = matrix_coefficients;
        FullRange = full_range_flag;
    }
    if (timing_info_present_flag) {
        Fill(Stream_Video, 0, Video_FrameRate, framerate, 3);
        if (equal_picture_interval)
            Fill(Stream_Video, 0, Video_FrameRate_Mode, "Constant");
    }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av2::padding_obu()
{
    Element_Begin1("padding_obu");
    Skip_XX(Element_Size,                                       "Padding");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::metadata_unit(int64u metadata_type, int64u payload_size)
{
    auto Element_Size_Save = Element_Size;
    if (payload_size > Element_Size - Element_Offset) {
        Trusted_IsNot("payload_size");
        payload_size = Element_Size - Element_Offset;
    }
    Element_Size = Element_Offset + payload_size;

    switch (metadata_type) {
    case 1: metadata_hdr_cll(); break;
    case 2: metadata_hdr_mdcv(); break;
    case 3: metadata_itu_t_t35(); break;
    case 4: metadata_timecode(); break;
    case 6: metadata_banding_hints(); break;
    case 7: metadata_icc_profile(); break;
    case 8: metadata_scan_type(); break;
    case 9: metadata_temporal_point_info(); break;
    default: Skip_XX(Element_Size - Element_Offset,             "(Unknown)");
    }

    Element_Size = Element_Size_Save;
}

//---------------------------------------------------------------------------
void File_Av2::metadata_short_obu()
{
    Element_Begin1("metadata_short_obu");
    Element_Begin1("muh");
    BS_Begin();
    Skip_SB(                                                    "metadata_is_suffix");
    Skip_S1(3,                                                  "muh_layer_idc");
    bool muh_cancel_flag;
    Get_SB(muh_cancel_flag,                                     "muh_cancel_flag");
    Skip_S1(3,                                                  "muh_persistence_idc");
    BS_End();
    int64u metadata_type;
    Get_leb128(metadata_type,                                   "metadata_type"); Param_Info1(Av2_metadata_type(static_cast<int8u>(metadata_type)));
    Element_End0();
    if (!muh_cancel_flag) {
        metadata_unit(metadata_type, Element_Size - Element_Offset);
    }
    Element_End0();
    if (Element_Size - Element_Offset) {
        BS_Begin();
        obu_end();
        BS_End();
    }

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av2::metadata_group_obu()
{
    Element_Begin1("metadata_group_obu");
    BS_Begin();
    Skip_SB(                                                    "metadata_is_suffix");
    Skip_S1(2,                                                  "metadata_necessity_idc");
    Skip_S1(5,                                                  "metadata_application_id");
    BS_End();
    int64u metadata_unit_cnt_minus1;
    Get_leb128(metadata_unit_cnt_minus1,                        "metadata_unit_cnt_minus1");
    if (metadata_unit_cnt_minus1 > Element_Size) {
        Reject();
        return;
    }
    for (int64u i = 0; i <= metadata_unit_cnt_minus1; ++i) {
        Element_Begin1("metadata_unit");
        Element_Begin1("muh");
        int64u metadata_type;
        Get_leb128(metadata_type,                               "metadata_type"); Param_Info1(Av2_metadata_type(static_cast<int8u>(metadata_type)));
        BS_Begin();
        int8u muh_header_size;
        Get_S1(7, muh_header_size,                              "muh_header_size");
        bool muh_cancel_flag;
        Get_SB(muh_cancel_flag,                                 "muh_cancel_flag");
        BS_End();
        auto Element_Offset_Header_Begin = Element_Offset;
        int64u muh_payload_size{};
        if (!muh_cancel_flag) {
            Get_leb128(muh_payload_size,                        "muh_payload_size"); 
            BS_Begin();
            int8u muh_layer_idc;
            Get_S1 (3, muh_layer_idc,                           "muh_layer_idc");
            Skip_S1(3,                                          "muh_persistence_idc");
            Skip_S1(8,                                          "muh_priority");
            Skip_S1(2,                                          "muh_reserved_zero_2bits");
            BS_End();
            if (muh_layer_idc == 3) {
                if (obu_xlayer_id == GLOBAL_XLAYER_ID) {
                    int32u muh_xlayer_map;
                    Get_B4(muh_xlayer_map,                      "muh_xlayer_map");
                    for (int8u n = 0; n < 31; ++n) {
                        if (muh_xlayer_map & (0x1 << n)) {
                            Skip_B1(                            "muh_mlayer_map");
                        }
                    }
                }
                else {
                    Skip_B1(                                    "muh_mlayer_map");
                }
            }
        }
        auto headerRemainingBytes = muh_header_size - (Element_Offset - Element_Offset_Header_Begin);
        Skip_XX(headerRemainingBytes,                           "muh_header_extension_bytes");
        Element_End0();
        if (!muh_cancel_flag) {
            metadata_unit(metadata_type, muh_payload_size);
        }
        Element_End0();
    }
    Element_End0();
    if (Element_Size - Element_Offset) {
        BS_Begin();
        obu_end();
        BS_End();
    }

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av2::metadata_itu_t_t35()
{
    Element_Begin1("metadata_itu_t_t35");

    // TODO: Use ITUT T35 parser
    Skip_XX(Element_Size,                                       "Data");
    
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::metadata_hdr_cll()
{
    Element_Begin1("metadata_hdr_cll");

    //Parsing
    Get_LightLevel(maximum_content_light_level, maximum_frame_average_light_level);
    
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::metadata_hdr_mdcv()
{
    Element_Begin1("metadata_hdr_mdcv");

    //Parsing
    auto& HDR_Format = HDR[Video_HDR_Format][HdrFormat_SmpteSt2086];
    if (HDR_Format.empty())
    {
        HDR_Format = __T("SMPTE ST 2086");
        HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt2086] = "HDR10";
    }
    Get_MasteringDisplayColorVolume(HDR[Video_MasteringDisplay_ColorPrimaries][HdrFormat_SmpteSt2086], HDR[Video_MasteringDisplay_Luminance][HdrFormat_SmpteSt2086], true);
    
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::metadata_timecode()
{
    BS_Begin();
    Element_Begin1("metadata_timecode");
    Skip_S1(5,                                                  "counting_type");
    bool full_timestamp_flag;
    Get_SB(full_timestamp_flag,                                 "full_timestamp_flag");
    Skip_SB(                                                    "discontinuity_flag");
    Skip_SB(                                                    "cnt_dropped_flag");
    Skip_S2(9,                                                  "n_frames");
    if (full_timestamp_flag) {
        Skip_S1(6,                                              "seconds_value");
        Skip_S1(6,                                              "minutes_value");
        Skip_S1(5,                                              "hours_value");
    }
    else {
        TEST_SB_SKIP(                                           "seconds_flag");
            Skip_S1(6,                                          "seconds_value");
            TEST_SB_SKIP(                                       "minutes_flag");
                Skip_S1(6,                                      "minutes_value");
                TEST_SB_SKIP(                                   "hours_flag");
                    Skip_S1(5,                                  "hours_value");
                TEST_SB_END();
            TEST_SB_END();
        TEST_SB_END();
    }
    int8u time_offset_length;
    Get_S1(5, time_offset_length,                               "time_offset_length");
    if (time_offset_length > 0) {
        Skip_BS(time_offset_length,                             "time_offset_value");
    }
    Element_End0();
    byte_alignment();
    BS_End();
}

//---------------------------------------------------------------------------
void File_Av2::metadata_banding_hints()
{
#if MEDIAINFO_TRACE
    if (Trace_Activated)
    {
        BS_Begin();
        Element_Begin1("metadata_banding_hints");
        bool coding_banding_present_flag;
        Get_SB(coding_banding_present_flag,                     "coding_banding_present_flag");
        Skip_SB(                                                "source_banding_present_flag");
        if (coding_banding_present_flag) {
            TEST_SB_SKIP(                                       "banding_hints_flag");
                bool three_color_components;
                Get_SB(three_color_components,                  "three_color_components");
                auto numComponents = three_color_components ? 3 : 1;
                for (int8u plane = 0; plane < numComponents; ++plane) {
                    TEST_SB_SKIP(                               "banding_in_component_present_flag");
                        Skip_S1(6,                              "max_band_width_minus4");
                        Skip_S1(4,                              "max_band_step_minus1");
                    TEST_SB_END();
                }
                TEST_SB_SKIP(                                   "band_units_information_present_flag");
                    int8u num_band_units_rows_minus_1, num_band_units_cols_minus_1;
                    Get_S1(5, num_band_units_rows_minus_1,      "num_band_units_rows_minus_1");
                    Get_S1(5, num_band_units_cols_minus_1,      "num_band_units_cols_minus_1");
                    TEST_SB_SKIP(                               "varying_size_band_units_flag");
                        Skip_S1(3,                              "band_block_in_luma_samples");
                        for (int8u r = 0; r <= num_band_units_rows_minus_1; r++) {
                            Skip_S1(5,                          "vert_size_in_band_blocks_minus1");
                        }
                        for (int8u c = 0; c <= num_band_units_cols_minus_1; c++) {
                            Skip_S1(5,                          "horz_size_in_band_blocks_minus1");
                        }
                    TEST_SB_END();
                    for (int8u r = 0; r <= num_band_units_rows_minus_1; r++) {
                        for (int8u c = 0; c <= num_band_units_cols_minus_1; c++) {
                            Skip_SB(                            "banding_in_band_unit_present_flag");
                        }
                    }
                TEST_SB_END();
            TEST_SB_END();
        }
        Element_End0();
        byte_alignment();
        BS_End();
    } else
#endif //MEDIAINFO_TRACE
        Skip_XX(Element_Size,                                   "Data");
}

//---------------------------------------------------------------------------
void File_Av2::metadata_icc_profile()
{
    Element_Begin1("metadata_icc_profile");
#if defined(MEDIAINFO_ICC_YES)
    File_Icc ICC_Parser;
    ICC_Parser.StreamKind = Stream_Video;
    ICC_Parser.IsAdditional = true;
    Open_Buffer_Init(&ICC_Parser);
    Open_Buffer_Continue(&ICC_Parser);
    Open_Buffer_Finalize(&ICC_Parser);
    Merge(ICC_Parser, Stream_Video, 0, 0);
#else
    Skip_XX(Element_Size,                                       "icc_profile_data_payload_bytes");
#endif
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::metadata_scan_type()
{
    Element_Begin1("metadata_scan_type");
    BS_Begin();
    Skip_S1(5,                                                  "mps_pic_struct_type");
    Skip_S1(2,                                                  "mps_source_scan_type_idc");
    Skip_S1(1,                                                  "mps_duplicate_flag");
    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::metadata_temporal_point_info()
{
    BS_Begin();
    Element_Begin1("metadata_temporal_point_info");
    int8u frame_presentation_time_length_minus_1;
    Get_S1(5, frame_presentation_time_length_minus_1,           "frame_presentation_time_length_minus_1");
    auto n = frame_presentation_time_length_minus_1 + 1;
    Skip_BS(n,                                                  "frame_presentation_time");
    Element_End0();
    byte_alignment();
    BS_End();
}

//---------------------------------------------------------------------------
void File_Av2::frame_header(bool isFirst)
{
    Element_Begin1("frame_header");
    if (isFirst)
        SeenFrameHeader = 1;
    Element_End0();

    FILLING_BEGIN();
    if (isFirst && sequence_header_Parsed) {
        Frame_Count++;
        if (!Status[IsAccepted]) {
            if (Frame_Count == 1) {
                Buffer_MaximumSize *= 4; // Frames may be big
            }
            if (Frame_Count >= Frame_Count_Valid) {
                Accept();
            }
        }
        if (!Status[IsFilled] && Frame_Count >= (Frame_Count_Valid << 2)) {
            Fill();
            if (Config->ParseSpeed <= 1.0) {
                Finish();
            }
        }
    }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av2::tile_params(int16u frameWidth, int16u frameHeight, int8u uniformSbSize, int8u sbSize, bool isBridge)
{
    const int8u Num_4x4_Blocks_Wide[BLOCK_SIZES] = {
        1, 1, 2, 2, 2, 4, 4, 4, 8, 8, 8,
        16, 16, 16, 32, 32, 32, 64, 64, 1, 4, 2, 8, 4, 16,
        1, 8, 2, 16, 1, 16
    };

    const int8u Mi_Width_Log2[BLOCK_SIZES] = {
        0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3,
        4, 4, 4, 5, 5, 5, 6, 6, 0, 2, 1, 3, 2, 4,
        0, 3, 1, 4, 0, 4,
    };

    Element_Begin1("tile_params");
    auto miCols = 2 * ((frameWidth + 7) >> 3);
    auto miRows = 2 * ((frameHeight + 7) >> 3);
    auto sb4x4 = Num_4x4_Blocks_Wide[sbSize];
    auto sbShift = Mi_Width_Log2[sbSize];
    auto sbCols = (miCols + sb4x4 - 1) >> sbShift;
    auto sbRows = (miRows + sb4x4 - 1) >> sbShift;
    auto maxTileWidthSb = MAX_TILE_WIDTH >> (sbShift + 2);
    auto maxTileAreaSb = MAX_TILE_AREA >> (2 * (sbShift + 2));
    auto minLog2TileCols = tile_log2(maxTileWidthSb, sbCols);
    auto maxLog2TileCols = tile_log2(1, std::min(sbCols, MAX_TILE_COLS));
    auto maxLog2TileRows = tile_log2(1, std::min(sbRows, MAX_TILE_ROWS));
    auto minLog2Tiles = std::max(minLog2TileCols, tile_log2(maxTileAreaSb, sbRows * sbCols));
    bool uniform_tile_spacing_flag;
    if (isBridge) {
        uniform_tile_spacing_flag = true;
    }
    else {
        Get_SB(uniform_tile_spacing_flag,                       "uniform_tile_spacing_flag");
    }
    auto tileColsLog2 = minLog2TileCols;
    if (uniform_tile_spacing_flag) {
        if (!isBridge) {
            while (tileColsLog2 < maxLog2TileCols) {
                TESTELSE_SB_SKIP(                               "increment_tile_cols_log2");
                    tileColsLog2 += 1;
                TESTELSE_SB_ELSE(                               "increment_tile_cols_log2");
                    break;
                TESTELSE_SB_END();
            }
        }
        auto minLog2TileRows = std::max(minLog2Tiles - tileColsLog2, 0);
        auto tileRowsLog2 = minLog2TileRows;
        if (!isBridge) {
            while (tileRowsLog2 < maxLog2TileRows) {
                TESTELSE_SB_SKIP(                               "increment_tile_rows_log2");
                    tileRowsLog2 += 1;
                TESTELSE_SB_ELSE(                               "increment_tile_cols_log2");
                    break;
                TESTELSE_SB_END();
            }
        }
    }
    else {
        auto widestTileSb = 1;
        auto startSb = 0;
        for (int i = 0; startSb < sbCols; ++i) {
            auto maxWidth = std::min(sbCols - startSb, maxTileWidthSb);
            int64u width_in_sbs_minus_1;
            Get_ns(maxWidth, width_in_sbs_minus_1,              "width_in_sbs_minus_1");
            auto sizeSb = static_cast<int>(width_in_sbs_minus_1) + 1;
            widestTileSb = std::max(sizeSb, widestTileSb);
            startSb += sizeSb;
        }
        startSb = 0;
        for (int i = 0; startSb < sbRows; ++i) {
            auto maxTileHeightSb = std::max(maxTileAreaSb / widestTileSb, 1);
            auto maxHeight = std::min(sbRows - startSb, maxTileHeightSb);
            int64u height_in_sbs_minus_1;
            Get_ns(maxHeight, height_in_sbs_minus_1,            "height_in_sbs_minus_1");
            auto sizeSb = static_cast<int>(height_in_sbs_minus_1) + 1;
            startSb += sizeSb;
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av2::tile_group_obu(int64u obuPayloadSize)
{
    Element_Begin1("tile_group_obu");
    BS_Begin();
    bool is_first_tile_group, uncompressed_header_flag;
    Get_SB(is_first_tile_group,                                 "is_first_tile_group");
    if (Element_Code == 10 || Element_Code == 21) {
        Skip_SB(                                                "restricted_prediction_switch");
    }
    if (is_first_tile_group) {
        uncompressed_header_flag = true;
    }
    else {
        Get_SB(uncompressed_header_flag,                        "uncompressed_header_flag");
    }
    if (uncompressed_header_flag) {
        frame_header(is_first_tile_group);
    }
    Skip_BS(BS->Remain(), "(Not parsed)");
    BS_End();
    Element_End0();
}

//***************************************************************************
// Functions
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Av2::is_extensible_obu(int8u obu_type) const
{
    switch (obu_type)
    {
    case  1:
    case  3:
    case 16:
    case 24:
    case 18:
    case 17: return true;
    default: return false;
    }
}

//---------------------------------------------------------------------------
int8u File_Av2::get_seq_sb_size() const
{
    if (use_256x256_superblock) {
        return BLOCK_256X256;
    }
    else if (use_128x128_superblock) {
        return BLOCK_128X128;
    }
    else {
        return BLOCK_64X64;
    }
}

//---------------------------------------------------------------------------
int File_Av2::tile_log2(int blkSize, int target) const
{
    int k;
    for (k = 0; (blkSize << k) < target; ++k) {}
    return k;
}

//***************************************************************************
// Custom functions for common code
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av2::obu_end()
{
    if (is_extensible_obu(static_cast<int8u>(Element_Code))) {
        TESTELSE_SB_SKIP(                                       "obu_extension_flag");
            obu_extension_data(BS->Remain());
        TESTELSE_SB_ELSE(                                       "obu_extension_flag");
            trailing_bits(BS->Remain());
        TESTELSE_SB_END();
    }
    else {
        trailing_bits(BS->Remain());
    }
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av2::Get_leb128(int64u& Info, const char* Name)
{
    Info = 0;
    for (int8u i = 0; i < 8; i++)
    {
        if (Element_Offset >= Element_Size)
            break; // End of stream reached, not normal
        int8u leb128_byte = BigEndian2int8u(Buffer + Buffer_Offset + (size_t)Element_Offset);
        Element_Offset++;
        Info |= (static_cast<int64u>(leb128_byte & 0x7f) << (i * 7));
        if (!(leb128_byte & 0x80))
        {
            #if MEDIAINFO_TRACE
            if (Trace_Activated)
            {
                Element_Offset -= (1LL + i);
                Param(Name, Info, (i + 1) * 8);
                Param_Info(__T("(") + Ztring::ToZtring(i + 1) + __T(" bytes)"));
                Element_Offset += (1LL + i);
            }
            #endif //MEDIAINFO_TRACE
            return;
        }
    }
    Trusted_IsNot("Size is wrong");
    Info = 0;
}

#define INTEGRITY(TOVALIDATE, ERRORTEXT, OFFSET) \
if (!(TOVALIDATE)) \
{ \
    Trusted_IsNot(ERRORTEXT); \
    return; \
} \

void File_Av2::Get_uvlc(int32u& Info, const char* Name)
{
    int64u Info64 = 0;
    auto remain_begin = BS->Remain();
    int64u leadingZeros = 0;
    while (true) {
        INTEGRITY(BS->Remain() >= 1, "Size is wrong", BS->Offset_Get())
        if (BS->GetB())
            break;
        ++leadingZeros;
    }
    if (leadingZeros >= 32) {
        Info64 = (1ULL << 32) - 1;
    }
    else {
        INTEGRITY(BS->Remain() >= leadingZeros, "Size is wrong", BS->Offset_Get())
        int64u value = BS->Get4(static_cast<int8u>(leadingZeros)); // leadingZeros < 32 at this point
        Info64 = value + (1ULL << leadingZeros) - 1;
    }
    Info = static_cast<int32u>(Info64);
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            auto Bits = remain_begin - BS->Remain();
            Param(Name, Info, static_cast<int8u>(Bits));
            Param_Info(__T("(") + Ztring::ToZtring(Bits) + __T(" bits)"));
        }
    #endif //MEDIAINFO_TRACE
}

void File_Av2::Skip_uvlc(const char* Name)
{
    int32u temp;
    Get_uvlc(temp, Name);
}

void File_Av2::Get_su(int8u n, int64s& Info, const char* Name)
{
    Info = 0;
    auto remain_begin = BS->Remain();
    INTEGRITY(BS->Remain() >= n, "Size is wrong", BS->Offset_Get())
    Info = BS->Get4(n);
    auto signMask = 1ULL << (n - 1);
    if (Info & signMask)
        Info = Info - 2 * signMask;
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            auto Bits = remain_begin - BS->Remain();
            Param(Name, Info, static_cast<int8u>(Bits));
            Param_Info(__T("(") + Ztring::ToZtring(Bits) + __T(" bits)"));
        }
    #endif //MEDIAINFO_TRACE
}

void File_Av2::Skip_su(int8u n, const char* Name)
{
    int64s temp;
    Get_su(n, temp, Name);
}

void File_Av2::Get_ns(int8u n, int64u& Info, const char* Name)
{
    Info = 0;
    auto remain_begin = BS->Remain();
    auto w = FloorLog2(n) + 1U;
    auto m = (1U << w) - n;
    INTEGRITY(BS->Remain() >= w - 1, "Size is wrong", BS->Offset_Get())
    auto v = BS->Get4(w - 1);
    if (v < m)
        Info = v;
    else {
        INTEGRITY(BS->Remain() >= 1, "Size is wrong", BS->Offset_Get())
        auto extra_bit = BS->GetB();
        Info = (static_cast<int64u>(v) << 1) - m + extra_bit;
    }
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            auto Bits = remain_begin - BS->Remain();
            Param(Name, Info, static_cast<int8u>(Bits));
            Param_Info(__T("(") + Ztring::ToZtring(Bits) + __T(" bits)"));
        }
    #endif //MEDIAINFO_TRACE
}

void File_Av2::Skip_ns(int8u n, const char* Name)
{
    int64u temp;
    Get_ns(n, temp, Name);
}

void File_Av2::Get_rg(int8u n, int64u& Info, const char* Name)
{
    Info = 0;
    auto remain_begin = BS->Remain();
    for (int8u q = 0; q < 32; ++q) {
        INTEGRITY(BS->Remain() >= 1, "Size is wrong", BS->Offset_Get())
        auto rg_bit = BS->GetB();
        if (rg_bit == 0) {
            INTEGRITY(BS->Remain() >= n, "Size is wrong", BS->Offset_Get())
            auto remainder = BS->Get4(n);
            Info = (static_cast<int64u>(q) << n) + remainder;
            #if MEDIAINFO_TRACE
                if (Trace_Activated)
                {
                    auto Bits = remain_begin - BS->Remain();
                    Param(Name, Info, static_cast<int8u>(Bits));
                    Param_Info(__T("(") + Ztring::ToZtring(Bits) + __T(" bits)"));
                }
            #endif //MEDIAINFO_TRACE
            return;
        }
    }
    Trusted_IsNot("Size is wrong"); // bitstream conformance fail
}

//---------------------------------------------------------------------------
} //NameSpace

#endif //MEDIAINFO_AV2_YES
