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
    QString     readingStatus;      // Want to Read / Reading / Read
    int         progressPercent = 0;
    int         currentPage = 0;
    QDateTime   dateStarted;
    QDateTime   dateFinished;
    QDateTime   lastReadingSession;
    int         readingMinutes = 0;
    QString     loanedTo;
    QDateTime   loanDueDate;

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

        int bookScore = 0, paperScore = 0, slideScore = 0, manualScore = 0, documentScore = 0;

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

        static const QStringList paperTokens = {
            "abstract", "introduction", "methodology", "methods", "results",
            "discussion", "conclusion", "references", "journal", "conference",
            "proceedings", "doi", "issn", "et al", "research paper", "preprint"
        };
        for (const QString& t : paperTokens)
            if (blob.contains(t)) paperScore += 2;

        static const QStringList slideTokens = {
            "slides", "slide deck", "powerpoint", "keynote", "presentation",
            "agenda", "speaker notes", "lecture slides", "seminar"
        };
        for (const QString& t : slideTokens)
            if (blob.contains(t)) slideScore += 2;

        static const QStringList manualTokens = {
            "manual", "user guide", "installation guide", "service manual",
            "reference manual", "operator", "datasheet", "handbook", "specification"
        };
        for (const QString& t : manualTokens)
            if (blob.contains(t)) manualScore += 2;

        static const QStringList documentTokens = {
            "invoice", "report", "manual", "documentation", "specification", "spec ",
            "datasheet", "presentation", "slides", "slide deck", "worksheet",
            "agenda", "minutes", "contract", "proposal", "policy", "resume",
            "curriculum vitae", "brochure", "form", "statement", "whitepaper",
            "standard operating", "meeting", "checklist"
        };
        for (const QString& t : documentTokens)
            if (blob.contains(t)) documentScore += 2;

        if (blob.contains("arxiv") || blob.contains("doi:") || blob.contains("issn"))
            paperScore += 3;
        if (format.compare("PDF", Qt::CaseInsensitive) == 0 && pages > 0 && pages <= 36)
            ++documentScore;
        if (format.compare("PDF", Qt::CaseInsensitive) == 0 && pages >= 4 && pages <= 40)
            slideScore += 1;
        if (format.compare("PDF", Qt::CaseInsensitive) == 0 && pages >= 6 && pages <= 80)
            paperScore += 1;
        static const QStringList officeFormats = {
            "Word", "Word Template", "Excel", "Excel Template", "PowerPoint",
            "PowerPoint Template", "PowerPoint Show", "PowerPoint Slide",
            "Visio", "Visio Stencil", "Visio Template", "Publisher", "Project",
            "Project Template", "Access", "Outlook Message", "OpenDocument Text",
            "OpenDocument Template", "OpenDocument Sheet", "OpenDocument Sheet Template",
            "OpenDocument Presentation", "OpenDocument Presentation Template",
            "OpenDocument Graphic", "OpenDocument Formula", "OneNote", "XPS",
            "OpenXPS", "HTML", "XML", "CSV", "Markdown"
        };
        if (officeFormats.contains(format, Qt::CaseInsensitive))
            documentScore += 2;
        if (format.contains("PowerPoint", Qt::CaseInsensitive) || format.compare("Keynote", Qt::CaseInsensitive) == 0)
            slideScore += 4;
        if (author.trimmed().isEmpty() && publisher.trimmed().isEmpty())
            ++documentScore;
        if (filePath.contains("/docs/", Qt::CaseInsensitive) || filePath.contains("\\docs\\", Qt::CaseInsensitive))
            ++documentScore;
        if (filePath.contains("/papers/", Qt::CaseInsensitive) || filePath.contains("\\papers\\", Qt::CaseInsensitive))
            paperScore += 2;
        if (filePath.contains("/slides/", Qt::CaseInsensitive) || filePath.contains("\\slides\\", Qt::CaseInsensitive))
            slideScore += 3;
        if (filePath.contains("/manual", Qt::CaseInsensitive) || filePath.contains("\\manual", Qt::CaseInsensitive))
            manualScore += 2;

        const int bestDocumentScore = qMax(qMax(paperScore, slideScore), qMax(manualScore, documentScore));
        if (bookScore >= bestDocumentScore + 1)
            return QStringLiteral("Book");
        if (paperScore >= qMax(slideScore, qMax(manualScore, documentScore)))
            return QStringLiteral("Research Paper");
        if (slideScore >= qMax(paperScore, qMax(manualScore, documentScore)))
            return QStringLiteral("Slide Deck");
        if (manualScore >= qMax(paperScore, qMax(slideScore, documentScore)))
            return QStringLiteral("Manual");
        return QStringLiteral("Document");
    }

    bool isLikelyDocument() const { return classificationTag() != QStringLiteral("Book"); }

    QString tagsDisplay() const { return tags.join(", "); }

    QString seriesDisplay() const {
        if (series.isEmpty()) return {};
        return seriesIndex > 0 ? QString("%1 #%2").arg(series).arg(seriesIndex) : series;
    }
};
