/*
 * (C) 2016 see Authors.txt
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

template <typename T>
// Clamps the specified value to the specified minimum and maximum range.
inline T clamp(T const& val, T const& lo, T const& hi)
{
	return (val > hi) ? hi : (val < lo) ? lo : val;
}

template <typename T, typename D>
// If the specified value is out of range, set to default values.
inline T discard(T const& val, T const& lo, T const& hi, D const& def)
{
	return (val > hi || val < lo) ? def : val;
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

//extern inline uint16_t bswap_16(uint16_t value);
//extern inline uint32_t bswap_32(uint32_t value);
//extern inline uint64_t bswap_64(uint64_t value);
