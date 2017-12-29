/*
 * (C) 2014-2017 see Authors.txt
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

#include <vector>

struct NoInitByte
{
	uint8_t value;
	NoInitByte() {
		// do nothing
		static_assert(sizeof(*this) == sizeof (value), "invalid size");
		//static_assert(__alignof(*this) == __alignof(value), "invalid alignment");
	}
};

// A dynamic byte array with a guaranteed padded block at the end and no member initialization.
class CPaddedArray : private std::vector<NoInitByte>
{
private:
	size_t m_padsize;

public:
	CPaddedArray(size_t padsize)
		: m_padsize(padsize)
	{
	}

	uint8_t* GetData()
	{
		return &__super::front().value;
	}

	size_t GetCount()
	{
		const size_t count = __super::size();
		return (count > m_padsize) ? count - m_padsize : 0;
	}

	bool SetCount(size_t nNewSize)
	{
		try {
			__super::resize(nNewSize + m_padsize);
		}
		catch (...) {
			return false;
		}
		memset(GetData() + nNewSize, 0, m_padsize);
		return true;
	}

	void RemoveAll()
	{
		__super::clear();
	}

	bool Append(uint8_t* p, size_t nSize)
	{
		const size_t oldSize = GetCount();
		if (SetCount(oldSize + nSize)) {
			memcpy(GetData() + oldSize, p, nSize);
			return true;
		}
		return false;
	}

	void RemoveHead(size_t nSize)
	{
		const size_t count = GetCount();
		if (nSize >= count) {
			RemoveAll();
		} else {
			memmove(GetData(), GetData() + nSize, count - nSize);
			SetCount(count - nSize);
		}
	}
};
