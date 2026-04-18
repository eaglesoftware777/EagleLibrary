// ============================================================
//  Eagle Library -- main.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QThread>
#include <QSettings>
#include <QTimer>
#include <QMessageBox>
#include <QIcon>

#include "SplashScreen.h"
#include "MainWindow.h"
#include "Database.h"
#include "AppConfig.h"

int main(int argc, char* argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("Eagle Library");
    app.setOrganizationName("Eagle Software");
    app.setApplicationVersion("1.0.0");

    QIcon appIcon(":/eagle_logo.png");
    if (!appIcon.isNull())
        app.setWindowIcon(appIcon);

    // Splash screen
    SplashScreen* splash = new SplashScreen;
    splash->show();
    app.processEvents();

    auto step = [&](int pct, const QString& msg) {
        splash->setProgress(pct, msg);
        app.processEvents();
        QThread::msleep(60);
    };

    step(5,  "Initializing Eagle Library...");
    step(15, "Preparing data directories...");

    QDir().mkpath(AppConfig::dataDir());
    QDir().mkpath(AppConfig::coversDir());
    QDir().mkpath(AppConfig::thumbsDir());

    step(35, "Opening library database...");
    Database::instance().open(AppConfig::dbPath());

    step(60, "Loading book catalogue...");
    step(80, "Building user interface...");

    MainWindow* mainWin = new MainWindow;

    step(95, "Finalizing...");
    app.processEvents();
    QThread::msleep(200);

    step(100, "Ready.");
    app.processEvents();
    QThread::msleep(300);

    mainWin->show();
    splash->finish(mainWin);
    splash->deleteLater();

    // First-run welcome
    QSettings s("Eagle Software", "Eagle Library");
    if (!s.contains("library/folders")) {
        QTimer::singleShot(500, mainWin, [mainWin]() {
            QMessageBox::information(mainWin,
                "Welcome to Eagle Library",
                "<h3>Welcome!</h3>"
                "<p>To get started, go to <b>Settings</b> and add the folders "
                "where your eBooks are stored.<br><br>"
                "Eagle Library will scan them automatically and keep your books "
                "organised without moving any files.</p>");
        });
    }

    return app.exec();
}
