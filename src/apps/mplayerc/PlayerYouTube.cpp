/*
 * (C) 2012-2016 see Authors.txt
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

#define YOUTUBE_PL_URL				L"youtube.com/playlist?"
#define YOUTUBE_URL					L"youtube.com/watch?"
#define YOUTUBE_URL_V				L"youtube.com/v/"
#define YOUTU_BE_URL				L"youtu.be/"

#define MATCH_STREAM_MAP_START		"\"url_encoded_fmt_stream_map\":\""
#define MATCH_ADAPTIVE_FMTS_START	"\"adaptive_fmts\":\""
#define MATCH_WIDTH_START			"meta property=\"og:video:width\" content=\""
#define MATCH_HLSVP_START			"\"hlsvp\":\""
#define MATCH_JS_START				"\"js\":\""
#define MATCH_END					"\""

#define MATCH_PLAYLIST_ITEM_START	"<li class=\"yt-uix-scroller-scroll-unit "
#define MATCH_PLAYLIST_ITEM_START2	"<tr class=\"pl-video yt-uix-tile "

#define INTERNET_OPEN_FALGS			INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD

namespace YoutubeParser {
	static const CString GOOGLE_API_KEY = L"AIzaSyDggqSjryBducTomr4ttodXqFpl2HGdoyg";

	static const YOUTUBE_PROFILES youtubeProfileEmpty = { 0, y_unknown, 0, NULL, false };

	enum youtubeProfiles {
		UNKNOWN_PROFILE = -1,
		AUDIO_PROFILE,
		VIDEO_PROFILE
	};

	const YOUTUBE_PROFILES* getProfile(int iTag, youtubeProfiles type) {
		YOUTUBE_PROFILES* profiles = NULL;
		int count = 0;

		switch (type) {
		case youtubeProfiles::AUDIO_PROFILE:
			profiles = (YOUTUBE_PROFILES*)youtubeAudioProfiles;
			count = _countof(youtubeAudioProfiles);
			break;
		case youtubeProfiles::VIDEO_PROFILE:
			profiles = (YOUTUBE_PROFILES*)youtubeVideoProfiles;
			count = _countof(youtubeVideoProfiles);
			break;
		}

		for (int i = 0; i < count; i++) {
			if (iTag == profiles[i].iTag) {
				return &profiles[i];
			}
		}

		return &youtubeProfileEmpty;
	}

	bool SelectBestProfile(int &itag_final, CString &ext_final, int itag_current, const YOUTUBE_PROFILES* sets, youtubeProfiles type)
	{
		const YOUTUBE_PROFILES* current = getProfile(itag_current, type);

		if (current->iTag <= 0
			|| current->quality > sets->quality) {
			return false;
		}

		if (itag_final != 0) {
			const YOUTUBE_PROFILES* fin = getProfile(itag_final, type);
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

	static inline CStringA GetEntry(LPCSTR pszBuff, LPCSTR pszMatchStart, LPCSTR pszMatchEnd)
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

	static void HandleURL(CString& url)
	{
		int pos = url.Find(L"youtube.com/");
		if (pos == -1) {
			pos = url.Find(L"youtu.be/");
		}

		if (pos != -1) {
			url.Delete(0, pos);
			url = L"https://www." + url;
		}
	}

	bool CheckURL(CString url)
	{
		CString tmp_fn(url);
		tmp_fn.MakeLower();

		if (tmp_fn.Find(YOUTUBE_URL) != -1 || tmp_fn.Find(YOUTUBE_URL_V) != -1 || tmp_fn.Find(YOUTU_BE_URL) != -1) {
			return true;
		}

		return false;
	}

	bool CheckPlaylist(CString url)
	{
		CString tmp_fn(url);
		tmp_fn.MakeLower();

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

	static void InternetReadData(HINTERNET& f, char** pData, DWORD& dataSize, LPCSTR endCondition)
	{
		char buffer[16 * KILOBYTE] = { 0 };
		const DWORD dwNumberOfBytesToRead = _countof(buffer);
		DWORD dwBytesRead = 0;
		for (;;) {
			if (InternetReadFile(f, (LPVOID)buffer, dwNumberOfBytesToRead, &dwBytesRead) == FALSE || dwBytesRead == 0) {
				break;
			}

			*pData = (char*)realloc(*pData, dataSize + dwBytesRead + 1);
			memcpy(*pData + dataSize, buffer, dwBytesRead);
			dataSize += dwBytesRead;
			(*pData)[dataSize] = 0;

			if (endCondition && strstr(*pData, endCondition)) {
				break;
			}
		}
	}

	static bool ParseMetadata(HINTERNET& s, const CString videoId, YOUTUBE_FIELDS& y_fields)
	{
		if (s && !videoId.IsEmpty()) {
			CString link;
			link.Format(L"https://www.googleapis.com/youtube/v3/videos?id=%s&key=%s&part=snippet,contentDetails&fields=items/snippet/title,items/snippet/publishedAt,items/snippet/channelTitle,items/snippet/description,items/contentDetails/duration", videoId, GOOGLE_API_KEY);
			HINTERNET f = InternetOpenUrl(s, link, NULL, 0, INTERNET_OPEN_FALGS, 0);
			if (f) {
				char* data = NULL;
				DWORD dataSize = 0;
				InternetReadData(f, &data, dataSize, NULL);
				InternetCloseHandle(f);

				if (dataSize) {
					Json::Reader reader;
					Json::Value root;
					if (reader.parse(data, root)) {
						Json::Value snippet = root["items"][0]["snippet"];
						if (!snippet["title"].empty()
								&& snippet["title"].isString()) {
							const CStringA sTitle = snippet["title"].asCString();
							y_fields.title = FixHtmlSymbols(UTF8To16(sTitle));
						}

						if (!snippet["channelTitle"].empty()
								&& snippet["channelTitle"].isString()) {
							const CStringA sAuthor = snippet["channelTitle"].asCString();
							y_fields.author = UTF8To16(sAuthor);
						}

						if (!snippet["description"].empty()
								&& snippet["description"].isString()) {
							const CStringA sContent = snippet["description"].asCString();
							y_fields.content = FixHtmlSymbols(UTF8To16(sContent));
						}

						if (!snippet["publishedAt"].empty()
								&& snippet["publishedAt"].isString()) {
							const CStringA sDate = snippet["publishedAt"].asCString();
							WORD y, m, d;
							WORD hh, mm, ss;
							if (sscanf_s(sDate, "%04hu-%02hu-%02huT%02hu:%02hu:%02hu", &y, &m, &d, &hh, &mm, &ss) == 6) {
								y_fields.dtime.wYear   = y;
								y_fields.dtime.wMonth  = m;
								y_fields.dtime.wDay    = d;
								y_fields.dtime.wHour   = hh;
								y_fields.dtime.wMinute = mm;
								y_fields.dtime.wSecond = ss;
							}
						}

						Json::Value contentDetails = root["items"][0]["contentDetails"];
						if (!contentDetails["duration"].empty()
								&& contentDetails["duration"].isString()) {
							const CStringA sDuration = contentDetails["duration"].asCString();
							WORD hh = 0, mm = 0, ss = 0;
							if (sDuration.Find("H") > 0) {
								sscanf_s(sDuration, "PT%huH%huM%huS", &hh, &mm, &ss);
							} else if (sDuration.Find("M") > 0) {
								sscanf_s(sDuration, "PT%huM%huS", &mm, &ss);
							} else if (sDuration.Find("S") > 0) {
								sscanf_s(sDuration, "PT%huS", &ss);
							}

							y_fields.duration = (ss + 60 * mm + 3600 * hh) * UNITS;
						}
					}

					free(data);

					return true;
				}
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

	bool Parse_URL(CString url, CAtlList<CString>& urls, YOUTUBE_FIELDS& y_fields, CSubtitleItemList& subs)
	{
		if (CheckURL(url)) {
			char* data = NULL;
			DWORD dataSize = 0;

			CString videoId;

			HINTERNET f, s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
			if (s) {
				CString link = url;
				HandleURL(link);
				if (link.Find(YOUTU_BE_URL) != -1) {
					link.Replace(YOUTU_BE_URL, YOUTUBE_URL);
					link.Replace(_T("watch?"), _T("watch?v="));
				} else if (link.Find(YOUTUBE_URL_V) != -1) {
					link.Replace(_T("v/"), _T("watch?v="));
				}

				videoId = RegExpParse(link, L"v={[-a-zA-Z0-9_]+}");

				f = InternetOpenUrl(s, link, NULL, 0, INTERNET_OPEN_FALGS, 0);
				if (f) {
					InternetReadData(f, &data, dataSize, NULL);
					InternetCloseHandle(f);
				}
			}

			if (!data) {
				if (s) {
					InternetCloseHandle(s);
				}

				return false;
			}

			const CString Title = UTF8To16(GetEntry(data, "<title>", "</title>"));
			y_fields.title = FixHtmlSymbols(Title);

			// "hlspv" - live streaming
			const CStringA hlspv_url = GetEntry(data, MATCH_HLSVP_START, MATCH_END);
			if (!hlspv_url.IsEmpty()) {
				url = UrlDecode(UrlDecode(hlspv_url));
				url.Replace(L"\\/", L"/");
				urls.AddHead(url);

				InternetCloseHandle(s);
				free(data);
				return true;
			}

			// url_encoded_fmt_stream_map
			const CStringA stream_map = GetEntry(data, MATCH_STREAM_MAP_START, MATCH_END);
			if (stream_map.IsEmpty()) {
				free(data);
				InternetCloseHandle(s);
				return false;
			}
			// adaptive_fmts
			const CStringA adaptive_fmts = GetEntry(data, MATCH_ADAPTIVE_FMTS_START, MATCH_END);

			CStringA strA = stream_map;
			if (!adaptive_fmts.IsEmpty()) {
				strA += ',';
				strA += adaptive_fmts;
			}
			strA.Replace("\\u0026", "&");

			CAtlArray<youtubeFuncType> JSFuncs;
			CAtlArray<int> JSFuncArgs;
			BOOL bJSParsed = FALSE;
			CString JSUrl = UTF8To16(GetEntry(data, MATCH_JS_START, MATCH_END));
			if (!JSUrl.IsEmpty()) {
				JSUrl.Replace(L"\\/", L"/");
				JSUrl = L"https:" + JSUrl;
			}
			free(data);

			const CAppSettings& sApp = AfxGetAppSettings();
			const YOUTUBE_PROFILES* youtubeVideoSets = getProfile(sApp.iYoutubeTag, youtubeProfiles::VIDEO_PROFILE);
			if (youtubeVideoSets->iTag == 0) {
				youtubeVideoSets = getProfile(22, youtubeProfiles::VIDEO_PROFILE);
			}

			CString final_video_url;
			CString final_video_ext;
			int final_video_itag = 0;

			const YOUTUBE_PROFILES* youtubeAudioSets = getProfile(251, youtubeProfiles::AUDIO_PROFILE);

			CString final_audio_url;
			CString final_audio_ext;
			int final_audio_itag = 0;

			CAtlList<CStringA> linesA;
			Explode(strA, linesA, ',');

			POSITION posLine = linesA.GetHeadPosition();
			while (posLine) {
				CStringA &lineA = linesA.GetNext(posLine);

				int itag = 0;
				CStringA url;
				CStringA signature;

				CAtlList<CStringA> paramsA;
				Explode(lineA, paramsA, '&');

				POSITION posParam = paramsA.GetHeadPosition();
				while (posParam) {
					CStringA &paramA = paramsA.GetNext(posParam);

					int k = paramA.Find('=');
					if (k > 0) {
						CStringA paramHeader = paramA.Left(k);
						CStringA paramValue = paramA.Mid(k + 1);

						// "quality", "fallback_host", "url", "itag", "type", "s"
						if (paramHeader == "url") {
							url = UrlDecode(UrlDecode(paramValue));
						} else if (paramHeader == "s") {
							signature = paramValue;
						} else if (paramHeader == "itag") {
							if (sscanf_s(paramValue, "%d", &itag) != 1) {
								itag = 0;
							}
						}
					}
				}

				if (itag) {
					auto SignatureDecode = [&](CString& final_url) {
						if (!signature.IsEmpty() && !JSUrl.IsEmpty()) {
							if (!bJSParsed) {
								bJSParsed = TRUE;
								f = InternetOpenUrl(s, JSUrl, NULL, 0, INTERNET_OPEN_FALGS, 0);
								if (f) {
									char* data = NULL;
									DWORD dataSize = 0;
									InternetReadData(f, &data, dataSize, NULL);
									InternetCloseHandle(f);
									if (dataSize) {
										const CStringA funcName = GetEntry(data, "\"signature\",", "(");
										if (!funcName.IsEmpty()) {
											const CStringA varfunc = funcName + "=function(a){";
											const CStringA funcBody = GetEntry(data, varfunc, "};");
											if (!funcBody.IsEmpty()) {
												CStringA funcGroup;
												CAtlList<CStringA> funcList;
												CAtlList<CStringA> funcCodeList;

												CAtlList<CStringA> code;
												Explode(funcBody, code, ';');

												POSITION pos = code.GetHeadPosition();
												while (pos) {
													const CStringA &line = code.GetNext(pos);

													if (line.Find("split") >= 0 || line.Find("return") >= 0) {
														continue;
													}

													funcList.AddTail(line);

													if (funcGroup.IsEmpty()) {
														int k = line.Find('.');
														if (k > 0) {
															funcGroup = line.Left(k);
														}
													}
												}

												if (!funcGroup.IsEmpty()) {
													CStringA tmp = "var " + funcGroup + "={";
													tmp = GetEntry(data, tmp, "};");
													if (!tmp.IsEmpty()) {
														Explode(tmp, funcCodeList, "},");
													}
												}

												if (!funcList.IsEmpty() && !funcCodeList.IsEmpty()) {
													funcGroup += '.';

													POSITION pos = funcList.GetHeadPosition();
													while (pos) {
														const CStringA& func = funcList.GetNext(pos);

														int funcArg = 0;
														CStringA funcArgs = GetEntry(func, "(", ")");
														CAtlList<CStringA> args;
														Explode(funcArgs, args, ',');
														if (args.GetCount() >= 1) {
															CStringA& arg = args.GetTail();
															int value = 0;
															if (sscanf_s(arg, "%d", &value) == 1) {
																funcArg = value;
															}
														}

														CStringA funcName = GetEntry(func, funcGroup, "(");
														funcName += ":function";

														youtubeFuncType funcType = youtubeFuncType::funcNONE;

														POSITION pos2 = funcCodeList.GetHeadPosition();
														while (pos2) {
															const CStringA& funcCode = funcCodeList.GetNext(pos2);
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
															JSFuncs.Add(funcType);
															JSFuncArgs.Add(funcArg);
														}
													}
												}
											}
										}

										free(data);
									}
								}
							}
						}

						if (!JSFuncs.IsEmpty()) {
							auto Delete = [](CStringA& a, int b) {
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

							for (size_t i = 0; i < JSFuncs.GetCount(); i++) {
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

							final_url.AppendFormat(L"&signature=%s", CString(signature));
						}
					};

					if (final_video_itag != youtubeVideoSets->iTag && SelectBestProfile(final_video_itag, final_video_ext, itag, youtubeVideoSets, youtubeProfiles::VIDEO_PROFILE)) {
						final_video_url = url;
						SignatureDecode(final_video_url);
					} else if (SelectBestProfile(final_audio_itag, final_audio_ext, itag, youtubeAudioSets, youtubeProfiles::AUDIO_PROFILE)) {
						final_audio_url = url;
						SignatureDecode(final_audio_url);
					}

					if (final_video_itag == youtubeVideoSets->iTag && youtubeVideoSets->videoOnly == false) {
						break;
					}
				}
			}

			if (!final_audio_url.IsEmpty()) {
				final_audio_url.Replace(L"http://", L"https://");
			}

			if (!final_video_url.IsEmpty()) {
				final_video_url.Replace(L"http://", L"https://");

				ParseMetadata(s, videoId, y_fields);

				y_fields.fname = y_fields.title + final_video_ext;
				FixFilename(y_fields.fname);

				if (!videoId.IsEmpty()) { // subtitle
					CString link;
					link.Format(L"https://video.google.com/timedtext?hl=en&type=list&v=%s", videoId);
					f = InternetOpenUrl(s, link, NULL, 0, INTERNET_OPEN_FALGS, 0);
					if (f) {
						char* data = NULL;
						DWORD dataSize = 0;
						InternetReadData(f, &data, dataSize, NULL);
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
											url.Format(L"https://www.youtube.com/api/timedtext?lang=%s&v=%s&fmt=vtt", xmlValue, videoId);
										} else if (xmlHeader == L"lang_original") {
											name = xmlValue;
										}
									}

									if (!url.IsEmpty() && !name.IsEmpty()) {
										subs.AddTail(CSubtitleItem(url, name));
									}
								}
							}
						}
					}
				}

				InternetCloseHandle(s);

				if (!final_video_url.IsEmpty()) {
					urls.AddHead(final_video_url);

					if (!final_audio_url.IsEmpty()) {
						const YOUTUBE_PROFILES* current = getProfile(final_video_itag, youtubeProfiles::VIDEO_PROFILE);
						if (current->videoOnly == true) {
							urls.AddTail(final_audio_url);
						}
					}
				}

				return !urls.IsEmpty();
			}
		}

		return false;
	}

	bool Parse_Playlist(CString url, YoutubePlaylist& youtubePlaylist, int& idx_CurrentPlay)
	{
		idx_CurrentPlay = 0;
		if (CheckPlaylist(url)) {

			char* data = NULL;
			DWORD dataSize = 0;

			HINTERNET f, s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
			if (s) {
				CString link(url);
				HandleURL(link);
				f = InternetOpenUrl(s, link, NULL, 0, INTERNET_OPEN_FALGS, 0);
				if (f) {
					InternetReadData(f, &data, dataSize, "id=\"footer\"");
					InternetCloseHandle(f);
				}
				InternetCloseHandle(s);
			}

			if (!data) {
				return false;
			}

			LPCSTR sMatch = NULL;
			if (strstr(data, MATCH_PLAYLIST_ITEM_START)) {
				sMatch = MATCH_PLAYLIST_ITEM_START;
			} else if (strstr(data, MATCH_PLAYLIST_ITEM_START2)) {
				sMatch = MATCH_PLAYLIST_ITEM_START2;
			} else {
				free(data);
				return false;
			}
			
			LPCSTR block = data;
			while ((block = strstr(block, sMatch)) != NULL) {
				const CStringA blockEntry = GetEntry(block, sMatch, ">");
				if (blockEntry.IsEmpty()) {
					break;
				}
				block += blockEntry.GetLength();
				
				CString item = UTF8To16(blockEntry);
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

			free(data);

			if (!youtubePlaylist.IsEmpty()) {
				return true;
			}
		}

		return false;
	}

	bool Parse_URL(CString url, YOUTUBE_FIELDS& y_fields)
	{
		bool bRet = false;
		if (CheckURL(url)) {
			HINTERNET s = InternetOpen(L"Googlebot", 0, NULL, NULL, 0);
			if (s) {
				CString link(url);
				HandleURL(link);
				if (link.Find(YOUTU_BE_URL) != -1) {
					link.Replace(YOUTU_BE_URL, YOUTUBE_URL);
					link.Replace(_T("watch?"), _T("watch?v="));
				} else if (link.Find(YOUTUBE_URL_V) != -1) {
					link.Replace(_T("v/"), _T("watch?v="));
				}

				CString videoId = RegExpParse(link, L"v={[-a-zA-Z0-9_]+}");
				bRet = ParseMetadata(s, videoId, y_fields);
				
				InternetCloseHandle(s);
			}
		}
	
		return bRet;
	}
}
