#pragma once
// ============================================================
//  Eagle Library — BookDelegate.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include <QStyledItemDelegate>
#include <QCache>
#include <QFont>
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
    void setMetrics(int cardWidth, int cardHeight, int coverHeight, int listRowHeight, int padding);

private:
    bool m_gridMode = true;
    int  m_cardWidth = 160;
    int  m_cardHeight = 240;
    int  m_coverHeight = 180;
    int  m_listRowHeight = 62;
    int  m_padding = 8;

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
    QFont   uiFont(int pointSize, QFont::Weight weight = QFont::Normal) const;
};
