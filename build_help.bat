@echo off
rem ============================================================
rem  Eagle Library — CHM Help Build Script
rem  Copyright (c) 2026 Eagle Software. All rights reserved.
rem ============================================================

setlocal

set "SCRIPT_DIR=%~dp0"
set "HELP_DIR=%SCRIPT_DIR%docs\help"
set "HHP_FILE=%HELP_DIR%\EagleLibrary.hhp"
set "CHM_FILE=%HELP_DIR%\EagleLibrary.chm"
set "PF86=%ProgramFiles(x86)%"
if "%PF86%"=="" set "PF86=C:\Program Files (x86)"
set "HHC_EXE=%PF86%\HTML Help Workshop\hhc.exe"
if not exist "%HHC_EXE%" set "HHC_EXE=C:\Program Files (x86)\HTML Help Workshop\hhc.exe"

echo.
echo  ╔══════════════════════════════════════════════╗
echo  ║     EAGLE LIBRARY  —  CHM Help Build         ║
echo  ╚══════════════════════════════════════════════╝
echo.

if not exist "%HHP_FILE%" (
    echo [ERROR] Help project not found:
    echo         %HHP_FILE%
    pause & exit /b 1
)

if not exist "%HHC_EXE%" (
    echo [ERROR] HTML Help Workshop was not found.
    echo         Download and install:
    echo         Microsoft HTML Help Workshop
    echo         Expected path:
    echo         C:\Program Files ^(x86^)\HTML Help Workshop\hhc.exe
    pause & exit /b 1
)

echo [INFO] Help source: %HELP_DIR%
echo [INFO] Compiler:    %HHC_EXE%
echo.
echo [1/1] Building CHM...
pushd "%HELP_DIR%"
"%HHC_EXE%" "EagleLibrary.hhp"
set "ERR=%ERRORLEVEL%"
popd

if exist "%CHM_FILE%" set "ERR=0"

if not "%ERR%"=="0" (
    echo [ERROR] CHM build failed.
    pause & exit /b %ERR%
)

echo.
echo [DONE] CHM created at:
echo        %CHM_FILE%
echo.
pause
