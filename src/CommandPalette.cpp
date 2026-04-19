// ============================================================
//  Eagle Library v2.0 — CommandPalette.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "CommandPalette.h"
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QEvent>
#include <QLabel>
#include <QFrame>
#include <QListWidgetItem>
#include <algorithm>

CommandPalette::CommandPalette(QWidget* parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Popup)
{
    setObjectName("commandPalette");
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(560);
    setMaximumWidth(640);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_input = new QLineEdit(this);
    m_input->setObjectName("commandInput");
    m_input->setPlaceholderText("Type a command...");
    m_input->setClearButtonEnabled(true);
    layout->addWidget(m_input);

    m_list = new QListWidget(this);
    m_list->setObjectName("commandList");
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setUniformItemSizes(true);
    layout->addWidget(m_list);

    connect(m_input, &QLineEdit::textChanged, this, &CommandPalette::onFilterChanged);
    connect(m_list, &QListWidget::itemActivated, this, &CommandPalette::onItemActivated);

    m_input->installEventFilter(this);
    m_list->installEventFilter(this);
}

void CommandPalette::registerCommand(const Command& cmd)
{
    m_commands.append(cmd);
}

void CommandPalette::registerCommands(const QList<Command>& cmds)
{
    m_commands.append(cmds);
}

void CommandPalette::clearCommands()
{
    m_commands.clear();
}

void CommandPalette::popup()
{
    m_input->clear();
    rebuildList();
    // Center on parent or screen
    QWidget* p = parentWidget() ? parentWidget() : QApplication::activeWindow();
    if (p) {
        const QRect pr = p->geometry();
        move(pr.center().x() - width() / 2, pr.top() + pr.height() / 5);
    } else {
        QScreen* scr = QApplication::primaryScreen();
        const QRect sr = scr->availableGeometry();
        move(sr.center().x() - width() / 2, sr.top() + sr.height() / 5);
    }
    show();
    raise();
    activateWindow();
    m_input->setFocus();
}

bool CommandPalette::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Down) {
            const int next = m_list->currentRow() + 1;
            if (next < m_list->count())
                m_list->setCurrentRow(next);
            return true;
        }
        if (ke->key() == Qt::Key_Up) {
            const int prev = m_list->currentRow() - 1;
            if (prev >= 0)
                m_list->setCurrentRow(prev);
            return true;
        }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            executeSelected();
            return true;
        }
        if (ke->key() == Qt::Key_Escape) {
            hide();
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void CommandPalette::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
        hide();
    else
        QDialog::keyPressEvent(event);
}

void CommandPalette::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    m_list->setCurrentRow(0);
}

void CommandPalette::onFilterChanged(const QString& text)
{
    rebuildList(text);
}

void CommandPalette::onItemActivated(QListWidgetItem* /*item*/)
{
    executeSelected();
}

void CommandPalette::executeSelected()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_filtered.size())
        return;
    hide();
    const Command cmd = m_filtered.at(row);
    if (cmd.action)
        cmd.action();
}

static int fuzzyScore(const QString& haystack, const QString& needle)
{
    if (needle.isEmpty()) return 1;
    const QString h = haystack.toLower();
    const QString n = needle.toLower();
    if (h.contains(n)) return 3;
    int score = 0, j = 0;
    for (int i = 0; i < h.size() && j < n.size(); ++i)
        if (h[i] == n[j]) { ++score; ++j; }
    return j == n.size() ? score : 0;
}

void CommandPalette::rebuildList(const QString& filter)
{
    m_list->clear();
    m_filtered.clear();

    const QString f = filter.trimmed();

    struct Scored { Command cmd; int score; };
    QList<Scored> scored;

    for (const Command& cmd : m_commands) {
        int s = fuzzyScore(cmd.label, f);
        if (s == 0) s = fuzzyScore(cmd.description, f);
        if (s == 0) s = fuzzyScore(cmd.category, f);
        if (s > 0 || f.isEmpty())
            scored.append({ cmd, f.isEmpty() ? 0 : s });
    }

    std::stable_sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) {
        return a.score > b.score;
    });

    const int n = qMin(static_cast<int>(scored.size()), MAX_RESULTS);
    for (int i = 0; i < n; ++i) {
        const Command& cmd = scored[i].cmd;
        m_filtered.append(cmd);

        auto* item = new QListWidgetItem(m_list);
        // Format: "Category  ›  Label    shortcut"
        QString text = cmd.label;
        if (!cmd.category.isEmpty())
            text = cmd.category + "  \u203a  " + text;
        item->setText(text);

        if (!cmd.shortcut.isEmpty())
            item->setToolTip(cmd.shortcut);

        item->setData(Qt::UserRole, i);
    }

    m_list->setFixedHeight(qMax(44, n * 38) + 4);
    adjustSize();

    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}
