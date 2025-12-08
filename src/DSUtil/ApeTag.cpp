/*
 * (C) 2012-2025 see Authors.txt
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
#include "FileHandle.h"
#include "std_helper.h"

bool CAPETag::LoadItems(CGolombBuffer &gb) {
	if ((gb.GetSize() - gb.GetPos()) < 8) {
		return false;
	}

	DWORD tag_size = gb.ReadDwordLE();
	const DWORD flags = gb.ReadDwordLE();

	CStringA key;
	BYTE b = gb.ReadByte();
	while (b) {
		key += b;
		b = gb.ReadByte();
	}

	if (flags & APE_TAG_FLAG_IS_BINARY) {
		const CString desc(key);
		key.Empty();
		b = gb.ReadByte();
		tag_size--;
		while (b && tag_size) {
			key += b;
			b = gb.ReadByte();
			tag_size--;
		}

		if (tag_size) {
			binary data(tag_size);
			gb.ReadBuffer(data.data(), tag_size);
			TagItems.emplace_back(CString(key), desc, std::move(data));
		}
	} else {
		auto data = std::make_unique<BYTE[]>(tag_size + 1);
		if (!data) {
			return false;
		}
		gb.ReadBuffer(data.get(), tag_size);
		TagItems.emplace_back(CString(key), L"", UTF8ToWStr((LPCSTR)data.get()));
	}

	return true;
}

void CAPETag::Clear()
{
	TagItems.clear();
	m_TagSize = m_TagFields = 0;
}

bool CAPETag::ReadFooter(BYTE *buf, const size_t len)
{
	m_TagSize = m_TagFields = 0;

	if (len < APE_TAG_FOOTER_BYTES) {
		return false;
	}

	if (memcmp(buf, "APETAGEX", 8) || memcmp(buf + 24, "\0\0\0\0\0\0\0\0", 8)) {
		return false;
	}

	auto start = buf + 8;

	CGolombBuffer gb(start, APE_TAG_FOOTER_BYTES - 8);
	const DWORD ver = gb.ReadDwordLE();
	if (ver != APE_TAG_VERSION) {
		return false;
	}

	const DWORD tag_size = gb.ReadDwordLE();
	const DWORD fields   = gb.ReadDwordLE();
	const DWORD flags    = gb.ReadDwordLE();

	if (fields > 65536) {
		return false;
	}

	if (flags & APE_TAG_FLAG_IS_HEADER) {
		return false;
	}

	m_TagSize   = tag_size;
	m_TagFields = fields;

	return true;
}

bool CAPETag::ReadTags(BYTE *buf, const size_t len)
{
	if (!m_TagSize || m_TagSize != len) {
		return false;
	}

	CGolombBuffer gb(buf, len);
	for (size_t i = 0; i < m_TagFields; i++) {
		if (!LoadItems(gb)) {
			Clear();
			return false;
		}
	}

	return true;
}

// additional functions

void SetAPETagProperties(IBaseFilter* pBF, const CAPETag* pAPETag)
{
	if (!pAPETag || pAPETag->TagItems.empty()) {
		return;
	}

	CString Artist, Comment, Title, Year, Album;
	for (const auto& [key, desc, value] : pAPETag->TagItems) {
		CString tagKey(key); tagKey.MakeLower();

		std::visit(overloaded{
			[&](const CAPETag::binary& tagValue) {
				if (!tagValue.empty()) {
					if (CComQIPtr<IDSMResourceBag> pRB = pBF) {
						CString CoverMime;
						if (!tagKey.IsEmpty()) {
							const auto ext = GetFileExt(tagKey);
							if (ext == L".jpeg" || ext == L".jpg") {
								CoverMime = L"image/jpeg";
							} else if (ext == L".png") {
								CoverMime = L"image/png";
							}
						}

						if (!CoverMime.IsEmpty()) {
							pRB->ResAppend(key, !desc.IsEmpty() ? desc : L"cover", CoverMime, (BYTE*)tagValue.data(), (DWORD)tagValue.size(), 0);
						}
					}
				}
			},
			[&](const CStringW& tagValue) {
				if (!tagValue.IsEmpty()) {
					if (tagKey == L"cuesheet") {
						std::list<Chapters> ChaptersList;
						CString sTitle, sPerformer;
						if (ParseCUESheet(tagValue, ChaptersList, sTitle, sPerformer)) {
							if (!sTitle.IsEmpty() && Title.IsEmpty()) {
								Title = sTitle;
							}
							if (!sPerformer.IsEmpty() && Artist.IsEmpty()) {
								Artist = sPerformer;
							}

							if (CComQIPtr<IDSMChapterBag> pCB = pBF) {
								pCB->ChapRemoveAll();
								for (const auto& cp : ChaptersList) {
									pCB->ChapAppend(cp.rt, cp.name);
								}
							}
						}
					} else if (tagKey == L"artist") {
						Artist = tagValue;
					} else if (tagKey == L"comment") {
						Comment = tagValue;
					} else if (tagKey == L"title") {
						Title = tagValue;
					} else if (tagKey == L"year") {
						Year = tagValue;
					} else if (tagKey == L"album") {
						Album = tagValue;
					}
				}
			}
		}, value);
	}

	if (CComQIPtr<IDSMPropertyBag> pPB = pBF) {
		if (!Artist.IsEmpty())  pPB->SetProperty(L"AUTH", Artist);
		if (!Comment.IsEmpty()) pPB->SetProperty(L"DESC", Comment);
		if (!Title.IsEmpty())   pPB->SetProperty(L"TITL", Title);
		if (!Year.IsEmpty())    pPB->SetProperty(L"YEAR", Year);
		if (!Album.IsEmpty())   pPB->SetProperty(L"ALBUM", Album);
	}
}

bool CAPEChapters::ReadFooter(BYTE *buf, const size_t len)
{
	m_TagSize = m_TagFields = 0;

	if (len < APE_TAG_FOOTER_BYTES - 8) {
		return false;
	}

	if (memcmp(buf + 16, "\0\0\0\0\0\0\0\0", 8)) {
		return false;
	}

	auto start = buf;

	CGolombBuffer gb(start, APE_TAG_FOOTER_BYTES - 8);
	const DWORD ver = gb.ReadDwordLE();
	if (ver != APE_TAG_VERSION) {
		return false;
	}

	const DWORD tag_size = gb.ReadDwordLE();
	const DWORD fields   = gb.ReadDwordLE();
	const DWORD flags    = gb.ReadDwordLE();

	if (fields > 65536) {
		return false;
	}

	m_TagSize   = tag_size;
	m_TagFields = fields;

	return true;
}
