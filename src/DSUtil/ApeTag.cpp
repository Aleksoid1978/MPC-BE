/*
 * (C) 2012-2017 see Authors.txt
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

#include "stdafx.h"
#include "ApeTag.h"
#include "DSUtil.h"
#include "CUE.h"
#include "DSMPropertyBag.h"

//
// ApeTagItem class
//

CApeTagItem::CApeTagItem()
	: m_type(APE_TYPE_STRING)
{
}

bool CApeTagItem::Load(CGolombBuffer &gb){
	if ((gb.GetSize() - gb.GetPos()) < 8) {
		return false;
	}

	DWORD tag_size    = gb.ReadDwordLE(); /* field size */
	const DWORD flags = gb.ReadDwordLE(); /* field flags */

	CStringA key;
	BYTE b = gb.ReadByte();
	while (b) {
		key += b;
		b = gb.ReadByte();
	}

	if (flags & APE_TAG_FLAG_IS_BINARY) {
		m_type = APE_TYPE_BINARY;

		key = "";
		b = gb.ReadByte();
		tag_size--;
		while (b && tag_size) {
			key += b;
			b = gb.ReadByte();
			tag_size--;
		}

		if (tag_size) {
			m_key = key;
			m_Data.SetCount(tag_size);
			gb.ReadBuffer(m_Data.GetData(), tag_size);
		}
	} else {
		BYTE* value = DNew BYTE[tag_size + 1];
		memset(value, 0, tag_size + 1);
		gb.ReadBuffer(value, tag_size);
		m_value = UTF8To16((LPCSTR)value);
		m_key   = key;
		delete [] value;
	}

	return true;
}

//
// ApeTag class
//

CAPETag::CAPETag()
	: m_TagSize(0)
	, m_TagFields(0)
{
}

CAPETag::~CAPETag()
{
	Clear();
}

void CAPETag::Clear()
{
	CApeTagItem* item;
	while (!TagItems.IsEmpty()) {
		item = TagItems.RemoveHead();
		if (item) {
			delete item;
		}
	}

	m_TagSize = m_TagFields = 0;
}

bool CAPETag::ReadFooter(BYTE *buf, size_t len)
{
	m_TagSize = m_TagFields = 0;

	if (len < APE_TAG_FOOTER_BYTES) {
		return false;
	}

	if (memcmp(buf, "APETAGEX", 8) || memcmp(buf + 24, "\0\0\0\0\0\0\0\0", 8)) {
		return false;
	}

	CGolombBuffer gb((BYTE*)buf + 8, APE_TAG_FOOTER_BYTES - 8);
	const DWORD ver = gb.ReadDwordLE();
	if (ver != APE_TAG_VERSION) {
		return false;
	}

	const DWORD tag_size = gb.ReadDwordLE();
	const DWORD fields   = gb.ReadDwordLE();
	const DWORD flags    = gb.ReadDwordLE();

	if ((fields > 65536) || (flags & APE_TAG_FLAG_IS_HEADER)) {
		return false;
	}

	m_TagSize   = tag_size;
	m_TagFields = fields;

	return true;
}

bool CAPETag::ReadTags(BYTE *buf, size_t len)
{
	if (!m_TagSize || m_TagSize != len) {
		return false;
	}

	CGolombBuffer gb(buf, len);
	for (size_t i = 0; i < m_TagFields; i++) {
		CApeTagItem *item = DNew CApeTagItem();
		if (!item->Load(gb)) {
			delete item;
			Clear();
			return false;
		}

		TagItems.AddTail(item);
	}

	return true;
}

CApeTagItem* CAPETag::Find(CString key)
{
	CString key_lc = key;
	key_lc.MakeLower();

	POSITION pos = TagItems.GetHeadPosition();
	while (pos) {
		CApeTagItem* item = TagItems.GetAt(pos);
		CString TagKey    = item->GetKey();
		TagKey.MakeLower();
		if (TagKey == key_lc) {
			return item;
		}

		TagItems.GetNext(pos);
	}

	return nullptr;
}

// additional functions

void SetAPETagProperties(IBaseFilter* pBF, const CAPETag* apetag)
{
	if (!apetag || apetag->TagItems.IsEmpty()) {
		return;
	}

	CAtlArray<BYTE> CoverData;
	CString CoverMime, CoverFileName;

	CString Artist, Comment, Title, Year, Album;

	POSITION pos = apetag->TagItems.GetHeadPosition();
	while (pos) {
		CApeTagItem* item = apetag->TagItems.GetAt(pos);
		CString TagKey    = item->GetKey();
		TagKey.MakeLower();

		if (item->GetType() == CApeTagItem::APE_TYPE_BINARY) {
			CoverMime.Empty();
			if (!TagKey.IsEmpty()) {
				CString ext = TagKey.Mid(TagKey.ReverseFind('.') + 1);
				if (ext == L"jpeg" || ext == L"jpg") {
					CoverMime = L"image/jpeg";
				} else if (ext == L"png") {
					CoverMime = L"image/png";
				}
			}

			if (!CoverData.GetCount() && CoverMime.GetLength() > 0) {
				CoverFileName = TagKey;
				CoverData.SetCount(item->GetDataLen());
				memcpy(CoverData.GetData(), item->GetData(), item->GetDataLen());
			}
		} else {
			CString sTitle, sPerformer;

			CString TagValue = item->GetValue();
			if (TagKey == L"cuesheet") {
				CAtlList<Chapters> ChaptersList;
				if (ParseCUESheet(TagValue, ChaptersList, sTitle, sPerformer)) {
					if (sTitle.GetLength() > 0 && Title.IsEmpty()) {
						Title = sTitle;
					}
					if (sPerformer.GetLength() > 0 && Artist.IsEmpty()) {
						Artist = sPerformer;
					}

					if (CComQIPtr<IDSMChapterBag> pCB = pBF) {
						pCB->ChapRemoveAll();
						while (ChaptersList.GetCount()) {
							Chapters cp = ChaptersList.RemoveHead();
							pCB->ChapAppend(cp.rt, cp.name);
						}
					}
				}
			}

			if (TagValue.GetLength() > 0) {
				if (TagKey == L"artist") {
					Artist = TagValue;
				} else if (TagKey == L"comment") {
					Comment = TagValue;
				} else if (TagKey == L"title") {
					Title = TagValue;
				} else if (TagKey == L"year") {
					Year = TagValue;
				} else if (TagKey == L"album") {
					Album = TagValue;
				}
			}
		}

		apetag->TagItems.GetNext(pos);
	}

	if (CComQIPtr<IDSMPropertyBag> pPB = pBF) {
		pPB->SetProperty(L"AUTH", Artist);
		pPB->SetProperty(L"DESC", Comment);
		pPB->SetProperty(L"TITL", Title);
		pPB->SetProperty(L"YEAR", Year);
		pPB->SetProperty(L"ALBUM", Album);
	}

	if (CComQIPtr<IDSMResourceBag> pRB = pBF) {
		if (CoverData.GetCount()) {
			pRB->ResAppend(CoverFileName, L"cover", CoverMime, CoverData.GetData(), (DWORD)CoverData.GetCount(), 0);
		}
	}
}


// ID3v2
// TODO: remove it from here

int id3v2_match_len(const unsigned char *buf)
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
