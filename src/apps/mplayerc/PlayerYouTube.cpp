/*
 * (C) 2012-2026 see Authors.txt
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
#include "DSUtil/HTTPAsync.h"
#include "DSUtil/text.h"
#include "DSUtil/std_helper.h"
#include "PlayerYouTube.h"

#include "rapidjsonHelper.h"

#define YOUTUBE_PL_URL              L"youtube.com/playlist?"
#define YOUTUBE_USER_URL            L"youtube.com/user/"
#define YOUTUBE_USER_SHORT_URL      L"youtube.com/c/"
#define YOUTUBE_CHANNEL_URL         L"youtube.com/channel/"
#define YOUTUBE_URL                 L"youtube.com/watch?"
#define YOUTUBE_URL_A               L"www.youtube.com/attribution_link"
#define YOUTUBE_URL_V               L"youtube.com/v/"
#define YOUTUBE_URL_EMBED           L"youtube.com/embed/"
#define YOUTUBE_URL_SHORTS          L"youtube.com/shorts/"
#define YOUTUBE_URL_CLIP            L"youtube.com/clip/"
#define YOUTUBE_URL_LIBRARY         L"youtube.com/@"
#define YOUTUBE_URL_LIVE            L"youtube.com/live/"
#define YOUTU_BE_URL                L"youtu.be/"

#define MATCH_JS_START              "\"js\":\""
#define MATCH_JS_START_2            "'PREFETCH_JS_RESOURCES': [\""
#define MATCH_JS_START_3            "\"PLAYER_JS_URL\":\""
#define MATCH_END                   "\""

#define MATCH_PLAYLIST_ITEM_START   "<li class=\"yt-uix-scroller-scroll-unit "
#define MATCH_PLAYLIST_ITEM_START2  "<tr class=\"pl-video yt-uix-tile "

#define MATCH_PLAYER_RESPONSE       "ytInitialPlayerResponse = "
#define MATCH_PLAYER_RESPONSE_END   "};"

namespace Youtube
{
#define USE_GOOGLE_API 0

#if USE_GOOGLE_API
#if __has_include("..\..\my_google_api_key.h")
#include "..\..\my_google_api_key.h"
#else
	constexpr LPCWSTR strGoogleApiKey = L"place_your_google_api_key_here";
#endif
#endif

	constexpr LPCWSTR videoIdRegExp = L"(?:v|video_ids)=([-a-zA-Z0-9_]+)";

	static CString FixHtmlSymbols(CString inStr)
	{
		inStr.Replace(L"&quot;", L"\"");
		inStr.Replace(L"&amp;",  L"&");
		inStr.Replace(L"&#39;",  L"'");
		inStr.Replace(L"&#039;", L"'");
		inStr.Replace(L"\\n",    L"\r\n");
		inStr.Replace(L"\n",     L"\r\n");
		inStr.Replace(L"\\",     L"");

		return inStr;
	}

	static CStringA GetEntry(LPCSTR pszBuff, LPCSTR pszMatchStart, LPCSTR pszMatchEnd)
	{
		if (pszBuff) {
			LPCSTR pStart = CStringA::StrTraits::StringFindString(pszBuff, pszMatchStart);
			if (pStart) {
				pStart += CStringA::StrTraits::SafeStringLen(pszMatchStart);

				LPCSTR pEnd = CStringA::StrTraits::StringFindString(pStart, pszMatchEnd);
				if (pEnd) {
					return CStringA(pStart, pEnd - pStart);
				} else {
					pEnd = pszBuff + CStringA::StrTraits::SafeStringLen(pszBuff);
					return CStringA(pStart, pEnd - pStart);
				}
			}
		}

		return {};
	}

	bool CheckYtVideoURL(CString url)
	{
		url.MakeLower();

		LPCWSTR s = url.GetString();
		StartsWithStep(s, L"https://");
		StartsWithStep(s, L"www.");

		if (StartsWith(s, L"youtu.be/")) {
			return true;
		}

		if (StartsWithStep(s, L"youtube.com/")) {
			if (StartsWith(s, L"watch?")
				|| StartsWith(s, L"attribution_link?")
				|| StartsWith(s, L"v/")
				|| StartsWith(s, L"embed/")
				|| StartsWith(s, L"shorts/")
				|| StartsWith(s, L"clip/")
				|| StartsWith(s, L"live/")
				) {
				return true;
			}
		}

		return false;
	}

	bool CheckYtPlaylistURL(CString url)
	{
		url.MakeLower();

		LPCWSTR s = url.GetString();
		StartsWithStep(s, L"https://");
		StartsWithStep(s, L"www.");

		if (StartsWithStep(s, L"youtube.com/")) {
			if (StartsWith(s, L"playlist?")
				|| StartsWith(s, L"user/")
				|| StartsWith(s, L"channel/")
				|| StartsWith(s, L"c/")
				|| StartsWith(s, L"@")
				) {
				return true;
			}

			if (StartsWithStep(s, L"watch?")) {
				if (wcsstr(s, L"&list=")) {
					return true;
				}
			}
			else if (StartsWithStep(s, L"attribution_link?")) {
				if (wcsstr(s, L"/watch_videos?video_ids")) {
					return true;
				}
			}
			else if (StartsWithStep(s, L"v/") || StartsWithStep(s, L"embed/") || StartsWithStep(s, L"shorts/")) {
				if (wcsstr(s, L"list=")) {
					return true;
				}
			}
		}
		else if (StartsWithStep(s, L"youtu.be/")) {
			if (wcsstr(s, L"list=")) {
				return true;
			}
		}

		return false;
	}

	using urlData = std::vector<char>;
	static bool URLReadData(LPCWSTR url, urlData& pData, LPCWSTR header = L"")
	{
		pData.clear();

		CHTTPAsync HTTPAsync;
		if (FAILED(HTTPAsync.Connect(url, http::connectTimeout, header))) {
			return false;
		}
		const auto contentLength = HTTPAsync.GetLenght();
		if (contentLength) {
			pData.resize(contentLength);
			DWORD dwSizeRead = 0;
			if (S_OK != HTTPAsync.Read((PBYTE)pData.data(), contentLength, dwSizeRead, http::readTimeout) || dwSizeRead != contentLength) {
				pData.clear();
			}
		} else {
			static std::vector<char> tmp(16 * KILOBYTE);
			for (;;) {
				DWORD dwSizeRead = 0;
				if (S_OK != HTTPAsync.Read((PBYTE)tmp.data(), tmp.size(), dwSizeRead, http::readTimeout)) {
					break;
				}

				pData.insert(pData.end(), tmp.begin(), tmp.begin() + dwSizeRead);
			}
		}

		if (!pData.empty()) {
			pData.emplace_back('\0');
			return true;
		}

		return false;
	}

	static bool URLPostData(LPCWSTR lpszAgent, LPCWSTR headers, CStringA& requestData, urlData& pData)
	{
		pData.clear();

		if (auto hInet = InternetOpenW(lpszAgent, INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0)) {
			if (auto hSession = InternetConnectW(hInet, L"www.youtube.com", 443, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 1)) {
				if (auto hRequest = HttpOpenRequestW(hSession,
													 L"POST",
													 L"youtubei/v1/player", nullptr, nullptr, nullptr,
													 INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 1)) {

					if (HttpSendRequestW(hRequest, headers, -1,
										 reinterpret_cast<LPVOID>(requestData.GetBuffer()), requestData.GetLength())) {

						static std::vector<char> tmp(16 * 1024);
						for (;;) {
							DWORD dwSizeRead = 0;
							if (!InternetReadFile(hRequest, reinterpret_cast<LPVOID>(tmp.data()), tmp.size(), &dwSizeRead) || !dwSizeRead) {
								break;
							}

							pData.insert(pData.end(), tmp.begin(), tmp.begin() + dwSizeRead);
						}

						if (!pData.empty()) {
							pData.emplace_back('\0');
						}
					}

					InternetCloseHandle(hRequest);
				}
				InternetCloseHandle(hSession);
			}
			InternetCloseHandle(hInet);
		}

		return !pData.empty();
	}

	static CStringA ExtractYtcfg(LPCWSTR videoId)
	{
		constexpr LPCWSTR urls[] = {
			L"https://www.youtube.com/embed/",
			L"https://www.youtube.com/watch?v="
		};

		urlData data;
		for (auto url : urls) {
			if (URLReadData(CStringW(url) + videoId, data)) {
				const std::regex regex(R"(ytcfg\.set\s*\(\s*(\{.+?\})\s*\)\s*;)");
				std::cmatch match;
				if (std::regex_search(data.data(), match, regex) && match.size() == 2) {
					return CStringA(match[1].first, match[1].length());
				}
			}
		}

		return {};
	}

	static CStringW ExtractVisitorData(LPCWSTR videoId)
	{
		CStringW visitorData;

		if (auto ytcfg = ExtractYtcfg(videoId); !ytcfg.IsEmpty()) {
			rapidjson::Document json;
			if (!json.Parse(ytcfg).HasParseError()) {
				if (!getJsonValue(json, "VISITOR_DATA", visitorData)) {
					if (auto value = GetValueByPointer(json, "/INNERTUBE_CONTEXT/client/visitorData"); value && value->IsString()) {
						visitorData = UTF8ToWStr(value->GetString());
					}
				}
			}
		}

		if (!visitorData.IsEmpty()) {
			Unescape(visitorData);
		}

		return visitorData;
	}

	static bool URLPostData(LPCWSTR videoId, urlData& pData)
	{
		constexpr static auto requestStr =
			R"({"contentCheckOk": true, "context": {"client": {"clientName": "IOS", "clientVersion": "19.45.4", "deviceMake": "Apple", "deviceModel": "iPhone16,2", "hl": "en", "osName": "iPhone", "osVersion": "18.1.0.22B83", "timeZone": "UTC", "utcOffsetMinutes": 0}}, "playbackContext": {"contentPlaybackContext": {"html5Preference": "HTML5_PREF_WANTS"}}, "racyCheckOk" : true, "videoId" : "%ls"})";

		CStringA requestData;
		requestData.Format(requestStr, videoId);

		CStringW headers =
			L"X-YouTube-Client-Name: 5\r\n"
			L"X-YouTube-Client-Version: 19.45.4\r\n"
			L"Origin: https://www.youtube.com\r\n"
			L"Content-Type: application/json\r\n";

		auto visitorData = ExtractVisitorData(videoId);
		if (!visitorData.IsEmpty()) {
			headers.AppendFormat(L"X-Goog-Visitor-Id: %s\r\n", visitorData.GetString());
		}

		return URLPostData(L"com.google.ios.youtube/19.45.4 (iPhone16,2; U; CPU iOS 18_1_0 like Mac OS X;)", headers.GetString(), requestData, pData);
	}

	static bool URLPostDataForLive(LPCWSTR videoId, urlData& pData)
	{
		constexpr static auto requestStr =
			R"({"contentCheckOk": true, "context": {"client": {"clientName": "MWEB", "clientVersion": "2.20240726.01.00", "hl": "en", "timeZone": "UTC", "utcOffsetMinutes": 0}}, "playbackContext": {"contentPlaybackContext": {"html5Preference": "HTML5_PREF_WANTS"}}, "racyCheckOk": true, "videoId": "%ls"})";

		CStringA requestData;
		requestData.Format(requestStr, videoId);

		CStringW headers =
			LR"(X-YouTube-Client-Name: 2\r\n)"
			LR"(X-YouTube-Client-Version: 2.20240726.01.00\r\n)"
			LR"(Origin: https://www.youtube.com\r\n)"
			LR"(Content-Type: application/json\r\n)";

		auto visitorData = ExtractVisitorData(videoId);
		if (!visitorData.IsEmpty()) {
			headers.AppendFormat(L"X-Goog-Visitor-Id: %s\r\n", visitorData.GetString());
		}

		return URLPostData(http::userAgent.GetString(), headers, requestData, pData);
	}

	static bool ParseResponseJson(rapidjson::Document& json, YoutubeFields& y_fields)
	{
		bool bParse = false;
		if (!json.IsNull()) {
			if (auto videoDetails = GetJsonObject(json, "videoDetails")) {
				bParse = true;

				if (getJsonValue(*videoDetails, "title", y_fields.title)) {
					y_fields.title = FixHtmlSymbols(y_fields.title);
				}

				getJsonValue(*videoDetails, "author", y_fields.author);

				if (getJsonValue(*videoDetails, "shortDescription", y_fields.content)) {
					y_fields.content.Replace(L"\\r\\n", L"\r\n");
					y_fields.content.Replace(L"\\n", L"\r\n");

					std::wstring wstr = std::regex_replace(y_fields.content.GetString(), std::wregex(LR"(\r\n|\r|\n)"), L"\r\n");
					y_fields.content = wstr.c_str();
				}

				CStringA lengthSeconds;
				if (getJsonValue(*videoDetails, "lengthSeconds", lengthSeconds)) {
					y_fields.duration = atoi(lengthSeconds.GetString()) * UNITS;
				}
			}

			if (auto playerMicroformatRenderer = GetValueByPointer(json, "/playerMicroformatRenderer/publishDate"); playerMicroformatRenderer && playerMicroformatRenderer->IsObject()) {
				CStringA publishDate;
				if (getJsonValue(*playerMicroformatRenderer, "publishDate", publishDate)) {
					WORD y, m, d;
					if (sscanf_s(publishDate.GetString(), "%04hu-%02hu-%02hu", &y, &m, &d) == 3) {
						y_fields.dtime.wYear = y;
						y_fields.dtime.wMonth = m;
						y_fields.dtime.wDay = d;
					}
				}
			}
		}

		return bParse;
	}

	static void HandleURL(CString& url)
	{
		Unescape(url);

		if (url.Find(YOUTUBE_URL_CLIP) != -1) {
			urlData data;
			if (URLReadData(url.GetString(), data)) {
				auto jsonEntry = GetEntry(data.data(), MATCH_PLAYER_RESPONSE, MATCH_PLAYER_RESPONSE_END);
				if (!jsonEntry.IsEmpty()) {
					jsonEntry += "}";

					rapidjson::Document json;
					if (!json.Parse(jsonEntry).HasParseError()) {
						if (auto videoDetails = GetJsonObject(json, "videoDetails")) {
							CString videoId;
							if (getJsonValue(*videoDetails, "videoId", videoId)) {
								url = L"https://www.youtube.com/watch?v=" + videoId;
							}
						}
					}
				}
			}

			return;
		}

		int pos = url.Find(L"youtube.com/");
		if (pos == -1) {
			pos = url.Find(L"youtu.be/");
		}

		if (pos != -1) {
			url.Delete(0, pos);
			url = L"https://www." + url;

			if (url.Find(YOUTU_BE_URL) != -1) {
				url.Replace(YOUTU_BE_URL, YOUTUBE_URL);
				url.Replace(L"watch?", L"watch?v=");
				url.Replace(L"?list=", L"&list=");
			} else if (url.Find(YOUTUBE_URL_V) != -1) {
				url.Replace(L"v/", L"watch?v=");
				url.Replace(L"?list=", L"&list=");
			} else if (url.Find(YOUTUBE_URL_EMBED) != -1) {
				url.Replace(L"embed/", L"watch?v=");
				url.Replace(L"?list=", L"&list=");
			} else if (url.Find(YOUTUBE_URL_SHORTS) != -1) {
				url.Replace(L"shorts/", L"watch?v=");
				url.Replace(L"?list=", L"&list=");
			} else if (url.Find(YOUTUBE_URL_LIVE) != -1) {
				url.Replace(L"live/", L"watch?v=");
			}
		}
	}

	static bool ParseMetadataLocal(const CString& videoId, YoutubeFields& y_fields)
	{
		if (!videoId.IsEmpty()) {
#if !USE_GOOGLE_API
			bool bParse = false;
			urlData data;
			if (URLPostDataForLive(videoId.GetString(), data)) {
				rapidjson::Document player_response_jsonDocument;
				player_response_jsonDocument.Parse(data.data());

				bParse = ParseResponseJson(player_response_jsonDocument, y_fields);
			}
			return bParse;
#else
			CString url;
			url.Format(L"https://www.googleapis.com/youtube/v3/videos?id=%s&key=%s&part=snippet,contentDetails&fields=items/snippet/title,items/snippet/publishedAt,items/snippet/channelTitle,items/snippet/description,items/contentDetails/duration", videoId, strGoogleApiKey);
			urlData data;
			if (URLReadData(url.GetString(), data)) {
				rapidjson::Document d;
				if (!d.Parse(data.data()).HasParseError()) {
					const auto& items = d.FindMember("items");
					if (items == d.MemberEnd()
							|| !items->value.IsArray()
							|| items->value.Empty()) {
						return false;
					}

					const auto& value0 = items->value[0];
					if (auto snippet = GetJsonObject(value0, "snippet")) {
						if (getJsonValue(*snippet, "title", y_fields.title)) {
							y_fields.title = FixHtmlSymbols(y_fields.title);
						}

						getJsonValue(*snippet, "channelTitle", y_fields.author);

						if (getJsonValue(*snippet, "description", y_fields.content)) {
							std::wregex rgx(LR"(\r\n|\r|\n)");
							std::wstring wstr = std::regex_replace(y_fields.content.GetString(), rgx, L"\r\n");
							y_fields.content = wstr.c_str();
						}

						CStringA publishedAt;
						if (getJsonValue(*snippet, "publishedAt", publishedAt)) {
							WORD y, m, d;
							WORD hh, mm, ss;
							if (sscanf_s(publishedAt.GetString(), "%04hu-%02hu-%02huT%02hu:%02hu:%02hu", &y, &m, &d, &hh, &mm, &ss) == 6) {
								y_fields.dtime.wYear   = y;
								y_fields.dtime.wMonth  = m;
								y_fields.dtime.wDay    = d;
								y_fields.dtime.wHour   = hh;
								y_fields.dtime.wMinute = mm;
								y_fields.dtime.wSecond = ss;
							}
						}
					}

					if (auto contentDetails = GetJsonObject(value0, "contentDetails")) {
						CStringA duration;
						if (getJsonValue(*contentDetails, "duration", duration)) {
							const std::regex regex("PT(\\d+H)?(\\d{1,2}M)?(\\d{1,2}S)?", std::regex_constants::icase);
							std::cmatch match;
							if (std::regex_search(duration.GetString(), match, regex) && match.size() == 4) {
								int h = 0;
								int m = 0;
								int s = 0;
								if (match[1].matched) {
									h = atoi(match[1].first);
								}
								if (match[2].matched) {
									m = atoi(match[2].first);
								}
								if (match[3].matched) {
									s = atoi(match[3].first);
								}

								y_fields.duration = (h * 3600 + m * 60 + s) * UNITS;
							}
						}
					}
				}

				return true;
			}
#endif
		}

		return false;
	}

	bool Parse_Playlist(CString url, CFileItemList& youtubePlaylist, int& idx_CurrentPlay)
	{
		idx_CurrentPlay = 0;
		youtubePlaylist.clear();
		if (CheckYtPlaylistURL(url)) {
#if !USE_GOOGLE_API
			HandleURL(url);

			auto channelId = RegExpParse(url.GetString(), L"www.youtube.com/(?:channel/|c/|user/|@)([^/?&#]+)");
			if (!channelId.IsEmpty()) {
				if (url.Find(L"/live") != -1) {
					urlData data;
					if (URLReadData(url.GetString(), data)) {
						auto jsonEntry = GetEntry(data.data(), MATCH_PLAYER_RESPONSE, MATCH_PLAYER_RESPONSE_END);
						if (!jsonEntry.IsEmpty()) {
							jsonEntry += "}";

							rapidjson::Document json;
							if (!json.Parse(jsonEntry).HasParseError()) {
								if (auto videoDetails = GetJsonObject(json, "videoDetails")) {
									CString videoId;
									if (getJsonValue(*videoDetails, "videoId", videoId)) {
										auto url = L"https://www.youtube.com/watch?v=" + videoId;
										CString title;
										getJsonValue(*videoDetails, "title", title);

										youtubePlaylist.emplace_back(url, title, 0);
										idx_CurrentPlay = 0;
										return true;
									}
								}
							}
						}
					}
				}

				if (url.Find(YOUTUBE_CHANNEL_URL) == -1) {
					DLog(L"Youtube::Parse_Playlist() : downloading user page '%s'", url);

					urlData data;
					if (URLReadData(url.GetString(), data)) {
						channelId = UTF8ToWStr(GetEntry(data.data(), R"(content="https://www.youtube.com/channel/)", R"(")"));
						if (channelId.IsEmpty()) {
							channelId = RegExpParse(data.data(), R"(\\?"channelId\\?":\\?"([-a-zA-Z0-9_]+)\\?)");
						}
					}
				}

				if (StartsWith(channelId, L"UC")) {
					const CString playlistId = L"UU" + channelId.Right(channelId.GetLength() - 2);
					url.Format(L"https://www.youtube.com/playlist?list=%s", playlistId);
				} else {
					DLog(L"Youtube::Parse_Playlist() : unsupported user/channel page '%s'", url);
				}
			}

			const auto videoIdCurrent = RegExpParse(url.GetString(), videoIdRegExp);
			const auto playlistId = RegExpParse(url.GetString(), L"list=([-a-zA-Z0-9_]+)");
			if (playlistId.IsEmpty()) {
				return false;
			}

			const auto plId_type = playlistId.Left(2);
			const auto extract_mix = plId_type == L"RD" || plId_type == L"UL" || plId_type == L"PU";
			if (extract_mix) {
				DLog(L"Youtube::Parse_Playlist() : mixed playlist");
				url.Format(L"https://www.youtube.com/watch?v=%s&list=%s", videoIdCurrent, playlistId);
			} else {
				url.Format(L"https://www.youtube.com/playlist?list=%s", playlistId);
			}

			unsigned index = 0;
			DLog(L"Youtube::Parse_Playlist() : downloading #%u playlist '%s'", ++index, url);

			urlData data;
			if (!URLReadData(url.GetString(), data)) {
				return false;
			}
			CStringA dataStr = data.data();

			while (!dataStr.IsEmpty()) {
				CStringA jsonEntry = GetEntry(dataStr.GetString(), "ytInitialData = ", "};");
				dataStr.Empty();
				if (!jsonEntry.IsEmpty()) {
					jsonEntry += "}";

					rapidjson::Document json;
					if (!json.Parse(jsonEntry).HasParseError()) {
						auto contents = GetValueByPointer(json, "/contents/twoColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents/0/itemSectionRenderer/contents");
						if (!contents) {
							contents = GetValueByPointer(json, "/contents/twoColumnWatchNextResults/secondaryResults/secondaryResults/results/0/itemSectionRenderer/contents");
						}

						CString lastvideoId;
						auto ParseJsonObject = [&](const auto& object) {
							CString contentId;
							getJsonValue(object, "contentId", contentId);
							if (!contentId.IsEmpty()) {
								if (auto content = GetValueByPointer(object, "/metadata/lockupMetadataViewModel/title/content"); content && content->IsString()) {
									CString title = UTF8ToWStr(content->GetString());
									CString url;
									url.Format(L"https://www.youtube.com/watch?v=%s", contentId.GetString());

									auto it = std::find_if(youtubePlaylist.cbegin(), youtubePlaylist.cend(), [&url](const CFileItem& item) {
										return item.GetPath() == url;
									});
									if (it == youtubePlaylist.cend()) {
										lastvideoId = contentId;

										REFERENCE_TIME duration = 0;
										if (auto durationText = GetValueByPointer(object, "/contentImage/thumbnailViewModel/overlays/0/thumbnailBottomOverlayViewModel/badges/0/thumbnailBadgeViewModel/text"); durationText && durationText->IsString()) {
											int t1 = 0;
											int t2 = 0;
											int t3 = 0;

											if (const auto ret = sscanf_s(durationText->GetString(), "%02d:%02d:%02d", &t1, &t2, &t3)) {
												if (ret == 3) {
													duration = (((60LL * t1) + t2) * 60LL + t3) * UNITS;
												} else if (ret == 2) {
													duration = ((60LL * t1) + t2) * UNITS;
												}
											}
										}

										youtubePlaylist.emplace_back(url, title, duration);

										if (contentId == videoIdCurrent) {
											idx_CurrentPlay = youtubePlaylist.size() - 1;
										}
									}
								}
							}
						};

						if (contents && contents->IsArray()) {
							if (contents && contents->IsArray()) {
								for (const auto& item : contents->GetArray()) {
									if (auto lockupViewModel = GetJsonObject(item, "lockupViewModel")) {
										ParseJsonObject(*lockupViewModel);
									}
								}
							}
						}

						if (!lastvideoId.IsEmpty()) {
							url.Format(L"https://www.youtube.com/watch?v=%s&list=%s", lastvideoId, playlistId);
							DLog(L"Youtube::Parse_Playlist() : downloading #%u playlist '%s'", ++index, url);
							if (URLReadData(url.GetString(), data)) {
								dataStr = data.data();
							}
						}
					}
				}
			}

			if (!youtubePlaylist.empty()) {
				return true;
			}

			CStringA moreStr;
			dataStr.Empty();

			index = 0;
			url += L"&disable_polymer=true";
			DLog(L"Youtube::Parse_Playlist() : downloading #%u playlist '%s'", ++index, url);
			if (URLReadData(url.GetString(), data)) {
				dataStr = data.data();
				if (!extract_mix) {
					moreStr = dataStr;
				}
			}

			while (!dataStr.IsEmpty()) {
				CString lastvideoId;

				LPCSTR sMatch = nullptr;
				if (strstr(dataStr.GetString(), MATCH_PLAYLIST_ITEM_START)) {
					sMatch = MATCH_PLAYLIST_ITEM_START;
				} else if (strstr(dataStr.GetString(), MATCH_PLAYLIST_ITEM_START2)) {
					sMatch = MATCH_PLAYLIST_ITEM_START2;
				} else {
					break;
				}

				LPCSTR block = dataStr.GetString();
				while ((block = strstr(block, sMatch)) != nullptr) {
					const CStringA blockEntry = GetEntry(block, sMatch, ">");
					if (blockEntry.IsEmpty()) {
						break;
					}

					block += blockEntry.GetLength();
					CString item = UTF8ToWStr(blockEntry);
					CString videoId;
					CString title;

					const std::wregex regex(L"([a-z-]+)=\"([^\"]+)\"");
					std::wcmatch match;
					LPCWSTR text = item.GetString();
					while (std::regex_search(text, match, regex)) {
						if (match.size() == 3) {
							const CString propHeader(match[1].first, match[1].length());
							const CString propValue(match[2].first, match[2].length());

							if (propHeader == L"data-video-id") {
								videoId = propValue;
							} else if (propHeader == L"data-video-title" || propHeader == L"data-title") {
								title = FixHtmlSymbols(propValue);
							}
						}

						text = match[0].second;
					}

					if (!videoId.IsEmpty()) {
						CString url; url.Format(L"https://www.youtube.com/watch?v=%s", videoId);
						auto it = std::find_if(youtubePlaylist.cbegin(), youtubePlaylist.cend(), [&url](const CFileItem& item) {
							return item.GetPath() == url;
						});
						if (it == youtubePlaylist.cend()) {
							lastvideoId = videoId;
							youtubePlaylist.emplace_back(url, title);

							if (videoId == videoIdCurrent) {
								idx_CurrentPlay = youtubePlaylist.size() - 1;
							}
						}
					}
				}

				dataStr.Empty();

				if (extract_mix) {
					if (lastvideoId.IsEmpty()) {
						break;
					}

					url.Format(L"https://www.youtube.com/watch?v=%s&list=%s&disable_polymer=true", lastvideoId, playlistId);
					DLog(L"Youtube::Parse_Playlist() : downloading #%u playlist '%s'", ++index, url);
					if (URLReadData(url.GetString(), data)) {
						dataStr = data.data();
					}

					continue;
				} else {
					auto moreUrl = UrlDecode(RegExpParse(moreStr.GetString(), R"(data-uix-load-more-href="/?([^"]+)\")"));
					moreStr.Empty();
					if (!moreUrl.IsEmpty()) {
						moreUrl.Replace("&amp;", "&"); moreUrl += "&disable_polymer=true";
						url.Format(L"https://www.youtube.com/%hs", moreUrl);

						DLog(L"Youtube::Parse_Playlist() : downloading #%u playlist '%s'", ++index, url);
						if (URLReadData(url.GetString(), data, L"x-youtube-client-name: 1\r\nx-youtube-client-version: 1.20200609.04.02\r\n")) {
							rapidjson::Document json;
							if (!json.Parse(data.data()).HasParseError()) {
								getJsonValue(json, "content_html", dataStr);
								getJsonValue(json, "load_more_widget_html", moreStr);
							}
						}
					}
				}
			}

			return !youtubePlaylist.empty();

#else
			const CString videoIdCurrent = RegExpParse(url.GetString(), videoIdRegExp);
			const CString playlistId = RegExpParse(url.GetString(), L"list=([-a-zA-Z0-9_]+)");
			if (playlistId.IsEmpty()) {
				return false;
			}

			CString nextToken;
			urlData data;

			for (;;) {
				CString link;
				link.Format(L"https://www.googleapis.com/youtube/v3/playlistItems?part=snippet&playlistId=%s&key=%s&maxResults=50", playlistId.GetString(), strGoogleApiKey);
				if (!nextToken.IsEmpty()) {
					link.AppendFormat(L"&pageToken=%s", nextToken.GetString());
					nextToken.Empty();
				}

				if (!URLReadData(link.GetString(), data)) {
					return false;
				}

				rapidjson::Document d;
				if (!d.Parse(data.data()).HasParseError()) {
					const auto& items = d.FindMember("items");
					if (items == d.MemberEnd()
							|| !items->value.IsArray()
							|| items->value.Empty()) {
						return false;
					}

					getJsonValue(d, "nextPageToken", nextToken);

					for (const auto& item : items->value.GetArray()) {
						if (auto snippet = GetJsonObject(item, "snippet")) {
							if (auto resourceId = GetJsonObject(*snippet, "resourceId")) {
								CString videoId;
								if (getJsonValue(*resourceId, "videoId", videoId)) {
									CString url;
									url.Format(L"https://www.youtube.com/watch?v=%s", videoId.GetString());

									CString title;
									if (getJsonValue(*snippet, "title", title)) {
										title = FixHtmlSymbols(title);
									}

									youtubePlaylist.emplace_back(url, title);

									if (videoIdCurrent == videoId) {
										idx_CurrentPlay = youtubePlaylist.size() - 1;
									}
								}
							}
						}
					}
				}

				if (nextToken.IsEmpty()) {
					break;
				}
			}

			if (!youtubePlaylist.empty()) {
				return true;
			}
#endif
		}

		return false;
	}

	bool ParseMetadata(CString url, YoutubeFields& y_fields)
	{
		bool bRet = false;

		if (CheckYtVideoURL(url)) {
			HandleURL(url);
			const CString videoId = RegExpParse(url.GetString(), videoIdRegExp);

			bRet = ParseMetadataLocal(videoId, y_fields);
		}

		return bRet;
	}
}
