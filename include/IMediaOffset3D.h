/*
 * (C) 2017 see Authors.txt
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
#include <IMediaSideData.h>

#pragma pack(push, 1)
struct MediaOffset3D {
	REFERENCE_TIME timestamp;
	MediaSideData3DOffset offset;
};
#pragma pack(pop)

interface __declspec(uuid("C8241FE9-7C4D-4CA2-A980-B7C9C4E117D8"))
IMediaOffset3D :
public IUnknown {
	STDMETHOD(SetSubtitles3DOffset)(const BYTE *pData, size_t size) PURE;
};
