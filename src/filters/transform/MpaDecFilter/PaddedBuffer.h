/*
 * (C) 2014-2023 see Authors.txt
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

#include <mpc_defines.h>

// A dynamic buffer with a guaranteed padded block at the end and no member initialization.
class CPaddedBuffer
{
private:
	uint8_t* m_data = nullptr;
	size_t   m_size = 0;
	size_t   m_capacity = 0;
	const size_t m_padsize;
	const size_t m_alignment;

public:
	CPaddedBuffer(size_t padsize, size_t alignment = 16)
		: m_padsize(padsize)
		, m_alignment(alignment)
	{
	}
	~CPaddedBuffer()
	{
		Clear();
	}

private:
	bool Reallocate(const size_t size)
	{
		size_t capacity = size + m_padsize;
		capacity = ALIGN(capacity, 256);

		if (capacity <= m_capacity) {
			return true;
		}

		uint8_t* ptr = (uint8_t*)_aligned_realloc(m_data, capacity, m_alignment);
		if (ptr) {
			m_data = ptr;
			m_capacity = capacity;
			return true;
		}

		return false;
	}

public:
	uint8_t* Data()
	{
		return m_data;
	}

	size_t Size()
	{
		return m_size;
	}

	bool Resize(const size_t size)
	{
		bool ok = Reallocate(size);
		if (ok) {
			m_size = size;
			memset(m_data + m_size, 0, m_padsize);
		}

		return ok;
	}

	void Clear()
	{
		_aligned_free(m_data);
		m_data = nullptr;
		m_size = 0;
		m_capacity = 0;
	}

	bool Append(uint8_t* p, const size_t count)
	{
		const size_t newsize = m_size + count;
		bool ok = Reallocate(newsize);
		if (ok) {
			memcpy(m_data + m_size, p, count);
			m_size = newsize;
			memset(m_data + m_size, 0, m_padsize);
		}
		return ok;
	}

	void RemoveHead(const size_t count)
	{
		if (count < m_size) {
			const size_t newsize = m_size - count;
			memmove(m_data, m_data + count, newsize);
			m_size = newsize;
			memset(m_data + m_size, 0, m_padsize);
		} 
		else {
			m_size = 0;
		}
	}
};
