#pragma once

#include <Windows.h>

typedef BOOL(WINAPI* PFNSetThreadErrorMode)(DWORD, LPDWORD);

inline PFNSetThreadErrorMode LoadSetThreadErrorModeFunction()
{
    static auto pSetThreadErrorMode = reinterpret_cast<PFNSetThreadErrorMode>(GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "SetThreadErrorMode"));
    return pSetThreadErrorMode;
}
