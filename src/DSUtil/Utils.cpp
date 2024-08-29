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

#include "stdafx.h"
#include <basestruct.h>
#include <d3d9.h>
#include <Mferror.h>
#include "Utils.h"

uint32_t CountBits(uint32_t v)
{
	// used code from \VirtualDub\h\vd2\system\bitmath.h (VDCountBits)
	v -= (v >> 1) & 0x55555555;
	v = ((v & 0xcccccccc) >> 2) + (v & 0x33333333);
	v = (v + (v >> 4)) & 0x0f0f0f0f;
	return (v * 0x01010101) >> 24;
}

uint32_t BitNum(uint32_t v, uint32_t b)
{
	ASSERT(b != 0 && (b & (b - 1)) == 0);
	ASSERT(v & b);

	return CountBits(v & (b - 1));
}

void fill_u32(void* dst, uint32_t c, size_t count)
{
#ifndef _WIN64
	__asm {
		mov eax, c
		mov ecx, count
		mov edi, dst
		cld
		rep stosd
	}
#else
	size_t& n = count;
	size_t o = n - (n % 4);

	__m128i val = _mm_set1_epi32((int)c);
	if (((uintptr_t)dst & 0x0F) == 0) { // 16-byte aligned
		for (size_t i = 0; i < o; i += 4) {
			_mm_store_si128((__m128i*) & (((DWORD*)dst)[i]), val);
		}
	}
	else {
		for (size_t i = 0; i < o; i += 4) {
			_mm_storeu_si128((__m128i*) & (((DWORD*)dst)[i]), val);
		}
	}

	switch (n - o) {
	case 3:
		((DWORD*)dst)[o + 2] = c;
	case 2:
		((DWORD*)dst)[o + 1] = c;
	case 1:
		((DWORD*)dst)[o + 0] = c;
	}
#endif
}

void memset_u32(void* dst, uint32_t c, size_t nbytes)
{
	fill_u32(dst, c, nbytes / 4);
}

void memset_u16(void* dst, uint16_t c, size_t nbytes)
{
	memset_u32(dst, c << 16 | c, nbytes);

	size_t n = nbytes / 2;
	size_t o = (n / 2) * 2;
	if ((n - o) == 1) {
		((WORD*)dst)[o] = c;
	}
}

uint64_t RescaleU64x32(uint64_t a, uint32_t b, uint32_t c)
{
	// used code from \VirtualDub\system\source\math.cpp (VDFractionScale64)
	uint32_t a0 = (uint32_t)a;
	uint32_t a1 = (uint32_t)(a >> 32);

	uint64_t m0 = (uint64_t)a0 * b;
	uint64_t m1 = (uint64_t)a1 * b;

	// collect all multiplier terms
	uint32_t s0  = (uint32_t)m0;
	uint32_t s1a = (uint32_t)(m0 >> 32);
	uint32_t s1b = (uint32_t)m1;
	uint32_t s2  = (uint32_t)(m1 >> 32);

	// form 96-bit intermediate product
	uint32_t acc0 = s0;
	uint32_t acc1 = s1a + s1b;
	uint32_t acc2 = s2 + (acc1 < s1b);

	// check for overflow (or divide by zero)
	if (acc2 >= c)
		return 0xFFFFFFFFFFFFFFFFULL;

	// do divide
	uint64_t div1 = ((uint64_t)acc2 << 32) + acc1;
	uint64_t q1 = div1 / c;
	uint64_t div0 = ((div1 % c) << 32) + acc0;
	uint32_t q0 = (uint32_t)(div0 / c);

	return (q1 << 32) + q0;
}

int64_t RescaleI64x32(int64_t a, uint32_t b, uint32_t c)
{
	return a < 0 ? -(int64_t)RescaleU64x32(-a, b, c) : RescaleU64x32(a, b, c);
}

int64_t RescaleI64(int64_t a, int64_t b, int64_t c)
{
	return llMulDiv(a, b, c, 0);
}

// code from ffmpeg
int64_t av_gcd(int64_t a, int64_t b) {
	if (b) return av_gcd(b, a%b);
	else  return a;
}

int av_reduce(int *dst_num, int *dst_den,
	int64_t num, int64_t den, int64_t max)
{
	fraction_t a0 = { 0, 1 }, a1 = { 1, 0 };
	int sign = (num < 0) ^ (den < 0);
	int64_t gcd = av_gcd(abs(num), abs(den));

	if (gcd) {
		num = abs(num) / gcd;
		den = abs(den) / gcd;
	}
	if (num <= max && den <= max) {
		a1 = { (int)num, (int)den };
		den = 0;
	}

	while (den) {
		int64_t x        = num / den;
		int64_t next_den = num - den * x;
		int64_t a2n      = x * a1.num + a0.num;
		int64_t a2d      = x * a1.den + a0.den;

		if (a2n > max || a2d > max) {
			if (a1.num) x =             (max - a0.num) / a1.num;
			if (a1.den) x = std::min(x, (max - a0.den) / a1.den);

			if (den * (2 * x * a1.den + a0.den) > num * a1.den)
				a1 = { int(x * a1.num + a0.num), int(x * a1.den + a0.den) };
			break;
		}

		a0 = a1;
		a1 = { (int)a2n, (int)a2d };
		num = den;
		den = next_den;
	}
	ASSERT(av_gcd(a1.num, a1.den) <= 1U);

	*dst_num = sign ? -a1.num : a1.num;
	*dst_den = a1.den;

	return den == 0;
}

fraction_t av_d2q(double d, int max)
{
#define LOG2  0.69314718055994530941723212145817656807550013436025
	fraction_t a;
	int exponent;
	INT64 den;
	if (isnan(d))
		return { 0,0 };
	if (fabs(d) > INT_MAX + 3LL)
		return{ d < 0 ? -1 : 1, 0 };
	exponent = std::max((int)(log(fabs(d) + 1e-20) / LOG2), 0);
	den = 1LL << (61 - exponent);
	// (int64_t)rint() and llrint() do not work with gcc on ia64 and sparc64
	av_reduce(&a.num, &a.den, floor(d * den + 0.5), den, max);
	if ((!a.num || !a.den) && d && max>0 && max<INT_MAX)
		av_reduce(&a.num, &a.den, floor(d * den + 0.5), den, INT_MAX);

	return a;
}
//

SIZE ReduceDim(double value)
{
	fraction_t a = av_d2q(value, INT_MAX);
	return{ a.num, a.den };
}

unsigned RoundUp(const unsigned value, const unsigned base)
{
	auto rem = value % base;
	return rem ? value - rem + base : value;
}

int IncreaseByGrid(int value, const int step)
{
	auto r = value % step;
	value -= r;
	return r<0 ? value : value + step;
}

int DecreaseByGrid(int value, const int step)
{
	auto r = value % step;
	value -= r;
	return r>0 ? value : value - step;
}

double IncreaseFloatByGrid(double value, const int step)
{
	if (step > 0) {
		value /= step;
	} else {
		value *= -step;
	}

	value = std::floor(value + 0.0625) + 1;

	if (step < 0) {
		value /= -step;
	} else {
		value *= step;
	}

	return value;
}

double DecreaseFloatByGrid(double value, const int step)
{
	if (step > 0) {
		value /= step;
	} else {
		value *= -step;
	}

	value = std::ceil(value - 0.0625) - 1;

	if (step < 0) {
		value /= -step;
	} else {
		value *= step;
	}

	return value;
}

bool AngleStep90(int& angle)
{
	if (angle % 90 == 0) {
		angle %= 360;
		if (angle < 0) {
			angle += 360;
		}
		return true;
	}
	return false;
}

bool StrToInt32(const wchar_t* str, int32_t& value)
{
	wchar_t* end;
	int32_t v = wcstol(str, &end, 10);
	if (end > str) {
		value = v;
		return true;
	}
	return false;
}

bool StrToUInt32(const wchar_t* str, uint32_t& value)
{
	wchar_t* end;
	uint32_t v = wcstoul(str, &end, 10);
	if (end > str) {
		value = v;
		return true;
	}
	return false;
}

bool StrToInt64(const wchar_t* str, int64_t& value)
{
	wchar_t* end;
	int64_t v = wcstoll(str, &end, 10);
	if (end > str) {
		value = v;
		return true;
	}
	return false;
}

bool StrToUInt64(const wchar_t* str, uint64_t& value)
{
	wchar_t* end;
	uint64_t v = wcstoull(str, &end, 10);
	if (end > str) {
		value = v;
		return true;
	}
	return false;
}

bool StrHexToUInt32(const wchar_t* str, uint32_t& value)
{
	wchar_t* end;
	uint32_t v = wcstoul(str, &end, 16);
	if (end > str) {
		value = v;
		return true;
	}
	return false;
}

bool StrHexToUInt64(const wchar_t* str, uint64_t& value)
{
	wchar_t* end;
	uint64_t v = wcstoull(str, &end, 16);
	if (end > str) {
		value = v;
		return true;
	}
	return false;
}

bool StrToDouble(const wchar_t* str, double& value)
{
	wchar_t* end;
	double v = wcstod(str, &end);
	if (end > str) {
		value = v;
		return true;
	}
	return false;
}

CStringW HR2Str(const HRESULT hr)
{
	CStringW str;
#define UNPACK_VALUE(VALUE) case VALUE: str = L#VALUE; break;
#define UNPACK_HR_WIN32(VALUE) case (((VALUE) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000): str = L#VALUE; break;
	switch (hr) {
		// Common HRESULT Values https://learn.microsoft.com/en-us/windows/win32/seccrypto/common-hresult-values
		UNPACK_VALUE(S_OK);
#ifdef _WINERROR_
		UNPACK_VALUE(S_FALSE);
		UNPACK_VALUE(E_NOTIMPL);
		UNPACK_VALUE(E_NOINTERFACE);
		UNPACK_VALUE(E_POINTER);
		UNPACK_VALUE(E_ABORT);
		UNPACK_VALUE(E_FAIL);
		UNPACK_VALUE(E_UNEXPECTED);
		UNPACK_VALUE(E_ACCESSDENIED);
		UNPACK_VALUE(E_HANDLE);
		UNPACK_VALUE(E_OUTOFMEMORY);
		UNPACK_VALUE(E_INVALIDARG);
		// some COM Error Codes (Generic) https://learn.microsoft.com/en-us/windows/win32/com/com-error-codes-1
		UNPACK_VALUE(REGDB_E_CLASSNOTREG);
		// some COM Error Codes (UI, Audio, DirectX, Codec) https://learn.microsoft.com/en-us/windows/win32/com/com-error-codes-10
		UNPACK_VALUE(WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT);
		UNPACK_VALUE(WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE);
		// some System Error Codes https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes
		UNPACK_HR_WIN32(ERROR_FILE_NOT_FOUND)
		UNPACK_HR_WIN32(ERROR_MOD_NOT_FOUND);
		UNPACK_HR_WIN32(ERROR_INVALID_WINDOW_HANDLE);
		UNPACK_HR_WIN32(ERROR_CLASS_ALREADY_EXISTS);
#endif
#ifdef __ERRORS__
		// some DirectShow Error and Success Codes https://learn.microsoft.com/en-us/windows/win32/directshow/error-and-success-codes
		UNPACK_VALUE(VFW_E_ENUM_OUT_OF_SYNC);
		UNPACK_VALUE(VFW_E_NOT_COMMITTED);
#endif
#ifdef _D3D9_H_
		// some D3DERR values https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3derr
		UNPACK_VALUE(S_PRESENT_OCCLUDED);
		UNPACK_VALUE(S_PRESENT_MODE_CHANGED);
		UNPACK_VALUE(D3DERR_DEVICEHUNG);
		UNPACK_VALUE(D3DERR_DEVICELOST);
		UNPACK_VALUE(D3DERR_DEVICENOTRESET);
		UNPACK_VALUE(D3DERR_DEVICEREMOVED);
		UNPACK_VALUE(D3DERR_DRIVERINTERNALERROR);
		UNPACK_VALUE(D3DERR_INVALIDCALL);
		UNPACK_VALUE(D3DERR_OUTOFVIDEOMEMORY);
		UNPACK_VALUE(D3DERR_WASSTILLDRAWING);
		UNPACK_VALUE(D3DERR_NOTAVAILABLE);
#endif
#ifdef _MFERROR_H
		// some MF_E values
		UNPACK_VALUE(MF_E_INVALIDSTREAMNUMBER);
		UNPACK_VALUE(MF_E_INVALIDMEDIATYPE);
		UNPACK_VALUE(MF_E_NOTACCEPTING);
		UNPACK_VALUE(MF_E_INVALID_FORMAT);
		UNPACK_VALUE(MF_E_TRANSFORM_NEED_MORE_INPUT);
		UNPACK_VALUE(MF_E_TRANSFORM_STREAM_CHANGE);
		UNPACK_VALUE(MF_E_TRANSFORM_TYPE_NOT_SET);
#endif
	default:
		str.Format(L"0x%08x", hr);
	};
#undef UNPACK_VALUE
#undef UNPACK_HR_WIN32
	return str;
}

// https://learn.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
//
// Usage: SetThreadName ((DWORD)-1, "MainThread");
//
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;     // Must be 0x1000.
	LPCSTR szName;    // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
void SetThreadName(DWORD dwThreadID, const char* threadName) {
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
#pragma warning(pop)
}
