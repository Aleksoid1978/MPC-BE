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
void uavs3d_avg_pel_w4_sse(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    int i_dst3 = i_dst + i_dst2;
    int i_dst4 = i_dst << 2;
    __m128i S1, S2, D0;

    while (height) {
        S1 = _mm_load_si128((const __m128i*)(src1));
        S2 = _mm_load_si128((const __m128i*)(src2));

        D0 = _mm_avg_epu8(S1, S2);
        M32(dst         ) = _mm_extract_epi32(D0, 0);
        M32(dst + i_dst ) = _mm_extract_epi32(D0, 1);
        M32(dst + i_dst2) = _mm_extract_epi32(D0, 2);
        M32(dst + i_dst3) = _mm_extract_epi32(D0, 3);

        src1 += 16;
        src2 += 16;
        dst  += i_dst4;
        height -= 4;
    }
}

void uavs3d_avg_pel_w8_sse(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    int i_dst3 = i_dst + i_dst2;
    int i_dst4 = i_dst << 2;
    __m128i S1, S2, S3, S4, D0, D1;

    while (height) {
        S1 = _mm_load_si128((const __m128i*)(src1));
        S2 = _mm_load_si128((const __m128i*)(src1 + 16));
        S3 = _mm_load_si128((const __m128i*)(src2));
        S4 = _mm_load_si128((const __m128i*)(src2 + 16));

        D0 = _mm_avg_epu8(S1, S3);
        D1 = _mm_avg_epu8(S2, S4);
        _mm_storel_epi64((__m128i*)dst, D0);
        _mm_storel_epi64((__m128i*)(dst + i_dst), _mm_srli_si128(D0, 8));
        _mm_storel_epi64((__m128i*)(dst + i_dst2), D1);
        _mm_storel_epi64((__m128i*)(dst + i_dst3), _mm_srli_si128(D1, 8));

        src1 += 32;
        src2 += 32;
        dst  += i_dst4;
        height -= 4;
    }
}

void uavs3d_avg_pel_w16_sse(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    int i_dst3 = i_dst + i_dst2;
    int i_dst4 = i_dst << 2;
    __m128i S0, S1, S2, S3, S4, S5, S6, S7;
    __m128i D0, D1, D2, D3;

    while (height) {
        S0 = _mm_load_si128((const __m128i*)(src1));
        S1 = _mm_load_si128((const __m128i*)(src1 + 16));
        S2 = _mm_load_si128((const __m128i*)(src1 + 32));
        S3 = _mm_load_si128((const __m128i*)(src1 + 48));
        S4 = _mm_load_si128((const __m128i*)(src2));
        S5 = _mm_load_si128((const __m128i*)(src2 + 16));
        S6 = _mm_load_si128((const __m128i*)(src2 + 32));
        S7 = _mm_load_si128((const __m128i*)(src2 + 48));

        D0 = _mm_avg_epu8(S0, S4);
        D1 = _mm_avg_epu8(S1, S5);
        D2 = _mm_avg_epu8(S2, S6);
        D3 = _mm_avg_epu8(S3, S7);
        _mm_storeu_si128((__m128i*)(dst), D0);
        _mm_storeu_si128((__m128i*)(dst + i_dst), D1);
        _mm_storeu_si128((__m128i*)(dst + i_dst2), D2);
        _mm_storeu_si128((__m128i*)(dst + i_dst3), D3);

        src1 += 64;
        src2 += 64;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_avg_pel_w32_sse(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    __m128i S0, S1, S2, S3, S4, S5, S6, S7;
    __m128i D0, D1, D2, D3;

    while (height) {
        S0 = _mm_load_si128((const __m128i*)(src1));
        S1 = _mm_load_si128((const __m128i*)(src1 + 16));
        S2 = _mm_load_si128((const __m128i*)(src1 + 32));
        S3 = _mm_load_si128((const __m128i*)(src1 + 48));
        S4 = _mm_load_si128((const __m128i*)(src2));
        S5 = _mm_load_si128((const __m128i*)(src2 + 16));
        S6 = _mm_load_si128((const __m128i*)(src2 + 32));
        S7 = _mm_load_si128((const __m128i*)(src2 + 48));

        D0 = _mm_avg_epu8(S0, S4);
        D1 = _mm_avg_epu8(S1, S5);
        D2 = _mm_avg_epu8(S2, S6);
        D3 = _mm_avg_epu8(S3, S7);
        _mm_store_si128((__m128i*)(dst), D0);
        _mm_store_si128((__m128i*)(dst + 16), D1);
        _mm_store_si128((__m128i*)(dst + i_dst), D2);
        _mm_store_si128((__m128i*)(dst + i_dst + 16), D3);

        src1 += 64;
        src2 += 64;
        dst += i_dst2;
        height -= 2;
    }
}

void uavs3d_avg_pel_w32x_sse(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int j;
    __m128i S0, S1, S2, S3, D0, D1;

    while (height--) {
        for (j = 0; j < width; j += 32) {
            S0 = _mm_load_si128((const __m128i*)(src1 + j));
            S1 = _mm_load_si128((const __m128i*)(src1 + j + 16));
            S2 = _mm_load_si128((const __m128i*)(src2 + j));
            S3 = _mm_load_si128((const __m128i*)(src2 + j + 16));
            D0 = _mm_avg_epu8(S0, S2);
            D1 = _mm_avg_epu8(S1, S3);
            _mm_store_si128((__m128i*)(dst + j), D0);
            _mm_store_si128((__m128i*)(dst + j + 16), D1);
        }

        src1 += width;
        src2 += width;
        dst += i_dst;
    }

}

void uavs3d_padding_rows_luma_sse(pel *src, int i_src, int width, int height, int start, int rows, int padh, int padv)
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
        __m128i Val1, Val2;
        uavs3d_prefetch(p + i_src, _MM_HINT_NTA);
        uavs3d_prefetch(p + i_src + width - 1, _MM_HINT_NTA);
        Val1 = _mm_set1_epi8((char)p[0]);
        Val2 = _mm_set1_epi8((char)p[width - 1]);
        p1 = p - padh;
        p2 = p + width;
        for (j = 0; j < padh - 15; j += 16) {
            _mm_storeu_si128((__m128i*)(p1 + j), Val1);
            _mm_storeu_si128((__m128i*)(p2 + j), Val2);
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

void uavs3d_padding_rows_chroma_sse(pel *src, int i_src, int width, int height, int start, int rows, int padh, int padv)
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
        __m128i Val1, Val2;

        uavs3d_prefetch(p + i_src, _MM_HINT_NTA);
        uavs3d_prefetch(p + i_src + width - 2, _MM_HINT_NTA);
        Val1 = _mm_set1_epi16(((short*)p)[0]);
        Val2 = _mm_set1_epi16(((short*)p)[width2 - 1]);
        p1 = p - padh;
        p2 = p + width;
        for (j = 0; j < padh - 15; j += 16) {
            _mm_storeu_si128((__m128i*)(p1 + j), Val1);
            _mm_storeu_si128((__m128i*)(p2 + j), Val2);
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

void uavs3d_recon_luma_w4_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    int i_rec2  = i_rec << 1;
    int i_pred2 = i_pred << 1;

    if (cbf == 0) {
        for (i = 0; i < height; i += 2) {
            CP32(rec, pred);
            CP32(rec + i_rec, pred + i_pred);
            rec  += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        int i_pred3 = i_pred + i_pred2;
        int i_pred4 = i_pred << 2;
        int i_rec3  = i_rec + i_rec2;
        int i_rec4  = i_rec << 2;
        __m128i p0, p1, p2, r0, r1;
        __m128i zero = _mm_setzero_si128();
        for (i = 0; i < height; i += 4) {
            p0 = _mm_set_epi32(*((s32*)(pred + i_pred3)), *((s32*)(pred + i_pred2)), *((s32*)(pred + i_pred)), *((s32*)(pred)));
            r0 = _mm_load_si128((const __m128i*)(resi));
            r1 = _mm_load_si128((const __m128i*)(resi + 8));
            p1 = _mm_unpacklo_epi8(p0, zero);
            p2 = _mm_unpackhi_epi8(p0, zero);

            p1 = _mm_adds_epi16(p1, r0);
            p2 = _mm_adds_epi16(p2, r1);
            p1 = _mm_packus_epi16(p1, p2);

            M32(rec         ) = _mm_extract_epi32(p1, 0);
            M32(rec + i_rec ) = _mm_extract_epi32(p1, 1);
            M32(rec + i_rec2) = _mm_extract_epi32(p1, 2);
            M32(rec + i_rec3) = _mm_extract_epi32(p1, 3);

            pred += i_pred4;
            rec  += i_rec4;
            resi += 16;
        }
    }
}

void uavs3d_recon_luma_w8_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    int i_rec2  = i_rec << 1;
    int i_pred2 = i_pred << 1;

    if (cbf == 0) {
        for (i = 0; i < height; i += 2) {
            CP64(rec, pred);
            CP64(rec + i_rec, pred + i_pred);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m128i p0, p1, r0, r1;
        __m128i zero = _mm_setzero_si128();
        for (i = 0; i < height; i += 2) {
            p0 = _mm_loadl_epi64((const __m128i*)(pred));
            p1 = _mm_loadl_epi64((const __m128i*)(pred + i_pred));
            r0 = _mm_load_si128((const __m128i*)(resi));
            r1 = _mm_load_si128((const __m128i*)(resi + 8));
            p0 = _mm_unpacklo_epi8(p0, zero);
            p1 = _mm_unpacklo_epi8(p1, zero);

            p0 = _mm_adds_epi16(p0, r0);
            p1 = _mm_adds_epi16(p1, r1);
            p0 = _mm_packus_epi16(p0, p1);

            _mm_storel_epi64((__m128i*)(rec), p0);
            _mm_storel_epi64((__m128i*)(rec + i_rec), _mm_srli_si128(p0, 8));

            pred += i_pred2;
            rec  += i_rec2;
            resi += 16;
        }
    }
}

void uavs3d_recon_luma_w16_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    int i_rec2 = i_rec << 1;
    int i_pred2 = i_pred << 1;

    if (cbf == 0) {
        __m128i p0, p1;
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + i_pred));

            _mm_storeu_si128((__m128i*)(rec), p0);
            _mm_storeu_si128((__m128i*)(rec + i_rec), p1);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m128i zero = _mm_setzero_si128();
        __m128i p0, p1, p2, p3, r0, r1, r2, r3;
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + i_pred));
            r0 = _mm_load_si128((const __m128i*)(resi));
            r1 = _mm_load_si128((const __m128i*)(resi + 8));
            r2 = _mm_load_si128((const __m128i*)(resi + 16));
            r3 = _mm_load_si128((const __m128i*)(resi + 24));
            p2 = _mm_unpacklo_epi8(p1, zero);
            p3 = _mm_unpackhi_epi8(p1, zero);
            p1 = _mm_unpackhi_epi8(p0, zero);
            p0 = _mm_unpacklo_epi8(p0, zero);

            p0 = _mm_adds_epi16(p0, r0);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p3 = _mm_adds_epi16(p3, r3);
            p0 = _mm_packus_epi16(p0, p1);
            p2 = _mm_packus_epi16(p2, p3);

            _mm_storeu_si128((__m128i*)(rec), p0);
            _mm_storeu_si128((__m128i*)(rec + i_rec), p2);

            pred += i_pred2;
            rec += i_rec2;
            resi += 32;
        }
    }
}

void uavs3d_recon_luma_w32_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    if (cbf == 0) {
        __m128i p0, p1, p2, p3;
        int i_rec2 = i_rec << 1;
        int i_pred2 = i_pred << 1;

        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 16));
            p2 = _mm_load_si128((const __m128i*)(pred + i_pred));
            p3 = _mm_load_si128((const __m128i*)(pred + i_pred + 16));

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 16), p1);
            _mm_store_si128((__m128i*)(rec + i_rec), p2);
            _mm_store_si128((__m128i*)(rec + i_rec + 16), p3);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m128i zero = _mm_setzero_si128();
        __m128i p0, p1, p2, p3, r0, r1, r2, r3;
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 16));
            r0 = _mm_load_si128((const __m128i*)(resi));
            r1 = _mm_load_si128((const __m128i*)(resi + 8));
            r2 = _mm_load_si128((const __m128i*)(resi + 16));
            r3 = _mm_load_si128((const __m128i*)(resi + 24));
            p2 = _mm_unpacklo_epi8(p1, zero);
            p3 = _mm_unpackhi_epi8(p1, zero);
            p1 = _mm_unpackhi_epi8(p0, zero);
            p0 = _mm_unpacklo_epi8(p0, zero);

            p0 = _mm_adds_epi16(p0, r0);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p3 = _mm_adds_epi16(p3, r3);
            p0 = _mm_packus_epi16(p0, p1);
            p2 = _mm_packus_epi16(p2, p3);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 16), p2);

            pred += i_pred;
            rec += i_rec;
            resi += 32;
        }
    }
}

void uavs3d_recon_luma_w64_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    if (cbf == 0) {
        __m128i p0, p1, p2, p3;
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 16));
            p2 = _mm_load_si128((const __m128i*)(pred + 32));
            p3 = _mm_load_si128((const __m128i*)(pred + 48));

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 16), p1);
            _mm_store_si128((__m128i*)(rec + 32), p2);
            _mm_store_si128((__m128i*)(rec + 48), p3);
            rec += i_rec;
            pred += i_pred;
        }
    }
    else {
        __m128i zero = _mm_setzero_si128();
        __m128i p0, p1, p2, p3, p4, p5, p6, p7;
        __m128i r0, r1, r2, r3, r4, r5, r6, r7;
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 16));
            p2 = _mm_load_si128((const __m128i*)(pred + 32));
            p3 = _mm_load_si128((const __m128i*)(pred + 48));
            r0 = _mm_load_si128((const __m128i*)(resi));
            r1 = _mm_load_si128((const __m128i*)(resi + 8));
            r2 = _mm_load_si128((const __m128i*)(resi + 16));
            r3 = _mm_load_si128((const __m128i*)(resi + 24));
            r4 = _mm_load_si128((const __m128i*)(resi + 32));
            r5 = _mm_load_si128((const __m128i*)(resi + 40));
            r6 = _mm_load_si128((const __m128i*)(resi + 48));
            r7 = _mm_load_si128((const __m128i*)(resi + 56));
            p4 = _mm_unpacklo_epi8(p2, zero);
            p5 = _mm_unpackhi_epi8(p2, zero);
            p6 = _mm_unpacklo_epi8(p3, zero);
            p7 = _mm_unpackhi_epi8(p3, zero);
            p2 = _mm_unpacklo_epi8(p1, zero);
            p3 = _mm_unpackhi_epi8(p1, zero);
            p1 = _mm_unpackhi_epi8(p0, zero);
            p0 = _mm_unpacklo_epi8(p0, zero);

            p0 = _mm_adds_epi16(p0, r0);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p3 = _mm_adds_epi16(p3, r3);
            p4 = _mm_adds_epi16(p4, r4);
            p5 = _mm_adds_epi16(p5, r5);
            p6 = _mm_adds_epi16(p6, r6);
            p7 = _mm_adds_epi16(p7, r7);

            p0 = _mm_packus_epi16(p0, p1);
            p2 = _mm_packus_epi16(p2, p3);
            p4 = _mm_packus_epi16(p4, p5);
            p6 = _mm_packus_epi16(p6, p7);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 16), p2);
            _mm_store_si128((__m128i*)(rec + 32), p4);
            _mm_store_si128((__m128i*)(rec + 48), p6);

            pred += i_pred;
            rec += i_rec;
            resi += 64;
        }
    }
}

void uavs3d_recon_chroma_w4_sse(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i;
    int i_rec2 = i_rec << 1;
    __m128i p0, p1, p2, r0, r1, r2;
    __m128i zero = _mm_setzero_si128();

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            r1 = _mm_load_si128((const __m128i*)(resi_v));
            p1 = _mm_unpacklo_epi8(p0, zero);
            p2 = _mm_unpackhi_epi8(p0, zero);
            r2 = _mm_unpackhi_epi16(r0, r1);    // UV interlaced
            r1 = _mm_unpacklo_epi16(r0, r1);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p1 = _mm_packus_epi16(p1, p2);

            _mm_storel_epi64((__m128i*)(rec), p1);
            _mm_storel_epi64((__m128i*)(rec + i_rec), _mm_srli_si128(p1, 8));

            pred += 16;
            resi_u += 8;
            resi_v += 8;
            rec += i_rec2;
        }
    } else if (cbf[0]) {
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            p1 = _mm_unpacklo_epi8(p0, zero);
            p2 = _mm_unpackhi_epi8(p0, zero);
            r2 = _mm_unpackhi_epi16(r0, zero);    // UV interlaced
            r1 = _mm_unpacklo_epi16(r0, zero);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p1 = _mm_packus_epi16(p1, p2);

            _mm_storel_epi64((__m128i*)(rec), p1);
            _mm_storel_epi64((__m128i*)(rec + i_rec), _mm_srli_si128(p1, 8));

            pred += 16;
            resi_u += 8;
            rec += i_rec2;
        }
    } else {
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            r0 = _mm_load_si128((const __m128i*)(resi_v));
            p1 = _mm_unpacklo_epi8(p0, zero);
            p2 = _mm_unpackhi_epi8(p0, zero);
            r2 = _mm_unpackhi_epi16(zero, r0);    // UV interlaced
            r1 = _mm_unpacklo_epi16(zero, r0);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p1 = _mm_packus_epi16(p1, p2);

            _mm_storel_epi64((__m128i*)(rec), p1);
            _mm_storel_epi64((__m128i*)(rec + i_rec), _mm_srli_si128(p1, 8));

            pred += 16;
            resi_v += 8;
            rec += i_rec2;
        }
    }
}

void uavs3d_recon_chroma_w8_sse(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i;
    __m128i p0, p1, p2, r0, r1, r2;
    __m128i zero = _mm_setzero_si128();

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            r1 = _mm_load_si128((const __m128i*)(resi_v));
            p1 = _mm_unpacklo_epi8(p0, zero);
            p2 = _mm_unpackhi_epi8(p0, zero);
            r2 = _mm_unpackhi_epi16(r0, r1);    // UV interlaced
            r1 = _mm_unpacklo_epi16(r0, r1);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p1 = _mm_packus_epi16(p1, p2);

            _mm_storeu_si128((__m128i*)(rec), p1);

            pred += 16;
            resi_u += 8;
            resi_v += 8;
            rec += i_rec;
        }
    }
    else if (cbf[0]) {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            p1 = _mm_unpacklo_epi8(p0, zero);
            p2 = _mm_unpackhi_epi8(p0, zero);
            r2 = _mm_unpackhi_epi16(r0, zero);    // UV interlaced
            r1 = _mm_unpacklo_epi16(r0, zero);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p1 = _mm_packus_epi16(p1, p2);

            _mm_storeu_si128((__m128i*)(rec), p1);

            pred += 16;
            resi_u += 8;
            rec += i_rec;
        }
    }
    else {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            r0 = _mm_load_si128((const __m128i*)(resi_v));
            p1 = _mm_unpacklo_epi8(p0, zero);
            p2 = _mm_unpackhi_epi8(p0, zero);
            r2 = _mm_unpackhi_epi16(zero, r0);    // UV interlaced
            r1 = _mm_unpacklo_epi16(zero, r0);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p1 = _mm_packus_epi16(p1, p2);

            _mm_storeu_si128((__m128i*)(rec), p1);

            pred += 16;
            resi_v += 8;
            rec += i_rec;
        }
    }
}

void uavs3d_recon_chroma_w16_sse(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i;
    __m128i p0, p1, p2, p3, r0, r1, r2, r3, r4, r5, r6, r7;
    __m128i zero = _mm_setzero_si128();

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 16));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            r1 = _mm_load_si128((const __m128i*)(resi_u + 8));
            r2 = _mm_load_si128((const __m128i*)(resi_v));
            r3 = _mm_load_si128((const __m128i*)(resi_v + 8));
            p2 = _mm_unpacklo_epi8(p1, zero);
            p3 = _mm_unpackhi_epi8(p1, zero);
            p1 = _mm_unpackhi_epi8(p0, zero);
            p0 = _mm_unpacklo_epi8(p0, zero);
            r4 = _mm_unpacklo_epi16(r0, r2);    // UV interlaced
            r5 = _mm_unpackhi_epi16(r0, r2);
            r6 = _mm_unpacklo_epi16(r1, r3);
            r7 = _mm_unpackhi_epi16(r1, r3);   
            p0 = _mm_adds_epi16(p0, r4);
            p1 = _mm_adds_epi16(p1, r5);
            p2 = _mm_adds_epi16(p2, r6);
            p3 = _mm_adds_epi16(p3, r7);

            p0 = _mm_packus_epi16(p0, p1);
            p2 = _mm_packus_epi16(p2, p3);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 16), p2);

            pred += 32;
            resi_u += 16;
            resi_v += 16;
            rec += i_rec;
        }
    }
    else if (cbf[0]) {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 16));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            r1 = _mm_load_si128((const __m128i*)(resi_u + 8));
            p2 = _mm_unpacklo_epi8(p1, zero);
            p3 = _mm_unpackhi_epi8(p1, zero);
            p1 = _mm_unpackhi_epi8(p0, zero);
            p0 = _mm_unpacklo_epi8(p0, zero);
            r4 = _mm_unpacklo_epi16(r0, zero);    // UV interlaced
            r5 = _mm_unpackhi_epi16(r0, zero);
            r6 = _mm_unpacklo_epi16(r1, zero);
            r7 = _mm_unpackhi_epi16(r1, zero);
            p0 = _mm_adds_epi16(p0, r4);
            p1 = _mm_adds_epi16(p1, r5);
            p2 = _mm_adds_epi16(p2, r6);
            p3 = _mm_adds_epi16(p3, r7);

            p0 = _mm_packus_epi16(p0, p1);
            p2 = _mm_packus_epi16(p2, p3);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 16), p2);

            pred += 32;
            resi_u += 16;
            rec += i_rec;
        }
    }
    else {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 16));
            r2 = _mm_load_si128((const __m128i*)(resi_v));
            r3 = _mm_load_si128((const __m128i*)(resi_v + 8));
            p2 = _mm_unpacklo_epi8(p1, zero);
            p3 = _mm_unpackhi_epi8(p1, zero);
            p1 = _mm_unpackhi_epi8(p0, zero);
            p0 = _mm_unpacklo_epi8(p0, zero);
            r4 = _mm_unpacklo_epi16(zero, r2);    // UV interlaced
            r5 = _mm_unpackhi_epi16(zero, r2);
            r6 = _mm_unpacklo_epi16(zero, r3);
            r7 = _mm_unpackhi_epi16(zero, r3);
            p0 = _mm_adds_epi16(p0, r4);
            p1 = _mm_adds_epi16(p1, r5);
            p2 = _mm_adds_epi16(p2, r6);
            p3 = _mm_adds_epi16(p3, r7);

            p0 = _mm_packus_epi16(p0, p1);
            p2 = _mm_packus_epi16(p2, p3);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 16), p2);

            pred += 32;
            resi_v += 16;
            rec += i_rec;
        }
    }
}

void uavs3d_recon_chroma_w16x_sse(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i, j;
    int width2 = width << 1;
    __m128i p0, p1, p2, p3, r0, r1, r2, r3, r4, r5, r6, r7;
    __m128i zero = _mm_setzero_si128();

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 16) {
                int j2 = j << 1;
                p0 = _mm_load_si128((const __m128i*)(pred + j2));
                p1 = _mm_load_si128((const __m128i*)(pred + j2 + 16));
                r0 = _mm_load_si128((const __m128i*)(resi_u + j));
                r1 = _mm_load_si128((const __m128i*)(resi_u + j + 8));
                r2 = _mm_load_si128((const __m128i*)(resi_v + j));
                r3 = _mm_load_si128((const __m128i*)(resi_v + j + 8));
                p2 = _mm_unpacklo_epi8(p1, zero);
                p3 = _mm_unpackhi_epi8(p1, zero);
                p1 = _mm_unpackhi_epi8(p0, zero);
                p0 = _mm_unpacklo_epi8(p0, zero);
                r4 = _mm_unpacklo_epi16(r0, r2);    // UV interlaced
                r5 = _mm_unpackhi_epi16(r0, r2);
                r6 = _mm_unpacklo_epi16(r1, r3);
                r7 = _mm_unpackhi_epi16(r1, r3);
                p0 = _mm_adds_epi16(p0, r4);
                p1 = _mm_adds_epi16(p1, r5);
                p2 = _mm_adds_epi16(p2, r6);
                p3 = _mm_adds_epi16(p3, r7);

                p0 = _mm_packus_epi16(p0, p1);
                p2 = _mm_packus_epi16(p2, p3);

                _mm_store_si128((__m128i*)(rec + j2), p0);
                _mm_store_si128((__m128i*)(rec + j2 + 16), p2);
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
                p0 = _mm_load_si128((const __m128i*)(pred + j2));
                p1 = _mm_load_si128((const __m128i*)(pred + j2 + 16));
                r0 = _mm_load_si128((const __m128i*)(resi_u + j));
                r1 = _mm_load_si128((const __m128i*)(resi_u + j + 8));
                p2 = _mm_unpacklo_epi8(p1, zero);
                p3 = _mm_unpackhi_epi8(p1, zero);
                p1 = _mm_unpackhi_epi8(p0, zero);
                p0 = _mm_unpacklo_epi8(p0, zero);
                r4 = _mm_unpacklo_epi16(r0, zero);    // UV interlaced
                r5 = _mm_unpackhi_epi16(r0, zero);
                r6 = _mm_unpacklo_epi16(r1, zero);
                r7 = _mm_unpackhi_epi16(r1, zero);
                p0 = _mm_adds_epi16(p0, r4);
                p1 = _mm_adds_epi16(p1, r5);
                p2 = _mm_adds_epi16(p2, r6);
                p3 = _mm_adds_epi16(p3, r7);

                p0 = _mm_packus_epi16(p0, p1);
                p2 = _mm_packus_epi16(p2, p3);

                _mm_store_si128((__m128i*)(rec + j2), p0);
                _mm_store_si128((__m128i*)(rec + j2 + 16), p2);
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
                p0 = _mm_load_si128((const __m128i*)(pred + j2));
                p1 = _mm_load_si128((const __m128i*)(pred + j2 + 16));
                r2 = _mm_load_si128((const __m128i*)(resi_v + j));
                r3 = _mm_load_si128((const __m128i*)(resi_v + j + 8));
                p2 = _mm_unpacklo_epi8(p1, zero);
                p3 = _mm_unpackhi_epi8(p1, zero);
                p1 = _mm_unpackhi_epi8(p0, zero);
                p0 = _mm_unpacklo_epi8(p0, zero);
                r4 = _mm_unpacklo_epi16(zero, r2);    // UV interlaced
                r5 = _mm_unpackhi_epi16(zero, r2);
                r6 = _mm_unpacklo_epi16(zero, r3);
                r7 = _mm_unpackhi_epi16(zero, r3);
                p0 = _mm_adds_epi16(p0, r4);
                p1 = _mm_adds_epi16(p1, r5);
                p2 = _mm_adds_epi16(p2, r6);
                p3 = _mm_adds_epi16(p3, r7);

                p0 = _mm_packus_epi16(p0, p1);
                p2 = _mm_packus_epi16(p2, p3);

                _mm_store_si128((__m128i*)(rec + j2), p0);
                _mm_store_si128((__m128i*)(rec + j2 + 16), p2);
            }
            pred += width2;
            resi_v += width;
            rec += i_rec;
        }
    }
}

#else // 10bit
void uavs3d_avg_pel_w4_sse(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    int i_dst3 = i_dst + i_dst2;
    int i_dst4 = i_dst << 2;
    __m128i S1, S2, S3, S4, D0, D1;

    while (height) {
        S1 = _mm_load_si128((const __m128i*)(src1));
        S2 = _mm_load_si128((const __m128i*)(src1 + 8));
        S3 = _mm_load_si128((const __m128i*)(src2));
        S4 = _mm_load_si128((const __m128i*)(src2 + 8));

        D0 = _mm_avg_epu16(S1, S3);
        D1 = _mm_avg_epu16(S2, S4);
        _mm_storel_epi64((__m128i*)dst, D0);
        _mm_storel_epi64((__m128i*)(dst + i_dst), _mm_srli_si128(D0, 8));
        _mm_storel_epi64((__m128i*)(dst + i_dst2), D1);
        _mm_storel_epi64((__m128i*)(dst + i_dst3), _mm_srli_si128(D1, 8));

        src1 += 16;
        src2 += 16;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_avg_pel_w8_sse(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    int i_dst3 = i_dst + i_dst2;
    int i_dst4 = i_dst << 2;
    __m128i S0, S1, S2, S3, S4, S5, S6, S7;
    __m128i D0, D1, D2, D3;

    while (height) {
        S0 = _mm_load_si128((const __m128i*)(src1));
        S1 = _mm_load_si128((const __m128i*)(src1 + 8));
        S2 = _mm_load_si128((const __m128i*)(src1 + 16));
        S3 = _mm_load_si128((const __m128i*)(src1 + 24));
        S4 = _mm_load_si128((const __m128i*)(src2));
        S5 = _mm_load_si128((const __m128i*)(src2 + 8));
        S6 = _mm_load_si128((const __m128i*)(src2 + 16));
        S7 = _mm_load_si128((const __m128i*)(src2 + 24));

        D0 = _mm_avg_epu16(S0, S4);
        D1 = _mm_avg_epu16(S1, S5);
        D2 = _mm_avg_epu16(S2, S6);
        D3 = _mm_avg_epu16(S3, S7);
        _mm_storeu_si128((__m128i*)(dst), D0);
        _mm_storeu_si128((__m128i*)(dst + i_dst), D1);
        _mm_storeu_si128((__m128i*)(dst + i_dst2), D2);
        _mm_storeu_si128((__m128i*)(dst + i_dst3), D3);

        src1 += 32;
        src2 += 32;
        dst += i_dst4;
        height -= 4;
    }
}

void uavs3d_avg_pel_w16_sse(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int i_dst2 = i_dst << 1;
    __m128i S0, S1, S2, S3, S4, S5, S6, S7;
    __m128i D0, D1, D2, D3;

    while (height) {
        S0 = _mm_load_si128((const __m128i*)(src1));
        S1 = _mm_load_si128((const __m128i*)(src1 + 8));
        S2 = _mm_load_si128((const __m128i*)(src1 + 16));
        S3 = _mm_load_si128((const __m128i*)(src1 + 24));
        S4 = _mm_load_si128((const __m128i*)(src2));
        S5 = _mm_load_si128((const __m128i*)(src2 + 8));
        S6 = _mm_load_si128((const __m128i*)(src2 + 16));
        S7 = _mm_load_si128((const __m128i*)(src2 + 24));

        D0 = _mm_avg_epu16(S0, S4);
        D1 = _mm_avg_epu16(S1, S5);
        D2 = _mm_avg_epu16(S2, S6);
        D3 = _mm_avg_epu16(S3, S7);
        _mm_store_si128((__m128i*)(dst), D0);
        _mm_store_si128((__m128i*)(dst + 8), D1);
        _mm_store_si128((__m128i*)(dst + i_dst), D2);
        _mm_store_si128((__m128i*)(dst + i_dst + 8), D3);

        src1 += 32;
        src2 += 32;
        dst += i_dst2;
        height -= 2;
    }
}

void uavs3d_avg_pel_w16x_sse(pel *dst, int i_dst, pel *src1, pel *src2, int width, int height)
{
    int j;
    __m128i S0, S1, S2, S3, D0, D1;

    while (height--) {
        for (j = 0; j < width; j += 16) {
            S0 = _mm_load_si128((const __m128i*)(src1 + j));
            S1 = _mm_load_si128((const __m128i*)(src1 + j + 8));
            S2 = _mm_load_si128((const __m128i*)(src2 + j));
            S3 = _mm_load_si128((const __m128i*)(src2 + j + 8));
            D0 = _mm_avg_epu16(S0, S2);
            D1 = _mm_avg_epu16(S1, S3);
            _mm_store_si128((__m128i*)(dst + j), D0);
            _mm_store_si128((__m128i*)(dst + j + 8), D1);
        }

        src1 += width;
        src2 += width;
        dst += i_dst;
    }

}

void uavs3d_padding_rows_luma_sse(pel *src, int i_src, int width, int height, int start, int rows, int padh, int padv)
{
    int i, j;
    pel *p, *p1, *p2;
    start = max(start, 0);

    uavs3d_assert(padh % 8 == 0);

    if (start + rows > height) {
        rows = height - start;
    }

    p = src + start * i_src;

    // left & right
    for (i = 0; i < rows; i++) {
        __m128i Val1, Val2;
        uavs3d_prefetch(p + i_src, _MM_HINT_NTA);
        uavs3d_prefetch(p + i_src + width - 1, _MM_HINT_NTA);
        Val1 = _mm_set1_epi16((pel)p[0]);
        Val2 = _mm_set1_epi16((pel)p[width - 1]);
        p1 = p - padh;
        p2 = p + width;
        for (j = 0; j < padh - 7; j += 8) {
            _mm_storeu_si128((__m128i*)(p1 + j), Val1);
            _mm_storeu_si128((__m128i*)(p2 + j), Val2);
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

void uavs3d_padding_rows_chroma_sse(pel *src, int i_src, int width, int height, int start, int rows, int padh, int padv)
{
    int i, j;
    pel *p, *p1, *p2;
    int width2 = width >> 1;
    start = max(start, 0);

    uavs3d_assert(padh % 8 == 0);

    if (start + rows > height) {
        rows = height - start;
    }

    p = src + start * i_src;

    // left & right
    for (i = 0; i < rows; i++) {
        __m128i Val1, Val2;

        uavs3d_prefetch(p + i_src, _MM_HINT_NTA);
        uavs3d_prefetch(p + i_src + width - 2, _MM_HINT_NTA);
        Val1 = _mm_set1_epi32(((u32*)p)[0]);
        Val2 = _mm_set1_epi32(((u32*)p)[width2 - 1]);
        p1 = p - padh;
        p2 = p + width;
        for (j = 0; j < padh - 7; j += 8) {
            _mm_storeu_si128((__m128i*)(p1 + j), Val1);
            _mm_storeu_si128((__m128i*)(p2 + j), Val2);
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

void uavs3d_recon_luma_w4_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    int i_rec2 = i_rec << 1;
    int i_pred2 = i_pred << 1;

    if (cbf == 0) {
        for (i = 0; i < height; i += 2) {
            CP64(rec, pred);
            CP64(rec + i_rec, pred + i_pred);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m128i p0, p1, r0;
        __m128i min_pel = _mm_setzero_si128();
        __m128i max_pel = _mm_set1_epi16((1 << bit_depth) - 1);
        for (i = 0; i < height; i += 2) {
            p0 = _mm_loadl_epi64((const __m128i*)(pred));
            p1 = _mm_loadl_epi64((const __m128i*)(pred + i_pred));
            r0 = _mm_load_si128((const __m128i*)(resi));
            
            p0 = _mm_unpacklo_epi64(p0, p1);
            p0 = _mm_adds_epi16(p0, r0);
            
            p0 = _mm_max_epi16(p0, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);

            _mm_storel_epi64((__m128i*)(rec), p0);
            _mm_storel_epi64((__m128i*)(rec + i_rec), _mm_srli_si128(p0, 8));

            pred += i_pred2;
            rec += i_rec2;
            resi += 8;
        }
    }
}

void uavs3d_recon_luma_w8_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    int i_rec2 = i_rec << 1;
    int i_pred2 = i_pred << 1;

    if (cbf == 0) {
        __m128i p0, p1;
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + i_pred));

            _mm_storeu_si128((__m128i*)(rec), p0);
            _mm_storeu_si128((__m128i*)(rec + i_rec), p1);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m128i p0, p1, r0, r1;
        __m128i min_pel = _mm_setzero_si128();
        __m128i max_pel = _mm_set1_epi16((1 << bit_depth) - 1);
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + i_pred));
            r0 = _mm_load_si128((const __m128i*)(resi));
            r1 = _mm_load_si128((const __m128i*)(resi + 8));
            
            p0 = _mm_adds_epi16(p0, r0);
            p1 = _mm_adds_epi16(p1, r1);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);

            _mm_storeu_si128((__m128i*)(rec), p0);
            _mm_storeu_si128((__m128i*)(rec + i_rec), p1);

            pred += i_pred2;
            rec += i_rec2;
            resi += 16;
        }
    }
}

void uavs3d_recon_luma_w16_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    int i_rec2 = i_rec << 1;
    int i_pred2 = i_pred << 1;

    if (cbf == 0) {
        __m128i p0, p1, p2, p3;
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            p2 = _mm_load_si128((const __m128i*)(pred + i_pred));
            p3 = _mm_load_si128((const __m128i*)(pred + i_pred + 8));

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);
            _mm_store_si128((__m128i*)(rec + i_rec), p2);
            _mm_store_si128((__m128i*)(rec + i_rec + 8), p3);
            rec += i_rec2;
            pred += i_pred2;
        }
    }
    else {
        __m128i p0, p1, p2, p3, r0, r1, r2, r3;
        __m128i min_pel = _mm_setzero_si128();
        __m128i max_pel = _mm_set1_epi16((1 << bit_depth) - 1);
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            p2 = _mm_load_si128((const __m128i*)(pred + i_pred));
            p3 = _mm_load_si128((const __m128i*)(pred + i_pred + 8));
            r0 = _mm_load_si128((const __m128i*)(resi));
            r1 = _mm_load_si128((const __m128i*)(resi + 8));
            r2 = _mm_load_si128((const __m128i*)(resi + 16));
            r3 = _mm_load_si128((const __m128i*)(resi + 24));
            
            p0 = _mm_adds_epi16(p0, r0);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p3 = _mm_adds_epi16(p3, r3);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p2 = _mm_max_epi16(p2, min_pel);
            p3 = _mm_max_epi16(p3, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);
            p2 = _mm_min_epi16(p2, max_pel);
            p3 = _mm_min_epi16(p3, max_pel);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);
            _mm_store_si128((__m128i*)(rec + i_rec), p2);
            _mm_store_si128((__m128i*)(rec + i_rec + 8), p3);

            pred += i_pred2;
            rec += i_rec2;
            resi += 32;
        }
    }
}

void uavs3d_recon_luma_w32_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    if (cbf == 0) {
        __m128i p0, p1, p2, p3;

        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            p2 = _mm_load_si128((const __m128i*)(pred + 16));
            p3 = _mm_load_si128((const __m128i*)(pred + 24));

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);
            _mm_store_si128((__m128i*)(rec + 16), p2);
            _mm_store_si128((__m128i*)(rec + 24), p3);
            rec += i_rec;
            pred += i_pred;
        }
    }
    else {
        __m128i min_pel = _mm_setzero_si128();
        __m128i max_pel = _mm_set1_epi16((1 << bit_depth) - 1);
        __m128i p0, p1, p2, p3, r0, r1, r2, r3;
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            p2 = _mm_load_si128((const __m128i*)(pred + 16));
            p3 = _mm_load_si128((const __m128i*)(pred + 24));
            r0 = _mm_load_si128((const __m128i*)(resi));
            r1 = _mm_load_si128((const __m128i*)(resi + 8));
            r2 = _mm_load_si128((const __m128i*)(resi + 16));
            r3 = _mm_load_si128((const __m128i*)(resi + 24));
            
            p0 = _mm_adds_epi16(p0, r0);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p3 = _mm_adds_epi16(p3, r3);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p2 = _mm_max_epi16(p2, min_pel);
            p3 = _mm_max_epi16(p3, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);
            p2 = _mm_min_epi16(p2, max_pel);
            p3 = _mm_min_epi16(p3, max_pel);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);
            _mm_store_si128((__m128i*)(rec + 16), p2);
            _mm_store_si128((__m128i*)(rec + 24), p3);

            pred += i_pred;
            rec += i_rec;
            resi += 32;
        }
    }
}

void uavs3d_recon_luma_w64_sse(s16 *resi, pel *pred, int i_pred, int width, int height, pel *rec, int i_rec, int cbf, int bit_depth)
{
    int i;
    if (cbf == 0) {
        __m128i p0, p1, p2, p3, p4, p5, p6, p7;
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            p2 = _mm_load_si128((const __m128i*)(pred + 16));
            p3 = _mm_load_si128((const __m128i*)(pred + 24));
            p4 = _mm_load_si128((const __m128i*)(pred + 32));
            p5 = _mm_load_si128((const __m128i*)(pred + 40));
            p6 = _mm_load_si128((const __m128i*)(pred + 48));
            p7 = _mm_load_si128((const __m128i*)(pred + 56));

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);
            _mm_store_si128((__m128i*)(rec + 16), p2);
            _mm_store_si128((__m128i*)(rec + 24), p3);
            _mm_store_si128((__m128i*)(rec + 32), p4);
            _mm_store_si128((__m128i*)(rec + 40), p5);
            _mm_store_si128((__m128i*)(rec + 48), p6);
            _mm_store_si128((__m128i*)(rec + 56), p7);
            rec += i_rec;
            pred += i_pred;
        }
    }
    else {
        __m128i min_pel = _mm_setzero_si128();
        __m128i max_pel = _mm_set1_epi16((1 << bit_depth) - 1);
        __m128i p0, p1, p2, p3, p4, p5, p6, p7;
        __m128i r0, r1, r2, r3, r4, r5, r6, r7;
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            p2 = _mm_load_si128((const __m128i*)(pred + 16));
            p3 = _mm_load_si128((const __m128i*)(pred + 24));
            p4 = _mm_load_si128((const __m128i*)(pred + 32));
            p5 = _mm_load_si128((const __m128i*)(pred + 40));
            p6 = _mm_load_si128((const __m128i*)(pred + 48));
            p7 = _mm_load_si128((const __m128i*)(pred + 56));

            r0 = _mm_loadu_si128((const __m128i*)(resi));
            r1 = _mm_loadu_si128((const __m128i*)(resi + 8));
            r2 = _mm_loadu_si128((const __m128i*)(resi + 16));
            r3 = _mm_loadu_si128((const __m128i*)(resi + 24));
            r4 = _mm_loadu_si128((const __m128i*)(resi + 32));
            r5 = _mm_loadu_si128((const __m128i*)(resi + 40));
            r6 = _mm_loadu_si128((const __m128i*)(resi + 48));
            r7 = _mm_loadu_si128((const __m128i*)(resi + 56));

            p0 = _mm_adds_epi16(p0, r0);
            p1 = _mm_adds_epi16(p1, r1);
            p2 = _mm_adds_epi16(p2, r2);
            p3 = _mm_adds_epi16(p3, r3);
            p4 = _mm_adds_epi16(p4, r4);
            p5 = _mm_adds_epi16(p5, r5);
            p6 = _mm_adds_epi16(p6, r6);
            p7 = _mm_adds_epi16(p7, r7);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p2 = _mm_max_epi16(p2, min_pel);
            p3 = _mm_max_epi16(p3, min_pel);
            p4 = _mm_max_epi16(p4, min_pel);
            p5 = _mm_max_epi16(p5, min_pel);
            p6 = _mm_max_epi16(p6, min_pel);
            p7 = _mm_max_epi16(p7, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);
            p2 = _mm_min_epi16(p2, max_pel);
            p3 = _mm_min_epi16(p3, max_pel);
            p4 = _mm_min_epi16(p4, max_pel);
            p5 = _mm_min_epi16(p5, max_pel);
            p6 = _mm_min_epi16(p6, max_pel);
            p7 = _mm_min_epi16(p7, max_pel);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);
            _mm_store_si128((__m128i*)(rec + 16), p2);
            _mm_store_si128((__m128i*)(rec + 24), p3);
            _mm_store_si128((__m128i*)(rec + 32), p4);
            _mm_store_si128((__m128i*)(rec + 40), p5);
            _mm_store_si128((__m128i*)(rec + 48), p6);
            _mm_store_si128((__m128i*)(rec + 56), p7);

            pred += i_pred;
            rec += i_rec;
            resi += 64;
        }
    }
}

void uavs3d_recon_chroma_w4_sse(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i;
    int i_rec2 = i_rec << 1;
    __m128i p0, p1, r0, r1, r2;
    __m128i min_pel = _mm_setzero_si128();
    __m128i max_pel = _mm_set1_epi16((1 << bit_depth) - 1);

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i += 2) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            r1 = _mm_load_si128((const __m128i*)(resi_v));
            r2 = _mm_unpackhi_epi16(r0, r1);    // UV interlaced
            r1 = _mm_unpacklo_epi16(r0, r1);
            p0 = _mm_adds_epi16(p0, r1);
            p1 = _mm_adds_epi16(p1, r2);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);

            _mm_storeu_si128((__m128i*)(rec), p0);
            _mm_storeu_si128((__m128i*)(rec + i_rec), p1);

            pred += 16;
            resi_u += 8;
            resi_v += 8;
            rec += i_rec2;
        }
    }
    else if (cbf[0]) {
        for (i = 0; i < height; i += 2) {
            p0 = _mm_loadu_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            r0 = _mm_loadu_si128((const __m128i*)(resi_u));
            r2 = _mm_unpackhi_epi16(r0, min_pel);    // UV interlaced
            r1 = _mm_unpacklo_epi16(r0, min_pel);
            p0 = _mm_adds_epi16(p0, r1);
            p1 = _mm_adds_epi16(p1, r2);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);

            _mm_storeu_si128((__m128i*)(rec), p0);
            _mm_storeu_si128((__m128i*)(rec + i_rec), p1);

            pred += 16;
            resi_u += 8;
            rec += i_rec2;
        }
    }
    else {
        for (i = 0; i < height; i += 2) {
            p0 = _mm_loadu_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            r0 = _mm_loadu_si128((const __m128i*)(resi_v));
            r2 = _mm_unpackhi_epi16(min_pel, r0);    // UV interlaced
            r1 = _mm_unpacklo_epi16(min_pel, r0);
            p0 = _mm_adds_epi16(p0, r1);
            p1 = _mm_adds_epi16(p1, r2);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);

            _mm_storeu_si128((__m128i*)(rec), p0);
            _mm_storeu_si128((__m128i*)(rec + i_rec), p1);

            pred += 16;
            resi_v += 8;
            rec += i_rec2;
        }
    }
}

void uavs3d_recon_chroma_w8_sse(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i;
    __m128i p0, p1, r0, r1, r2;
    __m128i min_pel = _mm_setzero_si128();
    __m128i max_pel = _mm_set1_epi16((1 << bit_depth) - 1);

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            r1 = _mm_load_si128((const __m128i*)(resi_v));
            r2 = _mm_unpackhi_epi16(r0, r1);    // UV interlaced
            r1 = _mm_unpacklo_epi16(r0, r1);
            p0 = _mm_adds_epi16(p0, r1);
            p1 = _mm_adds_epi16(p1, r2);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);

            pred += 16;
            resi_u += 8;
            resi_v += 8;
            rec += i_rec;
        }
    }
    else if (cbf[0]) {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            r2 = _mm_unpackhi_epi16(r0, min_pel);    // UV interlaced
            r1 = _mm_unpacklo_epi16(r0, min_pel);
            p0 = _mm_adds_epi16(p0, r1);
            p1 = _mm_adds_epi16(p1, r2);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);

            pred += 16;
            resi_u += 8;
            rec += i_rec;
        }
    }
    else {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            r1 = _mm_load_si128((const __m128i*)(resi_v));
            r2 = _mm_unpackhi_epi16(min_pel, r1);    // UV interlaced
            r1 = _mm_unpacklo_epi16(min_pel, r1);
            p0 = _mm_adds_epi16(p0, r1);
            p1 = _mm_adds_epi16(p1, r2);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);


            pred += 16;
            resi_v += 8;
            rec += i_rec;
        }
    }
}

void uavs3d_recon_chroma_w16_sse(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i;
    __m128i p0, p1, p2, p3, r0, r1, r2, r3, r4, r5, r6, r7;
    __m128i min_pel = _mm_setzero_si128();
    __m128i max_pel = _mm_set1_epi16((1 << bit_depth) - 1);

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            p2 = _mm_load_si128((const __m128i*)(pred + 16));
            p3 = _mm_load_si128((const __m128i*)(pred + 24));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            r1 = _mm_load_si128((const __m128i*)(resi_u + 8));
            r2 = _mm_load_si128((const __m128i*)(resi_v));
            r3 = _mm_load_si128((const __m128i*)(resi_v + 8));
            r4 = _mm_unpacklo_epi16(r0, r2);    // UV interlaced
            r5 = _mm_unpackhi_epi16(r0, r2);
            r6 = _mm_unpacklo_epi16(r1, r3);
            r7 = _mm_unpackhi_epi16(r1, r3);
            p0 = _mm_adds_epi16(p0, r4);
            p1 = _mm_adds_epi16(p1, r5);
            p2 = _mm_adds_epi16(p2, r6);
            p3 = _mm_adds_epi16(p3, r7);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p2 = _mm_max_epi16(p2, min_pel);
            p3 = _mm_max_epi16(p3, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);
            p2 = _mm_min_epi16(p2, max_pel);
            p3 = _mm_min_epi16(p3, max_pel);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);
            _mm_store_si128((__m128i*)(rec + 16), p2);
            _mm_store_si128((__m128i*)(rec + 24), p3);

            pred += 32;
            resi_u += 16;
            resi_v += 16;
            rec += i_rec;
        }
    }
    else if (cbf[0]) {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            p2 = _mm_load_si128((const __m128i*)(pred + 16));
            p3 = _mm_load_si128((const __m128i*)(pred + 24));
            r0 = _mm_load_si128((const __m128i*)(resi_u));
            r1 = _mm_load_si128((const __m128i*)(resi_u + 8));
            r4 = _mm_unpacklo_epi16(r0, min_pel);    // UV interlaced
            r5 = _mm_unpackhi_epi16(r0, min_pel);
            r6 = _mm_unpacklo_epi16(r1, min_pel);
            r7 = _mm_unpackhi_epi16(r1, min_pel);
            p0 = _mm_adds_epi16(p0, r4);
            p1 = _mm_adds_epi16(p1, r5);
            p2 = _mm_adds_epi16(p2, r6);
            p3 = _mm_adds_epi16(p3, r7);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p2 = _mm_max_epi16(p2, min_pel);
            p3 = _mm_max_epi16(p3, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);
            p2 = _mm_min_epi16(p2, max_pel);
            p3 = _mm_min_epi16(p3, max_pel);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);
            _mm_store_si128((__m128i*)(rec + 16), p2);
            _mm_store_si128((__m128i*)(rec + 24), p3);

            pred += 32;
            resi_u += 16;
            rec += i_rec;
        }
    }
    else {
        for (i = 0; i < height; i++) {
            p0 = _mm_load_si128((const __m128i*)(pred));
            p1 = _mm_load_si128((const __m128i*)(pred + 8));
            p2 = _mm_load_si128((const __m128i*)(pred + 16));
            p3 = _mm_load_si128((const __m128i*)(pred + 24));
            r2 = _mm_load_si128((const __m128i*)(resi_v));
            r3 = _mm_load_si128((const __m128i*)(resi_v + 8));
            r4 = _mm_unpacklo_epi16(min_pel, r2);    // UV interlaced
            r5 = _mm_unpackhi_epi16(min_pel, r2);
            r6 = _mm_unpacklo_epi16(min_pel, r3);
            r7 = _mm_unpackhi_epi16(min_pel, r3);
            p0 = _mm_adds_epi16(p0, r4);
            p1 = _mm_adds_epi16(p1, r5);
            p2 = _mm_adds_epi16(p2, r6);
            p3 = _mm_adds_epi16(p3, r7);

            p0 = _mm_max_epi16(p0, min_pel);
            p1 = _mm_max_epi16(p1, min_pel);
            p2 = _mm_max_epi16(p2, min_pel);
            p3 = _mm_max_epi16(p3, min_pel);
            p0 = _mm_min_epi16(p0, max_pel);
            p1 = _mm_min_epi16(p1, max_pel);
            p2 = _mm_min_epi16(p2, max_pel);
            p3 = _mm_min_epi16(p3, max_pel);

            _mm_store_si128((__m128i*)(rec), p0);
            _mm_store_si128((__m128i*)(rec + 8), p1);
            _mm_store_si128((__m128i*)(rec + 16), p2);
            _mm_store_si128((__m128i*)(rec + 24), p3);

            pred += 32;
            resi_v += 16;
            rec += i_rec;
        }
    }
}

void uavs3d_recon_chroma_w16x_sse(s16 *resi_u, s16 *resi_v, pel *pred, int width, int height, pel *rec, int i_rec, u8 cbf[2], int bit_depth)
{
    int i, j;
    int width2 = width << 1;
    __m128i p0, p1, p2, p3, r0, r1, r2, r3, r4, r5, r6, r7;
    __m128i min_pel = _mm_setzero_si128();
    __m128i max_pel = _mm_set1_epi16((1 << bit_depth) - 1);

    if (cbf[0] && cbf[1]) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j += 16) {
                int j2 = j << 1;
                p0 = _mm_loadu_si128((const __m128i*)(pred + j2));
                p1 = _mm_loadu_si128((const __m128i*)(pred + j2 + 8));
                p2 = _mm_loadu_si128((const __m128i*)(pred + j2 + 16));
                p3 = _mm_loadu_si128((const __m128i*)(pred + j2 + 24));
                r0 = _mm_loadu_si128((const __m128i*)(resi_u + j));
                r1 = _mm_loadu_si128((const __m128i*)(resi_u + j + 8));
                r2 = _mm_loadu_si128((const __m128i*)(resi_v + j));
                r3 = _mm_loadu_si128((const __m128i*)(resi_v + j + 8));
                r4 = _mm_unpacklo_epi16(r0, r2);    // UV interlaced
                r5 = _mm_unpackhi_epi16(r0, r2);
                r6 = _mm_unpacklo_epi16(r1, r3);
                r7 = _mm_unpackhi_epi16(r1, r3);
                p0 = _mm_adds_epi16(p0, r4);
                p1 = _mm_adds_epi16(p1, r5);
                p2 = _mm_adds_epi16(p2, r6);
                p3 = _mm_adds_epi16(p3, r7);

                p0 = _mm_max_epi16(p0, min_pel);
                p1 = _mm_max_epi16(p1, min_pel);
                p2 = _mm_max_epi16(p2, min_pel);
                p3 = _mm_max_epi16(p3, min_pel);
                p0 = _mm_min_epi16(p0, max_pel);
                p1 = _mm_min_epi16(p1, max_pel);
                p2 = _mm_min_epi16(p2, max_pel);
                p3 = _mm_min_epi16(p3, max_pel);

                _mm_store_si128((__m128i*)(rec + j2), p0);
                _mm_store_si128((__m128i*)(rec + j2 + 8), p1);
                _mm_store_si128((__m128i*)(rec + j2 + 16), p2);
                _mm_store_si128((__m128i*)(rec + j2 + 24), p3);
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
                p0 = _mm_loadu_si128((const __m128i*)(pred + j2));
                p1 = _mm_loadu_si128((const __m128i*)(pred + j2 + 8));
                p2 = _mm_loadu_si128((const __m128i*)(pred + j2 + 16));
                p3 = _mm_loadu_si128((const __m128i*)(pred + j2 + 24));
                r0 = _mm_loadu_si128((const __m128i*)(resi_u + j));
                r1 = _mm_loadu_si128((const __m128i*)(resi_u + j + 8));
                r4 = _mm_unpacklo_epi16(r0, min_pel);    // UV interlaced
                r5 = _mm_unpackhi_epi16(r0, min_pel);
                r6 = _mm_unpacklo_epi16(r1, min_pel);
                r7 = _mm_unpackhi_epi16(r1, min_pel);
                p0 = _mm_adds_epi16(p0, r4);
                p1 = _mm_adds_epi16(p1, r5);
                p2 = _mm_adds_epi16(p2, r6);
                p3 = _mm_adds_epi16(p3, r7);

                p0 = _mm_max_epi16(p0, min_pel);
                p1 = _mm_max_epi16(p1, min_pel);
                p2 = _mm_max_epi16(p2, min_pel);
                p3 = _mm_max_epi16(p3, min_pel);
                p0 = _mm_min_epi16(p0, max_pel);
                p1 = _mm_min_epi16(p1, max_pel);
                p2 = _mm_min_epi16(p2, max_pel);
                p3 = _mm_min_epi16(p3, max_pel);

                _mm_store_si128((__m128i*)(rec + j2), p0);
                _mm_store_si128((__m128i*)(rec + j2 + 8), p1);
                _mm_store_si128((__m128i*)(rec + j2 + 16), p2);
                _mm_store_si128((__m128i*)(rec + j2 + 24), p3);
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
                p0 = _mm_loadu_si128((const __m128i*)(pred + j2));
                p1 = _mm_loadu_si128((const __m128i*)(pred + j2 + 8));
                p2 = _mm_loadu_si128((const __m128i*)(pred + j2 + 16));
                p3 = _mm_loadu_si128((const __m128i*)(pred + j2 + 24));
                r2 = _mm_loadu_si128((const __m128i*)(resi_v + j));
                r3 = _mm_loadu_si128((const __m128i*)(resi_v + j + 8));
                r4 = _mm_unpacklo_epi16(min_pel, r2);    // UV interlaced
                r5 = _mm_unpackhi_epi16(min_pel, r2);
                r6 = _mm_unpacklo_epi16(min_pel, r3);
                r7 = _mm_unpackhi_epi16(min_pel, r3);
                p0 = _mm_adds_epi16(p0, r4);
                p1 = _mm_adds_epi16(p1, r5);
                p2 = _mm_adds_epi16(p2, r6);
                p3 = _mm_adds_epi16(p3, r7);

                p0 = _mm_max_epi16(p0, min_pel);
                p1 = _mm_max_epi16(p1, min_pel);
                p2 = _mm_max_epi16(p2, min_pel);
                p3 = _mm_max_epi16(p3, min_pel);
                p0 = _mm_min_epi16(p0, max_pel);
                p1 = _mm_min_epi16(p1, max_pel);
                p2 = _mm_min_epi16(p2, max_pel);
                p3 = _mm_min_epi16(p3, max_pel);

                _mm_store_si128((__m128i*)(rec + j2), p0);
                _mm_store_si128((__m128i*)(rec + j2 + 8), p1);
                _mm_store_si128((__m128i*)(rec + j2 + 16), p2);
                _mm_store_si128((__m128i*)(rec + j2 + 24), p3);
            }
            pred += width2;
            resi_v += width;
            rec += i_rec;
        }
    }
}

#endif

void uavs3d_conv_fmt_8bit_sse(unsigned char* src_y, unsigned char* src_uv, unsigned char* dst[3], int width, int height, int src_stride, int src_stridec, int dst_stride[3], int uv_shift)
{
    __m128i uv_mask = _mm_set1_epi16(0xff), msrc, msrcu, msrcv;
    unsigned char* p_src = src_y;
    unsigned char* p_dst_y = dst[0];
    unsigned char* p_dst_u = dst[1];
    unsigned char* p_dst_v = dst[2];
    int i, j, j2;
    int width16;
    for (i = 0; i < height; i++) {
        memcpy(p_dst_y, p_src, width);
        p_src += src_stride;
        p_dst_y += dst_stride[0];
    }
    width >>= uv_shift;
    height >>= uv_shift;

    p_src = src_uv;
    width <<= 1;
    width16 = (width >> 4) << 4;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width16; j += 16) {
            j2 = j >> 1;
            msrc = _mm_loadu_si128((const __m128i*)(p_src + j));
            msrcu = _mm_and_si128(msrc, uv_mask);
            msrcv = _mm_and_si128(_mm_srli_si128(msrc, 1), uv_mask);
            msrc = _mm_packus_epi16(msrcu, msrcv);
            _mm_storel_epi64((__m128i*)(p_dst_u + j2), msrc);
            _mm_storel_epi64((__m128i*)(p_dst_v + j2), _mm_srli_si128(msrc, 8));
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

void uavs3d_conv_fmt_16bit_sse(unsigned char* src_y, unsigned char* src_uv, unsigned char* dst[3], int width, int height, int src_stride, int src_stridec, int dst_stride[3], int uv_shift)
{
    __m128i uv_mask = _mm_set1_epi32(0xffff), msrc, msrcu, msrcv;
    unsigned char* p_src = src_y;
    unsigned char* p_dst_y = dst[0];
    unsigned char* p_dst_u = dst[1];
    unsigned char* p_dst_v = dst[2];
    int i, j, j2;
    int width16;
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
    width16 = (width >> 4) << 4;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width16; j += 16) {
            j2 = j >> 1;
            msrc = _mm_loadu_si128((const __m128i*)(p_src + j));
            msrcu = _mm_and_si128(msrc, uv_mask);
            msrcv = _mm_and_si128(_mm_srli_si128(msrc, 2), uv_mask);
            msrc = _mm_packus_epi32(msrcu, msrcv);
            _mm_storel_epi64((__m128i*)(p_dst_u + j2), msrc);
            _mm_storel_epi64((__m128i*)(p_dst_v + j2), _mm_srli_si128(msrc, 8));
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

void uavs3d_conv_fmt_16to8bit_sse(unsigned char* src_y, unsigned char* src_uv, unsigned char* dst[3], int width, int height, int src_stride, int src_stridec, int dst_stride[3], int uv_shift)
{
    __m128i uv_mask = _mm_set1_epi32(0xffff);
    __m128i m0, m1, m2, m3, m4, m5;
    unsigned char* p_src = src_y;
    unsigned char* p_dst_y = dst[0];
    unsigned char* p_dst_u = dst[1];
    unsigned char* p_dst_v = dst[2];
    int i, j, j2;
    int width32 = (width >> 4) << 5;

    width <<= 1;    // width for byte
    for (i = 0; i < height; i++) {
        for (j = 0; j < width32; j += 32) {
            j2 = j >> 1;
            m0 = _mm_loadu_si128((const __m128i*)(p_src + j));
            m1 = _mm_loadu_si128((const __m128i*)(p_src + j + 16));
            m2 = _mm_packus_epi16(m0, m1);
            _mm_storeu_si128((__m128i*)(p_dst_y + j2), m2);
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
    width32 = (width >> 5) << 5;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width32; j += 32) {
            j2 = j >> 1;
            m0 = _mm_loadu_si128((const __m128i*)(p_src + j));
            m1 = _mm_loadu_si128((const __m128i*)(p_src + j + 16));
            m2 = _mm_and_si128(m0, uv_mask);        // U: 0-4 (32bit)
            m3 = _mm_and_si128(m1, uv_mask);        // U: 4-8
            m4 = _mm_and_si128(_mm_srli_si128(m0, 2), uv_mask); // V: 0-4 (32bit)
            m5 = _mm_and_si128(_mm_srli_si128(m1, 2), uv_mask); // V: 4-8
            m2 = _mm_packus_epi32(m2, m3);
            m4 = _mm_packus_epi32(m4, m5);
            m2 = _mm_packus_epi16(m2, m4);          // U: 0-8; V: 0-8 (8bit)
            _mm_storel_epi64((__m128i*)(p_dst_u + (j2 >> 1)), m2);
            _mm_storel_epi64((__m128i*)(p_dst_v + (j2 >> 1)), _mm_srli_si128(m2, 8));
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