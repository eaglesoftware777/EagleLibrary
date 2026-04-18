// ============================================================
//  Eagle Library -- LibraryScanner.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include "LibraryScanner.h"
#include "AppConfig.h"
#include <QDirIterator>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QThreadPool>
#include <QRunnable>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QtConcurrent/QtConcurrent>
#include <QFutureSynchronizer>

// ── DB helpers ────────────────────────────────────────────────
bool ScanWorker::openThreadDb(const QString& conn) const
{
    if (QSqlDatabase::contains(conn)) return true;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conn);
    db.setDatabaseName(AppConfig::dbPath());
    if (!db.open()) return false;
    QSqlQuery q(db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA synchronous=NORMAL");
    q.exec("PRAGMA cache_size=5000");
    q.exec("PRAGMA temp_store=MEMORY");
    return true;
}

bool ScanWorker::dbBookExists(const QString& path, const QString& conn) const
{
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("SELECT 1 FROM books WHERE file_path=:fp LIMIT 1");
    q.bindValue(":fp", path);
    q.exec();
    return q.next();
}

bool ScanWorker::dbHashExists(const QString& hash, const QString& conn) const
{
    if (hash.isEmpty()) return false;
    QSqlQuery q(QSqlDatabase::database(conn));
    q.prepare("SELECT 1 FROM books WHERE file_hash=:h LIMIT 1");
    q.bindValue(":h", hash);
    q.exec();
    return q.next();
}

qint64 ScanWorker::dbInsertBook(const Book& b, const QString& conn) const
{
    QSqlQuery q(QSqlDatabase::database(conn));
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
    q.bindValue(":lo",   QString());
    q.bindValue(":oc",   0);
    q.bindValue(":fav",  0);
    q.bindValue(":notes",QString());
    if (!q.exec()) return -1;
    return q.lastInsertId().toLongLong();
}

// ── Build book from filesystem ────────────────────────────────
QString ScanWorker::computeHash(const QString& filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash h(QCryptographicHash::Sha1); // SHA-1 faster than SHA-256, enough for dedup
    h.addData(f.read(65536));
    return h.result().toHex();
}

QString ScanWorker::detectFormat(const QString& ext) const
{
    static const QHash<QString,QString> m = {
        {"pdf","PDF"},{"epub","EPUB"},{"mobi","MOBI"},
        {"azw","AZW"},{"azw3","AZW3"},{"djvu","DjVu"},
        {"fb2","FB2"},{"cbz","CBZ"},{"cbr","CBR"},
        {"txt","TXT"},{"rtf","RTF"},{"doc","DOC"},
        {"docx","DOCX"},{"chm","CHM"},{"lit","LIT"},
    };
    return m.value(ext, ext.toUpper());
}

Book ScanWorker::buildBook(const QString& filePath) const
{
    QFileInfo fi(filePath);
    Book b;
    b.filePath     = filePath;
    b.fileHash     = computeHash(filePath);
    b.format       = detectFormat(fi.suffix().toLower());
    b.fileSize     = fi.size();
    b.title        = fi.completeBaseName();
    b.dateAdded    = QDateTime::currentDateTime();
    b.dateModified = fi.lastModified();
    return b;
}

// ── ScanWorker constructor ────────────────────────────────────
ScanWorker::ScanWorker(const QStringList& folders, int parallelism, QObject* parent)
    : QObject(parent)
    , m_folders(folders)
    , m_parallelism(parallelism > 0 ? parallelism : QThread::idealThreadCount())
{}

// ── Main scan loop ────────────────────────────────────────────
void ScanWorker::run()
{
    // ── Phase 1: parallel directory walk ─────────────────────
    // Use multiple DirIterators concurrently for speed on large trees
    QStringList allFiles;
    QMutex fileMutex;

    // Collect top-level subdirectories per root folder
    QStringList scanRoots;
    for (const QString& root : m_folders) {
        if (m_cancelled.loadRelaxed()) break;
        scanRoots << root;
        // Add immediate subdirs so we can parallelise the walk
        QDir d(root);
        for (const auto& sub : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable))
            scanRoots << root + "/" + sub;
    }
    scanRoots.removeDuplicates();

    // Walk each root in a thread pool
    QVector<QFuture<void>> futures;
    for (const QString& root : scanRoots) {
        if (m_cancelled.loadRelaxed()) break;
        QFuture<void> f = QtConcurrent::run([&, root]() {
            if (m_cancelled.loadRelaxed()) return;
            QDirIterator it(root,
                            AppConfig::supportedFormats(),
                            QDir::Files | QDir::Readable,
                            QDirIterator::Subdirectories);
            QStringList local;
            while (it.hasNext() && !m_cancelled.loadRelaxed())
                local << it.next();
            QMutexLocker lk(&fileMutex);
            allFiles << local;
        });
        futures.append(f);
    }
    for (auto& f : futures) f.waitForFinished();
    allFiles.removeDuplicates();

    const int total = allFiles.size();
    emit progress(0, total, QString("Found %1 files...").arg(total));

    if (total == 0 || m_cancelled.loadRelaxed()) {
        emit finished(0, 0);
        return;
    }

    // ── Phase 2: parallel hash + insert ─────────────────────
    // Each thread gets its own DB connection
    QMutex emitMutex;
    const int chunkSize = qMax(1, total / m_parallelism);

    QVector<QFuture<void>> workFutures;
    for (int t = 0; t < m_parallelism; ++t) {
        int from = t * chunkSize;
        int to   = (t == m_parallelism - 1) ? total : from + chunkSize;
        if (from >= total) break;

        QFuture<void> f = QtConcurrent::run([&, t, from, to]() {
            const QString conn = QString("scan_%1").arg(t);
            if (!openThreadDb(conn)) return;

            for (int i = from; i < to && !m_cancelled.loadRelaxed(); ++i) {
                const QString& path = allFiles[i];

                int d = m_done.fetchAndAddRelaxed(1) + 1;
                if (d % 50 == 0 || d == total) {
                    QMutexLocker lk(&emitMutex);
                    emit progress(d, total, QFileInfo(path).fileName());
                }

                if (dbBookExists(path, conn)) { m_skipped.fetchAndAddRelaxed(1); continue; }

                Book b = buildBook(path);
                if (dbHashExists(b.fileHash, conn)) { m_skipped.fetchAndAddRelaxed(1); continue; }

                qint64 id = dbInsertBook(b, conn);
                if (id > 0) {
                    b.id = id;
                    m_added.fetchAndAddRelaxed(1);
                    QMutexLocker lk(&emitMutex);
                    emit bookFound(b);
                } else {
                    m_skipped.fetchAndAddRelaxed(1);
                }
            }

            QSqlDatabase::database(conn).close();
            QSqlDatabase::removeDatabase(conn);
        });
        workFutures.append(f);
    }

    for (auto& f : workFutures) f.waitForFinished();

    emit progress(total, total, {});
    emit finished(m_added.loadRelaxed(), m_skipped.loadRelaxed());
}

// ── LibraryScanner (controller) ───────────────────────────────
LibraryScanner::LibraryScanner(QObject* parent) : QObject(parent) {}

LibraryScanner::~LibraryScanner() { cancel(); }

void LibraryScanner::startScan(const QStringList& folders, int parallelism)
{
    if (m_thread && m_thread->isRunning()) {
        cancel();
        m_thread->wait(3000);
    }

    m_thread = new QThread(this);
    m_worker = new ScanWorker(folders, parallelism);
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started,    m_worker, &ScanWorker::run);
    connect(m_worker, &ScanWorker::finished,m_thread, &QThread::quit);
    connect(m_worker, &ScanWorker::bookFound, this,   &LibraryScanner::bookFound);
    connect(m_worker, &ScanWorker::progress,  this,   &LibraryScanner::progress);
    connect(m_worker, &ScanWorker::finished,  this,   &LibraryScanner::scanFinished);
    connect(m_worker, &ScanWorker::error,     this,   &LibraryScanner::scanError);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);

    m_thread->start(QThread::LowPriority);
}

void LibraryScanner::cancel()
{
    if (m_worker) m_worker->cancel();
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait(5000);
    }
}

bool LibraryScanner::isRunning() const
{
    return m_thread && m_thread->isRunning();
}

// ═════════════════════════════════════════════════════════════
//  SmartRenamer
// ═════════════════════════════════════════════════════════════
SmartRenamer::SmartRenamer(QObject* parent) : QObject(parent) {}

// Check if a title is just the raw filename (no useful info)
bool SmartRenamer::isWeakTitle(const QString& title, const QString& filePath)
{
    if (title.isEmpty()) return true;
    QString stem = QFileInfo(filePath).completeBaseName();
    // Same as stem -> definitely just filename
    if (title.compare(stem, Qt::CaseInsensitive) == 0) return true;
    // Very short or all digits
    if (title.length() < 4) return true;
    // Looks like "document_001" or "scan0042"
    static QRegularExpression re("^(scan|doc|file|book|untitled|unknown|page|copy)[\\s_\\-]?\\d*$",
                                  QRegularExpression::CaseInsensitiveOption);
    if (re.match(title).hasMatch()) return true;
    return false;
}

QString SmartRenamer::cleanToken(const QString& s)
{
    // Remove leading/trailing junk, normalise spaces
    QString r = s.trimmed();
    // Remove repeated spaces, underscores, dashes
    r.replace(QRegularExpression("[_]+"), " ");
    r.replace(QRegularExpression("\\s{2,}"), " ");
    // Capitalise first letter of each word (title case)
    QStringList words = r.split(' ', Qt::SkipEmptyParts);
    static const QSet<QString> smallWords = {
        "a","an","the","and","but","or","for","nor","on","at",
        "to","by","in","of","up","as","is","it"
    };
    for (int i = 0; i < words.size(); ++i) {
        QString w = words[i].toLower();
        if (i == 0 || !smallWords.contains(w))
            words[i] = w[0].toUpper() + w.mid(1);
        else
            words[i] = w;
    }
    return words.join(' ');
}

RenameResult SmartRenamer::parseFilename(const Book& book)
{
    RenameResult r;
    r.bookId   = book.id;
    r.oldTitle = book.title;

    QString stem = QFileInfo(book.filePath).completeBaseName();

    // ── Pattern 1: "Author - Title (Year)" or "Author - Title" ──
    // e.g. "Stephen King - The Shining (1977)"
    {
        static QRegularExpression re(
            R"(^(.+?)\s*[-–—]\s*(.+?)(?:\s*\((\d{4})\))?\s*$)");
        auto m = re.match(stem);
        if (m.hasMatch()) {
            r.newAuthor = cleanToken(m.captured(1));
            r.newTitle  = cleanToken(m.captured(2));
            r.newYear   = m.captured(3).toInt();
            r.changed   = true;
            return r;
        }
    }

    // ── Pattern 2: "Title - Author" (reversed) ─────────────────
    // detect if second part looks like a name (FirstName LastName)
    {
        static QRegularExpression re(
            R"(^(.+?)\s*[-–—]\s*([A-Z][a-z]+\s+[A-Z][a-z]+)\s*$)");
        auto m = re.match(stem);
        if (m.hasMatch()) {
            r.newTitle  = cleanToken(m.captured(1));
            r.newAuthor = cleanToken(m.captured(2));
            r.changed   = true;
            return r;
        }
    }

    // ── Pattern 3: "Title (Year)" ──────────────────────────────
    {
        static QRegularExpression re(R"(^(.+?)\s*\((\d{4})\)\s*$)");
        auto m = re.match(stem);
        if (m.hasMatch()) {
            r.newTitle = cleanToken(m.captured(1));
            r.newYear  = m.captured(2).toInt();
            r.changed  = true;
            return r;
        }
    }

    // ── Pattern 4: "Title_v2" or "Title [EN]" ──────────────────
    {
        static QRegularExpression re(R"(^(.+?)[\s_](?:v\d|en|fr|de|\[\w+\])\s*$)",
                                      QRegularExpression::CaseInsensitiveOption);
        auto m = re.match(stem);
        if (m.hasMatch()) {
            r.newTitle = cleanToken(m.captured(1));
            r.changed  = true;
            return r;
        }
    }

    // ── Fallback: just clean the stem ──────────────────────────
    r.newTitle = cleanToken(stem);
    // Only mark changed if it actually differs
    r.changed = (r.newTitle != book.title);
    return r;
}

RenameResult SmartRenamer::analyseBook(const Book& book)
{
    RenameResult r;
    r.bookId   = book.id;
    r.oldTitle = book.title;

    // If already has good metadata, preserve author/publisher/year
    // but still try to improve the title if it's weak
    bool weakTitle = isWeakTitle(book.title, book.filePath);

    if (!weakTitle && !book.author.isEmpty()) {
        // Nothing to do — metadata looks fine
        r.newTitle     = book.title;
        r.newAuthor    = book.author;
        r.newPublisher = book.publisher;
        r.newYear      = book.year;
        r.changed      = false;
        return r;
    }

    // Parse filename
    RenameResult parsed = parseFilename(book);

    // Merge: preserve existing good fields, fill in missing ones
    r.newTitle     = weakTitle ? parsed.newTitle : book.title;
    r.newAuthor    = book.author.isEmpty() ? parsed.newAuthor : book.author;
    r.newPublisher = book.publisher;
    r.newYear      = (book.year == 0 && parsed.newYear > 0) ? parsed.newYear : book.year;
    r.changed      = (r.newTitle  != book.title  ||
                      r.newAuthor != book.author  ||
                      r.newYear   != book.year);
    return r;
}

void SmartRenamer::renameAll(const QVector<Book>& books)
{
    const int total = books.size();
    int changed = 0;

    for (int i = 0; i < total; ++i) {
        if (m_cancelled.loadRelaxed()) break;
        emit progress(i, total);

        RenameResult r = analyseBook(books[i]);
        if (r.changed) {
            ++changed;
            emit renamed(r);
        }
    }

    emit progress(total, total);
    emit finished(changed);
}
