// ============================================================
//  Eagle Library — SettingsDialog.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "SettingsDialog.h"
#include "AppConfig.h"
#include "LanguageManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QLabel>
#include <QTabWidget>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalBlocker>
#include <QDesktopServices>
#include <QUrl>

namespace {

QString trl(const QString& key, const QString& fallback)
{
    return LanguageManager::instance().text(key, fallback);
}

}

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    m_initialLanguageCode = LanguageManager::instance().currentLanguage();
    setMinimumSize(680, 520);
    setModal(true);
    setupUi();
    applyStyles();
    retranslateUi();
    loadSettings();

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged, this, [this]() {
        retranslateUi();
    });
}

void SettingsDialog::setupUi()
{
    auto* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(16,16,16,16);
    mainLay->setSpacing(12);

    m_tabs = new QTabWidget;

    // ── Tab: Library Folders ──────────────────────────────────
    auto* folderTab = new QWidget;
    auto* folderLay = new QVBoxLayout(folderTab);
    folderLay->setSpacing(12);

    auto* libraryRow = new QHBoxLayout;
    m_libraryLabel = new QLabel;
    m_libraryProfileCombo = new QComboBox;
    m_addLibraryBtn = new QPushButton;
    m_renameLibraryBtn = new QPushButton;
    m_removeLibraryBtn = new QPushButton;
    libraryRow->addWidget(m_libraryLabel);
    libraryRow->addWidget(m_libraryProfileCombo, 1);
    libraryRow->addWidget(m_addLibraryBtn);
    libraryRow->addWidget(m_renameLibraryBtn);
    libraryRow->addWidget(m_removeLibraryBtn);
    folderLay->addLayout(libraryRow);

    m_profileInfoLabel = new QLabel;
    m_profileInfoLabel->setWordWrap(true);
    folderLay->addWidget(m_profileInfoLabel);

    m_folderList = new QListWidget;
    m_folderList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    folderLay->addWidget(m_folderList);

    auto* btnRow = new QHBoxLayout;
    m_addFolderBtn = new QPushButton;
    m_removeFolderBtn = new QPushButton;
    btnRow->addWidget(m_addFolderBtn);
    btnRow->addWidget(m_removeFolderBtn);
    btnRow->addStretch();
    folderLay->addLayout(btnRow);

    connect(m_addFolderBtn, &QPushButton::clicked, this, &SettingsDialog::addFolder);
    connect(m_removeFolderBtn, &QPushButton::clicked, this, &SettingsDialog::removeFolder);
    connect(m_addLibraryBtn, &QPushButton::clicked, this, &SettingsDialog::addLibraryProfile);
    connect(m_renameLibraryBtn, &QPushButton::clicked, this, &SettingsDialog::renameLibraryProfile);
    connect(m_removeLibraryBtn, &QPushButton::clicked, this, &SettingsDialog::removeLibraryProfile);
    connect(m_libraryProfileCombo, &QComboBox::currentTextChanged, this, [this](const QString& name) {
        loadProfileIntoUi(name);
    });
    m_tabs->addTab(folderTab, QString());

    // ── Tab: Options ─────────────────────────────────────────
    auto* optTab = new QWidget;
    auto* optLay = new QVBoxLayout(optTab);
    optLay->setSpacing(14);

    m_behaviorBox = new QGroupBox;
    auto* behaviorForm = new QFormLayout(m_behaviorBox);
    behaviorForm->setSpacing(14);

    m_autoMetaCheck  = new QCheckBox;
    m_autoCoverCheck = new QCheckBox;
    m_autoEnrichAfterScanCheck = new QCheckBox;
    m_fastScanModeCheck = new QCheckBox;
    m_iconSizeSpin   = new QSpinBox;
    m_iconSizeSpin->setRange(100, 280);
    m_iconSizeSpin->setSingleStep(20);
    m_iconSizeSpin->setSuffix(" px");
    auto* threadsWidget = new QWidget;
    auto* threadsLayout = new QHBoxLayout(threadsWidget);
    threadsLayout->setContentsMargins(0, 0, 0, 0);
    threadsLayout->setSpacing(8);
    m_scanThreadsPresetCombo = new QComboBox;
    m_scanThreadsSpin = new QSpinBox;
    m_scanThreadsSpin->setRange(1, 64);
    m_scanThreadsSpin->setSuffix(" workers");
    m_scanThreadsSpin->setVisible(false);
    threadsLayout->addWidget(m_scanThreadsPresetCombo, 1);
    threadsLayout->addWidget(m_scanThreadsSpin);
    connect(m_scanThreadsPresetCombo, &QComboBox::currentIndexChanged, this, [this]() {
        const bool custom = m_scanThreadsPresetCombo->currentData().toInt() < 0;
        m_scanThreadsSpin->setVisible(custom);
    });

    behaviorForm->addRow(QString(), m_autoMetaCheck);
    behaviorForm->addRow(QString(), m_autoCoverCheck);
    behaviorForm->addRow(QString(), m_autoEnrichAfterScanCheck);
    behaviorForm->addRow(QString(), m_fastScanModeCheck);
    behaviorForm->addRow(QStringLiteral("__grid_card_width__"), m_iconSizeSpin);
    behaviorForm->addRow(QStringLiteral("__scan_threads__"), threadsWidget);
    optLay->addWidget(m_behaviorBox);

    m_interfaceBox = new QGroupBox;
    auto* interfaceForm = new QFormLayout(m_interfaceBox);
    interfaceForm->setSpacing(14);

    m_startViewCombo = new QComboBox;
    m_compactModeCheck = new QCheckBox;
    m_showSidebarCheck = new QCheckBox;
    m_showSmartCategoriesCheck = new QCheckBox;
    m_rememberWindowCheck = new QCheckBox;
    m_languageCombo = new QComboBox;
    m_openLanguageFolderBtn = new QPushButton;
    auto* languageRow = new QWidget;
    auto* languageLayout = new QHBoxLayout(languageRow);
    languageLayout->setContentsMargins(0, 0, 0, 0);
    languageLayout->setSpacing(8);
    languageLayout->addWidget(m_languageCombo, 1);
    languageLayout->addWidget(m_openLanguageFolderBtn);

    interfaceForm->addRow(QStringLiteral("__startup_view__"), m_startViewCombo);
    interfaceForm->addRow("", m_compactModeCheck);
    interfaceForm->addRow("", m_showSidebarCheck);
    interfaceForm->addRow("", m_showSmartCategoriesCheck);
    interfaceForm->addRow("", m_rememberWindowCheck);
    interfaceForm->addRow(QStringLiteral("__language__"), languageRow);
    optLay->addWidget(m_interfaceBox);

    connect(m_openLanguageFolderBtn, &QPushButton::clicked, this, []() {
        const QStringList directories = LanguageManager::instance().packDirectories();
        if (!directories.isEmpty()) {
            QDir().mkpath(directories.first());
            QDesktopServices::openUrl(QUrl::fromLocalFile(directories.first()));
        }
    });
    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, [this]() {
        const QString code = m_languageCombo->currentData().toString();
        if (!code.isEmpty())
            LanguageManager::instance().applyLanguage(code);
    });

    m_languageHintLabel = new QLabel;
    m_languageHintLabel->setWordWrap(true);
    m_languageHintLabel->setObjectName("settingsNote");
    optLay->addWidget(m_languageHintLabel);

    m_layoutHintLabel = new QLabel;
    m_layoutHintLabel->setWordWrap(true);
    m_layoutHintLabel->setObjectName("settingsNote");
    optLay->addWidget(m_layoutHintLabel);
    optLay->addStretch();
    m_tabs->addTab(optTab, QString());

    // ── Tab: About ────────────────────────────────────────────
    auto* aboutTab = new QWidget;
    auto* aboutLay = new QVBoxLayout(aboutTab);
    aboutLay->setAlignment(Qt::AlignCenter);

    auto* logoLbl = new QLabel("EAGLE");
    logoLbl->setStyleSheet("font-size: 28px; font-weight: 700; letter-spacing: 3px; color: #efdb9d;");
    logoLbl->setAlignment(Qt::AlignCenter);

    m_aboutNameLabel = new QLabel;
    m_aboutNameLabel->setAlignment(Qt::AlignCenter);

    m_aboutVersionLabel = new QLabel;
    m_aboutVersionLabel->setAlignment(Qt::AlignCenter);

    m_aboutCopyrightLabel = new QLabel;
    m_aboutCopyrightLabel->setAlignment(Qt::AlignCenter);
    m_aboutCopyrightLabel->setStyleSheet("color: #7080a0; font-size: 10px;");

    m_aboutWebsiteLabel = new QLabel(
        QString("<a href='%1' style='color:#4a90d0;'>%1</a>").arg("https://eaglesoftware.biz/"));
    m_aboutWebsiteLabel->setOpenExternalLinks(true);
    m_aboutWebsiteLabel->setAlignment(Qt::AlignCenter);

    aboutLay->addStretch();
    aboutLay->addWidget(logoLbl);
    aboutLay->addWidget(m_aboutNameLabel);
    aboutLay->addWidget(m_aboutVersionLabel);
    aboutLay->addSpacing(8);
    aboutLay->addWidget(m_aboutCopyrightLabel);
    aboutLay->addWidget(m_aboutWebsiteLabel);
    aboutLay->addStretch();
    m_tabs->addTab(aboutTab, QString());

    mainLay->addWidget(m_tabs);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, [this]() { saveSettings(); accept(); });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, [this]() {
        LanguageManager::instance().applyLanguage(m_initialLanguageCode);
        reject();
    });
    mainLay->addWidget(m_buttonBox);
}

void SettingsDialog::addFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, trl("settings.selectBookFolder", "Select Book Folder"),
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

void SettingsDialog::syncCurrentProfileFromUi()
{
    if (!m_libraryProfileCombo || m_libraryProfileCombo->currentText().trimmed().isEmpty())
        return;

    QJsonArray folders;
    for (int i = 0; i < m_folderList->count(); ++i)
        folders.append(m_folderList->item(i)->text());
    m_libraryProfiles[m_libraryProfileCombo->currentText().trimmed()] = folders;
}

void SettingsDialog::loadProfileIntoUi(const QString& name)
{
    if (!m_lastLoadedProfile.isEmpty() && m_lastLoadedProfile != name) {
        QJsonArray folders;
        for (int i = 0; i < m_folderList->count(); ++i)
            folders.append(m_folderList->item(i)->text());
        m_libraryProfiles[m_lastLoadedProfile] = folders;
    }

    m_folderList->clear();
    const QJsonArray folders = m_libraryProfiles.value(name).toArray();
    for (const QJsonValue& folder : folders)
        m_folderList->addItem(folder.toString());
    m_lastLoadedProfile = name;
}

void SettingsDialog::addLibraryProfile()
{
    const QString name = QInputDialog::getText(this, trl("settings.newLibrary", "New Library"), trl("settings.libraryProfile", "Library name:"));
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || m_libraryProfiles.contains(trimmed))
        return;
    syncCurrentProfileFromUi();
    m_libraryProfiles[trimmed] = QJsonArray{};
    m_libraryProfileCombo->addItem(trimmed);
    m_libraryProfileCombo->setCurrentText(trimmed);
}

void SettingsDialog::removeLibraryProfile()
{
    const QString current = m_libraryProfileCombo->currentText().trimmed();
    if (current.isEmpty())
        return;
    if (m_libraryProfileCombo->count() <= 1)
        return;
    m_libraryProfiles.remove(current);
    m_libraryProfileCombo->removeItem(m_libraryProfileCombo->currentIndex());
}

void SettingsDialog::renameLibraryProfile()
{
    const QString current = m_libraryProfileCombo->currentText().trimmed();
    if (current.isEmpty())
        return;
    const QString renamed = QInputDialog::getText(this, trl("settings.rename", "Rename Library"), trl("settings.libraryProfile", "Library name:"), QLineEdit::Normal, current).trimmed();
    if (renamed.isEmpty() || renamed == current || m_libraryProfiles.contains(renamed))
        return;
    syncCurrentProfileFromUi();
    m_libraryProfiles[renamed] = m_libraryProfiles.value(current);
    m_libraryProfiles.remove(current);
    const int idx = m_libraryProfileCombo->currentIndex();
    m_libraryProfileCombo->setItemText(idx, renamed);
    m_libraryProfileCombo->setCurrentIndex(idx);
}

void SettingsDialog::removeFolder()
{
    qDeleteAll(m_folderList->selectedItems());
}

void SettingsDialog::loadSettings()
{
    QSettings s("Eagle Software", "Eagle Library");
    const QString currentLibrary = s.value("library/currentName", "Main Library").toString();
    const QJsonDocument profilesDoc = QJsonDocument::fromJson(s.value("library/profiles").toByteArray());
    if (profilesDoc.isObject())
        m_libraryProfiles = profilesDoc.object();
    if (m_libraryProfiles.isEmpty()) {
        QJsonArray folders;
        for (const QString& folder : s.value("library/folders").toStringList())
            folders.append(folder);
        m_libraryProfiles.insert(currentLibrary, folders);
    }
    for (const QString& key : m_libraryProfiles.keys())
        m_libraryProfileCombo->addItem(key);
    if (m_libraryProfileCombo->count() == 0) {
        m_libraryProfiles.insert("Main Library", QJsonArray{});
        m_libraryProfileCombo->addItem("Main Library");
    }
    const int libraryIndex = qMax(0, m_libraryProfileCombo->findText(currentLibrary));
    m_libraryProfileCombo->setCurrentIndex(libraryIndex);
    loadProfileIntoUi(m_libraryProfileCombo->currentText());
    m_autoMetaCheck->setChecked(s.value("options/autoMeta", true).toBool());
    m_autoCoverCheck->setChecked(s.value("options/autoCover", true).toBool());
    m_autoEnrichAfterScanCheck->setChecked(s.value("options/autoEnrichAfterScan", true).toBool());
    m_fastScanModeCheck->setChecked(s.value("options/fastScanMode", true).toBool());
    m_iconSizeSpin->setValue(s.value("options/iconSize", 160).toInt());
    const int scanThreads = s.value("options/scanThreads", 0).toInt();
    int presetIndex = m_scanThreadsPresetCombo->findData(scanThreads);
    if (presetIndex < 0) {
        presetIndex = m_scanThreadsPresetCombo->findData(-1);
        m_scanThreadsSpin->setValue(qMax(1, scanThreads));
    } else if (scanThreads > 0) {
        m_scanThreadsSpin->setValue(scanThreads);
    }
    {
        QSignalBlocker blocker(m_scanThreadsPresetCombo);
        m_scanThreadsPresetCombo->setCurrentIndex(qMax(0, presetIndex));
    }
    m_scanThreadsSpin->setVisible(m_scanThreadsPresetCombo->currentData().toInt() < 0);
    const QString startMode = s.value("view/startMode", "remember").toString();
    const int startIndex = qMax(0, m_startViewCombo->findData(startMode));
    m_startViewCombo->setCurrentIndex(startIndex);
    m_compactModeCheck->setChecked(s.value("view/compactMode", false).toBool());
    m_showSidebarCheck->setChecked(s.value("view/showSidebar", true).toBool());
    m_showSmartCategoriesCheck->setChecked(s.value("view/showSmartCategories", false).toBool());
    m_rememberWindowCheck->setChecked(s.value("view/rememberWindowState", true).toBool());

    const QString configuredLanguage = s.value("ui/language", LanguageManager::instance().currentLanguage()).toString();
    {
        QSignalBlocker blocker(m_languageCombo);
        m_languageCombo->clear();
        for (const LanguageInfo& info : LanguageManager::instance().availableLanguages())
            m_languageCombo->addItem(QString("%1 (%2)").arg(info.nativeName, info.name), info.code);
        const int langIndex = qMax(0, m_languageCombo->findData(configuredLanguage));
        m_languageCombo->setCurrentIndex(langIndex);
    }
}

void SettingsDialog::saveSettings()
{
    QSettings s("Eagle Software", "Eagle Library");
    syncCurrentProfileFromUi();
    s.setValue("library/currentName", currentLibraryName());
    s.setValue("library/profiles", QJsonDocument(m_libraryProfiles).toJson(QJsonDocument::Compact));
    s.setValue("library/folders",   watchedFolders());
    s.setValue("options/autoMeta",  m_autoMetaCheck->isChecked());
    s.setValue("options/autoCover", m_autoCoverCheck->isChecked());
    s.setValue("options/autoEnrichAfterScan", m_autoEnrichAfterScanCheck->isChecked());
    s.setValue("options/fastScanMode", m_fastScanModeCheck->isChecked());
    s.setValue("options/iconSize",  m_iconSizeSpin->value());
    s.setValue("options/scanThreads", scanThreads());
    s.setValue("view/startMode", m_startViewCombo->currentData().toString());
    s.setValue("view/compactMode", m_compactModeCheck->isChecked());
    s.setValue("view/showSidebar", m_showSidebarCheck->isChecked());
    s.setValue("view/showSmartCategories", m_showSmartCategoriesCheck->isChecked());
    s.setValue("view/rememberWindowState", m_rememberWindowCheck->isChecked());
    s.setValue("ui/language", selectedLanguage());
    m_initialLanguageCode = selectedLanguage();
}

QStringList SettingsDialog::watchedFolders() const
{
    QStringList list;
    for (int i = 0; i < m_folderList->count(); ++i)
        list << m_folderList->item(i)->text();
    return list;
}

QString SettingsDialog::currentLibraryName() const
{
    return m_libraryProfileCombo ? m_libraryProfileCombo->currentText().trimmed() : QString();
}

bool SettingsDialog::autoFetchMetadata() const { return m_autoMetaCheck->isChecked(); }
bool SettingsDialog::autoFetchCovers()   const { return m_autoCoverCheck->isChecked(); }
bool SettingsDialog::autoEnrichAfterScan() const { return m_autoEnrichAfterScanCheck->isChecked(); }
bool SettingsDialog::fastScanMode() const { return m_fastScanModeCheck->isChecked(); }
int  SettingsDialog::gridIconSize()      const { return m_iconSizeSpin->value(); }
int  SettingsDialog::scanThreads()       const
{
    if (!m_scanThreadsPresetCombo)
        return 0;
    const int preset = m_scanThreadsPresetCombo->currentData().toInt();
    return preset >= 0 ? preset : m_scanThreadsSpin->value();
}
QString SettingsDialog::startViewMode()  const { return m_startViewCombo->currentData().toString(); }
bool SettingsDialog::compactMode()       const { return m_compactModeCheck->isChecked(); }
bool SettingsDialog::showSidebar()       const { return m_showSidebarCheck->isChecked(); }
bool SettingsDialog::showSmartCategories() const { return m_showSmartCategoriesCheck->isChecked(); }
bool SettingsDialog::rememberWindowState() const { return m_rememberWindowCheck->isChecked(); }
QString SettingsDialog::selectedLanguage() const
{
    return m_languageCombo ? m_languageCombo->currentData().toString() : QStringLiteral("en");
}

void SettingsDialog::retranslateUi()
{
    setWindowTitle(trl("settings.windowTitle", "Settings - Eagle Library"));

    if (m_tabs) {
        m_tabs->setTabText(0, trl("settings.tab.folders", "Library Folders"));
        m_tabs->setTabText(1, trl("settings.tab.options", "Options"));
        m_tabs->setTabText(2, trl("settings.tab.about", "About"));
    }

    if (m_libraryLabel) m_libraryLabel->setText(trl("settings.libraryProfile", "Library profile:"));
    if (m_addLibraryBtn) m_addLibraryBtn->setText(trl("settings.newLibrary", "New Library..."));
    if (m_renameLibraryBtn) m_renameLibraryBtn->setText(trl("settings.rename", "Rename"));
    if (m_removeLibraryBtn) m_removeLibraryBtn->setText(trl("settings.remove", "Remove"));
    if (m_profileInfoLabel) m_profileInfoLabel->setText(trl("settings.profileInfo", "Each library profile owns its own watched folders. Shelves remain virtual inside the app, while scans only read the folders of the active library."));
    if (m_addFolderBtn) m_addFolderBtn->setText(trl("settings.addFolder", "Add Folder..."));
    if (m_removeFolderBtn) m_removeFolderBtn->setText(trl("settings.remove", "Remove"));

    if (m_behaviorBox) m_behaviorBox->setTitle(trl("settings.group.behavior", "Library Behavior"));
    if (m_autoMetaCheck) m_autoMetaCheck->setText(trl("settings.autoMeta", "Automatically fetch metadata from internet"));
    if (m_autoCoverCheck) m_autoCoverCheck->setText(trl("settings.autoCover", "Automatically download book covers"));
    if (m_autoEnrichAfterScanCheck) m_autoEnrichAfterScanCheck->setText(trl("settings.autoEnrich", "After scan, enrich only newly added books that still miss DB info"));
    if (m_fastScanModeCheck) {
        m_fastScanModeCheck->setText(trl("settings.fastScan", "Fast scan mode: index files first and defer deep metadata probing"));
        m_fastScanModeCheck->setToolTip(trl("settings.fastScanTip", "Recommended for large libraries. Scans become much faster because embedded PDF and OCR-style probing are delayed until repair, enrich, or smart rename tools are used."));
    }
    m_scanThreadsPresetCombo->setToolTip(trl("settings.scanWorkersTip", "Use Auto for most systems. Lower values keep the PC responsive; higher values scan faster."));
    m_scanThreadsSpin->setToolTip(trl("settings.customWorkersTip", "Used only when the Custom preset is selected."));

    {
        const int currentData = m_scanThreadsPresetCombo->currentData().toInt();
        QSignalBlocker blocker(m_scanThreadsPresetCombo);
        m_scanThreadsPresetCombo->clear();
        m_scanThreadsPresetCombo->addItem(trl("settings.autoRecommended", "Auto (Recommended)"), 0);
        m_scanThreadsPresetCombo->addItem(trl("settings.gentle", "Gentle - 2 workers"), 2);
        m_scanThreadsPresetCombo->addItem(trl("settings.balanced", "Balanced - 4 workers"), 4);
        m_scanThreadsPresetCombo->addItem(trl("settings.fast", "Fast - 8 workers"), 8);
        m_scanThreadsPresetCombo->addItem(trl("settings.veryFast", "Very Fast - 12 workers"), 12);
        m_scanThreadsPresetCombo->addItem(trl("settings.custom", "Custom"), -1);
        const int idx = m_scanThreadsPresetCombo->findData(currentData);
        m_scanThreadsPresetCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }

    if (m_interfaceBox) m_interfaceBox->setTitle(trl("settings.group.interface", "Interface"));
    {
        const QString currentStartMode = m_startViewCombo->currentData().toString();
        QSignalBlocker blocker(m_startViewCombo);
        m_startViewCombo->clear();
        m_startViewCombo->addItem(trl("settings.rememberLastView", "Remember last view"), "remember");
        m_startViewCombo->addItem(trl("settings.alwaysGrid", "Always start in grid view"), "grid");
        m_startViewCombo->addItem(trl("settings.alwaysList", "Always start in list view"), "list");
        const int idx = m_startViewCombo->findData(currentStartMode);
        m_startViewCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    if (m_compactModeCheck) m_compactModeCheck->setText(trl("settings.compactMode", "Use denser cards and rows to fit smaller screens"));
    if (m_showSidebarCheck) m_showSidebarCheck->setText(trl("settings.showSidebar", "Show the browse sidebar on wide screens"));
    if (m_showSmartCategoriesCheck) m_showSmartCategoriesCheck->setText(trl("settings.showSmartCategories", "Show smart category shortcuts in the sidebar"));
    if (m_rememberWindowCheck) m_rememberWindowCheck->setText(trl("settings.rememberWindowState", "Remember window size and position"));
    if (m_openLanguageFolderBtn) m_openLanguageFolderBtn->setText(trl("settings.openPackFolder", "Open Language Packs Folder"));
    if (m_languageHintLabel) m_languageHintLabel->setText(trl("settings.languageHint", "Language changes apply immediately. Add custom JSON language packs in the translations folder to extend Eagle Library."));
    if (m_layoutHintLabel) m_layoutHintLabel->setText(trl("settings.layoutHint", "Compact mode and the adaptive layout help Eagle stay usable on smaller laptops, tablets, and high-DPI displays."));
    if (m_aboutNameLabel) m_aboutNameLabel->setText(QString("<b style='font-size:18px;color:#c8aa50;'>%1</b>").arg(AppConfig::appName()));
    if (m_aboutVersionLabel) m_aboutVersionLabel->setText(trl("settings.about.version", "Version %1").arg(AppConfig::version()));
    if (m_aboutCopyrightLabel) m_aboutCopyrightLabel->setText(trl("settings.about.copyright", "Copyright (C) 2026 Eagle Software. All rights reserved."));

    if (m_buttonBox) {
        if (QAbstractButton* okButton = m_buttonBox->button(QDialogButtonBox::Ok))
            okButton->setText(trl("dialog.ok", "OK"));
        if (QAbstractButton* cancelButton = m_buttonBox->button(QDialogButtonBox::Cancel))
            cancelButton->setText(trl("dialog.cancel", "Cancel"));
    }

    auto updateFormLabels = [](QFormLayout* form, const QString& key, const QString& text) {
        for (int row = 0; row < form->rowCount(); ++row) {
            QLayoutItem* labelItem = form->itemAt(row, QFormLayout::LabelRole);
            QLabel* label = labelItem ? qobject_cast<QLabel*>(labelItem->widget()) : nullptr;
            if (label && label->text() == key) {
                label->setText(text);
                return;
            }
        }
    };
    if (auto* behaviorForm = qobject_cast<QFormLayout*>(m_behaviorBox->layout())) {
        updateFormLabels(behaviorForm, QStringLiteral("__grid_card_width__"), trl("settings.gridCardWidth", "Grid card width:"));
        updateFormLabels(behaviorForm, QStringLiteral("__scan_threads__"), trl("settings.scanWorkers", "Scan worker threads:"));
    }
    if (auto* interfaceForm = qobject_cast<QFormLayout*>(m_interfaceBox->layout())) {
        updateFormLabels(interfaceForm, QStringLiteral("__startup_view__"), trl("settings.startupView", "Startup view:"));
        updateFormLabels(interfaceForm, QStringLiteral("__language__"), trl("settings.language", "Language:"));
    }
}

void SettingsDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog, QWidget {
            background-color: #0c1727;
            color: #eadfc3;
        }
        QListWidget {
            background: #122134;
            border: 1px solid #2c5378;
            border-radius: 10px;
            color: #eadfc3;
            padding: 6px;
        }
        QListWidget::item {
            padding: 8px 10px;
            border-radius: 7px;
            margin: 2px 0;
        }
        QListWidget::item:selected { background: #244d75; color: #fff4d8; }
        QCheckBox { color: #d9cfb2; spacing: 8px; }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #44729c;
            border-radius: 4px;
            background: #122134;
        }
        QCheckBox::indicator:checked { background: #2a628f; }
        QSpinBox {
            background: #122134;
            border: 1px solid #2c5378;
            border-radius: 8px;
            color: #eadfc3;
            padding: 6px 8px;
        }
        QComboBox {
            background: #122134;
            border: 1px solid #2c5378;
            border-radius: 8px;
            color: #eadfc3;
            padding: 6px 8px;
            min-height: 18px;
        }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView {
            background: #0f1d2f;
            border: 1px solid #2c5378;
            color: #eadfc3;
            selection-background-color: #244d75;
        }
        QPushButton {
            background-color: #1c4264;
            color: #edd690;
            border: 1px solid #31648f;
            border-radius: 8px;
            padding: 8px 14px;
            font-weight: 600;
        }
        QPushButton:hover { background-color: #275983; }
        QTabWidget::pane {
            border: 1px solid #25486b;
            border-radius: 10px;
            background: #0f1d2f;
            top: -1px;
        }
        QTabBar::tab {
            background: #132235;
            color: #8ea5bf;
            padding: 8px 18px;
            margin-right: 4px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            font-weight: 600;
        }
        QTabBar::tab:selected { background: #0f1d2f; color: #efdb9d; }
        QLabel { color: #d7ccb0; }
        QLabel#settingsNote { color: #8fa7c2; padding: 0 4px; }
        QGroupBox {
            border: 1px solid #25486b;
            border-radius: 10px;
            margin-top: 12px;
            padding-top: 12px;
            font-weight: 600;
            color: #efdb9d;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
        }
        QDialogButtonBox QPushButton { min-width: 96px; }
    )");
}
