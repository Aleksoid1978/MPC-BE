@ECHO OFF
SETLOCAL
PUSHD %~dp0

SET GIT_REV_BRANCH=LOCAL
SET GIT_REV_HASH=0
SET GIT_REV_COUNT=0
SET GIT_REV_DATE=0

SET gitexe="git.exe"
%gitexe% --version
IF /I %ERRORLEVEL%==0 GOTO :GIT_OK

SET gitexe="c:\Program Files\Git\cmd\git.exe"
IF NOT EXIST %gitexe% SET gitexe="c:\Program Files\Git\bin\git.exe"
IF NOT EXIST %gitexe% GOTO :ÑHANGE_ÑHECK

:GIT_OK

FOR /F "delims=" %%A IN ('%gitexe% describe --long') DO (
  SET GIT_DESCRIBE_STR=%%A
)

IF NOT DEFINED GIT_DESCRIBE_STR GOTO :ÑHANGE_ÑHECK

FOR /F "tokens=2 delims=-" %%A IN ("%GIT_DESCRIBE_STR%") DO (
  SET GIT_REV_COUNT=%%A
)
FOR /F "tokens=3 delims=-" %%A IN ("%GIT_DESCRIBE_STR%") DO (
  SET GIT_REV_HASH=%%A
)
IF %GIT_REV_HASH:~0,1%==g SET GIT_REV_HASH=%GIT_REV_HASH:~1%

FOR /F "delims=" %%A IN ('%gitexe% symbolic-ref --short HEAD') DO (
  SET GIT_REV_BRANCH=%%A
)

FOR /F "delims=" %%A IN ('%gitexe% log -1 --date^=format:%%Y-%%m-%%d --pretty^=format:%%ad') DO (
  SET GIT_REV_DATE=%%A
)

:ÑHANGE_ÑHECK

SET SrcManifest="src\apps\mplayerc\res\mpc-be.exe.manifest.conf"
SET DstManifest="src\apps\mplayerc\res\mpc-be.exe.manifest"

IF NOT EXIST "revision.h" GOTO :UPDATE_REV
IF NOT EXIST %DstManifest% GOTO :UPDATE_REV

SET REVHASH="0"
SET REVNUM=0
FOR /F "tokens=3,4 delims= " %%A IN ('FINDSTR /I /L /C:"define REV_HASH" "revision.h"') DO SET REVHASH=%%A
FOR /F "tokens=3,4 delims= " %%A IN ('FINDSTR /I /L /C:"define REV_NUM" "revision.h"') DO SET REVNUM=%%A

IF NOT %REVHASH%=="%GIT_REV_HASH%" GOTO :UPDATE_REV
IF NOT %REVNUM%==%GIT_REV_COUNT% GOTO :UPDATE_REV

GOTO :DONT_UPDATE

:UPDATE_REV

ECHO #pragma once > revision.h
ECHO // %GIT_DESCRIBE_STR% >> revision.h
ECHO #define REV_DATE "%GIT_REV_DATE%" >> revision.h
ECHO #define REV_BRANCH "%GIT_REV_BRANCH%" >> revision.h
ECHO #define REV_HASH "%GIT_REV_HASH%" >> revision.h
ECHO #define REV_NUM %GIT_REV_COUNT% >> revision.h

IF EXIST %DstManifest% DEL /Q %DstManifest%
powershell -Command "(gc %SrcManifest%) -replace '_REV_NUM_', '%GIT_REV_COUNT%' | Out-File -encoding UTF8 %DstManifest%"

ECHO The revision number is %GIT_REV_COUNT%.

:END
POPD
ENDLOCAL
EXIT /B

:DONT_UPDATE
ECHO The revision number is up to date.
GOTO END