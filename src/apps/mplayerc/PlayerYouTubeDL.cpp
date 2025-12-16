/*
 * (C) 2018-2025 see Authors.txt
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
#include "DSUtil/Filehandle.h"
#include "DSUtil/text.h"
#include "PlayerYouTubeDL.h"

#include "rapidjsonHelper.h"

#define bufsize (2ul * KILOBYTE)

namespace YoutubeDL
{
	std::vector<std::unique_ptr<Youtube::YoutubeProfile>> YoutubeProfiles;

	bool Parse_URL(
		const CStringW& url,        // input parameter
		const CStringW& ydlExePath, // input parameter
		const int maxHeightOptions, // input parameter
		const bool bMaximumQuality, // input parameter
		CStringA lang,              // input parameter
		Youtube::YoutubeFields& y_fields,
		Youtube::YoutubeUrllist& youtubeUrllist,
		Youtube::YoutubeUrllist& youtubeAudioUrllist,
		OpenFileData* pOFD
	)
	{
		y_fields.Empty();
		YoutubeProfiles.clear();
		pOFD->Clear();

		const CStringW ydl_path = GetFullExePath(ydlExePath, true);
		const CStringW ydl_fname = GetFileName(ydlExePath);

		if (ydl_path.IsEmpty()) {
			return false;
		}

		HANDLE hStdout_r, hStdout_w;
		HANDLE hStderr_r, hStderr_w;

		SECURITY_ATTRIBUTES sec_attrib = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
		if (!CreatePipe(&hStdout_r, &hStdout_w, &sec_attrib, bufsize)) {
			return false;
		}
		if (!CreatePipe(&hStderr_r, &hStderr_w, &sec_attrib, bufsize)) {
			CloseHandle(hStdout_r);
			CloseHandle(hStdout_w);
			return false;
		}

		STARTUPINFOW startup_info = {};
		startup_info.cb = sizeof(STARTUPINFO);
		startup_info.hStdOutput = hStdout_w;
		startup_info.hStdError = hStderr_w;
		startup_info.wShowWindow = SW_HIDE;
		startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

		PROCESS_INFORMATION proc_info = {};
		CStringW args;
		if (ydl_fname.CompareNoCase(L"yt-dlp.exe") == 0) {
			// yt-dlp.exe
			args.Format(LR"(%s -j --all-subs --sub-format vtt --no-check-certificates "%s")", ydl_path.GetString(), url.GetString());
		} else {
			// youtube-dl.exe
			args.Format(LR"(%s -j --all-subs --sub-format vtt --no-check-certificate "%s")", ydl_path.GetString(), url.GetString());
		}

		const BOOL bSuccess = CreateProcessW(
			nullptr, args.GetBuffer(), nullptr, nullptr, TRUE, 0,
			nullptr, nullptr, &startup_info, &proc_info
		);
		CloseHandle(hStdout_w);
		CloseHandle(hStderr_w);

		if (!bSuccess) {
			CloseHandle(proc_info.hProcess);
			CloseHandle(proc_info.hThread);
			CloseHandle(hStdout_r);
			CloseHandle(hStderr_r);
			return false;
		}

		CStringA buf_out;
		CStringA buf_err;

		std::thread stdout_thread;
		std::thread stderr_thread;

		if (hStdout_r) {
			stdout_thread = std::thread([&]() {
				char buffer[bufsize] = { 0 };
				DWORD dwBytesRead = 0;
				while (ReadFile(hStdout_r, buffer, bufsize, &dwBytesRead, nullptr)) {
					if (!dwBytesRead) {
						break;
					}
					buf_out.Append(buffer, dwBytesRead);
				}
				});
		}

		if (hStderr_r) {
			stderr_thread = std::thread([&]() {
				char buffer[bufsize] = { 0 };
				DWORD dwBytesRead = 0;
				while (ReadFile(hStderr_r, buffer, bufsize, &dwBytesRead, nullptr)) {
					if (!dwBytesRead) {
						break;
					}
					buf_err.Append(buffer, dwBytesRead);
				}
				});
		}

		WaitForSingleObject(proc_info.hProcess, INFINITE);

		if (stdout_thread.joinable()) {
			stdout_thread.join();
		}
		if (stderr_thread.joinable()) {
			stderr_thread.join();
		}

		DWORD exitcode;
		GetExitCodeProcess(proc_info.hProcess, &exitcode);
		if (exitcode && buf_err.GetLength()) {
			if (buf_err.Find("Unsupported URL:") == -1) {
				MessageBoxW(AfxGetApp()->GetMainWnd()->m_hWnd, CStringW(buf_err), ydl_fname, MB_ICONERROR | MB_OK);
			}
		}

		CloseHandle(proc_info.hProcess);
		CloseHandle(proc_info.hThread);
		CloseHandle(hStdout_r);
		CloseHandle(hStderr_r);

		if (!exitcode && buf_out.GetLength()) {
			rapidjson::Document d;
			const int k = buf_out.Find("\n{\"id\": ", 64); // check presence of second JSON root element and ignore it
			if (!d.Parse(buf_out.GetString(), k > 0 ? k : buf_out.GetLength()).HasParseError()) {
				bool bIsYoutube = Youtube::CheckURL(url);
				int iTag = 1;

				if (auto formats = GetJsonArray(d, "formats")) {
					int vid_height = 0;
					bool bVideoOnly = false;

					float aud_bitrate = 0.0f;
					CStringA bestAudioUrl;
					struct audio_info_t {
						float tbr = 0;
						CStringA url;
					};
					std::map<CStringA, audio_info_t> audioUrls;

					CStringA bestUrl;
					getJsonValue(d, "url", bestUrl);

					auto GetAndCheckProtocol = [](const auto& json, CStringA& protocol) {
						if (!getJsonValue(json, "protocol", protocol)
								|| (!StartsWith(protocol, "http") && !EndsWith(protocol, "m3u8") && !EndsWith(protocol, "m3u8_native"))) {
							return false;
						}

						return true;
					};

					CStringA liveStatus;
					getJsonValue(d, "live_status", liveStatus);
					bool bIsLive = liveStatus == "is_live";

					for (const auto& format : formats->GetArray()) {
						CStringA protocol;
						if (!GetAndCheckProtocol(format, protocol)) {
							continue;
						}
						CStringA url;
						if (!getJsonValue(format, "url", url)) {
							continue;
						}
						CStringA vcodec;
						if (!getJsonValue(format, "vcodec", vcodec) || vcodec == "none") {
							CStringA video_ext;
							if (!getJsonValue(format, "video_ext", video_ext) || video_ext == "none") {
								continue;
							}
						}

						int height = 0;
						if (getJsonValue(format, "height", height)) {
							CStringA acodec;
							getJsonValue(format, "acodec", acodec);

							Youtube::YoutubeUrllistItem item = {};
							item.url = url;

							CStringA format_id = {};
							int itag = {};
							if (bIsYoutube && getJsonValue(format, "format_id", format_id) && StrToInt32(format_id.GetString(), itag)) {
								if (auto profile = Youtube::GetProfile(itag)) {
									item.profile = profile;
								}
							}

							CString ext;
							getJsonValue(format, "ext", ext);
							CString fmt;
							getJsonValue(format, "format", fmt);

							if (!item.profile) {
								auto profilePtr = std::make_unique<Youtube::YoutubeProfile>();
								auto profile = profilePtr.get();
								YoutubeProfiles.emplace_back(std::move(profilePtr));

								profile->format = Youtube::yformat::y_mp4_other;
								if (EndsWith(protocol, "m3u8") || EndsWith(protocol, "m3u8_native")) {
									if (bIsLive && acodec == "none") {
										continue;
									}
									profile->format = Youtube::yformat::y_stream;
								} else if (ext == L"mp4") {
									if (StartsWith(vcodec, "avc1")) {
										profile->format = Youtube::yformat::y_mp4_avc;
									} else if (StartsWith(vcodec, "av01")) {
										profile->format = Youtube::yformat::y_mp4_av1;
									} else {
										profile->format = Youtube::yformat::y_mp4_other;
									}
								} else if (ext == L"webm") {
									profile->format = Youtube::yformat::y_webm_vp9;
								}
								profile->type = acodec == "none" ? Youtube::ytype::y_video : Youtube::ytype::y_media;
								profile->ext = ext;
								profile->quality = height;
								profile->iTag = iTag++;

								item.profile = profile;
							}

							if (!ext.IsEmpty()) {
								item.title = ext.MakeUpper();
								if (!vcodec.IsEmpty()) {
									item.title.AppendFormat(L"(%hs)", vcodec.GetString());
								}
							}
							if (!fmt.IsEmpty()) {
								if (item.title.IsEmpty()) {
									item.title = fmt;
								} else {
									item.title.AppendFormat(L" - %s", fmt.GetString());
								}
							}

							youtubeUrllist.emplace_back(item);

							if (height > maxHeightOptions) {
								continue;
							}
							if ((height > vid_height)
									|| (height == vid_height && StartsWith(protocol, "http"))) {
								vid_height = height;
								bestUrl = url;
								bVideoOnly = false;

								if (acodec == "none") {
									bVideoOnly = true;
								}
							}
						}
					}

					// Find default/preference audio language.
					// yt-dlp can mark several languages as preferred, we choose the one with the highest "language_preference".
					CStringA defaultLanguage = lang;
					if (bIsYoutube) {
						int defaultLanguagePreference = -1;

						for (const auto& format : formats->GetArray()) {
							CStringA protocol;
							if (!GetAndCheckProtocol(format, protocol)) {
								continue;
							}
							CStringA url;
							if (!getJsonValue(format, "url", url)) {
								continue;
							}
							CStringA vcodec;
							if (!getJsonValue(format, "vcodec", vcodec) || vcodec != "none") {
								if (!getJsonValue(format, "video_ext", vcodec) || vcodec == "none") {
									continue;
								}
							}
							CStringA acodec;
							if (!getJsonValue(format, "acodec", acodec) || acodec == "none") {
								if (!getJsonValue(format, "audio_ext", acodec) || acodec == "none") {
									continue;
								}
							}
							CStringA format_id;
							if (getJsonValue(format, "format_id", format_id) && EndsWith(format_id, "-drc")) {
								continue;
							}

							CStringA language;
							if (getJsonValue(format, "language", language)) {
								int language_preference = 0;
								if (getJsonValue(format, "language_preference", language_preference)
										&& language_preference > 0
										&& language_preference > defaultLanguagePreference) {
									defaultLanguage = language;
									defaultLanguagePreference = language_preference;
								}
							}
						}
					}

					std::map<CStringA, Youtube::YoutubeUrllist> youtubeAudioUrllistWithLanguages;

					for (const auto& format : formats->GetArray()) {
						CStringA protocol;
						if (!GetAndCheckProtocol(format, protocol)) {
							continue;
						}
						CStringA url;
						if (!getJsonValue(format, "url", url)) {
							continue;
						}
						CStringA vcodec;
						if (!getJsonValue(format, "vcodec", vcodec) || vcodec != "none") {
							if (!getJsonValue(format, "video_ext", vcodec) || vcodec == "none") {
								continue;
							}
						}
						CStringA acodec;
						if (!getJsonValue(format, "acodec", acodec) || acodec == "none") {
							if (!getJsonValue(format, "audio_ext", acodec) || acodec == "none") {
								continue;
							}
						}
						CStringA format_id;
						if (getJsonValue(format, "format_id", format_id) && EndsWith(format_id, "-drc")) {
							continue;
						}

						CString ext;
						getJsonValue(format, "ext", ext);
						CString fmt;
						getJsonValue(format, "format", fmt);
						float tbr = -1.f;
						getJsonValue(format, "tbr", tbr);

						int itag = {};
						if (bIsYoutube && !format_id.IsEmpty() && StrToInt32(format_id.GetString(), itag)) {
							if (auto audioprofile = Youtube::GetAudioProfile(itag)) {
								Youtube::YoutubeUrllistItem item;
								item.profile = audioprofile;
								item.url = url;

								if (!ext.IsEmpty()) {
									item.title = ext.MakeUpper();
									if (!acodec.IsEmpty()) {
										item.title.AppendFormat(L"(%hs)", acodec.GetString());
									}
								}
								if (!fmt.IsEmpty()) {
									if (item.title.IsEmpty()) {
										item.title = fmt;
									} else {
										item.title.AppendFormat(L" - %s", fmt.GetString());
									}
								}

								if (tbr > 0.f) {
									item.title.AppendFormat(L" %dkbit/s", lround(tbr));
								} else {
									item.title.AppendFormat(L" %dkbit/s", audioprofile->quality);
								}

								CStringA language;
								if (getJsonValue(format, "language", language)) {
									youtubeAudioUrllistWithLanguages[language].emplace_back(item);
								} else {
									youtubeAudioUrllist.emplace_back(item);
								}
							}
						} else {
							if (tbr > 0.f) {
								if (aud_bitrate == 0.0f
										|| (vid_height > 360 && tbr > aud_bitrate)
										|| (vid_height <= 360 && tbr < aud_bitrate)
										|| (tbr == aud_bitrate && StartsWith(protocol, "http"))) {
									aud_bitrate = tbr;
									bestAudioUrl = url;
								}
							} else if (bestAudioUrl.IsEmpty()) {
								bestAudioUrl = url;
							}

							CStringA language;
							if (getJsonValue(format, "language", language)) {
								const auto it = audioUrls.find(language);
								if (it == audioUrls.cend() || tbr > (*it).second.tbr) {
									audioUrls[language] = { tbr, url };
									if (bIsYoutube) {
										int language_preference = 0;
										if (getJsonValue(format, "language_preference", language_preference) && language_preference > 0) {
											audioUrls[CStringA(Youtube::kDefaultAudioLanguage)] = { tbr, url };
										}
									}
								}
							}
						}
					}

					if (!bestUrl.IsEmpty()) {
						if (bIsYoutube) {
							if (!youtubeAudioUrllistWithLanguages.empty()) {
								if (lang == Youtube::kDefaultAudioLanguage) {
									lang = defaultLanguage;
								}

								auto it = youtubeAudioUrllistWithLanguages.find(lang);
								if (it == youtubeAudioUrllistWithLanguages.end()) {
									lang.AppendChar('-');
									it = std::find_if(youtubeAudioUrllistWithLanguages.begin(), youtubeAudioUrllistWithLanguages.end(), [&lang](const auto& pair) {
										return StartsWith(pair.first, lang);
									});
									if (it == youtubeAudioUrllistWithLanguages.end()) {
										if (lang != defaultLanguage) {
											it = youtubeAudioUrllistWithLanguages.find(defaultLanguage);
										}

										if (it == youtubeAudioUrllistWithLanguages.end()) {
											it = std::find_if(youtubeAudioUrllistWithLanguages.begin(), youtubeAudioUrllistWithLanguages.end(), [](const auto& pair) {
												return pair.first == "en-US" || pair.first == "en";
											});
											if (it == youtubeAudioUrllistWithLanguages.end()) {
												it = youtubeAudioUrllistWithLanguages.begin();
											}
										}
									}
								}

								for (const auto& item : it->second) {
									youtubeAudioUrllist.emplace_back(item);
								}
							}

							// For YouTube, we will select the quality/codec depending on the settings of the built-in parser.
							std::sort(youtubeUrllist.begin(), youtubeUrllist.end(), Youtube::CompareUrllistItem);
							std::sort(youtubeAudioUrllist.begin(), youtubeAudioUrllist.end(), Youtube::CompareUrllistItem);

							if (auto final_item = Youtube::SelectVideoStream(youtubeUrllist)) {
								bestUrl = final_item->url;
								bVideoOnly = final_item->profile->type == Youtube::y_video;
							}
							if (auto final_item = Youtube::SelectAudioStream(youtubeAudioUrllist)) {
								bestAudioUrl = final_item->url;
							}

							for (const auto& item : youtubeAudioUrllist) {
								switch (item.profile->format) {
									case Youtube::y_mp4_aac:
									case Youtube::y_webm_opus:
									case Youtube::y_mp4_ac3:
									case Youtube::y_mp4_eac3:
									case Youtube::y_mp4_dtse:
										youtubeUrllist.emplace_back(item);
										break;
								}
							}

							pOFD->fi = CStringW(bestUrl);
							if (bVideoOnly && !bestAudioUrl.IsEmpty()) {
								pOFD->auds.emplace_back(CStringW(bestAudioUrl));
							}
						} else if (bMaximumQuality) {
							float maxVideotbr = 0.0f;
							int maxVideofps = 0;

							for (const auto& format : formats->GetArray()) {
								CStringA protocol;
								if (!GetAndCheckProtocol(format, protocol)) {
									continue;
								}

								CStringA url;
								if (!getJsonValue(format, "url", url)) {
									continue;
								}

								int height = 0;
								if (getJsonValue(format, "height", height) && height == vid_height) {
									float tbr = .0f;
									getJsonValue(format, "tbr", tbr);

									int fps = 0;
									getJsonValue(format, "fps", fps);

									bool bMaxQuality = false;
									if (fps > maxVideofps
											|| (fps == maxVideofps && tbr > maxVideotbr)) {
										bMaxQuality = true;
									}

									maxVideotbr = std::max(maxVideotbr, tbr);
									maxVideofps = std::max(maxVideofps, fps);

									if (bMaxQuality) {
										CStringA acodec;
										getJsonValue(format, "acodec", acodec);

										if (bIsLive && acodec == "none" && (EndsWith(protocol, "m3u8") || EndsWith(protocol, "m3u8_native"))) {
											continue;
										}

										bVideoOnly = acodec == "none";
										bestUrl = url;
									}
								}
							}
						}

						if (getJsonValue(d, "title", y_fields.title)) {
							CStringA ext;
							if (getJsonValue(d, "ext", ext)) {
								y_fields.fname.Format(L"%s.%hs", y_fields.title.GetString(), ext.GetString());
							}
						}

						getJsonValue(d, "uploader", y_fields.author);

						getJsonValue(d, "description", y_fields.content);
						if (y_fields.content.Find('\n') && y_fields.content.Find(L"\r\n") == -1) {
							y_fields.content.Replace(L"\n", L"\r\n");
						}

						CStringA upload_date;
						if (getJsonValue(d, "upload_date", upload_date)) {
							WORD y, m, d;
							if (sscanf_s(upload_date.GetString(), "%04hu%02hu%02hu", &y, &m, &d) == 3) {
								y_fields.dtime.wYear = y;
								y_fields.dtime.wMonth = m;
								y_fields.dtime.wDay = d;
							}
						}

						if (!bIsYoutube) {
							pOFD->fi = CStringW(bestUrl);
							if (bVideoOnly) {
								if (audioUrls.size() > 1) {
									auto select = audioUrls.begin();

									if (auto it = audioUrls.find(lang); it != audioUrls.end()) {
										select = it;
									} else if (auto it = audioUrls.find("en"); it != audioUrls.end()) {
										select = it;
									}

									pOFD->auds.emplace_back(CStringW(select->second.url), CStringW(select->first), select->first);
								} else if (bestAudioUrl.GetLength()) {
									pOFD->auds.emplace_back(CStringW(bestAudioUrl));
								}
							}

							if (!bestAudioUrl.IsEmpty()) {
								auto profilePtr = std::make_unique<Youtube::YoutubeProfile>();
								auto profile = profilePtr.get();
								YoutubeProfiles.emplace_back(std::move(profilePtr));

								profile->type = Youtube::ytype::y_audio;

								Youtube::YoutubeUrllistItem item;
								item.profile = profile;
								item.url = CString(bestAudioUrl);
								youtubeAudioUrllist.emplace_back(item);
							}

							if (!youtubeUrllist.empty()) {
								std::sort(youtubeUrllist.begin(), youtubeUrllist.end(), [](const Youtube::YoutubeUrllistItem& a, const Youtube::YoutubeUrllistItem& b) {
									if (a.profile->format != b.profile->format) {
										return (a.profile->format < b.profile->format);
									}
									if (a.profile->ext != b.profile->ext) {
										return (a.profile->ext < b.profile->ext);
									}
									if (a.profile->quality != b.profile->quality) {
										return (a.profile->quality > b.profile->quality);
									}

									return false;
								});
							}
						}

						// subtitles
						if (auto requested_subtitles = GetJsonObject(d, "requested_subtitles")) {
							for (const auto& subtitle : requested_subtitles->GetObj()) {
								CStringW sub_url;
								CStringW sub_name;
								getJsonValue(subtitle.value, "url", sub_url);
								getJsonValue(subtitle.value, "name", sub_name);
								CStringA sub_lang = subtitle.name.GetString();

								if (sub_url.GetLength() && sub_lang.GetLength()) {
									pOFD->subs.emplace_back(sub_url, sub_name, sub_lang);
								}
							}
						}

						// chapters
						if (auto chapters = GetJsonArray(d, "chapters")) {
							for (const auto& chapter : chapters->GetArray()) {
								float start_time = 0.0f;
								CString title;
								if (getJsonValue(chapter, "title", title) && getJsonValue(chapter, "start_time", start_time)) {
									y_fields.chaptersList.emplace_back(title, REFERENCE_TIME(start_time * UNITS));
								}
							}
						}

						if (bIsYoutube && !youtubeAudioUrllist.empty()) {
							// thumbnails
							if (auto thumbnails = GetJsonArray(d, "thumbnails")) {
								CStringA thumbnailUrl;
								int maxHeight = 0;
								for (const auto& elem : thumbnails->GetArray()) {
									int height = 0;
									if (getJsonValue(elem, "height", height) && height > maxHeight) {
										if (getJsonValue(elem, "url", thumbnailUrl)) {
											maxHeight = height;
										}
									}
								}

								if (!thumbnailUrl.IsEmpty()) {
									y_fields.thumbnailUrl = thumbnailUrl;
								}
							}
						}
					}
				}
			}
		}

		return pOFD->fi.Valid();
	}

	void Clear()
	{
		YoutubeProfiles.clear();
	}
}
