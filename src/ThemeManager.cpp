// ============================================================
//  Eagle Library v2.0 — ThemeManager.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "ThemeManager.h"
#include "AppConfig.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStyle>
#include <QWidget>
#include <QDebug>

ThemeManager& ThemeManager::instance()
{
    static ThemeManager inst;
    return inst;
}

namespace {

QString themeSourcePath(ThemeManager::Theme t)
{
    static const char* resourcePaths[] = {
        ":/themes/blue.qss",
        ":/themes/white.qss",
        ":/themes/mac.qss"
    };
    static const char* fileNames[] = {
        "blue.qss",
        "white.qss",
        "mac.qss"
    };

    const QString externalPath = QDir(AppConfig::themesDir()).absoluteFilePath(QString::fromUtf8(fileNames[static_cast<int>(t)]));
    if (QFileInfo::exists(externalPath))
        return externalPath;
    return QString::fromUtf8(resourcePaths[static_cast<int>(t)]);
}

}

void ThemeManager::applyTheme(Theme t)
{
    m_current = t;
    const QString themePath = themeSourcePath(t);
    QFile f(themePath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qApp->setStyleSheet(QString());
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
        for (QWidget* widget : qApp->allWidgets()) {
            if (!widget)
                continue;
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
            widget->update();
        }
        f.close();
    } else {
        qWarning() << "ThemeManager: could not load theme" << themePath;
    }
    QSettings s(AppConfig::settingsPath(), QSettings::IniFormat);
    s.setValue("ui/theme", themeName(t));
    emit themeChanged(t);
}

void ThemeManager::applyTheme(const QString& name)
{
    applyTheme(themeFromName(name));
}

QString ThemeManager::currentThemeName() const
{
    return themeName(m_current);
}

QStringList ThemeManager::availableThemes()
{
    return { "Blue Pro", "Pure White", "macOS" };
}

QString ThemeManager::themeName(Theme t)
{
    switch (t) {
    case BluePro:   return "Blue Pro";
    case PureWhite: return "Pure White";
    case MacOS:     return "macOS";
    }
    return "Blue Pro";
}

ThemeManager::Theme ThemeManager::themeFromName(const QString& name)
{
    if (name == "Pure White") return PureWhite;
    if (name == "macOS")      return MacOS;
    return BluePro;
}
