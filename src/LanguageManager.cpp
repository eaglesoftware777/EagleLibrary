// ============================================================
//  Eagle Library -- LanguageManager.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "LanguageManager.h"
#include "AppConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QResource>
#include <QSettings>
#include <algorithm>

namespace {

QString normalizeCode(QString code)
{
    code = code.trimmed().toLower();
    return code.isEmpty() ? QStringLiteral("en") : code;
}

}

LanguageManager& LanguageManager::instance()
{
    static LanguageManager manager;
    return manager;
}

LanguageManager::LanguageManager(QObject* parent)
    : QObject(parent)
{
}

void LanguageManager::initialize()
{
    if (m_initialized)
        return;

    m_initialized = true;
    loadBuiltInPacks();
    loadExternalPacks();

    QSettings settings(AppConfig::settingsPath(), QSettings::IniFormat);
    const QString configuredCode = normalizeCode(settings.value(QStringLiteral("ui/language"), QStringLiteral("en")).toString());
    if (!applyLanguage(configuredCode))
        applyLanguage(QStringLiteral("en"));
}

void LanguageManager::loadBuiltInPacks()
{
    // English is always available as the built-in default, even without a JSON pack.
    if (!m_languageInfo.contains(QStringLiteral("en"))) {
        LanguageInfo en;
        en.code        = QStringLiteral("en");
        en.name        = QStringLiteral("English");
        en.nativeName  = QStringLiteral("English");
        en.externalPack = false;
        m_languageInfo.insert(QStringLiteral("en"), en);
        m_translations.insert(QStringLiteral("en"), QJsonObject());
    }

    const QDir resourceDir(QStringLiteral(":/translations"));
    const QStringList files = resourceDir.entryList(QStringList() << QStringLiteral("*.json"), QDir::Files, QDir::Name);
    for (const QString& file : files)
        loadPackFile(resourceDir.absoluteFilePath(file), false);
}

void LanguageManager::loadExternalPacks()
{
    for (const QString& baseDir : packDirectories()) {
        const QDir dir(baseDir);
        if (!dir.exists())
            continue;
        const QStringList files = dir.entryList(QStringList() << QStringLiteral("*.json"), QDir::Files, QDir::Name);
        for (const QString& file : files)
            loadPackFile(dir.absoluteFilePath(file), true);
    }
}

bool LanguageManager::loadPackFile(const QString& filePath, bool externalPack)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    const QJsonObject root = doc.object();
    const QString code = normalizeCode(root.value(QStringLiteral("code")).toString());
    const QJsonObject translations = root.value(QStringLiteral("translations")).toObject();
    if (code.isEmpty() || translations.isEmpty())
        return false;

    LanguageInfo info;
    info.code = code;
    info.name = root.value(QStringLiteral("name")).toString(code.toUpper());
    info.nativeName = root.value(QStringLiteral("nativeName")).toString(info.name);
    info.rightToLeft = root.value(QStringLiteral("rtl")).toBool(false);
    info.externalPack = externalPack;

    m_translations.insert(code, translations);
    m_languageInfo.insert(code, info);
    return true;
}

QVector<LanguageInfo> LanguageManager::availableLanguages() const
{
    QVector<LanguageInfo> items = m_languageInfo.values().toVector();
    std::sort(items.begin(), items.end(), [](const LanguageInfo& left, const LanguageInfo& right) {
        if (left.code == QStringLiteral("en"))
            return true;
        if (right.code == QStringLiteral("en"))
            return false;
        return QString::localeAwareCompare(left.nativeName, right.nativeName) < 0;
    });
    return items;
}

QString LanguageManager::currentLanguage() const
{
    return m_currentLanguage;
}

QString LanguageManager::text(const QString& key, const QString& fallback) const
{
    const auto lookup = [&](const QString& code) -> QString {
        const auto it = m_translations.constFind(code);
        if (it == m_translations.constEnd())
            return {};
        return it->value(key).toString();
    };

    const QString currentValue = lookup(m_currentLanguage);
    if (!currentValue.isEmpty())
        return currentValue;

    const QString englishValue = lookup(QStringLiteral("en"));
    if (!englishValue.isEmpty())
        return englishValue;

    return fallback;
}

bool LanguageManager::applyLanguage(const QString& code)
{
    const QString normalizedCode = normalizeCode(code);
    if (!m_translations.contains(normalizedCode))
        return false;
    if (m_currentLanguage == normalizedCode)
        return true;

    m_currentLanguage = normalizedCode;
    emit languageChanged(m_currentLanguage);
    return true;
}

bool LanguageManager::isRightToLeft() const
{
    return m_languageInfo.value(m_currentLanguage).rightToLeft;
}

QStringList LanguageManager::packDirectories() const
{
    QStringList dirs;
    dirs << AppConfig::translationsDir();
    return dirs;
}
