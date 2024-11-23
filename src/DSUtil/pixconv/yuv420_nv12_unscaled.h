/*
 * (C) 2013-2024 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "pixconv_sse2_templates.h"

//
// from LAVFilters/decoder/LAVVideo/pixconv/yuv2yuv_unscaled.cpp
//

// destination must be aligned to 32
inline void convert_yuv420_nv12(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
    const ptrdiff_t inLumaStride = srcStride[0];
    const ptrdiff_t inChromaStride = srcStride[1];

    const ptrdiff_t outLumaStride = dstStride[0];
    const ptrdiff_t outChromaStride = dstStride[1];

    const ptrdiff_t chromaWidth = (width + 1) >> 1;
    const ptrdiff_t chromaHeight = height >> 1;

    ptrdiff_t line, i;
    __m128i xmm0, xmm1, xmm2, xmm3, xmm4;

    _mm_sfence();

    // Y
    if (outLumaStride == inLumaStride)
    {
        memcpy(dst[0], src[0], outLumaStride * height);
    }
    else
    {
        for (line = 0; line < height; ++line)
        {
            PIXCONV_MEMCPY_ALIGNED(dst[0] + outLumaStride * line, src[0] + inLumaStride * line, width);
        }
    }

    // U/V
    for (line = 0; line < chromaHeight; ++line)
    {
        const uint8_t *const u = src[1] + line * inChromaStride;
        const uint8_t *const v = src[2] + line * inChromaStride;
        uint8_t *const d = dst[1] + line * outChromaStride;

        for (i = 0; i < (chromaWidth - 31); i += 32)
        {
            PIXCONV_LOAD_PIXEL8_ALIGNED(xmm0, v + i);
            PIXCONV_LOAD_PIXEL8_ALIGNED(xmm1, u + i);
            PIXCONV_LOAD_PIXEL8_ALIGNED(xmm2, v + i + 16);
            PIXCONV_LOAD_PIXEL8_ALIGNED(xmm3, u + i + 16);

            xmm4 = xmm0;
            xmm0 = _mm_unpacklo_epi8(xmm1, xmm0);
            xmm4 = _mm_unpackhi_epi8(xmm1, xmm4);

            xmm1 = xmm2;
            xmm2 = _mm_unpacklo_epi8(xmm3, xmm2);
            xmm1 = _mm_unpackhi_epi8(xmm3, xmm1);

            PIXCONV_PUT_STREAM(d + (i << 1) + 0, xmm0);
            PIXCONV_PUT_STREAM(d + (i << 1) + 16, xmm4);
            PIXCONV_PUT_STREAM(d + (i << 1) + 32, xmm2);
            PIXCONV_PUT_STREAM(d + (i << 1) + 48, xmm1);
        }
        for (; i < chromaWidth; i += 16)
        {
            PIXCONV_LOAD_PIXEL8_ALIGNED(xmm0, v + i);
            PIXCONV_LOAD_PIXEL8_ALIGNED(xmm1, u + i);

            xmm2 = xmm0;
            xmm0 = _mm_unpacklo_epi8(xmm1, xmm0);
            xmm2 = _mm_unpackhi_epi8(xmm1, xmm2);

            PIXCONV_PUT_STREAM(d + (i << 1) + 0, xmm0);
            PIXCONV_PUT_STREAM(d + (i << 1) + 16, xmm2);
        }
    }
}
