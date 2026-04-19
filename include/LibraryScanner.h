#pragma once
// ============================================================
//  Eagle Library -- LibraryScanner.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "Book.h"
#include <QObject>
#include <QThread>
#include <QStringList>
#include <QAtomicInt>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QThreadPool>
#include <QRunnable>

// ── Fast parallel scanner ─────────────────────────────────────
// Uses a producer thread to walk directories and N consumer
// threads (QThreadPool) to hash + insert files concurrently.

class ScanWorker : public QObject
{
    Q_OBJECT
public:
    explicit ScanWorker(const QStringList& folders,
                        int parallelism,
                        bool fastScanMode,
                        QObject* parent = nullptr);
    void cancel() { m_cancelled.storeRelaxed(1); }

public slots:
    void run();

signals:
    void bookFound(Book book);
    void progress(int done, int total, const QString& currentFile);
    void finished(int added, int skipped);
    void error(const QString& msg);

private:
    QStringList m_folders;
    int         m_parallelism;
    bool        m_fastScanMode = true;
    QAtomicInt  m_cancelled{0};

    // shared counters (atomic)
    QAtomicInt  m_done{0};
    QAtomicInt  m_added{0};
    QAtomicInt  m_skipped{0};

    QString computeHash(const QString& filePath) const;
    Book    buildBook(const QString& filePath) const;
    QString detectFormat(const QString& ext) const;

    // thread-safe DB helpers (each call gets its own connection)
    bool    dbBookExists(const QString& path,  const QString& conn) const;
    bool    dbHashExists(const QString& hash,  const QString& conn) const;
    qint64  dbInsertBook(const Book& b,        const QString& conn) const;
    bool    openThreadDb(const QString& conn)  const;
};

// ── Controller ────────────────────────────────────────────────
class LibraryScanner : public QObject
{
    Q_OBJECT
public:
    explicit LibraryScanner(QObject* parent = nullptr);
    ~LibraryScanner();

    void startScan(const QStringList& folders, int parallelism = 0, bool fastScanMode = true);
    void cancel();
    bool isRunning() const;

signals:
    void bookFound(Book book);
    void progress(int done, int total, const QString& currentFile);
    void scanFinished(int added, int skipped);
    void scanError(const QString& msg);

private:
    QThread*    m_thread  = nullptr;
    ScanWorker* m_worker  = nullptr;
};

// ── Smart Renamer ─────────────────────────────────────────────
struct RenameResult {
    qint64  bookId;
    QString oldTitle;
    QString newTitle;
    QString newAuthor;
    QString newPublisher;
    int     newYear = 0;
    bool    changed = false;
};

class SmartRenamer : public QObject
{
    Q_OBJECT
public:
    explicit SmartRenamer(QObject* parent = nullptr);

    // Analyse a filename/path and extract structured metadata
    static RenameResult analyseBook(const Book& book);

    // Batch rename all books with missing/weak metadata
    void renameAll(const QVector<Book>& books);
    void cancel() { m_cancelled.storeRelaxed(1); }

signals:
    void renamed(RenameResult result);
    void progress(int done, int total, const QString& currentFile, const QString& detail);
    void finished(int changed);

private:
    QAtomicInt m_cancelled{0};

    static QString cleanToken(const QString& s);
    static bool    isWeakTitle(const QString& title, const QString& filePath);
    static RenameResult parseFilename(const Book& book);
    static QString inferTitleFromContent(const Book& book);
};
