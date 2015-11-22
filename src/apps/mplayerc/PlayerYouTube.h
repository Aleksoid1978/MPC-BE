/*
 * (C) 2012-2015 see Authors.txt
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

#include <afxinet.h>

#define YOUTUBE_MP_URL	"://www.youtube.com/"
#define YOUTUBE_PL_URL	_T("youtube.com/playlist?")
#define YOUTUBE_URL		_T("youtube.com/watch?")
#define YOUTUBE_URL_V	_T("youtube.com/v/")
#define YOUTU_BE_URL	_T("youtu.be/")

#define ENABLE_YOUTUBE_3D	0
#define ENABLE_YOUTUBE_DASH	0

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

struct YOUTUBE_PROFILES {
	const int	iTag;
	const ytype	type;
	const int	quality;
	LPCTSTR		ext;
};

static const YOUTUBE_PROFILES youtubeVideoProfiles[] = {
	{22,	y_mp4,				 720,	_T("mp4") },
	{18,	y_mp4,				 360,	_T("mp4") },
	{43,	y_webm,				 360,	_T("webm")},
	{5,		y_flv,				 240,	_T("flv") },
	{36,	y_3gp,				 240,	_T("3gp") },
	{17,	y_3gp,				 144,	_T("3gp") },
	// VP9, 60fps
	{315,	y_webm_video_60fps,	2160,	_T("webm")},
	{308,	y_webm_video_60fps,	1440,	_T("webm")},
	{303,	y_webm_video_60fps,	1080,	_T("webm")},
	// VP9
	{313,	y_webm_video,		2160,	_T("webm")},
	{271,	y_webm_video,		1440,	_T("webm")},
	{248,	y_webm_video,		1080,	_T("webm")},
#if ENABLE_YOUTUBE_3D
	{84,	y_3d_mp4,			 720,	_T("mp4") },
	{83,	y_3d_mp4,			 480,	_T("mp4") },
	{82,	y_3d_mp4,			 360,	_T("mp4") },
	{100,	y_3d_webm,			 360,	_T("webm")},
#endif
#if ENABLE_YOUTUBE_DASH
	{138 	y_dash_mp4_video,	2160,	_T("mp4") },
	{137,	y_dash_mp4_video,	1080,	_T("mp4") },
	{136,	y_dash_mp4_video,	 720,	_T("mp4") },
	{135,	y_dash_mp4_video,	 480,	_T("mp4") },
	{134,	y_dash_mp4_video,	 360,	_T("mp4") },
	{133,	y_dash_mp4_video,	 240,	_T("mp4") },
	{160,	y_dash_mp4_video,	 144,	_T("mp4") }, // 15fps
	//
	{266,	y_dash_mp4_video,	2160,	_T("mp4") }, // h264, 30fps
	{299,	y_dash_mp4_video,	1080,	_T("mp4") }, // h264, 60fps
	{298,	y_dash_mp4_video,	 720,	_T("mp4") }, // h264, 60fps
#endif
};

static const YOUTUBE_PROFILES youtubeAudioProfiles[] = {
	{251,	y_webm_audio,		 256,	_T("webm")},
	{250,	y_webm_audio,		  64,	_T("webm")},
	{249,	y_webm_audio,		  48,	_T("webm")},
	{172,	y_webm_audio,		 192,	_T("webm")},
	{171,	y_webm_audio,		 128,	_T("webm")},
#if ENABLE_YOUTUBE_DASH
	{141,	y_dash_mp4_audio,	 256,	_T("m4a") },
	{140,	y_dash_mp4_audio,	 128,	_T("m4a") },
#endif
};

static const YOUTUBE_PROFILES youtubeProfileEmpty = {0, y_unknown, 0, NULL};

struct YOUTUBE_FIELDS {
	CString		author;
	CString		title;
	CString		content;
	CString		fname;
	SYSTEMTIME	dtime;

	YOUTUBE_FIELDS() {
		dtime = { 0 };
	}
	void Empty() {
		author.Empty();
		title.Empty();
		content.Empty();
		fname.Empty();
		dtime = { 0 };
	}
};

static DWORD strpos(char* h, char* n)
{
	char* p = strstr(h, n);

	if (p) {
		return p - h;
	}

	return 0;
}

bool PlayerYouTubeCheck(CString url);
bool PlayerYouTubePlaylistCheck(CString url);
CString PlayerYouTube(CString url, YOUTUBE_FIELDS& y_fields, CSubtitleItemList& subs);

struct YoutubePlaylistItem {
	CString url, title;

	YoutubePlaylistItem() {};
	YoutubePlaylistItem(CString url, CString title)
		: url(url)
		, title(title) {};
};
typedef CAtlList<YoutubePlaylistItem> YoutubePlaylist;

bool PlayerYouTubePlaylist(CString url, YoutubePlaylist& youtubePlaylist, int& idx_CurrentPlay);
