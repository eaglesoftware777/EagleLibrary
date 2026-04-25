#pragma once
// ============================================================
//  Eagle Library -- AppConfig.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include <QString>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QStandardPaths>

namespace AppConfig
{
    inline QString appName()   { return QString("Eagle Library"); }
    inline QString company()   { return QString("Eagle Software"); }
    inline QString version()   { return QString("1.1.0"); }
    inline QString website()   { return QString("https://eaglesoftware.biz/"); }
    inline QString copyright() { return QString("Copyright (C) 2026 Eagle Software. All rights reserved."); }

    inline QStringList supportedFormats() {
        return { "*.pdf", "*.epub", "*.mobi", "*.azw", "*.azw3",
                 "*.djvu", "*.fb2", "*.cbz", "*.cbr", "*.txt",
                 "*.rtf", "*.md", "*.csv", "*.doc", "*.docx",
                 "*.docm", "*.docb", "*.dot", "*.dotx", "*.dotm",
                 "*.xls", "*.xlsx", "*.xlsm", "*.xlsb", "*.xlt",
                 "*.xltx", "*.xltm", "*.ppt", "*.pptx", "*.pptm",
                 "*.pot", "*.potx", "*.potm", "*.pps", "*.ppsx", "*.ppsm",
                 "*.sldx", "*.sldm", "*.vsd", "*.vsdx", "*.vsdm",
                 "*.vss", "*.vssx", "*.vssm", "*.vst", "*.vstx", "*.vstm",
                 "*.vdx", "*.pub", "*.mpp", "*.mpt", "*.mdb", "*.accdb",
                 "*.msg", "*.odt",
                 "*.ott", "*.ods", "*.ots", "*.odp", "*.otp",
                 "*.odg", "*.odf", "*.pages", "*.numbers", "*.key",
                 "*.one", "*.xps", "*.oxps", "*.html", "*.htm",
                 "*.xml", "*.chm", "*.lit" };
    }

    inline QString appDir()
    {
        return QCoreApplication::applicationDirPath();
    }

    inline bool isPortableMode()
    {
        const QString baseDir = appDir();
        return QFileInfo::exists(baseDir + "/portable.flag")
            || QFileInfo::exists(baseDir + "/portable.ini");
    }

    inline QString localDataDir()
    {
        return appDir() + "/data";
    }

    inline QString localSettingsDir()
    {
        return appDir() + "/settings";
    }

    inline QString translationsDir()
    {
        return appDir() + "/translations";
    }

    inline QString themesDir()
    {
        return appDir() + "/themes";
    }

    inline QString hooksDir()
    {
        return appDir() + "/hooks";
    }

    inline QString resourcesDir()
    {
        return appDir() + "/resources";
    }

    inline QString legacyDataDir()
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }

    inline QString legacySettingsDir()
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }

    inline QString dataDir()
    {
        return localDataDir();
    }

    inline QString settingsDir()
    {
        return localSettingsDir();
    }

    inline QString settingsPath()
    {
        return settingsDir() + "/EagleLibrary.ini";
    }

    inline QString dbPath()      { return dataDir() + "/library.db"; }
    inline QString coversDir()   { return dataDir() + "/covers"; }
    inline QString thumbsDir()   { return dataDir() + "/thumbs"; }
    inline QString jsonDir()     { return dataDir() + "/json"; }
    inline QString backupsDir()  { return dataDir() + "/backups"; }
    inline QString runtimeDir()  { return dataDir() + "/runtime"; }
    inline QString sessionStatePath() { return runtimeDir() + "/session-state.json"; }
    inline QString pluginsDir()  { return appDir()  + "/plugins"; }
    inline QString sidecarsDir() { return dataDir() + "/sidecars"; }
    inline QString logoPngPath() { return resourcesDir() + "/eagle_logo.png"; }
    inline QString markSvgPath() { return resourcesDir() + "/eagle_mark.svg"; }
    inline QString normalizedPath(const QString& path) { return QDir::cleanPath(QDir::fromNativeSeparators(path)); }
    inline bool isManagedDataPath(const QString& path) {
        const QString normalized = normalizedPath(path);
        const QString data = normalizedPath(dataDir());
        return normalized == data || normalized.startsWith(data + "/");
    }
    inline bool isManagedCoverPath(const QString& path) {
        const QString normalized = normalizedPath(path);
        const QString covers = normalizedPath(coversDir());
        return normalized == covers || normalized.startsWith(covers + "/");
    }

    inline QString googleBooksUrl(const QString& query) {
        return QString("https://www.googleapis.com/books/v1/volumes?q=%1&maxResults=3").arg(query);
    }

    inline QString openLibrarySearchUrl(const QString& query) {
        return QString("https://openlibrary.org/search.json?q=%1&limit=3").arg(query);
    }

    inline QString openLibraryCoverUrl(const QString& key, const QString& value, const QString& size = "L") {
        return QString("https://covers.openlibrary.org/b/%1/%2-%3.jpg?default=false")
            .arg(key, value, size);
    }
}
