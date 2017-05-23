@echo off

set fxc="fxc.exe"
IF NOT EXIST %fxc% set fxc="c:\Program Files (x86)\Windows Kits\8.1\bin\x64\fxc.exe"
IF NOT EXIST %fxc% set fxc="c:\Program Files (x86)\Windows Kits\8.1\bin\x86\fxc.exe"
IF NOT EXIST %fxc% set fxc="C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Utilities\bin\x86\fxc.exe"

echo === Compiling downscaler shaders ===
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_box_x.cso" "Resizers\downscaler_box.hlsl" /DAXIS=0
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_box_y.cso" "Resizers\downscaler_box.hlsl" /DAXIS=1
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_bilinear_x.cso" "Resizers\downscaler.hlsl" /DFILTER=1 /DAXIS=0
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_bilinear_y.cso" "Resizers\downscaler.hlsl" /DFILTER=1 /DAXIS=1
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_hamming_x.cso" "Resizers\downscaler.hlsl" /DFILTER=2 /DAXIS=0
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_hamming_y.cso" "Resizers\downscaler.hlsl" /DFILTER=2 /DAXIS=1
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_bicubic_x.cso" "Resizers\downscaler.hlsl" /DFILTER=3 /DAXIS=0
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_bicubic_y.cso" "Resizers\downscaler.hlsl" /DFILTER=3 /DAXIS=1
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_lanczos_x.cso" "Resizers\downscaler.hlsl" /DFILTER=4 /DAXIS=0
%fxc% /nologo /T ps_3_0 /Fo "..\..\bin\shaders\downscaler_lanczos_y.cso" "Resizers\downscaler.hlsl" /DFILTER=4 /DAXIS=1

echo === Compiling Pixel Shaders 2.0 ===
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
set LIST=%LIST% Resizers\downscaler_simple_x.hlsl
set LIST=%LIST% Resizers\downscaler_simple_y.hlsl
set LIST=%LIST% Transformation\ycgco_correction.hlsl
set LIST=%LIST% Transformation\st2084_correction.hlsl
set LIST=%LIST% Transformation\hlg_correction.hlsl
set LIST=%LIST% Transformation\halfoverunder_to_interlace.hlsl

for %%f in (%LIST%) do (
  %fxc% /nologo /T ps_2_0 /Fo ..\..\bin\shaders\ps20_%%~nf.cso %%f
)

%fxc% /nologo /T ps_2_0 /Fo "..\..\bin\shaders\ps20_downscaler_box_x.cso" "Resizers\downscaler_box.hlsl" /DPS20 /DAXIS=0
%fxc% /nologo /T ps_2_0 /Fo "..\..\bin\shaders\ps20_downscaler_box_y.cso" "Resizers\downscaler_box.hlsl" /DPS20 /DAXIS=1
