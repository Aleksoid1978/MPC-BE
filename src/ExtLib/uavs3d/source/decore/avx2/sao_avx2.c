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

#include "avx2.h"


ALIGNED_32(static pel sao_mask_32[16][16]) = {
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0 }
};

#if (BIT_DEPTH == 8)

void uavs3d_sao_on_lcu_avx2(pel *src, int i_src, pel *dst, int i_dst, com_sao_param_t *saoBlkParam, int smb_pix_height, int smb_pix_width,
    int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int sample_bit_depth)
{
    int type;
    int start_x, end_x, start_y, end_y;
    int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
    int x, y;

    int smb_available_upleft = (smb_available_up && smb_available_left);
    int smb_available_upright = (smb_available_up && smb_available_right);
    int smb_available_leftdown = (smb_available_down && smb_available_left);
    int smb_available_rightdown = (smb_available_down && smb_available_right);

    type = saoBlkParam->type;

    switch (type) {
    case SAO_TYPE_EO_0: {
        __m256i off;
        __m256i s0, s1, s2;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask, offtmp;
        __m256i c2 = _mm256_set1_epi8(2);

        int end_x_32;
        int mask_low;

        offtmp = _mm_loadu_si128((__m128i*)saoBlkParam->offset);
        offtmp = _mm_packs_epi32(offtmp, _mm_set_epi32(0, 0, 0, saoBlkParam->offset[4]));
        offtmp = _mm_packs_epi16(offtmp, _mm_setzero_si128());

		off = _mm256_castsi128_si256(offtmp);
		off = _mm256_inserti128_si256(off, offtmp, 1);

        start_x = smb_available_left ? 0 : 1;
        end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        end_x_32 = end_x - ((end_x - start_x) & 0x1f);

        mask = _mm_loadu_si128((__m128i*)(sao_mask_32[(end_x - end_x_32)&0xf]));
        mask_low = end_x - end_x_32 > 16;
        for (y = 0; y < smb_pix_height; y++) {
            //diff = src[start_x] - src[start_x - 1];
            //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = start_x; x < end_x; x += 32) {
				s0 = _mm256_lddqu_si256((__m256i*)&src[x - 1]);
				s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + 1]);

                t3 = _mm256_min_epu8(s0, s1);
                t1 = _mm256_cmpeq_epi8(t3, s0);
                t2 = _mm256_cmpeq_epi8(t3, s1);
                t0 = _mm256_subs_epi8(t2, t1); //leftsign

                t3 = _mm256_min_epu8(s1, s2);
                t1 = _mm256_cmpeq_epi8(t3, s1);
                t2 = _mm256_cmpeq_epi8(t3, s2);
                t3 = _mm256_subs_epi8(t1, t2); //rightsign

                etype = _mm256_adds_epi8(t0, t3); //edgetype

                etype = _mm256_adds_epi8(etype, c2);

                t0 = _mm256_shuffle_epi8(off, etype);//get offset

                //convert byte to short for possible overflow
				t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
                t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
				t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
				t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

                t1 = _mm256_adds_epi16(t1, t3);
                t2 = _mm256_adds_epi16(t2, t4);
                t0 = _mm256_packus_epi16(t1, t2); //saturated
				t0 = _mm256_permute4x64_epi64(t0, 0xd8);

                if (x != end_x_32){
                    _mm256_storeu_si256((__m256i*)(dst + x), t0);
                }
                else{
                    if (mask_low)
                    {
					    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                        _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 16));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
    }
        break;
    case SAO_TYPE_EO_90: {
        __m256i off;
        __m256i s0, s1, s2;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask, offtmp;
        __m256i c2 = _mm256_set1_epi8(2);
        int end_x_32 = smb_pix_width - (smb_pix_width & 0x1f);
        int mask_low;

		offtmp = _mm_loadu_si128((__m128i*)saoBlkParam->offset);
		offtmp = _mm_packs_epi32(offtmp, _mm_set_epi32(0, 0, 0, saoBlkParam->offset[4]));
		offtmp = _mm_packs_epi16(offtmp, _mm_setzero_si128());

		off = _mm256_castsi128_si256(offtmp);
		off = _mm256_inserti128_si256(off, offtmp, 1);

        start_y = smb_available_up ? 0 : 1;
        end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1);

        dst += start_y * i_dst;
        src += start_y * i_src;

        mask = _mm_loadu_si128((__m128i*)(sao_mask_32[smb_pix_width & 0xf]));
        mask_low = (smb_pix_width & 0x1f) > 16;
        for (y = start_y; y < end_y; y++) {
            //diff = src[start_x] - src[start_x - 1];
            //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = 0; x < smb_pix_width; x += 32) {
				s0 = _mm256_lddqu_si256((__m256i*)&src[x - i_src]);
				s1 = _mm256_lddqu_si256((__m256i*)&src[x]);
				s2 = _mm256_lddqu_si256((__m256i*)&src[x + i_src]);

                t3 = _mm256_min_epu8(s0, s1);
                t1 = _mm256_cmpeq_epi8(t3, s0);
                t2 = _mm256_cmpeq_epi8(t3, s1);
                t0 = _mm256_subs_epi8(t2, t1); //leftsign

                t3 = _mm256_min_epu8(s1, s2);
                t1 = _mm256_cmpeq_epi8(t3, s1);
                t2 = _mm256_cmpeq_epi8(t3, s2);
                t3 = _mm256_subs_epi8(t1, t2); //rightsign

                etype = _mm256_adds_epi8(t0, t3); //edgetype

                etype = _mm256_adds_epi8(etype, c2);

                t0 = _mm256_shuffle_epi8(off, etype);//get offset

				//convert byte to short for possible overflow
				t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
				t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
				t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
				t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

				t1 = _mm256_adds_epi16(t1, t3);
				t2 = _mm256_adds_epi16(t2, t4);
				t0 = _mm256_packus_epi16(t1, t2); //saturated
				t0 = _mm256_permute4x64_epi64(t0, 0xd8);

                if (x != end_x_32){
                    _mm256_storeu_si256((__m256i*)(dst + x), t0);
                }
                else{
                    if (mask_low)
                    {
					    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                        _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 16));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
    }
        break;
    case SAO_TYPE_EO_135: {
        __m256i off;
        __m256i s0, s1, s2;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask, offtmp;
        __m256i c2 = _mm256_set1_epi8(2);
        int end_x_r0_32, end_x_r_32, end_x_rn_32;
        int mask_low, mask_width;

		offtmp = _mm_loadu_si128((__m128i*)saoBlkParam->offset);
		offtmp = _mm_packs_epi32(offtmp, _mm_set_epi32(0, 0, 0, saoBlkParam->offset[4]));
		offtmp = _mm_packs_epi16(offtmp, _mm_setzero_si128());

		off = _mm256_castsi128_si256(offtmp);
		off = _mm256_inserti128_si256(off, offtmp, 1);

        start_x_r0 = smb_available_upleft ? 0 : 1;
        end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;
        start_x_r = smb_available_left ? 0 : 1;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        start_x_rn = smb_available_down ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
        end_x_rn = smb_available_rightdown ? smb_pix_width : (smb_pix_width - 1);

        end_x_r0_32 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x1f);
        end_x_r_32 = end_x_r - ((end_x_r - start_x_r) & 0x1f);
        end_x_rn_32 = end_x_rn - ((end_x_rn - start_x_rn) & 0x1f);

        //first row
        for (x = start_x_r0; x < end_x_r0; x += 32) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 1]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 1]);

            t3 = _mm256_min_epu8(s0, s1);
            t1 = _mm256_cmpeq_epi8(t3, s0);
            t2 = _mm256_cmpeq_epi8(t3, s1);
            t0 = _mm256_subs_epi8(t2, t1); //upsign

            t3 = _mm256_min_epu8(s1, s2);
            t1 = _mm256_cmpeq_epi8(t3, s1);
            t2 = _mm256_cmpeq_epi8(t3, s2);
            t3 = _mm256_subs_epi8(t1, t2); //downsign

            etype = _mm256_adds_epi8(t0, t3); //edgetype

            etype = _mm256_adds_epi8(etype, c2);

            t0 = _mm256_shuffle_epi8(off, etype);//get offset
			//convert byte to short for possible overflow
			t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
			t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
			t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
			t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

			t1 = _mm256_adds_epi16(t1, t3);
			t2 = _mm256_adds_epi16(t2, t4);
			t0 = _mm256_packus_epi16(t1, t2); //saturated
			t0 = _mm256_permute4x64_epi64(t0, 0xd8);

            if (x != end_x_r0_32){
                _mm256_storeu_si256((__m256i*)(dst + x), t0);
            }
            else{
                mask_width = end_x_r0 - end_x_r0_32;
                mask = _mm_loadu_si128((__m128i*)sao_mask_32[mask_width & 0x0f]);
                if (mask_width > 16)
                {
				    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                    _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 16));
                }
                else
                {
                    _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                }
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_width = end_x_r - end_x_r_32;
        mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
        mask_low = mask_width > 16;
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 32) {
                s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 1]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 1]);

                t3 = _mm256_min_epu8(s0, s1);
                t1 = _mm256_cmpeq_epi8(t3, s0);
                t2 = _mm256_cmpeq_epi8(t3, s1);
                t0 = _mm256_subs_epi8(t2, t1); //upsign

                t3 = _mm256_min_epu8(s1, s2);
                t1 = _mm256_cmpeq_epi8(t3, s1);
                t2 = _mm256_cmpeq_epi8(t3, s2);
                t3 = _mm256_subs_epi8(t1, t2); //downsign

                etype = _mm256_adds_epi8(t0, t3); //edgetype

                etype = _mm256_adds_epi8(etype, c2);

                t0 = _mm256_shuffle_epi8(off, etype);//get offset

				//convert byte to short for possible overflow
				t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
				t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
				t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
				t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

				t1 = _mm256_adds_epi16(t1, t3);
				t2 = _mm256_adds_epi16(t2, t4);
				t0 = _mm256_packus_epi16(t1, t2); //saturated
				t0 = _mm256_permute4x64_epi64(t0, 0xd8);

                if (x != end_x_r_32){
                    _mm256_storeu_si256((__m256i*)(dst + x), t0);
                }
                else{
                    if (mask_low)
                    {
                        _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                        _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 16));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        //last row
        for (x = start_x_rn; x < end_x_rn; x += 32) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 1]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 1]);

            t3 = _mm256_min_epu8(s0, s1);
            t1 = _mm256_cmpeq_epi8(t3, s0);
            t2 = _mm256_cmpeq_epi8(t3, s1);
            t0 = _mm256_subs_epi8(t2, t1); //upsign

            t3 = _mm256_min_epu8(s1, s2);
            t1 = _mm256_cmpeq_epi8(t3, s1);
            t2 = _mm256_cmpeq_epi8(t3, s2);
            t3 = _mm256_subs_epi8(t1, t2); //downsign

            etype = _mm256_adds_epi8(t0, t3); //edgetype

            etype = _mm256_adds_epi8(etype, c2);

            t0 = _mm256_shuffle_epi8(off, etype);//get offset

			//convert byte to short for possible overflow
			t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
			t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
			t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
			t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

			t1 = _mm256_adds_epi16(t1, t3);
			t2 = _mm256_adds_epi16(t2, t4);
			t0 = _mm256_packus_epi16(t1, t2); //saturated
			t0 = _mm256_permute4x64_epi64(t0, 0xd8);

            if (x != end_x_rn_32){
                _mm256_storeu_si256((__m256i*)(dst + x), t0);
            }
            else{
                mask_width = end_x_rn - end_x_rn_32;
                mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
                if (mask_width > 16)
                {
				    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                    _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 16));
                }
                else
                {
                    _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                }
                break;
            }
        }
    }
        break;
    case SAO_TYPE_EO_45: {
		__m256i off;
		__m256i s0, s1, s2;
		__m256i t0, t1, t2, t3, t4, etype;
		__m128i mask, offtmp;
		__m256i c2 = _mm256_set1_epi8(2);
		int end_x_r0_32, end_x_r_32, end_x_rn_32;
        int mask_width, mask_low;

		offtmp = _mm_loadu_si128((__m128i*)saoBlkParam->offset);
		offtmp = _mm_packs_epi32(offtmp, _mm_set_epi32(0, 0, 0, saoBlkParam->offset[4]));
		offtmp = _mm_packs_epi16(offtmp, _mm_setzero_si128());

		off = _mm256_castsi128_si256(offtmp);
		off = _mm256_inserti128_si256(off, offtmp, 1);

		start_x_r0 = smb_available_up ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
		end_x_r0 = smb_available_upright ? smb_pix_width : (smb_pix_width - 1);
		start_x_r = smb_available_left ? 0 : 1;
		end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
		start_x_rn = smb_available_leftdown ? 0 : 1;
		end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;

		end_x_r0_32 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x1f);
		end_x_r_32 = end_x_r - ((end_x_r - start_x_r) & 0x1f);
		end_x_rn_32 = end_x_rn - ((end_x_rn - start_x_rn) & 0x1f);

		//first row
		for (x = start_x_r0; x < end_x_r0; x += 32) {
			s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 1]);
			s1 = _mm256_loadu_si256((__m256i*)&src[x]);
			s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 1]);

			t3 = _mm256_min_epu8(s0, s1);
			t1 = _mm256_cmpeq_epi8(t3, s0);
			t2 = _mm256_cmpeq_epi8(t3, s1);
			t0 = _mm256_subs_epi8(t2, t1); //upsign

			t3 = _mm256_min_epu8(s1, s2);
			t1 = _mm256_cmpeq_epi8(t3, s1);
			t2 = _mm256_cmpeq_epi8(t3, s2);
			t3 = _mm256_subs_epi8(t1, t2); //downsign

			etype = _mm256_adds_epi8(t0, t3); //edgetype

			etype = _mm256_adds_epi8(etype, c2);

			t0 = _mm256_shuffle_epi8(off, etype);//get offset

			//convert byte to short for possible overflow
			t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
			t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
			t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
			t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

			t1 = _mm256_adds_epi16(t1, t3);
			t2 = _mm256_adds_epi16(t2, t4);
			t0 = _mm256_packus_epi16(t1, t2); //saturated
			t0 = _mm256_permute4x64_epi64(t0, 0xd8);

			if (x != end_x_r0_32){
				_mm256_storeu_si256((__m256i*)(dst + x), t0);
			}
			else{
                mask_width = end_x_r0 - end_x_r0_32;
				mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
                if (mask_width > 16)
                {
				    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
				    _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 16));
                }
                else
                {
                    _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                }
				break;
			}
		}
		dst += i_dst;
		src += i_src;

        mask_width = end_x_r - end_x_r_32;
		mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
        mask_low = mask_width > 16;
		//middle rows
		for (y = 1; y < smb_pix_height - 1; y++) {
			for (x = start_x_r; x < end_x_r; x += 32) {
				s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 1]);
				s1 = _mm256_loadu_si256((__m256i*)&src[x]);
				s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 1]);

				t3 = _mm256_min_epu8(s0, s1);
				t1 = _mm256_cmpeq_epi8(t3, s0);
				t2 = _mm256_cmpeq_epi8(t3, s1);
				t0 = _mm256_subs_epi8(t2, t1); //upsign

				t3 = _mm256_min_epu8(s1, s2);
				t1 = _mm256_cmpeq_epi8(t3, s1);
				t2 = _mm256_cmpeq_epi8(t3, s2);
				t3 = _mm256_subs_epi8(t1, t2); //downsign

				etype = _mm256_adds_epi8(t0, t3); //edgetype

				etype = _mm256_adds_epi8(etype, c2);

				t0 = _mm256_shuffle_epi8(off, etype);//get offset

				//convert byte to short for possible overflow
				t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
				t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
				t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
				t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

				t1 = _mm256_adds_epi16(t1, t3);
				t2 = _mm256_adds_epi16(t2, t4);
				t0 = _mm256_packus_epi16(t1, t2); //saturated
				t0 = _mm256_permute4x64_epi64(t0, 0xd8);

				if (x != end_x_r_32){
					_mm256_storeu_si256((__m256i*)(dst + x), t0);
				}
				else{
                    if (mask_low)
                    {
					    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
					    _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 16));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
					break;
				}
			}
			dst += i_dst;
			src += i_src;
		}
		for (x = start_x_rn; x < end_x_rn; x += 32) {
			s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 1]);
			s1 = _mm256_loadu_si256((__m256i*)&src[x]);
			s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 1]);

			t3 = _mm256_min_epu8(s0, s1);
			t1 = _mm256_cmpeq_epi8(t3, s0);
			t2 = _mm256_cmpeq_epi8(t3, s1);
			t0 = _mm256_subs_epi8(t2, t1); //upsign

			t3 = _mm256_min_epu8(s1, s2);
			t1 = _mm256_cmpeq_epi8(t3, s1);
			t2 = _mm256_cmpeq_epi8(t3, s2);
			t3 = _mm256_subs_epi8(t1, t2); //downsign

			etype = _mm256_adds_epi8(t0, t3); //edgetype

			etype = _mm256_adds_epi8(etype, c2);

			t0 = _mm256_shuffle_epi8(off, etype);//get offset

			//convert byte to short for possible overflow
			t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
			t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
			t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
			t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

			t1 = _mm256_adds_epi16(t1, t3);
			t2 = _mm256_adds_epi16(t2, t4);
			t0 = _mm256_packus_epi16(t1, t2); //saturated
			t0 = _mm256_permute4x64_epi64(t0, 0xd8);

			if (x != end_x_rn_32){
				_mm256_storeu_si256((__m256i*)(dst + x), t0);
			}
			else{
                mask_width = end_x_rn - end_x_rn_32;
				mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
                if (mask_width > 16)
                {
				    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
				    _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 16));
                }
                else
                {
                    _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                }
				break;
			}
		}
    }
        break;
	case SAO_TYPE_BO: {
		__m256i r0, r1, r2, r3, off0, off1, off2, off3;
		__m256i t0, t1, t2, t3, t4, src0, src1;
		__m128i mask = _mm_setzero_si128();
        __m256i shift_mask = _mm256_set1_epi8(31);
        int mask_low;

		r0 = _mm256_set1_epi8(saoBlkParam->bandIdx[0]);
		r1 = _mm256_set1_epi8(saoBlkParam->bandIdx[1]);
		r2 = _mm256_set1_epi8(saoBlkParam->bandIdx[2]);
		r3 = _mm256_set1_epi8(saoBlkParam->bandIdx[3]);
		off0 = _mm256_set1_epi8(saoBlkParam->offset[0]);
		off1 = _mm256_set1_epi8(saoBlkParam->offset[1]);
		off2 = _mm256_set1_epi8(saoBlkParam->offset[2]);
		off3 = _mm256_set1_epi8(saoBlkParam->offset[3]);

        mask = _mm_load_si128((__m128i*)(sao_mask_32[smb_pix_width & 0x0f]));
        mask_low = smb_pix_width > 16;
		for (y = 0; y < smb_pix_height; y++) {
            for (x = 0; x < smb_pix_width; x += 32){
				src0 = _mm256_loadu_si256((__m256i*)&src[x]);
                src1 = _mm256_srli_epi16(src0, 3);
                src1 = _mm256_and_si256(src1, shift_mask);

				t0 = _mm256_cmpeq_epi8(src1, r0);
				t1 = _mm256_cmpeq_epi8(src1, r1);
				t2 = _mm256_cmpeq_epi8(src1, r2);
				t3 = _mm256_cmpeq_epi8(src1, r3);

				t0 = _mm256_and_si256(t0, off0);
				t1 = _mm256_and_si256(t1, off1);
				t2 = _mm256_and_si256(t2, off2);
				t3 = _mm256_and_si256(t3, off3);
				t0 = _mm256_or_si256(t0, t1);
				t2 = _mm256_or_si256(t2, t3);
				t0 = _mm256_or_si256(t0, t2);
				//src0 = _mm_adds_epi8(src0, t0);
				//convert byte to short for possible overflow
				t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
				t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
				t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(src0));
				t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(src0, 1));

				t1 = _mm256_add_epi16(t1, t3);
				t2 = _mm256_add_epi16(t2, t4);
				t0 = _mm256_packus_epi16(t1, t2); //saturated
				t0 = _mm256_permute4x64_epi64(t0, 0xd8);

                if (smb_pix_width - x >= 32){
                    _mm256_storeu_si256((__m256i*)(dst + x), t0);
                } else{
                    if (mask_low)
                    {
                        _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                        _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 16));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
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


void uavs3d_sao_on_lcu_chroma_avx2(pel *src, int i_src, pel *dst, int i_dst, com_sao_param_t *saoBlkParam, int smb_pix_height, int smb_pix_width,
    int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int sample_bit_depth)
{
    int type;
    int start_x, end_x, start_y, end_y;
    int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
    int x, y;

    int smb_available_upleft = (smb_available_up && smb_available_left);
    int smb_available_upright = (smb_available_up && smb_available_right);
    int smb_available_leftdown = (smb_available_down && smb_available_left);
    int smb_available_rightdown = (smb_available_down && smb_available_right);
    __m256i mask_uv = _mm256_set1_epi16(0xff);
    __m256i zero = _mm256_setzero_si256();

    smb_pix_width <<= 1;

    type = saoBlkParam->type;

    switch (type) {
    case SAO_TYPE_EO_0: {
        __m256i off;
        __m256i s0, s1, s2, s3, s4, s5;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask0, mask1, mask2, offtmp;
        __m256i c2 = _mm256_set1_epi8(2);
        __m256i mask;

        int end_x_32;
        int mask_low;

        offtmp = _mm_loadu_si128((__m128i*)saoBlkParam->offset);
        offtmp = _mm_packs_epi32(offtmp, _mm_set_epi32(0, 0, 0, saoBlkParam->offset[4]));
        offtmp = _mm_packs_epi16(offtmp, _mm_setzero_si128());

        off = _mm256_castsi128_si256(offtmp);
        off = _mm256_inserti128_si256(off, offtmp, 1);

        start_x = smb_available_left ? 0 : 2;
        end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        end_x_32 = end_x - ((end_x - start_x) & 0x3f);

        mask0 = _mm_loadu_si128((__m128i*)(sao_mask_32[((end_x - end_x_32) >> 1) & 0xf]));
        mask1 = _mm_unpacklo_epi8(mask0, _mm_setzero_si128());
        mask2 = _mm_unpackhi_epi8(mask0, _mm_setzero_si128());
        mask = _mm256_set_m128i(mask2, mask1);
        mask_low = end_x - end_x_32 > 32;
        for (y = 0; y < smb_pix_height; y++) {
            //diff = src[start_x] - src[start_x - 1];
            //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = start_x; x < end_x; x += 64) {
                s0 = _mm256_lddqu_si256((__m256i*)&src[x - 2]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + 2]);
                s3 = _mm256_loadu_si256((__m256i*)&src[x + 30]);
                s4 = _mm256_loadu_si256((__m256i*)&src[x + 32]);
                s5 = _mm256_loadu_si256((__m256i*)&src[x + 34]);
                s0 = _mm256_and_si256(s0, mask_uv);
                s1 = _mm256_and_si256(s1, mask_uv);
                s2 = _mm256_and_si256(s2, mask_uv);
                s3 = _mm256_and_si256(s3, mask_uv);
                s4 = _mm256_and_si256(s4, mask_uv);
                s5 = _mm256_and_si256(s5, mask_uv);
                s0 = _mm256_packus_epi16(s0, s3);
                s1 = _mm256_packus_epi16(s1, s4);
                s2 = _mm256_packus_epi16(s2, s5);

                t3 = _mm256_min_epu8(s0, s1);
                t1 = _mm256_cmpeq_epi8(t3, s0);
                t2 = _mm256_cmpeq_epi8(t3, s1);
                t0 = _mm256_subs_epi8(t2, t1); //leftsign

                t3 = _mm256_min_epu8(s1, s2);
                t1 = _mm256_cmpeq_epi8(t3, s1);
                t2 = _mm256_cmpeq_epi8(t3, s2);
                t3 = _mm256_subs_epi8(t1, t2); //rightsign

                etype = _mm256_adds_epi8(t0, t3); //edgetype

                etype = _mm256_adds_epi8(etype, c2);

                t0 = _mm256_shuffle_epi8(off, etype);//get offset

                                                     //convert byte to short for possible overflow
                t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
                t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
                t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
                t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

                t1 = _mm256_adds_epi16(t1, t3);
                t2 = _mm256_adds_epi16(t2, t4);
                t0 = _mm256_packus_epi16(t1, t2); //saturated
                t0 = _mm256_permute4x64_epi64(t0, 0xd8);
                t1 = _mm256_unpacklo_epi8(t0, zero);
                t2 = _mm256_unpackhi_epi8(t0, zero);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
                t4 = _mm256_loadu_si256((__m256i*)&dst[x + 32]);
                if (x != end_x_32) {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                }
                else {
                    if (mask_low)
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        break;
    }
    case SAO_TYPE_EO_90: {
        __m256i off;
        __m256i s0, s1, s2, s3, s4, s5;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask0, mask1, mask2, offtmp;
        __m256i c2 = _mm256_set1_epi8(2);
        __m256i mask;
        int end_x_32 = smb_pix_width - (smb_pix_width & 0x3f);
        int mask_low;

        offtmp = _mm_loadu_si128((__m128i*)saoBlkParam->offset);
        offtmp = _mm_packs_epi32(offtmp, _mm_set_epi32(0, 0, 0, saoBlkParam->offset[4]));
        offtmp = _mm_packs_epi16(offtmp, _mm_setzero_si128());

        off = _mm256_castsi128_si256(offtmp);
        off = _mm256_inserti128_si256(off, offtmp, 1);

        start_y = smb_available_up ? 0 : 1;
        end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1);

        dst += start_y * i_dst;
        src += start_y * i_src;

        mask0 = _mm_loadu_si128((__m128i*)(sao_mask_32[(smb_pix_width >> 1) & 0xf]));
        mask1 = _mm_unpacklo_epi8(mask0, _mm_setzero_si128());
        mask2 = _mm_unpackhi_epi8(mask0, _mm_setzero_si128());
        mask = _mm256_set_m128i(mask2, mask1);
        mask_low = (smb_pix_width & 0x3f) > 32;
        for (y = start_y; y < end_y; y++) {
            //diff = src[start_x] - src[start_x - 1];
            //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = 0; x < smb_pix_width; x += 64) {
                s0 = _mm256_lddqu_si256((__m256i*)&src[x - i_src]);
                s1 = _mm256_lddqu_si256((__m256i*)&src[x]);
                s2 = _mm256_lddqu_si256((__m256i*)&src[x + i_src]);
                s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 32]);
                s4 = _mm256_loadu_si256((__m256i*)&src[x + 32]);
                s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 32]);
                s0 = _mm256_and_si256(s0, mask_uv);
                s1 = _mm256_and_si256(s1, mask_uv);
                s2 = _mm256_and_si256(s2, mask_uv);
                s3 = _mm256_and_si256(s3, mask_uv);
                s4 = _mm256_and_si256(s4, mask_uv);
                s5 = _mm256_and_si256(s5, mask_uv);
                s0 = _mm256_packus_epi16(s0, s3);
                s1 = _mm256_packus_epi16(s1, s4);
                s2 = _mm256_packus_epi16(s2, s5);

                t3 = _mm256_min_epu8(s0, s1);
                t1 = _mm256_cmpeq_epi8(t3, s0);
                t2 = _mm256_cmpeq_epi8(t3, s1);
                t0 = _mm256_subs_epi8(t2, t1); //leftsign

                t3 = _mm256_min_epu8(s1, s2);
                t1 = _mm256_cmpeq_epi8(t3, s1);
                t2 = _mm256_cmpeq_epi8(t3, s2);
                t3 = _mm256_subs_epi8(t1, t2); //rightsign

                etype = _mm256_adds_epi8(t0, t3); //edgetype

                etype = _mm256_adds_epi8(etype, c2);

                t0 = _mm256_shuffle_epi8(off, etype);//get offset

                                                     //convert byte to short for possible overflow
                t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
                t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
                t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
                t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

                t1 = _mm256_adds_epi16(t1, t3);
                t2 = _mm256_adds_epi16(t2, t4);
                t0 = _mm256_packus_epi16(t1, t2); //saturated
                t0 = _mm256_permute4x64_epi64(t0, 0xd8);
                t1 = _mm256_unpacklo_epi8(t0, zero);
                t2 = _mm256_unpackhi_epi8(t0, zero);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
                t4 = _mm256_loadu_si256((__m256i*)&dst[x + 32]);
                if (x != end_x_32) {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                }
                else {
                    if (mask_low)
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        break;
    }
    case SAO_TYPE_EO_135: {
        __m256i off;
        __m256i s0, s1, s2, s3, s4, s5;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask0, mask1, mask2, offtmp;
        __m256i c2 = _mm256_set1_epi8(2);
        __m256i mask, zero = _mm256_setzero_si256();
        int end_x_r0_32, end_x_r_32, end_x_rn_32;
        int mask_low, mask_width;

        offtmp = _mm_loadu_si128((__m128i*)saoBlkParam->offset);
        offtmp = _mm_packs_epi32(offtmp, _mm_set_epi32(0, 0, 0, saoBlkParam->offset[4]));
        offtmp = _mm_packs_epi16(offtmp, _mm_setzero_si128());

        off = _mm256_castsi128_si256(offtmp);
        off = _mm256_inserti128_si256(off, offtmp, 1);

        start_x_r0 = smb_available_upleft ? 0 : 2;
        end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 2)) : 2;
        start_x_r = smb_available_left ? 0 : 2;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        start_x_rn = smb_available_down ? (smb_available_left ? 0 : 2) : (smb_pix_width - 2);
        end_x_rn = smb_available_rightdown ? smb_pix_width : (smb_pix_width - 2);

        end_x_r0_32 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x3f);
        end_x_r_32  = end_x_r - ((end_x_r - start_x_r) & 0x3f);
        end_x_rn_32 = end_x_rn - ((end_x_rn - start_x_rn) & 0x3f);

        //first row
        for (x = start_x_r0; x < end_x_r0; x += 64) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 2]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 2]);
            s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 30]);
            s4 = _mm256_loadu_si256((__m256i*)&src[x + 32]);
            s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 34]);
            s0 = _mm256_and_si256(s0, mask_uv);
            s1 = _mm256_and_si256(s1, mask_uv);
            s2 = _mm256_and_si256(s2, mask_uv);
            s3 = _mm256_and_si256(s3, mask_uv);
            s4 = _mm256_and_si256(s4, mask_uv);
            s5 = _mm256_and_si256(s5, mask_uv);
            s0 = _mm256_packus_epi16(s0, s3);
            s1 = _mm256_packus_epi16(s1, s4);
            s2 = _mm256_packus_epi16(s2, s5);

            t3 = _mm256_min_epu8(s0, s1);
            t1 = _mm256_cmpeq_epi8(t3, s0);
            t2 = _mm256_cmpeq_epi8(t3, s1);
            t0 = _mm256_subs_epi8(t2, t1); //upsign

            t3 = _mm256_min_epu8(s1, s2);
            t1 = _mm256_cmpeq_epi8(t3, s1);
            t2 = _mm256_cmpeq_epi8(t3, s2);
            t3 = _mm256_subs_epi8(t1, t2); //downsign

            etype = _mm256_adds_epi8(t0, t3); //edgetype

            etype = _mm256_adds_epi8(etype, c2);

            t0 = _mm256_shuffle_epi8(off, etype);//get offset
                                                 //convert byte to short for possible overflow
            t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
            t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
            t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
            t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

            t1 = _mm256_adds_epi16(t1, t3);
            t2 = _mm256_adds_epi16(t2, t4);
            t0 = _mm256_packus_epi16(t1, t2); //saturated
            t0 = _mm256_permute4x64_epi64(t0, 0xd8);
            t1 = _mm256_unpacklo_epi8(t0, zero);
            t2 = _mm256_unpackhi_epi8(t0, zero);
            t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
            t4 = _mm256_loadu_si256((__m256i*)&dst[x + 32]);

            if (x != end_x_r0_32) {
                t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                _mm256_storeu_si256((__m256i*)(dst + x), t1);
                _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
            }
            else {
                mask_width = (end_x_r0 - end_x_r0_32) >> 1;
                mask0 = _mm_loadu_si128((__m128i*)sao_mask_32[mask_width & 0x0f]);
                mask1 = _mm_unpacklo_epi8(mask0, _mm_setzero_si128());
                mask2 = _mm_unpackhi_epi8(mask0, _mm_setzero_si128());
                mask = _mm256_set_m128i(mask2, mask1);
                if (mask_width > 16)
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                }
                else
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                }
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_width = (end_x_r - end_x_r_32) >> 1;
        mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
        mask1 = _mm_unpacklo_epi8(mask0, _mm_setzero_si128());
        mask2 = _mm_unpackhi_epi8(mask0, _mm_setzero_si128());
        mask = _mm256_set_m128i(mask2, mask1);
        mask_low = mask_width > 16;
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 64) {
                s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 2]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 2]);
                s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 30]);
                s4 = _mm256_loadu_si256((__m256i*)&src[x + 32]);
                s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 34]);
                s0 = _mm256_and_si256(s0, mask_uv);
                s1 = _mm256_and_si256(s1, mask_uv);
                s2 = _mm256_and_si256(s2, mask_uv);
                s3 = _mm256_and_si256(s3, mask_uv);
                s4 = _mm256_and_si256(s4, mask_uv);
                s5 = _mm256_and_si256(s5, mask_uv);
                s0 = _mm256_packus_epi16(s0, s3);
                s1 = _mm256_packus_epi16(s1, s4);
                s2 = _mm256_packus_epi16(s2, s5);

                t3 = _mm256_min_epu8(s0, s1);
                t1 = _mm256_cmpeq_epi8(t3, s0);
                t2 = _mm256_cmpeq_epi8(t3, s1);
                t0 = _mm256_subs_epi8(t2, t1); //upsign

                t3 = _mm256_min_epu8(s1, s2);
                t1 = _mm256_cmpeq_epi8(t3, s1);
                t2 = _mm256_cmpeq_epi8(t3, s2);
                t3 = _mm256_subs_epi8(t1, t2); //downsign

                etype = _mm256_adds_epi8(t0, t3); //edgetype

                etype = _mm256_adds_epi8(etype, c2);

                t0 = _mm256_shuffle_epi8(off, etype);//get offset

                                                     //convert byte to short for possible overflow
                t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
                t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
                t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
                t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

                t1 = _mm256_adds_epi16(t1, t3);
                t2 = _mm256_adds_epi16(t2, t4);
                t0 = _mm256_packus_epi16(t1, t2); //saturated
                t0 = _mm256_permute4x64_epi64(t0, 0xd8);
                t1 = _mm256_unpacklo_epi8(t0, zero);
                t2 = _mm256_unpackhi_epi8(t0, zero);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
                t4 = _mm256_loadu_si256((__m256i*)&dst[x + 32]);
                if (x != end_x_r_32) {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                }
                else {
                    if (mask_low)
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        //last row
        for (x = start_x_rn; x < end_x_rn; x += 64) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 2]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 2]);
            s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 30]);
            s4 = _mm256_loadu_si256((__m256i*)&src[x + 32]);
            s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 34]);
            s0 = _mm256_and_si256(s0, mask_uv);
            s1 = _mm256_and_si256(s1, mask_uv);
            s2 = _mm256_and_si256(s2, mask_uv);
            s3 = _mm256_and_si256(s3, mask_uv);
            s4 = _mm256_and_si256(s4, mask_uv);
            s5 = _mm256_and_si256(s5, mask_uv);
            s0 = _mm256_packus_epi16(s0, s3);
            s1 = _mm256_packus_epi16(s1, s4);
            s2 = _mm256_packus_epi16(s2, s5);

            t3 = _mm256_min_epu8(s0, s1);
            t1 = _mm256_cmpeq_epi8(t3, s0);
            t2 = _mm256_cmpeq_epi8(t3, s1);
            t0 = _mm256_subs_epi8(t2, t1); //upsign

            t3 = _mm256_min_epu8(s1, s2);
            t1 = _mm256_cmpeq_epi8(t3, s1);
            t2 = _mm256_cmpeq_epi8(t3, s2);
            t3 = _mm256_subs_epi8(t1, t2); //downsign

            etype = _mm256_adds_epi8(t0, t3); //edgetype

            etype = _mm256_adds_epi8(etype, c2);

            t0 = _mm256_shuffle_epi8(off, etype);//get offset

                                                 //convert byte to short for possible overflow
            t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
            t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
            t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
            t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

            t1 = _mm256_adds_epi16(t1, t3);
            t2 = _mm256_adds_epi16(t2, t4);
            t0 = _mm256_packus_epi16(t1, t2); //saturated
            t0 = _mm256_permute4x64_epi64(t0, 0xd8);
            t1 = _mm256_unpacklo_epi8(t0, zero);
            t2 = _mm256_unpackhi_epi8(t0, zero);
            t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
            t4 = _mm256_loadu_si256((__m256i*)&dst[x + 32]);

            if (x != end_x_rn_32) {
                t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                _mm256_storeu_si256((__m256i*)(dst + x), t1);
                _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
            }
            else {
                mask_width = (end_x_rn - end_x_rn_32) >> 1;
                mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
                mask1 = _mm_unpacklo_epi8(mask0, _mm_setzero_si128());
                mask2 = _mm_unpackhi_epi8(mask0, _mm_setzero_si128());
                mask = _mm256_set_m128i(mask2, mask1);
                if (mask_width > 16)
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                }
                else
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                }
                break;
            }
        }
    }
        break;
    case SAO_TYPE_EO_45: {
        __m256i off;
        __m256i s0, s1, s2, s3, s4, s5;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask0, mask1, mask2, offtmp;
        __m256i mask;
        __m256i c2 = _mm256_set1_epi8(2);
        int end_x_r0_32, end_x_r_32, end_x_rn_32;
        int mask_width, mask_low;

        offtmp = _mm_loadu_si128((__m128i*)saoBlkParam->offset);
        offtmp = _mm_packs_epi32(offtmp, _mm_set_epi32(0, 0, 0, saoBlkParam->offset[4]));
        offtmp = _mm_packs_epi16(offtmp, _mm_setzero_si128());

        off = _mm256_castsi128_si256(offtmp);
        off = _mm256_inserti128_si256(off, offtmp, 1);

        start_x_r0 = smb_available_up ? (smb_available_left ? 0 : 2) : (smb_pix_width - 2);
        end_x_r0 = smb_available_upright ? smb_pix_width : (smb_pix_width - 2);
        start_x_r = smb_available_left ? 0 : 2;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        start_x_rn = smb_available_leftdown ? 0 : 2;
        end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 2)) : 2;

        end_x_r0_32 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x3f);
        end_x_r_32 = end_x_r - ((end_x_r - start_x_r) & 0x3f);
        end_x_rn_32 = end_x_rn - ((end_x_rn - start_x_rn) & 0x3f);

        //first row
        for (x = start_x_r0; x < end_x_r0; x += 64) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 2]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 2]);
            s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 34]);
            s4 = _mm256_loadu_si256((__m256i*)&src[x + 32]);
            s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 30]);
            s0 = _mm256_and_si256(s0, mask_uv);
            s1 = _mm256_and_si256(s1, mask_uv);
            s2 = _mm256_and_si256(s2, mask_uv);
            s3 = _mm256_and_si256(s3, mask_uv);
            s4 = _mm256_and_si256(s4, mask_uv);
            s5 = _mm256_and_si256(s5, mask_uv);
            s0 = _mm256_packus_epi16(s0, s3);
            s1 = _mm256_packus_epi16(s1, s4);
            s2 = _mm256_packus_epi16(s2, s5);

            t3 = _mm256_min_epu8(s0, s1);
            t1 = _mm256_cmpeq_epi8(t3, s0);
            t2 = _mm256_cmpeq_epi8(t3, s1);
            t0 = _mm256_subs_epi8(t2, t1); //upsign

            t3 = _mm256_min_epu8(s1, s2);
            t1 = _mm256_cmpeq_epi8(t3, s1);
            t2 = _mm256_cmpeq_epi8(t3, s2);
            t3 = _mm256_subs_epi8(t1, t2); //downsign

            etype = _mm256_adds_epi8(t0, t3); //edgetype

            etype = _mm256_adds_epi8(etype, c2);

            t0 = _mm256_shuffle_epi8(off, etype);//get offset

                                                 //convert byte to short for possible overflow
            t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
            t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
            t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
            t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

            t1 = _mm256_adds_epi16(t1, t3);
            t2 = _mm256_adds_epi16(t2, t4);
            t0 = _mm256_packus_epi16(t1, t2); //saturated
            t0 = _mm256_permute4x64_epi64(t0, 0xd8);
            t1 = _mm256_unpacklo_epi8(t0, zero);
            t2 = _mm256_unpackhi_epi8(t0, zero);
            t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
            t4 = _mm256_loadu_si256((__m256i*)&dst[x + 32]);

            if (x != end_x_r0_32) {
                t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                _mm256_storeu_si256((__m256i*)(dst + x), t1);
                _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
            }
            else {
                mask_width = (end_x_r0 - end_x_r0_32) >> 1;
                mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
                mask1 = _mm_unpacklo_epi8(mask0, _mm_setzero_si128());
                mask2 = _mm_unpackhi_epi8(mask0, _mm_setzero_si128());
                mask = _mm256_set_m128i(mask2, mask1);
                if (mask_width > 16)
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                }
                else
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                }
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_width = (end_x_r - end_x_r_32) >> 1;
        mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
        mask1 = _mm_unpacklo_epi8(mask0, _mm_setzero_si128());
        mask2 = _mm_unpackhi_epi8(mask0, _mm_setzero_si128());
        mask = _mm256_set_m128i(mask2, mask1);
        mask_low = mask_width > 16;
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 64) {
                s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 2]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 2]);
                s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 34]);
                s4 = _mm256_loadu_si256((__m256i*)&src[x + 32]);
                s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 30]);
                s0 = _mm256_and_si256(s0, mask_uv);
                s1 = _mm256_and_si256(s1, mask_uv);
                s2 = _mm256_and_si256(s2, mask_uv);
                s3 = _mm256_and_si256(s3, mask_uv);
                s4 = _mm256_and_si256(s4, mask_uv);
                s5 = _mm256_and_si256(s5, mask_uv);
                s0 = _mm256_packus_epi16(s0, s3);
                s1 = _mm256_packus_epi16(s1, s4);
                s2 = _mm256_packus_epi16(s2, s5);

                t3 = _mm256_min_epu8(s0, s1);
                t1 = _mm256_cmpeq_epi8(t3, s0);
                t2 = _mm256_cmpeq_epi8(t3, s1);
                t0 = _mm256_subs_epi8(t2, t1); //upsign

                t3 = _mm256_min_epu8(s1, s2);
                t1 = _mm256_cmpeq_epi8(t3, s1);
                t2 = _mm256_cmpeq_epi8(t3, s2);
                t3 = _mm256_subs_epi8(t1, t2); //downsign

                etype = _mm256_adds_epi8(t0, t3); //edgetype

                etype = _mm256_adds_epi8(etype, c2);

                t0 = _mm256_shuffle_epi8(off, etype);//get offset

                                                     //convert byte to short for possible overflow
                t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
                t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
                t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
                t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

                t1 = _mm256_adds_epi16(t1, t3);
                t2 = _mm256_adds_epi16(t2, t4);
                t0 = _mm256_packus_epi16(t1, t2); //saturated
                t0 = _mm256_permute4x64_epi64(t0, 0xd8);
                t1 = _mm256_unpacklo_epi8(t0, zero);
                t2 = _mm256_unpackhi_epi8(t0, zero);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
                t4 = _mm256_loadu_si256((__m256i*)&dst[x + 32]);

                if (x != end_x_r_32) {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                }
                else {
                    if (mask_low)
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        for (x = start_x_rn; x < end_x_rn; x += 64) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 2]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 2]);
            s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 34]);
            s4 = _mm256_loadu_si256((__m256i*)&src[x + 32]);
            s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 30]);
            s0 = _mm256_and_si256(s0, mask_uv);
            s1 = _mm256_and_si256(s1, mask_uv);
            s2 = _mm256_and_si256(s2, mask_uv);
            s3 = _mm256_and_si256(s3, mask_uv);
            s4 = _mm256_and_si256(s4, mask_uv);
            s5 = _mm256_and_si256(s5, mask_uv);
            s0 = _mm256_packus_epi16(s0, s3);
            s1 = _mm256_packus_epi16(s1, s4);
            s2 = _mm256_packus_epi16(s2, s5);

            t3 = _mm256_min_epu8(s0, s1);
            t1 = _mm256_cmpeq_epi8(t3, s0);
            t2 = _mm256_cmpeq_epi8(t3, s1);
            t0 = _mm256_subs_epi8(t2, t1); //upsign

            t3 = _mm256_min_epu8(s1, s2);
            t1 = _mm256_cmpeq_epi8(t3, s1);
            t2 = _mm256_cmpeq_epi8(t3, s2);
            t3 = _mm256_subs_epi8(t1, t2); //downsign

            etype = _mm256_adds_epi8(t0, t3); //edgetype

            etype = _mm256_adds_epi8(etype, c2);

            t0 = _mm256_shuffle_epi8(off, etype);//get offset

                                                 //convert byte to short for possible overflow
            t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
            t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
            t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(s1));
            t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(s1, 1));

            t1 = _mm256_adds_epi16(t1, t3);
            t2 = _mm256_adds_epi16(t2, t4);
            t0 = _mm256_packus_epi16(t1, t2); //saturated
            t0 = _mm256_permute4x64_epi64(t0, 0xd8);
            t1 = _mm256_unpacklo_epi8(t0, zero);
            t2 = _mm256_unpackhi_epi8(t0, zero);
            t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
            t4 = _mm256_loadu_si256((__m256i*)&dst[x + 32]);

            if (x != end_x_rn_32) {
                t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                _mm256_storeu_si256((__m256i*)(dst + x), t1);
                _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
            }
            else {
                mask_width = (end_x_rn - end_x_rn_32) >> 1;
                mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x0f]));
                mask1 = _mm_unpacklo_epi8(mask0, _mm_setzero_si128());
                mask2 = _mm_unpackhi_epi8(mask0, _mm_setzero_si128());
                mask = _mm256_set_m128i(mask2, mask1);
                if (mask_width > 16)
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                }
                else
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                }
                break;
            }
        }
    }
                         break;
    case SAO_TYPE_BO: {
        __m256i r0, r1, r2, r3, off0, off1, off2, off3;
        __m256i t0, t1, t2, t3, t4, src0, src1, src2;
        __m128i mask0, mask1, mask2;
        __m256i mask;
        __m256i shift_mask = _mm256_set1_epi8(31);
        int mask_low;

        r0 = _mm256_set1_epi8(saoBlkParam->bandIdx[0]);
        r1 = _mm256_set1_epi8(saoBlkParam->bandIdx[1]);
        r2 = _mm256_set1_epi8(saoBlkParam->bandIdx[2]);
        r3 = _mm256_set1_epi8(saoBlkParam->bandIdx[3]);
        off0 = _mm256_set1_epi8(saoBlkParam->offset[0]);
        off1 = _mm256_set1_epi8(saoBlkParam->offset[1]);
        off2 = _mm256_set1_epi8(saoBlkParam->offset[2]);
        off3 = _mm256_set1_epi8(saoBlkParam->offset[3]);

        mask0 = _mm_load_si128((__m128i*)(sao_mask_32[(smb_pix_width >> 1) & 0x0f]));
        mask1 = _mm_unpacklo_epi8(mask0, _mm_setzero_si128());
        mask2 = _mm_unpackhi_epi8(mask0, _mm_setzero_si128());
        mask = _mm256_set_m128i(mask2, mask1);

        mask_low = smb_pix_width - (smb_pix_width / 64)*64 > 32;
        for (y = 0; y < smb_pix_height; y++) {
            for (x = 0; x < smb_pix_width; x += 64) {
                src0 = _mm256_loadu_si256((__m256i*)&src[x]);
                src2 = _mm256_loadu_si256((__m256i*)&src[x + 32]);
                src0 = _mm256_and_si256(src0, mask_uv);
                src2 = _mm256_and_si256(src2, mask_uv);
                src0 = _mm256_packus_epi16(src0, src2);

                src1 = _mm256_srli_epi16(src0, 3);
                src1 = _mm256_and_si256(src1, shift_mask);

                t0 = _mm256_cmpeq_epi8(src1, r0);
                t1 = _mm256_cmpeq_epi8(src1, r1);
                t2 = _mm256_cmpeq_epi8(src1, r2);
                t3 = _mm256_cmpeq_epi8(src1, r3);

                t0 = _mm256_and_si256(t0, off0);
                t1 = _mm256_and_si256(t1, off1);
                t2 = _mm256_and_si256(t2, off2);
                t3 = _mm256_and_si256(t3, off3);
                t0 = _mm256_or_si256(t0, t1);
                t2 = _mm256_or_si256(t2, t3);
                t0 = _mm256_or_si256(t0, t2);
                //src0 = _mm_adds_epi8(src0, t0);
                //convert byte to short for possible overflow
                t1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
                t2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
                t3 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(src0));
                t4 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(src0, 1));

                t1 = _mm256_add_epi16(t1, t3);
                t2 = _mm256_add_epi16(t2, t4);
                t0 = _mm256_packus_epi16(t1, t2); //saturated
                t0 = _mm256_permute4x64_epi64(t0, 0xd8);
                t1 = _mm256_unpacklo_epi8(t0, zero);
                t2 = _mm256_unpackhi_epi8(t0, zero);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
                t4 = _mm256_loadu_si256((__m256i*)&dst[x + 32]);

                if (smb_pix_width - x >= 64) {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                }
                else {
                    if (mask_low)
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 32), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
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

#define CALCU_EO_10BIT              \
t3 = _mm256_min_epu16(s0, s1);      \
t1 = _mm256_cmpeq_epi16(t3, s0);    \
t2 = _mm256_cmpeq_epi16(t3, s1);    \
t0 = _mm256_subs_epi16(t2, t1);     \
                                    \
t3 = _mm256_min_epu16(s1, s2);      \
t1 = _mm256_cmpeq_epi16(t3, s1);    \
t2 = _mm256_cmpeq_epi16(t3, s2);    \
t3 = _mm256_subs_epi16(t1, t2);     \
                                    \
etype = _mm256_adds_epi16(t0, t3);  \
                                    \
t0 = _mm256_cmpeq_epi16(etype, c0); \
t1 = _mm256_cmpeq_epi16(etype, c1); \
t2 = _mm256_cmpeq_epi16(etype, c2); \
t3 = _mm256_cmpeq_epi16(etype, c3); \
t4 = _mm256_cmpeq_epi16(etype, c4); \
                                    \
t0 = _mm256_and_si256(t0, off0);    \
t1 = _mm256_and_si256(t1, off1);    \
t2 = _mm256_and_si256(t2, off2);    \
t3 = _mm256_and_si256(t3, off3);    \
t4 = _mm256_and_si256(t4, off4);    \
                                    \
t0 = _mm256_adds_epi16(t0, t1);     \
t2 = _mm256_adds_epi16(t2, t3);     \
t0 = _mm256_adds_epi16(t0, t4);     \
t0 = _mm256_adds_epi16(t0, t2);     \
                                    \
t0 = _mm256_adds_epi16(t0, s1);     \
t0 = _mm256_min_epi16(t0, max_val); \
t0 = _mm256_max_epi16(t0, min_val) 

void uavs3d_sao_on_lcu_avx2(pel *src, int i_src, pel *dst, int i_dst, com_sao_param_t *sao_params, int smb_pix_height, int smb_pix_width,
    int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int sample_bit_depth)
{
    int type;
    int start_x, end_x, start_y, end_y;
    int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
    int x, y;
    int reg = 0;

    int smb_available_upleft = (smb_available_up && smb_available_left);
    int smb_available_upright = (smb_available_up && smb_available_right);
    int smb_available_leftdown = (smb_available_down && smb_available_left);
    int smb_available_rightdown = (smb_available_down && smb_available_right);
    __m256i min_val = _mm256_setzero_si256();
    __m256i max_val = _mm256_set1_epi16((1 << sample_bit_depth) - 1);

    type = sao_params->type;

    switch (type) {
    case SAO_TYPE_EO_0: {
        __m256i s0, s1, s2;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask;
        __m256i off0, off1, off2, off3, off4;
        __m256i c0, c1, c2, c3, c4;
        int end_x_16;
        int mask_low;

        c0 = _mm256_set1_epi16(-2);
        c1 = _mm256_set1_epi16(-1);
        c2 = _mm256_set1_epi16(0);
        c3 = _mm256_set1_epi16(1);
        c4 = _mm256_set1_epi16(2);

        off0 = _mm256_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm256_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm256_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm256_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm256_set1_epi16((pel)sao_params->offset[4]);

        start_x = smb_available_left ? 0 : 1;
        end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        end_x_16 = end_x - ((end_x - start_x) & 0x0f);

        mask = _mm_loadu_si128((__m128i*)(sao_mask_32[(end_x - end_x_16) & 0x07]));
        mask_low = end_x - end_x_16 > 8;
        for (y = 0; y < smb_pix_height; y++) {
            //diff = src[start_x] - src[start_x - 1];
            //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = start_x; x < end_x; x += 16) {
                s0 = _mm256_lddqu_si256((__m256i*)&src[x - 1]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + 1]);

                CALCU_EO_10BIT;

                if (x != end_x_16) {
                    _mm256_storeu_si256((__m256i*)(dst + x), t0);
                }
                else {
                    if (mask_low)
                    {
                        _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                        _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 8));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        break;
    }
    case SAO_TYPE_EO_90: {
        __m256i s0, s1, s2;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask;
        __m256i off0, off1, off2, off3, off4;
        __m256i c0, c1, c2, c3, c4;
        int end_x_16 = smb_pix_width - (smb_pix_width & 0x0f);
        int mask_low;

        c0 = _mm256_set1_epi16(-2);
        c1 = _mm256_set1_epi16(-1);
        c2 = _mm256_set1_epi16(0);
        c3 = _mm256_set1_epi16(1);
        c4 = _mm256_set1_epi16(2);

        off0 = _mm256_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm256_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm256_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm256_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm256_set1_epi16((pel)sao_params->offset[4]);

        start_y = smb_available_up ? 0 : 1;
        end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1);

        dst += start_y * i_dst;
        src += start_y * i_src;

        mask = _mm_loadu_si128((__m128i*)(sao_mask_32[smb_pix_width & 0x07]));
        mask_low = (smb_pix_width & 0x0f) > 8;
        for (y = start_y; y < end_y; y++) {
            //diff = src[start_x] - src[start_x - 1];
            //leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
            for (x = 0; x < smb_pix_width; x += 16) {
                s0 = _mm256_lddqu_si256((__m256i*)&src[x - i_src]);
                s1 = _mm256_lddqu_si256((__m256i*)&src[x]);
                s2 = _mm256_lddqu_si256((__m256i*)&src[x + i_src]);

                CALCU_EO_10BIT;

                if (x != end_x_16) {
                    _mm256_storeu_si256((__m256i*)(dst + x), t0);
                }
                else {
                    if (mask_low)
                    {
                        _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                        _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 8));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        break;
    }
    case SAO_TYPE_EO_135: {
        __m256i s0, s1, s2;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask;
        __m256i off0, off1, off2, off3, off4;
        __m256i c0, c1, c2, c3, c4;
        int end_x_r0_16, end_x_r_16, end_x_rn_16;
        int mask_low, mask_width;

        c0 = _mm256_set1_epi16(-2);
        c1 = _mm256_set1_epi16(-1);
        c2 = _mm256_set1_epi16(0);
        c3 = _mm256_set1_epi16(1);
        c4 = _mm256_set1_epi16(2);

        off0 = _mm256_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm256_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm256_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm256_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm256_set1_epi16((pel)sao_params->offset[4]);

        start_x_r0 = smb_available_upleft ? 0 : 1;
        end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;
        start_x_r = smb_available_left ? 0 : 1;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        start_x_rn = smb_available_down ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
        end_x_rn = smb_available_rightdown ? smb_pix_width : (smb_pix_width - 1);

        end_x_r0_16 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x0f);
        end_x_r_16 = end_x_r - ((end_x_r - start_x_r) & 0x0f);
        end_x_rn_16 = end_x_rn - ((end_x_rn - start_x_rn) & 0x0f);

        //first row
        for (x = start_x_r0; x < end_x_r0; x += 16) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 1]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 1]);

            CALCU_EO_10BIT;

            if (x != end_x_r0_16) {
                _mm256_storeu_si256((__m256i*)(dst + x), t0);
            }
            else {
                mask_width = end_x_r0 - end_x_r0_16;
                mask = _mm_loadu_si128((__m128i*)sao_mask_32[mask_width & 0x07]);
                if (mask_width > 8)
                {
                    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                    _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 8));
                }
                else
                {
                    _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                }
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_width = end_x_r - end_x_r_16;
        mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
        mask_low = mask_width > 8;
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 16) {
                s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 1]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 1]);

                CALCU_EO_10BIT;

                if (x != end_x_r_16) {
                    _mm256_storeu_si256((__m256i*)(dst + x), t0);
                }
                else {
                    if (mask_low)
                    {
                        _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                        _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 8));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        //last row
        for (x = start_x_rn; x < end_x_rn; x += 16) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 1]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 1]);

            CALCU_EO_10BIT;

            if (x != end_x_rn_16) {
                _mm256_storeu_si256((__m256i*)(dst + x), t0);
            }
            else {
                mask_width = end_x_rn - end_x_rn_16;
                mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
                if (mask_width > 8)
                {
                    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                    _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 8));
                }
                else
                {
                    _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                }
                break;
            }
        }
        break;
    }
    case SAO_TYPE_EO_45: {
        __m256i s0, s1, s2;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask;
        __m256i off0, off1, off2, off3, off4;
        __m256i c0, c1, c2, c3, c4;
        int end_x_r0_16, end_x_r_16, end_x_rn_16;
        int mask_width, mask_low;

        c0 = _mm256_set1_epi16(-2);
        c1 = _mm256_set1_epi16(-1);
        c2 = _mm256_set1_epi16(0);
        c3 = _mm256_set1_epi16(1);
        c4 = _mm256_set1_epi16(2);

        off0 = _mm256_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm256_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm256_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm256_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm256_set1_epi16((pel)sao_params->offset[4]);

        start_x_r0 = smb_available_up ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
        end_x_r0 = smb_available_upright ? smb_pix_width : (smb_pix_width - 1);
        start_x_r = smb_available_left ? 0 : 1;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
        start_x_rn = smb_available_leftdown ? 0 : 1;
        end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;

        end_x_r0_16 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x0f);
        end_x_r_16 = end_x_r - ((end_x_r - start_x_r) & 0x0f);
        end_x_rn_16 = end_x_rn - ((end_x_rn - start_x_rn) & 0x0f);

        //first row
        for (x = start_x_r0; x < end_x_r0; x += 16) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 1]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 1]);

            CALCU_EO_10BIT;

            if (x != end_x_r0_16) {
                _mm256_storeu_si256((__m256i*)(dst + x), t0);
            }
            else {
                mask_width = end_x_r0 - end_x_r0_16;
                mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
                if (mask_width > 8)
                {
                    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                    _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 8));
                }
                else
                {
                    _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                }
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_width = end_x_r - end_x_r_16;
        mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
        mask_low = mask_width > 8;
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 16) {
                s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 1]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 1]);

                CALCU_EO_10BIT;

                if (x != end_x_r_16) {
                    _mm256_storeu_si256((__m256i*)(dst + x), t0);
                }
                else {
                    if (mask_low)
                    {
                        _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                        _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 8));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        for (x = start_x_rn; x < end_x_rn; x += 16) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 1]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 1]);

            CALCU_EO_10BIT;

            if (x != end_x_rn_16) {
                _mm256_storeu_si256((__m256i*)(dst + x), t0);
            }
            else {
                mask_width = end_x_rn - end_x_rn_16;
                mask = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
                if (mask_width > 8)
                {
                    _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                    _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 8));
                }
                else
                {
                    _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                }
                break;
            }
        }
        break;
    }
    case SAO_TYPE_BO: {
        __m256i r0, r1, r2, r3, off0, off1, off2, off3;
        __m256i t0, t1, t2, t3, src0, src1;
        __m128i mask = _mm_setzero_si128();
        //__m256i shift_mask = _mm256_set1_epi8(31);
        int shift_bo = sample_bit_depth - NUM_SAO_BO_CLASSES_IN_BIT;
        int mask_low;

        r0 = _mm256_set1_epi16(sao_params->bandIdx[0]);
        r1 = _mm256_set1_epi16(sao_params->bandIdx[1]);
        r2 = _mm256_set1_epi16(sao_params->bandIdx[2]);
        r3 = _mm256_set1_epi16(sao_params->bandIdx[3]);
        off0 = _mm256_set1_epi16(sao_params->offset[0]);
        off1 = _mm256_set1_epi16(sao_params->offset[1]);
        off2 = _mm256_set1_epi16(sao_params->offset[2]);
        off3 = _mm256_set1_epi16(sao_params->offset[3]);

        mask = _mm_load_si128((__m128i*)(sao_mask_32[smb_pix_width & 0x07]));
        mask_low = smb_pix_width - (smb_pix_width / 16) * 16 > 8;
        for (y = 0; y < smb_pix_height; y++) {
            for (x = 0; x < smb_pix_width; x += 16) {
                src0 = _mm256_loadu_si256((__m256i*)&src[x]);
                src1 = _mm256_srli_epi16(src0, shift_bo);
                //src1 = _mm256_and_si256(src1, shift_mask);

                t0 = _mm256_cmpeq_epi16(src1, r0);
                t1 = _mm256_cmpeq_epi16(src1, r1);
                t2 = _mm256_cmpeq_epi16(src1, r2);
                t3 = _mm256_cmpeq_epi16(src1, r3);

                t0 = _mm256_and_si256(t0, off0);
                t1 = _mm256_and_si256(t1, off1);
                t2 = _mm256_and_si256(t2, off2);
                t3 = _mm256_and_si256(t3, off3);
                t0 = _mm256_or_si256(t0, t1);
                t2 = _mm256_or_si256(t2, t3);
                t0 = _mm256_or_si256(t0, t2);

                t0 = _mm256_adds_epi16(src0, t0);
                t0 = _mm256_min_epi16(t0, max_val);
                t0 = _mm256_max_epi16(t0, min_val);

                if (smb_pix_width - x >= 16) {
                    _mm256_storeu_si256((__m256i*)(dst + x), t0);
                }
                else {
                    if (mask_low)
                    {
                        _mm_storeu_si128((__m128i*)(dst + x), _mm256_castsi256_si128(t0));
                        _mm_maskmoveu_si128(_mm256_extracti128_si256(t0, 1), mask, (s8*)(dst + x + 8));
                    }
                    else
                    {
                        _mm_maskmoveu_si128(_mm256_castsi256_si128(t0), mask, (s8*)(dst + x));
                    }
                }
            }
            dst += i_dst;
            src += i_src;
        }
        break;
    }
    default: {
        fprintf(stderr, "Not a supported SAO types\n");
        uavs3d_assert(0);
        exit(-1);
    }
    }
}

void uavs3d_sao_on_lcu_chroma_avx2(pel *src, int i_src, pel *dst, int i_dst, com_sao_param_t *sao_params, int smb_pix_height, int smb_pix_width,
    int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int sample_bit_depth)
{
    int type;
    int start_x, end_x, start_y, end_y;
    int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
    int x, y;
    int reg = 0;

    int smb_available_upleft = (smb_available_up && smb_available_left);
    int smb_available_upright = (smb_available_up && smb_available_right);
    int smb_available_leftdown = (smb_available_down && smb_available_left);
    int smb_available_rightdown = (smb_available_down && smb_available_right);
    __m256i min_val = _mm256_setzero_si256();
    __m256i max_val = _mm256_set1_epi16((1 << sample_bit_depth) - 1);
    __m256i mask_uv = _mm256_set1_epi32(0xffff);

    type = sao_params->type;

    smb_pix_width <<= 1;

    switch (type) {
    case SAO_TYPE_EO_0: {
        __m256i s0, s1, s2, s3, s4, s5;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask0, mask1, mask2;
        __m256i off0, off1, off2, off3, off4;
        __m256i c0, c1, c2, c3, c4;
        __m256i mask;
        int end_x_32;
        int mask_low;

        c0 = _mm256_set1_epi16(-2);
        c1 = _mm256_set1_epi16(-1);
        c2 = _mm256_set1_epi16(0);
        c3 = _mm256_set1_epi16(1);
        c4 = _mm256_set1_epi16(2);

        off0 = _mm256_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm256_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm256_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm256_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm256_set1_epi16((pel)sao_params->offset[4]);

        start_x = smb_available_left ? 0 : 2;
        end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        end_x_32 = end_x - ((end_x - start_x) & 0x1f);

        mask0 = _mm_loadu_si128((__m128i*)(sao_mask_32[((end_x - end_x_32) >> 1) & 0x07]));
        mask_low = end_x - end_x_32 > 16;
        mask1 = _mm_unpacklo_epi16(mask0, _mm_setzero_si128());
        mask2 = _mm_unpackhi_epi16(mask0, _mm_setzero_si128());
        mask = _mm256_set_m128i(mask2, mask1);

        for (y = 0; y < smb_pix_height; y++) {
            for (x = start_x; x < end_x; x += 32) {
                s0 = _mm256_lddqu_si256((__m256i*)&src[x - 2]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + 2]);
                s3 = _mm256_loadu_si256((__m256i*)&src[x + 14]);
                s4 = _mm256_loadu_si256((__m256i*)&src[x + 16]);
                s5 = _mm256_loadu_si256((__m256i*)&src[x + 18]);

                s0 = _mm256_and_si256(s0, mask_uv);
                s1 = _mm256_and_si256(s1, mask_uv);
                s2 = _mm256_and_si256(s2, mask_uv);
                s3 = _mm256_and_si256(s3, mask_uv);
                s4 = _mm256_and_si256(s4, mask_uv);
                s5 = _mm256_and_si256(s5, mask_uv);
                s0 = _mm256_packus_epi32(s0, s3);
                s1 = _mm256_packus_epi32(s1, s4);
                s2 = _mm256_packus_epi32(s2, s5);

                CALCU_EO_10BIT;

                t1 = _mm256_unpacklo_epi16(t0, min_val);
                t2 = _mm256_unpackhi_epi16(t0, min_val);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
                t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
                if (x != end_x_32) {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                }
                else {
                    if (mask_low)
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
    }
                        break;
    case SAO_TYPE_EO_90: {
        __m256i s0, s1, s2, s3, s4, s5;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask0, mask1, mask2;
        __m256i off0, off1, off2, off3, off4;
        __m256i c0, c1, c2, c3, c4;
        __m256i mask;
        int end_x_32 = smb_pix_width - (smb_pix_width & 0x1f);
        int mask_low;

        c0 = _mm256_set1_epi16(-2);
        c1 = _mm256_set1_epi16(-1);
        c2 = _mm256_set1_epi16(0);
        c3 = _mm256_set1_epi16(1);
        c4 = _mm256_set1_epi16(2);

        off0 = _mm256_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm256_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm256_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm256_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm256_set1_epi16((pel)sao_params->offset[4]);

        start_y = smb_available_up ? 0 : 1;
        end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1);

        dst += start_y * i_dst;
        src += start_y * i_src;

        mask0 = _mm_loadu_si128((__m128i*)(sao_mask_32[(smb_pix_width >> 1) & 0x07]));
        mask_low = (smb_pix_width & 0x1f) > 16;
        mask1 = _mm_unpacklo_epi16(mask0, _mm_setzero_si128());
        mask2 = _mm_unpackhi_epi16(mask0, _mm_setzero_si128());
        mask = _mm256_set_m128i(mask2, mask1);

        for (y = start_y; y < end_y; y++) {
            for (x = 0; x < smb_pix_width; x += 32) {
                s0 = _mm256_lddqu_si256((__m256i*)&src[x - i_src]);
                s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 16]);
                s1 = _mm256_lddqu_si256((__m256i*)&src[x]);
                s4 = _mm256_loadu_si256((__m256i*)&src[x + 16]);
                s2 = _mm256_lddqu_si256((__m256i*)&src[x + i_src]);
                s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 16]);

                s0 = _mm256_and_si256(s0, mask_uv);
                s1 = _mm256_and_si256(s1, mask_uv);
                s2 = _mm256_and_si256(s2, mask_uv);
                s3 = _mm256_and_si256(s3, mask_uv);
                s4 = _mm256_and_si256(s4, mask_uv);
                s5 = _mm256_and_si256(s5, mask_uv);
                s0 = _mm256_packus_epi32(s0, s3);
                s1 = _mm256_packus_epi32(s1, s4);
                s2 = _mm256_packus_epi32(s2, s5);

                CALCU_EO_10BIT;

                t1 = _mm256_unpacklo_epi16(t0, min_val);
                t2 = _mm256_unpackhi_epi16(t0, min_val);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
                t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
                if (x != end_x_32) {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                }
                else {
                    if (mask_low)
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
    }
                         break;
    case SAO_TYPE_EO_135: {
        __m256i s0, s1, s2, s3, s4, s5;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask0, mask1, mask2;
        __m256i off0, off1, off2, off3, off4;
        __m256i c0, c1, c2, c3, c4;
        __m256i mask;
        __m128i zero = _mm_setzero_si128();
        int end_x_r0_32, end_x_r_32, end_x_rn_32;
        int mask_low, mask_width;

        c0 = _mm256_set1_epi16(-2);
        c1 = _mm256_set1_epi16(-1);
        c2 = _mm256_set1_epi16(0);
        c3 = _mm256_set1_epi16(1);
        c4 = _mm256_set1_epi16(2);

        off0 = _mm256_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm256_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm256_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm256_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm256_set1_epi16((pel)sao_params->offset[4]);

        start_x_r0 = smb_available_upleft ? 0 : 2;
        end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 2)) : 2;
        start_x_r = smb_available_left ? 0 : 2;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        start_x_rn = smb_available_down ? (smb_available_left ? 0 : 2) : (smb_pix_width - 2);
        end_x_rn = smb_available_rightdown ? smb_pix_width : (smb_pix_width - 2);

        end_x_r0_32 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x1f);
        end_x_r_32 = end_x_r - ((end_x_r - start_x_r) & 0x1f);
        end_x_rn_32 = end_x_rn - ((end_x_rn - start_x_rn) & 0x1f);

        //first row
        for (x = start_x_r0; x < end_x_r0; x += 32) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 2]);
            s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 14]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s4 = _mm256_loadu_si256((__m256i*)&src[x + 16]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 2]);
            s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 18]);

            s0 = _mm256_and_si256(s0, mask_uv);
            s1 = _mm256_and_si256(s1, mask_uv);
            s2 = _mm256_and_si256(s2, mask_uv);
            s3 = _mm256_and_si256(s3, mask_uv);
            s4 = _mm256_and_si256(s4, mask_uv);
            s5 = _mm256_and_si256(s5, mask_uv);
            s0 = _mm256_packus_epi32(s0, s3);
            s1 = _mm256_packus_epi32(s1, s4);
            s2 = _mm256_packus_epi32(s2, s5);

            CALCU_EO_10BIT;

            t1 = _mm256_unpacklo_epi16(t0, min_val);
            t2 = _mm256_unpackhi_epi16(t0, min_val);
            t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
            t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
            if (x != end_x_r0_32) {
                t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                _mm256_storeu_si256((__m256i*)(dst + x), t1);
                _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
            }
            else {
                mask_width = (end_x_r0 - end_x_r0_32) >> 1;
                mask0 = _mm_loadu_si128((__m128i*)sao_mask_32[mask_width & 0x07]);
                mask1 = _mm_unpacklo_epi16(mask0, zero);
                mask2 = _mm_unpackhi_epi16(mask0, zero);
                mask = _mm256_set_m128i(mask2, mask1);
                if (mask_width > 8)
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                }
                else
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                }
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_width = (end_x_r - end_x_r_32) >> 1;
        mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
        mask_low = mask_width > 8;
        mask1 = _mm_unpacklo_epi16(mask0, zero);
        mask2 = _mm_unpackhi_epi16(mask0, zero);
        mask = _mm256_set_m128i(mask2, mask1);

        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 32) {
                s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 2]);
                s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 14]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s4 = _mm256_loadu_si256((__m256i*)&src[x + 16]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 2]);
                s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 18]);

                s0 = _mm256_and_si256(s0, mask_uv);
                s1 = _mm256_and_si256(s1, mask_uv);
                s2 = _mm256_and_si256(s2, mask_uv);
                s3 = _mm256_and_si256(s3, mask_uv);
                s4 = _mm256_and_si256(s4, mask_uv);
                s5 = _mm256_and_si256(s5, mask_uv);
                s0 = _mm256_packus_epi32(s0, s3);
                s1 = _mm256_packus_epi32(s1, s4);
                s2 = _mm256_packus_epi32(s2, s5);

                CALCU_EO_10BIT;

                t1 = _mm256_unpacklo_epi16(t0, min_val);
                t2 = _mm256_unpackhi_epi16(t0, min_val);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
                t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
                if (x != end_x_r_32) {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                }
                else {
                    if (mask_low)
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        //last row
        for (x = start_x_rn; x < end_x_rn; x += 32) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src - 2]);
            s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 14]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s4 = _mm256_loadu_si256((__m256i*)&src[x + 16]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 2]);
            s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 18]);

            s0 = _mm256_and_si256(s0, mask_uv);
            s1 = _mm256_and_si256(s1, mask_uv);
            s2 = _mm256_and_si256(s2, mask_uv);
            s3 = _mm256_and_si256(s3, mask_uv);
            s4 = _mm256_and_si256(s4, mask_uv);
            s5 = _mm256_and_si256(s5, mask_uv);
            s0 = _mm256_packus_epi32(s0, s3);
            s1 = _mm256_packus_epi32(s1, s4);
            s2 = _mm256_packus_epi32(s2, s5);

            CALCU_EO_10BIT;

            t1 = _mm256_unpacklo_epi16(t0, min_val);
            t2 = _mm256_unpackhi_epi16(t0, min_val);
            t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
            t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
            if (x != end_x_rn_32) {
                t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                _mm256_storeu_si256((__m256i*)(dst + x), t1);
                _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
            }
            else {
                mask_width = (end_x_rn - end_x_rn_32) >> 1;
                mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
                mask1 = _mm_unpacklo_epi16(mask0, zero);
                mask2 = _mm_unpackhi_epi16(mask0, zero);
                mask = _mm256_set_m128i(mask2, mask1);
                if (mask_width > 8)
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                }
                else
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                }
                break;
            }
        }
    }
                          break;
    case SAO_TYPE_EO_45: {
        __m256i s0, s1, s2, s3, s4, s5;
        __m256i t0, t1, t2, t3, t4, etype;
        __m128i mask0, mask1, mask2;
        __m256i off0, off1, off2, off3, off4;
        __m256i c0, c1, c2, c3, c4;
        __m256i mask;
        int end_x_r0_32, end_x_r_32, end_x_rn_32;
        int mask_width, mask_low;

        c0 = _mm256_set1_epi16(-2);
        c1 = _mm256_set1_epi16(-1);
        c2 = _mm256_set1_epi16(0);
        c3 = _mm256_set1_epi16(1);
        c4 = _mm256_set1_epi16(2);

        off0 = _mm256_set1_epi16((pel)sao_params->offset[0]);
        off1 = _mm256_set1_epi16((pel)sao_params->offset[1]);
        off2 = _mm256_set1_epi16((pel)sao_params->offset[2]);
        off3 = _mm256_set1_epi16((pel)sao_params->offset[3]);
        off4 = _mm256_set1_epi16((pel)sao_params->offset[4]);

        start_x_r0 = smb_available_up ? (smb_available_left ? 0 : 2) : (smb_pix_width - 2);
        end_x_r0 = smb_available_upright ? smb_pix_width : (smb_pix_width - 2);
        start_x_r = smb_available_left ? 0 : 2;
        end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 2);
        start_x_rn = smb_available_leftdown ? 0 : 2;
        end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 2)) : 2;

        end_x_r0_32 = end_x_r0 - ((end_x_r0 - start_x_r0) & 0x1f);
        end_x_r_32 = end_x_r - ((end_x_r - start_x_r) & 0x1f);
        end_x_rn_32 = end_x_rn - ((end_x_rn - start_x_rn) & 0x1f);

        //first row
        for (x = start_x_r0; x < end_x_r0; x += 32) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 2]);
            s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 18]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s4 = _mm256_loadu_si256((__m256i*)&src[x + 16]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 2]);
            s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 14]);

            s0 = _mm256_and_si256(s0, mask_uv);
            s1 = _mm256_and_si256(s1, mask_uv);
            s2 = _mm256_and_si256(s2, mask_uv);
            s3 = _mm256_and_si256(s3, mask_uv);
            s4 = _mm256_and_si256(s4, mask_uv);
            s5 = _mm256_and_si256(s5, mask_uv);
            s0 = _mm256_packus_epi32(s0, s3);
            s1 = _mm256_packus_epi32(s1, s4);
            s2 = _mm256_packus_epi32(s2, s5);

            CALCU_EO_10BIT;

            t1 = _mm256_unpacklo_epi16(t0, min_val);
            t2 = _mm256_unpackhi_epi16(t0, min_val);
            t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
            t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
            if (x != end_x_r0_32) {
                t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                _mm256_storeu_si256((__m256i*)(dst + x), t1);
                _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
            }
            else {
                mask_width = (end_x_r0 - end_x_r0_32) >> 1;
                mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
                mask1 = _mm_unpacklo_epi16(mask0, _mm_setzero_si128());
                mask2 = _mm_unpackhi_epi16(mask0, _mm_setzero_si128());
                mask = _mm256_set_m128i(mask2, mask1);
                if (mask_width > 8)
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                }
                else
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                }
                break;
            }
        }
        dst += i_dst;
        src += i_src;

        mask_width = (end_x_r - end_x_r_32) >> 1;
        mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
        mask_low = mask_width > 8;
        mask1 = _mm_unpacklo_epi16(mask0, _mm_setzero_si128());
        mask2 = _mm_unpackhi_epi16(mask0, _mm_setzero_si128());
        mask = _mm256_set_m128i(mask2, mask1);
        //middle rows
        for (y = 1; y < smb_pix_height - 1; y++) {
            for (x = start_x_r; x < end_x_r; x += 32) {
                s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 2]);
                s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 18]);
                s1 = _mm256_loadu_si256((__m256i*)&src[x]);
                s4 = _mm256_loadu_si256((__m256i*)&src[x + 16]);
                s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 2]);
                s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 14]);

                s0 = _mm256_and_si256(s0, mask_uv);
                s1 = _mm256_and_si256(s1, mask_uv);
                s2 = _mm256_and_si256(s2, mask_uv);
                s3 = _mm256_and_si256(s3, mask_uv);
                s4 = _mm256_and_si256(s4, mask_uv);
                s5 = _mm256_and_si256(s5, mask_uv);
                s0 = _mm256_packus_epi32(s0, s3);
                s1 = _mm256_packus_epi32(s1, s4);
                s2 = _mm256_packus_epi32(s2, s5);

                CALCU_EO_10BIT;

                t1 = _mm256_unpacklo_epi16(t0, min_val);
                t2 = _mm256_unpackhi_epi16(t0, min_val);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
                t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
                if (x != end_x_r_32) {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                }
                else {
                    if (mask_low)
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
                    break;
                }
            }
            dst += i_dst;
            src += i_src;
        }
        for (x = start_x_rn; x < end_x_rn; x += 32) {
            s0 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 2]);
            s3 = _mm256_loadu_si256((__m256i*)&src[x - i_src + 18]);
            s1 = _mm256_loadu_si256((__m256i*)&src[x]);
            s4 = _mm256_loadu_si256((__m256i*)&src[x + 16]);
            s2 = _mm256_loadu_si256((__m256i*)&src[x + i_src - 2]);
            s5 = _mm256_loadu_si256((__m256i*)&src[x + i_src + 14]);

            s0 = _mm256_and_si256(s0, mask_uv);
            s1 = _mm256_and_si256(s1, mask_uv);
            s2 = _mm256_and_si256(s2, mask_uv);
            s3 = _mm256_and_si256(s3, mask_uv);
            s4 = _mm256_and_si256(s4, mask_uv);
            s5 = _mm256_and_si256(s5, mask_uv);
            s0 = _mm256_packus_epi32(s0, s3);
            s1 = _mm256_packus_epi32(s1, s4);
            s2 = _mm256_packus_epi32(s2, s5);

            CALCU_EO_10BIT;

            t1 = _mm256_unpacklo_epi16(t0, min_val);
            t2 = _mm256_unpackhi_epi16(t0, min_val);
            t3 = _mm256_loadu_si256((__m256i*)&dst[x]);
            t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
            if (x != end_x_rn_32) {
                t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                _mm256_storeu_si256((__m256i*)(dst + x), t1);
                _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
            }
            else {
                mask_width = (end_x_rn - end_x_rn_32) >> 1;
                mask0 = _mm_load_si128((__m128i*)(sao_mask_32[mask_width & 0x07]));
                mask1 = _mm_unpacklo_epi16(mask0, _mm_setzero_si128());
                mask2 = _mm_unpackhi_epi16(mask0, _mm_setzero_si128());
                mask = _mm256_set_m128i(mask2, mask1);
                if (mask_width > 8)
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                }
                else
                {
                    t1 = _mm256_blendv_epi8(t3, t1, mask);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                }
                break;
            }
        }
    }
                         break;
    case SAO_TYPE_BO: {
        __m256i r0, r1, r2, r3, off0, off1, off2, off3;
        __m256i t0, t1, t2, t3, t4, src0, src1, src2;
        __m128i mask0, mask1, mask2;
        __m256i mask;
        int shift_bo = sample_bit_depth - NUM_SAO_BO_CLASSES_IN_BIT;
        int mask_low;

        r0 = _mm256_set1_epi16(sao_params->bandIdx[0]);
        r1 = _mm256_set1_epi16(sao_params->bandIdx[1]);
        r2 = _mm256_set1_epi16(sao_params->bandIdx[2]);
        r3 = _mm256_set1_epi16(sao_params->bandIdx[3]);
        off0 = _mm256_set1_epi16(sao_params->offset[0]);
        off1 = _mm256_set1_epi16(sao_params->offset[1]);
        off2 = _mm256_set1_epi16(sao_params->offset[2]);
        off3 = _mm256_set1_epi16(sao_params->offset[3]);

        mask0 = _mm_load_si128((__m128i*)(sao_mask_32[(smb_pix_width >> 1) & 0x07]));
        mask_low = smb_pix_width - (smb_pix_width / 32) * 32 > 16;
        mask1 = _mm_unpacklo_epi16(mask0, _mm_setzero_si128());
        mask2 = _mm_unpackhi_epi16(mask0, _mm_setzero_si128());
        mask = _mm256_set_m128i(mask2, mask1);
        for (y = 0; y < smb_pix_height; y++) {
            for (x = 0; x < smb_pix_width; x += 32) {
                src0 = _mm256_loadu_si256((__m256i*)&src[x]);
                src2 = _mm256_loadu_si256((__m256i*)&src[x + 16]);
                src0 = _mm256_and_si256(src0, mask_uv);
                src2 = _mm256_and_si256(src2, mask_uv);
                src0 = _mm256_packus_epi32(src0, src2);

                src1 = _mm256_srli_epi16(src0, shift_bo);

                t0 = _mm256_cmpeq_epi16(src1, r0);
                t1 = _mm256_cmpeq_epi16(src1, r1);
                t2 = _mm256_cmpeq_epi16(src1, r2);
                t3 = _mm256_cmpeq_epi16(src1, r3);

                t0 = _mm256_and_si256(t0, off0);
                t1 = _mm256_and_si256(t1, off1);
                t2 = _mm256_and_si256(t2, off2);
                t3 = _mm256_and_si256(t3, off3);
                t0 = _mm256_or_si256(t0, t1);
                t2 = _mm256_or_si256(t2, t3);
                t0 = _mm256_or_si256(t0, t2);

                t0 = _mm256_adds_epi16(src0, t0);
                t0 = _mm256_min_epi16(t0, max_val);
                t0 = _mm256_max_epi16(t0, min_val);

                t1 = _mm256_unpacklo_epi16(t0, min_val);
                t2 = _mm256_unpackhi_epi16(t0, min_val);
                t3 = _mm256_loadu_si256((__m256i*)&dst[x]);

                if (smb_pix_width - x >= 32) {
                    t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
                    t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                    t2 = _mm256_blendv_epi8(t4, t2, mask_uv);
                    _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                }
                else {
                    if (mask_low)
                    {
                        t4 = _mm256_loadu_si256((__m256i*)&dst[x + 16]);
                        t1 = _mm256_blendv_epi8(t3, t1, mask_uv);
                        t2 = _mm256_blendv_epi8(t4, t2, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                        _mm256_storeu_si256((__m256i*)(dst + x + 16), t2);
                    }
                    else
                    {
                        t1 = _mm256_blendv_epi8(t3, t1, mask);
                        _mm256_storeu_si256((__m256i*)(dst + x), t1);
                    }
                    break;
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

#endif
