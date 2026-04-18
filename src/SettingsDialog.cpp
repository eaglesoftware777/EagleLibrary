// ============================================================
//  Eagle Library — SettingsDialog.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================
#include "SettingsDialog.h"
#include "AppConfig.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QSettings>
#include <QLabel>
#include <QTabWidget>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings — Eagle Library");
    setMinimumSize(560, 400);
    setModal(true);
    setupUi();
    applyStyles();
    loadSettings();
}

void SettingsDialog::setupUi()
{
    auto* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(16,16,16,16);
    mainLay->setSpacing(12);

    auto* tabs = new QTabWidget;

    // ── Tab: Library Folders ──────────────────────────────────
    auto* folderTab = new QWidget;
    auto* folderLay = new QVBoxLayout(folderTab);

    auto* hdr = new QLabel("Add folders to monitor. Eagle Library will scan them recursively.");
    hdr->setWordWrap(true);
    folderLay->addWidget(hdr);

    m_folderList = new QListWidget;
    m_folderList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    folderLay->addWidget(m_folderList);

    auto* btnRow = new QHBoxLayout;
    auto* addBtn = new QPushButton("+ Add Folder…");
    auto* remBtn = new QPushButton("− Remove");
    btnRow->addWidget(addBtn);
    btnRow->addWidget(remBtn);
    btnRow->addStretch();
    folderLay->addLayout(btnRow);

    connect(addBtn, &QPushButton::clicked, this, &SettingsDialog::addFolder);
    connect(remBtn, &QPushButton::clicked, this, &SettingsDialog::removeFolder);
    tabs->addTab(folderTab, "Library Folders");

    // ── Tab: Options ─────────────────────────────────────────
    auto* optTab    = new QWidget;
    auto* optForm   = new QFormLayout(optTab);
    optForm->setSpacing(14);

    m_autoMetaCheck  = new QCheckBox("Automatically fetch metadata from internet");
    m_autoCoverCheck = new QCheckBox("Automatically download book covers");
    m_iconSizeSpin   = new QSpinBox;
    m_iconSizeSpin->setRange(100, 280);
    m_iconSizeSpin->setSingleStep(20);
    m_iconSizeSpin->setSuffix(" px");

    optForm->addRow("",          m_autoMetaCheck);
    optForm->addRow("",          m_autoCoverCheck);
    optForm->addRow("Grid card width:", m_iconSizeSpin);
    tabs->addTab(optTab, "Options");

    // ── Tab: About ────────────────────────────────────────────
    auto* aboutTab = new QWidget;
    auto* aboutLay = new QVBoxLayout(aboutTab);
    aboutLay->setAlignment(Qt::AlignCenter);

    auto* logoLbl = new QLabel("🦅");
    logoLbl->setStyleSheet("font-size: 52px;");
    logoLbl->setAlignment(Qt::AlignCenter);

    auto* nameLbl = new QLabel(
        QString("<b style='font-size:18px;color:#c8aa50;'>%1</b>").arg("Eagle Library"));
    nameLbl->setAlignment(Qt::AlignCenter);

    auto* verLbl = new QLabel(QString("Version %1").arg("1.0.0"));
    verLbl->setAlignment(Qt::AlignCenter);

    auto* cpLbl = new QLabel("Copyright (C) 2024 Eagle Software. All rights reserved.");
    cpLbl->setAlignment(Qt::AlignCenter);
    cpLbl->setStyleSheet("color: #7080a0; font-size: 10px;");

    auto* webLbl = new QLabel(
        QString("<a href='%1' style='color:#4a90d0;'>%1</a>").arg("https://eaglesoftware.biz/"));
    webLbl->setOpenExternalLinks(true);
    webLbl->setAlignment(Qt::AlignCenter);

    aboutLay->addStretch();
    aboutLay->addWidget(logoLbl);
    aboutLay->addWidget(nameLbl);
    aboutLay->addWidget(verLbl);
    aboutLay->addSpacing(8);
    aboutLay->addWidget(cpLbl);
    aboutLay->addWidget(webLbl);
    aboutLay->addStretch();
    tabs->addTab(aboutTab, "About");

    mainLay->addWidget(tabs);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btnBox, &QDialogButtonBox::accepted, this, [this]() { saveSettings(); accept(); });
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLay->addWidget(btnBox);
}

void SettingsDialog::addFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Book Folder",
                  QDir::homePath(), QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        // Avoid duplicates
        QStringList existing;
        for (int i = 0; i < m_folderList->count(); ++i)
            existing << m_folderList->item(i)->text();
        if (!existing.contains(dir))
            m_folderList->addItem(dir);
    }
}

void SettingsDialog::removeFolder()
{
    qDeleteAll(m_folderList->selectedItems());
}

void SettingsDialog::loadSettings()
{
    QSettings s("Eagle Software", "Eagle Library");
    QStringList folders = s.value("library/folders").toStringList();
    for (const auto& f : folders) m_folderList->addItem(f);
    m_autoMetaCheck->setChecked(s.value("options/autoMeta", true).toBool());
    m_autoCoverCheck->setChecked(s.value("options/autoCover", true).toBool());
    m_iconSizeSpin->setValue(s.value("options/iconSize", 160).toInt());
}

void SettingsDialog::saveSettings()
{
    QSettings s("Eagle Software", "Eagle Library");
    s.setValue("library/folders",   watchedFolders());
    s.setValue("options/autoMeta",  m_autoMetaCheck->isChecked());
    s.setValue("options/autoCover", m_autoCoverCheck->isChecked());
    s.setValue("options/iconSize",  m_iconSizeSpin->value());
}

QStringList SettingsDialog::watchedFolders() const
{
    QStringList list;
    for (int i = 0; i < m_folderList->count(); ++i)
        list << m_folderList->item(i)->text();
    return list;
}

bool SettingsDialog::autoFetchMetadata() const { return m_autoMetaCheck->isChecked(); }
bool SettingsDialog::autoFetchCovers()   const { return m_autoCoverCheck->isChecked(); }
int  SettingsDialog::gridIconSize()      const { return m_iconSizeSpin->value(); }

void SettingsDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog, QWidget { background-color: #0f1a38; color: #ddd8c8; }
        QGroupBox { color: #b49a46; font-weight: bold;
            border: 1px solid #2a3a6a; border-radius:6px; margin-top:8px; padding:6px; }
        QListWidget {
            background:#131e42; border:1px solid #2e3f70; border-radius:6px; color:#ddd8c8; }
        QListWidget::item:selected { background:#2a4aaa; }
        QCheckBox { color: #c8c0a8; spacing: 6px; }
        QCheckBox::indicator { width:16px; height:16px; border:1px solid #4060b0;
            border-radius:3px; background:#1a2548; }
        QCheckBox::indicator:checked { background:#3060c0; }
        QSpinBox { background:#1a2548; border:1px solid #2e3f70;
            border-radius:5px; color:#ddd8c8; padding:4px; }
        QPushButton { background-color:#1f3070; color:#c8aa50;
            border:1px solid #2e4090; border-radius:6px; padding:6px 14px; }
        QPushButton:hover { background-color:#283d8a; }
        QTabWidget::pane { border:1px solid #2e3f70; border-radius:6px; background:#131e42; }
        QTabBar::tab { background:#1a2548; color:#8898c0; padding:6px 16px;
            border-top-left-radius:6px; border-top-right-radius:6px; }
        QTabBar::tab:selected { background:#131e42; color:#c8aa50; }
        QLabel { color:#c8c0a8; }
    )");
}
