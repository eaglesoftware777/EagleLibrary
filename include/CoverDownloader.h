#pragma once
// ============================================================
//  Eagle Library — CoverDownloader.h
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================
#include <QObject>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QDir>

struct CoverRequest { qint64 bookId; QString url; };

class CoverDownloader : public QObject
{
    Q_OBJECT
public:
    explicit CoverDownloader(const QString& saveDir, QObject* parent = nullptr);
    void enqueue(qint64 bookId, const QString& url);

signals:
    void coverReady(qint64 bookId, QString localPath);

private:
    QNetworkAccessManager* m_nam;
    QQueue<CoverRequest>   m_queue;
    int                    m_active = 0;
    static constexpr int   MAX_CONCURRENT = 6;
    QString                m_saveDir;

    void processNext();
};
