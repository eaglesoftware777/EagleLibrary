// ============================================================
//  Eagle Library v2.0 — PluginManager.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "PluginManager.h"
#include "AppConfig.h"
#include "Database.h"

#include <QDir>
#include <QFileInfoList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QSettings>
#include <QDockWidget>
#include <QMainWindow>
#include <QDebug>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QStandardPaths>
#include <QSet>
#include <QMessageBox>
#include <QListView>

#include "BookModel.h"

PluginManager& PluginManager::instance()
{
    static PluginManager inst;
    return inst;
}

PluginManager::PluginManager(QObject* parent) : QObject(parent) {}

namespace {

struct StarterPluginSeed {
    const char* dirName;
    const char* json;
};

QString encoded(QString value)
{
    return QString::fromUtf8(QUrl::toPercentEncoding(value));
}

QString fillBookTemplate(QString templ, const Book& book)
{
    const QString query = QString("%1 %2").arg(book.displayTitle(), book.displayAuthor()).trimmed();
    templ.replace("{title}", encoded(book.displayTitle()));
    templ.replace("{author}", encoded(book.displayAuthor()));
    templ.replace("{publisher}", encoded(book.publisher));
    templ.replace("{isbn}", encoded(book.isbn));
    templ.replace("{language}", encoded(book.language));
    templ.replace("{filePath}", encoded(book.filePath));
    templ.replace("{query}", encoded(query));
    return templ;
}

void runPythonHook(const QString& eventName, qint64 bookId)
{
    const QSettings settings(AppConfig::settingsPath(), QSettings::IniFormat);
    if (!settings.value(QStringLiteral("plugins/enablePythonHooks"), false).toBool())
        return;

    const QString python = QStandardPaths::findExecutable("python").isEmpty()
        ? QStandardPaths::findExecutable("python3")
        : QStandardPaths::findExecutable("python");
    if (python.isEmpty())
        return;

    const Book book = Database::instance().bookById(bookId);
    const QFileInfo hooksRootInfo(AppConfig::hooksDir());
    const QString hooksRoot = hooksRootInfo.canonicalFilePath();
    const QFileInfo scriptInfo(QDir(AppConfig::hooksDir()).absoluteFilePath(eventName + ".py"));
    if (!scriptInfo.exists() || !scriptInfo.isFile() || scriptInfo.suffix().compare(QStringLiteral("py"), Qt::CaseInsensitive) != 0)
        return;
    const QString scriptPath = scriptInfo.canonicalFilePath();
    if (hooksRoot.isEmpty() || scriptPath.isEmpty() || !scriptPath.startsWith(hooksRoot + QDir::separator()))
        return;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("EAGLE_EVENT", eventName);
    env.insert("EAGLE_BOOK_ID", QString::number(bookId));
    env.insert("EAGLE_DB_PATH", AppConfig::dbPath());
    env.insert("EAGLE_BOOK_TITLE", book.displayTitle());
    env.insert("EAGLE_BOOK_AUTHOR", book.displayAuthor());
    env.insert("EAGLE_BOOK_PATH", book.filePath);

    auto* proc = new QProcess;
    proc->setProgram(python);
    proc->setArguments({scriptPath});
    proc->setProcessEnvironment(env);
    proc->setWorkingDirectory(scriptInfo.absolutePath());
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), proc, &QObject::deleteLater);
    proc->start();
}

bool isAllowedPluginUrl(const QUrl& url)
{
    if (!url.isValid() || url.scheme() != QStringLiteral("https"))
        return false;

    static const QSet<QString> allowedHosts = {
        QStringLiteral("www.google.com"),
        QStringLiteral("scholar.google.com"),
        QStringLiteral("books.google.com"),
        QStringLiteral("openlibrary.org"),
        QStringLiteral("catalog.loc.gov"),
        QStringLiteral("www.worldcat.org"),
        QStringLiteral("en.wikipedia.org"),
        QStringLiteral("archive.org"),
        QStringLiteral("www.semanticscholar.org")
    };
    return allowedHosts.contains(url.host().toLower());
}

QString describeBookAction(const QJsonObject& actionObj, const QString& fallback)
{
    return actionObj.value(QStringLiteral("description")).toString(fallback);
}

const StarterPluginSeed kStarterPlugins[] = {
    { "metadata_assistant", R"json({
  "id": "com.eaglesoftware.metadata_assistant",
  "name": "Metadata Assistant",
  "version": "1.0.0",
  "author": "Eagle Software",
  "description": "Quick metadata and catalog lookups for the current item.",
  "bookActions": [
    { "title": "Search Google Books", "description": "Look up the current item on Google Books.", "urlTemplate": "https://www.google.com/search?tbm=bks&q={query}" },
    { "title": "Search Open Library", "description": "Open an Open Library search for the current item.", "urlTemplate": "https://openlibrary.org/search?q={query}" },
    { "title": "Search Library of Congress", "description": "Search the Library of Congress catalog.", "urlTemplate": "https://catalog.loc.gov/vwebv/search?searchArg={query}&searchCode=GKEY%5E*&searchType=0&recCount=25" }
  ]
})json" },
    { "research_toolbox", R"json({
  "id": "com.eaglesoftware.research_toolbox",
  "name": "Research Toolbox",
  "version": "1.0.0",
  "author": "Eagle Software",
  "description": "Reference and academic research lookups for books and documents.",
  "bookActions": [
    { "title": "Search Google Scholar", "description": "Search academic references for the current item.", "urlTemplate": "https://scholar.google.com/scholar?q={query}" },
    { "title": "Search WorldCat", "description": "Search WorldCat for the current item.", "urlTemplate": "https://www.worldcat.org/search?q={query}" },
    { "title": "Search Wikipedia", "description": "Search Wikipedia for the current item.", "urlTemplate": "https://en.wikipedia.org/w/index.php?search={query}" }
  ]
})json" },
    { "device_sync_tools", R"json({
  "id": "com.eaglesoftware.device_sync_tools",
  "name": "Device Sync Tools",
  "version": "1.0.0",
  "author": "Eagle Software",
  "description": "Starter device and file-location helper plugin.",
  "bookActions": [
    { "title": "Search File Name on Google", "description": "Search the file name on Google.", "urlTemplate": "https://www.google.com/search?q={title}" },
    { "title": "Search PDF or Document Info", "description": "Search general web results for the current item.", "urlTemplate": "https://www.google.com/search?q={query}%20filetype%3Apdf" }
  ]
})json" }
};

}

void PluginManager::loadAll(const QString& pluginsDir)
{
    QDir dir(pluginsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    ensureStarterPlugins(pluginsDir);

    for (const QFileInfo& fi : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString manifestPath = fi.absoluteFilePath() + "/plugin.json";
        if (QFile::exists(manifestPath))
            loadPlugin(manifestPath);
    }

    syncPluginMenu();
}

void PluginManager::ensureStarterPlugins(const QString& pluginsDir)
{
    QDir root(pluginsDir);
    root.mkpath(".");
    for (const StarterPluginSeed& seed : kStarterPlugins) {
        const QString pluginDir = root.absoluteFilePath(QString::fromUtf8(seed.dirName));
        QDir().mkpath(pluginDir);
        const QString manifestPath = QDir(pluginDir).absoluteFilePath("plugin.json");
        if (QFileInfo::exists(manifestPath))
            continue;
        QFile file(manifestPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(seed.json);
            file.close();
        }
    }
}

bool PluginManager::loadPlugin(const QString& jsonManifestPath)
{
    QFile f(jsonManifestPath);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    if (!doc.isObject())
        return false;

    const QJsonObject obj = doc.object();
    const QString id = obj["id"].toString();
    if (id.isEmpty() || m_plugins.contains(id))
        return false;

    LoadedPlugin lp;
    lp.info.id          = id;
    lp.info.name        = obj["name"].toString();
    lp.info.version     = obj["version"].toString("1.0.0");
    lp.info.author      = obj["author"].toString();
    lp.info.description = obj["description"].toString();
    lp.info.iconPath    = obj["icon"].toString();
    lp.sourcePath       = jsonManifestPath;
    lp.bookActions      = obj["bookActions"].toArray();

    for (const QJsonValue& v : obj["requires"].toArray())
        lp.info.requires << v.toString();

    // Plugin instance is null for JSON-only/scripted plugins
    lp.instance = nullptr;
    lp.active   = false;

    m_plugins.insert(id, lp);
    syncPluginMenu();
    qDebug() << "PluginManager: registered plugin" << lp.info.name << lp.info.version;
    emit pluginLoaded(id);
    return true;
}

void PluginManager::unloadPlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId))
        return;
    LoadedPlugin& lp = m_plugins[pluginId];
    if (lp.menuAction) {
        delete lp.menuAction;
        lp.menuAction = nullptr;
    }
    if (lp.instance && lp.active) {
        lp.instance->deactivate();
        lp.active = false;
    }
    m_plugins.remove(pluginId);
    syncPluginMenu();
    emit pluginUnloaded(pluginId);
}

void PluginManager::unloadAll()
{
    for (const QString& id : m_plugins.keys())
        unloadPlugin(id);
}

bool PluginManager::isLoaded(const QString& pluginId) const
{
    return m_plugins.contains(pluginId);
}

void PluginManager::triggerJsonBookAction(const LoadedPlugin& plugin,
                                          const QString& title,
                                          const QString& description,
                                          const QString& urlTemplate,
                                          qint64 bookId)
{
    const Book book = Database::instance().bookById(bookId);
    if (book.id <= 0) {
        const QString message = QStringLiteral("Plugin action could not resolve the selected book.");
        qWarning().noquote() << "[PluginAction]" << plugin.info.name << title << "failed:" << message;
        emit statusMessage(message);
        return;
    }

    const QUrl url(fillBookTemplate(urlTemplate, book));
    qInfo().noquote() << "[PluginAction]" << plugin.info.name
                      << "title=" << title
                      << "bookId=" << bookId
                      << "url=" << url.toString();

    if (!isAllowedPluginUrl(url)) {
        const QString message = QString("Blocked unsafe plugin URL from %1.").arg(plugin.info.name);
        qWarning().noquote() << "[PluginAction]" << plugin.info.name << title << "blocked";
        emit statusMessage(message);
        if (m_mainWindow) {
            QMessageBox::warning(m_mainWindow,
                                 QStringLiteral("Plugin Action Blocked"),
                                 QString("%1\n\nAction: %2\nURL: %3")
                                     .arg(message, title, url.toString()));
        }
        return;
    }

    const bool opened = QDesktopServices::openUrl(url);
    if (opened) {
        emit statusMessage(QString("%1 -> %2").arg(plugin.info.name, title));
    } else {
        const QString message = QString("Plugin action failed to open: %1").arg(title);
        qWarning().noquote() << "[PluginAction]" << plugin.info.name << title << "openUrl failed";
        emit statusMessage(message);
        if (m_mainWindow) {
            QMessageBox::warning(m_mainWindow,
                                 QStringLiteral("Plugin Action Failed"),
                                 QString("%1\n\n%2").arg(message, description));
        }
    }
}

void PluginManager::syncPluginMenu()
{
    if (!m_pluginMenu)
        return;

    m_pluginMenu->setTitle(QString("&Plugins (%1)").arg(m_plugins.size()));

    QList<QAction*> existingActions = m_pluginMenu->actions();
    for (QAction* action : existingActions) {
        const QVariant pluginId = action->property("plugin_id");
        const bool isPlaceholder = action->property("plugin_placeholder").toBool();
        if (pluginId.isValid() || isPlaceholder)
            delete action;
    }

    if (m_plugins.isEmpty()) {
        QAction* emptyAction = new QAction("No plugins installed in the runtime plugins folder", m_pluginMenu);
        emptyAction->setEnabled(false);
        emptyAction->setProperty("plugin_placeholder", true);
        m_pluginMenu->addAction(emptyAction);
        return;
    }

    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        LoadedPlugin& lp = it.value();
        const QString title = lp.info.version.isEmpty()
            ? lp.info.name
            : QString("%1 (%2)").arg(lp.info.name, lp.info.version);
        QMenu* pluginSubMenu = m_pluginMenu->addMenu(title);
        pluginSubMenu->setToolTipsVisible(true);
        pluginSubMenu->menuAction()->setProperty("plugin_id", lp.info.id);
        pluginSubMenu->menuAction()->setToolTip(lp.info.description);
        pluginSubMenu->menuAction()->setStatusTip(lp.info.description);

        QAction* infoAction = pluginSubMenu->addAction(lp.info.description.isEmpty()
            ? QStringLiteral("No description")
            : lp.info.description);
        infoAction->setEnabled(false);

        const QList<qint64> selectedIds = selectedBookIds();
        bool addedExecutableAction = false;
        for (const QJsonValue& value : lp.bookActions) {
            if (!value.isObject())
                continue;
            const QJsonObject actionObj = value.toObject();
            const QString actionTitle = actionObj.value(QStringLiteral("title")).toString();
            const QString urlTemplate = actionObj.value(QStringLiteral("urlTemplate")).toString();
            if (actionTitle.isEmpty() || urlTemplate.isEmpty())
                continue;

            QAction* action = pluginSubMenu->addAction(actionTitle);
            const QString description = describeBookAction(actionObj, lp.info.description);
            action->setToolTip(description);
            action->setStatusTip(description);
            QObject::connect(action, &QAction::triggered, this, [this, lp, actionTitle, description, urlTemplate]() {
                const QList<qint64> selected = selectedBookIds();
                if (selected.isEmpty()) {
                    const QString message = QString("Select a book first to run %1.").arg(actionTitle);
                    qWarning().noquote() << "[PluginAction]" << lp.info.name << actionTitle << "no selection";
                    emit statusMessage(message);
                    if (m_mainWindow)
                        QMessageBox::information(m_mainWindow, QStringLiteral("Plugin Action"), message);
                    return;
                }
                triggerJsonBookAction(lp, actionTitle, description, urlTemplate, selected.first());
            });
            addedExecutableAction = true;
        }

        pluginSubMenu->addSeparator();
        QAction* manifestAction = pluginSubMenu->addAction(QStringLiteral("Open Plugin Folder"));
        QObject::connect(manifestAction, &QAction::triggered, this, [this, lp]() {
            const QString folder = QFileInfo(lp.sourcePath).absolutePath();
            qInfo().noquote() << "[PluginMenu] Open folder for" << lp.info.name << QDir::toNativeSeparators(folder);
            QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
        });

        if (!addedExecutableAction) {
            QAction* emptyAction = pluginSubMenu->addAction(selectedIds.isEmpty()
                ? QStringLiteral("No runnable actions for the current selection")
                : QStringLiteral("No JSON actions declared"));
            emptyAction->setEnabled(false);
        }

        lp.menuAction = pluginSubMenu->menuAction();
    }
}

void PluginManager::notifyBookAdded(qint64 id)
{
    runPythonHook(QStringLiteral("on_import"), id);
    for (auto& lp : m_plugins)
        if (lp.instance && lp.active)
            lp.instance->onBookAdded(id);
}

void PluginManager::notifyBookUpdated(qint64 id)
{
    runPythonHook(QStringLiteral("on_update"), id);
    for (auto& lp : m_plugins)
        if (lp.instance && lp.active)
            lp.instance->onBookUpdated(id);
}

void PluginManager::notifyBookRemoved(qint64 id)
{
    runPythonHook(QStringLiteral("on_delete"), id);
    for (auto& lp : m_plugins)
        if (lp.instance && lp.active)
            lp.instance->onBookRemoved(id);
}

QList<QAction*> PluginManager::contextActionsForBook(qint64 id)
{
    QList<QAction*> actions;
    const Book book = Database::instance().bookById(id);

    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        const LoadedPlugin& lp = it.value();
        for (const QJsonValue& value : lp.bookActions) {
            if (!value.isObject())
                continue;
            const QJsonObject actionObj = value.toObject();
            const QString title = actionObj.value("title").toString();
            const QString urlTemplate = actionObj.value("urlTemplate").toString();
            if (title.isEmpty() || urlTemplate.isEmpty())
                continue;
            QAction* action = new QAction(QString("%1: %2").arg(lp.info.name, title), nullptr);
            const QString description = describeBookAction(actionObj, lp.info.description);
            action->setToolTip(description);
            action->setStatusTip(description);
            QObject::connect(action, &QAction::triggered, this, [this, urlTemplate, lp, title, description, book]() {
                triggerJsonBookAction(lp, title, description, urlTemplate, book.id);
            });
            actions << action;
        }
    }

    for (auto& lp : m_plugins)
        if (lp.instance && lp.active)
            actions << lp.instance->bookContextActions(id);
    return actions;
}

// ── IPluginContext ─────────────────────────────────────────────

void PluginManager::setStatus(const QString& msg)
{
    emit statusMessage(msg);
}

QJsonObject PluginManager::settings() const
{
    return m_settings;
}

void PluginManager::saveSettings(const QJsonObject& s)
{
    m_settings = s;
    QSettings qs(AppConfig::settingsPath(), QSettings::IniFormat);
    qs.setValue("plugins/settings", QJsonDocument(s).toJson(QJsonDocument::Compact));
}

void PluginManager::addSidePanel(QWidget* panel, const QString& title)
{
    if (!m_mainWindow) return;
    QMainWindow* mw = qobject_cast<QMainWindow*>(m_mainWindow);
    if (!mw) return;
    QDockWidget* dock = new QDockWidget(title, mw);
    dock->setWidget(panel);
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    mw->addDockWidget(Qt::RightDockWidgetArea, dock);
}

void PluginManager::addToolBarAction(QAction* action)
{
    if (m_toolBar) m_toolBar->addAction(action);
}

QList<qint64> PluginManager::selectedBookIds() const
{
    QList<qint64> ids;
    if (!m_mainWindow)
        return ids;

    const QList<QListView*> views = m_mainWindow->findChildren<QListView*>();
    for (QListView* view : views) {
        if (!view || !view->selectionModel())
            continue;
        const QModelIndexList selection = view->selectionModel()->selectedIndexes();
        for (const QModelIndex& index : selection) {
            const qint64 id = index.data(IdRole).toLongLong();
            if (id > 0 && !ids.contains(id))
                ids << id;
        }
    }
    return ids;
}

void PluginManager::openBookById(qint64 /*id*/)
{
    // Connected via signal in MainWindow
}
