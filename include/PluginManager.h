#pragma once
// ============================================================
//  Eagle Library v2.0 — PluginManager.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "PluginInterface.h"
#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

struct LoadedPlugin {
    PluginInfo    info;
    IPlugin*      instance = nullptr;
    bool          active   = false;
    QString       sourcePath;
    QAction*      menuAction = nullptr;
    QJsonArray    bookActions;
};

class PluginManager : public QObject, public IPluginContext
{
    Q_OBJECT
public:
    static PluginManager& instance();

    void setMainWindow(QWidget* mw) { m_mainWindow = mw; }
    void setPluginMenu(QMenu* m)    { m_pluginMenu = m; }
    void setToolBar(QToolBar* tb)   { m_toolBar = tb; }

    // Scan plugins directory and load all valid plugins
    void    loadAll(const QString& pluginsDir);
    void    unloadAll();
    void    syncPluginMenu();

    // Individual management
    bool    loadPlugin(const QString& jsonManifestPath);
    void    unloadPlugin(const QString& pluginId);

    bool    isLoaded(const QString& pluginId) const;
    QList<LoadedPlugin> loadedPlugins() const { return m_plugins.values(); }

    // Emit events to all active plugins
    void notifyBookAdded(qint64 id);
    void notifyBookUpdated(qint64 id);
    void notifyBookRemoved(qint64 id);

    // Build context menu additions for a book
    QList<QAction*> contextActionsForBook(qint64 id);

    // IPluginContext implementation
    void       setStatus(const QString& msg) override;
    QJsonObject settings() const override;
    void        saveSettings(const QJsonObject& s) override;
    void        addSidePanel(QWidget* panel, const QString& title) override;
    QMenu*      pluginMenu() override  { return m_pluginMenu; }
    void        addToolBarAction(QAction* action) override;
    QList<qint64> selectedBookIds() const override;
    void          openBookById(qint64 id) override;

signals:
    void pluginLoaded(const QString& id);
    void pluginUnloaded(const QString& id);
    void statusMessage(const QString& msg);

private:
    explicit PluginManager(QObject* parent = nullptr);
    void ensureStarterPlugins(const QString& pluginsDir);
    void triggerJsonBookAction(const LoadedPlugin& plugin,
                               const QString& title,
                               const QString& description,
                               const QString& urlTemplate,
                               qint64 bookId);

    QMap<QString, LoadedPlugin> m_plugins;
    QWidget*   m_mainWindow = nullptr;
    QMenu*     m_pluginMenu = nullptr;
    QToolBar*  m_toolBar    = nullptr;
    QJsonObject m_settings;
};
