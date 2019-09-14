/*
 * (C) 2017-2019 see Authors.txt
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

#include <FilterInterfaces.h>

class CExFilterInfoImpl : public IExFilterInfo
{
	STDMETHODIMP GetPropertyInt   (LPCSTR field, int    *value)                  { return E_NOTIMPL; };
	STDMETHODIMP GetPropertyString(LPCSTR field, LPWSTR *value, unsigned *chars) { return E_NOTIMPL; };
	STDMETHODIMP GetPropertyBin   (LPCSTR field, LPVOID *value, unsigned *size ) { return E_NOTIMPL; };
};

class CExFilterConfigImpl : public IExFilterConfig
{
	STDMETHODIMP GetBool  (LPCSTR field, bool    *value)                  { return E_NOTIMPL; };
	STDMETHODIMP GetInt   (LPCSTR field, int     *value)                  { return E_NOTIMPL; };
	STDMETHODIMP GetInt64 (LPCSTR field, __int64 *value)                  { return E_NOTIMPL; };
	STDMETHODIMP GetDouble(LPCSTR field, double  *value)                  { return E_NOTIMPL; };
	STDMETHODIMP GetString(LPCSTR field, LPWSTR  *value, unsigned *chars) { return E_NOTIMPL; };
	STDMETHODIMP GetBin   (LPCSTR field, LPVOID  *value, unsigned *size ) { return E_NOTIMPL; };

	STDMETHODIMP SetBool  (LPCSTR field, bool    value)            { return E_NOTIMPL; };
	STDMETHODIMP SetInt   (LPCSTR field, int     value)            { return E_NOTIMPL; };
	STDMETHODIMP SetInt64 (LPCSTR field, __int64 value)            { return E_NOTIMPL; };
	STDMETHODIMP SetDouble(LPCSTR field, double  value)            { return E_NOTIMPL; };
	STDMETHODIMP SetString(LPCSTR field, LPWSTR  value, int chars) { return E_NOTIMPL; };
	STDMETHODIMP SetBin   (LPCSTR field, LPVOID  value, int size ) { return E_NOTIMPL; };
};
