// ============================================================
//  Eagle Library -- Database.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "Database.h"
#include "AppConfig.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

namespace {

constexpr auto kBookSelectColumns =
    "id,file_path,file_hash,format,file_size,title,author,publisher,"
    "isbn,language,year,pages,rating,description,tags,subjects,"
    "cover_path,has_cover,date_added,date_modified,last_opened,"
    "open_count,is_favourite,notes,"
    "COALESCE(series,'') AS series,"
    "COALESCE(series_index,0) AS series_index,"
    "COALESCE(edition,'') AS edition";

QString sanitizeTextValue(QString value)
{
    if (value.isEmpty())
        return value;

    value = value.normalized(QString::NormalizationForm_C);
    value.replace(QChar::ReplacementCharacter, QChar(' '));
    value.replace(QChar(0x25A1), QChar(' '));
    value.replace(QChar(0x25A0), QChar(' '));

    QString cleaned;
    cleaned.reserve(value.size());
    for (const QChar ch : value) {
        if (ch.category() == QChar::Other_Control
            && ch != QChar('\n')
            && ch != QChar('\r')
            && ch != QChar('\t')) {
            continue;
        }
        cleaned.append(ch);
    }

    return cleaned.simplified();
}

QStringList sanitizeTextList(const QStringList& values)
{
    QStringList cleaned;
    cleaned.reserve(values.size());
    for (const QString& value : values) {
        const QString sanitized = sanitizeTextValue(value);
        if (!sanitized.isEmpty())
            cleaned << sanitized;
    }
    cleaned.removeDuplicates();
    return cleaned;
}

Book sanitizedBook(Book book)
{
    book.filePath = sanitizeTextValue(book.filePath);
    book.fileHash = sanitizeTextValue(book.fileHash);
    book.format = sanitizeTextValue(book.format);
    book.title = sanitizeTextValue(book.title);
    book.author = sanitizeTextValue(book.author);
    book.publisher = sanitizeTextValue(book.publisher);
    book.isbn = sanitizeTextValue(book.isbn);
    book.language = sanitizeTextValue(book.language);
    book.description = sanitizeTextValue(book.description);
    book.coverPath = sanitizeTextValue(book.coverPath);
    book.notes = sanitizeTextValue(book.notes);
    book.series = sanitizeTextValue(book.series);
    book.edition = sanitizeTextValue(book.edition);
    book.tags = sanitizeTextList(book.tags);
    book.subjects = sanitizeTextList(book.subjects);
    return book;
}

QJsonObject bookToJson(const Book& b)
{
    QJsonObject obj;
    obj["file_path"] = b.filePath;
    obj["file_hash"] = b.fileHash;
    obj["format"] = b.format;
    obj["file_size"] = QString::number(b.fileSize);
    obj["title"] = b.title;
    obj["author"] = b.author;
    obj["publisher"] = b.publisher;
    obj["isbn"] = b.isbn;
    obj["language"] = b.language;
    obj["year"] = b.year;
    obj["pages"] = b.pages;
    obj["rating"] = b.rating;
    obj["description"] = b.description;
    obj["tags"] = QJsonArray::fromStringList(b.tags);
    obj["subjects"] = QJsonArray::fromStringList(b.subjects);
    obj["cover_path"] = b.coverPath;
    obj["has_cover"] = b.hasCover;
    obj["date_added"] = b.dateAdded.toString(Qt::ISODate);
    obj["date_modified"] = b.dateModified.toString(Qt::ISODate);
    obj["last_opened"] = b.lastOpened.toString(Qt::ISODate);
    obj["open_count"] = b.openCount;
    obj["is_favourite"] = b.isFavourite;
    obj["notes"] = b.notes;
    return obj;
}

Book bookFromJsonObject(const QJsonObject& obj)
{
    Book b;
    b.filePath = obj.value("file_path").toString();
    b.fileHash = obj.value("file_hash").toString();
    b.format = obj.value("format").toString();
    b.fileSize = obj.value("file_size").toString().toLongLong();
    b.title = obj.value("title").toString();
    b.author = obj.value("author").toString();
    b.publisher = obj.value("publisher").toString();
    b.isbn = obj.value("isbn").toString();
    b.language = obj.value("language").toString();
    b.year = obj.value("year").toInt();
    b.pages = obj.value("pages").toInt();
    b.rating = obj.value("rating").toDouble();
    b.description = obj.value("description").toString();
    for (const QJsonValue& v : obj.value("tags").toArray()) b.tags << v.toString();
    for (const QJsonValue& v : obj.value("subjects").toArray()) b.subjects << v.toString();
    b.coverPath = obj.value("cover_path").toString();
    if (!AppConfig::isManagedCoverPath(b.coverPath))
        b.coverPath.clear();
    b.hasCover = obj.value("has_cover").toBool();
    if (b.coverPath.isEmpty())
        b.hasCover = false;
    b.dateAdded = QDateTime::fromString(obj.value("date_added").toString(), Qt::ISODate);
    b.dateModified = QDateTime::fromString(obj.value("date_modified").toString(), Qt::ISODate);
    b.lastOpened = QDateTime::fromString(obj.value("last_opened").toString(), Qt::ISODate);
    b.openCount = obj.value("open_count").toInt();
    b.isFavourite = obj.value("is_favourite").toBool();
    b.notes = obj.value("notes").toString();
    return sanitizedBook(b);
}

} // namespace

Database& Database::instance()
{
    static Database db;
    return db;
}

bool Database::open(const QString& path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "eagle_lib");
    db.setDatabaseName(path);
    if (!db.open()) {
        qWarning() << "DB open failed:" << db.lastError().text();
        return false;
    }
    QSqlQuery q(db);
    q.exec("PRAGMA encoding='UTF-8'");
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA synchronous=NORMAL");
    q.exec("PRAGMA foreign_keys=ON");
    q.exec("PRAGMA busy_timeout=5000");
    q.exec("PRAGMA cache_size=20000");
    q.exec("PRAGMA temp_store=MEMORY");
    q.exec("PRAGMA mmap_size=268435456"); // 256 MB mmap
    return createTables() && createIndexes();
}

void Database::close()
{
    QSqlDatabase::database("eagle_lib").close();
    QSqlDatabase::removeDatabase("eagle_lib");
}

bool Database::isOpen() const
{
    return QSqlDatabase::database("eagle_lib").isOpen();
}

bool Database::createTables()
{
    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    QSqlQuery q(db);

    bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path     TEXT NOT NULL UNIQUE,
            file_hash     TEXT,
            format        TEXT,
            file_size     INTEGER DEFAULT 0,
            title         TEXT COLLATE NOCASE,
            author        TEXT COLLATE NOCASE,
            publisher     TEXT COLLATE NOCASE,
            isbn          TEXT,
            language      TEXT,
            year          INTEGER DEFAULT 0,
            pages         INTEGER DEFAULT 0,
            rating        REAL    DEFAULT 0,
            description   TEXT,
            tags          TEXT,
            subjects      TEXT,
            cover_path    TEXT,
            has_cover     INTEGER DEFAULT 0,
            date_added    TEXT,
            date_modified TEXT,
            last_opened   TEXT,
            open_count    INTEGER DEFAULT 0,
            is_favourite  INTEGER DEFAULT 0,
            notes         TEXT,
            series        TEXT COLLATE NOCASE,
            series_index  INTEGER DEFAULT 0,
            edition       TEXT
        )
    )");
    if (!ok) qWarning() << q.lastError().text();

    // Add new columns to existing databases (migration)
    q.exec("ALTER TABLE books ADD COLUMN series       TEXT COLLATE NOCASE");
    q.exec("ALTER TABLE books ADD COLUMN series_index INTEGER DEFAULT 0");
    q.exec("ALTER TABLE books ADD COLUMN edition      TEXT");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS collections (
            id    INTEGER PRIMARY KEY AUTOINCREMENT,
            name  TEXT NOT NULL UNIQUE COLLATE NOCASE,
            color TEXT DEFAULT '#4f9cf9',
            icon  TEXT DEFAULT ''
        )
    )");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS book_collections (
            book_id       INTEGER NOT NULL REFERENCES books(id) ON DELETE CASCADE,
            collection_id INTEGER NOT NULL REFERENCES collections(id) ON DELETE CASCADE,
            PRIMARY KEY (book_id, collection_id)
        )
    )");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS custom_field_defs (
            name          TEXT PRIMARY KEY NOT NULL,
            type          TEXT NOT NULL DEFAULT 'text',
            default_value TEXT DEFAULT ''
        )
    )");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS custom_field_values (
            book_id    INTEGER NOT NULL REFERENCES books(id) ON DELETE CASCADE,
            field_name TEXT NOT NULL REFERENCES custom_field_defs(name) ON DELETE CASCADE,
            value      TEXT,
            PRIMARY KEY (book_id, field_name)
        )
    )");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS saved_searches (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT NOT NULL UNIQUE COLLATE NOCASE,
            query       TEXT NOT NULL,
            filter_json TEXT DEFAULT '{}'
        )
    )");

    return ok;
}

bool Database::createIndexes()
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("CREATE INDEX IF NOT EXISTS idx_title    ON books(title COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_author   ON books(author COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_publisher ON books(publisher COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_hash     ON books(file_hash)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_isbn     ON books(isbn)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_format   ON books(format)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_language ON books(language)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_year     ON books(year)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_rating   ON books(rating)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_fav      ON books(is_favourite)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_cover    ON books(has_cover)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_added    ON books(date_added)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_opened   ON books(last_opened)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_filesize ON books(file_size)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_format_year ON books(format, year)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_fav_opened ON books(is_favourite, last_opened)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_series     ON books(series COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_bc_book    ON book_collections(book_id)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_bc_coll    ON book_collections(collection_id)");
    optimize();
    return true;
}

// ── Book from query ───────────────────────────────────────────
Book Database::bookFromQuery(const QSqlQuery& q) const
{
    Book b;
    b.id           = q.value("id").toLongLong();
    b.filePath     = q.value("file_path").toString();
    b.fileHash     = q.value("file_hash").toString();
    b.format       = q.value("format").toString();
    b.fileSize     = q.value("file_size").toLongLong();
    b.title        = q.value("title").toString();
    b.author       = q.value("author").toString();
    b.publisher    = q.value("publisher").toString();
    b.isbn         = q.value("isbn").toString();
    b.language     = q.value("language").toString();
    b.year         = q.value("year").toInt();
    b.pages        = q.value("pages").toInt();
    b.rating       = q.value("rating").toDouble();
    b.description  = q.value("description").toString();
    b.tags         = q.value("tags").toString().split("|", Qt::SkipEmptyParts);
    b.subjects     = q.value("subjects").toString().split("|", Qt::SkipEmptyParts);
    b.coverPath    = q.value("cover_path").toString();
    b.hasCover     = q.value("has_cover").toInt() != 0;
    b.dateAdded    = QDateTime::fromString(q.value("date_added").toString(), Qt::ISODate);
    b.dateModified = QDateTime::fromString(q.value("date_modified").toString(), Qt::ISODate);
    b.lastOpened   = QDateTime::fromString(q.value("last_opened").toString(), Qt::ISODate);
    b.openCount    = q.value("open_count").toInt();
    b.isFavourite  = q.value("is_favourite").toInt() != 0;
    b.notes        = q.value("notes").toString();
    b.series       = q.value("series").toString();
    b.seriesIndex  = q.value("series_index").toInt();
    b.edition      = q.value("edition").toString();
    return sanitizedBook(b);
}

QVector<Book> Database::booksFromQuery(QSqlQuery& q) const
{
    QVector<Book> books;
    while (q.next()) books.append(bookFromQuery(q));
    return books;
}

// ── Sort clause ───────────────────────────────────────────────
QString Database::sortClause(SortField s, SortOrder o) const
{
    QString col;
    switch (s) {
        case SortField::Title:     col = "title COLLATE NOCASE"; break;
        case SortField::Author:    col = "author COLLATE NOCASE"; break;
        case SortField::Year:      col = "year"; break;
        case SortField::DateAdded: col = "date_added"; break;
        case SortField::Rating:    col = "rating"; break;
        case SortField::FileSize:  col = "file_size"; break;
        case SortField::Format:    col = "format"; break;
        case SortField::OpenCount: col = "open_count"; break;
        case SortField::Series:    col = "series COLLATE NOCASE, series_index"; break;
    }
    return " ORDER BY " + col + (o == SortOrder::Asc ? " ASC" : " DESC");
}

// ── Where clause builder ──────────────────────────────────────
QString Database::buildWhereClause(const BookFilter& f, QSqlQuery& q) const
{
    QStringList clauses;

    if (!f.text.isEmpty()) {
        clauses << "(title LIKE :txt OR author LIKE :txt OR description LIKE :txt "
                   "OR isbn LIKE :txt OR tags LIKE :txt OR publisher LIKE :txt)";
        q.bindValue(":txt", "%" + f.text + "%");
    }
    if (!f.format.isEmpty()) {
        clauses << "format = :fmt";
        q.bindValue(":fmt", f.format);
    }
    if (!f.author.isEmpty()) {
        clauses << "author LIKE :aut";
        q.bindValue(":aut", "%" + f.author + "%");
    }
    if (!f.tag.isEmpty()) {
        clauses << "tags LIKE :tag";
        q.bindValue(":tag", "%" + f.tag + "%");
    }
    if (!f.language.isEmpty()) {
        clauses << "language = :lang";
        q.bindValue(":lang", f.language);
    }
    if (f.restrictToPathPrefixes && f.pathPrefixes.isEmpty()) {
        clauses << "0 = 1";
    } else if (!f.pathPrefixes.isEmpty()) {
        QStringList pathClauses;
        for (int i = 0; i < f.pathPrefixes.size(); ++i) {
            QString prefix = QDir::fromNativeSeparators(f.pathPrefixes.at(i)).trimmed();
            while (prefix.endsWith('/'))
                prefix.chop(1);
            if (prefix.isEmpty())
                continue;
            const QString exactKey = QString(":pathExact%1").arg(i);
            const QString likeKey = QString(":pathLike%1").arg(i);
            pathClauses << QString("(file_path = %1 OR file_path LIKE %2)").arg(exactKey, likeKey);
            q.bindValue(exactKey, prefix);
            q.bindValue(likeKey, prefix + "/%");
        }
        if (!pathClauses.isEmpty())
            clauses << "(" + pathClauses.join(" OR ") + ")";
    }
    if (f.yearFrom > 0) {
        clauses << "year >= :yf";
        q.bindValue(":yf", f.yearFrom);
    }
    if (f.yearTo > 0) {
        clauses << "year <= :yt";
        q.bindValue(":yt", f.yearTo);
    }
    if (f.favOnly)  clauses << "is_favourite = 1";
    if (f.noCover)  clauses << "has_cover = 0";
    if (f.noMeta)   clauses << "(author = '' OR author IS NULL)";
    if (!f.series.isEmpty()) {
        clauses << "series LIKE :ser";
        q.bindValue(":ser", "%" + f.series + "%");
    }
    if (!f.collection.isEmpty()) {
        clauses << "id IN (SELECT book_id FROM book_collections bc "
                   "JOIN collections c ON c.id=bc.collection_id WHERE c.name=:colname)";
        q.bindValue(":colname", f.collection);
    }

    return clauses.isEmpty() ? "" : " WHERE " + clauses.join(" AND ");
}

// ── Rich query ────────────────────────────────────────────────
QVector<Book> Database::query(const BookFilter& filter,
                               SortField sort, SortOrder order,
                               int limit, int offset) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.setForwardOnly(true);
    QString sql = QString("SELECT %1 FROM books").arg(kBookSelectColumns);
    sql += buildWhereClause(filter, q);
    sql += sortClause(sort, order);
    if (limit > 0)  sql += QString(" LIMIT %1").arg(limit);
    if (offset > 0) sql += QString(" OFFSET %1").arg(offset);
    q.prepare(sql);
    // rebind (Qt needs prepare before bind for named params)
    if (!filter.text.isEmpty())     q.bindValue(":txt",  "%" + filter.text + "%");
    if (!filter.format.isEmpty())   q.bindValue(":fmt",  filter.format);
    if (!filter.author.isEmpty())   q.bindValue(":aut",  "%" + filter.author + "%");
    if (!filter.tag.isEmpty())      q.bindValue(":tag",  "%" + filter.tag + "%");
    if (!filter.language.isEmpty()) q.bindValue(":lang", filter.language);
    for (int i = 0; i < filter.pathPrefixes.size(); ++i) {
        QString prefix = QDir::fromNativeSeparators(filter.pathPrefixes.at(i)).trimmed();
        while (prefix.endsWith('/'))
            prefix.chop(1);
        if (prefix.isEmpty())
            continue;
        q.bindValue(QString(":pathExact%1").arg(i), prefix);
        q.bindValue(QString(":pathLike%1").arg(i), prefix + "/%");
    }
    if (filter.yearFrom > 0)           q.bindValue(":yf",      filter.yearFrom);
    if (filter.yearTo > 0)            q.bindValue(":yt",      filter.yearTo);
    if (!filter.series.isEmpty())     q.bindValue(":ser",     "%" + filter.series + "%");
    if (!filter.collection.isEmpty()) q.bindValue(":colname", filter.collection);
    q.exec();
    return booksFromQuery(q);
}

QVector<Book> Database::allBooks(SortField sort, SortOrder order) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.setForwardOnly(true);
    q.exec(QString("SELECT %1 FROM books").arg(kBookSelectColumns) + sortClause(sort, order));
    return booksFromQuery(q);
}

QVector<Book> Database::search(const QString& text, SortField sort, SortOrder order) const
{
    BookFilter f; f.text = text;
    return query(f, sort, order);
}

QVector<Book> Database::favourites() const
{
    BookFilter f; f.favOnly = true;
    return query(f, SortField::Title, SortOrder::Asc);
}

QVector<Book> Database::recentlyAdded(int limit) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.setForwardOnly(true);
    q.prepare(QString("SELECT %1 FROM books ORDER BY date_added DESC LIMIT :lim").arg(kBookSelectColumns));
    q.bindValue(":lim", limit);
    q.exec();
    return booksFromQuery(q);
}

QVector<Book> Database::recentlyOpened(int limit) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.setForwardOnly(true);
    q.prepare(QString("SELECT %1 FROM books WHERE last_opened != '' "
                      "ORDER BY last_opened DESC LIMIT :lim").arg(kBookSelectColumns));
    q.bindValue(":lim", limit);
    q.exec();
    return booksFromQuery(q);
}

QVector<Book> Database::noMetadata(int limit) const
{
    BookFilter f; f.noMeta = true;
    return query(f, SortField::Title, SortOrder::Asc, limit, 0);
}

Book Database::bookById(qint64 id) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.setForwardOnly(true);
    q.prepare(QString("SELECT %1 FROM books WHERE id=:id").arg(kBookSelectColumns));
    q.bindValue(":id", id);
    q.exec();
    if (q.next()) return bookFromQuery(q);
    return {};
}

// ── CRUD ──────────────────────────────────────────────────────
qint64 Database::insertBook(const Book& b)
{
    const Book clean = sanitizedBook(b);
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare(R"(
        INSERT OR IGNORE INTO books
            (file_path,file_hash,format,file_size,title,author,publisher,
             isbn,language,year,pages,rating,description,tags,subjects,
             cover_path,has_cover,date_added,date_modified,last_opened,
             open_count,is_favourite,notes,series,series_index,edition)
        VALUES
            (:fp,:fh,:fmt,:fs,:ti,:au,:pub,
             :isbn,:lang,:yr,:pg,:rt,:desc,:tags,:subj,
             :cp,:hc,:da,:dm,:lo,:oc,:fav,:notes,:ser,:seridx,:ed)
    )");
    q.bindValue(":fp",     clean.filePath);
    q.bindValue(":fh",     clean.fileHash);
    q.bindValue(":fmt",    clean.format);
    q.bindValue(":fs",     clean.fileSize);
    q.bindValue(":ti",     clean.title);
    q.bindValue(":au",     clean.author);
    q.bindValue(":pub",    clean.publisher);
    q.bindValue(":isbn",   clean.isbn);
    q.bindValue(":lang",   clean.language);
    q.bindValue(":yr",     clean.year);
    q.bindValue(":pg",     clean.pages);
    q.bindValue(":rt",     clean.rating);
    q.bindValue(":desc",   clean.description);
    q.bindValue(":tags",   clean.tags.join("|"));
    q.bindValue(":subj",   clean.subjects.join("|"));
    q.bindValue(":cp",     clean.coverPath);
    q.bindValue(":hc",     clean.hasCover ? 1 : 0);
    q.bindValue(":da",     clean.dateAdded.toString(Qt::ISODate));
    q.bindValue(":dm",     clean.dateModified.toString(Qt::ISODate));
    q.bindValue(":lo",     clean.lastOpened.toString(Qt::ISODate));
    q.bindValue(":oc",     clean.openCount);
    q.bindValue(":fav",    clean.isFavourite ? 1 : 0);
    q.bindValue(":notes",  clean.notes);
    q.bindValue(":ser",    clean.series);
    q.bindValue(":seridx", clean.seriesIndex);
    q.bindValue(":ed",     clean.edition);
    if (!q.exec()) { qWarning() << "insertBook:" << q.lastError().text(); return -1; }
    return q.lastInsertId().toLongLong();
}

bool Database::updateBook(const Book& b)
{
    const Book clean = sanitizedBook(b);
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare(R"(
        UPDATE books SET
            title=:ti, author=:au, publisher=:pub, isbn=:isbn,
            language=:lang, year=:yr, pages=:pg, rating=:rt,
            description=:desc, tags=:tags, subjects=:subj,
            cover_path=:cp, has_cover=:hc, date_modified=:dm,
            is_favourite=:fav, notes=:notes,
            series=:ser, series_index=:seridx, edition=:ed
        WHERE id=:id
    )");
    q.bindValue(":ti",   clean.title);
    q.bindValue(":au",   clean.author);
    q.bindValue(":pub",  clean.publisher);
    q.bindValue(":isbn", clean.isbn);
    q.bindValue(":lang", clean.language);
    q.bindValue(":yr",   clean.year);
    q.bindValue(":pg",   clean.pages);
    q.bindValue(":rt",   clean.rating);
    q.bindValue(":desc", clean.description);
    q.bindValue(":tags", clean.tags.join("|"));
    q.bindValue(":subj", clean.subjects.join("|"));
    q.bindValue(":cp",   clean.coverPath);
    q.bindValue(":hc",   clean.hasCover ? 1 : 0);
    q.bindValue(":dm",     QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":fav",    clean.isFavourite ? 1 : 0);
    q.bindValue(":notes",  clean.notes);
    q.bindValue(":ser",    clean.series);
    q.bindValue(":seridx", clean.seriesIndex);
    q.bindValue(":ed",     clean.edition);
    q.bindValue(":id",     clean.id);
    return q.exec();
}

bool Database::upsertBook(const Book& b)
{
    const Book clean = sanitizedBook(b);
    if (clean.filePath.isEmpty())
        return false;

    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare(R"(
        INSERT INTO books
            (file_path,file_hash,format,file_size,title,author,publisher,
             isbn,language,year,pages,rating,description,tags,subjects,
             cover_path,has_cover,date_added,date_modified,last_opened,
             open_count,is_favourite,notes,series,series_index,edition)
        VALUES
            (:fp,:fh,:fmt,:fs,:ti,:au,:pub,
             :isbn,:lang,:yr,:pg,:rt,:desc,:tags,:subj,
             :cp,:hc,:da,:dm,:lo,:oc,:fav,:notes,:ser,:seridx,:ed)
        ON CONFLICT(file_path) DO UPDATE SET
            file_hash=excluded.file_hash,
            format=excluded.format,
            file_size=excluded.file_size,
            title=excluded.title,
            author=excluded.author,
            publisher=excluded.publisher,
            isbn=excluded.isbn,
            language=excluded.language,
            year=excluded.year,
            pages=excluded.pages,
            rating=excluded.rating,
            description=excluded.description,
            tags=excluded.tags,
            subjects=excluded.subjects,
            cover_path=excluded.cover_path,
            has_cover=excluded.has_cover,
            date_added=excluded.date_added,
            date_modified=excluded.date_modified,
            last_opened=excluded.last_opened,
            open_count=excluded.open_count,
            is_favourite=excluded.is_favourite,
            notes=excluded.notes,
            series=excluded.series,
            series_index=excluded.series_index,
            edition=excluded.edition
    )");
    q.bindValue(":fp",     clean.filePath);
    q.bindValue(":fh",     clean.fileHash);
    q.bindValue(":fmt",    clean.format);
    q.bindValue(":fs",     clean.fileSize);
    q.bindValue(":ti",     clean.title);
    q.bindValue(":au",     clean.author);
    q.bindValue(":pub",    clean.publisher);
    q.bindValue(":isbn",   clean.isbn);
    q.bindValue(":lang",   clean.language);
    q.bindValue(":yr",     clean.year);
    q.bindValue(":pg",     clean.pages);
    q.bindValue(":rt",     clean.rating);
    q.bindValue(":desc",   clean.description);
    q.bindValue(":tags",   clean.tags.join("|"));
    q.bindValue(":subj",   clean.subjects.join("|"));
    q.bindValue(":cp",     clean.coverPath);
    q.bindValue(":hc",     clean.hasCover ? 1 : 0);
    q.bindValue(":da",     clean.dateAdded.toString(Qt::ISODate));
    q.bindValue(":dm",     clean.dateModified.toString(Qt::ISODate));
    q.bindValue(":lo",     clean.lastOpened.toString(Qt::ISODate));
    q.bindValue(":oc",     clean.openCount);
    q.bindValue(":fav",    clean.isFavourite ? 1 : 0);
    q.bindValue(":notes",  clean.notes);
    q.bindValue(":ser",    clean.series);
    q.bindValue(":seridx", clean.seriesIndex);
    q.bindValue(":ed",     clean.edition);
    if (!q.exec()) {
        qWarning() << "upsertBook:" << q.lastError().text();
        return false;
    }
    return true;
}

bool Database::removeBook(qint64 id)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("DELETE FROM books WHERE id=:id");
    q.bindValue(":id", id);
    return q.exec();
}

bool Database::bookExists(const QString& filePath) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("SELECT 1 FROM books WHERE file_path=:fp LIMIT 1");
    q.bindValue(":fp", filePath);
    q.exec();
    return q.next();
}

bool Database::bookExistsByHash(const QString& hash) const
{
    if (hash.isEmpty()) return false;
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("SELECT 1 FROM books WHERE file_hash=:h LIMIT 1");
    q.bindValue(":h", hash);
    q.exec();
    return q.next();
}

// ── Stats ─────────────────────────────────────────────────────
int Database::totalBooks() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT COUNT(*) FROM books");
    return q.next() ? q.value(0).toInt() : 0;
}

int Database::totalByFormat(const QString& fmt) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("SELECT COUNT(*) FROM books WHERE format=:f");
    q.bindValue(":f", fmt);
    q.exec();
    return q.next() ? q.value(0).toInt() : 0;
}

QStringList Database::allAuthors() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT DISTINCT author FROM books WHERE author != '' ORDER BY author COLLATE NOCASE");
    QStringList l; while (q.next()) l << q.value(0).toString();
    return l;
}

QStringList Database::allTags() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT tags FROM books WHERE tags != ''");
    QSet<QString> s;
    while (q.next())
        for (const auto& t : q.value(0).toString().split("|", Qt::SkipEmptyParts))
            s.insert(t.trimmed());
    QStringList l(s.begin(), s.end());
    l.sort(Qt::CaseInsensitive);
    return l;
}

QStringList Database::allFormats() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT DISTINCT format FROM books WHERE format != '' ORDER BY format");
    QStringList l; while (q.next()) l << q.value(0).toString();
    return l;
}

QStringList Database::allLanguages() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT DISTINCT language FROM books WHERE language != '' ORDER BY language");
    QStringList l; while (q.next()) l << q.value(0).toString();
    return l;
}

QStringList Database::allYears() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT DISTINCT year FROM books WHERE year > 0 ORDER BY year DESC");
    QStringList l; while (q.next()) l << q.value(0).toString();
    return l;
}

QStringList Database::allSeries() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT DISTINCT series FROM books WHERE series != '' ORDER BY series COLLATE NOCASE");
    QStringList l; while (q.next()) l << q.value(0).toString();
    return l;
}

// ── Mutations ─────────────────────────────────────────────────
void Database::markOpened(qint64 id)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET last_opened=:t, open_count=open_count+1 WHERE id=:id");
    q.bindValue(":t",  QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":id", id);
    q.exec();
}

void Database::setFavourite(qint64 id, bool fav)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET is_favourite=:f WHERE id=:id");
    q.bindValue(":f",  fav ? 1 : 0);
    q.bindValue(":id", id);
    q.exec();
}

void Database::updateCoverPath(qint64 id, const QString& path)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET cover_path=:p, has_cover=1 WHERE id=:id");
    q.bindValue(":p",  path);
    q.bindValue(":id", id);
    q.exec();
}

void Database::batchUpdateTitle(qint64 id, const QString& title,
                                 const QString& author,
                                 const QString& publisher,
                                 int year)
{
    const QString cleanTitle = sanitizeTextValue(title);
    const QString cleanAuthor = sanitizeTextValue(author);
    const QString cleanPublisher = sanitizeTextValue(publisher);
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET title=:ti, author=:au, publisher=:pub, year=:yr, "
              "date_modified=:dm WHERE id=:id");
    q.bindValue(":ti",  cleanTitle);
    q.bindValue(":au",  cleanAuthor);
    q.bindValue(":pub", cleanPublisher);
    q.bindValue(":yr",  year);
    q.bindValue(":dm",  QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":id",  id);
    q.exec();
}

void Database::updateRating(qint64 id, double rating)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET rating=:r, date_modified=:dm WHERE id=:id");
    q.bindValue(":r",  rating);
    q.bindValue(":dm", QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":id", id);
    q.exec();
}

void Database::updateNotes(qint64 id, const QString& notes)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET notes=:n, date_modified=:dm WHERE id=:id");
    q.bindValue(":n",  sanitizeTextValue(notes));
    q.bindValue(":dm", QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":id", id);
    q.exec();
}

void Database::updateTags(qint64 id, const QStringList& tags)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET tags=:t, date_modified=:dm WHERE id=:id");
    q.bindValue(":t",  sanitizeTextList(tags).join("|"));
    q.bindValue(":dm", QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":id", id);
    q.exec();
}

// ── Collections ───────────────────────────────────────────────

QVector<Collection> Database::allCollections() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT c.id, c.name, c.color, c.icon, COUNT(bc.book_id) AS cnt "
           "FROM collections c LEFT JOIN book_collections bc ON bc.collection_id=c.id "
           "GROUP BY c.id ORDER BY c.name COLLATE NOCASE");
    QVector<Collection> res;
    while (q.next()) {
        Collection c;
        c.id    = q.value("id").toInt();
        c.name  = q.value("name").toString();
        c.color = q.value("color").toString();
        c.icon  = q.value("icon").toString();
        c.bookCount = q.value("cnt").toInt();
        res << c;
    }
    return res;
}

int Database::createCollection(const QString& name, const QString& color, const QString& icon)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("INSERT OR IGNORE INTO collections (name,color,icon) VALUES (:n,:c,:i)");
    q.bindValue(":n", name);
    q.bindValue(":c", color.isEmpty() ? "#4f9cf9" : color);
    q.bindValue(":i", icon);
    q.exec();
    return q.lastInsertId().toInt();
}

bool Database::updateCollection(int id, const QString& name, const QString& color, const QString& icon)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE collections SET name=:n, color=:c, icon=:i WHERE id=:id");
    q.bindValue(":n",  name);
    q.bindValue(":c",  color);
    q.bindValue(":i",  icon);
    q.bindValue(":id", id);
    return q.exec();
}

bool Database::deleteCollection(int id)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("DELETE FROM collections WHERE id=:id");
    q.bindValue(":id", id);
    return q.exec();
}

bool Database::addBookToCollection(qint64 bookId, int collectionId)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("INSERT OR IGNORE INTO book_collections (book_id, collection_id) VALUES (:b,:c)");
    q.bindValue(":b", bookId);
    q.bindValue(":c", collectionId);
    return q.exec();
}

bool Database::removeBookFromCollection(qint64 bookId, int collectionId)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("DELETE FROM book_collections WHERE book_id=:b AND collection_id=:c");
    q.bindValue(":b", bookId);
    q.bindValue(":c", collectionId);
    return q.exec();
}

QList<int> Database::collectionsForBook(qint64 bookId) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("SELECT collection_id FROM book_collections WHERE book_id=:b");
    q.bindValue(":b", bookId);
    q.exec();
    QList<int> ids;
    while (q.next()) ids << q.value(0).toInt();
    return ids;
}

QVector<Book> Database::booksInCollection(int collectionId) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.setForwardOnly(true);
    q.prepare(QString("SELECT %1 FROM books b "
                      "JOIN book_collections bc ON bc.book_id=b.id "
                      "WHERE bc.collection_id=:cid ORDER BY b.title COLLATE NOCASE")
              .arg(kBookSelectColumns));
    q.bindValue(":cid", collectionId);
    q.exec();
    return booksFromQuery(q);
}

// ── Custom Fields ─────────────────────────────────────────────

QVector<CustomFieldDef> Database::customFieldDefs() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT name, type, default_value FROM custom_field_defs ORDER BY name");
    QVector<CustomFieldDef> defs;
    while (q.next()) {
        CustomFieldDef d;
        d.name         = q.value("name").toString();
        d.type         = q.value("type").toString();
        d.defaultValue = q.value("default_value").toString();
        defs << d;
    }
    return defs;
}

bool Database::addCustomField(const CustomFieldDef& def)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("INSERT OR IGNORE INTO custom_field_defs (name,type,default_value) VALUES (:n,:t,:d)");
    q.bindValue(":n", def.name);
    q.bindValue(":t", def.type.isEmpty() ? "text" : def.type);
    q.bindValue(":d", def.defaultValue);
    return q.exec();
}

bool Database::removeCustomField(const QString& name)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("DELETE FROM custom_field_defs WHERE name=:n");
    q.bindValue(":n", name);
    return q.exec();
}

QMap<QString, QVariant> Database::customFieldValues(qint64 bookId) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("SELECT field_name, value FROM custom_field_values WHERE book_id=:b");
    q.bindValue(":b", bookId);
    q.exec();
    QMap<QString, QVariant> map;
    while (q.next())
        map.insert(q.value("field_name").toString(), q.value("value"));
    return map;
}

bool Database::setCustomFieldValue(qint64 bookId, const QString& fieldName, const QVariant& value)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("INSERT OR REPLACE INTO custom_field_values (book_id, field_name, value) "
              "VALUES (:b, :f, :v)");
    q.bindValue(":b", bookId);
    q.bindValue(":f", fieldName);
    q.bindValue(":v", value.toString());
    return q.exec();
}

// ── Saved Searches ────────────────────────────────────────────

QVector<SavedSearch> Database::savedSearches() const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT id, name, query, filter_json FROM saved_searches ORDER BY name COLLATE NOCASE");
    QVector<SavedSearch> res;
    while (q.next()) {
        SavedSearch s;
        s.id         = q.value("id").toInt();
        s.name       = q.value("name").toString();
        s.query      = q.value("query").toString();
        s.filterJson = q.value("filter_json").toString();
        res << s;
    }
    return res;
}

int Database::saveSearch(const QString& name, const QString& query, const QString& filterJson)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("INSERT OR REPLACE INTO saved_searches (name, query, filter_json) VALUES (:n,:q,:f)");
    q.bindValue(":n", name);
    q.bindValue(":q", query);
    q.bindValue(":f", filterJson.isEmpty() ? "{}" : filterJson);
    q.exec();
    return q.lastInsertId().toInt();
}

bool Database::deleteSearch(int id)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("DELETE FROM saved_searches WHERE id=:id");
    q.bindValue(":id", id);
    return q.exec();
}

// ── Maintenance ───────────────────────────────────────────────
void Database::optimize()
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("PRAGMA optimize");
    q.exec("ANALYZE");
}

void Database::vacuum()
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("VACUUM");
    optimize();
}

void Database::reindex()
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("REINDEX");
    optimize();
}

int Database::removeMissingFiles()
{
    // Load all paths, check existence, delete missing
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT id, file_path FROM books");
    QVector<qint64> toRemove;
    while (q.next()) {
        if (!QFileInfo::exists(q.value("file_path").toString()))
            toRemove << q.value("id").toLongLong();
    }
    for (qint64 id : toRemove) removeBook(id);
    return toRemove.size();
}

int Database::removeBooksInFolders(const QStringList& folders)
{
    if (folders.isEmpty())
        return 0;

    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    QSqlQuery countQuery(db);
    QStringList clauses;
    QStringList normalizedFolders;

    for (const QString& folder : folders) {
        QString normalized = QDir::fromNativeSeparators(folder).trimmed();
        if (normalized.isEmpty())
            continue;
        while (normalized.endsWith('/'))
            normalized.chop(1);
        normalizedFolders << normalized;
        clauses << QString("(file_path = :exact%1 OR file_path LIKE :prefix%1)").arg(normalizedFolders.size() - 1);
    }

    if (clauses.isEmpty())
        return 0;

    const QString whereClause = clauses.join(" OR ");
    countQuery.prepare("SELECT COUNT(*) FROM books WHERE " + whereClause);
    for (int i = 0; i < normalizedFolders.size(); ++i) {
        countQuery.bindValue(QString(":exact%1").arg(i), normalizedFolders.at(i));
        countQuery.bindValue(QString(":prefix%1").arg(i), normalizedFolders.at(i) + "/%");
    }

    int removed = 0;
    if (countQuery.exec() && countQuery.next())
        removed = countQuery.value(0).toInt();

    if (removed <= 0)
        return 0;

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare("DELETE FROM books WHERE " + whereClause);
    for (int i = 0; i < normalizedFolders.size(); ++i) {
        deleteQuery.bindValue(QString(":exact%1").arg(i), normalizedFolders.at(i));
        deleteQuery.bindValue(QString(":prefix%1").arg(i), normalizedFolders.at(i) + "/%");
    }

    if (!deleteQuery.exec()) {
        qWarning() << "removeBooksInFolders:" << deleteQuery.lastError().text();
        return 0;
    }

    return removed;
}

int Database::removeBooksForFolders(const QStringList& folderPaths)
{
    if (folderPaths.isEmpty())
        return 0;

    int total = 0;
    for (const QString& folder : folderPaths) {
        const QString norm = QDir::fromNativeSeparators(folder.trimmed());
        if (norm.isEmpty()) continue;
        const QString prefix = norm.endsWith('/') ? norm : norm + '/';
        QSqlQuery q(QSqlDatabase::database("eagle_lib"));
        q.prepare("DELETE FROM books WHERE file_path = :exact OR file_path LIKE :like");
        q.bindValue(":exact", norm);
        q.bindValue(":like", prefix + '%');
        q.exec();
        total += q.numRowsAffected();
    }
    return total;
}

bool Database::exportLibrary(const QString& filePath, const QStringList& watchedFolders) const
{
    QJsonObject root;
    root["app"] = "Eagle Library";
    root["version"] = AppConfig::version();
    root["exported_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    root["watched_folders"] = QJsonArray::fromStringList(watchedFolders);

    QJsonArray booksArray;
    BookFilter filter;
    filter.restrictToPathPrefixes = true;
    filter.pathPrefixes = watchedFolders;
    const QVector<Book> books = query(filter, SortField::Title, SortOrder::Asc);
    for (const Book& book : books)
        booksArray.append(bookToJson(book));
    root["books"] = booksArray;

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "exportLibrary open failed:" << file.errorString();
        return false;
    }

    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(json) != json.size() || !file.commit()) {
        qWarning() << "exportLibrary write failed:" << file.errorString();
        return false;
    }
    return true;
}

int Database::importLibrary(const QString& filePath, QStringList* watchedFolders)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "importLibrary open failed:" << file.errorString();
        return 0;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qWarning() << "importLibrary invalid JSON";
        return 0;
    }

    const QJsonObject root = doc.object();
    if (watchedFolders) {
        watchedFolders->clear();
        for (const QJsonValue& v : root.value("watched_folders").toArray())
            watchedFolders->append(v.toString());
    }

    int imported = 0;
    const QJsonArray books = root.value("books").toArray();
    for (const QJsonValue& value : books) {
        if (!value.isObject())
            continue;
        Book b = bookFromJsonObject(value.toObject());
        if (upsertBook(b))
            ++imported;
    }
    return imported;
}
