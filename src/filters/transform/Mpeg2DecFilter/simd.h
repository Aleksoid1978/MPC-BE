/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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

#ifndef _SIMD_H_
#define _SIMD_H_

#include "simd_common.h"

#pragma warning(push)
#pragma warning(disable:4799)
#pragma warning(disable:4309)
#pragma warning(disable:4700)

#define SSE2I_INSTRUCTION(instruction,function) \
 static __forceinline void instruction(__m128i &dst,const __m128i &src) {dst=function(dst,		   src);} \
 static __forceinline void instruction(__m128i &dst,const void	*src) {dst=function(dst,*(__m128i*)src);}

#include "simd_instructions.h"

#undef SSE2I_INSTRUCTION

static __forceinline void movq(__m128i &dst,const __m128i &src)
{
	dst=src;
}
static __forceinline void movq(__m128i &dst,const void *src)
{
	dst=*(__m128i*)src;
}
static __forceinline void movq(const void *dst,__m128i &src)
{
	*(__m128i*)dst=src;
}
static __forceinline void movd(__m128i &dst,const void *src)
{
	dst=_mm_loadl_epi64((__m128i*)src);
}
static __forceinline void movd(void *dst,const __m128i &src)
{
	_mm_storel_epi64((__m128i*)dst,src);
}

static __forceinline void movdqu(__m128i &dst,const void *src)
{
	dst=_mm_loadu_si128((__m128i*)src);
}
static __forceinline void movdqu(__m128i &dst,const __m128i &src)
{
	dst=_mm_loadu_si128(&src);
}
static __forceinline void movdqa(__m128i &dst,const __m128i &src)
{
	dst=src;
}
static __forceinline void movdqa(__m128i &dst,const void * src)
{
	dst=_mm_load_si128((__m128i*)src);
}
static __forceinline void movdqa(void *dst,const __m128i &src)
{
	_mm_store_si128((__m128i*)dst,src);
}
static __forceinline void movntdq(void *dst,const __m128i &src)
{
	_mm_stream_si128((__m128i*)dst,src);
}

static __forceinline void psrlw(__m128i &dst,int i)
{
	dst=_mm_srli_epi16(dst,i);
}
static __forceinline void psrlq(__m128i &dst,int i)
{
	dst=_mm_srli_epi64(dst,i);
}
static __forceinline void psrad(__m128i &dst,int i)
{
	dst=_mm_srai_epi32(dst,i);
}
static __forceinline void psraw(__m128i &dst,int i)
{
	dst=_mm_srai_epi16(dst,i);
}
static __forceinline void psraw(__m128i &dst,const __m128i &src)
{
	dst=_mm_sra_epi16(dst,src);
}
static __forceinline void psllw(__m128i &dst,int i)
{
	dst=_mm_slli_epi16(dst,i);
}
static __forceinline void pslld(__m128i &dst,int i)
{
	dst=_mm_slli_epi32(dst,i);
}
static __forceinline void psllq(__m128i &dst,int i)
{
	dst=_mm_slli_epi64(dst,i);
}

static __forceinline void cvtps2dq(__m128i &dst,const __m128 &src)
{
	dst=_mm_cvtps_epi32(src);
}
static __forceinline void cvtdq2ps(__m128 &dst,const __m128i &src)
{
	dst=_mm_cvtepi32_ps(src);
}

static __forceinline void movlpd(__m128d &dst,const void *src)
{
	dst=_mm_loadl_pd(dst,(double*)src);
}
static __forceinline void movhpd(__m128d &dst,const void *src)
{
	dst=_mm_loadh_pd(dst,(double*)src);
}
static __forceinline void movlpd(void *dst,const __m128d &src)
{
	_mm_storel_pd((double*)dst,src);
}
static __forceinline void movhpd(void *dst,const __m128d &src)
{
	_mm_storeh_pd((double*)dst,src);
}

// load the same width as the register aligned
static __forceinline void movVqa(__m128i &dst, const void *ptr)
{
	dst = _mm_load_si128((const __m128i*)ptr);
}

// load the same width as the register un-aligned
static __forceinline void movVqu(__m128i &dst, const void *ptr)
{
	dst = _mm_loadu_si128((const __m128i*)ptr);
}

// store the same width as the register un-aligned
static __forceinline void movVqu(void *ptr,const __m128i &m)
{
	_mm_storeu_si128((__m128i*)ptr,m);
}

// load half width of the register (8 bytes for SSE2)
static __forceinline void movHalf(__m128i &dst, const void *ptr)
{
	dst = _mm_loadl_epi64((const __m128i*)ptr);
}

// load quarter width of the register (4 bytes for SSE2)
static __forceinline void movQuarter(__m128i &dst, const void *ptr)
{
	dst = _mm_cvtsi32_si128(*(int*)ptr);
}

static __forceinline __m128i _mm_castps_si128(__m128 &src)
{
	return (__m128i&)src;
}
static __forceinline void movlpd(__m128i &dst,const void *src)
{
	(__m128d&)dst=_mm_loadl_pd((__m128d&)dst,(double*)src);
}
static __forceinline void movhpd(__m128i &dst,const void *src)
{
	(__m128d&)dst=_mm_loadh_pd((__m128d&)dst,(double*)src);
}
static __forceinline void movlpd(void *dst,const __m128i &src)
{
	_mm_storel_pd((double*)dst,(const __m128d&)src);
}
static __forceinline void movhpd(void *dst,const __m128i &src)
{
	_mm_storeh_pd((double*)dst,(const __m128d&)src);
}

static __forceinline void movlps(__m128i &dst,const void *src)
{
	(__m128&)dst=_mm_loadl_pi((__m128&)dst,(const __m64*)src);
}
static __forceinline void movlps(void *dst,const __m128i &src)
{
	_mm_storel_pi((__m64*)dst,(const __m128&)src);
}
static __forceinline void movhps(__m128i &dst,const void *src)
{
	(__m128&)dst=_mm_loadh_pi((__m128&)dst,(const __m64*)src);
}
static __forceinline void movhps(void *dst,const __m128i &src)
{
	_mm_storeh_pi((__m64*)dst,(const __m128&)src);
}

static __forceinline void movlhps(__m128i &dst,const __m128i &src)
{
	(__m128&)dst=_mm_movelh_ps((__m128&)dst,(const __m128&)src);
}

#endif

//====================================== SSE2 ======================================
struct Tsse2 {
	typedef __m128i __m;
	typedef __m64 int2;
	typedef int64_t integer2_t;
	static const size_t size=sizeof(__m);
	static const int align=16;
	static __forceinline __m setzero_si64(void) {
		return _mm_setzero_si128();
	}
	static __forceinline __m set_pi8(char b7,char b6,char b5,char b4,char b3,char b2,char b1,char b0) {
		return _mm_set_epi8(b7,b6,b5,b4,b3,b2,b1,b0,b7,b6,b5,b4,b3,b2,b1,b0);
	}
	static __forceinline __m set_pi32(int i1,int i0) {
		return _mm_set_epi32(i1,i0,i1,i0);
	}
	static __forceinline __m set1_pi8(char b) {
		return _mm_set1_epi8(b);
	}
	static __forceinline __m set1_pi16(short s) {
		return _mm_set1_epi16(s);
	}
	static __forceinline __m set1_pi64(int64_t s) {
		__align16(int64_t,x[])= {s,s};	//__m128i _mm_set1_epi64(*(__m64*)&s); TODO: _mm_set1_epi64x
		return *(__m*)x;
	}
	static __forceinline __m packs_pu16(const __m &m1,const __m &m2) {
		return _mm_packus_epi16(m1,m2);
	}
	static __forceinline __m slli_pi16(const __m &m,int count) {
		return _mm_slli_epi16(m,count);
	}
	static __forceinline __m srli_pi16(const __m &m,int count) {
		return _mm_srli_epi16(m,count);
	}
	static __forceinline __m srli_si64(const __m &m,int count) {
		return _mm_srli_epi64(m,count);
	}
	static __forceinline __m srai_pi16(const __m &m,int count) {
		return _mm_srai_epi16(m,count);
	}
	static __forceinline __m madd_pi16(const __m &m1,const __m &m2) {
		return _mm_madd_epi16(m1,m2);
	}
	static __forceinline __m add_pi16(const __m &m1,const __m &m2) {
		return _mm_add_epi16(m1,m2);
	}
	static __forceinline __m adds_pi16(const __m &m1,const __m &m2) {
		return _mm_adds_epi16(m1,m2);
	}
	static __forceinline __m adds_pu16(const __m &m1,const __m &m2) {
		return _mm_adds_epu16(m1,m2);
	}
	static __forceinline __m adds_pu8(const __m &m1,const __m &m2) {
		return _mm_adds_epu8(m1,m2);
	}
	static __forceinline __m sub_pi16(const __m &m1,const __m &m2) {
		return _mm_sub_epi16(m1,m2);
	}
	static __forceinline __m subs_pi16(const __m &m1,const __m &m2) {
		return _mm_subs_epi16(m1,m2);
	}
	static __forceinline __m subs_pu16(const __m &m1,const __m &m2) {
		return _mm_subs_epu16(m1,m2);
	}
	static __forceinline __m subs_pu8(const __m &m1,const __m &m2) {
		return _mm_subs_epu8(m1,m2);
	}
	static __forceinline __m or_si64(const __m &m1,const __m &m2) {
		return _mm_or_si128(m1,m2);
	}
	static __forceinline __m xor_si64(const __m &m1,const __m &m2) {
		return _mm_xor_si128(m1,m2);
	}
	static __forceinline __m and_si64(const __m &m1,const __m &m2) {
		return _mm_and_si128(m1,m2);
	}
	static __forceinline __m andnot_si64(const __m &m1,const __m &m2) {
		return _mm_andnot_si128(m1,m2);
	}
	static __forceinline __m mullo_pi16(const __m &m1,const __m &m2) {
		return _mm_mullo_epi16(m1,m2);
	}
	static __forceinline __m mulhi_pi16(const __m &m1,const __m &m2) {
		return _mm_mulhi_epi16(m1,m2);
	}
	static __forceinline __m unpacklo_pi8(const __m &m1,const __m &m2) {
		return _mm_unpacklo_epi8(m1,m2);
	}
	static __forceinline __m unpackhi_pi8(const __m &m1,const __m &m2) {
		return _mm_unpackhi_epi8(m1,m2);
	}
	static __forceinline __m cmpgt_pi16(const __m &m1,const __m &m2) {
		return _mm_cmpgt_epi16(m1,m2);
	}
	static __forceinline __m cmpeq_pi16(const __m &m1,const __m &m2) {
		return _mm_cmpeq_epi16(m1,m2);
	}
	static __forceinline __m cmpeq_pi8(const __m &m1,const __m &m2) {
		return _mm_cmpeq_epi8(m1,m2);
	}
	static __forceinline __m min_pi16(const __m &mm1,const __m &mm2) {
		return _mm_min_epi16(mm1,mm2);
	}
	static __forceinline __m max_pi16(const __m &mm1,const __m &mm2) {
		return _mm_max_epi16(mm1,mm2);
	}
	static __forceinline __m load2(const void *ptr) {
		return _mm_loadl_epi64((const __m128i*)ptr);
	}
	static __forceinline void store2(void *ptr,const __m &m) {
		_mm_storel_epi64((__m128i*)ptr,m);
	}
	static __forceinline void psadbw(__m &mm3,const __m &mm2) {
		mm3=_mm_sad_epu8(mm3,mm2);
	}
	static __forceinline void prefetchnta(const void *ptr) {
		_mm_prefetch((const char*)ptr,_MM_HINT_NTA);
	}
	static __forceinline __m shuffle_pi16_0(const __m &mm0) {
		return _mm_shufflehi_epi16(_mm_shufflelo_epi16(mm0,0),0);
	}
	static __forceinline void pmaxub(__m &mmr1,const __m &mmr2) {
		mmr1=_mm_max_epu8(mmr1,mmr2);
	}
	static __forceinline void pmulhuw(__m &mmr1,const __m &mmr2) {
		mmr1=_mm_mulhi_epu16(mmr1,mmr2);
	}
	static __forceinline void movntq(void *dst,const __m &src) {
		_mm_stream_si128((__m128i*)dst,src);
	}
	static __forceinline void pavgb(__m &mmr1,const __m &mmr2) {
		mmr1=_mm_avg_epu8(mmr1,mmr2);
	}
	static __forceinline void pavgb(__m &mmr1,const void *mmr2) {
		mmr1=_mm_avg_epu8(mmr1,*(__m*)mmr2);
	}
	static __forceinline void sfence(void) {
		_mm_sfence();
	}
	// store the same width as the register without polluting the cache
	static __forceinline void movntVq(void *ptr,const __m128i &m)
	{
		_mm_stream_si128((__m128i*)ptr,m);
	}
};

#pragma warning(pop)
