@ECHO OFF
SETLOCAL

IF "%~1" == "" (
  ECHO %~nx0: No input file specified!
  GOTO END
)

IF NOT EXIST "%~dp0signinfo.txt" (
  ECHO %~nx0: %~dp0signinfo.txt is not present!
  GOTO END
)

IF DEFINED VS140COMNTOOLS (
  SET "VSCOMNTOOLS=%VS140COMNTOOLS%"
) ELSE IF DEFINED VS120COMNTOOLS (
  SET "VSCOMNTOOLS=%VS120COMNTOOLS%"
) ELSE (
  ECHO %~nx0: Visual Studio does not seem to be installed...
  GOTO END
)

CALL "%VSCOMNTOOLS%..\..\VC\vcvarsall.bat" x86 > nul

TITLE Signing "%~1"...
ECHO. & ECHO Signing "%~1"...

FOR /F "delims=" %%A IN (%~dp0signinfo.txt) DO (SET "SIGN_CMD=%%A" && CALL :SIGN %1)

:END
ENDLOCAL
EXIT /B %ERRORLEVEL%

:SIGN
FOR /L %%i IN (1,1,5) DO (
  IF %%i GTR 1 ECHO %%i attempt
  signtool.exe sign %SIGN_CMD% "%~1"
  IF %ERRORLEVEL% EQU 0 EXIT /B %ERRORLEVEL%
)
