/*
 * (C) 2018 see Authors.txt
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
#include "PlayerYouTubeDL.h"

#define RAPIDJSON_SSE2
#include <rapidjson/include/rapidjson/document.h>

#define bufsize (2ul * KILOBYTE)

bool YoutubeDL::Parse_URL(const CString& url, const int maxHeightOptions, std::list<CString>& urls, Youtube::YoutubeFields& y_fields)
{
	urls.clear();

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

	CString args = "youtube-dl.exe -J -- \"" + url + "\"";
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
			char buffer[bufsize] = {0};
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
			char buffer[bufsize] = {0};
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
			const auto& formats = d["formats"];
			if (!formats.IsArray()) {
				return false;
			}

			int maxHeight = 0;
			CString bestUrl;
			bool bVideoOnly = false;

			float maxtbr = 0.0f;
			CString bestAudioUrl;

			for (rapidjson::SizeType i = 0; i < formats.Size(); i++) {
				const auto& format = formats[i];
				const auto& protocol = format["protocol"];
				if (!protocol.IsString()) {
					continue;
				}
				const CString _protocol(protocol.GetString());
				if (_protocol != L"http" && _protocol != L"https" && _protocol.Left(4) != L"m3u8") {
					continue;
				}

				const auto& url = format["url"];
				if (!url.IsString()) {
					continue;
				}

				const auto& vcodec = format["vcodec"];
				if (vcodec.IsString()) {
					const CString _vcodec = vcodec.GetString();
					if (_vcodec == L"none") {
						const auto& tbr = format["tbr"];
						if (tbr.IsFloat()) {
							const float _tbr = tbr.GetFloat();
							if ((_tbr > maxtbr)
									|| (_tbr == maxtbr && _protocol.Left(4) == L"http")) {
								maxtbr = _tbr;
								bestAudioUrl = url.GetString();
							}
						}
					}
				}

				const auto& height = format["height"];
				if (height.IsInt()) {
					const int _height = height.GetInt();
					if (_height > maxHeightOptions) {
						continue;
					}
					if ((_height > maxHeight)
							|| (_height == maxHeight && _protocol.Left(4) == L"http")) {
						maxHeight = _height;
						bestUrl = url.GetString();
						bVideoOnly = false;

						const auto& acodec = format["acodec"];
						if (acodec.IsString()) {
							const CString _acodec = acodec.GetString();
							if (_acodec == L"none") {
								bVideoOnly = true;
							}
						}
					}
				}
			}

			if (!bestUrl.IsEmpty()) {
				const auto& title = d["title"];
				if (title.IsString()) {
					y_fields.title = UTF8ToWStr(title.GetString());

					const auto& ext = d["ext"];
					if (ext.IsString()) {
						y_fields.fname.Format(L"%s.%S", y_fields.title, ext.GetString());
					}
				}
				const auto& description = d["description"];
				if (description.IsString()) {
					y_fields.content = UTF8ToWStr(description.GetString());
				}

				urls.push_back(bestUrl);
				if (bVideoOnly && !bestAudioUrl.IsEmpty()) {
					urls.push_back(bestAudioUrl);
				}
			}
		}
	}

	return !urls.empty();
}

