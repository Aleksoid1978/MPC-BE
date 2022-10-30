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
void uavs3d_avg_pel_w8_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    int i_dst3 = i_dst + i_dst2;
    int i_dst4 = i_dst << 2;
    __m256i s0, s1, d0;
    __m128i m0, m1;

    while (height) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src2));

        d0 = _mm256_avg_epu8(s0, s1);
        m0 = _mm256_castsi256_si128(d0);
        m1 = _mm256_extractf128_si256(d0, 1);

        _mm_storel_epi64((__m128i*)dst, m0);
        _mm_storel_epi64((__m128i*)(dst + i_dst), _mm_srli_si128(m0, 8));
        _mm_storel_epi64((__m128i*)(dst + i_dst2), m1);
        _mm_storel_epi64((__m128i*)(dst + i_dst3), _mm_srli_si128(m1, 8));

        src1 += 32;
        src2 += 32;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_avg_pel_w16_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    int i_dst3 = i_dst + i_dst2;
    int i_dst4 = i_dst << 2;
    __m256i s0, s1, s2, s3, d0, d1;
    __m128i m0, m1, m2, m3;

    while (height) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 32));
        s2 = _mm256_load_si256((const __m256i*)(src2));
        s3 = _mm256_load_si256((const __m256i*)(src2 + 32));

        d0 = _mm256_avg_epu8(s0, s2);
        d1 = _mm256_avg_epu8(s1, s3);
        m0 = _mm256_castsi256_si128(d0);
        m1 = _mm256_extractf128_si256(d0, 1);
        m2 = _mm256_castsi256_si128(d1);
        m3 = _mm256_extractf128_si256(d1, 1);

        _mm_storeu_si128((__m128i*)dst, m0);
        _mm_storeu_si128((__m128i*)(dst + i_dst), m1);
        _mm_storeu_si128((__m128i*)(dst + i_dst2), m2);
        _mm_storeu_si128((__m128i*)(dst + i_dst3), m3);

        src1 += 64;
        src2 += 64;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_avg_pel_w32_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    __m256i s0, s1, s2, s3, d0, d1;

    while (height) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 32));
        s2 = _mm256_load_si256((const __m256i*)(src2));
        s3 = _mm256_load_si256((const __m256i*)(src2 + 32));

        d0 = _mm256_avg_epu8(s0, s2);
        d1 = _mm256_avg_epu8(s1, s3);

        _mm256_storeu_si256((__m256i*)(dst), d0);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), d1);

        src1 += 64;
        src2 += 64;
        dst += i_dst2;
        height -= 2;
    }
}

void uavs3d_avg_pel_w64_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    __m256i s0, s1, s2, s3, d0, d1;

    while (height--) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 32));
        s2 = _mm256_load_si256((const __m256i*)(src2));
        s3 = _mm256_load_si256((const __m256i*)(src2 + 32));

        d0 = _mm256_avg_epu8(s0, s2);
        d1 = _mm256_avg_epu8(s1, s3);

        _mm256_storeu_si256((__m256i*)(dst), d0);
        _mm256_storeu_si256((__m256i*)(dst + 32), d1);

        src1 += 64;
        src2 += 64;
        dst += i_dst;
    }
}

void uavs3d_avg_pel_w128_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    __m256i d0, d1, d2, d3;

    while (height--) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 32));
        s2 = _mm256_load_si256((const __m256i*)(src1 + 64));
        s3 = _mm256_load_si256((const __m256i*)(src1 + 96));
        s4 = _mm256_load_si256((const __m256i*)(src2));
        s5 = _mm256_load_si256((const __m256i*)(src2 + 32));
        s6 = _mm256_load_si256((const __m256i*)(src2 + 64));
        s7 = _mm256_load_si256((const __m256i*)(src2 + 96));

        d0 = _mm256_avg_epu8(s0, s4);
        d1 = _mm256_avg_epu8(s1, s5);
        d2 = _mm256_avg_epu8(s2, s6);
        d3 = _mm256_avg_epu8(s3, s7);

        _mm256_storeu_si256((__m256i*)(dst), d0);
        _mm256_storeu_si256((__m256i*)(dst + 32), d1);
        _mm256_storeu_si256((__m256i*)(dst + 64), d2);
        _mm256_storeu_si256((__m256i*)(dst + 96), d3);

        src1 += 128;
        src2 += 128;
        dst += i_dst;
    }
}

void uavs3d_recon_luma_w16_avx2(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    int i_rec2 = i_rec << 1;
    int i_pred2 = i_pred << 1;

    if (cbf == 0) {
        __m128i p0, p1;
        for (i = 0; i < height; i += 2) {
            p0 = _mm_loadu_si128((const __m128i*)(pred));
            p1 = _mm_loadu_si128((const __m128i*)(pred + i_pred));

            _mm_storeu_si128((__m128i*)(rec), p0);
            _mm_storeu_si128((__m128i*)(rec + i_rec), p1);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m128i p0, p1;
        __m256i zero = _mm256_setzero_si256();
        __m256i m0, m1, r0, r1;
        for (i = 0; i < height; i += 2) {
            p0 = _mm_loadu_si128((const __m128i*)(pred));
            p1 = _mm_loadu_si128((const __m128i*)(pred + i_pred));
            r0 = _mm256_loadu_si256((const __m256i*)(resi));
            r1 = _mm256_loadu_si256((const __m256i*)(resi + 16));
            m0 = _mm256_set_m128i(p1, p0);
            m0 = _mm256_permute4x64_epi64(m0, 0xd8);
            m1 = _mm256_unpackhi_epi8(m0, zero);
            m0 = _mm256_unpacklo_epi8(m0, zero);

            m0 = _mm256_adds_epi16(m0, r0);
            m1 = _mm256_adds_epi16(m1, r1);
            m0 = _mm256_packus_epi16(m0, m1);
            m0 = _mm256_permute4x64_epi64(m0, 0xd8);

            _mm_storeu_si128((__m128i*)(rec), _mm256_castsi256_si128(m0));
            _mm_storeu_si128((__m128i*)(rec + i_rec), _mm256_extracti128_si256(m0, 1));

            pred += i_pred2;
            rec += i_rec2;
            resi += 32;
        }
    }
}

void uavs3d_recon_luma_w32_avx2(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    if (cbf == 0) {
        int i_rec2 = i_rec << 1;
        int i_pred2 = i_pred << 1;
        __m256i p0, p1;
        for (i = 0; i < height; i += 2) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            p1 = _mm256_loadu_si256((const __m256i*)(pred + i_pred));

            _mm256_storeu_si256((__m256i*)(rec), p0);
            _mm256_storeu_si256((__m256i*)(rec + i_rec), p1);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m256i zero = _mm256_setzero_si256();
        __m256i m0, m1, r0, r1;
        for (i = 0; i < height; i++) {
            m0 = _mm256_loadu_si256((const __m256i*)(pred));
            r0 = _mm256_loadu_si256((const __m256i*)(resi));
            r1 = _mm256_loadu_si256((const __m256i*)(resi + 16));
            m0 = _mm256_permute4x64_epi64(m0, 0xd8);
            m1 = _mm256_unpackhi_epi8(m0, zero);
            m0 = _mm256_unpacklo_epi8(m0, zero);

            m0 = _mm256_adds_epi16(m0, r0);
            m1 = _mm256_adds_epi16(m1, r1);
            m0 = _mm256_packus_epi16(m0, m1);
            m0 = _mm256_permute4x64_epi64(m0, 0xd8);

            _mm256_storeu_si256((__m256i*)(rec), m0);

            pred += i_pred;
            rec += i_rec;
            resi += 32;
        }
    }
}

void uavs3d_recon_luma_w64_avx2(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    if (cbf == 0) {
        __m256i p0, p1, p2, p3;
        int i_rec2 = i_rec << 1;
        int i_pred2 = i_pred << 1;
        for (i = 0; i < height; i += 2) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            p1 = _mm256_loadu_si256((const __m256i*)(pred + 32));
            p2 = _mm256_loadu_si256((const __m256i*)(pred + i_pred));
            p3 = _mm256_loadu_si256((const __m256i*)(pred + i_pred + 32));

            _mm256_storeu_si256((__m256i*)(rec), p0);
            _mm256_storeu_si256((__m256i*)(rec + 32), p1);
            _mm256_storeu_si256((__m256i*)(rec + i_rec), p2);
            _mm256_storeu_si256((__m256i*)(rec + i_rec + 32), p3);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m256i zero = _mm256_setzero_si256();
        __m256i m0, m1, m2, m3, r0, r1, r2, r3;
        for (i = 0; i < height; i++) {
            m0 = _mm256_loadu_si256((const __m256i*)(pred));
            m1 = _mm256_loadu_si256((const __m256i*)(pred + 32));
            r0 = _mm256_loadu_si256((const __m256i*)(resi));
            r1 = _mm256_loadu_si256((const __m256i*)(resi + 16));
            r2 = _mm256_loadu_si256((const __m256i*)(resi + 32));
            r3 = _mm256_loadu_si256((const __m256i*)(resi + 48));
            m0 = _mm256_permute4x64_epi64(m0, 0xd8);
            m1 = _mm256_permute4x64_epi64(m1, 0xd8);
            m2 = _mm256_unpacklo_epi8(m1, zero);
            m3 = _mm256_unpackhi_epi8(m1, zero);
            m1 = _mm256_unpackhi_epi8(m0, zero);
            m0 = _mm256_unpacklo_epi8(m0, zero);

            m0 = _mm256_adds_epi16(m0, r0);
            m1 = _mm256_adds_epi16(m1, r1);
            m2 = _mm256_adds_epi16(m2, r2);
            m3 = _mm256_adds_epi16(m3, r3);
            m0 = _mm256_packus_epi16(m0, m1);
            m2 = _mm256_packus_epi16(m2, m3);
            m0 = _mm256_permute4x64_epi64(m0, 0xd8);
            m2 = _mm256_permute4x64_epi64(m2, 0xd8);

            _mm256_storeu_si256((__m256i*)(rec), m0);
            _mm256_storeu_si256((__m256i*)(rec + 32), m2);

            pred += i_pred;
            rec  += i_rec;
            resi += 64;
        }
    }
}

void uavs3d_recon_chroma_w16_avx2(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i;
    __m256i p0, p1, r0, r1, r2, r3;
    __m256i zero = _mm256_setzero_si256();

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            r0 = _mm256_loadu_si256((const __m256i*)(resi_u));
            r1 = _mm256_loadu_si256((const __m256i*)(resi_v));
            p1 = _mm256_unpackhi_epi8(p0, zero);
            p0 = _mm256_unpacklo_epi8(p0, zero);
            r2 = _mm256_unpacklo_epi16(r0, r1);    // UV interlaced
            r3 = _mm256_unpackhi_epi16(r0, r1);
            p0 = _mm256_adds_epi16(p0, r2);
            p1 = _mm256_adds_epi16(p1, r3);

            p0 = _mm256_packus_epi16(p0, p1);

            _mm256_storeu_si256((__m256i*)(rec), p0);

            pred += 32;
            resi_u += 16;
            resi_v += 16;
            rec += i_rec;
        }
    }
    else if (cbf[0]) {
        for (i = 0; i < height; i++) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            r0 = _mm256_loadu_si256((const __m256i*)(resi_u));
            p1 = _mm256_unpackhi_epi8(p0, zero);
            p0 = _mm256_unpacklo_epi8(p0, zero);
            r2 = _mm256_unpacklo_epi16(r0, zero);    // UV interlaced
            r3 = _mm256_unpackhi_epi16(r0, zero);
            p0 = _mm256_adds_epi16(p0, r2);
            p1 = _mm256_adds_epi16(p1, r3);

            p0 = _mm256_packus_epi16(p0, p1);

            _mm256_storeu_si256((__m256i*)(rec), p0);

            pred += 32;
            resi_u += 16;
            rec += i_rec;
        }
    }
    else {
        for (i = 0; i < height; i++) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            r1 = _mm256_loadu_si256((const __m256i*)(resi_v));
            p1 = _mm256_unpackhi_epi8(p0, zero);
            p0 = _mm256_unpacklo_epi8(p0, zero);
            r2 = _mm256_unpacklo_epi16(zero, r1);    // UV interlaced
            r3 = _mm256_unpackhi_epi16(zero, r1);
            p0 = _mm256_adds_epi16(p0, r2);
            p1 = _mm256_adds_epi16(p1, r3);

            p0 = _mm256_packus_epi16(p0, p1);

            _mm256_storeu_si256((__m256i*)(rec), p0);

            pred += 32;
            resi_v += 16;
            rec += i_rec;
        }
    }
}

void uavs3d_recon_chroma_w16x_avx2(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i, j;
    int width2 = width << 1;
    __m256i p0, p1, r0, r1, r2, r3;
    __m256i zero = _mm256_setzero_si256();

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 16) {
                int j2 = j << 1;
                p0 = _mm256_loadu_si256((const __m256i*)(pred + j2));
                r0 = _mm256_loadu_si256((const __m256i*)(resi_u + j));
                r1 = _mm256_loadu_si256((const __m256i*)(resi_v + j));
                p1 = _mm256_unpackhi_epi8(p0, zero);
                p0 = _mm256_unpacklo_epi8(p0, zero);
                r2 = _mm256_unpacklo_epi16(r0, r1);    // UV interlaced
                r3 = _mm256_unpackhi_epi16(r0, r1);
                p0 = _mm256_adds_epi16(p0, r2);
                p1 = _mm256_adds_epi16(p1, r3);

                p0 = _mm256_packus_epi16(p0, p1);

                _mm256_storeu_si256((__m256i*)(rec + j2), p0);
            }
            pred += width2;
            resi_u += width;
            resi_v += width;
            rec += i_rec;
        }
    }
    else if (cbf[0]) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 16) {
                int j2 = j << 1;
                p0 = _mm256_loadu_si256((const __m256i*)(pred + j2));
                r0 = _mm256_loadu_si256((const __m256i*)(resi_u + j));
                p1 = _mm256_unpackhi_epi8(p0, zero);
                p0 = _mm256_unpacklo_epi8(p0, zero);
                r2 = _mm256_unpacklo_epi16(r0, zero);    // UV interlaced
                r3 = _mm256_unpackhi_epi16(r0, zero);
                p0 = _mm256_adds_epi16(p0, r2);
                p1 = _mm256_adds_epi16(p1, r3);

                p0 = _mm256_packus_epi16(p0, p1);

                _mm256_storeu_si256((__m256i*)(rec + j2), p0);
            }
            pred += width2;
            resi_u += width;
            rec += i_rec;
        }
    }
    else {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 16) {
                int j2 = j << 1;
                p0 = _mm256_loadu_si256((const __m256i*)(pred + j2));
                r1 = _mm256_loadu_si256((const __m256i*)(resi_v + j));
                p1 = _mm256_unpackhi_epi8(p0, zero);
                p0 = _mm256_unpacklo_epi8(p0, zero);
                r2 = _mm256_unpacklo_epi16(zero, r1);    // UV interlaced
                r3 = _mm256_unpackhi_epi16(zero, r1);
                p0 = _mm256_adds_epi16(p0, r2);
                p1 = _mm256_adds_epi16(p1, r3);

                p0 = _mm256_packus_epi16(p0, p1);

                _mm256_storeu_si256((__m256i*)(rec + j2), p0);
            }
            pred += width2;
            resi_v += width;
            rec += i_rec;
        }
    }
}

#else // 10bit
void uavs3d_avg_pel_w4_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    int i_dst3 = i_dst + i_dst2;
    int i_dst4 = i_dst << 2;
    __m256i s0, s1, d0;
    __m128i m0, m1;

    while (height) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src2));

        d0 = _mm256_avg_epu16(s0, s1);
        m0 = _mm256_castsi256_si128(d0);
        m1 = _mm256_extractf128_si256(d0, 1);

        _mm_storel_epi64((__m128i*)dst, m0);
        _mm_storel_epi64((__m128i*)(dst + i_dst), _mm_srli_si128(m0, 8));
        _mm_storel_epi64((__m128i*)(dst + i_dst2), m1);
        _mm_storel_epi64((__m128i*)(dst + i_dst3), _mm_srli_si128(m1, 8));

        src1 += 16;
        src2 += 16;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_avg_pel_w8_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    int i_dst3 = i_dst + i_dst2;
    int i_dst4 = i_dst << 2;
    __m256i s0, s1, s2, s3, d0, d1;
    __m128i m0, m1, m2, m3;

    while (height) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 16));
        s2 = _mm256_load_si256((const __m256i*)(src2));
        s3 = _mm256_load_si256((const __m256i*)(src2 + 16));

        d0 = _mm256_avg_epu16(s0, s2);
        d1 = _mm256_avg_epu16(s1, s3);
        m0 = _mm256_castsi256_si128(d0);
        m1 = _mm256_extractf128_si256(d0, 1);
        m2 = _mm256_castsi256_si128(d1);
        m3 = _mm256_extractf128_si256(d1, 1);

        _mm_storeu_si128((__m128i*)dst, m0);
        _mm_storeu_si128((__m128i*)(dst + i_dst), m1);
        _mm_storeu_si128((__m128i*)(dst + i_dst2), m2);
        _mm_storeu_si128((__m128i*)(dst + i_dst3), m3);

        src1 += 32;
        src2 += 32;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_avg_pel_w16_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    __m256i s0, s1, s2, s3, d0, d1;

    while (height) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 16));
        s2 = _mm256_load_si256((const __m256i*)(src2));
        s3 = _mm256_load_si256((const __m256i*)(src2 + 16));

        d0 = _mm256_avg_epu16(s0, s2);
        d1 = _mm256_avg_epu16(s1, s3);

        _mm256_storeu_si256((__m256i*)(dst), d0);
        _mm256_storeu_si256((__m256i*)(dst + i_dst), d1);

        src1 += 32;
        src2 += 32;
        dst += i_dst2;
        height -= 2;
    }
}

void uavs3d_avg_pel_w32_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    __m256i s0, s1, s2, s3, d0, d1;

    while (height--) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 16));
        s2 = _mm256_load_si256((const __m256i*)(src2));
        s3 = _mm256_load_si256((const __m256i*)(src2 + 16));

        d0 = _mm256_avg_epu16(s0, s2);
        d1 = _mm256_avg_epu16(s1, s3);

        _mm256_storeu_si256((__m256i*)(dst), d0);
        _mm256_storeu_si256((__m256i*)(dst + 16), d1);

        src1 += 32;
        src2 += 32;
        dst += i_dst;
    }
}

void uavs3d_avg_pel_w64_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    __m256i d0, d1, d2, d3;

    while (height--) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 16));
        s2 = _mm256_load_si256((const __m256i*)(src1 + 32));
        s3 = _mm256_load_si256((const __m256i*)(src1 + 48));
        s4 = _mm256_load_si256((const __m256i*)(src2));
        s5 = _mm256_load_si256((const __m256i*)(src2 + 16));
        s6 = _mm256_load_si256((const __m256i*)(src2 + 32));
        s7 = _mm256_load_si256((const __m256i*)(src2 + 48));

        d0 = _mm256_avg_epu16(s0, s4);
        d1 = _mm256_avg_epu16(s1, s5);
        d2 = _mm256_avg_epu16(s2, s6);
        d3 = _mm256_avg_epu16(s3, s7);

        _mm256_storeu_si256((__m256i*)(dst), d0);
        _mm256_storeu_si256((__m256i*)(dst + 16), d1);
        _mm256_storeu_si256((__m256i*)(dst + 32), d2);
        _mm256_storeu_si256((__m256i*)(dst + 48), d3);

        src1 += 64;
        src2 += 64;
        dst += i_dst;
    }
}

void uavs3d_avg_pel_w128_avx2(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    __m256i d0, d1, d2, d3;

    while (height--) {
        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 16));
        s2 = _mm256_load_si256((const __m256i*)(src1 + 32));
        s3 = _mm256_load_si256((const __m256i*)(src1 + 48));
        s4 = _mm256_load_si256((const __m256i*)(src2));
        s5 = _mm256_load_si256((const __m256i*)(src2 + 16));
        s6 = _mm256_load_si256((const __m256i*)(src2 + 32));
        s7 = _mm256_load_si256((const __m256i*)(src2 + 48));

        d0 = _mm256_avg_epu16(s0, s4);
        d1 = _mm256_avg_epu16(s1, s5);
        d2 = _mm256_avg_epu16(s2, s6);
        d3 = _mm256_avg_epu16(s3, s7);

        src1 += 64;
        src2 += 64;

        _mm256_storeu_si256((__m256i*)(dst), d0);
        _mm256_storeu_si256((__m256i*)(dst + 16), d1);
        _mm256_storeu_si256((__m256i*)(dst + 32), d2);
        _mm256_storeu_si256((__m256i*)(dst + 48), d3);

        s0 = _mm256_load_si256((const __m256i*)(src1));
        s1 = _mm256_load_si256((const __m256i*)(src1 + 16));
        s2 = _mm256_load_si256((const __m256i*)(src1 + 32));
        s3 = _mm256_load_si256((const __m256i*)(src1 + 48));
        s4 = _mm256_load_si256((const __m256i*)(src2));
        s5 = _mm256_load_si256((const __m256i*)(src2 + 16));
        s6 = _mm256_load_si256((const __m256i*)(src2 + 32));
        s7 = _mm256_load_si256((const __m256i*)(src2 + 48));

        d0 = _mm256_avg_epu16(s0, s4);
        d1 = _mm256_avg_epu16(s1, s5);
        d2 = _mm256_avg_epu16(s2, s6);
        d3 = _mm256_avg_epu16(s3, s7);

        src1 += 64;
        src2 += 64;

        _mm256_storeu_si256((__m256i*)(dst + 64), d0);
        _mm256_storeu_si256((__m256i*)(dst + 80), d1);
        _mm256_storeu_si256((__m256i*)(dst + 96), d2);
        _mm256_storeu_si256((__m256i*)(dst + 112), d3);

        dst += i_dst;
    }
}

void uavs3d_padding_rows_luma_avx2(pel *src, int i_src, int width, int height, int start, int rows, int padh, int padv)
{
    int i, j;
    pel *p, *p1, *p2;
    start = max(start, 0);

    uavs3d_assert(padh % 16 == 0);

    if (start + rows > height) {
        rows = height - start;
    }

    p = src + start * i_src;

    // left & right
    for (i = 0; i < rows; i++) {
        __m256i Val1, Val2;
        Val1 = _mm256_set1_epi16((pel)p[0]);
        Val2 = _mm256_set1_epi16((pel)p[width - 1]);
        uavs3d_prefetch(p + i_src, _MM_HINT_NTA);
        uavs3d_prefetch(p + i_src + width - 1, _MM_HINT_NTA);
        p1 = p - padh;
        p2 = p + width;
        for (j = 0; j < padh - 15; j += 16) {
            _mm256_storeu_si256((__m256i*)(p1 + j), Val1);
            _mm256_storeu_si256((__m256i*)(p2 + j), Val2);
        }

        p += i_src;
    }

    if (start == 0) {
        p = src - padh;
        for (i = 1; i <= padv; i++) {
            memcpy(p - i_src * i, p, (width + 2 * padh) * sizeof(pel));
        }
    }

    if (start + rows == height) {
        p = src + i_src * (height - 1) - padh;
        for (i = 1; i <= padv; i++) {
            memcpy(p + i_src * i, p, (width + 2 * padh) * sizeof(pel));
        }
    }
}

void uavs3d_padding_rows_chroma_avx2(pel *src, int i_src, int width, int height, int start, int rows, int padh, int padv)
{
    int i, j;
    pel *p, *p1, *p2;
    int width2 = width >> 1;
    start = max(start, 0);

    uavs3d_assert(padh % 16 == 0);

    if (start + rows > height) {
        rows = height - start;
    }

    p = src + start * i_src;

    // left & right
    for (i = 0; i < rows; i++) {
        __m256i Val1, Val2;

        Val1 = _mm256_set1_epi32(((u32*)p)[0]);
        Val2 = _mm256_set1_epi32(((u32*)p)[width2 - 1]);
        uavs3d_prefetch(p + i_src, _MM_HINT_NTA);
        uavs3d_prefetch(p + i_src + width - 2, _MM_HINT_NTA);
        p1 = p - padh;
        p2 = p + width;
        for (j = 0; j < padh - 15; j += 16) {
            _mm256_storeu_si256((__m256i*)(p1 + j), Val1);
            _mm256_storeu_si256((__m256i*)(p2 + j), Val2);
        }

        p += i_src;
    }

    if (start == 0) {
        p = src - padh;
        for (i = 1; i <= padv; i++) {
            memcpy(p - i_src * i, p, (width + 2 * padh) * sizeof(pel));
        }
    }

    if (start + rows == height) {
        p = src + i_src * (height - 1) - padh;
        for (i = 1; i <= padv; i++) {
            memcpy(p + i_src * i, p, (width + 2 * padh) * sizeof(pel));
        }
    }
}

void uavs3d_recon_luma_w8_avx2(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    int i_rec2 = i_rec << 1;
    int i_rec3 = i_rec + i_rec2;
    int i_rec4 = i_rec << 2;
    int i_pred2 = i_pred << 1;
    int i_pred3 = i_pred + i_pred2;
    int i_pred4 = i_pred << 2;
    __m128i p0, p1, p2, p3;

    if (cbf == 0) {
        for (i = 0; i < height; i += 4) {
            p0 = _mm_loadu_si128((const __m128i*)(pred));
            p1 = _mm_loadu_si128((const __m128i*)(pred + i_pred));
            p2 = _mm_loadu_si128((const __m128i*)(pred + i_pred2));
            p3 = _mm_loadu_si128((const __m128i*)(pred + i_pred3));

            _mm_storeu_si128((__m128i*)(rec), p0);
            _mm_storeu_si128((__m128i*)(rec + i_rec), p1);
            _mm_storeu_si128((__m128i*)(rec + i_rec2), p2);
            _mm_storeu_si128((__m128i*)(rec + i_rec3), p3);
            rec += i_rec4;
            pred += i_pred4;
        }
    }
    else {
        __m256i min_pel = _mm256_setzero_si256();
        __m256i max_pel = _mm256_set1_epi16((1 << bit_depth) - 1);
        __m256i m0, m1, r0, r1;
        for (i = 0; i < height; i += 4) {
            p0 = _mm_loadu_si128((const __m128i*)(pred));
            p1 = _mm_loadu_si128((const __m128i*)(pred + i_pred));
            p2 = _mm_loadu_si128((const __m128i*)(pred + i_pred2));
            p3 = _mm_loadu_si128((const __m128i*)(pred + i_pred3));
            r0 = _mm256_loadu_si256((const __m256i*)(resi));
            r1 = _mm256_loadu_si256((const __m256i*)(resi + 16));
            m0 = _mm256_set_m128i(p1, p0);
            m1 = _mm256_set_m128i(p3, p2);
            
            m0 = _mm256_adds_epi16(m0, r0);
            m1 = _mm256_adds_epi16(m1, r1);

            m0 = _mm256_min_epi16(m0, max_pel);
            m1 = _mm256_min_epi16(m1, max_pel);
            m0 = _mm256_max_epi16(m0, min_pel);
            m1 = _mm256_max_epi16(m1, min_pel);

            _mm_storeu_si128((__m128i*)(rec), _mm256_castsi256_si128(m0));
            _mm_storeu_si128((__m128i*)(rec + i_rec), _mm256_extracti128_si256(m0, 1));
            _mm_storeu_si128((__m128i*)(rec + i_rec2), _mm256_castsi256_si128(m1));
            _mm_storeu_si128((__m128i*)(rec + i_rec3), _mm256_extracti128_si256(m1, 1));

            pred += i_pred4;
            rec += i_rec4;
            resi += 32;
        }
    }
}

void uavs3d_recon_luma_w16_avx2(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    int i_rec2 = i_rec << 1;
    int i_pred2 = i_pred << 1;

    if (cbf == 0) {
        __m256i p0, p1;
        for (i = 0; i < height; i += 2) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            p1 = _mm256_loadu_si256((const __m256i*)(pred + i_pred));

            _mm256_storeu_si256((__m256i*)(rec), p0);
            _mm256_storeu_si256((__m256i*)(rec + i_rec), p1);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m256i p0, p1;
        __m256i min_pel = _mm256_setzero_si256();
        __m256i max_pel = _mm256_set1_epi16((1 << bit_depth) - 1);
        __m256i m0, m1, r0, r1;
        for (i = 0; i < height; i += 2) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            p1 = _mm256_loadu_si256((const __m256i*)(pred + i_pred));
            r0 = _mm256_loadu_si256((const __m256i*)(resi));
            r1 = _mm256_loadu_si256((const __m256i*)(resi + 16));
            
            m0 = _mm256_adds_epi16(p0, r0);
            m1 = _mm256_adds_epi16(p1, r1);
            
            m0 = _mm256_min_epi16(m0, max_pel);
            m1 = _mm256_min_epi16(m1, max_pel);
            m0 = _mm256_max_epi16(m0, min_pel);
            m1 = _mm256_max_epi16(m1, min_pel);

            _mm256_storeu_si256((__m256i*)(rec), m0);
            _mm256_storeu_si256((__m256i*)(rec + i_rec), m1);

            pred += i_pred2;
            rec += i_rec2;
            resi += 32;
        }
    }
}

void uavs3d_recon_luma_w32_avx2(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    if (cbf == 0) {
        int i_rec2 = i_rec << 1;
        int i_pred2 = i_pred << 1;
        __m256i p0, p1, p2, p3;
        for (i = 0; i < height; i += 2) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            p1 = _mm256_loadu_si256((const __m256i*)(pred + 16));
            p2 = _mm256_loadu_si256((const __m256i*)(pred + i_pred));
            p3 = _mm256_loadu_si256((const __m256i*)(pred + i_pred + 16));

            _mm256_storeu_si256((__m256i*)(rec), p0);
            _mm256_storeu_si256((__m256i*)(rec + 16), p1);
            _mm256_storeu_si256((__m256i*)(rec + i_rec), p2);
            _mm256_storeu_si256((__m256i*)(rec + i_rec + 16), p3);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m256i min_pel = _mm256_setzero_si256();
        __m256i max_pel = _mm256_set1_epi16((1 << bit_depth) - 1);
        __m256i m0, m1, r0, r1;
        for (i = 0; i < height; i++) {
            m0 = _mm256_loadu_si256((const __m256i*)(pred));
            m1 = _mm256_loadu_si256((const __m256i*)(pred + 16));
            r0 = _mm256_loadu_si256((const __m256i*)(resi));
            r1 = _mm256_loadu_si256((const __m256i*)(resi + 16));
            
            m0 = _mm256_adds_epi16(m0, r0);
            m1 = _mm256_adds_epi16(m1, r1);

            m0 = _mm256_min_epi16(m0, max_pel);
            m1 = _mm256_min_epi16(m1, max_pel);
            m0 = _mm256_max_epi16(m0, min_pel);
            m1 = _mm256_max_epi16(m1, min_pel);

            _mm256_storeu_si256((__m256i*)(rec), m0);
            _mm256_storeu_si256((__m256i*)(rec + 16), m1);

            pred += i_pred;
            rec += i_rec;
            resi += 32;
        }
    }
}

void uavs3d_recon_luma_w64_avx2(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    if (cbf == 0) {
        __m256i p0, p1, p2, p3;
        for (i = 0; i < height; i++) {
            p0 = _mm256_load_si256((const __m256i*)(pred));
            p1 = _mm256_load_si256((const __m256i*)(pred + 16));
            p2 = _mm256_load_si256((const __m256i*)(pred + 32));
            p3 = _mm256_load_si256((const __m256i*)(pred + 48));

            _mm256_storeu_si256((__m256i*)(rec), p0);
            _mm256_storeu_si256((__m256i*)(rec + 16), p1);
            _mm256_storeu_si256((__m256i*)(rec + 32), p2);
            _mm256_storeu_si256((__m256i*)(rec + 48), p3);
            rec += i_rec;
            pred += i_pred;
        }
    }
    else {
        __m256i min_pel = _mm256_setzero_si256();
        __m256i max_pel = _mm256_set1_epi16((1 << bit_depth) - 1);
        __m256i m0, m1, m2, m3, r0, r1, r2, r3;
        for (i = 0; i < height; i++) {
            m0 = _mm256_load_si256((const __m256i*)(pred));
            m1 = _mm256_load_si256((const __m256i*)(pred + 16));
            m2 = _mm256_load_si256((const __m256i*)(pred + 32));
            m3 = _mm256_load_si256((const __m256i*)(pred + 48));
            r0 = _mm256_load_si256((const __m256i*)(resi));
            r1 = _mm256_load_si256((const __m256i*)(resi + 16));
            r2 = _mm256_load_si256((const __m256i*)(resi + 32));
            r3 = _mm256_load_si256((const __m256i*)(resi + 48));
            
            m0 = _mm256_adds_epi16(m0, r0);
            m1 = _mm256_adds_epi16(m1, r1);
            m2 = _mm256_adds_epi16(m2, r2);
            m3 = _mm256_adds_epi16(m3, r3);

            m0 = _mm256_min_epi16(m0, max_pel);
            m1 = _mm256_min_epi16(m1, max_pel);
            m2 = _mm256_min_epi16(m2, max_pel);
            m3 = _mm256_min_epi16(m3, max_pel);
            m0 = _mm256_max_epi16(m0, min_pel);
            m1 = _mm256_max_epi16(m1, min_pel);
            m2 = _mm256_max_epi16(m2, min_pel);
            m3 = _mm256_max_epi16(m3, min_pel);

            _mm256_storeu_si256((__m256i*)(rec), m0);
            _mm256_storeu_si256((__m256i*)(rec + 16), m1);
            _mm256_storeu_si256((__m256i*)(rec + 32), m2);
            _mm256_storeu_si256((__m256i*)(rec + 48), m3);

            pred += i_pred;
            rec += i_rec;
            resi += 64;
        }
    }
}

void uavs3d_recon_chroma_w16_avx2(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i;
    __m256i p0, p1, r0, r1, r2, r3;
    __m256i zero = _mm256_setzero_si256();
    __m256i max_pel = _mm256_set1_epi16((1 << bit_depth) - 1);

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            p1 = _mm256_loadu_si256((const __m256i*)(pred + 16));
            r0 = _mm256_loadu_si256((const __m256i*)(resi_u));
            r1 = _mm256_loadu_si256((const __m256i*)(resi_v));
            r2 = _mm256_unpacklo_epi16(r0, r1);    // UV interlaced: uv0-uv4 uv8-uv12
            r3 = _mm256_unpackhi_epi16(r0, r1);
            r0 = _mm256_permute2x128_si256(r2, r3, 0x20);  // uv0-uv8
            r1 = _mm256_permute2x128_si256(r2, r3, 0x31);
            p0 = _mm256_adds_epi16(p0, r0);
            p1 = _mm256_adds_epi16(p1, r1);

            p0 = _mm256_min_epi16(p0, max_pel);
            p1 = _mm256_min_epi16(p1, max_pel);
            p0 = _mm256_max_epi16(p0, zero);
            p1 = _mm256_max_epi16(p1, zero);

            _mm256_storeu_si256((__m256i*)(rec), p0);
            _mm256_storeu_si256((__m256i*)(rec + 16), p1);

            pred += 32;
            resi_u += 16;
            resi_v += 16;
            rec += i_rec;
        }
    }
    else if (cbf[0]) {
        for (i = 0; i < height; i++) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            p1 = _mm256_loadu_si256((const __m256i*)(pred + 16));
            r0 = _mm256_loadu_si256((const __m256i*)(resi_u));
            r2 = _mm256_cvtepu16_epi32(_mm256_castsi256_si128(r0));
            r3 = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(r0, 1));
            p0 = _mm256_adds_epi16(p0, r2);
            p1 = _mm256_adds_epi16(p1, r3);

            p0 = _mm256_min_epi16(p0, max_pel);
            p1 = _mm256_min_epi16(p1, max_pel);
            p0 = _mm256_max_epi16(p0, zero);
            p1 = _mm256_max_epi16(p1, zero);

            _mm256_storeu_si256((__m256i*)(rec), p0);
            _mm256_storeu_si256((__m256i*)(rec + 16), p1);

            pred += 32;
            resi_u += 16;
            rec += i_rec;
        }
    }
    else {
        for (i = 0; i < height; i++) {
            p0 = _mm256_loadu_si256((const __m256i*)(pred));
            p1 = _mm256_loadu_si256((const __m256i*)(pred + 16));
            r1 = _mm256_loadu_si256((const __m256i*)(resi_v));
            r2 = _mm256_unpacklo_epi16(zero, r1);    // UV interlaced: uv0-uv4 uv8-uv12
            r3 = _mm256_unpackhi_epi16(zero, r1);
            r0 = _mm256_permute2x128_si256(r2, r3, 0x20);  // uv0-uv8
            r1 = _mm256_permute2x128_si256(r2, r3, 0x31);
            p0 = _mm256_adds_epi16(p0, r0);
            p1 = _mm256_adds_epi16(p1, r1);

            p0 = _mm256_min_epi16(p0, max_pel);
            p1 = _mm256_min_epi16(p1, max_pel);
            p0 = _mm256_max_epi16(p0, zero);
            p1 = _mm256_max_epi16(p1, zero);

            _mm256_storeu_si256((__m256i*)(rec), p0);
            _mm256_storeu_si256((__m256i*)(rec + 16), p1);

            pred += 32;
            resi_v += 16;
            rec += i_rec;
        }
    }
}

void uavs3d_recon_chroma_w16x_avx2(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i, j;
    int width2 = width << 1;
    __m256i p0, p1, r0, r1, r2, r3;
    __m256i zero = _mm256_setzero_si256();
    __m256i max_pel = _mm256_set1_epi16((1 << bit_depth) - 1);

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 16) {
                int j2 = j << 1;
                p0 = _mm256_loadu_si256((const __m256i*)(pred + j2));
                p1 = _mm256_loadu_si256((const __m256i*)(pred + j2 + 16));
                r0 = _mm256_loadu_si256((const __m256i*)(resi_u + j));
                r1 = _mm256_loadu_si256((const __m256i*)(resi_v + j));
                r2 = _mm256_unpacklo_epi16(r0, r1);    // UV interlaced: uv0-uv4 uv8-uv12
                r3 = _mm256_unpackhi_epi16(r0, r1);
                r0 = _mm256_permute2x128_si256(r2, r3, 0x20);  // uv0-uv8
                r1 = _mm256_permute2x128_si256(r2, r3, 0x31);
                p0 = _mm256_adds_epi16(p0, r0);
                p1 = _mm256_adds_epi16(p1, r1);

                p0 = _mm256_min_epi16(p0, max_pel);
                p1 = _mm256_min_epi16(p1, max_pel);
                p0 = _mm256_max_epi16(p0, zero);
                p1 = _mm256_max_epi16(p1, zero);

                _mm256_storeu_si256((__m256i*)(rec + j2), p0);
                _mm256_storeu_si256((__m256i*)(rec + j2 + 16), p1);
            }
            pred += width2;
            resi_u += width;
            resi_v += width;
            rec += i_rec;
        }
    }
    else if (cbf[0]) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 16) {
                int j2 = j << 1;
                p0 = _mm256_loadu_si256((const __m256i*)(pred + j2));
                p1 = _mm256_loadu_si256((const __m256i*)(pred + j2 + 16));
                r0 = _mm256_loadu_si256((const __m256i*)(resi_u + j));
                r2 = _mm256_cvtepu16_epi32(_mm256_castsi256_si128(r0));
                r3 = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(r0, 1));
                p0 = _mm256_adds_epi16(p0, r2);
                p1 = _mm256_adds_epi16(p1, r3);

                p0 = _mm256_min_epi16(p0, max_pel);
                p1 = _mm256_min_epi16(p1, max_pel);
                p0 = _mm256_max_epi16(p0, zero);
                p1 = _mm256_max_epi16(p1, zero);

                _mm256_storeu_si256((__m256i*)(rec + j2), p0);
                _mm256_storeu_si256((__m256i*)(rec + j2 + 16), p1);
            }
            pred += width2;
            resi_u += width;
            rec += i_rec;
        }
    }
    else {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 16) {
                int j2 = j << 1;
                p0 = _mm256_loadu_si256((const __m256i*)(pred + j2));
                p1 = _mm256_loadu_si256((const __m256i*)(pred + j2 + 16));
                r1 = _mm256_loadu_si256((const __m256i*)(resi_v + j));
                r2 = _mm256_unpacklo_epi16(zero, r1);    // UV interlaced: uv0-uv4 uv8-uv12
                r3 = _mm256_unpackhi_epi16(zero, r1);
                r0 = _mm256_permute2x128_si256(r2, r3, 0x20);  // uv0-uv8
                r1 = _mm256_permute2x128_si256(r2, r3, 0x31);
                p0 = _mm256_adds_epi16(p0, r0);
                p1 = _mm256_adds_epi16(p1, r1);

                p0 = _mm256_min_epi16(p0, max_pel);
                p1 = _mm256_min_epi16(p1, max_pel);
                p0 = _mm256_max_epi16(p0, zero);
                p1 = _mm256_max_epi16(p1, zero);

                _mm256_storeu_si256((__m256i*)(rec + j2), p0);
                _mm256_storeu_si256((__m256i*)(rec + j2 + 16), p1);
            }
            pred += width2;
            resi_v += width;
            rec += i_rec;
        }
    }
}

#endif

void uavs3d_conv_fmt_8bit_avx2(unsigned char* src_y, unsigned char* src_uv, unsigned char* dst[3], int width, int height, int src_stride, int src_stridec, int dst_stride[3], int uv_shift)
{
    __m256i uv_mask = _mm256_set1_epi16(0xff), msrc, msrcu, msrcv;
    unsigned char* p_src = src_y;
    unsigned char* p_dst_y = dst[0];
    unsigned char* p_dst_u = dst[1];
    unsigned char* p_dst_v = dst[2];
    int i, j, j2;
    int width32;
    for (i = 0; i < height; i++) {
        memcpy(p_dst_y, p_src, width);
        p_src += src_stride;
        p_dst_y += dst_stride[0];
    }
    width >>= uv_shift;
    height >>= uv_shift;

    p_src = src_uv;
    width <<= 1;
    width32 = (width >> 5) << 5;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width32; j += 32) {
            j2 = j >> 1;
            msrc = _mm256_loadu_si256((const __m256i*)(p_src + j));
            msrcu = _mm256_and_si256(msrc, uv_mask);
            msrcv = _mm256_and_si256(_mm256_srli_si256(msrc, 1), uv_mask);
            msrc = _mm256_packus_epi16(msrcu, msrcv);
            msrc = _mm256_permute4x64_epi64(msrc, 0xd8);
            _mm_storeu_si128((__m128i*)(p_dst_u + j2), _mm256_castsi256_si128(msrc));
            _mm_storeu_si128((__m128i*)(p_dst_v + j2), _mm256_extracti128_si256(msrc, 1));
        }
        for (; j < width; j += 2) {
            p_dst_u[j >> 1] = p_src[j];
            p_dst_v[j >> 1] = p_src[j + 1];
        }
        p_src += src_stridec;
        p_dst_u += dst_stride[1];
        p_dst_v += dst_stride[2];
    }
}

void uavs3d_conv_fmt_16bit_avx2(unsigned char* src_y, unsigned char* src_uv, unsigned char* dst[3], int width, int height, int src_stride, int src_stridec, int dst_stride[3], int uv_shift)
{
    __m256i uv_mask = _mm256_set1_epi32(0xffff), msrc, msrcu, msrcv;
    unsigned char* p_src = src_y;
    unsigned char* p_dst_y = dst[0];
    unsigned char* p_dst_u = dst[1];
    unsigned char* p_dst_v = dst[2];
    int i, j, j2;
    int width32;
    width <<= 1;
    for (i = 0; i < height; i++) {
        memcpy(p_dst_y, p_src, width);
        p_src += src_stride;
        p_dst_y += dst_stride[0];
    }
    width >>= uv_shift;
    height >>= uv_shift;

    p_src = src_uv;
    width <<= 1;
    width32 = (width >> 5) << 5;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width32; j += 32) {
            j2 = j >> 1;
            msrc = _mm256_loadu_si256((const __m256i*)(p_src + j));
            msrcu = _mm256_and_si256(msrc, uv_mask);
            msrcv = _mm256_and_si256(_mm256_srli_si256(msrc, 2), uv_mask);
            msrc = _mm256_packus_epi32(msrcu, msrcv);
            msrc = _mm256_permute4x64_epi64(msrc, 0xd8);
            _mm_storeu_si128((__m128i*)(p_dst_u + j2), _mm256_castsi256_si128(msrc));
            _mm_storeu_si128((__m128i*)(p_dst_v + j2), _mm256_extracti128_si256(msrc, 1));
        }
        for (; j < width; j += 4) {
            j2 = j >> 1;    // for short
            ((short*)p_dst_u)[j2 >> 1] = ((short*)p_src)[j2];
            ((short*)p_dst_v)[j2 >> 1] = ((short*)p_src)[j2 + 1];
        }
        p_src += src_stridec;
        p_dst_u += dst_stride[1];
        p_dst_v += dst_stride[2];
    }
}

void uavs3d_conv_fmt_16to8bit_avx2(unsigned char* src_y, unsigned char* src_uv, unsigned char* dst[3], int width, int height, int src_stride, int src_stridec, int dst_stride[3], int uv_shift)
{
    __m256i uv_mask = _mm256_set1_epi32(0xffff);
    __m256i m0, m1, m2, m3, m4, m5;
    unsigned char* p_src = src_y;
    unsigned char* p_dst_y = dst[0];
    unsigned char* p_dst_u = dst[1];
    unsigned char* p_dst_v = dst[2];
    int i, j, j2;
    int width64 = (width >> 5) << 6;

    width <<= 1;    // width for byte
    for (i = 0; i < height; i++) {
        for (j = 0; j < width64; j += 64) {
            j2 = j >> 1;
            m0 = _mm256_loadu_si256((const __m256i*)(p_src + j));
            m1 = _mm256_loadu_si256((const __m256i*)(p_src + j + 32));
            m2 = _mm256_packus_epi16(m0, m1);
            m2 = _mm256_permute4x64_epi64(m2, 0xd8);
            _mm256_storeu_si256((__m256i*)(p_dst_y + j2), m2);
        }
        for (; j < width; j += 2) {
            p_dst_y[j >> 1] = (unsigned char)((short*)p_src)[j >> 1];
        }
        p_src += src_stride;
        p_dst_y += dst_stride[0];
    }
    width >>= uv_shift;
    height >>= uv_shift;

    p_src = src_uv;
    width <<= 1;
    width64 = (width >> 6) << 6;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width64; j += 64) {
            j2 = j >> 1;
            m0 = _mm256_loadu_si256((const __m256i*)(p_src + j));
            m1 = _mm256_loadu_si256((const __m256i*)(p_src + j + 32));
            m2 = _mm256_and_si256(m0, uv_mask);        // U: 0-8 (32bit)
            m3 = _mm256_and_si256(m1, uv_mask);        // U: 8-16
            m4 = _mm256_and_si256(_mm256_srli_si256(m0, 2), uv_mask); // V: 0-8 (32bit)
            m5 = _mm256_and_si256(_mm256_srli_si256(m1, 2), uv_mask); // V: 8-16
            m2 = _mm256_packus_epi32(m2, m3);
            m4 = _mm256_packus_epi32(m4, m5);
            m2 = _mm256_permute4x64_epi64(m2, 0xd8);
            m4 = _mm256_permute4x64_epi64(m4, 0xd8);
            m2 = _mm256_packus_epi16(m2, m4);          // U: 0-16; V: 0-16 (8bit)
            m2 = _mm256_permute4x64_epi64(m2, 0xd8);
            _mm_storeu_si128((__m128i*)(p_dst_u + (j2 >> 1)), _mm256_castsi256_si128(m2));
            _mm_storeu_si128((__m128i*)(p_dst_v + (j2 >> 1)), _mm256_extracti128_si256(m2, 1));
        }
        for (; j < width; j += 4) {
            j2 = j >> 1;    // for short
            p_dst_u[j2 >> 1] = (unsigned char)((short*)p_src)[j2];
            p_dst_v[j2 >> 1] = (unsigned char)((short*)p_src)[j2 + 1];
        }
        p_src += src_stridec;
        p_dst_u += dst_stride[1];
        p_dst_v += dst_stride[2];
    }
}
