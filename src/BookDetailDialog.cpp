// ============================================================
//  Eagle Library — BookDetailDialog.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "BookDetailDialog.h"
#include "AppConfig.h"
#include "Database.h"
#include "PluginManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QPixmap>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QSizePolicy>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QMouseEvent>

namespace {
// Lightweight event filter that fires a QAction when the watched widget is clicked.
class CardClickFilter : public QObject {
    QAction* m_action;
public:
    CardClickFilter(QAction* action, QObject* parent)
        : QObject(parent), m_action(action) {}
    bool eventFilter(QObject*, QEvent* e) override {
        if (e->type() == QEvent::MouseButtonRelease && m_action)
            m_action->trigger();
        return false;
    }
};
} // namespace
#include <QFile>
#include <QListWidget>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace {

#ifdef Q_OS_WIN
QString shellOpenErrorString(INT_PTR code)
{
    switch (code) {
    case 0: return "The operating system ran out of memory or resources.";
    case ERROR_FILE_NOT_FOUND: return "The file was not found.";
    case ERROR_PATH_NOT_FOUND: return "The folder path was not found.";
    case ERROR_BAD_FORMAT: return "The file format is invalid for opening.";
    case SE_ERR_ACCESSDENIED: return "Access denied by Windows or the associated application.";
    case SE_ERR_ASSOCINCOMPLETE: return "The file association is incomplete.";
    case SE_ERR_DDEBUSY: return "The target application is busy.";
    case SE_ERR_DDEFAIL: return "The DDE transaction failed.";
    case SE_ERR_DDETIMEOUT: return "The target application timed out.";
    case SE_ERR_DLLNOTFOUND: return "A required DLL was not found.";
    case SE_ERR_NOASSOC: return "No application is associated with this file type.";
    case SE_ERR_OOM: return "Not enough memory to open the file.";
    case SE_ERR_SHARE: return "A sharing violation blocked the file.";
    default: return QString("Windows shell error %1.").arg(code);
    }
}
#endif

} // namespace

BookDetailDialog::BookDetailDialog(const Book& book, QWidget* parent)
    : QDialog(parent), m_book(book)
{
    setWindowTitle(QString("Book Details - %1").arg(book.displayTitle()));
    setMinimumSize(700, 520);
    setModal(true);
    setupUi();
    applyStyles();
}

void BookDetailDialog::setupUi()
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(16, 16, 16, 16);
    outerLayout->setSpacing(12);

    auto* mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(16);

    // ── LEFT: cover panel ─────────────────────────────────────
    auto* leftPanel = new QWidget;
    leftPanel->setFixedWidth(180);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0,0,0,0);
    leftLayout->setSpacing(8);

    m_coverLabel = new QLabel;
    m_coverLabel->setFixedSize(168, 230);
    m_coverLabel->setAlignment(Qt::AlignCenter);
    m_coverLabel->setObjectName("coverLabel");
    loadCover(m_book.coverPath);
    leftLayout->addWidget(m_coverLabel);

    // File info
    auto* fileInfoGroup = new QGroupBox("File Info");
    auto* fileInfoLayout = new QVBoxLayout(fileInfoGroup);
    auto addInfoRow = [&](const QString& label, const QString& value) {
        auto* lbl = new QLabel(QString("<b>%1</b><br><span>%2</span>")
                               .arg(label).arg(value));
        lbl->setWordWrap(true);
        fileInfoLayout->addWidget(lbl);
    };
    addInfoRow("Format", m_book.format);
    addInfoRow("Size",   QString("%1 KB").arg(m_book.fileSize / 1024));
    QFileInfo fi(m_book.filePath);
    addInfoRow("Folder", fi.absolutePath());
    addInfoRow("Added",  m_book.dateAdded.toString("dd MMM yyyy"));
    if (m_book.openCount > 0)
        addInfoRow("Opened", QString("%1x").arg(m_book.openCount));
    if (!m_book.readingStatus.trimmed().isEmpty())
        addInfoRow("Reading", m_book.readingStatus);
    if (!m_book.loanedTo.trimmed().isEmpty())
        addInfoRow("Loaned To", m_book.loanedTo);
    leftLayout->addWidget(fileInfoGroup);

    auto* locationsGroup = new QGroupBox("File Locations");
    auto* locationsLayout = new QVBoxLayout(locationsGroup);
    m_locationsList = new QListWidget(locationsGroup);
    m_locationsList->setSelectionMode(QAbstractItemView::SingleSelection);
    const QStringList locations = Database::instance().fileLocationsForBook(m_book.id);
    if (locations.isEmpty()) {
        m_locationsList->addItem(QDir::toNativeSeparators(m_book.filePath));
    } else {
        for (const QString& location : locations)
            m_locationsList->addItem(QDir::toNativeSeparators(location));
    }
    locationsLayout->addWidget(m_locationsList);

    auto* openLocationBtn = new QPushButton("Open Selected Location");
    connect(openLocationBtn, &QPushButton::clicked, this, [this]() {
        if (!m_locationsList || !m_locationsList->currentItem())
            return;
        const QString path = QDir::fromNativeSeparators(m_locationsList->currentItem()->text());
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    locationsLayout->addWidget(openLocationBtn);
    leftLayout->addWidget(locationsGroup);

    m_openBtn = new QPushButton("Open Book");
    m_openBtn->setObjectName("primaryBtn");
    connect(m_openBtn, &QPushButton::clicked, this, [this]() {
        const QString filePath = QFileInfo(m_book.filePath).absoluteFilePath();
        const QFileInfo fileInfo(filePath);
        QFile probe(filePath);
        const bool exists = fileInfo.exists();
        const bool isFile = fileInfo.isFile();
        const bool readable = fileInfo.isReadable();
        const bool probeOpen = probe.open(QIODevice::ReadOnly);
        const QString probeError = probe.errorString();
        if (probeOpen)
            probe.close();

        qWarning().noquote()
            << "[OpenBook] Requested path:" << m_book.filePath
            << "\n[OpenBook] Absolute path:" << filePath
            << "\n[OpenBook] Native path:" << QDir::toNativeSeparators(filePath)
            << "\n[OpenBook] Exists:" << exists
            << "IsFile:" << isFile
            << "Readable:" << readable
            << "Suffix:" << fileInfo.suffix()
            << "\n[OpenBook] QFile open probe:" << probeOpen
            << "Error:" << probeError;

        if (!exists) {
            QMessageBox::warning(this, "Open Book",
                                 QString("File does not exist:\n%1").arg(QDir::toNativeSeparators(filePath)));
            return;
        }

#ifdef Q_OS_WIN
        const std::wstring nativePath = QDir::toNativeSeparators(filePath).toStdWString();
        const HINSTANCE result = ShellExecuteW(nullptr, L"open", nativePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        qWarning() << "[OpenBook] ShellExecuteW result:" << reinterpret_cast<INT_PTR>(result);
        if (reinterpret_cast<INT_PTR>(result) > 32)
            return;

        const bool explorerStarted = QProcess::startDetached("explorer.exe", {QDir::toNativeSeparators(filePath)});
        qWarning() << "[OpenBook] explorer.exe fallback started:" << explorerStarted;
        if (explorerStarted)
            return;
#endif

        const QUrl localUrl = QUrl::fromLocalFile(filePath);
        const bool qtOpened = QDesktopServices::openUrl(localUrl);
        qWarning().noquote()
            << "[OpenBook] QDesktopServices URL:" << localUrl.toString()
            << "\n[OpenBook] QDesktopServices opened:" << qtOpened;

        if (!qtOpened) {
#ifdef Q_OS_WIN
            const QString details = shellOpenErrorString(reinterpret_cast<INT_PTR>(result));
#else
            const QString details = QString("No application could open this file.");
#endif
            QMessageBox::warning(this, "Open Book",
                                 QString("Cannot open file:\n%1\n\n%2")
                                     .arg(QDir::toNativeSeparators(filePath), details));
        }
    });
    leftLayout->addWidget(m_openBtn);

    m_fetchBtn = new QPushButton("Fetch Metadata");
    connect(m_fetchBtn, &QPushButton::clicked, this, [this]() {
        setResult(2);
        accept();
    });
    leftLayout->addWidget(m_fetchBtn);

    m_googleBtn = new QPushButton("Search on Google");
    connect(m_googleBtn, &QPushButton::clicked, this, [this]() {
        const QString query = QString("%1 %2").arg(m_book.title, m_book.author).trimmed();
        const QUrl url = QUrl(QString("https://www.google.com/search?q=%1")
                              .arg(QString::fromUtf8(QUrl::toPercentEncoding(query))));
        QDesktopServices::openUrl(url);
    });
    leftLayout->addWidget(m_googleBtn);

    const QList<QAction*> pluginActions = PluginManager::instance().contextActionsForBook(m_book.id);
    if (!pluginActions.isEmpty()) {
        // Section header
        auto* sectionHeader = new QLabel("PLUGIN ACTIONS");
        sectionHeader->setStyleSheet(
            "font-size:9px;font-weight:800;letter-spacing:1.2px;"
            "color:#3a6080;padding:12px 0 5px 2px;background:transparent;");
        leftLayout->addWidget(sectionHeader);

        for (QAction* action : pluginActions) {
            // Card frame — visual container
            auto* card = new QFrame;
            card->setCursor(Qt::PointingHandCursor);
            card->setFixedHeight(56);
            card->setStyleSheet(
                "QFrame{"
                "  background:#0e1d2e;"
                "  border:1px solid #1e3a54;"
                "  border-left:3px solid #286090;"
                "  border-radius:7px;"
                "}"
                "QFrame:hover{"
                "  background:#122234;"
                "  border-color:#3a6888;"
                "  border-left-color:#4a9adc;"
                "}");

            auto* cl = new QHBoxLayout(card);
            cl->setContentsMargins(12, 6, 12, 6);
            cl->setSpacing(8);

            auto* textCol = new QVBoxLayout;
            textCol->setSpacing(2);

            auto* nameLbl = new QLabel(action->text());
            nameLbl->setStyleSheet(
                "font-weight:700;font-size:12px;color:#dcd290;background:transparent;");
            textCol->addWidget(nameLbl);

            if (!action->toolTip().isEmpty()) {
                auto* tipLbl = new QLabel(action->toolTip());
                tipLbl->setStyleSheet(
                    "font-size:10px;color:#3a6880;background:transparent;");
                tipLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                textCol->addWidget(tipLbl);
            }

            auto* arrowLbl = new QLabel("›");
            arrowLbl->setStyleSheet(
                "font-size:20px;color:#284860;background:transparent;font-weight:300;");
            arrowLbl->setFixedWidth(16);

            cl->addLayout(textCol, 1);
            cl->addWidget(arrowLbl);

            // Make the whole card respond to clicks
            card->installEventFilter(new CardClickFilter(action, card));
            leftLayout->addWidget(card);
        }
    }
    leftLayout->addStretch();

    mainLayout->addWidget(leftPanel);

    // ── RIGHT: tabs ───────────────────────────────────────────
    auto* tabs = new QTabWidget;
    tabs->setObjectName("detailTabs");

    // Tab 1: Core metadata
    auto* metaWidget = new QWidget;
    auto* metaForm = new QFormLayout(metaWidget);
    metaForm->setLabelAlignment(Qt::AlignRight);
    metaForm->setSpacing(10);

    m_titleEdit     = new QLineEdit(m_book.title);
    m_authorEdit    = new QLineEdit(m_book.author);
    m_publisherEdit = new QLineEdit(m_book.publisher);
    m_isbnEdit      = new QLineEdit(m_book.isbn);
    m_langEdit      = new QLineEdit(m_book.language);
    m_yearSpin      = new QSpinBox;
    m_yearSpin->setRange(0, 2099);
    m_yearSpin->setValue(m_book.year);
    m_pagesSpin     = new QSpinBox;
    m_pagesSpin->setRange(0, 999999);
    m_pagesSpin->setValue(m_book.pages);
    m_ratingSpin    = new QDoubleSpinBox;
    m_ratingSpin->setRange(0, 5);
    m_ratingSpin->setSingleStep(0.5);
    m_ratingSpin->setValue(m_book.rating);
    m_tagsEdit      = new QLineEdit(m_book.tags.join(", "));
    m_tagsEdit->setPlaceholderText("Comma-separated tags: Physics, Math, Reading...");

    m_seriesEdit        = new QLineEdit(m_book.series);
    m_seriesEdit->setPlaceholderText("e.g. \"The Lord of the Rings\"");
    m_seriesIndexSpin   = new QSpinBox;
    m_seriesIndexSpin->setRange(0, 9999);
    m_seriesIndexSpin->setValue(m_book.seriesIndex);
    m_seriesIndexSpin->setSpecialValueText("—");
    m_editionEdit       = new QLineEdit(m_book.edition);
    m_editionEdit->setPlaceholderText("e.g. \"3rd Edition\"");

    metaForm->addRow("Title:",        m_titleEdit);
    metaForm->addRow("Author:",       m_authorEdit);
    metaForm->addRow("Publisher:",    m_publisherEdit);
    metaForm->addRow("ISBN:",         m_isbnEdit);
    metaForm->addRow("Language:",     m_langEdit);
    metaForm->addRow("Year:",         m_yearSpin);
    metaForm->addRow("Pages:",        m_pagesSpin);
    metaForm->addRow("Rating (0–5):", m_ratingSpin);
    metaForm->addRow("Tags:",         m_tagsEdit);
    metaForm->addRow("Series:",       m_seriesEdit);
    metaForm->addRow("Volume #:",     m_seriesIndexSpin);
    metaForm->addRow("Edition:",      m_editionEdit);
    tabs->addTab(metaWidget, "Metadata");

    // Tab 2: Reading
    auto* readingWidget = new QWidget;
    auto* readingForm = new QFormLayout(readingWidget);
    readingForm->setLabelAlignment(Qt::AlignRight);
    readingForm->setSpacing(10);

    m_readingStatusCombo = new QComboBox(readingWidget);
    m_readingStatusCombo->addItems({"", "Want to Read", "Reading", "Read"});
    {
        const int idx = m_readingStatusCombo->findText(m_book.readingStatus);
        m_readingStatusCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    m_progressSpin = new QSpinBox(readingWidget);
    m_progressSpin->setRange(0, 100);
    m_progressSpin->setSuffix("%");
    m_progressSpin->setValue(m_book.progressPercent);
    m_currentPageSpin = new QSpinBox(readingWidget);
    m_currentPageSpin->setRange(0, 1000000);
    m_currentPageSpin->setValue(m_book.currentPage);
    m_dateStartedEdit = new QDateEdit(readingWidget);
    m_dateStartedEdit->setCalendarPopup(true);
    m_dateStartedEdit->setSpecialValueText("Not set");
    m_dateStartedEdit->setMinimumDate(QDate(1900, 1, 1));
    m_dateStartedEdit->setDate(m_book.dateStarted.isValid() ? m_book.dateStarted.date() : m_dateStartedEdit->minimumDate());
    m_dateFinishedEdit = new QDateEdit(readingWidget);
    m_dateFinishedEdit->setCalendarPopup(true);
    m_dateFinishedEdit->setSpecialValueText("Not set");
    m_dateFinishedEdit->setMinimumDate(QDate(1900, 1, 1));
    m_dateFinishedEdit->setDate(m_book.dateFinished.isValid() ? m_book.dateFinished.date() : m_dateFinishedEdit->minimumDate());
    m_readingMinutesSpin = new QSpinBox(readingWidget);
    m_readingMinutesSpin->setRange(0, 100000);
    m_readingMinutesSpin->setSuffix(" min");
    m_readingMinutesSpin->setValue(m_book.readingMinutes);
    m_loanedToEdit = new QLineEdit(m_book.loanedTo, readingWidget);
    m_loanedToEdit->setPlaceholderText("Borrower name or device");
    m_loanDueDateEdit = new QDateEdit(readingWidget);
    m_loanDueDateEdit->setCalendarPopup(true);
    m_loanDueDateEdit->setSpecialValueText("Not set");
    m_loanDueDateEdit->setMinimumDate(QDate(1900, 1, 1));
    m_loanDueDateEdit->setDate(m_book.loanDueDate.isValid() ? m_book.loanDueDate.date() : m_loanDueDateEdit->minimumDate());

    readingForm->addRow("Status:", m_readingStatusCombo);
    readingForm->addRow("Progress:", m_progressSpin);
    readingForm->addRow("Current Page:", m_currentPageSpin);
    readingForm->addRow("Started:", m_dateStartedEdit);
    readingForm->addRow("Finished:", m_dateFinishedEdit);
    readingForm->addRow("Reading Time:", m_readingMinutesSpin);
    readingForm->addRow("Loaned To:", m_loanedToEdit);
    readingForm->addRow("Loan Due:", m_loanDueDateEdit);
    tabs->addTab(readingWidget, "Reading");

    // Tab 3: Description
    auto* descWidget = new QWidget;
    auto* descLayout = new QVBoxLayout(descWidget);
    m_descEdit = new QTextEdit(m_book.description);
    m_descEdit->setPlaceholderText("Description / Synopsis...");
    descLayout->addWidget(m_descEdit);
    tabs->addTab(descWidget, "Description");

    // Tab 4: Notes
    auto* notesWidget = new QWidget;
    auto* notesLayout = new QVBoxLayout(notesWidget);
    m_notesEdit = new QTextEdit(m_book.notes);
    m_notesEdit->setPlaceholderText("Personal notes...");
    notesLayout->addWidget(m_notesEdit);
    tabs->addTab(notesWidget, "Notes");

    mainLayout->addWidget(tabs, 1);

    // ── Buttons ───────────────────────────────────────────────
    outerLayout->addLayout(mainLayout);
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btnBox->button(QDialogButtonBox::Ok)->setText("Save");
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outerLayout->addWidget(btnBox);
}

void BookDetailDialog::applyStyles()
{
    // Minimal dialog-specific overrides; global theme handles everything else.
    m_coverLabel->setStyleSheet(
        "QLabel#coverLabel { border-radius: 8px; }");
    m_openBtn->setStyleSheet(
        "QPushButton#primaryBtn { font-weight: bold; }");
}

void BookDetailDialog::loadCover(const QString& path)
{
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        QPixmap pm(path);
        if (!pm.isNull()) {
            m_coverLabel->setPixmap(
                pm.scaled(168, 228, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
    }
    m_coverLabel->setText("No Cover\nAvailable");
    m_coverLabel->setStyleSheet("color: #6070a0; font-size: 11px;");
}

Book BookDetailDialog::editedBook() const
{
    Book b = m_book;
    b.title       = m_titleEdit->text().trimmed();
    b.author      = m_authorEdit->text().trimmed();
    b.publisher   = m_publisherEdit->text().trimmed();
    b.isbn        = m_isbnEdit->text().trimmed();
    b.language    = m_langEdit->text().trimmed();
    b.year        = m_yearSpin->value();
    b.pages       = m_pagesSpin->value();
    b.rating      = m_ratingSpin->value();
    b.description = m_descEdit->toPlainText().trimmed();
    b.notes       = m_notesEdit->toPlainText().trimmed();
    b.series      = m_seriesEdit->text().trimmed();
    b.seriesIndex = m_seriesIndexSpin->value();
    b.edition     = m_editionEdit->text().trimmed();
    b.readingStatus = m_readingStatusCombo ? m_readingStatusCombo->currentText().trimmed() : QString();
    b.progressPercent = m_progressSpin ? m_progressSpin->value() : 0;
    b.currentPage = m_currentPageSpin ? m_currentPageSpin->value() : 0;
    b.dateStarted = (m_dateStartedEdit && m_dateStartedEdit->date() != m_dateStartedEdit->minimumDate())
        ? QDateTime(m_dateStartedEdit->date(), QTime(0, 0))
        : QDateTime();
    b.dateFinished = (m_dateFinishedEdit && m_dateFinishedEdit->date() != m_dateFinishedEdit->minimumDate())
        ? QDateTime(m_dateFinishedEdit->date(), QTime(0, 0))
        : QDateTime();
    b.readingMinutes = m_readingMinutesSpin ? m_readingMinutesSpin->value() : 0;
    b.loanedTo = m_loanedToEdit ? m_loanedToEdit->text().trimmed() : QString();
    b.loanDueDate = (m_loanDueDateEdit && m_loanDueDateEdit->date() != m_loanDueDateEdit->minimumDate())
        ? QDateTime(m_loanDueDateEdit->date(), QTime(0, 0))
        : QDateTime();
    QString rawTags = m_tagsEdit->text();
    b.tags.clear();
    for (const auto& t : rawTags.split(",", Qt::SkipEmptyParts))
        b.tags << t.trimmed();
    return b;
}
