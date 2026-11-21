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
#include "PlayerLyrics.h"
#include "rapidjsonHelper.h"
#include "MainFrm.h"
#include "DSUtil/ISOLang.h"
#include "filters/parser/MpaSplitter/MpaSplitter.h"
#include <atlfile.h>
#include <cmath>

static bool ReadLyricTime(const rapidjson::Value& object, const char* name, REFERENCE_TIME& time)
{
	const auto it = object.FindMember(name);
	if (it == object.MemberEnd() || !it->value.IsNumber()) {
		return false;
	}

	const double seconds = it->value.GetDouble();
	const double units = seconds * 10000000.0;
	if (!std::isfinite(units) || units < 0 || units >= 0x1p63) {
		return false;
	}
	time = std::llround(units);
	return true;
}

static bool ReadLyricText(const rapidjson::Value& object, const char* name, CString& text)
{
	const auto it = object.FindMember(name);
	if (it == object.MemberEnd() || !it->value.IsString() || !it->value.GetStringLength()) {
		return false;
	}
	const CStringA utf8(it->value.GetString(), it->value.GetStringLength());
	if (utf8.GetLength() != strlen(utf8.GetString())) {
		return false;
	}
	text = UTF8ToWStr(utf8);
	return !text.IsEmpty();
}

bool ParseLyricJson(const CStringA& json, std::vector<LyricLine>& lines)
{
	if (json.GetLength() != strlen(json.GetString())) {
		return false;
	}
	rapidjson::Document document;
	document.Parse<rapidjson::kParseValidateEncodingFlag>(json.GetString(), json.GetLength());
	if (document.HasParseError() || !document.IsArray() || document.Empty()) {
		return false;
	}

	std::vector<LyricLine> parsed;
	for (const auto& segment : document.GetArray()) {
		LyricLine line;
		if (!segment.IsObject()
				|| !ReadLyricTime(segment, "start", line.start)
				|| !ReadLyricTime(segment, "end", line.end)
				|| line.end <= line.start
				|| (!parsed.empty() && line.start < parsed.back().start)
				|| !ReadLyricText(segment, "text", line.text)) {
			return false;
		}

		const auto words = segment.FindMember("words");
		if (words == segment.MemberEnd() || !words->value.IsArray()) {
			return false;
		}

		int offset = 0;
		for (const auto& value : words->value.GetArray()) {
			LyricWord word;
			CString text;
			if (!value.IsObject()
					|| !ReadLyricTime(value, "start", word.start)
					|| !ReadLyricTime(value, "end", word.end)
					|| word.end < word.start
					|| (!line.words.empty() && word.start < line.words.back().start)
					|| !ReadLyricText(value, "word", text)) {
				return false;
			}

			text.Trim();
			if (text.IsEmpty()) {
				continue;
			}
			word.textStart = line.text.Find(text, offset);
			if (word.textStart < 0) {
				return false;
			}
			word.textLength = text.GetLength();
			offset = word.textStart + word.textLength;
			line.words.emplace_back(word);
		}

		parsed.emplace_back(std::move(line));
	}

	lines = std::move(parsed);
	return true;
}

static CString GetLyricLanguage(const CString& code)
{
	const CStringA language(code);
	const CString name = language.GetLength() == 3 ? ISO6392ToLanguage(language)
		: language.GetLength() == 2 ? ISO6391ToLanguage(language) : CString();
	return name.IsEmpty() ? code : name;
}

static bool ReadLyricJsonFile(const CString& path, std::vector<LyricLine>& lines)
{
	CAtlFile file;
	ULONGLONG size = 0;
	if (FAILED(file.Create(path, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING))
			|| FAILED(file.GetSize(size)) || !size || size > 16 * 1024 * 1024) {
		return false;
	}

	CStringA json;
	DWORD read = 0;
	const HRESULT hr = file.Read(json.GetBufferSetLength(static_cast<int>(size)), static_cast<DWORD>(size), read);
	json.ReleaseBufferSetLength(read);
	if (FAILED(hr) || read != size) {
		return false;
	}
	if (json.Left(3) == "\xef\xbb\xbf") {
		json.Delete(0, 3);
	}
	return ParseLyricJson(json, lines);
}

static bool ReadLyricLrcFile(const CString& path, std::vector<LyricLine>& lines)
{
	CSimpleTextSubtitle subtitles;
	if (!subtitles.Open(path, DEFAULT_CHARSET, true, {}, {}) || subtitles.m_subtitleType != Subtitle::LRC) {
		return false;
	}
	subtitles.Sort();
	for (size_t i = 0; i < subtitles.GetCount(); i++) {
		const int index = static_cast<int>(i);
		lines.push_back({
			MILLISECONDS_TO_100NS_UNITS(REFERENCE_TIME(subtitles.TranslateStart(index, 0))),
			MILLISECONDS_TO_100NS_UNITS(REFERENCE_TIME(subtitles.TranslateEnd(index, 0))),
			subtitles.GetStrW(index)
		});
	}
	return !lines.empty();
}

static void JoinLyricFragments(std::vector<LyricLine>& lines)
{
	if (std::none_of(lines.begin(), lines.end(), [](const LyricLine& line) {
			return line.text.FindOneOf(L"\r\n") >= 0;
		})) {
		return;
	}

	CString text;
	for (auto& line : lines) {
		line.text.Replace(L"\r\n", L"\n");
		line.text.Replace(L'\r', L'\n');
		const int lastBreak = line.text.ReverseFind(L'\n');
		if (lastBreak < 0) {
			text += line.text;
			line.text = text;
		} else {
			text += line.text.Left(lastBreak);
			CString remainder = line.text.Mid(lastBreak + 1);
			line.text = remainder.IsEmpty() ? text : remainder;
			text = std::move(remainder);
		}
	}
}

void CMainFrame::SetupLyrics()
{
	m_lyricTracks.clear();
	m_lyricTrack = -1;
	const CString path = GetCurFileName();
	if (!m_bAudioOnly || GetPlaybackMode() != PM_FILE || PathIsURLW(path)
			|| path.Right(4).CompareNoCase(L".mp3")) {
		return;
	}

	REFERENCE_TIME duration = 0;
	if (m_pMS) {
		m_pMS->GetDuration(&duration);
	}

	const auto& settings = AfxGetAppSettings();
	BeginEnumFilters(m_pGB, pEF, pBF) {
		if (settings.fDisableInternalSubtitles) {
			break;
		}
		CComQIPtr<IMpaSyncLyrics> lyrics(pBF);
		if (!lyrics) {
			continue;
		}
		for (UINT i = 0, count = lyrics->GetLyricTrackCount(); i < count; i++) {
			CComBSTR language, descriptor;
			UINT n = 0;
			if (FAILED(lyrics->GetLyricTrack(i, &language, &descriptor, &n))) {
				continue;
			}

			LyricTrack track;
			const CString description(descriptor.m_str);
			track.language = language.m_str;
			const CString name = description.IsEmpty() ? GetLyricLanguage(track.language) : description;
			for (UINT j = 0; j < n; j++) {
				CComBSTR text;
				REFERENCE_TIME start;
				if (FAILED(lyrics->GetLyricEntry(i, j, &start, &text))) {
					track.lines.clear();
					break;
				}
				if (!track.lines.empty() && start == track.lines.back().start) {
					track.lines.back().text += text.m_str;
				} else {
					if (!track.lines.empty()) {
						track.lines.back().end = start;
					}
					track.lines.push_back({start, duration > start ? duration : MAXLONGLONG, CString(text.m_str)});
				}
			}
			if (!track.lines.empty()) {
				track.name = name + L" (SYLT)";
				if (track.lines.size() == 1 && track.lines[0].start == 0
						&& ParseLyricJson(WStrToUTF8(track.lines[0].text), track.lines)) {
					track.name = name + L" (SYLT JSON)";
				} else {
					JoinLyricFragments(track.lines);
				}
				m_lyricTracks.emplace_back(std::move(track));
			}
		}
		break;
	} EndEnumFilters;

	const CString base = path.Left(path.GetLength() - 4);
	const int first = std::max(base.ReverseFind(L'\\'), base.ReverseFind(L'/')) + 1;
	const CString folder = base.Left(first);
	const int prefix = base.GetLength() - first;
	std::vector<CString> files;
	CFileFind finder;
	BOOL found = finder.FindFile(base + L".*");
	while (found) {
		found = finder.FindNextFile();
		if (!finder.IsDirectory()) {
			const CString filename = finder.GetFileName();
			if (filename.Right(4).CompareNoCase(L".lrc") == 0
					|| filename.Right(5).CompareNoCase(L".json") == 0) {
				files.emplace_back(filename);
			}
		}
	}
	std::sort(files.begin(), files.end(), [](const CString& a, const CString& b) { return a.CompareNoCase(b) < 0; });
	for (const auto& file : files) {
		LyricTrack track;
		track.external = true;
		const bool json = file.Right(5).CompareNoCase(L".json") == 0;
		const int ext = file.ReverseFind(L'.');
		if (ext == prefix + 1) {
			continue;
		} else if (ext <= prefix) {
			if (json || ext != prefix) {
				continue;
			}
			track.name = file.Left(ext);
		} else {
			track.language = file.Mid(prefix + 1, ext - prefix - 1);
			track.name = GetLyricLanguage(track.language);
		}
		if (json ? ReadLyricJsonFile(folder + file, track.lines) : ReadLyricLrcFile(folder + file, track.lines)) {
			track.name += json ? L" (JSON)" : L" (LRC)";
			m_lyricTracks.emplace_back(std::move(track));
		}
	}

	const size_t count = std::min(m_lyricTracks.size(), size_t(ID_LYRICS_TRACK_LAST - ID_LYRICS_TRACK_FIRST + 1));
	std::vector<Stream> streams;
	streams.reserve(count);
	for (size_t i = 0; i < count; i++) {
		const auto& track = m_lyricTracks[i];
		Stream stream;
		stream.Ext = track.external;
		stream.Name = track.name + L" " + track.language + L" " + GetLyricLanguage(track.language);
		streams.emplace_back(std::move(stream));
	}

	size_t selected = 0;
	if (settings.fUseInternalSelectTrackLogic) {
		selected = GetSubSelIdx(streams);
	} else if (settings.fPrioritizeExternalSubtitles) {
		const auto external = std::find_if(streams.begin(), streams.end(), [](const Stream& stream) { return stream.Ext; });
		if (external != streams.end()) {
			selected = std::distance(streams.begin(), external);
		}
	}
	if (selected < streams.size()) {
		m_lyricTrack = static_cast<int>(selected);
	}
}

void CMainFrame::SetupLyricsMenu(CMenu& menu)
{
	if (m_lyricTracks.empty()) {
		return;
	}
	if (menu.GetMenuState(ID_SUBTITLES_ENABLE, MF_BYCOMMAND) == UINT(-1)) {
		menu.AppendMenuW(MF_STRING, ID_SUBTITLES_ENABLE, ResStr(IDS_SUBTITLES_ENABLE));
		menu.AppendMenuW(MF_SEPARATOR);
	} else {
		menu.EnableMenuItem(ID_SUBTITLES_ENABLE, MF_BYCOMMAND | MF_ENABLED);
	}
	CMenu lyrics;
	lyrics.CreatePopupMenu();
	lyrics.AppendMenuW(MF_STRING, ID_LYRICS_OFF, ResStr(IDS_LYRICS_OFF));
	const UINT count = std::min(static_cast<UINT>(m_lyricTracks.size()), UINT(ID_LYRICS_TRACK_LAST - ID_LYRICS_TRACK_FIRST + 1));
	for (UINT i = 0; i < count; i++) {
		CString label = m_lyricTracks[i].name;
		label.Replace(L"&", L"&&");
		lyrics.AppendMenuW(MF_STRING, ID_LYRICS_TRACK_FIRST + i, label);
	}
	const bool enabled = AfxGetAppSettings().fEnableSubtitles && m_lyricTrack >= 0;
	lyrics.CheckMenuRadioItem(ID_LYRICS_OFF, ID_LYRICS_TRACK_FIRST + count - 1,
		enabled ? ID_LYRICS_TRACK_FIRST + m_lyricTrack : ID_LYRICS_OFF, MF_BYCOMMAND);
	menu.AppendMenuW(MF_POPUP, reinterpret_cast<UINT_PTR>(lyrics.Detach()), ResStr(IDS_LYRICS));
}

void CMainFrame::OnSelectLyrics(UINT id)
{
	if (m_eMediaLoadState != MLS_LOADED) {
		return;
	}
	if (id == ID_LYRICS_OFF) {
		m_lyricTrack = -1;
		m_wndView.SetLyrics({}, 0, 0);
	} else if (id - ID_LYRICS_TRACK_FIRST < m_lyricTracks.size()) {
		m_lyricTrack = id - ID_LYRICS_TRACK_FIRST;
		OnTimer(TIMER_STREAMPOSPOLLER);
	}
}

void CMainFrame::UpdateLyrics(REFERENCE_TIME position)
{
	if (m_lyricTrack < 0 || position < 0 || m_bEndOfStream || !AfxGetAppSettings().fEnableSubtitles) {
		m_wndView.SetLyrics({}, 0, 0);
		return;
	}

	const auto state = GetMediaState();
	if (state == State_Running || state == State_Paused) {
		const auto& lines = m_lyricTracks[m_lyricTrack].lines;
		const auto next = std::upper_bound(lines.begin(), lines.end(), position,
			[](REFERENCE_TIME time, const LyricLine& line) { return time < line.start; });
		if (next != lines.begin()) {
			const auto& line = *std::prev(next);
			if (position < line.end) {
				const auto word = std::find_if(line.words.begin(), line.words.end(), [position](const LyricWord& word) {
					return position >= word.start && position < word.end;
				});
				m_wndView.SetLyrics(line.text, word == line.words.end() ? 0 : word->textStart,
					word == line.words.end() ? 0 : word->textLength);
				return;
			}
		}
	}
	m_wndView.SetLyrics({}, 0, 0);
}
