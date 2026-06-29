/*
 * (C) 2026 see Authors.txt
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
#include "PlayerYtDlp.h"
#include "DSUtil/HTTPAsync.h"

#define bufsize (2ul * KILOBYTE)

static bool RunYtDlp(LPCWSTR ydl_path, LPCWSTR yt_args, LPCWSTR url, CStringA& buf_out)
{
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
	args.Format(LR"("%s" %s --no-check-certificates "%s")", ydl_path, yt_args, url);

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

	CloseHandle(proc_info.hProcess);
	CloseHandle(proc_info.hThread);
	CloseHandle(hStdout_r);
	CloseHandle(hStderr_r);

	if (exitcode) {
		if (buf_err.Find("Unsupported URL:") == -1) {
			const CStringW ydl_fname = GetFileName(ydl_path);
			MessageBoxW(AfxGetApp()->GetMainWnd()->m_hWnd, CStringW(buf_err), ydl_fname, MB_ICONERROR | MB_OK);
		}
		return false;
	}

	return buf_out.GetLength() > 0;
}

static int vformat_cmp(const YT_DLP::yt_vformat_t& a, const YT_DLP::yt_vformat_t& b)
{
	if (a.protocol < b.protocol) {
		return -1;
	}
	if (a.protocol > b.protocol) {
		return +1;
	}

	if (a.codec < b.codec) {
		return -1;
	}
	if (a.codec > b.codec) {
		return +1;
	}

	if (a.height > b.height) {
		return -1;
	}
	if (a.height < b.height) {
		return +1;
	}

	if (a.fps > b.fps) {
		return -1;
	}
	if (a.fps < b.fps) {
		return +1;
	}

	if (a.hdr > b.hdr) {
		return -1;
	}
	if (a.hdr < b.hdr) {
		return +1;
	}

	if (a.bitrate > b.bitrate) {
		return -1;
	}
	if (a.bitrate < b.bitrate) {
		return +1;
	}

	return 0;
}

static int aformat_cmp(const YT_DLP::yt_aformat_t& a, const YT_DLP::yt_aformat_t& b)
{
	if (a.protocol < b.protocol) {
		return -1;
	}
	if (a.protocol > b.protocol) {
		return +1;
	}

	if (a.codec > b.codec) {
		return -1;
	}
	if (a.codec < b.codec) {
		return +1;
	}

	if (a.bitrate > b.bitrate) {
		return -1;
	}
	if (a.bitrate < b.bitrate) {
		return +1;
	}

	return 0;
}

static bool IsVideoFormat(const rapidjson::Value& format)
{
	CStringA value_str;
	if (getJsonValue(format, "vcodec", value_str) && value_str != "none") {
		return true;
	}
	if (getJsonValue(format, "video_ext", value_str) && value_str != "none") {
		return true;
	}
	return false;
}

static bool FormatHasAudio(const rapidjson::Value& format)
{
	CStringA value_str;
	if (getJsonValue(format, "acodec", value_str) && value_str != "none") {
		return true;
	}
	if (getJsonValue(format, "audio_ext", value_str) && value_str != "none") {
		return true;
	}
	return false;
}

static YT_DLP::yt_acodec_type GetAudioCodec(const rapidjson::Value& format)
{
	YT_DLP::yt_acodec_type acodec = YT_DLP::acodec_none;

	CStringA value_str;

	if (getJsonValue(format, "acodec", value_str) && value_str != "none") {
		if (StartsWith(value_str, "mp4a")) {
			acodec = YT_DLP::acodec_aac;
		}
		else if (StartsWith(value_str, "opus")) {
			acodec = YT_DLP::acodec_opus;
		}
		else {
			acodec = YT_DLP::acodec_unknoun;
		}
	}
	else if (getJsonValue(format, "audio_ext", value_str) && value_str != "none") {
		acodec = YT_DLP::acodec_unknoun;
	}

	return acodec;
}

static void SetVFormatDesc(YT_DLP::yt_vformat_t& v)
{
	auto& desc = v.desc;

	switch (v.protocol) {
	case YT_DLP::protocol_dash:
		desc = L"DASH ";
		break;
	case YT_DLP::protocol_hls:
		desc = L"HLS ";
		break;
	default:
		desc.Empty();
	}

	switch (v.codec) {
	case YT_DLP::vcodec_h264:
		desc.Append(L"H.264 ");
		break;
	case YT_DLP::vcodec_vp9:
		desc.Append(L"VP9 ");
		break;
	case YT_DLP::vcodec_av1:
		desc.Append(L"AV1 ");
		break;
	case YT_DLP::vcodec_unknoun:
		desc.Append(L"Video ");
		break;
	}

	if (desc.IsEmpty()) {
		desc = v.id;
		desc.AppendChar(' ');
	}

	if (v.height > 0) {
		desc.AppendFormat(L"%dp ", v.height);
	}

	if (v.fps >= 48) {
		desc.AppendFormat(L"%dfps ", v.fps);
	}

	if (v.hdr) {
		desc.Append(L"HDR ");
	}

	switch (v.audio) {
	case YT_DLP::acodec_aac:
		desc.Append(L"AAC ");
		break;
	case YT_DLP::acodec_opus:
		desc.Append(L"OPUS ");
		break;
	case YT_DLP::acodec_unknoun:
		desc.Append(L"Audio ");
		break;
	}

	//if (v.bitrate > 0) {
	//	desc.AppendFormat(L"%.0f kbit/s ", v.bitrate);
	//}

	if (v.audio != YT_DLP::acodec_none && v.language.GetLength()) {
		desc.Append(CStringW(v.language));
	}

	desc.TrimRight(' ');
}

static void SetAFormatDesc(YT_DLP::yt_aformat_t& a)
{
	auto& desc = a.desc;

	switch (a.protocol) {
	case YT_DLP::protocol_dash:
		desc = L"DASH ";
		break;
	case YT_DLP::protocol_hls:
		desc = L"HLS ";
		break;
	default:
		desc.Empty();
	}

	switch (a.codec) {
	case YT_DLP::acodec_aac:
		desc.Append(L"AAC ");
		break;
	case YT_DLP::acodec_opus:
		desc.Append(L"OPUS ");
		break;
	case YT_DLP::acodec_unknoun:
		desc.Append(L"Audio ");
		break;
	}

	if (desc.IsEmpty()) {
		desc = a.id;
		desc.AppendChar(' ');
	}

	if (a.bitrate > 0) {
		desc.AppendFormat(L"%.0f kbit/s ", a.bitrate);
	}

	if (a.language.GetLength()) {
		desc.Append(CStringW(a.language));
	}

	desc.TrimRight(' ');
}

//
// YT_DLP
//

LPCWSTR YT_DLP::CheckVideoURL(CStringW url)
{
	url.MakeLower();

	LPCWSTR s = url.GetString();
	StartsWithStep(s, L"https://");
	StartsWithStep(s, L"www.");

	if (StartsWith(s, L"youtu.be/")) {
		return L"YouTube";
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
			return L"YouTube";
		}
		return nullptr;
	}

	if (StartsWith(s, L"vkvideo.ru/video-") || StartsWith(s, L"vk.com/video-")) {
		return L"VK \u0412\u0438\u0434\u0435\u043e";
	}

	if (StartsWith(s, L"rutube.ru/video/")) {
		return L"Rutube";
	}

	if (StartsWith(s, L"dzen.ru/video/watch/")) {
		return L"\u0414\u0437\u0435\u043d";
	}

	return nullptr;
}

bool YT_DLP::CheckNonYtPlaylistURL(CString url)
{
	url.MakeLower();

	LPCWSTR s = url.GetString();
	StartsWithStep(s, L"https://");
	StartsWithStep(s, L"www.");

	if (StartsWith(s, L"vkvideo.ru/playlist/")) {
		return true;
	}
	if (StartsWith(s, L"rutube.ru/plst/")) {
		return true;
	}
	if (StartsWith(s, L"dzen.ru/suite/")) {
		return true;
	}

	return false;
}

bool YT_DLP::Parse_Playlist(const CStringW& pls_url, CFileItemList& playlist, int& idx_CurrentPlay)
{
	// get a copy of the settings

	CAppSettings& s = AfxGetAppSettings();

	const CStringW ydl_path = GetFullExePath(s.strYdlExePath, true);
	if (ydl_path.IsEmpty()) {
		return false;
	}

	// run yt-dlp.exe

	CStringA buf_out;
	bool ok = RunYtDlp(ydl_path, L"--flat-playlist --dump-single-json", pls_url, buf_out);
	if (!ok) {
		return false;
	}

	// processing a JSON file

	rapidjson::Document doc;
	if (doc.Parse(buf_out.GetString(), buf_out.GetLength()).HasParseError()) {
		return false;
	}

	auto entries = GetJsonArray(doc, "entries");
	if (!entries) {
		return false;
	}

	playlist.clear();
	idx_CurrentPlay = -1;

	for (const auto& entry : entries->GetArray()) {
		CStringW url;
		if (!getJsonValue(entry, "url", url)) {
			continue;
		}

		if (idx_CurrentPlay < 0) {
			CStringW id;
			if (getJsonValue(entry, "id", id)) {
				if (pls_url.Find(id) > 0) {
					idx_CurrentPlay = (int)playlist.size();
				}
			}
		}

		CStringW title;
		getJsonValue(entry, "title", title);
		int duration = 0;
		getJsonValue(entry, "duration", duration);

		playlist.emplace_back(url, title, UNITS * duration);
	}

	if (playlist.size()) {
		if (idx_CurrentPlay < 0) {
			idx_CurrentPlay = 0;
		}
		return true;
	}

	return false;
}

bool YT_DLP::SetFormats(const rapidjson::Document& doc)
{
	auto formats = GetJsonArray(doc, "formats");
	if (!formats) {
		return false;
	}

	CStringA value_str;
	int   value_int = 0;
	float value_float = 0.0f;

	bool hls_vcodec_defined = false;
	bool hls_acodec_defined = false;

	std::vector<yt_vformat_t> hlsVideoFormats;
	std::vector<yt_aformat_t> hlsAudioFormats;

	for (const auto& format : formats->GetArray()) {
		// mandatory parameters

		CStringA format_id;
		CStringW url;
		yt_protocol_type protocol = protocol_unknoun;

		if (!getJsonValue(format, "format_id", format_id)) {
			ASSERT(0);
			continue;
		}

		if (getJsonValue(format, "protocol", value_str)) {
			if (value_str == "http_dash_segments") {
				// not supported
				continue;
			}
			else if (StartsWith(value_str, "http")) {
				if (StartsWith(format_id, "dash")) {
					protocol = protocol_dash;
				} else if (getJsonValue(format, "container", value_str) && EndsWith(value_str, "_dash")) {
					protocol = protocol_dash;
				} else {
					protocol = protocol_http;
				}
			}
			else if (StartsWith(value_str, "m3u8")) {
				protocol = protocol_hls;
			}
			else {
				continue;
			}
		}
		else {
			ASSERT(0);
			continue;
		}

		if (!getJsonValue(format, "url", url)) {
			ASSERT(0);
			continue;
		}

		// optional parameters

		if (IsVideoFormat(format)) {
			// Ignore subsequent formats with the same URL (rutube.ru)
			if (protocol == protocol_hls) {
				auto it = std::find_if(hlsVideoFormats.cbegin(), hlsVideoFormats.cend(), [&](const auto& e) { return e.url == url; });
				if (it != hlsVideoFormats.cend()) {
					continue;
				}
			} else {
				auto it = std::find_if(mVideoFormats.cbegin(), mVideoFormats.cend(), [&](const auto& e) { return e.url == url; });
				if (it != mVideoFormats.cend()) {
					continue;
				}
			}

			yt_vformat_t vformat;
			if (!getJsonValue(format, "height", vformat.height)) {
				continue;
			}

			vformat.id = format_id;
			vformat.protocol = protocol;
			vformat.url = url;

			if (getJsonValue(format, "vcodec", value_str)) {
				if (StartsWith(value_str, "avc1") || value_str == "h264") {
					vformat.codec = vcodec_h264;
				}
				else if (StartsWith(value_str, "vp09") || value_str == "vp9") {
					vformat.codec = vcodec_vp9;
				}
				else if (StartsWith(value_str, "av01")) {
					vformat.codec = vcodec_av1;
				}

				if (vformat.codec >= 0 && vformat.protocol == protocol_hls) {
					hls_vcodec_defined  = true;
				}
			}

			if (getJsonValue(format, "fps", value_int) && value_int > 0) {
				vformat.fps = value_int;
			}

			if (getJsonValue(format, "dynamic_range", value_str) && value_str != "SDR") {
				vformat.hdr = true;
			}

			getJsonValue(format, "tbr", vformat.bitrate);

			if (vformat.protocol == protocol_hls) {
				vformat.ext = "m3u8";
			} else {
				getJsonValue(format, "ext", vformat.ext);
			}

			vformat.audio = GetAudioCodec(format);

			getJsonValue(format, "language", vformat.language);

			if (vformat.protocol == protocol_hls) {
				hlsVideoFormats.emplace_back(vformat);
			} else {
				mVideoFormats.emplace_back(vformat);
			}
		}
		else if (FormatHasAudio(format)) {
			if (EndsWith(format_id, "-drc")) {
				continue;
			}

			yt_aformat_t aformat;
			aformat.id = format_id;
			aformat.protocol = protocol;
			aformat.url = url;

			aformat.codec = GetAudioCodec(format);
			if (aformat.codec >= 0 && aformat.protocol == protocol_hls) {
				hls_acodec_defined  = true;
			}

			getJsonValue(format, "tbr", aformat.bitrate);

			getJsonValue(format, "ext", aformat.ext);
			if (aformat.ext == "webm") {
				aformat.ext = "mka";
			}

			getJsonValue(format, "language", aformat.language);

			if (aformat.protocol == protocol_hls) {
				hlsAudioFormats.emplace_back(aformat);
			} else {
				mAudioFormats.emplace_back(aformat);
			}
		}
	}

	const bool useHLS = (mVideoFormats.empty() && mAudioFormats.empty())
		|| (mVideoFormats.empty() && hlsVideoFormats.size());

	if (useHLS) {
		// If some codecs are identified, then remove formats with unknown codecs.
		if (hls_vcodec_defined) {
			hlsVideoFormats.erase(std::remove_if(hlsVideoFormats.begin(), hlsVideoFormats.end(), [](yt_vformat_t v) { return v.codec == vcodec_unknoun; }), hlsVideoFormats.end());
		}
		if (hls_acodec_defined) {
			hlsAudioFormats.erase(std::remove_if(hlsAudioFormats.begin(), hlsAudioFormats.end(), [](yt_aformat_t a) { return a.codec == acodec_unknoun; }), hlsAudioFormats.end());
		}

		mVideoFormats = std::move(hlsVideoFormats);
		mAudioFormats = std::move(hlsAudioFormats);
	}

	std::sort(mVideoFormats.begin(), mVideoFormats.end(), [](const yt_vformat_t& a, const yt_vformat_t& b) {
		return (vformat_cmp(a, b) < 0);
		});
	std::sort(mAudioFormats.begin(), mAudioFormats.end(), [](const yt_aformat_t& a, const yt_aformat_t& b) {
		return (aformat_cmp(a, b) < 0);
		});

	if (GetFormatsCount()) {
		int count = 0;
		yt_protocol_type protocol = protocol_unknoun;
		yt_vcodec_type vcodec = vcodec_none;
		yt_acodec_type acodec = acodec_none;

		for (auto& fmt : mVideoFormats) {
			switch (fmt.protocol) {
			case protocol_http: mHttpVideoCount++; break;
			case protocol_dash: mDashVideoCount++; break;
			}

			if (protocol != fmt.protocol || vcodec != fmt.codec) {
				if (count > 0) {
					fmt.menuflags = MF_SEPARATOR;
				}
				protocol = fmt.protocol;
				vcodec = fmt.codec;
			}
			SetVFormatDesc(fmt);
			count++;
		}

		for (auto& fmt : mAudioFormats) {
			if (protocol != fmt.protocol || acodec != fmt.codec) {
				if (count > 0) {
					fmt.menuflags = MF_SEPARATOR;
				}
				protocol = fmt.protocol;
				acodec = fmt.codec;
			}
			SetAFormatDesc(fmt);
			count++;
		}
	}

	return mVideoFormats.size() || mAudioFormats.size();
}

void YT_DLP::SetSubtitles(const rapidjson::Document& doc)
{
	if (auto requested_subtitles = GetJsonObject(doc, "requested_subtitles")) {
		for (const auto& subtitle : requested_subtitles->GetObj()) {
			yt_sub_t sub;
			sub.language = subtitle.name.GetString();
			if (sub.language.IsEmpty()) {
				continue;
			}
			if (!getJsonValue(subtitle.value, "ext", sub.ext) || sub.ext != "vtt") {
				continue;
			}
			if (!getJsonValue(subtitle.value, "url", sub.url) || sub.url.IsEmpty()) {
				continue;
			}
			getJsonValue(subtitle.value, "name", sub.name);

			mSubtitles.emplace_back(sub);
		}
	}
}

void YT_DLP::SetInfo(const rapidjson::Document& doc)
{
	getJsonValue(doc, "title", mTitle);

	getJsonValue(doc, "uploader", mAuthor);

	getJsonValue(doc, "description", mDescription);
	if (mDescription.Find('\n') && mDescription.Find(L"\r\n") == -1) {
		mDescription.Replace(L"\n", L"\r\n");
	}

	CStringA upload_date;
	if (getJsonValue(doc, "upload_date", upload_date)) {
		WORD y, m, d;
		if (sscanf_s(upload_date.GetString(), "%04hu%02hu%02hu", &y, &m, &d) == 3) {
			mTime.wYear = y;
			mTime.wMonth = m;
			mTime.wDay = d;
		}
	}

	// chapters
	if (auto _chapters = GetJsonArray(doc, "chapters")) {
		for (const auto& chapter : _chapters->GetArray()) {
			float start_time = 0.0f;
			CStringW title;
			if (getJsonValue(chapter, "title", title) && getJsonValue(chapter, "start_time", start_time)) {
				mChapters.emplace_back(title, REFERENCE_TIME(start_time * UNITS));
			}
		}
	}

	// thumbnails
	if (auto thumbnails = GetJsonArray(doc, "thumbnails")) {
		CStringA thumbnailUrl;
		int maxHeight = 0;

		for (const auto& elem : thumbnails->GetArray()) {
			int height = 0;
			if ((getJsonValue(elem, "height", height) && height > maxHeight) || thumbnailUrl.IsEmpty()) {
				if (getJsonValue(elem, "url", thumbnailUrl)) {
					maxHeight = height;
				}
			}
		}

		if (thumbnailUrl.GetLength()) {
			mThumbnail.url = thumbnailUrl;
		}
	}

	// user-agent
	if (auto http_headers = GetJsonObject(doc, "http_headers")) {
		getJsonValue(*http_headers, "User-Agent", mUserAgent);
		DLogIf(mUserAgent.GetLength(), L"User-Agent: %s", mUserAgent.GetString());
	}

	CStringA liveStatus;
	getJsonValue(doc, "live_status", liveStatus);
	mIsLive = (liveStatus == "is_live");
}

UINT YT_DLP::GetVideo(
	const yt_protocol_type protocol,
	const yt_vcodec_type vcodec,
	const int maxHeight,
	const bool bHighFps,
	const bool bHDR,
	const bool bHighBitrate)
{
	if (mVideoFormats.empty()) {
		return UINT_MAX;
	}

	// mVideoFormats must be sorted

	// select protocol
	UINT start = 0;
	UINT end = 0;

	switch (protocol) {
	case protocol_http:
		end = mHttpVideoCount;
		break;
	case protocol_dash:
		if (mDashVideoCount) {
			start = mHttpVideoCount;
			end = mVideoFormats.size();
		}
		break;
	case protocol_hls:
		if (mHttpVideoCount + mDashVideoCount == 0) {
			end = mVideoFormats.size();
		}
		break;
	}

	if (end <= start) {
		return UINT_MAX;
	}

	// select codec
	for (UINT i = start + 1; i < end; i++) {
		auto& prev = mVideoFormats[i - 1];
		auto& fmt = mVideoFormats[i];

		if (prev.codec <= vcodec && fmt.codec > vcodec) {
			end = i;
			break;
		}
		if (prev.codec != fmt.codec) {
			start = i;
		}
	}

	// select height, HDR, fps, and bitrate
	UINT idx = start;

	for (UINT i = start + 1; i < end; i++) {
		auto& sel = mVideoFormats[idx];
		auto& fmt = mVideoFormats[i];

		if (sel.height <= maxHeight && fmt.height < sel.height) {
			break;
		}

		if (fmt.height == sel.height) {
			if (bHighFps && sel.fps > fmt.fps) {
				break;
			}
			if (bHDR && sel.hdr > sel.hdr) {
				break;
			}
			if (bHighBitrate && sel.bitrate > fmt.bitrate) {
				break;
			}
		}

		idx = i;
	}

	return idx;
}

UINT YT_DLP::GetAudio(const yt_protocol_type protocol, yt_acodec_type acodec, const bool bHighBitrate)
{
	if (mAudioFormats.empty()) {
		return UINT_MAX;
	}

	// mAudioFormats must be sorted

	auto it = std::find_if(mAudioFormats.cbegin(), mAudioFormats.cend(), [&](const auto& e) { return e.protocol == protocol; });
	if (it == mAudioFormats.cend()) {
		// Do not select audio with a protocol different from video, because it does not work.
		return UINT_MAX;
	}

	auto it_acodec = std::find_if(it, mAudioFormats.cend(), [&](const auto& e) { return e.protocol == protocol && e.codec == acodec; });
	if (it_acodec != mAudioFormats.cend()) {
		it = it_acodec;
	} else {
		acodec = it->codec;
	}

	if (!bHighBitrate) {
		// choose medium bitrate
		auto it_end = std::find_if(it, mAudioFormats.cend(), [&](const auto& e) { return e.protocol == protocol && e.codec == acodec; });
		it += std::distance(it, it_end) / 2;
	}

	return (UINT)std::distance(mAudioFormats.cbegin(), it);
}

bool YT_DLP::SelectStreams()
{
	if (GetFormatsCount() == 0) {
		return false;
	}

	mIdxVideo = UINT_MAX;
	mIdxAudio = UINT_MAX;

	// get a copy of the settings

	CAppSettings& s = AfxGetAppSettings();

	const yt_vcodec_type vcodec       = (yt_vcodec_type)s.iYdlVcodec;
	const yt_acodec_type acodec       = (yt_acodec_type)s.iYdlAcodec;
	const int            maxHeight    = s.iYdlMaxHeight;
	const bool           bHighFps     = s.bYdlHighFps;
	const bool           bHDR         = s.bYdlHDR;
	const bool           bHighBitrate = s.bYdlHighBitrate;

	// select video
	if (mVideoFormats.size() && maxHeight > 0) {

		UINT idx_http = GetVideo(protocol_http, vcodec, maxHeight, bHighFps, bHDR, bHighBitrate);
		UINT idx_dash = GetVideo(protocol_dash, vcodec, maxHeight, bHighFps, bHDR, bHighBitrate);

		if (idx_http < mVideoFormats.size() && idx_dash < mVideoFormats.size()) {
			auto vfmt_http = mVideoFormats[idx_http];
			auto vfmt_dash = mVideoFormats[idx_dash];

			if (vfmt_http.codec == vcodec && vfmt_dash.codec != vcodec) {
				mIdxVideo = idx_http;
			}
			else if (vfmt_dash.codec == vcodec && vfmt_http.codec != vcodec) {
				mIdxVideo = idx_dash;
			}
			else {
				if (vfmt_http.height > vfmt_dash.height) {
					mIdxVideo = (vfmt_http.height <= maxHeight) ? idx_http : idx_dash;
				}
				else if (vfmt_dash.height > vfmt_http.height) {
					mIdxVideo = (vfmt_dash.height <= maxHeight) ? idx_dash : idx_http;
				}
				else {
					if (vfmt_http.fps > vfmt_dash.fps) {
						mIdxVideo = bHighFps ? idx_http : idx_dash;
					}
					else if (vfmt_dash.fps > vfmt_http.fps) {
						mIdxVideo = bHighFps ? idx_dash : idx_http;
					}
					else {
						if (vfmt_http.hdr > vfmt_dash.hdr) {
							mIdxVideo = bHDR ? idx_http : idx_dash;
						}
						else if (vfmt_dash.hdr > vfmt_http.hdr) {
							mIdxVideo = bHDR ? idx_dash : idx_http;
						}
						else {
							// If other parameters are the same, we prefer "http."
							mIdxVideo = idx_http;
							// PS: We're not comparing the bitrate, because "http"
							// may be slightly larger due to the included audio track.
							// Also, remember that bHighBitrate worked earlier.
						}
					}
				}
			}
		}
		else if (idx_http < mVideoFormats.size()) {
			mIdxVideo = idx_http;
		}
		else if (idx_dash < mVideoFormats.size()) {
			mIdxVideo = idx_dash;
		}
		else {
			mIdxVideo = GetVideo(protocol_hls, vcodec, maxHeight, bHighFps, bHDR, bHighBitrate);
		}

		DLogIf(mIdxVideo < mVideoFormats.size(), L"YT_DLP: selected video - '%hs' - %s", mVideoFormats[mIdxVideo].id, mVideoFormats[mIdxVideo].desc);
	}

	if (mAudioFormats.size()) {
		if (mIdxVideo == UINT_MAX) {
			mIdxAudio = GetAudio(mAudioFormats[0].protocol, acodec, bHighBitrate);
		}
		else if (mVideoFormats[mIdxVideo].audio == acodec_none) {
			mIdxAudio = GetAudio(mVideoFormats[mIdxVideo].protocol, acodec, bHighBitrate);
		}
		DLogIf(mIdxAudio < mAudioFormats.size(), L"YT_DLP: selected audio - '%hs' - %s", mAudioFormats[mIdxAudio].id, mAudioFormats[mIdxAudio].desc);
	}

	return mIdxVideo < mVideoFormats.size() || mIdxAudio < mAudioFormats.size();
}

///////////////////////////////////////////////////////////

void YT_DLP::Clear()
{
	mAuthor.Empty();
	mTitle.Empty();
	mDescription.Empty();
	mTime = {};
	mDuration = 0;
	mIsLive = false;
	mUserAgent.Empty();

	mThumbnail.Clear();
	mChapters.clear();
	mSubtitles.clear();

	mVideoFormats.clear();
	mAudioFormats.clear();
	mIdxVideo = UINT_MAX;
	mIdxAudio = UINT_MAX;

	mHttpVideoCount = 0;
	mDashVideoCount = 0;
}

bool YT_DLP::Parse_URL(const CStringW& url)
{
	Clear();

	// get a copy of the settings

	CAppSettings& s = AfxGetAppSettings();

	const CStringW ydl_path = GetFullExePath(s.strYdlExePath, true);
	if (ydl_path.IsEmpty()) {
		return false;
	}

	// run yt-dlp.exe

	CStringA buf_out;
	bool ok = RunYtDlp(ydl_path, L"-j -I1 --all-subs --sub-format vtt", url, buf_out);
	if (!ok) {
		return false;
	}

	// processing a JSON file

	rapidjson::Document doc;
	const int k = buf_out.Find("\n{\"", 64); // check presence of second JSON root element and ignore it
	if (doc.Parse(buf_out.GetString(), k > 0 ? k : buf_out.GetLength()).HasParseError()) {
		return false;
	}

	ok = SetFormats(doc);
	if (!ok) {
		return false;
	}

#ifdef _DEBUG
	for (const auto& v : mVideoFormats) {
		DLog(v.desc);
	}
	for (const auto& a : mAudioFormats) {
		DLog(a.desc);
	}
#endif

	SetSubtitles(doc);

	SetInfo(doc);

	ok = SelectStreams();

	return ok;
}

bool YT_DLP::ChangeFormats(UINT index)
{
	if (index < mVideoFormats.size()) {
		mIdxVideo = index;
		if (mVideoFormats[mIdxVideo].audio == acodec_none) {
			CAppSettings& s = AfxGetAppSettings();
			const yt_acodec_type acodec = (yt_acodec_type)s.iYdlAcodec;
			const bool bHighBitrate = s.bYdlHighBitrate;
			mIdxAudio = GetAudio(mVideoFormats[mIdxVideo].protocol, acodec, bHighBitrate);
		} else {
			mIdxAudio = UINT_MAX;
		}
		return true;
	}
	else if (index < mVideoFormats.size() + mAudioFormats.size()) {
		mIdxVideo = UINT_MAX;
		mIdxAudio = index - mVideoFormats.size();
		return true;
	}

	return false;
}

bool YT_DLP::FillOFD(OpenFileData& ofd) const
{
	ofd.fi.Clear();
	ofd.auds.clear();
	ofd.subs.clear();

	if (mIdxVideo < mVideoFormats.size()) {
		ofd.fi = mVideoFormats[mIdxVideo].url;
		ofd.title = mTitle;

		for (const auto& sub : mSubtitles) {
			ofd.subs.emplace_back(sub.url, sub.name, sub.language);
		}
	}
	if (mIdxAudio < mAudioFormats.size()) {
		const auto& audio = mAudioFormats[mIdxAudio];
		if (ofd.fi.Valid()) {
			ofd.auds.emplace_back(audio.url, (CStringW)audio.id, audio.language);
		}
		else {
			// audio only
			ofd.fi = audio.url;
			ofd.title = mTitle;
		}
	}

	return ofd.fi.Valid();
}

bool YT_DLP::DownloadThumbnail()
{
	if (mThumbnail.url.IsEmpty()) {
		return false;
	}

	if (mThumbnail.data.Size() && mThumbnail.ext.GetLength()) {
		return true;
	}

	CHTTPAsync HTTPAsync;
	if (SUCCEEDED(HTTPAsync.Connect(mThumbnail.url, http::connectTimeout))) {
		const auto contentLength = HTTPAsync.GetLenght();
		if (contentLength) {
			mThumbnail.data.SetSize(contentLength);
			DWORD dwSizeRead = 0;
			if (S_OK != HTTPAsync.Read((PBYTE)mThumbnail.data.Data(), contentLength, dwSizeRead, http::readTimeout) || dwSizeRead != contentLength) {
				mThumbnail.data.SetSize(0);
			}
		}
		else {
			ASSERT(0);
			/*
			std::vector<char> tmp(16 * KILOBYTE);
			for (;;) {
				DWORD dwSizeRead = 0;
				if (S_OK != HTTPAsync.Read((PBYTE)tmp.data(), tmp.size(), dwSizeRead, http::readTimeout)) {
					break;
				}

				mThumbnailData.insert(m_youtubeThumbnailData.end(), tmp.begin(), tmp.begin() + dwSizeRead);
			}
			*/
		}
	}

	if (mThumbnail.data.Bytes() > 16) {
		if (mThumbnail.data[0] == 0xFF && mThumbnail.data[1] == 0xD8 && mThumbnail.data[2] == 0xFF) {
			mThumbnail.ext = "jpg";
			return true;
		}

		uint32_t* d = (uint32_t*)mThumbnail.data.Data();
		if (d[0] == FCC('RIFF') && d[2] == FCC('WEBP') && (d[3] & 0xFFFFFF00) == FCC('VP8\0')) {
			mThumbnail.ext = "webp";
			return true;
		}
	}

	// If the download fails, we perform a complete cleanup,
	// including the URL, so that it doesn't download again.
	mThumbnail.Clear();

	return false;
}

UINT YT_DLP::GetFormatsCount() const
{
	return mVideoFormats.size() + mAudioFormats.size();
}

LPCWSTR YT_DLP::GetFormatDesc(UINT index, UINT& menuflags) const
{
	if (index < mVideoFormats.size()) {
		menuflags = mVideoFormats[index].menuflags;
		return mVideoFormats[index].desc;
	}
	else if (index < mVideoFormats.size() + mAudioFormats.size()) {
		index -= mVideoFormats.size();
		menuflags = mAudioFormats[index].menuflags;
		return mAudioFormats[index].desc;
	}
	return nullptr;
}

LPCWSTR YT_DLP::GetMainStreamUrl(UINT index) const
{
	if (index < mVideoFormats.size()) {
		return mVideoFormats[index].url;
	}
	else if (index < mVideoFormats.size() + mAudioFormats.size()) {
		index -= mVideoFormats.size();
		return mAudioFormats[index].url;
	}
	return nullptr;
}

CStringW YT_DLP::GetFilename() const
{
	CStringW filename;

	if (mTitle.GetLength()) {
		filename = mTitle;
		FixFilename(filename);
	} else {
		filename = L"file";
	}

	if (mIdxVideo < mVideoFormats.size()) {
		const auto& vfmt = mVideoFormats[mIdxVideo];
		filename.AppendFormat(L".%dp.%hs", vfmt.height, vfmt.ext);
	}
	else if (mIdxAudio < mAudioFormats.size()) {
		const auto& afmt = mAudioFormats[mIdxAudio];
		filename.AppendFormat(L".audio.%hs", afmt.ext);
	}

	return filename;
}

UINT YT_DLP::GetFilesCount() const
{
	UINT count = 0;

	if (mIdxVideo < mVideoFormats.size()) {
		count++;
	}
	if (mIdxAudio < mAudioFormats.size()) {
		count++;
		if (count == 1) { // audio only
			if (mThumbnail.data.Bytes()) {
				count++;
			}
		}
	}
	count += mSubtitles.size();

	return count;
}

const YT_DLP::yt_vformat_t* YT_DLP::GetVFormat() const
{
	if (mIdxVideo < mVideoFormats.size()) {
		return &mVideoFormats[mIdxVideo];
	}
	return nullptr;
}

const YT_DLP::yt_aformat_t* YT_DLP::GetAFormat() const
{
	if (mIdxAudio < mAudioFormats.size()) {
		return &mAudioFormats[mIdxAudio];
	}
	return nullptr;
}

const std::vector<YT_DLP::yt_sub_t>& YT_DLP::GetSubtitles() const
{
	return mSubtitles;
}

const YT_DLP::yt_thumb_t* YT_DLP::GetThumbnail() const
{
	if (mThumbnail.data.Bytes()) {
		return &mThumbnail;
	}
	return nullptr;
}
