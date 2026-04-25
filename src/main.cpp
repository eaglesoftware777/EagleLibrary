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
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QSysInfo>

#include "SplashScreen.h"
#include "MainWindow.h"
#include "Database.h"
#include "AppConfig.h"
#include "LanguageManager.h"
#include "LibraryScanner.h"

namespace {

QMutex g_logMutex;
QFile* g_logFile = nullptr;
QString g_logPath;

void writeDiagnosticLine(const QString& level, const QString& message)
{
    if (!g_logFile || !g_logFile->isOpen())
        return;

    QMutexLocker locker(&g_logMutex);
    QTextStream out(g_logFile);
    out << QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
        << " [" << level << "] " << message << '\n';
    out.flush();
    g_logFile->flush();
}

void writeDiagnosticLineUnlocked(const QString& level, const QString& message)
{
    if (!g_logFile || !g_logFile->isOpen())
        return;

    QTextStream out(g_logFile);
    out << QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
        << " [" << level << "] " << message << '\n';
    out.flush();
    g_logFile->flush();
}

void releaseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (!g_logFile) {
        if (type == QtFatalMsg)
            abort();
        return;
    }

    const char* level = "INFO";
    switch (type) {
    case QtDebugMsg: level = "DEBUG"; break;
    case QtInfoMsg: level = "INFO"; break;
    case QtWarningMsg: level = "WARN"; break;
    case QtCriticalMsg: level = "ERROR"; break;
    case QtFatalMsg: level = "FATAL"; break;
    }

    QString location;
    if (context.file)
        location = QStringLiteral(" (%1:%2 %3)").arg(context.file).arg(context.line).arg(context.function ? context.function : "");

    writeDiagnosticLine(QString::fromLatin1(level), message + location);

    if (type == QtFatalMsg)
        abort();
}

void configureDiagnostics()
{
    const QString logDir = QDir(AppConfig::appDir()).absoluteFilePath(QStringLiteral("diagnostic-logs"));
    QDir().mkpath(logDir);
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    g_logPath = QDir(logDir).absoluteFilePath(QStringLiteral("eagle-library-%1-p%2.log")
                                                  .arg(stamp)
                                                  .arg(QCoreApplication::applicationPid()));
    g_logFile = new QFile(g_logPath);
    if (!g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete g_logFile;
        g_logFile = nullptr;
    }

    writeDiagnosticLineUnlocked(QStringLiteral("INFO"), QStringLiteral("[Diagnostics] Log file opened"));
    writeDiagnosticLineUnlocked(QStringLiteral("INFO"), QStringLiteral("[Diagnostics] Log file: %1").arg(QDir::toNativeSeparators(g_logPath)));
    writeDiagnosticLineUnlocked(QStringLiteral("INFO"), QStringLiteral("[Diagnostics] App dir: %1").arg(QDir::toNativeSeparators(AppConfig::appDir())));
    writeDiagnosticLineUnlocked(QStringLiteral("INFO"), QStringLiteral("[Diagnostics] Portable mode: %1").arg(AppConfig::isPortableMode()));
    writeDiagnosticLineUnlocked(QStringLiteral("INFO"), QStringLiteral("[Diagnostics] App version: %1").arg(AppConfig::version()));
    writeDiagnosticLineUnlocked(QStringLiteral("INFO"), QStringLiteral("[Diagnostics] Qt: %1").arg(qVersion()));
    writeDiagnosticLineUnlocked(QStringLiteral("INFO"), QStringLiteral("[Diagnostics] OS: %1").arg(QSysInfo::prettyProductName()));

    qInstallMessageHandler(releaseMessageHandler);

    qInfo().noquote() << "[Diagnostics] Temporary portable test logging enabled";
    qInfo().noquote() << "[Diagnostics] Log file:" << QDir::toNativeSeparators(g_logPath);
    qInfo().noquote() << "[Diagnostics] App dir:" << QDir::toNativeSeparators(AppConfig::appDir());
    qInfo().noquote() << "[Diagnostics] Portable mode:" << AppConfig::isPortableMode();
    qInfo().noquote() << "[Diagnostics] App version:" << AppConfig::version();
    qInfo().noquote() << "[Diagnostics] Qt:" << qVersion();
    qInfo().noquote() << "[Diagnostics] OS:" << QSysInfo::prettyProductName();
}

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

bool copyFileIfMissing(const QString& sourcePath, const QString& targetPath)
{
    if (!sourcePath.startsWith(":/") && !QFileInfo::exists(sourcePath))
        return false;
    if (QFileInfo::exists(targetPath))
        return false;

    QDir().mkpath(QFileInfo(targetPath).absolutePath());
    return QFile::copy(sourcePath, targetPath);
}

void seedBundledRuntimeAssets()
{
    QDir().mkpath(AppConfig::translationsDir());
    QDir().mkpath(AppConfig::themesDir());
    QDir().mkpath(AppConfig::hooksDir());
    QDir().mkpath(AppConfig::resourcesDir());
    QDir().mkpath(AppConfig::runtimeDir());

    const QDir translationsResourceDir(":/translations");
    for (const QString& fileName : translationsResourceDir.entryList(QStringList() << "*.json", QDir::Files, QDir::Name))
        copyFileIfMissing(translationsResourceDir.absoluteFilePath(fileName),
                          QDir(AppConfig::translationsDir()).absoluteFilePath(fileName));

    const QDir themesResourceDir(":/themes");
    for (const QString& fileName : themesResourceDir.entryList(QStringList() << "*.qss", QDir::Files, QDir::Name))
        copyFileIfMissing(themesResourceDir.absoluteFilePath(fileName),
                          QDir(AppConfig::themesDir()).absoluteFilePath(fileName));

    copyFileIfMissing(":/eagle_logo.png", AppConfig::logoPngPath());
    copyFileIfMissing(":/eagle_mark.svg", AppConfig::markSvgPath());
    copyFileIfMissing(":/eagle_256.png", QDir(AppConfig::resourcesDir()).absoluteFilePath("eagle_256.png"));
}

void migrateLegacyStorage()
{
    QDir().mkpath(AppConfig::dataDir());
    QDir().mkpath(AppConfig::settingsDir());
    QDir().mkpath(AppConfig::translationsDir());
    QDir().mkpath(AppConfig::themesDir());
    QDir().mkpath(AppConfig::hooksDir());
    QDir().mkpath(AppConfig::resourcesDir());
    QDir().mkpath(AppConfig::runtimeDir());
    QDir().mkpath(AppConfig::backupsDir());

    const QString newDbPath = AppConfig::dbPath();
    const QStringList legacyDbPaths = {
        AppConfig::appDir() + "/library.db",
        AppConfig::legacyDataDir() + "/library.db"
    };
    for (const QString& legacyDbPath : legacyDbPaths) {
        if (!QFileInfo::exists(newDbPath) && QFileInfo::exists(legacyDbPath)) {
            QDir().mkpath(QFileInfo(newDbPath).absolutePath());
            QFile::copy(legacyDbPath, newDbPath);
        }
    }

    const QStringList dataSubdirs = { "covers", "thumbs", "json", "sidecars" };
    for (const QString& subdir : dataSubdirs) {
        copyDirectoryTree(QDir(AppConfig::legacyDataDir()).absoluteFilePath(subdir),
                          QDir(AppConfig::dataDir()).absoluteFilePath(subdir));
    }

    const QStringList legacyIniPaths = {
        AppConfig::appDir() + "/EagleLibrary.ini",
        AppConfig::legacySettingsDir() + "/EagleLibrary.ini",
        AppConfig::localSettingsDir() + "/EagleLibrary.ini"
    };
    for (const QString& legacyIniPath : legacyIniPaths) {
        if (!QFileInfo::exists(AppConfig::settingsPath()) && QFileInfo::exists(legacyIniPath)) {
            QDir().mkpath(QFileInfo(AppConfig::settingsPath()).absolutePath());
            QFile::copy(legacyIniPath, AppConfig::settingsPath());
        }
    }

    copyDirectoryTree(QDir(AppConfig::legacySettingsDir()).absoluteFilePath("translations"),
                      AppConfig::translationsDir());
    copyDirectoryTree(QDir(AppConfig::localSettingsDir()).absoluteFilePath("translations"),
                      AppConfig::translationsDir());
    copyDirectoryTree(QDir(AppConfig::legacySettingsDir()).absoluteFilePath("hooks"),
                      AppConfig::hooksDir());
    copyDirectoryTree(QDir(AppConfig::localSettingsDir()).absoluteFilePath("hooks"),
                      AppConfig::hooksDir());
    copyDirectoryTree(QDir(AppConfig::localSettingsDir()).absoluteFilePath("themes"),
                      AppConfig::themesDir());
    copyDirectoryTree(QDir(AppConfig::localSettingsDir()).absoluteFilePath("resources"),
                      AppConfig::resourcesDir());
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
    QDir().mkpath(QFileInfo(AppConfig::settingsPath()).absolutePath());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, AppConfig::appDir());
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
    QDir().mkpath(AppConfig::translationsDir());
    QDir().mkpath(AppConfig::themesDir());
    QDir().mkpath(AppConfig::hooksDir());
    QDir().mkpath(AppConfig::resourcesDir());
    QDir().mkpath(AppConfig::runtimeDir());
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
        QSettings s(AppConfig::settingsPath(), QSettings::IniFormat);
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
    configureDiagnostics();
    qInfo().noquote() << "[Startup] Settings storage configured";
    migrateLegacyStorage();
    qInfo().noquote() << "[Startup] Legacy storage migration checked";
    seedBundledRuntimeAssets();
    qInfo().noquote() << "[Startup] Bundled runtime assets checked";
    LanguageManager::instance().initialize();
    qInfo().noquote() << "[Startup] Language initialized:" << LanguageManager::instance().currentLanguage();
    app.setLayoutDirection(LanguageManager::instance().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    QObject::connect(&LanguageManager::instance(), &LanguageManager::languageChanged, &app, [&app]() {
        app.setLayoutDirection(LanguageManager::instance().isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    });

    if (app.arguments().size() > 1) {
        qInfo().noquote() << "[CLI] Arguments:" << app.arguments().join(' ');
        const int cliResult = runCliCommand(app);
        if (cliResult >= 0)
            return cliResult;
    }

    QIcon appIcon(AppConfig::logoPngPath());
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
    QDir().mkpath(AppConfig::translationsDir());
    QDir().mkpath(AppConfig::themesDir());
    QDir().mkpath(AppConfig::hooksDir());
    QDir().mkpath(AppConfig::resourcesDir());

    step(35, LanguageManager::instance().text("splash.database", "Opening library database..."));
    qInfo().noquote() << "[Startup] Opening database:" << QDir::toNativeSeparators(AppConfig::dbPath());
    Database::instance().open(AppConfig::dbPath());

    step(60, LanguageManager::instance().text("splash.catalogue", "Loading book catalogue..."));
    step(80, LanguageManager::instance().text("splash.interface", "Building user interface..."));

    qInfo().noquote() << "[Startup] Creating main window";
    MainWindow* mainWin = new MainWindow;
    qInfo().noquote() << "[Startup] Main window created";

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
    QSettings s(AppConfig::settingsPath(), QSettings::IniFormat);
    if (!s.contains("library/folders")) {
        qInfo().noquote() << "[Startup] First-run welcome scheduled";
        QTimer::singleShot(500, mainWin, [mainWin]() {
            QMessageBox::information(mainWin,
                LanguageManager::instance().text("message.welcome.title", "Welcome to Eagle Library"),
                LanguageManager::instance().text("message.welcome.body",
                    "<h3>Welcome!</h3><p>To get started, go to <b>Settings</b> and add the folders where your eBooks are stored.<br><br>Eagle Library will scan them automatically and keep your books organised without moving any files.</p>"));
        });
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, []() {
        qInfo().noquote() << "[Shutdown] QApplication aboutToQuit";
        writeDiagnosticLine(QStringLiteral("INFO"), QStringLiteral("[Shutdown] Final log flush"));
    });

    qInfo().noquote() << "[Startup] Entering event loop";
    const int result = app.exec();
    qInfo().noquote() << "[Shutdown] Event loop exited with code" << result;
    writeDiagnosticLine(QStringLiteral("INFO"), QStringLiteral("[Shutdown] Process returning %1").arg(result));
    return result;
}
