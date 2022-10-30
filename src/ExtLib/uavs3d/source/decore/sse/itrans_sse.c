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


ALIGNED_32(static tab_s16 tab_idct_8x8[12][8]) = {
    { 44, 38, 44, 38, 44, 38, 44, 38 },
    { 25, 9, 25, 9, 25, 9, 25, 9 },
    { 38, -9, 38, -9, 38, -9, 38, -9 },
    { -44, -25, -44, -25, -44, -25, -44, -25 },
    { 25, -44, 25, -44, 25, -44, 25, -44 },
    { 9, 38, 9, 38, 9, 38, 9, 38 },
    { 9, -25, 9, -25, 9, -25, 9, -25 },
    { 38, -44, 38, -44, 38, -44, 38, -44 },
    { 32, 32, 32, 32, 32, 32, 32, 32 },
    { 32, -32, 32, -32, 32, -32, 32, -32 },
    { 42, 17, 42, 17, 42, 17, 42, 17 },
    { 17, -42, 17, -42, 17, -42, 17, -42 }
};

#define TRANSPOSE_8x8_16BIT(I0, I1, I2, I3, I4, I5, I6, I7, O0, O1, O2, O3, O4, O5, O6, O7) \
	        tr0_0 = _mm_unpacklo_epi16(I0, I1); \
	        tr0_1 = _mm_unpacklo_epi16(I2, I3); \
	        tr0_2 = _mm_unpackhi_epi16(I0, I1); \
	        tr0_3 = _mm_unpackhi_epi16(I2, I3); \
	        tr0_4 = _mm_unpacklo_epi16(I4, I5); \
	        tr0_5 = _mm_unpacklo_epi16(I6, I7); \
	        tr0_6 = _mm_unpackhi_epi16(I4, I5); \
	        tr0_7 = _mm_unpackhi_epi16(I6, I7); \
	        tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1); \
	        tr1_1 = _mm_unpacklo_epi32(tr0_2, tr0_3); \
	        tr1_2 = _mm_unpackhi_epi32(tr0_0, tr0_1); \
	        tr1_3 = _mm_unpackhi_epi32(tr0_2, tr0_3); \
	        tr1_4 = _mm_unpacklo_epi32(tr0_4, tr0_5); \
	        tr1_5 = _mm_unpacklo_epi32(tr0_6, tr0_7); \
	        tr1_6 = _mm_unpackhi_epi32(tr0_4, tr0_5); \
	        tr1_7 = _mm_unpackhi_epi32(tr0_6, tr0_7); \
	        O0 = _mm_unpacklo_epi64(tr1_0, tr1_4); \
	        O1 = _mm_unpackhi_epi64(tr1_0, tr1_4); \
	        O2 = _mm_unpacklo_epi64(tr1_2, tr1_6); \
	        O3 = _mm_unpackhi_epi64(tr1_2, tr1_6); \
	        O4 = _mm_unpacklo_epi64(tr1_1, tr1_5); \
	        O5 = _mm_unpackhi_epi64(tr1_1, tr1_5); \
	        O6 = _mm_unpacklo_epi64(tr1_3, tr1_7); \
	        O7 = _mm_unpackhi_epi64(tr1_3, tr1_7); 

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

static void uavs3d_always_inline dct2_butterfly_h4_sse(s16* src, s16* dst, int line, int shift, int bit_depth)
{
    __m128i s0, s1, s2, s3;
    __m128i e0, e1, o0, o1;
    __m128i v0, v1, v2, v3;
    const __m128i c16_p17_p42 = _mm_set1_epi32(0x0011002A);
    const __m128i c16_n42_p17 = _mm_set1_epi32(0xFFD60011);
    const __m128i c16_n32_p32 = _mm_set1_epi32(0xFFe00020);
    const __m128i c16_p32_p32 = _mm_set1_epi32(0x00200020);
    __m128i off = _mm_set1_epi32(1 << (shift - 1));
    int j;
    int i_src  = line;
    int i_src2 = line << 1;
    int i_src3 = i_src + i_src2;

    for (j = 0; j < line; j += 4)
    {
        s0 = _mm_loadl_epi64((__m128i*)(src + j));
        s1 = _mm_loadl_epi64((__m128i*)(src + i_src + j));
        s2 = _mm_loadl_epi64((__m128i*)(src + i_src2 + j));
        s3 = _mm_loadl_epi64((__m128i*)(src + i_src3 + j));
        s2 = _mm_unpacklo_epi16(s0, s2);
        s3 = _mm_unpacklo_epi16(s1, s3);

        e0 = _mm_madd_epi16(s2, c16_p32_p32);
        e1 = _mm_madd_epi16(s2, c16_n32_p32);
        o0 = _mm_madd_epi16(s3, c16_p17_p42);
        o1 = _mm_madd_epi16(s3, c16_n42_p17);
        v0 = _mm_add_epi32(e0, o0);
        v1 = _mm_add_epi32(e1, o1);
        v2 = _mm_sub_epi32(e1, o1);
        v3 = _mm_sub_epi32(e0, o0);

        v0 = _mm_add_epi32(v0, off);
        v1 = _mm_add_epi32(v1, off);
        v2 = _mm_add_epi32(v2, off);
        v3 = _mm_add_epi32(v3, off);

        v0 = _mm_srai_epi32(v0, shift);
        v1 = _mm_srai_epi32(v1, shift);
        v2 = _mm_srai_epi32(v2, shift);
        v3 = _mm_srai_epi32(v3, shift);

        v0 = _mm_packs_epi32(v0, v2);       // 00 10 20 30 02 12 22 32
        v1 = _mm_packs_epi32(v1, v3);       // 01 11 21 31 03 13 23 33

        // inverse
        s0 = _mm_unpacklo_epi16(v0, v1);   
        s1 = _mm_unpackhi_epi16(v0, v1);
        v0 = _mm_unpacklo_epi32(s0, s1);    // 00 01 02 03 10 11 12 13
        v1 = _mm_unpackhi_epi32(s0, s1);    // 20 21 22 23 30 31 32 33

        if (bit_depth != MAX_TX_DYNAMIC_RANGE) {
            __m128i max_val = _mm_set1_epi16((1 << bit_depth) - 1);
            __m128i min_val = _mm_set1_epi16(-(1 << bit_depth));
            v0 = _mm_min_epi16(v0, max_val);
            v1 = _mm_min_epi16(v1, max_val);
            v0 = _mm_max_epi16(v0, min_val);
            v1 = _mm_max_epi16(v1, min_val);
        }
        _mm_store_si128((__m128i*)dst, v0);
        _mm_store_si128((__m128i*)(dst + 8), v1);
        dst += 16;
    }
}

void dct2_butterfly_h8_sse(s16* src, int i_src, s16* dst, int line, int shift, int bit_depth)
{
    __m128i s0, s1, s2, s3, s4, s5, s6, s7;
    __m128i e0l, e1l, e2l, e3l, O0l, O1l, O2l, O3l, ee0l, ee1l, e00l, e01l;
    __m128i t0, t1, t2, t3;
    __m128i c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11;
    __m128i offset = _mm_set1_epi32(1 << (shift - 1));
    __m128i zero = _mm_setzero_si128();
    int j;
    int i_src2 = i_src << 1;
    int i_src3 = i_src + i_src2;
    int i_src4 = i_src << 2;
    int i_src5 = i_src2 + i_src3;
    int i_src6 = i_src3 << 1;
    int i_src7 = i_src3 + i_src4;

    c0  = _mm_load_si128((__m128i*)(tab_idct_8x8[0 ]));
    c1  = _mm_load_si128((__m128i*)(tab_idct_8x8[1 ]));
    c2  = _mm_load_si128((__m128i*)(tab_idct_8x8[2 ]));
    c3  = _mm_load_si128((__m128i*)(tab_idct_8x8[3 ]));
    c4  = _mm_load_si128((__m128i*)(tab_idct_8x8[4 ]));
    c5  = _mm_load_si128((__m128i*)(tab_idct_8x8[5 ]));
    c6  = _mm_load_si128((__m128i*)(tab_idct_8x8[6 ]));
    c7  = _mm_load_si128((__m128i*)(tab_idct_8x8[7 ]));
    c8  = _mm_load_si128((__m128i*)(tab_idct_8x8[8 ]));
    c9  = _mm_load_si128((__m128i*)(tab_idct_8x8[9 ]));
    c10 = _mm_load_si128((__m128i*)(tab_idct_8x8[10]));
    c11 = _mm_load_si128((__m128i*)(tab_idct_8x8[11]));

    for (j = 0; j < line; j += 4)
    {
        // O[0] -- O[3]
        s1 = _mm_loadl_epi64((__m128i*)(src + i_src + j));
        s3 = _mm_loadl_epi64((__m128i*)(src + i_src3 + j));
        s5 = _mm_loadl_epi64((__m128i*)(src + i_src5 + j));
        s7 = _mm_loadl_epi64((__m128i*)(src + i_src7 + j));

        t0 = _mm_unpacklo_epi16(s1, s3);
        t2 = _mm_unpacklo_epi16(s5, s7);

        e1l = _mm_madd_epi16(t0, c0);
        e2l = _mm_madd_epi16(t2, c1);
        O0l = _mm_add_epi32(e1l, e2l);

        e1l = _mm_madd_epi16(t0, c2);
        e2l = _mm_madd_epi16(t2, c3);
        O1l = _mm_add_epi32(e1l, e2l);

        e1l = _mm_madd_epi16(t0, c4);
        e2l = _mm_madd_epi16(t2, c5);
        O2l = _mm_add_epi32(e1l, e2l);

        e1l = _mm_madd_epi16(t0, c6);
        e2l = _mm_madd_epi16(t2, c7);
        O3l = _mm_add_epi32(e1l, e2l);

        // E[0] - E[3]
        s0 = _mm_loadl_epi64((__m128i*)(src + j));
        s2 = _mm_loadl_epi64((__m128i*)(src + i_src2 + j));
        s4 = _mm_loadl_epi64((__m128i*)(src + i_src4 + j));
        s6 = _mm_loadl_epi64((__m128i*)(src + i_src6 + j));

        t0 = _mm_unpacklo_epi16(s0, s4);
        ee0l = _mm_madd_epi16(t0, c8);
        ee1l = _mm_madd_epi16(t0, c9);

        t0 = _mm_unpacklo_epi16(s2, s6);
        e00l = _mm_madd_epi16(t0, c10);
        e01l = _mm_madd_epi16(t0, c11);
        e0l = _mm_add_epi32(ee0l, e00l);
        e3l = _mm_sub_epi32(ee0l, e00l);
        e0l = _mm_add_epi32(e0l, offset);
        e3l = _mm_add_epi32(e3l, offset);

        e1l = _mm_add_epi32(ee1l, e01l);
        e2l = _mm_sub_epi32(ee1l, e01l);
        e1l = _mm_add_epi32(e1l, offset);
        e2l = _mm_add_epi32(e2l, offset);
        s0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e0l, O0l), shift), zero);		
        s7 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e0l, O0l), shift), zero);
        s1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e1l, O1l), shift), zero);
        s6 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e1l, O1l), shift), zero);
        s2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e2l, O2l), shift), zero);
        s5 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e2l, O2l), shift), zero);
        s3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e3l, O3l), shift), zero);
        s4 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e3l, O3l), shift), zero);

        /*  inverse   */
        e0l = _mm_unpacklo_epi16(s0, s4);
        e1l = _mm_unpacklo_epi16(s1, s5);
        e2l = _mm_unpacklo_epi16(s2, s6);
        e3l = _mm_unpacklo_epi16(s3, s7);
        t0 = _mm_unpacklo_epi16(e0l, e2l);
        t1 = _mm_unpacklo_epi16(e1l, e3l);
        s0 = _mm_unpacklo_epi16(t0, t1);
        s1 = _mm_unpackhi_epi16(t0, t1);
        t2 = _mm_unpackhi_epi16(e0l, e2l);
        t3 = _mm_unpackhi_epi16(e1l, e3l);
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
        dst += 32; 
    }
}

void dct2_butterfly_h16_sse(s16* src, int i_src, s16* dst, int line, int shift, int bit_depth)
{
    const __m128i c16_p43_p45 = _mm_set1_epi32(0x002B002D);		//row0 
    const __m128i c16_p35_p40 = _mm_set1_epi32(0x00230028);
    const __m128i c16_p21_p29 = _mm_set1_epi32(0x0015001D);
    const __m128i c16_p04_p13 = _mm_set1_epi32(0x0004000D);
    const __m128i c16_p29_p43 = _mm_set1_epi32(0x001D002B);		//row1
    const __m128i c16_n21_p04 = _mm_set1_epi32(0xFFEB0004);
    const __m128i c16_n45_n40 = _mm_set1_epi32(0xFFD3FFD8);
    const __m128i c16_n13_n35 = _mm_set1_epi32(0xFFF3FFDD);
    const __m128i c16_p04_p40 = _mm_set1_epi32(0x00040028);		//row2
    const __m128i c16_n43_n35 = _mm_set1_epi32(0xFFD5FFDD);
    const __m128i c16_p29_n13 = _mm_set1_epi32(0x001DFFF3);
    const __m128i c16_p21_p45 = _mm_set1_epi32(0x0015002D);
    const __m128i c16_n21_p35 = _mm_set1_epi32(0xFFEB0023);		//row3
    const __m128i c16_p04_n43 = _mm_set1_epi32(0x0004FFD5);
    const __m128i c16_p13_p45 = _mm_set1_epi32(0x000D002D);
    const __m128i c16_n29_n40 = _mm_set1_epi32(0xFFE3FFD8);
    const __m128i c16_n40_p29 = _mm_set1_epi32(0xFFD8001D);		//row4
    const __m128i c16_p45_n13 = _mm_set1_epi32(0x002DFFF3);
    const __m128i c16_n43_n04 = _mm_set1_epi32(0xFFD5FFFC);
    const __m128i c16_p35_p21 = _mm_set1_epi32(0x00230015);
    const __m128i c16_n45_p21 = _mm_set1_epi32(0xFFD30015);		//row5
    const __m128i c16_p13_p29 = _mm_set1_epi32(0x000D001D);
    const __m128i c16_p35_n43 = _mm_set1_epi32(0x0023FFD5);
    const __m128i c16_n40_p04 = _mm_set1_epi32(0xFFD80004);
    const __m128i c16_n35_p13 = _mm_set1_epi32(0xFFDD000D);		//row6
    const __m128i c16_n40_p45 = _mm_set1_epi32(0xFFD8002D);
    const __m128i c16_p04_p21 = _mm_set1_epi32(0x00040015);
    const __m128i c16_p43_n29 = _mm_set1_epi32(0x002BFFE3);
    const __m128i c16_n13_p04 = _mm_set1_epi32(0xFFF30004);		//row7
    const __m128i c16_n29_p21 = _mm_set1_epi32(0xFFE30015);
    const __m128i c16_n40_p35 = _mm_set1_epi32(0xFFD80023);
    const __m128i c16_n45_p43 = _mm_set1_epi32(0xFFD3002B);
    const __m128i c16_p38_p44 = _mm_set1_epi32(0x0026002C);
    const __m128i c16_p09_p25 = _mm_set1_epi32(0x00090019);
    const __m128i c16_n09_p38 = _mm_set1_epi32(0xFFF70026);
    const __m128i c16_n25_n44 = _mm_set1_epi32(0xFFE7FFD4);
    const __m128i c16_n44_p25 = _mm_set1_epi32(0xFFD40019);
    const __m128i c16_p38_p09 = _mm_set1_epi32(0x00260009);
    const __m128i c16_n25_p09 = _mm_set1_epi32(0xFFE70009);
    const __m128i c16_n44_p38 = _mm_set1_epi32(0xFFD40026);
    const __m128i c16_p17_p42 = _mm_set1_epi32(0x0011002A);
    const __m128i c16_n42_p17 = _mm_set1_epi32(0xFFD60011);
    const __m128i c16_n32_p32 = _mm_set1_epi32(0xFFE00020);
    const __m128i c16_p32_p32 = _mm_set1_epi32(0x00200020);
    int i;
    __m128i c32_off = _mm_set1_epi32(1 << (shift - 1));
    __m128i src00, src01, src02, src03, src04, src05, src06, src07;
    __m128i src08, src09, src10, src11, src12, src13, src14, src15;
    __m128i res00, res01, res02, res03, res04, res05, res06, res07;
    __m128i res08, res09, res10, res11, res12, res13, res14, res15;
    __m128i T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07;
    __m128i O0, O1, O2, O3, O4, O5, O6, O7;
    __m128i EO0, EO1, EO2, EO3;
    __m128i EEO0, EEO1;
    __m128i EEE0, EEE1;
    __m128i T00, T01;
    __m128i zero = _mm_setzero_si128();

    for (i = 0; i < line; i += 4)
    {
        src00 = _mm_loadl_epi64((const __m128i*)&src[0 * i_src + i]);    // [07 06 05 04 03 02 01 00]
        src01 = _mm_loadl_epi64((const __m128i*)&src[1 * i_src + i]);    // [17 16 15 14 13 12 11 10]
        src02 = _mm_loadl_epi64((const __m128i*)&src[2 * i_src + i]);    // [27 26 25 24 23 22 21 20]
        src03 = _mm_loadl_epi64((const __m128i*)&src[3 * i_src + i]);    // [37 36 35 34 33 32 31 30]
        src04 = _mm_loadl_epi64((const __m128i*)&src[4 * i_src + i]);    // [47 46 45 44 43 42 41 40]
        src05 = _mm_loadl_epi64((const __m128i*)&src[5 * i_src + i]);    // [57 56 55 54 53 52 51 50]
        src06 = _mm_loadl_epi64((const __m128i*)&src[6 * i_src + i]);    // [67 66 65 64 63 62 61 60]
        src07 = _mm_loadl_epi64((const __m128i*)&src[7 * i_src + i]);    // [77 76 75 74 73 72 71 70]
        src08 = _mm_loadl_epi64((const __m128i*)&src[8 * i_src + i]);
        src09 = _mm_loadl_epi64((const __m128i*)&src[9 * i_src + i]);
        src10 = _mm_loadl_epi64((const __m128i*)&src[10 * i_src + i]);
        src11 = _mm_loadl_epi64((const __m128i*)&src[11 * i_src + i]);
        src12 = _mm_loadl_epi64((const __m128i*)&src[12 * i_src + i]);
        src13 = _mm_loadl_epi64((const __m128i*)&src[13 * i_src + i]);
        src14 = _mm_loadl_epi64((const __m128i*)&src[14 * i_src + i]);
        src15 = _mm_loadl_epi64((const __m128i*)&src[15 * i_src + i]);

        T_00_00 = _mm_unpacklo_epi16(src01, src03);       // [33 13 32 12 31 11 30 10]
        T_00_01 = _mm_unpacklo_epi16(src05, src07);       // [ ]
        T_00_02 = _mm_unpacklo_epi16(src09, src11);       // [ ]
        T_00_03 = _mm_unpacklo_epi16(src13, src15);       // [ ]
        T_00_04 = _mm_unpacklo_epi16(src02, src06);       // [ ]
        T_00_05 = _mm_unpacklo_epi16(src10, src14);       // [ ]
        T_00_06 = _mm_unpacklo_epi16(src04, src12);       // [ ]
        T_00_07 = _mm_unpacklo_epi16(src00, src08);       // [83 03 82 02 81 01 81 00] row08 row00

#define COMPUTE_ROW(row0103, row0507, row0911, row1315, c0103, c0507, c0911, c1315, row) \
	    T00 = _mm_add_epi32(_mm_madd_epi16(row0103, c0103), _mm_madd_epi16(row0507, c0507)); \
	    T01 = _mm_add_epi32(_mm_madd_epi16(row0911, c0911), _mm_madd_epi16(row1315, c1315)); \
	    row = _mm_add_epi32(T00, T01);

        COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, c16_p43_p45, c16_p35_p40, c16_p21_p29, c16_p04_p13, O0)
        COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, c16_p29_p43, c16_n21_p04, c16_n45_n40, c16_n13_n35, O1)
        COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, c16_p04_p40, c16_n43_n35, c16_p29_n13, c16_p21_p45, O2)
        COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, c16_n21_p35, c16_p04_n43, c16_p13_p45, c16_n29_n40, O3)
        COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, c16_n40_p29, c16_p45_n13, c16_n43_n04, c16_p35_p21, O4)
        COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, c16_n45_p21, c16_p13_p29, c16_p35_n43, c16_n40_p04, O5)
        COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, c16_n35_p13, c16_n40_p45, c16_p04_p21, c16_p43_n29, O6)
        COMPUTE_ROW(T_00_00, T_00_01, T_00_02, T_00_03, c16_n13_p04, c16_n29_p21, c16_n40_p35, c16_n45_p43, O7)

#undef COMPUTE_ROW

        EO0 = _mm_add_epi32(_mm_madd_epi16(T_00_04, c16_p38_p44), _mm_madd_epi16(T_00_05, c16_p09_p25)); // EO0
        EO1 = _mm_add_epi32(_mm_madd_epi16(T_00_04, c16_n09_p38), _mm_madd_epi16(T_00_05, c16_n25_n44)); // EO1
        EO2 = _mm_add_epi32(_mm_madd_epi16(T_00_04, c16_n44_p25), _mm_madd_epi16(T_00_05, c16_p38_p09)); // EO2
        EO3 = _mm_add_epi32(_mm_madd_epi16(T_00_04, c16_n25_p09), _mm_madd_epi16(T_00_05, c16_n44_p38)); // EO3

        EEO0 = _mm_madd_epi16(T_00_06, c16_p17_p42);
        EEO1 = _mm_madd_epi16(T_00_06, c16_n42_p17);

        EEE0 = _mm_madd_epi16(T_00_07, c16_p32_p32);
        EEE1 = _mm_madd_epi16(T_00_07, c16_n32_p32);

        {
            const __m128i EE0 = _mm_add_epi32(EEE0, EEO0);          // EE0 = EEE0 + EEO0
            const __m128i EE1 = _mm_add_epi32(EEE1, EEO1);          // EE1 = EEE1 + EEO1
            const __m128i EE3 = _mm_sub_epi32(EEE0, EEO0);          // EE2 = EEE0 - EEO0
            const __m128i EE2 = _mm_sub_epi32(EEE1, EEO1);          // EE3 = EEE1 - EEO1

            const __m128i E0 = _mm_add_epi32(EE0, EO0);          // E0 = EE0 + EO0
            const __m128i E1 = _mm_add_epi32(EE1, EO1);          // E1 = EE1 + EO1
            const __m128i E2 = _mm_add_epi32(EE2, EO2);          // E2 = EE2 + EO2
            const __m128i E3 = _mm_add_epi32(EE3, EO3);          // E3 = EE3 + EO3
            const __m128i E7 = _mm_sub_epi32(EE0, EO0);          // E0 = EE0 - EO0
            const __m128i E6 = _mm_sub_epi32(EE1, EO1);          // E1 = EE1 - EO1
            const __m128i E5 = _mm_sub_epi32(EE2, EO2);          // E2 = EE2 - EO2
            const __m128i E4 = _mm_sub_epi32(EE3, EO3);          // E3 = EE3 - EO3

            const __m128i T10 = _mm_add_epi32(E0, c32_off);         // E0 + off
            const __m128i T11 = _mm_add_epi32(E1, c32_off);         // E1 + off
            const __m128i T12 = _mm_add_epi32(E2, c32_off);         // E2 + off
            const __m128i T13 = _mm_add_epi32(E3, c32_off);         // E3 + off
            const __m128i T14 = _mm_add_epi32(E4, c32_off);         // E4 + off
            const __m128i T15 = _mm_add_epi32(E5, c32_off);         // E5 + off
            const __m128i T16 = _mm_add_epi32(E6, c32_off);         // E6 + off
            const __m128i T17 = _mm_add_epi32(E7, c32_off);         // E7 + off

            const __m128i T20 = _mm_add_epi32(T10, O0);          // E0 + O0 + off
            const __m128i T21 = _mm_add_epi32(T11, O1);          // E1 + O1 + off
            const __m128i T22 = _mm_add_epi32(T12, O2);          // E2 + O2 + off
            const __m128i T23 = _mm_add_epi32(T13, O3);          // E3 + O3 + off
            const __m128i T24 = _mm_add_epi32(T14, O4);          // E4
            const __m128i T25 = _mm_add_epi32(T15, O5);          // E5
            const __m128i T26 = _mm_add_epi32(T16, O6);          // E6
            const __m128i T27 = _mm_add_epi32(T17, O7);          // E7
            const __m128i T2F = _mm_sub_epi32(T10, O0);          // E0 - O0 + off
            const __m128i T2E = _mm_sub_epi32(T11, O1);          // E1 - O1 + off
            const __m128i T2D = _mm_sub_epi32(T12, O2);          // E2 - O2 + off
            const __m128i T2C = _mm_sub_epi32(T13, O3);          // E3 - O3 + off
            const __m128i T2B = _mm_sub_epi32(T14, O4);          // E4
            const __m128i T2A = _mm_sub_epi32(T15, O5);          // E5
            const __m128i T29 = _mm_sub_epi32(T16, O6);          // E6
            const __m128i T28 = _mm_sub_epi32(T17, O7);          // E7

            const __m128i T30 = _mm_srai_epi32(T20, shift);             // [30 20 10 00]
            const __m128i T31 = _mm_srai_epi32(T21, shift);             // [31 21 11 01]
            const __m128i T32 = _mm_srai_epi32(T22, shift);             // [32 22 12 02]
            const __m128i T33 = _mm_srai_epi32(T23, shift);             // [33 23 13 03]
            const __m128i T34 = _mm_srai_epi32(T24, shift);             // [33 24 14 04]
            const __m128i T35 = _mm_srai_epi32(T25, shift);             // [35 25 15 05]
            const __m128i T36 = _mm_srai_epi32(T26, shift);             // [36 26 16 06]
            const __m128i T37 = _mm_srai_epi32(T27, shift);             // [37 27 17 07]

            const __m128i T38 = _mm_srai_epi32(T28, shift);             // [30 20 10 00] x8
            const __m128i T39 = _mm_srai_epi32(T29, shift);             // [31 21 11 01] x9
            const __m128i T3A = _mm_srai_epi32(T2A, shift);             // [32 22 12 02] xA
            const __m128i T3B = _mm_srai_epi32(T2B, shift);             // [33 23 13 03] xB
            const __m128i T3C = _mm_srai_epi32(T2C, shift);             // [33 24 14 04] xC
            const __m128i T3D = _mm_srai_epi32(T2D, shift);             // [35 25 15 05] xD
            const __m128i T3E = _mm_srai_epi32(T2E, shift);             // [36 26 16 06] xE
            const __m128i T3F = _mm_srai_epi32(T2F, shift);             // [37 27 17 07] xF

            res00 = _mm_packs_epi32(T30, zero);        // [70 60 50 40 30 20 10 00]
            res01 = _mm_packs_epi32(T31, zero);        // [71 61 51 41 31 21 11 01]
            res02 = _mm_packs_epi32(T32, zero);        // [72 62 52 42 32 22 12 02]
            res03 = _mm_packs_epi32(T33, zero);        // [73 63 53 43 33 23 13 03]
            res04 = _mm_packs_epi32(T34, zero);        // [74 64 54 44 34 24 14 04]
            res05 = _mm_packs_epi32(T35, zero);        // [75 65 55 45 35 25 15 05]
            res06 = _mm_packs_epi32(T36, zero);        // [76 66 56 46 36 26 16 06]
            res07 = _mm_packs_epi32(T37, zero);        // [77 67 57 47 37 27 17 07]

            res08 = _mm_packs_epi32(T38, zero);        // [A0 ... 80]
            res09 = _mm_packs_epi32(T39, zero);        // [A1 ... 81]
            res10 = _mm_packs_epi32(T3A, zero);        // [A2 ... 82]
            res11 = _mm_packs_epi32(T3B, zero);        // [A3 ... 83]
            res12 = _mm_packs_epi32(T3C, zero);        // [A4 ... 84]
            res13 = _mm_packs_epi32(T3D, zero);        // [A5 ... 85]
            res14 = _mm_packs_epi32(T3E, zero);        // [A6 ... 86]
            res15 = _mm_packs_epi32(T3F, zero);        // [A7 ... 87]
        }
        { // transpose 16x4 --> 4x16
            __m128i tr0_0, tr0_1, tr0_2, tr0_3;
            __m128i tr1_0, tr1_1, tr1_2, tr1_3;

            TRANSPOSE_8x4_16BIT(res00, res01, res02, res03, res04, res05, res06, res07, src00, src02, src04, src06)
            TRANSPOSE_8x4_16BIT(res08, res09, res10, res11, res12, res13, res14, res15, src01, src03, src05, src07)
        }
        if (bit_depth != MAX_TX_DYNAMIC_RANGE) { //clip
            __m128i max_val = _mm_set1_epi16((1 << bit_depth) - 1);
            __m128i min_val = _mm_set1_epi16(-(1 << bit_depth));

            src00 = _mm_min_epi16(src00, max_val);
            src01 = _mm_min_epi16(src01, max_val);
            src02 = _mm_min_epi16(src02, max_val);
            src03 = _mm_min_epi16(src03, max_val);
            src04 = _mm_min_epi16(src04, max_val);
            src05 = _mm_min_epi16(src05, max_val);
            src06 = _mm_min_epi16(src06, max_val);
            src07 = _mm_min_epi16(src07, max_val);

            src00 = _mm_max_epi16(src00, min_val);
            src01 = _mm_max_epi16(src01, min_val);
            src02 = _mm_max_epi16(src02, min_val);
            src03 = _mm_max_epi16(src03, min_val);
            src04 = _mm_max_epi16(src04, min_val);
            src05 = _mm_max_epi16(src05, min_val);
            src06 = _mm_max_epi16(src06, min_val);
            src07 = _mm_max_epi16(src07, min_val);

        }

        _mm_store_si128((__m128i*)&dst[0 ], src00);
        _mm_store_si128((__m128i*)&dst[8 ], src01);
        _mm_store_si128((__m128i*)&dst[16], src02);
        _mm_store_si128((__m128i*)&dst[24], src03);
        _mm_store_si128((__m128i*)&dst[32], src04);
        _mm_store_si128((__m128i*)&dst[40], src05);
        _mm_store_si128((__m128i*)&dst[48], src06);
        _mm_store_si128((__m128i*)&dst[56], src07);

        dst += 64; // 4 rows
    }
}

void dct2_butterfly_h32_sse(s16* src, int i_src, s16* dst, int line, int shift, int bit_depth)
{
    const __m128i c16_p45_p45 = _mm_set1_epi32(0x002D002D);
    const __m128i c16_p43_p44 = _mm_set1_epi32(0x002B002C);
    const __m128i c16_p39_p41 = _mm_set1_epi32(0x00270029);
    const __m128i c16_p34_p36 = _mm_set1_epi32(0x00220024);
    const __m128i c16_p27_p30 = _mm_set1_epi32(0x001B001E);
    const __m128i c16_p19_p23 = _mm_set1_epi32(0x00130017);
    const __m128i c16_p11_p15 = _mm_set1_epi32(0x000B000F);
    const __m128i c16_p02_p07 = _mm_set1_epi32(0x00020007);
    const __m128i c16_p41_p45 = _mm_set1_epi32(0x0029002D);
    const __m128i c16_p23_p34 = _mm_set1_epi32(0x00170022);
    const __m128i c16_n02_p11 = _mm_set1_epi32(0xFFFE000B);
    const __m128i c16_n27_n15 = _mm_set1_epi32(0xFFE5FFF1);
    const __m128i c16_n43_n36 = _mm_set1_epi32(0xFFD5FFDC);
    const __m128i c16_n44_n45 = _mm_set1_epi32(0xFFD4FFD3);
    const __m128i c16_n30_n39 = _mm_set1_epi32(0xFFE2FFD9);
    const __m128i c16_n07_n19 = _mm_set1_epi32(0xFFF9FFED);
    const __m128i c16_p34_p44 = _mm_set1_epi32(0x0022002C);
    const __m128i c16_n07_p15 = _mm_set1_epi32(0xFFF9000F);
    const __m128i c16_n41_n27 = _mm_set1_epi32(0xFFD7FFE5);
    const __m128i c16_n39_n45 = _mm_set1_epi32(0xFFD9FFD3);
    const __m128i c16_n02_n23 = _mm_set1_epi32(0xFFFEFFE9);
    const __m128i c16_p36_p19 = _mm_set1_epi32(0x00240013);
    const __m128i c16_p43_p45 = _mm_set1_epi32(0x002B002D);
    const __m128i c16_p11_p30 = _mm_set1_epi32(0x000B001E);
    const __m128i c16_p23_p43 = _mm_set1_epi32(0x0017002B);
    const __m128i c16_n34_n07 = _mm_set1_epi32(0xFFDEFFF9);
    const __m128i c16_n36_n45 = _mm_set1_epi32(0xFFDCFFD3);
    const __m128i c16_p19_n11 = _mm_set1_epi32(0x0013FFF5);
    const __m128i c16_p44_p41 = _mm_set1_epi32(0x002C0029);
    const __m128i c16_n02_p27 = _mm_set1_epi32(0xFFFE001B);
    const __m128i c16_n45_n30 = _mm_set1_epi32(0xFFD3FFE2);
    const __m128i c16_n15_n39 = _mm_set1_epi32(0xFFF1FFD9);
    const __m128i c16_p11_p41 = _mm_set1_epi32(0x000B0029);
    const __m128i c16_n45_n27 = _mm_set1_epi32(0xFFD3FFE5);
    const __m128i c16_p07_n30 = _mm_set1_epi32(0x0007FFE2);
    const __m128i c16_p43_p39 = _mm_set1_epi32(0x002B0027);
    const __m128i c16_n23_p15 = _mm_set1_epi32(0xFFE9000F);
    const __m128i c16_n34_n45 = _mm_set1_epi32(0xFFDEFFD3);
    const __m128i c16_p36_p02 = _mm_set1_epi32(0x00240002);
    const __m128i c16_p19_p44 = _mm_set1_epi32(0x0013002C);
    const __m128i c16_n02_p39 = _mm_set1_epi32(0xFFFE0027);
    const __m128i c16_n36_n41 = _mm_set1_epi32(0xFFDCFFD7);
    const __m128i c16_p43_p07 = _mm_set1_epi32(0x002B0007);
    const __m128i c16_n11_p34 = _mm_set1_epi32(0xFFF50022);
    const __m128i c16_n30_n44 = _mm_set1_epi32(0xFFE2FFD4);
    const __m128i c16_p45_p15 = _mm_set1_epi32(0x002D000F);
    const __m128i c16_n19_p27 = _mm_set1_epi32(0xFFED001B);
    const __m128i c16_n23_n45 = _mm_set1_epi32(0xFFE9FFD3);
    const __m128i c16_n15_p36 = _mm_set1_epi32(0xFFF10024);
    const __m128i c16_n11_n45 = _mm_set1_epi32(0xFFF5FFD3);
    const __m128i c16_p34_p39 = _mm_set1_epi32(0x00220027);
    const __m128i c16_n45_n19 = _mm_set1_epi32(0xFFD3FFED);
    const __m128i c16_p41_n07 = _mm_set1_epi32(0x0029FFF9);
    const __m128i c16_n23_p30 = _mm_set1_epi32(0xFFE9001E);
    const __m128i c16_n02_n44 = _mm_set1_epi32(0xFFFEFFD4);
    const __m128i c16_p27_p43 = _mm_set1_epi32(0x001B002B);
    const __m128i c16_n27_p34 = _mm_set1_epi32(0xFFE50022);
    const __m128i c16_p19_n39 = _mm_set1_epi32(0x0013FFD9);
    const __m128i c16_n11_p43 = _mm_set1_epi32(0xFFF5002B);
    const __m128i c16_p02_n45 = _mm_set1_epi32(0x0002FFD3);
    const __m128i c16_p07_p45 = _mm_set1_epi32(0x0007002D);
    const __m128i c16_n15_n44 = _mm_set1_epi32(0xFFF1FFD4);
    const __m128i c16_p23_p41 = _mm_set1_epi32(0x00170029);
    const __m128i c16_n30_n36 = _mm_set1_epi32(0xFFE2FFDC);
    const __m128i c16_n36_p30 = _mm_set1_epi32(0xFFDC001E);
    const __m128i c16_p41_n23 = _mm_set1_epi32(0x0029FFE9);
    const __m128i c16_n44_p15 = _mm_set1_epi32(0xFFD4000F);
    const __m128i c16_p45_n07 = _mm_set1_epi32(0x002DFFF9);
    const __m128i c16_n45_n02 = _mm_set1_epi32(0xFFD3FFFE);
    const __m128i c16_p43_p11 = _mm_set1_epi32(0x002B000B);
    const __m128i c16_n39_n19 = _mm_set1_epi32(0xFFD9FFED);
    const __m128i c16_p34_p27 = _mm_set1_epi32(0x0022001B);
    const __m128i c16_n43_p27 = _mm_set1_epi32(0xFFD5001B);
    const __m128i c16_p44_n02 = _mm_set1_epi32(0x002CFFFE);
    const __m128i c16_n30_n23 = _mm_set1_epi32(0xFFE2FFE9);
    const __m128i c16_p07_p41 = _mm_set1_epi32(0x00070029);
    const __m128i c16_p19_n45 = _mm_set1_epi32(0x0013FFD3);
    const __m128i c16_n39_p34 = _mm_set1_epi32(0xFFD90022);
    const __m128i c16_p45_n11 = _mm_set1_epi32(0x002DFFF5);
    const __m128i c16_n36_n15 = _mm_set1_epi32(0xFFDCFFF1);
    const __m128i c16_n45_p23 = _mm_set1_epi32(0xFFD30017);
    const __m128i c16_p27_p19 = _mm_set1_epi32(0x001B0013);
    const __m128i c16_p15_n45 = _mm_set1_epi32(0x000FFFD3);
    const __m128i c16_n44_p30 = _mm_set1_epi32(0xFFD4001E);
    const __m128i c16_p34_p11 = _mm_set1_epi32(0x0022000B);
    const __m128i c16_p07_n43 = _mm_set1_epi32(0x0007FFD5);
    const __m128i c16_n41_p36 = _mm_set1_epi32(0xFFD70024);
    const __m128i c16_p39_p02 = _mm_set1_epi32(0x00270002);
    const __m128i c16_n44_p19 = _mm_set1_epi32(0xFFD40013);
    const __m128i c16_n02_p36 = _mm_set1_epi32(0xFFFE0024);
    const __m128i c16_p45_n34 = _mm_set1_epi32(0x002DFFDE);
    const __m128i c16_n15_n23 = _mm_set1_epi32(0xFFF1FFE9);
    const __m128i c16_n39_p43 = _mm_set1_epi32(0xFFD9002B);
    const __m128i c16_p30_p07 = _mm_set1_epi32(0x001E0007);
    const __m128i c16_p27_n45 = _mm_set1_epi32(0x001BFFD3);
    const __m128i c16_n41_p11 = _mm_set1_epi32(0xFFD7000B);
    const __m128i c16_n39_p15 = _mm_set1_epi32(0xFFD9000F);
    const __m128i c16_n30_p45 = _mm_set1_epi32(0xFFE2002D);
    const __m128i c16_p27_p02 = _mm_set1_epi32(0x001B0002);
    const __m128i c16_p41_n44 = _mm_set1_epi32(0x0029FFD4);
    const __m128i c16_n11_n19 = _mm_set1_epi32(0xFFF5FFED);
    const __m128i c16_n45_p36 = _mm_set1_epi32(0xFFD30024);
    const __m128i c16_n07_p34 = _mm_set1_epi32(0xFFF90022);
    const __m128i c16_p43_n23 = _mm_set1_epi32(0x002BFFE9);
    const __m128i c16_n30_p11 = _mm_set1_epi32(0xFFE2000B);
    const __m128i c16_n45_p43 = _mm_set1_epi32(0xFFD3002B);
    const __m128i c16_n19_p36 = _mm_set1_epi32(0xFFED0024);
    const __m128i c16_p23_n02 = _mm_set1_epi32(0x0017FFFE);
    const __m128i c16_p45_n39 = _mm_set1_epi32(0x002DFFD9);
    const __m128i c16_p27_n41 = _mm_set1_epi32(0x001BFFD7);
    const __m128i c16_n15_n07 = _mm_set1_epi32(0xFFF1FFF9);
    const __m128i c16_n44_p34 = _mm_set1_epi32(0xFFD40022);
    const __m128i c16_n19_p07 = _mm_set1_epi32(0xFFED0007);
    const __m128i c16_n39_p30 = _mm_set1_epi32(0xFFD9001E);
    const __m128i c16_n45_p44 = _mm_set1_epi32(0xFFD3002C);
    const __m128i c16_n36_p43 = _mm_set1_epi32(0xFFDC002B);
    const __m128i c16_n15_p27 = _mm_set1_epi32(0xFFF1001B);
    const __m128i c16_p11_p02 = _mm_set1_epi32(0x000B0002);
    const __m128i c16_p34_n23 = _mm_set1_epi32(0x0022FFE9);
    const __m128i c16_p45_n41 = _mm_set1_epi32(0x002DFFD7);
    const __m128i c16_n07_p02 = _mm_set1_epi32(0xFFF90002);
    const __m128i c16_n15_p11 = _mm_set1_epi32(0xFFF1000B);
    const __m128i c16_n23_p19 = _mm_set1_epi32(0xFFE90013);
    const __m128i c16_n30_p27 = _mm_set1_epi32(0xFFE2001B);
    const __m128i c16_n36_p34 = _mm_set1_epi32(0xFFDC0022);
    const __m128i c16_n41_p39 = _mm_set1_epi32(0xFFD70027);
    const __m128i c16_n44_p43 = _mm_set1_epi32(0xFFD4002B);
    const __m128i c16_n45_p45 = _mm_set1_epi32(0xFFD3002D);

    const __m128i c16_p35_p40 = _mm_set1_epi32(0x00230028);
    const __m128i c16_p21_p29 = _mm_set1_epi32(0x0015001D);
    const __m128i c16_p04_p13 = _mm_set1_epi32(0x0004000D);
    const __m128i c16_p29_p43 = _mm_set1_epi32(0x001D002B);
    const __m128i c16_n21_p04 = _mm_set1_epi32(0xFFEB0004);
    const __m128i c16_n45_n40 = _mm_set1_epi32(0xFFD3FFD8);
    const __m128i c16_n13_n35 = _mm_set1_epi32(0xFFF3FFDD);
    const __m128i c16_p04_p40 = _mm_set1_epi32(0x00040028);
    const __m128i c16_n43_n35 = _mm_set1_epi32(0xFFD5FFDD);
    const __m128i c16_p29_n13 = _mm_set1_epi32(0x001DFFF3);
    const __m128i c16_p21_p45 = _mm_set1_epi32(0x0015002D);
    const __m128i c16_n21_p35 = _mm_set1_epi32(0xFFEB0023);
    const __m128i c16_p04_n43 = _mm_set1_epi32(0x0004FFD5);
    const __m128i c16_p13_p45 = _mm_set1_epi32(0x000D002D);
    const __m128i c16_n29_n40 = _mm_set1_epi32(0xFFE3FFD8);
    const __m128i c16_n40_p29 = _mm_set1_epi32(0xFFD8001D);
    const __m128i c16_p45_n13 = _mm_set1_epi32(0x002DFFF3);
    const __m128i c16_n43_n04 = _mm_set1_epi32(0xFFD5FFFC);
    const __m128i c16_p35_p21 = _mm_set1_epi32(0x00230015);
    const __m128i c16_n45_p21 = _mm_set1_epi32(0xFFD30015);
    const __m128i c16_p13_p29 = _mm_set1_epi32(0x000D001D);
    const __m128i c16_p35_n43 = _mm_set1_epi32(0x0023FFD5);
    const __m128i c16_n40_p04 = _mm_set1_epi32(0xFFD80004);
    const __m128i c16_n35_p13 = _mm_set1_epi32(0xFFDD000D);
    const __m128i c16_n40_p45 = _mm_set1_epi32(0xFFD8002D);
    const __m128i c16_p04_p21 = _mm_set1_epi32(0x00040015);
    const __m128i c16_p43_n29 = _mm_set1_epi32(0x002BFFE3);
    const __m128i c16_n13_p04 = _mm_set1_epi32(0xFFF30004);
    const __m128i c16_n29_p21 = _mm_set1_epi32(0xFFE30015);
    const __m128i c16_n40_p35 = _mm_set1_epi32(0xFFD80023);

    const __m128i c16_p38_p44 = _mm_set1_epi32(0x0026002C);
    const __m128i c16_p09_p25 = _mm_set1_epi32(0x00090019);
    const __m128i c16_n09_p38 = _mm_set1_epi32(0xFFF70026);
    const __m128i c16_n25_n44 = _mm_set1_epi32(0xFFE7FFD4);

    const __m128i c16_n44_p25 = _mm_set1_epi32(0xFFD40019);
    const __m128i c16_p38_p09 = _mm_set1_epi32(0x00260009);
    const __m128i c16_n25_p09 = _mm_set1_epi32(0xFFE70009);
    const __m128i c16_n44_p38 = _mm_set1_epi32(0xFFD40026);

    const __m128i c16_p17_p42 = _mm_set1_epi32(0x0011002A);
    const __m128i c16_n42_p17 = _mm_set1_epi32(0xFFD60011);

    const __m128i c16_p32_p32 = _mm_set1_epi32(0x00200020);
    const __m128i c16_n32_p32 = _mm_set1_epi32(0xFFE00020);

    __m128i c32_off = _mm_set1_epi32(1 << (shift - 1));

    __m128i src00, src01, src02, src03, src04, src05, src06, src07, src08, src09, src10, src11, src12, src13, src14, src15;
    __m128i src16, src17, src18, src19, src20, src21, src22, src23, src24, src25, src26, src27, src28, src29, src30, src31;
    __m128i res00, res01, res02, res03, res04, res05, res06, res07, res08, res09, res10, res11, res12, res13, res14, res15;
    __m128i res16, res17, res18, res19, res20, res21, res22, res23, res24, res25, res26, res27, res28, res29, res30, res31;
    int i;

    for (i = 0; i < line; i += 4)
    {
        src00 = _mm_loadl_epi64((const __m128i*)&src[0 * i_src + i]);
        src01 = _mm_loadl_epi64((const __m128i*)&src[1 * i_src + i]);
        src02 = _mm_loadl_epi64((const __m128i*)&src[2 * i_src + i]);
        src03 = _mm_loadl_epi64((const __m128i*)&src[3 * i_src + i]);
        src04 = _mm_loadl_epi64((const __m128i*)&src[4 * i_src + i]);
        src05 = _mm_loadl_epi64((const __m128i*)&src[5 * i_src + i]);
        src06 = _mm_loadl_epi64((const __m128i*)&src[6 * i_src + i]);
        src07 = _mm_loadl_epi64((const __m128i*)&src[7 * i_src + i]);
        src08 = _mm_loadl_epi64((const __m128i*)&src[8 * i_src + i]);
        src09 = _mm_loadl_epi64((const __m128i*)&src[9 * i_src + i]);
        src10 = _mm_loadl_epi64((const __m128i*)&src[10 * i_src + i]);
        src11 = _mm_loadl_epi64((const __m128i*)&src[11 * i_src + i]);
        src12 = _mm_loadl_epi64((const __m128i*)&src[12 * i_src + i]);
        src13 = _mm_loadl_epi64((const __m128i*)&src[13 * i_src + i]);
        src14 = _mm_loadl_epi64((const __m128i*)&src[14 * i_src + i]);
        src15 = _mm_loadl_epi64((const __m128i*)&src[15 * i_src + i]);
        src16 = _mm_loadl_epi64((const __m128i*)&src[16 * i_src + i]);
        src17 = _mm_loadl_epi64((const __m128i*)&src[17 * i_src + i]);
        src18 = _mm_loadl_epi64((const __m128i*)&src[18 * i_src + i]);
        src19 = _mm_loadl_epi64((const __m128i*)&src[19 * i_src + i]);
        src20 = _mm_loadl_epi64((const __m128i*)&src[20 * i_src + i]);
        src21 = _mm_loadl_epi64((const __m128i*)&src[21 * i_src + i]);
        src22 = _mm_loadl_epi64((const __m128i*)&src[22 * i_src + i]);
        src23 = _mm_loadl_epi64((const __m128i*)&src[23 * i_src + i]);
        src24 = _mm_loadl_epi64((const __m128i*)&src[24 * i_src + i]);
        src25 = _mm_loadl_epi64((const __m128i*)&src[25 * i_src + i]);
        src26 = _mm_loadl_epi64((const __m128i*)&src[26 * i_src + i]);
        src27 = _mm_loadl_epi64((const __m128i*)&src[27 * i_src + i]);
        src28 = _mm_loadl_epi64((const __m128i*)&src[28 * i_src + i]);
        src29 = _mm_loadl_epi64((const __m128i*)&src[29 * i_src + i]);
        src30 = _mm_loadl_epi64((const __m128i*)&src[30 * i_src + i]);
        src31 = _mm_loadl_epi64((const __m128i*)&src[31 * i_src + i]);

        {
            const __m128i T_00_00A = _mm_unpacklo_epi16(src01, src03);       // [33 13 32 12 31 11 30 10]
            const __m128i T_00_01A = _mm_unpacklo_epi16(src05, src07);       // [ ]
            const __m128i T_00_02A = _mm_unpacklo_epi16(src09, src11);       // [ ]
            const __m128i T_00_03A = _mm_unpacklo_epi16(src13, src15);       // [ ]
            const __m128i T_00_04A = _mm_unpacklo_epi16(src17, src19);       // [ ]
            const __m128i T_00_05A = _mm_unpacklo_epi16(src21, src23);       // [ ]
            const __m128i T_00_06A = _mm_unpacklo_epi16(src25, src27);       // [ ]
            const __m128i T_00_07A = _mm_unpacklo_epi16(src29, src31);       //

            const __m128i T_00_08A = _mm_unpacklo_epi16(src02, src06);       // [ ]
            const __m128i T_00_09A = _mm_unpacklo_epi16(src10, src14);       // [ ]
            const __m128i T_00_10A = _mm_unpacklo_epi16(src18, src22);       // [ ]
            const __m128i T_00_11A = _mm_unpacklo_epi16(src26, src30);       // [ ]

            const __m128i T_00_12A = _mm_unpacklo_epi16(src04, src12);       // [ ]
            const __m128i T_00_13A = _mm_unpacklo_epi16(src20, src28);       // [ ]

            const __m128i T_00_14A = _mm_unpacklo_epi16(src08, src24);       //
            const __m128i T_00_15A = _mm_unpacklo_epi16(src00, src16);       //

            __m128i O00A, O01A, O02A, O03A, O04A, O05A, O06A, O07A, O08A, O09A, O10A, O11A, O12A, O13A, O14A, O15A;
            __m128i EO0A, EO1A, EO2A, EO3A, EO4A, EO5A, EO6A, EO7A;
            {
                __m128i t0, t1, t2, t3, t4, t5, t6, t7;
#define COMPUTE_ROW(r0103, r0507, r0911, r1315, r1719, r2123, r2527, r2931, c0103, c0507, c0911, c1315, c1719, c2123, c2527, c2931, row) \
                t0 = _mm_madd_epi16(r0103, c0103); \
                t1 = _mm_madd_epi16(r0507, c0507); \
                t2 = _mm_madd_epi16(r0911, c0911); \
                t3 = _mm_madd_epi16(r1315, c1315); \
                t4 = _mm_madd_epi16(r1719, c1719); \
                t5 = _mm_madd_epi16(r2123, c2123); \
                t6 = _mm_madd_epi16(r2527, c2527); \
                t7 = _mm_madd_epi16(r2931, c2931); \
	            t0 = _mm_add_epi32(t0, t1); \
	            t2 = _mm_add_epi32(t2, t3); \
	            t4 = _mm_add_epi32(t4, t5); \
	            t6 = _mm_add_epi32(t6, t7); \
	            row = _mm_add_epi32(_mm_add_epi32(t0, t2), _mm_add_epi32(t4, t6));

                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p45_p45, c16_p43_p44, c16_p39_p41, c16_p34_p36, c16_p27_p30, c16_p19_p23, c16_p11_p15, c16_p02_p07, O00A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p41_p45, c16_p23_p34, c16_n02_p11, c16_n27_n15, c16_n43_n36, c16_n44_n45, c16_n30_n39, c16_n07_n19, O01A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p34_p44, c16_n07_p15, c16_n41_n27, c16_n39_n45, c16_n02_n23, c16_p36_p19, c16_p43_p45, c16_p11_p30, O02A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p23_p43, c16_n34_n07, c16_n36_n45, c16_p19_n11, c16_p44_p41, c16_n02_p27, c16_n45_n30, c16_n15_n39, O03A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p11_p41, c16_n45_n27, c16_p07_n30, c16_p43_p39, c16_n23_p15, c16_n34_n45, c16_p36_p02, c16_p19_p44, O04A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n02_p39, c16_n36_n41, c16_p43_p07, c16_n11_p34, c16_n30_n44, c16_p45_p15, c16_n19_p27, c16_n23_n45, O05A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n15_p36, c16_n11_n45, c16_p34_p39, c16_n45_n19, c16_p41_n07, c16_n23_p30, c16_n02_n44, c16_p27_p43, O06A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n27_p34, c16_p19_n39, c16_n11_p43, c16_p02_n45, c16_p07_p45, c16_n15_n44, c16_p23_p41, c16_n30_n36, O07A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n36_p30, c16_p41_n23, c16_n44_p15, c16_p45_n07, c16_n45_n02, c16_p43_p11, c16_n39_n19, c16_p34_p27, O08A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n43_p27, c16_p44_n02, c16_n30_n23, c16_p07_p41, c16_p19_n45, c16_n39_p34, c16_p45_n11, c16_n36_n15, O09A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n45_p23, c16_p27_p19, c16_p15_n45, c16_n44_p30, c16_p34_p11, c16_p07_n43, c16_n41_p36, c16_p39_p02, O10A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n44_p19, c16_n02_p36, c16_p45_n34, c16_n15_n23, c16_n39_p43, c16_p30_p07, c16_p27_n45, c16_n41_p11, O11A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n39_p15, c16_n30_p45, c16_p27_p02, c16_p41_n44, c16_n11_n19, c16_n45_p36, c16_n07_p34, c16_p43_n23, O12A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n30_p11, c16_n45_p43, c16_n19_p36, c16_p23_n02, c16_p45_n39, c16_p27_n41, c16_n15_n07, c16_n44_p34, O13A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n19_p07, c16_n39_p30, c16_n45_p44, c16_n36_p43, c16_n15_p27, c16_p11_p02, c16_p34_n23, c16_p45_n41, O14A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n07_p02, c16_n15_p11, c16_n23_p19, c16_n30_p27, c16_n36_p34, c16_n41_p39, c16_n44_p43, c16_n45_p45, O15A)

#undef COMPUTE_ROW
            }

            {
                __m128i T00, T01;
#define COMPUTE_ROW(row0206, row1014, row1822, row2630, c0206, c1014, c1822, c2630, row) \
	            T00 = _mm_add_epi32(_mm_madd_epi16(row0206, c0206), _mm_madd_epi16(row1014, c1014)); \
	            T01 = _mm_add_epi32(_mm_madd_epi16(row1822, c1822), _mm_madd_epi16(row2630, c2630)); \
	            row = _mm_add_epi32(T00, T01);

                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p43_p45, c16_p35_p40, c16_p21_p29, c16_p04_p13, EO0A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p29_p43, c16_n21_p04, c16_n45_n40, c16_n13_n35, EO1A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p04_p40, c16_n43_n35, c16_p29_n13, c16_p21_p45, EO2A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n21_p35, c16_p04_n43, c16_p13_p45, c16_n29_n40, EO3A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n40_p29, c16_p45_n13, c16_n43_n04, c16_p35_p21, EO4A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n45_p21, c16_p13_p29, c16_p35_n43, c16_n40_p04, EO5A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n35_p13, c16_n40_p45, c16_p04_p21, c16_p43_n29, EO6A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n13_p04, c16_n29_p21, c16_n40_p35, c16_n45_p43, EO7A)

#undef COMPUTE_ROW
            }
            {
                const __m128i EEO0A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_p38_p44), _mm_madd_epi16(T_00_13A, c16_p09_p25));
                const __m128i EEO1A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n09_p38), _mm_madd_epi16(T_00_13A, c16_n25_n44));
                const __m128i EEO2A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n44_p25), _mm_madd_epi16(T_00_13A, c16_p38_p09));
                const __m128i EEO3A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n25_p09), _mm_madd_epi16(T_00_13A, c16_n44_p38));

                const __m128i EEEO0A = _mm_madd_epi16(T_00_14A, c16_p17_p42);
                const __m128i EEEO1A = _mm_madd_epi16(T_00_14A, c16_n42_p17);

                const __m128i EEEE0A = _mm_madd_epi16(T_00_15A, c16_p32_p32);
                const __m128i EEEE1A = _mm_madd_epi16(T_00_15A, c16_n32_p32);

                const __m128i EEE0A = _mm_add_epi32(EEEE0A, EEEO0A);            // EEE0 = EEEE0 + EEEO0
                const __m128i EEE1A = _mm_add_epi32(EEEE1A, EEEO1A);            // EEE1 = EEEE1 + EEEO1
                const __m128i EEE3A = _mm_sub_epi32(EEEE0A, EEEO0A);            // EEE2 = EEEE0 - EEEO0
                const __m128i EEE2A = _mm_sub_epi32(EEEE1A, EEEO1A);            // EEE3 = EEEE1 - EEEO1

                const __m128i EE0A = _mm_add_epi32(EEE0A, EEO0A);               // EE0 = EEE0 + EEO0
                const __m128i EE1A = _mm_add_epi32(EEE1A, EEO1A);               // EE1 = EEE1 + EEO1
                const __m128i EE2A = _mm_add_epi32(EEE2A, EEO2A);               // EE2 = EEE0 + EEO0
                const __m128i EE3A = _mm_add_epi32(EEE3A, EEO3A);               // EE3 = EEE1 + EEO1
                const __m128i EE7A = _mm_sub_epi32(EEE0A, EEO0A);               // EE7 = EEE0 - EEO0
                const __m128i EE6A = _mm_sub_epi32(EEE1A, EEO1A);               // EE6 = EEE1 - EEO1
                const __m128i EE5A = _mm_sub_epi32(EEE2A, EEO2A);               // EE5 = EEE0 - EEO0
                const __m128i EE4A = _mm_sub_epi32(EEE3A, EEO3A);               // EE4 = EEE1 - EEO1

                const __m128i E0A = _mm_add_epi32(EE0A, EO0A);                  // E0 = EE0 + EO0
                const __m128i E1A = _mm_add_epi32(EE1A, EO1A);                  // E1 = EE1 + EO1
                const __m128i E2A = _mm_add_epi32(EE2A, EO2A);                  // E2 = EE2 + EO2
                const __m128i E3A = _mm_add_epi32(EE3A, EO3A);                  // E3 = EE3 + EO3
                const __m128i E4A = _mm_add_epi32(EE4A, EO4A);                  // E4 =
                const __m128i E5A = _mm_add_epi32(EE5A, EO5A);                  // E5 =
                const __m128i E6A = _mm_add_epi32(EE6A, EO6A);                  // E6 =
                const __m128i E7A = _mm_add_epi32(EE7A, EO7A);                  // E7 =
                const __m128i EFA = _mm_sub_epi32(EE0A, EO0A);                  // EF = EE0 - EO0
                const __m128i EEA = _mm_sub_epi32(EE1A, EO1A);                  // EE = EE1 - EO1
                const __m128i EDA = _mm_sub_epi32(EE2A, EO2A);                  // ED = EE2 - EO2
                const __m128i ECA = _mm_sub_epi32(EE3A, EO3A);                  // EC = EE3 - EO3
                const __m128i EBA = _mm_sub_epi32(EE4A, EO4A);                  // EB =
                const __m128i EAA = _mm_sub_epi32(EE5A, EO5A);                  // EA =
                const __m128i E9A = _mm_sub_epi32(EE6A, EO6A);                  // E9 =
                const __m128i E8A = _mm_sub_epi32(EE7A, EO7A);                  // E8 =

                const __m128i T10A = _mm_add_epi32(E0A, c32_off);               // E0 + off
                const __m128i T11A = _mm_add_epi32(E1A, c32_off);               // E1 + off
                const __m128i T12A = _mm_add_epi32(E2A, c32_off);               // E2 + off
                const __m128i T13A = _mm_add_epi32(E3A, c32_off);               // E3 + off
                const __m128i T14A = _mm_add_epi32(E4A, c32_off);               // E4 + off
                const __m128i T15A = _mm_add_epi32(E5A, c32_off);               // E5 + off
                const __m128i T16A = _mm_add_epi32(E6A, c32_off);               // E6 + off
                const __m128i T17A = _mm_add_epi32(E7A, c32_off);               // E7 + off
                const __m128i T18A = _mm_add_epi32(E8A, c32_off);               // E8 + off
                const __m128i T19A = _mm_add_epi32(E9A, c32_off);               // E9 + off
                const __m128i T1AA = _mm_add_epi32(EAA, c32_off);               // E10 + off
                const __m128i T1BA = _mm_add_epi32(EBA, c32_off);               // E11 + off
                const __m128i T1CA = _mm_add_epi32(ECA, c32_off);               // E12 + off
                const __m128i T1DA = _mm_add_epi32(EDA, c32_off);               // E13 + off
                const __m128i T1EA = _mm_add_epi32(EEA, c32_off);               // E14 + off
                const __m128i T1FA = _mm_add_epi32(EFA, c32_off);               // E15 + off

                const __m128i T2_00A = _mm_add_epi32(T10A, O00A);               // E0 + O0 + off
                const __m128i T2_01A = _mm_add_epi32(T11A, O01A);               // E1 + O1 + off
                const __m128i T2_02A = _mm_add_epi32(T12A, O02A);               // E2 + O2 + off
                const __m128i T2_03A = _mm_add_epi32(T13A, O03A);               // E3 + O3 + off
                const __m128i T2_04A = _mm_add_epi32(T14A, O04A);               // E4
                const __m128i T2_05A = _mm_add_epi32(T15A, O05A);               // E5
                const __m128i T2_06A = _mm_add_epi32(T16A, O06A);               // E6
                const __m128i T2_07A = _mm_add_epi32(T17A, O07A);               // E7
                const __m128i T2_08A = _mm_add_epi32(T18A, O08A);               // E8
                const __m128i T2_09A = _mm_add_epi32(T19A, O09A);               // E9
                const __m128i T2_10A = _mm_add_epi32(T1AA, O10A);               // E10
                const __m128i T2_11A = _mm_add_epi32(T1BA, O11A);               // E11
                const __m128i T2_12A = _mm_add_epi32(T1CA, O12A);               // E12
                const __m128i T2_13A = _mm_add_epi32(T1DA, O13A);               // E13
                const __m128i T2_14A = _mm_add_epi32(T1EA, O14A);               // E14
                const __m128i T2_15A = _mm_add_epi32(T1FA, O15A);               // E15
                const __m128i T2_31A = _mm_sub_epi32(T10A, O00A);               // E0 - O0 + off
                const __m128i T2_30A = _mm_sub_epi32(T11A, O01A);               // E1 - O1 + off
                const __m128i T2_29A = _mm_sub_epi32(T12A, O02A);               // E2 - O2 + off
                const __m128i T2_28A = _mm_sub_epi32(T13A, O03A);               // E3 - O3 + off
                const __m128i T2_27A = _mm_sub_epi32(T14A, O04A);               // E4
                const __m128i T2_26A = _mm_sub_epi32(T15A, O05A);               // E5
                const __m128i T2_25A = _mm_sub_epi32(T16A, O06A);               // E6
                const __m128i T2_24A = _mm_sub_epi32(T17A, O07A);               // E7
                const __m128i T2_23A = _mm_sub_epi32(T18A, O08A); 
                const __m128i T2_22A = _mm_sub_epi32(T19A, O09A); 
                const __m128i T2_21A = _mm_sub_epi32(T1AA, O10A); 
                const __m128i T2_20A = _mm_sub_epi32(T1BA, O11A); 
                const __m128i T2_19A = _mm_sub_epi32(T1CA, O12A); 
                const __m128i T2_18A = _mm_sub_epi32(T1DA, O13A); 
                const __m128i T2_17A = _mm_sub_epi32(T1EA, O14A); 
                const __m128i T2_16A = _mm_sub_epi32(T1FA, O15A); 

                const __m128i T3_00A = _mm_srai_epi32(T2_00A, shift);             // [30 20 10 00]
                const __m128i T3_01A = _mm_srai_epi32(T2_01A, shift);             // [31 21 11 01]
                const __m128i T3_02A = _mm_srai_epi32(T2_02A, shift);             // [32 22 12 02]
                const __m128i T3_03A = _mm_srai_epi32(T2_03A, shift);             // [33 23 13 03]
                const __m128i T3_04A = _mm_srai_epi32(T2_04A, shift);             // [33 24 14 04]
                const __m128i T3_05A = _mm_srai_epi32(T2_05A, shift);             // [35 25 15 05]
                const __m128i T3_06A = _mm_srai_epi32(T2_06A, shift);             // [36 26 16 06]
                const __m128i T3_07A = _mm_srai_epi32(T2_07A, shift);             // [37 27 17 07]
                const __m128i T3_08A = _mm_srai_epi32(T2_08A, shift);             // [30 20 10 00] x8
                const __m128i T3_09A = _mm_srai_epi32(T2_09A, shift);             // [31 21 11 01] x9
                const __m128i T3_10A = _mm_srai_epi32(T2_10A, shift);             // [32 22 12 02] xA
                const __m128i T3_11A = _mm_srai_epi32(T2_11A, shift);             // [33 23 13 03] xB
                const __m128i T3_12A = _mm_srai_epi32(T2_12A, shift);             // [33 24 14 04] xC
                const __m128i T3_13A = _mm_srai_epi32(T2_13A, shift);             // [35 25 15 05] xD
                const __m128i T3_14A = _mm_srai_epi32(T2_14A, shift);             // [36 26 16 06] xE
                const __m128i T3_15A = _mm_srai_epi32(T2_15A, shift);             // [37 27 17 07] xF

                const __m128i T3_16A = _mm_srai_epi32(T2_16A, shift);             // [30 20 10 00]
                const __m128i T3_17A = _mm_srai_epi32(T2_17A, shift);             // [31 21 11 01]
                const __m128i T3_18A = _mm_srai_epi32(T2_18A, shift);             // [32 22 12 02]
                const __m128i T3_19A = _mm_srai_epi32(T2_19A, shift);             // [33 23 13 03]
                const __m128i T3_20A = _mm_srai_epi32(T2_20A, shift);             // [33 24 14 04]
                const __m128i T3_21A = _mm_srai_epi32(T2_21A, shift);             // [35 25 15 05]
                const __m128i T3_22A = _mm_srai_epi32(T2_22A, shift);             // [36 26 16 06]
                const __m128i T3_23A = _mm_srai_epi32(T2_23A, shift);             // [37 27 17 07]
                const __m128i T3_24A = _mm_srai_epi32(T2_24A, shift);             // [30 20 10 00] x8
                const __m128i T3_25A = _mm_srai_epi32(T2_25A, shift);             // [31 21 11 01] x9
                const __m128i T3_26A = _mm_srai_epi32(T2_26A, shift);             // [32 22 12 02] xA
                const __m128i T3_27A = _mm_srai_epi32(T2_27A, shift);             // [33 23 13 03] xB
                const __m128i T3_28A = _mm_srai_epi32(T2_28A, shift);             // [33 24 14 04] xC
                const __m128i T3_29A = _mm_srai_epi32(T2_29A, shift);             // [35 25 15 05] xD
                const __m128i T3_30A = _mm_srai_epi32(T2_30A, shift);             // [36 26 16 06] xE
                const __m128i T3_31A = _mm_srai_epi32(T2_31A, shift);             // [37 27 17 07] xF
                const __m128i zero = _mm_setzero_si128();

                res00 = _mm_packs_epi32(T3_00A, zero);        // [70 60 50 40 30 20 10 00]
                res01 = _mm_packs_epi32(T3_01A, zero);        // [71 61 51 41 31 21 11 01]
                res02 = _mm_packs_epi32(T3_02A, zero);        // [72 62 52 42 32 22 12 02]
                res03 = _mm_packs_epi32(T3_03A, zero);        // [73 63 53 43 33 23 13 03]
                res04 = _mm_packs_epi32(T3_04A, zero);        // [74 64 54 44 34 24 14 04]
                res05 = _mm_packs_epi32(T3_05A, zero);        // [75 65 55 45 35 25 15 05]
                res06 = _mm_packs_epi32(T3_06A, zero);        // [76 66 56 46 36 26 16 06]
                res07 = _mm_packs_epi32(T3_07A, zero);        // [77 67 57 47 37 27 17 07]
                res08 = _mm_packs_epi32(T3_08A, zero);        // [A0 ... 80]
                res09 = _mm_packs_epi32(T3_09A, zero);        // [A1 ... 81]
                res10 = _mm_packs_epi32(T3_10A, zero);        // [A2 ... 82]
                res11 = _mm_packs_epi32(T3_11A, zero);        // [A3 ... 83]
                res12 = _mm_packs_epi32(T3_12A, zero);        // [A4 ... 84]
                res13 = _mm_packs_epi32(T3_13A, zero);        // [A5 ... 85]
                res14 = _mm_packs_epi32(T3_14A, zero);        // [A6 ... 86]
                res15 = _mm_packs_epi32(T3_15A, zero);        // [A7 ... 87]
                res16 = _mm_packs_epi32(T3_16A, zero);
                res17 = _mm_packs_epi32(T3_17A, zero);
                res18 = _mm_packs_epi32(T3_18A, zero);
                res19 = _mm_packs_epi32(T3_19A, zero);
                res20 = _mm_packs_epi32(T3_20A, zero);
                res21 = _mm_packs_epi32(T3_21A, zero);
                res22 = _mm_packs_epi32(T3_22A, zero);
                res23 = _mm_packs_epi32(T3_23A, zero);
                res24 = _mm_packs_epi32(T3_24A, zero);
                res25 = _mm_packs_epi32(T3_25A, zero);
                res26 = _mm_packs_epi32(T3_26A, zero);
                res27 = _mm_packs_epi32(T3_27A, zero);
                res28 = _mm_packs_epi32(T3_28A, zero);
                res29 = _mm_packs_epi32(T3_29A, zero);
                res30 = _mm_packs_epi32(T3_30A, zero);
                res31 = _mm_packs_epi32(T3_31A, zero);
            }
            //transpose 32x4 -> 4x32.
            {
                __m128i tr0_0, tr0_1, tr0_2, tr0_3;
                __m128i tr1_0, tr1_1, tr1_2, tr1_3;

                TRANSPOSE_8x4_16BIT(res00, res01, res02, res03, res04, res05, res06, res07, src00, src04, src08, src12)
                TRANSPOSE_8x4_16BIT(res08, res09, res10, res11, res12, res13, res14, res15, src01, src05, src09, src13)
                TRANSPOSE_8x4_16BIT(res16, res17, res18, res19, res20, res21, res22, res23, src02, src06, src10, src14)
                TRANSPOSE_8x4_16BIT(res24, res25, res26, res27, res28, res29, res30, res31, src03, src07, src11, src15)

            }
            if (bit_depth != MAX_TX_DYNAMIC_RANGE) { // clip
                __m128i max_val = _mm_set1_epi16((1 << bit_depth) - 1);
                __m128i min_val = _mm_set1_epi16(-(1 << bit_depth));

                src00 = _mm_min_epi16(src00, max_val);
                src01 = _mm_min_epi16(src01, max_val);
                src02 = _mm_min_epi16(src02, max_val);
                src03 = _mm_min_epi16(src03, max_val);
                src00 = _mm_max_epi16(src00, min_val);
                src01 = _mm_max_epi16(src01, min_val);
                src02 = _mm_max_epi16(src02, min_val);
                src03 = _mm_max_epi16(src03, min_val);

                src04 = _mm_min_epi16(src04, max_val);
                src05 = _mm_min_epi16(src05, max_val);
                src06 = _mm_min_epi16(src06, max_val);
                src07 = _mm_min_epi16(src07, max_val);
                src04 = _mm_max_epi16(src04, min_val);
                src05 = _mm_max_epi16(src05, min_val);
                src06 = _mm_max_epi16(src06, min_val);
                src07 = _mm_max_epi16(src07, min_val);

                src08 = _mm_min_epi16(src08, max_val);
                src09 = _mm_min_epi16(src09, max_val);
                src10 = _mm_min_epi16(src10, max_val);
                src11 = _mm_min_epi16(src11, max_val);
                src08 = _mm_max_epi16(src08, min_val);
                src09 = _mm_max_epi16(src09, min_val);
                src10 = _mm_max_epi16(src10, min_val);
                src11 = _mm_max_epi16(src11, min_val);

                src12 = _mm_min_epi16(src12, max_val);
                src13 = _mm_min_epi16(src13, max_val);
                src14 = _mm_min_epi16(src14, max_val);
                src15 = _mm_min_epi16(src15, max_val);
                src12 = _mm_max_epi16(src12, min_val);
                src13 = _mm_max_epi16(src13, min_val);
                src14 = _mm_max_epi16(src14, min_val);
                src15 = _mm_max_epi16(src15, min_val);

            }
        }
        _mm_store_si128((__m128i*)&dst[0 ], src00);
        _mm_store_si128((__m128i*)&dst[8 ], src01);
        _mm_store_si128((__m128i*)&dst[16], src02);
        _mm_store_si128((__m128i*)&dst[24], src03);
        _mm_store_si128((__m128i*)&dst[32], src04);
        _mm_store_si128((__m128i*)&dst[40], src05);
        _mm_store_si128((__m128i*)&dst[48], src06);
        _mm_store_si128((__m128i*)&dst[56], src07);
        _mm_store_si128((__m128i*)&dst[64], src08);
        _mm_store_si128((__m128i*)&dst[72], src09);
        _mm_store_si128((__m128i*)&dst[80], src10);
        _mm_store_si128((__m128i*)&dst[88], src11);
        _mm_store_si128((__m128i*)&dst[96], src12);
        _mm_store_si128((__m128i*)&dst[104], src13);
        _mm_store_si128((__m128i*)&dst[112], src14);
        _mm_store_si128((__m128i*)&dst[120], src15);

        dst += 4 * 32;
    }
}

void dct2_butterfly_h64_sse(s16* src, int i_src, s16* dst, int line, int shift, int bit_depth)
{
    // O[32] coeffs
    const __m128i c16_p45_p45 = _mm_set1_epi32(0x002d002d);
    const __m128i c16_p44_p44 = _mm_set1_epi32(0x002c002c);
    const __m128i c16_p42_p43 = _mm_set1_epi32(0x002a002b);
    const __m128i c16_p40_p41 = _mm_set1_epi32(0x00280029);
    const __m128i c16_p38_p39 = _mm_set1_epi32(0x00260027);
    const __m128i c16_p36_p37 = _mm_set1_epi32(0x00240025);
    const __m128i c16_p33_p34 = _mm_set1_epi32(0x00210022);
    const __m128i c16_p44_p45 = _mm_set1_epi32(0x002c002d);
    const __m128i c16_p39_p42 = _mm_set1_epi32(0x0027002a);
    const __m128i c16_p31_p36 = _mm_set1_epi32(0x001f0024);
    const __m128i c16_p20_p26 = _mm_set1_epi32(0x0014001a);
    const __m128i c16_p08_p14 = _mm_set1_epi32(0x0008000e);
    const __m128i c16_n06_p01 = _mm_set1_epi32(0xfffa0001);
    const __m128i c16_n18_n12 = _mm_set1_epi32(0xffeefff4);
    const __m128i c16_n30_n24 = _mm_set1_epi32(0xffe2ffe8);
    const __m128i c16_p42_p45 = _mm_set1_epi32(0x002a002d);
    const __m128i c16_p30_p37 = _mm_set1_epi32(0x001e0025);
    const __m128i c16_p10_p20 = _mm_set1_epi32(0x000a0014);
    const __m128i c16_n12_n01 = _mm_set1_epi32(0xfff4ffff);
    const __m128i c16_n31_n22 = _mm_set1_epi32(0xffe1ffea);
    const __m128i c16_n43_n38 = _mm_set1_epi32(0xffd5ffda);
    const __m128i c16_n45_n45 = _mm_set1_epi32(0xffd3ffd3);
    const __m128i c16_n36_n41 = _mm_set1_epi32(0xffdcffd7);
    const __m128i c16_p39_p45 = _mm_set1_epi32(0x0027002d);
    const __m128i c16_p16_p30 = _mm_set1_epi32(0x0010001e);
    const __m128i c16_n14_p01 = _mm_set1_epi32(0xfff20001);
    const __m128i c16_n38_n28 = _mm_set1_epi32(0xffdaffe4);
    const __m128i c16_n45_n44 = _mm_set1_epi32(0xffd3ffd4);
    const __m128i c16_n31_n40 = _mm_set1_epi32(0xffe1ffd8);
    const __m128i c16_n03_n18 = _mm_set1_epi32(0xfffdffee);
    const __m128i c16_p26_p12 = _mm_set1_epi32(0x001a000c);
    const __m128i c16_p36_p44 = _mm_set1_epi32(0x0024002c);
    const __m128i c16_p01_p20 = _mm_set1_epi32(0x00010014);
    const __m128i c16_n34_n18 = _mm_set1_epi32(0xffdeffee);
    const __m128i c16_n22_n37 = _mm_set1_epi32(0xffeaffdb);
    const __m128i c16_p16_n03 = _mm_set1_epi32(0x0010fffd);
    const __m128i c16_p43_p33 = _mm_set1_epi32(0x002b0021);
    const __m128i c16_p38_p45 = _mm_set1_epi32(0x0026002d);
    const __m128i c16_p31_p44 = _mm_set1_epi32(0x001f002c);
    const __m128i c16_n14_p10 = _mm_set1_epi32(0xfff2000a);
    const __m128i c16_n45_n34 = _mm_set1_epi32(0xffd3ffde);
    const __m128i c16_n28_n42 = _mm_set1_epi32(0xffe4ffd6);
    const __m128i c16_p18_n06 = _mm_set1_epi32(0x0012fffa);
    const __m128i c16_p45_p37 = _mm_set1_epi32(0x002d0025);
    const __m128i c16_p24_p40 = _mm_set1_epi32(0x00180028);
    const __m128i c16_n22_p01 = _mm_set1_epi32(0xffea0001);
    const __m128i c16_p26_p43 = _mm_set1_epi32(0x001a002b);
    const __m128i c16_n28_n01 = _mm_set1_epi32(0xffe4ffff);
    const __m128i c16_n42_n44 = _mm_set1_epi32(0xffd6ffd4);
    const __m128i c16_p03_n24 = _mm_set1_epi32(0x0003ffe8);
    const __m128i c16_p44_p30 = _mm_set1_epi32(0x002c001e);
    const __m128i c16_p22_p41 = _mm_set1_epi32(0x00160029);
    const __m128i c16_n31_n06 = _mm_set1_epi32(0xffe1fffa);
    const __m128i c16_n40_n45 = _mm_set1_epi32(0xffd8ffd3);
    const __m128i c16_p20_p42 = _mm_set1_epi32(0x0014002a);
    const __m128i c16_n38_n12 = _mm_set1_epi32(0xffdafff4);
    const __m128i c16_n28_n45 = _mm_set1_epi32(0xffe4ffd3);
    const __m128i c16_p33_p03 = _mm_set1_epi32(0x00210003);
    const __m128i c16_p34_p45 = _mm_set1_epi32(0x0022002d);
    const __m128i c16_n26_p06 = _mm_set1_epi32(0xffe60006);
    const __m128i c16_n39_n44 = _mm_set1_epi32(0xffd9ffd4);
    const __m128i c16_p18_n14 = _mm_set1_epi32(0x0012fff2);
    const __m128i c16_p14_p41 = _mm_set1_epi32(0x000e0029);
    const __m128i c16_n44_n22 = _mm_set1_epi32(0xffd4ffea);
    const __m128i c16_n06_n37 = _mm_set1_epi32(0xfffaffdb);
    const __m128i c16_p45_p30 = _mm_set1_epi32(0x002d001e);
    const __m128i c16_n03_p31 = _mm_set1_epi32(0xfffd001f);
    const __m128i c16_n45_n36 = _mm_set1_epi32(0xffd3ffdc);
    const __m128i c16_p12_n24 = _mm_set1_epi32(0x000cffe8);
    const __m128i c16_p42_p40 = _mm_set1_epi32(0x002a0028);
    const __m128i c16_p08_p40 = _mm_set1_epi32(0x00080028);
    const __m128i c16_n45_n31 = _mm_set1_epi32(0xffd3ffe1);
    const __m128i c16_p18_n22 = _mm_set1_epi32(0x0012ffea);
    const __m128i c16_p34_p44 = _mm_set1_epi32(0x0022002c);
    const __m128i c16_n38_n03 = _mm_set1_epi32(0xffdafffd);
    const __m128i c16_n12_n42 = _mm_set1_epi32(0xfff4ffd6);
    const __m128i c16_p45_p28 = _mm_set1_epi32(0x002d001c);
    const __m128i c16_n14_p26 = _mm_set1_epi32(0xfff2001a);
    const __m128i c16_p01_p39 = _mm_set1_epi32(0x00010027);
    const __m128i c16_n40_n38 = _mm_set1_epi32(0xffd8ffda);
    const __m128i c16_p37_n03 = _mm_set1_epi32(0x0025fffd);
    const __m128i c16_p06_p41 = _mm_set1_epi32(0x00060029);
    const __m128i c16_n42_n36 = _mm_set1_epi32(0xffd6ffdc);
    const __m128i c16_p34_n08 = _mm_set1_epi32(0x0022fff8);
    const __m128i c16_p10_p43 = _mm_set1_epi32(0x000a002b);
    const __m128i c16_n44_n33 = _mm_set1_epi32(0xffd4ffdf);
    const __m128i c16_n06_p38 = _mm_set1_epi32(0xfffa0026);
    const __m128i c16_n31_n43 = _mm_set1_epi32(0xffe1ffd5);
    const __m128i c16_p45_p16 = _mm_set1_epi32(0x002d0010);
    const __m128i c16_n26_p22 = _mm_set1_epi32(0xffe60016);
    const __m128i c16_n12_n45 = _mm_set1_epi32(0xfff4ffd3);
    const __m128i c16_p41_p34 = _mm_set1_epi32(0x00290022);
    const __m128i c16_n40_p01 = _mm_set1_epi32(0xffd80001);
    const __m128i c16_p10_n36 = _mm_set1_epi32(0x000affdc);
    const __m128i c16_n12_p37 = _mm_set1_epi32(0xfff40025);
    const __m128i c16_n18_n45 = _mm_set1_epi32(0xffeeffd3);
    const __m128i c16_p40_p33 = _mm_set1_epi32(0x00280021);
    const __m128i c16_n44_n06 = _mm_set1_epi32(0xffd4fffa);
    const __m128i c16_p28_n24 = _mm_set1_epi32(0x001cffe8);
    const __m128i c16_p01_p43 = _mm_set1_epi32(0x0001002b);
    const __m128i c16_n30_n42 = _mm_set1_epi32(0xffe2ffd6);
    const __m128i c16_p45_p22 = _mm_set1_epi32(0x002d0016);
    const __m128i c16_n18_p36 = _mm_set1_epi32(0xffee0024);
    const __m128i c16_n03_n45 = _mm_set1_epi32(0xfffdffd3);
    const __m128i c16_p24_p43 = _mm_set1_epi32(0x0018002b);
    const __m128i c16_n39_n31 = _mm_set1_epi32(0xffd9ffe1);
    const __m128i c16_p45_p12 = _mm_set1_epi32(0x002d000c);
    const __m128i c16_n40_p10 = _mm_set1_epi32(0xffd8000a);
    const __m128i c16_p26_n30 = _mm_set1_epi32(0x001affe2);
    const __m128i c16_n06_p42 = _mm_set1_epi32(0xfffa002a);
    const __m128i c16_n24_p34 = _mm_set1_epi32(0xffe80022);
    const __m128i c16_p12_n41 = _mm_set1_epi32(0x000cffd7);
    const __m128i c16_p01_p45 = _mm_set1_epi32(0x0001002d);
    const __m128i c16_n14_n45 = _mm_set1_epi32(0xfff2ffd3);
    const __m128i c16_p26_p40 = _mm_set1_epi32(0x001a0028);
    const __m128i c16_n36_n33 = _mm_set1_epi32(0xffdcffdf);
    const __m128i c16_p42_p22 = _mm_set1_epi32(0x002a0016);
    const __m128i c16_n45_n10 = _mm_set1_epi32(0xffd3fff6);
    const __m128i c16_n30_p33 = _mm_set1_epi32(0xffe20021);
    const __m128i c16_p26_n36 = _mm_set1_epi32(0x001affdc);
    const __m128i c16_n22_p38 = _mm_set1_epi32(0xffea0026);
    const __m128i c16_p18_n40 = _mm_set1_epi32(0x0012ffd8);
    const __m128i c16_n14_p42 = _mm_set1_epi32(0xfff2002a);
    const __m128i c16_p10_n44 = _mm_set1_epi32(0x000affd4);
    const __m128i c16_n06_p45 = _mm_set1_epi32(0xfffa002d);
    const __m128i c16_p01_n45 = _mm_set1_epi32(0x0001ffd3);
    const __m128i c16_n34_p31 = _mm_set1_epi32(0xffde001f);
    const __m128i c16_p37_n28 = _mm_set1_epi32(0x0025ffe4);
    const __m128i c16_n39_p24 = _mm_set1_epi32(0xffd90018);
    const __m128i c16_p41_n20 = _mm_set1_epi32(0x0029ffec);
    const __m128i c16_n43_p16 = _mm_set1_epi32(0xffd50010);
    const __m128i c16_p44_n12 = _mm_set1_epi32(0x002cfff4);
    const __m128i c16_n45_p08 = _mm_set1_epi32(0xffd30008);
    const __m128i c16_p45_n03 = _mm_set1_epi32(0x002dfffd);
    const __m128i c16_n38_p30 = _mm_set1_epi32(0xffda001e);
    const __m128i c16_p44_n18 = _mm_set1_epi32(0x002cffee);
    const __m128i c16_n45_p06 = _mm_set1_epi32(0xffd30006);
    const __m128i c16_p43_p08 = _mm_set1_epi32(0x002b0008);
    const __m128i c16_n37_n20 = _mm_set1_epi32(0xffdbffec);
    const __m128i c16_p28_p31 = _mm_set1_epi32(0x001c001f);
    const __m128i c16_n16_n39 = _mm_set1_epi32(0xfff0ffd9);
    const __m128i c16_p03_p44 = _mm_set1_epi32(0x0003002c);
    const __m128i c16_n41_p28 = _mm_set1_epi32(0xffd7001c);
    const __m128i c16_p45_n08 = _mm_set1_epi32(0x002dfff8);
    const __m128i c16_n38_n14 = _mm_set1_epi32(0xffdafff2);
    const __m128i c16_p22_p33 = _mm_set1_epi32(0x00160021);
    const __m128i c16_n01_n44 = _mm_set1_epi32(0xffffffd4);
    const __m128i c16_n20_p44 = _mm_set1_epi32(0xffec002c);
    const __m128i c16_p37_n34 = _mm_set1_epi32(0x0025ffde);
    const __m128i c16_n45_p16 = _mm_set1_epi32(0xffd30010);
    const __m128i c16_n44_p26 = _mm_set1_epi32(0xffd4001a);
    const __m128i c16_p41_p03 = _mm_set1_epi32(0x00290003);
    const __m128i c16_n20_n31 = _mm_set1_epi32(0xffecffe1);
    const __m128i c16_n10_p45 = _mm_set1_epi32(0xfff6002d);
    const __m128i c16_p36_n38 = _mm_set1_epi32(0x0024ffda);
    const __m128i c16_n45_p14 = _mm_set1_epi32(0xffd3000e);
    const __m128i c16_p34_p16 = _mm_set1_epi32(0x00220010);
    const __m128i c16_n08_n39 = _mm_set1_epi32(0xfff8ffd9);
    const __m128i c16_n45_p24 = _mm_set1_epi32(0xffd30018);
    const __m128i c16_p33_p14 = _mm_set1_epi32(0x0021000e);
    const __m128i c16_p03_n42 = _mm_set1_epi32(0x0003ffd6);
    const __m128i c16_n37_p39 = _mm_set1_epi32(0xffdb0027);
    const __m128i c16_p44_n08 = _mm_set1_epi32(0x002cfff8);
    const __m128i c16_n18_n30 = _mm_set1_epi32(0xffeeffe2);
    const __m128i c16_n20_p45 = _mm_set1_epi32(0xffec002d);
    const __m128i c16_p44_n28 = _mm_set1_epi32(0x002cffe4);
    const __m128i c16_n45_p22 = _mm_set1_epi32(0xffd30016);
    const __m128i c16_p20_p24 = _mm_set1_epi32(0x00140018);
    const __m128i c16_p26_n45 = _mm_set1_epi32(0x001affd3);
    const __m128i c16_n45_p18 = _mm_set1_epi32(0xffd30012);
    const __m128i c16_p16_p28 = _mm_set1_epi32(0x0010001c);
    const __m128i c16_p30_n45 = _mm_set1_epi32(0x001effd3);
    const __m128i c16_n44_p14 = _mm_set1_epi32(0xffd4000e);
    const __m128i c16_p12_p31 = _mm_set1_epi32(0x000c001f);
    const __m128i c16_n45_p20 = _mm_set1_epi32(0xffd30014);
    const __m128i c16_p06_p33 = _mm_set1_epi32(0x00060021);
    const __m128i c16_p41_n39 = _mm_set1_epi32(0x0029ffd9);
    const __m128i c16_n30_n10 = _mm_set1_epi32(0xffe2fff6);
    const __m128i c16_n24_p45 = _mm_set1_epi32(0xffe8002d);
    const __m128i c16_p44_n16 = _mm_set1_epi32(0x002cfff0);
    const __m128i c16_n01_n36 = _mm_set1_epi32(0xffffffdc);
    const __m128i c16_n43_p37 = _mm_set1_epi32(0xffd50025);
    const __m128i c16_n43_p18 = _mm_set1_epi32(0xffd50012);
    const __m128i c16_n10_p39 = _mm_set1_epi32(0xfff60027);
    const __m128i c16_p45_n26 = _mm_set1_epi32(0x002dffe6);
    const __m128i c16_p01_n34 = _mm_set1_epi32(0x0001ffde);
    const __m128i c16_n45_p33 = _mm_set1_epi32(0xffd30021);
    const __m128i c16_p08_p28 = _mm_set1_epi32(0x0008001c);
    const __m128i c16_p44_n38 = _mm_set1_epi32(0x002cffda);
    const __m128i c16_n16_n20 = _mm_set1_epi32(0xfff0ffec);
    const __m128i c16_n40_p16 = _mm_set1_epi32(0xffd80010);
    const __m128i c16_n24_p44 = _mm_set1_epi32(0xffe8002c);
    const __m128i c16_p36_n08 = _mm_set1_epi32(0x0024fff8);
    const __m128i c16_p31_n45 = _mm_set1_epi32(0x001fffd3);
    const __m128i c16_n30_n01 = _mm_set1_epi32(0xffe2ffff);
    const __m128i c16_n37_p45 = _mm_set1_epi32(0xffdb002d);
    const __m128i c16_p22_p10 = _mm_set1_epi32(0x0016000a);
    const __m128i c16_p41_n43 = _mm_set1_epi32(0x0029ffd5);
    const __m128i c16_n37_p14 = _mm_set1_epi32(0xffdb000e);
    const __m128i c16_n36_p45 = _mm_set1_epi32(0xffdc002d);
    const __m128i c16_p16_p12 = _mm_set1_epi32(0x0010000c);
    const __m128i c16_p45_n38 = _mm_set1_epi32(0x002dffda);
    const __m128i c16_p10_n34 = _mm_set1_epi32(0x000affde);
    const __m128i c16_n39_p18 = _mm_set1_epi32(0xffd90012);
    const __m128i c16_n33_p45 = _mm_set1_epi32(0xffdf002d);
    const __m128i c16_p20_p08 = _mm_set1_epi32(0x00140008);
    const __m128i c16_n33_p12 = _mm_set1_epi32(0xffdf000c);
    const __m128i c16_n43_p44 = _mm_set1_epi32(0xffd5002c);
    const __m128i c16_n08_p30 = _mm_set1_epi32(0xfff8001e);
    const __m128i c16_p36_n16 = _mm_set1_epi32(0x0024fff0);
    const __m128i c16_p41_n45 = _mm_set1_epi32(0x0029ffd3);
    const __m128i c16_p03_n26 = _mm_set1_epi32(0x0003ffe6);
    const __m128i c16_n38_p20 = _mm_set1_epi32(0xffda0014);
    const __m128i c16_n39_p45 = _mm_set1_epi32(0xffd9002d);
    const __m128i c16_n28_p10 = _mm_set1_epi32(0xffe4000a);
    const __m128i c16_n45_p40 = _mm_set1_epi32(0xffd30028);
    const __m128i c16_n30_p41 = _mm_set1_epi32(0xffe20029);
    const __m128i c16_p08_p12 = _mm_set1_epi32(0x0008000c);
    const __m128i c16_p39_n26 = _mm_set1_epi32(0x0027ffe6);
    const __m128i c16_p42_n45 = _mm_set1_epi32(0x002affd3);
    const __m128i c16_p14_n31 = _mm_set1_epi32(0x000effe1);
    const __m128i c16_n24_p06 = _mm_set1_epi32(0xffe80006);
    const __m128i c16_n22_p08 = _mm_set1_epi32(0xffea0008);
    const __m128i c16_n42_p34 = _mm_set1_epi32(0xffd60022);
    const __m128i c16_n43_p45 = _mm_set1_epi32(0xffd5002d);
    const __m128i c16_n24_p36 = _mm_set1_epi32(0xffe80024);
    const __m128i c16_p06_p10 = _mm_set1_epi32(0x0006000a);
    const __m128i c16_p33_n20 = _mm_set1_epi32(0x0021ffec);
    const __m128i c16_p45_n41 = _mm_set1_epi32(0x002dffd7);
    const __m128i c16_p37_n44 = _mm_set1_epi32(0x0025ffd4);
    const __m128i c16_n16_p06 = _mm_set1_epi32(0xfff00006);
    const __m128i c16_n34_p26 = _mm_set1_epi32(0xffde001a);
    const __m128i c16_n44_p40 = _mm_set1_epi32(0xffd40028);
    const __m128i c16_n44_p45 = _mm_set1_epi32(0xffd4002d);
    const __m128i c16_n33_p39 = _mm_set1_epi32(0xffdf0027);
    const __m128i c16_n14_p24 = _mm_set1_epi32(0xfff20018);
    const __m128i c16_p08_p03 = _mm_set1_epi32(0x00080003);
    const __m128i c16_p28_n18 = _mm_set1_epi32(0x001cffee);
    const __m128i c16_n10_p03 = _mm_set1_epi32(0xfff60003);
    const __m128i c16_n22_p16 = _mm_set1_epi32(0xffea0010);
    const __m128i c16_n33_p28 = _mm_set1_epi32(0xffdf001c);
    const __m128i c16_n40_p37 = _mm_set1_epi32(0xffd80025);
    const __m128i c16_n45_p43 = _mm_set1_epi32(0xffd3002b);
    const __m128i c16_n45_p45 = _mm_set1_epi32(0xffd3002d);
    const __m128i c16_n41_p44 = _mm_set1_epi32(0xffd7002c);
    const __m128i c16_n34_p38 = _mm_set1_epi32(0xffde0026);
    const __m128i c16_n03_p01 = _mm_set1_epi32(0xfffd0001);
    const __m128i c16_n08_p06 = _mm_set1_epi32(0xfff80006);
    const __m128i c16_n12_p10 = _mm_set1_epi32(0xfff4000a);
    const __m128i c16_n16_p14 = _mm_set1_epi32(0xfff0000e);
    const __m128i c16_n20_p18 = _mm_set1_epi32(0xffec0012);
    const __m128i c16_n24_p22 = _mm_set1_epi32(0xffe80016);
    const __m128i c16_n28_p26 = _mm_set1_epi32(0xffe4001a);
    const __m128i c16_n31_p30 = _mm_set1_epi32(0xffe1001e);

    // EO[16] coeffs
    const __m128i c16_p43_p44 = _mm_set1_epi32(0x002b002c);
    const __m128i c16_p39_p41 = _mm_set1_epi32(0x00270029);
    const __m128i c16_p34_p36 = _mm_set1_epi32(0x00220024);
    const __m128i c16_p41_p45 = _mm_set1_epi32(0x0029002d);
    const __m128i c16_p23_p34 = _mm_set1_epi32(0x00170022);
    const __m128i c16_n02_p11 = _mm_set1_epi32(0xfffe000b);
    const __m128i c16_n27_n15 = _mm_set1_epi32(0xffe5fff1);
    const __m128i c16_n07_p15 = _mm_set1_epi32(0xfff9000f);
    const __m128i c16_n41_n27 = _mm_set1_epi32(0xffd7ffe5);
    const __m128i c16_n39_n45 = _mm_set1_epi32(0xffd9ffd3);
    const __m128i c16_p23_p43 = _mm_set1_epi32(0x0017002b);
    const __m128i c16_n34_n07 = _mm_set1_epi32(0xffdefff9);
    const __m128i c16_n36_n45 = _mm_set1_epi32(0xffdcffd3);
    const __m128i c16_p19_n11 = _mm_set1_epi32(0x0013fff5);
    const __m128i c16_p11_p41 = _mm_set1_epi32(0x000b0029);
    const __m128i c16_n45_n27 = _mm_set1_epi32(0xffd3ffe5);
    const __m128i c16_p07_n30 = _mm_set1_epi32(0x0007ffe2);
    const __m128i c16_p43_p39 = _mm_set1_epi32(0x002b0027);
    const __m128i c16_n02_p39 = _mm_set1_epi32(0xfffe0027);
    const __m128i c16_p43_p07 = _mm_set1_epi32(0x002b0007);
    const __m128i c16_n11_p34 = _mm_set1_epi32(0xfff50022);
    const __m128i c16_n15_p36 = _mm_set1_epi32(0xfff10024);
    const __m128i c16_n11_n45 = _mm_set1_epi32(0xfff5ffd3);
    const __m128i c16_p34_p39 = _mm_set1_epi32(0x00220027);
    const __m128i c16_n45_n19 = _mm_set1_epi32(0xffd3ffed);
    const __m128i c16_n27_p34 = _mm_set1_epi32(0xffe50022);
    const __m128i c16_p19_n39 = _mm_set1_epi32(0x0013ffd9);
    const __m128i c16_n11_p43 = _mm_set1_epi32(0xfff5002b);
    const __m128i c16_p02_n45 = _mm_set1_epi32(0x0002ffd3);
    const __m128i c16_n36_p30 = _mm_set1_epi32(0xffdc001e);
    const __m128i c16_p41_n23 = _mm_set1_epi32(0x0029ffe9);
    const __m128i c16_n44_p15 = _mm_set1_epi32(0xffd4000f);
    const __m128i c16_p45_n07 = _mm_set1_epi32(0x002dfff9);
    const __m128i c16_n43_p27 = _mm_set1_epi32(0xffd5001b);
    const __m128i c16_p44_n02 = _mm_set1_epi32(0x002cfffe);
    const __m128i c16_n30_n23 = _mm_set1_epi32(0xffe2ffe9);
    const __m128i c16_p07_p41 = _mm_set1_epi32(0x00070029);
    const __m128i c16_n45_p23 = _mm_set1_epi32(0xffd30017);
    const __m128i c16_p27_p19 = _mm_set1_epi32(0x001b0013);
    const __m128i c16_p15_n45 = _mm_set1_epi32(0x000fffd3);
    const __m128i c16_n44_p30 = _mm_set1_epi32(0xffd4001e);
    const __m128i c16_n44_p19 = _mm_set1_epi32(0xffd40013);
    const __m128i c16_n02_p36 = _mm_set1_epi32(0xfffe0024);
    const __m128i c16_p45_n34 = _mm_set1_epi32(0x002dffde);
    const __m128i c16_n15_n23 = _mm_set1_epi32(0xfff1ffe9);
    const __m128i c16_n39_p15 = _mm_set1_epi32(0xffd9000f);
    const __m128i c16_n30_p45 = _mm_set1_epi32(0xffe2002d);
    const __m128i c16_p27_p02 = _mm_set1_epi32(0x001b0002);
    const __m128i c16_p41_n44 = _mm_set1_epi32(0x0029ffd4);
    const __m128i c16_n30_p11 = _mm_set1_epi32(0xffe2000b);
    const __m128i c16_n19_p36 = _mm_set1_epi32(0xffed0024);
    const __m128i c16_p23_n02 = _mm_set1_epi32(0x0017fffe);
    const __m128i c16_n19_p07 = _mm_set1_epi32(0xffed0007);
    const __m128i c16_n39_p30 = _mm_set1_epi32(0xffd9001e);
    const __m128i c16_n45_p44 = _mm_set1_epi32(0xffd3002c);
    const __m128i c16_n36_p43 = _mm_set1_epi32(0xffdc002b);
    const __m128i c16_n07_p02 = _mm_set1_epi32(0xfff90002);
    const __m128i c16_n15_p11 = _mm_set1_epi32(0xfff1000b);
    const __m128i c16_n23_p19 = _mm_set1_epi32(0xffe90013);
    const __m128i c16_n30_p27 = _mm_set1_epi32(0xffe2001b);

    // EEO[8] coeffs
    const __m128i c16_p43_p45 = _mm_set1_epi32(0x002b002d);
    const __m128i c16_p35_p40 = _mm_set1_epi32(0x00230028);
    const __m128i c16_p29_p43 = _mm_set1_epi32(0x001d002b);
    const __m128i c16_n21_p04 = _mm_set1_epi32(0xffeb0004);
    const __m128i c16_p04_p40 = _mm_set1_epi32(0x00040028);
    const __m128i c16_n43_n35 = _mm_set1_epi32(0xffd5ffdd);
    const __m128i c16_n21_p35 = _mm_set1_epi32(0xffeb0023);
    const __m128i c16_p04_n43 = _mm_set1_epi32(0x0004ffd5);
    const __m128i c16_n40_p29 = _mm_set1_epi32(0xffd8001d);
    const __m128i c16_p45_n13 = _mm_set1_epi32(0x002dfff3);
    const __m128i c16_n45_p21 = _mm_set1_epi32(0xffd30015);
    const __m128i c16_p13_p29 = _mm_set1_epi32(0x000d001d);
    const __m128i c16_n35_p13 = _mm_set1_epi32(0xffdd000d);
    const __m128i c16_n40_p45 = _mm_set1_epi32(0xffd8002d);
    const __m128i c16_n13_p04 = _mm_set1_epi32(0xfff30004);
    const __m128i c16_n29_p21 = _mm_set1_epi32(0xffe30015);

    // EEEO[4] coeffs
    const __m128i c16_p38_p44 = _mm_set1_epi32(0x0026002c);
    const __m128i c16_n09_p38 = _mm_set1_epi32(0xfff70026);
    const __m128i c16_n44_p25 = _mm_set1_epi32(0xffd40019);
    const __m128i c16_n25_p09 = _mm_set1_epi32(0xffe70009);

    // EEEE[4] coeffs
    const __m128i c16_p42_p32 = _mm_set1_epi32(0x002a0020);
    const __m128i c16_p17_p32 = _mm_set1_epi32(0x00110020);
    const __m128i c16_n17_p32 = _mm_set1_epi32(0xffef0020);
    const __m128i c16_n42_p32 = _mm_set1_epi32(0xffd60020);

    __m128i c32_off = _mm_set1_epi32(1 << (shift - 1));

    __m128i src00, src01, src02, src03, src04, src05, src06, src07, src08, src09, src10, src11, src12, src13, src14, src15;
    __m128i src16, src17, src18, src19, src20, src21, src22, src23, src24, src25, src26, src27, src28, src29, src30, src31;
    __m128i res00, res01, res02, res03, res04, res05, res06, res07, res08, res09, res10, res11, res12, res13, res14, res15;
    __m128i res16, res17, res18, res19, res20, res21, res22, res23, res24, res25, res26, res27, res28, res29, res30, res31;
    __m128i res32, res33, res34, res35, res36, res37, res38, res39, res40, res41, res42, res43, res44, res45, res46, res47;
    __m128i res48, res49, res50, res51, res52, res53, res54, res55, res56, res57, res58, res59, res60, res61, res62, res63;

    __m128i tr0_0, tr0_1, tr0_2, tr0_3;
    __m128i tr1_0, tr1_1, tr1_2, tr1_3;
    int i;

    for (i = 0; i < line; i += 4)
    {
        src00 = _mm_loadl_epi64((const __m128i*)&src[0 * i_src + i]);
        src01 = _mm_loadl_epi64((const __m128i*)&src[1 * i_src + i]);
        src02 = _mm_loadl_epi64((const __m128i*)&src[2 * i_src + i]);
        src03 = _mm_loadl_epi64((const __m128i*)&src[3 * i_src + i]);
        src04 = _mm_loadl_epi64((const __m128i*)&src[4 * i_src + i]);
        src05 = _mm_loadl_epi64((const __m128i*)&src[5 * i_src + i]);
        src06 = _mm_loadl_epi64((const __m128i*)&src[6 * i_src + i]);
        src07 = _mm_loadl_epi64((const __m128i*)&src[7 * i_src + i]);
        src08 = _mm_loadl_epi64((const __m128i*)&src[8 * i_src + i]);
        src09 = _mm_loadl_epi64((const __m128i*)&src[9 * i_src + i]);
        src10 = _mm_loadl_epi64((const __m128i*)&src[10 * i_src + i]);
        src11 = _mm_loadl_epi64((const __m128i*)&src[11 * i_src + i]);
        src12 = _mm_loadl_epi64((const __m128i*)&src[12 * i_src + i]);
        src13 = _mm_loadl_epi64((const __m128i*)&src[13 * i_src + i]);
        src14 = _mm_loadl_epi64((const __m128i*)&src[14 * i_src + i]);
        src15 = _mm_loadl_epi64((const __m128i*)&src[15 * i_src + i]);
        src16 = _mm_loadl_epi64((const __m128i*)&src[16 * i_src + i]);
        src17 = _mm_loadl_epi64((const __m128i*)&src[17 * i_src + i]);
        src18 = _mm_loadl_epi64((const __m128i*)&src[18 * i_src + i]);
        src19 = _mm_loadl_epi64((const __m128i*)&src[19 * i_src + i]);
        src20 = _mm_loadl_epi64((const __m128i*)&src[20 * i_src + i]);
        src21 = _mm_loadl_epi64((const __m128i*)&src[21 * i_src + i]);
        src22 = _mm_loadl_epi64((const __m128i*)&src[22 * i_src + i]);
        src23 = _mm_loadl_epi64((const __m128i*)&src[23 * i_src + i]);
        src24 = _mm_loadl_epi64((const __m128i*)&src[24 * i_src + i]);
        src25 = _mm_loadl_epi64((const __m128i*)&src[25 * i_src + i]);
        src26 = _mm_loadl_epi64((const __m128i*)&src[26 * i_src + i]);
        src27 = _mm_loadl_epi64((const __m128i*)&src[27 * i_src + i]);
        src28 = _mm_loadl_epi64((const __m128i*)&src[28 * i_src + i]);
        src29 = _mm_loadl_epi64((const __m128i*)&src[29 * i_src + i]);
        src30 = _mm_loadl_epi64((const __m128i*)&src[30 * i_src + i]);
        src31 = _mm_loadl_epi64((const __m128i*)&src[31 * i_src + i]);

        {
            const __m128i T_00_00A = _mm_unpacklo_epi16(src01, src03);       // [33 13 32 12 31 11 30 10]
            const __m128i T_00_01A = _mm_unpacklo_epi16(src05, src07);
            const __m128i T_00_02A = _mm_unpacklo_epi16(src09, src11);
            const __m128i T_00_03A = _mm_unpacklo_epi16(src13, src15);
            const __m128i T_00_04A = _mm_unpacklo_epi16(src17, src19);
            const __m128i T_00_05A = _mm_unpacklo_epi16(src21, src23);
            const __m128i T_00_06A = _mm_unpacklo_epi16(src25, src27);
            const __m128i T_00_07A = _mm_unpacklo_epi16(src29, src31);

            const __m128i T_00_08A = _mm_unpacklo_epi16(src02, src06);
            const __m128i T_00_09A = _mm_unpacklo_epi16(src10, src14);
            const __m128i T_00_10A = _mm_unpacklo_epi16(src18, src22);
            const __m128i T_00_11A = _mm_unpacklo_epi16(src26, src30);

            const __m128i T_00_12A = _mm_unpacklo_epi16(src04, src12);
            const __m128i T_00_13A = _mm_unpacklo_epi16(src20, src28);

            const __m128i T_00_14A = _mm_unpacklo_epi16(src08, src24);
            const __m128i T_00_15A = _mm_unpacklo_epi16(src00, src16);

            __m128i O00A, O01A, O02A, O03A, O04A, O05A, O06A, O07A, O08A, O09A, O10A, O11A, O12A, O13A, O14A, O15A;
            __m128i O16A, O17A, O18A, O19A, O20A, O21A, O22A, O23A, O24A, O25A, O26A, O27A, O28A, O29A, O30A, O31A;
            __m128i EO00A, EO01A, EO02A, EO03A, EO04A, EO05A, EO06A, EO07A, EO08A, EO09A, EO10A, EO11A, EO12A, EO13A, EO14A, EO15A;
            {
                __m128i t0, t1, t2, t3, t4, t5, t6, t7;
#define COMPUTE_ROW(r0103, r0507, r0911, r1315, r1719, r2123, r2527, r2931, c0103, c0507, c0911, c1315, c1719, c2123, c2527, c2931, row) \
                t0 = _mm_madd_epi16(r0103, c0103); \
                t1 = _mm_madd_epi16(r0507, c0507); \
                t2 = _mm_madd_epi16(r0911, c0911); \
                t3 = _mm_madd_epi16(r1315, c1315); \
                t4 = _mm_madd_epi16(r1719, c1719); \
                t5 = _mm_madd_epi16(r2123, c2123); \
                t6 = _mm_madd_epi16(r2527, c2527); \
                t7 = _mm_madd_epi16(r2931, c2931); \
	            t0 = _mm_add_epi32(t0, t1); \
	            t2 = _mm_add_epi32(t2, t3); \
	            t4 = _mm_add_epi32(t4, t5); \
	            t6 = _mm_add_epi32(t6, t7); \
	            row = _mm_add_epi32(_mm_add_epi32(t0, t2), _mm_add_epi32(t4, t6));

                // O[32] 
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p45_p45, c16_p45_p45, c16_p44_p44, c16_p42_p43, c16_p40_p41, c16_p38_p39, c16_p36_p37, c16_p33_p34, O00A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p44_p45, c16_p39_p42, c16_p31_p36, c16_p20_p26, c16_p08_p14, c16_n06_p01, c16_n18_n12, c16_n30_n24, O01A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p42_p45, c16_p30_p37, c16_p10_p20, c16_n12_n01, c16_n31_n22, c16_n43_n38, c16_n45_n45, c16_n36_n41, O02A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p39_p45, c16_p16_p30, c16_n14_p01, c16_n38_n28, c16_n45_n44, c16_n31_n40, c16_n03_n18, c16_p26_p12, O03A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p36_p44, c16_p01_p20, c16_n34_n18, c16_n45_n44, c16_n22_n37, c16_p16_n03, c16_p43_p33, c16_p38_p45, O04A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p31_p44, c16_n14_p10, c16_n45_n34, c16_n28_n42, c16_p18_n06, c16_p45_p37, c16_p24_p40, c16_n22_p01, O05A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p26_p43, c16_n28_n01, c16_n42_n44, c16_p03_n24, c16_p44_p30, c16_p22_p41, c16_n31_n06, c16_n40_n45, O06A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p20_p42, c16_n38_n12, c16_n28_n45, c16_p33_p03, c16_p34_p45, c16_n26_p06, c16_n39_n44, c16_p18_n14, O07A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p14_p41, c16_n44_n22, c16_n06_n37, c16_p45_p30, c16_n03_p31, c16_n45_n36, c16_p12_n24, c16_p42_p40, O08A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p08_p40, c16_n45_n31, c16_p18_n22, c16_p34_p44, c16_n38_n03, c16_n12_n42, c16_p45_p28, c16_n14_p26, O09A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p01_p39, c16_n40_n38, c16_p37_n03, c16_p06_p41, c16_n42_n36, c16_p34_n08, c16_p10_p43, c16_n44_n33, O10A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n06_p38, c16_n31_n43, c16_p45_p16, c16_n26_p22, c16_n12_n45, c16_p41_p34, c16_n40_p01, c16_p10_n36, O11A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n12_p37, c16_n18_n45, c16_p40_p33, c16_n44_n06, c16_p28_n24, c16_p01_p43, c16_n30_n42, c16_p45_p22, O12A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n18_p36, c16_n03_n45, c16_p24_p43, c16_n39_n31, c16_p45_p12, c16_n40_p10, c16_p26_n30, c16_n06_p42, O13A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n24_p34, c16_p12_n41, c16_p01_p45, c16_n14_n45, c16_p26_p40, c16_n36_n33, c16_p42_p22, c16_n45_n10, O14A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n30_p33, c16_p26_n36, c16_n22_p38, c16_p18_n40, c16_n14_p42, c16_p10_n44, c16_n06_p45, c16_p01_n45, O15A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n34_p31, c16_p37_n28, c16_n39_p24, c16_p41_n20, c16_n43_p16, c16_p44_n12, c16_n45_p08, c16_p45_n03, O16A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n38_p30, c16_p44_n18, c16_n45_p06, c16_p43_p08, c16_n37_n20, c16_p28_p31, c16_n16_n39, c16_p03_p44, O17A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n41_p28, c16_p45_n08, c16_n38_n14, c16_p22_p33, c16_n01_n44, c16_n20_p44, c16_p37_n34, c16_n45_p16, O18A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n44_p26, c16_p41_p03, c16_n20_n31, c16_n10_p45, c16_p36_n38, c16_n45_p14, c16_p34_p16, c16_n08_n39, O19A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n45_p24, c16_p33_p14, c16_p03_n42, c16_n37_p39, c16_p44_n08, c16_n18_n30, c16_n20_p45, c16_p44_n28, O20A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n45_p22, c16_p20_p24, c16_p26_n45, c16_n45_p18, c16_p16_p28, c16_p30_n45, c16_n44_p14, c16_p12_p31, O21A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n45_p20, c16_p06_p33, c16_p41_n39, c16_n30_n10, c16_n24_p45, c16_p44_n16, c16_n01_n36, c16_n43_p37, O22A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n43_p18, c16_n10_p39, c16_p45_n26, c16_p01_n34, c16_n45_p33, c16_p08_p28, c16_p44_n38, c16_n16_n20, O23A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n40_p16, c16_n24_p44, c16_p36_n08, c16_p31_n45, c16_n30_n01, c16_n37_p45, c16_p22_p10, c16_p41_n43, O24A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n37_p14, c16_n36_p45, c16_p16_p12, c16_p45_n38, c16_p10_n34, c16_n39_p18, c16_n33_p45, c16_p20_p08, O25A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n33_p12, c16_n43_p44, c16_n08_p30, c16_p36_n16, c16_p41_n45, c16_p03_n26, c16_n38_p20, c16_n39_p45, O26A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n28_p10, c16_n45_p40, c16_n30_p41, c16_p08_p12, c16_p39_n26, c16_p42_n45, c16_p14_n31, c16_n24_p06, O27A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n22_p08, c16_n42_p34, c16_n43_p45, c16_n24_p36, c16_p06_p10, c16_p33_n20, c16_p45_n41, c16_p37_n44, O28A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n16_p06, c16_n34_p26, c16_n44_p40, c16_n44_p45, c16_n33_p39, c16_n14_p24, c16_p08_p03, c16_p28_n18, O29A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n10_p03, c16_n22_p16, c16_n33_p28, c16_n40_p37, c16_n45_p43, c16_n45_p45, c16_n41_p44, c16_n34_p38, O30A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n03_p01, c16_n08_p06, c16_n12_p10, c16_n16_p14, c16_n20_p18, c16_n24_p22, c16_n28_p26, c16_n31_p30, O31A)

#undef COMPUTE_ROW
            }

            {
                __m128i T1, T2;
#define COMPUTE_ROW(row0206, row1014, row1822, row2630, c0206, c1014, c1822, c2630, row) \
	            T1 = _mm_add_epi32(_mm_madd_epi16(row0206, c0206), _mm_madd_epi16(row1014, c1014)); \
	            T2 = _mm_add_epi32(_mm_madd_epi16(row1822, c1822), _mm_madd_epi16(row2630, c2630)); \
	            row = _mm_add_epi32(T1, T2);

                // EO[16] 
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p45_p45, c16_p43_p44, c16_p39_p41, c16_p34_p36, EO00A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p41_p45, c16_p23_p34, c16_n02_p11, c16_n27_n15, EO01A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p34_p44, c16_n07_p15, c16_n41_n27, c16_n39_n45, EO02A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p23_p43, c16_n34_n07, c16_n36_n45, c16_p19_n11, EO03A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p11_p41, c16_n45_n27, c16_p07_n30, c16_p43_p39, EO04A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n02_p39, c16_n36_n41, c16_p43_p07, c16_n11_p34, EO05A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n15_p36, c16_n11_n45, c16_p34_p39, c16_n45_n19, EO06A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n27_p34, c16_p19_n39, c16_n11_p43, c16_p02_n45, EO07A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n36_p30, c16_p41_n23, c16_n44_p15, c16_p45_n07, EO08A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n43_p27, c16_p44_n02, c16_n30_n23, c16_p07_p41, EO09A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n45_p23, c16_p27_p19, c16_p15_n45, c16_n44_p30, EO10A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n44_p19, c16_n02_p36, c16_p45_n34, c16_n15_n23, EO11A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n39_p15, c16_n30_p45, c16_p27_p02, c16_p41_n44, EO12A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n30_p11, c16_n45_p43, c16_n19_p36, c16_p23_n02, EO13A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n19_p07, c16_n39_p30, c16_n45_p44, c16_n36_p43, EO14A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n07_p02, c16_n15_p11, c16_n23_p19, c16_n30_p27, EO15A)

#undef COMPUTE_ROW
            }
            {
                // EEO[8] 
                const __m128i EEO0A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_p43_p45), _mm_madd_epi16(T_00_13A, c16_p35_p40));
                const __m128i EEO1A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_p29_p43), _mm_madd_epi16(T_00_13A, c16_n21_p04));
                const __m128i EEO2A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_p04_p40), _mm_madd_epi16(T_00_13A, c16_n43_n35));
                const __m128i EEO3A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n21_p35), _mm_madd_epi16(T_00_13A, c16_p04_n43));
                const __m128i EEO4A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n40_p29), _mm_madd_epi16(T_00_13A, c16_p45_n13));
                const __m128i EEO5A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n45_p21), _mm_madd_epi16(T_00_13A, c16_p13_p29));
                const __m128i EEO6A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n35_p13), _mm_madd_epi16(T_00_13A, c16_n40_p45));
                const __m128i EEO7A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n13_p04), _mm_madd_epi16(T_00_13A, c16_n29_p21));

                // EEEO[4] 
                const __m128i EEEO0A = _mm_madd_epi16(T_00_14A, c16_p38_p44);
                const __m128i EEEO1A = _mm_madd_epi16(T_00_14A, c16_n09_p38);
                const __m128i EEEO2A = _mm_madd_epi16(T_00_14A, c16_n44_p25);
                const __m128i EEEO3A = _mm_madd_epi16(T_00_14A, c16_n25_p09);

                const __m128i EEEE0A = _mm_madd_epi16(T_00_15A, c16_p42_p32);
                const __m128i EEEE1A = _mm_madd_epi16(T_00_15A, c16_p17_p32);
                const __m128i EEEE2A = _mm_madd_epi16(T_00_15A, c16_n17_p32);
                const __m128i EEEE3A = _mm_madd_epi16(T_00_15A, c16_n42_p32);

                const __m128i EEE0A = _mm_add_epi32(EEEE0A, EEEO0A);          // EEE0 = EEEE0 + EEEO0
                const __m128i EEE1A = _mm_add_epi32(EEEE1A, EEEO1A);          // EEE1 = EEEE1 + EEEO1
                const __m128i EEE2A = _mm_add_epi32(EEEE2A, EEEO2A);          // EEE2 = EEEE2 + EEEO2
                const __m128i EEE3A = _mm_add_epi32(EEEE3A, EEEO3A);          // EEE3 = EEEE3 + EEEO3
                const __m128i EEE7A = _mm_sub_epi32(EEEE0A, EEEO0A);          // EEE7 = EEEE0 - EEEO0
                const __m128i EEE6A = _mm_sub_epi32(EEEE1A, EEEO1A);          // EEE6 = EEEE1 - EEEO1
                const __m128i EEE5A = _mm_sub_epi32(EEEE2A, EEEO2A);          // EEE7 = EEEE2 - EEEO2
                const __m128i EEE4A = _mm_sub_epi32(EEEE3A, EEEO3A);          // EEE6 = EEEE3 - EEEO3

                const __m128i EE00A = _mm_add_epi32(EEE0A, EEO0A);          // EE0 = EEE0 + EEO0
                const __m128i EE01A = _mm_add_epi32(EEE1A, EEO1A);          // EE1 = EEE1 + EEO1
                const __m128i EE02A = _mm_add_epi32(EEE2A, EEO2A);          // EE2 = EEE2 + EEO2
                const __m128i EE03A = _mm_add_epi32(EEE3A, EEO3A);          // EE3 = EEE3 + EEO3
                const __m128i EE04A = _mm_add_epi32(EEE4A, EEO4A);          // EE4 = EEE4 + EEO4
                const __m128i EE05A = _mm_add_epi32(EEE5A, EEO5A);          // EE5 = EEE5 + EEO5
                const __m128i EE06A = _mm_add_epi32(EEE6A, EEO6A);          // EE6 = EEE6 + EEO6
                const __m128i EE07A = _mm_add_epi32(EEE7A, EEO7A);          // EE7 = EEE7 + EEO7
                const __m128i EE15A = _mm_sub_epi32(EEE0A, EEO0A);          // EE15 = EEE0 - EEO0
                const __m128i EE14A = _mm_sub_epi32(EEE1A, EEO1A);
                const __m128i EE13A = _mm_sub_epi32(EEE2A, EEO2A);
                const __m128i EE12A = _mm_sub_epi32(EEE3A, EEO3A);
                const __m128i EE11A = _mm_sub_epi32(EEE4A, EEO4A);          // EE11 = EEE4 - EEO4
                const __m128i EE10A = _mm_sub_epi32(EEE5A, EEO5A);
                const __m128i EE09A = _mm_sub_epi32(EEE6A, EEO6A);
                const __m128i EE08A = _mm_sub_epi32(EEE7A, EEO7A);

                const __m128i E00A = _mm_add_epi32(EE00A, EO00A);          // E00 = EE00 + EO00
                const __m128i E01A = _mm_add_epi32(EE01A, EO01A);          // E01 = EE01 + EO01
                const __m128i E02A = _mm_add_epi32(EE02A, EO02A);          // E02 = EE02 + EO02
                const __m128i E03A = _mm_add_epi32(EE03A, EO03A);          // E03 = EE03 + EO03
                const __m128i E04A = _mm_add_epi32(EE04A, EO04A);
                const __m128i E05A = _mm_add_epi32(EE05A, EO05A);
                const __m128i E06A = _mm_add_epi32(EE06A, EO06A);
                const __m128i E07A = _mm_add_epi32(EE07A, EO07A);
                const __m128i E08A = _mm_add_epi32(EE08A, EO08A);          // E08 = EE08 + EO08
                const __m128i E09A = _mm_add_epi32(EE09A, EO09A);
                const __m128i E10A = _mm_add_epi32(EE10A, EO10A);
                const __m128i E11A = _mm_add_epi32(EE11A, EO11A);
                const __m128i E12A = _mm_add_epi32(EE12A, EO12A);
                const __m128i E13A = _mm_add_epi32(EE13A, EO13A);
                const __m128i E14A = _mm_add_epi32(EE14A, EO14A);
                const __m128i E15A = _mm_add_epi32(EE15A, EO15A);
                const __m128i E31A = _mm_sub_epi32(EE00A, EO00A);          // E31 = EE00 - EO00
                const __m128i E30A = _mm_sub_epi32(EE01A, EO01A);          // E30 = EE01 - EO01
                const __m128i E29A = _mm_sub_epi32(EE02A, EO02A);          // E29 = EE02 - EO02
                const __m128i E28A = _mm_sub_epi32(EE03A, EO03A);          // E28 = EE03 - EO03
                const __m128i E27A = _mm_sub_epi32(EE04A, EO04A);
                const __m128i E26A = _mm_sub_epi32(EE05A, EO05A);
                const __m128i E25A = _mm_sub_epi32(EE06A, EO06A);
                const __m128i E24A = _mm_sub_epi32(EE07A, EO07A);
                const __m128i E23A = _mm_sub_epi32(EE08A, EO08A);          // E23 = EE08 - EO08
                const __m128i E22A = _mm_sub_epi32(EE09A, EO09A);
                const __m128i E21A = _mm_sub_epi32(EE10A, EO10A);
                const __m128i E20A = _mm_sub_epi32(EE11A, EO11A);
                const __m128i E19A = _mm_sub_epi32(EE12A, EO12A);
                const __m128i E18A = _mm_sub_epi32(EE13A, EO13A);
                const __m128i E17A = _mm_sub_epi32(EE14A, EO14A);
                const __m128i E16A = _mm_sub_epi32(EE15A, EO15A);

                const __m128i T1_00A = _mm_add_epi32(E00A, c32_off);         // E0 + off
                const __m128i T1_01A = _mm_add_epi32(E01A, c32_off);         // E1 + off
                const __m128i T1_02A = _mm_add_epi32(E02A, c32_off);         // E2 + off
                const __m128i T1_03A = _mm_add_epi32(E03A, c32_off);         // E3 + off
                const __m128i T1_04A = _mm_add_epi32(E04A, c32_off);         // E4 + off
                const __m128i T1_05A = _mm_add_epi32(E05A, c32_off);         // E5 + off
                const __m128i T1_06A = _mm_add_epi32(E06A, c32_off);         // E6 + off
                const __m128i T1_07A = _mm_add_epi32(E07A, c32_off);         // E7 + off
                const __m128i T1_08A = _mm_add_epi32(E08A, c32_off);         // E8 + off
                const __m128i T1_09A = _mm_add_epi32(E09A, c32_off);         // E9 + off
                const __m128i T1_10A = _mm_add_epi32(E10A, c32_off);         // E10 + off
                const __m128i T1_11A = _mm_add_epi32(E11A, c32_off);         // E11 + off
                const __m128i T1_12A = _mm_add_epi32(E12A, c32_off);         // E12 + off
                const __m128i T1_13A = _mm_add_epi32(E13A, c32_off);         // E13 + off
                const __m128i T1_14A = _mm_add_epi32(E14A, c32_off);         // E14 + off
                const __m128i T1_15A = _mm_add_epi32(E15A, c32_off);         // E15 + off
                const __m128i T1_16A = _mm_add_epi32(E16A, c32_off);
                const __m128i T1_17A = _mm_add_epi32(E17A, c32_off);
                const __m128i T1_18A = _mm_add_epi32(E18A, c32_off);
                const __m128i T1_19A = _mm_add_epi32(E19A, c32_off);
                const __m128i T1_20A = _mm_add_epi32(E20A, c32_off);
                const __m128i T1_21A = _mm_add_epi32(E21A, c32_off);
                const __m128i T1_22A = _mm_add_epi32(E22A, c32_off);
                const __m128i T1_23A = _mm_add_epi32(E23A, c32_off);
                const __m128i T1_24A = _mm_add_epi32(E24A, c32_off);
                const __m128i T1_25A = _mm_add_epi32(E25A, c32_off);
                const __m128i T1_26A = _mm_add_epi32(E26A, c32_off);
                const __m128i T1_27A = _mm_add_epi32(E27A, c32_off);
                const __m128i T1_28A = _mm_add_epi32(E28A, c32_off);
                const __m128i T1_29A = _mm_add_epi32(E29A, c32_off);
                const __m128i T1_30A = _mm_add_epi32(E30A, c32_off);
                const __m128i T1_31A = _mm_add_epi32(E31A, c32_off);

                const __m128i T2_00A = _mm_add_epi32(T1_00A, O00A);          // E0 + O0 + off
                const __m128i T2_01A = _mm_add_epi32(T1_01A, O01A);
                const __m128i T2_02A = _mm_add_epi32(T1_02A, O02A);          // E1 + O1 + off
                const __m128i T2_03A = _mm_add_epi32(T1_03A, O03A);
                const __m128i T2_04A = _mm_add_epi32(T1_04A, O04A);          // E2 + O2 + off
                const __m128i T2_05A = _mm_add_epi32(T1_05A, O05A);
                const __m128i T2_06A = _mm_add_epi32(T1_06A, O06A);          // E3 + O3 + off
                const __m128i T2_07A = _mm_add_epi32(T1_07A, O07A);
                const __m128i T2_08A = _mm_add_epi32(T1_08A, O08A);          // E4
                const __m128i T2_09A = _mm_add_epi32(T1_09A, O09A);
                const __m128i T2_10A = _mm_add_epi32(T1_10A, O10A);          // E5
                const __m128i T2_11A = _mm_add_epi32(T1_11A, O11A);
                const __m128i T2_12A = _mm_add_epi32(T1_12A, O12A);          // E6
                const __m128i T2_13A = _mm_add_epi32(T1_13A, O13A);
                const __m128i T2_14A = _mm_add_epi32(T1_14A, O14A);          // E7
                const __m128i T2_15A = _mm_add_epi32(T1_15A, O15A);
                const __m128i T2_16A = _mm_add_epi32(T1_16A, O16A);          // E8
                const __m128i T2_17A = _mm_add_epi32(T1_17A, O17A);
                const __m128i T2_18A = _mm_add_epi32(T1_18A, O18A);          // E9
                const __m128i T2_19A = _mm_add_epi32(T1_19A, O19A);
                const __m128i T2_20A = _mm_add_epi32(T1_20A, O20A);          // E10
                const __m128i T2_21A = _mm_add_epi32(T1_21A, O21A);
                const __m128i T2_22A = _mm_add_epi32(T1_22A, O22A);          // E11
                const __m128i T2_23A = _mm_add_epi32(T1_23A, O23A);
                const __m128i T2_24A = _mm_add_epi32(T1_24A, O24A);          // E12
                const __m128i T2_25A = _mm_add_epi32(T1_25A, O25A);
                const __m128i T2_26A = _mm_add_epi32(T1_26A, O26A);          // E13
                const __m128i T2_27A = _mm_add_epi32(T1_27A, O27A);
                const __m128i T2_28A = _mm_add_epi32(T1_28A, O28A);          // E14
                const __m128i T2_29A = _mm_add_epi32(T1_29A, O29A);
                const __m128i T2_30A = _mm_add_epi32(T1_30A, O30A);          // E15
                const __m128i T2_31A = _mm_add_epi32(T1_31A, O31A);
                const __m128i T2_63A = _mm_sub_epi32(T1_00A, O00A);          // E00 - O00 + off
                const __m128i T2_62A = _mm_sub_epi32(T1_01A, O01A);
                const __m128i T2_61A = _mm_sub_epi32(T1_02A, O02A);
                const __m128i T2_60A = _mm_sub_epi32(T1_03A, O03A);
                const __m128i T2_59A = _mm_sub_epi32(T1_04A, O04A);
                const __m128i T2_58A = _mm_sub_epi32(T1_05A, O05A);
                const __m128i T2_57A = _mm_sub_epi32(T1_06A, O06A);
                const __m128i T2_56A = _mm_sub_epi32(T1_07A, O07A);
                const __m128i T2_55A = _mm_sub_epi32(T1_08A, O08A);
                const __m128i T2_54A = _mm_sub_epi32(T1_09A, O09A);
                const __m128i T2_53A = _mm_sub_epi32(T1_10A, O10A);
                const __m128i T2_52A = _mm_sub_epi32(T1_11A, O11A);
                const __m128i T2_51A = _mm_sub_epi32(T1_12A, O12A);
                const __m128i T2_50A = _mm_sub_epi32(T1_13A, O13A);
                const __m128i T2_49A = _mm_sub_epi32(T1_14A, O14A);
                const __m128i T2_48A = _mm_sub_epi32(T1_15A, O15A);
                const __m128i T2_47A = _mm_sub_epi32(T1_16A, O16A);
                const __m128i T2_46A = _mm_sub_epi32(T1_17A, O17A);
                const __m128i T2_45A = _mm_sub_epi32(T1_18A, O18A);
                const __m128i T2_44A = _mm_sub_epi32(T1_19A, O19A);
                const __m128i T2_43A = _mm_sub_epi32(T1_20A, O20A);
                const __m128i T2_42A = _mm_sub_epi32(T1_21A, O21A);
                const __m128i T2_41A = _mm_sub_epi32(T1_22A, O22A);
                const __m128i T2_40A = _mm_sub_epi32(T1_23A, O23A);
                const __m128i T2_39A = _mm_sub_epi32(T1_24A, O24A);
                const __m128i T2_38A = _mm_sub_epi32(T1_25A, O25A);
                const __m128i T2_37A = _mm_sub_epi32(T1_26A, O26A);
                const __m128i T2_36A = _mm_sub_epi32(T1_27A, O27A);
                const __m128i T2_35A = _mm_sub_epi32(T1_28A, O28A);
                const __m128i T2_34A = _mm_sub_epi32(T1_29A, O29A);
                const __m128i T2_33A = _mm_sub_epi32(T1_30A, O30A);
                const __m128i T2_32A = _mm_sub_epi32(T1_31A, O31A);

                const __m128i T3_00A = _mm_srai_epi32(T2_00A, shift);             // [30 20 10 00]
                const __m128i T3_01A = _mm_srai_epi32(T2_01A, shift);             // [70 60 50 40]
                const __m128i T3_02A = _mm_srai_epi32(T2_02A, shift);             // [31 21 11 01]
                const __m128i T3_03A = _mm_srai_epi32(T2_03A, shift);             // [71 61 51 41]
                const __m128i T3_04A = _mm_srai_epi32(T2_04A, shift);             // [32 22 12 02]
                const __m128i T3_05A = _mm_srai_epi32(T2_05A, shift);             // [72 62 52 42]
                const __m128i T3_06A = _mm_srai_epi32(T2_06A, shift);             // [33 23 13 03]
                const __m128i T3_07A = _mm_srai_epi32(T2_07A, shift);             // [73 63 53 43]
                const __m128i T3_08A = _mm_srai_epi32(T2_08A, shift);             // [33 24 14 04]
                const __m128i T3_09A = _mm_srai_epi32(T2_09A, shift);             // [74 64 54 44]
                const __m128i T3_10A = _mm_srai_epi32(T2_10A, shift);             // [35 25 15 05]
                const __m128i T3_11A = _mm_srai_epi32(T2_11A, shift);             // [75 65 55 45]
                const __m128i T3_12A = _mm_srai_epi32(T2_12A, shift);             // [36 26 16 06]
                const __m128i T3_13A = _mm_srai_epi32(T2_13A, shift);             // [76 66 56 46]
                const __m128i T3_14A = _mm_srai_epi32(T2_14A, shift);             // [37 27 17 07]
                const __m128i T3_15A = _mm_srai_epi32(T2_15A, shift);             // [77 67 57 47]
                const __m128i T3_16A = _mm_srai_epi32(T2_16A, shift);             // [30 20 10 00] x8
                const __m128i T3_17A = _mm_srai_epi32(T2_17A, shift);             // [70 60 50 40]
                const __m128i T3_18A = _mm_srai_epi32(T2_18A, shift);             // [31 21 11 01] x9
                const __m128i T3_19A = _mm_srai_epi32(T2_19A, shift);             // [71 61 51 41]
                const __m128i T3_20A = _mm_srai_epi32(T2_20A, shift);             // [32 22 12 02] xA
                const __m128i T3_21A = _mm_srai_epi32(T2_21A, shift);             // [72 62 52 42]
                const __m128i T3_22A = _mm_srai_epi32(T2_22A, shift);             // [33 23 13 03] xB
                const __m128i T3_23A = _mm_srai_epi32(T2_23A, shift);             // [73 63 53 43]
                const __m128i T3_24A = _mm_srai_epi32(T2_24A, shift);             // [33 24 14 04] xC
                const __m128i T3_25A = _mm_srai_epi32(T2_25A, shift);             // [74 64 54 44]
                const __m128i T3_26A = _mm_srai_epi32(T2_26A, shift);             // [35 25 15 05] xD
                const __m128i T3_27A = _mm_srai_epi32(T2_27A, shift);             // [75 65 55 45]
                const __m128i T3_28A = _mm_srai_epi32(T2_28A, shift);             // [36 26 16 06] xE
                const __m128i T3_29A = _mm_srai_epi32(T2_29A, shift);             // [76 66 56 46]
                const __m128i T3_30A = _mm_srai_epi32(T2_30A, shift);             // [37 27 17 07] xF
                const __m128i T3_31A = _mm_srai_epi32(T2_31A, shift);             // [77 67 57 47]
                const __m128i T3_63A = _mm_srai_epi32(T2_63A, shift);
                const __m128i T3_62A = _mm_srai_epi32(T2_62A, shift);
                const __m128i T3_61A = _mm_srai_epi32(T2_61A, shift);
                const __m128i T3_60A = _mm_srai_epi32(T2_60A, shift);
                const __m128i T3_59A = _mm_srai_epi32(T2_59A, shift);
                const __m128i T3_58A = _mm_srai_epi32(T2_58A, shift);
                const __m128i T3_57A = _mm_srai_epi32(T2_57A, shift);
                const __m128i T3_56A = _mm_srai_epi32(T2_56A, shift);
                const __m128i T3_55A = _mm_srai_epi32(T2_55A, shift);
                const __m128i T3_54A = _mm_srai_epi32(T2_54A, shift);
                const __m128i T3_53A = _mm_srai_epi32(T2_53A, shift);
                const __m128i T3_52A = _mm_srai_epi32(T2_52A, shift);
                const __m128i T3_51A = _mm_srai_epi32(T2_51A, shift);
                const __m128i T3_50A = _mm_srai_epi32(T2_50A, shift);
                const __m128i T3_49A = _mm_srai_epi32(T2_49A, shift);
                const __m128i T3_48A = _mm_srai_epi32(T2_48A, shift);
                const __m128i T3_47A = _mm_srai_epi32(T2_47A, shift);
                const __m128i T3_46A = _mm_srai_epi32(T2_46A, shift);
                const __m128i T3_45A = _mm_srai_epi32(T2_45A, shift);
                const __m128i T3_44A = _mm_srai_epi32(T2_44A, shift);
                const __m128i T3_43A = _mm_srai_epi32(T2_43A, shift);
                const __m128i T3_42A = _mm_srai_epi32(T2_42A, shift);
                const __m128i T3_41A = _mm_srai_epi32(T2_41A, shift);
                const __m128i T3_40A = _mm_srai_epi32(T2_40A, shift);
                const __m128i T3_39A = _mm_srai_epi32(T2_39A, shift);
                const __m128i T3_38A = _mm_srai_epi32(T2_38A, shift);
                const __m128i T3_37A = _mm_srai_epi32(T2_37A, shift);
                const __m128i T3_36A = _mm_srai_epi32(T2_36A, shift);
                const __m128i T3_35A = _mm_srai_epi32(T2_35A, shift);
                const __m128i T3_34A = _mm_srai_epi32(T2_34A, shift);
                const __m128i T3_33A = _mm_srai_epi32(T2_33A, shift);
                const __m128i T3_32A = _mm_srai_epi32(T2_32A, shift);
                const __m128i zero = _mm_setzero_si128();

                res00 = _mm_packs_epi32(T3_00A, zero);        // [70 60 50 40 30 20 10 00]
                res01 = _mm_packs_epi32(T3_01A, zero);        // [71 61 51 41 31 21 11 01]
                res02 = _mm_packs_epi32(T3_02A, zero);        // [72 62 52 42 32 22 12 02]
                res03 = _mm_packs_epi32(T3_03A, zero);        // [73 63 53 43 33 23 13 03]
                res04 = _mm_packs_epi32(T3_04A, zero);        // [74 64 54 44 34 24 14 04]
                res05 = _mm_packs_epi32(T3_05A, zero);        // [75 65 55 45 35 25 15 05]
                res06 = _mm_packs_epi32(T3_06A, zero);        // [76 66 56 46 36 26 16 06]
                res07 = _mm_packs_epi32(T3_07A, zero);        // [77 67 57 47 37 27 17 07]
                res08 = _mm_packs_epi32(T3_08A, zero);        // [A0 ... 80]
                res09 = _mm_packs_epi32(T3_09A, zero);        // [A1 ... 81]
                res10 = _mm_packs_epi32(T3_10A, zero);        // [A2 ... 82]
                res11 = _mm_packs_epi32(T3_11A, zero);        // [A3 ... 83]
                res12 = _mm_packs_epi32(T3_12A, zero);        // [A4 ... 84]
                res13 = _mm_packs_epi32(T3_13A, zero);        // [A5 ... 85]
                res14 = _mm_packs_epi32(T3_14A, zero);        // [A6 ... 86]
                res15 = _mm_packs_epi32(T3_15A, zero);        // [A7 ... 87]
                res16 = _mm_packs_epi32(T3_16A, zero);
                res17 = _mm_packs_epi32(T3_17A, zero);
                res18 = _mm_packs_epi32(T3_18A, zero);
                res19 = _mm_packs_epi32(T3_19A, zero);
                res20 = _mm_packs_epi32(T3_20A, zero);
                res21 = _mm_packs_epi32(T3_21A, zero);
                res22 = _mm_packs_epi32(T3_22A, zero);
                res23 = _mm_packs_epi32(T3_23A, zero);
                res24 = _mm_packs_epi32(T3_24A, zero);
                res25 = _mm_packs_epi32(T3_25A, zero);
                res26 = _mm_packs_epi32(T3_26A, zero);
                res27 = _mm_packs_epi32(T3_27A, zero);
                res28 = _mm_packs_epi32(T3_28A, zero);
                res29 = _mm_packs_epi32(T3_29A, zero);
                res30 = _mm_packs_epi32(T3_30A, zero);
                res31 = _mm_packs_epi32(T3_31A, zero);
                res32 = _mm_packs_epi32(T3_32A, zero);        // [70 60 50 40 30 20 10 00]
                res33 = _mm_packs_epi32(T3_33A, zero);        // [71 61 51 41 31 21 11 01]
                res34 = _mm_packs_epi32(T3_34A, zero);        // [72 62 52 42 32 22 12 02]
                res35 = _mm_packs_epi32(T3_35A, zero);        // [73 63 53 43 33 23 13 03]
                res36 = _mm_packs_epi32(T3_36A, zero);        // [74 64 54 44 34 24 14 04]
                res37 = _mm_packs_epi32(T3_37A, zero);        // [75 65 55 45 35 25 15 05]
                res38 = _mm_packs_epi32(T3_38A, zero);        // [76 66 56 46 36 26 16 06]
                res39 = _mm_packs_epi32(T3_39A, zero);        // [77 67 57 47 37 27 17 07]
                res40 = _mm_packs_epi32(T3_40A, zero);        // [A0 ... 80]
                res41 = _mm_packs_epi32(T3_41A, zero);        // [A1 ... 81]
                res42 = _mm_packs_epi32(T3_42A, zero);        // [A2 ... 82]
                res43 = _mm_packs_epi32(T3_43A, zero);        // [A3 ... 83]
                res44 = _mm_packs_epi32(T3_44A, zero);        // [A4 ... 84]
                res45 = _mm_packs_epi32(T3_45A, zero);        // [A5 ... 85]
                res46 = _mm_packs_epi32(T3_46A, zero);        // [A6 ... 86]
                res47 = _mm_packs_epi32(T3_47A, zero);        // [A7 ... 87]
                res48 = _mm_packs_epi32(T3_48A, zero);
                res49 = _mm_packs_epi32(T3_49A, zero);
                res50 = _mm_packs_epi32(T3_50A, zero);
                res51 = _mm_packs_epi32(T3_51A, zero);
                res52 = _mm_packs_epi32(T3_52A, zero);
                res53 = _mm_packs_epi32(T3_53A, zero);
                res54 = _mm_packs_epi32(T3_54A, zero);
                res55 = _mm_packs_epi32(T3_55A, zero);
                res56 = _mm_packs_epi32(T3_56A, zero);
                res57 = _mm_packs_epi32(T3_57A, zero);
                res58 = _mm_packs_epi32(T3_58A, zero);
                res59 = _mm_packs_epi32(T3_59A, zero);
                res60 = _mm_packs_epi32(T3_60A, zero);
                res61 = _mm_packs_epi32(T3_61A, zero);
                res62 = _mm_packs_epi32(T3_62A, zero);
                res63 = _mm_packs_epi32(T3_63A, zero);
            }
            {
                //transpose matrix H x W: 64x4 --> 4x64
                TRANSPOSE_8x4_16BIT(res00, res01, res02, res03, res04, res05, res06, res07, src00, src08, src16, src24)
                TRANSPOSE_8x4_16BIT(res08, res09, res10, res11, res12, res13, res14, res15, src01, src09, src17, src25)
                TRANSPOSE_8x4_16BIT(res16, res17, res18, res19, res20, res21, res22, res23, src02, src10, src18, src26)
                TRANSPOSE_8x4_16BIT(res24, res25, res26, res27, res28, res29, res30, res31, src03, src11, src19, src27)
                TRANSPOSE_8x4_16BIT(res32, res33, res34, res35, res36, res37, res38, res39, src04, src12, src20, src28)
                TRANSPOSE_8x4_16BIT(res40, res41, res42, res43, res44, res45, res46, res47, src05, src13, src21, src29)
                TRANSPOSE_8x4_16BIT(res48, res49, res50, res51, res52, res53, res54, res55, src06, src14, src22, src30)
                TRANSPOSE_8x4_16BIT(res56, res57, res58, res59, res60, res61, res62, res63, src07, src15, src23, src31)
            }
            if (bit_depth != MAX_TX_DYNAMIC_RANGE) {
                __m128i max_val = _mm_set1_epi16((1 << bit_depth) - 1);
                __m128i min_val = _mm_set1_epi16(-(1 << bit_depth));
                //the first row
                src00 = _mm_min_epi16(src00, max_val);
                src01 = _mm_min_epi16(src01, max_val);
                src02 = _mm_min_epi16(src02, max_val);
                src03 = _mm_min_epi16(src03, max_val);
                src04 = _mm_min_epi16(src04, max_val);
                src05 = _mm_min_epi16(src05, max_val);
                src06 = _mm_min_epi16(src06, max_val);
                src07 = _mm_min_epi16(src07, max_val);
                src00 = _mm_max_epi16(src00, min_val);
                src01 = _mm_max_epi16(src01, min_val);
                src02 = _mm_max_epi16(src02, min_val);
                src03 = _mm_max_epi16(src03, min_val);
                src04 = _mm_max_epi16(src04, min_val);
                src05 = _mm_max_epi16(src05, min_val);
                src06 = _mm_max_epi16(src06, min_val);
                src07 = _mm_max_epi16(src07, min_val);
                _mm_store_si128((__m128i*)&dst[ 0], src00); 
                _mm_store_si128((__m128i*)&dst[ 8], src01); 
                _mm_store_si128((__m128i*)&dst[16], src02); 
                _mm_store_si128((__m128i*)&dst[24], src03); 
                _mm_store_si128((__m128i*)&dst[32], src04); 
                _mm_store_si128((__m128i*)&dst[40], src05); 
                _mm_store_si128((__m128i*)&dst[48], src06); 
                _mm_store_si128((__m128i*)&dst[56], src07);

                dst += 64;
                //the second row
                src08 = _mm_min_epi16(src08, max_val);
                src09 = _mm_min_epi16(src09, max_val);
                src10 = _mm_min_epi16(src10, max_val);
                src11 = _mm_min_epi16(src11, max_val);
                src12 = _mm_min_epi16(src12, max_val);
                src13 = _mm_min_epi16(src13, max_val);
                src14 = _mm_min_epi16(src14, max_val);
                src15 = _mm_min_epi16(src15, max_val);
                src08 = _mm_max_epi16(src08, min_val);
                src09 = _mm_max_epi16(src09, min_val);
                src10 = _mm_max_epi16(src10, min_val);
                src11 = _mm_max_epi16(src11, min_val);
                src12 = _mm_max_epi16(src12, min_val);
                src13 = _mm_max_epi16(src13, min_val);
                src14 = _mm_max_epi16(src14, min_val);
                src15 = _mm_max_epi16(src15, min_val);
                _mm_store_si128((__m128i*)&dst[ 0], src08); 
                _mm_store_si128((__m128i*)&dst[ 8], src09); 
                _mm_store_si128((__m128i*)&dst[16], src10); 
                _mm_store_si128((__m128i*)&dst[24], src11); 
                _mm_store_si128((__m128i*)&dst[32], src12); 
                _mm_store_si128((__m128i*)&dst[40], src13); 
                _mm_store_si128((__m128i*)&dst[48], src14); 
                _mm_store_si128((__m128i*)&dst[56], src15);

                dst += 64;
                //the third row
                src16 = _mm_min_epi16(src16, max_val);
                src17 = _mm_min_epi16(src17, max_val);
                src18 = _mm_min_epi16(src18, max_val);
                src19 = _mm_min_epi16(src19, max_val);
                src20 = _mm_min_epi16(src20, max_val);
                src21 = _mm_min_epi16(src21, max_val);
                src22 = _mm_min_epi16(src22, max_val);
                src23 = _mm_min_epi16(src23, max_val);
                src16 = _mm_max_epi16(src16, min_val);
                src17 = _mm_max_epi16(src17, min_val);
                src18 = _mm_max_epi16(src18, min_val);
                src19 = _mm_max_epi16(src19, min_val);
                src20 = _mm_max_epi16(src20, min_val);
                src21 = _mm_max_epi16(src21, min_val);
                src22 = _mm_max_epi16(src22, min_val);
                src23 = _mm_max_epi16(src23, min_val);
                _mm_store_si128((__m128i*)&dst[ 0], src16);
                _mm_store_si128((__m128i*)&dst[ 8], src17);
                _mm_store_si128((__m128i*)&dst[16], src18);
                _mm_store_si128((__m128i*)&dst[24], src19);
                _mm_store_si128((__m128i*)&dst[32], src20);
                _mm_store_si128((__m128i*)&dst[40], src21);
                _mm_store_si128((__m128i*)&dst[48], src22);
                _mm_store_si128((__m128i*)&dst[56], src23);

                dst += 64;
                //the fourth row
                src24 = _mm_min_epi16(src24, max_val);
                src25 = _mm_min_epi16(src25, max_val);
                src26 = _mm_min_epi16(src26, max_val);
                src27 = _mm_min_epi16(src27, max_val);
                src28 = _mm_min_epi16(src28, max_val);
                src29 = _mm_min_epi16(src29, max_val);
                src30 = _mm_min_epi16(src30, max_val);
                src31 = _mm_min_epi16(src31, max_val);
                src24 = _mm_max_epi16(src24, min_val);
                src25 = _mm_max_epi16(src25, min_val);
                src26 = _mm_max_epi16(src26, min_val);
                src27 = _mm_max_epi16(src27, min_val);
                src28 = _mm_max_epi16(src28, min_val);
                src29 = _mm_max_epi16(src29, min_val);
                src30 = _mm_max_epi16(src30, min_val);
                src31 = _mm_max_epi16(src31, min_val);
                _mm_store_si128((__m128i*)&dst[ 0], src24);
                _mm_store_si128((__m128i*)&dst[ 8], src25);
                _mm_store_si128((__m128i*)&dst[16], src26);
                _mm_store_si128((__m128i*)&dst[24], src27);
                _mm_store_si128((__m128i*)&dst[32], src28);
                _mm_store_si128((__m128i*)&dst[40], src29);
                _mm_store_si128((__m128i*)&dst[48], src30);
                _mm_store_si128((__m128i*)&dst[56], src31);

                dst += 64;
            }
            else {
                //the first row
                _mm_store_si128((__m128i*)&dst[0 ], src00);
                _mm_store_si128((__m128i*)&dst[8 ], src01);
                _mm_store_si128((__m128i*)&dst[16], src02);
                _mm_store_si128((__m128i*)&dst[24], src03);
                _mm_store_si128((__m128i*)&dst[32], src04);
                _mm_store_si128((__m128i*)&dst[40], src05);
                _mm_store_si128((__m128i*)&dst[48], src06);
                _mm_store_si128((__m128i*)&dst[56], src07);

                dst += 64;
                //the second row
                _mm_store_si128((__m128i*)&dst[0 ], src08);
                _mm_store_si128((__m128i*)&dst[8 ], src09);
                _mm_store_si128((__m128i*)&dst[16], src10);
                _mm_store_si128((__m128i*)&dst[24], src11);
                _mm_store_si128((__m128i*)&dst[32], src12);
                _mm_store_si128((__m128i*)&dst[40], src13);
                _mm_store_si128((__m128i*)&dst[48], src14);
                _mm_store_si128((__m128i*)&dst[56], src15);

                dst += 64;
                //the third row
                _mm_store_si128((__m128i*)&dst[0 ], src16);
                _mm_store_si128((__m128i*)&dst[8 ], src17);
                _mm_store_si128((__m128i*)&dst[16], src18);
                _mm_store_si128((__m128i*)&dst[24], src19);
                _mm_store_si128((__m128i*)&dst[32], src20);
                _mm_store_si128((__m128i*)&dst[40], src21);
                _mm_store_si128((__m128i*)&dst[48], src22);
                _mm_store_si128((__m128i*)&dst[56], src23);

                dst += 64;
                //the fourth row
                _mm_store_si128((__m128i*)&dst[0 ], src24);
                _mm_store_si128((__m128i*)&dst[8 ], src25);
                _mm_store_si128((__m128i*)&dst[16], src26);
                _mm_store_si128((__m128i*)&dst[24], src27);
                _mm_store_si128((__m128i*)&dst[32], src28);
                _mm_store_si128((__m128i*)&dst[40], src29);
                _mm_store_si128((__m128i*)&dst[48], src30);
                _mm_store_si128((__m128i*)&dst[56], src31);

                dst += 64;
            }
        }
    }

}

void itrans_dct2_h4_w8_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h4_sse(src, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h8_sse(tmp, 4, dst, 4, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h4_w16_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h4_sse(src, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h16_sse(tmp, 4, dst, 4, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h4_w32_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h4_sse(src, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h32_sse(tmp, 4, dst, 4, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h8_w4_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h8_sse(src, 4, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h4_sse(tmp, dst, 8, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h8_w16_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h8_sse(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h16_sse(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h8_w32_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h8_sse(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h32_sse(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h8_w64_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h8_sse(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h64_sse(tmp, 8, dst, 8, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h16_w4_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h16_sse(src, 4, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h4_sse(tmp, dst, 16, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h16_w8_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h16_sse(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h8_sse(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h16_w32_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h16_sse(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h32_sse(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h16_w64_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h16_sse(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h64_sse(tmp, 16, dst, 16, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h32_w4_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h32_sse(src, 4, tmp, 4, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h4_sse(tmp, dst, 32, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h32_w8_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h32_sse(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h8_sse(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h32_w16_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h32_sse(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h16_sse(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h32_w64_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h32_sse(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h64_sse(tmp, 32, dst, 32, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h64_w8_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h64_sse(src, 8, tmp, 8, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h8_sse(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h64_w16_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h64_sse(src, 16, tmp, 16, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h16_sse(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h64_w32_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h64_sse(src, 32, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h32_sse(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h64_w64_sse(s16 *src, s16 *dst, int bit_depth)
{
    ALIGNED_16(s16 tmp[MAX_TR_DIM]);
    dct2_butterfly_h64_sse(src, 64, tmp, 32, 5, MAX_TX_DYNAMIC_RANGE);
    dct2_butterfly_h64_sse(tmp, 64, dst, 64, 20 - bit_depth, bit_depth);
}

void itrans_dct2_h4_w4_sse(s16 *src, s16 *dst, int bit_depth)
{
    __m128i s0, s1, s2, s3;
    __m128i e0, e1, o0, o1;
    __m128i v0, v1, v2, v3;
    const __m128i c16_p17_p42 = _mm_set1_epi32(0x0011002A);
    const __m128i c16_n42_p17 = _mm_set1_epi32(0xFFD60011);
    const __m128i c16_n32_p32 = _mm_set1_epi32(0xFFe00020);
    const __m128i c16_p32_p32 = _mm_set1_epi32(0x00200020);
    __m128i off = _mm_set1_epi32(16);
    __m128i max_val = _mm_set1_epi16((1 << bit_depth) - 1);
    __m128i min_val = _mm_set1_epi16(-(1 << bit_depth));
    int shift = 20 - bit_depth; // second transform

    s0 = _mm_load_si128((__m128i*)src);
    s1 = _mm_load_si128((__m128i*)(src + 8));
    s2 = _mm_unpacklo_epi16(s0, s1);
    s3 = _mm_unpackhi_epi16(s0, s1);

    e0 = _mm_madd_epi16(s2, c16_p32_p32);
    e1 = _mm_madd_epi16(s2, c16_n32_p32);
    o0 = _mm_madd_epi16(s3, c16_p17_p42);
    o1 = _mm_madd_epi16(s3, c16_n42_p17);
    v0 = _mm_add_epi32(e0, o0);
    v1 = _mm_add_epi32(e1, o1);
    v2 = _mm_sub_epi32(e1, o1);
    v3 = _mm_sub_epi32(e0, o0);

    v0 = _mm_add_epi32(v0, off);
    v1 = _mm_add_epi32(v1, off);
    v2 = _mm_add_epi32(v2, off);
    v3 = _mm_add_epi32(v3, off);

    v0 = _mm_srai_epi32(v0, 5);
    v1 = _mm_srai_epi32(v1, 5);
    v2 = _mm_srai_epi32(v2, 5);
    v3 = _mm_srai_epi32(v3, 5);

    v0 = _mm_packs_epi32(v0, v2);       // 00 10 20 30 02 12 22 32
    v1 = _mm_packs_epi32(v1, v3);       // 01 11 21 31 03 13 23 33

    // inverse
    s0 = _mm_unpacklo_epi16(v0, v1);   
    s1 = _mm_unpackhi_epi16(v0, v1);
    v0 = _mm_unpacklo_epi32(s0, s1);    // 00 01 02 03 10 11 12 13
    v1 = _mm_unpackhi_epi32(s0, s1);    // 20 21 22 23 30 31 32 33

    // second butterfly transform
    off = _mm_set1_epi32(1 << (shift - 1));

    s2 = _mm_unpacklo_epi16(v0, v1);
    s3 = _mm_unpackhi_epi16(v0, v1);

    e0 = _mm_madd_epi16(s2, c16_p32_p32);
    e1 = _mm_madd_epi16(s2, c16_n32_p32);
    o0 = _mm_madd_epi16(s3, c16_p17_p42);
    o1 = _mm_madd_epi16(s3, c16_n42_p17);
    v0 = _mm_add_epi32(e0, o0);
    v1 = _mm_add_epi32(e1, o1);
    v2 = _mm_sub_epi32(e1, o1);
    v3 = _mm_sub_epi32(e0, o0);

    v0 = _mm_add_epi32(v0, off);
    v1 = _mm_add_epi32(v1, off);
    v2 = _mm_add_epi32(v2, off);
    v3 = _mm_add_epi32(v3, off);
    v0 = _mm_srai_epi32(v0, shift);
    v1 = _mm_srai_epi32(v1, shift);
    v2 = _mm_srai_epi32(v2, shift);
    v3 = _mm_srai_epi32(v3, shift);

    v0 = _mm_packs_epi32(v0, v2);       // 00 10 20 30 02 12 22 32
    v1 = _mm_packs_epi32(v1, v3);       // 01 11 21 31 03 13 23 33

    // inverse
    s0 = _mm_unpacklo_epi16(v0, v1);
    s1 = _mm_unpackhi_epi16(v0, v1);
    v0 = _mm_unpacklo_epi32(s0, s1);    // 00 01 02 03 10 11 12 13
    v1 = _mm_unpackhi_epi32(s0, s1);    // 20 21 22 23 30 31 32 33

    // clip
    v0 = _mm_min_epi16(v0, max_val);
    v1 = _mm_min_epi16(v1, max_val);
    v0 = _mm_max_epi16(v0, min_val);
    v1 = _mm_max_epi16(v1, min_val);

    _mm_store_si128((__m128i*)dst, v0);
    _mm_store_si128((__m128i*)(dst + 8), v1);
}

void itrans_dct2_h8_w8_sse(s16 *src, s16 *dst, int bit_depth)
{
    __m128i s0, s1, s2, s3, s4, s5, s6, s7, offset;
    __m128i e0h, e1h, e2h, e3h, e0l, e1l, e2l, e3l, O0h, O1h, O2h, O3h, O0l, O1l, O2l, O3l, ee0l, ee1l, e00l, e01l, ee0h, ee1h, e00h, e01h;
	__m128i t0, t1, t2, t3, t4, t5, t6, t7;
    __m128i c0, c1;
    __m128i max_val, min_val;
    int shift = 20 - bit_depth;

	offset = _mm_set1_epi32(16);								

    // O[0] -- O[3]
	s1 = _mm_load_si128((__m128i*)&src[8]);
	s3 = _mm_load_si128((__m128i*)&src[24]);
	s5 = _mm_load_si128((__m128i*)&src[40]);
	s7 = _mm_load_si128((__m128i*)&src[56]);

    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[0]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[1]));
    
	t0 = _mm_unpacklo_epi16(s1, s3);
	t1 = _mm_unpackhi_epi16(s1, s3);
	t2 = _mm_unpacklo_epi16(s5, s7);
	t3 = _mm_unpackhi_epi16(s5, s7);

	e1l = _mm_madd_epi16(t0, c0);
	e1h = _mm_madd_epi16(t1, c0);
	e2l = _mm_madd_epi16(t2, c1);
	e2h = _mm_madd_epi16(t3, c1);
    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[2]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[3]));
	O0l = _mm_add_epi32(e1l, e2l);
	O0h = _mm_add_epi32(e1h, e2h);

	e1l = _mm_madd_epi16(t0, c0);
    e1h = _mm_madd_epi16(t1, c0);
    e2l = _mm_madd_epi16(t2, c1);
    e2h = _mm_madd_epi16(t3, c1);
    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[4]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[5]));
	O1l = _mm_add_epi32(e1l, e2l);
	O1h = _mm_add_epi32(e1h, e2h);

	e1l = _mm_madd_epi16(t0, c0);
    e1h = _mm_madd_epi16(t1, c0);
    e2l = _mm_madd_epi16(t2, c1);
    e2h = _mm_madd_epi16(t3, c1);
    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[6]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[7]));
	O2l = _mm_add_epi32(e1l, e2l);
	O2h = _mm_add_epi32(e1h, e2h);

	e1l = _mm_madd_epi16(t0, c0);
    e1h = _mm_madd_epi16(t1, c0);
    e2l = _mm_madd_epi16(t2, c1);
    e2h = _mm_madd_epi16(t3, c1);
	O3l = _mm_add_epi32(e1l, e2l);
	O3h = _mm_add_epi32(e1h, e2h);

	// E[0] - E[3]
	s0 = _mm_load_si128((__m128i*)&src[0]);
	s2 = _mm_load_si128((__m128i*)&src[16]);
	s4 = _mm_load_si128((__m128i*)&src[32]);
	s6 = _mm_load_si128((__m128i*)&src[48]);

    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[8]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[9]));
	t0 = _mm_unpacklo_epi16(s0, s4);
	t1 = _mm_unpackhi_epi16(s0, s4);
	ee0l = _mm_madd_epi16(t0, c0);
    ee0h = _mm_madd_epi16(t1, c0);
    ee1l = _mm_madd_epi16(t0, c1);
    ee1h = _mm_madd_epi16(t1, c1);

    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[10]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[11]));
	t0 = _mm_unpacklo_epi16(s2, s6);
	t1 = _mm_unpackhi_epi16(s2, s6);
	e00l = _mm_madd_epi16(t0, c0);
    e00h = _mm_madd_epi16(t1, c0);
    e01l = _mm_madd_epi16(t0, c1);
    e01h = _mm_madd_epi16(t1, c1);
	e0l = _mm_add_epi32(ee0l, e00l);
	e0h = _mm_add_epi32(ee0h, e00h);
	e3l = _mm_sub_epi32(ee0l, e00l);
	e3h = _mm_sub_epi32(ee0h, e00h);
	e0l = _mm_add_epi32(e0l, offset);
	e0h = _mm_add_epi32(e0h, offset);
	e3l = _mm_add_epi32(e3l, offset);
	e3h = _mm_add_epi32(e3h, offset);

	e1l = _mm_add_epi32(ee1l, e01l);
	e1h = _mm_add_epi32(ee1h, e01h);
	e2l = _mm_sub_epi32(ee1l, e01l);
	e2h = _mm_sub_epi32(ee1h, e01h);
	e1l = _mm_add_epi32(e1l, offset);
	e1h = _mm_add_epi32(e1h, offset);
	e2l = _mm_add_epi32(e2l, offset);
	e2h = _mm_add_epi32(e2h, offset);
	s0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e0l, O0l), 5), _mm_srai_epi32(_mm_add_epi32(e0h, O0h), 5));			// 首次反变换移位数
	s7 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e0l, O0l), 5), _mm_srai_epi32(_mm_sub_epi32(e0h, O0h), 5));
	s1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e1l, O1l), 5), _mm_srai_epi32(_mm_add_epi32(e1h, O1h), 5));
	s6 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e1l, O1l), 5), _mm_srai_epi32(_mm_sub_epi32(e1h, O1h), 5));
	s2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e2l, O2l), 5), _mm_srai_epi32(_mm_add_epi32(e2h, O2h), 5));
	s5 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e2l, O2l), 5), _mm_srai_epi32(_mm_sub_epi32(e2h, O2h), 5));
	s3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e3l, O3l), 5), _mm_srai_epi32(_mm_add_epi32(e3h, O3h), 5));
	s4 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e3l, O3l), 5), _mm_srai_epi32(_mm_sub_epi32(e3h, O3h), 5));

	/*  inverse   */
	e0l = _mm_unpacklo_epi16(s0, s4);
	e1l = _mm_unpacklo_epi16(s1, s5);
	e2l = _mm_unpacklo_epi16(s2, s6);
	e3l = _mm_unpacklo_epi16(s3, s7);
	O0l = _mm_unpackhi_epi16(s0, s4);
	O1l = _mm_unpackhi_epi16(s1, s5);
	O2l = _mm_unpackhi_epi16(s2, s6);
	O3l = _mm_unpackhi_epi16(s3, s7);
	t0 = _mm_unpacklo_epi16(e0l, e2l);
	t1 = _mm_unpacklo_epi16(e1l, e3l);
	s0 = _mm_unpacklo_epi16(t0, t1);
	s1 = _mm_unpackhi_epi16(t0, t1);
	t2 = _mm_unpackhi_epi16(e0l, e2l);
	t3 = _mm_unpackhi_epi16(e1l, e3l);
	s2 = _mm_unpacklo_epi16(t2, t3);
	s3 = _mm_unpackhi_epi16(t2, t3);
	t0 = _mm_unpacklo_epi16(O0l, O2l);
	t1 = _mm_unpacklo_epi16(O1l, O3l);
	s4 = _mm_unpacklo_epi16(t0, t1);
	s5 = _mm_unpackhi_epi16(t0, t1);
	t2 = _mm_unpackhi_epi16(O0l, O2l);
	t3 = _mm_unpackhi_epi16(O1l, O3l);
	s6 = _mm_unpacklo_epi16(t2, t3);
	s7 = _mm_unpackhi_epi16(t2, t3);

    // second transform
	offset = _mm_set1_epi32(1 << (shift - 1));						

    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[0]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[1]));

    t0 = _mm_unpacklo_epi16(s1, s3);
    t1 = _mm_unpackhi_epi16(s1, s3);
    t2 = _mm_unpacklo_epi16(s5, s7);
    t3 = _mm_unpackhi_epi16(s5, s7);

    e1l = _mm_madd_epi16(t0, c0);
    e1h = _mm_madd_epi16(t1, c0);
    e2l = _mm_madd_epi16(t2, c1);
    e2h = _mm_madd_epi16(t3, c1);
    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[2]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[3]));
    O0l = _mm_add_epi32(e1l, e2l);
    O0h = _mm_add_epi32(e1h, e2h);

    e1l = _mm_madd_epi16(t0, c0);
    e1h = _mm_madd_epi16(t1, c0);
    e2l = _mm_madd_epi16(t2, c1);
    e2h = _mm_madd_epi16(t3, c1);
    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[4]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[5]));
    O1l = _mm_add_epi32(e1l, e2l);
    O1h = _mm_add_epi32(e1h, e2h);

    e1l = _mm_madd_epi16(t0, c0);
    e1h = _mm_madd_epi16(t1, c0);
    e2l = _mm_madd_epi16(t2, c1);
    e2h = _mm_madd_epi16(t3, c1);
    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[6]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[7]));
    O2l = _mm_add_epi32(e1l, e2l);
    O2h = _mm_add_epi32(e1h, e2h);

    e1l = _mm_madd_epi16(t0, c0);
    e1h = _mm_madd_epi16(t1, c0);
    e2l = _mm_madd_epi16(t2, c1);
    e2h = _mm_madd_epi16(t3, c1);
    O3l = _mm_add_epi32(e1l, e2l);
    O3h = _mm_add_epi32(e1h, e2h);

    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[8]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[9]));
    t0 = _mm_unpacklo_epi16(s0, s4);
    t1 = _mm_unpackhi_epi16(s0, s4);
    ee0l = _mm_madd_epi16(t0, c0);
    ee0h = _mm_madd_epi16(t1, c0);
    ee1l = _mm_madd_epi16(t0, c1);
    ee1h = _mm_madd_epi16(t1, c1);

    c0 = _mm_load_si128((__m128i*)(tab_idct_8x8[10]));
    c1 = _mm_load_si128((__m128i*)(tab_idct_8x8[11]));
    t0 = _mm_unpacklo_epi16(s2, s6);
    t1 = _mm_unpackhi_epi16(s2, s6);
    e00l = _mm_madd_epi16(t0, c0);
    e00h = _mm_madd_epi16(t1, c0);
    e01l = _mm_madd_epi16(t0, c1);
    e01h = _mm_madd_epi16(t1, c1);
    e0l = _mm_add_epi32(ee0l, e00l);
    e0h = _mm_add_epi32(ee0h, e00h);
    e3l = _mm_sub_epi32(ee0l, e00l);
    e3h = _mm_sub_epi32(ee0h, e00h);
    e0l = _mm_add_epi32(e0l, offset);
    e0h = _mm_add_epi32(e0h, offset);
    e3l = _mm_add_epi32(e3l, offset);
    e3h = _mm_add_epi32(e3h, offset);

    e1l = _mm_add_epi32(ee1l, e01l);
    e1h = _mm_add_epi32(ee1h, e01h);
    e2l = _mm_sub_epi32(ee1l, e01l);
    e2h = _mm_sub_epi32(ee1h, e01h);
    e1l = _mm_add_epi32(e1l, offset);
    e1h = _mm_add_epi32(e1h, offset);
    e2l = _mm_add_epi32(e2l, offset);
    e2h = _mm_add_epi32(e2h, offset);

	s0 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e0l, O0l), shift), _mm_srai_epi32(_mm_add_epi32(e0h, O0h), shift));
	s7 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e0l, O0l), shift), _mm_srai_epi32(_mm_sub_epi32(e0h, O0h), shift));
	s1 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e1l, O1l), shift), _mm_srai_epi32(_mm_add_epi32(e1h, O1h), shift));
	s6 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e1l, O1l), shift), _mm_srai_epi32(_mm_sub_epi32(e1h, O1h), shift));
	s2 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e2l, O2l), shift), _mm_srai_epi32(_mm_add_epi32(e2h, O2h), shift));
	s5 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e2l, O2l), shift), _mm_srai_epi32(_mm_sub_epi32(e2h, O2h), shift));
	s3 = _mm_packs_epi32(_mm_srai_epi32(_mm_add_epi32(e3l, O3l), shift), _mm_srai_epi32(_mm_add_epi32(e3h, O3h), shift));
	s4 = _mm_packs_epi32(_mm_srai_epi32(_mm_sub_epi32(e3l, O3l), shift), _mm_srai_epi32(_mm_sub_epi32(e3h, O3h), shift));

	// [07 06 05 04 03 02 01 00]
	// [17 16 15 14 13 12 11 10]
	// [27 26 25 24 23 22 21 20]
	// [37 36 35 34 33 32 31 30]
	// [47 46 45 44 43 42 41 40]
	// [57 56 55 54 53 52 51 50]
	// [67 66 65 64 63 62 61 60]
	// [77 76 75 74 73 72 71 70]
	t0 = _mm_unpacklo_epi16(s0, s1);     // [13 03 12 02 11 01 10 00]
	
	t1 = _mm_unpackhi_epi16(s0, s1);     // [17 07 16 06 15 05 14 04]
	t2 = _mm_unpacklo_epi16(s2, s3);     // [33 23 32 22 31 21 30 20]
	
	t3 = _mm_unpackhi_epi16(s2, s3);     // [37 27 36 26 35 25 34 24]
	t4 = _mm_unpacklo_epi16(s4, s5);     // [53 43 52 42 51 41 50 40]
	
	t5 = _mm_unpackhi_epi16(s4, s5);     // [57 47 56 46 55 45 54 44]
	t6 = _mm_unpacklo_epi16(s6, s7);     // [73 63 72 62 71 61 70 60]
	
	t7 = _mm_unpackhi_epi16(s6, s7);     // [77 67 76 66 75 65 74 64]

	s0 = _mm_unpacklo_epi32(t0, t2);    // [31 21 11 01 30 20 10 00]
	s1 = _mm_unpackhi_epi32(t0, t2);    // [33 23 13 03 32 22 12 02]
	s2 = _mm_unpacklo_epi32(t4, t6);    // [71 61 51 41 70 60 50 40]
	s3 = _mm_unpackhi_epi32(t4, t6);    // [73 63 53 43 72 62 52 42]	
	s4 = _mm_unpacklo_epi32(t1, t3);    // [35 25 15 05 34 24 14 04]
	s5 = _mm_unpackhi_epi32(t1, t3);    // [37 27 17 07 36 26 16 06]
	s6 = _mm_unpacklo_epi32(t5, t7);    // [75 65 55 45 74 64 54 44]
	s7 = _mm_unpackhi_epi32(t5, t7);    // [77 67 57 47 76 66 56 46]

	//clip
    max_val = _mm_set1_epi16((1 << bit_depth) - 1);
    min_val = _mm_set1_epi16(-(1 << bit_depth));
	s0 = _mm_min_epi16(s0, max_val);
	s0 = _mm_max_epi16(s0, min_val);
	s1 = _mm_min_epi16(s1, max_val);
	s1 = _mm_max_epi16(s1, min_val);
	s2 = _mm_min_epi16(s2, max_val);
	s2 = _mm_max_epi16(s2, min_val);
	s3 = _mm_min_epi16(s3, max_val);
	s3 = _mm_max_epi16(s3, min_val);
	s4 = _mm_min_epi16(s4, max_val);
	s4 = _mm_max_epi16(s4, min_val);
	s5 = _mm_min_epi16(s5, max_val);
	s5 = _mm_max_epi16(s5, min_val);
	s6 = _mm_min_epi16(s6, max_val);
	s6 = _mm_max_epi16(s6, min_val);
	s7 = _mm_min_epi16(s7, max_val);
	s7 = _mm_max_epi16(s7, min_val);
	
    t0 = _mm_unpacklo_epi64(s0, s2);
    t1 = _mm_unpackhi_epi64(s0, s2);
    t2 = _mm_unpacklo_epi64(s1, s3);
    t3 = _mm_unpackhi_epi64(s1, s3);
    t4 = _mm_unpacklo_epi64(s4, s6);
    t5 = _mm_unpackhi_epi64(s4, s6);
    t6 = _mm_unpacklo_epi64(s5, s7);
    t7 = _mm_unpackhi_epi64(s5, s7);

    _mm_store_si128((__m128i*)dst, t0);
    _mm_store_si128((__m128i*)(dst + 8), t1);
    _mm_store_si128((__m128i*)(dst + 16), t2);
    _mm_store_si128((__m128i*)(dst + 24), t3);
    _mm_store_si128((__m128i*)(dst + 32), t4);
    _mm_store_si128((__m128i*)(dst + 40), t5);
    _mm_store_si128((__m128i*)(dst + 48), t6);
    _mm_store_si128((__m128i*)(dst + 56), t7);
}

void itrans_dct2_h16_w16_sse(s16 *src, s16 *dst, int bit_depth)
{
    const __m128i c16_p43_p45 = _mm_set1_epi32(0x002B002D);		//row0 
    const __m128i c16_p35_p40 = _mm_set1_epi32(0x00230028);
    const __m128i c16_p21_p29 = _mm_set1_epi32(0x0015001D);
    const __m128i c16_p04_p13 = _mm_set1_epi32(0x0004000D);
    const __m128i c16_p29_p43 = _mm_set1_epi32(0x001D002B);		//row1
    const __m128i c16_n21_p04 = _mm_set1_epi32(0xFFEB0004);
    const __m128i c16_n45_n40 = _mm_set1_epi32(0xFFD3FFD8);
    const __m128i c16_n13_n35 = _mm_set1_epi32(0xFFF3FFDD);
    const __m128i c16_p04_p40 = _mm_set1_epi32(0x00040028);		//row2
    const __m128i c16_n43_n35 = _mm_set1_epi32(0xFFD5FFDD);
    const __m128i c16_p29_n13 = _mm_set1_epi32(0x001DFFF3);
    const __m128i c16_p21_p45 = _mm_set1_epi32(0x0015002D);
    const __m128i c16_n21_p35 = _mm_set1_epi32(0xFFEB0023);		//row3
    const __m128i c16_p04_n43 = _mm_set1_epi32(0x0004FFD5);
    const __m128i c16_p13_p45 = _mm_set1_epi32(0x000D002D);
    const __m128i c16_n29_n40 = _mm_set1_epi32(0xFFE3FFD8);
    const __m128i c16_n40_p29 = _mm_set1_epi32(0xFFD8001D);		//row4
    const __m128i c16_p45_n13 = _mm_set1_epi32(0x002DFFF3);
    const __m128i c16_n43_n04 = _mm_set1_epi32(0xFFD5FFFC);
    const __m128i c16_p35_p21 = _mm_set1_epi32(0x00230015);
    const __m128i c16_n45_p21 = _mm_set1_epi32(0xFFD30015);		//row5
    const __m128i c16_p13_p29 = _mm_set1_epi32(0x000D001D);
    const __m128i c16_p35_n43 = _mm_set1_epi32(0x0023FFD5);
    const __m128i c16_n40_p04 = _mm_set1_epi32(0xFFD80004);
    const __m128i c16_n35_p13 = _mm_set1_epi32(0xFFDD000D);		//row6
    const __m128i c16_n40_p45 = _mm_set1_epi32(0xFFD8002D);
    const __m128i c16_p04_p21 = _mm_set1_epi32(0x00040015);
    const __m128i c16_p43_n29 = _mm_set1_epi32(0x002BFFE3);
    const __m128i c16_n13_p04 = _mm_set1_epi32(0xFFF30004);		//row7
    const __m128i c16_n29_p21 = _mm_set1_epi32(0xFFE30015);
    const __m128i c16_n40_p35 = _mm_set1_epi32(0xFFD80023);
    const __m128i c16_n45_p43 = _mm_set1_epi32(0xFFD3002B);
    const __m128i c16_p38_p44 = _mm_set1_epi32(0x0026002C);
    const __m128i c16_p09_p25 = _mm_set1_epi32(0x00090019);
    const __m128i c16_n09_p38 = _mm_set1_epi32(0xFFF70026);
    const __m128i c16_n25_n44 = _mm_set1_epi32(0xFFE7FFD4);
    const __m128i c16_n44_p25 = _mm_set1_epi32(0xFFD40019);
    const __m128i c16_p38_p09 = _mm_set1_epi32(0x00260009);
    const __m128i c16_n25_p09 = _mm_set1_epi32(0xFFE70009);
    const __m128i c16_n44_p38 = _mm_set1_epi32(0xFFD40026);
    const __m128i c16_p17_p42 = _mm_set1_epi32(0x0011002A);
    const __m128i c16_n42_p17 = _mm_set1_epi32(0xFFD60011);
    const __m128i c16_n32_p32 = _mm_set1_epi32(0xFFE00020);
    const __m128i c16_p32_p32 = _mm_set1_epi32(0x00200020);
    int i, pass, part;
    __m128i c32_off = _mm_set1_epi32(16);
    int shift = 5;

    __m128i in00[2], in01[2], in02[2], in03[2], in04[2], in05[2], in06[2], in07[2];
    __m128i in08[2], in09[2], in10[2], in11[2], in12[2], in13[2], in14[2], in15[2];
    __m128i res00[2], res01[2], res02[2], res03[2], res04[2], res05[2], res06[2], res07[2];
    __m128i res08[2], res09[2], res10[2], res11[2], res12[2], res13[2], res14[2], res15[2];

    for (i = 0; i < 2; i++)
    {
        const int offset = (i << 3);

        in00[i] = _mm_load_si128((const __m128i*)&src[0 * 16 + offset]);    // [07 06 05 04 03 02 01 00]
        in01[i] = _mm_load_si128((const __m128i*)&src[1 * 16 + offset]);    // [17 16 15 14 13 12 11 10]
        in02[i] = _mm_load_si128((const __m128i*)&src[2 * 16 + offset]);    // [27 26 25 24 23 22 21 20]
        in03[i] = _mm_load_si128((const __m128i*)&src[3 * 16 + offset]);    // [37 36 35 34 33 32 31 30]
        in04[i] = _mm_load_si128((const __m128i*)&src[4 * 16 + offset]);    // [47 46 45 44 43 42 41 40]
        in05[i] = _mm_load_si128((const __m128i*)&src[5 * 16 + offset]);    // [57 56 55 54 53 52 51 50]
        in06[i] = _mm_load_si128((const __m128i*)&src[6 * 16 + offset]);    // [67 66 65 64 63 62 61 60]
        in07[i] = _mm_load_si128((const __m128i*)&src[7 * 16 + offset]);    // [77 76 75 74 73 72 71 70]
        in08[i] = _mm_load_si128((const __m128i*)&src[8 * 16 + offset]);
        in09[i] = _mm_load_si128((const __m128i*)&src[9 * 16 + offset]);
        in10[i] = _mm_load_si128((const __m128i*)&src[10 * 16 + offset]);
        in11[i] = _mm_load_si128((const __m128i*)&src[11 * 16 + offset]);
        in12[i] = _mm_load_si128((const __m128i*)&src[12 * 16 + offset]);
        in13[i] = _mm_load_si128((const __m128i*)&src[13 * 16 + offset]);
        in14[i] = _mm_load_si128((const __m128i*)&src[14 * 16 + offset]);
        in15[i] = _mm_load_si128((const __m128i*)&src[15 * 16 + offset]);
    }

    for (pass = 0; pass < 2; pass++)
    {
        if (pass == 1)
        {
            shift = 20 - bit_depth;
            c32_off = _mm_set1_epi32(1 << (shift - 1));				// pass == 1 第二次四舍五入
        }

        for (part = 0; part < 2; part++)
        {
            const __m128i T_00_00A = _mm_unpacklo_epi16(in01[part], in03[part]);       // [33 13 32 12 31 11 30 10]
            const __m128i T_00_00B = _mm_unpackhi_epi16(in01[part], in03[part]);       // [37 17 36 16 35 15 34 14]
            const __m128i T_00_01A = _mm_unpacklo_epi16(in05[part], in07[part]);       // [ ]
            const __m128i T_00_01B = _mm_unpackhi_epi16(in05[part], in07[part]);       // [ ]
            const __m128i T_00_02A = _mm_unpacklo_epi16(in09[part], in11[part]);       // [ ]
            const __m128i T_00_02B = _mm_unpackhi_epi16(in09[part], in11[part]);       // [ ]
            const __m128i T_00_03A = _mm_unpacklo_epi16(in13[part], in15[part]);       // [ ]
            const __m128i T_00_03B = _mm_unpackhi_epi16(in13[part], in15[part]);       // [ ]
            const __m128i T_00_04A = _mm_unpacklo_epi16(in02[part], in06[part]);       // [ ]
            const __m128i T_00_04B = _mm_unpackhi_epi16(in02[part], in06[part]);       // [ ]
            const __m128i T_00_05A = _mm_unpacklo_epi16(in10[part], in14[part]);       // [ ]
            const __m128i T_00_05B = _mm_unpackhi_epi16(in10[part], in14[part]);       // [ ]
            const __m128i T_00_06A = _mm_unpacklo_epi16(in04[part], in12[part]);       // [ ]
            const __m128i T_00_06B = _mm_unpackhi_epi16(in04[part], in12[part]);       // [ ]
            const __m128i T_00_07A = _mm_unpacklo_epi16(in00[part], in08[part]);       // [83 03 82 02 81 01 81 00] row08 row00
            const __m128i T_00_07B = _mm_unpackhi_epi16(in00[part], in08[part]);       // [87 07 86 06 85 05 84 04]

            __m128i O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A;
            __m128i O0B, O1B, O2B, O3B, O4B, O5B, O6B, O7B;
            __m128i EO0A, EO1A, EO2A, EO3A;
            __m128i EO0B, EO1B, EO2B, EO3B;
            __m128i EEO0A, EEO1A;
            __m128i EEO0B, EEO1B;
            __m128i EEE0A, EEE1A;
            __m128i EEE0B, EEE1B;
            __m128i T00, T01;
#define COMPUTE_ROW(row0103, row0507, row0911, row1315, c0103, c0507, c0911, c1315, row) \
	T00 = _mm_add_epi32(_mm_madd_epi16(row0103, c0103), _mm_madd_epi16(row0507, c0507)); \
	T01 = _mm_add_epi32(_mm_madd_epi16(row0911, c0911), _mm_madd_epi16(row1315, c1315)); \
	row = _mm_add_epi32(T00, T01);

            COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c16_p43_p45, c16_p35_p40, c16_p21_p29, c16_p04_p13, O0A)
            COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c16_p29_p43, c16_n21_p04, c16_n45_n40, c16_n13_n35, O1A)
            COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c16_p04_p40, c16_n43_n35, c16_p29_n13, c16_p21_p45, O2A)
            COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c16_n21_p35, c16_p04_n43, c16_p13_p45, c16_n29_n40, O3A)
            COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c16_n40_p29, c16_p45_n13, c16_n43_n04, c16_p35_p21, O4A)
            COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c16_n45_p21, c16_p13_p29, c16_p35_n43, c16_n40_p04, O5A)
            COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c16_n35_p13, c16_n40_p45, c16_p04_p21, c16_p43_n29, O6A)
            COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c16_n13_p04, c16_n29_p21, c16_n40_p35, c16_n45_p43, O7A)

            COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c16_p43_p45, c16_p35_p40, c16_p21_p29, c16_p04_p13, O0B)
            COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c16_p29_p43, c16_n21_p04, c16_n45_n40, c16_n13_n35, O1B)
            COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c16_p04_p40, c16_n43_n35, c16_p29_n13, c16_p21_p45, O2B)
            COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c16_n21_p35, c16_p04_n43, c16_p13_p45, c16_n29_n40, O3B)
            COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c16_n40_p29, c16_p45_n13, c16_n43_n04, c16_p35_p21, O4B)
            COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c16_n45_p21, c16_p13_p29, c16_p35_n43, c16_n40_p04, O5B)
            COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c16_n35_p13, c16_n40_p45, c16_p04_p21, c16_p43_n29, O6B)
            COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c16_n13_p04, c16_n29_p21, c16_n40_p35, c16_n45_p43, O7B)
#undef COMPUTE_ROW

            EO0A = _mm_add_epi32(_mm_madd_epi16(T_00_04A, c16_p38_p44), _mm_madd_epi16(T_00_05A, c16_p09_p25)); // EO0
            EO0B = _mm_add_epi32(_mm_madd_epi16(T_00_04B, c16_p38_p44), _mm_madd_epi16(T_00_05B, c16_p09_p25));
            EO1A = _mm_add_epi32(_mm_madd_epi16(T_00_04A, c16_n09_p38), _mm_madd_epi16(T_00_05A, c16_n25_n44)); // EO1
            EO1B = _mm_add_epi32(_mm_madd_epi16(T_00_04B, c16_n09_p38), _mm_madd_epi16(T_00_05B, c16_n25_n44));
            EO2A = _mm_add_epi32(_mm_madd_epi16(T_00_04A, c16_n44_p25), _mm_madd_epi16(T_00_05A, c16_p38_p09)); // EO2
            EO2B = _mm_add_epi32(_mm_madd_epi16(T_00_04B, c16_n44_p25), _mm_madd_epi16(T_00_05B, c16_p38_p09));
            EO3A = _mm_add_epi32(_mm_madd_epi16(T_00_04A, c16_n25_p09), _mm_madd_epi16(T_00_05A, c16_n44_p38)); // EO3
            EO3B = _mm_add_epi32(_mm_madd_epi16(T_00_04B, c16_n25_p09), _mm_madd_epi16(T_00_05B, c16_n44_p38));

            EEO0A = _mm_madd_epi16(T_00_06A, c16_p17_p42);
            EEO0B = _mm_madd_epi16(T_00_06B, c16_p17_p42);
            EEO1A = _mm_madd_epi16(T_00_06A, c16_n42_p17);
            EEO1B = _mm_madd_epi16(T_00_06B, c16_n42_p17);

            EEE0A = _mm_madd_epi16(T_00_07A, c16_p32_p32);
            EEE0B = _mm_madd_epi16(T_00_07B, c16_p32_p32);
            EEE1A = _mm_madd_epi16(T_00_07A, c16_n32_p32);
            EEE1B = _mm_madd_epi16(T_00_07B, c16_n32_p32);

            {
                const __m128i EE0A = _mm_add_epi32(EEE0A, EEO0A);          // EE0 = EEE0 + EEO0
                const __m128i EE0B = _mm_add_epi32(EEE0B, EEO0B);
                const __m128i EE1A = _mm_add_epi32(EEE1A, EEO1A);          // EE1 = EEE1 + EEO1
                const __m128i EE1B = _mm_add_epi32(EEE1B, EEO1B);
                const __m128i EE3A = _mm_sub_epi32(EEE0A, EEO0A);          // EE2 = EEE0 - EEO0
                const __m128i EE3B = _mm_sub_epi32(EEE0B, EEO0B);
                const __m128i EE2A = _mm_sub_epi32(EEE1A, EEO1A);          // EE3 = EEE1 - EEO1
                const __m128i EE2B = _mm_sub_epi32(EEE1B, EEO1B);

                const __m128i E0A = _mm_add_epi32(EE0A, EO0A);          // E0 = EE0 + EO0
                const __m128i E0B = _mm_add_epi32(EE0B, EO0B);
                const __m128i E1A = _mm_add_epi32(EE1A, EO1A);          // E1 = EE1 + EO1
                const __m128i E1B = _mm_add_epi32(EE1B, EO1B);
                const __m128i E2A = _mm_add_epi32(EE2A, EO2A);          // E2 = EE2 + EO2
                const __m128i E2B = _mm_add_epi32(EE2B, EO2B);
                const __m128i E3A = _mm_add_epi32(EE3A, EO3A);          // E3 = EE3 + EO3
                const __m128i E3B = _mm_add_epi32(EE3B, EO3B);
                const __m128i E7A = _mm_sub_epi32(EE0A, EO0A);          // E0 = EE0 - EO0
                const __m128i E7B = _mm_sub_epi32(EE0B, EO0B);
                const __m128i E6A = _mm_sub_epi32(EE1A, EO1A);          // E1 = EE1 - EO1
                const __m128i E6B = _mm_sub_epi32(EE1B, EO1B);
                const __m128i E5A = _mm_sub_epi32(EE2A, EO2A);          // E2 = EE2 - EO2
                const __m128i E5B = _mm_sub_epi32(EE2B, EO2B);
                const __m128i E4A = _mm_sub_epi32(EE3A, EO3A);          // E3 = EE3 - EO3
                const __m128i E4B = _mm_sub_epi32(EE3B, EO3B);

                const __m128i T10A = _mm_add_epi32(E0A, c32_off);         // E0 + off
                const __m128i T10B = _mm_add_epi32(E0B, c32_off);
                const __m128i T11A = _mm_add_epi32(E1A, c32_off);         // E1 + off
                const __m128i T11B = _mm_add_epi32(E1B, c32_off);
                const __m128i T12A = _mm_add_epi32(E2A, c32_off);         // E2 + off
                const __m128i T12B = _mm_add_epi32(E2B, c32_off);
                const __m128i T13A = _mm_add_epi32(E3A, c32_off);         // E3 + off
                const __m128i T13B = _mm_add_epi32(E3B, c32_off);
                const __m128i T14A = _mm_add_epi32(E4A, c32_off);         // E4 + off
                const __m128i T14B = _mm_add_epi32(E4B, c32_off);
                const __m128i T15A = _mm_add_epi32(E5A, c32_off);         // E5 + off
                const __m128i T15B = _mm_add_epi32(E5B, c32_off);
                const __m128i T16A = _mm_add_epi32(E6A, c32_off);         // E6 + off
                const __m128i T16B = _mm_add_epi32(E6B, c32_off);
                const __m128i T17A = _mm_add_epi32(E7A, c32_off);         // E7 + off
                const __m128i T17B = _mm_add_epi32(E7B, c32_off);

                const __m128i T20A = _mm_add_epi32(T10A, O0A);          // E0 + O0 + off
                const __m128i T20B = _mm_add_epi32(T10B, O0B);
                const __m128i T21A = _mm_add_epi32(T11A, O1A);          // E1 + O1 + off
                const __m128i T21B = _mm_add_epi32(T11B, O1B);
                const __m128i T22A = _mm_add_epi32(T12A, O2A);          // E2 + O2 + off
                const __m128i T22B = _mm_add_epi32(T12B, O2B);
                const __m128i T23A = _mm_add_epi32(T13A, O3A);          // E3 + O3 + off
                const __m128i T23B = _mm_add_epi32(T13B, O3B);
                const __m128i T24A = _mm_add_epi32(T14A, O4A);          // E4
                const __m128i T24B = _mm_add_epi32(T14B, O4B);
                const __m128i T25A = _mm_add_epi32(T15A, O5A);          // E5
                const __m128i T25B = _mm_add_epi32(T15B, O5B);
                const __m128i T26A = _mm_add_epi32(T16A, O6A);          // E6
                const __m128i T26B = _mm_add_epi32(T16B, O6B);
                const __m128i T27A = _mm_add_epi32(T17A, O7A);          // E7
                const __m128i T27B = _mm_add_epi32(T17B, O7B);
                const __m128i T2FA = _mm_sub_epi32(T10A, O0A);          // E0 - O0 + off
                const __m128i T2FB = _mm_sub_epi32(T10B, O0B);
                const __m128i T2EA = _mm_sub_epi32(T11A, O1A);          // E1 - O1 + off
                const __m128i T2EB = _mm_sub_epi32(T11B, O1B);
                const __m128i T2DA = _mm_sub_epi32(T12A, O2A);          // E2 - O2 + off
                const __m128i T2DB = _mm_sub_epi32(T12B, O2B);
                const __m128i T2CA = _mm_sub_epi32(T13A, O3A);          // E3 - O3 + off
                const __m128i T2CB = _mm_sub_epi32(T13B, O3B);
                const __m128i T2BA = _mm_sub_epi32(T14A, O4A);          // E4
                const __m128i T2BB = _mm_sub_epi32(T14B, O4B);
                const __m128i T2AA = _mm_sub_epi32(T15A, O5A);          // E5
                const __m128i T2AB = _mm_sub_epi32(T15B, O5B);
                const __m128i T29A = _mm_sub_epi32(T16A, O6A);          // E6
                const __m128i T29B = _mm_sub_epi32(T16B, O6B);
                const __m128i T28A = _mm_sub_epi32(T17A, O7A);          // E7
                const __m128i T28B = _mm_sub_epi32(T17B, O7B);

                const __m128i T30A = _mm_srai_epi32(T20A, shift);             // [30 20 10 00]
                const __m128i T30B = _mm_srai_epi32(T20B, shift);             // [70 60 50 40]
                const __m128i T31A = _mm_srai_epi32(T21A, shift);             // [31 21 11 01]
                const __m128i T31B = _mm_srai_epi32(T21B, shift);             // [71 61 51 41]
                const __m128i T32A = _mm_srai_epi32(T22A, shift);             // [32 22 12 02]
                const __m128i T32B = _mm_srai_epi32(T22B, shift);             // [72 62 52 42]
                const __m128i T33A = _mm_srai_epi32(T23A, shift);             // [33 23 13 03]
                const __m128i T33B = _mm_srai_epi32(T23B, shift);             // [73 63 53 43]
                const __m128i T34A = _mm_srai_epi32(T24A, shift);             // [33 24 14 04]
                const __m128i T34B = _mm_srai_epi32(T24B, shift);             // [74 64 54 44]
                const __m128i T35A = _mm_srai_epi32(T25A, shift);             // [35 25 15 05]
                const __m128i T35B = _mm_srai_epi32(T25B, shift);             // [75 65 55 45]
                const __m128i T36A = _mm_srai_epi32(T26A, shift);             // [36 26 16 06]
                const __m128i T36B = _mm_srai_epi32(T26B, shift);             // [76 66 56 46]
                const __m128i T37A = _mm_srai_epi32(T27A, shift);             // [37 27 17 07]
                const __m128i T37B = _mm_srai_epi32(T27B, shift);             // [77 67 57 47]

                const __m128i T38A = _mm_srai_epi32(T28A, shift);             // [30 20 10 00] x8
                const __m128i T38B = _mm_srai_epi32(T28B, shift);             // [70 60 50 40]
                const __m128i T39A = _mm_srai_epi32(T29A, shift);             // [31 21 11 01] x9
                const __m128i T39B = _mm_srai_epi32(T29B, shift);             // [71 61 51 41]
                const __m128i T3AA = _mm_srai_epi32(T2AA, shift);             // [32 22 12 02] xA
                const __m128i T3AB = _mm_srai_epi32(T2AB, shift);             // [72 62 52 42]
                const __m128i T3BA = _mm_srai_epi32(T2BA, shift);             // [33 23 13 03] xB
                const __m128i T3BB = _mm_srai_epi32(T2BB, shift);             // [73 63 53 43]
                const __m128i T3CA = _mm_srai_epi32(T2CA, shift);             // [33 24 14 04] xC
                const __m128i T3CB = _mm_srai_epi32(T2CB, shift);             // [74 64 54 44]
                const __m128i T3DA = _mm_srai_epi32(T2DA, shift);             // [35 25 15 05] xD
                const __m128i T3DB = _mm_srai_epi32(T2DB, shift);             // [75 65 55 45]
                const __m128i T3EA = _mm_srai_epi32(T2EA, shift);             // [36 26 16 06] xE
                const __m128i T3EB = _mm_srai_epi32(T2EB, shift);             // [76 66 56 46]
                const __m128i T3FA = _mm_srai_epi32(T2FA, shift);             // [37 27 17 07] xF
                const __m128i T3FB = _mm_srai_epi32(T2FB, shift);             // [77 67 57 47]

                res00[part] = _mm_packs_epi32(T30A, T30B);        // [70 60 50 40 30 20 10 00]
                res01[part] = _mm_packs_epi32(T31A, T31B);        // [71 61 51 41 31 21 11 01]
                res02[part] = _mm_packs_epi32(T32A, T32B);        // [72 62 52 42 32 22 12 02]
                res03[part] = _mm_packs_epi32(T33A, T33B);        // [73 63 53 43 33 23 13 03]
                res04[part] = _mm_packs_epi32(T34A, T34B);        // [74 64 54 44 34 24 14 04]
                res05[part] = _mm_packs_epi32(T35A, T35B);        // [75 65 55 45 35 25 15 05]
                res06[part] = _mm_packs_epi32(T36A, T36B);        // [76 66 56 46 36 26 16 06]
                res07[part] = _mm_packs_epi32(T37A, T37B);        // [77 67 57 47 37 27 17 07]

                res08[part] = _mm_packs_epi32(T38A, T38B);        // [A0 ... 80]
                res09[part] = _mm_packs_epi32(T39A, T39B);        // [A1 ... 81]
                res10[part] = _mm_packs_epi32(T3AA, T3AB);        // [A2 ... 82]
                res11[part] = _mm_packs_epi32(T3BA, T3BB);        // [A3 ... 83]
                res12[part] = _mm_packs_epi32(T3CA, T3CB);        // [A4 ... 84]
                res13[part] = _mm_packs_epi32(T3DA, T3DB);        // [A5 ... 85]
                res14[part] = _mm_packs_epi32(T3EA, T3EB);        // [A6 ... 86]
                res15[part] = _mm_packs_epi32(T3FA, T3FB);        // [A7 ... 87]
            }
        }
        //transpose matrix 16x16 16bit.
        {
            __m128i tr0_0, tr0_1, tr0_2, tr0_3, tr0_4, tr0_5, tr0_6, tr0_7;
            __m128i tr1_0, tr1_1, tr1_2, tr1_3, tr1_4, tr1_5, tr1_6, tr1_7;

            TRANSPOSE_8x8_16BIT(res00[0], res01[0], res02[0], res03[0], res04[0], res05[0], res06[0], res07[0], in00[0], in01[0], in02[0], in03[0], in04[0], in05[0], in06[0], in07[0])
            TRANSPOSE_8x8_16BIT(res08[0], res09[0], res10[0], res11[0], res12[0], res13[0], res14[0], res15[0], in00[1], in01[1], in02[1], in03[1], in04[1], in05[1], in06[1], in07[1])
            TRANSPOSE_8x8_16BIT(res00[1], res01[1], res02[1], res03[1], res04[1], res05[1], res06[1], res07[1], in08[0], in09[0], in10[0], in11[0], in12[0], in13[0], in14[0], in15[0])
            TRANSPOSE_8x8_16BIT(res08[1], res09[1], res10[1], res11[1], res12[1], res13[1], res14[1], res15[1], in08[1], in09[1], in10[1], in11[1], in12[1], in13[1], in14[1], in15[1])

        }
    }

    {   //clip
        __m128i max_val = _mm_set1_epi16((1 << bit_depth) - 1);
        __m128i min_val = _mm_set1_epi16(-(1 << bit_depth));

        in00[0] = _mm_min_epi16(in00[0], max_val);
        in00[1] = _mm_min_epi16(in00[1], max_val);
        in01[0] = _mm_min_epi16(in01[0], max_val);
        in01[1] = _mm_min_epi16(in01[1], max_val);
        in00[0] = _mm_max_epi16(in00[0], min_val);
        in00[1] = _mm_max_epi16(in00[1], min_val);
        in01[0] = _mm_max_epi16(in01[0], min_val);
        in01[1] = _mm_max_epi16(in01[1], min_val);

        in02[0] = _mm_min_epi16(in02[0], max_val);
        in02[1] = _mm_min_epi16(in02[1], max_val);
        in03[0] = _mm_min_epi16(in03[0], max_val);
        in03[1] = _mm_min_epi16(in03[1], max_val);
        in02[0] = _mm_max_epi16(in02[0], min_val);
        in02[1] = _mm_max_epi16(in02[1], min_val);
        in03[0] = _mm_max_epi16(in03[0], min_val);
        in03[1] = _mm_max_epi16(in03[1], min_val);

        in04[0] = _mm_min_epi16(in04[0], max_val);
        in04[1] = _mm_min_epi16(in04[1], max_val);
        in05[0] = _mm_min_epi16(in05[0], max_val);
        in05[1] = _mm_min_epi16(in05[1], max_val);
        in04[0] = _mm_max_epi16(in04[0], min_val);
        in04[1] = _mm_max_epi16(in04[1], min_val);
        in05[0] = _mm_max_epi16(in05[0], min_val);
        in05[1] = _mm_max_epi16(in05[1], min_val);

        in06[0] = _mm_min_epi16(in06[0], max_val);
        in06[1] = _mm_min_epi16(in06[1], max_val);
        in07[0] = _mm_min_epi16(in07[0], max_val);
        in07[1] = _mm_min_epi16(in07[1], max_val);
        in06[0] = _mm_max_epi16(in06[0], min_val);
        in06[1] = _mm_max_epi16(in06[1], min_val);
        in07[0] = _mm_max_epi16(in07[0], min_val);
        in07[1] = _mm_max_epi16(in07[1], min_val);

        in08[0] = _mm_min_epi16(in08[0], max_val);
        in08[1] = _mm_min_epi16(in08[1], max_val);
        in09[0] = _mm_min_epi16(in09[0], max_val);
        in09[1] = _mm_min_epi16(in09[1], max_val);
        in08[0] = _mm_max_epi16(in08[0], min_val);
        in08[1] = _mm_max_epi16(in08[1], min_val);
        in09[0] = _mm_max_epi16(in09[0], min_val);
        in09[1] = _mm_max_epi16(in09[1], min_val);

        in10[0] = _mm_min_epi16(in10[0], max_val);
        in10[1] = _mm_min_epi16(in10[1], max_val);
        in11[0] = _mm_min_epi16(in11[0], max_val);
        in11[1] = _mm_min_epi16(in11[1], max_val);
        in10[0] = _mm_max_epi16(in10[0], min_val);
        in10[1] = _mm_max_epi16(in10[1], min_val);
        in11[0] = _mm_max_epi16(in11[0], min_val);
        in11[1] = _mm_max_epi16(in11[1], min_val);

        in12[0] = _mm_min_epi16(in12[0], max_val);
        in12[1] = _mm_min_epi16(in12[1], max_val);
        in13[0] = _mm_min_epi16(in13[0], max_val);
        in13[1] = _mm_min_epi16(in13[1], max_val);
        in12[0] = _mm_max_epi16(in12[0], min_val);
        in12[1] = _mm_max_epi16(in12[1], min_val);
        in13[0] = _mm_max_epi16(in13[0], min_val);
        in13[1] = _mm_max_epi16(in13[1], min_val);

        in14[0] = _mm_min_epi16(in14[0], max_val);
        in14[1] = _mm_min_epi16(in14[1], max_val);
        in15[0] = _mm_min_epi16(in15[0], max_val);
        in15[1] = _mm_min_epi16(in15[1], max_val);
        in14[0] = _mm_max_epi16(in14[0], min_val);
        in14[1] = _mm_max_epi16(in14[1], min_val);
        in15[0] = _mm_max_epi16(in15[0], min_val);
        in15[1] = _mm_max_epi16(in15[1], min_val);
    }

    _mm_store_si128((__m128i*)&dst[0], in00[0]);
    _mm_store_si128((__m128i*)&dst[8], in00[1]);
    _mm_store_si128((__m128i*)&dst[1 * 16    ], in01[0]);
    _mm_store_si128((__m128i*)&dst[1 * 16 + 8], in01[1]);
    _mm_store_si128((__m128i*)&dst[2 * 16    ], in02[0]);
    _mm_store_si128((__m128i*)&dst[2 * 16 + 8], in02[1]);
    _mm_store_si128((__m128i*)&dst[3 * 16    ], in03[0]);
    _mm_store_si128((__m128i*)&dst[3 * 16 + 8], in03[1]);
    _mm_store_si128((__m128i*)&dst[4 * 16    ], in04[0]);
    _mm_store_si128((__m128i*)&dst[4 * 16 + 8], in04[1]);
    _mm_store_si128((__m128i*)&dst[5 * 16    ], in05[0]);
    _mm_store_si128((__m128i*)&dst[5 * 16 + 8], in05[1]);
    _mm_store_si128((__m128i*)&dst[6 * 16    ], in06[0]);
    _mm_store_si128((__m128i*)&dst[6 * 16 + 8], in06[1]);
    _mm_store_si128((__m128i*)&dst[7 * 16    ], in07[0]);
    _mm_store_si128((__m128i*)&dst[7 * 16 + 8], in07[1]);
    _mm_store_si128((__m128i*)&dst[8 * 16    ], in08[0]);
    _mm_store_si128((__m128i*)&dst[8 * 16 + 8], in08[1]);
    _mm_store_si128((__m128i*)&dst[9 * 16    ], in09[0]);
    _mm_store_si128((__m128i*)&dst[9 * 16 + 8], in09[1]);
    _mm_store_si128((__m128i*)&dst[10 * 16    ], in10[0]);
    _mm_store_si128((__m128i*)&dst[10 * 16 + 8], in10[1]);
    _mm_store_si128((__m128i*)&dst[11 * 16    ], in11[0]);
    _mm_store_si128((__m128i*)&dst[11 * 16 + 8], in11[1]);
    _mm_store_si128((__m128i*)&dst[12 * 16    ], in12[0]);
    _mm_store_si128((__m128i*)&dst[12 * 16 + 8], in12[1]);
    _mm_store_si128((__m128i*)&dst[13 * 16    ], in13[0]);
    _mm_store_si128((__m128i*)&dst[13 * 16 + 8], in13[1]);
    _mm_store_si128((__m128i*)&dst[14 * 16    ], in14[0]);
    _mm_store_si128((__m128i*)&dst[14 * 16 + 8], in14[1]);
    _mm_store_si128((__m128i*)&dst[15 * 16    ], in15[0]);
    _mm_store_si128((__m128i*)&dst[15 * 16 + 8], in15[1]);
}

void itrans_dct2_h32_w32_sse(s16 *src, s16 *dst, int bit_depth)
{
    int k;
    
    const __m128i c16_p45_p45 = _mm_set1_epi32(0x002D002D);
    const __m128i c16_p43_p44 = _mm_set1_epi32(0x002B002C);
    const __m128i c16_p39_p41 = _mm_set1_epi32(0x00270029);
    const __m128i c16_p34_p36 = _mm_set1_epi32(0x00220024);
    const __m128i c16_p27_p30 = _mm_set1_epi32(0x001B001E);
    const __m128i c16_p19_p23 = _mm_set1_epi32(0x00130017);
    const __m128i c16_p11_p15 = _mm_set1_epi32(0x000B000F);
    const __m128i c16_p02_p07 = _mm_set1_epi32(0x00020007);
    const __m128i c16_p41_p45 = _mm_set1_epi32(0x0029002D);
    const __m128i c16_p23_p34 = _mm_set1_epi32(0x00170022);
    const __m128i c16_n02_p11 = _mm_set1_epi32(0xFFFE000B);
    const __m128i c16_n27_n15 = _mm_set1_epi32(0xFFE5FFF1);
    const __m128i c16_n43_n36 = _mm_set1_epi32(0xFFD5FFDC);
    const __m128i c16_n44_n45 = _mm_set1_epi32(0xFFD4FFD3);
    const __m128i c16_n30_n39 = _mm_set1_epi32(0xFFE2FFD9);
    const __m128i c16_n07_n19 = _mm_set1_epi32(0xFFF9FFED);
    const __m128i c16_p34_p44 = _mm_set1_epi32(0x0022002C);
    const __m128i c16_n07_p15 = _mm_set1_epi32(0xFFF9000F);
    const __m128i c16_n41_n27 = _mm_set1_epi32(0xFFD7FFE5);
    const __m128i c16_n39_n45 = _mm_set1_epi32(0xFFD9FFD3);
    const __m128i c16_n02_n23 = _mm_set1_epi32(0xFFFEFFE9);
    const __m128i c16_p36_p19 = _mm_set1_epi32(0x00240013);
    const __m128i c16_p43_p45 = _mm_set1_epi32(0x002B002D);
    const __m128i c16_p11_p30 = _mm_set1_epi32(0x000B001E);
    const __m128i c16_p23_p43 = _mm_set1_epi32(0x0017002B);
    const __m128i c16_n34_n07 = _mm_set1_epi32(0xFFDEFFF9);
    const __m128i c16_n36_n45 = _mm_set1_epi32(0xFFDCFFD3);
    const __m128i c16_p19_n11 = _mm_set1_epi32(0x0013FFF5);
    const __m128i c16_p44_p41 = _mm_set1_epi32(0x002C0029);
    const __m128i c16_n02_p27 = _mm_set1_epi32(0xFFFE001B);
    const __m128i c16_n45_n30 = _mm_set1_epi32(0xFFD3FFE2);
    const __m128i c16_n15_n39 = _mm_set1_epi32(0xFFF1FFD9);
    const __m128i c16_p11_p41 = _mm_set1_epi32(0x000B0029);
    const __m128i c16_n45_n27 = _mm_set1_epi32(0xFFD3FFE5);
    const __m128i c16_p07_n30 = _mm_set1_epi32(0x0007FFE2);
    const __m128i c16_p43_p39 = _mm_set1_epi32(0x002B0027);
    const __m128i c16_n23_p15 = _mm_set1_epi32(0xFFE9000F);
    const __m128i c16_n34_n45 = _mm_set1_epi32(0xFFDEFFD3);
    const __m128i c16_p36_p02 = _mm_set1_epi32(0x00240002);
    const __m128i c16_p19_p44 = _mm_set1_epi32(0x0013002C);
    const __m128i c16_n02_p39 = _mm_set1_epi32(0xFFFE0027);
    const __m128i c16_n36_n41 = _mm_set1_epi32(0xFFDCFFD7);
    const __m128i c16_p43_p07 = _mm_set1_epi32(0x002B0007);
    const __m128i c16_n11_p34 = _mm_set1_epi32(0xFFF50022);
    const __m128i c16_n30_n44 = _mm_set1_epi32(0xFFE2FFD4);
    const __m128i c16_p45_p15 = _mm_set1_epi32(0x002D000F);
    const __m128i c16_n19_p27 = _mm_set1_epi32(0xFFED001B);
    const __m128i c16_n23_n45 = _mm_set1_epi32(0xFFE9FFD3);
    const __m128i c16_n15_p36 = _mm_set1_epi32(0xFFF10024);
    const __m128i c16_n11_n45 = _mm_set1_epi32(0xFFF5FFD3);
    const __m128i c16_p34_p39 = _mm_set1_epi32(0x00220027);
    const __m128i c16_n45_n19 = _mm_set1_epi32(0xFFD3FFED);
    const __m128i c16_p41_n07 = _mm_set1_epi32(0x0029FFF9);
    const __m128i c16_n23_p30 = _mm_set1_epi32(0xFFE9001E);
    const __m128i c16_n02_n44 = _mm_set1_epi32(0xFFFEFFD4);
    const __m128i c16_p27_p43 = _mm_set1_epi32(0x001B002B);
    const __m128i c16_n27_p34 = _mm_set1_epi32(0xFFE50022);
    const __m128i c16_p19_n39 = _mm_set1_epi32(0x0013FFD9);
    const __m128i c16_n11_p43 = _mm_set1_epi32(0xFFF5002B);
    const __m128i c16_p02_n45 = _mm_set1_epi32(0x0002FFD3);
    const __m128i c16_p07_p45 = _mm_set1_epi32(0x0007002D);
    const __m128i c16_n15_n44 = _mm_set1_epi32(0xFFF1FFD4);
    const __m128i c16_p23_p41 = _mm_set1_epi32(0x00170029);
    const __m128i c16_n30_n36 = _mm_set1_epi32(0xFFE2FFDC);
    const __m128i c16_n36_p30 = _mm_set1_epi32(0xFFDC001E);
    const __m128i c16_p41_n23 = _mm_set1_epi32(0x0029FFE9);
    const __m128i c16_n44_p15 = _mm_set1_epi32(0xFFD4000F);
    const __m128i c16_p45_n07 = _mm_set1_epi32(0x002DFFF9);
    const __m128i c16_n45_n02 = _mm_set1_epi32(0xFFD3FFFE);
    const __m128i c16_p43_p11 = _mm_set1_epi32(0x002B000B);
    const __m128i c16_n39_n19 = _mm_set1_epi32(0xFFD9FFED);
    const __m128i c16_p34_p27 = _mm_set1_epi32(0x0022001B);
    const __m128i c16_n43_p27 = _mm_set1_epi32(0xFFD5001B);
    const __m128i c16_p44_n02 = _mm_set1_epi32(0x002CFFFE);
    const __m128i c16_n30_n23 = _mm_set1_epi32(0xFFE2FFE9);
    const __m128i c16_p07_p41 = _mm_set1_epi32(0x00070029);
    const __m128i c16_p19_n45 = _mm_set1_epi32(0x0013FFD3);
    const __m128i c16_n39_p34 = _mm_set1_epi32(0xFFD90022);
    const __m128i c16_p45_n11 = _mm_set1_epi32(0x002DFFF5);
    const __m128i c16_n36_n15 = _mm_set1_epi32(0xFFDCFFF1);
    const __m128i c16_n45_p23 = _mm_set1_epi32(0xFFD30017);
    const __m128i c16_p27_p19 = _mm_set1_epi32(0x001B0013);
    const __m128i c16_p15_n45 = _mm_set1_epi32(0x000FFFD3);
    const __m128i c16_n44_p30 = _mm_set1_epi32(0xFFD4001E);
    const __m128i c16_p34_p11 = _mm_set1_epi32(0x0022000B);
    const __m128i c16_p07_n43 = _mm_set1_epi32(0x0007FFD5);
    const __m128i c16_n41_p36 = _mm_set1_epi32(0xFFD70024);
    const __m128i c16_p39_p02 = _mm_set1_epi32(0x00270002);
    const __m128i c16_n44_p19 = _mm_set1_epi32(0xFFD40013);
    const __m128i c16_n02_p36 = _mm_set1_epi32(0xFFFE0024);
    const __m128i c16_p45_n34 = _mm_set1_epi32(0x002DFFDE);
    const __m128i c16_n15_n23 = _mm_set1_epi32(0xFFF1FFE9);
    const __m128i c16_n39_p43 = _mm_set1_epi32(0xFFD9002B);
    const __m128i c16_p30_p07 = _mm_set1_epi32(0x001E0007);
    const __m128i c16_p27_n45 = _mm_set1_epi32(0x001BFFD3);
    const __m128i c16_n41_p11 = _mm_set1_epi32(0xFFD7000B);
    const __m128i c16_n39_p15 = _mm_set1_epi32(0xFFD9000F);
    const __m128i c16_n30_p45 = _mm_set1_epi32(0xFFE2002D);
    const __m128i c16_p27_p02 = _mm_set1_epi32(0x001B0002);
    const __m128i c16_p41_n44 = _mm_set1_epi32(0x0029FFD4);
    const __m128i c16_n11_n19 = _mm_set1_epi32(0xFFF5FFED);
    const __m128i c16_n45_p36 = _mm_set1_epi32(0xFFD30024);
    const __m128i c16_n07_p34 = _mm_set1_epi32(0xFFF90022);
    const __m128i c16_p43_n23 = _mm_set1_epi32(0x002BFFE9);
    const __m128i c16_n30_p11 = _mm_set1_epi32(0xFFE2000B);
    const __m128i c16_n45_p43 = _mm_set1_epi32(0xFFD3002B);
    const __m128i c16_n19_p36 = _mm_set1_epi32(0xFFED0024);
    const __m128i c16_p23_n02 = _mm_set1_epi32(0x0017FFFE);
    const __m128i c16_p45_n39 = _mm_set1_epi32(0x002DFFD9);
    const __m128i c16_p27_n41 = _mm_set1_epi32(0x001BFFD7);
    const __m128i c16_n15_n07 = _mm_set1_epi32(0xFFF1FFF9);
    const __m128i c16_n44_p34 = _mm_set1_epi32(0xFFD40022);
    const __m128i c16_n19_p07 = _mm_set1_epi32(0xFFED0007);
    const __m128i c16_n39_p30 = _mm_set1_epi32(0xFFD9001E);
    const __m128i c16_n45_p44 = _mm_set1_epi32(0xFFD3002C);
    const __m128i c16_n36_p43 = _mm_set1_epi32(0xFFDC002B);
    const __m128i c16_n15_p27 = _mm_set1_epi32(0xFFF1001B);
    const __m128i c16_p11_p02 = _mm_set1_epi32(0x000B0002);
    const __m128i c16_p34_n23 = _mm_set1_epi32(0x0022FFE9);
    const __m128i c16_p45_n41 = _mm_set1_epi32(0x002DFFD7);
    const __m128i c16_n07_p02 = _mm_set1_epi32(0xFFF90002);
    const __m128i c16_n15_p11 = _mm_set1_epi32(0xFFF1000B);
    const __m128i c16_n23_p19 = _mm_set1_epi32(0xFFE90013);
    const __m128i c16_n30_p27 = _mm_set1_epi32(0xFFE2001B);
    const __m128i c16_n36_p34 = _mm_set1_epi32(0xFFDC0022);
    const __m128i c16_n41_p39 = _mm_set1_epi32(0xFFD70027);
    const __m128i c16_n44_p43 = _mm_set1_epi32(0xFFD4002B);
    const __m128i c16_n45_p45 = _mm_set1_epi32(0xFFD3002D);

    const __m128i c16_p35_p40 = _mm_set1_epi32(0x00230028);
    const __m128i c16_p21_p29 = _mm_set1_epi32(0x0015001D);
    const __m128i c16_p04_p13 = _mm_set1_epi32(0x0004000D);
    const __m128i c16_p29_p43 = _mm_set1_epi32(0x001D002B);
    const __m128i c16_n21_p04 = _mm_set1_epi32(0xFFEB0004);
    const __m128i c16_n45_n40 = _mm_set1_epi32(0xFFD3FFD8);
    const __m128i c16_n13_n35 = _mm_set1_epi32(0xFFF3FFDD);
    const __m128i c16_p04_p40 = _mm_set1_epi32(0x00040028);
    const __m128i c16_n43_n35 = _mm_set1_epi32(0xFFD5FFDD);
    const __m128i c16_p29_n13 = _mm_set1_epi32(0x001DFFF3);
    const __m128i c16_p21_p45 = _mm_set1_epi32(0x0015002D);
    const __m128i c16_n21_p35 = _mm_set1_epi32(0xFFEB0023);
    const __m128i c16_p04_n43 = _mm_set1_epi32(0x0004FFD5);
    const __m128i c16_p13_p45 = _mm_set1_epi32(0x000D002D);
    const __m128i c16_n29_n40 = _mm_set1_epi32(0xFFE3FFD8);
    const __m128i c16_n40_p29 = _mm_set1_epi32(0xFFD8001D);
    const __m128i c16_p45_n13 = _mm_set1_epi32(0x002DFFF3);
    const __m128i c16_n43_n04 = _mm_set1_epi32(0xFFD5FFFC);
    const __m128i c16_p35_p21 = _mm_set1_epi32(0x00230015);
    const __m128i c16_n45_p21 = _mm_set1_epi32(0xFFD30015);
    const __m128i c16_p13_p29 = _mm_set1_epi32(0x000D001D);
    const __m128i c16_p35_n43 = _mm_set1_epi32(0x0023FFD5);
    const __m128i c16_n40_p04 = _mm_set1_epi32(0xFFD80004);
    const __m128i c16_n35_p13 = _mm_set1_epi32(0xFFDD000D);
    const __m128i c16_n40_p45 = _mm_set1_epi32(0xFFD8002D);
    const __m128i c16_p04_p21 = _mm_set1_epi32(0x00040015);
    const __m128i c16_p43_n29 = _mm_set1_epi32(0x002BFFE3);
    const __m128i c16_n13_p04 = _mm_set1_epi32(0xFFF30004);
    const __m128i c16_n29_p21 = _mm_set1_epi32(0xFFE30015);
    const __m128i c16_n40_p35 = _mm_set1_epi32(0xFFD80023);
    
    const __m128i c16_p38_p44 = _mm_set1_epi32(0x0026002C);
    const __m128i c16_p09_p25 = _mm_set1_epi32(0x00090019);
    const __m128i c16_n09_p38 = _mm_set1_epi32(0xFFF70026);
    const __m128i c16_n25_n44 = _mm_set1_epi32(0xFFE7FFD4);

    const __m128i c16_n44_p25 = _mm_set1_epi32(0xFFD40019);
    const __m128i c16_p38_p09 = _mm_set1_epi32(0x00260009);
    const __m128i c16_n25_p09 = _mm_set1_epi32(0xFFE70009);
    const __m128i c16_n44_p38 = _mm_set1_epi32(0xFFD40026);

    const __m128i c16_p17_p42 = _mm_set1_epi32(0x0011002A);
    const __m128i c16_n42_p17 = _mm_set1_epi32(0xFFD60011);

    const __m128i c16_p32_p32 = _mm_set1_epi32(0x00200020);
    const __m128i c16_n32_p32 = _mm_set1_epi32(0xFFE00020);

    __m128i c32_off = _mm_set1_epi32(16);

    int shift = 5;
    int i, pass, part;

    __m128i in00[4], in01[4], in02[4], in03[4], in04[4], in05[4], in06[4], in07[4], in08[4], in09[4], in10[4], in11[4], in12[4], in13[4], in14[4], in15[4];
    __m128i in16[4], in17[4], in18[4], in19[4], in20[4], in21[4], in22[4], in23[4], in24[4], in25[4], in26[4], in27[4], in28[4], in29[4], in30[4], in31[4];
    __m128i res00[4], res01[4], res02[4], res03[4], res04[4], res05[4], res06[4], res07[4], res08[4], res09[4], res10[4], res11[4], res12[4], res13[4], res14[4], res15[4];
    __m128i res16[4], res17[4], res18[4], res19[4], res20[4], res21[4], res22[4], res23[4], res24[4], res25[4], res26[4], res27[4], res28[4], res29[4], res30[4], res31[4];

    for (i = 0; i < 4; i++)
    {
        const int offset = (i << 3);

        in00[i] = _mm_load_si128((const __m128i*)&src[0 * 32 + offset]);
        in01[i] = _mm_load_si128((const __m128i*)&src[1 * 32 + offset]);
        in02[i] = _mm_load_si128((const __m128i*)&src[2 * 32 + offset]);
        in03[i] = _mm_load_si128((const __m128i*)&src[3 * 32 + offset]);
        in04[i] = _mm_load_si128((const __m128i*)&src[4 * 32 + offset]);
        in05[i] = _mm_load_si128((const __m128i*)&src[5 * 32 + offset]);
        in06[i] = _mm_load_si128((const __m128i*)&src[6 * 32 + offset]);
        in07[i] = _mm_load_si128((const __m128i*)&src[7 * 32 + offset]);
        in08[i] = _mm_load_si128((const __m128i*)&src[8 * 32 + offset]);
        in09[i] = _mm_load_si128((const __m128i*)&src[9 * 32 + offset]);
        in10[i] = _mm_load_si128((const __m128i*)&src[10 * 32 + offset]);
        in11[i] = _mm_load_si128((const __m128i*)&src[11 * 32 + offset]);
        in12[i] = _mm_load_si128((const __m128i*)&src[12 * 32 + offset]);
        in13[i] = _mm_load_si128((const __m128i*)&src[13 * 32 + offset]);
        in14[i] = _mm_load_si128((const __m128i*)&src[14 * 32 + offset]);
        in15[i] = _mm_load_si128((const __m128i*)&src[15 * 32 + offset]);
        in16[i] = _mm_load_si128((const __m128i*)&src[16 * 32 + offset]);
        in17[i] = _mm_load_si128((const __m128i*)&src[17 * 32 + offset]);
        in18[i] = _mm_load_si128((const __m128i*)&src[18 * 32 + offset]);
        in19[i] = _mm_load_si128((const __m128i*)&src[19 * 32 + offset]);
        in20[i] = _mm_load_si128((const __m128i*)&src[20 * 32 + offset]);
        in21[i] = _mm_load_si128((const __m128i*)&src[21 * 32 + offset]);
        in22[i] = _mm_load_si128((const __m128i*)&src[22 * 32 + offset]);
        in23[i] = _mm_load_si128((const __m128i*)&src[23 * 32 + offset]);
        in24[i] = _mm_load_si128((const __m128i*)&src[24 * 32 + offset]);
        in25[i] = _mm_load_si128((const __m128i*)&src[25 * 32 + offset]);
        in26[i] = _mm_load_si128((const __m128i*)&src[26 * 32 + offset]);
        in27[i] = _mm_load_si128((const __m128i*)&src[27 * 32 + offset]);
        in28[i] = _mm_load_si128((const __m128i*)&src[28 * 32 + offset]);
        in29[i] = _mm_load_si128((const __m128i*)&src[29 * 32 + offset]);
        in30[i] = _mm_load_si128((const __m128i*)&src[30 * 32 + offset]);
        in31[i] = _mm_load_si128((const __m128i*)&src[31 * 32 + offset]);
    }

    for (pass = 0; pass < 2; pass++)
    {
        if (pass == 1)
        {
            c32_off = _mm_set1_epi32(1 << (19 - bit_depth));				
            shift = 20 - bit_depth;
        }

        for (part = 0; part < 4; part++)
        {
            const __m128i T_00_00A = _mm_unpacklo_epi16(in01[part], in03[part]);       // [33 13 32 12 31 11 30 10]
            const __m128i T_00_00B = _mm_unpackhi_epi16(in01[part], in03[part]);       // [37 17 36 16 35 15 34 14]
            const __m128i T_00_01A = _mm_unpacklo_epi16(in05[part], in07[part]);       // [ ]
            const __m128i T_00_01B = _mm_unpackhi_epi16(in05[part], in07[part]);       // [ ]
            const __m128i T_00_02A = _mm_unpacklo_epi16(in09[part], in11[part]);       // [ ]
            const __m128i T_00_02B = _mm_unpackhi_epi16(in09[part], in11[part]);       // [ ]
            const __m128i T_00_03A = _mm_unpacklo_epi16(in13[part], in15[part]);       // [ ]
            const __m128i T_00_03B = _mm_unpackhi_epi16(in13[part], in15[part]);       // [ ]
            const __m128i T_00_04A = _mm_unpacklo_epi16(in17[part], in19[part]);       // [ ]
            const __m128i T_00_04B = _mm_unpackhi_epi16(in17[part], in19[part]);       // [ ]
            const __m128i T_00_05A = _mm_unpacklo_epi16(in21[part], in23[part]);       // [ ]
            const __m128i T_00_05B = _mm_unpackhi_epi16(in21[part], in23[part]);       // [ ]
            const __m128i T_00_06A = _mm_unpacklo_epi16(in25[part], in27[part]);       // [ ]
            const __m128i T_00_06B = _mm_unpackhi_epi16(in25[part], in27[part]);       // [ ]
            const __m128i T_00_07A = _mm_unpacklo_epi16(in29[part], in31[part]);       //
            const __m128i T_00_07B = _mm_unpackhi_epi16(in29[part], in31[part]);       // [ ]

            const __m128i T_00_08A = _mm_unpacklo_epi16(in02[part], in06[part]);       // [ ]
            const __m128i T_00_08B = _mm_unpackhi_epi16(in02[part], in06[part]);       // [ ]
            const __m128i T_00_09A = _mm_unpacklo_epi16(in10[part], in14[part]);       // [ ]
            const __m128i T_00_09B = _mm_unpackhi_epi16(in10[part], in14[part]);       // [ ]
            const __m128i T_00_10A = _mm_unpacklo_epi16(in18[part], in22[part]);       // [ ]
            const __m128i T_00_10B = _mm_unpackhi_epi16(in18[part], in22[part]);       // [ ]
            const __m128i T_00_11A = _mm_unpacklo_epi16(in26[part], in30[part]);       // [ ]
            const __m128i T_00_11B = _mm_unpackhi_epi16(in26[part], in30[part]);       // [ ]

            const __m128i T_00_12A = _mm_unpacklo_epi16(in04[part], in12[part]);       // [ ]
            const __m128i T_00_12B = _mm_unpackhi_epi16(in04[part], in12[part]);       // [ ]
            const __m128i T_00_13A = _mm_unpacklo_epi16(in20[part], in28[part]);       // [ ]
            const __m128i T_00_13B = _mm_unpackhi_epi16(in20[part], in28[part]);       // [ ]

            const __m128i T_00_14A = _mm_unpacklo_epi16(in08[part], in24[part]);       //
            const __m128i T_00_14B = _mm_unpackhi_epi16(in08[part], in24[part]);       // [ ]
            const __m128i T_00_15A = _mm_unpacklo_epi16(in00[part], in16[part]);       //
            const __m128i T_00_15B = _mm_unpackhi_epi16(in00[part], in16[part]);       // [ ]

            __m128i O00A, O01A, O02A, O03A, O04A, O05A, O06A, O07A, O08A, O09A, O10A, O11A, O12A, O13A, O14A, O15A;
            __m128i O00B, O01B, O02B, O03B, O04B, O05B, O06B, O07B, O08B, O09B, O10B, O11B, O12B, O13B, O14B, O15B;
            __m128i EO0A, EO1A, EO2A, EO3A, EO4A, EO5A, EO6A, EO7A;
            __m128i EO0B, EO1B, EO2B, EO3B, EO4B, EO5B, EO6B, EO7B;
            {
                __m128i T00, T01, T02, T03;
#define COMPUTE_ROW(r0103, r0507, r0911, r1315, r1719, r2123, r2527, r2931, c0103, c0507, c0911, c1315, c1719, c2123, c2527, c2931, row) \
	            T00 = _mm_add_epi32(_mm_madd_epi16(r0103, c0103), _mm_madd_epi16(r0507, c0507)); \
	            T01 = _mm_add_epi32(_mm_madd_epi16(r0911, c0911), _mm_madd_epi16(r1315, c1315)); \
	            T02 = _mm_add_epi32(_mm_madd_epi16(r1719, c1719), _mm_madd_epi16(r2123, c2123)); \
	            T03 = _mm_add_epi32(_mm_madd_epi16(r2527, c2527), _mm_madd_epi16(r2931, c2931)); \
	            row = _mm_add_epi32(_mm_add_epi32(T00, T01), _mm_add_epi32(T02, T03));

                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p45_p45, c16_p43_p44, c16_p39_p41, c16_p34_p36, c16_p27_p30, c16_p19_p23, c16_p11_p15, c16_p02_p07, O00A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p41_p45, c16_p23_p34, c16_n02_p11, c16_n27_n15, c16_n43_n36, c16_n44_n45, c16_n30_n39, c16_n07_n19, O01A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p34_p44, c16_n07_p15, c16_n41_n27, c16_n39_n45, c16_n02_n23, c16_p36_p19, c16_p43_p45, c16_p11_p30, O02A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p23_p43, c16_n34_n07, c16_n36_n45, c16_p19_n11, c16_p44_p41, c16_n02_p27, c16_n45_n30, c16_n15_n39, O03A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_p11_p41, c16_n45_n27, c16_p07_n30, c16_p43_p39, c16_n23_p15, c16_n34_n45, c16_p36_p02, c16_p19_p44, O04A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n02_p39, c16_n36_n41, c16_p43_p07, c16_n11_p34, c16_n30_n44, c16_p45_p15, c16_n19_p27, c16_n23_n45, O05A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n15_p36, c16_n11_n45, c16_p34_p39, c16_n45_n19, c16_p41_n07, c16_n23_p30, c16_n02_n44, c16_p27_p43, O06A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n27_p34, c16_p19_n39, c16_n11_p43, c16_p02_n45, c16_p07_p45, c16_n15_n44, c16_p23_p41, c16_n30_n36, O07A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n36_p30, c16_p41_n23, c16_n44_p15, c16_p45_n07, c16_n45_n02, c16_p43_p11, c16_n39_n19, c16_p34_p27, O08A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n43_p27, c16_p44_n02, c16_n30_n23, c16_p07_p41, c16_p19_n45, c16_n39_p34, c16_p45_n11, c16_n36_n15, O09A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n45_p23, c16_p27_p19, c16_p15_n45, c16_n44_p30, c16_p34_p11, c16_p07_n43, c16_n41_p36, c16_p39_p02, O10A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n44_p19, c16_n02_p36, c16_p45_n34, c16_n15_n23, c16_n39_p43, c16_p30_p07, c16_p27_n45, c16_n41_p11, O11A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n39_p15, c16_n30_p45, c16_p27_p02, c16_p41_n44, c16_n11_n19, c16_n45_p36, c16_n07_p34, c16_p43_n23, O12A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n30_p11, c16_n45_p43, c16_n19_p36, c16_p23_n02, c16_p45_n39, c16_p27_n41, c16_n15_n07, c16_n44_p34, O13A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n19_p07, c16_n39_p30, c16_n45_p44, c16_n36_p43, c16_n15_p27, c16_p11_p02, c16_p34_n23, c16_p45_n41, O14A)
                COMPUTE_ROW(T_00_00A, T_00_01A, T_00_02A, T_00_03A, T_00_04A, T_00_05A, T_00_06A, T_00_07A, \
                    c16_n07_p02, c16_n15_p11, c16_n23_p19, c16_n30_p27, c16_n36_p34, c16_n41_p39, c16_n44_p43, c16_n45_p45, O15A)

                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_p45_p45, c16_p43_p44, c16_p39_p41, c16_p34_p36, c16_p27_p30, c16_p19_p23, c16_p11_p15, c16_p02_p07, O00B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_p41_p45, c16_p23_p34, c16_n02_p11, c16_n27_n15, c16_n43_n36, c16_n44_n45, c16_n30_n39, c16_n07_n19, O01B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_p34_p44, c16_n07_p15, c16_n41_n27, c16_n39_n45, c16_n02_n23, c16_p36_p19, c16_p43_p45, c16_p11_p30, O02B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_p23_p43, c16_n34_n07, c16_n36_n45, c16_p19_n11, c16_p44_p41, c16_n02_p27, c16_n45_n30, c16_n15_n39, O03B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_p11_p41, c16_n45_n27, c16_p07_n30, c16_p43_p39, c16_n23_p15, c16_n34_n45, c16_p36_p02, c16_p19_p44, O04B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n02_p39, c16_n36_n41, c16_p43_p07, c16_n11_p34, c16_n30_n44, c16_p45_p15, c16_n19_p27, c16_n23_n45, O05B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n15_p36, c16_n11_n45, c16_p34_p39, c16_n45_n19, c16_p41_n07, c16_n23_p30, c16_n02_n44, c16_p27_p43, O06B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n27_p34, c16_p19_n39, c16_n11_p43, c16_p02_n45, c16_p07_p45, c16_n15_n44, c16_p23_p41, c16_n30_n36, O07B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n36_p30, c16_p41_n23, c16_n44_p15, c16_p45_n07, c16_n45_n02, c16_p43_p11, c16_n39_n19, c16_p34_p27, O08B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n43_p27, c16_p44_n02, c16_n30_n23, c16_p07_p41, c16_p19_n45, c16_n39_p34, c16_p45_n11, c16_n36_n15, O09B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n45_p23, c16_p27_p19, c16_p15_n45, c16_n44_p30, c16_p34_p11, c16_p07_n43, c16_n41_p36, c16_p39_p02, O10B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n44_p19, c16_n02_p36, c16_p45_n34, c16_n15_n23, c16_n39_p43, c16_p30_p07, c16_p27_n45, c16_n41_p11, O11B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n39_p15, c16_n30_p45, c16_p27_p02, c16_p41_n44, c16_n11_n19, c16_n45_p36, c16_n07_p34, c16_p43_n23, O12B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n30_p11, c16_n45_p43, c16_n19_p36, c16_p23_n02, c16_p45_n39, c16_p27_n41, c16_n15_n07, c16_n44_p34, O13B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n19_p07, c16_n39_p30, c16_n45_p44, c16_n36_p43, c16_n15_p27, c16_p11_p02, c16_p34_n23, c16_p45_n41, O14B)
                COMPUTE_ROW(T_00_00B, T_00_01B, T_00_02B, T_00_03B, T_00_04B, T_00_05B, T_00_06B, T_00_07B, \
                    c16_n07_p02, c16_n15_p11, c16_n23_p19, c16_n30_p27, c16_n36_p34, c16_n41_p39, c16_n44_p43, c16_n45_p45, O15B)

#undef COMPUTE_ROW
            }


            {
                __m128i T00, T01;
#define COMPUTE_ROW(row0206, row1014, row1822, row2630, c0206, c1014, c1822, c2630, row) \
	T00 = _mm_add_epi32(_mm_madd_epi16(row0206, c0206), _mm_madd_epi16(row1014, c1014)); \
	T01 = _mm_add_epi32(_mm_madd_epi16(row1822, c1822), _mm_madd_epi16(row2630, c2630)); \
	row = _mm_add_epi32(T00, T01);

                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p43_p45, c16_p35_p40, c16_p21_p29, c16_p04_p13, EO0A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p29_p43, c16_n21_p04, c16_n45_n40, c16_n13_n35, EO1A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_p04_p40, c16_n43_n35, c16_p29_n13, c16_p21_p45, EO2A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n21_p35, c16_p04_n43, c16_p13_p45, c16_n29_n40, EO3A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n40_p29, c16_p45_n13, c16_n43_n04, c16_p35_p21, EO4A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n45_p21, c16_p13_p29, c16_p35_n43, c16_n40_p04, EO5A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n35_p13, c16_n40_p45, c16_p04_p21, c16_p43_n29, EO6A)
                COMPUTE_ROW(T_00_08A, T_00_09A, T_00_10A, T_00_11A, c16_n13_p04, c16_n29_p21, c16_n40_p35, c16_n45_p43, EO7A)

                COMPUTE_ROW(T_00_08B, T_00_09B, T_00_10B, T_00_11B, c16_p43_p45, c16_p35_p40, c16_p21_p29, c16_p04_p13, EO0B)
                COMPUTE_ROW(T_00_08B, T_00_09B, T_00_10B, T_00_11B, c16_p29_p43, c16_n21_p04, c16_n45_n40, c16_n13_n35, EO1B)
                COMPUTE_ROW(T_00_08B, T_00_09B, T_00_10B, T_00_11B, c16_p04_p40, c16_n43_n35, c16_p29_n13, c16_p21_p45, EO2B)
                COMPUTE_ROW(T_00_08B, T_00_09B, T_00_10B, T_00_11B, c16_n21_p35, c16_p04_n43, c16_p13_p45, c16_n29_n40, EO3B)
                COMPUTE_ROW(T_00_08B, T_00_09B, T_00_10B, T_00_11B, c16_n40_p29, c16_p45_n13, c16_n43_n04, c16_p35_p21, EO4B)
                COMPUTE_ROW(T_00_08B, T_00_09B, T_00_10B, T_00_11B, c16_n45_p21, c16_p13_p29, c16_p35_n43, c16_n40_p04, EO5B)
                COMPUTE_ROW(T_00_08B, T_00_09B, T_00_10B, T_00_11B, c16_n35_p13, c16_n40_p45, c16_p04_p21, c16_p43_n29, EO6B)
                COMPUTE_ROW(T_00_08B, T_00_09B, T_00_10B, T_00_11B, c16_n13_p04, c16_n29_p21, c16_n40_p35, c16_n45_p43, EO7B)
#undef COMPUTE_ROW
            }
            {
                const __m128i EEO0A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_p38_p44), _mm_madd_epi16(T_00_13A, c16_p09_p25));
                const __m128i EEO1A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n09_p38), _mm_madd_epi16(T_00_13A, c16_n25_n44));
                const __m128i EEO2A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n44_p25), _mm_madd_epi16(T_00_13A, c16_p38_p09));
                const __m128i EEO3A = _mm_add_epi32(_mm_madd_epi16(T_00_12A, c16_n25_p09), _mm_madd_epi16(T_00_13A, c16_n44_p38));
                const __m128i EEO0B = _mm_add_epi32(_mm_madd_epi16(T_00_12B, c16_p38_p44), _mm_madd_epi16(T_00_13B, c16_p09_p25));
                const __m128i EEO1B = _mm_add_epi32(_mm_madd_epi16(T_00_12B, c16_n09_p38), _mm_madd_epi16(T_00_13B, c16_n25_n44));
                const __m128i EEO2B = _mm_add_epi32(_mm_madd_epi16(T_00_12B, c16_n44_p25), _mm_madd_epi16(T_00_13B, c16_p38_p09));
                const __m128i EEO3B = _mm_add_epi32(_mm_madd_epi16(T_00_12B, c16_n25_p09), _mm_madd_epi16(T_00_13B, c16_n44_p38));

                const __m128i EEEO0A = _mm_madd_epi16(T_00_14A, c16_p17_p42);
                const __m128i EEEO0B = _mm_madd_epi16(T_00_14B, c16_p17_p42);
                const __m128i EEEO1A = _mm_madd_epi16(T_00_14A, c16_n42_p17);
                const __m128i EEEO1B = _mm_madd_epi16(T_00_14B, c16_n42_p17);

                const __m128i EEEE0A = _mm_madd_epi16(T_00_15A, c16_p32_p32);
                const __m128i EEEE0B = _mm_madd_epi16(T_00_15B, c16_p32_p32);
                const __m128i EEEE1A = _mm_madd_epi16(T_00_15A, c16_n32_p32);
                const __m128i EEEE1B = _mm_madd_epi16(T_00_15B, c16_n32_p32);

                const __m128i EEE0A = _mm_add_epi32(EEEE0A, EEEO0A);          // EEE0 = EEEE0 + EEEO0
                const __m128i EEE0B = _mm_add_epi32(EEEE0B, EEEO0B);
                const __m128i EEE1A = _mm_add_epi32(EEEE1A, EEEO1A);          // EEE1 = EEEE1 + EEEO1
                const __m128i EEE1B = _mm_add_epi32(EEEE1B, EEEO1B);
                const __m128i EEE3A = _mm_sub_epi32(EEEE0A, EEEO0A);          // EEE2 = EEEE0 - EEEO0
                const __m128i EEE3B = _mm_sub_epi32(EEEE0B, EEEO0B);
                const __m128i EEE2A = _mm_sub_epi32(EEEE1A, EEEO1A);          // EEE3 = EEEE1 - EEEO1
                const __m128i EEE2B = _mm_sub_epi32(EEEE1B, EEEO1B);

                const __m128i EE0A = _mm_add_epi32(EEE0A, EEO0A);          // EE0 = EEE0 + EEO0
                const __m128i EE0B = _mm_add_epi32(EEE0B, EEO0B);
                const __m128i EE1A = _mm_add_epi32(EEE1A, EEO1A);          // EE1 = EEE1 + EEO1
                const __m128i EE1B = _mm_add_epi32(EEE1B, EEO1B);
                const __m128i EE2A = _mm_add_epi32(EEE2A, EEO2A);          // EE2 = EEE0 + EEO0
                const __m128i EE2B = _mm_add_epi32(EEE2B, EEO2B);
                const __m128i EE3A = _mm_add_epi32(EEE3A, EEO3A);          // EE3 = EEE1 + EEO1
                const __m128i EE3B = _mm_add_epi32(EEE3B, EEO3B);
                const __m128i EE7A = _mm_sub_epi32(EEE0A, EEO0A);          // EE7 = EEE0 - EEO0
                const __m128i EE7B = _mm_sub_epi32(EEE0B, EEO0B);
                const __m128i EE6A = _mm_sub_epi32(EEE1A, EEO1A);          // EE6 = EEE1 - EEO1
                const __m128i EE6B = _mm_sub_epi32(EEE1B, EEO1B);
                const __m128i EE5A = _mm_sub_epi32(EEE2A, EEO2A);          // EE5 = EEE0 - EEO0
                const __m128i EE5B = _mm_sub_epi32(EEE2B, EEO2B);
                const __m128i EE4A = _mm_sub_epi32(EEE3A, EEO3A);          // EE4 = EEE1 - EEO1
                const __m128i EE4B = _mm_sub_epi32(EEE3B, EEO3B);

                const __m128i E0A = _mm_add_epi32(EE0A, EO0A);          // E0 = EE0 + EO0
                const __m128i E0B = _mm_add_epi32(EE0B, EO0B);
                const __m128i E1A = _mm_add_epi32(EE1A, EO1A);          // E1 = EE1 + EO1
                const __m128i E1B = _mm_add_epi32(EE1B, EO1B);
                const __m128i E2A = _mm_add_epi32(EE2A, EO2A);          // E2 = EE2 + EO2
                const __m128i E2B = _mm_add_epi32(EE2B, EO2B);
                const __m128i E3A = _mm_add_epi32(EE3A, EO3A);          // E3 = EE3 + EO3
                const __m128i E3B = _mm_add_epi32(EE3B, EO3B);
                const __m128i E4A = _mm_add_epi32(EE4A, EO4A);          // E4 =
                const __m128i E4B = _mm_add_epi32(EE4B, EO4B);
                const __m128i E5A = _mm_add_epi32(EE5A, EO5A);          // E5 =
                const __m128i E5B = _mm_add_epi32(EE5B, EO5B);
                const __m128i E6A = _mm_add_epi32(EE6A, EO6A);          // E6 =
                const __m128i E6B = _mm_add_epi32(EE6B, EO6B);
                const __m128i E7A = _mm_add_epi32(EE7A, EO7A);          // E7 =
                const __m128i E7B = _mm_add_epi32(EE7B, EO7B);
                const __m128i EFA = _mm_sub_epi32(EE0A, EO0A);          // EF = EE0 - EO0
                const __m128i EFB = _mm_sub_epi32(EE0B, EO0B);
                const __m128i EEA = _mm_sub_epi32(EE1A, EO1A);          // EE = EE1 - EO1
                const __m128i EEB = _mm_sub_epi32(EE1B, EO1B);
                const __m128i EDA = _mm_sub_epi32(EE2A, EO2A);          // ED = EE2 - EO2
                const __m128i EDB = _mm_sub_epi32(EE2B, EO2B);
                const __m128i ECA = _mm_sub_epi32(EE3A, EO3A);          // EC = EE3 - EO3
                const __m128i ECB = _mm_sub_epi32(EE3B, EO3B);
                const __m128i EBA = _mm_sub_epi32(EE4A, EO4A);          // EB =
                const __m128i EBB = _mm_sub_epi32(EE4B, EO4B);
                const __m128i EAA = _mm_sub_epi32(EE5A, EO5A);          // EA =
                const __m128i EAB = _mm_sub_epi32(EE5B, EO5B);
                const __m128i E9A = _mm_sub_epi32(EE6A, EO6A);          // E9 =
                const __m128i E9B = _mm_sub_epi32(EE6B, EO6B);
                const __m128i E8A = _mm_sub_epi32(EE7A, EO7A);          // E8 =
                const __m128i E8B = _mm_sub_epi32(EE7B, EO7B);

                const __m128i T10A = _mm_add_epi32(E0A, c32_off);         // E0 + off
                const __m128i T10B = _mm_add_epi32(E0B, c32_off);
                const __m128i T11A = _mm_add_epi32(E1A, c32_off);         // E1 + off
                const __m128i T11B = _mm_add_epi32(E1B, c32_off);
                const __m128i T12A = _mm_add_epi32(E2A, c32_off);         // E2 + off
                const __m128i T12B = _mm_add_epi32(E2B, c32_off);
                const __m128i T13A = _mm_add_epi32(E3A, c32_off);         // E3 + off
                const __m128i T13B = _mm_add_epi32(E3B, c32_off);
                const __m128i T14A = _mm_add_epi32(E4A, c32_off);         // E4 + off
                const __m128i T14B = _mm_add_epi32(E4B, c32_off);
                const __m128i T15A = _mm_add_epi32(E5A, c32_off);         // E5 + off
                const __m128i T15B = _mm_add_epi32(E5B, c32_off);
                const __m128i T16A = _mm_add_epi32(E6A, c32_off);         // E6 + off
                const __m128i T16B = _mm_add_epi32(E6B, c32_off);
                const __m128i T17A = _mm_add_epi32(E7A, c32_off);         // E7 + off
                const __m128i T17B = _mm_add_epi32(E7B, c32_off);
                const __m128i T18A = _mm_add_epi32(E8A, c32_off);         // E8 + off
                const __m128i T18B = _mm_add_epi32(E8B, c32_off);
                const __m128i T19A = _mm_add_epi32(E9A, c32_off);         // E9 + off
                const __m128i T19B = _mm_add_epi32(E9B, c32_off);
                const __m128i T1AA = _mm_add_epi32(EAA, c32_off);         // E10 + off
                const __m128i T1AB = _mm_add_epi32(EAB, c32_off);
                const __m128i T1BA = _mm_add_epi32(EBA, c32_off);         // E11 + off
                const __m128i T1BB = _mm_add_epi32(EBB, c32_off);
                const __m128i T1CA = _mm_add_epi32(ECA, c32_off);         // E12 + off
                const __m128i T1CB = _mm_add_epi32(ECB, c32_off);
                const __m128i T1DA = _mm_add_epi32(EDA, c32_off);         // E13 + off
                const __m128i T1DB = _mm_add_epi32(EDB, c32_off);
                const __m128i T1EA = _mm_add_epi32(EEA, c32_off);         // E14 + off
                const __m128i T1EB = _mm_add_epi32(EEB, c32_off);
                const __m128i T1FA = _mm_add_epi32(EFA, c32_off);         // E15 + off
                const __m128i T1FB = _mm_add_epi32(EFB, c32_off);

                const __m128i T2_00A = _mm_add_epi32(T10A, O00A);          // E0 + O0 + off
                const __m128i T2_00B = _mm_add_epi32(T10B, O00B);
                const __m128i T2_01A = _mm_add_epi32(T11A, O01A);          // E1 + O1 + off
                const __m128i T2_01B = _mm_add_epi32(T11B, O01B);
                const __m128i T2_02A = _mm_add_epi32(T12A, O02A);          // E2 + O2 + off
                const __m128i T2_02B = _mm_add_epi32(T12B, O02B);
                const __m128i T2_03A = _mm_add_epi32(T13A, O03A);          // E3 + O3 + off
                const __m128i T2_03B = _mm_add_epi32(T13B, O03B);
                const __m128i T2_04A = _mm_add_epi32(T14A, O04A);          // E4
                const __m128i T2_04B = _mm_add_epi32(T14B, O04B);
                const __m128i T2_05A = _mm_add_epi32(T15A, O05A);          // E5
                const __m128i T2_05B = _mm_add_epi32(T15B, O05B);
                const __m128i T2_06A = _mm_add_epi32(T16A, O06A);          // E6
                const __m128i T2_06B = _mm_add_epi32(T16B, O06B);
                const __m128i T2_07A = _mm_add_epi32(T17A, O07A);          // E7
                const __m128i T2_07B = _mm_add_epi32(T17B, O07B);
                const __m128i T2_08A = _mm_add_epi32(T18A, O08A);          // E8
                const __m128i T2_08B = _mm_add_epi32(T18B, O08B);
                const __m128i T2_09A = _mm_add_epi32(T19A, O09A);          // E9
                const __m128i T2_09B = _mm_add_epi32(T19B, O09B);
                const __m128i T2_10A = _mm_add_epi32(T1AA, O10A);          // E10
                const __m128i T2_10B = _mm_add_epi32(T1AB, O10B);
                const __m128i T2_11A = _mm_add_epi32(T1BA, O11A);          // E11
                const __m128i T2_11B = _mm_add_epi32(T1BB, O11B);
                const __m128i T2_12A = _mm_add_epi32(T1CA, O12A);          // E12
                const __m128i T2_12B = _mm_add_epi32(T1CB, O12B);
                const __m128i T2_13A = _mm_add_epi32(T1DA, O13A);          // E13
                const __m128i T2_13B = _mm_add_epi32(T1DB, O13B);
                const __m128i T2_14A = _mm_add_epi32(T1EA, O14A);          // E14
                const __m128i T2_14B = _mm_add_epi32(T1EB, O14B);
                const __m128i T2_15A = _mm_add_epi32(T1FA, O15A);          // E15
                const __m128i T2_15B = _mm_add_epi32(T1FB, O15B);
                const __m128i T2_31A = _mm_sub_epi32(T10A, O00A);          // E0 - O0 + off
                const __m128i T2_31B = _mm_sub_epi32(T10B, O00B);
                const __m128i T2_30A = _mm_sub_epi32(T11A, O01A);          // E1 - O1 + off
                const __m128i T2_30B = _mm_sub_epi32(T11B, O01B);
                const __m128i T2_29A = _mm_sub_epi32(T12A, O02A);          // E2 - O2 + off
                const __m128i T2_29B = _mm_sub_epi32(T12B, O02B);
                const __m128i T2_28A = _mm_sub_epi32(T13A, O03A);          // E3 - O3 + off
                const __m128i T2_28B = _mm_sub_epi32(T13B, O03B);
                const __m128i T2_27A = _mm_sub_epi32(T14A, O04A);          // E4
                const __m128i T2_27B = _mm_sub_epi32(T14B, O04B);
                const __m128i T2_26A = _mm_sub_epi32(T15A, O05A);          // E5
                const __m128i T2_26B = _mm_sub_epi32(T15B, O05B);
                const __m128i T2_25A = _mm_sub_epi32(T16A, O06A);          // E6
                const __m128i T2_25B = _mm_sub_epi32(T16B, O06B);
                const __m128i T2_24A = _mm_sub_epi32(T17A, O07A);          // E7
                const __m128i T2_24B = _mm_sub_epi32(T17B, O07B);
                const __m128i T2_23A = _mm_sub_epi32(T18A, O08A);          //
                const __m128i T2_23B = _mm_sub_epi32(T18B, O08B);
                const __m128i T2_22A = _mm_sub_epi32(T19A, O09A);          //
                const __m128i T2_22B = _mm_sub_epi32(T19B, O09B);
                const __m128i T2_21A = _mm_sub_epi32(T1AA, O10A);          //
                const __m128i T2_21B = _mm_sub_epi32(T1AB, O10B);
                const __m128i T2_20A = _mm_sub_epi32(T1BA, O11A);          //
                const __m128i T2_20B = _mm_sub_epi32(T1BB, O11B);
                const __m128i T2_19A = _mm_sub_epi32(T1CA, O12A);          //
                const __m128i T2_19B = _mm_sub_epi32(T1CB, O12B);
                const __m128i T2_18A = _mm_sub_epi32(T1DA, O13A);          //
                const __m128i T2_18B = _mm_sub_epi32(T1DB, O13B);
                const __m128i T2_17A = _mm_sub_epi32(T1EA, O14A);          //
                const __m128i T2_17B = _mm_sub_epi32(T1EB, O14B);
                const __m128i T2_16A = _mm_sub_epi32(T1FA, O15A);          //
                const __m128i T2_16B = _mm_sub_epi32(T1FB, O15B);

                const __m128i T3_00A = _mm_srai_epi32(T2_00A, shift);             // [30 20 10 00]
                const __m128i T3_00B = _mm_srai_epi32(T2_00B, shift);             // [70 60 50 40]
                const __m128i T3_01A = _mm_srai_epi32(T2_01A, shift);             // [31 21 11 01]
                const __m128i T3_01B = _mm_srai_epi32(T2_01B, shift);             // [71 61 51 41]
                const __m128i T3_02A = _mm_srai_epi32(T2_02A, shift);             // [32 22 12 02]
                const __m128i T3_02B = _mm_srai_epi32(T2_02B, shift);             // [72 62 52 42]
                const __m128i T3_03A = _mm_srai_epi32(T2_03A, shift);             // [33 23 13 03]
                const __m128i T3_03B = _mm_srai_epi32(T2_03B, shift);             // [73 63 53 43]
                const __m128i T3_04A = _mm_srai_epi32(T2_04A, shift);             // [33 24 14 04]
                const __m128i T3_04B = _mm_srai_epi32(T2_04B, shift);             // [74 64 54 44]
                const __m128i T3_05A = _mm_srai_epi32(T2_05A, shift);             // [35 25 15 05]
                const __m128i T3_05B = _mm_srai_epi32(T2_05B, shift);             // [75 65 55 45]
                const __m128i T3_06A = _mm_srai_epi32(T2_06A, shift);             // [36 26 16 06]
                const __m128i T3_06B = _mm_srai_epi32(T2_06B, shift);             // [76 66 56 46]
                const __m128i T3_07A = _mm_srai_epi32(T2_07A, shift);             // [37 27 17 07]
                const __m128i T3_07B = _mm_srai_epi32(T2_07B, shift);             // [77 67 57 47]
                const __m128i T3_08A = _mm_srai_epi32(T2_08A, shift);             // [30 20 10 00] x8
                const __m128i T3_08B = _mm_srai_epi32(T2_08B, shift);             // [70 60 50 40]
                const __m128i T3_09A = _mm_srai_epi32(T2_09A, shift);             // [31 21 11 01] x9
                const __m128i T3_09B = _mm_srai_epi32(T2_09B, shift);             // [71 61 51 41]
                const __m128i T3_10A = _mm_srai_epi32(T2_10A, shift);             // [32 22 12 02] xA
                const __m128i T3_10B = _mm_srai_epi32(T2_10B, shift);             // [72 62 52 42]
                const __m128i T3_11A = _mm_srai_epi32(T2_11A, shift);             // [33 23 13 03] xB
                const __m128i T3_11B = _mm_srai_epi32(T2_11B, shift);             // [73 63 53 43]
                const __m128i T3_12A = _mm_srai_epi32(T2_12A, shift);             // [33 24 14 04] xC
                const __m128i T3_12B = _mm_srai_epi32(T2_12B, shift);             // [74 64 54 44]
                const __m128i T3_13A = _mm_srai_epi32(T2_13A, shift);             // [35 25 15 05] xD
                const __m128i T3_13B = _mm_srai_epi32(T2_13B, shift);             // [75 65 55 45]
                const __m128i T3_14A = _mm_srai_epi32(T2_14A, shift);             // [36 26 16 06] xE
                const __m128i T3_14B = _mm_srai_epi32(T2_14B, shift);             // [76 66 56 46]
                const __m128i T3_15A = _mm_srai_epi32(T2_15A, shift);             // [37 27 17 07] xF
                const __m128i T3_15B = _mm_srai_epi32(T2_15B, shift);             // [77 67 57 47]

                const __m128i T3_16A = _mm_srai_epi32(T2_16A, shift);             // [30 20 10 00]
                const __m128i T3_16B = _mm_srai_epi32(T2_16B, shift);             // [70 60 50 40]
                const __m128i T3_17A = _mm_srai_epi32(T2_17A, shift);             // [31 21 11 01]
                const __m128i T3_17B = _mm_srai_epi32(T2_17B, shift);             // [71 61 51 41]
                const __m128i T3_18A = _mm_srai_epi32(T2_18A, shift);             // [32 22 12 02]
                const __m128i T3_18B = _mm_srai_epi32(T2_18B, shift);             // [72 62 52 42]
                const __m128i T3_19A = _mm_srai_epi32(T2_19A, shift);             // [33 23 13 03]
                const __m128i T3_19B = _mm_srai_epi32(T2_19B, shift);             // [73 63 53 43]
                const __m128i T3_20A = _mm_srai_epi32(T2_20A, shift);             // [33 24 14 04]
                const __m128i T3_20B = _mm_srai_epi32(T2_20B, shift);             // [74 64 54 44]
                const __m128i T3_21A = _mm_srai_epi32(T2_21A, shift);             // [35 25 15 05]
                const __m128i T3_21B = _mm_srai_epi32(T2_21B, shift);             // [75 65 55 45]
                const __m128i T3_22A = _mm_srai_epi32(T2_22A, shift);             // [36 26 16 06]
                const __m128i T3_22B = _mm_srai_epi32(T2_22B, shift);             // [76 66 56 46]
                const __m128i T3_23A = _mm_srai_epi32(T2_23A, shift);             // [37 27 17 07]
                const __m128i T3_23B = _mm_srai_epi32(T2_23B, shift);             // [77 67 57 47]
                const __m128i T3_24A = _mm_srai_epi32(T2_24A, shift);             // [30 20 10 00] x8
                const __m128i T3_24B = _mm_srai_epi32(T2_24B, shift);             // [70 60 50 40]
                const __m128i T3_25A = _mm_srai_epi32(T2_25A, shift);             // [31 21 11 01] x9
                const __m128i T3_25B = _mm_srai_epi32(T2_25B, shift);             // [71 61 51 41]
                const __m128i T3_26A = _mm_srai_epi32(T2_26A, shift);             // [32 22 12 02] xA
                const __m128i T3_26B = _mm_srai_epi32(T2_26B, shift);             // [72 62 52 42]
                const __m128i T3_27A = _mm_srai_epi32(T2_27A, shift);             // [33 23 13 03] xB
                const __m128i T3_27B = _mm_srai_epi32(T2_27B, shift);             // [73 63 53 43]
                const __m128i T3_28A = _mm_srai_epi32(T2_28A, shift);             // [33 24 14 04] xC
                const __m128i T3_28B = _mm_srai_epi32(T2_28B, shift);             // [74 64 54 44]
                const __m128i T3_29A = _mm_srai_epi32(T2_29A, shift);             // [35 25 15 05] xD
                const __m128i T3_29B = _mm_srai_epi32(T2_29B, shift);             // [75 65 55 45]
                const __m128i T3_30A = _mm_srai_epi32(T2_30A, shift);             // [36 26 16 06] xE
                const __m128i T3_30B = _mm_srai_epi32(T2_30B, shift);             // [76 66 56 46]
                const __m128i T3_31A = _mm_srai_epi32(T2_31A, shift);             // [37 27 17 07] xF
                const __m128i T3_31B = _mm_srai_epi32(T2_31B, shift);             // [77 67 57 47]

                res00[part] = _mm_packs_epi32(T3_00A, T3_00B);        // [70 60 50 40 30 20 10 00]
                res01[part] = _mm_packs_epi32(T3_01A, T3_01B);        // [71 61 51 41 31 21 11 01]
                res02[part] = _mm_packs_epi32(T3_02A, T3_02B);        // [72 62 52 42 32 22 12 02]
                res03[part] = _mm_packs_epi32(T3_03A, T3_03B);        // [73 63 53 43 33 23 13 03]
                res04[part] = _mm_packs_epi32(T3_04A, T3_04B);        // [74 64 54 44 34 24 14 04]
                res05[part] = _mm_packs_epi32(T3_05A, T3_05B);        // [75 65 55 45 35 25 15 05]
                res06[part] = _mm_packs_epi32(T3_06A, T3_06B);        // [76 66 56 46 36 26 16 06]
                res07[part] = _mm_packs_epi32(T3_07A, T3_07B);        // [77 67 57 47 37 27 17 07]
                res08[part] = _mm_packs_epi32(T3_08A, T3_08B);        // [A0 ... 80]
                res09[part] = _mm_packs_epi32(T3_09A, T3_09B);        // [A1 ... 81]
                res10[part] = _mm_packs_epi32(T3_10A, T3_10B);        // [A2 ... 82]
                res11[part] = _mm_packs_epi32(T3_11A, T3_11B);        // [A3 ... 83]
                res12[part] = _mm_packs_epi32(T3_12A, T3_12B);        // [A4 ... 84]
                res13[part] = _mm_packs_epi32(T3_13A, T3_13B);        // [A5 ... 85]
                res14[part] = _mm_packs_epi32(T3_14A, T3_14B);        // [A6 ... 86]
                res15[part] = _mm_packs_epi32(T3_15A, T3_15B);        // [A7 ... 87]
                res16[part] = _mm_packs_epi32(T3_16A, T3_16B);
                res17[part] = _mm_packs_epi32(T3_17A, T3_17B);
                res18[part] = _mm_packs_epi32(T3_18A, T3_18B);
                res19[part] = _mm_packs_epi32(T3_19A, T3_19B);
                res20[part] = _mm_packs_epi32(T3_20A, T3_20B);
                res21[part] = _mm_packs_epi32(T3_21A, T3_21B);
                res22[part] = _mm_packs_epi32(T3_22A, T3_22B);
                res23[part] = _mm_packs_epi32(T3_23A, T3_23B);
                res24[part] = _mm_packs_epi32(T3_24A, T3_24B);
                res25[part] = _mm_packs_epi32(T3_25A, T3_25B);
                res26[part] = _mm_packs_epi32(T3_26A, T3_26B);
                res27[part] = _mm_packs_epi32(T3_27A, T3_27B);
                res28[part] = _mm_packs_epi32(T3_28A, T3_28B);
                res29[part] = _mm_packs_epi32(T3_29A, T3_29B);
                res30[part] = _mm_packs_epi32(T3_30A, T3_30B);
                res31[part] = _mm_packs_epi32(T3_31A, T3_31B);
            }
        }
        //transpose matrix 32x32 16bit.
        {
            __m128i tr0_0, tr0_1, tr0_2, tr0_3, tr0_4, tr0_5, tr0_6, tr0_7;
            __m128i tr1_0, tr1_1, tr1_2, tr1_3, tr1_4, tr1_5, tr1_6, tr1_7;

            TRANSPOSE_8x8_16BIT(res00[0], res01[0], res02[0], res03[0], res04[0], res05[0], res06[0], res07[0], in00[0], in01[0], in02[0], in03[0], in04[0], in05[0], in06[0], in07[0])
            TRANSPOSE_8x8_16BIT(res00[1], res01[1], res02[1], res03[1], res04[1], res05[1], res06[1], res07[1], in08[0], in09[0], in10[0], in11[0], in12[0], in13[0], in14[0], in15[0])
            TRANSPOSE_8x8_16BIT(res00[2], res01[2], res02[2], res03[2], res04[2], res05[2], res06[2], res07[2], in16[0], in17[0], in18[0], in19[0], in20[0], in21[0], in22[0], in23[0])
            TRANSPOSE_8x8_16BIT(res00[3], res01[3], res02[3], res03[3], res04[3], res05[3], res06[3], res07[3], in24[0], in25[0], in26[0], in27[0], in28[0], in29[0], in30[0], in31[0])

            TRANSPOSE_8x8_16BIT(res08[0], res09[0], res10[0], res11[0], res12[0], res13[0], res14[0], res15[0], in00[1], in01[1], in02[1], in03[1], in04[1], in05[1], in06[1], in07[1])
            TRANSPOSE_8x8_16BIT(res08[1], res09[1], res10[1], res11[1], res12[1], res13[1], res14[1], res15[1], in08[1], in09[1], in10[1], in11[1], in12[1], in13[1], in14[1], in15[1])
            TRANSPOSE_8x8_16BIT(res08[2], res09[2], res10[2], res11[2], res12[2], res13[2], res14[2], res15[2], in16[1], in17[1], in18[1], in19[1], in20[1], in21[1], in22[1], in23[1])
            TRANSPOSE_8x8_16BIT(res08[3], res09[3], res10[3], res11[3], res12[3], res13[3], res14[3], res15[3], in24[1], in25[1], in26[1], in27[1], in28[1], in29[1], in30[1], in31[1])

            TRANSPOSE_8x8_16BIT(res16[0], res17[0], res18[0], res19[0], res20[0], res21[0], res22[0], res23[0], in00[2], in01[2], in02[2], in03[2], in04[2], in05[2], in06[2], in07[2])
            TRANSPOSE_8x8_16BIT(res16[1], res17[1], res18[1], res19[1], res20[1], res21[1], res22[1], res23[1], in08[2], in09[2], in10[2], in11[2], in12[2], in13[2], in14[2], in15[2])
            TRANSPOSE_8x8_16BIT(res16[2], res17[2], res18[2], res19[2], res20[2], res21[2], res22[2], res23[2], in16[2], in17[2], in18[2], in19[2], in20[2], in21[2], in22[2], in23[2])
            TRANSPOSE_8x8_16BIT(res16[3], res17[3], res18[3], res19[3], res20[3], res21[3], res22[3], res23[3], in24[2], in25[2], in26[2], in27[2], in28[2], in29[2], in30[2], in31[2])

            TRANSPOSE_8x8_16BIT(res24[0], res25[0], res26[0], res27[0], res28[0], res29[0], res30[0], res31[0], in00[3], in01[3], in02[3], in03[3], in04[3], in05[3], in06[3], in07[3])
            TRANSPOSE_8x8_16BIT(res24[1], res25[1], res26[1], res27[1], res28[1], res29[1], res30[1], res31[1], in08[3], in09[3], in10[3], in11[3], in12[3], in13[3], in14[3], in15[3])
            TRANSPOSE_8x8_16BIT(res24[2], res25[2], res26[2], res27[2], res28[2], res29[2], res30[2], res31[2], in16[3], in17[3], in18[3], in19[3], in20[3], in21[3], in22[3], in23[3])
            TRANSPOSE_8x8_16BIT(res24[3], res25[3], res26[3], res27[3], res28[3], res29[3], res30[3], res31[3], in24[3], in25[3], in26[3], in27[3], in28[3], in29[3], in30[3], in31[3])

        }
    }


    //clip
    {
        __m128i max_val = _mm_set1_epi16((1 << bit_depth) - 1);
        __m128i min_val = _mm_set1_epi16(-(1 << bit_depth));

        for (k = 0; k < 4; k++)
        {
            int offset = k << 3;
            in00[k] = _mm_min_epi16(in00[k], max_val);
            in01[k] = _mm_min_epi16(in01[k], max_val);
            in02[k] = _mm_min_epi16(in02[k], max_val);
            in03[k] = _mm_min_epi16(in03[k], max_val);
            in00[k] = _mm_max_epi16(in00[k], min_val);
            in01[k] = _mm_max_epi16(in01[k], min_val);
            in02[k] = _mm_max_epi16(in02[k], min_val);
            in03[k] = _mm_max_epi16(in03[k], min_val);
            _mm_store_si128((__m128i*)&dst[0 + offset], in00[k]);
            _mm_store_si128((__m128i*)&dst[1 * 32 + offset], in01[k]);
            _mm_store_si128((__m128i*)&dst[2 * 32 + offset], in02[k]);
            _mm_store_si128((__m128i*)&dst[3 * 32 + offset], in03[k]);

            in04[k] = _mm_min_epi16(in04[k], max_val);
            in05[k] = _mm_min_epi16(in05[k], max_val);
            in06[k] = _mm_min_epi16(in06[k], max_val);
            in07[k] = _mm_min_epi16(in07[k], max_val);
            in04[k] = _mm_max_epi16(in04[k], min_val);
            in05[k] = _mm_max_epi16(in05[k], min_val);
            in06[k] = _mm_max_epi16(in06[k], min_val);
            in07[k] = _mm_max_epi16(in07[k], min_val);
            _mm_store_si128((__m128i*)&dst[4 * 32 + offset], in04[k]);
            _mm_store_si128((__m128i*)&dst[5 * 32 + offset], in05[k]);
            _mm_store_si128((__m128i*)&dst[6 * 32 + offset], in06[k]);
            _mm_store_si128((__m128i*)&dst[7 * 32 + offset], in07[k]);

            in08[k] = _mm_min_epi16(in08[k], max_val);
            in09[k] = _mm_min_epi16(in09[k], max_val);
            in10[k] = _mm_min_epi16(in10[k], max_val);
            in11[k] = _mm_min_epi16(in11[k], max_val);
            in08[k] = _mm_max_epi16(in08[k], min_val);
            in09[k] = _mm_max_epi16(in09[k], min_val);
            in10[k] = _mm_max_epi16(in10[k], min_val);
            in11[k] = _mm_max_epi16(in11[k], min_val);
            _mm_store_si128((__m128i*)&dst[8  * 32 + offset], in08[k]);
            _mm_store_si128((__m128i*)&dst[9  * 32 + offset], in09[k]);
            _mm_store_si128((__m128i*)&dst[10 * 32 + offset], in10[k]);
            _mm_store_si128((__m128i*)&dst[11 * 32 + offset], in11[k]);

            in12[k] = _mm_min_epi16(in12[k], max_val);
            in13[k] = _mm_min_epi16(in13[k], max_val);
            in14[k] = _mm_min_epi16(in14[k], max_val);
            in15[k] = _mm_min_epi16(in15[k], max_val);
            in12[k] = _mm_max_epi16(in12[k], min_val);
            in13[k] = _mm_max_epi16(in13[k], min_val);
            in14[k] = _mm_max_epi16(in14[k], min_val);
            in15[k] = _mm_max_epi16(in15[k], min_val);
            _mm_store_si128((__m128i*)&dst[12 * 32 + offset], in12[k]);
            _mm_store_si128((__m128i*)&dst[13 * 32 + offset], in13[k]);
            _mm_store_si128((__m128i*)&dst[14 * 32 + offset], in14[k]);
            _mm_store_si128((__m128i*)&dst[15 * 32 + offset], in15[k]);

            in16[k] = _mm_min_epi16(in16[k], max_val);
            in17[k] = _mm_min_epi16(in17[k], max_val);
            in18[k] = _mm_min_epi16(in18[k], max_val);
            in19[k] = _mm_min_epi16(in19[k], max_val);
            in16[k] = _mm_max_epi16(in16[k], min_val);
            in17[k] = _mm_max_epi16(in17[k], min_val);
            in18[k] = _mm_max_epi16(in18[k], min_val);
            in19[k] = _mm_max_epi16(in19[k], min_val);
            _mm_store_si128((__m128i*)&dst[16 * 32 + offset], in16[k]);
            _mm_store_si128((__m128i*)&dst[17 * 32 + offset], in17[k]);
            _mm_store_si128((__m128i*)&dst[18 * 32 + offset], in18[k]);
            _mm_store_si128((__m128i*)&dst[19 * 32 + offset], in19[k]);

            in20[k] = _mm_min_epi16(in20[k], max_val);
            in21[k] = _mm_min_epi16(in21[k], max_val);
            in22[k] = _mm_min_epi16(in22[k], max_val);
            in23[k] = _mm_min_epi16(in23[k], max_val);
            in20[k] = _mm_max_epi16(in20[k], min_val);
            in21[k] = _mm_max_epi16(in21[k], min_val);
            in22[k] = _mm_max_epi16(in22[k], min_val);
            in23[k] = _mm_max_epi16(in23[k], min_val);
            _mm_store_si128((__m128i*)&dst[20 * 32 + offset], in20[k]);
            _mm_store_si128((__m128i*)&dst[21 * 32 + offset], in21[k]);
            _mm_store_si128((__m128i*)&dst[22 * 32 + offset], in22[k]);
            _mm_store_si128((__m128i*)&dst[23 * 32 + offset], in23[k]);

            in24[k] = _mm_min_epi16(in24[k], max_val);
            in25[k] = _mm_min_epi16(in25[k], max_val);
            in26[k] = _mm_min_epi16(in26[k], max_val);
            in27[k] = _mm_min_epi16(in27[k], max_val);
            in24[k] = _mm_max_epi16(in24[k], min_val);
            in25[k] = _mm_max_epi16(in25[k], min_val);
            in26[k] = _mm_max_epi16(in26[k], min_val);
            in27[k] = _mm_max_epi16(in27[k], min_val);
            _mm_store_si128((__m128i*)&dst[24 * 32 + offset], in24[k]);
            _mm_store_si128((__m128i*)&dst[25 * 32 + offset], in25[k]);
            _mm_store_si128((__m128i*)&dst[26 * 32 + offset], in26[k]);
            _mm_store_si128((__m128i*)&dst[27 * 32 + offset], in27[k]);

            in28[k] = _mm_min_epi16(in28[k], max_val);
            in29[k] = _mm_min_epi16(in29[k], max_val);
            in30[k] = _mm_min_epi16(in30[k], max_val);
            in31[k] = _mm_min_epi16(in31[k], max_val);
            in28[k] = _mm_max_epi16(in28[k], min_val);
            in29[k] = _mm_max_epi16(in29[k], min_val);
            in30[k] = _mm_max_epi16(in30[k], min_val);
            in31[k] = _mm_max_epi16(in31[k], min_val);
            _mm_store_si128((__m128i*)&dst[28 * 32 + offset], in28[k]);
            _mm_store_si128((__m128i*)&dst[29 * 32 + offset], in29[k]);
            _mm_store_si128((__m128i*)&dst[30 * 32 + offset], in30[k]);
            _mm_store_si128((__m128i*)&dst[31 * 32 + offset], in31[k]);
        }
    }

}

void itrans_dct8_pb4_sse(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i = 0;
    __m128i factor = _mm_set1_epi32(1 << (shift - 1));
    __m128i s0, s1, s2, s3;	//源数据
    __m128i c0, c1, c2, c3;	//系数矩阵
    __m128i zero = _mm_setzero_si128();
    __m128i e0, e1, e2, e3;
    __m128i v0, v1, v2, v3;
    __m128i r0;

    for (i; i < line; i++)
    {
        //load coef data
        //s8->s16
        c0 = _mm_loadl_epi64((__m128i*)iT);
        c0 = _mm_cvtepi8_epi16(c0);
        c1 = _mm_loadl_epi64((__m128i*)(iT + 4));
        c1 = _mm_cvtepi8_epi16(c1);
        c2 = _mm_loadl_epi64((__m128i*)(iT + 8));
        c2 = _mm_cvtepi8_epi16(c2);
        c3 = _mm_loadl_epi64((__m128i*)(iT + 12));
        c3 = _mm_cvtepi8_epi16(c3);
        c0 = _mm_unpacklo_epi16(c0, zero);
        c1 = _mm_unpacklo_epi16(c1, zero);
        c2 = _mm_unpacklo_epi16(c2, zero);
        c3 = _mm_unpacklo_epi16(c3, zero);

        //load src
        s0 = _mm_set1_epi16(coeff[0]);
        s1 = _mm_set1_epi16(coeff[line]);
        s2 = _mm_set1_epi16(coeff[2 * line]);
        s3 = _mm_set1_epi16(coeff[3 * line]);

        v0 = _mm_madd_epi16(s0, c0);
        v1 = _mm_madd_epi16(s1, c1);
        v2 = _mm_madd_epi16(s2, c2);
        v3 = _mm_madd_epi16(s3, c3);
        e0 = _mm_add_epi32(v0, v1);
        e1 = _mm_add_epi32(v2, v3);
        e2 = _mm_add_epi32(e0, e1);
        e3 = _mm_add_epi32(e2, factor);

        r0 = _mm_packs_epi32(_mm_srai_epi32(e3, shift), zero);
        _mm_storel_epi64((__m128i*)block, r0);

        coeff++;
        block += 4;
    }
}

void itrans_dct8_pb8_sse(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m128i s0, s1, s2, s3;
    __m128i c0, c1, c2, c3;
    __m128i e0, e1, e2;
    __m128i r0, r1;
    __m128i zero = _mm_setzero_si128();
    __m128i rnd_factor = _mm_set1_epi32(1 << (shift - 1));

    for (i = 0; i < line; i++)
    {
        s8 *pb8 = iT;
        for (j = 0; j < 2; j++)
        {
            __m128i v0 = zero;
            for (k = 0; k < 2; k++)
            {
                //load src
                s0 = _mm_set1_epi16(coeff[0]);
                s1 = _mm_set1_epi16(coeff[line]);
                s2 = _mm_set1_epi16(coeff[2 * line]);
                s3 = _mm_set1_epi16(coeff[3 * line]);

                c0 = _mm_loadl_epi64((__m128i*)pb8);
                c0 = _mm_cvtepi8_epi16(c0);
                c1 = _mm_loadl_epi64((__m128i*)(pb8 + 8));
                c1 = _mm_cvtepi8_epi16(c1);
                c2 = _mm_loadl_epi64((__m128i*)(pb8 + 16));
                c2 = _mm_cvtepi8_epi16(c2);
                c3 = _mm_loadl_epi64((__m128i*)(pb8 + 24));
                c3 = _mm_cvtepi8_epi16(c3);
                c0 = _mm_unpacklo_epi16(c0, zero);
                c1 = _mm_unpacklo_epi16(c1, zero);
                c2 = _mm_unpacklo_epi16(c2, zero);
                c3 = _mm_unpacklo_epi16(c3, zero);

                e0 = _mm_add_epi32(_mm_madd_epi16(s0, c0), _mm_madd_epi16(s1, c1));
                e1 = _mm_add_epi32(_mm_madd_epi16(s2, c2), _mm_madd_epi16(s3, c3));
                e2 = _mm_add_epi32(e0, e1);

                v0 = _mm_add_epi32(v0, e2);

                pb8 += 32;
                coeff += (4 * line);
            }
            r0 = _mm_add_epi32(v0, rnd_factor);
            r1 = _mm_packs_epi32(_mm_srai_epi32(r0, shift), zero);
            _mm_storel_epi64((__m128i*)block, r1);
            coeff -= (8 * line);
            pb8 -= 60;
            block += 4;
        }
        coeff++;
    }
}

void itrans_dct8_pb16_sse(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m128i s0, s1, s2, s3;
    __m128i c0, c1, c2, c3;
    __m128i e0, e1, e2;
    __m128i r0, r1;
    __m128i zero = _mm_setzero_si128();
    __m128i rnd_factor = _mm_set1_epi32(1 << (shift - 1));

    for (i = 0; i < line; i++)
    {
        s8 *pb16 = iT;
        for (j = 0; j < 4; j++)
        {
            __m128i v0 = zero;
            for (k = 0; k < 4; k++)
            {
                //load src
                s0 = _mm_set1_epi16(coeff[0]);
                s1 = _mm_set1_epi16(coeff[line]);
                s2 = _mm_set1_epi16(coeff[2 * line]);
                s3 = _mm_set1_epi16(coeff[3 * line]);

                c0 = _mm_loadl_epi64((__m128i*)pb16);
                c0 = _mm_cvtepi8_epi16(c0);
                c1 = _mm_loadl_epi64((__m128i*)(pb16 + 16));
                c1 = _mm_cvtepi8_epi16(c1);
                c2 = _mm_loadl_epi64((__m128i*)(pb16 + 32));
                c2 = _mm_cvtepi8_epi16(c2);
                c3 = _mm_loadl_epi64((__m128i*)(pb16 + 48));
                c3 = _mm_cvtepi8_epi16(c3);
                c0 = _mm_unpacklo_epi16(c0, zero);
                c1 = _mm_unpacklo_epi16(c1, zero);
                c2 = _mm_unpacklo_epi16(c2, zero);
                c3 = _mm_unpacklo_epi16(c3, zero);


                e0 = _mm_add_epi32(_mm_madd_epi16(s0, c0), _mm_madd_epi16(s1, c1));
                e1 = _mm_add_epi32(_mm_madd_epi16(s2, c2), _mm_madd_epi16(s3, c3));
                e2 = _mm_add_epi32(e0, e1);

                v0 = _mm_add_epi32(v0, e2);

                pb16 += 64;
                coeff += (4 * line);
            }
            r0 = _mm_add_epi32(v0, rnd_factor);
            r1 = _mm_packs_epi32(_mm_srai_epi32(r0, shift), zero);
            _mm_storel_epi64((__m128i*)block, r1);
            coeff -= (16 * line);
            pb16 -= 252;
            block += 4;
        }
        coeff++;
    }
}

void itrans_dct8_pb32_sse(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m128i s0, s1, s2, s3;
    __m128i c0, c1, c2, c3;
    __m128i e0, e1, e2;
    __m128i r0, r1;
    __m128i zero = _mm_setzero_si128();
    __m128i rnd_factor = _mm_set1_epi32(1 << (shift - 1));

    for (i = 0; i < line; i++)
    {
        s8 *pb32 = iT;
        for (j = 0; j < 8; j++)
        {
            __m128i v0 = zero;
            for (k = 0; k < 8; k++)
            {
                //load src
                s0 = _mm_set1_epi16(coeff[0]);
                s1 = _mm_set1_epi16(coeff[line]);
                s2 = _mm_set1_epi16(coeff[2 * line]);
                s3 = _mm_set1_epi16(coeff[3 * line]);

                c0 = _mm_loadl_epi64((__m128i*)pb32);
                c0 = _mm_cvtepi8_epi16(c0);
                c1 = _mm_loadl_epi64((__m128i*)(pb32 + 32));
                c1 = _mm_cvtepi8_epi16(c1);
                c2 = _mm_loadl_epi64((__m128i*)(pb32 + 64));
                c2 = _mm_cvtepi8_epi16(c2);
                c3 = _mm_loadl_epi64((__m128i*)(pb32 + 96));
                c3 = _mm_cvtepi8_epi16(c3);
                c0 = _mm_unpacklo_epi16(c0, zero);
                c1 = _mm_unpacklo_epi16(c1, zero);
                c2 = _mm_unpacklo_epi16(c2, zero);
                c3 = _mm_unpacklo_epi16(c3, zero);


                e0 = _mm_add_epi32(_mm_madd_epi16(s0, c0), _mm_madd_epi16(s1, c1));
                e1 = _mm_add_epi32(_mm_madd_epi16(s2, c2), _mm_madd_epi16(s3, c3));
                e2 = _mm_add_epi32(e0, e1);

                v0 = _mm_add_epi32(v0, e2);

                pb32 += 128;
                coeff += (4 * line);
            }
            r0 = _mm_add_epi32(v0, rnd_factor);
            r1 = _mm_packs_epi32(_mm_srai_epi32(r0, shift), zero);
            _mm_storel_epi64((__m128i*)block, r1);
            coeff -= (32 * line);
            pb32 -= 1020;
            block += 4;
        }
        coeff++;
    }
}

void itrans_dst7_pb4_sse(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i = 0;
    __m128i factor = _mm_set1_epi32(1 << (shift - 1));
    __m128i s0, s1, s2, s3;	//源数据
    __m128i c0, c1, c2, c3;	//系数矩阵
    __m128i zero = _mm_setzero_si128();
    __m128i e0, e1, e2, e3;
    __m128i v0, v1, v2, v3;
    __m128i r0;
    for (i = 0; i < line; i++)
    {
        //load coef data
        //s8->s16
        c0 = _mm_loadl_epi64((__m128i*)iT);
        c0 = _mm_cvtepi8_epi16(c0);
        c1 = _mm_loadl_epi64((__m128i*)(iT + 4));
        c1 = _mm_cvtepi8_epi16(c1);
        c2 = _mm_loadl_epi64((__m128i*)(iT + 8));
        c2 = _mm_cvtepi8_epi16(c2);
        c3 = _mm_loadl_epi64((__m128i*)(iT + 12));
        c3 = _mm_cvtepi8_epi16(c3);
        c0 = _mm_unpacklo_epi16(c0, zero);
        c1 = _mm_unpacklo_epi16(c1, zero);
        c2 = _mm_unpacklo_epi16(c2, zero);
        c3 = _mm_unpacklo_epi16(c3, zero);

        //load src
        s0 = _mm_set1_epi16(coeff[0]);
        s1 = _mm_set1_epi16(coeff[line]);
        s2 = _mm_set1_epi16(coeff[2 * line]);
        s3 = _mm_set1_epi16(coeff[3 * line]);

        v0 = _mm_madd_epi16(s0, c0);
        v1 = _mm_madd_epi16(s1, c1);
        v2 = _mm_madd_epi16(s2, c2);
        v3 = _mm_madd_epi16(s3, c3);
        e0 = _mm_add_epi32(v0, v1);
        e1 = _mm_add_epi32(v2, v3);
        e2 = _mm_add_epi32(e0, e1);
        e3 = _mm_add_epi32(e2, factor);

        r0 = _mm_packs_epi32(_mm_srai_epi32(e3, shift), zero);
        _mm_storel_epi64((__m128i*)block, r0);

        coeff++;
        block += 4;
    }
}

void itrans_dst7_pb8_sse(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m128i s0, s1, s2, s3;
    __m128i c0, c1, c2, c3;
    __m128i e0, e1, e2;
    __m128i r0, r1;
    __m128i zero = _mm_setzero_si128();
    __m128i rnd_factor = _mm_set1_epi32(1 << (shift - 1));
    for (i = 0; i < line; i++)
    {
        s8 *pb8 = iT;
        for (j = 0; j < 2; j++)
        {
            __m128i v0 = zero;
            for (k = 0; k < 2; k++)
            {
                //load src
                s0 = _mm_set1_epi16(coeff[0]);
                s1 = _mm_set1_epi16(coeff[line]);
                s2 = _mm_set1_epi16(coeff[2 * line]);
                s3 = _mm_set1_epi16(coeff[3 * line]);

                c0 = _mm_loadl_epi64((__m128i*)pb8);
                c0 = _mm_cvtepi8_epi16(c0);
                c1 = _mm_loadl_epi64((__m128i*)(pb8 + 8));
                c1 = _mm_cvtepi8_epi16(c1);
                c2 = _mm_loadl_epi64((__m128i*)(pb8 + 16));
                c2 = _mm_cvtepi8_epi16(c2);
                c3 = _mm_loadl_epi64((__m128i*)(pb8 + 24));
                c3 = _mm_cvtepi8_epi16(c3);
                c0 = _mm_unpacklo_epi16(c0, zero);
                c1 = _mm_unpacklo_epi16(c1, zero);
                c2 = _mm_unpacklo_epi16(c2, zero);
                c3 = _mm_unpacklo_epi16(c3, zero);

                e0 = _mm_add_epi32(_mm_madd_epi16(s0, c0), _mm_madd_epi16(s1, c1));
                e1 = _mm_add_epi32(_mm_madd_epi16(s2, c2), _mm_madd_epi16(s3, c3));
                e2 = _mm_add_epi32(e0, e1);

                v0 = _mm_add_epi32(v0, e2);

                pb8 += 32;
                coeff += (4 * line);
            }
            r0 = _mm_add_epi32(v0, rnd_factor);
            r1 = _mm_packs_epi32(_mm_srai_epi32(r0, shift), zero);
            _mm_storel_epi64((__m128i*)block, r1);
            coeff -= (8 * line);
            pb8 -= 60;
            block += 4;
        }
        coeff++;
    }
}

void itrans_dst7_pb16_sse(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m128i s0, s1, s2, s3;
    __m128i c0, c1, c2, c3;
    __m128i e0, e1, e2;
    __m128i r0, r1;
    __m128i zero = _mm_setzero_si128();
    __m128i rnd_factor = _mm_set1_epi32(1 << (shift - 1));
    for (i = 0; i < line; i++)
    {
        s8 *pb16 = iT;
        for (j = 0; j < 4; j++)
        {
            __m128i v0 = zero;
            for (k = 0; k < 4; k++)
            {
                //load src
                s0 = _mm_set1_epi16(coeff[0]);
                s1 = _mm_set1_epi16(coeff[line]);
                s2 = _mm_set1_epi16(coeff[2 * line]);
                s3 = _mm_set1_epi16(coeff[3 * line]);

                c0 = _mm_loadl_epi64((__m128i*)pb16);
                c0 = _mm_cvtepi8_epi16(c0);
                c1 = _mm_loadl_epi64((__m128i*)(pb16 + 16));
                c1 = _mm_cvtepi8_epi16(c1);
                c2 = _mm_loadl_epi64((__m128i*)(pb16 + 32));
                c2 = _mm_cvtepi8_epi16(c2);
                c3 = _mm_loadl_epi64((__m128i*)(pb16 + 48));
                c3 = _mm_cvtepi8_epi16(c3);
                c0 = _mm_unpacklo_epi16(c0, zero);
                c1 = _mm_unpacklo_epi16(c1, zero);
                c2 = _mm_unpacklo_epi16(c2, zero);
                c3 = _mm_unpacklo_epi16(c3, zero);



                e0 = _mm_add_epi32(_mm_madd_epi16(s0, c0), _mm_madd_epi16(s1, c1));
                e1 = _mm_add_epi32(_mm_madd_epi16(s2, c2), _mm_madd_epi16(s3, c3));
                e2 = _mm_add_epi32(e0, e1);

                v0 = _mm_add_epi32(v0, e2);

                pb16 += 64;
                coeff += (4 * line);
            }
            r0 = _mm_add_epi32(v0, rnd_factor);
            r1 = _mm_packs_epi32(_mm_srai_epi32(r0, shift), zero);
            _mm_storel_epi64((__m128i*)block, r1);
            coeff -= (16 * line);
            pb16 -= 252;
            block += 4;
        }
        coeff++;
    }
}

void itrans_dst7_pb32_sse(s16 *coeff, s16 *block, int shift, int line, int limit_line, int max_tr_val, int min_tr_val, s8 *iT)
{
    int i, j, k;
    __m128i s0, s1, s2, s3;
    __m128i c0, c1, c2, c3;
    __m128i e0, e1, e2;
    __m128i r0, r1;
    __m128i zero = _mm_setzero_si128();
    __m128i rnd_factor = _mm_set1_epi32(1 << (shift - 1));

    for (i = 0; i < line; i++)
    {
        s8 *pb32 = iT;
        for (j = 0; j < 8; j++)
        {
            __m128i v0 = zero;
            for (k = 0; k < 8; k++)
            {
                //load src
                s0 = _mm_set1_epi16(coeff[0]);
                s1 = _mm_set1_epi16(coeff[line]);
                s2 = _mm_set1_epi16(coeff[2 * line]);
                s3 = _mm_set1_epi16(coeff[3 * line]);

                c0 = _mm_loadl_epi64((__m128i*)pb32);
                c0 = _mm_cvtepi8_epi16(c0);
                c1 = _mm_loadl_epi64((__m128i*)(pb32 + 32));
                c1 = _mm_cvtepi8_epi16(c1);
                c2 = _mm_loadl_epi64((__m128i*)(pb32 + 64));
                c2 = _mm_cvtepi8_epi16(c2);
                c3 = _mm_loadl_epi64((__m128i*)(pb32 + 96));
                c3 = _mm_cvtepi8_epi16(c3);
                c0 = _mm_unpacklo_epi16(c0, zero);
                c1 = _mm_unpacklo_epi16(c1, zero);
                c2 = _mm_unpacklo_epi16(c2, zero);
                c3 = _mm_unpacklo_epi16(c3, zero);

                e0 = _mm_add_epi32(_mm_madd_epi16(s0, c0), _mm_madd_epi16(s1, c1));
                e1 = _mm_add_epi32(_mm_madd_epi16(s2, c2), _mm_madd_epi16(s3, c3));
                e2 = _mm_add_epi32(e0, e1);

                v0 = _mm_add_epi32(v0, e2);

                pb32 += 128;
                coeff += (4 * line);
            }
            r0 = _mm_add_epi32(v0, rnd_factor);
            r1 = _mm_packs_epi32(_mm_srai_epi32(r0, shift), zero);
            _mm_storel_epi64((__m128i*)block, r1);
            coeff -= (32 * line);
            pb32 -= 1020;
            block += 4;
        }
        coeff++;
    }
}

