/*
 * (C) 2016-2018 see Authors.txt
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
#include <vector>

template <typename T>
// If the specified value is out of range, set to default values.
inline T discard(T const& val, T const& def, T const& lo, T const& hi)
{
	return (val > hi || val < lo) ? def : val;
}

template <typename T>
// If the specified value is out of set, set to default values.
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

template <typename T>
// If the specified value is out of range, update lo or hi values.
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

void ReduceDim(long &num, long &den);
void ReduceDim(SIZE &dim);

SIZE ReduceDim(double value);

int IncreaseByGrid(int value, const int step);
int DecreaseByGrid(int value, const int step);

// steps < 0  mean 1.0/(-step)
double IncreaseFloatByGrid(double value, const int step);
// steps < 0  mean 1.0/(-step)
double DecreaseFloatByGrid(double value, const int step);
