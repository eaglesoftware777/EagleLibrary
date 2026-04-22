#pragma once
// ============================================================
//  Eagle Library — CoverDownloader.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include <QObject>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QDir>
#include <QStringList>
#include <QSet>

class QNetworkReply;

struct CoverRequest {
    qint64      bookId;
    QStringList urls;
    QString     label;
};

class CoverDownloader : public QObject
{
    Q_OBJECT
public:
    explicit CoverDownloader(const QString& saveDir, QObject* parent = nullptr);
    void enqueue(qint64 bookId, const QStringList& urls, const QString& label = {});
    void cancelAll();
    bool isRunning() const;

signals:
    void coverReady(qint64 bookId, QString localPath);
    void coverFailed(qint64 bookId, QString reason);
    void downloadProgress(int completed, int total, const QString& currentLabel);
    void downloadsFinished();

private:
    QNetworkAccessManager* m_nam;
    QQueue<CoverRequest>   m_queue;
    int                    m_active = 0;
    static constexpr int   MAX_CONCURRENT = 6;
    QString                m_saveDir;
    int                    m_totalQueued = 0;
    int                    m_totalDone   = 0;
    QSet<QNetworkReply*>   m_activeReplies;
    bool                   m_cancelling = false;
    quint64                m_generation = 0;

    void processNext();
};
