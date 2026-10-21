@echo off
call "D:\Visual Studio\Visual Studio 2026\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Failed to setup MSVC environment
    exit /b 1
)

cd /d "%~dp0"

echo Running qmake...
"D:\Qt\6.11.1\msvc2022_64\bin\qmake.exe" attendance.pro
if %ERRORLEVEL% NEQ 0 (
    echo qmake failed
    exit /b 1
)

echo Running nmake...
nmake -f Makefile.Debug
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b 1
)

echo Build succeeded!
