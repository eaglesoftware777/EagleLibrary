#pragma once
// ============================================================
//  Eagle Library — Book.h
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QPixmap>
#include <QFileInfo>

struct Book
{
    // ── Identity ─────────────────────────────────────────────
    qint64      id          = 0;
    QString     filePath;           // original file path (not moved)
    QString     fileHash;           // SHA-256 of first 64 KB
    QString     format;             // PDF, EPUB, MOBI, etc.
    qint64      fileSize    = 0;    // bytes

    // ── Metadata ─────────────────────────────────────────────
    QString     title;
    QString     author;
    QString     publisher;
    QString     isbn;
    QString     language;
    int         year        = 0;
    int         pages       = 0;
    double      rating      = 0.0;
    QString     description;
    QStringList tags;
    QStringList subjects;

    // ── Cover ─────────────────────────────────────────────────
    QString     coverPath;          // cached thumbnail path
    bool        hasCover    = false;

    // ── Housekeeping ──────────────────────────────────────────
    QDateTime   dateAdded;
    QDateTime   dateModified;
    QDateTime   lastOpened;
    int         openCount   = 0;
    bool        isFavourite = false;
    QString     notes;

    // ── Helpers ───────────────────────────────────────────────
    QString displayTitle() const {
        return title.isEmpty() ? QFileInfo(filePath).baseName() : title;
    }
    QString displayAuthor() const {
        return author.isEmpty() ? QObject::tr("Unknown Author") : author;
    }
};
