/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_AvcH
#define MediaInfo_AvcH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__Duplicate.h"
#include "MediaInfo/TimeCode.h"
#include <bitset>
#include <cmath>

//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Avc
//***************************************************************************

class File_Avc :
#if MEDIAINFO_DUPLICATE
    public File__Duplicate
#else //MEDIAINFO_DUPLICATE
    public File__Analyze
#endif //MEDIAINFO_DUPLICATE
{
public :
    //In
    int64u Frame_Count_Valid;
    bool   MustParse_SPS_PPS;
    bool   SizedBlocks;

    //Constructor/Destructor
    File_Avc();
    ~File_Avc();

    //AVC-Intra hardcoded headers
    struct avcintra_header
    {
        const int8u* Data;
        size_t       Size;

        avcintra_header(const int8u* Data_, size_t Size_)
            : Data(Data_)
            , Size(Size_)
        {}
    };
    static avcintra_header AVC_Intra_Headers_Data(int32u CodecID);
    static int32u AVC_Intra_CodecID_FromMeta(int32u Width, int32u Height, int32u Fields, int32u SampleDuration, int32u TimeScale, int32u SizePerFrame);

private :
    File_Avc(const File_Avc &File_Avc); //No copy
    File_Avc &operator =(const File_Avc &);

    //Structures - seq_parameter_set
    struct iso14496_base
    {
#if MEDIAINFO_DEMUX
        int8u*  Iso14496_10_Buffer;
        size_t  Iso14496_10_Buffer_Size;
        iso14496_base() :Iso14496_10_Buffer(NULL), Iso14496_10_Buffer_Size(0) {}
        ~iso14496_base()
        {
            delete[] Iso14496_10_Buffer;
        }
        void Init_Iso14496_10(int8u c, const int8u* Buffer, size_t Element_Size)
        {
            delete[] Iso14496_10_Buffer;
            Iso14496_10_Buffer_Size = (size_t)(Element_Size + 4);
            Iso14496_10_Buffer = new int8u[Iso14496_10_Buffer_Size];
            Iso14496_10_Buffer[0] = 0x00;
            Iso14496_10_Buffer[1] = 0x00;
            Iso14496_10_Buffer[2] = 0x01;
            Iso14496_10_Buffer[3] = c;
            std::memcpy(Iso14496_10_Buffer + 4, Buffer, (size_t)Element_Size);
        }
#endif //MEDIAINFO_DEMUX
    };

    enum vui_flag
    {
        video_signal_type_present_flag,
        video_full_range_flag,
        colour_description_present_flag,
        timing_info_present_flag,
        fixed_frame_rate_flag,
        pic_struct_present_flag,
        vui_flags_Max
    };
    typedef std::bitset<vui_flags_Max> vui_flags;
    struct seq_parameter_set_struct : public iso14496_base
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

                    xxl_data &operator=(const xxl_data &x)
                    {
                        bit_rate_value=x.bit_rate_value;
                        cpb_size_value=x.cpb_size_value;
                        cbr_flag=x.cbr_flag;
                        //initial_cpb_removal_delay=x.initial_cpb_removal_delay;
                        //initial_cpb_removal_delay_offset=x.initial_cpb_removal_delay_offset;
                        return *this;
                    }

                private:
                    xxl_data();
                };
                vector<xxl_data> SchedSel;
                int8u   initial_cpb_removal_delay_length_minus1;
                int8u   cpb_removal_delay_length_minus1;
                int8u   dpb_output_delay_length_minus1;
                int8u   time_offset_length;

                xxl(const vector<xxl_data> &SchedSel_, int8u initial_cpb_removal_delay_length_minus1_, int8u cpb_removal_delay_length_minus1_, int8u dpb_output_delay_length_minus1_, int8u time_offset_length_)
                    :
                    SchedSel(SchedSel_),
                    initial_cpb_removal_delay_length_minus1(initial_cpb_removal_delay_length_minus1_),
                    cpb_removal_delay_length_minus1(cpb_removal_delay_length_minus1_),
                    dpb_output_delay_length_minus1(dpb_output_delay_length_minus1_),
                    time_offset_length(time_offset_length_)
                {
                }

                xxl(const xxl &x)
                {
                    SchedSel=x.SchedSel;
                    initial_cpb_removal_delay_length_minus1=x.initial_cpb_removal_delay_length_minus1;
                    cpb_removal_delay_length_minus1=x.cpb_removal_delay_length_minus1;
                    dpb_output_delay_length_minus1=x.dpb_output_delay_length_minus1;
                    time_offset_length=x.time_offset_length;
                }

                xxl &operator=(const xxl &x)
                {
                    SchedSel=x.SchedSel;
                    initial_cpb_removal_delay_length_minus1=x.initial_cpb_removal_delay_length_minus1;
                    cpb_removal_delay_length_minus1=x.cpb_removal_delay_length_minus1;
                    dpb_output_delay_length_minus1=x.dpb_output_delay_length_minus1;
                    time_offset_length=x.time_offset_length;

                    return *this;
                }

            private:
                xxl();
            };
            xxl*    NAL;
            xxl*    VCL;
            int32u  num_units_in_tick;
            int32u  time_scale;
            int32u  chroma_sample_loc_type_top_field;
            int32u  chroma_sample_loc_type_bottom_field;
            int16u  sar_width;
            int16u  sar_height;
            int8u   video_format;
            int8u   colour_primaries;
            int8u   transfer_characteristics;
            int8u   matrix_coefficients;
            vui_flags flags;

            vui_parameters_struct(xxl* NAL_, xxl* VCL_, int32u num_units_in_tick_, int32u time_scale_, int32u chroma_sample_loc_type_top_field_, int32u chroma_sample_loc_type_bottom_field_, int16u  sar_width_, int16u  sar_height_, int8u video_format_, int8u colour_primaries_, int8u transfer_characteristics_, int8u matrix_coefficients_, vui_flags flags_)
                :
                NAL(NAL_),
                VCL(VCL_),
                num_units_in_tick(num_units_in_tick_),
                time_scale(time_scale_),
                chroma_sample_loc_type_top_field(chroma_sample_loc_type_top_field_),
                chroma_sample_loc_type_bottom_field(chroma_sample_loc_type_bottom_field_),
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
            }

        private:
            vui_parameters_struct &operator=(const vui_parameters_struct &v);
            vui_parameters_struct(const vui_parameters_struct &);
            vui_parameters_struct();
        };
        vui_parameters_struct* vui_parameters;
        int32u  pic_width_in_mbs_minus1;
        int32u  pic_height_in_map_units_minus1;
        int32u  frame_crop_left_offset;
        int32u  frame_crop_right_offset;
        int32u  frame_crop_top_offset;
        int32u  frame_crop_bottom_offset;
        int32u  MaxPicOrderCntLsb; //Computed value (for speed)
        int32u  MaxFrameNum; //Computed value (for speed)
        int16u  num_views_minus1; //MultiView specific field
        int8u   chroma_format_idc;
        int8u   profile_idc;
        int8u   level_idc;
        int8u   bit_depth_luma_minus8;
        int8u   bit_depth_chroma_minus8;
        int8u   log2_max_frame_num_minus4;
        int8u   pic_order_cnt_type;
        int8u   log2_max_pic_order_cnt_lsb_minus4;
        int8u   max_num_ref_frames;
        int8u   pic_struct_FirstDetected; //For stats only
        int8u   constraint_set_flags;
        bool    separate_colour_plane_flag;
        bool    delta_pic_order_always_zero_flag;
        bool    frame_mbs_only_flag;
        bool    mb_adaptive_frame_field_flag;

        //Computed values
        bool    NalHrdBpPresentFlag() {return vui_parameters && vui_parameters->NAL;}
        bool    VclHrdBpPresentFlag() {return vui_parameters && vui_parameters->VCL;}
        bool    CpbDpbDelaysPresentFlag() {return vui_parameters && (vui_parameters->NAL || vui_parameters->VCL);}
        int8u   ChromaArrayType() {return separate_colour_plane_flag?0:chroma_format_idc;}

        //Constructor/Destructor
        seq_parameter_set_struct(vui_parameters_struct* vui_parameters_, int32u pic_width_in_mbs_minus1_, int32u pic_height_in_map_units_minus1_, int32u frame_crop_left_offset_, int32u frame_crop_right_offset_, int32u frame_crop_top_offset_, int32u frame_crop_bottom_offset_, int8u chroma_format_idc_, int8u profile_idc_, int8u level_idc_, int8u bit_depth_luma_minus8_, int8u bit_depth_chroma_minus8_, int8u log2_max_frame_num_minus4_, int8u pic_order_cnt_type_, int8u log2_max_pic_order_cnt_lsb_minus4_, int8u max_num_ref_frames_, int8u constraint_set_flags_, bool separate_colour_plane_flag_, bool delta_pic_order_always_zero_flag_, bool frame_mbs_only_flag_, bool mb_adaptive_frame_field_flag_)
            :
            vui_parameters(vui_parameters_),
            pic_width_in_mbs_minus1(pic_width_in_mbs_minus1_),
            pic_height_in_map_units_minus1(pic_height_in_map_units_minus1_),
            frame_crop_left_offset(frame_crop_left_offset_),
            frame_crop_right_offset(frame_crop_right_offset_),
            frame_crop_top_offset(frame_crop_top_offset_),
            frame_crop_bottom_offset(frame_crop_bottom_offset_),
            num_views_minus1(0),
            chroma_format_idc(chroma_format_idc_),
            profile_idc(profile_idc_),
            level_idc(level_idc_),
            bit_depth_luma_minus8(bit_depth_luma_minus8_),
            bit_depth_chroma_minus8(bit_depth_chroma_minus8_),
            log2_max_frame_num_minus4(log2_max_frame_num_minus4_),
            pic_order_cnt_type(pic_order_cnt_type_),
            log2_max_pic_order_cnt_lsb_minus4(log2_max_pic_order_cnt_lsb_minus4_),
            max_num_ref_frames(max_num_ref_frames_),
            pic_struct_FirstDetected((int8u)-1), //For stats only, init
            constraint_set_flags(constraint_set_flags_),
            separate_colour_plane_flag(separate_colour_plane_flag_),
            delta_pic_order_always_zero_flag(delta_pic_order_always_zero_flag_),
            frame_mbs_only_flag(frame_mbs_only_flag_),
            mb_adaptive_frame_field_flag(mb_adaptive_frame_field_flag_)
        {
            switch (pic_order_cnt_type)
            {
                case 0 :
                            MaxPicOrderCntLsb = (int32u)std::pow(2.0, (int)(log2_max_pic_order_cnt_lsb_minus4 + 4));
                            MaxFrameNum = (int32u)-1; //Unused
                            break;
                case 1 :
                case 2 :
                            MaxPicOrderCntLsb = (int32u)-1; //Unused
                            MaxFrameNum = (int32u)std::pow(2.0, (int)(log2_max_frame_num_minus4 + 4));
                            break;
                default:
                            MaxFrameNum = (int32u)-1; //Unused
                            MaxPicOrderCntLsb = (int32u)-1; //Unused
            }
        }

        ~seq_parameter_set_struct()
        {
            delete vui_parameters; //vui_parameters=NULL;
        }

    private:
        seq_parameter_set_struct &operator=(const seq_parameter_set_struct &v);
        seq_parameter_set_struct(const seq_parameter_set_struct &);
        seq_parameter_set_struct();
    };
    typedef vector<seq_parameter_set_struct*> seq_parameter_set_structs;

    //Structures - pic_parameter_set
    struct pic_parameter_set_struct : public iso14496_base
    {
        #if MEDIAINFO_DEMUX
        #endif //MEDIAINFO_DEMUX
        int8u   seq_parameter_set_id;
        int8u   num_ref_idx_l0_default_active_minus1;
        int8u   num_ref_idx_l1_default_active_minus1;
        int8u   weighted_bipred_idc;
        int32u  num_slice_groups_minus1;
        int32u  slice_group_map_type;
        bool    entropy_coding_mode_flag;
        bool    bottom_field_pic_order_in_frame_present_flag;
        bool    weighted_pred_flag;
        bool    redundant_pic_cnt_present_flag;
        bool    deblocking_filter_control_present_flag;

        //Constructor/Destructor
        pic_parameter_set_struct(int8u seq_parameter_set_id_, int8u num_ref_idx_l0_default_active_minus1_, int8u num_ref_idx_l1_default_active_minus1_, int8u weighted_bipred_idc_, int32u num_slice_groups_minus1_, int32u slice_group_map_type_, bool entropy_coding_mode_flag_, bool bottom_field_pic_order_in_frame_present_flag_, bool weighted_pred_flag_, bool redundant_pic_cnt_present_flag_, bool deblocking_filter_control_present_flag_)
            :
            seq_parameter_set_id(seq_parameter_set_id_),
            num_ref_idx_l0_default_active_minus1(num_ref_idx_l0_default_active_minus1_),
            num_ref_idx_l1_default_active_minus1(num_ref_idx_l1_default_active_minus1_),
            weighted_bipred_idc(weighted_bipred_idc_),
            num_slice_groups_minus1(num_slice_groups_minus1_),
            slice_group_map_type(slice_group_map_type_),
            entropy_coding_mode_flag(entropy_coding_mode_flag_),
            bottom_field_pic_order_in_frame_present_flag(bottom_field_pic_order_in_frame_present_flag_),
            weighted_pred_flag(weighted_pred_flag_),
            redundant_pic_cnt_present_flag(redundant_pic_cnt_present_flag_),
            deblocking_filter_control_present_flag(deblocking_filter_control_present_flag_)
        {
        }

    private:
        pic_parameter_set_struct &operator=(const pic_parameter_set_struct &v);
        pic_parameter_set_struct(const pic_parameter_set_struct &);
        pic_parameter_set_struct();
    };
    typedef vector<pic_parameter_set_struct*> pic_parameter_set_structs;

    //Streams management
    void Streams_Fill();
    void Streams_Fill(vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item);
    void Streams_Fill_subset(vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item);
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
    bool Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10;
    void Data_Parse_Iso14496();
    #endif //MEDIAINFO_DEMUX

    //Buffer - Global
    #if MEDIAINFO_ADVANCED2
    void Read_Buffer_SegmentChange();
    #endif //MEDIAINFO_ADVANCED2
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    void Header_Parse();
    bool Header_Parser_QuickSearch();
    bool Header_Parser_Fill_Size();
    void Data_Parse();

    #if MEDIAINFO_DUPLICATE
        //Output buffer
        size_t Output_Buffer_Get (const String &Value);
        size_t Output_Buffer_Get (size_t Pos);
    #endif //MEDIAINFO_DUPLICATE

    //Options
    void Option_Manage ();

    //Elements
    void slice_layer_without_partitioning_IDR();
    void slice_layer_without_partitioning_non_IDR();
    void slice_header();
    void slice_data (bool AllCategories);
    void seq_parameter_set();
    void pic_parameter_set();
    void sei();
    void sei_message(int32u &seq_parameter_set_id);
    void sei_message_buffering_period(int32u &seq_parameter_set_id);
    void sei_message_buffering_period_xxl(seq_parameter_set_struct::vui_parameters_struct::xxl* xxl);
    void sei_message_pic_timing(int32u payloadSize, int32u seq_parameter_set_id);
    void sei_message_user_data_registered_itu_t_t35();
    void sei_message_user_data_registered_itu_t_t35_DTG1();
    void sei_message_user_data_registered_itu_t_t35_GA94();
    void sei_message_user_data_registered_itu_t_t35_GA94_03();
    void sei_message_user_data_registered_itu_t_t35_GA94_03_Delayed(int32u seq_parameter_set_id);
    void sei_message_user_data_registered_itu_t_t35_GA94_06();
    void sei_message_user_data_unregistered(int32u payloadSize);
    void sei_message_user_data_unregistered_x264(int32u payloadSize);
    void sei_message_user_data_unregistered_bluray(int32u payloadSize);
    void sei_message_user_data_unregistered_bluray_MDPM(int32u payloadSize);
    void sei_message_mastering_display_colour_volume();
    void sei_message_light_level();

    enum hdr_format
    {
        HdrFormat_EtsiTs103433,
        HdrFormat_SmpteSt209440,
        HdrFormat_SmpteSt2086,
    };

    typedef std::map<hdr_format, std::map<video, Ztring> > hdr;

    hdr                                 HDR;


    Ztring  maximum_content_light_level;
    Ztring  maximum_frame_average_light_level;

    void consumer_camera_1();
    void consumer_camera_2();
    void sei_message_recovery_point();
    void sei_message_mainconcept(int32u payloadSize);
    void sei_alternative_transfer_characteristics();
    void access_unit_delimiter();
    void filler_data();
    void prefix_nal_unit(bool svc_extension_flag);
    void subset_seq_parameter_set();
    void slice_layer_extension(bool svc_extension_flag);

    //Packets - SubElements
    seq_parameter_set_struct* seq_parameter_set_data(int32u &Data_id);
    void seq_parameter_set_data_Add(vector<seq_parameter_set_struct*> &Data, const int32u Data_id, seq_parameter_set_struct* Data_Item_New);
    void seq_parameter_set_svc_extension();
    void seq_parameter_set_mvc_extension(seq_parameter_set_struct* Data_Item);
    void scaling_list(int32u ScalingList_Size);
    void vui_parameters(seq_parameter_set_struct::vui_parameters_struct* &vui_parameters_Item);
    void svc_vui_parameters_extension();
    void mvc_vui_parameters_extension();
    void hrd_parameters(seq_parameter_set_struct::vui_parameters_struct::xxl* &hrd_parameters_Item);
    void nal_unit_header_svc_extension();
    void nal_unit_header_mvc_extension();
    void ref_pic_list_modification(int32u slice_type, bool mvc);
    void pred_weight_table(int32u  slice_type, int32u num_ref_idx_l0_active_minus1, int32u num_ref_idx_l1_active_minus1, int8u ChromaArrayType);
    void dec_ref_pic_marking(vector<int8u> &memory_management_control_operations);

    //Packets - Specific
    void SPS_PPS();

    //Streams
    struct stream : public stream_payload
    {
        bool   ShouldDuplicate;

        stream()
            :
            ShouldDuplicate(false)
        {
        }
    };
    vector<stream> Streams;

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
    seq_parameter_set_structs           seq_parameter_sets;
    seq_parameter_set_structs           subset_seq_parameter_sets;
    pic_parameter_set_structs           pic_parameter_sets;
    void Clean_Seq_Parameter();

    //File specific
    int8u                               SizeOfNALU_Minus1;

    //Status
    size_t                              IFrame_Count;
    int32s                              prevPicOrderCntMsb;
    int32u                              prevPicOrderCntLsb;
    int32u                              prevTopFieldOrderCnt;
    int32u                              prevFrameNum;
    int32u                              prevFrameNumOffset;
    vector<int8u>                       prevMemoryManagementControlOperations;

    //Count of a Packets
    size_t                              Block_Count;
    size_t                              Interlaced_Top;
    size_t                              Interlaced_Bottom;
    size_t                              Structure_Field;
    size_t                              Structure_Frame;
    size_t                              Slices_CountInThisFrame;
    size_t                              MaxSlicesCount;

    //Temp
    Ztring                              Encoded_Library;
    Ztring                              Encoded_Library_Name;
    Ztring                              Encoded_Library_Version;
    Ztring                              Encoded_Library_Date;
    Ztring                              Encoded_Library_Settings;
    Ztring                              BitRate_Nominal;
    Ztring                              MuxingMode;
    string                              PictureTypes_PreviousFrames;
    int64u                              tc;
    int64s                              pic_order_cnt_Delta;
    int64s                              pic_order_cnt_Displayed;
    int32u                              Firstpic_order_cnt_lsbInBlock;
    int8u                               nal_ref_idc;
    int8u                               FrameRate_Divider;
    int8u                               preferred_transfer_characteristics;
    bool                                FirstPFrameInGop_IsParsed;
    bool                                Config_IsRepeated;
    std::map<size_t, int>               FrameSizes;
    #if MEDIAINFO_ADVANCED2
        std::vector<std::string>        Dump_SPS;
        std::vector<std::string>        Dump_PPS;
    #endif //MEDIAINFO_ADVANCED2

    //Helpers
    string                              GOP_Detect                              (string PictureTypes);
    string                              ScanOrder_Detect                        (string ScanOrders);

    #if MEDIAINFO_DUPLICATE
        bool   File__Duplicate_Set  (const Ztring &Value); //Fill a new File__Duplicate value
        void   File__Duplicate_Write (int64u Element_Code, int32u frame_num=(int32u)-1);
        File__Duplicate__Writer Writer;
        int8u  Duplicate_Buffer[1024*1024];
        size_t Duplicate_Buffer_Size;
        size_t frame_num_Old;
        bool   SPS_PPS_AlreadyDone;
        bool   FLV;
    #endif //MEDIAINFO_DUPLICATE
};

} //NameSpace

#endif
