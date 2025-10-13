/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Immersive Audio Model and Formats (IAMF) files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_Iamf
#define MediaInfo_Iamf
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <memory>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Iamf
//***************************************************************************

class File_Iamf : public File__Analyze
{
public:
    // Constructor/Destructor
    File_Iamf();
    ~File_Iamf() override;

private:
    // Streams management
    void Streams_Accept() override;
    void Streams_Finish() override;

    //Buffer - File header
    bool FileHeader_Begin() override;

    // Buffer - Global
    void Read_Buffer_OutOfBand() override;

    // Buffer - Per element
    void Header_Parse() override;
    void Data_Parse() override;

    // Elements
    void ia_codec_config();
    void ia_audio_element();
    void ia_mix_presentation();
    void ia_sequence_header();
    void ParamDefinition(int64u param_definition_type);
    void ia_parameter_block();
    void ia_temporal_delimiter() {}; // Temporal Delimiter OBU has an empty payload.
    void ia_audio_frame(bool audio_substream_id_in_bitstream);

    //Temp
    int64u Frame_Count_Valid{};
    std::unique_ptr<File__Analyze> Parser_Opus;
    std::unique_ptr<File__Analyze> Parser_Flac;
    struct ParamDefinitionData{
        int64u param_definition_type{};
        int8u  param_definition_mode{};
        int64u duration{};
        int64u constant_subblock_duration{};
        int64u num_subblocks{};
    };
    std::map<int64u, ParamDefinitionData> param_definitions; // parameter_id -> ParamDefinitionData
    int8u num_layers{};
    int8u codec_config_count{};
    std::vector<bool> recon_gain_is_present_flag_Vec;
    std::map<int64u, int32u> codecs;            // codec_config_id  -> codec_id
    std::map<int64u, int64u> substreams;        // audio_substream_id -> codec_config_id
    std::map<int64u, int64u> mixpresentations;  // mix_presentation_id -> num_sub_mixes

    // Helpers
    void Get_leb128(int64u& Info, const char* Name);
};

} //NameSpace

#endif // !MediaInfo_Iamf
