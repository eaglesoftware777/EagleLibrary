// ============================================================
//  Eagle Library — ThumbnailCache.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================
#include "ThumbnailCache.h"
#include <QFileInfo>

ThumbnailCache& ThumbnailCache::instance()
{
    static ThumbnailCache tc;
    return tc;
}

QPixmap ThumbnailCache::get(const QString& path, const QSize& size) const
{
    if (path.isEmpty() || !QFileInfo::exists(path)) return {};
    QString key = path + QString("_%1x%2").arg(size.width()).arg(size.height());
    if (QPixmap* cached = m_cache.object(key))
        return *cached;
    QPixmap pm(path);
    if (pm.isNull()) return {};
    pm = pm.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_cache.insert(key, new QPixmap(pm));
    return pm;
}
