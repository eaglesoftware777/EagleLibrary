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
#include <QCommandLineParser>
#include <QTextStream>
#include <QEventLoop>
#include <QFileInfo>
#include <QFile>

#include "SplashScreen.h"
#include "MainWindow.h"
#include "Database.h"
#include "AppConfig.h"
#include "LanguageManager.h"
#include "LibraryScanner.h"

namespace {

bool copyDirectoryTree(const QString& sourcePath, const QString& targetPath)
{
    const QDir sourceDir(sourcePath);
    if (!sourceDir.exists())
        return false;

    QDir targetDir;
    targetDir.mkpath(targetPath);

    const QFileInfoList entries = sourceDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    bool copiedAny = false;
    for (const QFileInfo& entry : entries) {
        const QString targetEntryPath = QDir(targetPath).absoluteFilePath(entry.fileName());
        if (entry.isDir()) {
            copiedAny = copyDirectoryTree(entry.absoluteFilePath(), targetEntryPath) || copiedAny;
            continue;
        }

        if (QFileInfo::exists(targetEntryPath))
            continue;

        QDir().mkpath(QFileInfo(targetEntryPath).absolutePath());
        if (QFile::copy(entry.absoluteFilePath(), targetEntryPath))
            copiedAny = true;
    }
    return copiedAny;
}

void migrateLegacyStorage()
{
    QDir().mkpath(AppConfig::dataDir());
    QDir().mkpath(AppConfig::settingsDir());

    const QString newDbPath = AppConfig::dbPath();
    const QString legacyDbPath = AppConfig::legacyDataDir() + "/library.db";
    if (!QFileInfo::exists(newDbPath) && QFileInfo::exists(legacyDbPath)) {
        QDir().mkpath(QFileInfo(newDbPath).absolutePath());
        QFile::copy(legacyDbPath, newDbPath);
    }

    const QStringList dataSubdirs = { "covers", "thumbs", "json", "sidecars" };
    for (const QString& subdir : dataSubdirs) {
        copyDirectoryTree(QDir(AppConfig::legacyDataDir()).absoluteFilePath(subdir),
                          QDir(AppConfig::dataDir()).absoluteFilePath(subdir));
    }

    const QString legacyIniPath = AppConfig::legacySettingsDir() + "/EagleLibrary.ini";
    if (!QFileInfo::exists(AppConfig::settingsPath()) && QFileInfo::exists(legacyIniPath)) {
        QDir().mkpath(QFileInfo(AppConfig::settingsPath()).absolutePath());
        QFile::copy(legacyIniPath, AppConfig::settingsPath());
    }

    copyDirectoryTree(QDir(AppConfig::legacySettingsDir()).absoluteFilePath("translations"),
                      QDir(AppConfig::settingsDir()).absoluteFilePath("translations"));
    copyDirectoryTree(QDir(AppConfig::legacySettingsDir()).absoluteFilePath("hooks"),
                      QDir(AppConfig::settingsDir()).absoluteFilePath("hooks"));
}

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
    QDir().mkpath(AppConfig::settingsDir());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, AppConfig::settingsDir());
}

int runCliCommand(QApplication& app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Eagle Library CLI");
    parser.addHelpOption();
    parser.addPositionalArgument("command", "Command: search | saved-searches | export | optimize | scan");
    parser.addPositionalArgument("args", "Arguments for the selected command.");
    parser.process(app);

    const QStringList positional = parser.positionalArguments();
    if (positional.isEmpty())
        return -1;

    QDir().mkpath(AppConfig::dataDir());
    QDir().mkpath(AppConfig::settingsDir());
    Database::instance().open(AppConfig::dbPath());

    QTextStream out(stdout);
    QTextStream err(stderr);
    const QString command = positional.first().trimmed().toLower();

    if (command == "search") {
        if (positional.size() < 2) {
            err << "Usage: EagleLibrary.exe search <query>\n";
            return 2;
        }
        const QString query = positional.mid(1).join(' ');
        const QVector<Book> books = Database::instance().search(query);
        for (const Book& book : books)
            out << book.displayTitle() << " | " << book.displayAuthor() << " | " << QDir::toNativeSeparators(book.filePath) << "\n";
        return 0;
    }

    if (command == "saved-searches") {
        const QVector<SavedSearch> searches = Database::instance().savedSearches();
        for (const SavedSearch& search : searches)
            out << search.id << " | " << search.name << " | " << search.query << "\n";
        return 0;
    }

    if (command == "optimize") {
        Database::instance().optimize();
        out << "Database optimized.\n";
        return 0;
    }

    if (command == "export") {
        if (positional.size() < 2) {
            err << "Usage: EagleLibrary.exe export <output.json>\n";
            return 2;
        }
        QSettings s("Eagle Software", "Eagle Library");
        const QStringList folders = s.value("library/folders").toStringList();
        if (!Database::instance().exportLibrary(positional.at(1), folders)) {
            err << "Export failed.\n";
            return 1;
        }
        out << "Exported library to " << positional.at(1) << "\n";
        return 0;
    }

    if (command == "scan") {
        if (positional.size() < 2) {
            err << "Usage: EagleLibrary.exe scan <folder1> [folder2 ...]\n";
            return 2;
        }
        LibraryScanner scanner;
        QEventLoop loop;
        QObject::connect(&scanner, &LibraryScanner::scanFinished, &app, [&](int added, int skipped) {
            out << "Scan complete. Added: " << added << ", skipped: " << skipped << "\n";
            loop.quit();
        });
        QObject::connect(&scanner, &LibraryScanner::scanError, &app, [&](const QString& msg) {
            err << "Scan error: " << msg << "\n";
            loop.quit();
        });
        scanner.startScan(positional.mid(1), 0, true);
        loop.exec();
        return 0;
    }

    err << "Unknown command: " << command << "\n";
    return 2;
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
    app.setApplicationVersion(AppConfig::version());
    app.setFont(buildUiFont());

    configureSettingsStorage();
    migrateLegacyStorage();
    LanguageManager::instance().initialize();
    app.setLayoutDirection(LanguageManager::instance().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    QObject::connect(&LanguageManager::instance(), &LanguageManager::languageChanged, &app, [&app]() {
        app.setLayoutDirection(LanguageManager::instance().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    });

    if (app.arguments().size() > 1) {
        const int cliResult = runCliCommand(app);
        if (cliResult >= 0)
            return cliResult;
    }

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
