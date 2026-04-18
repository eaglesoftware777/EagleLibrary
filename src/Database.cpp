// ============================================================
//  Eagle Library -- Database.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include "Database.h"
#include "AppConfig.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>

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
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA synchronous=NORMAL");
    q.exec("PRAGMA foreign_keys=ON");
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
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
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
            notes         TEXT
        )
    )");
    if (!ok) qWarning() << q.lastError().text();
    return ok;
}

bool Database::createIndexes()
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("CREATE INDEX IF NOT EXISTS idx_title    ON books(title COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_author   ON books(author COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_hash     ON books(file_hash)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_format   ON books(format)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_year     ON books(year)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_rating   ON books(rating)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_fav      ON books(is_favourite)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_added    ON books(date_added)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_opened   ON books(last_opened)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_filesize ON books(file_size)");
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
    return b;
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

    return clauses.isEmpty() ? "" : " WHERE " + clauses.join(" AND ");
}

// ── Rich query ────────────────────────────────────────────────
QVector<Book> Database::query(const BookFilter& filter,
                               SortField sort, SortOrder order,
                               int limit, int offset) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    QString sql = "SELECT * FROM books";
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
    if (filter.yearFrom > 0)        q.bindValue(":yf",   filter.yearFrom);
    if (filter.yearTo > 0)          q.bindValue(":yt",   filter.yearTo);
    q.exec();
    return booksFromQuery(q);
}

QVector<Book> Database::allBooks(SortField sort, SortOrder order) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("SELECT * FROM books" + sortClause(sort, order));
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
    q.prepare("SELECT * FROM books ORDER BY date_added DESC LIMIT :lim");
    q.bindValue(":lim", limit);
    q.exec();
    return booksFromQuery(q);
}

QVector<Book> Database::recentlyOpened(int limit) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("SELECT * FROM books WHERE last_opened != '' "
              "ORDER BY last_opened DESC LIMIT :lim");
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
    q.prepare("SELECT * FROM books WHERE id=:id");
    q.bindValue(":id", id);
    q.exec();
    if (q.next()) return bookFromQuery(q);
    return {};
}

// ── CRUD ──────────────────────────────────────────────────────
qint64 Database::insertBook(const Book& b)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare(R"(
        INSERT OR IGNORE INTO books
            (file_path,file_hash,format,file_size,title,author,publisher,
             isbn,language,year,pages,rating,description,tags,subjects,
             cover_path,has_cover,date_added,date_modified,last_opened,
             open_count,is_favourite,notes)
        VALUES
            (:fp,:fh,:fmt,:fs,:ti,:au,:pub,
             :isbn,:lang,:yr,:pg,:rt,:desc,:tags,:subj,
             :cp,:hc,:da,:dm,:lo,:oc,:fav,:notes)
    )");
    q.bindValue(":fp",   b.filePath);
    q.bindValue(":fh",   b.fileHash);
    q.bindValue(":fmt",  b.format);
    q.bindValue(":fs",   b.fileSize);
    q.bindValue(":ti",   b.title);
    q.bindValue(":au",   b.author);
    q.bindValue(":pub",  b.publisher);
    q.bindValue(":isbn", b.isbn);
    q.bindValue(":lang", b.language);
    q.bindValue(":yr",   b.year);
    q.bindValue(":pg",   b.pages);
    q.bindValue(":rt",   b.rating);
    q.bindValue(":desc", b.description);
    q.bindValue(":tags", b.tags.join("|"));
    q.bindValue(":subj", b.subjects.join("|"));
    q.bindValue(":cp",   b.coverPath);
    q.bindValue(":hc",   b.hasCover ? 1 : 0);
    q.bindValue(":da",   b.dateAdded.toString(Qt::ISODate));
    q.bindValue(":dm",   b.dateModified.toString(Qt::ISODate));
    q.bindValue(":lo",   b.lastOpened.toString(Qt::ISODate));
    q.bindValue(":oc",   b.openCount);
    q.bindValue(":fav",  b.isFavourite ? 1 : 0);
    q.bindValue(":notes",b.notes);
    if (!q.exec()) { qWarning() << "insertBook:" << q.lastError().text(); return -1; }
    return q.lastInsertId().toLongLong();
}

bool Database::updateBook(const Book& b)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare(R"(
        UPDATE books SET
            title=:ti, author=:au, publisher=:pub, isbn=:isbn,
            language=:lang, year=:yr, pages=:pg, rating=:rt,
            description=:desc, tags=:tags, subjects=:subj,
            cover_path=:cp, has_cover=:hc, date_modified=:dm,
            is_favourite=:fav, notes=:notes
        WHERE id=:id
    )");
    q.bindValue(":ti",   b.title);
    q.bindValue(":au",   b.author);
    q.bindValue(":pub",  b.publisher);
    q.bindValue(":isbn", b.isbn);
    q.bindValue(":lang", b.language);
    q.bindValue(":yr",   b.year);
    q.bindValue(":pg",   b.pages);
    q.bindValue(":rt",   b.rating);
    q.bindValue(":desc", b.description);
    q.bindValue(":tags", b.tags.join("|"));
    q.bindValue(":subj", b.subjects.join("|"));
    q.bindValue(":cp",   b.coverPath);
    q.bindValue(":hc",   b.hasCover ? 1 : 0);
    q.bindValue(":dm",   QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":fav",  b.isFavourite ? 1 : 0);
    q.bindValue(":notes",b.notes);
    q.bindValue(":id",   b.id);
    return q.exec();
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
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET title=:ti, author=:au, publisher=:pub, year=:yr, "
              "date_modified=:dm WHERE id=:id");
    q.bindValue(":ti",  title);
    q.bindValue(":au",  author);
    q.bindValue(":pub", publisher);
    q.bindValue(":yr",  year);
    q.bindValue(":dm",  QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":id",  id);
    q.exec();
}

// ── Maintenance ───────────────────────────────────────────────
void Database::vacuum()
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("VACUUM");
}

void Database::reindex()
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.exec("REINDEX");
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
