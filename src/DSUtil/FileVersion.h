/*
 * (C) 2017-2018 see Authors.txt
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

#include <new>

namespace FileVersion
{
	// major.minor.build.revision
	union Ver {
		UINT64 value = 0;
		struct {
			UINT16 revision;
			UINT16 build;
			UINT16 minor;
			UINT16 major;
		};

		constexpr Ver(UINT64 _value)
			: value(_value)
		{
		}

		constexpr Ver(UINT16 _major, UINT16 _minor, UINT16 _build, UINT16 _revision)
			: major(_major), minor(_minor), build(_build), revision(_revision)
		{
		}
	};

	static Ver GetVer(LPCWSTR wstrFilename)
	{
		Ver ver(0, 0, 0, 0);

		const DWORD len = GetFileVersionInfoSizeW(wstrFilename, nullptr);
		if (len) {
			WCHAR* buf = new(std::nothrow) WCHAR[len];
			if (buf) {
				UINT uLen;
				VS_FIXEDFILEINFO* pvsf = nullptr;
				if (GetFileVersionInfoW(wstrFilename, 0, len, buf) && VerQueryValueW(buf, L"\\", (LPVOID*)&pvsf, &uLen)) {
					ver.value = ((UINT64)pvsf->dwFileVersionMS << 32) | pvsf->dwFileVersionLS;
				}

				delete[] buf;
			}
		}

		return ver;
	}
}
