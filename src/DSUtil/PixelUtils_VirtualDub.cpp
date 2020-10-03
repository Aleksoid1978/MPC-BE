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
#include <emmintrin.h>
#include "PixelUtils_VirtualDub.h"

#pragma warning(disable: 4799) // warning C4799: function has no EMMS instruction


// https://sourceforge.net/p/vdfiltermod/code/ci/master/tree/src/h/vd2/system/vdtypes.h
// commit 0ef5cdc3554e9c8f8129b17539a13f0f0b38c038 on 2015-03-01

#ifndef VD_CPU_DETECTED
	#define VD_CPU_DETECTED

	#if defined(_M_AMD64)
		#define VD_CPU_AMD64	1
	#elif defined(_M_IX86) || defined(__i386__)
		#define VD_CPU_X86		1
	#elif defined(_M_ARM)
		#define VD_CPU_ARM
	#endif
#endif

///////////////////////////////////////////////////////////////////////////
//
//	types
//
///////////////////////////////////////////////////////////////////////////

#ifndef VD_STANDARD_TYPES_DECLARED
	#if defined(_MSC_VER)
		typedef signed __int64		sint64;
		typedef unsigned __int64	uint64;
	#elif defined(__GNUC__)
		typedef signed long long	sint64;
		typedef unsigned long long	uint64;
	#endif
	typedef signed int			sint32;
	typedef unsigned int		uint32;
	typedef signed short		sint16;
	typedef unsigned short		uint16;
	typedef signed char			sint8;
	typedef unsigned char		uint8;

	typedef sint64				int64;
	typedef sint32				int32;
	typedef sint16				int16;
	typedef sint8				int8;

	#ifdef _M_AMD64
		typedef sint64 sintptr;
		typedef uint64 uintptr;
	#else
		#if _MSC_VER >= 1310
			typedef __w64 sint32 sintptr;
			typedef __w64 uint32 uintptr;
		#else
			typedef sint32 sintptr;
			typedef uint32 uintptr;
		#endif
	#endif
#endif

// end code from vdfiltermod/src/h/vd2/system/vdtypes.h


// https://sourceforge.net/p/vdfiltermod/code/ci/master/tree/src/VDFilters/source/VFDeinterlace.cpp

#ifdef _M_IX86
static void __declspec(naked) asm_blend_row_clipped(void *dst, const void *src, uint32 w, ptrdiff_t srcpitch) {
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		edi,[esp+20]
		mov		esi,[esp+24]
		sub		edi,esi
		mov		ebp,[esp+28]
		mov		edx,[esp+32]

xloop:
		mov		ecx,[esi]
		mov		eax,0fefefefeh

		mov		ebx,[esi+edx]
		and		eax,ecx

		shr		eax,1
		and		ebx,0fefefefeh

		shr		ebx,1
		add		esi,4

		add		eax,ebx
		dec		ebp

		mov		[edi+esi-4],eax
		jnz		xloop

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

static void __declspec(naked) asm_blend_row_ISSE(void *dst, const void *src, uint32 w, ptrdiff_t srcpitch) {
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		edi,[esp+20]
		mov		esi,[esp+24]
		sub		edi,esi
		mov		ebp,[esp+28]
		mov		edx,[esp+32]

		inc		ebp
		shr		ebp,1
		pcmpeqb	mm7, mm7

		align	16
xloop:
		movq	mm0, [esi]
		movq	mm2, mm7
		pxor	mm0, mm7

		pxor	mm2, [esi+edx*2]
		pavgb	mm0, mm2
		pxor	mm0, mm7

		pavgb	mm0, [esi+edx]
		add		esi,8

		movq	[edi+esi-8],mm0
		dec		ebp
		jne		xloop

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}
#else
static void asm_blend_row_clipped(void *dst0, const void *src0, uint32 w, ptrdiff_t srcpitch) {
	uint32 *dst = (uint32 *)dst0;
	const uint32 *src = (const uint32 *)src0;
	const uint32 *src2 = (const uint32 *)((const char *)src + srcpitch);

	do {
		const uint32 x = *src++;
		const uint32 y = *src2++;

		*dst++ = (x|y) - (((x^y)&0xfefefefe)>>1);
	} while(--w);
}
#endif

#if defined(VD_CPU_X86) || defined(VD_CPU_AMD64)
static void asm_blend_row_SSE2(void *dst, const void *src, uint32 w, ptrdiff_t srcpitch) {
	__m128i zero = _mm_setzero_si128();
	__m128i inv = _mm_cmpeq_epi8(zero, zero);

	w = (w + 3) >> 2;

	const __m128i *src1 = (const __m128i *)src;
	const __m128i *src2 = (const __m128i *)((const char *)src + srcpitch);
	const __m128i *src3 = (const __m128i *)((const char *)src + srcpitch*2);
	__m128i *dstrow = (__m128i *)dst;
	do {
		__m128i a = *src1++;
		__m128i b = *src2++;
		__m128i c = *src3++;

		*dstrow++ = _mm_avg_epu8(_mm_xor_si128(_mm_avg_epu8(_mm_xor_si128(a, inv), _mm_xor_si128(c, inv)), inv), b);
	} while(--w);
}
#endif

// end code from vdfiltermod/src/VDFilters/source/VFDeinterlace.cpp

void BlendPlane(void *dst, ptrdiff_t dstpitch, const void *src, ptrdiff_t srcpitch, uint32_t w, uint32_t h) {
	void (*blend_func)(void *, const void *, uint32, ptrdiff_t);
#if defined(VD_CPU_X86)
	if (!(srcpitch % 16))
		blend_func = asm_blend_row_SSE2;
	else
		blend_func = asm_blend_row_ISSE;
#else
	blend_func = asm_blend_row_SSE2;
#endif

	w = (w + 3) >> 2;

	asm_blend_row_clipped(dst, src, w, srcpitch);
	if (h-=2)
		do {
			dst = ((char *)dst + dstpitch);

			blend_func(dst, src, w, srcpitch);

			src = ((char *)src + srcpitch);
		} while(--h);

	asm_blend_row_clipped((char *)dst + dstpitch, src, w, srcpitch);

#ifdef _M_IX86
	__asm emms
#endif
}
