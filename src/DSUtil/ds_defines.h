/*
 * (C) 2021 see Authors.txt
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

#define LCID_NOSUBTITLES    -1

#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#define SAFE_CLOSE_HANDLE(p) { if (p) { if ((p) != INVALID_HANDLE_VALUE) VERIFY(CloseHandle(p)); (p) = nullptr; } }

#define EXIT_ON_ERROR(hres)  { if (FAILED(hres)) return hres; }

#define IsWaveFormatExtensible(wfe) (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe->cbSize == 22)

#define QI(i)  (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :
#define QI2(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :

#define BeginEnumFilters(pFilterGraph, pEnumFilters, pBaseFilter) \
	{CComPtr<IEnumFilters> pEnumFilters; \
	if (pFilterGraph && SUCCEEDED(pFilterGraph->EnumFilters(&pEnumFilters))) \
	{ \
		pEnumFilters->Reset(); \
		for (CComPtr<IBaseFilter> pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = nullptr) \
		{ \

#define EndEnumFilters }}}

#define BeginEnumCachedFilters(pGraphConfig, pEnumFilters, pBaseFilter) \
	{CComPtr<IEnumFilters> pEnumFilters; \
	if (pGraphConfig && SUCCEEDED(pGraphConfig->EnumCacheFilter(&pEnumFilters))) \
	{ \
		for (CComPtr<IBaseFilter> pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = nullptr) \
		{ \

#define EndEnumCachedFilters }}}

#define BeginEnumPins(pBaseFilter, pEnumPins, pPin) \
	{CComPtr<IEnumPins> pEnumPins; \
	if (pBaseFilter && SUCCEEDED(pBaseFilter->EnumPins(&pEnumPins))) \
	{ \
		for (CComPtr<IPin> pPin; S_OK == pEnumPins->Next(1, &pPin, 0); pPin = nullptr) \
		{ \

#define EndEnumPins }}}

#define BeginEnumMediaTypes(pPin, pEnumMediaTypes, pMediaType) \
	{CComPtr<IEnumMediaTypes> pEnumMediaTypes; \
	if (pPin && SUCCEEDED(pPin->EnumMediaTypes(&pEnumMediaTypes))) \
	{ \
		AM_MEDIA_TYPE* pMediaType = nullptr; \
		for (; S_OK == pEnumMediaTypes->Next(1, &pMediaType, nullptr); DeleteMediaType(pMediaType), pMediaType = nullptr) \
		{ \

#define EndEnumMediaTypes(pMediaType) } if (pMediaType) DeleteMediaType(pMediaType); }}

#define BeginEnumSysDev(clsid, pMoniker) \
	{CComPtr<ICreateDevEnum> pDevEnum4$##clsid; \
	if (SUCCEEDED(pDevEnum4$##clsid.CoCreateInstance(CLSID_SystemDeviceEnum))) \
	{ \
		CComPtr<IEnumMoniker> pClassEnum4$##clsid; \
		if (SUCCEEDED(pDevEnum4$##clsid->CreateClassEnumerator(clsid, &pClassEnum4$##clsid, 0)) \
		&& pClassEnum4$##clsid) \
		{ \
			for (CComPtr<IMoniker> pMoniker; pClassEnum4$##clsid->Next(1, &pMoniker, 0) == S_OK; pMoniker = nullptr) \
			{ \

#define EndEnumSysDev }}}}
