/*
 * (C) 2018-2021 see Authors.txt
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
#include "PlayerYouTubeDL.h"

#include <ExtLib/rapidjson/include/rapidjsonHelper.h>

#define bufsize (2ul * KILOBYTE)

namespace YoutubeDL
{
	std::vector<std::unique_ptr<Youtube::YoutubeProfile>> YoutubeProfiles;

	bool Parse_URL(const CString& url, const CString& ydlExePath, const int maxHeightOptions, const bool bMaximumQuality,
		std::list<CString>& urls, CSubtitleItemList& subs, Youtube::YoutubeFields& y_fields,
		Youtube::YoutubeUrllist& youtubeUrllist, Youtube::YoutubeUrllist& youtubeAudioUrllist)
	{
		y_fields.Empty();
		YoutubeProfiles.clear();
		urls.clear();

		CStringW ydl_path;

		CStringW apppath = GetProgramDir() + ydlExePath;
		if (::PathFileExistsW(apppath)) {
			ydl_path = apppath;
		}
		else {
			// CreateProcessW and other functions (unlike ShellExecuteExW) does not look for
			// an executable file in the "App Paths", so we will do it manually.

			// see "App Paths" in HKEY_CURRENT_USER
			apppath = GetRegAppPath(ydlExePath, true);
			if (apppath.GetLength() && ::PathFileExistsW(apppath)) {
				ydl_path = apppath;
			}
			else {
				// see "App Paths" in HKEY_LOCAL_MACHINE
				apppath = GetRegAppPath(ydlExePath, false);
				if (apppath.GetLength() && ::PathFileExistsW(apppath)) {
					ydl_path = apppath;
				}
			}
		}

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

		CStringW args;
		args.Format(LR"(%s -j --all-subs --sub-format vtt "%s")", ydl_path.GetString(), url.GetString());
		PROCESS_INFORMATION proc_info = {};
		const bool bSuccess = CreateProcessW(nullptr, args.GetBuffer(), nullptr, nullptr, true, 0,
											 nullptr, nullptr, &startup_info, &proc_info);
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
				AfxMessageBox(CStringW(buf_err), MB_ICONERROR, 0);
			}
		}

		CloseHandle(proc_info.hProcess);
		CloseHandle(proc_info.hThread);
		CloseHandle(hStdout_r);
		CloseHandle(hStderr_r);

		if (!exitcode && buf_out.GetLength()) {
			rapidjson::Document d;
			if (!d.Parse(buf_out.GetString()).HasParseError()) {
				int iTag = 1;
				if (auto formats = GetJsonArray(d, "formats")) {
					int vid_height = 0;
					bool bVideoOnly = false;

					float aud_bitrate = 0.0f;
					CStringA bestAudioUrl;
					std::map<CStringA, CStringA> audioUrls;

					CStringA bestUrl;
					getJsonValue(d, "url", bestUrl);

					for (const auto& format : formats->GetArray()) {
						CStringA protocol;
						if (!getJsonValue(format, "protocol", protocol)
								|| (protocol != "http" && protocol != "https" && protocol.Left(4) != "m3u8")) {
							continue;
						}
						CStringA url;
						if (!getJsonValue(format, "url", url)) {
							continue;
						}
						CStringA vcodec;
						if (!getJsonValue(format, "vcodec", vcodec) || vcodec == "none") {
							continue;
						}

						int height = 0;
						if (getJsonValue(format, "height", height)) {
							CStringA acodec;
							getJsonValue(format, "acodec", acodec);

							auto profilePtr = std::make_unique<Youtube::YoutubeProfile>();
							auto profile = profilePtr.get();
							YoutubeProfiles.push_back(std::move(profilePtr));

							CString ext;
							getJsonValue(format, "ext", ext);
							CString fmt;
							getJsonValue(format, "format", fmt);

							profile->format = Youtube::yformat::y_mp4_other;
							if (ext == L"mp4") {
								if (StartsWith(vcodec, "avc1")) {
									profile->format = Youtube::yformat::y_mp4_avc;
								}
								else if (StartsWith(vcodec, "av01")) {
									profile->format = Youtube::yformat::y_mp4_av1;
								}
								else {
									profile->format = Youtube::yformat::y_mp4_other;
								}
							}
							else if (ext == L"webm") {
								profile->format = Youtube::yformat::y_webm_vp9;
							}
							profile->type = acodec == "none" ? Youtube::ytype::y_video : Youtube::ytype::y_media;
							profile->ext = ext;
							profile->quality = height;
							profile->iTag = iTag++;

							Youtube::YoutubeUrllistItem item;
							item.profile = profile;
							item.url = CString(url);

							if (!ext.IsEmpty()) {
								item.title = ext.MakeUpper();
								if (!vcodec.IsEmpty()) {
									item.title.AppendFormat(L"(%S)", vcodec);
								}
							}
							if (!fmt.IsEmpty()) {
								if (item.title.IsEmpty()) {
									item.title = fmt;
								} else {
									item.title.AppendFormat(L" - %s", fmt);
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

					for (const auto& format : formats->GetArray()) {
						CStringA protocol;
						if (!getJsonValue(format, "protocol", protocol)
							|| (protocol != "http" && protocol != "https" && protocol.Left(4) != "m3u8")) {
							continue;
						}
						CStringA url;
						if (!getJsonValue(format, "url", url)) {
							continue;
						}
						CStringA vcodec;
						if (!getJsonValue(format, "vcodec", vcodec) || vcodec != "none") {
							continue;
						}

						float tbr = 0.0f;
						if (getJsonValue(format, "tbr", tbr)) {
							if (aud_bitrate == 0.0f
								||(vid_height > 360 && tbr > aud_bitrate)
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
							if (const auto it = audioUrls.find(language); it == audioUrls.cend()) {
								audioUrls[language] = url;
							}
						}
					}

					if (!bestUrl.IsEmpty()) {
						if (bMaximumQuality) {
							float maxVideotbr = 0.0f;
							int maxVideofps = 0;

							for (const auto& format : formats->GetArray()) {
								CStringA protocol;
								if (!getJsonValue(format, "protocol", protocol)
										|| (protocol != "http" && protocol != "https" && protocol.Left(4) != "m3u8")) {
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
										bestUrl = url;
										bVideoOnly = false;

										CStringA acodec;
										if (getJsonValue(format, "acodec", acodec)) {
											if (acodec == "none") {
												bVideoOnly = true;
											}
										}
									}
								}
							}
						}

						if (getJsonValue(d, "title", y_fields.title)) {
							CStringA ext;
							if (getJsonValue(d, "ext", ext)) {
								y_fields.fname.Format(L"%s.%S", y_fields.title.GetString(), ext.GetString());
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

						urls.push_back(CString(bestUrl));
						if (bVideoOnly) {
							if (audioUrls.size() > 1) {
								for (const auto& [language, audioUrl] : audioUrls) {
									urls.push_back(CString(audioUrl));
								}
							} else if (!bestAudioUrl.IsEmpty()) {
								urls.push_back(CString(bestAudioUrl));
							}
						}

						if (!bestAudioUrl.IsEmpty()) {
							auto profilePtr = std::make_unique<Youtube::YoutubeProfile>();
							auto profile = profilePtr.get();
							YoutubeProfiles.push_back(std::move(profilePtr));

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

						// subtitles
						if (auto requested_subtitles = GetJsonObject(d, "requested_subtitles")) {
							for (const auto& subtitle : requested_subtitles->GetObj()) {
								CString sub_url;
								getJsonValue(subtitle.value, "url", sub_url);
								CString sub_lang = UTF8ToWStr(subtitle.name.GetString());

								if (!sub_url.IsEmpty() && !sub_lang.IsEmpty()) {
									subs.push_back({ sub_url, sub_lang });
								}
							}
						}

						// chapters
						if (auto chapters = GetJsonArray(d, "chapters")) {
							for (const auto& chapter : chapters->GetArray()) {
								float start_time = 0.0f;
								CString title;
								if (getJsonValue(chapter, "title", title) && getJsonValue(chapter, "start_time", start_time)) {
									y_fields.chaptersList.push_back({ title, REFERENCE_TIME(start_time * UNITS) });
								}
							}
						}
					}
				}
			}
		}

		return !urls.empty();
	}

	void Clear()
	{
		YoutubeProfiles.clear();
	}
}
