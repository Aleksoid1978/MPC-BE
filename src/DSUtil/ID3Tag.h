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

#include <map>
#include <vector>
#include "CUE.h"
#include "GolombBuffer.h"

//
// ID3TagItem class
//

enum ID3Type {
	ID3_TYPE_STRING,
	ID3_TYPE_BINARY
};

class CID3TagItem
{
public:

	CID3TagItem(const DWORD tag, const CString& value)
		: m_type(ID3_TYPE_STRING)
		, m_tag(tag)
		, m_value(value) {}

	CID3TagItem(const DWORD tag, const std::vector<BYTE>& data, const CString& mime)
		: m_type(ID3_TYPE_BINARY)
		, m_tag(tag)
		, m_Mime(mime)
		, m_Data(data) {}

	DWORD GetTag()        const { return m_tag; }
	CString GetValue()    const { return m_value; }
	CString GetMime()     const { return m_Mime; }
	const BYTE* GetData() const { return m_Data.data(); }
	size_t GetDataLen()   const { return m_Data.size(); }
	ID3Type GetType()     const { return m_type; }

protected:
	DWORD             m_tag;

	// text value
	CString           m_value;

	// binary value
	std::vector<BYTE> m_Data;
	CString           m_Mime;

	ID3Type           m_type;
};

//
// ID3Tag class
//

class CID3Tag
{
protected:
	BYTE m_major;
	BYTE m_flags;

	CString ReadText(CGolombBuffer& gb, DWORD size, const BYTE encoding);
	CString ReadField(CGolombBuffer& gb, DWORD &size, const BYTE encoding);

	void ReadTag(const DWORD tag, CGolombBuffer& gbData, DWORD &size, CID3TagItem** item);
	void ReadChapter(CGolombBuffer& gbData, DWORD &size);

public:
	std::map<DWORD, CString> Tags;
	std::list<CID3TagItem*>  TagItems;
	std::list<Chapters>      ChaptersList;

	CID3Tag(BYTE major = 0, BYTE flags = 0);
	virtual ~CID3Tag();

	void Clear();

	// tag reading
	BOOL ReadTagsV2(BYTE *buf, size_t len);
	BOOL ReadTagsV1(BYTE *buf, size_t len);
};

// additional functions
void SetID3TagProperties(IBaseFilter* pBF, const CID3Tag* pID3tag);

// ID3v2
static unsigned int hexdec2uint(unsigned int size)
{
	return (((size & 0x7F000000) >> 3) |
			((size & 0x007F0000) >> 2) |
			((size & 0x00007F00) >> 1) |
			((size & 0x0000007F)));
}

#define ID3v2_HEADER_SIZE 10
static int id3v2_match_len(const unsigned char *buf)
{
	if (buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3'
			&& buf[3] != 0xff && buf[4] != 0xff
			&& (buf[6] & 0x80) == 0
			&& (buf[7] & 0x80) == 0
			&& (buf[8] & 0x80) == 0
			&& (buf[9] & 0x80) == 0) {
		int len = ((buf[6] & 0x7f) << 21) +
			((buf[7] & 0x7f) << 14) +
			((buf[8] & 0x7f) << 7) +
			(buf[9] & 0x7f) + ID3v2_HEADER_SIZE;

		if (buf[5] & 0x10) {
			len += ID3v2_HEADER_SIZE;
		}
		return len;
	}
	return 0;
}
