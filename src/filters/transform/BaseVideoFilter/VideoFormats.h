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

#pragma once

#include <moreuuids.h>

enum ColorModel_t {
	Cm_None,
	Cm_RGB,
	Cm_YUV420,
	Cm_YUV422,
	Cm_YUV444,
};

struct VFormatDesc {
	const wchar_t*     name;
	const GUID*        subtype;
	DWORD              fourcc;
	const ColorModel_t cmodel;
	const int          packsize;
	const int          planes;
	const int          cdepth;

	bool operator == (const struct VFormatDesc& fmt) const {
		return (subtype == fmt.subtype && fourcc == fmt.fourcc);
	}

	WORD GetBihBitCount() const { 
		WORD bitCount = packsize * 8;
		if (planes > 1) {
			if (cmodel == Cm_YUV420) {
				bitCount += packsize * 4;
			} else if (cmodel == Cm_YUV422) {
				bitCount *= 2;
			} else if (cmodel == Cm_YUV444 || cmodel == Cm_RGB) {
				bitCount *= 3;
			}
		}
		return bitCount;
	}
};

constexpr VFormatDesc GetVFormatDXVA(VFormatDesc vfdesc)
{
	vfdesc.fourcc = FCC('dxva');
	return vfdesc;
}

constexpr VFormatDesc VFormat_None      = { L"unknown",   &MEDIASUBTYPE_NULL,       0xFFFFFFFF,                Cm_None,   0, 0,  0 };

constexpr VFormatDesc VFormat_YV12      = { L"YV12",      &MEDIASUBTYPE_YV12,      FCC('YV12'),                Cm_YUV420, 1, 3,  8 };
constexpr VFormatDesc VFormat_YV16      = { L"YV16",      &MEDIASUBTYPE_YV16,      FCC('YV16'),                Cm_YUV422, 1, 3,  8 };
constexpr VFormatDesc VFormat_YV24      = { L"YV24",      &MEDIASUBTYPE_YV24,      FCC('YV24'),                Cm_YUV444, 1, 3,  8 };

constexpr VFormatDesc VFormat_IYUV      = { L"IYUV",      &MEDIASUBTYPE_IYUV,      FCC('IYUV'),                Cm_YUV420, 1, 3,  8 };
constexpr VFormatDesc VFormat_YUV444P16 = { L"YUV444P16", &MEDIASUBTYPE_YUV444P16, MAKEFOURCC('Y','3',0,16),   Cm_YUV444, 2, 3, 16 };

constexpr VFormatDesc VFormat_NV12      = { L"NV12",      &MEDIASUBTYPE_NV12,      FCC('NV12'),                Cm_YUV420, 1, 2,  8 };
constexpr VFormatDesc VFormat_P010      = { L"P010",      &MEDIASUBTYPE_P010,      FCC('P010'),                Cm_YUV420, 2, 2, 10 };
constexpr VFormatDesc VFormat_P016      = { L"P016",      &MEDIASUBTYPE_P016,      FCC('P016'),                Cm_YUV420, 2, 2, 16 };
constexpr VFormatDesc VFormat_P210      = { L"P210",      &MEDIASUBTYPE_P210,      FCC('P210'),                Cm_YUV422, 2, 2, 10 };
constexpr VFormatDesc VFormat_P216      = { L"P216",      &MEDIASUBTYPE_P216,      FCC('P216'),                Cm_YUV422, 2, 2, 16 };

constexpr VFormatDesc VFormat_YUY2      = { L"YUY2",      &MEDIASUBTYPE_YUY2,      FCC('YUY2'),                Cm_YUV422, 2, 1,  8 };
constexpr VFormatDesc VFormat_Y210      = { L"Y210",      &MEDIASUBTYPE_Y210,      FCC('Y210'),                Cm_YUV422, 4, 1, 10 };
constexpr VFormatDesc VFormat_Y216      = { L"Y216",      &MEDIASUBTYPE_Y216,      FCC('Y216'),                Cm_YUV422, 4, 1, 16 };
constexpr VFormatDesc VFormat_AYUV      = { L"AYUV",      &MEDIASUBTYPE_AYUV,      FCC('AYUV'),                Cm_YUV444, 4, 1,  8 };
constexpr VFormatDesc VFormat_Y410      = { L"Y410",      &MEDIASUBTYPE_Y410,      FCC('Y410'),                Cm_YUV444, 4, 1, 10 };
constexpr VFormatDesc VFormat_Y416      = { L"Y416",      &MEDIASUBTYPE_Y416,      FCC('Y416'),                Cm_YUV444, 8, 1, 16 };

constexpr VFormatDesc VFormat_RGB24     = { L"RGB24",     &MEDIASUBTYPE_RGB24,     BI_RGB,                     Cm_RGB,    3, 1,  8 };
constexpr VFormatDesc VFormat_RGB32     = { L"RGB32",     &MEDIASUBTYPE_RGB32,     BI_RGB,                     Cm_RGB,    4, 1,  8 };
constexpr VFormatDesc VFormat_ARGB32    = { L"ARGB32",    &MEDIASUBTYPE_ARGB32,    BI_RGB,                     Cm_RGB,    4, 1,  8 };
constexpr VFormatDesc VFormat_RGB48     = { L"RGB48",     &MEDIASUBTYPE_RGB48,     MAKEFOURCC('R','G','B',48), Cm_RGB,    6, 1, 16 };
