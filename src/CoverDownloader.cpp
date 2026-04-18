// ============================================================
//  Eagle Library — CoverDownloader.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================
#include "CoverDownloader.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QDebug>

CoverDownloader::CoverDownloader(const QString& saveDir, QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_saveDir(saveDir)
{
    QDir().mkpath(saveDir);
}

void CoverDownloader::enqueue(qint64 bookId, const QString& url)
{
    m_queue.enqueue({bookId, url});
    processNext();
}

void CoverDownloader::processNext()
{
    while (!m_queue.isEmpty() && m_active < MAX_CONCURRENT) {
        CoverRequest req = m_queue.dequeue();
        ++m_active;

        QNetworkRequest netReq(QUrl(req.url));
        netReq.setHeader(QNetworkRequest::UserAgentHeader,
                         "EagleLibrary/1.0");
        QNetworkReply* reply = m_nam->get(netReq);
        qint64 id = req.bookId;

        connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
            --m_active;
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QString ext  = ".jpg";
                QString ct   = reply->header(QNetworkRequest::ContentTypeHeader).toString();
                if (ct.contains("png")) ext = ".png";
                QString path = m_saveDir + "/" + QString::number(id) + ext;
                QFile f(path);
                if (f.open(QIODevice::WriteOnly)) {
                    f.write(data);
                    f.close();
                    emit coverReady(id, path);
                }
            }
            reply->deleteLater();
            processNext();
        });
    }
}
