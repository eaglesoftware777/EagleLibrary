// ============================================================
//  Eagle Library v2.0 — ThemeManager.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "ThemeManager.h"
#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QDebug>

ThemeManager& ThemeManager::instance()
{
    static ThemeManager inst;
    return inst;
}

void ThemeManager::applyTheme(Theme t)
{
    m_current = t;
    static const char* paths[] = {
        ":/themes/blue.qss",
        ":/themes/white.qss",
        ":/themes/mac.qss"
    };
    QFile f(paths[static_cast<int>(t)]);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
        f.close();
    } else {
        qWarning() << "ThemeManager: could not load theme" << paths[static_cast<int>(t)];
    }
    QSettings s;
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
