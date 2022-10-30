/**************************************************************************************
 * Copyright (c) 2018-2022 ["Peking University Shenzhen Graduate School",
 *   "Peng Cheng Laboratory", and "Guangdong Bohua UHD Innovation Corporation"]
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the organizations (Peking University Shenzhen Graduate School,
 *    Peng Cheng Laboratory and Guangdong Bohua UHD Innovation Corporation) nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * For more information, contact us at rgwang@pkusz.edu.cn.
 **************************************************************************************/

#ifndef __UAVS3D_H__
#define __UAVS3D_H__

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) && !defined(__GNUC__)

// ==> Start patch MPC
/*
#ifdef UAVS3D_EXPORTS
#define AVS3D_API __declspec(dllexport)
#else
#define AVS3D_API __declspec(dllimport)
#endif
*/
#define AVS3D_API
// ==> End patch MPC

#else 

#define AVS3D_API
// ==> Start patch MPC
//#define __cdecl
// ==> End patch MPC

#endif 

typedef struct
{
    const char* input_file;
    const char* output_file;
    int frm_threads;
    int dec_frames;
    int log_level;
    int check_md5;
}uavs3d_cfg_t;

/*****************************************************************************
* return values and error code
*****************************************************************************/
#define RET_OK                        0
#define RET_DELAYED                   1     
#define RET_SEQ_END                   2

#define ERR_OUT_OF_MEMORY           (-1)
#define ERR_UNEXPECTED              (-2)
#define ERR_BAD_BITDEPTH            (-3)     
#define ERR_UNKNOWN_NAL             (-4)
#define ERR_LOSS_REF_FRAME          (-5)
#define ERR_PIC_HEADER_UNREADY      (-6)
#define ERR_SEQ_HEADER_UNREADY      (-7)
#define ERR_SEQ_MAININFO_CHANGED    (-8)
#define ERR_PIC_BUFFER_FULL         (-9)

#define ERR_UNKNOWN               (-100)    

/*****************************************************************************
* nal type
*****************************************************************************/
#define NAL_PIC_HEADER              1
#define NAL_SEQ_HEADER              2
#define NAL_SLICE                   3
#define NAL_SEQ_END                 7

/*****************************************************************************
* slice type
*****************************************************************************/
#define SLICE_I                     1
#define SLICE_P                     2
#define SLICE_B                     3

/*****************************************************************************
* RPL structreu
*****************************************************************************/

#define MAX_RPLS  32
#define MAX_REFS  17

typedef struct uavs3d_com_rpl_t {
    int num;
    int active;
    int delta_doi[MAX_REFS];
} com_rpl_t;

/*****************************************************************************
* sequence header
*****************************************************************************/

typedef struct uavs3d_com_seqh_t {
    unsigned char  profile_id;                 /*  8 bits */
    unsigned char  level_id;                   /*  8 bits */
    unsigned char  progressive_sequence;       /*  1 bit  */
    unsigned char  field_coded_sequence;       /*  1 bit  */
    unsigned char  chroma_format;              /*  2 bits */
    unsigned char  encoding_precision;         /*  3 bits */
    unsigned char  output_reorder_delay;       /*  5 bits */
    unsigned char  sample_precision;           /*  3 bits */
    unsigned char  aspect_ratio;               /*  4 bits */
    unsigned char  frame_rate_code;            /*  4 bits */
    unsigned int   bit_rate_lower;             /* 18 bits */
    unsigned int   bit_rate_upper;             /* 18 bits */
    unsigned char  low_delay;                  /*  1 bit  */
    unsigned char  temporal_id_enable_flag;    /*  1 bit  */
    unsigned int   bbv_buffer_size;            /* 18 bits */
    int            horizontal_size;            /* 14 bits */
    int            vertical_size;              /* 14 bits */
	int            display_horizontal_size;    /* 14 bits */
    int            display_vertical_size;      /* 14 bits */
	
    unsigned char  log2_max_cu_width_height;   /*  3 bits */
    unsigned char  min_cu_size;
    unsigned char  max_part_ratio_log2;
    unsigned char  max_split_times;
    unsigned char  min_qt_size;
    unsigned char  max_bt_size;
    unsigned char  max_eqt_size;
    unsigned char  max_dt_size;

    int            rpl1_index_exist_flag;
    int            rpl1_same_as_rpl0_flag;
    com_rpl_t      rpls_l0[MAX_RPLS];
    com_rpl_t      rpls_l1[MAX_RPLS];
    int            rpls_l0_num;
    int            rpls_l1_num;
    int            num_ref_default_active_minus1[2];
    int            max_dpb_size;
    int            ipcm_enable_flag;
    unsigned char  amvr_enable_flag;
    int            umve_enable_flag;
    int            ipf_enable_flag;
    int            emvr_enable_flag;
    unsigned char  affine_enable_flag;
    unsigned char  smvd_enable_flag;
    unsigned char  dt_intra_enable_flag;
    unsigned char  num_of_hmvp_cand;
    unsigned char  tscpm_enable_flag;
    unsigned char  sample_adaptive_offset_enable_flag;
    unsigned char  adaptive_leveling_filter_enable_flag;
    unsigned char  secondary_transform_enable_flag;
    unsigned char  position_based_transform_enable_flag;

    unsigned char  wq_enable;
    unsigned char  seq_wq_mode;
    unsigned char  wq_4x4_matrix[16];
    unsigned char  wq_8x8_matrix[64];

    unsigned char  patch_stable;
    unsigned char  cross_patch_loop_filter;
    unsigned char  patch_ref_colocated;
    unsigned char  patch_uniform;
    unsigned char  patch_width;
    unsigned char  patch_height;

    /**********************************************************************/
    /*** Extension info ***/

    int            pic_width;      /* decoding picture width */
    int            pic_height;     /* decoding picture height */
    int            max_cuwh;       /* maximum CU width and height */
    int            log2_max_cuwh;  /* log2 of maximum CU width and height */

    int            pic_width_in_lcu;
    int            pic_height_in_lcu;
    int            f_lcu;               // = pic_width_in_lcu * pic_height_in_lcu

    int            pic_width_in_scu;
    int            pic_height_in_scu;
    int            i_scu;               // stride of scu data, = pic_width_in_scu + 2
    int            f_scu;               // total  scu data size, = i_scu * (pic_height_in_scu + 2)
    int            a_scu;               // actual scu data size, = i_scu *  pic_height_in_scu

    int            bit_depth_internal;
    int            bit_depth_input;
    int            bit_depth_2_qp_offset;

    /* patch size */
    int            patch_columns;
    int            patch_rows;
    int            patch_column_width[64];
    int            patch_row_height[128];

    /* alf map */
    unsigned char *alf_idx_map;

    /* hdr info */
    unsigned char colour_description;
    unsigned char colour_primaries;
    unsigned char transfer_characteristics;
    unsigned char matrix_coefficients;

} com_seqh_t;

#define FRAME_MAX_PLANES 3
typedef struct uavs3d_io_frm_t {
    void  * priv;
    int     got_pic;
    int     num_plane;                              /* number of planes */
    int     bit_depth;                              /* bit depth */
    int     width[FRAME_MAX_PLANES];                /* width (in unit of pixel) */
    int     height[FRAME_MAX_PLANES];               /* height (in unit of pixel) */
    int     stride[FRAME_MAX_PLANES];               /* buffer stride (in unit of byte) */
    void  * buffer[FRAME_MAX_PLANES];               /* address of each plane */

    /* frame info */
    long long   ptr;
    long long   pts;
    long long   dtr;
    long long   dts;
    int         type;
    int         refpic_num[2];
    long long   refpic[2][16];

    long long   pkt_pos;
    int         pkt_size;

    /* bitstream */
    unsigned char *bs;
    int bs_len;

    int nal_type;

    com_seqh_t *seqhdr;
} uavs3d_io_frm_t;

typedef void(__cdecl *uavs3d_lib_output_callback_t)(uavs3d_io_frm_t *frm);

typedef   void* (__cdecl *uavs3d_create_t     )(uavs3d_cfg_t * dec_cfg, uavs3d_lib_output_callback_t callback, int * err);
AVS3D_API void*  __cdecl  uavs3d_create        (uavs3d_cfg_t * dec_cfg, uavs3d_lib_output_callback_t callback, int * err);

typedef   void  (__cdecl *uavs3d_delete_t     )(void *h);
AVS3D_API void   __cdecl  uavs3d_delete        (void *h);

typedef   void  (__cdecl *uavs3d_reset_t      )(void *h);
AVS3D_API void   __cdecl  uavs3d_reset         (void *h);

typedef   int   (__cdecl *uavs3d_flush_t      )(void *h, uavs3d_io_frm_t* frm);
AVS3D_API int    __cdecl  uavs3d_flush         (void *h, uavs3d_io_frm_t* frm);

typedef   int   (__cdecl *uavs3d_decode_t     )(void *h, uavs3d_io_frm_t* frm);
AVS3D_API int    __cdecl  uavs3d_decode        (void *h, uavs3d_io_frm_t* frm);

typedef   void  (__cdecl *uavs3d_img_cpy_cvt_t)(uavs3d_io_frm_t * dst, uavs3d_io_frm_t * src, int bit_depth);
AVS3D_API void   __cdecl  uavs3d_img_cpy_cvt   (uavs3d_io_frm_t * dst, uavs3d_io_frm_t * src, int bit_depth);



#ifdef __cplusplus
}
#endif

#endif //#ifndef __UAVS3D_H__
