// ============================================================
//  Eagle Library -- main.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QThread>
#include <QSettings>
#include <QTimer>
#include <QMessageBox>
#include <QIcon>
#include <QFont>
#include <QFontDatabase>
#include <QElapsedTimer>

#include "SplashScreen.h"
#include "MainWindow.h"
#include "Database.h"
#include "AppConfig.h"
#include "LanguageManager.h"

namespace {

QFont buildUiFont()
{
    const QStringList preferredFamilies = {
        "Segoe UI Emoji",
        "Segoe UI Variable Text",
        "Segoe UI",
        "Aptos",
        "Nirmala UI",
        "Leelawadee UI",
        "Tahoma",
        "Arial Unicode MS",
        "Segoe UI Symbol",
        "Noto Sans CJK SC",
        "Noto Sans CJK TC",
        "Noto Sans CJK JP",
        "Noto Sans CJK KR",
        "Microsoft YaHei UI",
        "Microsoft YaHei",
        "Yu Gothic UI",
        "Meiryo UI",
        "Meiryo",
        "Malgun Gothic",
        "DejaVu Sans"
    };

    const QFontDatabase db;
    for (const QString& family : preferredFamilies) {
        if (db.families().contains(family)) {
            QFont font(family, 10);
            font.setStyleStrategy(QFont::PreferAntialias);
            font.setHintingPreference(QFont::PreferFullHinting);
            font.setStyleHint(QFont::SansSerif, QFont::PreferAntialias);
            return font;
        }
    }

    QFont font;
    font.setPointSize(10);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferFullHinting);
    font.setStyleHint(QFont::SansSerif, QFont::PreferAntialias);
    return font;
}

void configureSettingsStorage()
{
    if (!AppConfig::isPortableMode())
        return;

    QDir().mkpath(AppConfig::settingsDir());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, AppConfig::settingsDir());
}

} // namespace

int main(int argc, char* argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("Eagle Library");
    app.setOrganizationName("Eagle Software");
    app.setApplicationVersion("1.1.0");
    app.setFont(buildUiFont());

    configureSettingsStorage();
    LanguageManager::instance().initialize();
    app.setLayoutDirection(LanguageManager::instance().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    QObject::connect(&LanguageManager::instance(), &LanguageManager::languageChanged, &app, [&app]() {
        app.setLayoutDirection(LanguageManager::instance().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    });

    QIcon appIcon(":/eagle_mark.svg");
    if (appIcon.isNull())
        appIcon = QIcon(":/eagle_logo.png");
    if (!appIcon.isNull())
        app.setWindowIcon(appIcon);

    // Splash screen
    QElapsedTimer splashTimer;
    splashTimer.start();
    SplashScreen* splash = new SplashScreen;
    splash->show();
    app.processEvents();

    auto step = [&](int pct, const QString& msg) {
        splash->setProgress(pct, msg);
        app.processEvents();
        QThread::msleep(150);
    };

    step(5,  LanguageManager::instance().text("splash.initializing", "Initializing Eagle Library..."));
    step(15, LanguageManager::instance().text("splash.preparing", "Preparing data directories..."));

    QDir().mkpath(AppConfig::dataDir());
    QDir().mkpath(AppConfig::coversDir());
    QDir().mkpath(AppConfig::thumbsDir());
    QDir().mkpath(AppConfig::jsonDir());
    for (const QString& translationsDir : LanguageManager::instance().packDirectories())
        QDir().mkpath(translationsDir);
    if (AppConfig::isPortableMode())
        QDir().mkpath(AppConfig::settingsDir());

    step(35, LanguageManager::instance().text("splash.database", "Opening library database..."));
    Database::instance().open(AppConfig::dbPath());

    step(60, LanguageManager::instance().text("splash.catalogue", "Loading book catalogue..."));
    step(80, LanguageManager::instance().text("splash.interface", "Building user interface..."));

    MainWindow* mainWin = new MainWindow;

    step(95, LanguageManager::instance().text("splash.finalizing", "Finalizing..."));
    app.processEvents();
    QThread::msleep(450);

    step(100, LanguageManager::instance().text("splash.ready", "Ready."));
    app.processEvents();
    const qint64 minimumSplashMs = 7000;
    const qint64 elapsedMs = splashTimer.elapsed();
    if (elapsedMs < minimumSplashMs)
        QThread::msleep(static_cast<unsigned long>(minimumSplashMs - elapsedMs));

    mainWin->show();
    splash->finish(mainWin);
    splash->deleteLater();

    // First-run welcome
    QSettings s("Eagle Software", "Eagle Library");
    if (!s.contains("library/folders")) {
        QTimer::singleShot(500, mainWin, [mainWin]() {
            QMessageBox::information(mainWin,
                LanguageManager::instance().text("message.welcome.title", "Welcome to Eagle Library"),
                LanguageManager::instance().text("message.welcome.body",
                    "<h3>Welcome!</h3><p>To get started, go to <b>Settings</b> and add the folders where your eBooks are stored.<br><br>Eagle Library will scan them automatically and keep your books organised without moving any files.</p>"));
        });
    }

    return app.exec();
}
