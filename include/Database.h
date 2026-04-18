#pragma once
// ============================================================
//  Eagle Library -- Database.h
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include "Book.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QString>
#include <QStringList>

// ── Sort options ──────────────────────────────────────────────
enum class SortField  { Title, Author, Year, DateAdded, Rating, FileSize, Format, OpenCount };
enum class SortOrder  { Asc, Desc };

// ── Filter options ────────────────────────────────────────────
struct BookFilter {
    QString  text;          // full-text search
    QString  format;        // e.g. "PDF"
    QString  author;
    QString  tag;
    QString  language;
    int      yearFrom  = 0;
    int      yearTo    = 0;
    bool     favOnly   = false;
    bool     noCover   = false;
    bool     noMeta    = false;  // title == filename
};

class Database
{
public:
    static Database& instance();

    bool     open(const QString& path);
    void     close();
    bool     isOpen() const;

    // ── CRUD ──────────────────────────────────────────────────
    qint64   insertBook(const Book& book);
    bool     updateBook(const Book& book);
    bool     removeBook(qint64 id);
    bool     bookExists(const QString& filePath) const;
    bool     bookExistsByHash(const QString& hash) const;

    // ── Rich queries with filter + sort + pagination ──────────
    QVector<Book> query(const BookFilter& filter = {},
                        SortField sort  = SortField::Title,
                        SortOrder order = SortOrder::Asc,
                        int limit  = 0,
                        int offset = 0) const;

    QVector<Book> allBooks(SortField sort  = SortField::Title,
                           SortOrder order = SortOrder::Asc) const;

    QVector<Book> search(const QString& text,
                         SortField sort  = SortField::Title,
                         SortOrder order = SortOrder::Asc) const;

    QVector<Book> recentlyAdded(int limit = 30)  const;
    QVector<Book> recentlyOpened(int limit = 30) const;
    QVector<Book> favourites()                   const;
    QVector<Book> noMetadata(int limit = 200)    const;
    Book          bookById(qint64 id)            const;

    // ── Stats ─────────────────────────────────────────────────
    int         totalBooks()    const;
    int         totalByFormat(const QString& fmt) const;
    QStringList allAuthors()    const;
    QStringList allTags()       const;
    QStringList allFormats()    const;
    QStringList allLanguages()  const;
    QStringList allYears()      const;

    // ── Mutations ─────────────────────────────────────────────
    void markOpened(qint64 id);
    void setFavourite(qint64 id, bool fav);
    void updateCoverPath(qint64 id, const QString& path);
    void batchUpdateTitle(qint64 id, const QString& title,
                          const QString& author = {},
                          const QString& publisher = {},
                          int year = 0);

    // ── Maintenance ───────────────────────────────────────────
    void vacuum();
    void reindex();
    int  removeMissingFiles();   // removes records where file no longer exists

private:
    Database() = default;
    bool   createTables();
    bool   createIndexes();
    Book   bookFromQuery(const QSqlQuery& q) const;
    QVector<Book> booksFromQuery(QSqlQuery& q) const;
    QString buildWhereClause(const BookFilter& f, QSqlQuery& q) const;
    QString sortClause(SortField s, SortOrder o) const;
};
