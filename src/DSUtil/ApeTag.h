/*
 * (C) 2012-2019 see Authors.txt
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

#include <list>
#include <variant>
#include <vector>
#include "GolombBuffer.h"

#define APE_TAG_FOOTER_BYTES   32
#define APE_TAG_VERSION        2000

#define APE_TAG_FLAG_IS_HEADER (1 << 29)
#define APE_TAG_FLAG_IS_BINARY (1 << 1)

class CAPETag
{
public:
	enum ApeType {
		APE_TYPE_STRING,
		APE_TYPE_BINARY
	};

protected:
	size_t m_TagSize = 0;
	size_t m_TagFields = 0;

	using binary = std::vector<BYTE>;
	using values = std::variant<CString, binary>;
	using tagItem = std::tuple<ApeType, CString, values>;

	bool LoadItems(CGolombBuffer &gb);
	void Clear();

public:
	CAPETag() = default;
	~CAPETag() = default;

	bool ReadFooter(BYTE *buf, const size_t len);
	bool ReadTags(BYTE *buf, const size_t len);

	size_t GetTagSize() const { return m_TagSize; }

	std::list<tagItem> TagItems;
};

// additional functions
void SetAPETagProperties(IBaseFilter* pBF, const CAPETag* pAPETag);

