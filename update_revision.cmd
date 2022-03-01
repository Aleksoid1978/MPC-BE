REM Experimental!
@ECHO OFF
SETLOCAL
PUSHD %~dp0

SET gitexe="git.exe"
%gitexe% --version
IF /I %ERRORLEVEL%==0 GOTO :GitOK

SET gitexe="c:\Program Files\Git\cmd\git.exe"
IF NOT EXIST %gitexe% SET gitexe="c:\Program Files\Git\bin\git.exe"
IF NOT EXIST %gitexe% GOTO :SubError

:GitOK

FOR /F "delims=" %%A IN ('%gitexe% describe --long') DO (
  SET GIT_DESCRIBE_STR=%%A
)

IF NOT DEFINED GIT_DESCRIBE_STR GOTO :SubError

FOR /F "tokens=2 delims=-" %%A IN ("%GIT_DESCRIBE_STR%") DO (
  SET GIT_REV_COUNT=%%A
)
FOR /F "tokens=3 delims=-" %%A IN ("%GIT_DESCRIBE_STR%") DO (
  SET GIT_REV_HASH="%%A"
)

SET SrcManifest="src\apps\mplayerc\res\mpc-be.exe.manifest.conf"
SET DstManifest="src\apps\mplayerc\res\mpc-be.exe.manifest"

IF NOT EXIST "revision.h" GOTO :UPDATE_REV
IF NOT EXIST %DstManifest% GOTO :UPDATE_REV

SET REVHASH=0
SET REVNUM=0
FOR /F "tokens=3,4 delims= " %%A IN ('FINDSTR /I /L /C:"define REV_HASH" "revision.h"') DO SET "REVHASH=%%A"
FOR /F "tokens=3,4 delims= " %%A IN ('FINDSTR /I /L /C:"define REV_NUM" "revision.h"') DO SET "REVNUM=%%A"

IF NOT %REVHASH%==%GIT_REV_HASH% GOTO :UPDATE_REV
IF NOT %REVNUM%==%GIT_REV_COUNT% GOTO :UPDATE_REV

GOTO :DoNotUpdate

:UPDATE_REV

ECHO #pragma once > revision.h
ECHO // %GIT_DESCRIBE_STR% >> revision.h

%gitexe% log -1 --date=format:%%Y.%%m.%%d --pretty=format:"#define REV_DATE %%x22%%ad%%x22%%n" >> revision.h

SET GIT_REV_BRANCH=LOCAL
FOR /F "delims=" %%A IN ('%gitexe% symbolic-ref --short HEAD') DO SET GIT_REV_BRANCH=%%A
ECHO #define REV_BRANCH "%GIT_REV_BRANCH%" >> revision.h

ECHO #define REV_HASH %GIT_REV_HASH% >> revision.h
ECHO #define REV_NUM %GIT_REV_COUNT% >> revision.h

IF EXIST %dstfile% DEL /Q %DstManifest%
powershell -Command "(gc %SrcManifest%) -replace '_REV_NUM_', '%GIT_REV_COUNT%' | Out-File -encoding UTF8 %DstManifest%"

ECHO The revision number is %GIT_REV_COUNT%.

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
