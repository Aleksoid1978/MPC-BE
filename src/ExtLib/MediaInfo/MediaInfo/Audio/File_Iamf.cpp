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
#if defined(MEDIAINFO_IAMF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Iamf.h"
#include "MediaInfo/MediaInfo_Internal.h"
#ifdef MEDIAINFO_MPEG4_YES
    #include "MediaInfo/Multiple/File_Mpeg4_Descriptors.h"
#endif
#ifdef MEDIAINFO_FLAC_YES
    #include "MediaInfo/Audio/File_Flac.h"
#endif
#ifdef MEDIAINFO_OPUS_YES
    #include "MediaInfo/Audio/File_Opus.h"
#endif
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
namespace Elements
{
    const int32u iamf = 0x69616D66;
}

namespace CodecIDs
{
    const int32u Opus = 0x4F707573;
    const int32u mp4a = 0x6D703461;
    const int32u fLaC = 0x664C6143;
    const int32u ipcm = 0x6970636D;
}

// audio_element_type
#define CHANNEL_BASED   0
#define SCENE_BASED     1
#define OBJECT_BASED    2

// param_definition_type
#define PARAMETER_DEFINITION_MIX_GAIN           0
#define PARAMETER_DEFINITION_DEMIXING           1
#define PARAMETER_DEFINITION_RECON_GAIN         2
#define PARAMETER_DEFINITION_POLAR              3
#define PARAMETER_DEFINITION_CART_8             4
#define PARAMETER_DEFINITION_CART_16            5
#define PARAMETER_DEFINITION_DUAL_POLAR         6
#define PARAMETER_DEFINITION_DUAL_CART_8        7
#define PARAMETER_DEFINITION_DUAL_CART_16       8
#define PARAMETER_DEFINITION_MOMENTARY_LOUDNESS 9

// ambisonics_mode
#define MONO        0
#define PROJECTION  1

// layout_type
#define LOUDSPEAKERS_SS_CONVENTION  2
#define BINAURAL                    3

// animation_type
#define STEP         0
#define LINEAR       1
#define BEZIER       2
#define INTER_LINEAR 3

//---------------------------------------------------------------------------
static const char* Iamf_obu_type(int8u obu_type)
{
    switch (obu_type)
    {
    case 0x00   : return "OBU_IA_Codec_Config";
    case 0x01   : return "OBU_IA_Audio_Element";
    case 0x02   : return "OBU_IA_Mix_Presentation";
    case 0x03   : return "OBU_IA_Parameter_Block";
    case 0x04   : return "OBU_IA_Temporal_Delimiter";
    case 0x05   : return "OBU_IA_Audio_Frame";
    //   6~23   : OBU_IA_Audio_Frame_ID0 to OBU_IA_Audio_Frame_ID17
    case 0x06   : return "OBU_IA_Audio_Frame_ID0";
    case 0x07   : return "OBU_IA_Audio_Frame_ID1";
    case 0x08   : return "OBU_IA_Audio_Frame_ID2";
    case 0x09   : return "OBU_IA_Audio_Frame_ID3";
    case 0x0A   : return "OBU_IA_Audio_Frame_ID4";
    case 0x0B   : return "OBU_IA_Audio_Frame_ID5";
    case 0x0C   : return "OBU_IA_Audio_Frame_ID6";
    case 0x0D   : return "OBU_IA_Audio_Frame_ID7";
    case 0x0E   : return "OBU_IA_Audio_Frame_ID8";
    case 0x0F   : return "OBU_IA_Audio_Frame_ID9";
    case 0x10   : return "OBU_IA_Audio_Frame_ID10";
    case 0x11   : return "OBU_IA_Audio_Frame_ID11";
    case 0x12   : return "OBU_IA_Audio_Frame_ID12";
    case 0x13   : return "OBU_IA_Audio_Frame_ID13";
    case 0x14   : return "OBU_IA_Audio_Frame_ID14";
    case 0x15   : return "OBU_IA_Audio_Frame_ID15";
    case 0x16   : return "OBU_IA_Audio_Frame_ID16";
    case 0x17   : return "OBU_IA_Audio_Frame_ID17";
    //     24   : OBU_IA_Metadata
    case 0x18   : return "OBU_IA_Metadata";
    //  25~30   : Reserved for future use
    //     31   : OBU_IA_Sequence_Header
    case 0x1F   : return "OBU_IA_Sequence_Header";
    default     : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Iamf_audio_element_type(int8u audio_element_type)
{
    switch (audio_element_type)
    {
    case CHANNEL_BASED  : return "CHANNEL_BASED";
    case SCENE_BASED    : return "SCENE_BASED";
    case OBJECT_BASED   : return "OBJECT_BASED";
    default             : return "";
    }
}

//---------------------------------------------------------------------------
static string Iamf_profile(int8u profile)
{
    switch (profile)
    {
    case   0: return "Simple Profile";
    case   1: return "Base Profile";
    case   2: return "Base-Enhanced Profile";
    case   3: return "Base-Advanced Profile";
    case   4: return "Advanced-1 Profile";
    case   5: return "Advanced-2 Profile";
    default : return std::to_string(profile);
    }
}

//---------------------------------------------------------------------------
static string Iamf_loudspeaker_layout(int8u loudspeaker_layout)
{
    switch (loudspeaker_layout)
    {
    case 0x0: return "Mono";
    case 0x1: return "Stereo";
    case 0x2: return "5.1ch";
    case 0x3: return "5.1.2ch";
    case 0x4: return "5.1.4ch";
    case 0x5: return "7.1ch";
    case 0x6: return "7.1.2ch";
    case 0x7: return "7.1.4ch";
    case 0x8: return "3.1.2ch";
    case 0x9: return "Binaural";
    case 0xF: return "Expanded channel layouts";
    default: return std::to_string(loudspeaker_layout);
    }
}

//---------------------------------------------------------------------------
static string Iamf_expanded_loudspeaker_layout(int8u expanded_loudspeaker_layout)
{
    switch (expanded_loudspeaker_layout)
    {
    case 0:  return "LFE";            // The low-frequency effects subset (LFE) of 7.1.4ch
    case 1:  return "Stereo-S";       // The surround subset (Ls/Rs) of 5.1.4ch
    case 2:  return "Stereo-SS";      // The side surround subset (Lss/Rss) of 7.1.4ch
    case 3:  return "Stereo-RS";      // The rear surround subset (Lrs/Rrs) of 7.1.4ch
    case 4:  return "Stereo-TF";      // The top front subset (Ltf/Rtf) of 7.1.4ch
    case 5:  return "Stereo-TB";      // The top back subset (Ltb/Rtb) of 7.1.4ch
    case 6:  return "Top-4ch";        // The top 4 channels (Ltf/Rtf/Ltb/Rtb) of 7.1.4ch
    case 7:  return "3.0ch";          // The front 3 channels (L/C/R) of 7.1.4ch
    case 8:  return "9.1.6ch";        // Loudspeaker location ordering of 9.1.6ch
    case 9:  return "Stereo-F";       // The front subset (FL/FR) of 9.1.6ch
    case 10: return "Stereo-Si";      // The side subset (SiL/SiR) of 9.1.6ch
    case 11: return "Stereo-TpSi";    // The top side subset (TpSiL/TpSiR) of 9.1.6ch
    case 12: return "Top-6ch";        // The top 6 channels (TpFL/TpFR/TpSiL/TpSiR/TpBL/TpBR) of 9.1.6ch
    case 13: return "10.2.9.3ch";     // Loudspeaker location ordering of 10.2.9.3ch
    case 14: return "LFE-Pair";       // The low-frequency effects subset (LFE1/LFE2) of 10.2.9.3ch
    case 15: return "Bottom-3ch";     // The bottom 3 channels (BtFL/BtFC/BtFR) of 10.2.9.3ch
    case 16: return "7.1.5.4ch";      // Loudspeaker location ordering of 7.1.5.4ch
    case 17: return "Bottom-4ch";     // The bottom 4 channels (BtFL/BtFR/BtBL/BtBR) of 7.1.5.4ch
    case 18: return "Top-1ch";        // The top subset (TpC) of 7.1.5.4ch
    case 19: return "Top-5ch";        // The top 5 channels (Ltf/Rtf/TpC/Ltb/Rtb) of 7.1.5.4ch
    default: return std::to_string(expanded_loudspeaker_layout);
    }
}

//---------------------------------------------------------------------------
static string Iamf_animation_type(int64u animation_type)
{
    switch (animation_type)
    {
    case STEP           : return "STEP";
    case LINEAR         : return "LINEAR";
    case BEZIER         : return "BEZIER";
    case INTER_LINEAR   : return "INTER_LINEAR";
    default: return std::to_string(animation_type);
    }
}

//---------------------------------------------------------------------------
static string Iamf_headphones_rendering_mode(int8u headphones_rendering_mode)
{
    switch (headphones_rendering_mode)
    {
    case 0: return "HEADPHONES_RENDERING_MODE_STEREO";
    case 1: return "HEADPHONES_RENDERING_MODE_BINAURAL_WORLD_LOCKED";
    case 2: return "HEADPHONES_RENDERING_MODE_BINAURAL_HEAD_LOCKED";
    default: return std::to_string(headphones_rendering_mode);
    }
}

//---------------------------------------------------------------------------
static string Iamf_binaural_filter_profile(int8u binaural_filter_profile)
{
    switch (binaural_filter_profile)
    {
    case 0: return "BINAURAL_FILTER_PROFILE_AMBIENT";
    case 1: return "BINAURAL_FILTER_PROFILE_DIRECT";
    case 2: return "BINAURAL_FILTER_PROFILE_REVERBERANT";
    default: return std::to_string(binaural_filter_profile);
    }
}

//---------------------------------------------------------------------------
static string Iamf_loudness_info_type(int8u info_type)
{
    const char* info_types[]{
        "TRUE_PEAK",
        "ANCHORED_LOUDNESS",
        "LIVE",
        "MOMENTARY_LOUDNESS",
        "LOUDNESS_RANGE",
    };
    int info_types_size = sizeof(info_types) / sizeof(info_types[0]);

    string to_return;
    for (int i = 0; i < info_types_size; ++i) {
        if (info_type & (1U << i)) {
            if (!to_return.empty())
                to_return += ", ";
            to_return += info_types[i];
        }
    }
    return to_return;
}

//---------------------------------------------------------------------------
static string Iamf_parameter_definition_type(int64u parameter_definition_type)
{
    switch (parameter_definition_type)
    {
    case PARAMETER_DEFINITION_MIX_GAIN      : return "MIX_GAIN";
    case PARAMETER_DEFINITION_DEMIXING      : return "DEMIXING";
    case PARAMETER_DEFINITION_RECON_GAIN    : return "RECON_GAIN";
    case PARAMETER_DEFINITION_POLAR         : return "POLAR";
    case PARAMETER_DEFINITION_CART_8        : return "CART_8";
    case PARAMETER_DEFINITION_CART_16       : return "CART_16";
    case PARAMETER_DEFINITION_DUAL_POLAR    : return "DUAL_POLAR";
    case PARAMETER_DEFINITION_DUAL_CART_8   : return "DUAL_CART_8";
    case PARAMETER_DEFINITION_DUAL_CART_16  : return "DUAL_CART_16";
    default: return std::to_string(parameter_definition_type);
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Iamf::File_Iamf() :File__Analyze()
{
    // Configuration
    StreamSource = IsStream;
}

//---------------------------------------------------------------------------
File_Iamf::~File_Iamf()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iamf::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "IAMF");

    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "IAMF");
    Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Eclipsa Audio");
}

//---------------------------------------------------------------------------
void File_Iamf::Streams_Finish()
{
    if (Parser_Opus)
        Parser_Opus->Finish();
    if (Parser_Flac)
        Parser_Flac->Finish();

    Fill(Stream_Audio, 0, "NumberOfPresentations", mixpresentations.size());
    Fill(Stream_Audio, 0, "NumberOfSubstreams", substreams.size());
    Fill(Stream_Audio, 0, "NumberOfObjects", num_objects_total);

    // Currently multiple ia_codec_config are not well handled //TODO: how to manage multiple ia_codec_config or if it changes midstream?
    if (!IsSub && Config->ParseSpeed >= 1 && Frame_Count && substreams.size() && codec_config_count == 1) {
        int32u SamplingRate = Retrieve_Const(Stream_Audio, 0, Audio_SamplingRate).To_int32u();
        int32u SamplesPerFrame = Retrieve_Const(Stream_Audio, 0, Audio_SamplesPerFrame).To_int32u();
        if (SamplingRate && SamplesPerFrame)
        {
            int64u SamplesCountPerSubstream{ SamplesPerFrame * Frame_Count / substreams.size() };
            Fill(Stream_Audio, 0, Audio_Duration, SamplesCountPerSubstream / (static_cast<float64>(SamplingRate) / 1000), 0);
            Fill(Stream_Audio, 0, Audio_SamplingCount, SamplesPerFrame * Frame_Count);
            Fill(Stream_Audio, 0, Audio_BitRate, File_Size / (SamplesCountPerSubstream / static_cast<float64>(SamplingRate)) * 8, 0);
        }
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Iamf::FileHeader_Begin()
{
    //Must have enough buffer for having header
    if (Buffer_Size<6)
        return false; //Must wait for more data

    if (!IsSub
        // Must start with ia_sequence_header
        // obu_type (5)           = 31
        // obu_redundant_copy (1) = 0 or 1
        // reserved (1)           = 0
        // obu_extension_flag (1) = 0 or 1 (extension is not used as at IAMF v2)
        && Buffer[0] != 0xF8 && Buffer[0] != 0xFC  // obu_extension_flag = 0
        && Buffer[0] != 0xF9 && Buffer[0] != 0xFD) // obu_extension_flag = 1
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
void File_Iamf::Read_Buffer_OutOfBand()
{
    //Parsing
    int64u configOBUs_size;
    int8u configurationVersion;
    Get_B1 (configurationVersion,                               "configurationVersion");
    if (configurationVersion != 1)
    {
        Skip_XX(Element_Size - Element_Offset,                  "(Not supported)");
        Finish();
        return;
    }
    Get_leb128  (        configOBUs_size,                       "configOBUs_size");

    Open_Buffer_Continue(Buffer, Buffer_Size);
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iamf::Header_Parse()
{
    //Parsing
    int8u obu_type;
    bool obu_redundant_copy, obu_trimming_status_flag{}, obu_extension_flag;
    int64u obu_size, num_samples_to_trim_at_end, num_samples_to_trim_at_start, extension_header_size;
    BS_Begin();
    Get_S1      (5,     obu_type,                               "obu_type");
    Get_SB      (       obu_redundant_copy,                     "obu_redundant_copy");
    switch (obu_type) {
    case 0x02: // OBU_IA_Mix_Presentation
        Get_SB  (       optional_fields_flag,                   "optional_fields_flag");
        break;
    case 0x04: // OBU_IA_Temporal_Delimiter
        Skip_SB (                                               "is_not_key_frame");
        break;
    default:
        if (obu_type >= 0x05 && obu_type <= 0x17)
            Get_SB (    obu_trimming_status_flag,               "obu_trimming_status_flag");
        else
            Skip_SB(                                            "reserved");
        break;
    }
    Get_SB      (       obu_extension_flag,                     "obu_extension_flag");
    BS_End();
    Get_leb128  (       obu_size,                               "obu_size");
    int64u obu_size_begin{ Element_Offset };
    if (obu_trimming_status_flag) {
        Get_leb128(     num_samples_to_trim_at_end,             "num_samples_to_trim_at_end");
        Get_leb128(     num_samples_to_trim_at_start,           "num_samples_to_trim_at_start");
    }
    if (obu_extension_flag) {
        Get_leb128  (   extension_header_size,                  "extension_header_size");
        Skip_XX     (   extension_header_size,                  "extension_header_bytes");
    }

    //Filling
    FILLING_BEGIN();
        if (obu_type == 0x00 && !obu_redundant_copy)
            ++codec_config_count;
        Header_Fill_Size(obu_size_begin + obu_size);
        Header_Fill_Code(obu_type, Iamf_obu_type(obu_type));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Iamf::Data_Parse()
{
    //Parsing
    switch (Element_Code)
    {
    case 0x00   : ia_codec_config(); break;
    case 0x01   : ia_audio_element(); break;
    case 0x02   : ia_mix_presentation(); break;
    case 0x03   : ia_parameter_block(); break;
    case 0x04   : ia_temporal_delimiter(); break;
    case 0x05   : ia_audio_frame(true); break;
    case 0x06   :
    case 0x07   :
    case 0x08   :
    case 0x09   :
    case 0x0A   :
    case 0x0B   :
    case 0x0C   :
    case 0x0D   :
    case 0x0E   :
    case 0x0F   :
    case 0x10   :
    case 0x11   :
    case 0x12   :
    case 0x13   :
    case 0x14   :
    case 0x15   :
    case 0x16   :
    case 0x17   : ia_audio_frame(false); break;
    case 0x18   : ia_metadata(); break;
    case 0x1F   : ia_sequence_header(); break;
    default     : Skip_XX(Element_Size - Element_Offset, "Data"); break;
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iamf::ia_sequence_header()
{
    //Parsing
    int32u ia_code;
    int8u additional_profile;
    Get_C4 (ia_code,                                            "ia_code");
    Info_B1(primary_profile,                                    "primary_profile"); Param_Info1(Iamf_profile(primary_profile));
    Get_B1 (additional_profile,                                 "additional_profile"); Param_Info1(Iamf_profile(additional_profile));

    //Filling
    FILLING_BEGIN_PRECISE();
        if (!Status[IsAccepted])
        {
            if (ia_code != Elements::iamf) {
                Reject();
                return;
            }
            Accept();
            Fill(Stream_Audio, 0, Audio_Format_Profile, Iamf_profile(additional_profile));
        }
    FILLING_ELSE();
        Reject();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Iamf::ia_codec_config()
{
    //Parsing
    int64u codec_config_id, num_samples_per_frame;
    int32u codec_id{};
    int16u audio_roll_distance;
    auto IsFirst = Retrieve_Const(Stream_Audio, 0, Audio_CodecID).empty(); // Currently 2 ia_codec_config are not well handled about sampling rate //TODO: how to manage 2 difference sampling rates?
    Get_leb128 (        codec_config_id,                        "codec_config_id");
    Element_Begin1("codec_config");
        Get_C4 (        codec_id,                               "codec_id");
        Get_leb128 (    num_samples_per_frame,                  "num_samples_per_frame");
        Get_B2 (        audio_roll_distance,                    "audio_roll_distance"); Param_Info1(static_cast<int16s>(audio_roll_distance));
        FILLING_BEGIN();
            auto CodecID = Ztring::ToZtring_From_CC4(codec_id);
            if (CodecID != Retrieve_Const(Stream_Audio, 0, Audio_CodecID)) {
                Fill(Stream_Audio, 0, Audio_CodecID, CodecID);
            }
        FILLING_END();
        Element_Begin1("decoder_config");
            switch (codec_id)
            {
            case CodecIDs::Opus: {
                #if defined(MEDIAINFO_OPUS_YES)
                if (Parser_Opus)
                    Parser_Opus->Finish();
                auto MI = new File_Opus();
                MI->FromIamf = true;
                Open_Buffer_Init(MI);
                Open_Buffer_Continue(MI);
                Parser_Opus.reset(MI);
                FILLING_BEGIN_PRECISE();
                    if (IsFirst) {
                        Merge(*Parser_Opus.get(), Stream_Audio, 0, 0, false);
                    }
                FILLING_END();
                #endif
                break;
            }
            case CodecIDs::mp4a: {
                #if defined(MEDIAINFO_MPEG4_YES)
                File_Mpeg4_Descriptors MI;
                MI.FromIamf = true;
                Open_Buffer_Init(&MI);
                Open_Buffer_Continue(&MI);
                Open_Buffer_Finalize(&MI);
                FILLING_BEGIN_PRECISE();
                    if (IsFirst) {
                        Merge(MI, Stream_Audio, 0, 0, false);
                    }
                FILLING_END();
                #endif
                break;
            }
            case CodecIDs::fLaC: {
                #if defined(MEDIAINFO_FLAC_YES)
                if (Parser_Flac)
                    Parser_Flac->Finish();
                auto MI = new File_Flac();
                MI->NoFileHeader = true;
                MI->FromIamf = true;
                Open_Buffer_Init(MI);
                Open_Buffer_Continue(MI);
                Parser_Flac.reset(MI);
                FILLING_BEGIN_PRECISE();
                    if (IsFirst) {
                        Merge(*Parser_Flac.get(), Stream_Audio, 0, 0, false);
                    }
                FILLING_END();
                #endif
                break;
            }
            case CodecIDs::ipcm: {
                int32u sample_rate;
                int8u sample_format_flags, sample_size;
                Get_B1 (sample_format_flags,                    "sample_format_flags");
                Get_B1 (sample_size,                            "sample_size");
                Get_B4 (sample_rate,                            "sample_rate");
                FILLING_BEGIN_PRECISE();
                    const char* Endianness = sample_format_flags & 1 ? "Little" : "Big";
                    Fill(Stream_Audio, 0, Audio_Format_Settings_Endianness, Endianness);
                    Fill(Stream_Audio, 0, Audio_BitDepth, sample_size);
                    Fill(Stream_Audio, 0, Audio_SamplingRate, sample_rate);
                FILLING_END();
                break;
            }
            default:
                Skip_XX(Element_Size - Element_Offset, "(Not parsed)");
                break;
            }
        Element_End0();
    Element_End0();

    FILLING_BEGIN_PRECISE();
        codecs.insert({ codec_config_id, codec_id });
        if (IsFirst && num_samples_per_frame && Retrieve_Const(Stream_Audio, 0, Audio_SamplesPerFrame).empty()) {
            Fill(Stream_Audio, 0, Audio_SamplesPerFrame, num_samples_per_frame);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Iamf::ia_audio_element()
{
    //Parsing
    int64u audio_element_id, codec_config_id, num_substreams, num_parameters;
    int8u audio_element_type;
    Get_leb128 (        audio_element_id,                   "audio_element_id");
    BS_Begin();
    Get_S1 (     3,     audio_element_type,                 "audio_element_type"); Param_Info1(Iamf_audio_element_type(audio_element_type));
    Skip_S1(     5,                                         "reserved_for_future_use");
    BS_End();
    Get_leb128 (        codec_config_id,                    "codec_config_id");
    Get_leb128 (        num_substreams,                     "num_substreams");
    if (num_substreams > Element_Size) {
        Reject();
        return;
    }
    for (int64u i = 0; i < num_substreams; ++i) {
        int64u audio_substream_id;
        Get_leb128 (    audio_substream_id,                 "audio_substream_id");
        substreams.insert({ audio_substream_id, codec_config_id });
    }
    Get_leb128 (        num_parameters,                     "num_parameters");
    if (num_parameters > Element_Size) {
        Reject();
        return;
    }
    for (int64u i = 0; i < num_parameters; ++i) {
        int64u param_definition_type;
        Get_leb128 (    param_definition_type,              "param_definition_type");
        switch (param_definition_type) {
        case PARAMETER_DEFINITION_MIX_GAIN:
            Trusted_IsNot("PARAMETER_DEFINITION_MIX_GAIN SHALL NOT be present in Audio Element OBU");
            return;
            break;
        case PARAMETER_DEFINITION_DEMIXING: {
            Element_Begin1("demixing_info");
            ParamDefinition(PARAMETER_DEFINITION_DEMIXING);
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_RECON_GAIN: {
            Element_Begin1("recon_gain_info");
            ParamDefinition(PARAMETER_DEFINITION_RECON_GAIN);
            Element_End0();
            break;
        }
        default: {
            int64u param_definition_size;
            Get_leb128 (param_definition_size,              "param_definition_size");
            Skip_XX(    param_definition_size,              "param_definition_bytes");
        }
        }
    }
    switch (audio_element_type) {
    case CHANNEL_BASED: {
        Element_Begin1("scalable_channel_layout_config");
            BS_Begin();
            Get_S1 (3,      num_layers,                     "num_layers");
            Skip_S1(5,                                      "reserved_for_future_use");
            BS_End();
            recon_gain_is_present_flag_Vec.clear();
            for (int8u i = 1; i <= num_layers; ++i) {
                Element_Begin1("channel_audio_layer_config");
                int8u loudspeaker_layout;
                bool output_gain_is_present_flag, recon_gain_is_present_flag;
                BS_Begin();
                Get_S1 (4, loudspeaker_layout,              "loudspeaker_layout"); Param_Info1(Iamf_loudspeaker_layout(loudspeaker_layout));
                Get_SB (   output_gain_is_present_flag,     "output_gain_is_present_flag");
                Get_SB (   recon_gain_is_present_flag,      "recon_gain_is_present_flag");
                Skip_S1(2,                                  "reserved_for_future_use");
                BS_End();
                Skip_B1(                                    "substream_count");
                Skip_B1(                                    "coupled_substream_count");
                if (output_gain_is_present_flag == 1) {
                    int16u output_gain;
                    BS_Begin();
                    Skip_S1(6,                              "output_gain_flags");
                    Skip_S1(2,                              "reserved_for_future_use");
                    BS_End();
                    Get_B2(output_gain,                     "output_gain"); Param_Info1(static_cast<int16s>(output_gain));
                }
                if (num_layers == 1 && loudspeaker_layout == 15) {
                    int8u expanded_loudspeaker_layout;
                    Get_B1(expanded_loudspeaker_layout, "expanded_loudspeaker_layout"); Param_Info1(Iamf_expanded_loudspeaker_layout(expanded_loudspeaker_layout));
                }
                recon_gain_is_present_flag_Vec.push_back(recon_gain_is_present_flag);
                Element_End0();
            }
        Element_End0();
        break;
    }
    case SCENE_BASED: {
        Element_Begin1("ambisonics_config");
            int64u ambisonics_mode;
            Get_leb128  (ambisonics_mode,                   "ambisonics_mode"); Param_Info1(ambisonics_mode==MONO?"MONO":ambisonics_mode==PROJECTION?"PROJECTION":"");
            if (ambisonics_mode == MONO) {
                Element_Begin1("ambisonics_mono_config");
                    int8u output_channel_count;
                    Get_B1(output_channel_count,            "output_channel_count");
                    Skip_B1(                                "substream_count");
                    if (Config->ParseSpeed > 0.5) {
                        for (int8u i = 0; i < output_channel_count; ++i) {
                            Skip_B1(                        ("channel_mapping[" + std::to_string(i) + "]").c_str());
                        }
                    }
                    else {
                        Skip_XX(output_channel_count,       "channel_mapping");
                    }
                Element_End0();
            }
            else if (ambisonics_mode == PROJECTION) {
                Element_Begin1("ambisonics_projection_config");
                    int8u output_channel_count, substream_count, coupled_substream_count;
                    Get_B1(output_channel_count,            "output_channel_count");
                    Get_B1(substream_count,                 "substream_count");
                    Get_B1(coupled_substream_count,         "coupled_substream_count");
                    if (Config->ParseSpeed > 0.5) {
                        for (int i = 0; i < (substream_count + coupled_substream_count) * output_channel_count; ++i) {
                            int16u demixing_matrix;
                            Get_B2(demixing_matrix,         ("demixing_matrix[" + std::to_string(i) + "]").c_str()); Param_Info1(static_cast<int16s>(demixing_matrix));
                        }
                    }
                    else {
                        Skip_XX(2 * static_cast<int64u>(substream_count + coupled_substream_count) * output_channel_count, "demixing_matrix");
                    }
                Element_End0();
            }
        Element_End0();
        break;
    }
    case OBJECT_BASED: {
        Element_Begin1("objects_config");
        int64u objects_config_size;
        int8u num_objects;
        Get_leb128(objects_config_size,                     "objects_config_size");
        Get_B1(num_objects,                                 "num_objects");
        Skip_XX(objects_config_size - 1,                    "objects_config_extension_bytes");
        Element_End0();
        FILLING_BEGIN_PRECISE();
            num_objects_total += num_objects;
        FILLING_END();
        break;
    }
    default: {
        int64u audio_element_config_size;
        Get_leb128(     audio_element_config_size,          "audio_element_config_size");
        Skip_XX(        audio_element_config_size,          "audio_element_config_bytes");
    }
    }

    //Filling
    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Iamf::ia_mix_presentation()
{
    //Parsing
    int64u mix_presentation_id, count_label, num_sub_mixes;
    Get_leb128  (       mix_presentation_id,                "mix_presentation_id");
    Get_leb128  (       count_label,                        "count_label");
    if (count_label > Element_Size) {
        Reject();
        return;
    }
    for (int64u i = 0; i < count_label; ++i) {
        Skip_UTF8(SizeUpTo0(),                              "annotations_language");
        Skip_B1(                                            "zero");
    }
    for (int64u i = 0; i < count_label; ++i) {
        Skip_UTF8(SizeUpTo0(),                              "localized_presentation_annotations");
        Skip_B1(                                            "zero");
    }
    Get_leb128  (       num_sub_mixes,                      "num_sub_mixes");
    if (num_sub_mixes > Element_Size) {
        Reject();
        return;
    }
    for (int64u i = 0; i < num_sub_mixes; ++i) {
        int64u num_audio_elements;
        Get_leb128(     num_audio_elements,                 "num_audio_elements");
        if (num_audio_elements > Element_Size) {
            Reject();
            return;
        }
        for (int64u j = 0; j < num_audio_elements; ++j) {
            int64u audio_element_id;
            Get_leb128( audio_element_id,                   "audio_element_id");
            for (int64u k = 0; k < count_label; ++k) {
                Skip_UTF8(SizeUpTo0(),                      "localized_element_annotations");
                Skip_B1(                                    "zero");
            }
            Element_Begin1("rendering_config");
                RenderingConfig();
            Element_End0();
            Element_Begin1("element_mix_gain");
                ParamDefinition(PARAMETER_DEFINITION_MIX_GAIN);
            Element_End0();
        }
        Element_Begin1("output_mix_gain");
            ParamDefinition(PARAMETER_DEFINITION_MIX_GAIN);
        Element_End0();
        int64u num_layouts;
        Get_leb128  (   num_layouts,                        "num_layouts");
        if (num_layouts > Element_Size) {
            Reject();
            return;
        }
        for (int64u j = 0; j < num_layouts; ++j) {
            Element_Begin1("loudness_layout");
                int8u layout_type;
                BS_Begin();
                Get_S1(2, layout_type,                      "layout_type"); Param_Info1(layout_type==2?"LOUDSPEAKERS_SS_CONVENTION":layout_type==3?"BINAURAL":"RESERVED");
                if (layout_type == LOUDSPEAKERS_SS_CONVENTION) {
                    Skip_S1(4,                              "sound_system");
                    Skip_S1(2,                              "reserved_for_future_use");
                }
                else if (layout_type == BINAURAL || layout_type < 2) {
                    Skip_S1(6,                              "reserved_for_future_use");
                }
                BS_End();
            Element_End0();
            Element_Begin1("loudness");
            int16u integrated_loudness, digital_peak;
                int8u info_type;
                Get_B1( info_type,                          "info_type"); Param_Info1(Iamf_loudness_info_type(info_type));
                Get_B2( integrated_loudness,                "integrated_loudness"); Param_Info1(static_cast<int16s>(integrated_loudness));
                Get_B2( digital_peak,                       "digital_peak"); Param_Info1(static_cast<int16s>(digital_peak));
                if (info_type & 1) {
                    Element_Begin1("true_peak");
                    Skip_B2(                                "true_peak");
                    Element_End0();
                }
                if (info_type & 2) {
                    Element_Begin1("anchored_loudness");
                    int8u num_anchored_loudness;
                    Get_B1(num_anchored_loudness,           "num_anchored_loudness");
                    for (int8u n = 0; n < num_anchored_loudness; ++n) {
                        int16u anchored_loudness;
                        Skip_B1 (                           "anchor_element");
                        Get_B2  (anchored_loudness,         "anchored_loudness"); Param_Info1(static_cast<int16s>(anchored_loudness));
                    }
                    Element_End0();
                }
                if ((info_type & 0xFC) > 0) {
                    int64u info_type_size;
                    Get_leb128  (info_type_size,            "info_type_size");
                    auto Element_Offset_Begin = Element_Offset;
                    if (info_type & 8) {
                        Element_Begin1("momentary_loudness");
                        ParamDefinition(PARAMETER_DEFINITION_MOMENTARY_LOUDNESS);
                        Element_End0();
                    }
                    if (info_type & 16) {
                        BS_Begin();
                        Skip_S1(6,                          "loudness_range");
                        Skip_S1(2,                          "reserved_loudness_range");
                        BS_End();
                    }
                    auto info_type_size_remain = info_type_size - (Element_Offset - Element_Offset_Begin);
                    Skip_XX(info_type_size_remain,          "info_type_bytes");
                }
            Element_End0();
        }
    }
    if (optional_fields_flag || Element_Size > Element_Offset) {
        Element_Begin1("mix_presentation_tags");
            int8u num_tags;
            Get_B1(num_tags,                                "num_tags");
            for (int8u i = 0; i < num_tags; ++i) {
                Element_Begin1("tag");
                Skip_UTF8(SizeUpTo0(),                      "tag_name");
                Skip_B1("zero");
                Skip_UTF8(SizeUpTo0(),                      "tag_value");
                Skip_B1("zero");
                Element_End0();
            }
        Element_End0();
        if (optional_fields_flag) {
            Element_Begin1("mix_presentation_optional_fields");
            int64u optional_fields_size;
            Get_leb128(optional_fields_size,                "optional_fields_size");
            Skip_B1(                                        "preferred_loudspeaker_renderer");
            Skip_B1(                                        "preferred_binaural_renderer");
            Skip_XX(optional_fields_size - 2,               "optional_fields_remaining_bytes");
            Element_End0();
        }
    }

    //Filling
    FILLING_BEGIN_PRECISE();
        mixpresentations.insert({ mix_presentation_id, num_sub_mixes });
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Iamf::ParamDefinition(int64u param_definition_type)
{
    //Parsing
    int64u parameter_id, parameter_rate, duration{}, constant_subblock_duration{}, num_subblocks{}, subblock_duration;
    int8u param_definition_mode;
    Get_leb128  (       parameter_id,                       "parameter_id");
    Get_leb128  (       parameter_rate,                     "parameter_rate");
    BS_Begin();
    Get_S1      (1,     param_definition_mode,              "param_definition_mode");
    Skip_S1     (7,                                         "reserved_for_future_use");
    BS_End();
    if (param_definition_mode == 0) {
        Get_leb128(     duration,                           "duration");
        Get_leb128(     constant_subblock_duration,         "constant_subblock_duration");
        if (constant_subblock_duration == 0) {
            Get_leb128( num_subblocks,                      "num_subblocks");
            if (num_subblocks > Element_Size) {
                Reject();
                return;
            }
            for (int64u i = 0; i < num_subblocks; ++i) {
                Get_leb128(subblock_duration,               "subblock_duration");
            }
        }
    }
    if (param_definition_type == PARAMETER_DEFINITION_MIX_GAIN) {
        int16u default_mix_gain;
        Get_B2(         default_mix_gain,                   "default_mix_gain");
        Param_Info1(static_cast<int16s>(default_mix_gain));
        Param_Info2(static_cast<double>(static_cast<int16s>(default_mix_gain)) / 256, " dB");
    }
    if (param_definition_type == PARAMETER_DEFINITION_DEMIXING) {
        Element_Begin1("default_demixing_info_parameter_data");
            BS_Begin();
            Skip_S1(3,                                      "dmixp_mode");
            Skip_S1(5,                                      "reserved_for_future_use");
            Skip_S1(4,                                      "default_w");
            Skip_S1(4,                                      "reserved_for_future_use");
            BS_End();
        Element_End0();
    }
    if (param_definition_type == PARAMETER_DEFINITION_RECON_GAIN) {
    }
    if (param_definition_type == PARAMETER_DEFINITION_POLAR) {
        Element_Name("polar_param_definition");
        BS_Begin();
        Skip_S2(9,                                          "default_azimuth");
        Skip_S1(8,                                          "default_elevation");
        Skip_S1(7,                                          "default_distance");
        BS_End();
    }
    if (param_definition_type == PARAMETER_DEFINITION_CART_8) {
        Element_Name("cart8_param_definition");
        Skip_B1(                                            "default_x");
        Skip_B1(                                            "default_y");
        Skip_B1(                                            "default_z");
    }
    if (param_definition_type == PARAMETER_DEFINITION_CART_16) {
        Element_Name("cart16_param_definition");
        Skip_B2(                                            "default_x");
        Skip_B2(                                            "default_y");
        Skip_B2(                                            "default_z");
    }
    if (param_definition_type == PARAMETER_DEFINITION_DUAL_POLAR) {
        Element_Name("dual_polar_param_definition");
        BS_Begin();
        Skip_S2(9,                                          "default_first_azimuth");
        Skip_S1(8,                                          "default_first_elevation");
        Skip_S1(7,                                          "default_first_distance");
        Skip_S2(9,                                          "default_second_azimuth");
        Skip_S1(8,                                          "default_second_elevation");
        Skip_S1(7,                                          "default_second_distance");
        BS_End();
    }
    if (param_definition_type == PARAMETER_DEFINITION_DUAL_CART_8) {
        Element_Name("dual_cart8_param_definition");
        Skip_B1(                                            "default_first_x");
        Skip_B1(                                            "default_first_y");
        Skip_B1(                                            "default_first_z");
        Skip_B1(                                            "default_second_x");
        Skip_B1(                                            "default_second_y");
        Skip_B1(                                            "default_second_z");
    }
    if (param_definition_type == PARAMETER_DEFINITION_DUAL_CART_16) {
        Element_Name("dual_cart16_param_definition");
        Skip_B2(                                            "default_first_x");
        Skip_B2(                                            "default_first_y");
        Skip_B2(                                            "default_first_z");
        Skip_B2(                                            "default_second_x");
        Skip_B2(                                            "default_second_y");
        Skip_B2(                                            "default_second_z");
    }
    if (param_definition_type == PARAMETER_DEFINITION_MOMENTARY_LOUDNESS) {
        Element_Name("momentary_loudness_param_definition");
        int8u num_bin_pairs_minus_one;
        BS_Begin();
        Get_S1 (3, num_bin_pairs_minus_one,                 "num_bin_pairs_minus_one");
        Skip_S1(6,                                          "bin_width_minus_one");
        Skip_S1(6,                                          "first_bin_center");
        Skip_S1(1,                                          "reserved_momentary_loudness");
        BS_End();
        for (int8u i = 0; i <= num_bin_pairs_minus_one; ++i) {
            Element_Begin1("bin_Pair");
            Skip_B1(                                        "bin[0]");
            Skip_B1(                                        "bin[1]");
            Element_End0();
        }
    }

    //Filling
    FILLING_BEGIN();
        ParamDefinitionData param_definition{};
        param_definition.param_definition_type = param_definition_type;
        param_definition.param_definition_mode = param_definition_mode;
        param_definition.duration = duration;
        param_definition.constant_subblock_duration = constant_subblock_duration;
        param_definition.num_subblocks = num_subblocks;
        param_definitions.insert({ parameter_id, param_definition });
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Iamf::RenderingConfig()
{
    int8u headphones_rendering_mode, binaural_filter_profile;
    bool element_gain_offset_flag;
    int64u rendering_config_extension_size;
    BS_Begin();
    Get_S1    (2, headphones_rendering_mode,                    "headphones_rendering_mode"); Param_Info1(Iamf_headphones_rendering_mode(headphones_rendering_mode));
    Get_SB    (element_gain_offset_flag,                        "element_gain_offset_flag");
    Get_S1    (2, binaural_filter_profile,                      "binaural_filter_profile"); Param_Info1(Iamf_binaural_filter_profile(binaural_filter_profile));
    Skip_S1   (3,                                               "reserved");
    BS_End();
    Get_leb128(rendering_config_extension_size,                 "rendering_config_extension_size");
    auto Element_Offset_Ext_Begin = Element_Offset;
    if (rendering_config_extension_size) {
        int64u num_parameters;
        Get_leb128(num_parameters,                              "num_parameters");
        if (num_parameters > Element_Size) {
            Reject();
            return;
        }
        for (int64u i = 0; i < num_parameters; ++i) {
            int64u param_definition_type;
            Get_leb128(param_definition_type,                   "param_definition_type"); Param_Info1(Iamf_parameter_definition_type(param_definition_type));
            if (param_definition_type >= PARAMETER_DEFINITION_POLAR && param_definition_type <= PARAMETER_DEFINITION_DUAL_CART_16) {
                Element_Begin0();
                    ParamDefinition(param_definition_type);
                Element_End0();
            }
            else {
                int64u rendering_config_params_extension_size;
                Get_leb128(rendering_config_params_extension_size, "rendering_config_params_extension_size");
                Skip_XX(rendering_config_params_extension_size, "rendering_config_params_extension_bytes");
            }
        }
        if (element_gain_offset_flag) {
            Element_Begin1("element_gain_offset_config");
            int8u element_gain_offset_config_type;
            int64u element_gain_offset_size;
            Get_B1(element_gain_offset_config_type,             "element_gain_offset_config_type");
            Param_Info1(element_gain_offset_config_type == 0 ? "VALUE_TYPE" : element_gain_offset_config_type == 1 ? "RANGE_TYPE" : "");
            switch (element_gain_offset_config_type) {
            case 0: // value type
                int16u element_gain_offset;
                Get_B2(element_gain_offset,                     "element_gain_offset");
                Param_Info1(static_cast<int16s>(element_gain_offset));
                Param_Info2(static_cast<double>(static_cast<int16s>(element_gain_offset)) / 256, " dB");
                break;
            case 1: // range type
                int16u default_element_gain_offset, min_element_gain_offset, max_element_gain_offset;
                Get_B2(default_element_gain_offset,             "default_element_gain_offset");
                Param_Info1(static_cast<int16s>(default_element_gain_offset));
                Param_Info2(static_cast<double>(static_cast<int16s>(default_element_gain_offset)) / 256, " dB");
                Get_B2(min_element_gain_offset,                 "min_element_gain_offset");
                Param_Info1(static_cast<int16s>(min_element_gain_offset));
                Param_Info2(static_cast<double>(static_cast<int16s>(min_element_gain_offset)) / 256, " dB");
                Get_B2(max_element_gain_offset,                 "max_element_gain_offset");
                Param_Info1(static_cast<int16s>(max_element_gain_offset));
                Param_Info2(static_cast<double>(static_cast<int16s>(max_element_gain_offset)) / 256, " dB");
                break;
            default: // extension type
                Get_leb128(element_gain_offset_size,            "element_gain_offset_size");
                Skip_XX   (element_gain_offset_size,            "element_gain_offset_bytes");
            }
            Element_End0();
        }
    }
    auto N = Element_Offset - Element_Offset_Ext_Begin;
    Skip_XX   (rendering_config_extension_size - N,             "rendering_config_extension_bytes");
}

//---------------------------------------------------------------------------
void File_Iamf::ia_parameter_block()
{
    //Parsing
    int64u parameter_id, subblock_duration;
    Get_leb128    (parameter_id,                                "parameter_id");
    if (param_definitions.find(parameter_id) == param_definitions.end()) {
        Trusted_IsNot("parameter_id is not found in any OBU_IA_Audio_Element or OBU_IA_Mix_Presentation");
        return;
    }
    auto& param_definition = param_definitions[parameter_id];
    if (param_definition.param_definition_mode) {
        Get_leb128(param_definition.duration,                   "duration");
        Get_leb128(param_definition.constant_subblock_duration, "constant_subblock_duration");
        if (param_definition.constant_subblock_duration == 0)
            Get_leb128(param_definition.num_subblocks,          "num_subblocks");
    }
    if (param_definition.constant_subblock_duration) {
        param_definition.num_subblocks = param_definition.duration / param_definition.constant_subblock_duration;
        if (param_definition.duration % param_definition.constant_subblock_duration)
            ++param_definition.num_subblocks;
    }
    if (param_definition.num_subblocks > Element_Size) {
        Reject();
        return;
    }
    for (int64u i = 0; i < param_definition.num_subblocks; ++i) {
        if (param_definition.param_definition_mode) {
            if (param_definition.constant_subblock_duration == 0)
                Get_leb128(subblock_duration,                   "subblock_duration");
        }
        switch (param_definition.param_definition_type) {
        case PARAMETER_DEFINITION_MIX_GAIN: {
            Element_Begin1("mix_gain_parameter_data");
                int64u animation_type;
                Get_leb128(animation_type,                      "animation_type"); Param_Info1(Iamf_animation_type(animation_type));
                Element_Begin1("param_data");
                    switch (animation_type) {
                    case STEP:
                        Skip_B2(                                "start_point_value");
                        break;
                    case LINEAR:
                        Skip_B2(                                "start_point_value");
                        Skip_B2(                                "end_point_value");
                        break;
                    case BEZIER:
                        Skip_B2(                                "start_point_value");
                        Skip_B2(                                "end_point_value");
                        Skip_B2(                                "control_point_value");
                        Skip_B1(                                "control_point_relative_time");

                    }
                Element_End0();
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_DEMIXING: {
            Element_Begin1("demixing_info_parameter_data");
            BS_Begin();
            Skip_S1(3,                                          "dmixp_mode");
            Skip_S1(5,                                          "reserved_for_future_use");
            BS_End();
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_RECON_GAIN: {
            Element_Begin1("recon_gain_info_parameter_data");
            for (int8u n = 0; n < num_layers; ++n) {
                if (recon_gain_is_present_flag_Vec[n]) {
                    int64u recon_gain_flags_n;
                    Get_leb128(recon_gain_flags_n,              "recon_gain_flags");
                    for (int8u j = 0; j < 12; ++j) {
                        if ((recon_gain_flags_n >> j) & 1)
                            Skip_B1(                            "recon_gain");
                    }
                }
            }
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_POLAR: {
            Element_Begin1("polar_parameter_data");
            int8u animation_type;
            Get_B1(animation_type,                              "animation_type"); Param_Info1(Iamf_animation_type(animation_type));
            BS_Begin();
            Skip_S2(9,                                          "azimuth");
            Skip_S1(8,                                          "elevation");
            Skip_S1(7,                                          "distance");
            BS_End();
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_CART_8: {
            Element_Begin1("cart8_parameter_data");
            int8u animation_type;
            Get_B1(animation_type,                              "animation_type"); Param_Info1(Iamf_animation_type(animation_type));
            Skip_B1(                                            "x");
            Skip_B1(                                            "y");
            Skip_B1(                                            "z");
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_CART_16: {
            Element_Begin1("cart16_parameter_data");
            int8u animation_type;
            Get_B1(animation_type,                              "animation_type"); Param_Info1(Iamf_animation_type(animation_type));
            Skip_B2(                                            "x");
            Skip_B2(                                            "y");
            Skip_B2(                                            "z");
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_DUAL_POLAR: {
            Element_Begin1("dual_polar_parameter_data");
            int8u animation_type;
            Get_B1(animation_type,                              "animation_type"); Param_Info1(Iamf_animation_type(animation_type));
            BS_Begin();
            Skip_S2(9,                                          "first_azimuth");
            Skip_S1(8,                                          "first_elevation");
            Skip_S1(7,                                          "first_distance");
            Skip_S2(9,                                          "second_azimuth");
            Skip_S1(8,                                          "second_elevation");
            Skip_S1(7,                                          "second_distance");
            BS_End();
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_DUAL_CART_8: {
            Element_Begin1("dual_cart8_parameter_data");
            int8u animation_type;
            Get_B1(animation_type,                              "animation_type"); Param_Info1(Iamf_animation_type(animation_type));
            Skip_B1(                                            "first_x");
            Skip_B1(                                            "first_y");
            Skip_B1(                                            "first_z");
            Skip_B1(                                            "second_x");
            Skip_B1(                                            "second_y");
            Skip_B1(                                            "second_z");
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_DUAL_CART_16: {
            Element_Begin1("dual_cart16_parameter_data");
            int8u animation_type;
            Get_B1(animation_type,                              "animation_type"); Param_Info1(Iamf_animation_type(animation_type));
            Skip_B2(                                            "first_x");
            Skip_B2(                                            "first_y");
            Skip_B2(                                            "first_z");
            Skip_B2(                                            "second_x");
            Skip_B2(                                            "second_y");
            Skip_B2(                                            "second_z");
            Element_End0();
            break;
        }
        case PARAMETER_DEFINITION_MOMENTARY_LOUDNESS: {
            Element_Begin1("momentary_loudness_parameter_data");
            BS_Begin();
            Skip_S1(6,                                          "momentary_loudness");
            Skip_S1(2,                                          "reserved");
            BS_End();
            Element_End0();
            break;
        }
        default: {
            int64u parameter_data_size;
            Get_leb128(parameter_data_size,                     "parameter_data_size");
            Skip_XX   (parameter_data_size,                     "parameter_data_bytes");
            break;
        }
        }
    }

    //Filling
    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Iamf::ia_temporal_delimiter() {
    // Temporal Delimiter OBU has an empty payload.
    FILLING_BEGIN_PRECISE();
    FILLING_END();
};

//---------------------------------------------------------------------------
void File_Iamf::ia_audio_frame(bool audio_substream_id_in_bitstream)
{
    //Parsing
    int64u substream_id{};
    if (audio_substream_id_in_bitstream)
        Get_leb128(substream_id,                                "explicit_audio_substream_id");
    else
        substream_id = Element_Code - 6;

    Element_Begin1("audio_frame");
        #if MEDIAINFO_TRACE
            const auto& codec_id = codecs[substreams[substream_id]];
            if (codec_id && Trace_Activated) {
                switch (codec_id) {
                    case CodecIDs::Opus: {
                    #if defined(MEDIAINFO_OPUS_YES)
                    if (Parser_Opus)
                        Open_Buffer_Continue(Parser_Opus.get());
                    #endif
                    break;
                }
                case CodecIDs::fLaC:
                    #if defined(MEDIAINFO_FLAC_YES)
                    if (Parser_Flac)
                        Open_Buffer_Continue(Parser_Flac.get());
                    #endif
                    break;
                default:
                    Skip_XX(Element_Size - Element_Offset,      "Data");
                    break;
                }
            }
            else
        #endif // MEDIAINFO_TRACE
                Skip_XX(Element_Size - Element_Offset,          "Data");
    Element_End0();

    //Filling
    FILLING_BEGIN_PRECISE();
        ++Frame_Count;
        if (Config->ParseSpeed < 1) {
            if (!Frame_Count_Valid)
                Frame_Count_Valid = substreams.size();
            if (Frame_Count > Frame_Count_Valid)
                Finish();
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Iamf::ia_metadata()
{
    //Parsing
    int64u metadata_type{};
    Get_leb128(metadata_type,                                   "metadata_type");

    switch (metadata_type)
    {
    case 1: Param_Info1("METADATA_TYPE_ITUT_T35"); metadata_itu_t_t35(); break;
    case 2: Param_Info1("METADATA_TYPE_IAMF_TAGS"); metadata_iamf_tags(); break;
    default: Skip_XX(Element_Size - Element_Offset,             "Data");
    }
}

//---------------------------------------------------------------------------
void File_Iamf::metadata_itu_t_t35()
{
    // TODO: Use ITUT T35 parser
    Skip_XX(Element_Size - Element_Offset,                      "Data");
}

//---------------------------------------------------------------------------
void File_Iamf::metadata_iamf_tags()
{
    int8u num_tags;
    Get_B1(num_tags,                                            "num_tags");

    ZtringListList tags;
    for (int8u i = 0; i < num_tags; ++i) {
        Element_Begin1("tag");
        Ztring tag_name, tag_value;
        Get_UTF8(SizeUpTo0(), tag_name,                         "tag_name");
        Skip_B1("zero");
        Get_UTF8(SizeUpTo0(), tag_value,                        "tag_value");
        Skip_B1("zero");
        tags(i, 0) = tag_name;
        tags(i, 1) = tag_value;
        Element_End0();
    }

    FILLING_BEGIN_PRECISE();
    for (int8u i = 0; i < tags.size(); ++i) {
        Fill(IsSub ? Stream_Audio : Stream_General, 0, tags(i, 0).To_UTF8().c_str(), tags(i, 1));
    }
    FILLING_END();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iamf::Get_leb128(int64u& Info, const char* Name)
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

} // namespace

#endif // defined(MEDIAINFO_IAMF_YES)
