@ECHO OFF
SETLOCAL

IF "%~1" == "" (
  ECHO %~nx0: No input file specified!
  EXIT /B 1
)

IF NOT EXIST "%~dp0signinfo.txt" (
  ECHO %~nx0: %~dp0signinfo.txt is not present!
  SET SIGN_ERROR=True
  GOTO END
)

IF DEFINED VS140COMNTOOLS (
  SET "VSCOMNTOOLS=%VS140COMNTOOLS%"
) ELSE IF DEFINED VS120COMNTOOLS (
  SET "VSCOMNTOOLS=%VS120COMNTOOLS%"
) ELSE (
  ECHO %~nx0: Visual Studio does not seem to be installed...
  EXIT /B 1
)

CALL "%VSCOMNTOOLS%..\..\VC\vcvarsall.bat" x86 > nul

SET SIGN_CMD=
SET /P SIGN_CMD=<%~dp0signinfo.txt

TITLE Signing "%~1"...
ECHO. & ECHO Signing "%~1"...

FOR /L %%i IN (1,1,5) DO (
  IF %%i GTR 1 ECHO %%i attempt
  signtool.exe sign %SIGN_CMD% "%~1"
  IF NOT ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
)

EXIT /B %ERRORLEVEL%
