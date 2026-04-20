// ============================================================
//  Eagle Library -- MetadataFetcher.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "MetadataFetcher.h"
#include "AppConfig.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <algorithm>

namespace {

struct EmbeddedMetadata {
    Book book;

    bool hasAny() const
    {
        return !book.title.isEmpty() || !book.author.isEmpty() || !book.publisher.isEmpty()
            || book.year > 0 || !book.language.isEmpty();
    }
};

struct Candidate {
    Book        book;
    QStringList coverUrls;
    int         score = 0;
};

QString normaliseSpaces(QString value)
{
    value.replace(QRegularExpression("\\s+"), " ");
    return value.trimmed();
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
            return normaliseSpaces(out);
        }
        if (b0 == 0xFF && b1 == 0xFE) {
            QString out;
            for (int i = 2; i + 1 < bytes.size(); i += 2)
                out.append(QChar(static_cast<uchar>(bytes.at(i)) | (static_cast<uchar>(bytes.at(i + 1)) << 8)));
            return normaliseSpaces(out);
        }
    }

    QString out = QString::fromUtf8(bytes);
    if (out.contains(QChar::ReplacementCharacter))
        out = QString::fromLatin1(bytes);
    return normaliseSpaces(out);
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
            return normaliseSpaces(out);
        }
    }

    return normaliseSpaces(value);
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
    QString value = normaliseSpaces(raw);
    value.remove(QRegularExpression("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F]"));
    value = normaliseSpaces(value);
    return looksLikeGarbageMetadata(value) ? QString() : value;
}

QString extractPdfField(const QString& content, const QString& field)
{
    QRegularExpression re(QString("/%1\\s*(\\((?:\\\\.|[^\\)]){1,512}\\)|<([0-9A-Fa-f\\s]{2,1024})>)")
                              .arg(QRegularExpression::escape(field)),
                          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(content);
    if (match.hasMatch())
        return cleanPdfMetadataValue(decodePdfLiteral(match.captured(1)));
    return {};
}

int extractYear(const QString& text)
{
    QRegularExpression re("(19|20)\\d{2}");
    QRegularExpressionMatch match = re.match(text);
    return match.hasMatch() ? match.captured(0).toInt() : 0;
}

EmbeddedMetadata extractEmbeddedMetadata(const FetchRequest& req)
{
    EmbeddedMetadata meta;
    QFileInfo fi(req.filePath);
    if (!fi.exists() || req.format.compare("PDF", Qt::CaseInsensitive) != 0)
        return meta;

    QFile file(req.filePath);
    if (!file.open(QIODevice::ReadOnly))
        return meta;

    QByteArray raw = file.read(512 * 1024);
    if (file.size() > 512 * 1024 && file.seek(qMax<qint64>(0, file.size() - 256 * 1024)))
        raw += file.read(256 * 1024);

    QString text = QString::fromLatin1(raw);
    meta.book.title = extractPdfField(text, "Title");
    meta.book.author = extractPdfField(text, "Author");
    meta.book.publisher = extractPdfField(text, "Creator");
    if (meta.book.publisher.isEmpty())
        meta.book.publisher = extractPdfField(text, "Publisher");
    meta.book.language = extractPdfField(text, "Lang");

    QString dateText = extractPdfField(text, "CreationDate");
    if (dateText.isEmpty())
        dateText = extractPdfField(text, "ModDate");
    meta.book.year = extractYear(dateText);

    return meta;
}

QStringList uniqueUrls(QStringList urls)
{
    QStringList out;
    QSet<QString> seen;
    for (QString& url : urls) {
        if (url.isEmpty())
            continue;
        url.replace("http://", "https://");
        if (!seen.contains(url)) {
            seen.insert(url);
            out << url;
        }
    }
    return out;
}

int candidateScore(const Book& book, const FetchRequest& req)
{
    auto norm = [](QString s) {
        return s.toLower().remove(QRegularExpression("[^a-z0-9]+"));
    };

    int score = 0;
    if (!book.title.isEmpty())       score += 8;
    if (!book.author.isEmpty())      score += 8;
    if (!book.publisher.isEmpty())   score += 4;
    if (!book.isbn.isEmpty())        score += 8;
    if (book.year > 0)               score += 4;
    if (book.pages > 0)              score += 2;
    if (!book.description.isEmpty()) score += 2;
    if (!book.language.isEmpty())    score += 1;
    if (!book.subjects.isEmpty())    score += 2;
    if (book.rating > 0)             score += 1;

    if (!req.isbn.isEmpty() && !book.isbn.isEmpty() && norm(req.isbn) == norm(book.isbn))
        score += 30;

    const QString reqTitle = norm(req.title);
    const QString bookTitle = norm(book.title);
    if (!reqTitle.isEmpty() && !bookTitle.isEmpty()) {
        if (reqTitle == bookTitle) score += 20;
        else if (bookTitle.contains(reqTitle) || reqTitle.contains(bookTitle)) score += 12;
    }

    const QString reqAuthor = norm(req.author);
    const QString bookAuthor = norm(book.author);
    if (!reqAuthor.isEmpty() && !bookAuthor.isEmpty()) {
        if (reqAuthor == bookAuthor) score += 12;
        else if (bookAuthor.contains(reqAuthor) || reqAuthor.contains(bookAuthor)) score += 6;
    }

    return score;
}

Book mergeBooks(QVector<Candidate> candidates)
{
    if (candidates.isEmpty())
        return {};

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.score > b.score;
    });

    Book merged = candidates.first().book;
    for (int i = 1; i < candidates.size(); ++i) {
        const Book& src = candidates[i].book;
        if (merged.title.isEmpty())       merged.title = src.title;
        if (merged.author.isEmpty())      merged.author = src.author;
        if (merged.publisher.isEmpty())   merged.publisher = src.publisher;
        if (merged.isbn.isEmpty())        merged.isbn = src.isbn;
        if (merged.language.isEmpty())    merged.language = src.language;
        if (merged.year == 0)             merged.year = src.year;
        if (merged.pages == 0)            merged.pages = src.pages;
        if (merged.rating == 0)           merged.rating = src.rating;
        if (merged.description.isEmpty()) merged.description = src.description;
        if (merged.subjects.isEmpty())    merged.subjects = src.subjects;
    }
    return merged;
}

} // namespace

MetadataFetcher::MetadataFetcher(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_processTimer(new QTimer(this))
{
    m_nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_processTimer->setInterval(100);
    m_processTimer->setSingleShot(false);
    connect(m_processTimer, &QTimer::timeout, this, &MetadataFetcher::processQueue);
    m_processTimer->start();
}

void MetadataFetcher::enqueue(const FetchRequest& req)
{
    m_cancelling = false;
    m_queue.enqueue(req);
    ++m_totalQueued;
    qWarning().noquote()
        << "[Metadata] Queue bookId=" << req.bookId
        << "title=" << req.title
        << "author=" << req.author
        << "isbn=" << req.isbn
        << "path=" << req.filePath
        << "format=" << req.format;
    emit fetchProgress(m_totalDone, m_totalQueued, QFileInfo(req.filePath).fileName(), "Queued");
}

void MetadataFetcher::cancelAll()
{
    m_cancelling = true;
    ++m_generation;
    m_processTimer->stop();
    m_queue.clear();

    const auto activeReplies = m_activeReplies;
    for (QNetworkReply* reply : activeReplies) {
        if (reply)
            reply->abort();
    }
    m_activeReplies.clear();

    m_requests.clear();
    m_candidates.clear();
    m_coverCandidates.clear();
    m_pendingReplies.clear();
    m_active = 0;
    m_totalQueued = 0;
    m_totalDone = 0;
    m_processTimer->start();
}

void MetadataFetcher::processQueue()
{
    if (m_cancelling)
        return;
    while (!m_queue.isEmpty() && m_active < m_maxConcurrent) {
        ++m_active;
        fetchFromSources(m_queue.dequeue());
    }
}

QString MetadataFetcher::buildGoogleQuery(const FetchRequest& req) const
{
    if (!req.isbn.isEmpty())
        return QString("isbn:%1").arg(req.isbn);

    QString q = req.title;
    if (!req.author.isEmpty())
        q += QString(" inauthor:%1").arg(req.author);
    return q.trimmed();
}

QString MetadataFetcher::buildOpenLibraryQuery(const FetchRequest& req) const
{
    if (!req.isbn.isEmpty())
        return QString("isbn:%1").arg(req.isbn);

    QStringList terms;
    if (!req.title.isEmpty())
        terms << QString("title:(%1)").arg(req.title);
    if (!req.author.isEmpty())
        terms << QString("author:(%1)").arg(req.author);
    return terms.isEmpty() ? req.title : terms.join(' ');
}

void MetadataFetcher::fetchFromSources(const FetchRequest& req)
{
    m_requests.insert(req.bookId, req);
    m_candidates.insert(req.bookId, {});
    m_coverCandidates.insert(req.bookId, {});
    m_pendingReplies.insert(req.bookId, 0);

    if (req.useEmbedded) {
        const EmbeddedMetadata embedded = extractEmbeddedMetadata(req);
        if (embedded.hasAny()) {
            Candidate local;
            local.book = embedded.book;
            local.score = candidateScore(local.book, req) + 15;
            QVector<Book> books = m_candidates.value(req.bookId);
            books << local.book;
            m_candidates.insert(req.bookId, books);
            qWarning().noquote()
                << "[Metadata] Embedded metadata bookId=" << req.bookId
                << "title=" << embedded.book.title
                << "author=" << embedded.book.author
                << "publisher=" << embedded.book.publisher
                << "year=" << embedded.book.year;
            emit fetchProgress(m_totalDone, m_totalQueued, QFileInfo(req.filePath).fileName(),
                               "Found embedded file metadata");
        } else {
            qWarning() << "[Metadata] No embedded metadata for bookId=" << req.bookId;
            emit fetchProgress(m_totalDone, m_totalQueued, QFileInfo(req.filePath).fileName(),
                               "No embedded file metadata");
        }
    }

    if (req.useOpenLibrary)
        fetchFromOpenLibrary(req);
    if (req.useGoogle)
        fetchFromGoogle(req);

    if (m_pendingReplies.value(req.bookId) == 0) {
        QVector<Candidate> ranked;
        for (const Book& book : m_candidates.value(req.bookId)) {
            Candidate c;
            c.book = book;
            c.score = candidateScore(book, req);
            ranked << c;
        }
        Book merged = mergeBooks(ranked);
        ++m_totalDone;
        if (!merged.title.isEmpty() || !merged.author.isEmpty() || !merged.isbn.isEmpty()) {
            merged.id = req.bookId;
            emit metadataReady(req.bookId, merged);
            emit fetchProgress(m_totalDone, m_totalQueued, QFileInfo(req.filePath).fileName(), "Used embedded metadata");
        } else {
            emit fetchError(req.bookId, "No metadata source could be queried");
            emit fetchProgress(m_totalDone, m_totalQueued, QFileInfo(req.filePath).fileName(), "No sources available");
        }
        m_requests.remove(req.bookId);
        m_candidates.remove(req.bookId);
        m_coverCandidates.remove(req.bookId);
        m_pendingReplies.remove(req.bookId);
        --m_active;
        if (m_active == 0 && m_queue.isEmpty() && m_totalDone >= m_totalQueued) {
            m_totalDone = 0;
            m_totalQueued = 0;
        }
    }
}

void MetadataFetcher::fetchFromGoogle(const FetchRequest& req)
{
    QString query = buildGoogleQuery(req);
    if (query.isEmpty())
        return;

    QString urlStr = AppConfig::googleBooksUrl(QUrl::toPercentEncoding(query));
    qWarning().noquote() << "[Metadata] Google request bookId=" << req.bookId << "url=" << urlStr;

    QNetworkRequest netReq{QUrl(urlStr)};
    netReq.setHeader(QNetworkRequest::UserAgentHeader, QString("EagleLibrary/1.0 (eaglesoftware.biz)"));
    netReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_pendingReplies[req.bookId] = m_pendingReplies.value(req.bookId) + 1;

    QNetworkReply* reply = m_nam->get(netReq);
    m_activeReplies.insert(reply);
    const quint64 generation = m_generation;
    connect(reply, &QNetworkReply::finished, this, [this, reply, bookId = req.bookId, generation]() {
        m_activeReplies.remove(reply);
        if (generation != m_generation) {
            reply->deleteLater();
            return;
        }
        onGoogleReply(reply, bookId);
        reply->deleteLater();
    });
}

void MetadataFetcher::fetchFromOpenLibrary(const FetchRequest& req)
{
    QString query = buildOpenLibraryQuery(req);
    if (query.isEmpty())
        return;

    QUrl url(QStringLiteral("https://openlibrary.org/search.json"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    urlQuery.addQueryItem("limit", "3");
    urlQuery.addQueryItem("fields",
                          "title,author_name,first_publish_year,publisher,isbn,language,subject,edition_key,cover_i");
    url.setQuery(urlQuery);
    qWarning().noquote() << "[Metadata] OpenLibrary request bookId=" << req.bookId << "url=" << url.toString();

    QNetworkRequest netReq{url};
    netReq.setHeader(QNetworkRequest::UserAgentHeader, QString("EagleLibrary/1.0 (eaglesoftware.biz)"));
    netReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_pendingReplies[req.bookId] = m_pendingReplies.value(req.bookId) + 1;

    QNetworkReply* reply = m_nam->get(netReq);
    m_activeReplies.insert(reply);
    const quint64 generation = m_generation;
    connect(reply, &QNetworkReply::finished, this, [this, reply, bookId = req.bookId, generation]() {
        m_activeReplies.remove(reply);
        if (generation != m_generation) {
            reply->deleteLater();
            return;
        }
        onOpenLibraryReply(reply, bookId);
        reply->deleteLater();
    });
}

void MetadataFetcher::onGoogleReply(QNetworkReply* reply, qint64 bookId)
{
    if (m_cancelling)
        return;
    if (!m_requests.contains(bookId))
        return;

    const FetchRequest req = m_requests.value(bookId);
    const QString fileName = QFileInfo(req.filePath).fileName();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray items = doc.object().value("items").toArray();
        QVector<Book> books = m_candidates.value(bookId);
        QStringList coverUrls = m_coverCandidates.value(bookId);
        for (const QJsonValue& value : items) {
            QJsonObject volume = value.toObject();
            Candidate candidate;
            candidate.book = parseGoogleVolume(volume);
            candidate.score = candidateScore(candidate.book, req);

            QJsonObject imageLinks = volume.value("volumeInfo").toObject().value("imageLinks").toObject();
            QStringList urls{
                imageLinks.value("thumbnail").toString(),
                imageLinks.value("smallThumbnail").toString()
            };
            candidate.coverUrls = uniqueUrls(urls);
            if (!candidate.book.title.isEmpty() || !candidate.book.author.isEmpty() || !candidate.book.isbn.isEmpty()) {
                books << candidate.book;
                coverUrls.append(candidate.coverUrls);
            }
        }
        m_candidates.insert(bookId, books);
        m_coverCandidates.insert(bookId, coverUrls);
        emit fetchProgress(m_totalDone, m_totalQueued, fileName, "Checked Google Books");
        qWarning() << "[Metadata] Google success bookId=" << bookId
                   << "candidates=" << books.size()
                   << "coverUrls=" << coverUrls.size();
    } else {
        qWarning().noquote() << "[Metadata] Google error bookId=" << bookId
                             << "error=" << reply->errorString();
    }

    m_pendingReplies[bookId] = m_pendingReplies.value(bookId) - 1;
    if (m_pendingReplies.value(bookId) == 0) {
        QVector<Candidate> ranked;
        for (const Book& book : m_candidates.value(bookId)) {
            Candidate c;
            c.book = book;
            c.score = candidateScore(book, req);
            ranked << c;
        }
        Book merged = mergeBooks(ranked);
        if (!merged.title.isEmpty() || !merged.author.isEmpty() || !merged.isbn.isEmpty()) {
            merged.id = bookId;
            qWarning().noquote()
                << "[Metadata] Final merged bookId=" << bookId
                << "title=" << merged.title
                << "author=" << merged.author
                << "isbn=" << merged.isbn
                << "publisher=" << merged.publisher
                << "year=" << merged.year
                << "coverCandidates=" << m_coverCandidates.value(bookId).size();
            emit metadataReady(bookId, merged);
            emit coverUrlsReady(bookId, uniqueUrls(m_coverCandidates.value(bookId)));
            ++m_totalDone;
            emit fetchProgress(m_totalDone, m_totalQueued, fileName, "Metadata merged");
        } else {
            ++m_totalDone;
            emit fetchError(bookId, "No metadata found in configured sources");
            emit fetchProgress(m_totalDone, m_totalQueued, fileName, "No metadata found");
        }
        m_requests.remove(bookId);
        m_candidates.remove(bookId);
        m_coverCandidates.remove(bookId);
        m_pendingReplies.remove(bookId);
        --m_active;
        if (m_active == 0 && m_queue.isEmpty() && m_totalDone >= m_totalQueued) {
            m_totalDone = 0;
            m_totalQueued = 0;
        }
    }
}

void MetadataFetcher::onOpenLibraryReply(QNetworkReply* reply, qint64 bookId)
{
    if (m_cancelling)
        return;
    if (!m_requests.contains(bookId))
        return;

    const FetchRequest req = m_requests.value(bookId);
    const QString fileName = QFileInfo(req.filePath).fileName();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray docs = doc.object().value("docs").toArray();
        QVector<Book> books = m_candidates.value(bookId);
        QStringList coverUrls = m_coverCandidates.value(bookId);
        for (const QJsonValue& value : docs) {
            QJsonObject item = value.toObject();
            Candidate candidate;
            candidate.book = parseOpenLibraryDoc(item);
            candidate.score = candidateScore(candidate.book, req);

            QStringList urls;
            QStringList isbns;
            for (const QJsonValue& isbn : item.value("isbn").toArray())
                isbns << isbn.toString();
            for (const QString& isbn : isbns.mid(0, 2))
                urls << AppConfig::openLibraryCoverUrl("isbn", isbn);

            QStringList editionKeys;
            for (const QJsonValue& key : item.value("edition_key").toArray())
                editionKeys << key.toString();
            for (const QString& key : editionKeys.mid(0, 1))
                urls << AppConfig::openLibraryCoverUrl("olid", key);

            const int coverId = item.value("cover_i").toInt();
            if (coverId > 0)
                urls << AppConfig::openLibraryCoverUrl("id", QString::number(coverId));

            candidate.coverUrls = uniqueUrls(urls);
            if (!candidate.book.title.isEmpty() || !candidate.book.author.isEmpty() || !candidate.book.isbn.isEmpty()) {
                books << candidate.book;
                coverUrls.append(candidate.coverUrls);
            }
        }
        m_candidates.insert(bookId, books);
        m_coverCandidates.insert(bookId, coverUrls);
        emit fetchProgress(m_totalDone, m_totalQueued, fileName, "Checked Open Library");
        qWarning() << "[Metadata] OpenLibrary success bookId=" << bookId
                   << "candidates=" << books.size()
                   << "coverUrls=" << coverUrls.size();
    } else {
        qWarning().noquote() << "[Metadata] OpenLibrary error bookId=" << bookId
                             << "error=" << reply->errorString();
    }

    m_pendingReplies[bookId] = m_pendingReplies.value(bookId) - 1;
    if (m_pendingReplies.value(bookId) == 0) {
        QVector<Candidate> ranked;
        for (const Book& book : m_candidates.value(bookId)) {
            Candidate c;
            c.book = book;
            c.score = candidateScore(book, req);
            ranked << c;
        }
        Book merged = mergeBooks(ranked);
        if (!merged.title.isEmpty() || !merged.author.isEmpty() || !merged.isbn.isEmpty()) {
            merged.id = bookId;
            emit metadataReady(bookId, merged);
            emit coverUrlsReady(bookId, uniqueUrls(m_coverCandidates.value(bookId)));
            ++m_totalDone;
            emit fetchProgress(m_totalDone, m_totalQueued, fileName, "Metadata merged");
        } else {
            ++m_totalDone;
            emit fetchError(bookId, "No metadata found in configured sources");
            emit fetchProgress(m_totalDone, m_totalQueued, fileName, "No metadata found");
        }
        m_requests.remove(bookId);
        m_candidates.remove(bookId);
        m_coverCandidates.remove(bookId);
        m_pendingReplies.remove(bookId);
        --m_active;
        if (m_active == 0 && m_queue.isEmpty() && m_totalDone >= m_totalQueued) {
            m_totalDone = 0;
            m_totalQueued = 0;
        }
    }
}

Book MetadataFetcher::parseGoogleVolume(const QJsonObject& volume) const
{
    Book b;
    QJsonObject info = volume.value("volumeInfo").toObject();

    b.title = normaliseSpaces(info.value("title").toString());
    QString subtitle = normaliseSpaces(info.value("subtitle").toString());
    if (!subtitle.isEmpty())
        b.title += ": " + subtitle;

    QStringList authorList;
    for (const QJsonValue& value : info.value("authors").toArray())
        authorList << value.toString();
    b.author = authorList.join(", ");

    b.publisher = info.value("publisher").toString();
    b.description = info.value("description").toString();
    b.language = info.value("language").toString().toUpper();
    b.pages = info.value("pageCount").toInt();
    b.year = info.value("publishedDate").toString().left(4).toInt();

    QJsonArray ids = info.value("industryIdentifiers").toArray();
    for (const QJsonValue& value : ids) {
        const QJsonObject idObj = value.toObject();
        if (idObj.value("type").toString() == "ISBN_13") {
            b.isbn = idObj.value("identifier").toString();
            break;
        }
    }
    if (b.isbn.isEmpty()) {
        for (const QJsonValue& value : ids) {
            const QJsonObject idObj = value.toObject();
            if (idObj.value("type").toString() == "ISBN_10") {
                b.isbn = idObj.value("identifier").toString();
                break;
            }
        }
    }

    for (const QJsonValue& value : info.value("categories").toArray())
        b.subjects << value.toString();

    b.rating = info.value("averageRating").toDouble(0);
    return b;
}

Book MetadataFetcher::parseOpenLibraryDoc(const QJsonObject& doc) const
{
    Book b;
    b.title = normaliseSpaces(doc.value("title").toString());

    QStringList authors;
    for (const QJsonValue& value : doc.value("author_name").toArray())
        authors << value.toString();
    b.author = authors.join(", ");

    QJsonArray publishers = doc.value("publisher").toArray();
    if (!publishers.isEmpty())
        b.publisher = publishers.first().toString();

    b.year = doc.value("first_publish_year").toInt();

    QJsonArray isbns = doc.value("isbn").toArray();
    if (!isbns.isEmpty())
        b.isbn = isbns.first().toString();

    QJsonArray langs = doc.value("language").toArray();
    if (!langs.isEmpty()) {
        QString language = langs.first().toString();
        const int slash = language.lastIndexOf('/');
        if (slash >= 0)
            language = language.mid(slash + 1);
        b.language = language.toUpper();
    }

    QJsonArray subjects = doc.value("subject").toArray();
    for (int i = 0; i < subjects.size() && i < 6; ++i)
        b.subjects << subjects.at(i).toString();

    return b;
}
