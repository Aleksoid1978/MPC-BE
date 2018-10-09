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
#include "ID3Tag.h"
#include "DSUtil.h"
#include "DSMPropertyBag.h"

//
// ID3Tag class
//
CID3Tag::CID3Tag(BYTE major/* = 0*/, BYTE flags/* = 0*/)
	: m_major(major)
	, m_flags(flags)
{
}

CID3Tag::~CID3Tag()
{
	Clear();
}

void CID3Tag::Clear()
{
	for (auto& item : TagItems) {
		SAFE_DELETE(item);
	}
	TagItems.clear();
}

// text encoding:
// 0 - ISO-8859-1
// 1 - UCS-2 (UTF-16 encoded Unicode with BOM)
// 2 - UTF-16BE encoded Unicode without BOM
// 3 - UTF-8 encoded Unicode

enum ID3v2Encoding {
	ISO8859  = 0,
	UTF16BOM = 1,
	UTF16BE  = 2,
	UTF8     = 3,
};

CString CID3Tag::ReadText(CGolombBuffer& gb, DWORD size, const BYTE encoding)
{
	CString  str;
	CStringA strA;

	BOOL bUTF16LE = FALSE;
	BOOL bUTF16BE = FALSE;
	switch (encoding) {
		case ID3v2Encoding::ISO8859:
		case ID3v2Encoding::UTF8:
			gb.ReadBuffer((BYTE*)strA.GetBufferSetLength(size), size);
			str = encoding == ID3v2Encoding::ISO8859 ? CString(strA) : UTF8ToWStr(strA);
			break;
		case ID3v2Encoding::UTF16BOM:
			if (size > 2) {
				WORD bom = (WORD)gb.BitRead(16);
				size -= 2;
				if (bom == 0xfffe) {
					bUTF16LE = TRUE;
				} else if (bom == 0xfeff) {
					bUTF16BE = TRUE;
				}
			}
			break;
		case ID3v2Encoding::UTF16BE:
			if (size >= 2) {
				bUTF16BE = TRUE;
			}
			break;
	}

	if (bUTF16LE || bUTF16BE) {
		size /= 2;
		gb.ReadBuffer((BYTE*)str.GetBufferSetLength(size), size * 2);
		if (bUTF16BE) {
			for (int i = 0, j = str.GetLength(); i < j; i++) {
				str.SetAt(i, (str[i] << 8) | (str[i] >> 8));
			}
		}
	}

	str.Remove(0x00);
	return str.Trim();
}

CString CID3Tag::ReadField(CGolombBuffer& gb, DWORD &size, const BYTE encoding)
{
	int pos = gb.GetPos();

	DWORD fieldSize = 0;
	const DWORD fieldseparatorSize = (encoding == ID3v2Encoding::ISO8859 || encoding == ID3v2Encoding::UTF8)? 1 : 2;
	while (size >= fieldseparatorSize) {
		size      -= fieldseparatorSize;
		fieldSize += fieldseparatorSize;
		if (gb.BitRead(8 * fieldseparatorSize) == 0) {
			break;
		}
	}

	CString str;
	if (fieldSize) {
		gb.Seek(pos);
		str = ReadText(gb, fieldSize, encoding);
	};

	while (size >= fieldseparatorSize
			&& gb.BitRead(8 * fieldseparatorSize, true) == 0) {
		size -= fieldseparatorSize;
		gb.SkipBytes(fieldseparatorSize);
	}

	if (size && size < fieldseparatorSize) {
		size = 0;
		gb.SkipBytes(size);
	}

	return str;
}

static void ReadLang(CGolombBuffer &gb, DWORD &size)
{
	// Language
	/*
	CHAR lang[3] = { 0 };
	gb.ReadBuffer((BYTE*)lang, 3);
	UNREFERENCED_PARAMETER(lang);
	*/
	gb.SkipBytes(3);
	size -= 3;
}

static LPCWSTR picture_types[] = {
	L"Other",
	L"32x32 pixels(file icon)",
	L"Other file icon",
	L"Cover (front)",
	L"Cover (back)",
	L"Leaflet page",
	L"Media (lable side of CD)",
	L"Lead artist(lead performer)",
	L"Artist(performer)",
	L"Conductor",
	L"Band(Orchestra)",
	L"Composer",
	L"Lyricist(text writer)",
	L"Recording Location",
	L"During recording",
	L"During performance",
	L"Movie(video) screen capture",
	L"A bright coloured fish",
	L"Illustration",
	L"Band(artist) logo",
	L"Publisher(Studio) logo"
};

void CID3Tag::ReadTag(const DWORD tag, CGolombBuffer& gbData, DWORD &size, CID3TagItem** item)
{
	BYTE encoding = (BYTE)gbData.BitRead(8);
	size--;

	if (tag == 'APIC' || tag == '\0PIC') {
		WCHAR mime[64];
		memset(&mime, 0 ,64);

		int mime_len = 0;
		while (size-- && (mime[mime_len++] = gbData.BitRead(8)) != 0);

		BYTE pict_type = (BYTE)gbData.BitRead(8);
		size--;
		CString pictStr(L"cover");
		if (pict_type < sizeof(picture_types)) {
			pictStr = picture_types[pict_type];
		}

		if (tag == 'APIC') {
			CString Desc = ReadField(gbData, size, encoding);
			UNREFERENCED_PARAMETER(Desc);
		}

		CString mimeStr(mime);
		if (tag == '\0PIC') {
			if (CString(mimeStr).MakeUpper() == L"JPG") {
				mimeStr = L"image/jpeg";
			} else if (CString(mimeStr).MakeUpper() == L"PNG") {
				mimeStr = L"image/png";
			} else {
				mimeStr.Format(L"image/%s", CString(mime));
			}
		}

		std::vector<BYTE> data;
		data.resize(size);
		gbData.ReadBuffer(data.data(), size);

		*item = DNew CID3TagItem(tag, data, mimeStr, pictStr);
	} else {
		if (tag == 'COMM' || tag == '\0ULT' || tag == 'USLT') {
			ReadLang(gbData, size);
			CString Desc = ReadField(gbData, size, encoding);
			UNREFERENCED_PARAMETER(Desc);
		}

		CString text;
		if (((char*)&tag)[3] == 'T') {
			while (size) {
				CString field = ReadField(gbData, size, encoding);
				if (!field.IsEmpty()) {
					if (text.IsEmpty()) {
						text = field;
					} else {
						text.AppendFormat(L"; %s", field);
					}
				}
			}
		} else {
			text = ReadText(gbData, size, encoding);
		}

		if (!text.IsEmpty()) {
			*item = DNew CID3TagItem(tag, text);
		}
	}
}

void CID3Tag::ReadChapter(CGolombBuffer& gbData, DWORD &size)
{
	CString chapterName;
	CString element = ReadField(gbData, size, ID3v2Encoding::ISO8859);

	DWORD start = gbData.ReadDword();
	DWORD end   = gbData.ReadDword();
	gbData.SkipBytes(8);

	size -= 16;

	int pos = gbData.GetPos();
	while (size > 10) {
		gbData.Seek(pos);

		DWORD tag   = (DWORD)gbData.BitRead(32);
		DWORD len   = (DWORD)gbData.BitRead(32);
		DWORD flags = (WORD)gbData.BitRead(16); UNREFERENCED_PARAMETER(flags);
		size -= 10;

		pos += gbData.GetPos() + len;

		if (len > size || tag == 0) {
			break;
		}

		if (!len) {
			continue;
		}

		size -= len;

		if (((char*)&tag)[3] == 'T') {
			CID3TagItem* item = nullptr;
			ReadTag(tag, gbData, len, &item);
			if (item && item->GetType() == ID3Type::ID3_TYPE_STRING) {
				if (chapterName.IsEmpty()) {
					chapterName = item->GetValue();
				} else {
					chapterName += L" / " + item->GetValue();
				}
				delete item;
			}
		}
	}

	if (chapterName.IsEmpty()) {
		chapterName = element;
	}

	ChaptersList.emplace_back(Chapters{chapterName, MILLISECONDS_TO_100NS_UNITS(start)});
}

#define ID3v2_FLAG_DATALEN 0x0001
#define ID3v2_FLAG_UNSYNCH 0x0002

BOOL CID3Tag::ReadTagsV2(BYTE *buf, size_t len)
{
	CGolombBuffer gb(buf, len);

	DWORD tag;
	DWORD size;
	WORD flags;
	int tagsize;
	if (m_major == 2) {
		if (m_flags & 0x40) {
			return FALSE;
		}
		flags   = 0;
		tagsize = 6;
	} else {
		if (m_flags & 0x40) {
			// Extended header present, skip it
			DWORD extlen = gb.BitRead(32);
			extlen = hexdec2uint(extlen);
			if (m_major == 4) {
				if (extlen < 4) {
					return FALSE;
				}
				extlen -= 4;
			}
			if (extlen + 4 > len) {
				return FALSE;
			}

			gb.SkipBytes(extlen);
			len -= extlen + 4;
		}
		tagsize = 10;
	}
	int pos = gb.GetPos();

	while (!gb.IsEOF()) {
		if (m_major == 2) {
			tag   = (DWORD)gb.BitRead(24);
			size  = (DWORD)gb.BitRead(24);
		} else {
			tag   = (DWORD)gb.BitRead(32);
			size  = (DWORD)gb.BitRead(32);
			flags = (WORD)gb.BitRead(16);
			if (m_major == 4) {
				size = hexdec2uint(size);
			}
		}

		if (pos + tagsize + size > len) {
			break;
		}

		pos += tagsize + size;

		if (pos > gb.GetSize() || tag == 0) {
			break;
		}

		if (pos < gb.GetSize()) {
			const int save_pos = gb.GetPos();

			gb.Seek(pos);
			while (!gb.IsEOF() && !gb.BitRead(8, true)) {
				gb.SkipBytes(1);
				pos++;
				size++;
			}

			gb.Seek(save_pos);
		}

		if (!size) {
			gb.Seek(pos);
			continue;
		}

		if (flags & ID3v2_FLAG_DATALEN) {
			if (size < 4) {
				break;
			}
			gb.SkipBytes(4);
			size -= 4;
		}

		std::vector<BYTE> Data;
		BOOL bUnSync = m_flags & 0x80 || flags & ID3v2_FLAG_UNSYNCH;
		if (bUnSync) {
			DWORD dwSize = size;
			while (dwSize) {
				BYTE b = gb.ReadByte();
				Data.push_back(b);
				dwSize--;
				if (b == 0xFF && dwSize > 1) {
					b = gb.ReadByte();
					dwSize--;
					if (!b) {
						b = gb.ReadByte();
						dwSize--;
					}
					Data.push_back(b);
				}
			}
		} else {
			Data.resize(size);
			gb.ReadBuffer(Data.data(), size);
		}
		CGolombBuffer gbData(Data.data(), Data.size());
		size = Data.size();

		if (tag == 'TIT2'
				|| tag == 'TPE1'
				|| tag == 'TALB' || tag == '\0TAL'
				|| tag == 'TYER'
				|| tag == 'COMM'
				|| tag == 'TRCK'
				|| tag == 'TCOP'
				|| tag == '\0TP1'
				|| tag == '\0TT2'
				|| tag == '\0PIC' || tag == 'APIC'
				|| tag == '\0ULT' || tag == 'USLT') {
			CID3TagItem* item = nullptr;
			ReadTag(tag, gbData, size, &item);

			if (item) {
				TagItems.emplace_back(item);
			}
		} else if (tag == 'CHAP') {
			ReadChapter(gbData, size);
		}

		gb.Seek(pos);
	}

	for (const auto& item : TagItems) {
		if (item->GetType() == ID3Type::ID3_TYPE_STRING && item->GetTag()) {
			Tags[item->GetTag()] = item->GetValue();
		}
	}

	return !TagItems.empty() ? TRUE : FALSE;
}

static const LPCSTR s_genre[] = {
	"Blues", "Classic Rock", "Country", "Dance",
	"Disco", "Funk", "Grunge", "Hip-Hop",
	"Jazz", "Metal", "New Age", "Oldies",
	"Other", "Pop", "R&B", "Rap",
	"Reggae", "Rock", "Techno", "Industrial",
	"Alternative", "Ska", "Death Metal", "Pranks",
	"Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
	"Vocal", "Jazz+Funk", "Fusion", "Trance",
	"Classical", "Instrumental", "Acid", "House",
	"Game", "Sound Clip", "Gospel", "Noise",
	"Alternative Rock", "Bass", "Soul", "Punk",
	"Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
	"Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
	"Electronic", "Pop-Folk", "Eurodance", "Dream",
	"Southern Rock", "Comedy", "Cult", "Gangsta",
	"Top 40", "Christian Rap", "Pop/Funk", "Jungle",
	"Native US", "Cabaret", "New Wave", "Psychadelic",
	"Rave", "Showtunes", "Trailer", "Lo-Fi",
	"Tribal", "Acid Punk", "Acid Jazz", "Polka",
	"Retro", "Musical", "Rock & Roll", "Hard Rock",
	"Folk", "Folk-Rock", "National Folk", "Swing",
	"Fast Fusion", "Bebob", "Latin", "Revival",
	"Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
	"Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
	"Big Band", "Chorus", "Easy Listening", "Acoustic",
	"Humour", "Speech", "Chanson", "Opera",
	"Chamber Music", "Sonata", "Symphony", "Booty Bass",
	"Primus", "Porn Groove", "Satire", "Slow Jam",
	"Club", "Tango", "Samba", "Folklore",
	"Ballad", "Power Ballad", "Rhytmic Soul", "Freestyle",
	"Duet", "Punk Rock", "Drum Solo", "Acapella",
	"Euro-House", "Dance Hall", "Goa", "Drum & Bass",
	"Club-House", "Hardcore", "Terror", "Indie",
	"BritPop", "Negerpunk", "Polsk Punk", "Beat",
	"Christian Gangsta", "Heavy Metal", "Black Metal",
	"Crossover", "Contemporary C", "Christian Rock", "Merengue", "Salsa",
	"Thrash Metal", "Anime", "JPop", "SynthPop",
};

BOOL CID3Tag::ReadTagsV1(BYTE *buf, size_t len)
{
	CGolombBuffer gb(buf, len);

	CStringA str;

	CString value;
	DWORD tag;

	// title
	gb.ReadBuffer((BYTE*)str.GetBufferSetLength(30), 30);
	tag = 'TIT2';
	if (Tags.find(tag) == Tags.cend()) {
		CID3TagItem* item = DNew CID3TagItem(tag, CString(str).Trim());
		TagItems.emplace_back(item);

		Tags[tag] = CString(str).Trim();
	}

	// artist
	gb.ReadBuffer((BYTE*)str.GetBufferSetLength(30), 30);
	tag = 'TPE1';
	if (Tags.find(tag) == Tags.cend()) {
		value = CString(str).Trim();
		CID3TagItem* item = DNew CID3TagItem(tag, value);
		TagItems.emplace_back(item);

		Tags[tag] = value;
	}

	// album
	gb.ReadBuffer((BYTE*)str.GetBufferSetLength(30), 30);
	tag = 'TALB';
	if (Tags.find(tag) == Tags.cend()) {
		value = CString(str).Trim();
		CID3TagItem* item = DNew CID3TagItem(tag, value);
		TagItems.emplace_back(item);

		Tags[tag] = value;
	}

	// year
	gb.ReadBuffer((BYTE*)str.GetBufferSetLength(4), 4);
	tag = 'TYER';
	if (Tags.find(tag) == Tags.cend()) {
		value = CString(str).Trim();
		CID3TagItem* item = DNew CID3TagItem(tag, value);
		TagItems.emplace_back(item);

		Tags[tag] = value;
	}

	// comment
	gb.ReadBuffer((BYTE*)str.GetBufferSetLength(30), 30);
	tag = 'COMM';
	if (Tags.find(tag) == Tags.cend()) {
		value = CString(str).Trim();
		CID3TagItem* item = DNew CID3TagItem(tag, value);
		TagItems.emplace_back(item);

		Tags[tag] = value;
	}

	// track
	LPCSTR s = str;
	if (s[28] == 0 && s[29] != 0) {
		tag = 'TRCK';
		if (Tags.find(tag) == Tags.cend()) {
			value.Format(L"%d", s[29]);
			CID3TagItem* item = DNew CID3TagItem(tag, value);
			TagItems.emplace_back(item);

			Tags[tag] = value;
		}
	}

	// genre
	BYTE genre = (BYTE)gb.BitRead(8);
	if (genre < _countof(s_genre)) {
		tag = 'TCON';
		if (Tags.find(tag) == Tags.cend()) {
			value = s_genre[genre];
			CID3TagItem* item = DNew CID3TagItem(tag, value);
			TagItems.emplace_back(item);

			Tags[tag] = value;
		}
	}

	return TRUE;
}

// additional functions
void SetID3TagProperties(IBaseFilter* pBF, const CID3Tag* pID3tag)
{
	if (!pID3tag || pID3tag->Tags.empty()) {
		return;
	}

	const auto Lookup = [&](const DWORD& tag, CString& str) {
		if (const auto it = pID3tag->Tags.find(tag); it != pID3tag->Tags.cend()) {
			str = it->second;
			return true;
		}
		return false;
	};

	if (CComQIPtr<IDSMPropertyBag> pPB = pBF) {
		CString str, title;
		if (Lookup('TIT2', str) || Lookup('\0TT2', str)) {
			title = str;
		}
		if (Lookup('TYER', str) && !title.IsEmpty() && !str.IsEmpty()) {
			title += L" (" + str + L")";
		}
		if (!title.IsEmpty()) {
			pPB->SetProperty(L"TITL", title);
		}
		if (Lookup('TPE1', str) || Lookup('\0TP1', str)) {
			pPB->SetProperty(L"AUTH", str);
		}
		if (Lookup('TCOP', str)) {
			pPB->SetProperty(L"CPYR", str);
		}
		if (Lookup('COMM', str)) {
			pPB->SetProperty(L"DESC", str);
		}
		if (Lookup('USLT', str) || Lookup('\0ULT', str)) {
			pPB->SetProperty(L"LYRICS", str);
		}
		if (Lookup('TALB', str) || Lookup('\0TAL', str)) {
			pPB->SetProperty(L"ALBUM", str);
		}
	}

	if (CComQIPtr<IDSMResourceBag> pRB = pBF) {
		DWORD_PTR tag = 0;
		for (const auto& item : pID3tag->TagItems) {
			if (item->GetType() == ID3Type::ID3_TYPE_BINARY && item->GetDataLen()) {
				pRB->ResAppend(item->GetValue(), item->GetValue(), item->GetMime(), (BYTE*)item->GetData(), (DWORD)item->GetDataLen(), tag++);
			}
		}
	}

	if (CComQIPtr<IDSMChapterBag> pCB = pBF) {
		if (!pID3tag->ChaptersList.empty()) {
			pCB->ChapRemoveAll();
			for (const auto& cp : pID3tag->ChaptersList) {
				pCB->ChapAppend(cp.rt, cp.name);
			}
		}
	}
}
