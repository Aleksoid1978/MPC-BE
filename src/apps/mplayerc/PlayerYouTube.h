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

#pragma once

#define ENABLE_YOUTUBE_3D   0
#define ENABLE_YOUTUBE_DASH 0

namespace YoutubeParser {
	enum ytype {
		y_unknown,
		y_mp4,
		y_webm,
		y_flv,
		y_3gp,
		y_3d_mp4,
		y_3d_webm,
		y_apple_live,
		y_dash_mp4_video,
		y_dash_mp4_audio,
		y_webm_video,
		y_webm_video_60fps,
		y_webm_audio,
	};

	struct YoutubeProfiles {
		const int   iTag;
		const ytype type;
		const int   quality;
		LPCTSTR     ext;
		const bool  videoOnly;
	};

	static const YoutubeProfiles youtubeVideoProfiles[] = {
		{22,	y_mp4,				 720,	_T("mp4"),	false },
		{18,	y_mp4,				 360,	_T("mp4"),	false },
		{43,	y_webm,				 360,	_T("webm"),	false },
		{5,		y_flv,				 240,	_T("flv"),	false },
		{36,	y_3gp,				 240,	_T("3gp"),	false },
		{17,	y_3gp,				 144,	_T("3gp"),	false },
		// VP9, 60fps
		{315,	y_webm_video_60fps,	2160,	_T("webm"),	true },
		{308,	y_webm_video_60fps,	1440,	_T("webm"),	true },
		{303,	y_webm_video_60fps,	1080,	_T("webm"),	true },
		{302,	y_webm_video_60fps,	 720,	_T("webm"),	true },
		// VP9
		{313,	y_webm_video,		2160,	_T("webm"),	true },
		{271,	y_webm_video,		1440,	_T("webm"),	true },
		{248,	y_webm_video,		1080,	_T("webm"),	true },
		{247,	y_webm_video,		 720,	_T("webm"),	true },
	#if ENABLE_YOUTUBE_3D
		{84,	y_3d_mp4,			 720,	_T("mp4"),	false },
		{83,	y_3d_mp4,			 480,	_T("mp4"),	false },
		{82,	y_3d_mp4,			 360,	_T("mp4"),	false },
		{100,	y_3d_webm,			 360,	_T("webm"),	false },
	#endif
	#if ENABLE_YOUTUBE_DASH
		{138 	y_dash_mp4_video,	2160,	_T("mp4"),	true },
		{137,	y_dash_mp4_video,	1080,	_T("mp4"),	true },
		{136,	y_dash_mp4_video,	 720,	_T("mp4"),	true },
		{135,	y_dash_mp4_video,	 480,	_T("mp4"),	true },
		{134,	y_dash_mp4_video,	 360,	_T("mp4"),	true },
		{133,	y_dash_mp4_video,	 240,	_T("mp4"),	true },
		{160,	y_dash_mp4_video,	 144,	_T("mp4"),	true }, // 15fps
		//
		{266,	y_dash_mp4_video,	2160,	_T("mp4"),	true }, // h264, 30fps
		{299,	y_dash_mp4_video,	1080,	_T("mp4"),	true }, // h264, 60fps
		{298,	y_dash_mp4_video,	 720,	_T("mp4"),	true }, // h264, 60fps
	#endif
	};

	static const YoutubeProfiles youtubeAudioProfiles[] = {
		{251,	y_webm_audio,		 256,	_T("webm"),	false },
		{250,	y_webm_audio,		  64,	_T("webm"),	false },
		{249,	y_webm_audio,		  48,	_T("webm"),	false },
		{172,	y_webm_audio,		 192,	_T("webm"),	false },
		{171,	y_webm_audio,		 128,	_T("webm"),	false },
	#if ENABLE_YOUTUBE_DASH
		{141,	y_dash_mp4_audio,	 256,	_T("m4a"),	false },
		{140,	y_dash_mp4_audio,	 128,	_T("m4a"),	false },
	#endif
	};

	struct YoutubeFields {
		CString        author;
		CString        title;
		CString        content;
		CString        fname;
		SYSTEMTIME     dtime    = { 0 };
		REFERENCE_TIME duration = 0;

		void Empty() {
			author.Empty();
			title.Empty();
			content.Empty();
			fname.Empty();
			dtime = { 0 };
			duration = 0;
		}
	};

	struct YoutubePlaylistItem {
		CString url, title;

		YoutubePlaylistItem() {};
		YoutubePlaylistItem(CString _url, CString _title)
			: url(_url)
			, title(_title) {};
	};
	typedef CAtlList<YoutubePlaylistItem> YoutubePlaylist;

	struct YoutubeUrllistItem : YoutubePlaylistItem {
		int tag = 0;
		bool videoOnly = false;
	};
	typedef CAtlList<YoutubeUrllistItem> YoutubeUrllist;

	bool CheckURL(CString url);
	bool CheckPlaylist(CString url);

	bool Parse_URL(CString url, CAtlList<CString>& urls, YoutubeFields& y_fields, YoutubeUrllist& youtubeUrllist, CSubtitleItemList& subs, REFERENCE_TIME& rtStart);
	bool Parse_Playlist(CString url, YoutubePlaylist& youtubePlaylist, int& idx_CurrentPlay);

	bool Parse_URL(CString url, YoutubeFields& y_fields);

	const CString FormatProfiles(const YoutubeProfiles profile);
}
