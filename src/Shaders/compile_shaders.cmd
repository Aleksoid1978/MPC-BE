@ECHO OFF
REM (C) 2017 see Authors.txt
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

PUSHD %~dp0

where /q fxc.exe
IF %ERRORLEVEL% EQU 0 goto fxc_Ok

IF NOT DEFINED VS150COMNTOOLS (
  FOR /F "tokens=2*" %%A IN (
    'REG QUERY "HKLM\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "15.0" 2^>NUL ^| FIND "REG_SZ" ^|^|
     REG QUERY "HKLM\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\SxS\VS7" /v "15.0" 2^>NUL ^| FIND "REG_SZ"') DO SET "VS150COMNTOOLS=%%BCommon7\Tools\"
)

IF DEFINED VS150COMNTOOLS (
  SET "VCVARS=%VS150COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat"
)

IF NOT DEFINED VCVARS (
  IF DEFINED VS140COMNTOOLS (
    SET "VCVARS=%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
  )
)

IF NOT DEFINED VCVARS (
  ECHO ERROR: "Visual Studio environment variable(s) is missing - possible it's not installed on your PC"
  EXIT /B
)

CALL "%VCVARS%" x86 > nul

:fxc_Ok

CALL :SubColorText "0A" "=== Compiling downscaler shaders ===" & ECHO.
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_simple_x.cso" "Resizers\downscaler_simple.hlsl" /DAXIS=0
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_simple_y.cso" "Resizers\downscaler_simple.hlsl" /DAXIS=1
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_box_x.cso" "Resizers\downscaler_box.hlsl" /DAXIS=0
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_box_y.cso" "Resizers\downscaler_box.hlsl" /DAXIS=1
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_bilinear_x.cso" "Resizers\downscaler.hlsl" /DFILTER=1 /DAXIS=0
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_bilinear_y.cso" "Resizers\downscaler.hlsl" /DFILTER=1 /DAXIS=1
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_hamming_x.cso" "Resizers\downscaler.hlsl" /DFILTER=2 /DAXIS=0
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_hamming_y.cso" "Resizers\downscaler.hlsl" /DFILTER=2 /DAXIS=1
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_bicubic_x.cso" "Resizers\downscaler.hlsl" /DFILTER=3 /DAXIS=0
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_bicubic_y.cso" "Resizers\downscaler.hlsl" /DFILTER=3 /DAXIS=1
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_lanczos_x.cso" "Resizers\downscaler.hlsl" /DFILTER=4 /DAXIS=0
fxc /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_lanczos_y.cso" "Resizers\downscaler.hlsl" /DFILTER=4 /DAXIS=1

CALL :SubColorText "0A" "=== Compiling Pixel Shaders 2.0 ===" & ECHO.
set LIST=%LIST% Resizers\resizer_bspline4_x.hlsl
set LIST=%LIST% Resizers\resizer_bspline4_y.hlsl
set LIST=%LIST% Resizers\resizer_mitchell4_x.hlsl
set LIST=%LIST% Resizers\resizer_mitchell4_y.hlsl
set LIST=%LIST% Resizers\resizer_catmull4_x.hlsl
set LIST=%LIST% Resizers\resizer_catmull4_y.hlsl
set LIST=%LIST% Resizers\resizer_bicubic06_x.hlsl
set LIST=%LIST% Resizers\resizer_bicubic06_y.hlsl
set LIST=%LIST% Resizers\resizer_bicubic08_x.hlsl
set LIST=%LIST% Resizers\resizer_bicubic08_y.hlsl
set LIST=%LIST% Resizers\resizer_bicubic10_x.hlsl
set LIST=%LIST% Resizers\resizer_bicubic10_y.hlsl
set LIST=%LIST% Resizers\resizer_lanczos2_x.hlsl
set LIST=%LIST% Resizers\resizer_lanczos2_y.hlsl
set LIST=%LIST% Transformation\ycgco_correction.hlsl
set LIST=%LIST% Transformation\st2084_correction.hlsl
set LIST=%LIST% Transformation\hlg_correction.hlsl
set LIST=%LIST% Transformation\chroma_422_correction.hlsl
set LIST=%LIST% Transformation\halfoverunder_to_interlace.hlsl

for %%f in (%LIST%) do (
  fxc /nologo /T ps_2_0 /Fo ..\..\bin\shaders\ps20_%%~nf.cso %%f
)

fxc /nologo /T ps_2_0 /Fo "..\..\bin\shaders\ps20_downscaler_simple_x.cso" "Resizers\downscaler_simple.hlsl" /DPS20 /DAXIS=0
fxc /nologo /T ps_2_0 /Fo "..\..\bin\shaders\ps20_downscaler_simple_y.cso" "Resizers\downscaler_simple.hlsl" /DPS20 /DAXIS=1
fxc /nologo /T ps_2_0 /Fo "..\..\bin\shaders\ps20_downscaler_box_x.cso" "Resizers\downscaler_box.hlsl" /DPS20 /DAXIS=0
fxc /nologo /T ps_2_0 /Fo "..\..\bin\shaders\ps20_downscaler_box_y.cso" "Resizers\downscaler_box.hlsl" /DPS20 /DAXIS=1

EXIT /B

:SubColorText
FOR /F "tokens=1,2 delims=#" %%A IN (
  '"PROMPT #$H#$E# & ECHO ON & FOR %%B IN (1) DO REM"') DO (
  SET "DEL=%%A")
<NUL SET /p ".=%DEL%" > "%~2"
FINDSTR /v /a:%1 /R ".18" "%~2" NUL
DEL "%~2" > NUL 2>&1
EXIT /B
