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

#if (BIT_DEPTH == 8)

void uavs3d_alf_one_lcu_avx2(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;

    __m256i T00, T01, T10, T11, T20, T21, T30, T31, T40, T41, T50, T51, T60, T61, T70, T71, T80, T81, T82, T83;
    __m256i T8;
    __m256i T000, T001, T100, T101, T200, T201, T300, T301, T400, T401, T500, T501, T600, T601, T700, T701;
    __m256i C0, C1, C2, C3, C4, C5, C6, C7, C8;
    __m256i S0, S00, S01, S1, S10, S11, S2, S20, S21, S3, S30, S31, S4, S40, S41, S5, S50, S51, S6, S7, S60, S61, S70, S71, SS1, SS2, SS3, SS4, S;
    __m256i mAddOffset;
    __m256i mZero = _mm256_set1_epi16(0);
    __m256i mMax = _mm256_set1_epi16((short)((1 << sample_bit_depth) - 1));
    __m128i m0, m1;

    int i, j;
    int startPos = 0;
    int endPos = lcu_height;

    C0 = _mm256_set1_epi8(coef[0]);     // C0-C7: [-64, 63]
    C1 = _mm256_set1_epi8(coef[1]);
    C2 = _mm256_set1_epi8(coef[2]);
    C3 = _mm256_set1_epi8(coef[3]);
    C4 = _mm256_set1_epi8(coef[4]);
    C5 = _mm256_set1_epi8(coef[5]);
    C6 = _mm256_set1_epi8(coef[6]);
    C7 = _mm256_set1_epi8(coef[7]);
    C8 = _mm256_set1_epi32(coef[8]);    // [-1088, 1071]

    mAddOffset = _mm256_set1_epi32(32);

    for (i = startPos; i < endPos; i++) {
        imgPad1 = src + i_src;
        imgPad2 = src - i_src;
        imgPad3 = src + 2 * i_src;
        imgPad4 = src - 2 * i_src;
        imgPad5 = src + 3 * i_src;
        imgPad6 = src - 3 * i_src;
        if (i < 3) {
            if (i == 0) {
                imgPad4 = imgPad2 = src;
            }
            else if (i == 1) {
                imgPad4 = imgPad2;
            }
            imgPad6 = imgPad4;
        }
        else if (i > lcu_height - 4) {
            if (i == lcu_height - 1) {
                imgPad3 = imgPad1 = src;
            }
            else if (i == lcu_height - 2) {
                imgPad3 = imgPad1;
            }
            imgPad5 = imgPad3;
        }

        for (j = 0; j < lcu_width; j += 32) {
            T00 = _mm256_load_si256((__m256i*)&imgPad6[j]);
            T01 = _mm256_load_si256((__m256i*)&imgPad5[j]);
            T000 = _mm256_unpacklo_epi8(T00, T01);
            T001 = _mm256_unpackhi_epi8(T00, T01);
            S00 = _mm256_maddubs_epi16(T000, C0);
            S01 = _mm256_maddubs_epi16(T001, C0);

            T10 = _mm256_load_si256((__m256i*)&imgPad4[j]);
            T11 = _mm256_load_si256((__m256i*)&imgPad3[j]);
            T100 = _mm256_unpacklo_epi8(T10, T11);
            T101 = _mm256_unpackhi_epi8(T10, T11);
            S10 = _mm256_maddubs_epi16(T100, C1);
            S11 = _mm256_maddubs_epi16(T101, C1);

            T20 = _mm256_loadu_si256((__m256i*)&imgPad2[j - 1]);
            T30 = _mm256_load_si256((__m256i*)&imgPad2[j]);
            T40 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 1]);
            T41 = _mm256_loadu_si256((__m256i*)&imgPad1[j - 1]);
            T31 = _mm256_load_si256((__m256i*)&imgPad1[j]);
            T21 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 1]);

            T200 = _mm256_unpacklo_epi8(T20, T21);
            T201 = _mm256_unpackhi_epi8(T20, T21);
            T300 = _mm256_unpacklo_epi8(T30, T31);
            T301 = _mm256_unpackhi_epi8(T30, T31);
            T400 = _mm256_unpacklo_epi8(T40, T41);
            T401 = _mm256_unpackhi_epi8(T40, T41);
            S20 = _mm256_maddubs_epi16(T200, C2);
            S21 = _mm256_maddubs_epi16(T201, C2);
            S30 = _mm256_maddubs_epi16(T300, C3);
            S31 = _mm256_maddubs_epi16(T301, C3);
            S40 = _mm256_maddubs_epi16(T400, C4);
            S41 = _mm256_maddubs_epi16(T401, C4);

            T50 = _mm256_loadu_si256((__m256i*)&src[j - 3]);
            T60 = _mm256_loadu_si256((__m256i*)&src[j - 2]);
            T70 = _mm256_loadu_si256((__m256i*)&src[j - 1]);
            T8 = _mm256_load_si256((__m256i*)&src[j]);
            T71 = _mm256_loadu_si256((__m256i*)&src[j + 1]);
            T61 = _mm256_loadu_si256((__m256i*)&src[j + 2]);
            T51 = _mm256_loadu_si256((__m256i*)&src[j + 3]);

            m0 = _mm256_castsi256_si128(T8);
            m1 = _mm256_extracti128_si256(T8, 1);

            T80 = _mm256_cvtepu8_epi32(m0);
            T81 = _mm256_cvtepu8_epi32(_mm_srli_si128(m0, 8));
            T82 = _mm256_cvtepu8_epi32(m1);
            T83 = _mm256_cvtepu8_epi32(_mm_srli_si128(m1, 8));
            T80 = _mm256_mullo_epi32(T80, C8);
            T81 = _mm256_mullo_epi32(T81, C8);
            T82 = _mm256_mullo_epi32(T82, C8);
            T83 = _mm256_mullo_epi32(T83, C8);

            T500 = _mm256_unpacklo_epi8(T50, T51);
            T501 = _mm256_unpackhi_epi8(T50, T51);
            T600 = _mm256_unpacklo_epi8(T60, T61);
            T601 = _mm256_unpackhi_epi8(T60, T61);
            T700 = _mm256_unpacklo_epi8(T70, T71);
            T701 = _mm256_unpackhi_epi8(T70, T71);
            S50 = _mm256_maddubs_epi16(T500, C5);
            S51 = _mm256_maddubs_epi16(T501, C5);
            S60 = _mm256_maddubs_epi16(T600, C6);
            S61 = _mm256_maddubs_epi16(T601, C6);
            S70 = _mm256_maddubs_epi16(T700, C7);
            S71 = _mm256_maddubs_epi16(T701, C7);

            S0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S00));
            S1 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S10));
            S2 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S20));
            S3 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S30));
            S4 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S40));
            S5 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S50));
            S6 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S60));
            S7 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S70));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS1 = _mm256_add_epi32(S0, S4);
            SS1 = _mm256_add_epi32(SS1, T80);    //0-7

            S0 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S00, 1));
            S1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S10, 1));
            S2 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S20, 1));
            S3 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S30, 1));
            S4 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S40, 1));
            S5 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S50, 1));
            S6 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S60, 1));
            S7 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S70, 1));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS2 = _mm256_add_epi32(S0, S4);
            SS2 = _mm256_add_epi32(SS2, T82);    //16-23

            S0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S01));
            S1 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S11));
            S2 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S21));
            S3 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S31));
            S4 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S41));
            S5 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S51));
            S6 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S61));
            S7 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S71));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS3 = _mm256_add_epi32(S0, S4);
            SS3 = _mm256_add_epi32(SS3, T81);    //8-15

            S0 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S01, 1));
            S1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S11, 1));
            S2 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S21, 1));
            S3 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S31, 1));
            S4 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S41, 1));
            S5 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S51, 1));
            S6 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S61, 1));
            S7 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S71, 1));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS4 = _mm256_add_epi32(S0, S4);
            SS4 = _mm256_add_epi32(SS4, T83);    //24-31

            SS1 = _mm256_add_epi32(SS1, mAddOffset);
            SS2 = _mm256_add_epi32(SS2, mAddOffset);
            SS3 = _mm256_add_epi32(SS3, mAddOffset);
            SS4 = _mm256_add_epi32(SS4, mAddOffset);

            SS1 = _mm256_srai_epi32(SS1, 6);
            SS2 = _mm256_srai_epi32(SS2, 6);
            SS3 = _mm256_srai_epi32(SS3, 6);
            SS4 = _mm256_srai_epi32(SS4, 6);

            SS1 = _mm256_packs_epi32(SS1, SS2);
            SS3 = _mm256_packs_epi32(SS3, SS4);
            SS1 = _mm256_permute4x64_epi64(SS1, 0xd8);
            SS3 = _mm256_permute4x64_epi64(SS3, 0xd8);

            SS1 = _mm256_min_epi16(SS1, mMax);
            SS1 = _mm256_max_epi16(SS1, mZero);
            SS3 = _mm256_min_epi16(SS3, mMax);
            SS3 = _mm256_max_epi16(SS3, mZero);

            S = _mm256_packus_epi16(SS1, SS3);
            _mm256_storeu_si256((__m256i*)(dst + j), S);
        }

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_alf_one_lcu_chroma_avx2(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;

    __m256i T00, T01, T10, T11, T20, T21, T30, T31, T40, T41, T50, T51, T60, T61, T70, T71, T80, T81, T82, T83;
    __m256i T8;
    __m256i T000, T001, T100, T101, T200, T201, T300, T301, T400, T401, T500, T501, T600, T601, T700, T701;
    __m256i C0, C1, C2, C3, C4, C5, C6, C7, C8;
    __m256i S0, S00, S01, S1, S10, S11, S2, S20, S21, S3, S30, S31, S4, S40, S41, S5, S50, S51, S6, S7, S60, S61, S70, S71, SS1, SS2, SS3, SS4, S;
    __m256i mAddOffset;
    __m128i c0, c1, c2, m0, m1;

    int i, j;
    int startPos = 0;
    int endPos = lcu_height;

    C0 = _mm256_loadu_si256((__m256i*)coef);
    C1 = _mm256_loadu_si256((__m256i*)(coef + 8));
    T8 = _mm256_packs_epi32(C0, C1);
    c0 = _mm_packs_epi16(_mm256_castsi256_si128(T8), _mm256_extracti128_si256(T8, 1));
    c1 = _mm_unpacklo_epi8(c0, c0);     // coeffs: u0 u0 v0 v0 u1 u1 v1 v1 u4 u4 v4 v4 u5 u5 v5 v5
    c2 = _mm_unpackhi_epi8(c0, c0);
    C0 = _mm256_set1_epi32(_mm_extract_epi32(c1, 0));
    C1 = _mm256_set1_epi32(_mm_extract_epi32(c1, 1));
    C2 = _mm256_set1_epi32(_mm_extract_epi32(c2, 0));
    C3 = _mm256_set1_epi32(_mm_extract_epi32(c2, 1));
    C4 = _mm256_set1_epi32(_mm_extract_epi32(c1, 2));
    C5 = _mm256_set1_epi32(_mm_extract_epi32(c1, 3));
    C6 = _mm256_set1_epi32(_mm_extract_epi32(c2, 2));
    C7 = _mm256_set1_epi32(_mm_extract_epi32(c2, 3));
    C8 = _mm256_set1_epi64x((u32)coef[16] + ((u64)((u32)coef[17]) << 32));

    mAddOffset = _mm256_set1_epi32(32);

    for (i = startPos; i < endPos; i++) {
        imgPad1 = src + i_src;
        imgPad2 = src - i_src;
        imgPad3 = src + 2 * i_src;
        imgPad4 = src - 2 * i_src;
        imgPad5 = src + 3 * i_src;
        imgPad6 = src - 3 * i_src;
        if (i < 3) {
            if (i == 0) {
                imgPad4 = imgPad2 = src;
            }
            else if (i == 1) {
                imgPad4 = imgPad2;
            }
            imgPad6 = imgPad4;
        }
        else if (i > lcu_height - 4) {
            if (i == lcu_height - 1) {
                imgPad3 = imgPad1 = src;
            }
            else if (i == lcu_height - 2) {
                imgPad3 = imgPad1;
            }
            imgPad5 = imgPad3;
        }

        for (j = 0; j < lcu_width << 1; j += 32) {
            T00 = _mm256_loadu_si256((__m256i*)&imgPad6[j]);
            T01 = _mm256_loadu_si256((__m256i*)&imgPad5[j]);
            T000 = _mm256_unpacklo_epi8(T00, T01);
            T001 = _mm256_unpackhi_epi8(T00, T01);
            S00 = _mm256_maddubs_epi16(T000, C0);
            S01 = _mm256_maddubs_epi16(T001, C0);

            T10 = _mm256_loadu_si256((__m256i*)&imgPad4[j]);
            T11 = _mm256_loadu_si256((__m256i*)&imgPad3[j]);
            T100 = _mm256_unpacklo_epi8(T10, T11);
            T101 = _mm256_unpackhi_epi8(T10, T11);
            S10 = _mm256_maddubs_epi16(T100, C1);
            S11 = _mm256_maddubs_epi16(T101, C1);

            T20 = _mm256_loadu_si256((__m256i*)&imgPad2[j - 2]);
            T30 = _mm256_loadu_si256((__m256i*)&imgPad2[j]);
            T40 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 2]);
            T41 = _mm256_loadu_si256((__m256i*)&imgPad1[j - 2]);
            T31 = _mm256_loadu_si256((__m256i*)&imgPad1[j]);
            T21 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 2]);

            T200 = _mm256_unpacklo_epi8(T20, T21);
            T201 = _mm256_unpackhi_epi8(T20, T21);
            T300 = _mm256_unpacklo_epi8(T30, T31);
            T301 = _mm256_unpackhi_epi8(T30, T31);
            T400 = _mm256_unpacklo_epi8(T40, T41);
            T401 = _mm256_unpackhi_epi8(T40, T41);
            S20 = _mm256_maddubs_epi16(T200, C2);
            S21 = _mm256_maddubs_epi16(T201, C2);
            S30 = _mm256_maddubs_epi16(T300, C3);
            S31 = _mm256_maddubs_epi16(T301, C3);
            S40 = _mm256_maddubs_epi16(T400, C4);
            S41 = _mm256_maddubs_epi16(T401, C4);

            T50 = _mm256_loadu_si256((__m256i*)&src[j - 6]);
            T60 = _mm256_loadu_si256((__m256i*)&src[j - 4]);
            T70 = _mm256_loadu_si256((__m256i*)&src[j - 2]);
            T8 = _mm256_loadu_si256((__m256i*)&src[j]);
            T71 = _mm256_loadu_si256((__m256i*)&src[j + 2]);
            T61 = _mm256_loadu_si256((__m256i*)&src[j + 4]);
            T51 = _mm256_loadu_si256((__m256i*)&src[j + 6]);

            m0 = _mm256_castsi256_si128(T8);
            m1 = _mm256_extracti128_si256(T8, 1);

            T80 = _mm256_cvtepu8_epi32(m0);
            T81 = _mm256_cvtepu8_epi32(_mm_srli_si128(m0, 8));
            T82 = _mm256_cvtepu8_epi32(m1);
            T83 = _mm256_cvtepu8_epi32(_mm_srli_si128(m1, 8));
            T80 = _mm256_mullo_epi32(T80, C8);
            T81 = _mm256_mullo_epi32(T81, C8);
            T82 = _mm256_mullo_epi32(T82, C8);
            T83 = _mm256_mullo_epi32(T83, C8);

            T500 = _mm256_unpacklo_epi8(T50, T51);
            T501 = _mm256_unpackhi_epi8(T50, T51);
            T600 = _mm256_unpacklo_epi8(T60, T61);
            T601 = _mm256_unpackhi_epi8(T60, T61);
            T700 = _mm256_unpacklo_epi8(T70, T71);
            T701 = _mm256_unpackhi_epi8(T70, T71);
            S50 = _mm256_maddubs_epi16(T500, C5);
            S51 = _mm256_maddubs_epi16(T501, C5);
            S60 = _mm256_maddubs_epi16(T600, C6);
            S61 = _mm256_maddubs_epi16(T601, C6);
            S70 = _mm256_maddubs_epi16(T700, C7);
            S71 = _mm256_maddubs_epi16(T701, C7);

            S0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S00));
            S1 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S10));
            S2 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S20));
            S3 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S30));
            S4 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S40));
            S5 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S50));
            S6 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S60));
            S7 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S70));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS1 = _mm256_add_epi32(S0, S4);
            SS1 = _mm256_add_epi32(SS1, T80);    //0-7

            S0 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S00, 1));
            S1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S10, 1));
            S2 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S20, 1));
            S3 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S30, 1));
            S4 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S40, 1));
            S5 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S50, 1));
            S6 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S60, 1));
            S7 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S70, 1));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS2 = _mm256_add_epi32(S0, S4);
            SS2 = _mm256_add_epi32(SS2, T82);    //16-23

            S0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S01));
            S1 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S11));
            S2 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S21));
            S3 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S31));
            S4 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S41));
            S5 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S51));
            S6 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S61));
            S7 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S71));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS3 = _mm256_add_epi32(S0, S4);
            SS3 = _mm256_add_epi32(SS3, T81);    //8-15

            S0 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S01, 1));
            S1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S11, 1));
            S2 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S21, 1));
            S3 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S31, 1));
            S4 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S41, 1));
            S5 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S51, 1));
            S6 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S61, 1));
            S7 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S71, 1));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS4 = _mm256_add_epi32(S0, S4);
            SS4 = _mm256_add_epi32(SS4, T83);    //24-31

            SS1 = _mm256_add_epi32(SS1, mAddOffset);
            SS2 = _mm256_add_epi32(SS2, mAddOffset);
            SS3 = _mm256_add_epi32(SS3, mAddOffset);
            SS4 = _mm256_add_epi32(SS4, mAddOffset);

            SS1 = _mm256_srai_epi32(SS1, 6);
            SS2 = _mm256_srai_epi32(SS2, 6);
            SS3 = _mm256_srai_epi32(SS3, 6);
            SS4 = _mm256_srai_epi32(SS4, 6);

            SS1 = _mm256_packs_epi32(SS1, SS2);
            SS3 = _mm256_packs_epi32(SS3, SS4);
            SS1 = _mm256_permute4x64_epi64(SS1, 0xd8);
            SS3 = _mm256_permute4x64_epi64(SS3, 0xd8);

            S = _mm256_packus_epi16(SS1, SS3);
            _mm256_storeu_si256((__m256i*)(dst + j), S);
        }

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_alf_one_lcu_one_chroma_avx2(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;

    __m256i T00, T01, T10, T11, T20, T21, T30, T31, T40, T41, T50, T51, T60, T61, T70, T71, T80, T81;
    __m256i T8;
    __m256i T000, T001, T010, T011, T100, T101, T110, T111, T200, T201, T210, T211, T310, T311, T300, T301, T400, T401, T410, T411, T500, T501, T510, T511, T600, T601, T610, T611, T700, T701, T710, T711;
    __m256i E00, E01, E10, E11;
    __m256i C0, C1, C2, C3, C4, C5, C6, C7, C8;
    __m256i S0, S00, S01, S1, S10, S11, S2, S20, S21, S3, S30, S31, S4, S40, S41, S5, S50, S51, S6, S7, S60, S61, S70, S71, S80, S81, S82, S83, SS1, SS2, SS3, SS4;
    __m256i mAddOffset;
    __m256i mZero = _mm256_set1_epi16(0);
    __m256i mMax = _mm256_set1_epi16((short)((1 << sample_bit_depth) - 1));
    __m256i uv_mask = _mm256_set1_epi16(0xff);

    int i, j;
    int startPos = 0;
    int endPos = lcu_height;

    C0 = _mm256_set1_epi16(coef[0]);
    C1 = _mm256_set1_epi16(coef[2]);
    C2 = _mm256_set1_epi16(coef[4]);
    C3 = _mm256_set1_epi16(coef[6]);
    C4 = _mm256_set1_epi16(coef[8]);
    C5 = _mm256_set1_epi16(coef[10]);
    C6 = _mm256_set1_epi16(coef[12]);
    C7 = _mm256_set1_epi16(coef[14]);
    C8 = _mm256_set1_epi32(coef[16]);

    mAddOffset = _mm256_set1_epi32(32);

    for (i = startPos; i < endPos; i++) {
        imgPad1 = src + i_src;
        imgPad2 = src - i_src;
        imgPad3 = src + 2 * i_src;
        imgPad4 = src - 2 * i_src;
        imgPad5 = src + 3 * i_src;
        imgPad6 = src - 3 * i_src;
        if (i < 3) {
            if (i == 0) {
                imgPad4 = imgPad2 = src;
            }
            else if (i == 1) {
                imgPad4 = imgPad2;
            }
            imgPad6 = imgPad4;
        }
        else if (i > lcu_height - 4) {
            if (i == lcu_height - 1) {
                imgPad3 = imgPad1 = src;
            }
            else if (i == lcu_height - 2) {
                imgPad3 = imgPad1;
            }
            imgPad5 = imgPad3;
        }

        if (lcu_width <= 16) {
            T00 = _mm256_loadu_si256((__m256i*)&imgPad6[0]);
            T000 = _mm256_and_si256(T00, uv_mask);
            T00 = _mm256_loadu_si256((__m256i*)&imgPad5[0]);
            T010 = _mm256_and_si256(T00, uv_mask);
            E00 = _mm256_add_epi16(T000, T010);
            S00 = _mm256_mullo_epi16(C0, E00);//前16个像素所有C0*P0的结果

            T10 = _mm256_loadu_si256((__m256i*)&imgPad4[0]);
            T100 = _mm256_and_si256(T10, uv_mask);
            T11 = _mm256_loadu_si256((__m256i*)&imgPad3[0]);
            T110 = _mm256_and_si256(T11, uv_mask);
            E10 = _mm256_add_epi16(T100, T110);
            S10 = _mm256_mullo_epi16(C1, E10);//前16个像素所有C1*P1的结果


            T20 = _mm256_loadu_si256((__m256i*)&imgPad2[0 - 2]);
            T200 = _mm256_and_si256(T20, uv_mask);
            T30 = _mm256_loadu_si256((__m256i*)&imgPad2[0]);
            T300 = _mm256_and_si256(T30, uv_mask);
            T40 = _mm256_loadu_si256((__m256i*)&imgPad2[0 + 2]);
            T400 = _mm256_and_si256(T40, uv_mask);

            T41 = _mm256_loadu_si256((__m256i*)&imgPad1[0 - 2]);
            T410 = _mm256_and_si256(T41, uv_mask);
            T31 = _mm256_loadu_si256((__m256i*)&imgPad1[0]);
            T310 = _mm256_and_si256(T31, uv_mask);
            T21 = _mm256_loadu_si256((__m256i*)&imgPad1[0 + 2]);
            T210 = _mm256_and_si256(T21, uv_mask);

            T20 = _mm256_add_epi16(T200, T210);
            T30 = _mm256_add_epi16(T300, T310);
            T40 = _mm256_add_epi16(T400, T410);


            S20 = _mm256_mullo_epi16(T20, C2);
            S30 = _mm256_mullo_epi16(T30, C3);
            S40 = _mm256_mullo_epi16(T40, C4);

            T50 = _mm256_loadu_si256((__m256i*)&src[0 - 6]);
            T500 = _mm256_and_si256(T50, uv_mask);
            T60 = _mm256_loadu_si256((__m256i*)&src[0 - 4]);
            T600 = _mm256_and_si256(T60, uv_mask);
            T70 = _mm256_loadu_si256((__m256i*)&src[0 - 2]);
            T700 = _mm256_and_si256(T70, uv_mask);
            T8 = _mm256_loadu_si256((__m256i*)&src[0]);
            T80 = _mm256_and_si256(T8, uv_mask);
            T71 = _mm256_loadu_si256((__m256i*)&src[0 + 2]);
            T710 = _mm256_and_si256(T71, uv_mask);
            T61 = _mm256_loadu_si256((__m256i*)&src[0 + 4]);
            T610 = _mm256_and_si256(T61, uv_mask);
            T51 = _mm256_loadu_si256((__m256i*)&src[0 + 6]);
            T510 = _mm256_and_si256(T51, uv_mask);

            T50 = _mm256_add_epi16(T500, T510);
            T60 = _mm256_add_epi16(T600, T610);
            T70 = _mm256_add_epi16(T700, T710);

            S50 = _mm256_mullo_epi16(T50, C5);
            S60 = _mm256_mullo_epi16(T60, C6);
            S70 = _mm256_mullo_epi16(T70, C7);

            S80 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(T80));
            S81 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(T80, 1));
            S80 = _mm256_mullo_epi16(S80, C8);
            S81 = _mm256_mullo_epi16(S81, C8);

            S0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S00));
            S1 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S10));
            S2 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S20));
            S3 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S30));
            S4 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S40));
            S5 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S50));
            S6 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S60));
            S7 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S70));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS1 = _mm256_add_epi32(S0, S4);
            SS1 = _mm256_add_epi32(SS1, S80);    //0-7

            S0 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S00, 1));
            S1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S10, 1));
            S2 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S20, 1));
            S3 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S30, 1));
            S4 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S40, 1));
            S5 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S50, 1));
            S6 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S60, 1));
            S7 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S70, 1));
            //S8 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S80, 1));
            S0 = _mm256_add_epi32(S0, S1);
            S2 = _mm256_add_epi32(S2, S3);
            S4 = _mm256_add_epi32(S4, S5);
            S6 = _mm256_add_epi32(S6, S7);
            S0 = _mm256_add_epi32(S0, S2);
            S4 = _mm256_add_epi32(S4, S6);
            SS2 = _mm256_add_epi32(S0, S4);
            SS2 = _mm256_add_epi32(SS2, S81);    //8-15

            SS1 = _mm256_add_epi32(SS1, mAddOffset);
            SS2 = _mm256_add_epi32(SS2, mAddOffset);

            SS1 = _mm256_srai_epi32(SS1, 6);
            SS2 = _mm256_srai_epi32(SS2, 6);

            SS1 = _mm256_packs_epi32(SS1, SS2);
            SS1 = _mm256_permute4x64_epi64(SS1, 0xd8);

            T00 = _mm256_loadu_si256((__m256i*)&dst[0]);

            SS1 = _mm256_min_epi16(SS1, mMax);
            SS1 = _mm256_max_epi16(SS1, mZero);

            T00 = _mm256_blendv_epi8(T00, SS1, uv_mask);
            _mm256_storeu_si256((__m256i*)(dst), T00);
        }
        else {
            for (j = 0; j < lcu_width << 1; j += 64) {
                T00 = _mm256_loadu_si256((__m256i*)&imgPad6[j]);
                T01 = _mm256_loadu_si256((__m256i*)&imgPad6[j + 32]);
                T000 = _mm256_and_si256(T00, uv_mask);
                T001 = _mm256_and_si256(T01, uv_mask);
                T00 = _mm256_loadu_si256((__m256i*)&imgPad5[j]);
                T01 = _mm256_loadu_si256((__m256i*)&imgPad5[j + 32]);
                T010 = _mm256_and_si256(T00, uv_mask);
                T011 = _mm256_and_si256(T01, uv_mask);
                E00 = _mm256_add_epi16(T000, T010);
                E01 = _mm256_add_epi16(T001, T011);
                S00 = _mm256_mullo_epi16(C0, E00);//前16个像素所有C0*P0的结果
                S01 = _mm256_mullo_epi16(C0, E01);//后16个像素所有C0*P0的结果

                T10 = _mm256_loadu_si256((__m256i*)&imgPad4[j]);
                T11 = _mm256_loadu_si256((__m256i*)&imgPad4[j + 32]);
                T100 = _mm256_and_si256(T10, uv_mask);
                T101 = _mm256_and_si256(T11, uv_mask);
                T11 = _mm256_loadu_si256((__m256i*)&imgPad3[j]);
                T00 = _mm256_loadu_si256((__m256i*)&imgPad3[j + 32]);
                T110 = _mm256_and_si256(T11, uv_mask);
                T111 = _mm256_and_si256(T00, uv_mask);
                E10 = _mm256_add_epi16(T100, T110);
                E11 = _mm256_add_epi16(T101, T111);
                S10 = _mm256_mullo_epi16(C1, E10);//前16个像素所有C1*P1的结果
                S11 = _mm256_mullo_epi16(C1, E11);//后16个像素所有C1*P1的结果


                T20 = _mm256_loadu_si256((__m256i*)&imgPad2[j - 2]);
                T30 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 30]);
                T200 = _mm256_and_si256(T20, uv_mask);
                T201 = _mm256_and_si256(T30, uv_mask);
                T30 = _mm256_loadu_si256((__m256i*)&imgPad2[j]);
                T40 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 32]);
                T300 = _mm256_and_si256(T30, uv_mask);
                T301 = _mm256_and_si256(T40, uv_mask);
                T40 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 2]);
                T41 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 34]);
                T400 = _mm256_and_si256(T40, uv_mask);
                T401 = _mm256_and_si256(T41, uv_mask);

                T41 = _mm256_loadu_si256((__m256i*)&imgPad1[j - 2]);
                T40 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 30]);
                T410 = _mm256_and_si256(T41, uv_mask);
                T411 = _mm256_and_si256(T40, uv_mask);
                T31 = _mm256_loadu_si256((__m256i*)&imgPad1[j]);
                T00 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 32]);
                T310 = _mm256_and_si256(T31, uv_mask);
                T311 = _mm256_and_si256(T00, uv_mask);
                T21 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 2]);
                T01 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 34]);
                T210 = _mm256_and_si256(T21, uv_mask);
                T211 = _mm256_and_si256(T01, uv_mask);

                T20 = _mm256_add_epi16(T200, T210); // 前16个数
                T21 = _mm256_add_epi16(T201, T211); //后16个数
                T30 = _mm256_add_epi16(T300, T310);
                T31 = _mm256_add_epi16(T301, T311);
                T40 = _mm256_add_epi16(T400, T410);
                T41 = _mm256_add_epi16(T401, T411);


                S20 = _mm256_mullo_epi16(T20, C2);
                S21 = _mm256_mullo_epi16(T21, C2);
                S30 = _mm256_mullo_epi16(T30, C3);
                S31 = _mm256_mullo_epi16(T31, C3);
                S40 = _mm256_mullo_epi16(T40, C4);
                S41 = _mm256_mullo_epi16(T41, C4);

                T50 = _mm256_loadu_si256((__m256i*)&src[j - 6]);
                T00 = _mm256_loadu_si256((__m256i*)&src[j + 26]);
                T500 = _mm256_and_si256(T50, uv_mask);
                T501 = _mm256_and_si256(T00, uv_mask);
                T60 = _mm256_loadu_si256((__m256i*)&src[j - 4]);
                T20 = _mm256_loadu_si256((__m256i*)&src[j + 28]);
                T600 = _mm256_and_si256(T60, uv_mask);
                T601 = _mm256_and_si256(T20, uv_mask);
                T70 = _mm256_loadu_si256((__m256i*)&src[j - 2]);
                T30 = _mm256_loadu_si256((__m256i*)&src[j + 30]);
                T700 = _mm256_and_si256(T70, uv_mask);
                T701 = _mm256_and_si256(T30, uv_mask);
                T8 = _mm256_loadu_si256((__m256i*)&src[j]);
                T01 = _mm256_loadu_si256((__m256i*)&src[j + 32]);
                T80 = _mm256_and_si256(T8, uv_mask);
                T81 = _mm256_and_si256(T01, uv_mask);
                T71 = _mm256_loadu_si256((__m256i*)&src[j + 2]);
                T11 = _mm256_loadu_si256((__m256i*)&src[j + 34]);
                T710 = _mm256_and_si256(T71, uv_mask);
                T711 = _mm256_and_si256(T11, uv_mask);
                T61 = _mm256_loadu_si256((__m256i*)&src[j + 4]);
                T21 = _mm256_loadu_si256((__m256i*)&src[j + 36]);
                T610 = _mm256_and_si256(T61, uv_mask);
                T611 = _mm256_and_si256(T21, uv_mask);
                T51 = _mm256_loadu_si256((__m256i*)&src[j + 6]);
                T31 = _mm256_loadu_si256((__m256i*)&src[j + 38]);
                T510 = _mm256_and_si256(T51, uv_mask);
                T511 = _mm256_and_si256(T31, uv_mask);

                T50 = _mm256_add_epi16(T500, T510);
                T51 = _mm256_add_epi16(T501, T511);
                T60 = _mm256_add_epi16(T600, T610);
                T61 = _mm256_add_epi16(T601, T611);
                T70 = _mm256_add_epi16(T700, T710);
                T71 = _mm256_add_epi16(T701, T711);

                S50 = _mm256_mullo_epi16(T50, C5);
                S51 = _mm256_mullo_epi16(T51, C5);
                S60 = _mm256_mullo_epi16(T60, C6);
                S61 = _mm256_mullo_epi16(T61, C6);
                S70 = _mm256_mullo_epi16(T70, C7);
                S71 = _mm256_mullo_epi16(T71, C7);

                S80 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(T80));
                S81 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(T80, 1));
                S82 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(T81));
                S83 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(T81, 1));
                S80 = _mm256_mullo_epi16(S80, C8);
                S81 = _mm256_mullo_epi16(S81, C8);
                S82 = _mm256_mullo_epi16(S82, C8);
                S83 = _mm256_mullo_epi16(S83, C8);

                S0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S00));
                S1 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S10));
                S2 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S20));
                S3 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S30));
                S4 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S40));
                S5 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S50));
                S6 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S60));
                S7 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S70));
                S0 = _mm256_add_epi32(S0, S1);
                S2 = _mm256_add_epi32(S2, S3);
                S4 = _mm256_add_epi32(S4, S5);
                S6 = _mm256_add_epi32(S6, S7);
                S0 = _mm256_add_epi32(S0, S2);
                S4 = _mm256_add_epi32(S4, S6);
                SS1 = _mm256_add_epi32(S0, S4);
                SS1 = _mm256_add_epi32(SS1, S80);    //0-7

                S0 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S00, 1));
                S1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S10, 1));
                S2 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S20, 1));
                S3 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S30, 1));
                S4 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S40, 1));
                S5 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S50, 1));
                S6 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S60, 1));
                S7 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S70, 1));
                //S8 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S80, 1));
                S0 = _mm256_add_epi32(S0, S1);
                S2 = _mm256_add_epi32(S2, S3);
                S4 = _mm256_add_epi32(S4, S5);
                S6 = _mm256_add_epi32(S6, S7);
                S0 = _mm256_add_epi32(S0, S2);
                S4 = _mm256_add_epi32(S4, S6);
                SS2 = _mm256_add_epi32(S0, S4);
                SS2 = _mm256_add_epi32(SS2, S81);    //8-15

                S0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S01));
                S1 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S11));
                S2 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S21));
                S3 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S31));
                S4 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S41));
                S5 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S51));
                S6 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S61));
                S7 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S71));
                //S8 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(S81));
                S0 = _mm256_add_epi32(S0, S1);
                S2 = _mm256_add_epi32(S2, S3);
                S4 = _mm256_add_epi32(S4, S5);
                S6 = _mm256_add_epi32(S6, S7);
                S0 = _mm256_add_epi32(S0, S2);
                S4 = _mm256_add_epi32(S4, S6);
                SS3 = _mm256_add_epi32(S0, S4);
                SS3 = _mm256_add_epi32(SS3, S82);    //16-23

                S0 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S01, 1));
                S1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S11, 1));
                S2 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S21, 1));
                S3 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S31, 1));
                S4 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S41, 1));
                S5 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S51, 1));
                S6 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S61, 1));
                S7 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S71, 1));
                //S8 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(S81, 1));
                S0 = _mm256_add_epi32(S0, S1);
                S2 = _mm256_add_epi32(S2, S3);
                S4 = _mm256_add_epi32(S4, S5);
                S6 = _mm256_add_epi32(S6, S7);
                S0 = _mm256_add_epi32(S0, S2);
                S4 = _mm256_add_epi32(S4, S6);
                SS4 = _mm256_add_epi32(S0, S4);
                SS4 = _mm256_add_epi32(SS4, S83);    //24-31

                SS1 = _mm256_add_epi32(SS1, mAddOffset);
                SS2 = _mm256_add_epi32(SS2, mAddOffset);
                SS3 = _mm256_add_epi32(SS3, mAddOffset);
                SS4 = _mm256_add_epi32(SS4, mAddOffset);

                SS1 = _mm256_srai_epi32(SS1, 6);
                SS2 = _mm256_srai_epi32(SS2, 6);
                SS3 = _mm256_srai_epi32(SS3, 6);
                SS4 = _mm256_srai_epi32(SS4, 6);

                SS1 = _mm256_packs_epi32(SS1, SS2);
                SS3 = _mm256_packs_epi32(SS3, SS4);
                SS1 = _mm256_permute4x64_epi64(SS1, 0xd8);
                SS3 = _mm256_permute4x64_epi64(SS3, 0xd8);

                T00 = _mm256_loadu_si256((__m256i*)&dst[j]);
                T01 = _mm256_loadu_si256((__m256i*)&dst[j + 32]);

                SS1 = _mm256_min_epi16(SS1, mMax);
                SS1 = _mm256_max_epi16(SS1, mZero);
                SS3 = _mm256_min_epi16(SS3, mMax);
                SS3 = _mm256_max_epi16(SS3, mZero);

                T00 = _mm256_blendv_epi8(T00, SS1, uv_mask);
                T01 = _mm256_blendv_epi8(T01, SS3, uv_mask);
                _mm256_storeu_si256((__m256i*)(dst + j), T00);
                _mm256_storeu_si256((__m256i*)(dst + j + 32), T01);
            }
        }
        src += i_src;
        dst += i_dst;
    }
}
#else

void uavs3d_alf_one_lcu_avx2(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int bit_depth)
{
    pel *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;

    __m256i T00, T01, T10, T11, T20, T21, T30, T31, T40, T41, T42, T43, T44, T45;
    __m256i E00, E01, E10, E11, E20, E21, E30, E31, E40, E41, E42, E43, E44, E45;
    __m256i C0, C1, C2, C3, C4, C5, C6, C7, C8;
    __m256i S00, S01, S10, S11, S20, S21, S30, S31, S40, S41, S50, S51, S60, S61, SS1, SS2, S, S70, S71, S80, S81;
    __m256i mAddOffset;
    __m256i zero = _mm256_setzero_si256();
    int max_pixel = (1 << bit_depth) - 1;
    __m256i max_val = _mm256_set1_epi16(max_pixel);

    int i, j;
    int startPos = 0;
    int endPos = lcu_height;

    C0 = _mm256_set1_epi16((pel)coef[0]);
    C1 = _mm256_set1_epi16((pel)coef[1]);
    C2 = _mm256_set1_epi16((pel)coef[2]);
    C3 = _mm256_set1_epi16((pel)coef[3]);
    C4 = _mm256_set1_epi16((pel)coef[4]);
    C5 = _mm256_set1_epi16((pel)coef[5]);
    C6 = _mm256_set1_epi16((pel)coef[6]);
    C7 = _mm256_set1_epi16((pel)coef[7]);
    C8 = _mm256_set1_epi16((pel)coef[8]);

    mAddOffset = _mm256_set1_epi32(32);

    for (i = 0; i < lcu_height; i++) {
        imgPad1 = src + i_src;
        imgPad2 = src - i_src;
        imgPad3 = src + 2 * i_src;
        imgPad4 = src - 2 * i_src;
        imgPad5 = src + 3 * i_src;
        imgPad6 = src - 3 * i_src;
        if (i < 3) {
            if (i == 0) {
                imgPad4 = imgPad2 = src;
            }
            else if (i == 1) {
                imgPad4 = imgPad2;
            }
            imgPad6 = imgPad4;
        }
        else if (i > lcu_height - 4) {
            if (i == lcu_height - 1) {
                imgPad3 = imgPad1 = src;
            }
            else if (i == lcu_height - 2) {
                imgPad3 = imgPad1;
            }
            imgPad5 = imgPad3;
        }

        for (j = 0; j < lcu_width; j += 16) {
            T00 = _mm256_load_si256((__m256i*)&imgPad6[j]);
            T01 = _mm256_load_si256((__m256i*)&imgPad5[j]);
            T10 = _mm256_load_si256((__m256i*)&imgPad4[j]);
            T11 = _mm256_load_si256((__m256i*)&imgPad3[j]);
            E00 = _mm256_unpacklo_epi16(T00, T01);
            E01 = _mm256_unpackhi_epi16(T00, T01);
            E10 = _mm256_unpacklo_epi16(T10, T11);
            E11 = _mm256_unpackhi_epi16(T10, T11);

            S00 = _mm256_madd_epi16(E00, C0);//前8个像素所有C0*P0的结果
            S01 = _mm256_madd_epi16(E01, C0);//后8个像素所有C0*P0的结果
            S10 = _mm256_madd_epi16(E10, C1);//前8个像素所有C1*P1的结果
            S11 = _mm256_madd_epi16(E11, C1);//后8个像素所有C1*P1的结果

            T20 = _mm256_loadu_si256((__m256i*)&imgPad2[j - 1]);
            T21 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 1]);
            T30 = _mm256_load_si256((__m256i*)&imgPad2[j]);
            T31 = _mm256_load_si256((__m256i*)&imgPad1[j]);
            T40 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 1]);
            T41 = _mm256_loadu_si256((__m256i*)&imgPad1[j - 1]);

            E20 = _mm256_unpacklo_epi16(T20, T21);
            E21 = _mm256_unpackhi_epi16(T20, T21);
            E30 = _mm256_unpacklo_epi16(T30, T31);
            E31 = _mm256_unpackhi_epi16(T30, T31);
            E40 = _mm256_unpacklo_epi16(T40, T41);
            E41 = _mm256_unpackhi_epi16(T40, T41);

            S20 = _mm256_madd_epi16(E20, C2);
            S21 = _mm256_madd_epi16(E21, C2);
            S30 = _mm256_madd_epi16(E30, C3);
            S31 = _mm256_madd_epi16(E31, C3);
            S40 = _mm256_madd_epi16(E40, C4);
            S41 = _mm256_madd_epi16(E41, C4);

            T40 = _mm256_loadu_si256((__m256i*)&src[j - 3]);
            T41 = _mm256_loadu_si256((__m256i*)&src[j + 3]);
            T42 = _mm256_loadu_si256((__m256i*)&src[j - 2]);
            T43 = _mm256_loadu_si256((__m256i*)&src[j + 2]);
            T44 = _mm256_loadu_si256((__m256i*)&src[j - 1]);
            T45 = _mm256_loadu_si256((__m256i*)&src[j + 1]);

            E40 = _mm256_unpacklo_epi16(T40, T41);
            E41 = _mm256_unpackhi_epi16(T40, T41);
            E42 = _mm256_unpacklo_epi16(T42, T43);
            E43 = _mm256_unpackhi_epi16(T42, T43);
            E44 = _mm256_unpacklo_epi16(T44, T45);
            E45 = _mm256_unpackhi_epi16(T44, T45);

            S50 = _mm256_madd_epi16(E40, C5);
            S51 = _mm256_madd_epi16(E41, C5);
            S60 = _mm256_madd_epi16(E42, C6);
            S61 = _mm256_madd_epi16(E43, C6);
            S70 = _mm256_madd_epi16(E44, C7);
            S71 = _mm256_madd_epi16(E45, C7);

            T40 = _mm256_load_si256((__m256i*)&src[j]);
            E40 = _mm256_unpacklo_epi16(T40, zero);
            E41 = _mm256_unpackhi_epi16(T40, zero);
            S80 = _mm256_madd_epi16(E40, C8);
            S81 = _mm256_madd_epi16(E41, C8);

            SS1 = _mm256_add_epi32(S00, S10);
            SS1 = _mm256_add_epi32(SS1, S20);
            SS1 = _mm256_add_epi32(SS1, S30);
            SS1 = _mm256_add_epi32(SS1, S40);
            SS1 = _mm256_add_epi32(SS1, S50);
            SS1 = _mm256_add_epi32(SS1, S60);
            SS1 = _mm256_add_epi32(SS1, S70);
            SS1 = _mm256_add_epi32(SS1, S80);

            SS2 = _mm256_add_epi32(S01, S11);
            SS2 = _mm256_add_epi32(SS2, S21);
            SS2 = _mm256_add_epi32(SS2, S31);
            SS2 = _mm256_add_epi32(SS2, S41);
            SS2 = _mm256_add_epi32(SS2, S51);
            SS2 = _mm256_add_epi32(SS2, S61);
            SS2 = _mm256_add_epi32(SS2, S71);
            SS2 = _mm256_add_epi32(SS2, S81);

            SS1 = _mm256_add_epi32(SS1, mAddOffset);
            SS1 = _mm256_srai_epi32(SS1, 6);

            SS2 = _mm256_add_epi32(SS2, mAddOffset);
            SS2 = _mm256_srai_epi32(SS2, 6);

            S = _mm256_packus_epi32(SS1, SS2);
            S = _mm256_min_epu16(S, max_val);

            _mm256_store_si256((__m256i*)(dst + j), S);

        }

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_alf_one_lcu_chroma_avx2(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int bit_depth)
{
    pel *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;

    __m256i T00, T01, T10, T11, T20, T21, T30, T31, T40, T41;
    __m256i E00, E01, E10, E11, E20, E21, E30, E31, E40, E41;
    __m256i C0, C1, C2, C3, C4, C5, C6, C7, C8;
    __m256i S00, S01, S10, S11, S20, S21, S30, S31, S40, S41, S50, S51, S60, S61, SS1, SS2, S, S70, S71, S80, S81;
    __m256i mAddOffset;
    __m256i zero = _mm256_setzero_si256();
    int max_pixel = (1 << bit_depth) - 1;
    __m256i max_val = _mm256_set1_epi16(max_pixel);

    int i, j;
    int startPos = 0;
    int endPos = lcu_height;
    int xPosEnd = lcu_width << 1;

    src += (startPos*i_src);
    dst += (startPos*i_dst);

    C0 = _mm256_loadu_si256((__m256i*)coef);
    C1 = _mm256_loadu_si256((__m256i*)(coef + 8));
    C8 = _mm256_packs_epi32(C0, C1);
    T00 = _mm256_unpacklo_epi16(C8, C8);
    T01 = _mm256_unpackhi_epi16(C8, C8);

    C0 = _mm256_permute4x64_epi64(T00, 0x00);
    C1 = _mm256_permute4x64_epi64(T00, 0x55);
    C2 = _mm256_permute4x64_epi64(T00, 0xaa);
    C3 = _mm256_permute4x64_epi64(T00, 0xff);
    C4 = _mm256_permute4x64_epi64(T01, 0x00);
    C5 = _mm256_permute4x64_epi64(T01, 0x55);
    C6 = _mm256_permute4x64_epi64(T01, 0xaa);
    C7 = _mm256_permute4x64_epi64(T01, 0xff);
    C8 = _mm256_set1_epi32((unsigned short)coef[16] + (((unsigned short)coef[17]) << 16));
    C8 = _mm256_unpacklo_epi16(C8, C8);

    mAddOffset = _mm256_set1_epi32(32);

    for (i = startPos; i < endPos; i++) {
        imgPad1 = src + i_src;
        imgPad2 = src - i_src;
        imgPad3 = src + 2 * i_src;
        imgPad4 = src - 2 * i_src;
        imgPad5 = src + 3 * i_src;
        imgPad6 = src - 3 * i_src;
        if (i < 3) {
            if (i == 0) {
                imgPad4 = imgPad2 = src;
            }
            else if (i == 1) {
                imgPad4 = imgPad2;
            }
            imgPad6 = imgPad4;
        }
        else if (i > lcu_height - 4) {
            if (i == lcu_height - 1) {
                imgPad3 = imgPad1 = src;
            }
            else if (i == lcu_height - 2) {
                imgPad3 = imgPad1;
            }
            imgPad5 = imgPad3;
        }

        for (j = 0; j < xPosEnd; j += 16) {
            T00 = _mm256_load_si256((__m256i*)&imgPad6[j]);
            T01 = _mm256_load_si256((__m256i*)&imgPad5[j]);
            E00 = _mm256_unpacklo_epi16(T00, T01);
            E01 = _mm256_unpackhi_epi16(T00, T01);
            S00 = _mm256_madd_epi16(E00, C0);//前8个像素所有C0*P0的结果
            S01 = _mm256_madd_epi16(E01, C0);//后8个像素所有C0*P0的结果

            T10 = _mm256_load_si256((__m256i*)&imgPad4[j]);
            T11 = _mm256_load_si256((__m256i*)&imgPad3[j]);
            E10 = _mm256_unpacklo_epi16(T10, T11);
            E11 = _mm256_unpackhi_epi16(T10, T11);
            S10 = _mm256_madd_epi16(E10, C1);//前8个像素所有C1*P1的结果
            S11 = _mm256_madd_epi16(E11, C1);//后8个像素所有C1*P1的结果

            T20 = _mm256_loadu_si256((__m256i*)&imgPad2[j - 2]);
            T21 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 2]);
            E20 = _mm256_unpacklo_epi16(T20, T21);
            E21 = _mm256_unpackhi_epi16(T20, T21);
            S20 = _mm256_madd_epi16(E20, C2);
            S21 = _mm256_madd_epi16(E21, C2);

            T30 = _mm256_load_si256((__m256i*)&imgPad2[j]);
            T31 = _mm256_load_si256((__m256i*)&imgPad1[j]);
            E30 = _mm256_unpacklo_epi16(T30, T31);
            E31 = _mm256_unpackhi_epi16(T30, T31);
            S30 = _mm256_madd_epi16(E30, C3);
            S31 = _mm256_madd_epi16(E31, C3);

            T40 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 2]);
            T41 = _mm256_loadu_si256((__m256i*)&imgPad1[j - 2]);
            E40 = _mm256_unpacklo_epi16(T40, T41);
            E41 = _mm256_unpackhi_epi16(T40, T41);
            S40 = _mm256_madd_epi16(E40, C4);
            S41 = _mm256_madd_epi16(E41, C4);

            T40 = _mm256_loadu_si256((__m256i*)&src[j - 6]);
            T41 = _mm256_loadu_si256((__m256i*)&src[j + 6]);
            E40 = _mm256_unpacklo_epi16(T40, T41);
            E41 = _mm256_unpackhi_epi16(T40, T41);
            S50 = _mm256_madd_epi16(E40, C5);
            S51 = _mm256_madd_epi16(E41, C5);

            T40 = _mm256_loadu_si256((__m256i*)&src[j - 4]);
            T41 = _mm256_loadu_si256((__m256i*)&src[j + 4]);
            E40 = _mm256_unpacklo_epi16(T40, T41);
            E41 = _mm256_unpackhi_epi16(T40, T41);
            S60 = _mm256_madd_epi16(E40, C6);
            S61 = _mm256_madd_epi16(E41, C6);

            T40 = _mm256_loadu_si256((__m256i*)&src[j - 2]);
            T41 = _mm256_loadu_si256((__m256i*)&src[j + 2]);
            E40 = _mm256_unpacklo_epi16(T40, T41);
            E41 = _mm256_unpackhi_epi16(T40, T41);
            S70 = _mm256_madd_epi16(E40, C7);
            S71 = _mm256_madd_epi16(E41, C7);

            T40 = _mm256_load_si256((__m256i*)&src[j]);
            E40 = _mm256_unpacklo_epi16(T40, zero);
            E41 = _mm256_unpackhi_epi16(T40, zero);
            S80 = _mm256_madd_epi16(E40, C8);
            S81 = _mm256_madd_epi16(E41, C8);

            SS1 = _mm256_add_epi32(S00, S10);
            SS1 = _mm256_add_epi32(SS1, S20);
            SS1 = _mm256_add_epi32(SS1, S30);
            SS1 = _mm256_add_epi32(SS1, S40);
            SS1 = _mm256_add_epi32(SS1, S50);
            SS1 = _mm256_add_epi32(SS1, S60);
            SS1 = _mm256_add_epi32(SS1, S70);
            SS1 = _mm256_add_epi32(SS1, S80);

            SS2 = _mm256_add_epi32(S01, S11);
            SS2 = _mm256_add_epi32(SS2, S21);
            SS2 = _mm256_add_epi32(SS2, S31);
            SS2 = _mm256_add_epi32(SS2, S41);
            SS2 = _mm256_add_epi32(SS2, S51);
            SS2 = _mm256_add_epi32(SS2, S61);
            SS2 = _mm256_add_epi32(SS2, S71);
            SS2 = _mm256_add_epi32(SS2, S81);

            SS1 = _mm256_add_epi32(SS1, mAddOffset);
            SS1 = _mm256_srai_epi32(SS1, 6);

            SS2 = _mm256_add_epi32(SS2, mAddOffset);
            SS2 = _mm256_srai_epi32(SS2, 6);

            S = _mm256_packus_epi32(SS1, SS2);
            S = _mm256_min_epu16(S, max_val);

            _mm256_store_si256((__m256i*)(dst + j), S);

        }

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_alf_one_lcu_one_chroma_avx2(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int bit_depth)
{
    pel *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;

    __m256i T00, T01, T10, T11;
    __m256i E00, E01, E10, E11;
    __m256i C0, C1, C2, C3, C4, C5, C6, C7, C8;
    __m256i S00, S01, S10, S11, S20, S21, S30, S31, S40, S41, S50, S51, S60, S61, SS1, SS2, S70, S71, S80, S81;
    __m256i mAddOffset;
    __m256i zero = _mm256_setzero_si256();
    int max_pixel = (1 << bit_depth) - 1;
    __m256i max_val = _mm256_set1_epi16(max_pixel);
    __m256i uv_mask = _mm256_set1_epi32(0xffff);
    int i, j;
    int startPos = 0;
    int endPos = lcu_height;
    int xPosEnd = lcu_width << 1;

    src += (startPos*i_src);
    dst += (startPos*i_dst);

    C0 = _mm256_set1_epi32(coef[0]);
    C1 = _mm256_set1_epi32(coef[2]);
    C2 = _mm256_set1_epi32(coef[4]);
    C3 = _mm256_set1_epi32(coef[6]);
    C4 = _mm256_set1_epi32(coef[8]);
    C5 = _mm256_set1_epi32(coef[10]);
    C6 = _mm256_set1_epi32(coef[12]);
    C7 = _mm256_set1_epi32(coef[14]);
    C8 = _mm256_set1_epi32(coef[16]);

    mAddOffset = _mm256_set1_epi32(32);

    for (i = startPos; i < endPos; i++) {
        imgPad1 = src + i_src;
        imgPad2 = src - i_src;
        imgPad3 = src + 2 * i_src;
        imgPad4 = src - 2 * i_src;
        imgPad5 = src + 3 * i_src;
        imgPad6 = src - 3 * i_src;
        if (i < 3) {
            if (i == 0) {
                imgPad4 = imgPad2 = src;
            }
            else if (i == 1) {
                imgPad4 = imgPad2;
            }
            imgPad6 = imgPad4;
        }
        else if (i > lcu_height - 4) {
            if (i == lcu_height - 1) {
                imgPad3 = imgPad1 = src;
            }
            else if (i == lcu_height - 2) {
                imgPad3 = imgPad1;
            }
            imgPad5 = imgPad3;
        }

        for (j = 0; j < xPosEnd; j += 32) {
            T00 = _mm256_loadu_si256((__m256i*)&imgPad6[j]);
            T10 = _mm256_loadu_si256((__m256i*)&imgPad6[j + 16]);
            T01 = _mm256_loadu_si256((__m256i*)&imgPad5[j]);
            T11 = _mm256_loadu_si256((__m256i*)&imgPad5[j + 16]);
            E00 = _mm256_and_si256(T00, uv_mask);
            E01 = _mm256_and_si256(T01, uv_mask);
            E10 = _mm256_and_si256(T10, uv_mask);
            E11 = _mm256_and_si256(T11, uv_mask);
            E00 = _mm256_add_epi32(E00, E01);
            E10 = _mm256_add_epi32(E10, E11);
            S00 = _mm256_mullo_epi32(E00, C0);//前8个像素所有C0*P0的结果
            S01 = _mm256_mullo_epi32(E10, C0);//后8个像素所有C0*P0的结果

            T00 = _mm256_loadu_si256((__m256i*)&imgPad4[j]);
            T10 = _mm256_loadu_si256((__m256i*)&imgPad4[j + 16]);
            T01 = _mm256_loadu_si256((__m256i*)&imgPad3[j]);
            T11 = _mm256_loadu_si256((__m256i*)&imgPad3[j + 16]);
            E00 = _mm256_and_si256(T00, uv_mask);
            E01 = _mm256_and_si256(T01, uv_mask);
            E10 = _mm256_and_si256(T10, uv_mask);
            E11 = _mm256_and_si256(T11, uv_mask);
            E00 = _mm256_add_epi32(E00, E01);
            E10 = _mm256_add_epi32(E10, E11);
            S10 = _mm256_mullo_epi32(E00, C1);//前8个像素所有C1*P1的结果
            S11 = _mm256_mullo_epi32(E10, C1);//后8个像素所有C1*P1的结果

            T00 = _mm256_loadu_si256((__m256i*)&imgPad2[j - 2]);
            T10 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 14]);
            T01 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 2]);
            T11 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 18]);
            E00 = _mm256_and_si256(T00, uv_mask);
            E01 = _mm256_and_si256(T01, uv_mask);
            E10 = _mm256_and_si256(T10, uv_mask);
            E11 = _mm256_and_si256(T11, uv_mask);
            E00 = _mm256_add_epi32(E00, E01);
            E10 = _mm256_add_epi32(E10, E11);
            S20 = _mm256_mullo_epi32(E00, C2);
            S21 = _mm256_mullo_epi32(E10, C2);

            T00 = _mm256_loadu_si256((__m256i*)&imgPad2[j]);
            T10 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 16]);
            T01 = _mm256_loadu_si256((__m256i*)&imgPad1[j]);
            T11 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 16]);
            E00 = _mm256_and_si256(T00, uv_mask);
            E01 = _mm256_and_si256(T01, uv_mask);
            E10 = _mm256_and_si256(T10, uv_mask);
            E11 = _mm256_and_si256(T11, uv_mask);
            E00 = _mm256_add_epi32(E00, E01);
            E10 = _mm256_add_epi32(E10, E11);
            S30 = _mm256_mullo_epi32(E00, C3);
            S31 = _mm256_mullo_epi32(E10, C3);

            T00 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 2]);
            T10 = _mm256_loadu_si256((__m256i*)&imgPad2[j + 18]);
            T01 = _mm256_loadu_si256((__m256i*)&imgPad1[j - 2]);
            T11 = _mm256_loadu_si256((__m256i*)&imgPad1[j + 14]);
            E00 = _mm256_and_si256(T00, uv_mask);
            E01 = _mm256_and_si256(T01, uv_mask);
            E10 = _mm256_and_si256(T10, uv_mask);
            E11 = _mm256_and_si256(T11, uv_mask);
            E00 = _mm256_add_epi32(E00, E01);
            E10 = _mm256_add_epi32(E10, E11);
            S40 = _mm256_mullo_epi32(E00, C4);
            S41 = _mm256_mullo_epi32(E10, C4);

            T00 = _mm256_loadu_si256((__m256i*)&src[j - 6]);
            T10 = _mm256_loadu_si256((__m256i*)&src[j + 10]);
            T01 = _mm256_loadu_si256((__m256i*)&src[j + 6]);
            T11 = _mm256_loadu_si256((__m256i*)&src[j + 22]);
            E00 = _mm256_and_si256(T00, uv_mask);
            E01 = _mm256_and_si256(T01, uv_mask);
            E10 = _mm256_and_si256(T10, uv_mask);
            E11 = _mm256_and_si256(T11, uv_mask);
            E00 = _mm256_add_epi32(E00, E01);
            E10 = _mm256_add_epi32(E10, E11);
            S50 = _mm256_mullo_epi32(E00, C5);
            S51 = _mm256_mullo_epi32(E10, C5);

            T00 = _mm256_loadu_si256((__m256i*)&src[j - 4]);
            T10 = _mm256_loadu_si256((__m256i*)&src[j + 12]);
            T01 = _mm256_loadu_si256((__m256i*)&src[j + 4]);
            T11 = _mm256_loadu_si256((__m256i*)&src[j + 20]);
            E00 = _mm256_and_si256(T00, uv_mask);
            E01 = _mm256_and_si256(T01, uv_mask);
            E10 = _mm256_and_si256(T10, uv_mask);
            E11 = _mm256_and_si256(T11, uv_mask);
            E00 = _mm256_add_epi32(E00, E01);
            E10 = _mm256_add_epi32(E10, E11);
            S60 = _mm256_mullo_epi32(E00, C6);
            S61 = _mm256_mullo_epi32(E10, C6);

            T00 = _mm256_loadu_si256((__m256i*)&src[j - 2]);
            T10 = _mm256_loadu_si256((__m256i*)&src[j + 14]);
            T01 = _mm256_loadu_si256((__m256i*)&src[j + 2]);
            T11 = _mm256_loadu_si256((__m256i*)&src[j + 18]);
            E00 = _mm256_and_si256(T00, uv_mask);
            E01 = _mm256_and_si256(T01, uv_mask);
            E10 = _mm256_and_si256(T10, uv_mask);
            E11 = _mm256_and_si256(T11, uv_mask);
            E00 = _mm256_add_epi32(E00, E01);
            E10 = _mm256_add_epi32(E10, E11);
            S70 = _mm256_mullo_epi32(E00, C7);
            S71 = _mm256_mullo_epi32(E10, C7);

            T00 = _mm256_loadu_si256((__m256i*)&src[j]);
            T01 = _mm256_loadu_si256((__m256i*)&src[j + 16]);
            E00 = _mm256_and_si256(T00, uv_mask);
            E01 = _mm256_and_si256(T01, uv_mask);
            S80 = _mm256_madd_epi16(E00, C8);
            S81 = _mm256_madd_epi16(E01, C8);

            SS1 = _mm256_add_epi32(S00, S10);
            SS1 = _mm256_add_epi32(SS1, S20);
            SS1 = _mm256_add_epi32(SS1, S30);
            SS1 = _mm256_add_epi32(SS1, S40);
            SS1 = _mm256_add_epi32(SS1, S50);
            SS1 = _mm256_add_epi32(SS1, S60);
            SS1 = _mm256_add_epi32(SS1, S70);
            SS1 = _mm256_add_epi32(SS1, S80);

            SS2 = _mm256_add_epi32(S01, S11);
            SS2 = _mm256_add_epi32(SS2, S21);
            SS2 = _mm256_add_epi32(SS2, S31);
            SS2 = _mm256_add_epi32(SS2, S41);
            SS2 = _mm256_add_epi32(SS2, S51);
            SS2 = _mm256_add_epi32(SS2, S61);
            SS2 = _mm256_add_epi32(SS2, S71);
            SS2 = _mm256_add_epi32(SS2, S81);

            SS1 = _mm256_add_epi32(SS1, mAddOffset);
            SS1 = _mm256_srai_epi32(SS1, 6);

            SS2 = _mm256_add_epi32(SS2, mAddOffset);
            SS2 = _mm256_srai_epi32(SS2, 6);

            T00 = _mm256_loadu_si256((__m256i*)&dst[j]);
            T01 = _mm256_loadu_si256((__m256i*)&dst[j + 16]);

            S00 = _mm256_packus_epi32(SS1, SS2);
            S00 = _mm256_min_epu16(S00, max_val);
            S01 = _mm256_unpacklo_epi16(S00, zero);
            S10 = _mm256_unpackhi_epi16(S00, zero);

            T00 = _mm256_blendv_epi8(T00, S01, uv_mask);
            T01 = _mm256_blendv_epi8(T01, S10, uv_mask);
            _mm256_storeu_si256((__m256i*)(dst + j), T00);
            _mm256_storeu_si256((__m256i*)(dst + j + 16), T01);
        }

        src += i_src;
        dst += i_dst;
    }
}


#endif