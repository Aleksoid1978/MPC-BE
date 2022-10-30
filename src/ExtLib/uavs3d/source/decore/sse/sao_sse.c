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

#include "sse.h"

#if (BIT_DEPTH == 8)

void uavs3d_sao_on_lcu_sse(pel *src, int i_src, pel *dst, int i_dst, com_sao_param_t *sao_params, int smb_pix_height,
    int smb_pix_width, int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int sample_bit_depth)
{
    com_sao_param_t *saoBlkParam = (com_sao_param_t*)sao_params;
    int type;
    int start_x, end_x, start_y, end_y;
    int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
    int x, y;
    __m128i clipMin = _mm_setzero_si128();

    type = saoBlkParam->type;

    switch (type) {
    case SAO_TYPE_EO_0: {
        __m128i off0, off1, off2, off3, off4;
        __m128i s0, s1, s2;
        __m128i t0, t1, t2, t3, t4, etype;
        __m128i c0, c1, c2, c3, c4;
        __m128i mask;
        int end_x_16;

        c0 = _mm_set1_epi8(-2);
        c1 = _mm_set1_epi8(-1);
        c2 = _mm_set1_epi8(0);
        c3 = _mm_set1_epi8(1);
        c4 = _mm_set1_epi8(2);

        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);
        off4 = _mm_set1_epi8(saoBlkParam->offset[4]);

        start_x = smb_available_left ? 0 : 1;
        end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        end_x_16 = end_x - ((end_x - start_x) & 0x0f);

        mask = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x - end_x_16 - 1]));
        for (y = 0; y < smb_pix_height; y++) {
            //diff = src[start_x] - src[start_x - 1];
            //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = start_x; x < end_x; x += 16) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - 1]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + 1]);

                t3 = _mm_min_epu8(s0, s1);
                t1 = _mm_cmpeq_epi8(t3, s0);
                t2 = _mm_cmpeq_epi8(t3, s1);
                t0 = _mm_subs_epi8(t2, t1); //leftsign

                t3 = _mm_min_epu8(s1, s2);
                t1 = _mm_cmpeq_epi8(t3, s1);
                t2 = _mm_cmpeq_epi8(t3, s2);
                t3 = _mm_subs_epi8(t1, t2); //rightsign

                etype = _mm_adds_epi8(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi8(etype, c0);
                t1 = _mm_cmpeq_epi8(etype, c1);
                t2 = _mm_cmpeq_epi8(etype, c2);
                t3 = _mm_cmpeq_epi8(etype, c3);
                t4 = _mm_cmpeq_epi8(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi8(t0, t1);
                t2 = _mm_adds_epi8(t2, t3);
                t0 = _mm_adds_epi8(t0, t4);
                t0 = _mm_adds_epi8(t0, t2);//get offset

                //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(s1, clipMin);
                t4 = _mm_unpackhi_epi8(s1, clipMin);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                t0 = _mm_packus_epi16(t1, t2); //saturated

                if (x != end_x_16){
                    _mm_storeu_si128((__m128i*)(dst + x), t0);
                }
                else{
                    _mm_maskmoveu_si128(t0, mask, (s8*)(dst + x));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }

    }
        break;
    case SAO_TYPE_EO_90: {
        __m128i off0, off1, off2, off3, off4;
        __m128i s0, s1, s2;
        __m128i t0, t1, t2, t3, t4, etype;
        __m128i c0, c1, c2, c3, c4;
        __m128i mask = _mm_loadu_si128((__m128i*)(uavs3d_simd_mask[(smb_pix_width & 15) - 1]));
        int end_x_16 = smb_pix_width - 15;

        c0 = _mm_set1_epi8(-2);
        c1 = _mm_set1_epi8(-1);
        c2 = _mm_set1_epi8(0);
        c3 = _mm_set1_epi8(1);
        c4 = _mm_set1_epi8(2);

        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);
        off4 = _mm_set1_epi8(saoBlkParam->offset[4]);

        start_y = smb_available_up ? 0 : 1;
        end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1);

        dst += start_y * i_dst;
        src += start_y * i_src;

        for (y = start_y; y < end_y; y++) {
            for (x = 0; x < smb_pix_width; x += 16) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - i_src]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + i_src]);

                t3 = _mm_min_epu8(s0, s1);
                t1 = _mm_cmpeq_epi8(t3, s0);
                t2 = _mm_cmpeq_epi8(t3, s1);
                t0 = _mm_subs_epi8(t2, t1); //upsign

                t3 = _mm_min_epu8(s1, s2);
                t1 = _mm_cmpeq_epi8(t3, s1);
                t2 = _mm_cmpeq_epi8(t3, s2);
                t3 = _mm_subs_epi8(t1, t2); //downsign

                etype = _mm_adds_epi8(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi8(etype, c0);
                t1 = _mm_cmpeq_epi8(etype, c1);
                t2 = _mm_cmpeq_epi8(etype, c2);
                t3 = _mm_cmpeq_epi8(etype, c3);
                t4 = _mm_cmpeq_epi8(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi8(t0, t1);
                t2 = _mm_adds_epi8(t2, t3);
                t0 = _mm_adds_epi8(t0, t4);
                t0 = _mm_adds_epi8(t0, t2);//get offset

                //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(s1, clipMin);
                t4 = _mm_unpackhi_epi8(s1, clipMin);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                t0 = _mm_packus_epi16(t1, t2); //saturated

                if (x < end_x_16){
                    _mm_storeu_si128((__m128i*)(dst + x), t0);
                }
                else{
                    _mm_maskmoveu_si128(t0, mask, (s8*)(dst + x));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
    }
        break;
    case SAO_TYPE_EO_135: {
        __m128i off0, off1, off2, off3, off4;
        __m128i s0, s1, s2;
        __m128i t0, t1, t2, t3, t4, etype;
        __m128i c0, c1, c2, c3, c4;
        __m128i mask_r0, mask_r, mask_rn;
        int end_x_r0_16, end_x_r_16, end_x_rn_16;

        c0 = _mm_set1_epi8(-2);
        c1 = _mm_set1_epi8(-1);
        c2 = _mm_set1_epi8(0);
        c3 = _mm_set1_epi8(1);
        c4 = _mm_set1_epi8(2);

        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);
        off4 = _mm_set1_epi8(saoBlkParam->offset[4]);

        start_x_r0 = (smb_available_up && smb_available_left) ? 0 : 1;
        end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;
        start_x_r = smb_available_left ? 0 : 1;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        start_x_rn = smb_available_down ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
        end_x_rn = (smb_available_right && smb_available_down) ? smb_pix_width : (smb_pix_width - 1);

        end_x_r0_16 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x0f);
        end_x_r_16 = end_x_r - ((end_x_r - start_x_r) & 0x0f);
        end_x_rn_16 = end_x_rn - ((end_x_rn - start_x_rn) & 0x0f);


        //first row
        for (x = start_x_r0; x < end_x_r0; x += 16) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src - 1]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src + 1]);

            t3 = _mm_min_epu8(s0, s1);
            t1 = _mm_cmpeq_epi8(t3, s0);
            t2 = _mm_cmpeq_epi8(t3, s1);
            t0 = _mm_subs_epi8(t2, t1); //upsign

            t3 = _mm_min_epu8(s1, s2);
            t1 = _mm_cmpeq_epi8(t3, s1);
            t2 = _mm_cmpeq_epi8(t3, s2);
            t3 = _mm_subs_epi8(t1, t2); //downsign

            etype = _mm_adds_epi8(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi8(etype, c0);
            t1 = _mm_cmpeq_epi8(etype, c1);
            t2 = _mm_cmpeq_epi8(etype, c2);
            t3 = _mm_cmpeq_epi8(etype, c3);
            t4 = _mm_cmpeq_epi8(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi8(t0, t1);
            t2 = _mm_adds_epi8(t2, t3);
            t0 = _mm_adds_epi8(t0, t4);
            t0 = _mm_adds_epi8(t0, t2);//get offset

            //add 8 nums once for possible overflow
            t1 = _mm_cvtepi8_epi16(t0);
            t0 = _mm_srli_si128(t0, 8);
            t2 = _mm_cvtepi8_epi16(t0);
            t3 = _mm_unpacklo_epi8(s1, clipMin);
            t4 = _mm_unpackhi_epi8(s1, clipMin);

            t1 = _mm_adds_epi16(t1, t3);
            t2 = _mm_adds_epi16(t2, t4);
            t0 = _mm_packus_epi16(t1, t2); //saturated

            if (x != end_x_r0_16){
                _mm_storeu_si128((__m128i*)(dst + x), t0);
            }
            else{
                mask_r0 = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_r0 - end_x_r0_16 - 1]));
                _mm_maskmoveu_si128(t0, mask_r0, (s8*)(dst + x));
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_r = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_r - end_x_r_16 - 1]));
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 16) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - i_src - 1]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + i_src + 1]);

                t3 = _mm_min_epu8(s0, s1);
                t1 = _mm_cmpeq_epi8(t3, s0);
                t2 = _mm_cmpeq_epi8(t3, s1);
                t0 = _mm_subs_epi8(t2, t1); //upsign

                t3 = _mm_min_epu8(s1, s2);
                t1 = _mm_cmpeq_epi8(t3, s1);
                t2 = _mm_cmpeq_epi8(t3, s2);
                t3 = _mm_subs_epi8(t1, t2); //downsign

                etype = _mm_adds_epi8(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi8(etype, c0);
                t1 = _mm_cmpeq_epi8(etype, c1);
                t2 = _mm_cmpeq_epi8(etype, c2);
                t3 = _mm_cmpeq_epi8(etype, c3);
                t4 = _mm_cmpeq_epi8(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi8(t0, t1);
                t2 = _mm_adds_epi8(t2, t3);
                t0 = _mm_adds_epi8(t0, t4);
                t0 = _mm_adds_epi8(t0, t2);//get offset

                //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(s1, clipMin);
                t4 = _mm_unpackhi_epi8(s1, clipMin);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                t0 = _mm_packus_epi16(t1, t2); //saturated

                if (x != end_x_r_16){
                    _mm_storeu_si128((__m128i*)(dst + x), t0);
                }
                else{
                    _mm_maskmoveu_si128(t0, mask_r, (s8*)(dst + x));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        //last row
        for (x = start_x_rn; x < end_x_rn; x += 16) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src - 1]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src + 1]);

            t3 = _mm_min_epu8(s0, s1);
            t1 = _mm_cmpeq_epi8(t3, s0);
            t2 = _mm_cmpeq_epi8(t3, s1);
            t0 = _mm_subs_epi8(t2, t1); //upsign

            t3 = _mm_min_epu8(s1, s2);
            t1 = _mm_cmpeq_epi8(t3, s1);
            t2 = _mm_cmpeq_epi8(t3, s2);
            t3 = _mm_subs_epi8(t1, t2); //downsign

            etype = _mm_adds_epi8(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi8(etype, c0);
            t1 = _mm_cmpeq_epi8(etype, c1);
            t2 = _mm_cmpeq_epi8(etype, c2);
            t3 = _mm_cmpeq_epi8(etype, c3);
            t4 = _mm_cmpeq_epi8(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi8(t0, t1);
            t2 = _mm_adds_epi8(t2, t3);
            t0 = _mm_adds_epi8(t0, t4);
            t0 = _mm_adds_epi8(t0, t2);//get offset

            //add 8 nums once for possible overflow
            t1 = _mm_cvtepi8_epi16(t0);
            t0 = _mm_srli_si128(t0, 8);
            t2 = _mm_cvtepi8_epi16(t0);
            t3 = _mm_unpacklo_epi8(s1, clipMin);
            t4 = _mm_unpackhi_epi8(s1, clipMin);

            t1 = _mm_adds_epi16(t1, t3);
            t2 = _mm_adds_epi16(t2, t4);
            t0 = _mm_packus_epi16(t1, t2); //saturated

            if (x != end_x_rn_16){
                _mm_storeu_si128((__m128i*)(dst + x), t0);
            }
            else{
                mask_rn = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_rn - end_x_rn_16 - 1]));
                _mm_maskmoveu_si128(t0, mask_rn, (s8*)(dst + x));
                break;
            }
        }
    }
        break;
    case SAO_TYPE_EO_45: {
        __m128i off0, off1, off2, off3, off4;
        __m128i s0, s1, s2;
        __m128i t0, t1, t2, t3, t4, etype;
        __m128i c0, c1, c2, c3, c4;
        __m128i mask_r0, mask_r, mask_rn;
        int end_x_r0_16, end_x_r_16, end_x_rn_16;

        c0 = _mm_set1_epi8(-2);
        c1 = _mm_set1_epi8(-1);
        c2 = _mm_set1_epi8(0);
        c3 = _mm_set1_epi8(1);
        c4 = _mm_set1_epi8(2);

        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);
        off4 = _mm_set1_epi8(saoBlkParam->offset[4]);

        start_x_r0 = smb_available_up ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
        end_x_r0 = (smb_available_up && smb_available_right) ? smb_pix_width : (smb_pix_width - 1);
        start_x_r = smb_available_left ? 0 : 1;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        start_x_rn = (smb_available_left && smb_available_down) ? 0 : 1;
        end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;

        end_x_r0_16 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x0f);
        end_x_r_16 = end_x_r - ((end_x_r - start_x_r) & 0x0f);
        end_x_rn_16 = end_x_rn - ((end_x_rn - start_x_rn) & 0x0f);


        //first row
        for (x = start_x_r0; x < end_x_r0; x += 16) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src + 1]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src - 1]);

            t3 = _mm_min_epu8(s0, s1);
            t1 = _mm_cmpeq_epi8(t3, s0);
            t2 = _mm_cmpeq_epi8(t3, s1);
            t0 = _mm_subs_epi8(t2, t1); //upsign

            t3 = _mm_min_epu8(s1, s2);
            t1 = _mm_cmpeq_epi8(t3, s1);
            t2 = _mm_cmpeq_epi8(t3, s2);
            t3 = _mm_subs_epi8(t1, t2); //downsign

            etype = _mm_adds_epi8(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi8(etype, c0);
            t1 = _mm_cmpeq_epi8(etype, c1);
            t2 = _mm_cmpeq_epi8(etype, c2);
            t3 = _mm_cmpeq_epi8(etype, c3);
            t4 = _mm_cmpeq_epi8(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi8(t0, t1);
            t2 = _mm_adds_epi8(t2, t3);
            t0 = _mm_adds_epi8(t0, t4);
            t0 = _mm_adds_epi8(t0, t2);//get offset

            //add 8 nums once for possible overflow
            t1 = _mm_cvtepi8_epi16(t0);
            t0 = _mm_srli_si128(t0, 8);
            t2 = _mm_cvtepi8_epi16(t0);
            t3 = _mm_unpacklo_epi8(s1, clipMin);
            t4 = _mm_unpackhi_epi8(s1, clipMin);

            t1 = _mm_adds_epi16(t1, t3);
            t2 = _mm_adds_epi16(t2, t4);
            t0 = _mm_packus_epi16(t1, t2); //saturated

            if (x != end_x_r0_16){
                _mm_storeu_si128((__m128i*)(dst + x), t0);
            }
            else{
                mask_r0 = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_r0 - end_x_r0_16 - 1]));
                _mm_maskmoveu_si128(t0, mask_r0, (s8*)(dst + x));
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_r = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_r - end_x_r_16 - 1]));
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 16) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - i_src + 1]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + i_src - 1]);

                t3 = _mm_min_epu8(s0, s1);
                t1 = _mm_cmpeq_epi8(t3, s0);
                t2 = _mm_cmpeq_epi8(t3, s1);
                t0 = _mm_subs_epi8(t2, t1); //upsign

                t3 = _mm_min_epu8(s1, s2);
                t1 = _mm_cmpeq_epi8(t3, s1);
                t2 = _mm_cmpeq_epi8(t3, s2);
                t3 = _mm_subs_epi8(t1, t2); //downsign

                etype = _mm_adds_epi8(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi8(etype, c0);
                t1 = _mm_cmpeq_epi8(etype, c1);
                t2 = _mm_cmpeq_epi8(etype, c2);
                t3 = _mm_cmpeq_epi8(etype, c3);
                t4 = _mm_cmpeq_epi8(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi8(t0, t1);
                t2 = _mm_adds_epi8(t2, t3);
                t0 = _mm_adds_epi8(t0, t4);
                t0 = _mm_adds_epi8(t0, t2);//get offset

                //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(s1, clipMin);
                t4 = _mm_unpackhi_epi8(s1, clipMin);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                t0 = _mm_packus_epi16(t1, t2); //saturated

                if (x != end_x_r_16){
                    _mm_storeu_si128((__m128i*)(dst + x), t0);
                }
                else{
                    _mm_maskmoveu_si128(t0, mask_r, (s8*)(dst + x));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        for (x = start_x_rn; x < end_x_rn; x += 16) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src + 1]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src - 1]);

            t3 = _mm_min_epu8(s0, s1);
            t1 = _mm_cmpeq_epi8(t3, s0);
            t2 = _mm_cmpeq_epi8(t3, s1);
            t0 = _mm_subs_epi8(t2, t1); //upsign

            t3 = _mm_min_epu8(s1, s2);
            t1 = _mm_cmpeq_epi8(t3, s1);
            t2 = _mm_cmpeq_epi8(t3, s2);
            t3 = _mm_subs_epi8(t1, t2); //downsign

            etype = _mm_adds_epi8(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi8(etype, c0);
            t1 = _mm_cmpeq_epi8(etype, c1);
            t2 = _mm_cmpeq_epi8(etype, c2);
            t3 = _mm_cmpeq_epi8(etype, c3);
            t4 = _mm_cmpeq_epi8(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi8(t0, t1);
            t2 = _mm_adds_epi8(t2, t3);
            t0 = _mm_adds_epi8(t0, t4);
            t0 = _mm_adds_epi8(t0, t2);//get offset

            //add 8 nums once for possible overflow
            t1 = _mm_cvtepi8_epi16(t0);
            t0 = _mm_srli_si128(t0, 8);
            t2 = _mm_cvtepi8_epi16(t0);
            t3 = _mm_unpacklo_epi8(s1, clipMin);
            t4 = _mm_unpackhi_epi8(s1, clipMin);

            t1 = _mm_adds_epi16(t1, t3);
            t2 = _mm_adds_epi16(t2, t4);
            t0 = _mm_packus_epi16(t1, t2); //saturated

            if (x != end_x_rn_16){
                _mm_storeu_si128((__m128i*)(dst + x), t0);
            }
            else{
                mask_rn = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_rn - end_x_rn_16 - 1]));
                _mm_maskmoveu_si128(t0, mask_rn, (s8*)(dst + x));
                break;
            }
        }
    }
        break;
    case SAO_TYPE_BO: {
        __m128i r0, r1, r2, r3, off0, off1, off2, off3;
        __m128i t0, t1, t2, t3, t4, src0, src1;
        __m128i mask = _mm_load_si128((const __m128i*)uavs3d_simd_mask[(smb_pix_width & 15) - 1]);
        __m128i shift_mask = _mm_set1_epi8(31);
        int end_x_16 = smb_pix_width - 15;
        
        r0 = _mm_set1_epi8(saoBlkParam->bandIdx[0]);
        r1 = _mm_set1_epi8(saoBlkParam->bandIdx[1]);
        r2 = _mm_set1_epi8(saoBlkParam->bandIdx[2]);
        r3 = _mm_set1_epi8(saoBlkParam->bandIdx[3]);
        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);

        for (y = 0; y < smb_pix_height; y++) {
            for (x = 0; x < smb_pix_width; x += 16) {
                src0 = _mm_loadu_si128((__m128i*)&src[x]);
                src1 = _mm_and_si128(_mm_srai_epi16(src0, 3), shift_mask);

                t0 = _mm_cmpeq_epi8(src1, r0);
                t1 = _mm_cmpeq_epi8(src1, r1);
                t2 = _mm_cmpeq_epi8(src1, r2);
                t3 = _mm_cmpeq_epi8(src1, r3);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t0 = _mm_or_si128(t0, t1);
                t2 = _mm_or_si128(t2, t3);
                t0 = _mm_or_si128(t0, t2);
                //src0 = _mm_adds_epi8(src0, t0);
                //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(src0, clipMin);
                t4 = _mm_unpackhi_epi8(src0, clipMin);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                src0 = _mm_packus_epi16(t1, t2); //saturated
                
                if (x < end_x_16) {
                    _mm_storeu_si128((__m128i*)&dst[x], src0);
                }
                else {
                    _mm_maskmoveu_si128(src0, mask, (s8*)(dst + x));
                }
            }

            dst += i_dst;
            src += i_src;
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

void uavs3d_sao_on_lcu_chroma_sse(pel *src, int i_src, pel *dst, int i_dst, com_sao_param_t *saoBlkParam, int smb_pix_height,
    int smb_pix_width, int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int sample_bit_depth)
{
    int type;
    int start_x, end_x, start_y, end_y;
    int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
    int x, y;
    __m128i zero = _mm_setzero_si128();
    __m128i mask_uv = _mm_set1_epi16(0xff);

    type = saoBlkParam->type;

    smb_pix_width <<= 1;
    switch (type) {
    case SAO_TYPE_EO_0: {
        __m128i off0, off1, off2, off3, off4;
        __m128i s0, s1, s2, s3, s4, s5;
        __m128i t0, t1, t2, t3, t4, etype;
        __m128i c0, c1, c2, c3, c4;
        __m128i mask, mask1, mask2;
        int end_x_16;

        c0 = _mm_set1_epi8(-2);
        c1 = _mm_set1_epi8(-1);
        c2 = _mm_set1_epi8(0);
        c3 = _mm_set1_epi8(1);
        c4 = _mm_set1_epi8(2);

        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);
        off4 = _mm_set1_epi8(saoBlkParam->offset[4]);

        start_x = smb_available_left ? 0 : 2;
        end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        end_x_16 = end_x - ((end_x - start_x) & 0x1f);

        mask = _mm_load_si128((__m128i*)(uavs3d_simd_mask[((end_x - end_x_16)>>1) - 1]));
        mask1 = _mm_unpacklo_epi8(mask, zero);
        mask2 = _mm_unpackhi_epi8(mask, zero);
        mask1 = _mm_and_si128(mask1, mask_uv);
        mask2 = _mm_and_si128(mask2, mask_uv);

        for (y = 0; y < smb_pix_height; y++) {
            //diff = src[start_x] - src[start_x - 1];
            //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = start_x; x < end_x; x += 32) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - 2]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + 2]);
                s3 = _mm_loadu_si128((__m128i*)&src[x + 14]);
                s4 = _mm_loadu_si128((__m128i*)&src[x + 16]);
                s5 = _mm_loadu_si128((__m128i*)&src[x + 18]);
                s0 = _mm_and_si128(s0, mask_uv);
                s1 = _mm_and_si128(s1, mask_uv);
                s2 = _mm_and_si128(s2, mask_uv);
                s3 = _mm_and_si128(s3, mask_uv);
                s4 = _mm_and_si128(s4, mask_uv);
                s5 = _mm_and_si128(s5, mask_uv);
                s0 = _mm_packus_epi16(s0, s3);
                s1 = _mm_packus_epi16(s1, s4);
                s2 = _mm_packus_epi16(s2, s5);

                t3 = _mm_min_epu8(s0, s1);
                t1 = _mm_cmpeq_epi8(t3, s0);
                t2 = _mm_cmpeq_epi8(t3, s1);
                t0 = _mm_subs_epi8(t2, t1); //leftsign

                t3 = _mm_min_epu8(s1, s2);
                t1 = _mm_cmpeq_epi8(t3, s1);
                t2 = _mm_cmpeq_epi8(t3, s2);
                t3 = _mm_subs_epi8(t1, t2); //rightsign

                etype = _mm_adds_epi8(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi8(etype, c0);
                t1 = _mm_cmpeq_epi8(etype, c1);
                t2 = _mm_cmpeq_epi8(etype, c2);
                t3 = _mm_cmpeq_epi8(etype, c3);
                t4 = _mm_cmpeq_epi8(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi8(t0, t1);
                t2 = _mm_adds_epi8(t2, t3);
                t0 = _mm_adds_epi8(t0, t4);
                t0 = _mm_adds_epi8(t0, t2);//get offset

                //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(s1, zero);
                t4 = _mm_unpackhi_epi8(s1, zero);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                t0 = _mm_packus_epi16(t1, t2); //saturated

                t1 = _mm_unpacklo_epi8(t0, zero);
                t2 = _mm_unpackhi_epi8(t0, zero);
                if (x != end_x_16) {
                    _mm_maskmoveu_si128(t1, mask_uv, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask_uv, (s8*)(dst + x + 16));
                }
                else {
                    _mm_maskmoveu_si128(t1, mask1, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask2, (s8*)(dst + x + 16));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }

    }
    break;
    case SAO_TYPE_EO_90: {
        __m128i off0, off1, off2, off3, off4;
        __m128i s0, s1, s2, s3, s4, s5;
        __m128i t0, t1, t2, t3, t4, etype;
        __m128i c0, c1, c2, c3, c4;
        __m128i mask = _mm_loadu_si128((__m128i*)(uavs3d_simd_mask[((smb_pix_width>>1) & 15) - 1]));
        __m128i mask1, mask2;
        int end_x_16 = smb_pix_width - 31;

        c0 = _mm_set1_epi8(-2);
        c1 = _mm_set1_epi8(-1);
        c2 = _mm_set1_epi8(0);
        c3 = _mm_set1_epi8(1);
        c4 = _mm_set1_epi8(2);

        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);
        off4 = _mm_set1_epi8(saoBlkParam->offset[4]);

        start_y = smb_available_up ? 0 : 1;
        end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1);

        dst += start_y * i_dst;
        src += start_y * i_src;

        mask1 = _mm_unpacklo_epi8(mask, zero);
        mask2 = _mm_unpackhi_epi8(mask, zero);
        mask1 = _mm_and_si128(mask1, mask_uv);
        mask2 = _mm_and_si128(mask2, mask_uv);

        for (y = start_y; y < end_y; y++) {
            for (x = 0; x < smb_pix_width; x += 32) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - i_src]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + i_src]);
                s3 = _mm_loadu_si128((__m128i*)&src[x - i_src + 16]);
                s4 = _mm_loadu_si128((__m128i*)&src[x + 16]);
                s5 = _mm_loadu_si128((__m128i*)&src[x + i_src + 16]);
                s0 = _mm_and_si128(s0, mask_uv);
                s1 = _mm_and_si128(s1, mask_uv);
                s2 = _mm_and_si128(s2, mask_uv);
                s3 = _mm_and_si128(s3, mask_uv);
                s4 = _mm_and_si128(s4, mask_uv);
                s5 = _mm_and_si128(s5, mask_uv);
                s0 = _mm_packus_epi16(s0, s3);
                s1 = _mm_packus_epi16(s1, s4);
                s2 = _mm_packus_epi16(s2, s5);

                t3 = _mm_min_epu8(s0, s1);
                t1 = _mm_cmpeq_epi8(t3, s0);
                t2 = _mm_cmpeq_epi8(t3, s1);
                t0 = _mm_subs_epi8(t2, t1); //upsign

                t3 = _mm_min_epu8(s1, s2);
                t1 = _mm_cmpeq_epi8(t3, s1);
                t2 = _mm_cmpeq_epi8(t3, s2);
                t3 = _mm_subs_epi8(t1, t2); //downsign

                etype = _mm_adds_epi8(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi8(etype, c0);
                t1 = _mm_cmpeq_epi8(etype, c1);
                t2 = _mm_cmpeq_epi8(etype, c2);
                t3 = _mm_cmpeq_epi8(etype, c3);
                t4 = _mm_cmpeq_epi8(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi8(t0, t1);
                t2 = _mm_adds_epi8(t2, t3);
                t0 = _mm_adds_epi8(t0, t4);
                t0 = _mm_adds_epi8(t0, t2);//get offset

                                           //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(s1, zero);
                t4 = _mm_unpackhi_epi8(s1, zero);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                t0 = _mm_packus_epi16(t1, t2); //saturated
                t1 = _mm_unpacklo_epi8(t0, zero);
                t2 = _mm_unpackhi_epi8(t0, zero);
                if (x < end_x_16) {
                    _mm_maskmoveu_si128(t1, mask_uv, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask_uv, (s8*)(dst + x + 16));
                }
                else {
                    _mm_maskmoveu_si128(t1, mask1, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask2, (s8*)(dst + x + 16));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
    }
    break;
    case SAO_TYPE_EO_135: {
        __m128i off0, off1, off2, off3, off4;
        __m128i s0, s1, s2, s3, s4, s5;
        __m128i t0, t1, t2, t3, t4, etype;
        __m128i c0, c1, c2, c3, c4;
        __m128i mask_r0, mask_r, mask_rn, mask1, mask2;
        int end_x_r0_16, end_x_r_16, end_x_rn_16;

        c0 = _mm_set1_epi8(-2);
        c1 = _mm_set1_epi8(-1);
        c2 = _mm_set1_epi8(0);
        c3 = _mm_set1_epi8(1);
        c4 = _mm_set1_epi8(2);

        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);
        off4 = _mm_set1_epi8(saoBlkParam->offset[4]);

        start_x_r0 = (smb_available_up && smb_available_left) ? 0 : 2;
        end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 2)) : 2;
        start_x_r = smb_available_left ? 0 : 2;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        start_x_rn = smb_available_down ? (smb_available_left ? 0 : 2) : (smb_pix_width - 2);
        end_x_rn = (smb_available_right && smb_available_down) ? smb_pix_width : (smb_pix_width - 2);

        end_x_r0_16 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x1f);
        end_x_r_16 = end_x_r - ((end_x_r - start_x_r) & 0x1f);
        end_x_rn_16 = end_x_rn - ((end_x_rn - start_x_rn) & 0x1f);

        //first row
        for (x = start_x_r0; x < end_x_r0; x += 32) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src - 2]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src + 2]);
            s3 = _mm_loadu_si128((__m128i*)&src[x - i_src + 14]);
            s4 = _mm_loadu_si128((__m128i*)&src[x + 16]);
            s5 = _mm_loadu_si128((__m128i*)&src[x + i_src + 18]);
            s0 = _mm_and_si128(s0, mask_uv);
            s1 = _mm_and_si128(s1, mask_uv);
            s2 = _mm_and_si128(s2, mask_uv);
            s3 = _mm_and_si128(s3, mask_uv);
            s4 = _mm_and_si128(s4, mask_uv);
            s5 = _mm_and_si128(s5, mask_uv);
            s0 = _mm_packus_epi16(s0, s3);
            s1 = _mm_packus_epi16(s1, s4);
            s2 = _mm_packus_epi16(s2, s5);

            t3 = _mm_min_epu8(s0, s1);
            t1 = _mm_cmpeq_epi8(t3, s0);
            t2 = _mm_cmpeq_epi8(t3, s1);
            t0 = _mm_subs_epi8(t2, t1); //upsign

            t3 = _mm_min_epu8(s1, s2);
            t1 = _mm_cmpeq_epi8(t3, s1);
            t2 = _mm_cmpeq_epi8(t3, s2);
            t3 = _mm_subs_epi8(t1, t2); //downsign

            etype = _mm_adds_epi8(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi8(etype, c0);
            t1 = _mm_cmpeq_epi8(etype, c1);
            t2 = _mm_cmpeq_epi8(etype, c2);
            t3 = _mm_cmpeq_epi8(etype, c3);
            t4 = _mm_cmpeq_epi8(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi8(t0, t1);
            t2 = _mm_adds_epi8(t2, t3);
            t0 = _mm_adds_epi8(t0, t4);
            t0 = _mm_adds_epi8(t0, t2);//get offset

                                       //add 8 nums once for possible overflow
            t1 = _mm_cvtepi8_epi16(t0);
            t0 = _mm_srli_si128(t0, 8);
            t2 = _mm_cvtepi8_epi16(t0);
            t3 = _mm_unpacklo_epi8(s1, zero);
            t4 = _mm_unpackhi_epi8(s1, zero);

            t1 = _mm_adds_epi16(t1, t3);
            t2 = _mm_adds_epi16(t2, t4);
            t0 = _mm_packus_epi16(t1, t2); //saturated
            t1 = _mm_unpacklo_epi8(t0, zero);
            t2 = _mm_unpackhi_epi8(t0, zero);
            if (x != end_x_r0_16) {
                _mm_maskmoveu_si128(t1, mask_uv, (s8*)(dst + x));
                _mm_maskmoveu_si128(t2, mask_uv, (s8*)(dst + x + 16));
            }
            else {
                mask_r0 = _mm_load_si128((__m128i*)(uavs3d_simd_mask[((end_x_r0 - end_x_r0_16)>>1) - 1]));
                mask1 = _mm_unpacklo_epi8(mask_r0, zero);
                mask2 = _mm_unpackhi_epi8(mask_r0, zero);
                _mm_maskmoveu_si128(t1, mask1, (s8*)(dst + x));
                _mm_maskmoveu_si128(t2, mask2, (s8*)(dst + x + 16));
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_r = _mm_load_si128((__m128i*)(uavs3d_simd_mask[((end_x_r - end_x_r_16)>>1) - 1]));
        mask1 = _mm_unpacklo_epi8(mask_r, zero);
        mask2 = _mm_unpackhi_epi8(mask_r, zero);
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 32) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - i_src - 2]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + i_src + 2]);
                s3 = _mm_loadu_si128((__m128i*)&src[x - i_src + 14]);
                s4 = _mm_loadu_si128((__m128i*)&src[x + 16]);
                s5 = _mm_loadu_si128((__m128i*)&src[x + i_src + 18]);
                s0 = _mm_and_si128(s0, mask_uv);
                s1 = _mm_and_si128(s1, mask_uv);
                s2 = _mm_and_si128(s2, mask_uv);
                s3 = _mm_and_si128(s3, mask_uv);
                s4 = _mm_and_si128(s4, mask_uv);
                s5 = _mm_and_si128(s5, mask_uv);
                s0 = _mm_packus_epi16(s0, s3);
                s1 = _mm_packus_epi16(s1, s4);
                s2 = _mm_packus_epi16(s2, s5);

                t3 = _mm_min_epu8(s0, s1);
                t1 = _mm_cmpeq_epi8(t3, s0);
                t2 = _mm_cmpeq_epi8(t3, s1);
                t0 = _mm_subs_epi8(t2, t1); //upsign

                t3 = _mm_min_epu8(s1, s2);
                t1 = _mm_cmpeq_epi8(t3, s1);
                t2 = _mm_cmpeq_epi8(t3, s2);
                t3 = _mm_subs_epi8(t1, t2); //downsign

                etype = _mm_adds_epi8(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi8(etype, c0);
                t1 = _mm_cmpeq_epi8(etype, c1);
                t2 = _mm_cmpeq_epi8(etype, c2);
                t3 = _mm_cmpeq_epi8(etype, c3);
                t4 = _mm_cmpeq_epi8(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi8(t0, t1);
                t2 = _mm_adds_epi8(t2, t3);
                t0 = _mm_adds_epi8(t0, t4);
                t0 = _mm_adds_epi8(t0, t2);//get offset

                                           //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(s1, zero);
                t4 = _mm_unpackhi_epi8(s1, zero);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                t0 = _mm_packus_epi16(t1, t2); //saturated
                t1 = _mm_unpacklo_epi8(t0, zero);
                t2 = _mm_unpackhi_epi8(t0, zero);
                if (x != end_x_r_16) {
                    _mm_maskmoveu_si128(t1, mask_uv, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask_uv, (s8*)(dst + x + 16));
                }
                else {
                    _mm_maskmoveu_si128(t1, mask1, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask2, (s8*)(dst + x + 16));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        //last row
        for (x = start_x_rn; x < end_x_rn; x += 32) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src - 2]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src + 2]);
            s3 = _mm_loadu_si128((__m128i*)&src[x - i_src + 14]);
            s4 = _mm_loadu_si128((__m128i*)&src[x + 16]);
            s5 = _mm_loadu_si128((__m128i*)&src[x + i_src + 18]);
            s0 = _mm_and_si128(s0, mask_uv);
            s1 = _mm_and_si128(s1, mask_uv);
            s2 = _mm_and_si128(s2, mask_uv);
            s3 = _mm_and_si128(s3, mask_uv);
            s4 = _mm_and_si128(s4, mask_uv);
            s5 = _mm_and_si128(s5, mask_uv);
            s0 = _mm_packus_epi16(s0, s3);
            s1 = _mm_packus_epi16(s1, s4);
            s2 = _mm_packus_epi16(s2, s5);

            t3 = _mm_min_epu8(s0, s1);
            t1 = _mm_cmpeq_epi8(t3, s0);
            t2 = _mm_cmpeq_epi8(t3, s1);
            t0 = _mm_subs_epi8(t2, t1); //upsign

            t3 = _mm_min_epu8(s1, s2);
            t1 = _mm_cmpeq_epi8(t3, s1);
            t2 = _mm_cmpeq_epi8(t3, s2);
            t3 = _mm_subs_epi8(t1, t2); //downsign

            etype = _mm_adds_epi8(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi8(etype, c0);
            t1 = _mm_cmpeq_epi8(etype, c1);
            t2 = _mm_cmpeq_epi8(etype, c2);
            t3 = _mm_cmpeq_epi8(etype, c3);
            t4 = _mm_cmpeq_epi8(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi8(t0, t1);
            t2 = _mm_adds_epi8(t2, t3);
            t0 = _mm_adds_epi8(t0, t4);
            t0 = _mm_adds_epi8(t0, t2);//get offset

                                       //add 8 nums once for possible overflow
            t1 = _mm_cvtepi8_epi16(t0);
            t0 = _mm_srli_si128(t0, 8);
            t2 = _mm_cvtepi8_epi16(t0);
            t3 = _mm_unpacklo_epi8(s1, zero);
            t4 = _mm_unpackhi_epi8(s1, zero);

            t1 = _mm_adds_epi16(t1, t3);
            t2 = _mm_adds_epi16(t2, t4);
            t0 = _mm_packus_epi16(t1, t2); //saturated
            t1 = _mm_unpacklo_epi8(t0, zero);
            t2 = _mm_unpackhi_epi8(t0, zero);
            if (x != end_x_rn_16) {
                _mm_maskmoveu_si128(t1, mask_uv, (s8*)(dst + x));
                _mm_maskmoveu_si128(t2, mask_uv, (s8*)(dst + x + 16));
            }
            else {
                mask_rn = _mm_load_si128((__m128i*)(uavs3d_simd_mask[((end_x_rn - end_x_rn_16)>>1) - 1]));
                mask1 = _mm_unpacklo_epi8(mask_rn, zero);
                mask2 = _mm_unpackhi_epi8(mask_rn, zero);
                _mm_maskmoveu_si128(t1, mask1, (s8*)(dst + x));
                _mm_maskmoveu_si128(t2, mask2, (s8*)(dst + x + 16));
                break;
            }
        }
    }
    break;
    case SAO_TYPE_EO_45: {
        __m128i off0, off1, off2, off3, off4;
        __m128i s0, s1, s2, s3, s4, s5;
        __m128i t0, t1, t2, t3, t4, etype;
        __m128i c0, c1, c2, c3, c4;
        __m128i mask_r0, mask_r, mask_rn, mask1, mask2;
        int end_x_r0_16, end_x_r_16, end_x_rn_16;

        c0 = _mm_set1_epi8(-2);
        c1 = _mm_set1_epi8(-1);
        c2 = _mm_set1_epi8(0);
        c3 = _mm_set1_epi8(1);
        c4 = _mm_set1_epi8(2);

        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);
        off4 = _mm_set1_epi8(saoBlkParam->offset[4]);

        start_x_r0 = smb_available_up ? (smb_available_left ? 0 : 2) : (smb_pix_width - 2);
        end_x_r0 = (smb_available_up && smb_available_right) ? smb_pix_width : (smb_pix_width - 2);
        start_x_r = smb_available_left ? 0 : 2;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        start_x_rn = (smb_available_left && smb_available_down) ? 0 : 2;
        end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 2)) : 2;

        end_x_r0_16 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x1f);
        end_x_r_16 = end_x_r - ((end_x_r - start_x_r) & 0x1f);
        end_x_rn_16 = end_x_rn - ((end_x_rn - start_x_rn) & 0x1f);


        //first row
        for (x = start_x_r0; x < end_x_r0; x += 32) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src + 2]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src - 2]);
            s3 = _mm_loadu_si128((__m128i*)&src[x - i_src + 18]);
            s4 = _mm_loadu_si128((__m128i*)&src[x + 16]);
            s5 = _mm_loadu_si128((__m128i*)&src[x + i_src + 14]);
            s0 = _mm_and_si128(s0, mask_uv);
            s1 = _mm_and_si128(s1, mask_uv);
            s2 = _mm_and_si128(s2, mask_uv);
            s3 = _mm_and_si128(s3, mask_uv);
            s4 = _mm_and_si128(s4, mask_uv);
            s5 = _mm_and_si128(s5, mask_uv);
            s0 = _mm_packus_epi16(s0, s3);
            s1 = _mm_packus_epi16(s1, s4);
            s2 = _mm_packus_epi16(s2, s5);

            t3 = _mm_min_epu8(s0, s1);
            t1 = _mm_cmpeq_epi8(t3, s0);
            t2 = _mm_cmpeq_epi8(t3, s1);
            t0 = _mm_subs_epi8(t2, t1); //upsign

            t3 = _mm_min_epu8(s1, s2);
            t1 = _mm_cmpeq_epi8(t3, s1);
            t2 = _mm_cmpeq_epi8(t3, s2);
            t3 = _mm_subs_epi8(t1, t2); //downsign

            etype = _mm_adds_epi8(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi8(etype, c0);
            t1 = _mm_cmpeq_epi8(etype, c1);
            t2 = _mm_cmpeq_epi8(etype, c2);
            t3 = _mm_cmpeq_epi8(etype, c3);
            t4 = _mm_cmpeq_epi8(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi8(t0, t1);
            t2 = _mm_adds_epi8(t2, t3);
            t0 = _mm_adds_epi8(t0, t4);
            t0 = _mm_adds_epi8(t0, t2);//get offset

                                       //add 8 nums once for possible overflow
            t1 = _mm_cvtepi8_epi16(t0);
            t0 = _mm_srli_si128(t0, 8);
            t2 = _mm_cvtepi8_epi16(t0);
            t3 = _mm_unpacklo_epi8(s1, zero);
            t4 = _mm_unpackhi_epi8(s1, zero);

            t1 = _mm_adds_epi16(t1, t3);
            t2 = _mm_adds_epi16(t2, t4);
            t0 = _mm_packus_epi16(t1, t2); //saturated
            t1 = _mm_unpacklo_epi8(t0, zero);
            t2 = _mm_unpackhi_epi8(t0, zero);
            if (x != end_x_r0_16) {
                _mm_maskmoveu_si128(t1, mask_uv, (s8*)(dst + x));
                _mm_maskmoveu_si128(t2, mask_uv, (s8*)(dst + x + 16));
            }
            else {
                mask_r0 = _mm_load_si128((__m128i*)(uavs3d_simd_mask[((end_x_r0 - end_x_r0_16)>>1) - 1]));
                mask1 = _mm_unpacklo_epi8(mask_r0, zero);
                mask2 = _mm_unpackhi_epi8(mask_r0, zero);
                _mm_maskmoveu_si128(t1, mask1, (s8*)(dst + x));
                _mm_maskmoveu_si128(t2, mask2, (s8*)(dst + x + 16));
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_r = _mm_load_si128((__m128i*)(uavs3d_simd_mask[((end_x_r - end_x_r_16)>>1) - 1]));
        mask1 = _mm_unpacklo_epi8(mask_r, zero);
        mask2 = _mm_unpackhi_epi8(mask_r, zero);
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 32) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - i_src + 2]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + i_src - 2]);
                s3 = _mm_loadu_si128((__m128i*)&src[x - i_src + 18]);
                s4 = _mm_loadu_si128((__m128i*)&src[x + 16]);
                s5 = _mm_loadu_si128((__m128i*)&src[x + i_src + 14]);
                s0 = _mm_and_si128(s0, mask_uv);
                s1 = _mm_and_si128(s1, mask_uv);
                s2 = _mm_and_si128(s2, mask_uv);
                s3 = _mm_and_si128(s3, mask_uv);
                s4 = _mm_and_si128(s4, mask_uv);
                s5 = _mm_and_si128(s5, mask_uv);
                s0 = _mm_packus_epi16(s0, s3);
                s1 = _mm_packus_epi16(s1, s4);
                s2 = _mm_packus_epi16(s2, s5);

                t3 = _mm_min_epu8(s0, s1);
                t1 = _mm_cmpeq_epi8(t3, s0);
                t2 = _mm_cmpeq_epi8(t3, s1);
                t0 = _mm_subs_epi8(t2, t1); //upsign

                t3 = _mm_min_epu8(s1, s2);
                t1 = _mm_cmpeq_epi8(t3, s1);
                t2 = _mm_cmpeq_epi8(t3, s2);
                t3 = _mm_subs_epi8(t1, t2); //downsign

                etype = _mm_adds_epi8(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi8(etype, c0);
                t1 = _mm_cmpeq_epi8(etype, c1);
                t2 = _mm_cmpeq_epi8(etype, c2);
                t3 = _mm_cmpeq_epi8(etype, c3);
                t4 = _mm_cmpeq_epi8(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi8(t0, t1);
                t2 = _mm_adds_epi8(t2, t3);
                t0 = _mm_adds_epi8(t0, t4);
                t0 = _mm_adds_epi8(t0, t2);//get offset

                                           //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(s1, zero);
                t4 = _mm_unpackhi_epi8(s1, zero);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                t0 = _mm_packus_epi16(t1, t2); //saturated
                t1 = _mm_unpacklo_epi8(t0, zero);
                t2 = _mm_unpackhi_epi8(t0, zero);
                if (x != end_x_r_16) {
                    _mm_maskmoveu_si128(t1, mask_uv, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask_uv, (s8*)(dst + x + 16));
                }
                else {
                    _mm_maskmoveu_si128(t1, mask1, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask2, (s8*)(dst + x + 16));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        for (x = start_x_rn; x < end_x_rn; x += 32) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src + 2]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src - 2]);
            s3 = _mm_loadu_si128((__m128i*)&src[x - i_src + 18]);
            s4 = _mm_loadu_si128((__m128i*)&src[x + 16]);
            s5 = _mm_loadu_si128((__m128i*)&src[x + i_src + 14]);
            s0 = _mm_and_si128(s0, mask_uv);
            s1 = _mm_and_si128(s1, mask_uv);
            s2 = _mm_and_si128(s2, mask_uv);
            s3 = _mm_and_si128(s3, mask_uv);
            s4 = _mm_and_si128(s4, mask_uv);
            s5 = _mm_and_si128(s5, mask_uv);
            s0 = _mm_packus_epi16(s0, s3);
            s1 = _mm_packus_epi16(s1, s4);
            s2 = _mm_packus_epi16(s2, s5);

            t3 = _mm_min_epu8(s0, s1);
            t1 = _mm_cmpeq_epi8(t3, s0);
            t2 = _mm_cmpeq_epi8(t3, s1);
            t0 = _mm_subs_epi8(t2, t1); //upsign

            t3 = _mm_min_epu8(s1, s2);
            t1 = _mm_cmpeq_epi8(t3, s1);
            t2 = _mm_cmpeq_epi8(t3, s2);
            t3 = _mm_subs_epi8(t1, t2); //downsign

            etype = _mm_adds_epi8(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi8(etype, c0);
            t1 = _mm_cmpeq_epi8(etype, c1);
            t2 = _mm_cmpeq_epi8(etype, c2);
            t3 = _mm_cmpeq_epi8(etype, c3);
            t4 = _mm_cmpeq_epi8(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi8(t0, t1);
            t2 = _mm_adds_epi8(t2, t3);
            t0 = _mm_adds_epi8(t0, t4);
            t0 = _mm_adds_epi8(t0, t2);//get offset

                                       //add 8 nums once for possible overflow
            t1 = _mm_cvtepi8_epi16(t0);
            t0 = _mm_srli_si128(t0, 8);
            t2 = _mm_cvtepi8_epi16(t0);
            t3 = _mm_unpacklo_epi8(s1, zero);
            t4 = _mm_unpackhi_epi8(s1, zero);

            t1 = _mm_adds_epi16(t1, t3);
            t2 = _mm_adds_epi16(t2, t4);
            t0 = _mm_packus_epi16(t1, t2); //saturated
            t1 = _mm_unpacklo_epi8(t0, zero);
            t2 = _mm_unpackhi_epi8(t0, zero);
            if (x != end_x_rn_16) {
                _mm_maskmoveu_si128(t1, mask_uv, (s8*)(dst + x));
                _mm_maskmoveu_si128(t2, mask_uv, (s8*)(dst + x + 16));
            }
            else {
                mask_rn = _mm_load_si128((__m128i*)(uavs3d_simd_mask[((end_x_rn - end_x_rn_16)>>1) - 1]));
                mask1 = _mm_unpacklo_epi8(mask_rn, zero); 
                mask2 = _mm_unpackhi_epi8(mask_rn, zero);
                _mm_maskmoveu_si128(t1, mask1, (s8*)(dst + x));
                _mm_maskmoveu_si128(t2, mask2, (s8*)(dst + x + 16));
                break;
            }
        }
    }
    break;
    case SAO_TYPE_BO: {
        __m128i r0, r1, r2, r3, off0, off1, off2, off3;
        __m128i t0, t1, t2, t3, t4, src0, src1, src2;
        __m128i mask = _mm_load_si128((const __m128i*)uavs3d_simd_mask[((smb_pix_width>>1) & 15) - 1]);
        __m128i mask1, mask2;
        int end_x_16 = smb_pix_width - 31;

        r0 = _mm_set1_epi8(saoBlkParam->bandIdx[0]);
        r1 = _mm_set1_epi8(saoBlkParam->bandIdx[1]);
        r2 = _mm_set1_epi8(saoBlkParam->bandIdx[2]);
        r3 = _mm_set1_epi8(saoBlkParam->bandIdx[3]);
        off0 = _mm_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm_set1_epi8(saoBlkParam->offset[3]);

        mask1 = _mm_unpacklo_epi8(mask, zero);
        mask2 = _mm_unpackhi_epi8(mask, zero);

        for (y = 0; y < smb_pix_height; y++) {
            for (x = 0; x < smb_pix_width; x += 32) {
                src0 = _mm_loadu_si128((__m128i*)&src[x]);
                src2 = _mm_loadu_si128((__m128i*)&src[x + 16]);
                t0 = _mm_and_si128(src0, mask_uv);
                t2 = _mm_and_si128(src2, mask_uv);
                src0 = _mm_packus_epi16(t0, t2);
                t0 = _mm_srli_epi16(t0, 3);
                t2 = _mm_srli_epi16(t2, 3);
                src1 = _mm_packus_epi16(t0, t2);

                t0 = _mm_cmpeq_epi8(src1, r0);
                t1 = _mm_cmpeq_epi8(src1, r1);
                t2 = _mm_cmpeq_epi8(src1, r2);
                t3 = _mm_cmpeq_epi8(src1, r3);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t0 = _mm_or_si128(t0, t1);
                t2 = _mm_or_si128(t2, t3);
                t0 = _mm_or_si128(t0, t2);
                //src0 = _mm_adds_epi8(src0, t0);
                //add 8 nums once for possible overflow
                t1 = _mm_cvtepi8_epi16(t0);
                t0 = _mm_srli_si128(t0, 8);
                t2 = _mm_cvtepi8_epi16(t0);
                t3 = _mm_unpacklo_epi8(src0, zero);
                t4 = _mm_unpackhi_epi8(src0, zero);

                t1 = _mm_adds_epi16(t1, t3);
                t2 = _mm_adds_epi16(t2, t4);
                src0 = _mm_packus_epi16(t1, t2); //saturated
                t1 = _mm_unpacklo_epi8(src0, zero);
                t2 = _mm_unpackhi_epi8(src0, zero);
                if (x < end_x_16) {
                    _mm_maskmoveu_si128(t1, mask_uv, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask_uv, (s8*)(dst + x + 16));
                }
                else {
                    _mm_maskmoveu_si128(t1, mask1, (s8*)(dst + x));
                    _mm_maskmoveu_si128(t2, mask2, (s8*)(dst + x + 16));
                }
            }

            dst += i_dst;
            src += i_src;
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

#else

void uavs3d_sao_on_lcu_sse(pel *src, int i_src, pel *dst, int i_dst, com_sao_param_t *sao_params, int smb_pix_height,
    int smb_pix_width, int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int sample_bit_depth)
{
    int type;
    int start_x, end_x, start_y, end_y;
    int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
    int x, y;
    int max_pixel = (1 << sample_bit_depth) - 1;
    __m128i off0, off1, off2, off3, off4;
    __m128i s0, s1, s2;
    __m128i t0, t1, t2, t3, t4, etype;
    __m128i c0, c1, c2, c3, c4;
    __m128i mask;
    __m128i min_val = _mm_setzero_si128();
    __m128i max_val = _mm_set1_epi16(max_pixel);

    type = sao_params->type;

    switch (type) {
    case SAO_TYPE_EO_0: {
        int end_x_8;
        c0 = _mm_set1_epi16(-2);
        c1 = _mm_set1_epi16(-1);
        c2 = _mm_set1_epi16(0);
        c3 = _mm_set1_epi16(1);
        c4 = _mm_set1_epi16(2);

        off0 = _mm_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm_set1_epi16((pel)sao_params->offset[4]);
        start_x = smb_available_left ? 0 : 1;
        end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        end_x_8 = end_x - ((end_x - start_x) & 0x07);

        mask = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x - end_x_8 - 1]));
        if (smb_pix_width == 4) {

            for (y = 0; y < smb_pix_height; y++) {
                //diff = src[start_x] - src[start_x - 1];
                //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                s0 = _mm_loadu_si128((__m128i*)&src[start_x - 1]);
                s1 = _mm_srli_si128(s0, 2);
                s2 = _mm_srli_si128(s0, 4);

                t3 = _mm_min_epu16(s0, s1);
                t1 = _mm_cmpeq_epi16(t3, s0);
                t2 = _mm_cmpeq_epi16(t3, s1);
                t0 = _mm_subs_epi16(t2, t1); //leftsign

                t3 = _mm_min_epu16(s1, s2);
                t1 = _mm_cmpeq_epi16(t3, s1);
                t2 = _mm_cmpeq_epi16(t3, s2);
                t3 = _mm_subs_epi16(t1, t2); //rightsign

                etype = _mm_adds_epi16(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi16(etype, c0);
                t1 = _mm_cmpeq_epi16(etype, c1);
                t2 = _mm_cmpeq_epi16(etype, c2);
                t3 = _mm_cmpeq_epi16(etype, c3);
                t4 = _mm_cmpeq_epi16(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi16(t0, t1);
                t2 = _mm_adds_epi16(t2, t3);
                t0 = _mm_adds_epi16(t0, t4);
                t0 = _mm_adds_epi16(t0, t2);//get offset

                t1 = _mm_adds_epi16(t0, s1);
                t1 = _mm_min_epi16(t1, max_val);
                t1 = _mm_max_epi16(t1, min_val);
                _mm_maskmoveu_si128(t1, mask, (s8*)(dst));

                dst += i_dst;
                src += i_src;
            }
        }
        else {

            for (y = 0; y < smb_pix_height; y++) {
                //diff = src[start_x] - src[start_x - 1];
                //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
                for (x = start_x; x < end_x; x += 8) {
                    s0 = _mm_loadu_si128((__m128i*)&src[x - 1]);
                    s1 = _mm_loadu_si128((__m128i*)&src[x]);
                    s2 = _mm_loadu_si128((__m128i*)&src[x + 1]);

                    t3 = _mm_min_epu16(s0, s1);
                    t1 = _mm_cmpeq_epi16(t3, s0);
                    t2 = _mm_cmpeq_epi16(t3, s1);
                    t0 = _mm_subs_epi16(t2, t1); //leftsign

                    t3 = _mm_min_epu16(s1, s2);
                    t1 = _mm_cmpeq_epi16(t3, s1);
                    t2 = _mm_cmpeq_epi16(t3, s2);
                    t3 = _mm_subs_epi16(t1, t2); //rightsign

                    etype = _mm_adds_epi16(t0, t3); //edgetype

                    t0 = _mm_cmpeq_epi16(etype, c0);
                    t1 = _mm_cmpeq_epi16(etype, c1);
                    t2 = _mm_cmpeq_epi16(etype, c2);
                    t3 = _mm_cmpeq_epi16(etype, c3);
                    t4 = _mm_cmpeq_epi16(etype, c4);

                    t0 = _mm_and_si128(t0, off0);
                    t1 = _mm_and_si128(t1, off1);
                    t2 = _mm_and_si128(t2, off2);
                    t3 = _mm_and_si128(t3, off3);
                    t4 = _mm_and_si128(t4, off4);

                    t0 = _mm_adds_epi16(t0, t1);
                    t2 = _mm_adds_epi16(t2, t3);
                    t0 = _mm_adds_epi16(t0, t4);
                    t0 = _mm_adds_epi16(t0, t2);//get offset

                    t1 = _mm_adds_epi16(t0, s1);
                    t1 = _mm_min_epi16(t1, max_val);
                    t1 = _mm_max_epi16(t1, min_val);

                    if (x != end_x_8) {
                        _mm_storeu_si128((__m128i*)(dst + x), t1);
                    }
                    else {
                        _mm_maskmoveu_si128(t1, mask, (s8*)(dst + x));
                        break;
                    }
                }
                dst += i_dst;
                src += i_src;
            }
        }
    }
                        break;
    case SAO_TYPE_EO_90: {
        int end_x_8 = smb_pix_width - 7;
        c0 = _mm_set1_epi16(-2);
        c1 = _mm_set1_epi16(-1);
        c2 = _mm_set1_epi16(0);
        c3 = _mm_set1_epi16(1);
        c4 = _mm_set1_epi16(2);

        off0 = _mm_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm_set1_epi16((pel)sao_params->offset[4]);
        start_y = smb_available_up ? 0 : 1;
        end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1);

        dst += start_y * i_dst;
        src += start_y * i_src;

        if (smb_pix_width == 4) {
            mask = _mm_set_epi32(0, 0, -1, -1);

            for (y = start_y; y < end_y; y++) {
                s0 = _mm_loadu_si128((__m128i*)(src - i_src));
                s1 = _mm_loadu_si128((__m128i*)src);
                s2 = _mm_loadu_si128((__m128i*)(src + i_src));

                t3 = _mm_min_epu16(s0, s1);
                t1 = _mm_cmpeq_epi16(t3, s0);
                t2 = _mm_cmpeq_epi16(t3, s1);
                t0 = _mm_subs_epi16(t2, t1); //upsign

                t3 = _mm_min_epu16(s1, s2);
                t1 = _mm_cmpeq_epi16(t3, s1);
                t2 = _mm_cmpeq_epi16(t3, s2);
                t3 = _mm_subs_epi16(t1, t2); //downsign

                etype = _mm_adds_epi16(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi16(etype, c0);
                t1 = _mm_cmpeq_epi16(etype, c1);
                t2 = _mm_cmpeq_epi16(etype, c2);
                t3 = _mm_cmpeq_epi16(etype, c3);
                t4 = _mm_cmpeq_epi16(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi16(t0, t1);
                t2 = _mm_adds_epi16(t2, t3);
                t0 = _mm_adds_epi16(t0, t4);
                t0 = _mm_adds_epi16(t0, t2);//get offset

                                            //add 8 nums once for possible overflow
                t1 = _mm_adds_epi16(t0, s1);
                t1 = _mm_min_epi16(t1, max_val);
                t1 = _mm_max_epi16(t1, min_val);

                _mm_maskmoveu_si128(t1, mask, (s8*)(dst));

                dst += i_dst;
                src += i_src;
            }
        }
        else {
            if (smb_pix_width & 0x07) {
                mask = _mm_set_epi32(0, 0, -1, -1);

                for (y = start_y; y < end_y; y++) {
                    for (x = 0; x < smb_pix_width; x += 8) {
                        s0 = _mm_loadu_si128((__m128i*)&src[x - i_src]);
                        s1 = _mm_loadu_si128((__m128i*)&src[x]);
                        s2 = _mm_loadu_si128((__m128i*)&src[x + i_src]);

                        t3 = _mm_min_epu16(s0, s1);
                        t1 = _mm_cmpeq_epi16(t3, s0);
                        t2 = _mm_cmpeq_epi16(t3, s1);
                        t0 = _mm_subs_epi16(t2, t1); //upsign

                        t3 = _mm_min_epu16(s1, s2);
                        t1 = _mm_cmpeq_epi16(t3, s1);
                        t2 = _mm_cmpeq_epi16(t3, s2);
                        t3 = _mm_subs_epi16(t1, t2); //downsign

                        etype = _mm_adds_epi16(t0, t3); //edgetype

                        t0 = _mm_cmpeq_epi16(etype, c0);
                        t1 = _mm_cmpeq_epi16(etype, c1);
                        t2 = _mm_cmpeq_epi16(etype, c2);
                        t3 = _mm_cmpeq_epi16(etype, c3);
                        t4 = _mm_cmpeq_epi16(etype, c4);

                        t0 = _mm_and_si128(t0, off0);
                        t1 = _mm_and_si128(t1, off1);
                        t2 = _mm_and_si128(t2, off2);
                        t3 = _mm_and_si128(t3, off3);
                        t4 = _mm_and_si128(t4, off4);

                        t0 = _mm_adds_epi16(t0, t1);
                        t2 = _mm_adds_epi16(t2, t3);
                        t0 = _mm_adds_epi16(t0, t4);
                        t0 = _mm_adds_epi16(t0, t2);//get offset

                        t1 = _mm_adds_epi16(t0, s1);
                        t1 = _mm_min_epi16(t1, max_val);
                        t1 = _mm_max_epi16(t1, min_val);

                        if (x < end_x_8) {
                            _mm_storeu_si128((__m128i*)(dst + x), t1);
                        }
                        else {
                            _mm_maskmoveu_si128(t1, mask, (s8*)(dst + x));
                            break;
                        }
                    }
                    dst += i_dst;
                    src += i_src;
                }
            }
            else {
                for (y = start_y; y < end_y; y++) {
                    for (x = 0; x < smb_pix_width; x += 8) {
                        s0 = _mm_loadu_si128((__m128i*)&src[x - i_src]);
                        s1 = _mm_loadu_si128((__m128i*)&src[x]);
                        s2 = _mm_loadu_si128((__m128i*)&src[x + i_src]);

                        t3 = _mm_min_epu16(s0, s1);
                        t1 = _mm_cmpeq_epi16(t3, s0);
                        t2 = _mm_cmpeq_epi16(t3, s1);
                        t0 = _mm_subs_epi16(t2, t1); //upsign

                        t3 = _mm_min_epu16(s1, s2);
                        t1 = _mm_cmpeq_epi16(t3, s1);
                        t2 = _mm_cmpeq_epi16(t3, s2);
                        t3 = _mm_subs_epi16(t1, t2); //downsign

                        etype = _mm_adds_epi16(t0, t3); //edgetype

                        t0 = _mm_cmpeq_epi16(etype, c0);
                        t1 = _mm_cmpeq_epi16(etype, c1);
                        t2 = _mm_cmpeq_epi16(etype, c2);
                        t3 = _mm_cmpeq_epi16(etype, c3);
                        t4 = _mm_cmpeq_epi16(etype, c4);

                        t0 = _mm_and_si128(t0, off0);
                        t1 = _mm_and_si128(t1, off1);
                        t2 = _mm_and_si128(t2, off2);
                        t3 = _mm_and_si128(t3, off3);
                        t4 = _mm_and_si128(t4, off4);

                        t0 = _mm_adds_epi16(t0, t1);
                        t2 = _mm_adds_epi16(t2, t3);
                        t0 = _mm_adds_epi16(t0, t4);
                        t0 = _mm_adds_epi16(t0, t2);//get offset

                        t1 = _mm_adds_epi16(t0, s1);
                        t1 = _mm_min_epi16(t1, max_val);
                        t1 = _mm_max_epi16(t1, min_val);

                        _mm_storeu_si128((__m128i*)(dst + x), t1);
                    }
                    dst += i_dst;
                    src += i_src;
                }
            }
        }
    }
                         break;
    case SAO_TYPE_EO_135: {
        __m128i mask_r0, mask_r, mask_rn;
        int end_x_r0_8, end_x_r_8, end_x_rn_8;

        c0 = _mm_set1_epi16(-2);
        c1 = _mm_set1_epi16(-1);
        c2 = _mm_set1_epi16(0);
        c3 = _mm_set1_epi16(1);
        c4 = _mm_set1_epi16(2);

        off0 = _mm_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm_set1_epi16((pel)sao_params->offset[4]);

        start_x_r0 = (smb_available_up && smb_available_left) ? 0 : 1;
        end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;
        start_x_r = smb_available_left ? 0 : 1;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        start_x_rn = smb_available_down ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
        end_x_rn = (smb_available_right && smb_available_down) ? smb_pix_width : (smb_pix_width - 1);

        end_x_r0_8 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x07);
        end_x_r_8 = end_x_r - ((end_x_r - start_x_r) & 0x07);
        end_x_rn_8 = end_x_rn - ((end_x_rn - start_x_rn) & 0x07);


        //first row
        for (x = start_x_r0; x < end_x_r0; x += 8) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src - 1]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src + 1]);

            t3 = _mm_min_epu16(s0, s1);
            t1 = _mm_cmpeq_epi16(t3, s0);
            t2 = _mm_cmpeq_epi16(t3, s1);
            t0 = _mm_subs_epi16(t2, t1); //upsign

            t3 = _mm_min_epu16(s1, s2);
            t1 = _mm_cmpeq_epi16(t3, s1);
            t2 = _mm_cmpeq_epi16(t3, s2);
            t3 = _mm_subs_epi16(t1, t2); //downsign

            etype = _mm_adds_epi16(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi16(etype, c0);
            t1 = _mm_cmpeq_epi16(etype, c1);
            t2 = _mm_cmpeq_epi16(etype, c2);
            t3 = _mm_cmpeq_epi16(etype, c3);
            t4 = _mm_cmpeq_epi16(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi16(t0, t1);
            t2 = _mm_adds_epi16(t2, t3);
            t0 = _mm_adds_epi16(t0, t4);
            t0 = _mm_adds_epi16(t0, t2);//get offset


            t1 = _mm_adds_epi16(t0, s1);
            t1 = _mm_min_epi16(t1, max_val);
            t1 = _mm_max_epi16(t1, min_val);

            if (x != end_x_r0_8) {
                _mm_storeu_si128((__m128i*)(dst + x), t1);
            }
            else {
                mask_r0 = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_r0 - end_x_r0_8 - 1]));
                _mm_maskmoveu_si128(t1, mask_r0, (s8*)(dst + x));
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_r = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_r - end_x_r_8 - 1]));
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 8) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - i_src - 1]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + i_src + 1]);

                t3 = _mm_min_epu16(s0, s1);
                t1 = _mm_cmpeq_epi16(t3, s0);
                t2 = _mm_cmpeq_epi16(t3, s1);
                t0 = _mm_subs_epi16(t2, t1); //upsign

                t3 = _mm_min_epu16(s1, s2);
                t1 = _mm_cmpeq_epi16(t3, s1);
                t2 = _mm_cmpeq_epi16(t3, s2);
                t3 = _mm_subs_epi16(t1, t2); //downsign

                etype = _mm_adds_epi16(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi16(etype, c0);
                t1 = _mm_cmpeq_epi16(etype, c1);
                t2 = _mm_cmpeq_epi16(etype, c2);
                t3 = _mm_cmpeq_epi16(etype, c3);
                t4 = _mm_cmpeq_epi16(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi16(t0, t1);
                t2 = _mm_adds_epi16(t2, t3);
                t0 = _mm_adds_epi16(t0, t4);
                t0 = _mm_adds_epi16(t0, t2);//get offset

                t1 = _mm_adds_epi16(t0, s1);
                t1 = _mm_min_epi16(t1, max_val);
                t1 = _mm_max_epi16(t1, min_val);

                if (x != end_x_r_8) {
                    _mm_storeu_si128((__m128i*)(dst + x), t1);
                }
                else {
                    _mm_maskmoveu_si128(t1, mask_r, (s8*)(dst + x));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        //last row
        for (x = start_x_rn; x < end_x_rn; x += 8) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src - 1]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src + 1]);

            t3 = _mm_min_epu16(s0, s1);
            t1 = _mm_cmpeq_epi16(t3, s0);
            t2 = _mm_cmpeq_epi16(t3, s1);
            t0 = _mm_subs_epi16(t2, t1); //upsign

            t3 = _mm_min_epu16(s1, s2);
            t1 = _mm_cmpeq_epi16(t3, s1);
            t2 = _mm_cmpeq_epi16(t3, s2);
            t3 = _mm_subs_epi16(t1, t2); //downsign

            etype = _mm_adds_epi16(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi16(etype, c0);
            t1 = _mm_cmpeq_epi16(etype, c1);
            t2 = _mm_cmpeq_epi16(etype, c2);
            t3 = _mm_cmpeq_epi16(etype, c3);
            t4 = _mm_cmpeq_epi16(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi16(t0, t1);
            t2 = _mm_adds_epi16(t2, t3);
            t0 = _mm_adds_epi16(t0, t4);
            t0 = _mm_adds_epi16(t0, t2);//get offset

            t1 = _mm_adds_epi16(t0, s1);
            t1 = _mm_min_epi16(t1, max_val);
            t1 = _mm_max_epi16(t1, min_val);

            if (x != end_x_rn_8) {
                _mm_storeu_si128((__m128i*)(dst + x), t1);
            }
            else {
                mask_rn = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_rn - end_x_rn_8 - 1]));
                _mm_maskmoveu_si128(t1, mask_rn, (s8*)(dst + x));
                break;
            }
        }
    }
                          break;
    case SAO_TYPE_EO_45: {
        __m128i mask_r0, mask_r, mask_rn;
        int end_x_r0_8, end_x_r_8, end_x_rn_8;

        c0 = _mm_set1_epi16(-2);
        c1 = _mm_set1_epi16(-1);
        c2 = _mm_set1_epi16(0);
        c3 = _mm_set1_epi16(1);
        c4 = _mm_set1_epi16(2);

        off0 = _mm_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm_set1_epi16((pel)sao_params->offset[4]);

        start_x_r0 = smb_available_up ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
        end_x_r0 = (smb_available_up && smb_available_right) ? smb_pix_width : (smb_pix_width - 1);
        start_x_r = smb_available_left ? 0 : 1;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        start_x_rn = (smb_available_left && smb_available_down) ? 0 : 1;
        end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;

        end_x_r0_8 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x07);
        end_x_r_8 = end_x_r - ((end_x_r - start_x_r) & 0x07);
        end_x_rn_8 = end_x_rn - ((end_x_rn - start_x_rn) & 0x07);


        //first row
        for (x = start_x_r0; x < end_x_r0; x += 8) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src + 1]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src - 1]);

            t3 = _mm_min_epu16(s0, s1);
            t1 = _mm_cmpeq_epi16(t3, s0);
            t2 = _mm_cmpeq_epi16(t3, s1);
            t0 = _mm_subs_epi16(t2, t1); //upsign

            t3 = _mm_min_epu16(s1, s2);
            t1 = _mm_cmpeq_epi16(t3, s1);
            t2 = _mm_cmpeq_epi16(t3, s2);
            t3 = _mm_subs_epi16(t1, t2); //downsign

            etype = _mm_adds_epi16(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi16(etype, c0);
            t1 = _mm_cmpeq_epi16(etype, c1);
            t2 = _mm_cmpeq_epi16(etype, c2);
            t3 = _mm_cmpeq_epi16(etype, c3);
            t4 = _mm_cmpeq_epi16(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi16(t0, t1);
            t2 = _mm_adds_epi16(t2, t3);
            t0 = _mm_adds_epi16(t0, t4);
            t0 = _mm_adds_epi16(t0, t2);//get offset

            t1 = _mm_adds_epi16(t0, s1);
            t1 = _mm_min_epi16(t1, max_val);
            t1 = _mm_max_epi16(t1, min_val);

            if (x != end_x_r0_8) {
                _mm_storeu_si128((__m128i*)(dst + x), t1);
            }
            else {
                mask_r0 = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_r0 - end_x_r0_8 - 1]));
                _mm_maskmoveu_si128(t1, mask_r0, (s8*)(dst + x));
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_r = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_r - end_x_r_8 - 1]));
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 8) {
                s0 = _mm_loadu_si128((__m128i*)&src[x - i_src + 1]);
                s1 = _mm_loadu_si128((__m128i*)&src[x]);
                s2 = _mm_loadu_si128((__m128i*)&src[x + i_src - 1]);

                t3 = _mm_min_epu16(s0, s1);
                t1 = _mm_cmpeq_epi16(t3, s0);
                t2 = _mm_cmpeq_epi16(t3, s1);
                t0 = _mm_subs_epi16(t2, t1); //upsign

                t3 = _mm_min_epu16(s1, s2);
                t1 = _mm_cmpeq_epi16(t3, s1);
                t2 = _mm_cmpeq_epi16(t3, s2);
                t3 = _mm_subs_epi16(t1, t2); //downsign

                etype = _mm_adds_epi16(t0, t3); //edgetype

                t0 = _mm_cmpeq_epi16(etype, c0);
                t1 = _mm_cmpeq_epi16(etype, c1);
                t2 = _mm_cmpeq_epi16(etype, c2);
                t3 = _mm_cmpeq_epi16(etype, c3);
                t4 = _mm_cmpeq_epi16(etype, c4);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t4 = _mm_and_si128(t4, off4);

                t0 = _mm_adds_epi16(t0, t1);
                t2 = _mm_adds_epi16(t2, t3);
                t0 = _mm_adds_epi16(t0, t4);
                t0 = _mm_adds_epi16(t0, t2);//get offset

                t1 = _mm_adds_epi16(t0, s1);
                t1 = _mm_min_epi16(t1, max_val);
                t1 = _mm_max_epi16(t1, min_val);

                if (x != end_x_r_8) {
                    _mm_storeu_si128((__m128i*)(dst + x), t1);
                }
                else {
                    _mm_maskmoveu_si128(t1, mask_r, (s8*)(dst + x));
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        for (x = start_x_rn; x < end_x_rn; x += 8) {
            s0 = _mm_loadu_si128((__m128i*)&src[x - i_src + 1]);
            s1 = _mm_loadu_si128((__m128i*)&src[x]);
            s2 = _mm_loadu_si128((__m128i*)&src[x + i_src - 1]);

            t3 = _mm_min_epu16(s0, s1);
            t1 = _mm_cmpeq_epi16(t3, s0);
            t2 = _mm_cmpeq_epi16(t3, s1);
            t0 = _mm_subs_epi16(t2, t1); //upsign

            t3 = _mm_min_epu16(s1, s2);
            t1 = _mm_cmpeq_epi16(t3, s1);
            t2 = _mm_cmpeq_epi16(t3, s2);
            t3 = _mm_subs_epi16(t1, t2); //downsign

            etype = _mm_adds_epi16(t0, t3); //edgetype

            t0 = _mm_cmpeq_epi16(etype, c0);
            t1 = _mm_cmpeq_epi16(etype, c1);
            t2 = _mm_cmpeq_epi16(etype, c2);
            t3 = _mm_cmpeq_epi16(etype, c3);
            t4 = _mm_cmpeq_epi16(etype, c4);

            t0 = _mm_and_si128(t0, off0);
            t1 = _mm_and_si128(t1, off1);
            t2 = _mm_and_si128(t2, off2);
            t3 = _mm_and_si128(t3, off3);
            t4 = _mm_and_si128(t4, off4);

            t0 = _mm_adds_epi16(t0, t1);
            t2 = _mm_adds_epi16(t2, t3);
            t0 = _mm_adds_epi16(t0, t4);
            t0 = _mm_adds_epi16(t0, t2);//get offset

            t1 = _mm_adds_epi16(t0, s1);
            t1 = _mm_min_epi16(t1, max_val);
            t1 = _mm_max_epi16(t1, min_val);

            if (x != end_x_rn_8) {
                _mm_storeu_si128((__m128i*)(dst + x), t1);
            }
            else {
                mask_rn = _mm_load_si128((__m128i*)(uavs3d_simd_mask[end_x_rn - end_x_rn_8 - 1]));
                _mm_maskmoveu_si128(t1, mask_rn, (s8*)(dst + x));
                break;
            }
        }
    }
                         break;
    case SAO_TYPE_BO: {
        __m128i r0, r1, r2, r3;
        int shift_bo = sample_bit_depth - NUM_SAO_BO_CLASSES_IN_BIT;
        int end_x_8 = smb_pix_width - 7;

        r0 = _mm_set1_epi16(sao_params->bandIdx[0]);
        r1 = _mm_set1_epi16(sao_params->bandIdx[1]);
        r2 = _mm_set1_epi16(sao_params->bandIdx[2]);
        r3 = _mm_set1_epi16(sao_params->bandIdx[3]);
        off0 = _mm_set1_epi16(sao_params->offset[0]);
        off1 = _mm_set1_epi16(sao_params->offset[1]);
        off2 = _mm_set1_epi16(sao_params->offset[2]);
        off3 = _mm_set1_epi16(sao_params->offset[3]);

        if (smb_pix_width == 4) {
            mask = _mm_set_epi32(0, 0, -1, -1);

            for (y = 0; y < smb_pix_height; y++) {
                s0 = _mm_loadu_si128((__m128i*)src);

                s1 = _mm_srai_epi16(s0, shift_bo);

                t0 = _mm_cmpeq_epi16(s1, r0);
                t1 = _mm_cmpeq_epi16(s1, r1);
                t2 = _mm_cmpeq_epi16(s1, r2);
                t3 = _mm_cmpeq_epi16(s1, r3);

                t0 = _mm_and_si128(t0, off0);
                t1 = _mm_and_si128(t1, off1);
                t2 = _mm_and_si128(t2, off2);
                t3 = _mm_and_si128(t3, off3);
                t0 = _mm_or_si128(t0, t1);
                t2 = _mm_or_si128(t2, t3);
                t0 = _mm_or_si128(t0, t2);

                t1 = _mm_adds_epi16(s0, t0);
                t1 = _mm_min_epi16(t1, max_val);
                t1 = _mm_max_epi16(t1, min_val);

                _mm_maskmoveu_si128(t1, mask, (s8*)(dst));

                dst += i_dst;
                src += i_src;
            }
        }
        else {
            mask = _mm_load_si128((const __m128i*)uavs3d_simd_mask[(smb_pix_width & 7) - 1]);

            for (y = 0; y < smb_pix_height; y++) {
                for (x = 0; x < smb_pix_width; x += 8) {
                    s0 = _mm_loadu_si128((__m128i*)&src[x]);

                    s1 = _mm_srai_epi16(s0, shift_bo);

                    t0 = _mm_cmpeq_epi16(s1, r0);
                    t1 = _mm_cmpeq_epi16(s1, r1);
                    t2 = _mm_cmpeq_epi16(s1, r2);
                    t3 = _mm_cmpeq_epi16(s1, r3);

                    t0 = _mm_and_si128(t0, off0);
                    t1 = _mm_and_si128(t1, off1);
                    t2 = _mm_and_si128(t2, off2);
                    t3 = _mm_and_si128(t3, off3);
                    t0 = _mm_or_si128(t0, t1);
                    t2 = _mm_or_si128(t2, t3);
                    t0 = _mm_or_si128(t0, t2);
                    //src0 = _mm_adds_epi8(src0, t0);

                    //add 8 nums once for possible overflow
                    t1 = _mm_adds_epi16(s0, t0);
                    t1 = _mm_min_epi16(t1, max_val);
                    t1 = _mm_max_epi16(t1, min_val);

                    if (x < end_x_8) {
                        _mm_storeu_si128((__m128i*)&dst[x], t1);
                    }
                    else {
                        _mm_maskmoveu_si128(t1, mask, (s8*)(dst + x));
                    }

                }

                dst += i_dst;
                src += i_src;
            }
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

#endif
