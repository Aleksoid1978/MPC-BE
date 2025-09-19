/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
#include "STS.h"
#include <fstream>
#include <regex>
#include "RealTextParser.h"
#include "USFSubtitles.h"
#include "RegexUtil.h"
#include "DSUtil/std_helper.h"

static struct htmlcolor {
	LPCWSTR name;
	DWORD color;
} htmlcolors[] = {
	{L"white", 0xffffff},
	{L"whitesmoke", 0xf5f5f5},
	{L"ghostwhite", 0xf8f8ff},
	{L"snow", 0xfffafa},
	{L"gainsboro", 0xdcdcdc},
	{L"lightgrey", 0xd3d3d3},
	{L"silver", 0xc0c0c0},
	{L"darkgray", 0xa9a9a9},
	{L"gray", 0x808080},
	{L"dimgray", 0x696969},
	{L"lightslategray", 0x778899},
	{L"slategray", 0x708090},
	{L"darkslategray", 0x2f4f4f},
	{L"black", 0x000000},

	{L"azure", 0xf0ffff},
	{L"aliceblue", 0xf0f8ff},
	{L"mintcream", 0xf5fffa},
	{L"honeydew", 0xf0fff0},
	{L"lightcyan", 0xe0ffff},
	{L"paleturqoise", 0xafeeee},
	{L"powderblue", 0xb0e0e6},
	{L"lightblue", 0xadd8ed},
	{L"lightsteelblue", 0xb0c4de},
	{L"skyblue", 0x87ceeb},
	{L"lightskyblue", 0x87cefa},
	{L"cyan", 0x00ffff},
	{L"aqua", 0x00ff80},
	{L"deepskyblue", 0x00bfff},
	{L"aquamarine", 0x7fffd4},
	{L"turquoise", 0x40e0d0},
	{L"darkturquoise", 0x00ced1},
	{L"lightseagreen", 0x20b2aa},
	{L"mediumturquoise", 0x40e0dd},
	{L"mediumaquamarine", 0x66cdaa},
	{L"cadetblue", 0x5f9ea0},
	{L"teal", 0x008080},
	{L"darkcyan", 0x008b8b},
	{L"comflowerblue", 0x6495ed},
	{L"dodgerblue", 0x1e90ff},
	{L"steelblue", 0x4682b4},
	{L"royalblue", 0x4169e1},
	{L"blue", 0x0000ff},
	{L"mediumblue", 0x0000cd},
	{L"mediumslateblue", 0x7b68ee},
	{L"slateblue", 0x6a5acd},
	{L"darkslateblue", 0x483d8b},
	{L"darkblue", 0x00008b},
	{L"midnightblue", 0x191970},
	{L"navy", 0x000080},

	{L"palegreen", 0x98fb98},
	{L"lightgreen", 0x90ee90},
	{L"mediumspringgreen", 0x00fa9a},
	{L"springgreen", 0x00ff7f},
	{L"chartreuse", 0x7fff00},
	{L"lawngreen", 0x7cfc00},
	{L"lime", 0x00ff00},
	{L"limegreen", 0x32cd32},
	{L"greenyellow", 0xadff2f},
	{L"yellowgreen", 0x9acd32},
	{L"darkseagreen", 0x8fbc8f},
	{L"mediumseagreen", 0x3cb371},
	{L"seagreen", 0x2e8b57},
	{L"olivedrab", 0x6b8e23},
	{L"forestgreen", 0x228b22},
	{L"green", 0x008000},
	{L"darkkhaki", 0xbdb76b},
	{L"olive", 0x808000},
	{L"darkolivegreen", 0x556b2f},
	{L"darkgreen", 0x006400},

	{L"floralwhite", 0xfffaf0},
	{L"seashell", 0xfff5ee},
	{L"ivory", 0xfffff0},
	{L"beige", 0xf5f5dc},
	{L"cornsilk", 0xfff8dc},
	{L"lemonchiffon", 0xfffacd},
	{L"lightyellow", 0xffffe0},
	{L"lightgoldenrodyellow", 0xfafad2},
	{L"papayawhip", 0xffefd5},
	{L"blanchedalmond", 0xffedcd},
	{L"palegoldenrod", 0xeee8aa},
	{L"khaki", 0xf0eb8c},
	{L"bisque", 0xffe4c4},
	{L"moccasin", 0xffe4b5},
	{L"navajowhite", 0xffdead},
	{L"peachpuff", 0xffdab9},
	{L"yellow", 0xffff00},
	{L"gold", 0xffd700},
	{L"wheat", 0xf5deb3},
	{L"orange", 0xffa500},
	{L"darkorange", 0xff8c00},

	{L"oldlace", 0xfdf5e6},
	{L"linen", 0xfaf0e6},
	{L"antiquewhite", 0xfaebd7},
	{L"lightsalmon", 0xffa07a},
	{L"darksalmon", 0xe9967a},
	{L"salmon", 0xfa8072},
	{L"lightcoral", 0xf08080},
	{L"indianred", 0xcd5c5c},
	{L"coral", 0xff7f50},
	{L"tomato", 0xff6347},
	{L"orangered", 0xff4500},
	{L"red", 0xff0000},
	{L"crimson", 0xdc143c},
	{L"firebrick", 0xb22222},
	{L"maroon", 0x800000},
	{L"darkred", 0x8b0000},

	{L"lavender", 0xe6e6fe},
	{L"lavenderblush", 0xfff0f5},
	{L"mistyrose", 0xffe4e1},
	{L"thistle", 0xd8bfd8},
	{L"pink", 0xffc0cb},
	{L"lightpink", 0xffb6c1},
	{L"palevioletred", 0xdb7093},
	{L"hotpink", 0xff69b4},
	{L"fuchsia", 0xff00ee},
	{L"magenta", 0xff00ff},
	{L"mediumvioletred", 0xc71585},
	{L"deeppink", 0xff1493},
	{L"plum", 0xdda0dd},
	{L"violet", 0xee82ee},
	{L"orchid", 0xda70d6},
	{L"mediumorchid", 0xba55d3},
	{L"mediumpurple", 0x9370db},
	{L"purple", 0x9370db},
	{L"blueviolet", 0x8a2be2},
	{L"darkviolet", 0x9400d3},
	{L"darkorchid", 0x9932cc},

	{L"tan", 0xd2b48c},
	{L"burlywood", 0xdeb887},
	{L"sandybrown", 0xf4a460},
	{L"peru", 0xcd853f},
	{L"goldenrod", 0xdaa520},
	{L"darkgoldenrod", 0xb8860b},
	{L"chocolate", 0xd2691e},
	{L"rosybrown", 0xbc8f8f},
	{L"sienna", 0xa0522d},
	{L"saddlebrown", 0x8b4513},
	{L"brown", 0xa52a2a},
};

CHtmlColorMap::CHtmlColorMap()
{
	for (size_t i = 0; i < std::size(htmlcolors); i++) {
		SetAt(htmlcolors[i].name, htmlcolors[i].color);
	}
}

CHtmlColorMap g_colors;

static CStringW SSAColorTag(CStringW arg, CStringW ctag = L"c") {
	DWORD val, color;
	if (g_colors.Lookup(CString(arg), val)) {
		color = (DWORD)val;
	}
	else if ((color = wcstol(arg, nullptr, 16)) == 0) {
		color = 0x00ffffff;    // default is white
	}
	CStringW tmp;
	tmp.Format(L"%02x%02x%02x", color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff);
	return CStringW(L"{\\" + ctag + L"&H") + tmp + L"&}";
}

static std::wstring SSAColorTagCS(std::wstring arg, CStringW ctag = L"c") {
	CStringW _arg(arg.c_str());
	return SSAColorTag(_arg, ctag).GetString();
}

//

static CStringW RemoveSSATags(CStringW str)
{
	str.Replace (L"{\\i1}", L"<i>");
	str.Replace (L"{\\i}", L"</i>");

	for (int i = 0, j; i < str.GetLength(); ) {
		if ((i = str.Find(L'{', i)) < 0) {
			break;
		}
		if ((j = str.Find(L'}', i)) < 0) {
			break;
		}
		str.Delete(i, j-i+1);
	}

	str.Replace(L"\\N", L"\n");
	str.Replace(L"\\n", L"\n");
	str.Replace(L"\\h", L" ");

	return str;
}

//

static CStringW SubRipper2SSA(CStringW str)
{
	str.Replace(L"<i>", L"{\\i1}");
	str.Replace(L"</i>", L"{\\i}");
	str.Replace(L"<b>", L"{\\b1}");
	str.Replace(L"</b>", L"{\\b}");
	str.Replace(L"<u>", L"{\\u1}");
	str.Replace(L"</u>", L"{\\u}");
	str.Replace(L"<br>", L"\n");

	return str;
}

static CStringW WebVTTCueStrip(CStringW& str)
{
	CStringW cues;
	int p = str.Find(L'\n');
	if (p == 0) p = str.Find(L'\r');
	if (p > 0 && p < 6) { // check for optional cue id: https://w3c.github.io/webvtt/#webvtt-cue-identifier
		int cueid;
		WCHAR cr;
		int c = swscanf_s(str.Left(p), L"%d%c", &cueid, &cr, 1);
		if (c == 1 || c == 2 && cr == L'\r') {
			str.Delete(0, p + 1);
			p = str.Find(L'\n');
			if (p == 0) p = str.Find(L'\r');
		}
	}
	if (p > 0) {
		if (str.Left(6) == L"align:" || str.Left(9) == L"position:" || str.Left(9) == L"vertical:" || str.Left(5) == L"line:" || str.Left(5) == L"size:") {
			if (p > 1 && str[p - 1] == L'\r') {
				cues = str.Left(p - 1);
			}
			else {
				cues = str.Left(p);
			}
			str.Delete(0, p + 1);
		}
	}
	return cues;
}

using WebVTTcolorData = struct _WebVTTcolorData { std::wstring color; std::wstring bg; bool applied = false; };
using WebVTTcolorMap = std::map<std::wstring, WebVTTcolorData>;

static void WebVTT2SSA(CStringW& str, CStringW& cueTags, WebVTTcolorMap clrMap)
{
	std::vector<WebVTTcolorData> styleStack;
	auto applyStyle = [&styleStack, &str](std::wstring clr, std::wstring bg, int endTag, bool restoring = false) {
		std::wstring tags = L"";
		WebVTTcolorData previous;
		bool applied = false;
		if (styleStack.size() > 0 && !restoring) {
			auto tmp = styleStack.back();
			if (tmp.applied) {
				previous = tmp;
			}
		}
		if (clr != L"" && clr != previous.color) {
			tags += SSAColorTagCS(clr);
		}
		if (bg != L"" && bg != previous.bg) {
			tags += SSAColorTagCS(bg, L"3c");
		}
		if (tags.length() > 0) {
			if (-1 == endTag) {
				str = tags.c_str() + str;
				applied = true;
			}
			else if (str.Mid(endTag + 1, 1) != "<") { //if we are about to open or close a tag, don't set the style yet, as it may change before formattable text arrives
				str = str.Left(endTag + 1) + tags.c_str() + str.Mid(endTag + 1);
				applied = true;
			}
		}
		if (!restoring) {
			styleStack.push_back({ clr, bg, applied }); //push current colors for restoring
		}
	};

	std::wstring clr = L"", bg = L"";
	if (clrMap.count(L"::cue")) { //default cue style
		WebVTTcolorData colorData = clrMap[L"::cue"];
		clr = colorData.color;
		bg = colorData.bg;
		applyStyle(clr, bg, -1);
	}

	int tagPos = str.Find(L"<");
	while (tagPos != std::wstring::npos) {
		int endTag = str.Find(L">", tagPos);
		if (endTag == std::wstring::npos) break;
		CStringW inner = str.Mid(tagPos + 1, endTag - tagPos - 1);
		if (inner.Find(L"/") == 0) { //close tag
			if (styleStack.size() > 0) {//should always be true, unless poorly matched close tags in source
				styleStack.pop_back();
			}
			if (styleStack.size() > 0) {
				auto restoreStyle = styleStack[styleStack.size() - 1];
				clr = restoreStyle.color;
				bg = restoreStyle.bg;
				applyStyle(clr, bg, endTag, true);
			}
			else { //reset default style
				if (endTag + 1 != str.GetLength()) {
					str = str.Left(endTag + 1) + L"{\\r}" + str.Mid(endTag + 1);
				}
				clr = L"";
				bg = L"";
			}
			tagPos = str.Find(L"<", endTag);
			continue;
		}

		int dotPos = inner.Find(L".");
		if (dotPos == std::wstring::npos) {//it's a simple tag, so we can apply a single style to it, if it exists
			if (clrMap.count(inner.GetString())) {
				WebVTTcolorData colorData = clrMap[inner.GetString()];
				clr = colorData.color;
				bg = colorData.bg;
			}
		}
		else { //could find multiple classes
			RegexUtil::wregexResults results;
			std::wregex clsPattern(LR"((\.?[^\.]+))");
			RegexUtil::wstringMatch(clsPattern, (const wchar_t*)inner, results);
			if (results.size() > 1) {
				std::wstring type = results[0][0];

				for (auto iter = results.begin() + 1; iter != results.end(); ++iter) { //loop through all classes--whichever is last gets precedence
					std::wstring cls = (*iter)[0];
					WebVTTcolorData colorData;
					if (clrMap.count(type + cls)) {
						colorData = clrMap[type + cls];
					}
					else if (clrMap.count(cls)) {
						colorData = clrMap[cls];
					}
					if (colorData.color != L"") {
						clr = colorData.color;
					}
					if (colorData.bg != L"") {
						bg = colorData.bg;
					}
				}
			}
		}

		applyStyle(clr, bg, endTag);
		tagPos = str.Find(L"<", endTag);
	}

	if (str.Find(L'<') >= 0) {
		str.Replace(L"<i>", L"{\\i1}");
		str.Replace(L"</i>", L"{\\i}");
		str.Replace(L"<b>", L"{\\b1}");
		str.Replace(L"</b>", L"{\\b}");
		str.Replace(L"<u>", L"{\\u1}");
		str.Replace(L"</u>", L"{\\u}");
	}

	if (str.Find(L'<') >= 0) {
		std::wstring stdTmp(str);

		// remove tags we don't support
		stdTmp = std::regex_replace(stdTmp, std::wregex(L"<c[.\\w\\d]*>"), L"");
		stdTmp = std::regex_replace(stdTmp, std::wregex(L"</c[.\\w\\d]*>"), L"");
		stdTmp = std::regex_replace(stdTmp, std::wregex(L"<\\d\\d:\\d\\d:\\d\\d.\\d\\d\\d>"), L"");
		stdTmp = std::regex_replace(stdTmp, std::wregex(L"<v[ .][^>]*>"), L"");
		stdTmp = std::regex_replace(stdTmp, std::wregex(L"</v>"), L"");
		stdTmp = std::regex_replace(stdTmp, std::wregex(L"<lang[^>]*>"), L"");
		stdTmp = std::regex_replace(stdTmp, std::wregex(L"</lang>"), L"");
		str = stdTmp.c_str();
	}
	if (str.Find(L'&') >= 0) {
		str.Replace(L"&lt;", L"<");
		str.Replace(L"&gt;", L">");
		str.Replace(L"&nbsp;", L"\\h");
		str.Replace(L"&lrm;", L"");
		str.Replace(L"&rlm;", L"");
		str.Replace(L"&amp;", L"&");
	}

	if (!cueTags.IsEmpty()) {
		std::wstring stdTmp(cueTags);
		std::wregex alignRegex(L"align:(start|left|center|middle|end|right)");
		std::wsmatch match;

		if (std::regex_search(stdTmp, match, alignRegex)) {
			if (match[1] == L"start" || match[1] == L"left") {
				str = L"{\\an1}" + str;
			}
			else if (match[1] == L"center" || match[1] == L"middle") {
				str = L"{\\an2}" + str;
			}
			else {
				str = L"{\\an3}" + str;
			}
		}
	}
}

static void WebVTT2SSA(CStringW& str) {
	CStringW discard;
	WebVTTcolorMap discardMap;
	WebVTT2SSA(str, discard, discardMap);
}

static bool OpenVTT(CTextFile* file, CSimpleTextSubtitle& ret) {
	CStringW buff;
	file->ReadString(buff);
	if (buff.Left(6).Compare(L"WEBVTT") != 0) {
		return false;
	}

	auto readTimeCode = [](LPCWSTR str, int& hh, int& mm, int& ss, int& ms) {
		WCHAR sep;
		int c = swscanf_s(str, L"%d%c%d%c%d%c%d",
			&hh, &sep, 1, &mm, &sep, 1, &ss, &sep, 1, &ms);
		if (c == 5) {
			// Hours value is absent, shift read values
			ms = ss;
			ss = mm;
			mm = hh;
			hh = 0;
		}
		return (c == 5 || c == 7);
	};

	//default cue color classes: https://w3c.github.io/webvtt/#default-text-color
	WebVTTcolorMap cueColors = {
		{L".white", WebVTTcolorData({L"ffffff", L""})},
		{L".lime", WebVTTcolorData({L"00ff00", L""})},
		{L".cyan", WebVTTcolorData({L"00ffff", L""})},
		{L".red", WebVTTcolorData({L"ff0000", L""})},
		{L".yellow", WebVTTcolorData({L"ffff00", L""})},
		{L".magenta", WebVTTcolorData({L"ff00ff", L""})},
		{L".blue", WebVTTcolorData({L"0000ff", L""})},
		{L".black", WebVTTcolorData({L"000000", L""})},
		{L".bg_white", WebVTTcolorData({L"", L"ffffff"})},
		{L".bg_lime", WebVTTcolorData({L"", L"00ff00"})},
		{L".bg_cyan", WebVTTcolorData({L"", L"00ffff"})},
		{L".bg_red", WebVTTcolorData({L"", L"ff0000"})},
		{L".bg_yellow", WebVTTcolorData({L"", L"ffff00"})},
		{L".bg_magenta", WebVTTcolorData({L"", L"ff00ff"})},
		{L".bg_blue", WebVTTcolorData({L"", L"0000ff"})},
		{L".bg_black", WebVTTcolorData({L"", L"000000"})},
	};

	CStringW start, end, cueTags;

	auto parseStyle = [&file, &cueColors](CStringW& buff) {
		CStringW styleStr = L"";
		while (file->ReadString(buff)) {
			if (buff.Find(L"-->") != -1) { //not allowed in style block, so we drop out to cue parsing below
				FastTrimRight(buff);
				break;
			}
			if (buff.IsEmpty()) { //empty line not allowed in style block, drop out
				break;
			}
			styleStr += L" " + buff;
		}

		int startComment = styleStr.Find(L"/*");
		while (startComment != -1) { //remove comments
			int endComment = styleStr.Find(L"*/", startComment + 2);
			if (endComment == -1) {
				endComment = styleStr.GetLength() - 1;
			}
			styleStr.Delete(startComment, endComment - startComment + 1);
			startComment = styleStr.Find(L"/*");
		}

		if (!styleStr.IsEmpty()) {
			auto parseColor = [](std::wstring styles, std::wstring attr = L"color") {
				//we only support color styles for now
				std::wregex clrPat(LR"(^\s*)" + attr + LR"(\s*:\s*#?([a-zA-Z0-9]*)\s*;)"); //e.g., 0xffffff or white
				std::wregex rgbPat(LR"(^\s*)" + attr + LR"(\s*:\s*rgb\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*;)");
				std::wsmatch match;
				std::wstring clrStr = L"";
				if (std::regex_search(styles, match, clrPat)) {
					clrStr = match[1];
				}
				else if (std::regex_search(styles, match, rgbPat)) {
					int r = stoi(match[1]) & 0xff;
					int g = stoi(match[2]) & 0xff;
					int b = stoi(match[3]) & 0xff;
					DWORD clr = (r << 16) + (g << 8) + b;
					std::wstringstream hexClr;
					hexClr << std::hex << clr;
					clrStr = hexClr.str();
				}
				return clrStr;
			};

			RegexUtil::wregexResults results;
			std::wregex cueDefPattern(LR"(::cue\s*\{([^}]*)\})"); //default cue style
			RegexUtil::wstringMatch(cueDefPattern, (const wchar_t*)styleStr, results);
			if (results.size() > 0) {
				auto iter = results[results.size() - 1];
				std::wstring clr, bgClr;
				clr = parseColor(iter[0]);
				bgClr = parseColor(iter[0], L"background-color");
				if (bgClr == L"") {
					bgClr = parseColor(iter[0], L"background");
				}
				if (clr != L"" || bgClr != L"") {
					cueColors[L"::cue"] = WebVTTcolorData({ clr, bgClr });
				}
			}

			std::wregex cuePattern(LR"(::cue\(([^)]+)\)\s*\{([^}]*)\})");
			RegexUtil::wstringMatch(cuePattern, (const wchar_t*)styleStr, results);
			for (const auto& iter : results) {
				std::wstring clr, bgClr;
				clr = parseColor(iter[1]);
				bgClr = parseColor(iter[1], L"background-color");
				if (bgClr == L"") {
					bgClr = parseColor(iter[1], L"background");
				}
				if (clr != L"" || bgClr != L"") {
					cueColors[iter[0]] = WebVTTcolorData({ clr, bgClr });
				}
			}
		}
	};

	CStringW lastStr, lastBuff;
	bool foundFirstCue = false;
	while (file->ReadString(buff)) {
		FastTrimRight(buff);
		if (!foundFirstCue && !buff.IsEmpty()) { //STYLE blocks cannot show up after cues begin
			if (buff == L"STYLE" || buff == L"Style:" /*have seen webvtt with incorrect format using 'Style:' instead of 'STYLE'*/) {
				parseStyle(buff); //note that buff will contain next line when done, so we can still use it below
			}
		}
		if (buff.IsEmpty()) {
			continue;
		}

		int len = buff.GetLength();
		cueTags = L"";
		int c = swscanf_s(buff, L"%s --> %s %[^\n]s", start.GetBuffer(len), len, end.GetBuffer(len), len, cueTags.GetBuffer(len), len);
		start.ReleaseBuffer();
		end.ReleaseBuffer();
		cueTags.ReleaseBuffer();

		int hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2;

		if ((c == 2 || c == 3) //either start/end or start/end/cuetags
			&& readTimeCode(start, hh1, mm1, ss1, ms1)
			&& readTimeCode(end, hh2, mm2, ss2, ms2)) {
			foundFirstCue = true;

			CStringW str, tmp;

			while (file->ReadString(tmp)) {
				FastTrimRight(tmp);
				if (tmp.IsEmpty()) {
					break;
				}
				WebVTT2SSA(tmp, cueTags, cueColors);
				str += tmp + '\n';
			}

			if (lastStr != str || lastBuff != buff) { //discard repeated subs
				ret.Add(str,
					(((hh1 * 60i64 + mm1) * 60i64) + ss1) * 1000i64 + ms1,
					(((hh2 * 60i64 + mm2) * 60i64) + ss2) * 1000i64 + ms2);
			}

			lastStr = str;
			lastBuff = buff;
		}
		else {
			continue;
		}
	}

	// in case of embedded data, we initially might only get the header, so always return true
	return true;
}

static bool OpenSubRipper(CTextFile* file, CSimpleTextSubtitle& ret)
{
	CStringW buff;
	bool first_line_success = false;
	while (file->ReadString(buff)) {
		FastTrimRight(buff);
		if (buff.IsEmpty()) {
			continue;
		}

		WCHAR sep;
		int num = 0; // This one isn't really used just assigned a new value
		int hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2;
		WCHAR msStr1[5] = {0}, msStr2[5] = {0};
		int c = swscanf_s(buff, L"%d%c%d%c%d%4[^-] --> %d%c%d%c%d%4s\n",
						  &hh1, &sep, 1, &mm1, &sep, 1, &ss1, msStr1, (unsigned)std::size(msStr1),
						  &hh2, &sep, 1, &mm2, &sep, 1, &ss2, msStr2, (unsigned)std::size(msStr2));

		if (c == 1) { // numbering
			num = hh1;
		} else if (c >= 11) { // time info
			// Parse ms if present
			if (2 != swscanf_s(msStr1, L"%c%d", &sep, 1, &ms1)) {
				ms1 = 0;
			}
			if (2 != swscanf_s(msStr2, L"%c%d", &sep, 1, &ms2)) {
				ms2 = 0;
			}

			CStringW str, tmp;

			bool fFoundEmpty = false;

			while (file->ReadString(tmp)) {
				FastTrimRight(tmp);
				if (tmp.IsEmpty()) {
					fFoundEmpty = true;
				}

				int num2;
				WCHAR wc;
				if (swscanf_s(tmp, L"%d%c", &num2, &wc, 1) == 1 && fFoundEmpty) {
					num = num2;
					break;
				}

				str += tmp + L'\n';
			}

			ret.Add(
				SubRipper2SSA(str),
				(((hh1*60 + mm1)*60) + ss1)*1000 + ms1,
				(((hh2*60 + mm2)*60) + ss2)*1000 + ms2);
			first_line_success = true;
		} else if (c != EOF) { // might be another format
			if (first_line_success) // may be just a syntax error, try next lines...
				continue;

			return false;
		}
	}

	return !ret.IsEmpty();
}

static bool OpenOldSubRipper(CTextFile* file, CSimpleTextSubtitle& ret)
{
	CStringW buff;
	while (file->ReadString(buff)) {
		FastTrim(buff);
		if (buff.IsEmpty()) {
			continue;
		}

		for (int i = 0; i < buff.GetLength(); i++) {
			if ((i = buff.Find(L'|', i)) < 0) {
				break;
			}
			buff.SetAt(i, L'\n');
		}

		int hh1, mm1, ss1, hh2, mm2, ss2;
		int c = swscanf_s(buff, L"{%d:%d:%d}{%d:%d:%d}", &hh1, &mm1, &ss1, &hh2, &mm2, &ss2);

		if (c == 6) {
			ret.Add(
				buff.Mid(buff.Find(L'}', buff.Find(L'}')+1)+1),
				(((hh1*60 + mm1)*60) + ss1)*1000,
				(((hh2*60 + mm2)*60) + ss2)*1000);
		} else if (c != EOF) { // might be another format
			return false;
		}
	}

	return !ret.IsEmpty();
}

static bool OpenLRC(CTextFile* file, CSimpleTextSubtitle& ret)
{
	CString buff;

	struct lrc {
		CString text;
		int start, end;
	};
	std::vector<lrc> lrc_lines;

	for(unsigned line = 0; file->ReadString(buff); ++line) {
		if (line == 10 && lrc_lines.empty()) {
			break; // this is not a lrc file, stop analysis
		}

		FastTrim(buff);
		if (buff.IsEmpty()) {
			continue;
		}

		const std::wregex regex(L"\\[(\\d{2}):(\\d{2})(?:\\.(\\d{2}))?\\]");
		std::wcmatch match;

		std::vector<int> times;
		LPCWSTR text = buff.GetBuffer();
		while (std::regex_search(text, match, regex)) {
			if (match.size() == 4) {
				int mm = match[1].matched ? _wtoi(match[1].first) : 0;
				int ss = match[2].matched ? _wtoi(match[2].first) : 0;
				int ms = match[3].matched ? _wtoi(match[3].first) : 0;

				times.push_back(((mm * 60) + ss) * 1000 + ms);
			}

			text = match[0].second;
		}

		if (times.empty()) {
			continue;
		}

		CString txt(text);
		FastTrim(txt);

		for (const auto& time : times) {
			lrc_lines.emplace_back(lrc{ txt, time, time });
		}
	}

	if (!lrc_lines.empty()) {
		std::sort(lrc_lines.begin(), lrc_lines.end(), [](const lrc& a, const lrc& b) {
			return a.start < b.start;
		});

		for (size_t i = 0; i < lrc_lines.size() - 1; i++) {
			lrc_lines[i].end = lrc_lines[i + 1].start;
		}
		lrc_lines.back().end = lrc_lines.back().start + 10000; // 10 seconds
	}

	for (const auto& line : lrc_lines) {
		if (line.text.GetLength() && line.start < line.end) {
			ret.Add(line.text, line.start, line.end);
		}
	}

	return !ret.IsEmpty();
}

static CString TTML2SSA(CString str)
{
	str = SubRipper2SSA(str);
	str.Replace(L"&nbsp;", L" ");
	str.Replace(L"&quot;", L"\"");
	str.Replace(L"</span>", L"");

	static LPCWSTR tags2remove[] = {
		L"<span"
	};

	for (const auto& tag : tags2remove) {
		for (;;) {
			const auto pos = str.Find(tag);
			if (pos >= 0) {
				const auto end = str.Find(L'>', pos);
				if (end > pos) {
					str.Delete(pos, end - pos + 1);
				} else {
					break;
				}
			} else {
				break;
			}
		}
	}

	str = std::regex_replace(str.GetString(), std::wregex(LR"((<br[ ]*?\/>))"), L"\n\r").c_str();

	return str;
}

static bool OpenTTML(CTextFile* file, CSimpleTextSubtitle& ret)
{
	CString buff;

	if (!file->ReadString(buff)) {
		return false;
	}
	FastTrim(buff);
	if (buff.Find(L"<?xml") != 0 && buff.Find(L"<tt ") != 0) {
		return false;
	}

	if (buff.Find(L"<tt ") == -1) {
		for (unsigned line = 0; line < 10 && file->ReadString(buff); ++line) {
			FastTrim(buff);
			if (buff.IsEmpty()) {
				continue;
			}

			if (buff.Find(L"<tt ") == -1) {
				continue;
			}

			break;
		}
	}

	if (buff.Find(L"<tt ") != -1) {
		CString tmp(buff);

		while (file->ReadString(buff)) {
			FastTrim(buff);
			if (buff.IsEmpty()) {
				continue;
			}

			tmp.Append(buff);
		}

		if (tmp.Find(L"</tt>") != -1) {
			const auto body = RegExpParse(tmp.GetString(), LR"(<body[^>]*?>(.*)</body>)");
			if (!body.IsEmpty()) {
				const std::wregex regex(LR"(<p(.*?)</p>)");

				std::wcmatch match;
				auto body_text = body.GetString();
				while (std::regex_search(body_text, match, regex)) {
					if (match.size() == 2) {
						CString line(match[1].first, match[1].length());

						auto ParseTime = [](LPCWSTR buffer, LPCWSTR regexp) {
							int time = -1;

							const std::wregex regex(regexp);
							std::wcmatch match;
							if (std::regex_search(buffer, match, regex)) {
								if (match.size() == 5) {
									int hh = match[1].matched ? _wtoi(match[1].first) : 0;
									int mm = match[2].matched ? _wtoi(match[2].first) : 0;
									int ss = match[3].matched ? _wtoi(match[3].first) : 0;
									int ms = match[3].matched ? _wtoi(match[4].first) : 0;

									time = (((hh * 60 + mm) * 60) + ss) * 1000 + ms;
								} else if (match.size() == 3) {
									int ss = match[1].matched ? _wtoi(match[1].first) : 0;
									int ms = match[2].matched ? _wtoi(match[2].first) : 0;

									time = ss * 1000 + ms;
								} else if (match.size() == 2) {
									time = match[1].matched ? _wtoi(match[1].first) / 10000 : -1;
								}
							}

							return time;
						};

						int time_end = -1;
						int time_begin = ParseTime(line.GetString(), LR"(begin=\"(\d{2}):(\d{2}):(\d{2}).(\d{2,3})\")");
						if (time_begin != -1) {
							time_end = ParseTime(line.GetString(), LR"(end=\"(\d{2}):(\d{2}):(\d{2}).(\d{2,3})\")");
							if (time_end == -1) {
								int time_duration = ParseTime(line.GetString(), LR"(dur=\"(\d{2}):(\d{2}):(\d{2}).(\d{2,3})\")");
								if (time_duration != -1) {
									time_end = time_begin + time_duration;
								}
							}
						}
						if (time_begin == -1) {
							time_begin = ParseTime(line.GetString(), LR"(begin=\"(\d+)\.?(\d{1,3})?s\")");
							if (time_begin != -1) {
								time_end = ParseTime(line.GetString(), LR"(end=\"(\d+)\.?(\d{1,3})?s\")");
								if (time_end == -1) {
									int time_duration = ParseTime(line.GetString(), LR"(dur=\"(\d+)\.?(\d{1,3})?s\")");
									if (time_duration != -1) {
										time_end = time_begin + time_duration;
									}
								}
							}
						}
						if (time_begin == -1) {
							time_begin = ParseTime(line.GetString(), LR"(begin=\"(\d+)t\")");
							if (time_begin != -1) {
								time_end = ParseTime(line.GetString(), LR"(end=\"(\d+)t\")");
								if (time_end == -1) {
									int time_duration = ParseTime(line.GetString(), LR"(dur=\"(\d+)t\")");
									if (time_duration != -1) {
										time_end = time_begin + time_duration;
									}
								}
							}
						}
						if (time_begin != -1 && time_end > time_begin) {
							auto text = RegExpParse(line.GetString(), LR"(>(.+))");
							if (!text.IsEmpty()) {
								ret.Add(TTML2SSA(text), time_begin, time_end);
							}
						}
					}

					body_text = match[0].second;
				}
			}
		}
	}

	return !ret.IsEmpty();
}

static bool OpenSubViewer(CTextFile* file, CSimpleTextSubtitle& ret)
{
	STSStyle def;
	CStringW font, color, size;
	bool fBold = false;
	bool fItalic = false;
	bool fStriked = false;
	bool fUnderline = false;

	CStringW buff;
	while (file->ReadString(buff)) {
		FastTrim(buff);
		if (buff.IsEmpty()) {
			continue;
		}

		if (buff[0] == L'[') {
			for (int i = 0; i < buff.GetLength() && buff[i]== L'['; ) {
				int j = buff.Find(L']', ++i);
				if (j < i) {
					break;
				}

				CStringW tag = buff.Mid(i,j-i);
				FastTrim(tag);
				tag.MakeLower();

				i += j-i;

				j = buff.Find(L'[', ++i);
				if (j < 0) {
					j = buff.GetLength();
				}

				CStringW param = buff.Mid(i,j-i);
				param.Trim(L" \\t,");

				i = j;

				if (tag == L"font") {
					def.fontName.CompareNoCase(param) ? font.SetString(param) : font.Empty();
				} else if (tag == L"colf") {
					def.colors[0] != (DWORD)wcstol(((LPCWSTR)param)+2, 0, 16) ? color.SetString(param) : color.Empty();
				} else if (tag == L"size") {
					def.fontSize != (double)wcstol(param, 0, 10) ? size.SetString(param) : size.Empty();
				} else if (tag == L"style") {
					if (param.Find(L"no") >= 0) {
						fBold = fItalic = fStriked = fUnderline = false;
					} else {
						fBold = def.fontWeight < FW_BOLD && param.Find(L"bd") >= 0;
						fItalic = def.fItalic && param.Find(L"it") >= 0;
						fStriked = def.fStrikeOut && param.Find(L"st") >= 0;
						fUnderline = def.fUnderline && param.Find(L"ud") >= 0;
					}
				}
			}

			continue;
		}

		WCHAR sep;
		int hh1, mm1, ss1, hs1, hh2, mm2, ss2, hs2;
		int c = swscanf_s(buff, L"%d:%d:%d%c%d,%d:%d:%d%c%d\n",
			&hh1, &mm1, &ss1, &sep, 1, &hs1,
			&hh2, &mm2, &ss2, &sep, 1, &hs2);

		if (c == 10) {
			CStringW str;
			file->ReadString(str);

			str.Replace(L"[br]", L"\\N");

			CStringW prefix;
			if (!font.IsEmpty()) {
				prefix += L"\\fn" + font;
			}
			if (!color.IsEmpty()) {
				prefix += L"\\c" + color;
			}
			if (!size.IsEmpty()) {
				prefix += L"\\fs" + size;
			}
			if (fBold) {
				prefix += L"\\b1";
			}
			if (fItalic) {
				prefix += L"\\i1";
			}
			if (fStriked) {
				prefix += L"\\s1";
			}
			if (fUnderline) {
				prefix += L"\\u1";
			}
			if (!prefix.IsEmpty()) {
				str = L"{" + prefix + L"}" + str;
			}

			ret.Add(str,
					(((hh1*60 + mm1)*60) + ss1)*1000 + hs1*10,
					(((hh2*60 + mm2)*60) + ss2)*1000 + hs2*10);
		} else if (c != EOF) { // might be another format
			return false;
		}
	}

	return !ret.IsEmpty();
}

static STSStyle* GetMicroDVDStyle(CString str)
{
	STSStyle* ret = DNew STSStyle();
	if (!ret) {
		return nullptr;
	}

	for (int i = 0, len = str.GetLength(); i < len; i++) {
		int j = str.Find(L'{', i);
		if (j < 0) {
			j = len;
		}

		if (j >= len) {
			break;
		}

		int k = str.Find(L'}', j);
		if (k < 0) {
			k = len;
		}

		CString code = str.Mid(j, k-j);
		if (code.GetLength() > 2) {
			code.SetAt(1, (WCHAR)towlower(code[1]));
		}

		if (!_wcsnicmp(code, L"{c:$", 4)) {
			swscanf_s(code, L"{c:$%lx", &ret->colors[0]);
		} else if (!_wcsnicmp(code, L"{f:", 3)) {
			ret->fontName = code.Mid(3);
		} else if (!_wcsnicmp(code, L"{s:", 3)) {
			double f;
			if (1 == swscanf_s(code, L"{s:%lf", &f)) {
				ret->fontSize = f;
			}
		} else if (!_wcsnicmp(code, L"{h:", 3)) {
			swscanf_s(code, L"{h:%d", &ret->charSet);
		} else if (!_wcsnicmp(code, L"{y:", 3)) {
			code.MakeLower();
			if (code.Find(L'b') >= 0) {
				ret->fontWeight = FW_BOLD;
			}
			if (code.Find(L'i') >= 0) {
				ret->fItalic = true;
			}
			if (code.Find(L'u') >= 0) {
				ret->fUnderline = true;
			}
			if (code.Find(L's') >= 0) {
				ret->fStrikeOut = true;
			}
		} else if (!_wcsnicmp(code, L"{p:", 3)) {
			int p;
			swscanf_s(code, L"{p:%d", &p);
			ret->scrAlignment = (p == 0) ? 8 : 2;
		}

		i = k;
	}

	return ret;
}

static CStringW MicroDVD2SSA(CStringW str)
{
	CStringW ret;

	enum {
		COLOR = 0,
		FONTNAME,
		FONTSIZE,
		FONWCHARSET,
		BOLD,
		ITALIC,
		UNDERLINE,
		STRIKEOUT
	};

	bool fRestore[8];
	int fRestoreLen = 8;
	memset(fRestore, 0, sizeof(bool)*fRestoreLen);

	for (int pos = 0, eol; pos < str.GetLength(); pos++) {
		if ((eol = str.Find(L'|', pos)) < 0) {
			eol = str.GetLength();
		}

		CStringW line = str.Mid(pos, eol-pos);

		pos = eol;

		for (int i = 0, j, k, len = line.GetLength(); i < len; i++) {
			if ((j = line.Find(L'{', i)) < 0) {
				j = str.GetLength();
			}

			ret += line.Mid(i, j-i);

			if (j >= len) {
				break;
			}

			if ((k = line.Find(L'}', j)) < 0) {
				k = len;
			}

			{
				CStringW code = line.Mid(j, k-j);

				if (!_wcsnicmp(code, L"{c:$", 4)) {
					fRestore[COLOR] = (iswupper(code[1]) == 0);
					code.MakeLower();

					int color;
					swscanf_s(code, L"{c:$%x", &color);
					code.Format(L"{\\c&H%x&}", color);
					ret += code;
				} else if (!_wcsnicmp(code, L"{f:", 3)) {
					fRestore[FONTNAME] = (iswupper(code[1]) == 0);

					code.Format(L"{\\fn%s}", code.Mid(3));
					ret += code;
				} else if (!_wcsnicmp(code, L"{s:", 3)) {
					fRestore[FONTSIZE] = (iswupper(code[1]) == 0);
					code.MakeLower();

					double size;
					swscanf_s(code, L"{s:%lf", &size);
					code.Format(L"{\\fs%f}", size);
					ret += code;
				} else if (!_wcsnicmp(code, L"{h:", 3)) {
					fRestore[COLOR] = (_istupper(code[1]) == 0);
					code.MakeLower();

					int iCharSet;
					swscanf_s(code, L"{h:%d", &iCharSet);
					code.Format(L"{\\fe%d}", iCharSet);
					ret += code;
				} else if (!_wcsnicmp(code, L"{y:", 3)) {
					bool f = (_istupper(code[1]) == 0);

					code.MakeLower();

					ret += L'{';
					if (code.Find(L'b') >= 0) {
						ret += L"\\b1";
						fRestore[BOLD] = f;
					}
					if (code.Find(L'i') >= 0) {
						ret += L"\\i1";
						fRestore[ITALIC] = f;
					}
					if (code.Find(L'u') >= 0) {
						ret += L"\\u1";
						fRestore[UNDERLINE] = f;
					}
					if (code.Find(L's') >= 0) {
						ret += L"\\s1";
						fRestore[STRIKEOUT] = f;
					}
					ret += L'}';
				} else if (!_wcsnicmp(code, L"{o:", 3)) {
					code.MakeLower();

					int x, y;
					WCHAR c;
					swscanf_s(code, L"{o:%d%c%d", &x, &c, 1, &y);
					code.Format(L"{\\move(%d,%d,0,0,0,0)}", x, y);
					ret += code;
				} else {
					ret += code;
				}
			}

			i = k;
		}

		if (pos >= str.GetLength()) {
			break;
		}

		for (ptrdiff_t i = 0; i < fRestoreLen; i++) {
			if (fRestore[i]) {
				switch (i) {
					case COLOR:
						ret += L"{\\c}";
						break;
					case FONTNAME:
						ret += L"{\\fn}";
						break;
					case FONTSIZE:
						ret += L"{\\fs}";
						break;
					case FONWCHARSET:
						ret += L"{\\fe}";
						break;
					case BOLD:
						ret += L"{\\b}";
						break;
					case ITALIC:
						ret += L"{\\i}";
						break;
					case UNDERLINE:
						ret += L"{\\u}";
						break;
					case STRIKEOUT:
						ret += L"{\\s}";
						break;
					default:
						ASSERT(FALSE); // Shouldn't happen
						break;
				}
			}
		}

		memset(fRestore, 0, sizeof(bool) * fRestoreLen);

		ret += L"\\N";
	}

	return ret;
}

static bool OpenMicroDVD(CTextFile* file, CSimpleTextSubtitle& ret)
{
	bool fCheck = false, fCheck2 = false;

	CString style(L"Default");

	CStringW buff;
	while (file->ReadString(buff)) {
		FastTrim(buff);
		if (buff.IsEmpty()) {
			continue;
		}

		int start, end;
		int c = swscanf_s(buff, L"{%d}{%d}", &start, &end);

		if (c != 2) {
			c = swscanf_s(buff, L"{%d}{}", &start) + 1;
			end = start + 60;
			fCheck = true;
		}

		if (c != 2) {
			int i;
			if (buff.Find(L'{') == 0 && (i = buff.Find(L'}')) > 1 && i < buff.GetLength()) {
				if (STSStyle* s = GetMicroDVDStyle(buff.Mid(i+1))) {
					style = buff.Mid(1, i-1);
					if (style.GetLength()) {
						style = style.Left(1).MakeUpper() + style.Mid(1).MakeLower();
					}
					ret.AddStyle(style, s);
					continue;
				}
			}
		}

		if (c == 2) {
			if (fCheck2 && ret.GetCount()) {
				STSEntry& stse = ret[ret.GetCount()-1];
				stse.end = std::min(stse.end, start);
				fCheck2 = false;
			}

			ret.Add(
				MicroDVD2SSA(buff.Mid(buff.Find(L'}', buff.Find(L'}')+1)+1)),
				start, end,
				style);

			if (fCheck) {
				fCheck = false;
				fCheck2 = true;
			}
		} else if (c != EOF) { // might be another format
			return false;
		}
	}

	return !ret.IsEmpty();
}

static void ReplaceNoCase(CStringW& str, CStringW from, CStringW to)
{
	CStringW lstr = str;
	lstr.MakeLower();

	int i, j, k;

	for (i = 0, j = str.GetLength(); i < j; ) {
		if ((k = lstr.Find(from, i)) >= 0) {
			str.Delete(k, from.GetLength());
			lstr.Delete(k, from.GetLength());
			str.Insert(k, to);
			lstr.Insert(k, to);
			i = k + to.GetLength();
			j = str.GetLength();
		} else {
			break;
		}
	}
}

static int FindNoCase(LPCWSTR pszString, LPCWSTR pszSearch)
{
	int lenString = wcslen(pszString);
	int lenSearch = wcslen(pszSearch);
	if (lenSearch == 0 || lenSearch > lenString) {
		return -1;
	}
	for (int i = 0; i < lenString - lenSearch + 1; ++i) {
		if (!_wcsnicmp(&pszString[i], pszSearch, lenSearch )) {
			return i;
		}
	}

	return -1;
}

static CStringW SMI2SSA(CStringW str)
{
	ReplaceNoCase(str, L"&nbsp;", L" ");
	ReplaceNoCase(str, L"&quot;", L"\"");
	ReplaceNoCase(str, L"<br>", L"\\N");
	ReplaceNoCase(str, L"<i>", L"{\\i1}");
	ReplaceNoCase(str, L"</i>", L"{\\i}");
	ReplaceNoCase(str, L"<b>", L"{\\b1}");
	ReplaceNoCase(str, L"</b>", L"{\\b}");
	ReplaceNoCase(str, L"<u>", L"{\\u1}");
	ReplaceNoCase(str, L"</u>", L"{\\u}");

	CStringW lstr = str;
	lstr.MakeLower();

	// maven@maven.de
	// now parse line
	for (int i = 0, j = str.GetLength(); i < j; ) {
		int k;
		if ((k = lstr.Find(L'<', i)) < 0) {
			break;
		}

		int chars_inserted = 0;

		int l = 1;
		for (; k+l < j && lstr[k+l] != L'>'; l++) {
			;
		}
		l++;

		// Modified by Cookie Monster
		if (lstr.Find(L"<font ", k) == k) {
			CStringW args = lstr.Mid(k+6, l-6);	// delete "<font "
			CStringW arg ;

			args.Remove(L'\"');
			args.Remove(L'#');	// may include 2 * " + #
			arg.TrimLeft();
			arg.TrimRight(L" >");

			for (;;) {
				args.TrimLeft();
				arg = args.SpanExcluding(L" \t>");
				args = args.Mid(arg.GetLength());

				if (arg.IsEmpty()) {
					break;
				}
				if (arg.Find(L"color=") == 0 ) {
					DWORD color;

					arg = arg.Mid(6);	// delete "color="
					if ( arg.IsEmpty()) {
						continue;
					}

					DWORD val;
					if (g_colors.Lookup(CString(arg), val)) {
						color = (DWORD)val;
					} else if ((color = wcstol(arg, NULL, 16) ) == 0) {
						color = 0x00ffffff;	// default is white
					}

					arg.Format(L"%02x%02x%02x", color&0xff, (color>>8)&0xff, (color>>16)&0xff);
					lstr.Insert(k + l + chars_inserted, CStringW(L"{\\c&H") + arg + L"&}");
					str.Insert(k + l + chars_inserted, CStringW(L"{\\c&H") + arg + L"&}");
					chars_inserted += 5 + arg.GetLength() + 2;
				}
			}
		}

		else if (lstr.Find(L"</font>", k) == k) {
			lstr.Insert(k + l + chars_inserted, L"{\\c}");
			str.Insert(k + l + chars_inserted, L"{\\c}");
			chars_inserted += 4;
		}

		str.Delete(k, l);
		lstr.Delete(k, l);
		i = k + chars_inserted;
		j = str.GetLength();
	}

	return str;
}

static bool OpenSami(CTextFile* file, CSimpleTextSubtitle& ret)
{
	CStringW buff, samiBuff;

	ULONGLONG pos = file->GetPosition();

	bool fSAMI = false;
	while (file->ReadString(buff) && !fSAMI) {
		if (buff.MakeUpper().Find(L"<SAMI>") >= 0) {
			fSAMI = true;
		}
	}

	if (!fSAMI) {
		return false;
	}

	file->Seek(pos, CFile::begin);

	bool fComment = false;
	bool bEnd = false;

	while (file->ReadString(buff) && !bEnd) {
		FastTrim(buff);
		if (buff.IsEmpty()) {
			continue;
		}

		if (FindNoCase(buff, L"<!--") >= 0 || FindNoCase(buff, L"<TITLE>") >= 0) {
			fComment = true;
		}

		if (!fComment) {
			long end_time = MAXLONG;
			int syncpos = FindNoCase(buff, L"<SYNC START=");
			if (syncpos == -1) {
				syncpos = FindNoCase(buff, L"</BODY>");
				if (syncpos == -1) {
					syncpos = FindNoCase(buff, L"</SAMI>");
				}

				if (syncpos == 0) {
					bEnd = true;
				}
			} else {
				end_time = wcstol((LPCWSTR)buff + 12, NULL, 10);
			}

			if (syncpos == 0) {
				if (FindNoCase(samiBuff, L"<SYNC START=") == 0) {
					long start_time = wcstol((LPCWSTR)samiBuff + 12, NULL, 10);
					int endtime_pos = -1;
					if ((endtime_pos = FindNoCase(samiBuff, L"END=")) > 0) {
						end_time = wcstol((LPCWSTR)samiBuff + endtime_pos + 4, NULL, 10);
					}

					if (end_time > start_time) {
						ret.Add(
							SMI2SSA(samiBuff),
							start_time, end_time);
					}
				}

				samiBuff.Empty();
			}

			samiBuff += buff;
		}

		if (FindNoCase(buff, L"-->") >= 0 || FindNoCase(buff, L"</TITLE>") >= 0) {
			fComment = false;
		}
	}

	return true;
}

static bool OpenVPlayer(CTextFile* file, CSimpleTextSubtitle& ret)
{
	CStringW buff;
	while (file->ReadString(buff)) {
		FastTrim(buff);
		if (buff.IsEmpty()) {
			continue;
		}

		for (int i = 0; i < buff.GetLength(); i++) {
			if ((i = buff.Find(L'|', i)) < 0) {
				break;
			}
			buff.SetAt(i, L'\n');
		}

		int hh, mm, ss;
		int c = swscanf_s(buff, L"%d:%d:%d:", &hh, &mm, &ss);

		if (c == 3) {
			CStringW str = buff.Mid(buff.Find(':', buff.Find(':', buff.Find(':')+1)+1)+1);
			ret.Add(str,
					(((hh*60 + mm)*60) + ss)*1000,
					(((hh*60 + mm)*60) + ss)*1000 + 1000 + 50*str.GetLength());
		} else if (c != EOF) { // might be another format
			return false;
		}
	}

	return !ret.IsEmpty();
}

static void GetStrW(LPCWSTR& pszBuff, int& nLength, WCHAR sep, LPCWSTR& pszMatch, int& nMatchLength)
{
	// Trim left whitespace
	while (CStringW::StrTraits::IsSpace(*pszBuff)) {
		pszBuff++;
		nLength--;
	}

	LPCWSTR pEnd = CStringW::StrTraits::StringFindChar(pszBuff, sep);
	if (pEnd == NULL) {
		if (nLength < 1) {
			throw 1;
		}
		nMatchLength = nLength;
	} else {
		nMatchLength = int(pEnd - pszBuff);
	}

	pszMatch = pszBuff;
	if (nMatchLength < nLength) {
		pszBuff = pEnd + 1;
		nLength -= nMatchLength + 1;
	}
}

static CStringW GetStrW(LPCWSTR& pszBuff, int& nLength, WCHAR sep = L',')
{
	LPCWSTR pszMatch;
	int nMatchLength;
	GetStrW(pszBuff, nLength, sep, pszMatch, nMatchLength);

	return CStringW(pszMatch, nMatchLength);
}

static int GetInt(LPCWSTR& pszBuff, int& nLength, WCHAR sep = L',')
{
	LPCWSTR pszMatch;
	int nMatchLength;
	GetStrW(pszBuff, nLength, sep, pszMatch, nMatchLength);

	LPWSTR strEnd;
	int ret;

	if (nMatchLength > 2
			&& ((pszMatch[0] == L'&' && towlower(pszMatch[1]) == L'h')
				|| (pszMatch[0] == L'0' && towlower(pszMatch[1]) == L'x'))) {
		pszMatch += 2;
		nMatchLength -= 2;
		ret = (int)wcstoul(pszMatch, &strEnd, 16);
	} else {
		ret = wcstol(pszMatch, &strEnd, 10);
	}

	if (pszMatch == strEnd) { // Ensure something was parsed
		throw 1;
	}

	return ret;
}

static double GetFloat(LPCWSTR& pszBuff, int& nLength, WCHAR sep = L',')
{
	if (sep == L'.') { // Parsing a float with '.' as separator doesn't make much sense...
		ASSERT(FALSE);
		return GetInt(pszBuff, nLength, sep);
	}

	LPCWSTR pszMatch;
	int nMatchLength;
	GetStrW(pszBuff, nLength, sep, pszMatch, nMatchLength);

	LPWSTR strEnd;
	double ret = wcstod(pszMatch, &strEnd);
	if (pszMatch == strEnd) { // Ensure something was parsed
		throw 1;
	}

	return ret;
}

static bool LoadFont(const CString& font)
{
	int len = font.GetLength();

	if (len == 0 || (len & 3) == 1) {
		return false;
	}

	auto pData = std::make_unique<BYTE[]>(len);

	const WCHAR* s = font;
	const WCHAR* e = s + len;
	for (BYTE* p = pData.get(); s < e; s++, p++) {
		*p = *s - 33;
	}

	for (ptrdiff_t i = 0, j = 0, k = len & ~3; i < k; i += 4, j += 3) {
		pData[j + 0] = ((pData[i + 0] & 63) << 2) | ((pData[i + 1] >> 4) & 3);
		pData[j + 1] = ((pData[i + 1] & 15) << 4) | ((pData[i + 2] >> 2) & 15);
		pData[j + 2] = ((pData[i + 2] &  3) << 6) | ((pData[i + 3] >> 0) & 63);
	}

	int datalen = (len & ~3) * 3 / 4;

	if ((len & 3) == 2) {
		pData[datalen++] = ((pData[(len & ~3) + 0] & 63) << 2) | ((pData[(len & ~3) + 1] >> 4) & 3);
	} else if ((len & 3) == 3) {
		pData[datalen++] = ((pData[(len & ~3) + 0] & 63) << 2) | ((pData[(len & ~3) + 1] >> 4) & 3);
		pData[datalen++] = ((pData[(len & ~3) + 1] & 15) << 4) | ((pData[(len & ~3) + 2] >> 2) & 15);
	}

	HANDLE hFont = INVALID_HANDLE_VALUE;
	DWORD cFonts;
	hFont = AddFontMemResourceEx(pData.get(), datalen, NULL, &cFonts);

	if (hFont == INVALID_HANDLE_VALUE) {
		WCHAR path[MAX_PATH] = { 0 };
		GetTempPathW(MAX_PATH, path);

		DWORD chksum = 0;
		for (ptrdiff_t i = 0, j = datalen>>2; i < j; i++) {
			chksum += ((DWORD*)pData.get())[i];
		}

		CString fn;
		fn.Format(L"%sfont%08lx.ttf", path, chksum);

		if (!::PathFileExistsW(fn)) {
			CFile f;
			if (f.Open(fn, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary|CFile::shareDenyNone)) {
				f.Write(pData.get(), datalen);
				f.Close();
			}
		}

		return !!AddFontResourceW(fn);
	}

	return true;
}

static bool LoadUUEFont(CTextFile* file)
{
	CString s, font;
	int cnt = 0;
	while (file->ReadString(s)) {
		FastTrim(s);
		if (s.IsEmpty()) {
			break;
		}
		if (s[0] == L'[') { // check for some standatr blocks
			if (s.Find(L"[Script Info]") == 0) {
				break;
			}
			if (s.Find(L"[V4+ Styles]") == 0) {
				break;
			}
			if (s.Find(L"[V4 Styles]") == 0) {
				break;
			}
			if (s.Find(L"[Events]") == 0) {
				break;
			}
			if (s.Find(L"[Fonts]") == 0) {
				break;
			}
			if (s.Find(L"[Graphics]") == 0) {
				break;
			}
		}
		if (s.Find(L"fontname:") == 0) {
			cnt += LoadFont(font);
			font.Empty();
			continue;
		}

		font += s;
	}

	if (!font.IsEmpty()) {
		cnt += LoadFont(font);
	}

	return cnt ? true : false;
}

static bool OpenSubStationAlpha(CTextFile* file, CSimpleTextSubtitle& ret)
{
	bool bRet = false;

	bool script_info = false;
	bool events = false;
	bool styles = false;

	int version = 3, sver = 3;
	CStringW buff;

	while (file->ReadString(buff)) {
		FastTrim(buff);
		if (buff.IsEmpty() || buff.GetAt(0) == L';') {
			continue;
		}

		LPCWSTR pszBuff = buff;
		int nBuffLength = buff.GetLength();
		CStringW entry = GetStrW(pszBuff, nBuffLength, L':');
		entry.MakeLower();

		if (entry == L"dialogue") {
			if (events) {
				try {
					int hh1, mm1, ss1, ms1_div10, hh2, mm2, ss2, ms2_div10, layer = 0;
					CRect marginRect;

					if (version <= 4) {
						GetStrW(pszBuff, nBuffLength, L'=');		/* Marked = */
						GetInt(pszBuff, nBuffLength);
					}
					if (version >= 5) {
						layer = GetInt(pszBuff, nBuffLength);
					}
					hh1 = GetInt(pszBuff, nBuffLength, L':');
					mm1 = GetInt(pszBuff, nBuffLength, L':');
					ss1 = GetInt(pszBuff, nBuffLength, L'.');
					ms1_div10 = GetInt(pszBuff, nBuffLength);
					hh2 = GetInt(pszBuff, nBuffLength, L':');
					mm2 = GetInt(pszBuff, nBuffLength, L':');
					ss2 = GetInt(pszBuff, nBuffLength, L'.');
					ms2_div10 = GetInt(pszBuff, nBuffLength);
					CString Style = GetStrW(pszBuff, nBuffLength);
					CString Actor = GetStrW(pszBuff, nBuffLength);
					marginRect.left = GetInt(pszBuff, nBuffLength);
					marginRect.right = GetInt(pszBuff, nBuffLength);
					marginRect.top = marginRect.bottom = GetInt(pszBuff, nBuffLength);
					if (version >= 6) {
						marginRect.bottom = GetInt(pszBuff, nBuffLength);
					}

					CString Effect = GetStrW(pszBuff, nBuffLength);
					int len = std::min(Effect.GetLength(), nBuffLength);
					if (Effect.Left(len) == CString(pszBuff, len)) {
						Effect.Empty();
					}

					Style.TrimLeft(L'*');
					if (!Style.CompareNoCase(L"Default")) {
						Style = L"Default";
					}

					ret.Add(pszBuff,
							(((hh1*60 + mm1)*60) + ss1)*1000 + ms1_div10*10,
							(((hh2*60 + mm2)*60) + ss2)*1000 + ms2_div10*10,
							Style, Actor, Effect,
							marginRect,
							layer);
				} catch (...) {
					return false;
				}
			}
		} else if (entry == L"style") {
			if (styles) {
				STSStyle* style = DNew STSStyle;
				if (!style) {
					return false;
				}

				try {
					CString StyleName = GetStrW(pszBuff, nBuffLength);
					style->fontName = GetStrW(pszBuff, nBuffLength);
					style->fontSize = GetFloat(pszBuff, nBuffLength);
					for (size_t i = 0; i < 4; i++) {
						style->colors[i] = (COLORREF)GetInt(pszBuff, nBuffLength);
					}
					style->fontWeight = GetInt(pszBuff, nBuffLength) ? FW_BOLD : FW_NORMAL;
					style->fItalic = GetInt(pszBuff, nBuffLength);
					if (sver >= 5)	{
						style->fUnderline  = GetInt(pszBuff, nBuffLength);
						style->fStrikeOut  = GetInt(pszBuff, nBuffLength);
						style->fontScaleX  = GetFloat(pszBuff, nBuffLength);
						style->fontScaleY  = GetFloat(pszBuff, nBuffLength);
						style->fontSpacing = GetFloat(pszBuff, nBuffLength);
						style->fontAngleZ  = GetFloat(pszBuff, nBuffLength);
					}
					if (sver >= 4)	{
						style->borderStyle = GetInt(pszBuff, nBuffLength);
					}
					style->outlineWidthX = style->outlineWidthY = GetFloat(pszBuff, nBuffLength);
					style->shadowDepthX = style->shadowDepthY = GetFloat(pszBuff, nBuffLength);
					style->scrAlignment = GetInt(pszBuff, nBuffLength);
					style->marginRect.left = GetInt(pszBuff, nBuffLength);
					style->marginRect.right = GetInt(pszBuff, nBuffLength);
					style->marginRect.top = style->marginRect.bottom = GetInt(pszBuff, nBuffLength);
					if (sver >= 6)	{
						style->marginRect.bottom = GetInt(pszBuff, nBuffLength);
					}

					int alpha = 0;
					if (sver <= 4)	{
						alpha = GetInt(pszBuff, nBuffLength);
					}
					style->charSet = GetInt(pszBuff, nBuffLength);
					if (sver >= 6) {
						style->relativeTo = (STSStyle::RelativeTo)GetInt(pszBuff, nBuffLength);
					}

					if (sver <= 4)	{
						style->colors[2] = style->colors[3];	// style->colors[2] is used for drawing the outline
						alpha = std::clamp(alpha, 0, 0xff);
						for (size_t i = 0; i < 3; i++) {
							style->alpha[i] = alpha;
						}
						style->alpha[3] = 0x80;
					}
					if (sver >= 5)	{
						for (size_t i = 0; i < 4; i++) {
							style->alpha[i] = (BYTE)(style->colors[i] >> 24);
							style->colors[i] &= 0xffffff;
						}
						style->fontScaleX = std::max(style->fontScaleX, 0.0);
						style->fontScaleY = std::max(style->fontScaleY, 0.0);
					}
					style->fontAngleX = style->fontAngleY = 0;
					style->borderStyle = style->borderStyle == 1 ? 0 : style->borderStyle == 3 ? 1 : 0;
					style->outlineWidthX = std::max(style->outlineWidthX, 0.0);
					style->outlineWidthY = std::max(style->outlineWidthY, 0.0);
					style->shadowDepthX = std::max(style->shadowDepthX, 0.0);
					style->shadowDepthY = std::max(style->shadowDepthY, 0.0);
					if (sver <= 4)	{
						style->scrAlignment = (style->scrAlignment & 4) ? ((style->scrAlignment & 3) + 6) // top
											: (style->scrAlignment & 8) ? ((style->scrAlignment & 3) + 3) // mid
											: (style->scrAlignment & 3); // bottom
					}

					StyleName.TrimLeft(L'*');

					ret.AddStyle(StyleName, style);
				} catch (...) {
					UINT charSet = CodePageToCharSet(file->GetEncoding());
					ret.CreateDefaultStyle(charSet);
				}
			}
		} else if (entry == L"playresx") {
			if (script_info) {
				ret.m_dstScreenSizeActual = false;

				try {
					ret.m_dstScreenSize.cx = GetInt(pszBuff, nBuffLength);
				} catch (...) {
					ret.m_dstScreenSize = CSize(0, 0);
					return false;
				}

				if (ret.m_dstScreenSize.cy <= 0) {
					ret.m_dstScreenSize.cy = (ret.m_dstScreenSize.cx == 1280)
											 ? 1024
											 : ret.m_dstScreenSize.cx * 3 / 4;
				} else {
					ret.m_dstScreenSizeActual = true;
				}
			}
		} else if (entry == L"playresy") {
			if (script_info) {
				ret.m_dstScreenSizeActual = false;

				try {
					ret.m_dstScreenSize.cy = GetInt(pszBuff, nBuffLength);
				} catch (...) {
					ret.m_dstScreenSize = CSize(0, 0);
					return false;
				}

				if (ret.m_dstScreenSize.cx <= 0) {
					ret.m_dstScreenSize.cx = (ret.m_dstScreenSize.cy == 1024)
											 ? 1280
											 : ret.m_dstScreenSize.cy * 4 / 3;
				} else {
					ret.m_dstScreenSizeActual = true;
				}
			}
		} else if (entry == L"wrapstyle") {
			if (script_info) {
				try {
					ret.m_defaultWrapStyle = GetInt(pszBuff, nBuffLength);
				} catch (...) {
					ret.m_defaultWrapStyle = 1;
					return false;
				}
			}
		} else if (entry == L"scripttype") {
			if (script_info) {
				if (buff.GetLength() >= 4 && !buff.Right(4).CompareNoCase(L"4.00")) {
					version = sver = 4;
				} else if (buff.GetLength() >= 5 && !buff.Right(5).CompareNoCase(L"4.00+")) {
					version = sver = 5;
				} else if (buff.GetLength() >= 6 && !buff.Right(6).CompareNoCase(L"4.00++")) {
					version = sver = 6;
				}
			}
		} else if (entry == L"collisions") {
			if (script_info) {
				buff = GetStrW(pszBuff, nBuffLength);
				buff.MakeLower();
				ret.m_collisions = buff.Find(L"reverse") >= 0 ? 1 : 0;
			}
		} else if (entry == L"scaledborderandshadow") {
			if (script_info) {
				buff = GetStrW(pszBuff, nBuffLength);
				buff.MakeLower();
				ret.m_fScaledBAS = buff.Find(L"yes") >= 0;
			}
		} else if (entry == L"[script info]") {
			bRet = true;
			script_info = true;
		} else if (entry == L"[v4 styles]") {
			bRet = true;
			styles = true;
			sver = 4;
		} else if (entry == L"[v4+ styles]") {
			bRet = true;
			styles = true;
			sver = 5;
		} else if (entry == L"[v4++ styles]") {
			bRet = true;
			styles = true;
			sver = 6;
		} else if (entry == L"[events]") {
			bRet = true;
			events = true;
		} else if (entry == L"fontname") {
			if (LoadUUEFont(file)) {
				bRet = true;
			}
		}
	}

	return bRet;
}

static bool OpenXombieSub(CTextFile* file, CSimpleTextSubtitle& ret)
{
	//	CMapStringToPtr stylemap;

	CStringW buff;
	while (file->ReadString(buff)) {
		FastTrim(buff);
		if (buff.IsEmpty() || buff.GetAt(0) == L';') {
			continue;
		}

		LPCWSTR pszBuff = buff;
		int nBuffLength = buff.GetLength();
		CStringW entry = GetStrW(pszBuff, nBuffLength, L'=');
		entry.MakeLower();

		/*if (entry == L"version") {
			double version = GetFloat(pszBuff, nBuffLength);
		} else*/ if (entry == L"screenhorizontal") {
			try {
				ret.m_dstScreenSize.cx = GetInt(pszBuff, nBuffLength);
			} catch (...) {
				ret.m_dstScreenSize = CSize(0, 0);
				return false;
			}

			if (ret.m_dstScreenSize.cy <= 0) {
				ret.m_dstScreenSize.cy = (ret.m_dstScreenSize.cx == 1280)
										 ? 1024
										 : ret.m_dstScreenSize.cx * 3 / 4;
			}
		} else if (entry == L"screenvertical") {
			try {
				ret.m_dstScreenSize.cy = GetInt(pszBuff, nBuffLength);
			} catch (...) {
				ret.m_dstScreenSize = CSize(0, 0);
				return false;
			}

			if (ret.m_dstScreenSize.cx <= 0) {
				ret.m_dstScreenSize.cx = (ret.m_dstScreenSize.cy == 1024)
										 ? 1280
										 : ret.m_dstScreenSize.cy * 4 / 3;
			}
		} else if (entry == L"style") {
			STSStyle* style = DNew STSStyle;
			if (!style) {
				return false;
			}

			try {
				CString StyleName = GetStrW(pszBuff, nBuffLength) + L"_" + GetStrW(pszBuff, nBuffLength);
				style->fontName = GetStrW(pszBuff, nBuffLength);
				style->fontSize = GetFloat(pszBuff, nBuffLength);
				for (size_t i = 0; i < 4; i++) {
					style->colors[i] = (COLORREF)GetInt(pszBuff, nBuffLength);
				}
				for (size_t i = 0; i < 4; i++) {
					style->alpha[i] = GetInt(pszBuff, nBuffLength);
				}
				style->fontWeight = GetInt(pszBuff, nBuffLength) ? FW_BOLD : FW_NORMAL;
				style->fItalic = GetInt(pszBuff, nBuffLength);
				style->fUnderline = GetInt(pszBuff, nBuffLength);
				style->fStrikeOut = GetInt(pszBuff, nBuffLength);
				style->fBlur = GetInt(pszBuff, nBuffLength);
				style->fontScaleX = GetFloat(pszBuff, nBuffLength);
				style->fontScaleY = GetFloat(pszBuff, nBuffLength);
				style->fontSpacing = GetFloat(pszBuff, nBuffLength);
				style->fontAngleX = GetFloat(pszBuff, nBuffLength);
				style->fontAngleY = GetFloat(pszBuff, nBuffLength);
				style->fontAngleZ = GetFloat(pszBuff, nBuffLength);
				style->borderStyle = GetInt(pszBuff, nBuffLength);
				style->outlineWidthX = style->outlineWidthY = GetFloat(pszBuff, nBuffLength);
				style->shadowDepthX = style->shadowDepthY = GetFloat(pszBuff, nBuffLength);
				style->scrAlignment = GetInt(pszBuff, nBuffLength);
				style->marginRect.left = GetInt(pszBuff, nBuffLength);
				style->marginRect.right = GetInt(pszBuff, nBuffLength);
				style->marginRect.top = style->marginRect.bottom = GetInt(pszBuff, nBuffLength);
				style->charSet = GetInt(pszBuff, nBuffLength);

				style->fontScaleX = std::max(style->fontScaleX, 0.0);
				style->fontScaleY = std::max(style->fontScaleY, 0.0);
				style->fontSpacing = std::max(style->fontSpacing, 0.0);
				style->borderStyle = style->borderStyle == 1 ? 0 : style->borderStyle == 3 ? 1 : 0;
				style->outlineWidthX = std::max(style->outlineWidthX, 0.0);
				style->outlineWidthY = std::max(style->outlineWidthY, 0.0);
				style->shadowDepthX = std::max(style->shadowDepthX, 0.0);
				style->shadowDepthY = std::max(style->shadowDepthY, 0.0);

				ret.AddStyle(StyleName, style);
			} catch (...) {
				delete style;
				return false;
			}
		} else if (entry == L"line") {
			try {
				int hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2, layer = 0;
				CRect marginRect;

				if (GetStrW(pszBuff, nBuffLength) != L"D") {
					continue;
				}
				CString id = GetStrW(pszBuff, nBuffLength);
				layer = GetInt(pszBuff, nBuffLength);
				hh1 = GetInt(pszBuff, nBuffLength, L':');
				mm1 = GetInt(pszBuff, nBuffLength, L':');
				ss1 = GetInt(pszBuff, nBuffLength, L'.');
				ms1 = GetInt(pszBuff, nBuffLength);
				hh2 = GetInt(pszBuff, nBuffLength, L':');
				mm2 = GetInt(pszBuff, nBuffLength, L':');
				ss2 = GetInt(pszBuff, nBuffLength, L'.');
				ms2 = GetInt(pszBuff, nBuffLength);
				CString Style = GetStrW(pszBuff, nBuffLength) + L"_" + GetStrW(pszBuff, nBuffLength);
				CString Actor = GetStrW(pszBuff, nBuffLength);
				marginRect.left = GetInt(pszBuff, nBuffLength);
				marginRect.right = GetInt(pszBuff, nBuffLength);
				marginRect.top = marginRect.bottom = GetInt(pszBuff, nBuffLength);

				Style.TrimLeft('*');
				if (!Style.CompareNoCase(L"Default")) {
					Style = L"Default";
				}

				ret.Add(pszBuff,
						(((hh1*60 + mm1)*60) + ss1)*1000 + ms1,
						(((hh2*60 + mm2)*60) + ss2)*1000 + ms2,
						Style, Actor, L"",
						marginRect,
						layer);
			} catch (...) {
				return false;
			}
		} else if (entry == L"fontname") {
			LoadUUEFont(file);
		}
	}

	return !ret.IsEmpty();
}

static bool OpenUSF(CTextFile* file, CSimpleTextSubtitle& ret)
{
	CString str;
	while (file->ReadString(str)) {
		if (str.Find(L"USFSubtitles") >= 0) {
			CUSFSubtitles usf;
			if (usf.Read(file->GetFilePath()) && usf.ConvertToSTS(ret)) {
				return true;
			}

			break;
		}
	}

	return false;
}

static CStringW MPL22SSA(CStringW str)
{
	// Convert MPL2 italic tags to MicroDVD italic tags
	if (str[0] == L'/') {
		str = L"{y:i}" + str.Mid(1);
	}
	str.Replace(L"|/", L"|{y:i}");

	return MicroDVD2SSA(str);
}

static bool OpenMPL2(CTextFile* file, CSimpleTextSubtitle& ret)
{
	CStringW buff;
	while (file->ReadString(buff)) {
		FastTrim(buff);
		if (buff.IsEmpty()) {
			continue;
		}

		int start, end;
		int c = swscanf_s(buff, L"[%d][%d]", &start, &end);

		if (c == 2) {
			ret.Add(
				MPL22SSA(buff.Mid(buff.Find(L']', buff.Find(']')+1)+1)),
				start*100, end*100);
		} else if (c != EOF) { // might be another format
			return false;
		}
	}

	return !ret.IsEmpty();
}

typedef bool (*STSOpenFunct)(CTextFile* file, CSimpleTextSubtitle& ret);

static bool OpenRealText(CTextFile* file, CSimpleTextSubtitle& ret);

struct OpenFunctStruct {
	Subtitle::SubType type;
	tmode mode;
	STSOpenFunct open;
};

const static OpenFunctStruct s_OpenFuncts[] = {
	Subtitle::SSA,  TIME,  OpenSubStationAlpha,
	Subtitle::SRT,  TIME,  OpenSubRipper,
	Subtitle::SRT,  TIME,  OpenOldSubRipper,
	Subtitle::VTT,  TIME,  OpenVTT,
	Subtitle::LRC,  TIME,  OpenLRC,
	Subtitle::TTML, TIME,  OpenTTML,
	Subtitle::SUB,  TIME,  OpenSubViewer,
	Subtitle::SUB,  FRAME, OpenMicroDVD,
	Subtitle::SMI,  TIME,  OpenSami,
	Subtitle::SRT,  TIME,  OpenVPlayer,
	Subtitle::XSS,  TIME,  OpenXombieSub,
	Subtitle::USF,  TIME,  OpenUSF,
	Subtitle::SRT,  TIME,  OpenMPL2,
	Subtitle::RT,   TIME,  OpenRealText,
};

//

CSimpleTextSubtitle::CSimpleTextSubtitle()
	: m_mode(TIME)
	, m_dstScreenSizeActual(false)
	, m_defaultWrapStyle(0)
	, m_collisions(0)
	, m_fScaledBAS(false)
	, m_encoding(CP_ASCII)
	, m_lcid(0)
	, m_ePARCompensationType(EPCTDisabled)
	, m_dPARCompensation(1.0)
	, m_subtitleType(Subtitle::SRT)
	, m_fUsingAutoGeneratedDefaultStyle(false)
{
}

CSimpleTextSubtitle::~CSimpleTextSubtitle()
{
	Empty();
}
/*
CSimpleTextSubtitle::CSimpleTextSubtitle(CSimpleTextSubtitle& sts)
{
	*this = sts;
}

CSimpleTextSubtitle& CSimpleTextSubtitle::operator = (CSimpleTextSubtitle& sts)
{
	Copy(sts);

	return *this;
}
*/

void CSimpleTextSubtitle::Copy(CSimpleTextSubtitle& sts)
{
	if (this != &sts) {
		Empty();

		m_name = sts.m_name;
		m_mode = sts.m_mode;
		m_path = sts.m_path;
		m_subtitleType = sts.m_subtitleType;
		m_dstScreenSize = sts.m_dstScreenSize;
		m_defaultWrapStyle = sts.m_defaultWrapStyle;
		m_collisions = sts.m_collisions;
		m_fScaledBAS = sts.m_fScaledBAS;
		m_encoding = sts.m_encoding;
		m_fUsingAutoGeneratedDefaultStyle = sts.m_fUsingAutoGeneratedDefaultStyle;
		CopyStyles(sts.m_styles);
		m_segments.Copy(sts.m_segments);
		__super::Copy(sts);
	}
}

void CSimpleTextSubtitle::Append(CSimpleTextSubtitle& sts, int timeoff)
{
	if (timeoff < 0) {
		timeoff = !IsEmpty() ? GetAt(GetCount() - 1).end : 0;
	}

	for (size_t i = 0, j = GetCount(); i < j; i++) {
		if (GetAt(i).start > timeoff) {
			RemoveAt(i, j - i);
			break;
		}
	}

	CopyStyles(sts.m_styles, true);

	for (size_t i = 0, j = sts.GetCount(); i < j; i++) {
		STSEntry stse = sts.GetAt(i);
		stse.start += timeoff;
		stse.end += timeoff;
		stse.readorder += (int)GetCount();
		__super::Add(stse);
	}

	CreateSegments();
}

void CSTSStyleMap::Free()
{
	POSITION pos = GetStartPosition();
	while (pos) {
		CString key;
		STSStyle* val;
		GetNextAssoc(pos, key, val);
		delete val;
	}

	RemoveAll();
}

bool CSimpleTextSubtitle::CopyStyles(const CSTSStyleMap& styles, bool fAppend)
{
	if (!fAppend) {
		m_styles.Free();
	}

	POSITION pos = styles.GetStartPosition();
	while (pos) {
		CString key;
		STSStyle* val;
		styles.GetNextAssoc(pos, key, val);

		STSStyle* s = DNew STSStyle;
		if (!s) {
			return false;
		}

		*s = *val;

		AddStyle(key, s);
	}

	return true;
}

void CSimpleTextSubtitle::Empty()
{
	m_dstScreenSize = CSize(0, 0);
	m_styles.Free();
	m_segments.RemoveAll();
	RemoveAll();
}

static bool SegmentCompStart(const STSSegment& segment, int start)
{
	return (segment.start < start);
}

void CSimpleTextSubtitle::Add(CStringW str, int start, int end, CString style, CString actor, CString effect, const CRect& marginRect, int layer, int readorder)
{
	FastTrim(str);
	if (str.IsEmpty() || start > end) {
		return;
	}

	if (m_subtitleType == Subtitle::VTT) {
		CStringW cueTags = WebVTTCueStrip(str);
		WebVTTcolorMap clrMap;
		WebVTT2SSA(str, cueTags, clrMap);
		if (str.IsEmpty()) {
			return;
		}
	}

	str.Remove(L'\r');
	str.Replace(L"\n", L"\\N");
	if (style.IsEmpty()) {
		style = L"Default";
	}
	style.TrimLeft(L'*');

	STSEntry sub;
	sub.str = str;
	sub.style = style;
	sub.actor = actor;
	sub.effect = effect;
	sub.marginRect = marginRect;
	sub.layer = layer;
	sub.start = start;
	sub.end = end;
	sub.readorder = readorder < 0 ? (int)GetCount() : readorder;

	int n = (int)__super::Add(sub);

	// Entries with a null duration don't belong to any segments since
	// they are not to be rendered. We choose not to skip them completely
	// so that they are not lost when saving a subtitle file from MPC-BE
	// and so that one can change the timings of such entries using the
	// Subresync bar if necessary.
	if (start == end) {
		return;
	}

	size_t segmentsCount = m_segments.GetCount();

	if (segmentsCount == 0) { // First segment
		STSSegment stss(start, end);
		stss.subs.Add(n);
		m_segments.Add(stss);
	} else if (end <= m_segments[0].start) {
		STSSegment stss(start, end);
		stss.subs.Add(n);
		m_segments.InsertAt(0, stss);
	} else if (start >= m_segments[segmentsCount - 1].end) {
		STSSegment stss(start, end);
		stss.subs.Add(n);
		m_segments.Add(stss);
	} else {
		STSSegment* segmentsStart = m_segments.GetData();
		STSSegment* segmentsEnd   = segmentsStart + segmentsCount;
		STSSegment* segment = std::lower_bound(segmentsStart, segmentsEnd, start, SegmentCompStart);

		size_t i = segment - segmentsStart;
		if (i > 0 && m_segments[i - 1].end > start) {
			// The beginning of i-1th segment isn't modified
			// by the new entry so separate it in two segments
			STSSegment stss(m_segments[i - 1].start, start);
			stss.subs.Copy(m_segments[i - 1].subs);
			m_segments[i - 1].start = start;
			m_segments.InsertAt(i - 1, stss);
		} else if (i < segmentsCount && start < m_segments[i].start) {
			// The new entry doesn't start in an existing segment.
			// It might even not overlap with any segment at all
			STSSegment stss(start, std::min(end, m_segments[i].start));
			stss.subs.Add(n);
			m_segments.InsertAt(i, stss);
			i++;
		}

		int lastEnd = INT_MAX;
		for (; i < m_segments.GetCount() && m_segments[i].start < end; i++) {
			STSSegment& s = m_segments[i];

			if (lastEnd < s.start) {
				// There is a gap between the segments overlapped by
				// the new entry so we have to create a new one
				STSSegment stss(lastEnd, s.start);
				stss.subs.Add(n);
				lastEnd = s.start; // s might not point on the right segment after inserting so do the modification now
				m_segments.InsertAt(i, stss);
			} else {
				if (end < s.end) {
					// The end of current segment isn't modified
					// by the new entry so separate it in two segments
					STSSegment stss(end, s.end);
					stss.subs.Copy(s.subs);
					s.end = end; // s might not point on the right segment after inserting so do the modification now
					m_segments.InsertAt(i + 1, stss);
				}

				// The array might have been reallocated so create a new reference
				STSSegment& sAdd = m_segments[i];

				// Add the entry to the current segment now that we are you it belongs to it
				size_t entriesCount = sAdd.subs.GetCount();
				// Take a shortcut when possible
				if (!entriesCount || sub.readorder >= GetAt(sAdd.subs[entriesCount - 1]).readorder) {
					sAdd.subs.Add(n);
				} else {
					for (size_t j = 0; j < entriesCount; j++) {
						if (sub.readorder < GetAt(sAdd.subs[j]).readorder) {
							sAdd.subs.InsertAt(j, n);
							break;
						}
					}
				}

				lastEnd = sAdd.end;
			}
		}

		if (end > m_segments[i - 1].end) {
			// The new entry ends after the last overlapping segment.
			// It might even not overlap with any segment at all
			STSSegment stss(std::max(start, m_segments[i - 1].end), end);
			stss.subs.Add(n);
			m_segments.InsertAt(i, stss);
		}
	}
}

STSStyle* CSimpleTextSubtitle::CreateDefaultStyle(int CharSet)
{
	CString def(L"Default");

	STSStyle* ret = nullptr;

	if (!m_styles.Lookup(def, ret)) {
		STSStyle* style = DNew STSStyle();
		style->charSet = CharSet;
		AddStyle(def, style);
		m_styles.Lookup(def, ret);

		m_fUsingAutoGeneratedDefaultStyle = true;
	} else {
		m_fUsingAutoGeneratedDefaultStyle = false;
	}

	return ret;
}

void CSimpleTextSubtitle::ChangeUnknownStylesToDefault()
{
	CAtlMap<CString, STSStyle*, CStringElementTraits<CString> > unknown;
	bool fReport = true;

	for (size_t i = 0; i < GetCount(); i++) {
		STSEntry& stse = GetAt(i);

		STSStyle* val;
		if (!m_styles.Lookup(stse.style, val)) {
			if (!unknown.Lookup(stse.style, val)) {
				if (fReport) {
					CString msg;
					msg.Format(L"Unknown style found: \"%s\", changed to \"Default\"!\n\nPress Cancel to ignore further warnings.", stse.style);
					if (MessageBoxW(NULL, msg, L"Warning", MB_OKCANCEL | MB_ICONWARNING | MB_SYSTEMMODAL) != IDOK) {
						fReport = false;
					}
				}

				unknown[stse.style] = nullptr;
			}

			stse.style = L"Default";
		}
	}
}

void CSimpleTextSubtitle::AddStyle(CString name, STSStyle* style)
{
	if (name.IsEmpty()) {
		name = L"Default";
	}

	STSStyle* val;
	if (m_styles.Lookup(name, val)) {
		if (*val == *style) {
			delete style;
			return;
		}

		int i;
		int len = name.GetLength();

		for (i = len; i > 0 && _istdigit(name[i-1]); i--) {
			;
		}

		int idx = 1;

		CString name2 = name;

		if (i < len && swscanf_s(name.Right(len-i), L"%d", &idx) == 1) {
			name2 = name.Left(i);
		}

		idx++;

		CString name3;
		do {
			name3.Format(L"%s%d", name2, idx);
			idx++;
		} while (m_styles.Lookup(name3));

		m_styles.RemoveKey(name);
		m_styles[name3] = val;

		for (size_t k = 0, j = GetCount(); k < j; k++) {
			STSEntry& stse = GetAt(k);
			if (stse.style == name) {
				stse.style = name3;
			}
		}
	}

	m_styles[name] = style;
}

bool CSimpleTextSubtitle::SetDefaultStyle(STSStyle& s)
{
	STSStyle* val;
	if (m_styles.Lookup(L"Default", val)) {
		*val = s;
		m_fUsingAutoGeneratedDefaultStyle = false;
	} else {
		val = DNew STSStyle();
		*val = s;
		m_styles[L"Default"] = val;
		m_fUsingAutoGeneratedDefaultStyle = true;
	}

	return true;
}

bool CSimpleTextSubtitle::GetDefaultStyle(STSStyle& s)
{
	STSStyle* val;
	if (!m_styles.Lookup(L"Default", val)) {
		return false;
	}
	s = *val;
	return true;
}

void CSimpleTextSubtitle::ConvertToTimeBased(double fps)
{
	if (m_mode == TIME) {
		return;
	}

	for (size_t i = 0, j = GetCount(); i < j; i++) {
		STSEntry& stse = (*this)[i];
		stse.start = (int)(stse.start * 1000.0 / fps + 0.5);
		stse.end   = (int)(stse.end * 1000.0 / fps + 0.5);
	}

	m_mode = TIME;

	CreateSegments();
}

void CSimpleTextSubtitle::ConvertToFrameBased(double fps)
{
	if (m_mode == FRAME) {
		return;
	}

	for (size_t i = 0, j = GetCount(); i < j; i++) {
		STSEntry& stse = (*this)[i];
		stse.start = (int)(stse.start * fps / 1000 + 0.5);
		stse.end   = (int)(stse.end * fps / 1000 + 0.5);
	}

	m_mode = FRAME;

	CreateSegments();
}

int CSimpleTextSubtitle::SearchSub(int t, double fps)
{
	int i = 0, j = (int)GetCount() - 1, ret = -1;

	if (j >= 0 && t >= TranslateStart(j, fps)) {
		return j;
	}

	while (i < j) {
		int mid = (i + j) >> 1;

		int midt = TranslateStart(mid, fps);

		if (t == midt) {
			while (mid > 0 && t == TranslateStart(mid-1, fps)) {
				--mid;
			}
			ret = mid;
			break;
		} else if (t < midt) {
			ret = -1;
			if (j == mid) {
				mid--;
			}
			j = mid;
		} else if (t > midt) {
			ret = mid;
			if (i == mid) {
				++mid;
			}
			i = mid;
		}
	}

	return ret;
}

const STSSegment* CSimpleTextSubtitle::SearchSubs(int t, double fps, /*[out]*/ int* iSegment, int* nSegments)
{
	int i = 0, j = (int)m_segments.GetCount() - 1, ret = -1;

	if (nSegments) {
		*nSegments = j+1;
	}

	// last segment
	if (j >= 0 && t >= TranslateSegmentStart(j, fps) && t < TranslateSegmentEnd(j, fps)) {
		if (iSegment) {
			*iSegment = j;
		}
		return &m_segments[j];
	}

	// after last segment
	if (j >= 0 && t >= TranslateSegmentEnd(j, fps)) {
		if (iSegment) {
			*iSegment = j+1;
		}
		return nullptr;
	}

	// before first segment
	if (j > 0 && t < TranslateSegmentStart(i, fps)) {
		if (iSegment) {
			*iSegment = -1;
		}
		return nullptr;
	}

	while (i < j) {
		int mid = (i + j) >> 1;

		int midt = TranslateSegmentStart(mid, fps);

		if (t == midt) {
			ret = mid;
			break;
		} else if (t < midt) {
			ret = -1;
			if (j == mid) {
				mid--;
			}
			j = mid;
		} else if (t > midt) {
			ret = mid;
			if (i == mid) {
				mid++;
			}
			i = mid;
		}
	}

	if (0 <= ret && (size_t)ret < m_segments.GetCount()) {
		if (iSegment) {
			*iSegment = ret;
		}
	}

	if (0 <= ret && (size_t)ret < m_segments.GetCount()
			&& !m_segments[ret].subs.IsEmpty()
			&& TranslateSegmentStart(ret, fps) <= t && t < TranslateSegmentEnd(ret, fps)) {
		return &m_segments[ret];
	}

	return nullptr;
}

int CSimpleTextSubtitle::TranslateStart(int i, double fps)
{
	return (i < 0 || GetCount() <= (size_t)i ? -1 :
		   m_mode == TIME ? GetAt(i).start :
		   m_mode == FRAME ? (int)(GetAt(i).start*1000/fps) :
		   0);
}

int CSimpleTextSubtitle::TranslateEnd(int i, double fps)
{
	return (i < 0 || GetCount() <= (size_t)i ? -1 :
		   m_mode == TIME ? GetAt(i).end :
		   m_mode == FRAME ? (int)(GetAt(i).end*1000/fps) :
		   0);
}

int CSimpleTextSubtitle::TranslateSegmentStart(int i, double fps)
{
	return (i < 0 || m_segments.GetCount() <= (size_t)i ? -1 :
		   m_mode == TIME ? m_segments[i].start :
		   m_mode == FRAME ? (int)(m_segments[i].start*1000/fps) :
		   0);
}

int CSimpleTextSubtitle::TranslateSegmentEnd(int i, double fps)
{
	return (i < 0 || m_segments.GetCount() <= (size_t)i ? -1 :
		   m_mode == TIME ? m_segments[i].end :
		   m_mode == FRAME ? (int)(m_segments[i].end*1000/fps) :
		   0);
}

STSStyle* CSimpleTextSubtitle::GetStyle(int i)
{
	STSStyle* style = nullptr;
	m_styles.Lookup(GetAt(i).style, style);

	if (!style) {
		m_styles.Lookup(L"Default", style);
	}

	return style;
}

bool CSimpleTextSubtitle::GetStyle(int i, STSStyle& stss)
{
	STSStyle* style = nullptr;
	m_styles.Lookup(GetAt(i).style, style);

	STSStyle* defstyle = nullptr;
	m_styles.Lookup(L"Default", defstyle);

	if (!style) {
		if (!defstyle) {
			defstyle = CreateDefaultStyle(DEFAULT_CHARSET);
		}

		style = defstyle;
	}

	if (!style) {
		ASSERT(0);
		return false;
	}

	stss = *style;
	if (stss.relativeTo == STSStyle::AUTO && defstyle) {
		stss.relativeTo = defstyle->relativeTo;
		// If relative to is set to "auto" even for the default style, decide based on the subtitle type
		if (stss.relativeTo == STSStyle::AUTO) {
			if (m_subtitleType == Subtitle::SSA || m_subtitleType == Subtitle::ASS) {
				stss.relativeTo = STSStyle::VIDEO;
			} else {
				stss.relativeTo = STSStyle::WINDOW;
			}
		}
	}

	return true;
}

bool CSimpleTextSubtitle::GetStyle(CString styleName, STSStyle& stss)
{
	STSStyle* style = nullptr;
	m_styles.Lookup(styleName, style);
	if (!style) {
		return false;
	}

	stss = *style;

	STSStyle* defstyle = nullptr;
	m_styles.Lookup(L"Default", defstyle);
	if (defstyle && stss.relativeTo == STSStyle::AUTO) {
		stss.relativeTo = defstyle->relativeTo;
		// If relative to is set to "auto" even for the default style, decide based on the subtitle type
		if (stss.relativeTo == STSStyle::AUTO) {
			if (m_subtitleType == Subtitle::SSA || m_subtitleType == Subtitle::ASS) {
				stss.relativeTo = STSStyle::VIDEO;
			} else {
				stss.relativeTo = STSStyle::WINDOW;
			}
		}
	}

	return true;
}

int CSimpleTextSubtitle::GetCharSet(int i)
{
	const STSStyle* stss = GetStyle(i);
	return stss ? stss->charSet : DEFAULT_CHARSET;
}

static CString RemoveHtmlSpecialChars(CString str)
{
	str.Replace(L"&gt;", L">");
	str.Replace(L"&lt;", L"<");
	str.Replace(L"&amp;", L"&");
	str.Replace(L"&quot;", L"\"");
	str.Replace(L"&lrm;", L"\x200E");
	str.Replace(L"&rlm;", L"\x200F");

	return str;
}

CStringA CSimpleTextSubtitle::GetStrA(int i, UINT CodePage, bool fSSA)
{
	CStringW wstr = GetStrW(i, fSSA);

	CStringA str;
	int len = WideCharToMultiByte(CodePage, 0, wstr, -1, nullptr, 0, nullptr, nullptr) - 1;
	if (len > 0) {
		str.ReleaseBuffer(WideCharToMultiByte(CodePage, 0, wstr, -1, str.GetBuffer(len), len + 1, nullptr, nullptr) - 1);
	}
	return str;
}

CStringW CSimpleTextSubtitle::GetStrW(int i, bool fSSA)
{
	STSEntry const& stse = GetAt(i);
	int CharSet = GetCharSet(i);

	CStringW str = stse.str;

	if (!fSSA) {
		str = RemoveSSATags(str);
	}

	str = RemoveHtmlSpecialChars(str);

	return str;
}

void CSimpleTextSubtitle::SetStr(int i, CStringA str, UINT CodePage)
{
	SetStr(i, ConvertToWStr(str, CodePage));
}

void CSimpleTextSubtitle::SetStr(int i, CStringW str)
{
	STSEntry& stse = GetAt(i);

	str.Replace(L"\n", L"\\N");

	stse.str = str;
}

void CSimpleTextSubtitle::CodeToCharacter(CString& str)
{
	if (str.Find(L"&#") == -1) {
		return;
	}

	CString tmp;
	int strLen = str.GetLength();

	LPCWSTR pszString = str;
	LPCWSTR pszEnd = pszString + strLen;

	while (pszString < pszEnd) {
		int pos = FindNoCase(pszString, L"&#");
		if (pos == -1 || (strLen - pos < 3)) {
			pos = pszEnd - pszString;
			tmp.Append(pszString, pos);
			break;
		}
		tmp.Append(pszString, pos);

		pszString += pos + 2;
		int Radix = 10;
		if (towlower(*pszString) == L'x') {
			pszString++;
			Radix = 16;
		}

		wchar_t value = wcstol(pszString, (LPWSTR*)&pszString, Radix);
		tmp += value;
	}

	str = tmp;
}

static int comp1(const void* a, const void* b)
{
	int ret = ((STSEntry*)a)->start - ((STSEntry*)b)->start;
	if (ret == 0) {
		ret = ((STSEntry*)a)->layer - ((STSEntry*)b)->layer;
	}
	if (ret == 0) {
		ret = ((STSEntry*)a)->readorder - ((STSEntry*)b)->readorder;
	}
	return ret;
}

static int comp2(const void* a, const void* b)
{
	return (((STSEntry*)a)->readorder - ((STSEntry*)b)->readorder);
}

void CSimpleTextSubtitle::Sort(bool fRestoreReadorder)
{
	qsort(GetData(), GetCount(), sizeof(STSEntry), !fRestoreReadorder ? comp1 : comp2);
	CreateSegments();
}

struct Breakpoint {
	int t;
	bool isStart;

	Breakpoint(int t, bool isStart) : t(t), isStart(isStart) {};
};

static int BreakpointComp(const void* e1, const void* e2)
{
	const Breakpoint* bp1 = (const Breakpoint*)e1;
	const Breakpoint* bp2 = (const Breakpoint*)e2;

	return (bp1->t - bp2->t);
}

void CSimpleTextSubtitle::CreateSegments()
{
	m_segments.RemoveAll();

	CAtlArray<Breakpoint> breakpoints;

	for (size_t i = 0; i < GetCount(); i++) {
		STSEntry& stse = GetAt(i);
		breakpoints.Add(Breakpoint(stse.start, true));
		breakpoints.Add(Breakpoint(stse.end, false));
	}

	qsort(breakpoints.GetData(), breakpoints.GetCount(), sizeof(Breakpoint), BreakpointComp);

	ptrdiff_t startsCount = 0;
	for (size_t i = 1, end = breakpoints.GetCount(); i < end; i++) {
		startsCount += breakpoints[i - 1].isStart ? +1 : -1;
		if (breakpoints[i - 1].t != breakpoints[i].t && startsCount > 0) {
			m_segments.Add(STSSegment(breakpoints[i - 1].t, breakpoints[i].t));
		}
	}

	STSSegment* segmentsStart = m_segments.GetData();
	STSSegment* segmentsEnd   = segmentsStart + m_segments.GetCount();
	for (size_t i = 0; i < GetCount(); i++) {
		const STSEntry& stse = GetAt(i);
		STSSegment* segment = std::lower_bound(segmentsStart, segmentsEnd, stse.start, SegmentCompStart);
		for (size_t j = segment - segmentsStart; j < m_segments.GetCount() && m_segments[j].end <= stse.end; j++) {
			m_segments[j].subs.Add(int(i));
		}
	}

	OnChanged();
	/*
		for (size_t i = 0, j = m_segments.GetCount(); i < j; i++) {
			STSSegment& stss = m_segments[i];

			TRACE(L"%d - %d", stss.start, stss.end);

			for (size_t k = 0, l = stss.subs.GetCount(); k < l; k++) {
				TRACE(L", %d", stss.subs[k]);
			}

			TRACE(L"\n");
		}
	*/
}

bool CSimpleTextSubtitle::Open(const CString& fn, UINT codePage, bool bAutoDetectCodePage, CString name, CString videoName)
{
	Empty();

	CWebTextFile f(CP_UTF8, codePage, bAutoDetectCodePage);
	if (!f.Open(fn)) {
		return false;
	}

	if (name.IsEmpty()) {
		name = Subtitle::GuessSubtitleName(fn, videoName);
	}

	return Open(&f, name);
}

static size_t CountLines(CTextFile* f, ULONGLONG from, ULONGLONG to, CString& s)
{
	size_t n = 0;
	f->Seek(from, CFile::begin);
	while (f->ReadString(s) && f->GetPosition() < to) {
		n++;
	}
	return n;
}

bool CSimpleTextSubtitle::Open(CTextFile* f, const CString& name)
{
	Empty();

	const UINT charSet = CodePageToCharSet(f->GetEncoding());
	ULONGLONG pos = f->GetPosition();

	for (const auto& OpenFunct : s_OpenFuncts) {
		if (!OpenFunct.open(f, *this)) {
			if (!IsEmpty()) {
				CString lastLine;
				size_t n = CountLines(f, pos, f->GetPosition(), lastLine);
				CString msg;
				msg.Format(L"Unable to parse the subtitle file. Syntax error at line %Iu:\n\"%s\"", n + 1, lastLine);
				AfxMessageBox(msg, MB_OK|MB_ICONERROR);
				Empty();
				break;
			}

			f->Seek(pos, CFile::begin);
			Empty();
			continue;
		}

		m_name         = name;
		m_subtitleType = OpenFunct.type;
		m_mode         = OpenFunct.mode;
		m_encoding     = f->GetEncoding();
		m_path         = f->GetFilePath();

		// No need to call Sort() or CreateSegments(), everything is done on the fly

		CWebTextFile f2(CP_UTF8);
		if (f2.Open(f->GetFilePath() + L".style")) {
			OpenSubStationAlpha(&f2, *this);
			f2.Close();
		}

		CreateDefaultStyle(charSet);

		ChangeUnknownStylesToDefault();

		if (m_dstScreenSize == CSize(0, 0)) {
			m_dstScreenSize = DEFSCREENSIZE;
		}

		f->Close();
		return true;
	}

	f->Close();
	return false;
}

bool CSimpleTextSubtitle::Open(BYTE* data, int len, int CharSet, CString name)
{
	WCHAR path[MAX_PATH];
	if (!GetTempPathW(MAX_PATH, path)) {
		return false;
	}

	WCHAR fn[MAX_PATH];
	if (!GetTempFileNameW(path, L"vs", 0, fn)) {
		return false;
	}

	FILE* tmp = nullptr;
	if (_wfopen_s(&tmp, fn, L"wb") || !tmp) {
		return false;
	}

	int i = 0;
	for (; i <= (len-1024); i += 1024) {
		fwrite(&data[i], 1024, 1, tmp);
	}
	if (len > i) {
		fwrite(&data[i], len - i, 1, tmp);
	}

	fclose(tmp);

	bool fRet = Open(fn, CharSet, false, name, {});

	_wremove(fn);

	return fRet;
}

bool CSimpleTextSubtitle::SaveAs(CString fn, Subtitle::SubType type, double fps, int delay, UINT e, bool bCreateExternalStyleFile)
{
	LPCWSTR ext = Subtitle::GetSubtitleFileExt(type);
	if (fn.Mid(fn.ReverseFind('.') + 1).CompareNoCase(ext)) {
		if (fn[fn.GetLength()-1] != '.') {
			fn += L".";
		}
		fn += ext;
	}

	CTextFile f;
	if (!f.Save(fn, e)) {
		return false;
	}

	if (type == Subtitle::SMI) {
		CString str;

		str += L"<SAMI>\n<HEAD>\n";
		str += L"<STYLE TYPE=\"text/css\">\n";
		str += L"<!--\n";
		str += L"P {margin-left: 16pt; margin-right: 16pt; margin-bottom: 16pt; margin-top: 16pt;\n";
		str += L"   text-align: center; font-size: 18pt; font-family: arial; font-weight: bold; color: #f0f0f0;}\n";
		str += L".UNKNOWNCC {Name:Unknown; lang:en-US; SAMIType:CC;}\n";
		str += L"-->\n";
		str += L"</STYLE>\n";
		str += L"</HEAD>\n";
		str += L"\n";
		str += L"<BODY>\n";

		f.WriteString(str);
	} else if (type == Subtitle::SSA || type == Subtitle::ASS) {
		CString str;

		str  = L"[Script Info]\n";
		str += (type == Subtitle::SSA) ? L"; This is a Sub Station Alpha v4 script.\n" : L"; This is an Advanced Sub Station Alpha v4+ script.\n";
		str += L"; \n";
		if (type == Subtitle::ASS) {
			str += L"; Advanced Sub Station Alpha script format developed by #Anime-Fansubs@EfNET\n";
			str += L"; \n";
		}
		str += L"; Note: This file was saved by MPC-BE.\n";
		str += L"; \n";
		str += (type == Subtitle::SSA) ? L"ScriptType: v4.00\n" : L"ScriptType: v4.00+\n";
		str += (m_collisions == 0) ? L"Collisions: Normal\n" : L"Collisions: Reverse\n";
		if (type == Subtitle::ASS && m_fScaledBAS) {
			str += L"ScaledBorderAndShadow: Yes\n";
		}
		str += L"PlayResX: %d\n";
		str += L"PlayResY: %d\n";
		str += L"Timer: 100.0000\n";
		str += L"\n";
		str += (type == Subtitle::SSA)
			   ? L"[V4 Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour, Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding\n"
			   : L"[V4+ Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n";

		CString str2;
		str2.Format(str, m_dstScreenSize.cx, m_dstScreenSize.cy);
		f.WriteString(str2);

		str  = (type == Subtitle::SSA)
			   ? L"Style: %s,%s,%d,&H%06x,&H%06x,&H%06x,&H%06x,%d,%d,%d,%.2f,%.2f,%d,%d,%d,%d,%d,%d\n"
			   : L"Style: %s,%s,%d,&H%08x,&H%08x,&H%08x,&H%08x,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%d,%.2f,%.2f,%d,%d,%d,%d,%d\n";

		POSITION pos = m_styles.GetStartPosition();
		while (pos) {
			CString key;
			STSStyle* s;
			m_styles.GetNextAssoc(pos, key, s);

			if (type ==Subtitle::SSA) {
				str2.Format(str, key,
							s->fontName, (int)s->fontSize,
							s->colors[0]&0xffffff,
							s->colors[1]&0xffffff,
							s->colors[2]&0xffffff,
							s->colors[3]&0xffffff,
							s->fontWeight > FW_NORMAL ? -1 : 0, s->fItalic ? -1 : 0,
							s->borderStyle == 0 ? 1 : s->borderStyle == 1 ? 3 : 0,
							s->outlineWidthY, s->shadowDepthY,
							s->scrAlignment <= 3 ? s->scrAlignment : s->scrAlignment <= 6 ? ((s->scrAlignment-3)|8) : s->scrAlignment <= 9 ? ((s->scrAlignment-6)|4) : 2,
							s->marginRect.left, s->marginRect.right, (s->marginRect.top + s->marginRect.bottom) / 2,
							s->alpha[0],
							s->charSet);
				f.WriteString(str2);
			} else {
				str2.Format(str, key,
							s->fontName, (int)s->fontSize,
							(s->colors[0]&0xffffff) | (s->alpha[0]<<24),
							(s->colors[1]&0xffffff) | (s->alpha[1]<<24),
							(s->colors[2]&0xffffff) | (s->alpha[2]<<24),
							(s->colors[3]&0xffffff) | (s->alpha[3]<<24),
							s->fontWeight > FW_NORMAL ? -1 : 0,
							s->fItalic ? -1 : 0, s->fUnderline ? -1 : 0, s->fStrikeOut ? -1 : 0,
							s->fontScaleX, s->fontScaleY,
							s->fontSpacing, s->fontAngleZ,
							s->borderStyle == 0 ? 1 : s->borderStyle == 1 ? 3 : 0,
							s->outlineWidthY, s->shadowDepthY,
							s->scrAlignment,
							s->marginRect.left, s->marginRect.right, (int)((s->marginRect.top + s->marginRect.bottom) / 2),
							s->charSet);
				f.WriteString(str2);
			}
		}

		if (!IsEmpty()) {
			str  = L"\n";
			str += L"[Events]\n";
			str += (type == Subtitle::SSA)
				   ? L"Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"
				   : L"Format: Layer, Start, End, Style, Actor, MarginL, MarginR, MarginV, Effect, Text\n";
			f.WriteString(str);
		}
	}

	CStringW fmt =
		type == Subtitle::SRT ? L"%d\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\n%s\n\n" :
		type == Subtitle::SUB ? L"{%d}{%d}%s\n" :
		type == Subtitle::SMI ? L"<SYNC Start=%d><P Class=UNKNOWNCC>\n%s\n<SYNC Start=%d><P Class=UNKNOWNCC>&nbsp;\n" :
		type == Subtitle::PSB ? L"{%d:%02d:%02d}{%d:%02d:%02d}%s\n" :
		type == Subtitle::SSA ? L"Dialogue: Marked=0,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s,%s,%04d,%04d,%04d,%s,%s\n" :
		type == Subtitle::ASS ? L"Dialogue: %d,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s,%s,%04d,%04d,%04d,%s,%s\n" :
		L"";
	//	Sort(true);

	if (m_mode == FRAME) {
		delay = int(delay * fps / 1000);
	}

	for (int i = 0, j = (int)GetCount(), k = 0; i < j; i++) {
		STSEntry& stse = GetAt(i);

		int t1 = TranslateStart(i, fps) + delay;
		if (t1 < 0) {
			k++;
			continue;
		}

		int t2 = TranslateEnd(i, fps) + delay;

		int hh1 = (t1/60/60/1000);
		int mm1 = (t1/60/1000)%60;
		int ss1 = (t1/1000)%60;
		int ms1 = (t1)%1000;
		int hh2 = (t2/60/60/1000);
		int mm2 = (t2/60/1000)%60;
		int ss2 = (t2/1000)%60;
		int ms2 = (t2)%1000;

		CStringW str = GetStrW(i, type == Subtitle::SSA || type == Subtitle::ASS);

		CStringW str2;

		if (type ==Subtitle::SRT) {
			str2.Format(fmt, i - k + 1, hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2, str);
		} else if (type ==Subtitle::SUB) {
			str.Replace(L'\n', L'|');
			str2.Format(fmt, int(t1 * fps / 1000), int(t2 * fps / 1000), str);
		} else if (type ==Subtitle::SMI) {
			str.Replace(L"\n", L"<br>");
			str2.Format(fmt, t1, str, t2);
		} else if (type ==Subtitle::PSB) {
			str.Replace(L'\n', L'|');
			str2.Format(fmt, hh1, mm1, ss1, hh2, mm2, ss2, str);
		} else if (type ==Subtitle::SSA) {
			str.Replace(L"\n", L"\\N");
			str2.Format(fmt,
						hh1, mm1, ss1, ms1 / 10,
						hh2, mm2, ss2, ms2 / 10,
						stse.style, stse.actor,
						stse.marginRect.left, stse.marginRect.right, (stse.marginRect.top + stse.marginRect.bottom) / 2,
						stse.effect, str);
		} else if (type ==Subtitle::ASS) {
			str.Replace(L"\n", L"\\N");
			str2.Format(fmt,
						stse.layer,
						hh1, mm1, ss1, ms1 / 10,
						hh2, mm2, ss2, ms2 / 10,
						stse.style, stse.actor,
						stse.marginRect.left, stse.marginRect.right, (stse.marginRect.top + stse.marginRect.bottom) / 2,
						stse.effect, str);
		}

		f.WriteString(str2);
	}

	//	Sort();

	if (type ==Subtitle::SMI) {
		f.WriteString(L"</BODY>\n</SAMI>\n");
	}

	STSStyle* s;
	if (bCreateExternalStyleFile && !m_fUsingAutoGeneratedDefaultStyle && m_styles.Lookup(L"Default", s) && type != Subtitle::SSA && type != Subtitle::ASS) {
		CTextFile fstyle;
		if (!fstyle.Save(fn + L".style", e)) {
			return false;
		}

		CString str, str2;

		str += L"ScriptType: v4.00+\n";
		str += L"PlayResX: %d\n";
		str += L"PlayResY: %d\n";
		str += L"\n";
		str += L"[V4+ Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n";
		str2.Format(str, m_dstScreenSize.cx, m_dstScreenSize.cy);
		fstyle.WriteString(str2);

		str  = L"Style: Default,%s,%d,&H%08x,&H%08x,&H%08x,&H%08x,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%d,%.2f,%.2f,%d,%d,%d,%d,%d\n";
		str2.Format(str,
					s->fontName, (int)s->fontSize,
					(s->colors[0]&0xffffff) | (s->alpha[0]<<24),
					(s->colors[1]&0xffffff) | (s->alpha[1]<<24),
					(s->colors[2]&0xffffff) | (s->alpha[2]<<24),
					(s->colors[3]&0xffffff) | (s->alpha[3]<<24),
					s->fontWeight > FW_NORMAL ? -1 : 0,
					s->fItalic ? -1 : 0, s->fUnderline ? -1 : 0, s->fStrikeOut ? -1 : 0,
					s->fontScaleX, s->fontScaleY,
					s->fontSpacing, s->fontAngleZ,
					s->borderStyle == 0 ? 1 : s->borderStyle == 1 ? 3 : 0,
					s->outlineWidthY, s->shadowDepthY,
					s->scrAlignment,
					s->marginRect.left, s->marginRect.right, (int)((s->marginRect.top + s->marginRect.bottom) / 2),
					s->charSet);
		fstyle.WriteString(str2);
	}

	return true;
}

////////////////////////////////////////////////////////////////////

STSStyle::STSStyle()
{
	SetDefault();
}

void STSStyle::SetDefault()
{
	marginRect = CRect(20, 20, 20, 20);
	scrAlignment = 2;
	borderStyle = 0;
	outlineWidthX = outlineWidthY = 2;
	shadowDepthX = shadowDepthY = 3;
	colors[0] = 0x00ffffff;
	colors[1] = 0x0000ffff;
	colors[2] = 0x00000000;
	colors[3] = 0x00000000;
	alpha[0] = 0x00;
	alpha[1] = 0x00;
	alpha[2] = 0x00;
	alpha[3] = 0x80;
	charSet = DEFAULT_CHARSET;
	fontName = L"Arial";
	fontSize = 18;
	fontScaleX = fontScaleY = 100;
	fontSpacing = 0;
	fontWeight = FW_BOLD;
	fItalic = false;
	fUnderline = false;
	fStrikeOut = false;
	fBlur = 0;
	fGaussianBlur = 0;
	fontShiftX = fontShiftY = fontAngleZ = fontAngleX = fontAngleY = 0;
	relativeTo = STSStyle::AUTO;
}

bool STSStyle::operator == (const STSStyle& s) const
{
	return (marginRect == s.marginRect
		   && scrAlignment == s.scrAlignment
		   && borderStyle == s.borderStyle
		   && outlineWidthX == s.outlineWidthX
		   && outlineWidthY == s.outlineWidthY
		   && shadowDepthX == s.shadowDepthX
		   && shadowDepthY == s.shadowDepthY
		   && *((int*)&colors[0]) == *((int*)&s.colors[0])
		   && *((int*)&colors[1]) == *((int*)&s.colors[1])
		   && *((int*)&colors[2]) == *((int*)&s.colors[2])
		   && *((int*)&colors[3]) == *((int*)&s.colors[3])
		   && alpha[0] == s.alpha[0]
		   && alpha[1] == s.alpha[1]
		   && alpha[2] == s.alpha[2]
		   && alpha[3] == s.alpha[3]
		   && fBlur == s.fBlur
		   && fGaussianBlur == s.fGaussianBlur
		   && relativeTo == s.relativeTo
		   && IsFontStyleEqual(s));
}

bool STSStyle::IsFontStyleEqual(const STSStyle& s) const
{
	return (
			  charSet == s.charSet
			  && fontName == s.fontName
			  && fontSize == s.fontSize
			  && fontScaleX == s.fontScaleX
			  && fontScaleY == s.fontScaleY
			  && fontSpacing == s.fontSpacing
			  && fontWeight == s.fontWeight
			  && fItalic == s.fItalic
			  && fUnderline == s.fUnderline
			  && fStrikeOut == s.fStrikeOut
			  && fontAngleZ == s.fontAngleZ
			  && fontAngleX == s.fontAngleX
			  && fontAngleY == s.fontAngleY
			  // patch f001. fax fay patch (many instances at line)
			  && fontShiftX == s.fontShiftX
			  && fontShiftY == s.fontShiftY);
}

STSStyle& STSStyle::operator = (LOGFONTW& lf)
{
	charSet = lf.lfCharSet;
	fontName = lf.lfFaceName;
	HDC hDC = GetDC(0);
	fontSize = -MulDiv(lf.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
	ReleaseDC(0, hDC);
	//	fontAngleZ = lf.lfEscapement/10.0;
	fontWeight = lf.lfWeight;
	fItalic = lf.lfItalic;
	fUnderline = lf.lfUnderline;
	fStrikeOut = lf.lfStrikeOut;
	return *this;
}

LOGFONTA& operator <<= (LOGFONTA& lfa, STSStyle& s)
{
	lfa.lfCharSet = s.charSet;
	strncpy_s(lfa.lfFaceName, LF_FACESIZE, CStringA(s.fontName), _TRUNCATE);
	HDC hDC = GetDC(0);
	lfa.lfHeight = -MulDiv((int)(s.fontSize+0.5), GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(0, hDC);
	lfa.lfWeight = s.fontWeight;
	lfa.lfItalic = s.fItalic?-1:0;
	lfa.lfUnderline = s.fUnderline?-1:0;
	lfa.lfStrikeOut = s.fStrikeOut?-1:0;
	return lfa;
}

LOGFONTW& operator <<= (LOGFONTW& lfw, STSStyle& s)
{
	lfw.lfCharSet = s.charSet;
	wcsncpy_s(lfw.lfFaceName, LF_FACESIZE, s.fontName, _TRUNCATE);
	HDC hDC = GetDC(0);
	lfw.lfHeight = -MulDiv((int)(s.fontSize+0.5), GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(0, hDC);
	lfw.lfWeight = s.fontWeight;
	lfw.lfItalic = s.fItalic?-1:0;
	lfw.lfUnderline = s.fUnderline?-1:0;
	lfw.lfStrikeOut = s.fStrikeOut?-1:0;
	return lfw;
}

CString& operator <<= (CString& style, const STSStyle& s)
{
	style.Format(L"%d;%d;%d;%d;%d;%d;%f;%f;%f;%f;0x%06lx;0x%06lx;0x%06lx;0x%06lx;0x%02x;0x%02x;0x%02x;0x%02x;%d;%s;%f;%f;%f;%f;%ld;%d;%d;%d;%d;%f;%f;%f;%f;%d",
				 s.marginRect.left, s.marginRect.right, s.marginRect.top, s.marginRect.bottom,
				 s.scrAlignment, s.borderStyle,
				 s.outlineWidthX, s.outlineWidthY, s.shadowDepthX, s.shadowDepthY,
				 s.colors[0], s.colors[1], s.colors[2], s.colors[3],
				 s.alpha[0], s.alpha[1], s.alpha[2], s.alpha[3],
				 s.charSet,
				 s.fontName,s.fontSize,
				 s.fontScaleX, s.fontScaleY,
				 s.fontSpacing,s.fontWeight,
				 s.fItalic, s.fUnderline, s.fStrikeOut, s.fBlur, s.fGaussianBlur,
				 s.fontAngleZ, s.fontAngleX, s.fontAngleY,
				 s.relativeTo);

	return style;
}

STSStyle& operator <<= (STSStyle& s, const CString& style)
{
	s.SetDefault();

	try {
		CStringW str = style;
		LPCWSTR pszBuff = str;
		int nBuffLength = str.GetLength();
		if (str.Find(';')>=0) {
			s.marginRect.left = GetInt(pszBuff, nBuffLength, L';');
			s.marginRect.right = GetInt(pszBuff, nBuffLength, L';');
			s.marginRect.top = GetInt(pszBuff, nBuffLength, L';');
			s.marginRect.bottom = GetInt(pszBuff, nBuffLength, L';');
			s.scrAlignment = GetInt(pszBuff, nBuffLength, L';');
			s.borderStyle = GetInt(pszBuff, nBuffLength, L';');
			s.outlineWidthX = GetFloat(pszBuff, nBuffLength, L';');
			s.outlineWidthY = GetFloat(pszBuff, nBuffLength, L';');
			s.shadowDepthX = GetFloat(pszBuff, nBuffLength, L';');
			s.shadowDepthY = GetFloat(pszBuff, nBuffLength, L';');
			for (size_t i = 0; i < 4; i++) {
				s.colors[i] = (COLORREF)GetInt(pszBuff, nBuffLength, L';');
			}
			for (size_t i = 0; i < 4; i++) {
				s.alpha[i] = GetInt(pszBuff, nBuffLength, L';');
			}
			s.charSet = GetInt(pszBuff, nBuffLength, L';');
			s.fontName = GetStrW(pszBuff, nBuffLength, L';');
			s.fontSize = GetFloat(pszBuff, nBuffLength, L';');
			s.fontScaleX = GetFloat(pszBuff, nBuffLength, L';');
			s.fontScaleY = GetFloat(pszBuff, nBuffLength, L';');
			s.fontSpacing = GetFloat(pszBuff, nBuffLength, L';');
			s.fontWeight = GetInt(pszBuff, nBuffLength, L';');
			s.fItalic = GetInt(pszBuff, nBuffLength, L';');
			s.fUnderline = GetInt(pszBuff, nBuffLength, L';');
			s.fStrikeOut = GetInt(pszBuff, nBuffLength, L';');
			s.fBlur = GetInt(pszBuff, nBuffLength, L';');
			s.fGaussianBlur = GetFloat(pszBuff, nBuffLength, L';');
			s.fontAngleZ = GetFloat(pszBuff, nBuffLength, L';');
			s.fontAngleX = GetFloat(pszBuff, nBuffLength, L';');
			s.fontAngleY = GetFloat(pszBuff, nBuffLength, L';');
			s.relativeTo = (STSStyle::RelativeTo)GetInt(pszBuff, nBuffLength, L';');
		}
	} catch (...) {
		s.SetDefault();
	}

	return s;
}

static bool OpenRealText(CTextFile* file, CSimpleTextSubtitle& ret)
{
	std::wstring szFile;
	CStringW buff;

	while (file->ReadString(buff)) {
		FastTrim(buff);
		if (buff.IsEmpty()) {
			continue;
		}

		szFile += CStringW(L"\n") + buff.GetBuffer();
	}

	CRealTextParser RealTextParser;
	if (!RealTextParser.ParseRealText(szFile)) {
		return false;
	}

	CRealTextParser::Subtitles crRealText = RealTextParser.GetParsedSubtitles();

	for (auto i = crRealText.m_mapLines.cbegin(); i != crRealText.m_mapLines.cend(); ++i) {
		ret.Add(
			SubRipper2SSA(i->second.c_str()),
			i->first.first,
			i->first.second);
	}

	return !ret.IsEmpty();
}
