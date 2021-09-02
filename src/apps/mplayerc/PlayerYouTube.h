/*
 * (C) 2012-2021 see Authors.txt
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

#pragma once

#include "DSUtil/CUE.h"

namespace Youtube
{
	enum yformat {
		y_mp4 = 0,
		y_webm,
		y_mp4_av1,
		// flv, 3gp are not used.
	};

	enum ytype {
		y_media,
		y_video,
		y_audio,
	};

	struct YoutubeProfile {
		int     iTag;
		yformat format;
		ytype   type;
		int     quality;
		CString ext;
		bool    fps60;
		bool    hdr;
		bool    live;
	};

	// MP4, WebM and 360p resolution or above only
	static const YoutubeProfile YProfiles[] = {
		//iTag| format |   type |quality| ext |  fps60 | hdr |live
		// MP4 (H.264)
		{266, y_mp4,     y_video, 2160, L"mp4",  false, false},
		{138, y_mp4,     y_video, 2160, L"mp4",  false, false},
		{264, y_mp4,     y_video, 1440, L"mp4",  false, false},
		{299, y_mp4,     y_video, 1080, L"mp4",  true , false},
		{137, y_mp4,     y_video, 1080, L"mp4",  false, false},
		{298, y_mp4,     y_video,  720, L"mp4",  true , false},
		{ 22, y_mp4,     y_media,  720, L"mp4",  false, false}, // H.264 + AAC
		{136, y_mp4,     y_video,  720, L"mp4",  false, false},
		{135, y_mp4,     y_video,  480, L"mp4",  false, false},
		{ 18, y_mp4,     y_media,  360, L"mp4",  false, false}, // H.264 + AAC
		{134, y_mp4,     y_video,  360, L"mp4",  false, false},
		{133, y_mp4,     y_video,  240, L"mp4",  false, false},
		// WebM (VP9)
		{272, y_webm,    y_video, 4320, L"webm", true,  false},
		{272, y_webm,    y_video, 4320, L"webm", false, false},
		{272, y_webm,    y_video, 2880, L"webm", false, false},
		{337, y_webm,    y_video, 2160, L"webm", true , true },
		{315, y_webm,    y_video, 2160, L"webm", true , false},
		{313, y_webm,    y_video, 2160, L"webm", false, false},
		{336, y_webm,    y_video, 1440, L"webm", true , true },
		{308, y_webm,    y_video, 1440, L"webm", true , false},
		{271, y_webm,    y_video, 1440, L"webm", false, false},
		{335, y_webm,    y_video, 1080, L"webm", true , true },
		{303, y_webm,    y_video, 1080, L"webm", true , false},
		{248, y_webm,    y_video, 1080, L"webm", false, false},
		{334, y_webm,    y_video,  720, L"webm", true , true },
		{302, y_webm,    y_video,  720, L"webm", true , false},
		{247, y_webm,    y_video,  720, L"webm", false, false},
		{244, y_webm,    y_video,  480, L"webm", false, false},
		{ 43, y_webm,    y_media,  360, L"webm", false, false}, // VP8 + Vorbis
		{243, y_webm,    y_video,  360, L"webm", false, false},
		{242, y_webm,    y_video,  240, L"webm", false, false},
		// MP4 (AV1)
		{702, y_mp4_av1, y_video, 4320, L"mp4",  false, true},
		{701, y_mp4_av1, y_video, 2160, L"mp4",  false, true},
		{700, y_mp4_av1, y_video, 1440, L"mp4",  false, true},
		{699, y_mp4_av1, y_video, 1080, L"mp4",  false, true},
		{698, y_mp4_av1, y_video,  720, L"mp4",  false, true},
		{697, y_mp4_av1, y_video,  480, L"mp4",  false, true},
		{696, y_mp4_av1, y_video,  360, L"mp4",  false, true},
		{695, y_mp4_av1, y_video,  240, L"mp4",  false, true},

		{571, y_mp4_av1, y_video, 4320, L"mp4",  false, false},
		{402, y_mp4_av1, y_video, 4320, L"mp4",  false, false},
		{401, y_mp4_av1, y_video, 2160, L"mp4",  false, false},
		{400, y_mp4_av1, y_video, 1440, L"mp4",  false, false},
		{399, y_mp4_av1, y_video, 1080, L"mp4",  false, false},
		{398, y_mp4_av1, y_video,  720, L"mp4",  false, false},
		{397, y_mp4_av1, y_video,  480, L"mp4",  false, false},
		{396, y_mp4_av1, y_video,  360, L"mp4",  false, false},
		{395, y_mp4_av1, y_video,  240, L"mp4",  false, false},
		// live (ts)
		{267, y_mp4,     y_media, 2160, L"ts",   false, false, true},
		{265, y_mp4,     y_media, 1440, L"ts",   false, false, true},
		{301, y_mp4,     y_media, 1080, L"ts",   true , false, true},
		{300, y_mp4,     y_media,  720, L"ts",   true , false, true},
		{ 96, y_mp4,     y_media, 1080, L"ts",   false, false, true},
		{ 95, y_mp4,     y_media,  720, L"ts",   false, false, true},
		{ 94, y_mp4,     y_media,  480, L"ts",   false, false, true},
		{ 93, y_mp4,     y_media,  360, L"ts",   false, false, true},
		{ 92, y_mp4,     y_media,  240, L"ts",   false, false, true},
	};

	static const YoutubeProfile YAudioProfiles[] = {
		// AAC
		{258, y_mp4,  y_audio,  384, L"m4a"},
		{256, y_mp4,  y_audio,  192, L"m4a"},
		{140, y_mp4,  y_audio,  128, L"m4a"},
		{139, y_mp4,  y_audio,   48, L"m4a"}, // may be outdated and no longer supported
		// Opus
		{251, y_webm, y_audio,  160, L"webm"},
		{250, y_webm, y_audio,   70, L"webm"},
		{249, y_webm, y_audio,   50, L"webm"},
		// Vorbis
		//{249, y_webm, y_audio, 128, L"webm", false, false},
	};

	struct YoutubeFields {
		CString        author;
		CString        title;
		CString        content;
		CString        fname;
		SYSTEMTIME     dtime    = { 0 };
		REFERENCE_TIME duration = 0;
		CString        thumbnailUrl;
		std::vector<Chapters> chaptersList;

		void Empty() {
			author.Empty();
			title.Empty();
			content.Empty();
			fname.Empty();
			dtime = { 0 };
			duration = 0;
			thumbnailUrl.Empty();
			chaptersList.clear();
		}
	};

	struct YoutubePlaylistItem {
		CString url, title;
		REFERENCE_TIME duration = 0;

		YoutubePlaylistItem() = default;
		YoutubePlaylistItem(const CString& _url, const CString& _title, const REFERENCE_TIME _duration = 0)
			: url(_url)
			, title(_title)
			, duration(_duration) {};
	};
	typedef std::vector<YoutubePlaylistItem> YoutubePlaylist;

	struct YoutubeUrllistItem : YoutubePlaylistItem {
		const YoutubeProfile* profile;
		CString thumbnailUrl;
	};
	typedef std::vector<YoutubeUrllistItem> YoutubeUrllist;

	bool CheckURL(CString url);
	bool CheckPlaylist(CString url);

	bool Parse_URL(CString url, std::list<CString>& urls, YoutubeFields& y_fields, YoutubeUrllist& youtubeUrllist, YoutubeUrllist& youtubeAudioUrllist,
				   CSubtitleItemList& subs, REFERENCE_TIME& rtStart, CString& errorMessage);
	bool Parse_Playlist(CString url, YoutubePlaylist& youtubePlaylist, int& idx_CurrentPlay);

	bool Parse_URL(CString url, YoutubeFields& y_fields);

	const YoutubeUrllistItem* GetAudioUrl(const YoutubeProfile* profile, const YoutubeUrllist& youtubeAudioUrllist);
}
