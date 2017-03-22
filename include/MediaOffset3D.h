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

// {8F18231E-B24C-48C8-B776-891DDF6F077E}
DEFINE_GUID(IID_MediaOffset3D, 
	0x8f18231e, 0xb24c, 0x48c8, 0xb7, 0x76, 0x89, 0x1d, 0xdf, 0x6f, 0x7, 0x7e);

#pragma pack(push, 1)
struct MediaOffset3D {
	REFERENCE_TIME timestamp;
	MediaSideData3DOffset offset;
};
#pragma pack(pop)
