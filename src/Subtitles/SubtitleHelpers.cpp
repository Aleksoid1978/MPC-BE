/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include <io.h>
#include <regex>
#include "TextFile.h"
#include "SubtitleHelpers.h"

static LPCWSTR separators = L".\\-_";
static LPCWSTR extListVid = L"(avi)|(mkv)|(mp4)|((m2)?ts)";

LPCWSTR Subtitle::GetSubtitleFileExt(SubType type)
{
	return (type >= 0 && size_t(type) < std::size(s_SubFileExts)) ? s_SubFileExts[type] : nullptr;
}

void Subtitle::GetSubFileNames(CString fn, const std::vector<CString>& paths, std::vector<CString>& ret)
{
	ret.clear();

	fn.Replace('\\', '/');

	bool fWeb = false;
	{
		int i = fn.Find(L"http://");
		if (i > 0) {
			fn = L"http" + fn.Mid(i);
			fWeb = true;
		}
	}

	int l  = fn.ReverseFind('/') + 1;
	int l2 = fn.ReverseFind('.');
	if (l2 < l) { // no extension, read to the end
		l2 = fn.GetLength();
	}

	CString orgpath = fn.Left(l);
	CString title = fn.Mid(l, l2 - l);
	int titleLength = title.GetLength();
	//CString filename = title + L".nooneexpectsthespanishinquisition";

	if (!fWeb) {
		WIN32_FIND_DATA wfd;

		CString extListSub, regExpSub, regExpVid;

		for (const auto& subext : s_SubFileExts) {
			extListSub.AppendFormat(L"(%s)|", subext);
		}
		extListSub.TrimRight('|');

		regExpSub.Format(L"([%s]+.+)?\\.(%s)$", separators, extListSub);
		regExpVid.Format(L".+\\.(%s)$", extListVid);

		const std::wregex::flag_type reFlags = std::wregex::icase | std::wregex::optimize;
		std::wregex reSub(regExpSub, reFlags), reVid(regExpVid, reFlags);

		for (size_t k = 0; k < paths.size(); k++) {
			CString path = paths[k];
			path.Replace(L'\\', L'/');

			l = path.GetLength();
			if (l > 0 && path[l - 1] != L'/') {
				path += L'/';
			}

			if (path.Find(':') == -1 && path.Find(L"//") != 0) {
				path = orgpath + path;
			}

			path.Replace(L"/./", L"/");
			path.Replace(L'/', L'\\');

			std::list<CString> subs, vids;

			HANDLE hFile = FindFirstFileW(path + title + L"*", &wfd);
			if (hFile != INVALID_HANDLE_VALUE) {
				do {
					CString fn = path + wfd.cFileName;
					if (std::regex_match(&wfd.cFileName[titleLength], reSub)) {
						subs.push_back(fn);
					} else if (std::regex_match(&wfd.cFileName[titleLength], reVid)) {
						// Convert to lower-case and cut the extension for easier matching
						vids.push_back(fn.Left(fn.ReverseFind('.')).MakeLower());
					}
				} while (FindNextFileW(hFile, &wfd));

				FindClose(hFile);
			}

			for (const auto& sub : subs) {
				CString fnlower = sub;
				fnlower.MakeLower();

				// Check if there is an exact match for another video file
				bool bMatchAnotherVid = false;
				for (const auto& vid : vids) {
					if (fnlower.Find(vid) == 0) {
						bMatchAnotherVid = true;
						break;
					}
				}

				if (!bMatchAnotherVid) {
					ret.push_back(sub);
				}
			}
		}
	} else if (l > 7) {
		CWebTextFile wtf; // :)
		if (wtf.Open(orgpath + title + L".wse")) {
			CString fn2;
			while (wtf.ReadString(fn2) && fn2.Find(L"://") >= 0) {
				ret.push_back(fn2);
			}
		}
	}

	// sort files, this way the user can define the order (movie.00.English.srt, movie.01.Hungarian.srt, etc)
	std::sort(ret.begin(), ret.end(), [](const CString& a, const CString& b) {
		return a.CompareNoCase(b) < 0;

	});
}

CString Subtitle::GuessSubtitleName(CString fn, CString videoName)
{
	CString name, lang;
	bool bHearingImpaired = false;

	// The filename of the subtitle file
	int iExtStart = fn.ReverseFind('.');
	if (iExtStart < 0) {
		iExtStart = fn.GetLength();
	}
	CString subName = fn.Left(iExtStart).Mid(fn.ReverseFind('\\') + 1);

	if (!videoName.IsEmpty()) {
		// The filename of the video file
		iExtStart = videoName.ReverseFind('.');
		if (iExtStart < 0) {
			iExtStart = videoName.GetLength();
		}
		CString videoExt = videoName.Mid(iExtStart + 1).MakeLower();
		videoName = videoName.Left(iExtStart).Mid(videoName.ReverseFind('\\') + 1);

		CString subNameNoCase = CString(subName).MakeLower();
		CString videoNameNoCase = CString(videoName).MakeLower();

		// Check if the subtitle filename starts with the video filename
		// so that we can try to find a language info right after it
		if (subNameNoCase.Find(videoNameNoCase) == 0) {
			int iVideoNameEnd = videoName.GetLength();
			// Get ride of the video extension if it's in the subtitle filename
			if (subNameNoCase.Find(videoExt, iVideoNameEnd) == iVideoNameEnd + 1) {
				iVideoNameEnd += 1 + videoExt.GetLength();
			}
			subName = subName.Mid(iVideoNameEnd);

			std::wregex re(L"^[.\\-_ ]+([^.\\-_ ]+)(?:[.\\-_ ]+([^.\\-_ ]+))?", std::wregex::icase);
			std::wcmatch mc;
			if (std::regex_search((LPCWSTR)subName, mc, re)) {
				ASSERT(mc.size() == 3);
				ASSERT(mc[1].matched);
				lang = ISO639XToLanguage(CStringA(mc[1].str().c_str()), true);

				if (!lang.IsEmpty() && mc[2].matched) {
					bHearingImpaired = (CString(mc[2].str().c_str()).CompareNoCase(L"hi") == 0);
				}
			}
		}
	}

	// If we couldn't find any info yet, we try to find the language at the end of the filename
	if (lang.IsEmpty()) {
		std::wregex re(L".*?[.\\-_ ]+([^.\\-_ ]+)(?:[.\\-_ ]+([^.\\-_ ]+))?$", std::wregex::icase);
		std::wcmatch mc;
		if (std::regex_search((LPCWSTR)subName, mc, re)) {
			ASSERT(mc.size() == 3);
			ASSERT(mc[1].matched);
			lang = ISO639XToLanguage(CStringA(mc[1].str().c_str()), true);

			CStringA str;
			if (mc[2].matched) {
				str = mc[2].str().c_str();
			}

			if (!lang.IsEmpty() && str.CompareNoCase("hi") == 0) {
				bHearingImpaired = true;
			} else {
				lang = ISO639XToLanguage(str, true);
			}
		}
	}

	name = fn.Mid(fn.ReverseFind('\\') + 1);
	if (name.GetLength() > 100) { // Cut some part of the filename if it's too long
		name.Format(L"%s...%s", name.Left(50).TrimRight(L".-_ "), name.Right(50).TrimLeft(L".-_ "));
	}
	if (!lang.IsEmpty()) {
		name.AppendFormat(L" [%s]", lang);
		if (bHearingImpaired) {
			name.Append(L" [hearing impaired]");
		}
	}

	return name;
}
