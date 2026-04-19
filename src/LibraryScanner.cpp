// ============================================================
//  Eagle Library -- LibraryScanner.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
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
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtConcurrent/QtConcurrent>
#include <QFutureSynchronizer>

namespace {

struct FileHintMetadata {
    QString title;
    QString author;
    QString publisher;
    QString isbn;
    int     year = 0;

    bool hasAny() const
    {
        return !title.isEmpty() || !author.isEmpty() || !publisher.isEmpty() || !isbn.isEmpty() || year > 0;
    }
};

QString normaliseToken(QString value)
{
    value = value.trimmed();
    value.replace(QRegularExpression("\\s+"), " ");
    return value;
}

QString decodePdfHexLiteral(const QString& value)
{
    QByteArray bytes = QByteArray::fromHex(value.simplified().remove(' ').toLatin1());
    if (bytes.size() >= 2) {
        const uchar b0 = static_cast<uchar>(bytes.at(0));
        const uchar b1 = static_cast<uchar>(bytes.at(1));
        if (b0 == 0xFE && b1 == 0xFF) {
            QString out;
            for (int i = 2; i + 1 < bytes.size(); i += 2)
                out.append(QChar((static_cast<uchar>(bytes.at(i)) << 8) | static_cast<uchar>(bytes.at(i + 1))));
            return normaliseToken(out);
        }
        if (b0 == 0xFF && b1 == 0xFE) {
            QString out;
            for (int i = 2; i + 1 < bytes.size(); i += 2)
                out.append(QChar(static_cast<uchar>(bytes.at(i)) | (static_cast<uchar>(bytes.at(i + 1)) << 8)));
            return normaliseToken(out);
        }
    }

    QString out = QString::fromUtf8(bytes);
    if (out.contains(QChar::ReplacementCharacter))
        out = QString::fromLatin1(bytes);
    return normaliseToken(out);
}

QString decodePdfLiteral(QString value)
{
    value = value.trimmed();
    if (value.startsWith('<') && value.endsWith('>') && value.size() >= 2)
        return decodePdfHexLiteral(value.mid(1, value.size() - 2));

    if (value.startsWith('(') && value.endsWith(')') && value.size() >= 2)
        value = value.mid(1, value.size() - 2);
    value.replace("\\(", "(");
    value.replace("\\)", ")");
    value.replace("\\n", " ");
    value.replace("\\r", " ");
    value.replace("\\t", " ");
    value.replace("\\\\", "\\");

    QByteArray latin = value.toLatin1();
    if (latin.size() >= 2) {
        const uchar b0 = static_cast<uchar>(latin.at(0));
        const uchar b1 = static_cast<uchar>(latin.at(1));
        if ((b0 == 0xFE && b1 == 0xFF) || (b0 == 0xFF && b1 == 0xFE)) {
            QString out;
            if (b0 == 0xFE && b1 == 0xFF) {
                for (int i = 2; i + 1 < latin.size(); i += 2)
                    out.append(QChar((static_cast<uchar>(latin.at(i)) << 8) | static_cast<uchar>(latin.at(i + 1))));
            } else {
                for (int i = 2; i + 1 < latin.size(); i += 2)
                    out.append(QChar(static_cast<uchar>(latin.at(i)) | (static_cast<uchar>(latin.at(i + 1)) << 8)));
            }
            return normaliseToken(out);
        }
    }

    // Try to re-interpret the Latin-1 bytes as UTF-8 (common for modern PDFs without BOM)
    const QByteArray asUtf8Attempt = value.toLatin1();
    const QString utf8Try = QString::fromUtf8(asUtf8Attempt);
    if (!utf8Try.contains(QChar::ReplacementCharacter) && utf8Try != value)
        return normaliseToken(utf8Try);

    return normaliseToken(value);
}

bool looksLikeGarbageMetadata(const QString& value)
{
    if (value.isEmpty())
        return true;

    int controlCount = 0;
    for (const QChar ch : value) {
        if (ch.unicode() < 0x20 && !ch.isSpace())
            ++controlCount;
    }
    if (controlCount > 0)
        return true;

    const QString lowered = value.toLower();
    static const QStringList blockedTokens = {
        "tcpdf", "fpdf", "dompdf", "wkhtmltopdf", "acrobat distiller",
        "adobe acrobat", "ghostscript", "libreoffice", "microsoft word",
        "pdf generator", "www.tcpdf.org"
    };
    for (const QString& token : blockedTokens) {
        if (lowered.contains(token))
            return true;
    }
    return false;
}

QString cleanPdfMetadataValue(const QString& raw)
{
    QString value = normaliseToken(raw);
    value.remove(QRegularExpression("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F]"));
    value = normaliseToken(value);
    return looksLikeGarbageMetadata(value) ? QString() : value;
}

QString extractPdfField(const QString& content, const QString& field)
{
    QRegularExpression re(QString("/%1\\s*(\\((?:\\\\.|[^\\)]){1,512}\\)|<([0-9A-Fa-f\\s]{2,1024})>)")
                              .arg(QRegularExpression::escape(field)),
                          QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(content);
    return match.hasMatch() ? cleanPdfMetadataValue(decodePdfLiteral(match.captured(1))) : QString();
}

int extractYearToken(const QString& text)
{
    QRegularExpression re("(19|20)\\d{2}");
    const QRegularExpressionMatch match = re.match(text);
    return match.hasMatch() ? match.captured(0).toInt() : 0;
}

QString sanitizeIsbn(const QString& value)
{
    QString out;
    for (const QChar ch : value) {
        if (ch.isDigit() || ch.toUpper() == 'X')
            out.append(ch.toUpper());
    }
    return out;
}

bool isValidIsbn10(const QString& isbn)
{
    if (isbn.size() != 10)
        return false;
    int sum = 0;
    for (int i = 0; i < 9; ++i) {
        if (!isbn.at(i).isDigit())
            return false;
        sum += (10 - i) * isbn.at(i).digitValue();
    }
    const QChar last = isbn.at(9).toUpper();
    sum += (last == 'X') ? 10 : (last.isDigit() ? last.digitValue() : -1000);
    return sum % 11 == 0;
}

bool isValidIsbn13(const QString& isbn)
{
    if (isbn.size() != 13)
        return false;
    int sum = 0;
    for (int i = 0; i < 12; ++i) {
        if (!isbn.at(i).isDigit())
            return false;
        sum += isbn.at(i).digitValue() * ((i % 2 == 0) ? 1 : 3);
    }
    if (!isbn.at(12).isDigit())
        return false;
    const int check = (10 - (sum % 10)) % 10;
    return isbn.at(12).digitValue() == check;
}

QString extractIsbnFromText(const QString& text)
{
    static const QRegularExpression re(QStringLiteral(R"((?:97[89][-\s]?)?\d[-\s\d]{8,20}[\dXx])"));
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    while (it.hasNext()) {
        const QString isbn = sanitizeIsbn(it.next().captured(0));
        if (isbn.size() == 13 && isValidIsbn13(isbn))
            return isbn;
        if (isbn.size() == 10 && isValidIsbn10(isbn))
            return isbn;
    }
    return {};
}

QString readBookProbeText(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QByteArray bytes = file.read(1024 * 1024);
    if (file.size() > 1024 * 1024 && file.seek(qMax<qint64>(0, file.size() - 256 * 1024)))
        bytes += '\n' + file.read(256 * 1024);

    QString text = QString::fromUtf8(bytes);
    if (text.contains(QChar::ReplacementCharacter))
        text = QString::fromLatin1(bytes);
    return text;
}

QString runCommandCapture(const QString& program, const QStringList& arguments, int timeoutMs = 20000)
{
    QProcess proc;
    proc.start(program, arguments);
    if (!proc.waitForStarted(3000))
        return {};
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(1000);
        return {};
    }

    QByteArray bytes = proc.readAllStandardOutput();
    if (bytes.isEmpty())
        bytes = proc.readAllStandardError();
    QString text = QString::fromUtf8(bytes);
    if (text.contains(QChar::ReplacementCharacter))
        text = QString::fromLocal8Bit(bytes);
    return normaliseToken(text);
}

QString extractTextWithPdftotext(const QString& filePath)
{
    const QString tool = QStandardPaths::findExecutable("pdftotext");
    if (tool.isEmpty())
        return {};

    QTemporaryDir tempDir;
    if (!tempDir.isValid())
        return {};

    const QString outputPath = tempDir.filePath("probe.txt");
    QProcess proc;
    proc.start(tool, QStringList() << "-enc" << "UTF-8" << "-f" << "1" << "-l" << "5" << filePath << outputPath);
    if (!proc.waitForStarted(3000))
        return {};
    if (!proc.waitForFinished(25000)) {
        proc.kill();
        proc.waitForFinished(1000);
        return {};
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::ReadOnly))
        return {};
    const QByteArray bytes = outFile.readAll();
    QString text = QString::fromUtf8(bytes);
    if (text.contains(QChar::ReplacementCharacter))
        text = QString::fromLatin1(bytes);
    return normaliseToken(text);
}

QString ocrPdfWithExternalTools(const QString& filePath)
{
    const QString pdftoppm = QStandardPaths::findExecutable("pdftoppm");
    const QString tesseract = QStandardPaths::findExecutable("tesseract");
    if (pdftoppm.isEmpty() || tesseract.isEmpty())
        return {};

    QTemporaryDir tempDir;
    if (!tempDir.isValid())
        return {};

    const QString prefix = tempDir.filePath("page");
    QProcess renderProc;
    renderProc.start(pdftoppm, QStringList() << "-png" << "-f" << "1" << "-l" << "2" << filePath << prefix);
    if (!renderProc.waitForStarted(3000))
        return {};
    if (!renderProc.waitForFinished(30000)) {
        renderProc.kill();
        renderProc.waitForFinished(1000);
        return {};
    }

    QString combined;
    const QStringList images = QDir(tempDir.path()).entryList(QStringList() << "page-*.png", QDir::Files, QDir::Name);
    for (const QString& imageName : images) {
        const QString text = runCommandCapture(tesseract, QStringList() << tempDir.filePath(imageName) << "stdout" << "-l" << "eng", 25000);
        if (!text.isEmpty()) {
            if (!combined.isEmpty())
                combined += '\n';
            combined += text;
        }
    }
    return normaliseToken(combined);
}

QString bestContentTitleCandidate(const QString& text)
{
    if (text.isEmpty())
        return {};

    QStringList lines = text.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
    QString best;
    int bestScore = -1000;

    static const QStringList blockedTokens = {
        "abstract", "introduction", "keywords", "references", "contents",
        "table of contents", "copyright", "all rights reserved", "doi",
        "arxiv", "www.", "http", "journal", "conference", "proceedings",
        "invoice", "report", "manual", "specification"
    };

    for (int i = 0; i < qMin(lines.size(), 24); ++i) {
        QString line = normaliseToken(lines.at(i));
        if (line.size() < 10 || line.size() > 160)
            continue;
        if (line.contains(QRegularExpression("^\\d+$")))
            continue;
        if (line.count(QRegularExpression("[\\p{L}]")) < 6)
            continue;

        const QString lowered = line.toLower();
        bool blocked = false;
        for (const QString& token : blockedTokens) {
            if (lowered.contains(token)) {
                blocked = true;
                break;
            }
        }
        if (blocked)
            continue;

        int score = 0;
        if (i == 0) score += 14;
        else if (i < 4) score += 10;
        else score += 4;
        if (line.size() >= 20 && line.size() <= 120) score += 8;
        if (!line.endsWith('.') && !line.endsWith(':')) score += 3;
        if (line.count(QRegularExpression("[A-Z]")) >= 3) score += 2;
        if (line.count(QRegularExpression("[,;]")) == 0) score += 2;
        if (lowered.contains(" by ")) score -= 4;
        if (lowered.contains("vol.") || lowered.contains("issue ")) score -= 4;

        if (score > bestScore) {
            bestScore = score;
            best = line;
        }
    }

    return bestScore >= 10 ? best : QString();
}

FileHintMetadata extractFileHints(const Book& book)
{
    FileHintMetadata hint;
    if (book.format.compare("PDF", Qt::CaseInsensitive) == 0) {
        QFile file(book.filePath);
        if (!file.open(QIODevice::ReadOnly))
            return hint;

        QByteArray raw = file.read(512 * 1024);
        if (file.size() > 512 * 1024 && file.seek(qMax<qint64>(0, file.size() - 256 * 1024)))
            raw += file.read(256 * 1024);

        const QString text = QString::fromLatin1(raw);
        hint.title = extractPdfField(text, "Title");
        hint.author = extractPdfField(text, "Author");
        hint.publisher = extractPdfField(text, "Creator");
        if (hint.publisher.isEmpty())
            hint.publisher = extractPdfField(text, "Publisher");
        hint.year = extractYearToken(extractPdfField(text, "CreationDate"));
        if (hint.year == 0)
            hint.year = extractYearToken(extractPdfField(text, "ModDate"));
        hint.isbn = sanitizeIsbn(extractPdfField(text, "ISBN"));

        const QString extractedText = extractTextWithPdftotext(book.filePath);
        if (hint.title.isEmpty())
            hint.title = bestContentTitleCandidate(extractedText);
        if (hint.isbn.isEmpty())
            hint.isbn = extractIsbnFromText(extractedText);

        if ((hint.title.isEmpty() || hint.isbn.isEmpty())) {
            const QString ocrText = ocrPdfWithExternalTools(book.filePath);
            if (hint.title.isEmpty())
                hint.title = bestContentTitleCandidate(ocrText);
            if (hint.isbn.isEmpty())
                hint.isbn = extractIsbnFromText(ocrText);
        }

        hint.title = cleanPdfMetadataValue(hint.title);
        hint.author = cleanPdfMetadataValue(hint.author);
        hint.publisher = cleanPdfMetadataValue(hint.publisher);
    } else {
        const QString probeText = readBookProbeText(book.filePath);
        hint.isbn = extractIsbnFromText(probeText);
    }
    return hint;
}

QString renameDetail(const Book& book, const FileHintMetadata& hint)
{
    QStringList parts;
    if (hint.hasAny())
        parts << "embedded metadata";
    if (book.format.compare("PDF", Qt::CaseInsensitive) == 0)
        parts << "content title probe";
    if (!QFileInfo(book.filePath).suffix().isEmpty())
        parts << book.format;
    if (parts.isEmpty())
        return "filename analysis";
    return parts.join(" + ");
}

} // namespace

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

    if (m_fastScanMode)
        return b;

    const FileHintMetadata hints = extractFileHints(b);
    if (!hints.title.isEmpty())
        b.title = hints.title;
    if (!hints.author.isEmpty())
        b.author = hints.author;
    if (!hints.publisher.isEmpty())
        b.publisher = hints.publisher;
    if (!hints.isbn.isEmpty())
        b.isbn = sanitizeIsbn(hints.isbn);
    if (hints.year > 0)
        b.year = hints.year;

    return b;
}

// ── ScanWorker constructor ────────────────────────────────────
ScanWorker::ScanWorker(const QStringList& folders, int parallelism, bool fastScanMode, QObject* parent)
    : QObject(parent)
    , m_folders(folders)
    , m_parallelism(parallelism > 0 ? parallelism : QThread::idealThreadCount())
    , m_fastScanMode(fastScanMode)
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
            QSqlDatabase db = QSqlDatabase::database(conn);
            bool transactionOpen = db.transaction();
            int pendingWrites = 0;

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
                    ++pendingWrites;
                    if (transactionOpen && pendingWrites >= 48) {
                        db.commit();
                        transactionOpen = db.transaction();
                        pendingWrites = 0;
                    }
                    QMutexLocker lk(&emitMutex);
                    emit bookFound(b);
                } else {
                    m_skipped.fetchAndAddRelaxed(1);
                }
            }

            if (transactionOpen)
                db.commit();
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

void LibraryScanner::startScan(const QStringList& folders, int parallelism, bool fastScanMode)
{
    if (m_thread && m_thread->isRunning()) {
        cancel();
        m_thread->wait(3000);
    }

    m_thread = new QThread(this);
    m_worker = new ScanWorker(folders, parallelism, fastScanMode);
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

QString SmartRenamer::inferTitleFromContent(const Book& book)
{
    auto acceptCandidate = [&book](const QString& candidate) -> QString {
        const QString cleaned = cleanToken(candidate);
        return (!cleaned.isEmpty() && !isWeakTitle(cleaned, book.filePath)) ? cleaned : QString();
    };

    if (!book.title.trimmed().isEmpty() && !isWeakTitle(book.title, book.filePath))
        return cleanToken(book.title);

    const QString directCandidate = acceptCandidate(bestContentTitleCandidate(readBookProbeText(book.filePath)));
    if (!directCandidate.isEmpty())
        return directCandidate;

    if (book.format.compare("PDF", Qt::CaseInsensitive) == 0) {
        const QString pdftotextCandidate = acceptCandidate(bestContentTitleCandidate(extractTextWithPdftotext(book.filePath)));
        if (!pdftotextCandidate.isEmpty())
            return pdftotextCandidate;

        const QString ocrCandidate = acceptCandidate(bestContentTitleCandidate(ocrPdfWithExternalTools(book.filePath)));
        if (!ocrCandidate.isEmpty())
            return ocrCandidate;
    }

    return {};
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
    const FileHintMetadata hints = extractFileHints(book);

    // If already has good metadata, preserve author/publisher/year
    // but still try to improve the title if it's weak
    bool weakTitle = isWeakTitle(book.title, book.filePath);

    if (!weakTitle && !book.author.isEmpty() && !hints.hasAny()) {
        // Nothing to do — metadata looks fine
        r.newTitle     = book.title;
        r.newAuthor    = book.author;
        r.newPublisher = book.publisher;
        r.newYear      = book.year;
        r.changed      = false;
        return r;
    }

    const QString contentTitle = inferTitleFromContent(book);
    const QString hintedTitle = !hints.title.isEmpty() ? cleanToken(hints.title) : QString();

    // Parse filename
    RenameResult parsed = parseFilename(book);

    // Merge: preserve existing good fields, fill in missing ones
    QString chosenTitle = book.title;
    if (weakTitle) {
        if (!contentTitle.isEmpty())
            chosenTitle = contentTitle;
        else if (!hintedTitle.isEmpty())
            chosenTitle = hintedTitle;
        else
            chosenTitle = parsed.newTitle;
    } else if (!contentTitle.isEmpty() && contentTitle.length() > chosenTitle.length() + 6
               && isWeakTitle(chosenTitle, book.filePath)) {
        chosenTitle = contentTitle;
    }

    r.newTitle     = chosenTitle;
    r.newAuthor    = book.author.isEmpty()
        ? (!hints.author.isEmpty() ? cleanToken(hints.author) : parsed.newAuthor)
        : book.author;
    r.newPublisher = book.publisher.isEmpty() ? cleanToken(hints.publisher) : book.publisher;
    r.newYear      = book.year > 0 ? book.year : (hints.year > 0 ? hints.year : parsed.newYear);
    r.changed      = (r.newTitle  != book.title  ||
                      r.newAuthor != book.author  ||
                      r.newPublisher != book.publisher ||
                      r.newYear   != book.year);
    return r;
}

void SmartRenamer::renameAll(const QVector<Book>& books)
{
    m_cancelled.storeRelaxed(0);
    const int total = books.size();
    int changed = 0;

    for (int i = 0; i < total; ++i) {
        if (m_cancelled.loadRelaxed()) break;
        const FileHintMetadata hints = extractFileHints(books[i]);
        emit progress(i + 1, total, QFileInfo(books[i].filePath).fileName(), renameDetail(books[i], hints));

        RenameResult r = analyseBook(books[i]);
        if (r.changed) {
            ++changed;
            emit renamed(r);
        }
    }

    emit progress(total, total, {}, "done");
    emit finished(changed);
}
