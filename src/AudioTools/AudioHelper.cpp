/*
 * (C) 2014-2016 see Authors.txt.
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
#include <MMReg.h>
#include "AudioHelper.h"

#define limit(a, x, b) if (x < a) { x = a; } else if (x > b) { x = b; }

static bool haveSSE2()
{
	int info[4] = { 0 };
	__cpuid(info, 0);
	if (info[0] >= 1) {
		__cpuid(info, 0x00000001);
		return ((info[3] & (1 << 26)) != 0);
	}
	return false;
}

static const bool bSSE2 = haveSSE2();

static const __m128 __32bitScalar    = _mm_set_ps1(INT32_PEAK);
static const __m128 __32bitScalarDiv = _mm_set_ps1(1.0f / INT32_PEAK);
static const __m128 __32bitMax       = _mm_set_ps1(-1.0f);
static const __m128 __32bitMin       = _mm_set_ps1((float)INT24_MAX / INT24_PEAK);

inline static void convert_float_to_int32_sse2(int32_t* pOut, float* pIn, const size_t allsamples)
{
	__m128  __tmpIn  = _mm_setzero_ps();
	__m128i __tmpOut = _mm_setzero_si128();

	size_t k = 0;
	for (; k < allsamples - 3; k += 4) {
		__tmpIn  = _mm_loadu_ps(&pIn[k]);                                   // in = pIn
		__tmpIn  = _mm_min_ps(_mm_max_ps(__tmpIn, __32bitMax), __32bitMin); // in = min(max(in, -1.0f), 24MAX)
		__tmpIn  = _mm_mul_ps(__tmpIn, __32bitScalar);                      // in = in * INT32_PEAK

		__tmpOut = _mm_cvtps_epi32(__tmpIn);                                // out = in
		_mm_storeu_si128((__m128i*)&pOut[k], __tmpOut);                     // pOut = out
	}

	for (; k < allsamples; k++) {
		pOut[k] = SAMPLE_float_to_int32(pIn[k]);
	}
}

inline static void convert_int32_to_float_sse2(float* pOut, int32_t* pIn, const size_t allsamples)
{
	__m128i __tmpIn  = _mm_setzero_si128();
	__m128  __tmpOut = _mm_setzero_ps();

	size_t k = 0;
	for (; k < allsamples - 3; k += 4) {
		__tmpIn  = _mm_load_si128((const __m128i*)&pIn[k]); // in = pIn
		
		__tmpOut = _mm_cvtepi32_ps(__tmpIn);                // out = in
		__tmpOut = _mm_mul_ps(__tmpOut, __32bitScalarDiv);  // out = out / INT32_PEAK;
		_mm_storeu_ps(&pOut[k], __tmpOut);                  // pOut = out
	}

	for (; k < allsamples; k++) {
		pOut[k] = SAMPLE_int32_to_float(pIn[k]);
	}
}

inline static void convert_float_to_int24_sse2(BYTE* pOut, float* pIn, const size_t allsamples)
{
	__m128  __tmpIn  = _mm_setzero_ps();
	__m128i __tmpOut = _mm_setzero_si128();

	int32_t tmp[4] = { 0 };

	size_t k = 0;
	for (; k < allsamples - 3; k += 4) {
		__tmpIn  = _mm_loadu_ps(&pIn[k]);                                   // in = pIn
		__tmpIn  = _mm_min_ps(_mm_max_ps(__tmpIn, __32bitMax), __32bitMin); // in = min(max(in, -1.0f), 24MAX)
		__tmpIn  = _mm_mul_ps(__tmpIn, __32bitScalar);                      // in = in * INT32_PEAK

		__tmpOut = _mm_cvtps_epi32(__tmpIn);                                // out = in
		_mm_storeu_si128((__m128i*)&tmp, __tmpOut);                         // tmp = out

		for (size_t i = 0; i < 4; i++) {                                    // pOut = tmp
			int32_to_int24(tmp[i], pOut)
		}
	}

	for (; k < allsamples; k++) {
		int32_t i32 = SAMPLE_float_to_int32(pIn[k]);
		int32_to_int24(i32, pOut);
	}
}

static const __m128  __16bitScalar = _mm_set_ps1(INT16_PEAK);
static const __m128  __16bitMax    = __32bitMax;
static const __m128  __16bitMin    = _mm_set_ps1(F16MAX);
static const __m128i __zero        = _mm_setzero_si128();

inline static void convert_float_to_int16_sse2(int16_t* pOut, float* pIn, const size_t allsamples)
{
	__m128  __tmpInLo = _mm_setzero_ps();
	__m128  __tmpInHi = _mm_setzero_ps();
	__m128i __tmpOut  = _mm_setzero_si128();

	size_t k = 0;
	for (; k < allsamples - 7; k += 8) {
		__tmpInLo = _mm_loadu_ps(&pIn[k]);                                                   // in = pIn
		__tmpInLo = _mm_min_ps(_mm_max_ps(__tmpInLo, __16bitMax), __16bitMin);               // in = min(max(in, -1.0f), 16MAX)
		__tmpInLo = _mm_mul_ps(__tmpInLo, __16bitScalar);                                    // in = in * INT16_PEAK

		__tmpInHi = _mm_loadu_ps(&pIn[k + 4]);                                               // in = pIn
		__tmpInHi = _mm_min_ps(_mm_max_ps(__tmpInHi, __16bitMax), __16bitMin);               // in = min(max(in, -1.0f), 16MAX)
		__tmpInHi = _mm_mul_ps(__tmpInHi, __16bitScalar);                                    // in = in * INT16_PEAK

		__tmpOut  = _mm_packs_epi32(_mm_cvtps_epi32(__tmpInLo), _mm_cvtps_epi32(__tmpInHi)); // out = in
		_mm_storeu_si128((__m128i*)&pOut[k], __tmpOut);                                      // pOut = out
	}

	for (; k < allsamples; k++) {
		pOut[k] = SAMPLE_float_to_int16(pIn[k]);
	}
}

inline static void convert_int16_to_float_sse2(float* pOut, int16_t* pIn, const size_t allsamples)
{
	__m128i __tmpIn    = _mm_setzero_si128();
	__m128  __tmpOutLo = _mm_setzero_ps();
	__m128  __tmpOutHi = _mm_setzero_ps();

	size_t k = 0;
	for (; k < allsamples - 7; k += 8) {
		__tmpIn    = _mm_load_si128((const __m128i*)&pIn[k]);              // in = pIn

		__tmpOutLo = _mm_cvtepi32_ps(_mm_unpacklo_epi16(__zero, __tmpIn)); // out = in
		__tmpOutLo = _mm_mul_ps(__tmpOutLo, __32bitScalarDiv);             // out = out / INT32_PEAK;

		__tmpOutHi = _mm_cvtepi32_ps(_mm_unpackhi_epi16(__zero, __tmpIn)); // out = in
		__tmpOutHi = _mm_mul_ps(__tmpOutHi, __32bitScalarDiv);             // out = out / INT32_PEAK;

		_mm_storeu_ps(&pOut[k], __tmpOutLo);                               // pOut = out
		_mm_storeu_ps(&pOut[k + 4], __tmpOutHi);                           // pOut = out
	}

	for (; k < allsamples; k++) {
		pOut[k] = SAMPLE_int16_to_float(pIn[k]);
	}
}

SampleFormat GetSampleFormat(const WAVEFORMATEX* wfe)
{
	SampleFormat sample_format = SAMPLE_FMT_NONE;

	const WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;
	WORD tag = wfe->wFormatTag;

	if (tag == WAVE_FORMAT_PCM || (tag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)) {
		switch (wfe->wBitsPerSample) {
		case 8:
			sample_format = SAMPLE_FMT_U8;
			break;
		case 16:
			sample_format = SAMPLE_FMT_S16;
			break;
		case 24:
			sample_format = SAMPLE_FMT_S24;
			break;
		case 32:
			sample_format = SAMPLE_FMT_S32;
			break;
		}
	} else if (tag == WAVE_FORMAT_IEEE_FLOAT || (tag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
		switch (wfe->wBitsPerSample) {
		case 32:
			sample_format = SAMPLE_FMT_FLT;
			break;
		case 64:
			sample_format = SAMPLE_FMT_DBL;
			break;
		}
	}

	return sample_format;
}

HRESULT convert_to_int16(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, int16_t* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			convert_uint8_to_int16(pOut, (uint8_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S16:
			memcpy(pOut, pIn, allsamples * sizeof(int16_t));
			break;
		case SAMPLE_FMT_S24:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = *(int16_t*)(pIn + 1); // read the high bits only
				pIn += 3;
			}
			break;
		case SAMPLE_FMT_S32:
			convert_int32_to_int16(pOut, (int32_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_FLT:
			if (bSSE2) {
				convert_float_to_int16_sse2(pOut, (float*)pIn, allsamples);
			} else {
				convert_float_to_int16(pOut, (float*)pIn, allsamples);
			}
			break;
		case SAMPLE_FMT_DBL:
			convert_double_to_int16(pOut, (double*)pIn, allsamples);
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				uint8_t* p = (uint8_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_uint8_to_int16(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				int16_t* p = (int16_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = p[nSamples * ch];
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				int32_t* p = (int32_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_int32_to_int16(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				float* p = (float*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_float_to_int16(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				double* p = (double*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_double_to_int16(p[nSamples * ch]);
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_int24(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, BYTE* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = 0;
				*pOut++ = 0;
				*pOut++ = (*pIn++) ^ 0x80;
			}
			break;
		case SAMPLE_FMT_S16:
			for (size_t i = 0; i < allsamples; ++i) {
				*pOut++ = 0;
				*pOut++ = *pIn++;
				*pOut++ = *pIn++;
			}
			break;
		case SAMPLE_FMT_S24:
			memcpy(pOut, pIn, allsamples * 3);
			break;
		case SAMPLE_FMT_S32:
			for (size_t i = 0; i < allsamples; ++i) {
				pIn++;
				*pOut++ = *pIn++;
				*pOut++ = *pIn++;
				*pOut++ = *pIn++;
			}
			break;
		case SAMPLE_FMT_FLT:
			for (size_t i = 0; i < allsamples; ++i) {
				double d = (double)(*(float*)pIn);
				limit(-1, d, D32MAX);
				uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
				int32_to_int24(u32, pOut);
				pIn += sizeof(float);
			}
			break;
		case SAMPLE_FMT_DBL:
			for (size_t i = 0; i < allsamples; ++i) {
				double d = *(double*)pIn;
				limit(-1, d, D32MAX);
				uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
				int32_to_int24(u32, pOut);
				pIn += sizeof(double);
			}
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = 0;
					*pOut++ = 0;
					*pOut++ = pIn[nSamples * ch + i] ^ 0x80;
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					uint16_t u16 = ((uint16_t*)pIn)[nSamples * ch + i];
					*pOut++ = 0;
					*pOut++ = (BYTE)(u16);
					*pOut++ = (BYTE)(u16 >> 8);
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					uint32_t u32 = ((uint32_t*)pIn)[nSamples * ch + i];
					int32_to_int24(u32, pOut);
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					double d = (double)((float*)pIn)[nSamples * ch + i];
					limit(-1, d, D32MAX);
					uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
					int32_to_int24(u32, pOut);
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				for (int ch = 0; ch < nChannels; ++ch) {
					double d = ((double*)pIn)[nSamples * ch + i];
					limit(-1, d, D32MAX);
					uint32_t u32 = (uint32_t)(int32_t)round_d(d * INT32_PEAK);
					int32_to_int24(u32, pOut);
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_int32(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, int32_t* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			convert_uint8_to_int32(pOut, (uint8_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S16:
			convert_int16_to_int32(pOut, (int16_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S24:
			for (size_t i = 0; i < allsamples; ++i) {
				pOut[i] = (uint32_t)pIn[3 * i]     << 8  |
				          (uint32_t)pIn[3 * i + 1] << 16 |
				          (uint32_t)pIn[3 * i + 2] << 24;
			}
			break;
		case SAMPLE_FMT_S32:
			memcpy(pOut, pIn, nSamples * nChannels * sizeof(int32_t));
			break;
		case SAMPLE_FMT_FLT:
			if (bSSE2) {
				convert_float_to_int32_sse2(pOut, (float*)pIn, allsamples);
			} else {
				convert_float_to_int32(pOut, (float*)pIn, allsamples);
			}
			break;
		case SAMPLE_FMT_DBL:
			convert_double_to_int32(pOut, (double*)pIn, allsamples);
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				uint8_t* p = (uint8_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_uint8_to_int32(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				int16_t* p = (int16_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_int16_to_int32(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				int32_t* p = (int32_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = p[nSamples * ch];
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				float* p = (float*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_float_to_int32(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				double* p = (double*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_double_to_int32(p[nSamples * ch]);
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_float(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, float* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			convert_uint8_to_float(pOut, (uint8_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S16:
			if (bSSE2) {
				convert_int16_to_float_sse2(pOut, (int16_t*)pIn, allsamples);
			} else {
				convert_int16_to_float(pOut, (int16_t*)pIn, allsamples);
			}
			break;
		case SAMPLE_FMT_S24:
			convert_int24_to_float(pOut, pIn, allsamples);
			break;
		case SAMPLE_FMT_S32:
			if (bSSE2) {
				convert_int32_to_float_sse2(pOut, (int32_t*)pIn, allsamples);
			} else {
				convert_int32_to_float(pOut, (int32_t*)pIn, allsamples);
			}
			break;
		case SAMPLE_FMT_FLT:
			memcpy(pOut, pIn, allsamples * sizeof(float));
			break;
		case SAMPLE_FMT_DBL:
			convert_double_to_float(pOut, (double*)pIn, allsamples);
			break;
		// planar
		case SAMPLE_FMT_U8P:
			for (size_t i = 0; i < nSamples; ++i) {
				uint8_t* p = (uint8_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_uint8_to_float(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S16P:
			for (size_t i = 0; i < nSamples; ++i) {
				int16_t* p = (int16_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_int16_to_float(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_S32P:
			for (size_t i = 0; i < nSamples; ++i) {
				int32_t* p = (int32_t*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = SAMPLE_int32_to_float(p[nSamples * ch]);
				}
			}
			break;
		case SAMPLE_FMT_FLTP:
			for (size_t i = 0; i < nSamples; ++i) {
				float* p = (float*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = p[nSamples * ch];
				}
			}
			break;
		case SAMPLE_FMT_DBLP:
			for (size_t i = 0; i < nSamples; ++i) {
				double* p = (double*)pIn + i;
				for (int ch = 0; ch < nChannels; ++ch) {
					*pOut++ = (float)p[nSamples * ch];
				}
			}
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_to_planar_float(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, float* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			for (int ch = 0; ch < nChannels; ++ch) {
				uint8_t* p = (uint8_t*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = SAMPLE_uint8_to_float(p[nChannels * i]);
				}
			}
			break;
		case SAMPLE_FMT_S16:
			for (int ch = 0; ch < nChannels; ++ch) {
				int16_t* p = (int16_t*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = SAMPLE_int16_to_float(p[nChannels * i]);
				}
			}
			break;
		case SAMPLE_FMT_S32:
			for (int ch = 0; ch < nChannels; ++ch) {
				int32_t* p = (int32_t*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = SAMPLE_int32_to_float(p[nChannels * i]);
				}
			}
			break;
		case SAMPLE_FMT_FLT:
			for (int ch = 0; ch < nChannels; ++ch) {
				float* p = (float*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = p[nChannels * i];
				}
			}
			break;
		case SAMPLE_FMT_DBL:
			for (int ch = 0; ch < nChannels; ++ch) {
				double* p = (double*)pIn + ch;
				for (size_t i = 0; i < nSamples; ++i) {
					*pOut++ = (float)p[nChannels * i];
				}
			}
			break;
		// planar
		case SAMPLE_FMT_U8P:
			convert_uint8_to_float(pOut, (uint8_t*)pIn, allsamples);
			break;
		case SAMPLE_FMT_S16P:
			if (bSSE2) {
				convert_int16_to_float_sse2(pOut, (int16_t*)pIn, allsamples);
			} else {
				convert_int16_to_float(pOut, (int16_t*)pIn, allsamples);
			}
			break;
		case SAMPLE_FMT_S32P:
			if (bSSE2) {
				convert_int32_to_float_sse2(pOut, (int32_t*)pIn, allsamples);
			} else {
				convert_int32_to_float(pOut, (int32_t*)pIn, allsamples);
			}
			break;
		case SAMPLE_FMT_FLTP:
			memcpy(pOut, pIn, allsamples * sizeof(float));
			break;
		case SAMPLE_FMT_DBLP:
			convert_double_to_float(pOut, (double*)pIn, allsamples);
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}

HRESULT convert_float_to(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, float* pIn, BYTE* pOut)
{
	size_t allsamples = nSamples * nChannels;

	switch (sfmt) {
		case SAMPLE_FMT_U8:
			convert_float_to_uint8((uint8_t*)pOut, pIn, allsamples);
			break;
		case SAMPLE_FMT_S16:
			if (bSSE2) {
				convert_float_to_int16_sse2((int16_t*)pOut, (float*)pIn, allsamples);
			} else {
				convert_float_to_int16((int16_t*)pOut, pIn, allsamples);
			}
			break;
		case SAMPLE_FMT_S24:
			if (bSSE2) {
				convert_float_to_int24_sse2(pOut, pIn, allsamples);
			} else {
				convert_float_to_int24(pOut, pIn, allsamples);
			}
			break;
		case SAMPLE_FMT_S32:
			if (bSSE2) {
				convert_float_to_int32_sse2((int32_t*)pOut, pIn, allsamples);
			} else {
				convert_float_to_int32((int32_t*)pOut, pIn, allsamples);
			}
			break;
		case SAMPLE_FMT_FLT:
			memcpy(pOut, pIn, allsamples * sizeof(float));
			break;
		case SAMPLE_FMT_DBL:
			convert_float_to_double((double*)pOut, pIn, allsamples);
			break;
		default:
			return E_INVALIDARG;
	}
	return S_OK;
}
