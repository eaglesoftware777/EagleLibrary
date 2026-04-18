#pragma once
// ============================================================
//  Eagle Library — MetadataFetcher.h
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include "Book.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QTimer>

struct FetchRequest {
    qint64  bookId;
    QString title;
    QString author;
    QString isbn;
};

class MetadataFetcher : public QObject
{
    Q_OBJECT
public:
    explicit MetadataFetcher(QObject* parent = nullptr);

    void enqueue(const FetchRequest& req);
    void setMaxConcurrent(int n) { m_maxConcurrent = n; }

signals:
    void metadataReady(qint64 bookId, Book updatedBook);
    void coverUrlReady(qint64 bookId, const QString& url);
    void fetchError(qint64 bookId, const QString& msg);

private slots:
    void processQueue();
    void onGoogleReply(QNetworkReply* reply, qint64 bookId);

private:
    QNetworkAccessManager* m_nam;
    QQueue<FetchRequest>   m_queue;
    int                    m_active        = 0;
    int                    m_maxConcurrent = 4;
    QTimer*                m_processTimer;

    void fetchFromGoogle(const FetchRequest& req);
    Book parseGoogleVolume(const QJsonObject& volume) const;
    QString buildGoogleQuery(const FetchRequest& req) const;
};
