#pragma once
// ============================================================
//  Eagle Library — BookDetailDialog.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "Book.h"
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTabWidget>

class BookDetailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BookDetailDialog(const Book& book, QWidget* parent = nullptr);
    Book editedBook() const;

private:
    Book         m_book;
    QLabel*      m_coverLabel;
    QLineEdit*   m_titleEdit;
    QLineEdit*   m_authorEdit;
    QLineEdit*   m_publisherEdit;
    QLineEdit*   m_isbnEdit;
    QSpinBox*    m_yearSpin;
    QSpinBox*    m_pagesSpin;
    QDoubleSpinBox* m_ratingSpin;
    QLineEdit*   m_langEdit;
    QLineEdit*   m_tagsEdit;
    QLineEdit*   m_seriesEdit;
    QSpinBox*    m_seriesIndexSpin;
    QLineEdit*   m_editionEdit;
    QTextEdit*   m_descEdit;
    QTextEdit*   m_notesEdit;
    QPushButton* m_openBtn;
    QPushButton* m_fetchBtn;
    QPushButton* m_googleBtn;
    QPushButton* m_goodreadsBtn;

    void setupUi();
    void applyStyles();
    void loadCover(const QString& path);
};
