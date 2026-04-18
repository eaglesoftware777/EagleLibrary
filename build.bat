@echo off
rem ============================================================
rem  Eagle Library — Build Script for Visual Studio 2022
rem  Copyright (c) 2024 Eagle Software. All rights reserved.
rem ============================================================

setlocal EnableDelayedExpansion

echo.
echo  ╔══════════════════════════════════════════════╗
echo  ║     EAGLE LIBRARY  —  Build Script           ║
echo  ║     Eagle Software                           ║
echo  ╚══════════════════════════════════════════════╝
echo.

rem ── Adjust these paths to match your environment ─────────────
set QT_DIR=C:\Qt\6.7.0\msvc2022_64
set CMAKE_DIR=C:\Program Files\CMake\bin
set VS_DIR=C:\Program Files\Microsoft Visual Studio\2022\Community

rem ── You shouldn't need to edit below this line ───────────────
set PATH=%QT_DIR%\bin;%CMAKE_DIR%;%PATH%

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

rem ── Create build directory ────────────────────────────────────
set BUILD_DIR=%~dp0build-release
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo [1/3] Configuring with CMake...
cmake .. ^
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
cmake --build . --config Release --parallel
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

echo.
echo  ╔══════════════════════════════════════════════╗
echo  ║   BUILD COMPLETE                             ║
echo  ║   Output: build-release\Release\             ║
echo  ║                                              ║
echo  ║   To build the installer:                    ║
echo  ║   Copy Release\ contents to installer\       ║
echo  ║   Then run NSIS on installer\eagle_library.nsi
echo  ╚══════════════════════════════════════════════╝
echo.

cd /d "%~dp0"
pause
