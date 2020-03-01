/*
 * (C) 2006-2020 see Authors.txt
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
#include "Misc.h"
#include <DbgHelp.h>
#include "../../DSUtil/FileHandle.h"
#include "../../DSUtil/WinAPIUtils.h"

typedef BOOL(WINAPI* tpSymInitialize)(
	_In_ HANDLE hProcess,
	_In_opt_ PCSTR UserSearchPath,
	_In_ BOOL fInvadeProcess
	);
typedef BOOL(__stdcall* tpSymCleanup)(
	_In_ HANDLE hProcess
	);
typedef BOOL(__stdcall* tpStackWalk64)(
	_In_ DWORD MachineType,
	_In_ HANDLE hProcess,
	_In_ HANDLE hThread,
	_Inout_ LPSTACKFRAME64 StackFrame,
	_Inout_ PVOID ContextRecord,
	_In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
	_In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
	_In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
	_In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
	);
typedef PVOID(__stdcall* tpSymFunctionTableAccess64)(
	_In_ HANDLE hProcess,
	_In_ DWORD64 AddrBase
	);
typedef DWORD64(__stdcall* tpSymGetModuleBase64)(
	_In_ HANDLE hProcess,
	_In_ DWORD64 qwAddr
	);
typedef BOOL(__stdcall* tpSymFromAddrW)(
	_In_ HANDLE hProcess,
	_In_ DWORD64 Address,
	_Out_opt_ PDWORD64 Displacement,
	_Inout_ PSYMBOL_INFOW Symbol
	);
typedef BOOL(__stdcall* tpSymGetLineFromAddrW64)(
	_In_ HANDLE hProcess,
	_In_ DWORD64 qwAddr,
	_Out_ PDWORD pdwDisplacement,
	_Out_ PIMAGEHLP_LINEW64 Line
	);

static void DumpStackTrace(HMODULE hDbhHelp, LPCWSTR lpFileName, _EXCEPTION_POINTERS* exp)
{
	if (!hDbhHelp) {
		return;
	}

	auto pSymInitialize = (tpSymInitialize)GetProcAddress(hDbhHelp, "SymInitialize");
	auto pSymCleanup = (tpSymCleanup)GetProcAddress(hDbhHelp, "SymCleanup");
	auto pStackWalk64 = (tpStackWalk64)GetProcAddress(hDbhHelp, "StackWalk64");
	auto pSymFunctionTableAccess64 = (tpSymFunctionTableAccess64)GetProcAddress(hDbhHelp, "SymFunctionTableAccess64");
	auto pSymGetModuleBase64 = (tpSymGetModuleBase64)GetProcAddress(hDbhHelp, "SymGetModuleBase64");
	auto pSymFromAddrW = (tpSymFromAddrW)GetProcAddress(hDbhHelp, "SymFromAddrW");
	auto pSymGetLineFromAddrW64 = (tpSymGetLineFromAddrW64)GetProcAddress(hDbhHelp, "SymGetLineFromAddrW64");

	if (!pSymInitialize || !pSymCleanup || !pStackWalk64 || !pSymFunctionTableAccess64
			|| !pSymGetModuleBase64 || !pSymFromAddrW || !pSymGetLineFromAddrW64) {
		return;
	}

	HANDLE process = GetCurrentProcess();

	// load symbols
	if (!pSymInitialize(process, nullptr, TRUE)) {
		return;
	}

	auto ctx = exp->ContextRecord;

	char symbolBuffer[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t)];
	auto pSymbol = (PSYMBOL_INFOW)symbolBuffer;

	HANDLE thread = GetCurrentThread();

	STACKFRAME64 frame = {};
	DWORD imageType = 0;

#ifdef _M_X64
	imageType = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset = ctx->Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = ctx->Rsp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = ctx->Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
#else
	imageType = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset = ctx->Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = ctx->Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = ctx->Esp;
	frame.AddrStack.Mode = AddrModeFlat;
#endif

	FILE* f = nullptr;
	if (_wfopen_s(&f, lpFileName, L"wt, ccs=UTF-8") == 0) {
		fwprintf_s(f, L"*** Exception 0x%0X occured ***\n\n", exp->ExceptionRecord->ExceptionCode);

		while (pStackWalk64(imageType, process, thread, &frame, ctx, nullptr, pSymFunctionTableAccess64, pSymGetModuleBase64, nullptr)) {
			// get symbol name for address
			pSymbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
			pSymbol->MaxNameLen = MAX_SYM_NAME * sizeof(wchar_t);
			DWORD64 displacement = 0;
			pSymFromAddrW(process, frame.AddrPC.Offset, &displacement, pSymbol);

			// try to get line
			IMAGEHLP_LINEW64 line = { sizeof(IMAGEHLP_LINEW64) };
			DWORD disp = 0;
			if (pSymGetLineFromAddrW64(process, frame.AddrPC.Offset, &disp, &line)) {
				//fwprintf_s(f, L"%s(%lu) : %s(), address: 0x%0X\n", line.FileName, line.LineNumber, pSymbol->Name, pSymbol->Address);
				fwprintf_s(f, L"%s(%lu) : %s()\n", line.FileName, line.LineNumber, pSymbol->Name);
			} else {
				HMODULE hModule = nullptr;
				GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
								   (LPCTSTR)(frame.AddrPC.Offset), &hModule);
				if (hModule) {
					wchar_t module[MAX_PATH] = {};
					if (GetModuleFileNameW(hModule, module, MAX_PATH)) {
						//fwprintf_s(f, L"%s : %s(), address 0x%0X\n", module, pSymbol->Name, pSymbol->Address);
						fwprintf_s(f, L"%s : %s()\n", module, pSymbol->Name);
					}
				}
			}
		}

		fclose(f);
	}

	pSymCleanup(process);
}

CMiniDump _Singleton;

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

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
	return nullptr;
}

BOOL CMiniDump::PreventSetUnhandledExceptionFilter()
{
	BOOL bRet = FALSE;

	HMODULE hKernel32 = LoadLibraryW(L"kernel32.dll");
	if (hKernel32) {
		void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
		if (pOrgEntry) {
			unsigned char newJump[100];
			DWORD dwOrgEntryAddr = (DWORD)pOrgEntry;
			dwOrgEntryAddr += 5; // add 5 for 5 op-codes for jmp far
			void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
			DWORD dwNewEntryAddr = (DWORD)pNewFunc;
			DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;

			newJump[0] = 0xE9;  // JMP absolute
			memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
			SIZE_T bytesWritten;
			bRet = WriteProcessMemory(GetCurrentProcess(), pOrgEntry, newJump, sizeof(pNewFunc) + 1, &bytesWritten);
		}

		FreeLibrary(hKernel32);
	}

	return bRet;
}

LONG WINAPI CMiniDump::UnhandledExceptionFilter( _EXCEPTION_POINTERS *lpTopLevelExceptionFilter )
{
	LONG retval = EXCEPTION_CONTINUE_SEARCH;

	if (!m_bMiniDumpEnabled) {
		return retval;
	}

	WCHAR   szResult[800] = { 0 };
	CString strDumpPath;

	HMODULE hDll = ::LoadLibraryW(GetProgramDir() + L"dbghelp.dll");
	if (hDll == nullptr) {
		hDll = ::LoadLibraryW(L"dbghelp.dll");
	}

	if (hDll != nullptr) {
		MINIDUMPWRITEDUMP pMiniDumpWriteDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
		if (pMiniDumpWriteDump != nullptr && AfxGetMyApp()->GetAppSavePath(strDumpPath)) {
			// Check that the folder actually exists
			if (!::PathFileExistsW(strDumpPath)) {
				VERIFY(CreateDirectoryW(strDumpPath, nullptr));
			}

			strDumpPath.AppendFormat(L"%s.exe.%d.%d.%d.%d.dmp", AfxGetApp()->m_pszExeName, MPC_VERSION_SVN_NUM);

			HANDLE hFile = ::CreateFileW(strDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS,
										 FILE_ATTRIBUTE_NORMAL, nullptr);

			if (hFile != INVALID_HANDLE_VALUE) {
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				ExInfo.ThreadId = ::GetCurrentThreadId();
				ExInfo.ExceptionPointers = lpTopLevelExceptionFilter;
				ExInfo.ClientPointers = FALSE;

				BOOL bDumpCreated = pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, nullptr, nullptr);
				if (bDumpCreated) {
					swprintf_s(szResult, std::size(szResult), ResStr(IDS_MPC_CRASH), strDumpPath);
					retval = EXCEPTION_EXECUTE_HANDLER;
				} else {
					swprintf_s(szResult, std::size(szResult), ResStr(IDS_MPC_MINIDUMP_FAIL), strDumpPath, GetLastError());
				}

				::CloseHandle(hFile);

				CString strStackTraceDumpPath(strDumpPath);
				strStackTraceDumpPath.Replace(L".dmp", L".stacktrace.txt");
				DumpStackTrace(hDll, strStackTraceDumpPath, lpTopLevelExceptionFilter);
			} else {
				swprintf_s(szResult, std::size(szResult), ResStr(IDS_MPC_MINIDUMP_FAIL), strDumpPath, GetLastError());
			}
		}
		FreeLibrary(hDll);
	}

	if (szResult[0]) {
		switch (MessageBoxW(AfxGetApp()->GetMainWnd()->m_hWnd, szResult, L"MPC-BE Mini Dump", retval ? MB_YESNO : MB_OK)) {
			case IDYES:
				ShellExecuteW(nullptr, L"open", L"http://sourceforge.net/p/mpcbe/tickets/", nullptr, nullptr, SW_SHOWDEFAULT); // hmm
				ExploreToFile(strDumpPath);
				break;
			case IDNO:
				retval = EXCEPTION_CONTINUE_SEARCH; // rethrow the exception to make easier attaching a debugger
				break;
		}
	}

	return retval;
}
