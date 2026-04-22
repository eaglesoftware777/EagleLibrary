// ============================================================
//  Eagle Library — CoverDownloader.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "CoverDownloader.h"

#include <QFile>
#include <QBuffer>
#include <QImageReader>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDebug>
#include <functional>
#include <memory>

CoverDownloader::CoverDownloader(const QString& saveDir, QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_saveDir(saveDir)
{
    QDir().mkpath(saveDir);
}

void CoverDownloader::enqueue(qint64 bookId, const QStringList& urls, const QString& label)
{
    if (urls.isEmpty())
        return;

    m_cancelling = false;
    qWarning().noquote()
        << "[Cover] Queue bookId=" << bookId
        << "label=" << label
        << "urlCount=" << urls.size()
        << "firstUrl=" << urls.first();
    m_queue.enqueue({bookId, urls, label});
    ++m_totalQueued;
    emit downloadProgress(m_totalDone, m_totalQueued, label);
    processNext();
}

void CoverDownloader::cancelAll()
{
    m_cancelling = true;
    ++m_generation;
    m_queue.clear();

    const auto activeReplies = m_activeReplies;
    for (QNetworkReply* reply : activeReplies) {
        if (reply)
            reply->abort();
    }
    m_activeReplies.clear();

    m_active = 0;
    m_totalQueued = 0;
    m_totalDone = 0;
    emit downloadsFinished();
}

bool CoverDownloader::isRunning() const
{
    return !m_queue.isEmpty() || m_active > 0 || !m_activeReplies.isEmpty();
}

void CoverDownloader::processNext()
{
    if (m_cancelling)
        return;
    while (!m_queue.isEmpty() && m_active < MAX_CONCURRENT) {
        const CoverRequest req = m_queue.dequeue();
        const quint64 generation = m_generation;
        ++m_active;

        auto attempt = std::make_shared<std::function<void(int)>>();
        *attempt = [this, req, attempt, generation](int index) {
            if (m_cancelling || generation != m_generation)
                return;
            if (index >= req.urls.size()) {
                qWarning() << "[Cover] Failed all URLs for bookId=" << req.bookId
                           << "label=" << req.label;
                ++m_totalDone;
                --m_active;
                emit coverFailed(req.bookId, "No working cover URL");
                emit downloadProgress(m_totalDone, m_totalQueued, req.label);
                if (m_active == 0 && m_queue.isEmpty() && m_totalDone >= m_totalQueued) {
                    m_totalDone = 0;
                    m_totalQueued = 0;
                    emit downloadsFinished();
                }
                processNext();
                return;
            }

            QNetworkRequest netReq{QUrl(req.urls.at(index))};
            netReq.setHeader(QNetworkRequest::UserAgentHeader, "EagleLibrary/1.0");
            qWarning().noquote() << "[Cover] Request bookId=" << req.bookId
                                 << "attempt=" << (index + 1)
                                 << "url=" << req.urls.at(index);
            QNetworkReply* reply = m_nam->get(netReq);
            m_activeReplies.insert(reply);

            connect(reply, &QNetworkReply::finished, this, [this, req, index, reply, attempt, generation]() {
                m_activeReplies.remove(reply);
                if (m_cancelling || generation != m_generation) {
                    reply->deleteLater();
                    return;
                }
                const QByteArray data = reply->error() == QNetworkReply::NoError ? reply->readAll() : QByteArray();
                const QString errorString = reply->errorString();
                reply->deleteLater();

                QBuffer buffer;
                buffer.setData(data);
                buffer.open(QIODevice::ReadOnly);

                QImageReader reader(&buffer);
                reader.setDecideFormatFromContent(true);
                if (!data.isEmpty() && reader.canRead()) {
                    QString ext = QStringLiteral(".") + QString::fromLatin1(reader.format()).toLower();
                    if (ext == ".jpeg")
                        ext = ".jpg";
                    if (ext == ".")
                        ext = ".jpg";

                    const QString path = m_saveDir + "/" + QString::number(req.bookId) + ext;
                    QFile file(path);
                    if (file.open(QIODevice::WriteOnly)) {
                        qWarning().noquote() << "[Cover] Saved bookId=" << req.bookId
                                             << "path=" << path
                                             << "bytes=" << data.size();
                        file.write(data);
                        file.close();
                        ++m_totalDone;
                        --m_active;
                        emit coverReady(req.bookId, path);
                        emit downloadProgress(m_totalDone, m_totalQueued, req.label);
                        if (m_active == 0 && m_queue.isEmpty() && m_totalDone >= m_totalQueued) {
                            m_totalDone = 0;
                            m_totalQueued = 0;
                            emit downloadsFinished();
                        }
                        processNext();
                        return;
                    }
                }

                qWarning().noquote() << "[Cover] Attempt failed bookId=" << req.bookId
                                     << "attempt=" << (index + 1)
                                     << "networkError=" << errorString
                                     << "bytes=" << data.size()
                                     << "readerCanRead=" << reader.canRead();

                (*attempt)(index + 1);
            });
        };

        (*attempt)(0);
    }
}
