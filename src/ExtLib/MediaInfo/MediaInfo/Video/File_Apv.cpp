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
#if defined(MEDIAINFO_APV_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Apv.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <cmath>
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
static string Apv_pbu_type(int8u pbu_type)
{
    switch (pbu_type) {
    case  1: return "primary frame";
    case  2: return "non-primary frame";
    case 25: return "preview frame";
    case 26: return "depth frame";
    case 27: return "alpha frame";
    case 65: return "access unit information";
    case 66: return "metadata";
    case 67: return "filler";
    default: return std::to_string(pbu_type);
    }
}

//---------------------------------------------------------------------------
static string APV_Profile(int8u profile_idc)
{
    switch (profile_idc) {
    case 33: return "422-10";
    case 44: return "422-12";
    case 55: return "444-10";
    case 66: return "444-12";
    case 77: return "4444-10";
    case 88: return "4444-12";
    case 99: return "400-10";
    default: return std::to_string(profile_idc);
    }
}

//---------------------------------------------------------------------------
static string APV_Colorspace(int8u chroma_format_idc)
{
    switch (chroma_format_idc) {
    case 0:
    case 2:
    case 3: return "YUV";
    case 4: return "YUVA";
    default: return std::to_string(chroma_format_idc);
    }
}

//---------------------------------------------------------------------------
static string APV_Chroma(int8u chroma_format_idc)
{
    switch (chroma_format_idc) {
    case 0: return "4:0:0";
    case 2: return "4:2:2";
    case 3:
    case 4: return "4:4:4";
    default: return std::to_string(chroma_format_idc);
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Apv::File_Apv()
{
    StreamSource = IsStream;
    FrameIsAlwaysComplete = false;
}

//---------------------------------------------------------------------------
File_Apv::~File_Apv()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Apv::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "APV");

    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "APV");
    Fill(Stream_Video, 0, Video_Format_Profile, APV_Profile(frame_info_Current.profile_idc)+'@'+Ztring::ToZtring((float)frame_info_Current.level_idc / 30, 1).To_UTF8());
    Fill(Stream_Video, 0, "band_idc", frame_info_Current.band_idc);
    Fill(Stream_Video, 0, Video_Width, frame_info_Current.frame_width);
    Fill(Stream_Video, 0, Video_Height, frame_info_Current.frame_height);
    Fill(Stream_Video, 0, Video_ColorSpace, APV_Colorspace(frame_info_Current.chroma_format_idc));
    Fill(Stream_Video, 0, Video_ChromaSubsampling, APV_Chroma(frame_info_Current.chroma_format_idc));
    Fill(Stream_Video, 0, Video_BitDepth, frame_info_Current.bit_depth_minus8 + 8);
    if (frame_info_Current.color_description_present_flag) {
        Fill(Stream_Video, 0, Video_colour_primaries, Mpegv_colour_primaries(frame_info_Current.color_primaries));
        Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(frame_info_Current.transfer_characteristics));
        Fill(Stream_Video, 0, Video_matrix_coefficients, Mpegv_matrix_coefficients(frame_info_Current.matrix_coefficients));
        Fill(Stream_Video, 0, Video_colour_range, frame_info_Current.full_range_flag ? "Full" : "Limited");
    }
}

//---------------------------------------------------------------------------
void File_Apv::Streams_Fill()
{
}

//---------------------------------------------------------------------------
void File_Apv::Streams_Finish()
{
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
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Apv::FileHeader_Begin()
{
    if (IsSub)
        return true;

    //Must have enough buffer for having header
    if (Buffer_Size < 8)
        return false; //Must wait for more data

    if (CC4(Buffer + 4) != 0x61507631) // signature = aPv1
    {
        Reject();
        return false;
    }

    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Apv::Read_Buffer_OutOfBand()
{
    Element_Begin1("APVDecoderConfigurationRecord");
    int8u number_of_configuration_entry, number_of_frame_info;
    Skip_B1(                                                    "configurationVersion");
    Get_B1 (number_of_configuration_entry,                      "number_of_configuration_entry");
    for (int8u i = 0; i < number_of_configuration_entry; ++i) {
        Element_Begin1("configuration_entry");
        Skip_B1(                                                "pbu_type");
        Get_B1 (number_of_frame_info,                           "number_of_frame_info");
        for (int8u j = 0; j < number_of_frame_info; ++j) {
            Element_Begin1("frame_info");
            FrameInfo fi;
            BS_Begin();
            Skip_S1(6,                                          "reserved_zero_6bits");
            Get_SB (fi.color_description_present_flag,          "color_description_present_flag");
            Skip_SB(                                            "capture_time_distance_ignored");
            BS_End();
            Get_B1 (fi.profile_idc,                             "profile_idc");                 Param_Info1(APV_Profile(fi.profile_idc));
            Get_B1 (fi.level_idc,                               "level_idc");                   Param_Info3((float)fi.level_idc / 30, nullptr, 1);
            Get_B1 (fi.band_idc,                                "band_idc");
            Get_B4 (fi.frame_width,                             "frame_width");
            Get_B4 (fi.frame_height,                            "frame_height");
            BS_Begin();
            Get_S1 (4, fi.chroma_format_idc,                    "chroma_format_idc");           Param_Info1(APV_Chroma(fi.chroma_format_idc));
            Get_S1 (4, fi.bit_depth_minus8,                     "bit_depth_minus8");
            BS_End();
            Skip_B1(                                            "capture_time_distance");
            if (fi.color_description_present_flag) {
                Get_B1 (fi.color_primaries,                     "color_primaries");             Param_Info1(Mpegv_colour_primaries(fi.color_primaries));
                Get_B1 (fi.transfer_characteristics,            "transfer_characteristics");    Param_Info1(Mpegv_transfer_characteristics(fi.transfer_characteristics));
                Get_B1 (fi.matrix_coefficients,                 "matrix_coefficients");         Param_Info1(Mpegv_matrix_coefficients(fi.matrix_coefficients));
                BS_Begin();
                Get_SB (fi.full_range_flag,                     "full_range_flag");
                Skip_S1(7,                                      "reserved_zero_7bits");
                BS_End();
            }
            FILLING_BEGIN();
                if (!i && !j) {
                    frame_info_Current = fi; // Using the first frame info as the reference
                }
            FILLING_END();
            Element_End0();
        }
        Element_End0();
    }
    Element_End0();

    FILLING_BEGIN_PRECISE();
        Accept();
    FILLING_END();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Apv::Header_Parse()
{
    //Parsing
    switch (Element_Level) {
    case 2: {
        Get_B4 (au_size,                                        "au_size");
        FILLING_BEGIN();
            Header_Fill_Size(Element_Offset + au_size);
            Header_Fill_Code(0, "raw_bitstream_access_unit");
            DataMustAlwaysBeComplete = false;
        FILLING_END();
        break;
    }
    case 3: {
        int32u signature;
        Get_C4 (signature,                                      "signature");
        if (signature != 0x61507631) {
            Trusted_IsNot("signature");
            if (IsSub) {
                Header_Fill_Size(Buffer_Size);
            }
            else {
                Reject();
            }
            break;
        }
        FILLING_BEGIN();
            Header_Fill_Size(au_size);
            Header_Fill_Code(0, "access_unit");
            DataMustAlwaysBeComplete = false;
        FILLING_END();
        break;
    }
    case 4: {
        int32u pbu_size;
        int8u pbu_type;
        Get_B4 (pbu_size,                                       "pbu_size");
        Element_Begin1(                                         "pbu_header");
            Get_B1 (pbu_type,                                   "pbu_type");
            Skip_B2(                                            "group_id");
            Skip_B1(                                            "reserved_zero_8bits");
        Element_End0();
        FILLING_BEGIN();
            Header_Fill_Size(4 + pbu_size);
            Header_Fill_Code(pbu_type, Apv_pbu_type(pbu_type).c_str());
            switch (pbu_type) {
            case 65:
            case 66:
            case 67:
                DataMustAlwaysBeComplete = true;
                break;
            default:
                DataMustAlwaysBeComplete = false;
            }
        FILLING_END();
        break;
    }
    case 5: {
        int32u tile_size;
        Get_B4 (tile_size,                                      "tile_size");
        FILLING_BEGIN();
            Header_Fill_Size(Element_Offset + tile_size);
            Header_Fill_Code(0, PosTiles < frame_info_Current.NumTiles ? "title" : "filler");
            DataMustAlwaysBeComplete = true;
        FILLING_END();
        break;
    }
    }
}

//---------------------------------------------------------------------------
void File_Apv::Data_Parse()
{
    switch (Element_Level) {
    case 1:
    case 2:
        Element_ThisIsAList();
        break;
    case 3:
        switch (Element_Code) {
        case   1:
        case   2:
        case  25:
        case  26:
        case  27: frame(); break;
        case  65: au_info(); break;
        case  66: metadata(); break;
        case  67: filler(); break;
        default : Skip_XX(Element_Size,                         "(Unknown)");
        }
        break;
    case 4:
        if (PosTiles < frame_info_Current.NumTiles) {
            tile();
        }
        else {
            filler();
        }
        break;
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Apv::frame()
{
    frame_header();

    FILLING_BEGIN();
        Element_ThisIsAList();
        if (!Status[IsFilled] && Frame_Count) {
            Fill();
            if (Config->ParseSpeed < 1.0) {
                Finish();
            }
        }
        Frame_Count++;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Apv::frame_header()
{
    FrameInfo fi;
    Element_Begin1("frame_header");
    frame_info(fi);
    Skip_B1 (                                                   "reserved_zero_8bits");
    BS_Begin();
    TEST_SB_GET(fi.color_description_present_flag,              "color_description_present_flag");
        Get_S1 ( 8, fi.color_primaries,                         "color_primaries");             Param_Info1(Mpegv_colour_primaries(fi.color_primaries));
        Get_S1 ( 8, fi.transfer_characteristics,                "transfer_characteristics");    Param_Info1(Mpegv_transfer_characteristics(fi.transfer_characteristics));
        Get_S1 ( 8, fi.matrix_coefficients,                     "matrix_coefficients");         Param_Info1(Mpegv_matrix_coefficients(fi.matrix_coefficients));
        Get_SB (    fi.full_range_flag,                         "full_range_flag");
    TEST_SB_END();
    if (fi.NumComps) { // We can not decode if chroma_format_idc is unknown, so we skip the rest of the frame header
    TEST_SB_SKIP(                                               "use_q_matrix");
        quantization_matrix(fi);
    TEST_SB_END();
    tile_info(fi);
    Skip_S1 ( 8,                                                "reserved_zero_8bits");
    byte_alignment();
    }
    BS_End();
    Element_End0();

    FILLING_BEGIN();
        frame_info_Current = fi;
        PosTiles = 0;
        if (!Status[IsAccepted]) {
            Accept();
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Apv::frame_info(FrameInfo& fi)
{
    Element_Begin1("frame_info");
    Get_B1 (    fi.profile_idc,                                 "profile_idc");         Param_Info1(APV_Profile(fi.profile_idc));
    Get_B1 (    fi.level_idc,                                   "level_idc");           Param_Info3((float)fi.level_idc / 30, nullptr, 1);
    BS_Begin();
    Get_S1 ( 3, fi.band_idc,                                    "band_idc");
    Skip_S1( 5,                                                 "reserved_zero_5bits");
    BS_End();
    Get_B3 (    fi.frame_width,                                 "frame_width");
    Get_B3 (    fi.frame_height,                                "frame_height");
    BS_Begin();
    Get_S1 ( 4, fi.chroma_format_idc,                           "chroma_format_idc");   Param_Info1(APV_Chroma(fi.chroma_format_idc));
    Get_S1 ( 4, fi.bit_depth_minus8,                            "bit_depth_minus8");
    BS_End();
    Skip_B1(                                                    "capture_time_distance");
    Skip_B1(                                                    "reserved_zero_8bits");
    Element_End0();

    switch (fi.chroma_format_idc) {
    case 0: fi.NumComps = 1; break;
    case 2: fi.NumComps = 3; break;
    case 3: fi.NumComps = 3; break;
    case 4: fi.NumComps = 4; break;
    default: fi.NumComps = 0; break;
    }
}

//---------------------------------------------------------------------------
void File_Apv::quantization_matrix(const FrameInfo& fi)
{
    Element_Begin1("quantization_matrix");
    for (int8u i = 0; i < fi.NumComps; ++i) {
        for (int8u y = 0; y < 8; ++y) {
            for (int8u x = 0; x < 8; ++x) {
                Skip_S1(8,                                      "q_matrix");
            }
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::tile_info(FrameInfo& fi)
{
    Element_Begin1("tile_info");
    int32u tile_width_in_mbs, tile_height_in_mbs;
    Get_S4(20, tile_width_in_mbs,                               "tile_width_in_mbs");
    Get_S4(20, tile_height_in_mbs,                              "tile_height_in_mbs");
    // The value of tile_width_in_mbs MUST be greater than or equal to 16.
    // The value of tile_height_in_mbs MUST be greater than or equal to 8.
    if (tile_width_in_mbs < 16 || tile_height_in_mbs < 8) {
        Trusted_IsNot("Tile width or height is too small");
        return;
    }
    int32u FrameWidthInMbsY = static_cast<int32u>(ceil(fi.frame_width / 16));
    int32u FrameHeightInMbsY = static_cast<int32u>(ceil(fi.frame_height / 16));
    int32u tileCols = 0;
    int32u tileRows = 0;
    // Calculate number of columns
    for (int32u startMb = 0; startMb < FrameWidthInMbsY; startMb += tile_width_in_mbs) {
        ++tileCols;
    }

    // Calculate number of rows
    for (int32u startMb = 0; startMb < FrameHeightInMbsY; startMb += tile_height_in_mbs) {
        ++tileRows;
    }
    // The value of TileCols MUST be less than or equal to 20.
    // The value of TileRows MUST be less than or equal to 20.
    if (tileCols > 20 || tileRows > 20) {
        Trusted_IsNot("Tiles exceed the maximum allowed number");
        return;
    }
    fi.NumTiles = tileCols * tileRows;
    TEST_SB_SKIP(                                               "tile_size_present_in_fh_flag");
    for (int32u i = 0; i < fi.NumTiles; ++i) {
        Skip_S4(32,                                             "tile_size_in_fh");
    }
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::au_info()
{
    FrameInfo fi;

    Element_Begin1("au_info");
    int16u num_frames;
    Get_B2 (num_frames,                                         "num_frames");
    for (int16u i = 0; i < num_frames; ++i) {
        Skip_B1(                                                "pbu_type");
        Skip_B2(                                                "group_id");
        Skip_B1(                                                "reserved_zero_8bits");
        frame_info(fi);
    }
    Skip_B1(                                                    "reserved_zero_8bits");
    byte_alignment();
    filler();
    Element_End0();

    FILLING_BEGIN_PRECISE();
        frame_info_Current = fi;
        if (!Status[IsAccepted]) {
            Accept();
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Apv::metadata()
{
    Element_Begin1("metadata");
    int32u metadata_size;
    Get_B4(metadata_size,                                       "metadata_size");
    auto Element_Size_Save = Element_Size;
    if (metadata_size <= Element_Size - Element_Offset)
        Element_Size = Element_Offset + metadata_size;
    else
        Trusted_IsNot("Size is wrong (metadata_size)");
    do {
        int64u payloadType{ 0 };
        while (Element_Offset < Element_Size && Buffer[Buffer_Offset + Element_Offset] == 0xFF) {
            Skip_B1(                                            "ff_byte");
            payloadType += 0xFF;
        }
        int8u metadata_payload_type;
        Get_B1(metadata_payload_type,                           "metadata_payload_type");
        payloadType += metadata_payload_type;

        int64u payloadSize{ 0 };
        while (Element_Offset < Element_Size && Buffer[Buffer_Offset + Element_Offset] == 0xFF) {
            Skip_B1(                                            "ff_byte");
            payloadSize += 0xFF;
        }
        int8u metadata_payload_size;
        Get_B1(metadata_payload_size,                           "metadata_payload_size");
        payloadSize += metadata_payload_size;
        metadata_payload(payloadType, payloadSize);
    } while (Element_Offset < Element_Size);
    Element_Size = Element_Size_Save;
    filler();
    Element_End0();

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Apv::filler()
{
    Element_Begin1("filler");
    while (Element_Offset < Element_Size && Buffer[Buffer_Offset + Element_Offset] == 0xFF) {
        ++Element_Offset;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::tile()
{
    vector<int32u> tile_data_sizes;
    Element_Begin1("tile_header");
    auto Element_Offset_begin = Element_Offset;
    int16u tile_header_size;
    Get_B2 (tile_header_size,                                   "tile_header_size");
    Skip_B2(                                                    "tile_index");
    if (!frame_info_Current.NumComps) { // We can not decode if chroma_format_idc is unknown, so we skip the rest of the frame header
        Skip_XX(Element_Size - Element_Offset,                  "(Unknown)");
        return;
    }
    for (int8u i = 0; i < frame_info_Current.NumComps; ++i) {
        int32u tile_data_size;
        Get_B4(tile_data_size,                                  "tile_data_size");
        tile_data_sizes.push_back(tile_data_size);
    }
    for (int8u i = 0; i < frame_info_Current.NumComps; ++i) {
        Skip_B1(                                                "tile_qp");
    }
    Skip_B1(                                                    "reserved_zero_8bits");
    byte_alignment();
    if (Element_Offset_begin + tile_header_size != Element_Offset)
        Trusted_IsNot("Size is wrong (tile_header_size)");
    Element_End0();
    for (int8u i = 0; i < frame_info_Current.NumComps; ++i) {
        Skip_XX(tile_data_sizes.at(i),                          "tile_data");
    }
    Skip_XX(Element_Size - Element_Offset,                      "tile_dummy_byte");
    PosTiles++;
}

//---------------------------------------------------------------------------
void File_Apv::byte_alignment()
{
    Element_Begin1("byte_alignment");
    while (BS->Remain() & 7)
        Mark_0();// alignment_bit_equal_to_zero
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_payload(int64u payloadType, int64u payloadSize)
{
    Element_Begin1("metadata_payload");
    auto Element_Size_Save = Element_Size;
    if (payloadSize <= Element_Size - Element_Offset)
        Element_Size = Element_Offset + payloadSize;
    else
        Trusted_IsNot("Size is wrong (payloadSize)");
    switch (payloadType) {
    case   4: metadata_itu_t_t35(); break;
    case   5: metadata_mdcv(); break;
    case   6: metadata_cll(); break;
    case  10: metadata_filler(); break;
    case 170: metadata_user_defined(); break;
    default: metadata_undefined(); break;
    }
    Skip_XX(Element_Size - Element_Offset, "(Not parsed)");
    Element_Size = Element_Size_Save;
    byte_alignment();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_filler()
{
    Element_Begin1("metadata_filler");
    while (Element_Offset < Element_Size && Buffer[Buffer_Offset + Element_Offset] == 0xFF) {
        ++Element_Offset;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_itu_t_t35()
{
    Element_Begin1("metadata_itu_t_t35");
    // TODO: Use ITU T35 parser 
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_mdcv()
{
    Element_Begin1("metadata_mdcv");
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
void File_Apv::metadata_cll()
{
    Element_Begin1("metadata_cll");
    Get_LightLevel(maximum_content_light_level, maximum_frame_average_light_level);
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_user_defined()
{
    Element_Begin1("metadata_user_defined");
    Skip_UUID(                                                  "uuid");
    Skip_XX(Element_Size - Element_Offset,                      "user_defined_data_payload");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_undefined()
{
    Element_Begin1("metadata_undefined");
    Skip_XX(Element_Size - Element_Offset,                      "undefined_metadata_payload_byte");
    Element_End0();
}


//---------------------------------------------------------------------------
} //NameSpace

#endif //MEDIAINFO_APV_YES
