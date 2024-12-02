/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_HevcH
#define MediaInfo_HevcH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/TimeCode.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Hevc
//***************************************************************************

class File_Hevc : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    bool   MustParse_VPS_SPS_PPS;
    bool   MustParse_VPS_SPS_PPS_FromMatroska;
    bool   MustParse_VPS_SPS_PPS_FromFlv;
    bool   MustParse_VPS_SPS_PPS_FromLhvc;
    bool   SizedBlocks;
    size_t SizedBlocks_FileThenStream;

    //Constructor/Destructor
    File_Hevc();
    ~File_Hevc();

private :
    File_Hevc(const File_Hevc &File_Hevc); //No copy
    File_Hevc &operator =(const File_Hevc &); //No copy

    struct profile_tier_level_struct {
        int8u   profile_space;
        int8u   profile_idc;
        int8u   level_idc;
        bool    tier_flag;
        bool    general_progressive_source_flag;
        bool    general_interlaced_source_flag;
        bool    general_frame_only_constraint_flag;
        bool    general_max_8bit_constraint_flag;
        bool    general_max_10bit_constraint_flag;
        bool    general_max_12bit_constraint_flag;
        bool    general_max_14bit_constraint_flag;

        profile_tier_level_struct& Clear()
        {
            profile_space = -1;
            profile_idc = -1;
            level_idc = -1;
            tier_flag = true;
            general_progressive_source_flag = true;
            general_interlaced_source_flag = true;
            general_frame_only_constraint_flag = true;
            general_max_8bit_constraint_flag = true;
            general_max_10bit_constraint_flag = true;
            general_max_12bit_constraint_flag = true;
            general_max_14bit_constraint_flag = true;
            return *this;
        }
    };

    //Structures - video_parameter_set
    struct video_parameter_set_struct
    {
        #if MEDIAINFO_DEMUX
        int8u*  AnnexB_Buffer;
        size_t  AnnexB_Buffer_Size;
        #endif //MEDIAINFO_DEMUX
        vector<profile_tier_level_struct> profile_tier_level_info_layers;
        vector<int16u> view_id_val;
        int8u   vps_max_sub_layers_minus1;

        //Constructor/Destructor
        video_parameter_set_struct(const vector<profile_tier_level_struct>& profile_tier_level_info_layers_, int8u vps_max_sub_layers_minus1_, vector<int16u> view_id_val_)
        :
            #if MEDIAINFO_DEMUX
            AnnexB_Buffer(NULL),
            AnnexB_Buffer_Size(0),
            #endif //MEDIAINFO_DEMUX
            profile_tier_level_info_layers(profile_tier_level_info_layers_),
            view_id_val(view_id_val_),
            vps_max_sub_layers_minus1(vps_max_sub_layers_minus1_)
        {
        }

        ~video_parameter_set_struct()
        {
            #if MEDIAINFO_DEMUX
                delete[] AnnexB_Buffer;
            #endif //MEDIAINFO_DEMUX
        }

    private:
        video_parameter_set_struct &operator=(const video_parameter_set_struct &v);
        video_parameter_set_struct(const video_parameter_set_struct &a);
        video_parameter_set_struct();
    };
    typedef vector<video_parameter_set_struct*> video_parameter_set_structs;

    //Structures - seq_parameter_set
    enum vui_flag
    {
        video_signal_type_present_flag,
        video_full_range_flag,
        colour_description_present_flag,
        pic_struct_present_flag,
        frame_field_info_present_flag,
        vui_flags_Max
    };
    typedef std::bitset<vui_flags_Max> vui_flags;
    struct seq_parameter_set_struct
    {
        struct vui_parameters_struct
        {
            struct xxl
            {
                struct xxl_data
                {
                    //HRD configuration
                    int64u bit_rate_value;
                    int64u cpb_size_value;
                    bool   cbr_flag;

                    //sei_message_buffering_period
                    //int32u initial_cpb_removal_delay;
                    //int32u initial_cpb_removal_delay_offset;

                    xxl_data(int64u bit_rate_value_, int64u cpb_size_value_, bool cbr_flag_) //int32u initial_cpb_removal_delay_, int32u initial_cpb_removal_delay_offset_)
                        :
                        bit_rate_value(bit_rate_value_),
                        cpb_size_value(cpb_size_value_),
                        cbr_flag(cbr_flag_)
                        //initial_cpb_removal_delay(initial_cpb_removal_delay_),
                        //initial_cpb_removal_delay_offset(initial_cpb_removal_delay_offset_)
                    {
                    }

                private:
                    xxl_data();
                };
                vector<xxl_data> SchedSel;

                xxl(const vector<xxl_data> &SchedSel_)
                    :
                    SchedSel(SchedSel_)
                {
                }
            private:
                xxl &operator=(const xxl &x);
                xxl(const xxl &);
                xxl();
            };
            struct xxl_common
            {
                //Common to all xxl
                bool    sub_pic_hrd_params_present_flag;
                int8u   du_cpb_removal_delay_increment_length_minus1;
                int8u   dpb_output_delay_du_length_minus1;
                int8u   initial_cpb_removal_delay_length_minus1;
                int8u   au_cpb_removal_delay_length_minus1;
                int8u   dpb_output_delay_length_minus1;

                xxl_common(bool sub_pic_hrd_params_present_flag_, int8u du_cpb_removal_delay_increment_length_minus1_, int8u dpb_output_delay_du_length_minus1_, int8u initial_cpb_removal_delay_length_minus1_, int8u au_cpb_removal_delay_length_minus1_, int8u dpb_output_delay_length_minus1_)
                    :
                    sub_pic_hrd_params_present_flag(sub_pic_hrd_params_present_flag_),
                    du_cpb_removal_delay_increment_length_minus1(du_cpb_removal_delay_increment_length_minus1_),
                    dpb_output_delay_du_length_minus1(dpb_output_delay_du_length_minus1_),
                    initial_cpb_removal_delay_length_minus1(initial_cpb_removal_delay_length_minus1_),
                    au_cpb_removal_delay_length_minus1(au_cpb_removal_delay_length_minus1_),
                    dpb_output_delay_length_minus1(dpb_output_delay_length_minus1_)
                {
                }

            private:
                xxl_common();
            };
            xxl*    NAL;
            xxl*    VCL;
            xxl_common* xxL_Common;
            int32u  num_units_in_tick;
            int32u  time_scale;
            int16u  sar_width;
            int16u  sar_height;
            int8u   video_format;
            int8u   colour_primaries;
            int8u   transfer_characteristics;
            int8u   matrix_coefficients;
            vui_flags flags;

            vui_parameters_struct(xxl* NAL_, xxl* VCL_, xxl_common* xxL_Common_, int32u num_units_in_tick_, int32u time_scale_, int16u sar_width_, int16u sar_height_, int8u video_format_, int8u colour_primaries_, int8u transfer_characteristics_, int8u matrix_coefficients_, vui_flags flags_)
                :
                NAL(NAL_),
                VCL(VCL_),
                xxL_Common(xxL_Common_),
                num_units_in_tick(num_units_in_tick_),
                time_scale(time_scale_),
                sar_width(sar_width_),
                sar_height(sar_height_),
                video_format(video_format_),
                colour_primaries(colour_primaries_),
                transfer_characteristics(transfer_characteristics_),
                matrix_coefficients(matrix_coefficients_),
                flags(flags_)
            {
            }

            ~vui_parameters_struct()
            {
                delete NAL; //NAL=NULL;
                delete VCL; //VCL=NULL;
                delete xxL_Common; //xxL_Common=NULL
            }

        private:
            vui_parameters_struct &operator=(const vui_parameters_struct &v);
            vui_parameters_struct(const vui_parameters_struct &);
            vui_parameters_struct();
        };
        int32u  nuh_layer_id;
        vui_parameters_struct* vui_parameters;
        #if MEDIAINFO_DEMUX
        int8u*  AnnexB_Buffer;
        size_t  AnnexB_Buffer_Size;
        #endif //MEDIAINFO_DEMUX
        profile_tier_level_struct profile_tier_level_info;
        int32u  pic_width_in_luma_samples;
        int32u  pic_height_in_luma_samples;
        int32u  conf_win_left_offset;
        int32u  conf_win_right_offset;
        int32u  conf_win_top_offset;
        int32u  conf_win_bottom_offset;
        int8u   video_parameter_set_id;
        int8u   chroma_format_idc;
        bool    separate_colour_plane_flag;
        int8u   log2_max_pic_order_cnt_lsb_minus4;
        int8u   bit_depth_luma_minus8;
        int8u   bit_depth_chroma_minus8;
        int8u   sps_max_num_reorder_pics;

        //Computed value
        bool    NalHrdBpPresentFlag() {return vui_parameters && vui_parameters->NAL;}
        bool    VclHrdBpPresentFlag() {return vui_parameters && vui_parameters->VCL;}
        bool    CpbDpbDelaysPresentFlag() {return vui_parameters && vui_parameters->xxL_Common;} //xxL_Common is present if NAL or VCL
        int8u   ChromaArrayType() {return separate_colour_plane_flag?0:chroma_format_idc;}

        //Constructor/Destructor
        seq_parameter_set_struct(int32u nuh_layer_id_, vui_parameters_struct* vui_parameters_, const profile_tier_level_struct& profile_tier_level_info_, int32u pic_width_in_luma_samples_, int32u pic_height_in_luma_samples_, int32u conf_win_left_offset_, int32u conf_win_right_offset_, int32u conf_win_top_offset_, int32u conf_win_bottom_offset_, int8u video_parameter_set_id_, int8u chroma_format_idc_, bool separate_colour_plane_flag_, int8u log2_max_pic_order_cnt_lsb_minus4_, int8u bit_depth_luma_minus8_, int8u bit_depth_chroma_minus8_, int8u sps_max_num_reorder_pics_)
            :
            nuh_layer_id(nuh_layer_id_),
            vui_parameters(vui_parameters_),
            #if MEDIAINFO_DEMUX
            AnnexB_Buffer(NULL),
            AnnexB_Buffer_Size(0),
            #endif //MEDIAINFO_DEMUX
            profile_tier_level_info(profile_tier_level_info_),
            pic_width_in_luma_samples(pic_width_in_luma_samples_),
            pic_height_in_luma_samples(pic_height_in_luma_samples_),
            conf_win_left_offset(conf_win_left_offset_),
            conf_win_right_offset(conf_win_right_offset_),
            conf_win_top_offset(conf_win_top_offset_),
            conf_win_bottom_offset(conf_win_bottom_offset_),
            video_parameter_set_id(video_parameter_set_id_),
            chroma_format_idc(chroma_format_idc_),
            separate_colour_plane_flag(separate_colour_plane_flag_),
            log2_max_pic_order_cnt_lsb_minus4(log2_max_pic_order_cnt_lsb_minus4_),
            bit_depth_luma_minus8(bit_depth_luma_minus8_),
            bit_depth_chroma_minus8(bit_depth_chroma_minus8_),
            sps_max_num_reorder_pics(sps_max_num_reorder_pics_)
        {
        }

        ~seq_parameter_set_struct()
        {
            delete vui_parameters; //vui_parameters=NULL;
            #if MEDIAINFO_DEMUX
            delete[] AnnexB_Buffer;
            #endif //MEDIAINFO_DEMUX
        }

    private:
        seq_parameter_set_struct &operator=(const seq_parameter_set_struct &v);
        seq_parameter_set_struct(const seq_parameter_set_struct &);
        seq_parameter_set_struct();
    };
    typedef vector<seq_parameter_set_struct*> seq_parameter_set_structs;

    //Structures - pic_parameter_set
    struct pic_parameter_set_struct
    {
        #if MEDIAINFO_DEMUX
        int8u*  AnnexB_Buffer;
        size_t  AnnexB_Buffer_Size;
        #endif //MEDIAINFO_DEMUX
        int8u   seq_parameter_set_id;
        int8u   num_ref_idx_l0_default_active_minus1;
        int8u   num_ref_idx_l1_default_active_minus1;
        int8u   num_extra_slice_header_bits;
        bool    dependent_slice_segments_enabled_flag;

        //Constructor/Destructor
        pic_parameter_set_struct(int8u seq_parameter_set_id_, int8u num_ref_idx_l0_default_active_minus1_, int8u num_ref_idx_l1_default_active_minus1_, int8u num_extra_slice_header_bits_, bool dependent_slice_segments_enabled_flag_)
            :
            #if MEDIAINFO_DEMUX
            AnnexB_Buffer(NULL),
            AnnexB_Buffer_Size(0),
            #endif //MEDIAINFO_DEMUX
            seq_parameter_set_id(seq_parameter_set_id_),
            num_ref_idx_l0_default_active_minus1(num_ref_idx_l0_default_active_minus1_),
            num_ref_idx_l1_default_active_minus1(num_ref_idx_l1_default_active_minus1_),
            num_extra_slice_header_bits(num_extra_slice_header_bits_),
            dependent_slice_segments_enabled_flag(dependent_slice_segments_enabled_flag_)
        {
        }

        ~pic_parameter_set_struct()
        {
            #if MEDIAINFO_DEMUX
                delete[] AnnexB_Buffer;
            #endif //MEDIAINFO_DEMUX
        }

    private:
        pic_parameter_set_struct &operator=(const pic_parameter_set_struct &v);
        pic_parameter_set_struct(const pic_parameter_set_struct &a);
        pic_parameter_set_struct();
    };
    typedef vector<pic_parameter_set_struct*> pic_parameter_set_structs;

    //Streams management
    void Streams_Fill();
    void Streams_Fill_Profile(const profile_tier_level_struct& p);
    void Streams_Fill(vector<video_parameter_set_struct*>::iterator video_parameter_set_Item);
    void Streams_Fill(vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item);
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();
    void Synched_Init();

    //Buffer - Demux
    #if MEDIAINFO_DEMUX
    bool Demux_UnpacketizeContainer_Test();
    bool Demux_Transcode_Iso14496_15_to_AnnexB;
    #endif //MEDIAINFO_DEMUX

    //Buffer - Global
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    void Header_Parse();
    bool Header_Parser_QuickSearch();
    bool Header_Parser_Fill_Size();
    void Data_Parse();

    //Elements
    void slice_segment_layer();
    void video_parameter_set();
    void video_parameter_sets_creating_data(int8u vps_video_parameter_set_id, const vector<profile_tier_level_struct>& profile_tier_level_info_layers, int8u vps_max_sub_layers_minus1, const vector<int16u>& view_id_val);
    void seq_parameter_set();
    void pic_parameter_set();
    void access_unit_delimiter();
    void end_of_seq();
    void end_of_bitstream();
    void filler_data();
    void sei();
    void sei_message(int32u &seq_parameter_set_id);
    void sei_message_buffering_period(int32u &seq_parameter_set_id, int32u payloadSize);
    void sei_message_buffering_period_xxl(seq_parameter_set_struct::vui_parameters_struct::xxl_common* xxL_Common, bool irap_cpb_params_present_flag, seq_parameter_set_struct::vui_parameters_struct::xxl* xxl);
    void sei_message_pic_timing(int32u &seq_parameter_set_id, int32u payloadSize);
    void sei_message_user_data_registered_itu_t_t35();
    void sei_message_user_data_registered_itu_t_t35_B5();
    void sei_message_user_data_registered_itu_t_t35_B5_0031();
    void sei_message_user_data_registered_itu_t_t35_B5_0031_DTG1();
    void sei_message_user_data_registered_itu_t_t35_B5_0031_GA94();
    void sei_message_user_data_registered_itu_t_t35_B5_0031_GA94_03();
    void sei_message_user_data_registered_itu_t_t35_B5_0031_GA94_03_Delayed(int32u seq_parameter_set_id);
    void sei_message_user_data_registered_itu_t_t35_B5_0031_GA94_09();
    void sei_message_user_data_registered_itu_t_t35_B5_003A();
    void sei_message_user_data_registered_itu_t_t35_B5_003A_00();
    void sei_message_user_data_registered_itu_t_t35_B5_003A_02();
    void sei_message_user_data_registered_itu_t_t35_B5_003C();
    void sei_message_user_data_registered_itu_t_t35_B5_003C_0001();
    void sei_message_user_data_registered_itu_t_t35_B5_003C_0001_04();
    void sei_message_user_data_registered_itu_t_t35_26();
    void sei_message_user_data_registered_itu_t_t35_26_0004();
    void sei_message_user_data_registered_itu_t_t35_26_0004_0005();
    void sei_message_user_data_unregistered(int32u payloadSize);
    void sei_message_user_data_unregistered_Ateme(int32u payloadSize);
    void sei_message_user_data_unregistered_x265(int32u payloadSize);
    void sei_message_recovery_point();
    void sei_message_active_parameter_sets();
    void sei_time_code();
    void sei_message_decoded_picture_hash(int32u payloadSize);
    void sei_message_mastering_display_colour_volume();
    void sei_message_light_level();
    void sei_alternative_transfer_characteristics();
    void three_dimensional_reference_displays_info(int32u payloadSize);

    //Packets - SubElements
    void slice_segment_header();
    void profile_tier_level(profile_tier_level_struct& p, bool profilePresentFlag, int8u maxNumSubLayersMinus1);
    void short_term_ref_pic_sets(int8u num_short_term_ref_pic_sets);
    void vui_parameters(seq_parameter_set_struct::vui_parameters_struct* &vui_parameters_Item, int8u maxNumSubLayersMinus1);
    void hrd_parameters(bool commonInfPresentFlag, int8u maxNumSubLayersMinus1, seq_parameter_set_struct::vui_parameters_struct::xxl_common* &xxL_Common, seq_parameter_set_struct::vui_parameters_struct::xxl* &NAL, seq_parameter_set_struct::vui_parameters_struct::xxl* &VCL);
    void sub_layer_hrd_parameters(seq_parameter_set_struct::vui_parameters_struct::xxl_common* xxL_Common, int8u bit_rate_scale, int8u cpb_size_scale, int32u cpb_cnt_minus1, seq_parameter_set_struct::vui_parameters_struct::xxl* &hrd_parameters_Item);
    void scaling_list_data();

    //Packets - Specific
    void VPS_SPS_PPS();
    void VPS_SPS_PPS_FromMatroska();
    void EndOfxPS(const char* FlagName, const char* DataName);
    void rbsp_trailing_bits();
    size_t RiskCalculationN;
    size_t RiskCalculationD;

    vector<stream_payload> Streams;

    //Temporal references
    struct temporal_reference
    {
        #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
            buffer_data* GA94_03;
        #endif //MEDIAINFO_DTVCCTRANSPORT_YES

        int32u frame_num;
        int8u  slice_type;
        bool   IsTop;
        bool   IsField;

        temporal_reference()
        {
            #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
                GA94_03=NULL;
            #endif //MEDIAINFO_DTVCCTRANSPORT_YES
            slice_type=(int8u)-1;
        }

        ~temporal_reference()
        {
            #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
                delete GA94_03; //GA94_03=NULL;
            #endif //MEDIAINFO_DTVCCTRANSPORT_YES
        }
    };
    typedef vector<temporal_reference*> temporal_references;
    temporal_references                 TemporalReferences; //per pic_order_cnt_lsb
    void Clean_Temp_References();
    temporal_reference*                 TemporalReferences_DelayedElement;
    size_t                              TemporalReferences_Min;
    size_t                              TemporalReferences_Max;
    size_t                              TemporalReferences_Reserved;
    size_t                              TemporalReferences_Offset;
    size_t                              TemporalReferences_Offset_pic_order_cnt_lsb_Last;
    int64s                              TemporalReferences_pic_order_cnt_Min;
    int64u                              pic_order_cnt_DTS_Ref;

    //Text
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        File__Analyze*                  GA94_03_Parser;
        bool                            GA94_03_IsPresent;
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)

    //Misc
    TimeCode                            TC_Current;

    //Replacement of File__Analyze buffer
    const int8u*                        Buffer_ToSave;
    size_t                              Buffer_Size_ToSave;

    //parameter_sets
    video_parameter_set_structs         video_parameter_sets;
    seq_parameter_set_structs           seq_parameter_sets;
    pic_parameter_set_structs           pic_parameter_sets;


    void Clean_Seq_Parameter();

    //File specific
    int8u                               lengthSizeMinusOne;

    //File specific
    size_t                              IFrame_Count;

    //Temp
    Ztring                              Encoded_Library;
    Ztring                              Encoded_Library_Name;
    Ztring                              Encoded_Library_Version;
    Ztring                              Encoded_Library_Date;
    Ztring                              Encoded_Library_Settings;
    enum hdr_format
    {
        HdrFormat_SmpteSt209410,
        HdrFormat_SmpteSt209440,
        HdrFormat_EtsiTs103433,
        HdrFormat_HdrVivid,
        HdrFormat_SmpteSt2086,
        HdrFormat_Max,
    };
    typedef std::map<video, Ztring[HdrFormat_Max]> hdr;
    hdr                                 HDR;
    Ztring                              EtsiTS103433;
    int32u  chroma_format_idc;
    int32u  slice_pic_parameter_set_id;
    int32u  slice_type;
    int32u  chroma_sample_loc_type_top_field;
    int32u  chroma_sample_loc_type_bottom_field;
    Ztring  maximum_content_light_level;
    Ztring  maximum_frame_average_light_level;
    int8u   nuh_layer_id;
    int8u   preferred_transfer_characteristics;
    bool    RapPicFlag;
    bool    first_slice_segment_in_pic_flag;
};

} //NameSpace

#endif
