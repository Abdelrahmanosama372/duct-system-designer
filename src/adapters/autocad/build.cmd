@echo off
setlocal
if "%~1"=="" (
  if not defined ARX (
    echo Usage: build.cmd "C:\path\to\ObjectARX2027"
    echo Or set ARX to the SDK root.
    exit /b 2
  )
  set "DUCT_SDK=%ARX%"
) else (
  set "DUCT_SDK=%~1"
)
if not defined CMAKE_EXE set "CMAKE_EXE=cmake"
"%CMAKE_EXE%" -S "%~dp0" -B "%~dp0build" -G "Visual Studio 17 2022" -A x64 -DOBJECTARX_ROOT="%DUCT_SDK%"
if errorlevel 1 exit /b 1
"%CMAKE_EXE%" --build "%~dp0build" --config Release
exit /b %errorlevel%
