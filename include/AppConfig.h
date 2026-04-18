#pragma once
// ============================================================
//  Eagle Library -- AppConfig.h
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include <QString>
#include <QDir>
#include <QStandardPaths>

namespace AppConfig
{
    inline QString appName()   { return QString("Eagle Library"); }
    inline QString company()   { return QString("Eagle Software"); }
    inline QString version()   { return QString("1.0.0"); }
    inline QString website()   { return QString("https://eaglesoftware.biz/"); }
    inline QString copyright() { return QString("Copyright (C) 2024 Eagle Software. All rights reserved."); }

    inline QStringList supportedFormats() {
        return { "*.pdf", "*.epub", "*.mobi", "*.azw", "*.azw3",
                 "*.djvu", "*.fb2", "*.cbz", "*.cbr", "*.txt",
                 "*.rtf", "*.doc", "*.docx", "*.chm", "*.lit" };
    }

    inline QString dataDir()    { return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation); }
    inline QString dbPath()     { return dataDir() + "/library.db"; }
    inline QString coversDir()  { return dataDir() + "/covers"; }
    inline QString thumbsDir()  { return dataDir() + "/thumbs"; }

    inline QString googleBooksUrl(const QString& query) {
        return QString("https://www.googleapis.com/books/v1/volumes?q=%1&maxResults=1").arg(query);
    }
}
