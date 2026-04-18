// ============================================================
//  Eagle Library — BookDetailDialog.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================
#include "BookDetailDialog.h"
#include "AppConfig.h"
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
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QSizePolicy>

BookDetailDialog::BookDetailDialog(const Book& book, QWidget* parent)
    : QDialog(parent), m_book(book)
{
    setWindowTitle(QString("Book Details — %1").arg(book.displayTitle()));
    setMinimumSize(700, 520);
    setModal(true);
    setupUi();
    applyStyles();
}

void BookDetailDialog::setupUi()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
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
        addInfoRow("Opened", QString("%1×").arg(m_book.openCount));
    leftLayout->addWidget(fileInfoGroup);

    m_openBtn = new QPushButton("📖  Open Book");
    m_openBtn->setObjectName("primaryBtn");
    connect(m_openBtn, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_book.filePath));
    });
    leftLayout->addWidget(m_openBtn);

    m_fetchBtn = new QPushButton("🌐  Fetch Metadata");
    connect(m_fetchBtn, &QPushButton::clicked, this, [this]() {
        // Emit accepted with special flag — handled by MainWindow
        setResult(2);
        accept();
    });
    leftLayout->addWidget(m_fetchBtn);
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

    metaForm->addRow("Title:",     m_titleEdit);
    metaForm->addRow("Author:",    m_authorEdit);
    metaForm->addRow("Publisher:", m_publisherEdit);
    metaForm->addRow("ISBN:",      m_isbnEdit);
    metaForm->addRow("Language:",  m_langEdit);
    metaForm->addRow("Year:",      m_yearSpin);
    metaForm->addRow("Pages:",     m_pagesSpin);
    metaForm->addRow("Rating:",    m_ratingSpin);
    metaForm->addRow("Tags:",      m_tagsEdit);
    tabs->addTab(metaWidget, "Metadata");

    // Tab 2: Description
    auto* descWidget = new QWidget;
    auto* descLayout = new QVBoxLayout(descWidget);
    m_descEdit = new QTextEdit(m_book.description);
    m_descEdit->setPlaceholderText("Description / Synopsis…");
    descLayout->addWidget(m_descEdit);
    tabs->addTab(descWidget, "Description");

    // Tab 3: Notes
    auto* notesWidget = new QWidget;
    auto* notesLayout = new QVBoxLayout(notesWidget);
    m_notesEdit = new QTextEdit(m_book.notes);
    m_notesEdit->setPlaceholderText("Personal notes…");
    notesLayout->addWidget(m_notesEdit);
    tabs->addTab(notesWidget, "Notes");

    mainLayout->addWidget(tabs, 1);

    // ── Buttons ───────────────────────────────────────────────
    auto* outerLayout = new QVBoxLayout;
    outerLayout->setContentsMargins(0,0,0,0);
    outerLayout->addLayout(mainLayout);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btnBox->button(QDialogButtonBox::Ok)->setText("Save");
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outerLayout->addWidget(btnBox);

    setLayout(outerLayout);
}

void BookDetailDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #0f1a38;
            color: #ddd8c8;
        }
        QGroupBox {
            color: #b49a46;
            font-weight: bold;
            border: 1px solid #2a3a6a;
            border-radius: 6px;
            margin-top: 8px;
            padding: 6px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
        }
        QLabel { color: #c8c0a8; font-size: 11px; }
        #coverLabel {
            background-color: #1a2548;
            border: 1px solid #2e3f70;
            border-radius: 8px;
        }
        QLineEdit, QTextEdit, QSpinBox, QDoubleSpinBox {
            background-color: #1a2548;
            border: 1px solid #2e3f70;
            border-radius: 5px;
            color: #ddd8c8;
            padding: 4px 7px;
            font-size: 12px;
        }
        QLineEdit:focus, QTextEdit:focus {
            border-color: #b49a46;
        }
        QTabWidget::pane {
            border: 1px solid #2e3f70;
            border-radius: 6px;
            background: #131e42;
        }
        QTabBar::tab {
            background: #1a2548;
            color: #8898c0;
            padding: 6px 18px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        QTabBar::tab:selected {
            background: #131e42;
            color: #c8aa50;
        }
        QPushButton {
            background-color: #1f3070;
            color: #c8aa50;
            border: 1px solid #2e4090;
            border-radius: 6px;
            padding: 7px 14px;
            font-size: 12px;
        }
        QPushButton:hover { background-color: #283d8a; }
        QPushButton:pressed { background-color: #162060; }
        #primaryBtn {
            background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                stop:0 #2a4aaa, stop:1 #1a3080);
            font-weight: bold;
        }
        QDialogButtonBox QPushButton {
            min-width: 80px;
        }
    )");
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
    QString rawTags = m_tagsEdit->text();
    b.tags.clear();
    for (const auto& t : rawTags.split(",", Qt::SkipEmptyParts))
        b.tags << t.trimmed();
    return b;
}
