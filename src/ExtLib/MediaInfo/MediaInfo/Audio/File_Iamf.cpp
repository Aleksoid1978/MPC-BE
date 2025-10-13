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

// param_definition_type
#define PARAMETER_DEFINITION_MIX_GAIN   0
#define PARAMETER_DEFINITION_DEMIXING   1
#define PARAMETER_DEFINITION_RECON_GAIN 2

// ambisonics_mode
#define MONO        0
#define PROJECTION  1

// layout_type
#define LOUDSPEAKERS_SS_CONVENTION  2
#define BINAURAL                    3

// animation_type
#define STEP    0
#define LINEAR  1
#define BEZIER  2

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
    //  24~30   : Reserved for future use
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
static string Iamf_animation_type(int64u animation_type)
{
    switch (animation_type)
    {
    case STEP   : return "STEP";
    case LINEAR : return "LINEAR";
    case BEZIER : return "BEZIER";
    default: return std::to_string(animation_type);
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Iamf::File_Iamf()
    :File__Analyze()
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

    // Currently multiple ia_codec_config are not well handled //TODO: how to manage multiple ia_codec_config or if it changes midstream?
    if (!IsSub && Config->ParseSpeed >= 1 && Frame_Count && substreams.size() && codec_config_count == 1) {
        int32u SamplingRate = Retrieve_Const(Stream_Audio, 0, Audio_SamplingRate).To_int32u();
        int32u SamplesPerFrame = Retrieve_Const(Stream_Audio, 0, Audio_SamplesPerFrame).To_int32u();
        if (SamplingRate && SamplesPerFrame)
        {
            auto SamplesCountPerSubstream{ SamplesPerFrame * Frame_Count / substreams.size() };
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
     && ((Buffer[0] >> 3) != 0x1F // ia_sequence_header
      || CC4(Buffer + 2) != 0x69616D66)) {  // "iamf"
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
    bool obu_redundant_copy, obu_trimming_status_flag, obu_extension_flag;
    int64u obu_size, num_samples_to_trim_at_end, num_samples_to_trim_at_start, extension_header_size;
    BS_Begin();
    Get_S1      (5,     obu_type,                               "obu_type");
    Get_SB      (       obu_redundant_copy,                     "obu_redundant_copy");
    Get_SB      (       obu_trimming_status_flag,               "obu_trimming_status_flag");
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
        Get_B2 (        audio_roll_distance,                    "audio_roll_distance"); Param_Info1(reinterpret_cast<int16_t&>(audio_roll_distance));
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
                    Get_B2(output_gain,                     "output_gain"); Param_Info1(reinterpret_cast<int16_t&>(output_gain));
                }
                if (num_layers == 1 && loudspeaker_layout == 15)
                    Skip_B1(                                "expanded_loudspeaker_layout");
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
                    Skip_XX(output_channel_count,           "channel_mapping");
                Element_End0();
            }
            else if (ambisonics_mode == PROJECTION) {
                Element_Begin1("ambisonics_projection_config");
                    int8u output_channel_count, substream_count, coupled_substream_count;
                    Get_B1(output_channel_count,            "output_channel_count");
                    Get_B1(substream_count,                 "substream_count");
                    Get_B1(coupled_substream_count,         "coupled_substream_count");
                    Skip_XX(2*static_cast<int64u>(substream_count+coupled_substream_count)*output_channel_count, "demixing_matrix");
                Element_End0();
            }
        Element_End0();
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
        Skip_String(SizeUpTo0(),                            "annotations_language");
        Skip_B1(                                            "zero");
    }
    for (int64u i = 0; i < count_label; ++i) {
        Skip_String(SizeUpTo0(),                            "localized_presentation_annotations");
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
                Skip_String(SizeUpTo0(),                    "localized_element_annotations");
                Skip_B1(                                    "zero");
            }
            Element_Begin1("rendering_config");
                int8u headphones_rendering_mode;
                int64u rendering_config_extension_size;
                BS_Begin();
                Get_S1  (2, headphones_rendering_mode,      "headphones_rendering_mode");  Param_Info1(!headphones_rendering_mode?"Stereo":headphones_rendering_mode==1?"Binaural":"Reserved");
                Skip_S1 (6,                                 "reserved_for_future_use");
                BS_End();
                Get_leb128(rendering_config_extension_size, "rendering_config_extension_size");
                Skip_XX (  rendering_config_extension_size, "rendering_config_extension_bytes");
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
                Get_B1( info_type,                          "info_type");
                Get_B2( integrated_loudness,                "integrated_loudness"); Param_Info1(reinterpret_cast<int16_t&>(integrated_loudness));
                Get_B2( digital_peak,                       "digital_peak"); Param_Info1(reinterpret_cast<int16_t&>(digital_peak));
                if (info_type & 1) {
                    Skip_B2(                                "true_peak");
                }
                if (info_type & 2) {
                    int8u num_anchored_loudness;
                    Get_B1(num_anchored_loudness,           "num_anchored_loudness");
                    for (int8u n = 0; n < num_anchored_loudness; ++n) {
                        int16u anchored_loudness;
                        Skip_B1 (                           "anchor_element");
                        Get_B2  (anchored_loudness,         "anchored_loudness"); Param_Info1(reinterpret_cast<int16_t&>(anchored_loudness));
                    }
                }
                if ((info_type & 0b11111100) > 0) {
                    int64u info_type_size;
                    Get_leb128  (info_type_size,            "info_type_size");
                    Skip_XX     (info_type_size,            "info_type_bytes");
                }
            Element_End0();
        }
    }
    if (Element_Size > Element_Offset) {
        Element_Begin1("mix_presentation_tags");
            int8u num_tags;
            Get_B1(num_tags,                                "num_tags");
            for (int8u i = 0; i < num_tags; ++i) {
                Skip_String(SizeUpTo0(),                    "tag_name");
                Skip_B1("zero");
                Skip_String(SizeUpTo0(),                    "tag_value");
                Skip_B1("zero");
            }
        Element_End0();
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
        Get_B2(         default_mix_gain,                   "default_mix_gain"); Param_Info1(reinterpret_cast<int16_t&>(default_mix_gain));
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
        if (param_definition.param_definition_type == PARAMETER_DEFINITION_MIX_GAIN) {
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
        }
        else if (param_definition.param_definition_type == PARAMETER_DEFINITION_DEMIXING) {
            Element_Begin1("demixing_info_parameter_data");
                BS_Begin();
                Skip_S1(3,                                      "dmixp_mode");
                Skip_S1(5,                                      "reserved_for_future_use");
                BS_End();
            Element_End0();
        }
        else if (param_definition.param_definition_type == PARAMETER_DEFINITION_RECON_GAIN) {
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
        }
        else {
            int64u parameter_data_size;
            Get_leb128(parameter_data_size,                     "parameter_data_size");
            Skip_XX   (parameter_data_size,                     "parameter_data_bytes");
        }
    }

    //Filling
    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

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
