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

#define TRANSPOSE_8x4_16BIT(I0, I1, I2, I3, I4, I5, I6, I7, O0, O1, O2, O3) \
	    tr0_0 = _mm_unpacklo_epi16(I0, I1); \
	    tr0_1 = _mm_unpacklo_epi16(I2, I3); \
	    tr0_2 = _mm_unpacklo_epi16(I4, I5); \
	    tr0_3 = _mm_unpacklo_epi16(I6, I7); \
	    tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1); \
	    tr1_1 = _mm_unpackhi_epi32(tr0_0, tr0_1); \
	    tr1_2 = _mm_unpacklo_epi32(tr0_2, tr0_3); \
	    tr1_3 = _mm_unpackhi_epi32(tr0_2, tr0_3); \
	    O0 = _mm_unpacklo_epi64(tr1_0, tr1_2); \
	    O1 = _mm_unpackhi_epi64(tr1_0, tr1_2); \
	    O2 = _mm_unpacklo_epi64(tr1_1, tr1_3); \
	    O3 = _mm_unpackhi_epi64(tr1_1, tr1_3); 

// transpose 8x8: 8 x 8(32bit) --> 8 x 8(16bit)
// O0: row0, row4
// O1: row1, row5
// O2: row2, row6
// O3: row3, row7
#define TRANSPOSE_8x8_32BIT_16BIT(I0, I1, I2, I3, I4, I5, I6, I7, O0, O1, O2, O3) \
        I0 = _mm256_packs_epi32(I0, I4);    \
        I1 = _mm256_packs_epi32(I1, I5);    \
        I2 = _mm256_packs_epi32(I2, I6);    \
        I3 = _mm256_packs_epi32(I3, I7);    \
        I4 = _mm256_unpacklo_epi16(I0, I2); \
        I5 = _mm256_unpackhi_epi16(I0, I2); \
        I6 = _mm256_unpacklo_epi16(I1, I3); \
        I7 = _mm256_unpackhi_epi16(I1, I3); \
        I0 = _mm256_unpacklo_epi16(I4, I6); \
        I1 = _mm256_unpackhi_epi16(I4, I6); \
        I2 = _mm256_unpacklo_epi16(I5, I7); \
        I3 = _mm256_unpackhi_epi16(I5, I7); \
        O0 = _mm256_unpacklo_epi64(I0, I2); \
        O1 = _mm256_unpackhi_epi64(I0, I2); \
        O2 = _mm256_unpacklo_epi64(I1, I3); \
        O3 = _mm256_unpackhi_epi64(I1, I3)

// transpose 8x8: 16 x 8(32bit) --> 8 x 16(16bit)
#define TRANSPOSE_16x8_32BIT_16BIT(I00, I01, I02, I03, I04, I05, I06, I07, I08, I09, I10, I11, I12, I13, I14, I15, O0, O1, O2, O3, O4, O5, O6, O7)\
        TRANSPOSE_8x8_32BIT_16BIT(I00, I01, I02, I03, I04, I05, I06, I07, I04, I05, I06, I07); \
        TRANSPOSE_8x8_32BIT_16BIT(I08, I09, I10, I11, I12, I13, I14, I15, I12, I13, I14, I15); \
        O0 = _mm256_permute2x128_si256(I04, I12, 0x20);      \
        O1 = _mm256_permute2x128_si256(I05, I13, 0x20);      \
        O2 = _mm256_permute2x128_si256(I06, I14, 0x20);      \
        O3 = _mm256_permute2x128_si256(I07, I15, 0x20);      \
        O4 = _mm256_permute2x128_si256(I04, I12, 0x31); \
        O5 = _mm256_permute2x128_si256(I05, I13, 0x31); \
        O6 = _mm256_permute2x128_si256(I06, I14, 0x31); \
        O7 = _mm256_permute2x128_si256(I07, I15, 0x31)


static void uavs3d_always_inline dct2_butterfly_h4_avx2(s16* src, s16* dst, int line, int shift, int bit_depth)
{
    __m128i s0, s1, s2, s3;
    __m128i ss0, ss1, ss2, ss3;
    __m256i e0, e1, o0, o1, t0, t1;
    __m256i v0, v1, v2, v3;
    const __m256i c16_p17_p42 = _mm256_set1_epi32(0x0011002A);
    const __m256i c16_n42_p17 = _mm256_set1_epi32(0xFFD60011);
    const __m256i c16_n32_p32 = _mm256_set1_epi32(0xFFe00020);
    const __m256i c16_p32_p32 = _mm256_set1_epi32(0x00200020);
    __m256i off = _mm256_set1_epi32(1 << (shift - 1));
    int j;
    int i_src = line;
    int i_src2 = line << 1;
    int i_src3 = i_src + i_src2;

    uavs3d_assert(line >= 8);

    for (j = 0; j < line; j += 8)
    {
        s0 = _mm_load_si128((__m128i*)(src + j));
        s1 = _mm_load_si128((__m128i*)(src + i_src + j));
        s2 = _mm_load_si128((__m128i*)(src + i_src2 + j));
        s3 = _mm_load_si128((__m128i*)(src + i_src3 + j));
        ss0 = _mm_unpacklo_epi16(s0, s2);
        ss1 = _mm_unpackhi_epi16(s0, s2);
        ss2 = _mm_unpacklo_epi16(s1, s3);
        ss3 = _mm_unpackhi_epi16(s1, s3);

        t0 = _mm256_set_m128i(ss1, ss0);
        t1 = _mm256_set_m128i(ss3, ss2);

        e0 = _mm256_madd_epi16(t0, c16_p32_p32);
        e1 = _mm256_madd_epi16(t0, c16_n32_p32);
        o0 = _mm256_madd_epi16(t1, c16_p17_p42);
        o1 = _mm256_madd_epi16(t1, c16_n42_p17);
        v0 = _mm256_add_epi32(e0, o0);
        v1 = _mm256_add_epi32(e1, o1);
        v2 = _mm256_sub_epi32(e1, o1);
        v3 = _mm256_sub_epi32(e0, o0);

        v0 = _mm256_add_epi32(v0, off);
        v1 = _mm256_add_epi32(v1, off);
        v2 = _mm256_add_epi32(v2, off);
        v3 = _mm256_add_epi32(v3, off);

        v0 = _mm256_srai_epi32(v0, shift);
        v1 = _mm256_srai_epi32(v1, shift);
        v2 = _mm256_srai_epi32(v2, shift);
        v3 = _mm256_srai_epi32(v3, shift);

        v0 = _mm256_packs_epi32(v0, v2);        // 00 10 20 30 02 12 22 32 40 50 60 70 42 52 62 72
        v1 = _mm256_packs_epi32(v1, v3);        // 01 11 21 31 03 13 23 33 41 51 61 71 43 53 63 73

        // inverse
        v2 = _mm256_unpacklo_epi16(v0, v1);     // 00 01 10 11 20 21 30 31 40 41 50 51 60 61 70 71
        v3 = _mm256_unpackhi_epi16(v0, v1);     // 02 03 12 13 22 23 32 33 42 43 52 53 62 63 72 73
        v0 = _mm256_unpacklo_epi32(v2, v3);     // 00 01 02 03 10 11 12 13 40 41 42 43 50 51 52 53
        v1 = _mm256_unpackhi_epi32(v2, v3);     // 20 21 22 23 30 31 32 33

        if (bit_depth != MAX_TX_DYNAMIC_RANGE) {
            __m256i max_val = _mm256_set1_epi16((1 << bit_depth) - 1);
            __m256i min_val = _mm256_set1_epi16(-(1 << bit_depth));
            v0 = _mm256_min_epi16(v0, max_val);
            v1 = _mm256_min_epi16(v1, max_val);
            v0 = _mm256_max_epi16(v0, min_val);
            v1 = _mm256_max_epi16(v1, min_val);
        }
        _mm_store_si128((__m128i*)dst, _mm256_castsi256_si128(v0));
        _mm_store_si128((__m128i*)(dst + 8), _mm256_castsi256_si128(v1));
        _mm_store_si128((__m128i*)(dst + 16), _mm256_extracti128_si256(v0, 1));
        _mm_store_si128((__m128i*)(dst + 24), _mm256_extracti128_si256(v1, 1));
        dst += 32;
    }
}
static void uavs3d_always_inline dct2_butterfly_h8_avx2(s16* src, int i_src, s16* dst, int line, int shift, int bit_depth)
{
    if (line > 4) {
        const __m256i coeff_p44_p38 = _mm256_set1_epi32(0x0026002C);
        const __m256i coeff_p25_p9  = _mm256_set1_epi32(0x00090019);
        const __m256i coeff_p38_n9  = _mm256_set1_epi32(0xFFF70026);
        const __m256i coeff_n44_n25 = _mm256_set1_epi32(0xFFE7FFD4);
        const __m256i coeff_p25_n44 = _mm256_set1_epi32(0xFFD40019);
        const __m256i coeff_p9_p38  = _mm256_set1_epi32(0x00260009);
        const __m256i coeff_p9_n25  = _mm256_set1_epi32(0xFFE70009);
        const __m256i coeff_p38_n44 = _mm256_set1_epi32(0xFFD40026);
        const __m256i coeff_p32_p32 = _mm256_set1_epi32(0x00200020);
        const __m256i coeff_p32_n32 = _mm256_set1_epi32(0xFFE00020);
        const __m256i coeff_p42_p17 = _mm256_set1_epi32(0x0011002A);
        const __m256i coeff_p17_n42 = _mm256_set1_epi32(0xFFD60011);
        __m128i s0, s1, s2, s3, s4, s5, s6, s7;
        __m128i ss0, ss1, ss2, ss3;
        __m256i e0, e1, e2, e3, o0, o1, o2, o3, ee0, ee1, eo0, eo1;
        __m256i t0, t1, t2, t3;
        __m256i d0, d1, d2, d3, d4, d5, d6, d7;
        __m256i offset = _mm256_set1_epi32(1 << (shift - 1));
        int j;
        int i_src2 = i_src << 1;
        int i_src3 = i_src + i_src2;
        int i_src4 = i_src << 2;
        int i_src5 = i_src2 + i_src3;
        int i_src6 = i_src3 << 1;
        int i_src7 = i_src3 + i_src4;
        for (j = 0; j < line; j += 8)
        {
            // O[0] -- O[3]
            s1 = _mm_load_si128((__m128i*)(src + i_src + j));
            s3 = _mm_load_si128((__m128i*)(src + i_src3 + j));
            s5 = _mm_load_si128((__m128i*)(src + i_src5 + j));
            s7 = _mm_load_si128((__m128i*)(src + i_src7 + j));

            ss0 = _mm_unpacklo_epi16(s1, s3);
            ss1 = _mm_unpackhi_epi16(s1, s3);
            ss2 = _mm_unpacklo_epi16(s5, s7);
            ss3 = _mm_unpackhi_epi16(s5, s7);

            e0 = _mm256_set_m128i(ss1, ss0);
            e1 = _mm256_set_m128i(ss3, ss2);

            t0 = _mm256_madd_epi16(e0, coeff_p44_p38);
            t1 = _mm256_madd_epi16(e1, coeff_p25_p9);
            t2 = _mm256_madd_epi16(e0, coeff_p38_n9);
            t3 = _mm256_madd_epi16(e1, coeff_n44_n25);
            o0 = _mm256_add_epi32(t0, t1);
            o1 = _mm256_add_epi32(t2, t3);

            t0 = _mm256_madd_epi16(e0, coeff_p25_n44);
            t1 = _mm256_madd_epi16(e1, coeff_p9_p38);
            t2 = _mm256_madd_epi16(e0, coeff_p9_n25);
            t3 = _mm256_madd_epi16(e1, coeff_p38_n44);

            o2 = _mm256_add_epi32(t0, t1);
            o3 = _mm256_add_epi32(t2, t3);

            // E[0] - E[3]
            s0 = _mm_load_si128((__m128i*)(src + j));
            s2 = _mm_load_si128((__m128i*)(src + i_src2 + j));
            s4 = _mm_load_si128((__m128i*)(src + i_src4 + j));
            s6 = _mm_load_si128((__m128i*)(src + i_src6 + j));

            ss0 = _mm_unpacklo_epi16(s0, s4);
            ss1 = _mm_unpackhi_epi16(s0, s4);
            ss2 = _mm_unpacklo_epi16(s2, s6);
            ss3 = _mm_unpackhi_epi16(s2, s6);

            e0 = _mm256_set_m128i(ss1, ss0);
            e1 = _mm256_set_m128i(ss3, ss2);

            ee0 = _mm256_madd_epi16(e0, coeff_p32_p32);
            ee1 = _mm256_madd_epi16(e0, coeff_p32_n32);
            eo0 = _mm256_madd_epi16(e1, coeff_p42_p17);
            eo1 = _mm256_madd_epi16(e1, coeff_p17_n42);

            e0 = _mm256_add_epi32(ee0, eo0);
            e3 = _mm256_sub_epi32(ee0, eo0);
            e1 = _mm256_add_epi32(ee1, eo1);
            e2 = _mm256_sub_epi32(ee1, eo1);

            e0 = _mm256_add_epi32(e0, offset);
            e3 = _mm256_add_epi32(e3, offset);
            e1 = _mm256_add_epi32(e1, offset);
            e2 = _mm256_add_epi32(e2, offset);

            d0 = _mm256_add_epi32(e0, o0);
            d7 = _mm256_sub_epi32(e0, o0);
            d1 = _mm256_add_epi32(e1, o1);
            d6 = _mm256_sub_epi32(e1, o1);
            d2 = _mm256_add_epi32(e2, o2);
            d5 = _mm256_sub_epi32(e2, o2);
            d3 = _mm256_add_epi32(e3, o3);
            d4 = _mm256_sub_epi32(e3, o3);

            d0 = _mm256_srai_epi32(d0, shift);
            d7 = _mm256_srai_epi32(d7, shift);
            d1 = _mm256_srai_epi32(d1, shift);
            d6 = _mm256_srai_epi32(d6, shift);
            d2 = _mm256_srai_epi32(d2, shift);
            d5 = _mm256_srai_epi32(d5, shift);
            d3 = _mm256_srai_epi32(d3, shift);
            d4 = _mm256_srai_epi32(d4, shift);

            // transpose 8x8 : 8 x 8(32bit) --> 4 x 16(16bit)
            TRANSPOSE_8x8_32BIT_16BIT(d0, d1, d2, d3, d4, d5, d6, d7, d4, d5, d6, d7);
            d0 = _mm256_permute2x128_si256(d4, d5, 0x20);
            d2 = _mm256_permute2x128_si256(d4, d5, 0x31);
            d1 = _mm256_permute2x128_si256(d6, d7, 0x20);
            d3 = _mm256_permute2x128_si256(d6, d7, 0x31);

            if (bit_depth != MAX_TX_DYNAMIC_RANGE) {
                __m256i max_val = _mm256_set1_epi16((1 << bit_depth) - 1);
                __m256i min_val = _mm256_set1_epi16(-(1 << bit_depth));
                d4 = _mm256_min_epi16(d4, max_val);
                d5 = _mm256_min_epi16(d5, max_val);
                d6 = _mm256_min_epi16(d6, max_val);
                d7 = _mm256_min_epi16(d7, max_val);
                d4 = _mm256_max_epi16(d4, min_val);
                d5 = _mm256_max_epi16(d5, min_val);
                d6 = _mm256_max_epi16(d6, min_val);
                d7 = _mm256_max_epi16(d7, min_val);
            }
            // store line x 8
            _mm256_store_si256((__m256i*)dst, d0);
            _mm256_store_si256((__m256i*)(dst + 16), d1);
            _mm256_store_si256((__m256i*)(dst + 32), d2);
            _mm256_store_si256((__m256i*)(dst + 48), d3);
            dst += 64;
        }
    } else { // line == 4
        const __m128i coeff_p44_p38 = _mm_set1_epi32(0x0026002C);
        const __m128i coeff_p25_p9  = _mm_set1_epi32(0x00090019);
        const __m128i coeff_p38_n9  = _mm_set1_epi32(0xFFF70026);
        const __m128i coeff_n44_n25 = _mm_set1_epi32(0xFFE7FFD4);
        const __m128i coeff_p25_n44 = _mm_set1_epi32(0xFFD40019);
        const __m128i coeff_p9_p38  = _mm_set1_epi32(0x00260009);
        const __m128i coeff_p9_n25  = _mm_set1_epi32(0xFFE70009);
        const __m128i coeff_p38_n44 = _mm_set1_epi32(0xFFD40026);
        const __m128i coeff_p32_p32 = _mm_set1_epi32(0x00200020);
        const __m128i coeff_p32_n32 = _mm_set1_epi32(0xFFE00020);
        const __m128i coeff_p42_p17 = _mm_set1_epi32(0x0011002A);
        const __m128i coeff_p17_n42 = _mm_set1_epi32(0xFFD60011);

        __m128i s0, s1, s2, s3, s4, s5, s6, s7;
        __m128i e0, e1, e2, e3, o0, o1, o2, o3, ee0, ee1, eo0, eo1;
        __m128i t0, t1, t2, t3;
        __m128i offset = _mm_set1_epi32(1 << (shift - 1));
        __m128i zero = _mm_setzero_si128();

        // O[0] -- O[3]
        s1 = _mm_loadl_epi64((__m128i*)(src + 4));
        s3 = _mm_loadl_epi64((__m128i*)(src + 12));
        s5 = _mm_loadl_epi64((__m128i*)(src + 20));
        s7 = _mm_loadl_epi64((__m128i*)(src + 28));

        t0 = _mm_unpacklo_epi16(s1, s3);
        t2 = _mm_unpacklo_epi16(s5, s7);

        e1 = _mm_madd_epi16(t0, coeff_p44_p38);
        e2 = _mm_madd_epi16(t2, coeff_p25_p9);
        o0 = _mm_add_epi32(e1, e2);

        e1 = _mm_madd_epi16(t0, coeff_p38_n9);
        e2 = _mm_madd_epi16(t2, coeff_n44_n25);
        o1 = _mm_add_epi32(e1, e2);

        e1 = _mm_madd_epi16(t0, coeff_p25_n44);
        e2 = _mm_madd_epi16(t2, coeff_p9_p38);
        o2 = _mm_add_epi32(e1, e2);

        e1 = _mm_madd_epi16(t0, coeff_p9_n25);
        e2 = _mm_madd_epi16(t2, coeff_p38_n44);
        o3 = _mm_add_epi32(e1, e2);

        // E[0] - E[3]
        s0 = _mm_loadl_epi64((__m128i*)(src));
        s2 = _mm_loadl_epi64((__m128i*)(src + 8));
        s4 = _mm_loadl_epi64((__m128i*)(src + 16));
        s6 = _mm_loadl_epi64((__m128i*)(src + 24));

        t0 = _mm_unpacklo_epi16(s0, s4);
        ee0 = _mm_madd_epi16(t0, coeff_p32_p32);
        ee1 = _mm_madd_epi16(t0, coeff_p32_n32);

        t0 = _mm_unpacklo_epi16(s2, s6);
        eo0 = _mm_madd_epi16(t0, coeff_p42_p17);
        eo1 = _mm_madd_epi16(t0, coeff_p17_n42);
        e0 = _mm_add_epi32(ee0, eo0);
        e3 = _mm_sub_epi32(ee0, eo0);
        e0 = _mm_add_epi32(e0, offset);
        e3 = _mm_add_epi32(e3, offset);

        e1 = _mm_add_epi32(ee1, eo1);
        e2 = _mm_sub_epi32(ee1, eo1);
        e1 = _mm_add_epi32(e1, offset);
        e2 = _mm_add_epi32(e2, offset);
        s0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e0, o0), shift), zero);
        s7 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e0, o0), shift), zero);
        s1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e1, o1), shift), zero);
        s6 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e1, o1), shift), zero);
        s2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e2, o2), shift), zero);
        s5 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e2, o2), shift), zero);
        s3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e3, o3), shift), zero);
        s4 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e3, o3), shift), zero);

        /*  inverse   */
        e0 = _mm_unpacklo_epi16(s0, s4);
        e1 = _mm_unpacklo_epi16(s1, s5);
        e2 = _mm_unpacklo_epi16(s2, s6);
        e3 = _mm_unpacklo_epi16(s3, s7);
        t0 = _mm_unpacklo_epi16(e0, e2);
        t1 = _mm_unpacklo_epi16(e1, e3);
        s0 = _mm_unpacklo_epi16(t0, t1);
        s1 = _mm_unpackhi_epi16(t0, t1);
        t2 = _mm_unpackhi_epi16(e0, e2);
        t3 = _mm_unpackhi_epi16(e1, e3);
        s2 = _mm_unpacklo_epi16(t2, t3);
        s3 = _mm_unpackhi_epi16(t2, t3);

        if (bit_depth != MAX_TX_DYNAMIC_RANGE) {
            __m128i max_val = _mm_set1_epi16((1 << bit_depth) - 1);
            __m128i min_val = _mm_set1_epi16(-(1 << bit_depth));
            s0 = _mm_min_epi16(s0, max_val);
            s1 = _mm_min_epi16(s1, max_val);
            s2 = _mm_min_epi16(s2, max_val);
            s3 = _mm_min_epi16(s3, max_val);
            s0 = _mm_max_epi16(s0, min_val);
            s1 = _mm_max_epi16(s1, min_val);
            s2 = _mm_max_epi16(s2, min_val);
            s3 = _mm_max_epi16(s3, min_val);
        }

        // store line x 8
        _mm_store_si128((__m128i*)dst, s0);
        _mm_store_si128((__m128i*)(dst + 8), s1);
        _mm_store_si128((__m128i*)(dst + 16), s2);
        _mm_store_si128((__m128i*)(dst + 24), s3);
    }
}

void dct2_butterfly_h16_avx2(s16* src, int i_src, s16* dst, int line, int shift, int bit_depth)
{
    const __m256i c16_p43_p45 = _mm256_set1_epi32(0x002B002D);		//row0 
    const __m256i c16_p35_p40 = _mm256_set1_epi32(0x00230028);
    const __m256i c16_p21_p29 = _mm256_set1_epi32(0x0015001D);
    const __m256i c16_p04_p13 = _mm256_set1_epi32(0x0004000D);
    const __m256i c16_p29_p43 = _mm256_set1_epi32(0x001D002B);		//row1
    const __m256i c16_n21_p04 = _mm256_set1_epi32(0xFFEB0004);
    const __m256i c16_n45_n40 = _mm256_set1_epi32(0xFFD3FFD8);
    const __m256i c16_n13_n35 = _mm256_set1_epi32(0xFFF3FFDD);
    const __m256i c16_p04_p40 = _mm256_set1_epi32(0x00040028);		//row2
    const __m256i c16_n43_n35 = _mm256_set1_epi32(0xFFD5FFDD);
    const __m256i c16_p29_n13 = _mm256_set1_epi32(0x001DFFF3);
    const __m256i c16_p21_p45 = _mm256_set1_epi32(0x0015002D);
    const __m256i c16_n21_p35 = _mm256_set1_epi32(0xFFEB0023);		//row3
    const __m256i c16_p04_n43 = _mm256_set1_epi32(0x0004FFD5);
    const __m256i c16_p13_p45 = _mm256_set1_epi32(0x000D002D);
    const __m256i c16_n29_n40 = _mm256_set1_epi32(0xFFE3FFD8);
    const __m256i c16_n40_p29 = _mm256_set1_epi32(0xFFD8001D);		//row4
    const __m256i c16_p45_n13 = _mm256_set1_epi32(0x002DFFF3);
    const __m256i c16_n43_n04 = _mm256_set1_epi32(0xFFD5FFFC);
    const __m256i c16_p35_p21 = _mm256_set1_epi32(0x00230015);
    const __m256i c16_n45_p21 = _mm256_set1_epi32(0xFFD30015);		//row5
    const __m256i c16_p13_p29 = _mm256_set1_epi32(0x000D001D);
    const __m256i c16_p35_n43 = _mm256_set1_epi32(0x0023FFD5);
    const __m256i c16_n40_p04 = _mm256_set1_epi32(0xFFD80004);
    const __m256i c16_n35_p13 = _mm256_set1_epi32(0xFFDD000D);		//row6
    const __m256i c16_n40_p45 = _mm256_set1_epi32(0xFFD8002D);
    const __m256i c16_p04_p21 = _mm256_set1_epi32(0x00040015);
    const __m256i c16_p43_n29 = _mm256_set1_epi32(0x002BFFE3);
    const __m256i c16_n13_p04 = _mm256_set1_epi32(0xFFF30004);		//row7
    const __m256i c16_n29_p21 = _mm256_set1_epi32(0xFFE30015);
    const __m256i c16_n40_p35 = _mm256_set1_epi32(0xFFD80023);
    const __m256i c16_n45_p43 = _mm256_set1_epi32(0xFFD3002B);
    const __m256i c16_p38_p44 = _mm256_set1_epi32(0x0026002C);
    const __m256i c16_p09_p25 = _mm256_set1_epi32(0x00090019);
    const __m256i c16_n09_p38 = _mm256_set1_epi32(0xFFF70026);
    const __m256i c16_n25_n44 = _mm256_set1_epi32(0xFFE7FFD4);
    const __m256i c16_n44_p25 = _mm256_set1_epi32(0xFFD40019);
    const __m256i c16_p38_p09 = _mm256_set1_epi32(0x00260009);
    const __m256i c16_n25_p09 = _mm256_set1_epi32(0xFFE70009);
    const __m256i c16_n44_p38 = _mm256_set1_epi32(0xFFD40026);
    const __m256i c16_p17_p42 = _mm256_set1_epi32(0x0011002A);
    const __m256i c16_n42_p17 = _mm256_set1_epi32(0xFFD60011);
    const __m256i c16_n32_p32 = _mm256_set1_epi32(0xFFE00020);
    const __m256i c16_p32_p32 = _mm256_set1_epi32(0x00200020);
    int i;
    __m256i c32_off = _mm256_set1_epi32(1 << (shift - 1));
    __m128i src00, src01, src02, src03, src04, src05, src06, src07;
    __m128i src08, src09, src10, src11, src12, src13, src14, src15;
    __m128i ss0, ss1, ss2, ss3, ss4, ss5, ss6, ss7;
    __m256i res00, res01, res02, res03, res04, res05, res06, res07;
    __m256i t0, t1, t2, t3, t4, t5, t6, t7;
    __m256i t00, t01, t02, t03;
    __m256i o0, o1, o2, o3, o4, o5, o6, o7;
    __m256i eo0, eo1, eo2, eo3;
    __m256i eeo0, eeo1;
    __m256i eee0, eee1;

    for (i = 0; i < line; i += 8)
    {
        src01 = _mm_load_si128((const __m128i*)&src[1 * i_src + i]);    // [17 16 15 14 13 12 11 10]
        src03 = _mm_load_si128((const __m128i*)&src[3 * i_src + i]);    // [37 36 35 34 33 32 31 30]
        src05 = _mm_load_si128((const __m128i*)&src[5 * i_src + i]);    // [57 56 55 54 53 52 51 50]
        src07 = _mm_load_si128((const __m128i*)&src[7 * i_src + i]);    // [77 76 75 74 73 72 71 70]
        src09 = _mm_load_si128((const __m128i*)&src[9 * i_src + i]);
        src11 = _mm_load_si128((const __m128i*)&src[11 * i_src + i]);
        src13 = _mm_load_si128((const __m128i*)&src[13 * i_src + i]);
        src15 = _mm_load_si128((const __m128i*)&src[15 * i_src + i]);

        ss0 = _mm_unpacklo_epi16(src01, src03);
        ss1 = _mm_unpacklo_epi16(src05, src07);
        ss2 = _mm_unpacklo_epi16(src09, src11);
        ss3 = _mm_unpacklo_epi16(src13, src15);
        ss4 = _mm_unpackhi_epi16(src01, src03);
        ss5 = _mm_unpackhi_epi16(src05, src07);
        ss6 = _mm_unpackhi_epi16(src09, src11);
        ss7 = _mm_unpackhi_epi16(src13, src15);

        t0 = _mm256_set_m128i(ss4, ss0);
        t1 = _mm256_set_m128i(ss5, ss1);
        t2 = _mm256_set_m128i(ss6, ss2);
        t3 = _mm256_set_m128i(ss7, ss3);
#define COMPUTE_ROW(row0103, row0507, row0911, row1315, c0103, c0507, c0911, c1315, row) \
        t00 = _mm256_madd_epi16(row0103, c0103); \
        t01 = _mm256_madd_epi16(row0507, c0507); \
        t02 = _mm256_madd_epi16(row0911, c0911); \
        t03 = _mm256_madd_epi16(row1315, c1315); \
	    t00 = _mm256_add_epi32(t00, t01); \
	    t01 = _mm256_add_epi32(t02, t03); \
	    row = _mm256_add_epi32(t00, t01);

        COMPUTE_ROW(t0, t1, t2, t3, c16_p43_p45, c16_p35_p40, c16_p21_p29, c16_p04_p13, o0)
        COMPUTE_ROW(t0, t1, t2, t3, c16_p29_p43, c16_n21_p04, c16_n45_n40, c16_n13_n35, o1)
        COMPUTE_ROW(t0, t1, t2, t3, c16_p04_p40, c16_n43_n35, c16_p29_n13, c16_p21_p45, o2)
        COMPUTE_ROW(t0, t1, t2, t3, c16_n21_p35, c16_p04_n43, c16_p13_p45, c16_n29_n40, o3)
        COMPUTE_ROW(t0, t1, t2, t3, c16_n40_p29, c16_p45_n13, c16_n43_n04, c16_p35_p21, o4)
        COMPUTE_ROW(t0, t1, t2, t3, c16_n45_p21, c16_p13_p29, c16_p35_n43, c16_n40_p04, o5)
        COMPUTE_ROW(t0, t1, t2, t3, c16_n35_p13, c16_n40_p45, c16_p04_p21, c16_p43_n29, o6)
        COMPUTE_ROW(t0, t1, t2, t3, c16_n13_p04, c16_n29_p21, c16_n40_p35, c16_n45_p43, o7)

#undef COMPUTE_ROW

        src00 = _mm_load_si128((const __m128i*)&src[0 * i_src + i]);    // [07 06 05 04 03 02 01 00]
        src02 = _mm_load_si128((const __m128i*)&src[2 * i_src + i]);    // [27 26 25 24 23 22 21 20]
        src04 = _mm_load_si128((const __m128i*)&src[4 * i_src + i]);    // [47 46 45 44 43 42 41 40]
        src06 = _mm_load_si128((const __m128i*)&src[6 * i_src + i]);    // [67 66 65 64 63 62 61 60]
        src08 = _mm_load_si128((const __m128i*)&src[8 * i_src + i]);
        src10 = _mm_load_si128((const __m128i*)&src[10 * i_src + i]);
        src12 = _mm_load_si128((const __m128i*)&src[12 * i_src + i]);
        src14 = _mm_load_si128((const __m128i*)&src[14 * i_src + i]);

        ss0 = _mm_unpacklo_epi16(src02, src06);
        ss1 = _mm_unpacklo_epi16(src10, src14);
        ss2 = _mm_unpacklo_epi16(src04, src12);
        ss3 = _mm_unpacklo_epi16(src00, src08);
        ss4 = _mm_unpackhi_epi16(src02, src06);
        ss5 = _mm_unpackhi_epi16(src10, src14);
        ss6 = _mm_unpackhi_epi16(src04, src12);
        ss7 = _mm_unpackhi_epi16(src00, src08);

        t4 = _mm256_set_m128i(ss4, ss0);
        t5 = _mm256_set_m128i(ss5, ss1);
        t6 = _mm256_set_m128i(ss6, ss2);
        t7 = _mm256_set_m128i(ss7, ss3);

        eo0 = _mm256_add_epi32(_mm256_madd_epi16(t4, c16_p38_p44), _mm256_madd_epi16(t5, c16_p09_p25)); // EO0
        eo1 = _mm256_add_epi32(_mm256_madd_epi16(t4, c16_n09_p38), _mm256_madd_epi16(t5, c16_n25_n44)); // EO1
        eo2 = _mm256_add_epi32(_mm256_madd_epi16(t4, c16_n44_p25), _mm256_madd_epi16(t5, c16_p38_p09)); // EO2
        eo3 = _mm256_add_epi32(_mm256_madd_epi16(t4, c16_n25_p09), _mm256_madd_epi16(t5, c16_n44_p38)); // EO3

        eeo0 = _mm256_madd_epi16(t6, c16_p17_p42);
        eeo1 = _mm256_madd_epi16(t6, c16_n42_p17);

        eee0 = _mm256_madd_epi16(t7, c16_p32_p32);
        eee1 = _mm256_madd_epi16(t7, c16_n32_p32);

        {
            const __m256i ee0 = _mm256_add_epi32(eee0, eeo0);       // EE0 = EEE0 + EEO0
            const __m256i ee1 = _mm256_add_epi32(eee1, eeo1);       // EE1 = EEE1 + EEO1
            const __m256i ee3 = _mm256_sub_epi32(eee0, eeo0);       // EE2 = EEE0 - EEO0
            const __m256i ee2 = _mm256_sub_epi32(eee1, eeo1);       // EE3 = EEE1 - EEO1

            const __m256i e0 = _mm256_add_epi32(ee0, eo0);          // E0 = EE0 + EO0
            const __m256i e1 = _mm256_add_epi32(ee1, eo1);          // E1 = EE1 + EO1
            const __m256i e2 = _mm256_add_epi32(ee2, eo2);          // E2 = EE2 + EO2
            const __m256i e3 = _mm256_add_epi32(ee3, eo3);          // E3 = EE3 + EO3
            const __m256i e7 = _mm256_sub_epi32(ee0, eo0);          // E0 = EE0 - EO0
            const __m256i e6 = _mm256_sub_epi32(ee1, eo1);          // E1 = EE1 - EO1
            const __m256i e5 = _mm256_sub_epi32(ee2, eo2);          // E2 = EE2 - EO2
            const __m256i e4 = _mm256_sub_epi32(ee3, eo3);          // E3 = EE3 - EO3

            const __m256i t10 = _mm256_add_epi32(e0, c32_off);      // E0 + off
            const __m256i t11 = _mm256_add_epi32(e1, c32_off);      // E1 + off
            const __m256i t12 = _mm256_add_epi32(e2, c32_off);      // E2 + off
            const __m256i t13 = _mm256_add_epi32(e3, c32_off);      // E3 + off
            const __m256i t14 = _mm256_add_epi32(e4, c32_off);      // E4 + off
            const __m256i t15 = _mm256_add_epi32(e5, c32_off);      // E5 + off
            const __m256i t16 = _mm256_add_epi32(e6, c32_off);      // E6 + off
            const __m256i t17 = _mm256_add_epi32(e7, c32_off);      // E7 + off

            __m256i T20 = _mm256_add_epi32(t10, o0);                // E0 + O0 + off
            __m256i T21 = _mm256_add_epi32(t11, o1);                // E1 + O1 + off
            __m256i T22 = _mm256_add_epi32(t12, o2);                // E2 + O2 + off
            __m256i T23 = _mm256_add_epi32(t13, o3);                // E3 + O3 + off
            __m256i T24 = _mm256_add_epi32(t14, o4);                // E4
            __m256i T25 = _mm256_add_epi32(t15, o5);                // E5
            __m256i T26 = _mm256_add_epi32(t16, o6);                // E6
            __m256i T27 = _mm256_add_epi32(t17, o7);                // E7
            __m256i T2F = _mm256_sub_epi32(t10, o0);                // E0 - O0 + off
            __m256i T2E = _mm256_sub_epi32(t11, o1);                // E1 - O1 + off
            __m256i T2D = _mm256_sub_epi32(t12, o2);                // E2 - O2 + off
            __m256i T2C = _mm256_sub_epi32(t13, o3);                // E3 - O3 + off
            __m256i T2B = _mm256_sub_epi32(t14, o4);                // E4
            __m256i T2A = _mm256_sub_epi32(t15, o5);                // E5
            __m256i T29 = _mm256_sub_epi32(t16, o6);                // E6
            __m256i T28 = _mm256_sub_epi32(t17, o7);                // E7

            T20 = _mm256_srai_epi32(T20, shift);                    // [30 20 10 00]
            T21 = _mm256_srai_epi32(T21, shift);                    // [31 21 11 01]
            T22 = _mm256_srai_epi32(T22, shift);                    // [32 22 12 02]
            T23 = _mm256_srai_epi32(T23, shift);                    // [33 23 13 03]
            T24 = _mm256_srai_epi32(T24, shift);                    // [33 24 14 04]
            T25 = _mm256_srai_epi32(T25, shift);                    // [35 25 15 05]
            T26 = _mm256_srai_epi32(T26, shift);                    // [36 26 16 06]
            T27 = _mm256_srai_epi32(T27, shift);                    // [37 27 17 07]
            T28 = _mm256_srai_epi32(T28, shift);                    // [30 20 10 00] x8
            T29 = _mm256_srai_epi32(T29, shift);                    // [31 21 11 01] x9
            T2A = _mm256_srai_epi32(T2A, shift);                    // [32 22 12 02] xA
            T2B = _mm256_srai_epi32(T2B, shift);                    // [33 23 13 03] xB
            T2C = _mm256_srai_epi32(T2C, shift);                    // [33 24 14 04] xC
            T2D = _mm256_srai_epi32(T2D, shift);                    // [35 25 15 05] xD
            T2E = _mm256_srai_epi32(T2E, shift);                    // [36 26 16 06] xE
            T2F = _mm256_srai_epi32(T2F, shift);                    // [37 27 17 07] xF

            // transpose 16x8 --> 8x16
            TRANSPOSE_16x8_32BIT_16BIT(T20, T21, T22, T23, T24, T25, T26, T27, 
                T28, T29, T2A, T2B, T2C, T2D, T2E, T2F, res00, res01, res02, res03, res04, res05, res06, res07);
        }
        if (bit_depth != MAX_TX_DYNAMIC_RANGE) {
            __m256i max_val = _mm256_set1_epi16((1 << bit_depth) - 1);
            __m256i min_val = _mm256_set1_epi16(-(1 << bit_depth));

            res00 = _mm256_min_epi16(res00, max_val);
            res01 = _mm256_min_epi16(res01, max_val);
            res02 = _mm256_min_epi16(res02, max_val);
            res03 = _mm256_min_epi16(res03, max_val);
            res04 = _mm256_min_epi16(res04, max_val);
            res05 = _mm256_min_epi16(res05, max_val);
            res06 = _mm256_min_epi16(res06, max_val);
            res07 = _mm256_min_epi16(res07, max_val);
            res00 = _mm256_max_epi16(res00, min_val);
            res01 = _mm256_max_epi16(res01, min_val);
            res02 = _mm256_max_epi16(res02, min_val);
            res03 = _mm256_max_epi16(res03, min_val);
            res04 = _mm256_max_epi16(res04, min_val);
            res05 = _mm256_max_epi16(res05, min_val);
            res06 = _mm256_max_epi16(res06, min_val);
            res07 = _mm256_max_epi16(res07, min_val);

        }

        _mm256_store_si256((__m256i*)&dst[16 * 0], res00);
        _mm256_store_si256((__m256i*)&dst[16 * 1], res01);
        _mm256_store_si256((__m256i*)&dst[16 * 2], res02);
        _mm256_store_si256((__m256i*)&dst[16 * 3], res03);
        _mm256_store_si256((__m256i*)&dst[16 * 4], res04);
        _mm256_store_si256((__m256i*)&dst[16 * 5], res05);
        _mm256_store_si256((__m256i*)&dst[16 * 6], res06);
        _mm256_store_si256((__m256i*)&dst[16 * 7], res07);

        dst += 16*8; // 8 rows
    }
}
void dct2_butterfly_h32_avx2(s16* src, int i_src, s16* dst, int line, int shift, int bit_depth)
{
    const __m256i c16_p45_p45 = _mm256_set1_epi32(0x002D002D);
    const __m256i c16_p43_p44 = _mm256_set1_epi32(0x002B002C);
    const __m256i c16_p39_p41 = _mm256_set1_epi32(0x00270029);
    const __m256i c16_p34_p36 = _mm256_set1_epi32(0x00220024);
    const __m256i c16_p27_p30 = _mm256_set1_epi32(0x001B001E);
    const __m256i c16_p19_p23 = _mm256_set1_epi32(0x00130017);
    const __m256i c16_p11_p15 = _mm256_set1_epi32(0x000B000F);
    const __m256i c16_p02_p07 = _mm256_set1_epi32(0x00020007);
    const __m256i c16_p41_p45 = _mm256_set1_epi32(0x0029002D);
    const __m256i c16_p23_p34 = _mm256_set1_epi32(0x00170022);
    const __m256i c16_n02_p11 = _mm256_set1_epi32(0xFFFE000B);
    const __m256i c16_n27_n15 = _mm256_set1_epi32(0xFFE5FFF1);
    const __m256i c16_n43_n36 = _mm256_set1_epi32(0xFFD5FFDC);
    const __m256i c16_n44_n45 = _mm256_set1_epi32(0xFFD4FFD3);
    const __m256i c16_n30_n39 = _mm256_set1_epi32(0xFFE2FFD9);
    const __m256i c16_n07_n19 = _mm256_set1_epi32(0xFFF9FFED);
    const __m256i c16_p34_p44 = _mm256_set1_epi32(0x0022002C);
    const __m256i c16_n07_p15 = _mm256_set1_epi32(0xFFF9000F);
    const __m256i c16_n41_n27 = _mm256_set1_epi32(0xFFD7FFE5);
    const __m256i c16_n39_n45 = _mm256_set1_epi32(0xFFD9FFD3);
    const __m256i c16_n02_n23 = _mm256_set1_epi32(0xFFFEFFE9);
    const __m256i c16_p36_p19 = _mm256_set1_epi32(0x00240013);
    const __m256i c16_p43_p45 = _mm256_set1_epi32(0x002B002D);
    const __m256i c16_p11_p30 = _mm256_set1_epi32(0x000B001E);
    const __m256i c16_p23_p43 = _mm256_set1_epi32(0x0017002B);
    const __m256i c16_n34_n07 = _mm256_set1_epi32(0xFFDEFFF9);
    const __m256i c16_n36_n45 = _mm256_set1_epi32(0xFFDCFFD3);
    const __m256i c16_p19_n11 = _mm256_set1_epi32(0x0013FFF5);
    const __m256i c16_p44_p41 = _mm256_set1_epi32(0x002C0029);
    const __m256i c16_n02_p27 = _mm256_set1_epi32(0xFFFE001B);
    const __m256i c16_n45_n30 = _mm256_set1_epi32(0xFFD3FFE2);
    const __m256i c16_n15_n39 = _mm256_set1_epi32(0xFFF1FFD9);
    const __m256i c16_p11_p41 = _mm256_set1_epi32(0x000B0029);
    const __m256i c16_n45_n27 = _mm256_set1_epi32(0xFFD3FFE5);
    const __m256i c16_p07_n30 = _mm256_set1_epi32(0x0007FFE2);
    const __m256i c16_p43_p39 = _mm256_set1_epi32(0x002B0027);
    const __m256i c16_n23_p15 = _mm256_set1_epi32(0xFFE9000F);
    const __m256i c16_n34_n45 = _mm256_set1_epi32(0xFFDEFFD3);
    const __m256i c16_p36_p02 = _mm256_set1_epi32(0x00240002);
    const __m256i c16_p19_p44 = _mm256_set1_epi32(0x0013002C);
    const __m256i c16_n02_p39 = _mm256_set1_epi32(0xFFFE0027);
    const __m256i c16_n36_n41 = _mm256_set1_epi32(0xFFDCFFD7);
    const __m256i c16_p43_p07 = _mm256_set1_epi32(0x002B0007);
    const __m256i c16_n11_p34 = _mm256_set1_epi32(0xFFF50022);
    const __m256i c16_n30_n44 = _mm256_set1_epi32(0xFFE2FFD4);
    const __m256i c16_p45_p15 = _mm256_set1_epi32(0x002D000F);
    const __m256i c16_n19_p27 = _mm256_set1_epi32(0xFFED001B);
    const __m256i c16_n23_n45 = _mm256_set1_epi32(0xFFE9FFD3);
    const __m256i c16_n15_p36 = _mm256_set1_epi32(0xFFF10024);
    const __m256i c16_n11_n45 = _mm256_set1_epi32(0xFFF5FFD3);
    const __m256i c16_p34_p39 = _mm256_set1_epi32(0x00220027);
    const __m256i c16_n45_n19 = _mm256_set1_epi32(0xFFD3FFED);
    const __m256i c16_p41_n07 = _mm256_set1_epi32(0x0029FFF9);
    const __m256i c16_n23_p30 = _mm256_set1_epi32(0xFFE9001E);
    const __m256i c16_n02_n44 = _mm256_set1_epi32(0xFFFEFFD4);
    const __m256i c16_p27_p43 = _mm256_set1_epi32(0x001B002B);
    const __m256i c16_n27_p34 = _mm256_set1_epi32(0xFFE50022);
    const __m256i c16_p19_n39 = _mm256_set1_epi32(0x0013FFD9);
    const __m256i c16_n11_p43 = _mm256_set1_epi32(0xFFF5002B);
    const __m256i c16_p02_n45 = _mm256_set1_epi32(0x0002FFD3);
    const __m256i c16_p07_p45 = _mm256_set1_epi32(0x0007002D);
    const __m256i c16_n15_n44 = _mm256_set1_epi32(0xFFF1FFD4);
    const __m256i c16_p23_p41 = _mm256_set1_epi32(0x00170029);
    const __m256i c16_n30_n36 = _mm256_set1_epi32(0xFFE2FFDC);
    const __m256i c16_n36_p30 = _mm256_set1_epi32(0xFFDC001E);
    const __m256i c16_p41_n23 = _mm256_set1_epi32(0x0029FFE9);
    const __m256i c16_n44_p15 = _mm256_set1_epi32(0xFFD4000F);
    const __m256i c16_p45_n07 = _mm256_set1_epi32(0x002DFFF9);
    const __m256i c16_n45_n02 = _mm256_set1_epi32(0xFFD3FFFE);
    const __m256i c16_p43_p11 = _mm256_set1_epi32(0x002B000B);
    const __m256i c16_n39_n19 = _mm256_set1_epi32(0xFFD9FFED);
    const __m256i c16_p34_p27 = _mm256_set1_epi32(0x0022001B);
    const __m256i c16_n43_p27 = _mm256_set1_epi32(0xFFD5001B);
    const __m256i c16_p44_n02 = _mm256_set1_epi32(0x002CFFFE);
    const __m256i c16_n30_n23 = _mm256_set1_epi32(0xFFE2FFE9);
    const __m256i c16_p07_p41 = _mm256_set1_epi32(0x00070029);
    const __m256i c16_p19_n45 = _mm256_set1_epi32(0x0013FFD3);
    const __m256i c16_n39_p34 = _mm256_set1_epi32(0xFFD90022);
    const __m256i c16_p45_n11 = _mm256_set1_epi32(0x002DFFF5);
    const __m256i c16_n36_n15 = _mm256_set1_epi32(0xFFDCFFF1);
    const __m256i c16_n45_p23 = _mm256_set1_epi32(0xFFD30017);
    const __m256i c16_p27_p19 = _mm256_set1_epi32(0x001B0013);
    const __m256i c16_p15_n45 = _mm256_set1_epi32(0x000FFFD3);
    const __m256i c16_n44_p30 = _mm256_set1_epi32(0xFFD4001E);
    const __m256i c16_p34_p11 = _mm256_set1_epi32(0x0022000B);
    const __m256i c16_p07_n43 = _mm256_set1_epi32(0x0007FFD5);
    const __m256i c16_n41_p36 = _mm256_set1_epi32(0xFFD70024);
    const __m256i c16_p39_p02 = _mm256_set1_epi32(0x00270002);
    const __m256i c16_n44_p19 = _mm256_set1_epi32(0xFFD40013);
    const __m256i c16_n02_p36 = _mm256_set1_epi32(0xFFFE0024);
    const __m256i c16_p45_n34 = _mm256_set1_epi32(0x002DFFDE);
    const __m256i c16_n15_n23 = _mm256_set1_epi32(0xFFF1FFE9);
    const __m256i c16_n39_p43 = _mm256_set1_epi32(0xFFD9002B);
    const __m256i c16_p30_p07 = _mm256_set1_epi32(0x001E0007);
    const __m256i c16_p27_n45 = _mm256_set1_epi32(0x001BFFD3);
    const __m256i c16_n41_p11 = _mm256_set1_epi32(0xFFD7000B);
    const __m256i c16_n39_p15 = _mm256_set1_epi32(0xFFD9000F);
    const __m256i c16_n30_p45 = _mm256_set1_epi32(0xFFE2002D);
    const __m256i c16_p27_p02 = _mm256_set1_epi32(0x001B0002);
    const __m256i c16_p41_n44 = _mm256_set1_epi32(0x0029FFD4);
    const __m256i c16_n11_n19 = _mm256_set1_epi32(0xFFF5FFED);
    const __m256i c16_n45_p36 = _mm256_set1_epi32(0xFFD30024);
    const __m256i c16_n07_p34 = _mm256_set1_epi32(0xFFF90022);
    const __m256i c16_p43_n23 = _mm256_set1_epi32(0x002BFFE9);
    const __m256i c16_n30_p11 = _mm256_set1_epi32(0xFFE2000B);
    const __m256i c16_n45_p43 = _mm256_set1_epi32(0xFFD3002B);
    const __m256i c16_n19_p36 = _mm256_set1_epi32(0xFFED0024);
    const __m256i c16_p23_n02 = _mm256_set1_epi32(0x0017FFFE);
    const __m256i c16_p45_n39 = _mm256_set1_epi32(0x002DFFD9);
    const __m256i c16_p27_n41 = _mm256_set1_epi32(0x001BFFD7);
    const __m256i c16_n15_n07 = _mm256_set1_epi32(0xFFF1FFF9);
    const __m256i c16_n44_p34 = _mm256_set1_epi32(0xFFD40022);
    const __m256i c16_n19_p07 = _mm256_set1_epi32(0xFFED0007);
    const __m256i c16_n39_p30 = _mm256_set1_epi32(0xFFD9001E);
    const __m256i c16_n45_p44 = _mm256_set1_epi32(0xFFD3002C);
    const __m256i c16_n36_p43 = _mm256_set1_epi32(0xFFDC002B);
    const __m256i c16_n15_p27 = _mm256_set1_epi32(0xFFF1001B);
    const __m256i c16_p11_p02 = _mm256_set1_epi32(0x000B0002);
    const __m256i c16_p34_n23 = _mm256_set1_epi32(0x0022FFE9);
    const __m256i c16_p45_n41 = _mm256_set1_epi32(0x002DFFD7);
    const __m256i c16_n07_p02 = _mm256_set1_epi32(0xFFF90002);
    const __m256i c16_n15_p11 = _mm256_set1_epi32(0xFFF1000B);
    const __m256i c16_n23_p19 = _mm256_set1_epi32(0xFFE90013);
    const __m256i c16_n30_p27 = _mm256_set1_epi32(0xFFE2001B);
    const __m256i c16_n36_p34 = _mm256_set1_epi32(0xFFDC0022);
    const __m256i c16_n41_p39 = _mm256_set1_epi32(0xFFD70027);
    const __m256i c16_n44_p43 = _mm256_set1_epi32(0xFFD4002B);
    const __m256i c16_n45_p45 = _mm256_set1_epi32(0xFFD3002D);

    const __m256i c16_p35_p40 = _mm256_set1_epi32(0x00230028);
    const __m256i c16_p21_p29 = _mm256_set1_epi32(0x0015001D);
    const __m256i c16_p04_p13 = _mm256_set1_epi32(0x0004000D);
    const __m256i c16_p29_p43 = _mm256_set1_epi32(0x001D002B);
    const __m256i c16_n21_p04 = _mm256_set1_epi32(0xFFEB0004);
    const __m256i c16_n45_n40 = _mm256_set1_epi32(0xFFD3FFD8);
    const __m256i c16_n13_n35 = _mm256_set1_epi32(0xFFF3FFDD);
    const __m256i c16_p04_p40 = _mm256_set1_epi32(0x00040028);
    const __m256i c16_n43_n35 = _mm256_set1_epi32(0xFFD5FFDD);
    const __m256i c16_p29_n13 = _mm256_set1_epi32(0x001DFFF3);
    const __m256i c16_p21_p45 = _mm256_set1_epi32(0x0015002D);
    const __m256i c16_n21_p35 = _mm256_set1_epi32(0xFFEB0023);
    const __m256i c16_p04_n43 = _mm256_set1_epi32(0x0004FFD5);
    const __m256i c16_p13_p45 = _mm256_set1_epi32(0x000D002D);
    const __m256i c16_n29_n40 = _mm256_set1_epi32(0xFFE3FFD8);
    const __m256i c16_n40_p29 = _mm256_set1_epi32(0xFFD8001D);
    const __m256i c16_p45_n13 = _mm256_set1_epi32(0x002DFFF3);
    const __m256i c16_n43_n04 = _mm256_set1_epi32(0xFFD5FFFC);
    const __m256i c16_p35_p21 = _mm256_set1_epi32(0x00230015);
    const __m256i c16_n45_p21 = _mm256_set1_epi32(0xFFD30015);
    const __m256i c16_p13_p29 = _mm256_set1_epi32(0x000D001D);
    const __m256i c16_p35_n43 = _mm256_set1_epi32(0x0023FFD5);
    const __m256i c16_n40_p04 = _mm256_set1_epi32(0xFFD80004);
    const __m256i c16_n35_p13 = _mm256_set1_epi32(0xFFDD000D);
    const __m256i c16_n40_p45 = _mm256_set1_epi32(0xFFD8002D);
    const __m256i c16_p04_p21 = _mm256_set1_epi32(0x00040015);
    const __m256i c16_p43_n29 = _mm256_set1_epi32(0x002BFFE3);
    const __m256i c16_n13_p04 = _mm256_set1_epi32(0xFFF30004);
    const __m256i c16_n29_p21 = _mm256_set1_epi32(0xFFE30015);
    const __m256i c16_n40_p35 = _mm256_set1_epi32(0xFFD80023);

    const __m256i c16_p38_p44 = _mm256_set1_epi32(0x0026002C);
    const __m256i c16_p09_p25 = _mm256_set1_epi32(0x00090019);
    const __m256i c16_n09_p38 = _mm256_set1_epi32(0xFFF70026);
    const __m256i c16_n25_n44 = _mm256_set1_epi32(0xFFE7FFD4);

    const __m256i c16_n44_p25 = _mm256_set1_epi32(0xFFD40019);
    const __m256i c16_p38_p09 = _mm256_set1_epi32(0x00260009);
    const __m256i c16_n25_p09 = _mm256_set1_epi32(0xFFE70009);
    const __m256i c16_n44_p38 = _mm256_set1_epi32(0xFFD40026);

    const __m256i c16_p17_p42 = _mm256_set1_epi32(0x0011002A);
    const __m256i c16_n42_p17 = _mm256_set1_epi32(0xFFD60011);

    const __m256i c16_p32_p32 = _mm256_set1_epi32(0x00200020);
    const __m256i c16_n32_p32 = _mm256_set1_epi32(0xFFE00020);

    __m256i c32_off = _mm256_set1_epi32(1 << (shift - 1));

    __m128i src00, src01, src02, src03, src04, src05, src06, src07, src08, src09, src10, src11, src12, src13, src14, src15;
    __m128i src16, src17, src18, src19, src20, src21, src22, src23, src24, src25, src26, src27, src28, src29, src30, src31;
    __m128i ss00, ss01, ss02, ss03, ss04, ss05, ss06, ss07, ss08, ss09, ss10, ss11, ss12, ss13, ss14, ss15;
    __m256i res00, res01, res02, res03, res04, res05, res06, res07, res08, res09, res10, res11, res12, res13, res14, res15;
    __m256i t0, t1, t2, t3, t4, t5, t6, t7;
    __m256i O00, O01, O02, O03, O04, O05, O06, O07, O08, O09, O10, O11, O12, O13, O14, O15;
    __m256i EO0, EO1, EO2, EO3, EO4, EO5, EO6, EO7;
    int i;

    for (i = 0; i < line; i += 8)
    {
        src01 = _mm_load_si128((const __m128i*)&src[1 * i_src + i]);
        src03 = _mm_load_si128((const __m128i*)&src[3 * i_src + i]);
        src05 = _mm_load_si128((const __m128i*)&src[5 * i_src + i]);
        src07 = _mm_load_si128((const __m128i*)&src[7 * i_src + i]);
        src09 = _mm_load_si128((const __m128i*)&src[9 * i_src + i]);
        src11 = _mm_load_si128((const __m128i*)&src[11 * i_src + i]);
        src13 = _mm_load_si128((const __m128i*)&src[13 * i_src + i]);
        src15 = _mm_load_si128((const __m128i*)&src[15 * i_src + i]);
        src17 = _mm_load_si128((const __m128i*)&src[17 * i_src + i]);
        src19 = _mm_load_si128((const __m128i*)&src[19 * i_src + i]);
        src21 = _mm_load_si128((const __m128i*)&src[21 * i_src + i]);
        src23 = _mm_load_si128((const __m128i*)&src[23 * i_src + i]);
        src25 = _mm_load_si128((const __m128i*)&src[25 * i_src + i]);
        src27 = _mm_load_si128((const __m128i*)&src[27 * i_src + i]);
        src29 = _mm_load_si128((const __m128i*)&src[29 * i_src + i]);
        src31 = _mm_load_si128((const __m128i*)&src[31 * i_src + i]);

        ss00 = _mm_unpacklo_epi16(src01, src03);
        ss01 = _mm_unpacklo_epi16(src05, src07);
        ss02 = _mm_unpacklo_epi16(src09, src11);
        ss03 = _mm_unpacklo_epi16(src13, src15);
        ss04 = _mm_unpacklo_epi16(src17, src19);
        ss05 = _mm_unpacklo_epi16(src21, src23);
        ss06 = _mm_unpacklo_epi16(src25, src27);
        ss07 = _mm_unpacklo_epi16(src29, src31);

        ss08 = _mm_unpackhi_epi16(src01, src03);
        ss09 = _mm_unpackhi_epi16(src05, src07);
        ss10 = _mm_unpackhi_epi16(src09, src11);
        ss11 = _mm_unpackhi_epi16(src13, src15);
        ss12 = _mm_unpackhi_epi16(src17, src19);
        ss13 = _mm_unpackhi_epi16(src21, src23);
        ss14 = _mm_unpackhi_epi16(src25, src27);
        ss15 = _mm_unpackhi_epi16(src29, src31);

        {
            const __m256i T_00_00 = _mm256_set_m128i(ss08, ss00);       // [33 13 32 12 31 11 30 10]
            const __m256i T_00_01 = _mm256_set_m128i(ss09, ss01);       // [ ]
            const __m256i T_00_02 = _mm256_set_m128i(ss10, ss02);       // [ ]
            const __m256i T_00_03 = _mm256_set_m128i(ss11, ss03);       // [ ]
            const __m256i T_00_04 = _mm256_set_m128i(ss12, ss04);       // [ ]
            const __m256i T_00_05 = _mm256_set_m128i(ss13, ss05);       // [ ]
            const __m256i T_00_06 = _mm256_set_m128i(ss14, ss06);       // [ ]
            const __m256i T_00_07 = _mm256_set_m128i(ss15, ss07);       //

#define COMPUTE_ROW(r0103, r0507, r0911, r1315, r1719, r2123, r2527, r2931, c0103, c0507, c0911, c1315, c1719, c2123, c2527, c2931, row) \
            t0 = _mm256_madd_epi16(r0103, c0103); \
            t1 = _mm256_madd_epi16(r0507, c0507); \
            t2 = _mm256_madd_epi16(r0911, c0911); \
            t3 = _mm256_madd_epi16(r1315, c1315); \
            t4 = _mm256_madd_epi16(r1719, c1719); \
            t5 = _mm256_madd_epi16(r2123, c2123); \
            t6 = _mm256_madd_epi16(r2527, c2527); \
            t7 = _mm256_madd_epi16(r2931, c2931); \
	        t0 = _mm256_add_epi32(t0, t1); \
	        t2 = _mm256_add_epi32(t2, t3); \
	        t4 = _mm256_add_epi32(t4, t5); \
	        t6 = _mm256_add_epi32(t6, t7); \
	        row = _mm256_add_epi32(_mm256_add_epi32(t0, t2), _mm256_add_epi32(t4, t6));

            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_p45_p45, c16_p43_p44, c16_p39_p41, c16_p34_p36, c16_p27_p30, c16_p19_p23, c16_p11_p15, c16_p02_p07, O00)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_p41_p45, c16_p23_p34, c16_n02_p11, c16_n27_n15, c16_n43_n36, c16_n44_n45, c16_n30_n39, c16_n07_n19, O01)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_p34_p44, c16_n07_p15, c16_n41_n27, c16_n39_n45, c16_n02_n23, c16_p36_p19, c16_p43_p45, c16_p11_p30, O02)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_p23_p43, c16_n34_n07, c16_n36_n45, c16_p19_n11, c16_p44_p41, c16_n02_p27, c16_n45_n30, c16_n15_n39, O03)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_p11_p41, c16_n45_n27, c16_p07_n30, c16_p43_p39, c16_n23_p15, c16_n34_n45, c16_p36_p02, c16_p19_p44, O04)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n02_p39, c16_n36_n41, c16_p43_p07, c16_n11_p34, c16_n30_n44, c16_p45_p15, c16_n19_p27, c16_n23_n45, O05)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n15_p36, c16_n11_n45, c16_p34_p39, c16_n45_n19, c16_p41_n07, c16_n23_p30, c16_n02_n44, c16_p27_p43, O06)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n27_p34, c16_p19_n39, c16_n11_p43, c16_p02_n45, c16_p07_p45, c16_n15_n44, c16_p23_p41, c16_n30_n36, O07)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n36_p30, c16_p41_n23, c16_n44_p15, c16_p45_n07, c16_n45_n02, c16_p43_p11, c16_n39_n19, c16_p34_p27, O08)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n43_p27, c16_p44_n02, c16_n30_n23, c16_p07_p41, c16_p19_n45, c16_n39_p34, c16_p45_n11, c16_n36_n15, O09)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n45_p23, c16_p27_p19, c16_p15_n45, c16_n44_p30, c16_p34_p11, c16_p07_n43, c16_n41_p36, c16_p39_p02, O10)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n44_p19, c16_n02_p36, c16_p45_n34, c16_n15_n23, c16_n39_p43, c16_p30_p07, c16_p27_n45, c16_n41_p11, O11)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n39_p15, c16_n30_p45, c16_p27_p02, c16_p41_n44, c16_n11_n19, c16_n45_p36, c16_n07_p34, c16_p43_n23, O12)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n30_p11, c16_n45_p43, c16_n19_p36, c16_p23_n02, c16_p45_n39, c16_p27_n41, c16_n15_n07, c16_n44_p34, O13)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n19_p07, c16_n39_p30, c16_n45_p44, c16_n36_p43, c16_n15_p27, c16_p11_p02, c16_p34_n23, c16_p45_n41, O14)
            COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                c16_n07_p02, c16_n15_p11, c16_n23_p19, c16_n30_p27, c16_n36_p34, c16_n41_p39, c16_n44_p43, c16_n45_p45, O15)

#undef COMPUTE_ROW
        }

        src00 = _mm_load_si128((const __m128i*)&src[0 * i_src + i]);
        src02 = _mm_load_si128((const __m128i*)&src[2 * i_src + i]);
        src04 = _mm_load_si128((const __m128i*)&src[4 * i_src + i]);
        src06 = _mm_load_si128((const __m128i*)&src[6 * i_src + i]);
        src08 = _mm_load_si128((const __m128i*)&src[8 * i_src + i]);
        src10 = _mm_load_si128((const __m128i*)&src[10 * i_src + i]);
        src12 = _mm_load_si128((const __m128i*)&src[12 * i_src + i]);
        src14 = _mm_load_si128((const __m128i*)&src[14 * i_src + i]);
        src16 = _mm_load_si128((const __m128i*)&src[16 * i_src + i]);
        src18 = _mm_load_si128((const __m128i*)&src[18 * i_src + i]);
        src20 = _mm_load_si128((const __m128i*)&src[20 * i_src + i]);
        src22 = _mm_load_si128((const __m128i*)&src[22 * i_src + i]);
        src24 = _mm_load_si128((const __m128i*)&src[24 * i_src + i]);
        src26 = _mm_load_si128((const __m128i*)&src[26 * i_src + i]);
        src28 = _mm_load_si128((const __m128i*)&src[28 * i_src + i]);
        src30 = _mm_load_si128((const __m128i*)&src[30 * i_src + i]);

        ss00 = _mm_unpacklo_epi16(src02, src06);
        ss01 = _mm_unpacklo_epi16(src10, src14);
        ss02 = _mm_unpacklo_epi16(src18, src22);
        ss03 = _mm_unpacklo_epi16(src26, src30);
        ss04 = _mm_unpacklo_epi16(src04, src12);
        ss05 = _mm_unpacklo_epi16(src20, src28);
        ss06 = _mm_unpacklo_epi16(src08, src24);
        ss07 = _mm_unpacklo_epi16(src00, src16);

        ss08 = _mm_unpackhi_epi16(src02, src06);
        ss09 = _mm_unpackhi_epi16(src10, src14);
        ss10 = _mm_unpackhi_epi16(src18, src22);
        ss11 = _mm_unpackhi_epi16(src26, src30);
        ss12 = _mm_unpackhi_epi16(src04, src12);
        ss13 = _mm_unpackhi_epi16(src20, src28);
        ss14 = _mm_unpackhi_epi16(src08, src24);
        ss15 = _mm_unpackhi_epi16(src00, src16);

        {
            const __m256i T_00_08 = _mm256_set_m128i(ss08, ss00);
            const __m256i T_00_09 = _mm256_set_m128i(ss09, ss01);
            const __m256i T_00_10 = _mm256_set_m128i(ss10, ss02);
            const __m256i T_00_11 = _mm256_set_m128i(ss11, ss03);
            const __m256i T_00_12 = _mm256_set_m128i(ss12, ss04);
            const __m256i T_00_13 = _mm256_set_m128i(ss13, ss05);
            const __m256i T_00_14 = _mm256_set_m128i(ss14, ss06);
            const __m256i T_00_15 = _mm256_set_m128i(ss15, ss07);

#define COMPUTE_ROW(row0206, row1014, row1822, row2630, c0206, c1014, c1822, c2630, row) \
            t0 = _mm256_madd_epi16(row0206, c0206); \
            t1 = _mm256_madd_epi16(row1014, c1014); \
            t2 = _mm256_madd_epi16(row1822, c1822); \
            t3 = _mm256_madd_epi16(row2630, c2630); \
	        t0 = _mm256_add_epi32(t0, t1); \
	        t2 = _mm256_add_epi32(t2, t3); \
	        row = _mm256_add_epi32(t0, t2);

            COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_p43_p45, c16_p35_p40, c16_p21_p29, c16_p04_p13, EO0)
            COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_p29_p43, c16_n21_p04, c16_n45_n40, c16_n13_n35, EO1)
            COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_p04_p40, c16_n43_n35, c16_p29_n13, c16_p21_p45, EO2)
            COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n21_p35, c16_p04_n43, c16_p13_p45, c16_n29_n40, EO3)
            COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n40_p29, c16_p45_n13, c16_n43_n04, c16_p35_p21, EO4)
            COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n45_p21, c16_p13_p29, c16_p35_n43, c16_n40_p04, EO5)
            COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n35_p13, c16_n40_p45, c16_p04_p21, c16_p43_n29, EO6)
            COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n13_p04, c16_n29_p21, c16_n40_p35, c16_n45_p43, EO7)

#undef COMPUTE_ROW

            {
                const __m256i EEO0 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_p38_p44), _mm256_madd_epi16(T_00_13, c16_p09_p25));
                const __m256i EEO1 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_n09_p38), _mm256_madd_epi16(T_00_13, c16_n25_n44));
                const __m256i EEO2 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_n44_p25), _mm256_madd_epi16(T_00_13, c16_p38_p09));
                const __m256i EEO3 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_n25_p09), _mm256_madd_epi16(T_00_13, c16_n44_p38));

                const __m256i EEEO0 = _mm256_madd_epi16(T_00_14, c16_p17_p42);
                const __m256i EEEO1 = _mm256_madd_epi16(T_00_14, c16_n42_p17);

                const __m256i EEEE0 = _mm256_madd_epi16(T_00_15, c16_p32_p32);
                const __m256i EEEE1 = _mm256_madd_epi16(T_00_15, c16_n32_p32);

                const __m256i EEE0 = _mm256_add_epi32(EEEE0, EEEO0);          // EEE0 = EEEE0 + EEEO0
                const __m256i EEE1 = _mm256_add_epi32(EEEE1, EEEO1);          // EEE1 = EEEE1 + EEEO1
                const __m256i EEE3 = _mm256_sub_epi32(EEEE0, EEEO0);          // EEE2 = EEEE0 - EEEO0
                const __m256i EEE2 = _mm256_sub_epi32(EEEE1, EEEO1);          // EEE3 = EEEE1 - EEEO1

                const __m256i EE0 = _mm256_add_epi32(EEE0, EEO0);          // EE0 = EEE0 + EEO0
                const __m256i EE1 = _mm256_add_epi32(EEE1, EEO1);          // EE1 = EEE1 + EEO1
                const __m256i EE2 = _mm256_add_epi32(EEE2, EEO2);          // EE2 = EEE0 + EEO0
                const __m256i EE3 = _mm256_add_epi32(EEE3, EEO3);          // EE3 = EEE1 + EEO1
                const __m256i EE7 = _mm256_sub_epi32(EEE0, EEO0);          // EE7 = EEE0 - EEO0
                const __m256i EE6 = _mm256_sub_epi32(EEE1, EEO1);          // EE6 = EEE1 - EEO1
                const __m256i EE5 = _mm256_sub_epi32(EEE2, EEO2);          // EE5 = EEE0 - EEO0
                const __m256i EE4 = _mm256_sub_epi32(EEE3, EEO3);          // EE4 = EEE1 - EEO1

                const __m256i E0 = _mm256_add_epi32(EE0, EO0);          // E0 = EE0 + EO0
                const __m256i E1 = _mm256_add_epi32(EE1, EO1);          // E1 = EE1 + EO1
                const __m256i E2 = _mm256_add_epi32(EE2, EO2);          // E2 = EE2 + EO2
                const __m256i E3 = _mm256_add_epi32(EE3, EO3);          // E3 = EE3 + EO3
                const __m256i E4 = _mm256_add_epi32(EE4, EO4);          // E4 =
                const __m256i E5 = _mm256_add_epi32(EE5, EO5);          // E5 =
                const __m256i E6 = _mm256_add_epi32(EE6, EO6);          // E6 =
                const __m256i E7 = _mm256_add_epi32(EE7, EO7);          // E7 =
                const __m256i EF = _mm256_sub_epi32(EE0, EO0);          // EF = EE0 - EO0
                const __m256i EE = _mm256_sub_epi32(EE1, EO1);          // EE = EE1 - EO1
                const __m256i ED = _mm256_sub_epi32(EE2, EO2);          // ED = EE2 - EO2
                const __m256i EC = _mm256_sub_epi32(EE3, EO3);          // EC = EE3 - EO3
                const __m256i EB = _mm256_sub_epi32(EE4, EO4);          // EB =
                const __m256i EA = _mm256_sub_epi32(EE5, EO5);          // EA =
                const __m256i E9 = _mm256_sub_epi32(EE6, EO6);          // E9 =
                const __m256i E8 = _mm256_sub_epi32(EE7, EO7);          // E8 =

                const __m256i T10 = _mm256_add_epi32(E0, c32_off);         // E0 + off
                const __m256i T11 = _mm256_add_epi32(E1, c32_off);         // E1 + off
                const __m256i T12 = _mm256_add_epi32(E2, c32_off);         // E2 + off
                const __m256i T13 = _mm256_add_epi32(E3, c32_off);         // E3 + off
                const __m256i T14 = _mm256_add_epi32(E4, c32_off);         // E4 + off
                const __m256i T15 = _mm256_add_epi32(E5, c32_off);         // E5 + off
                const __m256i T16 = _mm256_add_epi32(E6, c32_off);         // E6 + off
                const __m256i T17 = _mm256_add_epi32(E7, c32_off);         // E7 + off
                const __m256i T18 = _mm256_add_epi32(E8, c32_off);         // E8 + off
                const __m256i T19 = _mm256_add_epi32(E9, c32_off);         // E9 + off
                const __m256i T1A = _mm256_add_epi32(EA, c32_off);         // E10 + off
                const __m256i T1B = _mm256_add_epi32(EB, c32_off);         // E11 + off
                const __m256i T1C = _mm256_add_epi32(EC, c32_off);         // E12 + off
                const __m256i T1D = _mm256_add_epi32(ED, c32_off);         // E13 + off
                const __m256i T1E = _mm256_add_epi32(EE, c32_off);         // E14 + off
                const __m256i T1F = _mm256_add_epi32(EF, c32_off);         // E15 + off

                __m256i T2_00 = _mm256_add_epi32(T10, O00);          // E0 + O0 + off
                __m256i T2_01 = _mm256_add_epi32(T11, O01);          // E1 + O1 + off
                __m256i T2_02 = _mm256_add_epi32(T12, O02);          // E2 + O2 + off
                __m256i T2_03 = _mm256_add_epi32(T13, O03);          // E3 + O3 + off
                __m256i T2_04 = _mm256_add_epi32(T14, O04);          // E4
                __m256i T2_05 = _mm256_add_epi32(T15, O05);          // E5
                __m256i T2_06 = _mm256_add_epi32(T16, O06);          // E6
                __m256i T2_07 = _mm256_add_epi32(T17, O07);          // E7
                __m256i T2_08 = _mm256_add_epi32(T18, O08);          // E8
                __m256i T2_09 = _mm256_add_epi32(T19, O09);          // E9
                __m256i T2_10 = _mm256_add_epi32(T1A, O10);          // E10
                __m256i T2_11 = _mm256_add_epi32(T1B, O11);          // E11
                __m256i T2_12 = _mm256_add_epi32(T1C, O12);          // E12
                __m256i T2_13 = _mm256_add_epi32(T1D, O13);          // E13
                __m256i T2_14 = _mm256_add_epi32(T1E, O14);          // E14
                __m256i T2_15 = _mm256_add_epi32(T1F, O15);          // E15
                __m256i T2_31 = _mm256_sub_epi32(T10, O00);          // E0 - O0 + off
                __m256i T2_30 = _mm256_sub_epi32(T11, O01);          // E1 - O1 + off
                __m256i T2_29 = _mm256_sub_epi32(T12, O02);          // E2 - O2 + off
                __m256i T2_28 = _mm256_sub_epi32(T13, O03);          // E3 - O3 + off
                __m256i T2_27 = _mm256_sub_epi32(T14, O04);          // E4
                __m256i T2_26 = _mm256_sub_epi32(T15, O05);          // E5
                __m256i T2_25 = _mm256_sub_epi32(T16, O06);          // E6
                __m256i T2_24 = _mm256_sub_epi32(T17, O07);          // E7
                __m256i T2_23 = _mm256_sub_epi32(T18, O08);          //
                __m256i T2_22 = _mm256_sub_epi32(T19, O09);          //
                __m256i T2_21 = _mm256_sub_epi32(T1A, O10);          //
                __m256i T2_20 = _mm256_sub_epi32(T1B, O11);          //
                __m256i T2_19 = _mm256_sub_epi32(T1C, O12);          //
                __m256i T2_18 = _mm256_sub_epi32(T1D, O13);          //
                __m256i T2_17 = _mm256_sub_epi32(T1E, O14);          //
                __m256i T2_16 = _mm256_sub_epi32(T1F, O15);          //

                T2_00 = _mm256_srai_epi32(T2_00, shift);             // [30 20 10 00]
                T2_01 = _mm256_srai_epi32(T2_01, shift);             // [31 21 11 01]
                T2_02 = _mm256_srai_epi32(T2_02, shift);             // [32 22 12 02]
                T2_03 = _mm256_srai_epi32(T2_03, shift);             // [33 23 13 03]
                T2_04 = _mm256_srai_epi32(T2_04, shift);             // [33 24 14 04]
                T2_05 = _mm256_srai_epi32(T2_05, shift);             // [35 25 15 05]
                T2_06 = _mm256_srai_epi32(T2_06, shift);             // [36 26 16 06]
                T2_07 = _mm256_srai_epi32(T2_07, shift);             // [37 27 17 07]
                T2_08 = _mm256_srai_epi32(T2_08, shift);             // [30 20 10 00] x8
                T2_09 = _mm256_srai_epi32(T2_09, shift);             // [31 21 11 01] x9
                T2_10 = _mm256_srai_epi32(T2_10, shift);             // [32 22 12 02] xA
                T2_11 = _mm256_srai_epi32(T2_11, shift);             // [33 23 13 03] xB
                T2_12 = _mm256_srai_epi32(T2_12, shift);             // [33 24 14 04] xC
                T2_13 = _mm256_srai_epi32(T2_13, shift);             // [35 25 15 05] xD
                T2_14 = _mm256_srai_epi32(T2_14, shift);             // [36 26 16 06] xE
                T2_15 = _mm256_srai_epi32(T2_15, shift);             // [37 27 17 07] xF
                T2_16 = _mm256_srai_epi32(T2_16, shift);             // [30 20 10 00]
                T2_17 = _mm256_srai_epi32(T2_17, shift);             // [31 21 11 01]
                T2_18 = _mm256_srai_epi32(T2_18, shift);             // [32 22 12 02]
                T2_19 = _mm256_srai_epi32(T2_19, shift);             // [33 23 13 03]
                T2_20 = _mm256_srai_epi32(T2_20, shift);             // [33 24 14 04]
                T2_21 = _mm256_srai_epi32(T2_21, shift);             // [35 25 15 05]
                T2_22 = _mm256_srai_epi32(T2_22, shift);             // [36 26 16 06]
                T2_23 = _mm256_srai_epi32(T2_23, shift);             // [37 27 17 07]
                T2_24 = _mm256_srai_epi32(T2_24, shift);             // [30 20 10 00] x8
                T2_25 = _mm256_srai_epi32(T2_25, shift);             // [31 21 11 01] x9
                T2_26 = _mm256_srai_epi32(T2_26, shift);             // [32 22 12 02] xA
                T2_27 = _mm256_srai_epi32(T2_27, shift);             // [33 23 13 03] xB
                T2_28 = _mm256_srai_epi32(T2_28, shift);             // [33 24 14 04] xC
                T2_29 = _mm256_srai_epi32(T2_29, shift);             // [35 25 15 05] xD
                T2_30 = _mm256_srai_epi32(T2_30, shift);             // [36 26 16 06] xE
                T2_31 = _mm256_srai_epi32(T2_31, shift);             // [37 27 17 07] xF

                //transpose 32x8 -> 8x32.
                TRANSPOSE_16x8_32BIT_16BIT(T2_00, T2_01, T2_02, T2_03, T2_04, T2_05, T2_06, T2_07,
                    T2_08, T2_09, T2_10, T2_11, T2_12, T2_13, T2_14, T2_15, res00, res02, res04, res06, res08, res10, res12, res14);
                TRANSPOSE_16x8_32BIT_16BIT(T2_16, T2_17, T2_18, T2_19, T2_20, T2_21, T2_22, T2_23,
                    T2_24, T2_25, T2_26, T2_27, T2_28, T2_29, T2_30, T2_31, res01, res03, res05, res07, res09, res11, res13, res15);
                
            }
            if (bit_depth != MAX_TX_DYNAMIC_RANGE) {
                __m256i max_val = _mm256_set1_epi16((1 << bit_depth) - 1);
                __m256i min_val = _mm256_set1_epi16(-(1 << bit_depth));

                res00 = _mm256_min_epi16(res00, max_val);
                res01 = _mm256_min_epi16(res01, max_val);
                res02 = _mm256_min_epi16(res02, max_val);
                res03 = _mm256_min_epi16(res03, max_val);
                res00 = _mm256_max_epi16(res00, min_val);
                res01 = _mm256_max_epi16(res01, min_val);
                res02 = _mm256_max_epi16(res02, min_val);
                res03 = _mm256_max_epi16(res03, min_val);
                res04 = _mm256_min_epi16(res04, max_val);
                res05 = _mm256_min_epi16(res05, max_val);
                res06 = _mm256_min_epi16(res06, max_val);
                res07 = _mm256_min_epi16(res07, max_val);
                res04 = _mm256_max_epi16(res04, min_val);
                res05 = _mm256_max_epi16(res05, min_val);
                res06 = _mm256_max_epi16(res06, min_val);
                res07 = _mm256_max_epi16(res07, min_val);

                res08 = _mm256_min_epi16(res08, max_val);
                res09 = _mm256_min_epi16(res09, max_val);
                res10 = _mm256_min_epi16(res10, max_val);
                res11 = _mm256_min_epi16(res11, max_val);
                res08 = _mm256_max_epi16(res08, min_val);
                res09 = _mm256_max_epi16(res09, min_val);
                res10 = _mm256_max_epi16(res10, min_val);
                res11 = _mm256_max_epi16(res11, min_val);
                res12 = _mm256_min_epi16(res12, max_val);
                res13 = _mm256_min_epi16(res13, max_val);
                res14 = _mm256_min_epi16(res14, max_val);
                res15 = _mm256_min_epi16(res15, max_val);
                res12 = _mm256_max_epi16(res12, min_val);
                res13 = _mm256_max_epi16(res13, min_val);
                res14 = _mm256_max_epi16(res14, min_val);
                res15 = _mm256_max_epi16(res15, min_val);

            }
        }
        _mm256_store_si256((__m256i*)&dst[0  * 16], res00);
        _mm256_store_si256((__m256i*)&dst[1  * 16], res01);
        _mm256_store_si256((__m256i*)&dst[2  * 16], res02);
        _mm256_store_si256((__m256i*)&dst[3  * 16], res03);
        _mm256_store_si256((__m256i*)&dst[4  * 16], res04);
        _mm256_store_si256((__m256i*)&dst[5  * 16], res05);
        _mm256_store_si256((__m256i*)&dst[6  * 16], res06);
        _mm256_store_si256((__m256i*)&dst[7  * 16], res07);
        _mm256_store_si256((__m256i*)&dst[8  * 16], res08);
        _mm256_store_si256((__m256i*)&dst[9  * 16], res09);
        _mm256_store_si256((__m256i*)&dst[10 * 16], res10);
        _mm256_store_si256((__m256i*)&dst[11 * 16], res11);
        _mm256_store_si256((__m256i*)&dst[12 * 16], res12);
        _mm256_store_si256((__m256i*)&dst[13 * 16], res13);
        _mm256_store_si256((__m256i*)&dst[14 * 16], res14);
        _mm256_store_si256((__m256i*)&dst[15 * 16], res15);

        dst += 8 * 32;  // 8rows
    }
}
void dct2_butterfly_h64_avx2(s16* src, int i_src, s16* dst, int line, int shift, int bit_depth)
{
    // O[32] coeffs
    const __m256i c16_p45_p45 = _mm256_set1_epi32(0x002d002d);
    const __m256i c16_p44_p44 = _mm256_set1_epi32(0x002c002c);
    const __m256i c16_p42_p43 = _mm256_set1_epi32(0x002a002b);
    const __m256i c16_p40_p41 = _mm256_set1_epi32(0x00280029);
    const __m256i c16_p38_p39 = _mm256_set1_epi32(0x00260027);
    const __m256i c16_p36_p37 = _mm256_set1_epi32(0x00240025);
    const __m256i c16_p33_p34 = _mm256_set1_epi32(0x00210022);
    const __m256i c16_p44_p45 = _mm256_set1_epi32(0x002c002d);
    const __m256i c16_p39_p42 = _mm256_set1_epi32(0x0027002a);
    const __m256i c16_p31_p36 = _mm256_set1_epi32(0x001f0024);
    const __m256i c16_p20_p26 = _mm256_set1_epi32(0x0014001a);
    const __m256i c16_p08_p14 = _mm256_set1_epi32(0x0008000e);
    const __m256i c16_n06_p01 = _mm256_set1_epi32(0xfffa0001);
    const __m256i c16_n18_n12 = _mm256_set1_epi32(0xffeefff4);
    const __m256i c16_n30_n24 = _mm256_set1_epi32(0xffe2ffe8);
    const __m256i c16_p42_p45 = _mm256_set1_epi32(0x002a002d);
    const __m256i c16_p30_p37 = _mm256_set1_epi32(0x001e0025);
    const __m256i c16_p10_p20 = _mm256_set1_epi32(0x000a0014);
    const __m256i c16_n12_n01 = _mm256_set1_epi32(0xfff4ffff);
    const __m256i c16_n31_n22 = _mm256_set1_epi32(0xffe1ffea);
    const __m256i c16_n43_n38 = _mm256_set1_epi32(0xffd5ffda);
    const __m256i c16_n45_n45 = _mm256_set1_epi32(0xffd3ffd3);
    const __m256i c16_n36_n41 = _mm256_set1_epi32(0xffdcffd7);
    const __m256i c16_p39_p45 = _mm256_set1_epi32(0x0027002d);
    const __m256i c16_p16_p30 = _mm256_set1_epi32(0x0010001e);
    const __m256i c16_n14_p01 = _mm256_set1_epi32(0xfff20001);
    const __m256i c16_n38_n28 = _mm256_set1_epi32(0xffdaffe4);
    const __m256i c16_n45_n44 = _mm256_set1_epi32(0xffd3ffd4);
    const __m256i c16_n31_n40 = _mm256_set1_epi32(0xffe1ffd8);
    const __m256i c16_n03_n18 = _mm256_set1_epi32(0xfffdffee);
    const __m256i c16_p26_p12 = _mm256_set1_epi32(0x001a000c);
    const __m256i c16_p36_p44 = _mm256_set1_epi32(0x0024002c);
    const __m256i c16_p01_p20 = _mm256_set1_epi32(0x00010014);
    const __m256i c16_n34_n18 = _mm256_set1_epi32(0xffdeffee);
    const __m256i c16_n22_n37 = _mm256_set1_epi32(0xffeaffdb);
    const __m256i c16_p16_n03 = _mm256_set1_epi32(0x0010fffd);
    const __m256i c16_p43_p33 = _mm256_set1_epi32(0x002b0021);
    const __m256i c16_p38_p45 = _mm256_set1_epi32(0x0026002d);
    const __m256i c16_p31_p44 = _mm256_set1_epi32(0x001f002c);
    const __m256i c16_n14_p10 = _mm256_set1_epi32(0xfff2000a);
    const __m256i c16_n45_n34 = _mm256_set1_epi32(0xffd3ffde);
    const __m256i c16_n28_n42 = _mm256_set1_epi32(0xffe4ffd6);
    const __m256i c16_p18_n06 = _mm256_set1_epi32(0x0012fffa);
    const __m256i c16_p45_p37 = _mm256_set1_epi32(0x002d0025);
    const __m256i c16_p24_p40 = _mm256_set1_epi32(0x00180028);
    const __m256i c16_n22_p01 = _mm256_set1_epi32(0xffea0001);
    const __m256i c16_p26_p43 = _mm256_set1_epi32(0x001a002b);
    const __m256i c16_n28_n01 = _mm256_set1_epi32(0xffe4ffff);
    const __m256i c16_n42_n44 = _mm256_set1_epi32(0xffd6ffd4);
    const __m256i c16_p03_n24 = _mm256_set1_epi32(0x0003ffe8);
    const __m256i c16_p44_p30 = _mm256_set1_epi32(0x002c001e);
    const __m256i c16_p22_p41 = _mm256_set1_epi32(0x00160029);
    const __m256i c16_n31_n06 = _mm256_set1_epi32(0xffe1fffa);
    const __m256i c16_n40_n45 = _mm256_set1_epi32(0xffd8ffd3);
    const __m256i c16_p20_p42 = _mm256_set1_epi32(0x0014002a);
    const __m256i c16_n38_n12 = _mm256_set1_epi32(0xffdafff4);
    const __m256i c16_n28_n45 = _mm256_set1_epi32(0xffe4ffd3);
    const __m256i c16_p33_p03 = _mm256_set1_epi32(0x00210003);
    const __m256i c16_p34_p45 = _mm256_set1_epi32(0x0022002d);
    const __m256i c16_n26_p06 = _mm256_set1_epi32(0xffe60006);
    const __m256i c16_n39_n44 = _mm256_set1_epi32(0xffd9ffd4);
    const __m256i c16_p18_n14 = _mm256_set1_epi32(0x0012fff2);
    const __m256i c16_p14_p41 = _mm256_set1_epi32(0x000e0029);
    const __m256i c16_n44_n22 = _mm256_set1_epi32(0xffd4ffea);
    const __m256i c16_n06_n37 = _mm256_set1_epi32(0xfffaffdb);
    const __m256i c16_p45_p30 = _mm256_set1_epi32(0x002d001e);
    const __m256i c16_n03_p31 = _mm256_set1_epi32(0xfffd001f);
    const __m256i c16_n45_n36 = _mm256_set1_epi32(0xffd3ffdc);
    const __m256i c16_p12_n24 = _mm256_set1_epi32(0x000cffe8);
    const __m256i c16_p42_p40 = _mm256_set1_epi32(0x002a0028);
    const __m256i c16_p08_p40 = _mm256_set1_epi32(0x00080028);
    const __m256i c16_n45_n31 = _mm256_set1_epi32(0xffd3ffe1);
    const __m256i c16_p18_n22 = _mm256_set1_epi32(0x0012ffea);
    const __m256i c16_p34_p44 = _mm256_set1_epi32(0x0022002c);
    const __m256i c16_n38_n03 = _mm256_set1_epi32(0xffdafffd);
    const __m256i c16_n12_n42 = _mm256_set1_epi32(0xfff4ffd6);
    const __m256i c16_p45_p28 = _mm256_set1_epi32(0x002d001c);
    const __m256i c16_n14_p26 = _mm256_set1_epi32(0xfff2001a);
    const __m256i c16_p01_p39 = _mm256_set1_epi32(0x00010027);
    const __m256i c16_n40_n38 = _mm256_set1_epi32(0xffd8ffda);
    const __m256i c16_p37_n03 = _mm256_set1_epi32(0x0025fffd);
    const __m256i c16_p06_p41 = _mm256_set1_epi32(0x00060029);
    const __m256i c16_n42_n36 = _mm256_set1_epi32(0xffd6ffdc);
    const __m256i c16_p34_n08 = _mm256_set1_epi32(0x0022fff8);
    const __m256i c16_p10_p43 = _mm256_set1_epi32(0x000a002b);
    const __m256i c16_n44_n33 = _mm256_set1_epi32(0xffd4ffdf);
    const __m256i c16_n06_p38 = _mm256_set1_epi32(0xfffa0026);
    const __m256i c16_n31_n43 = _mm256_set1_epi32(0xffe1ffd5);
    const __m256i c16_p45_p16 = _mm256_set1_epi32(0x002d0010);
    const __m256i c16_n26_p22 = _mm256_set1_epi32(0xffe60016);
    const __m256i c16_n12_n45 = _mm256_set1_epi32(0xfff4ffd3);
    const __m256i c16_p41_p34 = _mm256_set1_epi32(0x00290022);
    const __m256i c16_n40_p01 = _mm256_set1_epi32(0xffd80001);
    const __m256i c16_p10_n36 = _mm256_set1_epi32(0x000affdc);
    const __m256i c16_n12_p37 = _mm256_set1_epi32(0xfff40025);
    const __m256i c16_n18_n45 = _mm256_set1_epi32(0xffeeffd3);
    const __m256i c16_p40_p33 = _mm256_set1_epi32(0x00280021);
    const __m256i c16_n44_n06 = _mm256_set1_epi32(0xffd4fffa);
    const __m256i c16_p28_n24 = _mm256_set1_epi32(0x001cffe8);
    const __m256i c16_p01_p43 = _mm256_set1_epi32(0x0001002b);
    const __m256i c16_n30_n42 = _mm256_set1_epi32(0xffe2ffd6);
    const __m256i c16_p45_p22 = _mm256_set1_epi32(0x002d0016);
    const __m256i c16_n18_p36 = _mm256_set1_epi32(0xffee0024);
    const __m256i c16_n03_n45 = _mm256_set1_epi32(0xfffdffd3);
    const __m256i c16_p24_p43 = _mm256_set1_epi32(0x0018002b);
    const __m256i c16_n39_n31 = _mm256_set1_epi32(0xffd9ffe1);
    const __m256i c16_p45_p12 = _mm256_set1_epi32(0x002d000c);
    const __m256i c16_n40_p10 = _mm256_set1_epi32(0xffd8000a);
    const __m256i c16_p26_n30 = _mm256_set1_epi32(0x001affe2);
    const __m256i c16_n06_p42 = _mm256_set1_epi32(0xfffa002a);
    const __m256i c16_n24_p34 = _mm256_set1_epi32(0xffe80022);
    const __m256i c16_p12_n41 = _mm256_set1_epi32(0x000cffd7);
    const __m256i c16_p01_p45 = _mm256_set1_epi32(0x0001002d);
    const __m256i c16_n14_n45 = _mm256_set1_epi32(0xfff2ffd3);
    const __m256i c16_p26_p40 = _mm256_set1_epi32(0x001a0028);
    const __m256i c16_n36_n33 = _mm256_set1_epi32(0xffdcffdf);
    const __m256i c16_p42_p22 = _mm256_set1_epi32(0x002a0016);
    const __m256i c16_n45_n10 = _mm256_set1_epi32(0xffd3fff6);
    const __m256i c16_n30_p33 = _mm256_set1_epi32(0xffe20021);
    const __m256i c16_p26_n36 = _mm256_set1_epi32(0x001affdc);
    const __m256i c16_n22_p38 = _mm256_set1_epi32(0xffea0026);
    const __m256i c16_p18_n40 = _mm256_set1_epi32(0x0012ffd8);
    const __m256i c16_n14_p42 = _mm256_set1_epi32(0xfff2002a);
    const __m256i c16_p10_n44 = _mm256_set1_epi32(0x000affd4);
    const __m256i c16_n06_p45 = _mm256_set1_epi32(0xfffa002d);
    const __m256i c16_p01_n45 = _mm256_set1_epi32(0x0001ffd3);
    const __m256i c16_n34_p31 = _mm256_set1_epi32(0xffde001f);
    const __m256i c16_p37_n28 = _mm256_set1_epi32(0x0025ffe4);
    const __m256i c16_n39_p24 = _mm256_set1_epi32(0xffd90018);
    const __m256i c16_p41_n20 = _mm256_set1_epi32(0x0029ffec);
    const __m256i c16_n43_p16 = _mm256_set1_epi32(0xffd50010);
    const __m256i c16_p44_n12 = _mm256_set1_epi32(0x002cfff4);
    const __m256i c16_n45_p08 = _mm256_set1_epi32(0xffd30008);
    const __m256i c16_p45_n03 = _mm256_set1_epi32(0x002dfffd);
    const __m256i c16_n38_p30 = _mm256_set1_epi32(0xffda001e);
    const __m256i c16_p44_n18 = _mm256_set1_epi32(0x002cffee);
    const __m256i c16_n45_p06 = _mm256_set1_epi32(0xffd30006);
    const __m256i c16_p43_p08 = _mm256_set1_epi32(0x002b0008);
    const __m256i c16_n37_n20 = _mm256_set1_epi32(0xffdbffec);
    const __m256i c16_p28_p31 = _mm256_set1_epi32(0x001c001f);
    const __m256i c16_n16_n39 = _mm256_set1_epi32(0xfff0ffd9);
    const __m256i c16_p03_p44 = _mm256_set1_epi32(0x0003002c);
    const __m256i c16_n41_p28 = _mm256_set1_epi32(0xffd7001c);
    const __m256i c16_p45_n08 = _mm256_set1_epi32(0x002dfff8);
    const __m256i c16_n38_n14 = _mm256_set1_epi32(0xffdafff2);
    const __m256i c16_p22_p33 = _mm256_set1_epi32(0x00160021);
    const __m256i c16_n01_n44 = _mm256_set1_epi32(0xffffffd4);
    const __m256i c16_n20_p44 = _mm256_set1_epi32(0xffec002c);
    const __m256i c16_p37_n34 = _mm256_set1_epi32(0x0025ffde);
    const __m256i c16_n45_p16 = _mm256_set1_epi32(0xffd30010);
    const __m256i c16_n44_p26 = _mm256_set1_epi32(0xffd4001a);
    const __m256i c16_p41_p03 = _mm256_set1_epi32(0x00290003);
    const __m256i c16_n20_n31 = _mm256_set1_epi32(0xffecffe1);
    const __m256i c16_n10_p45 = _mm256_set1_epi32(0xfff6002d);
    const __m256i c16_p36_n38 = _mm256_set1_epi32(0x0024ffda);
    const __m256i c16_n45_p14 = _mm256_set1_epi32(0xffd3000e);
    const __m256i c16_p34_p16 = _mm256_set1_epi32(0x00220010);
    const __m256i c16_n08_n39 = _mm256_set1_epi32(0xfff8ffd9);
    const __m256i c16_n45_p24 = _mm256_set1_epi32(0xffd30018);
    const __m256i c16_p33_p14 = _mm256_set1_epi32(0x0021000e);
    const __m256i c16_p03_n42 = _mm256_set1_epi32(0x0003ffd6);
    const __m256i c16_n37_p39 = _mm256_set1_epi32(0xffdb0027);
    const __m256i c16_p44_n08 = _mm256_set1_epi32(0x002cfff8);
    const __m256i c16_n18_n30 = _mm256_set1_epi32(0xffeeffe2);
    const __m256i c16_n20_p45 = _mm256_set1_epi32(0xffec002d);
    const __m256i c16_p44_n28 = _mm256_set1_epi32(0x002cffe4);
    const __m256i c16_n45_p22 = _mm256_set1_epi32(0xffd30016);
    const __m256i c16_p20_p24 = _mm256_set1_epi32(0x00140018);
    const __m256i c16_p26_n45 = _mm256_set1_epi32(0x001affd3);
    const __m256i c16_n45_p18 = _mm256_set1_epi32(0xffd30012);
    const __m256i c16_p16_p28 = _mm256_set1_epi32(0x0010001c);
    const __m256i c16_p30_n45 = _mm256_set1_epi32(0x001effd3);
    const __m256i c16_n44_p14 = _mm256_set1_epi32(0xffd4000e);
    const __m256i c16_p12_p31 = _mm256_set1_epi32(0x000c001f);
    const __m256i c16_n45_p20 = _mm256_set1_epi32(0xffd30014);
    const __m256i c16_p06_p33 = _mm256_set1_epi32(0x00060021);
    const __m256i c16_p41_n39 = _mm256_set1_epi32(0x0029ffd9);
    const __m256i c16_n30_n10 = _mm256_set1_epi32(0xffe2fff6);
    const __m256i c16_n24_p45 = _mm256_set1_epi32(0xffe8002d);
    const __m256i c16_p44_n16 = _mm256_set1_epi32(0x002cfff0);
    const __m256i c16_n01_n36 = _mm256_set1_epi32(0xffffffdc);
    const __m256i c16_n43_p37 = _mm256_set1_epi32(0xffd50025);
    const __m256i c16_n43_p18 = _mm256_set1_epi32(0xffd50012);
    const __m256i c16_n10_p39 = _mm256_set1_epi32(0xfff60027);
    const __m256i c16_p45_n26 = _mm256_set1_epi32(0x002dffe6);
    const __m256i c16_p01_n34 = _mm256_set1_epi32(0x0001ffde);
    const __m256i c16_n45_p33 = _mm256_set1_epi32(0xffd30021);
    const __m256i c16_p08_p28 = _mm256_set1_epi32(0x0008001c);
    const __m256i c16_p44_n38 = _mm256_set1_epi32(0x002cffda);
    const __m256i c16_n16_n20 = _mm256_set1_epi32(0xfff0ffec);
    const __m256i c16_n40_p16 = _mm256_set1_epi32(0xffd80010);
    const __m256i c16_n24_p44 = _mm256_set1_epi32(0xffe8002c);
    const __m256i c16_p36_n08 = _mm256_set1_epi32(0x0024fff8);
    const __m256i c16_p31_n45 = _mm256_set1_epi32(0x001fffd3);
    const __m256i c16_n30_n01 = _mm256_set1_epi32(0xffe2ffff);
    const __m256i c16_n37_p45 = _mm256_set1_epi32(0xffdb002d);
    const __m256i c16_p22_p10 = _mm256_set1_epi32(0x0016000a);
    const __m256i c16_p41_n43 = _mm256_set1_epi32(0x0029ffd5);
    const __m256i c16_n37_p14 = _mm256_set1_epi32(0xffdb000e);
    const __m256i c16_n36_p45 = _mm256_set1_epi32(0xffdc002d);
    const __m256i c16_p16_p12 = _mm256_set1_epi32(0x0010000c);
    const __m256i c16_p45_n38 = _mm256_set1_epi32(0x002dffda);
    const __m256i c16_p10_n34 = _mm256_set1_epi32(0x000affde);
    const __m256i c16_n39_p18 = _mm256_set1_epi32(0xffd90012);
    const __m256i c16_n33_p45 = _mm256_set1_epi32(0xffdf002d);
    const __m256i c16_p20_p08 = _mm256_set1_epi32(0x00140008);
    const __m256i c16_n33_p12 = _mm256_set1_epi32(0xffdf000c);
    const __m256i c16_n43_p44 = _mm256_set1_epi32(0xffd5002c);
    const __m256i c16_n08_p30 = _mm256_set1_epi32(0xfff8001e);
    const __m256i c16_p36_n16 = _mm256_set1_epi32(0x0024fff0);
    const __m256i c16_p41_n45 = _mm256_set1_epi32(0x0029ffd3);
    const __m256i c16_p03_n26 = _mm256_set1_epi32(0x0003ffe6);
    const __m256i c16_n38_p20 = _mm256_set1_epi32(0xffda0014);
    const __m256i c16_n39_p45 = _mm256_set1_epi32(0xffd9002d);
    const __m256i c16_n28_p10 = _mm256_set1_epi32(0xffe4000a);
    const __m256i c16_n45_p40 = _mm256_set1_epi32(0xffd30028);
    const __m256i c16_n30_p41 = _mm256_set1_epi32(0xffe20029);
    const __m256i c16_p08_p12 = _mm256_set1_epi32(0x0008000c);
    const __m256i c16_p39_n26 = _mm256_set1_epi32(0x0027ffe6);
    const __m256i c16_p42_n45 = _mm256_set1_epi32(0x002affd3);
    const __m256i c16_p14_n31 = _mm256_set1_epi32(0x000effe1);
    const __m256i c16_n24_p06 = _mm256_set1_epi32(0xffe80006);
    const __m256i c16_n22_p08 = _mm256_set1_epi32(0xffea0008);
    const __m256i c16_n42_p34 = _mm256_set1_epi32(0xffd60022);
    const __m256i c16_n43_p45 = _mm256_set1_epi32(0xffd5002d);
    const __m256i c16_n24_p36 = _mm256_set1_epi32(0xffe80024);
    const __m256i c16_p06_p10 = _mm256_set1_epi32(0x0006000a);
    const __m256i c16_p33_n20 = _mm256_set1_epi32(0x0021ffec);
    const __m256i c16_p45_n41 = _mm256_set1_epi32(0x002dffd7);
    const __m256i c16_p37_n44 = _mm256_set1_epi32(0x0025ffd4);
    const __m256i c16_n16_p06 = _mm256_set1_epi32(0xfff00006);
    const __m256i c16_n34_p26 = _mm256_set1_epi32(0xffde001a);
    const __m256i c16_n44_p40 = _mm256_set1_epi32(0xffd40028);
    const __m256i c16_n44_p45 = _mm256_set1_epi32(0xffd4002d);
    const __m256i c16_n33_p39 = _mm256_set1_epi32(0xffdf0027);
    const __m256i c16_n14_p24 = _mm256_set1_epi32(0xfff20018);
    const __m256i c16_p08_p03 = _mm256_set1_epi32(0x00080003);
    const __m256i c16_p28_n18 = _mm256_set1_epi32(0x001cffee);
    const __m256i c16_n10_p03 = _mm256_set1_epi32(0xfff60003);
    const __m256i c16_n22_p16 = _mm256_set1_epi32(0xffea0010);
    const __m256i c16_n33_p28 = _mm256_set1_epi32(0xffdf001c);
    const __m256i c16_n40_p37 = _mm256_set1_epi32(0xffd80025);
    const __m256i c16_n45_p43 = _mm256_set1_epi32(0xffd3002b);
    const __m256i c16_n45_p45 = _mm256_set1_epi32(0xffd3002d);
    const __m256i c16_n41_p44 = _mm256_set1_epi32(0xffd7002c);
    const __m256i c16_n34_p38 = _mm256_set1_epi32(0xffde0026);
    const __m256i c16_n03_p01 = _mm256_set1_epi32(0xfffd0001);
    const __m256i c16_n08_p06 = _mm256_set1_epi32(0xfff80006);
    const __m256i c16_n12_p10 = _mm256_set1_epi32(0xfff4000a);
    const __m256i c16_n16_p14 = _mm256_set1_epi32(0xfff0000e);
    const __m256i c16_n20_p18 = _mm256_set1_epi32(0xffec0012);
    const __m256i c16_n24_p22 = _mm256_set1_epi32(0xffe80016);
    const __m256i c16_n28_p26 = _mm256_set1_epi32(0xffe4001a);
    const __m256i c16_n31_p30 = _mm256_set1_epi32(0xffe1001e);

    // EO[16] coeffs
    const __m256i c16_p43_p44 = _mm256_set1_epi32(0x002b002c);
    const __m256i c16_p39_p41 = _mm256_set1_epi32(0x00270029);
    const __m256i c16_p34_p36 = _mm256_set1_epi32(0x00220024);
    const __m256i c16_p41_p45 = _mm256_set1_epi32(0x0029002d);
    const __m256i c16_p23_p34 = _mm256_set1_epi32(0x00170022);
    const __m256i c16_n02_p11 = _mm256_set1_epi32(0xfffe000b);
    const __m256i c16_n27_n15 = _mm256_set1_epi32(0xffe5fff1);
    const __m256i c16_n07_p15 = _mm256_set1_epi32(0xfff9000f);
    const __m256i c16_n41_n27 = _mm256_set1_epi32(0xffd7ffe5);
    const __m256i c16_n39_n45 = _mm256_set1_epi32(0xffd9ffd3);
    const __m256i c16_p23_p43 = _mm256_set1_epi32(0x0017002b);
    const __m256i c16_n34_n07 = _mm256_set1_epi32(0xffdefff9);
    const __m256i c16_n36_n45 = _mm256_set1_epi32(0xffdcffd3);
    const __m256i c16_p19_n11 = _mm256_set1_epi32(0x0013fff5);
    const __m256i c16_p11_p41 = _mm256_set1_epi32(0x000b0029);
    const __m256i c16_n45_n27 = _mm256_set1_epi32(0xffd3ffe5);
    const __m256i c16_p07_n30 = _mm256_set1_epi32(0x0007ffe2);
    const __m256i c16_p43_p39 = _mm256_set1_epi32(0x002b0027);
    const __m256i c16_n02_p39 = _mm256_set1_epi32(0xfffe0027);
    const __m256i c16_p43_p07 = _mm256_set1_epi32(0x002b0007);
    const __m256i c16_n11_p34 = _mm256_set1_epi32(0xfff50022);
    const __m256i c16_n15_p36 = _mm256_set1_epi32(0xfff10024);
    const __m256i c16_n11_n45 = _mm256_set1_epi32(0xfff5ffd3);
    const __m256i c16_p34_p39 = _mm256_set1_epi32(0x00220027);
    const __m256i c16_n45_n19 = _mm256_set1_epi32(0xffd3ffed);
    const __m256i c16_n27_p34 = _mm256_set1_epi32(0xffe50022);
    const __m256i c16_p19_n39 = _mm256_set1_epi32(0x0013ffd9);
    const __m256i c16_n11_p43 = _mm256_set1_epi32(0xfff5002b);
    const __m256i c16_p02_n45 = _mm256_set1_epi32(0x0002ffd3);
    const __m256i c16_n36_p30 = _mm256_set1_epi32(0xffdc001e);
    const __m256i c16_p41_n23 = _mm256_set1_epi32(0x0029ffe9);
    const __m256i c16_n44_p15 = _mm256_set1_epi32(0xffd4000f);
    const __m256i c16_p45_n07 = _mm256_set1_epi32(0x002dfff9);
    const __m256i c16_n43_p27 = _mm256_set1_epi32(0xffd5001b);
    const __m256i c16_p44_n02 = _mm256_set1_epi32(0x002cfffe);
    const __m256i c16_n30_n23 = _mm256_set1_epi32(0xffe2ffe9);
    const __m256i c16_p07_p41 = _mm256_set1_epi32(0x00070029);
    const __m256i c16_n45_p23 = _mm256_set1_epi32(0xffd30017);
    const __m256i c16_p27_p19 = _mm256_set1_epi32(0x001b0013);
    const __m256i c16_p15_n45 = _mm256_set1_epi32(0x000fffd3);
    const __m256i c16_n44_p30 = _mm256_set1_epi32(0xffd4001e);
    const __m256i c16_n44_p19 = _mm256_set1_epi32(0xffd40013);
    const __m256i c16_n02_p36 = _mm256_set1_epi32(0xfffe0024);
    const __m256i c16_p45_n34 = _mm256_set1_epi32(0x002dffde);
    const __m256i c16_n15_n23 = _mm256_set1_epi32(0xfff1ffe9);
    const __m256i c16_n39_p15 = _mm256_set1_epi32(0xffd9000f);
    const __m256i c16_n30_p45 = _mm256_set1_epi32(0xffe2002d);
    const __m256i c16_p27_p02 = _mm256_set1_epi32(0x001b0002);
    const __m256i c16_p41_n44 = _mm256_set1_epi32(0x0029ffd4);
    const __m256i c16_n30_p11 = _mm256_set1_epi32(0xffe2000b);
    const __m256i c16_n19_p36 = _mm256_set1_epi32(0xffed0024);
    const __m256i c16_p23_n02 = _mm256_set1_epi32(0x0017fffe);
    const __m256i c16_n19_p07 = _mm256_set1_epi32(0xffed0007);
    const __m256i c16_n39_p30 = _mm256_set1_epi32(0xffd9001e);
    const __m256i c16_n45_p44 = _mm256_set1_epi32(0xffd3002c);
    const __m256i c16_n36_p43 = _mm256_set1_epi32(0xffdc002b);
    const __m256i c16_n07_p02 = _mm256_set1_epi32(0xfff90002);
    const __m256i c16_n15_p11 = _mm256_set1_epi32(0xfff1000b);
    const __m256i c16_n23_p19 = _mm256_set1_epi32(0xffe90013);
    const __m256i c16_n30_p27 = _mm256_set1_epi32(0xffe2001b);

    // EEO[8] coeffs
    const __m256i c16_p43_p45 = _mm256_set1_epi32(0x002b002d);
    const __m256i c16_p35_p40 = _mm256_set1_epi32(0x00230028);
    const __m256i c16_p29_p43 = _mm256_set1_epi32(0x001d002b);
    const __m256i c16_n21_p04 = _mm256_set1_epi32(0xffeb0004);
    const __m256i c16_p04_p40 = _mm256_set1_epi32(0x00040028);
    const __m256i c16_n43_n35 = _mm256_set1_epi32(0xffd5ffdd);
    const __m256i c16_n21_p35 = _mm256_set1_epi32(0xffeb0023);
    const __m256i c16_p04_n43 = _mm256_set1_epi32(0x0004ffd5);
    const __m256i c16_n40_p29 = _mm256_set1_epi32(0xffd8001d);
    const __m256i c16_p45_n13 = _mm256_set1_epi32(0x002dfff3);
    const __m256i c16_n45_p21 = _mm256_set1_epi32(0xffd30015);
    const __m256i c16_p13_p29 = _mm256_set1_epi32(0x000d001d);
    const __m256i c16_n35_p13 = _mm256_set1_epi32(0xffdd000d);
    const __m256i c16_n40_p45 = _mm256_set1_epi32(0xffd8002d);
    const __m256i c16_n13_p04 = _mm256_set1_epi32(0xfff30004);
    const __m256i c16_n29_p21 = _mm256_set1_epi32(0xffe30015);

    // EEEO[4] coeffs
    const __m256i c16_p38_p44 = _mm256_set1_epi32(0x0026002c);
    const __m256i c16_n09_p38 = _mm256_set1_epi32(0xfff70026);
    const __m256i c16_n44_p25 = _mm256_set1_epi32(0xffd40019);
    const __m256i c16_n25_p09 = _mm256_set1_epi32(0xffe70009);

    // EEEE[4] coeffs
    const __m256i c16_p42_p32 = _mm256_set1_epi32(0x002a0020);
    const __m256i c16_p17_p32 = _mm256_set1_epi32(0x00110020);
    const __m256i c16_n17_p32 = _mm256_set1_epi32(0xffef0020);
    const __m256i c16_n42_p32 = _mm256_set1_epi32(0xffd60020);

    __m256i c32_off = _mm256_set1_epi32(1 << (shift - 1));

    __m128i in00, in01, in02, in03, in04, in05, in06, in07, in08, in09, in10, in11, in12, in13, in14, in15;
    __m128i in16, in17, in18, in19, in20, in21, in22, in23, in24, in25, in26, in27, in28, in29, in30, in31;
    __m128i ss00, ss01, ss02, ss03, ss04, ss05, ss06, ss07, ss08, ss09, ss10, ss11, ss12, ss13, ss14, ss15;
    __m256i res00, res01, res02, res03, res04, res05, res06, res07, res08, res09, res10, res11, res12, res13, res14, res15;
    __m256i res16, res17, res18, res19, res20, res21, res22, res23, res24, res25, res26, res27, res28, res29, res30, res31;

    int i;

    for (i = 0; i < line; i += 8)
    {
        in01 = _mm_load_si128((const __m128i*)&src[1 * i_src + i]);
        in03 = _mm_load_si128((const __m128i*)&src[3 * i_src + i]);
        in05 = _mm_load_si128((const __m128i*)&src[5 * i_src + i]);
        in07 = _mm_load_si128((const __m128i*)&src[7 * i_src + i]);
        in09 = _mm_load_si128((const __m128i*)&src[9 * i_src + i]);
        in11 = _mm_load_si128((const __m128i*)&src[11 * i_src + i]);
        in13 = _mm_load_si128((const __m128i*)&src[13 * i_src + i]);
        in15 = _mm_load_si128((const __m128i*)&src[15 * i_src + i]);
        in17 = _mm_load_si128((const __m128i*)&src[17 * i_src + i]);
        in19 = _mm_load_si128((const __m128i*)&src[19 * i_src + i]);
        in21 = _mm_load_si128((const __m128i*)&src[21 * i_src + i]);
        in23 = _mm_load_si128((const __m128i*)&src[23 * i_src + i]);
        in25 = _mm_load_si128((const __m128i*)&src[25 * i_src + i]);
        in27 = _mm_load_si128((const __m128i*)&src[27 * i_src + i]);
        in29 = _mm_load_si128((const __m128i*)&src[29 * i_src + i]);
        in31 = _mm_load_si128((const __m128i*)&src[31 * i_src + i]);

        ss00 = _mm_unpacklo_epi16(in01, in03);
        ss01 = _mm_unpacklo_epi16(in05, in07);
        ss02 = _mm_unpacklo_epi16(in09, in11);
        ss03 = _mm_unpacklo_epi16(in13, in15);
        ss04 = _mm_unpacklo_epi16(in17, in19);
        ss05 = _mm_unpacklo_epi16(in21, in23);
        ss06 = _mm_unpacklo_epi16(in25, in27);
        ss07 = _mm_unpacklo_epi16(in29, in31);

        ss08 = _mm_unpackhi_epi16(in01, in03);
        ss09 = _mm_unpackhi_epi16(in05, in07);
        ss10 = _mm_unpackhi_epi16(in09, in11);
        ss11 = _mm_unpackhi_epi16(in13, in15);
        ss12 = _mm_unpackhi_epi16(in17, in19);
        ss13 = _mm_unpackhi_epi16(in21, in23);
        ss14 = _mm_unpackhi_epi16(in25, in27);
        ss15 = _mm_unpackhi_epi16(in29, in31);


        {
            const __m256i T_00_00 = _mm256_set_m128i(ss08, ss00);       // [33 13 32 12 31 11 30 10]
            const __m256i T_00_01 = _mm256_set_m128i(ss09, ss01);       
            const __m256i T_00_02 = _mm256_set_m128i(ss10, ss02);       
            const __m256i T_00_03 = _mm256_set_m128i(ss11, ss03);       
            const __m256i T_00_04 = _mm256_set_m128i(ss12, ss04);       
            const __m256i T_00_05 = _mm256_set_m128i(ss13, ss05);       
            const __m256i T_00_06 = _mm256_set_m128i(ss14, ss06);       
            const __m256i T_00_07 = _mm256_set_m128i(ss15, ss07);       

            __m256i O00, O01, O02, O03, O04, O05, O06, O07, O08, O09, O10, O11, O12, O13, O14, O15;
            __m256i O16, O17, O18, O19, O20, O21, O22, O23, O24, O25, O26, O27, O28, O29, O30, O31;
            __m256i EO00, EO01, EO02, EO03, EO04, EO05, EO06, EO07, EO08, EO09, EO10, EO11, EO12, EO13, EO14, EO15;
            {
                __m256i t0, t1, t2, t3, t4, t5, t6, t7;
#define COMPUTE_ROW(r0103, r0507, r0911, r1315, r1719, r2123, r2527, r2931, c0103, c0507, c0911, c1315, c1719, c2123, c2527, c2931, row) \
                t0 = _mm256_madd_epi16(r0103, c0103); \
                t1 = _mm256_madd_epi16(r0507, c0507); \
                t2 = _mm256_madd_epi16(r0911, c0911); \
                t3 = _mm256_madd_epi16(r1315, c1315); \
                t4 = _mm256_madd_epi16(r1719, c1719); \
                t5 = _mm256_madd_epi16(r2123, c2123); \
                t6 = _mm256_madd_epi16(r2527, c2527); \
                t7 = _mm256_madd_epi16(r2931, c2931); \
	            t0 = _mm256_add_epi32(t0, t1); \
	            t2 = _mm256_add_epi32(t2, t3); \
	            t4 = _mm256_add_epi32(t4, t5); \
	            t6 = _mm256_add_epi32(t6, t7); \
	            row = _mm256_add_epi32(_mm256_add_epi32(t0, t2), _mm256_add_epi32(t4, t6));

                // O[32] 
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p45_p45, c16_p45_p45, c16_p44_p44, c16_p42_p43, c16_p40_p41, c16_p38_p39, c16_p36_p37, c16_p33_p34, O00)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p44_p45, c16_p39_p42, c16_p31_p36, c16_p20_p26, c16_p08_p14, c16_n06_p01, c16_n18_n12, c16_n30_n24, O01)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p42_p45, c16_p30_p37, c16_p10_p20, c16_n12_n01, c16_n31_n22, c16_n43_n38, c16_n45_n45, c16_n36_n41, O02)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p39_p45, c16_p16_p30, c16_n14_p01, c16_n38_n28, c16_n45_n44, c16_n31_n40, c16_n03_n18, c16_p26_p12, O03)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p36_p44, c16_p01_p20, c16_n34_n18, c16_n45_n44, c16_n22_n37, c16_p16_n03, c16_p43_p33, c16_p38_p45, O04)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p31_p44, c16_n14_p10, c16_n45_n34, c16_n28_n42, c16_p18_n06, c16_p45_p37, c16_p24_p40, c16_n22_p01, O05)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p26_p43, c16_n28_n01, c16_n42_n44, c16_p03_n24, c16_p44_p30, c16_p22_p41, c16_n31_n06, c16_n40_n45, O06)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p20_p42, c16_n38_n12, c16_n28_n45, c16_p33_p03, c16_p34_p45, c16_n26_p06, c16_n39_n44, c16_p18_n14, O07)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p14_p41, c16_n44_n22, c16_n06_n37, c16_p45_p30, c16_n03_p31, c16_n45_n36, c16_p12_n24, c16_p42_p40, O08)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p08_p40, c16_n45_n31, c16_p18_n22, c16_p34_p44, c16_n38_n03, c16_n12_n42, c16_p45_p28, c16_n14_p26, O09)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_p01_p39, c16_n40_n38, c16_p37_n03, c16_p06_p41, c16_n42_n36, c16_p34_n08, c16_p10_p43, c16_n44_n33, O10)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n06_p38, c16_n31_n43, c16_p45_p16, c16_n26_p22, c16_n12_n45, c16_p41_p34, c16_n40_p01, c16_p10_n36, O11)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n12_p37, c16_n18_n45, c16_p40_p33, c16_n44_n06, c16_p28_n24, c16_p01_p43, c16_n30_n42, c16_p45_p22, O12)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n18_p36, c16_n03_n45, c16_p24_p43, c16_n39_n31, c16_p45_p12, c16_n40_p10, c16_p26_n30, c16_n06_p42, O13)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n24_p34, c16_p12_n41, c16_p01_p45, c16_n14_n45, c16_p26_p40, c16_n36_n33, c16_p42_p22, c16_n45_n10, O14)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n30_p33, c16_p26_n36, c16_n22_p38, c16_p18_n40, c16_n14_p42, c16_p10_n44, c16_n06_p45, c16_p01_n45, O15)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n34_p31, c16_p37_n28, c16_n39_p24, c16_p41_n20, c16_n43_p16, c16_p44_n12, c16_n45_p08, c16_p45_n03, O16)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n38_p30, c16_p44_n18, c16_n45_p06, c16_p43_p08, c16_n37_n20, c16_p28_p31, c16_n16_n39, c16_p03_p44, O17)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n41_p28, c16_p45_n08, c16_n38_n14, c16_p22_p33, c16_n01_n44, c16_n20_p44, c16_p37_n34, c16_n45_p16, O18)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n44_p26, c16_p41_p03, c16_n20_n31, c16_n10_p45, c16_p36_n38, c16_n45_p14, c16_p34_p16, c16_n08_n39, O19)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n45_p24, c16_p33_p14, c16_p03_n42, c16_n37_p39, c16_p44_n08, c16_n18_n30, c16_n20_p45, c16_p44_n28, O20)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n45_p22, c16_p20_p24, c16_p26_n45, c16_n45_p18, c16_p16_p28, c16_p30_n45, c16_n44_p14, c16_p12_p31, O21)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n45_p20, c16_p06_p33, c16_p41_n39, c16_n30_n10, c16_n24_p45, c16_p44_n16, c16_n01_n36, c16_n43_p37, O22)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n43_p18, c16_n10_p39, c16_p45_n26, c16_p01_n34, c16_n45_p33, c16_p08_p28, c16_p44_n38, c16_n16_n20, O23)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n40_p16, c16_n24_p44, c16_p36_n08, c16_p31_n45, c16_n30_n01, c16_n37_p45, c16_p22_p10, c16_p41_n43, O24)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n37_p14, c16_n36_p45, c16_p16_p12, c16_p45_n38, c16_p10_n34, c16_n39_p18, c16_n33_p45, c16_p20_p08, O25)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n33_p12, c16_n43_p44, c16_n08_p30, c16_p36_n16, c16_p41_n45, c16_p03_n26, c16_n38_p20, c16_n39_p45, O26)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n28_p10, c16_n45_p40, c16_n30_p41, c16_p08_p12, c16_p39_n26, c16_p42_n45, c16_p14_n31, c16_n24_p06, O27)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n22_p08, c16_n42_p34, c16_n43_p45, c16_n24_p36, c16_p06_p10, c16_p33_n20, c16_p45_n41, c16_p37_n44, O28)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n16_p06, c16_n34_p26, c16_n44_p40, c16_n44_p45, c16_n33_p39, c16_n14_p24, c16_p08_p03, c16_p28_n18, O29)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n10_p03, c16_n22_p16, c16_n33_p28, c16_n40_p37, c16_n45_p43, c16_n45_p45, c16_n41_p44, c16_n34_p38, O30)
                COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07, \
                    c16_n03_p01, c16_n08_p06, c16_n12_p10, c16_n16_p14, c16_n20_p18, c16_n24_p22, c16_n28_p26, c16_n31_p30, O31)

#undef COMPUTE_ROW
            }

            in00 = _mm_load_si128((const __m128i*)&src[0 * i_src + i]);
            in02 = _mm_load_si128((const __m128i*)&src[2 * i_src + i]);
            in04 = _mm_load_si128((const __m128i*)&src[4 * i_src + i]);
            in06 = _mm_load_si128((const __m128i*)&src[6 * i_src + i]);
            in08 = _mm_load_si128((const __m128i*)&src[8 * i_src + i]);
            in10 = _mm_load_si128((const __m128i*)&src[10 * i_src + i]);
            in12 = _mm_load_si128((const __m128i*)&src[12 * i_src + i]);
            in14 = _mm_load_si128((const __m128i*)&src[14 * i_src + i]);
            in16 = _mm_load_si128((const __m128i*)&src[16 * i_src + i]);
            in18 = _mm_load_si128((const __m128i*)&src[18 * i_src + i]);
            in20 = _mm_load_si128((const __m128i*)&src[20 * i_src + i]);
            in22 = _mm_load_si128((const __m128i*)&src[22 * i_src + i]);
            in24 = _mm_load_si128((const __m128i*)&src[24 * i_src + i]);
            in26 = _mm_load_si128((const __m128i*)&src[26 * i_src + i]);
            in28 = _mm_load_si128((const __m128i*)&src[28 * i_src + i]);
            in30 = _mm_load_si128((const __m128i*)&src[30 * i_src + i]);

            ss00 = _mm_unpacklo_epi16(in02, in06);
            ss01 = _mm_unpacklo_epi16(in10, in14);
            ss02 = _mm_unpacklo_epi16(in18, in22);
            ss03 = _mm_unpacklo_epi16(in26, in30);
            ss04 = _mm_unpacklo_epi16(in04, in12);
            ss05 = _mm_unpacklo_epi16(in20, in28);
            ss06 = _mm_unpacklo_epi16(in08, in24);
            ss07 = _mm_unpacklo_epi16(in00, in16);

            ss08 = _mm_unpackhi_epi16(in02, in06);
            ss09 = _mm_unpackhi_epi16(in10, in14);
            ss10 = _mm_unpackhi_epi16(in18, in22);
            ss11 = _mm_unpackhi_epi16(in26, in30);
            ss12 = _mm_unpackhi_epi16(in04, in12);
            ss13 = _mm_unpackhi_epi16(in20, in28);
            ss14 = _mm_unpackhi_epi16(in08, in24);
            ss15 = _mm_unpackhi_epi16(in00, in16);

            {
                __m256i T1, T2;
                const __m256i T_00_08 = _mm256_set_m128i(ss08, ss00);
                const __m256i T_00_09 = _mm256_set_m128i(ss09, ss01);
                const __m256i T_00_10 = _mm256_set_m128i(ss10, ss02);
                const __m256i T_00_11 = _mm256_set_m128i(ss11, ss03);
                const __m256i T_00_12 = _mm256_set_m128i(ss12, ss04);
                const __m256i T_00_13 = _mm256_set_m128i(ss13, ss05);
                const __m256i T_00_14 = _mm256_set_m128i(ss14, ss06);
                const __m256i T_00_15 = _mm256_set_m128i(ss15, ss07);

#define COMPUTE_ROW(row0206, row1014, row1822, row2630, c0206, c1014, c1822, c2630, row) \
	            T1 = _mm256_add_epi32(_mm256_madd_epi16(row0206, c0206), _mm256_madd_epi16(row1014, c1014)); \
	            T2 = _mm256_add_epi32(_mm256_madd_epi16(row1822, c1822), _mm256_madd_epi16(row2630, c2630)); \
	            row = _mm256_add_epi32(T1, T2);

                // EO[16] 
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_p45_p45, c16_p43_p44, c16_p39_p41, c16_p34_p36, EO00)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_p41_p45, c16_p23_p34, c16_n02_p11, c16_n27_n15, EO01)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_p34_p44, c16_n07_p15, c16_n41_n27, c16_n39_n45, EO02)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_p23_p43, c16_n34_n07, c16_n36_n45, c16_p19_n11, EO03)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_p11_p41, c16_n45_n27, c16_p07_n30, c16_p43_p39, EO04)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n02_p39, c16_n36_n41, c16_p43_p07, c16_n11_p34, EO05)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n15_p36, c16_n11_n45, c16_p34_p39, c16_n45_n19, EO06)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n27_p34, c16_p19_n39, c16_n11_p43, c16_p02_n45, EO07)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n36_p30, c16_p41_n23, c16_n44_p15, c16_p45_n07, EO08)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n43_p27, c16_p44_n02, c16_n30_n23, c16_p07_p41, EO09)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n45_p23, c16_p27_p19, c16_p15_n45, c16_n44_p30, EO10)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n44_p19, c16_n02_p36, c16_p45_n34, c16_n15_n23, EO11)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n39_p15, c16_n30_p45, c16_p27_p02, c16_p41_n44, EO12)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n30_p11, c16_n45_p43, c16_n19_p36, c16_p23_n02, EO13)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n19_p07, c16_n39_p30, c16_n45_p44, c16_n36_p43, EO14)
                COMPUTE_ROW(T_00_08, T_00_09, T_00_10, T_00_11, c16_n07_p02, c16_n15_p11, c16_n23_p19, c16_n30_p27, EO15)

#undef COMPUTE_ROW

                {
                    // EEO[8] 
                    const __m256i EEO0 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_p43_p45), _mm256_madd_epi16(T_00_13, c16_p35_p40));
                    const __m256i EEO1 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_p29_p43), _mm256_madd_epi16(T_00_13, c16_n21_p04));
                    const __m256i EEO2 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_p04_p40), _mm256_madd_epi16(T_00_13, c16_n43_n35));
                    const __m256i EEO3 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_n21_p35), _mm256_madd_epi16(T_00_13, c16_p04_n43));
                    const __m256i EEO4 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_n40_p29), _mm256_madd_epi16(T_00_13, c16_p45_n13));
                    const __m256i EEO5 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_n45_p21), _mm256_madd_epi16(T_00_13, c16_p13_p29));
                    const __m256i EEO6 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_n35_p13), _mm256_madd_epi16(T_00_13, c16_n40_p45));
                    const __m256i EEO7 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, c16_n13_p04), _mm256_madd_epi16(T_00_13, c16_n29_p21));

                    // EEEO[4] 
                    const __m256i EEEO0 = _mm256_madd_epi16(T_00_14, c16_p38_p44);
                    const __m256i EEEO1 = _mm256_madd_epi16(T_00_14, c16_n09_p38);
                    const __m256i EEEO2 = _mm256_madd_epi16(T_00_14, c16_n44_p25);
                    const __m256i EEEO3 = _mm256_madd_epi16(T_00_14, c16_n25_p09);

                    const __m256i EEEE0 = _mm256_madd_epi16(T_00_15, c16_p42_p32);
                    const __m256i EEEE1 = _mm256_madd_epi16(T_00_15, c16_p17_p32);
                    const __m256i EEEE2 = _mm256_madd_epi16(T_00_15, c16_n17_p32);
                    const __m256i EEEE3 = _mm256_madd_epi16(T_00_15, c16_n42_p32);

                    const __m256i EEE0 = _mm256_add_epi32(EEEE0, EEEO0);          // EEE0 = EEEE0 + EEEO0
                    const __m256i EEE1 = _mm256_add_epi32(EEEE1, EEEO1);          // EEE1 = EEEE1 + EEEO1
                    const __m256i EEE2 = _mm256_add_epi32(EEEE2, EEEO2);          // EEE2 = EEEE2 + EEEO2
                    const __m256i EEE3 = _mm256_add_epi32(EEEE3, EEEO3);          // EEE3 = EEEE3 + EEEO3
                    const __m256i EEE7 = _mm256_sub_epi32(EEEE0, EEEO0);          // EEE7 = EEEE0 - EEEO0
                    const __m256i EEE6 = _mm256_sub_epi32(EEEE1, EEEO1);          // EEE6 = EEEE1 - EEEO1
                    const __m256i EEE5 = _mm256_sub_epi32(EEEE2, EEEO2);          // EEE7 = EEEE2 - EEEO2
                    const __m256i EEE4 = _mm256_sub_epi32(EEEE3, EEEO3);          // EEE6 = EEEE3 - EEEO3

                    const __m256i EE00 = _mm256_add_epi32(EEE0, EEO0);          // EE0 = EEE0 + EEO0
                    const __m256i EE01 = _mm256_add_epi32(EEE1, EEO1);          // EE1 = EEE1 + EEO1
                    const __m256i EE02 = _mm256_add_epi32(EEE2, EEO2);          // EE2 = EEE2 + EEO2
                    const __m256i EE03 = _mm256_add_epi32(EEE3, EEO3);          // EE3 = EEE3 + EEO3
                    const __m256i EE04 = _mm256_add_epi32(EEE4, EEO4);          // EE4 = EEE4 + EEO4
                    const __m256i EE05 = _mm256_add_epi32(EEE5, EEO5);          // EE5 = EEE5 + EEO5
                    const __m256i EE06 = _mm256_add_epi32(EEE6, EEO6);          // EE6 = EEE6 + EEO6
                    const __m256i EE07 = _mm256_add_epi32(EEE7, EEO7);          // EE7 = EEE7 + EEO7
                    const __m256i EE15 = _mm256_sub_epi32(EEE0, EEO0);          // EE15 = EEE0 - EEO0
                    const __m256i EE14 = _mm256_sub_epi32(EEE1, EEO1);
                    const __m256i EE13 = _mm256_sub_epi32(EEE2, EEO2);
                    const __m256i EE12 = _mm256_sub_epi32(EEE3, EEO3);
                    const __m256i EE11 = _mm256_sub_epi32(EEE4, EEO4);          // EE11 = EEE4 - EEO4
                    const __m256i EE10 = _mm256_sub_epi32(EEE5, EEO5);
                    const __m256i EE09 = _mm256_sub_epi32(EEE6, EEO6);
                    const __m256i EE08 = _mm256_sub_epi32(EEE7, EEO7);

                    const __m256i E00 = _mm256_add_epi32(EE00, EO00);          // E00 = EE00 + EO00
                    const __m256i E01 = _mm256_add_epi32(EE01, EO01);          // E01 = EE01 + EO01
                    const __m256i E02 = _mm256_add_epi32(EE02, EO02);          // E02 = EE02 + EO02
                    const __m256i E03 = _mm256_add_epi32(EE03, EO03);          // E03 = EE03 + EO03
                    const __m256i E04 = _mm256_add_epi32(EE04, EO04);
                    const __m256i E05 = _mm256_add_epi32(EE05, EO05);
                    const __m256i E06 = _mm256_add_epi32(EE06, EO06);
                    const __m256i E07 = _mm256_add_epi32(EE07, EO07);
                    const __m256i E08 = _mm256_add_epi32(EE08, EO08);          // E08 = EE08 + EO08
                    const __m256i E09 = _mm256_add_epi32(EE09, EO09);
                    const __m256i E10 = _mm256_add_epi32(EE10, EO10);
                    const __m256i E11 = _mm256_add_epi32(EE11, EO11);
                    const __m256i E12 = _mm256_add_epi32(EE12, EO12);
                    const __m256i E13 = _mm256_add_epi32(EE13, EO13);
                    const __m256i E14 = _mm256_add_epi32(EE14, EO14);
                    const __m256i E15 = _mm256_add_epi32(EE15, EO15);
                    const __m256i E31 = _mm256_sub_epi32(EE00, EO00);          // E31 = EE00 - EO00
                    const __m256i E30 = _mm256_sub_epi32(EE01, EO01);          // E30 = EE01 - EO01
                    const __m256i E29 = _mm256_sub_epi32(EE02, EO02);          // E29 = EE02 - EO02
                    const __m256i E28 = _mm256_sub_epi32(EE03, EO03);          // E28 = EE03 - EO03
                    const __m256i E27 = _mm256_sub_epi32(EE04, EO04);
                    const __m256i E26 = _mm256_sub_epi32(EE05, EO05);
                    const __m256i E25 = _mm256_sub_epi32(EE06, EO06);
                    const __m256i E24 = _mm256_sub_epi32(EE07, EO07);
                    const __m256i E23 = _mm256_sub_epi32(EE08, EO08);          // E23 = EE08 - EO08
                    const __m256i E22 = _mm256_sub_epi32(EE09, EO09);
                    const __m256i E21 = _mm256_sub_epi32(EE10, EO10);
                    const __m256i E20 = _mm256_sub_epi32(EE11, EO11);
                    const __m256i E19 = _mm256_sub_epi32(EE12, EO12);
                    const __m256i E18 = _mm256_sub_epi32(EE13, EO13);
                    const __m256i E17 = _mm256_sub_epi32(EE14, EO14);
                    const __m256i E16 = _mm256_sub_epi32(EE15, EO15);

                    const __m256i T1_00 = _mm256_add_epi32(E00, c32_off);         // E0 + off
                    const __m256i T1_01 = _mm256_add_epi32(E01, c32_off);         // E1 + off
                    const __m256i T1_02 = _mm256_add_epi32(E02, c32_off);         // E2 + off
                    const __m256i T1_03 = _mm256_add_epi32(E03, c32_off);         // E3 + off
                    const __m256i T1_04 = _mm256_add_epi32(E04, c32_off);         // E4 + off
                    const __m256i T1_05 = _mm256_add_epi32(E05, c32_off);         // E5 + off
                    const __m256i T1_06 = _mm256_add_epi32(E06, c32_off);         // E6 + off
                    const __m256i T1_07 = _mm256_add_epi32(E07, c32_off);         // E7 + off
                    const __m256i T1_08 = _mm256_add_epi32(E08, c32_off);         // E8 + off
                    const __m256i T1_09 = _mm256_add_epi32(E09, c32_off);         // E9 + off
                    const __m256i T1_10 = _mm256_add_epi32(E10, c32_off);         // E10 + off
                    const __m256i T1_11 = _mm256_add_epi32(E11, c32_off);         // E11 + off
                    const __m256i T1_12 = _mm256_add_epi32(E12, c32_off);         // E12 + off
                    const __m256i T1_13 = _mm256_add_epi32(E13, c32_off);         // E13 + off
                    const __m256i T1_14 = _mm256_add_epi32(E14, c32_off);         // E14 + off
                    const __m256i T1_15 = _mm256_add_epi32(E15, c32_off);         // E15 + off
                    const __m256i T1_16 = _mm256_add_epi32(E16, c32_off);
                    const __m256i T1_17 = _mm256_add_epi32(E17, c32_off);
                    const __m256i T1_18 = _mm256_add_epi32(E18, c32_off);
                    const __m256i T1_19 = _mm256_add_epi32(E19, c32_off);
                    const __m256i T1_20 = _mm256_add_epi32(E20, c32_off);
                    const __m256i T1_21 = _mm256_add_epi32(E21, c32_off);
                    const __m256i T1_22 = _mm256_add_epi32(E22, c32_off);
                    const __m256i T1_23 = _mm256_add_epi32(E23, c32_off);
                    const __m256i T1_24 = _mm256_add_epi32(E24, c32_off);
                    const __m256i T1_25 = _mm256_add_epi32(E25, c32_off);
                    const __m256i T1_26 = _mm256_add_epi32(E26, c32_off);
                    const __m256i T1_27 = _mm256_add_epi32(E27, c32_off);
                    const __m256i T1_28 = _mm256_add_epi32(E28, c32_off);
                    const __m256i T1_29 = _mm256_add_epi32(E29, c32_off);
                    const __m256i T1_30 = _mm256_add_epi32(E30, c32_off);
                    const __m256i T1_31 = _mm256_add_epi32(E31, c32_off);

                    __m256i T2_00 = _mm256_add_epi32(T1_00, O00);          // E0 + O0 + off
                    __m256i T2_01 = _mm256_add_epi32(T1_01, O01);
                    __m256i T2_02 = _mm256_add_epi32(T1_02, O02);          // E1 + O1 + off
                    __m256i T2_03 = _mm256_add_epi32(T1_03, O03);
                    __m256i T2_04 = _mm256_add_epi32(T1_04, O04);          // E2 + O2 + off
                    __m256i T2_05 = _mm256_add_epi32(T1_05, O05);
                    __m256i T2_06 = _mm256_add_epi32(T1_06, O06);          // E3 + O3 + off
                    __m256i T2_07 = _mm256_add_epi32(T1_07, O07);
                    __m256i T2_08 = _mm256_add_epi32(T1_08, O08);          // E4
                    __m256i T2_09 = _mm256_add_epi32(T1_09, O09);
                    __m256i T2_10 = _mm256_add_epi32(T1_10, O10);          // E5
                    __m256i T2_11 = _mm256_add_epi32(T1_11, O11);
                    __m256i T2_12 = _mm256_add_epi32(T1_12, O12);          // E6
                    __m256i T2_13 = _mm256_add_epi32(T1_13, O13);
                    __m256i T2_14 = _mm256_add_epi32(T1_14, O14);          // E7
                    __m256i T2_15 = _mm256_add_epi32(T1_15, O15);
                    __m256i T2_16 = _mm256_add_epi32(T1_16, O16);          // E8
                    __m256i T2_17 = _mm256_add_epi32(T1_17, O17);
                    __m256i T2_18 = _mm256_add_epi32(T1_18, O18);          // E9
                    __m256i T2_19 = _mm256_add_epi32(T1_19, O19);
                    __m256i T2_20 = _mm256_add_epi32(T1_20, O20);          // E10
                    __m256i T2_21 = _mm256_add_epi32(T1_21, O21);
                    __m256i T2_22 = _mm256_add_epi32(T1_22, O22);          // E11
                    __m256i T2_23 = _mm256_add_epi32(T1_23, O23);
                    __m256i T2_24 = _mm256_add_epi32(T1_24, O24);          // E12
                    __m256i T2_25 = _mm256_add_epi32(T1_25, O25);
                    __m256i T2_26 = _mm256_add_epi32(T1_26, O26);          // E13
                    __m256i T2_27 = _mm256_add_epi32(T1_27, O27);
                    __m256i T2_28 = _mm256_add_epi32(T1_28, O28);          // E14
                    __m256i T2_29 = _mm256_add_epi32(T1_29, O29);
                    __m256i T2_30 = _mm256_add_epi32(T1_30, O30);          // E15
                    __m256i T2_31 = _mm256_add_epi32(T1_31, O31);
                    __m256i T2_63 = _mm256_sub_epi32(T1_00, O00);          // E00 - O00 + off
                    __m256i T2_62 = _mm256_sub_epi32(T1_01, O01);
                    __m256i T2_61 = _mm256_sub_epi32(T1_02, O02);
                    __m256i T2_60 = _mm256_sub_epi32(T1_03, O03);
                    __m256i T2_59 = _mm256_sub_epi32(T1_04, O04);
                    __m256i T2_58 = _mm256_sub_epi32(T1_05, O05);
                    __m256i T2_57 = _mm256_sub_epi32(T1_06, O06);
                    __m256i T2_56 = _mm256_sub_epi32(T1_07, O07);
                    __m256i T2_55 = _mm256_sub_epi32(T1_08, O08);
                    __m256i T2_54 = _mm256_sub_epi32(T1_09, O09);
                    __m256i T2_53 = _mm256_sub_epi32(T1_10, O10);
                    __m256i T2_52 = _mm256_sub_epi32(T1_11, O11);
                    __m256i T2_51 = _mm256_sub_epi32(T1_12, O12);
                    __m256i T2_50 = _mm256_sub_epi32(T1_13, O13);
                    __m256i T2_49 = _mm256_sub_epi32(T1_14, O14);
                    __m256i T2_48 = _mm256_sub_epi32(T1_15, O15);
                    __m256i T2_47 = _mm256_sub_epi32(T1_16, O16);
                    __m256i T2_46 = _mm256_sub_epi32(T1_17, O17);
                    __m256i T2_45 = _mm256_sub_epi32(T1_18, O18);
                    __m256i T2_44 = _mm256_sub_epi32(T1_19, O19);
                    __m256i T2_43 = _mm256_sub_epi32(T1_20, O20);
                    __m256i T2_42 = _mm256_sub_epi32(T1_21, O21);
                    __m256i T2_41 = _mm256_sub_epi32(T1_22, O22);
                    __m256i T2_40 = _mm256_sub_epi32(T1_23, O23);
                    __m256i T2_39 = _mm256_sub_epi32(T1_24, O24);
                    __m256i T2_38 = _mm256_sub_epi32(T1_25, O25);
                    __m256i T2_37 = _mm256_sub_epi32(T1_26, O26);
                    __m256i T2_36 = _mm256_sub_epi32(T1_27, O27);
                    __m256i T2_35 = _mm256_sub_epi32(T1_28, O28);
                    __m256i T2_34 = _mm256_sub_epi32(T1_29, O29);
                    __m256i T2_33 = _mm256_sub_epi32(T1_30, O30);
                    __m256i T2_32 = _mm256_sub_epi32(T1_31, O31);

                    T2_00 = _mm256_srai_epi32(T2_00, shift);             // [30 20 10 00]
                    T2_01 = _mm256_srai_epi32(T2_01, shift);             // [70 60 50 40]
                    T2_02 = _mm256_srai_epi32(T2_02, shift);             // [31 21 11 01]
                    T2_03 = _mm256_srai_epi32(T2_03, shift);             // [71 61 51 41]
                    T2_04 = _mm256_srai_epi32(T2_04, shift);             // [32 22 12 02]
                    T2_05 = _mm256_srai_epi32(T2_05, shift);             // [72 62 52 42]
                    T2_06 = _mm256_srai_epi32(T2_06, shift);             // [33 23 13 03]
                    T2_07 = _mm256_srai_epi32(T2_07, shift);             // [73 63 53 43]
                    T2_08 = _mm256_srai_epi32(T2_08, shift);             // [33 24 14 04]
                    T2_09 = _mm256_srai_epi32(T2_09, shift);             // [74 64 54 44]
                    T2_10 = _mm256_srai_epi32(T2_10, shift);             // [35 25 15 05]
                    T2_11 = _mm256_srai_epi32(T2_11, shift);             // [75 65 55 45]
                    T2_12 = _mm256_srai_epi32(T2_12, shift);             // [36 26 16 06]
                    T2_13 = _mm256_srai_epi32(T2_13, shift);             // [76 66 56 46]
                    T2_14 = _mm256_srai_epi32(T2_14, shift);             // [37 27 17 07]
                    T2_15 = _mm256_srai_epi32(T2_15, shift);             // [77 67 57 47]
                    T2_16 = _mm256_srai_epi32(T2_16, shift);             // [30 20 10 00] x8
                    T2_17 = _mm256_srai_epi32(T2_17, shift);             // [70 60 50 40]
                    T2_18 = _mm256_srai_epi32(T2_18, shift);             // [31 21 11 01] x9
                    T2_19 = _mm256_srai_epi32(T2_19, shift);             // [71 61 51 41]
                    T2_20 = _mm256_srai_epi32(T2_20, shift);             // [32 22 12 02] xA
                    T2_21 = _mm256_srai_epi32(T2_21, shift);             // [72 62 52 42]
                    T2_22 = _mm256_srai_epi32(T2_22, shift);             // [33 23 13 03] xB
                    T2_23 = _mm256_srai_epi32(T2_23, shift);             // [73 63 53 43]
                    T2_24 = _mm256_srai_epi32(T2_24, shift);             // [33 24 14 04] xC
                    T2_25 = _mm256_srai_epi32(T2_25, shift);             // [74 64 54 44]
                    T2_26 = _mm256_srai_epi32(T2_26, shift);             // [35 25 15 05] xD
                    T2_27 = _mm256_srai_epi32(T2_27, shift);             // [75 65 55 45]
                    T2_28 = _mm256_srai_epi32(T2_28, shift);             // [36 26 16 06] xE
                    T2_29 = _mm256_srai_epi32(T2_29, shift);             // [76 66 56 46]
                    T2_30 = _mm256_srai_epi32(T2_30, shift);             // [37 27 17 07] xF
                    T2_31 = _mm256_srai_epi32(T2_31, shift);             // [77 67 57 47]
                    T2_63 = _mm256_srai_epi32(T2_63, shift);
                    T2_62 = _mm256_srai_epi32(T2_62, shift);
                    T2_61 = _mm256_srai_epi32(T2_61, shift);
                    T2_60 = _mm256_srai_epi32(T2_60, shift);
                    T2_59 = _mm256_srai_epi32(T2_59, shift);
                    T2_58 = _mm256_srai_epi32(T2_58, shift);
                    T2_57 = _mm256_srai_epi32(T2_57, shift);
                    T2_56 = _mm256_srai_epi32(T2_56, shift);
                    T2_55 = _mm256_srai_epi32(T2_55, shift);
                    T2_54 = _mm256_srai_epi32(T2_54, shift);
                    T2_53 = _mm256_srai_epi32(T2_53, shift);
                    T2_52 = _mm256_srai_epi32(T2_52, shift);
                    T2_51 = _mm256_srai_epi32(T2_51, shift);
                    T2_50 = _mm256_srai_epi32(T2_50, shift);
                    T2_49 = _mm256_srai_epi32(T2_49, shift);
                    T2_48 = _mm256_srai_epi32(T2_48, shift);
                    T2_47 = _mm256_srai_epi32(T2_47, shift);
                    T2_46 = _mm256_srai_epi32(T2_46, shift);
                    T2_45 = _mm256_srai_epi32(T2_45, shift);
                    T2_44 = _mm256_srai_epi32(T2_44, shift);
                    T2_43 = _mm256_srai_epi32(T2_43, shift);
                    T2_42 = _mm256_srai_epi32(T2_42, shift);
                    T2_41 = _mm256_srai_epi32(T2_41, shift);
                    T2_40 = _mm256_srai_epi32(T2_40, shift);
                    T2_39 = _mm256_srai_epi32(T2_39, shift);
                    T2_38 = _mm256_srai_epi32(T2_38, shift);
                    T2_37 = _mm256_srai_epi32(T2_37, shift);
                    T2_36 = _mm256_srai_epi32(T2_36, shift);
                    T2_35 = _mm256_srai_epi32(T2_35, shift);
                    T2_34 = _mm256_srai_epi32(T2_34, shift);
                    T2_33 = _mm256_srai_epi32(T2_33, shift);
                    T2_32 = _mm256_srai_epi32(T2_32, shift);

                    //transpose matrix H x W: 64x8 --> 8x64
                    TRANSPOSE_16x8_32BIT_16BIT(T2_00, T2_01, T2_02, T2_03, T2_04, T2_05, T2_06, T2_07, T2_08, T2_09, T2_10, T2_11, T2_12, T2_13, T2_14, T2_15, res00, res04, res08, res12, res16, res20, res24, res28);
                    TRANSPOSE_16x8_32BIT_16BIT(T2_16, T2_17, T2_18, T2_19, T2_20, T2_21, T2_22, T2_23, T2_24, T2_25, T2_26, T2_27, T2_28, T2_29, T2_30, T2_31, res01, res05, res09, res13, res17, res21, res25, res29);
                    TRANSPOSE_16x8_32BIT_16BIT(T2_32, T2_33, T2_34, T2_35, T2_36, T2_37, T2_38, T2_39, T2_40, T2_41, T2_42, T2_43, T2_44, T2_45, T2_46, T2_47, res02, res06, res10, res14, res18, res22, res26, res30);
                    TRANSPOSE_16x8_32BIT_16BIT(T2_48, T2_49, T2_50, T2_51, T2_52, T2_53, T2_54, T2_55, T2_56, T2_57, T2_58, T2_59, T2_60, T2_61, T2_62, T2_63, res03, res07, res11, res15, res19, res23, res27, res31);

                }
                if (bit_depth != MAX_TX_DYNAMIC_RANGE) {
                    __m256i max_val = _mm256_set1_epi16((1 << bit_depth) - 1);
                    __m256i min_val = _mm256_set1_epi16(-(1 << bit_depth));
                    //the first and second row
                    res00 = _mm256_min_epi16(res00, max_val);
                    res01 = _mm256_min_epi16(res01, max_val);
                    res02 = _mm256_min_epi16(res02, max_val);
                    res03 = _mm256_min_epi16(res03, max_val);
                    res04 = _mm256_min_epi16(res04, max_val);
                    res05 = _mm256_min_epi16(res05, max_val);
                    res06 = _mm256_min_epi16(res06, max_val);
                    res07 = _mm256_min_epi16(res07, max_val);
                    res00 = _mm256_max_epi16(res00, min_val);
                    res01 = _mm256_max_epi16(res01, min_val);
                    res02 = _mm256_max_epi16(res02, min_val);
                    res03 = _mm256_max_epi16(res03, min_val);
                    res04 = _mm256_max_epi16(res04, min_val);
                    res05 = _mm256_max_epi16(res05, min_val);
                    res06 = _mm256_max_epi16(res06, min_val);
                    res07 = _mm256_max_epi16(res07, min_val);
                    _mm256_store_si256((__m256i*)&dst[0 * 16], res00);
                    _mm256_store_si256((__m256i*)&dst[1 * 16], res01);
                    _mm256_store_si256((__m256i*)&dst[2 * 16], res02);
                    _mm256_store_si256((__m256i*)&dst[3 * 16], res03);
                    _mm256_store_si256((__m256i*)&dst[4 * 16], res04);
                    _mm256_store_si256((__m256i*)&dst[5 * 16], res05);
                    _mm256_store_si256((__m256i*)&dst[6 * 16], res06);
                    _mm256_store_si256((__m256i*)&dst[7 * 16], res07);

                    dst += 8*16;
                    //the third and fourth row
                    res08 = _mm256_min_epi16(res08, max_val);
                    res09 = _mm256_min_epi16(res09, max_val);
                    res10 = _mm256_min_epi16(res10, max_val);
                    res11 = _mm256_min_epi16(res11, max_val);
                    res12 = _mm256_min_epi16(res12, max_val);
                    res13 = _mm256_min_epi16(res13, max_val);
                    res14 = _mm256_min_epi16(res14, max_val);
                    res15 = _mm256_min_epi16(res15, max_val);
                    res08 = _mm256_max_epi16(res08, min_val);
                    res09 = _mm256_max_epi16(res09, min_val);
                    res10 = _mm256_max_epi16(res10, min_val);
                    res11 = _mm256_max_epi16(res11, min_val);
                    res12 = _mm256_max_epi16(res12, min_val);
                    res13 = _mm256_max_epi16(res13, min_val);
                    res14 = _mm256_max_epi16(res14, min_val);
                    res15 = _mm256_max_epi16(res15, min_val);
                    _mm256_store_si256((__m256i*)&dst[0 * 16], res08);
                    _mm256_store_si256((__m256i*)&dst[1 * 16], res09);
                    _mm256_store_si256((__m256i*)&dst[2 * 16], res10);
                    _mm256_store_si256((__m256i*)&dst[3 * 16], res11);
                    _mm256_store_si256((__m256i*)&dst[4 * 16], res12);
                    _mm256_store_si256((__m256i*)&dst[5 * 16], res13);
                    _mm256_store_si256((__m256i*)&dst[6 * 16], res14);
                    _mm256_store_si256((__m256i*)&dst[7 * 16], res15);

                    dst += 8*16;
                    //the fifth and sixth row
                    res16 = _mm256_min_epi16(res16, max_val);
                    res17 = _mm256_min_epi16(res17, max_val);
                    res18 = _mm256_min_epi16(res18, max_val);
                    res19 = _mm256_min_epi16(res19, max_val);
                    res20 = _mm256_min_epi16(res20, max_val);
                    res21 = _mm256_min_epi16(res21, max_val);
                    res22 = _mm256_min_epi16(res22, max_val);
                    res23 = _mm256_min_epi16(res23, max_val);
                    res16 = _mm256_max_epi16(res16, min_val);
                    res17 = _mm256_max_epi16(res17, min_val);
                    res18 = _mm256_max_epi16(res18, min_val);
                    res19 = _mm256_max_epi16(res19, min_val);
                    res20 = _mm256_max_epi16(res20, min_val);
                    res21 = _mm256_max_epi16(res21, min_val);
                    res22 = _mm256_max_epi16(res22, min_val);
                    res23 = _mm256_max_epi16(res23, min_val);
                    _mm256_store_si256((__m256i*)&dst[0 * 16], res16);
                    _mm256_store_si256((__m256i*)&dst[1 * 16], res17);
                    _mm256_store_si256((__m256i*)&dst[2 * 16], res18);
                    _mm256_store_si256((__m256i*)&dst[3 * 16], res19);
                    _mm256_store_si256((__m256i*)&dst[4 * 16], res20);
                    _mm256_store_si256((__m256i*)&dst[5 * 16], res21);
                    _mm256_store_si256((__m256i*)&dst[6 * 16], res22);
                    _mm256_store_si256((__m256i*)&dst[7 * 16], res23);

                    dst += 8*16;
                    //the seventh and eighth row
                    res24 = _mm256_min_epi16(res24, max_val);
                    res25 = _mm256_min_epi16(res25, max_val);
                    res26 = _mm256_min_epi16(res26, max_val);
                    res27 = _mm256_min_epi16(res27, max_val);
                    res28 = _mm256_min_epi16(res28, max_val);
                    res29 = _mm256_min_epi16(res29, max_val);
                    res30 = _mm256_min_epi16(res30, max_val);
                    res31 = _mm256_min_epi16(res31, max_val);
                    res24 = _mm256_max_epi16(res24, min_val);
                    res25 = _mm256_max_epi16(res25, min_val);
                    res26 = _mm256_max_epi16(res26, min_val);
                    res27 = _mm256_max_epi16(res27, min_val);
                    res28 = _mm256_max_epi16(res28, min_val);
                    res29 = _mm256_max_epi16(res29, min_val);
                    res30 = _mm256_max_epi16(res30, min_val);
                    res31 = _mm256_max_epi16(res31, min_val);
                    _mm256_store_si256((__m256i*)&dst[0 * 16], res24);
                    _mm256_store_si256((__m256i*)&dst[1 * 16], res25);
                    _mm256_store_si256((__m256i*)&dst[2 * 16], res26);
                    _mm256_store_si256((__m256i*)&dst[3 * 16], res27);
                    _mm256_store_si256((__m256i*)&dst[4 * 16], res28);
                    _mm256_store_si256((__m256i*)&dst[5 * 16], res29);
                    _mm256_store_si256((__m256i*)&dst[6 * 16], res30);
                    _mm256_store_si256((__m256i*)&dst[7 * 16], res31);

                    dst += 8*16;
                }
                else {
                    _mm256_store_si256((__m256i*)&dst[0 * 16], res00);
                    _mm256_store_si256((__m256i*)&dst[1 * 16], res01);
                    _mm256_store_si256((__m256i*)&dst[2 * 16], res02);
                    _mm256_store_si256((__m256i*)&dst[3 * 16], res03);
                    _mm256_store_si256((__m256i*)&dst[4 * 16], res04);
                    _mm256_store_si256((__m256i*)&dst[5 * 16], res05);
                    _mm256_store_si256((__m256i*)&dst[6 * 16], res06);
                    _mm256_store_si256((__m256i*)&dst[7 * 16], res07);

                    dst += 8 * 16;

                    _mm256_store_si256((__m256i*)&dst[0 * 16], res08);
                    _mm256_store_si256((__m256i*)&dst[1 * 16], res09);
                    _mm256_store_si256((__m256i*)&dst[2 * 16], res10);
                    _mm256_store_si256((__m256i*)&dst[3 * 16], res11);
                    _mm256_store_si256((__m256i*)&dst[4 * 16], res12);
                    _mm256_store_si256((__m256i*)&dst[5 * 16], res13);
                    _mm256_store_si256((__m256i*)&dst[6 * 16], res14);
                    _mm256_store_si256((__m256i*)&dst[7 * 16], res15);

                    dst += 8 * 16;

                    _mm256_store_si256((__m256i*)&dst[0 * 16], res16);
                    _mm256_store_si256((__m256i*)&dst[1 * 16], res17);
                    _mm256_store_si256((__m256i*)&dst[2 * 16], res18);
                    _mm256_store_si256((__m256i*)&dst[3 * 16], res19);
                    _mm256_store_si256((__m256i*)&dst[4 * 16], res20);
                    _mm256_store_si256((__m256i*)&dst[5 * 16], res21);
                    _mm256_store_si256((__m256i*)&dst[6 * 16], res22);
                    _mm256_store_si256((__m256i*)&dst[7 * 16], res23);

                    dst += 8 * 16;

                    _mm256_store_si256((__m256i*)&dst[0 * 16], res24);
                    _mm256_store_si256((__m256i*)&dst[1 * 16], res25);
                    _mm256_store_si256((__m256i*)&dst[2 * 16], res26);
                    _mm256_store_si256((__m256i*)&dst[3 * 16], res27);
                    _mm256_store_si256((__m256i*)&dst[4 * 16], res28);
                    _mm256_store_si256((__m256i*)&dst[5 * 16], res29);
                    _mm256_store_si256((__m256i*)&dst[6 * 16], res30);
                    _mm256_store_si256((__m256i*)&dst[7 * 16], res31);

                    dst += 8 * 16;
                }
            }
        }
    }

}

void itrans_dct2_h4_w8_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[4 * 8]);
    dct2_butterfly_h4_avx2(src, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h8_sse(tmp, 4, dst, 4, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h4_w16_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[4 * 16]);
    dct2_butterfly_h4_avx2(src, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h16_sse(tmp, 4, dst, 4, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h4_w32_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[4 * 32]);
    dct2_butterfly_h4_avx2(src, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h32_sse(tmp, 4, dst, 4, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h8_w4_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[8 * 4]);
    dct2_butterfly_h8_sse(src, 4, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h4_avx2(tmp, dst, 8, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h8_w8_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[8 * 8]);
    dct2_butterfly_h8_avx2(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h8_avx2(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h8_w16_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[8 * 16]);
    dct2_butterfly_h8_avx2(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h16_avx2(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h8_w32_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[8 * 32]);
    dct2_butterfly_h8_avx2(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h32_avx2(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h8_w64_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[8 * 64]);
    dct2_butterfly_h8_avx2(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h64_avx2(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h16_w4_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[16 * 4]);
    dct2_butterfly_h16_sse(src, 4, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h4_avx2(tmp, dst, 16, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h16_w8_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[16 * 8]);
    dct2_butterfly_h16_avx2(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h8_avx2(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h16_w16_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[16 * 16]);
    dct2_butterfly_h16_avx2(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h16_avx2(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h16_w32_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[16 * 32]);
    dct2_butterfly_h16_avx2(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h32_avx2(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h16_w64_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[16 * 64]);
    dct2_butterfly_h16_avx2(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h64_avx2(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h32_w4_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[32 * 4]);
    dct2_butterfly_h32_sse(src, 4, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h4_avx2(tmp, dst, 32, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h32_w8_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[32 * 8]);
    dct2_butterfly_h32_avx2(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h8_avx2(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h32_w16_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[32 * 16]);
    dct2_butterfly_h32_avx2(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h16_avx2(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h32_w32_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[32 * 32]);
    dct2_butterfly_h32_avx2(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h32_avx2(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h32_w64_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[32 * 64]);
    dct2_butterfly_h32_avx2(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h64_avx2(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h64_w8_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[64 * 8]);
    dct2_butterfly_h64_avx2(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h8_avx2(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h64_w16_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[64 * 16]);
    dct2_butterfly_h64_avx2(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h16_avx2(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h64_w32_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[64 * 32]);
    dct2_butterfly_h64_avx2(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h32_avx2(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h64_w64_avx2(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_32(s16 tmp[64 * 64]);
    dct2_butterfly_h64_avx2(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h64_avx2(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}

void itrans_dct8_pb4_avx2(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i = 0;
    __m256i factor = _mm256_set1_epi32(1 << (shift - 1));
    __m256i s0, s1, s2, s3;	//源数据
    __m256i c0, c1, c2, c3;	//系数矩阵
    __m256i zero = _mm256_setzero_si256();
    __m256i e0, e1, e2, e3;
    __m256i v0, v1, v2, v3;
    __m256i r0;
    __m256i mask = _mm256_set_epi64x(0, 0, -1, -1);

    for (i; i < line; i++)
    {
        //load coef data
        //s8->s16
        c0 = _mm256_loadu_si256((__m256i*)(iT));
        c0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(c0));
        c1 = _mm256_loadu_si256((__m256i*)(iT + 4));
        c1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(c1));
        c2 = _mm256_loadu_si256((__m256i*)(iT + 8));
        c2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(c2));
        c3 = _mm256_loadu_si256((__m256i*)(iT + 12));
        c3 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(c3));
        c0 = _mm256_unpacklo_epi16(c0, zero);
        c1 = _mm256_unpacklo_epi16(c1, zero);
        c2 = _mm256_unpacklo_epi16(c2, zero);
        c3 = _mm256_unpacklo_epi16(c3, zero);

        //load src
        s0 = _mm256_set1_epi16(coeff[0]);
        s1 = _mm256_set1_epi16(coeff[line]);
        s2 = _mm256_set1_epi16(coeff[2 * line]);
        s3 = _mm256_set1_epi16(coeff[3 * line]);

        v0 = _mm256_madd_epi16(s0, c0);
        v1 = _mm256_madd_epi16(s1, c1);
        v2 = _mm256_madd_epi16(s2, c2);
        v3 = _mm256_madd_epi16(s3, c3);

        e0 = _mm256_add_epi32(v0, v1);
        e1 = _mm256_add_epi32(v2, v3);
        e2 = _mm256_add_epi32(e0, e1);
        e3 = _mm256_add_epi32(e2, factor);

        r0 = _mm256_packs_epi32(_mm256_srai_epi32(e3, shift), zero);
        _mm256_maskstore_epi64((long long*)block, mask, r0);
        coeff++;
        block += 4;
    }
}

void itrans_dct8_pb8_avx2(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i;
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;
    __m256i e0, e1, e2, e3, e4, e5, e6;
    __m256i r0, r1, r2;
    __m256i zero = _mm256_setzero_si256();
    __m256i rnd_factor = _mm256_set1_epi32(1 << (shift - 1));
    __m256i mask = _mm256_set_epi64x(0, 0, -1, -1);

    for (i = 0; i < line; i++)
    {
        s0 = _mm256_set1_epi32(coeff[0]);
        s1 = _mm256_set1_epi32(coeff[line]);
        s2 = _mm256_set1_epi32(coeff[2 * line]);
        s3 = _mm256_set1_epi32(coeff[3 * line]);
        s4 = _mm256_set1_epi32(coeff[4 * line]);
        s5 = _mm256_set1_epi32(coeff[5 * line]);
        s6 = _mm256_set1_epi32(coeff[6 * line]);
        s7 = _mm256_set1_epi32(coeff[7 * line]);

        c0 = _mm256_loadu_si256((__m256i*)iT);
        c0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c0));
        c1 = _mm256_loadu_si256((__m256i*)(iT + 8));
        c1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c1));
        c2 = _mm256_loadu_si256((__m256i*)(iT + 16));
        c2 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c2));
        c3 = _mm256_loadu_si256((__m256i*)(iT + 24));
        c3 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c3));
        c4 = _mm256_loadu_si256((__m256i*)(iT + 32));
        c4 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c4));
        c5 = _mm256_loadu_si256((__m256i*)(iT + 40));
        c5 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c5));
        c6 = _mm256_loadu_si256((__m256i*)(iT + 48));
        c6 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c6));
        c7 = _mm256_loadu_si256((__m256i*)(iT + 56));
        c7 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c7));

        e0 = _mm256_add_epi32(_mm256_mullo_epi32(s0, c0), _mm256_mullo_epi32(s1, c1));
        e1 = _mm256_add_epi32(_mm256_mullo_epi32(s2, c2), _mm256_mullo_epi32(s3, c3));
        e2 = _mm256_add_epi32(_mm256_mullo_epi32(s4, c4), _mm256_mullo_epi32(s5, c5));
        e3 = _mm256_add_epi32(_mm256_mullo_epi32(s6, c6), _mm256_mullo_epi32(s7, c7));
        e4 = _mm256_add_epi32(e0, e1);
        e5 = _mm256_add_epi32(e2, e3);
        e6 = _mm256_add_epi32(e4, e5);

        r0 = _mm256_add_epi32(e6, rnd_factor);
        r1 = _mm256_packs_epi32(_mm256_srai_epi32(r0, shift), zero);
        r2 = _mm256_permute4x64_epi64(r1, 0x08);
        _mm256_maskstore_epi64((long long*)block, mask, r2);

        coeff++;
        block += 8;
    }
}

void itrans_dct8_pb16_avx2(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;
    __m256i e0, e1, e2, e3, e4, e5, e6;
    __m256i r0, r1, r2;
    __m256i mask = _mm256_set_epi64x(0, 0, -1, -1);
    __m256i zero = _mm256_setzero_si256();
    __m256i rnd_factor = _mm256_set1_epi32(1 << (shift - 1));
    for (i = 0; i < line; i++)
    {
        s8 *pb16 = iT;
        for (j = 0; j < 2; j++)
        {
            __m256i v0 = zero;
            for (k = 0; k < 2; k++)
            {
                s0 = _mm256_set1_epi32(coeff[0]);
                s1 = _mm256_set1_epi32(coeff[line]);
                s2 = _mm256_set1_epi32(coeff[2 * line]);
                s3 = _mm256_set1_epi32(coeff[3 * line]);
                s4 = _mm256_set1_epi32(coeff[4 * line]);
                s5 = _mm256_set1_epi32(coeff[5 * line]);
                s6 = _mm256_set1_epi32(coeff[6 * line]);
                s7 = _mm256_set1_epi32(coeff[7 * line]);

                c0 = _mm256_loadu_si256((__m256i*)pb16);
                c0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c0));
                c1 = _mm256_loadu_si256((__m256i*)(pb16 + 16));
                c1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c1));
                c2 = _mm256_loadu_si256((__m256i*)(pb16 + 32));
                c2 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c2));
                c3 = _mm256_loadu_si256((__m256i*)(pb16 + 48));
                c3 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c3));
                c4 = _mm256_loadu_si256((__m256i*)(pb16 + 64));
                c4 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c4));
                c5 = _mm256_loadu_si256((__m256i*)(pb16 + 80));
                c5 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c5));
                c6 = _mm256_loadu_si256((__m256i*)(pb16 + 96));
                c6 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c6));
                c7 = _mm256_loadu_si256((__m256i*)(pb16 + 112));
                c7 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c7));

                e0 = _mm256_add_epi32(_mm256_mullo_epi32(s0, c0), _mm256_mullo_epi32(s1, c1));
                e1 = _mm256_add_epi32(_mm256_mullo_epi32(s2, c2), _mm256_mullo_epi32(s3, c3));
                e2 = _mm256_add_epi32(_mm256_mullo_epi32(s4, c4), _mm256_mullo_epi32(s5, c5));
                e3 = _mm256_add_epi32(_mm256_mullo_epi32(s6, c6), _mm256_mullo_epi32(s7, c7));
                e4 = _mm256_add_epi32(e0, e1);
                e5 = _mm256_add_epi32(e2, e3);
                e6 = _mm256_add_epi32(e4, e5);

                v0 = _mm256_add_epi32(v0, e6);

                pb16 += 128;
                coeff += (8 * line);
            }
            r0 = _mm256_add_epi32(v0, rnd_factor);
            r1 = _mm256_packs_epi32(_mm256_srai_epi32(r0, shift), zero);
            r2 = _mm256_permute4x64_epi64(r1, 0x08);
            _mm256_maskstore_epi64((long long*)block, mask, r2);
            coeff -= (16 * line);
            pb16 -= 248;
            block += 8;
        }
        coeff++;
    }
}

void itrans_dct8_pb32_avx2(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;
    __m256i e0, e1, e2, e3, e4, e5, e6;
    __m256i r0, r1, r2;
    __m256i mask = _mm256_set_epi64x(0, 0, -1, -1);
    __m256i zero = _mm256_setzero_si256();
    __m256i rnd_factor = _mm256_set1_epi32(1 << (shift - 1));

    for (i = 0; i < line; i++)
    {
        s8 *pb16 = iT;
        for (j = 0; j < 4; j++)
        {
            __m256i v0 = zero;
            for (k = 0; k < 4; k++)
            {
                s0 = _mm256_set1_epi32(coeff[0]);
                s1 = _mm256_set1_epi32(coeff[line]);
                s2 = _mm256_set1_epi32(coeff[2 * line]);
                s3 = _mm256_set1_epi32(coeff[3 * line]);
                s4 = _mm256_set1_epi32(coeff[4 * line]);
                s5 = _mm256_set1_epi32(coeff[5 * line]);
                s6 = _mm256_set1_epi32(coeff[6 * line]);
                s7 = _mm256_set1_epi32(coeff[7 * line]);

                c0 = _mm256_load_si256((__m256i*)pb16);
                c0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c0));
                c1 = _mm256_load_si256((__m256i*)(pb16 + 32));
                c1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c1));
                c2 = _mm256_load_si256((__m256i*)(pb16 + 64));
                c2 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c2));
                c3 = _mm256_load_si256((__m256i*)(pb16 + 96));
                c3 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c3));
                c4 = _mm256_load_si256((__m256i*)(pb16 + 128));
                c4 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c4));
                c5 = _mm256_load_si256((__m256i*)(pb16 + 160));
                c5 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c5));
                c6 = _mm256_load_si256((__m256i*)(pb16 + 192));
                c6 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c6));
                c7 = _mm256_load_si256((__m256i*)(pb16 + 224));
                c7 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c7));

                e0 = _mm256_add_epi32(_mm256_mullo_epi32(s0, c0), _mm256_mullo_epi32(s1, c1));
                e1 = _mm256_add_epi32(_mm256_mullo_epi32(s2, c2), _mm256_mullo_epi32(s3, c3));
                e2 = _mm256_add_epi32(_mm256_mullo_epi32(s4, c4), _mm256_mullo_epi32(s5, c5));
                e3 = _mm256_add_epi32(_mm256_mullo_epi32(s6, c6), _mm256_mullo_epi32(s7, c7));
                e4 = _mm256_add_epi32(e0, e1);
                e5 = _mm256_add_epi32(e2, e3);
                e6 = _mm256_add_epi32(e4, e5);

                v0 = _mm256_add_epi32(v0, e6);

                pb16 += 256;
                coeff += (32 * line);
            }
            r0 = _mm256_add_epi32(v0, rnd_factor);
            r1 = _mm256_packs_epi32(_mm256_srai_epi32(r0, shift), zero);
            r2 = _mm256_permute4x64_epi64(r1, 0x08);
            _mm256_maskstore_epi64((long long*)block, mask, r2);
            coeff -= (32 * line);
            pb16 -= 1016;
            block += 8;
        }
        coeff++;
    }
}

void itrans_dst7_pb4_avx2(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i = 0;
    __m256i factor = _mm256_set1_epi32(1 << (shift - 1));
    __m256i s0, s1, s2, s3;	//源数据
    __m256i c0, c1, c2, c3;	//系数矩阵
    __m256i zero = _mm256_setzero_si256();
    __m256i e0, e1, e2, e3;
    __m256i v0, v1, v2, v3;
    __m256i r0;
    __m256i mask = _mm256_set_epi64x(0, 0, -1, -1);

    for (i; i < line; i++)
    {
        //load coef data
        //s8->s16
        c0 = _mm256_loadu_si256((__m256i*)(iT));
        c0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(c0));
        c1 = _mm256_loadu_si256((__m256i*)(iT + 4));
        c1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(c1));
        c2 = _mm256_loadu_si256((__m256i*)(iT + 8));
        c2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(c2));
        c3 = _mm256_loadu_si256((__m256i*)(iT + 12));
        c3 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(c3));
        c0 = _mm256_unpacklo_epi16(c0, zero);
        c1 = _mm256_unpacklo_epi16(c1, zero);
        c2 = _mm256_unpacklo_epi16(c2, zero);
        c3 = _mm256_unpacklo_epi16(c3, zero);

        //load src
        s0 = _mm256_set1_epi16(coeff[0]);
        s1 = _mm256_set1_epi16(coeff[line]);
        s2 = _mm256_set1_epi16(coeff[2 * line]);
        s3 = _mm256_set1_epi16(coeff[3 * line]);

        v0 = _mm256_madd_epi16(s0, c0);
        v1 = _mm256_madd_epi16(s1, c1);
        v2 = _mm256_madd_epi16(s2, c2);
        v3 = _mm256_madd_epi16(s3, c3);

        e0 = _mm256_add_epi32(v0, v1);
        e1 = _mm256_add_epi32(v2, v3);
        e2 = _mm256_add_epi32(e0, e1);
        e3 = _mm256_add_epi32(e2, factor);

        r0 = _mm256_packs_epi32(_mm256_srai_epi32(e3, shift), zero);
        _mm256_maskstore_epi64((long long*)block, mask, r0);
        coeff++;
        block += 4;
    }
}

void itrans_dst7_pb8_avx2(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i;
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;
    __m256i e0, e1, e2, e3, e4, e5, e6;
    __m256i r0, r1, r2;
    __m256i zero = _mm256_setzero_si256();
    __m256i rnd_factor = _mm256_set1_epi32(1 << (shift - 1));
    __m256i mask = _mm256_set_epi64x(0, 0, -1, -1);

    for (i = 0; i < line; i++)
    {
        s0 = _mm256_set1_epi32(coeff[0]);
        s1 = _mm256_set1_epi32(coeff[line]);
        s2 = _mm256_set1_epi32(coeff[2 * line]);
        s3 = _mm256_set1_epi32(coeff[3 * line]);
        s4 = _mm256_set1_epi32(coeff[4 * line]);
        s5 = _mm256_set1_epi32(coeff[5 * line]);
        s6 = _mm256_set1_epi32(coeff[6 * line]);
        s7 = _mm256_set1_epi32(coeff[7 * line]);

        c0 = _mm256_loadu_si256((__m256i*)iT);
        c0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c0));
        c1 = _mm256_loadu_si256((__m256i*)(iT + 8));
        c1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c1));
        c2 = _mm256_loadu_si256((__m256i*)(iT + 16));
        c2 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c2));
        c3 = _mm256_loadu_si256((__m256i*)(iT + 24));
        c3 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c3));
        c4 = _mm256_loadu_si256((__m256i*)(iT + 32));
        c4 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c4));
        c5 = _mm256_loadu_si256((__m256i*)(iT + 40));
        c5 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c5));
        c6 = _mm256_loadu_si256((__m256i*)(iT + 48));
        c6 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c6));
        c7 = _mm256_loadu_si256((__m256i*)(iT + 56));
        c7 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c7));

        e0 = _mm256_add_epi32(_mm256_mullo_epi32(s0, c0), _mm256_mullo_epi32(s1, c1));
        e1 = _mm256_add_epi32(_mm256_mullo_epi32(s2, c2), _mm256_mullo_epi32(s3, c3));
        e2 = _mm256_add_epi32(_mm256_mullo_epi32(s4, c4), _mm256_mullo_epi32(s5, c5));
        e3 = _mm256_add_epi32(_mm256_mullo_epi32(s6, c6), _mm256_mullo_epi32(s7, c7));
        e4 = _mm256_add_epi32(e0, e1);
        e5 = _mm256_add_epi32(e2, e3);
        e6 = _mm256_add_epi32(e4, e5);

        r0 = _mm256_add_epi32(e6, rnd_factor);
        r1 = _mm256_packs_epi32(_mm256_srai_epi32(r0, shift), zero);
        r2 = _mm256_permute4x64_epi64(r1, 0x08);
        _mm256_maskstore_epi64((long long*)block, mask, r2);

        coeff++;
        block += 8;
    }
}

void itrans_dst7_pb16_avx2(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;
    __m256i e0, e1, e2, e3, e4, e5, e6;
    __m256i r0, r1, r2;
    __m256i mask = _mm256_set_epi64x(0, 0, -1, -1);
    __m256i zero = _mm256_setzero_si256();
    __m256i rnd_factor = _mm256_set1_epi32(1 << (shift - 1));

    for (i = 0; i < line; i++)
    {
        s8 *pb16 = iT;
        for (j = 0; j < 2; j++)
        {
            __m256i v0 = zero;
            for (k = 0; k < 2; k++)
            {
                s0 = _mm256_set1_epi32(coeff[0]);
                s1 = _mm256_set1_epi32(coeff[line]);
                s2 = _mm256_set1_epi32(coeff[2 * line]);
                s3 = _mm256_set1_epi32(coeff[3 * line]);
                s4 = _mm256_set1_epi32(coeff[4 * line]);
                s5 = _mm256_set1_epi32(coeff[5 * line]);
                s6 = _mm256_set1_epi32(coeff[6 * line]);
                s7 = _mm256_set1_epi32(coeff[7 * line]);

                c0 = _mm256_loadu_si256((__m256i*)pb16);
                c0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c0));
                c1 = _mm256_loadu_si256((__m256i*)(pb16 + 16));
                c1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c1));
                c2 = _mm256_loadu_si256((__m256i*)(pb16 + 32));
                c2 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c2));
                c3 = _mm256_loadu_si256((__m256i*)(pb16 + 48));
                c3 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c3));
                c4 = _mm256_loadu_si256((__m256i*)(pb16 + 64));
                c4 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c4));
                c5 = _mm256_loadu_si256((__m256i*)(pb16 + 80));
                c5 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c5));
                c6 = _mm256_loadu_si256((__m256i*)(pb16 + 96));
                c6 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c6));
                c7 = _mm256_loadu_si256((__m256i*)(pb16 + 112));
                c7 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c7));

                e0 = _mm256_add_epi32(_mm256_mullo_epi32(s0, c0), _mm256_mullo_epi32(s1, c1));
                e1 = _mm256_add_epi32(_mm256_mullo_epi32(s2, c2), _mm256_mullo_epi32(s3, c3));
                e2 = _mm256_add_epi32(_mm256_mullo_epi32(s4, c4), _mm256_mullo_epi32(s5, c5));
                e3 = _mm256_add_epi32(_mm256_mullo_epi32(s6, c6), _mm256_mullo_epi32(s7, c7));
                e4 = _mm256_add_epi32(e0, e1);
                e5 = _mm256_add_epi32(e2, e3);
                e6 = _mm256_add_epi32(e4, e5);

                v0 = _mm256_add_epi32(v0, e6);

                pb16 += 128;
                coeff += (8 * line);
            }
            r0 = _mm256_add_epi32(v0, rnd_factor);
            r1 = _mm256_packs_epi32(_mm256_srai_epi32(r0, shift), zero);
            r2 = _mm256_permute4x64_epi64(r1, 0x08);
            _mm256_maskstore_epi64((long long*)block, mask, r2);
            coeff -= (16 * line);
            pb16 -= 248;
            block += 8;
        }
        coeff++;
    }
}

void itrans_dst7_pb32_avx2(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    __m256i c0, c1, c2, c3, c4, c5, c6, c7;
    __m256i e0, e1, e2, e3, e4, e5, e6;
    __m256i r0, r1, r2;
    __m256i mask = _mm256_set_epi64x(0, 0, -1, -1);
    __m256i zero = _mm256_setzero_si256();
    __m256i rnd_factor = _mm256_set1_epi32(1 << (shift - 1));

    for (i = 0; i < line; i++)
    {
        s8 *pb16 = iT;
        for (j = 0; j < 4; j++)
        {
            __m256i v0 = zero;
            for (k = 0; k < 4; k++)
            {
                s0 = _mm256_set1_epi32(coeff[0]);
                s1 = _mm256_set1_epi32(coeff[line]);
                s2 = _mm256_set1_epi32(coeff[2 * line]);
                s3 = _mm256_set1_epi32(coeff[3 * line]);
                s4 = _mm256_set1_epi32(coeff[4 * line]);
                s5 = _mm256_set1_epi32(coeff[5 * line]);
                s6 = _mm256_set1_epi32(coeff[6 * line]);
                s7 = _mm256_set1_epi32(coeff[7 * line]);

                c0 = _mm256_load_si256((__m256i*)pb16);
                c0 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c0));
                c1 = _mm256_load_si256((__m256i*)(pb16 + 32));
                c1 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c1));
                c2 = _mm256_load_si256((__m256i*)(pb16 + 64));
                c2 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c2));
                c3 = _mm256_load_si256((__m256i*)(pb16 + 96));
                c3 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c3));
                c4 = _mm256_load_si256((__m256i*)(pb16 + 128));
                c4 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c4));
                c5 = _mm256_load_si256((__m256i*)(pb16 + 160));
                c5 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c5));
                c6 = _mm256_load_si256((__m256i*)(pb16 + 192));
                c6 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c6));
                c7 = _mm256_load_si256((__m256i*)(pb16 + 224));
                c7 = _mm256_cvtepi8_epi32(_mm256_castsi256_si128(c7));

                e0 = _mm256_add_epi32(_mm256_mullo_epi32(s0, c0), _mm256_mullo_epi32(s1, c1));
                e1 = _mm256_add_epi32(_mm256_mullo_epi32(s2, c2), _mm256_mullo_epi32(s3, c3));
                e2 = _mm256_add_epi32(_mm256_mullo_epi32(s4, c4), _mm256_mullo_epi32(s5, c5));
                e3 = _mm256_add_epi32(_mm256_mullo_epi32(s6, c6), _mm256_mullo_epi32(s7, c7));
                e4 = _mm256_add_epi32(e0, e1);
                e5 = _mm256_add_epi32(e2, e3);
                e6 = _mm256_add_epi32(e4, e5);

                v0 = _mm256_add_epi32(v0, e6);

                pb16 += 256;
                coeff += (32 * line);
            }
            r0 = _mm256_add_epi32(v0, rnd_factor);
            r1 = _mm256_packs_epi32(_mm256_srai_epi32(r0, shift), zero);
            r2 = _mm256_permute4x64_epi64(r1, 0x08);
            _mm256_maskstore_epi64((long long*)block, mask, r2);
            coeff -= (32 * line);
            pb16 -= 1016;
            block += 8;
        }
        coeff++;
    }
}
