/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_Ac4H
#define MediaInfo_Ac4H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/TimeCode.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{
typedef const int8s (*ac4_huffman)[2];

struct variable_size
{
    int8u AddedSize;
    int16u Value;
};

//***************************************************************************
// Class File_Ac4
//***************************************************************************

class File_Ac4 : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    bool   MustParse_dac4;

    struct loudness_info
    {
        int8u dialnorm_bits;
        int8u loud_prac_type;
        int8u loud_dialgate_prac_type;
        int16u max_truepk;
        bool b_loudcorr_type;
        int16u loudrelgat;
        int16u loudspchgat;
        int8u loudspchgat_dialgate_prac_type;
        int16u lra;
        int8u lra_prac_type;
        int16u max_loudmntry;

        loudness_info() :
            dialnorm_bits((int8u)-1),
            loud_prac_type((int8u)-1),
            loud_dialgate_prac_type((int8u)-1),
            max_truepk((int16u)-1),
            loudrelgat((int16u)-1),
            loudspchgat((int16u)-1),
            lra((int16u)-1),
            max_loudmntry((int16u)-1)
        {}
    };

    struct preprocessing
    {
        int8u pre_dmixtyp_2ch;
        int8u phase90_info_2ch;
        int8u pre_dmixtyp_5ch;
        int8u phase90_info_mc;
        bool b_surround_attenuation_known;
        bool b_lfe_attenuation_known;

        preprocessing() :
            pre_dmixtyp_2ch((int8u)-1),
            pre_dmixtyp_5ch((int8u)-1),
            phase90_info_mc((int8u)-1)
        {}
    };

    struct drc_decoder_config_curve
    {
        int8u drc_lev_nullband_low;
        int8u drc_lev_nullband_high;
        int8u drc_gain_max_boost;
        int8u drc_gain_max_cut;
        int8u drc_lev_max_cut;
        int8u drc_gain_section_cut;
        int8u drc_lev_section_cut;
        int8u drc_tc_attack;
        int8u drc_tc_release;
        int8u drc_tc_attack_fast;
        int8u drc_tc_release_fast;
        int8u drc_attack_threshold;
        int8u drc_release_threshold;

        bool operator==(const drc_decoder_config_curve& C) const
        {
            return !memcmp(this, &C, sizeof(drc_decoder_config_curve));
        }
    };

    struct drc_decoder_config
    {
        int8u drc_repeat_id;
        bool drc_default_profile_flag;
        int8u drc_decoder_mode_id;
        bool drc_compression_curve_flag;
        drc_decoder_config_curve drc_compression_curve;
        int8u drc_gains_config;

        drc_decoder_config() :
            drc_repeat_id((int8u)-1)
        {}
    };

    struct drc_info
    {
        vector<drc_decoder_config> Decoders;
        int8u drc_eac3_profile;

        drc_info() :
            drc_eac3_profile((int8u)-1)
        {}
    };

    struct de_config
    {
        int8u de_method;
        int8u de_max_gain;
        int8u de_channel_config;

        de_config() :
            de_max_gain((int8u)-1)
        {}
    };

    struct de_info
    {
        bool b_de_data_present;
        de_config Config;

        de_info() :
            b_de_data_present(false)
        {}
    };

    struct dmx
    {
        int8u loro_centre_mixgain;
        int8u loro_surround_mixgain;
        int8u ltrt_centre_mixgain;
        int8u ltrt_surround_mixgain;
        int8u lfe_mixgain;
        int8u preferred_dmx_method;
        struct cdmx
        {
            struct gain
            {
                enum class type : int8u
                {
                    f1_code,
                    f2_code,
                    b_code,
                    t1_code,
                    t2a_code,
                    t2b_code,
                    t2c_code,
                    t2d_code,
                    t2e_code,
                    t2f_code,
                };
                type        Type;
                int8u       Value;
            };
            int8u out_ch_config=(int8u)-1;
            vector<gain> Gains;
        };
        vector<cdmx> Cdmxs;

        dmx() :
            loro_centre_mixgain((int8u)-1),
            loro_surround_mixgain((int8u)-1),
            ltrt_centre_mixgain((int8u)-1),
            ltrt_surround_mixgain((int8u)-1),
            lfe_mixgain((int8u)-1),
            preferred_dmx_method((int8u)-1)
        {}
    };

    struct content_info
    {
        int8u content_classifier;
        string language_tag_bytes;

        content_info() :
            content_classifier((int8u)-1)
        {}
    };

    //Constructor/Destructor
    File_Ac4();
    ~File_Ac4();

private :
    enum substream_type_t
    {
        Type_Unknown,
        Type_Ac4_Substream,
        Type_Ac4_Hsf_Ext_Substream,
        Type_Emdf_Payloads_Substream,
        Type_Ac4_Presentation_Substream,
        Type_Oamd_Substream,
        Type_Max,
    };

    //Presentations
    struct presentation_substream
    {
        substream_type_t substream_type;
        int8u substream_index;
    };
    struct presentation
    {
        vector<size_t> substream_group_info_specifiers;
        vector<presentation_substream> Substreams;

        int8u presentation_version;
        int8u mdcompat;
        int32u presentation_id;
        bool b_pres_ndot;
        bool b_alternative;
        bool dolby_atmos_indicator;
        int8u substream_index;
        int8u presentation_config;
        int8u n_substream_groups;
        int8u b_multi_pid_PresentAndValue;
        int8u frame_rate_fraction_minus1;
        loudness_info LoudnessInfo;
        drc_info DrcInfo;
        dmx Dmx;

        //Computed
        int8u pres_ch_mode;
        int8u pres_ch_mode_core;
        int8u pres_immersive_stereo;
        int8u n_substreams_in_presentation;
        bool b_pres_4_back_channels_present;
        bool b_pres_centre_present;
        int8u pres_top_channel_pairs;
        string Language;

        presentation() :
            presentation_config((int8u)-1),
            mdcompat((int8u)-1),
            presentation_id((int32u)-1),
            frame_rate_fraction_minus1(0),
            dolby_atmos_indicator(false),
            substream_index((int8u)-1),
            b_multi_pid_PresentAndValue((int8u)-1),
            pres_ch_mode((int8u)-1),
            pres_ch_mode_core((int8u)-1),
            pres_immersive_stereo((int8u)-1),
            n_substreams_in_presentation(0),
            b_pres_4_back_channels_present(false),
            b_pres_centre_present(false),
            pres_top_channel_pairs(0)
        {}

        void reset_cumulative()
        {
            substream_group_info_specifiers.clear();
            Substreams.clear();
            frame_rate_fraction_minus1=0;
        }
    };
    vector<presentation> Presentations;
    vector<presentation> Presentations_IFrame;
    vector<presentation> Presentations_dac4;

    //Groups
    struct group_substream
    {
        substream_type_t substream_type;
        int8u substream_index;
        bool b_iframe;
        bool sus_ver;

        // b_channel_coded
        int8u ch_mode;
        bool b_4_back_channels_present;
        bool b_centre_present;
        int8u top_channels_present;
        int8u hsf_substream_index;

        // b_ajoc
        bool b_ajoc;
        bool b_static_dmx;
        int8u n_fullband_upmix_signals;
        int8u n_fullband_dmx_signals;

        // !b_ajoc
        int8u n_objects_code;
        bool b_dynamic_objects;
        bool b_lfe;
        int32u nonstd_bed_channel_assignment_mask;

        // Computed
        int8u ch_mode_core;
        int8u immersive_stereo;
        int8u top_channel_pairs;

        group_substream() :
            substream_index((int8u)-1),
            sus_ver(false),
            ch_mode((int8u)-1),
            ch_mode_core((int8u)-1),
            immersive_stereo((int8u)-1),
            top_channels_present((int8u)-1),
            hsf_substream_index((int8u)-1),
            nonstd_bed_channel_assignment_mask((int32u)-1)
        {}
    };
    struct group
    {
        vector<group_substream> Substreams;
        content_info ContentInfo;
        bool b_channel_coded;
        bool b_hsf_ext;
    };
    vector<group> Groups;
    vector<group> Groups_IFrame;
    vector<group> Groups_dac4;

    //Audio substreams
    struct buffer
    {
        int8u* Data;
        size_t Offset;
        size_t Size;

        void Create(size_t NewSize)
        {
            delete[] Data; 
            Size=NewSize;
            Data=new int8u[Size];
            Offset=0;
        }

        void Clear()
        {
            delete[] Data;
            Data=NULL;
        }

        void Append(const uint8_t* BufferToAdd, size_t SizeToAdd)
        {
            if (!Data)
                Create(SizeToAdd);
            size_t NewOffset=Offset+SizeToAdd;
            if (NewOffset>Size)
            {
                uint8_t* Data_ToDelete=Data;
                Size=NewOffset;
                Data=new int8u[Size];
                if (Data_ToDelete)
                    memcpy(Data, Data_ToDelete, Offset);
                delete[] Data_ToDelete;
            }
            if (Data+Offset)
                memcpy(Data+Offset, BufferToAdd, SizeToAdd);
            Offset=NewOffset;
        }

        buffer() :
            Data(NULL)
            {}

        ~buffer()
        {
            delete[] Data;
        }
    };
    struct audio_substream
    {
        loudness_info LoudnessInfo;
        int8u dialog_max_gain;
        drc_info DrcInfo;
        de_info DeInfo;
        preprocessing Preprocessing;
        buffer Buffer;
        int8u Buffer_Index;
        bool b_dialog;
        bool b_iframe;

        audio_substream(bool b_iframe_=true) :
            dialog_max_gain((int8u)-1),
            Buffer_Index(0),
            b_dialog(false),
            b_iframe(b_iframe_)
        {}
    };
    std::map<int8u, audio_substream> AudioSubstreams;
    std::map<int8u, audio_substream> AudioSubstreams_IFrame;

    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Synchro
    bool Synchronize();
    void Synched_Init();
    bool Synched_Test();

    //Buffer - Global
    void Read_Buffer_Continue ();
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void raw_ac4_frame();
    void raw_ac4_frame_substreams();
    void ac4_toc();
    void ac4_presentation_info(presentation& P);
    void ac4_presentation_v1_info(presentation& P);
    void ac4_sgi_specifier(presentation& P);
    void ac4_substream_info(presentation& P);
    void ac4_substream_group_info(group& G);
    void ac4_hsf_ext_substream_info(group_substream& G, bool b_substreams_present);
    void ac4_substream_info_chan(group_substream& G, size_t Pos, bool b_substreams_present);
    void ac4_substream_info_ajoc(group_substream& G, bool b_substreams_present);
    void ac4_substream_info_obj(group_substream& G, bool b_substreams_present);
    void ac4_presentation_substream_info(presentation& P);
    void presentation_config_ext_info(presentation& P);
    void bed_dyn_obj_assignment(group_substream& G, int8u n_signals);
    void content_type(content_info& ContentInfo);
    void frame_rate_multiply_info();
    void frame_rate_fractions_info(presentation& P);
    void emdf_info(presentation_substream& P);
    void emdf_payloads_substream_info(presentation_substream& P);
    void emdf_protection();
    void substream_index_table();
    void oamd_substream_info(group_substream& G, bool b_substreams_present);
    void oamd_common_data();

    void ac4_substream(size_t substream_index);
    void ac4_presentation_substream(size_t substream_index, size_t Substream_Index);

    void metadata(audio_substream& AudioSubstream, size_t Substream_Index);
    void basic_metadata(loudness_info& LoudnessInfo, preprocessing& Preprocessing, int8u ch_mode, bool sus_ver);
    void extended_metadata(audio_substream& AudioSubstream, bool b_associated, int8u ch_mode, bool sus_ver);
    void dialog_enhancement(de_info& Info, int8u ch_mode, bool b_iframe);
    void dialog_enhancement_config(de_info& Info);
    void dialog_enhancement_data(de_info& Info, bool b_iframe, bool b_de_simulcast);
    void custom_dmx_data(dmx& Dmx, int8u pres_ch_mode, int8u pres_ch_mode_core, bool b_pres_4_back_channels_present, int8u pres_top_channel_pairs, bool b_pres_has_lfe);
    void cdmx_parameters(int8u bs_ch_config, int8u out_ch_config);
    void tool_scr_to_c_l();
    void tool_b4_to_b2();
    void tool_t4_to_t2();
    void tool_t4_to_f_s_b();
    void tool_t4_to_f_s();
    void tool_t2_to_f_s_b();
    void tool_t2_to_f_s();
    void loud_corr(int8u pres_ch_mode, int8u pres_ch_mode_core, bool b_objects);
    void drc_frame(drc_info& DrcInfo, bool b_iframe);
    void drc_config(drc_info& DrcInfo);
    void drc_data(drc_info& DrcInfo);
    void drc_gains(drc_decoder_config& Decoder);
    void drc_decoder_mode_config(drc_decoder_config& Decoder);
    void drc_compression_curve(drc_decoder_config_curve& Curve);

    void further_loudness_info(loudness_info& LoudnessInfo, bool sus_ver, bool b_presentation_ldn);

    void dac4();
    void alternative_info();
    void ac4_bitrate_dsi();
    void ac4_presentation_v1_dsi(presentation& P);
    void ac4_substream_group_dsi(presentation& P);

    //Parsing
    void Get_V4 (int8u  Bits, int32u  &Info, const char* Name);
    void Skip_V4(int8u  Bits, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, int32u &Info, const char* Name);
    void Skip_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int32u  &Info, const char* Name);
    void Get_V4(const variable_size* Bits, int8u &Info, const char* Name);
    void Get_VB (int8u  &Info, const char* Name);
    void Skip_VB(const char* Name);

    //Utils
    bool CRC_Compute(size_t Size);
    void ac4_toc_Compute(vector<presentation>& Ps, vector<group>& Gs, bool FromDac4);
    int8u Superset(int8u Ch_Mode1, int8u Ch_Mode2);
    int16u Huffman_Decode(const ac4_huffman& Table, const char* Name);
    void Get_Gain(int8u Bits, dmx::cdmx::gain::type Type, const char* Name);

    //Temp
    int32u frame_size;
    int32u payload_base;
    int16u sync_word;
    int8u bitstream_version;
    int8u frame_rate_index;
    bool fs_index;
    int8u frame_rate_factor;
    int8u frame_rate_fraction;
    int8u max_group_index;
    int8u n_substreams;
    vector<size_t> IFrames;
    std::vector<size_t> Substream_Size;
    std::map<int8u, substream_type_t> Substream_Type;
};

} //NameSpace

#endif
