; ============================================================
;  Eagle Library — NSIS Installer Script
;  Copyright (c) 2026 Eagle Software. All rights reserved.
;  Requires: NSIS 3.x + Modern UI 2
;
;  Expected input:
;  Copy the deployed release payload into this installer folder
;  before compiling the script. This script intentionally packages
;  the runtime DLLs directly, so no VC++ redistributable installer
;  is required at install time.
; ============================================================

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinVer.nsh"

; ── General ──────────────────────────────────────────────────
Name              "Eagle Library"
OutFile           "EagleLibrary_Setup_1.0.0.exe"
InstallDir        "$PROGRAMFILES64\Eagle Library"
InstallDirRegKey  HKLM "Software\Eagle Software\Eagle Library" "InstallDir"
RequestExecutionLevel admin

; ── Version info ─────────────────────────────────────────────
VIProductVersion  "1.0.0.0"
VIAddVersionKey   "ProductName"     "Eagle Library"
VIAddVersionKey   "CompanyName"     "Eagle Software"
VIAddVersionKey   "LegalCopyright"  "Copyright (C) 2026 Eagle Software"
VIAddVersionKey   "FileDescription" "Eagle Library 1.0 Installer"
VIAddVersionKey   "FileVersion"     "1.0.0.0"

; ── MUI Settings ─────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_ICON              "..\resources\eagle.ico"
!define MUI_UNICON            "..\resources\eagle.ico"
!define MUI_HEADERIMAGE

!define MUI_WELCOMEPAGE_TITLE   "Welcome to Eagle Library 1.0 Setup"
!define MUI_WELCOMEPAGE_TEXT    "Eagle Library is a professional eBook library manager by Eagle Software.$\r$\n$\r$\nManage thousands of books across any folder structure, fetch metadata from the internet, and build your personal reading collection.$\r$\n$\r$\nClick Next to continue."

!define MUI_FINISHPAGE_RUN         "$INSTDIR\EagleLibrary.exe"
!define MUI_FINISHPAGE_RUN_TEXT    "Launch Eagle Library"
!define MUI_FINISHPAGE_LINK        "Visit eaglesoftware.biz"
!define MUI_FINISHPAGE_LINK_LOCATION "https://eaglesoftware.biz/"

; ── Pages ─────────────────────────────────────────────────────
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE    "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ── Installer sections ────────────────────────────────────────
Section "Eagle Library (required)" SecMain
    SectionIn RO
    SetShellVarContext all

    ; OS check — Windows 7 or later
    ${IfNot} ${AtLeastWin7}
        MessageBox MB_OK|MB_ICONSTOP "Eagle Library requires Windows 7 or later."
        Abort
    ${EndIf}

    SetOutPath "$INSTDIR"

    CreateDirectory "$INSTDIR\data"
    CreateDirectory "$INSTDIR\data\backups"
    CreateDirectory "$INSTDIR\settings"

    ; Preserve the previous database before installing over an existing copy,
    ; then clear active DB files so the new version starts with a fresh library.
    IfFileExists "$INSTDIR\library.db" 0 +4
        CopyFiles /SILENT "$INSTDIR\library.db" "$INSTDIR\data\backups\library-preupgrade-root.db"
        Delete "$INSTDIR\library.db"
        Delete "$INSTDIR\library.db-wal"
    Delete "$INSTDIR\library.db-shm"
    IfFileExists "$INSTDIR\data\library.db" 0 +4
        CopyFiles /SILENT "$INSTDIR\data\library.db" "$INSTDIR\data\backups\library-preupgrade-data.db"
        Delete "$INSTDIR\data\library.db"
        Delete "$INSTDIR\data\library.db-wal"
    Delete "$INSTDIR\data\library.db-shm"
    IfFileExists "$INSTDIR\EagleLibrary.ini" 0 +1
        CopyFiles /SILENT "$INSTDIR\EagleLibrary.ini" "$INSTDIR\data\backups\EagleLibrary-preupgrade-root.ini"
    IfFileExists "$INSTDIR\settings\EagleLibrary.ini" 0 +1
        CopyFiles /SILENT "$INSTDIR\settings\EagleLibrary.ini" "$INSTDIR\data\backups\EagleLibrary-preupgrade-settings.ini"

    ; Main executable
    File "EagleLibrary.exe"
    FileOpen $0 "$INSTDIR\portable.flag" w
    FileClose $0

    ; Installed layout is intentionally self-contained like a portable app.
    ; Grant standard users write access only to runtime-state folders.
    nsExec::ExecToLog 'icacls "$INSTDIR\data" /grant *S-1-5-32-545:(OI)(CI)M /T /C'
    nsExec::ExecToLog 'icacls "$INSTDIR\settings" /grant *S-1-5-32-545:(OI)(CI)M /T /C'

    ; Qt runtime DLLs (produced by windeployqt in build dir)
    File "Qt6Core.dll"
    File "Qt6Gui.dll"
    File "Qt6Widgets.dll"
    File "Qt6Network.dll"
    File "Qt6Sql.dll"
    ; Optional Qt runtime DLLs that may appear depending on build usage
    File /nonfatal "Qt6Svg.dll"
    File /nonfatal "Qt6Concurrent.dll"
    File /nonfatal "Qt6Xml.dll"

    ; MSVC runtime
    File /nonfatal "concrt140.dll"
    File /nonfatal "vccorlib140.dll"
    File /nonfatal "vcruntime140.dll"
    File /nonfatal "vcruntime140_1.dll"
    File /nonfatal "vcruntime140_threads.dll"
    File /nonfatal "msvcp140.dll"
    File /nonfatal "msvcp140_1.dll"
    File /nonfatal "msvcp140_2.dll"
    File /nonfatal "msvcp140_atomic_wait.dll"
    File /nonfatal "msvcp140_codecvt_ids.dll"

    ; Software renderer / DirectX compiler support when deployed
    File /nonfatal "d3dcompiler_47.dll"
    File /nonfatal "dxcompiler.dll"
    File /nonfatal "dxil.dll"
    File /nonfatal "opengl32sw.dll"

    ; Qt platform plugin
    SetOutPath "$INSTDIR\platforms"
    File /r "platforms\*.*"

    ; Qt image format plugins
    SetOutPath "$INSTDIR\imageformats"
    File /r "imageformats\*.*"

    ; Qt SQL driver
    SetOutPath "$INSTDIR\sqldrivers"
    File /nonfatal "sqldrivers\qsqlite.dll"

    ; Qt style plugins
    SetOutPath "$INSTDIR\styles"
    File /nonfatal /r "styles\*.*"

    ; SVG icon engine
    SetOutPath "$INSTDIR\iconengines"
    File /nonfatal /r "iconengines\*.*"

    ; Windows TLS backend
    SetOutPath "$INSTDIR\tls"
    File /nonfatal "tls\qschannelbackend.dll"

    ; Docs
    SetOutPath "$INSTDIR"
    File "README.txt"
    File "LICENSE.txt"
    SetOutPath "$INSTDIR\help"
    File /nonfatal "help\EagleLibrary.chm"
    SetOutPath "$INSTDIR\translations"
    File /nonfatal /r "translations\*.*"
    SetOutPath "$INSTDIR\themes"
    File /nonfatal /r "themes\*.*"
    SetOutPath "$INSTDIR\resources"
    File /nonfatal /r "resources\*.*"
    SetOutPath "$INSTDIR\hooks"
    File /nonfatal /r "hooks\*.*"
    SetOutPath "$INSTDIR\plugins"
    File /nonfatal /r "plugins\*.*"

    ; ── Registry ─────────────────────────────────────────────
    WriteRegStr HKLM "Software\Eagle Software\Eagle Library" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\Eagle Software\Eagle Library" "Version"    "1.0.0"

    ; ── Uninstall info (Add/Remove Programs) ─────────────────
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary" \
        "DisplayName"          "Eagle Library"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary" \
        "UninstallString"      '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary" \
        "InstallLocation"      "$INSTDIR"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary" \
        "Publisher"            "Eagle Software"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary" \
        "DisplayVersion"       "1.0.0"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary" \
        "URLInfoAbout"         "https://eaglesoftware.biz/"
    WriteRegDWORD HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary" \
        "NoModify" 1
    WriteRegDWORD HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary" \
        "NoRepair"  1

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; ── Shortcuts ─────────────────────────────────────────────
    CreateDirectory "$SMPROGRAMS\Eagle Library"
    CreateShortcut  "$SMPROGRAMS\Eagle Library\Eagle Library.lnk" \
                    "$INSTDIR\EagleLibrary.exe" "" "$INSTDIR\EagleLibrary.exe" 0
    CreateShortcut  "$SMPROGRAMS\Eagle Library\Uninstall.lnk" \
                    "$INSTDIR\Uninstall.exe"
    CreateShortcut  "$DESKTOP\Eagle Library.lnk" \
                    "$INSTDIR\EagleLibrary.exe" "" "$INSTDIR\EagleLibrary.exe" 0

SectionEnd

; ── Uninstaller ───────────────────────────────────────────────
Section "Uninstall"
    SetShellVarContext all

    ; Remove files
    Delete "$INSTDIR\EagleLibrary.exe"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\README.txt"
    Delete "$INSTDIR\LICENSE.txt"
    Delete "$INSTDIR\portable.flag"
    Delete "$INSTDIR\library.db"
    Delete "$INSTDIR\library.db-wal"
    Delete "$INSTDIR\library.db-shm"
    Delete "$INSTDIR\Uninstall.exe"
    Delete "$INSTDIR\help\EagleLibrary.chm"
    RMDir /r "$INSTDIR\platforms"
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\sqldrivers"
    RMDir /r "$INSTDIR\styles"
    RMDir /r "$INSTDIR\iconengines"
    RMDir /r "$INSTDIR\tls"
    RMDir /r "$INSTDIR\help"
    RMDir /r "$INSTDIR\translations"
    RMDir /r "$INSTDIR\themes"
    RMDir /r "$INSTDIR\resources"
    RMDir /r "$INSTDIR\hooks"
    RMDir /r "$INSTDIR\plugins"

    ; Remove shortcuts
    Delete "$SMPROGRAMS\Eagle Library\Eagle Library.lnk"
    Delete "$SMPROGRAMS\Eagle Library\Uninstall.lnk"
    Delete "$DESKTOP\Eagle Library.lnk"
    RMDir  "$SMPROGRAMS\Eagle Library"

    ; Remove registry entries
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary"
    DeleteRegKey HKLM "Software\Eagle Software\Eagle Library"

    ; Optional: remove local runtime data
    MessageBox MB_YESNO|MB_ICONQUESTION \
        "Do you want to remove the local Eagle Library database, settings, and cached data from the install folder?$\r$\n(This deletes the settings folder, the data folder, and any backup copies.)" \
        IDNO skip_userdata
        Delete "$INSTDIR\EagleLibrary.ini"
        Delete "$INSTDIR\settings\EagleLibrary.ini"
        RMDir /r "$INSTDIR\data"
        RMDir /r "$INSTDIR\settings"
    skip_userdata:

    RMDir "$INSTDIR"

SectionEnd
