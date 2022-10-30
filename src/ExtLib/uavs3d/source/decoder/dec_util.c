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

#include "dec_type.h"

static void uavs3d_always_inline copy_motion(com_motion_t *motion_dst, s16 mv_new[REFP_NUM][MV_D], s8 refi_new[REFP_NUM])
{
    CP64(motion_dst->mv, mv_new);
    CP16(motion_dst->ref_idx, refi_new);
}

static int uavs3d_always_inline same_motion(s16 mv1[REFP_NUM][MV_D], s8 ref_idx1[REFP_NUM], s16 mv2[REFP_NUM][MV_D], s8 ref_idx2[REFP_NUM])
{
    return (M16(ref_idx1) == M16(ref_idx2) &&
            (ref_idx1[PRED_L0] == REFI_INVALID || M32(mv1[PRED_L0]) == M32(mv2[PRED_L0])) &&
            (ref_idx1[PRED_L1] == REFI_INVALID || M32(mv1[PRED_L1]) == M32(mv2[PRED_L1])));
}

static int  uavs3d_always_inline get_colocal_scup(int scup, int i_scu, int pic_width_in_scu, int pic_height_in_scu, int *scu_y)
{
    const int mask = (-1) ^ 3;
    int bx = scup % i_scu;
    int by = scup / i_scu;
    int xpos = (bx & mask) + 2;
    int ypos = (by & mask) + 2;

    if (ypos >= pic_height_in_scu) {
        ypos = ((by & mask) + pic_height_in_scu) >> 1;
    }
    if (xpos >= pic_width_in_scu) {
        xpos = ((bx & mask) + pic_width_in_scu) >> 1;
    }
    *scu_y = ypos;

    return ypos * i_scu + xpos;
}

static void uavs3d_inline scaling_mv(int dist_cur, int dist_neb, s16 mvp[MV_D], s16 mv[MV_D])
{
    M32(mv) = 0;

    if (M32(mvp)) {
        if (dist_neb == dist_cur && (1 << MV_SCALE_PREC) % dist_neb == 0) {
            M32(mv) = M32(mvp);
        } else {
            const int offset = 1 << (MV_SCALE_PREC - 1);
            int ratio = offset / dist_neb * dist_cur << 1; // note: divide first for constraining bit-depth

            if (mvp[MV_X]) {
                s64 tmp_mv = (s64)mvp[MV_X] * ratio;
                s64 mask = tmp_mv >> 63;
                tmp_mv = (mask ^ tmp_mv) - mask;
                tmp_mv = (mask ^ ((tmp_mv + offset) >> MV_SCALE_PREC)) - mask;
                mv[MV_X] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, tmp_mv);
            }
            if (mvp[MV_Y]) {
                s64 tmp_mv = (s64)mvp[MV_Y] * ratio;
                s64 mask = tmp_mv >> 63;
                tmp_mv = (mask ^ tmp_mv) - mask;
                tmp_mv = (mask ^ ((tmp_mv + offset) >> MV_SCALE_PREC)) - mask;
                mv[MV_Y] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, tmp_mv);
            }
        }
    }
}

static void uavs3d_always_inline scaling_mv_amvr(int dist_cur, int dist_neb, s16 mvp[MV_D], s16 mv[MV_D], int amvr_idx)
{
    M32(mv) = 0;

    if (M32(mvp)) {
        if (dist_neb == dist_cur && (1 << MV_SCALE_PREC) % dist_neb == 0) {
            int add = (amvr_idx > 0) ? (1 << (amvr_idx - 1)) : 0;
            int amvr_mask = ~((1 << amvr_idx) - 1);

            if (mvp[MV_X]) {
                s64 tmp_mv = mvp[MV_X];
                s64 mask = tmp_mv >> 63;
                tmp_mv = (mask ^ tmp_mv) - mask;
                tmp_mv = (mask ^ ((tmp_mv + add) & amvr_mask)) - mask;
                mv[MV_X] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, tmp_mv);
            }
            if (mvp[MV_Y]) {
                s64 tmp_mv = mvp[MV_Y];
                s64 mask = tmp_mv >> 63;
                tmp_mv = (mask ^ tmp_mv) - mask;
                tmp_mv = (mask ^ ((tmp_mv + add) & amvr_mask)) - mask;
                mv[MV_Y] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, tmp_mv);
            }

        } else {
            int offset = 1 << (MV_SCALE_PREC - 1);
            int ratio = offset / dist_neb * dist_cur << 1; // note: divide first for constraining bit-depth
            int add = (amvr_idx > 0) ? (1 << (amvr_idx - 1)) : 0;
            int amvr_mask = ~((1 << amvr_idx) - 1);

            offset += add << MV_SCALE_PREC;

            if (mvp[MV_X]) {
                s64 tmp_mv = (s64)mvp[MV_X] * ratio;
                s64 mask = tmp_mv >> 63;
                tmp_mv = (mask ^ tmp_mv) - mask;
                tmp_mv = (mask ^ (((tmp_mv + offset) >> MV_SCALE_PREC) & amvr_mask)) - mask;
                mv[MV_X] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, tmp_mv);
            }
            if (mvp[MV_Y]) {
                s64 tmp_mv = (s64)mvp[MV_Y] * ratio;
                s64 mask = tmp_mv >> 63;
                tmp_mv = (mask ^ tmp_mv) - mask;
                tmp_mv = (mask ^ (((tmp_mv + offset) >> MV_SCALE_PREC) & amvr_mask)) - mask;
                mv[MV_Y] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, tmp_mv);
            }
        }
    }
}

static void uavs3d_always_inline com_mv_rounding_s16(s16 mvp[MV_D], s32 shift)
{
    int add = shift ? 1 << (shift - 1) : 0;

    if (mvp[0]) {
        int tmp_mv = mvp[0];
        int mask = tmp_mv >> 31;
        tmp_mv = (mask ^ tmp_mv) - mask;
        tmp_mv = (((tmp_mv + add) >> shift) << shift);
        tmp_mv = (mask ^ tmp_mv) - mask;
        mvp[0] = (CPMV)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, tmp_mv);
    }
    if (mvp[1]) {
        int tmp_mv = mvp[1];
        int mask = tmp_mv >> 31;
        tmp_mv = (mask ^ tmp_mv) - mask;
        tmp_mv = (((tmp_mv + add) >> shift) << shift);
        tmp_mv = (mask ^ tmp_mv) - mask;
        mvp[1] = (CPMV)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, tmp_mv);
    }
}

static void uavs3d_always_inline com_mv_rounding_s32(s32 mv[MV_D], s16 mvp[MV_D], s32 shift, int add)
{    
    M64(mv) = 0;

    if (mvp[0]) {
        int tmp_mv = mvp[0] << 2;
        int mask = tmp_mv >> 31;
        tmp_mv = (mask ^ tmp_mv) - mask;
        tmp_mv = (((tmp_mv + add) >> shift) << shift);
        tmp_mv = (mask ^ tmp_mv) - mask;
        mv[0] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, tmp_mv);
    }
    if (mvp[1]) {
        int tmp_mv = mvp[1] << 2;
        int mask = tmp_mv >> 31;
        tmp_mv = (mask ^ tmp_mv) - mask;
        tmp_mv = (((tmp_mv + add) >> shift) << shift);
        tmp_mv = (mask ^ tmp_mv) - mask;
        mv[1] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, tmp_mv);
    }
}

static void uavs3d_always_inline com_mv_rounding_s32_shift7(s32 hor, s32 ver, s32 * rounded_hor, s32 * rounded_ver)
{
    if (hor == 0) {
        *rounded_hor = 0;
    } else {
        int mask = hor >> 31;
        hor = (mask ^ hor) - mask;
        hor = (hor + 64) >> 7;
        *rounded_hor = (mask ^ hor) - mask;
    }

    if (ver == 0) {
        *rounded_ver = 0;
    } else {
        int mask = ver >> 31;
        ver = (mask ^ ver) - mask;
        ver = (ver + 64) >> 7;
        *rounded_ver = (mask ^ ver) - mask;
    }
}


static void uavs3d_always_inline com_mv_rounding_affine(s32 hor, s32 ver, s32 * mv1_x, s32 * mv1_y, s16 * mv2_x, s16 * mv2_y)
{
    // 1/16 precision, 18 bits, for MC;  1/4 precision, 16 bits, for mv storage

    const int add1 = 1 << (7 - 1);
    const int add2 = 1 << (2 - 1);

    if (hor == 0) {
        *mv1_x = *mv2_x = 0;
    } else if (hor > 0) {
        *mv1_x = (hor + add1) >> 7;         
        *mv1_x = COM_MIN(COM_INT18_MAX, *mv1_x); 
        int val = ((*mv1_x + add2) >> 2);
        *mv2_x = COM_MIN(COM_INT16_MAX, val);
    } else {
        int t = ((-hor + add1) >> 7);
        *mv1_x = COM_MAX(COM_INT18_MIN, -t); 
        int val = -((t + add2) >> 2);
        *mv2_x = COM_MAX(COM_INT16_MIN, val);
    }

    if (ver == 0) {
        *mv1_y = *mv2_y = 0;
    } else if (ver > 0) {
        *mv1_y = (ver + add1) >> 7;         
        *mv1_y = COM_MIN(COM_INT18_MAX, *mv1_y); 
        int val = ((*mv1_y + add2) >> 2);
        *mv2_y = COM_MIN(COM_INT16_MAX, val);
    } else {
        int t = ((-ver + add1) >> 7);  
        *mv1_y = COM_MAX(COM_INT18_MIN, -t); 
        int val = -((t + add2) >> 2);
        *mv2_y = COM_MAX(COM_INT16_MIN, val);
    }
}

static void uavs3d_always_inline check_umve_motion_availability(int scup, int cu_width, int cu_height, int i_scu, int neighbor[5], int valid[5], com_scu_t * map_scu, s16(*map_mv)[REFP_NUM][MV_D], s8(*map_refi)[REFP_NUM])
{
    int cu_width_in_scu = cu_width >> MIN_CU_LOG2;
    int cu_height_in_scu = cu_height >> MIN_CU_LOG2;
    int tmp_valid[5];

    neighbor[0] = scup + i_scu * (cu_height_in_scu - 1) - 1; //F
    neighbor[1] = scup - i_scu + cu_width_in_scu - 1;        //G
    neighbor[2] = scup - i_scu + cu_width_in_scu;            //C
    neighbor[3] = scup - 1;                                  //A
    neighbor[4] = scup - i_scu - 1;                          //D

    valid[0] = tmp_valid[0] = map_scu[neighbor[0]].inter;
    valid[1] = tmp_valid[1] = map_scu[neighbor[1]].inter;
    valid[2] = tmp_valid[2] = map_scu[neighbor[2]].inter;
    valid[3] = tmp_valid[3] = map_scu[neighbor[3]].inter;
    valid[4] = tmp_valid[4] = map_scu[neighbor[4]].inter;

    if (tmp_valid[0] && tmp_valid[1]) {
        valid[1] = !same_motion(map_mv[neighbor[1]], map_refi[neighbor[1]], map_mv[neighbor[0]], map_refi[neighbor[0]]);
    }
    if (tmp_valid[1] && tmp_valid[2]) {
        valid[2] = !same_motion(map_mv[neighbor[1]], map_refi[neighbor[1]], map_mv[neighbor[2]], map_refi[neighbor[2]]);
    }
    if (tmp_valid[0] && tmp_valid[3]) {
        valid[3] = !same_motion(map_mv[neighbor[3]], map_refi[neighbor[3]], map_mv[neighbor[0]], map_refi[neighbor[0]]);
    }
    if (tmp_valid[4]) {
        valid[4] = (!tmp_valid[3] || !same_motion(map_mv[neighbor[4]], map_refi[neighbor[4]], map_mv[neighbor[3]], map_refi[neighbor[3]])) &&
                        (!tmp_valid[1] || !same_motion(map_mv[neighbor[4]], map_refi[neighbor[4]], map_mv[neighbor[1]], map_refi[neighbor[1]]));
    }
}

static void uavs3d_always_inline get_mvp_hmvp(com_core_t *core, com_motion_t motion, int lidx, s8 cur_refi, s16 mvp[MV_D], int amvr_idx)
{
    com_ref_pic_t(*refp)[REFP_NUM] = core->refp;
    s8  refi_hmvp = motion.ref_idx[lidx];
    int dist_cur = refp[cur_refi][lidx].dist;

    if (refi_hmvp == REFI_INVALID) {
        lidx = !lidx;
        refi_hmvp = motion.ref_idx[lidx];
    }
    scaling_mv_amvr(dist_cur, refp[refi_hmvp][lidx].dist, motion.mv[lidx], mvp, amvr_idx);
}

static void get_mvp_default(com_core_t *core, int lidx, s8 cur_refi, u8 amvr_idx, s16 mvp[MV_D])
{
    com_map_t *map = &core->map;
    int scup = core->cu_scup;
    int cu_width = core->cu_width;
    int i_scu = core->seqhdr->i_scu;
    com_ref_pic_t(*refp)[REFP_NUM] = core->refp;
    s16(*map_mv)[REFP_NUM][MV_D] = map->map_mv;
    s8(*map_refi)[REFP_NUM] = map->map_refi;
    com_scu_t* map_scu = map->map_scu;
    int mvPredType = MVPRED_xy_MIN;
    int rFrameL, rFrameU, rFrameUR;
    int neighbor[NUM_SPATIAL_MV];
    s16 MVPs[NUM_SPATIAL_MV][MV_D] = { 0 };
    int dist_cur = refp[cur_refi][lidx].dist;

    neighbor[0] = scup - 1;                                     // A
    neighbor[1] = scup - i_scu;                                 // B
    neighbor[2] = scup - i_scu + (cu_width >> MIN_CU_LOG2);     // C
    
    rFrameL  = map_scu[neighbor[0]].coded ? map_refi[neighbor[0]][lidx] : REFI_INVALID;
    rFrameU  = map_scu[neighbor[1]].coded ? map_refi[neighbor[1]][lidx] : REFI_INVALID;
    rFrameUR = map_scu[neighbor[2]].coded ? map_refi[neighbor[2]][lidx] : REFI_INVALID;

    if (REFI_IS_VALID(rFrameL)) {
        scaling_mv(dist_cur, refp[rFrameL][lidx].dist, map_mv[neighbor[0]][lidx], MVPs[0]);
    }
    if (REFI_IS_VALID(rFrameU)) {
        scaling_mv(dist_cur, refp[rFrameU][lidx].dist, map_mv[neighbor[1]][lidx], MVPs[1]);
    }
    if (REFI_IS_VALID(rFrameUR)) {
        scaling_mv(dist_cur, refp[rFrameUR][lidx].dist, map_mv[neighbor[2]][lidx], MVPs[2]);
    } else {
        neighbor[2] = scup - i_scu - 1;              // D
        rFrameUR = map_scu[neighbor[2]].coded ? map_refi[neighbor[2]][lidx] : REFI_INVALID;

        if (REFI_IS_VALID(rFrameUR)) {
            scaling_mv(dist_cur, refp[rFrameUR][lidx].dist, map_mv[neighbor[2]][lidx], MVPs[2]);
        }
    }
    if ((rFrameL != REFI_INVALID) && (rFrameU == REFI_INVALID) && (rFrameUR == REFI_INVALID)) {
        mvPredType = MVPRED_L;
    }
    else if ((rFrameL == REFI_INVALID) && (rFrameU != REFI_INVALID) && (rFrameUR == REFI_INVALID)) {
        mvPredType = MVPRED_U;
    }
    else if ((rFrameL == REFI_INVALID) && (rFrameU == REFI_INVALID) && (rFrameUR != REFI_INVALID)) {
        mvPredType = MVPRED_UR;
    }

    switch (mvPredType) {
    case MVPRED_xy_MIN:
        for (int hv = 0; hv < MV_D; hv++) {
            s32 mva = (s32)MVPs[0][hv], mvb = (s32)MVPs[1][hv], mvc = (s32)MVPs[2][hv];

            if ((mva < 0 && mvb > 0 && mvc > 0) || (mva > 0 && mvb < 0 && mvc < 0)) {
                mvp[hv] = (s16)((mvb + mvc) / 2);
            }
            else if ((mvb < 0 && mva > 0 && mvc > 0) || (mvb > 0 && mva < 0 && mvc < 0)) {
                mvp[hv] = (s16)((mvc + mva) / 2);
            }
            else if ((mvc < 0 && mva > 0 && mvb > 0) || (mvc > 0 && mva < 0 && mvb < 0)) {
                mvp[hv] = (s16)((mva + mvb) / 2);
            }
            else {
                s32 mva_ext = abs(mva - mvb);
                s32 mvb_ext = abs(mvb - mvc);
                s32 mvc_ext = abs(mvc - mva);
                s32 pred_vec = min(mva_ext, min(mvb_ext, mvc_ext));
                if (pred_vec == mva_ext) {
                    mvp[hv] = (s16)((mva + mvb) / 2);
                }
                else if (pred_vec == mvb_ext) {
                    mvp[hv] = (s16)((mvb + mvc) / 2);
                }
                else {
                    mvp[hv] = (s16)((mvc + mva) / 2);
                }
            }
        }
        break;
    case MVPRED_L:
        CP32(mvp, MVPs[0]);
        break;
    case MVPRED_U:
        CP32(mvp, MVPs[1]);
        break;
    case MVPRED_UR:
        CP32(mvp, MVPs[2]);
        break;
    default:
        uavs3d_assert(0);
        break;
    }
  
    com_mv_rounding_s16(mvp, amvr_idx);
}

void dec_derive_mvp(com_core_t *core, int ref_list, int emvp_flag, int mvr_idx, s16 mvp[MV_D])
{
    int ref_idx = core->refi[ref_list];
    int hmvp_cands_cnt = core->hmvp_cands_cnt;
    com_motion_t *hmvp_cands_list = core->hmvp_cands_list;
 
    if (!emvp_flag) {
        get_mvp_default(core, ref_list, ref_idx, mvr_idx, mvp);
    } else {
        if (hmvp_cands_cnt == 0) {
            M32(mvp) = 0;
        }
        else if (hmvp_cands_cnt < mvr_idx + 1) {
            com_motion_t motion = hmvp_cands_list[hmvp_cands_cnt - 1];
            get_mvp_hmvp(core, motion, ref_list, ref_idx, mvp, mvr_idx);
        }
        else {
            com_motion_t motion = hmvp_cands_list[hmvp_cands_cnt - 1 - mvr_idx];
            get_mvp_hmvp(core, motion, ref_list, ref_idx, mvp, mvr_idx);
        }
    }
}

static void derive_mhb_skip_motions(com_core_t* core, com_motion_t *motion_list)
{
    int scup = core->cu_scup;
    int cu_width = core->cu_width;
    int cu_height = core->cu_height;
    int i_scu = core->seqhdr->i_scu;
    com_map_t *map = &core->map;
    com_scu_t *map_scu = map->map_scu;
    s16(*map_mv)[REFP_NUM][MV_D] = map->map_mv;
    s8(*map_refi)[REFP_NUM] = map->map_refi;
    int cu_width_in_scu = cu_width >> MIN_CU_LOG2;
    int cu_height_in_scu = cu_height >> MIN_CU_LOG2;
    int k;
    int last_bi_idx = 0;
    u8 L0_motion_found = 0, L1_motion_found = 0, BI_motion_found = 0;
    int neighbor[NUM_SKIP_SPATIAL_MV];
    int valid[NUM_SKIP_SPATIAL_MV];
    s8 refi[REFP_NUM];

    memset(motion_list, 0, sizeof(com_motion_t) * PRED_DIR_NUM);

    motion_list[2].ref_idx[REFP_1] = REFI_INVALID;
    motion_list[1].ref_idx[REFP_0] = REFI_INVALID;

    neighbor[0] = scup + (cu_height_in_scu - 1) * i_scu - 1;
    valid   [0] = map_scu[neighbor[0]].inter;

    neighbor[1] = scup - i_scu + cu_width_in_scu - 1;
    valid   [1] = map_scu[neighbor[1]].inter;

    neighbor[2] = scup - i_scu + cu_width_in_scu;
    valid   [2] = map_scu[neighbor[2]].inter;

    neighbor[3] = scup - 1;
    valid   [3] = map_scu[neighbor[3]].inter;

    neighbor[4] = scup - i_scu;
    valid   [4] = map_scu[neighbor[4]].inter;

    neighbor[5] = scup - i_scu - 1;
    valid   [5] = map_scu[neighbor[5]].inter;

    for (k = 0; k < NUM_SKIP_SPATIAL_MV; k++) {
        if (valid[k]) {
            refi[REFP_0] = REFI_IS_VALID(map_refi[neighbor[k]][REFP_0]) ? map_refi[neighbor[k]][REFP_0] : REFI_INVALID;
            refi[REFP_1] = REFI_IS_VALID(map_refi[neighbor[k]][REFP_1]) ? map_refi[neighbor[k]][REFP_1] : REFI_INVALID;

            if (REFI_IS_VALID(refi[REFP_0]) && !REFI_IS_VALID(refi[REFP_1]) && !L0_motion_found) {
                L0_motion_found = 1;
                CP32(motion_list[2].mv[REFP_0], map_mv[neighbor[k]][REFP_0]);
                motion_list[2].ref_idx[REFP_0] = refi[REFP_0];
            }

            if (!REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1]) && !L1_motion_found) {
                L1_motion_found = 1;
                CP32(motion_list[1].mv[REFP_1], map_mv[neighbor[k]][REFP_1]);
                motion_list[1].ref_idx[REFP_1] = refi[REFP_1];
            }
 
            if (REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1])) {
                if (!BI_motion_found) {
                    CP32(motion_list[0].mv[REFP_0], map_mv[neighbor[k]][REFP_0]);
                    CP32(motion_list[0].mv[REFP_1], map_mv[neighbor[k]][REFP_1]);
                    motion_list[0].ref_idx[REFP_0] = refi[REFP_0];
                    motion_list[0].ref_idx[REFP_1] = refi[REFP_1];
                }
                BI_motion_found = 1;
                last_bi_idx = k;
            }
        }
    }

    if (L0_motion_found && L1_motion_found && !BI_motion_found) {
        CP32(motion_list[0].mv[REFP_0], motion_list[2].mv[REFP_0]);
        CP32(motion_list[0].mv[REFP_1], motion_list[1].mv[REFP_1]);
        motion_list[0].ref_idx[REFP_0]  = motion_list[2].ref_idx[REFP_0];
        motion_list[0].ref_idx[REFP_1]  = motion_list[1].ref_idx[REFP_1];
    }

    if (!L0_motion_found && BI_motion_found) {
        CP32(motion_list[2].mv[REFP_0], map_mv[neighbor[last_bi_idx]][REFP_0]);
        motion_list[2].ref_idx[REFP_0] = map_refi[neighbor[last_bi_idx]][REFP_0];
    }

    if (!L1_motion_found && BI_motion_found) {
        CP32(motion_list[1].mv[REFP_1], map_mv[neighbor[last_bi_idx]][REFP_1]);
        motion_list[1].ref_idx[REFP_1] = map_refi[neighbor[last_bi_idx]][REFP_1];
    }
}

static void uavs3d_always_inline get_col_mv_from_list0(com_ref_pic_t *p, int scup_co, s16 mvp[REFP_NUM][MV_D])
{
    int refi = p->map_refi[scup_co][REFP_0];

    scaling_mv(p->dist, p->pic->list_dist[refi], p->map_mv[scup_co][REFP_0], mvp[REFP_0]);
}

static void uavs3d_always_inline get_col_mv(com_ref_pic_t refp[REFP_NUM], int scup_co, s16 mvp[REFP_NUM][MV_D])
{
    com_ref_pic_t *p = &refp[REFP_1];
    int refi = p->map_refi[scup_co][REFP_0];
    s16 dist_col;
    
    uavs3d_assert(refi != REFI_INVALID);
    if (refi == REFI_INVALID) {
        refi = 0;
    }

    dist_col = p->pic->list_dist[refi];

    scaling_mv(refp[REFP_0].dist, dist_col, p->map_mv[scup_co][0], mvp[REFP_0]);
    scaling_mv(refp[REFP_1].dist, dist_col, p->map_mv[scup_co][0], mvp[REFP_1]);
}

static void derive_temporal_motions(com_core_t* core, s16 pmv_temp[REFP_NUM][MV_D], s8 refi_temp[REFP_NUM])
{
    int scup = core->cu_scup;
    com_seqh_t *seqhdr = core->seqhdr;
    int scu_y;
    int scup_co = get_colocal_scup(scup, seqhdr->i_scu, seqhdr->pic_width_in_scu, seqhdr->pic_height_in_scu, &scu_y);

    M16(refi_temp) = 0;

    if (core->slice_type == SLICE_P) {
        com_ref_pic_t *refp = &core->refp[0][REFP_0];
        refi_temp[REFP_1] = -1;
        uavs3d_check_ref_avaliable(refp->pic, scu_y << MIN_CU_LOG2);

        if (REFI_IS_VALID(refp->map_refi[scup_co][REFP_0])) {
            get_col_mv_from_list0(refp, scup_co, pmv_temp);
        } else {
            M32(pmv_temp) = 0;
        }
    } else {
        com_ref_pic_t *refp = &core->refp[0][REFP_1];
        uavs3d_check_ref_avaliable(refp->pic, scu_y << MIN_CU_LOG2);

        if (!REFI_IS_VALID(refp->map_refi[scup_co][REFP_0])) {
            get_mvp_default(core, REFP_0, 0, 0, pmv_temp[REFP_0]);
            get_mvp_default(core, REFP_1, 0, 0, pmv_temp[REFP_1]);
        } else {
            get_col_mv(core->refp[0], scup_co, pmv_temp);
        }
    }
}

static int derive_umve_base_motions(com_core_t *core, int base_idx, s16 umve_base_pmv[REFP_NUM][MV_D], s8 umve_base_refi[REFP_NUM])
{
    com_map_t *map = &core->map;
    int scup = core->cu_scup;
    int cu_width = core->cu_width;
    int cu_height = core->cu_height;
    int i_scu = core->seqhdr->i_scu;
    com_scu_t * map_scu = map->map_scu;
    s16(*map_mv)[REFP_NUM][MV_D] = map->map_mv;
    s8(*map_refi)[REFP_NUM] = map->map_refi;
    int i;
    int cnt = 0;

    int neighbor[5];
    int valid[5];

    base_idx = COM_MIN(base_idx, UMVE_BASE_NUM - 1);

    umve_base_refi[0] = umve_base_refi[1] = REFI_INVALID;

    check_umve_motion_availability(scup, cu_width, cu_height, i_scu, neighbor, valid, map_scu, map_mv, map_refi);

    for (i = 0; i < 5 && cnt <= base_idx; i++) {
        if (valid[i]) {
            if (cnt == base_idx) {
                if (REFI_IS_VALID(map_refi[neighbor[i]][REFP_0])) {
                    CP32(umve_base_pmv[REFP_0], map_mv[neighbor[i]][REFP_0]);
                    umve_base_refi[REFP_0] = map_refi[neighbor[i]][REFP_0];
                }
                if (REFI_IS_VALID(map_refi[neighbor[i]][REFP_1])) {
                    CP32(umve_base_pmv[REFP_1], map_mv[neighbor[i]][REFP_1]);
                    umve_base_refi[REFP_1] = map_refi[neighbor[i]][REFP_1];
                }
            }
            cnt++;
        }
    }
    if (cnt <= base_idx) {
        if (cnt == base_idx) {
            return 1;
        }
        cnt++;
    }
    if (cnt <= base_idx) {
        umve_base_refi[REFP_0] = 0;
        umve_base_refi[REFP_1] = REFI_INVALID;
        M32(umve_base_pmv[REFP_0]) = 0;
    }
    return 0;
}

static void derive_umve_final_motions(com_core_t *core, int umve_idx,  s16 umve_base_pmv[REFP_NUM][MV_D], s8 umve_base_refi[REFP_NUM])
{
    s16(*umve_final_pmv)[MV_D] = core->mv;
    s8 *umve_final_refi = core->refi;
    com_ref_pic_t(*refp)[REFP_NUM] = core->refp;
    int base_idx = umve_idx / UMVE_MAX_REFINE_NUM;
    int refine_step = (umve_idx - (base_idx * UMVE_MAX_REFINE_NUM)) / 4;
    int direction = umve_idx - base_idx * UMVE_MAX_REFINE_NUM - refine_step * 4;
    s32 mv_offset[REFP_NUM][MV_D];
    int ref_mvd0, ref_mvd1;

    const int ref_mvd_cands[5] = { 1, 2, 4, 8, 16 };
    const int refi0 = umve_base_refi[REFP_0];
    const int refi1 = umve_base_refi[REFP_1];

    ref_mvd0 = ref_mvd_cands[refine_step];
    ref_mvd1 = ref_mvd_cands[refine_step];

    umve_final_refi[REFP_0] = REFI_INVALID;
    umve_final_refi[REFP_1] = REFI_INVALID;

    if (REFI_IS_VALID(refi0) && REFI_IS_VALID(refi1)) {
        const int dist0 = refp[refi0][REFP_0].dist << 1;
        const int dist1 = refp[refi1][REFP_1].dist << 1;
        int list0_weight, list1_weight;
        int list0_sign = 1;
        int list1_sign = 1;

        if (abs(dist1) >= abs(dist0)) {
            list0_weight = (1 << MV_SCALE_PREC) / (abs(dist1)) * abs(dist0);
            list1_weight = 1 << MV_SCALE_PREC;
            if (dist0 * dist1 < 0) {
                list0_sign = -1;
            }
        } else {
            list0_weight = 1 << MV_SCALE_PREC;
            list1_weight = (1 << MV_SCALE_PREC) / (abs(dist0)) * abs(dist1);
            if (dist0 * dist1 < 0) {
                list1_sign = -1;
            }
        }

        ref_mvd0 = (list0_weight * ref_mvd0 + (1 << (MV_SCALE_PREC - 1))) >> MV_SCALE_PREC;
        ref_mvd1 = (list1_weight * ref_mvd1 + (1 << (MV_SCALE_PREC - 1))) >> MV_SCALE_PREC;

        ref_mvd0 = COM_CLIP3(-(1 << 15), (1 << 15) - 1, list0_sign * ref_mvd0);
        ref_mvd1 = COM_CLIP3(-(1 << 15), (1 << 15) - 1, list1_sign * ref_mvd1);

        if (direction == 0) {
            mv_offset[REFP_0][MV_X] = ref_mvd0;
            mv_offset[REFP_0][MV_Y] = 0;
            mv_offset[REFP_1][MV_X] = ref_mvd1;
            mv_offset[REFP_1][MV_Y] = 0;
        }
        else if (direction == 1) {
            mv_offset[REFP_0][MV_X] = -ref_mvd0;
            mv_offset[REFP_0][MV_Y] = 0;
            mv_offset[REFP_1][MV_X] = -ref_mvd1;
            mv_offset[REFP_1][MV_Y] = 0;
        }
        else if (direction == 2) {
            mv_offset[REFP_0][MV_X] = 0;
            mv_offset[REFP_0][MV_Y] = ref_mvd0;
            mv_offset[REFP_1][MV_X] = 0;
            mv_offset[REFP_1][MV_Y] = ref_mvd1;
        }
        else {
            mv_offset[REFP_0][MV_X] = 0;
            mv_offset[REFP_0][MV_Y] = -ref_mvd0;
            mv_offset[REFP_1][MV_X] = 0;
            mv_offset[REFP_1][MV_Y] = -ref_mvd1;
        }

        s32 mv_x = (s32)umve_base_pmv[REFP_0][MV_X] + mv_offset[REFP_0][MV_X];
        s32 mv_y = (s32)umve_base_pmv[REFP_0][MV_Y] + mv_offset[REFP_0][MV_Y];

        umve_final_pmv[REFP_0][MV_X] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_x);
        umve_final_pmv[REFP_0][MV_Y] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_y);
        umve_final_refi[REFP_0] = umve_base_refi[REFP_0];

        mv_x = (s32)umve_base_pmv[REFP_1][MV_X] + mv_offset[REFP_1][MV_X];
        mv_y = (s32)umve_base_pmv[REFP_1][MV_Y] + mv_offset[REFP_1][MV_Y];

        umve_final_pmv[REFP_1][MV_X] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_x);
        umve_final_pmv[REFP_1][MV_Y] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_y);
        umve_final_refi[REFP_1] = umve_base_refi[REFP_1];
    }
    else if (REFI_IS_VALID(refi0)) {
        if (direction == 0) {
            mv_offset[REFP_0][MV_X] = ref_mvd0;
            mv_offset[REFP_0][MV_Y] = 0;
        }
        else if (direction == 1) {
            mv_offset[REFP_0][MV_X] = -ref_mvd0;
            mv_offset[REFP_0][MV_Y] = 0;
        }
        else if (direction == 2) {
            mv_offset[REFP_0][MV_X] = 0;
            mv_offset[REFP_0][MV_Y] = ref_mvd0;
        }
        else { // (direction == 3)
            mv_offset[REFP_0][MV_X] = 0;
            mv_offset[REFP_0][MV_Y] = -ref_mvd0;
        }

        s32 mv_x = (s32)umve_base_pmv[REFP_0][MV_X] + mv_offset[REFP_0][MV_X];
        s32 mv_y = (s32)umve_base_pmv[REFP_0][MV_Y] + mv_offset[REFP_0][MV_Y];

        umve_final_pmv[REFP_0][MV_X] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_x);
        umve_final_pmv[REFP_0][MV_Y] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_y);
        umve_final_refi[REFP_0] = umve_base_refi[REFP_0];
    }
    else if (REFI_IS_VALID(refi1)) {
        if (direction == 0) {
            mv_offset[REFP_1][MV_X] = ref_mvd1;
            mv_offset[REFP_1][MV_Y] = 0;
        }
        else if (direction == 1) {
            mv_offset[REFP_1][MV_X] = -ref_mvd1;
            mv_offset[REFP_1][MV_Y] = 0;
        }
        else if (direction == 2) {
            mv_offset[REFP_1][MV_X] = 0;
            mv_offset[REFP_1][MV_Y] = ref_mvd1;
        }
        else { // (direction == 3)
            mv_offset[REFP_1][MV_X] = 0;
            mv_offset[REFP_1][MV_Y] = -ref_mvd1;
        }
        s32 mv_x = (s32)umve_base_pmv[REFP_1][MV_X] + mv_offset[REFP_1][MV_X];
        s32 mv_y = (s32)umve_base_pmv[REFP_1][MV_Y] + mv_offset[REFP_1][MV_Y];

        umve_final_pmv[REFP_1][MV_X] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_x);
        umve_final_pmv[REFP_1][MV_Y] = (s16)COM_CLIP3(COM_INT16_MIN, COM_INT16_MAX, mv_y);
        umve_final_refi[REFP_1] = umve_base_refi[REFP_1];
    }
}

void update_hmvp_candidates(com_core_t *core)
{
    int i, num_cands = core->hmvp_cands_cnt;
    const int max_hmvp_num = core->seqhdr->num_of_hmvp_cand;
    com_motion_t *hmvp_cands_list = core->hmvp_cands_list;
    s16(*mv_new)[MV_D] = core->map.map_mv[core->cu_scup]; 
    s8 *refi_new = core->map.map_refi[core->cu_scup];

    for (i = num_cands - 1; i >= 0; i--) {
        if (same_motion(hmvp_cands_list[i].mv, hmvp_cands_list[i].ref_idx, mv_new, refi_new)) {
            for (; i < num_cands - 1; i++) {
                copy_motion(&hmvp_cands_list[i], hmvp_cands_list[i + 1].mv, hmvp_cands_list[i + 1].ref_idx);
            }
            copy_motion(&hmvp_cands_list[num_cands - 1], mv_new, refi_new);
            return;
        }
    }

    if (num_cands < max_hmvp_num) {
        copy_motion(&hmvp_cands_list[num_cands], mv_new, refi_new);
        core->hmvp_cands_cnt++;
    } else {
        for (i = 1; i < max_hmvp_num; i++) {
            copy_motion(&hmvp_cands_list[i - 1], hmvp_cands_list[i].mv, hmvp_cands_list[i].ref_idx);
        }
        copy_motion(&hmvp_cands_list[max_hmvp_num - 1], mv_new, refi_new);
    }
   
}

static int fill_skip_candidates(com_motion_t hmvp_cands_list[ALLOWED_HMVP_NUM], s8 num_cands, const int num_hmvp_cands, s16 mv_new[REFP_NUM][MV_D], s8 refi_new[REFP_NUM])
{
    for (int i = 0; i < num_cands; i++) {
        if (same_motion(hmvp_cands_list[i].mv, hmvp_cands_list[i].ref_idx, mv_new, refi_new)) {
            return num_cands;
        }
    }
    if (num_cands < TRADITIONAL_SKIP_NUM + num_hmvp_cands) {
        copy_motion(&hmvp_cands_list[num_cands], mv_new, refi_new);
        num_cands++;
    }
    return num_cands;
}

static int derive_affine_constructed_candidate( int cu_width, int cu_height, int cp_valid[VER_NUM],
    s16 cp_mv[REFP_NUM][VER_NUM][MV_D], int cp_refi[REFP_NUM][VER_NUM], int cp_idx[VER_NUM], int model_idx, int ver_num,
    CPMV mrg_list_cp_mv[REFP_NUM][VER_NUM][MV_D], s8 mrg_list_refi[REFP_NUM],  int *mrg_list_cp_num, int *cnt, int mrg_idx)
{
    uavs3d_assert(g_tbl_log2[cu_width ] >= 4);
    uavs3d_assert(g_tbl_log2[cu_height] >= 4);

    int lidx, i;
    int valid_model[2] = { 0, 0 };
    s32 cpmv_tmp[REFP_NUM][VER_NUM][MV_D];
    int tmp_hor, tmp_ver;
    
    if (ver_num == 2) {
        int idx0 = cp_idx[0], idx1 = cp_idx[1];
        if (!cp_valid[idx0] || !cp_valid[idx1]) {
            return 0;
        }
        for (lidx = 0; lidx < REFP_NUM; lidx++) {
            if (REFI_IS_VALID(cp_refi[lidx][idx0]) && REFI_IS_VALID(cp_refi[lidx][idx1]) && cp_refi[lidx][idx0] == cp_refi[lidx][idx1]) {
                valid_model[lidx] = 1;
            }
        }
    } else if (ver_num == 3) {
        int idx0 = cp_idx[0], idx1 = cp_idx[1], idx2 = cp_idx[2];
        if (!cp_valid[idx0] || !cp_valid[idx1] || !cp_valid[idx2]) {
            return 0;
        }
        for (lidx = 0; lidx < REFP_NUM; lidx++) {
            if (REFI_IS_VALID(cp_refi[lidx][idx0]) && REFI_IS_VALID(cp_refi[lidx][idx1]) && REFI_IS_VALID(cp_refi[lidx][idx2]) && cp_refi[lidx][idx0] == cp_refi[lidx][idx1] && cp_refi[lidx][idx0] == cp_refi[lidx][idx2]) {
                valid_model[lidx] = 1;
            }
        }
    }

    if (valid_model[0] || valid_model[1]) {
        *mrg_list_cp_num = ver_num;
    } else {
        return 0;
    }
    if (*cnt != mrg_idx) {
        (*cnt)++;
        return 1;
    }
    for (lidx = 0; lidx < REFP_NUM; lidx++) {
        if (valid_model[lidx]) {
            for (i = 0; i < ver_num; i++) {
                cpmv_tmp[lidx][cp_idx[i]][MV_X] = (s32)cp_mv[lidx][cp_idx[i]][MV_X];
                cpmv_tmp[lidx][cp_idx[i]][MV_Y] = (s32)cp_mv[lidx][cp_idx[i]][MV_Y];
            }

            switch (model_idx) {
            case 0: 
                break;
            case 1:
                cpmv_tmp[lidx][2][MV_X] = cpmv_tmp[lidx][3][MV_X] + cpmv_tmp[lidx][0][MV_X] - cpmv_tmp[lidx][1][MV_X];
                cpmv_tmp[lidx][2][MV_Y] = cpmv_tmp[lidx][3][MV_Y] + cpmv_tmp[lidx][0][MV_Y] - cpmv_tmp[lidx][1][MV_Y];
                break;
            case 2:
                cpmv_tmp[lidx][1][MV_X] = cpmv_tmp[lidx][3][MV_X] + cpmv_tmp[lidx][0][MV_X] - cpmv_tmp[lidx][2][MV_X];
                cpmv_tmp[lidx][1][MV_Y] = cpmv_tmp[lidx][3][MV_Y] + cpmv_tmp[lidx][0][MV_Y] - cpmv_tmp[lidx][2][MV_Y];
                break;
            case 3:
                cpmv_tmp[lidx][0][MV_X] = cpmv_tmp[lidx][1][MV_X] + cpmv_tmp[lidx][2][MV_X] - cpmv_tmp[lidx][3][MV_X];
                cpmv_tmp[lidx][0][MV_Y] = cpmv_tmp[lidx][1][MV_Y] + cpmv_tmp[lidx][2][MV_Y] - cpmv_tmp[lidx][3][MV_Y];
                break;
            case 4:
                break;
            case 5:
                {
                    int shiftHtoW = 7 + g_tbl_log2[cu_width] - g_tbl_log2[cu_height]; // x * cuWidth / cuHeight
                    tmp_hor = +((cpmv_tmp[lidx][2][MV_Y] - cpmv_tmp[lidx][0][MV_Y]) << shiftHtoW) + (cpmv_tmp[lidx][0][MV_X] << 7);
                    tmp_ver = -((cpmv_tmp[lidx][2][MV_X] - cpmv_tmp[lidx][0][MV_X]) << shiftHtoW) + (cpmv_tmp[lidx][0][MV_Y] << 7);
                    com_mv_rounding_s32_shift7(tmp_hor, tmp_ver, &cpmv_tmp[lidx][1][MV_X], &cpmv_tmp[lidx][1][MV_Y]);
                }
                break;
            default:
                uavs3d_assert(0);
            }

            mrg_list_refi[lidx] = cp_refi[lidx][cp_idx[0]];
            for (i = 0; i < ver_num; i++) {
                // convert to 1/16 precision
                cpmv_tmp[lidx][i][MV_X] <<= 2;
                cpmv_tmp[lidx][i][MV_Y] <<= 2;

                mrg_list_cp_mv[lidx][i][MV_X] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, cpmv_tmp[lidx][i][MV_X]);
                mrg_list_cp_mv[lidx][i][MV_Y] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, cpmv_tmp[lidx][i][MV_Y]);
            }
        } else {
            mrg_list_refi[lidx] = -1;
        }
    }

    (*cnt)++;
    return 1;
}

static void derive_affine_model_mv(int scup, int scun, int lidx, s16(*map_mv)[REFP_NUM][MV_D], int cu_width, int cu_height, int i_scu, CPMV mvp[VER_NUM][MV_D], u32 *map_pos, int * vertex_num, int log2_max_cuwh)
{
    s16 neb_mv[VER_NUM][MV_D];
    int i;
    int neighbor[VER_NUM];
    int neb_log_w = MCU_GET_LOGW(map_pos[scun]);
    int neb_log_h = MCU_GET_LOGH(map_pos[scun]);
    int neb_w = 1 << neb_log_w;
    int neb_h = 1 << neb_log_h;
    int neb_x, neb_y;
    int cur_x, cur_y;
    int diff_w = 7 - neb_log_w;
    int diff_h = 7 - neb_log_h;
    s32 dmv_hor_x, dmv_hor_y, dmv_ver_x, dmv_ver_y, hor_base, ver_base;
    s32 tmp_hor, tmp_ver;
    int is_top_ctu_boundary = FALSE;

    neighbor[0] = MCU_GET_SCUP(map_pos[scun]);
    neighbor[1] = neighbor[0] + ((neb_w >> MIN_CU_LOG2) - 1);
    neighbor[2] = neighbor[0] + ((neb_h >> MIN_CU_LOG2) - 1) * i_scu;
    neighbor[3] = neighbor[2] + ((neb_w >> MIN_CU_LOG2) - 1);

    neb_x = (neighbor[0] % i_scu) << MIN_CU_LOG2;
    neb_y = (neighbor[0] / i_scu) << MIN_CU_LOG2;
    cur_x = (scup % i_scu) << MIN_CU_LOG2;
    cur_y = (scup / i_scu) << MIN_CU_LOG2;

    for (i = 0; i < VER_NUM; i++) {
        CP32(neb_mv[i], map_mv[neighbor[i]][lidx]);
    }
    if ((neb_y + neb_h) % (1 << log2_max_cuwh) == 0 && (neb_y + neb_h) == cur_y) {
        is_top_ctu_boundary = TRUE;
        neb_y += neb_h;
        CP32(neb_mv[0], neb_mv[2]);
        CP32(neb_mv[1], neb_mv[3]);
    }

    dmv_hor_x = (s32)(neb_mv[1][MV_X] - neb_mv[0][MV_X]) << diff_w;     
    dmv_hor_y = (s32)(neb_mv[1][MV_Y] - neb_mv[0][MV_Y]) << diff_w;

    if (*vertex_num == 3 && !is_top_ctu_boundary) {
        dmv_ver_x = (s32)(neb_mv[2][MV_X] - neb_mv[0][MV_X]) << diff_h; 
        dmv_ver_y = (s32)(neb_mv[2][MV_Y] - neb_mv[0][MV_Y]) << diff_h;
    } else {
        dmv_ver_x = -dmv_hor_y;                                      
        dmv_ver_y = dmv_hor_x;
        *vertex_num = 2;
    }
    hor_base = (s32)neb_mv[0][MV_X] << 7;
    ver_base = (s32)neb_mv[0][MV_Y] << 7;

    // derive CPMV 0
    tmp_hor = dmv_hor_x * (cur_x - neb_x) + dmv_ver_x * (cur_y - neb_y) + hor_base;
    tmp_ver = dmv_hor_y * (cur_x - neb_x) + dmv_ver_y * (cur_y - neb_y) + ver_base;
    com_mv_rounding_s32_shift7(tmp_hor, tmp_ver, &tmp_hor, &tmp_ver);

    tmp_hor <<= 2;
    tmp_ver <<= 2;

    mvp[0][MV_X] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, tmp_hor);
    mvp[0][MV_Y] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, tmp_ver);

    // derive CPMV 1
    tmp_hor = dmv_hor_x * (cur_x - neb_x + cu_width) + dmv_ver_x * (cur_y - neb_y) + hor_base;
    tmp_ver = dmv_hor_y * (cur_x - neb_x + cu_width) + dmv_ver_y * (cur_y - neb_y) + ver_base;
    com_mv_rounding_s32_shift7(tmp_hor, tmp_ver, &tmp_hor, &tmp_ver);

    tmp_hor <<= 2;
    tmp_ver <<= 2;

    mvp[1][MV_X] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, tmp_hor);
    mvp[1][MV_Y] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, tmp_ver);

    // derive CPMV 2
    if (*vertex_num == 3) {
        tmp_hor = dmv_hor_x * (cur_x - neb_x) + dmv_ver_x * (cur_y - neb_y + cu_height) + hor_base;
        tmp_ver = dmv_hor_y * (cur_x - neb_x) + dmv_ver_y * (cur_y - neb_y + cu_height) + ver_base;
        com_mv_rounding_s32_shift7(tmp_hor, tmp_ver, &tmp_hor, &tmp_ver);

        tmp_hor <<= 2;
        tmp_ver <<= 2;

        mvp[2][MV_X] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, tmp_hor);
        mvp[2][MV_Y] = (CPMV)COM_CLIP3(COM_CPMV_MIN, COM_CPMV_MAX, tmp_ver);
    }
}

static int get_affine_merge_candidate(com_core_t *core, s8 mrg_list_refi[REFP_NUM], CPMV mrg_list_cpmv[REFP_NUM][VER_NUM][MV_D], int *mrg_list_cp_num, int mrg_idx)
{
    int scup = core->cu_scup;
    com_map_t *map = &core->map;
    com_seqh_t *seqhdr = core->seqhdr;
    int i_scu = seqhdr->i_scu;
    int lidx, i, k;
    int cu_width = core->cu_width;
    int cu_height = core->cu_height;
    int cu_width_in_scu  = cu_width >> MIN_CU_LOG2;
    int cu_height_in_scu = cu_height >> MIN_CU_LOG2;
    int cnt = 0;
    s8(*map_refi)[REFP_NUM] = map->map_refi;
    s16(*map_mv)[REFP_NUM][MV_D] = map->map_mv;
    com_scu_t* map_scu = map->map_scu;
    u32* map_pos = map->map_pos;
    int log2_max_cuwh = seqhdr->log2_max_cuwh;

    //-------------------  Model based affine MVP  -------------------//
    {
        int neighbor[5];
        int valid[5];
        int top_left[5];
        int max_candidate = COM_MIN(mrg_idx + 1, AFF_MODEL_CAND);

        neighbor[0] = scup + i_scu * (cu_height_in_scu - 1) - 1; 
        neighbor[1] = scup - i_scu + cu_width_in_scu - 1;        
        neighbor[2] = scup - i_scu + cu_width_in_scu;            
        neighbor[3] = scup - 1;                                  
        neighbor[4] = scup - i_scu - 1;                          

        for (k = 0; k < 5; k++) {
            valid[k] = map_scu[neighbor[k]].affine;

            if (valid[k]) {
                top_left[k] = MCU_GET_SCUP(map_pos[neighbor[k]]);
            }
        }
        if (valid[2] && valid[1] && top_left[1] == top_left[2]) {
            valid[2] = 0;
        }

        int valid_flag_3_bakup = valid[3];

        if (valid[3] && valid[0] && top_left[0] == top_left[3]) {
            valid[3] = 0;
        }
        if ((valid[4] && valid_flag_3_bakup && top_left[4] == top_left[3]) || (valid[4] && valid[1] && top_left[4] == top_left[1])) {
            valid[4] = 0;
        }
        for ( k = 0; k < 5 && cnt < max_candidate; k++ ) {
            if (valid[k]) { 
                if (cnt == mrg_idx) {
                    *mrg_list_cp_num = (map_scu[neighbor[k]].affine == 1 ? 2 : 3);
                    for (lidx = 0; lidx < REFP_NUM; lidx++) {
                        mrg_list_refi[lidx] = map_refi[neighbor[k]][lidx];
                        if (REFI_IS_VALID(mrg_list_refi[lidx])) {
                            derive_affine_model_mv(scup, neighbor[k], lidx, map_mv, cu_width, cu_height, i_scu, mrg_list_cpmv[lidx], map_pos, mrg_list_cp_num, log2_max_cuwh);
                        }
                    }
                }
                cnt++;
            }
        }
    }

    //-------------------  control point based affine MVP  -------------------//
    if (cnt <= mrg_idx) {
        s16 cp_mv[REFP_NUM][VER_NUM][MV_D];
        int cp_refi[REFP_NUM][VER_NUM];
        int cp_valid[VER_NUM];
        int max_candidate = COM_MIN(mrg_idx + 1, AFF_MAX_NUM_MRG);

        int neb_addr_lt[AFFINE_MAX_NUM_LT];
        int neb_addr_rt[AFFINE_MAX_NUM_RT];
        int neb_addr_lb;

        for (i = 0; i < VER_NUM; i++) {
            for (lidx = 0; lidx < REFP_NUM; lidx++) {
                cp_refi[lidx][i] = -1;
            }
            cp_valid[i] = 0;
        }

        // LT
        neb_addr_lt[0] = scup - 1;                  
        neb_addr_lt[1] = scup - i_scu;     
        neb_addr_lt[2] = scup - i_scu - 1;  

        for (k = 0; k < AFFINE_MAX_NUM_LT; k++) {
            if (map_scu[neb_addr_lt[k]].inter) {
                for (lidx = 0; lidx < REFP_NUM; lidx++) {
                    cp_refi[lidx][0] = map_refi[neb_addr_lt[k]][lidx];
                    CP32(cp_mv[lidx][0], map_mv[neb_addr_lt[k]][lidx]);
                }
                cp_valid[0] = 1;
                break;
            }
        }

        // RT
        neb_addr_rt[0] = scup - i_scu + cu_width_in_scu - 1;  
        neb_addr_rt[1] = scup - i_scu + cu_width_in_scu;      
 
        for (k = 0; k < AFFINE_MAX_NUM_RT; k++) {
            if (map_scu[neb_addr_rt[k]].inter) {
                for (lidx = 0; lidx < REFP_NUM; lidx++) {
                    cp_refi[lidx][1] = map_refi[neb_addr_rt[k]][lidx];
                    CP32(cp_mv[lidx][1], map_mv[neb_addr_rt[k]][lidx]);
                }
                cp_valid[1] = 1;
                break;
            }
        }

        // LB
        neb_addr_lb = scup + i_scu * (cu_height_in_scu - 1) - 1; 

        if (map_scu[neb_addr_lb].inter) {
            for (lidx = 0; lidx < REFP_NUM; lidx++) {
                cp_refi[lidx][2] = map_refi[neb_addr_lb][lidx];
                CP32(cp_mv[lidx][2], map_mv[neb_addr_lb][lidx]);
            }
            cp_valid[2] = 1;
        }

        // RB
        s16 mv_tmp[REFP_NUM][MV_D];
        int neb_addr_rb = scup + i_scu * (cu_height_in_scu - 1) + (cu_width_in_scu - 1);
        int scu_y;
        int scup_co = get_colocal_scup(neb_addr_rb, i_scu, seqhdr->pic_width_in_scu, seqhdr->pic_height_in_scu, &scu_y);
        com_ref_pic_t(*refp)[REFP_NUM] = core->refp;

        if (core->pichdr->slice_type == SLICE_B) {
            uavs3d_check_ref_avaliable(refp[0][REFP_1].pic, scu_y << MIN_CU_LOG2);
            if (REFI_IS_VALID(refp[0][REFP_1].map_refi[scup_co][REFP_0])) {
                get_col_mv(refp[0], scup_co, mv_tmp);
                for (lidx = 0; lidx < REFP_NUM; lidx++) {
                    cp_refi[lidx][3] = 0; // ref idx
                    CP32(cp_mv[lidx][3], mv_tmp[lidx]);
                }
                cp_valid[3] = 1;
            } else {
                cp_valid[3] = 0;
            }
        } else {
            uavs3d_check_ref_avaliable(refp[0][REFP_0].pic, scu_y << MIN_CU_LOG2);
            if (REFI_IS_VALID(refp[0][REFP_0].map_refi[scup_co][REFP_0]))  {
                get_col_mv_from_list0(refp[0], scup_co, mv_tmp);
                cp_refi[0][3] = 0;
                CP32(cp_mv[0][3], mv_tmp[0]);
                cp_refi[1][3] = REFI_INVALID;
                cp_valid[3] = 1;
            } else {
                cp_valid[3] = 0;
            }
        }

        //-------------------  insert model  -------------------//
        static tab_s32 cp_num[6] = { 3, 3, 3, 3, 2, 2 };
        
        int const_model[6][VER_NUM] = {
            { 0, 1, 2 },  // 0: LT, RT, LB
            { 0, 1, 3 },  // 1: LT, RT, RB
            { 0, 2, 3 },  // 2: LT, LB, RB
            { 1, 2, 3 },  // 3: RT, LB, RB
            { 0, 1 },     // 4: LT, RT
            { 0, 2 },     // 5: LT, LB
        };

        for (int idx = 0; idx < 6 && cnt < max_candidate; idx++) {
            derive_affine_constructed_candidate(cu_width, cu_height, cp_valid, cp_mv, cp_refi, const_model[idx], idx, cp_num[idx], mrg_list_cpmv, mrg_list_refi, mrg_list_cp_num, &cnt, mrg_idx);
        }
    }

    if (cnt <= mrg_idx) {
        *mrg_list_cp_num = 2;
        mrg_list_refi[REFP_0] = 0;
        mrg_list_refi[REFP_1] = REFI_INVALID;
        memset(mrg_list_cpmv, 0, sizeof(CPMV) * REFP_NUM * VER_NUM * MV_D);
    }

    return cnt;
}

void dec_derive_skip_mv_affine(com_core_t * core, int mrg_idx)
{
    s8  mrg_list_refi  [REFP_NUM];
    int mrg_list_cp_num;
    CPMV mrg_list_cp_mv[REFP_NUM][VER_NUM][MV_D];
    int cp_idx, lidx;

    get_affine_merge_candidate(core, mrg_list_refi, mrg_list_cp_mv, &mrg_list_cp_num, mrg_idx);
    
    core->affine_flag = mrg_list_cp_num - 1;

    for (lidx = 0; lidx < REFP_NUM; lidx++) {
        if (REFI_IS_VALID(mrg_list_refi[lidx])) {
            core->refi[lidx] = mrg_list_refi[lidx];
            for (cp_idx = 0; cp_idx < mrg_list_cp_num; cp_idx++) {
                CP64(core->affine_mv[lidx][cp_idx], mrg_list_cp_mv[lidx][cp_idx]);
            }
        } else {
            core->refi[lidx] = REFI_INVALID;
        }
    }
}

void dec_derive_skip_mv_umve(com_core_t * core, int umve_idx)
{
    s16 pmv_base_cands[REFP_NUM][MV_D];
    s8  refi_base_cands[REFP_NUM];
    int base_idx = umve_idx / UMVE_MAX_REFINE_NUM;

    if (derive_umve_base_motions(core, base_idx, pmv_base_cands, refi_base_cands)) {
        derive_temporal_motions(core, pmv_base_cands, refi_base_cands);
    } 
    derive_umve_final_motions(core, umve_idx, pmv_base_cands, refi_base_cands);
}

void dec_derive_skip_mv(com_core_t * core, int spatial_skip_idx)
{
    com_seqh_t *seqhdr = core->seqhdr;
    
    if (spatial_skip_idx == 0) {
        derive_temporal_motions(core, core->mv, core->refi);
    }
    else if (spatial_skip_idx < TRADITIONAL_SKIP_NUM) {
        com_motion_t motion_cands_curr[MAX_SKIP_NUM];
        derive_mhb_skip_motions(core, motion_cands_curr);
        
        spatial_skip_idx--;
        CP64(core->mv, motion_cands_curr[spatial_skip_idx].mv);
        CP16(core->refi, motion_cands_curr[spatial_skip_idx].ref_idx);
    } else {
        int skip_idx;
        com_motion_t motion_cands_curr[MAX_SKIP_NUM];
        s8 cnt_hmvp_cands_curr;

        derive_temporal_motions(core, motion_cands_curr[0].mv, motion_cands_curr[0].ref_idx);

        uavs3d_assert(seqhdr->num_of_hmvp_cand);

        derive_mhb_skip_motions(core, motion_cands_curr + 1);
        cnt_hmvp_cands_curr = PRED_DIR_NUM + 1;

        for (skip_idx = core->hmvp_cands_cnt - 1; skip_idx >= 0 && cnt_hmvp_cands_curr <= spatial_skip_idx; skip_idx--) { 
            com_motion_t motion = core->hmvp_cands_list[skip_idx];
            cnt_hmvp_cands_curr = fill_skip_candidates(motion_cands_curr, cnt_hmvp_cands_curr, seqhdr->num_of_hmvp_cand, motion.mv, motion.ref_idx);
        }

        if (spatial_skip_idx > cnt_hmvp_cands_curr - 1) {
            com_motion_t *motion = core->hmvp_cands_cnt ? &core->hmvp_cands_list[core->hmvp_cands_cnt - 1] : &motion_cands_curr[TRADITIONAL_SKIP_NUM - 1];
            CP64(core->mv, motion->mv);
            CP16(core->refi, motion->ref_idx);
        } else {
            CP64(core->mv, motion_cands_curr[spatial_skip_idx].mv);
            CP16(core->refi, motion_cands_curr[spatial_skip_idx].ref_idx);
        }
    }
}

static void set_affine_mvf(com_core_t *core, int scup, int log2_cuw, int log2_cuh, int i_scu, com_map_t * pic_map, com_pic_header_t * sh)
{
    int cu_w_in_scu = (1 << log2_cuw) >> MIN_CU_LOG2;
    int cu_h_in_scu = (1 << log2_cuh) >> MIN_CU_LOG2;
    int cp_num = core->affine_flag + 1;

    if (REFI_IS_VALID(core->refi[0]) && REFI_IS_VALID(core->refi[1])) {
        s16(*map_mv)[REFP_NUM][MV_D] = pic_map->map_mv + scup;
        s16 mv[2][MV_D];
        s32 mv18[2][MV_D];

        s32 dmv_ver_x0, dmv_ver_y0;
        s32 mv_scale_tmp_hor0, mv_scale_tmp_ver0;
        CPMV(*ac_mv0)[MV_D] = core->affine_mv[0];
        s32(*asb_mv0)[MV_D] = core->affine_sb_mv[0];
        s32 mv_scale_hor0 = (s32)ac_mv0[0][MV_X] << 7;
        s32 mv_scale_ver0 = (s32)ac_mv0[0][MV_Y] << 7;
        s32 dmv_hor_x0 = (((s32)ac_mv0[1][MV_X] - (s32)ac_mv0[0][MV_X]) << 7) >> log2_cuw;    
        s32 dmv_hor_y0 = (((s32)ac_mv0[1][MV_Y] - (s32)ac_mv0[0][MV_Y]) << 7) >> log2_cuw;

        s32 dmv_ver_x1, dmv_ver_y1;
        s32 mv_scale_tmp_hor1, mv_scale_tmp_ver1;
        CPMV(*ac_mv1)[MV_D] = core->affine_mv[1];
        s32(*asb_mv1)[MV_D] = core->affine_sb_mv[1];
        s32 mv_scale_hor1 = (s32)ac_mv1[0][MV_X] << 7;
        s32 mv_scale_ver1 = (s32)ac_mv1[0][MV_Y] << 7;
        s32 dmv_hor_x1 = (((s32)ac_mv1[1][MV_X] - (s32)ac_mv1[0][MV_X]) << 7) >> log2_cuw;    
        s32 dmv_hor_y1 = (((s32)ac_mv1[1][MV_Y] - (s32)ac_mv1[0][MV_Y]) << 7) >> log2_cuw;

        if (cp_num == 3) {
            dmv_ver_x0 = (((s32)ac_mv0[2][MV_X] - (s32)ac_mv0[0][MV_X]) << 7) >> log2_cuh; 
            dmv_ver_y0 = (((s32)ac_mv0[2][MV_Y] - (s32)ac_mv0[0][MV_Y]) << 7) >> log2_cuh;
            dmv_ver_x1 = (((s32)ac_mv1[2][MV_X] - (s32)ac_mv1[0][MV_X]) << 7) >> log2_cuh;
            dmv_ver_y1 = (((s32)ac_mv1[2][MV_Y] - (s32)ac_mv1[0][MV_Y]) << 7) >> log2_cuh;
        } else {
            dmv_ver_x0 = -dmv_hor_y0;                                                   
            dmv_ver_y0 =  dmv_hor_x0;
            dmv_ver_x1 = -dmv_hor_y1;                                                   
            dmv_ver_y1 =  dmv_hor_x1;
        }

        for (int h_sch = 0; h_sch < cu_h_in_scu; h_sch += 2) {
            for (int w_sch = 0; w_sch < cu_w_in_scu; w_sch += 2) {
                if (w_sch == 0 && h_sch == 0) {
                    mv_scale_tmp_hor0 = mv_scale_hor0;
                    mv_scale_tmp_ver0 = mv_scale_ver0;
                    mv_scale_tmp_hor1 = mv_scale_hor1;
                    mv_scale_tmp_ver1 = mv_scale_ver1;
                } else if (w_sch + 2 == cu_w_in_scu && h_sch == 0) {
                    mv_scale_tmp_hor0 = mv_scale_hor0 + (dmv_hor_x0 << log2_cuw);
                    mv_scale_tmp_ver0 = mv_scale_ver0 + (dmv_hor_y0 << log2_cuw);
                    mv_scale_tmp_hor1 = mv_scale_hor1 + (dmv_hor_x1 << log2_cuw);
                    mv_scale_tmp_ver1 = mv_scale_ver1 + (dmv_hor_y1 << log2_cuw);
                } else if (w_sch == 0 && h_sch + 2 == cu_h_in_scu && cp_num == 3) {
                    mv_scale_tmp_hor0 = mv_scale_hor0 + (dmv_ver_x0 << log2_cuh);
                    mv_scale_tmp_ver0 = mv_scale_ver0 + (dmv_ver_y0 << log2_cuh);
                    mv_scale_tmp_hor1 = mv_scale_hor1 + (dmv_ver_x1 << log2_cuh);
                    mv_scale_tmp_ver1 = mv_scale_ver1 + (dmv_ver_y1 << log2_cuh);
                } else {
                    int pos_x = (w_sch << MIN_CU_LOG2) + 4;
                    int pos_y = (h_sch << MIN_CU_LOG2) + 4;
                    mv_scale_tmp_hor0 = mv_scale_hor0 + dmv_hor_x0 * pos_x + dmv_ver_x0 * pos_y;
                    mv_scale_tmp_ver0 = mv_scale_ver0 + dmv_hor_y0 * pos_x + dmv_ver_y0 * pos_y;
                    mv_scale_tmp_hor1 = mv_scale_hor1 + dmv_hor_x1 * pos_x + dmv_ver_x1 * pos_y;
                    mv_scale_tmp_ver1 = mv_scale_ver1 + dmv_hor_y1 * pos_x + dmv_ver_y1 * pos_y;
                }

                com_mv_rounding_affine(mv_scale_tmp_hor0, mv_scale_tmp_ver0, &mv18[0][MV_X], &mv18[0][MV_Y], &mv[0][MV_X], &mv[0][MV_Y]);
                com_mv_rounding_affine(mv_scale_tmp_hor1, mv_scale_tmp_ver1, &mv18[1][MV_X], &mv18[1][MV_Y], &mv[1][MV_X], &mv[1][MV_Y]);

                CP64(map_mv[w_sch], mv);
                CP64(map_mv[w_sch + 1], mv);
                CP64(map_mv[w_sch + i_scu], mv);
                CP64(map_mv[w_sch + i_scu + 1], mv);
                CP64(asb_mv0++, mv18[0]);
                CP64(asb_mv1++, mv18[1]);
            }
            map_mv += i_scu << 1;
        }
    } else {
        s16 mv[MV_D];
        s32 mv18[MV_D];
        s32 dmv_ver_x, dmv_ver_y;
        s32 mv_scale_tmp_hor, mv_scale_tmp_ver;
        int lidx = REFI_IS_VALID(core->refi[0]) ? 0 : 1;
        CPMV(*ac_mv)[MV_D] = core->affine_mv[lidx];
        s32(*asb_mv)[MV_D] = core->affine_sb_mv[lidx];
        s16(*map_mv)[REFP_NUM][MV_D] = pic_map->map_mv + scup;
        s32 mv_scale_hor = (s32)ac_mv[0][MV_X] << 7;
        s32 mv_scale_ver = (s32)ac_mv[0][MV_Y] << 7;
        s32 dmv_hor_x = (((s32)ac_mv[1][MV_X] - (s32)ac_mv[0][MV_X]) << 7) >> log2_cuw;
        s32 dmv_hor_y = (((s32)ac_mv[1][MV_Y] - (s32)ac_mv[0][MV_Y]) << 7) >> log2_cuw;
        int sub_blk_size = (sh->affine_subblock_size_idx == 1) ? 8 : 4;
  
        if (cp_num == 3) {
            dmv_ver_x = (((s32)ac_mv[2][MV_X] - (s32)ac_mv[0][MV_X]) << 7) >> log2_cuh;
            dmv_ver_y = (((s32)ac_mv[2][MV_Y] - (s32)ac_mv[0][MV_Y]) << 7) >> log2_cuh;
        } else {
            dmv_ver_x = -dmv_hor_y;                                                    
            dmv_ver_y = dmv_hor_x;
        }
        if (sub_blk_size == 4) {
            for (int h_sch = 0; h_sch < cu_h_in_scu; h_sch++) {
                for (int w_sch = 0; w_sch < cu_w_in_scu; w_sch++) {
                    if (w_sch == 0 && h_sch == 0) {
                        mv_scale_tmp_hor = mv_scale_hor;
                        mv_scale_tmp_ver = mv_scale_ver;
                    } else if (w_sch == cu_w_in_scu - 1 && h_sch == 0) {
                        mv_scale_tmp_hor = mv_scale_hor + (dmv_hor_x << log2_cuw);
                        mv_scale_tmp_ver = mv_scale_ver + (dmv_hor_y << log2_cuw);
                    } else if (w_sch == 0 && h_sch == cu_h_in_scu - 1 && cp_num == 3) {
                        mv_scale_tmp_hor = mv_scale_hor + (dmv_ver_x << log2_cuh);
                        mv_scale_tmp_ver = mv_scale_ver + (dmv_ver_y << log2_cuh);
                    } else {
                        int pos_x = (w_sch << MIN_CU_LOG2) + 2;
                        int pos_y = (h_sch << MIN_CU_LOG2) + 2;
                        mv_scale_tmp_hor = mv_scale_hor + dmv_hor_x * pos_x + dmv_ver_x * pos_y;
                        mv_scale_tmp_ver = mv_scale_ver + dmv_hor_y * pos_x + dmv_ver_y * pos_y;
                    }
                    com_mv_rounding_affine(mv_scale_tmp_hor, mv_scale_tmp_ver, &mv18[MV_X], &mv18[MV_Y], &mv[MV_X], &mv[MV_Y]);
                    CP32(map_mv[w_sch][lidx], mv);
                    CP64(asb_mv++, mv18);
                }
                map_mv += i_scu;
            }
        } else {
            for (int h_sch = 0; h_sch < cu_h_in_scu; h_sch += 2) {
                for (int w_sch = 0; w_sch < cu_w_in_scu; w_sch += 2) {
                    if (w_sch == 0 && h_sch == 0) {
                        mv_scale_tmp_hor = mv_scale_hor;
                        mv_scale_tmp_ver = mv_scale_ver;
                    } else if (w_sch + 2 == cu_w_in_scu && h_sch == 0) {
                        mv_scale_tmp_hor = mv_scale_hor + (dmv_hor_x << log2_cuw);
                        mv_scale_tmp_ver = mv_scale_ver + (dmv_hor_y << log2_cuw);
                    } else if (w_sch == 0 && h_sch + 2 == cu_h_in_scu && cp_num == 3) {
                        mv_scale_tmp_hor = mv_scale_hor + (dmv_ver_x << log2_cuh);
                        mv_scale_tmp_ver = mv_scale_ver + (dmv_ver_y << log2_cuh);
                    } else {
                        int pos_x = (w_sch << MIN_CU_LOG2) + 4;
                        int pos_y = (h_sch << MIN_CU_LOG2) + 4;
                        mv_scale_tmp_hor = mv_scale_hor + dmv_hor_x * pos_x + dmv_ver_x * pos_y;
                        mv_scale_tmp_ver = mv_scale_ver + dmv_hor_y * pos_x + dmv_ver_y * pos_y;
                    }
                    com_mv_rounding_affine(mv_scale_tmp_hor, mv_scale_tmp_ver, &mv18[MV_X], &mv18[MV_Y], &mv[MV_X], &mv[MV_Y]);
                    CP32(map_mv[w_sch][lidx], mv);
                    CP32(map_mv[w_sch + 1][lidx], mv);
                    CP32(map_mv[w_sch + i_scu][lidx], mv);
                    CP32(map_mv[w_sch + i_scu + 1][lidx], mv);
                    CP64(asb_mv++, mv18);
                }
                map_mv += i_scu << 1;
            }
        }
    }
}

void dec_scale_affine_mvp(com_core_t *core, int lidx, CPMV mvp[VER_NUM][MV_D], u8 curr_mvr)
{
    s8 cur_refi = core->refi[lidx];
    com_ref_pic_t(*refp)[REFP_NUM] = core->refp;
    com_map_t *map = &core->map;
    int scup = core->cu_scup;
    int i_scu = core->seqhdr->i_scu;
    int cu_width_in_scu = core->cu_width >> MIN_CU_LOG2;
    s16(*map_mv)[REFP_NUM][MV_D] = map->map_mv;
    s8(*map_refi)[REFP_NUM] = map->map_refi;
    com_scu_t *map_scu = map->map_scu;
    s16 mvp_lt[MV_D] = { 0 }, mvp_rt[MV_D] = { 0 };
    int dist_cur = refp[cur_refi][lidx].dist;
    int idx, refi;

    // LT
    idx = scup - 1;
    refi = map_refi[idx][lidx];
    if (map_scu[idx].inter && REFI_IS_VALID(refi)) { 
        scaling_mv(dist_cur, refp[refi][lidx].dist, map_mv[idx][lidx], mvp_lt);
    } else {
        idx = scup - i_scu;
        refi = map_refi[idx][lidx];
        if (map_scu[idx].inter && REFI_IS_VALID(refi)) { 
            scaling_mv(dist_cur, refp[refi][lidx].dist, map_mv[idx][lidx], mvp_lt);
        } else {
            idx = scup - i_scu - 1;
            refi = map_refi[idx][lidx];
            if (map_scu[idx].inter && REFI_IS_VALID(refi)) { 
                scaling_mv(dist_cur, refp[refi][lidx].dist, map_mv[idx][lidx], mvp_lt);
            }
        }
    }

    // RT
    idx = scup - i_scu + cu_width_in_scu - 1;
    refi = map_refi[idx][lidx];
    if (map_scu[idx].inter && REFI_IS_VALID(refi)) {
        scaling_mv(dist_cur, refp[refi][lidx].dist, map_mv[idx][lidx], mvp_rt);
    } else {
        idx = scup - i_scu + cu_width_in_scu;
        refi = map_refi[idx][lidx];
        if (map_scu[idx].inter && REFI_IS_VALID(refi)) { 
            scaling_mv(dist_cur, refp[refi][lidx].dist, map_mv[idx][lidx], mvp_rt);
        }
    }

    int amvr_shift = Tab_Affine_AMVR(curr_mvr);
    int add = amvr_shift ? 1 << (amvr_shift - 1) : 0;
    com_mv_rounding_s32(mvp[0], mvp_lt, amvr_shift, add);
    com_mv_rounding_s32(mvp[1], mvp_rt, amvr_shift, add);
}

int dec_check_pic_md5(com_pic_t * pic, u8 md5_buf[16])
{
    u8 pic_md5[16];
    int ret;

    ret = com_md5_image(pic, pic_md5);
    uavs3d_assert_return(!ret, ret);
    
    if (memcmp(md5_buf, pic_md5, 16)) {
        printf(" >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  Warnning: enc/dec mismatch! ptr = %lld\n", (long long)pic->ptr);
    }

    return RET_OK;
}

void dec_update_map(com_core_t * core)
{
    int i, j;
    int scup  = core->cu_scup;
    int i_scu = core->seqhdr->i_scu;
    int w_cu = 1 << (core->cu_log2w - MIN_CU_LOG2);
    int h_cu = 1 << (core->cu_log2h - MIN_CU_LOG2);
    s8  (*map_refi)[REFP_NUM]       = core->map.map_refi + scup;
    s16 (*map_mv)  [REFP_NUM][MV_D] = core->map.map_mv   + scup;
    u32  *map_pos                   = core->map.map_pos  + scup;
    com_scu_t *map_scu              = core->map.map_scu  + scup;
    u32 cu_pos = 0;
    com_scu_t scu = { 0 };
    s16 (*mv)[MV_D] = core->mv;
    u16 refi = M16(core->refi);
    
    scu.coded  = 1;
    scu.intra  = core->cu_mode == MODE_INTRA;
    scu.inter  = !scu.intra;
    scu.affine = core->affine_flag;
    scu.skip   = core->cu_mode == MODE_SKIP;
    scu.cbf = M32(core->cbfy) || M16(core->cbfc) || scu.intra; // only used for deblock

    MCU_SET_LOGW(cu_pos, core->cu_log2w);
    MCU_SET_LOGH(cu_pos, core->cu_log2h);
    MCU_SET_SCUP(cu_pos, core->cu_scup);

    if (core->affine_flag) {
        for (i = 0; i < h_cu; i++) {
            memset(map_scu, *(u8*)&scu, w_cu);
            for (j = 0; j < w_cu; j++) {
                M16(map_refi[j]) = refi;
            }
            map_pos[w_cu - 1] = cu_pos;
            map_refi += i_scu;
            map_mv   += i_scu;
            map_pos  += i_scu;
            map_scu  += i_scu;
        }
        set_affine_mvf(core, scup, core->cu_log2w, core->cu_log2h, i_scu, &core->map, core->pichdr);
    } else {
        if ((w_cu & 3) == 0) {
            u64 refi_64 = refi * 0x0001000100010001;
            u32 scu_32 = (*(u8*)&scu) * 0x01010101;
            uavs3d_union128_t mv_128 = { M64(mv), M64(mv) };

            for (i = 0; i < h_cu; i++) {
                for (j = 0; j < w_cu; j += 4) {
                    M32(map_scu + j) = scu_32;
                    M64(map_refi + j) = refi_64;
                    CP128(map_mv + j, &mv_128);
                    CP128(map_mv + j + 2, &mv_128);
                }
                map_pos[w_cu - 1] = cu_pos;
                map_refi += i_scu;
                map_mv   += i_scu;
                map_pos  += i_scu;
                map_scu  += i_scu;
            }
        } else if ((w_cu & 1) == 0) {
            u32 refi_32 = refi * 0x00010001;
            u16 scu_16 = (*(u8*)&scu) * 0x0101;
            uavs3d_union128_t mv_128 = { M64(mv), M64(mv) };

            for (i = 0; i < h_cu; i++) {
                M16(map_scu) = scu_16;
                M32(map_refi) = refi_32;
                CP128(map_mv, &mv_128);
                map_pos[w_cu - 1] = cu_pos;
                map_refi += i_scu;
                map_mv   += i_scu;
                map_pos  += i_scu;
                map_scu  += i_scu;
            }
        } else {
            for (i = 0; i < h_cu; i++) {
                *map_scu = scu;
                M16(map_refi) = refi;
                CP64(map_mv, mv);
                map_pos[0] = cu_pos;
                map_refi += i_scu;
                map_mv   += i_scu;
                map_pos  += i_scu;
                map_scu  += i_scu;
            }
        }
    }

    map_pos -= i_scu;

    for (j = 0; j < w_cu; j++) {
        map_pos[j] = cu_pos;
    }
}

void dec_update_map_for_intra(com_scu_t *map_scu, s8 *map_ipm, int tb_x, int tb_y, int tb_w, int tb_h, int i_scu, int ipm)
{
    int scu_x = tb_x >> MIN_CU_LOG2;
    int scu_y = tb_y >> MIN_CU_LOG2;
    int scu_w = tb_w >> MIN_CU_LOG2;
    int scu_h = tb_h >> MIN_CU_LOG2;
    int scup = scu_y * i_scu + scu_x;
    com_scu_t scu = { 0 };
    scu.coded = 1;
    scu.intra = 1;

    map_ipm += scup;
    map_scu += scup;

    for (int j = 0; j < scu_h; j++) {
        for (int i = 0; i < scu_w; i++) {
            map_ipm[i] = ipm;
            map_scu[i] = scu;
        }
        map_ipm += i_scu;
        map_scu += i_scu;
    }
}

static int split_get_part_size(int split_mode, int part_num, int length)
{
    int ans;

    switch (split_mode) {
    case SPLIT_QUAD:
    case SPLIT_BI_HOR:
    case SPLIT_BI_VER:
        ans = length >> 1;
        break;
    case SPLIT_EQT_HOR:
    case SPLIT_EQT_VER:
        if ((part_num == 1) || (part_num == 2)) {
            ans = length >> 1;
        } else {
            ans = length >> 2;
        }
        break;
    default:
        ans = length;
        break;
    }
    return ans;
}

static int split_get_part_size_idx(int split_mode, int part_num, int length_idx)
{
    int ans;

    switch (split_mode) {
    case SPLIT_QUAD:
    case SPLIT_BI_HOR:
    case SPLIT_BI_VER:
        ans = length_idx - 1;
        break;
    case SPLIT_EQT_HOR:
    case SPLIT_EQT_VER:
        if ((part_num == 1) || (part_num == 2)) {
            ans = length_idx - 1;
        } else {
            ans = length_idx - 2;
        }
        break;
    default:
        ans = length_idx;
        break;
    }
    return ans;
}

static int split_is_vertical(com_split_mode_t mode)
{
    return mode == SPLIT_BI_VER || mode == SPLIT_EQT_VER;
}

static int split_is_eqt(com_split_mode_t mode)
{
    return mode == SPLIT_EQT_HOR || mode == SPLIT_EQT_VER;
}

static int split_is_bt(com_split_mode_t mode)
{
    return mode == SPLIT_BI_HOR || mode == SPLIT_BI_VER;
}

void dec_get_split_struct(int split_mode, int x0, int y0, int cu_width, int cu_height, dec_split_info_t* split_struct)
{
    int i;
    int log_cuw = COM_LOG2(cu_width);
    int log_cuh = COM_LOG2(cu_height);
    static tab_u8 tab_part_count[NUM_SPLIT_MODE] = { 0, 2, 2, 4, 4, 4 };
    split_struct->part_count = tab_part_count[split_mode];
    split_struct->x_pos[0] = x0;
    split_struct->y_pos[0] = y0;
    
    switch (split_mode) {
    case NO_SPLIT:
        split_struct->width  [0] = cu_width;
        split_struct->height [0] = cu_height;
        split_struct->log_cuw[0] = log_cuw;
        split_struct->log_cuh[0] = log_cuh;
        break;
    case SPLIT_QUAD:
        split_struct->width  [0] = cu_width >> 1;
        split_struct->height [0] = cu_height >> 1;
        split_struct->log_cuw[0] = log_cuw - 1;
        split_struct->log_cuh[0] = log_cuh - 1;

        for (i = 1; i < split_struct->part_count; ++i) {
            split_struct->width  [i] = split_struct->width[0];
            split_struct->height [i] = split_struct->height[0];
            split_struct->log_cuw[i] = split_struct->log_cuw[0];
            split_struct->log_cuh[i] = split_struct->log_cuh[0];
        }
        split_struct->x_pos[1] = x0 + split_struct->width[0];
        split_struct->y_pos[1] = y0;
        split_struct->x_pos[2] = x0;
        split_struct->y_pos[2] = y0 + split_struct->height[0];
        split_struct->x_pos[3] = split_struct->x_pos[1];
        split_struct->y_pos[3] = split_struct->y_pos[2];
        break;
    default:
        if (split_is_vertical(split_mode)) {
            for (i = 0; i < split_struct->part_count; ++i) {
                split_struct->width[i] = split_get_part_size(split_mode, i, cu_width);
                split_struct->log_cuw[i] = split_get_part_size_idx(split_mode, i, log_cuw);

                if (split_mode == SPLIT_EQT_VER) {
                    if (i == 0 || i == 3) {
                        split_struct->height [i] = cu_height;
                        split_struct->log_cuh[i] = log_cuh;
                    } else {
                        split_struct->height [i] = cu_height >> 1;
                        split_struct->log_cuh[i] = log_cuh - 1;
                    }
                } else {
                    split_struct->height [i] = cu_height;
                    split_struct->log_cuh[i] = log_cuh;

                    if (i) {
                        split_struct->x_pos[i] = split_struct->x_pos[i - 1] + split_struct->width[i - 1];
                        split_struct->y_pos[i] = split_struct->y_pos[i - 1];
                    }
                }
            }
            if (split_mode == SPLIT_EQT_VER) {
                split_struct->x_pos[1] = split_struct->x_pos[0] + split_struct->width[0];
                split_struct->y_pos[1] = split_struct->y_pos[0];
                split_struct->x_pos[2] = split_struct->x_pos[1];
                split_struct->y_pos[2] = split_struct->y_pos[1] + split_struct->height[1];
                split_struct->x_pos[3] = split_struct->x_pos[1] + split_struct->width[1];
                split_struct->y_pos[3] = split_struct->y_pos[1];
            }
        } else {
            for (i = 0; i < split_struct->part_count; ++i) {
                if (split_mode == SPLIT_EQT_HOR) {
                    if (i == 0 || i == 3) {
                        split_struct->width [i] = cu_width;
                        split_struct->log_cuw[i] = log_cuw;
                    } else {
                        split_struct->width [i] = cu_width >> 1;
                        split_struct->log_cuw[i] = log_cuw - 1;
                    }
                } else {
                    split_struct->width [i] = cu_width;
                    split_struct->log_cuw[i] = log_cuw;

                    if (i) {
                        split_struct->y_pos[i] = split_struct->y_pos[i - 1] + split_struct->height[i - 1];
                        split_struct->x_pos[i] = split_struct->x_pos[i - 1];
                    }
                }
                split_struct->height [i] = split_get_part_size(split_mode, i, cu_height);
                split_struct->log_cuh[i] = split_get_part_size_idx(split_mode, i, log_cuh);
            }
            if (split_mode == SPLIT_EQT_HOR) {
                split_struct->y_pos[1] = split_struct->y_pos[0] + split_struct->height[0];
                split_struct->x_pos[1] = split_struct->x_pos[0];
                split_struct->y_pos[2] = split_struct->y_pos[1];
                split_struct->x_pos[2] = split_struct->x_pos[1] + split_struct->width[1];
                split_struct->y_pos[3] = split_struct->y_pos[1] + split_struct->height[1];
                split_struct->x_pos[3] = split_struct->x_pos[1];
            }
        }
        break;
    }
}

int dec_get_split_available(com_seqh_t* seqhdr, int x, int y, int cu_w, int cu_h, int qt_depth, int bet_depth, int slice_type)
{
    const int pic_width  = seqhdr->pic_width;
    const int pic_height = seqhdr->pic_height;
    int split_tab = 0;

    if (x + cu_w > pic_width || y + cu_h > pic_height) {
        if ((cu_w == 64 && cu_h == 128) || (cu_h == 64 && cu_w == 128)) {  
            split_tab = (1 << SPLIT_BI_HOR) | (1 << SPLIT_BI_VER);
        }
        else if (slice_type == SLICE_I && cu_w == 128 && cu_h == 128) {  
            split_tab = (1 << SPLIT_QUAD) | (1 << NO_SPLIT);
        }
        else if (y + cu_h <= pic_height) {
            split_tab = 1 << SPLIT_BI_VER;
        }
        else if (x + cu_w <= pic_width) {
            split_tab = 1 << SPLIT_BI_HOR;
        }
        else {
            split_tab = 1 << SPLIT_QUAD;
        }
    } else {
        if ((cu_w == 64 && cu_h == 128) || (cu_h == 64 && cu_w == 128)) { 
            split_tab = (1 << NO_SPLIT) | (1 << SPLIT_BI_HOR) | (1 << SPLIT_BI_VER);
        }
        else if (qt_depth + bet_depth < seqhdr->max_split_times) { 
            if (slice_type == SLICE_I && cu_w == 128 && cu_h == 128) {
                split_tab = (1 << NO_SPLIT) | (1 << SPLIT_QUAD);
            } else {
                const int max_bt_size     = seqhdr->max_bt_size;
                const int max_eqt_size    = seqhdr->max_eqt_size;
                const int max_aspect_log2 = seqhdr->max_part_ratio_log2;
                const int min_eqt_size    = seqhdr->min_cu_size;
                const int min_bt_size     = seqhdr->min_cu_size;

                split_tab = (1 << NO_SPLIT) | ((bet_depth == 0 && cu_w > seqhdr->min_qt_size) << SPLIT_QUAD);

                if (cu_w <= max_bt_size && cu_h <= max_bt_size) {
                    split_tab |= ((cu_h > min_bt_size && cu_w < (cu_h << max_aspect_log2)) << SPLIT_BI_HOR) |
                                 ((cu_w > min_bt_size && cu_h < (cu_w << max_aspect_log2)) << SPLIT_BI_VER);
                }
                if (cu_w <= max_eqt_size && cu_h <= max_eqt_size && cu_w > min_eqt_size && cu_h > min_eqt_size) {
                    int max_aspect_log2_eqt = max_aspect_log2 - 1;
                    split_tab |= ((cu_h > (min_eqt_size << 1) && cu_w < (cu_h << max_aspect_log2_eqt)) << SPLIT_EQT_HOR) |
                                 ((cu_w > (min_eqt_size << 1) && cu_h < (cu_w << max_aspect_log2_eqt)) << SPLIT_EQT_VER);
                }
            }
        }
    }
    return split_tab;
}

u8 dec_cons_allow(int w, int h, com_split_mode_t split)
{
    int s = w * h;
    return ((split_is_eqt(split) && s == 128) || ((split_is_bt(split) || split == SPLIT_QUAD) && s == 64));
}

int dec_dt_allow(int cu_w, int cu_h, int pred_mode, int max_size)
{
    if (cu_w <= max_size && cu_h <= max_size) {
        const int min_size = 16;
        int hori_allow = cu_h >= min_size && cu_w < (cu_h << 2);
        int vert_allow = cu_w >= min_size && cu_h < (cu_w << 2);
        return hori_allow + (vert_allow << 1);
    } else {
        return 0;
    }
}

void dec_get_part_info(com_core_t *core, com_part_info_t* sub_info)
{
    int i;
    int x = core->cu_pix_x;
    int y = core->cu_pix_y;
    int w = core->cu_width;
    int h = core->cu_height;
    int part_size = core->pb_part;
    int qw = w >> MIN_CU_LOG2;
    int qh = h >> MIN_CU_LOG2;

    switch (part_size)
    {
    case SIZE_2Nx2N:
        sub_info->num_sub_part = 1;
        sub_info->sub_x[0] = x;
        sub_info->sub_y[0] = y;
        sub_info->sub_w[0] = w;
        sub_info->sub_h[0] = h;
        break;
    case SIZE_2NxhN:
        sub_info->num_sub_part = 4;
        for (i = 0; i < sub_info->num_sub_part; i++)
        {
            sub_info->sub_x[i] = x;
            sub_info->sub_y[i] = qh * i + y;
            sub_info->sub_w[i] = w;
            sub_info->sub_h[i] = qh;
        }
        break;
    case SIZE_2NxnU:
        sub_info->num_sub_part = 2;
        for (i = 0; i < sub_info->num_sub_part; i++)
        {
            sub_info->sub_x[i] = x;
            sub_info->sub_y[i] = qh * (i == 0 ? 0 : 1) + y;
            sub_info->sub_w[i] = w;
            sub_info->sub_h[i] = qh * (i == 0 ? 1 : 3);
        }
        break;
    case SIZE_2NxnD:
        sub_info->num_sub_part = 2;
        for (i = 0; i < sub_info->num_sub_part; i++)
        {
            sub_info->sub_x[i] = x;
            sub_info->sub_y[i] = qh * (i == 0 ? 0 : 3) + y;
            sub_info->sub_w[i] = w;
            sub_info->sub_h[i] = qh * (i == 0 ? 3 : 1);
        }
        break;
    case SIZE_hNx2N:
        sub_info->num_sub_part = 4;
        for (i = 0; i < sub_info->num_sub_part; i++)
        {
            sub_info->sub_x[i] = qw * i + x;
            sub_info->sub_y[i] = y;
            sub_info->sub_w[i] = qw;
            sub_info->sub_h[i] = h;
        }
        break;
    case SIZE_nLx2N:
        sub_info->num_sub_part = 2;
        for (i = 0; i < sub_info->num_sub_part; i++)
        {
            sub_info->sub_x[i] = qw * (i == 0 ? 0 : 1) + x;
            sub_info->sub_y[i] = y;
            sub_info->sub_w[i] = qw * (i == 0 ? 1 : 3);
            sub_info->sub_h[i] = h;
        }
        break;
    case SIZE_nRx2N:
        sub_info->num_sub_part = 2;
        for (i = 0; i < sub_info->num_sub_part; i++)
        {
            sub_info->sub_x[i] = qw * (i == 0 ? 0 : 3) + x;
            sub_info->sub_y[i] = y;
            sub_info->sub_w[i] = qw * (i == 0 ? 3 : 1);
            sub_info->sub_h[i] = h;
        }
        break;
    case SIZE_NxN:
        sub_info->num_sub_part = 4;
        for (i = 0; i < sub_info->num_sub_part; i++)
        {
            sub_info->sub_x[i] = qw * (i == 0 || i == 2 ? 0 : 2) + x;
            sub_info->sub_y[i] = qh * (i == 0 || i == 1 ? 0 : 2) + y;
            sub_info->sub_w[i] = qw * 2;
            sub_info->sub_h[i] = qh * 2;
        }
        break;
    default:
        uavs3d_assert(0);
    }
}

int dec_get_pb_idx_by_tb(com_part_size_t pb_part, int idx)
{
    switch (pb_part) {
    case SIZE_2NxnU:
    case SIZE_nLx2N:
        return idx == 0 ? 0 : 1;
    case SIZE_2NxnD:
    case SIZE_nRx2N:
        return idx == 3 ? 1 : 0;
    case SIZE_2NxhN:
    case SIZE_hNx2N:
    case SIZE_2Nx2N:
        return idx;
    default:
        uavs3d_assert(0);
        return -1;
    }
}

void dec_get_tb_width_height(int w, int h, com_part_size_t part, int *tb_w, int *tb_h)
{
    switch (part) {
    case SIZE_2Nx2N:
        break;
    case SIZE_NxN:
        w >>= 1;
        h >>= 1;
        break;
    case SIZE_2NxhN:
        h >>= 2;
        break;
    case SIZE_hNx2N:
        w >>= 2;
        break;
    default:
        uavs3d_assert(0);
        break;
    }

    *tb_w = w;
    *tb_h = h;
}

void dec_get_tb_start_pos(int w, int h, com_part_size_t part, int idx, int *pos_x, int *pos_y)
{
    int x = 0, y = 0;

    switch (part) {
    case SIZE_2Nx2N:
        break;
    case SIZE_NxN:
        y = (idx / 2) * h / 2;
        x = (idx % 2) * w / 2;
        break;
    case SIZE_2NxhN:
        y = idx * (h / 4);
        break;
    case SIZE_hNx2N:
        x = idx * (w / 4);
        break;
    default:
        uavs3d_assert(0);
        break;
    }

    *pos_x = x;
    *pos_y = y;
}

u8 dec_is_separate_tree(int w, int h, com_split_mode_t split)
{
    if (split == SPLIT_QUAD) {
        return (w == 8);
    }
    else if (split == SPLIT_EQT_HOR) {
        return (h == 16 || w == 8);
    }
    else if (split == SPLIT_EQT_VER) {
        return (w == 16 || h == 8);
    }
    else if (split == SPLIT_BI_HOR) {
        return (h == 8);
    }
    else if (split == SPLIT_BI_VER) {
        return (w == 8);
    }
    else {
        return 0;
    }
}

void __cdecl uavs3d_img_cpy_cvt(uavs3d_io_frm_t * dst, uavs3d_io_frm_t * src, int bit_depth)
{
    if (bit_depth == 10) {
        uavs3d_funs_handle.conv_fmt_16bit((unsigned char*)src->buffer[0], (unsigned char*)src->buffer[1], (unsigned char**)dst->buffer, src->width[0], src->height[0],
            src->stride[0], src->stride[1], dst->stride, 1);
    } else { // the output reconstructed file is 8-bit storage for 8-bit depth
#if (BIT_DEPTH == 8)
        uavs3d_funs_handle.conv_fmt_8bit((unsigned char*)src->buffer[0], (unsigned char*)src->buffer[1], (unsigned char**)dst->buffer, src->width[0], src->height[0],
            src->stride[0], src->stride[1], dst->stride, 1);
#else
        uavs3d_funs_handle.conv_fmt_16to8bit((unsigned char*)src->buffer[0], (unsigned char*)src->buffer[1], (unsigned char**)dst->buffer, src->width[0], src->height[0],
            src->stride[0], src->stride[1], dst->stride, 1);
#endif
    }
}
