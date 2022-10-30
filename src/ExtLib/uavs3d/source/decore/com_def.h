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

#ifndef __COM_DEF_H__
#define __COM_DEF_H__

#include "com_sys.h"

#define CHECK_RAND_STRM 0

#if defined(__linux__)
#if COMPILE_10BIT
typedef unsigned short pel; /* pixel type */
#define BIT_DEPTH 10
#else
typedef unsigned char pel; /* pixel type */
#define BIT_DEPTH 8
#endif

#else
// ==> Start patch MPC
// #define COMPILE_10BIT 0
// ==> End patch MPC
#if COMPILE_10BIT
typedef unsigned short pel;
#define BIT_DEPTH 10
#else
typedef unsigned char pel;
#define BIT_DEPTH 8
#endif

#endif

/*************************  profile & level **********************************************/
#define PROFILE_ID  0x22
#define LEVEL_ID    0x6A

/*************************  CABAC ******************************************************/
#define CABAC_BITS 16
#define CABAC_MASK ((1<<CABAC_BITS)-1)
#define PROB_LPS_BITS                     8
#define PROB_BITS                         (PROB_LPS_BITS + 1)  // 9
#define PROB_LPS_SHIFTNO                  2
#define PROB_LPS_BITS_STORE               (PROB_LPS_BITS + PROB_LPS_SHIFTNO) // 10
#define PROB_MASK                         ((1 << (PROB_LPS_BITS_STORE + 1)) - 1) // mask for PROB_LPS + mps_character


// 0                 256                512                                  1024
// |------------------|------------------|------------------|------------------|
//  <------------ MAX_RANGE  ----------->
//  <-- HALF_RANGE -->

#define HALF_RANGE                        ( 1 << (PROB_LPS_BITS) )           //  256
#define MAX_LPS_RANGE                     ((1 << (PROB_LPS_BITS    )) - 1)   //  255
#define MAX_RANGE                         ((1 << (PROB_LPS_BITS + 1)) - 1)   //  511
#define DOUBLE_RANGE                      ((1 << (PROB_LPS_BITS + 2)) - 1)   // 1203

#define LBAC_CTX_NUM_SKIP_FLAG                  4
#define LBAC_CTX_NUM_SKIP_IDX                   (MAX_SKIP_NUM - 1)
#define LBAC_CTX_NUM_DIRECT_FLAG                2
#define LBAC_CTX_NUM_UMVE_BASE_IDX              1
#define LBAC_CTX_NUM_UMVE_STEP_IDX              1
#define LBAC_CTX_NUM_UMVE_DIR_IDX               2
#define LBAC_CTX_NUM_SPLIT_FLAG                 4
#define LBAC_CTX_NUM_SPLIT_MODE                 3
#define LBAC_CTX_NUM_BT_SPLIT_FLAG              9
#define LBAC_CTX_NUM_SPLIT_DIR                  5
#define LBAC_CTX_NUM_CBF                        3   
#define LBAC_CTX_NUM_CTP_ZERO_FLAG              2   
#define LBAC_CTX_NUM_PRED_MODE                  6   
#define LBAC_CTX_NUM_CONS_MODE                  1   
#define LBAC_CTX_NUM_TB_SPLIT                   1
#define LBAC_CTX_NUM_DELTA_QP                   4
#define LBAC_CTX_NUM_INTER_DIR                  3   
#define LBAC_CTX_NUM_REFI                       3
#define LBAC_CTX_NUM_IPF                        1 
#define LBAC_CTX_NUM_MVR_IDX                    (MAX_NUM_MVR - 1)
#define LBAC_CTX_NUM_AFFINE_MVR_IDX             (MAX_NUM_AFFINE_MVR - 1)
#define LBAC_CTX_NUM_EXTEND_AMVR_FLAG           1
#define LBAC_CTX_NUM_MVD                        3       
#define LBAC_CTX_NUM_INTRA_DIR                  10
#define LBAC_CTX_NUM_AFFINE_FLAG                1
#define LBAC_CTX_NUM_AFFINE_MRG                 AFF_MAX_NUM_MRG - 1
#define LBAC_CTX_NUM_SMVD_FLAG                  1
#define LBAC_CTX_NUM_PART_SIZE                  6
#define LBAC_CTX_NUM_SAO_MERGE_FLAG             3
#define LBAC_CTX_NUM_SAO_MODE                   1
#define LBAC_CTX_NUM_SAO_OFFSET                 1
#define LBAC_CTX_NUM_ALF                        1

#define RANK_NUM 6

#define LBAC_CTX_NUM_LEVEL                      (RANK_NUM * 4)
#define LBAC_CTX_NUM_RUN                        (RANK_NUM * 4)
#define LBAC_CTX_NUM_RUN_RDOQ                   (RANK_NUM * 4)
#define LBAC_CTX_NUM_LAST1                       RANK_NUM
#define LBAC_CTX_NUM_LAST2                      12
#define LBAC_CTX_NUM_LAST                       2

/************************* QUANT **************************************************/
#define MIN_QUANT                          0
#define MAX_QUANT_BASE                     63

/************************* AMVR ***************************************************/
#define MAX_NUM_MVR                        5  /* 0 (1/4-pel) ~ 4 (4-pel) */
#define MAX_NUM_AFFINE_MVR                 3
#define FAST_MVR_IDX                       2
#define SKIP_MVR_IDX                       1

/************************* DEBLOCK ************************************************/
#define EDGE_TYPE_LUMA                     0x1
#define EDGE_TYPE_ALL                      0x3
#define LOOPFILTER_GRID                    8
#define LOOPFILTER_GRID_C                  (LOOPFILTER_GRID << 1)

/************************* SAO ****************************************************/
#define NUM_BO_OFFSET                      32
#define MAX_NUM_SAO_CLASSES                5
#define NUM_SAO_BO_CLASSES_LOG2            5
#define NUM_SAO_BO_CLASSES_IN_BIT          5
#define NUM_SAO_EO_TYPES_LOG2              2
#define NUM_SAO_BO_CLASSES                 (1<<NUM_SAO_BO_CLASSES_LOG2)
#define SAO_SHIFT_PIX_NUM                  4

#define SAO_TYPE_EO_0    0
#define SAO_TYPE_EO_90   1
#define SAO_TYPE_EO_135  2
#define SAO_TYPE_EO_45   3
#define SAO_TYPE_BO      4

typedef enum uavs3d_com_sao_eo_class_t {
    SAO_CLASS_EO_FULL_VALLEY,
    SAO_CLASS_EO_HALF_VALLEY,
    SAO_CLASS_EO_PLAIN,
    SAO_CLASS_EO_HALF_PEAK,
    SAO_CLASS_EO_FULL_PEAK,
    SAO_CLASS_BO,
    NUM_SAO_OFFSET
} com_sao_eo_class_t;

/************************* ALF ***************************************************/
#define ALF_MAX_NUM_COEF                    9
#define NO_VAR_BINS                        16
#define ALF_NUM_BIT_SHIFT                   6

/************************* AFFINE ************************************************/
#define VER_NUM                             4
#define AFFINE_MAX_NUM_LT                   3 ///< max number of motion candidates in top-left corner
#define AFFINE_MAX_NUM_RT                   2 ///< max number of motion candidates in top-right corner
#define AFFINE_MAX_NUM_LB                   2 ///< max number of motion candidates in left-bottom corner
#define AFFINE_MAX_NUM_RB                   1 ///< max number of motion candidates in right-bottom corner
#define AFFINE_MIN_BLOCK_SIZE               4 ///< Minimum affine MC block size
#define AFF_MAX_NUM_MRG                     5 // maximum affine merge candidates
#define AFF_MODEL_CAND                      2 // maximum affine model based candidate
#define AFF_BITSIZE                         4
#define AFF_SIZE                           16
#define Tab_Affine_AMVR(x)                 ((x==0) ? 2 : ((x==1) ? 4 : 0) )

/************************* COLOR PLANE ********************************************/
#define Y_C                                0  /* Y luma */
#define UV_C                               1  /* CbCr Chroma */
#define U_C                                1  /* Cb Chroma */
#define V_C                                2  /* Cr Chroma */
#define N_C                                3  /* number of color component */

/************************* REF LIST DEFINE ****************************************/
#define REFP_0                             0
#define REFP_1                             1
#define REFP_NUM                           2

typedef enum uavs3d_com_pred_direction_t {
    PRED_L0 = 0,
    PRED_L1 = 1,
    PRED_BI = 2,
    PRED_DIR_NUM = 3,
} com_pred_direction_t;

/************************* MC ****************************************************/
#define MV_X                               0
#define MV_Y                               1
#define MV_D                               2


typedef int                                CPMV;

#define CPMV_BIT_DEPTH                     18
#define COM_CPMV_MAX                       ((s32)((1<<(CPMV_BIT_DEPTH - 1)) - 1))
#define COM_CPMV_MIN                       ((s32)(-(1<<(CPMV_BIT_DEPTH - 1))))

#define NUM_SPATIAL_MV                     3
#define NUM_SKIP_SPATIAL_MV                6
#define MVPRED_L                           0
#define MVPRED_U                           1
#define MVPRED_UR                          2
#define MVPRED_xy_MIN                      3
#define MV_SCALE_PREC                      14 

typedef enum uavs3d_com_ipfilter_t {
    IPFILTER_H_8,
    IPFILTER_H_4,
    IPFILTER_V_8,
    IPFILTER_V_4,
    NUM_IPFILTER
} com_ipfilter_t;

typedef enum uavs3d_com_ipfilter_txt_t {
    IPFILTER_EXT_8,
    IPFILTER_EXT_4,
    NUM_IPFILTER_Ext
} com_ipfilter_txt_t;


/************************* BLOCK SIZE & POSITION ************************************************/
#define MAX_CU_LOG2                        7
#define MIN_CU_LOG2                        2
#define MAX_CU_SIZE                       (1 << MAX_CU_LOG2)
#define MIN_CU_SIZE                       (1 << MIN_CU_LOG2)
#define MAX_CU_DIM                        (MAX_CU_SIZE * MAX_CU_SIZE)
#define MIN_CU_DIM                        (MIN_CU_SIZE * MIN_CU_SIZE)
#define MAX_CU_DEPTH                       6  /* 128x128 ~ 4x4 */
#define MAX_TR_LOG2                        6  /* 64x64 */
#define MAX_TR_SIZE                        (1 << MAX_TR_LOG2)
#define MAX_TR_DIM                         (MAX_TR_SIZE * MAX_TR_SIZE)
#define MAX_NUM_PB                         4
#define MAX_NUM_TB                         4
#define MAX_CU_CNT_IN_LCU                  (MAX_CU_DIM/MIN_CU_DIM)
#define BLOCK_WIDTH_TYPES_NUM              (MAX_CU_LOG2 - MIN_CU_LOG2 + 1) // 4 --> 128

/************************* PB/TB BLOCK ************************************************/
#define PB0                                0  // default PB idx
#define TB0                                0  // default TB idx
#define TBUV0                              0  // default TB idx for chroma


/************************* PIC PADDING ************************************************/
#define PIC_PAD_SIZE_L                     (MAX_CU_SIZE + 32)
#define PIC_PAD_SIZE_C                     (PIC_PAD_SIZE_L >> 1)

/************************* REFERENCE FRAME MANAGER ************************************************/
#define MAX_NUM_ACTIVE_REF_FRAME           4
#define EXTRA_FRAME                        MAX_REFS                 
#define MAX_PB_SIZE                       (MAX_REFS + EXTRA_FRAME) 
#define DOI_CYCLE_LENGTH                   256                       


/************************* Neighboring block availability ************************************************/
#define AVAIL_BIT_UP                       0
#define AVAIL_BIT_LE                       1
#define AVAIL_BIT_UL                       2

#define AVAIL_UP                          (1 << AVAIL_BIT_UP)
#define AVAIL_LE                          (1 << AVAIL_BIT_LE)
#define AVAIL_UL                          (1 << AVAIL_BIT_UL)

#define IS_AVAIL(avail, pos)             ((avail)&(pos))

/************************* PREDICT MODE ************************************************/
#define MODE_INTRA                         0
#define MODE_INTER                         1
#define MODE_SKIP                          2
#define MODE_DIR                           3

/************************* skip / direct mode ************************************************/
#define TRADITIONAL_SKIP_NUM               (PRED_DIR_NUM + 1)
#define ALLOWED_HMVP_NUM                   8
#define MAX_SKIP_NUM                       (TRADITIONAL_SKIP_NUM + ALLOWED_HMVP_NUM)

#define UMVE_BASE_NUM                      2
#define UMVE_REFINE_STEP                   5
#define UMVE_MAX_REFINE_NUM                (UMVE_REFINE_STEP * 4)


/************************* INTRA MODE ************************************************/
#define IPD_DC                              0  /* Luma, DC */
#define IPD_PLN                             1  /* Luma, Planar */
#define IPD_BI                              2  /* Luma, Bilinear */
#define IPD_DM_C                            0  /* Chroma, DM */
#define IPD_DC_C                            1  /* Chroma, DC */
#define IPD_HOR_C                           2  /* Chroma, Horizontal*/
#define IPD_VER_C                           3  /* Chroma, Vertical */
#define IPD_BI_C                            4  /* Chroma, Bilinear */
#define IPD_TSCPM_C                         5  /* Chroma, Tscpm */

#define IPD_CHROMA_CNT                      5
#define IPD_HOR                            24  
#define IPD_VER                            12  
#define IPD_CNT                            33
#define IPD_IPCM                           33
#define IPD_DIA_R                          18  
#define IPD_DIA_L                           3  
#define IPD_DIA_U                          32  

#define COM_IPRED_CHK_CONV(mode) ((mode) == IPD_VER || (mode) == IPD_HOR || (mode) == IPD_DC || (mode) == IPD_BI)


/************************* REF idx Set ************************************************/
#define REFI_INVALID                      (-1)
#define REFI_IS_VALID(refi)               ((refi) >= 0)

/************************* SCU MAP SET ************************************************/
#define MCU_SET_SCUP(m,v)               (m)=(((m) & 0x3F)|(v)<<6)
#define MCU_GET_SCUP(m)                 (int)((m) >> 6)

/* set cu_log2w & cu_log2h to map */
#define MCU_SET_LOGW(m, v)              (m)=(((m) & (~0x07))|((v)&0x07)   )
#define MCU_SET_LOGH(m, v)              (m)=(((m) & (~0x38))|((v)&0x07)<<3)

/* get cu_log2w & cu_log2h to map */
#define MCU_GET_LOGW(m)                 (int)(((m   ))&0x07)
#define MCU_GET_LOGH(m)                 (int)(((m)>>3)&0x07)


/*************************** QUANT & ITRANS **********************************************/
#define MAX_TX_DYNAMIC_RANGE                     15
#define GET_TRANS_SHIFT(bit_depth, tr_size_log2) (MAX_TX_DYNAMIC_RANGE - bit_depth - tr_size_log2)

typedef enum uavs3d_com_trans_type_t {
    DCT2,
    DCT8,
    DST7,
    NUM_TRANS_TYPE
} com_trans_type_t;

/*************************** user data types **********************************************/
#define COM_UD_PIC_SIGNATURE              0x10
#define COM_UD_END                        0xFF

/*****************************************************************************
 * split info
 *****************************************************************************/
typedef enum uavs3d_com_split_mode_t {
    NO_SPLIT,
    SPLIT_BI_VER,
    SPLIT_BI_HOR,
    SPLIT_EQT_VER,
    SPLIT_EQT_HOR,
    SPLIT_QUAD,
    NUM_SPLIT_MODE
} com_split_mode_t;

typedef enum uavs3d_com_split_dir_t {
    SPLIT_VER,
    SPLIT_HOR,
    SPLIT_QT,
} com_split_dir_t;

typedef enum uavs3d_com_cons_mode_t {
    NO_MODE_CONS,
    ONLY_INTER,
    ONLY_INTRA,
} com_cons_mode_t;

typedef enum uavs3d_com_tree_status_t {
    TREE_LC,
    TREE_L,
    TREE_C,
} com_tree_status_t;

typedef enum uavs3d_com_part_size_t {
    SIZE_2Nx2N,           ///< symmetric partition,  2Nx2N
    SIZE_2NxnU,           ///< asymmetric partition, 2Nx( N/2) + 2Nx(3N/2)
    SIZE_2NxnD,           ///< asymmetric partition, 2Nx(3N/2) + 2Nx( N/2)
    SIZE_nLx2N,           ///< asymmetric partition, ( N/2)x2N + (3N/2)x2N
    SIZE_nRx2N,           ///< asymmetric partition, (3N/2)x2N + ( N/2)x2N
    SIZE_NxN,             ///< symmetric partition, NxN
    SIZE_2NxhN,           ///< symmetric partition, 2N x 0.5N
    SIZE_hNx2N,           ///< symmetric partition, 0.5N x 2N
    NUM_PART_SIZE
} com_part_size_t;

typedef enum uavs3d_com_blk_shape_t {
    NON_SQUARE_18,
    NON_SQUARE_14,
    NON_SQUARE_12,
    SQUARE,
    NON_SQUARE_21,
    NON_SQUARE_41,
    NON_SQUARE_81,
    NUM_BLOCK_SHAPE,
} com_blk_shape_t;

#endif /* __COM_DEF_H__ */
