@echo off
echo === Compiling Pixel Shaders 2.0 ===
set LIST=%LIST% resizer_smootherstep.hlsl
set LIST=%LIST% resizer_bspline4.hlsl
set LIST=%LIST% resizer_mitchell4.hlsl
set LIST=%LIST% resizer_bicubic06.hlsl
set LIST=%LIST% resizer_bicubic08.hlsl
set LIST=%LIST% resizer_bicubic10.hlsl

for %%f in (%LIST%) do (
  fxc.exe /nologo /T ps_2_0 /Fo ..\..\..\bin\shaders\ps20_%%~nf.cso %%f
)