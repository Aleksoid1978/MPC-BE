/*
 * (C) 2014-2016 see Authors.txt
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

#pragma once

#include <stdint.h>
#include <math.h>
#include "SampleFormat.h"

#ifdef _MSC_VER
#define bswap_16(x) _byteswap_ushort((unsigned short)(x))
#define bswap_32(x) _byteswap_ulong ((unsigned long)(x))
#define bswap_64(x) _byteswap_uint64((unsigned __int64)(x))
#else
#define bswap_16(x) ((uint16_t)(x) >> 8 | (uint16_t)(x) << 8)
#define bswap_32(x) ((uint32_t)(x) >> 24              | \
                    ((uint32_t)(x) & 0x00ff0000) >> 8 | \
                    ((uint32_t)(x) & 0x0000ff00) << 8 | \
                     (uint32_t)(x) << 24)
#define bswap_64(x) ((uint64_t)(x) >> 56                       | \
                    ((uint64_t)(x) & 0x00FF000000000000) >> 40 | \
                    ((uint64_t)(x) & 0x0000FF0000000000) >> 24 | \
                    ((uint64_t)(x) & 0x000000FF00000000) >>  8 | \
                    ((uint64_t)(x) & 0x00000000FF000000) <<  8 | \
                    ((uint64_t)(x) & 0x0000000000FF0000) << 24 | \
                    ((uint64_t)(x) & 0x000000000000FF00) << 40 | \
                     (uint64_t)(x) << 56)
#endif

#define INT8_PEAK  128
#define INT16_PEAK 32768
#define INT24_PEAK 8388608
#define INT32_PEAK 2147483648
#define INT24_MAX  8388607

static const float  F8MAX  =  float(INT8_MAX ) / INT8_PEAK;
static const float  F16MAX =  float(INT16_MAX) / INT16_PEAK;
static const double D32MAX = double(INT32_MAX) / INT32_PEAK;

#define round_f(x) ((x) > 0 ? (x) + 0.5f : (x) - 0.5f)
#define round_d(x) ((x) > 0 ? (x) + 0.5  : (x) - 0.5)

#define SAMPLE_int16_to_uint8(sample)  ((uint8_t)((sample) >> 8) ^ 0x80)
#define SAMPLE_float_to_uint8(sample)  ((sample) < -1.0f ? 0 : (sample) > F8MAX ? UINT8_MAX : (uint8_t)(int8_t)round_f((sample) * INT8_PEAK) ^ 0x80)

#define SAMPLE_uint8_to_int16(sample)  ((int16_t)((sample) ^ 0x80) << 8)
#define SAMPLE_int32_to_int16(sample)  ((int16_t)((sample) >> 16))
#define SAMPLE_float_to_int16(sample)  ((sample) < -1.0f ? INT16_MIN : (sample) > F16MAX ? INT16_MAX : (int16_t)round_f((sample) * INT16_PEAK))
#define SAMPLE_double_to_int16(sample) ((sample) < -1.0  ? INT16_MIN : (sample) > F16MAX ? INT16_MAX : (int16_t)round_d((sample) * INT16_PEAK))

#define SAMPLE_uint8_to_int32(sample)  ((int32_t)((sample) ^ 0x80) << 24)
#define SAMPLE_int16_to_int32(sample)  ((int32_t)(sample) << 16)
#define SAMPLE_float_to_int32(sample)  ((sample) < -1.0f ? INT32_MIN : (sample) > D32MAX ? INT32_MAX : (int32_t)round_d(double(sample) * INT32_PEAK))
#define SAMPLE_double_to_int32(sample) ((sample) < -1.0  ? INT32_MIN : (sample) > D32MAX ? INT32_MAX : (int32_t)round_d((sample) * INT32_PEAK))

#define SAMPLE_uint8_to_float(sample)  ((float)(int8_t((sample) ^ 0x80)) / INT8_PEAK)
#define SAMPLE_int16_to_float(sample)  ((float)(sample) / INT16_PEAK)
#define SAMPLE_int32_to_float(sample)  ((float)(double(sample) / INT32_PEAK))
#define SAMPLE_float_to_float(sample)  (sample)
#define SAMPLE_double_to_float(sample) ((float)(sample))

#define SAMPLE_float_to_double(sample) ((double)(sample))

//#define SAMPLE_int24_to_int32(p) (int32_t((uint32_t)(*p++) << 8 | (uint32_t)(*p++) << 16 | (uint32_t)(*p++) << 24))
//#define SAMPLE_int24_to_float(p) SAMPLE_int32_to_float(SAMPLE_int24_to_int32(p))


#define SAMPLECONVERTFUNCT(in, out) \
inline void convert_##in##_to_##out## (##out##_t* output, ##in##_t* input, size_t allsamples) \
{ \
    for (##in##_t* end = input + allsamples; input < end; ++input) { \
        *output++ = SAMPLE_##in##_to_##out##(*input); \
    } \
} \

SAMPLECONVERTFUNCT(int16, uint8)
SAMPLECONVERTFUNCT(float, uint8)

SAMPLECONVERTFUNCT(uint8, int16)
SAMPLECONVERTFUNCT(int32, int16)
SAMPLECONVERTFUNCT(float, int16)
SAMPLECONVERTFUNCT(double, int16)

SAMPLECONVERTFUNCT(uint8, int32)
SAMPLECONVERTFUNCT(int16, int32)
SAMPLECONVERTFUNCT(float, int32)
SAMPLECONVERTFUNCT(double, int32)

SAMPLECONVERTFUNCT(uint8, float)
SAMPLECONVERTFUNCT(int16, float)
SAMPLECONVERTFUNCT(int32, float)
SAMPLECONVERTFUNCT(double, float)

SAMPLECONVERTFUNCT(float, double)

SampleFormat GetSampleFormat(const WAVEFORMATEX* wfe);

HRESULT convert_to_int16(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, int16_t* pOut);
HRESULT convert_to_int24(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, BYTE* pOut);
HRESULT convert_to_int32(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, int32_t* pOut);
HRESULT convert_to_float(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, float* pOut);

HRESULT convert_to_planar_float(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, BYTE* pIn, float* pOut);

HRESULT convert_float_to(const SampleFormat sfmt, const WORD nChannels, const DWORD nSamples, float* pIn, BYTE* pOut);

#define int24_to_int32(pIn)  \
    (uint32_t)pIn[0] <<  8 | \
    (uint32_t)pIn[1] << 16 | \
    (uint32_t)pIn[2] << 24   \

inline void convert_int24_to_int32(int32_t* pOut, BYTE* pIn, size_t allsamples)
{
    for (size_t i = 0; i < allsamples; i++) {
        pOut[i] = int24_to_int32((&(pIn[3 * i])));
    }
}

#define int32_to_int24(i32, pOut) \
    *pOut++ = (BYTE)(i32 >>  8);  \
    *pOut++ = (BYTE)(i32 >> 16);  \
    *pOut++ = (BYTE)(i32 >> 24);  \

inline void convert_int32_to_int24(BYTE* pOut, int32_t* pIn, size_t allsamples)
{
    for (size_t i = 0; i < allsamples; i++) {
        int32_to_int24(pIn[i], pOut);
    }
}

inline void convert_int24_to_float(float* pOut, BYTE* pIn, size_t allsamples)
{
    for (size_t i = 0; i < allsamples; i++) {
        int32_t i32 = int24_to_int32((&(pIn[3 * i])));
        pOut[i] = SAMPLE_int32_to_float(i32);
    }
}

inline void convert_float_to_int24(BYTE* pOut, float* pIn, size_t allsamples)
{
    for (size_t i = 0; i < allsamples; i++) {
        int32_t i32 = SAMPLE_float_to_int32(pIn[i]);
        int32_to_int24(i32, pOut);
    }
}
