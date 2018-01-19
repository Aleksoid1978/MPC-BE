/*
 * (C) 2012-2018 see Authors.txt
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
#include <vector>
#include "GolombBuffer.h"

#define APE_TAG_FOOTER_BYTES   32
#define APE_TAG_VERSION        2000

#define APE_TAG_FLAG_IS_HEADER (1 << 29)
#define APE_TAG_FLAG_IS_BINARY (1 << 1)

//
// ApeTagItem class
//

class CApeTagItem
{
public:
	enum ApeType {
		APE_TYPE_STRING,
		APE_TYPE_BINARY
	};

	CApeTagItem();

	// load
	bool Load(CGolombBuffer &gb);

	CString GetKey()      const { return m_key; }
	CString GetValue()    const { return m_value; }
	const BYTE* GetData() const { return m_Data.data(); }
	size_t GetDataLen()   const { return m_Data.size(); }
	ApeType GetType()     const { return m_type; }

protected:
	CString           m_key;

	// text value
	CString           m_value;

	// binary value
	std::vector<BYTE> m_Data;

	ApeType           m_type;
};

//
// ApeTag class
//

class CAPETag
{
protected:
	size_t m_TagSize;
	size_t m_TagFields;

public:
	std::list<CApeTagItem*> TagItems;

	CAPETag();
	virtual ~CAPETag();

	void Clear();

	// tag reading
	bool ReadFooter(BYTE *buf, const size_t& len);
	bool ReadTags(BYTE *buf, const size_t& len);

	size_t GetTagSize() const { return m_TagSize; }
	//const CApeTagItem* Find(const CString& key);
};

// additional functions
void SetAPETagProperties(IBaseFilter* pBF, const CAPETag* pAPETag);

