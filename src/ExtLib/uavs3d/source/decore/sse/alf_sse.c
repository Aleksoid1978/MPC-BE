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
void uavs3d_alf_one_lcu_sse(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;

    __m128i T00, T01, T10, T11, T20, T21, T30, T31, T40, T41, T50, T51;
    __m128i T1, T2, T3, T4, T5, T6, T7, T8;
    __m128i E00, E01, E10, E11, E20, E21, E30, E31, E40, E41;
    __m128i C0, C1, C2, C3, C4, C30, C31, C32, C33;
	__m128i S0, S00, S01, S1, S10, S11, S2, S20, S21, S3, S30, S31, S4, S40, S41, S5, S50, S51, S52, S53, S6, S7, S8, SS1, SS2, SS3, S;
    __m128i mSwitch1, mSwitch2, mSwitch3, mSwitch4, mSwitch5;
    __m128i mAddOffset;
    __m128i mZero = _mm_set1_epi16(0);
    __m128i mMax = _mm_set1_epi16((short)((1 << sample_bit_depth) - 1));
	__m128i ones = _mm_set1_epi16(1);

    int i, j;

    C0 = _mm_set1_epi8(coef[0]);
    C1 = _mm_set1_epi8(coef[1]);
    C2 = _mm_set1_epi8(coef[2]);
    C3 = _mm_set1_epi8(coef[3]);
    C4 = _mm_set1_epi8(coef[4]);

    mSwitch1 = _mm_setr_epi8(0, 1, 2, 3, 2, 1, 0, 3, 0, 1, 2, 3, 2, 1, 0, 3);
    C30 = _mm_loadu_si128((__m128i*)&coef[5]);
    C31 = _mm_packs_epi32(C30, C30);
    C32 = _mm_packs_epi16(C31, C31);
    C33 = _mm_shuffle_epi8(C32, mSwitch1);
    mSwitch2 = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, -1, 1, 2, 3, 4, 5, 6, 7, -1);
    mSwitch3 = _mm_setr_epi8(2, 3, 4, 5, 6, 7, 8, -1, 3, 4, 5, 6, 7, 8, 9, -1);
    mSwitch4 = _mm_setr_epi8(4, 5, 6, 7, 8, 9, 10, -1, 5, 6, 7, 8, 9, 10, 11, -1);
    mSwitch5 = _mm_setr_epi8(6, 7, 8, 9, 10, 11, 12, -1, 7, 8, 9, 10, 11, 12, 13, -1);
    mAddOffset = _mm_set1_epi32(32);

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

        for (j = 0; j < lcu_width - 15; j += 16) {
            T00 = _mm_load_si128((__m128i*)&imgPad6[j]);
            T01 = _mm_load_si128((__m128i*)&imgPad5[j]);
            E00 = _mm_unpacklo_epi8(T00, T01);
            E01 = _mm_unpackhi_epi8(T00, T01);
            S00 = _mm_maddubs_epi16(E00, C0);//前8个像素所有C0*P0的结果
            S01 = _mm_maddubs_epi16(E01, C0);//后8个像素所有C0*P0的结果

            T10 = _mm_load_si128((__m128i*)&imgPad4[j]);
            T11 = _mm_load_si128((__m128i*)&imgPad3[j]);
            E10 = _mm_unpacklo_epi8(T10, T11);
            E11 = _mm_unpackhi_epi8(T10, T11);
            S10 = _mm_maddubs_epi16(E10, C1);//前8个像素所有C1*P1的结果
            S11 = _mm_maddubs_epi16(E11, C1);//后8个像素所有C1*P1的结果

            T20 = _mm_loadu_si128((__m128i*)&imgPad2[j - 1]);
            T21 = _mm_loadu_si128((__m128i*)&imgPad1[j + 1]);
            E20 = _mm_unpacklo_epi8(T20, T21);
            E21 = _mm_unpackhi_epi8(T20, T21);
            S20 = _mm_maddubs_epi16(E20, C2);
            S21 = _mm_maddubs_epi16(E21, C2);

            T30 = _mm_load_si128((__m128i*)&imgPad2[j]);
            T31 = _mm_load_si128((__m128i*)&imgPad1[j]);
            E30 = _mm_unpacklo_epi8(T30, T31);
            E31 = _mm_unpackhi_epi8(T30, T31);
            S30 = _mm_maddubs_epi16(E30, C3);
            S31 = _mm_maddubs_epi16(E31, C3);

            T40 = _mm_loadu_si128((__m128i*)&imgPad2[j + 1]);
            T41 = _mm_loadu_si128((__m128i*)&imgPad1[j - 1]);
            E40 = _mm_unpacklo_epi8(T40, T41);
            E41 = _mm_unpackhi_epi8(T40, T41);
            S40 = _mm_maddubs_epi16(E40, C4);
            S41 = _mm_maddubs_epi16(E41, C4);

            T50 = _mm_loadu_si128((__m128i*)&src[j - 3]);
            T51 = _mm_loadu_si128((__m128i*)&src[j + 5]);
            T1 = _mm_shuffle_epi8(T50, mSwitch2);
            T2 = _mm_shuffle_epi8(T50, mSwitch3);
            T3 = _mm_shuffle_epi8(T50, mSwitch4);
            T4 = _mm_shuffle_epi8(T50, mSwitch5);
            T5 = _mm_shuffle_epi8(T51, mSwitch2);
            T6 = _mm_shuffle_epi8(T51, mSwitch3);
            T7 = _mm_shuffle_epi8(T51, mSwitch4);
            T8 = _mm_shuffle_epi8(T51, mSwitch5);

            S5 = _mm_maddubs_epi16(T1, C33);
            S6 = _mm_maddubs_epi16(T2, C33);
            S7 = _mm_maddubs_epi16(T3, C33);
            S8 = _mm_maddubs_epi16(T4, C33);
			S50 = _mm_madd_epi16(S5, ones);
			S51 = _mm_madd_epi16(S6, ones);
			S52 = _mm_madd_epi16(S7, ones);
			S53 = _mm_madd_epi16(S8, ones);
			S5 = _mm_hadd_epi32(S50, S51);
			S6 = _mm_hadd_epi32(S52, S53);//前8个
            S3 = _mm_maddubs_epi16(T5, C33);
            S4 = _mm_maddubs_epi16(T6, C33);
            S7 = _mm_maddubs_epi16(T7, C33);
            S8 = _mm_maddubs_epi16(T8, C33);
			S50 = _mm_madd_epi16(S3, ones);
			S51 = _mm_madd_epi16(S4, ones);
			S52 = _mm_madd_epi16(S7, ones);
			S53 = _mm_madd_epi16(S8, ones);
			S7 = _mm_hadd_epi32(S50, S51);
			S8 = _mm_hadd_epi32(S52, S53);//后8个

			S0 = _mm_cvtepi16_epi32(S00);
			S1 = _mm_cvtepi16_epi32(S10);
			S2 = _mm_cvtepi16_epi32(S20);
			S3 = _mm_cvtepi16_epi32(S30);
			S4 = _mm_cvtepi16_epi32(S40);
			S0 = _mm_add_epi32(S0, S1);
			S2 = _mm_add_epi32(S2, S3);
			S5 = _mm_add_epi32(S4, S5);
			S5 = _mm_add_epi32(S0, S5);
			S5 = _mm_add_epi32(S2, S5);
			S0 = _mm_cvtepi16_epi32(_mm_srli_si128(S00, 8));
			S1 = _mm_cvtepi16_epi32(_mm_srli_si128(S10, 8));
			S2 = _mm_cvtepi16_epi32(_mm_srli_si128(S20, 8));
			S3 = _mm_cvtepi16_epi32(_mm_srli_si128(S30, 8));
			S4 = _mm_cvtepi16_epi32(_mm_srli_si128(S40, 8));
			S0 = _mm_add_epi32(S0, S1);
			S2 = _mm_add_epi32(S2, S3);
			S6 = _mm_add_epi32(S4, S6);
			S6 = _mm_add_epi32(S0, S6);
			S6 = _mm_add_epi32(S2, S6);//前8个
			S0 = _mm_cvtepi16_epi32(S01);
			S1 = _mm_cvtepi16_epi32(S11);
			S2 = _mm_cvtepi16_epi32(S21);
			S3 = _mm_cvtepi16_epi32(S31);
			S4 = _mm_cvtepi16_epi32(S41);
			S0 = _mm_add_epi32(S0, S1);
			S2 = _mm_add_epi32(S2, S3);
			S7 = _mm_add_epi32(S4, S7);
			S7 = _mm_add_epi32(S0, S7);
			S7 = _mm_add_epi32(S2, S7);
			S0 = _mm_cvtepi16_epi32(_mm_srli_si128(S01, 8));
			S1 = _mm_cvtepi16_epi32(_mm_srli_si128(S11, 8));
			S2 = _mm_cvtepi16_epi32(_mm_srli_si128(S21, 8));
			S3 = _mm_cvtepi16_epi32(_mm_srli_si128(S31, 8));
			S4 = _mm_cvtepi16_epi32(_mm_srli_si128(S41, 8));
			S0 = _mm_add_epi32(S0, S1);
			S2 = _mm_add_epi32(S2, S3);
			S8 = _mm_add_epi32(S4, S8);
			S8 = _mm_add_epi32(S0, S8);
			S8 = _mm_add_epi32(S2, S8);//后8个

            SS1 = _mm_add_epi32(S5, mAddOffset);
            SS1 = _mm_srai_epi32(SS1, 6);
			SS2 = _mm_add_epi32(S6, mAddOffset);
			SS2 = _mm_srai_epi32(SS2, 6);
			SS1 = _mm_packus_epi32(SS1, SS2);
            SS1 = _mm_min_epi16(SS1, mMax);
            SS1 = _mm_max_epi16(SS1, mZero);

			SS2 = _mm_add_epi32(S7, mAddOffset);
			SS2 = _mm_srai_epi32(SS2, 6);
			SS3 = _mm_add_epi32(S8, mAddOffset);
			SS3 = _mm_srai_epi32(SS3, 6);
			SS2 = _mm_packus_epi32(SS2, SS3);
			SS2 = _mm_min_epi16(SS2, mMax);
			SS2 = _mm_max_epi16(SS2, mZero);

            S = _mm_packus_epi16(SS1, SS2);
            _mm_store_si128((__m128i*)(dst + j), S);
        }

        src += i_src;
        dst += i_dst;
    }
}

#else

void uavs3d_alf_one_lcu_sse(pel *dst, int i_dst, pel *src, int i_src, int lcu_width, int lcu_height, int *coef, int sample_bit_depth)
{
    pel *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;

    __m128i T00, T01, T10, T11, T20, T21, T30, T31, T40, T41;
    __m128i E00, E01, E10, E11, E20, E21, E30, E31, E40, E41;
    __m128i C0, C1, C2, C3, C4, C5, C6, C7, C8;
    __m128i S00, S01, S10, S11, S20, S21, S30, S31, S40, S41, S50, S51, S60, S61, SS1, SS2, S, S70, S71, S80, S81;
    __m128i mAddOffset;
    __m128i zero = _mm_setzero_si128();
    int max_pixel = (1 << sample_bit_depth) - 1;
    __m128i max_val = _mm_set1_epi16(max_pixel);

    int i, j;
    
    C0 = _mm_set1_epi16((pel)coef[0]);
    C1 = _mm_set1_epi16((pel)coef[1]);
    C2 = _mm_set1_epi16((pel)coef[2]);
    C3 = _mm_set1_epi16((pel)coef[3]);
    C4 = _mm_set1_epi16((pel)coef[4]);
    C5 = _mm_set1_epi16((pel)coef[5]);
    C6 = _mm_set1_epi16((pel)coef[6]);
    C7 = _mm_set1_epi16((pel)coef[7]);
    C8 = _mm_set1_epi16((pel)coef[8]);

    mAddOffset = _mm_set1_epi32(32);

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

        for (j = 0; j < lcu_width; j += 8) {
            T00 = _mm_load_si128((__m128i*)&imgPad6[j]);
            T01 = _mm_load_si128((__m128i*)&imgPad5[j]);
            E00 = _mm_unpacklo_epi16(T00, T01);
            E01 = _mm_unpackhi_epi16(T00, T01);
            S00 = _mm_madd_epi16(E00, C0);
            S01 = _mm_madd_epi16(E01, C0);

            T10 = _mm_load_si128((__m128i*)&imgPad4[j]);
            T11 = _mm_load_si128((__m128i*)&imgPad3[j]);
            E10 = _mm_unpacklo_epi16(T10, T11);
            E11 = _mm_unpackhi_epi16(T10, T11);
            S10 = _mm_madd_epi16(E10, C1);
            S11 = _mm_madd_epi16(E11, C1);

            T20 = _mm_loadu_si128((__m128i*)&imgPad2[j - 1]);
            T21 = _mm_loadu_si128((__m128i*)&imgPad1[j + 1]);
            E20 = _mm_unpacklo_epi16(T20, T21);
            E21 = _mm_unpackhi_epi16(T20, T21);
            S20 = _mm_madd_epi16(E20, C2);
            S21 = _mm_madd_epi16(E21, C2);

            T30 = _mm_load_si128((__m128i*)&imgPad2[j]);
            T31 = _mm_load_si128((__m128i*)&imgPad1[j]);
            E30 = _mm_unpacklo_epi16(T30, T31);
            E31 = _mm_unpackhi_epi16(T30, T31);
            S30 = _mm_madd_epi16(E30, C3);
            S31 = _mm_madd_epi16(E31, C3);

            T40 = _mm_loadu_si128((__m128i*)&imgPad2[j + 1]);
            T41 = _mm_loadu_si128((__m128i*)&imgPad1[j - 1]);
            E40 = _mm_unpacklo_epi16(T40, T41);
            E41 = _mm_unpackhi_epi16(T40, T41);
            S40 = _mm_madd_epi16(E40, C4);
            S41 = _mm_madd_epi16(E41, C4);

            T40 = _mm_loadu_si128((__m128i*)&src[j - 3]);
            T41 = _mm_loadu_si128((__m128i*)&src[j + 3]);
            E40 = _mm_unpacklo_epi16(T40, T41);
            E41 = _mm_unpackhi_epi16(T40, T41);
            S50 = _mm_madd_epi16(E40, C5);
            S51 = _mm_madd_epi16(E41, C5);

            T40 = _mm_loadu_si128((__m128i*)&src[j - 2]);
            T41 = _mm_loadu_si128((__m128i*)&src[j + 2]);
            E40 = _mm_unpacklo_epi16(T40, T41);
            E41 = _mm_unpackhi_epi16(T40, T41);
            S60 = _mm_madd_epi16(E40, C6);
            S61 = _mm_madd_epi16(E41, C6);

            T40 = _mm_loadu_si128((__m128i*)&src[j - 1]);
            T41 = _mm_loadu_si128((__m128i*)&src[j + 1]);
            E40 = _mm_unpacklo_epi16(T40, T41);
            E41 = _mm_unpackhi_epi16(T40, T41);
            S70 = _mm_madd_epi16(E40, C7);
            S71 = _mm_madd_epi16(E41, C7);

            T40 = _mm_load_si128((__m128i*)&src[j]);
            E40 = _mm_unpacklo_epi16(T40, zero);
            E41 = _mm_unpackhi_epi16(T40, zero);
            S80 = _mm_madd_epi16(E40, C8);
            S81 = _mm_madd_epi16(E41, C8);

            SS1 = _mm_add_epi32(S00, S10);
            SS1 = _mm_add_epi32(SS1, S20);
            SS1 = _mm_add_epi32(SS1, S30);
            SS1 = _mm_add_epi32(SS1, S40);
            SS1 = _mm_add_epi32(SS1, S50);
            SS1 = _mm_add_epi32(SS1, S60);
            SS1 = _mm_add_epi32(SS1, S70);
            SS1 = _mm_add_epi32(SS1, S80);

            SS2 = _mm_add_epi32(S01, S11);
            SS2 = _mm_add_epi32(SS2, S21);
            SS2 = _mm_add_epi32(SS2, S31);
            SS2 = _mm_add_epi32(SS2, S41);
            SS2 = _mm_add_epi32(SS2, S51);
            SS2 = _mm_add_epi32(SS2, S61);
            SS2 = _mm_add_epi32(SS2, S71);
            SS2 = _mm_add_epi32(SS2, S81);

            SS1 = _mm_add_epi32(SS1, mAddOffset);
            SS1 = _mm_srai_epi32(SS1, 6);

            SS2 = _mm_add_epi32(SS2, mAddOffset);
            SS2 = _mm_srai_epi32(SS2, 6);

            S = _mm_packus_epi32(SS1, SS2);
            S = _mm_min_epu16(S, max_val);

            _mm_store_si128((__m128i*)(dst + j), S);

        }

        src += i_src;
        dst += i_dst;
    }
}

#endif