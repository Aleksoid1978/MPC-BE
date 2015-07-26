/*
 *
 * (C) 2013 v0lt
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
#include "math.h"
#include "AudioTools.h"

#define INT8_PEAK       128
#define INT16_PEAK      32768
#define INT24_PEAK      8388608
#define INT32_PEAK      2147483648

#define limit(a, x, b) if ((x) < (a)) { x = a; } else if ((x) > (b)) { x = b;}

// gains

void gain_uint8(const double factor, const size_t allsamples, uint8_t* pData)
{
    uint8_t* end = pData + allsamples;
    for (; pData < end; ++pData) {
        double d = factor * (int8_t)(*pData ^ 0x80);
        limit(INT8_MIN, d, INT8_MAX);
        *pData = (uint8_t)(int8_t)d ^ 0x80;
    }
}

void gain_int16(const double factor, const size_t allsamples, int16_t* pData)
{
    int16_t* end = pData + allsamples;
    for (; pData < end; ++pData) {
        double d = factor * (*pData);
        limit(INT16_MIN, d, INT16_MAX);
        *pData = (int16_t)d;
    }
}

void gain_int24(const double factor, const size_t allsamples, BYTE* pData)
{
    BYTE* end = pData + allsamples * 3;
    while (pData < end) {
        int32_t i32 = 0;
        BYTE* p = (BYTE*)(&i32);
        p[1] = *(pData);
        p[2] = *(pData + 1);
        p[3] = *(pData + 2);
        double d = factor * i32;
        limit(INT32_MIN, d, INT32_MAX);
        i32 = (int32_t)d;
        *pData++ = p[1];
        *pData++ = p[2];
        *pData++ = p[3];
    }
}

void gain_int32(const double factor, const size_t allsamples, int32_t* pData)
{
    int32_t* end = pData + allsamples;
    for (; pData < end; ++pData) {
        double d = factor * (*pData);
        limit(INT32_MIN, d, INT32_MAX);
        *pData = (int32_t)d;
    }
}

void gain_float(const double factor, const size_t allsamples, float* pData)
{
    float* end = pData + allsamples;
    for (; pData < end; ++pData) {
        double d = factor * (*pData);
        limit(-1.0, d, 1.0);
        *pData = (float)d;
    }
}

void gain_double(const double factor, const size_t allsamples, double* pData)
{
    double* end = pData + allsamples;
    for (; pData < end; ++pData) {
        double d = factor * (*pData);
        limit(-1.0, d, 1.0);
        *pData = d;
    }
}

// get_peaks

double get_max_peak_uint8(uint8_t* pData, const size_t allsamples)
{
    int max_peak = 0;

    for (uint8_t* end = pData + allsamples; pData < end; ++pData) {
        int peak = abs((int8_t)(*pData ^ 0x80));
        if (peak > max_peak) {
            max_peak = peak;
        }
    }

    return (double)max_peak / INT8_PEAK;
}

double get_max_peak_int16(int16_t* pData, const size_t allsamples)
{
    int max_peak = 0;

    for (int16_t* end = pData + allsamples; pData < end; ++pData) {
        int peak = abs(*pData);
        if (peak > max_peak) {
            max_peak = peak;
        }
    }

    return (double)max_peak / INT16_PEAK;
}

double get_max_peak_int24(BYTE* pData, const size_t allsamples)
{
    int max_peak = 0;

    BYTE* end = pData + allsamples * 3;
    while (pData < end) {
        int32_t i32 = 0;
        BYTE* p = (BYTE*)(&i32);
        p[1] = *(pData++);
        p[2] = *(pData++);
        p[3] = *(pData++);
        int peak = abs(i32 >> 8);
        if (peak > max_peak) {
            max_peak = peak;
        }
    }

    return (double)max_peak / INT24_PEAK;
}

double get_max_peak_int32 (int32_t* pData, const size_t allsamples)
{
    int max_peak = 0;

    for (int32_t* end = pData + allsamples; pData < end; ++pData) {
        if ((*pData) == INT32_MIN) {
            return 1.0;
        }
        int peak = abs(*pData);
        if (peak > max_peak) {
            max_peak = peak;
        }
    }

    return (double)max_peak / INT32_PEAK;
}

double get_max_peak_float(float* pData, const size_t allsamples)
{
    float max_peak = 0.0f;

    for (float* end = pData + allsamples; pData < end; ++pData) {
        float peak = fabsf(*pData);
        if (peak > max_peak) {
            max_peak = peak;
        }
    }

    return (double)max_peak;
}

double get_max_peak_double(double* pData, const size_t allsamples)
{
    double max_peak = 0.0;

    for (double* end = pData + allsamples; pData < end; ++pData) {
        double peak = fabs(*pData);
        if (peak > max_peak) {
            max_peak = peak;
        }
    }

    return max_peak;
}
