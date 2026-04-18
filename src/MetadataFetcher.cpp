// ============================================================
//  Eagle Library -- MetadataFetcher.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include "MetadataFetcher.h"
#include "AppConfig.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

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
    m_queue.enqueue(req);
}

void MetadataFetcher::processQueue()
{
    while (!m_queue.isEmpty() && m_active < m_maxConcurrent) {
        fetchFromGoogle(m_queue.dequeue());
        ++m_active;
    }
}

QString MetadataFetcher::buildGoogleQuery(const FetchRequest& req) const
{
    if (!req.isbn.isEmpty())
        return QString("isbn:%1").arg(req.isbn);
    QString q = req.title;
    if (!req.author.isEmpty())
        q += QString("+inauthor:%1").arg(req.author);
    return q;
}

void MetadataFetcher::fetchFromGoogle(const FetchRequest& req)
{
    QString query  = buildGoogleQuery(req);
    QString urlStr = AppConfig::googleBooksUrl(QUrl::toPercentEncoding(query));

    QUrl url(urlStr);
    QNetworkRequest netReq(url);
    netReq.setHeader(QNetworkRequest::UserAgentHeader,
                     QString("EagleLibrary/1.0 (eaglesoftware.biz)"));
    netReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::NoLessSafeRedirectPolicy);

    qint64 bookId = req.bookId;
    QNetworkReply* reply = m_nam->get(netReq);

    connect(reply, &QNetworkReply::finished, this, [this, reply, bookId]() {
        --m_active;
        onGoogleReply(reply, bookId);
        reply->deleteLater();
    });
}

void MetadataFetcher::onGoogleReply(QNetworkReply* reply, qint64 bookId)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit fetchError(bookId, reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) { emit fetchError(bookId, "JSON parse error"); return; }

    QJsonObject root = doc.object();
    if (root["totalItems"].toInt(0) == 0) {
        emit fetchError(bookId, "No results found");
        return;
    }

    QJsonArray items = root["items"].toArray();
    if (items.isEmpty()) { emit fetchError(bookId, "Empty items"); return; }

    QJsonObject volume = items.first().toObject();
    Book updated = parseGoogleVolume(volume);
    updated.id = bookId;
    emit metadataReady(bookId, updated);

    // Cover image
    QJsonObject imageLinks = volume["volumeInfo"].toObject()["imageLinks"].toObject();
    QString coverUrl = imageLinks["thumbnail"].toString();
    if (coverUrl.isEmpty()) coverUrl = imageLinks["smallThumbnail"].toString();
    if (!coverUrl.isEmpty()) {
        coverUrl.replace("http://", "https://");
        coverUrl.replace("zoom=1", "zoom=2");
        emit coverUrlReady(bookId, coverUrl);
    }
}

Book MetadataFetcher::parseGoogleVolume(const QJsonObject& volume) const
{
    Book b;
    QJsonObject info = volume["volumeInfo"].toObject();

    b.title = info["title"].toString();
    QString subtitle = info["subtitle"].toString();
    if (!subtitle.isEmpty()) b.title += ": " + subtitle;

    QJsonArray authors = info["authors"].toArray();
    QStringList authorList;
    for (const auto& a : authors) authorList << a.toString();
    b.author = authorList.join(", ");

    b.publisher   = info["publisher"].toString();
    b.description = info["description"].toString();
    b.language    = info["language"].toString().toUpper();
    b.pages       = info["pageCount"].toInt();

    QString publishedDate = info["publishedDate"].toString();
    b.year = publishedDate.left(4).toInt();

    QJsonArray ids = info["industryIdentifiers"].toArray();
    for (const auto& id : ids) {
        QJsonObject idObj = id.toObject();
        if (idObj["type"].toString() == "ISBN_13") {
            b.isbn = idObj["identifier"].toString();
            break;
        }
    }
    if (b.isbn.isEmpty()) {
        for (const auto& id : ids) {
            QJsonObject idObj = id.toObject();
            if (idObj["type"].toString() == "ISBN_10") {
                b.isbn = idObj["identifier"].toString();
                break;
            }
        }
    }

    QJsonArray cats = info["categories"].toArray();
    for (const auto& c : cats) b.subjects << c.toString();

    b.rating = info["averageRating"].toDouble(0);
    return b;
}
