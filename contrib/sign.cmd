@ECHO OFF
REM (C) 2015-2019 see Authors.txt
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

IF NOT DEFINED VCVARS (
  FOR /f "delims=" %%A IN ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -prerelease -property installationPath -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.ATLMFC Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -latest') DO SET "VCVARS=%%A\Common7\Tools\vsdevcmd.bat"
)

IF NOT EXIST "%VCVARS%" (
  ECHO ERROR: "Visual Studio environment variable(s) is missing - possible it's not installed on your PC"
  GOTO END
)

CALL "%VCVARS%" -no_logo -arch=x86

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
