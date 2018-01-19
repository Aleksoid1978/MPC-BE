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
			m_Data.resize(tag_size);
			gb.ReadBuffer(m_Data.data(), tag_size);
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
	for (auto& item : TagItems) {
		SAFE_DELETE(item);
	}
	TagItems.clear();

	m_TagSize = m_TagFields = 0;
}

bool CAPETag::ReadFooter(BYTE *buf, const size_t& len)
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

bool CAPETag::ReadTags(BYTE *buf, const size_t& len)
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

		TagItems.emplace_back(item);
	}

	return true;
}

// additional functions

void SetAPETagProperties(IBaseFilter* pBF, const CAPETag* pAPETag)
{
	if (!pAPETag || pAPETag->TagItems.empty()) {
		return;
	}

	CAtlArray<BYTE> CoverData;
	CString CoverMime, CoverFileName;

	CString Artist, Comment, Title, Year, Album;

	for (const auto& item : pAPETag->TagItems) {
		CString TagKey = item->GetKey().MakeLower();

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
				std::list<Chapters> ChaptersList;
				if (ParseCUESheet(TagValue, ChaptersList, sTitle, sPerformer)) {
					if (sTitle.GetLength() > 0 && Title.IsEmpty()) {
						Title = sTitle;
					}
					if (sPerformer.GetLength() > 0 && Artist.IsEmpty()) {
						Artist = sPerformer;
					}

					if (CComQIPtr<IDSMChapterBag> pCB = pBF) {
						pCB->ChapRemoveAll();
						for (const auto& cp : ChaptersList) {
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
