#pragma once
// ============================================================
//  Eagle Library — BookDelegate.h
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include <QStyledItemDelegate>
#include <QCache>
#include <QPixmap>
#include <QSize>

class BookDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit BookDelegate(QObject* parent = nullptr);

    void  paint(QPainter* painter, const QStyleOptionViewItem& option,
                const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    void setGridMode(bool grid) { m_gridMode = grid; }

private:
    bool m_gridMode = true;

    mutable QCache<QString, QPixmap> m_coverCache{200}; // 200 items

    void    paintGrid(QPainter* p, const QStyleOptionViewItem& opt,
                      const QModelIndex& idx) const;
    void    paintList(QPainter* p, const QStyleOptionViewItem& opt,
                      const QModelIndex& idx) const;
    QPixmap loadCover(const QString& path, const QSize& size) const;
    QPixmap defaultCover(const QSize& size, const QString& format) const;
    void    drawStars(QPainter* p, const QRectF& rect, double rating) const;
    void    drawFormatBadge(QPainter* p, const QRect& rect, const QString& fmt) const;
    void    drawFavStar(QPainter* p, const QRect& rect, bool isFav) const;

    static constexpr int CARD_W   = 160;
    static constexpr int CARD_H   = 240;
    static constexpr int COVER_H  = 180;
    static constexpr int PADDING  = 8;
};
