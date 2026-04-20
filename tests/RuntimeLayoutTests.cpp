#include <QtTest/QtTest>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

#include "AppConfig.h"
#include "LanguageManager.h"
#include "ThemeManager.h"

class RuntimeLayoutTests : public QObject
{
    Q_OBJECT

private slots:
    void pathsStayInsideAppFolder();
    void settingsWriteToSingleAppIni();
    void languagePacksResolveToAppFolder();
    void externalThemeOverridesBundledTheme();
};

void RuntimeLayoutTests::pathsStayInsideAppFolder()
{
    const QString appDir = AppConfig::appDir();
    QVERIFY(AppConfig::settingsPath().startsWith(appDir));
    QVERIFY(AppConfig::dbPath().startsWith(appDir));
    QVERIFY(AppConfig::pluginsDir().startsWith(appDir));
    QVERIFY(AppConfig::translationsDir().startsWith(appDir));
    QVERIFY(AppConfig::themesDir().startsWith(appDir));
    QVERIFY(AppConfig::hooksDir().startsWith(appDir));
    QVERIFY(AppConfig::resourcesDir().startsWith(appDir));
}

void RuntimeLayoutTests::settingsWriteToSingleAppIni()
{
    QDir().mkpath(AppConfig::appDir());

    QSettings settings(AppConfig::settingsPath(), QSettings::IniFormat);
    settings.setValue("tests/runtimeLayout", "ok");
    settings.sync();

    QVERIFY(QFileInfo::exists(AppConfig::settingsPath()));
    QCOMPARE(QFileInfo(AppConfig::settingsPath()).absolutePath(), AppConfig::appDir());
    QVERIFY(!AppConfig::settingsPath().startsWith(AppConfig::settingsDir() + "/"));

    settings.remove("tests/runtimeLayout");
    settings.sync();
}

void RuntimeLayoutTests::languagePacksResolveToAppFolder()
{
    const QStringList packDirs = LanguageManager::instance().packDirectories();
    QCOMPARE(packDirs.size(), 1);
    QCOMPARE(packDirs.first(), AppConfig::translationsDir());
}

void RuntimeLayoutTests::externalThemeOverridesBundledTheme()
{
    QDir().mkpath(AppConfig::themesDir());
    const QString themePath = QDir(AppConfig::themesDir()).absoluteFilePath("blue.qss");

    QFile themeFile(themePath);
    QVERIFY(themeFile.open(QIODevice::WriteOnly | QIODevice::Text));
    themeFile.write("QWidget { background: rgb(1, 2, 3); }");
    themeFile.close();

    ThemeManager::instance().applyTheme(ThemeManager::BluePro);
    QVERIFY(qApp->styleSheet().contains("rgb(1, 2, 3)"));

    QFile::remove(themePath);
}

QTEST_MAIN(RuntimeLayoutTests)
#include "RuntimeLayoutTests.moc"
