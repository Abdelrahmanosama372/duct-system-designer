@echo off
setlocal
rem The integrated application now depends on dsd_core. Use the CMake build path.
call "%~dp0build.cmd" %*
exit /b %errorlevel%
