//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2007 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "stdafx.h"
#include "vd.h"

#include <emmintrin.h>
#include <vd2/system/memory.h>
#include <vd2/system/cpuaccel.h>
#include <vd2/system/vdstl.h>

#pragma warning(disable: 4799) // warning C4799: function has no EMMS instruction

///////////////////////////////////////////////////////////////////////////

// Blend

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

static void __declspec(naked) asm_blend_row(void *dst, const void *src, uint32 w, ptrdiff_t srcpitch) {
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
		mov		eax,0fcfcfcfch

		mov		ebx,[esi+edx]
		and		eax,ecx

		shr		ebx,1
		mov		ecx,[esi+edx*2]

		shr		ecx,2
		and		ebx,07f7f7f7fh

		shr		eax,2
		and		ecx,03f3f3f3fh

		add		eax,ebx
		add		esi,4

		add		eax,ecx
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

static void __declspec(naked) asm_blend_row_MMX(void *dst, const void *src, uint32 w, ptrdiff_t srcpitch) {
	static const __declspec(align(8)) __int64 mask0 = 0xfcfcfcfcfcfcfcfci64;
	static const __declspec(align(8)) __int64 mask1 = 0x7f7f7f7f7f7f7f7fi64;
	static const __declspec(align(8)) __int64 mask2 = 0x3f3f3f3f3f3f3f3fi64;
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

		movq	mm5,mask0
		movq	mm6,mask1
		movq	mm7,mask2
		inc		ebp
		shr		ebp,1
xloop:
		movq	mm2,[esi]
		movq	mm0,mm5

		movq	mm1,[esi+edx]
		pand	mm0,mm2

		psrlq	mm1,1
		movq	mm2,[esi+edx*2]

		psrlq	mm2,2
		pand	mm1,mm6

		psrlq	mm0,2
		pand	mm2,mm7

		paddb	mm0,mm1
		add		esi,8

		paddb	mm0,mm2
		dec		ebp

		movq	[edi+esi-8],mm0
		jne		xloop

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

static void asm_blend_row(void *dst0, const void *src0, uint32 w, ptrdiff_t srcpitch) {
	uint32 *dst = (uint32 *)dst0;
	const uint32 *src = (const uint32 *)src0;
	const uint32 *src2 = (const uint32 *)((const char *)src + srcpitch);
	const uint32 *src3 = (const uint32 *)((const char *)src2 + srcpitch);

	do {
		const uint32 a = *src++;
		const uint32 b = *src2++;
		const uint32 c = *src3++;
		const uint32 hi = (a & 0xfcfcfc) + 2*(b & 0xfcfcfc) + (c & 0xfcfcfc);
		const uint32 lo = (a & 0x030303) + 2*(b & 0x030303) + (c & 0x030303) + 0x020202;

		*dst++ = (hi + (lo & 0x0c0c0c))>>2;
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

void BlendPlane(void *dst, ptrdiff_t dstpitch, const void *src, ptrdiff_t srcpitch, uint32 w, uint32 h) {
	void (*blend_func)(void *, const void *, uint32, ptrdiff_t);
#if defined(VD_CPU_X86)
	if (SSE2_enabled && !(srcpitch % 16))
		blend_func = asm_blend_row_SSE2;
	else
		blend_func = ISSE_enabled ? asm_blend_row_ISSE : MMX_enabled ? asm_blend_row_MMX : asm_blend_row;
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
	if (MMX_enabled)
		__asm emms
#endif
}

void DeinterlaceBlend(BYTE* dst, BYTE* src, DWORD w, DWORD h, DWORD dstpitch, DWORD srcpitch)
{
	BlendPlane(dst, dstpitch, src, srcpitch, w, h);
}

// Bob

void AvgLines8(BYTE* dst, DWORD h, DWORD pitch)
{
	if(h <= 1) return;

	BYTE* s = dst;
	BYTE* d = dst + (h-2)*pitch;

	for(; s < d; s += pitch*2)
	{
		BYTE* tmp = s;

		{
			for(int i = pitch; i--; tmp++)
			{
				tmp[pitch] = (tmp[0] + tmp[pitch<<1] + 1) >> 1;
			}
		}
	}

	if(!(h&1) && h >= 2)
	{
		dst += (h-2)*pitch;
		memcpy(dst + pitch, dst, pitch);
	}

}

void DeinterlaceBob(BYTE* dst, BYTE* src, DWORD rowbytes, DWORD h, DWORD dstpitch, DWORD srcpitch, bool topfield)
{
	if(topfield)
	{
		BitBltFromRGBToRGB(rowbytes, h/2, dst, dstpitch*2, 8, src, srcpitch*2, 8);
		AvgLines8(dst, h, dstpitch);
	}
	else
	{
		BitBltFromRGBToRGB(rowbytes, h/2, dst + dstpitch, dstpitch*2, 8, src + srcpitch, srcpitch*2, 8);
		AvgLines8(dst + dstpitch, h-1, dstpitch);
	}
}
