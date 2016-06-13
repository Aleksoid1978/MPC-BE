/*
 * (C) 2013-2016 see Authors.txt
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
#include <VersionHelpers.h>
#include "SysVersion.h"

static WORD InitGetSysVersion()
{
	const struct {
		WORD wMajor;
		WORD wMinor;
	}
	ver[] = {
		{ 5, 1 }, // XP
		{ 6, 0 }, // Vista
		{ 6, 1 }, // 7
		{ 6, 2 }, // 8
		{ 6, 3 }, // 8.1
		{ 10, 0 } // 10
	};

	int i = 1;
	for (; i < _countof(ver); i++) {
		if (!IsWindowsVersionOrGreater(ver[i].wMajor, ver[i].wMinor, 0)) {
			i--;
			break;
		}
	}
	ASSERT(ver[i].wMajor >= 6);

	return MAKEWORD(ver[i].wMinor,ver[i].wMajor);
}

WORD GetSysVersion()
{
	const static WORD ver = InitGetSysVersion();
	return ver;
}

BOOL IsWinXP()
{
	return (GetSysVersion() == 0x0501);
}

BOOL IsWinXPorLater()
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

BOOL IsWin7()
{
	return (GetSysVersion() == 0x0601);
}

BOOL IsWin7orLater()
{
	return (GetSysVersion() >= 0x0601);
}

BOOL IsWin8()
{
	return (GetSysVersion() == 0x0602);
}

BOOL IsWin8orLater()
{
	return (GetSysVersion() >= 0x0602);
}

BOOL IsWin81orLater()
{
	return (GetSysVersion() >= 0x0603);
}

BOOL IsWin10()
{
	return (GetSysVersion() == 0x0A00);
}

BOOL IsWin10orLater()
{
	return (GetSysVersion() >= 0x0A00);
}

static BOOL InitIsW64()
{
#ifdef _WIN64
	return TRUE;
#endif

	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");
	BOOL bIsWow64 = FALSE;
	if (fnIsWow64Process) {
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	}

	return bIsWow64;
}

BOOL IsW64()
{
	const static BOOL bIsW64 = InitIsW64();
	return bIsW64;
}
