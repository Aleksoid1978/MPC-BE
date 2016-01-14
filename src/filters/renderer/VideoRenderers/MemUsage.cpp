/*
 * (C) 2015 see Authors.txt
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

#include "StdAfx.h"
#include <Psapi.h>
#include "MemUsage.h"

const size_t CMemUsage::GetUsage()
{
	size_t szMemUsage = m_szMemUsage;
	if (::InterlockedIncrement(&m_lRunCount) == 1) {

		if (!EnoughTimePassed()) {
			::InterlockedDecrement(&m_lRunCount);
			return szMemUsage;
		}

		PROCESS_MEMORY_COUNTERS pmc = { 0 };
		if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)) && pmc.WorkingSetSize > 0) {
			m_szMemUsage = pmc.WorkingSetSize;
		
		}

		m_dwLastRun	= GetTickCount();
		szMemUsage = m_szMemUsage;
	}

	::InterlockedDecrement(&m_lRunCount);

	return szMemUsage;
}

const bool CMemUsage::EnoughTimePassed()
{
	const int minElapsedMS = 1000;
	return (GetTickCount() - m_dwLastRun) >= minElapsedMS;
}
