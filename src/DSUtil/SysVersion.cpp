/*
 * Copyright (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru).
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
#include <Windows.h>
#include "SysVersion.h"

static OSVERSIONINFOEX osvi = { sizeof(OSVERSIONINFOEX), 0, 0, 0, 0, _T(""), 0, 0, 0, 0, 0 };

WORD GetSysVersion()
{
	static WORD ver = 0;

	if (osvi.dwMajorVersion == 0) {
		GetVersionEx((LPOSVERSIONINFOW)&osvi);
		ver = MAKEWORD(osvi.dwMinorVersion, osvi.dwMajorVersion);
	}

	return ver;
}

BOOL IsWinXP()
{
	return (GetSysVersion() == 0x0501);
}

BOOL IsWinXPOrLater()
{
	return (GetSysVersion() >= 0x0501);
}

BOOL IsWinVista()
{
	return (GetSysVersion() == 0x0600);
}

BOOL IsWinVistaOrLater()
{
	return (GetSysVersion() >= 0x0600);
}

BOOL IsWinSeven()
{
	return (GetSysVersion() == 0x0601);
}

BOOL IsWinSevenOrLater()
{
	return (GetSysVersion() >= 0x0601);
}

BOOL IsWinEight()
{
	return (GetSysVersion() == 0x0602);
}

BOOL IsWinEightOrLater()
{
	return (GetSysVersion() >= 0x0602);
}

BOOL IsWinTen()
{
	return (GetSysVersion() == 0x0A00);
}

BOOL IsWinTenOrLater()
{
	return (GetSysVersion() >= 0x0A00);
}

BOOL IsWow64()
{
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	static LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
	BOOL bIsWow64 = FALSE;
	if (NULL == fnIsWow64Process) {
		fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");
	}
	if (NULL != fnIsWow64Process) {
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	}
	return bIsWow64;
}

BOOL IsW64()
{
#ifdef _WIN64
	return TRUE;
#endif

	return IsWow64();
}
