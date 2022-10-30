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

#define FLT_8TAP_HOR(src, i, coef) ( \
    (src)[i-3] * (coef)[0] + \
    (src)[i-2] * (coef)[1] + \
    (src)[i-1] * (coef)[2] + \
    (src)[i  ] * (coef)[3] + \
    (src)[i+1] * (coef)[4] + \
    (src)[i+2] * (coef)[5] + \
    (src)[i+3] * (coef)[6] + \
    (src)[i+4] * (coef)[7])

#define FLT_8TAP_VER(src, i, i_src, coef) ( \
    (src)[i-3 * i_src] * (coef)[0] + \
    (src)[i-2 * i_src] * (coef)[1] + \
    (src)[i-1 * i_src] * (coef)[2] + \
    (src)[i          ] * (coef)[3] + \
    (src)[i+1 * i_src] * (coef)[4] + \
    (src)[i+2 * i_src] * (coef)[5] + \
    (src)[i+3 * i_src] * (coef)[6] + \
    (src)[i+4 * i_src] * (coef)[7])

#define FLT_4TAP_HOR(src, i, coef) ( \
    (src)[i - 2] * (coef)[0] + \
    (src)[i    ] * (coef)[1] + \
    (src)[i + 2] * (coef)[2] + \
    (src)[i + 4] * (coef)[3])

#define FLT_4TAP_VER(src, i, i_src, coef) ( \
    (src)[i-1 * i_src] * (coef)[0] + \
    (src)[i          ] * (coef)[1] + \
    (src)[i+1 * i_src] * (coef)[2] + \
    (src)[i+2 * i_src] * (coef)[3])


static void uavs3d_if_hor_luma(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, s8 const *coeff, int max_val)
{
    int row, col;
    int sum, val;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            sum = FLT_8TAP_HOR(src, col, coeff);
            val = (sum + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, val);
        }
        src += i_src;
        dst += i_dst;
    }
}

static void uavs3d_if_ver_luma(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, s8 const *coeff, int max_val)
{
    int row, col;
    int sum, val;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            sum = FLT_8TAP_VER(src, col, i_src, coeff);
            val = (sum + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, val);
        }
        src += i_src;
        dst += i_dst;
    }
}

static void uavs3d_if_hor_ver_luma(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff_h, const s8 *coeff_v, int max_val)
{
    int row, col;
    int sum, val;
    int add1, shift1;
    int add2, shift2;

    ALIGNED_16(s16 tmp_res[(128 + 7) * 128]);
    s16 *tmp;

    if (max_val == 255) { // 8 bit_depth
        shift1 = 0;
        shift2 = 12;
    }
    else { // 10 bit_depth
        shift1 = 2;
        shift2 = 10;
    }

    add1 = (1 << (shift1)) >> 1;
    add2 = 1 << (shift2 - 1);

    src += -3 * i_src;
    tmp = tmp_res;

    if (shift1) {
        for (row = -3; row < height + 4; row++) {
            for (col = 0; col < width; col++) {
                sum = FLT_8TAP_HOR(src, col, coeff_h);
                tmp[col] = (sum + add1) >> shift1;
            }
            src += i_src;
            tmp += width;
        }
    }
    else {
        for (row = -3; row < height + 4; row++) {
            for (col = 0; col < width; col++) {
                tmp[col] = FLT_8TAP_HOR(src, col, coeff_h);
            }
            src += i_src;
            tmp += width;
        }
    }

    tmp = tmp_res + 3 * width;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            sum = FLT_8TAP_VER(tmp, col, width, coeff_v);
            val = (sum + add2) >> shift2;
            dst[col] = COM_CLIP3(0, max_val, val);
        }
        dst += i_dst;
        tmp += width;
    }
}

static void uavs3d_if_hor_chroma(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, s8 const *coeff, int max_val)
{
    int row, col;
    int sumu, sumv, valu, valv;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col += 2) {
            sumu = FLT_4TAP_HOR(src, col, coeff);
            sumv = FLT_4TAP_HOR(src, col + 1, coeff);
            valu = (sumu + 32) >> 6;
            valv = (sumv + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, valu);
            dst[col + 1] = COM_CLIP3(0, max_val, valv);
        }
        src += i_src;
        dst += i_dst;
    }
}

static void uavs3d_if_ver_chroma(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, s8 const *coeff, int max_val)
{
    int row, col;
    int sumu, valu, sumv, valv;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col += 2) {
            sumu = FLT_4TAP_VER(src, col, i_src, coeff);
            sumv = FLT_4TAP_VER(src, col + 1, i_src, coeff);
            valu = (sumu + 32) >> 6;
            valv = (sumv + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, valu);
            dst[col + 1] = COM_CLIP3(0, max_val, valv);
        }
        src += i_src;
        dst += i_dst;
    }
}

static void uavs3d_if_hor_ver_chroma(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff_h, const s8 *coeff_v, int max_val)
{
    int row, col;
    int sumu, sumv, valu, valv;
    int add1, shift1;
    int add2, shift2;

    ALIGNED_16(s16 tmp_res[(64 + 3) * 64 * 2]); // UV interlaced
    s16 *tmp;

    if (max_val == 255) { // 8 bit_depth
        shift1 = 0;
        shift2 = 12;
    }
    else { // 10 bit_depth
        shift1 = 2;
        shift2 = 10;
    }

    add1 = (1 << (shift1)) >> 1;
    add2 = 1 << (shift2 - 1);

    src += -1 * i_src;
    tmp = tmp_res;

    if (shift1) {
        for (row = -1; row < height + 2; row++) {
            for (col = 0; col < width; col += 2) {
                sumu = FLT_4TAP_HOR(src, col, coeff_h);
                sumv = FLT_4TAP_HOR(src, col + 1, coeff_h);
                tmp[col] = (sumu + add1) >> shift1;
                tmp[col + 1] = (sumv + add1) >> shift1;
            }
            src += i_src;
            tmp += width;
        }
    }
    else {
        for (row = -1; row < height + 2; row++) {
            for (col = 0; col < width; col += 2) {
                tmp[col] = FLT_4TAP_HOR(src, col, coeff_h);
                tmp[col + 1] = FLT_4TAP_HOR(src, col + 1, coeff_h);
            }
            src += i_src;
            tmp += width;
        }
    }

    tmp = tmp_res + 1 * width;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col += 2) {
            sumu = FLT_4TAP_VER(tmp, col, width, coeff_v);
            sumv = FLT_4TAP_VER(tmp, col + 1, width, coeff_v);
            valu = (sumu + add2) >> shift2;
            valv = (sumv + add2) >> shift2;
            dst[col] = COM_CLIP3(0, max_val, valu);
            dst[col + 1] = COM_CLIP3(0, max_val, valv);
        }
        dst += i_dst;
        tmp += width;
    }
}

static void uavs3d_avg_pel(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i, j;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            dst[j] = (src1[j] + src2[j] + 1) >> 1;
        }
        dst += i_dst;
        src1 += width;
        src2 += width;
    }
}

void uavs3d_if_cpy(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int row;
    for (row = 0; row < height; row++) {
        memcpy(dst, src, sizeof(pel)* width);
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_always_inline mc_core_luma(com_pic_t* ref, int i_src, pel *dst, int dst_stride, int x_pos, int y_pos, int width, int height, int widx, int max_posx, int max_posy, int max_val, int hp_flag) 
{
    const s8 (*coeff)[8];
    int dx, dy;

    if (hp_flag) {
        dx = x_pos & 15;
        dy = y_pos & 15;
        x_pos >>= 4;
        y_pos >>= 4;
        coeff = g_tbl_mc_coeff_luma_hp;
    } else {
        dx = x_pos & 3;
        dy = y_pos & 3;
        x_pos >>= 2;
        y_pos >>= 2;
        coeff = g_tbl_mc_coeff_luma;
    }

    x_pos = COM_CLIP3(-MAX_CU_SIZE - 4, max_posx, x_pos);
    y_pos = COM_CLIP3(-MAX_CU_SIZE - 4, max_posy, y_pos);

    uavs3d_check_ref_avaliable(ref, y_pos + height + 4);

    pel *src = ref->y + y_pos * i_src + x_pos;

    if (dx == 0 && dy == 0) {
        uavs3d_funs_handle.ipcpy[widx](src, i_src, dst, dst_stride, width, height);
    }
    else if (dy == 0) {
        uavs3d_funs_handle.ipflt[IPFILTER_H_8][widx](src, i_src, dst, dst_stride, width, height, coeff[dx], max_val);
    }
    else if (dx == 0) {
        uavs3d_funs_handle.ipflt[IPFILTER_V_8][widx](src, i_src, dst, dst_stride, width, height, coeff[dy], max_val);
    }
    else {
        uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][widx](src, i_src, dst, dst_stride, width, height, coeff[dx], coeff[dy], max_val);
    }
}

void uavs3d_always_inline mc_core_chroma(com_pic_t* ref, int i_src, pel *dst, int dst_stride, int x_pos, int y_pos, int width, int height, int widx, int max_posx, int max_posy, int max_val, int hp_flag) 
{
    int dx,dy;
    const s8 (*coeff)[4];
        
    if (hp_flag) {
        dx = x_pos & 31;
        dy = y_pos & 31;
        x_pos >>= 5;
        y_pos >>= 5;
        coeff = g_tbl_mc_coeff_chroma_hp;
    } else {
        dx = x_pos & 7;
        dy = y_pos & 7;
        x_pos >>= 3;
        y_pos >>= 3;
        coeff = g_tbl_mc_coeff_chroma;
    }

    x_pos = COM_CLIP3(-(MAX_CU_SIZE >> 1) - 2, max_posx, x_pos);
    y_pos = COM_CLIP3(-(MAX_CU_SIZE >> 1) - 2, max_posy, y_pos);

    uavs3d_check_ref_avaliable(ref, ((y_pos + height) << 1) + 4);

    pel *src = ref->uv + y_pos * i_src + (x_pos << 1);

    if (dx == 0 && dy == 0) {
        uavs3d_funs_handle.ipcpy[widx](src, i_src, dst, dst_stride, width, height);
    }
    else if (dy == 0) {
        uavs3d_funs_handle.ipflt[IPFILTER_H_4][widx](src, i_src, dst, dst_stride, width, height, coeff[dx], max_val);
    }
    else if (dx == 0) {
        uavs3d_funs_handle.ipflt[IPFILTER_V_4][widx](src, i_src, dst, dst_stride, width, height, coeff[dy], max_val);
    }
    else {
        uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][widx](src, i_src, dst, dst_stride, width, height, coeff[dx], coeff[dy], max_val);
    }
}

void com_mc(com_core_t *core, pel *pred_buf)
{
    s8 *refi = core->refi;
    s16 (*mv)[MV_D] = core->mv;
    com_pic_t *pic = core->pic;
    com_seqh_t *seqhdr = core->seqhdr;
    int x = core->cu_pix_x;
    int y = core->cu_pix_y;
    int w = core->cu_width;
    int h = core->cu_height;
    com_ref_pic_t(*refp)[REFP_NUM] = core->refp;
    ALIGNED_32(pel pred_snd[MAX_CU_DIM + MAX_CU_DIM / 2]);
    int lidx_base = REFI_IS_VALID(refi[REFP_0]) ? REFP_0 : REFP_1;
    int tree_status = core->tree_status;
    int max_posx = seqhdr->pic_width  + 4;
    int max_posy = seqhdr->pic_height + 4;
    int max_val = (1 << seqhdr->bit_depth_internal) - 1;
    int widx = core->cu_log2w - 2;
    int i_dst;
    int i_src = pic->stride_luma;
    int i_srcc = pic->stride_chroma;
    pel *dst;
    int bi = REFI_IS_VALID(refi[REFP_0]) &&
             REFI_IS_VALID(refi[REFP_1]) &&
            (refp[refi[REFP_0]][REFP_0].pic != refp[refi[REFP_1]][REFP_1].pic || M32(mv[REFP_0]) != M32(mv[REFP_1]));
   
    if (tree_status != TREE_C) {
        int i, lidx = lidx_base;

        for (i = 0; i <= bi; i++) {
            com_pic_t *ref_pic = refp[refi[lidx]][lidx].pic;
            int qpel_gmv_x = (x << 2) + mv[lidx][MV_X];
            int qpel_gmv_y = (y << 2) + mv[lidx][MV_Y];
            lidx = !lidx;

            if (i) {
                i_dst = w;
                dst = pred_snd;
            } else if (!bi && !M32(core->cbfy)) {
                i_dst = pic->stride_luma;
                dst = pic->y + y * i_dst + x;
            } else {
                i_dst = w;
                dst = pred_buf;
            }
            mc_core_luma(ref_pic, i_src, dst, i_dst, qpel_gmv_x, qpel_gmv_y, w, h, widx, max_posx, max_posy, max_val, 0);
        }
        if (bi) {
            if (!M32(core->cbfy)) {
                i_dst = pic->stride_luma;
                uavs3d_funs_handle.avg_pel[widx](pic->y + y * i_dst + x, i_dst, pred_buf, pred_snd, w, h);
            } else {
                uavs3d_funs_handle.avg_pel[widx](pred_buf, w, pred_buf, pred_snd, w, h);
            }
        }
    }
    if (tree_status != TREE_L) {
        int i, lidx = lidx_base;
        int blk_dim = w * h;

        for (i = 0; i <= bi; i++) {
            com_pic_t *ref_pic = refp[refi[lidx]][lidx].pic;
            int qpel_gmv_x = (x << 2) + mv[lidx][MV_X];
            int qpel_gmv_y = (y << 2) + mv[lidx][MV_Y];
            
            lidx = !lidx;
            if (i) {
                i_dst = w;
                dst = pred_snd;
            } else if (!bi && !M16(core->cbfc)) {
                i_dst = pic->stride_chroma;
                dst = pic->uv + y / 2 * i_dst + x;
            } else {
                i_dst = w;
                dst = pred_buf + blk_dim;
            }
        
            mc_core_chroma(ref_pic, i_srcc, dst, i_dst, qpel_gmv_x, qpel_gmv_y, w, h >> 1, widx, max_posx >> 1, max_posy >> 1, max_val, 0);
        }
        if (bi) {
            if (!M16(core->cbfc)) {
                i_dst = pic->stride_chroma;
                uavs3d_funs_handle.avg_pel[widx](pic->uv + y / 2 * i_dst + x, i_dst, pred_buf + blk_dim, pred_snd, w, h >> 1);
            } else {
                uavs3d_funs_handle.avg_pel[widx](pred_buf + blk_dim, w, pred_buf + blk_dim, pred_snd, w, h >> 1);
            }
        }
    }
}

void uavs3d_always_inline com_affine_mc_luma(com_core_t *core, pel *dstl, int i_dstl, int x, int y, int cu_width, int cu_height, com_pic_t* ref_pic, int sub_blk_size, int lidx)
{
    int w, h;
    s32(*asb_mv)[MV_D] = core->affine_sb_mv[lidx];
    com_seqh_t *seqhdr = core->seqhdr;
    int max_val = (1 << seqhdr->bit_depth_internal) - 1;
    int max_posx = seqhdr->pic_width  + 4;
    int max_posy = seqhdr->pic_height + 4;
    int widx = g_tbl_log2[sub_blk_size] - 2;
    int i_src = ref_pic->stride_luma;

    for (h = 0; h < cu_height; h += sub_blk_size) {
        int base_y = (y + h) << 4;
        for (w = 0; w < cu_width; w += sub_blk_size, asb_mv++) {
            int qpel_gmv_x = ((x + w) << 4) + (*asb_mv)[MV_X];
            int qpel_gmv_y = base_y         + (*asb_mv)[MV_Y];
            mc_core_luma(ref_pic, i_src, dstl + w, i_dstl, qpel_gmv_x, qpel_gmv_y, sub_blk_size, sub_blk_size, widx, max_posx, max_posy, max_val, 1);
        }
        dstl += sub_blk_size * i_dstl;
    }
}

void uavs3d_always_inline com_affine_mc_chroma(com_core_t *core, pel *dstc, int i_dstc, int x, int y, int cu_width, int cu_height, com_pic_t* ref_pic, int sub_blk_size, int lidx)
{
    int w, h;
    s32 (*asb_mv0)[MV_D] = core->affine_sb_mv[lidx];
    com_seqh_t *seqhdr = core->seqhdr;
    int max_val = (1 << seqhdr->bit_depth_internal) - 1;
    int max_posx = (seqhdr->pic_width  + 4) >> 1;
    int max_posy = (seqhdr->pic_height + 4) >> 1;
    int i_asb_mv = cu_width >> 2;
    int i_src = ref_pic->stride_chroma;

    if (sub_blk_size == 4) {
        s32(*asb_mv1)[MV_D] = asb_mv0 + i_asb_mv;
        for (h = 0; h < cu_height; h += 8) {
            int base_y = (y + h) << 4;
            for (w = 0; w < cu_width; w += 8, asb_mv0 += 2, asb_mv1 += 2) {
                int mv_scale_tmp_hor = (asb_mv0[0][MV_X] + asb_mv0[1][MV_X] + asb_mv1[0][MV_X] + asb_mv1[1][MV_X] + 2) >> 2;
                int mv_scale_tmp_ver = (asb_mv0[0][MV_Y] + asb_mv0[1][MV_Y] + asb_mv1[0][MV_Y] + asb_mv1[1][MV_Y] + 2) >> 2;
                int qpel_gmv_x = ((x + w) << 4) + mv_scale_tmp_hor;
                int qpel_gmv_y = base_y + mv_scale_tmp_ver;

                mc_core_chroma(ref_pic, i_src, dstc + w, i_dstc, qpel_gmv_x, qpel_gmv_y, 8, 4, 1, max_posx, max_posy, max_val, 1);
            }
            dstc += i_dstc << 2;
            asb_mv0 += i_asb_mv;
            asb_mv1 += i_asb_mv;
        }
    } else {
        for (h = 0; h < cu_height; h += 8) {
            int base_y = (y + h) << 4;
            for (w = 0; w < cu_width; w += 8, asb_mv0++) {
                int qpel_gmv_x = ((x + w) << 4) + (*asb_mv0)[MV_X];
                int qpel_gmv_y = base_y + (*asb_mv0)[MV_Y];
                mc_core_chroma(ref_pic, i_src, dstc + w, i_dstc, qpel_gmv_x, qpel_gmv_y, 8, 4, 1, max_posx, max_posy, max_val, 1);
            }
            dstc += i_dstc << 2;
        }
    }
}

void com_mc_affine(com_core_t *core, pel *pred_buf)
{
    com_pic_t *pic = core->pic;
    int x = core->cu_pix_x;
    int y = core->cu_pix_y;
    int cu_width = core->cu_width;
    int cu_height = core->cu_height;
    com_ref_pic_t(*refp)[REFP_NUM] = core->refp;
    s8 *refi = core->refi;
    com_pic_t *ref_pic;
    ALIGNED_32(pel pred_snd[MAX_CU_DIM + MAX_CU_DIM / 2]);
    int sub_blk_size;
    int bi = REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1]);
    int lidx, lidx_base = REFI_IS_VALID(refi[REFP_0]) ? REFP_0 : REFP_1;
    pel *dstl, *dstc;
    int i_dstl, i_dstc;
    int blk_dim = cu_width * cu_height;

    if (core->pichdr->affine_subblock_size_idx == 1 || (REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1]))) {
        sub_blk_size = 8;
    } else {
        sub_blk_size = 4;
    }

    lidx = lidx_base;

    for (int i = 0; i <= bi; i++) {
        ref_pic = refp[refi[lidx]][lidx].pic;

        if (i) {
            i_dstl = cu_width;
            dstl = pred_snd;
        } else if (!bi && !M32(core->cbfy)) {
            i_dstl = pic->stride_luma;
            dstl = pic->y + y * i_dstl + x;
        } else {
            i_dstl = cu_width;
            dstl = pred_buf;
        }
        com_affine_mc_luma(core, dstl, i_dstl, x, y, cu_width, cu_height, ref_pic, sub_blk_size, lidx);
        lidx = !lidx;
    }
    if (bi) {
        if (!M32(core->cbfy)) {
            i_dstl = pic->stride_luma;
            dstl = pic->y + y * i_dstl + x;
        } else {
            i_dstl = cu_width;
            dstl = pred_buf;
        }
        uavs3d_funs_handle.avg_pel[core->cu_log2w - 2](dstl, i_dstl, pred_buf, pred_snd, cu_width, cu_height);
    }

    lidx = lidx_base;

    for (int i = 0; i <= bi; i++) {
        ref_pic = refp[refi[lidx]][lidx].pic;

        if (i) {
            i_dstc = cu_width;
            dstc = pred_snd;
        } else if (!bi && !M16(core->cbfc)) {
            i_dstc = pic->stride_chroma;
            dstc = pic->uv + y / 2 * i_dstc + x;
        } else {
            i_dstc = cu_width;
            dstc = pred_buf + blk_dim;
        }
        com_affine_mc_chroma(core, dstc, i_dstc, x, y, cu_width, cu_height, ref_pic, sub_blk_size, lidx);
        lidx = !lidx;
    }
    if (bi) {
        if (!M16(core->cbfc)) {
            i_dstc = pic->stride_chroma;
            dstc = pic->uv + y / 2 * i_dstc + x;
        } else {
            i_dstc = cu_width;
            dstc = pred_buf + blk_dim;
        }
        uavs3d_funs_handle.avg_pel[core->cu_log2w - 2](dstc, i_dstc, pred_buf + blk_dim, pred_snd, cu_width, cu_height >> 1);
    }
}

void uavs3d_funs_init_mc_c() {
    int i;
    for (i = 0; i < BLOCK_WIDTH_TYPES_NUM; i++) {
        uavs3d_funs_handle.ipcpy[i] = uavs3d_if_cpy;
        uavs3d_funs_handle.ipflt[IPFILTER_H_8][i] = uavs3d_if_hor_luma;
        uavs3d_funs_handle.ipflt[IPFILTER_H_4][i] = uavs3d_if_hor_chroma;
        uavs3d_funs_handle.ipflt[IPFILTER_V_8][i] = uavs3d_if_ver_luma;
        uavs3d_funs_handle.ipflt[IPFILTER_V_4][i] = uavs3d_if_ver_chroma;
        uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_8][i] = uavs3d_if_hor_ver_luma;
        uavs3d_funs_handle.ipflt_ext[IPFILTER_EXT_4][i] = uavs3d_if_hor_ver_chroma;
        uavs3d_funs_handle.avg_pel[i]   = uavs3d_avg_pel;
    }
}
