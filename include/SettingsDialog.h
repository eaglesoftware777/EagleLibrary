#pragma once
// ============================================================
//  Eagle Library — SettingsDialog.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include <QDialog>
#include <QListWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QString>
#include <QStringList>
#include <QJsonObject>

class QLabel;
class QPushButton;
class QTabWidget;
class QGroupBox;
class QDialogButtonBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    QStringList watchedFolders() const;
    QString     currentLibraryName() const;
    bool        autoFetchMetadata() const;
    bool        autoFetchCovers() const;
    bool        autoEnrichAfterScan() const;
    bool        fastScanMode() const;
    int         gridIconSize() const;
    int         scanThreads() const;
    QString     startViewMode() const;
    bool        compactMode() const;
    bool        showSidebar() const;
    bool        showSmartCategories() const;
    bool        rememberWindowState() const;
    QString     selectedLanguage() const;

private:
    QTabWidget*  m_tabs = nullptr;
    QComboBox*   m_libraryProfileCombo;
    QLabel*      m_libraryLabel;
    QPushButton* m_addLibraryBtn;
    QPushButton* m_renameLibraryBtn;
    QPushButton* m_removeLibraryBtn;
    QLabel*      m_profileInfoLabel;
    QListWidget* m_folderList;
    QPushButton* m_addFolderBtn;
    QPushButton* m_removeFolderBtn;
    QCheckBox*   m_autoMetaCheck;
    QCheckBox*   m_autoCoverCheck;
    QCheckBox*   m_autoEnrichAfterScanCheck;
    QCheckBox*   m_fastScanModeCheck;
    QSpinBox*    m_iconSizeSpin;
    QComboBox*   m_scanThreadsPresetCombo;
    QSpinBox*    m_scanThreadsSpin;
    QComboBox*   m_startViewCombo;
    QCheckBox*   m_compactModeCheck;
    QCheckBox*   m_showSidebarCheck;
    QCheckBox*   m_showSmartCategoriesCheck;
    QCheckBox*   m_rememberWindowCheck;
    QComboBox*   m_languageCombo;
    QPushButton* m_openLanguageFolderBtn;
    QLabel*      m_languageHintLabel;
    QLabel*      m_layoutHintLabel;
    QLabel*      m_aboutNameLabel;
    QLabel*      m_aboutVersionLabel;
    QLabel*      m_aboutCopyrightLabel;
    QLabel*      m_aboutWebsiteLabel;
    QGroupBox*   m_behaviorBox = nullptr;
    QGroupBox*   m_interfaceBox = nullptr;
    QDialogButtonBox* m_buttonBox = nullptr;
    QJsonObject  m_libraryProfiles;
    QString      m_lastLoadedProfile;
    QString      m_initialLanguageCode;

    void setupUi();
    void applyStyles();
    void retranslateUi();
    void syncCurrentProfileFromUi();
    void loadProfileIntoUi(const QString& name);
    void addLibraryProfile();
    void removeLibraryProfile();
    void renameLibraryProfile();
    void addFolder();
    void removeFolder();
    void loadSettings();
    void saveSettings();
};
