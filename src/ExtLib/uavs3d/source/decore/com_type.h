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

#ifndef __COM_TYPE_H__
#define __COM_TYPE_H__

#include "com_def.h"
#include "com_table.h"

#include "../decoder/uavs3d.h"


/*****************************************************************************
* picture structrue
*****************************************************************************/

typedef struct uavs3d_com_pic_t {
    void      *mem_base;
    pel       *y, *uv;                      
    int        stride_luma;                 
    int        stride_chroma;               
    int        width_luma;                  
    int        height_luma;                 
    int        width_chroma;                
    int        height_chroma;               
    int        padsize_luma;                
    int        padsize_chroma; 
    int        doi; // -256 ~255                                    
    int        dtr;
    s64        ptr; 
    int        output_delay;
    u8         is_ref;                      
    u8         is_output;                   
    u8         temporal_id;            
    s16      (*map_mv)[REFP_NUM][MV_D];     // MV  on L1
    s8       (*map_refi)[REFP_NUM];         // Ref on L1
    s16        list_dist[MAX_REFS];

    /*** picture info for output ***/
    s64        pts;
    s64        dts;
    int        type;
    int        bit_depth;
    int        refpic_num[2];
    long long  refpic[2][16];

    long long  pkt_pos;
    int        pkt_size;

    /*** for parallel ***/
    int                     parallel_enable;
    int                     finished_line;
    int                     ref_cnt;
    uavs3d_pthread_mutex_t  mutex;
    uavs3d_pthread_cond_t   cond;
} com_pic_t;

typedef struct uavs3d_com_pic_param_t {
    int width; 
    int height;
    int pad_l; 
    int pad_c; 
    int i_scu;
    int f_scu;
    int bit_depth;
    int parallel;
} com_pic_param_t;

typedef struct uavs3d_com_pic_manager_t {
    com_pic_t         **list;       /* picture list */
    int               max_pb_size;  /* max picture buffer */
    int               cur_pb_size;  /* current picture buffer */
    s64               doi_cycles;   /* DOI cycle time */
    u8                prev_doi;     /* previous doi */
    com_pic_param_t   pic_param;    /* picture param */
} com_pic_manager_t;

typedef struct uavs3d_com_ref_pic_t {
    com_pic_t  *pic;                        /* address of reference picture */
    s16       (*map_mv  )[REFP_NUM][MV_D];  /* point to pic->map_mv   */
    s8        (*map_refi)[REFP_NUM];        /* point to pic->map_refi */
    s16         dist; 
} com_ref_pic_t;

typedef struct uavs3d_com_scu_t {
    u8 coded    : 1;
    u8 intra    : 1;
    u8 inter    : 1;
    u8 skip     : 1;
    u8 cbf      : 1;
    u8 affine   : 2;
    u8 reserved : 1;
} com_scu_t;

typedef struct uavs3d_com_map_t {
    void      *map_base;
    com_scu_t *map_scu;
    u32       *map_pos;
    u8        *map_edge;
    s8        *map_ipm;              

    s8        *map_patch;
    s8        *map_qp;

    s16      (*map_mv  )[REFP_NUM][MV_D];  /* MV  on L0 */
    s8       (*map_refi)[REFP_NUM];        /* Ref on L0 */

} com_map_t;

typedef struct uavs3d_com_sao_param_t {
    int enable; 
    int type; 
    int bandIdx[4];
    int offset[MAX_NUM_SAO_CLASSES];
} com_sao_param_t;

typedef struct uavs3d_com_alf_pic_param_t {
    int pic_alf_enable;
    int alf_flag[3];
    int varIndTab[NO_VAR_BINS];
    int filterCoeff_luma[NO_VAR_BINS][ALF_MAX_NUM_COEF];
    int filterCoeff_chroma[ALF_MAX_NUM_COEF * 2]; // U and V
} com_alf_pic_param_t;

typedef struct uavs3d_com_pic_header_t {
    int                 rpl_l0_idx;
    int                 rpl_l1_idx;
    com_rpl_t           rpl_l0;
    com_rpl_t           rpl_l1;
    u8                  num_ref_idx_active_override_flag;
    u8                  ref_pic_list_sps_flag[2];
                        
    u8                  slice_type;
    u8                  temporal_id;
    u8                  loop_filter_disable_flag;
    u32                 bbv_delay;
    u16                 bbv_check_time;
                        
    u8                  loop_filter_parameter_flag;
    int                 alpha_c_offset;
    int                 beta_offset;
                        
    u8                  chroma_quant_param_disable_flag;
    s8                  chroma_quant_param_delta_cb;
    s8                  chroma_quant_param_delta_cr;
                        
    s64                 ptr;

    com_alf_pic_param_t alf_param;

    int                 fixed_picture_qp_flag;
    int                 random_access_decodable_flag;
    int                 time_code_flag;
    int                 time_code;
    u8                  decode_order_index;
    int                 output_delay;
    int                 bbv_check_times;
    int                 progressive_frame;
    int                 picture_structure;
    int                 top_field_first;
    int                 repeat_first_field;
    int                 top_field_picture_flag;
    int                 picture_qp;
    int                 affine_subblock_size_idx;
    int                 pic_wq_enable;
    int                 pic_wq_data_idx;
    int                 wq_param;
    int                 wq_model;
    int                 wq_param_vector[6];
    u8                  wq_4x4_matrix[16];
    u8                  wq_8x8_matrix[64];
                        
    u8                  pic_md5[16];   
    u8                  pic_md5_exist; 

} com_pic_header_t;

typedef struct uavs3d_com_patch_header_t {
    u8               slice_sao_enable[N_C];
    u8               fixed_slice_qp_flag;
    u8               slice_qp;
} com_patch_header_t;

typedef struct uavs3d_com_part_info_t {
    u8 num_sub_part;
    int sub_x[MAX_NUM_PB];  
    int sub_y[MAX_NUM_PB];
    int sub_w[MAX_NUM_PB];
    int sub_h[MAX_NUM_PB];
} com_part_info_t;

typedef struct uavs3d_com_motion_t {
    s16 mv[REFP_NUM][MV_D];
    s8 ref_idx[REFP_NUM];
} com_motion_t;

typedef struct uavs3d_com_frm_t {
    com_pic_t         *pic;
    int                pic_header_inited;
    com_seqh_t         seqhdr;
    com_pic_header_t   pichdr;
    u8                *bs_buf;
    int                bs_buf_size;
    int                bs_length;
    int                num_refp[REFP_NUM];
    com_ref_pic_t      refp[MAX_REFS][REFP_NUM];
} com_frm_t;

typedef struct uavs3d_com_bs_t {
    u32  code;     
    int  leftbits; 
    u8  *cur;      
    u8  *end;      
} com_bs_t;

typedef u16 lbac_ctx_model_t;

typedef struct uavs3d_com_lbac_all_ctx_t {
    lbac_ctx_model_t   skip_flag         [LBAC_CTX_NUM_SKIP_FLAG];
    lbac_ctx_model_t   skip_idx_ctx      [LBAC_CTX_NUM_SKIP_IDX];
    lbac_ctx_model_t   direct_flag       [LBAC_CTX_NUM_DIRECT_FLAG];
    lbac_ctx_model_t   umve_flag;        
    lbac_ctx_model_t   umve_base_idx     [LBAC_CTX_NUM_UMVE_BASE_IDX];
    lbac_ctx_model_t   umve_step_idx     [LBAC_CTX_NUM_UMVE_STEP_IDX];
    lbac_ctx_model_t   umve_dir_idx      [LBAC_CTX_NUM_UMVE_DIR_IDX];
                                         
    lbac_ctx_model_t   inter_dir         [LBAC_CTX_NUM_INTER_DIR];
    lbac_ctx_model_t   intra_dir         [LBAC_CTX_NUM_INTRA_DIR];
    lbac_ctx_model_t   pred_mode         [LBAC_CTX_NUM_PRED_MODE];
    lbac_ctx_model_t   cons_mode         [LBAC_CTX_NUM_CONS_MODE];
    lbac_ctx_model_t   ipf_flag          [LBAC_CTX_NUM_IPF];
    lbac_ctx_model_t   refi              [LBAC_CTX_NUM_REFI];
    lbac_ctx_model_t   mvr_idx           [LBAC_CTX_NUM_MVR_IDX];
    lbac_ctx_model_t   affine_mvr_idx    [LBAC_CTX_NUM_AFFINE_MVR_IDX];

    lbac_ctx_model_t   mvp_from_hmvp_flag[LBAC_CTX_NUM_EXTEND_AMVR_FLAG];
    lbac_ctx_model_t   mvd               [2][LBAC_CTX_NUM_MVD];
    lbac_ctx_model_t   ctp_zero_flag     [LBAC_CTX_NUM_CTP_ZERO_FLAG];
    lbac_ctx_model_t   cbf               [LBAC_CTX_NUM_CBF];
    lbac_ctx_model_t   tb_split          [LBAC_CTX_NUM_TB_SPLIT];
    lbac_ctx_model_t   run               [LBAC_CTX_NUM_RUN];
    lbac_ctx_model_t   run_rdoq          [LBAC_CTX_NUM_RUN];
    lbac_ctx_model_t   last1             [LBAC_CTX_NUM_LAST1 * 2];
    lbac_ctx_model_t   last2             [LBAC_CTX_NUM_LAST2 * 2 - 2];
    lbac_ctx_model_t   level             [LBAC_CTX_NUM_LEVEL];
    lbac_ctx_model_t   split_flag        [LBAC_CTX_NUM_SPLIT_FLAG];
    lbac_ctx_model_t   bt_split_flag     [LBAC_CTX_NUM_BT_SPLIT_FLAG];
    lbac_ctx_model_t   split_dir         [LBAC_CTX_NUM_SPLIT_DIR];
    lbac_ctx_model_t   split_mode        [LBAC_CTX_NUM_SPLIT_MODE];
    lbac_ctx_model_t   affine_flag       [LBAC_CTX_NUM_AFFINE_FLAG];
    lbac_ctx_model_t   affine_mrg_idx    [LBAC_CTX_NUM_AFFINE_MRG];
    lbac_ctx_model_t   smvd_flag         [LBAC_CTX_NUM_SMVD_FLAG];
    lbac_ctx_model_t   part_size         [LBAC_CTX_NUM_PART_SIZE];
    lbac_ctx_model_t   sao_merge_flag    [LBAC_CTX_NUM_SAO_MERGE_FLAG];
    lbac_ctx_model_t   sao_mode          [LBAC_CTX_NUM_SAO_MODE];
    lbac_ctx_model_t   sao_offset        [LBAC_CTX_NUM_SAO_OFFSET];
    lbac_ctx_model_t   alf_lcu_enable    [LBAC_CTX_NUM_ALF];
    lbac_ctx_model_t   lcu_qp_delta      [LBAC_CTX_NUM_DELTA_QP];
} com_lbac_all_ctx_t;

typedef struct uavs3d_com_lbac_t {
    s32 range, low;
    u8  *cur, *end;
    com_lbac_all_ctx_t ctx;
} com_lbac_t;

typedef struct uavs3d_com_core_t {
    /*** Sequence Info ***/
    com_seqh_t *seqhdr;

    /*** Current CU ***/
    int  cu_mode;
    int  pb_part;
    int  tb_part;
    u8   cbfy[MAX_NUM_TB];
    u8   cbfc[2];
    s8   refi[REFP_NUM];
    s8   ipm[MAX_NUM_PB];
    s8   ipm_c;
    u8   ipf_flag;
    u8   affine_flag;
    s16  mv[REFP_NUM][MV_D];
    CPMV affine_mv[REFP_NUM][VER_NUM][MV_D];
    s32  affine_sb_mv[REFP_NUM][(MAX_CU_DIM >> MIN_CU_LOG2) >> MIN_CU_LOG2][MV_D];

    int  cu_scup;  // CU position in current frame 
    int  cu_pix_x;    
    int  cu_pix_y;    
    int  cu_log2w;    
    int  cu_log2h;    
    int  cu_width;
    int  cu_height;

    /*** current CU Tree  ***/
    int  tree_scup;
    u8   tree_status;
    u8   cons_pred_mode;

    /*** current LCU  ***/
    int  lcu_x;      
    int  lcu_y;      
    int  lcu_pix_x;  
    int  lcu_pix_y;  
    int  lcu_qp_y;   
    int  lcu_qp_u;   
    int  lcu_qp_v;   

    /*** current PIC ***/
    com_bs_t       bs;
    com_lbac_t     lbac;
    int            slice_type;
    int            num_refp[REFP_NUM];
    com_ref_pic_t  refp[MAX_REFS][REFP_NUM];
    com_pic_t     *pic;

    s8             hmvp_cands_cnt;
    com_motion_t   hmvp_cands_list[ALLOWED_HMVP_NUM];
    com_map_t      map;

    com_sao_param_t(*sao_param_map)[N_C];
    u8             (*alf_enable_map)[N_C];

    pel         *linebuf_intra[2];
    pel         *linebuf_sao[2];
    pel         *sao_src_buf[2];
    pel         *alf_src_buf[2];

    com_pic_header_t   *pichdr;  /* current picture header */
    com_patch_header_t  pathdr;  /* current patch   header */

} com_core_t;

#endif /* __COM_TYPE_H__ */
