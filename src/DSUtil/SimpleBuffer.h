/*
 * (C) 2017 see Authors.txt
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

// CSimpleBuffer - fast and simple container
// PS: When resizing the old data will be lost.

template <typename T>
class CSimpleBuffer
{
	T* m_data = nullptr;
	size_t m_size = 0;

public:
	CSimpleBuffer() {};
	~CSimpleBuffer()
	{
		if (m_data) {
			delete[] m_data;
		}
	}

	// Returns pointer to the data.
	T* Data() { return m_data; }

	// Returns the number of elements.
	size_t Size() { return m_size; }

	// Returns allocated size in bytes.
	size_t Bytes() { return m_size * sizeof(T); }

	// Set new size. Old data will be lost.
	void SetSize(const size_t size)
	{
		if (m_data) {
			delete[] m_data;
		}

		if (size) {
			m_data = DNew T[size];
		} else {
			m_data = nullptr;
		}

		m_size = size;
	}

	// Increase the size if necessary. Old data may be lost.
	void ExpandSize(const size_t size)
	{
		if (size > m_size) {
			SetSize(size);
		}
	}

	// Write to the buffer from the specified position. The data before the specified position will be saved.
	void WriteData(size_t pos, const T* data, const size_t size)
	{
		size_t required_size = pos + size;

		if (required_size > m_size) {
			T* new_data = DNew T[required_size];
			memcpy(new_data, m_data, pos * sizeof(T));
			delete[] m_data;
			m_data = new_data;
			m_size = required_size;
		}

		memcpy(m_data + pos, data, size * sizeof(T));
	}
};
