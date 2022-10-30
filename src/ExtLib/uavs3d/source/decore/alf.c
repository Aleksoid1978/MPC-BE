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

void com_alf_init_map(com_seqh_t *seqhdr, u8 *alf_idx_map)
{
    const int tab_region[NO_VAR_BINS] = { 0, 1, 4, 5, 15, 2, 3, 6, 14, 11, 10, 7, 13, 12,  9,  8 };
    int lcu_size = seqhdr->max_cuwh;
    int img_width_in_lcu  = seqhdr->pic_width_in_lcu;
    int img_height_in_lcu = seqhdr->pic_height_in_lcu;
    int regin_w = ((img_width_in_lcu  + 1) / 4 * lcu_size);
    int regin_h = ((img_height_in_lcu + 1) / 4 * lcu_size);

    seqhdr->alf_idx_map = alf_idx_map;

    for (int i = 0; i < img_height_in_lcu; i++) {
        int offset_y = (regin_h ? COM_MIN(3, (i * lcu_size) / regin_h) : 3) << 2;

        for (int j = 0; j < img_width_in_lcu; j++) {
            int offset_x = (regin_w ? COM_MIN(3, (j * lcu_size) / regin_w) : 3);
            alf_idx_map[j] = tab_region[offset_y + offset_x];
        }
        alf_idx_map += img_width_in_lcu;
    }
}

static void alf_one_blk(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *p1,*p2,*p3, *p4, *p5, *p6;

    int i, j;
    int max_pel = (1 << sample_bit_depth) - 1;

    for(i = 0; i < lcu_height; i++) {
        p1 = src + i_src;
        p2 = src - i_src;
        p3 = src + 2*i_src;
        p4 = src - 2*i_src;
        p5 = src + 3*i_src;
        p6 = src - 3*i_src;
        if (i < 3) {
            if(i == 0) {
                p4 = p2 = src;
            } else if (i == 1) {
                p4 = p2;
            }
            p6 = p4;
        } else if(i > lcu_height - 4) {
            if(i == lcu_height - 1) {
                p3 = p1 = src;
            } else if (i == lcu_height - 2) {
                p3 = p1;
            }
            p5 = p3;
        }
        
        for (j = 0; j < lcu_width; j++)
        {
            int sum;
            int xLeft = j - 1;
            int xRight = j + 1;

            sum = coef[0] * (p5[j] + p6[j]);
            sum += coef[1] * (p3[j] + p4[j]);
            sum += coef[2] * (p1[xRight] + p2[xLeft]);
            sum += coef[3] * (p1[j] + p2[j]);
            sum += coef[4] * (p1[xLeft] + p2[xRight]);
            sum += coef[7] * (src[xRight] + src[xLeft]);

            xLeft = j - 2;
            xRight = j + 2;
            sum += coef[6] * (src[xRight] + src[xLeft]);

            xLeft = j - 3;
            xRight = j + 3;
            sum += coef[5] * (src[xRight] + src[xLeft]);

            sum += coef[8] * (src[j]);

            sum = (int)((sum + 32) >> 6);

            dst[j] = COM_CLIP3(0, max_pel, sum);

        }
        src += i_src;
        dst += i_dst;
    }  
}

static void alf_one_blk_chroma(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *p1, *p2, *p3, *p4, *p5, *p6;

    int i, j;
    int startPos = 0;
    int endPos = lcu_height;
    int max_pel = (1 << sample_bit_depth) - 1;

    for (i = startPos; i < endPos; i++)
    {
        p1 = src + i_src;
        p2 = src - i_src;
        p3 = src + 2 * i_src;
        p4 = src - 2 * i_src;
        p5 = src + 3 * i_src;
        p6 = src - 3 * i_src;
        if (i < 3) {
            if (i == 0) {
                p4 = p2 = src;
            }
            else if (i == 1) {
                p4 = p2;
            }
            p6 = p4;
        }
        else if (i > lcu_height - 4) {
            if (i == lcu_height - 1) {
                p3 = p1 = src;
            }
            else if (i == lcu_height - 2) {
                p3 = p1;
            }
            p5 = p3;
        }

        for (j = 0; j < lcu_width << 1; j += 2)
        {
            int pixelIntU, pixelIntV;
            int xLeft = j - 2;
            int xRight = j + 2;

            pixelIntU  = coef[0 ] * (p5[j         ] + p6[j]);
            pixelIntV  = coef[1 ] * (p5[j + 1     ] + p6[j + 1]);
            pixelIntU += coef[2 ] * (p3[j         ] + p4[j]);
            pixelIntV += coef[3 ] * (p3[j + 1     ] + p4[j + 1]);
            pixelIntU += coef[4 ] * (p1[xRight    ] + p2[xLeft]);
            pixelIntV += coef[5 ] * (p1[xRight + 1] + p2[xLeft + 1]);
            pixelIntU += coef[6 ] * (p1[j         ] + p2[j]);
            pixelIntV += coef[7 ] * (p1[j + 1     ] + p2[j + 1]);
            pixelIntU += coef[8 ] * (p1[xLeft     ] + p2[xRight]);
            pixelIntV += coef[9 ] * (p1[xLeft + 1 ] + p2[xRight + 1]);
            pixelIntU += coef[14] * (src [xRight    ] + src [xLeft]);
            pixelIntV += coef[15] * (src [xRight + 1] + src [xLeft + 1]);

            xLeft  = j - 4;
            xRight = j + 4;
            pixelIntU += coef[12] * (src [xRight    ] + src [xLeft]);
            pixelIntV += coef[13] * (src [xRight + 1] + src [xLeft + 1]);

            xLeft  = j - 6;
            xRight = j + 6;
            pixelIntU += coef[10] * (src [xRight    ] + src [xLeft]);
            pixelIntV += coef[11] * (src [xRight + 1] + src [xLeft + 1]);

            pixelIntU += coef[16] * (src[j]);
            pixelIntV += coef[17] * (src[j + 1]);

            pixelIntU = (int)((pixelIntU + 32) >> 6);
            pixelIntV = (int)((pixelIntV + 32) >> 6);

            dst[j    ] = COM_CLIP3(0, max_pel, pixelIntU);
            dst[j + 1] = COM_CLIP3(0, max_pel, pixelIntV);

        }
        src += i_src;
        dst += i_dst;
    }
}

static void alf_one_blk_chroma_u_or_v(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *p1, *p2, *p3, *p4, *p5, *p6;

    int i, j;
    int startPos = 0;
    int endPos = lcu_height;

    for (i = startPos; i < endPos; i++)
    {
        p1 = src + i_src;
        p2 = src - i_src;
        p3 = src + 2 * i_src;
        p4 = src - 2 * i_src;
        p5 = src + 3 * i_src;
        p6 = src - 3 * i_src;
        if (i < 3) {
            if (i == 0) {
                p4 = p2 = src;
            }
            else if (i == 1) {
                p4 = p2;
            }
            p6 = p4;
        }
        else if (i > lcu_height - 4) {
            if (i == lcu_height - 1) {
                p3 = p1 = src;
            }
            else if (i == lcu_height - 2) {
                p3 = p1;
            }
            p5 = p3;
        }

        for (j = 0; j < lcu_width << 1; j += 2)
        {
            int sum;
            int xLeft = j - 2;
            int xRight = j + 2;

            sum = coef[0] * (p5[j] + p6[j]);
            sum += coef[2] * (p3[j] + p4[j]);
            sum += coef[4] * (p1[xRight] + p2[xLeft]);
            sum += coef[6] * (p1[j] + p2[j]);
            sum += coef[8] * (p1[xLeft] + p2[xRight]);
            sum += coef[14] * (src[xRight] + src[xLeft]);

            xLeft = j - 4;
            xRight = j + 4;
            sum += coef[12] * (src[xRight] + src[xLeft]);

            xLeft = j - 6;
            xRight = j + 6;
            sum += coef[10] * (src[xRight] + src[xLeft]);

            sum += coef[16] * (src[j]);

            sum = (int)((sum + 32) >> 6);

            dst[j] = COM_CLIP3(0, ((1 << sample_bit_depth) - 1), sum);

        }
        src += i_src;
        dst += i_dst;
    }
}

static void alf_one_blk_fix(pel *dst, int i_dst, pel *src, int i_src,  int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *p1, *p2, *p3, *p4, *p5, *p6;
    int sum;
    int max_pel = (1 << sample_bit_depth) - 1;

    if (src[0] != src[-1]) {
        p1 = src + 1 * i_src;
        p2 = src;
        p3 = src + 2 * i_src;
        p4 = src;
        p5 = src + 3 * i_src;
        p6 = src;

        sum  = coef[0] * (p5[ 0] + p6[ 0]);
        sum += coef[1] * (p3[ 0] + p4[ 0]);
        sum += coef[2] * (p1[ 1] + p2[ 0]);
        sum += coef[3] * (p1[ 0] + p2[ 0]);
        sum += coef[4] * (p1[-1] + p2[ 1]);
        sum += coef[7] * (src [ 1] + src [-1]);
        sum += coef[6] * (src [ 2] + src [-2]);
        sum += coef[5] * (src [ 3] + src [-3]);
        sum += coef[8] * (src [ 0]);

        sum = (int)((sum + 32) >> 6);
        dst[0] = COM_CLIP3(0, max_pel, sum);
    }

    src += lcu_width - 1;
    dst += lcu_width - 1;

    if (src[0] != src[1]) {
        p1 = src + 1 * i_src;
        p2 = src;
        p3 = src + 2 * i_src;
        p4 = src;
        p5 = src + 3 * i_src;
        p6 = src;

        sum  = coef[0] * (p5[ 0] + p6[ 0]);
        sum += coef[1] * (p3[ 0] + p4[ 0]);
        sum += coef[2] * (p1[ 1] + p2[-1]);
        sum += coef[3] * (p1[ 0] + p2[ 0]);
        sum += coef[4] * (p1[-1] + p2[ 0]);
        sum += coef[7] * (src [ 1] + src [-1]);
        sum += coef[6] * (src [ 2] + src [-2]);
        sum += coef[5] * (src [ 3] + src [-3]);
        sum += coef[8] * (src [ 0]);

        sum = (int)((sum + 32) >> 6);
        dst[0] = COM_CLIP3(0, max_pel, sum);
    }

    /* last line */
    src -= lcu_width - 1;
    dst -= lcu_width - 1;
    src += ((lcu_height - 1) * i_src);
    dst += ((lcu_height - 1) * i_dst);

    if (src[0] != src[-1]) {
        p1 = src;
        p2 = src - 1 * i_src;
        p3 = src;
        p4 = src - 2 * i_src;
        p5 = src;
        p6 = src - 3 * i_src;

        sum  = coef[0] * (p5[ 0] + p6[ 0]);
        sum += coef[1] * (p3[ 0] + p4[ 0]);
        sum += coef[2] * (p1[ 1] + p2[-1]);
        sum += coef[3] * (p1[ 0] + p2[ 0]);
        sum += coef[4] * (p1[ 0] + p2[ 1]);
        sum += coef[7] * (src [ 1] + src [-1]);
        sum += coef[6] * (src [ 2] + src [-2]);
        sum += coef[5] * (src [ 3] + src [-3]);
        sum += coef[8] * (src [ 0]);

        sum = (int)((sum + 32) >> 6);
        dst[0] = COM_CLIP3(0, max_pel, sum);
    }

    src += lcu_width - 1;
    dst += lcu_width - 1;

    if (src[0] != src[1]) {
        p1 = src;
        p2 = src - 1 * i_src;
        p3 = src;
        p4 = src - 2 * i_src;
        p5 = src;
        p6 = src - 3 * i_src;

        sum  = coef[0] * (p5[ 0] + p6[ 0]);
        sum += coef[1] * (p3[ 0] + p4[ 0]);
        sum += coef[2] * (p1[ 0] + p2[-1]);
        sum += coef[3] * (p1[ 0] + p2[ 0]);
        sum += coef[4] * (p1[-1] + p2[ 1]);
        sum += coef[7] * (src [ 1] + src [-1]);
        sum += coef[6] * (src [ 2] + src [-2]);
        sum += coef[5] * (src [ 3] + src [-3]);
        sum += coef[8] * (src [ 0]);

        sum = (int)((sum + 32) >> 6);
        dst[0] = COM_CLIP3(0, max_pel, sum);
    }
}

static void alf_one_blk_fix_chroma(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *p1, *p2, *p3, *p4, *p5, *p6;
    int sum;
    int max_pel = (1 << sample_bit_depth) - 1;
    int lastCol = (lcu_width - 1) << 1;

    if (src[0] != src[-2]) {
        p1 = src + 1 * i_src;
        p2 = src;
        p3 = src + 2 * i_src;
        p4 = src;
        p5 = src + 3 * i_src;
        p6 = src;

        sum = coef[0] * (p5[0] + p6[0]);
        sum += coef[2] * (p3[0] + p4[0]);
        sum += coef[4] * (p1[2] + p2[0]);
        sum += coef[6] * (p1[0] + p2[0]);
        sum += coef[8] * (p1[-2] + p2[2]);
        sum += coef[14] * (src[2] + src[-2]);
        sum += coef[12] * (src[4] + src[-4]);
        sum += coef[10] * (src[6] + src[-6]);
        sum += coef[16] * (src[0]);

        sum = (int)((sum + 32) >> 6);
        dst[0] = COM_CLIP3(0, max_pel, sum);
    }

    src += lastCol;
    dst += lastCol;

    if (src[0] != src[2]) {
        p1 = src + 1 * i_src;
        p2 = src;
        p3 = src + 2 * i_src;
        p4 = src;
        p5 = src + 3 * i_src;
        p6 = src;

        sum = coef[0] * (p5[0] + p6[0]);
        sum += coef[2] * (p3[0] + p4[0]);
        sum += coef[4] * (p1[2] + p2[-2]);
        sum += coef[6] * (p1[0] + p2[0]);
        sum += coef[8] * (p1[-2] + p2[0]);
        sum += coef[14] * (src[2] + src[-2]);
        sum += coef[12] * (src[4] + src[-4]);
        sum += coef[10] * (src[6] + src[-6]);
        sum += coef[16] * (src[0]);

        sum = (int)((sum + 32) >> 6);
        dst[0] = COM_CLIP3(0, max_pel, sum);
    }

    /* last line */
    src -= lastCol;
    dst -= lastCol;
    src += ((lcu_height - 1) * i_src);
    dst += ((lcu_height - 1) * i_dst);

    if (src[0] != src[-2]) {
        p1 = src;
        p2 = src - 1 * i_src;
        p3 = src;
        p4 = src - 2 * i_src;
        p5 = src;
        p6 = src - 3 * i_src;

        sum = coef[0] * (p5[0] + p6[0]);
        sum += coef[2] * (p3[0] + p4[0]);
        sum += coef[4] * (p1[2] + p2[-2]);
        sum += coef[6] * (p1[0] + p2[0]);
        sum += coef[8] * (p1[0] + p2[2]);
        sum += coef[14] * (src[2] + src[-2]);
        sum += coef[12] * (src[4] + src[-4]);
        sum += coef[10] * (src[6] + src[-6]);
        sum += coef[16] * (src[0]);

        sum = (int)((sum + 32) >> 6);
        dst[0] = COM_CLIP3(0, max_pel, sum);
    }

    src += lastCol;
    dst += lastCol;

    if (src[0] != src[2]) {
        p1 = src;
        p2 = src - 1 * i_src;
        p3 = src;
        p4 = src - 2 * i_src;
        p5 = src;
        p6 = src - 3 * i_src;

        sum = coef[0] * (p5[0] + p6[0]);
        sum += coef[2] * (p3[0] + p4[0]);
        sum += coef[4] * (p1[0] + p2[-2]);
        sum += coef[6] * (p1[0] + p2[0]);
        sum += coef[8] * (p1[-2] + p2[2]);
        sum += coef[14] * (src[2] + src[-2]);
        sum += coef[12] * (src[4] + src[-4]);
        sum += coef[10] * (src[6] + src[-6]);
        sum += coef[16] * (src[0]);

        sum = (int)((sum + 32) >> 6);
        dst[0] = COM_CLIP3(0, max_pel, sum);
    }
}

void com_alf_lcu_row(com_core_t *core, int lcu_y)
{
    com_pic_t* pic = core->pic;
    com_seqh_t *seqhdr = core->seqhdr;
    com_map_t *map = &core->map;
    com_alf_pic_param_t *alfParam = &core->pichdr->alf_param;
    int lcu_size = seqhdr->max_cuwh;
    int pix_x, pix_y = lcu_y * lcu_size;
    int bit_depth = seqhdr->bit_depth_internal;
    int img_width = seqhdr->pic_width;
    int img_height = seqhdr->pic_height;
    int i_dstl = pic->stride_luma;
    int i_dstc = pic->stride_chroma;
    pel *dstl  = pic->y;
    pel *dstc  = pic->uv;
    int lcu_idx = lcu_y * seqhdr->pic_width_in_lcu;
    int lcu_h = COM_MIN(lcu_size, img_height - pix_y);
    int lcu_h_c = lcu_h >> 1;
    int isAboveAvail = (lcu_y > 0);
    int isBelowAvail = (lcu_y < seqhdr->pic_height_in_lcu - 1);
    int y_offset = 0, height_offset = 0;
    int pix_y_c = pix_y >> 1;
    int i_bufl = lcu_size + ALIGN_BASIC;
    int i_bufc = (lcu_size / 2 + ALIGN_BASIC) * 2;
    pel *bufl = core->alf_src_buf[0] + ALIGN_BASIC;
    pel *bufc = core->alf_src_buf[1] + ALIGN_BASIC;
    int adj_l = 3, adj_c = 6;

    if (!seqhdr->cross_patch_loop_filter && isAboveAvail) {
        s8* map_patch = map->map_patch + lcu_y * seqhdr->pic_width_in_lcu;
        if (isAboveAvail && map_patch[0] != map_patch[-seqhdr->pic_width_in_lcu]) {
            isAboveAvail = FALSE;
        }
    }
    if (isAboveAvail) {
        y_offset -= 4;
        height_offset += 4;
    }
    if (isBelowAvail) {
        height_offset -= 4;
    }
    dstl += (pix_y + y_offset) * i_dstl;
    dstc += (pix_y_c + y_offset) * i_dstc;
    
    lcu_h   += height_offset;
    lcu_h_c += height_offset;

    // padding
    pel *tmpl = dstl, *tmpr = dstl + img_width - 1;
    for (int i = 0; i < lcu_h; i++) {
#if (BIT_DEPTH == 8)
        M32(tmpl - 4) = tmpl[0] * 0x01010101;
        M32(tmpr + 1) = tmpr[0] * 0x01010101;
#else
        M64(tmpl - 4) = tmpl[0] * 0x0001000100010001;
        M64(tmpr + 1) = tmpr[0] * 0x0001000100010001;
#endif
        tmpl += i_dstl;
        tmpr += i_dstl;
    }

    tmpl = dstc, tmpr = dstc + img_width - 2;
    for (int i = 0; i < lcu_h_c; i++) {
#if (BIT_DEPTH == 8)
        M64(tmpl - 8) = M16(tmpl) * 0x0001000100010001;
        M64(tmpr + 2) = M16(tmpr) * 0x0001000100010001;
#else
        M64(tmpl - 8) = M64(tmpl - 4) = M32(tmpl) * 0x0000000100000001;
        M64(tmpr + 2) = M64(tmpr + 6) = M32(tmpr) * 0x0000000100000001;
#endif
        tmpl += i_dstc;
        tmpr += i_dstc;
    }

    for (pix_x = 0; pix_x < img_width; pix_x += lcu_size, lcu_idx++, dstl += lcu_size, dstc += lcu_size, bufl += lcu_size, bufc += lcu_size) {
        int lcu_w = COM_MIN(lcu_size, img_width  - pix_x);
        int lcu_w_c = lcu_w >> 1;
        u8 *enable_flag = core->alf_enable_map[lcu_idx];

        if (enable_flag[Y_C]) {
            int *coef = alfParam->filterCoeff_luma[alfParam->varIndTab[seqhdr->alf_idx_map[lcu_idx]]];
            pel *tmp_d = bufl - adj_l;
            pel *tmp_s = dstl - adj_l;
            int cpy_size = (lcu_w + 3 + adj_l) * sizeof(pel);
 
            for (int i = 0; i < lcu_h; i++) {
                memcpy(tmp_d, tmp_s, cpy_size);
                tmp_d += i_bufl;
                tmp_s += i_dstl;
            }
            uavs3d_funs_handle.alf    [Y_C](dstl, i_dstl, bufl, i_bufl, lcu_w, lcu_h, coef, bit_depth);
            uavs3d_funs_handle.alf_fix[Y_C](dstl, i_dstl, bufl, i_bufl, lcu_w, lcu_h, coef, bit_depth);
            adj_l = -3;
        } else {
            adj_l = 3;
        }
        if (enable_flag[U_C] || enable_flag[V_C]) {
            pel *tmp_d = bufc - adj_c;
            pel *tmp_s = dstc - adj_c;
            int cpy_size = (lcu_w_c * 2 + 6 + adj_c) * sizeof(pel);

            for (int i = 0; i < lcu_h_c; i++) {
                memcpy(tmp_d, tmp_s, cpy_size);
                tmp_d += i_bufc;
                tmp_s += i_dstc;
            }
            adj_c = -6;
        } else {
            adj_c = 6;
        }
        if (enable_flag[U_C] && enable_flag[V_C]) {
            uavs3d_funs_handle.alf    [UV_C](dstc,     i_dstc, bufc,     i_bufc, lcu_w_c, lcu_h_c, alfParam->filterCoeff_chroma    , bit_depth);
            uavs3d_funs_handle.alf_fix[UV_C](dstc,     i_dstc, bufc,     i_bufc, lcu_w_c, lcu_h_c, alfParam->filterCoeff_chroma    , bit_depth);
            uavs3d_funs_handle.alf_fix[UV_C](dstc + 1, i_dstc, bufc + 1, i_bufc, lcu_w_c, lcu_h_c, alfParam->filterCoeff_chroma + 1, bit_depth);
        } else {
            if (enable_flag[U_C]) {
                uavs3d_funs_handle.alf    [ 2  ](dstc, i_dstc, bufc, i_bufc, lcu_w_c, lcu_h_c, alfParam->filterCoeff_chroma, bit_depth);
                uavs3d_funs_handle.alf_fix[UV_C](dstc, i_dstc, bufc, i_bufc, lcu_w_c, lcu_h_c, alfParam->filterCoeff_chroma, bit_depth);
            }
            if (enable_flag[V_C]) {
                uavs3d_funs_handle.alf    [ 2  ](dstc + 1, i_dstc, bufc + 1, i_bufc, lcu_w_c, lcu_h_c, alfParam->filterCoeff_chroma + 1, bit_depth);
                uavs3d_funs_handle.alf_fix[UV_C](dstc + 1, i_dstc, bufc + 1, i_bufc, lcu_w_c, lcu_h_c, alfParam->filterCoeff_chroma + 1, bit_depth);
            }
        }
    }
}

void uavs3d_funs_init_alf_c()
{
    uavs3d_funs_handle.alf[ Y_C] = alf_one_blk;
    uavs3d_funs_handle.alf[UV_C] = alf_one_blk_chroma;
    uavs3d_funs_handle.alf[  2 ] = alf_one_blk_chroma_u_or_v;
    uavs3d_funs_handle.alf_fix[ Y_C] = alf_one_blk_fix;
    uavs3d_funs_handle.alf_fix[UV_C] = alf_one_blk_fix_chroma;
}


