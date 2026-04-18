; ============================================================
;  Eagle Library — NSIS Installer Script
;  Copyright (c) 2024 Eagle Software. All rights reserved.
;  Requires: NSIS 3.x + Modern UI 2
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
VIAddVersionKey   "LegalCopyright"  "Copyright (C) 2024 Eagle Software"
VIAddVersionKey   "FileDescription" "Eagle Library Installer"
VIAddVersionKey   "FileVersion"     "1.0.0.0"

; ── MUI Settings ─────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_ICON              "..\resources\eagle.ico"
!define MUI_UNICON            "..\resources\eagle.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "banner.bmp"   ; 150x57 BMP
!define MUI_WELCOMEFINISHPAGE_BITMAP "wizard.bmp" ; 164x314 BMP

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

    ; OS check — Windows 7 or later
    ${IfNot} ${AtLeastWin7}
        MessageBox MB_OK|MB_ICONSTOP "Eagle Library requires Windows 7 or later."
        Abort
    ${EndIf}

    SetOutPath "$INSTDIR"

    ; Main executable
    File "EagleLibrary.exe"

    ; Qt runtime DLLs (produced by windeployqt in build dir)
    File "Qt6Core.dll"
    File "Qt6Gui.dll"
    File "Qt6Widgets.dll"
    File "Qt6Network.dll"
    File "Qt6Sql.dll"
    File "Qt6Concurrent.dll"
    File "Qt6Xml.dll"

    ; MSVC runtime (bundled via windeployqt --compiler-runtime)
    File /nonfatal "vcruntime140.dll"
    File /nonfatal "vcruntime140_1.dll"
    File /nonfatal "msvcp140.dll"
    File /nonfatal "msvcp140_1.dll"
    File /nonfatal "msvcp140_2.dll"

    ; Qt platform plugin
    SetOutPath "$INSTDIR\platforms"
    File /r "platforms\*.*"

    ; Qt image format plugins
    SetOutPath "$INSTDIR\imageformats"
    File /r "imageformats\*.*"

    ; Qt SQL driver
    SetOutPath "$INSTDIR\sqldrivers"
    File /r "sqldrivers\*.*"

    ; Qt style plugins
    SetOutPath "$INSTDIR\styles"
    File /nonfatal /r "styles\*.*"

    ; Docs
    SetOutPath "$INSTDIR"
    File "README.txt"
    File "LICENSE.txt"

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
    ; Remove files
    Delete "$INSTDIR\EagleLibrary.exe"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\README.txt"
    Delete "$INSTDIR\LICENSE.txt"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$INSTDIR\platforms"
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\sqldrivers"
    RMDir /r "$INSTDIR\styles"
    RMDir  "$INSTDIR"

    ; Remove shortcuts
    Delete "$SMPROGRAMS\Eagle Library\Eagle Library.lnk"
    Delete "$SMPROGRAMS\Eagle Library\Uninstall.lnk"
    Delete "$DESKTOP\Eagle Library.lnk"
    RMDir  "$SMPROGRAMS\Eagle Library"

    ; Remove registry entries
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EagleLibrary"
    DeleteRegKey HKLM "Software\Eagle Software\Eagle Library"

    ; Optional: remove user data (ask)
    MessageBox MB_YESNO|MB_ICONQUESTION \
        "Do you want to remove your Eagle Library database and settings?$\r$\n(All metadata and covers will be deleted.)" \
        IDNO skip_userdata
        SetShellVarContext current
        RMDir /r "$APPDATA\Eagle Software\Eagle Library"
    skip_userdata:

SectionEnd
