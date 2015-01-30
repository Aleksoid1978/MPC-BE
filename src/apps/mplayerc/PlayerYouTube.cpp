/*
 * Copyright (C) 2012 Sergey "Exodus8" (rusguy6@gmail.com)
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
#include "atl/atlrx.h"
#include "PlayerYouTube.h"
#include "../../DSUtil/text.h"
#include <jsoncpp/include/json/json.h>

#define MATCH_STREAM_MAP_START		"\"url_encoded_fmt_stream_map\":\""
#define MATCH_ADAPTIVE_FMTS_START	"\"adaptive_fmts\":\""
#define MATCH_WIDTH_START			"meta property=\"og:video:width\" content=\""
#define MATCH_DASHMPD_START			"\"dashmpd\":\"http:\\/\\/www.youtube.com\\/api\\/manifest\\/dash\\/"
#define MATCH_HLSVP_START			"\"hlsvp\":\""
#define MATCH_END					"\""

#define MATCH_PLAYLIST_ITEM_START	"<li class=\"yt-uix-scroller-scroll-unit "
#define MATCH_PLAYLIST_ITEM_START2	"<tr class=\"pl-video yt-uix-tile "

#define INTERNET_OPEN_FALGS			INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD

const YOUTUBE_PROFILES* getProfile(int iTag) {
	for (int i = 0; i < _countof(youtubeProfiles); i++)
		if (iTag == youtubeProfiles[i].iTag) {
			return &youtubeProfiles[i];
	}

	return &youtubeProfileEmpty;
}

bool SelectBestProfile(int &itag_final, CString &ext_final, int itag_current, const YOUTUBE_PROFILES* sets)
{
	const YOUTUBE_PROFILES* current = getProfile(itag_current);

	if (current->iTag <= 0
			|| current->type != sets->type
			|| current->quality > sets->quality) {
		return false;
	}

	if (itag_final != 0) {
		const YOUTUBE_PROFILES* fin = getProfile(itag_final);
		if (current->quality < fin->quality) {
			return false;
		}
	}

	itag_final = current->iTag;
	ext_final = '.' + CString(current->ext);

	return true;
}

static CString FixHtmlSymbols(CString inStr)
{
	inStr.Replace(_T("&quot;"), _T("\""));
	inStr.Replace(_T("&amp;"), _T("&"));
	inStr.Replace(_T("&#39;"), _T("'"));
	inStr.Replace(_T("&#039;"), _T("'"));
	inStr.Replace(_T("\\n"), _T("\r\n"));
	inStr.Replace(_T("\n"), _T("\r\n"));
	inStr.Replace(_T("\\"), _T(""));

	return inStr;
}

static CStringA GetEntry(LPCSTR pszBuff, LPCSTR pszMatchStart, LPCSTR pszMatchEnd)
{
	LPCSTR pStart = CStringA::StrTraits::StringFindString(pszBuff, pszMatchStart);
	if (pStart) {
		pStart += CStringA::StrTraits::SafeStringLen(pszMatchStart);

		LPCSTR pEnd = CStringA::StrTraits::StringFindString(pStart, pszMatchEnd);
		if (pEnd) {
			return CStringA(pStart, pEnd - pStart);
		}
	}

	return "";
}

bool PlayerYouTubeCheck(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (tmp_fn.Find(YOUTUBE_URL) != -1 || tmp_fn.Find(YOUTU_BE_URL) != -1) {
		return true;
	}

	return false;
}

bool PlayerYouTubePlaylistCheck(CString fn)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (tmp_fn.Find(YOUTUBE_PL_URL) != -1 || (tmp_fn.Find(YOUTUBE_URL) != -1 && tmp_fn.Find(_T("&list=")) != -1)) {
		return true;
	}

	return false;
}

static CString RegExpParse(CString szIn, CString szRE)
{
	CAtlRegExp<> re;
	REParseError pe = re.Parse(szRE);
	if (REPARSE_ERROR_OK == pe) {
		LPCTSTR szEnd = szIn.GetBuffer();
		CAtlREMatchContext<> mc;
		if (re.Match(szEnd, &mc)) {
			LPCTSTR szStart;
			mc.GetMatch(0, &szStart, &szEnd);
			return CString(szStart, szEnd - szStart);
		}
	}

	return L"";
}

CString PlayerYouTube(CString fn, YOUTUBE_FIELDS* y_fields, CSubtitleItemList* subs)
{
	CString tmp_fn(CString(fn).MakeLower());

	if (PlayerYouTubeCheck(fn)) {
		char* data = NULL;
		DWORD dataSize = 0;

		int stream_map_start = 0;
		int stream_map_len = 0;

		int adaptive_fmts_start = 0;
		int adaptive_fmts_len = 0;

		int video_width_start = 0;
		int video_width_len = 0;

		int hlsvp_start = 0;
		int hlsvp_len = 0;

		int nMaxWidth = 0;

		AppSettings& sApp = AfxGetAppSettings();
		const YOUTUBE_PROFILES* youtubeSets = getProfile(sApp.iYoutubeTag);
		if (youtubeSets->iTag == 0) {
			youtubeSets = getProfile(22);
		}

#if 0
		BOOL bIsFullHD = FALSE;
		if (sApp.laylisiYoutubeTag == 37) {
			// Full HD resolution, format .MP4
			match_itag	= FALSE;
			bIsFullHD	= TRUE;
		}
#endif

		CString videoId;

		HINTERNET f, s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
		if (s) {
			CString link = fn;
			if (link.Find(YOUTU_BE_URL) != -1) {
				link.Replace(YOUTU_BE_URL, YOUTUBE_URL);
				link.Replace(_T("watch?"), _T("watch?v="));
			}

			videoId = RegExpParse(link, L"v={[-a-zA-Z0-9_]+}");

			f = InternetOpenUrl(s, link, NULL, 0, INTERNET_OPEN_FALGS, 0);
			if (f) {
				char buffer[4096] = { 0 };
				DWORD dwBytesRead = 0;

				do {
					if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead) == FALSE) {
						break;
					}

					data = (char*)realloc(data, dataSize + dwBytesRead + 1);
					memcpy(data + dataSize, buffer, dwBytesRead);
					dataSize += dwBytesRead;
					data[dataSize] = 0;

					// url_encoded_fmt_stream_map
					if (!stream_map_start && (stream_map_start = strpos(data, MATCH_STREAM_MAP_START)) != 0) {
						stream_map_start += strlen(MATCH_STREAM_MAP_START);
					}
					if (stream_map_start && !stream_map_len) {
						stream_map_len = strpos(data + stream_map_start, MATCH_END);
					}

					// adaptive_fmts
					if (!adaptive_fmts_start && (adaptive_fmts_start = strpos(data, MATCH_ADAPTIVE_FMTS_START)) != 0) {
						adaptive_fmts_start += strlen(MATCH_STREAM_MAP_START);
					}
					if (adaptive_fmts_start && !adaptive_fmts_len) {
						adaptive_fmts_len = strpos(data + adaptive_fmts_start, MATCH_END);
					}

					// <meta property="og:video:width" content="....">
					if (!video_width_start && (video_width_start = strpos(data, MATCH_WIDTH_START)) != 0) {
						video_width_start += strlen(MATCH_WIDTH_START);
					}
					if (video_width_start && !video_width_len) {
						video_width_len = strpos(data + video_width_start, MATCH_END);
					}

					// "hlspv" - live streaming
					if (!hlsvp_start && (hlsvp_start = strpos(data, MATCH_HLSVP_START)) != 0) {
						hlsvp_start += strlen(MATCH_HLSVP_START);
					}
					if (hlsvp_start && !hlsvp_len) {
						hlsvp_len = strpos(data + hlsvp_start, MATCH_END);
					}

					// optimization - to not download the entire page
					if (stream_map_len && adaptive_fmts_len) {
						if (nMaxWidth != 1920) {
							break;
						}
					}

					if (hlsvp_len) {
						break;
					}
				} while (dwBytesRead);

				InternetCloseHandle(f);
			}
			InternetCloseHandle(s);
		}

		if (!data || !f || !s) {
			return fn;
		}

		if (!stream_map_len && !hlsvp_len) {
			if (strstr(data, YOUTUBE_MP_URL)) {
				// This is looks like Youtube page, but this page doesn't contains necessary information about video, so may be you have to register on google.com to view it.
				fn.Empty();
			}
			free(data);
			return fn;
		}

		CString Title = UTF8To16(GetEntry(data, "<title>", "</title>"));

		if (hlsvp_len) {
			char *tmp = DNew char[hlsvp_len + 1];
			memcpy(tmp, data + hlsvp_start, hlsvp_len);
			tmp[hlsvp_len] = 0;
			free(data);

			CStringA strA = CStringA(tmp);
			delete [] tmp;

			CString url = UrlDecode(UrlDecode(strA));
			url.Replace(L"\\/", L"/");

			return url;
		}

		char *tmp = DNew char[stream_map_len + adaptive_fmts_len + 2];
		memcpy(tmp, data + stream_map_start, stream_map_len);
		tmp[stream_map_len] = ',';
		memcpy(tmp + stream_map_len + 1, data + adaptive_fmts_start, adaptive_fmts_len);
		tmp[stream_map_len + 1 + adaptive_fmts_len] = 0;
		free(data);

		CStringA strA = CStringA(tmp);
		delete [] tmp;
		strA.Replace("\\u0026", "&");

		CString final_url;
		CString final_ext;
		int final_itag = 0;

		CAtlList<CStringA> linesA;
		Explode(strA, linesA, ',');

		POSITION posLine = linesA.GetHeadPosition();
		while (posLine) {
			CStringA &lineA = linesA.GetNext(posLine);

			int itag = 0;
			CStringA url;
			CString ext;

			CAtlList<CStringA> paramsA;
			Explode(lineA, paramsA, '&');

			POSITION posParam = paramsA.GetHeadPosition();
			while (posParam) {
				CStringA &paramA = paramsA.GetNext(posParam);

				int k = paramA.Find('=');
				if (k >= 0) {
					CStringA paramHeader = paramA.Left(k);
					CStringA paramValue = paramA.Mid(k + 1);

					// "quality", "fallback_host", "url", "itag", "type", "s"
					if (paramHeader == "url") {
						url = UrlDecode(UrlDecode(paramValue));
					} else if (paramHeader == "itag") {
						if (sscanf_s(paramValue, "%d", &itag) != 1) {
							itag = 0;
						}
					}
				}
			}

			if (itag) {
				if (SelectBestProfile(final_itag, final_ext, itag, youtubeSets)) {
					final_url = url;
				}
			}
		}

		if (y_fields) {
			y_fields->title = FixHtmlSymbols(Title);
			y_fields->fname = y_fields->title + final_ext;
			FixFilename(y_fields->fname);
		}

		if (!final_url.IsEmpty()) {
			final_url.Replace(L"http://", L"https://");

			if (!videoId.IsEmpty() && y_fields) {
				HINTERNET f, s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
				if (s) {
					CString link;
					link.Format(L"https://gdata.youtube.com/feeds/api/videos/%s?alt=json&fields=published,title,author/name,content", videoId);
					f = InternetOpenUrl(s, link, NULL, 0, INTERNET_OPEN_FALGS, 0);
					if (f) {
						char* data = NULL;
						DWORD dataSize = 0;

						char buffer[4096] = { 0 };
						DWORD dwBytesRead = 0;
						do {
							if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead) == FALSE) {
								break;
							}

							data = (char*)realloc(data, dataSize + dwBytesRead + 1);
							memcpy(data + dataSize, buffer, dwBytesRead);
							dataSize += dwBytesRead;
							data[dataSize] = 0;
						} while (dwBytesRead);
						InternetCloseHandle(f);

						if (dataSize) {
							Json::Reader reader;
							Json::Value root;
							if (reader.parse(data, root)) {
								if (!root["entry"]["title"]["$t"].empty()
										&& root["entry"]["title"]["$t"].isString()) {
									CStringA sTitle = root["entry"]["title"]["$t"].asCString();
									if (!sTitle.IsEmpty()) {
										y_fields->title = FixHtmlSymbols(UTF8To16(sTitle));
										y_fields->fname = y_fields->title + final_ext;
										FixFilename(y_fields->fname);
									}
								}

								if (!root["entry"]["author"][0]["name"]["$t"].empty()
										&& root["entry"]["author"][0]["name"]["$t"].isString()) {
									CStringA sAuthor = root["entry"]["author"][0]["name"]["$t"].asCString();
									if (!sAuthor.IsEmpty()) {
										y_fields->author = UTF8To16(sAuthor);
									}
								}

								if (!root["entry"]["content"]["$t"].empty()
										&& root["entry"]["content"]["$t"].isString()) {
									CStringA sContent = root["entry"]["content"]["$t"].asCString();
									if (!sContent.IsEmpty()) {
										y_fields->content = FixHtmlSymbols(UTF8To16(sContent));
									}
								}

								if (!root["entry"]["published"]["$t"].empty()
										&& root["entry"]["published"]["$t"].isString()) {
									CStringA sDate = root["entry"]["published"]["$t"].asCString();
									if (!sDate.IsEmpty()) {
										WORD y, m, d;
										WORD hh, mm, ss;
										if (sscanf_s(sDate, "%04hu-%02hu-%02huT%02hu:%02hu:%02hu", &y, &m, &d, &hh, &mm, &ss) == 6) {
											y_fields->dtime.wYear = y;
											y_fields->dtime.wMonth = m;
											y_fields->dtime.wDay = d;
											y_fields->dtime.wHour = hh;
											y_fields->dtime.wMinute = mm;
											y_fields->dtime.wSecond = ss;
										}
									}
								}
							}

							free(data);
						}
					}
					InternetCloseHandle(s);
				}
			}

			if (!videoId.IsEmpty() && subs) { // subtitle
				HINTERNET f, s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
				if (s) {
					CString link;
					link.Format(L"https://video.google.com/timedtext?hl=en&type=list&v=%s", videoId);
					f = InternetOpenUrl(s, link, NULL, 0, INTERNET_OPEN_FALGS, 0);
					if (f) {
						char* data = NULL;
						DWORD dataSize = 0;

						char buffer[1024] = { 0 };
						DWORD dwBytesRead = 0;
						do {
							if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead) == FALSE) {
								break;
							}

							data = (char*)realloc(data, dataSize + dwBytesRead + 1);
							memcpy(data + dataSize, buffer, dwBytesRead);
							dataSize += dwBytesRead;
							data[dataSize] = 0;
						} while (dwBytesRead);
						InternetCloseHandle(f);

						if (dataSize) {
							CString xml = UTF8To16(data);
							free(data);

							CAtlRegExp<> re;
							CAtlREMatchContext<> mc;
							REParseError pe = re.Parse(L"<track id{.*?}/>");

							if (REPARSE_ERROR_OK == pe) {
								LPCTSTR szEnd = xml.GetBuffer();
								while (re.Match(szEnd, &mc)) {
									CString url, name;

									LPCTSTR szStart;
									mc.GetMatch(0, &szStart, &szEnd);
									CString xmlElement = CString(szStart, int(szEnd - szStart));

									CAtlRegExp<> reElement;
									CAtlREMatchContext<> mcElement;
									pe = reElement.Parse(L" {[a-z_]+}=\"{[^\"]+}\"");
									LPCTSTR szElementEnd = xmlElement.GetBuffer();
									while (reElement.Match(szElementEnd, &mcElement)) {
										LPCTSTR szElementStart;

										mcElement.GetMatch(0, &szElementStart, &szElementEnd);
										CString xmlHeader = CString(szElementStart, int(szElementEnd - szElementStart));

										mcElement.GetMatch(1, &szElementStart, &szElementEnd);
										CString xmlValue = CString(szElementStart, int(szElementEnd - szElementStart));

										if (xmlHeader == L"lang_code") {
											url.Format(L"https://www.youtube.com/api/timedtext?lang=%s&v=%s&fmt=srt", xmlValue, videoId);
										} else if (xmlHeader == L"lang_original") {
											name = xmlValue;
										}
									}

									if (!url.IsEmpty() && !name.IsEmpty()) {
										subs->AddTail(CSubtitleItem(url, name));
									}
								}
							}
						}
					}
					InternetCloseHandle(s);
				}
			}

			return final_url;
		}
	}

	return fn;
}

bool PlayerYouTubePlaylist(CString fn, YoutubePlaylist& youtubePlaylist, int& idx_CurrentPlay)
{
	idx_CurrentPlay = 0;
	if (PlayerYouTubePlaylistCheck(fn)) {

		char* data = NULL;
		DWORD dataSize = 0;

		HINTERNET f, s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
		if (s) {
			f = InternetOpenUrl(s, fn, NULL, 0, INTERNET_OPEN_FALGS, 0);
			if (f) {
				char buffer[4096] = { 0 };
				DWORD dwBytesRead = 0;

				do {
					if (InternetReadFile(f, (LPVOID)buffer, _countof(buffer), &dwBytesRead) == FALSE) {
						break;
					}

					data = (char*)realloc(data, dataSize + dwBytesRead + 1);
					memcpy(data + dataSize, buffer, dwBytesRead);
					dataSize += dwBytesRead;
					data[dataSize] = 0;

					if (/*strstr(data, "id=\"player\"") || */strstr(data, "id=\"footer\"")) {
						break;
					}
				} while (dwBytesRead);

				InternetCloseHandle(f);
			}
			InternetCloseHandle(s);
		}

		if (!data || !f || !s) {
			return false;
		}

		char* block = data;

		char* sMatch = NULL;
		if (strstr(block, MATCH_PLAYLIST_ITEM_START)) {
			sMatch = MATCH_PLAYLIST_ITEM_START;
		} else if (strstr(block, MATCH_PLAYLIST_ITEM_START2)) {
			sMatch = MATCH_PLAYLIST_ITEM_START2;
		} else {
			free(data);
			return false;
		}
		while ((block = strstr(block, sMatch)) != NULL) {
			block += strlen(sMatch);

			int block_len = strpos(block, ">");
			if (block_len) {
				char* tmp = DNew char[block_len + 1];
				memcpy(tmp, block, block_len);
				tmp[block_len] = 0;
				CString item = UTF8To16(tmp);
				delete [] tmp;

				CString data_video_id;
				int data_index = 0;
				CString data_video_username;
				CString data_video_title;

				bool bCurrentPlay = (item.Find(L"currently-playing") != -1);

				CAtlRegExp<> re;
				CAtlREMatchContext<> mc;
				REParseError pe = re.Parse(L" {[a-z-]+}=\"{[^\"]+}\"");
				if (REPARSE_ERROR_OK == pe) {

					LPCTSTR szEnd = item.GetBuffer();
					while (re.Match(szEnd, &mc)) {
						LPCTSTR szStart;
						mc.GetMatch(0, &szStart, &szEnd);
						CString propHeader = CString(szStart, int(szEnd - szStart));

						mc.GetMatch(1, &szStart, &szEnd);
						CString propValue = CString(szStart, int(szEnd - szStart));

						// data-video-id, data-video-clip-end, data-index, data-video-username, data-video-title, data-video-clip-start.
						if (propHeader == L"data-video-id") {
							data_video_id = propValue;
						} else if (propHeader == L"data-index") {
							if (swscanf_s(propValue, L"%d", &data_index) != 1) {
								data_index = 0;
							}
						} else if (propHeader == L"data-video-username") {
							data_video_username = propValue;
						} else if (propHeader == L"data-video-title" || propHeader == L"data-title") {
							data_video_title = FixHtmlSymbols(propValue);
						}
					}
				}

				if (!data_video_id.IsEmpty()) {
					CString url;
					url.Format(L"http://www.youtube.com/watch?v=%s", data_video_id);
					YoutubePlaylistItem item(url, data_video_title);
					youtubePlaylist.AddTail(item);

					if (bCurrentPlay) {
						idx_CurrentPlay = youtubePlaylist.GetCount() - 1;
					}
				}
			}
		}

		free(data);

		if (!youtubePlaylist.IsEmpty()) {
			return true;
		}
	}

	return false;
}
