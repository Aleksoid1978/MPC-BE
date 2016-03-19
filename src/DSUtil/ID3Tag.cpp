/*
 * (C) 2012-2015 see Authors.txt
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
// ID3TagItem class
//

CID3TagItem::CID3TagItem(DWORD tag, CString value)
	: m_type(ID3_TYPE_STRING)
	, m_tag(tag)
	, m_value(value)
{
}

CID3TagItem::CID3TagItem(DWORD tag, CAtlArray<BYTE>& data, CString mime)
	: m_type(ID3_TYPE_BINARY)
	, m_tag(tag)
	, m_Mime(mime)
{
	m_Data.SetCount(data.GetCount());
	memcpy(m_Data.GetData(), data.GetData(), data.GetCount());
}


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
	CID3TagItem* item;
	while (!TagItems.IsEmpty()) {
		item = TagItems.RemoveHead();
		SAFE_DELETE(item);
	}
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

CString CID3Tag::ReadText(CGolombBuffer& gb, DWORD size, BYTE encoding)
{
	CString  str;
	CStringA strA;

	BOOL bUTF16LE = FALSE;
	BOOL bUTF16BE = FALSE;
	switch (encoding) {
		case ID3v2Encoding::ISO8859:
		case ID3v2Encoding::UTF8:
			gb.ReadBuffer((BYTE*)strA.GetBufferSetLength(size), size);
			str = encoding == ID3v2Encoding::ISO8859 ? CString(strA) : UTF8To16(strA);
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

CString CID3Tag::ReadField(CGolombBuffer& gb, DWORD &size, BYTE encoding)
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

#define ID3v2_FLAG_DATALEN 0x0001
#define ID3v2_FLAG_UNSYNCH 0x0002

BOOL CID3Tag::ReadTagsV2(BYTE *buf, size_t len)
{
	CGolombBuffer gb(buf, len);
	int pos = gb.GetPos();
	while (!gb.IsEOF()) {
		gb.Seek(pos);

		DWORD tag;
		DWORD size;
		WORD flags;
		int tagsize;
		if (m_major == 2) {
			tag		= (DWORD)gb.BitRead(24);
			size	= (DWORD)gb.BitRead(24);
			flags	= 0;
			tagsize = 6;
		} else {
			tag		= (DWORD)gb.BitRead(32);
			size	= (DWORD)gb.BitRead(32);
			flags	= (WORD)gb.BitRead(16);
			tagsize = 10;
		}

		if (pos + tagsize + size > len) {
			break;
		}

		pos += tagsize + size;

		if (pos > gb.GetSize() || tag == 0) {
			break;
		}

		if (!size) {
			continue;
		}

		if (flags & ID3v2_FLAG_DATALEN) {
			if (size < 4) {
				break;
			}
			gb.SkipBytes(4);
			size -= 4;
		}

		CAtlArray<BYTE> Data;
		BOOL bUnSync = m_flags & 0x80 || flags & ID3v2_FLAG_UNSYNCH;
		if (bUnSync) {
			DWORD dwSize = size;
			while (dwSize) {
				BYTE b = gb.ReadByte();
				Data.Add(b);
				dwSize--;
				if (b == 0xFF && dwSize > 1) {
					b = gb.ReadByte();
					dwSize--;
					if (!b) {
						b = gb.ReadByte();
						dwSize--;
					}
					Data.Add(b);
				}
			}
		} else {
			Data.SetCount(size);
			gb.ReadBuffer(Data.GetData(), size);
		}
		CGolombBuffer gbData(Data.GetData(), Data.GetCount());
		size = Data.GetCount();

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
				|| tag == '\0ULT' || tag == 'USLT'
				) {
			BYTE encoding = (BYTE)gbData.BitRead(8);
			size--;

			if (tag == 'APIC' || tag == '\0PIC') {
				TCHAR mime[64];
				memset(&mime, 0 ,64);

				int mime_len = 0;
				while (size-- && (mime[mime_len++] = gbData.BitRead(8)) != 0);

				BYTE pic_type = (BYTE)gbData.BitRead(8);
				size--;

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

				CAtlArray<BYTE> data;
				data.SetCount(size);
				gbData.ReadBuffer(data.GetData(), size);

				CID3TagItem* item = DNew CID3TagItem(tag, data, mimeStr);
				TagItems.AddTail(item);
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
					CID3TagItem* item = DNew CID3TagItem(tag, text);
					TagItems.AddTail(item);
				}
			}
		}
	}

	POSITION pos2 = TagItems.GetHeadPosition();
	while (pos2) {
		CID3TagItem* item = TagItems.GetNext(pos2);
		if (item->GetType() == ID3Type::ID3_TYPE_STRING && item->GetTag()) {
			Tags[item->GetTag()] = item->GetValue();
		}
	}

	return TagItems.GetCount() ? TRUE : FALSE;
}

static const LPCTSTR s_genre[] = {
	_T("Blues"), _T("Classic Rock"), _T("Country"), _T("Dance"),
	_T("Disco"), _T("Funk"), _T("Grunge"), _T("Hip-Hop"),
	_T("Jazz"), _T("Metal"), _T("New Age"), _T("Oldies"),
	_T("Other"), _T("Pop"), _T("R&B"), _T("Rap"),
	_T("Reggae"), _T("Rock"), _T("Techno"), _T("Industrial"),
	_T("Alternative"), _T("Ska"), _T("Death Metal"), _T("Pranks"),
	_T("Soundtrack"), _T("Euro-Techno"), _T("Ambient"), _T("Trip-Hop"),
	_T("Vocal"), _T("Jazz+Funk"), _T("Fusion"), _T("Trance"),
	_T("Classical"), _T("Instrumental"), _T("Acid"), _T("House"),
	_T("Game"), _T("Sound Clip"), _T("Gospel"), _T("Noise"),
	_T("Alternative Rock"), _T("Bass"), _T("Soul"), _T("Punk"),
	_T("Space"), _T("Meditative"), _T("Instrumental Pop"), _T("Instrumental Rock"),
	_T("Ethnic"), _T("Gothic"), _T("Darkwave"), _T("Techno-Industrial"),
	_T("Electronic"), _T("Pop-Folk"), _T("Eurodance"), _T("Dream"),
	_T("Southern Rock"), _T("Comedy"), _T("Cult"), _T("Gangsta"),
	_T("Top 40"), _T("Christian Rap"), _T("Pop/Funk"), _T("Jungle"),
	_T("Native US"), _T("Cabaret"), _T("New Wave"), _T("Psychadelic"),
	_T("Rave"), _T("Showtunes"), _T("Trailer"), _T("Lo-Fi"),
	_T("Tribal"), _T("Acid Punk"), _T("Acid Jazz"), _T("Polka"),
	_T("Retro"), _T("Musical"), _T("Rock & Roll"), _T("Hard Rock"),
	_T("Folk"), _T("Folk-Rock"), _T("National Folk"), _T("Swing"),
	_T("Fast Fusion"), _T("Bebob"), _T("Latin"), _T("Revival"),
	_T("Celtic"), _T("Bluegrass"), _T("Avantgarde"), _T("Gothic Rock"),
	_T("Progressive Rock"), _T("Psychedelic Rock"), _T("Symphonic Rock"), _T("Slow Rock"),
	_T("Big Band"), _T("Chorus"), _T("Easy Listening"), _T("Acoustic"),
	_T("Humour"), _T("Speech"), _T("Chanson"), _T("Opera"),
	_T("Chamber Music"), _T("Sonata"), _T("Symphony"), _T("Booty Bass"),
	_T("Primus"), _T("Porn Groove"), _T("Satire"), _T("Slow Jam"),
	_T("Club"), _T("Tango"), _T("Samba"), _T("Folklore"),
	_T("Ballad"), _T("Power Ballad"), _T("Rhytmic Soul"), _T("Freestyle"),
	_T("Duet"), _T("Punk Rock"), _T("Drum Solo"), _T("Acapella"),
	_T("Euro-House"), _T("Dance Hall"), _T("Goa"), _T("Drum & Bass"),
	_T("Club-House"), _T("Hardcore"), _T("Terror"), _T("Indie"),
	_T("BritPop"), _T("Negerpunk"), _T("Polsk Punk"), _T("Beat"),
	_T("Christian Gangsta"), _T("Heavy Metal"), _T("Black Metal"),
	_T("Crossover"), _T("Contemporary C"), _T("Christian Rock"), _T("Merengue"), _T("Salsa"),
	_T("Thrash Metal"), _T("Anime"), _T("JPop"), _T("SynthPop"),
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
	if (!Tags.Lookup(tag, value)) {
		CID3TagItem* item = DNew CID3TagItem(tag, CString(str).Trim());
		TagItems.AddTail(item);

		Tags[tag] = CString(str).Trim();
	}

	// artist
	gb.ReadBuffer((BYTE*)str.GetBufferSetLength(30), 30);
	tag = 'TPE1';
	if (!Tags.Lookup(tag, value)) {
		value = CString(str).Trim();
		CID3TagItem* item = DNew CID3TagItem(tag, value);
		TagItems.AddTail(item);

		Tags[tag] = value;
	}

	// album
	gb.ReadBuffer((BYTE*)str.GetBufferSetLength(30), 30);
	tag = 'TALB';
	if (!Tags.Lookup(tag, value)) {
		value = CString(str).Trim();
		CID3TagItem* item = DNew CID3TagItem(tag, value);
		TagItems.AddTail(item);

		Tags[tag] = value;
	}

	// year
	gb.ReadBuffer((BYTE*)str.GetBufferSetLength(4), 4);
	tag = 'TYER';
	if (!Tags.Lookup(tag, value)) {
		value = CString(str).Trim();
		CID3TagItem* item = DNew CID3TagItem(tag, value);
		TagItems.AddTail(item);

		Tags[tag] = value;
	}

	// comment
	gb.ReadBuffer((BYTE*)str.GetBufferSetLength(30), 30);
	tag = 'COMM';
	if (!Tags.Lookup(tag, value)) {
		value = CString(str).Trim();
		CID3TagItem* item = DNew CID3TagItem(tag, value);
		TagItems.AddTail(item);

		Tags[tag] = value;
	}

	// track
	LPCSTR s = str;
	if (s[28] == 0 && s[29] != 0) {
		tag = 'TRCK';
		if (!Tags.Lookup(tag, value)) {
			value.Format(L"%d", s[29]);
			CID3TagItem* item = DNew CID3TagItem(tag, value);
			TagItems.AddTail(item);

			Tags[tag] = value;
		}
	}

	// genre
	BYTE genre = (BYTE)gb.BitRead(8);
	if (genre < _countof(s_genre)) {
		tag = 'TCON';
		if (!Tags.Lookup(tag, value)) {
			value = CString(s_genre[genre]);
			CID3TagItem* item = DNew CID3TagItem(tag, value);
			TagItems.AddTail(item);

			Tags[tag] = value;
		}
	}

	return TRUE;
}

// additional functions
void SetID3TagProperties(IBaseFilter* pBF, const CID3Tag* ID3tag)
{
	if (!ID3tag || ID3tag->Tags.IsEmpty()) {
		return;
	}

	if (CComQIPtr<IDSMPropertyBag> pPB = pBF) {
		CStringW str, title;
		if (ID3tag->Tags.Lookup('TIT2', str) || ID3tag->Tags.Lookup('\0TT2', str)) {
			title = str;
		}
		if (ID3tag->Tags.Lookup('TYER', str) && !title.IsEmpty() && !str.IsEmpty()) {
			title += L" (" + str + L")";
		}
		if (!title.IsEmpty()) {
			pPB->SetProperty(L"TITL", title);
		}
		if (ID3tag->Tags.Lookup('TPE1', str) || ID3tag->Tags.Lookup('\0TP1', str)) {
			pPB->SetProperty(L"AUTH", str);
		}
		if (ID3tag->Tags.Lookup('TCOP', str)) {
			pPB->SetProperty(L"CPYR", str);
		}
		if (ID3tag->Tags.Lookup('COMM', str)) {
			pPB->SetProperty(L"DESC", str);
		}
		if (ID3tag->Tags.Lookup('USLT', str) || ID3tag->Tags.Lookup('\0ULT', str)) {
			pPB->SetProperty(L"LYRICS", str);
		}
		if (ID3tag->Tags.Lookup('TALB', str) || ID3tag->Tags.Lookup('\0TAL', str)) {
			pPB->SetProperty(L"ALBUM", str);
		}
	}


	if (CComQIPtr<IDSMResourceBag> pRB = pBF) {
		BOOL bResAppend = FALSE;

		POSITION pos = ID3tag->TagItems.GetHeadPosition();
		while (pos && !bResAppend) {
			CID3TagItem* item = ID3tag->TagItems.GetNext(pos);
			if (item->GetType() == ID3Type::ID3_TYPE_BINARY && item->GetDataLen()) {
				CString mime = item->GetMime();
				CString fname;
				if (mime == L"image/jpeg" || mime == L"image/jpg") {
					fname = L"cover.jpg";
				} else if (mime == L"image/png") {
					fname = L"cover.png";
				} else {
					fname = mime;
					fname.Replace(L"image/", L"cover.");
				}

				HRESULT hr2 = pRB->ResAppend(fname, L"cover", mime, (BYTE*)item->GetData(), (DWORD)item->GetDataLen(), 0);
				bResAppend = SUCCEEDED(hr2);
			}
		}
	}
}
