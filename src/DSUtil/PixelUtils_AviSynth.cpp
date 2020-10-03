/*
 * (C) 2020 see Authors.txt
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

#include "stdafx.h"
#include <cassert>
#include "PixelUtils_AviSynth.h"

#ifndef __SSE2__
#define __SSE2__ 0
#endif

// https://github.com/AviSynth/AviSynthPlus/blob/master/avs_core/include/avisynth.h
// commit e2c86d843a4350dee6b830d4e7e4c7607b6b4a2c on 2020-06-15

#define AVS_UNUSED(x) (void)(x)

// end code from AviSynthPlus/avs_core/include/avisynth.h


// https://github.com/AviSynth/AviSynthPlus/blob/master/avs_core/include/avs/config.h
// commit 180e44512fca411b09b5d99a458a9eb675e0e9d0 on 2020-06-15

#if   defined(_M_AMD64) || defined(__x86_64)
#   define X86_64
#elif defined(_M_IX86) || defined(__i386__)
#   define X86_32
// VS2017 introduced _M_ARM64
#elif defined(_M_ARM64) || defined(__aarch64__)
#   define ARM64
#elif defined(_M_ARM) || defined(__arm__)
#   define ARM32
#else
#   error Unsupported CPU architecture.
#endif

//            VC++  LLVM-Clang-cl   MinGW-Gnu
// MSVC        x          x
// MSVC_PURE   x
// CLANG                  x
// GCC                                  x

#if defined(__clang__)
// Check clang first. clang-cl also defines __MSC_VER
// We set MSVC because they are mostly compatible
#   define CLANG
#if defined(_MSC_VER)
#   define MSVC
#   define AVS_FORCEINLINE __attribute__((always_inline))
#else
#   define AVS_FORCEINLINE __attribute__((always_inline)) inline
#endif
#elif   defined(_MSC_VER)
#   define MSVC
#   define MSVC_PURE
#   define AVS_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#   define GCC
#   define AVS_FORCEINLINE __attribute__((always_inline)) inline
#else
#   error Unsupported compiler.
#   define AVS_FORCEINLINE inline
#   undef __forceinline
#   define __forceinline inline
#endif

#if defined(_WIN32)
#   define AVS_WINDOWS
#elif defined(__linux__)
#   define AVS_LINUX
#   define AVS_POSIX
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#   define AVS_BSD
#   define AVS_POSIX
#elif defined(__APPLE__)
#   define AVS_MACOS
#   define AVS_POSIX
#else
#   error Operating system unsupported.
#endif

// end code from AviSynthPlus/avs_core/include/avs/config.h



// https://github.com/AviSynth/AviSynthPlus/blob/master/avs_core/include/avs/alignment.h
// commit c1e9f3929471e8da15088b733577e5f23b8c7dcb on 2020-03-03

// Functions and macros to help work with alignment requirements.

// Tells if a number is a power of two.
#define IS_POWER2(n) ((n) && !((n) & ((n) - 1)))

// Tells if the pointer "ptr" is aligned to "align" bytes.
#define IS_PTR_ALIGNED(ptr, align) (((uintptr_t)ptr & ((uintptr_t)(align-1))) == 0)

template<typename T>
static bool IsPtrAligned(T* ptr, size_t align)
{
  assert(IS_POWER2(align));
  return (bool)IS_PTR_ALIGNED(ptr, align);
}

// https://github.com/AviSynth/AviSynthPlus/blob/master/avs_core/convert/intel/convert_yv12_sse.cpp
// commit b287f15beb44293b9efc26f774e5b4fd219687d0 on 2020-05-14

#include <emmintrin.h>


/* YV12 -> YUY2 conversion */


static inline void copy_yv12_line_to_yuy2_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int width) {
  for (int x = 0; x < width / 2; ++x) {
    dstp[x*4] = srcY[x*2];
    dstp[x*4+2] = srcY[x*2+1];
    dstp[x*4+1] = srcU[x];
    dstp[x*4+3] = srcV[x];
  }
}

void convert_yv12_to_yuy2_progressive_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height) {
  //first two lines
  copy_yv12_line_to_yuy2_c(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_c(srcY+src_pitch_y, srcU, srcV, dstp+dst_pitch, src_width);

  //last two lines. Easier to do them here
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y*2;
  srcU += src_pitch_uv;
  srcV += src_pitch_uv;
  dstp += dst_pitch*2;

  for (int y = 2; y < height-2; y+=2) {
    for (int x = 0; x < src_width / 2; ++x) {
      dstp[x*4] = srcY[x*2];
      dstp[x*4+2] = srcY[x*2+1];

      //avg(avg(a, b)-1, b)
      dstp[x*4+1] = ((((srcU[x-src_pitch_uv] + srcU[x] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3] = ((((srcV[x-src_pitch_uv] + srcV[x] + 1) / 2) + srcV[x]) / 2);

      dstp[x*4 + dst_pitch] = srcY[x*2 + src_pitch_y];
      dstp[x*4+2 + dst_pitch] = srcY[x*2+1 + src_pitch_y];

      dstp[x*4+1 + dst_pitch] = ((((srcU[x] + srcU[x+src_pitch_uv] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3 + dst_pitch] = ((((srcV[x] + srcV[x+src_pitch_uv] + 1) / 2) + srcV[x]) / 2);
    }
    srcY += src_pitch_y*2;
    dstp += dst_pitch*2;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
}

void convert_yv12_to_yuy2_interlaced_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height) {
  //first four lines
  copy_yv12_line_to_yuy2_c(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y*2, srcU, srcV, dstp + dst_pitch*2, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y*3, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch*3, src_width);

  //last four lines. Easier to do them here
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-4),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-4),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-3),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-3),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y * 4;
  srcU += src_pitch_uv * 2;
  srcV += src_pitch_uv * 2;
  dstp += dst_pitch * 4;

  for (int y = 4; y < height-4; y+= 2) {
    for (int x = 0; x < src_width / 2; ++x) {
      dstp[x*4] = srcY[x*2];
      dstp[x*4+2] = srcY[x*2+1];

      dstp[x*4+1] = ((((srcU[x-src_pitch_uv*2] + srcU[x] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3] = ((((srcV[x-src_pitch_uv*2] + srcV[x] + 1) / 2) + srcV[x]) / 2);

      dstp[x*4 + dst_pitch*2] = srcY[x*2 + src_pitch_y*2];
      dstp[x*4+2 + dst_pitch*2] = srcY[x*2+1 + src_pitch_y*2];

      dstp[x*4+1 + dst_pitch*2] = ((((srcU[x] + srcU[x+src_pitch_uv*2] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3 + dst_pitch*2] = ((((srcV[x] + srcV[x+src_pitch_uv*2] + 1) / 2) + srcV[x]) / 2);
    }

    if (y % 4 == 0) {
      //top field processed, jumb to the bottom
      srcY += src_pitch_y;
      dstp += dst_pitch;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
    } else {
      //bottom field processed, jump to the next top
      srcY += src_pitch_y*3;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
      dstp += dst_pitch*3;
    }
  }
}


#ifdef X86_32

#pragma warning(push)
#pragma warning(disable: 4799)
//75% of the first argument and 25% of the second one.
static AVS_FORCEINLINE __m64 convert_yv12_to_yuy2_merge_chroma_isse(const __m64 &line75p, const __m64 &line25p, const __m64 &one) {
  __m64 avg_chroma_lo = _mm_avg_pu8(line75p, line25p);
  avg_chroma_lo = _mm_subs_pu8(avg_chroma_lo, one);
  return _mm_avg_pu8(avg_chroma_lo, line75p);
}

// first parameter is 8 luma pixels
// second and third - 4 chroma bytes in low dwords
// last two params are OUT
static AVS_FORCEINLINE void convert_yv12_pixels_to_yuy2_isse(const __m64 &y, const __m64 &u, const __m64 &v,  const __m64 &zero, __m64 &out_low, __m64 &out_high) {
  __m64 chroma = _mm_unpacklo_pi8(u, v);
  out_low = _mm_unpacklo_pi8(y, chroma);
  out_high = _mm_unpackhi_pi8(y, chroma);
}

static inline void copy_yv12_line_to_yuy2_isse(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int width) {
  __m64 zero = _mm_setzero_si64();
  for (int x = 0; x < width / 2; x+=4) {
    __m64 src_y = *reinterpret_cast<const __m64*>(srcY+x*2); //Y Y Y Y Y Y Y Y
    __m64 src_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU+x)); //0 0 0 0 U U U U
    __m64 src_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV+x)); //0 0 0 0 V V V V

    __m64 dst_lo, dst_hi;
    convert_yv12_pixels_to_yuy2_isse(src_y, src_u, src_v, zero, dst_lo, dst_hi);

    *reinterpret_cast<__m64*>(dstp + x*4) = dst_lo;
    *reinterpret_cast<__m64*>(dstp + x*4 + 8) = dst_hi;
  }
}
#pragma warning(pop)

void convert_yv12_to_yuy2_interlaced_isse(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height)
{
  //first four lines
  copy_yv12_line_to_yuy2_isse(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_isse(srcY + src_pitch_y*2, srcU, srcV, dstp + dst_pitch*2, src_width);
  copy_yv12_line_to_yuy2_isse(srcY + src_pitch_y, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch, src_width);
  copy_yv12_line_to_yuy2_isse(srcY + src_pitch_y*3, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch*3, src_width);

  //last four lines. Easier to do them here
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-4),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-4),
    src_width
    );
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-3),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-3),
    src_width
    );
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y * 4;
  srcU += src_pitch_uv * 2;
  srcV += src_pitch_uv * 2;
  dstp += dst_pitch * 4;

  __m64 one = _mm_set1_pi8(1);
  __m64 zero = _mm_setzero_si64();

  for (int y = 4; y < height-4; y+= 2) {
    for (int x = 0; x < src_width / 2; x+=4) {

      __m64 luma_line = *reinterpret_cast<const __m64*>(srcY + x*2); //Y Y Y Y Y Y Y Y
      __m64 src_current_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU + x)); //0 0 0 0 U U U U
      __m64 src_current_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV + x)); //0 0 0 0 V V V V
      __m64 src_prev_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU - src_pitch_uv*2 + x)); //0 0 0 0 U U U U
      __m64 src_prev_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV - src_pitch_uv*2 + x)); //0 0 0 0 V V V V

      __m64 src_u = convert_yv12_to_yuy2_merge_chroma_isse(src_current_u, src_prev_u, one);
      __m64 src_v = convert_yv12_to_yuy2_merge_chroma_isse(src_current_v, src_prev_v, one);

      __m64 dst_lo, dst_hi;
      convert_yv12_pixels_to_yuy2_isse(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      *reinterpret_cast<__m64*>(dstp + x*4) = dst_lo;
      *reinterpret_cast<__m64*>(dstp + x*4 + 8) = dst_hi;

      luma_line = *reinterpret_cast<const __m64*>(srcY + src_pitch_y *2+ x*2); //Y Y Y Y Y Y Y Y
      __m64 src_next_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU + src_pitch_uv*2 + x)); //0 0 0 0 U U U U
      __m64 src_next_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV + src_pitch_uv*2 + x)); //0 0 0 0 V V V V

      src_u = convert_yv12_to_yuy2_merge_chroma_isse(src_current_u, src_next_u, one);
      src_v = convert_yv12_to_yuy2_merge_chroma_isse(src_current_v, src_next_v, one);

      convert_yv12_pixels_to_yuy2_isse(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      *reinterpret_cast<__m64*>(dstp + dst_pitch*2 + x*4) = dst_lo;
      *reinterpret_cast<__m64*>(dstp + dst_pitch*2 + x*4 + 8) = dst_hi;
    }

    if (y % 4 == 0) {
      //top field processed, jumb to the bottom
      srcY += src_pitch_y;
      dstp += dst_pitch;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
    } else {
      //bottom field processed, jump to the next top
      srcY += src_pitch_y*3;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
      dstp += dst_pitch*3;
    }
  }
  _mm_empty();
}

void convert_yv12_to_yuy2_progressive_isse(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height)
{
  //first two lines
  copy_yv12_line_to_yuy2_isse(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_isse(srcY+src_pitch_y, srcU, srcV, dstp+dst_pitch, src_width);

  //last two lines. Easier to do them here
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y*2;
  srcU += src_pitch_uv;
  srcV += src_pitch_uv;
  dstp += dst_pitch*2;

  __m64 one = _mm_set1_pi8(1);
  __m64 zero = _mm_setzero_si64();

  for (int y = 2; y < height-2; y+=2) {
    for (int x = 0; x < src_width / 2; x+=4) {
      __m64 luma_line = *reinterpret_cast<const __m64*>(srcY + x*2); //Y Y Y Y Y Y Y Y
      __m64 src_current_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU + x)); //0 0 0 0 U U U U
      __m64 src_current_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV + x)); //0 0 0 0 V V V V
      __m64 src_prev_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU - src_pitch_uv + x)); //0 0 0 0 U U U U
      __m64 src_prev_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV - src_pitch_uv + x)); //0 0 0 0 V V V V

      __m64 src_u = convert_yv12_to_yuy2_merge_chroma_isse(src_current_u, src_prev_u, one);
      __m64 src_v = convert_yv12_to_yuy2_merge_chroma_isse(src_current_v, src_prev_v, one);

      __m64 dst_lo, dst_hi;
      convert_yv12_pixels_to_yuy2_isse(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      *reinterpret_cast<__m64*>(dstp + x*4) = dst_lo;
      *reinterpret_cast<__m64*>(dstp + x*4 + 8) = dst_hi;

      luma_line = *reinterpret_cast<const __m64*>(srcY + src_pitch_y + x*2); //Y Y Y Y Y Y Y Y
      __m64 src_next_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU + src_pitch_uv + x)); //0 0 0 0 U U U U
      __m64 src_next_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV + src_pitch_uv + x)); //0 0 0 0 V V V V

      src_u = convert_yv12_to_yuy2_merge_chroma_isse(src_current_u, src_next_u, one);
      src_v = convert_yv12_to_yuy2_merge_chroma_isse(src_current_v, src_next_v, one);

      convert_yv12_pixels_to_yuy2_isse(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      *reinterpret_cast<__m64*>(dstp + dst_pitch + x*4) = dst_lo;
      *reinterpret_cast<__m64*>(dstp + dst_pitch + x*4 + 8) = dst_hi;
    }
    srcY += src_pitch_y*2;
    dstp += dst_pitch*2;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
  _mm_empty();
}
#endif

#ifdef __SSE2__
//75% of the first argument and 25% of the second one.
static AVS_FORCEINLINE __m128i convert_yv12_to_yuy2_merge_chroma_sse2(const __m128i &line75p, const __m128i &line25p, const __m128i &one) {
  __m128i avg_chroma_lo = _mm_avg_epu8(line75p, line25p);
  avg_chroma_lo = _mm_subs_epu8(avg_chroma_lo, one);
  return _mm_avg_epu8(avg_chroma_lo, line75p);
}

// first parameter is 16 luma pixels
// second and third - 8 chroma bytes in low dwords
// last two params are OUT
static AVS_FORCEINLINE void convert_yv12_pixels_to_yuy2_sse2(const __m128i &y, const __m128i &u, const __m128i &v,  const __m128i &zero, __m128i &out_low, __m128i &out_high) {
  AVS_UNUSED(zero);
  __m128i chroma = _mm_unpacklo_epi8(u, v); //...V3 U3 V2 U2 V1 U1 V0 U0
  out_low = _mm_unpacklo_epi8(y, chroma);
  out_high = _mm_unpackhi_epi8(y, chroma);
}

static inline void copy_yv12_line_to_yuy2_sse2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int width) {
  __m128i zero = _mm_setzero_si128();
  for (int x = 0; x < width / 2; x+=8) {
    __m128i src_y = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY+x*2)); //Y Y Y Y Y Y Y Y
    __m128i src_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU+x)); //0 0 0 0 U U U U
    __m128i src_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV+x)); //0 0 0 0 V V V V

    __m128i dst_lo, dst_hi;
    convert_yv12_pixels_to_yuy2_sse2(src_y, src_u, src_v, zero, dst_lo, dst_hi);

    _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4), dst_lo);
    _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4 + 16), dst_hi);
  }
}

void convert_yv12_to_yuy2_interlaced_sse2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height)
{
  //first four lines
  copy_yv12_line_to_yuy2_sse2(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_sse2(srcY + src_pitch_y*2, srcU, srcV, dstp + dst_pitch*2, src_width);
  copy_yv12_line_to_yuy2_sse2(srcY + src_pitch_y, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch, src_width);
  copy_yv12_line_to_yuy2_sse2(srcY + src_pitch_y*3, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch*3, src_width);

  //last four lines. Easier to do them here
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-4),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-4),
    src_width
    );
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-3),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-3),
    src_width
    );
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y * 4;
  srcU += src_pitch_uv * 2;
  srcV += src_pitch_uv * 2;
  dstp += dst_pitch * 4;

  __m128i one = _mm_set1_epi8(1);
  __m128i zero = _mm_setzero_si128();

  for (int y = 4; y < height-4; y+= 2) {
    for (int x = 0; x < src_width / 2; x+=8) {

      __m128i luma_line = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY + x*2)); //Y Y Y Y Y Y Y Y
      __m128i src_current_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU + x)); //0 0 0 0 U U U U
      __m128i src_current_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV + x)); //0 0 0 0 V V V V
      __m128i src_prev_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU - src_pitch_uv*2 + x)); //0 0 0 0 U U U U
      __m128i src_prev_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV - src_pitch_uv*2 + x)); //0 0 0 0 V V V V

      __m128i src_u = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_u, src_prev_u, one);
      __m128i src_v = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_v, src_prev_v, one);

      __m128i dst_lo, dst_hi;
      convert_yv12_pixels_to_yuy2_sse2(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4), dst_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4 + 16), dst_hi);

      luma_line = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY + src_pitch_y*2+ x*2)); //Y Y Y Y Y Y Y Y
      __m128i src_next_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU + src_pitch_uv*2 + x)); //0 0 0 0 U U U U
      __m128i src_next_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV + src_pitch_uv*2 + x)); //0 0 0 0 V V V V

      src_u = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_u, src_next_u, one);
      src_v = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_v, src_next_v, one);

      convert_yv12_pixels_to_yuy2_sse2(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + dst_pitch*2 + x*4), dst_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + dst_pitch*2 + x*4 + 16), dst_hi);
    }

    if (y % 4 == 0) {
      //top field processed, jumb to the bottom
      srcY += src_pitch_y;
      dstp += dst_pitch;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
    } else {
      //bottom field processed, jump to the next top
      srcY += src_pitch_y*3;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
      dstp += dst_pitch*3;
    }
  }
}

void convert_yv12_to_yuy2_progressive_sse2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height)
{
  //first two lines
  copy_yv12_line_to_yuy2_sse2(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_sse2(srcY+src_pitch_y, srcU, srcV, dstp+dst_pitch, src_width);

  //last two lines. Easier to do them here
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y*2;
  srcU += src_pitch_uv;
  srcV += src_pitch_uv;
  dstp += dst_pitch*2;

  __m128i one = _mm_set1_epi8(1);
  __m128i zero = _mm_setzero_si128();

  for (int y = 2; y < height-2; y+=2) {
    for (int x = 0; x < src_width / 2; x+=8) {
      __m128i luma_line = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY + x*2)); //Y Y Y Y Y Y Y Y
      __m128i src_current_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU + x)); //0 0 0 0 U U U U
      __m128i src_current_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV + x)); //0 0 0 0 V V V V
      __m128i src_prev_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU - src_pitch_uv + x)); //0 0 0 0 U U U U
      __m128i src_prev_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV - src_pitch_uv + x)); //0 0 0 0 V V V V

      __m128i src_u = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_u, src_prev_u, one);
      __m128i src_v = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_v, src_prev_v, one);

      __m128i dst_lo, dst_hi;
      convert_yv12_pixels_to_yuy2_sse2(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4), dst_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4 + 16), dst_hi);

      luma_line = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY + src_pitch_y + x*2)); //Y Y Y Y Y Y Y Y
      __m128i src_next_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU + src_pitch_uv + x)); //0 0 0 0 U U U U
      __m128i src_next_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV + src_pitch_uv + x)); //0 0 0 0 V V V V

      src_u = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_u, src_next_u, one);
      src_v = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_v, src_next_v, one);

      convert_yv12_pixels_to_yuy2_sse2(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + dst_pitch + x*4), dst_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + dst_pitch + x*4 + 16), dst_hi);
    }
    srcY += src_pitch_y*2;
    dstp += dst_pitch*2;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
}
#endif

// end code from AviSynthPlus/avs_core/convert/intel/convert_yv12_sse.cpp


//////////////////////////////////////////////////////////////////////////////////

void convert_yuv420p_to_yuy2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height, bool interlaced)
{
	const bool bPtrsAligned = IsPtrAligned(srcY, 16) && IsPtrAligned(srcU, 16) && IsPtrAligned(srcV, 16);

	if (interlaced) {
#if __SSE2__
		if (bPtrsAligned) {
			convert_yv12_to_yuy2_interlaced_sse2(srcY, srcU, srcV, src_width, src_pitch_y, src_pitch_uv, dstp, dst_pitch, height);
		} else
#endif
		{
#ifdef X86_32
			convert_yv12_to_yuy2_interlaced_isse(srcY, srcU, srcV, src_width, src_pitch_y, src_pitch_uv, dstp, dst_pitch, height);
#else
			convert_yv12_to_yuy2_interlaced_c(srcY, srcU, srcV, src_width, src_pitch_y, src_pitch_uv, dstp, dst_pitch, height);
#endif
		}
	} else {
#if __SSE2__
		if (bPtrsAligned) {
			convert_yv12_to_yuy2_progressive_sse2(srcY, srcU, srcV, src_width, src_pitch_y, src_pitch_uv, dstp, dst_pitch, height);
		} else
#endif
		{
#ifdef X86_32
			convert_yv12_to_yuy2_progressive_isse(srcY, srcU, srcV, src_width, src_pitch_y, src_pitch_uv, dstp, dst_pitch, height);
#else
			convert_yv12_to_yuy2_progressive_c(srcY, srcU, srcV, src_width, src_pitch_y, src_pitch_uv, dstp, dst_pitch, height);
#endif
		}
	}
}
