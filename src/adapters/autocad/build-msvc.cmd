@echo off
setlocal
set "DUCT_APPLICATION_ROOT=%~dp0..\..\application"
if "%~1"=="" (
  if not defined ARX (
    echo Usage: build-msvc.cmd "C:\path\to\ObjectARX2027"
    exit /b 2
  )
  set "DUCT_SDK=%ARX%"
) else (
  set "DUCT_SDK=%~1"
)
where cl >nul 2>&1
if errorlevel 1 (
  echo Run this script from an x64 Native Tools Command Prompt for VS 2022.
  exit /b 2
)
if not exist "%DUCT_SDK%\inc\AcRxDefault.def" (
  echo Invalid ObjectARX SDK directory: "%DUCT_SDK%"
  exit /b 2
)
if not exist "%~dp0build-msvc" mkdir "%~dp0build-msvc"
pushd "%~dp0build-msvc"
> sources.rsp (
  for /r "%~dp0src" %%F in (*.cpp) do echo "%%F"
  for /r "%DUCT_APPLICATION_ROOT%\src" %%F in (*.cpp) do echo "%%F"
)
cl /nologo /c /MD /EHsc /std:c++20 /utf-8 /D_AFXDLL /D_AFXEXT /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS /D_WIN32_WINNT=0x0A00 /D_WIN32_IE=0x0601 /I"%~dp0src" /I"%~dp0include" /I"%DUCT_APPLICATION_ROOT%\include" /I"%DUCT_SDK%\inc" /I"%DUCT_SDK%\inc-x64" @sources.rsp > compile.log 2>&1
if errorlevel 1 (
  type compile.log
  popd
  exit /b 1
)
link /nologo /DLL /MACHINE:X64 /INCREMENTAL:NO /OUT:TestARX15.arx /DEF:"%DUCT_SDK%\inc\AcRxDefault.def" /LIBPATH:"%DUCT_SDK%\lib-x64" *.obj rxapi.lib acdb26.lib accore.lib acad.lib adui26.lib acui26.lib gdiplus.lib uxtheme.lib comctl32.lib > link.log 2>&1
if errorlevel 1 (
  type link.log
  popd
  exit /b 1
)
xcopy "%~dp0assets\fonts\*" "fonts\" /I /Y /Q >nul
if errorlevel 1 (
  popd
  exit /b 1
)
echo Built %~dp0build-msvc\TestARX15.arx
popd
exit /b 0
