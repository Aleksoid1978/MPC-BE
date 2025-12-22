/*
 * (C) 2025 see Authors.txt
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

#include "ID3V2PictureType.h"

// ID3v2.3 Attached picture - https://id3.org/id3v2.3.0#Attached_picture
// FLAC Attached Picture - https://www.rfc-editor.org/rfc/rfc9639.html#table13
// WM_PICTURE - https://learn.microsoft.com/ru-ru/previous-versions/windows/desktop/api/wmsdkidl/ns-wmsdkidl-wm_picture

static LPCWSTR picture_types[] = {
	L"Other",
	L"Icon32x32.png",
	L"Icon",
	L"Front cover",
	L"Back cover",
	L"Leaflet page",
	L"Media lable",
	L"Lead artist(performer)",
	L"Artist(performer)",
	L"Conductor",
	L"Band(Orchestra)",
	L"Composer",
	L"Lyricist(text writer)",
	L"Recording Location",
	L"During recording",
	L"During performance",
	L"Movie(video) screen capture",
	L"A bright coloured fish",
	L"Illustration",
	L"Band(artist) logotype",
	L"Publisher(Studio) logotype"
};

LPCWSTR ID3v2PictureTypeToStr(BYTE pictureType)
{
	if (pictureType < std::size(picture_types)) {
		return picture_types[pictureType];
	}
	return L"";
}
