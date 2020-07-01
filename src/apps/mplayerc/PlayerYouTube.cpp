/*
 * (C) 2012-2020 see Authors.txt
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
#include <afxinet.h>
#include "../../DSUtil/text.h"
#include "../../DSUtil/std_helper.h"
#include "PlayerYouTube.h"

#include <rapidjson/include/rapidjsonHelper.h>

#define YOUTUBE_PL_URL              L"youtube.com/playlist?"
#define YOUTUBE_USER_URL            L"youtube.com/user/"
#define YOUTUBE_CHANNEL_URL         L"youtube.com/channel/"
#define YOUTUBE_URL                 L"youtube.com/watch?"
#define YOUTUBE_URL_A               L"www.youtube.com/attribution_link"
#define YOUTUBE_URL_V               L"youtube.com/v/"
#define YOUTUBE_URL_EMBED           L"youtube.com/embed/"
#define YOUTU_BE_URL                L"youtu.be/"

#define MATCH_STREAM_MAP_START      "\"url_encoded_fmt_stream_map\":\""
#define MATCH_ADAPTIVE_FMTS_START   "\"adaptive_fmts\":\""
#define MATCH_WIDTH_START           "meta property=\"og:video:width\" content=\""
#define MATCH_HLSVP_START           "\"hlsvp\":\""
#define MATCH_HLSMANIFEST_START     "\\\"hlsManifestUrl\\\":\\\""
#define MATCH_JS_START              "\"js\":\""
#define MATCH_MPD_START             "\"dashmpd\":\""
#define MATCH_END                   "\""

#define MATCH_PLAYLIST_ITEM_START   "<li class=\"yt-uix-scroller-scroll-unit "
#define MATCH_PLAYLIST_ITEM_START2  "<tr class=\"pl-video yt-uix-tile "

#define MATCH_AGE_RESTRICTION       "player-age-gate-content\">"
#define MATCH_STREAM_MAP_START_2    "url_encoded_fmt_stream_map="
#define MATCH_ADAPTIVE_FMTS_START_2 "adaptive_fmts="
#define MATCH_JS_START_2            "'PREFETCH_JS_RESOURCES': [\""
#define MATCH_END_2                 "&"

#define MATCH_PLAYER_RESPONSE       "\"player_response\":\""
#define MATCH_PLAYER_RESPONSE_END   "}\""

#define INTERNET_OPEN_FALGS         INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD

#define USER_AGENT                  L"MPC-BE"

namespace Youtube
{
#define USE_GOOGLE_API 0

#if USE_GOOGLE_API
#if __has_include("..\..\my_google_api_key.h")
#include "..\..\my_google_api_key.h"
#else
	static LPCWSTR strGoogleApiKey = L"place_your_google_api_key_here";
#endif
#endif

	static LPCWSTR videoIdRegExp = L"(?:v|video_ids)=([-a-zA-Z0-9_]+)";

	const YoutubeProfile* GetProfile(int iTag)
	{
		for (const auto& profile : YProfiles) {
			if (iTag == profile.iTag) {
				return &profile;
			}
		}

		return nullptr;
	}

	const YoutubeProfile* GetAudioProfile(int iTag)
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

	static inline CStringA GetEntry(LPCSTR pszBuff, LPCSTR pszMatchStart, LPCSTR pszMatchEnd)
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

		return "";
	}

	static void HandleURL(CString& url)
	{
		url = UrlDecode(CStringA(url));

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
			} else if (url.Find(YOUTUBE_URL_V) != -1) {
				url.Replace(L"v/", L"watch?v=");
				url.Replace(L"?list=", L"&list=");
			} else if (url.Find(YOUTUBE_URL_EMBED) != -1) {
				url.Replace(L"embed/", L"watch?v=");
				url.Replace(L"?list=", L"&list=");
			}
		}
	}

	bool CheckURL(CString url)
	{
		url.MakeLower();

		if (url.Find(YOUTUBE_URL) != -1
				|| url.Find(YOUTUBE_URL_A) != -1
				|| url.Find(YOUTUBE_URL_V) != -1
				|| url.Find(YOUTUBE_URL_EMBED) != -1
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
				|| (url.Find(YOUTUBE_URL) != -1 && url.Find(L"&list=") != -1)
				|| (url.Find(YOUTUBE_URL_A) != -1 && url.Find(L"/watch_videos?video_ids") != -1)
				|| ((url.Find(YOUTUBE_URL_V) != -1 || url.Find(YOUTUBE_URL_EMBED) != -1) && url.Find(L"list=") != -1)) {
			return true;
		}

		return false;
	}

	static void InternetReadData(HINTERNET& hInternet, const CString& url, char** pData, DWORD& dataSize)
	{
		dataSize = 0;
		*pData = nullptr;

		const HINTERNET hFile = InternetOpenUrlW(hInternet, url.GetString(), nullptr, 0, INTERNET_OPEN_FALGS, 0);
		if (hFile) {
			const DWORD dwNumberOfBytesToRead = 16 * KILOBYTE;
			auto buffer = DNew char[dwNumberOfBytesToRead];
			DWORD dwBytesRead = 0;
			for (;;) {
				if (InternetReadFile(hFile, (LPVOID)buffer, dwNumberOfBytesToRead, &dwBytesRead) == FALSE || dwBytesRead == 0) {
					break;
				}

				auto pTmp = (char*)realloc(*pData, dataSize + dwBytesRead + 1);
				if (!pTmp) {
					if (*pData) {
						free(*pData);
						*pData = nullptr;
					}
					dataSize = 0;
					break;
				}
				*pData = pTmp;
				memcpy(*pData + dataSize, buffer, dwBytesRead);
				dataSize += dwBytesRead;
				(*pData)[dataSize] = 0;
			}

			delete [] buffer;
			InternetCloseHandle(hFile);
		}
	}

	static bool ParseResponseJson(rapidjson::Document& json, YoutubeFields& y_fields, const bool bReplacePlus)
	{
		bool bParse = false;
		if (!json.IsNull()) {
			if (isJsonObject(videoDetails, json, "videoDetails")) {
				bParse = true;

				if (getJsonValue(videoDetails->value, "title", y_fields.title)) {
					y_fields.title = FixHtmlSymbols(y_fields.title);
					if (bReplacePlus) {
						y_fields.title.Replace('+', ' ');
					}
				}

				getJsonValue(videoDetails->value, "author", y_fields.author);
				if (bReplacePlus) {
					y_fields.author.Replace('+', ' ');
				}

				if (getJsonValue(videoDetails->value, "shortDescription", y_fields.content)) {
					y_fields.content.Replace(L"\\r\\n", L"\r\n");
					y_fields.content.Replace(L"\\n", L"\r\n");
					if (bReplacePlus) {
						y_fields.content.Replace('+', ' ');
					}
				}

				CStringA lengthSeconds;
				if (getJsonValue(videoDetails->value, "lengthSeconds", lengthSeconds)) {
					y_fields.duration = atoi(lengthSeconds.GetString()) * UNITS;
				}
			}

			if (isJsonObject(microformat, json, "microformat")) {
				if (isJsonObject(playerMicroformatRenderer, microformat->value, "playerMicroformatRenderer")) {
					CStringA publishDate;
					if (getJsonValue(playerMicroformatRenderer->value, "publishDate", publishDate)) {
						WORD y, m, d;
						if (sscanf_s(publishDate.GetString(), "%04hu-%02hu-%02hu", &y, &m, &d) == 3) {
							y_fields.dtime.wYear = y;
							y_fields.dtime.wMonth = m;
							y_fields.dtime.wDay = d;
						}
					}
				}
			}
		}

		return bParse;
	}

	static bool ParseMetadata(HINTERNET& hInet, const CString& videoId, YoutubeFields& y_fields)
	{
		if (hInet && !videoId.IsEmpty()) {
#if !USE_GOOGLE_API
			bool bParse = false;

			CString url;
			url.Format(L"https://www.youtube.com/get_video_info?video_id=%s", videoId);
			char* data = nullptr;
			DWORD dataSize = 0;
			InternetReadData(hInet, url, &data, dataSize);

			if (dataSize) {
				const CStringA strData = UrlDecode(data);
				free(data);

				const auto player_response_jsonData = RegExpParse<CStringA>(strData.GetString(), R"(player_response=(\{\S+\}))");
				if (!player_response_jsonData.IsEmpty()) {
					rapidjson::Document player_response_jsonDocument;
					player_response_jsonDocument.Parse(player_response_jsonData);

					bParse = ParseResponseJson(player_response_jsonDocument, y_fields, true);
				}
			}

			return bParse;
#else
			CString url;
			url.Format(L"https://www.googleapis.com/youtube/v3/videos?id=%s&key=%s&part=snippet,contentDetails&fields=items/snippet/title,items/snippet/publishedAt,items/snippet/channelTitle,items/snippet/description,items/contentDetails/duration", videoId, strGoogleApiKey);
			char* data = nullptr;
			DWORD dataSize = 0;
			InternetReadData(hInet, url, &data, dataSize);

			if (dataSize) {
				rapidjson::Document d;
				if (!d.Parse(data).HasParseError()) {
					const auto& items = d.FindMember("items");
					if (items == d.MemberEnd()
							|| !items->value.IsArray()
							|| items->value.Empty()) {
						free(data);
						return false;
					}

					const auto& value0 = items->value[0];
					if (isJsonObject(snippet, value0, "snippet")) {
						if (getJsonValue(snippet->value, "title", y_fields.title)) {
							y_fields.title = FixHtmlSymbols(y_fields.title);
						}

						getJsonValue(snippet->value, "channelTitle", y_fields.author);

						if (getJsonValue(snippet->value, "description", y_fields.content)) {
							std::wregex rgx(LR"(\r\n|\r|\n)");
							std::wstring wstr = std::regex_replace(y_fields.content.GetString(), rgx, L"\r\n");
							y_fields.content = wstr.c_str();
						}

						CStringA publishedAt;
						if (getJsonValue(snippet->value, "publishedAt", publishedAt)) {
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

					if (isJsonObject(contentDetails, value0, "contentDetails")) {
						CStringA duration;
						if (getJsonValue(contentDetails->value, "duration", duration)) {
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

				free(data);

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
		funcSWAP
	};

	bool Parse_URL(CString url, std::list<CString>& urls, YoutubeFields& y_fields, YoutubeUrllist& youtubeUrllist, YoutubeUrllist& youtubeAudioUrllist, CSubtitleItemList& subs, REFERENCE_TIME& rtStart)
	{
		if (CheckURL(url)) {
			DLog(L"Youtube::Parse_URL() : \"%s\"", url);

			char* data = nullptr;
			DWORD dataSize = 0;

			CString videoId;

			HINTERNET hInet = InternetOpenW(USER_AGENT, 0, nullptr, nullptr, 0);
			if (hInet) {
				HandleURL(url); url += L"&gl=US&hl=en&has_verified=1&bpctr=9999999999";

				videoId = RegExpParse<CString>(url.GetString(), videoIdRegExp);

				if (rtStart <= 0) {
					BOOL bMatch = FALSE;

					const std::wregex regex(L"t=(\\d+h)?(\\d{1,2}m)?(\\d{1,2}s)?", std::regex_constants::icase);
					std::wcmatch match;
					if (std::regex_search(url.GetBuffer(), match, regex) && match.size() == 4) {
						int h = 0;
						int m = 0;
						int s = 0;
						if (match[1].matched) {
							h = _wtoi(match[1].first);
							bMatch = TRUE;
						}
						if (match[2].matched) {
							m = _wtoi(match[2].first);
							bMatch = TRUE;
						}
						if (match[3].matched) {
							s = _wtoi(match[3].first);
							bMatch = TRUE;
						}

						rtStart = (h * 3600 + m * 60 + s) * UNITS;
					}

					if (!bMatch) {
						const CString timeStart = RegExpParse<CString>(url.GetString(), L"(?:t|time_continue)=([0-9]+)");
						if (!timeStart.IsEmpty()) {
							rtStart = _wtol(timeStart) * UNITS;
						}
					}
				}

				InternetReadData(hInet, url, &data, dataSize);
			}

			if (!data) {
				if (hInet) {
					InternetCloseHandle(hInet);
				}

				return false;
			}

			const CString Title = AltUTF8ToWStr(GetEntry(data, "<title>", "</title>"));
			y_fields.title = FixHtmlSymbols(Title);

			std::vector<youtubeFuncType> JSFuncs;
			std::vector<int> JSFuncArgs;
			BOOL bJSParsed = FALSE;
			CString JSUrl = UTF8ToWStr(GetEntry(data, MATCH_JS_START, MATCH_END));
			if (JSUrl.IsEmpty()) {
				JSUrl = UTF8ToWStr(GetEntry(data, MATCH_JS_START_2, MATCH_END));
			}

			rapidjson::Document player_response_jsonDocument;
			bool bReplacePlus = false;

			CStringA strUrls;
			std::list<CStringA> strUrlsLive;
			if (strstr(data, MATCH_AGE_RESTRICTION)) {
				bReplacePlus = true;

				free(data);

				CString link; link.Format(L"https://www.youtube.com/embed/%s", videoId);
				InternetReadData(hInet, link, &data, dataSize);

				if (!data) {
					InternetCloseHandle(hInet);
					return false;
				}

				if (JSUrl.IsEmpty()) {
					JSUrl = UTF8ToWStr(GetEntry(data, MATCH_JS_START, MATCH_END));
				}
				if (JSUrl.IsEmpty()) {
					JSUrl = UTF8ToWStr(GetEntry(data, MATCH_JS_START_2, MATCH_END));
				}

				const CStringA sts = RegExpParse<CStringA>(data, "\"sts\"\\s*:\\s*(\\d+)");

				free(data);

				link.Format(L"https://www.youtube.com/get_video_info?video_id=%s&eurl=https://youtube.googleapis.com/v/%s&sts=%S", videoId, videoId, sts);
				InternetReadData(hInet, link, &data, dataSize);

				if (!data) {
					InternetCloseHandle(hInet);
					return false;
				}

				const CStringA strData = UrlDecode(data);

				const auto player_response_jsonData = RegExpParse<CStringA>(strData.GetString(), R"(player_response=(\{\S+\}))");
				if (!player_response_jsonData.IsEmpty()) {
					player_response_jsonDocument.Parse(player_response_jsonData);
				}

				// url_encoded_fmt_stream_map
				const CStringA stream_map = GetEntry(strData, MATCH_STREAM_MAP_START_2, MATCH_END_2);
				if (!stream_map.IsEmpty()) {
					strUrls = stream_map;
				}
				// adaptive_fmts
				CStringA adaptive_fmts = GetEntry(strData, MATCH_ADAPTIVE_FMTS_START_2, MATCH_END_2);
				if (!adaptive_fmts.IsEmpty()) {
					if (!strUrls.IsEmpty()) {
						strUrls += ',';
					}
					strUrls += adaptive_fmts;
				}
			} else {
				auto player_response_jsonData = GetEntry(data, MATCH_PLAYER_RESPONSE, MATCH_PLAYER_RESPONSE_END);
				if (!player_response_jsonData.IsEmpty()) {
					player_response_jsonData += "}";
					player_response_jsonData.Replace(R"(\/)", "/");
					player_response_jsonData.Replace(R"(\")", R"(")");
					player_response_jsonData.Replace(R"(\\)", R"(\)");
					player_response_jsonDocument.Parse(player_response_jsonData);
				}

				// live streaming
				CStringA live_url = GetEntry(data, MATCH_HLSVP_START, MATCH_END);
				if (live_url.IsEmpty()) {
					live_url = GetEntry(data, MATCH_HLSMANIFEST_START, MATCH_END);
				}
				if (!live_url.IsEmpty()) {
					url = UrlDecode(UrlDecode(live_url));
					url.Replace(L"\\/", L"/");
					DLog(L"Youtube::Parse_URL() : Downloading m3u8 information \"%s\"", url);
					char* m3u8 = nullptr;
					DWORD m3u8Size = 0;
					InternetReadData(hInet, url, &m3u8, m3u8Size);
					if (m3u8Size) {
						CStringA m3u8Str(m3u8);
						free(m3u8);

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

					if (strUrlsLive.empty()) {
						urls.push_front(url);

						free(data);
						InternetCloseHandle(hInet);
						return true;
					}
				} else {
					// url_encoded_fmt_stream_map
					const CStringA stream_map = GetEntry(data, MATCH_STREAM_MAP_START, MATCH_END);
					if (!stream_map.IsEmpty()) {
						strUrls = stream_map;
					}
					// adaptive_fmts
					const CStringA adaptive_fmts = GetEntry(data, MATCH_ADAPTIVE_FMTS_START, MATCH_END);
					if (!adaptive_fmts.IsEmpty()) {
						if (!strUrls.IsEmpty()) {
							strUrls += ',';
						}
						strUrls += adaptive_fmts;
					}
					strUrls.Replace("\\u0026", "&");
				}
			}

			using streamingDataFormat = std::tuple<int, CStringA, CStringA, CStringA>;
			std::list<streamingDataFormat> streamingDataFormatList;
			if (!player_response_jsonDocument.IsNull()) {
				if (isJsonObject(streamingData, player_response_jsonDocument, "streamingData")) {
					if (isJsonArray(formats, streamingData->value, "formats")) {
						for (const auto& format : formats->value.GetArray()) {
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

					if (isJsonArray(adaptiveFormats, streamingData->value, "adaptiveFormats")) {
						for (const auto& adaptiveFormat : adaptiveFormats->value.GetArray()) {
							CStringA type;
							if (getJsonValue(adaptiveFormat, "type", type) && type == L"FORMAT_STREAM_TYPE_OTF") {
								// fragmented url
								continue;
							}

							streamingDataFormat element;

							getJsonValue(adaptiveFormat, "itag", std::get<0>(element));
							getJsonValue(adaptiveFormat, "qualityLabel", std::get<3>(element));
							if (getJsonValue(adaptiveFormat, "url", std::get<1>(element))) {
								streamingDataFormatList.emplace_back(element);
							} else if (getJsonValue(adaptiveFormat, "cipher", std::get<2>(element)) || getJsonValue(adaptiveFormat, "signatureCipher", std::get<2>(element))) {
								streamingDataFormatList.emplace_back(element);
							}
						}
					}
				}
			}

			if (!JSUrl.IsEmpty()) {
				JSUrl.Replace(L"\\/", L"/");
				JSUrl.Trim();

				if (JSUrl.Left(2) == L"//") {
					JSUrl = L"https:" + JSUrl;
				} else if (JSUrl.Find(L"http://") == -1 && JSUrl.Find(L"https://") == -1) {
					JSUrl = L"https://www.youtube.com" + JSUrl;
				}
			}

			auto AddUrl = [](YoutubeUrllist& videoUrls, YoutubeUrllist& audioUrls, const CString& url, const int itag, const int fps = 0, LPCSTR quality_label = nullptr) {
				if (url.Find(L"dur=0.000") > 0) {
					return;
				}

				if (const YoutubeProfile* profile = GetProfile(itag)) {
					YoutubeUrllistItem item;
					item.profile = profile;
					item.url = url;

					if (quality_label) {
						item.title.Format(L"%s %S",
							profile->format == y_webm ? L"WebM" : (profile->live ? L"HLS Live" : (profile->format == y_mp4_av1 ? L"MP4(AV1)" : L"MP4")),
							quality_label);
						if (profile->type == y_video) {
							item.title.Append(L" dash");
						}
						if (profile->hdr) {
							item.title.Append(L" (10 bit)");
						}
					} else {
						item.title.Format(L"%s %dp",
							profile->format == y_webm ? L"WebM" : (profile->live ? L"HLS Live" : (profile->format == y_mp4_av1 ? L"MP4(AV1)" : L"MP4")),
							profile->quality);
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
					item.title.Format(L"%s %dkbit/s",
						audioprofile->format == y_webm ? L"WebM/Opus" : L"MP4/AAC",
						audioprofile->quality);

					audioUrls.emplace_back(item);
				}
			};

			auto SignatureDecode = [&](CStringA& url, CStringA signature, LPCSTR format) {
				if (!signature.IsEmpty() && !JSUrl.IsEmpty()) {
					if (!bJSParsed) {
						bJSParsed = TRUE;
						char* data = nullptr;
						DWORD dataSize = 0;
						InternetReadData(hInet, JSUrl, &data, dataSize);
						if (dataSize) {
							static LPCSTR signatureRegExps[] = {
								R"(\b[cs]\s*&&\s*[adf]\.set\([^,]+\s*,\s*encodeURIComponent\s*\(\s*([a-zA-Z0-9$]+)\()",
								R"(\b[a-zA-Z0-9]+\s*&&\s*[a-zA-Z0-9]+\.set\([^,]+\s*,\s*encodeURIComponent\s*\(\s*([a-zA-Z0-9$]+)\()",
								R"(\b([a-zA-Z0-9$]{2})\s*=\s*function\(\s*a\s*\)\s*\{\s*a\s*=\s*a\.split\(\s*""\s*\))",
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
								funcName = RegExpParse<CStringA>(data, sigRegExp);
								if (!funcName.IsEmpty()) {
									break;
								}
							}
							if (!funcName.IsEmpty()) {
								CStringA funcRegExp = funcName + "=function\\(a\\)\\{([^\\n]+)\\};"; funcRegExp.Replace("$", "\\$");
								const CStringA funcBody = RegExpParse<CStringA>(data, funcRegExp.GetString());
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

										funcList.push_back(line);

										if (funcGroup.IsEmpty()) {
											const int k = line.Find('.');
											if (k > 0) {
												funcGroup = line.Left(k);
											}
										}
									}

									if (!funcGroup.IsEmpty()) {
										CStringA tmp; tmp.Format("var %s={", funcGroup);
										tmp = GetEntry(data, tmp, "};");
										if (!tmp.IsEmpty()) {
											tmp.Remove('\n');
											Explode(tmp, funcCodeList, "},");
										}
									}

									if (!funcList.empty() && !funcCodeList.empty()) {
										funcGroup += '.';

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

											CStringA funcName = GetEntry(func, funcGroup, "(");
											funcName += ":function";

											youtubeFuncType funcType = youtubeFuncType::funcNONE;

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
												JSFuncs.push_back(funcType);
												JSFuncArgs.push_back(funcArg);
											}
										}
									}
								}
							}

							free(data);
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

					for (size_t i = 0; i < JSFuncs.size(); i++) {
						const youtubeFuncType func = JSFuncs[i];
						const int arg = JSFuncArgs[i];
						switch (func) {
							case youtubeFuncType::funcDELETE:
								Delete(signature, arg);
								break;
							case youtubeFuncType::funcSWAP:
								Swap(signature, arg);
								break;
							case youtubeFuncType::funcREVERSE:
								Reverse(signature);
								break;
						}
					}

					url.AppendFormat(format, signature);
				}
			};

			if (strUrlsLive.empty()) {
				CString dashmpdUrl = UTF8ToWStr(GetEntry(data, MATCH_MPD_START, MATCH_END));
				if (!dashmpdUrl.IsEmpty()) {
					dashmpdUrl.Replace(L"\\/", L"/");
					if (dashmpdUrl.Find(L"/s/") > 0) {
						CStringA url(dashmpdUrl);
						CStringA signature = RegExpParse<CStringA>(url.GetString(), "/s/([0-9A-Z]+.[0-9A-Z]+)");
						if (!signature.IsEmpty()) {
							SignatureDecode(url, signature, "/signature/%s");
							dashmpdUrl = url;
						}
					}

					DLog(L"Youtube::Parse_URL() : Downloading MPD manifest \"%s\"", dashmpdUrl);
					char* dashmpd = nullptr;
					DWORD dashmpdSize = 0;
					InternetReadData(hInet, dashmpdUrl, &dashmpd, dashmpdSize);
					if (dashmpdSize) {
						CString xml = UTF8ToWStr(dashmpd);
						free(dashmpd);
						const std::wregex regex(L"<Representation(.*?)</Representation>");
						std::wcmatch match;
						LPCWSTR text = xml.GetBuffer();
						while (std::regex_search(text, match, regex)) {
							if (match.size() == 2) {
								const CString xmlElement(match[1].first, match[1].length());
								const CString url = RegExpParse<CString>(xmlElement.GetString(), L"<BaseURL>(.*?)</BaseURL>");
								const int itag    = _wtoi(RegExpParse<CString>(xmlElement.GetString(), L"id=\"([0-9]+)\""));
								const int fps     = _wtoi(RegExpParse<CString>(xmlElement.GetString(), L"frameRate=\"([0-9]+)\""));
								if (url.Find(L"dur/") > 0) {
									AddUrl(youtubeUrllist, youtubeAudioUrllist, url, itag, fps);
								}
							}

							text = match[0].second;
						}
					}
				}
			}

			CStringA chaptersStr = GetEntry(data, R"({"chapteredPlayerBarRenderer":)", "]");
			if (!chaptersStr.IsEmpty()) {
				chaptersStr.Replace(R"(\/)", "/");
				chaptersStr.Replace(R"(\")", R"(")");
				chaptersStr.Replace(R"(\\)", R"(\)");
				chaptersStr += "]}";

				rapidjson::Document d;
				if (!d.Parse(chaptersStr).HasParseError()) {
					if (isJsonArray(chapters, d, "chapters")) {
						for (const auto& chapter : chapters->value.GetArray()) {
							if (isJsonObject(chapterRenderer, chapter, "chapterRenderer")) {
								if (isJsonObject(title, chapterRenderer->value, "title")) {
									int timeRangeStartMillis;
									CString simpleText;
									if (getJsonValue(title->value, "simpleText", simpleText) && getJsonValue(chapterRenderer->value, "timeRangeStartMillis", timeRangeStartMillis)) {
										y_fields.chaptersList.push_back({ simpleText, 10000LL * timeRangeStartMillis });
									}
								}
							}
						}
					}
				}
			} else {
				const std::regex regex(R"((?:^|<br\s*\/>)([^<]*<a[^>]+onclick=["\']yt\.www\.watch\.player\.seekTo[^>]+>(\d{1,2}:\d{1,2}(?::\d{1,2})?)<\/a>[^>]*)(?=$|<br\s*\/>))");
				std::cmatch match;
				LPCSTR text = data;
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
								y_fields.chaptersList.push_back({ UTF8ToWStr(entries.GetString()), rt });
							}
						}
					}

					text = match[0].second;
				}
			}

			free(data);

			if (strUrlsLive.empty()) {
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
			} else {
				for (const auto& urlLive : strUrlsLive) {
					CStringA itag = RegExpParse<CStringA>(urlLive.GetString(), "/itag/(\\d+)");
					if (!itag.IsEmpty()) {
						AddUrl(youtubeUrllist, youtubeAudioUrllist, CString(urlLive), atoi(itag));
					}
				}
			}

			if (youtubeUrllist.empty()) {
				y_fields.Empty();
				return false;
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

			const CAppSettings& s = AfxGetAppSettings();

			// select video stream
			const YoutubeUrllistItem* final_item = nullptr;
			size_t k;
			for (k = 0; k < youtubeUrllist.size(); k++) {
				if (s.YoutubeFormat.fmt == youtubeUrllist[k].profile->format) {
					final_item = &youtubeUrllist[k];
					break;
				}
			}
			if (!final_item) {
				final_item = &youtubeUrllist[0];
				k = 0;
				DLog(L"YouTube::Parse_URL() : %s format not found, used %s", s.YoutubeFormat.fmt == y_webm ? L"WebM" : L"MP4", final_item->profile->format == y_webm ? L"WebM" : L"MP4");
			}

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

			DLog(L"Youtube::Parse_URL() : output video format - %s, \"%s\"", final_item->title, final_item->url);

			CString final_video_url = final_item->url;
			const YoutubeProfile* final_video_profile = final_item->profile;

			CString final_audio_url;
			if (final_item->profile->type == y_video && !youtubeAudioUrllist.empty()) {
				const auto audio_item = GetAudioUrl(final_item->profile->format, youtubeAudioUrllist);
				final_audio_url = audio_item->url;
				DLog(L"Youtube::Parse_URL() : output audio format - %s, \"%s\"", audio_item->title, audio_item->url);
			}

			if (!final_audio_url.IsEmpty()) {
				final_audio_url.Replace(L"http://", L"https://");
			}

			if (!final_video_url.IsEmpty()) {
				final_video_url.Replace(L"http://", L"https://");

				const auto bParseMetadata = ParseResponseJson(player_response_jsonDocument, y_fields, bReplacePlus);
				if (!bParseMetadata) {
					ParseMetadata(hInet, videoId, y_fields);
				}

				y_fields.fname.Format(L"%s.%dp.%s", y_fields.title, final_video_profile->quality, final_video_profile->ext);
				FixFilename(y_fields.fname);

				if (!videoId.IsEmpty()) {
					// subtitles
					if (!player_response_jsonDocument.IsNull()) {
						if (isJsonObject(captions, player_response_jsonDocument, "captions")) {
							if (isJsonObject(trackList, captions->value, "playerCaptionsTracklistRenderer")) {
								if (isJsonArray(captionTracks, trackList->value, "captionTracks")) {
									for (const auto& elem : captionTracks->value.GetArray()) {
										CStringA kind;
										if (getJsonValue(elem, "kind", kind) && kind == "asr") {
											continue;
										}

										CString sub_url;
										if (getJsonValue(elem, "baseUrl", sub_url)) {
											sub_url += L"&fmt=vtt";
										}

										CString sub_name;
										if (isJsonObject(name, elem, "name")) {
											getJsonValue(name->value, "simpleText", sub_name);
										}

										if (!sub_url.IsEmpty() && !sub_name.IsEmpty()) {
											subs.push_back({ sub_url, sub_name });
										}
									}
								}
							}
						}
					}
				}

				InternetCloseHandle(hInet);

				if (!final_video_url.IsEmpty()) {
					urls.push_front(final_video_url);

					if (!final_audio_url.IsEmpty()) {
						urls.push_back(final_audio_url);
					}
				}

				return !urls.empty();
			}
		}

		return false;
	}

	bool Parse_Playlist(CString url, YoutubePlaylist& youtubePlaylist, int& idx_CurrentPlay)
	{
		idx_CurrentPlay = 0;
		if (CheckPlaylist(url)) {
#if !USE_GOOGLE_API
			HINTERNET hInet = InternetOpenW(USER_AGENT, 0, nullptr, nullptr, 0);
			if (hInet) {
				auto channelId = RegExpParse<CString>(url.GetString(), L"www.youtube.com(?:/channel|/user)/([-a-zA-Z0-9_]+)");
				if (!channelId.IsEmpty()) {
					if (url.Find(YOUTUBE_USER_URL) != -1) {
						url.Format(L"https://www.youtube.com/user/%s", channelId);
						DLog(L"Youtube::Parse_Playlist() : downloading user page '%s'", url);

						char* data = nullptr;
						DWORD dataSize = 0;
						InternetReadData(hInet, url, &data, dataSize);
						if (data) {
							channelId = UTF8ToWStr(GetEntry(data, R"(content="https://www.youtube.com/channel/)", R"(")"));
							free(data);
						}
					}

					if (!channelId.IsEmpty()) {
						if (channelId.Left(2) == L"UC") {
							const CString playlistId = L"UU" + channelId.Right(channelId.GetLength() - 2);
							url.Format(L"https://www.youtube.com/playlist?list=%s", playlistId);
						} else {
							DLog(L"Youtube::Parse_Playlist() : unsupported user/channel page '%s'", url);
						}
					}
				}

				const auto videoIdCurrent = RegExpParse<CString>(url.GetString(), videoIdRegExp);
				const auto playlistId = RegExpParse<CString>(url.GetString(), L"list=([-a-zA-Z0-9_]+)");
				if (playlistId.IsEmpty()) {
					return false;
				}

				const auto extract_mix = playlistId.Left(2) == L"RD" || playlistId.Left(2) == L"UL" || playlistId.Left(2) == L"PU";
				if (extract_mix) {
					DLog(L"Youtube::Parse_Playlist() : mixed playlist");
				} else {
					url.Format(L"https://www.youtube.com/playlist?list=%s", playlistId);
				}

				char* data = nullptr;
				CStringA dataStr, moreStr;
				DWORD dataSize = 0;

				unsigned index = 0;
				DLog(L"Youtube::Parse_Playlist() : downloading #%u playlist '%s'", ++index, url);
				InternetReadData(hInet, url, &data, dataSize);
				if (data) {
					dataStr = data; free(data);
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
								youtubePlaylist.push_back({ url, title });

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

						url.Format(L"https://www.youtube.com/watch?v=%s&list=%s", lastvideoId, playlistId);
						DLog(L"Youtube::Parse_Playlist() : downloading #%u playlist '%s'", ++index, url);
						InternetReadData(hInet, url, &data, dataSize);
						if (data) {
							dataStr = data; free(data);
						}

						continue;
					} else {
						auto moreUrl = UrlDecode(RegExpParse<CStringA>(moreStr.GetString(), R"(data-uix-load-more-href="/?([^"]+)\")"));
						moreStr.Empty();
						if (!moreUrl.IsEmpty()) {
							moreUrl.Replace("&amp;", "&"); moreUrl += "&disable_polymer=true";
							url.Format(L"https://www.youtube.com/%S", moreUrl);

							DLog(L"Youtube::Parse_Playlist() : downloading #%u playlist '%s'", ++index, url);
							InternetReadData(hInet, url, &data, dataSize);
							if (data) {
								rapidjson::Document json;
								if (!json.Parse(data).HasParseError()) {
									getJsonValue(json, "content_html", dataStr);
									getJsonValue(json, "load_more_widget_html", moreStr);
								}

								free(data);
							}
						}
					}
				}

				InternetCloseHandle(hInet);

				return !youtubePlaylist.empty();
			}

#else
			const CString videoIdCurrent = RegExpParse<CString>(url.GetString(), videoIdRegExp);
			const CString playlistId = RegExpParse<CString>(url.GetString(), L"list=([-a-zA-Z0-9_]+)");
			if (playlistId.IsEmpty()) {
				return false;
			}

			HINTERNET hInet = InternetOpenW(USER_AGENT, 0, nullptr, nullptr, 0);
			char* data = nullptr;
			DWORD dataSize = 0;

			CString nextToken;

			for (;;) {
				if (hInet) {
					CString link;
					link.Format(L"https://www.googleapis.com/youtube/v3/playlistItems?part=snippet&playlistId=%s&key=%s&maxResults=50", playlistId.GetString(), strGoogleApiKey);
					if (!nextToken.IsEmpty()) {
						link.AppendFormat(L"&pageToken=%s", nextToken.GetString());
						nextToken.Empty();
					}

					InternetReadData(hInet, link, &data, dataSize);
				}

				if (!data) {
					InternetCloseHandle(hInet);
					return false;
				}

				rapidjson::Document d;
				if (!d.Parse(data).HasParseError()) {
					const auto& items = d.FindMember("items");
					if (items == d.MemberEnd()
							|| !items->value.IsArray()
							|| items->value.Empty()) {
						free(data);
						InternetCloseHandle(hInet);
						return false;
					}

					getJsonValue(d, "nextPageToken", nextToken);

					for (const auto& item : items->value.GetArray()) {
						if (isJsonObject(snippet, item, "snippet")) {
							if (isJsonObject(resourceId, snippet->value, "resourceId")) {
								CString videoId;
								if (getJsonValue(resourceId->value, "videoId", videoId)) {
									CString url;
									url.Format(L"https://www.youtube.com/watch?v=%s", videoId.GetString());

									CString title;
									if (getJsonValue(snippet->value, "title", title)) {
										title = FixHtmlSymbols(title);
									}

									youtubePlaylist.push_back({ url, title });

									if (videoIdCurrent == videoId) {
										idx_CurrentPlay = youtubePlaylist.size() - 1;
									}
								}
							}
						}
					}
				}

				free(data);

				if (nextToken.IsEmpty()) {
					InternetCloseHandle(hInet);
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
			HINTERNET hInet = InternetOpenW(USER_AGENT, 0, nullptr, nullptr, 0);
			if (hInet) {
				HandleURL(url);

				const CString videoId = RegExpParse<CString>(url.GetString(), videoIdRegExp);

				bRet = ParseMetadata(hInet, videoId, y_fields);

				InternetCloseHandle(hInet);
			}
		}

		return bRet;
	}

	const YoutubeUrllistItem* GetAudioUrl(const yformat format, const YoutubeUrllist& youtubeAudioUrllist)
	{
		const YoutubeUrllistItem* audio_item = nullptr;
		if (!youtubeAudioUrllist.empty()) {
			if (youtubeAudioUrllist.size() > 1) {
				for (const auto& item : youtubeAudioUrllist) {
					if (format == item.profile->format) {
						audio_item = &item;
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