@echo off
setlocal EnableExtensions

rem Refresh a fully self-contained portable folder from a deployed build.
rem Usage:
rem   refresh_portable.bat [build_output_dir] [portable_target_dir]

set "SRC=%~1"
if "%SRC%"=="" set "SRC=%~dp0..\build-release\Release"

set "DST=%~2"
if "%DST%"=="" set "DST=C:\eagle_software\EagleLibrary_Portable"

if not exist "%SRC%\EagleLibrary.exe" (
    echo [ERROR] Build output not found: %SRC%
    exit /b 1
)

echo [1/5] Preparing portable folder without touching local data...
if not exist "%DST%" mkdir "%DST%"
if not exist "%DST%\data" mkdir "%DST%\data"
if not exist "%DST%\translations" mkdir "%DST%\translations"
if not exist "%DST%\themes" mkdir "%DST%\themes"
if not exist "%DST%\hooks" mkdir "%DST%\hooks"
if not exist "%DST%\resources" mkdir "%DST%\resources"
if not exist "%DST%\plugins" mkdir "%DST%\plugins"
if not exist "%DST%\platforms" mkdir "%DST%\platforms"
type nul > "%DST%\portable.flag"

echo [2/5] Copying app payload...
copy /y "%SRC%\EagleLibrary.exe" "%DST%\" >nul
copy /y "%SRC%\Qt6Core.dll" "%DST%\" >nul
copy /y "%SRC%\Qt6Gui.dll" "%DST%\" >nul
copy /y "%SRC%\Qt6Widgets.dll" "%DST%\" >nul
copy /y "%SRC%\Qt6Network.dll" "%DST%\" >nul
copy /y "%SRC%\Qt6Sql.dll" "%DST%\" >nul
copy /y "%SRC%\Qt6Svg.dll" "%DST%\" >nul
copy /y "%SRC%\README.txt" "%DST%\" >nul
copy /y "%SRC%\LICENSE.txt" "%DST%\" >nul

for %%F in (d3dcompiler_47.dll dxcompiler.dll dxil.dll opengl32sw.dll) do (
    if exist "%SRC%\%%F" copy /y "%SRC%\%%F" "%DST%\" >nul
)

echo [3/5] Copying Qt plugins...
if exist "%SRC%\platforms\qwindows.dll" (
    mkdir "%DST%\platforms"
    copy /y "%SRC%\platforms\qwindows.dll" "%DST%\platforms\" >nul
)
if exist "%SRC%\imageformats" xcopy "%SRC%\imageformats" "%DST%\imageformats" /E /I /Y >nul
if exist "%SRC%\styles\qmodernwindowsstyle.dll" (
    mkdir "%DST%\styles"
    copy /y "%SRC%\styles\qmodernwindowsstyle.dll" "%DST%\styles\" >nul
)
if exist "%SRC%\plugins" xcopy "%SRC%\plugins" "%DST%\plugins" /E /I /Y >nul
if exist "%SRC%\translations" xcopy "%SRC%\translations" "%DST%\translations" /E /I /Y >nul
if exist "%SRC%\themes" xcopy "%SRC%\themes" "%DST%\themes" /E /I /Y >nul
if exist "%SRC%\hooks" xcopy "%SRC%\hooks" "%DST%\hooks" /E /I /Y >nul
if exist "%SRC%\resources" xcopy "%SRC%\resources" "%DST%\resources" /E /I /Y >nul
if exist "%SRC%\iconengines\qsvgicon.dll" (
    mkdir "%DST%\iconengines"
    copy /y "%SRC%\iconengines\qsvgicon.dll" "%DST%\iconengines\" >nul
)
if exist "%SRC%\sqldrivers\qsqlite.dll" (
    mkdir "%DST%\sqldrivers"
    copy /y "%SRC%\sqldrivers\qsqlite.dll" "%DST%\sqldrivers\" >nul
)
if exist "%SRC%\tls\qschannelbackend.dll" (
    mkdir "%DST%\tls"
    copy /y "%SRC%\tls\qschannelbackend.dll" "%DST%\tls\" >nul
)

if exist "%~dp0..\docs\help\EagleLibrary.chm" (
    mkdir "%DST%\help"
    copy /y "%~dp0..\docs\help\EagleLibrary.chm" "%DST%\help\" >nul
)
if exist "%~dp0translations\README.txt" (
    copy /y "%~dp0translations\README.txt" "%DST%\translations\" >nul
)

echo [4/5] Copying MSVC runtime DLLs...
set "CRT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.42.34433\x64\Microsoft.VC143.CRT"
if not exist "%CRT%" set "CRT=C:\Windows\System32"
for %%F in (concrt140.dll vccorlib140.dll vcruntime140.dll vcruntime140_1.dll vcruntime140_threads.dll msvcp140.dll msvcp140_1.dll msvcp140_2.dll msvcp140_atomic_wait.dll msvcp140_codecvt_ids.dll) do (
    if exist "%CRT%\%%F" copy /y "%CRT%\%%F" "%DST%\" >nul
)

echo [5/5] Portable package refreshed:
echo        %DST%
echo.
echo Portable mode is enabled by portable.flag and keeps runtime state in:
echo   %DST%\settings\EagleLibrary.ini
echo   %DST%\data\library.db
echo   %DST%\data\backups
echo   %DST%\translations
echo   %DST%\themes
echo   %DST%\plugins
echo   %DST%\hooks

exit /b 0
