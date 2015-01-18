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
 *  Adaptation for MPC-BE (C) 2013 v0lt & Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
 *
 */

#include "stdafx.h"
#include "FormatConverter.h"
#include "pixconv_sse2_templates.h"

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
  if (shift > 0) {
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
