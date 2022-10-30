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

void uavs3d_if_cpy_w32_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    int i_src3 = i_src2 + i_src;
    int i_dst3 = i_dst2 + i_dst;
    int i_src4 = i_src << 2;
    int i_dst4 = i_dst << 2;
    __m256i m0, m1, m2, m3;
    while (height > 0) {
        m0 = _mm256_loadu_si256((const __m256i*)(src));
        m1 = _mm256_loadu_si256((const __m256i*)(src + i_src));
        m2 = _mm256_loadu_si256((const __m256i*)(src + i_src2));
        m3 = _mm256_loadu_si256((const __m256i*)(src + i_src3));
        _mm256_storeu_si256((__m256i*)dst, m0);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), m1);
        _mm256_storeu_si256((__m256i*)(dst + i_dst2), m2);
        _mm256_storeu_si256((__m256i*)(dst + i_dst3), m3);
        src += i_src4;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_if_cpy_w64_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    int i_src3 = i_src2 + i_src;
    int i_dst3 = i_dst2 + i_dst;
    int i_src4 = i_src << 2;
    int i_dst4 = i_dst << 2;
    __m256i m0, m1, m2, m3, m4, m5, m6, m7;
    while (height) {
        m0 = _mm256_loadu_si256((const __m256i*)(src));
        m1 = _mm256_loadu_si256((const __m256i*)(src + 32));
        m2 = _mm256_loadu_si256((const __m256i*)(src + i_src));
        m3 = _mm256_loadu_si256((const __m256i*)(src + i_src + 32));
        m4 = _mm256_loadu_si256((const __m256i*)(src + i_src2));
        m5 = _mm256_loadu_si256((const __m256i*)(src + i_src2 + 32));
        m6 = _mm256_loadu_si256((const __m256i*)(src + i_src3));
        m7 = _mm256_loadu_si256((const __m256i*)(src + i_src3 + 32));

        _mm256_storeu_si256((__m256i*)(dst), m0);
        _mm256_storeu_si256((__m256i*)(dst + 32), m1);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), m2);
        _mm256_storeu_si256((__m256i*)(dst + i_dst + 32), m3);
        _mm256_storeu_si256((__m256i*)(dst + i_dst2), m4);
        _mm256_storeu_si256((__m256i*)(dst + i_dst2 + 32), m5);
        _mm256_storeu_si256((__m256i*)(dst + i_dst3), m6);
        _mm256_storeu_si256((__m256i*)(dst + i_dst3 + 32), m7);

        height -= 4;
        src += i_src4;
        dst += i_dst4;
    }
}

void uavs3d_if_cpy_w128_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    __m256i m0, m1, m2, m3, m4, m5, m6, m7;
    while (height) {
        m0 = _mm256_loadu_si256((const __m256i*)(src));
        m1 = _mm256_loadu_si256((const __m256i*)(src + 32));
        m2 = _mm256_loadu_si256((const __m256i*)(src + 64));
        m3 = _mm256_loadu_si256((const __m256i*)(src + 96));
        m4 = _mm256_loadu_si256((const __m256i*)(src + i_src));
        m5 = _mm256_loadu_si256((const __m256i*)(src + i_src + 32));
        m6 = _mm256_loadu_si256((const __m256i*)(src + i_src + 64));
        m7 = _mm256_loadu_si256((const __m256i*)(src + i_src + 96));

        _mm256_storeu_si256((__m256i*)(dst), m0);
        _mm256_storeu_si256((__m256i*)(dst + 32), m1);
        _mm256_storeu_si256((__m256i*)(dst + 64), m2);
        _mm256_storeu_si256((__m256i*)(dst + 96), m3);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), m4);
        _mm256_storeu_si256((__m256i*)(dst + i_dst + 32), m5);
        _mm256_storeu_si256((__m256i*)(dst + i_dst + 64), m6);
        _mm256_storeu_si256((__m256i*)(dst + i_dst + 96), m7);

        height -= 2;
        src += i_src2;
        dst += i_dst2;
    }
}

void uavs3d_if_hor_chroma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;

    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coeff);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i mSwitch0 = _mm256_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
    __m256i mSwitch1 = _mm256_setr_epi8(0+4, 2+4, 1+4, 3+4, 2+4, 4+4, 3+4, 5+4, 4+4, 6+4, 5+4, 7+4, 6+4, 8+4, 7+4, 9+4,
                                        0+4, 2+4, 1+4, 3+4, 2+4, 4+4, 3+4, 5+4, 4+4, 6+4, 5+4, 7+4, 6+4, 8+4, 7+4, 9+4);
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i T0, T1, S0, R0, R1, sum;
    __m128i s0, s1;

    src -= 2;

    while (height > 0) {
        s0 = _mm_loadu_si128((__m128i*)(src));
        s1 = _mm_loadu_si128((__m128i*)(src + i_src));
        src += i_src << 1;
        uavs3d_prefetch(src, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        S0 = _mm256_set_m128i(s1, s0);

        R0 = _mm256_shuffle_epi8(S0, mSwitch0);      // 4 rows s0 and s1 
        R1 = _mm256_shuffle_epi8(S0, mSwitch1);

        T0 = _mm256_maddubs_epi16(R0, mCoefy1_hor); // 4x4: s0*c0 + s1*c1 
        T1 = _mm256_maddubs_epi16(R1, mCoefy2_hor);
        sum = _mm256_add_epi16(T0, T1);

        sum = _mm256_add_epi16(sum, mAddOffset);
        sum = _mm256_srai_epi16(sum, shift);

        s0 = _mm_packus_epi16(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
        _mm_storel_epi64((__m128i*)(dst), s0);
        _mm_storeh_pi((__m64*)(dst + i_dst), _mm_castsi128_ps(s0));

        height -= 2;
        dst += i_dst << 1;
    }
}

void uavs3d_if_hor_chroma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;

    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coeff);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i mSwitch1 = _mm256_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
    __m256i mSwitch2 = _mm256_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13, 4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i T0, T1, T2, T3, S0, S1, S2, S3, R0, R1, R2, R3, sum0, sum1;

    src -= 2;

    while (height) {
        S0 = _mm256_loadu_si256((__m256i*)(src));
        S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
        src += i_src << 1;
        uavs3d_prefetch(src, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        S2 = _mm256_permute4x64_epi64(S0, 0x94);
        S3 = _mm256_permute4x64_epi64(S1, 0x94);
        R0 = _mm256_shuffle_epi8(S2, mSwitch1);
        R1 = _mm256_shuffle_epi8(S2, mSwitch2);
        R2 = _mm256_shuffle_epi8(S3, mSwitch1);
        R3 = _mm256_shuffle_epi8(S3, mSwitch2);
        T0 = _mm256_maddubs_epi16(R0, mCoefy1_hor);
        T1 = _mm256_maddubs_epi16(R1, mCoefy2_hor);
        T2 = _mm256_maddubs_epi16(R2, mCoefy1_hor);
        T3 = _mm256_maddubs_epi16(R3, mCoefy2_hor);
        sum0 = _mm256_add_epi16(T0, T1);
        sum1 = _mm256_add_epi16(T2, T3);

        height -= 2;

        sum0 = _mm256_add_epi16(sum0, mAddOffset);
        sum1 = _mm256_add_epi16(sum1, mAddOffset);
        sum0 = _mm256_srai_epi16(sum0, shift);
        sum1 = _mm256_srai_epi16(sum1, shift);
        _mm_storeu_si128((__m128i*)(dst), _mm_packus_epi16(_mm256_castsi256_si128(sum0), _mm256_extracti128_si256(sum0, 1)));
        _mm_storeu_si128((__m128i*)(dst + i_dst), _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1)));

        dst += i_dst << 1;
    }
}

void uavs3d_if_hor_chroma_w32_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;

    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coeff);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i mSwitch1 = _mm256_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
    __m256i mSwitch2 = _mm256_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13, 4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i T0, T1, T2, T3, S0, S1, S2, S3, R0, R1, R2, R3, sum0, sum1;

    src -= 2;

    while (height--) {
        S0 = _mm256_loadu_si256((__m256i*)(src));
        S1 = _mm256_loadu_si256((__m256i*)(src + 16));
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        S2 = _mm256_permute4x64_epi64(S0, 0x94);
        S3 = _mm256_permute4x64_epi64(S1, 0x94);
        R0 = _mm256_shuffle_epi8(S2, mSwitch1);
        R1 = _mm256_shuffle_epi8(S2, mSwitch2);
        R2 = _mm256_shuffle_epi8(S3, mSwitch1);
        R3 = _mm256_shuffle_epi8(S3, mSwitch2);
        T0 = _mm256_maddubs_epi16(R0, mCoefy1_hor);
        T1 = _mm256_maddubs_epi16(R1, mCoefy2_hor);
        T2 = _mm256_maddubs_epi16(R2, mCoefy1_hor);
        T3 = _mm256_maddubs_epi16(R3, mCoefy2_hor);
        sum0 = _mm256_add_epi16(T0, T1);
        sum1 = _mm256_add_epi16(T2, T3);

        sum0 = _mm256_add_epi16(sum0, mAddOffset);
        sum1 = _mm256_add_epi16(sum1, mAddOffset);
        sum0 = _mm256_srai_epi16(sum0, shift);
        sum1 = _mm256_srai_epi16(sum1, shift);

        _mm_storeu_si128((__m128i*)(dst), _mm_packus_epi16(_mm256_castsi256_si128(sum0), _mm256_extracti128_si256(sum0, 1)));
        _mm_storeu_si128((__m128i*)(dst + 16), _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1)));

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_chroma_w32x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
	int col;
	const int offset = 32;
	const int shift = 6;

	__m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coeff);
	__m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coeff + 2));
	__m256i mSwitch1 = _mm256_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
	__m256i mSwitch2 = _mm256_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13, 4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);
	__m256i mAddOffset = _mm256_set1_epi16(offset);
	__m256i T0, T1, T2, T3, S0, S1, S2, S3, R0, R1, R2, R3, sum0, sum1;

	src -= 2;

	while (height--) {
	    uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
		for (col = 0; col < width; col += 32) {
			S0 = _mm256_loadu_si256((__m256i*)(src + col));
            S1 = _mm256_loadu_si256((__m256i*)(src + col + 16));
			S2 = _mm256_permute4x64_epi64(S0, 0x94);
            S3 = _mm256_permute4x64_epi64(S1, 0x94);
			R0 = _mm256_shuffle_epi8(S2, mSwitch1);
			R1 = _mm256_shuffle_epi8(S2, mSwitch2);
            R2 = _mm256_shuffle_epi8(S3, mSwitch1);
            R3 = _mm256_shuffle_epi8(S3, mSwitch2);
			T0 = _mm256_maddubs_epi16(R0, mCoefy1_hor);
			T1 = _mm256_maddubs_epi16(R1, mCoefy2_hor);
            T2 = _mm256_maddubs_epi16(R2, mCoefy1_hor);
            T3 = _mm256_maddubs_epi16(R3, mCoefy2_hor);
			sum0 = _mm256_add_epi16(T0, T1);
            sum1 = _mm256_add_epi16(T2, T3);

			sum0 = _mm256_add_epi16(sum0, mAddOffset);
            sum1 = _mm256_add_epi16(sum1, mAddOffset);
			sum0 = _mm256_srai_epi16(sum0, shift);
            sum1 = _mm256_srai_epi16(sum1, shift);
			_mm_storeu_si128((__m128i*)(dst + col), _mm_packus_epi16(_mm256_castsi256_si128(sum0), _mm256_extracti128_si256(sum0, 1)));
            _mm_storeu_si128((__m128i*)(dst + col + 16), _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1)));
		}
		src += i_src;
		dst += i_dst;
	}
}

void uavs3d_if_hor_luma_w4_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i mSwitch1 = _mm256_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6, 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
    __m256i mSwitch2 = _mm256_setr_epi8(4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10, 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10);
    __m256i T0, T1, T2, T3, S0, S1, sum;
    __m256i r0, r1, r2, r3;
    __m128i s0, s1, s2, s3;
    __m256i mCoefy1_hor = _mm256_set1_epi32(*(s32*)coeff);
    __m256i mCoefy2_hor = _mm256_set1_epi32(*(s32*)(coeff + 4));
    src -= 3;

    while (height > 0) {
        s0 = _mm_loadu_si128((__m128i*)(src));
        s1 = _mm_loadu_si128((__m128i*)(src + i_src));
        s2 = _mm_loadu_si128((__m128i*)(src + i_src * 2));
        s3 = _mm_loadu_si128((__m128i*)(src + i_src * 3));
        src += i_src << 2;
        uavs3d_prefetch(src, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        S0 = _mm256_set_m128i(s2, s0);
        S1 = _mm256_set_m128i(s3, s1);

        r0 = _mm256_shuffle_epi8(S0, mSwitch1);
        r1 = _mm256_shuffle_epi8(S0, mSwitch2);
        r2 = _mm256_shuffle_epi8(S1, mSwitch1);
        r3 = _mm256_shuffle_epi8(S1, mSwitch2);

        T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
        T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
        T2 = _mm256_maddubs_epi16(r2, mCoefy1_hor);
        T3 = _mm256_maddubs_epi16(r3, mCoefy2_hor);

        T0 = _mm256_add_epi16(T0, T1);
        T1 = _mm256_add_epi16(T2, T3);
        sum = _mm256_hadd_epi16(T0, T1);

        sum = _mm256_add_epi16(sum, mAddOffset);
        sum = _mm256_srai_epi16(sum, shift);

        s0 = _mm_packus_epi16(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));

        height -= 4;
        M32(dst) = _mm_extract_epi32(s0, 0);
        M32(dst + i_dst) = _mm_extract_epi32(s0, 1);
        M32(dst + i_dst * 2) = _mm_extract_epi32(s0, 2);
        M32(dst + i_dst * 3) = _mm_extract_epi32(s0, 3);

        dst += i_dst << 2;
    }
}

void uavs3d_if_hor_luma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i mSwitch1 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
    __m256i mSwitch2 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
    __m256i mSwitch3 = _mm256_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
    __m256i mSwitch4 = _mm256_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);
    __m256i T0, T1, T2, T3, S, sum;
    __m256i r0, r1, r2, r3;
    __m128i s0, s1;
    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coeff);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i mCoefy3_hor = _mm256_set1_epi16(*(s16*)(coeff + 4));
    __m256i mCoefy4_hor = _mm256_set1_epi16(*(s16*)(coeff + 6));

    src -= 3;

    while (height) {
        s0 = _mm_loadu_si128((__m128i*)(src));
        s1 = _mm_loadu_si128((__m128i*)(src + i_src));
        src += i_src << 1;
        uavs3d_prefetch(src, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        S = _mm256_set_m128i(s1, s0);

        r0 = _mm256_shuffle_epi8(S, mSwitch1);
        r1 = _mm256_shuffle_epi8(S, mSwitch2);
        r2 = _mm256_shuffle_epi8(S, mSwitch3);
        r3 = _mm256_shuffle_epi8(S, mSwitch4);

        T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
        T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
        T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
        T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

        T0 = _mm256_add_epi16(T0, T1);
        T1 = _mm256_add_epi16(T2, T3);
        sum = _mm256_add_epi16(T0, T1);

        sum = _mm256_add_epi16(sum, mAddOffset);
        sum = _mm256_srai_epi16(sum, shift);

        height -= 2;
        s0 = _mm_packus_epi16(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
        _mm_storel_epi64((__m128i*)(dst), s0);
        _mm_storeh_pi((__m64*)(dst + i_dst), _mm_castsi128_ps(s0));

        dst += i_dst << 1;
    }
}

void uavs3d_if_hor_luma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
	const int offset = 32;
	const int shift = 6;
	__m256i mAddOffset = _mm256_set1_epi16(offset);
	__m256i mSwitch1 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
	__m256i mSwitch2 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
	__m256i mSwitch3 = _mm256_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
	__m256i mSwitch4 = _mm256_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);
	__m256i T0, T1, S0, S1, S2, S3, sum0, sum1, T2, T3;
	__m256i r0, r1, r2, r3;
	__m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coeff);
	__m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coeff + 2));
	__m256i mCoefy3_hor = _mm256_set1_epi16(*(s16*)(coeff + 4));
	__m256i mCoefy4_hor = _mm256_set1_epi16(*(s16*)(coeff + 6));

    src -= 3;

	while (height) {
		S0 = _mm256_loadu_si256((__m256i*)(src));
        S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
		S2 = _mm256_permute4x64_epi64(S0, 0x94);
        S3 = _mm256_permute4x64_epi64(S1, 0x94);
        src += i_src << 1;
        uavs3d_prefetch(src, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

		r0 = _mm256_shuffle_epi8(S2, mSwitch1);
		r1 = _mm256_shuffle_epi8(S2, mSwitch2);
		r2 = _mm256_shuffle_epi8(S2, mSwitch3);
		r3 = _mm256_shuffle_epi8(S2, mSwitch4);

		T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
		T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
		T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
		T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

        r0 = _mm256_shuffle_epi8(S3, mSwitch1);
        r1 = _mm256_shuffle_epi8(S3, mSwitch2);
        r2 = _mm256_shuffle_epi8(S3, mSwitch3);
        r3 = _mm256_shuffle_epi8(S3, mSwitch4);

        r0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
        r1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
        r2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
        r3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

		T0 = _mm256_add_epi16(T0, T1);
		T1 = _mm256_add_epi16(T2, T3);
        r0 = _mm256_add_epi16(r0, r1);
        r1 = _mm256_add_epi16(r2, r3);
		sum0 = _mm256_add_epi16(T0, T1);
        sum1 = _mm256_add_epi16(r0, r1);

		sum0 = _mm256_add_epi16(sum0, mAddOffset);
        sum1 = _mm256_add_epi16(sum1, mAddOffset);
		sum0 = _mm256_srai_epi16(sum0, shift);
        sum1 = _mm256_srai_epi16(sum1, shift);

        height -= 2;
		_mm_storeu_si128((__m128i*)(dst), _mm_packus_epi16(_mm256_castsi256_si128(sum0), _mm256_extracti128_si256(sum0, 1)));
        _mm_storeu_si128((__m128i*)(dst + i_dst), _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1)));

		dst += i_dst << 1;
	}
}

void uavs3d_if_hor_luma_w32_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i mSwitch1 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
    __m256i mSwitch2 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
    __m256i mSwitch3 = _mm256_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
    __m256i mSwitch4 = _mm256_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);
    __m256i T0, T1, S0, S1, S2, S3, sum0, sum1, T2, T3;
    __m256i r0, r1, r2, r3;
    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coeff);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i mCoefy3_hor = _mm256_set1_epi16(*(s16*)(coeff + 4));
    __m256i mCoefy4_hor = _mm256_set1_epi16(*(s16*)(coeff + 6));

    src -= 3;

    while (height--) {
        S0 = _mm256_loadu_si256((__m256i*)(src));
        S1 = _mm256_loadu_si256((__m256i*)(src + 16));
        S2 = _mm256_permute4x64_epi64(S0, 0x94);
        S3 = _mm256_permute4x64_epi64(S1, 0x94);

        src += i_src;
        uavs3d_prefetch(src, _MM_HINT_NTA);
        
        r0 = _mm256_shuffle_epi8(S2, mSwitch1);
        r1 = _mm256_shuffle_epi8(S2, mSwitch2);
        r2 = _mm256_shuffle_epi8(S2, mSwitch3);
        r3 = _mm256_shuffle_epi8(S2, mSwitch4);

        T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
        T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
        T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
        T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

        r0 = _mm256_shuffle_epi8(S3, mSwitch1);
        r1 = _mm256_shuffle_epi8(S3, mSwitch2);
        r2 = _mm256_shuffle_epi8(S3, mSwitch3);
        r3 = _mm256_shuffle_epi8(S3, mSwitch4);

        r0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
        r1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
        r2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
        r3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

        T0 = _mm256_add_epi16(T0, T1);
        T1 = _mm256_add_epi16(T2, T3);
        r0 = _mm256_add_epi16(r0, r1);
        r1 = _mm256_add_epi16(r2, r3);
        sum0 = _mm256_add_epi16(T0, T1);
        sum1 = _mm256_add_epi16(r0, r1);

        sum0 = _mm256_add_epi16(sum0, mAddOffset);
        sum1 = _mm256_add_epi16(sum1, mAddOffset);
        sum0 = _mm256_srai_epi16(sum0, shift);
        sum1 = _mm256_srai_epi16(sum1, shift);

        _mm_storeu_si128((__m128i*)(dst), _mm_packus_epi16(_mm256_castsi256_si128(sum0), _mm256_extracti128_si256(sum0, 1)));
        _mm_storeu_si128((__m128i*)(dst + 16), _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1)));
    
        dst += i_dst;
    }
}

void uavs3d_if_hor_luma_w32x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int col;
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i mSwitch1 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
    __m256i mSwitch2 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
    __m256i mSwitch3 = _mm256_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
    __m256i mSwitch4 = _mm256_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);
    __m256i T0, T1, S0, S1, S2, S3, sum0, sum1, T2, T3;
    __m256i r0, r1, r2, r3;
    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coeff);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i mCoefy3_hor = _mm256_set1_epi16(*(s16*)(coeff + 4));
    __m256i mCoefy4_hor = _mm256_set1_epi16(*(s16*)(coeff + 6));

    src -= 3;

    while (height--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 32)
        {
            S0 = _mm256_loadu_si256((__m256i*)(src + col));
            S1 = _mm256_loadu_si256((__m256i*)(src + col + 16));
            S2 = _mm256_permute4x64_epi64(S0, 0x94);
            S3 = _mm256_permute4x64_epi64(S1, 0x94);

            r0 = _mm256_shuffle_epi8(S2, mSwitch1);
            r1 = _mm256_shuffle_epi8(S2, mSwitch2);
            r2 = _mm256_shuffle_epi8(S2, mSwitch3);
            r3 = _mm256_shuffle_epi8(S2, mSwitch4);

            T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
            T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
            T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
            T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

            r0 = _mm256_shuffle_epi8(S3, mSwitch1);
            r1 = _mm256_shuffle_epi8(S3, mSwitch2);
            r2 = _mm256_shuffle_epi8(S3, mSwitch3);
            r3 = _mm256_shuffle_epi8(S3, mSwitch4);

            r0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
            r1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
            r2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
            r3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

            T0 = _mm256_add_epi16(T0, T1);
            T1 = _mm256_add_epi16(T2, T3);
            r0 = _mm256_add_epi16(r0, r1);
            r1 = _mm256_add_epi16(r2, r3);
            sum0 = _mm256_add_epi16(T0, T1);
            sum1 = _mm256_add_epi16(r0, r1);

            sum0 = _mm256_add_epi16(sum0, mAddOffset);
            sum1 = _mm256_add_epi16(sum1, mAddOffset);
            sum0 = _mm256_srai_epi16(sum0, shift); 
            sum1 = _mm256_srai_epi16(sum1, shift);

            _mm_storeu_si128((__m128i*)(dst + col), _mm_packus_epi16(_mm256_castsi256_si128(sum0), _mm256_extracti128_si256(sum0, 1)));
            _mm_storeu_si128((__m128i*)(dst + col + 16), _mm_packus_epi16(_mm256_castsi256_si128(sum1), _mm256_extracti128_si256(sum1, 1)));
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_chroma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    const int i_src2 = i_src * 2;
    const int i_src3 = i_src * 3;
    const int i_src4 = i_src * 4;
    __m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
    __m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i T0, T1, mVal;
    __m256i S0, S1, S2, S3;
    __m128i s0, s1, s2, s3, s4;

    src -= i_src;

    while (height) {
        s0 = _mm_loadl_epi64((__m128i*)(src));
        s1 = _mm_loadl_epi64((__m128i*)(src + i_src));
        s2 = _mm_loadl_epi64((__m128i*)(src + i_src2));
        s3 = _mm_loadl_epi64((__m128i*)(src + i_src3));
        s4 = _mm_loadl_epi64((__m128i*)(src + i_src4));

        src += 2 * i_src;
        height -= 2;
        uavs3d_prefetch(src + i_src3, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src4, _MM_HINT_NTA);

        S0 = _mm256_set_m128i(s1, s0);
        S1 = _mm256_set_m128i(s2, s1);
        S2 = _mm256_set_m128i(s3, s2);
        S3 = _mm256_set_m128i(s4, s3);

        S0 = _mm256_unpacklo_epi8(S0, S1);
        S2 = _mm256_unpacklo_epi8(S2, S3);

        T0 = _mm256_maddubs_epi16(S0, coeff0);
        T1 = _mm256_maddubs_epi16(S2, coeff1);

        mVal = _mm256_add_epi16(T0, T1);

        mVal = _mm256_add_epi16(mVal, mAddOffset);
        mVal = _mm256_srai_epi16(mVal, shift);
        s0 = _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1));

        _mm_storel_epi64((__m128i*)(dst), s0);
        _mm_storeh_pi((__m64*)(dst + i_dst), _mm_castsi128_ps(s0));

        dst += 2 * i_dst;
    }
}

void uavs3d_if_ver_chroma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    const int i_src2 = i_src * 2;
    const int i_src3 = i_src * 3;
    const int i_src4 = i_src * 4;
    __m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
    __m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i T0, T1, T2, T3, mVal0, mVal1;
    __m128i s0, s1, s2, s3, s4;
    __m256i S0, S1, S2, S3;

    src -= i_src;

    while (height) {
        s0 = _mm_loadu_si128((__m128i*)(src));
        s1 = _mm_loadu_si128((__m128i*)(src + i_src));
        s2 = _mm_loadu_si128((__m128i*)(src + i_src2));
        s3 = _mm_loadu_si128((__m128i*)(src + i_src3));
        s4 = _mm_loadu_si128((__m128i*)(src + i_src4));

        src += 2 * i_src;
        uavs3d_prefetch(src + i_src3, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src4, _MM_HINT_NTA);
        height -= 2;

        S0 = _mm256_set_m128i(s1, s0);
        S1 = _mm256_set_m128i(s2, s1);
        S2 = _mm256_set_m128i(s3, s2);
        S3 = _mm256_set_m128i(s4, s3);

        T0 = _mm256_unpacklo_epi8(S0, S1);
        T1 = _mm256_unpackhi_epi8(S0, S1);
        T2 = _mm256_unpacklo_epi8(S2, S3);
        T3 = _mm256_unpackhi_epi8(S2, S3);

        T0 = _mm256_maddubs_epi16(T0, coeff0);
        T1 = _mm256_maddubs_epi16(T1, coeff0);
        T2 = _mm256_maddubs_epi16(T2, coeff1);
        T3 = _mm256_maddubs_epi16(T3, coeff1);

        mVal0 = _mm256_add_epi16(T0, T2);
        mVal1 = _mm256_add_epi16(T1, T3);

        mVal0 = _mm256_add_epi16(mVal0, mAddOffset);
        mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
        mVal0 = _mm256_srai_epi16(mVal0, shift);
        mVal1 = _mm256_srai_epi16(mVal1, shift);
        mVal0 = _mm256_packus_epi16(mVal0, mVal1);
        
        _mm_storeu_si128((__m128i*)dst, _mm256_castsi256_si128(mVal0));
        _mm_storeu_si128((__m128i*)(dst + i_dst), _mm256_extracti128_si256(mVal0, 1));

        dst += 2 * i_dst;
    }
}

void uavs3d_if_ver_chroma_w32_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
	const int offset = 32;
	const int shift = 6;
	__m256i mAddOffset = _mm256_set1_epi16(offset);
	const int i_src2 = i_src * 2;
	const int i_src3 = i_src * 3;
	const int i_src4 = i_src * 4;
	__m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
	__m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i T0, T1, T2, T3, T4, T5, T6, T7, mVal0, mVal1, mVal2, mVal3;
    __m256i S0, S1, S2, S3, S4;

	src -= i_src;

	while (height) {
		S0 = _mm256_loadu_si256((__m256i*)(src));
		S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
		S2 = _mm256_loadu_si256((__m256i*)(src + i_src2));
		S3 = _mm256_loadu_si256((__m256i*)(src + i_src3));
		S4 = _mm256_loadu_si256((__m256i*)(src + i_src4));

        src += 2 * i_src;
        height -= 2;
        uavs3d_prefetch(src + i_src3, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src4, _MM_HINT_NTA);

		T0 = _mm256_unpacklo_epi8(S0, S1);
		T1 = _mm256_unpackhi_epi8(S0, S1);
		T2 = _mm256_unpacklo_epi8(S2, S3);
		T3 = _mm256_unpackhi_epi8(S2, S3);
        T4 = _mm256_unpacklo_epi8(S1, S2);
        T5 = _mm256_unpackhi_epi8(S1, S2);
        T6 = _mm256_unpacklo_epi8(S3, S4);
        T7 = _mm256_unpackhi_epi8(S3, S4);

		T0 = _mm256_maddubs_epi16(T0, coeff0);
		T1 = _mm256_maddubs_epi16(T1, coeff0);
		T2 = _mm256_maddubs_epi16(T2, coeff1);
		T3 = _mm256_maddubs_epi16(T3, coeff1);
        T4 = _mm256_maddubs_epi16(T4, coeff0);
        T5 = _mm256_maddubs_epi16(T5, coeff0);
        T6 = _mm256_maddubs_epi16(T6, coeff1);
        T7 = _mm256_maddubs_epi16(T7, coeff1);

		mVal0 = _mm256_add_epi16(T0, T2);
		mVal1 = _mm256_add_epi16(T1, T3);
        mVal2 = _mm256_add_epi16(T4, T6);
        mVal3 = _mm256_add_epi16(T5, T7);

        mVal0 = _mm256_add_epi16(mVal0, mAddOffset);
        mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
        mVal3 = _mm256_add_epi16(mVal3, mAddOffset);
        mVal0 = _mm256_srai_epi16(mVal0, shift);
        mVal1 = _mm256_srai_epi16(mVal1, shift);
        mVal2 = _mm256_srai_epi16(mVal2, shift);
        mVal3 = _mm256_srai_epi16(mVal3, shift);
        mVal0 = _mm256_packus_epi16(mVal0, mVal1);
        mVal2 = _mm256_packus_epi16(mVal2, mVal3);

        _mm256_storeu_si256((__m256i*)dst, mVal0);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), mVal2);

		dst += 2 * i_dst;

	}
}

void uavs3d_if_ver_chroma_w64_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
	const int offset = 32;
	const int shift = 6;
	__m256i mAddOffset = _mm256_set1_epi16(offset);
	const int i_src2 = i_src * 2;
	const int i_src3 = i_src * 3;
	__m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
	__m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i S0, S1, S2, S3, S4, S5, S6, S7;
    __m256i T0, T1, T2, T3, T4, T5, T6, T7, mVal0, mVal1, mVal2, mVal3;

	src -= i_src;

	while (height--){
        S0 = _mm256_loadu_si256((__m256i*)(src));
        S4 = _mm256_loadu_si256((__m256i*)(src + 32));
        S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
        S5 = _mm256_loadu_si256((__m256i*)(src + i_src + 32));
        S2 = _mm256_loadu_si256((__m256i*)(src + i_src2));
        S6 = _mm256_loadu_si256((__m256i*)(src + i_src2 + 32));
        S3 = _mm256_loadu_si256((__m256i*)(src + i_src3));
        S7 = _mm256_loadu_si256((__m256i*)(src + i_src3 + 32));

		src += i_src;
		T0 = _mm256_unpacklo_epi8(S0, S1);
		T1 = _mm256_unpacklo_epi8(S2, S3);
		T2 = _mm256_unpackhi_epi8(S0, S1);
		T3 = _mm256_unpackhi_epi8(S2, S3);
        T4 = _mm256_unpacklo_epi8(S4, S5);
        T5 = _mm256_unpacklo_epi8(S6, S7);
        T6 = _mm256_unpackhi_epi8(S4, S5);
        T7 = _mm256_unpackhi_epi8(S6, S7);

        uavs3d_prefetch(src + i_src3, _MM_HINT_NTA);

		T0 = _mm256_maddubs_epi16(T0, coeff0);
		T1 = _mm256_maddubs_epi16(T1, coeff1);
		T2 = _mm256_maddubs_epi16(T2, coeff0);
		T3 = _mm256_maddubs_epi16(T3, coeff1);
        T4 = _mm256_maddubs_epi16(T4, coeff0);
        T5 = _mm256_maddubs_epi16(T5, coeff1);
        T6 = _mm256_maddubs_epi16(T6, coeff0);
        T7 = _mm256_maddubs_epi16(T7, coeff1);

        mVal0 = _mm256_add_epi16(T0, T1);
        mVal1 = _mm256_add_epi16(T2, T3);
        mVal2 = _mm256_add_epi16(T4, T5);
        mVal3 = _mm256_add_epi16(T6, T7);

        mVal0 = _mm256_add_epi16(mVal0, mAddOffset);
        mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
        mVal3 = _mm256_add_epi16(mVal3, mAddOffset);
        mVal0 = _mm256_srai_epi16(mVal0, shift);
        mVal1 = _mm256_srai_epi16(mVal1, shift);
        mVal2 = _mm256_srai_epi16(mVal2, shift);
        mVal3 = _mm256_srai_epi16(mVal3, shift);
        mVal0 = _mm256_packus_epi16(mVal0, mVal1);
        mVal1 = _mm256_packus_epi16(mVal2, mVal3);

        _mm256_storeu_si256((__m256i*)(dst), mVal0);
        _mm256_storeu_si256((__m256i*)(dst + 32), mVal1);

		dst += i_dst;
	}
}

void uavs3d_if_ver_chroma_w128_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    const int i_src2 = i_src * 2;
    const int i_src3 = i_src * 3;
    __m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
    __m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i S0, S1, S2, S3, S4, S5, S6, S7;
    __m256i T0, T1, T2, T3, T4, T5, T6, T7, mVal0, mVal1, mVal2, mVal3;

    src -= i_src;

    while (height--) {
        S0 = _mm256_loadu_si256((__m256i*)(src));
        S4 = _mm256_loadu_si256((__m256i*)(src + 32));
        S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
        S5 = _mm256_loadu_si256((__m256i*)(src + i_src + 32));
        S2 = _mm256_loadu_si256((__m256i*)(src + i_src2));
        S6 = _mm256_loadu_si256((__m256i*)(src + i_src2 + 32));
        S3 = _mm256_loadu_si256((__m256i*)(src + i_src3));
        S7 = _mm256_loadu_si256((__m256i*)(src + i_src3 + 32));

        T0 = _mm256_unpacklo_epi8(S0, S1);
        T1 = _mm256_unpacklo_epi8(S2, S3);
        T2 = _mm256_unpackhi_epi8(S0, S1);
        T3 = _mm256_unpackhi_epi8(S2, S3);
        T4 = _mm256_unpacklo_epi8(S4, S5);
        T5 = _mm256_unpacklo_epi8(S6, S7);
        T6 = _mm256_unpackhi_epi8(S4, S5);
        T7 = _mm256_unpackhi_epi8(S6, S7);

        T0 = _mm256_maddubs_epi16(T0, coeff0);
        T1 = _mm256_maddubs_epi16(T1, coeff1);
        T2 = _mm256_maddubs_epi16(T2, coeff0);
        T3 = _mm256_maddubs_epi16(T3, coeff1);
        T4 = _mm256_maddubs_epi16(T4, coeff0);
        T5 = _mm256_maddubs_epi16(T5, coeff1);
        T6 = _mm256_maddubs_epi16(T6, coeff0);
        T7 = _mm256_maddubs_epi16(T7, coeff1);

        mVal0 = _mm256_add_epi16(T0, T1);
        mVal1 = _mm256_add_epi16(T2, T3);
        mVal2 = _mm256_add_epi16(T4, T5);
        mVal3 = _mm256_add_epi16(T6, T7);

        mVal0 = _mm256_add_epi16(mVal0, mAddOffset);
        mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
        mVal3 = _mm256_add_epi16(mVal3, mAddOffset);
        mVal0 = _mm256_srai_epi16(mVal0, shift);
        mVal1 = _mm256_srai_epi16(mVal1, shift);
        mVal2 = _mm256_srai_epi16(mVal2, shift);
        mVal3 = _mm256_srai_epi16(mVal3, shift);
        mVal0 = _mm256_packus_epi16(mVal0, mVal1);
        mVal1 = _mm256_packus_epi16(mVal2, mVal3);

        _mm256_storeu_si256((__m256i*)(dst), mVal0);
        _mm256_storeu_si256((__m256i*)(dst + 32), mVal1);

        S0 = _mm256_loadu_si256((__m256i*)(src + 64));
        S4 = _mm256_loadu_si256((__m256i*)(src + 96));
        S1 = _mm256_loadu_si256((__m256i*)(src + i_src + 64));
        S5 = _mm256_loadu_si256((__m256i*)(src + i_src + 96));
        S2 = _mm256_loadu_si256((__m256i*)(src + i_src2 + 64));
        S6 = _mm256_loadu_si256((__m256i*)(src + i_src2 + 96));
        S3 = _mm256_loadu_si256((__m256i*)(src + i_src3 + 64));
        S7 = _mm256_loadu_si256((__m256i*)(src + i_src3 + 96));

        src += i_src;
        uavs3d_prefetch(src + i_src3, _MM_HINT_NTA);

        T0 = _mm256_unpacklo_epi8(S0, S1);
        T1 = _mm256_unpacklo_epi8(S2, S3);
        T2 = _mm256_unpackhi_epi8(S0, S1);
        T3 = _mm256_unpackhi_epi8(S2, S3);
        T4 = _mm256_unpacklo_epi8(S4, S5);
        T5 = _mm256_unpacklo_epi8(S6, S7);
        T6 = _mm256_unpackhi_epi8(S4, S5);
        T7 = _mm256_unpackhi_epi8(S6, S7);

        T0 = _mm256_maddubs_epi16(T0, coeff0);
        T1 = _mm256_maddubs_epi16(T1, coeff1);
        T2 = _mm256_maddubs_epi16(T2, coeff0);
        T3 = _mm256_maddubs_epi16(T3, coeff1);
        T4 = _mm256_maddubs_epi16(T4, coeff0);
        T5 = _mm256_maddubs_epi16(T5, coeff1);
        T6 = _mm256_maddubs_epi16(T6, coeff0);
        T7 = _mm256_maddubs_epi16(T7, coeff1);

        mVal0 = _mm256_add_epi16(T0, T1);
        mVal1 = _mm256_add_epi16(T2, T3);
        mVal2 = _mm256_add_epi16(T4, T5);
        mVal3 = _mm256_add_epi16(T6, T7);

        mVal0 = _mm256_add_epi16(mVal0, mAddOffset);
        mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
        mVal3 = _mm256_add_epi16(mVal3, mAddOffset);
        mVal0 = _mm256_srai_epi16(mVal0, shift);
        mVal1 = _mm256_srai_epi16(mVal1, shift);
        mVal2 = _mm256_srai_epi16(mVal2, shift);
        mVal3 = _mm256_srai_epi16(mVal3, shift);
        mVal0 = _mm256_packus_epi16(mVal0, mVal1);
        mVal1 = _mm256_packus_epi16(mVal2, mVal3);

        _mm256_storeu_si256((__m256i*)(dst + 64), mVal0);
        _mm256_storeu_si256((__m256i*)(dst + 96), mVal1);

        dst += i_dst;
    }
}

void uavs3d_if_ver_luma_w4_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
    __m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i coeff2 = _mm256_set1_epi16(*(s16*)(coeff + 4));
    __m256i coeff3 = _mm256_set1_epi16(*(s16*)(coeff + 6));
    __m256i T0, T1, T2, T3, T4, mVal;
    __m256i R0, R1, R2, R3, R4, R5, R6, R7, R8, R9;

    src -= 3 * i_src;

    while (height > 0) {
        __m128i S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10;

        S0 = _mm_loadl_epi64((__m128i*)(src));
        S1 = _mm_loadl_epi64((__m128i*)(src + i_src));
        S2 = _mm_loadl_epi64((__m128i*)(src + i_src * 2));
        S3 = _mm_loadl_epi64((__m128i*)(src + i_src * 3));
        S4 = _mm_loadl_epi64((__m128i*)(src + i_src * 4));
        S5 = _mm_loadl_epi64((__m128i*)(src + i_src * 5));

        R0 = _mm256_set_m128i(S1, S0);
        R1 = _mm256_set_m128i(S2, S1);
        R2 = _mm256_set_m128i(S3, S2);
        R3 = _mm256_set_m128i(S4, S3);
         
        S6 = _mm_loadl_epi64((__m128i*)(src + i_src * 6));
        S7 = _mm_loadl_epi64((__m128i*)(src + i_src * 7));
        S8 = _mm_loadl_epi64((__m128i*)(src + i_src * 8));
        S9 = _mm_loadl_epi64((__m128i*)(src + i_src * 9));
        S10 = _mm_loadl_epi64((__m128i*)(src + i_src * 10));

        T0 = _mm256_unpacklo_epi8(R0, R1);
        T1 = _mm256_unpacklo_epi8(R2, R3);

        R4 = _mm256_set_m128i(S5, S4);
        R5 = _mm256_set_m128i(S6, S5);
        R6 = _mm256_set_m128i(S7, S6);
        R7 = _mm256_set_m128i(S8, S7);
        R8 = _mm256_set_m128i(S9, S8);
        R9 = _mm256_set_m128i(S10, S9);

        T2 = _mm256_unpacklo_epi8(R4, R5);
        T3 = _mm256_unpacklo_epi8(R6, R7);
        T4 = _mm256_unpacklo_epi8(R8, R9);

        T0 = _mm256_unpacklo_epi64(T0, T1);
        T1 = _mm256_unpacklo_epi64(T1, T2);
        T2 = _mm256_unpacklo_epi64(T2, T3);
        T3 = _mm256_unpacklo_epi64(T3, T4);

        T0 = _mm256_maddubs_epi16(T0, coeff0);
        T1 = _mm256_maddubs_epi16(T1, coeff1);
        T2 = _mm256_maddubs_epi16(T2, coeff2);
        T3 = _mm256_maddubs_epi16(T3, coeff3);

        T0 = _mm256_add_epi16(T0, T1);
        T2 = _mm256_add_epi16(T2, T3);
        mVal = _mm256_add_epi16(T0, T2);

        mVal = _mm256_add_epi16(mVal, mAddOffset);
        mVal = _mm256_srai_epi16(mVal, shift);
        S0 = _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1));

        M32(dst) = _mm_extract_epi32(S0, 0);
        M32(dst + i_dst) = _mm_extract_epi32(S0, 2);
        M32(dst + i_dst * 2) = _mm_extract_epi32(S0, 1);
        M32(dst + i_dst * 3) = _mm_extract_epi32(S0, 3);

        height -= 4;
        src += i_src << 2;
        dst += i_dst << 2;
    }
}

void uavs3d_if_ver_luma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    const int i_src2 = i_src * 2;
    const int i_src3 = i_src * 3;
    const int i_src4 = i_src * 4;
    const int i_src5 = i_src * 5;
    const int i_src6 = i_src * 6;
    const int i_src7 = i_src * 7;
    const int i_src8 = i_src * 8;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
    __m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i coeff2 = _mm256_set1_epi16(*(s16*)(coeff + 4));
    __m256i coeff3 = _mm256_set1_epi16(*(s16*)(coeff + 6));
    __m256i T0, T1, T2, T3, mVal;
    __m256i R0, R1, R2, R3, R4, R5, R6, R7;

    src -= 3 * i_src;

    while (height) {
        __m128i S0, S1, S2, S3, S4, S5, S6, S7, S8;

        S0 = _mm_loadl_epi64((__m128i*)(src));
        S1 = _mm_loadl_epi64((__m128i*)(src + i_src));
        S2 = _mm_loadl_epi64((__m128i*)(src + i_src2));
        S3 = _mm_loadl_epi64((__m128i*)(src + i_src3));
        S4 = _mm_loadl_epi64((__m128i*)(src + i_src4));
        S5 = _mm_loadl_epi64((__m128i*)(src + i_src5));
        S6 = _mm_loadl_epi64((__m128i*)(src + i_src6));
        S7 = _mm_loadl_epi64((__m128i*)(src + i_src7));
        S8 = _mm_loadl_epi64((__m128i*)(src + i_src8));

        R0 = _mm256_set_m128i(S1, S0);
        R1 = _mm256_set_m128i(S2, S1);
        R2 = _mm256_set_m128i(S3, S2);
        R3 = _mm256_set_m128i(S4, S3);
        R4 = _mm256_set_m128i(S5, S4);
        R5 = _mm256_set_m128i(S6, S5);
        R6 = _mm256_set_m128i(S7, S6);
        R7 = _mm256_set_m128i(S8, S7);

        src += 2 * i_src;
        height -= 2;
        uavs3d_prefetch(src + i_src7, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src8, _MM_HINT_NTA);

        T0 = _mm256_unpacklo_epi8(R0, R1);
        T1 = _mm256_unpacklo_epi8(R2, R3);
        T2 = _mm256_unpacklo_epi8(R4, R5);
        T3 = _mm256_unpacklo_epi8(R6, R7);

        T0 = _mm256_maddubs_epi16(T0, coeff0);
        T1 = _mm256_maddubs_epi16(T1, coeff1);
        T2 = _mm256_maddubs_epi16(T2, coeff2);
        T3 = _mm256_maddubs_epi16(T3, coeff3);

        T0 = _mm256_add_epi16(T0, T1);
        T2 = _mm256_add_epi16(T2, T3);
        mVal = _mm256_add_epi16(T0, T2);

        mVal = _mm256_add_epi16(mVal, mAddOffset);
        mVal = _mm256_srai_epi16(mVal, shift);
        S0 = _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1));

        _mm_storel_epi64((__m128i*)(dst), S0);
        _mm_storeh_pi((__m64*)(dst + i_dst), _mm_castsi128_ps(S0));
        dst += 2 * i_dst;
    }
}

void uavs3d_if_ver_luma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
	const int offset = 32;
	const int shift = 6;
	const int i_src2 = i_src * 2;
	const int i_src3 = i_src * 3;
	const int i_src4 = i_src * 4;
	const int i_src5 = i_src * 5;
	const int i_src6 = i_src * 6;
	const int i_src7 = i_src * 7;
	const int i_src8 = i_src * 8;
	__m256i mAddOffset = _mm256_set1_epi16(offset);
	__m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
	__m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
	__m256i coeff2 = _mm256_set1_epi16(*(s16*)(coeff + 4));
	__m256i coeff3 = _mm256_set1_epi16(*(s16*)(coeff + 6));
	__m256i T0, T1, T2, T3, T4, T5, T6, T7, mVal1, mVal2;
	__m256i R0, R1, R2, R3, R4, R5, R6, R7;

	src -= 3 * i_src;

	while(height) {
        __m128i S0, S1, S2, S3, S4, S5, S6, S7, S8;
		S0 = _mm_loadu_si128((__m128i*)(src));
		S1 = _mm_loadu_si128((__m128i*)(src + i_src));
		S2 = _mm_loadu_si128((__m128i*)(src + i_src2));
		S3 = _mm_loadu_si128((__m128i*)(src + i_src3));
		S4 = _mm_loadu_si128((__m128i*)(src + i_src4));
		S5 = _mm_loadu_si128((__m128i*)(src + i_src5));
		S6 = _mm_loadu_si128((__m128i*)(src + i_src6));
		S7 = _mm_loadu_si128((__m128i*)(src + i_src7));
		S8 = _mm_loadu_si128((__m128i*)(src + i_src8));

		R0 = _mm256_set_m128i(S0, S1);
		R1 = _mm256_set_m128i(S1, S2);
		R2 = _mm256_set_m128i(S2, S3);
		R3 = _mm256_set_m128i(S3, S4);
		R4 = _mm256_set_m128i(S4, S5);
		R5 = _mm256_set_m128i(S5, S6);
		R6 = _mm256_set_m128i(S6, S7);
		R7 = _mm256_set_m128i(S7, S8);

		src += 2 * i_src;
        height -= 2;

        uavs3d_prefetch(src + i_src7, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src8, _MM_HINT_NTA);

		T0 = _mm256_unpacklo_epi8(R0, R1);
		T1 = _mm256_unpackhi_epi8(R0, R1);
		T2 = _mm256_unpacklo_epi8(R2, R3);
		T3 = _mm256_unpackhi_epi8(R2, R3);
		T4 = _mm256_unpacklo_epi8(R4, R5);
		T5 = _mm256_unpackhi_epi8(R4, R5);
		T6 = _mm256_unpacklo_epi8(R6, R7);
		T7 = _mm256_unpackhi_epi8(R6, R7);

		T0 = _mm256_maddubs_epi16(T0, coeff0);
		T1 = _mm256_maddubs_epi16(T1, coeff0);
		T2 = _mm256_maddubs_epi16(T2, coeff1);
		T3 = _mm256_maddubs_epi16(T3, coeff1);
		T4 = _mm256_maddubs_epi16(T4, coeff2);
		T5 = _mm256_maddubs_epi16(T5, coeff2);
		T6 = _mm256_maddubs_epi16(T6, coeff3);
		T7 = _mm256_maddubs_epi16(T7, coeff3);

		mVal1 = _mm256_add_epi16(T0, T2);
		mVal2 = _mm256_add_epi16(T1, T3);
		mVal1 = _mm256_add_epi16(mVal1, T4);
		mVal2 = _mm256_add_epi16(mVal2, T5);
		mVal1 = _mm256_add_epi16(mVal1, T6);
		mVal2 = _mm256_add_epi16(mVal2, T7);

		mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
		mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
		mVal1 = _mm256_srai_epi16(mVal1, shift);
		mVal2 = _mm256_srai_epi16(mVal2, shift);
		mVal1 = _mm256_packus_epi16(mVal1, mVal2);

        _mm_storeu_si128((__m128i*)dst, _mm256_extractf128_si256(mVal1, 1));
        _mm_storeu_si128((__m128i*)(dst + i_dst), _mm256_castsi256_si128(mVal1));
		dst += 2 * i_dst;
	}
}

void uavs3d_if_ver_luma_w32_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
	const int offset = 32;
	const int shift = 6;
	const int i_src2 = i_src * 2;
	const int i_src3 = i_src * 3;
	const int i_src4 = i_src * 4;
	const int i_src5 = i_src * 5;
	const int i_src6 = i_src * 6;
	const int i_src7 = i_src * 7;

	__m256i mAddOffset = _mm256_set1_epi16(offset);
	__m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
	__m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
	__m256i coeff2 = _mm256_set1_epi16(*(s16*)(coeff + 4));
	__m256i coeff3 = _mm256_set1_epi16(*(s16*)(coeff + 6));
	__m256i T0, T1, T2, T3, T4, T5, T6, T7, mVal1, mVal2;


	src -= 3 * i_src;
	while (height--) {
        __m256i S0, S1, S2, S3, S4, S5, S6, S7;
		S0 = _mm256_loadu_si256((__m256i*)(src));
		S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
		S2 = _mm256_loadu_si256((__m256i*)(src + i_src2));
		S3 = _mm256_loadu_si256((__m256i*)(src + i_src3));
		S4 = _mm256_loadu_si256((__m256i*)(src + i_src4));
		S5 = _mm256_loadu_si256((__m256i*)(src + i_src5));
		S6 = _mm256_loadu_si256((__m256i*)(src + i_src6));
		S7 = _mm256_loadu_si256((__m256i*)(src + i_src7));

		src += i_src;
		T0 = _mm256_unpacklo_epi8(S0, S1);
		T1 = _mm256_unpacklo_epi8(S2, S3);
		T2 = _mm256_unpacklo_epi8(S4, S5);
		T3 = _mm256_unpacklo_epi8(S6, S7);
		T4 = _mm256_unpackhi_epi8(S0, S1);
		T5 = _mm256_unpackhi_epi8(S2, S3);
		T6 = _mm256_unpackhi_epi8(S4, S5);
		T7 = _mm256_unpackhi_epi8(S6, S7);

        uavs3d_prefetch(src + i_src7, _MM_HINT_NTA);

		T0 = _mm256_maddubs_epi16(T0, coeff0);
		T1 = _mm256_maddubs_epi16(T1, coeff1);
		T2 = _mm256_maddubs_epi16(T2, coeff2);
		T3 = _mm256_maddubs_epi16(T3, coeff3);
		T4 = _mm256_maddubs_epi16(T4, coeff0);
		T5 = _mm256_maddubs_epi16(T5, coeff1);
		T6 = _mm256_maddubs_epi16(T6, coeff2);
		T7 = _mm256_maddubs_epi16(T7, coeff3);

		mVal1 = _mm256_add_epi16(T0, T1);
		mVal2 = _mm256_add_epi16(T4, T5);
		mVal1 = _mm256_add_epi16(mVal1, T2);
		mVal2 = _mm256_add_epi16(mVal2, T6);
		mVal1 = _mm256_add_epi16(mVal1, T3);
		mVal2 = _mm256_add_epi16(mVal2, T7);

		mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
		mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
		mVal1 = _mm256_srai_epi16(mVal1, shift);
		mVal2 = _mm256_srai_epi16(mVal2, shift);
		mVal1 = _mm256_packus_epi16(mVal1, mVal2);

		_mm256_storeu_si256((__m256i*)(dst), mVal1);

		dst += i_dst;
	}
}

void uavs3d_if_ver_luma_w64_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
	const int offset = 32;
	const int shift = 6;
	const int i_src2 = i_src * 2;
	const int i_src3 = i_src * 3;
	const int i_src4 = i_src * 4;
	const int i_src5 = i_src * 5;
	const int i_src6 = i_src * 6;
	const int i_src7 = i_src * 7;
	__m256i mAddOffset = _mm256_set1_epi16(offset);
	__m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
	__m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
	__m256i coeff2 = _mm256_set1_epi16(*(s16*)(coeff + 4));
	__m256i coeff3 = _mm256_set1_epi16(*(s16*)(coeff + 6));
	__m256i T0, T1, T2, T3, T4, T5, T6, T7, mVal1, mVal2;

	src -= i_src3;

	while (height--) {
		const pel *p = src + 32;
        __m256i S0, S1, S2, S3, S4, S5, S6, S7;
		S0 = _mm256_loadu_si256((__m256i*)(src));
		S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
		S2 = _mm256_loadu_si256((__m256i*)(src + i_src2));
		S3 = _mm256_loadu_si256((__m256i*)(src + i_src3));
		S4 = _mm256_loadu_si256((__m256i*)(src + i_src4));
		S5 = _mm256_loadu_si256((__m256i*)(src + i_src5));
		S6 = _mm256_loadu_si256((__m256i*)(src + i_src6));
		S7 = _mm256_loadu_si256((__m256i*)(src + i_src7));

		T0 = _mm256_unpacklo_epi8(S0, S1);
		T1 = _mm256_unpacklo_epi8(S2, S3);
		T2 = _mm256_unpacklo_epi8(S4, S5);
		T3 = _mm256_unpacklo_epi8(S6, S7);
		T4 = _mm256_unpackhi_epi8(S0, S1);
		T5 = _mm256_unpackhi_epi8(S2, S3);
		T6 = _mm256_unpackhi_epi8(S4, S5);
		T7 = _mm256_unpackhi_epi8(S6, S7);

		T0 = _mm256_maddubs_epi16(T0, coeff0);
		T1 = _mm256_maddubs_epi16(T1, coeff1);
		T2 = _mm256_maddubs_epi16(T2, coeff2);
		T3 = _mm256_maddubs_epi16(T3, coeff3);
		T4 = _mm256_maddubs_epi16(T4, coeff0);
		T5 = _mm256_maddubs_epi16(T5, coeff1);
		T6 = _mm256_maddubs_epi16(T6, coeff2);
		T7 = _mm256_maddubs_epi16(T7, coeff3);

		mVal1 = _mm256_add_epi16(T0, T1);
		mVal2 = _mm256_add_epi16(T4, T5);
		mVal1 = _mm256_add_epi16(mVal1, T2);
		mVal2 = _mm256_add_epi16(mVal2, T6);
		mVal1 = _mm256_add_epi16(mVal1, T3);
		mVal2 = _mm256_add_epi16(mVal2, T7);

		mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
		mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
		mVal1 = _mm256_srai_epi16(mVal1, shift);
		mVal2 = _mm256_srai_epi16(mVal2, shift);
		mVal1 = _mm256_packus_epi16(mVal1, mVal2);

		_mm256_storeu_si256((__m256i*)(dst), mVal1);

		S0 = _mm256_loadu_si256((__m256i*)(p));
		S1 = _mm256_loadu_si256((__m256i*)(p + i_src));
		S2 = _mm256_loadu_si256((__m256i*)(p + i_src2));
		S3 = _mm256_loadu_si256((__m256i*)(p + i_src3));
		S4 = _mm256_loadu_si256((__m256i*)(p + i_src4));
		S5 = _mm256_loadu_si256((__m256i*)(p + i_src5));
		S6 = _mm256_loadu_si256((__m256i*)(p + i_src6));
		S7 = _mm256_loadu_si256((__m256i*)(p + i_src7));

        src += i_src;
		T0 = _mm256_unpacklo_epi8(S0, S1);
		T1 = _mm256_unpacklo_epi8(S2, S3);
		T2 = _mm256_unpacklo_epi8(S4, S5);
		T3 = _mm256_unpacklo_epi8(S6, S7);
		T4 = _mm256_unpackhi_epi8(S0, S1);
		T5 = _mm256_unpackhi_epi8(S2, S3);
		T6 = _mm256_unpackhi_epi8(S4, S5);
		T7 = _mm256_unpackhi_epi8(S6, S7);

        uavs3d_prefetch(src + i_src7, _MM_HINT_NTA);

		T0 = _mm256_maddubs_epi16(T0, coeff0);
		T1 = _mm256_maddubs_epi16(T1, coeff1);
		T2 = _mm256_maddubs_epi16(T2, coeff2);
		T3 = _mm256_maddubs_epi16(T3, coeff3);
		T4 = _mm256_maddubs_epi16(T4, coeff0);
		T5 = _mm256_maddubs_epi16(T5, coeff1);
		T6 = _mm256_maddubs_epi16(T6, coeff2);
		T7 = _mm256_maddubs_epi16(T7, coeff3);

		mVal1 = _mm256_add_epi16(T0, T1);
		mVal2 = _mm256_add_epi16(T4, T5);
		mVal1 = _mm256_add_epi16(mVal1, T2);
		mVal2 = _mm256_add_epi16(mVal2, T6);
		mVal1 = _mm256_add_epi16(mVal1, T3);
		mVal2 = _mm256_add_epi16(mVal2, T7);

		mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
		mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
		mVal1 = _mm256_srai_epi16(mVal1, shift);
		mVal2 = _mm256_srai_epi16(mVal2, shift);
		mVal1 = _mm256_packus_epi16(mVal1, mVal2);

		_mm256_storeu_si256((__m256i*)(dst + 32), mVal1);

		dst += i_dst;
	}
}

void uavs3d_if_ver_luma_w128_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    const int i_src2 = i_src * 2;
    const int i_src3 = i_src * 3;
    const int i_src4 = i_src * 4;
    const int i_src5 = i_src * 5;
    const int i_src6 = i_src * 6;
    const int i_src7 = i_src * 7;
    __m256i mAddOffset = _mm256_set1_epi16(offset);
    __m256i coeff0 = _mm256_set1_epi16(*(s16*)coeff);
    __m256i coeff1 = _mm256_set1_epi16(*(s16*)(coeff + 2));
    __m256i coeff2 = _mm256_set1_epi16(*(s16*)(coeff + 4));
    __m256i coeff3 = _mm256_set1_epi16(*(s16*)(coeff + 6));
    __m256i T0, T1, T2, T3, T4, T5, T6, T7, mVal1, mVal2;

    src -= 3 * i_src;

    while (height--) {
        const pel *p = src + 32;
        __m256i S0, S1, S2, S3, S4, S5, S6, S7;
        S0 = _mm256_loadu_si256((__m256i*)(src));
        S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
        S2 = _mm256_loadu_si256((__m256i*)(src + i_src2));
        S3 = _mm256_loadu_si256((__m256i*)(src + i_src3));
        S4 = _mm256_loadu_si256((__m256i*)(src + i_src4));
        S5 = _mm256_loadu_si256((__m256i*)(src + i_src5));
        S6 = _mm256_loadu_si256((__m256i*)(src + i_src6));
        S7 = _mm256_loadu_si256((__m256i*)(src + i_src7));

        T0 = _mm256_unpacklo_epi8(S0, S1);
        T1 = _mm256_unpacklo_epi8(S2, S3);
        T2 = _mm256_unpacklo_epi8(S4, S5);
        T3 = _mm256_unpacklo_epi8(S6, S7);
        T4 = _mm256_unpackhi_epi8(S0, S1);
        T5 = _mm256_unpackhi_epi8(S2, S3);
        T6 = _mm256_unpackhi_epi8(S4, S5);
        T7 = _mm256_unpackhi_epi8(S6, S7);

        T0 = _mm256_maddubs_epi16(T0, coeff0);
        T1 = _mm256_maddubs_epi16(T1, coeff1);
        T2 = _mm256_maddubs_epi16(T2, coeff2);
        T3 = _mm256_maddubs_epi16(T3, coeff3);
        T4 = _mm256_maddubs_epi16(T4, coeff0);
        T5 = _mm256_maddubs_epi16(T5, coeff1);
        T6 = _mm256_maddubs_epi16(T6, coeff2);
        T7 = _mm256_maddubs_epi16(T7, coeff3);

        mVal1 = _mm256_add_epi16(T0, T1);
        mVal2 = _mm256_add_epi16(T4, T5);
        mVal1 = _mm256_add_epi16(mVal1, T2);
        mVal2 = _mm256_add_epi16(mVal2, T6);
        mVal1 = _mm256_add_epi16(mVal1, T3);
        mVal2 = _mm256_add_epi16(mVal2, T7);

        mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
        mVal1 = _mm256_srai_epi16(mVal1, shift);
        mVal2 = _mm256_srai_epi16(mVal2, shift);
        mVal1 = _mm256_packus_epi16(mVal1, mVal2);

        _mm256_storeu_si256((__m256i*)(dst), mVal1);

        S0 = _mm256_loadu_si256((__m256i*)(p));
        S1 = _mm256_loadu_si256((__m256i*)(p + i_src));
        S2 = _mm256_loadu_si256((__m256i*)(p + i_src2));
        S3 = _mm256_loadu_si256((__m256i*)(p + i_src3));
        S4 = _mm256_loadu_si256((__m256i*)(p + i_src4));
        S5 = _mm256_loadu_si256((__m256i*)(p + i_src5));
        S6 = _mm256_loadu_si256((__m256i*)(p + i_src6));
        S7 = _mm256_loadu_si256((__m256i*)(p + i_src7));

        p += 32;

        T0 = _mm256_unpacklo_epi8(S0, S1);
        T1 = _mm256_unpacklo_epi8(S2, S3);
        T2 = _mm256_unpacklo_epi8(S4, S5);
        T3 = _mm256_unpacklo_epi8(S6, S7);
        T4 = _mm256_unpackhi_epi8(S0, S1);
        T5 = _mm256_unpackhi_epi8(S2, S3);
        T6 = _mm256_unpackhi_epi8(S4, S5);
        T7 = _mm256_unpackhi_epi8(S6, S7);

        T0 = _mm256_maddubs_epi16(T0, coeff0);
        T1 = _mm256_maddubs_epi16(T1, coeff1);
        T2 = _mm256_maddubs_epi16(T2, coeff2);
        T3 = _mm256_maddubs_epi16(T3, coeff3);
        T4 = _mm256_maddubs_epi16(T4, coeff0);
        T5 = _mm256_maddubs_epi16(T5, coeff1);
        T6 = _mm256_maddubs_epi16(T6, coeff2);
        T7 = _mm256_maddubs_epi16(T7, coeff3);

        mVal1 = _mm256_add_epi16(T0, T1);
        mVal2 = _mm256_add_epi16(T4, T5);
        mVal1 = _mm256_add_epi16(mVal1, T2);
        mVal2 = _mm256_add_epi16(mVal2, T6);
        mVal1 = _mm256_add_epi16(mVal1, T3);
        mVal2 = _mm256_add_epi16(mVal2, T7);

        mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
        mVal1 = _mm256_srai_epi16(mVal1, shift);
        mVal2 = _mm256_srai_epi16(mVal2, shift);
        mVal1 = _mm256_packus_epi16(mVal1, mVal2);

        _mm256_storeu_si256((__m256i*)(dst + 32), mVal1);

        S0 = _mm256_loadu_si256((__m256i*)(p));
        S1 = _mm256_loadu_si256((__m256i*)(p + i_src));
        S2 = _mm256_loadu_si256((__m256i*)(p + i_src2));
        S3 = _mm256_loadu_si256((__m256i*)(p + i_src3));
        S4 = _mm256_loadu_si256((__m256i*)(p + i_src4));
        S5 = _mm256_loadu_si256((__m256i*)(p + i_src5));
        S6 = _mm256_loadu_si256((__m256i*)(p + i_src6));
        S7 = _mm256_loadu_si256((__m256i*)(p + i_src7));

        p += 32;

        T0 = _mm256_unpacklo_epi8(S0, S1);
        T1 = _mm256_unpacklo_epi8(S2, S3);
        T2 = _mm256_unpacklo_epi8(S4, S5);
        T3 = _mm256_unpacklo_epi8(S6, S7);
        T4 = _mm256_unpackhi_epi8(S0, S1);
        T5 = _mm256_unpackhi_epi8(S2, S3);
        T6 = _mm256_unpackhi_epi8(S4, S5);
        T7 = _mm256_unpackhi_epi8(S6, S7);

        T0 = _mm256_maddubs_epi16(T0, coeff0);
        T1 = _mm256_maddubs_epi16(T1, coeff1);
        T2 = _mm256_maddubs_epi16(T2, coeff2);
        T3 = _mm256_maddubs_epi16(T3, coeff3);
        T4 = _mm256_maddubs_epi16(T4, coeff0);
        T5 = _mm256_maddubs_epi16(T5, coeff1);
        T6 = _mm256_maddubs_epi16(T6, coeff2);
        T7 = _mm256_maddubs_epi16(T7, coeff3);

        mVal1 = _mm256_add_epi16(T0, T1);
        mVal2 = _mm256_add_epi16(T4, T5);
        mVal1 = _mm256_add_epi16(mVal1, T2);
        mVal2 = _mm256_add_epi16(mVal2, T6);
        mVal1 = _mm256_add_epi16(mVal1, T3);
        mVal2 = _mm256_add_epi16(mVal2, T7);

        mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
        mVal1 = _mm256_srai_epi16(mVal1, shift);
        mVal2 = _mm256_srai_epi16(mVal2, shift);
        mVal1 = _mm256_packus_epi16(mVal1, mVal2);

        _mm256_storeu_si256((__m256i*)(dst + 64), mVal1);

        S0 = _mm256_loadu_si256((__m256i*)(p));
        S1 = _mm256_loadu_si256((__m256i*)(p + i_src));
        S2 = _mm256_loadu_si256((__m256i*)(p + i_src2));
        S3 = _mm256_loadu_si256((__m256i*)(p + i_src3));
        S4 = _mm256_loadu_si256((__m256i*)(p + i_src4));
        S5 = _mm256_loadu_si256((__m256i*)(p + i_src5));
        S6 = _mm256_loadu_si256((__m256i*)(p + i_src6));
        S7 = _mm256_loadu_si256((__m256i*)(p + i_src7));

        src += i_src;
        uavs3d_prefetch(src + i_src7, _MM_HINT_NTA);
        T0 = _mm256_unpacklo_epi8(S0, S1);
        T1 = _mm256_unpacklo_epi8(S2, S3);
        T2 = _mm256_unpacklo_epi8(S4, S5);
        T3 = _mm256_unpacklo_epi8(S6, S7);
        T4 = _mm256_unpackhi_epi8(S0, S1);
        T5 = _mm256_unpackhi_epi8(S2, S3);
        T6 = _mm256_unpackhi_epi8(S4, S5);
        T7 = _mm256_unpackhi_epi8(S6, S7);

        T0 = _mm256_maddubs_epi16(T0, coeff0);
        T1 = _mm256_maddubs_epi16(T1, coeff1);
        T2 = _mm256_maddubs_epi16(T2, coeff2);
        T3 = _mm256_maddubs_epi16(T3, coeff3);
        T4 = _mm256_maddubs_epi16(T4, coeff0);
        T5 = _mm256_maddubs_epi16(T5, coeff1);
        T6 = _mm256_maddubs_epi16(T6, coeff2);
        T7 = _mm256_maddubs_epi16(T7, coeff3);

        mVal1 = _mm256_add_epi16(T0, T1);
        mVal2 = _mm256_add_epi16(T4, T5);
        mVal1 = _mm256_add_epi16(mVal1, T2);
        mVal2 = _mm256_add_epi16(mVal2, T6);
        mVal1 = _mm256_add_epi16(mVal1, T3);
        mVal2 = _mm256_add_epi16(mVal2, T7);

        mVal1 = _mm256_add_epi16(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi16(mVal2, mAddOffset);
        mVal1 = _mm256_srai_epi16(mVal1, shift);
        mVal2 = _mm256_srai_epi16(mVal2, shift);
        mVal1 = _mm256_packus_epi16(mVal1, mVal2);

        _mm256_storeu_si256((__m256i*)(dst + 96), mVal1);

        dst += i_dst;
    }
}

void uavs3d_if_hor_ver_chroma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    int i_src2 = i_src << 1;
    int i_src3 = i_src + i_src2;
    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coef_x);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coef_x + 2));
    __m256i mSwitch1 = _mm256_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
    __m256i mSwitch2 = _mm256_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13, 4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);
    __m256i mVal[4];
    __m128i s0, s1, s2, s3;
    __m128i mCoefy11 = _mm_set1_epi16(*(s16*)coef_y);
    __m128i mCoefy22 = _mm_set1_epi16(*(s16*)(coef_y + 2));
    __m256i mCoefy1_ver = _mm256_cvtepi8_epi16(mCoefy11);
    __m256i mCoefy2_ver = _mm256_cvtepi8_epi16(mCoefy22);
    __m256i mAddOffset = _mm256_set1_epi32(1 << 11);
    __m256i R0, R1, R2, R3;
    __m256i T0, T1, T2, T3, T4, T5;
    __m256i S0, S1, S2, S3;
    int shift = 12;

    src = src - i_src - 2;

    // hor filter: first 3 row
    s0 = _mm_loadu_si128((__m128i*)(src));
    s1 = _mm_loadu_si128((__m128i*)(src + i_src));
    s2 = _mm_loadu_si128((__m128i*)(src + i_src2));

    S0 = _mm256_set_m128i(s0, s0);
    S1 = _mm256_set_m128i(s2, s1);

    T0 = _mm256_shuffle_epi8(S0, mSwitch1);
    T1 = _mm256_shuffle_epi8(S0, mSwitch2);
    T2 = _mm256_shuffle_epi8(S1, mSwitch1);
    T3 = _mm256_shuffle_epi8(S1, mSwitch2);

    T0 = _mm256_maddubs_epi16(T0, mCoefy1_hor);
    T1 = _mm256_maddubs_epi16(T1, mCoefy2_hor);
    T2 = _mm256_maddubs_epi16(T2, mCoefy1_hor);
    T3 = _mm256_maddubs_epi16(T3, mCoefy2_hor);

    mVal[0] = _mm256_add_epi16(T0, T1);
    mVal[1] = _mm256_add_epi16(T2, T3);

    src += i_src3;

    mVal[0] = _mm256_permute4x64_epi64(mVal[0], 0x44);

    while (height > 0) {
        // hor
        s0 = _mm_loadu_si128((__m128i*)(src));
        s1 = _mm_loadu_si128((__m128i*)(src + i_src));
        s2 = _mm_loadu_si128((__m128i*)(src + i_src2));
        s3 = _mm_loadu_si128((__m128i*)(src + i_src3));

        S0 = _mm256_set_m128i(s1, s0);
        S1 = _mm256_set_m128i(s3, s2);

        T0 = _mm256_shuffle_epi8(S0, mSwitch1);
        T1 = _mm256_shuffle_epi8(S0, mSwitch2);
        T2 = _mm256_shuffle_epi8(S1, mSwitch1);
        T3 = _mm256_shuffle_epi8(S1, mSwitch2);

        T0 = _mm256_maddubs_epi16(T0, mCoefy1_hor);
        T1 = _mm256_maddubs_epi16(T1, mCoefy2_hor);
        T2 = _mm256_maddubs_epi16(T2, mCoefy1_hor);
        T3 = _mm256_maddubs_epi16(T3, mCoefy2_hor);

        mVal[2] = _mm256_add_epi16(T0, T1);
        mVal[3] = _mm256_add_epi16(T2, T3);

        src += i_src2 << 1;

        // ver
        S0 = _mm256_permute2x128_si256(mVal[0], mVal[1], 0x21);
        S1 = mVal[1];
        S2 = _mm256_permute2x128_si256(mVal[1], mVal[2], 0x21);
        S3 = mVal[2];

        uavs3d_prefetch(src + i_src3, _MM_HINT_NTA);

        T0 = _mm256_unpacklo_epi16(S0, S1);
        T1 = _mm256_unpacklo_epi16(S2, S3);
        T2 = _mm256_unpackhi_epi16(S0, S1);
        T3 = _mm256_unpackhi_epi16(S2, S3);

        R0 = _mm256_madd_epi16(T0, mCoefy1_ver);
        R1 = _mm256_madd_epi16(T1, mCoefy2_ver);
        R2 = _mm256_madd_epi16(T2, mCoefy1_ver);
        R3 = _mm256_madd_epi16(T3, mCoefy2_ver);

        R0 = _mm256_add_epi32(R0, R1);
        R2 = _mm256_add_epi32(R2, R3);

        S2 = _mm256_permute2x128_si256(mVal[2], mVal[3], 0x21);
        S3 = mVal[3];

        R0 = _mm256_add_epi32(R0, mAddOffset);
        R2 = _mm256_add_epi32(R2, mAddOffset);
        R0 = _mm256_srai_epi32(R0, shift);
        R2 = _mm256_srai_epi32(R2, shift);
        R0 = _mm256_packs_epi32(R0, R2);

        T4 = _mm256_unpacklo_epi16(S2, S3);
        T5 = _mm256_unpackhi_epi16(S2, S3);

        mVal[0] = mVal[2];
        mVal[1] = mVal[3];

        T0 = _mm256_madd_epi16(T1, mCoefy1_ver);
        T1 = _mm256_madd_epi16(T4, mCoefy2_ver);
        T2 = _mm256_madd_epi16(T3, mCoefy1_ver);
        T3 = _mm256_madd_epi16(T5, mCoefy2_ver);

        T0 = _mm256_add_epi32(T0, T1);
        T2 = _mm256_add_epi32(T2, T3);

        T0 = _mm256_add_epi32(T0, mAddOffset);
        T2 = _mm256_add_epi32(T2, mAddOffset);
        T0 = _mm256_srai_epi32(T0, shift);
        T2 = _mm256_srai_epi32(T2, shift);

        s2 = _mm_packus_epi16(_mm256_castsi256_si128(R0), _mm256_extracti128_si256(R0, 1));

        T0 = _mm256_packs_epi32(T0, T2);
        s3 = _mm_packus_epi16(_mm256_castsi256_si128(T0), _mm256_extracti128_si256(T0, 1));

        _mm_storel_epi64((__m128i*)(dst), s2);
        _mm_storeh_pi((__m64*)(dst + i_dst), _mm_castsi128_ps(s2));
        _mm_storel_epi64((__m128i*)(dst + i_dst*2), s3);
        _mm_storeh_pi((__m64*)(dst + i_dst*3), _mm_castsi128_ps(s3));

        dst += i_dst << 2;
        height -= 4;
    }
}

void uavs3d_if_hor_ver_chroma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(128 + 3) * 16]);
    s16 *tmp = tmp_res;
    const int i_tmp  = 16;
    const int i_tmp2 = 32;
    const int i_tmp3 = 48;
    const int i_tmp4 = 64;
    int row;
    int shift = 12;
    __m256i mAddOffset = _mm256_set1_epi32(1 << 11);
    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coef_x);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coef_x + 2));
    __m256i mSwitch1 = _mm256_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
    __m256i mSwitch2 = _mm256_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13, 4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);
    __m256i T0, T1, T2, T3, S0, S1, S2, S3, S4, R0, R1, sum;
    __m128i mCoefy11 = _mm_set1_epi16(*(s16*)coef_y);
    __m128i mCoefy22 = _mm_set1_epi16(*(s16*)(coef_y + 2));
    __m256i mVal1, mVal2, mVal0;

    __m256i mCoefy1_ver = _mm256_cvtepi8_epi16(mCoefy11);
    __m256i mCoefy2_ver = _mm256_cvtepi8_epi16(mCoefy22);
    int i_dst2 = i_dst * 2;


    //HOR
    src = src - i_src - 2;
    row = height + 3;

    while (row--) {
        S0 = _mm256_loadu_si256((__m256i*)(src));
        src += i_src;
        S1 = _mm256_permute4x64_epi64(S0, 0x94);
        uavs3d_prefetch(src, _MM_HINT_NTA);
        R0 = _mm256_shuffle_epi8(S1, mSwitch1);
        R1 = _mm256_shuffle_epi8(S1, mSwitch2);
        T0 = _mm256_maddubs_epi16(R0, mCoefy1_hor);
        T1 = _mm256_maddubs_epi16(R1, mCoefy2_hor);
        sum = _mm256_add_epi16(T0, T1);

        _mm256_store_si256((__m256i*)(tmp), sum);
        tmp += i_tmp;
    }

    // VER
    tmp = tmp_res;
    while (height) {
        S0 = _mm256_load_si256((__m256i*)(tmp));
        S1 = _mm256_load_si256((__m256i*)(tmp + i_tmp));
        S2 = _mm256_load_si256((__m256i*)(tmp + i_tmp2));
        S3 = _mm256_load_si256((__m256i*)(tmp + i_tmp3));
        S4 = _mm256_load_si256((__m256i*)(tmp + i_tmp4));

        T0 = _mm256_unpacklo_epi16(S0, S1);
        T1 = _mm256_unpacklo_epi16(S2, S3);
        T2 = _mm256_unpackhi_epi16(S0, S1);
        T3 = _mm256_unpackhi_epi16(S2, S3);

        T0 = _mm256_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm256_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm256_madd_epi16(T2, mCoefy1_ver);
        T3 = _mm256_madd_epi16(T3, mCoefy2_ver);

        mVal1 = _mm256_add_epi32(T0, T1);
        mVal2 = _mm256_add_epi32(T2, T3);

        mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm256_srai_epi32(mVal1, shift);
        mVal2 = _mm256_srai_epi32(mVal2, shift);

        mVal0 = _mm256_packs_epi32(mVal1, mVal2);

        T0 = _mm256_unpacklo_epi16(S1, S2);
        T1 = _mm256_unpacklo_epi16(S3, S4);
        T2 = _mm256_unpackhi_epi16(S1, S2);
        T3 = _mm256_unpackhi_epi16(S3, S4);

        T0 = _mm256_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm256_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm256_madd_epi16(T2, mCoefy1_ver);
        T3 = _mm256_madd_epi16(T3, mCoefy2_ver);

        mVal1 = _mm256_add_epi32(T0, T1);
        mVal2 = _mm256_add_epi32(T2, T3);

        mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm256_srai_epi32(mVal1, shift);
        mVal2 = _mm256_srai_epi32(mVal2, shift);

        mVal1 = _mm256_packs_epi32(mVal1, mVal2);

        _mm_storeu_si128((__m128i*)(dst), _mm_packus_epi16(_mm256_castsi256_si128(mVal0), _mm256_extracti128_si256(mVal0, 1)));
        _mm_storeu_si128((__m128i*)(dst + i_dst), _mm_packus_epi16(_mm256_castsi256_si128(mVal1), _mm256_extracti128_si256(mVal1, 1)));

        tmp += i_tmp2;
        dst += i_dst2;
        height -= 2;
    }
}

void uavs3d_if_hor_ver_chroma_w32x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(128 + 3) * 128]);
    s16 *tmp = tmp_res;
    const int i_tmp = width;
    const int i_tmp2 = width << 1;
    const int i_tmp3 = width + i_tmp2;
    const int i_tmp4 = width << 2;
    const int i_tmp5 = i_tmp2 + i_tmp3;
    const int i_tmp6 = i_tmp3 << 1;

	int row, col;
	int shift = 12;
	__m256i mAddOffset = _mm256_set1_epi32(1 << 11);
	__m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coef_x);
	__m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coef_x + 2));
	__m256i mSwitch1 = _mm256_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9, 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
	__m256i mSwitch2 = _mm256_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13, 4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);
	__m256i T0, T1, T2, T3, S0, S1, S2, S3, S4, S5, S6, R0, R1, R2, R3, sum0, sum1;

    __m128i mCoefy11 = _mm_set1_epi16(*(s16*)coef_y);
    __m128i mCoefy22 = _mm_set1_epi16(*(s16*)(coef_y + 2));
    __m256i mVal1, mVal2, mVal;

    __m256i mCoefy1_ver = _mm256_cvtepi8_epi16(mCoefy11);
    __m256i mCoefy2_ver = _mm256_cvtepi8_epi16(mCoefy22);

	//HOR
	src = src - i_src - 2;
    row = height + 3;

	while (row--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
		for (col = 0; col < width; col += 32)
		{
			S0 = _mm256_loadu_si256((__m256i*)(src + col));
            S1 = _mm256_loadu_si256((__m256i*)(src + col + 16));
			S2 = _mm256_permute4x64_epi64(S0, 0x94);
            S3 = _mm256_permute4x64_epi64(S1, 0x94);
			R0 = _mm256_shuffle_epi8(S2, mSwitch1);
			R1 = _mm256_shuffle_epi8(S2, mSwitch2);
            R2 = _mm256_shuffle_epi8(S3, mSwitch1);
            R3 = _mm256_shuffle_epi8(S3, mSwitch2);
			T0 = _mm256_maddubs_epi16(R0, mCoefy1_hor);
			T1 = _mm256_maddubs_epi16(R1, mCoefy2_hor);
            T2 = _mm256_maddubs_epi16(R2, mCoefy1_hor);
            T3 = _mm256_maddubs_epi16(R3, mCoefy2_hor);
			sum0 = _mm256_add_epi16(T0, T1);
            sum1 = _mm256_add_epi16(T2, T3);

			_mm256_store_si256((__m256i*)(tmp + col), sum0);
            _mm256_store_si256((__m256i*)(tmp + col + 16), sum1);
		}
		src += i_src;
		tmp += i_tmp;
	}

	// VER
	tmp = tmp_res;
	
	while (height) {
		for (col = 0; col < width; col += 16)
		{
			S0 = _mm256_load_si256((__m256i*)(tmp + col));
			S1 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp));
			S2 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp2));
			S3 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp3));
			S4 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp4));
			S5 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp5));
			S6 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp6));

			T0 = _mm256_unpacklo_epi16(S0, S1);
			T1 = _mm256_unpacklo_epi16(S2, S3);
			T2 = _mm256_unpackhi_epi16(S0, S1);
			T3 = _mm256_unpackhi_epi16(S2, S3);

			T0 = _mm256_madd_epi16(T0, mCoefy1_ver);
			T1 = _mm256_madd_epi16(T1, mCoefy2_ver);
			T2 = _mm256_madd_epi16(T2, mCoefy1_ver);
			T3 = _mm256_madd_epi16(T3, mCoefy2_ver);

			mVal1 = _mm256_add_epi32(T0, T1);
			mVal2 = _mm256_add_epi32(T2, T3);

			mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
			mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
			mVal1 = _mm256_srai_epi32(mVal1, shift);
			mVal2 = _mm256_srai_epi32(mVal2, shift);

			mVal = _mm256_packs_epi32(mVal1, mVal2);
			_mm_storeu_si128((__m128i*)(dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

			T0 = _mm256_unpacklo_epi16(S1, S2);
			T1 = _mm256_unpacklo_epi16(S3, S4);
			T2 = _mm256_unpackhi_epi16(S1, S2);
			T3 = _mm256_unpackhi_epi16(S3, S4);

			T0 = _mm256_madd_epi16(T0, mCoefy1_ver);
			T1 = _mm256_madd_epi16(T1, mCoefy2_ver);
			T2 = _mm256_madd_epi16(T2, mCoefy1_ver);
			T3 = _mm256_madd_epi16(T3, mCoefy2_ver);

			mVal1 = _mm256_add_epi32(T0, T1);
			mVal2 = _mm256_add_epi32(T2, T3);

			mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
			mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
			mVal1 = _mm256_srai_epi32(mVal1, shift);
			mVal2 = _mm256_srai_epi32(mVal2, shift);

			mVal = _mm256_packs_epi32(mVal1, mVal2);
			_mm_storeu_si128((__m128i*)(dst + i_dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

			T0 = _mm256_unpacklo_epi16(S2, S3);
			T1 = _mm256_unpacklo_epi16(S4, S5);
			T2 = _mm256_unpackhi_epi16(S2, S3);
			T3 = _mm256_unpackhi_epi16(S4, S5);

			T0 = _mm256_madd_epi16(T0, mCoefy1_ver);
			T1 = _mm256_madd_epi16(T1, mCoefy2_ver);
			T2 = _mm256_madd_epi16(T2, mCoefy1_ver);
			T3 = _mm256_madd_epi16(T3, mCoefy2_ver);

			mVal1 = _mm256_add_epi32(T0, T1);
			mVal2 = _mm256_add_epi32(T2, T3);

			mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
			mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
			mVal1 = _mm256_srai_epi32(mVal1, shift);
			mVal2 = _mm256_srai_epi32(mVal2, shift);

			mVal = _mm256_packs_epi32(mVal1, mVal2);
			_mm_storeu_si128((__m128i*)(dst + 2 * i_dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

			T0 = _mm256_unpacklo_epi16(S3, S4);
			T1 = _mm256_unpacklo_epi16(S5, S6);
			T2 = _mm256_unpackhi_epi16(S3, S4);
			T3 = _mm256_unpackhi_epi16(S5, S6);

			T0 = _mm256_madd_epi16(T0, mCoefy1_ver);
			T1 = _mm256_madd_epi16(T1, mCoefy2_ver);
			T2 = _mm256_madd_epi16(T2, mCoefy1_ver);
			T3 = _mm256_madd_epi16(T3, mCoefy2_ver);

			mVal1 = _mm256_add_epi32(T0, T1);
			mVal2 = _mm256_add_epi32(T2, T3);

			mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
			mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
			mVal1 = _mm256_srai_epi32(mVal1, shift);
			mVal2 = _mm256_srai_epi32(mVal2, shift);

			mVal = _mm256_packs_epi32(mVal1, mVal2);
			_mm_storeu_si128((__m128i*)(dst + 3 * i_dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));
		}
		tmp += 4 * i_tmp;
		dst += 4 * i_dst;
		height -= 4;
	}
}

void uavs3d_if_hor_ver_luma_w4_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(32 + 7) * 4]);
    s16 *tmp = tmp_res;
    int shift = 12;
    __m256i mSwitch1 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12, 0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12);
    __m256i mSwitch2 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14, 2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14);
    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coef_x);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coef_x + 2));
    __m256i mCoefy3_hor = _mm256_set1_epi16(*(s16*)(coef_x + 4));
    __m256i mCoefy4_hor = _mm256_set1_epi16(*(s16*)(coef_x + 6));
    __m256i S0, S1, S2, S3; 
    __m256i T0, T1, T2, T3, T4, T5, T6, T7;
    __m256i r0, r1, r2, r3, r4, r5, r6, r7;
    __m256i mAddOffset = _mm256_set1_epi32(1 << 11);
    __m128i mCoefy11 = _mm_set1_epi16(*(s16*)coef_y);
    __m128i mCoefy22 = _mm_set1_epi16(*(s16*)(coef_y + 2));
    __m128i mCoefy33 = _mm_set1_epi16(*(s16*)(coef_y + 4));
    __m128i mCoefy44 = _mm_set1_epi16(*(s16*)(coef_y + 6));
    __m256i mCoefy1 = _mm256_cvtepi8_epi16(mCoefy11);
    __m256i mCoefy2 = _mm256_cvtepi8_epi16(mCoefy22);
    __m256i mCoefy3 = _mm256_cvtepi8_epi16(mCoefy33);
    __m256i mCoefy4 = _mm256_cvtepi8_epi16(mCoefy44);
    __m128i s0, s1, s2, s3;
    const int i_src2 = i_src << 1;
    const int i_src3 = i_src + i_src2;
    const int i_src4 = i_src << 2;

    src = src - 3 * i_src - 3;

    // hor
    // first 7 rows
    s0 = _mm_loadu_si128((__m128i*)(src));
    s1 = _mm_loadu_si128((__m128i*)(src + i_src));
    s2 = _mm_loadu_si128((__m128i*)(src + i_src2));

    S0 = _mm256_set_m128i(s2, s0);
    S1 = _mm256_set_m128i(s1, s1);

    S2 = _mm256_srli_si256(S0, 4);
    S3 = _mm256_srli_si256(S1, 4);

    T0 = _mm256_unpacklo_epi64(S0, S1);
    T1 = _mm256_unpacklo_epi64(S2, S3);

    r0 = _mm256_shuffle_epi8(T0, mSwitch1);
    r1 = _mm256_shuffle_epi8(T0, mSwitch2);
    r2 = _mm256_shuffle_epi8(T1, mSwitch1);
    r3 = _mm256_shuffle_epi8(T1, mSwitch2);

    T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
    T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
    T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
    T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

    T0 = _mm256_add_epi16(T0, T1);
    T1 = _mm256_add_epi16(T2, T3);
    T0 = _mm256_add_epi16(T0, T1);

    src += i_src3;

    _mm_storeu_si128((__m128i*)(tmp), _mm256_castsi256_si128(T0));
    _mm_storel_epi64((__m128i*)(tmp + 8), _mm256_extracti128_si256(T0, 1));

    s0 = _mm_loadu_si128((__m128i*)(src));
    s1 = _mm_loadu_si128((__m128i*)(src + i_src));
    s2 = _mm_loadu_si128((__m128i*)(src + i_src2));
    s3 = _mm_loadu_si128((__m128i*)(src + i_src3));

    S0 = _mm256_set_m128i(s2, s0);
    S1 = _mm256_set_m128i(s3, s1);

    S2 = _mm256_srli_si256(S0, 4);
    S3 = _mm256_srli_si256(S1, 4);

    T0 = _mm256_unpacklo_epi64(S0, S1);
    T1 = _mm256_unpacklo_epi64(S2, S3);

    r0 = _mm256_shuffle_epi8(T0, mSwitch1);
    r1 = _mm256_shuffle_epi8(T0, mSwitch2);
    r2 = _mm256_shuffle_epi8(T1, mSwitch1);
    r3 = _mm256_shuffle_epi8(T1, mSwitch2);

    T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
    T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
    T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
    T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

    T0 = _mm256_add_epi16(T0, T1);
    T1 = _mm256_add_epi16(T2, T3);
    T0 = _mm256_add_epi16(T0, T1);

    _mm256_storeu_si256((__m256i*)(tmp + 12), T0);

    src += i_src4;

    while (height > 0) {
        __m128i d0;
        // hor
        s0 = _mm_loadu_si128((__m128i*)(src));
        s1 = _mm_loadu_si128((__m128i*)(src + i_src));
        s2 = _mm_loadu_si128((__m128i*)(src + i_src2));
        s3 = _mm_loadu_si128((__m128i*)(src + i_src3));

        S0 = _mm256_set_m128i(s2, s0);
        S1 = _mm256_set_m128i(s3, s1);

        S2 = _mm256_srli_si256(S0, 4);
        S3 = _mm256_srli_si256(S1, 4);

        T0 = _mm256_unpacklo_epi64(S0, S1);
        T1 = _mm256_unpacklo_epi64(S2, S3);

        r0 = _mm256_shuffle_epi8(T0, mSwitch1);
        r1 = _mm256_shuffle_epi8(T0, mSwitch2);
        r2 = _mm256_shuffle_epi8(T1, mSwitch1);
        r3 = _mm256_shuffle_epi8(T1, mSwitch2);

        T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
        T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
        T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
        T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

        T0 = _mm256_add_epi16(T0, T1);
        T1 = _mm256_add_epi16(T2, T3);
        T7 = _mm256_add_epi16(T0, T1);
        _mm256_storeu_si256((__m256i*)(tmp + 28), T7);

        src += i_src4;

        // ver
        T0 = _mm256_load_si256((__m256i*)(tmp));
        T1 = _mm256_loadu_si256((__m256i*)(tmp + 4));
        T2 = _mm256_loadu_si256((__m256i*)(tmp + 8));
        T3 = _mm256_loadu_si256((__m256i*)(tmp + 12));
        T4 = _mm256_load_si256((__m256i*)(tmp + 16));
        T5 = _mm256_loadu_si256((__m256i*)(tmp + 20));
        T6 = _mm256_loadu_si256((__m256i*)(tmp + 24));
            
        r0 = _mm256_unpacklo_epi16(T0, T1);     // line0-1 line2-3
        r1 = _mm256_unpacklo_epi16(T2, T3);
        r2 = _mm256_unpacklo_epi16(T4, T5);
        r3 = _mm256_unpacklo_epi16(T6, T7);
        r4 = _mm256_unpackhi_epi16(T0, T1);     // line1-2 line3-4
        r5 = _mm256_unpackhi_epi16(T2, T3);
        r6 = _mm256_unpackhi_epi16(T4, T5);
        r7 = _mm256_unpackhi_epi16(T6, T7);

        T0 = _mm256_madd_epi16(r0, mCoefy1);
        T1 = _mm256_madd_epi16(r1, mCoefy2);
        T2 = _mm256_madd_epi16(r2, mCoefy3);
        T3 = _mm256_madd_epi16(r3, mCoefy4);
        T4 = _mm256_madd_epi16(r4, mCoefy1);
        T5 = _mm256_madd_epi16(r5, mCoefy2);
        T6 = _mm256_madd_epi16(r6, mCoefy3);
        T7 = _mm256_madd_epi16(r7, mCoefy4);

        T0 = _mm256_add_epi32(T0, T1);
        T2 = _mm256_add_epi32(T2, T3);
        T4 = _mm256_add_epi32(T4, T5);
        T6 = _mm256_add_epi32(T6, T7);
        T0 = _mm256_add_epi32(T0, T2);
        T4 = _mm256_add_epi32(T4, T6);
        T0 = _mm256_add_epi32(T0, mAddOffset);
        T4 = _mm256_add_epi32(T4, mAddOffset);
        T0 = _mm256_srai_epi32(T0, shift);    // dst line0, line2
        T4 = _mm256_srai_epi32(T4, shift);    // dst line1, line3

        T0 = _mm256_packs_epi32(T0, T4);   
        d0 = _mm_packus_epi16(_mm256_castsi256_si128(T0), _mm256_extracti128_si256(T0, 1));
            
        M32(dst) = _mm_extract_epi32(d0, 0);
        M32(dst + i_dst) = _mm_extract_epi32(d0, 1);
        M32(dst + i_dst * 2) = _mm_extract_epi32(d0, 2);
        M32(dst + i_dst * 3) = _mm_extract_epi32(d0, 3);
            
        tmp += 16;
        dst += i_dst << 2;
        height -= 4;
    }
}

void uavs3d_if_hor_ver_luma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    const int i_src2 = i_src << 1;
    
    __m256i T0, T1, T2, T3, T4, T5, T6, T7, T8, T9;
    __m256i r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;
    __m256i mVal[6];
    __m256i mSwitch1 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
    __m256i mSwitch2 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
    __m256i mSwitch3 = _mm256_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
    __m256i mSwitch4 = _mm256_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);
    __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coef_x);
    __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coef_x + 2));
    __m256i mCoefy3_hor = _mm256_set1_epi16(*(s16*)(coef_x + 4));
    __m256i mCoefy4_hor = _mm256_set1_epi16(*(s16*)(coef_x + 6));

    //HOR
    {
        int row;
        src = src - 3 * i_src - 3;

        // first row
        {
            __m128i mSrc0 = _mm_loadu_si128((__m128i*)(src));
            T0 = _mm256_set_m128i(mSrc0, mSrc0);
            src += i_src;

            uavs3d_prefetch(src, _MM_HINT_NTA);

            r0 = _mm256_shuffle_epi8(T0, mSwitch1);
            r1 = _mm256_shuffle_epi8(T0, mSwitch2);
            r2 = _mm256_shuffle_epi8(T0, mSwitch3);
            r3 = _mm256_shuffle_epi8(T0, mSwitch4);

            T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
            T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
            T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
            T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

            T0 = _mm256_add_epi16(T0, T1);
            T1 = _mm256_add_epi16(T2, T3);
            mVal[0] = _mm256_add_epi16(T0, T1);

            mVal[0] = _mm256_permute4x64_epi64(mVal[0], 0x44);

        }

        for (row = 1; row < 4; row++) {
            __m128i mSrc0 = _mm_loadu_si128((__m128i*)(src));
            __m128i mSrc1 = _mm_loadu_si128((__m128i*)(src + i_src));
            T0 = _mm256_set_m128i(mSrc1, mSrc0);
            src += i_src2;

            uavs3d_prefetch(src, _MM_HINT_NTA);
            uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

            r0 = _mm256_shuffle_epi8(T0, mSwitch1);
            r1 = _mm256_shuffle_epi8(T0, mSwitch2);
            r2 = _mm256_shuffle_epi8(T0, mSwitch3);
            r3 = _mm256_shuffle_epi8(T0, mSwitch4);

            T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
            T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
            T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
            T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

            T0 = _mm256_add_epi16(T0, T1);
            T1 = _mm256_add_epi16(T2, T3);
            mVal[row] = _mm256_add_epi16(T0, T1);
        }
    }
    
    {
        __m256i mAddOffset = _mm256_set1_epi32(1 << 11);
        __m128i mCoefy11 = _mm_set1_epi16(*(s16*)coef_y);
        __m128i mCoefy22 = _mm_set1_epi16(*(s16*)(coef_y + 2));
        __m128i mCoefy33 = _mm_set1_epi16(*(s16*)(coef_y + 4));
        __m128i mCoefy44 = _mm_set1_epi16(*(s16*)(coef_y + 6));
        __m256i mCoefy1 = _mm256_cvtepi8_epi16(mCoefy11);
        __m256i mCoefy2 = _mm256_cvtepi8_epi16(mCoefy22);
        __m256i mCoefy3 = _mm256_cvtepi8_epi16(mCoefy33);
        __m256i mCoefy4 = _mm256_cvtepi8_epi16(mCoefy44);
        const int shift = 12;

        while (height > 0) {
            __m128i s0, s1;
            //hor
            s0 = _mm_loadu_si128((__m128i*)(src));
            s1 = _mm_loadu_si128((__m128i*)(src + i_src));
            T0 = _mm256_set_m128i(s1, s0);

            src += i_src2;

            uavs3d_prefetch(src, _MM_HINT_NTA);
            uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

            r0 = _mm256_shuffle_epi8(T0, mSwitch1);
            r1 = _mm256_shuffle_epi8(T0, mSwitch2);
            r2 = _mm256_shuffle_epi8(T0, mSwitch3);
            r3 = _mm256_shuffle_epi8(T0, mSwitch4);

            T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
            T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
            T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
            T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

            s0 = _mm_loadu_si128((__m128i*)(src));
            s1 = _mm_loadu_si128((__m128i*)(src + i_src));

            T0 = _mm256_add_epi16(T0, T1);
            T1 = _mm256_add_epi16(T2, T3);
            mVal[4] = _mm256_add_epi16(T0, T1);

            src += i_src2;

            T0 = _mm256_set_m128i(s1, s0);

            uavs3d_prefetch(src, _MM_HINT_NTA);
            uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

            r0 = _mm256_shuffle_epi8(T0, mSwitch1);
            r1 = _mm256_shuffle_epi8(T0, mSwitch2);
            r2 = _mm256_shuffle_epi8(T0, mSwitch3);
            r3 = _mm256_shuffle_epi8(T0, mSwitch4);

            T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
            T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
            T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
            T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

            T0 = _mm256_add_epi16(T0, T1);
            T1 = _mm256_add_epi16(T2, T3);
            mVal[5] = _mm256_add_epi16(T0, T1);

            T0 = _mm256_permute2x128_si256(mVal[0], mVal[1], 0x21); 
            T1 = mVal[1];
            T2 = _mm256_permute2x128_si256(mVal[1], mVal[2], 0x21);
            T3 = mVal[2];
            T4 = _mm256_permute2x128_si256(mVal[2], mVal[3], 0x21);
            T5 = mVal[3];
            T6 = _mm256_permute2x128_si256(mVal[3], mVal[4], 0x21);
            T7 = mVal[4];
            T8 = _mm256_permute2x128_si256(mVal[4], mVal[5], 0x21);
            T9 = mVal[5];

            mVal[0] = mVal[2];
            mVal[1] = mVal[3];
            mVal[2] = mVal[4];
            mVal[3] = mVal[5];

            r0 = _mm256_unpacklo_epi16(T0, T1);
            r1 = _mm256_unpacklo_epi16(T2, T3);
            r2 = _mm256_unpacklo_epi16(T4, T5);
            r3 = _mm256_unpacklo_epi16(T6, T7);
            r5 = _mm256_unpackhi_epi16(T0, T1);
            r6 = _mm256_unpackhi_epi16(T2, T3);
            r7 = _mm256_unpackhi_epi16(T4, T5);
            r8 = _mm256_unpackhi_epi16(T6, T7);

            T0 = _mm256_madd_epi16(r0, mCoefy1);
            T1 = _mm256_madd_epi16(r1, mCoefy2);
            T2 = _mm256_madd_epi16(r2, mCoefy3);
            T3 = _mm256_madd_epi16(r3, mCoefy4);
            T4 = _mm256_madd_epi16(r5, mCoefy1);
            T5 = _mm256_madd_epi16(r6, mCoefy2);
            T6 = _mm256_madd_epi16(r7, mCoefy3);
            T7 = _mm256_madd_epi16(r8, mCoefy4);

            T0 = _mm256_add_epi32(T0, T1);
            T2 = _mm256_add_epi32(T2, T3);
            T4 = _mm256_add_epi32(T4, T5);
            T6 = _mm256_add_epi32(T6, T7);
            T0 = _mm256_add_epi32(T0, T2);
            T4 = _mm256_add_epi32(T4, T6);
            T0 = _mm256_add_epi32(T0, mAddOffset);
            T4 = _mm256_add_epi32(T4, mAddOffset);
            T0 = _mm256_srai_epi32(T0, shift);
            T4 = _mm256_srai_epi32(T4, shift);

            T0 = _mm256_packs_epi32(T0, T4);
            s0 = _mm_packus_epi16(_mm256_castsi256_si128(T0), _mm256_extracti128_si256(T0, 1));

            _mm_storel_epi64((__m128i*)(dst), s0);
            _mm_storeh_pi((__m64*)(dst + i_dst), _mm_castsi128_ps(s0));

            r4 = _mm256_unpacklo_epi16(T8, T9);
            r9 = _mm256_unpackhi_epi16(T8, T9);

            T0 = _mm256_madd_epi16(r1, mCoefy1);
            T1 = _mm256_madd_epi16(r2, mCoefy2);
            T2 = _mm256_madd_epi16(r3, mCoefy3);
            T3 = _mm256_madd_epi16(r4, mCoefy4);
            T4 = _mm256_madd_epi16(r6, mCoefy1);
            T5 = _mm256_madd_epi16(r7, mCoefy2);
            T6 = _mm256_madd_epi16(r8, mCoefy3);
            T7 = _mm256_madd_epi16(r9, mCoefy4);

            T0 = _mm256_add_epi32(T0, T1);
            T2 = _mm256_add_epi32(T2, T3);
            T4 = _mm256_add_epi32(T4, T5);
            T6 = _mm256_add_epi32(T6, T7);
            T0 = _mm256_add_epi32(T0, T2);
            T4 = _mm256_add_epi32(T4, T6);
            T0 = _mm256_add_epi32(T0, mAddOffset);
            T4 = _mm256_add_epi32(T4, mAddOffset);
            T0 = _mm256_srai_epi32(T0, shift);
            T4 = _mm256_srai_epi32(T4, shift);

            T0 = _mm256_packs_epi32(T0, T4);
            s0 = _mm_packus_epi16(_mm256_castsi256_si128(T0), _mm256_extracti128_si256(T0, 1));

            height -= 4;
            _mm_storel_epi64((__m128i*)(dst + i_dst * 2), s0);
            _mm_storeh_pi((__m64*)(dst + i_dst * 3), _mm_castsi128_ps(s0));

            dst += i_dst << 2;
        }
    }
}

void uavs3d_if_hor_ver_luma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(128 + 7) * 16]);
    s16 *tmp = tmp_res;
    __m256i mVal1, mVal2, mVal;
    __m256i T0, T1, T2, T3, T4, T5, T6, T7, T8, T9;
    __m256i S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10;
    __m256i r0, r1, r2, r3;
    __m256i sum, S;

    //HOR
    {
        int row;
        __m256i mSwitch1 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
        __m256i mSwitch2 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
        __m256i mSwitch3 = _mm256_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
        __m256i mSwitch4 = _mm256_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);

        __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coef_x);
        __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coef_x + 2));
        __m256i mCoefy3_hor = _mm256_set1_epi16(*(s16*)(coef_x + 4));
        __m256i mCoefy4_hor = _mm256_set1_epi16(*(s16*)(coef_x + 6));

        src = src - 3 * i_src - 3;

        row = height + 7;
        while (row--) {
            S = _mm256_loadu_si256((__m256i*)(src));
            src += i_src;
            S0 = _mm256_permute4x64_epi64(S, 0x94);
            uavs3d_prefetch(src, _MM_HINT_NTA);

            r0 = _mm256_shuffle_epi8(S0, mSwitch1);
            r1 = _mm256_shuffle_epi8(S0, mSwitch2);
            r2 = _mm256_shuffle_epi8(S0, mSwitch3);
            r3 = _mm256_shuffle_epi8(S0, mSwitch4);

            T0 = _mm256_maddubs_epi16(r0, mCoefy1_hor);
            T1 = _mm256_maddubs_epi16(r1, mCoefy2_hor);
            T2 = _mm256_maddubs_epi16(r2, mCoefy3_hor);
            T3 = _mm256_maddubs_epi16(r3, mCoefy4_hor);

            T0 = _mm256_add_epi16(T0, T1);
            T1 = _mm256_add_epi16(T2, T3);
            sum = _mm256_add_epi16(T0, T1);

            _mm256_store_si256((__m256i*)(tmp), sum);

            tmp += 16;
        }
    }
    // VER
    {
        const int i_tmp = 16;
        const int i_tmp2 = 32;
        const int i_tmp3 = 16 * 3;
        const int i_tmp4 = 16 * 4;
        const int i_tmp5 = 16 * 5;
        const int i_tmp6 = 16 * 6;
        const int i_tmp7 = 16 * 7;
        const int i_tmp8 = 16 * 8;
        const int i_tmp9 = 16 * 9;
        const int i_tmp10 = 16 * 10;
        int shift = 12;
        __m256i mAddOffset = _mm256_set1_epi32(1 << 11);
        __m128i mCoefy11 = _mm_set1_epi16(*(s16*)coef_y);
        __m128i mCoefy22 = _mm_set1_epi16(*(s16*)(coef_y + 2));
        __m128i mCoefy33 = _mm_set1_epi16(*(s16*)(coef_y + 4));
        __m128i mCoefy44 = _mm_set1_epi16(*(s16*)(coef_y + 6));
        __m256i mCoefy1 = _mm256_cvtepi8_epi16(mCoefy11);
        __m256i mCoefy2 = _mm256_cvtepi8_epi16(mCoefy22);
        __m256i mCoefy3 = _mm256_cvtepi8_epi16(mCoefy33);
        __m256i mCoefy4 = _mm256_cvtepi8_epi16(mCoefy44);

        tmp = tmp_res;

        while (height > 0) {
            __m256i r4, r5, r6, r7;
            S0 = _mm256_load_si256((__m256i*)(tmp));
            S1 = _mm256_load_si256((__m256i*)(tmp + i_tmp));
            S2 = _mm256_load_si256((__m256i*)(tmp + i_tmp2));
            S3 = _mm256_load_si256((__m256i*)(tmp + i_tmp3));
            S4 = _mm256_load_si256((__m256i*)(tmp + i_tmp4));
            S5 = _mm256_load_si256((__m256i*)(tmp + i_tmp5));
            S6 = _mm256_load_si256((__m256i*)(tmp + i_tmp6));
            S7 = _mm256_load_si256((__m256i*)(tmp + i_tmp7));
            S8 = _mm256_load_si256((__m256i*)(tmp + i_tmp8));
            S9 = _mm256_load_si256((__m256i*)(tmp + i_tmp9));
            S10 = _mm256_load_si256((__m256i*)(tmp + i_tmp10));

            T0 = _mm256_unpacklo_epi16(S0, S1);
            T1 = _mm256_unpacklo_epi16(S2, S3);
            T2 = _mm256_unpacklo_epi16(S4, S5);
            T3 = _mm256_unpacklo_epi16(S6, S7);
            T4 = _mm256_unpackhi_epi16(S0, S1);
            T5 = _mm256_unpackhi_epi16(S2, S3);
            T6 = _mm256_unpackhi_epi16(S4, S5);
            T7 = _mm256_unpackhi_epi16(S6, S7);

            r0 = _mm256_madd_epi16(T0, mCoefy1);
            r1 = _mm256_madd_epi16(T1, mCoefy2);
            r2 = _mm256_madd_epi16(T2, mCoefy3);
            r3 = _mm256_madd_epi16(T3, mCoefy4);
            r4 = _mm256_madd_epi16(T4, mCoefy1);
            r5 = _mm256_madd_epi16(T5, mCoefy2);
            r6 = _mm256_madd_epi16(T6, mCoefy3);
            r7 = _mm256_madd_epi16(T7, mCoefy4);

            mVal1 = _mm256_add_epi32(r0, r1);
            mVal2 = _mm256_add_epi32(r4, r5);
            mVal1 = _mm256_add_epi32(mVal1, r2);
            mVal2 = _mm256_add_epi32(mVal2, r6);
            mVal1 = _mm256_add_epi32(mVal1, r3);
            mVal2 = _mm256_add_epi32(mVal2, r7);

            mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
            mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
            mVal1 = _mm256_srai_epi32(mVal1, shift);
            mVal2 = _mm256_srai_epi32(mVal2, shift);

            mVal = _mm256_packs_epi32(mVal1, mVal2);
            _mm_storeu_si128((__m128i*)(dst), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

            T8 = _mm256_unpacklo_epi16(S8, S9);
            T9 = _mm256_unpackhi_epi16(S8, S9);

            T0 = _mm256_madd_epi16(T1, mCoefy1);
            T1 = _mm256_madd_epi16(T2, mCoefy2);
            T2 = _mm256_madd_epi16(T3, mCoefy3);
            T3 = _mm256_madd_epi16(T8, mCoefy4);
            T4 = _mm256_madd_epi16(T5, mCoefy1);
            T5 = _mm256_madd_epi16(T6, mCoefy2);
            T6 = _mm256_madd_epi16(T7, mCoefy3);
            T7 = _mm256_madd_epi16(T9, mCoefy4);

            mVal1 = _mm256_add_epi32(T0, T1);
            mVal2 = _mm256_add_epi32(T4, T5);
            mVal1 = _mm256_add_epi32(mVal1, T2);
            mVal2 = _mm256_add_epi32(mVal2, T6);
            mVal1 = _mm256_add_epi32(mVal1, T3);
            mVal2 = _mm256_add_epi32(mVal2, T7);

            mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
            mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
            mVal1 = _mm256_srai_epi32(mVal1, shift);
            mVal2 = _mm256_srai_epi32(mVal2, shift);

            mVal = _mm256_packs_epi32(mVal1, mVal2);
            _mm_storeu_si128((__m128i*)(dst + 2 * i_dst), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

            T0 = _mm256_unpacklo_epi16(S1, S2);
            T1 = _mm256_unpacklo_epi16(S3, S4);
            T2 = _mm256_unpacklo_epi16(S5, S6);
            T3 = _mm256_unpacklo_epi16(S7, S8);
            T4 = _mm256_unpackhi_epi16(S1, S2);
            T5 = _mm256_unpackhi_epi16(S3, S4);
            T6 = _mm256_unpackhi_epi16(S5, S6);
            T7 = _mm256_unpackhi_epi16(S7, S8);

            r0 = _mm256_madd_epi16(T0, mCoefy1);
            r1 = _mm256_madd_epi16(T1, mCoefy2);
            r2 = _mm256_madd_epi16(T2, mCoefy3);
            r3 = _mm256_madd_epi16(T3, mCoefy4);
            r4 = _mm256_madd_epi16(T4, mCoefy1);
            r5 = _mm256_madd_epi16(T5, mCoefy2);
            r6 = _mm256_madd_epi16(T6, mCoefy3);
            r7 = _mm256_madd_epi16(T7, mCoefy4);

            mVal1 = _mm256_add_epi32(r0, r1);
            mVal2 = _mm256_add_epi32(r4, r5);
            mVal1 = _mm256_add_epi32(mVal1, r2);
            mVal2 = _mm256_add_epi32(mVal2, r6);
            mVal1 = _mm256_add_epi32(mVal1, r3);
            mVal2 = _mm256_add_epi32(mVal2, r7);

            mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
            mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
            mVal1 = _mm256_srai_epi32(mVal1, shift);
            mVal2 = _mm256_srai_epi32(mVal2, shift);

            mVal = _mm256_packs_epi32(mVal1, mVal2);
            _mm_storeu_si128((__m128i*)(dst + i_dst), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

            T8 = _mm256_unpacklo_epi16(S9, S10);
            T9 = _mm256_unpackhi_epi16(S9, S10);

            T0 = _mm256_madd_epi16(T1, mCoefy1);
            T1 = _mm256_madd_epi16(T2, mCoefy2);
            T2 = _mm256_madd_epi16(T3, mCoefy3);
            T3 = _mm256_madd_epi16(T8, mCoefy4);
            T4 = _mm256_madd_epi16(T5, mCoefy1);
            T5 = _mm256_madd_epi16(T6, mCoefy2);
            T6 = _mm256_madd_epi16(T7, mCoefy3);
            T7 = _mm256_madd_epi16(T9, mCoefy4);

            mVal1 = _mm256_add_epi32(T0, T1);
            mVal2 = _mm256_add_epi32(T4, T5);
            mVal1 = _mm256_add_epi32(mVal1, T2);
            mVal2 = _mm256_add_epi32(mVal2, T6);
            mVal1 = _mm256_add_epi32(mVal1, T3);
            mVal2 = _mm256_add_epi32(mVal2, T7);

            mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
            mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
            mVal1 = _mm256_srai_epi32(mVal1, shift);
            mVal2 = _mm256_srai_epi32(mVal2, shift);

            mVal = _mm256_packs_epi32(mVal1, mVal2);
            _mm_storeu_si128((__m128i*)(dst + 3 * i_dst), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

            height -= 4;
            tmp += 4 * i_tmp;
            dst += 4 * i_dst;
        }
    }
}

void uavs3d_if_hor_ver_luma_w32_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(128 + 7) * 32]);
    s16 *tmp = tmp_res;
    const int i_tmp = 32;
    //HOR
    {
        int row;
        __m256i mSwitch1 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
        __m256i mSwitch2 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
        __m256i mSwitch3 = _mm256_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
        __m256i mSwitch4 = _mm256_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);

        __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coef_x);
        __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coef_x + 2));
        __m256i mCoefy3_hor = _mm256_set1_epi16(*(s16*)(coef_x + 4));
        __m256i mCoefy4_hor = _mm256_set1_epi16(*(s16*)(coef_x + 6));

        __m256i T0, T1, T2, T3, T4, T5, T6, T7;
        __m256i S0, S1;

        src = src - 3 * i_src - 3;

        row = height + 7;
        while (row--) {
            S0 = _mm256_loadu_si256((__m256i*)(src));
            S1 = _mm256_loadu_si256((__m256i*)(src + 8));
            src += i_src;
            uavs3d_prefetch(src, _MM_HINT_NTA);

            T0 = _mm256_shuffle_epi8(S0, mSwitch1);
            T1 = _mm256_shuffle_epi8(S0, mSwitch2);
            T2 = _mm256_shuffle_epi8(S0, mSwitch3);
            T3 = _mm256_shuffle_epi8(S0, mSwitch4);
            T4 = _mm256_shuffle_epi8(S1, mSwitch1);
            T5 = _mm256_shuffle_epi8(S1, mSwitch2);
            T6 = _mm256_shuffle_epi8(S1, mSwitch3);
            T7 = _mm256_shuffle_epi8(S1, mSwitch4);

            T0 = _mm256_maddubs_epi16(T0, mCoefy1_hor);
            T1 = _mm256_maddubs_epi16(T1, mCoefy2_hor);
            T2 = _mm256_maddubs_epi16(T2, mCoefy3_hor);
            T3 = _mm256_maddubs_epi16(T3, mCoefy4_hor);
            T4 = _mm256_maddubs_epi16(T4, mCoefy1_hor);
            T5 = _mm256_maddubs_epi16(T5, mCoefy2_hor);
            T6 = _mm256_maddubs_epi16(T6, mCoefy3_hor);
            T7 = _mm256_maddubs_epi16(T7, mCoefy4_hor);

            T0 = _mm256_add_epi16(T0, T1);
            T2 = _mm256_add_epi16(T2, T3);
            T4 = _mm256_add_epi16(T4, T5);
            T6 = _mm256_add_epi16(T6, T7);
            T0 = _mm256_add_epi16(T0, T2);
            T4 = _mm256_add_epi16(T4, T6);

            T1 = _mm256_permute2x128_si256(T0, T4, 0x20);
            T3 = _mm256_permute2x128_si256(T0, T4, 0x31);
            _mm256_store_si256((__m256i*)(tmp), T1);
            _mm256_store_si256((__m256i*)(tmp + 16), T3);

            tmp += i_tmp;
        }
    }

    // VER
    {
        const int i_tmp2 = 32 * 2;
        const int i_tmp3 = 32 * 3;
        const int i_tmp4 = 32 * 4;
        const int i_tmp5 = 32 * 5;
        const int i_tmp6 = 32 * 6;
        const int i_tmp7 = 32 * 7;;
        const int i_tmp8 = 32 * 8;
        const int i_tmp9 = 32 * 9;
        const int i_tmp10 = 32 * 10;
        int col;
        const int shift = 12;
        __m256i mAddOffset = _mm256_set1_epi32(1 << 11);
        __m128i mCoefy11 = _mm_set1_epi16(*(s16*)coef_y);
        __m128i mCoefy22 = _mm_set1_epi16(*(s16*)(coef_y + 2));
        __m128i mCoefy33 = _mm_set1_epi16(*(s16*)(coef_y + 4));
        __m128i mCoefy44 = _mm_set1_epi16(*(s16*)(coef_y + 6));
        __m256i mCoefy1 = _mm256_cvtepi8_epi16(mCoefy11);
        __m256i mCoefy2 = _mm256_cvtepi8_epi16(mCoefy22);
        __m256i mCoefy3 = _mm256_cvtepi8_epi16(mCoefy33);
        __m256i mCoefy4 = _mm256_cvtepi8_epi16(mCoefy44);
        __m256i mVal1, mVal2, mVal;
        __m256i T0, T1, T2, T3, T4, T5, T6, T7, T8, T9;
        __m256i S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10;
        __m256i r0, r1, r2, r3;

        tmp = tmp_res;

        while (height) {
            __m256i r4, r5, r6, r7;
            for (col = 0; col < width; col += 16)
            {
                S0 = _mm256_load_si256((__m256i*)(tmp + col));
                S1 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp));
                S2 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp2));
                S3 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp3));
                S4 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp4));
                S5 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp5));
                S6 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp6));
                S7 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp7));
                S8 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp8));
                S9 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp9));
                S10 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp10));

                T0 = _mm256_unpacklo_epi16(S0, S1);
                T1 = _mm256_unpacklo_epi16(S2, S3);
                T2 = _mm256_unpacklo_epi16(S4, S5);
                T3 = _mm256_unpacklo_epi16(S6, S7);
                T4 = _mm256_unpackhi_epi16(S0, S1);
                T5 = _mm256_unpackhi_epi16(S2, S3);
                T6 = _mm256_unpackhi_epi16(S4, S5);
                T7 = _mm256_unpackhi_epi16(S6, S7);

                r0 = _mm256_madd_epi16(T0, mCoefy1);
                r1 = _mm256_madd_epi16(T1, mCoefy2);
                r2 = _mm256_madd_epi16(T2, mCoefy3);
                r3 = _mm256_madd_epi16(T3, mCoefy4);
                r4 = _mm256_madd_epi16(T4, mCoefy1);
                r5 = _mm256_madd_epi16(T5, mCoefy2);
                r6 = _mm256_madd_epi16(T6, mCoefy3);
                r7 = _mm256_madd_epi16(T7, mCoefy4);

                mVal1 = _mm256_add_epi32(r0, r1);
                mVal2 = _mm256_add_epi32(r4, r5);
                mVal1 = _mm256_add_epi32(mVal1, r2);
                mVal2 = _mm256_add_epi32(mVal2, r6);
                mVal1 = _mm256_add_epi32(mVal1, r3);
                mVal2 = _mm256_add_epi32(mVal2, r7);

                mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
                mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
                mVal1 = _mm256_srai_epi32(mVal1, shift);
                mVal2 = _mm256_srai_epi32(mVal2, shift);

                mVal = _mm256_packs_epi32(mVal1, mVal2);
                _mm_storeu_si128((__m128i*)(dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

                T8 = _mm256_unpacklo_epi16(S8, S9);
                T9 = _mm256_unpackhi_epi16(S8, S9);

                T0 = _mm256_madd_epi16(T1, mCoefy1);
                T1 = _mm256_madd_epi16(T2, mCoefy2);
                T2 = _mm256_madd_epi16(T3, mCoefy3);
                T3 = _mm256_madd_epi16(T8, mCoefy4);
                T4 = _mm256_madd_epi16(T5, mCoefy1);
                T5 = _mm256_madd_epi16(T6, mCoefy2);
                T6 = _mm256_madd_epi16(T7, mCoefy3);
                T7 = _mm256_madd_epi16(T9, mCoefy4);

                mVal1 = _mm256_add_epi32(T0, T1);
                mVal2 = _mm256_add_epi32(T4, T5);
                mVal1 = _mm256_add_epi32(mVal1, T2);
                mVal2 = _mm256_add_epi32(mVal2, T6);
                mVal1 = _mm256_add_epi32(mVal1, T3);
                mVal2 = _mm256_add_epi32(mVal2, T7);

                mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
                mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
                mVal1 = _mm256_srai_epi32(mVal1, shift);
                mVal2 = _mm256_srai_epi32(mVal2, shift);

                mVal = _mm256_packs_epi32(mVal1, mVal2);
                _mm_storeu_si128((__m128i*)(dst + 2 * i_dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

                T0 = _mm256_unpacklo_epi16(S1, S2);
                T1 = _mm256_unpacklo_epi16(S3, S4);
                T2 = _mm256_unpacklo_epi16(S5, S6);
                T3 = _mm256_unpacklo_epi16(S7, S8);
                T4 = _mm256_unpackhi_epi16(S1, S2);
                T5 = _mm256_unpackhi_epi16(S3, S4);
                T6 = _mm256_unpackhi_epi16(S5, S6);
                T7 = _mm256_unpackhi_epi16(S7, S8);

                r0 = _mm256_madd_epi16(T0, mCoefy1);
                r1 = _mm256_madd_epi16(T1, mCoefy2);
                r2 = _mm256_madd_epi16(T2, mCoefy3);
                r3 = _mm256_madd_epi16(T3, mCoefy4);
                r4 = _mm256_madd_epi16(T4, mCoefy1);
                r5 = _mm256_madd_epi16(T5, mCoefy2);
                r6 = _mm256_madd_epi16(T6, mCoefy3);
                r7 = _mm256_madd_epi16(T7, mCoefy4);

                mVal1 = _mm256_add_epi32(r0, r1);
                mVal2 = _mm256_add_epi32(r4, r5);
                mVal1 = _mm256_add_epi32(mVal1, r2);
                mVal2 = _mm256_add_epi32(mVal2, r6);
                mVal1 = _mm256_add_epi32(mVal1, r3);
                mVal2 = _mm256_add_epi32(mVal2, r7);

                mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
                mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
                mVal1 = _mm256_srai_epi32(mVal1, shift);
                mVal2 = _mm256_srai_epi32(mVal2, shift);

                mVal = _mm256_packs_epi32(mVal1, mVal2);
                _mm_storeu_si128((__m128i*)(dst + i_dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

                T8 = _mm256_unpacklo_epi16(S9, S10);
                T9 = _mm256_unpackhi_epi16(S9, S10);

                T0 = _mm256_madd_epi16(T1, mCoefy1);
                T1 = _mm256_madd_epi16(T2, mCoefy2);
                T2 = _mm256_madd_epi16(T3, mCoefy3);
                T3 = _mm256_madd_epi16(T8, mCoefy4);
                T4 = _mm256_madd_epi16(T5, mCoefy1);
                T5 = _mm256_madd_epi16(T6, mCoefy2);
                T6 = _mm256_madd_epi16(T7, mCoefy3);
                T7 = _mm256_madd_epi16(T9, mCoefy4);

                mVal1 = _mm256_add_epi32(T0, T1);
                mVal2 = _mm256_add_epi32(T4, T5);
                mVal1 = _mm256_add_epi32(mVal1, T2);
                mVal2 = _mm256_add_epi32(mVal2, T6);
                mVal1 = _mm256_add_epi32(mVal1, T3);
                mVal2 = _mm256_add_epi32(mVal2, T7);

                mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
                mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
                mVal1 = _mm256_srai_epi32(mVal1, shift);
                mVal2 = _mm256_srai_epi32(mVal2, shift);

                mVal = _mm256_packs_epi32(mVal1, mVal2);
                _mm_storeu_si128((__m128i*)(dst + 3 * i_dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));
            }
            tmp += 4 * i_tmp;
            dst += 4 * i_dst;
            height -= 4;
        }
    }
}

void uavs3d_if_hor_ver_luma_w32x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
	ALIGNED_32(s16 tmp_res[(128 + 7) * 128]);
	s16 *tmp = tmp_res;
    const int i_tmp = width;
    __m256i mVal1, mVal2, mVal;
    __m256i T0, T1, T2, T3, T4, T5, T6, T7, T8, T9;
    __m256i S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10;
	__m256i r0, r1, r2, r3;

	//HOR
    {
        int row, col;
        __m256i mSwitch1 = _mm256_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
        __m256i mSwitch2 = _mm256_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
        __m256i mSwitch3 = _mm256_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
        __m256i mSwitch4 = _mm256_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);

        __m256i mCoefy1_hor = _mm256_set1_epi16(*(s16*)coef_x);
        __m256i mCoefy2_hor = _mm256_set1_epi16(*(s16*)(coef_x + 2));
        __m256i mCoefy3_hor = _mm256_set1_epi16(*(s16*)(coef_x + 4));
        __m256i mCoefy4_hor = _mm256_set1_epi16(*(s16*)(coef_x + 6));

        src = src - 3 * i_src - 3;

        row = height + 7;
        while (row--) {
            uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
            for (col = 0; col < width; col += 32)
            {
                S0 = _mm256_loadu_si256((__m256i*)(src + col));
                S1 = _mm256_loadu_si256((__m256i*)(src + col + 8));

                T0 = _mm256_shuffle_epi8(S0, mSwitch1);
                T1 = _mm256_shuffle_epi8(S0, mSwitch2);
                T2 = _mm256_shuffle_epi8(S0, mSwitch3);
                T3 = _mm256_shuffle_epi8(S0, mSwitch4);
                T4 = _mm256_shuffle_epi8(S1, mSwitch1);
                T5 = _mm256_shuffle_epi8(S1, mSwitch2);
                T6 = _mm256_shuffle_epi8(S1, mSwitch3);
                T7 = _mm256_shuffle_epi8(S1, mSwitch4);

                T0 = _mm256_maddubs_epi16(T0, mCoefy1_hor);
                T1 = _mm256_maddubs_epi16(T1, mCoefy2_hor);
                T2 = _mm256_maddubs_epi16(T2, mCoefy3_hor);
                T3 = _mm256_maddubs_epi16(T3, mCoefy4_hor);
                T4 = _mm256_maddubs_epi16(T4, mCoefy1_hor);
                T5 = _mm256_maddubs_epi16(T5, mCoefy2_hor);
                T6 = _mm256_maddubs_epi16(T6, mCoefy3_hor);
                T7 = _mm256_maddubs_epi16(T7, mCoefy4_hor);

                T0 = _mm256_add_epi16(T0, T1);
                T2 = _mm256_add_epi16(T2, T3);
                T4 = _mm256_add_epi16(T4, T5);
                T6 = _mm256_add_epi16(T6, T7);
                T0 = _mm256_add_epi16(T0, T2);
                T4 = _mm256_add_epi16(T4, T6);

                T1 = _mm256_permute2x128_si256(T0, T4, 0x20);
                T3 = _mm256_permute2x128_si256(T0, T4, 0x31);
                _mm256_store_si256((__m256i*)(tmp + col), T1);
                _mm256_store_si256((__m256i*)(tmp + col + 16), T3);
            }
            src += i_src;
            tmp += i_tmp;
        }
    }

	// VER
    {
        const int i_tmp2 = width << 1;
        const int i_tmp3 = width + i_tmp2;
        const int i_tmp4 = width << 2;
        const int i_tmp5 = i_tmp2 + i_tmp3;
        const int i_tmp6 = i_tmp3 << 1;
        const int i_tmp7 = i_tmp4 + i_tmp3;;
        const int i_tmp8 = width << 3;
        const int i_tmp9 = i_tmp4 + i_tmp5;;
        const int i_tmp10 = i_tmp5 << 1;

        int shift = 12;
        int col;
        __m256i mAddOffset = _mm256_set1_epi32(1 << 11);
        __m128i mCoefy11 = _mm_set1_epi16(*(s16*)coef_y);
        __m128i mCoefy22 = _mm_set1_epi16(*(s16*)(coef_y + 2));
        __m128i mCoefy33 = _mm_set1_epi16(*(s16*)(coef_y + 4));
        __m128i mCoefy44 = _mm_set1_epi16(*(s16*)(coef_y + 6));
        __m256i mCoefy1 = _mm256_cvtepi8_epi16(mCoefy11);
        __m256i mCoefy2 = _mm256_cvtepi8_epi16(mCoefy22);
        __m256i mCoefy3 = _mm256_cvtepi8_epi16(mCoefy33);
        __m256i mCoefy4 = _mm256_cvtepi8_epi16(mCoefy44);

        tmp = tmp_res;

        while (height) {
            __m256i r4, r5, r6, r7;
            for (col = 0; col < width; col += 16)
            {
                S0 = _mm256_load_si256((__m256i*)(tmp + col));
                S1 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp));
                S2 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp2));
                S3 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp3));
                S4 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp4));
                S5 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp5));
                S6 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp6));
                S7 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp7));
                S8 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp8));
                S9 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp9));
                S10 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp10));

                T0 = _mm256_unpacklo_epi16(S0, S1);
                T1 = _mm256_unpacklo_epi16(S2, S3);
                T2 = _mm256_unpacklo_epi16(S4, S5);
                T3 = _mm256_unpacklo_epi16(S6, S7);
                T4 = _mm256_unpackhi_epi16(S0, S1);
                T5 = _mm256_unpackhi_epi16(S2, S3);
                T6 = _mm256_unpackhi_epi16(S4, S5);
                T7 = _mm256_unpackhi_epi16(S6, S7);

                r0 = _mm256_madd_epi16(T0, mCoefy1);
                r1 = _mm256_madd_epi16(T1, mCoefy2);
                r2 = _mm256_madd_epi16(T2, mCoefy3);
                r3 = _mm256_madd_epi16(T3, mCoefy4);
                r4 = _mm256_madd_epi16(T4, mCoefy1);
                r5 = _mm256_madd_epi16(T5, mCoefy2);
                r6 = _mm256_madd_epi16(T6, mCoefy3);
                r7 = _mm256_madd_epi16(T7, mCoefy4);

                mVal1 = _mm256_add_epi32(r0, r1);
                mVal2 = _mm256_add_epi32(r4, r5);
                mVal1 = _mm256_add_epi32(mVal1, r2);
                mVal2 = _mm256_add_epi32(mVal2, r6);
                mVal1 = _mm256_add_epi32(mVal1, r3);
                mVal2 = _mm256_add_epi32(mVal2, r7);

                mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
                mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
                mVal1 = _mm256_srai_epi32(mVal1, shift);
                mVal2 = _mm256_srai_epi32(mVal2, shift);

                mVal = _mm256_packs_epi32(mVal1, mVal2);
                _mm_storeu_si128((__m128i*)(dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

                T8 = _mm256_unpacklo_epi16(S8, S9);
                T9 = _mm256_unpackhi_epi16(S8, S9);

                T0 = _mm256_madd_epi16(T1, mCoefy1);
                T1 = _mm256_madd_epi16(T2, mCoefy2);
                T2 = _mm256_madd_epi16(T3, mCoefy3);
                T3 = _mm256_madd_epi16(T8, mCoefy4);
                T4 = _mm256_madd_epi16(T5, mCoefy1);
                T5 = _mm256_madd_epi16(T6, mCoefy2);
                T6 = _mm256_madd_epi16(T7, mCoefy3);
                T7 = _mm256_madd_epi16(T9, mCoefy4);

                mVal1 = _mm256_add_epi32(T0, T1);
                mVal2 = _mm256_add_epi32(T4, T5);
                mVal1 = _mm256_add_epi32(mVal1, T2);
                mVal2 = _mm256_add_epi32(mVal2, T6);
                mVal1 = _mm256_add_epi32(mVal1, T3);
                mVal2 = _mm256_add_epi32(mVal2, T7);

                mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
                mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
                mVal1 = _mm256_srai_epi32(mVal1, shift);
                mVal2 = _mm256_srai_epi32(mVal2, shift);

                mVal = _mm256_packs_epi32(mVal1, mVal2);
                _mm_storeu_si128((__m128i*)(dst + 2 * i_dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

                T0 = _mm256_unpacklo_epi16(S1, S2);
                T1 = _mm256_unpacklo_epi16(S3, S4);
                T2 = _mm256_unpacklo_epi16(S5, S6);
                T3 = _mm256_unpacklo_epi16(S7, S8);
                T4 = _mm256_unpackhi_epi16(S1, S2);
                T5 = _mm256_unpackhi_epi16(S3, S4);
                T6 = _mm256_unpackhi_epi16(S5, S6);
                T7 = _mm256_unpackhi_epi16(S7, S8);

                r0 = _mm256_madd_epi16(T0, mCoefy1);
                r1 = _mm256_madd_epi16(T1, mCoefy2);
                r2 = _mm256_madd_epi16(T2, mCoefy3);
                r3 = _mm256_madd_epi16(T3, mCoefy4);
                r4 = _mm256_madd_epi16(T4, mCoefy1);
                r5 = _mm256_madd_epi16(T5, mCoefy2);
                r6 = _mm256_madd_epi16(T6, mCoefy3);
                r7 = _mm256_madd_epi16(T7, mCoefy4);

                mVal1 = _mm256_add_epi32(r0, r1);
                mVal2 = _mm256_add_epi32(r4, r5);
                mVal1 = _mm256_add_epi32(mVal1, r2);
                mVal2 = _mm256_add_epi32(mVal2, r6);
                mVal1 = _mm256_add_epi32(mVal1, r3);
                mVal2 = _mm256_add_epi32(mVal2, r7);

                mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
                mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
                mVal1 = _mm256_srai_epi32(mVal1, shift);
                mVal2 = _mm256_srai_epi32(mVal2, shift);

                mVal = _mm256_packs_epi32(mVal1, mVal2);
                _mm_storeu_si128((__m128i*)(dst + i_dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));

                T8 = _mm256_unpacklo_epi16(S9, S10);
                T9 = _mm256_unpackhi_epi16(S9, S10);

                T0 = _mm256_madd_epi16(T1, mCoefy1);
                T1 = _mm256_madd_epi16(T2, mCoefy2);
                T2 = _mm256_madd_epi16(T3, mCoefy3);
                T3 = _mm256_madd_epi16(T8, mCoefy4);
                T4 = _mm256_madd_epi16(T5, mCoefy1);
                T5 = _mm256_madd_epi16(T6, mCoefy2);
                T6 = _mm256_madd_epi16(T7, mCoefy3);
                T7 = _mm256_madd_epi16(T9, mCoefy4);

                mVal1 = _mm256_add_epi32(T0, T1);
                mVal2 = _mm256_add_epi32(T4, T5);
                mVal1 = _mm256_add_epi32(mVal1, T2);
                mVal2 = _mm256_add_epi32(mVal2, T6);
                mVal1 = _mm256_add_epi32(mVal1, T3);
                mVal2 = _mm256_add_epi32(mVal2, T7);

                mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
                mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
                mVal1 = _mm256_srai_epi32(mVal1, shift);
                mVal2 = _mm256_srai_epi32(mVal2, shift);

                mVal = _mm256_packs_epi32(mVal1, mVal2);
                _mm_storeu_si128((__m128i*)(dst + 3 * i_dst + col), _mm_packus_epi16(_mm256_castsi256_si128(mVal), _mm256_extracti128_si256(mVal, 1)));
            }
            tmp += 4 * i_tmp;
            dst += 4 * i_dst;
            height -= 4;
        }
    }
}

#else

void uavs3d_if_cpy_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    int i_src3 = i_src2 + i_src;
    int i_dst3 = i_dst2 + i_dst;
    int i_src4 = i_src << 2;
    int i_dst4 = i_dst << 2;
    __m256i m0, m1, m2, m3;
    while (height > 0) {
        m0 = _mm256_loadu_si256((const __m256i*)(src));
        m1 = _mm256_loadu_si256((const __m256i*)(src + i_src));
        m2 = _mm256_loadu_si256((const __m256i*)(src + i_src2));
        m3 = _mm256_loadu_si256((const __m256i*)(src + i_src3));
        _mm256_storeu_si256((__m256i*)dst, m0);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), m1);
        _mm256_storeu_si256((__m256i*)(dst + i_dst2), m2);
        _mm256_storeu_si256((__m256i*)(dst + i_dst3), m3);
        src += i_src4;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_if_cpy_w32_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    __m256i m0, m1, m2, m3;
    while (height > 0) {
        m0 = _mm256_loadu_si256((const __m256i*)(src));
        m1 = _mm256_loadu_si256((const __m256i*)(src + 16));
        m2 = _mm256_loadu_si256((const __m256i*)(src + i_src));
        m3 = _mm256_loadu_si256((const __m256i*)(src + i_src + 16));
        _mm256_storeu_si256((__m256i*)dst, m0);
        _mm256_storeu_si256((__m256i*)(dst + 16), m1);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), m2);
        _mm256_storeu_si256((__m256i*)(dst + i_dst + 16), m3);
        src += i_src2;
        dst += i_dst2;
        height -= 2;
    }
}

void uavs3d_if_cpy_w64_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    __m256i m0, m1, m2, m3, m4, m5, m6, m7;
    while (height) {
        m0 = _mm256_loadu_si256((const __m256i*)(src));
        m1 = _mm256_loadu_si256((const __m256i*)(src + 16));
        m2 = _mm256_loadu_si256((const __m256i*)(src + 32));
        m3 = _mm256_loadu_si256((const __m256i*)(src + 48));
        m4 = _mm256_loadu_si256((const __m256i*)(src + i_src));
        m5 = _mm256_loadu_si256((const __m256i*)(src + i_src + 16));
        m6 = _mm256_loadu_si256((const __m256i*)(src + i_src + 32));
        m7 = _mm256_loadu_si256((const __m256i*)(src + i_src + 48));

        _mm256_storeu_si256((__m256i*)(dst), m0);
        _mm256_storeu_si256((__m256i*)(dst + 16), m1);
        _mm256_storeu_si256((__m256i*)(dst + 32), m2);
        _mm256_storeu_si256((__m256i*)(dst + 48), m3);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), m4);
        _mm256_storeu_si256((__m256i*)(dst + i_dst + 16), m5);
        _mm256_storeu_si256((__m256i*)(dst + i_dst + 32), m6);
        _mm256_storeu_si256((__m256i*)(dst + i_dst + 48), m7);

        height -= 2;
        src += i_src2;
        dst += i_dst2;
    }
}

void uavs3d_if_cpy_w128_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    __m256i m0, m1, m2, m3, m4, m5, m6, m7;
    while (height) {
        m0 = _mm256_loadu_si256((const __m256i*)(src));
        m1 = _mm256_loadu_si256((const __m256i*)(src + 16));
        m2 = _mm256_loadu_si256((const __m256i*)(src + 32));
        m3 = _mm256_loadu_si256((const __m256i*)(src + 48));
        m4 = _mm256_loadu_si256((const __m256i*)(src + 64));
        m5 = _mm256_loadu_si256((const __m256i*)(src + 80));
        m6 = _mm256_loadu_si256((const __m256i*)(src + 96));
        m7 = _mm256_loadu_si256((const __m256i*)(src + 112));

        _mm256_storeu_si256((__m256i*)(dst), m0);
        _mm256_storeu_si256((__m256i*)(dst + 16), m1);
        _mm256_storeu_si256((__m256i*)(dst + 32), m2);
        _mm256_storeu_si256((__m256i*)(dst + 48), m3);
        _mm256_storeu_si256((__m256i*)(dst + 64), m4);
        _mm256_storeu_si256((__m256i*)(dst + 80), m5);
        _mm256_storeu_si256((__m256i*)(dst + 96), m6);
        _mm256_storeu_si256((__m256i*)(dst + 112), m7);

        height--;
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_luma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    __m256i T0, T1, T2, T3, T4, T5;
    __m256i M0, M1, M2, M3, M4, M5, M6, M7;
    __m256i S0, S1, S2;
    __m256i offset = _mm256_set1_epi32(32);
    __m256i mShuffle0 = _mm256_setr_epi8(0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9, 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9);
    __m256i mShuffle1 = _mm256_setr_epi8(4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13, 4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13);
    __m256i mCoef0 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[0]));
    __m256i mCoef1 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[1]));
    __m256i mCoef2 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[2]));
    __m256i mCoef3 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[3]));
    __m128i s0, s1;

    src -= 3;

    while (height) {
        T0 = _mm256_loadu_si256((__m256i*)(src));
        s0 = _mm_loadu_si128((__m128i*)(src + 4));
        T1 = _mm256_loadu_si256((__m256i*)(src + i_src));
        s1 = _mm_loadu_si128((__m128i*)(src + i_src + 4));
        height -= 2;
        src += i_src << 1;
        uavs3d_prefetch(src, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        S0 = _mm256_permute2x128_si256(T0, T1, 0x20);
        S2 = _mm256_permute2x128_si256(T0, T1, 0x31);
        S1 = _mm256_set_m128i(s1, s0);

        T0 = _mm256_shuffle_epi8(S0, mShuffle0);
        T1 = _mm256_shuffle_epi8(S0, mShuffle1);
        T2 = _mm256_shuffle_epi8(S1, mShuffle0);
        T3 = _mm256_shuffle_epi8(S1, mShuffle1);
        T4 = _mm256_shuffle_epi8(S2, mShuffle0);
        T5 = _mm256_shuffle_epi8(S2, mShuffle1);

        M0 = _mm256_madd_epi16(T0, mCoef0);
        M1 = _mm256_madd_epi16(T1, mCoef1);
        M2 = _mm256_madd_epi16(T2, mCoef2);
        M3 = _mm256_madd_epi16(T3, mCoef3);
        M4 = _mm256_madd_epi16(T2, mCoef0);
        M5 = _mm256_madd_epi16(T3, mCoef1);
        M6 = _mm256_madd_epi16(T4, mCoef2);
        M7 = _mm256_madd_epi16(T5, mCoef3);

        M0 = _mm256_add_epi32(M0, M1);
        M1 = _mm256_add_epi32(M2, M3);
        M2 = _mm256_add_epi32(M4, M5);
        M3 = _mm256_add_epi32(M6, M7);

        M0 = _mm256_add_epi32(M0, M1);
        M1 = _mm256_add_epi32(M2, M3);

        M2 = _mm256_add_epi32(M0, offset);
        M3 = _mm256_add_epi32(M1, offset);
        M2 = _mm256_srai_epi32(M2, 6);
        M3 = _mm256_srai_epi32(M3, 6);
        M2 = _mm256_packus_epi32(M2, M3);
        M2 = _mm256_min_epu16(M2, max_pel);

        _mm_storeu_si128((__m128i*)(dst), _mm256_castsi256_si128(M2));
        _mm_storeu_si128((__m128i*)(dst + i_dst), _mm256_extracti128_si256(M2, 1));

        dst += i_dst << 1;
    }
}

void uavs3d_if_hor_luma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    __m256i T0, T1, T2, T3, T4, T5;
    __m256i M0, M1, M2, M3, M4, M5, M6, M7;
    __m256i S0, S1, S2;
    __m256i offset = _mm256_set1_epi32(32);
    __m256i mShuffle0 = _mm256_setr_epi8(0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9, 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9);
    __m256i mShuffle1 = _mm256_setr_epi8(4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13, 4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13);
    __m256i mCoef0 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[0]));
    __m256i mCoef1 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[1]));
    __m256i mCoef2 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[2]));
    __m256i mCoef3 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[3]));

    src -= 3;

    while (height--) {
        S0 = _mm256_lddqu_si256((__m256i*)(src));
        S1 = _mm256_loadu_si256((__m256i*)(src + 4));
        S2 = _mm256_loadu_si256((__m256i*)(src + 8));

        src += i_src;
        T0 = _mm256_shuffle_epi8(S0, mShuffle0);
        T1 = _mm256_shuffle_epi8(S0, mShuffle1);
        T2 = _mm256_shuffle_epi8(S1, mShuffle0);
        T3 = _mm256_shuffle_epi8(S1, mShuffle1);
        T4 = _mm256_shuffle_epi8(S2, mShuffle0);
        T5 = _mm256_shuffle_epi8(S2, mShuffle1);
        uavs3d_prefetch(src, _MM_HINT_NTA);

        M0 = _mm256_madd_epi16(T0, mCoef0);
        M1 = _mm256_madd_epi16(T1, mCoef1);
        M2 = _mm256_madd_epi16(T2, mCoef2);
        M3 = _mm256_madd_epi16(T3, mCoef3);
        M4 = _mm256_madd_epi16(T2, mCoef0);
        M5 = _mm256_madd_epi16(T3, mCoef1);
        M6 = _mm256_madd_epi16(T4, mCoef2);
        M7 = _mm256_madd_epi16(T5, mCoef3);

        M0 = _mm256_add_epi32(M0, M1);
        M1 = _mm256_add_epi32(M2, M3);
        M2 = _mm256_add_epi32(M4, M5);
        M3 = _mm256_add_epi32(M6, M7);

        M0 = _mm256_add_epi32(M0, M1);
        M1 = _mm256_add_epi32(M2, M3);

        M2 = _mm256_add_epi32(M0, offset);
        M3 = _mm256_add_epi32(M1, offset);
        M2 = _mm256_srai_epi32(M2, 6);
        M3 = _mm256_srai_epi32(M3, 6);
        M2 = _mm256_packus_epi32(M2, M3);
        M2 = _mm256_min_epu16(M2, max_pel);

        _mm256_storeu_si256((__m256i*)(dst), M2);

        dst += i_dst;
    }
}

void uavs3d_if_hor_luma_w16x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int col;
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    __m256i T0, T1, T2, T3, T4, T5;
    __m256i M0, M1, M2, M3, M4, M5, M6, M7;
    __m256i S0, S1, S2;
    __m256i offset = _mm256_set1_epi32(32);
    __m256i mShuffle0 = _mm256_setr_epi8(0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9, 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9);
    __m256i mShuffle1 = _mm256_setr_epi8(4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13, 4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13);
    __m256i mCoef0 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[0]));
    __m256i mCoef1 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[1]));
    __m256i mCoef2 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[2]));
    __m256i mCoef3 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coeff)[3]));

    src -= 3;

    while (height--) {
        const pel *p_src = src;
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 16)
        {
            S0 = _mm256_loadu_si256((__m256i*)(p_src));
            S1 = _mm256_loadu_si256((__m256i*)(p_src + 4));
            S2 = _mm256_loadu_si256((__m256i*)(p_src + 8));

            T0 = _mm256_shuffle_epi8(S0, mShuffle0);
            T1 = _mm256_shuffle_epi8(S0, mShuffle1);
            T2 = _mm256_shuffle_epi8(S1, mShuffle0);
            T3 = _mm256_shuffle_epi8(S1, mShuffle1);
            T4 = _mm256_shuffle_epi8(S2, mShuffle0);
            T5 = _mm256_shuffle_epi8(S2, mShuffle1);

            M0 = _mm256_madd_epi16(T0, mCoef0);
            M1 = _mm256_madd_epi16(T1, mCoef1);
            M2 = _mm256_madd_epi16(T2, mCoef2);
            M3 = _mm256_madd_epi16(T3, mCoef3);
            M4 = _mm256_madd_epi16(T2, mCoef0);
            M5 = _mm256_madd_epi16(T3, mCoef1);
            M6 = _mm256_madd_epi16(T4, mCoef2);
            M7 = _mm256_madd_epi16(T5, mCoef3);

            M0 = _mm256_add_epi32(M0, M1);
            M1 = _mm256_add_epi32(M2, M3);
            M2 = _mm256_add_epi32(M4, M5);
            M3 = _mm256_add_epi32(M6, M7);

            M0 = _mm256_add_epi32(M0, M1);
            M1 = _mm256_add_epi32(M2, M3);

            M2 = _mm256_add_epi32(M0, offset);
            M3 = _mm256_add_epi32(M1, offset);
            M2 = _mm256_srai_epi32(M2, 6);
            M3 = _mm256_srai_epi32(M3, 6);
            M2 = _mm256_packus_epi32(M2, M3);
            M2 = _mm256_min_epu16(M2, max_pel);

            p_src += 16;
            _mm256_storeu_si256((__m256i*)(dst + col), M2);
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_chroma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;

    __m128i coef0 = _mm_cvtepi8_epi16(_mm_set1_epi16(((s16*)coeff)[0]));
    __m128i coef1 = _mm_cvtepi8_epi16(_mm_set1_epi16(((s16*)coeff)[1]));
    __m256i mCoef0 = _mm256_set_m128i(coef1, coef0);
    __m256i mCoef1 = _mm256_set_m128i(coef0, coef1);
    __m256i mSwitch = _mm256_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11, 0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11);
    __m256i mAddOffset = _mm256_set1_epi32((s16)offset);
    __m256i T0, T1, S0, S1;
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    __m128i s0;

    src -= 2;

    while (height--) {
        uavs3d_prefetch(src + i_src * 2, _MM_HINT_NTA);
        S0 = _mm256_loadu_si256((__m256i*)(src));
        s0 = _mm_loadu_si128((__m128i*)(src + 4));
        src += i_src;
        S1 = _mm256_set_m128i(s0, s0);
        uavs3d_prefetch(src, _MM_HINT_NTA);
        T0 = _mm256_shuffle_epi8(S0, mSwitch);
        T1 = _mm256_shuffle_epi8(S1, mSwitch);
        T0 = _mm256_madd_epi16(T0, mCoef0);
        T1 = _mm256_madd_epi16(T1, mCoef1);
        T0 = _mm256_add_epi32(T0, T1);

        T0 = _mm256_add_epi32(T0, mAddOffset);
        T0 = _mm256_srai_epi32(T0, shift);
        T0 = _mm256_min_epu16(T0, max_pel);
        s0 = _mm_packus_epi32(_mm256_castsi256_si128(T0), _mm256_extracti128_si256(T0, 1));

        _mm_storeu_si128((__m128i*)(dst), s0);

        dst += i_dst;
    }
}

void uavs3d_if_hor_chroma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;

    __m128i coef0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coef1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m256i mCoef0 = _mm256_cvtepi8_epi16(coef0);
    __m256i mCoef1 = _mm256_cvtepi8_epi16(coef1);
    __m256i mSwitch = _mm256_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11, 0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11);
    __m256i mAddOffset = _mm256_set1_epi32((s16)offset);
    __m256i T0, T1, T2, T3, S0, S1, S2;
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);

    src -= 2;

    while (height--) {
        uavs3d_prefetch(src + i_src * 2, _MM_HINT_NTA);
        S0 = _mm256_loadu_si256((__m256i*)(src));
        S1 = _mm256_loadu_si256((__m256i*)(src + 4));
        S2 = _mm256_loadu_si256((__m256i*)(src + 8));
        T0 = _mm256_shuffle_epi8(S0, mSwitch);
        T1 = _mm256_shuffle_epi8(S1, mSwitch);
        T2 = _mm256_shuffle_epi8(S1, mSwitch);
        T3 = _mm256_shuffle_epi8(S2, mSwitch);
        T0 = _mm256_madd_epi16(T0, mCoef0);
        T1 = _mm256_madd_epi16(T1, mCoef1);
        T2 = _mm256_madd_epi16(T2, mCoef0);
        T3 = _mm256_madd_epi16(T3, mCoef1);
        T0 = _mm256_add_epi32(T0, T1);
        T2 = _mm256_add_epi32(T2, T3);

        T0 = _mm256_add_epi32(T0, mAddOffset);
        T2 = _mm256_add_epi32(T2, mAddOffset);
        T0 = _mm256_srai_epi32(T0, shift);
        T2 = _mm256_srai_epi32(T2, shift);
        T0 = _mm256_packus_epi32(T0, T2);

        T0 = _mm256_min_epu16(T0, max_pel);
        _mm256_storeu_si256((__m256i*)(dst), T0);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_chroma_w16x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int col;
    const int offset = 32;
    const int shift = 6;

    __m128i coef0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coef1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m256i mCoef0 = _mm256_cvtepi8_epi16(coef0);
    __m256i mCoef1 = _mm256_cvtepi8_epi16(coef1);
    __m256i mSwitch = _mm256_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11, 0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11);
    __m256i mAddOffset = _mm256_set1_epi32((s16)offset);
    __m256i T0, T1, T2, T3, S0, S1, S2;
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);

    src -= 2;

    while (height--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 16) {
            S0 = _mm256_loadu_si256((__m256i*)(src + col));
            S1 = _mm256_loadu_si256((__m256i*)(src + col + 4));
            S2 = _mm256_loadu_si256((__m256i*)(src + col + 8));
            T0 = _mm256_shuffle_epi8(S0, mSwitch);
            T1 = _mm256_shuffle_epi8(S1, mSwitch);
            T2 = _mm256_shuffle_epi8(S1, mSwitch);
            T3 = _mm256_shuffle_epi8(S2, mSwitch);
            T0 = _mm256_madd_epi16(T0, mCoef0);
            T1 = _mm256_madd_epi16(T1, mCoef1);
            T2 = _mm256_madd_epi16(T2, mCoef0);
            T3 = _mm256_madd_epi16(T3, mCoef1);
            T0 = _mm256_add_epi32(T0, T1);
            T2 = _mm256_add_epi32(T2, T3);

            T0 = _mm256_add_epi32(T0, mAddOffset);
            T2 = _mm256_add_epi32(T2, mAddOffset);
            T0 = _mm256_srai_epi32(T0, shift);
            T2 = _mm256_srai_epi32(T2, shift);
            T0 = _mm256_packus_epi32(T0, T2);
            T0 = _mm256_min_epu16(T0, max_pel);

            _mm256_storeu_si256((__m256i*)(dst + col), T0);
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_luma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int i_src2 = i_src * 2;
    const int i_src3 = i_src * 3;
    const int i_src4 = i_src * 4;
    const int i_src5 = i_src * 5;
    const int i_src6 = i_src * 6;
    const int i_src7 = i_src * 7;
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    __m256i mAddOffset = _mm256_set1_epi32(32);
    __m128i s0, s1, s2, s3, s4, s5, s6, s7, s8;
    __m256i T0, T1, T2, T3, T4, T5, T6, T7;
    __m256i N0, N1, N2, N3, N4, N5, N6, N7;
    __m256i coeff00 = _mm256_cvtepi8_epi16(coeff0);
    __m256i coeff01 = _mm256_cvtepi8_epi16(coeff1);
    __m256i coeff02 = _mm256_cvtepi8_epi16(coeff2);
    __m256i coeff03 = _mm256_cvtepi8_epi16(coeff3);

    src -= i_src3;

    while (height > 0) {
        s0 = _mm_loadu_si128((__m128i*)(src));
        s1 = _mm_loadu_si128((__m128i*)(src + i_src));
        s2 = _mm_loadu_si128((__m128i*)(src + i_src2));
        s3 = _mm_loadu_si128((__m128i*)(src + i_src3));
        s4 = _mm_loadu_si128((__m128i*)(src + i_src4));
        s5 = _mm_loadu_si128((__m128i*)(src + i_src5));
        s6 = _mm_loadu_si128((__m128i*)(src + i_src6));
        s7 = _mm_loadu_si128((__m128i*)(src + i_src7));
        s8 = _mm_loadu_si128((__m128i*)(src + (i_src << 3)));

        height -= 2;
        src += i_src2;
        uavs3d_prefetch(src + i_src7, _MM_HINT_NTA);

        T0 = _mm256_set_m128i(s1, s0);
        T1 = _mm256_set_m128i(s2, s1);
        T2 = _mm256_set_m128i(s3, s2);
        T3 = _mm256_set_m128i(s4, s3);
        T4 = _mm256_set_m128i(s5, s4);
        T5 = _mm256_set_m128i(s6, s5);
        T6 = _mm256_set_m128i(s7, s6);
        T7 = _mm256_set_m128i(s8, s7);

        N0 = _mm256_unpacklo_epi16(T0, T1);
        N1 = _mm256_unpacklo_epi16(T2, T3);
        N2 = _mm256_unpacklo_epi16(T4, T5);
        N3 = _mm256_unpacklo_epi16(T6, T7);
        N4 = _mm256_unpackhi_epi16(T0, T1);
        N5 = _mm256_unpackhi_epi16(T2, T3);
        N6 = _mm256_unpackhi_epi16(T4, T5);
        N7 = _mm256_unpackhi_epi16(T6, T7);

        N0 = _mm256_madd_epi16(N0, coeff00);
        N1 = _mm256_madd_epi16(N1, coeff01);
        N2 = _mm256_madd_epi16(N2, coeff02);
        N3 = _mm256_madd_epi16(N3, coeff03);
        N4 = _mm256_madd_epi16(N4, coeff00);
        N5 = _mm256_madd_epi16(N5, coeff01);
        N6 = _mm256_madd_epi16(N6, coeff02);
        N7 = _mm256_madd_epi16(N7, coeff03);

        N0 = _mm256_add_epi32(N0, N1);
        N1 = _mm256_add_epi32(N2, N3);
        N2 = _mm256_add_epi32(N4, N5);
        N3 = _mm256_add_epi32(N6, N7);

        N0 = _mm256_add_epi32(N0, N1);
        N1 = _mm256_add_epi32(N2, N3);

        N0 = _mm256_add_epi32(N0, mAddOffset);
        N1 = _mm256_add_epi32(N1, mAddOffset);
        N0 = _mm256_srai_epi32(N0, 6);
        N1 = _mm256_srai_epi32(N1, 6);
        N0 = _mm256_packus_epi32(N0, N1);
        N0 = _mm256_min_epu16(N0, max_pel);
        _mm_storeu_si128((__m128i*)(dst), _mm256_castsi256_si128(N0));
        _mm_storeu_si128((__m128i*)(dst + i_dst), _mm256_extracti128_si256(N0, 1));

        dst += i_dst << 1;
    }
}

void uavs3d_if_ver_luma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int i_src2 = i_src * 2;
    const int i_src3 = i_src * 3;
    const int i_src4 = i_src * 4;
    const int i_src5 = i_src * 5;
    const int i_src6 = i_src * 6;
    const int i_src7 = i_src * 7;
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    __m256i mAddOffset = _mm256_set1_epi32(32);
    __m256i T0, T1, T2, T3, T4, T5, T6, T7;
    __m256i N0, N1, N2, N3, N4, N5, N6, N7;
    __m256i coeff00 = _mm256_cvtepi8_epi16(coeff0);
    __m256i coeff01 = _mm256_cvtepi8_epi16(coeff1);
    __m256i coeff02 = _mm256_cvtepi8_epi16(coeff2);
    __m256i coeff03 = _mm256_cvtepi8_epi16(coeff3);

    src -= 3 * i_src;

    while (height--) {
        T0 = _mm256_loadu_si256((__m256i*)(src));
        T1 = _mm256_loadu_si256((__m256i*)(src + i_src));
        T2 = _mm256_loadu_si256((__m256i*)(src + i_src2));
        T3 = _mm256_loadu_si256((__m256i*)(src + i_src3));
        T4 = _mm256_loadu_si256((__m256i*)(src + i_src4));
        T5 = _mm256_loadu_si256((__m256i*)(src + i_src5));
        T6 = _mm256_loadu_si256((__m256i*)(src + i_src6));
        T7 = _mm256_loadu_si256((__m256i*)(src + i_src7));
        uavs3d_prefetch(src + 8 * i_src, _MM_HINT_NTA);

        N0 = _mm256_unpacklo_epi16(T0, T1);
        N1 = _mm256_unpacklo_epi16(T2, T3);
        N2 = _mm256_unpacklo_epi16(T4, T5);
        N3 = _mm256_unpacklo_epi16(T6, T7);
        N4 = _mm256_unpackhi_epi16(T0, T1);
        N5 = _mm256_unpackhi_epi16(T2, T3);
        N6 = _mm256_unpackhi_epi16(T4, T5);
        N7 = _mm256_unpackhi_epi16(T6, T7);

        N0 = _mm256_madd_epi16(N0, coeff00);
        N1 = _mm256_madd_epi16(N1, coeff01);
        N2 = _mm256_madd_epi16(N2, coeff02);
        N3 = _mm256_madd_epi16(N3, coeff03);
        N4 = _mm256_madd_epi16(N4, coeff00);
        N5 = _mm256_madd_epi16(N5, coeff01);
        N6 = _mm256_madd_epi16(N6, coeff02);
        N7 = _mm256_madd_epi16(N7, coeff03);

        N0 = _mm256_add_epi32(N0, N1);
        N1 = _mm256_add_epi32(N2, N3);
        N2 = _mm256_add_epi32(N4, N5);
        N3 = _mm256_add_epi32(N6, N7);

        N0 = _mm256_add_epi32(N0, N1);
        N1 = _mm256_add_epi32(N2, N3);

        N0 = _mm256_add_epi32(N0, mAddOffset);
        N1 = _mm256_add_epi32(N1, mAddOffset);
        N0 = _mm256_srai_epi32(N0, 6);
        N1 = _mm256_srai_epi32(N1, 6);
        N0 = _mm256_packus_epi32(N0, N1);
        N0 = _mm256_min_epu16(N0, max_pel);
        _mm256_storeu_si256((__m256i*)(dst), N0);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_luma_w16x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int i;
    const int i_src2 = i_src * 2;
    const int i_src3 = i_src * 3;
    const int i_src4 = i_src * 4;
    const int i_src5 = i_src * 5;
    const int i_src6 = i_src * 6;
    const int i_src7 = i_src * 7;
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    __m256i mAddOffset = _mm256_set1_epi32(32);
    __m256i T0, T1, T2, T3, T4, T5, T6, T7;
    __m256i M0, M1, M2, M3, M4, M5, M6, M7;
    __m256i N0, N1, N2, N3, N4, N5, N6, N7;
    __m256i coeff00 = _mm256_cvtepi8_epi16(coeff0);
    __m256i coeff01 = _mm256_cvtepi8_epi16(coeff1);
    __m256i coeff02 = _mm256_cvtepi8_epi16(coeff2);
    __m256i coeff03 = _mm256_cvtepi8_epi16(coeff3);

    src -= 3 * i_src;

    while (height--) {
        const pel *p_src = src;
        uavs3d_prefetch(src + 8 * i_src, _MM_HINT_NTA);
        for (i = 0; i < width; i += 16) {
            T0 = _mm256_loadu_si256((__m256i*)(p_src));
            T1 = _mm256_loadu_si256((__m256i*)(p_src + i_src));
            T2 = _mm256_loadu_si256((__m256i*)(p_src + i_src2));
            T3 = _mm256_loadu_si256((__m256i*)(p_src + i_src3));
            T4 = _mm256_loadu_si256((__m256i*)(p_src + i_src4));
            T5 = _mm256_loadu_si256((__m256i*)(p_src + i_src5));
            T6 = _mm256_loadu_si256((__m256i*)(p_src + i_src6));
            T7 = _mm256_loadu_si256((__m256i*)(p_src + i_src7));

            M0 = _mm256_unpacklo_epi16(T0, T1);
            M1 = _mm256_unpacklo_epi16(T2, T3);
            M2 = _mm256_unpacklo_epi16(T4, T5);
            M3 = _mm256_unpacklo_epi16(T6, T7);
            M4 = _mm256_unpackhi_epi16(T0, T1);
            M5 = _mm256_unpackhi_epi16(T2, T3);
            M6 = _mm256_unpackhi_epi16(T4, T5);
            M7 = _mm256_unpackhi_epi16(T6, T7);

            N0 = _mm256_madd_epi16(M0, coeff00);
            N1 = _mm256_madd_epi16(M1, coeff01);
            N2 = _mm256_madd_epi16(M2, coeff02);
            N3 = _mm256_madd_epi16(M3, coeff03);
            N4 = _mm256_madd_epi16(M4, coeff00);
            N5 = _mm256_madd_epi16(M5, coeff01);
            N6 = _mm256_madd_epi16(M6, coeff02);
            N7 = _mm256_madd_epi16(M7, coeff03);

            N0 = _mm256_add_epi32(N0, N1);
            N1 = _mm256_add_epi32(N2, N3);
            N2 = _mm256_add_epi32(N4, N5);
            N3 = _mm256_add_epi32(N6, N7);

            N0 = _mm256_add_epi32(N0, N1);
            N1 = _mm256_add_epi32(N2, N3);

            N0 = _mm256_add_epi32(N0, mAddOffset);
            N1 = _mm256_add_epi32(N1, mAddOffset);
            N0 = _mm256_srai_epi32(N0, 6);
            N1 = _mm256_srai_epi32(N1, 6);
            N0 = _mm256_packus_epi32(N0, N1);
            p_src += 16;
            N0 = _mm256_min_epu16(N0, max_pel);
            _mm256_storeu_si256((__m256i*)(dst + i), N0);
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_chroma_w16_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi32(offset);
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    const int i_src2 = i_src << 1;
    const int i_src3 = i_src + i_src2;
    const int i_src4 = i_src << 2;
    __m128i coeff00 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff11 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m256i coeff0 = _mm256_cvtepi8_epi16(coeff00);
    __m256i coeff1 = _mm256_cvtepi8_epi16(coeff11);
    __m256i T0, T1, T2, T3, mVal1, mVal2;

    src -= i_src;

    while (height) {
        __m256i S0, S1, S2, S3, S4;
        S0 = _mm256_loadu_si256((__m256i*)(src));
        S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
        S2 = _mm256_loadu_si256((__m256i*)(src + i_src2));
        S3 = _mm256_loadu_si256((__m256i*)(src + i_src3));
        S4 = _mm256_loadu_si256((__m256i*)(src + i_src4));

        height -= 2;
        src += i_src2;

        T0 = _mm256_unpacklo_epi16(S0, S1);
        T1 = _mm256_unpackhi_epi16(S0, S1);
        T2 = _mm256_unpacklo_epi16(S2, S3);
        T3 = _mm256_unpackhi_epi16(S2, S3);

        uavs3d_prefetch(src + i_src3, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src4, _MM_HINT_NTA);

        T0 = _mm256_madd_epi16(T0, coeff0);
        T1 = _mm256_madd_epi16(T1, coeff0);
        T2 = _mm256_madd_epi16(T2, coeff1);
        T3 = _mm256_madd_epi16(T3, coeff1);

        mVal1 = _mm256_add_epi32(T0, T2);
        mVal2 = _mm256_add_epi32(T1, T3);

        mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm256_srai_epi32(mVal1, shift);
        mVal2 = _mm256_srai_epi32(mVal2, shift);
        mVal1 = _mm256_packus_epi32(mVal1, mVal2);

        mVal1 = _mm256_min_epu16(mVal1, max_pel);
        _mm256_storeu_si256((__m256i*)dst, mVal1);

        T0 = _mm256_unpacklo_epi16(S1, S2);
        T1 = _mm256_unpackhi_epi16(S1, S2);
        T2 = _mm256_unpacklo_epi16(S3, S4);
        T3 = _mm256_unpackhi_epi16(S3, S4);

        T0 = _mm256_madd_epi16(T0, coeff0);
        T1 = _mm256_madd_epi16(T1, coeff0);
        T2 = _mm256_madd_epi16(T2, coeff1);
        T3 = _mm256_madd_epi16(T3, coeff1);

        mVal1 = _mm256_add_epi32(T0, T2);
        mVal2 = _mm256_add_epi32(T1, T3);

        mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm256_srai_epi32(mVal1, shift);
        mVal2 = _mm256_srai_epi32(mVal2, shift);
        mVal1 = _mm256_packus_epi32(mVal1, mVal2);

        mVal1 = _mm256_min_epu16(mVal1, max_pel);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), mVal1);

        dst += 2 * i_dst;
    }
}

void uavs3d_if_ver_chroma_w32_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi32(offset);
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    const int i_src2 = i_src << 1;
    const int i_src3 = i_src + i_src2;
    const int i_src4 = i_src << 2;
    __m128i coeff00 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff11 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m256i coeff0 = _mm256_cvtepi8_epi16(coeff00);
    __m256i coeff1 = _mm256_cvtepi8_epi16(coeff11);
    __m256i T0, T1, T2, T3, T4, T5, T6, T7;
    __m256i S0, S1, S2, S3, S4, S5, S6, S7, S8, S9;

    src -= i_src;

    while (height) {
        S0 = _mm256_loadu_si256((__m256i*)(src));
        S5 = _mm256_loadu_si256((__m256i*)(src + 16));
        S1 = _mm256_loadu_si256((__m256i*)(src + i_src));
        S6 = _mm256_loadu_si256((__m256i*)(src + i_src + 16));
        S2 = _mm256_loadu_si256((__m256i*)(src + i_src2));
        S7 = _mm256_loadu_si256((__m256i*)(src + i_src2 + 16));
        S3 = _mm256_loadu_si256((__m256i*)(src + i_src3));
        S8 = _mm256_loadu_si256((__m256i*)(src + i_src3 + 16));
        S4 = _mm256_loadu_si256((__m256i*)(src + i_src4));
        S9 = _mm256_loadu_si256((__m256i*)(src + i_src4 + 16));

        height -= 2;
        src += i_src2;

        T0 = _mm256_unpacklo_epi16(S0, S1);
        T1 = _mm256_unpackhi_epi16(S0, S1);
        T2 = _mm256_unpacklo_epi16(S2, S3);
        T3 = _mm256_unpackhi_epi16(S2, S3);
        T4 = _mm256_unpacklo_epi16(S5, S6);
        T5 = _mm256_unpackhi_epi16(S5, S6);
        T6 = _mm256_unpacklo_epi16(S7, S8);
        T7 = _mm256_unpackhi_epi16(S7, S8);

        uavs3d_prefetch(src + i_src3, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src4, _MM_HINT_NTA);

        T0 = _mm256_madd_epi16(T0, coeff0);
        T1 = _mm256_madd_epi16(T1, coeff0);
        T2 = _mm256_madd_epi16(T2, coeff1);
        T3 = _mm256_madd_epi16(T3, coeff1);
        T4 = _mm256_madd_epi16(T4, coeff0);
        T5 = _mm256_madd_epi16(T5, coeff0);
        T6 = _mm256_madd_epi16(T6, coeff1);
        T7 = _mm256_madd_epi16(T7, coeff1);

        T0 = _mm256_add_epi32(T0, T2);
        T1 = _mm256_add_epi32(T1, T3);
        T2 = _mm256_add_epi32(T4, T6);
        T3 = _mm256_add_epi32(T5, T7);

        T0 = _mm256_add_epi32(T0, mAddOffset);
        T1 = _mm256_add_epi32(T1, mAddOffset);
        T2 = _mm256_add_epi32(T2, mAddOffset);
        T3 = _mm256_add_epi32(T3, mAddOffset);
        T0 = _mm256_srai_epi32(T0, shift);
        T1 = _mm256_srai_epi32(T1, shift);
        T2 = _mm256_srai_epi32(T2, shift);
        T3 = _mm256_srai_epi32(T3, shift);
        T0 = _mm256_packus_epi32(T0, T1);
        T2 = _mm256_packus_epi32(T2, T3);

        T0 = _mm256_min_epu16(T0, max_pel);
        T2 = _mm256_min_epu16(T2, max_pel);
        _mm256_storeu_si256((__m256i*)dst, T0);
        _mm256_storeu_si256((__m256i*)(dst + 16), T2);

        T0 = _mm256_unpacklo_epi16(S1, S2);
        T1 = _mm256_unpackhi_epi16(S1, S2);
        T2 = _mm256_unpacklo_epi16(S3, S4);
        T3 = _mm256_unpackhi_epi16(S3, S4);
        T4 = _mm256_unpacklo_epi16(S6, S7);
        T5 = _mm256_unpackhi_epi16(S6, S7);
        T6 = _mm256_unpacklo_epi16(S8, S9);
        T7 = _mm256_unpackhi_epi16(S8, S9);

        T0 = _mm256_madd_epi16(T0, coeff0);
        T1 = _mm256_madd_epi16(T1, coeff0);
        T2 = _mm256_madd_epi16(T2, coeff1);
        T3 = _mm256_madd_epi16(T3, coeff1);
        T4 = _mm256_madd_epi16(T4, coeff0);
        T5 = _mm256_madd_epi16(T5, coeff0);
        T6 = _mm256_madd_epi16(T6, coeff1);
        T7 = _mm256_madd_epi16(T7, coeff1);

        T0 = _mm256_add_epi32(T0, T2);
        T1 = _mm256_add_epi32(T1, T3);
        T2 = _mm256_add_epi32(T4, T6);
        T3 = _mm256_add_epi32(T5, T7);

        T0 = _mm256_add_epi32(T0, mAddOffset);
        T1 = _mm256_add_epi32(T1, mAddOffset);
        T2 = _mm256_add_epi32(T2, mAddOffset);
        T3 = _mm256_add_epi32(T3, mAddOffset);
        T0 = _mm256_srai_epi32(T0, shift);
        T1 = _mm256_srai_epi32(T1, shift);
        T2 = _mm256_srai_epi32(T2, shift);
        T3 = _mm256_srai_epi32(T3, shift);
        T0 = _mm256_packus_epi32(T0, T1);
        T2 = _mm256_packus_epi32(T2, T3);

        T0 = _mm256_min_epu16(T0, max_pel);
        T2 = _mm256_min_epu16(T2, max_pel);

        _mm256_storeu_si256((__m256i*)(dst + i_dst), T0);
        _mm256_storeu_si256((__m256i*)(dst + i_dst + 16), T2);

        dst += 2 * i_dst;
    }
}

void uavs3d_if_ver_chroma_w32x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int col;
    const int offset = 32;
    const int shift = 6;
    __m256i mAddOffset = _mm256_set1_epi32(offset);
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);
    const int i_src2 = i_src << 1;
    const int i_src3 = i_src + i_src2;
    const int i_src4 = i_src << 2;
    __m128i coeff00 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff11 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m256i coeff0 = _mm256_cvtepi8_epi16(coeff00);
    __m256i coeff1 = _mm256_cvtepi8_epi16(coeff11);
    __m256i T0, T1, T2, T3, T4, T5, T6, T7;
    __m256i S0, S1, S2, S3, S4, S5, S6, S7;

    src -= i_src;

    while (height--) {
        const pel *p_src = src;
        uavs3d_prefetch(src + 4 * i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 32) {
            S0 = _mm256_loadu_si256((__m256i*)(p_src));
            S4 = _mm256_loadu_si256((__m256i*)(p_src + 16));
            S1 = _mm256_loadu_si256((__m256i*)(p_src + i_src));
            S5 = _mm256_loadu_si256((__m256i*)(p_src + i_src + 16));
            S2 = _mm256_loadu_si256((__m256i*)(p_src + i_src2));
            S6 = _mm256_loadu_si256((__m256i*)(p_src + i_src2 + 16));
            S3 = _mm256_loadu_si256((__m256i*)(p_src + i_src3));
            S7 = _mm256_loadu_si256((__m256i*)(p_src + i_src3 + 16));

            T0 = _mm256_unpacklo_epi16(S0, S1);
            T1 = _mm256_unpackhi_epi16(S0, S1);
            T2 = _mm256_unpacklo_epi16(S2, S3);
            T3 = _mm256_unpackhi_epi16(S2, S3);
            T4 = _mm256_unpacklo_epi16(S4, S5);
            T5 = _mm256_unpackhi_epi16(S4, S5);
            T6 = _mm256_unpacklo_epi16(S6, S7);
            T7 = _mm256_unpackhi_epi16(S6, S7);

            T0 = _mm256_madd_epi16(T0, coeff0);
            T1 = _mm256_madd_epi16(T1, coeff0);
            T2 = _mm256_madd_epi16(T2, coeff1);
            T3 = _mm256_madd_epi16(T3, coeff1);
            T4 = _mm256_madd_epi16(T4, coeff0);
            T5 = _mm256_madd_epi16(T5, coeff0);
            T6 = _mm256_madd_epi16(T6, coeff1);
            T7 = _mm256_madd_epi16(T7, coeff1);

            T0 = _mm256_add_epi32(T0, T2);
            T1 = _mm256_add_epi32(T1, T3);
            T2 = _mm256_add_epi32(T4, T6);
            T3 = _mm256_add_epi32(T5, T7);

            T0 = _mm256_add_epi32(T0, mAddOffset);
            T1 = _mm256_add_epi32(T1, mAddOffset);
            T2 = _mm256_add_epi32(T2, mAddOffset);
            T3 = _mm256_add_epi32(T3, mAddOffset);
            T0 = _mm256_srai_epi32(T0, shift);
            T1 = _mm256_srai_epi32(T1, shift);
            T2 = _mm256_srai_epi32(T2, shift);
            T3 = _mm256_srai_epi32(T3, shift);
            T0 = _mm256_packus_epi32(T0, T1);
            T2 = _mm256_packus_epi32(T2, T3);

            T0 = _mm256_min_epu16(T0, max_pel);
            T2 = _mm256_min_epu16(T2, max_pel);
            p_src += 32;
            _mm256_storeu_si256((__m256i*)(dst + col), T0);
            _mm256_storeu_si256((__m256i*)(dst + col + 16), T2);
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_ver_luma_w4_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(32 + 7) * 4]);
    s16 *tmp = tmp_res;
    int row;
    int add1, shift1;
    int add2, shift2;
    __m256i offset;
    __m256i T0, T1, T2, T3;
    __m256i M0, M1, M2, M3;
    const int i_tmp = 4;
    __m256i mCoef0, mCoef1, mCoef2, mCoef3;
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);

    if (max_val == 255) { // 8 bit_depth
        shift1 = 0;
        shift2 = 12;
    }
    else { // 10 bit_depth
        shift1 = 2;
        shift2 = 10;
    }

    add1 = (1 << (shift1)) >> 1;
    add2 = 1 << (shift2 - 1);

    src += -3 * i_src - 3;

    {
        __m128i s0, s1, s2, s3;
        __m256i S0, S1;
        __m256i mShuffle0 = _mm256_setr_epi8(0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9, 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9);
        __m256i mShuffle1 = _mm256_setr_epi8(4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13, 4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13);

        mCoef0 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[0]));
        mCoef1 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[1]));
        mCoef2 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[2]));
        mCoef3 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[3]));
        offset = _mm256_set1_epi32(add1);

        row = height + 6;

        while (row > 0) {
            s0 = _mm_loadu_si128((__m128i*)(src));
            s1 = _mm_loadu_si128((__m128i*)(src + 4));
            s2 = _mm_loadu_si128((__m128i*)(src + i_src));
            s3 = _mm_loadu_si128((__m128i*)(src + i_src + 4));
            row -= 2;
            src += i_src << 1;
            uavs3d_prefetch(src, _MM_HINT_NTA);
            uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

            S0 = _mm256_set_m128i(s2, s0);
            S1 = _mm256_set_m128i(s3, s1);

            T0 = _mm256_shuffle_epi8(S0, mShuffle0);
            T1 = _mm256_shuffle_epi8(S0, mShuffle1);
            T2 = _mm256_shuffle_epi8(S1, mShuffle0);
            T3 = _mm256_shuffle_epi8(S1, mShuffle1);

            M0 = _mm256_madd_epi16(T0, mCoef0);
            M1 = _mm256_madd_epi16(T1, mCoef1);
            M2 = _mm256_madd_epi16(T2, mCoef2);
            M3 = _mm256_madd_epi16(T3, mCoef3);

            M0 = _mm256_add_epi32(M0, M1);
            M1 = _mm256_add_epi32(M2, M3);

            M0 = _mm256_add_epi32(M0, M1);

            M2 = _mm256_add_epi32(M0, offset);
            M2 = _mm256_srai_epi32(M2, shift1);
            
            s0 = _mm_packs_epi32(_mm256_castsi256_si128(M2), _mm256_extracti128_si256(M2, 1));
            _mm_store_si128((__m128i*)(tmp), s0);

            tmp += i_tmp * 2;
        }
        {
            // the last row
            __m128i t0, t1, t2, t3;
            __m128i m0, m1, m2, m3;
            s0 = _mm_loadu_si128((__m128i*)(src));
            s1 = _mm_loadu_si128((__m128i*)(src + 4));
            src += i_src;

            t0 = _mm_shuffle_epi8(s0, _mm256_castsi256_si128(mShuffle0));
            t1 = _mm_shuffle_epi8(s0, _mm256_castsi256_si128(mShuffle1));
            t2 = _mm_shuffle_epi8(s1, _mm256_castsi256_si128(mShuffle0));
            t3 = _mm_shuffle_epi8(s1, _mm256_castsi256_si128(mShuffle1));

            m0 = _mm_madd_epi16(t0, _mm256_castsi256_si128(mCoef0));
            m1 = _mm_madd_epi16(t1, _mm256_castsi256_si128(mCoef1));
            m2 = _mm_madd_epi16(t2, _mm256_castsi256_si128(mCoef2));
            m3 = _mm_madd_epi16(t3, _mm256_castsi256_si128(mCoef3));

            m0 = _mm_add_epi32(m0, m1);
            m1 = _mm_add_epi32(m2, m3);

            m0 = _mm_add_epi32(m0, m1);

            m0 = _mm_add_epi32(m0, _mm256_castsi256_si128(offset));
            m0 = _mm_srai_epi32(m0, shift1);
            m0 = _mm_packs_epi32(m0, m0);
            _mm_storel_epi64((__m128i*)tmp, m0);
        }
    }

    {
        __m256i T4, T5, T6, T7, M4, M5, M6, M7;
        __m128i d0, d1;

        offset = _mm256_set1_epi32(add2);
        tmp = tmp_res;

        mCoef0 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[0]));
        mCoef1 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[1]));
        mCoef2 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[2]));
        mCoef3 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[3]));

        while (height > 0) {
            T0 = _mm256_load_si256((__m256i*)(tmp));
            T1 = _mm256_loadu_si256((__m256i*)(tmp + i_tmp));
            T2 = _mm256_loadu_si256((__m256i*)(tmp + 2 * i_tmp));
            T3 = _mm256_loadu_si256((__m256i*)(tmp + 3 * i_tmp));
            T4 = _mm256_load_si256((__m256i*)(tmp + 4 * i_tmp));
            T5 = _mm256_loadu_si256((__m256i*)(tmp + 5 * i_tmp));
            T6 = _mm256_loadu_si256((__m256i*)(tmp + 6 * i_tmp));
            T7 = _mm256_loadu_si256((__m256i*)(tmp + 7 * i_tmp));
            height -= 4;
            tmp += i_tmp * 4;

            M0 = _mm256_unpacklo_epi16(T0, T1);
            M1 = _mm256_unpacklo_epi16(T2, T3);
            M2 = _mm256_unpacklo_epi16(T4, T5);
            M3 = _mm256_unpacklo_epi16(T6, T7);
            M4 = _mm256_unpackhi_epi16(T0, T1);
            M5 = _mm256_unpackhi_epi16(T2, T3);
            M6 = _mm256_unpackhi_epi16(T4, T5);
            M7 = _mm256_unpackhi_epi16(T6, T7);

            M0 = _mm256_madd_epi16(M0, mCoef0);
            M1 = _mm256_madd_epi16(M1, mCoef1);
            M2 = _mm256_madd_epi16(M2, mCoef2);
            M3 = _mm256_madd_epi16(M3, mCoef3);
            M4 = _mm256_madd_epi16(M4, mCoef0);
            M5 = _mm256_madd_epi16(M5, mCoef1);
            M6 = _mm256_madd_epi16(M6, mCoef2);
            M7 = _mm256_madd_epi16(M7, mCoef3);

            M0 = _mm256_add_epi32(M0, M1);
            M1 = _mm256_add_epi32(M2, M3);
            M2 = _mm256_add_epi32(M4, M5);
            M3 = _mm256_add_epi32(M6, M7);

            M0 = _mm256_add_epi32(M0, M1);
            M1 = _mm256_add_epi32(M2, M3);

            M0 = _mm256_add_epi32(M0, offset);
            M1 = _mm256_add_epi32(M1, offset);
            M0 = _mm256_srai_epi32(M0, shift2);
            M1 = _mm256_srai_epi32(M1, shift2);
            M0 = _mm256_packus_epi32(M0, M1);
            M0 = _mm256_min_epu16(M0, max_pel);

            d0 = _mm256_castsi256_si128(M0);
            d1 = _mm256_extracti128_si256(M0, 1);
            _mm_storel_epi64((__m128i*)(dst), d0);
            _mm_storeh_pi((__m64*)(dst + i_dst), _mm_castsi128_ps(d0));
            _mm_storel_epi64((__m128i*)(dst + (i_dst << 1)), d1);
            _mm_storeh_pi((__m64*)(dst + i_dst * 3), _mm_castsi128_ps(d1));

            dst += i_dst << 2;
        }
    }
}

void uavs3d_if_hor_ver_luma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(64 + 7) * 8]);
    s16 *tmp = tmp_res;
    int row;
    int add1, shift1;
    int add2, shift2;
    __m256i offset;
    __m256i T0, T1, T2, T3, T4, T5;
    __m256i M0, M1, M2, M3, M4, M5, M6, M7;
    const int i_tmp = 8;
    __m256i mCoef0, mCoef1, mCoef2, mCoef3;
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);

    if (max_val == 255) { // 8 bit_depth
        shift1 = 0;
        shift2 = 12;
    }
    else { // 10 bit_depth
        shift1 = 2;
        shift2 = 10;
    }

    add1 = (1 << (shift1)) >> 1;
    add2 = 1 << (shift2 - 1);

    src += -3 * i_src - 3;

    {
        __m128i s0, s1;
        __m256i S0, S1, S2;
        __m256i mShuffle0 = _mm256_setr_epi8(0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9, 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9);
        __m256i mShuffle1 = _mm256_setr_epi8(4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13, 4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13);

        mCoef0 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[0]));
        mCoef1 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[1]));
        mCoef2 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[2]));
        mCoef3 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[3]));
        offset = _mm256_set1_epi32(add1);

        row = height + 6;

        while (row > 0) {
            T0 = _mm256_loadu_si256((__m256i*)(src));
            s0 = _mm_loadu_si128((__m128i*)(src + 4));
            T1 = _mm256_loadu_si256((__m256i*)(src + i_src));
            s1 = _mm_loadu_si128((__m128i*)(src + i_src + 4));
            row -= 2;
            src += i_src << 1;
            uavs3d_prefetch(src, _MM_HINT_NTA);
            uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

            S0 = _mm256_permute2x128_si256(T0, T1, 0x20);
            S2 = _mm256_permute2x128_si256(T0, T1, 0x31);
            S1 = _mm256_set_m128i(s1, s0);

            T0 = _mm256_shuffle_epi8(S0, mShuffle0);
            T1 = _mm256_shuffle_epi8(S0, mShuffle1);
            T2 = _mm256_shuffle_epi8(S1, mShuffle0);
            T3 = _mm256_shuffle_epi8(S1, mShuffle1);
            T4 = _mm256_shuffle_epi8(S2, mShuffle0);
            T5 = _mm256_shuffle_epi8(S2, mShuffle1);

            M0 = _mm256_madd_epi16(T0, mCoef0);
            M1 = _mm256_madd_epi16(T1, mCoef1);
            M2 = _mm256_madd_epi16(T2, mCoef2);
            M3 = _mm256_madd_epi16(T3, mCoef3);
            M4 = _mm256_madd_epi16(T2, mCoef0);
            M5 = _mm256_madd_epi16(T3, mCoef1);
            M6 = _mm256_madd_epi16(T4, mCoef2);
            M7 = _mm256_madd_epi16(T5, mCoef3);

            M0 = _mm256_add_epi32(M0, M1);
            M1 = _mm256_add_epi32(M2, M3);
            M2 = _mm256_add_epi32(M4, M5);
            M3 = _mm256_add_epi32(M6, M7);

            M0 = _mm256_add_epi32(M0, M1);
            M1 = _mm256_add_epi32(M2, M3);

            M2 = _mm256_add_epi32(M0, offset);
            M3 = _mm256_add_epi32(M1, offset);
            M2 = _mm256_srai_epi32(M2, shift1);
            M3 = _mm256_srai_epi32(M3, shift1);
            M2 = _mm256_packs_epi32(M2, M3);

            _mm256_store_si256((__m256i*)(tmp), M2);

            tmp += i_tmp * 2;
        }
        {
            // the last row
            __m128i t0, t1, t2, t3, t4, t5;
            __m128i m0, m1, m2, m3, m4, m5, m6, m7;
            __m128i s2;
            s0 = _mm_loadu_si128((__m128i*)(src));
            s1 = _mm_loadu_si128((__m128i*)(src + 4));
            s2 = _mm_loadu_si128((__m128i*)(src + 8));
            src += i_src;

            t0 = _mm_shuffle_epi8(s0, _mm256_castsi256_si128(mShuffle0));
            t1 = _mm_shuffle_epi8(s0, _mm256_castsi256_si128(mShuffle1));
            t2 = _mm_shuffle_epi8(s1, _mm256_castsi256_si128(mShuffle0));
            t3 = _mm_shuffle_epi8(s1, _mm256_castsi256_si128(mShuffle1));
            t4 = _mm_shuffle_epi8(s2, _mm256_castsi256_si128(mShuffle0));
            t5 = _mm_shuffle_epi8(s2, _mm256_castsi256_si128(mShuffle1));

            m0 = _mm_madd_epi16(t0, _mm256_castsi256_si128(mCoef0));
            m1 = _mm_madd_epi16(t1, _mm256_castsi256_si128(mCoef1));
            m2 = _mm_madd_epi16(t2, _mm256_castsi256_si128(mCoef2));
            m3 = _mm_madd_epi16(t3, _mm256_castsi256_si128(mCoef3));
            m4 = _mm_madd_epi16(t2, _mm256_castsi256_si128(mCoef0));
            m5 = _mm_madd_epi16(t3, _mm256_castsi256_si128(mCoef1));
            m6 = _mm_madd_epi16(t4, _mm256_castsi256_si128(mCoef2));
            m7 = _mm_madd_epi16(t5, _mm256_castsi256_si128(mCoef3));

            m0 = _mm_add_epi32(m0, m1);
            m1 = _mm_add_epi32(m2, m3);
            m2 = _mm_add_epi32(m4, m5);
            m3 = _mm_add_epi32(m6, m7);

            m0 = _mm_add_epi32(m0, m1);
            m1 = _mm_add_epi32(m2, m3);

            m2 = _mm_add_epi32(m0, _mm256_castsi256_si128(offset));
            m3 = _mm_add_epi32(m1, _mm256_castsi256_si128(offset));
            m2 = _mm_srai_epi32(m2, shift1);
            m3 = _mm_srai_epi32(m3, shift1);
            m2 = _mm_packs_epi32(m2, m3);
            _mm_store_si128((__m128i*)tmp, m2);
        }
    }

    {
        __m256i N0, N1, N2, N3, N4, N5, N6, N7;
        __m256i T6, T7;
        offset = _mm256_set1_epi32(add2);
        tmp = tmp_res;

        mCoef0 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[0]));
        mCoef1 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[1]));
        mCoef2 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[2]));
        mCoef3 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[3]));

        while (height > 0) {
            T0 = _mm256_load_si256((__m256i*)(tmp));
            T1 = _mm256_loadu_si256((__m256i*)(tmp + i_tmp));
            T2 = _mm256_load_si256((__m256i*)(tmp + 2 * i_tmp));
            T3 = _mm256_loadu_si256((__m256i*)(tmp + 3 * i_tmp));
            T4 = _mm256_load_si256((__m256i*)(tmp + 4 * i_tmp));
            T5 = _mm256_loadu_si256((__m256i*)(tmp + 5 * i_tmp));
            T6 = _mm256_load_si256((__m256i*)(tmp + 6 * i_tmp));
            T7 = _mm256_loadu_si256((__m256i*)(tmp + 7 * i_tmp));
            height -= 2;
            tmp += i_tmp * 2;

            M0 = _mm256_unpacklo_epi16(T0, T1);
            M1 = _mm256_unpacklo_epi16(T2, T3);
            M2 = _mm256_unpacklo_epi16(T4, T5);
            M3 = _mm256_unpacklo_epi16(T6, T7);
            M4 = _mm256_unpackhi_epi16(T0, T1);
            M5 = _mm256_unpackhi_epi16(T2, T3);
            M6 = _mm256_unpackhi_epi16(T4, T5);
            M7 = _mm256_unpackhi_epi16(T6, T7);

            N0 = _mm256_madd_epi16(M0, mCoef0);
            N1 = _mm256_madd_epi16(M1, mCoef1);
            N2 = _mm256_madd_epi16(M2, mCoef2);
            N3 = _mm256_madd_epi16(M3, mCoef3);
            N4 = _mm256_madd_epi16(M4, mCoef0);
            N5 = _mm256_madd_epi16(M5, mCoef1);
            N6 = _mm256_madd_epi16(M6, mCoef2);
            N7 = _mm256_madd_epi16(M7, mCoef3);

            N0 = _mm256_add_epi32(N0, N1);
            N1 = _mm256_add_epi32(N2, N3);
            N2 = _mm256_add_epi32(N4, N5);
            N3 = _mm256_add_epi32(N6, N7);

            N0 = _mm256_add_epi32(N0, N1);
            N1 = _mm256_add_epi32(N2, N3);

            N0 = _mm256_add_epi32(N0, offset);
            N1 = _mm256_add_epi32(N1, offset);
            N0 = _mm256_srai_epi32(N0, shift2);
            N1 = _mm256_srai_epi32(N1, shift2);
            N0 = _mm256_packus_epi32(N0, N1);
            N0 = _mm256_min_epu16(N0, max_pel);

            _mm_storeu_si128((__m128i*)(dst), _mm256_castsi256_si128(N0));
            _mm_storeu_si128((__m128i*)(dst + i_dst), _mm256_extracti128_si256(N0, 1));

            dst += i_dst << 1;
        }
    }
}

void uavs3d_if_hor_ver_luma_w16x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(128 + 7) * 128]);
    s16 *tmp = tmp_res;
    int row, i;;
    int add1, shift1;
    int add2, shift2;
    __m256i offset;
    __m256i T0, T1, T2, T3, T4, T5;
    __m256i M0, M1, M2, M3, M4, M5, M6, M7;
    int i_tmp = width;
    __m256i mCoef0, mCoef1, mCoef2, mCoef3;
    __m256i max_pel = _mm256_set1_epi16((pel)max_val);

    if (max_val == 255) { // 8 bit_depth
        shift1 = 0;
        shift2 = 12;
    }
    else { // 10 bit_depth
        shift1 = 2;
        shift2 = 10;
    }

    add1 = (1 << (shift1)) >> 1;
    add2 = 1 << (shift2 - 1);

    src += -3 * i_src - 3;

    {
        __m256i S0, S1, S2;
        __m256i mShuffle0 = _mm256_setr_epi8(0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9, 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9);
        __m256i mShuffle1 = _mm256_setr_epi8(4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13, 4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13);

        mCoef0 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[0]));
        mCoef1 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[1]));
        mCoef2 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[2]));
        mCoef3 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_x)[3]));
        offset = _mm256_set1_epi32(add1);

        row = height + 7;

        while (row--) {
            const pel *p = src;
            uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
            for (i = 0; i < width; i += 16) {
                S0 = _mm256_loadu_si256((__m256i*)(p));
                S1 = _mm256_loadu_si256((__m256i*)(p + 4));
                S2 = _mm256_loadu_si256((__m256i*)(p + 8));

                T0 = _mm256_shuffle_epi8(S0, mShuffle0);
                T1 = _mm256_shuffle_epi8(S0, mShuffle1);
                T2 = _mm256_shuffle_epi8(S1, mShuffle0);
                T3 = _mm256_shuffle_epi8(S1, mShuffle1);
                T4 = _mm256_shuffle_epi8(S2, mShuffle0);
                T5 = _mm256_shuffle_epi8(S2, mShuffle1);

                M0 = _mm256_madd_epi16(T0, mCoef0);
                M1 = _mm256_madd_epi16(T1, mCoef1);
                M2 = _mm256_madd_epi16(T2, mCoef2);
                M3 = _mm256_madd_epi16(T3, mCoef3);
                M4 = _mm256_madd_epi16(T2, mCoef0);
                M5 = _mm256_madd_epi16(T3, mCoef1);
                M6 = _mm256_madd_epi16(T4, mCoef2);
                M7 = _mm256_madd_epi16(T5, mCoef3);

                M0 = _mm256_add_epi32(M0, M1);
                M1 = _mm256_add_epi32(M2, M3);
                M2 = _mm256_add_epi32(M4, M5);
                M3 = _mm256_add_epi32(M6, M7);

                M0 = _mm256_add_epi32(M0, M1);
                M1 = _mm256_add_epi32(M2, M3);

                p += 16;
                M2 = _mm256_add_epi32(M0, offset);
                M3 = _mm256_add_epi32(M1, offset);
                M2 = _mm256_srai_epi32(M2, shift1);
                M3 = _mm256_srai_epi32(M3, shift1);
                M2 = _mm256_packs_epi32(M2, M3);
                _mm256_storeu_si256((__m256i*)(tmp + i), M2);
            }
            tmp += i_tmp;
            src += i_src;
        }
    }

    {
        __m256i N0, N1, N2, N3, N4, N5, N6, N7;
        __m256i T6, T7;
        offset = _mm256_set1_epi32(add2);
        tmp = tmp_res;

        mCoef0 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[0]));
        mCoef1 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[1]));
        mCoef2 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[2]));
        mCoef3 = _mm256_cvtepi8_epi16(_mm_set1_epi16(((short*)coef_y)[3]));

        while (height--) {
            const pel *p = (pel*)tmp;
            for (i = 0; i < width; i += 16) {
                T0 = _mm256_load_si256((__m256i*)(p));
                T1 = _mm256_load_si256((__m256i*)(p + i_tmp));
                T2 = _mm256_load_si256((__m256i*)(p + 2 * i_tmp));
                T3 = _mm256_load_si256((__m256i*)(p + 3 * i_tmp));
                T4 = _mm256_load_si256((__m256i*)(p + 4 * i_tmp));
                T5 = _mm256_load_si256((__m256i*)(p + 5 * i_tmp));
                T6 = _mm256_load_si256((__m256i*)(p + 6 * i_tmp));
                T7 = _mm256_load_si256((__m256i*)(p + 7 * i_tmp));

                M0 = _mm256_unpacklo_epi16(T0, T1);
                M1 = _mm256_unpacklo_epi16(T2, T3);
                M2 = _mm256_unpacklo_epi16(T4, T5);
                M3 = _mm256_unpacklo_epi16(T6, T7);
                M4 = _mm256_unpackhi_epi16(T0, T1);
                M5 = _mm256_unpackhi_epi16(T2, T3);
                M6 = _mm256_unpackhi_epi16(T4, T5);
                M7 = _mm256_unpackhi_epi16(T6, T7);

                N0 = _mm256_madd_epi16(M0, mCoef0);
                N1 = _mm256_madd_epi16(M1, mCoef1);
                N2 = _mm256_madd_epi16(M2, mCoef2);
                N3 = _mm256_madd_epi16(M3, mCoef3);
                N4 = _mm256_madd_epi16(M4, mCoef0);
                N5 = _mm256_madd_epi16(M5, mCoef1);
                N6 = _mm256_madd_epi16(M6, mCoef2);
                N7 = _mm256_madd_epi16(M7, mCoef3);

                N0 = _mm256_add_epi32(N0, N1);
                N1 = _mm256_add_epi32(N2, N3);
                N2 = _mm256_add_epi32(N4, N5);
                N3 = _mm256_add_epi32(N6, N7);

                N0 = _mm256_add_epi32(N0, N1);
                N1 = _mm256_add_epi32(N2, N3);

                N0 = _mm256_add_epi32(N0, offset);
                N1 = _mm256_add_epi32(N1, offset);
                N0 = _mm256_srai_epi32(N0, shift2);
                N1 = _mm256_srai_epi32(N1, shift2);
                N0 = _mm256_packus_epi32(N0, N1);
                N0 = _mm256_min_epu16(N0, max_pel);
                _mm256_storeu_si256((__m256i*)(dst + i), N0);

                p += 16;
            }
            dst += i_dst;
            tmp += i_tmp;
        }
    }
}

void uavs3d_if_hor_ver_chroma_w8_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(64 + 3) * 8]);
    s16 *tmp = tmp_res;
    const int i_tmp = 8;
    const int i_tmp2 = 16;
    const int i_tmp3 = 24;
    int row;
    int shift1, shift2;
    int add1, add2;

    if (max_val == 255) { // 8 bit_depth
        shift1 = 0;
        shift2 = 12;
    }
    else { // 10 bit_depth
        shift1 = 2;
        shift2 = 10;
    }

    add1 = (1 << (shift1)) >> 1;
    add2 = 1 << (shift2 - 1);

    //HOR
    __m128i coef0 = _mm_cvtepi8_epi16(_mm_set1_epi16(((s16*)coef_x)[0]));
    __m128i coef1 = _mm_cvtepi8_epi16(_mm_set1_epi16(((s16*)coef_x)[1]));
    __m256i mCoef0 = _mm256_set_m128i(coef1, coef0);
    __m256i mCoef1 = _mm256_set_m128i(coef0, coef1);
    __m256i mSwitch = _mm256_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11, 0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11);
    __m256i T0, T1, S0, S1, sum;
    __m256i mAddOffset = _mm256_set1_epi32(add1);
    __m128i mDst;
    __m128i s0;

    src = src - i_src - 2;
    row = height + 3;
    while (row--) {
        S0 = _mm256_loadu_si256((__m256i*)(src));
        s0 = _mm_loadu_si128((__m128i*)(src + 4));
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        S1 = _mm256_set_m128i(s0, s0);
        T0 = _mm256_shuffle_epi8(S0, mSwitch);
        T1 = _mm256_shuffle_epi8(S1, mSwitch);
        T0 = _mm256_madd_epi16(T0, mCoef0);
        T1 = _mm256_madd_epi16(T1, mCoef1);
        sum = _mm256_add_epi32(T0, T1);

        sum = _mm256_add_epi32(sum, mAddOffset);
        sum = _mm256_srai_epi32(sum, shift1);
        mDst = _mm_packs_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
        _mm_store_si128((__m128i*)(tmp), mDst);

        src += i_src;
        tmp += i_tmp;
    }

    // VER
    tmp = tmp_res;

    {
        __m128i coeff0, coeff1;
        __m256i MaxVal = _mm256_set1_epi16((pel)max_val);
        __m256i C0, C1, mVal, mAddOffset2;
        __m256i M0, M1, M2, M3;

        coeff0 = _mm_set1_epi16(*(s16*)coef_y);
        coeff1 = _mm_set1_epi16(*(s16*)(coef_y + 2));
        mAddOffset2 = _mm256_set1_epi32(add2);

        C0 = _mm256_cvtepi8_epi16(coeff0);
        C1 = _mm256_cvtepi8_epi16(coeff1);
        while (height) {
            __m256i T00 = _mm256_load_si256((__m256i*)(tmp));
            __m256i T10 = _mm256_loadu_si256((__m256i*)(tmp + i_tmp));
            __m256i T20 = _mm256_load_si256((__m256i*)(tmp + i_tmp2));
            __m256i T30 = _mm256_loadu_si256((__m256i*)(tmp + i_tmp3));

            M0 = _mm256_unpacklo_epi16(T00, T10);
            M1 = _mm256_unpacklo_epi16(T20, T30);
            M2 = _mm256_unpackhi_epi16(T00, T10);
            M3 = _mm256_unpackhi_epi16(T20, T30);
            
            M0 = _mm256_madd_epi16(M0, C0);
            M1 = _mm256_madd_epi16(M1, C1);
            M2 = _mm256_madd_epi16(M2, C0);
            M3 = _mm256_madd_epi16(M3, C1);
            
            M0 = _mm256_add_epi32(M0, M1);
            M2 = _mm256_add_epi32(M2, M3);
            
            M0 = _mm256_add_epi32(M0, mAddOffset2);
            M2 = _mm256_add_epi32(M2, mAddOffset2);
            M0 = _mm256_srai_epi32(M0, shift2);
            M2 = _mm256_srai_epi32(M2, shift2);

            mVal = _mm256_packus_epi32(M0, M2);
            mVal = _mm256_min_epu16(mVal, MaxVal);
            _mm_storeu_si128((__m128i*)dst, _mm256_castsi256_si128(mVal));
            _mm_storeu_si128((__m128i*)(dst + i_dst), _mm256_extracti128_si256(mVal, 1));

            height -= 2;
            tmp += i_tmp2;
            dst += i_dst << 1;
        }
    }
}

void uavs3d_if_hor_ver_chroma_w16x_avx2(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    ALIGNED_32(s16 tmp_res[(128 + 3) * 128]);
    s16 *tmp = tmp_res;
    const int i_tmp = width;
    const int i_tmp2 = width << 1;
    const int i_tmp3 = width + i_tmp2;
    int row, col;
    int shift1, shift2;
    int add1, add2;

    __m128i coef0 = _mm_set1_epi16(*(s16*)coef_x);
    __m128i coef1 = _mm_set1_epi16(*(s16*)(coef_x + 2));
    __m256i mCoef0 = _mm256_cvtepi8_epi16(coef0);
    __m256i mCoef1 = _mm256_cvtepi8_epi16(coef1);
    __m256i mSwitch = _mm256_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11, 0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11);
    __m256i T0, T1, T2, T3, S0, S1, S2, S3;
    __m256i mAddOffset;
    __m256i max_val1 = _mm256_set1_epi16((pel)max_val);
    __m256i mVal1, mVal2, mVal;
    __m128i mCoefy11, mCoefy22;
    __m256i mCoefy1, mCoefy2;

    if (max_val == 255) { // 8 bit_depth
        shift1 = 0;
        shift2 = 12;
    }
    else { // 10 bit_depth
        shift1 = 2;
        shift2 = 10;
    }

    add1 = (1 << (shift1)) >> 1;
    add2 = 1 << (shift2 - 1);

    mAddOffset = _mm256_set1_epi32(add1);
    //HOR
    src = src - i_src - 2;
    row = height + 3;
    while (row--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 16) {
            S0 = _mm256_loadu_si256((__m256i*)(src + col));
            S1 = _mm256_loadu_si256((__m256i*)(src + col + 4));
            S2 = _mm256_loadu_si256((__m256i*)(src + col + 8));
            T0 = _mm256_shuffle_epi8(S0, mSwitch);
            T1 = _mm256_shuffle_epi8(S1, mSwitch);
            T2 = _mm256_shuffle_epi8(S1, mSwitch);
            T3 = _mm256_shuffle_epi8(S2, mSwitch);
            S0 = _mm256_madd_epi16(T0, mCoef0);
            S1 = _mm256_madd_epi16(T1, mCoef1);
            S2 = _mm256_madd_epi16(T2, mCoef0);
            S3 = _mm256_madd_epi16(T3, mCoef1);
            T0 = _mm256_add_epi32(S0, S1);
            T2 = _mm256_add_epi32(S2, S3);

            T0 = _mm256_add_epi32(T0, mAddOffset);
            T2 = _mm256_add_epi32(T2, mAddOffset);
            T0 = _mm256_srai_epi32(T0, shift1);
            T2 = _mm256_srai_epi32(T2, shift1);
            T0 = _mm256_packs_epi32(T0, T2);
            _mm256_store_si256((__m256i*)(tmp + col), T0);
        }
        src += i_src;
        tmp += i_tmp;
    }

    // VER
    tmp = tmp_res;
    mAddOffset = _mm256_set1_epi32(add2);

    mCoefy11 = _mm_set1_epi16(*(s16*)coef_y);
    mCoefy22 = _mm_set1_epi16(*(s16*)(coef_y + 2));
    mCoefy1 = _mm256_cvtepi8_epi16(mCoefy11);
    mCoefy2 = _mm256_cvtepi8_epi16(mCoefy22);

    while (height--) {
        for (col = 0; col < width; col += 16)
        {
            S0 = _mm256_load_si256((__m256i*)(tmp + col));
            S1 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp));
            S2 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp2));
            S3 = _mm256_load_si256((__m256i*)(tmp + col + i_tmp3));

            T0 = _mm256_unpacklo_epi16(S0, S1);
            T1 = _mm256_unpacklo_epi16(S2, S3);
            T2 = _mm256_unpackhi_epi16(S0, S1);
            T3 = _mm256_unpackhi_epi16(S2, S3);

            T0 = _mm256_madd_epi16(T0, mCoefy1);
            T1 = _mm256_madd_epi16(T1, mCoefy2);
            T2 = _mm256_madd_epi16(T2, mCoefy1);
            T3 = _mm256_madd_epi16(T3, mCoefy2);

            mVal1 = _mm256_add_epi32(T0, T1);
            mVal2 = _mm256_add_epi32(T2, T3);

            mVal1 = _mm256_add_epi32(mVal1, mAddOffset);
            mVal2 = _mm256_add_epi32(mVal2, mAddOffset);
            mVal1 = _mm256_srai_epi32(mVal1, shift2);
            mVal2 = _mm256_srai_epi32(mVal2, shift2);

            mVal = _mm256_packus_epi32(mVal1, mVal2);
            mVal = _mm256_min_epu16(mVal, max_val1);
            _mm256_storeu_si256((__m256i*)(dst + col), mVal);
        }
        tmp += i_tmp;
        dst += i_dst;
    }
}

#endif
