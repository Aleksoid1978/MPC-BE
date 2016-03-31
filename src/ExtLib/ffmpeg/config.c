/*
 * (C) 2009-2016 see Authors.txt
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

#include <stdio.h>
#include "libavcodec/version.h"

#ifdef __GNUC__
#define _aligned_malloc  __mingw_aligned_malloc
#define _aligned_realloc __mingw_aligned_realloc
#define _aligned_free    __mingw_aligned_free
#endif

#if defined(DEBUG) || defined(_DEBUG)
	#define COMPILER " Debug"
#else
	#define COMPILER ""
#endif

#if defined(__AVX2__)
	#define COMPILER_SSE " (AVX2)"
#elif defined(__AVX__)
	#define COMPILER_SSE " (AVX)"
#elif defined(__SSE4_2__)
	#define COMPILER_SSE " (SSE4.2)"
#elif defined(__SSE4_1__)
	#define COMPILER_SSE " (SSE4.1)"
#elif defined(__SSE4__)
	#define COMPILER_SSE " (SSE4)"
#elif defined(__SSSE3__)
	#define COMPILER_SSE " (SSSE3)"
#elif defined(__SSE3__)
	#define COMPILER_SSE " (SSE3)"
#elif !ARCH_X86_64
	#if defined(__SSE2__)
		#define COMPILER_SSE " (SSE2)"
	#elif defined(__SSE__)
		#define COMPILER_SSE " (SSE)"
	#elif defined(__MMX__)
		#define COMPILER_SSE " (MMX)"
	#else
		#define COMPILER_SSE ""
	#endif
#else
	#define COMPILER_SSE ""
#endif

static char g_Gcc_Compiler[31];
static char g_libavcodec_Version[31];

char* GetFFmpegCompiler()
{
	snprintf(g_Gcc_Compiler, sizeof(g_Gcc_Compiler), "GCC %d.%d.%d%s%s", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__, COMPILER, COMPILER_SSE);
	return g_Gcc_Compiler;
}

char* GetlibavcodecVersion()
{
	snprintf(g_libavcodec_Version, sizeof(g_libavcodec_Version), "%d.%d.%d / %d.%d.%d", LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR, LIBAVCODEC_VERSION_MICRO, LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO);
	return g_libavcodec_Version;
}
