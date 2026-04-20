#pragma once
// ============================================================
//  Eagle Library -- LanguageManager.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include <QObject>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QVector>

struct LanguageInfo
{
    QString code;
    QString name;
    QString nativeName;
    bool rightToLeft = false;
    bool externalPack = false;
};

class LanguageManager : public QObject
{
    Q_OBJECT
public:
    static LanguageManager& instance();

    void initialize();
    QVector<LanguageInfo> availableLanguages() const;
    QString currentLanguage() const;
    QString text(const QString& key, const QString& fallback = QString()) const;
    bool applyLanguage(const QString& code);
    bool isRightToLeft() const;
    QStringList packDirectories() const;

signals:
    void languageChanged(const QString& code);

private:
    explicit LanguageManager(QObject* parent = nullptr);

    void loadBuiltInPacks();
    void loadExternalPacks();
    bool loadPackFile(const QString& filePath, bool externalPack);

    QHash<QString, QJsonObject> m_translations;
    QHash<QString, LanguageInfo> m_languageInfo;
    QString m_currentLanguage = QStringLiteral("en");
    bool m_initialized = false;
};
