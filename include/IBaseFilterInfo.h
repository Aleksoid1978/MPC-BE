/*
 * (C) 2016-2017 see Authors.txt
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
interface __declspec(uuid("3F56FEBC-633C-4C76-8455-0787FC62C8F8"))
IBaseFilterInfo :
public IUnknown {
	// The memory for strings and binary data is allocated by the callee
	// by using LocalAlloc. It is the caller's responsibility to release the
	// memory by calling LocalFree.
	STDMETHOD(GetInt   )(LPCSTR field, int    *value) PURE;
	STDMETHOD(GetString)(LPCSTR field, LPWSTR *value, size_t *chars) PURE;
	STDMETHOD(GetBin   )(LPCSTR field, LPVOID *value, size_t *size ) PURE;
};
