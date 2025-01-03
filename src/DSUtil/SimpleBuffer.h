/*
 * (C) 2017-2025 see Authors.txt
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

#include <cstdlib>

#define SIMPLE_BLOCK_ALIGNMENT 16

// CSimpleBlock - simple container
template <typename T>
class CSimpleBlock
{
protected:
#if (__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= SIMPLE_BLOCK_ALIGNMENT)
	std::unique_ptr<T[]> m_data;
#else
	std::unique_ptr<T[], std::integral_constant<decltype(&_aligned_free), &_aligned_free>> m_data;
#endif
	size_t m_size = 0;

public:
	// Returns pointer to the data.
	auto* Data() { return m_data.get(); }

	// Returns the number of elements.
	auto Size() const { return m_size; }

	// Returns allocated size in bytes.
	size_t Bytes() const { return m_size * sizeof(T); }

	// Set new size. Old data will be lost.
	void SetSize(const size_t size)
	{
		if (size != m_size) {
#if (__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= SIMPLE_BLOCK_ALIGNMENT)
			m_data.reset(size ? new T[size] : nullptr);
#else
			m_data.reset(size ? static_cast<T*>(_aligned_malloc(sizeof(T) * size, SIMPLE_BLOCK_ALIGNMENT)) : nullptr);
#endif
			m_size = size;
		}
	}

	auto const& operator[](size_t i) const noexcept { return m_data[i]; }
	auto& operator[](size_t i) noexcept { return m_data[i]; }
};

// CSimpleBuffer - simple container for buffers.
// Use the ExtendSize method to have a buffer of sufficient size.
template <typename T>
class CSimpleBuffer : public CSimpleBlock<T>
{
	size_t CalcRoundedSize(size_t size) {
		return ((size * sizeof(T) + 255) & ~(size_t)255) / sizeof(T); // rounded up a multiple of 256 bytes.
	}

public:
	// Increase the size if necessary. Old data may be lost. The size will be rounded up to a multiple of 256 bytes.
	void ExtendSize(const size_t size)
	{
		if (size > this->m_size) {
			size_t newsize = CalcRoundedSize(size);
			this->SetSize(newsize);
		}
	}

	// Increase the size if necessary. Old data will be intact. The size will be rounded up to a multiple of 256 bytes.
	void ExtendSizeNoDiscard(const size_t size)
	{
		if (size > this->m_size) {
			size_t old_bytes = this->Bytes();
			auto old_data = std::move(this->m_data);
			this->m_size = 0;

			size_t newsize = CalcRoundedSize(size);
			this->SetSize(newsize);

			memcpy(this->m_data.get(), old_data.get(), old_bytes);
		}
	}
};
