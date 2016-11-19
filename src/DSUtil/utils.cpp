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

#include "stdafx.h"
#include "utils.h"

uint32_t CountBits(uint32_t v)
{
	// used code from \VirtualDub\h\vd2\system\bitmath.h (VDCountBits)
	v -= (v >> 1) & 0x55555555;
	v = ((v & 0xcccccccc) >> 2) + (v & 0x33333333);
	v = (v + (v >> 4)) & 0x0f0f0f0f;
	return (v * 0x01010101) >> 24;
}

//#ifdef _MSC_VER
//inline uint16_t bswap_16(uint16_t value) { return (uint16_t)_byteswap_ushort((unsigned short)value); }
//inline uint32_t bswap_32(uint32_t value) { return (uint32_t)_byteswap_ulong((unsigned long)value); }
//inline uint64_t bswap_64(uint64_t value) { return (uint64_t)_byteswap_uint64((unsigned __int64)value); }
//#else
//inline uint16_t bswap_16(uint16_t value) {
//	return (value >> 8) + (value << 8);
//}
//inline uint32_t bswap_32(uint32_t value) {
//	return (value >> 24) + (value << 24) + ((value & 0xff00) << 8) + ((value & 0xff0000) >> 8);
//}
//inline uint64_t bswap_64(uint64_t value) {
//	return	((value & 0xFF00000000000000) >> 56) +
//		((value & 0x00FF000000000000) >> 40) +
//		((value & 0x0000FF0000000000) >> 24) +
//		((value & 0x000000FF00000000) >> 8) +
//		((value & 0x00000000FF000000) << 8) +
//		((value & 0x0000000000FF0000) << 24) +
//		((value & 0x000000000000FF00) << 40) +
//		((value & 0x00000000000000FF) << 56);
//}
//#endif
