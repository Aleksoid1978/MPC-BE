@ECHO OFF
REM (C) 2009-2019 see Authors.txt
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

IF EXIST "environments.bat" CALL "environments.bat"

IF DEFINED MPCBE_MINGW GOTO VarOk
ECHO ERROR: Please define MPCBE_MINGW environment variable
ENDLOCAL
EXIT /B

:VarOk

FOR /f "tokens=1,2 delims=" %%K IN (
  '%MPCBE_MINGW%\bin\gcc -dumpversion'
) DO (
  SET "gccver=%%K" & Call :SubGCCVer %%gccver:*Z=%%
)

IF EXIST "%MPCBE_MINGW%\mpcbe_libs\lib\libmingwex.a" (
  COPY /V /Y "%MPCBE_MINGW%\mpcbe_libs\lib\libmingwex.a" "lib\"
) ELSE (
  COPY /V /Y "%MPCBE_MINGW%\i686-w64-mingw32\lib\libmingwex.a" "lib\"
)
COPY /V /Y "%MPCBE_MINGW%\lib\gcc\i686-w64-mingw32\%gccver%\libgcc.a" "lib\"

IF EXIST "%MPCBE_MINGW%\mpcbe_libs\lib64\libmingwex.a" (
  COPY /V /Y "%MPCBE_MINGW%\mpcbe_libs\lib64\libmingwex.a" "lib64\"
) ELSE (
  COPY /V /Y "%MPCBE_MINGW%\x86_64-w64-mingw32\lib\libmingwex.a" "lib64\"
)
COPY /V /Y "%MPCBE_MINGW%\lib\gcc\x86_64-w64-mingw32\%gccver%\libgcc.a" "lib64\"

PAUSE
EXIT /B

:SubGCCVer
SET gccver=%*
@ECHO gccver=%*
EXIT /B
