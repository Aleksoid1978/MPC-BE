/*
 * (C) 2017-2023 see Authors.txt
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

template <typename T>
class CSimpleBlock
{
protected:
	std::unique_ptr<T[]> m_data;
	size_t m_size = 0;

public:
	// Returns pointer to the data.
	auto* Data() { return m_data.get(); }

	// Returns the number of elements.
	auto Size() { return m_size; }

	// Returns allocated size in bytes.
	size_t Bytes() { return m_size * sizeof(T); }

	// Set new size. Old data will be lost.
	void SetSize(const size_t size)
	{
		if (size != m_size) {
			m_data.reset(size ? new T[size] : nullptr);
			m_size = size;
		}
	}

	auto const& operator[](size_t i) const noexcept { return m_data[i]; }
	auto& operator[](size_t i) noexcept { return m_data[i]; }
};

// CSimpleBuffer - fast and simple container
// PS: When resizing the old data will be lost.

template <typename T>
class CSimpleBuffer : public CSimpleBlock<T>
{
public:
	// Increase the size if necessary. Old data may be lost. The size will be rounded up to a multiple of 256 bytes.
	void ExpandSize(const size_t size)
	{
		size_t newsize = ((size * sizeof(T) + 255) & ~(size_t)255) / sizeof(T); // rounded up a multiple of 256 bytes.

		if (newsize > m_size) {
			SetSize(newsize);
		}
	}

	// Write to the buffer from the specified position. The data before the specified position will be saved.
	void WriteData(size_t pos, const T* data, const size_t size)
	{
		size_t required_size = pos + size;

		if (required_size > m_size) {
			std::unique_ptr<T[]> new_data = std::make_unique<T[]>(required_size);
			memcpy(new_data.get(), m_data.get(), pos * sizeof(T));
			m_data = std::move(new_data);
			m_size = required_size;
		}

		memcpy(m_data.get() + pos, data, size * sizeof(T));
	}
};
