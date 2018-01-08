@ECHO OFF
REM (C) 2015-2018 see Authors.txt
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

IF "%~1" == "" (
  ECHO %~nx0: No input file specified!
  GOTO END
)

IF NOT EXIST "%~dp0signinfo.txt" (
  ECHO %~nx0: %~dp0signinfo.txt is not present!
  GOTO END
)

IF NOT DEFINED VS150COMNTOOLS (
  FOR /F "tokens=2*" %%A IN (
    'REG QUERY "HKLM\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "15.0" 2^>NUL ^| FIND "REG_SZ" ^|^|
     REG QUERY "HKLM\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\SxS\VS7" /v "15.0" 2^>NUL ^| FIND "REG_SZ"') DO SET "VS150COMNTOOLS=%%BCommon7\Tools\"
)

IF DEFINED VS150COMNTOOLS (
  SET "VSNAME=Visual Studio 2017"
  SET "VCVARS=%VS150COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat"
) ELSE (
  ECHO ERROR: "Visual Studio environment variable(s) is missing - possible it's not installed on your PC"
  GOTO END
)

CALL "%VCVARS%" x86 > nul

TITLE Signing "%*"...
ECHO. & ECHO Signing "%*"...

FOR /F "delims=" %%A IN (%~dp0signinfo.txt) DO (SET "SIGN_CMD=%%A" && CALL :SIGN %*)

:END
ENDLOCAL
EXIT /B %ERRORLEVEL%

:SIGN
FOR /L %%i IN (1,1,5) DO (
  IF %%i GTR 1 ECHO %%i attempt
  signtool.exe sign %SIGN_CMD% %*
  IF %ERRORLEVEL% EQU 0 EXIT /B %ERRORLEVEL%
)
