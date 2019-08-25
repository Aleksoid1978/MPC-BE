/*
 * (C) 2012-2019 see Authors.txt
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
#include <regex>
#include "../../DSUtil/text.h"
#include "PlayerYouTube.h"

#define RAPIDJSON_SSE2
#include <rapidjson/include/rapidjson/document.h>

#define YOUTUBE_PL_URL              L"youtube.com/playlist?"
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

#define MATCH_AGE_RESTRICTION       "player-age-gate-content\">"
#define MATCH_STREAM_MAP_START_2    "url_encoded_fmt_stream_map="
#define MATCH_ADAPTIVE_FMTS_START_2 "adaptive_fmts="
#define MATCH_JS_START_2            "'PREFETCH_JS_RESOURCES': [\""
#define MATCH_END_2                 "&"

#define MATCH_PLAYER_RESPONSE       "\"player_response\":\""
#define MATCH_PLAYER_RESPONSE_END   "}\""

#define INTERNET_OPEN_FALGS         INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD

namespace Youtube
{
	static LPCWSTR GOOGLE_API_KEY = L"AIzaSyDggqSjryBducTomr4ttodXqFpl2HGdoyg";

	static LPCWSTR videoIdRegExp = L"(v|video_ids)=([-a-zA-Z0-9_]+)";

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
		CString tmp_fn(url);
		tmp_fn.MakeLower();

		if (tmp_fn.Find(YOUTUBE_URL) != -1
				|| tmp_fn.Find(YOUTUBE_URL_A) != -1
				|| tmp_fn.Find(YOUTUBE_URL_V) != -1
				|| tmp_fn.Find(YOUTUBE_URL_EMBED) != -1
				|| tmp_fn.Find(YOUTU_BE_URL) != -1) {
			return true;
		}

		return false;
	}

	bool CheckPlaylist(CString url)
	{
		CString tmp_fn(url);
		tmp_fn.MakeLower();

		if (tmp_fn.Find(YOUTUBE_PL_URL) != -1
				|| (tmp_fn.Find(YOUTUBE_URL) != -1 && tmp_fn.Find(L"&list=") != -1)
				|| (tmp_fn.Find(YOUTUBE_URL_A) != -1 && tmp_fn.Find(L"/watch_videos?video_ids") != -1)
				|| ((tmp_fn.Find(YOUTUBE_URL_V) != -1 || tmp_fn.Find(YOUTUBE_URL_EMBED) != -1) && tmp_fn.Find(L"list=") != -1)) {
			return true;
		}

		return false;
	}

	static CString RegExpParse(LPCTSTR szIn, LPCTSTR szRE, const size_t size)
	{
		const std::wregex regex(szRE);
		std::wcmatch match;
		if (std::regex_search(szIn, match, regex) && match.size() == size) {
			return CString(match[size - 1].first, match[size - 1].length());
		}

		return L"";
	}

	static CStringA RegExpParseA(LPCSTR szIn, LPCSTR szRE)
	{
		const std::regex regex(szRE);
		std::cmatch match;
		if (std::regex_search(szIn, match, regex) && match.size() == 2) {
			return CStringA(match[1].first, match[1].length());
		}

		return "";
	}

	static void InternetReadData(HINTERNET& hInternet, const CString& url, char** pData, DWORD& dataSize)
	{
		dataSize = 0;
		*pData = nullptr;

		const HINTERNET hFile = InternetOpenUrlW(hInternet, url.GetString(), nullptr, 0, INTERNET_OPEN_FALGS, 0);
		if (hFile) {
			char buffer[16 * KILOBYTE] = { 0 };
			const DWORD dwNumberOfBytesToRead = _countof(buffer);
			DWORD dwBytesRead = 0;
			for (;;) {
				if (InternetReadFile(hFile, (LPVOID)buffer, dwNumberOfBytesToRead, &dwBytesRead) == FALSE || dwBytesRead == 0) {
					break;
				}

				*pData = (char*)realloc(*pData, dataSize + dwBytesRead + 1);
				memcpy(*pData + dataSize, buffer, dwBytesRead);
				dataSize += dwBytesRead;
				(*pData)[dataSize] = 0;
			}

			InternetCloseHandle(hFile);
		}
	}

	static bool ParseMetadata(HINTERNET& hInet, const CString videoId, YoutubeFields& y_fields)
	{
		if (hInet && !videoId.IsEmpty()) {
			CString url;
			url.Format(L"https://www.googleapis.com/youtube/v3/videos?id=%s&key=%s&part=snippet,contentDetails&fields=items/snippet/title,items/snippet/publishedAt,items/snippet/channelTitle,items/snippet/description,items/contentDetails/duration", videoId, GOOGLE_API_KEY);
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

					const rapidjson::Value& snippet = items->value[0]["snippet"];
					if (snippet["title"].IsString()) {
						y_fields.title = FixHtmlSymbols(AltUTF8ToWStr(snippet["title"].GetString()));
					}
					if (snippet["channelTitle"].IsString()) {
						y_fields.author = UTF8ToWStr(snippet["channelTitle"].GetString());
					}
					if (snippet["description"].IsString()) {
						y_fields.content = UTF8ToWStr(snippet["description"].GetString());
					}
					if (snippet["publishedAt"].IsString()) {
						WORD y, m, d;
						WORD hh, mm, ss;
						if (sscanf_s(snippet["publishedAt"].GetString(), "%04hu-%02hu-%02huT%02hu:%02hu:%02hu", &y, &m, &d, &hh, &mm, &ss) == 6) {
							y_fields.dtime.wYear   = y;
							y_fields.dtime.wMonth  = m;
							y_fields.dtime.wDay    = d;
							y_fields.dtime.wHour   = hh;
							y_fields.dtime.wMinute = mm;
							y_fields.dtime.wSecond = ss;
						}
					}

					const rapidjson::Value& duration = items->value[0]["contentDetails"]["duration"];
					if (duration.IsString()) {
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

				free(data);

				return true;
			}
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

			HINTERNET hInet = InternetOpenW(L"Googlebot", 0, nullptr, nullptr, 0);
			if (hInet) {
				HandleURL(url);

				videoId = RegExpParse(url, videoIdRegExp, 3);

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
						const CString timeStart = RegExpParse(url, L"(t|time_continue)=([0-9]+)", 3);
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
			if (!JSUrl.IsEmpty()) {
				JSUrl.Replace(L"\\/", L"/");
				if (JSUrl.Find(L"s.ytimg.com") == -1) {
					JSUrl = L"//s.ytimg.com" + JSUrl;
				}
				JSUrl = L"https:" + JSUrl;
			}

			CStringA strUrls;
			std::list<CStringA> strUrlsLive;
			if (strstr(data, MATCH_AGE_RESTRICTION)) {
				free(data);

				CString link; link.Format(L"https://www.youtube.com/embed/%s", videoId);
				InternetReadData(hInet, link, &data, dataSize);

				if (!data) {
					InternetCloseHandle(hInet);
					return false;
				}

				const CStringA sts = RegExpParseA(data, "\"sts\"\\s*:\\s*(\\d+)");

				free(data);

				link.Format(L"https://www.youtube.com/get_video_info?video_id=%s&eurl=https://youtube.googleapis.com/v/%s&sts=%S", videoId, videoId, sts);
				InternetReadData(hInet, link, &data, dataSize);

				if (!data) {
					InternetCloseHandle(hInet);
					return false;
				}

				// url_encoded_fmt_stream_map
				const CStringA stream_map = UrlDecode(GetEntry(data, MATCH_STREAM_MAP_START_2, MATCH_END_2));
				if (stream_map.IsEmpty()) {
					free(data);
					InternetCloseHandle(hInet);
					return false;
				}
				// adaptive_fmts
				const CStringA adaptive_fmts = UrlDecode(GetEntry(data, MATCH_ADAPTIVE_FMTS_START_2, MATCH_END_2));

				strUrls = stream_map;
				if (!adaptive_fmts.IsEmpty()) {
					strUrls += ',';
					strUrls += adaptive_fmts;
				}
			} else {
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
					if (stream_map.IsEmpty()) {
						free(data);
						InternetCloseHandle(hInet);
						return false;
					}
					// adaptive_fmts
					const CStringA adaptive_fmts = GetEntry(data, MATCH_ADAPTIVE_FMTS_START, MATCH_END);

					strUrls = stream_map;
					if (!adaptive_fmts.IsEmpty()) {
						strUrls += ',';
						strUrls += adaptive_fmts;
					}
					strUrls.Replace("\\u0026", "&");
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
								"\\b[cs]\\s*&&\\s*[adf]\\.set\\([^,]+\\s*,\\s*encodeURIComponent\\s*\\(\\s*([a-zA-Z0-9$]+)\\(",
								"\\b[a-zA-Z0-9]+\\s*&&\\s*[a-zA-Z0-9]+\\.set\\([^,]+\\s*,\\s*encodeURIComponent\\s*\\(\\s*([a-zA-Z0-9$]+)\\(",
								"([a-zA-Z0-9$]+)\\s*=\\s*function\\(\\s*a\\s*\\)\\s*{\\s*a\\s*=\\s*a\\.split\\(\\s*""\\s*\\)",
								"([\"'])signature\\1\\s*,\\s*([a-zA-Z0-9$]+)\\(",
								"\\.sig\\|\\|([a-zA-Z0-9$]+)\\(",
								"yt\\.akamaized\\.net/\\)\\s*\\|\\|\\s*.*?\\s*[cs]\\s*&&\\s*[adf]\\.set\\([^,]+\\s*,\\s*(?:encodeURIComponent\\s*\\()?\\s*([a-zA-Z0-9$]+)\\(",
								"\\b[cs]\\s*&&\\s*[adf]\\.set\\([^,]+\\s*,\\s*([a-zA-Z0-9$]+)\\(",
								"\\b[a-zA-Z0-9]+\\s*&&\\s*[a-zA-Z0-9]+\\.set\\([^,]+\\s*,\\s*([a-zA-Z0-9$]+)\\(",
								"\\bc\\s*&&\\s*a\\.set\\([^,]+\\s*,\\s*\\([^)]*\\)\\s*\\(\\s*([a-zA-Z0-9$]+)\\(",
								"\\bc\\s*&&\\s*[a-zA-Z0-9]+\\.set\\([^,]+\\s*,\\s*\\([^)]*\\)\\s*\\(\\s*([a-zA-Z0-9$]+)\\(",
								"\\bc\\s*&&\\s*[a-zA-Z0-9]+\\.set\\([^,]+\\s*,\\s*\\([^)]*\\)\\s*\\(\\s*([a-zA-Z0-9$]+)\\("
							};
							CStringA funcName;
							for (const auto& sigRegExp : signatureRegExps) {
								funcName = RegExpParseA(data, sigRegExp);
								if (!funcName.IsEmpty()) {
									break;
								}
							}
							if (!funcName.IsEmpty()) {
								CStringA funcRegExp = funcName + "=function\\(a\\)\\{([^\\n]+)\\};"; funcRegExp.Replace("$", "\\$");
								const CStringA funcBody = RegExpParseA(data, funcRegExp);
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
						CStringA signature = RegExpParseA(url, "/s/([0-9A-Z]+.[0-9A-Z]+)");
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
						LPCTSTR text = xml.GetBuffer();
						while (std::regex_search(text, match, regex)) {
							if (match.size() == 2) {
								const CString xmlElement(match[1].first, match[1].length());
								const CString url = RegExpParse(xmlElement, L"<BaseURL>(.*?)</BaseURL>", 2);
								const int itag    = _wtoi(RegExpParse(xmlElement, L"id=\"([0-9]+)\"", 2));
								const int fps     = _wtoi(RegExpParse(xmlElement, L"frameRate=\"([0-9]+)\"", 2));
								if (url.Find(L"dur/") > 0) {
									AddUrl(youtubeUrllist, youtubeAudioUrllist, url, itag, fps);
								}
							}

							text = match[0].second;
						}
					}
				}
			}

			CStringA player_responce_jsonData = UrlDecode(GetEntry(data, MATCH_PLAYER_RESPONSE, MATCH_PLAYER_RESPONSE_END));

			free(data);

			if (strUrlsLive.empty()) {
				std::list<CStringA> linesA;
				Explode(strUrls, linesA, ',');

				for (const auto& lineA : linesA) {
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
			} else {
				for (const auto& urlLive : strUrlsLive) {
					CStringA itag = RegExpParseA(urlLive, "/itag/(\\d+)");
					if (!itag.IsEmpty()) {
						AddUrl(youtubeUrllist, youtubeAudioUrllist, CString(urlLive), atoi(itag));
					}
				}
			}

			if (youtubeUrllist.empty()) {
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
				DLog(L"YouTube::Parse_URL() : %s format not found, used %s", s.YoutubeFormat.fmt == 1 ? L"WebM" : L"MP4", final_item->profile->format == y_webm ? L"WebM" : L"MP4");
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

				ParseMetadata(hInet, videoId, y_fields);

				y_fields.fname.Format(L"%s.%dp.%s", y_fields.title, final_video_profile->quality, final_video_profile->ext);
				FixFilename(y_fields.fname);

				if (!videoId.IsEmpty()) {
					// subtitle
					if (!player_responce_jsonData.IsEmpty()) {
						player_responce_jsonData += "}";
						player_responce_jsonData.Replace("\\u0026", "&");
						player_responce_jsonData.Replace("\\/", "/");
						player_responce_jsonData.Replace("\\\"", "\"");
						player_responce_jsonData.Replace("\\\\\"", "\\\"");
						player_responce_jsonData.Replace("\\&", "&");

						rapidjson::Document d;
						if (!d.Parse(player_responce_jsonData).HasParseError()) {
							const auto& root = d.FindMember("captions");
							if (root != d.MemberEnd()) {
								const auto& iter = root->value.FindMember("playerCaptionsTracklistRenderer");
								if (iter != root->value.MemberEnd()) {
									const auto& captionTracks = iter->value.FindMember("captionTracks");
									if (captionTracks != iter->value.MemberEnd() && captionTracks->value.IsArray()) {
										for (auto elem = captionTracks->value.Begin(); elem != captionTracks->value.End(); ++elem) {
											const auto& kind = elem->FindMember("kind");
											if (kind != elem->MemberEnd() && kind->value.IsString()) {
												const CStringA kind_value = kind->value.GetString();
												if (kind_value == "asr") {
													continue;
												}
											}

											CString url, name;

											const auto& baseUrl = elem->FindMember("baseUrl");
											if (baseUrl != elem->MemberEnd() && baseUrl->value.IsString()) {
												const CStringA urlA = baseUrl->value.GetString();
												if (!urlA.IsEmpty()) {
													url = UTF8ToWStr(urlA) + L"&fmt=vtt";
												}
											}

											const auto& nameObject = elem->FindMember("name");
											if (nameObject != elem->MemberEnd()) {
												const auto& simpleText = nameObject->value.FindMember("simpleText");
												if (simpleText != nameObject->value.MemberEnd() && simpleText->value.IsString()) {
													const CStringA nameA = simpleText->value.GetString();
													if (!nameA.IsEmpty()) {
														name = UTF8ToWStr(nameA);
													}
												}
											}

											if (!url.IsEmpty() && !name.IsEmpty()) {
												subs.push_back(CSubtitleItem(url, name));
											}
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
			const CString videoId = RegExpParse(url, videoIdRegExp, 3);
			const CString playlistId = RegExpParse(url, L"list=([-a-zA-Z0-9_]+)", 2);
			if (playlistId.IsEmpty()) {
				return false;
			}

			HINTERNET hInet = InternetOpenW(L"Googlebot", 0, nullptr, nullptr, 0);
			char* data = nullptr;
			DWORD dataSize = 0;

			CString nextToken;

			for (;;) {
				if (hInet) {
					CString link;
					link.Format(L"https://www.googleapis.com/youtube/v3/playlistItems?part=snippet&playlistId=%s&key=%s&maxResults=50", playlistId.GetString(), GOOGLE_API_KEY);
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
					const auto& items = d["items"];
					if (!items.IsArray()) {
						free(data);
						InternetCloseHandle(hInet);
						return false;
					}

					const auto& nextPageToken = d["nextPageToken"];
					if (nextPageToken.IsString()) {
						nextToken = UTF8ToWStr(nextPageToken.GetString());
					}

					for (rapidjson::SizeType i = 0; i < items.Size(); i++) {
						const auto& snippet = items[i]["snippet"];
						if (snippet.IsObject()) {
							const auto& resourceId = snippet["resourceId"];
							if (resourceId.IsObject()) {
								const auto& vid_id = resourceId["videoId"];
								if (vid_id.IsString()) {
									CString url, title;
									const CString videoIdStr = UTF8ToWStr(vid_id.GetString());
									url.Format(L"https://www.youtube.com/watch?v=%s", videoIdStr.GetString());

									const auto& vid_title = snippet["title"];
									if (vid_title.IsString()) {
										title = FixHtmlSymbols(AltUTF8ToWStr(vid_title.GetString()));
									}

									const YoutubePlaylistItem item(url, title);
									youtubePlaylist.emplace_back(item);

									if (videoId == videoIdStr) {
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
		}

		return false;
	}

	bool Parse_URL(CString url, YoutubeFields& y_fields)
	{
		bool bRet = false;
		if (CheckURL(url)) {
			HINTERNET hInet = InternetOpenW(L"Googlebot", 0, nullptr, nullptr, 0);
			if (hInet) {
				HandleURL(url);

				const CString videoId = RegExpParse(url, videoIdRegExp, 3);

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
			for (const auto& item : youtubeAudioUrllist) {
				if (format == item.profile->format) {
					audio_item = &item;
					break;
				}
			}

			if (!audio_item) {
				audio_item = &youtubeAudioUrllist[0];
			}
		}

		return audio_item;
	}
}