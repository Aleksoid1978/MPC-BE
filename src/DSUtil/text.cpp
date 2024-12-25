/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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
	::TranslateCharsetInfo((DWORD*)(DWORD_PTR)dwCharSet, &cs, TCI_SRCCHARSET);
	return cs.ciACP;
}

CStringA UrlEncode(const CStringA& str_in, const bool bArg/* = false*/)
{
	CStringA str_out;

	for (int i = 0; i < str_in.GetLength(); i++) {
		char c = str_in[i];
		if (bArg && (c == '#' || c == '?' || c == '%' || c == '&' || c == '=')) {
			str_out.AppendFormat("%%%02x", (BYTE)c);
		} else if (c > 0x20 && c < 0x7f) {
			str_out += c;
		} else {
			str_out.AppendFormat("%%%02x", (BYTE)c);
		}
	}

	return str_out;
}

CStringA UrlDecode(const CStringA& str_in)
{
	CStringA str_out;

	for (int i = 0, len = str_in.GetLength(); i < len; i++) {
		if (str_in[i] == '%' && i + 2 < len) {
			bool b = true;
			char c1 = str_in[i + 1];
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
				char c2 = str_in[i + 2];
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

bool Unescape(CStringW& str)
{
	const int len = str.GetLength();
	HRESULT hr = UrlUnescapeW(str.GetBuffer(), nullptr, nullptr, URL_ESCAPE_URI_COMPONENT | URL_UNESCAPE_INPLACE);
	if (SUCCEEDED(hr)) {
		str.ReleaseBuffer();
	}
	return len != str.GetLength();
}

CStringW ExtractTag(CStringW tag, CMapStringToString& attribs, bool& fClosing)
{
	tag.Trim();
	attribs.RemoveAll();

	fClosing = !tag.IsEmpty() ? tag[0] == '/' : false;
	tag.TrimLeft('/');

	int i = tag.Find(' ');
	if (i < 0) {
		i = tag.GetLength();
	}
	CStringW type = tag.Left(i).MakeLower();
	tag = tag.Mid(i).Trim();

	while ((i = tag.Find('=')) > 0) {
		CStringW attrib = tag.Left(i).Trim().MakeLower();
		tag = tag.Mid(i+1);
		for (i = 0; i < tag.GetLength() && _istspace(tag[i]); i++) {
			;
		}
		tag = tag.Mid(i);
		if (!tag.IsEmpty() && tag[0] == '\"') {
			tag = tag.Mid(1);
			i = tag.Find('\"');
		} else {
			i = tag.Find(' ');
		}
		if (i < 0) {
			i = tag.GetLength();
		}
		CStringW param = tag.Left(i).Trim();
		if (!param.IsEmpty()) {
			attribs[attrib] = param;
		}
		tag = tag.Mid(i+1);
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

CStringW ConvertToWStr(LPCSTR lpMultiByteStr, UINT CodePage)
{
	CStringW str;
	int len = MultiByteToWideChar(CodePage, 0, lpMultiByteStr, -1, nullptr, 0) - 1;
	if (len > 0) {
		str.ReleaseBuffer(MultiByteToWideChar(CodePage, 0, lpMultiByteStr, -1, str.GetBuffer(len), len + 1) - 1);
	}
	return str;
}

CStringW UTF8ToWStr(LPCSTR lpUTF8Str)
{
	return ConvertToWStr(lpUTF8Str, CP_UTF8);
}

void ReplaceCharacterU16(wchar_t& ch)
{
	if (!SysVersion::IsWin8orLater()) {
		switch (ch) {
		case 0x2705: // White Heavy Check Mark
			ch = 0x2714; // Heavy Check Mark
			return;
		}
		if (ch >= 0x2667 && ch <= 0x2775) {
			ch = 0x0020;
		}
	}
}

void ReplaceCharacterU32(uint32_t& ch)
{
	if (!SysVersion::IsWin10orLater()) {
		switch (ch) {
		case 0x1F388: // Balloon
		case 0x1F534: // Large Red Circle
		case 0x1F535: // Large Blue Circle
			ch = 0x25CF; // Black Circle
			return;
		}
		if (ch >= 0x1F600 && ch <= 0x1F64F) { // Emoticons
			ch = 0x263A; // White Smiling Face
		}
		else if (ch >= 0x1F300) {
			ch = 0x0020;
		}
	}
}

CStringW AltUTF8ToWStr(LPCSTR lpUTF8Str) // Use if MultiByteToWideChar() function does not work.
{
	if (!lpUTF8Str) {
		return L"";
	}

	CStringW str;
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
				wchar_t u16 = (wchar_t)((((wchar_t)(*Z & 0x0F)) << 12) | ((*(Z + 1) & 0x3F) << 6) | (*(Z + 2) & 0x3F));
				ReplaceCharacterU16(u16);
				str += u16;

				Z += 3;
			} else {
				return L""; //Bad character
			}
		}
		//4 bytes
		else if ((*Z & 0xF8) == 0xF0) {
			if ((*(Z + 1) & 0xC0) == 0x80 && (*(Z + 2) & 0xC0) == 0x80 && (*(Z + 3) & 0xC0) == 0x80) {
				uint32_t u32 = ((uint32_t)(*Z & 0x0F) << 18) | ((uint32_t)(*(Z + 1) & 0x3F) << 12) | ((uint32_t)(*(Z + 2) & 0x3F) << 6) | ((uint32_t)*(Z + 3) & 0x3F);
				ReplaceCharacterU32(u32);
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

CStringW UTF8orLocalToWStr(LPCSTR lpMultiByteStr)
{
	CStringW str = AltUTF8ToWStr(lpMultiByteStr);
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
				break;
			case '\t':
				str.GetBuffer()[i] = ' ';
				break;
		}
	}

	CStringW tmp;
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

void EllipsisText(CStringW& text, const int maxlen)
{
	ASSERT(maxlen > 40);

	if (text.GetLength() > maxlen) {
		int minsize = maxlen - 12;
		for (int i = maxlen - 1; i > minsize; i--) {
			auto ch = text[i];
			if (ch < L'0' || (ch > '9' && ch < 'A') || (ch > 'Z' && ch < 'a') || (ch > 'Z' && ch <= 0xA0)) {
				text.Truncate(i);
				break;
			}
		}
		if (text.GetLength() > maxlen) {
			text.Truncate(maxlen);
		}
		text.TrimRight();
		text.AppendChar(L'\x2026');
	}
}

void EllipsisURL(CStringW& url, const int maxlen)
{
	if (url.GetLength() > maxlen) {
		int k = url.Find(L"://");
		if (k > 0) {
			k = url.Find('/', k+1);
			if (k > 0) {
				int q = url.Find('?', k+1);
				if (q > 0 && q < maxlen) {
					k = q;
				} else {
					while ((q = url.Find('/', k+1)) > 0 && q < maxlen) {
						k = q;
					}
				}
				url.Truncate(std::max(maxlen, k));
				url.AppendChar(L'\x2026');
			}
		}
	}
}

void EllipsisPath(CStringW& path, const int maxlen)
{
	ASSERT(maxlen > 10);

	if (path.GetLength() > maxlen) {
		int k = -1;
		if (StartsWith(path, L"\\\\")) {
			if (StartsWith(path, L"?\\", 2) && StartsWith(path, L":\\", 5)) {
				k = 6;
			}
			else {
				k = path.Find('\\', 3);
			}
		}
		else if (StartsWith(path, L":\\", 1)) {
			k = 2;
		}

		if (k > 0) {
			const int n = path.ReverseFind('\\');
			if (n > k) {
				const int midlen = maxlen + n - path.GetLength();
				int q = -1;
				while ((q = path.Find('\\', k+1)) > 0 && q < midlen) {
					k = q;
				}
				path = path.Left(k + 1) + L'\x2026' + path.Mid(n);
			}
		}
	}
}

CStringW FormatNumber(const CStringW& szNumber, const bool bNoFractionalDigits /*= true*/)
{
	CStringW ret;

	int nChars = GetNumberFormatW(LOCALE_USER_DEFAULT, 0, szNumber, nullptr, nullptr, 0);
	GetNumberFormatW(LOCALE_USER_DEFAULT, 0, szNumber, nullptr, ret.GetBuffer(nChars), nChars);
	ret.ReleaseBuffer();

	if (bNoFractionalDigits) {
		WCHAR szNumberFractionalDigits[2] = {0};
		GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, szNumberFractionalDigits, std::size(szNumberFractionalDigits));
		int nNumberFractionalDigits = wcstol(szNumberFractionalDigits, nullptr, 10);
		if (nNumberFractionalDigits) {
			ret.Truncate(ret.GetLength() - nNumberFractionalDigits - 1);
		}
	}

	return ret;
}

CStringW FourccToWStr(uint32_t fourcc)
{
	if (fourcc == 0) {
		return L"0";
	}

	CStringW ret;

	for (unsigned i = 0; i < 4; i++) {
		const uint32_t c = fourcc & 0xff;
		ret.AppendFormat(c < 32 ? L"[%u]" : L"%C", c);
		fourcc >>= 8;
	}

	return ret;
}
