/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Aac_GeneralAudio_SbrH
#define MediaInfo_File_Aac_GeneralAudio_SbrH
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
enum sbr_ratio
{
    DUAL,
    QUAD,
};
struct sbr_handler
{
    //sbr_header
    bool   present=false;
    int8u  bs_amp_res[2];
    int8u  bs_pvc_mode; //For USAC parsing
    int8u  bs_amp_res_FromHeader;
    int8u  bs_start_freq;
    int8u  bs_stop_freq;
    int8u  bs_xover_band;
    int8u  bs_freq_scale;
    int8u  bs_alter_scale;
    int8u  bs_noise_bands;

    //sbr_grid
    int8u   bs_num_env[2];
    int8u   bs_frame_class[2]; //For USAC parsing
    bool    bs_freq_res[2][8];
    int8u   bs_num_noise[2];

    //sbr_dtdf
    int8u   bs_df_env[2][8];
    int8u   bs_df_noise[2][2];

    //Computed values
    sbr_ratio ratio=DUAL;
    int8u  num_noise_bands;
    int8u  num_env_bands[2];
};

struct ps_handler
{
    bool   enable_iid;
    bool   enable_icc;
    bool   enable_ext;
    int8u  iid_mode;
    int8u  icc_mode;
};

//---------------------------------------------------------------------------
extern const int8s t_huffman_env_1_5dB[][2];
extern const int8s f_huffman_env_1_5dB[][2];
extern const int8s t_huffman_env_bal_1_5dB[][2];
extern const int8s f_huffman_env_bal_1_5dB[][2];

//---------------------------------------------------------------------------
extern const int8s t_huffman_env_3_0dB[][2];
extern const int8s f_huffman_env_3_0dB[][2];
extern const int8s t_huffman_env_bal_3_0dB[][2];
extern const int8s f_huffman_env_bal_3_0dB[][2];
extern const int8s t_huffman_noise_3_0dB[][2];
extern const int8s t_huffman_noise_bal_3_0dB[][2];

//---------------------------------------------------------------------------
// Master frequency band table
// k0 = lower frequency boundary
extern const int8u Aac_k0_startMin[];
extern const int8s Aac_k0_offset[][2][16];

//---------------------------------------------------------------------------
// Master frequency band table
// k2 = upper frequency boundary
extern const int8u Aac_k2_stopMin[];
extern const int8s Aac_k2_offset[][2][14];

} //NameSpace

#endif
