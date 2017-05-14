@echo off

set fxc="fxc.exe"
IF NOT EXIST %fxc% set fxc="c:\Program Files (x86)\Windows Kits\8.1\bin\x64\fxc.exe"
IF NOT EXIST %fxc% set fxc="c:\Program Files (x86)\Windows Kits\8.1\bin\x86\fxc.exe"
IF NOT EXIST %fxc% set fxc="C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Utilities\bin\x86\fxc.exe"

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
