/*
 * (C) 2006-2016 see Authors.txt
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

#include <atlcoll.h>

static const TCHAR* MPEG2_Profile[]=
{
	L"4:2:2",
	L"High Profile",
	L"Spatially Scalable Profile",
	L"SNR Scalable Profile",
	L"Main Profile",
	L"Simple Profile",
	L"6",
	L"7",
};

static const TCHAR* MPEG2_Level[]=
{
	L"0",
	L"1",
	L"Main Level",
	L"3",
	L"High Level",
	L"High Level",
	L"High1440 Level",
	L"5",
	L"Main Level",
	L"6",
	L"Low Level",
	L"7",
	L"8",
	L"9",
	L"10",
	L"11",
};

//
template <typename T>
static T GetFormatHelper(T &_pInfo, const CMediaType *_pFormat)
{
	ASSERT(_pFormat->cbFormat >= sizeof(*_pInfo));
	_pInfo = (T)_pFormat->pbFormat;
	return _pInfo;
}

static int GetHighestBitSet32(unsigned long _Value)
{
	unsigned long Ret;
	unsigned char bNonZero = _BitScanReverse(&Ret, _Value);
	if (bNonZero) {
		return Ret;
	} else {
		return -1;
	}
}

static CString FormatBitrate(double _Bitrate)
{
	CString Temp;
	if (_Bitrate > 20000000) { // More than 2 mbit
		Temp.Format(L"%.2f mbit/s", double(_Bitrate)/1000000.0);
	} else {
		Temp.Format(L"%.1f kbit/s", double(_Bitrate)/1000.0);
	}

	return Temp;
}

static CString FormatString(const wchar_t *pszFormat, ...)
{
	CString Temp;
	ATLASSERT(AtlIsValidString(pszFormat));

	va_list argList;
	va_start(argList, pszFormat);
	Temp.FormatV(pszFormat, argList);
	va_end(argList);

	return Temp;
}

CString GetMediaTypeDesc(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter);
CString GetMediaTypeDesc(const CMediaType* pmt, LPCWSTR pName);