# Eagle Library Wiki

Eagle Library is a Windows desktop catalog for books, papers, manuals, and mixed-format document archives. It keeps files in their original folders, indexes metadata into a local database, and provides shelves, search, maintenance tools, exports, plugins, and offline help.

## Release 1.0

The current prerelease is `v1.0`. The release page provides:

- `EagleLibrary-Setup-x64.exe` - 64-bit Windows installer.
- `EagleLibrary-Portable-x64.zip` - portable build with `portable.flag`.
- `EagleLibrary.chm` - offline help file.
- `SHA256SUMS.txt` - checksums for verifying downloads.

## Supported Formats

Eagle Library scans common ebook and document formats in one library:

- Ebooks and publications: PDF, EPUB, MOBI, AZW/AZW3, DjVu, FB2, CBZ/CBR, CHM, LIT.
- Text and web documents: TXT, RTF, Markdown, CSV, HTML, HTM, XML.
- Microsoft Office: Word, Excel, PowerPoint, Visio, Publisher, Project, Access, Outlook message files, and OneNote.
- OpenDocument: ODT, OTT, ODS, OTS, ODP, OTP, ODG, ODF.
- Apple iWork: Pages, Numbers, Keynote.
- Windows document containers: XPS and OpenXPS.

Office and OpenDocument families are treated as documents by default. They can be filtered from the toolbar format selector or the browse sidebar after scanning.

## Scanning Workflow

1. Create or select a library profile in Settings.
2. Add the folders that contain books and documents.
3. Keep Fast Scan Mode enabled for large libraries.
4. Run Scan to index paths, hashes, sizes, titles, and formats.
5. Use Smart Categorize, Fetch Metadata, Extract ISBNs, and Smart Rename after indexing.

## Sidebar And Shelves

The browse sidebar provides quick filters for common ebook formats and document families, including Word, Excel, PowerPoint, Visio, Publisher, Project, Access, OneNote, and OpenDocument Text.

Shelves such as Books Only, Documents Only, Favourites, Recently Added, Missing Metadata, and No Cover are virtual views. They do not move files.

## Packaging

The GitHub Actions workflow builds the Windows release, runs runtime tests, packages a portable ZIP, optionally builds the NSIS installer, generates checksums, and updates the `v1.0` prerelease assets when release publishing is enabled.
