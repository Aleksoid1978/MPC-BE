/*
 * (C) 2016 see Authors.txt
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
#include <intrin.h>
#include "CPUInfo.h"

#define CPUID_MMX      (1 << 23)
#define CPUID_SSE      (1 << 25)
#define CPUID_SSE2     (1 << 26)
#define CPUID_SSE3     (1 <<  0)
#define CPUID_SSE41    (1 << 19)
#define CPUID_SSE42    (1 << 20)
#define CPUID_AVX     ((1 << 27) | (1 << 28))
#define CPUID_AVX2    ((1 <<  5) | (1 <<  3) | (1 << 8))

// Intel specifics
#define CPUID_SSSE3    (1 <<  9)

// AMD specifics
#define CPUID_3DNOW    (1 << 31)
#define CPUID_3DNOWEXT (1 << 30)
#define CPUID_MMXEXT   (1 << 22)

static int nCPUType = CPUInfo::PROCESSOR_UNKNOWN;

static int GetCPUFeatures()
{
	unsigned nHighestFeature   = 0;
	unsigned nHighestFeatureEx = 0;
	int      nCPUFeatures      = 0;
	int      nBuff[4]  = { 0 };
	char     szMan[13] = { 0 };

	__cpuid(nBuff, 0);
	nHighestFeature   = (unsigned)nBuff[0];
	nHighestFeatureEx = (unsigned)nBuff[0];
	*(int*)&szMan[0]  = nBuff[1];
	*(int*)&szMan[4]  = nBuff[3];
	*(int*)&szMan[8]  = nBuff[2];
	if (strcmp(szMan, "GenuineIntel") == 0) {
		nCPUType = CPUInfo::PROCESSOR_INTEL;
	} else if (strcmp(szMan, "AuthenticAMD") == 0) {
		nCPUType = CPUInfo::PROCESSOR_AMD;
	}

	if (nHighestFeature >= 1) {
		__cpuid(nBuff, 1);
		if (nBuff[3] & CPUID_MMX)   nCPUFeatures |= CPUInfo::CPU_MMX;
		if (nBuff[3] & CPUID_SSE)   nCPUFeatures |= CPUInfo::CPU_SSE;
		if (nBuff[3] & CPUID_SSE2)  nCPUFeatures |= CPUInfo::CPU_SSE2;
		if (nBuff[2] & CPUID_SSE3)  nCPUFeatures |= CPUInfo::CPU_SSE3;
		if (nBuff[2] & CPUID_SSE41) nCPUFeatures |= CPUInfo::CPU_SSE4;
		if (nBuff[2] & CPUID_SSE42) nCPUFeatures |= CPUInfo::CPU_SSE42;

		// Intel specific:
		if (nCPUType == CPUInfo::PROCESSOR_INTEL) {
			if (nBuff[2] & CPUID_SSSE3) nCPUFeatures |= CPUInfo::CPU_SSSE3;
			if ((nBuff[2] & CPUID_AVX) == CPUID_AVX) {
				// Check for OS support
				const unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
				if ((xcrFeatureMask & 0x6) == 0x6) {
					nCPUFeatures |= CPUInfo::CPU_AVX;

					if (nHighestFeature >= 7) {
						__cpuid(nBuff, 7);
						if ((nBuff[1] & CPUID_AVX2) == CPUID_AVX2) {
							nCPUFeatures |= CPUInfo::CPU_AVX2;
						}
					}
				}
			}
		}
	}

	if (nCPUType == CPUInfo::PROCESSOR_AMD) {
		if (nHighestFeatureEx >= 0x80000001) {
			__cpuid(nBuff, 0x80000000);
			if (nBuff[3] & CPUID_3DNOW)    nCPUFeatures |= CPUInfo::CPU_3DNOW;
			if (nBuff[3] & CPUID_3DNOWEXT) nCPUFeatures |= CPUInfo::CPU_3DNOWEXT;
			if (nBuff[3] & CPUID_MMXEXT)   nCPUFeatures |= CPUInfo::CPU_MMXEXT;
		}
	}

	return nCPUFeatures;
}
static const int  nCPUFeatures = GetCPUFeatures();
static const bool bSSE2        = !!(nCPUFeatures & CPUInfo::CPU_SSE2);
static const bool bSSE4        = !!(nCPUFeatures & CPUInfo::CPU_SSE4);
static const bool bAVX2        = !!(nCPUFeatures & CPUInfo::CPU_AVX2);

static DWORD GetProcessorNumber()
{
	SYSTEM_INFO SystemInfo = { 0 };
	GetSystemInfo(&SystemInfo);

	return SystemInfo.dwNumberOfProcessors;
}
static const DWORD dwNumberOfProcessors = GetProcessorNumber();

namespace CPUInfo {
	const int GetType()              { return nCPUType; }
	const int GetFeatures()          { return nCPUFeatures; }
	const DWORD GetProcessorNumber() { return dwNumberOfProcessors; }

	const bool HaveSSE2()            { return bSSE2; }
	const bool HaveSSE4()            { return bSSE4; }
	const bool HaveAVX2()            { return bAVX2; }
} // namespace CPUInfo
