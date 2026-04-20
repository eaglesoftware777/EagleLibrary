#pragma once
// ============================================================
//  Eagle Library — MetadataFetcher.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "Book.h"
#include <QObject>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QHash>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QSet>

struct FetchRequest {
    qint64  bookId;
    QString title;
    QString author;
    QString isbn;
    QString filePath;
    QString format;
    bool    useEmbedded    = true;
    bool    useGoogle      = true;
    bool    useOpenLibrary = true;
};

// One search result candidate. embedded=true means it came from the file itself.
struct Candidate {
    Book        book;
    QStringList coverUrls;
    int         score    = 0;
    bool        embedded = false;
};

class MetadataFetcher : public QObject
{
    Q_OBJECT
public:
    explicit MetadataFetcher(QObject* parent = nullptr);

    void enqueue(const FetchRequest& req);
    void setMaxConcurrent(int n) { m_maxConcurrent = n; }
    void cancelAll();

signals:
    void metadataReady(qint64 bookId, Book updatedBook);
    void coverUrlsReady(qint64 bookId, const QStringList& urls);
    void fetchError(qint64 bookId, const QString& msg);
    void fetchProgress(int completed, int total, const QString& currentFile, const QString& stage);

private slots:
    void processQueue();
    void onGoogleReply(QNetworkReply* reply, qint64 bookId);
    void onOpenLibraryReply(QNetworkReply* reply, qint64 bookId);

private:
    QNetworkAccessManager*          m_nam;
    QQueue<FetchRequest>            m_queue;
    int                             m_active        = 0;
    int                             m_maxConcurrent = 4;
    QTimer*                         m_processTimer;
    int                             m_totalQueued   = 0;
    int                             m_totalDone     = 0;
    QHash<qint64, FetchRequest>     m_requests;
    QHash<qint64, QVector<Candidate>> m_candidates;
    QHash<qint64, int>              m_pendingReplies;
    QSet<QNetworkReply*>            m_activeReplies;
    bool                            m_cancelling = false;
    quint64                         m_generation = 0;

    void fetchFromSources(const FetchRequest& req);
    void fetchFromGoogle(const FetchRequest& req);
    void fetchFromOpenLibrary(const FetchRequest& req);
    void finalize(qint64 bookId);
    Book parseGoogleVolume(const QJsonObject& volume) const;
    Book parseOpenLibraryDoc(const QJsonObject& doc) const;
    QString buildGoogleQuery(const FetchRequest& req) const;
    QString buildOpenLibraryQuery(const FetchRequest& req) const;
};
