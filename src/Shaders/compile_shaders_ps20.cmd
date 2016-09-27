@echo off
echo === Compiling Pixel Shaders 2.0 ===
set LIST=%LIST% resizer_bspline4_x.hlsl
set LIST=%LIST% resizer_bspline4_y.hlsl
set LIST=%LIST% resizer_mitchell4_x.hlsl
set LIST=%LIST% resizer_mitchell4_y.hlsl
set LIST=%LIST% resizer_catmull4_x.hlsl
set LIST=%LIST% resizer_catmull4_y.hlsl
set LIST=%LIST% resizer_bicubic06_x.hlsl
set LIST=%LIST% resizer_bicubic06_y.hlsl
set LIST=%LIST% resizer_bicubic08_x.hlsl
set LIST=%LIST% resizer_bicubic08_y.hlsl
set LIST=%LIST% resizer_bicubic10_x.hlsl
set LIST=%LIST% resizer_bicubic10_y.hlsl
set LIST=%LIST% resizer_lanczos2_x.hlsl
set LIST=%LIST% resizer_lanczos2_y.hlsl
set LIST=%LIST% resizer_downscaling_x.hlsl
set LIST=%LIST% resizer_downscaling_y.hlsl
set LIST=%LIST% YCgCo_correction.hlsl
set LIST=%LIST% halfoverunder_to_interlace.hlsl

for %%f in (%LIST%) do (
  fxc.exe /nologo /T ps_2_0 /Fo ..\..\bin\shaders\ps20_%%~nf.cso %%f
)