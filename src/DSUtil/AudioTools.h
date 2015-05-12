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

#pragma once

#include <stdint.h>

#define INT24_MAX       8388607
#define INT24_MIN     (-8388608)

void gain_uint8 (const double factor, const size_t allsamples, uint8_t* pData);
void gain_int16 (const double factor, const size_t allsamples, int16_t* pData);
void gain_int24 (const double factor, const size_t allsamples, BYTE*    pData);
void gain_int32 (const double factor, const size_t allsamples, int32_t* pData);
void gain_float (const double factor, const size_t allsamples, float*   pData);
void gain_double(const double factor, const size_t allsamples, double*  pData);

double get_max_peak_uint8 (uint8_t* pData, const size_t allsamples);
double get_max_peak_int16 (int16_t* pData, const size_t allsamples);
double get_max_peak_int24 (BYTE*    pData, const size_t allsamples);
double get_max_peak_int32 (int32_t* pData, const size_t allsamples);
double get_max_peak_float (float*   pData, const size_t allsamples);
double get_max_peak_double(double*  pData, const size_t allsamples);
