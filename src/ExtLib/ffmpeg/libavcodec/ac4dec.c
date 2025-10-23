/*
 * AC-4 Audio Decoder
 *
 * Copyright (c) 2019 Paul B Mahol
 *
 * This file is part of Librempeg
 *
 * Librempeg is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Librempeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Librempeg; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define ASSERT_LEVEL 2
#include "libavutil/avassert.h"
#include "libavutil/tx.h"
#include "libavutil/channel_layout.h"
#include "libavutil/float_dsp.h"
#include "libavutil/mem_internal.h"
#include "libavutil/mem.h"
#include "libavutil/thread.h"
#include "libavutil/qsort.h"
#include "libavutil/opt.h"

#include "ac4dec_data.h"
#include "avcodec.h"
#include "get_bits.h"
#include "codec_internal.h"
#include "decode.h"
#include "kbdwin.h"
#include "unary.h"

#define MAX_QMF_TSLOTS 32
#define MAX_QMF_BANDS 64
#define MAX_SBG_NOISE 6
#define MAX_ASPX_SIGNAL 5

/* Number of model bits */
#define SSF_MODEL_BITS 15

/* Model unit for the CDF specification */
#define SSF_MODEL_UNIT (1U<<(SSF_MODEL_BITS))

/* Number of range bits */
#define SSF_RANGE_BITS    30

/* Half of the range unit */
#define SSF_THRESHOLD_LARGE (1U<<((SSF_RANGE_BITS)-1))

/* Quarter of the range unit */
#define SSF_THRESHOLD_SMALL    (1U<<((SSF_RANGE_BITS)-2))

/* Offset bits */
#define SSF_OFFSET_BITS 14

typedef struct ACState {
    uint32_t ui_low;
    uint32_t ui_range;
    uint32_t ui_offset;
    uint32_t ui_offset2;

    uint32_t ui_threshold_small;
    uint32_t ui_threshold_large;
    uint32_t ui_model_unit;

    uint32_t ui_range_bits;
    uint32_t ui_model_bits;
} ACState;

typedef struct EMDFInfo {
    int     version;
    int     key_id;
    int     substream_index;
} EMDFInfo;

typedef struct SubstreamChannelParameters {
    uint8_t long_frame;
    uint8_t transf_length_idx[2];
    int     transf_length[2];

    uint8_t different_framing;
    uint8_t max_sfb_side[2];
    uint8_t max_sfb[2];
    uint8_t scale_factor_grouping[18];

    uint8_t num_windows;
    uint8_t num_window_groups;
    uint8_t window_to_group[16];
    uint8_t num_win_in_group[16];

    uint8_t dual_maxsfb;
    uint8_t side_limited;
    uint8_t side_channel;
} SubstreamChannelParameters;

typedef struct SubstreamChannel {
    SubstreamChannelParameters scp;

    int     master_reset;
    int     num_sbg_master;
    int     num_sb_aspx;
    int     num_sbg_noise;
    int     num_sbg_sig_highres;
    int     num_sbg_sig_lowres;
    int     sba;
    int     sbx;
    int     sbz;

    int     sap_mode;
    int     N_prev;
    int     idx_prev;

    uint8_t ms_used[16][128];
    uint8_t sap_coeff_used[16][128];
    int     dpcm_alpha_q[16][128];

    int     delta_code_time;

    int     num_sec_lsf[16];
    int     num_sec[16];
    uint8_t sfb_cb[16][128];
    uint8_t sect_cb[16][128];
    int     sect_start[16][128];
    int     sect_end[16][128];
    int     sect_sfb_offset[16][128];

    int16_t quant_spec[8192];
    int16_t offset2sfb[8192];
    uint8_t offset2g[8192];
    int     win_offset[16];

    int     max_quant_idx[16][128];
    int     dpcm_sf[16][128];
    int     dpcm_snf[16][128];
    int     snf_data_exists;

    float   sf_gain[16][128];

    int     aspx_int_class;
    int     aspx_num_noise;
    int     aspx_num_noise_prev;
    int     aspx_num_rel_left;
    int     aspx_num_rel_right;
    int     aspx_num_env;
    int     aspx_num_env_prev;
    int     num_atsg_sig;
    int     num_atsg_sig_prev;
    int     num_atsg_noise;
    int     num_atsg_noise_prev;
    uint8_t aspx_freq_res[MAX_ASPX_SIGNAL+1];
    int     aspx_var_bord_left;
    int     aspx_var_bord_right;
    int     aspx_rel_bord_left[4];
    int     aspx_rel_bord_right[4];
    int     aspx_tsg_ptr;
    int     aspx_tsg_ptr_prev;

    int     aspx_qmode_env;
    uint8_t aspx_sig_delta_dir[MAX_ASPX_SIGNAL];
    uint8_t aspx_noise_delta_dir[2];
    uint8_t aspx_tna_mode[MAX_SBG_NOISE];
    uint8_t aspx_tna_mode_prev[MAX_SBG_NOISE];
    uint8_t aspx_add_harmonic[MAX_QMF_TSLOTS];
    uint8_t aspx_fic_used_in_sfb[MAX_QMF_TSLOTS];
    uint8_t aspx_tic_used_in_slot[MAX_QMF_TSLOTS];
    int     aspx_xover_subband_offset;
    int     aspx_balance;

    uint8_t atsg_freqres[MAX_ASPX_SIGNAL];
    uint8_t atsg_freqres_prev[MAX_ASPX_SIGNAL];
    uint8_t atsg_sig[MAX_SBG_NOISE];
    uint8_t atsg_noise[MAX_ASPX_SIGNAL + 1];
    int     previous_stop_pos;

    int     sbg_noise[MAX_SBG_NOISE];
    int     sbg_sig_lowres[24];
    int     sbg_sig_highres[24];
    int     sbg_lim[32];
    int     sbg_patches[MAX_SBG_NOISE];
    int     sbg_patch_num_sb[MAX_SBG_NOISE];
    int     sbg_patch_start_sb[MAX_SBG_NOISE];
    int     sbg_master[24];

    int     num_sbg_sig[6];
    int     sbg_sig[6][24];
    int     num_sbg_patches;
    int     num_sbg_lim;

    int16_t aspx_data[2][MAX_ASPX_SIGNAL][MAX_QMF_BANDS];

    int8_t  qscf_prev[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    int8_t  qscf_noise_prev[2][MAX_QMF_BANDS];
    int8_t  qscf_sig_sbg[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    int8_t  qscf_sig_sbg_prev[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   scf_sig_sbg[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   scf_sig_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    int8_t  qscf_noise_sbg[2][MAX_QMF_BANDS];
    float   scf_noise_sbg[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   scf_noise_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];

    float   gain_vec[MAX_QMF_BANDS];
    float   chirp_arr[MAX_SBG_NOISE];
    float   chirp_arr_prev[MAX_SBG_NOISE];
    float   est_sig_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    uint8_t sine_idx_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    uint8_t sine_idx_sb_prev[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    uint8_t sine_area_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   sine_lev_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   noise_lev_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   sig_gain_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   max_sig_gain_sbg[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   max_sig_gain_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   noise_lev_sb_lim[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   sig_gain_sb_lim[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   boost_fact_sbg[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   boost_fact_sb[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   sig_gain_sb_adj[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   noise_lev_sb_adj[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];
    float   sine_lev_sb_adj[MAX_ASPX_SIGNAL][MAX_QMF_BANDS];

    int8_t  sine_idx_prev[MAX_QMF_TSLOTS][MAX_QMF_BANDS];
    int16_t noise_idx_prev[MAX_QMF_TSLOTS][MAX_QMF_BANDS];

    int     acpl_interpolation_type;
    int     acpl_num_param_sets_cod;
    int     acpl_param_timeslot[2];
    int     acpl_data[11][16];

    int     start_block, end_block;
    int     stride_flag;
    int     num_bands;
    int     predictor_presence[4];
    int     predictor_lag_delta[4];
    int     predictor_lag[4];
    int     variance_preserving[4];
    int     alloc_offset[4];
    int     delta[4];
    int     gain_bits[4];
    int     env_idx[19];
    ACState acs;

    DECLARE_ALIGNED(32, float, scaled_spec)[2048];
    DECLARE_ALIGNED(32, float, spec_reord)[2048];

    DECLARE_ALIGNED(32, float, pcm)[2048];
    DECLARE_ALIGNED(32, float, imdct_out)[4096];
    DECLARE_ALIGNED(32, float, overlap)[4096];

    DECLARE_ALIGNED(32, float, qana_filt)[MAX_QMF_BANDS*10];
    DECLARE_ALIGNED(32, float, qmf_sine)[2][MAX_QMF_TSLOTS][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, qmf_noise)[2][MAX_QMF_TSLOTS][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, qsyn_filt)[MAX_QMF_BANDS*20];
    DECLARE_ALIGNED(32, float, L)[MAX_QMF_TSLOTS];
    DECLARE_ALIGNED(32, float, g)[MAX_QMF_TSLOTS];
    DECLARE_ALIGNED(32, float, E)[MAX_QMF_TSLOTS][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, Q)[2][MAX_QMF_TSLOTS][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, Q_prev)[2][MAX_QMF_TSLOTS][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, Q_low)[2][MAX_QMF_TSLOTS+6][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, Q_low_prev)[2][MAX_QMF_TSLOTS+6][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, Q_low_ext)[2][MAX_QMF_TSLOTS+10][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, Q_high)[2][MAX_QMF_TSLOTS+4][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, cov)[MAX_QMF_BANDS][3][3][2];
    DECLARE_ALIGNED(32, float, alpha0)[MAX_QMF_BANDS][2];
    DECLARE_ALIGNED(32, float, alpha1)[MAX_QMF_BANDS][2];
    DECLARE_ALIGNED(32, float, Y)[2][MAX_QMF_TSLOTS][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, Y_prev)[2][MAX_QMF_TSLOTS][MAX_QMF_BANDS];
} SubstreamChannel;

typedef struct Substream {
    int     codec_mode;
    int     im_codec_mode;
    int     core_5ch_grouping;
    int     use_sap_add_ch;

    int     aspx_quant_mode_env;
    int     aspx_start_freq;
    int     prev_aspx_start_freq;
    int     aspx_stop_freq;
    int     prev_aspx_stop_freq;
    int     aspx_master_freq_scale;
    int     prev_aspx_master_freq_scale;
    int     aspx_interpolation;
    int     aspx_preflat;
    int     aspx_limiter;
    int     aspx_noise_sbg;
    int     aspx_num_env_bits_fixfix;
    int     aspx_freq_res_mode;

    int     acpl_qmf_band;
    int     acpl_param_band;
    int     acpl_num_param_bands_id;
    uint8_t acpl_quant_mode[2];

    uint8_t sync_flag;
    uint8_t mode_2ch;
    uint8_t chel_matsel;

    uint8_t compand_on[12];
    uint8_t compand_avg;

    int     max_sfb_master;

    uint8_t coding_config;
    uint8_t mdct_stereo_proc[8];
    float   matrix_stereo[16][128][2][2];
    float   alpha_q[16][128];

    int     spec_frontend_l;
    int     spec_frontend_r;
    int     spec_frontend_m;
    int     spec_frontend_s;

    DECLARE_ALIGNED(32, float, g_sum)[MAX_QMF_TSLOTS];

    SubstreamChannel ssch[12];
} Substream;

typedef struct PresentationSubstreamInfo {
    int     alternative;
    int     pres_ndot;
    int     substream_index;
} PresentationSubstreamInfo;

typedef struct Metadata {
    int     dialnorm_bits;
    int     pre_dmixtyp_2ch;
    int     phase90_info_2ch;
    int     loro_center_mixgain;
    int     loro_surround_mixgain;
    int     loro_dmx_loud_corr;
    int     ltrt_center_mixgain;
    int     ltrt_surround_mixgain;
    int     ltrt_dmx_loud_corr;
    int     lfe_mixgain;
    int     preferred_dmx_method;
    int     pre_dmixtyp_5ch;
    int     pre_upmixtyp_5ch;
    int     pre_upmixtyp_3_4;
    int     pre_upmixtyp_3_2_2;
    int     phase90_info_mc;
    int     surround_attenuation_known;
    int     lfe_attenuation_known;
    int     dc_block_on;

    int     loudness_version;
    int     loud_prac_type;
    int     dialgate_prac_type;
    int     loudcorr_type;
    int     loudrelgat;
    int     loudspchgat;
    int     loudstrm3s;
    int     max_loudstrm3s;
    int     truepk;
    int     max_truepk;
    int     prgmbndy;
    int     end_or_start;
    int     prgmbndy_offset;
    int     lra;
    int     lra_prac_type;
    int     loudmntry;
    int     max_loudmntry;

    int     drc_decoder_nr_modes;
    int     drc_eac3_profile;
} Metadata;

typedef struct SubstreamInfo {
    int     sus_ver;
    int     channel_mode;
    int     substream_index;
    int     hsf_ext_substream_index;
    int     sf_multiplier;
    int     bitrate_indicator;
    int     add_ch_base;
    int     iframe[4];
    int     back_channels_present;
    int     centre_present;
    int     top_channels_present;

    int lfe;
    int static_dmx;
    int audio_ndot[4];
    int n_fullband_dmx_signals;
    int n_fullband_upmix_signals;

    int n_objects;

    Metadata meta;
} SubstreamInfo;

typedef struct SubstreamGroupInfo {
    int           channel_coded;
    int           ajoc;
    int           group_index;
    SubstreamInfo ssinfo;
} SubstreamGroupInfo;

typedef struct PresentationInfo {
    int     single_substream;
    int     enable_presentation;
    int     presentation_config;
    int     presentation_version;
    int     add_emdf_substreams;
    int     n_add_emdf_substreams;
    int     n_substream_groups;
    int     mdcompat;
    int     presentation_id;
    int     multiplier;
    int     multiplier_bit;
    int     pre_virtualized;
    int     frame_rate_factor;
    int     frame_rate_fraction;
    int     multi_pid;
    int     hsf_ext;
    EMDFInfo emdf[32];
    PresentationSubstreamInfo psinfo;
    SubstreamInfo ssinfo[4];
} PresentationInfo;

typedef struct AC4DecodeContext {
    AVClass        *class;                  ///< class for AVOptions
    AVCodecContext *avctx;                  ///< parent context
    AVFloatDSPContext *fdsp;
    GetBitContext   gbc;                    ///< bitstream reader

    int             target_presentation;

    int             version;
    int             sequence_counter;
    int             sequence_counter_prev;
    int             wait_frames;
    int             nb_wait_frames;
    int             fs_index;
    int             frame_rate_index;
    int             frame_len_base;
    int             frame_len_base_idx;
    AVRational      resampling_ratio;
    int             num_qmf_timeslots;
    int             num_aspx_timeslots;
    int             num_ts_in_ats;
    int             ts_offset_hfgen;
    int             transform_length;
    int             iframe_global;
    int             first_frame;
    int             have_iframe;
    int             nb_presentations;
    int             payload_base;
    int             short_program_id;
    int             nb_substreams;
    int             total_groups;
    int             substream_size[32];
    int             substream_type[32];
    int             ch_map[256];

    av_tx_fn        tx_fn[8][5];
    AVTXContext    *tx_ctx[8][5];

    DECLARE_ALIGNED(32, float, kbd_window)[8][5][2048];

    float           quant_lut[8192];

    DECLARE_ALIGNED(32, float, cos_atab)[MAX_QMF_BANDS][MAX_QMF_BANDS*2];
    DECLARE_ALIGNED(32, float, sin_atab)[MAX_QMF_BANDS][MAX_QMF_BANDS*2];
    DECLARE_ALIGNED(32, float, cos_stab)[MAX_QMF_BANDS*2][MAX_QMF_BANDS];
    DECLARE_ALIGNED(32, float, sin_stab)[MAX_QMF_BANDS*2][MAX_QMF_BANDS];

    PresentationInfo   pinfo[16];
    SubstreamGroupInfo ssgroup[16];
    Substream          substream;
} AC4DecodeContext;

enum StrideFlag {
    LONG_STRIDE,
    SHORT_STRIDE,
};

enum ImmersiveMode {
    IM_SCPL,
    IM_ASPX_SCPL,
    IM_ASPX_ACPL_1,
    IM_ASPX_ACPL_2,
    IM_ASPX_AJCC,
};

enum ACPLMode {
    ACPL_FULL,
    ACPL_PARTIAL,
};

enum SubstreamType {
    ST_SUBSTREAM,
    ST_PRESENTATION,
};

enum StereoMode {
    SM_LEVEL,
    SM_BALANCE,
};

enum DataType {
    DT_SIGNAL,
    DT_NOISE,
};

enum SpectralFrontend {
    SF_ASF,
    SF_SSF,
};

enum HCBType {
    F0,
    DF,
    DT,
};

enum CodecMode {
    CM_SIMPLE,
    CM_ASPX,
    CM_ASPX_ACPL_1,
    CM_ASPX_ACPL_2,
    CM_ASPX_ACPL_3,
};

enum IntervalClass {
    FIXFIX,
    FIXVAR,
    VARFIX,
    VARVAR,
};

enum ACPLDataType {
    ALPHA1,
    ALPHA2,
    BETA1,
    BETA2,
    BETA3,
    GAMMA1,
    GAMMA2,
    GAMMA3,
    GAMMA4,
    GAMMA5,
    GAMMA6,
};

static const uint16_t decode_delays[] = {
    288,
    288,
    352,
    96,
    96,
    960,
    960,
    1056,
    672,
    672,
    1312,
    864,
    864,
    352,
    352,
};

static const AVChannelLayout channel_mode_layouts[] = {
    AV_CHANNEL_LAYOUT_MONO,
    AV_CHANNEL_LAYOUT_STEREO,
    AV_CHANNEL_LAYOUT_SURROUND,
    AV_CHANNEL_LAYOUT_5POINT0,
    AV_CHANNEL_LAYOUT_5POINT1,
    AV_CHANNEL_LAYOUT_7POINT0,
    AV_CHANNEL_LAYOUT_7POINT1,
    AV_CHANNEL_LAYOUT_7POINT0_FRONT,
    {
        .nb_channels = 7,
        .order = AV_CHANNEL_ORDER_NATIVE,
        .u.mask = AV_CH_LAYOUT_7POINT0 | AV_CH_LOW_FREQUENCY,
    },
    { 0 },
    { 0 },
    { 0 },
    {
        .nb_channels = 12,
        .order = AV_CHANNEL_ORDER_NATIVE,
        .u.mask = AV_CH_LAYOUT_7POINT0 | AV_CH_LOW_FREQUENCY |
                  AV_CH_TOP_FRONT_LEFT | AV_CH_TOP_FRONT_RIGHT |
                  AV_CH_TOP_BACK_LEFT | AV_CH_TOP_BACK_RIGHT,
    },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
};

static VLC channel_mode_vlc;
static VLC bitrate_indicator_vlc;
static VLC scale_factors_vlc;
static VLC snf_vlc;
static VLC asf_codebook_vlc[11];
static VLC acpl_codebook_vlc[4][2][3];
static VLC aspx_int_class_vlc;
static VLC aspx_codebook_signal_vlc[2][2][3];
static VLC aspx_codebook_noise_vlc[2][3];

static av_cold void ac4_init_static(void)
{
    vlc_init(&channel_mode_vlc, 9, sizeof(channel_mode_bits),
             channel_mode_bits, 1, 1, channel_mode_codes, 2, 2, 0);
    vlc_init(&bitrate_indicator_vlc, 5, sizeof(bitrate_indicator_bits),
             bitrate_indicator_bits, 1, 1, bitrate_indicator_codes, 1, 1, 0);
    vlc_init(&scale_factors_vlc, 9, sizeof(scale_factors_bits),
             scale_factors_bits, 1, 1, scale_factors_codes, 1, 1, 0);
    vlc_init(&snf_vlc, 7, sizeof(snf_bits),
             snf_bits, 1, 1, snf_codes, 1, 1, 0);

    vlc_init(&asf_codebook_vlc[0], 9, sizeof(asf_codebook_1_bits),
             asf_codebook_1_bits, 1, 1, asf_codebook_1_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[1], 9, sizeof(asf_codebook_2_bits),
             asf_codebook_2_bits, 1, 1, asf_codebook_2_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[2], 9, sizeof(asf_codebook_3_bits),
             asf_codebook_3_bits, 1, 1, asf_codebook_3_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[3], 9, sizeof(asf_codebook_4_bits),
             asf_codebook_4_bits, 1, 1, asf_codebook_4_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[4], 9, sizeof(asf_codebook_5_bits),
             asf_codebook_5_bits, 1, 1, asf_codebook_5_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[5], 9, sizeof(asf_codebook_6_bits),
             asf_codebook_6_bits, 1, 1, asf_codebook_6_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[6], 9, sizeof(asf_codebook_7_bits),
             asf_codebook_7_bits, 1, 1, asf_codebook_7_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[7], 9, sizeof(asf_codebook_8_bits),
             asf_codebook_8_bits, 1, 1, asf_codebook_8_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[8], 9, sizeof(asf_codebook_9_bits),
             asf_codebook_9_bits, 1, 1, asf_codebook_9_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[9], 9, sizeof(asf_codebook_10_bits),
             asf_codebook_10_bits, 1, 1, asf_codebook_10_codes, 1, 1, 0);
    vlc_init(&asf_codebook_vlc[10], 9, sizeof(asf_codebook_11_bits),
             asf_codebook_11_bits, 1, 1, asf_codebook_11_codes, 1, 1, 0);

    vlc_init(&aspx_int_class_vlc, 5, sizeof(aspx_int_class_bits),
             aspx_int_class_bits, 1, 1, aspx_int_class_codes, 1, 1, 0);

    vlc_init(&aspx_codebook_signal_vlc[0][0][0], 9, sizeof(aspx_hcb_env_level_15_f0_bits),
             aspx_hcb_env_level_15_f0_bits, 1, 1, aspx_hcb_env_level_15_f0_codes, 4, 4, 0);
    vlc_init(&aspx_codebook_signal_vlc[0][0][1], 9, sizeof(aspx_hcb_env_level_15_df_bits),
             aspx_hcb_env_level_15_df_bits, 1, 1, aspx_hcb_env_level_15_df_codes, 4, 4, 0);
    vlc_init(&aspx_codebook_signal_vlc[0][0][2], 9, sizeof(aspx_hcb_env_level_15_dt_bits),
             aspx_hcb_env_level_15_dt_bits, 1, 1, aspx_hcb_env_level_15_dt_codes, 4, 4, 0);

    vlc_init(&aspx_codebook_signal_vlc[0][1][0], 9, sizeof(aspx_hcb_env_level_30_f0_bits),
             aspx_hcb_env_level_30_f0_bits, 1, 1, aspx_hcb_env_level_30_f0_codes, 4, 4, 0);
    vlc_init(&aspx_codebook_signal_vlc[0][1][1], 9, sizeof(aspx_hcb_env_level_30_df_bits),
             aspx_hcb_env_level_30_df_bits, 1, 1, aspx_hcb_env_level_30_df_codes, 4, 4, 0);
    vlc_init(&aspx_codebook_signal_vlc[0][1][2], 9, sizeof(aspx_hcb_env_level_30_dt_bits),
             aspx_hcb_env_level_30_dt_bits, 1, 1, aspx_hcb_env_level_30_dt_codes, 4, 4, 0);

    vlc_init(&aspx_codebook_signal_vlc[1][0][0], 9, sizeof(aspx_hcb_env_balance_15_f0_bits),
             aspx_hcb_env_balance_15_f0_bits, 1, 1, aspx_hcb_env_balance_15_f0_codes, 4, 4, 0);
    vlc_init(&aspx_codebook_signal_vlc[1][0][1], 9, sizeof(aspx_hcb_env_balance_15_df_bits),
             aspx_hcb_env_balance_15_df_bits, 1, 1, aspx_hcb_env_balance_15_df_codes, 4, 4, 0);
    vlc_init(&aspx_codebook_signal_vlc[1][0][2], 9, sizeof(aspx_hcb_env_balance_15_dt_bits),
             aspx_hcb_env_balance_15_dt_bits, 1, 1, aspx_hcb_env_balance_15_dt_codes, 4, 4, 0);

    vlc_init(&aspx_codebook_signal_vlc[1][1][0], 9, sizeof(aspx_hcb_env_balance_30_f0_bits),
             aspx_hcb_env_balance_30_f0_bits, 1, 1, aspx_hcb_env_balance_30_f0_codes, 2, 2, 0);
    vlc_init(&aspx_codebook_signal_vlc[1][1][1], 9, sizeof(aspx_hcb_env_balance_30_df_bits),
             aspx_hcb_env_balance_30_df_bits, 1, 1, aspx_hcb_env_balance_30_df_codes, 4, 4, 0);
    vlc_init(&aspx_codebook_signal_vlc[1][1][2], 9, sizeof(aspx_hcb_env_balance_30_dt_bits),
             aspx_hcb_env_balance_30_dt_bits, 1, 1, aspx_hcb_env_balance_30_dt_codes, 2, 2, 0);

    vlc_init(&aspx_codebook_noise_vlc[0][0], 9, sizeof(aspx_hcb_noise_level_f0_bits),
             aspx_hcb_noise_level_f0_bits, 1, 1, aspx_hcb_noise_level_f0_codes, 2, 2, 0);
    vlc_init(&aspx_codebook_noise_vlc[0][1], 9, sizeof(aspx_hcb_noise_level_df_bits),
             aspx_hcb_noise_level_df_bits, 1, 1, aspx_hcb_noise_level_df_codes, 4, 4, 0);
    vlc_init(&aspx_codebook_noise_vlc[0][2], 9, sizeof(aspx_hcb_noise_level_dt_bits),
             aspx_hcb_noise_level_dt_bits, 1, 1, aspx_hcb_noise_level_dt_codes, 2, 2, 0);

    vlc_init(&aspx_codebook_noise_vlc[1][0], 9, sizeof(aspx_hcb_noise_balance_f0_bits),
             aspx_hcb_noise_balance_f0_bits, 1, 1, aspx_hcb_noise_balance_f0_codes, 2, 2, 0);
    vlc_init(&aspx_codebook_noise_vlc[1][1], 9, sizeof(aspx_hcb_noise_balance_df_bits),
             aspx_hcb_noise_balance_df_bits, 1, 1, aspx_hcb_noise_balance_df_codes, 2, 2, 0);
    vlc_init(&aspx_codebook_noise_vlc[1][2], 9, sizeof(aspx_hcb_noise_balance_dt_bits),
             aspx_hcb_noise_balance_dt_bits, 1, 1, aspx_hcb_noise_balance_dt_codes, 2, 2, 0);

    vlc_init(&acpl_codebook_vlc[0][1][0], 9, sizeof(acpl_hcb_alpha_coarse_f0_bits),
             acpl_hcb_alpha_coarse_f0_bits, 1, 1, acpl_hcb_alpha_coarse_f0_codes, 2, 2, 0);
    vlc_init(&acpl_codebook_vlc[0][1][1], 9, sizeof(acpl_hcb_alpha_coarse_df_bits),
             acpl_hcb_alpha_coarse_df_bits, 1, 1, acpl_hcb_alpha_coarse_df_codes, 4, 4, 0);
    vlc_init(&acpl_codebook_vlc[0][1][2], 9, sizeof(acpl_hcb_alpha_coarse_dt_bits),
             acpl_hcb_alpha_coarse_dt_bits, 1, 1, acpl_hcb_alpha_coarse_dt_codes, 4, 4, 0);

    vlc_init(&acpl_codebook_vlc[0][0][0], 9, sizeof(acpl_hcb_alpha_fine_f0_bits),
             acpl_hcb_alpha_fine_f0_bits, 1, 1, acpl_hcb_alpha_fine_f0_codes, 2, 2, 0);
    vlc_init(&acpl_codebook_vlc[0][0][1], 9, sizeof(acpl_hcb_alpha_fine_df_bits),
             acpl_hcb_alpha_fine_df_bits, 1, 1, acpl_hcb_alpha_fine_df_codes, 4, 4, 0);
    vlc_init(&acpl_codebook_vlc[0][0][2], 9, sizeof(acpl_hcb_alpha_fine_dt_bits),
             acpl_hcb_alpha_fine_dt_bits, 1, 1, acpl_hcb_alpha_fine_dt_codes, 4, 4, 0);

    vlc_init(&acpl_codebook_vlc[1][1][0], 9, sizeof(acpl_hcb_beta_coarse_f0_bits),
             acpl_hcb_beta_coarse_f0_bits, 1, 1, acpl_hcb_beta_coarse_f0_codes, 1, 1, 0);
    vlc_init(&acpl_codebook_vlc[1][1][1], 9, sizeof(acpl_hcb_beta_coarse_df_bits),
             acpl_hcb_beta_coarse_df_bits, 1, 1, acpl_hcb_beta_coarse_df_codes, 1, 1, 0);
    vlc_init(&acpl_codebook_vlc[1][1][2], 9, sizeof(acpl_hcb_beta_coarse_dt_bits),
             acpl_hcb_beta_coarse_dt_bits, 1, 1, acpl_hcb_beta_coarse_dt_codes, 1, 1, 0);

    vlc_init(&acpl_codebook_vlc[1][0][0], 9, sizeof(acpl_hcb_beta_fine_f0_bits),
             acpl_hcb_beta_fine_f0_bits, 1, 1, acpl_hcb_beta_fine_f0_codes, 1, 1, 0);
    vlc_init(&acpl_codebook_vlc[1][0][1], 9, sizeof(acpl_hcb_beta_fine_df_bits),
             acpl_hcb_beta_fine_df_bits, 1, 1, acpl_hcb_beta_fine_df_codes, 4, 4, 0);
    vlc_init(&acpl_codebook_vlc[1][0][2], 9, sizeof(acpl_hcb_beta_fine_dt_bits),
             acpl_hcb_beta_fine_dt_bits, 1, 1, acpl_hcb_beta_fine_dt_codes, 4, 4, 0);

    vlc_init(&acpl_codebook_vlc[2][1][0], 9, sizeof(acpl_hcb_beta3_coarse_f0_bits),
             acpl_hcb_beta3_coarse_f0_bits, 1, 1, acpl_hcb_beta3_coarse_f0_codes, 1, 1, 0);
    vlc_init(&acpl_codebook_vlc[2][1][1], 9, sizeof(acpl_hcb_beta3_coarse_df_bits),
             acpl_hcb_beta3_coarse_df_bits, 1, 1, acpl_hcb_beta3_coarse_df_codes, 4, 4, 0);
    vlc_init(&acpl_codebook_vlc[2][1][2], 9, sizeof(acpl_hcb_beta3_coarse_dt_bits),
             acpl_hcb_beta3_coarse_dt_bits, 1, 1, acpl_hcb_beta3_coarse_dt_codes, 2, 2, 0);

    vlc_init(&acpl_codebook_vlc[2][0][0], 9, sizeof(acpl_hcb_beta3_fine_f0_bits),
             acpl_hcb_beta3_fine_f0_bits, 1, 1, acpl_hcb_beta3_fine_f0_codes, 1, 1, 0);
    vlc_init(&acpl_codebook_vlc[2][0][1], 9, sizeof(acpl_hcb_beta3_fine_df_bits),
             acpl_hcb_beta3_fine_df_bits, 1, 1, acpl_hcb_beta3_fine_df_codes, 4, 4, 0);
    vlc_init(&acpl_codebook_vlc[2][0][2], 9, sizeof(acpl_hcb_beta3_fine_dt_bits),
             acpl_hcb_beta3_fine_dt_bits, 1, 1, acpl_hcb_beta3_fine_dt_codes, 4, 4, 0);

    vlc_init(&acpl_codebook_vlc[3][1][0], 9, sizeof(acpl_hcb_gamma_coarse_f0_bits),
             acpl_hcb_gamma_coarse_f0_bits, 1, 1, acpl_hcb_gamma_coarse_f0_codes, 2, 2, 0);
    vlc_init(&acpl_codebook_vlc[3][1][1], 9, sizeof(acpl_hcb_gamma_coarse_df_bits),
             acpl_hcb_gamma_coarse_df_bits, 1, 1, acpl_hcb_gamma_coarse_df_codes, 4, 4, 0);
    vlc_init(&acpl_codebook_vlc[3][1][2], 9, sizeof(acpl_hcb_gamma_coarse_dt_bits),
             acpl_hcb_gamma_coarse_dt_bits, 1, 1, acpl_hcb_gamma_coarse_dt_codes, 4, 4, 0);

    vlc_init(&acpl_codebook_vlc[3][0][0], 9, sizeof(acpl_hcb_gamma_fine_f0_bits),
             acpl_hcb_gamma_fine_f0_bits, 1, 1, acpl_hcb_gamma_fine_f0_codes, 4, 4, 0);
    vlc_init(&acpl_codebook_vlc[3][0][1], 9, sizeof(acpl_hcb_gamma_fine_df_bits),
             acpl_hcb_gamma_fine_df_bits, 1, 1, acpl_hcb_gamma_fine_df_codes, 4, 4, 0);
    vlc_init(&acpl_codebook_vlc[3][0][2], 9, sizeof(acpl_hcb_gamma_fine_dt_bits),
             acpl_hcb_gamma_fine_dt_bits, 1, 1, acpl_hcb_gamma_fine_dt_codes, 4, 4, 0);
}

static av_cold int ac4_decode_init(AVCodecContext *avctx)
{
    AC4DecodeContext *s = avctx->priv_data;
    static AVOnce init_static_once = AV_ONCE_INIT;
    int ret;

    s->avctx = avctx;
    s->first_frame = 1;

    avctx->sample_fmt = AV_SAMPLE_FMT_FLTP;

    for (int j = 0; j < 8; j++) {
        const uint16_t *transf_lengths = transf_length_48khz[j];

        for (int i = 0; i < 5; i++) {
            int N_w = transf_lengths[i];
            float alpha = kbd_window_alpha[j][i];
            float scale = 1.f / N_w;

            if ((ret = av_tx_init(&s->tx_ctx[j][i], &s->tx_fn[j][i],
                                  AV_TX_FLOAT_MDCT, 1, N_w, &scale, AV_TX_FULL_IMDCT)))
                return ret;

            ff_kbd_window_init(s->kbd_window[j][i], alpha, N_w);
        }
    }

    for (int i = 0; i < 8192; i++)
        s->quant_lut[i] = powf(i, 4.f / 3.f);

    for (int i = 0; i < MAX_QMF_BANDS; i++) {
        for (int n = 0; n < MAX_QMF_BANDS*2; n++) {
            s->cos_atab[i][n] = cosf((M_PI/(MAX_QMF_BANDS*2))*(i+0.5)*(2*n-1));
            s->sin_atab[i][n] = sinf((M_PI/(MAX_QMF_BANDS*2))*(i+0.5)*(2*n-1));
            s->cos_stab[n][i] = cosf((M_PI/(MAX_QMF_BANDS*2))*(i+0.5)*(2*n-255)) / MAX_QMF_BANDS;
            s->sin_stab[n][i] = sinf((M_PI/(MAX_QMF_BANDS*2))*(i+0.5)*(2*n-255)) / MAX_QMF_BANDS;
        }
    }

    s->fdsp = avpriv_float_dsp_alloc(avctx->flags & AV_CODEC_FLAG_BITEXACT);
    if (!s->fdsp)
        return AVERROR(ENOMEM);

    ff_thread_once(&init_static_once, ac4_init_static);

    return 0;
}

static int variable_bits(GetBitContext *gb, int bits)
{
    int value = 0;
    int read_more;

    do {
        value += get_bits(gb, bits);
        read_more = get_bits1(gb);
        if (read_more) {
            value <<= bits;
            value += 1 << bits;
        }
    } while (read_more);

    return value;
}

static int check_sequence(AC4DecodeContext *s)
{
    if (s->sequence_counter > 1020) {
        av_log(s->avctx, AV_LOG_ERROR, "invalid sequence counter: %d\n", s->sequence_counter);
        return AVERROR_INVALIDDATA;
    }

    if (s->sequence_counter == s->sequence_counter_prev + 1)
        return 0;

    if (s->sequence_counter != 0 && s->sequence_counter_prev == 0)
        return 0;

    if (s->sequence_counter == 1 && s->sequence_counter_prev == 1020)
        return 0;

    if (s->sequence_counter == 0 && s->sequence_counter_prev == 0)
        return 0;

    av_log(s->avctx, AV_LOG_ERROR, "unexpected sequence counter: %d vs %d\n", s->sequence_counter, s->sequence_counter_prev);
    return AVERROR_INVALIDDATA;
}

static int frame_rate_multiply_info(AC4DecodeContext *s, PresentationInfo *p)
{
    GetBitContext *gb = &s->gbc;

    p->multiplier_bit = 0;

    switch (s->frame_rate_index) {
    case 2:
    case 3:
    case 4:
        p->multiplier = get_bits1(gb);
        if (p->multiplier)
            p->multiplier_bit = get_bits1(gb);
        p->frame_rate_factor = p->multiplier ? (p->multiplier_bit ? 4 : 2) : 1;
        break;
    case 0:
    case 1:
    case 7:
    case 8:
    case 9:
        p->multiplier = get_bits1(gb);
        p->frame_rate_factor = p->multiplier ? 2 : 1;
        break;
    default:
        p->frame_rate_factor = 1;
        break;
    }

    av_log(s->avctx, AV_LOG_DEBUG, "frame_rate_factor : %d\n", p->frame_rate_factor);

    return 0;
}

static int emdf_payloads_substream_info(AC4DecodeContext *s, EMDFInfo *e)
{
    GetBitContext *gb = &s->gbc;

    e->substream_index = get_bits(gb, 2);
    if (e->substream_index == 3)
        e->substream_index += variable_bits(gb, 2);

    return 0;
}

static int emdf_protection(AC4DecodeContext *s, EMDFInfo *e)
{
    GetBitContext *gb = &s->gbc;
    int first, second, skip = 0;

    first = get_bits(gb, 2);
    second = get_bits(gb, 2);

    if (first > 0)
        skip += 1 << (2 * (first - 1));
    if (second > 0)
        skip += 1 << (2 * (second - 1));

    skip_bits_long(gb, 8 * skip);

    return 0;
}

static int emdf_info(AC4DecodeContext *s, EMDFInfo *e)
{
    GetBitContext *gb = &s->gbc;

    e->version = get_bits(gb, 2);
    if (e->version == 3)
        e->version += variable_bits(gb, 2);
    e->key_id = get_bits(gb, 3);
    if (e->key_id == 7)
        e->key_id += variable_bits(gb, 3);

    if (get_bits1(gb))
        emdf_payloads_substream_info(s, e);

    emdf_protection(s, e);

    return 0;
}

static int content_type(AC4DecodeContext *s, PresentationInfo *p)
{
    GetBitContext *gb = &s->gbc;

    s->avctx->audio_service_type = get_bits(gb, 3);

    if (get_bits1(gb)) {
        if (get_bits1(gb)) {
            skip_bits(gb, 1);
            skip_bits(gb, 16);
        } else {
            int language_tag_bytes = get_bits(gb, 6);

            skip_bits_long(gb, 8 * language_tag_bytes);
        }
    }

    return 0;
}

static int ac4_hsf_ext_substream_info(AC4DecodeContext *s, SubstreamInfo *ssi,
                                      int substream_present)
{
    GetBitContext *gb = &s->gbc;

    if (substream_present) {
        ssi->hsf_ext_substream_index = get_bits(gb, 2);
        if (ssi->hsf_ext_substream_index == 3)
            ssi->hsf_ext_substream_index += variable_bits(gb, 2);
    }

    return 0;
}

static int ac4_substream_info(AC4DecodeContext *s, PresentationInfo *p,
                              SubstreamInfo *ssi)
{
    GetBitContext *gb = &s->gbc;

    ssi->sus_ver = 0;
    ssi->channel_mode = get_vlc2(gb, channel_mode_vlc.table, channel_mode_vlc.bits, 3);
    av_log(s->avctx, AV_LOG_DEBUG, "channel mode: %d\n", ssi->channel_mode);
    if (ssi->channel_mode < 0) {
        av_log(s->avctx, AV_LOG_ERROR, "invalid channel mode: %d\n", ssi->channel_mode);
        return AVERROR_INVALIDDATA;
    }

    if (ssi->channel_mode == 16)
        ssi->channel_mode += variable_bits(gb, 2);

    if (s->fs_index == 1 && get_bits1(gb))
        ssi->sf_multiplier = 1 + get_bits1(gb);
    av_log(s->avctx, AV_LOG_DEBUG, "sf_multiplier: %d\n", ssi->sf_multiplier);

    if (get_bits1(gb))
        ssi->bitrate_indicator = get_vlc2(gb, bitrate_indicator_vlc.table,
                                          bitrate_indicator_vlc.bits, 3);

    if (ssi->channel_mode == 7 ||
        ssi->channel_mode == 8 ||
        ssi->channel_mode == 9 ||
        ssi->channel_mode == 10) {
        ssi->add_ch_base = get_bits1(gb);
    }

    if (get_bits1(gb))
        content_type(s, p);

    for (int i = 0; i < p->frame_rate_factor; i++)
        ssi->iframe[i] = get_bits1(gb);

    ssi->substream_index = get_bits(gb, 2);
    if (ssi->substream_index == 3)
        ssi->substream_index += variable_bits(gb, 2);
    if (ssi->substream_index >= FF_ARRAY_ELEMS(s->substream_type))
        return AVERROR_INVALIDDATA;
    s->substream_type[ssi->substream_index] = ST_SUBSTREAM;
    av_log(s->avctx, AV_LOG_DEBUG, "substream index: %d\n", ssi->substream_index);

    return 0;
}

static int presentation_config_ext_info(AC4DecodeContext *s)
{
    GetBitContext *gb = &s->gbc;
    int n_skip_bytes;

    n_skip_bytes = get_bits(gb, 5);
    if (get_bits1(gb))
        n_skip_bytes += variable_bits(gb, 2) << 5;

    skip_bits_long(gb, 8 * n_skip_bytes);

    return 0;
}

static int ac4_presentation_info(AC4DecodeContext *s, PresentationInfo *p)
{
    GetBitContext *gb = &s->gbc;
    int ret;

    p->single_substream = get_bits1(gb);
    if (p->single_substream != 1) {
        p->presentation_config = get_bits(gb, 3);
        if (p->presentation_config == 7)
            p->presentation_config += variable_bits(gb, 2);
        av_log(s->avctx, AV_LOG_DEBUG, "presentation config: %d\n", p->presentation_config);
    }

    p->presentation_version = get_unary(gb, 0, 31);
    av_log(s->avctx, AV_LOG_DEBUG, "presentation version: %d\n", p->presentation_version);

    s->total_groups = 0;

    p->add_emdf_substreams = 0;
    if (p->single_substream != 1 && p->presentation_config == 6) {
        p->add_emdf_substreams = 1;
    } else {
        p->mdcompat = get_bits(gb, 3);

        if (get_bits1(gb))
            p->presentation_id = variable_bits(gb, 2);

        frame_rate_multiply_info(s, p);
        emdf_info(s, &p->emdf[0]);

        if (p->single_substream == 1) {
            ret = ac4_substream_info(s, p, &p->ssinfo[0]);
            if (ret < 0)
                return ret;
        } else {
            p->hsf_ext = get_bits1(gb);
            switch (p->presentation_config) {
            case 0:
                ret = ac4_substream_info(s, p, &p->ssinfo[0]);
                if (ret < 0)
                    return ret;
                if (p->hsf_ext) {
                    ret = ac4_hsf_ext_substream_info(s, &p->ssinfo[0], 1);
                    if (ret < 0)
                        return ret;
                }
                ret = ac4_substream_info(s, p, &p->ssinfo[1]);
                if (ret < 0)
                    return ret;
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                //FIXME
                break;
            default:
                presentation_config_ext_info(s);
            }
        }

        p->pre_virtualized = get_bits1(gb);
        p->add_emdf_substreams = get_bits1(gb);
    }

    if (p->add_emdf_substreams) {
        p->n_add_emdf_substreams = get_bits(gb, 2);
        if (p->n_add_emdf_substreams == 0)
            p->n_add_emdf_substreams = variable_bits(gb, 2) + 4;
        if (p->n_add_emdf_substreams > FF_ARRAY_ELEMS(p->emdf))
            return AVERROR_INVALIDDATA;

        for (int i = 0; i < p->n_add_emdf_substreams; i++)
            emdf_info(s, &p->emdf[i]);
    }

    return 0;
}

static int substream_index_table(AC4DecodeContext *s)
{
    GetBitContext *gb = &s->gbc;
    int size_present;

    s->nb_substreams = get_bits(gb, 2);
    if (s->nb_substreams == 0)
        s->nb_substreams = variable_bits(gb, 2) + 4;

    av_log(s->avctx, AV_LOG_DEBUG, "nb_substreams: %d\n", s->nb_substreams);

    if (s->nb_substreams > FF_ARRAY_ELEMS(s->substream_size))
        return AVERROR_INVALIDDATA;

    if (s->nb_substreams == 1) {
        size_present = get_bits1(gb);
    } else {
        size_present = 1;
    }

    if (size_present) {
        for (int i = 0; i < s->nb_substreams; i++) {
            int more_bits = get_bits1(gb);

            s->substream_size[i] = get_bits(gb, 10);
            if (more_bits)
                s->substream_size[i] += variable_bits(gb, 2) << 10;
            av_log(s->avctx, AV_LOG_DEBUG, "substream[%d] size: %d\n", i, s->substream_size[i]);
        }
    }

    return 0;
}

static int presentation_substream_info(AC4DecodeContext *s, PresentationSubstreamInfo *psi)
{
    GetBitContext *gb = &s->gbc;

    psi->alternative = get_bits1(gb);
    psi->pres_ndot = get_bits1(gb);
    psi->substream_index = get_bits(gb, 2);
    if (psi->substream_index == 3)
        psi->substream_index += variable_bits(gb, 2);

    if (psi->substream_index >= FF_ARRAY_ELEMS(s->substream_type))
        return AVERROR_INVALIDDATA;

    s->substream_type[psi->substream_index] = ST_PRESENTATION;
    av_log(s->avctx, AV_LOG_DEBUG, "presentation substream index: %d\n", psi->substream_index);

    return 0;
}

static int frame_rate_fractions_info(AC4DecodeContext *s, PresentationInfo *p)
{
    GetBitContext *gb = &s->gbc;

    p->frame_rate_fraction = 1;
    if (s->frame_rate_index >= 5 && s->frame_rate_index <= 9) {
        if (p->frame_rate_factor == 1) {
            if (get_bits1(gb))
                p->frame_rate_fraction = 2;
        }
    }

    if (s->frame_rate_index >= 10 && s->frame_rate_index <= 12) {
        if (get_bits1(gb)) {
            if (get_bits1(gb))
                p->frame_rate_fraction = 4;
            else
                p->frame_rate_fraction = 2;
        }
    }

    return 0;
}

static int oamd_substream_info(AC4DecodeContext *s, SubstreamGroupInfo *ssi,
                               int substreams_present)
{
    GetBitContext *gb = &s->gbc;

    skip_bits1(gb);
    if (substreams_present) {
        int substream_index = get_bits(gb, 2);
        if (substream_index == 3)
            substream_index += variable_bits(gb, 2);
    }

    return 0;
}

static int ac4_substream_info_chan(AC4DecodeContext *s, SubstreamGroupInfo *g,
                                   int substreams_present,
                                   int sus_ver)
{
    GetBitContext *gb = &s->gbc;
    SubstreamInfo *ssi = &g->ssinfo;

    ssi->sus_ver = sus_ver;
    ssi->channel_mode = get_vlc2(gb, channel_mode_vlc.table, channel_mode_vlc.bits, 3);
    av_log(s->avctx, AV_LOG_DEBUG, "channel mode: %d\n", ssi->channel_mode);
    if (ssi->channel_mode < 0)
        return AVERROR_INVALIDDATA;

    if (ssi->channel_mode == 16)
        ssi->channel_mode += variable_bits(gb, 2);

    if (ssi->channel_mode >= 5 && ssi->channel_mode <= 10) {
        for (int i = 0; i < s->nb_presentations; i++) {
            if (s->pinfo[i].presentation_version == 2) {
                ssi->channel_mode = 1; /* immersive stereo */
                break;
            }
        }
    }

    if (ssi->channel_mode == 11 ||
        ssi->channel_mode == 12 ||
        ssi->channel_mode == 13 ||
        ssi->channel_mode == 14) {
        ssi->back_channels_present = get_bits1(gb);
        ssi->centre_present        = get_bits1(gb);
        ssi->top_channels_present  = get_bits(gb, 2);
    }

    if (s->fs_index && get_bits1(gb))
        ssi->sf_multiplier = 1 + get_bits1(gb);
    av_log(s->avctx, AV_LOG_DEBUG, "sf_multiplier: %d\n", ssi->sf_multiplier);

    if (get_bits1(gb))
        ssi->bitrate_indicator = get_vlc2(gb, bitrate_indicator_vlc.table, bitrate_indicator_vlc.bits, 3);

    if (ssi->channel_mode == 7 ||
        ssi->channel_mode == 8 ||
        ssi->channel_mode == 9 ||
        ssi->channel_mode == 10)
        ssi->add_ch_base = get_bits1(gb);

    for (int i = 0; i < s->pinfo[0].frame_rate_factor; i++)
        ssi->iframe[i] = get_bits1(gb);

    if (substreams_present) {
        ssi->substream_index = get_bits(gb, 2);
        if (ssi->substream_index == 3)
            ssi->substream_index += variable_bits(gb, 2);
        av_log(s->avctx, AV_LOG_DEBUG, "substream index: %d\n", ssi->substream_index);
    }

    return 0;
}

static void ac4_bed_dyn_obj_assignment(AC4DecodeContext *s, int n_signals)
{
    GetBitContext *gb = &s->gbc;
    if (!get_bits1(gb)) { //b_dyn_objects_only
        if (get_bits1(gb)) { //b_isf
            skip_bits(gb, 3); //isf_config
        } else {
            if (get_bits1(gb)) { //b_ch_assign_code
                skip_bits(gb, 3); //bed_chan_assign_code
            } else {
                if (get_bits1(gb)) { //b_chan_assign_mask;
                    if (get_bits1(gb)) { //b_nonstd_bed_channel_assignment
                        skip_bits(gb, 17); //nonstd_bed_channel_assignment_mask
                    } else {
                        skip_bits(gb, 10); //std_bed_channel_assignment_mask
                    }
                } else {
                    int n_bed_signals;
                    if (n_signals > 1) {
                        n_bed_signals = get_bits_long(gb, ff_log2(n_signals)) + 1;
                    } else {
                        n_bed_signals = 1;
                    }
                    for (int i = 0; i < n_bed_signals; i++)
                        skip_bits(gb, 4);
                }
            }
        }
    }
}

static void ac4_oamd_common_data(AC4DecodeContext *s)
{
    GetBitContext *gb = &s->gbc;

    if (get_bits1(gb) == 0) { //b_default_screen_size_ratio;
        skip_bits(gb, 5); //master_screen_size_ratio_code
    }

    skip_bits(gb, 1); //b_bed_object_chan_distribute

    if (get_bits1(gb)) { //b_additional_data
        int add_data_bytes = get_bits(gb, 1) + 1;
        if (add_data_bytes == 2)
            add_data_bytes += variable_bits(gb, 2);
        skip_bits_long(gb, add_data_bytes * 8);
    }
}

static int ac4_substream_info_ajoc(AC4DecodeContext *s, SubstreamInfo *ssi, int substreams_present)
{
    GetBitContext *gb = &s->gbc;

    ssi->lfe = get_bits1(gb);
    ssi->static_dmx = get_bits1(gb);

    if (ssi->static_dmx) {
        ssi->n_fullband_dmx_signals = 5;
    } else {
        ssi->n_fullband_dmx_signals = get_bits(gb, 4) + 1;
        ac4_bed_dyn_obj_assignment(s, ssi->n_fullband_dmx_signals);
    }

    if (get_bits1(gb)) //b_oamd_common_data_present
        ac4_oamd_common_data(s);

    ssi->n_fullband_upmix_signals = get_bits(gb, 4) + 1;
    if (ssi->n_fullband_upmix_signals == 16)
        ssi->n_fullband_upmix_signals += variable_bits(gb, 3);
    if (ssi->n_fullband_upmix_signals > 4096) /* arbitary value to prevent slowdowns */
        return AVERROR_INVALIDDATA;

    ac4_bed_dyn_obj_assignment(s, ssi->n_fullband_upmix_signals);

    if (s->fs_index == 1) {
        if (get_bits1(gb)) //b_sf_multiplier;
            ssi->sf_multiplier = get_bits1(gb);
    }

    if (get_bits1(gb)) //b_bitrate_info;
        ssi->bitrate_indicator = get_vlc2(gb, bitrate_indicator_vlc.table, bitrate_indicator_vlc.bits, 1);

    for (int i = 0; i < s->pinfo[0].frame_rate_factor; i++)
        ssi->audio_ndot[i] = get_bits1(gb);

    if (substreams_present) {
       ssi->substream_index = get_bits(gb, 2);
       if (ssi->substream_index == 3)
           ssi->substream_index += variable_bits(gb, 2);
    }

    return 0;
}

static int ac4_substream_info_obj(AC4DecodeContext *s, SubstreamInfo *ssi, int substreams_present)
{
    GetBitContext *gb = &s->gbc;
    int n_objects_code;

    n_objects_code = get_bits(gb, 3);
    switch (n_objects_code) {
    case 0:
    case 1:
    case 2:
    case 3:
        ssi->n_objects = ssi->lfe + n_objects_code;
        break;
    case 4:
        ssi->n_objects = ssi->lfe + 5;
        break;
    default:
        return AVERROR_INVALIDDATA;
    }

    if (get_bits1(gb)) { //b_dynamic_objects;
        skip_bits1(gb); //b_lfe
    } else {
        if (get_bits1(gb)) { //b_bed_objects
            if (get_bits1(gb)) { //b_bed_start
                if (get_bits1(gb)) { //b_ch_assign_code
                    skip_bits(gb, 3); //bed_chan_assign_code
                } else {
                    if (get_bits1(gb)) { //b_nonstd_bed_channel_assignment
                        skip_bits(gb, 17); //nonstd_bed_channel_assignment_mask
                    } else {
                        skip_bits(gb, 10); //std_bed_channel_assignment_mask
                    }
                }
            }
        } else {
            if (get_bits1(gb)) { //b_isf
                if (get_bits1(gb)) { //b_isf_start
                    skip_bits(gb, 3); //isf_config
                }
            } else {
                int res_bytes = get_bits(gb, 4);
                skip_bits_long(gb, 8*res_bytes); //reserved_data
            }
        }
    }

    if (s->fs_index == 1) {
        if (get_bits1(gb)) //b_sf_multiplier;
            ssi->sf_multiplier = get_bits1(gb);
    }

    if (get_bits1(gb)) //b_bitrate_info;
        ssi->bitrate_indicator = get_vlc2(gb, bitrate_indicator_vlc.table, bitrate_indicator_vlc.bits, 1);

    for (int i = 0; i < s->pinfo[0].frame_rate_factor; i++) {
        skip_bits1(gb); //b_audio_ndot
    }

    if (substreams_present) {
       ssi->substream_index = get_bits(gb, 2);
       if (ssi->substream_index == 3)
           ssi->substream_index += variable_bits(gb, 2);
    }

    return 0;
}

static int ac4_substream_group_info(AC4DecodeContext *s,
                                    SubstreamGroupInfo *g)
{
    GetBitContext *gb = &s->gbc;
    int substreams_present;
    int n_lf_substreams;
    int hsf_ext;
    int sus_ver;
    int ret;

    substreams_present = get_bits1(gb);
    hsf_ext = get_bits1(gb);
    if (get_bits1(gb)) {
        n_lf_substreams = 1;
    } else {
        n_lf_substreams = get_bits(gb, 2) + 2;
        if (n_lf_substreams == 5) {
            n_lf_substreams += variable_bits(gb, 2);
            if (n_lf_substreams > 16) /* arbitary limit to prevent slowdowns */
                 return AVERROR_INVALIDDATA;
        }
    }

    g->channel_coded = get_bits1(gb);
    g->ajoc = 0;
    if (g->channel_coded) {
        for (int sus = 0; sus < n_lf_substreams; sus++) {
            if (s->version == 1) {
                sus_ver = get_bits1(gb);
            } else {
                sus_ver = 1;
            }

            ret = ac4_substream_info_chan(s, g, substreams_present, sus_ver);
            if (ret < 0)
                return ret;
            if (hsf_ext)
                ac4_hsf_ext_substream_info(s, &g->ssinfo, substreams_present);
        }
    } else {
        if (get_bits1(gb))
            oamd_substream_info(s, g, substreams_present);
        for (int sus = 0; sus < n_lf_substreams; sus++) {
            if (get_bits1(gb)) {
                g->ajoc = 1;
                if ((ret = ac4_substream_info_ajoc(s, &g->ssinfo, substreams_present)) < 0)
                    return ret;
            } else {
                ac4_substream_info_obj(s, &g->ssinfo, substreams_present);
            }
            if (hsf_ext)
                ac4_hsf_ext_substream_info(s, &g->ssinfo, substreams_present);
        }
    }

    if (get_bits1(gb))
        content_type(s, NULL);

    return 0;
}

static int ac4_sgi_specifier(AC4DecodeContext *s, SubstreamGroupInfo *g)
{
    GetBitContext *gb = &s->gbc;

    if (s->version == 1) {
        ac4_substream_group_info(s, g);
        //FIXME: g->group_index = ?
    } else {
        g->group_index = get_bits(gb, 3);
        if (g->group_index == 7)
            g->group_index += variable_bits(gb, 2);
    }

    s->total_groups = FFMAX(s->total_groups, g->group_index);

    return 0;
}

static int ac4_presentation_v1_info(AC4DecodeContext *s, PresentationInfo *p)
{
    GetBitContext *gb = &s->gbc;
    int single_substream_group, ret;

    single_substream_group = get_bits1(gb);
    if (single_substream_group != 1) {
        p->presentation_config = get_bits(gb, 3);
        if (p->presentation_config == 7)
            p->presentation_config += variable_bits(gb, 2);
    }
    if (s->version != 1)
        p->presentation_version = get_unary(gb, 0, 31);

    av_log(s->avctx, AV_LOG_DEBUG, "presentation version: %d\n", p->presentation_version);
    s->total_groups = 0;

    if (single_substream_group != 1 && p->presentation_config == 6) {
        p->add_emdf_substreams = 1;
        av_log(s->avctx, AV_LOG_DEBUG, "add_emdf_substreams\n");
    } else {
        if (s->version != 1)
            p->mdcompat = get_bits(gb, 3);

        if (get_bits1(gb))
            p->presentation_id = variable_bits(gb, 2);

        frame_rate_multiply_info(s, p);
        frame_rate_fractions_info(s, p);
        emdf_info(s, &p->emdf[0]);

        if (get_bits1(gb))
            p->enable_presentation = get_bits1(gb);

        if (single_substream_group == 1) {
            ac4_sgi_specifier(s, &s->ssgroup[0]);
            av_log(s->avctx, AV_LOG_DEBUG, "single substream group\n");
            p->n_substream_groups = 1;
        } else {
            p->multi_pid = get_bits1(gb);
            switch (p->presentation_config) {
            case 0:
                /* Music and Effects + Dialogue */
                ac4_sgi_specifier(s, &s->ssgroup[0]);
                ac4_sgi_specifier(s, &s->ssgroup[1]);
                p->n_substream_groups = 2;
                break;
            case 1:
                /* Main + DE */
                ac4_sgi_specifier(s, &s->ssgroup[0]);
                ac4_sgi_specifier(s, &s->ssgroup[1]);
                p->n_substream_groups = 1;
                break;
            case 2:
                /* Main + Associated Audio */
                ac4_sgi_specifier(s, &s->ssgroup[0]);
                ac4_sgi_specifier(s, &s->ssgroup[1]);
                p->n_substream_groups = 2;
                break;
            case 3:
                /* Music and Effects + Dialogue + Associated Audio */
                ac4_sgi_specifier(s, &s->ssgroup[0]);
                ac4_sgi_specifier(s, &s->ssgroup[1]);
                ac4_sgi_specifier(s, &s->ssgroup[2]);
                p->n_substream_groups = 3;
                break;
            case 4:
                /* Main + DE + Associated Audio */
                ac4_sgi_specifier(s, &s->ssgroup[0]);
                ac4_sgi_specifier(s, &s->ssgroup[1]);
                ac4_sgi_specifier(s, &s->ssgroup[2]);
                p->n_substream_groups = 2;
                break;
            case 5:
                /* Arbitrary number of roles and substream groups */
                p->n_substream_groups = get_bits(gb, 2) + 2;
                if (p->n_substream_groups == 5)
                    p->n_substream_groups += variable_bits(gb, 2);
                if (p->n_substream_groups > FF_ARRAY_ELEMS(s->ssgroup))
                    return AVERROR_INVALIDDATA;

                for (int sg = 0; sg < p->n_substream_groups; sg++)
                    ac4_sgi_specifier(s, &s->ssgroup[sg]);
                break;
            default:
                /* EMDF and other data */
                presentation_config_ext_info(s);
                break;
            }
        }
        p->pre_virtualized = get_bits1(gb);
        p->add_emdf_substreams = get_bits1(gb);
        if ((ret = presentation_substream_info(s, &p->psinfo)) < 0)
            return ret;
    }

    if (p->add_emdf_substreams) {
        p->n_add_emdf_substreams = get_bits(gb, 2);
        if (p->n_add_emdf_substreams == 0)
            p->n_add_emdf_substreams = variable_bits(gb, 2) + 4;
        if (p->n_add_emdf_substreams > FF_ARRAY_ELEMS(p->emdf))
            return AVERROR_INVALIDDATA;

        for (int i = 0; i < p->n_add_emdf_substreams; i++)
            emdf_info(s, &p->emdf[i]);
    }

    return 0;
}

static int get_num_ts_in_ats(int frame_length)
{
    if (frame_length <= 2048 && frame_length >= 1536)
        return 2;

    return 1;
}

static int ac4_toc(AC4DecodeContext *s)
{
    GetBitContext *gb = &s->gbc;
    int ret;

    s->version = get_bits(gb, 2);
    if (s->version == 3)
        s->version += variable_bits(gb, 2);
    av_log(s->avctx, AV_LOG_DEBUG, "bitstream version: %d\n", s->version);
    s->sequence_counter_prev = s->sequence_counter;
    s->sequence_counter = get_bits(gb, 10);
    av_log(s->avctx, AV_LOG_DEBUG, "sequence counter: %d\n", s->sequence_counter);

    s->wait_frames = get_bits1(gb);
    if (s->wait_frames) {
        s->nb_wait_frames = get_bits(gb, 3);
        if (s->nb_wait_frames > 0)
            skip_bits(gb, 2);
    }

    s->fs_index = get_bits1(gb);
    s->frame_rate_index = get_bits(gb, 4);
    av_log(s->avctx, AV_LOG_DEBUG, "frame_rate_index: %d\n", s->frame_rate_index);
    s->frame_len_base = frame_len_base_48khz[s->frame_rate_index];
    s->num_ts_in_ats = get_num_ts_in_ats(s->frame_len_base);
    s->frame_len_base_idx = frame_len_base_idx_48khz[s->frame_rate_index];
    av_log(s->avctx, AV_LOG_DEBUG, "frame_len_base: %d\n", s->frame_len_base);
    s->resampling_ratio = resampling_ratios[s->frame_rate_index];
    s->num_qmf_timeslots = s->frame_len_base / MAX_QMF_BANDS;
    s->num_aspx_timeslots = s->num_qmf_timeslots / s->num_ts_in_ats;
    av_log(s->avctx, AV_LOG_DEBUG, "num_aspx_timeslots: %d\n", s->num_aspx_timeslots);
    s->ts_offset_hfgen = 3 * s->num_ts_in_ats;
    s->iframe_global = get_bits1(gb);
    if (s->iframe_global) {
        s->have_iframe = 1;
    } else {
        ret = check_sequence(s);
        if (ret < 0)
            return ret;
    }
    if (get_bits1(gb)) {
        s->nb_presentations = 1;
    } else {
        if (get_bits1(gb)) {
            s->nb_presentations = 2 + variable_bits(gb, 2);
            if (s->nb_presentations > FF_ARRAY_ELEMS(s->pinfo))
                return AVERROR_INVALIDDATA;
        } else {
            s->nb_presentations = 0;
        }
    }

    s->payload_base = 0;
    if (get_bits1(gb)) {
        s->payload_base = get_bits(gb, 5) + 1;
        if (s->payload_base == 0x20) {
            s->payload_base += variable_bits(gb, 3);
        }
    }

    av_log(s->avctx, AV_LOG_DEBUG, "presentations: %d\n", s->nb_presentations);

    if (s->version <= 1) {
        for (int i = 0; i < s->nb_presentations; i++) {
            ret = ac4_presentation_info(s, &s->pinfo[i]);
            if (ret < 0)
                return ret;
        }
    } else {
        if (get_bits1(gb)) {
            s->short_program_id = get_bits(gb, 16);
            if (get_bits1(gb)) {
                skip_bits_long(gb, 16 * 8);
            }
        }

        for (int i = 0; i < s->nb_presentations; i++) {
            ret = ac4_presentation_v1_info(s, &s->pinfo[i]);
            if (ret < 0)
                return ret;
        }

        if (s->total_groups + 1 >= FF_ARRAY_ELEMS(s->ssgroup))
            return AVERROR_INVALIDDATA;

        av_log(s->avctx, AV_LOG_DEBUG, "total_groups: %d\n", s->total_groups + 1);
        for (int i = 0; i <= s->total_groups; i++) {
            ret = ac4_substream_group_info(s, &s->ssgroup[i]);
            if (ret < 0)
                return ret;
        }
    }

    substream_index_table(s);

    align_get_bits(gb);

    av_log(s->avctx, AV_LOG_DEBUG, "TOC size: %d\n", get_bits_count(gb) >> 3);

    return 0;
}

static int sb_to_pb(int acpl_num_param_bands_id, int acpl_qmf_band)
{
    if (acpl_qmf_band >= 0 &&
        acpl_qmf_band <= 8)
        return qmf_subbands[acpl_qmf_band][acpl_num_param_bands_id];
    if (acpl_qmf_band >= 9 &&
        acpl_qmf_band <= 10)
        return qmf_subbands[9][acpl_num_param_bands_id];
    if (acpl_qmf_band >= 11 &&
        acpl_qmf_band <= 13)
        return qmf_subbands[10][acpl_num_param_bands_id];
    if (acpl_qmf_band >= 14 &&
        acpl_qmf_band <= 17)
        return qmf_subbands[11][acpl_num_param_bands_id];
    if (acpl_qmf_band >= 18 &&
        acpl_qmf_band <= 22)
        return qmf_subbands[12][acpl_num_param_bands_id];
    if (acpl_qmf_band >= 23 &&
        acpl_qmf_band <= 34)
        return qmf_subbands[13][acpl_num_param_bands_id];
    if (acpl_qmf_band >= 35 &&
        acpl_qmf_band <= 63)
        return qmf_subbands[14][acpl_num_param_bands_id];
    return 0;
}

static int acpl_config_1ch(AC4DecodeContext *s, Substream *ss, int mode)
{
    GetBitContext *gb = &s->gbc;

    ss->acpl_qmf_band = 0;
    ss->acpl_param_band = 0;
    ss->acpl_num_param_bands_id = get_bits(gb, 2);
    ss->acpl_quant_mode[0] = get_bits1(gb);
    if (mode == ACPL_PARTIAL) {
        ss->acpl_qmf_band = get_bits(gb, 3) + 1;
        ss->acpl_param_band = sb_to_pb(ss->acpl_num_param_bands_id, ss->acpl_qmf_band);
    }

    return 0;
}

static int acpl_config_2ch(AC4DecodeContext *s, Substream *ss)
{
    GetBitContext *gb = &s->gbc;

    ss->acpl_qmf_band = 0;
    ss->acpl_param_band = 0;
    ss->acpl_num_param_bands_id = get_bits(gb, 2);
    ss->acpl_quant_mode[0] = get_bits1(gb);
    ss->acpl_quant_mode[1] = get_bits1(gb);

    return 0;
}

static void aspx_config(AC4DecodeContext *s, Substream *ss)
{
    GetBitContext *gb = &s->gbc;

    ss->aspx_quant_mode_env = get_bits1(gb);
    ss->prev_aspx_start_freq = ss->aspx_start_freq;
    ss->aspx_start_freq = get_bits(gb, 3);
    ss->prev_aspx_stop_freq = ss->aspx_stop_freq;
    ss->aspx_stop_freq = get_bits(gb, 2);
    ss->prev_aspx_master_freq_scale = ss->aspx_master_freq_scale;
    ss->aspx_master_freq_scale = get_bits1(gb);
    ss->aspx_interpolation = get_bits1(gb);
    ss->aspx_preflat = get_bits1(gb);
    ss->aspx_limiter = get_bits1(gb);
    ss->aspx_noise_sbg = get_bits(gb, 2);
    ss->aspx_num_env_bits_fixfix = get_bits1(gb);
    ss->aspx_freq_res_mode = get_bits(gb, 2);
}

static int get_transfer_length_from_idx(AC4DecodeContext *s, int idx)
{
    const uint16_t *transf_length_tab;

    switch (s->frame_len_base) {
    case 2048:
        transf_length_tab = transf_length_48khz_2048;
        break;
    case 1920:
        transf_length_tab = transf_length_48khz_1920;
        break;
    case 1536:
        transf_length_tab = transf_length_48khz_1536;
        break;
    case 1024:
        transf_length_tab = transf_length_48khz_1024;
        break;
    case 960:
        transf_length_tab = transf_length_48khz_960;
        break;
    case 768:
        transf_length_tab = transf_length_48khz_768;
        break;
    case 512:
        transf_length_tab = transf_length_48khz_512;
        break;
    case 384:
        transf_length_tab = transf_length_48khz_384;
        break;
    default:
        av_assert0(0);
    }

    return transf_length_tab[idx];
}

static int asf_transform_info(AC4DecodeContext *s, Substream *ss,
                              SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;

    if (s->frame_len_base >= 1536) {
        ssch->scp.long_frame = get_bits1(gb);
        if (ssch->scp.long_frame == 0) {
            ssch->scp.transf_length_idx[0] = get_bits(gb, 2);
            ssch->scp.transf_length_idx[1] = get_bits(gb, 2);
            ssch->scp.transf_length[0] = get_transfer_length_from_idx(s, ssch->scp.transf_length_idx[0]);
            ssch->scp.transf_length[1] = get_transfer_length_from_idx(s, ssch->scp.transf_length_idx[1]);
        } else {
            ssch->scp.transf_length[0] = s->frame_len_base;
            ssch->scp.transf_length[1] = 0;
        }
    } else {
        ssch->scp.long_frame = 0;
        ssch->scp.transf_length_idx[0] = get_bits(gb, 2);
        ssch->scp.transf_length[0] = get_transfer_length_from_idx(s, ssch->scp.transf_length_idx[0]);
    }

    return 0;
}

static int get_msfbl_bits(int transf_length)
{
    if (transf_length <= 2048 && transf_length >= 1536)
        return 3;

    return 2;
}

static int get_grp_bits(AC4DecodeContext *s, SubstreamChannel *ssch)
{
    if (s->frame_len_base >= 1536 && ssch->scp.long_frame == 1)
        return 0;

    if (s->frame_len_base >= 1536 && ssch->scp.long_frame == 0) {
        if (ssch->scp.transf_length_idx[0] >= FF_ARRAY_ELEMS(n_grp_bits_a))
            return AVERROR_INVALIDDATA;
        if (ssch->scp.transf_length_idx[1] >= FF_ARRAY_ELEMS(n_grp_bits_a[0]))
            return AVERROR_INVALIDDATA;

        return n_grp_bits_a[ssch->scp.transf_length_idx[0]][ssch->scp.transf_length_idx[1]];
    }

    if (s->frame_len_base < 1536 && s->frame_len_base > 512) {
        if (ssch->scp.transf_length_idx[0] >= FF_ARRAY_ELEMS(n_grp_bits_b))
            return AVERROR_INVALIDDATA;

        return n_grp_bits_b[ssch->scp.transf_length_idx[0]];
    }

    if (s->frame_len_base <= 512) {
        if (ssch->scp.transf_length_idx[0] >= FF_ARRAY_ELEMS(n_grp_bits_b))
            return AVERROR_INVALIDDATA;

        return n_grp_bits_c[ssch->scp.transf_length_idx[0]];
    }

    return 0;
}

static int get_msfb_bits(int transf_length)
{
    if (transf_length <= 2048 && transf_length >= 384)
        return 6;

    if (transf_length <= 256 && transf_length >= 192)
        return 5;

    return 4;
}

static int get_side_bits(int transf_length)
{
    if (transf_length <= 2048 && transf_length >= 480)
        return 5;

    if (transf_length <= 384 && transf_length >= 240)
        return 4;

    return 3;
}

static int get_max_sfb(AC4DecodeContext *s, SubstreamChannel *ssch,
                       int g)
{
    int idx = 0;

    if (s->frame_len_base >= 1536 && (ssch->scp.long_frame == 0) &&
        (ssch->scp.transf_length_idx[0] != ssch->scp.transf_length_idx[1])) {
        int num_windows_0 = 1 << (3 - ssch->scp.transf_length_idx[0]);

        if (g >= ssch->scp.window_to_group[num_windows_0])
            idx = 1;
    }

    if ((ssch->scp.side_limited == 1) ||
        (ssch->scp.dual_maxsfb == 1 && ssch->scp.side_channel == 1))  {
        return ssch->scp.max_sfb_side[idx];
    } else {
        return ssch->scp.max_sfb[idx];
    }
}

static int get_transf_length(AC4DecodeContext *s, SubstreamChannel *ssch, int g, int *idx)
{
    const uint16_t *transf_length_tab;

    switch (s->frame_len_base) {
    case 2048:
        transf_length_tab = transf_length_48khz_2048;
        break;
    case 1920:
        transf_length_tab = transf_length_48khz_1920;
        break;
    case 1536:
        transf_length_tab = transf_length_48khz_1536;
        break;
    case 1024:
        transf_length_tab = transf_length_48khz_1024;
        break;
    case 960:
        transf_length_tab = transf_length_48khz_960;
        break;
    case 768:
        transf_length_tab = transf_length_48khz_768;
        break;
    case 512:
        transf_length_tab = transf_length_48khz_512;
        break;
    case 384:
        transf_length_tab = transf_length_48khz_384;
        break;
    default:
        av_assert0(0);
    }

    if (s->frame_len_base >= 1536) {
        if (ssch->scp.long_frame == 0) {
            int num_windows_0 = 1 << (3 - ssch->scp.transf_length_idx[0]);

            if (g < ssch->scp.window_to_group[num_windows_0]) {
                if (idx)
                    *idx = ssch->scp.transf_length_idx[0];
                return transf_length_tab[ssch->scp.transf_length_idx[0]];
            } else {
                if (idx)
                    *idx = ssch->scp.transf_length_idx[1];
                return transf_length_tab[ssch->scp.transf_length_idx[1]];
            }
        } else {
            if (idx)
                *idx = 4;
            return s->frame_len_base; // long frame, the transform length equals to frame_length
        }
    } else {
        if (idx)
            *idx = ssch->scp.transf_length_idx[0];
        return transf_length_tab[ssch->scp.transf_length_idx[0]];
    }
}

static int get_sfb_size(int transf_length)
{
    switch (transf_length) {
    case 2048:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_2048);
        break;
    case 1920:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_1920);
        break;
    case 1536:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_1536);
        break;
    case 1024:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_1024);
        break;
    case 960:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_960);
        break;
    case 768:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_768);
        break;
    case 512:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_512);
        break;
    case 480:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_480);
        break;
    case 384:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_384);
        break;
    case 256:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_256);
        break;
    case 240:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_240);
        break;
    case 192:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_192);
        break;
    case 128:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_128);
        break;
    case 120:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_120);
        break;
    case 96:
        return FF_ARRAY_ELEMS(sfb_offset_48khz_96);
        break;
    default:
        av_assert0(0);
    }
    return 0;
}

static const uint16_t *get_sfb_offset(int transf_length)
{
    switch (transf_length) {
    case 2048:
        return sfb_offset_48khz_2048;
        break;
    case 1920:
        return sfb_offset_48khz_1920;
        break;
    case 1536:
        return sfb_offset_48khz_1536;
        break;
    case 1024:
        return sfb_offset_48khz_1024;
        break;
    case 960:
        return sfb_offset_48khz_960;
        break;
    case 768:
        return sfb_offset_48khz_768;
        break;
    case 512:
        return sfb_offset_48khz_512;
        break;
    case 480:
        return sfb_offset_48khz_480;
        break;
    case 384:
        return sfb_offset_48khz_384;
        break;
    case 256:
        return sfb_offset_48khz_256;
        break;
    case 240:
        return sfb_offset_48khz_240;
        break;
    case 192:
        return sfb_offset_48khz_192;
        break;
    case 128:
        return sfb_offset_48khz_128;
        break;
    case 120:
        return sfb_offset_48khz_120;
        break;
    case 96:
        return sfb_offset_48khz_96;
        break;
    default:
        av_assert0(0);
    }

    return NULL;
}

/*
static int num_sfb_96(int transf_length)
{
    if (transf_length >= 4096)
        return 79;
    else if (transf_length >= 3840)
        return 76;
    else if (transf_length >= 3072)
        return 67;
    else if (transf_length >= 2048)
        return 57;
    else if (transf_length >= 1920)
        return 57;
    else if (transf_length >= 1536)
        return 49;
    else if (transf_length >= 1024)
        return 44;
    else if (transf_length >= 920)
        return 44;
    else if (transf_length >= 768)
        return 39;
    else if (transf_length >= 512)
        return 28;
    else if (transf_length >= 480)
        return 28;
    else if (transf_length >= 384)
        return 24;
    else if (transf_length >= 256)
        return 22;
    else if (transf_length >= 240)
        return 22;
    else
        return 18;
}
*/

static int num_sfb_48(int transf_length)
{
    switch (transf_length) {
    case 2048:
        return 63; break;
    case 1920:
        return 61; break;
    case 1536:
        return 55; break;
    case 1024:
    case 960:
        return 49; break;
    case 768:
        return 43; break;
    case 512:
    case 480:
        return 36; break;
    case 384:
        return 33; break;
    case 256:
    case 240:
        return 20; break;
    case 192:
        return 18; break;
    case 128:
    case 120:
        return 14; break;
    case 96:
        return 12; break;
    default:
        av_assert0(0);
    }

    return 0;
}

static int asf_psy_elements(AC4DecodeContext *s, Substream *ss,
                            SubstreamChannel *ssch, int n_grp_bits)
{
    int group_offset, win_offset, win;

    ssch->scp.num_windows = 1;
    ssch->scp.num_window_groups = 1;
    ssch->scp.window_to_group[0] = 0;

    if (ssch->scp.long_frame == 0) {
        ssch->scp.num_windows = n_grp_bits + 1;
        if (ssch->scp.different_framing) {
            int num_windows_0 = 1 << (3 - ssch->scp.transf_length_idx[0]);

            for (int i = n_grp_bits; i >= num_windows_0; i--) {
                ssch->scp.scale_factor_grouping[i] = ssch->scp.scale_factor_grouping[i - 1];
            }

            ssch->scp.scale_factor_grouping[num_windows_0 - 1] = 0;
            ssch->scp.num_windows++;
        }

        for (int i = 0; i < ssch->scp.num_windows - 1; i++) {
            if (ssch->scp.scale_factor_grouping[i] == 0)
                ssch->scp.num_window_groups++;

            ssch->scp.window_to_group[i + 1] = ssch->scp.num_window_groups - 1;
        }
    }

    group_offset = 0;
    win_offset = 0;
    win = 0;
    memset(ssch->offset2sfb, 0, sizeof(ssch->offset2sfb));
    memset(ssch->offset2g, 0, sizeof(ssch->offset2g));

    av_assert2(ssch->scp.num_window_groups <= 16);

    for (int g = 0; g < ssch->scp.num_window_groups; g++) {
        int transf_length_g = get_transf_length(s, ssch, g, NULL);
        const uint16_t *sfb_offset = get_sfb_offset(transf_length_g);
        const int sfb_max_size = get_sfb_size(transf_length_g);
        int max_sfb;

        if (!sfb_offset)
            return AVERROR_INVALIDDATA;

        ssch->scp.num_win_in_group[g] = 0;
        for (int w = 0; w < ssch->scp.num_windows; w++) {
            if (ssch->scp.window_to_group[w] == g)
                ssch->scp.num_win_in_group[g]++;
        }

        max_sfb = get_max_sfb(s, ssch, g);
        if (max_sfb >= sfb_max_size) {
            av_log(s->avctx, AV_LOG_ERROR, "max_sfb=%d > sfb_max_size=%d\n", max_sfb, sfb_max_size);
            return AVERROR_INVALIDDATA;
        }
        for (int sfb = 0; sfb < max_sfb; sfb++)
            ssch->sect_sfb_offset[g][sfb] = group_offset + sfb_offset[sfb] * ssch->scp.num_win_in_group[g];
        group_offset += sfb_offset[max_sfb] * ssch->scp.num_win_in_group[g];
        ssch->sect_sfb_offset[g][max_sfb] = group_offset;
        for (int sfb = 0; sfb < max_sfb; sfb++) {
            av_assert2(sfb + 1 < FF_ARRAY_ELEMS(ssch->sect_sfb_offset[g]));
            for (int j = ssch->sect_sfb_offset[g][sfb]; j < ssch->sect_sfb_offset[g][sfb+1]; j++) {
                av_assert2(j >= 0 && j < FF_ARRAY_ELEMS(ssch->offset2sfb));
                av_assert2(j >= 0 && j < FF_ARRAY_ELEMS(ssch->offset2g));
                if (j >= FF_ARRAY_ELEMS(ssch->offset2g))
                    return AVERROR_INVALIDDATA;
                ssch->offset2sfb[j] = sfb;
                ssch->offset2g[j] = g;
            }
        }

        for (int w = 0; w < ssch->scp.num_win_in_group[g]; w++) {
            av_assert2(win + w >= 0 && win + w < FF_ARRAY_ELEMS(ssch->win_offset));
            ssch->win_offset[win + w] = win_offset;
            win_offset += transf_length_g;
        }
        win += ssch->scp.num_win_in_group[g];
    }
    av_assert2(win_offset == s->frame_len_base);

    av_log(s->avctx, AV_LOG_DEBUG, "long_frame: %d\n", ssch->scp.long_frame);
    av_log(s->avctx, AV_LOG_DEBUG, "different_framing: %d\n", ssch->scp.different_framing);
    av_log(s->avctx, AV_LOG_DEBUG, "num_windows: %d\n", ssch->scp.num_windows);
    av_log(s->avctx, AV_LOG_DEBUG, "num_window_groups: %d\n", ssch->scp.num_window_groups);
    av_log(s->avctx, AV_LOG_DEBUG, "transf_lengths:");
    for (int g = 0; g < ssch->scp.num_window_groups; g++) {
        av_log(s->avctx, AV_LOG_DEBUG, " %d", get_transf_length(s, ssch, g, NULL));
    }
    av_log(s->avctx, AV_LOG_DEBUG, "\n");
    av_log(s->avctx, AV_LOG_DEBUG, "num_win_in_group:");
    for (int g = 0; g < ssch->scp.num_window_groups; g++) {
        av_log(s->avctx, AV_LOG_DEBUG, " %d", ssch->scp.num_win_in_group[g]);
    }
    av_log(s->avctx, AV_LOG_DEBUG, "\n");

    return 0;
}

static int asf_psy_info(AC4DecodeContext *s, Substream *ss,
                        SubstreamChannel *ssch,
                        int dual_maxsfb, int side_limited)
{
    GetBitContext *gb = &s->gbc;
    int n_side_bits = get_side_bits(ssch->scp.transf_length[0]);
    int n_msfb_bits = get_msfb_bits(ssch->scp.transf_length[0]);
    int n_grp_bits = get_grp_bits(s, ssch);

    if (n_grp_bits < 0)
        return n_grp_bits;

    ssch->scp.different_framing = 0;
    if ((s->frame_len_base >= 1536) && (ssch->scp.long_frame == 0) &&
        (ssch->scp.transf_length_idx[0] != ssch->scp.transf_length_idx[1])) {
        ssch->scp.different_framing = 1;
    }

    if (side_limited) {
        ssch->scp.max_sfb_side[0] = get_bits(gb, n_side_bits);
    } else {
        ssch->scp.max_sfb[0] = get_bits(gb, n_msfb_bits);
        if (dual_maxsfb)
            ssch->scp.max_sfb_side[0] = get_bits(gb, n_msfb_bits);
    }

    if (ssch->scp.different_framing) {
        n_side_bits = get_side_bits(ssch->scp.transf_length[1]);
        n_msfb_bits = get_msfb_bits(ssch->scp.transf_length[1]);

        if (side_limited) {
            ssch->scp.max_sfb_side[1] = get_bits(gb, n_side_bits);
        } else {
            ssch->scp.max_sfb[1] = get_bits(gb, n_msfb_bits);
            if (dual_maxsfb)
                ssch->scp.max_sfb_side[1] = get_bits(gb, n_msfb_bits);
        }
    }

    memset(ssch->scp.scale_factor_grouping, 0, sizeof(ssch->scp.scale_factor_grouping));
    for (int i = 0; i < n_grp_bits; i++)
        ssch->scp.scale_factor_grouping[i] = get_bits1(gb);

    return asf_psy_elements(s, ss, ssch, n_grp_bits);
}

static int sf_info(AC4DecodeContext *s, Substream *ss,
                   int ch_id, int spec_frontend, int dual_maxsfb,
                   int side_limited)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    int ret = 0;

    ssch->scp.dual_maxsfb = dual_maxsfb;
    ssch->scp.side_limited = side_limited;

    if (spec_frontend == SF_ASF) {
        asf_transform_info(s, ss, ssch);
        ret = asf_psy_info(s, ss, ssch, dual_maxsfb, side_limited);
    }

    return ret;
}

static int sap_data(AC4DecodeContext *s, Substream *ss, SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;

    memset(ssch->sap_coeff_used, 0, sizeof(ssch->sap_coeff_used));

    if (!get_bits1(gb)) {
        for (int g = 0; g < ssch->scp.num_window_groups; g++) {
            int max_sfb_g = get_max_sfb(s, ssch, g);

            for (int sfb = 0; sfb < max_sfb_g; sfb += 2) {
                ssch->sap_coeff_used[g][sfb] = get_bits1(gb);
                if (sfb + 1 < max_sfb_g)
                    ssch->sap_coeff_used[g][sfb + 1] = ssch->sap_coeff_used[g][sfb];
            }
        }
    } else {
        for (int g = 0; g < ssch->scp.num_window_groups; g++) {
            int max_sfb_g = get_max_sfb(s, ssch, g);

            for (int sfb = 0; sfb < max_sfb_g; sfb++)
                ssch->sap_coeff_used[g][sfb] = 1;
        }
    }

    ssch->delta_code_time = 0;
    if (ssch->scp.num_window_groups != 1)
        ssch->delta_code_time = get_bits1(gb);

    av_assert2(ssch->scp.num_window_groups <= FF_ARRAY_ELEMS(ssch->dpcm_alpha_q));
    av_assert2(ssch->scp.num_window_groups <= FF_ARRAY_ELEMS(ssch->sap_coeff_used));

    for (int g = 0; g < ssch->scp.num_window_groups; g++) {
        int max_sfb_g = get_max_sfb(s, ssch, g);

        av_assert2(max_sfb_g <= FF_ARRAY_ELEMS(ssch->dpcm_alpha_q[0]));
        av_assert2(max_sfb_g <= FF_ARRAY_ELEMS(ssch->sap_coeff_used[0]));
        for (int sfb = 0; sfb < max_sfb_g; sfb += 2) {
            if (ssch->sap_coeff_used[g][sfb]) {
                ssch->dpcm_alpha_q[g][sfb] = get_vlc2(gb, scale_factors_vlc.table,
                                                      scale_factors_vlc.bits, 3);
                if (ssch->dpcm_alpha_q[g][sfb] < 0)
                    return AVERROR_INVALIDDATA;
            }
        }
    }

    return 0;
}

static int ssf_st_data(AC4DecodeContext *s, Substream *ss,
                       SubstreamChannel *ssch, int iframe)
{
    GetBitContext *gb = &s->gbc;
    int num_blocks;

    ssch->env_idx[0] = get_bits(gb, 5);
    if (iframe == 1 && ssch->stride_flag == SHORT_STRIDE)
        get_bits(gb, 5);

    if (ssch->stride_flag == SHORT_STRIDE) {
        for (int block = 0; block < 4; block++)
            ssch->gain_bits[block] = get_bits(gb, 4);
    }

    num_blocks = (ssch->stride_flag == SHORT_STRIDE) ? 4 : 1;

    for (int block = 0; block < num_blocks; block++) {
        if (block >= ssch->start_block && block < ssch->end_block) {
            if (ssch->predictor_presence[block]) {
                if (ssch->delta[block])
                    ssch->predictor_lag_delta[block] = get_bits(gb, 4);
                else
                    ssch->predictor_lag[block] = get_bits(gb, 9);
            }
        }
        ssch->variance_preserving[block] = get_bits1(gb);
        ssch->alloc_offset[block] = get_bits(gb, 5);
    }

    return 0;
}

static int ac_init(AC4DecodeContext *s, ACState *acs)
{
    GetBitContext *gb = &s->gbc;

    acs->ui_model_bits = SSF_MODEL_BITS;
    acs->ui_model_unit = SSF_MODEL_UNIT;
    acs->ui_range_bits = SSF_RANGE_BITS;
    acs->ui_threshold_large = SSF_THRESHOLD_LARGE;
    acs->ui_threshold_small = SSF_THRESHOLD_SMALL;

    acs->ui_low = 0;
    acs->ui_range = SSF_THRESHOLD_LARGE;

    acs->ui_offset = get_bits1(gb);
    for (int index = 1; index < acs->ui_range_bits; index++) {
        uint32_t ui_tmp = get_bits1(gb);

        acs->ui_offset <<= 1;
        acs->ui_offset += ui_tmp;
    }

    acs->ui_offset2 = acs->ui_offset;

    return 0;
}

static int32_t ac_decode(AC4DecodeContext *s, uint32_t cdf_low,
                         uint32_t cdf_high, ACState *acs)
{
    GetBitContext *gb = &s->gbc;
    uint32_t ui_tmp1, ui_tmp2;
    uint32_t ui_range;

    ui_range = acs->ui_range >> acs->ui_model_bits;
    ui_tmp1 = ui_range * cdf_low;
    acs->ui_offset = acs->ui_offset - ui_tmp1;

    if (cdf_high < acs->ui_model_unit) {
        ui_tmp2 = cdf_high - cdf_low;
        acs->ui_range = ui_range * ui_tmp2;
    } else {
        acs->ui_range = acs->ui_range - ui_tmp1;
    }

    // denormalize
    while (acs->ui_range <= acs->ui_threshold_small) {
        /* Read a single bit from the bitstream */
        uint32_t ui_tmp1 = get_bits1(gb);

        acs->ui_range <<= 1;
        acs->ui_offset <<= 1;
        acs->ui_offset += ui_tmp1;
        acs->ui_offset2 <<= 1;
        if (acs->ui_offset & 1)
            acs->ui_offset2++;
    }

    return 0;
}

static int32_t ac_decode_finish(ACState *acs)

{
    uint32_t fact, ui_bits, ui_val;
    uint32_t ui_tmp1, ui_tmp2, ui_rev_idx;

    acs->ui_low = acs->ui_offset & (acs->ui_threshold_large-1);

    ui_tmp1 = acs->ui_threshold_large - acs->ui_offset;

    acs->ui_low +=ui_tmp1;

    for (int bit_idx = 1; bit_idx <= acs->ui_range_bits; bit_idx++) {
        ui_rev_idx = acs->ui_range_bits - bit_idx;
        fact = 1U << ui_rev_idx;
        fact = fact - 1U;
        ui_tmp1 = acs->ui_low + fact;
        ui_bits = ui_tmp1 >> ui_rev_idx;
        ui_val = ui_bits << ui_rev_idx;
        ui_tmp1 = ui_val + fact;
        ui_tmp2 = acs->ui_range - 1U;
        ui_tmp2 += acs->ui_low;

        if ((acs->ui_low <= ui_val) && (ui_tmp1 <= ui_tmp2))
            break;
    }

    return 0;
}

static int ssf_ac_data(AC4DecodeContext *s, Substream *ss,
                       SubstreamChannel *ssch)
{
    int ret;

    ret = ac_init(s, &ssch->acs);
    if (ret < 0)
        return ret;

    // Not sure if passing in ui_low and ui_range are the right values
    // for the cdf_low and cdf_high parameters - but it seems to work.
    ret = ac_decode(s, ssch->acs.ui_low, ssch->acs.ui_range, &ssch->acs);
    if (ret < 0)
        return ret;

    return ac_decode_finish(&ssch->acs);
}

static int ssf_granule(AC4DecodeContext *s, Substream *ss,
                       SubstreamChannel *ssch, int iframe)
{
    GetBitContext *gb = &s->gbc;
    int ret;

    ssch->stride_flag = get_bits1(gb);
    if (iframe)
        ssch->num_bands = get_bits(gb, 3) + 12;

    ssch->start_block = 0;
    ssch->end_block = 0;
    if (ssch->stride_flag == LONG_STRIDE && !iframe)
        ssch->end_block = 1;

    if (ssch->stride_flag == SHORT_STRIDE) {
        ssch->end_block = 4;
        if (iframe)
            ssch->start_block = 1;
    }

    for (int block = ssch->start_block; block < ssch->end_block; block++) {
        ssch->predictor_presence[block] = get_bits1(gb);
        if (ssch->predictor_presence[block]) {
            if (ssch->start_block == 1 && block == 1) {
                ssch->delta[block] = 0;
            } else {
                ssch->delta[block] = get_bits1(gb);
            }
        }
    }

    ret = ssf_st_data(s, ss, ssch, iframe);
    if (ret < 0)
        return ret;

    return ssf_ac_data(s, ss, ssch);
}

static int ssf_data(AC4DecodeContext *s, Substream *ss,
                    uint8_t ch_id, int iframe)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    GetBitContext *gb = &s->gbc;
    int ssf_iframe, ret;

    if (iframe)
        ssf_iframe = 1;
    else
        ssf_iframe = get_bits1(gb);

    ret = ssf_granule(s, ss, ssch, ssf_iframe);
    if (ret < 0)
        return ret;
    if (s->frame_len_base >= 1536)
        ret = ssf_granule(s, ss, ssch, 0);

    return ret;
}

static int asf_section_data(AC4DecodeContext *s, Substream *ss, SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;

    memset(ssch->sect_cb, 0, sizeof(ssch->sect_cb));
    memset(ssch->sfb_cb, 0, sizeof(ssch->sfb_cb));

    for (int g = 0; g < ssch->scp.num_window_groups; g++) {
        int gidx;
        int transf_length_g = get_transf_length(s, ssch, g, &gidx);
        int sect_esc_val;
        int n_sect_bits;
        int k, i, max_sfb;

        if (gidx <= 2) {
            sect_esc_val = (1 << 3) - 1;
            n_sect_bits = 3;
        } else {
            sect_esc_val = (1 << 5) - 1;
            n_sect_bits = 5;
        }
        k = 0;
        i = 0;
        ssch->num_sec_lsf[g] = 0;
        max_sfb = get_max_sfb(s, ssch, g);
        while (k < max_sfb) {
            int sect_len_incr;
            int sect_len;

            ssch->sect_cb[g][i] = get_bits(gb, 4);
            if (ssch->sect_cb[g][i] > 11) {
                av_log(s->avctx, AV_LOG_ERROR, "sect_cb[%d][%d] > 11\n", g, i);
                return AVERROR_INVALIDDATA;
            }
            sect_len = 1;
            sect_len_incr = get_bits(gb, n_sect_bits);
            while (sect_len_incr == sect_esc_val) {
                sect_len += sect_esc_val;
                sect_len_incr = get_bits(gb, n_sect_bits);
            }

            sect_len += sect_len_incr;
            ssch->sect_start[g][i] = k;
            ssch->sect_end[g][i] = k + sect_len;

            if (ssch->sect_start[g][i] < num_sfb_48(transf_length_g) &&
                ssch->sect_end[g][i] >= num_sfb_48(transf_length_g)) {
                ssch->num_sec_lsf[g] = i + 1;
                if (ssch->sect_end[g][i] > num_sfb_48(transf_length_g)) {
                    ssch->sect_end[g][i] = num_sfb_48(transf_length_g);
                    i++;
                    ssch->sect_start[g][i] = num_sfb_48(transf_length_g);
                    ssch->sect_end[g][i] = k + sect_len;
                    ssch->sect_cb[g][i] = ssch->sect_cb[g][i-1];
                }
            }

            if (k + sect_len >= FF_ARRAY_ELEMS(ssch->sfb_cb[0])) {
                av_log(s->avctx, AV_LOG_ERROR, "k + sect_len overflow\n");
                return AVERROR_INVALIDDATA;
            }

            for (int sfb = k; sfb < k + sect_len; sfb++)
                ssch->sfb_cb[g][sfb] = ssch->sect_cb[g][i];
            k += sect_len;
            i++;
        }

        ssch->num_sec[g] = i;
        if (ssch->num_sec_lsf[g] == 0)
            ssch->num_sec_lsf[g] = ssch->num_sec[g];
    }

    return 0;
}

static int ext_decode(AC4DecodeContext *s)
{
    GetBitContext *gb = &s->gbc;
    int N_ext = get_unary(gb, 0, 17);

    return (1 << (N_ext + 4)) + get_bits_long(gb, N_ext + 4);
}

static int asf_spectral_data(AC4DecodeContext *s, Substream *ss, SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;

    memset(ssch->max_quant_idx, 0, sizeof(ssch->max_quant_idx));
    memset(ssch->quant_spec, 0, sizeof(ssch->quant_spec));

    for (int g = 0; g < ssch->scp.num_window_groups; g++) {
        for (int i = 0; i < ssch->num_sec_lsf[g]; i++) {
            int sect_start_line, sect_end_line, cb;

            if (ssch->sect_cb[g][i] == 0 || ssch->sect_cb[g][i] > 11)
                continue;

            sect_start_line = ssch->sect_sfb_offset[g][ssch->sect_start[g][i]];
            sect_end_line = ssch->sect_sfb_offset[g][ssch->sect_end[g][i]];
            cb = ssch->sect_cb[g][i] - 1;

            for (int k = sect_start_line; k < sect_end_line;) {
                int cb_off = asf_codebook_off[cb];
                int cb_mod = asf_codebook_mod[cb];
                int x, y = g;

                if (asf_codebook_dim[cb] == 4) {
                    int cb_idx = get_vlc2(gb, asf_codebook_vlc[cb].table,
                                          asf_codebook_vlc[cb].bits, 3);
                    int cb_mod2 = 9;
                    int cb_mod3 = 27;

                    if (cb_idx < 0)
                        return AVERROR_INVALIDDATA;

                    av_assert2(k < FF_ARRAY_ELEMS(ssch->quant_spec));

                    ssch->quant_spec[k]   = (cb_idx / cb_mod3) - cb_off;
                    cb_idx -= (ssch->quant_spec[k]   + cb_off) * cb_mod3;
                    ssch->quant_spec[k+1] = (cb_idx / cb_mod2) - cb_off;
                    cb_idx -= (ssch->quant_spec[k+1] + cb_off) * cb_mod2;
                    ssch->quant_spec[k+2] = (cb_idx / cb_mod)  - cb_off;
                    cb_idx -= (ssch->quant_spec[k+2] + cb_off) * cb_mod;
                    ssch->quant_spec[k+3] = cb_idx - cb_off;

                    if (asf_codebook_unsigned[cb]) {
                        if (ssch->quant_spec[k] && get_bits1(gb))
                            ssch->quant_spec[k]   = -ssch->quant_spec[k];
                        if (ssch->quant_spec[k+1] && get_bits1(gb))
                            ssch->quant_spec[k+1] = -ssch->quant_spec[k+1];
                        if (ssch->quant_spec[k+2] && get_bits1(gb))
                            ssch->quant_spec[k+2] = -ssch->quant_spec[k+2];
                        if (ssch->quant_spec[k+3] && get_bits1(gb))
                            ssch->quant_spec[k+3] = -ssch->quant_spec[k+3];
                    }
                    x = ssch->offset2sfb[k];
                    av_assert2(x < FF_ARRAY_ELEMS(ssch->max_quant_idx[y]));
                    ssch->max_quant_idx[y][x] = FFMAX(ssch->max_quant_idx[y][x], FFABS(ssch->quant_spec[k]));
                    x = ssch->offset2sfb[k+1];
                    ssch->max_quant_idx[y][x] = FFMAX(ssch->max_quant_idx[y][x], FFABS(ssch->quant_spec[k+1]));
                    x = ssch->offset2sfb[k+2];
                    ssch->max_quant_idx[y][x] = FFMAX(ssch->max_quant_idx[y][x], FFABS(ssch->quant_spec[k+2]));
                    x = ssch->offset2sfb[k+3];
                    ssch->max_quant_idx[y][x] = FFMAX(ssch->max_quant_idx[y][x], FFABS(ssch->quant_spec[k+3]));
                    k += 4;
                } else { /* (asf_codebook_dim[ssch->sect_cb[g][i]] == 2) */
                    int cb_idx = get_vlc2(gb, asf_codebook_vlc[cb].table,
                                          asf_codebook_vlc[cb].bits, 3);
                    int sign0 = 0, sign1 = 0;

                    if (cb_idx < 0)
                        return AVERROR_INVALIDDATA;

                    av_assert2(k < FF_ARRAY_ELEMS(ssch->quant_spec));

                    ssch->quant_spec[k]  = (cb_idx / cb_mod) - cb_off;
                    cb_idx -= (ssch->quant_spec[k] + cb_off) * cb_mod;
                    ssch->quant_spec[k+1] = cb_idx - cb_off;

                    if (asf_codebook_unsigned[cb]) {
                        if (ssch->quant_spec[k] && get_bits1(gb))
                            sign0 = 1;
                        if (ssch->quant_spec[k+1] && get_bits1(gb))
                            sign1 = 1;
                    }

                    if (ssch->sect_cb[g][i] == 11) {
                        if (ssch->quant_spec[k] == 16)
                            ssch->quant_spec[k] = ext_decode(s);
                        if (ssch->quant_spec[k+1] == 16)
                            ssch->quant_spec[k+1] = ext_decode(s);
                    }

                    if (sign0)
                        ssch->quant_spec[k] = -ssch->quant_spec[k];
                    if (sign1)
                        ssch->quant_spec[k+1] = -ssch->quant_spec[k+1];

                    x = ssch->offset2sfb[k];
                    ssch->max_quant_idx[y][x] = FFMAX(ssch->max_quant_idx[y][x], FFABS(ssch->quant_spec[k]));
                    x = ssch->offset2sfb[k+1];
                    ssch->max_quant_idx[y][x] = FFMAX(ssch->max_quant_idx[y][x], FFABS(ssch->quant_spec[k+1]));
                    k += 2;
                }
            }
        }
    }

    return 0;
}

static int asf_scalefac_data(AC4DecodeContext *s, Substream *ss, SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;
    int first_scf_found = 0;
    int scale_factor;

    scale_factor = get_bits(gb, 8);
    memset(ssch->sf_gain, 0, sizeof(ssch->sf_gain));

    for (int g = 0; g < ssch->scp.num_window_groups; g++) {
        int max_sfb = FFMIN(get_max_sfb(s, ssch, g), num_sfb_48(get_transf_length(s, ssch, g, NULL)));

        for (int sfb = 0; sfb < max_sfb; sfb++) {
            if ((ssch->sfb_cb[g][sfb]) != 0 && (ssch->max_quant_idx[g][sfb] > 0)) {
                if (first_scf_found == 1) {
                    ssch->dpcm_sf[g][sfb] = get_vlc2(gb, scale_factors_vlc.table,
                                                     scale_factors_vlc.bits, 3);
                    if (ssch->dpcm_sf[g][sfb] < 0)
                        return AVERROR_INVALIDDATA;
                    scale_factor += ssch->dpcm_sf[g][sfb] - 60;
                } else {
                    first_scf_found = 1;
                }

                if (scale_factor < 0 || scale_factor > 255)
                    return AVERROR_INVALIDDATA;
                ssch->sf_gain[g][sfb] = powf(2.f, (scale_factor - 100) / 4.f);
            }
        }
    }

    return 0;
}

static int asf_snf_data(AC4DecodeContext *s, Substream *ss, SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;

    ssch->snf_data_exists = get_bits1(gb);
    if (ssch->snf_data_exists) {
        for (int g = 0; g < ssch->scp.num_window_groups; g++) {
            int transf_length_g = get_transf_length(s, ssch, g, NULL);
            int max_sfb = FFMIN(get_max_sfb(s, ssch, g), num_sfb_48(transf_length_g));

            for (int sfb = 0; sfb < max_sfb; sfb++) {
                if ((ssch->sfb_cb[g][sfb] == 0) || (ssch->max_quant_idx[g][sfb] == 0)) {
                    ssch->dpcm_snf[g][sfb] = get_vlc2(gb, snf_vlc.table, snf_vlc.bits, 3);
                    if (ssch->dpcm_snf[g][sfb] < 0)
                        return AVERROR_INVALIDDATA;
                    ssch->dpcm_snf[g][sfb] -= 17;
                }
            }
        }
    }

    return 0;
}

static int sf_data(AC4DecodeContext *s, Substream *ss, int ch_id,
                   int iframe, int spec_frontend)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    int ret;

    av_log(s->avctx, AV_LOG_DEBUG, "sf_data: channel %d\n", ch_id);
    av_log(s->avctx, AV_LOG_DEBUG, "spec_frontend: %d\n", spec_frontend);
    if (spec_frontend == SF_ASF) {
        ret = asf_section_data(s, ss, ssch);
        if (ret < 0)
            return ret;
        ret = asf_spectral_data(s, ss, ssch);
        if (ret < 0)
            return ret;
        ret = asf_scalefac_data(s, ss, ssch);
        if (ret < 0)
            return ret;
        ret = asf_snf_data(s, ss, ssch);
    } else {
        ret = ssf_data(s, ss, ch_id, iframe);
    }

    return ret;
}

static int chparam_info(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    GetBitContext *gb = &s->gbc;
    int ret;

    ssch->sap_mode = get_bits(gb, 2);
    av_log(s->avctx, AV_LOG_DEBUG, "sap_mode: %d\n", ssch->sap_mode);

    if (ssch->sap_mode == 1) {
        av_assert2(ssch->scp.num_window_groups <= FF_ARRAY_ELEMS(ssch->ms_used));
        for (int g = 0; g < ssch->scp.num_window_groups; g++) {
            int max_sfb_g = get_max_sfb(s, ssch, g);

            av_assert2(max_sfb_g <= FF_ARRAY_ELEMS(ssch->ms_used[0]));
            for (int sfb = 0; sfb < max_sfb_g; sfb++) {
                ssch->ms_used[g][sfb] = get_bits1(gb);
            }
        }
    }

    if (ssch->sap_mode == 3) {
        ret = sap_data(s, ss, ssch);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int stereo_data(AC4DecodeContext *s, Substream *ss, int iframe)
{
    GetBitContext *gb = &s->gbc;
    int ret;

    ss->mdct_stereo_proc[0] = get_bits1(gb);
    if (ss->mdct_stereo_proc[0]) {
        ss->spec_frontend_l = SF_ASF;
        ss->spec_frontend_r = SF_ASF;
        ret = sf_info(s, ss, 0, SF_ASF, 0, 0);
        if (ret < 0)
            return ret;

        memcpy(&ss->ssch[1].scp, &ss->ssch[0].scp, sizeof(ss->ssch[0].scp));
        memcpy(&ss->ssch[1].sect_sfb_offset, &ss->ssch[0].sect_sfb_offset, sizeof(ss->ssch[0].sect_sfb_offset));
        memcpy(&ss->ssch[1].offset2sfb, &ss->ssch[0].offset2sfb, sizeof(ss->ssch[0].offset2sfb));
        memcpy(&ss->ssch[1].offset2g, &ss->ssch[0].offset2g, sizeof(ss->ssch[0].offset2g));
        memcpy(&ss->ssch[1].win_offset, &ss->ssch[0].win_offset, sizeof(ss->ssch[0].win_offset));

        ret = chparam_info(s, ss, 0);
        if (ret < 0)
            return ret;
    } else {
        ss->spec_frontend_l = get_bits1(gb);
        sf_info(s, ss, 0, ss->spec_frontend_l, 0, 0);
        ss->spec_frontend_r = get_bits1(gb);
        sf_info(s, ss, 1, ss->spec_frontend_r, 0, 0);
    }

    ret = sf_data(s, ss, 0, iframe, ss->spec_frontend_l);
    if (ret < 0)
        return ret;
    ret = sf_data(s, ss, 1, iframe, ss->spec_frontend_r);
    if (ret < 0)
        return ret;

    return 0;
}

static int companding_control(AC4DecodeContext *s, Substream *ss, int num_chan)
{
    GetBitContext *gb = &s->gbc;
    int need_avg = 0;
    int nc;

    ss->sync_flag = 0;
    if (num_chan > 1)
        ss->sync_flag = get_bits1(gb);

    nc = ss->sync_flag ? 1 : num_chan;

    for (int i = 0; i < nc; i++) {
        ss->compand_on[i] = get_bits1(gb);
        if (!ss->compand_on[i])
            need_avg = 1;
    }

    if (need_avg == 1)
        ss->compand_avg = get_bits1(gb);

    return 0;
}

static int noise_mid_border(int aspx_tsg_ptr, int aspx_int_class, int num_atsg_sig)
{
    if (aspx_tsg_ptr == -1) {
        if (aspx_int_class == VARFIX)
            return 1;
        else if (aspx_int_class != FIXFIX)
            return num_atsg_sig - 1;
        else
            av_assert0(0);
    } else if (aspx_tsg_ptr >= 0) {
        if (aspx_int_class == VARFIX)
            return num_atsg_sig - 1;
        else if (aspx_int_class != FIXFIX)
            return FFMAX(1, FFMIN(num_atsg_sig - 1, aspx_tsg_ptr));
        else
            av_assert0(0);
    } else {
        av_assert0(0);
    }

    return 0;
}

static int freq_res(uint8_t *atsg_sig, int atsg, int aspx_tsg_ptr,
                    int num_aspx_timeslots, int aspx_freq_res_mode,
                    const uint8_t *aspx_freq_res)
{
    int freq_res;

    switch (aspx_freq_res_mode) {
    case 0:
        freq_res = aspx_freq_res[atsg];
        break;
    case 1:
        freq_res = 0;
        break;
    case 2:
        if ((atsg < aspx_tsg_ptr && num_aspx_timeslots > 8) ||
            (atsg_sig[atsg+1]-atsg_sig[atsg]) > (num_aspx_timeslots/6.0+3.25))
            freq_res = 1;
        else
            freq_res = 0;
        break;
    case 3:
        freq_res = 1;
        break;
    default:
        av_assert0(0);
    }

    return freq_res;
}

static void get_tab_border(uint8_t *atsg_sig, int num_aspx_timeslots, int num_atsg)
{
    const size_t size = sizeof(tab_border[0][0]);

    switch (num_aspx_timeslots) {
    case 6:
        memcpy(atsg_sig, tab_border[0][num_atsg >> 1], size);
        break;
    case 8:
        memcpy(atsg_sig, tab_border[1][num_atsg >> 1], size);
        break;
    case 12:
        memcpy(atsg_sig, tab_border[2][num_atsg >> 1], size);
        break;
    case 15:
        memcpy(atsg_sig, tab_border[3][num_atsg >> 1], size);
        break;
    case 16:
        memcpy(atsg_sig, tab_border[4][num_atsg >> 1], size);
        break;
    default:
        av_assert0(0);
    }
}

static int aspx_atsg(AC4DecodeContext *s, Substream *ss, SubstreamChannel *ssch, int iframe)
{
    int num_atsg_sig, num_atsg_noise;

    ssch->num_atsg_sig_prev = ssch->num_atsg_sig;
    ssch->num_atsg_noise_prev = ssch->num_atsg_noise;

    num_atsg_sig = ssch->num_atsg_sig = ssch->aspx_num_env;
    num_atsg_noise = ssch->num_atsg_noise = ssch->aspx_num_noise;

    if (ssch->previous_stop_pos == 0)
        ssch->previous_stop_pos = s->num_aspx_timeslots;

    if (ssch->aspx_int_class == FIXFIX) {
        get_tab_border(ssch->atsg_sig, s->num_aspx_timeslots, num_atsg_sig);
        get_tab_border(ssch->atsg_noise, s->num_aspx_timeslots, num_atsg_noise);
        ssch->atsg_freqres[0] = freq_res(ssch->atsg_sig, 0, 0, s->num_aspx_timeslots,
                                         ss->aspx_freq_res_mode, ssch->aspx_freq_res);
        for (int atsg = 1; atsg < num_atsg_sig; atsg++)
            ssch->atsg_freqres[atsg] = ssch->atsg_freqres[0];
    } else {
        switch (ssch->aspx_int_class) {
        case FIXVAR:
            ssch->atsg_sig[0] = 0;
            ssch->atsg_sig[num_atsg_sig] = ssch->aspx_var_bord_right + s->num_aspx_timeslots;
            for (int tsg = 0; tsg < ssch->aspx_num_rel_right; tsg++)
                ssch->atsg_sig[num_atsg_sig-tsg-1] = ssch->atsg_sig[num_atsg_sig-tsg] - ssch->aspx_rel_bord_right[tsg];
            break;
        case VARFIX:
            if (iframe)
                ssch->atsg_sig[0] = ssch->aspx_var_bord_left;
            else
                ssch->atsg_sig[0] = ssch->previous_stop_pos - s->num_aspx_timeslots;
            ssch->atsg_sig[num_atsg_sig] = s->num_aspx_timeslots;
            for (int tsg = 0; tsg < ssch->aspx_num_rel_left; tsg++)
                ssch->atsg_sig[tsg+1] = ssch->atsg_sig[tsg] + ssch->aspx_rel_bord_left[tsg];
            break;
        case VARVAR:
            if (iframe)
                ssch->atsg_sig[0] = ssch->aspx_var_bord_left;
            else
                ssch->atsg_sig[0] = ssch->previous_stop_pos - s->num_aspx_timeslots;
            ssch->atsg_sig[num_atsg_sig] = ssch->aspx_var_bord_right + s->num_aspx_timeslots;
            for (int tsg = 0; tsg < ssch->aspx_num_rel_left; tsg++)
                ssch->atsg_sig[tsg+1] = ssch->atsg_sig[tsg] + ssch->aspx_rel_bord_left[tsg];
            for (int tsg = 0; tsg < ssch->aspx_num_rel_right; tsg++)
                ssch->atsg_sig[num_atsg_sig-tsg-1] = ssch->atsg_sig[num_atsg_sig-tsg] - ssch->aspx_rel_bord_right[tsg];
            break;
        }

        ssch->atsg_noise[0] = ssch->atsg_sig[0];
        ssch->atsg_noise[num_atsg_noise] = ssch->atsg_sig[num_atsg_sig];
        if (num_atsg_noise > 1)
            ssch->atsg_noise[1] = ssch->atsg_sig[noise_mid_border(ssch->aspx_tsg_ptr,
                                                                  ssch->aspx_int_class,
                                                                  num_atsg_sig)];
        for (int atsg = 0; atsg < num_atsg_sig; atsg++)
            ssch->atsg_freqres[atsg] = freq_res(ssch->atsg_sig, atsg, ssch->aspx_tsg_ptr,
                                                s->num_aspx_timeslots, ss->aspx_freq_res_mode,
                                                ssch->aspx_freq_res);
    }

    ssch->previous_stop_pos = ssch->atsg_sig[num_atsg_sig];

    av_assert0(num_atsg_sig > 0 && num_atsg_sig <= 5);
    for (int atsg = 0; atsg < num_atsg_sig; atsg++) {
        av_log(s->avctx, AV_LOG_DEBUG, "atsg_sig[%d]: %d\n", atsg, ssch->atsg_sig[atsg]);

        if (ssch->atsg_freqres[atsg]) {
            ssch->num_sbg_sig[atsg] = ssch->num_sbg_sig_highres;
            memcpy(ssch->sbg_sig[atsg], ssch->sbg_sig_highres, sizeof(ssch->sbg_sig_highres));
        } else {
            ssch->num_sbg_sig[atsg] = ssch->num_sbg_sig_lowres;
            memcpy(ssch->sbg_sig[atsg], ssch->sbg_sig_lowres, sizeof(ssch->sbg_sig_lowres));
        }
    }
    ssch->atsg_sig[num_atsg_sig] = FFMIN(s->num_aspx_timeslots, ssch->atsg_sig[num_atsg_sig]);
    av_log(s->avctx, AV_LOG_DEBUG, "atsg_sig[%d]: %d\n", num_atsg_sig, ssch->atsg_sig[num_atsg_sig]);

    return 0;
}

static int aspx_framing(AC4DecodeContext *s, Substream *ss, int ch_id, int iframe)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    GetBitContext *gb = &s->gbc;

    ssch->aspx_num_rel_left = 0;
    ssch->aspx_num_rel_right = 0;

    ssch->aspx_int_class = get_vlc2(gb, aspx_int_class_vlc.table,
                                    aspx_int_class_vlc.bits, 3);
    if (ssch->aspx_int_class < 0)
        return AVERROR_INVALIDDATA;

    ssch->aspx_num_env_prev = FFMAX(1, ssch->aspx_num_env);

    switch (ssch->aspx_int_class) {
    case FIXFIX:
        ssch->aspx_num_env = 1 << get_bits(gb, 1 + ss->aspx_num_env_bits_fixfix);
        if (ssch->aspx_num_env > 4) {
            av_log(s->avctx, AV_LOG_ERROR, "invalid aspx num env in FIXFIX: %d\n", ssch->aspx_num_env);
            return AVERROR_INVALIDDATA;
        }

        if (ss->aspx_freq_res_mode == 0)
            ssch->aspx_freq_res[0] = get_bits1(gb);
        break;
    case FIXVAR:
        ssch->aspx_var_bord_right = get_bits(gb, 2);
        ssch->aspx_num_rel_right = get_bits(gb, 1 + (s->num_aspx_timeslots > 8));
        for (int i = 0; i < ssch->aspx_num_rel_right; i++)
            ssch->aspx_rel_bord_right[i] = 2 * get_bits(gb, 1 + (s->num_aspx_timeslots > 8)) + 2;
        break;
    case VARVAR:
        if (iframe)
            ssch->aspx_var_bord_left = get_bits(gb, 2);
        ssch->aspx_num_rel_left = get_bits(gb, 1 + (s->num_aspx_timeslots > 8));
        for (int i = 0; i < ssch->aspx_num_rel_left; i++)
            ssch->aspx_rel_bord_left[i] = 2 * get_bits(gb, 1 + (s->num_aspx_timeslots > 8)) + 2;
        ssch->aspx_var_bord_right = get_bits(gb, 2);
        ssch->aspx_num_rel_right = get_bits(gb, 1 + (s->num_aspx_timeslots > 8));
        for (int i = 0; i < ssch->aspx_num_rel_right; i++)
            ssch->aspx_rel_bord_right[i] = 2 * get_bits(gb, 1 + (s->num_aspx_timeslots > 8)) + 2;
        break;
    case VARFIX:
        if (iframe)
            ssch->aspx_var_bord_left = get_bits(gb, 2);
        ssch->aspx_num_rel_left = get_bits(gb, 1 + (s->num_aspx_timeslots > 8));
        for (int i = 0; i < ssch->aspx_num_rel_left; i++)
            ssch->aspx_rel_bord_left[i] = 2 * get_bits(gb, 1 + (s->num_aspx_timeslots > 8)) + 2;
        break;
    }

    if (ssch->aspx_int_class != FIXFIX) {
        int ptr_bits;

        ssch->aspx_num_env = ssch->aspx_num_rel_left + ssch->aspx_num_rel_right + 1;
        if (ssch->aspx_num_env > 5) {
            av_log(s->avctx, AV_LOG_ERROR, "invalid aspx num env: %d (class %d)\n", ssch->aspx_num_env, ssch->aspx_int_class);
            return AVERROR_INVALIDDATA;
        }

        ptr_bits = ceilf(logf(ssch->aspx_num_env + 2) / logf(2));
        av_assert2(ptr_bits > 0);
        ssch->aspx_tsg_ptr_prev = ssch->aspx_tsg_ptr;
        ssch->aspx_tsg_ptr = get_bits(gb, ptr_bits);
        ssch->aspx_tsg_ptr -= 1;
        if (ss->aspx_freq_res_mode == 0)
            for (int env = 0; env < ssch->aspx_num_env; env++)
                ssch->aspx_freq_res[env] = get_bits1(gb);
    }

    ssch->aspx_num_noise_prev = ssch->aspx_num_noise;

    if (ssch->aspx_num_env > 1)
        ssch->aspx_num_noise = 2;
    else
        ssch->aspx_num_noise = 1;

    if (!ssch->aspx_num_env_prev)
        ssch->aspx_num_env_prev = ssch->aspx_num_env;
    ssch->aspx_num_env_prev = FFMAX(1, ssch->aspx_num_env_prev);

    if (!ssch->aspx_num_noise_prev)
        ssch->aspx_num_noise_prev = ssch->aspx_num_noise;

    av_log(s->avctx, AV_LOG_DEBUG, "aspx_num_sig: %d\n", ssch->aspx_num_env);
    av_log(s->avctx, AV_LOG_DEBUG, "aspx_num_noise: %d\n", ssch->aspx_num_noise);

    return aspx_atsg(s, ss, ssch, iframe);
}

static void aspx_delta_dir(AC4DecodeContext *s, SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;

    av_assert2(ssch->aspx_num_env <= FF_ARRAY_ELEMS(ssch->aspx_sig_delta_dir));
    for (int env = 0; env < ssch->aspx_num_env; env++)
        ssch->aspx_sig_delta_dir[env] = get_bits1(gb);

    av_assert2(ssch->aspx_num_noise <= FF_ARRAY_ELEMS(ssch->aspx_noise_delta_dir));
    for (int env = 0; env < ssch->aspx_num_noise; env++)
        ssch->aspx_noise_delta_dir[env] = get_bits1(gb);
}

static int aspx_hfgen_iwc_2ch(AC4DecodeContext *s, Substream *ss,
                              SubstreamChannel *ssch0,
                              SubstreamChannel *ssch1,
                              int aspx_balance)
{
    GetBitContext *gb = &s->gbc;

    memcpy(ssch0->aspx_tna_mode_prev, ssch0->aspx_tna_mode, sizeof(ssch0->aspx_tna_mode));
    memcpy(ssch1->aspx_tna_mode_prev, ssch1->aspx_tna_mode, sizeof(ssch1->aspx_tna_mode));
    memset(ssch0->aspx_tna_mode, 0, sizeof(ssch0->aspx_tna_mode));
    memset(ssch1->aspx_tna_mode, 0, sizeof(ssch1->aspx_tna_mode));
    memset(ssch0->aspx_fic_used_in_sfb, 0, sizeof(ssch0->aspx_fic_used_in_sfb));
    memset(ssch1->aspx_fic_used_in_sfb, 0, sizeof(ssch1->aspx_fic_used_in_sfb));
    memset(ssch0->aspx_tic_used_in_slot, 0, sizeof(ssch0->aspx_tic_used_in_slot));
    memset(ssch1->aspx_tic_used_in_slot, 0, sizeof(ssch1->aspx_tic_used_in_slot));

    av_assert2(ssch0->num_sbg_noise <= FF_ARRAY_ELEMS(ssch0->aspx_tna_mode));
    av_assert2(ssch1->num_sbg_noise <= FF_ARRAY_ELEMS(ssch1->aspx_tna_mode));

    for (int n = 0; n < ssch0->num_sbg_noise; n++)
        ssch0->aspx_tna_mode[n] = get_bits(gb, 2);
    if (aspx_balance == 0) {
        for (int n = 0; n < ssch0->num_sbg_noise; n++)
            ssch1->aspx_tna_mode[n] = get_bits(gb, 2);
    } else {
        for (int n = 0; n < ssch0->num_sbg_noise; n++)
            ssch1->aspx_tna_mode[n] = ssch0->aspx_tna_mode[n];
    }
    if (get_bits1(gb)) {
        for (int n = 0; n < ssch0->num_sbg_sig_highres; n++)
            ssch0->aspx_add_harmonic[n] = get_bits1(gb);
    }
    if (get_bits1(gb)) {
        for (int n = 0; n < ssch0->num_sbg_sig_highres; n++)
            ssch1->aspx_add_harmonic[n] = get_bits1(gb);
    }

    if (get_bits1(gb)) {
        if (get_bits1(gb)) {
            for (int n = 0; n < ssch0->num_sbg_sig_highres; n++)
                ssch0->aspx_fic_used_in_sfb[n] = get_bits1(gb);
        }

        if (get_bits1(gb)) {
            for (int n = 0; n < ssch0->num_sbg_sig_highres; n++)
                ssch1->aspx_fic_used_in_sfb[n] = get_bits1(gb);
        }
    }

    if (get_bits1(gb)) {
        int aspx_tic_copy = get_bits1(gb);
        int aspx_tic_left = 0, aspx_tic_right = 0;

        if (aspx_tic_copy == 0) {
            aspx_tic_left = get_bits1(gb);
            aspx_tic_right = get_bits1(gb);
        }

        if (aspx_tic_copy || aspx_tic_left) {
            for (int n = 0; n < s->num_aspx_timeslots; n++)
                ssch0->aspx_tic_used_in_slot[n] = get_bits1(gb);
        }

        if (aspx_tic_right) {
            for (int n = 0; n < s->num_aspx_timeslots; n++)
                ssch1->aspx_tic_used_in_slot[n] = get_bits1(gb);
        }

        if (aspx_tic_copy) {
            for (int n = 0; n < s->num_aspx_timeslots; n++)
                ssch1->aspx_tic_used_in_slot[n] = ssch0->aspx_tic_used_in_slot[n];
        }
    }

    return 0;
}

static VLC *get_aspx_hcb(int data_type, int quant_mode, int stereo_mode, int hcb_type)
{
    VLC *aspx_cb;

    if (data_type == DT_SIGNAL)
        aspx_cb = &aspx_codebook_signal_vlc[stereo_mode][quant_mode][hcb_type];
    else // NOISE
        aspx_cb = &aspx_codebook_noise_vlc[stereo_mode][hcb_type];

    return aspx_cb;
}

static int get_aspx_off(int data_type, int quant_mode, int stereo_mode)
{
    int off;

    if (data_type == DT_SIGNAL)
        off = aspx_codebook_signal_off[stereo_mode][quant_mode];
    else // NOISE
        off = aspx_codebook_noise_off[stereo_mode];

    return off;
}

static int aspx_huff_data(AC4DecodeContext *s,
                          int data_type, int num_sbg,
                          int quant_mode, int stereo_mode,
                          int direction, int16_t *data)
{
    GetBitContext *gb = &s->gbc;
    VLC *aspx_cb;
    int aspx_off;

    if (direction == 0) { // FREQ
        aspx_cb = get_aspx_hcb(data_type, quant_mode, stereo_mode, F0);
        data[0] = get_vlc2(gb, aspx_cb->table, aspx_cb->bits, 3);
        if (data[0] < 0)
            return AVERROR_INVALIDDATA;
        aspx_cb = get_aspx_hcb(data_type, quant_mode, stereo_mode, DF);
        aspx_off = get_aspx_off(data_type, quant_mode, stereo_mode);
        for (int i = 1; i < num_sbg; i++) {
            data[i] = get_vlc2(gb, aspx_cb->table, aspx_cb->bits, 3);
            if (data[i] < 0)
                return AVERROR_INVALIDDATA;
            data[i] -= aspx_off;
        }
    } else { // TIME
        aspx_cb = get_aspx_hcb(data_type, quant_mode, stereo_mode, DT);
        aspx_off = get_aspx_off(data_type, quant_mode, stereo_mode);
        for (int i = 0; i < num_sbg; i++) {
            data[i] = get_vlc2(gb, aspx_cb->table, aspx_cb->bits, 3);
            if (data[i] < 0)
                return AVERROR_INVALIDDATA;
            data[i] -= aspx_off;
        }
    }

    return 0;
}

static int aspx_ec_data(AC4DecodeContext *s,
                        Substream *ss,
                        SubstreamChannel *ssch,
                        int data_type, int num_env,
                        uint8_t *freq_res, int quant_mode,
                        int stereo_mode, uint8_t *direction)
{
    int dir, num_sbg, ret;

    for (int env = 0; env < num_env; env++) {
        if (data_type == DT_SIGNAL) {
            if (freq_res[env])
                num_sbg = ssch->num_sbg_sig_highres;
            else
                num_sbg = ssch->num_sbg_sig_lowres;
        } else {
            num_sbg = ssch->num_sbg_noise;
        }
        dir = direction[env];
        av_assert2(num_sbg <= FF_ARRAY_ELEMS(ssch->aspx_data[data_type][env]));
        ret = aspx_huff_data(s, data_type, num_sbg, quant_mode, stereo_mode, dir,
                             ssch->aspx_data[data_type][env]);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int is_element_of_sbg_patches(int sbg_lim_sbg, int *sbg_patches,
                                     int num_sbg_patches)
{
    for (int i = 0; i <= num_sbg_patches; i++) {
        if (sbg_patches[i] == sbg_lim_sbg)
            return 1;
    }

    return 0;
}

static void remove_element(int *sbg_lim, int num_sbg_lim, int sbg)
{
    for (int i = sbg; i < num_sbg_lim; i++)
        sbg_lim[i] = sbg_lim[i + 1];
}

static int cmpints(const void *p1, const void *p2)
{
    int left  = *(const int *)p1;
    int right = *(const int *)p2;
    return FFDIFFSIGN(left, right);
}

static int aspx_elements(AC4DecodeContext *s, Substream *ss, SubstreamChannel *ssch,
                         const int iframe)
{
    int sb, j, sbg = 0, goal_sb, msb, usb;
    int source_band_low, prev_idx;

    ssch->master_reset = 0;
    if (iframe) {
        ssch->master_reset = !!(s->first_frame +
                                (ss->prev_aspx_start_freq != ss->aspx_start_freq) +
                                (ss->prev_aspx_stop_freq != ss->aspx_stop_freq) +
                                (ss->prev_aspx_master_freq_scale != ss->aspx_master_freq_scale));
    }

    if (ssch->master_reset) {
        if (ss->aspx_master_freq_scale == 1) {
            ssch->num_sbg_master = 22 - 2 * ss->aspx_start_freq - 2 * ss->aspx_stop_freq;
            for (int sbg = 0; sbg <= ssch->num_sbg_master; sbg++) {
                ssch->sbg_master[sbg] = sbg_template_highres[2 * ss->aspx_start_freq + sbg];
            }
        } else {
            ssch->num_sbg_master = 20 - 2 * ss->aspx_start_freq - 2 * ss->aspx_stop_freq;
            for (int sbg = 0; sbg <= ssch->num_sbg_master; sbg++) {
                ssch->sbg_master[sbg] = sbg_template_lowres[2 * ss->aspx_start_freq + sbg];
            }
        }
    }

    ssch->sba = ssch->sbg_master[0];
    ssch->sbz = ssch->sbg_master[ssch->num_sbg_master];

    ssch->num_sbg_sig_highres = ssch->num_sbg_master - ssch->aspx_xover_subband_offset;
    av_log(s->avctx, AV_LOG_DEBUG, "num_sbg_sig_highres: %d\n", ssch->num_sbg_sig_highres);
    for (int sbg = 0; sbg <= ssch->num_sbg_sig_highres; sbg++)
        ssch->sbg_sig_highres[sbg] = ssch->sbg_master[sbg + ssch->aspx_xover_subband_offset];

    ssch->sbx = ssch->sbg_sig_highres[0];
    av_log(s->avctx, AV_LOG_DEBUG, "sbx: %d\n", ssch->sbx);
    if (ssch->sbx <= 0)
        return AVERROR_INVALIDDATA;
    ssch->num_sb_aspx = ssch->sbg_sig_highres[ssch->num_sbg_sig_highres] - ssch->sbx;

    ssch->num_sbg_sig_lowres = ssch->num_sbg_sig_highres - floorf(ssch->num_sbg_sig_highres / 2.f);
    av_log(s->avctx, AV_LOG_DEBUG, "num_sbg_sig_lowres: %d\n", ssch->num_sbg_sig_lowres);

    ssch->sbg_sig_lowres[0] = ssch->sbg_sig_highres[0];
    if ((ssch->num_sbg_sig_highres & 1) == 0) {
        av_assert2(ssch->num_sbg_sig_lowres < FF_ARRAY_ELEMS(ssch->sbg_sig_lowres));
        av_assert2(ssch->num_sbg_sig_lowres * 2 < FF_ARRAY_ELEMS(ssch->sbg_sig_highres));
        for (int sbg = 1; sbg <= ssch->num_sbg_sig_lowres; sbg++)
            ssch->sbg_sig_lowres[sbg] = ssch->sbg_sig_highres[2*sbg];
    } else {
        av_assert2(ssch->num_sbg_sig_lowres < FF_ARRAY_ELEMS(ssch->sbg_sig_lowres));
        av_assert2(ssch->num_sbg_sig_lowres * 2 < FF_ARRAY_ELEMS(ssch->sbg_sig_highres));
        for (int sbg = 1; sbg <= ssch->num_sbg_sig_lowres; sbg++)
            ssch->sbg_sig_lowres[sbg] = ssch->sbg_sig_highres[2*sbg-1];
    }

    ssch->num_sbg_sig[0] = ssch->num_sbg_sig_lowres;
    ssch->num_sbg_sig[1] = ssch->num_sbg_sig_highres;

    ssch->num_sbg_noise = FFMAX(1, floorf(ss->aspx_noise_sbg * log2f(ssch->sbz / (float)ssch->sbx) + 0.5f));
    if (ssch->num_sbg_noise > 5) {
        av_log(s->avctx, AV_LOG_ERROR, "invalid num sbg noise: %d\n", ssch->num_sbg_noise);
        return AVERROR_INVALIDDATA;
    }

    prev_idx = 0;
    ssch->sbg_noise[0] = ssch->sbg_sig_lowres[0];
    for (int sbg = 1; sbg <= ssch->num_sbg_noise; sbg++) {
        int idx = prev_idx;

        idx += floorf((ssch->num_sbg_sig_lowres - prev_idx) / (float)(ssch->num_sbg_noise + 1 - sbg));
        ssch->sbg_noise[sbg] = ssch->sbg_sig_lowres[idx];
        prev_idx = idx;
    }

    msb = ssch->sba;
    usb = ssch->sbx;
    ssch->num_sbg_patches = 0;
    if (s->fs_index)
        goal_sb = 43;
    else
        goal_sb = 46;
    if (ss->aspx_master_freq_scale == 1)
        source_band_low = 4;
    else
        source_band_low = 2;

    if (goal_sb < ssch->sbx + ssch->num_sb_aspx) {
        for (int i = 0; ssch->sbg_master[i] < goal_sb; i++)
            sbg = i + 1;
    } else {
        sbg = ssch->num_sbg_master;
    }

    do {
        int odd;

        j = sbg;
        sb = ssch->sbg_master[j];
        odd = (sb - 2 + ssch->sba) % 2;

        while (sb > (ssch->sba - source_band_low + msb - odd) && j >= 1) {
            j--;
            sb = ssch->sbg_master[j];
            odd = (sb - 2 + ssch->sba) % 2;
        }

        ssch->sbg_patch_num_sb[ssch->num_sbg_patches] = FFMAX(sb - usb, 0);
        ssch->sbg_patch_start_sb[ssch->num_sbg_patches] = ssch->sba - odd - FFMAX(sb - usb, 0);
        if (ssch->sbg_patch_num_sb[ssch->num_sbg_patches] > 0) {
            usb = sb;
            msb = sb;
            ssch->num_sbg_patches++;
        } else {
            msb = ssch->sbx;
        }

        if (ssch->sbg_master[sbg] - sb < 3)
            sbg = ssch->num_sbg_master;
    } while (sb != (ssch->sbx + ssch->num_sb_aspx) && j > 0);

    if ((ssch->num_sbg_patches > 1) && (ssch->sbg_patch_num_sb[ssch->num_sbg_patches - 1] < 3))
        ssch->num_sbg_patches--;

    if (ssch->num_sbg_patches > 5)
        return AVERROR_INVALIDDATA;

    ssch->sbg_patches[0] = ssch->sbx;
    for (int i = 1; i <= ssch->num_sbg_patches; i++)
        ssch->sbg_patches[i] = ssch->sbg_patches[i-1] + ssch->sbg_patch_num_sb[i-1];

    /* Copy sbg_sig_lowres into lower part of limiter table */
    for (int sbg = 0; sbg <= ssch->num_sbg_sig_lowres; sbg++)
        ssch->sbg_lim[sbg] = ssch->sbg_sig_lowres[sbg];

    /* Copy patch borders into higher part of limiter table */
    for (int sbg = 1; sbg < ssch->num_sbg_patches; sbg++)
        ssch->sbg_lim[sbg + ssch->num_sbg_sig_lowres] = ssch->sbg_patches[sbg];

    /* Sort patch borders + low res sbg into temporary limiter table */
    ssch->num_sbg_lim = ssch->num_sbg_sig_lowres + ssch->num_sbg_patches - 1;
    AV_QSORT(ssch->sbg_lim, ssch->num_sbg_lim, int, cmpints);
    sbg = 1;

    while (sbg <= ssch->num_sbg_lim) {
        float num_octaves = log2(ssch->sbg_lim[sbg] / (float)ssch->sbg_lim[sbg - 1]);

        if (num_octaves < 0.245f) {
            if (ssch->sbg_lim[sbg] == ssch->sbg_lim[sbg-1]) {
                remove_element(ssch->sbg_lim, ssch->num_sbg_lim, sbg);
                ssch->num_sbg_lim--;
                continue;
            } else {
                if (is_element_of_sbg_patches(ssch->sbg_lim[sbg],
                                              ssch->sbg_patches,
                                              ssch->num_sbg_patches)) {
                    if (is_element_of_sbg_patches(ssch->sbg_lim[sbg - 1],
                                                  ssch->sbg_patches,
                                                  ssch->num_sbg_patches)) {
                        sbg++;
                        continue;
                    } else {
                        remove_element(ssch->sbg_lim, ssch->num_sbg_lim, sbg - 1);
                        ssch->num_sbg_lim--;
                        continue;
                    }
                } else {
                    remove_element(ssch->sbg_lim, ssch->num_sbg_lim, sbg);
                    ssch->num_sbg_lim--;
                    continue;
                }
            }
        } else {
            sbg++;
            continue;
        }
    }

    return 0;
}

static int aspx_data_2ch(AC4DecodeContext *s, Substream *ss,
                         uint8_t ch_id[2], int iframe)
{
    SubstreamChannel *ssch0 = &ss->ssch[ch_id[0]];
    SubstreamChannel *ssch1 = &ss->ssch[ch_id[1]];
    GetBitContext *gb = &s->gbc;
    int ret;

    if (iframe) {
        ssch0->aspx_xover_subband_offset = get_bits(gb, 3);
        ssch1->aspx_xover_subband_offset = ssch0->aspx_xover_subband_offset;
    }

    ret = aspx_elements(s, ss, ssch0, iframe);
    if (ret < 0)
        return ret;

    ret = aspx_elements(s, ss, ssch1, iframe);
    if (ret < 0)
        return ret;

    ret = aspx_framing(s, ss, ch_id[0], iframe);
    if (ret < 0)
        return ret;

    ssch0->aspx_qmode_env = ssch1->aspx_qmode_env = ss->aspx_quant_mode_env;
    if (ssch0->aspx_int_class == FIXFIX && ssch0->aspx_num_env == 1)
        ssch0->aspx_qmode_env = ssch1->aspx_qmode_env = 0;

    ssch0->aspx_balance = ssch1->aspx_balance = get_bits1(gb);

    if (ssch0->aspx_balance == 0) {
        ret = aspx_framing(s, ss, ch_id[1], iframe);
        if (ret < 0)
            return ret;
        ssch1->aspx_qmode_env = ss->aspx_quant_mode_env;
        if (ssch1->aspx_int_class == FIXFIX && ssch1->aspx_num_env == 1)
            ssch1->aspx_qmode_env = 0;
    } else {
        ssch1->sbx = ssch0->sbx;
        ssch1->num_sb_aspx = ssch0->num_sb_aspx;
        ssch1->aspx_num_env = ssch0->aspx_num_env;
        ssch1->aspx_num_noise = ssch0->aspx_num_noise;
        ssch1->num_atsg_sig = ssch0->num_atsg_sig;
        ssch1->num_atsg_noise = ssch0->num_atsg_noise;
        memcpy(ssch1->sbg_sig, ssch0->sbg_sig, sizeof(ssch0->sbg_sig));
        memcpy(ssch1->atsg_sig, ssch0->atsg_sig, sizeof(ssch0->atsg_sig));
        memcpy(ssch1->num_sbg_sig, ssch0->num_sbg_sig, sizeof(ssch0->num_sbg_sig));
        memcpy(ssch1->atsg_freqres, ssch0->atsg_freqres, sizeof(ssch0->atsg_freqres));
    }

    aspx_delta_dir(s, ssch0);
    aspx_delta_dir(s, ssch1);
    aspx_hfgen_iwc_2ch(s, ss, ssch0, ssch1, ssch0->aspx_balance);

    memset(ssch0->aspx_data, 0, sizeof(ssch0->aspx_data));
    memset(ssch1->aspx_data, 0, sizeof(ssch1->aspx_data));

    ret = aspx_ec_data(s, ss, ssch0, DT_SIGNAL,
                       ssch0->aspx_num_env,
                       ssch0->atsg_freqres,
                       ssch0->aspx_qmode_env,
                       SM_LEVEL,
                       ssch0->aspx_sig_delta_dir);
    if (ret < 0)
        return ret;
    ret = aspx_ec_data(s, ss, ssch1, DT_SIGNAL,
                       ssch1->aspx_num_env,
                       ssch1->atsg_freqres,
                       ssch1->aspx_qmode_env,
                       ssch0->aspx_balance ? SM_BALANCE : SM_LEVEL,
                       ssch1->aspx_sig_delta_dir);
    if (ret < 0)
        return ret;
    ret = aspx_ec_data(s, ss, ssch0, DT_NOISE,
                       ssch0->aspx_num_noise,
                       0,
                       0,
                       SM_LEVEL,
                       ssch0->aspx_noise_delta_dir);
    if (ret < 0)
        return ret;
    ret = aspx_ec_data(s, ss, ssch1, DT_NOISE,
                       ssch1->aspx_num_noise,
                       0,
                       0,
                       ssch0->aspx_balance ? SM_BALANCE : SM_LEVEL,
                       ssch1->aspx_noise_delta_dir);

    return ret;
}

static int aspx_hfgen_iwc_1ch(AC4DecodeContext *s, Substream *ss,
                              SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;

    memcpy(ssch->aspx_tna_mode_prev, ssch->aspx_tna_mode, sizeof(ssch->aspx_tna_mode));
    memset(ssch->aspx_tna_mode, 0, sizeof(ssch->aspx_tna_mode));
    memset(ssch->aspx_fic_used_in_sfb, 0, sizeof(ssch->aspx_fic_used_in_sfb));
    memset(ssch->aspx_tic_used_in_slot, 0, sizeof(ssch->aspx_tic_used_in_slot));

    av_assert2(ssch->num_sbg_noise <= FF_ARRAY_ELEMS(ssch->aspx_tna_mode));

    for (int n = 0; n < ssch->num_sbg_noise; n++)
        ssch->aspx_tna_mode[n] = get_bits(gb, 2);

    if (get_bits1(gb)) {
        for (int n = 0; n < ssch->num_sbg_sig_highres; n++)
            ssch->aspx_add_harmonic[n] = get_bits1(gb);
    }

    if (get_bits1(gb)) {
        for (int n = 0; n < ssch->num_sbg_sig_highres; n++)
            ssch->aspx_fic_used_in_sfb[n] = get_bits1(gb);
    }

    if (get_bits1(gb)) {
        for (int n = 0; n < s->num_aspx_timeslots; n++)
            ssch->aspx_tic_used_in_slot[n] = get_bits1(gb);
    }

    return 0;
}

static int aspx_data_1ch(AC4DecodeContext *s, Substream *ss,
                         int ch_id, int iframe)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    GetBitContext *gb = &s->gbc;
    int ret;

    if (iframe)
        ssch->aspx_xover_subband_offset = get_bits(gb, 3);

    ssch->aspx_balance = 0;

    ret = aspx_elements(s, ss, ssch, iframe);
    if (ret < 0)
        return ret;

    ret = aspx_framing(s, ss, ch_id, iframe);
    if (ret < 0)
        return ret;

    ssch->aspx_qmode_env = ss->aspx_quant_mode_env;
    if (ssch->aspx_int_class == FIXFIX && ssch->aspx_num_env == 1)
        ssch->aspx_qmode_env = 0;

    aspx_delta_dir(s, ssch);
    aspx_hfgen_iwc_1ch(s, ss, ssch);

    memset(ssch->aspx_data, 0, sizeof(ssch->aspx_data));

    ret = aspx_ec_data(s, ss, ssch, DT_SIGNAL,
                       ssch->aspx_num_env,
                       ssch->atsg_freqres,
                       ssch->aspx_qmode_env,
                       0,
                       ssch->aspx_sig_delta_dir);
    if (ret < 0)
        return ret;
    ret = aspx_ec_data(s, ss, ssch, DT_NOISE,
                       ssch->aspx_num_noise,
                       0,
                       0,
                       0,
                       ssch->aspx_noise_delta_dir);
    return ret;
}

static int acpl_framing_data(AC4DecodeContext *s, Substream *ss,
                             SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;

    ssch->acpl_interpolation_type = get_bits1(gb);
    ssch->acpl_num_param_sets_cod = get_bits1(gb);
    if (ssch->acpl_interpolation_type) {
        for (int ps = 0; ps < ssch->acpl_num_param_sets_cod + 1; ps++)
            ssch->acpl_param_timeslot[ps] = get_bits(gb, 5);
    }

    return 0;
}

static VLC *get_acpl_hcb(int data_type, int quant_mode, int hcb_type)
{
    VLC *acpl_cb;

    acpl_cb = &acpl_codebook_vlc[data_type][quant_mode][hcb_type];

    return acpl_cb;
}

static int get_acpl_off(int data_type, int quant_mode, int hcb_type)
{
    int off;

    off = acpl_codebook_signal_off[data_type][quant_mode][hcb_type];

    return off;
}

static int acpl_huff_data(AC4DecodeContext *s,
                          int data_type, int data_bands,
                          int start_band, int quant_mode,
                          int *data)
{
    GetBitContext *gb = &s->gbc;
    int diff_type;
    VLC *acpl_cb;
    int acpl_off;

    switch (data_type) {
    case ALPHA1:
    case ALPHA2:
        data_type = 0;
        break;
    case BETA1:
    case BETA2:
        data_type = 1;
        break;
    case BETA3:
        data_type = 2;
        break;
    default:
        data_type = 3;
        break;
    };

    diff_type = get_bits1(gb);
    if (diff_type == 0) { // DIFF_FREQ
        acpl_cb = get_acpl_hcb(data_type, quant_mode, F0);
        acpl_off = get_acpl_off(data_type, quant_mode, F0);
        data[start_band] = get_vlc2(gb, acpl_cb->table, acpl_cb->bits, 3);
        if (data[start_band] < 0)
            return AVERROR_INVALIDDATA;
        data[start_band] -= acpl_off;
        acpl_cb = get_acpl_hcb(data_type, quant_mode, DF);
        acpl_off = get_acpl_off(data_type, quant_mode, DF);
        for (int i = start_band + 1; i < data_bands; i++) {
            data[i] = get_vlc2(gb, acpl_cb->table, acpl_cb->bits, 3);
            if (data[i] < 0)
                return AVERROR_INVALIDDATA;
            data[i] -= acpl_off;
        }
    } else { // DIFF_TIME
        acpl_cb = get_acpl_hcb(data_type, quant_mode, DT);
        acpl_off = get_acpl_off(data_type, quant_mode, DT);
        for (int i = start_band; i < data_bands; i++) {
            data[i] = get_vlc2(gb, acpl_cb->table, acpl_cb->bits, 3);
            if (data[i] < 0)
                return AVERROR_INVALIDDATA;
            data[i] -= acpl_off;
        }
    }

    return 0;
}

static int acpl_ec_data(AC4DecodeContext *s, Substream *ss,
                        SubstreamChannel *ssch,
                        int data_type, int data_bands,
                        int start_band, int quant_mode)
{
    int ret;

    for (int ps = 0; ps < ssch->acpl_num_param_sets_cod + 1; ps++) {
        ret = acpl_huff_data(s, data_type, data_bands,
                             start_band, quant_mode,
                             ssch->acpl_data[data_type]);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int acpl_data_2ch(AC4DecodeContext *s, Substream *ss,
                         SubstreamChannel *ssch0,
                         SubstreamChannel *ssch1)
{
    int ret, num_bands, st;

    acpl_framing_data(s, ss, ssch0);

    num_bands = acpl_num_param_bands[ss->acpl_num_param_bands_id];
    st = ss->acpl_param_band;

    ret = acpl_ec_data(s, ss, ssch0, ALPHA1, num_bands, st, ss->acpl_quant_mode[0]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch0, ALPHA2, num_bands, st, ss->acpl_quant_mode[0]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch0, BETA1, num_bands, st, ss->acpl_quant_mode[0]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch0, BETA2, num_bands, st, ss->acpl_quant_mode[0]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch0, BETA3, num_bands, st, ss->acpl_quant_mode[0]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch1, GAMMA1, num_bands, st, ss->acpl_quant_mode[1]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch1, GAMMA2, num_bands, st, ss->acpl_quant_mode[1]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch1, GAMMA3, num_bands, st, ss->acpl_quant_mode[1]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch1, GAMMA4, num_bands, st, ss->acpl_quant_mode[1]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch1, GAMMA5, num_bands, st, ss->acpl_quant_mode[1]);
    if (ret < 0)
        return ret;
    ret = acpl_ec_data(s, ss, ssch1, GAMMA6, num_bands, st, ss->acpl_quant_mode[1]);
    if (ret < 0)
        return ret;

    return 0;
}

static int acpl_data_1ch(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    int ret, num_bands, start;

    acpl_framing_data(s, ss, ssch);

    num_bands = acpl_num_param_bands[ss->acpl_num_param_bands_id];
    start = ss->acpl_param_band;

    ret = acpl_ec_data(s, ss, ssch, ALPHA1, num_bands, start, ss->acpl_quant_mode[0]);
    if (ret < 0)
        return ret;

    ret = acpl_ec_data(s, ss, ssch, BETA1, num_bands, start, ss->acpl_quant_mode[0]);
    if (ret < 0)
        return ret;

    return 0;
}

static int channel_pair_element(AC4DecodeContext *s, int iframe)
{
    GetBitContext *gb = &s->gbc;
    Substream *ss = &s->substream;
    int spec_frontend, ret = 0;

    s->ch_map[AV_CHAN_FRONT_LEFT]  = 0;
    s->ch_map[AV_CHAN_FRONT_RIGHT] = 1;

    ss->codec_mode = get_bits(gb, 2);
    av_log(s->avctx, AV_LOG_DEBUG, "codec_mode: %d\n", ss->codec_mode);
    if (iframe) {
        if (ss->codec_mode != CM_SIMPLE)
            aspx_config(s, ss);
        if (ss->codec_mode == CM_ASPX_ACPL_1)
            acpl_config_1ch(s, ss, ACPL_PARTIAL);
        if (ss->codec_mode == CM_ASPX_ACPL_2)
            acpl_config_1ch(s, ss, ACPL_FULL);
    }

    switch (ss->codec_mode) {
    case CM_SIMPLE:
        ret = stereo_data(s, ss, iframe);
        break;
    case CM_ASPX:
        companding_control(s, ss, 2);
        ret = stereo_data(s, ss, iframe);
        if (ret < 0)
            return ret;
        ret = aspx_data_2ch(s, ss, (uint8_t []){0, 1}, iframe);
        break;
    case CM_ASPX_ACPL_1:
        companding_control(s, ss, 1);
        ss->mdct_stereo_proc[0] = get_bits1(gb);
        if (ss->mdct_stereo_proc[0]) {
            ss->spec_frontend_m = SF_ASF;
            ss->spec_frontend_s = SF_ASF;
            ret = sf_info(s, ss, 0, SF_ASF, 1, 0);
            if (ret < 0)
                return ret;

            memcpy(&ss->ssch[1].scp, &ss->ssch[0].scp, sizeof(ss->ssch[0].scp));
            memcpy(&ss->ssch[1].sect_sfb_offset, &ss->ssch[0].sect_sfb_offset, sizeof(ss->ssch[0].sect_sfb_offset));
            memcpy(&ss->ssch[1].offset2sfb, &ss->ssch[0].offset2sfb, sizeof(ss->ssch[0].offset2sfb));
            memcpy(&ss->ssch[1].offset2g, &ss->ssch[0].offset2g, sizeof(ss->ssch[0].offset2g));
            memcpy(&ss->ssch[1].win_offset, &ss->ssch[0].win_offset, sizeof(ss->ssch[0].win_offset));

            ret = chparam_info(s, ss, 0);
            if (ret < 0)
                return ret;
        } else {
            ss->spec_frontend_m = get_bits1(gb);
            ret = sf_info(s, ss, 0, ss->spec_frontend_m, 0, 0);
            if (ret < 0)
                return ret;
            ss->spec_frontend_s = get_bits1(gb);
            ret = sf_info(s, ss, 1, ss->spec_frontend_s, 0, 1);
            if (ret < 0)
                return ret;
        }
        ret = sf_data(s, ss, 0, iframe, ss->spec_frontend_m);
        if (ret < 0)
            return ret;
        ret = sf_data(s, ss, 1, iframe, ss->spec_frontend_m);
        if (ret < 0)
            return ret;
        ret = aspx_data_1ch(s, ss, 0, iframe);
        if (ret < 0)
            return ret;
        ret = acpl_data_1ch(s, ss, 0);
        break;
    case CM_ASPX_ACPL_2:
        companding_control(s, ss, 1);
        spec_frontend = get_bits1(gb);
        ret = sf_info(s, ss, 0, spec_frontend, 0, 0);
        if (ret < 0)
            return ret;
        ret = sf_data(s, ss, 0, iframe, spec_frontend);
        if (ret < 0)
            return ret;
        ret = aspx_data_1ch(s, ss, 0, iframe);
        if (ret < 0)
            return ret;
        ret = acpl_data_1ch(s, ss, 0);
        break;
    }

    return ret;
}

static int four_channel_data(AC4DecodeContext *s, Substream *ss, int iframe, uint8_t ch_id[4])
{
    int ret;

    ret = sf_info(s, ss, ch_id[0], SF_ASF, 0, 0);
    if (ret < 0)
        return ret;

    for (int i = 1; i < 4; i++)
        memcpy(&ss->ssch[ch_id[i]], &ss->ssch[ch_id[0]], sizeof(ss->ssch[0]));

    for (int i = 0; i < 4; i++) {
        ret = chparam_info(s, ss, ch_id[i]);
        if (ret < 0)
            return ret;
    }

    for (int i = 0; i < 4; i++) {
        ret = sf_data(s, ss, ch_id[i], iframe, SF_ASF);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int five_channel_info(AC4DecodeContext *s, Substream *ss, uint8_t ch_id[5])
{
    GetBitContext *gb = &s->gbc;
    int ret;

    ss->chel_matsel = get_bits(gb, 4);

    for (int n = 0; n < 5; n++) {
        ret = chparam_info(s, ss, ch_id[n]);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int five_channel_data(AC4DecodeContext *s, Substream *ss, int iframe, uint8_t ch_id[5])
{
    SubstreamChannel *ssch[5];
    int ret;

    for (int n = 0; n < 5; n++)
        ssch[n] = &ss->ssch[ch_id[n]];

    ret = sf_info(s, ss, ch_id[0], SF_ASF, 0, 0);
    if (ret < 0)
        return ret;

    for (int i = 1; i < 5; i++) {
        memcpy(&ssch[i]->scp, &ssch[0]->scp, sizeof(ss->ssch[0].scp));
        memcpy(&ssch[i]->sect_sfb_offset, &ssch[0]->sect_sfb_offset, sizeof(ss->ssch[0].sect_sfb_offset));
        memcpy(&ssch[i]->offset2sfb, &ssch[0]->offset2sfb, sizeof(ss->ssch[0].offset2sfb));
        memcpy(&ssch[i]->offset2g, &ssch[0]->offset2g, sizeof(ss->ssch[0].offset2g));
        memcpy(&ssch[i]->win_offset, &ssch[0]->win_offset, sizeof(ss->ssch[0].win_offset));
    }

    ret = five_channel_info(s, ss, ch_id);
    if (ret < 0)
        return ret;

    for (int n = 0; n < 5; n++) {
        ret = sf_data(s, ss, ch_id[n], iframe, SF_ASF);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int sf_info_lfe(AC4DecodeContext *s, Substream *ss,
                        SubstreamChannel *ssch)
{
    GetBitContext *gb = &s->gbc;
    int n_msfbl_bits = get_msfbl_bits(s->frame_len_base);
    int n_grp_bits = get_grp_bits(s, ssch);

    ssch->scp.long_frame = 1;
    ssch->scp.max_sfb[0] = get_bits(gb, n_msfbl_bits);
    ssch->scp.num_window_groups = 1;
    ssch->scp.transf_length_idx[0] = 4;

    return asf_psy_elements(s, ss, ssch, n_grp_bits);
}

static int mono_data(AC4DecodeContext *s, Substream *ss,
                     int ch_id, int lfe, int iframe)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    GetBitContext *gb = &s->gbc;
    int spec_frontend;
    int ret;

    if (lfe) {
        spec_frontend = SF_ASF;
        ret = sf_info_lfe(s, ss, ssch);
    } else {
        spec_frontend = get_bits1(gb);
        ret = sf_info(s, ss, ch_id, spec_frontend, 0, 0);
    }
    if (ret < 0)
        return ret;
    return sf_data(s, ss, ch_id, iframe, spec_frontend);
}

static int two_channel_data(AC4DecodeContext *s, Substream *ss,
                            const uint8_t ch_id[2], int x, int iframe)
{
    SubstreamChannel *ssch0 = &ss->ssch[ch_id[0]];
    SubstreamChannel *ssch1 = &ss->ssch[ch_id[1]];
    GetBitContext *gb = &s->gbc;
    int ret;

    ss->mdct_stereo_proc[x] = get_bits1(gb);
    if (ss->mdct_stereo_proc[x]) {
        ret = sf_info(s, ss, ch_id[0], SF_ASF, 0, 0);
        if (ret < 0)
            return ret;

        memcpy(&ssch1->scp, &ssch0->scp, sizeof(ss->ssch[0].scp));
        memcpy(&ssch1->sect_sfb_offset, &ssch0->sect_sfb_offset, sizeof(ss->ssch[0].sect_sfb_offset));
        memcpy(&ssch1->offset2sfb, &ssch0->offset2sfb, sizeof(ss->ssch[0].offset2sfb));
        memcpy(&ssch1->offset2g, &ssch0->offset2g, sizeof(ss->ssch[0].offset2g));
        memcpy(&ssch1->win_offset, &ssch0->win_offset, sizeof(ss->ssch[0].win_offset));

        ret = chparam_info(s, ss, ch_id[0]);
        if (ret < 0)
            return ret;
    } else {
        ret = sf_info(s, ss, ch_id[0], SF_ASF, 0, 0);
        if (ret < 0)
            return ret;
        ret = sf_info(s, ss, ch_id[1], SF_ASF, 0, 0);
        if (ret < 0)
            return ret;
    }
    ret = sf_data(s, ss, ch_id[0], iframe, SF_ASF);
    if (ret < 0)
        return ret;
    ret = sf_data(s, ss, ch_id[1], iframe, SF_ASF);

    return ret;
}

static int three_channel_info(AC4DecodeContext *s, Substream *ss,
                              uint8_t ch_id[3])
{
    GetBitContext *gb = &s->gbc;
    int ret;

    ss->chel_matsel = get_bits(gb, 4);
    ret = chparam_info(s, ss, ch_id[0]);
    if (ret < 0)
        return ret;
    return chparam_info(s, ss, ch_id[1]);
}

static int three_channel_data(AC4DecodeContext *s, Substream *ss,
                              uint8_t ch_id[3], int iframe)
{
    SubstreamChannel *ssch0 = &ss->ssch[ch_id[0]];
    SubstreamChannel *ssch1 = &ss->ssch[ch_id[1]];
    SubstreamChannel *ssch2 = &ss->ssch[ch_id[2]];
    int ret;

    ret = sf_info(s, ss, ch_id[0], SF_ASF, 0, 0);
    if (ret < 0)
        return ret;

    memcpy(&ssch1->scp, &ssch0->scp, sizeof(ss->ssch[0].scp));
    memcpy(&ssch1->sect_sfb_offset, &ssch0->sect_sfb_offset, sizeof(ss->ssch[0].sect_sfb_offset));
    memcpy(&ssch1->offset2sfb, &ssch0->offset2sfb, sizeof(ss->ssch[0].offset2sfb));
    memcpy(&ssch1->offset2g, &ssch0->offset2g, sizeof(ss->ssch[0].offset2g));
    memcpy(&ssch1->win_offset, &ssch0->win_offset, sizeof(ss->ssch[0].win_offset));

    memcpy(&ssch2->scp, &ssch0->scp, sizeof(ss->ssch[0].scp));
    memcpy(&ssch2->sect_sfb_offset, &ssch0->sect_sfb_offset, sizeof(ss->ssch[0].sect_sfb_offset));
    memcpy(&ssch2->offset2sfb, &ssch0->offset2sfb, sizeof(ss->ssch[0].offset2sfb));
    memcpy(&ssch2->offset2g, &ssch0->offset2g, sizeof(ss->ssch[0].offset2g));
    memcpy(&ssch2->win_offset, &ssch0->win_offset, sizeof(ss->ssch[0].win_offset));

    ret = three_channel_info(s, ss, ch_id);
    if (ret < 0)
        return ret;
    ret = sf_data(s, ss, ch_id[0], iframe, SF_ASF);
    if (ret < 0)
        return ret;
    ret = sf_data(s, ss, ch_id[1], iframe, SF_ASF);
    if (ret < 0)
        return ret;
    ret = sf_data(s, ss, ch_id[2], iframe, SF_ASF);
    if (ret < 0)
        return ret;

    return 0;
}

static int immers_cfg(AC4DecodeContext *s, Substream *ss)
{
    if (ss->im_codec_mode != IM_SCPL)
        aspx_config(s, ss);

    if (ss->im_codec_mode == IM_ASPX_ACPL_1)
        acpl_config_1ch(s, ss, ACPL_PARTIAL);

    if (ss->im_codec_mode == IM_ASPX_ACPL_2)
        acpl_config_1ch(s, ss, ACPL_FULL);

    return 0;
}

static const uint8_t immersive_ch_index[2][2][2] = {
    {
        { 0, 1 },
        { 3, 4 },
    },
    {
        { 0, 3 },
        { 1, 4 },
    },
};

static int immersive_channel_element(AC4DecodeContext *s, int lfe, int fronts, int iframe)
{
    GetBitContext *gb = &s->gbc;
    Substream *ss = &s->substream;
    int ret;

    s->ch_map[AV_CHAN_FRONT_LEFT]      = 0;
    s->ch_map[AV_CHAN_FRONT_RIGHT]     = 1;
    s->ch_map[AV_CHAN_FRONT_CENTER]    = 2;
    s->ch_map[AV_CHAN_LOW_FREQUENCY]   = 7;
    s->ch_map[AV_CHAN_SIDE_LEFT]       = 3;
    s->ch_map[AV_CHAN_SIDE_RIGHT]      = 4;
    s->ch_map[AV_CHAN_BACK_LEFT]       = 5;
    s->ch_map[AV_CHAN_BACK_RIGHT]      = 6;
    s->ch_map[AV_CHAN_TOP_FRONT_LEFT]  = 8;
    s->ch_map[AV_CHAN_TOP_FRONT_RIGHT] = 9;
    s->ch_map[AV_CHAN_TOP_BACK_LEFT]   = 10;
    s->ch_map[AV_CHAN_TOP_BACK_RIGHT]  = 11;

    ss->im_codec_mode = get_bits1(gb);
    if (!ss->im_codec_mode)
        ss->im_codec_mode = get_bits(gb, 2);
    else
        ss->im_codec_mode = IM_ASPX_AJCC;
    av_log(s->avctx, AV_LOG_DEBUG, "im_codec_mode: %d\n", ss->im_codec_mode);

    if (iframe)
        immers_cfg(s, ss);

    if (lfe) {
        ret = mono_data(s, ss, 7, 1, iframe);
        if (ret < 0)
            return ret;
    }

    if (ss->im_codec_mode == IM_ASPX_AJCC)
        companding_control(s, ss, 5);

    ss->core_5ch_grouping = get_bits(gb, 2);
    av_log(s->avctx, AV_LOG_DEBUG, "core_5ch_grouping: %d\n", ss->core_5ch_grouping);
    switch (ss->core_5ch_grouping) {
    case 0:
        ss->mode_2ch = get_bits1(gb);
        ret = two_channel_data(s, ss, immersive_ch_index[ss->mode_2ch][0], 0, iframe);
        if (ret < 0)
            return ret;
        ret = two_channel_data(s, ss, immersive_ch_index[ss->mode_2ch][1], 1, iframe);
        if (ret < 0)
            return ret;
        ret = mono_data(s, ss, 2, 0, iframe);
        break;
    case 1:
        ret = three_channel_data(s, ss, (uint8_t []){0, 1, 2}, iframe);
        if (ret < 0)
            return ret;
        ret = two_channel_data(s, ss, (uint8_t []){3, 4}, 0, iframe);
        if (ret < 0)
            return ret;
        break;
    case 2:
        ret = four_channel_data(s, ss, iframe, (uint8_t []){0, 1, 2, 3});
        if (ret < 0)
            return ret;
        ret = mono_data(s, ss, 4, 0, iframe);
        break;
    case 3:
        ret = five_channel_data(s, ss, iframe, (uint8_t []){0, 1, 2, 3, 4});
        break;
    }

    if (ret < 0)
        return ret;

    if (ss->im_codec_mode != IM_ASPX_AJCC) {
        ss->use_sap_add_ch = get_bits1(gb);

        if (ss->use_sap_add_ch) {
            ret = chparam_info(s, ss, 5);
            if (ret < 0)
                return ret;
            ret = chparam_info(s, ss, 6);
            if (ret < 0)
                return ret;
        }

        ret = two_channel_data(s, ss, (uint8_t []){5, 6}, 2, iframe);
        if (ret < 0)
            return ret;
    }

    if (ss->im_codec_mode == IM_ASPX_SCPL) {
        ret = aspx_data_2ch(s, ss, (uint8_t []){0, 1}, iframe);
        if (ret < 0)
            return ret;
        ret = aspx_data_2ch(s, ss, (uint8_t []){2, 3}, iframe);
        if (ret < 0)
            return ret;
        ret = aspx_data_1ch(s, ss, 7, iframe);
        if (ret < 0)
            return ret;

        if (fronts) {
            ret = aspx_data_2ch(s, ss, (uint8_t []){4, 5}, iframe);
            if (ret < 0)
                return ret;
            ret = aspx_data_2ch(s, ss, (uint8_t []){6, 8}, iframe);
            if (ret < 0)
                return ret;
        } else {
            ret = aspx_data_2ch(s, ss, (uint8_t []){4, 5}, iframe);
            if (ret < 0)
                return ret;
        }

        ret = aspx_data_2ch(s, ss, (uint8_t []){9, 10}, iframe);
        if (ret < 0)
            return ret;
        ret = aspx_data_2ch(s, ss, (uint8_t []){10, 11}, iframe);
        if (ret < 0)
            return ret;
    } else if (ss->im_codec_mode != IM_SCPL) {
        ret = aspx_data_2ch(s, ss, (uint8_t []){0, 1}, iframe);
        if (ret < 0)
            return ret;
        ret = aspx_data_2ch(s, ss, (uint8_t []){3, 4}, iframe);
        if (ret < 0)
            return ret;

        if (ss->im_codec_mode != IM_ASPX_AJCC) {
            ret = aspx_data_2ch(s, ss, (uint8_t []){5, 6}, iframe);
            if (ret < 0)
                return ret;
        }

        ret = aspx_data_1ch(s, ss, 2, iframe);
        if (ret < 0)
            return ret;
    }

    if (ss->im_codec_mode == IM_ASPX_AJCC)
        ;

    if (ss->im_codec_mode == IM_SCPL ||
        ss->im_codec_mode == IM_ASPX_SCPL ||
        ss->im_codec_mode == IM_ASPX_ACPL_1) {
        ret = two_channel_data(s, ss, (uint8_t []){8, 9}, 3, iframe);
        if (ret < 0)
            return ret;

        ret = two_channel_data(s, ss, (uint8_t []){10, 11}, 4, iframe);
        if (ret < 0)
            return ret;
    }

    if (ss->im_codec_mode == IM_ASPX_ACPL_1 ||
        ss->im_codec_mode == IM_ASPX_ACPL_2) {
        ret = acpl_data_1ch(s, ss, 0);
        if (ret < 0)
            return ret;
        ret = acpl_data_1ch(s, ss, 1);
        if (ret < 0)
            return ret;
        ret = acpl_data_1ch(s, ss, 2);
        if (ret < 0)
            return ret;
        ret = acpl_data_1ch(s, ss, 3);
        if (ret < 0)
            return ret;
        if (fronts) {
            ret = acpl_data_1ch(s, ss, 4);
            if (ret < 0)
                return ret;
            ret = acpl_data_1ch(s, ss, 5);
            if (ret < 0)
                return ret;
        }
    }

    return 0;
}

static int channel_element_7x(AC4DecodeContext *s, int channel_mode, int iframe)
{
    GetBitContext *gb = &s->gbc;
    Substream *ss = &s->substream;
    int ret = 0;

    ss->codec_mode = get_bits(gb, 2);
    av_log(s->avctx, AV_LOG_DEBUG, "codec_mode: %d\n", ss->codec_mode);
    if (iframe) {
        if (ss->codec_mode != CM_SIMPLE)
            aspx_config(s, ss);
        if (ss->codec_mode == CM_ASPX_ACPL_1)
            acpl_config_1ch(s, ss, ACPL_PARTIAL);
        if (ss->codec_mode == CM_ASPX_ACPL_2)
            acpl_config_1ch(s, ss, ACPL_FULL);
    }

    if (channel_mode == 6) {
        ret = mono_data(s, ss, 7, 1, iframe);
        if (ret < 0)
            return ret;
    }

    if (ss->codec_mode == CM_ASPX_ACPL_1 ||
        ss->codec_mode == CM_ASPX_ACPL_2)
        companding_control(s, ss, 5);

    ss->coding_config = get_bits(gb, 2);
    av_log(s->avctx, AV_LOG_DEBUG, "coding config: %d\n", ss->coding_config);
    switch (ss->coding_config) {
    case 0:
        ss->mode_2ch = get_bits1(gb);
        ret = two_channel_data(s, ss, (uint8_t []){0, 1}, 0, iframe);
        if (ret < 0)
            return ret;
        ret = two_channel_data(s, ss, (uint8_t []){2, 3}, 1, iframe);
        break;
    case 1:
        ret = three_channel_data(s, ss, (uint8_t []){0, 1, 2}, iframe);
        if (ret < 0)
            return ret;
        ret = two_channel_data(s, ss, (uint8_t []){3, 4}, 1, iframe);
        break;
    case 2:
        ret = four_channel_data(s, ss, iframe, (uint8_t []){0, 1, 2, 3});
        break;
    case 3:
        ret = five_channel_data(s, ss, iframe, (uint8_t []){0, 1, 2, 3, 4});
        break;
    default:
        av_assert0(0);
    }
    if (ret < 0)
        return ret;

    if (ss->codec_mode == CM_SIMPLE || ss->codec_mode == CM_ASPX) {
        if (get_bits1(gb)) {
            chparam_info(s, ss, 5);
            chparam_info(s, ss, 6);
        }
        ret = two_channel_data(s, ss, (uint8_t []){5, 6}, 1, iframe);
        if (ret < 0)
            return ret;
    }

    if (ss->codec_mode == CM_ASPX_ACPL_1) {
        // XXX
    }

    return ret;
}

static int channel_element_3x(AC4DecodeContext *s, int iframe)
{
    GetBitContext *gb = &s->gbc;
    Substream *ss = &s->substream;
    int ret;

    ss->codec_mode = get_bits1(gb);
    av_log(s->avctx, AV_LOG_DEBUG, "codec_mode: %d\n", ss->codec_mode);
    if (ss->codec_mode == CM_ASPX) {
        if (iframe)
            aspx_config(s, ss);
        companding_control(s, ss, 3);
    }

    s->ch_map[AV_CHAN_FRONT_LEFT]   = 0;
    s->ch_map[AV_CHAN_FRONT_RIGHT]  = 1;
    s->ch_map[AV_CHAN_FRONT_CENTER] = 2;

    ss->coding_config = get_bits1(gb);
    switch (ss->coding_config) {
    case 0:
        ret = stereo_data(s, ss, iframe);
        if (ret < 0)
            return ret;
        ret = mono_data(s, ss, 2, 0, iframe);
        if (ret < 0)
            return ret;
        break;
    case 1:
        ret = three_channel_data(s, ss, (uint8_t []){0, 1, 2}, iframe);
        if (ret < 0)
            return ret;
        break;
    }

    if (ss->codec_mode == CM_ASPX) {
        ret = aspx_data_2ch(s, ss, (uint8_t []){0, 1}, iframe);
        if (ret < 0)
            return ret;
        ret = aspx_data_1ch(s, ss, 2, iframe);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int channel_element_5x(AC4DecodeContext *s, int lfe, int iframe)
{
    GetBitContext *gb = &s->gbc;
    Substream *ss = &s->substream;
    int ret = 0;

    ss->codec_mode = get_bits(gb, 3);
    av_log(s->avctx, AV_LOG_DEBUG, "codec_mode: %d\n", ss->codec_mode);
    if (iframe) {
        if (ss->codec_mode != CM_SIMPLE)
            aspx_config(s, ss);
        if (ss->codec_mode == CM_ASPX_ACPL_1)
            acpl_config_1ch(s, ss, ACPL_PARTIAL);
        if (ss->codec_mode == CM_ASPX_ACPL_2)
            acpl_config_1ch(s, ss, ACPL_FULL);
        if (ss->codec_mode == CM_ASPX_ACPL_3)
            acpl_config_2ch(s, ss);
    }

    if (lfe) {
        s->ch_map[AV_CHAN_LOW_FREQUENCY] = 5;

        ret = mono_data(s, ss, 5, 1, iframe);
        if (ret < 0)
            return ret;
    }

    switch (ss->codec_mode) {
    case CM_SIMPLE:
    case CM_ASPX:
        if (ss->codec_mode == CM_ASPX)
            companding_control(s, ss, 5);

        ss->coding_config = get_bits(gb, 2);
        av_log(s->avctx, AV_LOG_DEBUG, "coding_config: %d\n", ss->coding_config);
        switch (ss->coding_config) {
        case 0:
            ss->mode_2ch = get_bits1(gb);

            if (ss->mode_2ch) {
                s->ch_map[AV_CHAN_FRONT_LEFT]  = 0;
                s->ch_map[AV_CHAN_SIDE_LEFT]   = 1;
                s->ch_map[AV_CHAN_FRONT_RIGHT] = 2;
                s->ch_map[AV_CHAN_SIDE_RIGHT]  = 3;
            } else {
                s->ch_map[AV_CHAN_FRONT_LEFT]  = 0;
                s->ch_map[AV_CHAN_FRONT_RIGHT] = 1;
                s->ch_map[AV_CHAN_SIDE_LEFT]   = 2;
                s->ch_map[AV_CHAN_SIDE_RIGHT]  = 3;
            }
            s->ch_map[AV_CHAN_FRONT_CENTER] = 4;

            ret = two_channel_data(s, ss, (uint8_t []){0, 1}, 0, iframe);
            if (ret < 0)
                return ret;
            ret = two_channel_data(s, ss, (uint8_t []){2, 3}, 1, iframe);
            if (ret < 0)
                return ret;
            ret = mono_data(s, ss, 4, 0, iframe);
            break;
        case 1:
            s->ch_map[AV_CHAN_FRONT_LEFT]   = 0;
            s->ch_map[AV_CHAN_FRONT_RIGHT]  = 1;
            s->ch_map[AV_CHAN_FRONT_CENTER] = 2;
            s->ch_map[AV_CHAN_SIDE_LEFT]    = 3;
            s->ch_map[AV_CHAN_SIDE_RIGHT]   = 4;

            ret = three_channel_data(s, ss, (uint8_t []){0, 1, 2}, iframe);
            if (ret < 0)
                return ret;
            ret = two_channel_data(s, ss, (uint8_t []){3, 4}, 0, iframe);
            break;
        case 2:
            s->ch_map[AV_CHAN_FRONT_LEFT]   = 0;
            s->ch_map[AV_CHAN_FRONT_RIGHT]  = 1;
            s->ch_map[AV_CHAN_SIDE_LEFT]    = 2;
            s->ch_map[AV_CHAN_SIDE_RIGHT]   = 3;
            s->ch_map[AV_CHAN_FRONT_CENTER] = 4;

            ret = four_channel_data(s, ss, iframe, (uint8_t []){0, 1, 2, 3});
            if (ret < 0)
                return ret;
            ret = mono_data(s, ss, 4, 0, iframe);
            break;
        case 3:
            s->ch_map[AV_CHAN_FRONT_LEFT]   = 0;
            s->ch_map[AV_CHAN_FRONT_RIGHT]  = 1;
            s->ch_map[AV_CHAN_FRONT_CENTER] = 2;
            s->ch_map[AV_CHAN_SIDE_LEFT]    = 3;
            s->ch_map[AV_CHAN_SIDE_RIGHT]   = 4;

            ret = five_channel_data(s, ss, iframe, (uint8_t []){0, 1, 2, 3, 4});
            break;
        }
        if (ret < 0)
            return ret;

        if (ss->codec_mode == CM_ASPX) {
            ret = aspx_data_2ch(s, ss, (uint8_t []){0, 1}, iframe);
            if (ret < 0)
                return ret;
            ret = aspx_data_2ch(s, ss, (uint8_t []){2, 3}, iframe);
            if (ret < 0)
                return ret;
            ret = aspx_data_1ch(s, ss, 4, iframe);
            if (ret < 0)
                return ret;
        }
        break;
    case CM_ASPX_ACPL_1:
    case CM_ASPX_ACPL_2:
        companding_control(s, ss, 3);
        ss->coding_config = get_bits1(gb);
        av_log(s->avctx, AV_LOG_DEBUG, "coding_config: %d\n", ss->coding_config);
        if (ss->coding_config)
            ret = three_channel_data(s, ss, (uint8_t []){0, 1, 2}, iframe);
        else
            ret = two_channel_data(s, ss, (uint8_t []){0, 1}, 0, iframe);
        if (ret < 0)
            return ret;

        if (ss->codec_mode == CM_ASPX_ACPL_1) {
            ss->max_sfb_master = get_bits(gb, 5); // XXX
            ret = chparam_info(s, ss, 2 + ss->coding_config);
            if (ret < 0)
                return ret;
            ret = chparam_info(s, ss, 3 + ss->coding_config);
            if (ret < 0)
                return ret;
            ret = sf_data(s, ss, 2 + ss->coding_config, iframe, SF_ASF);
            if (ret < 0)
                return ret;
            ret = sf_data(s, ss, 3 + ss->coding_config, iframe, SF_ASF);
            if (ret < 0)
                return ret;
        }
        if (ss->coding_config == 0) {
            ret = mono_data(s, ss, 2, 0, iframe);
            if (ret < 0)
                return ret;
        }

        s->ch_map[AV_CHAN_FRONT_LEFT]   = 0;
        s->ch_map[AV_CHAN_FRONT_RIGHT]  = 1;
        s->ch_map[AV_CHAN_FRONT_CENTER] = 2;
        s->ch_map[AV_CHAN_SIDE_LEFT]    = 3;
        s->ch_map[AV_CHAN_SIDE_RIGHT]   = 4;

        ret = aspx_data_2ch(s, ss, (uint8_t []){0, 1}, iframe);
        if (ret < 0)
            return ret;
        ret = aspx_data_1ch(s, ss, 2, iframe);
        if (ret < 0)
            return ret;
        ret = acpl_data_1ch(s, ss, 0);
        if (ret < 0)
            return ret;
        ret = acpl_data_1ch(s, ss, 1);
        break;
    case CM_ASPX_ACPL_3:
        companding_control(s, ss, 2);
        ret = stereo_data(s, ss, iframe);
        if (ret < 0)
            return ret;

        s->ch_map[AV_CHAN_FRONT_LEFT]   = 0;
        s->ch_map[AV_CHAN_FRONT_RIGHT]  = 1;
        s->ch_map[AV_CHAN_FRONT_CENTER] = 2;
        s->ch_map[AV_CHAN_SIDE_LEFT]    = 3;
        s->ch_map[AV_CHAN_SIDE_RIGHT]   = 4;

        ret = aspx_data_2ch(s, ss, (uint8_t []){0, 1}, iframe);
        if (ret < 0)
            return ret;
        ret = acpl_data_2ch(s, ss, &ss->ssch[0], &ss->ssch[1]);
        break;
    default:
        av_log(s->avctx, AV_LOG_ERROR, "invalid codec mode: %d\n", ss->codec_mode);
        return AVERROR_INVALIDDATA;
    }

    return ret;
}

static int single_channel_element(AC4DecodeContext *s, int iframe)
{
    GetBitContext *gb = &s->gbc;
    Substream *ss = &s->substream;
    int ret = 0;

    s->ch_map[AV_CHAN_FRONT_CENTER] = 0;

    ss->codec_mode = get_bits1(gb);
    av_log(s->avctx, AV_LOG_DEBUG, "codec_mode: %d\n", ss->codec_mode);
    if (iframe) {
        if (ss->codec_mode == CM_ASPX)
            aspx_config(s, ss);
    }
    if (ss->codec_mode == CM_SIMPLE) {
        ret = mono_data(s, ss, 0, 0, iframe);
    } else {
        companding_control(s, ss, 1);
        ret = mono_data(s, ss, 0, 0, iframe);
        if (ret < 0)
            return ret;
        ret = aspx_data_1ch(s, ss, 0, iframe);
    }

    return ret;
}

static int audio_data(AC4DecodeContext *s, int channel_mode, int iframe)
{
    int ret = 0;

    av_log(s->avctx, AV_LOG_DEBUG, "channel_mode: %d\n", channel_mode);
    switch (channel_mode) {
    case 0:
        ret = single_channel_element(s, iframe);
        break;
    case 1:
        ret = channel_pair_element(s, iframe);
        break;
    case 2:
        ret = channel_element_3x(s, iframe);
        break;
    case 3:
        ret = channel_element_5x(s, 0, iframe);
        break;
    case 4:
        ret = channel_element_5x(s, 1, iframe);
        break;
    case 5:
        ret = channel_element_7x(s, channel_mode, iframe);
        break;
    case 6:
        ret = channel_element_7x(s, channel_mode, iframe);
        break;
    case 11:
        ret = immersive_channel_element(s, 0, 0, iframe);
        break;
    case 12:
        ret = immersive_channel_element(s, 1, 0, iframe);
        break;
    case 13:
        ret = immersive_channel_element(s, 0, 1, iframe);
        break;
    case 14:
        ret = immersive_channel_element(s, 1, 1, iframe);
        break;
    default:
        avpriv_report_missing_feature(s->avctx, "channel_mode = %d", channel_mode);
        return AVERROR_PATCHWELCOME;
    }

    return ret;
}

static int ac4_substream(AC4DecodeContext *s,
                         SubstreamGroupInfo *ssginfo,
                         SubstreamInfo *ssinfo,
                         int substream_size)
{
    GetBitContext *gb = &s->gbc;
    int audio_size, offset, consumed, ret;

    audio_size = get_bits(gb, 15);
    if (get_bits1(gb))
        audio_size += variable_bits(gb, 7) << 15;
    if (audio_size > 131072 ||
        (substream_size > 0 && audio_size > substream_size)) {
        av_log(s->avctx, AV_LOG_ERROR, "invalid audio_size: %d\n", audio_size);
        return AVERROR_INVALIDDATA;
    }

    av_log(s->avctx, AV_LOG_DEBUG, "audio_size: %d\n", audio_size);

    align_get_bits(gb);

    offset = get_bits_count(gb) >> 3;
    if (ssginfo->channel_coded) {
        ret = audio_data(s, ssinfo->channel_mode, ssinfo->iframe[0]);
    } else {
        if (ssginfo->ajoc) {
            //audio_data_ajoc();
        } else {
            //audio_data_objs();
        }

        ret = AVERROR_PATCHWELCOME;
    }
    if (ret < 0)
        return ret;

    align_get_bits(gb);
    consumed = (get_bits_count(gb) >> 3) - offset;
    if (consumed > audio_size)
        av_log(s->avctx, AV_LOG_WARNING, "substream audio data overread: %d\n", consumed - audio_size);

    if (consumed < audio_size) {
        int non_zero = 0;

        for (int i = consumed; i < audio_size; i++)
            non_zero += !!get_bits(gb, 8);
        if (non_zero)
            av_log(s->avctx, AV_LOG_WARNING, "substream audio data underread: %d\n", non_zero);
    }

    align_get_bits(gb);

    if (substream_size > 0)
        skip_bits_long(gb, (substream_size - audio_size - 2) * 8);

    return 0;
}

/*
static int further_loudness_info(AC4DecodeContext *s, SubstreamInfo *ssi)
{
    GetBitContext *gb = &s->gbc;
    Metadata *m = &ssi->meta;

    m->loudness_version = get_bits(gb, 2);
    if (m->loudness_version == 3)
        m->loudness_version += get_bits(gb, 4);

    m->loud_prac_type = get_bits(gb, 4);
    if (m->loud_prac_type != 0) {
        if (get_bits1(gb))
            m->dialgate_prac_type = get_bits(gb, 3);
        m->loudcorr_type = get_bits1(gb);
    }

    if (get_bits1(gb))
        m->loudrelgat = get_bits(gb, 11);

    if (get_bits1(gb)) {
        m->loudspchgat = get_bits(gb, 11);
        m->dialgate_prac_type = get_bits(gb, 3);
    }

    if (get_bits1(gb))
        m->loudstrm3s = get_bits(gb, 11);

    if (get_bits1(gb))
        m->max_loudstrm3s = get_bits(gb, 11);

    if (get_bits1(gb))
        m->truepk = get_bits(gb, 11);

    if (get_bits1(gb))
        m->max_truepk = get_bits(gb, 11);

    if (get_bits1(gb)) {
        int prgmbndy_bit = 0;

        m->prgmbndy = 1;
        while (prgmbndy_bit == 0) {
            m->prgmbndy <<= 1;
            prgmbndy_bit = get_bits1(gb);
        }

        m->end_or_start = get_bits1(gb);
        if (get_bits1(gb))
            m->prgmbndy_offset = get_bits(gb, 11);
    }

    if (get_bits1(gb)) {
        m->lra = get_bits(gb, 10);
        m->lra_prac_type = get_bits(gb, 3);
    }

    if (get_bits1(gb))
        m->loudmntry = get_bits(gb, 11);

    if (get_bits1(gb))
        m->max_loudmntry = get_bits(gb, 11);

    if (get_bits1(gb)) {
        int e_bits_size = get_bits(gb, 5);
        if (e_bits_size == 31)
            e_bits_size += variable_bits(gb, 4);
        skip_bits_long(gb, e_bits_size);
    }

    return 0;
}

static int channel_mode_contains_lfe(int channel_mode)
{
    if (channel_mode == 4 ||
        channel_mode == 6 ||
        channel_mode == 8 ||
        channel_mode == 10)
        return 1;
    return 0;
}

static int basic_metadata(AC4DecodeContext *s, SubstreamInfo *ssi)
{
    GetBitContext *gb = &s->gbc;
    Metadata *m = &ssi->meta;

    if (ssi->sus_ver == 0)
        m->dialnorm_bits = get_bits(gb, 7);

    if (get_bits1(gb)) {
        if (get_bits1(gb))
            further_loudness_info(s, ssi);
        if (ssi->channel_mode == 1) {
            if (get_bits1(gb)) {
                m->pre_dmixtyp_2ch = get_bits(gb, 3);
                m->phase90_info_2ch = get_bits(gb, 2);
            }
        }

        if (ssi->channel_mode > 1) {
            if (get_bits1(gb)) {
                m->loro_center_mixgain = get_bits(gb, 3);
                m->loro_surround_mixgain = get_bits(gb, 3);
                if (get_bits1(gb))
                    m->loro_dmx_loud_corr = get_bits(gb, 5);
                if (get_bits1(gb)) {
                    m->ltrt_center_mixgain = get_bits(gb, 3);
                    m->ltrt_surround_mixgain = get_bits(gb, 3);
                }
                if (get_bits1(gb))
                    m->ltrt_dmx_loud_corr = get_bits(gb, 5);
                if (channel_mode_contains_lfe(ssi->channel_mode)) {
                    if (get_bits1(gb))
                        m->lfe_mixgain = get_bits(gb, 5);
                }
                m->preferred_dmx_method = get_bits(gb, 2);
            }
            if (ssi->channel_mode == 3 ||
                ssi->channel_mode == 4) {
                if (get_bits1(gb))
                    m->pre_dmixtyp_5ch = get_bits(gb, 3);
                if (get_bits1(gb))
                    m->pre_upmixtyp_5ch = get_bits(gb, 4);
            }

            if (ssi->channel_mode >= 5 && ssi->channel_mode <= 10) {
                if (get_bits1(gb)) {
                    if (ssi->channel_mode >= 5 && ssi->channel_mode <= 6) {
                        m->pre_upmixtyp_3_4 = get_bits(gb, 2);
                    } else if (ssi->channel_mode >= 9 && ssi->channel_mode <= 10) {
                        m->pre_upmixtyp_3_2_2 = get_bits(gb, 1);
                    }
                }
            }
            m->phase90_info_mc = get_bits(gb, 2);
            m->surround_attenuation_known = get_bits1(gb);
            m->lfe_attenuation_known = get_bits1(gb);
        }

        if (get_bits1(gb))
            m->dc_block_on = get_bits1(gb);
    }
    return 0;
}

static int extended_metadata(AC4DecodeContext *s)
{
    return 0;
}

static int drc_decoder_mode_config(AC4DecodeContext *s, SubstreamInfo *ssi)
{
    return 0;
}

static int drc_config(AC4DecodeContext *s, SubstreamInfo *ssi)
{
    GetBitContext *gb = &s->gbc;
    Metadata *m = &ssi->meta;

    m->drc_decoder_nr_modes = get_bits(gb, 3);
    for (int i = 0; i <= m->drc_decoder_nr_modes; i++)
        drc_decoder_mode_config(s, ssi);
    m->drc_eac3_profile = get_bits(gb, 3);

    return 0;
}

static int drc_data(AC4DecodeContext *s, SubstreamInfo *ssi)
{
    return 0;
}

static int drc_frame(AC4DecodeContext *s, SubstreamInfo *ssi, int iframe)
{
    GetBitContext *gb = &s->gbc;

    if (get_bits1(gb)) {
        if (iframe)
            drc_config(s, ssi);

        drc_data(s, ssi);
    }

    return 0;
}

static int dialog_enhancement(AC4DecodeContext *s, int iframe)
{
    return 0;
}

static int emdf_payloads_substream(AC4DecodeContext *s)
{
    return 0;
}

static int metadata(AC4DecodeContext *s, SubstreamInfo *ssi, int iframe)
{
    GetBitContext *gb = &s->gbc;
    int tools_metadata_size;

    basic_metadata(s, ssi);
    extended_metadata(s);
    tools_metadata_size = get_bits(gb, 7);
    if (get_bits1(gb))
        tools_metadata_size += variable_bits(gb, 3) << 7;

    drc_frame(s, ssi, iframe);

    dialog_enhancement(s, iframe);

    if (get_bits1(gb))
        emdf_payloads_substream(s);

    return 0;
}
*/

static void spectral_reordering(AC4DecodeContext *s, SubstreamChannel *ssch)
{
    float *scaled_spec = ssch->scaled_spec;
    float *spec_reord = ssch->spec_reord;
    int *win_offset = ssch->win_offset;
    int k, win;

    k = 0;
    win = 0;
    memset(ssch->spec_reord, 0, sizeof(ssch->spec_reord));

    for (int g = 0; g < ssch->scp.num_window_groups; g++) {
        int transf_length_g = get_transf_length(s, ssch, g, NULL);
        const uint16_t *sfb_offset = get_sfb_offset(transf_length_g);
        int max_sfb = get_max_sfb(s, ssch, g);

        av_assert2(max_sfb < get_sfb_size(transf_length_g));

        for (int sfb = 0; sfb < max_sfb; sfb++) {
            for (int w = 0; w < ssch->scp.num_win_in_group[g]; w++) {
                for (int l = sfb_offset[sfb]; l < sfb_offset[sfb+1]; l++)
                    spec_reord[win_offset[win+w] + l] = scaled_spec[k++];
            }
        }
        win += ssch->scp.num_win_in_group[g];
    }
}

static void scale_spec(AC4DecodeContext *s, int ch)
{
    Substream *ss = &s->substream;
    SubstreamChannel *ssch = &ss->ssch[ch];
    const float *quant_lut = s->quant_lut;

    memset(ssch->scaled_spec, 0, sizeof(ssch->scaled_spec));

    av_assert2(FF_ARRAY_ELEMS(ssch->scaled_spec) >= s->frame_len_base);
    for (int k = 0; k < s->frame_len_base; k++) {
        int x = ssch->quant_spec[k];
        int sfb = ssch->offset2sfb[k];
        int g = ssch->offset2g[k];

        av_assert2(sfb < FF_ARRAY_ELEMS(ssch->sf_gain[g]));
        av_assert2(g < FF_ARRAY_ELEMS(ssch->sf_gain));
        av_assert2(FFABS(x) < FF_ARRAY_ELEMS(s->quant_lut));
        ssch->scaled_spec[k] = ssch->sf_gain[g][sfb] * copysignf(quant_lut[FFABS(x)], x);
    }
}

static int two_channel_processing(AC4DecodeContext *s, Substream *ss,
                                  SubstreamChannel *ssch0,
                                  SubstreamChannel *ssch1)
{
    int max_sfb_prev;
    float sap_gain;

    memset(ss->alpha_q, 0, sizeof(ss->alpha_q));

    max_sfb_prev = get_max_sfb(s, ssch0, 0);
    for (int g = 0; g < ssch0->scp.num_window_groups; g++) {
        int max_sfb_g = get_max_sfb(s, ssch0, g);

        for (int sfb = 0; sfb < max_sfb_g; sfb++) {
            float m[2][2];

            if (ssch0->sap_mode == 0 ||
                (ssch0->sap_mode == 1 && ssch0->ms_used[g][sfb] == 0)) {
                m[0][0] = m[1][1] = 1;
                m[0][1] = m[1][0] = 0;
            } else if (ssch0->sap_mode == 2 ||
                       ((ssch0->sap_mode == 1 && ssch0->ms_used[g][sfb] == 1))) {
                m[0][0] =
                m[0][1] =
                m[1][0] =  1;
                m[1][1] = -1;
            } else { // sap_mode == 3
                if (ssch0->sap_coeff_used[g][sfb]) { // setup alpha_q[g][sfb]
                    if (sfb & 1) {
                        ss->alpha_q[g][sfb] = ss->alpha_q[g][sfb-1];
                    } else {
                        float delta = ssch0->dpcm_alpha_q[g][sfb] - 60;
                        int code_delta;

                        if ((g == 0) || (max_sfb_g != max_sfb_prev)) {
                            code_delta = 0;
                        } else {
                            code_delta = ssch0->delta_code_time;
                        }

                        if (code_delta) {
                            ss->alpha_q[g][sfb] = ss->alpha_q[g-1][sfb] + delta;
                        } else if (sfb == 0) {
                            ss->alpha_q[g][sfb] = delta;
                        } else {
                            ss->alpha_q[g][sfb] = ss->alpha_q[g][sfb-2] + delta;
                        }
                    }
                    // inverse quantize alpha_q[g][sfb]
                    sap_gain = ss->alpha_q[g][sfb] * 0.1f;
                    m[0][0] =  1 + sap_gain;
                    m[0][1] =  1;
                    m[1][0] =  1 - sap_gain;
                    m[1][1] = -1;
                } else {
                    m[0][0] = 1;
                    m[0][1] = 0;
                    m[1][0] = 0;
                    m[1][1] = 1;
                }
            }

            memcpy(ss->matrix_stereo[g][sfb], m, sizeof(m));
        }

        max_sfb_prev = max_sfb_g;
    }

    for (int k = 0; k < s->frame_len_base; k++) {
        int sfb = ssch0->offset2sfb[k];
        int g = ssch0->offset2g[k];
        float a = ss->matrix_stereo[g][sfb][0][0];
        float b = ss->matrix_stereo[g][sfb][0][1];
        float c = ss->matrix_stereo[g][sfb][1][0];
        float d = ss->matrix_stereo[g][sfb][1][1];
        float i0 = ssch0->scaled_spec[k];
        float i1 = ssch1->scaled_spec[k];
        float o0, o1;

        o0 = i0 * a + i1 * b;
        o1 = i0 * c + i1 * d;

        ssch0->scaled_spec[k] = o0;
        ssch1->scaled_spec[k] = o1;
    }

    return 0;
}

static int stereo_processing(AC4DecodeContext *s, Substream *ss)
{
    if (ss->mdct_stereo_proc[0])
        two_channel_processing(s, ss, &ss->ssch[0], &ss->ssch[1]);

    return 0;
}

static int m5channel_processing(AC4DecodeContext *s, Substream *ss)
{
    switch (ss->codec_mode) {
    case CM_SIMPLE:
    case CM_ASPX:
        switch (ss->coding_config) {
        case 0:
            if (ss->mdct_stereo_proc[0])
                two_channel_processing(s, ss, &ss->ssch[0], &ss->ssch[1]);
            if (ss->mdct_stereo_proc[1])
                two_channel_processing(s, ss, &ss->ssch[2], &ss->ssch[3]);
            break;
        }
        break;
    case CM_ASPX_ACPL_1:
    case CM_ASPX_ACPL_2:
    case CM_ASPX_ACPL_3:
        switch (ss->coding_config) {
        case 0:
            if (ss->mdct_stereo_proc[0])
                two_channel_processing(s, ss, &ss->ssch[0], &ss->ssch[1]);
            break;
        }
        break;
    }

    return 0;
}

static void qmf_analysis(AC4DecodeContext *s, SubstreamChannel *ssch)
{
    float *qana_filt = ssch->qana_filt;
    float *pcm = ssch->pcm;
    LOCAL_ALIGNED_32(float, u, [MAX_QMF_BANDS*2]);
    LOCAL_ALIGNED_32(float, z, [MAX_QMF_BANDS*10]);

    for (int ts = 0; ts < s->num_qmf_timeslots; ts++) {
        /* shift time-domain input samples by MAX_QMF_BANDS */
        memmove(qana_filt + MAX_QMF_BANDS, qana_filt, sizeof(*qana_filt) * (MAX_QMF_BANDS*10 - MAX_QMF_BANDS));

        /* feed new audio samples */
        for (int sb = MAX_QMF_BANDS-1; sb >= 0; sb--)
            qana_filt[sb] = pcm[ts * MAX_QMF_BANDS + MAX_QMF_BANDS-1 - sb] / 32768.f;

        /* multiply input samples by window coefficients */
        s->fdsp->vector_fmul(z, qana_filt, qwin, MAX_QMF_BANDS*10);

        /* sum the samples to create vector u */
        for (int n = 0; n < MAX_QMF_BANDS*2; n++) {
            u[n] = z[n];
            for (int k = 1; k < 5; k++)
                u[n] += z[n + k * MAX_QMF_BANDS*2];
        }

        /* compute MAX_QMF_BANDS new subband samples */
        for (int sb = 0; sb < MAX_QMF_BANDS; sb++) {
            const float *cos_atab = s->cos_atab[sb];
            const float *sin_atab = s->sin_atab[sb];

            ssch->Q[0][ts][sb] = s->fdsp->scalarproduct_float(u, cos_atab, MAX_QMF_BANDS*2);
            ssch->Q[1][ts][sb] = s->fdsp->scalarproduct_float(u, sin_atab, MAX_QMF_BANDS*2);
        }
    }
}

static void qmf_synthesis(AC4DecodeContext *s, SubstreamChannel *ssch, float *pcm)
{
    float *qsyn_filt = ssch->qsyn_filt;
    LOCAL_ALIGNED_32(float, g, [MAX_QMF_BANDS*10]);
    LOCAL_ALIGNED_32(float, w, [MAX_QMF_BANDS*10]);

    for (int ts = 0; ts < s->num_qmf_timeslots; ts++) {
        /* shift samples by MAX_QMF_BANDS*2 */
        memmove(qsyn_filt + MAX_QMF_BANDS*2, qsyn_filt, sizeof(*qsyn_filt) * (MAX_QMF_BANDS*20 - MAX_QMF_BANDS*2));

        for (int n = 0; n < MAX_QMF_BANDS*2; n++) {
            const float *cos_stab = s->cos_stab[n];
            const float *sin_stab = s->sin_stab[n];

            qsyn_filt[n] = s->fdsp->scalarproduct_float(ssch->Q[0][ts], cos_stab, MAX_QMF_BANDS) -
                           s->fdsp->scalarproduct_float(ssch->Q[1][ts], sin_stab, MAX_QMF_BANDS);
        }

        for (int n = 0; n < 5; n++) {
            memcpy(g + MAX_QMF_BANDS*2 * n, qsyn_filt + 256 * n, MAX_QMF_BANDS * sizeof(float));
            memcpy(g + MAX_QMF_BANDS*2 * n + MAX_QMF_BANDS, qsyn_filt + 256 * n + 192, MAX_QMF_BANDS * sizeof(float));
        }
        /* multiply by window coefficients */
        s->fdsp->vector_fmul(w, g, qwin, MAX_QMF_BANDS*10);

        /* compute MAX_QMF_BANDS new time-domain output samples */
        for (int sb = 0; sb < MAX_QMF_BANDS; sb++) {
            float temp = 0;

            for (int n = 0; n < 10; n++)
                temp += w[MAX_QMF_BANDS*n + sb];
            pcm[ts*MAX_QMF_BANDS + sb] = temp;
        }
    }
}

static void spectral_synthesis(AC4DecodeContext *s, SubstreamChannel *ssch)
{
    LOCAL_ALIGNED_32(float, swinl, [2048]);
    LOCAL_ALIGNED_32(float, swinr, [2048]);
    const int *win_offset = ssch->win_offset;
    const int N_full = s->frame_len_base;
    float *imdct_out = ssch->imdct_out;
    float *overlap = ssch->overlap;
    float *pcm = ssch->pcm;
    int win = 0;

    for (int g = 0; g < ssch->scp.num_window_groups; g++) {
        int midx = s->frame_len_base_idx, idx;
        const int N = get_transf_length(s, ssch, g, &idx);
        const int N_prev = !ssch->N_prev ? N_full : ssch->N_prev;
        const int N_w = FFMIN(N, N_prev);
        const float *winl = s->kbd_window[midx][idx];
        const float *winr = s->kbd_window[midx][ssch->idx_prev];
        const float *winf = (N_w == N) ? winl : winr;
        const int wnskip_prev = (N_prev - N_w) >> 1;
        const int wnskip = (N - N_w) >> 1;
        const int nskip_prev = (N_full - N_prev) >> 1;
        const int nskip = (N_full - N) >> 1;

        av_assert2(wnskip + wnskip + N_w == N);
        memset(swinl, 0, wnskip * sizeof(float));
        memcpy(swinl + wnskip, winf, N_w * sizeof(float));
        for (int n = 0; n < wnskip; n++)
            swinl[wnskip + N_w + n] = 1.f;

        av_assert2(wnskip_prev + wnskip_prev + N_w == N_prev);
        memset(swinr, 0, wnskip_prev * sizeof(float));
        memcpy(swinr + wnskip_prev, winf, N_w * sizeof(float));
        for (int n = 0; n < wnskip_prev; n++)
            swinr[wnskip_prev + N_w + n] = 1.f;

        for (int w = 0; w < ssch->scp.num_win_in_group[g]; w++) {
            s->tx_fn[midx][idx](s->tx_ctx[midx][idx], imdct_out,
                                ssch->spec_reord + win_offset[win + w],
                                sizeof(float));

            s->fdsp->vector_fmul(imdct_out, imdct_out, swinl, N);
            if (!(N_prev & 15))
                s->fdsp->vector_fmul_reverse(overlap + nskip_prev,
                                             overlap + nskip_prev, swinr, N_prev);
            else
                ff_vector_fmul_reverse_c(overlap + nskip_prev,
                                         overlap + nskip_prev, swinr, N_prev);

            for (int n = 0; n < N; n++)
                overlap[nskip + n] += imdct_out[n];

            memcpy(pcm + win_offset[win + w], overlap, N * sizeof(float));
            memmove(overlap, overlap + N, nskip * sizeof(float));
            memcpy(overlap + nskip, imdct_out + N, N * sizeof(float));
        }

        ssch->N_prev = N;
        ssch->idx_prev = idx;

        win += ssch->scp.num_win_in_group[g];
    }
}

#define MAX_ORDER 5

static int polyfit(int order,
                   int countOfElements,
                   const float *const dependentValues,
                   const float *const independentValues,
                   float *coefficients)
{
    float B[MAX_ORDER + 1] = {0.0f};
    float P[((MAX_ORDER + 1) * 2 ) + 1] = {0.0f};
    float A[(MAX_ORDER + 1) * 2 * (MAX_ORDER + 1)] = {0.0f};
    float x, y, powx;
    int ii, jj, kk;

    // This method requires that the countOfElements >
    // (order+1)
    if (countOfElements <= order)
        return -1;

    // This method has imposed an arbitrary bound of
    // order <= MAX_ORDER.  Increase MAX_ORDER if necessary.
    if (order > MAX_ORDER)
        return -1;

    // Identify the column vector
    for (ii = 0; ii < countOfElements; ii++) {
        x    = dependentValues[ii];
        y    = independentValues[ii];
        powx = 1;

        for (jj = 0; jj < (order + 1); jj++) {
            B[jj] = B[jj] + (y * powx);
            powx  = powx * x;
        }
    }

    // Initialize the PowX array
    P[0] = countOfElements;
    // Compute the sum of the Powers of X
    for (ii = 0; ii < countOfElements; ii++) {
        x    = dependentValues[ii];
        powx = dependentValues[ii];

        for (jj = 1; jj < (2 * (order + 1)) + 1; jj++) {
            P[jj] = P[jj] + powx;
            powx  = powx * x;
        }
    }

    // Initialize the reduction matrix
    //
    for (ii = 0; ii < (order + 1); ii++) {
        for (jj = 0; jj < (order + 1); jj++) {
            A[(ii * (2 * (order + 1))) + jj] = P[ii+jj];
        }
        A[(ii*(2 * (order + 1))) + (ii + (order + 1))] = 1;
    }

    // Move the Identity matrix portion of the redux matrix
    // to the left side (find the inverse of the left side
    // of the redux matrix
    for (ii = 0; ii < (order + 1); ii++) {
        x = A[(ii * (2 * (order + 1))) + ii];
        if (x != 0) {
            for (kk = 0; kk < (2 * (order + 1)); kk++) {
                A[(ii * (2 * (order + 1))) + kk] =
                    A[(ii * (2 * (order + 1))) + kk] / x;
            }

            for (jj = 0; jj < (order + 1); jj++) {
                if ((jj - ii) != 0) {
                    y = A[(jj * (2 * (order + 1))) + ii];
                    for (kk = 0; kk < (2 * (order + 1)); kk++) {
                        A[(jj * (2 * (order + 1))) + kk] =
                            A[(jj * (2 * (order + 1))) + kk] -
                            y * A[(ii * (2 * (order + 1))) + kk];
                    }
                }
            }
        } else { // Cannot work with singular matrices
            return 0;
        }
    }

    // Calculate and Identify the coefficients
    for (ii = 0; ii < order + 1; ii++) {
        for (jj = 0; jj < order + 1; jj++) {
            x = 0;
            for (kk = 0; kk < (order + 1); kk++) {
                x = x + (A[(ii * (2 * (order + 1))) + (kk + (order + 1))] *
                         B[kk]);
            }
            coefficients[ii] = x;
        }
    }

    return 0;
}

static int get_qsignal_scale_factors(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    int sbg_idx_high2low[24] = {0};
    int sbg_idx_low2high[24] = {0};
    int delta;

    for (int sbg = 0, sbg_low = 0; sbg < ssch->num_sbg_sig_highres; sbg++) {
        if (ssch->sbg_sig_lowres[sbg_low+1] == ssch->sbg_sig_highres[sbg]) {
            sbg_low++;
            sbg_idx_low2high[sbg_low] = sbg;
        }
        sbg_idx_high2low[sbg] = sbg_low;
    }

    delta = ((ch_id & 1) && (ssch->aspx_balance == 1)) + 1;

    memcpy(ssch->qscf_sig_sbg_prev, ssch->qscf_sig_sbg, sizeof(ssch->qscf_sig_sbg));
    memset(ssch->qscf_sig_sbg, 0, sizeof(ssch->qscf_sig_sbg));

    ssch->num_atsg_sig_prev = FFMAX(1, ssch->num_atsg_sig_prev);

    /* Loop over Envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over scale factor subband groups */
        for (int sbg = 0; sbg < ssch->num_sbg_sig[ssch->atsg_freqres[atsg]]; sbg++) {
            if (atsg == 0) {
                ssch->atsg_freqres_prev[atsg] = ssch->atsg_freqres[ssch->num_atsg_sig_prev - 1];
                ssch->qscf_prev[atsg][sbg] = ssch->qscf_sig_sbg_prev[ssch->num_atsg_sig_prev - 1][sbg];
            } else {
                ssch->atsg_freqres_prev[atsg] = ssch->atsg_freqres[atsg-1];
                ssch->qscf_prev[atsg][sbg] = ssch->qscf_sig_sbg[atsg-1][sbg];
            }
            if (ssch->aspx_sig_delta_dir[atsg] == 0) { /* FREQ */
                ssch->qscf_sig_sbg[atsg][sbg] = 0;
                for (int i = 0; i <= sbg; i++)
                    ssch->qscf_sig_sbg[atsg][sbg] += delta * ssch->aspx_data[0][atsg][i];
            } else { /* TIME */
                if (ssch->atsg_freqres[atsg] == ssch->atsg_freqres_prev[atsg]) {
                    ssch->qscf_sig_sbg[atsg][sbg]  = ssch->qscf_prev[atsg][sbg];
                    ssch->qscf_sig_sbg[atsg][sbg] += delta * ssch->aspx_data[0][atsg][sbg];
                } else if ((ssch->atsg_freqres[atsg] == 0) && (ssch->atsg_freqres_prev[atsg] == 1)) {
                    ssch->qscf_sig_sbg[atsg][sbg]  = ssch->qscf_prev[atsg][sbg_idx_low2high[sbg]];
                    ssch->qscf_sig_sbg[atsg][sbg] += delta * ssch->aspx_data[0][atsg][sbg];
                } else if ((ssch->atsg_freqres[atsg] == 1) && (ssch->atsg_freqres_prev[atsg] == 0)) {
                    ssch->qscf_sig_sbg[atsg][sbg]  = ssch->qscf_prev[atsg][sbg_idx_high2low[sbg]];
                    ssch->qscf_sig_sbg[atsg][sbg] += delta * ssch->aspx_data[0][atsg][sbg];
                }
            }

            av_assert2(ssch->qscf_sig_sbg[atsg][sbg] >= -63);
            av_assert2(ssch->qscf_sig_sbg[atsg][sbg] <=  63);
        }
    }

    return 0;
}

static int get_qnoise_scale_factors(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    int delta;

    delta = ((ch_id & 1) && (ssch->aspx_balance == 1)) + 1;

    memcpy(ssch->qscf_noise_prev, ssch->qscf_noise_sbg, sizeof(ssch->qscf_noise_sbg));
    memset(ssch->qscf_noise_sbg, 0, sizeof(ssch->qscf_noise_sbg));

    ssch->num_atsg_noise_prev = FFMAX(1, ssch->num_atsg_noise_prev);

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_noise; atsg++) {
        /* Loop over noise subband groups */
        for (int sbg = 0; sbg < ssch->num_sbg_noise; sbg++) {
            if (ssch->aspx_noise_delta_dir[atsg] == 0) { /* FREQ */
                for (int i = 0; i <= sbg; i++)
                    ssch->qscf_noise_sbg[atsg][sbg] += delta * ssch->aspx_data[1][atsg][i];
            } else { /* TIME */
                if (atsg == 0) {
                    ssch->qscf_noise_sbg[atsg][sbg]  = ssch->qscf_noise_prev[ssch->num_atsg_noise_prev-1][sbg];
                    ssch->qscf_noise_sbg[atsg][sbg] += delta * ssch->aspx_data[1][atsg][sbg];
                } else {
                    ssch->qscf_noise_sbg[atsg][sbg]  = ssch->qscf_noise_sbg[atsg-1][sbg];
                    ssch->qscf_noise_sbg[atsg][sbg] += delta * ssch->aspx_data[1][atsg][sbg];
                }
            }

            av_assert2(ssch->qscf_noise_sbg[atsg][sbg] >= -63);
            av_assert2(ssch->qscf_noise_sbg[atsg][sbg] <=  63);
        }
    }

    return 0;
}

static void prepare_channel(AC4DecodeContext *s, int ch)
{
    Substream *ss = &s->substream;
    SubstreamChannel *ssch = &ss->ssch[ch];

    spectral_reordering(s, ssch);
    spectral_synthesis(s, ssch);

    qmf_analysis(s, ssch);
}

static void aspx_processing(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    const int num_qmf_timeslots = s->num_qmf_timeslots;
    const int ts_offset_hfgen = s->ts_offset_hfgen;
    SubstreamChannel *ssch = &ss->ssch[ch_id];

    memcpy(ssch->Q_low_prev, ssch->Q_low, sizeof(ssch->Q_low));
    memset(ssch->Q_low, 0, sizeof(ssch->Q_low));

    for (int ts = 0; ts < ts_offset_hfgen; ts++) {
        for (int sb = 0; sb < MAX_QMF_BANDS; sb++) {
            if (sb < ssch->sbx) {
                ssch->Q_low[0][ts][sb] = ssch->Q_prev[0][ts + num_qmf_timeslots - ts_offset_hfgen][sb];
                ssch->Q_low[1][ts][sb] = ssch->Q_prev[1][ts + num_qmf_timeslots - ts_offset_hfgen][sb];
            }
        }
    }

    for (int ts = ts_offset_hfgen; ts < num_qmf_timeslots + ts_offset_hfgen; ts++) {
        for (int sb = 0; sb < MAX_QMF_BANDS; sb++) {
            if (sb < ssch->sbx) {
                ssch->Q_low[0][ts][sb] = ssch->Q[0][ts - ts_offset_hfgen][sb];
                ssch->Q_low[1][ts][sb] = ssch->Q[1][ts - ts_offset_hfgen][sb];
            }
        }
    }
}

static void mono_deq_signal_factors(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    const float a = 1 + (ssch->aspx_qmode_env == 0);
    const float b = ssch->sbx;

    memset(ssch->scf_sig_sbg, 0, sizeof(ssch->scf_sig_sbg));

    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        for (int sbg = 0; sbg < ssch->num_sbg_sig[ssch->atsg_freqres[atsg]]; sbg++)
            ssch->scf_sig_sbg[atsg][sbg] = b * powf(2.f, ssch->qscf_sig_sbg[atsg][sbg] / a);

        if (ssch->aspx_sig_delta_dir[atsg] == 0 &&
            ssch->qscf_sig_sbg[atsg][0] == 0 &&
            ssch->scf_sig_sbg[atsg][1] < 0) {
            ssch->scf_sig_sbg[atsg][0] = ssch->scf_sig_sbg[atsg][1];
        }
    }
}

static void mono_deq_noise_factors(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
#define NOISE_FLOOR_OFFSET 6
    memset(ssch->scf_noise_sbg, 0, sizeof(ssch->scf_noise_sbg));

    for (int atsg = 0; atsg < ssch->num_atsg_noise; atsg++) {
        for (int sbg = 0; sbg < ssch->num_sbg_noise; sbg++)
            ssch->scf_noise_sbg[atsg][sbg] = powf(2.f, NOISE_FLOOR_OFFSET - ssch->qscf_noise_sbg[atsg][sbg]);
    }
}

static void joint_deq_signoise_factors(AC4DecodeContext *s,
                                       Substream *ss, uint8_t ch_id[2])
{
    SubstreamChannel *ssch0 = &ss->ssch[ch_id[0]];
    SubstreamChannel *ssch1 = &ss->ssch[ch_id[1]];
#define PAN_OFFSET 12
    const float a = 1 + (ssch0->aspx_qmode_env == 0);
    const float b = ssch0->sbx;

    memset(ssch0->scf_sig_sbg, 0, sizeof(ssch0->scf_sig_sbg));
    memset(ssch0->scf_noise_sbg, 0, sizeof(ssch0->scf_noise_sbg));

    memset(ssch1->scf_sig_sbg, 0, sizeof(ssch1->scf_sig_sbg));
    memset(ssch1->scf_noise_sbg, 0, sizeof(ssch1->scf_noise_sbg));

    for (int atsg = 0; atsg < ssch0->num_atsg_sig; atsg++) {
        for (int sbg = 0; sbg < ssch0->num_sbg_sig[ssch0->atsg_freqres[atsg]]; sbg++) {
            float nom = b * powf(2.f, ssch0->qscf_sig_sbg[atsg][sbg] / a + 1.f);
            float denom_a = 1.f + powf(2.f, PAN_OFFSET - ssch1->qscf_sig_sbg[atsg][sbg] / a);
            float denom_b = 1.f + powf(2.f, ssch1->qscf_sig_sbg[atsg][sbg] / a - PAN_OFFSET);

            ssch0->scf_sig_sbg[atsg][sbg] = nom / denom_a;
            ssch1->scf_sig_sbg[atsg][sbg] = nom / denom_b;
        }
    }

    for (int atsg = 0; atsg < ssch0->num_atsg_noise; atsg++) {
        for (int sbg = 0; sbg < ssch0->num_sbg_noise; sbg++) {
            float nom = powf(2.f, NOISE_FLOOR_OFFSET - ssch0->qscf_noise_sbg[atsg][sbg] + 1.f);
            float denom_a = 1.f + powf(2.f, PAN_OFFSET - ssch1->qscf_noise_sbg[atsg][sbg]);
            float denom_b = 1.f + powf(2.f, ssch1->qscf_noise_sbg[atsg][sbg] - PAN_OFFSET);

            ssch0->scf_noise_sbg[atsg][sbg] = nom / denom_a;
            ssch1->scf_noise_sbg[atsg][sbg] = nom / denom_b;
        }
    }
}

static void preflattening(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    float mean_energy = 0;
    int polynomial_order = 3;
    int num_qmf_subbands = ssch->sbx;
    float poly_array[MAX_QMF_BANDS];
    float pow_env[MAX_QMF_BANDS];
    float slope[MAX_QMF_BANDS];
    float x[MAX_QMF_BANDS];

    if (ssch->sbx < 3)
        return;

    for (int i = 0; i < num_qmf_subbands; i++) {
        x[i] = i;
        slope[i] = 0;
    }
    /* Calculate the spectral signal envelope in dB over the current interval. */
    for (int sb = 0; sb < num_qmf_subbands; sb++) {
        float denom, nom = 0.f;

        for (int ts = ssch->atsg_sig[0] * s->num_ts_in_ats; ts < ssch->atsg_sig[ssch->num_atsg_sig] * s->num_ts_in_ats; ts++) {
            nom += powf(ssch->Q_low[0][ts][sb], 2);
            nom += powf(ssch->Q_low[1][ts][sb], 2);
        }

        denom = (ssch->atsg_sig[ssch->num_atsg_sig] - ssch->atsg_sig[0]) * s->num_ts_in_ats;
        pow_env[sb] = nom / denom;
        pow_env[sb] = 10.f * log10f(pow_env[sb] + 1.f);
        mean_energy += pow_env[sb];
    }

    mean_energy /= num_qmf_subbands;
    av_assert2(polyfit(polynomial_order, num_qmf_subbands, x, pow_env, poly_array) == 0);

    /* Transform polynomial into slope */
    for (int k = polynomial_order; k >= 0; k--) {
        for (int sb = 0; sb < num_qmf_subbands; sb++)
            slope[sb] += powf(x[sb], k) * poly_array[k];
    }

    /* Derive a gain vector from the slope */
    av_assert2(num_qmf_subbands <= FF_ARRAY_ELEMS(ssch->gain_vec));
    for (int sb = 0; sb < num_qmf_subbands; sb++)
        ssch->gain_vec[sb] = powf(10, (mean_energy - slope[sb]) / 20.f);
}

static void get_chirps(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    memcpy(ssch->chirp_arr_prev, ssch->chirp_arr, sizeof(ssch->chirp_arr));

    for (int sbg = 0; sbg < ssch->num_sbg_noise; sbg++) {
        float new_chirp = new_chirp_tab[ssch->aspx_tna_mode[sbg]][ssch->aspx_tna_mode_prev[sbg]];

        av_assert2(ssch->aspx_tna_mode[sbg] <= 3);
        av_assert2(ssch->aspx_tna_mode_prev[sbg] <= 3);
        if (new_chirp < ssch->chirp_arr_prev[sbg]) {
            new_chirp = 0.75000f * new_chirp + 0.25000f * ssch->chirp_arr_prev[sbg];
        } else {
            new_chirp = 0.90625f * new_chirp + 0.09375f * ssch->chirp_arr_prev[sbg];
        }

        if (new_chirp < 0.015625f) {
            ssch->chirp_arr[sbg] = 0.f;
        } else {
            ssch->chirp_arr[sbg] = new_chirp;
        }
    }
}

static void fcomplex_mul(float *r, float *i, float x, float yi, float u, float vi)
{
    *r = x*u - yi*vi;
    *i = x*vi + u*yi;
}

static void get_covariance(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    const int ts_offset_hfadj = 4;
    int num_ts_ext;

    /* Create an additional delay of ts_offset_hfadj QMF time slots */
    for (int sb = 0; sb < ssch->sba; sb++) {
        int ts_offset_prev = s->num_qmf_timeslots - ts_offset_hfadj;

        av_assert2(ts_offset_hfadj <= FF_ARRAY_ELEMS(ssch->Q_low_ext[0]));
        av_assert2(ts_offset_prev + ts_offset_hfadj <= FF_ARRAY_ELEMS(ssch->Q_low_prev[0]));
        for (int ts = 0; ts < ts_offset_hfadj; ts++) {
            ssch->Q_low_ext[0][ts][sb] = ssch->Q_low_prev[0][ts + ts_offset_prev][sb];
            ssch->Q_low_ext[1][ts][sb] = ssch->Q_low_prev[1][ts + ts_offset_prev][sb];
        }

        av_assert2(s->num_qmf_timeslots + s->ts_offset_hfgen + ts_offset_hfadj <= FF_ARRAY_ELEMS(ssch->Q_low_ext[0]));
        av_assert2(s->num_qmf_timeslots + s->ts_offset_hfgen <= FF_ARRAY_ELEMS(ssch->Q_low[0]));
        for (int ts = 0; ts < s->num_qmf_timeslots + s->ts_offset_hfgen; ts++) {
            ssch->Q_low_ext[0][ts + ts_offset_hfadj][sb] = ssch->Q_low[0][ts][sb];
            ssch->Q_low_ext[1][ts + ts_offset_hfadj][sb] = ssch->Q_low[1][ts][sb];
        }
    }

    num_ts_ext = s->num_qmf_timeslots + s->ts_offset_hfgen + ts_offset_hfadj;
    /* Loop over QMF subbands */
    for (int sb = 0; sb < ssch->sba; sb++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 1; j < 3; j++) {
                ssch->cov[sb][i][j][0] = 0;
                ssch->cov[sb][i][j][1] = 0;
                /* Loop over QMF time slots */
                for (int ts = ts_offset_hfadj; ts < num_ts_ext; ts += 2) {
                    float re, im;

                    fcomplex_mul(&re, &im,
                                 ssch->Q_low_ext[0][ts - 2*i][sb],  ssch->Q_low_ext[1][ts - 2*i][sb],
                                 ssch->Q_low_ext[0][ts - 2*j][sb], -ssch->Q_low_ext[1][ts - 2*j][sb]);

                    ssch->cov[sb][i][j][0] += re;
                    ssch->cov[sb][i][j][1] += im;
                }
            }
        }
    }
}

static float sqr(float a, float b)
{
    return a * a + b * b;
}

static void fcomplex_div(float *r, float *i, const float x, const float yi,
                         const float u, const float vi)
{
    const float div = sqr(u, vi);
    const float re = x*u + yi*vi;
    const float im = x*vi - u*yi;
    *r = re / div;
    *i = im / div;
}

static void get_alphas(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    float EPSILON_INV = ldexpf(1.f, -20);

    for (int sb = 0; sb < ssch->sba; sb++) {
        float denom[2];

        fcomplex_mul(&denom[0], &denom[1], ssch->cov[sb][2][2][0], ssch->cov[sb][2][2][1], ssch->cov[sb][1][1][0], ssch->cov[sb][1][1][1]);
        denom[0] -= sqr(ssch->cov[sb][1][2][0], ssch->cov[sb][1][2][1]) / (1.f+EPSILON_INV);
        if (sqr(denom[0], denom[1]) <= 1e-6f) {
            ssch->alpha1[sb][0] = 0.f;
            ssch->alpha1[sb][1] = 0.f;
        } else {
            float a, bi, c, di;

            fcomplex_mul(&a, &bi, ssch->cov[sb][0][1][0], ssch->cov[sb][0][1][1],
                                  ssch->cov[sb][1][2][0], ssch->cov[sb][1][2][1]);
            fcomplex_mul(&c, &di, ssch->cov[sb][0][2][0], ssch->cov[sb][0][2][1],
                                  ssch->cov[sb][1][1][0], ssch->cov[sb][1][1][1]);
            ssch->alpha1[sb][0] = a - c;
            ssch->alpha1[sb][1] = bi - di;
            fcomplex_div(&ssch->alpha1[sb][0], &ssch->alpha1[sb][1], ssch->alpha1[sb][0], ssch->alpha1[sb][1], denom[0], denom[1]);
        }

        if (sqr(ssch->cov[sb][1][1][0], ssch->cov[sb][1][1][1]) <= 1e-6f) {
            ssch->alpha0[sb][0] = 0.f;
            ssch->alpha0[sb][1] = 0.f;
        } else {
            float a, bi;

            fcomplex_mul(&a, &bi, ssch->alpha1[sb][0],     ssch->alpha1[sb][1],
                                  ssch->cov[sb][1][2][0], -ssch->cov[sb][1][2][1]);

            ssch->alpha0[sb][0]  = -ssch->cov[sb][0][1][0] + a;
            ssch->alpha0[sb][1]  = -ssch->cov[sb][0][1][1] + bi;
            fcomplex_div(&ssch->alpha0[sb][0], &ssch->alpha0[sb][1], ssch->alpha0[sb][0], ssch->alpha0[sb][1], ssch->cov[sb][1][1][0], ssch->cov[sb][1][1][1]);
        }

        if (sqr(ssch->alpha0[sb][0], ssch->alpha0[sb][1]) >= 16.f ||
            sqr(ssch->alpha1[sb][0], ssch->alpha1[sb][1]) >= 16.f) {
            ssch->alpha0[sb][0] = ssch->alpha0[sb][1] = 0.f;
            ssch->alpha1[sb][0] = ssch->alpha1[sb][1] = 0.f;
        }
    }
}

static void create_high_signal(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    int ts_offset_hfadj = 4;
    float m;

    /* Loop over QMF time slots */
    for (int ts = ssch->atsg_sig[0] * s->num_ts_in_ats;
         ts < ssch->atsg_sig[ssch->num_atsg_sig] * s->num_ts_in_ats; ts++) {
        int sum_sb_patches = 0;
        int g = 0;
        /* Loop over number of patches */
        for (int i = 0; i < ssch->num_sbg_patches; i++) {
            /* Loop over number of subbands per patch */
            for (int sb = 0; sb < ssch->sbg_patch_num_sb[i]; sb++) {
                float cplx[2] = {0};
                /* Map to High QMF Subband */
                int n, p;
                int sb_high = ssch->sbx + sum_sb_patches + sb;

                /* Map to current noise envelope */
                if (ssch->sbg_noise[g+1] == sb_high)
                    g++;

                n = ts + ts_offset_hfadj;
                /* Current low QMF Subband */
                p = ssch->sbg_patch_start_sb[i] + sb;

                av_assert2(ts < FF_ARRAY_ELEMS(ssch->Q_high[0]));
                av_assert2(sb_high < FF_ARRAY_ELEMS(ssch->Q_high[0][0]));

                av_assert2(n < FF_ARRAY_ELEMS(ssch->Q_low_ext[0]));
                av_assert2(p < FF_ARRAY_ELEMS(ssch->Q_low_ext[0][0]));

                ssch->Q_high[0][ts][sb_high] = ssch->Q_low_ext[0][n][p];
                ssch->Q_high[1][ts][sb_high] = ssch->Q_low_ext[1][n][p];

                fcomplex_mul(&cplx[0], &cplx[1], ssch->alpha0[p][0], ssch->alpha0[p][1], ssch->Q_low_ext[0][n-2][p], ssch->Q_low_ext[1][n-2][p]);
                ssch->Q_high[0][ts][sb_high] += cplx[0] * ssch->chirp_arr[g];
                ssch->Q_high[1][ts][sb_high] += cplx[1] * ssch->chirp_arr[g];

                fcomplex_mul(&cplx[0], &cplx[1], ssch->alpha1[p][0], ssch->alpha1[p][1], ssch->Q_low_ext[0][n-4][p], ssch->Q_low_ext[1][n-4][p]);
                m = powf(ssch->chirp_arr[g], 2);
                ssch->Q_high[0][ts][sb_high] += cplx[0] * m;
                ssch->Q_high[1][ts][sb_high] += cplx[1] * m;

                if (ss->aspx_preflat) {
                    m = 1.f / ssch->gain_vec[p];
                    ssch->Q_high[0][ts][sb_high] *= m;
                    ssch->Q_high[1][ts][sb_high] *= m;
                }
            }
            sum_sb_patches += ssch->sbg_patch_num_sb[i];
        }
    }
}

static void estimate_spectral_envelopes(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    const int ts_offset_hfadj = 4;

    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over QMF subbands in A-SPX range */
        for (int sb = 0, sbg = 0; sb < ssch->num_sb_aspx; sb++) {
            float est_sig = 0.f;
            int tsa, tsz;

            av_assert2(sbg+1 < FF_ARRAY_ELEMS(ssch->sbg_sig[0]));
            /* Update current subband group */
            if (sb == ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg+1])
                sbg++;

            tsa = ssch->atsg_sig[atsg]*s->num_ts_in_ats + ts_offset_hfadj;
            tsz = ssch->atsg_sig[atsg+1]*s->num_ts_in_ats + ts_offset_hfadj;
            for (int ts = tsa; ts < tsz; ts++) {
                if (ss->aspx_interpolation == 0) {
                    for (int j = ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg]; j < ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg+1]; j++) {
                        est_sig += sqr(ssch->Q_high[0][ts][j], ssch->Q_high[1][ts][j]);
                    }
                } else {
                    est_sig += sqr(ssch->Q_high[0][ts][sb+ssch->sbx], ssch->Q_high[1][ts][sb+ssch->sbx]);
                }
            }

            if (ss->aspx_interpolation == 0) {
                est_sig /= ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg+1] - ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg];
                est_sig /= ssch->atsg_sig[atsg+1] - ssch->atsg_sig[atsg];
            } else {
                est_sig /= ssch->atsg_sig[atsg+1] - ssch->atsg_sig[atsg];
            }
            ssch->est_sig_sb[atsg][sb] = est_sig;
        }
    }
}

static void map_signoise(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    int atsg_noise = 0;

    memset(ssch->scf_noise_sb, 0, sizeof(ssch->scf_noise_sb));
    memset(ssch->scf_sig_sb, 0, sizeof(ssch->scf_sig_sb));

    /* Loop over Signal Envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Map Signal Envelopes from subband groups to QMF subbands */
        for (int sbg = 0; sbg < ssch->num_sbg_sig[ssch->atsg_freqres[atsg]]; sbg++) {
            for (int sb = ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg]-ssch->sbx; sb < ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg+1]-ssch->sbx; sb++)
                ssch->scf_sig_sb[atsg][sb] = ssch->scf_sig_sbg[atsg][sbg];
        }

        if (ssch->atsg_sig[atsg] == ssch->atsg_noise[atsg_noise + 1])
            atsg_noise++;

        av_assert2(atsg_noise < FF_ARRAY_ELEMS(ssch->scf_noise_sbg));
        /* Map Noise Floors from subband groups to QMF subbands, and to signal envelopes */
        for (int sbg = 0; sbg < ssch->num_sbg_noise; sbg++) {
            for (int sb = ssch->sbg_noise[sbg] - ssch->sbx; sb < ssch->sbg_noise[sbg + 1] - ssch->sbx; sb++)
                ssch->scf_noise_sb[atsg][sb] = ssch->scf_noise_sbg[atsg_noise][sbg];
        }
    }
}

static void add_sinusoids(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    float EPSILON = 1.0f;
    float LIM_GAIN = 1.41254f;
    float EPSILON0 = powf(10.f, -12.f);
    float MAX_SIG_GAIN = powf(10.f, 5.f);
    float MAX_BOOST_FACT = 1.584893192f;
    int p_sine_at_end;

    if (ssch->aspx_tsg_ptr_prev == ssch->num_atsg_sig_prev)
        p_sine_at_end = 0;
    else
        p_sine_at_end = -1;

    memcpy(ssch->sine_idx_sb_prev, ssch->sine_idx_sb, sizeof(ssch->sine_idx_sb));
    memset(ssch->sine_idx_sb, 0, sizeof(ssch->sine_idx_sb));

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over high resolution signal envelope subband groups */
        for (int sbg = 0; sbg < ssch->num_sbg_sig_highres; sbg++) {
            int sba = ssch->sbg_sig_highres[sbg] - ssch->sbx;
            int sbz = ssch->sbg_sig_highres[sbg+1] - ssch->sbx;
            int sb_mid = (int)0.5f*(sbz + sba);
            /* Map sinusoid markers to QMF subbands */
            for (int sb = ssch->sbg_sig_highres[sbg]-ssch->sbx; sb < ssch->sbg_sig_highres[sbg+1]-ssch->sbx; sb++) {
                if ((sb == sb_mid) && ((atsg >= ssch->aspx_tsg_ptr) || (p_sine_at_end == 0)
                                       || ssch->sine_idx_sb_prev[ssch->num_atsg_sig_prev-1][sb])) {
                    ssch->sine_idx_sb[atsg][sb] = ssch->aspx_add_harmonic[sbg];
                }
            }
        }
    }

    memset(ssch->sine_area_sb, 0, sizeof(ssch->sine_area_sb));

    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over subband groups */
        for (int sbg = 0; sbg < ssch->num_sbg_sig[ssch->atsg_freqres[atsg]]; sbg++) {
            int sine_present = 0;
            /* Additional sinusoid present in SF band? */
            for (int sb = ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg]-ssch->sbx; sb < ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg+1]-ssch->sbx; sb++) {
                if (ssch->sine_idx_sb[atsg][sb] == 1)
                    sine_present = 1;
            }

            /* Mark all subbands in current subband group accordingly */
            for (int sb = ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg]-ssch->sbx; sb < ssch->sbg_sig[ssch->atsg_freqres[atsg]][sbg+1]-ssch->sbx; sb++) {
                ssch->sine_area_sb[atsg][sb] = sine_present;
            }
        }
    }

    memset(ssch->sine_lev_sb, 0, sizeof(ssch->sine_lev_sb));
    memset(ssch->noise_lev_sb, 0, sizeof(ssch->noise_lev_sb));

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over QMF subbands in A-SPX range */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            const float sig_noise_fact = ssch->scf_sig_sb[atsg][sb] / (1.f + ssch->scf_noise_sb[atsg][sb]);

            ssch->sine_lev_sb[atsg][sb]  = sqrtf(ssch->sine_idx_sb[atsg][sb] * sig_noise_fact);
            ssch->noise_lev_sb[atsg][sb] = sqrtf(ssch->scf_sig_sb[atsg][sb]);
        }
    }

    memset(ssch->sig_gain_sb, 0, sizeof(ssch->sig_gain_sb));

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over QMF subbands in A-SPX range */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            if (ssch->sine_area_sb[atsg][sb] == 0) {
                float denom = EPSILON + ssch->est_sig_sb[atsg][sb];

                if (!(atsg == ssch->aspx_tsg_ptr || atsg == p_sine_at_end))
                    denom *= 1.f + ssch->scf_noise_sb[atsg][sb];
                ssch->sig_gain_sb[atsg][sb] = sqrtf(ssch->scf_sig_sb[atsg][sb] / denom);
            } else {
                float denom = EPSILON + ssch->est_sig_sb[atsg][sb];

                denom *= 1.f + ssch->scf_noise_sb[atsg][sb];
                ssch->sig_gain_sb[atsg][sb] = sqrtf(ssch->scf_sig_sb[atsg][sb] * ssch->scf_noise_sb[atsg][sb] / denom);
            }
        }
    }

    memset(ssch->max_sig_gain_sbg, 0, sizeof(ssch->max_sig_gain_sbg));
    memset(ssch->max_sig_gain_sb, 0, sizeof(ssch->max_sig_gain_sb));

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over limiter subband groups */
        for (int sbg = 0; sbg < ssch->num_sbg_lim; sbg++) {
            float denom = EPSILON0, nom = 0.f;

            for (int sb = ssch->sbg_lim[sbg]-ssch->sbx; sb < ssch->sbg_lim[sbg+1]-1-ssch->sbx; sb++) {
                av_assert2(sb < FF_ARRAY_ELEMS(ssch->scf_sig_sb[0]));
                av_assert2(sb < FF_ARRAY_ELEMS(ssch->est_sig_sb[0]));
                av_assert2(sb >= 0);
                nom += ssch->scf_sig_sb[atsg][sb];
                denom += ssch->est_sig_sb[atsg][sb];
            }

            ssch->max_sig_gain_sbg[atsg][sbg] = sqrtf(nom/denom) * LIM_GAIN;
        }

        /* Map to QMF subbands */
        for (int sb = 0, sbg = 0; sb < ssch->num_sb_aspx; sb++) {
            if (sb == ssch->sbg_lim[sbg+1]-ssch->sbx)
                sbg++;
            ssch->max_sig_gain_sb[atsg][sb] = fminf(ssch->max_sig_gain_sbg[atsg][sbg], MAX_SIG_GAIN);
        }
    }

    memset(ssch->noise_lev_sb_lim, 0, sizeof(ssch->noise_lev_sb_lim));

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over QMF subbands */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            float tmp = ssch->noise_lev_sb[atsg][sb];

            tmp *= ssch->max_sig_gain_sb[atsg][sb] / ssch->sig_gain_sb[atsg][sb];
            ssch->noise_lev_sb_lim[atsg][sb] = fminf(ssch->noise_lev_sb[atsg][sb], tmp);
        }
    }

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over QMF subbands */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            ssch->sig_gain_sb_lim[atsg][sb] = fminf(ssch->sig_gain_sb[atsg][sb],
                                                    ssch->max_sig_gain_sb[atsg][sb]);
        }
    }

    memset(ssch->boost_fact_sbg, 0, sizeof(ssch->boost_fact_sbg));

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over limiter subband groups */
        for (int sbg = 0; sbg < ssch->num_sbg_lim; sbg++) {
            float denom = EPSILON0, nom = 0.f;

            /* Loop over subbands */
            for (int sb = ssch->sbg_lim[sbg]-ssch->sbx; sb < ssch->sbg_lim[sbg+1]-1-ssch->sbx; sb++) {
                av_assert2(sb >= 0);
                av_assert2(sb < FF_ARRAY_ELEMS(ssch->scf_sig_sb[0]));
                av_assert2(sb < FF_ARRAY_ELEMS(ssch->est_sig_sb[0]));
                av_assert2(sb < FF_ARRAY_ELEMS(ssch->sig_gain_sb_lim[0]));
                av_assert2(sb < FF_ARRAY_ELEMS(ssch->sine_lev_sb[0]));
                av_assert2(sb < FF_ARRAY_ELEMS(ssch->noise_lev_sb_lim[0]));
                av_assert2(sb < FF_ARRAY_ELEMS(ssch->boost_fact_sbg[0]));

                nom   += ssch->scf_sig_sb[atsg][sb];
                denom += ssch->est_sig_sb[atsg][sb] * powf(ssch->sig_gain_sb_lim[atsg][sb], 2);
                denom += powf(ssch->sine_lev_sb[atsg][sb], 2);
                if (!((ssch->sine_lev_sb[atsg][sb] != 0)
                      || (atsg == ssch->aspx_tsg_ptr) || (atsg == p_sine_at_end)))
                    denom += powf(ssch->noise_lev_sb_lim[atsg][sb], 2);
            }
            ssch->boost_fact_sbg[atsg][sbg] = sqrtf(nom/denom);
        }
    }

    memset(ssch->boost_fact_sb, 0, sizeof(ssch->boost_fact_sb));

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        int sbg = 0;
        /* Loop over QMF subbands */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            if (sb == ssch->sbg_lim[sbg+1]-ssch->sbx)
                sbg++;
            ssch->boost_fact_sb[atsg][sb] = fminf(ssch->boost_fact_sbg[atsg][sbg], MAX_BOOST_FACT);
        }
    }

    memset(ssch->noise_lev_sb_adj, 0, sizeof(ssch->noise_lev_sb_adj));
    memset(ssch->sine_lev_sb_adj, 0, sizeof(ssch->sine_lev_sb_adj));
    memset(ssch->sig_gain_sb_adj, 0, sizeof(ssch->sig_gain_sb_adj));

    /* Loop over envelopes */
    for (int atsg = 0; atsg < ssch->num_atsg_sig; atsg++) {
        /* Loop over QMF subbands */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            const float boost_fact           = ssch->boost_fact_sb[atsg][sb];
            ssch->sig_gain_sb_adj[atsg][sb]  = ssch->sig_gain_sb_lim[atsg][sb] * boost_fact;
            ssch->noise_lev_sb_adj[atsg][sb] = ssch->noise_lev_sb_lim[atsg][sb] * boost_fact;
            ssch->sine_lev_sb_adj[atsg][sb]  = ssch->sine_lev_sb[atsg][sb] * boost_fact;
        }
    }
}

static int sine_idx(int sb, int ts, AC4DecodeContext *s, SubstreamChannel *ssch)
{
    int index;

    if (s->first_frame) {
        index = 1;
    } else {
        index = (ssch->sine_idx_prev[ts][sb] + 1) % 4;
    }
    index += ts - ssch->atsg_sig[0];

    return index % 4;
}

static int noise_idx(int sb, int ts, Substream *ss, SubstreamChannel *ssch)
{
    int index;

    if (ssch->master_reset) {
        index = 0;
    } else {
        index = ssch->noise_idx_prev[ts][sb];
    }
    index += ssch->num_sb_aspx * (ts - ssch->atsg_sig[0]);
    index += sb + 1;

    return index % 512;
}

static int generate_noise(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];

    memset(ssch->qmf_noise, 0, sizeof(ssch->qmf_noise));

    /* Loop over QMF time slots */
    for (int ts = ssch->atsg_sig[0] * s->num_ts_in_ats, atsg = 0;
         ts < ssch->atsg_sig[ssch->num_atsg_sig] * s->num_ts_in_ats; ts++) {
        if (ts == ssch->atsg_sig[atsg+1] * s->num_ts_in_ats)
            atsg++;
        /* Loop over QMF subbands in A-SPX */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            int idx;

            av_assert2(ts < FF_ARRAY_ELEMS(ssch->noise_idx_prev));
            av_assert2(sb < FF_ARRAY_ELEMS(ssch->noise_idx_prev[0]));

            av_assert2(ts < FF_ARRAY_ELEMS(ssch->qmf_noise[0]));
            av_assert2(sb < FF_ARRAY_ELEMS(ssch->qmf_noise[0][0]));

            ssch->noise_idx_prev[ts][sb] = idx = noise_idx(sb, ts, ss, ssch);
            if (idx < 0)
                return AVERROR_INVALIDDATA;

            ssch->qmf_noise[0][ts][sb] = ssch->noise_lev_sb_adj[atsg][sb] * aspx_noise[idx][0];
            ssch->qmf_noise[1][ts][sb] = ssch->noise_lev_sb_adj[atsg][sb] * aspx_noise[idx][1];
        }
    }

    return 0;
}

static int generate_tones(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];

    memset(ssch->qmf_sine, 0, sizeof(ssch->qmf_sine));

    /* Loop over QMF time slots */
    for (int ts = ssch->atsg_sig[0] * s->num_ts_in_ats, atsg = 0;
         ts < ssch->atsg_sig[ssch->num_atsg_sig] * s->num_ts_in_ats; ts++) {
        if (ts == ssch->atsg_sig[atsg+1] * s->num_ts_in_ats)
            atsg++;

        /* Loop over QMF subbands in A-SPX */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            int idx;

            av_assert2(ts < FF_ARRAY_ELEMS(ssch->sine_idx_prev));
            av_assert2(sb < FF_ARRAY_ELEMS(ssch->sine_idx_prev[0]));

            av_assert2(ts < FF_ARRAY_ELEMS(ssch->qmf_sine[0]));
            av_assert2(sb < FF_ARRAY_ELEMS(ssch->qmf_sine[0][0]));

            av_assert2(atsg < FF_ARRAY_ELEMS(ssch->sine_lev_sb_adj));
            av_assert2(sb < FF_ARRAY_ELEMS(ssch->sine_lev_sb_adj[0]));

            ssch->sine_idx_prev[ts][sb] = idx = sine_idx(sb, ts, s, ssch);
            if (idx < 0)
                return AVERROR_INVALIDDATA;

            ssch->qmf_sine[0][ts][sb]  = ssch->sine_lev_sb_adj[atsg][sb];
            ssch->qmf_sine[0][ts][sb] *= aspx_sine[0][idx];
            ssch->qmf_sine[1][ts][sb]  = ssch->sine_lev_sb_adj[atsg][sb] * powf(-1, sb + ssch->sbx);
            ssch->qmf_sine[1][ts][sb] *= aspx_sine[1][idx];
        }
    }

    return 0;
}

static void assemble_hf_signal(AC4DecodeContext *s, Substream *ss, int ch_id)
{
    SubstreamChannel *ssch = &ss->ssch[ch_id];
    const int sbx = ssch->sbx;
    const int ts_offset_hfadj = 4;

    memcpy(ssch->Y_prev, ssch->Y, sizeof(ssch->Y));
    memset(ssch->Y, 0, sizeof(ssch->Y));

    /* Get delayed QMF subsamples from delay buffer */
    for (int ts = 0; ts < ssch->atsg_sig[0] * s->num_ts_in_ats; ts++) {
        av_assert2(ts < FF_ARRAY_ELEMS(ssch->Y[0]));
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            av_assert2(ts < FF_ARRAY_ELEMS(ssch->Y_prev[0]));
            av_assert2(ts < FF_ARRAY_ELEMS(ssch->Y[0]));
            av_assert2(sb < FF_ARRAY_ELEMS(ssch->Y[0][0]));

            ssch->Y[0][ts][sb] = ssch->Y_prev[0][ts][sb];
            ssch->Y[1][ts][sb] = ssch->Y_prev[1][ts][sb];
        }
    }

    /* Loop over QMF time slots */
    for (int ts = ssch->atsg_sig[0] * s->num_ts_in_ats, atsg = 0;
         ts < ssch->atsg_sig[ssch->num_atsg_sig] * s->num_ts_in_ats; ts++) {
        if (ts == ssch->atsg_sig[atsg+1] * s->num_ts_in_ats)
            atsg++;
        av_assert2(ts < FF_ARRAY_ELEMS(ssch->Y[0]));
        av_assert2(ts + ts_offset_hfadj < FF_ARRAY_ELEMS(ssch->Q_high[0]));
        /* Loop over QMF subbands */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            ssch->Y[0][ts][sb] = ssch->sig_gain_sb_adj[atsg][sb];
            ssch->Y[1][ts][sb] = 0;
            fcomplex_mul(&ssch->Y[0][ts][sb], &ssch->Y[1][ts][sb],
                         ssch->Y[0][ts][sb], ssch->Y[1][ts][sb],
                         ssch->Q_high[0][ts + ts_offset_hfadj][sb + sbx],
                         ssch->Q_high[1][ts + ts_offset_hfadj][sb + sbx]);
        }
    }

    /* Loop over time slots */
    for (int ts = ssch->atsg_sig[0] * s->num_ts_in_ats;
         ts < ssch->atsg_sig[ssch->num_atsg_sig] * s->num_ts_in_ats; ts++) {
        av_assert2(ts < FF_ARRAY_ELEMS(ssch->Y[0]));
        /* Loop over QMF subbands */
        for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
            ssch->Y[0][ts][sb] += ssch->qmf_sine[0][ts][sb];
            ssch->Y[1][ts][sb] += ssch->qmf_sine[1][ts][sb];
            ssch->Y[0][ts][sb] += ssch->qmf_noise[0][ts][sb];
            ssch->Y[1][ts][sb] += ssch->qmf_noise[1][ts][sb];
        }
    }

    memcpy(ssch->Q_prev, ssch->Q, sizeof(ssch->Q));

    for (int ts = 0; ts < ssch->atsg_sig[ssch->num_atsg_sig] * s->num_ts_in_ats; ts++) {
        av_assert2(ts < FF_ARRAY_ELEMS(ssch->Y[0]));
        av_assert2(ssch->num_sb_aspx + sbx <= MAX_QMF_BANDS);
        av_assert2(ts / s->num_ts_in_ats < FF_ARRAY_ELEMS(ssch->aspx_tic_used_in_slot));
        /* Loop over QMF subbands */

        if (ssch->aspx_tic_used_in_slot[ts / s->num_ts_in_ats]) {
            for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
                ssch->Q[0][ts][sbx+sb] = ssch->Q_prev[0][ts][sbx+sb];
                ssch->Q[1][ts][sbx+sb] = ssch->Q_prev[1][ts][sbx+sb];
            }
        } else {
            for (int sb = 0; sb < ssch->num_sb_aspx; sb++) {
                ssch->Q[0][ts][sbx+sb] = ssch->Q_prev[0][ts][sbx+sb];
                ssch->Q[1][ts][sbx+sb] = ssch->Q_prev[1][ts][sbx+sb];

                ssch->Q[0][ts][sbx+sb] += ssch->Y_prev[0][ts][sb] / 32768.f;
                ssch->Q[1][ts][sbx+sb] += ssch->Y_prev[1][ts][sb] / 32768.f;
            }
        }
    }
}

static int channels_aspx_processing(AC4DecodeContext *s, Substream *ss, int nb_ch)
{
    int ret;

    if (s->substream.codec_mode < CM_ASPX)
        return 0;

    for (int ch = 0; ch < nb_ch; ch++)
        aspx_processing(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++)
        get_qsignal_scale_factors(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++)
        get_qnoise_scale_factors(s, ss, ch);

    for (int ch = 0; ch < nb_ch; ch++) {
        if (ss->ssch[ch].aspx_balance == 0)
            mono_deq_signal_factors(s, ss, ch);
    }

    for (int ch = 0; ch < nb_ch; ch++) {
        if (ss->ssch[ch].aspx_balance == 0)
            mono_deq_noise_factors(s, ss, ch);
    }

    for (int ch = 0; ch < nb_ch; ch++) {
        if (ss->ssch[ch].aspx_balance) {
            joint_deq_signoise_factors(s, ss, (uint8_t[]){ch, ch+1});
            ch++;
        }
    }

    for (int ch = 0; ch < nb_ch; ch++)
        preflattening(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++)
        get_covariance(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++)
        get_alphas(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++)
        get_chirps(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++)
        create_high_signal(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++)
        estimate_spectral_envelopes(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++)
        map_signoise(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++)
        add_sinusoids(s, ss, ch);
    for (int ch = 0; ch < nb_ch; ch++) {
        ret = generate_tones(s, ss, ch);
        if (ret < 0)
            return ret;
    }
    for (int ch = 0; ch < nb_ch; ch++) {
        ret = generate_noise(s, ss, ch);
        if (ret < 0)
            return ret;
    }
    for (int ch = 0; ch < nb_ch; ch++)
        assemble_hf_signal(s, ss, ch);

    return 0;
}

static void channel_companding(AC4DecodeContext *s, Substream *ss, int ch)
{
    SubstreamChannel *ssch = &ss->ssch[ch];
    const int ts1 = ssch->atsg_sig[ssch->aspx_num_env] * s->num_ts_in_ats;
    const int ts0 = ssch->atsg_sig[0] * s->num_ts_in_ats;
    const int sb0 = 0, sb1 = ssch->sbx;
    const int K = sb1 - sb0;
    const int D = ts1 - ts0;
    const float a = 0.65f;
    const float G = powf(0.5f, -1.f / a);

    for (int ts = ts0; ts < ts1; ts++) {
        float sum = 0.f;

        for (int sb = sb0; sb < sb1; sb++) {
            float re = fabsf(ssch->Q[0][ts][sb]);
            float im = fabsf(ssch->Q[1][ts][sb]);

            ssch->E[ts][sb] = fmaxf(re, im) + 0.5f * fminf(re, im);
            sum += ssch->E[ts][sb];
        }

        ssch->L[ts] = sum * 0.9105f / K;
        ssch->g[ts] = powf(ssch->L[ts], (1.f - a) / a);
        ss->g_sum[ts] += ssch->g[ts];
    }

    if (ss->sync_flag == 0) {
        if (ss->compand_on[ch]) {
            for (int ts = ts0; ts < ts1; ts++) {
                const float factor = G * ssch->g[ts];

                for (int sb = sb0; sb < sb1; sb++) {
                    ssch->Q[0][ts][sb] *= factor;
                    ssch->Q[1][ts][sb] *= factor;
                }
            }
        } else if (ss->compand_avg) {
            float sum = 0.f;
            float factor;

            for (int ts = ts0; ts < ts1; ts++)
                sum += ssch->L[ts];

            factor = G * powf(sum / D, (1.f - a) / a);

            for (int ts = ts0; ts < ts1; ts++) {
                for (int sb = sb0; sb < sb1; sb++) {
                    ssch->Q[0][ts][sb] *= factor;
                    ssch->Q[1][ts][sb] *= factor;
                }
            }
        }
    }
}

static void channels_companding(AC4DecodeContext *s, Substream *ss, int nb_ch)
{
    if (s->substream.codec_mode < CM_ASPX)
        return;

    memset(ss->g_sum, 0, sizeof(ss->g_sum));

    for (int ch = 0; ch < nb_ch; ch++)
        channel_companding(s, ss, ch);

    if (ss->sync_flag) {
        const float a = 0.65f;
        const float G = powf(0.5f, -1.f / a);

        for (int ch = 0; ch < nb_ch; ch++) {
            SubstreamChannel *ssch = &ss->ssch[ch];
            const int ts1 = ssch->atsg_sig[ssch->aspx_num_env] * s->num_ts_in_ats;
            const int ts0 = ssch->atsg_sig[0] * s->num_ts_in_ats;
            const int sb0 = 0, sb1 = ssch->sbx;

            if (ss->compand_on[ch]) {
                for (int ts = ts0; ts < ts1; ts++) {
                    float factor = G * ss->g_sum[ts] / nb_ch;

                    for (int sb = sb0; sb < sb1; sb++) {
                        ssch->Q[0][ts][sb] *= factor;
                        ssch->Q[1][ts][sb] *= factor;
                    }
                }
            } else if (ss->compand_avg) {
                const int D = ts1 - ts0;
                float factor = 0.f;

                for (int ts = ts0; ts < ts1; ts++)
                    factor += ss->g_sum[ts] / nb_ch;
                factor /= D;
                factor *= G;

                for (int ts = ts0; ts < ts1; ts++) {
                    for (int sb = sb0; sb < sb1; sb++) {
                        ssch->Q[0][ts][sb] *= factor;
                        ssch->Q[1][ts][sb] *= factor;
                    }
                }
            }
        }
    }
}

static void decode_channel(AC4DecodeContext *s, int ch, float *pcm)
{
    Substream *ss = &s->substream;
    SubstreamChannel *ssch = &ss->ssch[ch];

    qmf_synthesis(s, ssch, pcm);
}

static int ac4_decode_frame(AVCodecContext *avctx, AVFrame *frame,
                            int *got_frame_ptr, AVPacket *avpkt)
{
    AC4DecodeContext *s = avctx->priv_data;
    GetBitContext *gb = &s->gbc;
    int ret, start_offset = 0;
    SubstreamInfo *ssinfo;
    int presentation;
    uint32_t header;

    if (avpkt->size < 8) {
        av_log(s->avctx, AV_LOG_ERROR, "invalid packet size: %d\n", avpkt->size);
        return AVERROR_INVALIDDATA;
    }

    header = AV_RB16(avpkt->data);
    if (header == 0xAC40 || header == 0xAC41) {
        int size = AV_RB16(avpkt->data + 2);

        start_offset = 4;
        if (size == 0xFFFF)
            start_offset += 3;
    }

    if ((ret = init_get_bits8(gb, avpkt->data, avpkt->size)) < 0)
        return ret;
    av_log(s->avctx, AV_LOG_DEBUG, "packet_size: %d\n", avpkt->size);
    skip_bits_long(gb, start_offset * 8);

    ret = ac4_toc(s);
    if (ret < 0)
        return ret;

    if (!s->have_iframe)
        return avpkt->size;

    if (s->version == 2 && !s->ssgroup[0].channel_coded) {
        avpriv_report_missing_feature(s->avctx, "object coding");
        return AVERROR_PATCHWELCOME;
    }

    presentation = FFMIN(s->target_presentation, FFMAX(0, s->nb_presentations - 1));
    ssinfo = s->version == 2 ? &s->ssgroup[0].ssinfo : &s->pinfo[presentation].ssinfo[0];
    avctx->sample_rate = s->fs_index ? 48000 : 44100;

    if (ssinfo->channel_mode >= FF_ARRAY_ELEMS(channel_mode_layouts)) {
        av_log(s->avctx, AV_LOG_ERROR, "invalid channel mode: %d\n", ssinfo->channel_mode);
        return AVERROR_INVALIDDATA;
    }

    av_channel_layout_copy(&avctx->ch_layout, &channel_mode_layouts[ssinfo->channel_mode]);
    avctx->sample_rate = av_rescale(avctx->sample_rate,
                                    s->resampling_ratio.den,
                                    s->resampling_ratio.num);
    frame->nb_samples = s->frame_len_base;
    if ((ret = ff_get_buffer(avctx, frame, 0)) < 0)
        return ret;

    skip_bits_long(gb, s->payload_base * 8);

    for (int i = 0; i < s->nb_substreams; i++) {
        int substream_type = s->substream_type[i];

        switch (substream_type) {
        case ST_SUBSTREAM:
            ret = ac4_substream(s, &s->ssgroup[0], ssinfo, s->substream_size[i]);
            break;
        case ST_PRESENTATION:
            skip_bits_long(gb, s->substream_size[i] * 8);
            break;
        default:
            av_assert0(0);
        }

        if (ret < 0)
            return ret;
        if (substream_type == ST_SUBSTREAM)
            break;
    }

    if (get_bits_left(gb) < 0)
        av_log(s->avctx, AV_LOG_WARNING, "overread\n");

    for (int ch = 0; ch < avctx->ch_layout.nb_channels; ch++)
        scale_spec(s, ch);

    switch (ssinfo->channel_mode) {
    case 0:
        /* nothing to do */
        break;
    case 1:
        stereo_processing(s, &s->substream);
        break;
    case 3:
    case 4:
        m5channel_processing(s, &s->substream);
        break;
    }

    for (int ch = 0; ch < avctx->ch_layout.nb_channels; ch++)
        prepare_channel(s, ch);

    channels_companding(s, &s->substream, avctx->ch_layout.nb_channels);

    ret = channels_aspx_processing(s, &s->substream, avctx->ch_layout.nb_channels);
    if (ret < 0)
        return ret;

    if (get_bits_left(gb) > 0)
        av_log(s->avctx, AV_LOG_DEBUG, "underread %d\n", get_bits_left(gb));

    for (int ch = 0; ch < avctx->ch_layout.nb_channels; ch++) {
        const int sch = av_channel_layout_channel_from_index(&avctx->ch_layout, ch);
        const int dch = s->ch_map[sch];

        decode_channel(s, dch, (float *)frame->extended_data[ch]);
    }

    if (s->iframe_global)
        frame->flags |= AV_FRAME_FLAG_KEY;

    s->first_frame = 0;

    *got_frame_ptr = 1;

    return avpkt->size;
}

static av_cold void ac4_flush(AVCodecContext *avctx)
{
    AC4DecodeContext *s = avctx->priv_data;

    s->have_iframe = 0;
    s->sequence_counter_prev = 0;
}

static av_cold int ac4_decode_end(AVCodecContext *avctx)
{
    AC4DecodeContext *s = avctx->priv_data;

    av_freep(&s->fdsp);

    for (int j = 0; j < 8; j++)
        for (int i = 0; i < 5; i++)
            av_tx_uninit(&s->tx_ctx[j][i]);

    return 0;
}

#define OFFSET(param) offsetof(AC4DecodeContext, param)
#define FLAGS AV_OPT_FLAG_DECODING_PARAM | AV_OPT_FLAG_AUDIO_PARAM

static const AVOption options[] = {
    { "presentation", "select presentation", OFFSET(target_presentation), AV_OPT_TYPE_INT, {.i64=0}, 0, INT_MAX, FLAGS },
    { NULL},
};

static const AVClass ac4_decoder_class = {
    .class_name = "AC4 decoder",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

const FFCodec ff_ac4_decoder = {
    .p.name         = "ac4",
    CODEC_LONG_NAME("AC-4"),
    .p.type         = AVMEDIA_TYPE_AUDIO,
    .p.id           = AV_CODEC_ID_AC4,
    .p.priv_class   = &ac4_decoder_class,
    .priv_data_size = sizeof(AC4DecodeContext),
    .init           = ac4_decode_init,
    .close          = ac4_decode_end,
    FF_CODEC_DECODE_CB(ac4_decode_frame),
    .flush          = ac4_flush,
    .p.capabilities = AV_CODEC_CAP_DR1 | AV_CODEC_CAP_CHANNEL_CONF,
    .caps_internal  = FF_CODEC_CAP_INIT_CLEANUP,
};
