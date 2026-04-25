#pragma once
// ============================================================
//  Eagle Library -- MainWindow.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include <QMainWindow>
#include <QListView>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QToolBar>
#include <QDockWidget>
#include <QComboBox>
#include <QAction>
#include <QThread>
#include <QSet>
#include <QJsonObject>
#include <QQueue>
#include <QFutureWatcher>
#include <QPointer>
#include <functional>

#include "Book.h"
#include "BookModel.h"
#include "LibraryScanner.h"
#include "MetadataFetcher.h"
#include "CoverDownloader.h"
#include "ThemeManager.h"
#include "CommandPalette.h"
#include "PluginManager.h"

class BookDelegate;
class QCloseEvent;
class QResizeEvent;
class QPushButton;
class QVBoxLayout;
class QFileSystemWatcher;
class QTimer;
class QScrollArea;
class QToolButton;
class QListWidget;
class QFrame;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    // Scan
    void onScanStart();
    void onScanFinished(int added, int skipped);
    void onBookFound(Book book);
    void onScanProgress(int done, int total, const QString& file);
    // Metadata / covers
    void onMetadataReady(qint64 id, Book updated);
    void onCoverReady(qint64 id, const QString& path);
    void onCoverUrlsReady(qint64 id, const QStringList& urls);
    void onMetadataFetchProgress(int completed, int total, const QString& currentFile, const QString& stage);
    void onMetadataFetchError(qint64 id, const QString& msg);
    void onCoverDownloadProgress(int completed, int total, const QString& currentLabel);
    void onCoverDownloadFailed(qint64 id, const QString& reason);
    // Smart rename
    void onSmartRenameAll();
    void onSmartRenameSelected();
    void onRenameResult(RenameResult r);
    void onRenameFinished(int changed);
    // UI
    void openSettings();
    void openBookDetail(const QModelIndex& index);
    void openBookDetail();
    void setGridView();
    void setListView();
    void searchChanged(const QString& text);
    void filterByFormat(const QString& fmt);
    void filterByCategory(const QString& category);
    void sortChanged(int idx);
    void showFavourites(bool on);
    void showNoCoverFilter(bool on);
    void showNoMetaFilter(bool on);
    void fetchAllMetadata();
    void openMetadataManager();
    void bulkEditMetadata();
    void enrichIncompleteBooksAction();
    void runQualityCheck();
    void findDuplicates();
    void extractMissingIsbns();
    void countPagesForLibrary();
    void generateMissingCovers();
    void normalizeCovers();
    void smartCategorizeLibrary();
    void removeSelectedBook();
    void cleanMissingFiles();
    void refreshLibrary();
    void clearCurrentLibraryAndRescan();
    void exportLibrarySnapshot();
    void importLibrarySnapshot();
    void importPreviousDatabaseBackup();
    void stopAllTasks();
    void consultDatabaseSummary();
    void diagnoseDatabaseText();
    void repairDatabaseText();
    void openExternalToolsDialog();
    void openDatabaseFolder();
    void openDatabaseEditor();
    void remapLibraryPaths();
    void openAdvancedSearch();
    void showAbout();
    void switchLibrary(const QString& libraryName);
    void applyShelf(const QString& shelfName);
    // Theme
    void switchTheme(const QString& name);
    // Command Palette
    void openCommandPalette();
    // Web Search
    void searchSelectedOnGoogle();
    void searchSelectedOnWeb(const QString& engine = "google");
    void lookupSelectedOnGoodreads();
    // Collections
    void manageCollections();
    void createCollection();
    void openReadingDashboard();
    void openLibraryDashboard();
    void addSelectedBooksToCollection();
    void removeSelectedBooksFromCollection();
    // Plugins
    void openPluginManager();
    void openTaskCenter();
    void runBatchTasks();
    void recoverFromHungState();
    // Advanced Search
    void openAdvancedSearchDialog();
    // Saved searches
    void loadSavedSearch(int id);
    void saveCurrentSearch();

private:
    // Models
    BookModel*       m_model       = nullptr;
    BookFilterModel* m_filterModel = nullptr;

    // Views
    QListView*    m_gridView     = nullptr;
    QListView*    m_listView     = nullptr;
    QStackedWidget* m_stack      = nullptr;
    BookDelegate* m_gridDelegate = nullptr;
    BookDelegate* m_listDelegate = nullptr;

    // Toolbar
    QLineEdit*  m_searchBox   = nullptr;
    QComboBox*  m_libraryCombo = nullptr;
    QComboBox*  m_shelfCombo = nullptr;
    QComboBox*  m_formatCombo = nullptr;
    QComboBox*  m_categoryCombo = nullptr;
    QComboBox*  m_sortCombo   = nullptr;
    QAction*    m_gridAction  = nullptr;
    QAction*    m_listAction  = nullptr;
    QAction*    m_favAction   = nullptr;
    QAction*    m_sortAscAct  = nullptr;
    QToolBar*   m_mainToolBar = nullptr;
    QDockWidget* m_sidebarDock = nullptr;
    QScrollArea* m_sidebarScrollArea = nullptr;
    QWidget*    m_sidebarContent = nullptr;
    QVBoxLayout* m_sidebarLayout = nullptr;
    QWidget*    m_smartCategorySection = nullptr;
    QVBoxLayout* m_smartCategoryButtonsLayout = nullptr;
    QWidget*    m_savedSearchSection = nullptr;
    QVBoxLayout* m_savedSearchButtonsLayout = nullptr;

    // Status
    QLabel*       m_statusLabel = nullptr;
    QLabel*       m_libraryStatsLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel*       m_scanLabel   = nullptr;
    QLabel*       m_taskQueueLabel = nullptr;
    QLabel*       m_sidebarStatsLabel = nullptr;
    QWidget*      m_workspaceHeader = nullptr;
    QLabel*       m_workspaceTitleLabel = nullptr;
    QLabel*       m_workspaceMetaLabel = nullptr;
    QLabel*       m_workspaceHintLabel = nullptr;
    QLabel*       m_workspaceLibraryChip = nullptr;
    QLabel*       m_workspaceViewChip = nullptr;
    QLabel*       m_workspaceActionChip = nullptr;
    QToolButton*  m_taskCenterButton = nullptr;
    QFrame*       m_taskToastFrame = nullptr;
    QLabel*       m_taskToastTitleLabel = nullptr;
    QLabel*       m_taskToastBodyLabel = nullptr;
    QLabel*       m_taskToastMetaLabel = nullptr;
    QTimer*       m_taskToastTimer = nullptr;
    // Backend
    LibraryScanner*  m_scanner          = nullptr;
    MetadataFetcher* m_metaFetcher      = nullptr;
    CoverDownloader* m_coverDownloader  = nullptr;
    QPointer<SmartRenamer>    m_activeRenamer;
    QPointer<QThread>         m_renameThread;

    // Command Palette
    CommandPalette* m_commandPalette = nullptr;
    // Plugin menu ref
    QMenu* m_pluginMenu = nullptr;
    // Theme action group
    QAction* m_themeBlueAct  = nullptr;
    QAction* m_themeWhiteAct = nullptr;
    QAction* m_themeMacAct   = nullptr;
    QActionGroup* m_languageActionGroup = nullptr;

    bool        m_isGridView  = true;
    SortOrder   m_sortOrder   = SortOrder::Asc;
    QString     m_currentLibraryName;
    QString     m_activeShelfName;
    QJsonObject m_libraryProfiles;
    QStringList m_watchFolders;
    bool        m_compactMode = false;
    bool        m_showSidebarPreference = true;
    bool        m_showSmartCategories = false;
    bool        m_autoEnrichAfterScan = true;
    bool        m_fastScanMode = true;
    bool        m_rememberWindowState = true;
    bool        m_isClosing = false;
    int         m_scanThreads = 0;
    QString     m_startViewMode = "remember";
    QSet<qint64> m_forceCoverFetchIds;
    QVector<Book> m_recentlyAddedBooks;
    QFileSystemWatcher* m_libraryWatcher = nullptr;
    QTimer*      m_libraryChangeTimer = nullptr;
    bool         m_incrementalScanPending = false;
    struct QueuedMenuTask {
        QString name;
        std::function<void()> run;
    };
    struct TaskNotice {
        QString title;
        QString message;
        QString detail;
        QString kind;
        QDateTime createdAt;
    };
    QQueue<QueuedMenuTask> m_menuTaskQueue;
    QQueue<int>  m_pendingTaskToastIndexes;
    QVector<TaskNotice> m_taskNotices;
    bool        m_menuTaskActive = false;
    QString     m_activeMenuTaskName;
    bool        m_metadataTaskWaitingForCovers = false;
    bool        m_taskPopupsEnabled = true;
    QTimer*     m_recoveryHeartbeatTimer = nullptr;
    QTimer*     m_stallWatchdogTimer = nullptr;
    QDateTime   m_lastProgressPulseUtc;
    QString     m_runtimeActivity;
    QString     m_runtimeDetail;
    bool        m_detectedUncleanShutdown = false;
    bool        m_stallRecoveryArmed = false;
    struct IsbnExtractionResult {
        Book book;
        QString isbn;
        bool invalidExisting = false;
        bool invalidCandidate = false;
    };
    QPointer<QFutureWatcher<IsbnExtractionResult>> m_isbnExtractionWatcher;
    bool        m_layoutRefreshQueued = false;

    void setupMenuBar();
    void setupToolBar();
    void setupSidebar();
    void setupViews();
    void setupStatusBar();
    void setupCommandPalette();
    void applyStyles();
    void retranslateUi();
    void loadSettings();
    void saveSettings();
    void updateStatusCount();
    void updateWorkspaceHeader();
    void refreshCategoryOptions();
    void applyResponsiveLayout();
    void setScanInteractionEnabled(bool enabled);
    void rebuildSmartCategorySidebar();
    void rebuildSavedSearchSidebar();
    void refreshLibrarySelector();
    void refreshShelfOptions();
    void reloadCurrentLibrary();
    void refreshLibraryWatcher();
    void scheduleIncrementalScan(const QString& changedPath = QString());
    bool bookNeedsEnrichment(const Book& book) const;
    void enrichIncompleteBooks(const QVector<Book>& books, const QString& title);
    void showTaskProgress(const QString& title, const QString& status, int current, int total, const QString& detail = QString());
    void hideTaskProgress(const QString& finalStatus = QString());
    void pushTaskNotice(const QString& title, const QString& message, const QString& detail = QString(), const QString& kind = QStringLiteral("info"));
    void updateTaskQueueIndicators();
    void positionTaskToast();
    void applyTaskToastVisual(const QString& kind);
    void showNextTaskToast();
    void hideTaskToast();
    bool backgroundWorkRunning() const;
    void queueOrRunMenuTask(const QString& name, std::function<void()> task);
    void startMenuTask(const QString& name, std::function<void()> task);
    void finishMenuTask(const QString& finalStatus = QString());
    void runNextQueuedMenuTask();
    void maybeFinishMetadataMenuTask();
    void setupRecoveryMonitor();
    void pulseRecovery(const QString& activity = QString(), const QString& detail = QString());
    void writeRecoverySnapshot(bool cleanShutdown = false, const QString& reason = QString()) const;
    void recoverFromHang(const QString& reason, bool automatic);
    void startScanNow();
    void scheduleMetadataFetch(const Book& book,
                               bool forceCover = false,
                               bool useEmbedded = true,
                               bool useGoogle = true,
                               bool useOpenLibrary = true);
    void startSmartRename(const QVector<Book>& books, const QString& title, const QString& prompt);
    QVector<Book> chooseBooksScope(const QString& featureName);
    QVector<Book> currentLibraryBooks(SortField sort = SortField::Title, SortOrder order = SortOrder::Asc) const;
    QVector<Book> selectedBooks() const;
    QString promptForNonEmptyName(const QString& title,
                                  const QString& label,
                                  const QString& initialValue = QString(),
                                  const QString& errorMessage = QString(),
                                  const QSet<QString>& forbiddenNames = {}) const;
    void ensureDefaultVirtualWorkspace();
    QString saveJsonArtifact(const QString& baseName, const QJsonObject& payload) const;
    void showReferenceLookup(const Book& book);
    void openBookFile(const QModelIndex& index);
    void openBookFile();
    QListView* currentView() const;
};
