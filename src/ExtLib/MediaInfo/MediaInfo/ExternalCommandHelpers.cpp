/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/ExternalCommandHelpers.h"

#ifdef WINDOWS
#include <windows.h>
#include <process.h>
#include <wchar.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#endif //WINDOWS
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>

using namespace std;
using namespace ZenLib;

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
const ZtringList ffmpeg_PossibleNames= // Hack, ZtringList needs to handle C++11 style static initialization
{
    #ifdef WINDOWS
    __T("ffmpeg-ma.exe;ffmpeg.exe")
    #else
    __T("ffmpeg-ma;ffmpeg")
    #endif //WINDOWS
};

//---------------------------------------------------------------------------
Ztring External_Command_Exists(const ZtringList& PossibleNames)
{
    #ifndef WINDOWS_UWP
    Ztring Path;

    #ifdef WINDOWS
    Ztring ExeDirectory;

    DWORD EnvPathLen=GetEnvironmentVariableW(L"PATH", nullptr, 0);
    if (EnvPathLen)
    {
        wchar_t* EnvPath=new wchar_t[EnvPathLen+1];
        EnvPathLen=GetEnvironmentVariableW(L"PATH", EnvPath, EnvPathLen+1);

        if (EnvPathLen>0 && EnvPathLen<EnvPathLen+1)
            Path.From_Unicode(EnvPath);

        delete[] EnvPath;
    }

    wchar_t* ExePath=new wchar_t[MAX_PATH+1];
    DWORD ExePathLen=GetModuleFileNameW(NULL, ExePath, MAX_PATH+1);
    if (ExePathLen>0 && ExePathLen<MAX_PATH+1)
    {
        ExeDirectory=Ztring().From_Unicode(ExePath);
        size_t Pos=ExeDirectory.find_last_of(__T("\\"));
        if (Pos!=string::npos)
            ExeDirectory=ExeDirectory.substr(0, Pos);
    }
    delete[] ExePath;

    const Ztring EnvSep = __T(";");
    const Ztring PathSep = __T("\\");
    #else
    Path.From_Local(getenv("PATH"));
    const Ztring EnvSep=__T(":");
    const Ztring PathSep=__T("/");
    #endif //WINDOWS

    for (const Ztring& Command : PossibleNames)
    {
        #ifdef WINDOWS
        if (!ExeDirectory.empty()) // Search in program directory
        {
            Ztring FullPath=ExeDirectory+PathSep+Command;
            struct _stat64 Buf;
            if (_wstat64(FullPath.To_Unicode().c_str(), &Buf)==0 && Buf.st_mode&S_IFREG && Buf.st_mode&S_IEXEC)
                return FullPath;
        }
        #endif //WINDOWS

        size_t Start=0;
        while (Start<Path.size())
        {
            size_t End=Path.find(EnvSep, Start);
            if (End==string::npos)
                End=Path.size()-1;

            Ztring FullPath=Path.substr(Start, End-Start)+PathSep+Command;

            #ifdef WINDOWS
            struct _stat64 Buf;
            if (_wstat64(FullPath.To_Unicode().c_str(), &Buf)==0 && Buf.st_mode&S_IFREG && Buf.st_mode&S_IEXEC)
            #else
            struct stat Buf;
            if (stat(FullPath.To_Local().c_str(), &Buf)==0 && Buf.st_mode&S_IFREG && Buf.st_mode&S_IXUSR)
            #endif //WINDOWS
                return FullPath;

            Start=End+1;
        }
    }
    #endif //WINDOWS_UWP

    return Ztring();
}

//---------------------------------------------------------------------------
int External_Command_Run(const Ztring& Command, const ZtringList& Arguments, Ztring* StdOut, Ztring* StdErr)
{
    #ifdef WINDOWS
    DWORD ExitCode=-1;
    #else
    int ExitCode=-1;
    #endif

    #ifndef WINDOWS_UWP
    #ifdef WINDOWS
    ZtringList CommandLine(Command);
    CommandLine+=Arguments;
    CommandLine.Separator_Set(0, __T(" "));

    SECURITY_ATTRIBUTES Attrs;
    ZeroMemory(&Attrs, sizeof(SECURITY_ATTRIBUTES));
    Attrs.nLength=sizeof(SECURITY_ATTRIBUTES);
    Attrs.lpSecurityDescriptor=nullptr;
    Attrs.bInheritHandle=TRUE;

    HANDLE StdOutRead, StdOutWrite, StdErrRead, StdErrWrite;
    if (StdOut)
    {
        if (!CreatePipe(&StdOutRead, &StdOutWrite, &Attrs, 0))
            return -1;
    }
    else
    {
        StdOutRead=nullptr;
        StdOutWrite=nullptr;
    }

    if (StdErr)
    {
        if (!CreatePipe(&StdErrRead, &StdErrWrite, &Attrs, 0))
        {
            if (StdOutWrite) CloseHandle(StdOutWrite);
            if (StdOutRead)  CloseHandle(StdOutRead);
            return -1;
        }
    }
    else
    {
        StdErrRead=nullptr;
        StdErrWrite=nullptr;
    }

    STARTUPINFOW StartupInfo;
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb=sizeof(StartupInfo);
    StartupInfo.hStdError=StdErrWrite;
    StartupInfo.hStdOutput=StdOutWrite;
    StartupInfo.dwFlags|=STARTF_USESTDHANDLES;

    PROCESS_INFORMATION ProcessInfo;
    ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));

    if (!CreateProcessW(nullptr, (LPWSTR)CommandLine.Read().To_Unicode().c_str(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &StartupInfo, &ProcessInfo))
    {
        if (StdOutWrite) CloseHandle(StdOutWrite);
        if (StdOutRead)  CloseHandle(StdOutRead);
        if (StdErrWrite) CloseHandle(StdErrWrite);
        if (StdErrRead)  CloseHandle(StdErrRead);
        return -1;
    }

    if (StdOutWrite) CloseHandle(StdOutWrite);
    if (StdErrWrite) CloseHandle(StdErrWrite);

    char Buf[128];
    if (StdOut)
    {
        for (;;)
        {
            DWORD OutReadCount;
            if (!ReadFile(StdOutRead, Buf, sizeof(Buf), &OutReadCount, nullptr) || !OutReadCount)
                break;
            *StdOut+=Ztring().From_UTF8(Buf, OutReadCount);
        }
    }
    if (StdErr)
    {
        for (;;)
        {
            DWORD ErrReadCount;
            if (!ReadFile(StdErrRead, Buf, sizeof(Buf), &ErrReadCount, nullptr) || !ErrReadCount)
                break;
            *StdErr+=Ztring().From_UTF8(Buf, ErrReadCount);
        }
    }

    if (StdOutRead) CloseHandle(StdOutRead);
    if (StdErrRead) CloseHandle(StdErrRead);

    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);

    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
    #else
    int StdOutPipe[2];
    int StdErrPipe[2];

    if (pipe(StdOutPipe)!=0)
        return -1;

    if (pipe(StdErrPipe)!=0)
    {
        close(StdOutPipe[0]);
        close(StdOutPipe[1]);
        return -1;
    }

    pid_t PID=fork();
    if (PID==-1)
    {
        close(StdOutPipe[0]);
        close(StdOutPipe[1]);
        close(StdErrPipe[0]);
        close(StdErrPipe[1]);
        return -1;
    }
    else if (PID==0)
    {
        close(StdOutPipe[0]);
        close(StdErrPipe[0]);
        dup2(StdOutPipe[1], STDOUT_FILENO);
        dup2(StdErrPipe[1], STDERR_FILENO);

        close(StdOutPipe[1]);
        close(StdErrPipe[1]);

        size_t Count=Arguments.size()+1; // Add Command as argv[0]
        char** ArgumentsArr=new char*[Count+1]; // Add final nullptr
        for (size_t Pos=0; Pos<Count; Pos++)
        {
            string Argument=(Pos==0)?Command.To_Local():Arguments[Pos-1].To_Local();

            ArgumentsArr[Pos]=new char[Argument.size()+1];
            memcpy(ArgumentsArr[Pos], Argument.c_str(), Argument.size());
            ArgumentsArr[Pos][Argument.size()]=0;
        }
        ArgumentsArr[Count]=nullptr;

        execvp(Command.To_UTF8().c_str(), ArgumentsArr);

        for (size_t Pos=0; Pos<Count; Pos++)
            delete[] ArgumentsArr[Pos];
        delete[] ArgumentsArr;
        _exit(1);
    }
    else {
        close(StdOutPipe[1]);
        close(StdErrPipe[1]);

        char Buf[128];
        if (StdOut)
        {
            for (;;)
            {
                auto OutReadCount=read(StdOutPipe[0], Buf, sizeof(Buf));
                if (!OutReadCount)
                    break;
                *StdOut+=Ztring().From_UTF8(Buf, OutReadCount);
            }
        }
        if (StdErr)
        {
            for (;;)
            {
                auto ErrReadCount=read(StdErrPipe[0], Buf, sizeof(Buf));
                if (!ErrReadCount)
                    break;
                *StdErr+=Ztring().From_UTF8(Buf, ErrReadCount);
            }
        }

        close(StdOutPipe[0]);
        close(StdErrPipe[0]);

        waitpid(PID, &ExitCode, 0);
    }
    #endif //WINDOWS
    #endif //WINDOWS_UWP

    return ExitCode;
}

} //Namespace
