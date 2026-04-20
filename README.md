# Eagle Library

Professional Windows desktop software for indexing, organizing, repairing, and researching large collections of books, papers, manuals, and mixed-format document archives.

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
- Built-in database editor, diagnosis tools, and metadata repair workflows
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

### Database and Repair Tools

- Built-in database editor
- Text diagnosis for mojibake, binary noise, and broken PDF metadata
- Repair tools for suspicious records
- Duplicate analysis
- Search-index rebuild
- SQLite optimize / maintenance tools

### UI and Workflow

- List and grid view modes
- Command palette
- Blue, white, and mac-style themes
- Status-based task progress
- Detail dialogs with plugin actions
- Multilingual UI with built-in language packs and external language-pack support

### Extensibility

- Runtime plugin folder
- Starter plugin manifests
- Plugin actions in item details
- External translations folder for custom language packs

## Supported Formats

Current format support includes:

- PDF
- EPUB
- MOBI
- AZW / AZW3
- DjVu
- FB2
- CBZ / CBR
- TXT
- RTF
- DOC / DOCX
- CHM
- LIT

## Screens / Packaging Outputs

Typical generated outputs:

- App: `build-release/Release/EagleLibrary.exe`
- Installer: `installer/EagleLibrary_Setup_2.0.0.exe`
- Portable folder: `C:\eagle_software\EagleLibrary_Portable`
- GitHub-style release assets:
  - `release/EagleLibrary-Setup-x64.exe`
  - `release/EagleLibrary-Portable-x64.zip`
  - `release/EagleLibrary.chm`
  - `release/SHA256SUMS.txt`

## Requirements

### Runtime

- Windows 10 or Windows 11 recommended

### Build Requirements

- Visual Studio 2022 or Visual Studio 2022 Build Tools
- `Desktop development with C++`
- MSVC v143 toolset
- Windows 10 or 11 SDK
- Qt 6.7.x `msvc2022_64`
- CMake
- NSIS 3.x
- Microsoft HTML Help Workshop

### Required Qt Modules

- Qt Core
- Qt Gui
- Qt Widgets
- Qt Network
- Qt Sql
- Qt Concurrent
- Qt Xml
- Qt Svg

## Build Instructions

### 1. Build the application

From the repository root:

```bat
build.bat
```

This script:

- locates Visual Studio 2022
- locates Qt for MSVC 2022
- configures CMake
- builds the Release target
- runs `windeployqt`
- copies starter plugins into the runtime output

Main output:

```text
build-release\Release\EagleLibrary.exe
```

### 2. Build the CHM help file

```bat
build_help.bat
```

Output:

```text
docs\help\EagleLibrary.chm
```

### 3. Refresh the portable package

```bat
installer\refresh_portable.bat build-release\Release C:\eagle_software\EagleLibrary_Portable
```

Portable mode stores data locally under:

- `data\`
- `settings\`
- `help\`
- `plugins\`
- `translations\`

Portable mode is enabled by:

- `portable.flag`
- or `portable.ini`

### 4. Build the NSIS installer

Compile:

```text
installer\eagle_library.nsi
```

Current setup filename:

```text
installer\EagleLibrary_Setup_2.0.0.exe
```

## Documentation

Documentation is available in two forms:

- HTML help pages under `docs/help/`
- offline CHM help: `docs/help/EagleLibrary.chm`

The help currently covers:

- libraries and shelves
- menus and toolbar
- settings reference
- metadata and scanning tools
- database tools and repair
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

### 2.0.0

- upgraded product version to `2.0.0`
- added stronger reference-style indexing
- added incremental watch-based refresh support
- expanded metadata repair and diagnosis tools
- improved plugin visibility and packaging
- added more language packs and easier language switching
- added metadata management controls
- improved theme switching behavior
- improved splash branding and packaging outputs
- refreshed installer, portable package, and CHM help

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

## Company

Eagle Software  
https://eaglesoftware.biz/
