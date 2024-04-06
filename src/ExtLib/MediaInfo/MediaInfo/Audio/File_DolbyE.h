/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Dolby E files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_DolbyEH
#define MediaInfo_File_DolbyEH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_DolbyE
//***************************************************************************

class File_DolbyE : public File__Analyze
{
public :
    //In
    int64s GuardBand_Before;
    
    //Out
    int64s GuardBand_After;

    //Constructor/Destructor
    File_DolbyE();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Fill_PerProgram(size_t program);
    void Streams_Finish();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();

    //Buffer - Global
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void sync_segment();
    void metadata_segment();
    void audio_segment();
    void metadata_extension_segment();
    void audio_extension_segment();
    void meter_segment();
    void ac3_metadata_subsegment(bool xbsi);

    //Evolution
    void guard_band();
    void intelligent_loudness_evolution_data_segment();
    void evo_frame();
    void evo_payload_config();
    void evo_protection();
    void object_audio_metadata_payload();
    void program_assignment();
    void oa_element_md(bool b_alternate_object_data_present);
    void object_element();
    void md_update_info(int8u& num_obj_info_blocks_bits);
    void block_update_info();
    void object_data(int8u obj_idx, int8u num_obj_info_blocks_bits);
    void object_info_block(int8u obj_idx, int8u blk);
    void object_basic_info(int8u object_basic_info_array, int8u blk);
    void object_render_info(int8u object_render_info_status_idx, int8u blk);
    void Get_V4 (int8u  Bits, int32u& Info, const char* Name);
    void Skip_V4(int8u  Bits, const char* Name);
    void Get_V4 (int8u  Bits, int8u MaxLoops, int32u& Info, const char* Name);
    int32u Guardband_EMDF_PresentAndSize;
    int8u object_count;
    vector<bool> b_object_in_bed_or_isf;
    vector<int32u> nonstd_bed_channel_assignment_masks;
    int32u num_desc_packets_m1;
    vector<uint8_t> description_packet_data;
    void mgi_payload();
    void Streams_Fill_ED2();
    struct dyn_object
    {
        int8u sound_category;
        struct dyn_object_alt
        {
            int8u pos3d_x_bits;
            int8u pos3d_y_bits;
            bool  pos3d_z_sig;
            int8u pos3d_z_bits;
            int8s obj_gain_db; // In dB, INT8_MAX = Not available, INT8_MIN = Infinite
            int8u hp_render_mode;
        };
        vector<dyn_object_alt> Alts;
    };
    vector<dyn_object> ObjectElements; // OAMD
    vector<dyn_object> DynObjects;
    struct bed_instance
    {
        struct bed_alt
        {
            int8s bed_gain_db; // In dB, INT8_MAX = Not available, INT8_MIN = Infinite
        };
        vector<bed_alt> Alts;
    };
    vector<bed_instance> BedInstances;
    struct preset
    {
        int32u default_target_device_config;
        struct target_device_config
        {
            int32u target_devices_mask;
            vector<int32u> md_indexes;
        };
        vector<target_device_config> target_device_configs;
    };
    vector<preset> Presets;
    struct preset_more
    {
        string description;
    };
    vector<preset_more> Presets_More;
    struct substream_mapping
    {
        int8u substream_id;
        int32u channel_index;
    };
    vector<substream_mapping> substream_mappings;


    //Helpers
    void Descramble_20bit(int32u key, int16u size);

    //Temp
    int64u  SMPTE_time_code_StartTimecode;
    int16u  channel_subsegment_size[8];
    int8u   program_config;
    int8u   metadata_extension_segment_size;
    int8u   meter_segment_size;
    int8u   frame_rate_code;
    int8u   bit_depth;
    bool    key_present;
    int8u*  Descrambled_Buffer; //Used in case of key_present
    std::map<int64u, int64u> FrameSizes;
    std::map<int16u, int64u> channel_subsegment_sizes[8];
    int64u  GuardBand_Before_Initial;
    int64u  GuardBand_After_Initial;
    struct description_text_value
    {
        string Previous;
        string Current;
    };
    vector<description_text_value> description_text_Values;
};

} //NameSpace

#endif
