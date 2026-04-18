#pragma once
// ============================================================
//  Eagle Library -- MainWindow.h
//  Copyright (c) 2024 Eagle Software. All rights reserved.
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

#include "Book.h"
#include "BookModel.h"
#include "LibraryScanner.h"
#include "MetadataFetcher.h"
#include "CoverDownloader.h"

class BookDelegate;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Scan
    void onScanStart();
    void onScanFinished(int added, int skipped);
    void onBookFound(Book book);
    void onScanProgress(int done, int total, const QString& file);
    // Metadata / covers
    void onMetadataReady(qint64 id, Book updated);
    void onCoverReady(qint64 id, const QString& path);
    void onCoverUrlReady(qint64 id, const QString& url);
    // Smart rename
    void onSmartRenameAll();
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
    void sortChanged(int idx);
    void showFavourites(bool on);
    void showNoCoverFilter(bool on);
    void showNoMetaFilter(bool on);
    void fetchAllMetadata();
    void removeSelectedBook();
    void cleanMissingFiles();
    void refreshLibrary();
    void showAbout();

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
    QComboBox*  m_formatCombo = nullptr;
    QComboBox*  m_sortCombo   = nullptr;
    QAction*    m_gridAction  = nullptr;
    QAction*    m_listAction  = nullptr;
    QAction*    m_favAction   = nullptr;
    QAction*    m_sortAscAct  = nullptr;

    // Status
    QLabel*       m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel*       m_scanLabel   = nullptr;

    // Backend
    LibraryScanner*  m_scanner          = nullptr;
    MetadataFetcher* m_metaFetcher      = nullptr;
    CoverDownloader* m_coverDownloader  = nullptr;
    SmartRenamer*    m_renamer          = nullptr;
    QThread*         m_renameThread     = nullptr;

    bool        m_isGridView  = true;
    SortOrder   m_sortOrder   = SortOrder::Asc;
    QStringList m_watchFolders;

    void setupMenuBar();
    void setupToolBar();
    void setupSidebar();
    void setupViews();
    void setupStatusBar();
    void applyStyles();
    void loadSettings();
    void saveSettings();
    void updateStatusCount();
    void scheduleMetadataFetch(const Book& book);
    QListView* currentView() const;
};
