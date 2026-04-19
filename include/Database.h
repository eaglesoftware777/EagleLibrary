#pragma once
// ============================================================
//  Eagle Library v2.0 — Database.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "Book.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>

// ── Sort options ──────────────────────────────────────────────
enum class SortField  { Title, Author, Year, DateAdded, Rating, FileSize, Format, OpenCount, Series };
enum class SortOrder  { Asc, Desc };

// ── Filter options ────────────────────────────────────────────
struct BookFilter {
    QString  text;          // full-text / query syntax
    QString  format;
    QString  author;
    QString  tag;
    QString  language;
    QString  collection;    // virtual collection name
    QString  series;
    bool     restrictToPathPrefixes = false;
    QStringList pathPrefixes;
    int      yearFrom  = 0;
    int      yearTo    = 0;
    bool     favOnly   = false;
    bool     noCover   = false;
    bool     noMeta    = false;
};

// ── Custom field definition ───────────────────────────────────
struct CustomFieldDef {
    QString name;           // e.g. "Course Code"
    QString type;           // "text", "integer", "real", "date", "bool"
    QString defaultValue;
};

// ── Saved search ──────────────────────────────────────────────
struct SavedSearch {
    int     id = 0;
    QString name;
    QString query;          // raw query text
    QString filterJson;     // serialized BookFilter
};

// ── Collection ────────────────────────────────────────────────
struct Collection {
    int     id   = 0;
    QString name;
    QString color;   // hex color for badge e.g. "#4f9cf9"
    QString icon;    // optional emoji or icon name
    int     bookCount = 0;
};

class Database
{
public:
    static Database& instance();

    bool     open(const QString& path);
    void     close();
    bool     isOpen() const;

    // ── Core CRUD ─────────────────────────────────────────────
    qint64   insertBook(const Book& book);
    bool     updateBook(const Book& book);
    bool     upsertBook(const Book& book);
    bool     removeBook(qint64 id);
    bool     bookExists(const QString& filePath) const;
    bool     bookExistsByHash(const QString& hash) const;

    // ── Rich queries ──────────────────────────────────────────
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
    QVector<Book> booksInCollection(int collectionId) const;
    Book          bookById(qint64 id)            const;

    // ── Stats ─────────────────────────────────────────────────
    int         totalBooks()    const;
    int         totalByFormat(const QString& fmt) const;
    QStringList allAuthors()    const;
    QStringList allTags()       const;
    QStringList allFormats()    const;
    QStringList allLanguages()  const;
    QStringList allYears()      const;
    QStringList allSeries()     const;

    // ── Mutations ─────────────────────────────────────────────
    void markOpened(qint64 id);
    void setFavourite(qint64 id, bool fav);
    void updateCoverPath(qint64 id, const QString& path);
    void batchUpdateTitle(qint64 id, const QString& title,
                          const QString& author = {},
                          const QString& publisher = {},
                          int year = 0);
    void updateRating(qint64 id, double rating);
    void updateNotes(qint64 id, const QString& notes);
    void updateTags(qint64 id, const QStringList& tags);

    // ── Collections ───────────────────────────────────────────
    QVector<Collection> allCollections() const;
    int     createCollection(const QString& name, const QString& color = "#4f9cf9", const QString& icon = {});
    bool    updateCollection(int id, const QString& name, const QString& color, const QString& icon);
    bool    deleteCollection(int id);
    bool    addBookToCollection(qint64 bookId, int collectionId);
    bool    removeBookFromCollection(qint64 bookId, int collectionId);
    QList<int> collectionsForBook(qint64 bookId) const;

    // ── Custom Fields ─────────────────────────────────────────
    QVector<CustomFieldDef> customFieldDefs() const;
    bool    addCustomField(const CustomFieldDef& def);
    bool    removeCustomField(const QString& name);
    QMap<QString, QVariant> customFieldValues(qint64 bookId) const;
    bool    setCustomFieldValue(qint64 bookId, const QString& fieldName, const QVariant& value);

    // ── Saved Searches ────────────────────────────────────────
    QVector<SavedSearch> savedSearches() const;
    int     saveSearch(const QString& name, const QString& query, const QString& filterJson = {});
    bool    deleteSearch(int id);

    // ── Maintenance ───────────────────────────────────────────
    void optimize();
    void vacuum();
    void reindex();
    int  removeMissingFiles();
    int  removeBooksInFolders(const QStringList& folders);
    int  removeBooksForFolders(const QStringList& folderPaths);
    bool exportLibrary(const QString& filePath, const QStringList& watchedFolders) const;
    int  importLibrary(const QString& filePath, QStringList* watchedFolders = nullptr);

private:
    Database() = default;
    bool   createTables();
    bool   createIndexes();
    Book   bookFromQuery(const QSqlQuery& q) const;
    QVector<Book> booksFromQuery(QSqlQuery& q) const;
    QString buildWhereClause(const BookFilter& f, QSqlQuery& q) const;
    QString sortClause(SortField s, SortOrder o) const;
};
