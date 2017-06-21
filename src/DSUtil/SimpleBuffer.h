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

// Fast and simple container

template <typename T>
class CSimpleBuffer
{
	T* m_data = NULL;
	size_t m_size = 0;

public:
	CSimpleBuffer() {};
	~CSimpleBuffer()
	{
		if (m_data) {
			delete[] m_data;
		}
	}

	// returns pointer to the data
	T* Data() { return m_data; }
	
	// returns the number of elements
	size_t Size() { return m_size; }
	
	// returns allocated size in bytes
	size_t Bytes() { return m_size * sizeof(T); }

	// set new size. old data will be lost
	void SetSize(const size_t size)
	{
		if (m_data) {
			delete[] m_data;
		}

		if (size) {
			m_data = DNew T[size];
		} else {
			m_data = NULL;
		}

		m_size = size;
	}

	// increase the size if necessary. old data may be lost
	void ExpandSize(const size_t size)
	{
		if (size > m_size) {
			SetSize(size);
		}
	}
};
