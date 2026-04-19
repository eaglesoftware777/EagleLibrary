// ============================================================
//  Eagle Library — ThumbnailCache.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "ThumbnailCache.h"
#include <QFileInfo>
#include <QImageReader>

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
    QImageReader reader(path);
    reader.setAutoTransform(true);
    if (reader.canRead()) {
        const QSize sourceSize = reader.size();
        if (sourceSize.isValid()) {
            QSize scaled = sourceSize;
            scaled.scale(size, Qt::KeepAspectRatio);
            reader.setScaledSize(scaled);
        }
    }
    const QImage image = reader.read();
    if (image.isNull()) return {};
    QPixmap pm = QPixmap::fromImage(image);
    if (pm.isNull()) return {};
    m_cache.insert(key, new QPixmap(pm));
    return pm;
}
