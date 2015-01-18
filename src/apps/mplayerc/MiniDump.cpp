/*
 * (C) 2006-2014 see Authors.txt
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
#include "MiniDump.h"
#include <DbgHelp.h>
#include "../../DSUtil/FileHandle.h"
#include "../../DSUtil/WinAPIUtils.h"

CMiniDump	_Singleton;
bool		CMiniDump::m_bMiniDumpEnabled = true;

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
		CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
		CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
		CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

CMiniDump::CMiniDump()
{
#ifndef _DEBUG
	SetUnhandledExceptionFilter(UnhandledExceptionFilter);

#ifndef _WIN64
	// Enable catching in CRT (http://blog.kalmbachnet.de/?postid=75)
	// PreventSetUnhandledExceptionFilter();
#endif
#endif
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter )
{
	return NULL;
}

BOOL CMiniDump::PreventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if (hKernel32 == NULL) {
		return FALSE;
	}

	void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if (pOrgEntry == NULL) {
		return FALSE;
	}

	unsigned char newJump[100];
	DWORD dwOrgEntryAddr = (DWORD) pOrgEntry;
	dwOrgEntryAddr += 5; // add 5 for 5 op-codes for jmp far
	void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
	DWORD dwNewEntryAddr = (DWORD) pNewFunc;
	DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;

	newJump[0] = 0xE9;  // JMP absolute
	memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory(GetCurrentProcess(), pOrgEntry, newJump, sizeof(pNewFunc) + 1, &bytesWritten);
	FreeLibrary(hKernel32);

	return bRet;
}

LONG WINAPI CMiniDump::UnhandledExceptionFilter( _EXCEPTION_POINTERS *lpTopLevelExceptionFilter )
{
	LONG	retval	= EXCEPTION_CONTINUE_SEARCH;
	HMODULE	hDll	= NULL;
	TCHAR	szResult[800] = {0};
	CString strDumpPath;

	if (!m_bMiniDumpEnabled) {
		return retval;
	}

	hDll = ::LoadLibrary(GetProgramPath() + _T("DBGHELP.DLL"));

	if (hDll == NULL) {
		hDll = ::LoadLibrary(_T("DBGHELP.DLL"));
	}

	if (hDll != NULL) {
		MINIDUMPWRITEDUMP pMiniDumpWriteDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
		if (pMiniDumpWriteDump != NULL && AfxGetMyApp()->GetAppSavePath(strDumpPath)) {
            // Check that the folder actually exists
            if (!::PathFileExists(strDumpPath)) {
				VERIFY(CreateDirectory(strDumpPath, NULL));
            }
			
			strDumpPath.AppendFormat(_T("%s.exe.%d.%d.%d.%d.dmp"), AfxGetApp()->m_pszExeName, MPC_VERSION_NUM_SVN);

			HANDLE hFile = ::CreateFile(strDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
										FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile != INVALID_HANDLE_VALUE) {
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				ExInfo.ThreadId = ::GetCurrentThreadId();
				ExInfo.ExceptionPointers = lpTopLevelExceptionFilter;
				ExInfo.ClientPointers = NULL;

				BOOL bDumpCreated = pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
				if (bDumpCreated) {
					_stprintf_s(szResult, _countof(szResult), ResStr(IDS_MPC_CRASH), strDumpPath);
					retval = EXCEPTION_EXECUTE_HANDLER;
				} else {
					_stprintf_s(szResult, _countof(szResult), ResStr(IDS_MPC_MINIDUMP_FAIL), strDumpPath, GetLastError());
				}

				::CloseHandle( hFile );
			} else {
				_stprintf_s(szResult, _countof(szResult), ResStr(IDS_MPC_MINIDUMP_FAIL), strDumpPath, GetLastError());
			}
		}
		FreeLibrary(hDll);
	}

	if (szResult[0]) {
		switch (MessageBox(AfxGetMyApp()->GetMainWnd()->m_hWnd, szResult, _T("MPC-BE Mini Dump"), retval ? MB_YESNO : MB_OK)) {
			case IDYES:
				ShellExecute(NULL, _T("open"), _T("http://dev.mpc-next.ru/index.php?board=25.0"), NULL, NULL, SW_SHOWDEFAULT);
				ExploreToFile(strDumpPath);
				break;
			case IDNO:
				retval = EXCEPTION_CONTINUE_SEARCH; // rethrow the exception to make easier attaching a debugger
				break;
		}
	}

	return retval;
}
