@ECHO OFF
REM $Id$
REM
REM (C) 2009-2014 see Authors.txt
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

FOR /f "tokens=1,2 delims=" %%K IN (
  '%MINGW32%\bin\gcc -dumpversion'
) DO (
  SET "gccver=%%K" & Call :SubGCCVer %%gccver:*Z=%%
)

COPY /V /Y "%MINGW32%\i686-w64-mingw32\lib\libmingwex.a" "lib\"
COPY /V /Y "%MINGW32%\lib\gcc\i686-w64-mingw32\%gccver%\libgcc.a" "lib\"

COPY /V /Y "%MINGW32%\x86_64-w64-mingw32\lib\libmingwex.a" "lib64\"
COPY /V /Y "%MINGW32%\lib\gcc\x86_64-w64-mingw32\%gccver%\libgcc.a" "lib64\"

PAUSE
EXIT /B

:SubGCCVer
SET gccver=%*
@ECHO gccver=%*
EXIT /B
