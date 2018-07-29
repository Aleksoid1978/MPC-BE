/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include "SysVersion.h"
#include "text.h"

DWORD CharSetToCodePage(DWORD dwCharSet)
{
	if (dwCharSet == CP_UTF8) {
		return CP_UTF8;
	}
	if (dwCharSet == CP_UTF7) {
		return CP_UTF7;
	}
	CHARSETINFO cs= {0};
	::TranslateCharsetInfo((DWORD *)dwCharSet, &cs, TCI_SRCCHARSET);
	return cs.ciACP;
}

CStringA ConvertMBCS(CStringA str, DWORD SrcCharSet, DWORD DstCharSet)
{
	WCHAR* utf16 = DNew WCHAR[str.GetLength()+1];
	memset(utf16, 0, (str.GetLength()+1)*sizeof(WCHAR));

	CHAR* mbcs = DNew CHAR[str.GetLength()*6+1];
	memset(mbcs, 0, str.GetLength()*6+1);

	int len = MultiByteToWideChar(
				  CharSetToCodePage(SrcCharSet), 0,
				  str, -1, // null terminated string
				  utf16, str.GetLength()+1);

	len = WideCharToMultiByte(
			  CharSetToCodePage(DstCharSet), 0,
			  utf16, len,
			  mbcs, str.GetLength()*6,
			  nullptr, nullptr);

	str = mbcs;

	delete [] utf16;
	delete [] mbcs;

	return str;
}

CStringA UrlEncode(CStringA str_in, bool fArg)
{
	CStringA str_out;

	for (int i = 0; i < str_in.GetLength(); i++) {
		char c = str_in[i];
		if (fArg && (c == '#' || c == '?' || c == '%' || c == '&' || c == '=')) {
			str_out.AppendFormat("%%%02x", (BYTE)c);
		} else if (c > 0x20 && c < 0x7f) {
			str_out += c;
		} else {
			str_out.AppendFormat("%%%02x", (BYTE)c);
		}
	}

	return str_out;
}

CStringA UrlDecode(CStringA str_in)
{
	CStringA str_out;

	for (int i = 0, len = str_in.GetLength(); i < len; i++) {
		if (str_in[i] == '%' && i + 2 < len) {
			bool b = true;
			char c1 = str_in[i+1];
			if (c1 >= '0' && c1 <= '9') {
				c1 -= '0';
			} else if (c1 >= 'A' && c1 <= 'F') {
				c1 -= 'A' - 10;
			} else if (c1 >= 'a' && c1 <= 'f') {
				c1 -= 'a' - 10;
			} else {
				b = false;
			}
			if (b) {
				char c2 = str_in[i+2];
				if (c2 >= '0' && c2 <= '9') {
					c2 -= '0';
				} else if (c2 >= 'A' && c2 <= 'F') {
					c2 -= 'A' - 10;
				} else if (c2 >= 'a' && c2 <= 'f') {
					c2 -= 'a' - 10;
				} else {
					b = false;
				}
				if (b) {
					str_out += (char)((c1 << 4) | c2);
					i += 2;
					continue;
				}
			}
		}
		str_out += str_in[i];
	}

	return str_out;
}

CString ExtractTag(CString tag, CMapStringToString& attribs, bool& fClosing)
{
	tag.Trim();
	attribs.RemoveAll();

	fClosing = !tag.IsEmpty() ? tag[0] == '/' : false;
	tag.TrimLeft('/');

	int i = tag.Find(' ');
	if (i < 0) {
		i = tag.GetLength();
	}
	CString type = tag.Left(i).MakeLower();
	tag = tag.Mid(i).Trim();

	while ((i = tag.Find('=')) > 0) {
		CString attrib = tag.Left(i).Trim().MakeLower();
		tag = tag.Mid(i+1);
		for (i = 0; i < tag.GetLength() && _istspace(tag[i]); i++) {
			;
		}
		tag = i < tag.GetLength() ? tag.Mid(i) : L"";
		if (!tag.IsEmpty() && tag[0] == '\"') {
			tag = tag.Mid(1);
			i = tag.Find('\"');
		} else {
			i = tag.Find(' ');
		}
		if (i < 0) {
			i = tag.GetLength();
		}
		CString param = tag.Left(i).Trim();
		if (!param.IsEmpty()) {
			attribs[attrib] = param;
		}
		tag = i+1 < tag.GetLength() ? tag.Mid(i+1) : L"";
	}

	return type;
}

CStringA HtmlSpecialChars(CStringA str, bool bQuotes /*= false*/)
{
	str.Replace("&", "&amp;");
	str.Replace("\"", "&quot;");
	if (bQuotes) {
		str.Replace("\'", "&#039;");
	}
	str.Replace("<", "&lt;");
	str.Replace(">", "&gt;");

	return str;
}

CStringA WStrToUTF8(LPCWSTR lpWideCharStr)
{
	CStringA str;
	int len = WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, -1, nullptr, 0, nullptr, nullptr) - 1;
	if (len > 0) {
		str.ReleaseBuffer(WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, -1, str.GetBuffer(len), len + 1, nullptr, nullptr) - 1);
	}
	return str;
}

CString ConvertToWStr(LPCSTR lpMultiByteStr, UINT CodePage)
{
	CString str;
	int len = MultiByteToWideChar(CodePage, 0, lpMultiByteStr, -1, nullptr, 0) - 1;
	if (len > 0) {
		str.ReleaseBuffer(MultiByteToWideChar(CodePage, 0, lpMultiByteStr, -1, str.GetBuffer(len), len + 1) - 1);
	}
	return str;
}

CString UTF8ToWStr(LPCSTR lpUTF8Str)
{
	return ConvertToWStr(lpUTF8Str, CP_UTF8);
}

void ReplaceCharacter(uint32_t& ch)
{
	if (!SysVersion::IsWin10orLater()) {
		if (ch == 0x1F534 || ch == 0x1F535) { // Large Red Circle, Large Blue Circle
			ch = 0x25CF; // Black Circle
		}
		else if (ch >= 0x1F600 && ch <= 0x1F64F) { // Emoticons
			ch = 0x263A; // White Smiling Face
		}
	}
}

CString AltUTF8ToWStr(LPCSTR lpUTF8Str) // Use if MultiByteToWideChar() function does not work.
{
	if (!lpUTF8Str) {
		return L"";
	}

	CString str;
	// Don't use MultiByteToWideChar(), some characters are not well decoded
	const unsigned char* Z = (const unsigned char*)lpUTF8Str;
	while (*Z) { //0 is end
		//1 byte
		if (*Z < 0x80) {
			str += (wchar_t)(*Z);
			Z++;
		}
		//2 bytes
		else if ((*Z & 0xE0) == 0xC0) {
			if ((*(Z + 1) & 0xC0) == 0x80) {
				str += (wchar_t)((((wchar_t)(*Z & 0x1F)) << 6) | (*(Z + 1) & 0x3F));
				Z += 2;
			} else {
				return L""; //Bad character
			}
		}
		//3 bytes
		else if ((*Z & 0xF0) == 0xE0) {
			if ((*(Z + 1) & 0xC0) == 0x80 && (*(Z + 2) & 0xC0) == 0x80) {
				str += (wchar_t)((((wchar_t)(*Z & 0x0F)) << 12) | ((*(Z + 1) & 0x3F) << 6) | (*(Z + 2) & 0x3F));
				Z += 3;
			} else {
				return L""; //Bad character
			}
		}
		//4 bytes
		else if ((*Z & 0xF8) == 0xF0) {
			if ((*(Z + 1) & 0xC0) == 0x80 && (*(Z + 2) & 0xC0) == 0x80 && (*(Z + 3) & 0xC0) == 0x80) {
				uint32_t u32 = ((uint32_t)(*Z & 0x0F) << 18) | ((uint32_t)(*(Z + 1) & 0x3F) << 12) | ((uint32_t)(*(Z + 2) & 0x3F) << 6) | ((uint32_t)*(Z + 3) & 0x3F);
				ReplaceCharacter(u32);
				if (u32 <= UINT16_MAX) {
					str += (wchar_t)u32;
				} else {
					str += (wchar_t)((((u32 - 0x010000) & 0x000FFC00) >> 10) | 0xD800);
					str += (wchar_t)((u32 & 0x000003FF) | 0xDC00);
				}
				Z += 4;
			} else {
				return L""; //Bad character
			}
		}
		else {
			return L""; //Bad character
		}
	}

	return str;
}

CString UTF8orLocalToWStr(LPCSTR lpMultiByteStr)
{
	CString str = AltUTF8ToWStr(lpMultiByteStr);
	if (str.IsEmpty()) {
		str = ConvertToWStr(lpMultiByteStr, CP_ACP); // Trying Local...
	}

	return str;
}

void FixFilename(CStringW& str)
{
	str.Trim();

	for (int i = 0, l = str.GetLength(); i < l; i++) {
		switch (str[i]) {
			case '?':
			case '"':
			case '/':
			case '\\':
			case '<':
			case '>':
			case '*':
			case '|':
			case ':':
				str.GetBuffer()[i] = '_';
		}
	}

	CString tmp;
	// not support the following file names: "con", "con.txt" "con.name.txt". But supported "content" "name.con" and "name.con.txt".
	if (str.GetLength() == 3 || str.Find('.') == 3) {
		tmp = str.Left(3).MakeUpper();
		if (tmp == L"CON" || tmp == L"AUX" || tmp == L"PRN" || tmp == L"NUL") {
			LPWSTR buffer = str.GetBuffer();
			buffer[0] = '_';
			buffer[1] = '_';
			buffer[2] = '_';
		}
	}
	else if (str.GetLength() == 4 || str.Find('.') == 4) {
		tmp = str.Left(4).MakeUpper();
		if (tmp == L"COM1" || tmp == L"COM2" || tmp == L"COM3" || tmp == L"COM4" || tmp == L"LPT1" || tmp == L"LPT2" || tmp == L"LPT3") {
			LPWSTR buffer = str.GetBuffer();
			buffer[0] = '_';
			buffer[1] = '_';
			buffer[2] = '_';
			buffer[3] = '_';
		}
	}
}

CString FormatNumber(CString szNumber, bool bNoFractionalDigits /*= true*/)
{
	CString ret;

	int nChars = GetNumberFormatW(LOCALE_USER_DEFAULT, 0, szNumber, nullptr, nullptr, 0);
	GetNumberFormatW(LOCALE_USER_DEFAULT, 0, szNumber, nullptr, ret.GetBuffer(nChars), nChars);
	ret.ReleaseBuffer();

	if (bNoFractionalDigits) {
		WCHAR szNumberFractionalDigits[2] = {0};
		GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, szNumberFractionalDigits, _countof(szNumberFractionalDigits));
		int nNumberFractionalDigits = wcstol(szNumberFractionalDigits, nullptr, 10);
		if (nNumberFractionalDigits) {
			ret.Truncate(ret.GetLength() - nNumberFractionalDigits - 1);
		}
	}

	return ret;
}
