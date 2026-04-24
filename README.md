# Eagle Library

Professional Windows desktop software for indexing, organizing, maintaining, and researching large collections of books, papers, manuals, and mixed-format document archives.

## Overview

Eagle Library is a native Qt application designed around a non-destructive, reference-first model:

- files stay in their original locations
- the library acts as an index, not a container
- metadata, covers, tags, and virtual organization live on top of your existing folders

It is aimed at users who manage mixed collections such as:

- books
- research papers
- manuals
- technical PDFs
- office documents
- scanned archives

## Highlights

- Multi-library profiles with separate watched folders
- Fast library scanning with incremental re-index support
- Virtual organization through shelves, tags, collections, and saved searches
- Metadata enrichment from embedded metadata, Open Library, and Google Books
- Upgrade-safe local storage with backup/import support for previous databases
- Built-in database editor, metadata enrichment, and maintenance workflows
- Task Center with popup notices, queued task history, and batch execution
- Portable mode with local data and settings
- CHM offline help and HTML documentation
- Plugin-ready architecture for extra book/document actions
- Themes, command palette, and multilingual UI support

## Core Philosophy

Eagle Library is built around these principles:

- `Library = index of files, not file storage`
- `No forced copy / move workflow`
- `Files remain in original locations`
- `Database can be rebuilt from scan`
- `Organization is virtual whenever possible`

## Main Features

### Library Management

- Multiple library profiles
- Separate watched folders per library
- Active library switching from the main toolbar
- Drive/folder path remapping when external disk letters change
- Portable and installed deployment modes

### Scanning and Indexing

- Parallel scanning
- Fast scan mode for large collections
- Incremental scan scheduling when watched folders change
- Mixed-format support for books and documents
- Reference-based indexing without moving source files

### Metadata and Covers

- Embedded metadata reading
- Online metadata enrichment
- Cover lookup and cover normalization
- Metadata Manager for:
  - only items missing metadata
  - selected items
  - all items in the active library
- Smart rename with safer content/title fallback

### Search and Organization

- Search box with field-aware queries
- Advanced search
- Saved searches
- Format, category, favourites, no-cover, and no-metadata filters
- Tags, collections, and smart grouping
- Books vs documents categorization

### Database and Maintenance Tools

- Built-in database editor
- Duplicate analysis
- Search-index rebuild
- SQLite optimize / maintenance tools
- Import previous database backups into the current library

### UI and Workflow

- List and grid view modes
- Command palette
- Blue, white, and mac-style themes
- Status-bar task progress with popup notifications
- Task Center for queued tasks and recent activity
- Batch task runner for maintenance workflows
- Detail dialogs with plugin actions
- Multilingual UI with built-in language packs and external language-pack support

### Extensibility

- Runtime plugin folder
- Starter plugin manifests
- Plugin actions in item details
- External translations folder for custom language packs
- Hardened local hook execution inside the app hooks directory

## Supported Formats

Current format support includes:

- PDF
- EPUB
- MOBI
- AZW / AZW3
- DjVu
- FB2
- CBZ / CBR
- TXT / RTF / Markdown / CSV
- Microsoft Word: DOC / DOCX / DOCM / DOCB / DOT / DOTX / DOTM
- Microsoft Excel: XLS / XLSX / XLSM / XLSB / XLT / XLTX / XLTM
- Microsoft PowerPoint: PPT / PPTX / PPTM / POT / POTX / POTM / PPS / PPSX / PPSM / SLDX / SLDM
- Microsoft Visio / Publisher / Project / Access / Outlook: VSD / VSDX / VSDM / VSS / VSSX / VSSM / VST / VSTX / VSTM / VDX / PUB / MPP / MPT / MDB / ACCDB / MSG
- OpenDocument: ODT / OTT / ODS / OTS / ODP / OTP / ODG / ODF
- Apple iWork: Pages / Numbers / Keynote
- OneNote, XPS / OpenXPS, HTML, XML
- CHM
- LIT

## Screens / Packaging Outputs

Typical generated outputs:

- App: `build-release/Release/EagleLibrary.exe`
- Installer: `installer/EagleLibrary_Setup_1.0.0.exe`
- Portable folder: `\EagleLibrary_Portable`
- GitHub-style release assets:
  - `release/EagleLibrary-Setup-x64.exe`
  - `release/EagleLibrary-Portable-x64.zip`
  - `release/EagleLibrary.chm`
  - `release/SHA256SUMS.txt`

## Requirements

### Runtime

- Windows 10 or Windows 11 (64-bit)

### Build prerequisites

| Tool | Required version | Download |
|---|---|---|
| Visual Studio 2022 | Community / Professional / Enterprise / Build Tools | https://visualstudio.microsoft.com/downloads/ |
| VS workload | **Desktop development with C++** (includes MSVC v143 + Windows SDK) | via VS Installer |
| Qt 6.7.x | `msvc2022_64` kit | https://www.qt.io/download-qt-installer |
| CMake | 3.20 or later | https://cmake.org/download/ |
| NSIS 3.x | only needed to build the installer | https://nsis.sourceforge.io/Download |

### Required Qt modules

Qt Core · Qt Gui · Qt Widgets · Qt Network · Qt Sql · Qt Concurrent · Qt Xml · Qt Svg

---

## Building locally (Windows)

### Step 0 — set the QT_DIR environment variable once

Every build path (Visual Studio, `build.bat`, and manual CMake) reads Qt's
location from the `QT_DIR` environment variable.  Set it once in Windows and
you never have to touch it again:

1. Open **Start → Edit the system environment variables**
2. Click **Environment Variables…**
3. Under *User variables*, click **New**
4. Variable name: `QT_DIR`
5. Variable value: path to your Qt MSVC 2022 64-bit kit, e.g.
   `C:\Qt\6.7.3\msvc2022_64`
6. Click **OK** everywhere, then restart any open terminals or Visual Studio

```
QT_DIR = C:\Qt\6.7.3\msvc2022_64   ← adjust to your installed version
```

---

### Option A — Visual Studio 2022 (Open Folder)

This is the recommended workflow for day-to-day development.

1. Open Visual Studio 2022
2. Choose **Open a local folder** and select the repository root
3. Visual Studio detects `CMakePresets.json` automatically
4. In the toolbar dropdown select **Windows x64 Release (MSVC)**
   (or **Windows x64 Debug (MSVC)** for a debug build)
5. Press **Ctrl+Shift+B** to build

Output: `out\build\windows-release\Release\EagleLibrary.exe`

> Qt DLLs are copied automatically by the `windeployqt` post-build step
> defined in `CMakeLists.txt`.

**If you prefer a hardcoded path instead of the env var:**

Copy `CMakeUserPresets.json.template` to `CMakeUserPresets.json` (gitignored)
and edit the `CMAKE_PREFIX_PATH` value to your Qt path.  Visual Studio picks it
up automatically and adds a **local** preset to the dropdown.

---

### Option B — build.bat (quick command-line build)

Double-click **`build.bat`** in the repository root.

The script reads `QT_DIR` if set, otherwise auto-detects Qt 6.7.x and
Visual Studio 2022 in common install locations.

```
build-release\Release\EagleLibrary.exe   ← ready to run
```

Override any path without editing the file:

```bat
set QT_DIR=C:\Qt\6.7.3\msvc2022_64
set VS_DIR=C:\Program Files\Microsoft Visual Studio\2022\Community
set CMAKE_DIR=C:\Program Files\CMake\bin
build.bat
```

---

### Option C — manual CMake (command line)

Open a **Visual Studio 2022 x64 Developer Command Prompt**, then:

```bat
cmake -S . -B build-release ^
      -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
      -DCMAKE_BUILD_TYPE=Release

cmake --build build-release --config Release --parallel

"%QT_DIR%\bin\windeployqt.exe" ^
      --no-translations --compiler-runtime ^
      build-release\Release\EagleLibrary.exe
```

---

### Build the CHM help file

```bat
build_help.bat
```

Output: `docs\help\EagleLibrary.chm`

Requires **Microsoft HTML Help Workshop** (`hhc.exe`) on the PATH.

### Create a portable package

```bat
installer\refresh_portable.bat build-release\Release \EagleLibrary_Portable
```

Portable mode activates when `portable.flag` (or `portable.ini`) exists next
to the `.exe`. All data and settings are then stored locally:

```
settings\EagleLibrary.ini   data\library.db   data\backups\   plugins\   translations\   themes\   hooks\   resources\   help\
```

### Build the NSIS installer

```bat
makensis installer\eagle_library.nsi
```

Or open the `.nsi` file in NSIS and click **Compile**.

Output: `installer\EagleLibrary_Setup_1.0.0.exe`

---

## Building on GitHub (Actions)

No local setup needed — GitHub's runners handle everything.

### How to trigger a build

1. Go to your repository on **GitHub**
2. Click the **Actions** tab
3. Select **Build EagleLibrary** in the left sidebar
4. Click **Run workflow** (top-right of the workflow list)
5. Choose whether to also build the NSIS installer
6. Click the green **Run workflow** button

> Builds do **not** run automatically on commits or pull requests.
> They only run when you manually trigger them here.

### Downloading the output

After the workflow finishes (≈ 10–15 minutes):

1. Click the completed run
2. Scroll to **Artifacts** at the bottom of the page
3. Download:
   - `EagleLibrary-Portable-x64` — portable folder (`.exe` + Qt DLLs)
   - `EagleLibrary-Setup-x64` — NSIS installer (if selected)

Artifacts are kept for **30 days**.

### What the workflow does

| Step | Action |
|---|---|
| Checkout | Clones the repository |
| Install Qt 6.7.3 | Downloads and caches the MSVC 64-bit Qt kit, sets `QT_DIR` |
| Configure CMake | Visual Studio 17 2022 generator, Release config |
| Build | Compiles all sources in parallel |
| windeployqt | Copies required Qt DLLs next to the executable |
| Copy plugins | Bundles the `plugins/` folder into the output |
| CPack / NSIS | Creates the installer (optional, chosen at trigger time) |
| Upload artifacts | Makes both builds available for download |

## Documentation

Documentation is available in two forms:

- HTML help pages under `docs/help/`
- offline CHM help: `docs/help/EagleLibrary.chm`

The help currently covers:

- libraries and shelves
- menus and toolbar
- settings reference
- metadata and scanning tools
- database tools and maintenance
- portable deployment

## Minimal Feature Summary

At minimum, Eagle Library currently provides:

- local library indexing
- metadata viewing and editing
- metadata enrichment
- cover management
- duplicate analysis
- saved search support
- theme switching
- multilingual UI
- plugin-ready extensions
- portable and installer packaging

## Release Notes

### 1.0

**Eagle Library 1.0 — GitHub release package**

This release prepares the first GitHub-ready distribution of Eagle Library with
installer, portable package, offline help, and checksums published together.
It also refreshes document-library support so ebook collections can include
Microsoft Office and other mixed document archives in the same catalog.

#### Included downloads

- `EagleLibrary-Setup-x64.exe` — Windows installer for 64-bit systems
- `EagleLibrary-Portable-x64.zip` — portable package that keeps data beside the executable
- `EagleLibrary.chm` — offline help file
- `SHA256SUMS.txt` — checksums for release verification

#### Highlights

- Reference-first library management: files remain in their original folders
- Local database and settings stored in `data\` and `settings\` beside the executable
- Mixed ebook and document indexing for PDF, EPUB, MOBI, AZW/AZW3, DjVu, FB2,
  CBZ/CBR, CHM, LIT, plain text, Markdown, CSV, HTML/XML, OpenDocument,
  Apple iWork, OneNote, XPS/OpenXPS, and Microsoft Office files
- Microsoft Office scan support now includes Word, Excel, PowerPoint, Visio,
  Publisher, Project, Access, and Outlook message formats
- Browse sidebar format shortcuts now include Office document families so Word,
  Excel, PowerPoint, Visio, Publisher, Project, Access, and OneNote files are
  easy to filter after scanning
- Smart categorization treats Office and OpenDocument families as documents,
  while still allowing long-form PDFs and ebooks to remain in book workflows
- NSIS setup installs a self-contained Program Files layout with Qt runtime DLLs, plugins, themes, translations, help, and Start Menu shortcuts
- Installer upgrades now back up the previous database and settings, then allow import from the backup inside the app
- Metadata enrichment, cover tools, and database maintenance
- Grid/list views, command palette, themes, and multilingual UI
- Plugin actions for book and document details
- CHM documentation bundled with release artifacts
- Improved company-logo branding in the toolbar and splash screen
- Improved scan/status progress indicators without modal background dialogs
- Task Center, popup task notices, and batch maintenance execution
- Extra scan-time protection now locks live library widgets while the first scan is inserting rows
- Diagnostic logs are disabled by default and can be enabled from Settings
- Plugin URL actions are restricted to trusted HTTPS destinations
- Python hook execution is disabled unless explicitly enabled
- Release workflow supports Authenticode signing when certificate secrets are configured
- Release workflow runs unit tests before packaging

#### Windows publisher note

Windows SmartScreen and UAC identify unsigned executables as coming from an unknown
publisher. The release workflow can sign `EagleLibrary.exe` and the installer when
the repository has a valid Authenticode certificate configured through
`WINDOWS_CODESIGN_CERT_BASE64` and `WINDOWS_CODESIGN_CERT_PASSWORD`. Without a
code-signing certificate, no source-code change can make Windows show a verified
publisher.


#### New features

- Multi-library profiles with separate watched folders and active-library toolbar switcher
- Parallel incremental library scanner with fast-scan mode for large collections
- Smart rename engine: extracts title and author from filename patterns, embedded
  PDF metadata, and content text (first-page extraction + OCR fallback via Tesseract)
- Metadata Manager with per-item, selection-based, and full-library fetch modes
- Online metadata enrichment from Google Books and Open Library APIs
- Cover download, normalization, and local caching
- Plugin Manager with scrollable card list, status badges, action counts, and runtime plugin folder
- Plugin actions displayed in book and document detail panels
- Advanced search with field-aware query tokens (`author:`, `title:`, `isbn:`, `tag:`, etc.)
- Saved searches and smart category sidebar
- Built-in database editor with direct SQLite record editing
- Nonblocking ISBN extraction and metadata lookup so slow PDF/OCR probing no longer freezes the app
- Duplicate analysis and database maintenance tools for large catalogs
- Full-text search index with rebuild and optimize controls
- Blue, white, and mac-style themes with runtime switching
- Command palette for keyboard-driven access to all actions
- Multilingual UI with built-in language packs (EN, FR, ES, DE, PT-BR, RU, AR, ZH, HI, JA)
  and external language-pack folder support
- Portable deployment mode: all data, settings, and covers stored beside the executable
- CHM offline help with full topic coverage

#### Fixes and improvements

- Metadata accuracy: query title cleaned before API search — edition markers, version numbers,
  source tags (`[z-lib]`, `[libgen]`), and trailing years stripped so APIs return correct results
- Metadata accuracy: zero-word title overlap now scores −18; results with no meaningful words
  in common with the searched title are rejected, eliminating false book matches
- Metadata accuracy: ISBN-10 ↔ ISBN-13 (978-prefix) cross-match now scores +28 instead of −30,
  preventing false mismatches between identical books in different edition formats
- Cover accuracy: cover URLs now taken exclusively from the highest-scoring candidate;
  the previous flat pool allowed wrong-book covers to override correct ones
- Embedded metadata +15 bonus is now preserved through the scoring pipeline instead of being
  silently discarded during the finalization re-score pass
- ISBN extraction searches labeled `ISBN:` text before falling back to numeric patterns,
  preventing phone numbers and order IDs from being misidentified
- PDF `/Creator` field (authoring tool name) is no longer used as publisher source;
  only the `/Publisher` field is accepted
- Theme switching applies correctly; per-widget stylesheet clear that broke the QSS cascade removed
- Runtime theme switching now repolishes open widgets so the selected theme applies immediately and reliably
- Language switching now refreshes more menu, submenu, toolbar, status, and release-facing labels
- About dialogs now show the Eagle Software company logo, and the bundled SVG mark matches the company logo
- Database text diagnosis/repair actions were removed from the visible menu and help surface
- ISBN extraction runs in a capped background worker pool to keep the UI responsive during slow file/PDF/OCR reads
- Scanner thread waits up to 3 seconds after cancel before the window closes; Qt events flushed
  before plugin unload to prevent stale signal delivery
- Task popup and status updates now marshal to the GUI thread and stop cleanly during shutdown
- Library views and filter widgets are temporarily locked during active scans to avoid interaction with changing model indexes
- Toolbar/sidebar teardown now uses deferred deletion, reducing use-after-free risks during UI refresh and language changes
- Resize handling now defers heavy responsive layout work instead of mutating the live widget tree directly inside `resizeEvent()`
- Scanner, rename, and ISBN watcher lifetimes are guarded with tracked pointers so stale async callbacks do not query freed objects
- Metadata timer restarts correctly on subsequent fetches
- App data and settings stored beside the executable for both portable and installed modes
- CMake: Qt auto-detected in `C:/Qt/6.*/msvc2022_64` and `C:/Qt/6.*/msvc2019_64` — no
  environment variable required for Visual Studio Open Folder workflow
- CI: Actions workflow uses a named `ci-release` CMakePresets preset for reproducible builds;
  `cache-key-prefix` makes Qt installation cache explicit and predictable

## Repository Layout

```text
include/       public headers
src/           application source
resources/     icons, themes, translations, branding
plugins/       starter plugin manifests
docs/help/     HTML help + CHM project files
installer/     installer script and packaging payload
release/       GitHub-ready release artifacts
```

## License

This project is released under the MIT License.

See [LICENSE](LICENSE).


Eagle Software  
https://eaglesoftware.biz/
