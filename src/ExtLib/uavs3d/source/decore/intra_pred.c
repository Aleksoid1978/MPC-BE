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

#define COM_IPRED_CHK_CONV(mode) ((mode) == IPD_VER || (mode) == IPD_HOR || (mode) == IPD_DC || (mode) == IPD_BI)

void com_get_nbr_l(pel *dst, int ipm, int ipf, int x, int y, int width, int height, pel *srcT, pel *srcL, int s_src, u16 avail_cu, int scup, com_scu_t * map_scu, int i_scu, int bit_depth)
{
    int  i;
    int  width_in_scu = width >> MIN_CU_LOG2;
    int  height_in_scu = height >> MIN_CU_LOG2;
    pel *srcTL = srcT - 1;
    pel *left = dst - 1;
    pel *up = dst + 1;

    int pad_le_in_scu = height_in_scu;
    int pad_up_in_scu = width_in_scu;
    int default_val = 1 << (bit_depth - 1);
    int pad_range = COM_MAX(width, height) * 2 + 4;

    if (ipm < IPD_HOR || ipf) {
        if (IS_AVAIL(avail_cu, AVAIL_UP)) {
            com_scu_t *map_up_scu = map_scu + scup - i_scu + width_in_scu;
         
            memcpy(up, srcT, width * sizeof(pel));
            up += width, srcT += width;

            for (i = 0; i < pad_up_in_scu; i++, up += MIN_CU_SIZE, srcT += MIN_CU_SIZE) {
                if ((map_up_scu++)->coded) {
                    com_mcpy_4pel(up, srcT);
                } else {
                    break;
                }
            }
            com_mset_pel(up, up[-1], pad_range - (width + i * MIN_CU_SIZE));
        } else {
            com_mset_pel(up, default_val, pad_range);
        }
    }
    if (ipm <= IPD_BI || ipm > IPD_VER || ipf) {
        if (IS_AVAIL(avail_cu, AVAIL_LE)) {
            com_scu_t *map_left_scu = map_scu + scup - 1 + height_in_scu * i_scu;
            pel *src = srcL;

            for (i = 0; i < height; i += 4) {
                *left-- = *src;
                src += s_src;
                *left-- = *src;
                src += s_src;
                *left-- = *src;
                src += s_src;
                *left-- = *src;
                src += s_src;
            }
            for (i = 0; i < pad_le_in_scu; i++, map_left_scu += i_scu) {
                if (map_left_scu->coded) {
                    *left-- = *src;
                    src += s_src;
                    *left-- = *src;
                    src += s_src;
                    *left-- = *src;
                    src += s_src;
                    *left-- = *src;
                    src += s_src;
                } else {
                    break;
                }
            }
            int pads = pad_range - height - i * MIN_CU_SIZE;
            com_mset_pel(left - pads + 1, left[1], pads);
        } else {
            com_mset_pel(left - pad_range + 1, default_val, pad_range);
        }
    }

    if (IS_AVAIL(avail_cu, AVAIL_UL)) {
        dst[0] = srcTL[0];
    } else if (IS_AVAIL(avail_cu, AVAIL_UP)) {
        dst[0] = srcTL[1];
    } else if (IS_AVAIL(avail_cu, AVAIL_LE)) {
        dst[0] = srcL[0];
    } else {
        dst[0] = default_val;
    }
}

void com_get_nbr_c(pel *dst, int ipm_c, int ipm, int x, int y, int width, int height, pel *srcT, pel *srcL, int s_src, u16 avail_cu, int scup, com_scu_t * map_scu, int i_scu, int bit_depth)
{
    int  i;
    int  width_in_scu = width >> (MIN_CU_LOG2 - 1);
    int  height_in_scu = height >> (MIN_CU_LOG2 - 1);
    pel *srcTL = srcT - 2;
    pel *up   = dst + 2;
    pel *left = dst - 2;

    int pad_le_in_scu = height_in_scu;
    int pad_up_in_scu = width_in_scu;

    int width2 = width << 1;
    int height2 = height << 1;
    int default_val = 1 << (bit_depth - 1);
    int pad_range = COM_MAX(width, height) * 2 + 4;

    if (ipm_c != IPD_HOR_C && (ipm_c != IPD_DM_C || ipm < IPD_HOR)) {
        if (IS_AVAIL(avail_cu, AVAIL_UP)) {
            com_scu_t *map_up_scu = map_scu + scup - i_scu + width_in_scu;

            memcpy(up, srcT, width2 * sizeof(pel));
            up += width2, srcT += width2;

            for (i = 0; i < pad_up_in_scu; i++, up += MIN_CU_SIZE, srcT += MIN_CU_SIZE) {
                if ((map_up_scu++)->coded) {
                    com_mcpy_4pel(up, srcT);
                } else {
                    break;
                }
            }
            const int shift = sizeof(pel) * 8;
            int uv_pixel = (up[-1] << shift) + up[-2];
            com_mset_2pel(up, uv_pixel, pad_range - (width + i * MIN_CU_SIZE / 2));
        } else {
            com_mset_pel(up, default_val, pad_range << 1);
        }
    }
    if (ipm_c != IPD_VER_C && (ipm_c != IPD_DM_C || ipm <= IPD_BI || ipm > IPD_VER)) {
        if (IS_AVAIL(avail_cu, AVAIL_LE)) {
            com_scu_t *map_left_scu = map_scu + scup - 1 + height_in_scu * i_scu;
            pel *src = srcL;

            for (i = 0; i < height; i+=2) {
                com_mcpy_2pel(left, src); src += s_src; left -= 2;
                com_mcpy_2pel(left, src); src += s_src; left -= 2;
            }
            for (i = 0; i < pad_le_in_scu; i++, map_left_scu += i_scu) {
                if (map_left_scu->coded) {
                    com_mcpy_2pel(left, src); src += s_src; left -= 2;
                    com_mcpy_2pel(left, src); src += s_src; left -= 2;
                } else {
                    break;
                }
            }

            const int shift = sizeof(pel) * 8;
            int uv_pixel = (left[3] << shift) + left[2];
            int pads = pad_range - height - i * MIN_CU_SIZE / 2;
            com_mset_2pel(left - pads * 2 + 2, uv_pixel, pads);
        } else {
            com_mset_pel(left - (pad_range << 1) + 2, default_val, pad_range << 1);
        }
    }
    if (IS_AVAIL(avail_cu, AVAIL_UL)) {
        dst[0] = srcTL[0];
        dst[1] = srcTL[1];
    } else if (IS_AVAIL(avail_cu, AVAIL_UP)) {
        dst[0] = srcTL[2];
        dst[1] = srcTL[3];
    } else if (IS_AVAIL(avail_cu, AVAIL_LE)) {
        dst[0] = srcL[0];
        dst[1] = srcL[1];
    } else {
        dst[0] = dst[1] = default_val;
    }
}

void ipred_hor(pel *src, pel *dst, int i_dst, int w, int h)
{
    for(int i = 0; i < h; i++) {
        com_mset_pel(dst, src[-i], w);
        dst += i_dst;
    }
}

void ipred_hor_uv(pel *src, pel *dst, int i_dst, int w, int h)
{
    int i;
    int shift = sizeof(pel) * 8;
    
    for (i = 0; i < h; i++) {
        int uv_pixel = (src[1] << shift) + src[0];
        com_mset_2pel(dst, uv_pixel, w);
        dst += i_dst;
        src -= 2;
    }
}

void ipred_vert(pel *src, pel *dst, int i_dst, int w, int h)
{
    while(h--) {
        memcpy(dst, src, w * sizeof(pel));
        dst += i_dst;
    }
}

void ipred_dc(pel *src, pel *dst, int i_dst, int w, int h, u16 avail_cu, int bit_depth)
{
    uavs3d_assert(g_tbl_log2[w] >= 2);
    uavs3d_assert(g_tbl_log2[h] >= 2);

    int dc = 0;
    int i, j;
    if(IS_AVAIL(avail_cu, AVAIL_LE)) {
        for (i = 0; i < h; i++) {
            dc += src[-i-1];
        }
        if(IS_AVAIL(avail_cu, AVAIL_UP)) {
            for (j = 0; j < w; j++) {
                dc += src[j+1];
            }
            dc = (dc + ((w + h) >> 1)) * (4096 / (w + h)) >> 12;
        } else {
            dc = (dc + (h >> 1)) >> g_tbl_log2[h];
        }
    } else if(IS_AVAIL(avail_cu, AVAIL_UP)) {
        for (j = 0; j < w; j++) {
            dc += src[j+1];
        }
        dc = (dc + (w >> 1)) >> g_tbl_log2[w];
    } else {     
        dc = 1 << (bit_depth - 1);
    }

    while(h--) {
        com_mset_pel(dst, dc, w);
        dst += i_dst;
    }
}

void ipred_dc_uv(pel *src, pel *dst, int i_dst, int w, int h, u16 avail_cu, int bit_depth)
{
    uavs3d_assert(g_tbl_log2[w] >= 2);
    uavs3d_assert(g_tbl_log2[h] >= 2);

    int dc_u = 0, dc_v = 0;
    int i, j;
    int w2 = w << 1;
    int h2 = h << 1;

    if (IS_AVAIL(avail_cu, AVAIL_LE)) {
        for (i = 0; i < h2; i += 2) {
            dc_u += src[-i - 2];
            dc_v += src[-i - 1];
        }
        if (IS_AVAIL(avail_cu, AVAIL_UP)) {
            for (j = 0; j < w2; j += 2) {
                dc_u += src[j + 2];
                dc_v += src[j + 3];
            }
            dc_u = (dc_u + ((w + h) >> 1)) * (4096 / (w + h)) >> 12;
            dc_v = (dc_v + ((w + h) >> 1)) * (4096 / (w + h)) >> 12;
        } else {
            dc_u = (dc_u + (h >> 1)) >> g_tbl_log2[h];
            dc_v = (dc_v + (h >> 1)) >> g_tbl_log2[h];
        }
    } else if (IS_AVAIL(avail_cu, AVAIL_UP)) {
        for (j = 0; j < w2; j += 2) {
            dc_u += src[j + 2];
            dc_v += src[j + 3];
        }
        dc_u = (dc_u + (w >> 1)) >> g_tbl_log2[w];
        dc_v = (dc_v + (w >> 1)) >> g_tbl_log2[w];
    } else {
        dc_u = dc_v = 1 << (bit_depth - 1);
    }

    int shift = sizeof(pel) * 8;
    int uv_pixel = (dc_v << shift) + dc_u;

    while (h--) {
        com_mset_2pel(dst, uv_pixel, w);
        dst += i_dst;
    }
}

void ipred_plane(pel *src, pel *dst, int i_dst, int w, int h, int bit_depth)
{
    uavs3d_assert(g_tbl_log2[w] >= 2);
    uavs3d_assert(g_tbl_log2[h] >= 2);

    pel *rsrc;
    int  coef_h = 0, coef_v = 0;
    int  a, b, c, x, y;
    int  w2 = w >> 1;
    int  h2 = h >> 1;
    int  ib_mult[5]  = { 13, 17, 5, 11, 23 };
    int  ib_shift[5] = { 7, 10, 11, 15, 19 };
    int  idx_w = g_tbl_log2[w] - 2;
    int  idx_h = g_tbl_log2[h] - 2;
    int  im_h, is_h, im_v, is_v, temp;
    int  max_pel = (1 << bit_depth) - 1;
    int  val;

    im_h = ib_mult[idx_w];
    is_h = ib_shift[idx_w];
    im_v = ib_mult[idx_h];
    is_v = ib_shift[idx_h];
    rsrc = src + w2;

    for (x = 1; x < w2 + 1; x++) {
        coef_h += x * (rsrc[x] - rsrc[-x]);
    }
    rsrc = src - h2;
    for (y = 1; y < h2 + 1; y++) {
        coef_v += y * (rsrc[-y] - rsrc[y]);
    }
    a = (src[-h] + src[w]) << 4;
    b = ((coef_h << 5) * im_h + (1 << (is_h - 1))) >> is_h;
    c = ((coef_v << 5) * im_v + (1 << (is_v - 1))) >> is_v;
    temp = a - (h2 - 1) * c - (w2 - 1) * b + 16;
    
    for (y = 0; y < h; y++) {
        int temp2 = temp;
        for (x = 0; x < w; x++) {
            val = temp2 >> 5;
            dst[x] = (pel)COM_CLIP3(0, max_pel, val);
            temp2 += b;
        }
        temp += c;
        dst += i_dst;
    }
}

void ipred_plane_ipf(pel *src, s16 *dst, int w, int h)
{
    uavs3d_assert(g_tbl_log2[w] >= 2);
    uavs3d_assert(g_tbl_log2[h] >= 2);

    pel *rsrc;
    int  coef_h = 0, coef_v = 0;
    int  a, b, c, x, y;
    int  w2 = w >> 1;
    int  h2 = h >> 1;
    int  ib_mult[5] = { 13, 17, 5, 11, 23 };
    int  ib_shift[5] = { 7, 10, 11, 15, 19 };
    int  idx_w = g_tbl_log2[w] - 2;
    int  idx_h = g_tbl_log2[h] - 2;
    int  im_h, is_h, im_v, is_v, temp;
    im_h = ib_mult[idx_w];
    is_h = ib_shift[idx_w];
    im_v = ib_mult[idx_h];
    is_v = ib_shift[idx_h];
    rsrc = src + w2;

    for (x = 1; x < w2 + 1; x++) {
        coef_h += x * (rsrc[x] - rsrc[-x]);
    }
    rsrc = src - h2;
    for (y = 1; y < h2 + 1; y++) {
        coef_v += y * (rsrc[-y] - rsrc[y]);
    }
    a = (src[-h] + src[w]) << 4;
    b = ((coef_h << 5) * im_h + (1 << (is_h - 1))) >> is_h;
    c = ((coef_v << 5) * im_v + (1 << (is_v - 1))) >> is_v;
    temp = a - (h2 - 1) * c - (w2 - 1) * b + 16;

    for (y = 0; y < h; y++) {
        int temp2 = temp;
        for (x = 0; x < w; x++) {
            dst[x] = (s16)(temp2 >> 5);
            temp2 += b;
        }
        temp += c;
        dst += w;
    }
}

void ipred_plane_uv(pel *src, pel *dst, int i_dst, int w, int h, int bit_depth)
{
    uavs3d_assert(g_tbl_log2[w] >= 2);
    uavs3d_assert(g_tbl_log2[h] >= 2);

    pel *rsrc;
    int  coef_hor_u = 0, coef_ver_u = 0;
    int  coef_hor_v = 0, coef_ver_v = 0;
    int  a_u, b_u, c_u, a_v, b_v, c_v, x, y;
    int  w2 = w >> 1;
    int  h2 = h >> 1;
    int  width2  = w << 1;
    int  height2 = h << 1;
    int  ib_mult[5] = { 13, 17, 5, 11, 23 };
    int  ib_shift[5] = { 7, 10, 11, 15, 19 };
    int  idx_w = g_tbl_log2[w] - 2;
    int  idx_h = g_tbl_log2[h] - 2;
    int  im_h, is_h, im_v, is_v;
    int  temp_u, temp_v;
    int  max_pel = (1 << bit_depth) - 1;
    int  val_u, val_v;

    im_h = ib_mult[idx_w];
    is_h = ib_shift[idx_w];
    im_v = ib_mult[idx_h];
    is_v = ib_shift[idx_h];
    rsrc = src + w;
    for (x = 1; x < w2 + 1; x++)
    {
        int x2 = x << 1;
        coef_hor_u += x * (rsrc[x2] - rsrc[-x2]);
        coef_hor_v += x * (rsrc[x2 + 1] - rsrc[-x2 + 1]);
    }
    rsrc = src - h;
    for (y = 1; y < h2 + 1; y++)
    {
        int y2 = y << 1;
        coef_ver_u += y * (rsrc[-y2] - rsrc[y2]);
        coef_ver_v += y * (rsrc[-y2 + 1] - rsrc[y2 + 1]);
    }
    a_u = (src[-height2] + src[width2]) << 4;
    b_u = ((coef_hor_u << 5) * im_h + (1 << (is_h - 1))) >> is_h;
    c_u = ((coef_ver_u << 5) * im_v + (1 << (is_v - 1))) >> is_v;
    a_v = (src[-height2 + 1] + src[width2 + 1]) << 4;
    b_v = ((coef_hor_v << 5) * im_h + (1 << (is_h - 1))) >> is_h;
    c_v = ((coef_ver_v << 5) * im_v + (1 << (is_v - 1))) >> is_v;
    temp_u = a_u - (h2 - 1) * c_u - (w2 - 1) * b_u + 16;
    temp_v = a_v - (h2 - 1) * c_v - (w2 - 1) * b_v + 16;

    for (y = 0; y < h; y++) {
        int temp2_u = temp_u;
        int temp2_v = temp_v;
        for (x = 0; x < width2; x += 2) {
            val_u = temp2_u >> 5;
            val_v = temp2_v >> 5;
            temp2_u += b_u;
            temp2_v += b_v;
            dst[x    ] = COM_CLIP3(0, max_pel, val_u);
            dst[x + 1] = COM_CLIP3(0, max_pel, val_v);
        }
        temp_u += c_u;
        temp_v += c_v;
        dst += i_dst;
    }
}

void ipred_bi(pel *src, pel *dst, int i_dst, int w, int h, int bit_depth)
{
    uavs3d_assert(g_tbl_log2[w] >= 2);
    uavs3d_assert(g_tbl_log2[h] >= 2);

    int x, y;
    int ishift_x = g_tbl_log2[w];
    int ishift_y = g_tbl_log2[h];
    int ishift = COM_MIN(ishift_x, ishift_y);
    int ishift_xy = ishift_x + ishift_y + 1;
    int offset = 1 << (ishift_x + ishift_y);
    int a, b, c, wt, tmp;
    int ref_up[MAX_CU_SIZE], ref_le[MAX_CU_SIZE], up[MAX_CU_SIZE], le[MAX_CU_SIZE], wy[MAX_CU_SIZE];
    int wc, tbl_wc[6] = {-1, 21, 13, 7, 4, 2};
    int max_pel = (1 << bit_depth) - 1;
    int val;

    wc = ishift_x > ishift_y ? ishift_x - ishift_y : ishift_y - ishift_x;
    uavs3d_assert(wc <= 5);

    wc = tbl_wc[wc];
    for( x = 0; x < w; x++ ) {
        ref_up[x] = src[x + 1];
    }
    for( y = 0; y < h; y++ ) {
        ref_le[y] = src[-y - 1];
    }

    a = src[w];
    b = src[-h];
    c = (w == h) ? (a + b + 1) >> 1 : (((a << ishift_x) + (b << ishift_y)) * wc + (1 << (ishift + 5))) >> (ishift + 6);
    wt = (c << 1) - a - b;

    for( x = 0; x < w; x++ ) {
        up[x] = b - ref_up[x];
        ref_up[x] <<= ishift_y;
    }
    tmp = 0;
    for( y = 0; y < h; y++ ) {
        le[y] = a - ref_le[y];
        ref_le[y] <<= ishift_x;
        wy[y] = tmp;
        tmp += wt;
    }
    for( y = 0; y < h; y++ ) {
        int predx = ref_le[y];
        int wxy = 0;
        for( x = 0; x < w; x++ ) {
            predx += le[y];
            ref_up[x] += up[x];
            val = ((predx << ishift_y) + (ref_up[x] << ishift_x) + wxy + offset) >> ishift_xy;
            dst[x] = (pel)COM_CLIP3(0, max_pel, val);
            wxy += wy[y];
        }
        dst += i_dst;
    }
}

void ipred_bi_ipf(pel *src, s16 *dst, int w, int h)
{
    uavs3d_assert(g_tbl_log2[w] >= 2);
    uavs3d_assert(g_tbl_log2[h] >= 2);

    int x, y;
    int ishift_x = g_tbl_log2[w];
    int ishift_y = g_tbl_log2[h];
    int ishift = COM_MIN(ishift_x, ishift_y);
    int ishift_xy = ishift_x + ishift_y + 1;
    int offset = 1 << (ishift_x + ishift_y);
    int a, b, c, wt, tmp;
    int ref_up[MAX_CU_SIZE], ref_le[MAX_CU_SIZE], up[MAX_CU_SIZE], le[MAX_CU_SIZE], wy[MAX_CU_SIZE];
    int wc, tbl_wc[6] = { -1, 21, 13, 7, 4, 2 };
    wc = ishift_x > ishift_y ? ishift_x - ishift_y : ishift_y - ishift_x;
    uavs3d_assert(wc <= 5);

    wc = tbl_wc[wc];
    for (x = 0; x < w; x++) {
        ref_up[x] = src[x + 1];
    }
    for (y = 0; y < h; y++) {
        ref_le[y] = src[-y - 1];
    }

    a = src[w];
    b = src[-h];
    c = (w == h) ? (a + b + 1) >> 1 : (((a << ishift_x) + (b << ishift_y)) * wc + (1 << (ishift + 5))) >> (ishift + 6);
    wt = (c << 1) - a - b;

    for (x = 0; x < w; x++) {
        up[x] = b - ref_up[x];
        ref_up[x] <<= ishift_y;
    }
    tmp = 0;
    for (y = 0; y < h; y++) {
        le[y] = a - ref_le[y];
        ref_le[y] <<= ishift_x;
        wy[y] = tmp;
        tmp += wt;
    }
    for (y = 0; y < h; y++) {
        int predx = ref_le[y];
        int wxy = 0;
        for (x = 0; x < w; x++) {
            predx += le[y];
            ref_up[x] += up[x];
            dst[x] = (s16)(((predx << ishift_y) + (ref_up[x] << ishift_x) + wxy + offset) >> ishift_xy);
            wxy += wy[y];
        }
        dst += w;
    }
}

void ipred_bi_uv(pel *src, pel *dst, int i_dst, int w, int h, int bit_depth)
{
    uavs3d_assert(g_tbl_log2[w] >= 2);
    uavs3d_assert(g_tbl_log2[h] >= 2);

    int x, y;
    int ishift_x = g_tbl_log2[w];
    int ishift_y = g_tbl_log2[h];
    int ishift = COM_MIN(ishift_x, ishift_y);
    int ishift_xy = ishift_x + ishift_y + 1;
    int offset = 1 << (ishift_x + ishift_y);
    int a_u, b_u, c_u, wt_u, tmp_u;
    int a_v, b_v, c_v, wt_v, tmp_v;
    int ref_up[MAX_CU_SIZE], ref_le[MAX_CU_SIZE], up[MAX_CU_SIZE], le[MAX_CU_SIZE], wy[MAX_CU_SIZE];
    int wc, tbl_wc[6] = { -1, 21, 13, 7, 4, 2 };
    int w2 = w << 1;
    int h2 = h << 1;
    int max_pel = (1 << bit_depth) - 1;
    int val_u, val_v;

    wc = ishift_x > ishift_y ? ishift_x - ishift_y : ishift_y - ishift_x;
    uavs3d_assert(wc <= 5);

    wc = tbl_wc[wc];
    for (x = 0; x < w2; x++) {
        ref_up[x] = src[x + 2];  
    }
    for (y = 0; y < h2; y += 2) {
        ref_le[y    ] = src[-y - 2];
        ref_le[y + 1] = src[-y - 1];
    }

    a_u = src[w2];
    b_u = src[-h2];
    c_u = (w == h) ? (a_u + b_u + 1) >> 1 : (((a_u << ishift_x) + (b_u << ishift_y)) * wc + (1 << (ishift + 5))) >> (ishift + 6);
    wt_u = (c_u << 1) - a_u - b_u;

    a_v = src[w2 + 1];
    b_v = src[-h2 + 1];
    c_v = (w == h) ? (a_v + b_v + 1) >> 1 : (((a_v << ishift_x) + (b_v << ishift_y)) * wc + (1 << (ishift + 5))) >> (ishift + 6);
    wt_v = (c_v << 1) - a_v - b_v;

    for (x = 0; x < w2; x += 2) {
        up[x    ] = b_u - ref_up[x];
        up[x + 1] = b_v - ref_up[x + 1];
        ref_up[x    ] <<= ishift_y;
        ref_up[x + 1] <<= ishift_y;
    }
    tmp_u = tmp_v = 0;
    for (y = 0; y < h2; y += 2) {
        le[y    ] = a_u - ref_le[y];
        le[y + 1] = a_v - ref_le[y + 1];
        ref_le[y    ] <<= ishift_x;
        ref_le[y + 1] <<= ishift_x;
        wy[y    ] = tmp_u;
        wy[y + 1] = tmp_v;
        tmp_u += wt_u;
        tmp_v += wt_v;
    }
    for (y = 0; y < h; y++) {
        int y2 = y << 1;
        int predx_u = ref_le[y2    ];
        int predx_v = ref_le[y2 + 1];
        int wxy_u = 0;
        int wxy_v = 0;
        for (x = 0; x < w2; x += 2) {
            predx_u += le[y2];
            predx_v += le[y2 + 1];
            ref_up[x    ] += up[x];
            ref_up[x + 1] += up[x + 1];
            val_u = ((predx_u << ishift_y) + (ref_up[x    ] << ishift_x) + wxy_u + offset) >> ishift_xy;
            val_v = ((predx_v << ishift_y) + (ref_up[x + 1] << ishift_x) + wxy_v + offset) >> ishift_xy;
            dst[x    ] = COM_CLIP3(0, max_pel, val_u);
            dst[x + 1] = COM_CLIP3(0, max_pel, val_v);
            wxy_u += wy[y2];
            wxy_v += wy[y2 + 1];
        }
        dst += i_dst;
    }
}

void down_sample_uv(int uiCWidth, int uiCHeight, int bitDept, pel *pSrc, int uiSrcStride, pel *pDst, int uiDstStride)
{
    int val_u, val_v;

    for (int j = 0; j < uiCHeight; j++)
    {
        for (int i = 0; i < uiCWidth; i++)
        {
            int i2 = i << 1;
            int i4 = i << 2;
            if (i == 0)
            {
                val_u = (pSrc[i4] + pSrc[i4 + uiSrcStride] + 1) >> 1;
                val_v = (pSrc[i4 + 1] + pSrc[i4 + 1 + uiSrcStride] + 1) >> 1;
            }
            else
            {
                val_u = (pSrc[i4] * 2 + pSrc[i4 + 2] + pSrc[i4 - 2] + pSrc[i4 + uiSrcStride] * 2
                         + pSrc[i4 + uiSrcStride + 2] + pSrc[i4 + uiSrcStride - 2] + 4) >> 3;
                val_v = (pSrc[i4 + 1] * 2 + pSrc[i4 + 3] + pSrc[i4 - 1] + pSrc[i4 + 1 + uiSrcStride] * 2
                    + pSrc[i4 + uiSrcStride + 3] + pSrc[i4 + uiSrcStride - 1] + 4) >> 3;
            }

            pDst[i2    ] = val_u;
            pDst[i2 + 1] = val_v;
        }
        pDst += uiDstStride;
        pSrc += uiSrcStride * 2;
    }
}

pel xGetLumaBorderPixel(int uiIdx, int bAbovePixel, int uiCWidth, int uiCHeight, int bAboveAvail, int bLeftAvail, pel *luma)
{
    pel *pSrc = NULL;
    pel dstPoint = -1;

    if (bAbovePixel) {
        if (bAboveAvail) {
            pSrc = luma + 1;
            int i = uiIdx;
            if (i < (uiCWidth << 1)) {
                if (i == 0 && !bLeftAvail) {
                    dstPoint = (3 * pSrc[i] + pSrc[i + 1] + 2) >> 2;
                } else {
                    dstPoint = (2 * pSrc[i] + pSrc[i - 1] + pSrc[i + 1] + 2) >> 2;
                }
            }
        }
    } else {
        if (bLeftAvail) {
            pSrc = luma - 1;
            int j = uiIdx;
            if (j < (uiCHeight << 1)) {
                dstPoint = (pSrc[-j] + pSrc[-j - 1] + 1) >> 1;
            }
        }
    }
    uavs3d_assert(dstPoint >= 0);
    return dstPoint;
}

#define xGetSrcPixel(idx, bAbovePixel)  xGetLumaBorderPixel((idx), (bAbovePixel), uiCWidth, uiCHeight, bAboveAvaillable, bLeftAvaillable, luma)
#define xExchange(a, b, type) {type exTmp; exTmp = (a); (a) = (b); (b) = exTmp;}

void get_tscpm_params(int *a_u, int *b_u, int *a_v, int *b_v, int *iShift, int bAbove, int bLeft, int uiCWidth, int uiCHeight, int bitDept, pel *src, pel *luma)
{
    int bAboveAvaillable = bAbove;
    int bLeftAvaillable = bLeft;

    pel *pCur = NULL;
    
    unsigned uiInternalBitDepth = bitDept;

    int minDim = bLeftAvaillable && bAboveAvaillable ? min(uiCHeight, uiCWidth) : (bLeftAvaillable ? uiCHeight : uiCWidth);
    int numSteps = minDim;
    int yMax[2] = { 0, 0 };
    int xMax = -COM_INT_MAX;
    int yMin[2] = { 0, 0 };
    int xMin = COM_INT_MAX;

    // four points start
    int iRefPointLuma[4] = { -1, -1, -1, -1 };
    int iRefPointChroma[2][4] = { { -1, -1, -1, -1 },{ -1, -1, -1, -1 } };

    if (bAboveAvaillable) {
        pCur = src + 2;
        int idx = (((numSteps - 1) * uiCWidth) / minDim) << 1;
        iRefPointLuma[0] = xGetSrcPixel(0, 1); // pSrc[0];
        iRefPointChroma[0][0] = pCur[0]; // U
        iRefPointChroma[1][0] = pCur[1]; // V
        iRefPointLuma[1] = xGetSrcPixel(idx, 1); // pSrc[idx];
        iRefPointChroma[0][1] = pCur[idx];
        iRefPointChroma[1][1] = pCur[idx + 1];
        // using 4 points when only one border
        if (!bLeftAvaillable && uiCWidth >= 4)
        {
            int uiStep = uiCWidth >> 1;
            for (int i = 0; i < 4; i++)
            {
                iRefPointLuma[i] = xGetSrcPixel(i * uiStep, 1); // pSrc[i * uiStep];
                iRefPointChroma[0][i] = pCur[i * uiStep];
                iRefPointChroma[1][i] = pCur[i * uiStep + 1];
            }
        }
    }
    if (bLeftAvaillable)
    {
        pCur = src - 2;
        int idx = (((numSteps - 1) * uiCHeight) / minDim) << 1;
        iRefPointLuma[2] = xGetSrcPixel(0, 0); // pSrc[0];
        iRefPointChroma[0][2] = pCur[0];
        iRefPointChroma[1][2] = pCur[1];
        iRefPointLuma[3] = xGetSrcPixel(idx, 0); // pSrc[idx * iSrcStride];
        iRefPointChroma[0][3] = pCur[-idx];
        iRefPointChroma[1][3] = pCur[-idx + 1];
        // using 4 points when only one border
        if (!bAboveAvaillable && uiCHeight >= 4)
        {
            int uiStep = uiCHeight >> 1;
            for (int i = 0; i < 4; i++)
            {
                iRefPointLuma[i] = xGetSrcPixel(i * uiStep, 0); // pSrc[i * uiStep * iSrcStride];
                iRefPointChroma[0][i] = pCur[-i * uiStep];
                iRefPointChroma[1][i] = pCur[-i * uiStep + 1];
            }
        }
    }

    if ((bAboveAvaillable &&  bLeftAvaillable)
            || (bAboveAvaillable && !bLeftAvaillable  && uiCWidth >= 4)
            || (bLeftAvaillable && !bAboveAvaillable && uiCHeight >= 4) )
    {
        int minGrpIdx[2] = { 0, 2 };
        int maxGrpIdx[2] = { 1, 3 };
        int *tmpMinGrp = minGrpIdx;
        int *tmpMaxGrp = maxGrpIdx;
        if (iRefPointLuma[tmpMinGrp[0]] > iRefPointLuma[tmpMinGrp[1]]) xExchange(tmpMinGrp[0], tmpMinGrp[1], int);
        if (iRefPointLuma[tmpMaxGrp[0]] > iRefPointLuma[tmpMaxGrp[1]]) xExchange(tmpMaxGrp[0], tmpMaxGrp[1], int);
        if (iRefPointLuma[tmpMinGrp[0]] > iRefPointLuma[tmpMaxGrp[1]]) xExchange(tmpMinGrp,    tmpMaxGrp,    int *);
        if (iRefPointLuma[tmpMinGrp[1]] > iRefPointLuma[tmpMaxGrp[0]]) xExchange(tmpMinGrp[1], tmpMaxGrp[0], int);

        uavs3d_assert(iRefPointLuma[tmpMaxGrp[0]] >= iRefPointLuma[tmpMinGrp[0]]);
        uavs3d_assert(iRefPointLuma[tmpMaxGrp[0]] >= iRefPointLuma[tmpMinGrp[1]]);
        uavs3d_assert(iRefPointLuma[tmpMaxGrp[1]] >= iRefPointLuma[tmpMinGrp[0]]);
        uavs3d_assert(iRefPointLuma[tmpMaxGrp[1]] >= iRefPointLuma[tmpMinGrp[1]]);

        xMin = (iRefPointLuma  [tmpMinGrp[0]] + iRefPointLuma  [tmpMinGrp[1]] + 1 )>> 1;
        xMax = (iRefPointLuma  [tmpMaxGrp[0]] + iRefPointLuma  [tmpMaxGrp[1]] + 1 )>> 1;

        yMin[0] = (iRefPointChroma[0][tmpMinGrp[0]] + iRefPointChroma[0][tmpMinGrp[1]] + 1) >> 1;
        yMin[1] = (iRefPointChroma[1][tmpMinGrp[0]] + iRefPointChroma[1][tmpMinGrp[1]] + 1) >> 1;
        
        yMax[0] = (iRefPointChroma[0][tmpMaxGrp[0]] + iRefPointChroma[0][tmpMaxGrp[1]] + 1) >> 1;
        yMax[1] = (iRefPointChroma[1][tmpMaxGrp[0]] + iRefPointChroma[1][tmpMaxGrp[1]] + 1) >> 1;
    }
    else if (bAboveAvaillable)
    {
        for (int k = 0; k < 2; k++)
        {
            if (iRefPointLuma[k] > xMax)
            {
                xMax = iRefPointLuma[k];
                yMax[0] = iRefPointChroma[0][k];
                yMax[1] = iRefPointChroma[1][k];
            }
            if (iRefPointLuma[k] < xMin)
            {
                xMin = iRefPointLuma[k];
                yMin[0] = iRefPointChroma[0][k];
                yMin[1] = iRefPointChroma[1][k];
            }
        }
    }
    else if (bLeftAvaillable)
    {
        for (int k = 2; k < 4; k++)
        {
            if (iRefPointLuma[k] > xMax)
            {
                xMax = iRefPointLuma[k];
                yMax[0] = iRefPointChroma[0][k];
                yMax[1] = iRefPointChroma[1][k];
            }
            if (iRefPointLuma[k] < xMin)
            {
                xMin = iRefPointLuma[k];
                yMin[0] = iRefPointChroma[0][k];
                yMin[1] = iRefPointChroma[1][k];
            }
        }
    }
    // four points end

    if (bLeftAvaillable || bAboveAvaillable)
    {
        *a_u = *a_v = 0;
        *iShift = 16;
        int diff = xMax - xMin;
        int add = 0;
        int shift = 0;
        if (diff > 64)
        {
            shift = (uiInternalBitDepth > 8) ? uiInternalBitDepth - 6 : 2;
            add = shift ? 1 << (shift - 1) : 0;
            diff = (diff + add) >> shift;

            if (uiInternalBitDepth == 10)
            {
                uavs3d_assert(shift == 4 && add == 8); // for default 10bit
            }
        }

        if (diff > 0)
        {
            *a_u = ((yMax[0] - yMin[0]) * g_tbl_ai_tscpm_div[diff - 1] + add) >> shift;
            *a_v = ((yMax[1] - yMin[1]) * g_tbl_ai_tscpm_div[diff - 1] + add) >> shift;
        }
        *b_u = yMin[0] - (((s64)(*a_u) * xMin) >> (*iShift));
        *b_v = yMin[1] - (((s64)(*a_v) * xMin) >> (*iShift));
    }
    if (!bLeftAvaillable && !bAboveAvaillable)
    {
        *a_u = *a_v = 0;
        *b_u = *b_v = 1 << (uiInternalBitDepth - 1);
        *iShift = 0;
        return;
    }
}

void tscpm_linear_transform(pel *pSrc, int iSrcStride, pel *pDst, int iDstStride, int a_u, int b_u, int a_v, int b_v, int iShift, int uiWidth, int uiHeight, int bitDept)
{
    int maxResult = (1 << bitDept) - 1;

    iShift = (iShift >= 0 ? iShift : 0);

    for (int j = 0; j < uiHeight; j++)
    {
        for (int i = 0; i < uiWidth; i++)
        {
            int i2 = i << 1;
            int tempValue_u = (((s64)a_u * pSrc[i]) >> iShift) + b_u;
            int tempValue_v = (((s64)a_v * pSrc[i]) >> iShift) + b_v;
            pDst[i2    ] = COM_CLIP3(0, maxResult, tempValue_u);
            pDst[i2 + 1] = COM_CLIP3(0, maxResult, tempValue_v);
        }
        pDst += iDstStride;
        pSrc += iSrcStride;
    }
}

void ipred_tscpm(pel *dst, int i_dst, pel *src, pel* luma_nb, pel *luma_rec, int i_luma_rec, int uiWidth, int uiHeight, int bAbove, int bLeft, int bitDept)
{
    int a_u, b_u, shift;   // parameters of Linear Model : a, b, shift
    int a_v, b_v;
    pel tmp_buf[MAX_CU_SIZE * MAX_CU_SIZE * 2];
    int tmp_stride = uiWidth * 2 * 2;

    get_tscpm_params(&a_u, &b_u, &a_v, &b_v, &shift, bAbove, bLeft, uiWidth, uiHeight, bitDept, src, luma_nb);
    tscpm_linear_transform(luma_rec, i_luma_rec, tmp_buf, tmp_stride, a_u, b_u, a_v, b_v, shift, uiWidth << 1, uiHeight << 1, bitDept);
    down_sample_uv(uiWidth, uiHeight, bitDept, tmp_buf, tmp_stride, dst, i_dst);
}

static tab_s8 g_ipf_pred_param[5][16] = {
    { 24,  6,  2,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0 }, //4x4, 24, 0.5
    { 44, 25, 14,  8,  4,  2,  1,  1,  0,  0, 0, 0, 0, 0, 0, 0 }, //8x8, 44-1.2
    { 40, 27, 19, 13,  9,  6,  4,  3,  2,  1, 0, 0, 0, 0, 0, 0 }, //16x16, 40-1.8
    { 36, 27, 21, 16, 12,  9,  7,  5,  4,  3, 0, 0, 0, 0, 0, 0 }, //32x32, 36-2.5
    { 52, 44, 37, 31, 26, 22, 18, 15, 13, 11, 0, 0, 0, 0, 0, 0 }, //64x64
};

void ipred_ipf_core(pel *src, pel *dst, int i_dst, int flt_range_hor, int flt_range_ver, const s8* flt_hor_coef, const s8* flt_ver_coef, int w, int h, int bit_depth)
{
    pel *p_top = src + 1;
    int max_val = (1 << bit_depth) - 1;
    int row;

    for (row = 0; row < flt_range_ver; ++row) {
        s32 coeff_top = flt_ver_coef[row];
        int col;
        for (col = 0; col < flt_range_hor; col++) {
            s32 coeff_left = flt_hor_coef[col];
            s32 coeff_cur = 64 - coeff_left - coeff_top;
            s32 sample_val = (coeff_left* src[-row - 1] + coeff_top * p_top[col] + coeff_cur * dst[col] + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, sample_val);
        }
        for (; col < w; col++) {
            s32 coeff_cur = 64 - coeff_top;
            s32 sample_val = (coeff_top * p_top[col] + coeff_cur * dst[col] + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, sample_val);
        }
        dst += i_dst;
    }
    for (; row < h; ++row) {
        for (s32 col = 0; col < flt_range_hor; col++) {
            s32 coeff_left = flt_hor_coef[col];
            s32 coeff_cur = 64 - coeff_left;
            s32 sample_val = (coeff_left* src[-row - 1] + coeff_cur * dst[col] + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, sample_val);
        }
        dst += i_dst;
    }
}

void ipred_ipf_core_s16(pel *src, pel *dst, int i_dst, s16 *pred, int flt_range_hor, int flt_range_ver, const s8 *flt_hor_coef, const s8 *flt_ver_coef, int w, int h, int bit_depth)
{
    pel *p_top = src + 1;
    int max_val = (1 << bit_depth) - 1;
    int row;

    for (row = 0; row < flt_range_ver; ++row) {
        s32 coeff_top = flt_ver_coef[row];
        int col;

        for (col = 0; col < flt_range_hor; col++) {
            s32 coeff_left = flt_hor_coef[col];
            s32 coeff_cur = 64 - coeff_left - coeff_top;
            s32 sample_val = (coeff_left* src[-row - 1] + coeff_top * p_top[col] + coeff_cur * pred[col] + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, sample_val);
        }
        for (; col < w; col++) {
            s32 coeff_cur = 64 - coeff_top;
            s32 sample_val = (coeff_top * p_top[col] + coeff_cur * pred[col] + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, sample_val);
        }
        dst += i_dst;
        pred += w;
    }
    for (; row < h; ++row) {
        for (s32 col = 0; col < w; col++) {
            s32 coeff_left = (col < flt_range_hor) ? flt_hor_coef[col] : 0;
            s32 coeff_cur = 64 - coeff_left;
            s32 sample_val = (coeff_left* src[-row - 1] + coeff_cur * pred[col] + 32) >> 6;
            dst[col] = COM_CLIP3(0, max_val, sample_val);
        }
        dst += i_dst;
        pred += w;
    }
}

static void uavs3d_always_inline ipf_core(pel *src, pel *dst, int i_dst, int ipm, int w, int h, int bit_depth)
{
    s32 filter_idx_hor = (s32)g_tbl_log2[w] - 2; //Block Size
    s32 filter_idx_ver = (s32)g_tbl_log2[h] - 2; //Block Size
    s32 ver_filter_range = COM_MIN(h, 10);
    s32 hor_filter_range = COM_MIN(w, 10);

    // TODO: g_ipf_pred_param doesn't support 128
    if (filter_idx_hor > 4) {
        filter_idx_hor = 4;
        hor_filter_range = 0; // don't use IPF at horizontal direction
    }
    if (filter_idx_ver > 4) {
        filter_idx_ver = 4;
        ver_filter_range = 0; // don't use IPF at vertical direction
    }

    const s8 *filter_hori_param = g_ipf_pred_param[filter_idx_hor];
    const s8 *filter_vert_param = g_ipf_pred_param[filter_idx_ver];

    if (IPD_DIA_L <= ipm && ipm <= IPD_DIA_R) { // vertical mode use left reference pixels, don't use top reference
        ver_filter_range = 0;
    }
    if (IPD_DIA_R < ipm) { // horizontal mode use top reference pixels, don't use left reference
        hor_filter_range = 0;
    }
   
    uavs3d_funs_handle.intra_pred_ipf(src, dst, i_dst, hor_filter_range, ver_filter_range, filter_hori_param, filter_vert_param, w, h, bit_depth);
}

static void uavs3d_always_inline ipf_core_s16(pel *src, pel *dst, int i_dst, s16 *pred, int ipm, int w, int h, int bit_depth)
{
    s32 filter_idx_hor = (s32)g_tbl_log2[w] - 2; //Block Size
    s32 filter_idx_ver = (s32)g_tbl_log2[h] - 2; //Block Size
    s32 ver_filter_range = COM_MIN(h, 10);
    s32 hor_filter_range = COM_MIN(w, 10);

    // TODO: g_ipf_pred_param doesn't support 128
    if (filter_idx_hor > 4) {
        filter_idx_hor = 4;
        hor_filter_range = 0; // don't use IPF at horizontal direction
    }
    if (filter_idx_ver > 4) {
        filter_idx_ver = 4;
        ver_filter_range = 0; // don't use IPF at vertical direction
    }

    const s8 *filter_hori_param = g_ipf_pred_param[filter_idx_hor];
    const s8 *filter_vert_param = g_ipf_pred_param[filter_idx_ver];

    if (IPD_DIA_L <= ipm && ipm <= IPD_DIA_R) { // vertical mode use left reference pixels, don't use top reference
        ver_filter_range = 0;
    }
    if (IPD_DIA_R < ipm) { // horizontal mode use top reference pixels, don't use left reference
        hor_filter_range = 0;
    }
    uavs3d_funs_handle.intra_pred_ipf_s16(src, dst, i_dst, pred, hor_filter_range, ver_filter_range, filter_hori_param, filter_vert_param, w, h, bit_depth);
}

void com_ipred_l(pel *src, pel *dst, int i_dst, pel *tmp_buf, int ipm, int w, int h, int bit_depth, u16 avail_cu, u8 ipf_flag)
{
    uavs3d_assert(w <= 64 && h <= 64);
  
    if (ipm != IPD_PLN && ipm != IPD_BI) {
        switch (ipm) {
        case IPD_VER:
            uavs3d_funs_handle.intra_pred_ver[Y_C](src + 1, dst, i_dst, w, h);
            break;
        case IPD_HOR:
            uavs3d_funs_handle.intra_pred_hor[Y_C](src - 1, dst, i_dst, w, h);
            break;
        case IPD_DC:
            uavs3d_funs_handle.intra_pred_dc[Y_C](src, dst, i_dst, w, h, avail_cu, bit_depth);
            break;
        default:
            uavs3d_funs_handle.intra_pred_ang[ipm](src, dst, i_dst, ipm, w, h);
            break;
        }
        if (ipf_flag) {
            uavs3d_assert((w < MAX_CU_SIZE) && (h < MAX_CU_SIZE));
            ipf_core(src, dst, i_dst, ipm, w, h, bit_depth);
        }
    } else {
        if (ipf_flag) {
            switch (ipm) {
            case IPD_PLN:
                uavs3d_funs_handle.intra_pred_plane_ipf(src, (s16*)tmp_buf, w, h);
                break;
            case IPD_BI:
                uavs3d_funs_handle.intra_pred_bi_ipf(src, (s16*)tmp_buf, w, h);
                break;
            }
            uavs3d_assert((w < MAX_CU_SIZE) && (h < MAX_CU_SIZE));
            ipf_core_s16(src, dst, i_dst, (s16*)tmp_buf, ipm, w, h, bit_depth);
        } else {
            switch (ipm) {
            case IPD_PLN:
                uavs3d_funs_handle.intra_pred_plane[Y_C](src, dst, i_dst, w, h, bit_depth);
                break;
            case IPD_BI:
                uavs3d_funs_handle.intra_pred_bi[Y_C](src, dst, i_dst, w, h, bit_depth);
                break;
            }
        }
    }
}

void com_ipred_c(pel *dst, int i_dst, pel *src, pel *luma, s16 *tmp_dst, int ipm_c, int ipm, int w, int h, int bit_depth, u16 avail_cu, pel *piRecoY, int uiStrideY)
{
    uavs3d_assert(w <= 64 && h <= 64);

    if(ipm_c == IPD_DM_C && COM_IPRED_CHK_CONV(ipm)) {
        ipm_c = (ipm == IPD_VER) ? IPD_VER_C : (ipm == IPD_HOR ? IPD_HOR_C : (ipm == IPD_DC ? IPD_DC_C : IPD_BI_C));
    }

    switch(ipm_c) {
    case IPD_DM_C:
        switch(ipm) {
        case IPD_PLN:
            uavs3d_funs_handle.intra_pred_plane[UV_C](src, dst, i_dst, w, h, bit_depth);
            break;
        default:
            uavs3d_funs_handle.intra_pred_chroma_ang[ipm](src, dst, i_dst, ipm, w, h);
            break;
        }
        break;
    case IPD_DC_C:
        uavs3d_funs_handle.intra_pred_dc[UV_C](src, dst, i_dst, w, h, avail_cu, bit_depth);
        break;
    case IPD_HOR_C:
        uavs3d_funs_handle.intra_pred_hor[UV_C](src - 2, dst, i_dst, w, h);
        break;
    case IPD_VER_C:
        uavs3d_funs_handle.intra_pred_ver[UV_C](src + 2, dst, i_dst, w << 1, h);
        break;
    case IPD_BI_C:
        uavs3d_funs_handle.intra_pred_bi[UV_C](src, dst, i_dst, w, h, bit_depth);
        break;
    case IPD_TSCPM_C:
        ipred_tscpm(dst, i_dst, src, luma, piRecoY, uiStrideY, w,  h, IS_AVAIL(avail_cu, AVAIL_UP), IS_AVAIL(avail_cu, AVAIL_LE), bit_depth);
        break;

    default:
        printf("\n illegal chroma intra prediction mode\n");
        break;
    }
}

static tab_s8 tab_auc_dir_dxdy[2][IPD_CNT][2] = {
    {
        // dx/dy
        {0, 0}, {0, 0}, {0, 0}, {11, 2}, {2, 0},
        {11, 3}, {1, 0}, {93, 7}, {1, 1}, {93, 8},
        {1, 2}, {1, 3}, {0, 0}, {1, 3}, {1, 2},
        {93, 8}, {1, 1}, {93, 7}, {1, 0}, {11, 3},
        {2, 0}, {11, 2}, {4, 0}, {8, 0}, {0, 0},
        {8, 0}, {4, 0}, {11, 2}, {2, 0}, {11, 3},
        {1, 0}, {93, 7}, {1, 1},
    },
    {
        // dy/dx
        {0, 0}, {0, 0}, {0, 0}, {93, 8}, {1, 1},
        {93, 7}, {1, 0}, {11, 3}, {2, 0}, {11, 2},
        {4, 0}, {8, 0}, {0, 0}, {8, 0}, {4, 0},
        {11, 2}, {2, 0}, {11, 3}, {1, 0}, {93, 7},
        {1, 1}, {93, 8}, {1, 2}, {1, 3}, {0, 0},
        {1, 3}, {1, 2}, {93, 8}, {1, 1}, {93, 7},
        {1, 0}, {11, 3}, {2, 0}
    }
};

static int uavs3d_always_inline getContextPixel(int uiDirMode, int uiXYflag, int iTempD, int *offset)
{
    int imult  = tab_auc_dir_dxdy[uiXYflag][uiDirMode][0];
    int ishift = tab_auc_dir_dxdy[uiXYflag][uiDirMode][1];

    int iTempDn = iTempD * imult >> ishift;
    *offset = ((iTempD * imult * 32) >> ishift) - iTempDn * 32;
    return iTempDn;
}

static void xPredIntraAngAdi_X(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i, j;
    int offset;
    int width2 = width << 1;

    for (j = 0; j < height; j++) {
        int c1, c2, c3, c4;
        int idx = getContextPixel(mode, 0, j + 1, &offset);
        int pred_width = COM_MIN(width, width2 - idx + 1);

        c1 = 32 - offset;
        c2 = 64 - offset;
        c3 = 32 + offset;
        c4 = offset;

        for (i = 0; i < pred_width; i++, idx++) {
            dst[i] = (src[idx] * c1 + src[idx + 1] * c2 + src[idx + 2] * c3 + src[idx + 3] * c4 + 64) >> 7;
        }
        if (pred_width <= 0) {
            dst[0] = (src[width2] * c1 + src[width2 + 1] * c2 + src[width2 + 2] * c3 + src[width2 + 3] * c4 + 64) >> 7;
            pred_width = 1;
        }
        for (; i < width; i++) {
            dst[i] = dst[pred_width - 1];
        }
        dst += i_dst;
    }
}

static void xPredIntraAngAdiChroma_X(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i, j;
    int offset;
    int width2 = width << 1;

    for (j = 0; j < height; j++) {
        int c1, c2, c3, c4;
        int idx = getContextPixel(mode, 0, j + 1, &offset);
        int pred_width = (COM_MIN(width, width2 - idx + 1)) << 1;

        idx <<= 1;
        c1 = 32 - offset;
        c2 = 64 - offset;
        c3 = 32 + offset;
        c4 = offset;

        for (i = 0; i < pred_width; i += 2, idx += 2) {
            dst[i] = (src[idx] * c1 + src[idx + 2] * c2 + src[idx + 4] * c3 + src[idx + 6] * c4 + 64) >> 7;  //U
            dst[i + 1] = (src[idx + 1] * c1 + src[idx + 3] * c2 + src[idx + 5] * c3 + src[idx + 7] * c4 + 64) >> 7;  //V
        }
        if (pred_width <= 0) {
            idx = width2 << 1;
            dst[0] = (src[idx] * c1 + src[idx + 2] * c2 + src[idx + 4] * c3 + src[idx + 6] * c4 + 64) >> 7;  //U
            dst[1] = (src[idx + 1] * c1 + src[idx + 3] * c2 + src[idx + 5] * c3 + src[idx + 7] * c4 + 64) >> 7;  //V
            pred_width = 2;
        }
        for (; i < width2; i += 2) {
            dst[i] = dst[pred_width - 2];
            dst[i + 1] = dst[pred_width - 1];
        }
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_X_4(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    ALIGNED_16(pel first_line[64 + 128]);
    int line_size = iWidth + (iHeight - 1) * 2;
    int real_size = min(line_size, iWidth * 2 - 1);
    int iHeight2 = iHeight * 2;
    int i;

    pSrc += 3;

    for (i = 0; i < real_size; i++, pSrc++) {
        first_line[i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
    }

    // padding
    for (; i < line_size; i++) {
        first_line[i] = first_line[real_size - 1];
    }

    for (i = 0; i < iHeight2; i += 2) {
        memcpy(dst, first_line + i, iWidth * sizeof(pel));
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_X_6(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    ALIGNED_16(pel first_line[64 + 64]);
    int line_size = iWidth + iHeight - 1;
    int real_size = min(line_size, iWidth * 2);
    int i;

    pSrc += 2;

    for (i = 0; i < real_size; i++, pSrc++) {
        first_line[i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
    }

    // padding
    for (; i < line_size; i++) {
        first_line[i] = first_line[real_size - 1];
    }

    for (i = 0; i < iHeight; i++) {
        memcpy(dst, first_line + i, iWidth * sizeof(pel));
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_X_8(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    ALIGNED_16(pel first_line[2 * (64 + 32)]);
    int line_size = iWidth + iHeight / 2 - 1;
    int real_size = min(line_size, iWidth * 2 + 1);
    int i;
    int aligned_line_size = ((line_size + 15) >> 4) << 4;
    pel *pfirst[2] = { first_line, first_line + aligned_line_size };

    for (i = 0; i < real_size; i++, pSrc++) {
        pfirst[0][i] = (pSrc[0] + (pSrc[1] + pSrc[2]) * 3 + pSrc[3] + 4) >> 3;
        pfirst[1][i] = (pSrc[1] + pSrc[2] * 2 + pSrc[3] + 2) >> 2;
    }

    // padding
    if (real_size < line_size) {
        int pad1, pad2;

        pfirst[1][real_size - 1] = pfirst[1][real_size - 2];

        pad1 = pfirst[0][real_size - 1];
        pad2 = pfirst[1][real_size - 1];
        for (; i < line_size; i++) {
            pfirst[0][i] = pad1;
            pfirst[1][i] = pad2;
        }
    }

    iHeight /= 2;

    for (i = 0; i < iHeight; i++) {
        memcpy(dst, pfirst[0] + i, iWidth * sizeof(pel));
        memcpy(dst + i_dst, pfirst[1] + i, iWidth * sizeof(pel));
        dst += i_dst * 2;
    }
}

static void xPredIntraAngAdi_X_10(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i;
    pel *dst1 = dst;
    pel *dst2 = dst1 + i_dst;
    pel *dst3 = dst2 + i_dst;
    pel *dst4 = dst3 + i_dst;

    if (iHeight != 4) {
        ALIGNED_16(pel first_line[4 * (64 + 16)]);
        int line_size = iWidth + iHeight / 4 - 1;
        int aligned_line_size = ((line_size + 15) >> 4) << 4;
        pel *pfirst[4] = { first_line, first_line + aligned_line_size, first_line + aligned_line_size * 2, first_line + aligned_line_size * 3 };

        for (i = 0; i < line_size; i++, pSrc++) {
            pfirst[0][i] = (pSrc[0] * 3 + pSrc[1] * 7 + pSrc[2] * 5 + pSrc[3] + 8) >> 4;
            pfirst[1][i] = (pSrc[0] + (pSrc[1] + pSrc[2]) * 3 + pSrc[3] + 4) >> 3;
            pfirst[2][i] = (pSrc[0] + pSrc[1] * 5 + pSrc[2] * 7 + pSrc[3] * 3 + 8) >> 4;
            pfirst[3][i] = (pSrc[1] + pSrc[2] * 2 + pSrc[3] + 2) >> 2;
        }

        iHeight /= 4;

        for (i = 0; i < iHeight; i++) {
            memcpy(dst1, pfirst[0] + i, iWidth * sizeof(pel));
            memcpy(dst2, pfirst[1] + i, iWidth * sizeof(pel));
            memcpy(dst3, pfirst[2] + i, iWidth * sizeof(pel));
            memcpy(dst4, pfirst[3] + i, iWidth * sizeof(pel));
            dst1 += i_dst * 4;
            dst2 += i_dst * 4;
            dst3 += i_dst * 4;
            dst4 += i_dst * 4;
        }
    }
    else {
        for (i = 0; i < iWidth; i++, pSrc++) {
            dst1[i] = (pSrc[0] * 3 + pSrc[1] * 7 + pSrc[2] * 5 + pSrc[3] + 8) >> 4;
            dst2[i] = (pSrc[0] + (pSrc[1] + pSrc[2]) * 3 + pSrc[3] + 4) >> 3;
            dst3[i] = (pSrc[0] + pSrc[1] * 5 + pSrc[2] * 7 + pSrc[3] * 3 + 8) >> 4;
            dst4[i] = (pSrc[1] + pSrc[2] * 2 + pSrc[3] + 2) >> 2;
        }
    }
}

static void xPredIntraAngAdi_Y(pel *src, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    int offset;
    int iY;
    int iHeight2 = iHeight << 1;
    int offsets[64];
    int xsteps[64];

    for (i = 0; i < iWidth; i++) {
        xsteps[i] = getContextPixel(uiDirMode, 1, i + 1, &offsets[i]);
    }

    for (j = 0; j < iHeight; j++) {
        for (i = 0; i < iWidth; i++) {
            int idx;
            iY = j + xsteps[i];
            idx = COM_MAX(-iHeight2, -iY);

            offset = offsets[i];
            dst[i] = (src[idx] * (32 - offset) + src[idx - 1] * (64 - offset) + src[idx - 2] * (32 + offset) + src[idx - 3] * offset + 64) >> 7;
        }
        dst += i_dst;
    }
}

static void xPredIntraAngAdiChroma_Y(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    int offset;
    int iY;
    int iHeight2 = iHeight << 1;
    int offsets[64];
    int xsteps[64];

    for (i = 0; i < iWidth; i++) {
        xsteps[i] = getContextPixel(uiDirMode, 1, i + 1, &offsets[i]);
    }

    for (j = 0; j < iHeight; j++) {
        for (i = 0; i < iWidth; i++) {
            int idx;
            int i2 = i << 1;
            iY = j + xsteps[i];
            idx = COM_MAX(-iHeight2, -iY) << 1;

            offset = offsets[i];
            dst[i2] = (pSrc[idx] * (32 - offset) + pSrc[idx - 2] * (64 - offset) + pSrc[idx - 4] * (32 + offset) + pSrc[idx - 6] * offset + 64) >> 7; //U
            dst[i2 + 1] = (pSrc[idx + 1] * (32 - offset) + pSrc[idx - 1] * (64 - offset) + pSrc[idx - 3] * (32 + offset) + pSrc[idx - 5] * offset + 64) >> 7; //V
        }
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_Y_26(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i;

    if (iWidth != 4) {
        ALIGNED_16(pel first_line[64 + 256]);
        int line_size = iWidth + (iHeight - 1) * 4;
        int iHeight4 = iHeight << 2;
        for (i = 0; i < line_size; i += 4, pSrc--) {
            first_line[i] = (pSrc[0] * 3 + pSrc[-1] * 7 + pSrc[-2] * 5 + pSrc[-3] + 8) >> 4;
            first_line[i + 1] = (pSrc[0] + (pSrc[-1] + pSrc[-2]) * 3 + pSrc[-3] + 4) >> 3;
            first_line[i + 2] = (pSrc[0] + pSrc[-1] * 5 + pSrc[-2] * 7 + pSrc[-3] * 3 + 8) >> 4;
            first_line[i + 3] = (pSrc[-1] + pSrc[-2] * 2 + pSrc[-3] + 2) >> 2;
        }

        for (i = 0; i < iHeight4; i += 4) {
            memcpy(dst, first_line + i, iWidth * sizeof(pel));
            dst += i_dst;
        }
    }
    else {
        for (i = 0; i < iHeight; i++, pSrc--) {
            dst[0] = (pSrc[0] * 3 + pSrc[-1] * 7 + pSrc[-2] * 5 + pSrc[-3] + 8) >> 4;
            dst[1] = (pSrc[0] + (pSrc[-1] + pSrc[-2]) * 3 + pSrc[-3] + 4) >> 3;
            dst[2] = (pSrc[0] + pSrc[-1] * 5 + pSrc[-2] * 7 + pSrc[-3] * 3 + 8) >> 4;
            dst[3] = (pSrc[-1] + pSrc[-2] * 2 + pSrc[-3] + 2) >> 2;
            dst += i_dst;
        }
    }
}

static void xPredIntraAngAdi_Y_28(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    ALIGNED_16(pel first_line[64 + 128]);
    int line_size = iWidth + (iHeight - 1) * 2;
    int real_size = min(line_size, iHeight * 4 + 1);
    int i;
    int iHeight2 = iHeight << 1;

    for (i = 0; i < real_size; i += 2, pSrc--) {
        first_line[i] = (pSrc[0] + (pSrc[-1] + pSrc[-2]) * 3 + pSrc[-3] + 4) >> 3;
        first_line[i + 1] = (pSrc[-1] + pSrc[-2] * 2 + pSrc[-3] + 2) >> 2;
    }

    // padding
    if (real_size < line_size) {
        int pad1, pad2;
        first_line[i - 1] = first_line[i - 3];

        pad1 = first_line[i - 2];
        pad2 = first_line[i - 1];

        for (; i < line_size; i += 2) {
            first_line[i] = pad1;
            first_line[i + 1] = pad2;
        }
    }

    for (i = 0; i < iHeight2; i += 2) {
        memcpy(dst, first_line + i, iWidth * sizeof(pel));
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_Y_30(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    ALIGNED_16(pel first_line[64 + 64]);
    int line_size = iWidth + iHeight - 1;
    int real_size = min(line_size, iHeight * 2);
    int i;

    pSrc -= 2;

    for (i = 0; i < real_size; i++, pSrc--) {
        first_line[i] = (pSrc[1] + pSrc[0] * 2 + pSrc[-1] + 2) >> 2;
    }

    // padding
    for (; i < line_size; i++) {
        first_line[i] = first_line[real_size - 1];
    }

    for (i = 0; i < iHeight; i++) {
        memcpy(dst, first_line + i, iWidth * sizeof(pel));
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_Y_32(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    ALIGNED_16(pel first_line[2 * (32 + 64)]);
    int line_size = iHeight / 2 + iWidth - 1;
    int real_size = min(line_size, iHeight);
    int i;
    pel pad_val;
    int aligned_line_size = ((line_size + 15) >> 4) << 4;
    pel *pfirst[2] = { first_line, first_line + aligned_line_size };

    pSrc -= 3;

    for (i = 0; i < real_size; i++, pSrc -= 2) {
        pfirst[0][i] = (pSrc[1] + pSrc[0] * 2 + pSrc[-1] + 2) >> 2;
        pfirst[1][i] = (pSrc[0] + pSrc[-1] * 2 + pSrc[-2] + 2) >> 2;
    }

    // padding
    pad_val = pfirst[1][i - 1];
    for (; i < line_size; i++) {
        pfirst[0][i] = pad_val;
        pfirst[1][i] = pad_val;
    }

    iHeight /= 2;
    for (i = 0; i < iHeight; i++) {
        memcpy(dst, pfirst[0] + i, iWidth * sizeof(pel));
        memcpy(dst + i_dst, pfirst[1] + i, iWidth * sizeof(pel));
        dst += i_dst * 2;
    }
}

static void xPredIntraAngAdi_XY(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    int offsetx, offsety;
    pel *rpSrc = pSrc;
    int xoffsets[64];
    int xsteps[64];

    for (i = 0; i < iWidth; i++) {
        xsteps[i] = getContextPixel(uiDirMode, 1, i + 1, &xoffsets[i]);
    }

    for (j = 0; j < iHeight; j++) {
        pel *px = pSrc - getContextPixel(uiDirMode, 0, j + 1, &offsetx);

        for (i = 0; i < iWidth; i++, px++) {
            int iYy = j - xsteps[i];
            if (iYy <= -1) {
                dst[i] = (px[2] * (32 - offsetx) + px[1] * (64 - offsetx) + px[0] * (32 + offsetx) + px[-1] * offsetx + 64) >> 7;
            }
            else {
                offsety = xoffsets[i];
                dst[i] = (rpSrc[-iYy - 2] * (32 - offsety) + rpSrc[-iYy - 1] * (64 - offsety) + rpSrc[-iYy] * (32 + offsety) + rpSrc[-iYy + 1] * offsety + 64) >> 7;
            }
        }
        dst += i_dst;
    }
}

static void xPredIntraAngAdiChroma_XY(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    int offsetx, offsety;
    pel *rpSrc = pSrc;
    int xoffsets[64];
    int xsteps[64];

    for (i = 0; i < iWidth; i++) {
        xsteps[i] = getContextPixel(uiDirMode, 1, i + 1, &xoffsets[i]);
    }

    for (j = 0; j < iHeight; j++) {
        int srcOff = getContextPixel(uiDirMode, 0, j + 1, &offsetx) << 1;
        pel *px = pSrc - srcOff;

        for (i = 0; i < iWidth; i++, px += 2) {
            int iYy = j - xsteps[i];
            int i2 = i << 1;
            if (iYy <= -1) {
                dst[i2] = (px[4] * (32 - offsetx) + px[2] * (64 - offsetx) + px[0] * (32 + offsetx) + px[-2] * offsetx + 64) >> 7; //U
                dst[i2 + 1] = (px[5] * (32 - offsetx) + px[3] * (64 - offsetx) + px[1] * (32 + offsetx) + px[-1] * offsetx + 64) >> 7; //V
            }
            else {
                offsety = xoffsets[i];
                iYy *= 2;
                dst[i2] = (rpSrc[-iYy - 4] * (32 - offsety) + rpSrc[-iYy - 2] * (64 - offsety) + rpSrc[-iYy] * (32 + offsety) + rpSrc[-iYy + 2] * offsety + 64) >> 7; //U
                dst[i2 + 1] = (rpSrc[-iYy - 3] * (32 - offsety) + rpSrc[-iYy - 1] * (64 - offsety) + rpSrc[-iYy + 1] * (32 + offsety) + rpSrc[-iYy + 3] * offsety + 64) >> 7; //U
            }
        }
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_XY_13(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    pel *rpSrc = pSrc;
    pel *dst_base = dst - 8 * i_dst - 1;
    int step1_height = min(iHeight, 7);

    for (j = 0; j < step1_height; j++) {
        int iTempD = j + 1;
        pel *p = pSrc - (iTempD >> 3);
        int offsetx = (iTempD << 2) & 0x1f;
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;

        for (i = 0; i < iWidth; i++, p++) {
            dst[i] = (p[2] * a + p[1] * b + p[0] * c + p[-1] * offsetx + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }

    for (; j < iHeight; j++) {
        int iTempD = j + 1;
        int step1_width = (int)(((j + 1) / 8.0 + 0.9999)) - 1;
        int offsetx = (iTempD << 2) & 0x1f;
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
        pel *px = pSrc + step1_width - (iTempD >> 3);

        step1_width = min(step1_width, iWidth);

        for (i = 0; i < step1_width; i++) {
            pel *py = rpSrc - j + ((i + 1) << 3);
            dst[i] = (py[-2] + (py[-1] << 1) + py[0] + 2) >> 2;
        }
        for (; i < iWidth; i++, px++) {
            dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_XY_14(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i;

    if (iHeight != 4) {
        ALIGNED_16(pel first_line[4 * (64 + 16)]);
        int line_size = iWidth + iHeight / 4 - 1;
        int left_size = line_size - iWidth;
        int aligned_line_size = ((line_size + 15) >> 4) << 4;
        pel *pfirst[4] = { first_line, first_line + aligned_line_size, first_line + aligned_line_size * 2, first_line + aligned_line_size * 3 };

        pSrc -= iHeight - 4;
        for (i = 0; i < left_size; i++, pSrc += 4) {
            pfirst[0][i] = (pSrc[2] + pSrc[3] * 2 + pSrc[4] + 2) >> 2;
            pfirst[1][i] = (pSrc[1] + pSrc[2] * 2 + pSrc[3] + 2) >> 2;
            pfirst[2][i] = (pSrc[0] + pSrc[1] * 2 + pSrc[2] + 2) >> 2;
            pfirst[3][i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
        }

        for (; i < line_size; i++, pSrc++) {
            pfirst[0][i] = (pSrc[-1] + pSrc[0] * 5 + pSrc[1] * 7 + pSrc[2] * 3 + 8) >> 4;
            pfirst[1][i] = (pSrc[-1] + (pSrc[0] + pSrc[1]) * 3 + pSrc[2] + 4) >> 3;
            pfirst[2][i] = (pSrc[-1] * 3 + pSrc[0] * 7 + pSrc[1] * 5 + pSrc[2] + 8) >> 4;
            pfirst[3][i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
        }

        pfirst[0] += left_size;
        pfirst[1] += left_size;
        pfirst[2] += left_size;
        pfirst[3] += left_size;

        iHeight /= 4;

        for (i = 0; i < iHeight; i++) {
            memcpy(dst, pfirst[0] - i, iWidth * sizeof(pel));
            dst += i_dst;
            memcpy(dst, pfirst[1] - i, iWidth * sizeof(pel));
            dst += i_dst;
            memcpy(dst, pfirst[2] - i, iWidth * sizeof(pel));
            dst += i_dst;
            memcpy(dst, pfirst[3] - i, iWidth * sizeof(pel));
            dst += i_dst;
        }
    }
    else {
        pel *dst1 = dst;
        pel *dst2 = dst1 + i_dst;
        pel *dst3 = dst2 + i_dst;
        pel *dst4 = dst3 + i_dst;

        for (i = 0; i < iWidth; i++, pSrc++) {
            dst1[i] = (pSrc[-1] + pSrc[0] * 5 + pSrc[1] * 7 + pSrc[2] * 3 + 8) >> 4;
            dst2[i] = (pSrc[-1] + (pSrc[0] + pSrc[1]) * 3 + pSrc[2] + 4) >> 3;
            dst3[i] = (pSrc[-1] * 3 + pSrc[0] * 7 + pSrc[1] * 5 + pSrc[2] + 8) >> 4;
            dst4[i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
        }
    }
}

static void xPredIntraAngAdi_XY_15(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    pel *rpSrc = pSrc;
    pel *dst_base = dst - 11 * i_dst - 4;
    int xoffsets[64];
    int xsteps[64];
    int iTempD = 11;

    for (i = 0; i < iWidth; i++, iTempD += 11) {
        xoffsets[i] = (iTempD << 3) & 0x1f;
        xsteps[i] = (iTempD >> 2);
    }

    iTempD = 93;

    for (j = 0; j < 2; j++, iTempD += 93) {
        int offsetx = (iTempD >> 3) & 0x1f;
        pel *px = pSrc - (iTempD >> 8);
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
        for (i = 0; i < iWidth; i++, px++) {
            dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }

    for (; j < iHeight; j++, iTempD += 93) {
        int setp1_width = (int)(((j + 1) << 2) / 11.0 + 0.9999) - 1;
        int offsetx = (iTempD >> 3) & 0x1f;
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
        pel *px = pSrc - (iTempD >> 8);

        setp1_width = min(setp1_width, iWidth);

        for (i = 0; i < setp1_width; i++) {
            pel *py = rpSrc - j + xsteps[i];
            int offsety = xoffsets[i];
            dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
        }

        px += i;

        for (; i < iWidth; i++, px++) {
            dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_XY_16(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    ALIGNED_16(pel first_line[2 * (64 + 32)]);
    int line_size = iWidth + iHeight / 2 - 1;
    int left_size = line_size - iWidth;
    int aligned_line_size = ((line_size + 15) >> 4) << 4;
    pel *pfirst[2] = { first_line, first_line + aligned_line_size };

    int i;

    pSrc -= iHeight - 2;

    for (i = 0; i < left_size; i++, pSrc += 2) {
        pfirst[0][i] = (pSrc[0] + pSrc[1] * 2 + pSrc[2] + 2) >> 2;
        pfirst[1][i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
    }

    for (; i < line_size; i++, pSrc++) {
        pfirst[0][i] = (pSrc[-1] + (pSrc[0] + pSrc[1]) * 3 + pSrc[2] + 4) >> 3;
        pfirst[1][i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
    }

    pfirst[0] += left_size;
    pfirst[1] += left_size;

    iHeight /= 2;

    for (i = 0; i < iHeight; i++) {
        memcpy(dst, pfirst[0] - i, iWidth * sizeof(pel));
        memcpy(dst + i_dst, pfirst[1] - i, iWidth * sizeof(pel));
        dst += 2 * i_dst;
    }
}

static void xPredIntraAngAdi_XY_17(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    pel *rpSrc = pSrc;
    pel *dst_base = dst - 11 * i_dst - 8;
    int xoffsets[64];
    int xsteps[64];
    int iTempD = 11;

    for (i = 0; i < iWidth; i++, iTempD += 11) {
        xoffsets[i] = (iTempD << 2) & 0x1f;
        xsteps[i] = iTempD >> 3;
    }

    iTempD = 93;

    {
        int offsetx = (iTempD >> 2) & 0x1f;
        pel *px = pSrc - (iTempD >> 7);
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
        for (i = 0; i < iWidth; i++, px++) {
            dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
        iTempD += 93;
    }

    for (j = 1; j < iHeight; j++, iTempD += 93) {
        int setp1_width = (int)(((j + 1) << 3) / 11.0 + 0.9999) - 1;
        int offsetx = (iTempD >> 2) & 0x1f;
        pel *px = pSrc - (iTempD >> 7);
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;

        setp1_width = min(setp1_width, iWidth);

        for (i = 0; i < setp1_width; i++) {
            pel *py = rpSrc - j + xsteps[i];
            int offsety = xoffsets[i];
            dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
        }
        px += i;
        for (; i < iWidth; i++, px++) {
            dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_XY_18(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    ALIGNED_16(pel first_line[64 + 64]);
    int line_size = iWidth + iHeight - 1;
    int i;
    pel *pfirst = first_line + iHeight - 1;

    pSrc -= iHeight - 1;

    for (i = 0; i < line_size; i++, pSrc++) {
        first_line[i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
    }

    for (i = 0; i < iHeight; i++) {
        memcpy(dst, pfirst, iWidth * sizeof(pel));
        pfirst--;
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_XY_19(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    pel *rpSrc = pSrc;
    pel *dst_base = dst - 8 * i_dst - 11;
    int xoffsets[64];
    int xsteps[64];
    int iTempD = 93;
    int step2_height = ((93 * iWidth) >> 7);

    for (i = 0; i < iWidth; i++, iTempD += 93) {
        xoffsets[i] = (iTempD >> 2) & 0x1f;
        xsteps[i] = iTempD >> 7;
    }

    iTempD = 11;

    step2_height = min(step2_height, iHeight);

    for (j = 0; j < step2_height; j++, iTempD += 11) {
        int step1_width = (int)(((j + 1) << 7) / 93.0 + 0.9999) - 1;
        int offsetx = (iTempD << 2) & 0x1f;
        pel *px = pSrc - (iTempD >> 3);
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;

        step1_width = min(step1_width, iWidth);

        for (i = 0; i < step1_width; i++) {
            pel *py = rpSrc - j + xsteps[i];
            int offsety = xoffsets[i];
            dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
        }
        px += i;

        for (; i < iWidth; i++, px++) {
            dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }

    for (; j < iHeight; j++) {
        for (i = 0; i < iWidth; i++) {
            pel *py = rpSrc - j + xsteps[i];
            int offsety = xoffsets[i];
            dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_XY_20(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    ALIGNED_16(pel first_line[64 + 128]);
    int left_size = (iHeight - 1) * 2 + 1;
    int top_size = iWidth - 1;
    int line_size = left_size + top_size;
    int i;
    pel *pfirst = first_line + left_size - 1;

    pSrc -= iHeight;

    for (i = 0; i < left_size; i += 2, pSrc++) {
        first_line[i] = (pSrc[-1] + (pSrc[0] + pSrc[1]) * 3 + pSrc[2] + 4) >> 3;
        first_line[i + 1] = (pSrc[0] + pSrc[1] * 2 + pSrc[2] + 2) >> 2;
    }
    i--;

    for (; i < line_size; i++, pSrc++) {
        first_line[i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
    }

    for (i = 0; i < iHeight; i++) {
        memcpy(dst, pfirst, iWidth * sizeof(pel));
        pfirst -= 2;
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_XY_21(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    pel *rpSrc = pSrc;
    pel *dst_base = dst - 4 * i_dst - 11;
    int xoffsets[64];
    int xsteps[64];
    int iTempD = 93;
    int step2_height = (93 * iWidth) >> 8;

    for (i = 0; i < iWidth; i++, iTempD += 93) {
        xoffsets[i] = (iTempD >> 3) & 0x1f;
        xsteps[i] = iTempD >> 8;
    }

    iTempD = 11;
    step2_height = min(step2_height, iHeight);

    for (j = 0; j < step2_height; j++, iTempD += 11) {
        int step1_width = (int)(((j + 1) << 8) / 93.0 + 0.9999) - 1;
        int offsetx = (iTempD << 3) & 0x1f;
        pel *px = pSrc - (iTempD >> 2);
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;

        step1_width = min(step1_width, iWidth);

        for (i = 0; i < step1_width; i++) {
            pel *py = rpSrc - j + xsteps[i];
            int offsety = xoffsets[i];
            dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
        }

        px += i;

        for (; i < iWidth; i++, px++) {
            dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }

    for (; j < iHeight; j++) {
        for (i = 0; i < iWidth; i++) {
            pel *py = rpSrc - j + xsteps[i];
            int offsety = xoffsets[i];
            dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }
}

static void xPredIntraAngAdi_XY_22(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i;
    pSrc -= iHeight;

    if (iWidth != 4) {
        ALIGNED_16(pel first_line[64 + 256]);
        int left_size = (iHeight - 1) * 4 + 3;
        int top_size = iWidth - 3;
        int line_size = left_size + top_size;
        pel *pfirst = first_line + left_size - 3;
        for (i = 0; i < left_size; i += 4, pSrc++) {
            first_line[i] = (pSrc[-1] * 3 + pSrc[0] * 7 + pSrc[1] * 5 + pSrc[2] + 8) >> 4;
            first_line[i + 1] = (pSrc[-1] + (pSrc[0] + pSrc[1]) * 3 + pSrc[2] + 4) >> 3;
            first_line[i + 2] = (pSrc[-1] + pSrc[0] * 5 + pSrc[1] * 7 + pSrc[2] * 3 + 8) >> 4;
            first_line[i + 3] = (pSrc[0] + pSrc[1] * 2 + pSrc[2] + 2) >> 2;
        }
        i--;

        for (; i < line_size; i++, pSrc++) {
            first_line[i] = (pSrc[-1] + pSrc[0] * 2 + pSrc[1] + 2) >> 2;
        }

        for (i = 0; i < iHeight; i++) {
            memcpy(dst, pfirst, iWidth * sizeof(pel));
            dst += i_dst;
            pfirst -= 4;
        }
    }
    else {
        dst += (iHeight - 1) * i_dst;
        for (i = 0; i < iHeight; i++, pSrc++) {
            dst[0] = (pSrc[-1] * 3 + pSrc[0] * 7 + pSrc[1] * 5 + pSrc[2] + 8) >> 4;
            dst[1] = (pSrc[-1] + (pSrc[0] + pSrc[1]) * 3 + pSrc[2] + 4) >> 3;
            dst[2] = (pSrc[-1] + pSrc[0] * 5 + pSrc[1] * 7 + pSrc[2] * 3 + 8) >> 4;
            dst[3] = (pSrc[0] + pSrc[1] * 2 + pSrc[2] + 2) >> 2;
            dst -= i_dst;
        }
    }
}

static void xPredIntraAngAdi_XY_23(pel *pSrc, pel *dst, int i_dst, int uiDirMode, int iWidth, int iHeight)
{
    int i, j;
    pel *rpSrc = pSrc;
    pel *dst_base = dst - i_dst - 8;
    int xoffsets[64];
    int xsteps[64];
    int iTempD = 1;
    int step2_height = (iWidth >> 3);

    for (i = 0; i < iWidth; i++, iTempD++) {
        xoffsets[i] = (iTempD << 2) & 0x1f;
        xsteps[i] = iTempD >> 3;
    }

    iTempD = 8;

    for (j = 0; j < step2_height; j++, iTempD += 8) {
        int step1_width = ((j + 1) << 3) - 1;
        pel *px = pSrc - iTempD;

        step1_width = min(step1_width, iWidth);

        for (i = 0; i < step1_width; i++) {
            pel *py = rpSrc - j + xsteps[i];
            int offsety = xoffsets[i];
            dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
        }
        px += i;
        for (; i < iWidth; i++, px++) {
            dst[i] = (px[2] + (px[1] << 1) + px[0] + 2) >> 2;
        }
        dst_base += i_dst;
        dst += i_dst;
    }

    for (; j < iHeight; j++) {
        for (i = 0; i < iWidth; i++) {
            pel *py = rpSrc - j + xsteps[i];
            int offsety = xoffsets[i];
            dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
        }
        dst_base += i_dst;
        dst += i_dst;
    }
}

void uavs3d_funs_init_intra_pred_c()
{
    int i;

    for (i = IPD_BI + 1; i < IPD_VER; i++) {
        uavs3d_funs_handle.intra_pred_ang[i] = xPredIntraAngAdi_X;
        uavs3d_funs_handle.intra_pred_chroma_ang[i] = xPredIntraAngAdiChroma_X;
    }
    for (i = IPD_VER; i < IPD_HOR; i++) {
        uavs3d_funs_handle.intra_pred_ang[i] = xPredIntraAngAdi_XY;
        uavs3d_funs_handle.intra_pred_chroma_ang[i] = xPredIntraAngAdiChroma_XY;
    }
    for (i = IPD_HOR + 1; i < IPD_CNT; i++) {
        uavs3d_funs_handle.intra_pred_ang[i] = xPredIntraAngAdi_Y;
        uavs3d_funs_handle.intra_pred_chroma_ang[i] = xPredIntraAngAdiChroma_Y;
    }

    uavs3d_funs_handle.intra_pred_dc   [ Y_C] = ipred_dc;
    uavs3d_funs_handle.intra_pred_bi   [ Y_C] = ipred_bi;
    uavs3d_funs_handle.intra_pred_plane[ Y_C] = ipred_plane;
    uavs3d_funs_handle.intra_pred_hor  [ Y_C] = ipred_hor;
    uavs3d_funs_handle.intra_pred_ver  [ Y_C] = ipred_vert;
    uavs3d_funs_handle.intra_pred_dc   [UV_C] = ipred_dc_uv;
    uavs3d_funs_handle.intra_pred_bi   [UV_C] = ipred_bi_uv;
    uavs3d_funs_handle.intra_pred_plane[UV_C] = ipred_plane_uv;
    uavs3d_funs_handle.intra_pred_hor  [UV_C] = ipred_hor_uv;
    uavs3d_funs_handle.intra_pred_ver  [UV_C] = ipred_vert;
    uavs3d_funs_handle.intra_pred_bi_ipf      = ipred_bi_ipf;
    uavs3d_funs_handle.intra_pred_plane_ipf   = ipred_plane_ipf;

    uavs3d_funs_handle.intra_pred_ang[ 4] = xPredIntraAngAdi_X_4;
    uavs3d_funs_handle.intra_pred_ang[ 6] = xPredIntraAngAdi_X_6;
    uavs3d_funs_handle.intra_pred_ang[ 8] = xPredIntraAngAdi_X_8;
    uavs3d_funs_handle.intra_pred_ang[10] = xPredIntraAngAdi_X_10;

    uavs3d_funs_handle.intra_pred_ang[13] = xPredIntraAngAdi_XY_13;
    uavs3d_funs_handle.intra_pred_ang[15] = xPredIntraAngAdi_XY_15;
    uavs3d_funs_handle.intra_pred_ang[17] = xPredIntraAngAdi_XY_17;
    uavs3d_funs_handle.intra_pred_ang[19] = xPredIntraAngAdi_XY_19;
    uavs3d_funs_handle.intra_pred_ang[21] = xPredIntraAngAdi_XY_21;
    uavs3d_funs_handle.intra_pred_ang[23] = xPredIntraAngAdi_XY_23;

    uavs3d_funs_handle.intra_pred_ang[14] = xPredIntraAngAdi_XY_14;
    uavs3d_funs_handle.intra_pred_ang[16] = xPredIntraAngAdi_XY_16;
    uavs3d_funs_handle.intra_pred_ang[18] = xPredIntraAngAdi_XY_18;
    uavs3d_funs_handle.intra_pred_ang[20] = xPredIntraAngAdi_XY_20;
    uavs3d_funs_handle.intra_pred_ang[22] = xPredIntraAngAdi_XY_22;

    uavs3d_funs_handle.intra_pred_ang[26] = xPredIntraAngAdi_Y_26;
    uavs3d_funs_handle.intra_pred_ang[28] = xPredIntraAngAdi_Y_28;
    uavs3d_funs_handle.intra_pred_ang[30] = xPredIntraAngAdi_Y_30;
    uavs3d_funs_handle.intra_pred_ang[32] = xPredIntraAngAdi_Y_32;

    uavs3d_funs_handle.intra_pred_ipf     = ipred_ipf_core;
    uavs3d_funs_handle.intra_pred_ipf_s16 = ipred_ipf_core_s16;
}
