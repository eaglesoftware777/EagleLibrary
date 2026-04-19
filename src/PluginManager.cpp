// ============================================================
//  Eagle Library v2.0 — PluginManager.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "PluginManager.h"
#include "AppConfig.h"

#include <QDir>
#include <QFileInfoList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QSettings>
#include <QDockWidget>
#include <QMainWindow>
#include <QDebug>

PluginManager& PluginManager::instance()
{
    static PluginManager inst;
    return inst;
}

PluginManager::PluginManager(QObject* parent) : QObject(parent) {}

void PluginManager::loadAll(const QString& pluginsDir)
{
    QDir dir(pluginsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
        return;
    }

    const QFileInfoList entries = dir.entryInfoList(QStringList() << "plugin.json", QDir::Files, QDir::Name);
    QList<QDir> subdirs;
    for (const QFileInfo& fi : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString manifestPath = fi.absoluteFilePath() + "/plugin.json";
        if (QFile::exists(manifestPath))
            loadPlugin(manifestPath);
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

    for (const QJsonValue& v : obj["requires"].toArray())
        lp.info.requires << v.toString();

    // Plugin instance is null for JSON-only/scripted plugins
    lp.instance = nullptr;
    lp.active   = false;

    m_plugins.insert(id, lp);
    qDebug() << "PluginManager: registered plugin" << lp.info.name << lp.info.version;
    emit pluginLoaded(id);
    return true;
}

void PluginManager::unloadPlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId))
        return;
    LoadedPlugin& lp = m_plugins[pluginId];
    if (lp.instance && lp.active) {
        lp.instance->deactivate();
        lp.active = false;
    }
    m_plugins.remove(pluginId);
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

void PluginManager::notifyBookAdded(qint64 id)
{
    for (auto& lp : m_plugins)
        if (lp.instance && lp.active)
            lp.instance->onBookAdded(id);
}

void PluginManager::notifyBookUpdated(qint64 id)
{
    for (auto& lp : m_plugins)
        if (lp.instance && lp.active)
            lp.instance->onBookUpdated(id);
}

void PluginManager::notifyBookRemoved(qint64 id)
{
    for (auto& lp : m_plugins)
        if (lp.instance && lp.active)
            lp.instance->onBookRemoved(id);
}

QList<QAction*> PluginManager::contextActionsForBook(qint64 id)
{
    QList<QAction*> actions;
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
    QSettings qs;
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
    return {};
}

void PluginManager::openBookById(qint64 /*id*/)
{
    // Connected via signal in MainWindow
}
