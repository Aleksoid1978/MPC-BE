/*
 *      Copyright (C) 2010-2019 Hendrik Leppkes
 *      http://www.1f0.de
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 *  Adaptation for MPC-BE (C) 2013-2017 v0lt & Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
 */

#include "stdafx.h"
#include "FormatConverter.h"

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C" {
    #include <ExtLib/ffmpeg/libavutil/intreadwrite.h>
    #include <ExtLib/ffmpeg/libswscale/swscale.h>
}
#pragma warning(pop)

int sws_scale2(struct SwsContext *c, const uint8_t *const srcSlice[], const ptrdiff_t srcStride[], int srcSliceY, int srcSliceH, uint8_t *const dst[], const ptrdiff_t dstStride[])
{
    int srcStride2[4];
    int dstStride2[4];

    for (int i = 0; i < 4; i++) {
        srcStride2[i] = (int)srcStride[i];
        dstStride2[i] = (int)dstStride[i];
    }
    return sws_scale(c, srcSlice, srcStride2, srcSliceY, srcSliceH, dst, dstStride2);
}

HRESULT CFormatConverter::ConvertGeneric(CONV_FUNC_PARAMS)
{
    if (!m_pSwsContext) {
        InitSWSContext();
    }

    CheckPointer(m_pSwsContext, E_POINTER);

    switch (m_out_pixfmt) {
    case PixFmt_AYUV:
        ConvertToAYUV(src, srcStride, dst, width, height, dstStride);
        break;
    case PixFmt_P010:
    case PixFmt_P016:
        ConvertToPX1X(src, srcStride, dst, width, height, dstStride, 2);
        break;
    case PixFmt_Y410:
        ConvertToY410(src, srcStride, dst, width, height, dstStride);
        break;
    case PixFmt_P210:
    case PixFmt_P216:
        ConvertToPX1X(src, srcStride, dst, width, height, dstStride, 1);
        break;
    case PixFmt_Y416:
        ConvertToY416(src, srcStride, dst, width, height, dstStride);
        break;
    default:
        if (m_out_pixfmt == PixFmt_YV12 || m_out_pixfmt == PixFmt_YV16 || m_out_pixfmt == PixFmt_YV24) {
            std::swap(dst[1], dst[2]);
        }
        else if (m_out_pixfmt == PixFmt_RGB32 && m_OutHeight > 0) {
            // flip the image, if necessary
            dst[0] += dstStride[0] * (height - 1);
            const ptrdiff_t dstStride2 = -dstStride[0];
            int ret = sws_scale2(m_pSwsContext, src, srcStride, 0, height, dst, &dstStride2);
            break;
        }
        int ret = sws_scale2(m_pSwsContext, src, srcStride, 0, height, dst, dstStride);
    }

    return S_OK;
}

//
// based on LAVFilters/decoder/LAVVideo/pixconv/convert_generic.cpp
//

HRESULT CFormatConverter::ConvertToAYUV(CONV_FUNC_PARAMS)
{
    const BYTE *y = nullptr;
    const BYTE *u = nullptr;
    const BYTE *v = nullptr;
    ptrdiff_t line, i = 0;
    ptrdiff_t sourceStride = 0;
    BYTE *pTmpBuffer = nullptr;

    if (m_FProps.avpixfmt != AV_PIX_FMT_YUV444P)
    {
        uint8_t *tmp[4] = {nullptr};
        ptrdiff_t tmpStride[4] = {0};
        ptrdiff_t scaleStride = FFALIGN(width, 32);

        pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 3);
        if (pTmpBuffer == nullptr)
            return E_OUTOFMEMORY;

        tmp[0] = pTmpBuffer;
        tmp[1] = tmp[0] + (height * scaleStride);
        tmp[2] = tmp[1] + (height * scaleStride);
        tmp[3] = nullptr;
        tmpStride[0] = scaleStride;
        tmpStride[1] = scaleStride;
        tmpStride[2] = scaleStride;
        tmpStride[3] = 0;

        sws_scale2(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

        y = tmp[0];
        u = tmp[1];
        v = tmp[2];
        sourceStride = scaleStride;
    }
    else
    {
        y = src[0];
        u = src[1];
        v = src[2];
        sourceStride = srcStride[0];
    }

#define YUV444_PACK_AYUV(offset) *idst++ = v[i + offset] | (u[i + offset] << 8) | (y[i + offset] << 16) | (0xff << 24);

    BYTE *out = dst[0];
    for (line = 0; line < height; ++line)
    {
        uint32_t *idst = (uint32_t *)out;
        for (i = 0; i < (width - 7); i += 8)
        {
            YUV444_PACK_AYUV(0)
            YUV444_PACK_AYUV(1)
            YUV444_PACK_AYUV(2)
            YUV444_PACK_AYUV(3)
            YUV444_PACK_AYUV(4)
            YUV444_PACK_AYUV(5)
            YUV444_PACK_AYUV(6)
            YUV444_PACK_AYUV(7)
        }
        for (; i < width; ++i)
        {
            YUV444_PACK_AYUV(0)
        }
        y += sourceStride;
        u += sourceStride;
        v += sourceStride;
        out += dstStride[0];
    }

    av_freep(&pTmpBuffer);

    return S_OK;
}

HRESULT CFormatConverter::ConvertToPX1X(CONV_FUNC_PARAMS, int chromaVertical)
{
    const BYTE *y = nullptr;
    const BYTE *u = nullptr;
    const BYTE *v = nullptr;
    ptrdiff_t line, i = 0;
    ptrdiff_t sourceStride = 0;

    int shift = 0;

    BYTE *pTmpBuffer = nullptr;

    if ((chromaVertical == 1 && m_FProps.pftype != PFType_YUV422Px) ||
        (chromaVertical == 2 && m_FProps.pftype != PFType_YUV420Px))
    {
        uint8_t *tmp[4] = {nullptr};
        ptrdiff_t tmpStride[4] = {0};
        ptrdiff_t scaleStride = FFALIGN(width, 32) * 2;

        pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 2);
        if (pTmpBuffer == nullptr)
            return E_OUTOFMEMORY;

        tmp[0] = pTmpBuffer;
        tmp[1] = tmp[0] + (height * scaleStride);
        tmp[2] = tmp[1] + ((height / chromaVertical) * (scaleStride / 2));
        tmp[3] = nullptr;
        tmpStride[0] = scaleStride;
        tmpStride[1] = scaleStride / 2;
        tmpStride[2] = scaleStride / 2;
        tmpStride[3] = 0;

        sws_scale2(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

        y = tmp[0];
        u = tmp[1];
        v = tmp[2];
        sourceStride = scaleStride;
    }
    else
    {
        y = src[0];
        u = src[1];
        v = src[2];
        sourceStride = srcStride[0];

        shift = (16 - m_FProps.lumabits);
    }

    // copy Y
    BYTE *pLineOut = dst[0];
    const BYTE *pLineIn = y;
    for (line = 0; line < height; ++line)
    {
        if (shift == 0)
        {
            memcpy(pLineOut, pLineIn, width * 2);
        }
        else
        {
            const uint16_t *yc = (uint16_t *)pLineIn;
            uint16_t *idst = (uint16_t *)pLineOut;
            for (i = 0; i < width; ++i)
            {
                uint16_t yv = AV_RL16(yc + i);
                if (shift)
                    yv <<= shift;
                *idst++ = yv;
            }
        }
        pLineOut += dstStride[0];
        pLineIn += sourceStride;
    }

    sourceStride >>= 2;

    // Merge U/V
    BYTE *out = dst[1];
    const uint16_t *uc = (uint16_t *)u;
    const uint16_t *vc = (uint16_t *)v;
    for (line = 0; line < height / chromaVertical; ++line)
    {
        uint32_t *idst = (uint32_t *)out;
        for (i = 0; i < width / 2; ++i)
        {
            uint16_t uv = AV_RL16(uc + i);
            uint16_t vv = AV_RL16(vc + i);
            if (shift)
            {
                uv <<= shift;
                vv <<= shift;
            }
            *idst++ = uv | (vv << 16);
        }
        uc += sourceStride;
        vc += sourceStride;
        out += dstStride[1];
    }

    av_freep(&pTmpBuffer);

    return S_OK;
}

#define YUV444_PACKED_LOOP_HEAD(width, height, y, u, v, out) \
    for (int line = 0; line < height; ++line)                \
    {                                                        \
        uint32_t *idst = (uint32_t *)out;                    \
        for (int i = 0; i < width; ++i)                      \
        {                                                    \
            uint32_t yv, uv, vv;

#define YUV444_PACKED_LOOP_HEAD_LE(width, height, y, u, v, out) \
    YUV444_PACKED_LOOP_HEAD(width, height, y, u, v, out)        \
    yv = AV_RL16(y + i);                                        \
    uv = AV_RL16(u + i);                                        \
    vv = AV_RL16(v + i);

#define YUV444_PACKED_LOOP_END(y, u, v, out, srcStride, dstStride) \
    }                                                              \
    y += srcStride;                                                \
    u += srcStride;                                                \
    v += srcStride;                                                \
    out += dstStride;                                              \
    }

HRESULT CFormatConverter::ConvertToY410(CONV_FUNC_PARAMS)
{
    const uint16_t *y = nullptr;
    const uint16_t *u = nullptr;
    const uint16_t *v = nullptr;
    ptrdiff_t sourceStride = 0;
    bool b9Bit = false;

    BYTE *pTmpBuffer = nullptr;

    if (m_FProps.pftype != PFType_YUV444Px || m_FProps.lumabits > 10)
    {
        uint8_t *tmp[4] = {nullptr};
        ptrdiff_t tmpStride[4] = {0};
        ptrdiff_t scaleStride = FFALIGN(width, 32);

        pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 6);
        if (pTmpBuffer == nullptr)
            return E_OUTOFMEMORY;

        tmp[0] = pTmpBuffer;
        tmp[1] = tmp[0] + (height * scaleStride * 2);
        tmp[2] = tmp[1] + (height * scaleStride * 2);
        tmp[3] = nullptr;
        tmpStride[0] = scaleStride * 2;
        tmpStride[1] = scaleStride * 2;
        tmpStride[2] = scaleStride * 2;
        tmpStride[3] = 0;

        sws_scale2(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

        y = (uint16_t *)tmp[0];
        u = (uint16_t *)tmp[1];
        v = (uint16_t *)tmp[2];
        sourceStride = scaleStride;
    }
    else
    {
        y = (uint16_t *)src[0];
        u = (uint16_t *)src[1];
        v = (uint16_t *)src[2];
        sourceStride = srcStride[0] / 2;

        b9Bit = (m_FProps.lumabits == 9);
    }

#define YUV444_Y410_PACK *idst++ = (uv & 0x3FF) | ((yv & 0x3FF) << 10) | ((vv & 0x3FF) << 20) | (3 << 30);

    BYTE *out = dst[0];
    YUV444_PACKED_LOOP_HEAD_LE(width, height, y, u, v, out)
    if (b9Bit)
    {
        yv <<= 1;
        uv <<= 1;
        vv <<= 1;
    }
    YUV444_Y410_PACK
    YUV444_PACKED_LOOP_END(y, u, v, out, sourceStride, dstStride[0])

    av_freep(&pTmpBuffer);

    return S_OK;
}

HRESULT CFormatConverter::ConvertToY416(CONV_FUNC_PARAMS)
{
    const uint16_t *y = nullptr;
    const uint16_t *u = nullptr;
    const uint16_t *v = nullptr;
    ptrdiff_t sourceStride = 0;

    BYTE *pTmpBuffer = nullptr;

    int shift = (16 - m_FProps.lumabits);
    if (m_FProps.pftype != PFType_YUV444Px)
    {
        uint8_t *tmp[4] = {nullptr};
        ptrdiff_t tmpStride[4] = {0};
        ptrdiff_t scaleStride = FFALIGN(width, 32);

        pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 6);
        if (pTmpBuffer == nullptr)
            return E_OUTOFMEMORY;

        tmp[0] = pTmpBuffer;
        tmp[1] = tmp[0] + (height * scaleStride * 2);
        tmp[2] = tmp[1] + (height * scaleStride * 2);
        tmp[3] = nullptr;
        tmpStride[0] = scaleStride * 2;
        tmpStride[1] = scaleStride * 2;
        tmpStride[2] = scaleStride * 2;
        tmpStride[3] = 0;

        sws_scale2(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

        y = (uint16_t *)tmp[0];
        u = (uint16_t *)tmp[1];
        v = (uint16_t *)tmp[2];
        sourceStride = scaleStride;
        shift = 0;
    }
    else
    {
        y = (uint16_t *)src[0];
        u = (uint16_t *)src[1];
        v = (uint16_t *)src[2];
        sourceStride = srcStride[0] / 2;
    }

    BYTE *out = dst[0];
    YUV444_PACKED_LOOP_HEAD_LE(width, height, y, u, v, out)
    uint16_t *p = (uint16_t *)idst;
    p[0] = (uv << shift);
    p[1] = (yv << shift);
    p[2] = (vv << shift);
    p[3] = 0xFFFF;

    idst += 2;
    YUV444_PACKED_LOOP_END(y, u, v, out, sourceStride, dstStride[0])

    av_freep(&pTmpBuffer);

    return S_OK;
}
