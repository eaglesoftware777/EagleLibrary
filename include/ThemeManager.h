#pragma once
// ============================================================
//  Eagle Library v2.0 — ThemeManager.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include <QObject>
#include <QString>
#include <QStringList>

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum Theme { BluePro = 0, PureWhite = 1, MacOS = 2 };

    static ThemeManager& instance();

    void    applyTheme(Theme t);
    void    applyTheme(const QString& name);
    Theme   currentTheme() const { return m_current; }
    QString currentThemeName() const;

    static QStringList availableThemes();
    static QString     themeName(Theme t);
    static Theme       themeFromName(const QString& name);

signals:
    void themeChanged(Theme newTheme);

private:
    ThemeManager() = default;
    Theme   m_current = BluePro;
};
