/*
 * (C) 2021 see Authors.txt
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

class CBitsWriter
{
	uint8_t* m_buffer = nullptr;
	size_t m_bufferSize = 0;

	size_t m_bitPosition = 0;
public:
	CBitsWriter(uint8_t* buf, const size_t size)
		: m_buffer(buf)
		, m_bufferSize(size)
	{}

	bool writeBits(size_t numBits, uint64_t value) {
		if (numBits > 64 || (m_bitPosition + numBits) > m_bufferSize * 8) {
			return false;
		}

		auto bytePos = m_bitPosition >> 3;
		auto bitOffset = 8 - (m_bitPosition & 7);

		m_bitPosition += numBits;

		for (; numBits > bitOffset; bitOffset = 8) {
			auto bitMask = (1ull << bitOffset) - 1;
			m_buffer[bytePos] &= ~bitMask;
			m_buffer[bytePos++] |= (value >> (numBits - bitOffset)) & bitMask;

			numBits -= bitOffset;
		}
		if (numBits == bitOffset) {
			auto bitMask = (1ull << bitOffset) - 1;
			m_buffer[bytePos] &= ~bitMask;
			m_buffer[bytePos] |= value & bitMask;
		} else {
			auto bitMask = (1ull << numBits) - 1;
			m_buffer[bytePos] &= ~(bitMask << (bitOffset - numBits));
			m_buffer[bytePos] |= (value & bitMask) << (bitOffset - numBits);
		}

		return true;
	}
};
