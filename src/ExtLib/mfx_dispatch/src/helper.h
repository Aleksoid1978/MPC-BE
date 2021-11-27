#pragma once

#include <Windows.h>

typedef BOOL(WINAPI* PFNSetThreadErrorMode)(
    _In_ DWORD dwNewMode,
    _In_opt_ LPDWORD lpOldMode);

inline PFNSetThreadErrorMode LoadSetThreadErrorModeFunction()
{
    static auto hKernel32 = LoadLibraryW(L"Kernel32.dll");
    if (hKernel32) {
        static auto pSetThreadErrorMode = reinterpret_cast<PFNSetThreadErrorMode>(GetProcAddress(hKernel32, "SetThreadErrorMode"));
        if (pSetThreadErrorMode) {
            return pSetThreadErrorMode;
        }
    }

    return nullptr;
}
