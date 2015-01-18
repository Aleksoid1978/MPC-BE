/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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

#ifndef _SIMD_COMMON_H_
#define _SIMD_COMMON_H_

#if !defined(__GNUC__) && !defined(__SSE2__)
  #define __SSE2__
#endif

#ifdef __GNUC__
  #ifndef __forceinline
    #define __forceinline __attribute__((__always_inline__)) inline
  #endif
#endif

#ifdef __GNUC__
 #define __inline __forceinline  // GCC needs to force inlining of intrinsics functions
#endif

#include <mmintrin.h>
#include <xmmintrin.h>
#ifdef __SSE2__
  #include <emmintrin.h>
#endif

#ifdef __GNUC__
 #undef __inline
#endif

#ifdef __GNUC__
  #define __align8(t,v) t v __attribute__ ((aligned (8)))
  #define __align16(t,v) t v __attribute__ ((aligned (16)))
#else
  #define __align8(t,v) __declspec(align(8)) t v
  #define __align16(t,v) __declspec(align(16)) t v
#endif

#endif
