@echo off
setlocal
set "DUCT_APPLICATION_ROOT=%~dp0..\..\..\application"
where cl >nul 2>&1
if errorlevel 1 (
  echo Run from an x64 Native Tools Command Prompt for VS 2022.
  exit /b 2
)
if not exist "%~dp0..\build-msvc\checks" mkdir "%~dp0..\build-msvc\checks"
pushd "%~dp0..\build-msvc\checks"
> sources.rsp (
  echo "%~dp0NativeUiChecks.cpp"
  for /r "%DUCT_APPLICATION_ROOT%\src" %%F in (*.cpp) do echo "%%F"
  for /r "%~dp0..\src\ui\controls" %%F in (*.cpp) do echo "%%F"
  for /r "%~dp0..\src\ui\layout" %%F in (*.cpp) do echo "%%F"
  for /r "%~dp0..\src\ui\model" %%F in (*.cpp) do echo "%%F"
  for /r "%~dp0..\src\ui\panels" %%F in (*.cpp) do echo "%%F"
)
cl /nologo /MD /EHsc /std:c++20 /utf-8 /DDUCT_UI_NATIVE_TESTS /D_AFXDLL /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /D_WIN32_IE=0x0601 /I"%~dp0..\include" /I"%DUCT_APPLICATION_ROOT%\include" @sources.rsp /Fe:DuctUiNativeChecks.exe /link /SUBSYSTEM:WINDOWS comctl32.lib gdiplus.lib uxtheme.lib > build.log 2>&1
if errorlevel 1 (
  type build.log
  popd
  exit /b 1
)
xcopy "%~dp0..\assets\fonts\*" "fonts\" /I /Y /Q >nul
if errorlevel 1 (
  popd
  exit /b 1
)
DuctUiNativeChecks.exe
set "DUCT_CHECK_RESULT=%errorlevel%"
type qa.log
popd
exit /b %DUCT_CHECK_RESULT%
