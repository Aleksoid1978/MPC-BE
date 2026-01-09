/*
 * (C) 2016-2026 see Authors.txt
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
#include <numeric>

// If the specified value is out of range, set to default values.
template <typename T>
inline T discard(T const& val, T const& def, T const& lo, T const& hi)
{
	return (val > hi || val < lo) ? def : val;
}

// If the specified value is out of set, set to default values.
template <typename T>
inline T discard(T const& val, T const& def, const std::vector<T>& vars)
{
	if (val != def) {
		for (const auto& v : vars) {
			if (val == v) {
				return val;
			}
		}
	}
	return def;
}

// If the specified value is out of range, update lo or hi values.
template <typename T>
inline void expand_range(T const& val, T& lo, T& hi)
{
	if (val > hi) {
		hi = val;
	}
	if (val < lo) { // do not use "else if" because at the beginning 'lo' may be greater than 'hi'
		lo = val;
	}
}

uint32_t CountBits(uint32_t v);
uint32_t BitNum(uint32_t v, uint32_t b);

void fill_u32(void* dst, const uint32_t c, const size_t count);
void memset_u32(void* dst, const uint32_t c, const size_t nbytes);
void memset_u16(void* dst, const uint16_t c, const size_t nbytes);

// a * b / c
uint64_t RescaleU64x32(uint64_t a, uint32_t b, uint32_t c);
// a * b / c
int64_t RescaleI64x32(int64_t a, uint32_t b, uint32_t c);
// a * b / c
int64_t RescaleI64(int64_t a, int64_t b, int64_t c);

template <typename T>
inline void ReduceDim(T &num, T &den)
{
	if (den > 0 && num > 0) {
		const auto gcd = std::gcd(num, den);
		num /= gcd;
		den /= gcd;
	}
}

inline void ReduceDim(SIZE &dim)
{
	ReduceDim(dim.cx, dim.cy);
}

SIZE ReduceDim(double value);

unsigned RoundUp(const unsigned value, const unsigned base);

int IncreaseByGrid(int value, const int step);
int DecreaseByGrid(int value, const int step);

// steps < 0  mean 1.0/(-step)
double IncreaseFloatByGrid(double value, const int step);
// steps < 0  mean 1.0/(-step)
double DecreaseFloatByGrid(double value, const int step);

// checks the multiplicity of the angle of 90 degrees and brings it to the values of 0, 90, 270
bool AngleStep90(int& angle);

// Functions to convert strings to numeric values. On error, the current value does not change and false is returned.

bool StrToInt32(const wchar_t* str, int32_t& value);
bool StrToInt32(const char* str, int32_t& value);
bool StrToUInt32(const wchar_t* str, uint32_t& value);
bool StrToInt64(const wchar_t* str, int64_t& value);
bool StrToUInt64(const wchar_t* str, uint64_t& value);
bool StrHexToUInt32(const wchar_t* str, uint32_t& value);
bool StrHexToUInt64(const wchar_t* str, uint64_t& value);
bool StrToDouble(const wchar_t* str, double& value);

CStringW HR2Str(const HRESULT hr);

// Usage: SetThreadName ((DWORD)-1, "MainThread");
void SetThreadName(DWORD dwThreadID, const char* threadName);
