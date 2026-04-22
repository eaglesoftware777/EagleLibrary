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

// Minimum score a remote (Google/OpenLibrary) candidate must reach to be accepted.
// Embedded-metadata candidates are exempt — they come directly from the file.
constexpr int kMinAcceptScore = 20;

struct EmbeddedMetadata {
    Book book;
    bool hasAny() const {
        return !book.title.isEmpty() || !book.author.isEmpty()
            || !book.publisher.isEmpty() || book.year > 0 || !book.language.isEmpty();
    }
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
    for (const QString& token : blockedTokens)
        if (lowered.contains(token)) return true;
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
    QRegularExpression re(
        QString("/%1\\s*(\\((?:\\\\.|[^\\)]){1,512}\\)|<([0-9A-Fa-f\\s]{2,1024})>)")
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
    meta.book.title     = extractPdfField(text, "Title");
    meta.book.author    = extractPdfField(text, "Author");
    meta.book.publisher = extractPdfField(text, "Publisher");
    meta.book.language  = extractPdfField(text, "Lang");

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
        if (url.isEmpty()) continue;
        url.replace("http://", "https://");
        if (!seen.contains(url)) {
            seen.insert(url);
            out << url;
        }
    }
    return out;
}

// Strip filename artifacts so the title is a clean book search query.
QString cleanQueryTitle(const QString& raw)
{
    if (raw.isEmpty())
        return {};

    QString s = raw;

    // Filename separators → spaces
    s.replace('_', ' ');
    // mid-word dot used as word separator (e.g. "intro.to.python")
    s.replace(QRegularExpression(R"((?<=[A-Za-z0-9])\.(?=[A-Za-z0-9]))"), " ");

    // Strip ISBNs (978/979 prefix or bare 10-digit) that end up in filenames
    s.remove(QRegularExpression(R"(\b97[89][0-9 \-]{9,15}\b)"));
    s.remove(QRegularExpression(R"(\b[0-9]{9,10}[Xx]?\b)"));

    // Strip content in square brackets (source tags, format indicators)
    // e.g. [z-lib.org], [libgen], [PDF], [EN], [2023]
    s.remove(QRegularExpression(R"(\[[^\]]{1,48}\])"));

    // Strip publisher / source names in parentheses
    s.remove(QRegularExpression(
        R"(\((?:O'?Reilly|Springer|Packt|Wiley|Manning|Apress|No Starch|MIT Press|Pearson|Addison|Cambridge|Oxford|McGraw|Academic)[^)]{0,30}\))",
        QRegularExpression::CaseInsensitiveOption));

    // Year in parentheses or brackets
    s.remove(QRegularExpression(R"(\((?:19|20)\d{2}\))"));
    s.remove(QRegularExpression(R"(\[(?:19|20)\d{2}\])"));

    // Trailing year after a dash or as standalone: " - 2023", " 2021"
    s.remove(QRegularExpression(R"(\s*[-–]\s*(?:19|20)\d{2}\s*$)"));
    s.remove(QRegularExpression(R"(\s+(?:19|20)\d{2}\s*$)"));

    // Edition markers: "3rd Edition", "Second Edition", "Ed.", "Vol. 2"
    s.remove(QRegularExpression(
        R"(\b\d{1,2}(?:st|nd|rd|th)\s+[Ee]d(?:ition)?\b)"));
    s.remove(QRegularExpression(
        R"(\b(?:First|Second|Third|Fourth|Fifth|Sixth|Seventh|Eighth|Ninth|Tenth)\s+[Ee]dition\b)",
        QRegularExpression::CaseInsensitiveOption));
    s.remove(QRegularExpression(
        R"(\b(?:[Ee]d(?:ition)?\.?|[Vv]ol(?:ume)?\.?\s*\d+|[Rr]ev(?:ised)?|[Uu]pdated)\b)"));

    // Version numbers: v2.0, v3, version 3.1
    s.remove(QRegularExpression(
        R"(\bv(?:er(?:sion)?)?\s*\d+(?:\.\d+)*\b)",
        QRegularExpression::CaseInsensitiveOption));

    // Language suffix: "- English", ", French", "_en"
    s.remove(QRegularExpression(
        R"(\s*[-,]\s*(?:English|French|German|Russian|Spanish|Chinese|Japanese|Italian)\s*$)",
        QRegularExpression::CaseInsensitiveOption));

    // "by Author Name" at end
    s.remove(QRegularExpression(R"(\s+[Bb]y\s+[A-Z][a-z].+$)"));

    s = s.simplified();

    // Truncate to at most 8 meaningful words to avoid over-constraining the query
    QStringList words = s.split(u' ', Qt::SkipEmptyParts);
    if (words.size() > 8)
        words = words.first(8);

    return words.join(u' ').trimmed();
}

// Count meaningful (non-stopword, >= 3 chars) words shared between two titles.
int titleWordOverlap(const QString& a, const QString& b)
{
    static const QSet<QString> stopwords = {
        "the","a","an","of","in","on","at","to","for","and","or","by",
        "with","from","is","it","as","be","if","no","its","this","that","de"
    };
    auto tokens = [&](const QString& s) -> QSet<QString> {
        QSet<QString> out;
        for (const QString& w : s.toLower().split(QRegularExpression("[^a-z0-9]+"), Qt::SkipEmptyParts))
            if (w.length() >= 3 && !stopwords.contains(w))
                out.insert(w);
        return out;
    };
    const QSet<QString> ta = tokens(a);
    const QSet<QString> tb = tokens(b);
    int n = 0;
    for (const QString& w : ta)
        if (tb.contains(w)) ++n;
    return n;
}

// Score how well a fetched book matches the original request.
// Higher = better match. Embedded candidates add +15 externally.
int candidateScore(const Book& book, const FetchRequest& req)
{
    auto norm = [](const QString& s) {
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

    // ISBN — strongest single signal
    if (!req.isbn.isEmpty() && !book.isbn.isEmpty()) {
        const QString rn = norm(req.isbn);
        const QString bn = norm(book.isbn);
        if (rn == bn) {
            score += 30;
        } else {
            // Cross-match ISBN-10 ↔ ISBN-13 (978-prefix): same book, different edition format
            bool crossMatch = false;
            if (rn.size() == 13 && bn.size() == 10)
                crossMatch = (rn.mid(3, 9) == bn.left(9));
            else if (rn.size() == 10 && bn.size() == 13)
                crossMatch = (bn.mid(3, 9) == rn.left(9));
            score += crossMatch ? 28 : -30;
        }
    }

    // Title comparison — with word-level overlap penalty for zero-match results
    const QString rTitle = norm(req.title);
    const QString bTitle = norm(book.title);
    if (!rTitle.isEmpty() && !bTitle.isEmpty()) {
        if (rTitle == bTitle) {
            score += 20;
        } else if (bTitle.contains(rTitle) || rTitle.contains(bTitle)) {
            score += 12;
        } else {
            const int overlap = titleWordOverlap(req.title, book.title);
            if (overlap >= 3)      score += overlap * 4;
            else if (overlap == 2) score += 8;
            else if (overlap == 1) score += 2;
            else                   score -= 18; // no shared meaningful words → likely wrong book
        }
    }

    // Author comparison
    const QString rAuthor = norm(req.author);
    const QString bAuthor = norm(book.author);
    if (!rAuthor.isEmpty() && !bAuthor.isEmpty()) {
        if (rAuthor == bAuthor)                                              score += 12;
        else if (bAuthor.contains(rAuthor) || rAuthor.contains(bAuthor))   score += 6;
    }

    return score;
}

Book mergeBooks(QVector<Candidate>& candidates)
{
    if (candidates.isEmpty())
        return {};

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.score > b.score;
    });

    Book merged = candidates.first().book;
    for (int i = 1; i < candidates.size(); ++i) {
        const Book& src = candidates[i].book;
        if (merged.title.isEmpty())       merged.title       = src.title;
        if (merged.author.isEmpty())      merged.author      = src.author;
        if (merged.publisher.isEmpty())   merged.publisher   = src.publisher;
        if (merged.isbn.isEmpty())        merged.isbn        = src.isbn;
        if (merged.language.isEmpty())    merged.language    = src.language;
        if (merged.year == 0)             merged.year        = src.year;
        if (merged.pages == 0)            merged.pages       = src.pages;
        if (merged.rating == 0)           merged.rating      = src.rating;
        if (merged.description.isEmpty()) merged.description = src.description;
        if (merged.subjects.isEmpty())    merged.subjects    = src.subjects;
    }
    return merged;
}

} // namespace

// ─────────────────────────────────────────────────────────────

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
    if (m_pendingBookIds.contains(req.bookId)) {
        qDebug().noquote()
            << "[Metadata] Duplicate queue request ignored bookId=" << req.bookId
            << "title=" << req.title;
        return;
    }

    m_pendingBookIds.insert(req.bookId);
    m_queue.enqueue(req);
    ++m_totalQueued;
    if (!m_processTimer->isActive())
        m_processTimer->start();
    if (m_totalQueued <= 5 || (m_totalQueued % 50) == 0) {
        qInfo().noquote()
            << "[Metadata] Queue total=" << m_totalQueued
            << "latestBookId=" << req.bookId
            << "title=" << req.title;
    }
    emit fetchProgress(m_totalDone, m_totalQueued, QFileInfo(req.filePath).fileName(), "Queued");
}

void MetadataFetcher::cancelAll()
{
    m_cancelling = true;
    ++m_generation;
    m_processTimer->stop();
    m_queue.clear();

    const auto activeReplies = m_activeReplies;
    for (QNetworkReply* reply : activeReplies)
        if (reply) reply->abort();
    m_activeReplies.clear();

    m_requests.clear();
    m_candidates.clear();
    m_pendingReplies.clear();
    m_pendingBookIds.clear();
    m_active      = 0;
    m_totalQueued = 0;
    m_totalDone   = 0;
    emit fetchFinished();
}

bool MetadataFetcher::isRunning() const
{
    return !m_queue.isEmpty() || m_active > 0 || !m_activeReplies.isEmpty();
}

void MetadataFetcher::processQueue()
{
    if (m_cancelling) return;
    while (!m_queue.isEmpty() && m_active < m_maxConcurrent) {
        ++m_active;
        fetchFromSources(m_queue.dequeue());
    }
}

QString MetadataFetcher::buildGoogleQuery(const FetchRequest& req) const
{
    if (!req.isbn.isEmpty())
        return QString("isbn:%1").arg(req.isbn);

    const QString cleanTitle = cleanQueryTitle(req.title);
    // Skip search if we have nothing useful
    if (cleanTitle.length() < 4 && req.author.isEmpty())
        return {};

    QString q = cleanTitle;
    if (!req.author.isEmpty())
        q += QString(" inauthor:\"%1\"").arg(req.author);
    return q.trimmed();
}

QString MetadataFetcher::buildOpenLibraryQuery(const FetchRequest& req) const
{
    if (!req.isbn.isEmpty())
        return QString("isbn:%1").arg(req.isbn);

    const QString cleanTitle = cleanQueryTitle(req.title);
    if (cleanTitle.length() < 4 && req.author.isEmpty())
        return {};

    QStringList terms;
    if (!cleanTitle.isEmpty())
        terms << QString("title:(%1)").arg(cleanTitle);
    if (!req.author.isEmpty())
        terms << QString("author:(%1)").arg(req.author);
    return terms.isEmpty() ? cleanTitle : terms.join(' ');
}

void MetadataFetcher::fetchFromSources(const FetchRequest& req)
{
    m_requests.insert(req.bookId, req);
    m_candidates.insert(req.bookId, {});
    m_pendingReplies.insert(req.bookId, 0);

    if (req.useEmbedded) {
        const EmbeddedMetadata embedded = extractEmbeddedMetadata(req);
        if (embedded.hasAny()) {
            Candidate local;
            local.book     = embedded.book;
            local.embedded = true;
            local.score    = candidateScore(local.book, req) + 15;
            m_candidates[req.bookId].append(local);
            qDebug().noquote()
                << "[Metadata] Embedded bookId=" << req.bookId
                << "title=" << embedded.book.title << "author=" << embedded.book.author;
            emit fetchProgress(m_totalDone, m_totalQueued,
                               QFileInfo(req.filePath).fileName(), "Found embedded file metadata");
        }
    }

    if (req.useOpenLibrary) fetchFromOpenLibrary(req);
    if (req.useGoogle)      fetchFromGoogle(req);

    if (m_pendingReplies.value(req.bookId) == 0)
        finalize(req.bookId);
}

void MetadataFetcher::fetchFromGoogle(const FetchRequest& req)
{
    const QString query = buildGoogleQuery(req);
    if (query.isEmpty())
        return;

    const QString urlStr = AppConfig::googleBooksUrl(QUrl::toPercentEncoding(query));
    qDebug().noquote() << "[Metadata] Google bookId=" << req.bookId << "url=" << urlStr;

    QNetworkRequest netReq{QUrl(urlStr)};
    netReq.setHeader(QNetworkRequest::UserAgentHeader, "EagleLibrary/1.0 (eaglesoftware.biz)");
    netReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::NoLessSafeRedirectPolicy);

    m_pendingReplies[req.bookId]++;
    QNetworkReply* reply = m_nam->get(netReq);
    m_activeReplies.insert(reply);
    const quint64 generation = m_generation;
    connect(reply, &QNetworkReply::finished, this, [this, reply, bookId = req.bookId, generation]() {
        m_activeReplies.remove(reply);
        if (generation != m_generation) { reply->deleteLater(); return; }
        onGoogleReply(reply, bookId);
        reply->deleteLater();
    });
}

void MetadataFetcher::fetchFromOpenLibrary(const FetchRequest& req)
{
    const QString query = buildOpenLibraryQuery(req);
    if (query.isEmpty())
        return;

    QUrl url("https://openlibrary.org/search.json");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    urlQuery.addQueryItem("limit", "3");
    urlQuery.addQueryItem("fields",
                          "title,author_name,first_publish_year,publisher,isbn,language,subject,edition_key,cover_i");
    url.setQuery(urlQuery);
    qDebug().noquote() << "[Metadata] OpenLibrary bookId=" << req.bookId << "url=" << url.toString();

    QNetworkRequest netReq{url};
    netReq.setHeader(QNetworkRequest::UserAgentHeader, "EagleLibrary/1.0 (eaglesoftware.biz)");
    netReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::NoLessSafeRedirectPolicy);

    m_pendingReplies[req.bookId]++;
    QNetworkReply* reply = m_nam->get(netReq);
    m_activeReplies.insert(reply);
    const quint64 generation = m_generation;
    connect(reply, &QNetworkReply::finished, this, [this, reply, bookId = req.bookId, generation]() {
        m_activeReplies.remove(reply);
        if (generation != m_generation) { reply->deleteLater(); return; }
        onOpenLibraryReply(reply, bookId);
        reply->deleteLater();
    });
}

void MetadataFetcher::onGoogleReply(QNetworkReply* reply, qint64 bookId)
{
    if (m_cancelling || !m_requests.contains(bookId))
        return;

    const FetchRequest req = m_requests.value(bookId);
    const QString fileName  = QFileInfo(req.filePath).fileName();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc   = QJsonDocument::fromJson(reply->readAll());
        QJsonArray    items = doc.object().value("items").toArray();

        for (const QJsonValue& value : items) {
            QJsonObject volume = value.toObject();
            Candidate   c;
            c.book     = parseGoogleVolume(volume);
            c.embedded = false;

            QJsonObject imageLinks =
                volume.value("volumeInfo").toObject().value("imageLinks").toObject();
            c.coverUrls = uniqueUrls({
                imageLinks.value("thumbnail").toString(),
                imageLinks.value("smallThumbnail").toString()
            });

            if (!c.book.title.isEmpty() || !c.book.author.isEmpty() || !c.book.isbn.isEmpty())
                m_candidates[bookId].append(c);
        }

        qDebug() << "[Metadata] Google success bookId=" << bookId
                 << "total candidates=" << m_candidates.value(bookId).size();
        emit fetchProgress(m_totalDone, m_totalQueued, fileName, "Checked Google Books");
    } else {
        qDebug().noquote() << "[Metadata] Google error bookId=" << bookId
                           << reply->errorString();
    }

    m_pendingReplies[bookId]--;
    if (m_pendingReplies.value(bookId) == 0)
        finalize(bookId);
}

void MetadataFetcher::onOpenLibraryReply(QNetworkReply* reply, qint64 bookId)
{
    if (m_cancelling || !m_requests.contains(bookId))
        return;

    const FetchRequest req = m_requests.value(bookId);
    const QString fileName  = QFileInfo(req.filePath).fileName();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc  = QJsonDocument::fromJson(reply->readAll());
        QJsonArray    docs = doc.object().value("docs").toArray();

        for (const QJsonValue& value : docs) {
            QJsonObject item = value.toObject();
            Candidate   c;
            c.book     = parseOpenLibraryDoc(item);
            c.embedded = false;

            QStringList urls;
            QStringList isbns;
            for (const QJsonValue& v : item.value("isbn").toArray())
                isbns << v.toString();
            for (const QString& isbn : isbns.mid(0, 2))
                urls << AppConfig::openLibraryCoverUrl("isbn", isbn);

            QStringList editionKeys;
            for (const QJsonValue& v : item.value("edition_key").toArray())
                editionKeys << v.toString();
            if (!editionKeys.isEmpty())
                urls << AppConfig::openLibraryCoverUrl("olid", editionKeys.first());

            const int coverId = item.value("cover_i").toInt();
            if (coverId > 0)
                urls << AppConfig::openLibraryCoverUrl("id", QString::number(coverId));

            c.coverUrls = uniqueUrls(urls);

            if (!c.book.title.isEmpty() || !c.book.author.isEmpty() || !c.book.isbn.isEmpty())
                m_candidates[bookId].append(c);
        }

        qDebug() << "[Metadata] OpenLibrary success bookId=" << bookId
                 << "total candidates=" << m_candidates.value(bookId).size();
        emit fetchProgress(m_totalDone, m_totalQueued, fileName, "Checked Open Library");
    } else {
        qDebug().noquote() << "[Metadata] OpenLibrary error bookId=" << bookId
                           << reply->errorString();
    }

    m_pendingReplies[bookId]--;
    if (m_pendingReplies.value(bookId) == 0)
        finalize(bookId);
}

// Common finalization: score, filter, merge, emit.
void MetadataFetcher::finalize(qint64 bookId)
{
    const FetchRequest req      = m_requests.value(bookId);
    const QString      fileName = QFileInfo(req.filePath).fileName();

    QVector<Candidate>& raw = m_candidates[bookId];

    // Score every candidate (embedded ones already have score set; re-score is fine as
    // the +15 is re-applied via the embedded flag below).
    for (Candidate& c : raw) {
        c.score = candidateScore(c.book, req);
        if (c.embedded) c.score += 15;
    }

    // Accept remote candidates only if they meet the minimum confidence bar.
    // Embedded candidates are always kept — they come from the file itself.
    QVector<Candidate> accepted;
    for (const Candidate& c : raw)
        if (c.embedded || c.score >= kMinAcceptScore)
            accepted.append(c);

    // If everything was filtered and we have embedded data, use it as a last resort.
    if (accepted.isEmpty()) {
        for (const Candidate& c : raw) {
            if (c.embedded && c.score >= 0) {
                accepted.append(c);
                break;
            }
        }
    }

    // Sort by score descending so mergeBooks picks the best one first.
    std::sort(accepted.begin(), accepted.end(), [](const Candidate& a, const Candidate& b) {
        return a.score > b.score;
    });

    Book merged = mergeBooks(accepted); // accepted is sorted in-place again (harmless)

    // Cover URLs come ONLY from the highest-scoring candidate that has any.
    // This prevents a wrong book's cover from polluting the result.
    QStringList bestCovers;
    for (const Candidate& c : accepted) {
        if (!c.coverUrls.isEmpty()) {
            bestCovers = c.coverUrls;
            break;
        }
    }

    ++m_totalDone;
    if (!merged.title.isEmpty() || !merged.author.isEmpty() || !merged.isbn.isEmpty()) {
        merged.id = bookId;
        qDebug().noquote()
            << "[Metadata] Final bookId=" << bookId
            << "title=" << merged.title << "author=" << merged.author
            << "isbn=" << merged.isbn << "year=" << merged.year
            << "covers=" << bestCovers.size()
            << "topScore=" << (accepted.isEmpty() ? 0 : accepted.first().score);
        emit metadataReady(bookId, merged);
        if (!bestCovers.isEmpty())
            emit coverUrlsReady(bookId, uniqueUrls(bestCovers));
        emit fetchProgress(m_totalDone, m_totalQueued, fileName, "Metadata merged");
    } else {
        emit fetchError(bookId, "No metadata found in configured sources");
        emit fetchProgress(m_totalDone, m_totalQueued, fileName, "No metadata found");
    }

    m_requests.remove(bookId);
    m_candidates.remove(bookId);
    m_pendingReplies.remove(bookId);
    m_pendingBookIds.remove(bookId);
    --m_active;
    if (m_active == 0 && m_queue.isEmpty() && m_totalDone >= m_totalQueued) {
        m_totalDone   = 0;
        m_totalQueued = 0;
        emit fetchFinished();
    }
}

Book MetadataFetcher::parseGoogleVolume(const QJsonObject& volume) const
{
    Book b;
    QJsonObject info = volume.value("volumeInfo").toObject();

    b.title = normaliseSpaces(info.value("title").toString());
    const QString subtitle = normaliseSpaces(info.value("subtitle").toString());
    if (!subtitle.isEmpty())
        b.title += ": " + subtitle;

    QStringList authorList;
    for (const QJsonValue& v : info.value("authors").toArray())
        authorList << v.toString();
    b.author = authorList.join(", ");

    b.publisher   = info.value("publisher").toString();
    b.description = info.value("description").toString();
    b.language    = info.value("language").toString().toUpper();
    b.pages       = info.value("pageCount").toInt();
    b.year        = info.value("publishedDate").toString().left(4).toInt();

    QJsonArray ids = info.value("industryIdentifiers").toArray();
    for (const QJsonValue& v : ids) {
        const QJsonObject idObj = v.toObject();
        if (idObj.value("type").toString() == "ISBN_13") {
            b.isbn = idObj.value("identifier").toString();
            break;
        }
    }
    if (b.isbn.isEmpty()) {
        for (const QJsonValue& v : ids) {
            const QJsonObject idObj = v.toObject();
            if (idObj.value("type").toString() == "ISBN_10") {
                b.isbn = idObj.value("identifier").toString();
                break;
            }
        }
    }

    for (const QJsonValue& v : info.value("categories").toArray())
        b.subjects << v.toString();

    b.rating = info.value("averageRating").toDouble(0);
    return b;
}

Book MetadataFetcher::parseOpenLibraryDoc(const QJsonObject& doc) const
{
    Book b;
    b.title = normaliseSpaces(doc.value("title").toString());

    QStringList authors;
    for (const QJsonValue& v : doc.value("author_name").toArray())
        authors << v.toString();
    b.author = authors.join(", ");

    QJsonArray publishers = doc.value("publisher").toArray();
    if (!publishers.isEmpty())
        b.publisher = publishers.first().toString();

    b.year = doc.value("first_publish_year").toInt();

    QJsonArray isbns = doc.value("isbn").toArray();
    for (const QJsonValue& v : isbns) {
        const QString candidate = v.toString().remove('-').remove(' ');
        if (candidate.size() == 13 && (candidate.startsWith("978") || candidate.startsWith("979"))) {
            b.isbn = v.toString();
            break;
        }
    }
    if (b.isbn.isEmpty()) {
        for (const QJsonValue& v : isbns) {
            const QString candidate = v.toString().remove('-').remove(' ');
            if (candidate.size() == 10) { b.isbn = v.toString(); break; }
        }
    }

    QJsonArray langs = doc.value("language").toArray();
    if (!langs.isEmpty()) {
        QString language = langs.first().toString();
        const int slash = language.lastIndexOf('/');
        if (slash >= 0) language = language.mid(slash + 1);
        b.language = language.toUpper();
    }

    QJsonArray subjects = doc.value("subject").toArray();
    for (int i = 0; i < subjects.size() && i < 6; ++i)
        b.subjects << subjects.at(i).toString();

    return b;
}
