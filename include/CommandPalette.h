#pragma once
// ============================================================
//  Eagle Library v2.0 — CommandPalette.h
//  VS Code-style command launcher
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QString>
#include <QStringList>
#include <functional>
#include <QList>

struct Command {
    QString            id;
    QString            label;
    QString            description;
    QString            shortcut;
    QString            category;
    std::function<void()> action;
};

class CommandPalette : public QDialog
{
    Q_OBJECT
public:
    explicit CommandPalette(QWidget* parent = nullptr);

    void registerCommand(const Command& cmd);
    void registerCommands(const QList<Command>& cmds);
    void clearCommands();

    void popup();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onFilterChanged(const QString& text);
    void onItemActivated(QListWidgetItem* item);
    void executeSelected();

private:
    void rebuildList(const QString& filter = {});
    void styleWidgets();

    QLineEdit*   m_input;
    QListWidget* m_list;
    QList<Command> m_commands;
    QList<Command> m_filtered;

    static constexpr int MAX_RESULTS = 12;
};
