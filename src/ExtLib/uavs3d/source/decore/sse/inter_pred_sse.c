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

void uavs3d_if_cpy_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int col;

    while (height--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 4) {
            CP32(dst + col, src + col);
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_cpy_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    if (height < 4) {
        CP32(dst, src);
        CP32(dst + i_dst, src + i_src);
    }
    else {
        int i_src2 = i_src << 1;
        int i_dst2 = i_dst << 1;
        int i_src3 = i_src + i_src2;
        int i_dst3 = i_dst + i_dst2;
        int i_src4 = i_src << 2;
        int i_dst4 = i_dst << 2;
        while (height > 0) {
            CP32(dst, src);
            CP32(dst + i_dst, src + i_src);
            CP32(dst + i_dst2, src + i_src2);
            CP32(dst + i_dst3, src + i_src3);
            height -= 4;
            src += i_src4;
            dst += i_dst4;
        }
    }
}

void uavs3d_if_cpy_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    if (height < 4) {
        __m128i m0, m1;
        m0 = _mm_loadl_epi64((const __m128i*)src);
        m1 = _mm_loadl_epi64((const __m128i*)(src + i_src));
        _mm_storel_epi64((__m128i*)dst, m0);
        _mm_storel_epi64((__m128i*)(dst + i_dst), m1);
    } else {
        __m128i m0, m1, m2, m3;
        int i_src2 = i_src << 1;
        int i_dst2 = i_dst << 1;
        int i_src3 = i_src + i_src2;
        int i_dst3 = i_dst + i_dst2;
        int i_src4 = i_src << 2;
        int i_dst4 = i_dst << 2;
        while (height) {
            m0 = _mm_loadl_epi64((const __m128i*)src);
            m1 = _mm_loadl_epi64((const __m128i*)(src + i_src));
            m2 = _mm_loadl_epi64((const __m128i*)(src + i_src2));
            m3 = _mm_loadl_epi64((const __m128i*)(src + i_src3));
            height -= 4;
            src += i_src4;

            _mm_storel_epi64((__m128i*)dst, m0);
            _mm_storel_epi64((__m128i*)(dst + i_dst), m1);
            _mm_storel_epi64((__m128i*)(dst + i_dst2), m2);
            _mm_storel_epi64((__m128i*)(dst + i_dst3), m3);
            dst += i_dst4;
        }
    }
}

void uavs3d_if_cpy_w16_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    int i_src3 = i_src2 + i_src;
    int i_dst3 = i_dst2 + i_dst;
    int i_src4 = i_src << 2;
    int i_dst4 = i_dst << 2;
    __m128i m0, m1, m2, m3;
    while (height) {
        m0 = _mm_loadu_si128((const __m128i*)(src));
        m1 = _mm_loadu_si128((const __m128i*)(src + i_src));
        m2 = _mm_loadu_si128((const __m128i*)(src + i_src2));
        m3 = _mm_loadu_si128((const __m128i*)(src + i_src3));
        _mm_storeu_si128((__m128i*)(dst), m0);
        _mm_storeu_si128((__m128i*)(dst + i_dst), m1);
        _mm_storeu_si128((__m128i*)(dst + i_dst2), m2);
        _mm_storeu_si128((__m128i*)(dst + i_dst3), m3);
        src += i_src4;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_if_cpy_w16x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int col;

    while (height--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 16) {
            CP128(dst + col, src + col);
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_chroma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int col;
    const int offset = 32;
    const int shift = 6;

    __m128i mAddOffset = _mm_set1_epi16(offset);
	__m128i mSwitch1 = _mm_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
	__m128i mSwitch2 = _mm_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);

	__m128i mCoefy1_hor = _mm_set1_epi16(*(s16*)coeff);
	__m128i mCoefy2_hor = _mm_set1_epi16(*(s16*)(coeff + 2));
 
    src -= 2;

    while (height--) {
        __m128i mT20, mT40, mSum, mVal;
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        for (col = 0; col < width; col += 8) {

            __m128i mSrc = _mm_loadu_si128((__m128i*)(src + col));
            
			mT20 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch1), mCoefy1_hor);
			mT40 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch2), mCoefy2_hor);
       
            mSum = _mm_add_epi16(mT20, mT40);
            mVal = _mm_add_epi16(mSum, mAddOffset);

            mVal = _mm_srai_epi16(mVal, shift);
			mVal = _mm_packus_epi16(mVal, mVal);

			_mm_storel_epi64((__m128i*)&dst[col], mVal);
        }

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_chroma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;

    __m128i mAddOffset = _mm_set1_epi16(offset);
    __m128i mSwitch1 = _mm_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
    __m128i mSwitch2 = _mm_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);

    __m128i mCoefy1_hor = _mm_set1_epi16(*(s16*)coeff);
    __m128i mCoefy2_hor = _mm_set1_epi16(*(s16*)(coeff + 2));

    src -= 2;

    while (height) {
        __m128i m0, m1, m2, m3;
        __m128i mSrc0, mSrc1;
        uavs3d_prefetch(src + i_src*2, _MM_HINT_NTA);

        mSrc0 = _mm_loadu_si128((__m128i*)(src));
        mSrc1 = _mm_loadu_si128((__m128i*)(src + i_src));

        m0 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc0, mSwitch1), mCoefy1_hor);
        m1 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc0, mSwitch2), mCoefy2_hor);
        m2 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc1, mSwitch1), mCoefy1_hor);
        m3 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc1, mSwitch2), mCoefy2_hor);

        m0 = _mm_add_epi16(m0, m1);
        m2 = _mm_add_epi16(m2, m3);
        m0 = _mm_add_epi16(m0, mAddOffset);
        m2 = _mm_add_epi16(m2, mAddOffset);

        m0 = _mm_srai_epi16(m0, shift);
        m2 = _mm_srai_epi16(m2, shift);
        m0 = _mm_packus_epi16(m0, m2);

        _mm_storel_epi64((__m128i*)dst, m0);
        _mm_storel_epi64((__m128i*)(dst + i_dst), _mm_srli_si128(m0, 8));
        
        height -= 2;
        src += i_src << 1;
        dst += i_dst << 1;
    }
}

void uavs3d_if_hor_luma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int col;
    const int offset = 32;
    const int shift = 6;

    __m128i mAddOffset = _mm_set1_epi16(offset);

	__m128i mSwitch1 = _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
	__m128i mSwitch2 = _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
	__m128i mSwitch3 = _mm_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
	__m128i mSwitch4 = _mm_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);

	__m128i mCoefy1_hor = _mm_set1_epi16(*(s16*)coeff);
	__m128i mCoefy2_hor = _mm_set1_epi16(*(s16*)(coeff + 2));
	__m128i mCoefy3_hor = _mm_set1_epi16(*(s16*)(coeff + 4));
	__m128i mCoefy4_hor = _mm_set1_epi16(*(s16*)(coeff + 6));

    src -= 3;
    while (height--) {

        __m128i T20, T40, T60, T80, s1, s2, sum, val;

        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 8) {

			__m128i srcCoeff = _mm_loadu_si128((__m128i*)(src + col));

			T20 = _mm_maddubs_epi16(_mm_shuffle_epi8(srcCoeff, mSwitch1), mCoefy1_hor);
			T40 = _mm_maddubs_epi16(_mm_shuffle_epi8(srcCoeff, mSwitch2), mCoefy2_hor);
			T60 = _mm_maddubs_epi16(_mm_shuffle_epi8(srcCoeff, mSwitch3), mCoefy3_hor);
			T80 = _mm_maddubs_epi16(_mm_shuffle_epi8(srcCoeff, mSwitch4), mCoefy4_hor);

            s1 = _mm_add_epi16(T20, T40);
            s2 = _mm_add_epi16(T60, T80);
            sum = _mm_add_epi16(s1, s2);

            val = _mm_add_epi16(sum, mAddOffset);

            val = _mm_srai_epi16(val, shift);
			val = _mm_packus_epi16(val, val);

			_mm_storel_epi64((__m128i*)&dst[col], val);
        }

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_luma_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    
    __m128i mAddOffset = _mm_set1_epi16(offset);

    __m128i mSwitch1 = _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12);
    __m128i mSwitch2 = _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14);

    __m128i mCoefy1_hor = _mm_set1_epi16(*(s16*)coeff);
    __m128i mCoefy2_hor = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i mCoefy3_hor = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i mCoefy4_hor = _mm_set1_epi16(*(s16*)(coeff + 6));

    src -= 3;
    while (height) {
        __m128i T0, T1, T2, T3;
        __m128i m0, m1;
        __m128i src0, src1, src2, src3;
        
        uavs3d_prefetch(src + i_src*2, _MM_HINT_NTA);
        src0 = _mm_loadu_si128((__m128i*)(src));
        src1 = _mm_loadu_si128((__m128i*)(src + i_src));
        src2 = _mm_srli_si128(src0, 4);
        src3 = _mm_srli_si128(src1, 4);

        m0 = _mm_unpacklo_epi64(src0, src1);
        m1 = _mm_unpacklo_epi64(src2, src3);

        T0 = _mm_shuffle_epi8(m0, mSwitch1);
        T1 = _mm_shuffle_epi8(m0, mSwitch2);
        T2 = _mm_shuffle_epi8(m1, mSwitch1);
        T3 = _mm_shuffle_epi8(m1, mSwitch2);

        T0 = _mm_maddubs_epi16(T0, mCoefy1_hor);
        T1 = _mm_maddubs_epi16(T1, mCoefy2_hor);
        T2 = _mm_maddubs_epi16(T2, mCoefy3_hor);
        T3 = _mm_maddubs_epi16(T3, mCoefy4_hor);

        T0 = _mm_add_epi16(T0, T1);
        T2 = _mm_add_epi16(T2, T3);
        
        T0 = _mm_add_epi16(T0, T2);
        
        T0 = _mm_add_epi16(T0, mAddOffset);

        T0 = _mm_srai_epi16(T0, shift);
        T0 = _mm_packus_epi16(T0, T0);

        height -= 2;
        src += i_src << 1;

        M32(dst) = _mm_extract_epi32(T0, 0);
        M32(dst + i_dst) = _mm_extract_epi32(T0, 1);

        dst += i_dst << 1;
    }
}
void uavs3d_if_hor_luma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;

    __m128i mAddOffset = _mm_set1_epi16(offset);

    __m128i mSwitch1 = _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
    __m128i mSwitch2 = _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
    __m128i mSwitch3 = _mm_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
    __m128i mSwitch4 = _mm_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);

    __m128i mCoefy1_hor = _mm_set1_epi16(*(s16*)coeff);
    __m128i mCoefy2_hor = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i mCoefy3_hor = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i mCoefy4_hor = _mm_set1_epi16(*(s16*)(coeff + 6));

    src -= 3;
    while (height) {

        __m128i T0, T1, T2, T3;
        __m128i m0, m1, m2, m3;
        __m128i src0, src1;

        uavs3d_prefetch(src + i_src * 2, _MM_HINT_NTA);
        src0 = _mm_loadu_si128((__m128i*)(src));
        src1 = _mm_loadu_si128((__m128i*)(src + i_src));
        T0 = _mm_shuffle_epi8(src0, mSwitch1);
        T1 = _mm_shuffle_epi8(src0, mSwitch2);
        T2 = _mm_shuffle_epi8(src0, mSwitch3);
        T3 = _mm_shuffle_epi8(src0, mSwitch4);
        m0 = _mm_shuffle_epi8(src1, mSwitch1);
        m1 = _mm_shuffle_epi8(src1, mSwitch2);
        m2 = _mm_shuffle_epi8(src1, mSwitch3);
        m3 = _mm_shuffle_epi8(src1, mSwitch4);

        T0 = _mm_maddubs_epi16(T0, mCoefy1_hor);
        T1 = _mm_maddubs_epi16(T1, mCoefy2_hor);
        T2 = _mm_maddubs_epi16(T2, mCoefy3_hor);
        T3 = _mm_maddubs_epi16(T3, mCoefy4_hor);
        m0 = _mm_maddubs_epi16(m0, mCoefy1_hor);
        m1 = _mm_maddubs_epi16(m1, mCoefy2_hor);
        m2 = _mm_maddubs_epi16(m2, mCoefy3_hor);
        m3 = _mm_maddubs_epi16(m3, mCoefy4_hor);

        T0 = _mm_add_epi16(T0, T1);
        T2 = _mm_add_epi16(T2, T3);
        m0 = _mm_add_epi16(m0, m1);
        m2 = _mm_add_epi16(m2, m3);
        T0 = _mm_add_epi16(T0, T2);
        m0 = _mm_add_epi16(m0, m2);

        T0 = _mm_add_epi16(T0, mAddOffset);
        m0 = _mm_add_epi16(m0, mAddOffset);

        T0 = _mm_srai_epi16(T0, shift);
        m0 = _mm_srai_epi16(m0, shift);
        T0 = _mm_packus_epi16(T0, m0);

        height -= 2;

        _mm_storel_epi64((__m128i*)dst, T0);
        _mm_storeh_pi((__m64*)(dst + i_dst), _mm_castsi128_ps(T0));

        src += i_src << 1;
        dst += i_dst << 1;
    }
}

void uavs3d_if_ver_chroma_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi16(offset);
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i mVal;
    __m128i m0, m1, m2, m3;

    src -= i_src;

    while (height > 0) {
        __m128i T0 = _mm_loadl_epi64((__m128i*)(src));
        __m128i T1 = _mm_loadl_epi64((__m128i*)(src + i_src));
        __m128i T2 = _mm_loadl_epi64((__m128i*)(src + 2 * i_src));
        __m128i T3 = _mm_loadl_epi64((__m128i*)(src + 3 * i_src));
        __m128i T4 = _mm_loadl_epi64((__m128i*)(src + 4 * i_src));
        uavs3d_prefetch(src + 5 * i_src, _MM_HINT_NTA);

        m0 = _mm_unpacklo_epi8(T0, T1);
        m1 = _mm_unpacklo_epi8(T2, T3);
        m2 = _mm_unpacklo_epi8(T1, T2);
        m3 = _mm_unpacklo_epi8(T3, T4);

        T0 = _mm_unpacklo_epi64(m0, m2);
        T1 = _mm_unpacklo_epi64(m1, m3);

        T0 = _mm_maddubs_epi16(T0, coeff0);
        T1 = _mm_maddubs_epi16(T1, coeff1);

        mVal = _mm_add_epi16(T0, T1);

        mVal = _mm_add_epi16(mVal, mAddOffset);
        mVal = _mm_srai_epi16(mVal, shift);
        mVal = _mm_packus_epi16(mVal, mVal);

        height -= 2;

        M32(dst) = _mm_extract_epi32(mVal, 0);
        M32(dst + i_dst) = _mm_extract_epi32(mVal, 1);

        src += i_src << 1;
        dst += i_dst << 1;
    }
}

void uavs3d_if_ver_chroma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi16(offset);
    __m128i coeff0 = _mm_set1_epi16(*(s32*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s32*)(coeff + 2));
    __m128i mVal;

    src -= i_src;

    while (height--) {
        __m128i T00 = _mm_loadl_epi64((__m128i*)(src));
        __m128i T10 = _mm_loadl_epi64((__m128i*)(src + i_src));
        __m128i T20 = _mm_loadl_epi64((__m128i*)(src + 2 * i_src));
        __m128i T30 = _mm_loadl_epi64((__m128i*)(src + 3 * i_src));
        uavs3d_prefetch(src + 4 * i_src, _MM_HINT_NTA);

        T00 = _mm_unpacklo_epi8(T00, T10);
        T10 = _mm_unpacklo_epi8(T20, T30);

        T00 = _mm_maddubs_epi16(T00, coeff0);
        T10 = _mm_maddubs_epi16(T10, coeff1);

        mVal = _mm_add_epi16(T00, T10);

        mVal = _mm_add_epi16(mVal, mAddOffset);
        mVal = _mm_srai_epi16(mVal, shift);
        mVal = _mm_packus_epi16(mVal, mVal);

        _mm_storel_epi64((__m128i*)dst, mVal);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_chroma_w16_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi16(offset);
    __m128i coeff0 = _mm_set1_epi16(*(s32*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s32*)(coeff + 2));
    __m128i mVal1, mVal2;

    src -= i_src;

    while (height--) {
        uavs3d_prefetch(src + 4 * i_src, _MM_HINT_NTA);
        __m128i T01 = _mm_loadu_si128((__m128i*)(src));
        __m128i T11 = _mm_loadu_si128((__m128i*)(src + i_src));
        __m128i T21 = _mm_loadu_si128((__m128i*)(src + 2 * i_src));
        __m128i T31 = _mm_loadu_si128((__m128i*)(src + 3 * i_src));

        __m128i T00 = _mm_unpacklo_epi8(T01, T11);
        __m128i T10 = _mm_unpacklo_epi8(T21, T31);
        __m128i T20 = _mm_unpackhi_epi8(T01, T11);
        __m128i T30 = _mm_unpackhi_epi8(T21, T31);

        T00 = _mm_maddubs_epi16(T00, coeff0);
        T10 = _mm_maddubs_epi16(T10, coeff1);
        T20 = _mm_maddubs_epi16(T20, coeff0);
        T30 = _mm_maddubs_epi16(T30, coeff1);

        mVal1 = _mm_add_epi16(T00, T10);
        mVal2 = _mm_add_epi16(T20, T30);

        mVal1 = _mm_add_epi16(mVal1, mAddOffset);
        mVal1 = _mm_srai_epi16(mVal1, shift);
        mVal2 = _mm_add_epi16(mVal2, mAddOffset);
        mVal2 = _mm_srai_epi16(mVal2, shift);
        mVal1 = _mm_packus_epi16(mVal1, mVal2);

        _mm_storeu_si128((__m128i*)dst, mVal1);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_chroma_w16x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
	int col;
	const int offset = 32;
	const int shift = 6;
	__m128i mAddOffset = _mm_set1_epi16(offset);
	__m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
	__m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
	__m128i mVal1, mVal2;

	src -= i_src;

	while (height--) {
		pel const *p = src;
        uavs3d_prefetch(src + 4 * i_src, _MM_HINT_NTA);
		for (col = 0; col < width; col += 16) {
			__m128i T01 = _mm_loadu_si128((__m128i*)(p));
			__m128i T11 = _mm_loadu_si128((__m128i*)(p + i_src));
			__m128i T21 = _mm_loadu_si128((__m128i*)(p + 2 * i_src));
			__m128i T31 = _mm_loadu_si128((__m128i*)(p + 3 * i_src));

			__m128i T00 = _mm_unpacklo_epi8(T01, T11);
			__m128i T10 = _mm_unpacklo_epi8(T21, T31);
			__m128i T20 = _mm_unpackhi_epi8(T01, T11);
			__m128i T30 = _mm_unpackhi_epi8(T21, T31);

			T00 = _mm_maddubs_epi16(T00, coeff0);
			T10 = _mm_maddubs_epi16(T10, coeff1);
			T20 = _mm_maddubs_epi16(T20, coeff0);
			T30 = _mm_maddubs_epi16(T30, coeff1);

			mVal1 = _mm_add_epi16(T00, T10);
			mVal2 = _mm_add_epi16(T20, T30);

			mVal1 = _mm_add_epi16(mVal1, mAddOffset);
			mVal1 = _mm_srai_epi16(mVal1, shift);
			mVal2 = _mm_add_epi16(mVal2, mAddOffset);
			mVal2 = _mm_srai_epi16(mVal2, shift);
			mVal1 = _mm_packus_epi16(mVal1, mVal2);

			_mm_storeu_si128((__m128i*)&dst[col], mVal1);

			p += 16;
		}

		src += i_src;
		dst += i_dst;
	}
}

void uavs3d_if_ver_luma_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi16(offset);
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));
    __m128i m0, m1, m2, m3, m4, m5, m6, m7;
    __m128i mVal;

    src -= 3 * i_src;

    while (height) {
        __m128i T00 = _mm_loadl_epi64((__m128i*)(src));
        __m128i T10 = _mm_loadl_epi64((__m128i*)(src + i_src));
        __m128i T20 = _mm_loadl_epi64((__m128i*)(src + 2 * i_src));
        __m128i T30 = _mm_loadl_epi64((__m128i*)(src + 3 * i_src));
        __m128i T40 = _mm_loadl_epi64((__m128i*)(src + 4 * i_src));
        __m128i T50 = _mm_loadl_epi64((__m128i*)(src + 5 * i_src));
        __m128i T60 = _mm_loadl_epi64((__m128i*)(src + 6 * i_src));
        __m128i T70 = _mm_loadl_epi64((__m128i*)(src + 7 * i_src));
        __m128i T80 = _mm_loadl_epi64((__m128i*)(src + 8 * i_src));
        uavs3d_prefetch(src + 9 * i_src, _MM_HINT_NTA);

        m0 = _mm_unpacklo_epi8(T00, T10);
        m1 = _mm_unpacklo_epi8(T20, T30);
        m2 = _mm_unpacklo_epi8(T40, T50);
        m3 = _mm_unpacklo_epi8(T60, T70);
        m4 = _mm_unpacklo_epi8(T10, T20);
        m5 = _mm_unpacklo_epi8(T30, T40);
        m6 = _mm_unpacklo_epi8(T50, T60);
        m7 = _mm_unpacklo_epi8(T70, T80);

        T00 = _mm_unpacklo_epi64(m0, m4);
        T10 = _mm_unpacklo_epi64(m1, m5);
        T20 = _mm_unpacklo_epi64(m2, m6);
        T30 = _mm_unpacklo_epi64(m3, m7);

        T00 = _mm_maddubs_epi16(T00, coeff0);
        T10 = _mm_maddubs_epi16(T10, coeff1);
        T20 = _mm_maddubs_epi16(T20, coeff2);
        T30 = _mm_maddubs_epi16(T30, coeff3);

        mVal = _mm_add_epi16(T00, T10);
        mVal = _mm_add_epi16(mVal, T20);
        mVal = _mm_add_epi16(mVal, T30);

        mVal = _mm_add_epi16(mVal, mAddOffset);
        mVal = _mm_srai_epi16(mVal, shift);
        mVal = _mm_packus_epi16(mVal, mVal);

        height -= 2;

        M32(dst) = _mm_extract_epi32(mVal, 0);
        M32(dst + i_dst) = _mm_extract_epi32(mVal, 1);

        src += i_src << 1;
        dst += i_dst << 1;
    }
}

void uavs3d_if_ver_luma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi16(offset);
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));
    __m128i mVal;

    src -= 3 * i_src;

    while (height--) {
        __m128i T00 = _mm_loadl_epi64((__m128i*)(src));
        __m128i T10 = _mm_loadl_epi64((__m128i*)(src + i_src));
        __m128i T20 = _mm_loadl_epi64((__m128i*)(src + 2 * i_src));
        __m128i T30 = _mm_loadl_epi64((__m128i*)(src + 3 * i_src));
        __m128i T40 = _mm_loadl_epi64((__m128i*)(src + 4 * i_src));
        __m128i T50 = _mm_loadl_epi64((__m128i*)(src + 5 * i_src));
        __m128i T60 = _mm_loadl_epi64((__m128i*)(src + 6 * i_src));
        __m128i T70 = _mm_loadl_epi64((__m128i*)(src + 7 * i_src));
        uavs3d_prefetch(src + 8 * i_src, _MM_HINT_NTA);

        T00 = _mm_unpacklo_epi8(T00, T10);
        T10 = _mm_unpacklo_epi8(T20, T30);
        T20 = _mm_unpacklo_epi8(T40, T50);
        T30 = _mm_unpacklo_epi8(T60, T70);

        T00 = _mm_maddubs_epi16(T00, coeff0);
        T10 = _mm_maddubs_epi16(T10, coeff1);
        T20 = _mm_maddubs_epi16(T20, coeff2);
        T30 = _mm_maddubs_epi16(T30, coeff3);

        mVal = _mm_add_epi16(T00, T10);
        mVal = _mm_add_epi16(mVal, T20);
        mVal = _mm_add_epi16(mVal, T30);

        mVal = _mm_add_epi16(mVal, mAddOffset);
        mVal = _mm_srai_epi16(mVal, shift);
        mVal = _mm_packus_epi16(mVal, mVal);

        _mm_storel_epi64((__m128i*)dst, mVal);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_luma_w16_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi16(offset);

    src -= 3 * i_src;

    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));
    __m128i mVal1, mVal2;
    while (height--) {
        __m128i T01 = _mm_loadu_si128((__m128i*)(src));
        __m128i T11 = _mm_loadu_si128((__m128i*)(src + i_src));
        __m128i T21 = _mm_loadu_si128((__m128i*)(src + 2 * i_src));
        __m128i T31 = _mm_loadu_si128((__m128i*)(src + 3 * i_src));
        __m128i T41 = _mm_loadu_si128((__m128i*)(src + 4 * i_src));
        __m128i T51 = _mm_loadu_si128((__m128i*)(src + 5 * i_src));
        __m128i T61 = _mm_loadu_si128((__m128i*)(src + 6 * i_src));
        __m128i T71 = _mm_loadu_si128((__m128i*)(src + 7 * i_src));

        __m128i T00 = _mm_unpacklo_epi8(T01, T11);
        __m128i T10 = _mm_unpacklo_epi8(T21, T31);
        __m128i T20 = _mm_unpacklo_epi8(T41, T51);
        __m128i T30 = _mm_unpacklo_epi8(T61, T71);

        __m128i T40 = _mm_unpackhi_epi8(T01, T11);
        __m128i T50 = _mm_unpackhi_epi8(T21, T31);
        __m128i T60 = _mm_unpackhi_epi8(T41, T51);
        __m128i T70 = _mm_unpackhi_epi8(T61, T71);

        T00 = _mm_maddubs_epi16(T00, coeff0);
        T10 = _mm_maddubs_epi16(T10, coeff1);
        T20 = _mm_maddubs_epi16(T20, coeff2);
        T30 = _mm_maddubs_epi16(T30, coeff3);

        T40 = _mm_maddubs_epi16(T40, coeff0);
        T50 = _mm_maddubs_epi16(T50, coeff1);
        T60 = _mm_maddubs_epi16(T60, coeff2);
        T70 = _mm_maddubs_epi16(T70, coeff3);

        mVal1 = _mm_add_epi16(T00, T10);
        mVal1 = _mm_add_epi16(mVal1, T20);
        mVal1 = _mm_add_epi16(mVal1, T30);

        mVal2 = _mm_add_epi16(T40, T50);
        mVal2 = _mm_add_epi16(mVal2, T60);
        mVal2 = _mm_add_epi16(mVal2, T70);

        mVal1 = _mm_add_epi16(mVal1, mAddOffset);
        mVal1 = _mm_srai_epi16(mVal1, shift);

        mVal2 = _mm_add_epi16(mVal2, mAddOffset);
        mVal2 = _mm_srai_epi16(mVal2, shift);
        mVal1 = _mm_packus_epi16(mVal1, mVal2);

        _mm_storeu_si128((__m128i*)dst, mVal1);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_luma_w16x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
	int col;
	const int offset = 32;
	const int shift = 6;
	__m128i mAddOffset = _mm_set1_epi16(offset);
	__m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
	__m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
	__m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
	__m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));
	__m128i mVal1, mVal2;

	src -= 3 * i_src;

	while (height--) {
		pel const *p = src;
        uavs3d_prefetch(src + 8 * i_src, _MM_HINT_NTA);
		for (col = 0; col < width; col += 16) {
			__m128i T01 = _mm_loadu_si128((__m128i*)(p));
			__m128i T11 = _mm_loadu_si128((__m128i*)(p + i_src));
			__m128i T21 = _mm_loadu_si128((__m128i*)(p + 2 * i_src));
			__m128i T31 = _mm_loadu_si128((__m128i*)(p + 3 * i_src));
			__m128i T41 = _mm_loadu_si128((__m128i*)(p + 4 * i_src));
			__m128i T51 = _mm_loadu_si128((__m128i*)(p + 5 * i_src));
			__m128i T61 = _mm_loadu_si128((__m128i*)(p + 6 * i_src));
			__m128i T71 = _mm_loadu_si128((__m128i*)(p + 7 * i_src));

			__m128i T00 = _mm_unpacklo_epi8(T01, T11);
			__m128i T10 = _mm_unpacklo_epi8(T21, T31);
			__m128i T20 = _mm_unpacklo_epi8(T41, T51);
			__m128i T30 = _mm_unpacklo_epi8(T61, T71);

			__m128i T40 = _mm_unpackhi_epi8(T01, T11);
			__m128i T50 = _mm_unpackhi_epi8(T21, T31);
			__m128i T60 = _mm_unpackhi_epi8(T41, T51);
			__m128i T70 = _mm_unpackhi_epi8(T61, T71);

			T00 = _mm_maddubs_epi16(T00, coeff0);
			T10 = _mm_maddubs_epi16(T10, coeff1);
			T20 = _mm_maddubs_epi16(T20, coeff2);
			T30 = _mm_maddubs_epi16(T30, coeff3);

			T40 = _mm_maddubs_epi16(T40, coeff0);
			T50 = _mm_maddubs_epi16(T50, coeff1);
			T60 = _mm_maddubs_epi16(T60, coeff2);
			T70 = _mm_maddubs_epi16(T70, coeff3);

			mVal1 = _mm_add_epi16(T00, T10);
			mVal1 = _mm_add_epi16(mVal1, T20);
			mVal1 = _mm_add_epi16(mVal1, T30);

			mVal2 = _mm_add_epi16(T40, T50);
			mVal2 = _mm_add_epi16(mVal2, T60);
			mVal2 = _mm_add_epi16(mVal2, T70);

			mVal1 = _mm_add_epi16(mVal1, mAddOffset);
			mVal1 = _mm_srai_epi16(mVal1, shift);

			mVal2 = _mm_add_epi16(mVal2, mAddOffset);
			mVal2 = _mm_srai_epi16(mVal2, shift);
			mVal1 = _mm_packus_epi16(mVal1, mVal2);

			_mm_storeu_si128((__m128i*)&dst[col], mVal1);

			p += 16;
		}

		src += i_src;
		dst += i_dst;
	}
}

void uavs3d_if_hor_ver_chroma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    int row, col;
    int shift;
    ALIGNED_16(s16 tmp_res[(64 + 3) * 64*2]);
    s16 *tmp = tmp_res;
    const int i_tmp = width;

    __m128i mAddOffset;

	__m128i mSwitch1 = _mm_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
	__m128i mSwitch2 = _mm_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);

	__m128i coeff0_hor = _mm_set1_epi16(*(s16*)coef_x);
	__m128i coeff1_hor = _mm_set1_epi16(*(s16*)(coef_x + 2));
	__m128i coeff0_ver = _mm_set1_epi16(*(s32*)coef_y);
	__m128i coeff1_ver = _mm_set1_epi16(*(s32*)(coef_y + 2));
	__m128i mVal1, mVal2, mVal;

    // HOR
    src = src - 1 * i_src - 2;
    row = height + 3;

    while (row--) {
        __m128i mT0, mT1, mVal;
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 8) {
            __m128i mSrc = _mm_loadu_si128((__m128i*)(src + col));
			mT0 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch1), coeff0_hor);
			mT1 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch2), coeff1_hor);

            mVal = _mm_add_epi16(mT0, mT1);
            _mm_storeu_si128((__m128i*)&tmp[col], mVal);
        }
        src += i_src;
        tmp += i_tmp;
    }
    

    // VER
    shift = 12;
    mAddOffset = _mm_set1_epi32(1 << 11);

    tmp = tmp_res;
	coeff0_ver = _mm_cvtepi8_epi16(coeff0_ver);
	coeff1_ver = _mm_cvtepi8_epi16(coeff1_ver);
		
	while (height--) {
        s16 const *p = tmp;
		for (col = 0; col < width; col += 8) {
			__m128i T00 = _mm_load_si128((__m128i*)(p));
			__m128i T10 = _mm_load_si128((__m128i*)(p + i_tmp));
			__m128i T20 = _mm_load_si128((__m128i*)(p + 2 * i_tmp));
			__m128i T30 = _mm_load_si128((__m128i*)(p + 3 * i_tmp));

			__m128i M0 = _mm_unpacklo_epi16(T00, T10);
			__m128i M1 = _mm_unpacklo_epi16(T20, T30);
			__m128i M2 = _mm_unpackhi_epi16(T00, T10);
			__m128i M3 = _mm_unpackhi_epi16(T20, T30);

			M0 = _mm_madd_epi16(M0, coeff0_ver);
			M1 = _mm_madd_epi16(M1, coeff1_ver);
			M2 = _mm_madd_epi16(M2, coeff0_ver);
			M3 = _mm_madd_epi16(M3, coeff1_ver);

			mVal1 = _mm_add_epi32(M0, M1);
			mVal2 = _mm_add_epi32(M2, M3);

			mVal1 = _mm_add_epi32(mVal1, mAddOffset);
			mVal2 = _mm_add_epi32(mVal2, mAddOffset);
			mVal1 = _mm_srai_epi32(mVal1, shift);
			mVal2 = _mm_srai_epi32(mVal2, shift);
			mVal = _mm_packs_epi32(mVal1, mVal2);
			mVal = _mm_packus_epi16(mVal, mVal);

			_mm_storel_epi64((__m128i*)&dst[col], mVal);

			p += 8;
		}

		tmp += i_tmp;
		dst += i_dst;
	}
}

void uavs3d_if_hor_ver_chroma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
	int row;
	int shift;
	ALIGNED_16(s16 tmp_res[(64 + 3) * 8]);
	s16 *tmp = tmp_res;
	const int i_tmp = 8;

	__m128i mAddOffset;
    __m128i mSwitch1 = _mm_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9);
    __m128i mSwitch2 = _mm_setr_epi8(4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13);

	__m128i coeff0_hor = _mm_set1_epi16(*(s16*)coef_x);
    __m128i coeff1_hor = _mm_set1_epi16(*(s16*)(coef_x + 2));
    __m128i coeff0_ver = _mm_set1_epi16(*(s32*)coef_y);
    __m128i coeff1_ver = _mm_set1_epi16(*(s32*)(coef_y + 2));
    __m128i mVal1, mVal2, mVal;

	// HOR
	src = src - 1 * i_src - 2;
	row = height + 3;

    while (row--) {
        __m128i mT0, mT1, mVal;
        __m128i mSrc = _mm_loadu_si128((__m128i*)(src));

        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        mT0 = _mm_shuffle_epi8(mSrc, mSwitch1);
        mT1 = _mm_shuffle_epi8(mSrc, mSwitch2);
        mT0 = _mm_maddubs_epi16(mT0, coeff0_hor);
        mT1 = _mm_maddubs_epi16(mT1, coeff1_hor);

        mVal = _mm_add_epi16(mT0, mT1);
        _mm_storeu_si128((__m128i*)tmp, mVal);

        src += i_src;
        tmp += i_tmp;
    }
	
	// VER
	shift = 12;
	mAddOffset = _mm_set1_epi32(1 << 11);

	tmp = tmp_res;
	coeff0_ver = _mm_cvtepi8_epi16(coeff0_ver);
	coeff1_ver = _mm_cvtepi8_epi16(coeff1_ver);

    while (height) {
        __m128i T00 = _mm_load_si128((__m128i*)(tmp));
        __m128i T10 = _mm_load_si128((__m128i*)(tmp + i_tmp));
        __m128i T20 = _mm_load_si128((__m128i*)(tmp + 2 * i_tmp));
        __m128i T30 = _mm_load_si128((__m128i*)(tmp + 3 * i_tmp));
        __m128i T40 = _mm_load_si128((__m128i*)(tmp + 4 * i_tmp));

        __m128i M0 = _mm_unpacklo_epi16(T00, T10);
        __m128i M1 = _mm_unpacklo_epi16(T20, T30);
        __m128i M2 = _mm_unpackhi_epi16(T00, T10);
        __m128i M3 = _mm_unpackhi_epi16(T20, T30);

        M0 = _mm_madd_epi16(M0, coeff0_ver);
        M1 = _mm_madd_epi16(M1, coeff1_ver);
        M2 = _mm_madd_epi16(M2, coeff0_ver);
        M3 = _mm_madd_epi16(M3, coeff1_ver);

        mVal1 = _mm_add_epi32(M0, M1);
        mVal2 = _mm_add_epi32(M2, M3);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal2 = _mm_srai_epi32(mVal2, shift);
        mVal = _mm_packs_epi32(mVal1, mVal2);
        mVal = _mm_packus_epi16(mVal, mVal);

        _mm_storel_epi64((__m128i*)dst, mVal);

        M0 = _mm_unpacklo_epi16(T10, T20);
        M1 = _mm_unpacklo_epi16(T30, T40);
        M2 = _mm_unpackhi_epi16(T10, T20);
        M3 = _mm_unpackhi_epi16(T30, T40);

        M0 = _mm_madd_epi16(M0, coeff0_ver);
        M1 = _mm_madd_epi16(M1, coeff1_ver);
        M2 = _mm_madd_epi16(M2, coeff0_ver);
        M3 = _mm_madd_epi16(M3, coeff1_ver);

        mVal1 = _mm_add_epi32(M0, M1);
        mVal2 = _mm_add_epi32(M2, M3);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal2 = _mm_srai_epi32(mVal2, shift);
        mVal = _mm_packs_epi32(mVal1, mVal2);
        mVal = _mm_packus_epi16(mVal, mVal);

        height -= 2;
        _mm_storel_epi64((__m128i*)&dst[i_dst], mVal);

        tmp += 2 * i_tmp;
        dst += 2 * i_dst;
    }
}

void uavs3d_if_hor_ver_luma_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    int row;
    int shift;

    ALIGNED_16(s16 tmp_res[(32 + 7) * 4]);
    s16 *tmp = tmp_res;
    const int i_tmp = 4;

    __m128i mAddOffset;
    __m128i mSwitch12 = _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 2, 3, 3, 4, 4, 5, 5, 6);
    __m128i mSwitch34 = _mm_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 6, 7, 7, 8, 8, 9, 9, 10);

    __m128i mCoefy1_hor = _mm_set1_epi16(*(s16*)coef_x);
    __m128i mCoefy2_hor = _mm_set1_epi16(*(s16*)(coef_x + 2));
    __m128i mCoefy3_hor = _mm_set1_epi16(*(s16*)(coef_x + 4));
    __m128i mCoefy4_hor = _mm_set1_epi16(*(s16*)(coef_x + 6));
    __m128i mCoefy1_ver = _mm_set1_epi16(*(s16*)coef_y);
    __m128i mCoefy2_ver = _mm_set1_epi16(*(s16*)(coef_y + 2));
    __m128i mCoefy3_ver = _mm_set1_epi16(*(s16*)(coef_y + 4));
    __m128i mCoefy4_ver = _mm_set1_epi16(*(s16*)(coef_y + 6));
    __m128i mVal1, mVal;

    // HOR
    src = src - 3 * i_src - 3;
    row = height + 6; // height + 7 - 1

    { // odd number of rows, process 1st row
        __m128i mT0, mT1, mT2, mT3, s1, s2, mVal;
        __m128i mSrc = _mm_loadu_si128((__m128i*)(src));

        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        mT0 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch12), mCoefy1_hor);
        mT1 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, _mm_srli_si128(mSwitch12, 8)), mCoefy2_hor);
        mT2 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch34), mCoefy3_hor);
        mT3 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, _mm_srli_si128(mSwitch34, 8)), mCoefy4_hor);

        s1 = _mm_add_epi16(mT0, mT1);
        s2 = _mm_add_epi16(mT2, mT3);
        mVal = _mm_add_epi16(s1, s2);

        _mm_storel_epi64((__m128i*)tmp, mVal);

        src += i_src;
        tmp += i_tmp;
    }
    while (row) {
        __m128i mT0, mT1, mT2, mT3, s1, s2, swSrc1, swSrc2;
        __m128i mSrc1 = _mm_loadu_si128((__m128i*)(src));
        __m128i mSrc2 = _mm_loadu_si128((__m128i*)(src + i_src));
        row -= 2;
        src += i_src << 1;
        uavs3d_prefetch(src, _MM_HINT_NTA);
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        swSrc1 = _mm_shuffle_epi8(mSrc1, mSwitch12);
        swSrc2 = _mm_shuffle_epi8(mSrc2, mSwitch12);

        mT0 = _mm_maddubs_epi16(_mm_unpacklo_epi64(swSrc1, swSrc2), mCoefy1_hor);
        mT1 = _mm_maddubs_epi16(_mm_unpackhi_epi64(swSrc1, swSrc2), mCoefy2_hor);

        swSrc1 = _mm_shuffle_epi8(mSrc1, mSwitch34);
        swSrc2 = _mm_shuffle_epi8(mSrc2, mSwitch34);

        mT2 = _mm_maddubs_epi16(_mm_unpacklo_epi64(swSrc1, swSrc2), mCoefy3_hor);
        mT3 = _mm_maddubs_epi16(_mm_unpackhi_epi64(swSrc1, swSrc2), mCoefy4_hor);

        s1 = _mm_add_epi16(mT0, mT1);
        s2 = _mm_add_epi16(mT2, mT3);

        _mm_storeu_si128((__m128i*)tmp, _mm_add_epi16(s1, s2));

        tmp += i_tmp << 1;
    }

    // VER
    shift = 12;
    mAddOffset = _mm_set1_epi32(1 << (shift - 1));
    tmp = tmp_res;

    mCoefy1_ver = _mm_cvtepi8_epi16(mCoefy1_ver);
    mCoefy2_ver = _mm_cvtepi8_epi16(mCoefy2_ver);
    mCoefy3_ver = _mm_cvtepi8_epi16(mCoefy3_ver);
    mCoefy4_ver = _mm_cvtepi8_epi16(mCoefy4_ver);

    while (height) {
        __m128i T00  = _mm_loadu_si128((__m128i*)(tmp));
        __m128i T10  = _mm_srli_si128(T00, 8);
        __m128i T20  = _mm_loadu_si128((__m128i*)(tmp + 2 * i_tmp));
        __m128i T30  = _mm_srli_si128(T20, 8);
        __m128i T40  = _mm_loadu_si128((__m128i*)(tmp + 4 * i_tmp));
        __m128i T50  = _mm_srli_si128(T40, 8);
        __m128i T60  = _mm_loadu_si128((__m128i*)(tmp + 6 * i_tmp));
        __m128i T70  = _mm_srli_si128(T60, 8);
        __m128i T80  = _mm_loadu_si128((__m128i*)(tmp + 8 * i_tmp));
        __m128i T90  = _mm_srli_si128(T80, 8);
        __m128i T100 = _mm_loadu_si128((__m128i*)(tmp + 10 * i_tmp));

        __m128i T0 = _mm_unpacklo_epi16(T00, T10);
        __m128i T1 = _mm_unpacklo_epi16(T20, T30);
        __m128i T2 = _mm_unpacklo_epi16(T40, T50);
        __m128i T3 = _mm_unpacklo_epi16(T60, T70);

        height -= 4;

        T0 = _mm_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm_madd_epi16(T2, mCoefy3_ver);
        T3 = _mm_madd_epi16(T3, mCoefy4_ver);

        mVal1 = _mm_add_epi32(T0, T1);
        mVal1 = _mm_add_epi32(mVal1, T2);
        mVal1 = _mm_add_epi32(mVal1, T3);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal = _mm_packs_epi32(mVal1, mVal1);
        mVal = _mm_packus_epi16(mVal, mVal);

        M32(dst) = _mm_extract_epi32(mVal, 0);

        T0 = _mm_unpacklo_epi16(T10, T20);
        T1 = _mm_unpacklo_epi16(T30, T40);
        T2 = _mm_unpacklo_epi16(T50, T60);
        T3 = _mm_unpacklo_epi16(T70, T80);

        T0 = _mm_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm_madd_epi16(T2, mCoefy3_ver);
        T3 = _mm_madd_epi16(T3, mCoefy4_ver);

        mVal1 = _mm_add_epi32(T0, T1);
        mVal1 = _mm_add_epi32(mVal1, T2);
        mVal1 = _mm_add_epi32(mVal1, T3);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal = _mm_packs_epi32(mVal1, mVal1);
        mVal = _mm_packus_epi16(mVal, mVal);

        M32(dst + i_dst) = _mm_extract_epi32(mVal, 0);

        T0 = _mm_unpacklo_epi16(T20, T30);
        T1 = _mm_unpacklo_epi16(T40, T50);
        T2 = _mm_unpacklo_epi16(T60, T70);
        T3 = _mm_unpacklo_epi16(T80, T90);

        T0 = _mm_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm_madd_epi16(T2, mCoefy3_ver);
        T3 = _mm_madd_epi16(T3, mCoefy4_ver);

        mVal1 = _mm_add_epi32(T0, T1);
        mVal1 = _mm_add_epi32(mVal1, T2);
        mVal1 = _mm_add_epi32(mVal1, T3);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal = _mm_packs_epi32(mVal1, mVal1);
        mVal = _mm_packus_epi16(mVal, mVal);

        M32(dst + 2*i_dst) = _mm_extract_epi32(mVal, 0);

        T0 = _mm_unpacklo_epi16(T30, T40);
        T1 = _mm_unpacklo_epi16(T50, T60);
        T2 = _mm_unpacklo_epi16(T70, T80);
        T3 = _mm_unpacklo_epi16(T90, T100);

        T0 = _mm_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm_madd_epi16(T2, mCoefy3_ver);
        T3 = _mm_madd_epi16(T3, mCoefy4_ver);

        mVal1 = _mm_add_epi32(T0, T1);
        mVal1 = _mm_add_epi32(mVal1, T2);
        mVal1 = _mm_add_epi32(mVal1, T3);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal = _mm_packs_epi32(mVal1, mVal1);
        mVal = _mm_packus_epi16(mVal, mVal);

        M32(dst + 3*i_dst) = _mm_extract_epi32(mVal, 0);

        tmp += 4 * i_tmp;
        dst += 4 * i_dst;
    }
}

void uavs3d_if_hor_ver_luma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
	int row;
	int shift;

	ALIGNED_16(s16 tmp_res[(64 + 7) * 8]);
	s16 *tmp = tmp_res;
	const int i_tmp = 8;

	__m128i mAddOffset;

	__m128i mSwitch1 = _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
	__m128i mSwitch2 = _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
	__m128i mSwitch3 = _mm_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
	__m128i mSwitch4 = _mm_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);

	__m128i mCoefy1_hor = _mm_set1_epi16(*(s16*)coef_x);
	__m128i mCoefy2_hor = _mm_set1_epi16(*(s16*)(coef_x + 2));
	__m128i mCoefy3_hor = _mm_set1_epi16(*(s16*)(coef_x + 4));
	__m128i mCoefy4_hor = _mm_set1_epi16(*(s16*)(coef_x + 6));
    __m128i mCoefy1_ver = _mm_set1_epi16(*(s16*)coef_y);
    __m128i mCoefy2_ver = _mm_set1_epi16(*(s16*)(coef_y + 2));
    __m128i mCoefy3_ver = _mm_set1_epi16(*(s16*)(coef_y + 4));
    __m128i mCoefy4_ver = _mm_set1_epi16(*(s16*)(coef_y + 6));
    __m128i mVal1, mVal2, mVal;

	// HOR
	src = src - 3 * i_src - 3;
	row = height + 7;

    while (row--) {
        __m128i mT0, mT1, mT2, mT3, s1, s2, mVal;
        __m128i mSrc = _mm_loadu_si128((__m128i*)(src));

        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        mT0 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch1), mCoefy1_hor);
        mT1 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch2), mCoefy2_hor);
        mT2 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch3), mCoefy3_hor);
        mT3 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch4), mCoefy4_hor);

        s1 = _mm_add_epi16(mT0, mT1);
        s2 = _mm_add_epi16(mT2, mT3);
        mVal = _mm_add_epi16(s1, s2);

        _mm_storeu_si128((__m128i*)tmp, mVal);

        src += i_src;
        tmp += i_tmp;
    }

	// VER
	shift = 12;
	mAddOffset = _mm_set1_epi32(1 << (shift - 1));
	tmp = tmp_res;

	mCoefy1_ver = _mm_cvtepi8_epi16(mCoefy1_ver);
	mCoefy2_ver = _mm_cvtepi8_epi16(mCoefy2_ver);
	mCoefy3_ver = _mm_cvtepi8_epi16(mCoefy3_ver);
	mCoefy4_ver = _mm_cvtepi8_epi16(mCoefy4_ver);

    while (height) {
        __m128i T00 = _mm_load_si128((__m128i*)(tmp));
        __m128i T10 = _mm_load_si128((__m128i*)(tmp + i_tmp));
        __m128i T20 = _mm_load_si128((__m128i*)(tmp + 2 * i_tmp));
        __m128i T30 = _mm_load_si128((__m128i*)(tmp + 3 * i_tmp));
        __m128i T40 = _mm_load_si128((__m128i*)(tmp + 4 * i_tmp));
        __m128i T50 = _mm_load_si128((__m128i*)(tmp + 5 * i_tmp));
        __m128i T60 = _mm_load_si128((__m128i*)(tmp + 6 * i_tmp));
        __m128i T70 = _mm_load_si128((__m128i*)(tmp + 7 * i_tmp));
        __m128i T80 = _mm_load_si128((__m128i*)(tmp + 8 * i_tmp));
        __m128i T90 = _mm_load_si128((__m128i*)(tmp + 9 * i_tmp));
        __m128i T100 = _mm_load_si128((__m128i*)(tmp + 10 * i_tmp));

        __m128i T0 = _mm_unpacklo_epi16(T00, T10);
        __m128i T1 = _mm_unpacklo_epi16(T20, T30);
        __m128i T2 = _mm_unpacklo_epi16(T40, T50);
        __m128i T3 = _mm_unpacklo_epi16(T60, T70);
        __m128i T4 = _mm_unpackhi_epi16(T00, T10);
        __m128i T5 = _mm_unpackhi_epi16(T20, T30);
        __m128i T6 = _mm_unpackhi_epi16(T40, T50);
        __m128i T7 = _mm_unpackhi_epi16(T60, T70);

        T0 = _mm_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm_madd_epi16(T2, mCoefy3_ver);
        T3 = _mm_madd_epi16(T3, mCoefy4_ver);
        T4 = _mm_madd_epi16(T4, mCoefy1_ver);
        T5 = _mm_madd_epi16(T5, mCoefy2_ver);
        T6 = _mm_madd_epi16(T6, mCoefy3_ver);
        T7 = _mm_madd_epi16(T7, mCoefy4_ver);

        mVal1 = _mm_add_epi32(T0, T1);
        mVal1 = _mm_add_epi32(mVal1, T2);
        mVal1 = _mm_add_epi32(mVal1, T3);

        mVal2 = _mm_add_epi32(T4, T5);
        mVal2 = _mm_add_epi32(mVal2, T6);
        mVal2 = _mm_add_epi32(mVal2, T7);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal2 = _mm_srai_epi32(mVal2, shift);
        mVal = _mm_packs_epi32(mVal1, mVal2);
        mVal = _mm_packus_epi16(mVal, mVal);

        _mm_storel_epi64((__m128i*)dst, mVal);

        T0 = _mm_unpacklo_epi16(T10, T20);
        T1 = _mm_unpacklo_epi16(T30, T40);
        T2 = _mm_unpacklo_epi16(T50, T60);
        T3 = _mm_unpacklo_epi16(T70, T80);
        T4 = _mm_unpackhi_epi16(T10, T20);
        T5 = _mm_unpackhi_epi16(T30, T40);
        T6 = _mm_unpackhi_epi16(T50, T60);
        T7 = _mm_unpackhi_epi16(T70, T80);

        T0 = _mm_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm_madd_epi16(T2, mCoefy3_ver);
        T3 = _mm_madd_epi16(T3, mCoefy4_ver);
        T4 = _mm_madd_epi16(T4, mCoefy1_ver);
        T5 = _mm_madd_epi16(T5, mCoefy2_ver);
        T6 = _mm_madd_epi16(T6, mCoefy3_ver);
        T7 = _mm_madd_epi16(T7, mCoefy4_ver);

        mVal1 = _mm_add_epi32(T0, T1);
        mVal1 = _mm_add_epi32(mVal1, T2);
        mVal1 = _mm_add_epi32(mVal1, T3);

        mVal2 = _mm_add_epi32(T4, T5);
        mVal2 = _mm_add_epi32(mVal2, T6);
        mVal2 = _mm_add_epi32(mVal2, T7);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal2 = _mm_srai_epi32(mVal2, shift);
        mVal = _mm_packs_epi32(mVal1, mVal2);
        mVal = _mm_packus_epi16(mVal, mVal);

        _mm_storel_epi64((__m128i*)&dst[i_dst], mVal);

        T0 = _mm_unpacklo_epi16(T20, T30);
        T1 = _mm_unpacklo_epi16(T40, T50);
        T2 = _mm_unpacklo_epi16(T60, T70);
        T3 = _mm_unpacklo_epi16(T80, T90);
        T4 = _mm_unpackhi_epi16(T20, T30);
        T5 = _mm_unpackhi_epi16(T40, T50);
        T6 = _mm_unpackhi_epi16(T60, T70);
        T7 = _mm_unpackhi_epi16(T80, T90);

        T0 = _mm_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm_madd_epi16(T2, mCoefy3_ver);
        T3 = _mm_madd_epi16(T3, mCoefy4_ver);
        T4 = _mm_madd_epi16(T4, mCoefy1_ver);
        T5 = _mm_madd_epi16(T5, mCoefy2_ver);
        T6 = _mm_madd_epi16(T6, mCoefy3_ver);
        T7 = _mm_madd_epi16(T7, mCoefy4_ver);

        mVal1 = _mm_add_epi32(T0, T1);
        mVal1 = _mm_add_epi32(mVal1, T2);
        mVal1 = _mm_add_epi32(mVal1, T3);

        mVal2 = _mm_add_epi32(T4, T5);
        mVal2 = _mm_add_epi32(mVal2, T6);
        mVal2 = _mm_add_epi32(mVal2, T7);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal2 = _mm_srai_epi32(mVal2, shift);
        mVal = _mm_packs_epi32(mVal1, mVal2);
        mVal = _mm_packus_epi16(mVal, mVal);

        _mm_storel_epi64((__m128i*)&dst[2 * i_dst], mVal);

        T0 = _mm_unpacklo_epi16(T30, T40);
        T1 = _mm_unpacklo_epi16(T50, T60);
        T2 = _mm_unpacklo_epi16(T70, T80);
        T3 = _mm_unpacklo_epi16(T90, T100);
        T4 = _mm_unpackhi_epi16(T30, T40);
        T5 = _mm_unpackhi_epi16(T50, T60);
        T6 = _mm_unpackhi_epi16(T70, T80);
        T7 = _mm_unpackhi_epi16(T90, T100);

        T0 = _mm_madd_epi16(T0, mCoefy1_ver);
        T1 = _mm_madd_epi16(T1, mCoefy2_ver);
        T2 = _mm_madd_epi16(T2, mCoefy3_ver);
        T3 = _mm_madd_epi16(T3, mCoefy4_ver);
        T4 = _mm_madd_epi16(T4, mCoefy1_ver);
        T5 = _mm_madd_epi16(T5, mCoefy2_ver);
        T6 = _mm_madd_epi16(T6, mCoefy3_ver);
        T7 = _mm_madd_epi16(T7, mCoefy4_ver);

        mVal1 = _mm_add_epi32(T0, T1);
        mVal1 = _mm_add_epi32(mVal1, T2);
        mVal1 = _mm_add_epi32(mVal1, T3);

        mVal2 = _mm_add_epi32(T4, T5);
        mVal2 = _mm_add_epi32(mVal2, T6);
        mVal2 = _mm_add_epi32(mVal2, T7);

        mVal1 = _mm_add_epi32(mVal1, mAddOffset);
        mVal2 = _mm_add_epi32(mVal2, mAddOffset);
        mVal1 = _mm_srai_epi32(mVal1, shift);
        mVal2 = _mm_srai_epi32(mVal2, shift);
        mVal = _mm_packs_epi32(mVal1, mVal2);
        mVal = _mm_packus_epi16(mVal, mVal);

        _mm_storel_epi64((__m128i*)&dst[3 * i_dst], mVal);

        tmp += 4 * i_tmp;
        dst += 4 * i_dst;
        height -= 4;
    }
}

void uavs3d_if_hor_ver_luma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    int row, col;
    int shift;

    ALIGNED_16(s16 tmp_res[(128 + 7) * 128]);
    s16 *tmp = tmp_res;
    const int i_tmp = width;

    __m128i mAddOffset;

    __m128i mSwitch1 = _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8);
    __m128i mSwitch2 = _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10);
    __m128i mSwitch3 = _mm_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12);
    __m128i mSwitch4 = _mm_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14);

    __m128i mCoefy1_hor = _mm_set1_epi16(*(s16*)coef_x);
    __m128i mCoefy2_hor = _mm_set1_epi16(*(s16*)(coef_x + 2));
    __m128i mCoefy3_hor = _mm_set1_epi16(*(s16*)(coef_x + 4));
    __m128i mCoefy4_hor = _mm_set1_epi16(*(s16*)(coef_x + 6));
    __m128i mCoefy1_ver = _mm_set1_epi16(*(s16*)coef_y);
    __m128i mCoefy2_ver = _mm_set1_epi16(*(s16*)(coef_y + 2));
    __m128i mCoefy3_ver = _mm_set1_epi16(*(s16*)(coef_y + 4));
    __m128i mCoefy4_ver = _mm_set1_epi16(*(s16*)(coef_y + 6));
    __m128i mVal1, mVal2, mVal;

    // HOR
    src = src - 3 * i_src - 3;
    row = height + 7;

    while (row--) {
        __m128i mT0, mT1, mT2, mT3, s1, s2, mVal;

        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 8) {

            __m128i mSrc = _mm_loadu_si128((__m128i*)(src + col));

            mT0 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch1), mCoefy1_hor);
            mT1 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch2), mCoefy2_hor);
            mT2 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch3), mCoefy3_hor);
            mT3 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch4), mCoefy4_hor);

            s1 = _mm_add_epi16(mT0, mT1);
            s2 = _mm_add_epi16(mT2, mT3);
            mVal = _mm_add_epi16(s1, s2);

            _mm_storeu_si128((__m128i*)&tmp[col], mVal);
        }

        src += i_src;
        tmp += i_tmp;
    }

    // VER
    shift = 12;
    mAddOffset = _mm_set1_epi32(1 << (shift - 1));
    tmp = tmp_res;

    mCoefy1_ver = _mm_cvtepi8_epi16(mCoefy1_ver);
    mCoefy2_ver = _mm_cvtepi8_epi16(mCoefy2_ver);
    mCoefy3_ver = _mm_cvtepi8_epi16(mCoefy3_ver);
    mCoefy4_ver = _mm_cvtepi8_epi16(mCoefy4_ver);

    while (height--) {
        s16 const *p = tmp;
        for (col = 0; col < width; col += 8) {
            __m128i T00 = _mm_load_si128((__m128i*)(p));
            __m128i T10 = _mm_load_si128((__m128i*)(p + i_tmp));
            __m128i T20 = _mm_load_si128((__m128i*)(p + 2 * i_tmp));
            __m128i T30 = _mm_load_si128((__m128i*)(p + 3 * i_tmp));
            __m128i T40 = _mm_load_si128((__m128i*)(p + 4 * i_tmp));
            __m128i T50 = _mm_load_si128((__m128i*)(p + 5 * i_tmp));
            __m128i T60 = _mm_load_si128((__m128i*)(p + 6 * i_tmp));
            __m128i T70 = _mm_load_si128((__m128i*)(p + 7 * i_tmp));

            __m128i T0 = _mm_unpacklo_epi16(T00, T10);
            __m128i T1 = _mm_unpacklo_epi16(T20, T30);
            __m128i T2 = _mm_unpacklo_epi16(T40, T50);
            __m128i T3 = _mm_unpacklo_epi16(T60, T70);
            __m128i T4 = _mm_unpackhi_epi16(T00, T10);
            __m128i T5 = _mm_unpackhi_epi16(T20, T30);
            __m128i T6 = _mm_unpackhi_epi16(T40, T50);
            __m128i T7 = _mm_unpackhi_epi16(T60, T70);

            T0 = _mm_madd_epi16(T0, mCoefy1_ver);
            T1 = _mm_madd_epi16(T1, mCoefy2_ver);
            T2 = _mm_madd_epi16(T2, mCoefy3_ver);
            T3 = _mm_madd_epi16(T3, mCoefy4_ver);
            T4 = _mm_madd_epi16(T4, mCoefy1_ver);
            T5 = _mm_madd_epi16(T5, mCoefy2_ver);
            T6 = _mm_madd_epi16(T6, mCoefy3_ver);
            T7 = _mm_madd_epi16(T7, mCoefy4_ver);

            mVal1 = _mm_add_epi32(T0, T1);
            mVal1 = _mm_add_epi32(mVal1, T2);
            mVal1 = _mm_add_epi32(mVal1, T3);

            mVal2 = _mm_add_epi32(T4, T5);
            mVal2 = _mm_add_epi32(mVal2, T6);
            mVal2 = _mm_add_epi32(mVal2, T7);

            mVal1 = _mm_add_epi32(mVal1, mAddOffset);
            mVal2 = _mm_add_epi32(mVal2, mAddOffset);
            mVal1 = _mm_srai_epi32(mVal1, shift);
            mVal2 = _mm_srai_epi32(mVal2, shift);
            mVal = _mm_packs_epi32(mVal1, mVal2);
            mVal = _mm_packus_epi16(mVal, mVal);

            _mm_storel_epi64((__m128i*)&dst[col], mVal);

            p += 8;
        }
        tmp += i_tmp;
        dst += i_dst;
    }
}

#else

void uavs3d_if_cpy_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    while (height) {
        CP64(dst, src);
        CP64(dst + i_dst, src + i_src);
        height -= 2;
        src += i_src << 1;
        dst += i_dst << 1;
    }
}

void uavs3d_if_cpy_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    __m128i m0, m1;
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    while (height) {
        m0 = _mm_loadu_si128((const __m128i*)(src));
        m1 = _mm_loadu_si128((const __m128i*)(src + i_src));
        _mm_storeu_si128((__m128i*)(dst), m0);
        _mm_storeu_si128((__m128i*)(dst + i_dst), m1);
        src += i_src2;
        dst += i_dst2;
        height -= 2;
    }
}

void uavs3d_if_cpy_w16_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    __m128i m0, m1, m2, m3;
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    while (height) {
        m0 = _mm_loadu_si128((const __m128i*)(src));
        m1 = _mm_loadu_si128((const __m128i*)(src + 8));
        m2 = _mm_loadu_si128((const __m128i*)(src + i_src));
        m3 = _mm_loadu_si128((const __m128i*)(src + i_src + 8));
        _mm_storeu_si128((__m128i*)(dst), m0);
        _mm_storeu_si128((__m128i*)(dst + 8), m1);
        _mm_storeu_si128((__m128i*)(dst + i_dst), m2);
        _mm_storeu_si128((__m128i*)(dst + i_dst + 8), m3);
        src += i_src2;
        dst += i_dst2;
        height -= 2;
    }
}

void uavs3d_if_cpy_w16x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height)
{
    int col;
    __m128i m0, m1;
    int i_src2 = i_src << 1;
    int i_dst2 = i_dst << 1;
    while (height--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 16) {
            m0 = _mm_loadu_si128((const __m128i*)(src + col));
            m1 = _mm_loadu_si128((const __m128i*)(src + col + 8));
            _mm_storeu_si128((__m128i*)(dst + col), m0);
            _mm_storeu_si128((__m128i*)(dst + col + 8), m1);
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_luma_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i mCoef = _mm_loadl_epi64((__m128i*)coeff);

    __m128i T0, T1, T2, T3;
    __m128i M0, M1, M2, M3;
    __m128i offset = _mm_set1_epi32(32);

    mCoef = _mm_cvtepi8_epi16(mCoef);

    src -= 3;
    while (height--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        T0 = _mm_loadu_si128((__m128i*)(src + 0));
        T1 = _mm_loadu_si128((__m128i*)(src + 1));
        T2 = _mm_loadu_si128((__m128i*)(src + 2));
        T3 = _mm_loadu_si128((__m128i*)(src + 3));

        M0 = _mm_madd_epi16(T0, mCoef);
        M1 = _mm_madd_epi16(T1, mCoef);
        M2 = _mm_madd_epi16(T2, mCoef);
        M3 = _mm_madd_epi16(T3, mCoef);

        M0 = _mm_hadd_epi32(M0, M1);
        M1 = _mm_hadd_epi32(M2, M3);

        M0 = _mm_hadd_epi32(M0, M1);

        M2 = _mm_add_epi32(M0, offset);
        M2 = _mm_srai_epi32(M2, 6);
        M2 = _mm_packus_epi32(M2, M2);
        M2 = _mm_min_epu16(M2, max_pel);
        _mm_storel_epi64((__m128i*)(dst), M2);

        src += i_src;
        dst += i_dst;
    }
}
void uavs3d_if_hor_luma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    __m128i mCoef = _mm_loadl_epi64((__m128i*)coeff);
    __m128i offset = _mm_set1_epi32(32);
    __m128i T0, T1, T2, T3, T4, T5, T6, T7;
    __m128i M0, M1, M2, M3, M4, M5, M6, M7;
    __m128i max_pel = _mm_set1_epi16((pel)max_val);

    mCoef = _mm_cvtepi8_epi16(mCoef);

    src -= 3;
    while (height--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        T0 = _mm_loadu_si128((__m128i*)(src + 0));
        T1 = _mm_loadu_si128((__m128i*)(src + 1));
        T2 = _mm_loadu_si128((__m128i*)(src + 2));
        T3 = _mm_loadu_si128((__m128i*)(src + 3));
        T4 = _mm_loadu_si128((__m128i*)(src + 4));
        T5 = _mm_loadu_si128((__m128i*)(src + 5));
        T6 = _mm_loadu_si128((__m128i*)(src + 6));
        T7 = _mm_loadu_si128((__m128i*)(src + 7));

        M0 = _mm_madd_epi16(T0, mCoef);
        M1 = _mm_madd_epi16(T1, mCoef);
        M2 = _mm_madd_epi16(T2, mCoef);
        M3 = _mm_madd_epi16(T3, mCoef);
        M4 = _mm_madd_epi16(T4, mCoef);
        M5 = _mm_madd_epi16(T5, mCoef);
        M6 = _mm_madd_epi16(T6, mCoef);
        M7 = _mm_madd_epi16(T7, mCoef);

        M0 = _mm_hadd_epi32(M0, M1);
        M1 = _mm_hadd_epi32(M2, M3);
        M2 = _mm_hadd_epi32(M4, M5);
        M3 = _mm_hadd_epi32(M6, M7);

        M0 = _mm_hadd_epi32(M0, M1);
        M1 = _mm_hadd_epi32(M2, M3);

        M2 = _mm_add_epi32(M0, offset);
        M3 = _mm_add_epi32(M1, offset);
        M2 = _mm_srai_epi32(M2, 6);
        M3 = _mm_srai_epi32(M3, 6);
        M2 = _mm_packus_epi32(M2, M3);
        M2 = _mm_min_epu16(M2, max_pel);
        _mm_storeu_si128((__m128i*)(dst), M2);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_luma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int col;
    __m128i mCoef = _mm_loadl_epi64((__m128i*)coeff);
    __m128i offset = _mm_set1_epi32(32);
    __m128i T0, T1, T2, T3, T4, T5, T6, T7;
    __m128i M0, M1, M2, M3, M4, M5, M6, M7;
    __m128i max_pel = _mm_set1_epi16((pel)max_val);

    mCoef = _mm_cvtepi8_epi16(mCoef);

    src -= 3;

    while (height--) {
        const pel *p_src = src;
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 8) {
            T0 = _mm_loadu_si128((__m128i*)p_src++);
            T1 = _mm_loadu_si128((__m128i*)p_src++);
            T2 = _mm_loadu_si128((__m128i*)p_src++);
            T3 = _mm_loadu_si128((__m128i*)p_src++);
            T4 = _mm_loadu_si128((__m128i*)p_src++);
            T5 = _mm_loadu_si128((__m128i*)p_src++);
            T6 = _mm_loadu_si128((__m128i*)p_src++);
            T7 = _mm_loadu_si128((__m128i*)p_src++);

            M0 = _mm_madd_epi16(T0, mCoef);
            M1 = _mm_madd_epi16(T1, mCoef);
            M2 = _mm_madd_epi16(T2, mCoef);
            M3 = _mm_madd_epi16(T3, mCoef);
            M4 = _mm_madd_epi16(T4, mCoef);
            M5 = _mm_madd_epi16(T5, mCoef);
            M6 = _mm_madd_epi16(T6, mCoef);
            M7 = _mm_madd_epi16(T7, mCoef);

            M0 = _mm_hadd_epi32(M0, M1);
            M1 = _mm_hadd_epi32(M2, M3);
            M2 = _mm_hadd_epi32(M4, M5);
            M3 = _mm_hadd_epi32(M6, M7);

            M0 = _mm_hadd_epi32(M0, M1);
            M1 = _mm_hadd_epi32(M2, M3);

            M2 = _mm_add_epi32(M0, offset);
            M3 = _mm_add_epi32(M1, offset);
            M2 = _mm_srai_epi32(M2, 6);
            M3 = _mm_srai_epi32(M3, 6);
            M2 = _mm_packus_epi32(M2, M3);
            M2 = _mm_min_epu16(M2, max_pel);
            _mm_storeu_si128((__m128i*)(dst + col), M2);
        }

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_chroma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;

    __m128i mAddOffset = _mm_set1_epi32(offset);
    __m128i mSwitch = _mm_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11);
    __m128i mCoef0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i mCoef1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i mSrc0, mSrc1, mSrc2;

    mCoef0 = _mm_cvtepi8_epi16(mCoef0);
    mCoef1 = _mm_cvtepi8_epi16(mCoef1);
    src -= 2;

    while (height--) {
        __m128i mT20, mT40, mVal, mT10, mT30;
        __m128i t0, t1, t2;
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        mSrc0 = _mm_loadu_si128((__m128i*)(src));
        mSrc1 = _mm_loadu_si128((__m128i*)(src + 4));
        mSrc2 = _mm_loadu_si128((__m128i*)(src + 8));

        t0 = _mm_shuffle_epi8(mSrc0, mSwitch);
        t1 = _mm_shuffle_epi8(mSrc1, mSwitch);
        t2 = _mm_shuffle_epi8(mSrc2, mSwitch);

        mT20 = _mm_madd_epi16(t0, mCoef0);
        mT40 = _mm_madd_epi16(t1, mCoef1);

        mT10 = _mm_madd_epi16(t1, mCoef0);
        mT30 = _mm_madd_epi16(t2, mCoef1);

        mT20 = _mm_add_epi32(mT20, mT40);
        mT10 = _mm_add_epi32(mT10, mT30);

        mT20 = _mm_add_epi32(mT20, mAddOffset);
        mT10 = _mm_add_epi32(mT10, mAddOffset);
        mT20 = _mm_srai_epi32(mT20, shift);
        mT10 = _mm_srai_epi32(mT10, shift);

        mVal = _mm_packus_epi32(mT20, mT10);
        mVal = _mm_min_epu16(mVal, max_pel);
        _mm_storeu_si128((__m128i*)dst, mVal);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_chroma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int col;
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi32(offset);
    __m128i mSwitch = _mm_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11);
    __m128i mCoef0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i mCoef1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i mSrc0, mSrc1, mSrc2;

    mCoef0 = _mm_cvtepi8_epi16(mCoef0);
    mCoef1 = _mm_cvtepi8_epi16(mCoef1);

    src -= 2;

    while (height--) {
        __m128i mT20, mT40, mVal, mT10, mT30;
        __m128i t0, t1, t2;
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);

        for (col = 0; col < width; col += 8) {
            mSrc0 = _mm_loadu_si128((__m128i*)(src + col));
            mSrc1 = _mm_loadu_si128((__m128i*)(src + col + 4));
            mSrc2 = _mm_loadu_si128((__m128i*)(src + col + 8));

            t0 = _mm_shuffle_epi8(mSrc0, mSwitch);
            t1 = _mm_shuffle_epi8(mSrc1, mSwitch);
            t2 = _mm_shuffle_epi8(mSrc2, mSwitch);

            mT20 = _mm_madd_epi16(t0, mCoef0);
            mT40 = _mm_madd_epi16(t1, mCoef1);

            mT10 = _mm_madd_epi16(t1, mCoef0);
            mT30 = _mm_madd_epi16(t2, mCoef1);

            mT20 = _mm_add_epi32(mT20, mT40);
            mT10 = _mm_add_epi32(mT10, mT30);

            mT20 = _mm_add_epi32(mT20, mAddOffset);
            mT10 = _mm_add_epi32(mT10, mAddOffset);
            mT20 = _mm_srai_epi32(mT20, shift);
            mT10 = _mm_srai_epi32(mT10, shift);

            mVal = _mm_packus_epi32(mT20, mT10);
            mVal = _mm_min_epu16(mVal, max_pel);
            _mm_storeu_si128((__m128i*)&dst[col], mVal);
        }

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_luma_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i mAddOffset = _mm_set1_epi32(32);
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));
    __m128i T0, T1, T2, T3, T4, T5, T6, T7;
    __m128i M0, M1, M2, M3;
    __m128i N0, N1, N2, N3;

    coeff0 = _mm_cvtepi8_epi16(coeff0);
    coeff1 = _mm_cvtepi8_epi16(coeff1);
    coeff2 = _mm_cvtepi8_epi16(coeff2);
    coeff3 = _mm_cvtepi8_epi16(coeff3);

    src -= 3 * i_src;

    while (height--) {
        uavs3d_prefetch(src + 9 * i_src, _MM_HINT_NTA);
        T0 = _mm_loadl_epi64((__m128i*)(src));
        T1 = _mm_loadl_epi64((__m128i*)(src + i_src));
        T2 = _mm_loadl_epi64((__m128i*)(src + 2 * i_src));
        T3 = _mm_loadl_epi64((__m128i*)(src + 3 * i_src));
        T4 = _mm_loadl_epi64((__m128i*)(src + 4 * i_src));
        T5 = _mm_loadl_epi64((__m128i*)(src + 5 * i_src));
        T6 = _mm_loadl_epi64((__m128i*)(src + 6 * i_src));
        T7 = _mm_loadl_epi64((__m128i*)(src + 7 * i_src));

        M0 = _mm_unpacklo_epi16(T0, T1);
        M1 = _mm_unpacklo_epi16(T2, T3);
        M2 = _mm_unpacklo_epi16(T4, T5);
        M3 = _mm_unpacklo_epi16(T6, T7);

        N0 = _mm_madd_epi16(M0, coeff0);
        N1 = _mm_madd_epi16(M1, coeff1);
        N2 = _mm_madd_epi16(M2, coeff2);
        N3 = _mm_madd_epi16(M3, coeff3);

        N0 = _mm_add_epi32(N0, N1);
        N1 = _mm_add_epi32(N2, N3);

        N0 = _mm_add_epi32(N0, N1);

        N0 = _mm_add_epi32(N0, mAddOffset);
        N0 = _mm_srai_epi32(N0, 6);
        N0 = _mm_packus_epi32(N0, N0);
        N0 = _mm_min_epu16(N0, max_pel);
        _mm_storel_epi64((__m128i*)(dst), N0);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_luma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i mAddOffset = _mm_set1_epi32(32);
    __m128i T0, T1, T2, T3, T4, T5, T6, T7;
    __m128i M0, M1, M2, M3, M4, M5, M6, M7;
    __m128i N0, N1, N2, N3, N4, N5, N6, N7;
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));
    coeff0 = _mm_cvtepi8_epi16(coeff0);
    coeff1 = _mm_cvtepi8_epi16(coeff1);
    coeff2 = _mm_cvtepi8_epi16(coeff2);
    coeff3 = _mm_cvtepi8_epi16(coeff3);

    src -= 3 * i_src;

    while (height--) {
        T0 = _mm_loadu_si128((__m128i*)(src));
        T1 = _mm_loadu_si128((__m128i*)(src + i_src));
        T2 = _mm_loadu_si128((__m128i*)(src + 2 * i_src));
        T3 = _mm_loadu_si128((__m128i*)(src + 3 * i_src));
        T4 = _mm_loadu_si128((__m128i*)(src + 4 * i_src));
        T5 = _mm_loadu_si128((__m128i*)(src + 5 * i_src));
        T6 = _mm_loadu_si128((__m128i*)(src + 6 * i_src));
        T7 = _mm_loadu_si128((__m128i*)(src + 7 * i_src));

        M0 = _mm_unpacklo_epi16(T0, T1);
        M1 = _mm_unpacklo_epi16(T2, T3);
        M2 = _mm_unpacklo_epi16(T4, T5);
        M3 = _mm_unpacklo_epi16(T6, T7);
        M4 = _mm_unpackhi_epi16(T0, T1);
        M5 = _mm_unpackhi_epi16(T2, T3);
        M6 = _mm_unpackhi_epi16(T4, T5);
        M7 = _mm_unpackhi_epi16(T6, T7);

        N0 = _mm_madd_epi16(M0, coeff0);
        N1 = _mm_madd_epi16(M1, coeff1);
        N2 = _mm_madd_epi16(M2, coeff2);
        N3 = _mm_madd_epi16(M3, coeff3);
        N4 = _mm_madd_epi16(M4, coeff0);
        N5 = _mm_madd_epi16(M5, coeff1);
        N6 = _mm_madd_epi16(M6, coeff2);
        N7 = _mm_madd_epi16(M7, coeff3);

        N0 = _mm_add_epi32(N0, N1);
        N1 = _mm_add_epi32(N2, N3);
        N2 = _mm_add_epi32(N4, N5);
        N3 = _mm_add_epi32(N6, N7);

        N0 = _mm_add_epi32(N0, N1);
        N1 = _mm_add_epi32(N2, N3);

        N0 = _mm_add_epi32(N0, mAddOffset);
        N1 = _mm_add_epi32(N1, mAddOffset);
        N0 = _mm_srai_epi32(N0, 6);
        N1 = _mm_srai_epi32(N1, 6);
        N0 = _mm_packus_epi32(N0, N1);
        N0 = _mm_min_epu16(N0, max_pel);
        _mm_storeu_si128((__m128i*)(dst), N0);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_luma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int i;
    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i mAddOffset = _mm_set1_epi32(32);
    __m128i T0, T1, T2, T3, T4, T5, T6, T7;
    __m128i M0, M1, M2, M3, M4, M5, M6, M7;
    __m128i N0, N1, N2, N3, N4, N5, N6, N7;
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i coeff2 = _mm_set1_epi16(*(s16*)(coeff + 4));
    __m128i coeff3 = _mm_set1_epi16(*(s16*)(coeff + 6));

    coeff0 = _mm_cvtepi8_epi16(coeff0);
    coeff1 = _mm_cvtepi8_epi16(coeff1);
    coeff2 = _mm_cvtepi8_epi16(coeff2);
    coeff3 = _mm_cvtepi8_epi16(coeff3);

    src -= 3 * i_src;

    while (height--) {
        const pel *p_src = src;
        uavs3d_prefetch(src + 8 * i_src, _MM_HINT_NTA);
        for (i = 0; i < width; i += 8) {
            T0 = _mm_loadu_si128((__m128i*)(p_src));
            T1 = _mm_loadu_si128((__m128i*)(p_src + i_src));
            T2 = _mm_loadu_si128((__m128i*)(p_src + 2 * i_src));
            T3 = _mm_loadu_si128((__m128i*)(p_src + 3 * i_src));
            T4 = _mm_loadu_si128((__m128i*)(p_src + 4 * i_src));
            T5 = _mm_loadu_si128((__m128i*)(p_src + 5 * i_src));
            T6 = _mm_loadu_si128((__m128i*)(p_src + 6 * i_src));
            T7 = _mm_loadu_si128((__m128i*)(p_src + 7 * i_src));

            M0 = _mm_unpacklo_epi16(T0, T1);
            M1 = _mm_unpacklo_epi16(T2, T3);
            M2 = _mm_unpacklo_epi16(T4, T5);
            M3 = _mm_unpacklo_epi16(T6, T7);
            M4 = _mm_unpackhi_epi16(T0, T1);
            M5 = _mm_unpackhi_epi16(T2, T3);
            M6 = _mm_unpackhi_epi16(T4, T5);
            M7 = _mm_unpackhi_epi16(T6, T7);

            N0 = _mm_madd_epi16(M0, coeff0);
            N1 = _mm_madd_epi16(M1, coeff1);
            N2 = _mm_madd_epi16(M2, coeff2);
            N3 = _mm_madd_epi16(M3, coeff3);
            N4 = _mm_madd_epi16(M4, coeff0);
            N5 = _mm_madd_epi16(M5, coeff1);
            N6 = _mm_madd_epi16(M6, coeff2);
            N7 = _mm_madd_epi16(M7, coeff3);

            N0 = _mm_add_epi32(N0, N1);
            N1 = _mm_add_epi32(N2, N3);
            N2 = _mm_add_epi32(N4, N5);
            N3 = _mm_add_epi32(N6, N7);

            N0 = _mm_add_epi32(N0, N1);
            N1 = _mm_add_epi32(N2, N3);

            N0 = _mm_add_epi32(N0, mAddOffset);
            N1 = _mm_add_epi32(N1, mAddOffset);
            N0 = _mm_srai_epi32(N0, 6);
            N1 = _mm_srai_epi32(N1, 6);
            N0 = _mm_packus_epi32(N0, N1);
            N0 = _mm_min_epu16(N0, max_pel);
            _mm_storeu_si128((__m128i*)(dst + i), N0);

            p_src += 8;
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_chroma_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi32(offset);
    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i mVal;
    __m128i M0, M1;

    coeff0 = _mm_cvtepi8_epi16(coeff0);
    coeff1 = _mm_cvtepi8_epi16(coeff1);
    src -= i_src;

    while (height--) {
        __m128i T00 = _mm_loadl_epi64((__m128i*)(src));
        __m128i T10 = _mm_loadl_epi64((__m128i*)(src + i_src));
        __m128i T20 = _mm_loadl_epi64((__m128i*)(src + 2 * i_src));
        __m128i T30 = _mm_loadl_epi64((__m128i*)(src + 3 * i_src));

        M0 = _mm_unpacklo_epi16(T00, T10);
        M1 = _mm_unpacklo_epi16(T20, T30);

        M0 = _mm_madd_epi16(M0, coeff0);
        M1 = _mm_madd_epi16(M1, coeff1);

        M0 = _mm_add_epi32(M0, M1);

        M0 = _mm_add_epi32(M0, mAddOffset);
        M0 = _mm_srai_epi32(M0, shift);

        mVal = _mm_packus_epi32(M0, M0);
        mVal = _mm_min_epu16(mVal, max_pel);
        _mm_storel_epi64((__m128i*)dst, mVal);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_chroma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi32(offset);
    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i M0, M1, M2, M3;
    __m128i mVal;

    coeff0 = _mm_cvtepi8_epi16(coeff0);
    coeff1 = _mm_cvtepi8_epi16(coeff1);
    src -= i_src;

    while (height--) {
        __m128i T00 = _mm_loadu_si128((__m128i*)(src));
        __m128i T10 = _mm_loadu_si128((__m128i*)(src + i_src));
        __m128i T20 = _mm_loadu_si128((__m128i*)(src + 2 * i_src));
        __m128i T30 = _mm_loadu_si128((__m128i*)(src + 3 * i_src));
        uavs3d_prefetch(src + 4 * i_src, _MM_HINT_NTA);

        M0 = _mm_unpacklo_epi16(T00, T10);
        M1 = _mm_unpacklo_epi16(T20, T30);
        M2 = _mm_unpackhi_epi16(T00, T10);
        M3 = _mm_unpackhi_epi16(T20, T30);

        M0 = _mm_madd_epi16(M0, coeff0);
        M1 = _mm_madd_epi16(M1, coeff1);
        M2 = _mm_madd_epi16(M2, coeff0);
        M3 = _mm_madd_epi16(M3, coeff1);

        M0 = _mm_add_epi32(M0, M1);
        M2 = _mm_add_epi32(M2, M3);

        M0 = _mm_add_epi32(M0, mAddOffset);
        M2 = _mm_add_epi32(M2, mAddOffset);
        M0 = _mm_srai_epi32(M0, shift);
        M2 = _mm_srai_epi32(M2, shift);

        mVal = _mm_packus_epi32(M0, M2);
        mVal = _mm_min_epu16(mVal, max_pel);
        _mm_storeu_si128((__m128i*)dst, mVal);

        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_ver_chroma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff, int max_val)
{
    int col;
    const int offset = 32;
    const int shift = 6;
    __m128i mAddOffset = _mm_set1_epi32(offset);
    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i coeff0 = _mm_set1_epi16(*(s16*)coeff);
    __m128i coeff1 = _mm_set1_epi16(*(s16*)(coeff + 2));
    __m128i M0, M1, M2, M3;
    __m128i mVal;

    coeff0 = _mm_cvtepi8_epi16(coeff0);
    coeff1 = _mm_cvtepi8_epi16(coeff1);
    src -= i_src;

    while (height--) {
        const pel *p_src = src;
        for (col = 0; col < width; col += 8) {
            __m128i T00 = _mm_loadu_si128((__m128i*)(p_src));
            __m128i T10 = _mm_loadu_si128((__m128i*)(p_src + i_src));
            __m128i T20 = _mm_loadu_si128((__m128i*)(p_src + 2 * i_src));
            __m128i T30 = _mm_loadu_si128((__m128i*)(p_src + 3 * i_src));
            uavs3d_prefetch(src + 4 * i_src, _MM_HINT_NTA);

            M0 = _mm_unpacklo_epi16(T00, T10);
            M1 = _mm_unpacklo_epi16(T20, T30);
            M2 = _mm_unpackhi_epi16(T00, T10);
            M3 = _mm_unpackhi_epi16(T20, T30);

            M0 = _mm_madd_epi16(M0, coeff0);
            M1 = _mm_madd_epi16(M1, coeff1);
            M2 = _mm_madd_epi16(M2, coeff0);
            M3 = _mm_madd_epi16(M3, coeff1);

            M0 = _mm_add_epi32(M0, M1);
            M2 = _mm_add_epi32(M2, M3);

            M0 = _mm_add_epi32(M0, mAddOffset);
            M2 = _mm_add_epi32(M2, mAddOffset);
            M0 = _mm_srai_epi32(M0, shift);
            M2 = _mm_srai_epi32(M2, shift);

            mVal = _mm_packus_epi32(M0, M2);
            p_src += 8;
            mVal = _mm_min_epu16(mVal, max_pel);
            _mm_storeu_si128((__m128i*)(dst + col), mVal);
        }
        src += i_src;
        dst += i_dst;
    }
}

void uavs3d_if_hor_ver_luma_w4_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    int rows;
    int shift1, shift2;
    int add1, add2;

    ALIGNED_16(s16 tmp_res[(32 + 7) * 4 + 8]);
    s16 *tmp = tmp_res;
    const int i_tmp = 4;

    __m128i mAddOffset1, mAddOffset2;
    __m128i coeff00, coeff01, coeff02, coeff03;
    __m128i T0, T1, T2, T3, T4, T5, T6, T7;
    __m128i M0, M1, M2, M3;
    __m128i N0, N1, N2, N3;
    __m128i mCoef, offset, max_pel;

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
    tmp = tmp_res;

    mCoef = _mm_loadl_epi64((__m128i*)coef_x);
    offset = _mm_set1_epi32(add1);
    max_pel = _mm_set1_epi16((pel)max_val);
    mCoef = _mm_cvtepi8_epi16(mCoef);

    add1 = (1 << (shift1)) >> 1;
    add2 = 1 << (shift2 - 1);

    mAddOffset1 = _mm_set1_epi32(add1);
    mAddOffset2 = _mm_set1_epi32(add2);

    // HOR
    rows = height + 7;

    while (rows--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        T0 = _mm_loadu_si128((__m128i*)(src + 0));
        T1 = _mm_loadu_si128((__m128i*)(src + 1));
        T2 = _mm_loadu_si128((__m128i*)(src + 2));
        T3 = _mm_loadu_si128((__m128i*)(src + 3));

        M0 = _mm_madd_epi16(T0, mCoef);
        M1 = _mm_madd_epi16(T1, mCoef);
        M2 = _mm_madd_epi16(T2, mCoef);
        M3 = _mm_madd_epi16(T3, mCoef);

        M0 = _mm_hadd_epi32(M0, M1);
        M1 = _mm_hadd_epi32(M2, M3);

        M0 = _mm_hadd_epi32(M0, M1);

        M0 = _mm_add_epi32(M0, offset);
        M0 = _mm_srai_epi32(M0, shift1);
        M0 = _mm_packs_epi32(M0, M0);
        _mm_storeu_si128((__m128i*)tmp, M0);

        tmp += i_tmp;
        src += i_src;
    }

    offset = _mm_set1_epi32(add2);
    tmp = tmp_res;

    coeff00 = _mm_set1_epi16(*(s16*)coef_y);
    coeff01 = _mm_set1_epi16(*(s16*)(coef_y + 2));
    coeff02 = _mm_set1_epi16(*(s16*)(coef_y + 4));
    coeff03 = _mm_set1_epi16(*(s16*)(coef_y + 6));
    coeff00 = _mm_cvtepi8_epi16(coeff00);
    coeff01 = _mm_cvtepi8_epi16(coeff01);
    coeff02 = _mm_cvtepi8_epi16(coeff02);
    coeff03 = _mm_cvtepi8_epi16(coeff03);

    while (height--) {
        T0 = _mm_loadl_epi64((__m128i*)(tmp));
        T1 = _mm_loadl_epi64((__m128i*)(tmp + i_tmp));
        T2 = _mm_loadl_epi64((__m128i*)(tmp + 2 * i_tmp));
        T3 = _mm_loadl_epi64((__m128i*)(tmp + 3 * i_tmp));
        T4 = _mm_loadl_epi64((__m128i*)(tmp + 4 * i_tmp));
        T5 = _mm_loadl_epi64((__m128i*)(tmp + 5 * i_tmp));
        T6 = _mm_loadl_epi64((__m128i*)(tmp + 6 * i_tmp));
        T7 = _mm_loadl_epi64((__m128i*)(tmp + 7 * i_tmp));

        M0 = _mm_unpacklo_epi16(T0, T1);
        M1 = _mm_unpacklo_epi16(T2, T3);
        M2 = _mm_unpacklo_epi16(T4, T5);
        M3 = _mm_unpacklo_epi16(T6, T7);

        N0 = _mm_madd_epi16(M0, coeff00);
        N1 = _mm_madd_epi16(M1, coeff01);
        N2 = _mm_madd_epi16(M2, coeff02);
        N3 = _mm_madd_epi16(M3, coeff03);

        N0 = _mm_add_epi32(N0, N1);
        N1 = _mm_add_epi32(N2, N3);

        N0 = _mm_add_epi32(N0, N1);

        N0 = _mm_add_epi32(N0, offset);
        N0 = _mm_srai_epi32(N0, shift2);
        N0 = _mm_packus_epi32(N0, N0);
        N0 = _mm_min_epu16(N0, max_pel);
        _mm_storel_epi64((__m128i*)(dst), N0);

        dst += i_dst;
        tmp += i_tmp;
    }
}

void uavs3d_if_hor_ver_luma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    int rows;
    int add1, shift1;
    int add2, shift2;
    __m128i T0, T1, T2, T3, T4, T5;
    __m128i M0, M1, M2, M3, M4, M5, M6, M7;
    __m128i offset, max_pel;
    ALIGNED_16(s16 tmp_res[(64 + 7) * 8]);
    s16 *tmp = tmp_res;
    const int i_tmp = 8;
    __m128i mCoef0, mCoef1, mCoef2, mCoef3;

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
        __m128i mShuffle0 = _mm_setr_epi8(0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9);
        __m128i mShuffle1 = _mm_setr_epi8(4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13);
        __m128i S0, S1, S2;

        mCoef0 = _mm_set1_epi16(((s16*)coef_x)[0]);
        mCoef1 = _mm_set1_epi16(((s16*)coef_x)[1]);
        mCoef2 = _mm_set1_epi16(((s16*)coef_x)[2]);
        mCoef3 = _mm_set1_epi16(((s16*)coef_x)[3]);
        mCoef0 = _mm_cvtepi8_epi16(mCoef0);
        mCoef1 = _mm_cvtepi8_epi16(mCoef1);
        mCoef2 = _mm_cvtepi8_epi16(mCoef2);
        mCoef3 = _mm_cvtepi8_epi16(mCoef3);
        offset = _mm_set1_epi32(add1);

        // HOR
        rows = height + 7;
        while (rows--) {
            S0 = _mm_loadu_si128((__m128i*)(src));
            S1 = _mm_loadu_si128((__m128i*)(src + 4));
            S2 = _mm_loadu_si128((__m128i*)(src + 8));
            src += i_src;
            uavs3d_prefetch(src, _MM_HINT_NTA);

            T0 = _mm_shuffle_epi8(S0, mShuffle0);
            T1 = _mm_shuffle_epi8(S0, mShuffle1);
            T2 = _mm_shuffle_epi8(S1, mShuffle0);
            T3 = _mm_shuffle_epi8(S1, mShuffle1);
            T4 = _mm_shuffle_epi8(S2, mShuffle0);
            T5 = _mm_shuffle_epi8(S2, mShuffle1);

            M0 = _mm_madd_epi16(T0, mCoef0);
            M1 = _mm_madd_epi16(T1, mCoef1);
            M2 = _mm_madd_epi16(T2, mCoef2);
            M3 = _mm_madd_epi16(T3, mCoef3);
            M4 = _mm_madd_epi16(T2, mCoef0);
            M5 = _mm_madd_epi16(T3, mCoef1);
            M6 = _mm_madd_epi16(T4, mCoef2);
            M7 = _mm_madd_epi16(T5, mCoef3);

            M0 = _mm_add_epi32(M0, M1);
            M1 = _mm_add_epi32(M2, M3);
            M2 = _mm_add_epi32(M4, M5);
            M3 = _mm_add_epi32(M6, M7);

            M0 = _mm_add_epi32(M0, M1);
            M1 = _mm_add_epi32(M2, M3);

            M2 = _mm_add_epi32(M0, offset);
            M3 = _mm_add_epi32(M1, offset);
            M2 = _mm_srai_epi32(M2, shift1);
            M3 = _mm_srai_epi32(M3, shift1);
            M2 = _mm_packs_epi32(M2, M3);
            _mm_store_si128((__m128i*)tmp, M2);

            tmp += i_tmp;
        }
    }

    {
        __m128i N0, N1, N2, N3, N4, N5, N6, N7;
        __m128i T6, T7;

        offset = _mm_set1_epi32(add2);
        max_pel = _mm_set1_epi16((pel)max_val);
        tmp = tmp_res;

        mCoef0 = _mm_set1_epi16(((s16*)coef_y)[0]);
        mCoef1 = _mm_set1_epi16(((s16*)coef_y)[1]);
        mCoef2 = _mm_set1_epi16(((s16*)coef_y)[2]);
        mCoef3 = _mm_set1_epi16(((s16*)coef_y)[3]);
        mCoef0 = _mm_cvtepi8_epi16(mCoef0);
        mCoef1 = _mm_cvtepi8_epi16(mCoef1);
        mCoef2 = _mm_cvtepi8_epi16(mCoef2);
        mCoef3 = _mm_cvtepi8_epi16(mCoef3);

        while (height--) {
            T0 = _mm_load_si128((__m128i*)(tmp));
            T1 = _mm_load_si128((__m128i*)(tmp + i_tmp));
            T2 = _mm_load_si128((__m128i*)(tmp + 2 * i_tmp));
            T3 = _mm_load_si128((__m128i*)(tmp + 3 * i_tmp));
            T4 = _mm_load_si128((__m128i*)(tmp + 4 * i_tmp));
            T5 = _mm_load_si128((__m128i*)(tmp + 5 * i_tmp));
            T6 = _mm_load_si128((__m128i*)(tmp + 6 * i_tmp));
            T7 = _mm_load_si128((__m128i*)(tmp + 7 * i_tmp));

            M0 = _mm_unpacklo_epi16(T0, T1);
            M1 = _mm_unpacklo_epi16(T2, T3);
            M2 = _mm_unpacklo_epi16(T4, T5);
            M3 = _mm_unpacklo_epi16(T6, T7);
            M4 = _mm_unpackhi_epi16(T0, T1);
            M5 = _mm_unpackhi_epi16(T2, T3);
            M6 = _mm_unpackhi_epi16(T4, T5);
            M7 = _mm_unpackhi_epi16(T6, T7);

            N0 = _mm_madd_epi16(M0, mCoef0);
            N1 = _mm_madd_epi16(M1, mCoef1);
            N2 = _mm_madd_epi16(M2, mCoef2);
            N3 = _mm_madd_epi16(M3, mCoef3);
            N4 = _mm_madd_epi16(M4, mCoef0);
            N5 = _mm_madd_epi16(M5, mCoef1);
            N6 = _mm_madd_epi16(M6, mCoef2);
            N7 = _mm_madd_epi16(M7, mCoef3);

            N0 = _mm_add_epi32(N0, N1);
            N1 = _mm_add_epi32(N2, N3);
            N2 = _mm_add_epi32(N4, N5);
            N3 = _mm_add_epi32(N6, N7);

            N0 = _mm_add_epi32(N0, N1);
            N1 = _mm_add_epi32(N2, N3);

            N0 = _mm_add_epi32(N0, offset);
            N1 = _mm_add_epi32(N1, offset);
            N0 = _mm_srai_epi32(N0, shift2);
            N1 = _mm_srai_epi32(N1, shift2);
            N0 = _mm_packus_epi32(N0, N1);
            N0 = _mm_min_epu16(N0, max_pel);
            _mm_storeu_si128((__m128i*)(dst), N0);

            dst += i_dst;
            tmp += i_tmp;
        }
    }
}

void uavs3d_if_hor_ver_luma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coef_x, const s8 *coef_y, int max_val)
{
    int row, col;
    int add1, shift1;
    int add2, shift2;
    __m128i T0, T1, T2, T3, T4, T5, T6, T7;
    __m128i M0, M1, M2, M3, M4, M5, M6, M7;
    __m128i N0, N1, N2, N3, N4, N5, N6, N7;
    __m128i mCoef, offset, max_pel;
    ALIGNED_16(s16 tmp_res[(128 + 7) * 128]);
    s16 *tmp = tmp_res;
    const int i_tmp = width;
    __m128i coeff00, coeff01, coeff02, coeff03;

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

    mCoef = _mm_loadl_epi64((__m128i*)coef_x);
    offset = _mm_set1_epi32(add1);
    mCoef = _mm_cvtepi8_epi16(mCoef);

    // HOR
    row = height + 7;
    while (row--) {
        for (col = 0; col < width; col += 8) {
            uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
            T0 = _mm_loadu_si128((__m128i*)(src + col + 0));
            T1 = _mm_loadu_si128((__m128i*)(src + col + 1));
            T2 = _mm_loadu_si128((__m128i*)(src + col + 2));
            T3 = _mm_loadu_si128((__m128i*)(src + col + 3));
            T4 = _mm_loadu_si128((__m128i*)(src + col + 4));
            T5 = _mm_loadu_si128((__m128i*)(src + col + 5));
            T6 = _mm_loadu_si128((__m128i*)(src + col + 6));
            T7 = _mm_loadu_si128((__m128i*)(src + col + 7));

            M0 = _mm_madd_epi16(T0, mCoef);
            M1 = _mm_madd_epi16(T1, mCoef);
            M2 = _mm_madd_epi16(T2, mCoef);
            M3 = _mm_madd_epi16(T3, mCoef);
            M4 = _mm_madd_epi16(T4, mCoef);
            M5 = _mm_madd_epi16(T5, mCoef);
            M6 = _mm_madd_epi16(T6, mCoef);
            M7 = _mm_madd_epi16(T7, mCoef);

            M0 = _mm_hadd_epi32(M0, M1);
            M1 = _mm_hadd_epi32(M2, M3);
            M2 = _mm_hadd_epi32(M4, M5);
            M3 = _mm_hadd_epi32(M6, M7);

            M0 = _mm_hadd_epi32(M0, M1);
            M1 = _mm_hadd_epi32(M2, M3);

            M2 = _mm_add_epi32(M0, offset);
            M3 = _mm_add_epi32(M1, offset);
            M2 = _mm_srai_epi32(M2, shift1);
            M3 = _mm_srai_epi32(M3, shift1);
            M2 = _mm_packs_epi32(M2, M3);
            _mm_storeu_si128((__m128i*)(tmp + col), M2);
        }
        tmp += i_tmp;
        src += i_src;
    }

    offset = _mm_set1_epi32(add2);
    max_pel = _mm_set1_epi16((pel)max_val);
    tmp = tmp_res;

    coeff00 = _mm_set1_epi16(*(s16*)coef_y);
    coeff01 = _mm_set1_epi16(*(s16*)(coef_y + 2));
    coeff02 = _mm_set1_epi16(*(s16*)(coef_y + 4));
    coeff03 = _mm_set1_epi16(*(s16*)(coef_y + 6));
    coeff00 = _mm_cvtepi8_epi16(coeff00);
    coeff01 = _mm_cvtepi8_epi16(coeff01);
    coeff02 = _mm_cvtepi8_epi16(coeff02);
    coeff03 = _mm_cvtepi8_epi16(coeff03);

    while (height--) {
        for (col = 0; col < width; col += 8) {
            T0 = _mm_load_si128((__m128i*)(tmp + col));
            T1 = _mm_load_si128((__m128i*)(tmp + col + i_tmp));
            T2 = _mm_load_si128((__m128i*)(tmp + col + 2 * i_tmp));
            T3 = _mm_load_si128((__m128i*)(tmp + col + 3 * i_tmp));
            T4 = _mm_load_si128((__m128i*)(tmp + col + 4 * i_tmp));
            T5 = _mm_load_si128((__m128i*)(tmp + col + 5 * i_tmp));
            T6 = _mm_load_si128((__m128i*)(tmp + col + 6 * i_tmp));
            T7 = _mm_load_si128((__m128i*)(tmp + col + 7 * i_tmp));

            M0 = _mm_unpacklo_epi16(T0, T1);
            M1 = _mm_unpacklo_epi16(T2, T3);
            M2 = _mm_unpacklo_epi16(T4, T5);
            M3 = _mm_unpacklo_epi16(T6, T7);
            M4 = _mm_unpackhi_epi16(T0, T1);
            M5 = _mm_unpackhi_epi16(T2, T3);
            M6 = _mm_unpackhi_epi16(T4, T5);
            M7 = _mm_unpackhi_epi16(T6, T7);

            N0 = _mm_madd_epi16(M0, coeff00);
            N1 = _mm_madd_epi16(M1, coeff01);
            N2 = _mm_madd_epi16(M2, coeff02);
            N3 = _mm_madd_epi16(M3, coeff03);
            N4 = _mm_madd_epi16(M4, coeff00);
            N5 = _mm_madd_epi16(M5, coeff01);
            N6 = _mm_madd_epi16(M6, coeff02);
            N7 = _mm_madd_epi16(M7, coeff03);

            N0 = _mm_add_epi32(N0, N1);
            N1 = _mm_add_epi32(N2, N3);
            N2 = _mm_add_epi32(N4, N5);
            N3 = _mm_add_epi32(N6, N7);

            N0 = _mm_add_epi32(N0, N1);
            N1 = _mm_add_epi32(N2, N3);

            N0 = _mm_add_epi32(N0, offset);
            N1 = _mm_add_epi32(N1, offset);
            N0 = _mm_srai_epi32(N0, shift2);
            N1 = _mm_srai_epi32(N1, shift2);
            N0 = _mm_packus_epi32(N0, N1);
            N0 = _mm_min_epu16(N0, max_pel);
            _mm_storeu_si128((__m128i*)(dst + col), N0);
        }
        dst += i_dst;
        tmp += i_tmp;
    }
}

void uavs3d_if_hor_ver_chroma_w8_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff_h, const s8 *coeff_v, int max_val)
{
    int row;
    int shift1, shift2;
    int add1, add2;

    ALIGNED_16(s16 tmp_res[(64 + 3) * 8]);
    s16 *tmp = tmp_res;
    const int i_tmp = 8;

    __m128i mAddOffset1, mAddOffset2;

    __m128i mSwitch = _mm_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11);
    __m128i mCoef0 = _mm_set1_epi16(*(s16*)coeff_h);
    __m128i mCoef1 = _mm_set1_epi16(*(s16*)(coeff_h + 2));

    __m128i max_pel = _mm_set1_epi16((pel)max_val);
    __m128i mVal;
    __m128i M0, M1, M2, M3;
    __m128i mSrc0, mSrc1, mSrc2;
    __m128i mT0, mT1, mT2, mT3;

    mCoef0 = _mm_cvtepi8_epi16(mCoef0);
    mCoef1 = _mm_cvtepi8_epi16(mCoef1);

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

    mAddOffset1 = _mm_set1_epi32(add1);
    mAddOffset2 = _mm_set1_epi32(add2);

    // HOR
    src = src - 1 * i_src - 2;
    row = height + 3;

    while (row--) {
        mSrc0 = _mm_loadu_si128((__m128i*)(src));
        mSrc1 = _mm_loadu_si128((__m128i*)(src + 4));
        mSrc2 = _mm_loadu_si128((__m128i*)(src + 8));

        M0 = _mm_shuffle_epi8(mSrc0, mSwitch);
        M1 = _mm_shuffle_epi8(mSrc1, mSwitch);
        M2 = _mm_shuffle_epi8(mSrc2, mSwitch);

        mT0 = _mm_madd_epi16(M0, mCoef0);
        mT1 = _mm_madd_epi16(M1, mCoef1);

        mT2 = _mm_madd_epi16(M1, mCoef0);
        mT3 = _mm_madd_epi16(M2, mCoef1);

        mT0 = _mm_add_epi32(mT0, mT1);
        mT2 = _mm_add_epi32(mT2, mT3);

        mT0 = _mm_add_epi32(mT0, mAddOffset1);
        mT2 = _mm_add_epi32(mT2, mAddOffset1);
        mT0 = _mm_srai_epi32(mT0, shift1);
        mT2 = _mm_srai_epi32(mT2, shift1);

        mVal = _mm_packs_epi32(mT0, mT2);
        _mm_storeu_si128((__m128i*)tmp, mVal);

        src += i_src;
        tmp += i_tmp;
    }

    // VER

    tmp = tmp_res;

    mCoef0 = _mm_set1_epi16(*(s16*)coeff_v);
    mCoef1 = _mm_set1_epi16(*(s16*)(coeff_v + 2));

    mCoef0 = _mm_cvtepi8_epi16(mCoef0);
    mCoef1 = _mm_cvtepi8_epi16(mCoef1);

    while (height--) {
        __m128i T00 = _mm_load_si128((__m128i*)(tmp));
        __m128i T10 = _mm_load_si128((__m128i*)(tmp + i_tmp));
        __m128i T20 = _mm_load_si128((__m128i*)(tmp + 2 * i_tmp));
        __m128i T30 = _mm_load_si128((__m128i*)(tmp + 3 * i_tmp));

        M0 = _mm_unpacklo_epi16(T00, T10);
        M1 = _mm_unpacklo_epi16(T20, T30);
        M2 = _mm_unpackhi_epi16(T00, T10);
        M3 = _mm_unpackhi_epi16(T20, T30);

        M0 = _mm_madd_epi16(M0, mCoef0);
        M1 = _mm_madd_epi16(M1, mCoef1);
        M2 = _mm_madd_epi16(M2, mCoef0);
        M3 = _mm_madd_epi16(M3, mCoef1);

        M0 = _mm_add_epi32(M0, M1);
        M2 = _mm_add_epi32(M2, M3);

        M0 = _mm_add_epi32(M0, mAddOffset2);
        M2 = _mm_add_epi32(M2, mAddOffset2);
        M0 = _mm_srai_epi32(M0, shift2);
        M2 = _mm_srai_epi32(M2, shift2);

        mVal = _mm_packus_epi32(M0, M2);
        mVal = _mm_min_epu16(mVal, max_pel);
        _mm_storeu_si128((__m128i*)dst, mVal);

        tmp += i_tmp;
        dst += i_dst;
    }
}

void uavs3d_if_hor_ver_chroma_w8x_sse(const pel *src, int i_src, pel *dst, int i_dst, int width, int height, const s8 *coeff_h, const s8 *coeff_v, int max_val)
{
    int row, col;
    s16 const *p;
    int shift1, shift2;
    int add1, add2;

    ALIGNED_16(s16 tmp_res[(128 + 3) * 128]);
    s16 *tmp = tmp_res;
    const int i_tmp = width;

    __m128i mAddOffset1, mAddOffset2;

    __m128i mSwitch = _mm_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 4, 5, 8, 9, 6, 7, 10, 11);
    __m128i mCoef0 = _mm_set1_epi16(*(s16*)coeff_h);
    __m128i mCoef1 = _mm_set1_epi16(*(s16*)(coeff_h + 2));

    __m128i max_val1 = _mm_set1_epi16((pel)max_val);
    __m128i mVal;
    __m128i M0, M1, M2, M3;
    __m128i mSrc0, mSrc1, mSrc2;
    __m128i mT0, mT1, mT2, mT3;

    mCoef0 = _mm_cvtepi8_epi16(mCoef0);
    mCoef1 = _mm_cvtepi8_epi16(mCoef1);

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

    mAddOffset1 = _mm_set1_epi32(add1);
    mAddOffset2 = _mm_set1_epi32(add2);

    // HOR
    src = src - 1 * i_src - 2;
    row = height + 3;

    while (row--) {
        uavs3d_prefetch(src + i_src, _MM_HINT_NTA);
        for (col = 0; col < width; col += 8) {
            mSrc0 = _mm_loadu_si128((__m128i*)(src + col));
            mSrc1 = _mm_loadu_si128((__m128i*)(src + col + 4));
            mSrc2 = _mm_loadu_si128((__m128i*)(src + col + 8));

            mT0 = _mm_madd_epi16(_mm_shuffle_epi8(mSrc0, mSwitch), mCoef0);
            mT1 = _mm_madd_epi16(_mm_shuffle_epi8(mSrc1, mSwitch), mCoef1);

            mT2 = _mm_madd_epi16(_mm_shuffle_epi8(mSrc1, mSwitch), mCoef0);
            mT3 = _mm_madd_epi16(_mm_shuffle_epi8(mSrc2, mSwitch), mCoef1);

            mT0 = _mm_add_epi32(mT0, mT1);
            mT2 = _mm_add_epi32(mT2, mT3);

            mT0 = _mm_add_epi32(mT0, mAddOffset1);
            mT2 = _mm_add_epi32(mT2, mAddOffset1);
            mT0 = _mm_srai_epi32(mT0, shift1);
            mT2 = _mm_srai_epi32(mT2, shift1);

            mVal = _mm_packs_epi32(mT0, mT2);
            _mm_storeu_si128((__m128i*)&tmp[col], mVal);
        }
        src += i_src;
        tmp += i_tmp;
    }

    // VER

    tmp = tmp_res;

    mCoef0 = _mm_set1_epi16(*(s16*)coeff_v);
    mCoef1 = _mm_set1_epi16(*(s16*)(coeff_v + 2));

    mCoef0 = _mm_cvtepi8_epi16(mCoef0);
    mCoef1 = _mm_cvtepi8_epi16(mCoef1);

    while (height--) {
        p = tmp;
        for (col = 0; col < width; col += 8) {
            __m128i T00 = _mm_load_si128((__m128i*)(p));
            __m128i T10 = _mm_load_si128((__m128i*)(p + i_tmp));
            __m128i T20 = _mm_load_si128((__m128i*)(p + 2 * i_tmp));
            __m128i T30 = _mm_load_si128((__m128i*)(p + 3 * i_tmp));

            M0 = _mm_unpacklo_epi16(T00, T10);
            M1 = _mm_unpacklo_epi16(T20, T30);
            M2 = _mm_unpackhi_epi16(T00, T10);
            M3 = _mm_unpackhi_epi16(T20, T30);

            M0 = _mm_madd_epi16(M0, mCoef0);
            M1 = _mm_madd_epi16(M1, mCoef1);
            M2 = _mm_madd_epi16(M2, mCoef0);
            M3 = _mm_madd_epi16(M3, mCoef1);

            M0 = _mm_add_epi32(M0, M1);
            M2 = _mm_add_epi32(M2, M3);

            M0 = _mm_add_epi32(M0, mAddOffset2);
            M2 = _mm_add_epi32(M2, mAddOffset2);
            M0 = _mm_srai_epi32(M0, shift2);
            M2 = _mm_srai_epi32(M2, shift2);

            mVal = _mm_packus_epi32(M0, M2);
            mVal = _mm_min_epu16(mVal, max_val1);
            _mm_storeu_si128((__m128i*)&dst[col], mVal);

            p += 8;
        }

        tmp += i_tmp;
        dst += i_dst;
    }
}

#endif
