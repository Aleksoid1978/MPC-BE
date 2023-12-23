@ECHO OFF
SETLOCAL
CD /D %~dp0

SET gitexe="git.exe"
%gitexe% --version
IF /I %ERRORLEVEL%==0 GOTO :GitOK

SET gitexe="c:\Program Files\Git\cmd\git.exe"
IF NOT EXIST %gitexe% set gitexe="c:\Program Files\Git\bin\git.exe"
IF NOT EXIST %gitexe% (
  ECHO ERROR: Git not found!
  GOTO :END
)

:GitOK

%gitexe% submodule update --init --recursive --progress
IF %ERRORLEVEL%==0 (
   ECHO Submodule update completed successfully.
) ELSE (
   ECHO ERROR: %errorlevel%!
)
:END
TIMEOUT /T 3
ENDLOCAL
EXIT /B
