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

static void sao_on_lcu(pel *src, int i_src, pel *dst, int i_dst, com_sao_param_t *sao_params, int smb_pix_height, int smb_pix_width, int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int sample_bit_depth)
{
    int type;
    int start_x, end_x, start_y, end_y;
    int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
    int x, y;
    s8 leftsign, rightsign, upsign, downsign;
    int diff;
    s8 signupline[MAX_CU_SIZE + 8], *signupline1;
    int reg = 0;
    int edgetype;
    int max_pel = (1 << sample_bit_depth) - 1;

    type = sao_params->type;

    switch (type) {
    case SAO_TYPE_EO_0: {

        start_x = smb_available_left ? 0 : 1;
        end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 1);

        for (y = 0; y < smb_pix_height; y++) {
            diff = src[start_x] - src[start_x - 1];
            leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = start_x; x < end_x; x++) {
                diff = src[x] - src[x + 1];
                rightsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                edgetype = leftsign + rightsign;
                leftsign = -rightsign;
                dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
            }
            dst += i_dst;
            src += i_src;
        }

    }
                        break;
    case SAO_TYPE_EO_90: {
        pel *dst_base = dst;
        pel *src_base = src;
        start_y = smb_available_up ? 0 : 1;
        end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1);
        for (x = 0; x < smb_pix_width; x++) {
            src = src_base + start_y * i_src;
            diff = src[0] - src[-i_src];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            dst = dst_base + start_y * i_dst;
            for (y = start_y; y < end_y; y++) {
                diff = src[0] - src[i_src];
                downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                edgetype = downsign + upsign;
                upsign = -downsign;
                *dst = COM_CLIP3(0, max_pel, src[0] + sao_params->offset[edgetype + 2]);
                dst += i_dst;
                src += i_src;
            }
            dst_base++;
            src_base++;
        }
    }
                         break;
    case SAO_TYPE_EO_135: {
        start_x_r0 = (smb_available_up && smb_available_left) ? 0 : 1;
        end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;
        start_x_r = smb_available_left ? 0 : 1;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        start_x_rn = smb_available_down ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
        end_x_rn = (smb_available_right && smb_available_down) ? smb_pix_width : (smb_pix_width - 1);

        //init the line buffer
        for (x = start_x_r + 1; x < end_x_r + 1; x++) {
            diff = src[x + i_src] - src[x - 1];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            signupline[x] = upsign;
        }
        //first row
        for (x = start_x_r0; x < end_x_r0; x++) {
            diff = src[x] - src[x - 1 - i_src];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            edgetype = upsign - signupline[x + 1];
            dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
        }
        dst += i_dst;
        src += i_src;

        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x++) {
                if (x == start_x_r) {
                    diff = src[x] - src[x - 1 - i_src];
                    upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                    signupline[x] = upsign;
                }
                diff = src[x] - src[x + 1 + i_src];
                downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                edgetype = downsign + signupline[x];
                dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
                signupline[x] = reg;
                reg = -downsign;
            }
            dst += i_dst;
            src += i_src;
        }
        //last row
        for (x = start_x_rn; x < end_x_rn; x++) {
            if (x == start_x_r) {
                diff = src[x] - src[x - 1 - i_src];
                upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                signupline[x] = upsign;
            }
            diff = src[x] - src[x + 1 + i_src];
            downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            edgetype = downsign + signupline[x];
            dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
        }
    }
                          break;
    case SAO_TYPE_EO_45: {
        start_x_r0 = smb_available_up ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
        end_x_r0 = (smb_available_up && smb_available_right) ? smb_pix_width : (smb_pix_width - 1);
        start_x_r = smb_available_left ? 0 : 1;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        start_x_rn = (smb_available_left && smb_available_down) ? 0 : 1;
        end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;

        //init the line buffer
        signupline1 = signupline + 1;
        for (x = start_x_r - 1; x < end_x_r - 1; x++) {
            diff = src[x + i_src] - src[x + 1];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            signupline1[x] = upsign;
        }
        //first row
        for (x = start_x_r0; x < end_x_r0; x++) {
            diff = src[x] - src[x + 1 - i_src];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            edgetype = upsign - signupline1[x - 1];
            dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
        }
        dst += i_dst;
        src += i_src;

        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x++) {
                if (x == end_x_r - 1) {
                    diff = src[x] - src[x + 1 - i_src];
                    upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                    signupline1[x] = upsign;
                }
                diff = src[x] - src[x - 1 + i_src];
                downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                edgetype = downsign + signupline1[x];
                dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
                signupline1[x - 1] = -downsign;
            }
            dst += i_dst;
            src += i_src;
        }
        for (x = start_x_rn; x < end_x_rn; x++) {
            if (x == end_x_r - 1) {
                diff = src[x] - src[x + 1 - i_src];
                upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                signupline1[x] = upsign;
            }
            diff = src[x] - src[x - 1 + i_src];
            downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            edgetype = downsign + signupline1[x];
            dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
        }
    }
                         break;
    case SAO_TYPE_BO: {
        for (y = 0; y < smb_pix_height; y++) {
            for (x = 0; x < smb_pix_width; x++) {
                int val = dst[x];
                int band_idx = val >> (sample_bit_depth - NUM_SAO_BO_CLASSES_IN_BIT);
                if (band_idx == sao_params->bandIdx[0]) {
                    dst[x] = COM_CLIP3(0, max_pel, val + sao_params->offset[0]);
                } 
                else if (band_idx == sao_params->bandIdx[1]) {
                    dst[x] = COM_CLIP3(0, max_pel, val + sao_params->offset[1]);
                }
                else if (band_idx == sao_params->bandIdx[2]) {
                    dst[x] = COM_CLIP3(0, max_pel, val + sao_params->offset[2]);
                }
                else if (band_idx == sao_params->bandIdx[3]) {
                    dst[x] = COM_CLIP3(0, max_pel, val + sao_params->offset[3]);
                }
               
            }
            dst += i_dst;
        }
    }
                      break;
    default: {
        fprintf(stderr, "Not a supported SAO types\n");
        uavs3d_assert(0);
        exit(-1);
    }
    }
}

static void sao_on_lcu_chroma(pel *src, int i_src, pel *dst, int i_dst, com_sao_param_t *sao_params, int smb_pix_height, int smb_pix_width, int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int sample_bit_depth)
{
    int type;
    int start_x, end_x, start_y, end_y;
    int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
    int x, y;
    s8 leftsign, rightsign, upsign, downsign;
    int diff;
    s8 signupline[MAX_CU_SIZE + 8], *signupline1;
    int reg = 0;
    int edgetype;
    int max_pel = (1 << sample_bit_depth) - 1;

    type = sao_params->type;
    smb_pix_width <<= 1; 

    switch (type) {
    case SAO_TYPE_EO_0: {

        start_x = smb_available_left ? 0 : 2;
        end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 2);

        for (y = 0; y < smb_pix_height; y++) {
            diff = src[start_x] - src[start_x - 2];
            leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = start_x; x < end_x; x += 2) {
                diff = src[x] - src[x + 2];
                rightsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                edgetype = leftsign + rightsign;
                leftsign = -rightsign;
                dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
            }
            dst += i_dst;
            src += i_src;
        }

    }
                        break;
    case SAO_TYPE_EO_90: {
        pel *dst_base = dst;
        pel *src_base = src;
        start_y = smb_available_up ? 0 : 1;
        end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1);
        for (x = 0; x < smb_pix_width; x += 2) {
            src = src_base + start_y * i_src;
            diff = src[0] - src[-i_src];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            dst = dst_base + start_y * i_dst;
            for (y = start_y; y < end_y; y++) {
                diff = src[0] - src[i_src];
                downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                edgetype = downsign + upsign;
                upsign = -downsign;
                *dst = COM_CLIP3(0, max_pel, src[0] + sao_params->offset[edgetype + 2]);
                dst += i_dst;
                src += i_src;
            }
            dst_base += 2;
            src_base += 2;
        }
    }
                         break;
    case SAO_TYPE_EO_135: {
        start_x_r0 = (smb_available_up && smb_available_left) ? 0 : 2;
        end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 2)) : 2;
        start_x_r = smb_available_left ? 0 : 2;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        start_x_rn = smb_available_down ? (smb_available_left ? 0 : 2) : (smb_pix_width - 2);
        end_x_rn = (smb_available_right && smb_available_down) ? smb_pix_width : (smb_pix_width - 2);

        //init the line buffer
        for (x = start_x_r + 2; x < end_x_r + 2; x += 2) {
            diff = src[x + i_src] - src[x - 2];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            signupline[x >> 1] = upsign;
        }
        //first row
        for (x = start_x_r0; x < end_x_r0; x += 2) {
            diff = src[x] - src[x - 2 - i_src];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            edgetype = upsign - signupline[(x >> 1) + 1];
            dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
        }
        dst += i_dst;
        src += i_src;

        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 2) {
                int x2 = x >> 1;
                if (x == start_x_r) {
                    diff = src[x] - src[x - 2 - i_src];
                    upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                    signupline[x2] = upsign;
                }
                diff = src[x] - src[x + 2 + i_src];
                downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                edgetype = downsign + signupline[x2];
                dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
                signupline[x2] = reg;
                reg = -downsign;
            }
            dst += i_dst;
            src += i_src;
        }
        //last row
        for (x = start_x_rn; x < end_x_rn; x += 2) {
            if (x == start_x_r) {
                diff = src[x] - src[x - 2 - i_src];
                upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                signupline[x >> 1] = upsign;
            }
            diff = src[x] - src[x + 2 + i_src];
            downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            edgetype = downsign + signupline[x >> 1];
            dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
        }
    }
                          break;
    case SAO_TYPE_EO_45: {
        start_x_r0 = smb_available_up ? (smb_available_left ? 0 : 2) : (smb_pix_width - 2);
        end_x_r0 = (smb_available_up && smb_available_right) ? smb_pix_width : (smb_pix_width - 2);
        start_x_r = smb_available_left ? 0 : 2;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        start_x_rn = (smb_available_left && smb_available_down) ? 0 : 2;
        end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 2)) : 2;

        //init the line buffer
        signupline1 = signupline + 1;
        for (x = start_x_r - 2; x < end_x_r - 2; x += 2) {
            diff = src[x + i_src] - src[x + 2];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            signupline1[x >> 1] = upsign;
        }
        //first row
        for (x = start_x_r0; x < end_x_r0; x += 2) {
            diff = src[x] - src[x + 2 - i_src];
            upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            edgetype = upsign - signupline1[(x >> 1) - 1];
            dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
        }
        dst += i_dst;
        src += i_src;

        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 2) {
                int x2 = x >> 1;
                if (x == end_x_r - 2) {
                    diff = src[x] - src[x + 2 - i_src];
                    upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                    signupline1[x2] = upsign;
                }
                diff = src[x] - src[x - 2 + i_src];
                downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                edgetype = downsign + signupline1[x2];
                dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
                signupline1[x2 - 1] = -downsign;
            }
            dst += i_dst;
            src += i_src;
        }
        for (x = start_x_rn; x < end_x_rn; x += 2) {
            if (x == end_x_r - 2) {
                diff = src[x] - src[x + 2 - i_src];
                upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                signupline1[x >> 1] = upsign;
            }
            diff = src[x] - src[x - 2 + i_src];
            downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            edgetype = downsign + signupline1[x >> 1];
            dst[x] = COM_CLIP3(0, max_pel, src[x] + sao_params->offset[edgetype + 2]);
        }
    }
                         break;
    case SAO_TYPE_BO: {
        for (y = 0; y < smb_pix_height; y++) {
            for (x = 0; x < smb_pix_width; x += 2) {
                int val = dst[x];
                int band_idx = val >> (sample_bit_depth - NUM_SAO_BO_CLASSES_IN_BIT);
                if (band_idx == sao_params->bandIdx[0]) {
                    dst[x] = COM_CLIP3(0, max_pel, val + sao_params->offset[0]);
                }
                else if (band_idx == sao_params->bandIdx[1]) {
                    dst[x] = COM_CLIP3(0, max_pel, val + sao_params->offset[1]);
                }
                else if (band_idx == sao_params->bandIdx[2]) {
                    dst[x] = COM_CLIP3(0, max_pel, val + sao_params->offset[2]);
                }
                else if (band_idx == sao_params->bandIdx[3]) {
                    dst[x] = COM_CLIP3(0, max_pel, val + sao_params->offset[3]);
                }

            }
            dst += i_dst;
        }

    }
                      break;
    default: {
        fprintf(stderr, "Not a supported SAO types\n");
        uavs3d_assert(0);
        exit(-1);
    }
    }
}

void com_sao_lcu_row(com_core_t *core, int lcu_y)
{
    com_pic_t  *pic_rec =  core->pic;
    com_seqh_t *seqhdr    =  core->seqhdr;
    com_map_t  *map     = &core->map;
    int pic_pix_height = seqhdr->pic_height;
    int pic_pix_width  = seqhdr->pic_width;
    int lcu_size = seqhdr->max_cuwh;
    int sample_bit_depth = seqhdr->bit_depth_internal;
    s8* map_patch = map->map_patch;
    int pix_x, pix_y = lcu_y * lcu_size;
    int lcu_pix_height = COM_MIN(lcu_size, pic_pix_height - pix_y);
    int lcu_idx = lcu_y * seqhdr->pic_width_in_lcu;
    int scup = (pix_y >> MIN_CU_LOG2) * seqhdr->i_scu;
    int isAboveAvail = pix_y ? 1 : 0;
    int isBelowAvail = pix_y + lcu_size < seqhdr->pic_height ? 1 : 0;
    int isFltSliceBorder = isAboveAvail;
    int i_bufl = lcu_size + SAO_SHIFT_PIX_NUM + 2;
    int i_bufc = (lcu_size / 2 + SAO_SHIFT_PIX_NUM + 2) * 2;
    int i_dstl = pic_rec->stride_luma;
    int i_dstc = pic_rec->stride_chroma;
    pel *bufl = core->sao_src_buf[0] + (SAO_SHIFT_PIX_NUM + 1) * i_bufl + (SAO_SHIFT_PIX_NUM + 1);
    pel *bufc = core->sao_src_buf[1] + (SAO_SHIFT_PIX_NUM + 1) * i_bufc + (SAO_SHIFT_PIX_NUM + 1) * 2;
    pel *dstl = pic_rec->y  + pix_y     * i_dstl;
    pel *dstc = pic_rec->uv + pix_y / 2 * i_dstc;
    pel *linel = core->linebuf_sao[0];
    pel *linec = core->linebuf_sao[1];
    pel *line_srcl = dstl + (lcu_size - SAO_SHIFT_PIX_NUM - 1) * i_dstl;
    pel *line_srcc = dstc + (lcu_size / 2 - SAO_SHIFT_PIX_NUM - 1) * i_dstc;
    pel topleftl = 0, topleftc[2] = { 0 };
    int adj_l = 0, adj_c = 0;
    int y_offset = 0, height_offset = 0, height_offset_bo = 0;

    if (isAboveAvail) {
        y_offset -= SAO_SHIFT_PIX_NUM;
        height_offset_bo += SAO_SHIFT_PIX_NUM;
    }
    if (isBelowAvail) {
        height_offset -= SAO_SHIFT_PIX_NUM;
    }

    if (!seqhdr->cross_patch_loop_filter && isFltSliceBorder && map_patch[lcu_idx] != map_patch[lcu_idx - seqhdr->pic_width_in_lcu]) {
        isFltSliceBorder = 0;
    }

    for (pix_x = 0; pix_x < pic_pix_width; pix_x += lcu_size, lcu_idx++, dstl += lcu_size, dstc += lcu_size) {
        com_sao_param_t *saoBlkParam = core->sao_param_map[lcu_idx];
        int sao_chroma = saoBlkParam[U_C].enable | saoBlkParam[V_C].enable;
        int lcu_pix_width = COM_MIN(lcu_size, pic_pix_width - pix_x);
        int isLeftAvail = (pix_x != 0);
        int isRightAvail = pix_x + lcu_pix_width < seqhdr->pic_width;
        int x_offset = 0, width_offset = 0;
        int blk_x_l, blk_x_c, blk_widthl, blk_widthc;
        pel *src, *dst;
        pel *ll, *lc;
        const int uv_shift = 1;
   
        if (isLeftAvail) {
            x_offset -= SAO_SHIFT_PIX_NUM;
            width_offset += SAO_SHIFT_PIX_NUM;
        }
        if (isRightAvail) {
            width_offset -= SAO_SHIFT_PIX_NUM;
        }

        blk_widthl = width_offset +  lcu_pix_width;
        blk_widthc = width_offset + (lcu_pix_width >> uv_shift);
        blk_x_l = pix_x + x_offset;
        blk_x_c = pix_x + x_offset * 2;
        ll = linel + blk_x_l;
        lc = linec + blk_x_c;

#define FLUSH_LUMA() if (isBelowAvail) { \
    memcpy(ll, line_srcl + blk_x_l, (blk_widthl - isRightAvail) * sizeof(pel)); \
    ll[-1] = topleftl; \
    topleftl = line_srcl[blk_x_l + blk_widthl - isRightAvail]; \
}
#define FLUSH_CHROMA() if (isBelowAvail) { \
    memcpy(lc, line_srcc + blk_x_c, (blk_widthc - isRightAvail) * sizeof(pel) * 2); \
    lc[- 2] = topleftc[0]; \
    lc[- 1] = topleftc[1]; \
    topleftc[0] = line_srcc[blk_x_c + (blk_widthc - isRightAvail) * 2]; \
    topleftc[1] = line_srcc[blk_x_c + (blk_widthc - isRightAvail) * 2 + 1]; \
}

        if (!saoBlkParam[Y_C].enable && !sao_chroma) {
            FLUSH_LUMA();
            FLUSH_CHROMA();
            adj_l = 1, adj_c = 2;
            continue;
        }

        if (saoBlkParam[Y_C].enable == 0) {
            FLUSH_LUMA();
            adj_l = 1;
        } else if (saoBlkParam[Y_C].type == SAO_TYPE_BO) {
            int cpy_offset = blk_widthl + adj_l - 1;
            pel *tmp_d = bufl + y_offset * i_bufl + x_offset - adj_l - i_bufl + cpy_offset;
            pel *tmp_s = dstl + y_offset * i_dstl + x_offset - adj_l + cpy_offset;
            int blk_height = height_offset + lcu_pix_height + height_offset_bo;
            tmp_d[0] = ll[cpy_offset - adj_l    ];
            tmp_d[1] = ll[cpy_offset - adj_l + 1];
            tmp_d += i_bufl;

            for (int i = -1; i < blk_height; i++) {
                tmp_d[0] = tmp_s[0];
                tmp_d[1] = tmp_s[1];
                tmp_d += i_bufl;
                tmp_s += i_dstl;
            }
            FLUSH_LUMA();
    
            dst = dstl + y_offset * i_dstl + x_offset;
            uavs3d_funs_handle.sao[Y_C](dst, i_dstl, dst, i_dstl, &(saoBlkParam[0]), blk_height, blk_widthl, isLeftAvail, isRightAvail, isFltSliceBorder, isBelowAvail, sample_bit_depth);

            bufl += lcu_size;
            adj_l = -1;

        } else {
            pel *tmp_d = bufl - SAO_SHIFT_PIX_NUM * i_bufl + x_offset - adj_l - i_bufl;
            pel *tmp_s = dstl - SAO_SHIFT_PIX_NUM * i_dstl + x_offset - adj_l;
            int blk_height = height_offset + lcu_pix_height;
            int cpy_size = (blk_widthl + 1 + adj_l) * sizeof(pel);

            memcpy(tmp_d, ll - adj_l, cpy_size);  tmp_d += i_bufl;
   
            for (int i = -SAO_SHIFT_PIX_NUM; i <= blk_height; i++) {
                memcpy(tmp_d, tmp_s, cpy_size);
                tmp_d += i_bufl;
                tmp_s += i_dstl;
            }
            FLUSH_LUMA();
            if (isAboveAvail) {
                dst = dstl - SAO_SHIFT_PIX_NUM * i_dstl + x_offset;
                src = bufl - SAO_SHIFT_PIX_NUM * i_bufl + x_offset;
                uavs3d_funs_handle.sao[Y_C](src, i_bufl, dst, i_dstl, &(saoBlkParam[0]), SAO_SHIFT_PIX_NUM, blk_widthl, isLeftAvail, isRightAvail, 1, isFltSliceBorder, sample_bit_depth);
            }
            dst = dstl + x_offset;
            src = bufl + x_offset;
            uavs3d_funs_handle.sao[Y_C](src, i_bufl, dst, i_dstl, &(saoBlkParam[0]), blk_height, blk_widthl, isLeftAvail, isRightAvail, isFltSliceBorder, isBelowAvail, sample_bit_depth);

            bufl += lcu_size;
            adj_l = -1;
        } 

        if (!sao_chroma) {
            FLUSH_CHROMA();
            adj_c = 2;
        } else if (saoBlkParam[U_C].type == SAO_TYPE_BO && saoBlkParam[V_C].type == SAO_TYPE_BO) {
            int cpy_offset = blk_widthc * 2 + adj_c - 2;
            pel *tmp_d = bufc + y_offset * i_bufc + x_offset * 2 - adj_c - i_bufc + cpy_offset;
            pel *tmp_s = dstc + y_offset * i_dstc + x_offset * 2 - adj_c + cpy_offset;
            int blk_height = height_offset + (lcu_pix_height >> uv_shift) + height_offset_bo;
 
            tmp_d[0] = lc[cpy_offset - adj_c + 0]; 
            tmp_d[1] = lc[cpy_offset - adj_c + 1];
            tmp_d[2] = lc[cpy_offset - adj_c + 2];
            tmp_d[3] = lc[cpy_offset - adj_c + 3];
            
            tmp_d += i_bufc;

            for (int i = -1; i < blk_height; i++) {
                tmp_d[0] = tmp_s[0];
                tmp_d[1] = tmp_s[1];
                tmp_d[2] = tmp_s[2];
                tmp_d[3] = tmp_s[3];
                tmp_d += i_bufc;
                tmp_s += i_dstc;
            }
            FLUSH_CHROMA();

            dst = dstc + y_offset * i_dstc + x_offset * 2;
            if (saoBlkParam[U_C].enable) {
                uavs3d_funs_handle.sao[UV_C](dst, i_dstc, dst, i_dstc, &(saoBlkParam[1]), blk_height, blk_widthc, isLeftAvail, isRightAvail, isFltSliceBorder, isBelowAvail, sample_bit_depth);
            }
            if (saoBlkParam[V_C].enable) {
                uavs3d_funs_handle.sao[UV_C](dst + 1, i_dstc, dst + 1, i_dstc, &(saoBlkParam[2]), blk_height, blk_widthc, isLeftAvail, isRightAvail, isFltSliceBorder, isBelowAvail, sample_bit_depth);
            }
            bufc += lcu_size;
            adj_c = -2;
        } else {
            pel *tmp_d = bufc - SAO_SHIFT_PIX_NUM * i_bufc + x_offset * 2 - adj_c - i_bufc;
            pel *tmp_s = dstc - SAO_SHIFT_PIX_NUM * i_dstc + x_offset * 2 - adj_c;
            int blk_height = height_offset + (lcu_pix_height >> uv_shift);
            int cpy_size = (blk_widthc * 2 + 2 + adj_c) * sizeof(pel);

            memcpy(tmp_d, lc - adj_c, cpy_size); tmp_d += i_bufc;

            for (int i = -SAO_SHIFT_PIX_NUM; i <= blk_height; i++) {
                memcpy(tmp_d, tmp_s, cpy_size);
                tmp_d += i_bufc;
                tmp_s += i_dstc;
            }
            FLUSH_CHROMA();

            if (isAboveAvail) {
                dst = dstc - SAO_SHIFT_PIX_NUM * i_dstc + x_offset * 2;
                src = bufc - SAO_SHIFT_PIX_NUM * i_bufc + x_offset * 2;
                if (saoBlkParam[U_C].enable) {
                    uavs3d_funs_handle.sao[UV_C](src, i_bufc, dst, i_dstc, &(saoBlkParam[1]), SAO_SHIFT_PIX_NUM, blk_widthc, isLeftAvail, isRightAvail, 1, isFltSliceBorder, sample_bit_depth);
                }
                if (saoBlkParam[V_C].enable) {
                    uavs3d_funs_handle.sao[UV_C](src + 1, i_bufc, dst + 1, i_dstc, &(saoBlkParam[2]), SAO_SHIFT_PIX_NUM, blk_widthc, isLeftAvail, isRightAvail, 1, isFltSliceBorder, sample_bit_depth);
                }
            }
            dst = dstc + x_offset * 2;
            src = bufc + x_offset * 2;
            if (saoBlkParam[U_C].enable) {
                uavs3d_funs_handle.sao[UV_C](src, i_bufc, dst, i_dstc, &(saoBlkParam[1]), blk_height, blk_widthc, isLeftAvail, isRightAvail, isFltSliceBorder, isBelowAvail, sample_bit_depth);
            }
            if (saoBlkParam[V_C].enable) {
                uavs3d_funs_handle.sao[UV_C](src + 1, i_bufc, dst + 1, i_dstc, &(saoBlkParam[2]), blk_height, blk_widthc, isLeftAvail, isRightAvail, isFltSliceBorder, isBelowAvail, sample_bit_depth);
            }
            bufc += lcu_size;
            adj_c = -2;
        }
    }
}

void uavs3d_funs_init_sao_c()
{
    uavs3d_funs_handle.sao[ Y_C] = sao_on_lcu;
    uavs3d_funs_handle.sao[UV_C] = sao_on_lcu_chroma;
}