/*
 *      Copyright (C) 2010-2014 Hendrik Leppkes
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
 *  Adaptation for MPC-BE (C) 2013-2016 v0lt & Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
 *
 */

#include "stdafx.h"
#include "FormatConverter.h"
#include "pixconv_sse2_templates.h"
#include "../../../DSUtil/Log.h"

#include <ppl.h>

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C" {
  #include <ffmpeg/libavutil/pixfmt.h>
  #include <ffmpeg/libavutil/intreadwrite.h>
  #include <ffmpeg/libswscale/swscale.h>
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

HRESULT CFormatConverter::ConvertToAYUV(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const BYTE *y = NULL;
  const BYTE *u = NULL;
  const BYTE *v = NULL;
  ptrdiff_t line, i = 0;
  ptrdiff_t sourceStride = 0;
  BYTE *pTmpBuffer = NULL;

  if (m_FProps.avpixfmt != AV_PIX_FMT_YUV444P) {
    uint8_t  *tmp[4] = {NULL};
    ptrdiff_t tmpStride[4] = {0};
    ptrdiff_t scaleStride = FFALIGN(width, 32);

    pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 3);

    tmp[0] = pTmpBuffer;
    tmp[1] = tmp[0] + (height * scaleStride);
    tmp[2] = tmp[1] + (height * scaleStride);
    tmp[3] = NULL;
    tmpStride[0] = scaleStride;
    tmpStride[1] = scaleStride;
    tmpStride[2] = scaleStride;
    tmpStride[3] = 0;

    sws_scale2(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

    y = tmp[0];
    u = tmp[1];
    v = tmp[2];
    sourceStride = scaleStride;
  } else {
    y = src[0];
    u = src[1];
    v = src[2];
    sourceStride = srcStride[0];
  }

#define YUV444_PACK_AYUV(offset) *idst++ = v[i+offset] | (u[i+offset] << 8) | (y[i+offset] << 16) | (0xff << 24);

  BYTE *out = dst[0];
  for (line = 0; line < height; ++line) {
    int32_t *idst = (int32_t *)out;
    for (i = 0; i < (width-7); i+=8) {
      YUV444_PACK_AYUV(0)
      YUV444_PACK_AYUV(1)
      YUV444_PACK_AYUV(2)
      YUV444_PACK_AYUV(3)
      YUV444_PACK_AYUV(4)
      YUV444_PACK_AYUV(5)
      YUV444_PACK_AYUV(6)
      YUV444_PACK_AYUV(7)
    }
    for (; i < width; ++i) {
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

HRESULT CFormatConverter::ConvertToPX1X(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[], int chromaVertical)
{
  const BYTE *y = NULL;
  const BYTE *u = NULL;
  const BYTE *v = NULL;
  ptrdiff_t line, i = 0;
  ptrdiff_t sourceStride = 0;

  int shift = 0;

  BYTE *pTmpBuffer = NULL;

  if ((m_FProps.pftype != PFType_YUV422Px && chromaVertical == 1) || (m_FProps.pftype != PFType_YUV420Px && chromaVertical == 2)) {
    uint8_t  *tmp[4] = {NULL};
    ptrdiff_t tmpStride[4] = {0};
    ptrdiff_t scaleStride = FFALIGN(width, 32) * 2;

    pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 2);

    tmp[0] = pTmpBuffer;
    tmp[1] = tmp[0] + (height * scaleStride);
    tmp[2] = tmp[1] + ((height / chromaVertical) * (scaleStride / 2));
    tmp[3] = NULL;
    tmpStride[0] = scaleStride;
    tmpStride[1] = scaleStride / 2;
    tmpStride[2] = scaleStride / 2;
    tmpStride[3] = 0;

    sws_scale2(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

    y = tmp[0];
    u = tmp[1];
    v = tmp[2];
    sourceStride = scaleStride;
  } else {
    y = src[0];
    u = src[1];
    v = src[2];
    sourceStride = srcStride[0];

    shift = (16 - m_FProps.lumabits);
  }

  // copy Y
  BYTE *pLineOut = dst[0];
  const BYTE *pLineIn = y;
  for (line = 0; line < height; ++line) {
    if (shift == 0) {
      memcpy(pLineOut, pLineIn, width * 2);
    } else {
      const int16_t *yc = (int16_t *)pLineIn;
      int16_t *idst = (int16_t *)pLineOut;
      for (i = 0; i < width; ++i) {
        int32_t yv = AV_RL16(yc+i);
        if (shift) yv <<= shift;
        *idst++ = yv;
      }
    }
    pLineOut += dstStride[0];
    pLineIn += sourceStride;
  }

  sourceStride >>= 2;

  // Merge U/V
  BYTE *out = dst[1];
  const int16_t *uc = (int16_t *)u;
  const int16_t *vc = (int16_t *)v;
  for (line = 0; line < height/chromaVertical; ++line) {
    int32_t *idst = (int32_t *)out;
    for (i = 0; i < width/2; ++i) {
      int32_t uv = AV_RL16(uc+i);
      int32_t vv = AV_RL16(vc+i);
      if (shift) {
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
  for (int line = 0; line < height; ++line) { \
    int32_t *idst = (int32_t *)out; \
    for(int i = 0; i < width; ++i) { \
      int32_t yv, uv, vv;

#define YUV444_PACKED_LOOP_HEAD_LE(width, height, y, u, v, out) \
  YUV444_PACKED_LOOP_HEAD(width, height, y, u, v, out) \
    yv = AV_RL16(y+i); uv = AV_RL16(u+i); vv = AV_RL16(v+i);

#define YUV444_PACKED_LOOP_END(y, u, v, out, srcStride, dstStride) \
    } \
    y += srcStride; \
    u += srcStride; \
    v += srcStride; \
    out += dstStride; \
  }

HRESULT CFormatConverter::ConvertToY410(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const int16_t *y = NULL;
  const int16_t *u = NULL;
  const int16_t *v = NULL;
  ptrdiff_t sourceStride = 0;
  bool b9Bit = false;

  BYTE *pTmpBuffer = NULL;

  if (m_FProps.pftype != PFType_YUV444Px || m_FProps.lumabits > 10) {
    uint8_t  *tmp[4] = {NULL};
    ptrdiff_t tmpStride[4] = {0};
    ptrdiff_t scaleStride = FFALIGN(width, 32);

    pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 6);

    tmp[0] = pTmpBuffer;
    tmp[1] = tmp[0] + (height * scaleStride * 2);
    tmp[2] = tmp[1] + (height * scaleStride * 2);
    tmp[3] = NULL;
    tmpStride[0] = scaleStride * 2;
    tmpStride[1] = scaleStride * 2;
    tmpStride[2] = scaleStride * 2;
    tmpStride[3] = 0;

    sws_scale2(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

    y = (int16_t *)tmp[0];
    u = (int16_t *)tmp[1];
    v = (int16_t *)tmp[2];
    sourceStride = scaleStride;
  } else {
    y = (int16_t *)src[0];
    u = (int16_t *)src[1];
    v = (int16_t *)src[2];
    sourceStride = srcStride[0] / 2;

    b9Bit = (m_FProps.lumabits == 9);
  }

#define YUV444_Y410_PACK \
  *idst++ = (uv & 0x3FF) | ((yv & 0x3FF) << 10) | ((vv & 0x3FF) << 20) | (3 << 30);

  BYTE *out = dst[0];
  YUV444_PACKED_LOOP_HEAD_LE(width, height, y, u, v, out)
    if (b9Bit) {
      yv <<= 1;
      uv <<= 1;
      vv <<= 1;
    }
    YUV444_Y410_PACK
  YUV444_PACKED_LOOP_END(y, u, v, out, sourceStride, dstStride[0])

  av_freep(&pTmpBuffer);

  return S_OK;
}

HRESULT CFormatConverter::ConvertToY416(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const int16_t *y = NULL;
  const int16_t *u = NULL;
  const int16_t *v = NULL;
  ptrdiff_t sourceStride = 0;

  BYTE *pTmpBuffer = NULL;

  if (m_FProps.pftype != PFType_YUV444Px || m_FProps.lumabits != 16) {
    uint8_t  *tmp[4] = {NULL};
    ptrdiff_t tmpStride[4] = {0};
    ptrdiff_t scaleStride = FFALIGN(width, 32);

    pTmpBuffer = (BYTE *)av_malloc(height * scaleStride * 6);

    tmp[0] = pTmpBuffer;
    tmp[1] = tmp[0] + (height * scaleStride * 2);
    tmp[2] = tmp[1] + (height * scaleStride * 2);
    tmp[3] = NULL;
    tmpStride[0] = scaleStride * 2;
    tmpStride[1] = scaleStride * 2;
    tmpStride[2] = scaleStride * 2;
    tmpStride[3] = 0;

    sws_scale2(m_pSwsContext, src, srcStride, 0, height, tmp, tmpStride);

    y = (int16_t *)tmp[0];
    u = (int16_t *)tmp[1];
    v = (int16_t *)tmp[2];
    sourceStride = scaleStride;
  } else {
    y = (int16_t *)src[0];
    u = (int16_t *)src[1];
    v = (int16_t *)src[2];
    sourceStride = srcStride[0] / 2;
  }

#define YUV444_Y416_PACK \
  *idst++ = 0xFFFF | (vv << 16); \
  *idst++ = yv | (uv << 16);

  BYTE *out = dst[0];
  YUV444_PACKED_LOOP_HEAD_LE(width, height, y, u, v, out)
    YUV444_Y416_PACK
  YUV444_PACKED_LOOP_END(y, u, v, out, sourceStride, dstStride[0])

  av_freep(&pTmpBuffer);

  return S_OK;
}

HRESULT CFormatConverter::ConvertGeneric(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
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
    int ret = sws_scale2(m_pSwsContext, src, srcStride, 0, height, dst, dstStride);
  }

  return S_OK;
}

HRESULT CFormatConverter::convert_yuv444_y410(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const uint16_t *y = (const uint16_t *)src[0];
  const uint16_t *u = (const uint16_t *)src[1];
  const uint16_t *v = (const uint16_t *)src[2];

  const ptrdiff_t inStride = srcStride[0] >> 1;
  const ptrdiff_t outStride = dstStride[0];
  int shift = 10 - m_FProps.lumabits;

  ptrdiff_t line, i;

  __m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;

  xmm7 = _mm_set1_epi32(0xC0000000);
  xmm6 = _mm_setzero_si128();

  _mm_sfence();

  for (line = 0; line < height; ++line) {
    __m128i *dst128 = (__m128i *)(dst[0] + line * outStride);

    for (i = 0; i < width; i+=8) {
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm0, (y+i));
      xmm0 = _mm_slli_epi16(xmm0, shift);
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm1, (u+i));
      xmm1 = _mm_slli_epi16(xmm1, shift);
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm2, (v+i));
      xmm2 = _mm_slli_epi16(xmm2, shift+4);  // +4 so its directly aligned properly (data from bit 14 to bit 4)

      xmm3 = _mm_unpacklo_epi16(xmm1, xmm2); // 0VVVVV00000UUUUU
      xmm4 = _mm_unpackhi_epi16(xmm1, xmm2); // 0VVVVV00000UUUUU
      xmm3 = _mm_or_si128(xmm3, xmm7);       // AVVVVV00000UUUUU
      xmm4 = _mm_or_si128(xmm4, xmm7);       // AVVVVV00000UUUUU

      xmm5 = _mm_unpacklo_epi16(xmm0, xmm6); // 00000000000YYYYY
      xmm2 = _mm_unpackhi_epi16(xmm0, xmm6); // 00000000000YYYYY
      xmm5 = _mm_slli_epi32(xmm5, 10);       // 000000YYYYY00000
      xmm2 = _mm_slli_epi32(xmm2, 10);       // 000000YYYYY00000

      xmm3 = _mm_or_si128(xmm3, xmm5);       // AVVVVVYYYYYUUUUU
      xmm4 = _mm_or_si128(xmm4, xmm2);       // AVVVVVYYYYYUUUUU

      // Write data back
      _mm_stream_si128(dst128++, xmm3);
      _mm_stream_si128(dst128++, xmm4);
    }

    y += inStride;
    u += inStride;
    v += inStride;
  }
  return S_OK;
}

#define PIXCONV_INTERLEAVE_AYUV(regY, regU, regV, regA, regOut1, regOut2) \
  regY    = _mm_unpacklo_epi8(regY, regA);     /* YAYAYAYA */             \
  regV    = _mm_unpacklo_epi8(regV, regU);     /* VUVUVUVU */             \
  regOut1 = _mm_unpacklo_epi16(regV, regY);    /* VUYAVUYA */             \
  regOut2 = _mm_unpackhi_epi16(regV, regY);    /* VUYAVUYA */

#define YUV444_PACK_AYUV(dst) *idst++ = v[i] | (u[i] << 8) | (y[i] << 16) | (0xff << 24);

HRESULT CFormatConverter::convert_yuv444_ayuv(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const uint8_t *y = (const uint8_t *)src[0];
  const uint8_t *u = (const uint8_t *)src[1];
  const uint8_t *v = (const uint8_t *)src[2];

  const ptrdiff_t inStride = srcStride[0];
  const ptrdiff_t outStride = dstStride[0];

  ptrdiff_t line, i;

  __m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6;

  xmm6 = _mm_set1_epi32(-1);

  _mm_sfence();

  for (line = 0; line < height; ++line) {
    __m128i *dst128 = (__m128i *)(dst[0] + line * outStride);

    for (i = 0; i < width; i+=16) {
      // Load pixels into registers
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm0, (y+i)); /* YYYYYYYY */
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm1, (u+i)); /* UUUUUUUU */
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm2, (v+i)); /* VVVVVVVV */

      // Interlave into AYUV
      xmm4 = xmm0;
      xmm0 = _mm_unpacklo_epi8(xmm0, xmm6);     /* YAYAYAYA */
      xmm4 = _mm_unpackhi_epi8(xmm4, xmm6);     /* YAYAYAYA */

      xmm5 = xmm2;
      xmm2 = _mm_unpacklo_epi8(xmm2, xmm1);     /* VUVUVUVU */
      xmm5 = _mm_unpackhi_epi8(xmm5, xmm1);     /* VUVUVUVU */

      xmm1 = _mm_unpacklo_epi16(xmm2, xmm0);    /* VUYAVUYA */
      xmm2 = _mm_unpackhi_epi16(xmm2, xmm0);    /* VUYAVUYA */

      xmm0 = _mm_unpacklo_epi16(xmm5, xmm4);    /* VUYAVUYA */
      xmm3 = _mm_unpackhi_epi16(xmm5, xmm4);    /* VUYAVUYA */

      // Write data back
      _mm_stream_si128(dst128++, xmm1);
      _mm_stream_si128(dst128++, xmm2);
      _mm_stream_si128(dst128++, xmm0);
      _mm_stream_si128(dst128++, xmm3);
    }

    y += inStride;
    u += inStride;
    v += inStride;
  }

  return S_OK;
}

HRESULT CFormatConverter::convert_yuv444_ayuv_dither_le(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const uint16_t *y = (const uint16_t *)src[0];
  const uint16_t *u = (const uint16_t *)src[1];
  const uint16_t *v = (const uint16_t *)src[2];

  const ptrdiff_t inStride = srcStride[0] >> 1;
  const ptrdiff_t outStride = dstStride[0];

  const uint16_t *dithers = NULL;

  ptrdiff_t line, i;

  __m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;

  xmm7 = _mm_set1_epi16(-256); /* 0xFF00 - 0A0A0A0A */

  _mm_sfence();

  for (line = 0; line < height; ++line) {
    // Load dithering coefficients for this line
    PIXCONV_LOAD_DITHER_COEFFS(xmm6,line,8,dithers);
    xmm4 = xmm5 = xmm6;

    __m128i *dst128 = (__m128i *)(dst[0] + line * outStride);

    for (i = 0; i < width; i+=8) {
      // Load pixels into registers, and apply dithering
      PIXCONV_LOAD_PIXEL16_DITHER(xmm0, xmm4, (y+i), m_FProps.lumabits); /* Y0Y0Y0Y0 */
      PIXCONV_LOAD_PIXEL16_DITHER_HIGH(xmm1, xmm5, (u+i), m_FProps.lumabits); /* U0U0U0U0 */
      PIXCONV_LOAD_PIXEL16_DITHER(xmm2, xmm6, (v+i), m_FProps.lumabits); /* V0V0V0V0 */

      // Interlave into AYUV
      xmm0 = _mm_or_si128(xmm0, xmm7);          /* YAYAYAYA */
      xmm1 = _mm_and_si128(xmm1, xmm7);         /* clear out clobbered low-bytes */
      xmm2 = _mm_or_si128(xmm2, xmm1);          /* VUVUVUVU */

      xmm3 = xmm2;
      xmm2 = _mm_unpacklo_epi16(xmm2, xmm0);    /* VUYAVUYA */
      xmm3 = _mm_unpackhi_epi16(xmm3, xmm0);    /* VUYAVUYA */

      // Write data back
      _mm_stream_si128(dst128++, xmm2);
      _mm_stream_si128(dst128++, xmm3);
    }

    y += inStride;
    u += inStride;
    v += inStride;
  }

  return S_OK;
}

HRESULT CFormatConverter::convert_yuv420_px1x_le(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const ptrdiff_t inYStride   = srcStride[0];
  const ptrdiff_t inUVStride  = srcStride[1];
  const ptrdiff_t outYStride  = dstStride[0];
  const ptrdiff_t outUVStride = dstStride[1];
  const ptrdiff_t uvHeight    = (m_out_pixfmt == PixFmt_P010 || m_out_pixfmt == PixFmt_P016) ? (height >> 1) : height;
  const ptrdiff_t uvWidth     = (width + 1) >> 1;

  ptrdiff_t line, i;
  __m128i xmm0,xmm1,xmm2;

  _mm_sfence();

  // Process Y
  for (line = 0; line < height; ++line) {
    const uint16_t * const y = (const uint16_t *)(src[0] + line * inYStride);
          uint16_t * const d = (      uint16_t *)(dst[0] + line * outYStride);

    for (i = 0; i < width; i+=16) {
      // Load 2x8 pixels into registers
      PIXCONV_LOAD_PIXEL16X2(xmm0, xmm1, (y+i+0), (y+i+8), m_FProps.lumabits);
      // and write them out
      PIXCONV_PUT_STREAM(d+i+0, xmm0);
      PIXCONV_PUT_STREAM(d+i+8, xmm1);
    }
  }

  // Process UV
  for (line = 0; line < uvHeight; ++line) {
    const uint16_t * const u = (const uint16_t *)(src[1] + line * inUVStride);
    const uint16_t * const v = (const uint16_t *)(src[2] + line * inUVStride);
          uint16_t * const d = (      uint16_t *)(dst[1] + line * outUVStride);

    for (i = 0; i < uvWidth; i+=8) {
      // Load 8 pixels into register
      PIXCONV_LOAD_PIXEL16X2(xmm0, xmm1, (v+i), (u+i), m_FProps.lumabits); // Load V and U

      xmm2 = xmm0;
      xmm0 = _mm_unpacklo_epi16(xmm1, xmm0);    /* UVUV */
      xmm2 = _mm_unpackhi_epi16(xmm1, xmm2);    /* UVUV */

      PIXCONV_PUT_STREAM(d + (i << 1) + 0, xmm0);
      PIXCONV_PUT_STREAM(d + (i << 1) + 8, xmm2);
    }
  }

  return S_OK;
}

// This function converts 8x2 pixels from the source into 8x2 YUY2 pixels in the destination
template <MPCPixFmtType inputFormat, int shift, int uyvy, int dithertype> __forceinline
static int yuv420yuy2_convert_pixels(const uint8_t* &srcY, const uint8_t* &srcU, const uint8_t* &srcV, uint8_t* &dst, ptrdiff_t srcStrideY, ptrdiff_t srcStrideUV, ptrdiff_t dstStride, ptrdiff_t line, const uint16_t* &dithers, ptrdiff_t pos)
{
  __m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;
  xmm7 = _mm_setzero_si128 ();

  // Shift > 0 is for 9/10 bit formats
  if (shift > 0) {
    // Load 4 U/V values from line 0/1 into registers
    PIXCONV_LOAD_4PIXEL16(xmm1, srcU);
    PIXCONV_LOAD_4PIXEL16(xmm3, srcU+srcStrideUV);
    PIXCONV_LOAD_4PIXEL16(xmm0, srcV);
    PIXCONV_LOAD_4PIXEL16(xmm2, srcV+srcStrideUV);

    // Interleave U and V
    xmm0 = _mm_unpacklo_epi16(xmm1, xmm0);                       /* 0V0U0V0U */
    xmm2 = _mm_unpacklo_epi16(xmm3, xmm2);                       /* 0V0U0V0U */
  } else if (inputFormat == PFType_NV12) {
    // Load 4 16-bit macro pixels, which contain 4 UV samples
    PIXCONV_LOAD_4PIXEL16(xmm0, srcU);
    PIXCONV_LOAD_4PIXEL16(xmm2, srcU+srcStrideUV);

    // Expand to 16-bit
    xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);                       /* 0V0U0V0U */
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm7);                       /* 0V0U0V0U */
  } else {
    PIXCONV_LOAD_4PIXEL8(xmm1, srcU);
    PIXCONV_LOAD_4PIXEL8(xmm3, srcU+srcStrideUV);
    PIXCONV_LOAD_4PIXEL8(xmm0, srcV);
    PIXCONV_LOAD_4PIXEL8(xmm2, srcV+srcStrideUV);

    // Interleave U and V
    xmm0 = _mm_unpacklo_epi8(xmm1, xmm0);                       /* VUVU0000 */
    xmm2 = _mm_unpacklo_epi8(xmm3, xmm2);                       /* VUVU0000 */

    // Expand to 16-bit
    xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);                       /* 0V0U0V0U */
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm7);                       /* 0V0U0V0U */
  }

  // xmm0/xmm2 contain 4 interleaved U/V samples from two lines each in the 16bit parts, still in their native bitdepth

  // Chroma upsampling
  if (shift > 0 || inputFormat == PFType_NV12) {
    srcU += 8;
    srcV += 8;
  } else {
    srcU += 4;
    srcV += 4;
  }

  xmm1 = xmm0;
  xmm1 = _mm_add_epi16(xmm1, xmm0);                         /* 2x line 0 */
  xmm1 = _mm_add_epi16(xmm1, xmm0);                         /* 3x line 0 */
  xmm1 = _mm_add_epi16(xmm1, xmm2);                         /* 3x line 0 + line 1 (10bit) */

  xmm3 = xmm2;
  xmm3 = _mm_add_epi16(xmm3, xmm2);                         /* 2x line 1 */
  xmm3 = _mm_add_epi16(xmm3, xmm2);                         /* 3x line 1 */
  xmm3 = _mm_add_epi16(xmm3, xmm0);                         /* 3x line 1 + line 0 (10bit) */

  // After this step, xmm1 and xmm3 contain 8 16-bit values, V and U interleaved. For 4:2:0, filling input+2 bits (10, 11, 12).
  // Load Y
  if (shift > 0) {
    // Load 8 Y values from line 0/1 into registers
    PIXCONV_LOAD_PIXEL8_ALIGNED(xmm0, srcY);
    PIXCONV_LOAD_PIXEL8_ALIGNED(xmm5, srcY+srcStrideY);

    srcY += 16;
  } else {
    PIXCONV_LOAD_4PIXEL16(xmm0, srcY);
    PIXCONV_LOAD_4PIXEL16(xmm5, srcY+srcStrideY);
    srcY += 8;

    xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);                     /* YYYYYYYY (16-bit fields)*/
    xmm5 = _mm_unpacklo_epi8(xmm5, xmm7);                     /* YYYYYYYY (16-bit fields) */
  }

  // Dither everything to 8-bit

  // Dithering
  PIXCONV_LOAD_DITHER_COEFFS(xmm6, line+0, shift+2, odithers);
  PIXCONV_LOAD_DITHER_COEFFS(xmm7, line+1, shift+2, odithers2);

  // Dither UV
  xmm1 = _mm_adds_epu16(xmm1, xmm6);
  xmm3 = _mm_adds_epu16(xmm3, xmm7);
  xmm1 = _mm_srai_epi16(xmm1, shift+2);
  xmm3 = _mm_srai_epi16(xmm3, shift+2);

  if (shift) {                                                /* Y only needs to be dithered if it was > 8 bit */
    xmm6 = _mm_srli_epi16(xmm6, 2);                           /* Shift dithering coeffs to proper strength */
    xmm7 = _mm_srli_epi16(xmm6, 2);

    xmm0 = _mm_adds_epu16(xmm0, xmm6);                        /* Apply dithering coeffs */
    xmm0 = _mm_srai_epi16(xmm0, shift);                       /* Shift to 8 bit */

    xmm5 = _mm_adds_epu16(xmm5, xmm7);                        /* Apply dithering coeffs */
    xmm5 = _mm_srai_epi16(xmm5, shift);                       /* Shift to 8 bit */
  }

  // Pack into 8-bit containers
  xmm0 = _mm_packus_epi16(xmm0, xmm5);
  xmm1 = _mm_packus_epi16(xmm1, xmm3);

  // Interleave U/V with Y
  if (uyvy) {
    xmm3 = xmm1;
    xmm3 = _mm_unpacklo_epi8(xmm3, xmm0);
    xmm4 = _mm_unpackhi_epi8(xmm1, xmm0);
  } else {
    xmm3 = xmm0;
    xmm3 = _mm_unpacklo_epi8(xmm3, xmm1);
    xmm4 = _mm_unpackhi_epi8(xmm0, xmm1);
  }

  // Write back into the target memory
  _mm_stream_si128((__m128i *)(dst), xmm3);
  _mm_stream_si128((__m128i *)(dst + dstStride), xmm4);

  dst += 16;

  return 0;
}

template <MPCPixFmtType inputFormat, int shift, int uyvy, int dithertype>
static int __stdcall yuv420yuy2_process_lines(const uint8_t *srcY, const uint8_t *srcU, const uint8_t *srcV, uint8_t *dst, int width, int height, ptrdiff_t srcStrideY, ptrdiff_t srcStrideUV, ptrdiff_t dstStride, const uint16_t *dithers)
{
  const uint8_t *y = srcY;
  const uint8_t *u = srcU;
  const uint8_t *v = srcV;
  uint8_t *yuy2 = dst;

  // Processing starts at line 1, and ends at height - 1. The first and last line have special handling
  ptrdiff_t line = 1;
  const ptrdiff_t lastLine = height - 1;

  const uint16_t *lineDither = dithers;

  _mm_sfence();

  // Process first line
  // This needs special handling because of the chroma offset of YUV420
  for (ptrdiff_t i = 0; i < width; i += 8) {
    yuv420yuy2_convert_pixels<inputFormat, shift, uyvy, dithertype>(y, u, v, yuy2, 0, 0, 0, 0, lineDither, i);
  }

  for (; line < lastLine; line += 2) {
    y = srcY + line * srcStrideY;

    u = srcU + (line >> 1) * srcStrideUV;
    v = srcV + (line >> 1) * srcStrideUV;

    yuy2 = dst + line * dstStride;

    for (int i = 0; i < width; i += 8) {
      yuv420yuy2_convert_pixels<inputFormat, shift, uyvy, dithertype>(y, u, v, yuy2, srcStrideY, srcStrideUV, dstStride, line, lineDither, i);
    }
  }

  // Process last line
  // This needs special handling because of the chroma offset of YUV420

  y = srcY + (height - 1) * srcStrideY;
  u = srcU + ((height >> 1) - 1)  * srcStrideUV;
  v = srcV + ((height >> 1) - 1)  * srcStrideUV;
  yuy2 = dst + (height - 1) * dstStride;

  for (ptrdiff_t i = 0; i < width; i += 8) {
    yuv420yuy2_convert_pixels<inputFormat, shift, uyvy, dithertype>(y, u, v, yuy2, 0, 0, 0, line, lineDither, i);
  }
  return 0;
}

template<int uyvy, int dithertype>
static int __stdcall yuv420yuy2_dispatch(MPCPixFmtType inputFormat, int bpp, const uint8_t *srcY, const uint8_t *srcU, const uint8_t *srcV, uint8_t *dst, int width, int height, ptrdiff_t srcStrideY, ptrdiff_t srcStrideUV, ptrdiff_t dstStride, const uint16_t *dithers)
{
  // Wrap the input format into template args
  switch (inputFormat) {
  case PFType_YUV420:
    return yuv420yuy2_process_lines<PFType_YUV420, 0, uyvy, dithertype>(srcY, srcU, srcV, dst, width, height, srcStrideY, srcStrideUV, dstStride, dithers);
  case PFType_NV12:
    return yuv420yuy2_process_lines<PFType_NV12, 0, uyvy, dithertype>(srcY, srcU, srcV, dst, width, height, srcStrideY, srcStrideUV, dstStride, dithers);
  case PFType_YUV420Px:
    if (bpp == 9)
      return yuv420yuy2_process_lines<PFType_YUV420, 1, uyvy, dithertype>(srcY, srcU, srcV, dst, width, height, srcStrideY, srcStrideUV, dstStride, dithers);
    else if (bpp == 10)
      return yuv420yuy2_process_lines<PFType_YUV420, 2, uyvy, dithertype>(srcY, srcU, srcV, dst, width, height, srcStrideY, srcStrideUV, dstStride, dithers);
    /*else if (bpp == 11)
      return yuv420yuy2_process_lines<LAVPixFmt_YUV420, 3, uyvy, dithertype>(srcY, srcU, srcV, dst, width, height, srcStrideY, srcStrideUV, dstStride, dithers);*/
    else if (bpp == 12)
      return yuv420yuy2_process_lines<PFType_YUV420, 4, uyvy, dithertype>(srcY, srcU, srcV, dst, width, height, srcStrideY, srcStrideUV, dstStride, dithers);
    /*else if (bpp == 13)
      return yuv420yuy2_process_lines<LAVPixFmt_YUV420, 5, uyvy, dithertype>(srcY, srcU, srcV, dst, width, height, srcStrideY, srcStrideUV, dstStride, dithers);*/
    else if (bpp == 14)
      return yuv420yuy2_process_lines<PFType_YUV420, 6, uyvy, dithertype>(srcY, srcU, srcV, dst, width, height, srcStrideY, srcStrideUV, dstStride, dithers);
    else
      ASSERT(0);
    break;
  default:
    ASSERT(0);
  }
  return 0;
}

HRESULT CFormatConverter::convert_yuv420_yuy2(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const uint16_t *dithers = NULL;
  yuv420yuy2_dispatch<0, 0>(m_FProps.pftype, m_FProps.lumabits, src[0], src[1], src[2], dst[0], width, height, srcStride[0], srcStride[1], dstStride[0], NULL);

  return S_OK;
}


HRESULT CFormatConverter::convert_yuv422_yuy2_uyvy_dither_le(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const ptrdiff_t inLumaStride    = srcStride[0];
  const ptrdiff_t inChromaStride  = srcStride[1];
  const ptrdiff_t outStride       = dstStride[0];
  const ptrdiff_t chromaWidth     = (width + 1) >> 1;

  const uint16_t *dithers = NULL;

  ptrdiff_t line,i;
  __m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;

  _mm_sfence();

  for (line = 0;  line < height; ++line) {
    const uint16_t * const y = (const uint16_t *)(src[0] + line * inLumaStride);
    const uint16_t * const u = (const uint16_t *)(src[1] + line * inChromaStride);
    const uint16_t * const v = (const uint16_t *)(src[2] + line * inChromaStride);
          uint16_t * const d = (      uint16_t *)(dst[0] + line * outStride);

    PIXCONV_LOAD_DITHER_COEFFS(xmm7,line,8,dithers);
    xmm4 = xmm5 = xmm6 = xmm7;

    for (i = 0; i < chromaWidth; i+=8) {
      // Load pixels
      PIXCONV_LOAD_PIXEL16_DITHER(xmm0, xmm4, (y+(i*2)+0), m_FProps.lumabits);  /* YYYY */
      PIXCONV_LOAD_PIXEL16_DITHER(xmm1, xmm5, (y+(i*2)+8), m_FProps.lumabits);  /* YYYY */
      PIXCONV_LOAD_PIXEL16_DITHER(xmm2, xmm6, (u+i), m_FProps.lumabits);        /* UUUU */
      PIXCONV_LOAD_PIXEL16_DITHER(xmm3, xmm7, (v+i), m_FProps.lumabits);        /* VVVV */

      // Pack Ys
      xmm0 = _mm_packus_epi16(xmm0, xmm1);

      // Interleave Us and Vs
      xmm2 = _mm_packus_epi16(xmm2, xmm2);
      xmm3 = _mm_packus_epi16(xmm3, xmm3);
      xmm2 = _mm_unpacklo_epi8(xmm2, xmm3);

      // Interlave those with the Ys
      xmm3 = xmm0;
      xmm3 = _mm_unpacklo_epi8(xmm3, xmm2);
      xmm2 = _mm_unpackhi_epi8(xmm0, xmm2);

      PIXCONV_PUT_STREAM(d + (i << 1) + 0, xmm3);
      PIXCONV_PUT_STREAM(d + (i << 1) + 8, xmm2);
    }
  }

  return S_OK;
}

HRESULT CFormatConverter::convert_yuv_yv_nv12_dither_le(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const ptrdiff_t inYStride   = srcStride[0];
  const ptrdiff_t inUVStride  = srcStride[1];

  const ptrdiff_t outYStride  = dstStride[0];
  const ptrdiff_t outUVStride = dstStride[1];

  ptrdiff_t chromaWidth       = width;
  ptrdiff_t chromaHeight      = height;

  const uint16_t *dithers = NULL;

  if (m_FProps.pftype == PFType_YUV420Px)
    chromaHeight = chromaHeight >> 1;
  if (m_FProps.pftype == PFType_YUV420Px || m_FProps.pftype == PFType_YUV422Px)
    chromaWidth = (chromaWidth + 1) >> 1;

  ptrdiff_t line, i;

  __m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;

  _mm_sfence();

  // Process Y
  for (line = 0; line < height; ++line) {
    // Load dithering coefficients for this line
    PIXCONV_LOAD_DITHER_COEFFS(xmm7,line,8,dithers);
    xmm4 = xmm5 = xmm6 = xmm7;

    const uint16_t * const y  = (const uint16_t *)(src[0] + line * inYStride);
          uint16_t * const dy = (      uint16_t *)(dst[0] + line * outYStride);

    for (i = 0; i < width; i+=32) {
      // Load pixels into registers, and apply dithering
      PIXCONV_LOAD_PIXEL16_DITHER(xmm0, xmm4, (y+i+ 0), m_FProps.lumabits);  /* Y0Y0Y0Y0 */
      PIXCONV_LOAD_PIXEL16_DITHER(xmm1, xmm5, (y+i+ 8), m_FProps.lumabits);  /* Y0Y0Y0Y0 */
      PIXCONV_LOAD_PIXEL16_DITHER(xmm2, xmm6, (y+i+16), m_FProps.lumabits);  /* Y0Y0Y0Y0 */
      PIXCONV_LOAD_PIXEL16_DITHER(xmm3, xmm7, (y+i+24), m_FProps.lumabits);  /* Y0Y0Y0Y0 */
      xmm0 = _mm_packus_epi16(xmm0, xmm1);                     /* YYYYYYYY */
      xmm2 = _mm_packus_epi16(xmm2, xmm3);                     /* YYYYYYYY */

      // Write data back
      PIXCONV_PUT_STREAM(dy + (i >> 1) + 0, xmm0);
      PIXCONV_PUT_STREAM(dy + (i >> 1) + 8, xmm2);
    }

    // Process U/V for chromaHeight lines
    if (line < chromaHeight) {
      const uint16_t * const u = (const uint16_t *)(src[1] + line * inUVStride);
      const uint16_t * const v = (const uint16_t *)(src[2] + line * inUVStride);

      uint8_t * const duv = (uint8_t *)(dst[1] + line * outUVStride);
      uint8_t * const du  = (uint8_t *)(dst[2] + line * outUVStride);
      uint8_t * const dv  = (uint8_t *)(dst[1] + line * outUVStride);

       for (i = 0; i < chromaWidth; i+=16) {
        PIXCONV_LOAD_PIXEL16_DITHER(xmm0, xmm4, (u+i+0), m_FProps.lumabits);  /* U0U0U0U0 */
        PIXCONV_LOAD_PIXEL16_DITHER(xmm1, xmm5, (u+i+8), m_FProps.lumabits);  /* U0U0U0U0 */
        PIXCONV_LOAD_PIXEL16_DITHER(xmm2, xmm6, (v+i+0), m_FProps.lumabits);  /* V0V0V0V0 */
        PIXCONV_LOAD_PIXEL16_DITHER(xmm3, xmm7, (v+i+8), m_FProps.lumabits);  /* V0V0V0V0 */

        xmm0 = _mm_packus_epi16(xmm0, xmm1);                    /* UUUUUUUU */
        xmm2 = _mm_packus_epi16(xmm2, xmm3);                    /* VVVVVVVV */
        if (m_out_pixfmt == PixFmt_NV12) {
          xmm1 = xmm0;
          xmm0 = _mm_unpacklo_epi8(xmm0, xmm2);
          xmm1 = _mm_unpackhi_epi8(xmm1, xmm2);

          PIXCONV_PUT_STREAM(duv + (i << 1) + 0, xmm0);
          PIXCONV_PUT_STREAM(duv + (i << 1) + 16, xmm1);
        } else {
          PIXCONV_PUT_STREAM(du + i, xmm0);
          PIXCONV_PUT_STREAM(dv + i, xmm2);
        }
      }
    }
  }

  return S_OK;
}

HRESULT CFormatConverter::convert_yuv_yv(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const uint8_t *y = src[0];
  const uint8_t *u = src[1];
  const uint8_t *v = src[2];

  const ptrdiff_t inLumaStride    = srcStride[0];
  const ptrdiff_t inChromaStride  = srcStride[1];

  const ptrdiff_t outLumaStride   = dstStride[0];
  const ptrdiff_t outChromaStride = dstStride[1];

  ptrdiff_t line;
  ptrdiff_t chromaWidth       = width;
  ptrdiff_t chromaHeight      = height;

  if (m_FProps.pftype == PFType_YUV420)
    chromaHeight = chromaHeight >> 1;
  if (m_FProps.pftype == PFType_YUV420 || m_FProps.pftype == PFType_YUV422)
    chromaWidth = (chromaWidth + 1) >> 1;

  // Copy planes

  _mm_sfence();

  // Y
  if ((outLumaStride % 16) == 0 && ((intptr_t)dst % 16u) == 0) {
    for(line = 0; line < height; ++line) {
      PIXCONV_MEMCPY_ALIGNED(dst[0] + outLumaStride * line, y, width);
      y += inLumaStride;
    }
  } else {
    for(line = 0; line < height; ++line) {
      memcpy(dst[0] + outLumaStride * line, y, width);
      y += inLumaStride;
    }
  }

  // U/V
  if ((outChromaStride % 16) == 0 && ((intptr_t)dst % 16u) == 0) {
    for(line = 0; line < chromaHeight; ++line) {
      PIXCONV_MEMCPY_ALIGNED_TWO(
        dst[2] + outChromaStride * line, u,
        dst[1] + outChromaStride * line, v,
        chromaWidth);
      u += inChromaStride;
      v += inChromaStride;
    }
  } else {
    for(line = 0; line < chromaHeight; ++line) {
      memcpy(dst[2] + outChromaStride * line, u, chromaWidth);
      memcpy(dst[1] + outChromaStride * line, v, chromaWidth);
      u += inChromaStride;
      v += inChromaStride;
    }
  }

  return S_OK;
}

HRESULT CFormatConverter::convert_yuv420_nv12(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const ptrdiff_t inLumaStride    = srcStride[0];
  const ptrdiff_t inChromaStride  = srcStride[1];

  const ptrdiff_t outLumaStride   = dstStride[0];
  const ptrdiff_t outChromaStride = dstStride[1];

  const ptrdiff_t chromaWidth     = (width + 1) >> 1;
  const ptrdiff_t chromaHeight    = height >> 1;

  ptrdiff_t line,i;
  __m128i xmm0,xmm1,xmm2,xmm3,xmm4;

  _mm_sfence();

  // Y
  for(line = 0; line < height; ++line) {
    PIXCONV_MEMCPY_ALIGNED(dst[0] + outLumaStride * line, src[0] + inLumaStride * line, width);
  }

  // U/V
  for(line = 0; line < chromaHeight; ++line) {
    const uint8_t * const u = src[1] + line * inChromaStride;
    const uint8_t * const v = src[2] + line * inChromaStride;
          uint8_t * const d = dst[1] + line * outChromaStride;

    for (i = 0; i < (chromaWidth - 31); i += 32) {
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
    for (; i < chromaWidth; i += 16) {
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm0, v + i);
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm1, u + i);

      xmm2 = xmm0;
      xmm0 = _mm_unpacklo_epi8(xmm1, xmm0);
      xmm2 = _mm_unpackhi_epi8(xmm1, xmm2);

      PIXCONV_PUT_STREAM(d + (i << 1) +  0, xmm0);
      PIXCONV_PUT_STREAM(d + (i << 1) + 16, xmm2);
    }
  }

  return S_OK;
}

HRESULT CFormatConverter::convert_nv12_yv12(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const ptrdiff_t inLumaStride    = srcStride[0];
  const ptrdiff_t inChromaStride  = srcStride[1];
  const ptrdiff_t outLumaStride   = dstStride[0];
  const ptrdiff_t outChromaStride = dstStride[1];
  const ptrdiff_t chromaHeight    = height >> 1;

  ptrdiff_t line, i;
  __m128i xmm0,xmm1,xmm2,xmm3,xmm7;

  xmm7 = _mm_set1_epi16(0x00FF);

  _mm_sfence();

  // Copy the y
  for (line = 0; line < height; line++) {
    PIXCONV_MEMCPY_ALIGNED(dst[0] + outLumaStride * line, src[0] + inLumaStride * line, width);
  }

  for (line = 0; line < chromaHeight; line++) {
    const uint8_t * const uv = src[1] + line * inChromaStride;
          uint8_t * const dv = dst[1] + outChromaStride * line;
          uint8_t * const du = dst[2] + outChromaStride * line;

    for (i = 0; i < width; i+=32) {
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm0, uv+i+0);
      PIXCONV_LOAD_PIXEL8_ALIGNED(xmm1, uv+i+16);
      xmm2 = xmm0;
      xmm3 = xmm1;

      // null out the high-order bytes to get the U values
      xmm0 = _mm_and_si128(xmm0, xmm7);
      xmm1 = _mm_and_si128(xmm1, xmm7);
      // right shift the V values
      xmm2 = _mm_srli_epi16(xmm2, 8);
      xmm3 = _mm_srli_epi16(xmm3, 8);
      // unpack the values
      xmm0 = _mm_packus_epi16(xmm0, xmm1);
      xmm2 = _mm_packus_epi16(xmm2, xmm3);

      PIXCONV_PUT_STREAM(du + (i>>1), xmm0);
      PIXCONV_PUT_STREAM(dv + (i>>1), xmm2);
    }
  }

  return S_OK;
}

HRESULT CFormatConverter::convert_nv12_yv12_direct_sse4(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const ptrdiff_t inStride = srcStride[0];
  const ptrdiff_t outStride = dstStride[0];
  const ptrdiff_t outChromaStride = dstStride[1];
  const ptrdiff_t chromaHeight = (height >> 1);

  const ptrdiff_t stride = min(FFALIGN(width, 64), min(inStride, outStride));

  __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm7;
  xmm7 = _mm_set1_epi16(0x00FF);

  _mm_sfence();

  ptrdiff_t line, i;

  for (line = 0; line < height; line++) {
    const uint8_t *y = (src[0] + line * inStride);
    uint8_t *dy = (dst[0] + line * outStride);
    for (i = 0; i < (stride - 63); i += 64) {
      PIXCONV_STREAM_LOAD(xmm0, y + i + 0);
      PIXCONV_STREAM_LOAD(xmm1, y + i + 16);
      PIXCONV_STREAM_LOAD(xmm2, y + i + 32);
      PIXCONV_STREAM_LOAD(xmm3, y + i + 48);

      _ReadWriteBarrier();

      PIXCONV_PUT_STREAM(dy + i + 0, xmm0);
      PIXCONV_PUT_STREAM(dy + i + 16, xmm1);
      PIXCONV_PUT_STREAM(dy + i + 32, xmm2);
      PIXCONV_PUT_STREAM(dy + i + 48, xmm3);
    }

    for (; i < width; i += 16) {
      PIXCONV_LOAD_ALIGNED(xmm0, y + i);
      PIXCONV_PUT_STREAM(dy + i, xmm0);
    }
  }

  for (line = 0; line < chromaHeight; line++) {
    const uint8_t *uv = (src[1] + line * inStride);
    uint8_t *dv = (dst[1] + line * outChromaStride);
    uint8_t *du = (dst[2] + line * outChromaStride);
    for (i = 0; i < (stride - 63); i += 64) {
      PIXCONV_STREAM_LOAD(xmm0, uv + i + 0);
      PIXCONV_STREAM_LOAD(xmm1, uv + i + 16);
      PIXCONV_STREAM_LOAD(xmm2, uv + i + 32);
      PIXCONV_STREAM_LOAD(xmm3, uv + i + 48);

      _ReadWriteBarrier();

      // process first pair
      xmm4 = _mm_srli_epi16(xmm0, 8);
      xmm5 = _mm_srli_epi16(xmm1, 8);
      xmm0 = _mm_and_si128(xmm0, xmm7);
      xmm1 = _mm_and_si128(xmm1, xmm7);
      xmm0 = _mm_packus_epi16(xmm0, xmm1);
      xmm4 = _mm_packus_epi16(xmm4, xmm5);

      PIXCONV_PUT_STREAM(du + (i >> 1) + 0, xmm0);
      PIXCONV_PUT_STREAM(dv + (i >> 1) + 0, xmm4);

      // and second pair
      xmm4 = _mm_srli_epi16(xmm2, 8);
      xmm5 = _mm_srli_epi16(xmm3, 8);
      xmm2 = _mm_and_si128(xmm2, xmm7);
      xmm3 = _mm_and_si128(xmm3, xmm7);
      xmm2 = _mm_packus_epi16(xmm2, xmm3);
      xmm4 = _mm_packus_epi16(xmm4, xmm5);

      PIXCONV_PUT_STREAM(du + (i >> 1) + 16, xmm2);
      PIXCONV_PUT_STREAM(dv + (i >> 1) + 16, xmm4);
    }

    for (; i < width; i += 32) {
      PIXCONV_LOAD_ALIGNED(xmm0, uv + i + 0);
      PIXCONV_LOAD_ALIGNED(xmm1, uv + i + 16);

      xmm4 = _mm_srli_epi16(xmm0, 8);
      xmm5 = _mm_srli_epi16(xmm1, 8);
      xmm0 = _mm_and_si128(xmm0, xmm7);
      xmm1 = _mm_and_si128(xmm1, xmm7);
      xmm0 = _mm_packus_epi16(xmm0, xmm1);
      xmm4 = _mm_packus_epi16(xmm4, xmm5);

      PIXCONV_PUT_STREAM(du + (i >> 1), xmm0);
      PIXCONV_PUT_STREAM(dv + (i >> 1), xmm4);
    }
  }

  return S_OK;
}

HRESULT CFormatConverter::plane_copy_sse2(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const SW_OUT_FMT& swof = s_sw_formats[m_out_pixfmt];

  const int widthBytes = width * swof.codedbytes;
  const int planes = max(swof.planes, 1);

  ptrdiff_t line, plane;

  for (plane = 0; plane < planes; plane++) {
    const int planeWidth = widthBytes / swof.planeWidth[plane];
    const int planeHeight = height / swof.planeHeight[plane];
    const ptrdiff_t srcPlaneStride = srcStride[plane];
    const ptrdiff_t dstPlaneStride = dstStride[plane];
    const uint8_t * const srcBuf = src[plane];
          uint8_t * const dstBuf = dst[plane];

    if ((dstPlaneStride % 16) == 0 && ((intptr_t)dstBuf % 16u) == 0) {
      for (line = 0; line < planeHeight; ++line) {
        PIXCONV_MEMCPY_ALIGNED(dstBuf + line * dstPlaneStride, srcBuf + line * srcPlaneStride, planeWidth);
      }
    } else {
      for (line = 0; line < planeHeight; ++line) {
        memcpy(dstBuf + line * dstPlaneStride, srcBuf + line * srcPlaneStride, planeWidth);
      }
    }
  }

  return S_OK;
}

// This function is only designed for NV12-like pixel formats, like NV12, P010, P016, ...
HRESULT CFormatConverter::plane_copy_direct_sse4(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const ptrdiff_t inStride     = srcStride[0];
  const ptrdiff_t outStride    = dstStride[0];
  const ptrdiff_t chromaHeight = (height >> 1);

  const ptrdiff_t byteWidth    = (m_out_pixfmt == PixFmt_P010 || m_out_pixfmt == PixFmt_P016) ? width << 1 : width;
  const ptrdiff_t stride       = min(FFALIGN(byteWidth, 64), min(inStride, outStride));

  __m128i xmm0,xmm1,xmm2,xmm3;

  _mm_sfence();

  ptrdiff_t line, i;

  for (line = 0; line < height; line++) {
    const uint8_t *y  = (src[0] + line * inStride);
          uint8_t *dy = (dst[0] + line * outStride);
    for (i = 0; i < (stride - 63); i += 64) {
      PIXCONV_STREAM_LOAD(xmm0, y + i +  0);
      PIXCONV_STREAM_LOAD(xmm1, y + i + 16);
      PIXCONV_STREAM_LOAD(xmm2, y + i + 32);
      PIXCONV_STREAM_LOAD(xmm3, y + i + 48);

      _ReadWriteBarrier();

      PIXCONV_PUT_STREAM(dy + i +  0, xmm0);
      PIXCONV_PUT_STREAM(dy + i + 16, xmm1);
      PIXCONV_PUT_STREAM(dy + i + 32, xmm2);
      PIXCONV_PUT_STREAM(dy + i + 48, xmm3);
    }

    for (; i < byteWidth; i += 16) {
      PIXCONV_LOAD_ALIGNED(xmm0, y + i);
      PIXCONV_PUT_STREAM(dy + i, xmm0);
    }
  }

  for (line = 0; line < chromaHeight; line++) {
    const uint8_t *uv  = (src[1] + line * inStride);
          uint8_t *duv = (dst[1] + line * outStride);
    for (i = 0; i < (stride - 63); i += 64) {
      PIXCONV_STREAM_LOAD(xmm0, uv + i +  0);
      PIXCONV_STREAM_LOAD(xmm1, uv + i + 16);
      PIXCONV_STREAM_LOAD(xmm2, uv + i + 32);
      PIXCONV_STREAM_LOAD(xmm3, uv + i + 48);

      _ReadWriteBarrier();

      PIXCONV_PUT_STREAM(duv + i +  0, xmm0);
      PIXCONV_PUT_STREAM(duv + i + 16, xmm1);
      PIXCONV_PUT_STREAM(duv + i + 32, xmm2);
      PIXCONV_PUT_STREAM(duv + i + 48, xmm3);
    }

    for (; i < byteWidth; i += 16) {
      PIXCONV_LOAD_ALIGNED(xmm0, uv + i);
      PIXCONV_PUT_STREAM(duv + i, xmm0);
    }
  }

  return S_OK;
}

HRESULT CFormatConverter::plane_copy(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const SW_OUT_FMT& swof = s_sw_formats[m_out_pixfmt];

  const int widthBytes = width * swof.codedbytes;
  const int planes = max(swof.planes, 1);

  ptrdiff_t line, plane;

  for (plane = 0; plane < planes; plane++) {
    const int planeWidth = widthBytes / swof.planeWidth[plane];
    const int planeHeight = height / swof.planeHeight[plane];
    const ptrdiff_t srcPlaneStride = srcStride[plane];
    const ptrdiff_t dstPlaneStride = dstStride[plane];
    const uint8_t * const srcBuf = src[plane];
          uint8_t * const dstBuf = dst[plane];

    for (line = 0; line < planeHeight; ++line) {
      memcpy(dstBuf + line * dstPlaneStride, srcBuf + line * srcPlaneStride, planeWidth);
    }
  }

  return S_OK;
}

// yuv2rgb
#pragma warning(push)
#pragma warning(disable: 4556)

// This function converts 4x2 pixels from the source into 4x2 RGB pixels in the destination
template <MPCPixFmtType inputFormat, int shift, int outFmt, int right_edge, int ycgco> __forceinline
static int yuv2rgb_convert_pixels(const uint8_t* &srcY, const uint8_t* &srcU, const uint8_t* &srcV, uint8_t* &dst, ptrdiff_t srcStrideY, ptrdiff_t srcStrideUV, ptrdiff_t dstStride, ptrdiff_t line, const RGBCoeffs *coeffs, const uint16_t* &dithers, ptrdiff_t pos)
{
  __m128i xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7;
  xmm7 = _mm_setzero_si128 ();

  // Shift > 0 is for 9/10 bit formats
  if (inputFormat == PFType_P010) {
    // Load 2 32-bit macro pixels from each line, which contain 4 UV at 16-bit each samples
    PIXCONV_LOAD_PIXEL8(xmm0, srcU);
    PIXCONV_LOAD_PIXEL8(xmm2, srcU+srcStrideUV);
  } else if (shift > 0) {
    // Load 4 U/V values from line 0/1 into registers
    PIXCONV_LOAD_4PIXEL16(xmm1, srcU);
    PIXCONV_LOAD_4PIXEL16(xmm3, srcU+srcStrideUV);
    PIXCONV_LOAD_4PIXEL16(xmm0, srcV);
    PIXCONV_LOAD_4PIXEL16(xmm2, srcV+srcStrideUV);

    // Interleave U and V
    xmm0 = _mm_unpacklo_epi16(xmm1, xmm0);                       /* 0V0U0V0U */
    xmm2 = _mm_unpacklo_epi16(xmm3, xmm2);                       /* 0V0U0V0U */
  } else if (inputFormat == PFType_NV12) {
    // Load 4 16-bit macro pixels, which contain 4 UV samples
    PIXCONV_LOAD_4PIXEL16(xmm0, srcU);
    PIXCONV_LOAD_4PIXEL16(xmm2, srcU+srcStrideUV);

    // Expand to 16-bit
    xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);                       /* 0V0U0V0U */
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm7);                       /* 0V0U0V0U */
  } else {
    PIXCONV_LOAD_4PIXEL8(xmm1, srcU);
    PIXCONV_LOAD_4PIXEL8(xmm3, srcU+srcStrideUV);
    PIXCONV_LOAD_4PIXEL8(xmm0, srcV);
    PIXCONV_LOAD_4PIXEL8(xmm2, srcV+srcStrideUV);

    // Interleave U and V
    xmm0 = _mm_unpacklo_epi8(xmm1, xmm0);                       /* VUVU0000 */
    xmm2 = _mm_unpacklo_epi8(xmm3, xmm2);                       /* VUVU0000 */

    // Expand to 16-bit
    xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);                       /* 0V0U0V0U */
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm7);                       /* 0V0U0V0U */
  }

  // xmm0/xmm2 contain 4 interleaved U/V samples from two lines each in the 16bit parts, still in their native bitdepth

  // Chroma upsampling required
  if (inputFormat == PFType_YUV420 || inputFormat == PFType_NV12 || inputFormat == PFType_YUV422 || inputFormat == PFType_P010) {
    if (inputFormat == PFType_P010) {
      srcU += 8;
      srcV += 8;
    } else if (shift > 0 || inputFormat == PFType_NV12) {
      srcU += 4;
      srcV += 4;
    } else {
      srcU += 2;
      srcV += 2;
    }

    // Cut off the over-read into the stride and replace it with the last valid pixel
    if (right_edge) {
      xmm6 = _mm_set_epi32(0, 0xffffffff, 0, 0);

      // First line
      xmm1 = xmm0;
      xmm1 = _mm_slli_si128(xmm1, 4);
      xmm1 = _mm_and_si128(xmm1, xmm6);
      xmm0 = _mm_andnot_si128(xmm6, xmm0);
      xmm0 = _mm_or_si128(xmm0, xmm1);

      // Second line
      xmm3 = xmm2;
      xmm3 = _mm_slli_si128(xmm3, 4);
      xmm3 = _mm_and_si128(xmm3, xmm6);
      xmm2 = _mm_andnot_si128(xmm6, xmm2);
      xmm2 = _mm_or_si128(xmm2, xmm3);
    }

    // 4:2:0 - upsample to 4:2:2 using 75:25
    if (inputFormat == PFType_YUV420 || inputFormat == PFType_NV12 || inputFormat == PFType_P010) {
      // Too high bitdepth, shift down to 14-bit
      if (shift >= 7) {
        xmm0 = _mm_srli_epi16(xmm0, shift-6);
        xmm2 = _mm_srli_epi16(xmm2, shift-6);
      }
      xmm1 = xmm0;
      xmm1 = _mm_add_epi16(xmm1, xmm0);                         /* 2x line 0 */
      xmm1 = _mm_add_epi16(xmm1, xmm0);                         /* 3x line 0 */
      xmm1 = _mm_add_epi16(xmm1, xmm2);                         /* 3x line 0 + line 1 (10bit) */

      xmm3 = xmm2;
      xmm3 = _mm_add_epi16(xmm3, xmm2);                         /* 2x line 1 */
      xmm3 = _mm_add_epi16(xmm3, xmm2);                         /* 3x line 1 */
      xmm3 = _mm_add_epi16(xmm3, xmm0);                         /* 3x line 1 + line 0 (10bit) */

      // If the bit depth is too high, we need to reduce it here (max 15bit)
      // 14-16 bits need the reduction, because they all result in a 16-bit result
      if (shift >= 6) {
        xmm1 = _mm_srli_epi16(xmm1, 1);
        xmm3 = _mm_srli_epi16(xmm3, 1);
      }
    } else {
      xmm1 = xmm0;
      xmm3 = xmm2;

      // Shift to maximum of 15-bit, if required
      if (shift >= 8) {
        xmm1 = _mm_srli_epi16(xmm1, 1);
        xmm3 = _mm_srli_epi16(xmm3, 1);
      }
    }
    // After this step, xmm1 and xmm3 contain 8 16-bit values, V and U interleaved. For 4:2:2, filling 8 to 15 bits (original bit depth). For 4:2:0, filling input+2 bits (10 to 15).

    // Upsample to 4:4:4 using 100:0, 50:50, 0:100 scheme (MPEG2 chroma siting)
    // TODO: MPEG1 chroma siting, use 75:25

    xmm0 = xmm1;                                               /* UV UV UV UV */
    xmm0 = _mm_unpacklo_epi32(xmm0, xmm7);                     /* UV 00 UV 00 */
    xmm1 = _mm_srli_si128(xmm1, 4);                            /* UV UV UV 00 */
    xmm1 = _mm_unpacklo_epi32(xmm7, xmm1);                     /* 00 UV 00 UV */

    xmm1 = _mm_add_epi16(xmm1, xmm0);                         /*  UV  UV  UV  UV */
    xmm1 = _mm_add_epi16(xmm1, xmm0);                         /* 2UV  UV 2UV  UV */

    xmm0 = _mm_slli_si128(xmm0, 4);                            /*  00  UV  00  UV */
    xmm1 = _mm_add_epi16(xmm1, xmm0);                         /* 2UV 2UV 2UV 2UV */

    // Same for the second row
    xmm2 = xmm3;                                               /* UV UV UV UV */
    xmm2 = _mm_unpacklo_epi32(xmm2, xmm7);                     /* UV 00 UV 00 */
    xmm3 = _mm_srli_si128(xmm3, 4);                            /* UV UV UV 00 */
    xmm3 = _mm_unpacklo_epi32(xmm7, xmm3);                     /* 00 UV 00 UV */

    xmm3 = _mm_add_epi16(xmm3, xmm2);                         /*  UV  UV  UV  UV */
    xmm3 = _mm_add_epi16(xmm3, xmm2);                         /* 2UV  UV 2UV  UV */

    xmm2 = _mm_slli_si128(xmm2, 4);                            /*  00  UV  00  UV */
    xmm3 = _mm_add_epi16(xmm3, xmm2);                         /* 2UV 2UV 2UV 2UV */

    // Shift the result to 12 bit
    // For 10-bit input, we need to shift one bit off, or we exceed the allowed processing depth
    // For 8-bit, we need to add one bit
    if ((inputFormat == PFType_YUV420 && shift > 1) || inputFormat == PFType_P010) {
      if (shift >= 5) {
        xmm1 = _mm_srli_epi16(xmm1, 4);
        xmm3 = _mm_srli_epi16(xmm3, 4);
      } else {
        xmm1 = _mm_srli_epi16(xmm1, shift-1);
        xmm3 = _mm_srli_epi16(xmm3, shift-1);
      }
    } else if (inputFormat == PFType_YUV422) {
      if (shift >= 7) {
        xmm1 = _mm_srli_epi16(xmm1, 4);
        xmm3 = _mm_srli_epi16(xmm3, 4);
      } else if (shift > 3) {
        xmm1 = _mm_srli_epi16(xmm1, shift-3);
        xmm3 = _mm_srli_epi16(xmm3, shift-3);
      } else if (shift < 3) {
        xmm1 = _mm_slli_epi16(xmm1, 3-shift);
        xmm3 = _mm_slli_epi16(xmm3, 3-shift);
      }
    } else if ((inputFormat == PFType_YUV420 && shift == 0) || inputFormat == PFType_NV12) {
      xmm1 = _mm_slli_epi16(xmm1, 1);
      xmm3 = _mm_slli_epi16(xmm3, 1);
    }

    // 12-bit result, xmm1 & xmm3 with 4 UV combinations each
  } else if (inputFormat == PFType_YUV444) {
    if (shift > 0) {
      srcU += 8;
      srcV += 8;
    } else {
      srcU += 4;
      srcV += 4;
    }
    // Shift to 12 bit
    if (shift > 4) {
      xmm1 = _mm_srli_epi16(xmm0, shift-4);
      xmm3 = _mm_srli_epi16(xmm2, shift-4);
    } else if (shift < 4) {
      xmm1 = _mm_slli_epi16(xmm0, 4-shift);
      xmm3 = _mm_slli_epi16(xmm2, 4-shift);
    } else {
      xmm1 = xmm0;
      xmm3 = xmm2;
    }
  }

  // Load Y
  if (shift > 0) {
    // Load 4 Y values from line 0/1 into registers
    PIXCONV_LOAD_4PIXEL16(xmm5, srcY);
    PIXCONV_LOAD_4PIXEL16(xmm0, srcY+srcStrideY);

    srcY += 8;
  } else {
    PIXCONV_LOAD_4PIXEL8(xmm5, srcY);
    PIXCONV_LOAD_4PIXEL8(xmm0, srcY+srcStrideY);
    srcY += 4;

    xmm5 = _mm_unpacklo_epi8(xmm5, xmm7);                       /* YYYY0000 (16-bit fields) */
    xmm0 = _mm_unpacklo_epi8(xmm0, xmm7);                       /* YYYY0000 (16-bit fields)*/
  }

  xmm0 = _mm_unpacklo_epi64(xmm0, xmm5);                        /* YYYYYYYY */

  // After this step, xmm1 & xmm3 contain 4 UV pairs, each in a 16-bit value, filling 12-bit.
  if (!ycgco) {
    // YCbCr conversion
    // Shift Y to 14 bits
    if (shift < 6) {
      xmm0 = _mm_slli_epi16(xmm0, 6-shift);
    } else if (shift > 6) {
      xmm0 = _mm_srli_epi16(xmm0, shift-6);
    }
    xmm0 = _mm_subs_epu16(xmm0, coeffs->Ysub);                  /* Y-16 (in case of range expansion) */
    xmm0 = _mm_mulhi_epi16(xmm0, coeffs->cy);                   /* Y*cy (result is 28 bits, with 12 high-bits packed into the result) */
    xmm0 = _mm_add_epi16(xmm0, coeffs->rgb_add);                /* Y*cy + 16 (in case of range compression) */

    xmm1 = _mm_subs_epi16(xmm1, coeffs->CbCr_center);           /* move CbCr to proper range */
    xmm3 = _mm_subs_epi16(xmm3, coeffs->CbCr_center);

    xmm6 = xmm1;
    xmm4 = xmm3;
    xmm6 = _mm_madd_epi16(xmm6, coeffs->cR_Cr);                 /* Result is 25 bits (12 from chroma, 13 from coeff) */
    xmm4 = _mm_madd_epi16(xmm4, coeffs->cR_Cr);
    xmm6 = _mm_srai_epi32(xmm6, 13);                            /* Reduce to 12 bit */
    xmm4 = _mm_srai_epi32(xmm4, 13);
    xmm6 = _mm_packs_epi32(xmm6, xmm7);                         /* Pack back into 16 bit cells */
    xmm4 = _mm_packs_epi32(xmm4, xmm7);
    xmm6 = _mm_unpacklo_epi64(xmm4, xmm6);                      /* Interleave both parts */
    xmm6 = _mm_add_epi16(xmm6, xmm0);                           /* R (12bit) */

    xmm5 = xmm1;
    xmm4 = xmm3;
    xmm5 = _mm_madd_epi16(xmm5, coeffs->cG_Cb_cG_Cr);           /* Result is 25 bits (12 from chroma, 13 from coeff) */
    xmm4 = _mm_madd_epi16(xmm4, coeffs->cG_Cb_cG_Cr);
    xmm5 = _mm_srai_epi32(xmm5, 13);                            /* Reduce to 12 bit */
    xmm4 = _mm_srai_epi32(xmm4, 13);
    xmm5 = _mm_packs_epi32(xmm5, xmm7);                         /* Pack back into 16 bit cells */
    xmm4 = _mm_packs_epi32(xmm4, xmm7);
    xmm5 = _mm_unpacklo_epi64(xmm4, xmm5);                      /* Interleave both parts */
    xmm5 = _mm_add_epi16(xmm5, xmm0);                           /* G (12bit) */

    xmm1 = _mm_madd_epi16(xmm1, coeffs->cB_Cb);                 /* Result is 25 bits (12 from chroma, 13 from coeff) */
    xmm3 = _mm_madd_epi16(xmm3, coeffs->cB_Cb);
    xmm1 = _mm_srai_epi32(xmm1, 13);                            /* Reduce to 12 bit */
    xmm3 = _mm_srai_epi32(xmm3, 13);
    xmm1 = _mm_packs_epi32(xmm1, xmm7);                         /* Pack back into 16 bit cells */
    xmm3 = _mm_packs_epi32(xmm3, xmm7);
    xmm1 = _mm_unpacklo_epi64(xmm3, xmm1);                      /* Interleave both parts */
    xmm1 = _mm_add_epi16(xmm1, xmm0);                           /* B (12bit) */
  } else {
    // YCgCo conversion
    // Shift Y to 12 bits
    if (shift < 4) {
      xmm0 = _mm_slli_epi16(xmm0, 4-shift);
    } else if (shift > 4) {
      xmm0 = _mm_srli_epi16(xmm0, shift-4);
    }

    xmm7 = _mm_set1_epi32(0x0000FFFF);
    xmm2 = xmm1;
    xmm4 = xmm3;

    xmm1 = _mm_and_si128(xmm1, xmm7);                          /* null out the high-order bytes to get the Cg values */
    xmm4 = _mm_and_si128(xmm4, xmm7);

    xmm3 = _mm_srli_epi32(xmm3, 16);                           /* right shift the Co values */
    xmm2 = _mm_srli_epi32(xmm2, 16);

    xmm1 = _mm_packs_epi32(xmm4, xmm1);                       /* Pack Cg into xmm1 */
    xmm3 = _mm_packs_epi32(xmm3, xmm2);                       /* Pack Co into xmm3 */

    xmm2 = coeffs->CbCr_center;                               /* move CgCo to proper range */
    xmm1 = _mm_subs_epi16(xmm1, xmm2);
    xmm3 = _mm_subs_epi16(xmm3, xmm2);

    xmm2 = xmm0;
    xmm2 = _mm_subs_epi16(xmm2, xmm1);                         /* tmp = Y - Cg */
    xmm6 = _mm_adds_epi16(xmm2, xmm3);                         /* R = tmp + Co */
    xmm5 = _mm_adds_epi16(xmm0, xmm1);                         /* G = Y + Cg */
    xmm1 = _mm_subs_epi16(xmm2, xmm3);                         /* B = tmp - Co */
  }

  // Dithering
  /* Load dithering coeffs and combine them for two lines */
  const uint16_t *d1 = dither_8x8_256[line % 8];
  xmm2 = _mm_load_si128((const __m128i *)d1);
  const uint16_t *d2 = dither_8x8_256[(line+1) % 8];
  xmm3 = _mm_load_si128((const __m128i *)d2);

  xmm4 = xmm2;
  xmm2 = _mm_unpacklo_epi64(xmm2, xmm3);
  xmm4 = _mm_unpackhi_epi64(xmm4, xmm3);
  xmm2 = _mm_srli_epi16(xmm2, 4);
  xmm4 = _mm_srli_epi16(xmm4, 4);

  xmm3 = xmm4;

  xmm6 = _mm_adds_epu16(xmm6, xmm2);                          /* Apply coefficients to the RGB values */
  xmm5 = _mm_adds_epu16(xmm5, xmm3);
  xmm1 = _mm_adds_epu16(xmm1, xmm4);

  xmm6 = _mm_srai_epi16(xmm6, 4);                             /* Shift to 8 bit */
  xmm5 = _mm_srai_epi16(xmm5, 4);
  xmm1 = _mm_srai_epi16(xmm1, 4);

  xmm2 = _mm_cmpeq_epi8(xmm2, xmm2);                          /* 0xffffffff,0xffffffff,0xffffffff,0xffffffff */
  xmm6 = _mm_packus_epi16(xmm6, xmm7);                        /* R (lower 8bytes,8bit) * 8 */
  xmm5 = _mm_packus_epi16(xmm5, xmm7);                        /* G (lower 8bytes,8bit) * 8 */
  xmm1 = _mm_packus_epi16(xmm1, xmm7);                        /* B (lower 8bytes,8bit) * 8 */

  xmm6 = _mm_unpacklo_epi8(xmm6,xmm2); // 0xff,R
  xmm1 = _mm_unpacklo_epi8(xmm1,xmm5); // G,B
  xmm2 = xmm1;

  xmm1 = _mm_unpackhi_epi16(xmm1, xmm6); // 0xff,RGB * 4 (line 0)
  xmm2 = _mm_unpacklo_epi16(xmm2, xmm6); // 0xff,RGB * 4 (line 1)

  // TODO: RGB limiting

  if (outFmt == 1) {
    _mm_stream_si128((__m128i *)(dst), xmm1);
    _mm_stream_si128((__m128i *)(dst + dstStride), xmm2);
    dst += 16;
  } else {
    // RGB 24 output is terribly inefficient due to the un-aligned size of 3 bytes per pixel
    uint32_t eax;
    DECLARE_ALIGNED(16, uint8_t, rgbbuf)[32];
    *(uint32_t *)rgbbuf = _mm_cvtsi128_si32(xmm1);
    xmm1 = _mm_srli_si128(xmm1, 4);
    *(uint32_t *)(rgbbuf+3) = _mm_cvtsi128_si32 (xmm1);
    xmm1 = _mm_srli_si128(xmm1, 4);
    *(uint32_t *)(rgbbuf+6) = _mm_cvtsi128_si32 (xmm1);
    xmm1 = _mm_srli_si128(xmm1, 4);
    *(uint32_t *)(rgbbuf+9) = _mm_cvtsi128_si32 (xmm1);

    *(uint32_t *)(rgbbuf+16) = _mm_cvtsi128_si32 (xmm2);
    xmm2 = _mm_srli_si128(xmm2, 4);
    *(uint32_t *)(rgbbuf+19) = _mm_cvtsi128_si32 (xmm2);
    xmm2 = _mm_srli_si128(xmm2, 4);
    *(uint32_t *)(rgbbuf+22) = _mm_cvtsi128_si32 (xmm2);
    xmm2 = _mm_srli_si128(xmm2, 4);
    *(uint32_t *)(rgbbuf+25) = _mm_cvtsi128_si32 (xmm2);

    xmm1 = _mm_loadl_epi64((const __m128i *)(rgbbuf));
    xmm2 = _mm_loadl_epi64((const __m128i *)(rgbbuf+16));

    _mm_storel_epi64((__m128i *)(dst), xmm1);
    eax = *(uint32_t *)(rgbbuf + 8);
    *(uint32_t *)(dst + 8) = eax;

    _mm_storel_epi64((__m128i *)(dst + dstStride), xmm2);
    eax = *(uint32_t *)(rgbbuf + 24);
    *(uint32_t *)(dst + dstStride + 8) = eax;

    dst += 12;
  }

  return 0;
}

template <MPCPixFmtType inputFormat, int shift, int outFmt, int ycgco>
static int __stdcall yuv2rgb_convert(const uint8_t *srcY, const uint8_t *srcU, const uint8_t *srcV, uint8_t *dst, int width, int height, ptrdiff_t srcStrideY, ptrdiff_t srcStrideUV, ptrdiff_t dstStride, ptrdiff_t sliceYStart, ptrdiff_t sliceYEnd, const RGBCoeffs *coeffs, const uint16_t *dithers)
{
  const uint8_t *y = srcY;
  const uint8_t *u = srcU;
  const uint8_t *v = srcV;
  uint8_t *rgb = dst;

  ptrdiff_t line = sliceYStart;
  ptrdiff_t lastLine = sliceYEnd;
  bool lastLineInOddHeight = false;

  const ptrdiff_t endx = width - 4;

  const uint16_t *lineDither = dithers;

  _mm_sfence();

  // 4:2:0 needs special handling for the first and the last line
  if (inputFormat == PFType_YUV420 || inputFormat == PFType_NV12 || inputFormat == PFType_P010) {
    if (line == 0) {
      for (ptrdiff_t i = 0; i < endx; i += 4) {
        yuv2rgb_convert_pixels<inputFormat, shift, outFmt, 0, ycgco>(y, u, v, rgb, 0, 0, 0, line, coeffs, lineDither, i);
      }
      yuv2rgb_convert_pixels<inputFormat, shift, outFmt, 1, ycgco>(y, u, v, rgb, 0, 0, 0, line, coeffs, lineDither, 0);

      line = 1;
    }
    if (lastLine == height)
      lastLine--;
  } else if (lastLine == height && (lastLine & 1)) {
    lastLine--;
    lastLineInOddHeight = true;
  }

  for (; line < lastLine; line += 2) {
    y = srcY + line * srcStrideY;

    if (inputFormat == PFType_YUV420 || inputFormat == PFType_NV12 || inputFormat == PFType_P010) {
      u = srcU + (line >> 1) * srcStrideUV;
      v = srcV + (line >> 1) * srcStrideUV;
    } else {
      u = srcU + line * srcStrideUV;
      v = srcV + line * srcStrideUV;
    }

    rgb = dst + line * dstStride;

    for (ptrdiff_t i = 0; i < endx; i += 4) {
      yuv2rgb_convert_pixels<inputFormat, shift, outFmt, 0, ycgco>(y, u, v, rgb, srcStrideY, srcStrideUV, dstStride, line, coeffs, lineDither, i);
    }
    yuv2rgb_convert_pixels<inputFormat, shift, outFmt, 1, ycgco>(y, u, v, rgb, srcStrideY, srcStrideUV, dstStride, line, coeffs, lineDither, 0);
  }

  if (inputFormat == PFType_YUV420 || inputFormat == PFType_NV12 || inputFormat == PFType_P010 || lastLineInOddHeight) {
    if (sliceYEnd == height) {
      y = srcY + (height - 1) * srcStrideY;
      if (inputFormat == PFType_YUV420 || inputFormat == PFType_NV12 || inputFormat == PFType_P010) {
        u = srcU + ((height >> 1) - 1)  * srcStrideUV;
        v = srcV + ((height >> 1) - 1)  * srcStrideUV;
      } else {
        u = srcU + (height - 1)  * srcStrideUV;
        v = srcV + (height - 1)  * srcStrideUV;
      }
      rgb = dst + (height - 1) * dstStride;

      for (ptrdiff_t i = 0; i < endx; i += 4) {
        yuv2rgb_convert_pixels<inputFormat, shift, outFmt, 0, ycgco>(y, u, v, rgb, 0, 0, 0, line, coeffs, lineDither, i);
      }
      yuv2rgb_convert_pixels<inputFormat, shift, outFmt, 1, ycgco>(y, u, v, rgb, 0, 0, 0, line, coeffs, lineDither, 0);
    }
  }
  return 0;
}

HRESULT CFormatConverter::convert_yuv_rgb(const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[])
{
  const RGBCoeffs *coeffs = getRGBCoeffs(width, height);
  if (coeffs == nullptr)
    return E_OUTOFMEMORY;

  if (!m_bRGBConvInit) {
    m_bRGBConvInit = TRUE;
    InitRGBConvDispatcher();
  }

  BOOL bYCgCo = m_FProps.colorspace == AVCOL_SPC_YCGCO;
  int shift = max(m_FProps.lumabits - 8, 0);
  ASSERT(shift >= 0 && shift <= 8);

  const int outFmt = 1; // RGB32
  const uint16_t *dithers = nullptr;

  // Map the bX formats to their normal counter part, the shift parameter controls this now
  MPCPixFmtType inputFormat = m_FProps.pftype;
  switch (inputFormat) {
  case PFType_YUV420Px:
    inputFormat = PFType_YUV420;
    break;
  case PFType_YUV422Px:
    inputFormat = PFType_YUV422;
    break;
  case PFType_YUV444Px:
    inputFormat = PFType_YUV444;
    break;
  }

  // P010 has the data in the high bits, so set shift appropriately
  if (inputFormat == PFType_P010)
    shift = 8;

  YUVRGBConversionFunc convFn = m_RGBConvFuncs[outFmt][0][bYCgCo][inputFormat][shift];
  if (convFn == nullptr) {
    ASSERT(0);
    return E_FAIL;
  }

  // run conversion, threaded
  if (m_NumThreads <= 1) {
    convFn(src[0], src[1], src[2], dst[0], width, height, srcStride[0], srcStride[1], dstStride[0], 0, height, coeffs, dithers);
  } else {
    const int is_odd = (inputFormat == PFType_YUV420 || inputFormat == PFType_NV12 || inputFormat == PFType_P010);
    const ptrdiff_t lines_per_thread = (height / m_NumThreads)&~1;

    Concurrency::parallel_for(0, m_NumThreads, [&](int i) {
      const ptrdiff_t starty = (i * lines_per_thread);
      const ptrdiff_t endy = (i == (m_NumThreads - 1)) ? height : starty + lines_per_thread + is_odd;
      convFn(src[0], src[1], src[2], dst[0], width, height, srcStride[0], srcStride[1], dstStride[0], starty + (i ? is_odd : 0), endy, coeffs, dithers);
    });
  }

  return S_OK;
}

#define CONV_FUNC_INT2(out32, ycgco, format, shift) \
  m_RGBConvFuncs[out32][0][ycgco][format][shift] = yuv2rgb_convert<format, shift, out32, ycgco>;

#define CONV_FUNC_INT(ycgco, format, shift) \
  CONV_FUNC_INT2(0, ycgco, format, shift)   \
  CONV_FUNC_INT2(1, ycgco, format, shift)   \

#define CONV_FUNC(format, shift)  \
  CONV_FUNC_INT(0, format, shift) \
  CONV_FUNC_INT(1, format, shift) \

#define CONV_FUNCX(format)   \
  CONV_FUNC(format, 0)       \
  CONV_FUNC(format, 1)       \
  CONV_FUNC(format, 2)       \
  /* CONV_FUNC(format, 3) */ \
  CONV_FUNC(format, 4)       \
  /* CONV_FUNC(format, 5) */ \
  CONV_FUNC(format, 6)       \
  /* CONV_FUNC(format, 7) */ \
  CONV_FUNC(format, 8)

void CFormatConverter::InitRGBConvDispatcher()
{
  ZeroMemory(&m_RGBConvFuncs, sizeof(m_RGBConvFuncs));

  CONV_FUNC(PFType_NV12, 0);
  CONV_FUNC(PFType_P010, 8);

  CONV_FUNCX(PFType_YUV420);
  CONV_FUNCX(PFType_YUV422);
  CONV_FUNCX(PFType_YUV444);
}

const RGBCoeffs* CFormatConverter::getRGBCoeffs(int width, int height)
{
  if (!m_rgbCoeffs || width != m_swsWidth || height != m_swsHeight) {
    m_swsWidth = width;
    m_swsHeight = height;

    if (!m_rgbCoeffs) {
      m_rgbCoeffs = (RGBCoeffs *)_aligned_malloc(sizeof(RGBCoeffs), 16);
      if (m_rgbCoeffs == nullptr)
        return nullptr;
    }

    BOOL inFullRange = (m_FProps.colorrange == AVCOL_RANGE_JPEG || m_FProps.avpixfmt == AV_PIX_FMT_YUVJ420P || m_FProps.avpixfmt == AV_PIX_FMT_YUVJ422P || m_FProps.avpixfmt == AV_PIX_FMT_YUVJ444P);
    BOOL outFullRange = m_dstRGBRange;

    int inputWhite, inputBlack, inputChroma, outputWhite, outputBlack;
    if (inFullRange) {
      inputWhite = 255;
      inputBlack = 0;
      inputChroma = 1;
    } else {
      inputWhite = 235;
      inputBlack = 16;
      inputChroma = 16;
    }

    if (outFullRange) {
      outputWhite = 255;
      outputBlack = 0;
    } else {
      outputWhite = 235;
      outputBlack = 16;
    }

    double Kr, Kg, Kb;
    switch (m_FProps.colorspace) {
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
      Kr = 0.299;
      Kg = 0.587;
      Kb = 0.114;
      break;
    case AVCOL_SPC_SMPTE240M:
      Kr = 0.2120;
      Kg = 0.7010;
      Kb = 0.0870;
      break;
    case AVCOL_SPC_YCGCO:
      Kr = 0.300;
      Kg = 0.590;
      Kb = 0.110;
      break;
	case AVCOL_SPC_BT2020_CL:
	case AVCOL_SPC_BT2020_NCL:
      Kr = 0.2627;
      Kg = 0.6780;
      Kb = 0.0593;
      break;
    default:
      DLog(L"CFormatConverter()::getRGBCoeffs(): Unknown color space: %d - defaulting to BT709", m_FProps.colorspace);
    case AVCOL_SPC_BT709:
      Kr = 0.2126;
      Kg = 0.7152;
      Kb = 0.0722;
      break;
    }

    double in_y_range = inputWhite - inputBlack;
    double chr_range = 128 - inputChroma;

    double cspOptionsRGBrange = outputWhite - outputBlack;

    double y_mul, vr_mul, ug_mul, vg_mul, ub_mul;
    y_mul  = cspOptionsRGBrange / in_y_range;
    vr_mul = (cspOptionsRGBrange / chr_range) * (1.0 - Kr);
    ug_mul = (cspOptionsRGBrange / chr_range) * (1.0 - Kb) * Kb / Kg;
    vg_mul = (cspOptionsRGBrange / chr_range) * (1.0 - Kr) * Kr / Kg;
    ub_mul = (cspOptionsRGBrange / chr_range) * (1.0 - Kb);
    short sub = min(outputBlack, inputBlack);
    short Ysub = inputBlack - sub;
    short RGB_add1 = outputBlack - sub;

    short cy  = short(y_mul * 16384 + 0.5);
    short crv = short(vr_mul * 8192 + 0.5);
    short cgu = short(-ug_mul * 8192 - 0.5);
    short cgv = short(-vg_mul * 8192 - 0.5);
    short cbu = short(ub_mul * 8192 + 0.5);

    m_rgbCoeffs->Ysub        = _mm_set1_epi16(Ysub << 6);
    m_rgbCoeffs->cy          = _mm_set1_epi16(cy);
    m_rgbCoeffs->CbCr_center = _mm_set1_epi16(128 << 4);

    m_rgbCoeffs->cR_Cr       = _mm_set1_epi32(crv << 16);         // R
    m_rgbCoeffs->cG_Cb_cG_Cr = _mm_set1_epi32((cgv << 16) + cgu); // G
    m_rgbCoeffs->cB_Cb       = _mm_set1_epi32(cbu);               // B

    m_rgbCoeffs->rgb_add     = _mm_set1_epi16(RGB_add1 << 4);

    // YCgCo
    if (m_FProps.colorspace == AVCOL_SPC_YCGCO) {
      m_rgbCoeffs->CbCr_center = _mm_set1_epi16(0x0800);
      // Other Coeffs are not used in YCgCo
    }

  }
  return m_rgbCoeffs;
}

#pragma warning(pop)
