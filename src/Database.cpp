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
#include <QSqlRecord>

namespace {

constexpr auto kBookSelectColumns =
    "id,file_path,file_hash,format,file_size,title,author,publisher,"
    "isbn,language,year,pages,rating,description,tags,subjects,"
    "cover_path,has_cover,date_added,date_modified,last_opened,"
    "open_count,is_favourite,notes,"
    "COALESCE(reading_status,'') AS reading_status,"
    "COALESCE(progress_percent,0) AS progress_percent,"
    "COALESCE(current_page,0) AS current_page,"
    "COALESCE(date_started,'') AS date_started,"
    "COALESCE(date_finished,'') AS date_finished,"
    "COALESCE(last_reading_session,'') AS last_reading_session,"
    "COALESCE(reading_minutes,0) AS reading_minutes,"
    "COALESCE(loaned_to,'') AS loaned_to,"
    "COALESCE(loan_due_date,'') AS loan_due_date,"
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

bool hasReplacementLikeGlyphs(const QString& value)
{
    return value.contains(QChar::ReplacementCharacter)
        || value.contains(QChar(0x25A1))
        || value.contains(QChar(0x25A0))
        || value.contains(QStringLiteral("\uFFFD"));
}

bool looksLikePdfStructureText(const QString& value)
{
    const QString lowered = value.toLower();
    static const QStringList blockedTokens = {
        QStringLiteral("/filter"),
        QStringLiteral("/flatedecode"),
        QStringLiteral("/ascii85decode"),
        QStringLiteral("/lzwdecode"),
        QStringLiteral("/length"),
        QStringLiteral("/type"),
        QStringLiteral("/catalog"),
        QStringLiteral("/pages"),
        QStringLiteral("/metadata"),
        QStringLiteral("endstream"),
        QStringLiteral("endobj"),
        QStringLiteral("xref"),
        QStringLiteral("linearized"),
        QStringLiteral(" obj<<"),
        QStringLiteral(" obj <")
    };
    for (const QString& token : blockedTokens) {
        if (lowered.contains(token))
            return true;
    }

    static const QRegularExpression objRefRe(QStringLiteral(R"(\b\d+\s+\d+\s+obj\b)"),
                                             QRegularExpression::CaseInsensitiveOption);
    return objRefRe.match(lowered).hasMatch();
}

bool looksLikeBinaryishTitle(const QString& value)
{
    if (value.size() < 8)
        return false;

    int letters = 0;
    int digits = 0;
    int spaces = 0;
    int symbols = 0;
    int controls = 0;
    for (const QChar ch : value) {
        if (ch.isLetter())
            ++letters;
        else if (ch.isDigit())
            ++digits;
        else if (ch.isSpace())
            ++spaces;
        else if (ch.category() == QChar::Other_Control)
            ++controls;
        else
            ++symbols;
    }

    if (controls > 0)
        return true;
    if (letters < 3 && symbols >= value.size() / 3)
        return true;
    if ((letters + digits + spaces) * 2 < value.size())
        return true;
    return false;
}

bool looksLikeMojibakeText(const QString& value)
{
    static const QStringList markers = {
        QStringLiteral("Ã"), QStringLiteral("Â"), QStringLiteral("Ð"),
        QStringLiteral("Ñ"), QStringLiteral("â€"), QStringLiteral("ï»¿")
    };
    for (const QString& marker : markers) {
        if (value.contains(marker))
            return true;
    }
    return false;
}

bool looksSuspiciousTitleValue(const QString& value, const QString& filePath)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty())
        return true;
    if (hasReplacementLikeGlyphs(trimmed) || looksLikeMojibakeText(trimmed))
        return true;
    if (looksLikePdfStructureText(trimmed) || looksLikeBinaryishTitle(trimmed))
        return true;
    if (trimmed.size() <= 2)
        return true;
    const QString fileBase = QFileInfo(filePath).completeBaseName().trimmed();
    if (!fileBase.isEmpty() && trimmed.compare(fileBase, Qt::CaseInsensitive) == 0)
        return false;

    int punctuationCount = 0;
    for (const QChar ch : trimmed) {
        if (!ch.isLetterOrNumber() && !ch.isSpace())
            ++punctuationCount;
    }
    return trimmed.size() >= 6 && punctuationCount > trimmed.size() / 2;
}

QString safeTitleForBook(const Book& book)
{
    const QString fileBase = sanitizeTextValue(QFileInfo(book.filePath).completeBaseName());
    const QString title = sanitizeTextValue(book.title);
    return looksSuspiciousTitleValue(title, book.filePath) ? fileBase : title;
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
    book.title = safeTitleForBook(book);
    book.author = sanitizeTextValue(book.author);
    if (looksLikeMojibakeText(book.author) || hasReplacementLikeGlyphs(book.author))
        book.author.clear();
    book.publisher = sanitizeTextValue(book.publisher);
    if (looksLikeMojibakeText(book.publisher) || hasReplacementLikeGlyphs(book.publisher))
        book.publisher.clear();
    book.isbn = sanitizeTextValue(book.isbn);
    book.language = sanitizeTextValue(book.language);
    book.description = sanitizeTextValue(book.description);
    book.coverPath = sanitizeTextValue(book.coverPath);
    book.notes = sanitizeTextValue(book.notes);
    book.readingStatus = sanitizeTextValue(book.readingStatus);
    book.loanedTo = sanitizeTextValue(book.loanedTo);
    book.progressPercent = qBound(0, book.progressPercent, 100);
    book.currentPage = qMax(0, book.currentPage);
    book.readingMinutes = qMax(0, book.readingMinutes);
    book.series = sanitizeTextValue(book.series);
    book.edition = sanitizeTextValue(book.edition);
    book.tags = sanitizeTextList(book.tags);
    book.subjects = sanitizeTextList(book.subjects);
    return book;
}

QString toFtsMatchExpression(const QString& raw)
{
    QStringList positiveTerms;
    QRegularExpression tokenRe(R"re((\w+):"([^"]+)"|"([^"]+)"|(\S+))re");
    QRegularExpressionMatchIterator it = tokenRe.globalMatch(raw);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        QString token;
        if (!match.captured(1).isEmpty())
            token = match.captured(2);
        else if (!match.captured(3).isEmpty())
            token = match.captured(3);
        else
            token = match.captured(4);

        token = token.trimmed();
        if (token.isEmpty() || token.compare(QStringLiteral("and"), Qt::CaseInsensitive) == 0)
            continue;
        if (token.startsWith('-'))
            continue;

        token.replace('"', ' ');
        token = token.simplified();
        if (token.isEmpty())
            continue;

        if (token.contains(' '))
            positiveTerms << QString("\"%1\"").arg(token);
        else
            positiveTerms << token;
    }
    return positiveTerms.join(" AND ");
}

QString buildPathPrefixClause(const QStringList& prefixes, QSqlQuery& q, const QString& tableAlias = QString())
{
    QStringList pathClauses;
    const QString pathColumn = tableAlias.isEmpty() ? QStringLiteral("file_path") : tableAlias + QStringLiteral(".file_path");
    for (int i = 0; i < prefixes.size(); ++i) {
        QString prefix = QDir::fromNativeSeparators(prefixes.at(i)).trimmed();
        while (prefix.endsWith('/'))
            prefix.chop(1);
        if (prefix.isEmpty())
            continue;
        const QString exactKey = QString(":pathExact%1").arg(i);
        const QString likeKey = QString(":pathLike%1").arg(i);
        pathClauses << QString("(%1 = %2 OR %1 LIKE %3)").arg(pathColumn, exactKey, likeKey);
        q.bindValue(exactKey, prefix);
        q.bindValue(likeKey, prefix + "/%");
    }
    return pathClauses.isEmpty() ? QString() : "(" + pathClauses.join(" OR ") + ")";
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
    obj["reading_status"] = b.readingStatus;
    obj["progress_percent"] = b.progressPercent;
    obj["current_page"] = b.currentPage;
    obj["date_started"] = b.dateStarted.toString(Qt::ISODate);
    obj["date_finished"] = b.dateFinished.toString(Qt::ISODate);
    obj["last_reading_session"] = b.lastReadingSession.toString(Qt::ISODate);
    obj["reading_minutes"] = b.readingMinutes;
    obj["loaned_to"] = b.loanedTo;
    obj["loan_due_date"] = b.loanDueDate.toString(Qt::ISODate);
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
    b.readingStatus = obj.value("reading_status").toString();
    b.progressPercent = obj.value("progress_percent").toInt();
    b.currentPage = obj.value("current_page").toInt();
    b.dateStarted = QDateTime::fromString(obj.value("date_started").toString(), Qt::ISODate);
    b.dateFinished = QDateTime::fromString(obj.value("date_finished").toString(), Qt::ISODate);
    b.lastReadingSession = QDateTime::fromString(obj.value("last_reading_session").toString(), Qt::ISODate);
    b.readingMinutes = obj.value("reading_minutes").toInt();
    b.loanedTo = obj.value("loaned_to").toString();
    b.loanDueDate = QDateTime::fromString(obj.value("loan_due_date").toString(), Qt::ISODate);
    return sanitizedBook(b);
}

bool hasFtsTable(const QSqlDatabase& db)
{
    QSqlQuery q(db);
    q.prepare("SELECT 1 FROM sqlite_master WHERE type='table' AND name='books_fts' LIMIT 1");
    q.exec();
    return q.next();
}

qint64 bookIdForPath(const QSqlDatabase& db, const QString& filePath)
{
    QSqlQuery q(db);
    q.prepare("SELECT id FROM books WHERE file_path=:fp LIMIT 1");
    q.bindValue(":fp", filePath);
    q.exec();
    return q.next() ? q.value(0).toLongLong() : 0;
}

QString queryValue(const QSqlQuery& query, const QString& fieldName)
{
    const int index = query.record().indexOf(fieldName);
    return index >= 0 ? query.value(index).toString() : QString();
}

int queryIntValue(const QSqlQuery& query, const QString& fieldName)
{
    const int index = query.record().indexOf(fieldName);
    return index >= 0 ? query.value(index).toInt() : 0;
}

qint64 queryInt64Value(const QSqlQuery& query, const QString& fieldName)
{
    const int index = query.record().indexOf(fieldName);
    return index >= 0 ? query.value(index).toLongLong() : 0;
}

double queryDoubleValue(const QSqlQuery& query, const QString& fieldName)
{
    const int index = query.record().indexOf(fieldName);
    return index >= 0 ? query.value(index).toDouble() : 0.0;
}

bool queryBoolValue(const QSqlQuery& query, const QString& fieldName)
{
    const int index = query.record().indexOf(fieldName);
    return index >= 0 ? query.value(index).toBool() : false;
}

QStringList splitStoredList(const QString& stored)
{
    QStringList out = stored.split(',', Qt::SkipEmptyParts);
    for (QString& item : out)
        item = item.trimmed();
    out.removeAll(QString());
    out.removeDuplicates();
    return out;
}

void syncBookFileRecord(const QSqlDatabase& db, qint64 bookId, const Book& book)
{
    if (bookId <= 0 || book.filePath.isEmpty())
        return;

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO book_files
            (book_id, file_path, file_hash, file_size, is_primary, last_seen, exists_flag)
        VALUES
            (:book_id, :file_path, :file_hash, :file_size, 1, :last_seen, 1)
        ON CONFLICT(file_path) DO UPDATE SET
            book_id = excluded.book_id,
            file_hash = excluded.file_hash,
            file_size = excluded.file_size,
            is_primary = 1,
            last_seen = excluded.last_seen,
            exists_flag = 1
    )");
    q.bindValue(":book_id", bookId);
    q.bindValue(":file_path", book.filePath);
    q.bindValue(":file_hash", book.fileHash);
    q.bindValue(":file_size", book.fileSize);
    q.bindValue(":last_seen", QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec())
        qWarning() << "syncBookFileRecord:" << q.lastError().text();
}

bool registerFileLocationRecord(const QSqlDatabase& db, qint64 bookId, const QString& filePath,
                                const QString& fileHash, qint64 fileSize, bool isPrimary)
{
    if (bookId <= 0 || filePath.isEmpty())
        return false;

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO book_files
            (book_id, file_path, file_hash, file_size, is_primary, last_seen, exists_flag)
        VALUES
            (:book_id, :file_path, :file_hash, :file_size, :is_primary, :last_seen, 1)
        ON CONFLICT(file_path) DO UPDATE SET
            book_id = excluded.book_id,
            file_hash = excluded.file_hash,
            file_size = excluded.file_size,
            is_primary = excluded.is_primary,
            last_seen = excluded.last_seen,
            exists_flag = 1
    )");
    q.bindValue(":book_id", bookId);
    q.bindValue(":file_path", filePath);
    q.bindValue(":file_hash", fileHash);
    q.bindValue(":file_size", fileSize);
    q.bindValue(":is_primary", isPrimary ? 1 : 0);
    q.bindValue(":last_seen", QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "registerFileLocationRecord:" << q.lastError().text();
        return false;
    }
    return true;
}

void removeBookSearchRow(const QSqlDatabase& db, qint64 bookId)
{
    if (!hasFtsTable(db) || bookId <= 0)
        return;
    QSqlQuery q(db);
    q.prepare("DELETE FROM books_fts WHERE rowid=:id");
    q.bindValue(":id", bookId);
    q.exec();
}

void syncBookSearchRow(const QSqlDatabase& db, qint64 bookId, const Book& book)
{
    if (!hasFtsTable(db) || bookId <= 0)
        return;

    removeBookSearchRow(db, bookId);

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO books_fts
            (rowid, title, author, publisher, isbn, description, tags, subjects, notes, reading_status, loaned_to, series, file_path)
        VALUES
            (:rowid, :title, :author, :publisher, :isbn, :description, :tags, :subjects, :notes, :reading_status, :loaned_to, :series, :file_path)
    )");
    q.bindValue(":rowid", bookId);
    q.bindValue(":title", book.title);
    q.bindValue(":author", book.author);
    q.bindValue(":publisher", book.publisher);
    q.bindValue(":isbn", book.isbn);
    q.bindValue(":description", book.description);
    q.bindValue(":tags", book.tags.join(" "));
    q.bindValue(":subjects", book.subjects.join(" "));
    q.bindValue(":notes", book.notes);
    q.bindValue(":reading_status", book.readingStatus);
    q.bindValue(":loaned_to", book.loanedTo);
    q.bindValue(":series", book.series);
    q.bindValue(":file_path", book.filePath);
    if (!q.exec())
        qWarning() << "syncBookSearchRow:" << q.lastError().text();
}

void syncBookArtifacts(const QSqlDatabase& db, qint64 bookId, const Book& book)
{
    syncBookFileRecord(db, bookId, book);
    syncBookSearchRow(db, bookId, book);
}

} // namespace

QJsonObject BookFilter::toJson() const
{
    QJsonObject obj;
    obj["text"] = text;
    obj["format"] = format;
    obj["author"] = author;
    obj["tag"] = tag;
    obj["language"] = language;
    obj["collection"] = collection;
    obj["series"] = series;
    obj["restrictToPathPrefixes"] = restrictToPathPrefixes;
    obj["yearFrom"] = yearFrom;
    obj["yearTo"] = yearTo;
    obj["favOnly"] = favOnly;
    obj["noCover"] = noCover;
    obj["noMeta"] = noMeta;
    obj["pathPrefixes"] = QJsonArray::fromStringList(pathPrefixes);
    return obj;
}

BookFilter BookFilter::fromJson(const QString& jsonText)
{
    BookFilter filter;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8());
    if (!doc.isObject())
        return filter;
    const QJsonObject obj = doc.object();
    filter.text = obj.value("text").toString();
    filter.format = obj.value("format").toString();
    filter.author = obj.value("author").toString();
    filter.tag = obj.value("tag").toString();
    filter.language = obj.value("language").toString();
    filter.collection = obj.value("collection").toString();
    filter.series = obj.value("series").toString();
    filter.restrictToPathPrefixes = obj.value("restrictToPathPrefixes").toBool(false);
    filter.yearFrom = obj.value("yearFrom").toInt();
    filter.yearTo = obj.value("yearTo").toInt();
    filter.favOnly = obj.value("favOnly").toBool(false);
    filter.noCover = obj.value("noCover").toBool(false);
    filter.noMeta = obj.value("noMeta").toBool(false);
    for (const QJsonValue& value : obj.value("pathPrefixes").toArray())
        filter.pathPrefixes << value.toString();
    return filter;
}

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
    const bool ok = createTables() && createIndexes();
    if (ok) {
        // Only rebuild the FTS index when it is empty (fresh DB or out-of-sync).
        // Incremental updates via syncBookSearchRow keep it current during normal use.
        QSqlQuery countQ(db);
        countQ.exec("SELECT COUNT(*) FROM books_fts");
        const int ftsCount = countQ.next() ? countQ.value(0).toInt() : 0;
        QSqlQuery bookCountQ(db);
        bookCountQ.exec("SELECT COUNT(*) FROM books");
        const int bookCount = bookCountQ.next() ? bookCountQ.value(0).toInt() : 0;
        if (ftsCount == 0 && bookCount > 0)
            rebuildSearchIndex();
    }
    return ok;
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
            reading_status TEXT DEFAULT '',
            progress_percent INTEGER DEFAULT 0,
            current_page INTEGER DEFAULT 0,
            date_started TEXT,
            date_finished TEXT,
            last_reading_session TEXT,
            reading_minutes INTEGER DEFAULT 0,
            loaned_to TEXT DEFAULT '',
            loan_due_date TEXT,
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
    q.exec("ALTER TABLE books ADD COLUMN reading_status TEXT DEFAULT ''");
    q.exec("ALTER TABLE books ADD COLUMN progress_percent INTEGER DEFAULT 0");
    q.exec("ALTER TABLE books ADD COLUMN current_page INTEGER DEFAULT 0");
    q.exec("ALTER TABLE books ADD COLUMN date_started TEXT");
    q.exec("ALTER TABLE books ADD COLUMN date_finished TEXT");
    q.exec("ALTER TABLE books ADD COLUMN last_reading_session TEXT");
    q.exec("ALTER TABLE books ADD COLUMN reading_minutes INTEGER DEFAULT 0");
    q.exec("ALTER TABLE books ADD COLUMN loaned_to TEXT DEFAULT ''");
    q.exec("ALTER TABLE books ADD COLUMN loan_due_date TEXT");

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

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS book_files (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            book_id    INTEGER NOT NULL REFERENCES books(id) ON DELETE CASCADE,
            file_path  TEXT NOT NULL UNIQUE,
            file_hash  TEXT,
            file_size  INTEGER DEFAULT 0,
            is_primary INTEGER DEFAULT 1,
            last_seen  TEXT,
            exists_flag INTEGER DEFAULT 1
        )
    )");

    if (!q.exec(R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS books_fts USING fts5(
            title,
            author,
            publisher,
            isbn,
            description,
            tags,
            subjects,
            notes,
            reading_status,
            loaned_to,
            series,
            file_path
        )
    )")) {
        qWarning() << "books_fts unavailable:" << q.lastError().text();
    }

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
    q.exec("CREATE INDEX IF NOT EXISTS idx_publisher_fmt ON books(publisher COLLATE NOCASE, format)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_reading_status ON books(reading_status COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_progress_percent ON books(progress_percent)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_loaned_to ON books(loaned_to COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_format_year ON books(format, year)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_fav_opened ON books(is_favourite, last_opened)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_series     ON books(series COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_book_files_book  ON book_files(book_id)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_book_files_hash  ON book_files(file_hash)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_book_files_seen  ON book_files(last_seen)");
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
    b.readingStatus = q.value("reading_status").toString();
    b.progressPercent = q.value("progress_percent").toInt();
    b.currentPage = q.value("current_page").toInt();
    b.dateStarted = QDateTime::fromString(q.value("date_started").toString(), Qt::ISODate);
    b.dateFinished = QDateTime::fromString(q.value("date_finished").toString(), Qt::ISODate);
    b.lastReadingSession = QDateTime::fromString(q.value("last_reading_session").toString(), Qt::ISODate);
    b.readingMinutes = q.value("reading_minutes").toInt();
    b.loanedTo = q.value("loaned_to").toString();
    b.loanDueDate = QDateTime::fromString(q.value("loan_due_date").toString(), Qt::ISODate);
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
        case SortField::Publisher: col = "publisher COLLATE NOCASE"; break;
        case SortField::Year:      col = "year"; break;
        case SortField::DateAdded: col = "date_added"; break;
        case SortField::Rating:    col = "rating"; break;
        case SortField::Progress:  col = "progress_percent"; break;
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
    if (!f.readingStatus.isEmpty()) {
        clauses << "reading_status = :readingStatus";
        q.bindValue(":readingStatus", f.readingStatus);
    }
    if (!f.language.isEmpty()) {
        clauses << "language = :lang";
        q.bindValue(":lang", f.language);
    }
    if (f.restrictToPathPrefixes && f.pathPrefixes.isEmpty()) {
        clauses << "0 = 1";
    } else if (!f.pathPrefixes.isEmpty()) {
        const QString pathClause = buildPathPrefixClause(f.pathPrefixes, q);
        if (!pathClause.isEmpty())
            clauses << pathClause;
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
    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    QSqlQuery q(db);
    q.setForwardOnly(true);
    const QString ftsQuery = toFtsMatchExpression(filter.text);
    QString sql;
    if (!ftsQuery.isEmpty() && hasFtsTable(db)) {
        sql = QString("SELECT %1 FROM books "
                      "JOIN books_fts ON books_fts.rowid = books.id").arg(kBookSelectColumns);
        QStringList clauses;
        clauses << "books_fts MATCH :ftsQuery";
        q.bindValue(":ftsQuery", ftsQuery);

        BookFilter residual = filter;
        residual.text.clear();
        const QString residualClause = buildWhereClause(residual, q);
        if (!residualClause.isEmpty())
            clauses << residualClause.mid(QStringLiteral(" WHERE ").size());
        sql += " WHERE " + clauses.join(" AND ");
    } else {
        sql = QString("SELECT %1 FROM books").arg(kBookSelectColumns);
        sql += buildWhereClause(filter, q);
    }
    sql += sortClause(sort, order);
    if (limit > 0)  sql += QString(" LIMIT %1").arg(limit);
    if (offset > 0) sql += QString(" OFFSET %1").arg(offset);
    q.prepare(sql);
    // rebind (Qt needs prepare before bind for named params)
    if (!ftsQuery.isEmpty() && hasFtsTable(db))
        q.bindValue(":ftsQuery", ftsQuery);
    else if (!filter.text.isEmpty())
        q.bindValue(":txt",  "%" + filter.text + "%");
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
    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    QSqlQuery q(db);
    q.prepare(R"(
        INSERT OR IGNORE INTO books
            (file_path,file_hash,format,file_size,title,author,publisher,
             isbn,language,year,pages,rating,description,tags,subjects,
             cover_path,has_cover,date_added,date_modified,last_opened,
             open_count,is_favourite,notes,reading_status,progress_percent,current_page,
             date_started,date_finished,last_reading_session,reading_minutes,loaned_to,loan_due_date,
             series,series_index,edition)
        VALUES
            (:fp,:fh,:fmt,:fs,:ti,:au,:pub,
             :isbn,:lang,:yr,:pg,:rt,:desc,:tags,:subj,
             :cp,:hc,:da,:dm,:lo,:oc,:fav,:notes,:reading_status,:progress_percent,:current_page,
             :date_started,:date_finished,:last_reading_session,:reading_minutes,:loaned_to,:loan_due_date,
             :ser,:seridx,:ed)
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
    q.bindValue(":reading_status", clean.readingStatus);
    q.bindValue(":progress_percent", clean.progressPercent);
    q.bindValue(":current_page", clean.currentPage);
    q.bindValue(":date_started", clean.dateStarted.toString(Qt::ISODate));
    q.bindValue(":date_finished", clean.dateFinished.toString(Qt::ISODate));
    q.bindValue(":last_reading_session", clean.lastReadingSession.toString(Qt::ISODate));
    q.bindValue(":reading_minutes", clean.readingMinutes);
    q.bindValue(":loaned_to", clean.loanedTo);
    q.bindValue(":loan_due_date", clean.loanDueDate.toString(Qt::ISODate));
    q.bindValue(":ser",    clean.series);
    q.bindValue(":seridx", clean.seriesIndex);
    q.bindValue(":ed",     clean.edition);
    if (!q.exec()) { qWarning() << "insertBook:" << q.lastError().text(); return -1; }
    const qint64 id = q.lastInsertId().toLongLong();
    syncBookArtifacts(db, id, clean);
    return id;
}

bool Database::updateBook(const Book& b)
{
    const Book clean = sanitizedBook(b);
    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    QSqlQuery q(db);
    q.prepare(R"(
        UPDATE books SET
            title=:ti, author=:au, publisher=:pub, isbn=:isbn,
            language=:lang, year=:yr, pages=:pg, rating=:rt,
            description=:desc, tags=:tags, subjects=:subj,
            cover_path=:cp, has_cover=:hc, date_modified=:dm,
            is_favourite=:fav, notes=:notes,
            reading_status=:reading_status, progress_percent=:progress_percent, current_page=:current_page,
            date_started=:date_started, date_finished=:date_finished, last_reading_session=:last_reading_session,
            reading_minutes=:reading_minutes, loaned_to=:loaned_to, loan_due_date=:loan_due_date,
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
    q.bindValue(":reading_status", clean.readingStatus);
    q.bindValue(":progress_percent", clean.progressPercent);
    q.bindValue(":current_page", clean.currentPage);
    q.bindValue(":date_started", clean.dateStarted.toString(Qt::ISODate));
    q.bindValue(":date_finished", clean.dateFinished.toString(Qt::ISODate));
    q.bindValue(":last_reading_session", clean.lastReadingSession.toString(Qt::ISODate));
    q.bindValue(":reading_minutes", clean.readingMinutes);
    q.bindValue(":loaned_to", clean.loanedTo);
    q.bindValue(":loan_due_date", clean.loanDueDate.toString(Qt::ISODate));
    q.bindValue(":ser",    clean.series);
    q.bindValue(":seridx", clean.seriesIndex);
    q.bindValue(":ed",     clean.edition);
    q.bindValue(":id",     clean.id);
    const bool ok = q.exec();
    if (ok)
        syncBookArtifacts(db, clean.id, clean);
    return ok;
}

bool Database::upsertBook(const Book& b)
{
    const Book clean = sanitizedBook(b);
    if (clean.filePath.isEmpty())
        return false;

    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO books
            (file_path,file_hash,format,file_size,title,author,publisher,
             isbn,language,year,pages,rating,description,tags,subjects,
             cover_path,has_cover,date_added,date_modified,last_opened,
             open_count,is_favourite,notes,reading_status,progress_percent,current_page,
             date_started,date_finished,last_reading_session,reading_minutes,loaned_to,loan_due_date,
             series,series_index,edition)
        VALUES
            (:fp,:fh,:fmt,:fs,:ti,:au,:pub,
             :isbn,:lang,:yr,:pg,:rt,:desc,:tags,:subj,
             :cp,:hc,:da,:dm,:lo,:oc,:fav,:notes,:reading_status,:progress_percent,:current_page,
             :date_started,:date_finished,:last_reading_session,:reading_minutes,:loaned_to,:loan_due_date,
             :ser,:seridx,:ed)
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
            reading_status=excluded.reading_status,
            progress_percent=excluded.progress_percent,
            current_page=excluded.current_page,
            date_started=excluded.date_started,
            date_finished=excluded.date_finished,
            last_reading_session=excluded.last_reading_session,
            reading_minutes=excluded.reading_minutes,
            loaned_to=excluded.loaned_to,
            loan_due_date=excluded.loan_due_date,
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
    q.bindValue(":reading_status", clean.readingStatus);
    q.bindValue(":progress_percent", clean.progressPercent);
    q.bindValue(":current_page", clean.currentPage);
    q.bindValue(":date_started", clean.dateStarted.toString(Qt::ISODate));
    q.bindValue(":date_finished", clean.dateFinished.toString(Qt::ISODate));
    q.bindValue(":last_reading_session", clean.lastReadingSession.toString(Qt::ISODate));
    q.bindValue(":reading_minutes", clean.readingMinutes);
    q.bindValue(":loaned_to", clean.loanedTo);
    q.bindValue(":loan_due_date", clean.loanDueDate.toString(Qt::ISODate));
    q.bindValue(":ser",    clean.series);
    q.bindValue(":seridx", clean.seriesIndex);
    q.bindValue(":ed",     clean.edition);
    if (!q.exec()) {
        qWarning() << "upsertBook:" << q.lastError().text();
        return false;
    }
    const qint64 id = bookIdForPath(db, clean.filePath);
    syncBookArtifacts(db, id, clean);
    return true;
}

bool Database::removeBook(qint64 id)
{
    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    removeBookSearchRow(db, id);
    QSqlQuery q(db);
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

qint64 Database::bookIdByHash(const QString& hash) const
{
    if (hash.isEmpty())
        return 0;
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("SELECT id FROM books WHERE file_hash=:h LIMIT 1");
    q.bindValue(":h", hash);
    q.exec();
    return q.next() ? q.value(0).toLongLong() : 0;
}

bool Database::registerFileLocation(qint64 bookId, const QString& filePath,
                                    const QString& fileHash, qint64 fileSize, bool isPrimary)
{
    return registerFileLocationRecord(QSqlDatabase::database("eagle_lib"), bookId, filePath, fileHash, fileSize, isPrimary);
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
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    q.prepare("UPDATE books SET last_opened=:t, open_count=open_count+1, "
              "last_reading_session=:t, "
              "reading_status=CASE WHEN COALESCE(reading_status,'')='' OR reading_status='Want to Read' THEN 'Reading' ELSE reading_status END, "
              "date_started=CASE WHEN COALESCE(date_started,'')='' THEN :t ELSE date_started END "
              "WHERE id=:id");
    q.bindValue(":t",  now);
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

void Database::updateReadingState(qint64 id,
                                  const QString& readingStatus,
                                  int progressPercent,
                                  int currentPage,
                                  const QDateTime& dateStarted,
                                  const QDateTime& dateFinished,
                                  int readingMinutes)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET reading_status=:status, progress_percent=:progress, current_page=:page, "
              "date_started=:started, date_finished=:finished, last_reading_session=:session, "
              "reading_minutes=:minutes, date_modified=:dm WHERE id=:id");
    q.bindValue(":status", sanitizeTextValue(readingStatus));
    q.bindValue(":progress", qBound(0, progressPercent, 100));
    q.bindValue(":page", qMax(0, currentPage));
    q.bindValue(":started", dateStarted.toString(Qt::ISODate));
    q.bindValue(":finished", dateFinished.toString(Qt::ISODate));
    q.bindValue(":session", QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":minutes", qMax(0, readingMinutes));
    q.bindValue(":dm", QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(":id", id);
    q.exec();
}

void Database::updateLoanState(qint64 id, const QString& loanedTo, const QDateTime& loanDueDate)
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("UPDATE books SET loaned_to=:loanedTo, loan_due_date=:dueDate, date_modified=:dm WHERE id=:id");
    q.bindValue(":loanedTo", sanitizeTextValue(loanedTo));
    q.bindValue(":dueDate", loanDueDate.toString(Qt::ISODate));
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

void Database::rebuildSearchIndex()
{
    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    if (!hasFtsTable(db))
        return;

    QSqlQuery wipe(db);
    wipe.exec("DELETE FROM books_fts");

    QSqlQuery q(db);
    q.exec(QString("SELECT %1 FROM books").arg(kBookSelectColumns));
    while (q.next()) {
        const Book book = bookFromQuery(q);
        syncBookSearchRow(db, book.id, book);
        syncBookFileRecord(db, book.id, book);
    }
}

int Database::removeMissingFiles()
{
    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    QSqlQuery q(db);
    q.exec("SELECT id, file_path FROM books");
    int removed = 0;
    while (q.next()) {
        const qint64 bookId = q.value("id").toLongLong();
        const QString primaryPath = q.value("file_path").toString();
        const QStringList locations = fileLocationsForBook(bookId);
        QStringList existingLocations;

        for (const QString& location : locations) {
            const bool exists = QFileInfo::exists(location);
            QSqlQuery mark(db);
            mark.prepare("UPDATE book_files SET exists_flag=:exists, last_seen=:seen WHERE book_id=:id AND file_path=:path");
            mark.bindValue(":exists", exists ? 1 : 0);
            mark.bindValue(":seen", QDateTime::currentDateTime().toString(Qt::ISODate));
            mark.bindValue(":id", bookId);
            mark.bindValue(":path", location);
            mark.exec();
            if (exists)
                existingLocations << location;
        }

        if (QFileInfo::exists(primaryPath)) {
            if (existingLocations.isEmpty()) {
                const QFileInfo info(primaryPath);
                registerFileLocation(bookId, primaryPath, {}, info.size(), true);
            }
            continue;
        }

        if (!existingLocations.isEmpty()) {
            const QString promotedPath = existingLocations.first();
            const QFileInfo info(promotedPath);
            QSqlQuery update(db);
            update.prepare("UPDATE books SET file_path=:path, file_size=:size, date_modified=:dm WHERE id=:id");
            update.bindValue(":path", promotedPath);
            update.bindValue(":size", info.size());
            update.bindValue(":dm", info.lastModified().toString(Qt::ISODate));
            update.bindValue(":id", bookId);
            update.exec();

            QSqlQuery demote(db);
            demote.prepare("UPDATE book_files SET is_primary=0 WHERE book_id=:id");
            demote.bindValue(":id", bookId);
            demote.exec();
            registerFileLocation(bookId, promotedPath, {}, info.size(), true);
            continue;
        }

        removeBook(bookId);
        ++removed;
    }
    return removed;
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

    QVector<qint64> idsToRemove;
    QSqlQuery idQuery(db);
    idQuery.prepare("SELECT id FROM books WHERE " + whereClause);
    for (int i = 0; i < normalizedFolders.size(); ++i) {
        idQuery.bindValue(QString(":exact%1").arg(i), normalizedFolders.at(i));
        idQuery.bindValue(QString(":prefix%1").arg(i), normalizedFolders.at(i) + "/%");
    }
    if (idQuery.exec()) {
        while (idQuery.next())
            idsToRemove << idQuery.value(0).toLongLong();
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

    for (qint64 id : idsToRemove)
        removeBookSearchRow(db, id);

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
        QSqlDatabase db = QSqlDatabase::database("eagle_lib");
        QVector<qint64> idsToRemove;
        QSqlQuery ids(db);
        ids.prepare("SELECT id FROM books WHERE file_path = :exact OR file_path LIKE :like");
        ids.bindValue(":exact", norm);
        ids.bindValue(":like", prefix + '%');
        if (ids.exec()) {
            while (ids.next())
                idsToRemove << ids.value(0).toLongLong();
        }

        QSqlQuery q(db);
        q.prepare("DELETE FROM books WHERE file_path = :exact OR file_path LIKE :like");
        q.bindValue(":exact", norm);
        q.bindValue(":like", prefix + '%');
        q.exec();
        total += q.numRowsAffected();
        for (qint64 id : idsToRemove)
            removeBookSearchRow(db, id);
    }
    return total;
}

int Database::remapPathPrefix(const QString& fromPrefix, const QString& toPrefix)
{
    QString from = QDir::fromNativeSeparators(fromPrefix.trimmed());
    QString to = QDir::fromNativeSeparators(toPrefix.trimmed());
    while (from.endsWith('/'))
        from.chop(1);
    while (to.endsWith('/'))
        to.chop(1);
    if (from.isEmpty() || to.isEmpty() || from == to)
        return 0;

    QSqlDatabase db = QSqlDatabase::database("eagle_lib");
    if (!db.isOpen())
        return 0;

    QSqlQuery select(db);
    select.prepare("SELECT id, file_path FROM books WHERE file_path = :exact OR file_path LIKE :like");
    select.bindValue(":exact", from);
    select.bindValue(":like", from + "/%");
    if (!select.exec()) {
        qWarning() << "remapPathPrefix select failed:" << select.lastError().text();
        return 0;
    }

    struct PathRow { qint64 id; QString oldPath; QString newPath; };
    QVector<PathRow> rows;
    while (select.next()) {
        const qint64 id = select.value(0).toLongLong();
        const QString oldPath = select.value(1).toString();
        QString newPath = oldPath;
        if (oldPath == from)
            newPath = to;
        else if (oldPath.startsWith(from + "/"))
            newPath = to + oldPath.mid(from.size());
        if (newPath != oldPath)
            rows.push_back({id, oldPath, newPath});
    }
    if (rows.isEmpty())
        return 0;

    db.transaction();
    int changed = 0;
    for (const PathRow& row : rows) {
        QSqlQuery updateBook(db);
        updateBook.prepare("UPDATE books SET file_path=:newPath WHERE id=:id");
        updateBook.bindValue(":newPath", row.newPath);
        updateBook.bindValue(":id", row.id);
        if (!updateBook.exec()) {
            qWarning() << "remapPathPrefix update books failed:" << updateBook.lastError().text();
            continue;
        }

        QSqlQuery updateFiles(db);
        updateFiles.prepare("UPDATE book_files SET file_path=:newPath WHERE file_path=:oldPath");
        updateFiles.bindValue(":newPath", row.newPath);
        updateFiles.bindValue(":oldPath", row.oldPath);
        if (!updateFiles.exec())
            qWarning() << "remapPathPrefix update book_files failed:" << updateFiles.lastError().text();
        ++changed;
    }
    db.commit();
    rebuildSearchIndex();
    return changed;
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
    for (const Book& book : books) {
        QJsonObject obj = bookToJson(book);
        obj["file_locations"] = QJsonArray::fromStringList(fileLocationsForBook(book.id));
        booksArray.append(obj);
    }
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
        const QJsonObject obj = value.toObject();
        Book b = bookFromJsonObject(obj);
        if (upsertBook(b)) {
            ++imported;
            const qint64 bookId = bookIdForPath(QSqlDatabase::database("eagle_lib"), b.filePath);
            for (const QJsonValue& locationValue : obj.value("file_locations").toArray()) {
                const QString location = locationValue.toString();
                if (!location.isEmpty())
                    registerFileLocation(bookId, location, b.fileHash, b.fileSize, location == b.filePath);
            }
        }
    }
    return imported;
}

int Database::importFromDatabase(const QString& filePath)
{
    const QFileInfo sourceInfo(filePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        qWarning() << "importFromDatabase missing file:" << filePath;
        return 0;
    }

    const QString connectionName = QStringLiteral("eagle_import_%1")
                                       .arg(QDateTime::currentMSecsSinceEpoch());
    int imported = 0;
    {
        QSqlDatabase sourceDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        sourceDb.setDatabaseName(sourceInfo.absoluteFilePath());
        if (!sourceDb.open()) {
            qWarning() << "importFromDatabase open failed:" << sourceDb.lastError().text();
            return 0;
        }

        QSqlQuery select(sourceDb);
        if (!select.exec(QStringLiteral("SELECT * FROM books ORDER BY id"))) {
            qWarning() << "importFromDatabase select failed:" << select.lastError().text();
            sourceDb.close();
            return 0;
        }

        while (select.next()) {
            Book book;
            book.filePath = queryValue(select, QStringLiteral("file_path"));
            if (book.filePath.trimmed().isEmpty())
                continue;

            book.fileHash = queryValue(select, QStringLiteral("file_hash"));
            book.format = queryValue(select, QStringLiteral("format"));
            book.fileSize = queryInt64Value(select, QStringLiteral("file_size"));
            book.title = queryValue(select, QStringLiteral("title"));
            book.author = queryValue(select, QStringLiteral("author"));
            book.publisher = queryValue(select, QStringLiteral("publisher"));
            book.isbn = queryValue(select, QStringLiteral("isbn"));
            book.language = queryValue(select, QStringLiteral("language"));
            book.year = queryIntValue(select, QStringLiteral("year"));
            book.pages = queryIntValue(select, QStringLiteral("pages"));
            book.rating = queryDoubleValue(select, QStringLiteral("rating"));
            book.description = queryValue(select, QStringLiteral("description"));
            book.tags = splitStoredList(queryValue(select, QStringLiteral("tags")));
            book.subjects = splitStoredList(queryValue(select, QStringLiteral("subjects")));
            book.coverPath = queryValue(select, QStringLiteral("cover_path"));
            book.hasCover = queryBoolValue(select, QStringLiteral("has_cover"));
            book.openCount = queryIntValue(select, QStringLiteral("open_count"));
            book.isFavourite = queryBoolValue(select, QStringLiteral("is_favourite"));
            book.notes = queryValue(select, QStringLiteral("notes"));
            book.readingStatus = queryValue(select, QStringLiteral("reading_status"));
            book.progressPercent = queryIntValue(select, QStringLiteral("progress_percent"));
            book.currentPage = queryIntValue(select, QStringLiteral("current_page"));
            book.readingMinutes = queryIntValue(select, QStringLiteral("reading_minutes"));
            book.loanedTo = queryValue(select, QStringLiteral("loaned_to"));
            book.series = queryValue(select, QStringLiteral("series"));
            book.seriesIndex = queryIntValue(select, QStringLiteral("series_index"));
            book.edition = queryValue(select, QStringLiteral("edition"));

            const QString dateAdded = queryValue(select, QStringLiteral("date_added"));
            if (!dateAdded.isEmpty())
                book.dateAdded = QDateTime::fromString(dateAdded, Qt::ISODate);
            const QString dateModified = queryValue(select, QStringLiteral("date_modified"));
            if (!dateModified.isEmpty())
                book.dateModified = QDateTime::fromString(dateModified, Qt::ISODate);
            const QString lastOpened = queryValue(select, QStringLiteral("last_opened"));
            if (!lastOpened.isEmpty())
                book.lastOpened = QDateTime::fromString(lastOpened, Qt::ISODate);
            const QString dateStarted = queryValue(select, QStringLiteral("date_started"));
            if (!dateStarted.isEmpty())
                book.dateStarted = QDateTime::fromString(dateStarted, Qt::ISODate);
            const QString dateFinished = queryValue(select, QStringLiteral("date_finished"));
            if (!dateFinished.isEmpty())
                book.dateFinished = QDateTime::fromString(dateFinished, Qt::ISODate);
            const QString lastReadingSession = queryValue(select, QStringLiteral("last_reading_session"));
            if (!lastReadingSession.isEmpty())
                book.lastReadingSession = QDateTime::fromString(lastReadingSession, Qt::ISODate);
            const QString loanDueDate = queryValue(select, QStringLiteral("loan_due_date"));
            if (!loanDueDate.isEmpty())
                book.loanDueDate = QDateTime::fromString(loanDueDate, Qt::ISODate);

            if (!upsertBook(book))
                continue;

            ++imported;
            qint64 newBookId = book.fileHash.isEmpty() ? 0 : bookIdByHash(book.fileHash);
            if (newBookId <= 0)
                newBookId = bookIdForPath(QSqlDatabase::database(QStringLiteral("eagle_lib")), book.filePath);
            const qint64 sourceBookId = queryInt64Value(select, QStringLiteral("id"));
            if (newBookId <= 0 || sourceBookId <= 0)
                continue;

            QSqlQuery locations(sourceDb);
            locations.prepare(QStringLiteral("SELECT file_path, file_hash, file_size, is_primary "
                                             "FROM book_files WHERE book_id=:id"));
            locations.bindValue(QStringLiteral(":id"), sourceBookId);
            if (!locations.exec())
                continue;

            while (locations.next()) {
                const QString location = locations.value(0).toString();
                if (location.trimmed().isEmpty())
                    continue;
                registerFileLocation(newBookId,
                                     location,
                                     locations.value(1).toString(),
                                     locations.value(2).toLongLong(),
                                     locations.value(3).toBool());
            }
        }

        sourceDb.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    return imported;
}

QStringList Database::fileLocationsForBook(qint64 bookId) const
{
    QSqlQuery q(QSqlDatabase::database("eagle_lib"));
    q.prepare("SELECT file_path FROM book_files WHERE book_id=:id ORDER BY is_primary DESC, file_path COLLATE NOCASE");
    q.bindValue(":id", bookId);
    q.exec();
    QStringList locations;
    while (q.next())
        locations << q.value(0).toString();
    return locations;
}
