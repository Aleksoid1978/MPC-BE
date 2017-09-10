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

namespace CPUInfo {
	enum PROCESSOR_TYPE {
		PROCESSOR_AMD,
		PROCESSOR_INTEL,
		PROCESSOR_UNKNOWN
	};

	enum PROCESSOR_FEATURES {
		CPU_MMX      = 0x0001,
		CPU_3DNOW    = 0x0004,
		CPU_MMXEXT   = 0x0002,
		CPU_SSE      = 0x0008,
		CPU_SSE2     = 0x0010,
		CPU_3DNOWEXT = 0x0020,
		CPU_SSE3     = 0x0040,
		CPU_SSSE3    = 0x0080,
		CPU_SSE4     = 0x0100,
		CPU_SSE42    = 0x0200,
		CPU_AVX      = 0x4000,
		CPU_AVX2     = 0x8000,
	};

	const int GetType();
	const int GetFeatures();
	const DWORD GetProcessorNumber();

	const bool HaveSSE2();
	const bool HaveSSE4();
	const bool HaveAVX2();
} // namespace CPUInfo
