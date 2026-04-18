#pragma once
// ============================================================
//  Eagle Library — SettingsDialog.h
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================
#include <QDialog>
#include <QListWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    QStringList watchedFolders() const;
    bool        autoFetchMetadata() const;
    bool        autoFetchCovers() const;
    int         gridIconSize() const;

private:
    QListWidget* m_folderList;
    QCheckBox*   m_autoMetaCheck;
    QCheckBox*   m_autoCoverCheck;
    QSpinBox*    m_iconSizeSpin;

    void setupUi();
    void applyStyles();
    void addFolder();
    void removeFolder();
    void loadSettings();
    void saveSettings();
};
