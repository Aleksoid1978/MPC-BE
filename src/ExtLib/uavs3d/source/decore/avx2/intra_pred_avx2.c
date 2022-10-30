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
#include "../sse/sse.h"

#if BIT_DEPTH == 8
void uavs3d_ipred_ver_avx2(pel *src, pel *dst, int i_dst, int width, int height)
{
    int y;
    switch (width) {
    case 4:
    {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        for (y = 0; y < height; y += 4) {
            CP32(dst, src);
            CP32(dst + i_dst, src);
            CP32(dst + i_dst2, src);
            CP32(dst + i_dst3, src);
            dst += i_dst4;
        }
        break;
    }
    case 8:
        for (y = 0; y < height; y += 2) {
            CP64(dst, src);
            CP64(dst + i_dst, src);
            dst += i_dst << 1;
        }
        break;
    case 16:
    {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        __m128i T0;
        T0 = _mm_loadu_si128((__m128i*)src);
        for (y = 0; y < height; y += 4) {
            _mm_storeu_si128((__m128i*)(dst), T0);
            _mm_storeu_si128((__m128i*)(dst + i_dst), T0);
            _mm_storeu_si128((__m128i*)(dst + i_dst2), T0);
            _mm_storeu_si128((__m128i*)(dst + i_dst3), T0);
            dst += i_dst4;
        }
        break;
    }
    case 32:
    {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        __m256i T0;
        T0 = _mm256_loadu_si256((__m256i*)(src));
        for (y = 0; y < height; y += 4) {
            _mm256_storeu_si256((__m256i*)(dst), T0);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), T0);
            _mm256_storeu_si256((__m256i*)(dst + i_dst2), T0);
            _mm256_storeu_si256((__m256i*)(dst + i_dst3), T0);
            dst += i_dst4;
        }
        break;
    }
    case 64:
    {
        __m256i T0, T1;
        int i_dst2 = i_dst << 1;
        T0 = _mm256_loadu_si256((__m256i*)(src));
        T1 = _mm256_loadu_si256((__m256i*)(src + 32));
        for (y = 0; y < height; y += 2) {
            _mm256_storeu_si256((__m256i*)(dst), T0);
            _mm256_storeu_si256((__m256i*)(dst + 32), T1);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), T0);
            _mm256_storeu_si256((__m256i*)(dst + i_dst + 32), T1);
            dst += i_dst2;
        }
        break;
    }
    default:
        uavs3d_assert(0);
        break;
    }
}

void uavs3d_ipred_hor_avx2(pel *src, pel *dst, int i_dst, int width, int height)
{
    int y;
    switch (width) {
    case 4:
    {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        for (y = 0; y < height; y += 4) {
            M32(dst) = 0x01010101 * src[-y];
            M32(dst + i_dst) = 0x01010101 * src[-y - 1];
            M32(dst + i_dst2) = 0x01010101 * src[-y - 2];
            M32(dst + i_dst3) = 0x01010101 * src[-y - 3];
            dst += i_dst4;
        }
        break;
    }
    case 8:
    {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        for (y = 0; y < height; y += 4) {
            M64(dst) = 0x0101010101010101 * src[-y];
            M64(dst + i_dst) = 0x0101010101010101 * src[-y - 1];
            M64(dst + i_dst2) = 0x0101010101010101 * src[-y - 2];
            M64(dst + i_dst3) = 0x0101010101010101 * src[-y - 3];
            dst += i_dst4;
        }
        break;
    }
    case 16:
    {
        __m128i T0, T1, T2, T3;
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        for (y = 0; y < height; y += 4) {
            T0 = _mm_set1_epi8((char)src[-y]);
            T1 = _mm_set1_epi8((char)src[-y - 1]);
            T2 = _mm_set1_epi8((char)src[-y - 2]);
            T3 = _mm_set1_epi8((char)src[-y - 3]);
            _mm_storeu_si128((__m128i*)(dst), T0);
            _mm_storeu_si128((__m128i*)(dst + i_dst), T1);
            _mm_storeu_si128((__m128i*)(dst + i_dst2), T2);
            _mm_storeu_si128((__m128i*)(dst + i_dst3), T3);
            dst += i_dst4;
        }
        break;
    }
    case 32:
    {
        __m256i T0, T1;
        int i_dst2 = i_dst << 1;
        for (y = 0; y < height; y += 2) {
            T0 = _mm256_set1_epi8((char)src[-y]);
            T1 = _mm256_set1_epi8((char)src[-y - 1]);
            _mm256_storeu_si256((__m256i*)(dst), T0);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), T1);
            dst += i_dst2;
        }
        break;
    }
    case 64:
    {
        __m256i T0, T1;
        int i_dst2 = i_dst << 1;
        for (y = 0; y < height; y += 2) {
            T0 = _mm256_set1_epi8((char)src[-y]);
            T1 = _mm256_set1_epi8((char)src[-y - 1]);
            _mm256_storeu_si256((__m256i*)(dst), T0);
            _mm256_storeu_si256((__m256i*)(dst + 32), T0);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), T1);
            _mm256_storeu_si256((__m256i*)(dst + i_dst + 32), T1);
            dst += i_dst2;
        }
        break;
    }
    default:
        uavs3d_assert(0);
        break;
    }
}

void uavs3d_ipred_dc_avx2(pel *src, pel *dst, int i_dst, int width, int height, u16 avail_cu, int bit_depth)
{
	int   x, y;
	int   dc;
	pel  *p_src = src - 1;
    int left_avail = IS_AVAIL(avail_cu, AVAIL_LE);
    int above_avail = IS_AVAIL(avail_cu, AVAIL_UP);

    if (left_avail && above_avail) {
        int length = width + height + 1;
        int i;
        __m128i sum = _mm_setzero_si128();
        __m128i val;

        p_src = src - height;

        for (i = 0; i < length - 7; i += 8) {
            val = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(p_src + i)));
            sum = _mm_add_epi16(sum, val);
        }
        if (i < length) {
            int left_pixels = length - i;
            __m128i mask = _mm_load_si128((__m128i*)(uavs3d_simd_mask[(left_pixels << 1) - 1]));
            val = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(p_src + i)));
            val = _mm_and_si128(val, mask);
            sum = _mm_add_epi16(sum, val);
        }
        sum = _mm_add_epi16(sum, _mm_srli_si128(sum, 8));
        sum = _mm_add_epi16(sum, _mm_srli_si128(sum, 4));
        sum = _mm_add_epi16(sum, _mm_srli_si128(sum, 2));

        dc = _mm_extract_epi16(sum, 0) + ((width + height) >> 1) - src[0];

        dc = (dc * (4096 / (width + height))) >> 12;

    } else if (left_avail) {
        dc = 0;
		for (y = 0; y < height; y++) {
			dc += p_src[-y];
		}
        dc += height / 2;
        dc /= height;
	} else {
		p_src = src + 1;
        dc = 0;
		if (above_avail) {
			for (x = 0; x < width; x++) {
				dc += p_src[x];
			}
			dc += width / 2;
			dc /= width;
		} else {
			dc = 1 << (bit_depth - 1);
		}
	}

    switch (width) {
    case 4:
    {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        u32 v32 = 0x01010101 * dc;
        for (y = 0; y < height; y += 4) {
            M32(dst) = v32;
            M32(dst + i_dst) = v32;
            M32(dst + i_dst2) = v32;
            M32(dst + i_dst3) = v32;
            dst += i_dst4;
        }
        break;
    }
    case 8:
    {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        u64 v64 = 0x0101010101010101 * dc;
        for (y = 0; y < height; y += 4) {
            M64(dst) = v64;
            M64(dst + i_dst) = v64;
            M64(dst + i_dst2) = v64;
            M64(dst + i_dst3) = v64;
            dst += i_dst4;
        }
        break;
    }
    case 16:
    {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        __m128i T = _mm_set1_epi8((s8)dc);
        for (y = 0; y < height; y += 4) {
            _mm_storeu_si128((__m128i*)(dst), T);
            _mm_storeu_si128((__m128i*)(dst + i_dst), T);
            _mm_storeu_si128((__m128i*)(dst + i_dst2), T);
            _mm_storeu_si128((__m128i*)(dst + i_dst3), T);
            dst += i_dst4;
        }
        break;
    }
    case 32:
    {
        __m256i T = _mm256_set1_epi8((s8)dc);
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        for (y = 0; y < height; y += 4) {
            _mm256_storeu_si256((__m256i*)(dst), T);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), T);
            _mm256_storeu_si256((__m256i*)(dst + i_dst2), T);
            _mm256_storeu_si256((__m256i*)(dst + i_dst3), T);
            dst += i_dst4;
        }
        break;
    }
    case 64:
    {
        int i_dst2 = i_dst << 1;
        __m256i T = _mm256_set1_epi8((s8)dc);
        for (y = 0; y < height; y += 2) {
            _mm256_storeu_si256((__m256i*)(dst), T);
            _mm256_storeu_si256((__m256i*)(dst + 32), T);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), T);
            _mm256_storeu_si256((__m256i*)(dst + i_dst + 32), T);
            dst += i_dst2;
        }
        break;
    }
    default:
        uavs3d_assert(0);
        break;
    }
}

static tab_s8 uavs3d_tab_auc_dir_dxdy[2][IPD_CNT][2] = {
    {
        // dx/dy
        { 0, 0 },{ 0, 0 },{ 0, 0 },{ 11, 2 },{ 2, 0 },
        { 11, 3 },{ 1, 0 },{ 93, 7 },{ 1, 1 },{ 93, 8 },
        { 1, 2 },{ 1, 3 },{ 0, 0 },{ 1, 3 },{ 1, 2 },
        { 93, 8 },{ 1, 1 },{ 93, 7 },{ 1, 0 },{ 11, 3 },
        { 2, 0 },{ 11, 2 },{ 4, 0 },{ 8, 0 },{ 0, 0 },
        { 8, 0 },{ 4, 0 },{ 11, 2 },{ 2, 0 },{ 11, 3 },
        { 1, 0 },{ 93, 7 },{ 1, 1 },
    },
    {
        // dy/dx
        { 0, 0 },{ 0, 0 },{ 0, 0 },{ 93, 8 },{ 1, 1 },
        { 93, 7 },{ 1, 0 },{ 11, 3 },{ 2, 0 },{ 11, 2 },
        { 4, 0 },{ 8, 0 },{ 0, 0 },{ 8, 0 },{ 4, 0 },
        { 11, 2 },{ 2, 0 },{ 11, 3 },{ 1, 0 },{ 93, 7 },
        { 1, 1 },{ 93, 8 },{ 1, 2 },{ 1, 3 },{ 0, 0 },
        { 1, 3 },{ 1, 2 },{ 93, 8 },{ 1, 1 },{ 93, 7 },
        { 1, 0 },{ 11, 3 },{ 2, 0 }
    }
};
static u8 uavs3d_always_inline get_context_pixel(int mode, int xyflag, int d, u8 *offset)
{
    int imult = uavs3d_tab_auc_dir_dxdy[xyflag][mode][0];
    int ishift = uavs3d_tab_auc_dir_dxdy[xyflag][mode][1];
    int dn = d * imult >> ishift;
    *offset = (u8)(((d * imult * 32) >> ishift) - dn * 32);
    return (u8)dn;
}

static u8 uavs3d_ipred_offsets[IPD_CNT - 3][2][64];
static u8 uavs3d_ipred_steps[IPD_CNT - 3][2][64];
void uavs3d_ipred_offsets_seteps_init() {
    int mode, i;

    for (mode = 3; mode < IPD_CNT; mode++) {
        for (i = 0; i < 64; i++) {
            uavs3d_ipred_steps[mode - 3][0][i] = get_context_pixel(mode, 0, i + 1, &uavs3d_ipred_offsets[mode - 3][0][i]);
            uavs3d_ipred_steps[mode - 3][1][i] = get_context_pixel(mode, 1, i + 1, &uavs3d_ipred_offsets[mode - 3][1][i]);
        }
    }
}

void uavs3d_ipred_ang_x_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    if (width == 4) {
        int i, j;
        int width2 = width << 1;

        for (j = 0; j < height; j++) {
            int c0, c1, c2, c3;
            int idx = uavs3d_ipred_steps[mode - 3][0][j]; 
            int offset = uavs3d_ipred_offsets[mode - 3][0][j];
            int pred_width = COM_MIN(width, width2 - idx + 1);
            
            c0 = 32 - offset;
            c1 = 64 - offset;
            c2 = 32 + offset;
            c3 = offset;

            for (i = 0; i < pred_width; i++, idx++) {
                dst[i] = (src[idx] * c0 + src[idx + 1] * c1 + src[idx + 2] * c2 + src[idx + 3] * c3 + 64) >> 7;
            }
            if (pred_width <= 0) {
                dst[0] = (src[width2] * c0 + src[width2 + 1] * c1 + src[width2 + 2] * c2 + src[width2 + 3] * c3 + 64) >> 7;
                pred_width = 1;
            }
            for (i = pred_width; i < width; i++) {
                dst[i] = dst[pred_width - 1];
            }
            dst += i_dst;
        }
    }
    else if (width == 8) {
        __m128i off = _mm_set1_epi16(64);
        __m256i mSwitch = _mm256_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6, 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
        int j, i;
        for (j = 0; j < height; j++) {
            int idx = uavs3d_ipred_steps[mode - 3][0][j];
            int offset = uavs3d_ipred_offsets[mode - 3][0][j];
            pel *psrc = src + idx;
            int c0 = 32 - offset;
            int c1 = 64 - offset;
            int c2 = 32 + offset;
            int c3 = offset;
            int pred_width = COM_MIN(8, 16 - idx + 1);

            if (pred_width <= 0) {
                dst[0] = (src[16] * c0 + src[16 + 1] * c1 + src[16 + 2] * c2 + src[16 + 3] * c3 + 64) >> 7;
                pred_width = 1;
            }
            else {
                int coef = ((c3 << 24)) | (c2 << 16) | (c1 << 8) | c0;
                __m256i C = _mm256_set1_epi32(coef);
                __m256i mSrc, T;
                __m128i m0, m1;

                mSrc = _mm256_set_m128i(_mm_loadl_epi64((__m128i*)(psrc + 4)), _mm_loadl_epi64((__m128i*)(psrc)));

                T = _mm256_shuffle_epi8(mSrc, mSwitch);
                T = _mm256_maddubs_epi16(T, C);
                m0 = _mm256_castsi256_si128(T);
                m1 = _mm256_extracti128_si256(T, 1);
                m0 = _mm_hadd_epi16(m0, m1);
                m0 = _mm_add_epi16(m0, off);
                m0 = _mm_srai_epi16(m0, 7);
                m0 = _mm_packus_epi16(m0, m0);
                _mm_storel_epi64((__m128i*)dst, m0);
            }
            for (i = pred_width; i < width; i++) {
                dst[i] = dst[pred_width - 1];
            }
            dst += i_dst;
        }
    }
    else { // x 16
        __m256i off = _mm256_set1_epi16(64);
        __m256i mSwitch0 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
        __m256i mSwitch1 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
        int j, col;
        for (j = 0; j < height; j++) {
            int idx = uavs3d_ipred_steps[mode - 3][0][j];
            int offset = uavs3d_ipred_offsets[mode - 3][0][j];
            int width2 = width << 1;
            pel *psrc = src + idx;
            int c0 = 32 - offset;
            int c1 = 64 - offset;
            int c2 = 32 + offset;
            int c3 = offset;
            int pred_width = COM_MIN(width, width2 - idx + 1);
            if (pred_width <= 0) {
                dst[0] = (src[width2] * c0 + src[width2 + 1] * c1 + src[width2 + 2] * c2 + src[width2 + 3] * c3 + 64) >> 7;
                pred_width = 1;
            }
            else {
                int coef0 = ((64 - offset) << 8) | (32 - offset);
                int coef1 = (offset << 8) | (32 + offset);
                __m256i C0 = _mm256_set1_epi16(coef0);
                __m256i C1 = _mm256_set1_epi16(coef1);
                __m128i m0, m1;

                for (col = 0; col < pred_width; col += 16) {
                    __m256i mSrc0 = _mm256_set_m128i(_mm_loadu_si128((__m128i*)(psrc + col + 8)), _mm_loadu_si128((__m128i*)(psrc + col)));
                    __m256i T0, T1;

                    T0 = _mm256_shuffle_epi8(mSrc0, mSwitch0);
                    T1 = _mm256_shuffle_epi8(mSrc0, mSwitch1);
                    T0 = _mm256_maddubs_epi16(T0, C0);
                    T1 = _mm256_maddubs_epi16(T1, C1);
                    T0 = _mm256_add_epi16(T0, T1);
                    T0 = _mm256_add_epi16(T0, off);
                    T0 = _mm256_srai_epi16(T0, 7);
                    m0 = _mm256_castsi256_si128(T0);
                    m1 = _mm256_extracti128_si256(T0, 1);
                    m0 = _mm_packus_epi16(m0, m1);

                    _mm_storeu_si128((__m128i*)(dst + col), m0);
                }
            }
            for (col = pred_width; col < width; col++) {
                dst[col] = dst[pred_width - 1];
            }
            dst += i_dst;
        }
    }
}

void uavs3d_ipred_ang_x_4_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    ALIGNED_32(pel first_line[64 + 128 + 32]);
    int line_size = width + (height - 1) * 2;
    int real_size = COM_MIN(line_size, width * 2 - 1);
    int height2 = height * 2;
    int i;
    __m256i zero = _mm256_setzero_si256();
    __m256i offset = _mm256_set1_epi16(2);

    src += 3;

    for (i = 0; i < real_size - 16; i += 32, src += 32) {
        __m256i S0 = _mm256_loadu_si256((__m256i *)(src - 1));
        __m256i S2 = _mm256_loadu_si256((__m256i *)(src + 1));
        __m256i S1 = _mm256_loadu_si256((__m256i *)(src));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m256i H0 = _mm256_unpackhi_epi8(S0, zero);
        __m256i H1 = _mm256_unpackhi_epi8(S1, zero);
        __m256i H2 = _mm256_unpackhi_epi8(S2, zero);

        __m256i sum1 = _mm256_add_epi16(L0, L1);
        __m256i sum2 = _mm256_add_epi16(L1, L2);
        __m256i sum3 = _mm256_add_epi16(H0, H1);
        __m256i sum4 = _mm256_add_epi16(H1, H2);

        sum1 = _mm256_add_epi16(sum1, sum2);
        sum3 = _mm256_add_epi16(sum3, sum4);

        sum1 = _mm256_add_epi16(sum1, offset);
        sum3 = _mm256_add_epi16(sum3, offset);

        sum1 = _mm256_srli_epi16(sum1, 2);
        sum3 = _mm256_srli_epi16(sum3, 2);

        sum1 = _mm256_packus_epi16(sum1, sum3);

        _mm256_storeu_si256((__m256i *)&first_line[i], sum1);
    }

    if (i < real_size) {
        __m128i z = _mm_setzero_si128();
        __m128i off = _mm_set1_epi16(2);
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src));

        __m128i L0 = _mm_unpacklo_epi8(S0, z);
        __m128i L1 = _mm_unpacklo_epi8(S1, z);
        __m128i L2 = _mm_unpacklo_epi8(S2, z);
        __m128i H0 = _mm_unpackhi_epi8(S0, z);
        __m128i H1 = _mm_unpackhi_epi8(S1, z);
        __m128i H2 = _mm_unpackhi_epi8(S2, z);

        __m128i sum1 = _mm_add_epi16(L0, L1);
        __m128i sum2 = _mm_add_epi16(L1, L2);
        __m128i sum3 = _mm_add_epi16(H0, H1);
        __m128i sum4 = _mm_add_epi16(H1, H2);

        sum1 = _mm_add_epi16(sum1, sum2);
        sum3 = _mm_add_epi16(sum3, sum4);

        sum1 = _mm_add_epi16(sum1, off);
        sum3 = _mm_add_epi16(sum3, off);

        sum1 = _mm_srli_epi16(sum1, 2);
        sum3 = _mm_srli_epi16(sum3, 2);

        sum1 = _mm_packus_epi16(sum1, sum3);

        _mm_storeu_si128((__m128i *)&first_line[i], sum1);
    }

    // padding
    for (i = real_size; i < line_size; i += 32) {
        __m256i pad = _mm256_set1_epi8((char)first_line[real_size - 1]);
        _mm256_storeu_si256((__m256i *)&first_line[i], pad);
    }

    switch (width) {
    case 4: {
        __m128i m0, m1;
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        for (i = 0; i < height2; i += 8) {
            m0 = _mm_loadu_si128((const __m128i*)(first_line + i));
            m1 = _mm_srli_si128(m0, 2);
            *((s32*)dst) = _mm_extract_epi32(m0, 0);
            *((s32*)(dst + i_dst)) = _mm_extract_epi32(m1, 0);
            *((s32*)(dst + i_dst2)) = _mm_extract_epi32(m0, 1);
            *((s32*)(dst + i_dst3)) = _mm_extract_epi32(m1, 1);

            dst += i_dst << 2;
        }
        break;
    }
    case 8: {
        __m128i m0, m1, m2, m3;
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        for (i = 0; i < height2; i += 8) {
            m0 = _mm_loadu_si128((const __m128i*)(first_line + i));
            m1 = _mm_srli_si128(m0, 2);
            m2 = _mm_srli_si128(m0, 4);
            m3 = _mm_srli_si128(m0, 6);
            _mm_storel_epi64((__m128i*)(dst), m0);
            _mm_storel_epi64((__m128i*)(dst + i_dst), m1);
            _mm_storel_epi64((__m128i*)(dst + i_dst2), m2);
            _mm_storel_epi64((__m128i*)(dst + i_dst3), m3);

            dst += i_dst << 2;
        }
        break;
    }
    case 16: {
        __m128i m0, m1, m2, m3;
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        pel* psrc = first_line;
        for (i = 0; i < height2; i += 8) {
            m0 = _mm_loadu_si128((const __m128i*)(psrc));
            m1 = _mm_loadu_si128((const __m128i*)(psrc + 2));
            m2 = _mm_loadu_si128((const __m128i*)(psrc + 4));
            m3 = _mm_loadu_si128((const __m128i*)(psrc + 6));
            _mm_storeu_si128((__m128i*)(dst), m0);
            _mm_storeu_si128((__m128i*)(dst + i_dst), m1);
            _mm_storeu_si128((__m128i*)(dst + i_dst2), m2);
            _mm_storeu_si128((__m128i*)(dst + i_dst3), m3);
            psrc += 8;
            dst += i_dst << 2;
        }
        break;
    }
    case 32: {
        __m256i m0, m1, m2, m3;
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        pel* psrc = first_line;
        for (i = 0; i < height2; i += 8) {
            m0 = _mm256_loadu_si256((const __m256i*)(psrc));
            m1 = _mm256_loadu_si256((const __m256i*)(psrc + 2));
            m2 = _mm256_loadu_si256((const __m256i*)(psrc + 4));
            m3 = _mm256_loadu_si256((const __m256i*)(psrc + 6));
            _mm256_storeu_si256((__m256i*)(dst), m0);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), m1);
            _mm256_storeu_si256((__m256i*)(dst + i_dst2), m2);
            _mm256_storeu_si256((__m256i*)(dst + i_dst3), m3);
            psrc += 8;
            dst += i_dst << 2;
        }
        break;
    }
    case 64: {
        __m256i m0, m1, m2, m3, m4, m5, m6, m7;
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        pel* psrc = first_line;
        for (i = 0; i < height2; i += 8) {
            m0 = _mm256_loadu_si256((const __m256i*)(psrc));
            m1 = _mm256_loadu_si256((const __m256i*)(psrc + 2));
            m2 = _mm256_loadu_si256((const __m256i*)(psrc + 4));
            m3 = _mm256_loadu_si256((const __m256i*)(psrc + 6));
            m4 = _mm256_loadu_si256((const __m256i*)(psrc + 32));
            m5 = _mm256_loadu_si256((const __m256i*)(psrc + 34));
            m6 = _mm256_loadu_si256((const __m256i*)(psrc + 36));
            m7 = _mm256_loadu_si256((const __m256i*)(psrc + 38));
            _mm256_storeu_si256((__m256i*)(dst), m0);
            _mm256_storeu_si256((__m256i*)(dst + 32), m4);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), m1);
            _mm256_storeu_si256((__m256i*)(dst + 32 + i_dst), m5);
            _mm256_storeu_si256((__m256i*)(dst + i_dst2), m2);
            _mm256_storeu_si256((__m256i*)(dst + 32 + i_dst2), m6);
            _mm256_storeu_si256((__m256i*)(dst + i_dst3), m3);
            _mm256_storeu_si256((__m256i*)(dst + 32 + i_dst3), m7);
            psrc += 8;
            dst += i_dst << 2;
        }
        break;
    }
    }
}

void uavs3d_ipred_ang_x_6_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    ALIGNED_16(pel first_line[64 + 64]);
    int line_size = width + height - 1;
    int real_size = COM_MIN(line_size, width * 2);
    int i;
    __m256i zero = _mm256_setzero_si256();
    __m256i offset = _mm256_set1_epi16(2);
    src += 2;

    for (i = 0; i < real_size - 8; i += 16, src += 16) {
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);

        __m256i sum1 = _mm256_add_epi16(L0, L1);
        __m256i sum2 = _mm256_add_epi16(L1, L2);
        __m128i d0;

        sum1 = _mm256_add_epi16(sum1, sum2);

        sum1 = _mm256_add_epi16(sum1, offset);

        sum1 = _mm256_srli_epi16(sum1, 2);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1));

        _mm_store_si128((__m128i *)&first_line[i], d0);
    }

    if (i < real_size) {
        __m128i zero = _mm_setzero_si128();
        __m128i offset = _mm_set1_epi16(2);
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src));

        __m128i L0 = _mm_unpacklo_epi8(S0, zero);
        __m128i L1 = _mm_unpacklo_epi8(S1, zero);
        __m128i L2 = _mm_unpacklo_epi8(S2, zero);

        __m128i sum1 = _mm_add_epi16(L0, L1);
        __m128i sum2 = _mm_add_epi16(L1, L2);

        sum1 = _mm_add_epi16(sum1, sum2);
        sum1 = _mm_add_epi16(sum1, offset);
        sum1 = _mm_srli_epi16(sum1, 2);

        sum1 = _mm_packus_epi16(sum1, sum1);
        _mm_storel_epi64((__m128i *)&first_line[i], sum1);
    }

    // padding
    for (i = real_size; i < line_size; i += 16) {
        __m128i pad = _mm_set1_epi8((char)first_line[real_size - 1]);
        _mm_storeu_si128((__m128i *)&first_line[i], pad);
    }

#define COPY_X6(w) { \
    for (i = 0; i < height; i++) { \
        memcpy(dst, first_line + i, w * sizeof(pel)); \
        dst += i_dst;\
    } \
}
    switch (width) {
    case 4:
        COPY_X6(4)
            break;
    case 8:
        COPY_X6(8)
            break;
    case 16:
        COPY_X6(16)
            break;
    case 32:
        COPY_X6(32)
            break;
    case 64:
        COPY_X6(64)
            break;
    }
}

void uavs3d_ipred_ang_x_8_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    ALIGNED_32(pel first_line[2 * (64 + 48 + 32)]);
    int line_size = width + height / 2 - 1;
    int real_size = COM_MIN(line_size, width * 2 + 1);
    int i;
    int aligned_line_size = ((line_size + 31) >> 4) << 4;
    pel *pfirst[2];

    __m256i zero = _mm256_setzero_si256();
    __m256i coeff = _mm256_set1_epi16(3);
    __m256i offset1 = _mm256_set1_epi16(4);
    __m256i offset2 = _mm256_set1_epi16(2);
    int i_dst2 = i_dst * 2;

    pfirst[0] = first_line;
    pfirst[1] = first_line + aligned_line_size;

    for (i = 0; i < real_size - 16; i += 32, src += 32) {
        __m256i p01, p02, p11, p12;
        __m256i S0 = _mm256_loadu_si256((__m256i *)(src));
        __m256i S3 = _mm256_loadu_si256((__m256i *)(src + 3));
        __m256i S1 = _mm256_loadu_si256((__m256i *)(src + 1));
        __m256i S2 = _mm256_loadu_si256((__m256i *)(src + 2));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m256i L3 = _mm256_unpacklo_epi8(S3, zero);
        __m256i H0 = _mm256_unpackhi_epi8(S0, zero);
        __m256i H1 = _mm256_unpackhi_epi8(S1, zero);
        __m256i H2 = _mm256_unpackhi_epi8(S2, zero);
        __m256i H3 = _mm256_unpackhi_epi8(S3, zero);

        p01 = _mm256_add_epi16(L1, L2);
        p11 = _mm256_add_epi16(H1, H2);
        p01 = _mm256_mullo_epi16(p01, coeff);
        p11 = _mm256_mullo_epi16(p11, coeff);
        p02 = _mm256_add_epi16(L0, L3);
        p12 = _mm256_add_epi16(H0, H3);
        p02 = _mm256_add_epi16(p02, offset1);
        p12 = _mm256_add_epi16(p12, offset1);
        p01 = _mm256_add_epi16(p01, p02);
        p11 = _mm256_add_epi16(p11, p12);
        p01 = _mm256_srli_epi16(p01, 3);
        p11 = _mm256_srli_epi16(p11, 3);

        p01 = _mm256_packus_epi16(p01, p11);
        _mm256_storeu_si256((__m256i *)&pfirst[0][i], p01);

        p01 = _mm256_add_epi16(L1, L2);
        p02 = _mm256_add_epi16(L2, L3);
        p11 = _mm256_add_epi16(H1, H2);
        p12 = _mm256_add_epi16(H2, H3);

        p01 = _mm256_add_epi16(p01, p02);
        p11 = _mm256_add_epi16(p11, p12);

        p01 = _mm256_add_epi16(p01, offset2);
        p11 = _mm256_add_epi16(p11, offset2);

        p01 = _mm256_srli_epi16(p01, 2);
        p11 = _mm256_srli_epi16(p11, 2);

        p01 = _mm256_packus_epi16(p01, p11);
        _mm256_storeu_si256((__m256i *)&pfirst[1][i], p01);
    }

    if (i < real_size) {
        __m128i p01, p02, p11, p12;
        __m128i z = _mm_setzero_si128();
        __m128i coef = _mm_set1_epi16(3);
        __m128i off1 = _mm_set1_epi16(4);
        __m128i off2 = _mm_set1_epi16(2);
        __m128i S0 = _mm_loadu_si128((__m128i *)(src));
        __m128i S3 = _mm_loadu_si128((__m128i *)(src + 3));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 2));

        __m128i L0 = _mm_unpacklo_epi8(S0, z);
        __m128i L1 = _mm_unpacklo_epi8(S1, z);
        __m128i L2 = _mm_unpacklo_epi8(S2, z);
        __m128i L3 = _mm_unpacklo_epi8(S3, z);
        __m128i H0 = _mm_unpackhi_epi8(S0, z);
        __m128i H1 = _mm_unpackhi_epi8(S1, z);
        __m128i H2 = _mm_unpackhi_epi8(S2, z);
        __m128i H3 = _mm_unpackhi_epi8(S3, z);

        p01 = _mm_add_epi16(L1, L2);
        p11 = _mm_add_epi16(H1, H2);
        p01 = _mm_mullo_epi16(p01, coef);
        p11 = _mm_mullo_epi16(p11, coef);
        p02 = _mm_add_epi16(L0, L3);
        p12 = _mm_add_epi16(H0, H3);
        p02 = _mm_add_epi16(p02, off1);
        p12 = _mm_add_epi16(p12, off1);
        p01 = _mm_add_epi16(p01, p02);
        p11 = _mm_add_epi16(p11, p12);
        p01 = _mm_srli_epi16(p01, 3);
        p11 = _mm_srli_epi16(p11, 3);

        p01 = _mm_packus_epi16(p01, p11);
        _mm_storeu_si128((__m128i *)&pfirst[0][i], p01);

        p01 = _mm_add_epi16(L1, L2);
        p02 = _mm_add_epi16(L2, L3);
        p11 = _mm_add_epi16(H1, H2);
        p12 = _mm_add_epi16(H2, H3);

        p01 = _mm_add_epi16(p01, p02);
        p11 = _mm_add_epi16(p11, p12);

        p01 = _mm_add_epi16(p01, off2);
        p11 = _mm_add_epi16(p11, off2);

        p01 = _mm_srli_epi16(p01, 2);
        p11 = _mm_srli_epi16(p11, 2);

        p01 = _mm_packus_epi16(p01, p11);
        _mm_storeu_si128((__m128i *)&pfirst[1][i], p01);
    }

    // padding
    if (real_size < line_size) {
        __m128i pad1, pad2;
        pfirst[1][real_size - 1] = pfirst[1][real_size - 2];

        pad1 = _mm_set1_epi8((char)pfirst[0][real_size - 1]);
        pad2 = _mm_set1_epi8((char)pfirst[1][real_size - 1]);
        for (i = real_size; i < line_size; i += 16) {
            _mm_storeu_si128((__m128i *)&pfirst[0][i], pad1);
            _mm_storeu_si128((__m128i *)&pfirst[1][i], pad2);
        }
    }

    height /= 2;

    switch (width) {
    case 4: {
        int i_dst3 = i_dst + i_dst2;
        for (i = 0; i < height; i += 2) {
            *((s32*)dst) = *((s32*)(pfirst[0] + i));
            *((s32*)(dst + i_dst)) = *((s32*)(pfirst[1] + i));
            *((s32*)(dst + i_dst2)) = *((s32*)(pfirst[0] + i + 1));
            *((s32*)(dst + i_dst3)) = *((s32*)(pfirst[1] + i + 1));

            dst += i_dst << 2;
        }
        break;
    }
    case 8: {
        __m128i m0, m1, m2, m3;
        int i_dst3 = i_dst + i_dst2;
        for (i = 0; i < height; i += 2) {
            m0 = _mm_loadu_si128((const __m128i*)(pfirst[0] + i));
            m1 = _mm_loadu_si128((const __m128i*)(pfirst[1] + i));
            m2 = _mm_srli_si128(m0, 1);
            m3 = _mm_srli_si128(m1, 1);
            _mm_storel_epi64((__m128i*)(dst), m0);
            _mm_storel_epi64((__m128i*)(dst + i_dst), m1);
            _mm_storel_epi64((__m128i*)(dst + i_dst2), m2);
            _mm_storel_epi64((__m128i*)(dst + i_dst3), m3);

            dst += i_dst << 2;
        }
        break;
    }
    case 16: {
        __m128i m0, m1, m2, m3;
        int i_dst3 = i_dst + i_dst2;
        for (i = 0; i < height; i += 2) {
            m0 = _mm_loadu_si128((const __m128i*)(pfirst[0] + i));
            m1 = _mm_loadu_si128((const __m128i*)(pfirst[1] + i));
            m2 = _mm_loadu_si128((const __m128i*)(pfirst[0] + i + 1));
            m3 = _mm_loadu_si128((const __m128i*)(pfirst[1] + i + 1));
            _mm_storeu_si128((__m128i*)(dst), m0);
            _mm_storeu_si128((__m128i*)(dst + i_dst), m1);
            _mm_storeu_si128((__m128i*)(dst + i_dst2), m2);
            _mm_storeu_si128((__m128i*)(dst + i_dst3), m3);

            dst += i_dst << 2;
        }
        break;
    }
    case 32: {
        __m256i m0, m1, m2, m3;
        int i_dst3 = i_dst + i_dst2;
        for (i = 0; i < height; i += 2) {
            m0 = _mm256_loadu_si256((const __m256i*)(pfirst[0] + i));
            m1 = _mm256_loadu_si256((const __m256i*)(pfirst[1] + i));
            m2 = _mm256_loadu_si256((const __m256i*)(pfirst[0] + i + 1));
            m3 = _mm256_loadu_si256((const __m256i*)(pfirst[1] + i + 1));
            _mm256_storeu_si256((__m256i*)(dst), m0);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), m1);
            _mm256_storeu_si256((__m256i*)(dst + i_dst2), m2);
            _mm256_storeu_si256((__m256i*)(dst + i_dst3), m3);

            dst += i_dst << 2;
        }
        break;
    }
    case 64: {
        __m256i m0, m1, m2, m3, m4, m5, m6, m7;
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        for (i = 0; i < height; i += 2) {
            m0 = _mm256_loadu_si256((const __m256i*)(pfirst[0] + i));
            m1 = _mm256_loadu_si256((const __m256i*)(pfirst[1] + i));
            m2 = _mm256_loadu_si256((const __m256i*)(pfirst[0] + i + 1));
            m3 = _mm256_loadu_si256((const __m256i*)(pfirst[1] + i + 1));
            m4 = _mm256_loadu_si256((const __m256i*)(pfirst[0] + i + 32));
            m5 = _mm256_loadu_si256((const __m256i*)(pfirst[1] + i + 32));
            m6 = _mm256_loadu_si256((const __m256i*)(pfirst[0] + i + 33));
            m7 = _mm256_loadu_si256((const __m256i*)(pfirst[1] + i + 33));
            _mm256_storeu_si256((__m256i*)(dst), m0);
            _mm256_storeu_si256((__m256i*)(dst + 32), m4);
            _mm256_storeu_si256((__m256i*)(dst + i_dst), m1);
            _mm256_storeu_si256((__m256i*)(dst + 32 + i_dst), m5);
            _mm256_storeu_si256((__m256i*)(dst + i_dst2), m2);
            _mm256_storeu_si256((__m256i*)(dst + 32 + i_dst2), m6);
            _mm256_storeu_si256((__m256i*)(dst + i_dst3), m3);
            _mm256_storeu_si256((__m256i*)(dst + 32 + i_dst3), m7);

            dst += i_dst << 2;
        }
        break;
    }
    }
}

void uavs3d_ipred_ang_x_10_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i;
    pel *dst1 = dst;
    pel *dst2 = dst1 + i_dst;
    pel *dst3 = dst2 + i_dst;
    pel *dst4 = dst3 + i_dst;
    __m256i zero = _mm256_setzero_si256();
    __m256i coeff2 = _mm256_set1_epi16(2);
    __m256i coeff3 = _mm256_set1_epi16(3);
    __m256i coeff4 = _mm256_set1_epi16(4);
    __m256i coeff5 = _mm256_set1_epi16(5);
    __m256i coeff7 = _mm256_set1_epi16(7);
    __m256i coeff8 = _mm256_set1_epi16(8);

    ALIGNED_16(pel first_line[4 * (64 + 32)]);
    int line_size = width + height / 4 - 1;
    int aligned_line_size = ((line_size + 31) >> 4) << 4;
    pel *pfirst[4];
    int i_dstx4 = i_dst << 2;

    pfirst[0] = first_line;
    pfirst[1] = first_line + aligned_line_size;
    pfirst[2] = first_line + aligned_line_size * 2;
    pfirst[3] = first_line + aligned_line_size * 3;

    for (i = 0; i < line_size - 8; i += 16, src += 16) {
        __m256i p00, p10, p20, p30;
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 10), 0, *(s64 *)(src + 2));
        __m256i S3 = _mm256_set_epi64x(0, *(s64 *)(src + 11), 0, *(s64 *)(src + 3));
        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m256i L3 = _mm256_unpacklo_epi8(S3, zero);
        __m128i d0;

        p00 = _mm256_mullo_epi16(L0, coeff3);
        p10 = _mm256_mullo_epi16(L1, coeff7);
        p20 = _mm256_mullo_epi16(L2, coeff5);
        p30 = _mm256_add_epi16(L3, coeff8);
        p00 = _mm256_add_epi16(p00, p30);
        p00 = _mm256_add_epi16(p00, p10);
        p00 = _mm256_add_epi16(p00, p20);
        p00 = _mm256_srli_epi16(p00, 4);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_store_si128((__m128i *)&pfirst[0][i], d0);

        p00 = _mm256_add_epi16(L1, L2);
        p00 = _mm256_mullo_epi16(p00, coeff3);
        p10 = _mm256_add_epi16(L0, L3);
        p10 = _mm256_add_epi16(p10, coeff4);
        p00 = _mm256_add_epi16(p10, p00);
        p00 = _mm256_srli_epi16(p00, 3);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_store_si128((__m128i *)&pfirst[1][i], d0);

        p10 = _mm256_mullo_epi16(L1, coeff5);
        p20 = _mm256_mullo_epi16(L2, coeff7);
        p30 = _mm256_mullo_epi16(L3, coeff3);
        p00 = _mm256_add_epi16(L0, coeff8);
        p00 = _mm256_add_epi16(p00, p10);
        p00 = _mm256_add_epi16(p00, p20);
        p00 = _mm256_add_epi16(p00, p30);
        p00 = _mm256_srli_epi16(p00, 4);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_store_si128((__m128i *)&pfirst[2][i], d0);

        p00 = _mm256_add_epi16(L1, L2);
        p10 = _mm256_add_epi16(L2, L3);
        p00 = _mm256_add_epi16(p00, p10);
        p00 = _mm256_add_epi16(p00, coeff2);
        p00 = _mm256_srli_epi16(p00, 2);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_store_si128((__m128i *)&pfirst[3][i], d0);
    }

    if (i < line_size) {
        __m128i zero = _mm_setzero_si128();
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i coeff3 = _mm_set1_epi16(3);
        __m128i coeff4 = _mm_set1_epi16(4);
        __m128i coeff5 = _mm_set1_epi16(5);
        __m128i coeff7 = _mm_set1_epi16(7);
        __m128i coeff8 = _mm_set1_epi16(8);

        __m128i p00, p10, p20, p30;
        __m128i S0 = _mm_loadu_si128((__m128i *)(src));
        __m128i S3 = _mm_loadu_si128((__m128i *)(src + 3));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 2));

        __m128i L0 = _mm_unpacklo_epi8(S0, zero);
        __m128i L1 = _mm_unpacklo_epi8(S1, zero);
        __m128i L2 = _mm_unpacklo_epi8(S2, zero);
        __m128i L3 = _mm_unpacklo_epi8(S3, zero);

        p00 = _mm_mullo_epi16(L0, coeff3);
        p10 = _mm_mullo_epi16(L1, coeff7);
        p20 = _mm_mullo_epi16(L2, coeff5);
        p30 = _mm_add_epi16(L3, coeff8);
        p00 = _mm_add_epi16(p00, p30);
        p00 = _mm_add_epi16(p00, p10);
        p00 = _mm_add_epi16(p00, p20);
        p00 = _mm_srli_epi16(p00, 4);

        p00 = _mm_packus_epi16(p00, p00);
        _mm_storel_epi64((__m128i *)&pfirst[0][i], p00);

        p00 = _mm_add_epi16(L1, L2);
        p00 = _mm_mullo_epi16(p00, coeff3);
        p10 = _mm_add_epi16(L0, L3);
        p10 = _mm_add_epi16(p10, coeff4);
        p00 = _mm_add_epi16(p10, p00);
        p00 = _mm_srli_epi16(p00, 3);

        p00 = _mm_packus_epi16(p00, p00);
        _mm_storel_epi64((__m128i *)&pfirst[1][i], p00);

        p10 = _mm_mullo_epi16(L1, coeff5);
        p20 = _mm_mullo_epi16(L2, coeff7);
        p30 = _mm_mullo_epi16(L3, coeff3);
        p00 = _mm_add_epi16(L0, coeff8);
        p00 = _mm_add_epi16(p00, p10);
        p00 = _mm_add_epi16(p00, p20);
        p00 = _mm_add_epi16(p00, p30);
        p00 = _mm_srli_epi16(p00, 4);

        p00 = _mm_packus_epi16(p00, p00);
        _mm_storel_epi64((__m128i *)&pfirst[2][i], p00);

        p00 = _mm_add_epi16(L1, L2);
        p10 = _mm_add_epi16(L2, L3);
        p00 = _mm_add_epi16(p00, p10);
        p00 = _mm_add_epi16(p00, coeff2);
        p00 = _mm_srli_epi16(p00, 2);

        p00 = _mm_packus_epi16(p00, p00);
        _mm_storel_epi64((__m128i *)&pfirst[3][i], p00);
    }

    height >>= 2;

#define COPY_X10(w) { \
    for (i = 0; i < height; i++) { \
        memcpy(dst1, pfirst[0] + i, w * sizeof(pel)); \
        memcpy(dst2, pfirst[1] + i, w * sizeof(pel)); \
        memcpy(dst3, pfirst[2] + i, w * sizeof(pel)); \
        memcpy(dst4, pfirst[3] + i, w * sizeof(pel)); \
        dst1 += i_dstx4;\
        dst2 += i_dstx4;\
        dst3 += i_dstx4;\
        dst4 += i_dstx4;\
    } \
}
    switch (width) {
    case 4:
        for (i = 0; i < height; i++) {
            CP32(dst1, pfirst[0] + i);
            dst1 += i_dstx4;
            CP32(dst2, pfirst[1] + i);
            dst2 += i_dstx4;
            CP32(dst3, pfirst[2] + i);
            dst3 += i_dstx4;
            CP32(dst4, pfirst[3] + i);
            dst4 += i_dstx4;
        }
        break;
    case 8:
        for (i = 0; i < height; i++) {
            CP64(dst1, pfirst[0] + i);
            dst1 += i_dstx4;
            CP64(dst2, pfirst[1] + i);
            dst2 += i_dstx4;
            CP64(dst3, pfirst[2] + i);
            dst3 += i_dstx4;
            CP64(dst4, pfirst[3] + i);
            dst4 += i_dstx4;
        }
        break;
    case 16:
        for (i = 0; i < height; i++) {
            CP128(dst1, pfirst[0] + i);
            dst1 += i_dstx4;
            CP128(dst2, pfirst[1] + i);
            dst2 += i_dstx4;
            CP128(dst3, pfirst[2] + i);
            dst3 += i_dstx4;
            CP128(dst4, pfirst[3] + i);
            dst4 += i_dstx4;
        }
        break;
    case 32:
        COPY_X10(32)
            break;
    case 64:
        COPY_X10(64)
            break;
    default:
        uavs3d_assert(0);
        break;
    }
}

#define TRANS_4x4(t0) {                  \
    __m128i t1 = _mm_srli_si128(t0,  4); \
    __m128i t2 = _mm_srli_si128(t0,  8); \
    __m128i t3 = _mm_srli_si128(t0, 12); \
    t0 = _mm_unpacklo_epi8 (t0, t1);     \
    t2 = _mm_unpacklo_epi8 (t2, t3);     \
    t0 = _mm_unpacklo_epi16(t0, t2);     \
}

#define TRANS_8x4(t0, m0, m1) {             \
    __m256i t1;                             \
    __m128i m2, m3;                         \
    t1 = _mm256_srli_si256(t0, 8);          \
    t0 = _mm256_unpacklo_epi8(t0, t1);      \
    m2 = _mm256_castsi256_si128(t0);        \
    m3 = _mm256_extracti128_si256(t0, 1);   \
    m0 = _mm_unpacklo_epi16(m2, m3);        \
    m1 = _mm_unpackhi_epi16(m2, m3);        \
}

#define TRANS_8x8(t0246, t1357) {           \
    __m256i t0, t1, t2, t3;                 \
    t2 = _mm256_srli_si256(t0246, 8);       \
    t3 = _mm256_srli_si256(t1357, 8);       \
    t0 = _mm256_unpacklo_epi8(t0246, t1357); /*01 45*/     \
    t1 = _mm256_unpacklo_epi8(t2, t3);      /*23 67*/      \
    t2 = _mm256_unpacklo_epi16(t0, t1); /*(0123 4567)[0-3]*/            \
    t3 = _mm256_unpackhi_epi16(t0, t1); /*(0123 4567)[4-7]*/            \
    t0 = _mm256_permute2x128_si256(t2, t3, 0x20); /*(0123)[0-3] (0123)[4-7]*/ \
    t1 = _mm256_permute2x128_si256(t2, t3, 0x31); /*(4567)[0-3] (4567)[4-7]*/ \
    t0246 = _mm256_unpacklo_epi32(t0, t1);  /*(0-7)[0-1] (0-7)[4-5]*/   \
    t1357 = _mm256_unpackhi_epi32(t0, t1);     \
}

void uavs3d_ipred_ang_y_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    u8 *poffsets = uavs3d_ipred_offsets[mode - 3][1];
    u8 *psteps = uavs3d_ipred_steps[mode - 3][1];
    if (height == 4) {
        __m128i mAddOffset = _mm_set1_epi16(64);
        __m128i mSwitch = _mm_setr_epi8(3, 4, 5, 6, 2, 3, 4, 5, 1, 2, 3, 4, 0, 1, 2, 3);
        __m128i m_c_32 = _mm_set1_epi8(32);
        __m128i m_c_64 = _mm_set1_epi8(64);

        for (int i = 0; i < width; i += 4) {
            pel *p1 = src - psteps[i] - 6;
            pel *p2 = src - psteps[i + 1] - 6;
            pel *p3 = src - psteps[i + 2] - 6;
            pel *p4 = src - psteps[i + 3] - 6;
            __m128i m_c0 = _mm_loadl_epi64((const __m128i*)(poffsets + i));
            __m128i m_c1 = _mm_add_epi8(m_c0, m_c_32);
            __m128i m_c2 = _mm_sub_epi8(m_c_64, m_c0);
            __m128i m_c3 = _mm_sub_epi8(m_c_32, m_c0);
            m_c0 = _mm_unpacklo_epi8(m_c0, m_c1);
            m_c2 = _mm_unpacklo_epi8(m_c2, m_c3);
            m_c0 = _mm_unpacklo_epi16(m_c0, m_c2); 

            __m128i C1 = _mm_set1_epi32(_mm_extract_epi32(m_c0, 0));
            __m128i C2 = _mm_set1_epi32(_mm_extract_epi32(m_c0, 1));
            __m128i C3 = _mm_set1_epi32(_mm_extract_epi32(m_c0, 2));
            __m128i C4 = _mm_set1_epi32(_mm_extract_epi32(m_c0, 3));

            __m128i T1 = _mm_maddubs_epi16(_mm_shuffle_epi8(_mm_loadl_epi64((__m128i*)p1), mSwitch), C1);
            __m128i T2 = _mm_maddubs_epi16(_mm_shuffle_epi8(_mm_loadl_epi64((__m128i*)p2), mSwitch), C2);
            __m128i T3 = _mm_maddubs_epi16(_mm_shuffle_epi8(_mm_loadl_epi64((__m128i*)p3), mSwitch), C3);
            __m128i T4 = _mm_maddubs_epi16(_mm_shuffle_epi8(_mm_loadl_epi64((__m128i*)p4), mSwitch), C4);

            T1 = _mm_hadd_epi16(T1, T2); T1 = _mm_add_epi16(T1, mAddOffset); T1 = _mm_srai_epi16(T1, 7);
            T3 = _mm_hadd_epi16(T3, T4); T3 = _mm_add_epi16(T3, mAddOffset); T3 = _mm_srai_epi16(T3, 7);
            T1 = _mm_packus_epi16(T1, T3);

            TRANS_4x4(T1);

            pel *p = dst + i;

            *(int*)p = _mm_extract_epi32(T1, 0); p += i_dst;
            *(int*)p = _mm_extract_epi32(T1, 1); p += i_dst;
            *(int*)p = _mm_extract_epi32(T1, 2); p += i_dst;
            *(int*)p = _mm_extract_epi32(T1, 3);
        }
    }
    else { // height == 8x
        __m256i mOffset = _mm256_set1_epi16(64);
        if (width == 4) {
            __m256i mSwitch1 = _mm256_setr_epi8(7, 8, 6, 7, 5, 6, 4, 5, 3, 4, 2, 3, 1, 2, 0, 1, 7, 8, 6, 7, 5, 6, 4, 5, 3, 4, 2, 3, 1, 2, 0, 1);
            __m256i mSwitch2 = _mm256_setr_epi8(9, 10, 8, 9, 7, 8, 6, 7, 5, 6, 4, 5, 3, 4, 2, 3, 9, 10, 8, 9, 7, 8, 6, 7, 5, 6, 4, 5, 3, 4, 2, 3);
            __m128i m0, m1;

            int offset1 = poffsets[0];
            int offset2 = poffsets[1];
            int offset3 = poffsets[2];
            int offset4 = poffsets[3];
            pel *p1 = src - psteps[0] - 10;
            pel *p2 = src - psteps[1] - 10;
            pel *p3 = src - psteps[2] - 10;
            pel *p4 = src - psteps[3] - 10;

            __m128i C1_1 = _mm_set1_epi16(((32 + offset1) << 8) | offset1);
            __m128i C2_1 = _mm_set1_epi16(((32 + offset2) << 8) | offset2);
            __m128i C3_1 = _mm_set1_epi16(((32 + offset3) << 8) | offset3);
            __m128i C4_1 = _mm_set1_epi16(((32 + offset4) << 8) | offset4);
            __m128i C1_2 = _mm_set1_epi16(((32 - offset1) << 8) | (64 - offset1));
            __m128i C2_2 = _mm_set1_epi16(((32 - offset2) << 8) | (64 - offset2));
            __m128i C3_2 = _mm_set1_epi16(((32 - offset3) << 8) | (64 - offset3));
            __m128i C4_2 = _mm_set1_epi16(((32 - offset4) << 8) | (64 - offset4));

            __m256i C13_1 = _mm256_set_m128i(C3_1, C1_1);
            __m256i C24_1 = _mm256_set_m128i(C4_1, C2_1);
            __m256i C13_2 = _mm256_set_m128i(C3_2, C1_2);
            __m256i C24_2 = _mm256_set_m128i(C4_2, C2_2);

            for (int j = 0; j < height; j += 8) {
                __m256i mSrc13 = _mm256_loadu2_m128i((__m128i*)p3, (__m128i*)p1);
                __m256i mSrc24 = _mm256_loadu2_m128i((__m128i*)p4, (__m128i*)p2);
                __m256i T1, T2, T3, T4;

                T1 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc13, mSwitch1), C13_1);
                T2 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc24, mSwitch1), C24_1);
                T3 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc13, mSwitch2), C13_2);
                T4 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc24, mSwitch2), C24_2);

                T1 = _mm256_add_epi16(T1, T3);
                T2 = _mm256_add_epi16(T2, T4);

                T1 = _mm256_add_epi16(T1, mOffset);
                T2 = _mm256_add_epi16(T2, mOffset);
                T1 = _mm256_srai_epi16(T1, 7);
                T2 = _mm256_srai_epi16(T2, 7);

                T1 = _mm256_packus_epi16(T1, T2);

                TRANS_8x4(T1, m0, m1);

                pel *p = dst + j * i_dst;
                *(int*)p = _mm_extract_epi32(m0, 0); p += i_dst;
                *(int*)p = _mm_extract_epi32(m0, 1); p += i_dst;
                *(int*)p = _mm_extract_epi32(m0, 2); p += i_dst;
                *(int*)p = _mm_extract_epi32(m0, 3); p += i_dst;
                *(int*)p = _mm_extract_epi32(m1, 0); p += i_dst;
                *(int*)p = _mm_extract_epi32(m1, 1); p += i_dst;
                *(int*)p = _mm_extract_epi32(m1, 2); p += i_dst;
                *(int*)p = _mm_extract_epi32(m1, 3);

                p1 -= 8;
                p2 -= 8;
                p3 -= 8;
                p4 -= 8;
            }
        }
        else { // width == 8x && height == 8x
            __m256i mSwitch1 = _mm256_setr_epi8(7, 8, 6, 7, 5, 6, 4, 5, 3, 4, 2, 3, 1, 2, 0, 1, 7, 8, 6, 7, 5, 6, 4, 5, 3, 4, 2, 3, 1, 2, 0, 1);
            __m256i mSwitch2 = _mm256_setr_epi8(9, 10, 8, 9, 7, 8, 6, 7, 5, 6, 4, 5, 3, 4, 2, 3, 9, 10, 8, 9, 7, 8, 6, 7, 5, 6, 4, 5, 3, 4, 2, 3);
            __m128i m_c_32 = _mm_set1_epi8(32);
            __m128i m_c_64 = _mm_set1_epi8(64);
            __m256i m256_shuffle_c[4];
            m256_shuffle_c[0] = _mm256_set_m128i(_mm_set1_epi16(((8 + 1) << 8) + 8), _mm_set1_epi16(((0 + 1) << 8) + 0));
            m256_shuffle_c[1] = _mm256_set_m128i(_mm_set1_epi16(((10 + 1) << 8) + 10), _mm_set1_epi16(((2 + 1) << 8) + 2));
            m256_shuffle_c[2] = _mm256_set_m128i(_mm_set1_epi16(((12 + 1) << 8) + 12), _mm_set1_epi16(((4 + 1) << 8) + 4));
            m256_shuffle_c[3] = _mm256_set_m128i(_mm_set1_epi16(((14 + 1) << 8) + 14), _mm_set1_epi16(((6 + 1) << 8) + 6));
            for (int i = 0; i < width; i += 8) {
                pel *p1 = src - psteps[i + 0] - 10;
                pel *p2 = src - psteps[i + 1] - 10;
                pel *p3 = src - psteps[i + 2] - 10;
                pel *p4 = src - psteps[i + 3] - 10;
                pel *p5 = src - psteps[i + 4] - 10;
                pel *p6 = src - psteps[i + 5] - 10;
                pel *p7 = src - psteps[i + 6] - 10;
                pel *p8 = src - psteps[i + 7] - 10;
                __m128i m_c0 = _mm_loadl_epi64((const __m128i*)(poffsets + i));
                __m128i m_c1 = _mm_add_epi8(m_c0, m_c_32);
                __m128i m_c2 = _mm_sub_epi8(m_c_64, m_c0);
                __m128i m_c3 = _mm_sub_epi8(m_c_32, m_c0);
                __m128i m_c01 = _mm_unpacklo_epi8(m_c0, m_c1);
                __m128i m_c23 = _mm_unpacklo_epi8(m_c2, m_c3);
                __m256i m256_c01 = _mm256_set_m128i(m_c01, m_c01);
                __m256i m256_c23 = _mm256_set_m128i(m_c23, m_c23);

                __m256i C15_1 = _mm256_shuffle_epi8(m256_c01, m256_shuffle_c[0]);
                __m256i C26_1 = _mm256_shuffle_epi8(m256_c01, m256_shuffle_c[1]);
                __m256i C37_1 = _mm256_shuffle_epi8(m256_c01, m256_shuffle_c[2]);
                __m256i C48_1 = _mm256_shuffle_epi8(m256_c01, m256_shuffle_c[3]);
                __m256i C15_2 = _mm256_shuffle_epi8(m256_c23, m256_shuffle_c[0]);
                __m256i C26_2 = _mm256_shuffle_epi8(m256_c23, m256_shuffle_c[1]);
                __m256i C37_2 = _mm256_shuffle_epi8(m256_c23, m256_shuffle_c[2]);
                __m256i C48_2 = _mm256_shuffle_epi8(m256_c23, m256_shuffle_c[3]);
                
                for (int j = 0; j < height; j += 8) {
                    __m256i mSrc15 = _mm256_loadu2_m128i((__m128i*)p5, (__m128i*)p1);
                    __m256i mSrc26 = _mm256_loadu2_m128i((__m128i*)p6, (__m128i*)p2);
                    __m256i mSrc37 = _mm256_loadu2_m128i((__m128i*)p7, (__m128i*)p3);
                    __m256i mSrc48 = _mm256_loadu2_m128i((__m128i*)p8, (__m128i*)p4);
                    __m256i T1, T2, T3, T4, T5, T6, T7, T8;

                    T1 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc15, mSwitch1), C15_1);
                    T2 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc26, mSwitch1), C26_1);
                    T3 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc37, mSwitch1), C37_1);
                    T4 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc48, mSwitch1), C48_1);
                    T5 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc15, mSwitch2), C15_2);
                    T6 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc26, mSwitch2), C26_2);
                    T7 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc37, mSwitch2), C37_2);
                    T8 = _mm256_maddubs_epi16(_mm256_shuffle_epi8(mSrc48, mSwitch2), C48_2);

                    T1 = _mm256_add_epi16(T1, T5);
                    T2 = _mm256_add_epi16(T2, T6);
                    T3 = _mm256_add_epi16(T3, T7);
                    T4 = _mm256_add_epi16(T4, T8);

                    T1 = _mm256_add_epi16(T1, mOffset);
                    T2 = _mm256_add_epi16(T2, mOffset);
                    T3 = _mm256_add_epi16(T3, mOffset);
                    T4 = _mm256_add_epi16(T4, mOffset);
                    T1 = _mm256_srai_epi16(T1, 7);
                    T2 = _mm256_srai_epi16(T2, 7);
                    T3 = _mm256_srai_epi16(T3, 7);
                    T4 = _mm256_srai_epi16(T4, 7);

                    T1 = _mm256_packus_epi16(T1, T3);
                    T3 = _mm256_packus_epi16(T2, T4);

                    TRANS_8x8(T1, T3);

                    T2 = _mm256_srli_si256(T1, 8);
                    T4 = _mm256_srli_si256(T3, 8);

                    pel *p = dst + j * i_dst;
                    _mm_storel_epi64((__m128i *)p, _mm256_castsi256_si128(T1)); p += i_dst;
                    _mm_storel_epi64((__m128i *)p, _mm256_castsi256_si128(T2)); p += i_dst;
                    _mm_storel_epi64((__m128i *)p, _mm256_castsi256_si128(T3)); p += i_dst;
                    _mm_storel_epi64((__m128i *)p, _mm256_castsi256_si128(T4)); p += i_dst;
                    _mm_storel_epi64((__m128i *)p, _mm256_extracti128_si256(T1, 1)); p += i_dst;
                    _mm_storel_epi64((__m128i *)p, _mm256_extracti128_si256(T2, 1)); p += i_dst;
                    _mm_storel_epi64((__m128i *)p, _mm256_extracti128_si256(T3, 1)); p += i_dst;
                    _mm_storel_epi64((__m128i *)p, _mm256_extracti128_si256(T4, 1));

                    p1 -= 8;
                    p2 -= 8;
                    p3 -= 8;
                    p4 -= 8;
                    p5 -= 8;
                    p6 -= 8;
                    p7 -= 8;
                    p8 -= 8;
                }
                dst += 8;
            }
        }
    }
}

void uavs3d_ipred_ang_y_26_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i;
    __m256i zero = _mm256_setzero_si256();
    __m256i coeff2 = _mm256_set1_epi16(2);
    __m256i coeff3 = _mm256_set1_epi16(3);
    __m256i coeff4 = _mm256_set1_epi16(4);
    __m256i coeff5 = _mm256_set1_epi16(5);
    __m256i coeff7 = _mm256_set1_epi16(7);
    __m256i coeff8 = _mm256_set1_epi16(8);
    __m256i shuffle = _mm256_setr_epi8(7, 15, 6, 14, 5, 13, 4, 12, 3, 11, 2, 10, 1, 9, 0, 8, 7, 15, 6, 14, 5, 13, 4, 12, 3, 11, 2, 10, 1, 9, 0, 8);

    ALIGNED_16(pel first_line[64 + 256]);
    int line_size = width + (height - 1) * 4;
    int height4 = height << 2;

    src -= 15;

    for (i = 0; i < line_size - 32; i += 64, src -= 16) {
        __m256i p00, p10, p20, p30;
        __m256i M1, M3, M5, M7;
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 6), 0, *(s64 *)(src - 2));
        __m256i S3 = _mm256_set_epi64x(0, *(s64 *)(src + 5), 0, *(s64 *)(src - 3));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m256i L3 = _mm256_unpacklo_epi8(S3, zero);

        p00 = _mm256_mullo_epi16(L0, coeff3);
        p10 = _mm256_mullo_epi16(L1, coeff7);
        p20 = _mm256_mullo_epi16(L2, coeff5);
        p30 = _mm256_add_epi16(L3, coeff8);
        p00 = _mm256_add_epi16(p00, p30);
        p00 = _mm256_add_epi16(p00, p10);
        p00 = _mm256_add_epi16(p00, p20);
        M1 = _mm256_srli_epi16(p00, 4);

        p00 = _mm256_add_epi16(L1, L2);
        p00 = _mm256_mullo_epi16(p00, coeff3);
        p10 = _mm256_add_epi16(L0, L3);
        p10 = _mm256_add_epi16(p10, coeff4);
        p00 = _mm256_add_epi16(p10, p00);
        M3 = _mm256_srli_epi16(p00, 3);

        p10 = _mm256_mullo_epi16(L1, coeff5);
        p20 = _mm256_mullo_epi16(L2, coeff7);
        p30 = _mm256_mullo_epi16(L3, coeff3);
        p00 = _mm256_add_epi16(L0, coeff8);
        p00 = _mm256_add_epi16(p00, p10);
        p00 = _mm256_add_epi16(p00, p20);
        p00 = _mm256_add_epi16(p00, p30);
        M5 = _mm256_srli_epi16(p00, 4);

        p00 = _mm256_add_epi16(L1, L2);
        p10 = _mm256_add_epi16(L2, L3);
        p00 = _mm256_add_epi16(p00, p10);
        p00 = _mm256_add_epi16(p00, coeff2);
        M7 = _mm256_srli_epi16(p00, 2);

        M1 = _mm256_packus_epi16(M1, M3);
        M5 = _mm256_packus_epi16(M5, M7);
        M1 = _mm256_shuffle_epi8(M1, shuffle);
        M5 = _mm256_shuffle_epi8(M5, shuffle);

        M3 = _mm256_unpacklo_epi16(M1, M5);
        M7 = _mm256_unpackhi_epi16(M1, M5);

        _mm_store_si128((__m128i *)&first_line[i], _mm256_extracti128_si256(M3, 1));
        _mm_store_si128((__m128i *)&first_line[16 + i], _mm256_extracti128_si256(M7, 1));
        _mm_store_si128((__m128i *)&first_line[32 + i], _mm256_castsi256_si128(M3));
        _mm_store_si128((__m128i *)&first_line[48 + i], _mm256_castsi256_si128(M7));
    }

    if (i < line_size) {
        __m128i zero = _mm_setzero_si128();
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i coeff3 = _mm_set1_epi16(3);
        __m128i coeff4 = _mm_set1_epi16(4);
        __m128i coeff5 = _mm_set1_epi16(5);
        __m128i coeff7 = _mm_set1_epi16(7);
        __m128i coeff8 = _mm_set1_epi16(8);
        __m128i shuffle = _mm_setr_epi8(7, 15, 6, 14, 5, 13, 4, 12, 3, 11, 2, 10, 1, 9, 0, 8);
        __m128i p01, p11, p21, p31;
        __m128i M2, M4, M6, M8;
        __m128i S0 = _mm_loadu_si128((__m128i *)(src));
        __m128i S3 = _mm_loadu_si128((__m128i *)(src - 3));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src - 2));

        __m128i H0 = _mm_unpackhi_epi8(S0, zero);
        __m128i H1 = _mm_unpackhi_epi8(S1, zero);
        __m128i H2 = _mm_unpackhi_epi8(S2, zero);
        __m128i H3 = _mm_unpackhi_epi8(S3, zero);

        p01 = _mm_mullo_epi16(H0, coeff3);
        p11 = _mm_mullo_epi16(H1, coeff7);
        p21 = _mm_mullo_epi16(H2, coeff5);
        p31 = _mm_add_epi16(H3, coeff8);
        p01 = _mm_add_epi16(p01, p31);
        p01 = _mm_add_epi16(p01, p11);
        p01 = _mm_add_epi16(p01, p21);
        M2 = _mm_srli_epi16(p01, 4);

        p01 = _mm_add_epi16(H1, H2);
        p01 = _mm_mullo_epi16(p01, coeff3);
        p11 = _mm_add_epi16(H0, H3);
        p11 = _mm_add_epi16(p11, coeff4);
        p01 = _mm_add_epi16(p11, p01);
        M4 = _mm_srli_epi16(p01, 3);

        p11 = _mm_mullo_epi16(H1, coeff5);
        p21 = _mm_mullo_epi16(H2, coeff7);
        p31 = _mm_mullo_epi16(H3, coeff3);
        p01 = _mm_add_epi16(H0, coeff8);
        p01 = _mm_add_epi16(p01, p11);
        p01 = _mm_add_epi16(p01, p21);
        p01 = _mm_add_epi16(p01, p31);
        M6 = _mm_srli_epi16(p01, 4);

        p01 = _mm_add_epi16(H1, H2);
        p11 = _mm_add_epi16(H2, H3);
        p01 = _mm_add_epi16(p01, p11);
        p01 = _mm_add_epi16(p01, coeff2);
        M8 = _mm_srli_epi16(p01, 2);

        M2 = _mm_packus_epi16(M2, M4);
        M6 = _mm_packus_epi16(M6, M8);
        M2 = _mm_shuffle_epi8(M2, shuffle);
        M6 = _mm_shuffle_epi8(M6, shuffle);

        M4 = _mm_unpacklo_epi16(M2, M6);
        M8 = _mm_unpackhi_epi16(M2, M6);

        _mm_store_si128((__m128i *)&first_line[i], M4);
        _mm_store_si128((__m128i *)&first_line[16 + i], M8);
    }

    switch (width) {
    case 4:
        for (i = 0; i < height4; i += 4) {
            CP32(dst, first_line + i);
            dst += i_dst;
        }
        break;
    case 8:
        for (i = 0; i < height4; i += 4) {
            CP64(dst, first_line + i);
            dst += i_dst;
        }
        break;
    case 16:
        for (i = 0; i < height4; i += 4) {
            CP128(dst, first_line + i);
            dst += i_dst;
        }
        break;
    default:
        for (i = 0; i < height4; i += 4) {
            memcpy(dst, first_line + i, width * sizeof(pel));
            dst += i_dst;
        }
        break;
    }

}

void uavs3d_ipred_ang_y_28_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    ALIGNED_32(pel first_line[64 + 128 + 32]);
    int line_size = width + (height - 1) * 2;
    int real_size = COM_MIN(line_size, height * 4 + 1);
    int i;
    int height2 = height << 1;
    __m128i pad;
    __m256i coeff2 = _mm256_set1_epi16(2);
    __m256i coeff3 = _mm256_set1_epi16(3);
    __m256i coeff4 = _mm256_set1_epi16(4);
    __m256i shuffle = _mm256_setr_epi8(7, 15, 6, 14, 5, 13, 4, 12, 3, 11, 2, 10, 1, 9, 0, 8, 7, 15, 6, 14, 5, 13, 4, 12, 3, 11, 2, 10, 1, 9, 0, 8);
    __m256i zero = _mm256_setzero_si256();

    //src -= 31;

    for (i = 0; i < real_size - 32; i += 64, src -= 32) {
        __m256i p00, p10, p01, p11;
        __m256i S0 = _mm256_set_epi64x(*(s64 *)(src - 31), *(s64 *)(src - 15), *(s64 *)(src - 23), *(s64 *)(src - 7));
        __m256i S1 = _mm256_set_epi64x(*(s64 *)(src - 32), *(s64 *)(src - 16), *(s64 *)(src - 24), *(s64 *)(src - 8));
        __m256i S2 = _mm256_set_epi64x(*(s64 *)(src - 33), *(s64 *)(src - 17), *(s64 *)(src - 25), *(s64 *)(src - 9));
        __m256i S3 = _mm256_set_epi64x(*(s64 *)(src - 34), *(s64 *)(src - 18), *(s64 *)(src - 26), *(s64 *)(src - 10));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m256i L3 = _mm256_unpacklo_epi8(S3, zero);
        __m256i H0 = _mm256_unpackhi_epi8(S0, zero);
        __m256i H1 = _mm256_unpackhi_epi8(S1, zero);
        __m256i H2 = _mm256_unpackhi_epi8(S2, zero);
        __m256i H3 = _mm256_unpackhi_epi8(S3, zero);

        p00 = _mm256_adds_epi16(L1, L2);
        p01 = _mm256_add_epi16(L1, L2);
        p00 = _mm256_mullo_epi16(p00, coeff3);
        p10 = _mm256_adds_epi16(L0, L3);
        p11 = _mm256_add_epi16(L2, L3);
        p10 = _mm256_adds_epi16(p10, coeff4);
        p00 = _mm256_adds_epi16(p00, p10);
        p01 = _mm256_add_epi16(p01, p11);
        p01 = _mm256_add_epi16(p01, coeff2);

        p00 = _mm256_srli_epi16(p00, 3);
        p01 = _mm256_srli_epi16(p01, 2);

        p00 = _mm256_packus_epi16(p00, p01);
        p00 = _mm256_shuffle_epi8(p00, shuffle);
        _mm256_storeu_si256((__m256i *)&first_line[i], p00);

        p00 = _mm256_adds_epi16(H1, H2);
        p01 = _mm256_add_epi16(H1, H2);
        p00 = _mm256_mullo_epi16(p00, coeff3);
        p10 = _mm256_adds_epi16(H0, H3);
        p11 = _mm256_add_epi16(H2, H3);
        p10 = _mm256_adds_epi16(p10, coeff4);
        p00 = _mm256_adds_epi16(p00, p10);
        p01 = _mm256_add_epi16(p01, p11);
        p01 = _mm256_add_epi16(p01, coeff2);

        p00 = _mm256_srli_epi16(p00, 3);
        p01 = _mm256_srli_epi16(p01, 2);

        p00 = _mm256_packus_epi16(p00, p01);
        p00 = _mm256_shuffle_epi8(p00, shuffle);

        _mm256_storeu_si256((__m256i *)&first_line[i + 32], p00);
    }

    if (i < real_size) {
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i coeff3 = _mm_set1_epi16(3);
        __m128i coeff4 = _mm_set1_epi16(4);
        __m128i shuffle = _mm_setr_epi8(7, 15, 6, 14, 5, 13, 4, 12, 3, 11, 2, 10, 1, 9, 0, 8);
        __m128i zero = _mm_setzero_si128();
        __m128i p00, p10, p01, p11;
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 15));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src - 16));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src - 17));
        __m128i S3 = _mm_loadu_si128((__m128i *)(src - 18));

        __m128i L0 = _mm_unpacklo_epi8(S0, zero);
        __m128i L1 = _mm_unpacklo_epi8(S1, zero);
        __m128i L2 = _mm_unpacklo_epi8(S2, zero);
        __m128i L3 = _mm_unpacklo_epi8(S3, zero);

        __m128i H0 = _mm_unpackhi_epi8(S0, zero);
        __m128i H1 = _mm_unpackhi_epi8(S1, zero);
        __m128i H2 = _mm_unpackhi_epi8(S2, zero);
        __m128i H3 = _mm_unpackhi_epi8(S3, zero);

        p00 = _mm_adds_epi16(L1, L2);
        p01 = _mm_add_epi16(L1, L2);
        p00 = _mm_mullo_epi16(p00, coeff3);
        p10 = _mm_adds_epi16(L0, L3);
        p11 = _mm_add_epi16(L2, L3);
        p10 = _mm_adds_epi16(p10, coeff4);
        p00 = _mm_adds_epi16(p00, p10);
        p01 = _mm_add_epi16(p01, p11);
        p01 = _mm_add_epi16(p01, coeff2);

        p00 = _mm_srli_epi16(p00, 3);
        p01 = _mm_srli_epi16(p01, 2);

        p00 = _mm_packus_epi16(p00, p01);
        p00 = _mm_shuffle_epi8(p00, shuffle);

        _mm_store_si128((__m128i *)&first_line[i + 16], p00);

        p00 = _mm_adds_epi16(H1, H2);
        p01 = _mm_add_epi16(H1, H2);
        p00 = _mm_mullo_epi16(p00, coeff3);
        p10 = _mm_adds_epi16(H0, H3);
        p11 = _mm_add_epi16(H2, H3);
        p10 = _mm_adds_epi16(p10, coeff4);
        p00 = _mm_adds_epi16(p00, p10);
        p01 = _mm_add_epi16(p01, p11);
        p01 = _mm_add_epi16(p01, coeff2);

        p00 = _mm_srli_epi16(p00, 3);
        p01 = _mm_srli_epi16(p01, 2);

        p00 = _mm_packus_epi16(p00, p01);
        p00 = _mm_shuffle_epi8(p00, shuffle);

        _mm_store_si128((__m128i *)&first_line[i], p00);
    }

    // padding
    if (real_size < line_size) {
        i = real_size + 1;
        first_line[i - 1] = first_line[i - 3];

        pad = _mm_set1_epi16(((short *)&first_line[i - 2])[0]);

        for (; i < line_size; i += 16) {
            _mm_storeu_si128((__m128i *)&first_line[i], pad);
        }
    }

#define COPY_Y28(w) { \
    for (i = 0; i < height2; i += 2) { \
        memcpy(dst, first_line + i, w * sizeof(pel)); \
        dst += i_dst;\
    } \
}
    switch (width) {
    case 4:
        COPY_Y28(4)
            break;
    case 8:
        COPY_Y28(8)
            break;
    case 16:
        COPY_Y28(16)
            break;
    case 32:
        COPY_Y28(32)
            break;
    case 64:
        COPY_Y28(64)
            break;
    }
}

void uavs3d_ipred_ang_y_30_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    ALIGNED_16(pel first_line[64 + 64 + 16]);
    int line_size = width + height - 1;
    int real_size = COM_MIN(line_size, height * 2);
    int i;
    __m128i shuffle = _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    __m256i coeff2 = _mm256_set1_epi16(2);
    __m256i zero = _mm256_setzero_si256();

    src -= 17;

    for (i = 0; i < real_size - 8; i += 16, src -= 16) {
        __m256i p0, p1;
        __m128i d0;
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);

        p0 = _mm256_add_epi16(L0, L1);
        p1 = _mm256_add_epi16(L1, L2);
        p0 = _mm256_add_epi16(p0, p1);
        p0 = _mm256_add_epi16(p0, coeff2);

        p0 = _mm256_srli_epi16(p0, 2);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p0), _mm256_extracti128_si256(p0, 1));
        d0 = _mm_shuffle_epi8(d0, shuffle);

        _mm_store_si128((__m128i *)&first_line[i], d0);
    }

    if (i < real_size) {
        __m128i p01, p11;
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i zero = _mm_setzero_si128();
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src));

        __m128i H0 = _mm_unpackhi_epi8(S0, zero);
        __m128i H1 = _mm_unpackhi_epi8(S1, zero);
        __m128i H2 = _mm_unpackhi_epi8(S2, zero);

        p01 = _mm_add_epi16(H0, H1);
        p11 = _mm_add_epi16(H1, H2);

        p01 = _mm_add_epi16(p01, p11);
        p01 = _mm_add_epi16(p01, coeff2);

        p01 = _mm_srli_epi16(p01, 2);

        p01 = _mm_packus_epi16(p01, p01);
        p01 = _mm_shuffle_epi8(p01, shuffle);

        _mm_store_si128((__m128i *)&first_line[i], p01);
    }
    // padding
    for (i = real_size; i < line_size; i += 16) {
        __m128i pad = _mm_set1_epi8((char)first_line[real_size - 1]);
        _mm_storeu_si128((__m128i *)&first_line[i], pad);
    }

#define COPY_Y30(w) { \
    for (i = 0; i < height; i++) { \
        memcpy(dst, first_line + i, w * sizeof(pel)); \
        dst += i_dst;\
    } \
}
    switch (width) {
    case 4:
        COPY_Y30(4)
            break;
    case 8:
        COPY_Y30(8)
            break;
    case 16:
        COPY_Y30(16)
            break;
    case 32:
        COPY_Y30(32)
            break;
    case 64:
        COPY_Y30(64)
            break;
    }
}

void uavs3d_ipred_ang_y_32_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    ALIGNED_16(pel first_line[2 * (64 + 64)]);
    int line_size = height / 2 + width - 1;
    int real_size = COM_MIN(line_size, height);
    int i;
    __m128i pad_val;
    int aligned_line_size = ((line_size + 63) >> 4) << 4;
    pel *pfirst[2];
    __m256i coeff2 = _mm256_set1_epi16(2);
    __m256i zero = _mm256_setzero_si256();
    __m128i shuffle1 = _mm_setr_epi8(15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0);
    __m128i shuffle2 = _mm_setr_epi8(14, 12, 10, 8, 6, 4, 2, 0, 15, 13, 11, 9, 7, 5, 3, 1);
    int i_dst2 = i_dst * 2;

    pfirst[0] = first_line;
    pfirst[1] = first_line + aligned_line_size;

    src -= 18;

    for (i = 0; i < real_size - 4; i += 8, src -= 16) {
        __m256i p0, p1;
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m128i m0, d0, d1;

        p0 = _mm256_add_epi16(L0, L1);
        p1 = _mm256_add_epi16(L1, L2);
        p0 = _mm256_add_epi16(p0, coeff2);
        p0 = _mm256_add_epi16(p0, p1);
        p0 = _mm256_srli_epi16(p0, 2);

        m0 = _mm_packus_epi16(_mm256_castsi256_si128(p0), _mm256_extracti128_si256(p0, 1));
        d0 = _mm_shuffle_epi8(m0, shuffle1);
        d1 = _mm_shuffle_epi8(m0, shuffle2);
        _mm_storel_epi64((__m128i *)&pfirst[0][i], d0);
        _mm_storel_epi64((__m128i *)&pfirst[1][i], d1);
    }

    if (i < real_size) {
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i zero = _mm_setzero_si128();
        __m128i p10, p11;
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src));

        __m128i H0 = _mm_unpackhi_epi8(S0, zero);
        __m128i H1 = _mm_unpackhi_epi8(S1, zero);
        __m128i H2 = _mm_unpackhi_epi8(S2, zero);

        p10 = _mm_add_epi16(H0, H1);
        p11 = _mm_add_epi16(H1, H2);
        p10 = _mm_add_epi16(p10, coeff2);
        p10 = _mm_add_epi16(p10, p11);
        p10 = _mm_srli_epi16(p10, 2);

        p11 = _mm_packus_epi16(p10, p10);
        p10 = _mm_shuffle_epi8(p11, shuffle2);
        p11 = _mm_shuffle_epi8(p11, shuffle1);
        ((int *)&pfirst[0][i])[0] = _mm_cvtsi128_si32(p11);
        ((int *)&pfirst[1][i])[0] = _mm_cvtsi128_si32(p10);
    }

    // padding
    if (real_size < line_size) {
        pad_val = _mm_set1_epi8((char)pfirst[1][real_size - 1]);
        for (i = real_size; i < line_size; i++) {
            _mm_storeu_si128((__m128i *)&pfirst[0][i], pad_val);
            _mm_storeu_si128((__m128i *)&pfirst[1][i], pad_val);
        }
    }

    height /= 2;

#define COPY_Y32(w) { \
    for (i = 0; i < height; i++) { \
        memcpy(dst, pfirst[0] + i, w * sizeof(pel)); \
        memcpy(dst + i_dst, pfirst[1] + i, w * sizeof(pel)); \
        dst += i_dst2;\
    } \
}
    switch (width) {
    case 4:
        COPY_Y32(4)
            break;
    case 8:
        COPY_Y32(8)
            break;
    case 16:
        COPY_Y32(16)
            break;
    case 32:
        COPY_Y32(32)
            break;
    case 64:
        COPY_Y32(64)
            break;
    }
}

#define IPRED_ANG_XY_X8(px, shuffle_x, coeff_x, off, shift, dst) \
{\
        __m256i s;                  \
        __m128i d;                  \
        s = _mm256_set_epi64x(0, *(s64*)(px + 3), 0, *(s64*)(px - 1));                  \
        s = _mm256_shuffle_epi8(s, shuffle_x);                                          \
        s = _mm256_maddubs_epi16(s, coeff_x);                                           \
        d = _mm_hadd_epi16(_mm256_castsi256_si128(s), _mm256_extracti128_si256(s, 1));  \
        d = _mm_add_epi16(d, off);                                                      \
        d = _mm_srli_epi16(d, shift);                                                   \
        d = _mm_packus_epi16(d, d);                                                     \
        _mm_storel_epi64((__m128i*)(dst), d);                                           \
}

#define IPRED_ANG_XY_X4(px, shuffle_x, coeff_x, off, shift, dst) \
{\
        __m128i s, d;                               \
        s = _mm_set_epi64x(0, *(s64*)(px - 1));     \
        s = _mm_shuffle_epi8(s, shuffle_x);         \
        s = _mm_maddubs_epi16(s, coeff_x);          \
        d = _mm_hadd_epi16(s, s);                   \
        d = _mm_add_epi16(d, off);                  \
        d = _mm_srli_epi16(d, shift);               \
        d = _mm_packus_epi16(d, d);                 \
        *(int*)(dst) = _mm_extract_epi32(d, 0);     \
}

#define IPRED_ANG_XY_Y4(py, xsteps, xoffsets, shuffle, sign, c, off, shift, dst) \
{ \
        __m128i s, d;                               \
        __m128i coeff = _mm_set1_epi32(*(s32*)(xoffsets));                                                            \
        s = _mm_set_epi32(*(s32*)(py + xsteps[3]), *(s32*)(py + xsteps[2]), *(s32*)(py + xsteps[1]), *(s32*)(py + xsteps[0]));  \
        coeff = _mm_sign_epi8(coeff, sign);                                                                                     \
        coeff = _mm_add_epi8(coeff, c);                                                                                        \
        coeff = _mm_shuffle_epi8(coeff, shuffle);                                                                               \
        d = _mm_maddubs_epi16(s, coeff);                                                                                        \
        d = _mm_hadd_epi16(d, d);                                                                                               \
        d = _mm_add_epi16(d, off);                                                                                              \
        d = _mm_srli_epi16(d, shift);                                                                                           \
        d = _mm_packus_epi16(d, d);                                                                                             \
        *(s32*)(dst) = _mm_extract_epi32(d, 0);                                                                                 \
}
#define IPRED_ANG_XY_Y8(py, xsteps, xoffsets, shuffle, sign, c, off, shift, dst) \
{ \
        __m128i d;                               \
        __m256i coeff = _mm256_set_m128i(_mm_set1_epi32(*(s32*)(xoffsets + 4)), _mm_set1_epi32(*(s32*)(xoffsets)));             \
        __m256i s = _mm256_set_epi32(*(s32*)(py + xsteps[7]), *(s32*)(py + xsteps[6]), *(s32*)(py + xsteps[5]), *(s32*)(py + xsteps[4]), \
                *(s32*)(py + xsteps[3]), *(s32*)(py + xsteps[2]), *(s32*)(py + xsteps[1]), *(s32*)(py + xsteps[0]));            \
        coeff = _mm256_sign_epi8(coeff, sign);                                                                                  \
        coeff = _mm256_add_epi8(coeff, c);                                                                                      \
        coeff = _mm256_shuffle_epi8(coeff, shuffle);                                                                            \
        s = _mm256_maddubs_epi16(s, coeff);                                                                                     \
        d = _mm_hadd_epi16(_mm256_castsi256_si128(s), _mm256_extracti128_si256(s, 1));                                          \
        d = _mm_add_epi16(d, off);                                                                                              \
        d = _mm_srli_epi16(d, shift);                                                                                           \
        d = _mm_packus_epi16(d, d);                                                                                             \
        _mm_storel_epi64((__m128i*)(dst), d);                                                                                   \
}

void uavs3d_ipred_ang_xy_13_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i, j;
    pel *psrc = src;
    int step1_height = COM_MIN(height, 7);

    __m256i shuffle_x = _mm256_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6, 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
    __m128i c_64 = _mm_set1_epi16(64);

    for (j = 0; j < step1_height; j++) {
        int d = j + 1;
        pel *px = src - (d >> 3);
        int offsetx = (d << 2) & 0x1f;
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
        __m256i coeff_x = _mm256_set1_epi32(offsetx + (c << 8) + (b << 16) + (a << 24));

        for (i = 0; i + 4 < width; i += 8, px += 8) {
            IPRED_ANG_XY_X8(px, shuffle_x, coeff_x, c_64, 7, dst + i)
        }
        if (i < width) {
            IPRED_ANG_XY_X4(px, _mm256_castsi256_si128(shuffle_x), _mm256_castsi256_si128(coeff_x), c_64, 7, dst + i)
        }
        dst += i_dst;
    }

    if (width == 4) {
        for (; j < height; j++) {
            int d = j + 1;
            int step1_width = (int)(((j + 1) / 8.0 + 0.9999)) - 1;
            int offsetx = (d << 2) & 0x1f;
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
            pel *px = src + step1_width - (d >> 3);
            pel *py = psrc - j + 8;

            step1_width = COM_MIN(step1_width, width);

            for (i = 0; i < step1_width; i++, py += 8) {
                dst[i] = (py[-2] + (py[-1] << 1) + py[0] + 2) >> 2;
            }
            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }
    }
    else {
        for (; j < height; j++) {
            int d = j + 1;
            int step1_width = (int)(((j + 1) / 8.0 + 0.9999)) - 1;
            int offsetx = (d << 2) & 0x1f;
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
            pel *px = src + step1_width - (d >> 3);
            pel *py = psrc - j + 8;
            __m256i coeff_x = _mm256_set1_epi32(offsetx + (c << 8) + (b << 16) + (a << 24));

            step1_width = COM_MIN(step1_width, width);

            for (i = 0; i < step1_width; i++, py += 8) {
                dst[i] = (py[-2] + (py[-1] << 1) + py[0] + 2) >> 2;
            }

            for (i = step1_width; i + 7 < width; i += 8, px += 8) {
                IPRED_ANG_XY_X8(px, shuffle_x, coeff_x, c_64, 7, dst + i)
            }
            if (i + 3 < width) {
                IPRED_ANG_XY_X4(px, _mm256_castsi256_si128(shuffle_x), _mm256_castsi256_si128(coeff_x), c_64, 7, dst + i)
                    i += 4;
                px += 4;
            }
            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }
    }
}

void uavs3d_ipred_ang_xy_14_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i;
    __m256i coeff2 = _mm256_set1_epi16(2);
    __m256i coeff3 = _mm256_set1_epi16(3);
    __m256i coeff4 = _mm256_set1_epi16(4);
    __m256i coeff5 = _mm256_set1_epi16(5);
    __m256i coeff7 = _mm256_set1_epi16(7);
    __m256i coeff8 = _mm256_set1_epi16(8);
    __m256i zero = _mm256_setzero_si256();

    ALIGNED_16(pel first_line[4 * (64 + 32)]);
    int line_size = width + height / 4 - 1;
    int left_size = line_size - width;
    int aligned_line_size = ((line_size + 31) >> 4) << 4;
    pel *pfirst[4];
    __m128i shuffle1 = _mm_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);
    __m128i shuffle2 = _mm_setr_epi8(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 0, 4, 8, 12);
    __m128i shuffle3 = _mm_setr_epi8(2, 6, 10, 14, 3, 7, 11, 15, 0, 4, 8, 12, 1, 5, 9, 13);
    __m128i shuffle4 = _mm_setr_epi8(3, 7, 11, 15, 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14);
    pel *src1 = src;

    pfirst[0] = first_line;
    pfirst[1] = first_line + aligned_line_size;
    pfirst[2] = first_line + aligned_line_size * 2;
    pfirst[3] = first_line + aligned_line_size * 3;

    src -= height - 4;
    for (i = 0; i < left_size - 1; i += 4, src += 16) {
        __m256i p00, p01;
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m128i m0, d0, d1, d2, d3;

        p00 = _mm256_add_epi16(L0, L1);
        p01 = _mm256_add_epi16(L1, L2);

        p00 = _mm256_add_epi16(p00, coeff2);
        p00 = _mm256_add_epi16(p00, p01);

        p00 = _mm256_srli_epi16(p00, 2);

        m0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        d3 = _mm_shuffle_epi8(m0, shuffle4);
        d2 = _mm_shuffle_epi8(m0, shuffle3);
        d1 = _mm_shuffle_epi8(m0, shuffle2);
        d0 = _mm_shuffle_epi8(m0, shuffle1);

        ((int *)&pfirst[0][i])[0] = _mm_cvtsi128_si32(d3);
        ((int *)&pfirst[1][i])[0] = _mm_cvtsi128_si32(d2);
        ((int *)&pfirst[2][i])[0] = _mm_cvtsi128_si32(d1);
        ((int *)&pfirst[3][i])[0] = _mm_cvtsi128_si32(d0);
    }

    if (i < left_size) { //使用c语言可能会更优
        __m128i zero = _mm_setzero_si128();
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i p00, p01, p10;
        __m128i p20, p30;
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src));

        __m128i L0 = _mm_unpacklo_epi8(S0, zero);
        __m128i L1 = _mm_unpacklo_epi8(S1, zero);
        __m128i L2 = _mm_unpacklo_epi8(S2, zero);

        p00 = _mm_add_epi16(L0, L1);
        p01 = _mm_add_epi16(L1, L2);

        p00 = _mm_add_epi16(p00, coeff2);
        p00 = _mm_add_epi16(p00, p01);

        p00 = _mm_srli_epi16(p00, 2);

        p00 = _mm_packus_epi16(p00, p00);
        p10 = _mm_shuffle_epi8(p00, shuffle2);
        p20 = _mm_shuffle_epi8(p00, shuffle3);
        p30 = _mm_shuffle_epi8(p00, shuffle4);
        p00 = _mm_shuffle_epi8(p00, shuffle1);

        ((int *)&pfirst[0][i])[0] = _mm_cvtsi128_si32(p30);
        ((int *)&pfirst[1][i])[0] = _mm_cvtsi128_si32(p20);
        ((int *)&pfirst[2][i])[0] = _mm_cvtsi128_si32(p10);
        ((int *)&pfirst[3][i])[0] = _mm_cvtsi128_si32(p00);
    }

    src = src1;

    for (i = left_size; i < line_size; i += 16, src += 16) {
        __m256i p00, p10, p20, p30;
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));
        __m256i S3 = _mm256_set_epi64x(0, *(s64 *)(src + 10), 0, *(s64 *)(src + 2));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m256i L3 = _mm256_unpacklo_epi8(S3, zero);
        __m128i d0;

        p00 = _mm256_mullo_epi16(L0, coeff3);
        p10 = _mm256_mullo_epi16(L1, coeff7);
        p20 = _mm256_mullo_epi16(L2, coeff5);
        p30 = _mm256_add_epi16(L3, coeff8);
        p00 = _mm256_add_epi16(p00, p30);
        p00 = _mm256_add_epi16(p00, p10);
        p00 = _mm256_add_epi16(p00, p20);
        p00 = _mm256_srli_epi16(p00, 4);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_storeu_si128((__m128i *)&pfirst[2][i], d0);

        p00 = _mm256_add_epi16(L1, L2);
        p00 = _mm256_mullo_epi16(p00, coeff3);
        p10 = _mm256_add_epi16(L0, L3);
        p10 = _mm256_add_epi16(p10, coeff4);
        p00 = _mm256_add_epi16(p10, p00);
        p00 = _mm256_srli_epi16(p00, 3);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_storeu_si128((__m128i *)&pfirst[1][i], d0);

        p10 = _mm256_mullo_epi16(L1, coeff5);
        p20 = _mm256_mullo_epi16(L2, coeff7);
        p30 = _mm256_mullo_epi16(L3, coeff3);
        p00 = _mm256_add_epi16(L0, coeff8);
        p00 = _mm256_add_epi16(p00, p10);
        p00 = _mm256_add_epi16(p00, p20);
        p00 = _mm256_add_epi16(p00, p30);
        p00 = _mm256_srli_epi16(p00, 4);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_storeu_si128((__m128i *)&pfirst[0][i], d0);

        p00 = _mm256_add_epi16(L0, L1);
        p10 = _mm256_add_epi16(L1, L2);
        p00 = _mm256_add_epi16(p00, p10);
        p00 = _mm256_add_epi16(p00, coeff2);
        p00 = _mm256_srli_epi16(p00, 2);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_storeu_si128((__m128i *)&pfirst[3][i], d0);
    }

    pfirst[0] += left_size;
    pfirst[1] += left_size;
    pfirst[2] += left_size;
    pfirst[3] += left_size;

    height >>= 2;

#define COPY_XY14(w)  \
    for (i = 0; i < height; i++) {                       \
        memcpy(dst, pfirst[0] - i, width * sizeof(pel)); \
        dst += i_dst;                                    \
        memcpy(dst, pfirst[1] - i, width * sizeof(pel)); \
        dst += i_dst;                                    \
        memcpy(dst, pfirst[2] - i, width * sizeof(pel)); \
        dst += i_dst;                                    \
        memcpy(dst, pfirst[3] - i, width * sizeof(pel)); \
        dst += i_dst;                                    \
    }

    switch (width) {
    case 4:
        for (i = 0; i < height; i++) {
            CP32(dst, pfirst[0] - i);
            dst += i_dst;
            CP32(dst, pfirst[1] - i);
            dst += i_dst;
            CP32(dst, pfirst[2] - i);
            dst += i_dst;
            CP32(dst, pfirst[3] - i);
            dst += i_dst;
        }
        break;
    case 8:
        for (i = 0; i < height; i++) {
            CP64(dst, pfirst[0] - i);
            dst += i_dst;
            CP64(dst, pfirst[1] - i);
            dst += i_dst;
            CP64(dst, pfirst[2] - i);
            dst += i_dst;
            CP64(dst, pfirst[3] - i);
            dst += i_dst;
        }
        break;
    case 16:
        for (i = 0; i < height; i++) {
            CP128(dst, pfirst[0] - i);
            dst += i_dst;
            CP128(dst, pfirst[1] - i);
            dst += i_dst;
            CP128(dst, pfirst[2] - i);
            dst += i_dst;
            CP128(dst, pfirst[3] - i);
            dst += i_dst;
        }
        break;
    case 32:
        COPY_XY14(32)
            break;
    case 64:
        COPY_XY14(64)
            break;
    default:
        uavs3d_assert(0);
        break;
    }
}

void uavs3d_ipred_ang_xy_15_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i, j;
    pel *psrc = src;
    u8 *xoffsets = uavs3d_ipred_offsets[15-3][1];
    u8 *xsteps = uavs3d_ipred_steps[15-3][1];
    int d = 93;

    __m256i shuffle_x = _mm256_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6, 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
    __m128i c_64 = _mm_set1_epi16(64);

    for (j = 0; j < 2; j++, d += 93) {
        int offsetx = (d >> 3) & 0x1f;
        pel *px = src - (d >> 8);
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
        __m256i coeff_x = _mm256_set1_epi32(offsetx + (c << 8) + (b << 16) + (a << 24));

        for (i = 0; i + 4 < width; i += 8, px += 8) {
            IPRED_ANG_XY_X8(px, shuffle_x, coeff_x, c_64, 7, dst + i)
        }
        if (i < width) {
            IPRED_ANG_XY_X4(px, _mm256_castsi256_si128(shuffle_x), _mm256_castsi256_si128(coeff_x), c_64, 7, dst + i)
        }
        dst += i_dst;
    }

    if (width <= 8) {
        for (; j < height; j++, d += 93) {
            int step1_width = (int)(((j + 1) << 2) / 11.0 + 0.9999) - 1;
            int offsetx = (d >> 3) & 0x1f;
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
            pel *px = src - (d >> 8);

            step1_width = COM_MIN(step1_width, width);

            for (i = 0; i < step1_width; i++) {
                pel *py = psrc - j + xsteps[i];
                int offsety = xoffsets[i];
                dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
            }

            px += i;

            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }
    }
    else {
        __m128i shuffle_c = _mm_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);
        __m128i sign = _mm_set_epi32(0x01010101, 0x01010101, -1, -1);
        __m128i c0 = _mm_set_epi8(0, 0, 0, 0, 32, 32, 32, 32, 64, 64, 64, 64, 32, 32, 32, 32);
        for (; j < height; j++, d += 93) {
            int step1_width = (int)(((j + 1) << 2) / 11.0 + 0.9999) - 1;
            int offsetx = (d >> 3) & 0x1f;
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
            pel *px = src - (d >> 8);
            pel *py = psrc - j - 2;
            __m256i coeff_x = _mm256_set1_epi32(offsetx + (c << 8) + (b << 16) + (a << 24));
            u8 *psteps = xsteps;

            step1_width = COM_MIN(step1_width, width);

            for (i = 0; i < step1_width; i += 4, psteps += 4) {
                IPRED_ANG_XY_Y4(py, psteps, xoffsets + i, shuffle_c, sign, c0, c_64, 7, dst + i)
            }

            px += step1_width;

            for (i = step1_width; i + 7 < width; i += 8, px += 8) {
                IPRED_ANG_XY_X8(px, shuffle_x, coeff_x, c_64, 7, dst + i)
            }
            if (i + 3 < width) {
                IPRED_ANG_XY_X4(px, _mm256_castsi256_si128(shuffle_x), _mm256_castsi256_si128(coeff_x), c_64, 7, dst + i)
                i += 4;
                px += 4;
            }
            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }
    }
}

void uavs3d_ipred_ang_xy_16_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    ALIGNED_16(pel first_line[2 * (64 + 48)]);
    int line_size = width + height / 2 - 1;
    int left_size = line_size - width;
    int aligned_line_size = ((line_size + 31) >> 4) << 4;
    pel *pfirst[2];
    __m256i zero = _mm256_setzero_si256();
    __m256i coeff2 = _mm256_set1_epi16(2);
    __m256i coeff3 = _mm256_set1_epi16(3);
    __m256i coeff4 = _mm256_set1_epi16(4);
    __m128i shuffle1 = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15);
    __m128i shuffle2 = _mm_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14);
    int i;
    pel *src1;

    pfirst[0] = first_line;
    pfirst[1] = first_line + aligned_line_size;

    src -= height - 2;

    src1 = src;

    for (i = 0; i < left_size - 4; i += 8, src += 16) {
        __m256i p00, p01;
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m128i m0, d0, d1;

        p00 = _mm256_add_epi16(L0, L1);
        p01 = _mm256_add_epi16(L1, L2);
        p00 = _mm256_add_epi16(p00, coeff2);
        p00 = _mm256_add_epi16(p00, p01);

        p00 = _mm256_srli_epi16(p00, 2);
        m0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));

        d0 = _mm_shuffle_epi8(m0, shuffle1);
        d1 = _mm_shuffle_epi8(m0, shuffle2);
        _mm_storel_epi64((__m128i *)&pfirst[1][i], d0);
        _mm_storel_epi64((__m128i *)&pfirst[0][i], d1);
    }

    if (i < left_size) {
        __m128i p00, p01;
        __m128i zero = _mm_setzero_si128();
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src));

        __m128i L0 = _mm_unpacklo_epi8(S0, zero);
        __m128i L1 = _mm_unpacklo_epi8(S1, zero);
        __m128i L2 = _mm_unpacklo_epi8(S2, zero);

        p00 = _mm_add_epi16(L0, L1);
        p01 = _mm_add_epi16(L1, L2);
        p00 = _mm_add_epi16(p00, coeff2);
        p00 = _mm_add_epi16(p00, p01);
        p00 = _mm_srli_epi16(p00, 2);
        p00 = _mm_packus_epi16(p00, p00);

        p01 = _mm_shuffle_epi8(p00, shuffle2);
        p00 = _mm_shuffle_epi8(p00, shuffle1);
        ((int *)&pfirst[1][i])[0] = _mm_cvtsi128_si32(p00);
        ((int *)&pfirst[0][i])[0] = _mm_cvtsi128_si32(p01);
    }

    src = src1 + left_size + left_size;

    for (i = left_size; i < line_size; i += 16, src += 16) {
        __m256i p00, p01;
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));
        __m256i S3 = _mm256_set_epi64x(0, *(s64 *)(src + 10), 0, *(s64 *)(src + 2));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m256i L3 = _mm256_unpacklo_epi8(S3, zero);
        __m128i d0;

        p00 = _mm256_add_epi16(L1, L2);
        p00 = _mm256_mullo_epi16(p00, coeff3);
        p01 = _mm256_add_epi16(L0, L3);
        p00 = _mm256_add_epi16(p00, coeff4);
        p00 = _mm256_add_epi16(p00, p01);
        p00 = _mm256_srli_epi16(p00, 3);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_storeu_si128((__m128i *)&pfirst[0][i], d0);

        p00 = _mm256_add_epi16(L0, L1);
        p01 = _mm256_add_epi16(L1, L2);
        p00 = _mm256_add_epi16(p00, coeff2);
        p00 = _mm256_add_epi16(p00, p01);
        p00 = _mm256_srli_epi16(p00, 2);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(p00), _mm256_extracti128_si256(p00, 1));
        _mm_storeu_si128((__m128i *)&pfirst[1][i], d0);
    }

    pfirst[0] += left_size;
    pfirst[1] += left_size;

    height >>= 1;

#define COPY_XY16(w) { \
    for (i = 0; i < height; i++) { \
        memcpy(dst,         pfirst[0] - i, w * sizeof(pel)); \
        memcpy(dst + i_dst, pfirst[1] - i, w * sizeof(pel)); \
        dst += (i_dst << 1); \
    } \
}
    switch (width) {
    case 4:
        COPY_XY16(4)
            break;
    case 8:
        COPY_XY16(8)
            break;
    case 16:
        COPY_XY16(16)
            break;
    case 32:
        COPY_XY16(32)
            break;
    case 64:
        COPY_XY16(64)
            break;
    }

}

void uavs3d_ipred_ang_xy_17_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i, j;
    pel *psrc = src;
    u8 *xoffsets = uavs3d_ipred_offsets[17-3][1];
    u8 *xsteps = uavs3d_ipred_steps[17-3][1];
    int d = 93;
    __m256i shuffle_x = _mm256_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6, 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
    __m128i c_64 = _mm_set1_epi16(64);

    {
        int offsetx = (d >> 2) & 0x1f;
        pel *px = src - (d >> 7);
        int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
        __m256i coef = _mm256_set1_epi32(offsetx + (c << 8) + (b << 16) + (a << 24));
        for (i = 0; i < width; i += 8, px += 8) {
            IPRED_ANG_XY_X8(px, shuffle_x, coef, c_64, 7, dst + i)
        }
        if (i < width) {
            IPRED_ANG_XY_X4(px, _mm256_castsi256_si128(shuffle_x), _mm256_castsi256_si128(coef), c_64, 7, dst + i)
        }
        dst += i_dst;
        d += 93;
    }
    if (width <= 8) {
        for (j = 1; j < height; j++, d += 93) {
            int step1_width = (int)(((j + 1) << 3) / 11.0 + 0.9999) - 1;
            int offsetx = (d >> 2) & 0x1f;
            pel *px = src - (d >> 7);
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;

            step1_width = COM_MIN(step1_width, width);

            for (i = 0; i < step1_width; i++) {
                pel *py = psrc - j + xsteps[i];
                int offsety = xoffsets[i];
                dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
            }
            px += i;
            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }
    }
    else {
        for (j = 1; j < height; j++, d += 93) {
            int step1_width = (int)(((j + 1) << 3) / 11.0 + 0.9999) - 1;
            int offsetx = (d >> 2) & 0x1f;
            pel *px = src - (d >> 7);
            pel *py = psrc - j - 2;
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
            u8 *psteps = xsteps;
            __m128i shuffle_c;
            __m128i sign = _mm_set_epi32(0x01010101, 0x01010101, -1, -1);
            __m128i c0;
            __m256i coeff_x = _mm256_set1_epi32(offsetx + (c << 8) + (b << 16) + (a << 24));
            c0 = _mm_set_epi8(0, 0, 0, 0, 32, 32, 32, 32, 64, 64, 64, 64, 32, 32, 32, 32);
            shuffle_c = _mm_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);
            step1_width = COM_MIN(step1_width, width);

            for (i = 0; i < step1_width; i += 4, psteps += 4) {
                IPRED_ANG_XY_Y4(py, psteps, xoffsets + i, shuffle_c, sign, c0, c_64, 7, dst + i)
            }
            px += step1_width;
            for (i = step1_width; i + 7 < width; i += 8, px += 8) {
                IPRED_ANG_XY_X8(px, shuffle_x, coeff_x, c_64, 7, dst + i)
            }
            if (i + 3 < width) {
                IPRED_ANG_XY_X4(px, _mm256_castsi256_si128(shuffle_x), _mm256_castsi256_si128(coeff_x), c_64, 7, dst + i)
                    i += 4;
                px += 4;
            }
            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }
    }
}

void uavs3d_ipred_ang_xy_18_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    ALIGNED_16(pel first_line[64 + 64]);
    int line_size = width + height - 1;
    int i;
    pel *pfirst = first_line + height - 1;
    __m256i coeff2 = _mm256_set1_epi16(2);
    __m256i zero = _mm256_setzero_si256();

    src -= height - 1;

    for (i = 0; i < line_size - 8; i += 16, src += 16) {
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);

        __m256i sum1 = _mm256_add_epi16(L0, L1);
        __m256i sum2 = _mm256_add_epi16(L1, L2);
        __m128i d0;

        sum1 = _mm256_add_epi16(sum1, sum2);
        sum1 = _mm256_add_epi16(sum1, coeff2);
        sum1 = _mm256_srli_epi16(sum1, 2);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1));

        _mm_store_si128((__m128i *)&first_line[i], d0);
    }

    if (i < line_size) {
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i zero = _mm_setzero_si128();
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src));

        __m128i L0 = _mm_unpacklo_epi8(S0, zero);
        __m128i L1 = _mm_unpacklo_epi8(S1, zero);
        __m128i L2 = _mm_unpacklo_epi8(S2, zero);

        __m128i sum1 = _mm_add_epi16(L0, L1);
        __m128i sum2 = _mm_add_epi16(L1, L2);

        sum1 = _mm_add_epi16(sum1, sum2);
        sum1 = _mm_add_epi16(sum1, coeff2);
        sum1 = _mm_srli_epi16(sum1, 2);

        sum1 = _mm_packus_epi16(sum1, sum1);
        _mm_storel_epi64((__m128i *)&first_line[i], sum1);
    }

    switch (width) {
    case 4:
        for (i = 0; i < height; i++) {
            CP32(dst, pfirst--);
            dst += i_dst;
        }
        break;
    case 8:
        for (i = 0; i < height; i++) {
            CP64(dst, pfirst--);
            dst += i_dst;
        }
        break;
    case 16:
        for (i = 0; i < height; i++) {
            CP128(dst, pfirst--);
            dst += i_dst;
        }
        break;
    default:
        for (i = 0; i < height; i++) {
            memcpy(dst, pfirst--, width * sizeof(pel));
            dst += i_dst;
        }
        break;
    }

}

void uavs3d_ipred_ang_xy_19_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i, j;
    pel *psrc = src;
    u8 *xoffsets = uavs3d_ipred_offsets[19-3][1];
    u8 *xsteps = uavs3d_ipred_steps[19-3][1];
    int d = 11;
    int step2_height = ((93 * width) >> 7);

    __m128i shuffle_c;
    __m128i sign = _mm_set_epi32(0x01010101, 0x01010101, -1, -1);
    __m128i c0, c_64;
    c0 = _mm_set_epi8(0, 0, 0, 0, 32, 32, 32, 32, 64, 64, 64, 64, 32, 32, 32, 32);
    c_64 = _mm_set1_epi16(64);
    shuffle_c = _mm_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);

    step2_height = COM_MIN(step2_height, height);

    if (width <= 8) {
        for (j = 0; j < step2_height; j++, d += 11) {
            int step1_width = (int)(((j + 1) << 7) / 93.0 + 0.9999) - 1;
            int offsetx = (d << 2) & 0x1f;
            pel *px = src - (d >> 3);
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;

            step1_width = COM_MIN(step1_width, width);

            for (i = 0; i < step1_width; i++) {
                pel *py = psrc - j + xsteps[i];
                int offsety = xoffsets[i];
                dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
            }
            px += i;

            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }
    }
    else {
        for (j = 0; j < step2_height; j++, d += 11) {
            int step1_width = (int)(((j + 1) << 7) / 93.0 + 0.9999) - 1;
            int offsetx = (d << 2) & 0x1f;
            pel *px = src - (d >> 3);
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
            pel *py = psrc - j - 2;
            u8 *psteps = xsteps;
            __m256i shuffle_x = _mm256_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6, 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
            __m256i coeff_x = _mm256_set1_epi32(offsetx + (c << 8) + (b << 16) + (a << 24));

            step1_width = COM_MIN(step1_width, width);
            for (i = 0; i < step1_width; i += 4, psteps += 4) {
                IPRED_ANG_XY_Y4(py, psteps, xoffsets + i, shuffle_c, sign, c0, c_64, 7, dst + i)
            }

            px += step1_width;

            for (i = step1_width; i + 7 < width; i += 8, px += 8) {
                IPRED_ANG_XY_X8(px, shuffle_x, coeff_x, c_64, 7, dst + i)
            }
            for (; i + 3 < width; i += 4, px += 4) {
                IPRED_ANG_XY_X4(px, _mm256_castsi256_si128(shuffle_x), _mm256_castsi256_si128(coeff_x), c_64, 7, dst + i)
            }
            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }

    }

    {
        __m256i shuffle_c_256 = _mm256_set_m128i(shuffle_c, shuffle_c);
        __m256i sign_256 = _mm256_set_m128i(sign, sign);
        __m256i c0_256 = _mm256_set_m128i(c0, c0);
        for (; j < height; j++) {
            pel *py = psrc - j - 2;
            u8 *psteps = xsteps;
            for (i = 0; i + 4 < width; i += 8, psteps += 8) {
                IPRED_ANG_XY_Y8(py, psteps, xoffsets + i, shuffle_c_256, sign_256, c0_256, c_64, 7, dst + i)
            }
            if (i < width) {
                IPRED_ANG_XY_Y4(py, psteps, xoffsets + i, shuffle_c, sign, c0, c_64, 7, dst + i)
            }
            dst += i_dst;
        }
    }
}

void uavs3d_ipred_ang_xy_20_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    ALIGNED_16(pel first_line[64 + 128]);
    int left_size = (height - 1) * 2 + 1;
    int top_size = width - 1;
    int line_size = left_size + top_size;
    int i;
    pel *pfirst = first_line + left_size - 1;
    __m256i zero = _mm256_setzero_si256();
    __m256i coeff2 = _mm256_set1_epi16(2);
    __m256i coeff3 = _mm256_set1_epi16(3);
    __m256i coeff4 = _mm256_set1_epi16(4);
    __m256i shuffle = _mm256_setr_epi8(0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15, 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15);
    pel *src1 = src;

    src -= height;

    for (i = 0; i < left_size - 16; i += 32, src += 16) {
        __m256i p00, p01, p20, p21;
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));
        __m256i S3 = _mm256_set_epi64x(0, *(s64 *)(src + 10), 0, *(s64 *)(src + 2));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
        __m256i L3 = _mm256_unpacklo_epi8(S3, zero);

        p00 = _mm256_add_epi16(L1, L2);
        p00 = _mm256_mullo_epi16(p00, coeff3);
        p01 = _mm256_add_epi16(L0, L3);
        p00 = _mm256_add_epi16(p00, coeff4);
        p00 = _mm256_add_epi16(p00, p01);

        p00 = _mm256_srli_epi16(p00, 3);

        p20 = _mm256_add_epi16(L1, L2);
        p21 = _mm256_add_epi16(L2, L3);
        p20 = _mm256_add_epi16(p20, coeff2);
        p20 = _mm256_add_epi16(p20, p21);

        p20 = _mm256_srli_epi16(p20, 2);

        p00 = _mm256_packus_epi16(p00, p20);

        p00 = _mm256_shuffle_epi8(p00, shuffle);
        _mm_store_si128((__m128i *)&first_line[i], _mm256_castsi256_si128(p00));
        _mm_store_si128((__m128i *)&first_line[i + 16], _mm256_extracti128_si256(p00, 1));
    }

    if (i < left_size) {
        __m128i p00, p01;
        __m128i p20, p21;
        __m128i shuffle = _mm_setr_epi8(0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15);
        __m128i zero = _mm_setzero_si128();
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i coeff3 = _mm_set1_epi16(3);
        __m128i coeff4 = _mm_set1_epi16(4);
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S3 = _mm_loadu_si128((__m128i *)(src + 2));

        __m128i L0 = _mm_unpacklo_epi8(S0, zero);
        __m128i L1 = _mm_unpacklo_epi8(S1, zero);
        __m128i L2 = _mm_unpacklo_epi8(S2, zero);
        __m128i L3 = _mm_unpacklo_epi8(S3, zero);

        p00 = _mm_add_epi16(L1, L2);
        p00 = _mm_mullo_epi16(p00, coeff3);

        p01 = _mm_add_epi16(L0, L3);
        p00 = _mm_add_epi16(p00, coeff4);
        p00 = _mm_add_epi16(p00, p01);

        p00 = _mm_srli_epi16(p00, 3);

        p20 = _mm_add_epi16(L1, L2);
        p21 = _mm_add_epi16(L2, L3);
        p20 = _mm_add_epi16(p20, coeff2);
        p20 = _mm_add_epi16(p20, p21);

        p20 = _mm_srli_epi16(p20, 2);

        p00 = _mm_packus_epi16(p00, p20);

        p00 = _mm_shuffle_epi8(p00, shuffle);
        _mm_store_si128((__m128i *)&first_line[i], p00);
    }

    src = src1;

    for (i = left_size; i < line_size - 8; i += 16, src += 16) {
        __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
        __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
        __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));

        __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
        __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
        __m256i L2 = _mm256_unpacklo_epi8(S2, zero);

        __m256i sum1 = _mm256_add_epi16(L0, L1);
        __m256i sum2 = _mm256_add_epi16(L1, L2);
        __m128i d0;

        sum1 = _mm256_add_epi16(sum1, sum2);
        sum1 = _mm256_add_epi16(sum1, coeff2);
        sum1 = _mm256_srli_epi16(sum1, 2);

        d0 = _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1));

        _mm_storeu_si128((__m128i *)&first_line[i], d0);
    }

    if (i < line_size) {
        __m128i zero = _mm_setzero_si128();
        __m128i coeff2 = _mm_set1_epi16(2);
        __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
        __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
        __m128i S1 = _mm_loadu_si128((__m128i *)(src));

        __m128i L0 = _mm_unpacklo_epi8(S0, zero);
        __m128i L1 = _mm_unpacklo_epi8(S1, zero);
        __m128i L2 = _mm_unpacklo_epi8(S2, zero);

        __m128i sum1 = _mm_add_epi16(L0, L1);
        __m128i sum2 = _mm_add_epi16(L1, L2);

        sum1 = _mm_add_epi16(sum1, sum2);
        sum1 = _mm_add_epi16(sum1, coeff2);
        sum1 = _mm_srli_epi16(sum1, 2);

        sum1 = _mm_packus_epi16(sum1, sum1);
        _mm_storel_epi64((__m128i *)&first_line[i], sum1);
    }

#define COPY_XY20(w) { \
    for (i = 0; i < height; i++) { \
        memcpy(dst, pfirst, w * sizeof(pel)); \
        pfirst -= 2; \
        dst += i_dst;\
    } \
}
    switch (width) {
    case 4:
        COPY_XY20(4)
            break;
    case 8:
        COPY_XY20(8)
            break;
    case 16:
        COPY_XY20(16)
            break;
    case 32:
        COPY_XY20(32)
            break;
    case 64:
        COPY_XY20(64)
            break;
    }
}

void uavs3d_ipred_ang_xy_21_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i, j;
    pel *psrc = src;
    u8 *xoffsets = uavs3d_ipred_offsets[21-3][1];
    u8 *xsteps = uavs3d_ipred_steps[21-3][1];
    int d = 11;
    int step2_height = (93 * width) >> 8;

    __m128i shuffle_c;
    __m128i sign = _mm_set_epi32(0x01010101, 0x01010101, -1, -1);
    __m128i c0, c_64;
    c0 = _mm_set_epi8(0, 0, 0, 0, 32, 32, 32, 32, 64, 64, 64, 64, 32, 32, 32, 32);
    c_64 = _mm_set1_epi16(64);
    shuffle_c = _mm_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);

    step2_height = COM_MIN(step2_height, height);
    if (width == 4) {
        for (j = 0; j < step2_height; j++, d += 11) {
            int step1_width = (int)(((j + 1) << 8) / 93.0 + 0.9999) - 1;
            int offsetx = (d << 3) & 0x1f;
            pel *px = src - (d >> 2);
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;

            step1_width = COM_MIN(step1_width, width);

            for (i = 0; i < step1_width; i++) {
                pel *py = psrc - j + xsteps[i];
                int offsety = xoffsets[i];
                dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
            }

            px += i;

            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }
    }
    else {
        for (j = 0; j < step2_height; j++, d += 11) {
            int step1_width = (int)(((j + 1) << 8) / 93.0 + 0.9999) - 1;
            int offsetx = (d << 3) & 0x1f;
            pel *px = src - (d >> 2);
            int a = 32 - offsetx, b = 64 - offsetx, c = 32 + offsetx;
            pel *py = psrc - j - 2;
            u8 *psteps = xsteps;
            __m256i shuffle_x = _mm256_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6, 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
            __m256i coeff_x = _mm256_set1_epi32(offsetx + (c << 8) + (b << 16) + (a << 24));

            step1_width = COM_MIN(step1_width, width);
            for (i = 0; i < step1_width; i += 4, psteps += 4) {
                IPRED_ANG_XY_Y4(py, psteps, xoffsets + i, shuffle_c, sign, c0, c_64, 7, dst + i)
            }

            px += step1_width;

            for (i = step1_width; i + 7 < width; i += 8, px += 8) {
                IPRED_ANG_XY_X8(px, shuffle_x, coeff_x, c_64, 7, dst + i)
            }
            for (; i + 3 < width; i += 4, px += 4) {
                IPRED_ANG_XY_X4(px, _mm256_castsi256_si128(shuffle_x), _mm256_castsi256_si128(coeff_x), c_64, 7, dst + i)
            }
            for (; i < width; i++, px++) {
                dst[i] = (px[2] * a + px[1] * b + px[0] * c + px[-1] * offsetx + 64) >> 7;
            }
            dst += i_dst;
        }
    }
    {
        __m256i shuffle_c_256 = _mm256_set_m128i(shuffle_c, shuffle_c);
        __m256i sign_256 = _mm256_set_m128i(sign, sign);
        __m256i c0_256 = _mm256_set_m128i(c0, c0);
        for (; j < height; j++) {
            pel *py = psrc - j - 2;
            u8 *psteps = xsteps;
            for (i = 0; i + 4 < width; i += 8, psteps += 8) {
                IPRED_ANG_XY_Y8(py, psteps, xoffsets + i, shuffle_c_256, sign_256, c0_256, c_64, 7, dst + i)
            }
            if (i < width) {
                IPRED_ANG_XY_Y4(py, psteps, xoffsets + i, shuffle_c, sign, c0, c_64, 7, dst + i)
            }
            dst += i_dst;
        }
    }
}

void uavs3d_ipred_ang_xy_22_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i;
    src -= height;

    if (width != 4) {
        ALIGNED_16(pel first_line[64 + 256]);
        int left_size = (height - 1) * 4 + 3;
        int top_size = width - 3;
        int line_size = left_size + top_size;
        pel *pfirst = first_line + left_size - 3;
        pel *src1 = src;

        __m256i zero = _mm256_setzero_si256();
        __m256i coeff2 = _mm256_set1_epi16(2);
        __m256i coeff3 = _mm256_set1_epi16(3);
        __m256i coeff4 = _mm256_set1_epi16(4);
        __m256i coeff5 = _mm256_set1_epi16(5);
        __m256i coeff7 = _mm256_set1_epi16(7);
        __m256i coeff8 = _mm256_set1_epi16(8);
        __m256i shuffle = _mm256_setr_epi8(0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15, 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15);

        for (i = 0; i < line_size - 32; i += 64, src += 16) {
            __m256i p00, p10, p20, p30;
            __m256i M1, M3, M5, M7;
            __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
            __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
            __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));
            __m256i S3 = _mm256_set_epi64x(0, *(s64 *)(src + 10), 0, *(s64 *)(src + 2));

            __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
            __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
            __m256i L2 = _mm256_unpacklo_epi8(S2, zero);
            __m256i L3 = _mm256_unpacklo_epi8(S3, zero);

            p00 = _mm256_mullo_epi16(L0, coeff3);
            p10 = _mm256_mullo_epi16(L1, coeff7);
            p20 = _mm256_mullo_epi16(L2, coeff5);
            p30 = _mm256_add_epi16(L3, coeff8);
            p00 = _mm256_add_epi16(p00, p30);
            p00 = _mm256_add_epi16(p00, p10);
            p00 = _mm256_add_epi16(p00, p20);
            M1 = _mm256_srli_epi16(p00, 4);

            p00 = _mm256_add_epi16(L1, L2);
            p00 = _mm256_mullo_epi16(p00, coeff3);
            p10 = _mm256_add_epi16(L0, L3);
            p10 = _mm256_add_epi16(p10, coeff4);
            p00 = _mm256_add_epi16(p10, p00);
            M3 = _mm256_srli_epi16(p00, 3);

            p10 = _mm256_mullo_epi16(L1, coeff5);
            p20 = _mm256_mullo_epi16(L2, coeff7);
            p30 = _mm256_mullo_epi16(L3, coeff3);
            p00 = _mm256_add_epi16(L0, coeff8);
            p00 = _mm256_add_epi16(p00, p10);
            p00 = _mm256_add_epi16(p00, p20);
            p00 = _mm256_add_epi16(p00, p30);
            M5 = _mm256_srli_epi16(p00, 4);

            p00 = _mm256_add_epi16(L1, L2);
            p10 = _mm256_add_epi16(L2, L3);
            p00 = _mm256_add_epi16(p00, p10);
            p00 = _mm256_add_epi16(p00, coeff2);
            M7 = _mm256_srli_epi16(p00, 2);

            M1 = _mm256_packus_epi16(M1, M3);
            M5 = _mm256_packus_epi16(M5, M7);
            M1 = _mm256_shuffle_epi8(M1, shuffle);
            M5 = _mm256_shuffle_epi8(M5, shuffle);

            M3 = _mm256_unpacklo_epi16(M1, M5);
            M7 = _mm256_unpackhi_epi16(M1, M5);

            _mm_store_si128((__m128i *)&first_line[i], _mm256_castsi256_si128(M3));
            _mm_store_si128((__m128i *)&first_line[16 + i], _mm256_castsi256_si128(M7));
            _mm_store_si128((__m128i *)&first_line[32 + i], _mm256_extracti128_si256(M3, 1));
            _mm_store_si128((__m128i *)&first_line[48 + i], _mm256_extracti128_si256(M7, 1));
        }

        if (i < left_size) {
            __m128i p00, p10, p20, p30;
            __m128i M1, M3, M5, M7;
            __m128i zero = _mm_setzero_si128();
            __m128i coeff2 = _mm_set1_epi16(2);
            __m128i coeff3 = _mm_set1_epi16(3);
            __m128i coeff4 = _mm_set1_epi16(4);
            __m128i coeff5 = _mm_set1_epi16(5);
            __m128i coeff7 = _mm_set1_epi16(7);
            __m128i coeff8 = _mm_set1_epi16(8);
            __m128i shuffle = _mm_setr_epi8(0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15);
            __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
            __m128i S3 = _mm_loadu_si128((__m128i *)(src + 2));
            __m128i S1 = _mm_loadu_si128((__m128i *)(src));
            __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));

            __m128i L0 = _mm_unpacklo_epi8(S0, zero);
            __m128i L1 = _mm_unpacklo_epi8(S1, zero);
            __m128i L2 = _mm_unpacklo_epi8(S2, zero);
            __m128i L3 = _mm_unpacklo_epi8(S3, zero);

            p00 = _mm_mullo_epi16(L0, coeff3);
            p10 = _mm_mullo_epi16(L1, coeff7);
            p20 = _mm_mullo_epi16(L2, coeff5);
            p30 = _mm_add_epi16(L3, coeff8);
            p00 = _mm_add_epi16(p00, p30);
            p00 = _mm_add_epi16(p00, p10);
            p00 = _mm_add_epi16(p00, p20);
            M1 = _mm_srli_epi16(p00, 4);

            p00 = _mm_add_epi16(L1, L2);
            p00 = _mm_mullo_epi16(p00, coeff3);
            p10 = _mm_add_epi16(L0, L3);
            p10 = _mm_add_epi16(p10, coeff4);
            p00 = _mm_add_epi16(p10, p00);
            M3 = _mm_srli_epi16(p00, 3);

            p10 = _mm_mullo_epi16(L1, coeff5);
            p20 = _mm_mullo_epi16(L2, coeff7);
            p30 = _mm_mullo_epi16(L3, coeff3);
            p00 = _mm_add_epi16(L0, coeff8);
            p00 = _mm_add_epi16(p00, p10);
            p00 = _mm_add_epi16(p00, p20);
            p00 = _mm_add_epi16(p00, p30);
            M5 = _mm_srli_epi16(p00, 4);

            p00 = _mm_add_epi16(L1, L2);
            p10 = _mm_add_epi16(L2, L3);
            p00 = _mm_add_epi16(p00, p10);
            p00 = _mm_add_epi16(p00, coeff2);
            M7 = _mm_srli_epi16(p00, 2);

            M1 = _mm_packus_epi16(M1, M3);
            M5 = _mm_packus_epi16(M5, M7);
            M1 = _mm_shuffle_epi8(M1, shuffle);
            M5 = _mm_shuffle_epi8(M5, shuffle);

            M3 = _mm_unpacklo_epi16(M1, M5);
            M7 = _mm_unpackhi_epi16(M1, M5);

            _mm_store_si128((__m128i *)&first_line[i], M3);
            _mm_store_si128((__m128i *)&first_line[16 + i], M7);
        }

        src = src1 + height;

        for (i = left_size; i < line_size - 8; i += 16, src += 16) {
            __m256i S0 = _mm256_set_epi64x(0, *(s64 *)(src + 7), 0, *(s64 *)(src - 1));
            __m256i S1 = _mm256_set_epi64x(0, *(s64 *)(src + 8), 0, *(s64 *)(src));
            __m256i S2 = _mm256_set_epi64x(0, *(s64 *)(src + 9), 0, *(s64 *)(src + 1));

            __m256i L0 = _mm256_unpacklo_epi8(S0, zero);
            __m256i L1 = _mm256_unpacklo_epi8(S1, zero);
            __m256i L2 = _mm256_unpacklo_epi8(S2, zero);

            __m256i sum1 = _mm256_add_epi16(L0, L1);
            __m256i sum2 = _mm256_add_epi16(L1, L2);
            __m128i d0;

            sum1 = _mm256_add_epi16(sum1, sum2);
            sum1 = _mm256_add_epi16(sum1, coeff2);
            sum1 = _mm256_srli_epi16(sum1, 2);

            d0 = _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1));

            _mm_storeu_si128((__m128i *)&first_line[i], d0);
        }

        if (i < line_size) {
            __m128i zero = _mm_setzero_si128();
            __m128i coeff2 = _mm_set1_epi16(2);
            __m128i S0 = _mm_loadu_si128((__m128i *)(src - 1));
            __m128i S2 = _mm_loadu_si128((__m128i *)(src + 1));
            __m128i S1 = _mm_loadu_si128((__m128i *)(src));

            __m128i L0 = _mm_unpacklo_epi8(S0, zero);
            __m128i L1 = _mm_unpacklo_epi8(S1, zero);
            __m128i L2 = _mm_unpacklo_epi8(S2, zero);

            __m128i sum1 = _mm_add_epi16(L0, L1);
            __m128i sum2 = _mm_add_epi16(L1, L2);

            sum1 = _mm_add_epi16(sum1, sum2);
            sum1 = _mm_add_epi16(sum1, coeff2);
            sum1 = _mm_srli_epi16(sum1, 2);

            sum1 = _mm_packus_epi16(sum1, sum1);
            _mm_storel_epi64((__m128i *)&first_line[i], sum1);
        }

#define COPY_XY22(w) \
        while (height--) {                            \
            memcpy(dst, pfirst, w * sizeof(pel)); \
            dst += i_dst;                             \
            pfirst -= 4;                              \
        }

        switch (width) {
        case 8:
            while (height--) {
                CP64(dst, pfirst);
                dst += i_dst;
                pfirst -= 4;
            }
            break;
        case 16:
            while (height--) {
                CP128(dst, pfirst);
                dst += i_dst;
                pfirst -= 4;
            }
            break;
        case 32:
            COPY_XY22(32)
                break;
        case 64:
            COPY_XY22(64)
                break;
        default:
            uavs3d_assert(0);
            break;
        }
    }
    else {
        dst += (height - 1) * i_dst;
        for (i = 0; i < height; i++, src++) {
            dst[0] = (src[-1] * 3 + src[0] * 7 + src[1] * 5 + src[2] + 8) >> 4;
            dst[1] = (src[-1] + (src[0] + src[1]) * 3 + src[2] + 4) >> 3;
            dst[2] = (src[-1] + src[0] * 5 + src[1] * 7 + src[2] * 3 + 8) >> 4;
            dst[3] = (src[0] + src[1] * 2 + src[2] + 2) >> 2;
            dst -= i_dst;
        }
    }
}

void uavs3d_ipred_ang_xy_23_avx2(pel *src, pel *dst, int i_dst, int mode, int width, int height)
{
    int i, j;
    pel *psrc = src;
    u8 *xoffsets = uavs3d_ipred_offsets[23-3][1];
    u8 *xsteps = uavs3d_ipred_steps[23-3][1];
    int d = 8;
    int step2_height = (width >> 3);

    __m128i shuffle_c;
    __m128i sign = _mm_set_epi32(0x01010101, 0x01010101, -1, -1);
    __m128i c0, c_64;
    c0 = _mm_set_epi8(0, 0, 0, 0, 32, 32, 32, 32, 64, 64, 64, 64, 32, 32, 32, 32);
    c_64 = _mm_set1_epi16(64);
    shuffle_c = _mm_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);

    if (width == 4) {
        for (j = 0; j < step2_height; j++, d += 8) {
            int step1_width = ((j + 1) << 3) - 1;
            pel *px = src - d;

            step1_width = COM_MIN(step1_width, width);

            for (i = 0; i < step1_width; i++) {
                pel *py = psrc - j + xsteps[i];
                int offsety = xoffsets[i];
                dst[i] = (py[-2] * (32 - offsety) + py[-1] * (64 - offsety) + py[0] * (32 + offsety) + py[1] * offsety + 64) >> 7;
            }
            px += i;
            for (; i < width; i++, px++) {
                dst[i] = (px[2] + (px[1] << 1) + px[0] + 2) >> 2;
            }
            dst += i_dst;
        }
    }
    else {
        for (j = 0; j < step2_height; j++, d += 8) {
            int step1_width = ((j + 1) << 3) - 1;
            pel *px = src - d;
            pel *py = psrc - j - 2;
            u8 *psteps = xsteps;
            __m256i shuffle_x = _mm256_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6, 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
            __m256i coeff_x = _mm256_set1_epi32(1 + (2 << 8) + (1 << 16));
            __m128i c_2 = _mm_set1_epi16(2);

            step1_width = COM_MIN(step1_width, width);
            for (i = 0; i < step1_width; i += 4, psteps += 4) {
                IPRED_ANG_XY_Y4(py, psteps, xoffsets + i, shuffle_c, sign, c0, c_64, 7, dst + i)
            }

            px += step1_width + 1;

            for (i = step1_width; i + 7 < width; i += 8, px += 8) {
                IPRED_ANG_XY_X8(px, shuffle_x, coeff_x, c_2, 2, dst + i)
            }
            for (; i < width; i++, px++) {
                dst[i] = (px[1] + (px[0] << 1) + px[-1] + 2) >> 2;
            }
            dst += i_dst;
        }
    }
    {
        __m256i shuffle_c_256 = _mm256_set_m128i(shuffle_c, shuffle_c);
        __m256i sign_256 = _mm256_set_m128i(sign, sign);
        __m256i c0_256 = _mm256_set_m128i(c0, c0);
        for (; j < height; j++) {
            pel *py = psrc - j - 2;
            u8 *psteps = xsteps;
            for (i = 0; i + 4 < width; i += 8, psteps += 8) {
                IPRED_ANG_XY_Y8(py, psteps, xoffsets + i, shuffle_c_256, sign_256, c0_256, c_64, 7, dst + i)
            }
            if (i < width) {
                IPRED_ANG_XY_Y4(py, psteps, xoffsets + i, shuffle_c, sign, c0, c_64, 7, dst + i)
            }
            dst += i_dst;
        }
    }
}

#else
void uavs3d_ipred_ver_avx2(pel *src, pel *dst, int i_dst, int width, int height)
{
    int y;
    switch (width) {
    case 4: {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        for (y = 0; y < height; y += 4) {
            CP64(dst, src);
            CP64(dst + i_dst, src);
            CP64(dst + i_dst2, src);
            CP64(dst + i_dst3, src);
            dst += i_dst4;
        }
        break;
    }
    case 8: {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        __m128i T0;
        T0 = _mm_loadu_si128((__m128i *)src);
        for (y = 0; y < height; y += 4) {
            _mm_storeu_si128((__m128i *)(dst), T0);
            _mm_storeu_si128((__m128i *)(dst + i_dst), T0);
            _mm_storeu_si128((__m128i *)(dst + i_dst2), T0);
            _mm_storeu_si128((__m128i *)(dst + i_dst3), T0);
            dst += i_dst4;
        }
        break;
    }
    case 16: {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        __m256i T0;
        T0 = _mm256_loadu_si256((__m256i *)(src));
        for (y = 0; y < height; y += 4) {
            _mm256_storeu_si256((__m256i *)(dst), T0);
            _mm256_storeu_si256((__m256i *)(dst + i_dst), T0);
            _mm256_storeu_si256((__m256i *)(dst + i_dst2), T0);
            _mm256_storeu_si256((__m256i *)(dst + i_dst3), T0);
            dst += i_dst4;
        }
        break;
    }
    case 32: {
        __m256i T0, T1;
        int i_dst2 = i_dst << 1;
        T0 = _mm256_loadu_si256((__m256i *)(src));
        T1 = _mm256_loadu_si256((__m256i *)(src + 16));
        for (y = 0; y < height; y += 2) {
            _mm256_storeu_si256((__m256i *)(dst), T0);
            _mm256_storeu_si256((__m256i *)(dst + 16), T1);
            _mm256_storeu_si256((__m256i *)(dst + i_dst), T0);
            _mm256_storeu_si256((__m256i *)(dst + i_dst + 16), T1);
            dst += i_dst2;
        }
        break;
    }
    case 64: {
        __m256i T0, T1, T2, T3;
        int i_dst2 = i_dst << 1;
        T0 = _mm256_loadu_si256((__m256i *)(src));
        T1 = _mm256_loadu_si256((__m256i *)(src + 16));
        T2 = _mm256_loadu_si256((__m256i *)(src + 32));
        T3 = _mm256_loadu_si256((__m256i *)(src + 48));
        for (y = 0; y < height; y += 2) {
            _mm256_storeu_si256((__m256i *)(dst), T0);
            _mm256_storeu_si256((__m256i *)(dst + 16), T1);
            _mm256_storeu_si256((__m256i *)(dst + 32), T2);
            _mm256_storeu_si256((__m256i *)(dst + 48), T3);
            _mm256_storeu_si256((__m256i *)(dst + i_dst), T0);
            _mm256_storeu_si256((__m256i *)(dst + i_dst + 16), T1);
            _mm256_storeu_si256((__m256i *)(dst + i_dst + 32), T2);
            _mm256_storeu_si256((__m256i *)(dst + i_dst + 48), T3);
            dst += i_dst2;
        }
        break;
    }
    default:
        uavs3d_assert(0);
        break;
    }
}

void uavs3d_ipred_hor_avx2(pel *src, pel *dst, int i_dst, int width, int height)
{
    int y;
    switch (width) {
    case 4: {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        for (y = 0; y < height; y += 4) {
            M64(dst) = 0x0001000100010001 * src[-y];
            M64(dst + i_dst) = 0x0001000100010001 * src[-y - 1];
            M64(dst + i_dst2) = 0x0001000100010001 * src[-y - 2];
            M64(dst + i_dst3) = 0x0001000100010001 * src[-y - 3];
            dst += i_dst4;
        }
        break;
    }
    case 8: {
        __m128i T0, T1, T2, T3;
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        for (y = 0; y < height; y += 4) {
            T0 = _mm_set1_epi16(src[-y]);
            T1 = _mm_set1_epi16(src[-y - 1]);
            T2 = _mm_set1_epi16(src[-y - 2]);
            T3 = _mm_set1_epi16(src[-y - 3]);
            _mm_storeu_si128((__m128i *)(dst), T0);
            _mm_storeu_si128((__m128i *)(dst + i_dst), T1);
            _mm_storeu_si128((__m128i *)(dst + i_dst2), T2);
            _mm_storeu_si128((__m128i *)(dst + i_dst3), T3);
            dst += i_dst4;
        }
        break;
    }
    case 16: {
        __m256i T0, T1;
        int i_dst2 = i_dst << 1;
        for (y = 0; y < height; y += 2) {
            T0 = _mm256_set1_epi16((short)src[-y]);
            T1 = _mm256_set1_epi16((short)src[-y - 1]);
            _mm256_storeu_si256((__m256i *)(dst), T0);
            _mm256_storeu_si256((__m256i *)(dst + i_dst), T1);
            dst += i_dst2;
        }
        break;
    }
    case 32: {
        __m256i T0, T1;
        int i_dst2 = i_dst << 1;
        for (y = 0; y < height; y += 2) {
            T0 = _mm256_set1_epi16(src[-y]);
            T1 = _mm256_set1_epi16(src[-y - 1]);
            _mm256_storeu_si256((__m256i *)(dst), T0);
            _mm256_storeu_si256((__m256i *)(dst + 16), T0);
            _mm256_storeu_si256((__m256i *)(dst + i_dst), T1);
            _mm256_storeu_si256((__m256i *)(dst + i_dst + 16), T1);
            dst += i_dst2;
        }
        break;
    }
    case 64: {
        __m256i T0, T1;
        int i_dst2 = i_dst << 1;
        for (y = 0; y < height; y += 2) {
            T0 = _mm256_set1_epi16(src[-y]);
            T1 = _mm256_set1_epi16(src[-y - 1]);
            _mm256_storeu_si256((__m256i *)(dst), T0);
            _mm256_storeu_si256((__m256i *)(dst + 16), T0);
            _mm256_storeu_si256((__m256i *)(dst + 32), T0);
            _mm256_storeu_si256((__m256i *)(dst + 48), T0);
            _mm256_storeu_si256((__m256i *)(dst + i_dst), T1);
            _mm256_storeu_si256((__m256i *)(dst + i_dst + 16), T1);
            _mm256_storeu_si256((__m256i *)(dst + i_dst + 32), T1);
            _mm256_storeu_si256((__m256i *)(dst + i_dst + 48), T1);
            dst += i_dst2;
        }
        break;
    }
    default:
        uavs3d_assert(0);
        break;
    }
}

void uavs3d_ipred_dc_avx2(pel *src, pel *dst, int i_dst, int width, int height, u16 avail_cu, int bit_depth)
{
    int   i, x, y;
    int   dc;
    pel  *p_src = src - 1;
    int left_avail = IS_AVAIL(avail_cu, AVAIL_LE);
    int above_avail = IS_AVAIL(avail_cu, AVAIL_UP);

    if (left_avail && above_avail) {
        int length = width + height + 1;
        __m128i zero = _mm_setzero_si128();
        __m128i sum = zero;
        __m128i val;

        p_src = src - height;

        for (i = 0; i < length - 7; i += 8) {
            val = _mm_loadu_si128((__m128i *)(p_src + i));
            sum = _mm_add_epi16(sum, val);
        }
        if (i < length) {
            int left_pixels = length - i;
            __m128i mask = _mm_load_si128((__m128i *)(uavs3d_simd_mask[left_pixels - 1]));
            val = _mm_loadu_si128((__m128i *)(p_src + i));
            val = _mm_and_si128(val, mask);
            sum = _mm_add_epi16(sum, val);
        }
        val = _mm_unpackhi_epi16(sum, zero);
        sum = _mm_unpacklo_epi16(sum, zero);
        sum = _mm_add_epi32(sum, val);
        sum = _mm_hadd_epi32(sum, sum);
        dc = _mm_extract_epi32(sum, 0) + _mm_extract_epi32(sum, 1) + ((width + height) >> 1) - src[0];

        dc = (dc * (4096 / (width + height))) >> 12;

    }
    else if (left_avail) {
        dc = 0;
        for (y = 0; y < height; y++) {
            dc += p_src[-y];
        }
        dc += height / 2;
        dc /= height;
    }
    else {
        p_src = src + 1;
        dc = 0;
        if (above_avail) {
            for (x = 0; x < width; x++) {
                dc += p_src[x];
            }
            dc += width / 2;
            dc /= width;
        }
        else {
            dc = 1 << (bit_depth - 1);
        }
    }

    switch (width) {
    case 4: {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        u64 v64 = 0x0001000100010001 * dc;
        for (y = 0; y < height; y += 4) {
            M64(dst) = v64;
            M64(dst + i_dst) = v64;
            M64(dst + i_dst2) = v64;
            M64(dst + i_dst3) = v64;
            dst += i_dst4;
        }
        break;
    }
    case 8: {
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        __m128i T = _mm_set1_epi16((s16)dc);
        for (y = 0; y < height; y += 4) {
            _mm_storeu_si128((__m128i *)(dst), T);
            _mm_storeu_si128((__m128i *)(dst + i_dst), T);
            _mm_storeu_si128((__m128i *)(dst + i_dst2), T);
            _mm_storeu_si128((__m128i *)(dst + i_dst3), T);
            dst += i_dst4;
        }
        break;
    }
    case 16: {
        __m256i T = _mm256_set1_epi16((s16)dc);
        int i_dst2 = i_dst << 1;
        int i_dst3 = i_dst + i_dst2;
        int i_dst4 = i_dst << 2;
        for (y = 0; y < height; y += 4) {
            _mm256_storeu_si256((__m256i *)(dst), T);
            _mm256_storeu_si256((__m256i *)(dst + i_dst), T);
            _mm256_storeu_si256((__m256i *)(dst + i_dst2), T);
            _mm256_storeu_si256((__m256i *)(dst + i_dst3), T);
            dst += i_dst4;
        }
        break;
    }
    case 32: {
        int i_dst2 = i_dst << 1;
        __m256i T = _mm256_set1_epi16((s16)dc);
        for (y = 0; y < height; y += 2) {
            _mm256_storeu_si256((__m256i *)(dst), T);
            _mm256_storeu_si256((__m256i *)(dst + 16), T);
            _mm256_storeu_si256((__m256i *)(dst + i_dst), T);
            _mm256_storeu_si256((__m256i *)(dst + i_dst + 16), T);
            dst += i_dst2;
        }
        break;
    }
    case 64: {
        __m256i T = _mm256_set1_epi16((s16)dc);
        for (y = 0; y < height; y++) {
            _mm256_storeu_si256((__m256i *)(dst), T);
            _mm256_storeu_si256((__m256i *)(dst + 16), T);
            _mm256_storeu_si256((__m256i *)(dst + 32), T);
            _mm256_storeu_si256((__m256i *)(dst + 48), T);
            dst += i_dst;
        }
        break;
    }
    default:
        uavs3d_assert(0);
        break;
    }
}

#endif
