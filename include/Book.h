#pragma once
// ============================================================
//  Eagle Library v2.0 — Book.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QPixmap>
#include <QFileInfo>
#include <QRegularExpression>
#include <QMap>
#include <QVariant>

struct Book
{
    // ── Identity ─────────────────────────────────────────────
    qint64      id          = 0;
    QString     filePath;
    QString     fileHash;           // SHA-256 of first 64 KB
    QString     format;
    qint64      fileSize    = 0;

    // ── Core Metadata ─────────────────────────────────────────
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

    // ── Extended Metadata ─────────────────────────────────────
    QString     series;             // e.g. "The Lord of the Rings"
    int         seriesIndex = 0;    // volume/part number
    QString     edition;
    QStringList categories;         // virtual library collections
    QMap<QString, QVariant> customFields; // user-defined extra columns

    // ── Cover ─────────────────────────────────────────────────
    QString     coverPath;
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
    QString classificationTag() const {
        const QString blob = (title + " " + author + " " + publisher + " " + description + " "
                              + filePath + " " + tags.join(' ') + " " + subjects.join(' ')).toLower();

        int bookScore = 0, documentScore = 0;

        if (!isbn.trimmed().isEmpty()) bookScore += 4;
        if (!author.trimmed().isEmpty()) bookScore += 2;
        if (year > 0) bookScore += 1;
        if (pages >= 80) bookScore += 2;

        static const QStringList bookTokens = {
            "novel", "biography", "fiction", "textbook", "anthology", "poetry",
            "history", "philosophy", "science fiction", "literature", "author"
        };
        for (const QString& t : bookTokens)
            if (blob.contains(t)) ++bookScore;

        static const QStringList documentTokens = {
            "invoice", "report", "manual", "documentation", "specification", "spec ",
            "datasheet", "presentation", "slides", "slide deck", "worksheet",
            "agenda", "minutes", "contract", "proposal", "policy", "resume",
            "curriculum vitae", "brochure", "form", "statement", "whitepaper",
            "standard operating", "meeting", "checklist"
        };
        for (const QString& t : documentTokens)
            if (blob.contains(t)) documentScore += 2;

        if (format.compare("PDF", Qt::CaseInsensitive) == 0 && pages > 0 && pages <= 36)
            ++documentScore;
        static const QStringList officeFormats = {
            "Word", "Excel", "PowerPoint", "PowerPoint Show", "OpenDocument Text",
            "OpenDocument Sheet", "OpenDocument Presentation", "OneNote", "XPS",
            "OpenXPS", "HTML", "XML", "CSV", "Markdown"
        };
        if (officeFormats.contains(format, Qt::CaseInsensitive))
            documentScore += 2;
        if (author.trimmed().isEmpty() && publisher.trimmed().isEmpty())
            ++documentScore;
        if (filePath.contains("/docs/", Qt::CaseInsensitive) || filePath.contains("\\docs\\", Qt::CaseInsensitive))
            ++documentScore;

        return documentScore >= bookScore + 2 ? QStringLiteral("Document") : QStringLiteral("Book");
    }

    bool isLikelyDocument() const { return classificationTag() == QStringLiteral("Document"); }

    QString tagsDisplay() const { return tags.join(", "); }

    QString seriesDisplay() const {
        if (series.isEmpty()) return {};
        return seriesIndex > 0 ? QString("%1 #%2").arg(series).arg(seriesIndex) : series;
    }
};
