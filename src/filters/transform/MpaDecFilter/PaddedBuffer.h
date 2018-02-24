/*
 * (C) 2014-2018 see Authors.txt
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

#include <basestruct.h>
#include <vector>

// A dynamic buffer with a guaranteed padded block at the end and no member initialization.
class CPaddedBuffer
{
private:
	std::vector<NoInitByte> m_data;
	size_t m_padsize;

public:
	CPaddedBuffer(size_t padsize)
		: m_padsize(padsize)
	{
	}

	uint8_t* Data()
	{
		return (uint8_t*)m_data.data(); // don't use "&front().value" here, because it does not work for an empty array
	}

	size_t Size()
	{
		const size_t count = m_data.size();
		return (count > m_padsize) ? count - m_padsize : 0;
	}

	bool Resize(const size_t count)
	{
		try {
			m_data.resize(count + m_padsize);
		}
		catch (...) {
			return false;
		}
		memset(Data() + count, 0, m_padsize);
		return true;
	}

	void Clear()
	{
		m_data.clear();
	}

	bool Append(uint8_t* p, const size_t count)
	{
		const size_t oldsize = Size();
		const size_t newsize = oldsize + count;
		if (Resize(newsize)) {
			memcpy(Data() + oldsize, p, count);
			memset(Data() + newsize, 0, m_padsize);
			return true;
		}
		return false;
	}

	void RemoveHead(const size_t count)
	{
		const size_t oldsize = Size();
		if (count >= oldsize) {
			Clear();
		} else {
			const size_t newsize = oldsize - count;
			memmove(Data(), Data() + count, newsize);
			memset(Data() + newsize, 0, m_padsize);
			m_data.resize(newsize + m_padsize);
		}
	}
};
