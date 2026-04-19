#pragma once
// ============================================================
//  Eagle Library v2.0 — PluginInterface.h
//  Plugin API for extending Eagle Library
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QWidget>
#include <QMenu>
#include <QToolBar>
#include <QtPlugin>

// ── Plugin Manifest ───────────────────────────────────────────
struct PluginInfo {
    QString id;           // unique reverse-domain id e.g. "com.example.myplugin"
    QString name;         // display name
    QString version;      // semver e.g. "1.0.0"
    QString author;
    QString description;
    QString iconPath;     // optional icon resource or file path
    QStringList requires; // list of plugin ids this depends on
};

// ── Plugin Context ────────────────────────────────────────────
// Passed to the plugin on activation so it can interact with the app
class IPluginContext
{
public:
    virtual ~IPluginContext() = default;

    // Status bar message
    virtual void setStatus(const QString& msg) = 0;

    // Access settings
    virtual QJsonObject settings() const = 0;
    virtual void        saveSettings(const QJsonObject& s) = 0;

    // Add a panel widget to the sidebar dock area
    virtual void addSidePanel(QWidget* panel, const QString& title) = 0;

    // Add items to the plugin menu
    virtual QMenu* pluginMenu() = 0;

    // Add actions to the main toolbar (plugin zone)
    virtual void addToolBarAction(QAction* action) = 0;

    // Get currently selected book IDs
    virtual QList<qint64> selectedBookIds() const = 0;

    // Open a book by ID (triggers detail dialog)
    virtual void openBookById(qint64 id) = 0;
};

// ── Plugin Base Interface ─────────────────────────────────────
class IPlugin
{
public:
    virtual ~IPlugin() = default;

    // Called once when plugin is loaded
    virtual PluginInfo info() const = 0;

    // Called when the plugin is activated (app ready)
    virtual bool activate(IPluginContext* ctx) = 0;

    // Called before the plugin is unloaded
    virtual void deactivate() = 0;

    // Called when a book context-menu is opened for bookId
    // Return actions to add to the context menu (plugin owns the actions)
    virtual QList<QAction*> bookContextActions(qint64 /*bookId*/) { return {}; }

    // Called on application events
    virtual void onBookAdded(qint64 /*bookId*/)   {}
    virtual void onBookUpdated(qint64 /*bookId*/) {}
    virtual void onBookRemoved(qint64 /*bookId*/) {}
};

Q_DECLARE_INTERFACE(IPlugin, "com.eaglesoftware.eaglelibrary.IPlugin/2.0")
