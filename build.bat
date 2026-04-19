@echo off
rem ============================================================
rem  Eagle Library — Build Script for Visual Studio 2022
rem  Copyright (c) 2026 Eagle Software. All rights reserved.
rem ============================================================

setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%build-release"

if "%QT_DIR%"=="" set "QT_DIR=C:\Qt\6.7.3\msvc2022_64"
if "%CMAKE_DIR%"=="" set "CMAKE_DIR=C:\Program Files\CMake\bin"
if "%NINJA_DIR%"=="" set "NINJA_DIR=%CMAKE_DIR%"
if "%VS_DIR%"=="" set "VS_DIR=C:\Program Files\Microsoft Visual Studio\2022\Community"

echo.
echo  ╔══════════════════════════════════════════════╗
echo  ║   EAGLE LIBRARY 2.0  —  Build Script          ║
echo  ║     Eagle Software                           ║
echo  ╚══════════════════════════════════════════════╝
echo.

rem ── Auto-detect common Visual Studio 2022 locations ──────────
if not exist "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" (
    for %%D in (
        "C:\Program Files\Microsoft Visual Studio\2022\Community"
        "C:\Program Files\Microsoft Visual Studio\2022\Professional"
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools"
    ) do (
        if exist "%%~D\VC\Auxiliary\Build\vcvars64.bat" set "VS_DIR=%%~D"
    )
)

rem ── Auto-detect common Qt MSVC locations ─────────────────────
if not exist "%QT_DIR%\bin\qmake.exe" (
    for %%D in (
        "C:\Qt\6.7.3\msvc2022_64"
        "C:\Qt\6.7.2\msvc2022_64"
        "C:\Qt\6.7.1\msvc2022_64"
        "C:\Qt\6.7.0\msvc2022_64"
        "C:\Qt\6.6.3\msvc2022_64"
        "C:\Qt\6.6.2\msvc2022_64"
        "C:\Qt\6.6.1\msvc2022_64"
        "C:\Qt\6.6.0\msvc2022_64"
    ) do (
        if exist "%%~D\bin\qmake.exe" set "QT_DIR=%%~D"
    )
)

set "PATH=%QT_DIR%\bin;%CMAKE_DIR%;%NINJA_DIR%;%PATH%"

rem ── Find vcvars64 ─────────────────────────────────────────────
if exist "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" (
    call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
) else (
    echo [ERROR] Visual Studio 2022 not found at: %VS_DIR%
    echo         Edit VS_DIR in this script to point to your VS installation.
    pause & exit /b 1
)

rem ── Check Qt ──────────────────────────────────────────────────
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [ERROR] Qt not found at: %QT_DIR%
    echo         Edit QT_DIR in this script to your Qt 6 installation path.
    pause & exit /b 1
)

if not exist "%CMAKE_DIR%\cmake.exe" (
    echo [ERROR] CMake not found at: %CMAKE_DIR%
    echo         Install CMake or set CMAKE_DIR before running this script.
    pause & exit /b 1
)

rem ── Create build directory ────────────────────────────────────
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo [INFO] Visual Studio: %VS_DIR%
echo [INFO] Qt:            %QT_DIR%
echo [INFO] CMake:         %CMAKE_DIR%
echo [INFO] Build dir:     %BUILD_DIR%
echo.

echo [1/3] Configuring with CMake...
"%CMAKE_DIR%\cmake.exe" .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause & exit /b 1
)

echo.
echo [2/3] Building (Release)...
"%CMAKE_DIR%\cmake.exe" --build . --config Release --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    pause & exit /b 1
)

echo.
echo [3/3] Deploying Qt DLLs...
"%QT_DIR%\bin\windeployqt.exe" ^
    --no-translations ^
    --compiler-runtime ^
    "Release\EagleLibrary.exe"
if %ERRORLEVEL% neq 0 (
    echo [WARNING] windeployqt encountered issues.
)

if exist "%SCRIPT_DIR%plugins" (
    echo.
    echo [post] Copying starter plugins...
    if exist "Release\plugins" rmdir /s /q "Release\plugins"
    xcopy "%SCRIPT_DIR%plugins" "Release\plugins" /E /I /Y >nul
    if exist "%SCRIPT_DIR%installer\plugins" rmdir /s /q "%SCRIPT_DIR%installer\plugins"
    xcopy "%SCRIPT_DIR%plugins" "%SCRIPT_DIR%installer\plugins" /E /I /Y >nul
)

echo.
echo  ╔══════════════════════════════════════════════╗
echo  ║   BUILD COMPLETE                             ║
echo  ║   Output: build-release\Release\             ║
echo  ║   App: build-release\Release\EagleLibrary.exe║
echo  ║                                              ║
echo  ║   Portable refresh:                          ║
echo  ║   installer\refresh_portable.bat             ║
echo  ║   CHM help build:                            ║
echo  ║   build_help.bat                             ║
echo  ║   Installer script:                          ║
echo  ║   installer\eagle_library.nsi                ║
echo  ╚══════════════════════════════════════════════╝
echo.

cd /d "%SCRIPT_DIR%"
pause
