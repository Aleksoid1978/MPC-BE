/*
 * (C) 2012-2026 see Authors.txt
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

#include "OpenMediaData.h"
#include "DSUtil/CUE.h"

namespace Youtube
{
	struct YoutubeFields {
		CString        author;
		CString        title;
		CString        content;
		CString        fname;
		SYSTEMTIME     dtime    = { 0 };
		REFERENCE_TIME duration = 0;
		CString        thumbnailUrl;
		CString        userAgent;
		std::vector<Chapters> chaptersList;

		void Empty() {
			author.Empty();
			title.Empty();
			content.Empty();
			fname.Empty();
			dtime = { 0 };
			duration = 0;
			thumbnailUrl.Empty();
			userAgent.Empty();
			chaptersList.clear();
		}
	};

	bool CheckYtVideoURL(CString url);
	bool CheckYtPlaylistURL(CString url);

	bool Parse_Playlist(CString url, CFileItemList& youtubePlaylist, int& idx_CurrentPlay);

	bool ParseMetadata(CString url, YoutubeFields& y_fields);
}
