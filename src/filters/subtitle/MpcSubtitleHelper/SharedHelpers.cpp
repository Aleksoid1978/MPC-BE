/*
 * (C) 2016-2024 see Authors.txt
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

#include "pch.h"
#include "SharedHelpers.h"

bool LoadResource(UINT resid, CStringA& str, LPCWSTR restype)
{
	str.Empty();
	HRSRC hrsrc = FindResourceW(AfxGetApp()->m_hInstance, MAKEINTRESOURCEW(resid), restype);
	if (!hrsrc) {
		return false;
	}
	HGLOBAL hGlobal = LoadResource(AfxGetApp()->m_hInstance, hrsrc);
	if (!hGlobal) {
		return false;
	}
	DWORD size = SizeofResource(AfxGetApp()->m_hInstance, hrsrc);
	if (!size) {
		return false;
	}
	memcpy(str.GetBufferSetLength(size), LockResource(hGlobal), size);
	return true;
}
