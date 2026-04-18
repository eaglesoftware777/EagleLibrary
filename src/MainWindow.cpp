// ============================================================
//  Eagle Library -- MainWindow.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================
#include "MainWindow.h"
#include "BookDelegate.h"
#include "BookDetailDialog.h"
#include "SettingsDialog.h"
#include "Database.h"
#include "AppConfig.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QDockWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QScrollBar>
#include <QShortcut>
#include <QKeySequence>
#include <QGuiApplication>
#include <QScreen>
#include <QIcon>
#include <functional>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Eagle Library  --  Eagle Software");
    setMinimumSize(1100, 680);

    // Set application icon
    QIcon appIcon;
    appIcon.addFile(":/eagle_logo.png");
    if (!appIcon.isNull()) {
        setWindowIcon(appIcon);
        qApp->setWindowIcon(appIcon);
    }

    // Centre on screen
    if (QScreen* sc = QGuiApplication::primaryScreen()) {
        QRect sg = sc->availableGeometry();
        resize(qMin(1400, sg.width() - 80), qMin(880, sg.height() - 80));
        move(sg.center() - QPoint(width()/2, height()/2));
    }

    // Backend
    m_model        = new BookModel(this);
    m_filterModel  = new BookFilterModel(this);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setSortRole(TitleRole);
    m_filterModel->sort(0);

    m_scanner         = new LibraryScanner(this);
    m_renamer         = new SmartRenamer(this);
    m_metaFetcher     = new MetadataFetcher(this);
    m_coverDownloader = new CoverDownloader(AppConfig::coversDir(), this);

    // UI
    setupMenuBar();
    setupToolBar();
    setupViews();
    setupSidebar();
    setupStatusBar();
    applyStyles();
    loadSettings();

    // Signals
    connect(m_scanner, &LibraryScanner::bookFound,    this, &MainWindow::onBookFound);
    connect(m_scanner, &LibraryScanner::progress,     this, &MainWindow::onScanProgress);
    connect(m_scanner, &LibraryScanner::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_metaFetcher,     &MetadataFetcher::metadataReady, this, &MainWindow::onMetadataReady);
    connect(m_metaFetcher,     &MetadataFetcher::coverUrlReady, this, &MainWindow::onCoverUrlReady);
    connect(m_coverDownloader, &CoverDownloader::coverReady,    this, &MainWindow::onCoverReady);

    connect(m_renamer, &SmartRenamer::renamed,   this, &MainWindow::onRenameResult);
    connect(m_renamer, &SmartRenamer::finished,  this, &MainWindow::onRenameFinished);

    // Ctrl+F -> search
    auto* sch = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(sch, &QShortcut::activated, m_searchBox, qOverload<>(&QLineEdit::setFocus));

    m_model->loadAll();
    updateStatusCount();
}

MainWindow::~MainWindow() { saveSettings(); }

// ── Menu bar ─────────────────────────────────────────────────
void MainWindow::setupMenuBar()
{
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* settingsAct = new QAction("Settings...", this);
    settingsAct->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAct, &QAction::triggered, this, &MainWindow::openSettings);
    fileMenu->addAction(settingsAct);

    fileMenu->addSeparator();

    QAction* quitAct = new QAction("Quit", this);
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(quitAct);

    // Library menu
    QMenu* libMenu = menuBar()->addMenu("&Library");

    QAction* scanAct = new QAction("Scan Folders", this);
    scanAct->setShortcut(QKeySequence("F5"));
    connect(scanAct, &QAction::triggered, this, &MainWindow::onScanStart);
    libMenu->addAction(scanAct);

    QAction* fetchAct = new QAction("Fetch All Metadata", this);
    connect(fetchAct, &QAction::triggered, this, &MainWindow::fetchAllMetadata);
    libMenu->addAction(fetchAct);

    QAction* renameMenuAct = new QAction("Smart Rename All...", this);
    renameMenuAct->setShortcut(QKeySequence("Ctrl+R"));
    connect(renameMenuAct, &QAction::triggered, this, &MainWindow::onSmartRenameAll);
    libMenu->addAction(renameMenuAct);

    libMenu->addSeparator();

    QAction* cleanAct = new QAction("Remove Missing Files", this);
    connect(cleanAct, &QAction::triggered, this, &MainWindow::cleanMissingFiles);
    libMenu->addAction(cleanAct);

    libMenu->addSeparator();

    QAction* removeAct = new QAction("Remove Selected", this);
    removeAct->setShortcut(QKeySequence::Delete);
    connect(removeAct, &QAction::triggered, this, &MainWindow::removeSelectedBook);
    libMenu->addAction(removeAct);

    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");

    QAction* gridAct = new QAction("Grid View", this);
    gridAct->setShortcut(QKeySequence("Ctrl+G"));
    connect(gridAct, &QAction::triggered, this, &MainWindow::setGridView);
    viewMenu->addAction(gridAct);

    QAction* listAct = new QAction("List View", this);
    listAct->setShortcut(QKeySequence("Ctrl+L"));
    connect(listAct, &QAction::triggered, this, &MainWindow::setListView);
    viewMenu->addAction(listAct);

    viewMenu->addSeparator();

    QAction* refreshAct = new QAction("Refresh", this);
    connect(refreshAct, &QAction::triggered, this, &MainWindow::refreshLibrary);
    viewMenu->addAction(refreshAct);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");

    QAction* aboutAct = new QAction("About Eagle Library", this);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAct);

    QAction* webAct = new QAction("Visit Website", this);
    connect(webAct, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://eaglesoftware.biz/"));
    });
    helpMenu->addAction(webAct);
}

// ── Toolbar ──────────────────────────────────────────────────
void MainWindow::setupToolBar()
{
    QToolBar* tb = addToolBar("Main Toolbar");
    tb->setObjectName("mainToolBar");
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextOnly);

    // Scan
    QAction* scanAct = new QAction("Scan", this);
    scanAct->setToolTip("Scan library folders for new books (F5)");
    connect(scanAct, &QAction::triggered, this, &MainWindow::onScanStart);
    tb->addAction(scanAct);

    // Smart Rename
    QAction* renameAct = new QAction("Smart Rename", this);
    renameAct->setToolTip("Auto-rename books from filename patterns (Author - Title, Year...)");
    connect(renameAct, &QAction::triggered, this, &MainWindow::onSmartRenameAll);
    tb->addAction(renameAct);
    tb->addSeparator();

    // Grid / List
    m_gridAction = new QAction("Grid", this);
    m_gridAction->setCheckable(true);
    m_gridAction->setChecked(true);
    connect(m_gridAction, &QAction::triggered, this, &MainWindow::setGridView);
    tb->addAction(m_gridAction);

    m_listAction = new QAction("List", this);
    m_listAction->setCheckable(true);
    connect(m_listAction, &QAction::triggered, this, &MainWindow::setListView);
    tb->addAction(m_listAction);
    tb->addSeparator();

    // Favourites filter
    m_favAction = new QAction("Favourites", this);
    m_favAction->setCheckable(true);
    connect(m_favAction, &QAction::triggered, this, &MainWindow::showFavourites);
    tb->addAction(m_favAction);
    tb->addSeparator();

    // Format filter
    tb->addWidget(new QLabel("  Format: "));
    m_formatCombo = new QComboBox;
    m_formatCombo->setMinimumWidth(90);
    m_formatCombo->addItem("All");
    for (const char* fmt : {"PDF","EPUB","MOBI","AZW","AZW3","DjVu","CBZ","CBR","FB2","TXT"})
        m_formatCombo->addItem(fmt);
    connect(m_formatCombo, &QComboBox::currentTextChanged, this, &MainWindow::filterByFormat);
    tb->addWidget(m_formatCombo);
    tb->addSeparator();

    // Sort
    tb->addWidget(new QLabel("  Sort: "));
    m_sortCombo = new QComboBox;
    m_sortCombo->setMinimumWidth(110);
    m_sortCombo->addItem("Title",     (int)SortField::Title);
    m_sortCombo->addItem("Author",    (int)SortField::Author);
    m_sortCombo->addItem("Year",      (int)SortField::Year);
    m_sortCombo->addItem("Format",    (int)SortField::Format);
    m_sortCombo->addItem("Date Added",(int)SortField::DateAdded);
    m_sortCombo->addItem("Rating",    (int)SortField::Rating);
    m_sortCombo->addItem("File Size", (int)SortField::FileSize);
    connect(m_sortCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::sortChanged);
    tb->addWidget(m_sortCombo);

    m_sortAscAct = new QAction("Asc", this);
    m_sortAscAct->setCheckable(true);
    m_sortAscAct->setChecked(true);
    m_sortAscAct->setToolTip("Toggle sort direction");
    connect(m_sortAscAct, &QAction::triggered, this, [this](bool checked){
        m_sortOrder = checked ? SortOrder::Asc : SortOrder::Desc;
        m_sortAscAct->setText(checked ? "Asc" : "Desc");
        sortChanged(m_sortCombo->currentIndex());
    });
    tb->addAction(m_sortAscAct);
    tb->addSeparator();

    // Search
    tb->addWidget(new QLabel("  Search: "));
    m_searchBox = new QLineEdit;
    m_searchBox->setPlaceholderText("Search title, author, ISBN, tags...");
    m_searchBox->setMinimumWidth(220);
    m_searchBox->setClearButtonEnabled(true);
    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::searchChanged);
    tb->addWidget(m_searchBox);

    auto* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);

    QAction* settAct = new QAction("Settings", this);
    connect(settAct, &QAction::triggered, this, &MainWindow::openSettings);
    tb->addAction(settAct);
}

// ── Sidebar ──────────────────────────────────────────────────
void MainWindow::setupSidebar()
{
    auto* dock = new QDockWidget("Browse", this);
    dock->setObjectName("sidebarDock");
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setMinimumWidth(180);
    dock->setMaximumWidth(220);

    auto* sideWidget = new QWidget;
    auto* sideLayout = new QVBoxLayout(sideWidget);
    sideLayout->setContentsMargins(4, 8, 4, 8);
    sideLayout->setSpacing(4);

    auto makeHeader = [&](const QString& title) {
        auto* lbl = new QLabel(title);
        lbl->setObjectName("sideHeader");
        sideLayout->addWidget(lbl);
    };

    auto makeBtn = [&](const QString& text, std::function<void()> fn) {
        auto* btn = new QPushButton(text);
        btn->setObjectName("navBtn");
        btn->setFlat(true);
        connect(btn, &QPushButton::clicked, fn);
        sideLayout->addWidget(btn);
    };

    makeHeader("LIBRARY");
    makeBtn("All Books", [this]() {
        m_filterModel->clearFilters();
        m_searchBox->clear();
        m_favAction->setChecked(false);
    });
    makeBtn("Favourites", [this]() {
        m_filterModel->clearFilters();
        m_filterModel->setFilterFavourites(true);
        m_favAction->setChecked(true);
    });
    makeBtn("Recently Added", [this]() {
        m_filterModel->clearFilters();
        m_filterModel->sortBy(SortField::DateAdded, SortOrder::Desc);
    });
    makeBtn("Most Opened", [this]() {
        m_filterModel->clearFilters();
        m_filterModel->sortBy(SortField::OpenCount, SortOrder::Desc);
    });

    sideLayout->addSpacing(8);
    makeHeader("SMART VIEWS");
    makeBtn("No Cover Art", [this]() {
        m_filterModel->clearFilters();
        m_filterModel->setFilterNoCover(true);
    });
    makeBtn("Missing Metadata", [this]() {
        m_filterModel->clearFilters();
        m_filterModel->setFilterNoMeta(true);
    });

    sideLayout->addSpacing(8);
    makeHeader("FORMAT");
    for (const char* fmt : {"PDF","EPUB","MOBI","AZW","CBZ","DjVu","TXT"}) {
        QString f = fmt;
        makeBtn(f, [this, f]() {
            m_filterModel->setFilterFormat(f);
            m_formatCombo->setCurrentText(f);
        });
    }

    sideLayout->addStretch();

    auto* statsLbl = new QLabel;
    statsLbl->setObjectName("statsLabel");
    statsLbl->setAlignment(Qt::AlignCenter);
    statsLbl->setWordWrap(true);
    connect(m_model, &QAbstractItemModel::modelReset, this, [this, statsLbl]() {
        statsLbl->setText(QString("%1 books").arg(m_model->rowCount()));
    });
    sideLayout->addWidget(statsLbl);

    dock->setWidget(sideWidget);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

// ── Central views ────────────────────────────────────────────
void MainWindow::setupViews()
{
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    // Grid view
    m_gridView = new QListView;
    m_gridView->setObjectName("gridView");
    m_gridView->setModel(m_filterModel);
    m_gridView->setViewMode(QListView::IconMode);
    m_gridView->setResizeMode(QListView::Adjust);
    m_gridView->setMovement(QListView::Static);
    m_gridView->setSpacing(4);
    m_gridView->setUniformItemSizes(true);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setMouseTracking(true);

    m_gridDelegate = new BookDelegate(m_gridView);
    m_gridDelegate->setGridMode(true);
    m_gridView->setItemDelegate(m_gridDelegate);

    connect(m_gridView, &QListView::doubleClicked,
            this, [this](const QModelIndex& idx) { openBookDetail(idx); });

    // List view
    m_listView = new QListView;
    m_listView->setObjectName("listView");
    m_listView->setModel(m_filterModel);
    m_listView->setViewMode(QListView::ListMode);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setMouseTracking(true);

    m_listDelegate = new BookDelegate(m_listView);
    m_listDelegate->setGridMode(false);
    m_listView->setItemDelegate(m_listDelegate);

    connect(m_listView, &QListView::doubleClicked,
            this, [this](const QModelIndex& idx) { openBookDetail(idx); });

    m_stack->addWidget(m_gridView);
    m_stack->addWidget(m_listView);
    m_stack->setCurrentWidget(m_gridView);

    // Context menu for both views
    for (QListView* v : {m_gridView, m_listView}) {
        v->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(v, &QListView::customContextMenuRequested,
                this, [this](const QPoint& pos) {
            QListView* view = currentView();
            QModelIndex idx = view->indexAt(pos);
            QMenu menu;
            menu.setStyleSheet(styleSheet());
            if (idx.isValid()) {
                QAction* openAct = menu.addAction("Open");
                connect(openAct, &QAction::triggered, this, [this]() { openBookDetail(); });
                QAction* editAct = menu.addAction("Edit Details...");
                connect(editAct, &QAction::triggered, this, [this]() { openBookDetail(); });
                menu.addSeparator();
                QAction* fetchAct = menu.addAction("Fetch Metadata");
                connect(fetchAct, &QAction::triggered, this, [this, idx]() {
                    QModelIndex src = m_filterModel->mapToSource(idx);
                    const Book& b = m_model->bookAt(src.row());
                    scheduleMetadataFetch(b);
                });
                menu.addSeparator();
                QAction* removeAct = menu.addAction("Remove from Library");
                connect(removeAct, &QAction::triggered, this, &MainWindow::removeSelectedBook);
            }
            if (!menu.isEmpty())
                menu.exec(view->viewport()->mapToGlobal(pos));
        });
    }
}

// ── Status bar ───────────────────────────────────────────────
void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel, 1);

    m_scanLabel = new QLabel;
    statusBar()->addWidget(m_scanLabel);

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setFixedWidth(200);
    m_progressBar->setFixedHeight(14);
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);
}

// ── Slots ─────────────────────────────────────────────────────
void MainWindow::onScanStart()
{
    if (m_watchFolders.isEmpty()) { openSettings(); return; }
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Scanning library folders...");
    m_scanner->startScan(m_watchFolders, 0); // 0 = auto-detect CPU count
}

void MainWindow::onScanProgress(int done, int total, const QString& file)
{
    if (total > 0) m_progressBar->setValue(done * 100 / total);
    m_scanLabel->setText(file.left(50));
}

void MainWindow::onBookFound(Book book)
{
    m_model->addBook(book);
    QSettings s("Eagle Software", "Eagle Library");
    if (s.value("options/autoMeta", true).toBool())
        scheduleMetadataFetch(book);
}

void MainWindow::onScanFinished(int added, int skipped)
{
    m_progressBar->setVisible(false);
    m_scanLabel->clear();
    m_statusLabel->setText(
        QString("Scan complete -- %1 new, %2 already indexed").arg(added).arg(skipped));
    updateStatusCount();
}

void MainWindow::onMetadataReady(qint64 id, Book updated)
{
    m_model->updateMetadata(id, updated);
    Database::instance().updateBook(updated);
}

void MainWindow::onCoverUrlReady(qint64 id, const QString& url)
{
    QSettings s("Eagle Software", "Eagle Library");
    if (s.value("options/autoCover", true).toBool())
        m_coverDownloader->enqueue(id, url);
}

void MainWindow::onCoverReady(qint64 id, const QString& path)
{
    m_model->updateCover(id, path);
    Database::instance().updateCoverPath(id, path);
}

void MainWindow::openSettings()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        m_watchFolders = dlg.watchedFolders();
        if (!m_watchFolders.isEmpty())
            QTimer::singleShot(200, this, &MainWindow::onScanStart);
    }
}

void MainWindow::openBookDetail(const QModelIndex& index)
{
    QModelIndex srcIdx = m_filterModel->mapToSource(index);
    if (!srcIdx.isValid()) return;
    const Book& b = m_model->bookAt(srcIdx.row());
    BookDetailDialog dlg(b, this);
    int result = dlg.exec();
    if (result == QDialog::Accepted) {
        Book edited = dlg.editedBook();
        m_model->updateBook(edited);
        Database::instance().updateBook(edited);
    } else if (result == 2) {
        scheduleMetadataFetch(b);
    }
}

void MainWindow::openBookDetail()
{
    QListView* v = currentView();
    QModelIndexList sel = v->selectionModel()->selectedIndexes();
    if (!sel.isEmpty()) openBookDetail(sel.first());
}

void MainWindow::setGridView()
{
    m_isGridView = true;
    m_stack->setCurrentWidget(m_gridView);
    m_gridAction->setChecked(true);
    m_listAction->setChecked(false);
}

void MainWindow::setListView()
{
    m_isGridView = false;
    m_stack->setCurrentWidget(m_listView);
    m_listAction->setChecked(true);
    m_gridAction->setChecked(false);
}


void MainWindow::searchChanged(const QString& text)   { m_filterModel->setFilterText(text); }
void MainWindow::filterByFormat(const QString& fmt)   { m_filterModel->setFilterFormat(fmt == "All" ? QString() : fmt); }
void MainWindow::showFavourites(bool on)              { m_filterModel->setFilterFavourites(on); }
void MainWindow::showNoCoverFilter(bool on)           { m_filterModel->setFilterNoCover(on); }
void MainWindow::showNoMetaFilter(bool on)            { m_filterModel->setFilterNoMeta(on); }

void MainWindow::sortChanged(int idx)
{
    SortField sf = (SortField)m_sortCombo->itemData(idx).toInt();
    m_filterModel->sortBy(sf, m_sortOrder);
    // Also reload model from DB with same sort for consistency
    m_model->reload(m_model->currentFilter(), sf, m_sortOrder);
}

// ── Smart Rename ─────────────────────────────────────────────
void MainWindow::onSmartRenameAll()
{
    QVector<Book> books = Database::instance().allBooks();
    if (books.isEmpty()) {
        m_statusLabel->setText("No books in library to rename.");
        return;
    }

    int reply = QMessageBox::question(this, "Smart Rename",
        QString("Analyse %1 books and auto-rename those whose title\n"
                "matches their filename (i.e. no real metadata yet)?\n\n"
                "Books with good metadata are NOT touched.\n"
                "This reads filename patterns like:\n"
                "  Author - Title (Year)\n"
                "  Title - Author\n"
                "  Title (Year)").arg(books.size()),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (reply != QMessageBox::Yes) return;

    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Smart renaming...");

    // Run renamer on background thread
    if (m_renameThread && m_renameThread->isRunning()) return;
    m_renameThread = new QThread(this);
    m_renamer->moveToThread(m_renameThread);
    connect(m_renameThread, &QThread::started, this, [this, books]() {
        m_renamer->renameAll(books);
    });
    connect(m_renamer, &SmartRenamer::progress, this, [this](int done, int total) {
        if (total > 0) m_progressBar->setValue(done * 100 / total);
    }, Qt::QueuedConnection);
    connect(m_renameThread, &QThread::finished, m_renameThread, &QObject::deleteLater);
    m_renameThread->start(QThread::LowPriority);
}

void MainWindow::onRenameResult(RenameResult r)
{
    // Apply to DB
    Database::instance().batchUpdateTitle(r.bookId, r.newTitle, r.newAuthor, {}, r.newYear);

    // Update model in place
    const Book* b = m_model->bookById(r.bookId);
    if (b) {
        Book updated = *b;
        updated.title  = r.newTitle;
        updated.author = r.newAuthor.isEmpty() ? updated.author : r.newAuthor;
        if (r.newYear > 0) updated.year = r.newYear;
        m_model->updateBook(updated);
    }
}

void MainWindow::onRenameFinished(int changed)
{
    m_progressBar->setVisible(false);
    if (m_renameThread) {
        m_renameThread->quit();
        m_renameThread = nullptr;
    }
    // Move renamer back to main thread
    m_renamer->moveToThread(QCoreApplication::instance()->thread());
    m_statusLabel->setText(QString("Smart rename complete — %1 books updated.").arg(changed));
    if (changed > 0) m_model->loadAll(); // full reload to reflect all changes
}

void MainWindow::cleanMissingFiles()
{
    int removed = Database::instance().removeMissingFiles();
    m_model->loadAll();
    m_statusLabel->setText(QString("Cleaned %1 missing file records.").arg(removed));
    updateStatusCount();
}

void MainWindow::fetchAllMetadata()
{
    QVector<Book> books = Database::instance().allBooks();
    int count = 0;
    for (const auto& b : books) {
        if (b.title.isEmpty() || b.author.isEmpty()) {
            scheduleMetadataFetch(b);
            ++count;
        }
    }
    m_statusLabel->setText(QString("Queued metadata fetch for %1 books...").arg(count));
}

void MainWindow::scheduleMetadataFetch(const Book& book)
{
    FetchRequest req;
    req.bookId = book.id;
    req.title  = book.title;
    req.author = book.author;
    req.isbn   = book.isbn;
    m_metaFetcher->enqueue(req);
}

void MainWindow::removeSelectedBook()
{
    QListView* v = currentView();
    QModelIndexList sel = v->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;
    int res = QMessageBox::question(this, "Remove Books",
        QString("Remove %1 book(s) from library?\n(Files will NOT be deleted.)").arg(sel.size()),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (res != QMessageBox::Yes) return;
    for (const auto& proxyIdx : sel) {
        QModelIndex srcIdx = m_filterModel->mapToSource(proxyIdx);
        qint64 id = m_model->bookAt(srcIdx.row()).id;
        Database::instance().removeBook(id);
        m_model->removeBook(id);
    }
    updateStatusCount();
}

void MainWindow::refreshLibrary()
{
    m_model->loadAll();
    updateStatusCount();
}

void MainWindow::showAbout()
{
    QMessageBox box(this);
    box.setWindowTitle("About Eagle Library");
    box.setText(
        "<h2 style='color:#c8aa50;'>Eagle Library</h2>"
        "<p style='color:#aaa;'>Version 1.0.0</p>"
        "<p>Copyright &copy; 2024 Eagle Software. All rights reserved.</p>"
        "<p>Professional eBook library manager. Manage thousands of books "
        "across any folder structure, fetch metadata from the internet, "
        "auto-download cover art, and build your personal reading collection.</p>"
        "<p><a href='https://eaglesoftware.biz/' style='color:#4a90d0;'>"
        "https://eaglesoftware.biz/</a></p>");
    box.setStyleSheet(styleSheet());
    box.exec();
}

void MainWindow::updateStatusCount()
{
    int total = m_model->rowCount();
    int shown = m_filterModel->rowCount();
    if (total == shown)
        m_statusLabel->setText(QString("%1 books in library").arg(total));
    else
        m_statusLabel->setText(QString("Showing %1 of %2 books").arg(shown).arg(total));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    m_scanner->cancel();
    saveSettings();
    event->accept();
}

void MainWindow::loadSettings()
{
    QSettings s("Eagle Software", "Eagle Library");
    m_watchFolders = s.value("library/folders").toStringList();
    m_isGridView   = s.value("view/isGrid", true).toBool();
    if (m_isGridView) setGridView(); else setListView();
    if (s.contains("window/geometry")) restoreGeometry(s.value("window/geometry").toByteArray());
    if (s.contains("window/state"))    restoreState(s.value("window/state").toByteArray());
}

void MainWindow::saveSettings()
{
    QSettings s("Eagle Software", "Eagle Library");
    s.setValue("view/isGrid",     m_isGridView);
    s.setValue("window/geometry", saveGeometry());
    s.setValue("window/state",    saveState());
}

QListView* MainWindow::currentView() const { return m_isGridView ? m_gridView : m_listView; }

void MainWindow::applyStyles()
{
    qApp->setStyle("Fusion");
    setStyleSheet(R"(
QMainWindow, QWidget {
    background-color: #0c1530;
    color: #d8d0b8;
    font-family: "Segoe UI", Arial, sans-serif;
    font-size: 12px;
}
QMenuBar {
    background: #080f22;
    color: #b0a880;
    border-bottom: 1px solid #1e2f60;
    padding: 2px 4px;
}
QMenuBar::item:selected { background: #1a2f70; color: #e8d890; border-radius: 4px; }
QMenu { background: #0f1e48; color: #d8d0b8; border: 1px solid #2a3a70; }
QMenu::item:selected { background: #2040a0; color: #fff; }
QMenu::separator { background: #2a3a70; height: 1px; margin: 2px 0; }
QToolBar#mainToolBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #111e48, stop:1 #0c1535);
    border-bottom: 1px solid #1e2f60;
    padding: 3px 6px;
    spacing: 4px;
}
QToolBar QToolButton {
    color: #b0a870; background: transparent;
    border: 1px solid transparent; border-radius: 6px;
    padding: 4px 10px; font-size: 12px;
}
QToolBar QToolButton:hover { background: #1a3070; border-color: #2a4090; color: #e8d890; }
QToolBar QToolButton:checked { background: #2040a0; border-color: #3050c0; color: #fff; }
QToolBar::separator { background: #1e2f60; width: 1px; margin: 4px 4px; }
QToolBar QLabel { color: #8898c0; padding: 0 2px; }
QLineEdit {
    background: #131e48; border: 1px solid #2e3f70;
    border-radius: 6px; color: #ddd8c8; padding: 5px 10px;
}
QLineEdit:focus { border-color: #b49a46; }
QComboBox {
    background: #131e48; border: 1px solid #2e3f70;
    border-radius: 6px; color: #ddd8c8; padding: 4px 8px;
}
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView {
    background: #0f1e48; border: 1px solid #2a3a70;
    color: #ddd8c8; selection-background-color: #2040a0;
}
QDockWidget { color: #8898c0; font-weight: bold; font-size: 11px; }
QDockWidget::title { background: #0a1228; border-bottom: 1px solid #1e2f60; padding: 4px 8px; }
QDockWidget > QWidget { background: #0a1228; }
QPushButton#navBtn {
    background: transparent; color: #8898c0;
    border: none; border-radius: 5px;
    text-align: left; padding: 6px 12px; font-size: 12px;
}
QPushButton#navBtn:hover { background: #1a2a60; color: #c8aa50; }
QLabel#sideHeader { color: #5060a0; font-size: 9px; font-weight: bold; padding: 8px 12px 2px 12px; }
QLabel#statsLabel { color: #5060a0; font-size: 10px; padding: 8px; border-top: 1px solid #1a2a50; }
QListView { background: #0e1838; border: none; outline: none; }
QListView::item:selected { background: transparent; outline: none; }
QScrollBar:vertical { background: #0a1225; width: 8px; border-radius: 4px; }
QScrollBar::handle:vertical { background: #2a3a70; border-radius: 4px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #3a50a0; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: #0a1225; height: 8px; border-radius: 4px; }
QScrollBar::handle:horizontal { background: #2a3a70; border-radius: 4px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QStatusBar { background: #080f22; color: #6878a0; border-top: 1px solid #1a2a50; font-size: 11px; }
QProgressBar {
    background: #1a2548; border: 1px solid #2a3a70;
    border-radius: 4px; color: transparent; height: 14px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #2040a0, stop:1 #c8a030);
    border-radius: 4px;
}
QMessageBox { background: #0f1a38; color: #d8d0b8; }
QMessageBox QLabel { color: #d8d0b8; }
QPushButton {
    background-color: #1f3070; color: #c8aa50;
    border: 1px solid #2e4090; border-radius: 6px; padding: 6px 16px;
}
QPushButton:hover { background-color: #283d8a; }
QPushButton:pressed { background-color: #162060; }
    )");
}
