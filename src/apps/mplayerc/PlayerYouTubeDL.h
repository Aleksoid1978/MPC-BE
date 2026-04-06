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

#pragma once

#include "PlayerYouTube.h"

namespace YT_DLP
{
	enum yt_vcodec_type {
		vcodec_unknoun = -1,
		vcodec_h264 = 0,
		vcodec_vp9,
		vcodec_av1,
	};
	enum yt_acodec_type {
		acodec_unknoun = -1,
		acodec_aac = 0,
		acodec_opus,
	};
	enum yt_protocol_type {
		protocol_unknoun = -1,
		protocol_http = 0,
		protocol_dash,
		protocol_hls,
	};

	bool Parse_URL(
		const CStringW& url,        // input parameter
		Youtube::YoutubeFields& y_fields,
		Youtube::YoutubeUrllist& youtubeUrllist,
		Youtube::YoutubeUrllist& youtubeAudioUrllist,
		OpenFileData* pOFD
	);
	void Clear();
}
