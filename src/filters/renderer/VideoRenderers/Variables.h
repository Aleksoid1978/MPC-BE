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
 *
 */

#pragma once

enum FRAME_TYPE {
	PICT_NONE,
	PICT_TOP_FIELD,
	PICT_BOTTOM_FIELD,
	PICT_FRAME
};

inline REFERENCE_TIME g_tSegmentStart    = 0;
inline FRAME_TYPE     g_nFrameType       = PICT_NONE;
inline HANDLE         g_hNewSegmentEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

inline bool g_bNoDuration           = false;
inline bool g_bExternalSubtitleTime = false;
