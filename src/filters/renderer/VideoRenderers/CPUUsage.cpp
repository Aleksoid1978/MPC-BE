/*
 *
 * Copyright (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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
 * use source from http://www.philosophicalgeek.com/2009/01/03/determine-cpu-usage-of-current-process-c-and-c/
 *
 */

#include "StdAfx.h"
#include "CPUUsage.h"

CCPUUsage::CCPUUsage()
	: m_nCPUUsage(0)
	, m_dwLastRun(0)
	, m_lRunCount(0)
{
	ZeroMemory(&m_ftPrevSysKernel, sizeof(FILETIME));
	ZeroMemory(&m_ftPrevSysUser, sizeof(FILETIME));

	ZeroMemory(&m_ftPrevProcKernel, sizeof(FILETIME));
	ZeroMemory(&m_ftPrevProcUser, sizeof(FILETIME));

	GetUsage();
}


/**********************************************
* CCPUUsage::GetUsage
* returns the percent of the CPU that this process
* has used since the last time the method was called.
* If there is not enough information, -1 is returned.
* If the method is recalled to quickly, the previous value
* is returned.
***********************************************/
const short CCPUUsage::GetUsage()
{
	//create a local copy to protect against race conditions in setting the
	//member variable
	short nCpuCopy = m_nCPUUsage;
	if (::InterlockedIncrement(&m_lRunCount) == 1) {
		/*
		If this is called too often, the measurement itself will greatly affect the
		results.
		*/

		if (!EnoughTimePassed()) {
			::InterlockedDecrement(&m_lRunCount);
			return nCpuCopy;
		}

		FILETIME ftSysIdle, ftSysKernel, ftSysUser;
		FILETIME ftProcCreation, ftProcExit, ftProcKernel, ftProcUser;

		if (!GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser) ||
			!GetProcessTimes(GetCurrentProcess(), &ftProcCreation, &ftProcExit, &ftProcKernel, &ftProcUser)) {

			::InterlockedDecrement(&m_lRunCount);
			return nCpuCopy;
		}

		if (!IsFirstRun()) {
			/*
			CPU usage is calculated by getting the total amount of time the system has operated
			since the last measurement (made up of kernel + user) and the total
			amount of time the process has run (kernel + user).
			*/
			ULONGLONG ftSysKernelDiff	= SubtractTimes(ftSysKernel, m_ftPrevSysKernel);
			ULONGLONG ftSysUserDiff		= SubtractTimes(ftSysUser, m_ftPrevSysUser);

			ULONGLONG ftProcKernelDiff	= SubtractTimes(ftProcKernel, m_ftPrevProcKernel);
			ULONGLONG ftProcUserDiff	= SubtractTimes(ftProcUser, m_ftPrevProcUser);

			ULONGLONG nTotalSys			=  ftSysKernelDiff + ftSysUserDiff;
			ULONGLONG nTotalProc		= ftProcKernelDiff + ftProcUserDiff;

			if (nTotalSys > 0) {
				m_nCPUUsage = (short)((100.0 * nTotalProc) / nTotalSys);
			}
		}

		m_ftPrevSysKernel	= ftSysKernel;
		m_ftPrevSysUser		= ftSysUser;
		m_ftPrevProcKernel	= ftProcKernel;
		m_ftPrevProcUser	= ftProcUser;

		m_dwLastRun = GetTickCount();

		nCpuCopy = m_nCPUUsage;
	}

	::InterlockedDecrement(&m_lRunCount);

	return nCpuCopy;
}

ULONGLONG CCPUUsage::SubtractTimes(const FILETIME& ftA, const FILETIME& ftB)
{
	LARGE_INTEGER a, b;
	a.LowPart	= ftA.dwLowDateTime;
	a.HighPart	= ftA.dwHighDateTime;

	b.LowPart	= ftB.dwLowDateTime;
	b.HighPart	= ftB.dwHighDateTime;

	return a.QuadPart - b.QuadPart;
}

bool CCPUUsage::EnoughTimePassed()
{
	const int minElapsedMS = 1000;
	return (GetTickCount() - m_dwLastRun) >= minElapsedMS;
}
