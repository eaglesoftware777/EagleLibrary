#pragma once
// ============================================================
//  Eagle Library -- BookModel.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "Book.h"
#include "Database.h"
#include <QAbstractListModel>
#include <QVector>
#include <QSortFilterProxyModel>
#include <QTimer>

enum BookRole {
    IdRole          = Qt::UserRole + 1,
    TitleRole,
    AuthorRole,
    FormatRole,
    YearRole,
    RatingRole,
    CoverPathRole,
    HasCoverRole,
    IsFavouriteRole,
    FileSizeRole,
    FilePathRole,
    DescriptionRole,
    TagsRole,
    IsbnRole,
    PagesRole,
    PublisherRole,
    LanguageRole,
    OpenCountRole,
    DateAddedRole,
    CategoryRole,
    ReadingStatusRole,
    ProgressRole,
    LoanedToRole,
};

class BookModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit BookModel(QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ── Load / reload ─────────────────────────────────────────
    void loadAll();
    void reload(const BookFilter& filter = {},
                SortField sort  = SortField::Title,
                SortOrder order = SortOrder::Asc);

    // ── Single-book mutations ─────────────────────────────────
    void addBook(const Book& book);
    void updateBook(const Book& book);
    void removeBook(qint64 id);
    void updateCover(qint64 id, const QString& coverPath);
    void updateMetadata(qint64 id, const Book& updated);

    // ── Accessors ─────────────────────────────────────────────
    const Book& bookAt(int row) const { return m_books[row]; }
    const Book* bookById(qint64 id)  const;
    int         rowForId(qint64 id)  const;

    // ── Current sort/filter state ─────────────────────────────
    SortField   currentSort()  const { return m_sort; }
    SortOrder   currentOrder() const { return m_order; }
    BookFilter  currentFilter() const { return m_filter; }

public slots:
    void setSortField(SortField f);
    void setSortOrder(SortOrder o);
    void toggleSortOrder();

signals:
    void countChanged(int total);

private:
    QVector<Book> m_books;
    BookFilter    m_filter;
    SortField     m_sort  = SortField::Title;
    SortOrder     m_order = SortOrder::Asc;
};

// ── In-memory filter proxy (fast, no DB round-trip) ───────────
class BookFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit BookFilterModel(QObject* parent = nullptr);

    void setFilterText(const QString& text);
    void setFilterFormat(const QString& fmt);
    void setFilterFavourites(bool fav);
    void setFilterNoCover(bool v);
    void setFilterNoMeta(bool v);
    void setFilterYear(int from, int to);
    void setFilterAuthor(const QString& author);
    void setFilterTag(const QString& tag);
    void setFilterCategory(const QString& category);
    void clearFilters();

    // Column sort (click column header)
    void sortBy(SortField f, SortOrder o = SortOrder::Asc);

protected:
    bool filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const override;
    bool lessThan(const QModelIndex& l, const QModelIndex& r)       const override;

private:
    QString   m_filterText;
    QString   m_filterFormat;
    QString   m_filterAuthor;
    QString   m_filterTag;
    QString   m_filterCategory;
    bool      m_filterFav    = false;
    bool      m_filterNoCover= false;
    bool      m_filterNoMeta = false;
    int       m_yearFrom     = 0;
    int       m_yearTo       = 0;
    SortField m_sortField    = SortField::Title;
};
