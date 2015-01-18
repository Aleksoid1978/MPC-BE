/*
 *
 *      Copyright (C) 2011 Hendrik Leppkes
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
 *  Taken from the QuickSync decoder by Eric Gur
 *
 *  Adaptation for MPC-BE (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
 *
 */

#pragma once

#include <intrin.h>
#include <emmintrin.h>

inline void* memcpy_sse2(void* dst, const void* src, size_t nBytes)
{
#ifndef _WIN64
	__asm
	{
		// optimized on Intel Core 2 Duo

		mov	ecx, nBytes
		mov	edi, dst
		mov	esi, src
		add	ecx, edi

		prefetchnta [esi]
		prefetchnta [esi+32]
		prefetchnta [esi+64]
		prefetchnta [esi+96]

		// handle nBytes lower than 128
		cmp	nBytes, 512
		jge	fast

slow:
		mov	bl, [esi]
		mov	[edi], bl
		inc	edi
		inc	esi
		cmp	ecx, edi
		jnz	slow
		jmp	end

fast:
		// align dstEnd to 128 bytes
		and	ecx, 0xFFFFFF80

		// get srcEnd aligned to dstEnd aligned to 128 bytes
		mov	ebx, esi
		sub	ebx, edi
		add	ebx, ecx

		// skip unaligned copy if dst is aligned
		mov	eax, edi
		and	edi, 0xFFFFFF80
		cmp	eax, edi
		jne	first
		jmp	more

first:
		// copy the first 128 bytes unaligned
		movdqu	xmm0, [esi]
		movdqu	xmm1, [esi+16]
		movdqu	xmm2, [esi+32]
		movdqu	xmm3, [esi+48]

		movdqu	xmm4, [esi+64]
		movdqu	xmm5, [esi+80]
		movdqu	xmm6, [esi+96]
		movdqu	xmm7, [esi+112]

		movdqu	[eax], xmm0
		movdqu	[eax+16], xmm1
		movdqu	[eax+32], xmm2
		movdqu	[eax+48], xmm3

		movdqu	[eax+64], xmm4
		movdqu	[eax+80], xmm5
		movdqu	[eax+96], xmm6
		movdqu	[eax+112], xmm7

		// add 128 bytes to edi aligned earlier
		add	edi, 128

		// offset esi by the same value
		sub	eax, edi
		sub	esi, eax

		// last bytes if dst at dstEnd
		cmp	ecx, edi
		jnz	more
		jmp	last

more:
		// handle equally aligned arrays
		mov	eax, esi
		and	eax, 0xFFFFFF80
		cmp	eax, esi
		jne	unaligned4k

aligned4k:
		mov	eax, esi
		add	eax, 4096
		cmp	eax, ebx
		jle	aligned4kin
		cmp	ecx, edi
		jne	alignedlast
		jmp	last

aligned4kin:
		prefetchnta [esi]
		prefetchnta [esi+32]
		prefetchnta [esi+64]
		prefetchnta [esi+96]

		add	esi, 128

		cmp	eax, esi
		jne	aligned4kin

		sub	esi, 4096

alinged4kout:
		movdqa	xmm0, [esi]
		movdqa	xmm1, [esi+16]
		movdqa	xmm2, [esi+32]
		movdqa	xmm3, [esi+48]

		movdqa	xmm4, [esi+64]
		movdqa	xmm5, [esi+80]
		movdqa	xmm6, [esi+96]
		movdqa	xmm7, [esi+112]

		movntdq	[edi], xmm0
		movntdq	[edi+16], xmm1
		movntdq	[edi+32], xmm2
		movntdq	[edi+48], xmm3

		movntdq	[edi+64], xmm4
		movntdq	[edi+80], xmm5
		movntdq	[edi+96], xmm6
		movntdq	[edi+112], xmm7

		add	esi, 128
		add	edi, 128

		cmp	eax, esi
		jne	alinged4kout
		jmp	aligned4k

alignedlast:
		mov	eax, esi

alignedlastin:
		prefetchnta [esi]
		prefetchnta [esi+32]
		prefetchnta [esi+64]
		prefetchnta [esi+96]

		add	esi, 128

		cmp	ebx, esi
		jne	alignedlastin

		mov	esi, eax

alignedlastout:
		movdqa	xmm0, [esi]
		movdqa	xmm1, [esi+16]
		movdqa	xmm2, [esi+32]
		movdqa	xmm3, [esi+48]

		movdqa	xmm4, [esi+64]
		movdqa	xmm5, [esi+80]
		movdqa	xmm6, [esi+96]
		movdqa	xmm7, [esi+112]

		movntdq	[edi], xmm0
		movntdq	[edi+16], xmm1
		movntdq	[edi+32], xmm2
		movntdq	[edi+48], xmm3

		movntdq	[edi+64], xmm4
		movntdq	[edi+80], xmm5
		movntdq	[edi+96], xmm6
		movntdq	[edi+112], xmm7

		add	esi, 128
		add	edi, 128

		cmp	ecx, edi
		jne	alignedlastout
		jmp	last

unaligned4k:
		mov	eax, esi
		add	eax, 4096
		cmp	eax, ebx
		jle	unaligned4kin
		cmp	ecx, edi
		jne	unalignedlast
		jmp	last

unaligned4kin:
		prefetchnta [esi]
		prefetchnta [esi+32]
		prefetchnta [esi+64]
		prefetchnta [esi+96]

		add	esi, 128

		cmp	eax, esi
		jne	unaligned4kin

		sub	esi, 4096

unalinged4kout:
		movdqu	xmm0, [esi]
		movdqu	xmm1, [esi+16]
		movdqu	xmm2, [esi+32]
		movdqu	xmm3, [esi+48]

		movdqu	xmm4, [esi+64]
		movdqu	xmm5, [esi+80]
		movdqu	xmm6, [esi+96]
		movdqu	xmm7, [esi+112]

		movntdq	[edi], xmm0
		movntdq	[edi+16], xmm1
		movntdq	[edi+32], xmm2
		movntdq	[edi+48], xmm3

		movntdq	[edi+64], xmm4
		movntdq	[edi+80], xmm5
		movntdq	[edi+96], xmm6
		movntdq	[edi+112], xmm7

		add	esi, 128
		add	edi, 128

		cmp	eax, esi
		jne	unalinged4kout
		jmp	unaligned4k

unalignedlast:
		mov	eax, esi

unalignedlastin:
		prefetchnta [esi]
		prefetchnta [esi+32]
		prefetchnta [esi+64]
		prefetchnta [esi+96]

		add	esi, 128

		cmp	ebx, esi
		jne	unalignedlastin

		mov	esi, eax

unalignedlastout:
		movdqu	xmm0, [esi]
		movdqu	xmm1, [esi+16]
		movdqu	xmm2, [esi+32]
		movdqu	xmm3, [esi+48]

		movdqu	xmm4, [esi+64]
		movdqu	xmm5, [esi+80]
		movdqu	xmm6, [esi+96]
		movdqu	xmm7, [esi+112]

		movntdq	[edi], xmm0
		movntdq	[edi+16], xmm1
		movntdq	[edi+32], xmm2
		movntdq	[edi+48], xmm3

		movntdq	[edi+64], xmm4
		movntdq	[edi+80], xmm5
		movntdq	[edi+96], xmm6
		movntdq	[edi+112], xmm7

		add	esi, 128
		add	edi, 128

		cmp	ecx, edi
		jne	unalignedlastout
		jmp	last

last:
		// get the last 128 bytes
		mov	ecx, nBytes
		mov	edi, dst
		mov	esi, src
		add	edi, ecx
		add	esi, ecx
		sub	edi, 128
		sub	esi, 128

		// copy the last 128 bytes unaligned
		movdqu	xmm0, [esi]
		movdqu	xmm1, [esi+16]
		movdqu	xmm2, [esi+32]
		movdqu	xmm3, [esi+48]

		movdqu	xmm4, [esi+64]
		movdqu	xmm5, [esi+80]
		movdqu	xmm6, [esi+96]
		movdqu	xmm7, [esi+112]

		movdqu	[edi], xmm0
		movdqu	[edi+16], xmm1
		movdqu	[edi+32], xmm2
		movdqu	[edi+48], xmm3

		movdqu	[edi+64], xmm4
		movdqu	[edi+80], xmm5
		movdqu	[edi+96], xmm6
		movdqu	[edi+112], xmm7

end:
	}

	return dst;
#else
	return memcpy(dst, src, nBytes);
#endif
}

static bool SSE = 0, SSE2 = 0, SSE41 = 0;

inline void check_sse()
{
	if (!SSE && !SSE2 && !SSE41)
	{
		int info[4];
		__cpuid(info, 0);

		// Detect Instruction Set
		if (info[0] >= 1)
		{
			__cpuid(info, 0x00000001);

			SSE   = (info[3] & ((int)1 << 25)) != 0;
			SSE2  = (info[3] & ((int)1 << 26)) != 0;
			SSE41 = (info[2] & ((int)1 << 19)) != 0;
		}
	}
}

inline void* memcpy_sse(void* d, const void* s, size_t size)
{
	if (d == NULL || s == NULL || size == 0)
	{
		return NULL;
	}

	// If memory is not aligned, use memcpy
	bool isAligned = (((size_t)(s) | (size_t)(d)) & 0xF) == 0;
	if (!isAligned)
	{
		return memcpy(d, s, size);
	}

	check_sse();

	if (!SSE41)
	{
		if (SSE2)
		{
			return memcpy_sse2(d, s, size);
		}

		return memcpy(d, s, size);
	}

	static const size_t regsInLoop = sizeof(size_t) * 2; // 8 or 16

	__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
#ifdef _M_X64
	__m128i xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
#endif

	size_t reminder = size & (regsInLoop * sizeof(xmm0) - 1); // Copy 128 or 256 bytes every loop
	size_t end = 0;

	__m128i* pTrg = (__m128i*)d;
	__m128i* pTrgEnd = pTrg + ((size - reminder) >> 4);
	__m128i* pSrc = (__m128i*)s;

	// Make sure source is synced - doesn't hurt if not needed.
	_mm_sfence();

	while (pTrg < pTrgEnd)
	{
		// _mm_stream_load_si128 emits the Streaming SIMD Extensions 4 (SSE4.1) instruction MOVNTDQA
		// Fastest method for copying GPU RAM. Available since Penryn (45nm Core 2 Duo/Quad)
		xmm0  = _mm_stream_load_si128(pSrc);
		xmm1  = _mm_stream_load_si128(pSrc + 1);
		xmm2  = _mm_stream_load_si128(pSrc + 2);
		xmm3  = _mm_stream_load_si128(pSrc + 3);
		xmm4  = _mm_stream_load_si128(pSrc + 4);
		xmm5  = _mm_stream_load_si128(pSrc + 5);
		xmm6  = _mm_stream_load_si128(pSrc + 6);
		xmm7  = _mm_stream_load_si128(pSrc + 7);
#ifdef _M_X64 // Use all 16 xmm registers
		xmm8  = _mm_stream_load_si128(pSrc + 8);
		xmm9  = _mm_stream_load_si128(pSrc + 9);
		xmm10 = _mm_stream_load_si128(pSrc + 10);
		xmm11 = _mm_stream_load_si128(pSrc + 11);
		xmm12 = _mm_stream_load_si128(pSrc + 12);
		xmm13 = _mm_stream_load_si128(pSrc + 13);
		xmm14 = _mm_stream_load_si128(pSrc + 14);
		xmm15 = _mm_stream_load_si128(pSrc + 15);
#endif
		pSrc += regsInLoop;
		// _mm_store_si128 emit the SSE2 intruction MOVDQA (aligned store)
		_mm_store_si128(pTrg	, xmm0);
		_mm_store_si128(pTrg +  1, xmm1);
		_mm_store_si128(pTrg +  2, xmm2);
		_mm_store_si128(pTrg +  3, xmm3);
		_mm_store_si128(pTrg +  4, xmm4);
		_mm_store_si128(pTrg +  5, xmm5);
		_mm_store_si128(pTrg +  6, xmm6);
		_mm_store_si128(pTrg +  7, xmm7);
#ifdef _M_X64 // Use all 16 xmm registers
		_mm_store_si128(pTrg +  8, xmm8);
		_mm_store_si128(pTrg +  9, xmm9);
		_mm_store_si128(pTrg + 10, xmm10);
		_mm_store_si128(pTrg + 11, xmm11);
		_mm_store_si128(pTrg + 12, xmm12);
		_mm_store_si128(pTrg + 13, xmm13);
		_mm_store_si128(pTrg + 14, xmm14);
		_mm_store_si128(pTrg + 15, xmm15);
#endif
		pTrg += regsInLoop;
	}

	// Copy in 16 byte steps
	if (reminder >= 16)
	{
		size = reminder;
		reminder = size & 15;
		end = size >> 4;

		for (size_t i = 0; i < end; ++i)
		{
			pTrg[i] = _mm_stream_load_si128(pSrc + i);
		}
	}

	// Copy last bytes - shouldn't happen as strides are modulu 16
	if (reminder)
	{
		__m128i temp = _mm_stream_load_si128(pSrc + end);

		char* ps = (char*)(&temp);
		char* pt = (char*)(pTrg + end);

		for (size_t i = 0; i < reminder; ++i)
		{
			pt[i] = ps[i];
		}
	}

	return d;
}
