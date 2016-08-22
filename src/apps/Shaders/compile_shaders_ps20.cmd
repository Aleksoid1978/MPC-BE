@echo off
for %%f in (*.hlsl) do (
  if %%f NEQ resizer_downscaling.hlsl (
    fxc.exe /nologo /T ps_2_0 /Fo ..\..\..\bin\shaders\%%~nf_ps20.cso %%f
  )
)