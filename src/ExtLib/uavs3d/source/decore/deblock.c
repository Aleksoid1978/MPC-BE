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

#include "modules.h"

static tab_u8 com_tbl_deblock_alpha[64] =
{
    0,  0,  0,  0,  0,  0,  0,  0,
    1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  2,  2,  2,
    2,  2,  2,  3,  3,  3,  3,  4,
    4,  4,  5,  5,  6,  6,  7,  7,
    8,  9,  10, 10, 11, 12, 13, 15,
    16, 17, 19, 21, 23, 25, 27, 29,
    32, 35, 38, 41, 45, 49, 54, 59
};
static tab_u8  com_tbl_deblock_beta[64] =
{
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  2,  2,
    2,  2,  2,  2,  3,  3,  3,  3,
    4,  4,  5,  5,  5,  6,  6,  7,
    8,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22,
    23, 23, 24, 24, 25, 25, 26, 27
};


static int uavs3d_always_inline skip_filter(com_map_t *map, com_ref_pic_t refp[MAX_REFS][REFP_NUM], s8 *refi_p, com_pic_t *p_pic0, com_pic_t *p_pic1, int scup, int offset)
{
    com_scu_t MbQ = map->map_scu[scup + offset];
    com_pic_t *q_pic0, *q_pic1;
    const com_scu_t mask = {0, 1, 0, 0, 1, 0, 0};

    if ((*(u8*)&MbQ) & (*(u8*)&mask)) {
        return 0;
    }

    s8 *refi_q = map->map_refi[scup + offset];

    if (M16(refi_p) == M16(refi_q)) {
        s16(*mv_p)[MV_D] = map->map_mv[scup];
        s16(*mv_q)[MV_D] = map->map_mv[scup + offset];

        if (REFI_IS_VALID(refi_p[REFP_0])) {
            if ((abs(mv_p[REFP_0][MV_X] - mv_q[REFP_0][MV_X]) >= 4) ||
                (abs(mv_p[REFP_0][MV_Y] - mv_q[REFP_0][MV_Y]) >= 4)) {
                return 0;
            }
        }
        if (REFI_IS_VALID(refi_p[REFP_1])) {
            if ((abs(mv_p[REFP_1][MV_X] - mv_q[REFP_1][MV_X]) >= 4) ||
                (abs(mv_p[REFP_1][MV_Y] - mv_q[REFP_1][MV_Y]) >= 4)) {
                return 0;
            }
        }
        return 1;
    } 

    q_pic0 = REFI_IS_VALID(refi_q[REFP_0]) ? refp[refi_q[REFP_0]][REFP_0].pic : NULL;
    q_pic1 = REFI_IS_VALID(refi_q[REFP_1]) ? refp[refi_q[REFP_1]][REFP_1].pic : NULL;

    if (p_pic0 == q_pic1 && p_pic1 == q_pic0) {
        s16(*mv_p)[MV_D] = map->map_mv[scup];
        s16(*mv_q)[MV_D] = map->map_mv[scup + offset];

        if (REFI_IS_VALID(refi_p[REFP_0])) {
            if ((abs(mv_p[REFP_0][MV_X] - mv_q[REFP_1][MV_X]) >= 4) ||
                (abs(mv_p[REFP_0][MV_Y] - mv_q[REFP_1][MV_Y]) >= 4)) {
                return 0;
            }
        }
        if (REFI_IS_VALID(refi_p[REFP_1])) {
            if ((abs(mv_p[REFP_1][MV_X] - mv_q[REFP_0][MV_X]) >= 4) ||
                (abs(mv_p[REFP_1][MV_Y] - mv_q[REFP_0][MV_Y]) >= 4)) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

static void uavs3d_always_inline com_df_set_edge_ver_inline(com_map_t *map, int i_scu, com_ref_pic_t refp[MAX_REFS][REFP_NUM], int scu_x, int scu_y, int h, int edgecondition)
{
    int scup = scu_y * i_scu + scu_x;
    u8 *edge_filter = map->map_edge + scup;
    s8 *refi_p = map->map_refi[scup];
    com_pic_t *p_pic0 = REFI_IS_VALID(refi_p[REFP_0]) ? refp[refi_p[REFP_0]][REFP_0].pic : NULL;
    com_pic_t *p_pic1 = REFI_IS_VALID(refi_p[REFP_1]) ? refp[refi_p[REFP_1]][REFP_1].pic : NULL;

    while (h--) {
        *edge_filter |= skip_filter(map, refp, refi_p, p_pic0, p_pic1, scup, -1) ? 0 : edgecondition;
        edge_filter += i_scu;
        scup += i_scu;
    }
}

static void uavs3d_always_inline com_df_set_edge_hor_inline(com_map_t *map, int i_scu, com_ref_pic_t refp[MAX_REFS][REFP_NUM], int scu_x, int scu_y, int w, int edgecondition)
{
    int scup = scu_y * i_scu + scu_x;
    u8 *edge_filter = map->map_edge + scup;
    s8 *refi_p = map->map_refi[scup];
    com_pic_t *p_pic0 = REFI_IS_VALID(refi_p[REFP_0]) ? refp[refi_p[REFP_0]][REFP_0].pic : NULL;
    com_pic_t *p_pic1 = REFI_IS_VALID(refi_p[REFP_1]) ? refp[refi_p[REFP_1]][REFP_1].pic : NULL;

    edgecondition <<= 2;

    while (w--) {
        *edge_filter++ |= skip_filter(map, refp, refi_p, p_pic0, p_pic1, scup++, -i_scu) ? 0 : edgecondition;
    }
}

static void com_df_set_edge_ver(com_map_t *map, int i_scu, com_ref_pic_t refp[MAX_REFS][REFP_NUM], int pix_x, int scu_y, int cuh, int edgecondition)
{
    com_df_set_edge_ver_inline(map, i_scu, refp, pix_x, scu_y, cuh, edgecondition);
}

static void com_df_set_edge_hor(com_map_t *map, int i_scu, com_ref_pic_t refp[MAX_REFS][REFP_NUM], int scu_x, int pix_y, int cuw, int edgecondition)
{
    com_df_set_edge_hor_inline(map, i_scu, refp, scu_x, pix_y, cuw, edgecondition);
}

static void uavs3d_always_inline com_df_set_edge_ver_force(u8 *edge_filter, int i_scu, int scu_x, int scu_y, int h, int edgecondition)
{
    const int grad_mask = (LOOPFILTER_GRID >> 2) - 1;
    if (((scu_x)& grad_mask) == 0) {
        edge_filter += scu_y * i_scu + scu_x;
        while (h--) {
            *edge_filter |= edgecondition;
            edge_filter += i_scu;
        }
    }
}

static void uavs3d_always_inline com_df_set_edge_hor_force(u8 *edge_filter, int i_scu, int scu_x, int scu_y, int w, int edgecondition)
{
    const int grad_mask = (LOOPFILTER_GRID >> 2) - 1;
    if (((scu_y)& grad_mask) == 0) {
        edgecondition <<= 2;
        edge_filter += scu_y * i_scu + scu_x;
        while (w--) {
            *edge_filter++ |= edgecondition;
        }
    }
}

static void uavs3d_always_inline com_df_set_edge_ver_force_cu(u8 *edge_filter, int i_scu, int h, int edgecondition)
{
    while (h--) {
        *edge_filter |= edgecondition;
        edge_filter += i_scu;
    }
}

static void uavs3d_always_inline com_df_set_edge_hor_force_cu(u8 *edge_filter, int i_scu, int w, int edgecondition)
{
    edgecondition <<= 2;

    while (w--) {
        *edge_filter++ |= edgecondition;
    }
}

void com_deblock_set_edge(com_core_t *core)
{
    com_seqh_t *seqhdr = core->seqhdr;
    com_map_t *map = &core->map;
    s8 *patch_idx = map->map_patch;
    int i_scu = seqhdr->i_scu;
    int scup = core->cu_scup;
    int lcu_idx = core->lcu_y * seqhdr->pic_width_in_lcu + core->lcu_x;
    int scu_w = core->cu_width  >> MIN_CU_LOG2;
    int scu_h = core->cu_height >> MIN_CU_LOG2;
    int scu_x = core->cu_pix_x  >> MIN_CU_LOG2;
    int scu_y = core->cu_pix_y  >> MIN_CU_LOG2;
    const int grad_mask = (LOOPFILTER_GRID >> 2) - 1;
    const com_scu_t mask = { 0, 1, 0, 0, 1, 0, 0 };
    com_scu_t scu = map->map_scu[scup];

    if ((*(u8*)&scu) & (*(u8*)&mask)) {
        u8 *edge = map->map_edge;

        switch (core->tb_part) {
        case SIZE_2NxhN:
            com_df_set_edge_hor_force(edge, i_scu, scu_x, scu_y + scu_h / 4 * 1, scu_w, EDGE_TYPE_LUMA);
            com_df_set_edge_hor_force(edge, i_scu, scu_x, scu_y + scu_h / 2    , scu_w, EDGE_TYPE_LUMA);
            com_df_set_edge_hor_force(edge, i_scu, scu_x, scu_y + scu_h / 4 * 3, scu_w, EDGE_TYPE_LUMA);
            break;                    
        case SIZE_hNx2N:              
            com_df_set_edge_ver_force(edge, i_scu, scu_x + scu_w / 4 * 1, scu_y, scu_h, EDGE_TYPE_LUMA);
            com_df_set_edge_ver_force(edge, i_scu, scu_x + scu_w / 2    , scu_y, scu_h, EDGE_TYPE_LUMA);
            com_df_set_edge_ver_force(edge, i_scu, scu_x + scu_w / 4 * 3, scu_y, scu_h, EDGE_TYPE_LUMA);
            break;                    
        case SIZE_NxN:                
            com_df_set_edge_hor_force(edge, i_scu, scu_x, scu_y + scu_h / 2, scu_w, EDGE_TYPE_LUMA);
            com_df_set_edge_ver_force(edge, i_scu, scu_x + scu_w / 2, scu_y, scu_h, EDGE_TYPE_LUMA);
            break;
        default:
            break;
        }
        if (scu_y && (scu_y & grad_mask) == 0 && (core->cu_pix_y != core->lcu_pix_y || seqhdr->cross_patch_loop_filter || patch_idx[lcu_idx] == patch_idx[lcu_idx - seqhdr->pic_width_in_lcu])) {
            com_df_set_edge_hor_force_cu(edge + scup, i_scu, scu_w, EDGE_TYPE_ALL);  // UP
        }
        if (scu_x && (scu_x & grad_mask) == 0 && (core->cu_pix_x != core->lcu_pix_x || seqhdr->cross_patch_loop_filter || patch_idx[lcu_idx] == patch_idx[lcu_idx - 1])) {
            com_df_set_edge_ver_force_cu(edge + scup, i_scu, scu_h, EDGE_TYPE_ALL);  // LEFT
        }

    } else {
        com_ref_pic_t(*refp)[REFP_NUM] = core->refp;

#define set_edge_hor(map, i_scu, refp, scu_x, scu_y, cu_w, flag) { if (((scu_y) & grad_mask) == 0)  com_df_set_edge_hor(map, i_scu, refp, scu_x, scu_y, cu_w, flag); }
#define set_edge_ver(map, i_scu, refp, scu_x, scu_y, cu_w, flag) { if (((scu_x) & grad_mask) == 0)  com_df_set_edge_ver(map, i_scu, refp, scu_x, scu_y, cu_w, flag); }
        
        switch (core->tb_part) {
        case SIZE_2NxhN:
            set_edge_hor(map, i_scu, refp, scu_x, scu_y + scu_h / 4 * 1, scu_w, EDGE_TYPE_LUMA);
            set_edge_hor(map, i_scu, refp, scu_x, scu_y + scu_h / 2    , scu_w, EDGE_TYPE_LUMA);
            set_edge_hor(map, i_scu, refp, scu_x, scu_y + scu_h / 4 * 3, scu_w, EDGE_TYPE_LUMA);
            break;
        case SIZE_hNx2N:
            set_edge_ver(map, i_scu, refp, scu_x + scu_w / 4 * 1, scu_y, scu_h, EDGE_TYPE_LUMA);
            set_edge_ver(map, i_scu, refp, scu_x + scu_w / 2    , scu_y, scu_h, EDGE_TYPE_LUMA);
            set_edge_ver(map, i_scu, refp, scu_x + scu_w / 4 * 3, scu_y, scu_h, EDGE_TYPE_LUMA);
            break;
        case SIZE_NxN:
            set_edge_hor(map, i_scu, refp, scu_x, scu_y + scu_h / 2, scu_w, EDGE_TYPE_LUMA);
            set_edge_ver(map, i_scu, refp, scu_x + scu_w / 2, scu_y, scu_h, EDGE_TYPE_LUMA);
            break;
        default:
            break;
        }
        
        if (scu_y && (scu_y & grad_mask) == 0 && (core->cu_pix_y != core->lcu_pix_y || seqhdr->cross_patch_loop_filter || patch_idx[lcu_idx] == patch_idx[lcu_idx - seqhdr->pic_width_in_lcu])) {
            com_df_set_edge_hor_inline(map, i_scu, refp, scu_x, scu_y, scu_w, EDGE_TYPE_ALL);  // UP
        }
        if (scu_x && (scu_x & grad_mask) == 0 && (core->cu_pix_x != core->lcu_pix_x || seqhdr->cross_patch_loop_filter || patch_idx[lcu_idx] == patch_idx[lcu_idx - 1])) {
            com_df_set_edge_ver_inline(map, i_scu, refp, scu_x, scu_y, scu_h, EDGE_TYPE_ALL);  // LEFT
        }
    }
}

void deblock_edge_luma_ver(pel *src, int stride, int Alpha, int Beta, int edge_flag)
{
    int i, line_size = 8;

    if ((edge_flag & 0x0101) != 0x0101) {
        line_size = 4;
    }
    if (!(edge_flag & 0x1)) {
        src += 4 * stride;
    }

    for (i = 0; i < line_size; i++, src += stride) {
        int L3 = src[-4];
        int L2 = src[-3];
        int L1 = src[-2];
        int L0 = src[-1];
        int R0 = src[0];
        int R1 = src[1];
        int R2 = src[2];
        int R3 = src[3];
        int abs0 = COM_ABS(R0 - L0);
        int flat_l = (COM_ABS(L1 - L0) < Beta) ? 2 : 0;
        int flat_r = (COM_ABS(R0 - R1) < Beta) ? 2 : 0;
        int fs;

        if (COM_ABS(L2 - L0) < Beta) {
            flat_l++;
        }
        if (COM_ABS(R0 - R2) < Beta) {
            flat_r++;
        }

        switch (flat_l + flat_r) {
        case 6:
            fs = (COM_ABS(R0 - R1) <= Beta / 4 && COM_ABS(L0 - L1) <= Beta / 4 && abs0 < Alpha) ? 4 : 3;
            break;
        case 5:
            fs = (R1 == R0 && L0 == L1) ? 3 : 2;
            break;
        case 4:
            fs = (flat_l == 2) ? 2 : 1;
            break;
        case 3:
            fs = (COM_ABS(L1 - R1) < Beta) ? 1 : 0;
            break;
        default:
            fs = 0;
            break;
        }

        switch (fs) {
        case 4:
            src[-3] = (pel)((((L3 + L2 + L1) << 1) + L0 + R0 + 4) >> 3);                          //L2
            src[-2] = (pel)((((L2 + L0 + L1) << 2) + L1 + R0 * 3 + 8) >> 4);                      //L1
            src[-1] = (pel)(((L2 + R1) * 3 + ((L1 + L0 + R0) << 3) + (L0 << 1) + 16) >> 5);       //L0
            src[ 0] = (pel)(((R2 + L1) * 3 + ((R1 + R0 + L0) << 3) + (R0 << 1) + 16) >> 5);       //R0
            src[ 1] = (pel)((((R2 + R1 + R0) << 2) + R1 + L0 * 3 + 8) >> 4);                      //R1
            src[ 2] = (pel)((((R3 + R2 + R1) << 1) + R0 + L0 + 4) >> 3);                          //R2
            break;
        case 3:
            src[-2] = (pel)((L2 * 3 + L1 * 8 + L0 * 4 + R0 + 8) >> 4);                           //L1
            src[-1] = (pel)((L2 + (L1 << 2) + (L0 << 2) + (L0 << 1) + (R0 << 2) + R1 + 8) >> 4); //L0
            src[ 0] = (pel)((L1 + (L0 << 2) + (R0 << 2) + (R0 << 1) + (R1 << 2) + R2 + 8) >> 4); //R0
            src[ 1] = (pel)((R2 * 3 + R1 * 8 + R0 * 4 + L0 + 8) >> 4);                           //R1
            break;
        case 2:
            src[-1] = (pel)(((L1 << 1) + L1 + (L0 << 3) + (L0 << 1) + (R0 << 1) + R0 + 8) >> 4);
            src[ 0] = (pel)(((L0 << 1) + L0 + (R0 << 3) + (R0 << 1) + (R1 << 1) + R1 + 8) >> 4);
            break;
        case 1:
            src[-1] = (pel)((L0 * 3 + R0 + 2) >> 2);
            src[ 0] = (pel)((R0 * 3 + L0 + 2) >> 2);
            break;
        default:
            break;
        }
    }
}

void deblock_edge_luma_hor(pel *src, int stride, int Alpha, int Beta, int edge_flag)
{
    int i, line_size = 8;
    int inc = stride;
    int inc2 = inc << 1;
    int inc3 = inc + inc2;
    int inc4 = inc + inc3;

    if ((edge_flag & 0x0101) != 0x0101) {
        line_size = 4;
    }
    if (!(edge_flag & 0x1)) {
        src += 4;
    }

    for (i = 0; i < line_size; i++, src++) {
        int L3 = src[-inc4];
        int L2 = src[-inc3];
        int L1 = src[-inc2];
        int L0 = src[-inc];
        int R0 = src[0];
        int R1 = src[inc];
        int R2 = src[inc2];
        int R3 = src[inc3];
        int abs0 = COM_ABS(R0 - L0);
        int flat_l = (COM_ABS(L1 - L0) < Beta) ? 2 : 0;
        int flat_r = (COM_ABS(R0 - R1) < Beta) ? 2 : 0;
        int fs;

        if (COM_ABS(L2 - L0) < Beta) {
            flat_l++;
        }
        if (COM_ABS(R0 - R2) < Beta) {
            flat_r++;
        }

        switch (flat_l + flat_r) {
        case 6:
            fs = (COM_ABS(R0 - R1) <= Beta / 4 && COM_ABS(L0 - L1) <= Beta / 4 && abs0 < Alpha) ? 4 : 3;
            break;
        case 5:
            fs = (R1 == R0 && L0 == L1) ? 3 : 2;
            break;
        case 4:
            fs = (flat_l == 2) ? 2 : 1;
            break;
        case 3:
            fs = (COM_ABS(L1 - R1) < Beta) ? 1 : 0;
            break;
        default:
            fs = 0;
            break;
        }

        switch (fs) {
        case 4:
            src[-inc3] = (pel)((((L3 + L2 + L1) << 1) + L0 + R0 + 4) >> 3);                         //L2
            src[-inc2] = (pel)((((L2 + L0 + L1) << 2) + L1 + R0 * 3 + 8) >> 4);                     //L1
            src[-inc ] = (pel)(((L2 + R1) * 3 + ((L1 + L0 + R0) << 3) + (L0 << 1) + 16) >> 5);      //L0
            src[ 0   ] = (pel)(((R2 + L1) * 3 + ((R1 + R0 + L0) << 3) + (R0 << 1) + 16) >> 5);      //R0
            src[ inc ] = (pel)((((R2 + R1 + R0) << 2) + R1 + L0 * 3 + 8) >> 4);                     //R1
            src[ inc2] = (pel)((((R3 + R2 + R1) << 1) + R0 + L0 + 4) >> 3);                         //R2
            break;
        case 3:
            src[-inc2] = (pel)((L2 * 3 + L1 * 8 + L0 * 4 + R0 + 8) >> 4);                           //L1
            src[-inc ] = (pel)((L2 + (L1 << 2) + (L0 << 2) + (L0 << 1) + (R0 << 2) + R1 + 8) >> 4); //L0
            src[ 0   ] = (pel)((L1 + (L0 << 2) + (R0 << 2) + (R0 << 1) + (R1 << 2) + R2 + 8) >> 4); //R0
            src[ inc ] = (pel)((R2 * 3 + R1 * 8 + R0 * 4 + L0 + 8) >> 4);                           //R2
            break;
        case 2:
            src[-inc] = (pel)(((L1 << 1) + L1 + (L0 << 3) + (L0 << 1) + (R0 << 1) + R0 + 8) >> 4);
            src[0] = (pel)(((L0 << 1) + L0 + (R0 << 3) + (R0 << 1) + (R1 << 1) + R1 + 8) >> 4);
            break;
        case 1:
            src[-inc] = (pel)((L0 * 3 + R0 + 2) >> 2);
            src[0] = (pel)((R0 * 3 + L0 + 2) >> 2);
            break;
        default:
            break;
        }
    }
}

void deblock_edge_chro_ver(pel *src, int stride, int alpha_u, int beta_u, int alpha_v, int beta_v, int edge_flag)
{
    int i, line_size = 4;
    int alpha[2] = { alpha_u, alpha_v };
    int beta[2] = { beta_u, beta_v };
    pel* p_src;
    int uv;

    if ((edge_flag & 0x0202) != 0x0202) {
        line_size = 2;
    }
    if (!(edge_flag & 0x2)) {
        src += 2 * stride;
    }

    for (uv = 0; uv < 2; uv++) {
        p_src = src + uv;
        for (i = 0; i < line_size; i++, p_src += stride) {
            int L2 = p_src[-6];
            int L1 = p_src[-4];
            int L0 = p_src[-2];
            int R0 = p_src[0];
            int R1 = p_src[2];
            int R2 = p_src[4];
            int delta_m = COM_ABS(R0 - L0);
            int delta_l = COM_ABS(L1 - L0);
            int delta_r = COM_ABS(R0 - R1);

            if ((delta_l < beta[uv]) && (delta_r < beta[uv])) {
                p_src[-2] = (pel)((L1 * 3 + L0 * 10 + R0 * 3 + 8) >> 4);                  // L0
                p_src[ 0] = (pel)((R1 * 3 + R0 * 10 + L0 * 3 + 8) >> 4);                  // R0
                if ((COM_ABS(L2 - L0) < beta[uv]) && (COM_ABS(R2 - R0) < beta[uv]) &&
                    delta_r <= beta[uv] / 4 && delta_l <= beta[uv] / 4 && delta_m < alpha[uv]) {
                    p_src[-4] = (pel)((L2 * 3 + L1 * 8 + L0 * 3 + R0 * 2 + 8) >> 4);      // L1
                    p_src[ 2] = (pel)((R2 * 3 + R1 * 8 + R0 * 3 + L0 * 2 + 8) >> 4);      // R1
                }
            }
        }
    }
}

void deblock_edge_chro_hor(pel *src, int stride, int alpha_u, int beta_u, int alpha_v, int beta_v, int edge_flag)
{
    int i, line_size = 4;
    int inc = stride;
    int inc2 = inc << 1;
    int inc3 = inc + inc2;
    int alpha[2] = { alpha_u, alpha_v };
    int beta[2] = { beta_u, beta_v };
    pel *p_src;
    int uv;

    if ((edge_flag & 0x0202) != 0x0202) {
        line_size = 2;
    }
    if (!(edge_flag & 0x2)) {
        src += 4;
    }

    for (uv = 0; uv < 2; uv++) {
        p_src = src + uv;
        for (i = 0; i < line_size; i++, p_src += 2) {
            int L2 = p_src[-inc3];
            int L1 = p_src[-inc2];
            int L0 = p_src[-inc];
            int R0 = p_src[0];
            int R1 = p_src[inc];
            int R2 = p_src[inc2];
            int delta_m = COM_ABS(R0 - L0);
            int delta_l = COM_ABS(L1 - L0);
            int delta_r = COM_ABS(R0 - R1);

            if ((delta_l < beta[uv]) && (delta_r < beta[uv])) {
                p_src[-inc] = (pel)((L1 * 3 + L0 * 10 + R0 * 3 + 8) >> 4);                  // L0
                p_src[0] = (pel)((R1 * 3 + R0 * 10 + L0 * 3 + 8) >> 4);                     // R0
                if ((COM_ABS(L2 - L0) < beta[uv]) && (COM_ABS(R2 - R0) < beta[uv]) && 
                    delta_r <= beta[uv] / 4 && delta_l <= beta[uv] / 4 && delta_m < alpha[uv]) {
                    p_src[-inc2] = (pel)((L2 * 3 + L1 * 8 + L0 * 3 + R0 * 2 + 8) >> 4);     // L1
                    p_src[inc] = (pel)((R2 * 3 + R1 * 8 + R0 * 3 + L0 * 2 + 8) >> 4);       // R1
                }
            }
        }
    }
}

static void uavs3d_always_inline com_df_cal_y_param(com_pic_header_t *pichdr, int qp_p, int qp_q, int qp_offset, int shift, int *alpha, int *beta)
{
    int qp = (qp_p + qp_q + 1 - qp_offset - qp_offset) >> 1;
    *alpha = com_tbl_deblock_alpha[COM_CLIP3(MIN_QUANT, MAX_QUANT_BASE, qp + pichdr->alpha_c_offset)] << shift;
    *beta  = com_tbl_deblock_beta [COM_CLIP3(MIN_QUANT, MAX_QUANT_BASE, qp + pichdr->beta_offset   )] << shift;
}

static void uavs3d_always_inline com_df_cal_c_param(com_pic_header_t *pichdr, int qp_p, int qp_q, int qp_offset, int shift, int *alpha, int *beta, int delta)
{
    int qp;
    int qp_uv_p = qp_p + delta - qp_offset;
    int qp_uv_q = qp_q + delta - qp_offset;

    if (qp_uv_p >= 0) {
        qp_uv_p = g_tbl_qp_chroma_adjust[COM_MIN(MAX_QUANT_BASE, qp_uv_p)];
    }
    if (qp_uv_q >= 0) {
        qp_uv_q = g_tbl_qp_chroma_adjust[COM_MIN(MAX_QUANT_BASE, qp_uv_q)];
    }
    qp_uv_p = COM_MAX(0, qp_uv_p + qp_offset);
    qp_uv_q = COM_MAX(0, qp_uv_q + qp_offset);

    qp = ((qp_uv_p + qp_uv_q + 1) >> 1) - qp_offset;

    *alpha = com_tbl_deblock_alpha[COM_CLIP3(MIN_QUANT, MAX_QUANT_BASE, qp + pichdr->alpha_c_offset)] << shift;
    *beta  = com_tbl_deblock_beta [COM_CLIP3(MIN_QUANT, MAX_QUANT_BASE, qp + pichdr->beta_offset   )] << shift;
}

void com_deblock_lcu_row(com_core_t *core, int lcu_y)
{
    com_seqh_t *seqhdr =  core->seqhdr;
    com_pic_header_t *pichdr =  core->pichdr;
    com_map_t *map   = &core->map;
    com_pic_t * pic  =  core->pic;
    int pix_y, pix_x;
    int i_lcu = seqhdr->pic_width_in_lcu;
    int i_scu = seqhdr->i_scu;
    int s_l = pic->stride_luma;
    int s_c = pic->stride_chroma;
    int shift = seqhdr->bit_depth_internal - 8;
    int alpha_hor_y, beta_hor_y, alpha_ver_y, beta_ver_y;
    int alpha_hor_u, beta_hor_u, alpha_ver_u, beta_ver_u;
    int alpha_hor_v, beta_hor_v, alpha_ver_v, beta_ver_v;
    int fix_qp = pichdr->fixed_picture_qp_flag;
    int pix_start_y = lcu_y * seqhdr->max_cuwh;
    int pix_end_y = COM_MIN(pix_start_y + seqhdr->max_cuwh, seqhdr->pic_height);
    pel *src_l = pic->y + pix_start_y * s_l;
    pel *src_uv = pic->uv + (pix_start_y >> 1) * s_c;
    int qp_offset = seqhdr->bit_depth_2_qp_offset;

    if (fix_qp) {
        int qp = map->map_qp[lcu_y * i_lcu];
        com_df_cal_y_param(pichdr, qp, qp, qp_offset, shift, &alpha_hor_y, &beta_hor_y);
        com_df_cal_c_param(pichdr, qp, qp, qp_offset, shift, &alpha_hor_u, &beta_hor_u, pichdr->chroma_quant_param_delta_cb);
        com_df_cal_c_param(pichdr, qp, qp, qp_offset, shift, &alpha_hor_v, &beta_hor_v, pichdr->chroma_quant_param_delta_cr);
        com_df_cal_y_param(pichdr, qp, qp, qp_offset, shift, &alpha_ver_y, &beta_ver_y);
        com_df_cal_c_param(pichdr, qp, qp, qp_offset, shift, &alpha_ver_u, &beta_ver_u, pichdr->chroma_quant_param_delta_cb);
        com_df_cal_c_param(pichdr, qp, qp, qp_offset, shift, &alpha_ver_v, &beta_ver_v, pichdr->chroma_quant_param_delta_cr);
    }

    for (pix_y = pix_start_y; pix_y < pix_end_y; pix_y += LOOPFILTER_GRID) {
        u8 *edge_flag = map->map_edge + (pix_y >> MIN_CU_LOG2) * i_scu;
        s8 *map_cur = map->map_qp + (pix_y >> seqhdr->log2_max_cuwh) * i_lcu;
        s8 *map_top = map->map_qp + ((pix_y - LOOPFILTER_GRID) >> seqhdr->log2_max_cuwh) * i_lcu;

        if (!fix_qp) {
            int qp_p = map_cur[0];
            if (pix_y) {
                int qp_q = map_top[0];
                com_df_cal_y_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_hor_y, &beta_hor_y);
                com_df_cal_c_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_hor_u, &beta_hor_u, pichdr->chroma_quant_param_delta_cb);
                com_df_cal_c_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_hor_v, &beta_hor_v, pichdr->chroma_quant_param_delta_cr);
            }
            {
                int qp_q = qp_p;
                com_df_cal_y_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_ver_y, &beta_ver_y);
                com_df_cal_c_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_ver_u, &beta_ver_u, pichdr->chroma_quant_param_delta_cb);
                com_df_cal_c_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_ver_v, &beta_ver_v, pichdr->chroma_quant_param_delta_cr);
            }
        }
        for (pix_x = 0; pix_x < seqhdr->pic_width; pix_x += LOOPFILTER_GRID) {
            int pix_x_ver = pix_x + LOOPFILTER_GRID;

            if (!fix_qp) {
                s8 *map_qp = map_cur + (pix_x >> seqhdr->log2_max_cuwh);
                int qp_p = map_qp[0];

                if (pix_x % seqhdr->max_cuwh == 0 && pix_x && pix_y) {
                    int qp_q = map_top[pix_x >> seqhdr->log2_max_cuwh];
                    com_df_cal_y_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_hor_y, &beta_hor_y);
                    com_df_cal_c_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_hor_u, &beta_hor_u, pichdr->chroma_quant_param_delta_cb);
                    com_df_cal_c_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_hor_v, &beta_hor_v, pichdr->chroma_quant_param_delta_cr);
                } 
                if ((pix_x % seqhdr->max_cuwh == 0 && pix_x) || (pix_x_ver % seqhdr->max_cuwh == 0 && pix_x_ver < seqhdr->pic_width)) {
                    int qp_q;
                    if (pix_x_ver % seqhdr->max_cuwh == 0) {
                        qp_q = map_qp[1];
                    } else {
                        qp_q = map_qp[0];
                    }
                    com_df_cal_y_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_ver_y, &beta_ver_y);
                    com_df_cal_c_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_ver_u, &beta_ver_u, pichdr->chroma_quant_param_delta_cb);
                    com_df_cal_c_param(pichdr, qp_p, qp_q, qp_offset, shift, &alpha_ver_v, &beta_ver_v, pichdr->chroma_quant_param_delta_cr);
                }
            }

            int edge = ((edge_flag[2]) | ((edge_flag[2 + i_scu]) << 8)) & 0x0303;

            if (edge) {
                uavs3d_funs_handle.deblock_luma[0](src_l + pix_x_ver, s_l, alpha_ver_y, beta_ver_y, edge);

                if (pix_x_ver % LOOPFILTER_GRID_C == 0 && (edge & 0x0202)) {
                    uavs3d_funs_handle.deblock_chroma[0](src_uv + pix_x_ver, s_c, alpha_ver_u, beta_ver_u, alpha_ver_v, beta_ver_v, edge);
                }
            }
            
            edge = (M16(edge_flag) >> 2) & 0x0303;

            if (edge) {
                uavs3d_funs_handle.deblock_luma[1](src_l + pix_x, s_l, alpha_hor_y, beta_hor_y, edge);

                if (pix_y % LOOPFILTER_GRID_C == 0 && (edge & 0x0202)) {
                    uavs3d_funs_handle.deblock_chroma[1](src_uv + pix_x, s_c, alpha_hor_u, beta_hor_u, alpha_hor_v, beta_hor_v, edge);
                }
            }
            
            edge_flag += 2;
        }

        src_l  += s_l *  LOOPFILTER_GRID;
        src_uv += s_c * (LOOPFILTER_GRID / 2);
    }
}

void uavs3d_funs_init_deblock_c()
{
    uavs3d_funs_handle.deblock_luma[0] = deblock_edge_luma_ver;
    uavs3d_funs_handle.deblock_luma[1] = deblock_edge_luma_hor;
    uavs3d_funs_handle.deblock_chroma[0] = deblock_edge_chro_ver;
    uavs3d_funs_handle.deblock_chroma[1] = deblock_edge_chro_hor;
}
