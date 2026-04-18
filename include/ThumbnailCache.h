#pragma once
// ============================================================
//  Eagle Library — ThumbnailCache.h
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================
#include <QObject>
#include <QCache>
#include <QPixmap>
#include <QString>

class ThumbnailCache : public QObject
{
    Q_OBJECT
public:
    static ThumbnailCache& instance();

    QPixmap get(const QString& path, const QSize& size) const;
    void    clear() { m_cache.clear(); }

private:
    ThumbnailCache() = default;
    mutable QCache<QString, QPixmap> m_cache{500};
};
