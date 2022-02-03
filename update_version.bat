@ECHO OFF
REM (C) 2009-2022 see Authors.txt
REM
REM This file is part of MPC-BE.
REM
REM MPC-BE is free software; you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation; either version 3 of the License, or
REM (at your option) any later version.
REM
REM MPC-BE is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with this program.  If not, see <http://www.gnu.org/licenses/>.

SETLOCAL
PUSHD %~dp0

SET gitexe="git.exe"
IF NOT EXIST %gitexe% set gitexe="c:\Program Files\Git\bin\git.exe"
IF NOT EXIST %gitexe% set gitexe="c:\Program Files\Git\cmd\git.exe"

IF NOT EXIST %gitexe% GOTO :SubError

SET REVHASH=0
SET REVNUM=0

SET GIT_REV_HASH=0
SET GIT_REV_COUNT=0

SET SrcManifest="src\apps\mplayerc\res\mpc-be.exe.manifest.conf"
SET DstManifest="src\apps\mplayerc\res\mpc-be.exe.manifest"

FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define REV_HASH" "revision.h"') DO (SET "REVHASH=%%A")
FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define REV_NUM" "revision.h"') DO (SET "REVNUM=%%A")

FOR /f "delims=" %%A IN ('%gitexe% rev-parse --short HEAD') DO (
  SET GIT_REV_HASH="%%A"
)
FOR /f "delims=" %%A IN ('%gitexe% rev-list --count HEAD') DO (
  SET /A GIT_REV_COUNT=%%A + 73
)

IF %REVHASH%==%GIT_REV_HASH% (
  IF %REVNUM%==%GIT_REV_COUNT% (
    IF EXIST %DstManifest% GOTO :DoNotUpdate
  )
)

ECHO #pragma once > revision.h

%gitexe% log -1 --date=format:%%Y-%%m-%%d --pretty=format:"#define REV_DATE %%ad%%n" >> revision.h

SET GIT_REV_BRANCH=LOCAL
FOR /f "delims=" %%A IN ('%gitexe% symbolic-ref --short HEAD') DO SET GIT_REV_BRANCH=%%A
ECHO #define REV_BRANCH "%GIT_REV_BRANCH%" >> revision.h

ECHO #define REV_HASH %GIT_REV_HASH% >> revision.h
ECHO #define REV_NUM %GIT_REV_COUNT% >> revision.h

IF EXIST %dstfile% DEL /Q %DstManifest%
powershell -Command "(gc %SrcManifest%) -replace '_REV_NUM_', '%GIT_REV_COUNT%' | Out-File -encoding UTF8 %DstManifest%"

:END
POPD
ENDLOCAL
EXIT /B

:DoNotUpdate
ECHO The revision number is up to date.
GOTO END

:SubError
ECHO Something went wrong when generating the revision number.
ECHO #pragma once > revision.h
GOTO END
