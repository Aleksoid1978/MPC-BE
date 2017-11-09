@ECHO OFF
REM (C) 2009-2017 see Authors.txt
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

IF /I "%~1"=="help"   GOTO SHOWHELP
IF /I "%~1"=="/help"  GOTO SHOWHELP
IF /I "%~1"=="-help"  GOTO SHOWHELP
IF /I "%~1"=="--help" GOTO SHOWHELP
IF /I "%~1"=="/?"     GOTO SHOWHELP

IF EXIST "%~dp0..\..\..\environments.bat" CALL "%~dp0..\..\..\environments.bat"

IF DEFINED MPCBE_MINGW IF DEFINED MPCBE_MSYS GOTO VarOk
ECHO ERROR: Please define MPCBE_MINGW and MPCBE_MSYS environment variable(s)
EXIT /B

:VarOk
SET PATH=%MPCBE_MSYS%\bin;%MPCBE_MINGW%\bin;%PATH%

SET "BUILDTYPE=build"

SET ARG=%*
SET ARG=%ARG:/=%
SET ARG=%ARG:-=%

FOR %%A IN (%ARG%) DO (
	IF /I "%%A" == "clean" SET "BUILDTYPE=clean"
	IF /I "%%A" == "rebuild" SET "BUILDTYPE=rebuild"
	IF /I "%%A" == "64" SET "BIT=64BIT=yes"
	IF /I "%%A" == "Debug" SET "DEBUG=DEBUG=yes"
)

IF /I "%BUILDTYPE%" == "rebuild" (
  SET "BUILDTYPE=clean"
  CALL :SubMake clean
  SET "BUILDTYPE=build"
  CALL :SubMake
  EXIT /B
) ELSE (
  CALL :SubMake
  EXIT /B
)

:SubMake
IF "%BUILDTYPE%" == "clean" (
  SET JOBS=1
) ELSE (
  SET "BUILDTYPE="
  IF DEFINED NUMBER_OF_PROCESSORS (
    SET JOBS=%NUMBER_OF_PROCESSORS%
  ) ELSE (
    SET JOBS=4
  )
)

make.exe -f ffmpeg.mak %BUILDTYPE% -j%JOBS% %BIT% %DEBUG%

ENDLOCAL
EXIT /B

:SHOWHELP
TITLE "%~nx0 %1"
ECHO. & ECHO.
ECHO Usage:   %~nx0 [32^|64] [Clean^|Build^|Rebuild] [Debug]
ECHO.
ECHO Notes:   The arguments are not case sensitive.
ECHO. & ECHO.
ECHO Executing "%~nx0" will use the defaults: "%~nx0"
ECHO.
EXIT /B
