/*
 * (C) 2012-2024 see Authors.txt
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

	static const YoutubeProfile* GetProfile(int iTag)
	{
		for (const auto& profile : YProfiles) {
			if (iTag == profile.iTag) {
				return &profile;
			}
		}

		return nullptr;
	}

	static const YoutubeProfile* GetAudioProfile(int iTag)
	{
		for (const auto& profile : YAudioProfiles) {
			if (iTag == profile.iTag) {
				return &profile;
			}
		}

		return nullptr;
	}

	static bool CompareProfile(const YoutubeProfile* a, const YoutubeProfile* b)
	{
		if (a->format != b->format) {
			return (a->format < b->format);
		}

		if (a->quality != b->quality) {
			return (a->quality > b->quality);
		}

		if (a->fps60 != b->fps60) {
			return (a->fps60 > b->fps60);
		}

		if (a->type != b->type) {
			return (a->type < b->type);
		}

		return (a->hdr > b->hdr);
	}

	static bool CompareUrllistItem(const YoutubeUrllistItem& a, const YoutubeUrllistItem& b)
	{
		return CompareProfile(a.profile, b.profile);
	}

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

		return {};
	}

	bool CheckURL(CString url)
	{
		url.MakeLower();

		if (url.Find(YOUTUBE_URL) != -1
				|| url.Find(YOUTUBE_URL_A) != -1
				|| url.Find(YOUTUBE_URL_V) != -1
				|| url.Find(YOUTUBE_URL_EMBED) != -1
				|| url.Find(YOUTUBE_URL_SHORTS) != -1
				|| url.Find(YOUTUBE_URL_CLIP) != -1
				|| url.Find(YOUTUBE_URL_LIVE) != -1
				|| url.Find(YOUTU_BE_URL) != -1) {
			return true;
		}

		return false;
	}

	bool CheckPlaylist(CString url)
	{
		url.MakeLower();

		if (url.Find(YOUTUBE_PL_URL) != -1
				|| url.Find(YOUTUBE_USER_URL) != -1
				|| url.Find(YOUTUBE_CHANNEL_URL) != -1
				|| url.Find(YOUTUBE_USER_SHORT_URL) != -1
				|| url.Find(YOUTUBE_URL_LIBRARY) != -1
				|| (url.Find(YOUTUBE_URL) != -1 && url.Find(L"&list=") != -1)
				|| (url.Find(YOUTUBE_URL_A) != -1 && url.Find(L"/watch_videos?video_ids") != -1)
				|| ((url.Find(YOUTUBE_URL_V) != -1 || url.Find(YOUTUBE_URL_EMBED) != -1
					 || url.Find(YOUTU_BE_URL) != -1 || url.Find(YOUTUBE_URL_SHORTS) != -1) && url.Find(L"list=") != -1)) {
			return true;
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

	static bool URLPostData(LPCWSTR lpszAgent, const CStringW& headers, CStringA& requestData, urlData& pData)
	{
		if (auto hInet = InternetOpenW(lpszAgent, INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0)) {
			if (auto hSession = InternetConnectW(hInet, L"www.youtube.com", 443, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 1)) {
				if (auto hRequest = HttpOpenRequestW(hSession,
													 L"POST",
													 L"youtubei/v1/player", nullptr, nullptr, nullptr,
													 INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 1)) {

					if (HttpSendRequestW(hRequest, headers.GetString(), headers.GetLength(),
										 reinterpret_cast<LPVOID>(requestData.GetBuffer()), requestData.GetLength())) {
						pData.clear();
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
							return true;
						}
					}

					InternetCloseHandle(hRequest);
				}
				InternetCloseHandle(hSession);
			}
			InternetCloseHandle(hInet);
		}

		return false;
	}

	static bool URLPostData(LPCWSTR videoId, urlData& pData)
	{
		constexpr auto requestStr =
			R"({"contentCheckOk": true, "context": {"client": {"clientName": "IOS", "clientVersion": "19.45.4", "deviceMake": "Apple", "deviceModel": "iPhone16,2", "hl": "en", "osName": "iPhone", "osVersion": "18.1.0.22B83", "timeZone": "UTC", "utcOffsetMinutes": 0}}, "playbackContext": {"contentPlaybackContext": {"html5Preference": "HTML5_PREF_WANTS"}}, "racyCheckOk" : true, "videoId" : "%S"})";

		static const CStringW headers =
			LR"(X-YouTube-Client-Name: 5\r\n)"
			LR"(X-YouTube-Client-Version: 19.45.4\r\n)"
			LR"(Origin: https://www.youtube.com\r\n)"
			LR"(content-type: application/json\r\n)";

		CStringA requestData;
		requestData.Format(requestStr, videoId);

		return URLPostData(L"com.google.ios.youtube/19.45.4 (iPhone16,2; U; CPU iOS 18_1_0 like Mac OS X;)", headers, requestData, pData);
	}

	static bool URLPostDataForLive(LPCWSTR videoId, urlData& pData)
	{
		constexpr auto requestStr =
			R"({"contentCheckOk": true, "context": {"client": {"clientName": "MWEB", "clientVersion": "2.20240726.01.00", "hl": "en", "timeZone": "UTC", "utcOffsetMinutes": 0}}, "playbackContext": {"contentPlaybackContext": {"html5Preference": "HTML5_PREF_WANTS"}}, "racyCheckOk": true, "videoId": "%S"})";

		static const CStringW headers =
			LR"(X-YouTube-Client-Name: 2\r\n)"
			LR"(X-YouTube-Client-Version: 2.20240726.01.00\r\n)"
			LR"(Origin: https://www.youtube.com\r\n)"
			LR"(content-type: application/json\r\n)";

		CStringA requestData;
		requestData.Format(requestStr, videoId);

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
		url = UrlDecode(url);

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

	static bool ParseMetadata(const CString& videoId, YoutubeFields& y_fields)
	{
		if (!videoId.IsEmpty()) {
#if !USE_GOOGLE_API
			bool bParse = false;
			urlData data;
			if (URLPostData(videoId.GetString(), data)) {
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

	enum youtubeFuncType {
		funcNONE = -1,
		funcDELETE,
		funcREVERSE,
		funcSWAP,
		funcLAST
	};


	static const YoutubeUrllistItem* SelectVideoStream(YoutubeUrllist& youtubeUrllist)
	{
		const CAppSettings& s = AfxGetAppSettings();
		const YoutubeUrllistItem* final_item = nullptr;

		for (;;) {
			if (youtubeUrllist.empty()) {
				DLog(L"Youtube::Parse_URL() : no output video format found");

				return nullptr;
			}

			int k_mp4 = -1;
			int k_webm = -1;
			int k_av1 = -1;
			for (int i = 0; i < (int)youtubeUrllist.size(); i++) {
				const auto& format = youtubeUrllist[i].profile->format;
				if (k_mp4 < 0 && format == y_mp4_avc) {
					k_mp4 = i;
				}
				else if (k_webm < 0 && format == y_webm_vp9) {
					k_webm = i;
				}
				else if (k_av1 < 0 && format == y_mp4_av1) {
					k_av1 = i;
				}
			}

			size_t k = 0;
			if (s.YoutubeFormat.fmt == y_mp4_avc) {
				k = (k_mp4 >= 0) ? k_mp4 : (k_webm >= 0) ? k_webm : 0;
			}
			else if (s.YoutubeFormat.fmt == y_webm_vp9) {
				k = (k_webm >= 0) ? k_webm : (k_mp4 >= 0) ? k_mp4 : 0;
			}
			else if (s.YoutubeFormat.fmt == y_mp4_av1) {
				k = (k_av1 >= 0) ? k_av1 : (k_webm >= 0) ? k_webm : 0;
			}
			final_item = &youtubeUrllist[k];

			for (size_t i = k + 1; i < youtubeUrllist.size(); i++) {
				const auto profile = youtubeUrllist[i].profile;

				if (final_item->profile->format == profile->format) {
					if (profile->quality == final_item->profile->quality) {
						if (profile->fps60 != s.YoutubeFormat.fps60) {
							// same resolution as that of the previous, but not suitable fps
							continue;
						}
						if (profile->hdr != s.YoutubeFormat.hdr) {
							// same resolution as that of the previous, but not suitable HDR
							continue;
						}
					}

					if (profile->quality < final_item->profile->quality && final_item->profile->quality <= s.YoutubeFormat.res) {
						break;
					}

					final_item = &youtubeUrllist[i];
				}
			}

			const auto& url = final_item->url;
			CHTTPAsync HTTPAsync;
			if (FAILED(HTTPAsync.Connect(url, http::connectTimeout))) {
				DLog(L"Youtube::Parse_URL() : failed to connect \"%s\", skip", url.GetString());
				auto it = std::find_if(youtubeUrllist.cbegin(), youtubeUrllist.cend(),
					[&url](const YoutubeUrllistItem& item) { return item.url == url; });
				youtubeUrllist.erase(it);
				final_item = nullptr;

				continue;
			}

			break;
		}

		return final_item;
	}


	static const YoutubeUrllistItem* SelectAudioStream(YoutubeUrllist& youtubeAudioUrllist)
	{
		const CAppSettings& s = AfxGetAppSettings();
		const YoutubeUrllistItem* final_item = nullptr;

		for (;;) {
			if (youtubeAudioUrllist.empty()) {
				DLog(L"Youtube::Parse_URL() : no output audio format found");

				return nullptr;
			}

			int k_aac = -1;
			int k_opus = -1;
			for (int i = 0; i < (int)youtubeAudioUrllist.size(); i++) {
				const auto& format = youtubeAudioUrllist[i].profile->format;
				if (k_aac < 0 && format == y_mp4_aac) {
					k_aac = i;
				}
				else if (k_opus < 0 && format == y_webm_aud) {
					k_opus = i;
				}
			}

			size_t k = 0;
			if (s.YoutubeFormat.fmt == y_mp4_avc || s.YoutubeFormat.fmt == y_mp4_av1) {
				k = (k_aac >= 0) ? k_aac : (k_opus >= 0) ? k_opus : 0;
			}
			else if (s.YoutubeFormat.fmt == y_webm_vp9) {
				k = (k_opus >= 0) ? k_opus : (k_aac >= 0) ? k_aac : 0;
			}
			final_item = &youtubeAudioUrllist[k];

			const auto& url = final_item->url;
			CHTTPAsync HTTPAsync;
			if (FAILED(HTTPAsync.Connect(url, http::connectTimeout))) {
				DLog(L"Youtube::Parse_URL() : failed to connect \"%s\", skip", url.GetString());
				auto it = std::find_if(youtubeAudioUrllist.cbegin(), youtubeAudioUrllist.cend(),
					[&url](const YoutubeUrllistItem& item) { return item.url == url; });
				youtubeAudioUrllist.erase(it);
				final_item = nullptr;

				continue;
			}

			break;
		}

		return final_item;
	}


	bool Parse_URL(
		CStringW url,           // input parameter
		REFERENCE_TIME rtStart, // input parameter
		YoutubeFields& y_fields,
		YoutubeUrllist& youtubeUrllist,
		YoutubeUrllist& youtubeAudioUrllist,
		OpenFileData* pOFD,
		CStringW& errorMessage)
	{
		if (!CheckURL(url)) {
			return false;
		}

		DLog(L"Youtube::Parse_URL() : \"%s\"", url);

		HandleURL(url);
		url += L"&gl=US&hl=en&has_verified=1&bpctr=9999999999";

		CString videoId = RegExpParse(url.GetString(), videoIdRegExp);

		if (rtStart <= 0) {
			bool bMatch = false;

			const std::wregex regex(L"t=(\\d+h)?(\\d{1,2}m)?(\\d{1,2}s)?", std::regex_constants::icase);
			std::wcmatch match;
			if (std::regex_search(url.GetBuffer(), match, regex) && match.size() == 4) {
				int h = 0;
				int m = 0;
				int s = 0;
				if (match[1].matched) {
					h = _wtoi(match[1].first);
					bMatch = true;
				}
				if (match[2].matched) {
					m = _wtoi(match[2].first);
					bMatch = true;
				}
				if (match[3].matched) {
					s = _wtoi(match[3].first);
					bMatch = true;
				}

				rtStart = (h * 3600 + m * 60 + s) * UNITS;
			}

			if (!bMatch) {
				const CString timeStart = RegExpParse(url.GetString(), L"(?:t|time_continue)=([0-9]+)");
				if (!timeStart.IsEmpty()) {
					rtStart = _wtol(timeStart) * UNITS;
				}
			}
		}

		const auto& s = AfxGetAppSettings();

		urlData data;
		URLReadData(url.GetString(), data);

		pOFD->rtStart = rtStart;

		CStringA strUrls;
		std::list<CStringA> strUrlsLive;

		rapidjson::Document player_response_jsonDocument;
		urlData postData;
		if (URLPostData(videoId.GetString(), postData)) {
			player_response_jsonDocument.Parse(postData.data());
		}

		bool bTryAgainLiveStream = true;
		for (;;) {
			using streamingDataFormat = std::tuple<int, CStringA, CStringA, CStringA>;
			std::list<streamingDataFormat> streamingDataFormatList;
			std::map<CStringA, std::list<streamingDataFormat>> streamingDataFormatListAudioWithLanguages;
			if (!player_response_jsonDocument.IsNull()) {
				if (auto playabilityStatus = GetJsonObject(player_response_jsonDocument, "playabilityStatus")) {
					CStringA status;
					if (getJsonValue(*playabilityStatus, "status", status) && status != "OK") {
						if (getJsonValue(*playabilityStatus, "reason", errorMessage) && !errorMessage.IsEmpty()) {
							errorMessage = L"Youtube : " + errorMessage;
						}
						return false;
					}
				}

				if (auto videoDetails = GetJsonObject(player_response_jsonDocument, "videoDetails")) {
					bool isLive = false;
					if (getJsonValue(*videoDetails, "isLive", isLive) && isLive) {
						if (auto streamingData = GetJsonObject(player_response_jsonDocument, "streamingData")) {
							CString hlsManifestUrl;
							if (getJsonValue(*streamingData, "hlsManifestUrl", hlsManifestUrl)) {
								DLog(L"Youtube::Parse_URL() : Downloading m3u8 hls manifest \"%s\"", hlsManifestUrl);
								urlData m3u8Data;
								if (URLReadData(hlsManifestUrl.GetString(), m3u8Data)) {
									CStringA m3u8Str(m3u8Data.data());

									m3u8Str.Replace("\r\n", "\n");
									std::list<CStringA> lines;
									Explode(m3u8Str, lines, '\n');
									for (auto& line : lines) {
										line.Trim();
										if (line.IsEmpty() || (line.GetAt(0) == '#')) {
											continue;
										}

										line.Replace("/keepalive/yes/", "/");
										strUrlsLive.emplace_back(line);
									}
								}
							}
						}
					}
				}

				if (auto streamingData = GetJsonObject(player_response_jsonDocument, "streamingData")) {
					if (auto formats = GetJsonArray(*streamingData, "formats")) {
						for (const auto& format : formats->GetArray()) {
							streamingDataFormat element;

							getJsonValue(format, "itag", std::get<0>(element));
							getJsonValue(format, "qualityLabel", std::get<3>(element));
							if (getJsonValue(format, "url", std::get<1>(element))) {
								streamingDataFormatList.emplace_back(element);
							} else if (getJsonValue(format, "cipher", std::get<2>(element)) || getJsonValue(format, "signatureCipher", std::get<2>(element))) {
								streamingDataFormatList.emplace_back(element);
							}
						}
					}

					if (auto adaptiveFormats = GetJsonArray(*streamingData, "adaptiveFormats")) {
						for (const auto& adaptiveFormat : adaptiveFormats->GetArray()) {
							CStringA type;
							if (getJsonValue(adaptiveFormat, "type", type) && type == L"FORMAT_STREAM_TYPE_OTF") {
								// fragmented url
								continue;
							}

							streamingDataFormat element;

							getJsonValue(adaptiveFormat, "itag", std::get<0>(element));
							getJsonValue(adaptiveFormat, "qualityLabel", std::get<3>(element));
							if (getJsonValue(adaptiveFormat, "url", std::get<1>(element))
								|| getJsonValue(adaptiveFormat, "cipher", std::get<2>(element)) || getJsonValue(adaptiveFormat, "signatureCipher", std::get<2>(element))) {

								if (std::get<1>(element).Find("xtags=drc") > 0) {
									// DRC audio
									continue;
								}

								CStringA lang_id;
								if (auto audioTrack = GetJsonObject(adaptiveFormat, "audioTrack")) {
									if (getJsonValue(*audioTrack, "id", lang_id)) {
										auto pos = lang_id.Find('.');
										if (pos != -1) {
											lang_id = lang_id.Left(pos);
										}
									}
								}

								if (lang_id.IsEmpty()) {
									streamingDataFormatList.emplace_back(element);
								} else {
									streamingDataFormatListAudioWithLanguages[lang_id].emplace_back(element);
								}
							}
						}
					}
				}
			}

			if (!streamingDataFormatListAudioWithLanguages.empty()) {
				// Removing existing audio formats without "language" tag.
				for (auto it = streamingDataFormatList.begin(); it != streamingDataFormatList.end();) {
					auto itag = std::get<0>(*it);
					if (auto audioprofile = GetAudioProfile(itag)) {
						it = streamingDataFormatList.erase(it);
					} else {
						++it;
					}
				}

				auto it = streamingDataFormatListAudioWithLanguages.find("en");
				if (!s.strYoutubeAudioLang.IsEmpty()) {
					it = streamingDataFormatListAudioWithLanguages.find(WStrToUTF8(s.strYoutubeAudioLang.GetString()));
					if (it == streamingDataFormatListAudioWithLanguages.end()) {
						it = streamingDataFormatListAudioWithLanguages.find("en");
					}
				}

				if (it == streamingDataFormatListAudioWithLanguages.end()) {
					it = streamingDataFormatListAudioWithLanguages.begin();
				}

				streamingDataFormatList.splice(streamingDataFormatList.end(), it->second);
			};

			auto AddUrl = [](YoutubeUrllist& videoUrls, YoutubeUrllist& audioUrls, const CString& url, const int itag, const int fps = 0, LPCSTR quality_label = nullptr) {
				if (url.Find(L"dur=0.000") > 0) {
					return;
				}

				if (const YoutubeProfile* profile = GetProfile(itag)) {
					YoutubeUrllistItem item;
					item.profile = profile;
					item.url = url;

					switch (profile->format) {
						case y_mp4_avc:  item.title = L"MP4(H.264)"; break;
						case y_webm_vp9: item.title = L"WebM(VP9)";  break;
						case y_mp4_av1:  item.title = L"MP4(AV1)";   break;
						case y_stream:   item.title = L"HLS Live";  break;
						default:         item.title = L"unknown";   break;
					}

					if (quality_label) {
						item.title.AppendFormat(L" %S", quality_label);
						if (profile->type == y_video) {
							item.title.Append(L" dash");
						}
						if (profile->hdr) {
							item.title.Append(L" (10 bit)");
						}
					} else {
						item.title.AppendFormat(L"%dp", profile->quality);
						if (profile->type == y_video) {
							item.title.Append(L" dash");
						}
						if (fps) {
							item.title.AppendFormat(L" %dfps", fps);
						} else if (profile->fps60) {
							item.title.Append(L" 60fps");
						}
						if (profile->hdr) {
							item.title.Append(L" HDR (10 bit)");
						}
					}

					videoUrls.emplace_back(item);
				} else if (const YoutubeProfile* audioprofile = GetAudioProfile(itag)) {
					YoutubeUrllistItem item;
					item.profile = audioprofile;
					item.url = url;

					switch (audioprofile->format) {
						case y_mp4_aac:  item.title = L"MP4/AAC";         break;
						case y_webm_aud: item.title = L"WebM/Opus";       break;
						case y_mp4_ac3:  item.title = L"MP4/AC3";         break;
						case y_mp4_eac3: item.title = L"MP4/E-AC3";       break;
						case y_mp4_dtse: item.title = L"MP4/DTS-Express"; break;
						default:         item.title = L"unknown";         break;
					}
					item.title.AppendFormat(L" %dkbit/s", audioprofile->quality);

					audioUrls.emplace_back(item);
				}
			};

			std::vector<std::pair<youtubeFuncType, int>> JSFuncs;
			bool bJSParsed = false;

			auto SignatureDecode = [&](CStringA& url, CStringA signature, LPCSTR format) {
				if (!signature.IsEmpty()) {
					if (!bJSParsed) {
						bJSParsed = true;

						CString JSUrl = UTF8ToWStr(GetEntry(data.data(), MATCH_JS_START, MATCH_END));
						if (JSUrl.IsEmpty()) {
							JSUrl = UTF8ToWStr(GetEntry(data.data(), MATCH_JS_START_2, MATCH_END));
							if (JSUrl.IsEmpty()) {
								JSUrl = UTF8ToWStr(GetEntry(data.data(), MATCH_JS_START_3, MATCH_END));
							}
						}

						if (!JSUrl.IsEmpty()) {
							JSUrl.Replace(L"\\/", L"/");
							JSUrl.Trim();

							if (StartsWith(JSUrl, L"//")) {
								JSUrl = L"https:" + JSUrl;
							} else if (JSUrl.Find(L"http://") == -1 && JSUrl.Find(L"https://") == -1) {
								JSUrl = L"https://www.youtube.com" + JSUrl;
							}
						}

						if (JSUrl.IsEmpty()) {
							return;
						}

						const auto JSPlayerId = RegExpParse(JSUrl.GetString(), LR"(/s/player/([a-zA-Z0-9_-]{8,})/player)");

						auto& youtubeSignatureCache = AfxGetAppSettings().youtubeSignatureCache;
						const auto& it = youtubeSignatureCache.find(JSPlayerId);
						if (it != youtubeSignatureCache.cend() && !it->second.IsEmpty()) {
							rapidjson::GenericDocument<rapidjson::UTF16<>> d;
							if (!d.Parse(it->second.GetString()).HasParseError()) {
								const auto maxNum = signature.GetLength() - 1;
								for (auto it = d.MemberBegin(); it != d.MemberEnd(); ++it) {
									int funcType;
									if (StrToInt32(it->name.GetString(), funcType) && (funcType > youtubeFuncType::funcNONE && funcType < youtubeFuncType::funcLAST)) {
										const auto value = it->value.GetInt();
										if (value < 0 || value > maxNum) {
											JSFuncs.clear();
											break;
										}
										JSFuncs.emplace_back(static_cast<youtubeFuncType>(funcType), it->value.GetInt());
									}
								}
							}
						}

						if (JSFuncs.empty()) {
							urlData jsData;
							if (URLReadData(JSUrl.GetString(), jsData)) {
								static LPCSTR signatureRegExps[] = {
									R"(\b[cs]\s*&&\s*[adf]\.set\([^,]+\s*,\s*encodeURIComponent\s*\(\s*([a-zA-Z0-9$]+)\()",
									R"(\b[a-zA-Z0-9]+\s*&&\s*[a-zA-Z0-9]+\.set\([^,]+\s*,\s*encodeURIComponent\s*\(\s*([a-zA-Z0-9$]+)\()",
									R"((?:\b|[^a-zA-Z0-9$])([a-zA-Z0-9$]{2})\s*=\s*function\(\s*a\s*\)\s*\{\s*a\s*=\s*a\.split\(\s*""\s*\))",
									R"(([a-zA-Z0-9$]+)\s*=\s*function\(\s*a\s*\)\s*\{\s*a\s*=\s*a\.split\(\s*""\s*\))",
									R"((["\'])signature\1\s*,\s*([a-zA-Z0-9$]+)\()",
									R"(\.sig\|\|([a-zA-Z0-9$]+)\()",
									R"(yt\.akamaized\.net/\)\s*\|\|\s*.*?\s*[cs]\s*&&\s*[adf]\.set\([^,]+\s*,\s*(?:encodeURIComponent\s*\()?\s*([a-zA-Z0-9$]+)\()",
									R"(\b[cs]\s*&&\s*[adf]\.set\([^,]+\s*,\s*([a-zA-Z0-9$]+)\()",
									R"(\b[a-zA-Z0-9]+\s*&&\s*[a-zA-Z0-9]+\.set\([^,]+\s*,\s*([a-zA-Z0-9$]+)\()",
									R"(\bc\s*&&\s*a\.set\([^,]+\s*,\s*\([^)]*\)\s*\(\s*([a-zA-Z0-9$]+)\()",
									R"(\bc\s*&&\s*[a-zA-Z0-9]+\.set\([^,]+\s*,\s*\([^)]*\)\s*\(\s*([a-zA-Z0-9$]+)\()",
									R"(\bc\s*&&\s*[a-zA-Z0-9]+\.set\([^,]+\s*,\s*\([^)]*\)\s*\(\s*([a-zA-Z0-9$]+)\()"
								};
								CStringA funcName;
								for (const auto& sigRegExp : signatureRegExps) {
									funcName = RegExpParse(jsData.data(), sigRegExp);
									if (!funcName.IsEmpty()) {
										break;
									}
								}
								if (!funcName.IsEmpty()) {
									CStringA funcRegExp = funcName + "=function\\(a\\)\\{([^\\n]+)\\};"; funcRegExp.Replace("$", "\\$");
									const CStringA funcBody = RegExpParse(jsData.data(), funcRegExp.GetString());
									if (!funcBody.IsEmpty()) {
										CStringA funcGroup;
										std::list<CStringA> funcList;
										std::list<CStringA> funcCodeList;

										std::list<CStringA> code;
										Explode(funcBody, code, ';');

										for (const auto& line : code) {

											if (line.Find("split") >= 0 || line.Find("return") >= 0) {
												continue;
											}

											funcList.emplace_back(line);

											if (funcGroup.IsEmpty()) {
												const int k = line.Find('.');
												if (k > 0) {
													funcGroup = line.Left(k);
												}
											}
										}

										if (!funcGroup.IsEmpty()) {
											CStringA tmp; tmp.Format("var %s={", funcGroup);
											tmp = GetEntry(jsData.data(), tmp, "};");
											if (!tmp.IsEmpty()) {
												tmp.Remove('\n');
												Explode(tmp, funcCodeList, "},");
											}
										}

										if (!funcList.empty() && !funcCodeList.empty()) {
											for (const auto& func : funcList) {
												int funcArg = 0;
												const CStringA funcArgs = GetEntry(func, "(", ")");

												std::list<CStringA> args;
												Explode(funcArgs, args, ',');
												if (args.size() >= 1) {
													CStringA& arg = args.back();
													int value = 0;
													if (sscanf_s(arg, "%d", &value) == 1) {
														funcArg = value;
													}
												}

												CStringA funcName = GetEntry(func, funcGroup + '.', "(");
												if (funcName.IsEmpty()) {
													funcName = GetEntry(func, funcGroup, "(");
													if (funcName.IsEmpty()) {
														continue;
													}
												}
												if (funcName.Find("[") != -1) {
													funcName.Replace("[", ""); funcName.Replace("]", "");
												}
												funcName += ":function";

												auto funcType = youtubeFuncType::funcNONE;

												for (const auto& funcCode : funcCodeList) {
													if (funcCode.Find(funcName) >= 0) {
														if (funcCode.Find("splice") > 0) {
															funcType = youtubeFuncType::funcDELETE;
														} else if (funcCode.Find("reverse") > 0) {
															funcType = youtubeFuncType::funcREVERSE;
														} else if (funcCode.Find(".length]") > 0) {
															funcType = youtubeFuncType::funcSWAP;
														}
														break;
													}
												}

												if (funcType != youtubeFuncType::funcNONE) {
													JSFuncs.emplace_back(funcType, funcArg);
												}
											}
										}
									}
								}

								if (!JSFuncs.empty()) {
									CString buffer = L"{ ";
									for (auto it = JSFuncs.cbegin(); it != JSFuncs.cend(); ++it) {
										buffer.AppendFormat(L"\"%d\" : %d", it->first, it->second);

										if (it != std::prev(JSFuncs.cend())) {
											buffer += L", ";
										}
									}
									buffer += L" }";

									youtubeSignatureCache[JSPlayerId] = buffer;
								}
							}
						}
					}
				}

				if (!JSFuncs.empty()) {
					auto Delete = [](CStringA& a, const int b) {
						a.Delete(0, b);
					};
					auto Swap = [](CStringA& a, int b) {
						const CHAR c = a[0];
						b %= a.GetLength();
						a.SetAt(0, a[b]);
						a.SetAt(b, c);
					};
					auto Reverse = [](CStringA& a) {
						CHAR c;
						const int len = a.GetLength();

						for (int i = 0; i < len / 2; ++i) {
							c = a[i];
							a.SetAt(i, a[len - i - 1]);
							a.SetAt(len - i - 1, c);
						}
					};

					for (const auto [funcType, funcArg] : JSFuncs) {
						switch (funcType) {
							case youtubeFuncType::funcDELETE:
								Delete(signature, funcArg);
								break;
							case youtubeFuncType::funcSWAP:
								Swap(signature, funcArg);
								break;
							case youtubeFuncType::funcREVERSE:
								Reverse(signature);
								break;
						}
					}

					url.AppendFormat(format, signature);
				}
			};

			CStringA chaptersStr = GetEntry(data.data(), R"({"chapteredPlayerBarRenderer":)", "}}}]");
			if (chaptersStr.IsEmpty()) {
				chaptersStr = GetEntry(data.data(), R"("markersMap":[{"key":"DESCRIPTION_CHAPTERS","value":)", "}}}]");
			}
			if (!chaptersStr.IsEmpty()) {
				chaptersStr.Replace(R"(\/)", "/");
				chaptersStr.Replace(R"(\\)", R"(\)");
				chaptersStr += "}}}]}";

				rapidjson::Document d;
				if (!d.Parse(chaptersStr).HasParseError()) {
					if (auto chapters = GetJsonArray(d, "chapters")) {
						for (const auto& chapter : chapters->GetArray()) {
							if (auto chapterRenderer = GetJsonObject(chapter, "chapterRenderer")) {
								if (auto title = GetJsonObject(*chapterRenderer, "title")) {
									int timeRangeStartMillis;
									CString simpleText;
									if (getJsonValue(*title, "simpleText", simpleText) && getJsonValue(*chapterRenderer, "timeRangeStartMillis", timeRangeStartMillis)) {
										y_fields.chaptersList.emplace_back(simpleText, 10000LL * timeRangeStartMillis);
									}
								}
							}
						}
					}
				}
			}

			if (y_fields.chaptersList.empty()) {
				const std::regex regex(R"((?:^|<br\s*\/>)([^<]*<a[^>]+onclick=["\']yt\.www\.watch\.player\.seekTo[^>]+>(\d{1,2}:\d{1,2}(?::\d{1,2})?)<\/a>[^>]*)(?=$|<br\s*\/>))");
				std::cmatch match;
				LPCSTR text = data.data();
				while (std::regex_search(text, match, regex)) {
					if (match.size() == 3) {
						CStringA entries(match[1].first, match[1].length());
						CStringA time(match[2].first, match[2].length());

						auto pos = entries.Find("</a>");
						if (pos > 0) {
							entries.Delete(0, pos + 4);
							entries.TrimLeft("- ");
						}

						if (!entries.IsEmpty()) {
							REFERENCE_TIME rt = INVALID_TIME;
							int t1 = 0;
							int t2 = 0;
							int t3 = 0;

							if (const auto ret = sscanf_s(time.GetString(), "%02d:%02d:%02d", &t1, &t2, &t3)) {
								if (ret == 3) {
									rt = (((60LL * t1) + t2) * 60LL + t3) * UNITS;
								} else if (ret == 2) {
									rt = ((60LL * t1) + t2) * UNITS;
								}
							}

							if (rt != INVALID_TIME) {
								y_fields.chaptersList.emplace_back(UTF8ToWStr(entries.GetString()), rt);
							}
						}
					}

					text = match[0].second;
				}
			}

			if (strUrlsLive.empty()) {
				bTryAgainLiveStream = false;
				if (!streamingDataFormatList.empty()) {
					for (const auto& element : streamingDataFormatList) {
						const auto& [itag, url, cipher, quality_label] = element;
						if (!url.IsEmpty()) {
							AddUrl(youtubeUrllist, youtubeAudioUrllist, CString(url), itag, 0, !quality_label.IsEmpty() ? quality_label.GetString() : nullptr);
						} else {
							CStringA url;
							CStringA signature;
							CStringA signature_prefix = "signature";

							std::list<CStringA> paramsA;
							Explode(cipher, paramsA, '&');

							for (const auto& paramA : paramsA) {
								const auto pos = paramA.Find('=');
								if (pos > 0) {
									const auto paramHeader = paramA.Left(pos);
									const auto paramValue = paramA.Mid(pos + 1);

									if (paramHeader == "url") {
										url = UrlDecode(paramValue);
									} else if (paramHeader == "s") {
										signature = UrlDecode(paramValue);
									} else if (paramHeader == "sp") {
										signature_prefix = paramValue;
									}
								}
							}

							if (!url.IsEmpty()) {
								const auto signature_format("&" + signature_prefix + "=%s");
								SignatureDecode(url, signature, signature_format.GetString());

								AddUrl(youtubeUrllist, youtubeAudioUrllist, CString(url), itag, 0, !quality_label.IsEmpty() ? quality_label.GetString() : nullptr);
							}
						}
					}
				} else {
					std::list<CStringA> urlsList;
					Explode(strUrls, urlsList, ',');

					for (const auto& lineA : urlsList) {
						int itag = 0;
						CStringA url;
						CStringA signature;
						CStringA signature_prefix = "signature";

						CStringA quality_label;

						std::list<CStringA> paramsA;
						Explode(lineA, paramsA, '&');

						for (const auto& paramA : paramsA) {
							int k = paramA.Find('=');
							if (k > 0) {
								const CStringA paramHeader = paramA.Left(k);
								const CStringA paramValue = paramA.Mid(k + 1);

								if (paramHeader == "url") {
									url = UrlDecode(UrlDecode(paramValue));
								} else if (paramHeader == "s") {
									signature = UrlDecode(UrlDecode(paramValue));
								} else if (paramHeader == "quality_label") {
									quality_label = paramValue;
								} else if (paramHeader == "itag") {
									if (sscanf_s(paramValue, "%d", &itag) != 1) {
										itag = 0;
									}
								} else if (paramHeader == "sp") {
									signature_prefix = paramValue;
								}
							}
						}

						if (itag) {
							const CStringA signature_format("&" + signature_prefix + "=%s");
							SignatureDecode(url, signature, signature_format.GetString());

							AddUrl(youtubeUrllist, youtubeAudioUrllist, CString(url), itag, 0, !quality_label.IsEmpty() ? quality_label.GetString() : nullptr);
						}
					}
				}

				if (streamingDataFormatListAudioWithLanguages.size()) {
					// remove non-DASH formats from the list
					youtubeUrllist.erase(
						std::remove_if(youtubeUrllist.begin(), youtubeUrllist.end(),
									   [](const Youtube::YoutubeUrllistItem& item) {
						return item.profile->type == y_media; }),
						youtubeUrllist.end());
				}
			} else {
				for (const auto& urlLive : strUrlsLive) {
					CStringA itag = RegExpParse(urlLive.GetString(), "/itag/(\\d+)");
					if (!itag.IsEmpty()) {
						AddUrl(youtubeUrllist, youtubeAudioUrllist, CString(urlLive), atoi(itag));
					}
				}
			}

			if (youtubeUrllist.empty()) {
				if (!strUrlsLive.empty() && bTryAgainLiveStream) {
					bTryAgainLiveStream = false;

					if (URLPostDataForLive(videoId.GetString(), postData)) {
						player_response_jsonDocument.Parse(postData.data());
						continue;
					}
				}

				DLog(L"Youtube::Parse_URL() : no output video format found");

				y_fields.Empty();
				return false;
			}

			break;
		}

		std::sort(youtubeUrllist.begin(), youtubeUrllist.end(), CompareUrllistItem);
		std::sort(youtubeAudioUrllist.begin(), youtubeAudioUrllist.end(), CompareUrllistItem);

		const auto last = std::unique(youtubeUrllist.begin(), youtubeUrllist.end(), [](const YoutubeUrllistItem& a, const YoutubeUrllistItem& b) {
			return a.profile->format == b.profile->format && a.profile->quality == b.profile->quality &&
				   a.profile->fps60 == b.profile->fps60 && a.profile->hdr == b.profile->hdr;
		});
#ifdef DEBUG_OR_LOG
		YoutubeUrllist youtubeDuplicate;
		if (last != youtubeUrllist.end()) {
			youtubeDuplicate.insert(youtubeDuplicate.begin(), last, youtubeUrllist.end());
		}
#endif
		youtubeUrllist.erase(last, youtubeUrllist.end());

#ifdef DEBUG_OR_LOG
		DLog(L"Youtube::Parse_URL() : parsed video formats list:");
		for (const auto& item : youtubeUrllist) {
			DLog(L"    %-35s, \"%s\"", item.title, item.url);
		}

		if (!youtubeDuplicate.empty()) {
			DLog(L"Youtube::Parse_URL() : removed(duplicate) video formats list:");
			for (const auto& item : youtubeDuplicate) {
				DLog(L"    %-35s, \"%s\"", item.title, item.url);
			}
		}

		if (!youtubeAudioUrllist.empty()) {
			DLog(L"Youtube::Parse_URL() : parsed audio formats list:");
			for (const auto& item : youtubeAudioUrllist) {
				DLog(L"    %-35s, \"%s\"", item.title, item.url);
			}
		}
#endif

		const YoutubeUrllistItem* final_item = nullptr;
		CStringW final_media_url;
		CStringW final_audio_url;

		if (s.YoutubeFormat.res == 0) { // audio only
			final_item = SelectAudioStream(youtubeAudioUrllist);
			if (final_item) {
				DLog(L"Youtube::Parse_URL() : output audio format - %s, \"%s\"", final_item->title, final_item->url);
				final_media_url = final_item->url;
			}
		}

		if (!final_item) {
			final_item = SelectVideoStream(youtubeUrllist);
			if (final_item) {
				DLog(L"Youtube::Parse_URL() : output video format - %s, \"%s\"", final_item->title, final_item->url);
				final_media_url = final_item->url;

				if (final_item->profile->type == y_video && !youtubeAudioUrllist.empty()) {
					const auto audio_item = GetAudioUrl(final_item->profile, youtubeAudioUrllist);
					DLog(L"Youtube::Parse_URL() : output audio format - %s, \"%s\"", audio_item->title, audio_item->url);
					final_audio_url = audio_item->url;
				}
			}
		}

		if (final_media_url.IsEmpty()) {
			return false;
		}

		final_media_url.Replace(L"http://", L"https://");
		final_audio_url.Replace(L"http://", L"https://");

		const auto bParseMetadata = ParseResponseJson(player_response_jsonDocument, y_fields);
		if (!bParseMetadata) {
			ParseMetadata(videoId, y_fields);
		}

		if (final_item->profile->type == Youtube::y_audio) {
			y_fields.fname.Format(L"%s.%s", y_fields.title, final_item->profile->ext);
		} else {
			y_fields.fname.Format(L"%s.%dp.%s", y_fields.title, final_item->profile->quality, final_item->profile->ext);
		}
		FixFilename(y_fields.fname);

		// subtitles
		if (!player_response_jsonDocument.IsNull()) {
			if (auto captionTracks = GetValueByPointer(player_response_jsonDocument, "/captions/playerCaptionsTracklistRenderer/captionTracks"); captionTracks && captionTracks->IsArray()) {
				for (const auto& elem : captionTracks->GetArray()) {
					CStringA kind;
					if (getJsonValue(elem, "kind", kind) && kind == "asr") {
						continue;
					}

					CString sub_url;
					if (getJsonValue(elem, "baseUrl", sub_url)) {
						if (sub_url.Find(L"&fmt=") == -1) {
							sub_url += L"&fmt=vtt";
						} else {
							auto result = std::regex_replace(sub_url.GetString(), std::wregex(LR"((&fmt=.[^&]+))"), L"&fmt=vtt");
							sub_url = result.c_str();
						}
					}

					CString sub_name;
					if (auto name = GetJsonObject(elem, "name")) {
						getJsonValue(*name, "simpleText", sub_name);
					}
					if (sub_name.IsEmpty()) {
						if (auto name = GetValueByPointer(elem, "/name/runs"); name && name->IsArray()) {
							auto array = name->GetArray();
							if (!array.Empty()) {
								getJsonValue(array[0], "text", sub_name);
							}
						}
					}
					if (EndsWith(sub_name, L" - Default")) {
						// Removing the useless " - Default" at the end of the subtitle names of some YouTube clips
						// Example: https://www.youtube.com/watch?v=_TTqjLv0PYI
						sub_name.Truncate(sub_name.GetLength() - 10);
					}

					CStringA sub_lang;
					getJsonValue(elem, "languageCode", sub_lang);

					if (!sub_url.IsEmpty() && !sub_name.IsEmpty()) {
						pOFD->subs.emplace_back(sub_url, sub_name, sub_lang);
					}
				}
			}
		}

		if (youtubeAudioUrllist.size()) {
			CString thumbnailUrl;
			if (!player_response_jsonDocument.IsNull()) {
				if (auto thumbnails = GetValueByPointer(player_response_jsonDocument, "/videoDetails/thumbnail/thumbnails"); thumbnails && thumbnails->IsArray()) {
					int maxHeight = 0;
					for (const auto& elem : thumbnails->GetArray()) {
						CString url;
						if (!getJsonValue(elem, "url", url)) {
							continue;
						}

						int height = 0;
						if (!getJsonValue(elem, "height", height)) {
							continue;
						}

						if (height > maxHeight) {
							maxHeight = height;
							thumbnailUrl = url;
						}
					}
					y_fields.thumbnailUrl = thumbnailUrl;
				}
			}

			for (const auto& item : youtubeAudioUrllist) {
				if (item.profile->format == Youtube::y_mp4_aac) {
					youtubeUrllist.emplace_back(item);
					break;
				}
			}
			for (const auto& item : youtubeAudioUrllist) {
				if (item.profile->format == Youtube::y_webm_aud) {
					youtubeUrllist.emplace_back(item);
					break;
				}
			}
			for (const auto& item : youtubeAudioUrllist) {
				if (item.profile->format == Youtube::y_mp4_ac3) {
					youtubeUrllist.emplace_back(item);
					break;
				}
			}
			for (const auto& item : youtubeAudioUrllist) {
				if (item.profile->format == Youtube::y_mp4_eac3) {
					youtubeUrllist.emplace_back(item);
					break;
				}
			}
			for (const auto& item : youtubeAudioUrllist) {
				if (item.profile->format == Youtube::y_mp4_dtse) {
					youtubeUrllist.emplace_back(item);
					break;
				}
			}
		}

		pOFD->fi = final_media_url;
		if (final_audio_url.GetLength()) {
			pOFD->auds.emplace_back(final_audio_url);
		}

		return pOFD->fi.Valid();
	}

	bool Parse_Playlist(CString url, YoutubePlaylist& youtubePlaylist, int& idx_CurrentPlay)
	{
		idx_CurrentPlay = 0;
		youtubePlaylist.clear();
		if (CheckPlaylist(url)) {
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
						auto contents = GetValueByPointer(json, "/contents/twoColumnBrowseResultsRenderer/tabs/0/tabRenderer/content/sectionListRenderer/contents/0/itemSectionRenderer/contents/0/playlistVideoListRenderer/contents");
						if (!contents) {
							contents = GetValueByPointer(json, "/contents/twoColumnWatchNextResults/playlist/playlist/contents");
						}

						CString lastvideoId;
						auto ParseJsonObject = [&](const auto& object) {
							CString videoId;
							getJsonValue(object, "videoId", videoId);

							if (!videoId.IsEmpty()) {
								if (auto title = GetJsonObject(object, "title")) {
									CString simpleText;
									if (!getJsonValue(*title, "simpleText", simpleText)) {
										auto text = GetValueByPointer(*title, "/runs/0/text");
										if (text && text->IsString()) {
											simpleText = UTF8ToWStr(text->GetString());
										}
									}
									if (!simpleText.IsEmpty()) {
										REFERENCE_TIME duration = 0;
										CString lengthSeconds;
										if (getJsonValue(object, "lengthSeconds", lengthSeconds)) {
											duration = UNITS * _wtoi(lengthSeconds);
										} else if (auto lengthText = GetJsonObject(object, "lengthText")) {
											CString simpleText;
											if (getJsonValue(*lengthText, "simpleText", simpleText)) {
												int t1 = 0;
												int t2 = 0;
												int t3 = 0;

												if (const auto ret = swscanf_s(simpleText.GetString(), L"%02d:%02d:%02d", &t1, &t2, &t3)) {
													if (ret == 3) {
														duration = (((60LL * t1) + t2) * 60LL + t3) * UNITS;
													} else if (ret == 2) {
														duration = ((60LL * t1) + t2) * UNITS;
													}
												}
											}
										}

										CString url; url.Format(L"https://www.youtube.com/watch?v=%s", videoId);
										auto it = std::find_if(youtubePlaylist.cbegin(), youtubePlaylist.cend(), [&url](const YoutubePlaylistItem& item) {
											return item.url == url;
										});
										if (it == youtubePlaylist.cend()) {
											lastvideoId = videoId;
											youtubePlaylist.emplace_back(url, simpleText, duration);

											if (videoId == videoIdCurrent) {
												idx_CurrentPlay = youtubePlaylist.size() - 1;
											}
										}
									}
								}
							}
						};

						if (contents && contents->IsArray()) {
							for (const auto& item : contents->GetArray()) {
								if (auto playlistPanelVideoRenderer = GetJsonObject(item, "playlistPanelVideoRenderer")) {
									ParseJsonObject(*playlistPanelVideoRenderer);
								} else if (auto playlistVideoRenderer = GetJsonObject(item, "playlistVideoRenderer")) {
									ParseJsonObject(*playlistVideoRenderer);
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
						auto it = std::find_if(youtubePlaylist.cbegin(), youtubePlaylist.cend(), [&url](const YoutubePlaylistItem& item) {
							return item.url == url;
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
						url.Format(L"https://www.youtube.com/%S", moreUrl);

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

	bool Parse_URL(CString url, YoutubeFields& y_fields)
	{
		bool bRet = false;
		if (CheckURL(url)) {
			HandleURL(url);
			const CString videoId = RegExpParse(url.GetString(), videoIdRegExp);

			bRet = ParseMetadata(videoId, y_fields);
		}

		return bRet;
	}

	const YoutubeUrllistItem* GetAudioUrl(const YoutubeProfile* vprofile, const YoutubeUrllist& youtubeAudioUrllist)
	{
		const YoutubeUrllistItem* audio_item = nullptr;

		if (youtubeAudioUrllist.size()) {
			for (const auto& item : youtubeAudioUrllist) {
				if ((vprofile->format == y_mp4_avc || vprofile->format == y_mp4_av1) && item.profile->format == y_mp4_aac
						|| (vprofile->format != y_mp4_avc && vprofile->format != y_mp4_av1) && item.profile->format != y_mp4_aac) {
					audio_item = &item;
					if (vprofile->type != y_video || vprofile->quality > 360) {
						break;
					}
				}
			}

			if (!audio_item) {
				audio_item = &youtubeAudioUrllist.front();
			}
		}

		return audio_item;
	}
}
