// ============================================================
//  Eagle Library — BookDelegate.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "BookDelegate.h"
#include "BookModel.h"
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QFileInfo>
#include <QImageReader>
#include <QLinearGradient>
#include <QFontMetrics>
#include <QApplication>

static const QHash<QString, QColor> FORMAT_COLORS = {
    {"PDF",  QColor(220,  60,  60)},
    {"EPUB", QColor( 60, 160,  60)},
    {"MOBI", QColor( 60,  60, 210)},
    {"AZW",  QColor(200, 120,  30)},
    {"AZW3", QColor(200, 120,  30)},
    {"DjVu", QColor(130,  60, 200)},
    {"CBZ",  QColor( 40, 170, 200)},
    {"CBR",  QColor( 40, 170, 200)},
    {"TXT",  QColor(110, 110, 110)},
    {"FB2",  QColor(200, 100, 100)},
};

QFont BookDelegate::uiFont(int pointSize, QFont::Weight weight) const
{
    QFont font = QApplication::font();
    font.setPointSize(pointSize);
    font.setWeight(weight);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

BookDelegate::BookDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void BookDelegate::setMetrics(int cardWidth, int cardHeight, int coverHeight, int listRowHeight, int padding)
{
    m_cardWidth = qMax(120, cardWidth);
    m_cardHeight = qMax(m_cardWidth + 40, cardHeight);
    m_coverHeight = qBound(100, coverHeight, m_cardHeight - 40);
    m_listRowHeight = qMax(52, listRowHeight);
    m_padding = qMax(6, padding);
    m_coverCache.clear();
}

QSize BookDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    if (m_gridMode) return {m_cardWidth + 12, m_cardHeight + 12};
    return {0, m_listRowHeight};
}

void BookDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                         const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    if (m_gridMode) paintGrid(painter, option, index);
    else            paintList(painter, option, index);
    painter->restore();
}

// NOTE: paintGrid and paintList declared inline below for single-file brevity
// ── Grid card ────────────────────────────────────────────────
void BookDelegate::paintGrid(QPainter* p, const QStyleOptionViewItem& opt,
                              const QModelIndex& idx) const
{
    QRect r = opt.rect.adjusted(6, 6, -6, -6);

    // ── Card shadow ──────────────────────────────────────────
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(0, 0, 0, 30));
    p->drawRoundedRect(r.adjusted(2, 4, 2, 4), 10, 10);

    // ── Card background ──────────────────────────────────────
    bool selected = opt.state & QStyle::State_Selected;
    bool hovered  = opt.state & QStyle::State_MouseOver;
    const bool isDocument = idx.data(CategoryRole).toString().compare("Document", Qt::CaseInsensitive) == 0;

    QLinearGradient cardGrad(r.topLeft(), r.bottomLeft());
    if (selected) {
        cardGrad.setColorAt(0, QColor(30, 55, 120));
        cardGrad.setColorAt(1, QColor(20, 40,  90));
    } else if (isDocument) {
        cardGrad.setColorAt(0, QColor(82, 44, 36));
        cardGrad.setColorAt(1, QColor(58, 30, 25));
    } else if (hovered) {
        cardGrad.setColorAt(0, QColor(40, 55,  90));
        cardGrad.setColorAt(1, QColor(25, 38,  70));
    } else {
        cardGrad.setColorAt(0, QColor(28, 38,  65));
        cardGrad.setColorAt(1, QColor(18, 26,  50));
    }
    p->setBrush(cardGrad);
    p->setPen(QPen(isDocument ? QColor(255, 189, 146, 70) : QColor(255, 255, 255, selected ? 60 : 20), 1));
    p->drawRoundedRect(r, 10, 10);

    // ── Cover image ──────────────────────────────────────────
    QRect coverRect(r.left() + 6, r.top() + 6, r.width() - 12, m_coverHeight);
    QString coverPath = idx.data(CoverPathRole).toString();
    QPixmap cover = loadCover(coverPath, coverRect.size());
    if (!cover.isNull()) {
        QPainterPath clipPath;
        clipPath.addRoundedRect(coverRect, 7, 7);
        p->setClipPath(clipPath);
        p->drawPixmap(coverRect, cover);
        p->setClipping(false);
    } else {
        QString format = idx.data(FormatRole).toString();
        QPixmap def = defaultCover(coverRect.size(), format);
        QPainterPath clipPath;
        clipPath.addRoundedRect(coverRect, 7, 7);
        p->setClipPath(clipPath);
        p->drawPixmap(coverRect, def);
        p->setClipping(false);
    }

    // Gradient overlay at bottom of cover for text readability
    QLinearGradient overlay(coverRect.bottomLeft(), coverRect.topLeft());
    overlay.setColorAt(0.0, QColor(0, 0, 0, 160));
    overlay.setColorAt(0.5, Qt::transparent);
    p->fillRect(coverRect, overlay);

    // ── Format badge ─────────────────────────────────────────
    QString format = idx.data(FormatRole).toString();
    drawFormatBadge(p, coverRect, format);
    if (isDocument) {
        QRect badge(coverRect.left() + 6, coverRect.bottom() - 24, 70, 18);
        p->setBrush(QColor(185, 95, 62, 220));
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(badge, 5, 5);
        p->setPen(QColor(255, 244, 232));
        p->setFont(uiFont(6, QFont::Bold));
        p->drawText(badge, Qt::AlignCenter, "DOCUMENT");
    }

    // ── Favourite star ───────────────────────────────────────
    drawFavStar(p, coverRect, idx.data(IsFavouriteRole).toBool());

    // ── Title ────────────────────────────────────────────────
    QRect textRect(r.left() + m_padding, r.top() + m_coverHeight + 10,
                   r.width() - m_padding * 2, 32);
    QFont titleFont = uiFont(8, QFont::Bold);
    p->setFont(titleFont);
    p->setPen(QColor(240, 235, 220));
    p->drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                idx.data(TitleRole).toString());

    // ── Author ───────────────────────────────────────────────
    QRect authorRect(r.left() + m_padding, r.top() + m_coverHeight + 42,
                     r.width() - m_padding * 2, 18);
    QFont authorFont = uiFont(7);
    p->setFont(authorFont);
    p->setPen(QColor(180, 155, 90));
    QString author = idx.data(AuthorRole).toString();
    QFontMetrics fm(authorFont);
    p->drawText(authorRect, Qt::AlignLeft | Qt::AlignVCenter,
                fm.elidedText(author, Qt::ElideRight, authorRect.width()));

    // ── Stars ────────────────────────────────────────────────
    QRectF starsRect(r.left() + m_padding, r.top() + m_coverHeight + 62,
                     r.width() - m_padding * 2, 12);
    drawStars(p, starsRect, idx.data(RatingRole).toDouble());
}

// ── List row ─────────────────────────────────────────────────
void BookDelegate::paintList(QPainter* p, const QStyleOptionViewItem& opt,
                              const QModelIndex& idx) const
{
    QRect r = opt.rect;
    bool selected = opt.state & QStyle::State_Selected;
    const bool isDocument = idx.data(CategoryRole).toString().compare("Document", Qt::CaseInsensitive) == 0;

    // Row background
    p->setPen(Qt::NoPen);
    if (selected)
        p->setBrush(QColor(40, 70, 160, 200));
    else if (isDocument)
        p->setBrush(QColor(96, 45, 35, 85));
    else if (opt.state & QStyle::State_MouseOver)
        p->setBrush(QColor(255, 255, 255, 12));
    else
        p->setBrush(Qt::transparent);
    p->drawRect(r);

    // Tiny cover thumbnail (42x55)
    QRect thumbRect(r.left() + 6, r.top() + 4, 38, 54);
    QString coverPath = idx.data(CoverPathRole).toString();
    QPixmap cover = loadCover(coverPath, thumbRect.size());
    if (!cover.isNull())
        p->drawPixmap(thumbRect, cover);
    else {
        p->setBrush(QColor(35, 50, 90));
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(thumbRect, 3, 3);
        QFont fmtFont = uiFont(6, QFont::Bold);
        p->setFont(fmtFont);
        p->setPen(QColor(180, 150, 60));
        p->drawText(thumbRect, Qt::AlignCenter, idx.data(FormatRole).toString());
    }

    // Title
    QFont titleFont = uiFont(9, QFont::Bold);
    p->setFont(titleFont);
    p->setPen(selected ? QColor(255,245,220) : QColor(220, 215, 200));
    QRect titleRect(r.left() + 52, r.top() + 6, r.width() - 200, 20);
    p->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                idx.data(TitleRole).toString());

    // Author
    QFont authorFont = uiFont(8);
    p->setFont(authorFont);
    p->setPen(QColor(180, 155, 90));
    QRect authorRect(r.left() + 52, r.top() + 26, r.width() - 200, 16);
    p->drawText(authorRect, Qt::AlignLeft | Qt::AlignVCenter,
                idx.data(AuthorRole).toString());

    // Format badge (right side)
    QRect fmtRect(r.right() - 110, r.top() + 8, 48, 18);
    drawFormatBadge(p, fmtRect, idx.data(FormatRole).toString());

    // Year
    p->setPen(QColor(140, 160, 200));
    QFont yearFont = uiFont(8);
    p->setFont(yearFont);
    int year = idx.data(YearRole).toInt();
    QRect yearRect(r.right() - 55, r.top(), 50, r.height());
    p->drawText(yearRect, Qt::AlignVCenter | Qt::AlignLeft,
                year > 0 ? QString::number(year) : "-");

    // Separator line
    p->setPen(QColor(255, 255, 255, 15));
    p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());

    if (isDocument) {
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(214, 124, 84));
        p->drawRoundedRect(QRect(r.left() + 46, r.top() + 34, 82, 14), 4, 4);
        p->setPen(QColor(255, 247, 238));
        p->setFont(uiFont(7, QFont::Bold));
        p->drawText(QRect(r.left() + 46, r.top() + 34, 82, 14), Qt::AlignCenter, "DOCUMENT");
    }
}

QPixmap BookDelegate::loadCover(const QString& path, const QSize& size) const
{
    if (path.isEmpty() || !QFileInfo::exists(path)) return {};
    QString key = QString("%1_%2x%3").arg(path).arg(size.width()).arg(size.height());
    if (QPixmap* cached = m_coverCache.object(key))
        return *cached;

    QImageReader reader(path);
    reader.setAutoTransform(true);
    if (reader.canRead()) {
        const QSize sourceSize = reader.size();
        if (sourceSize.isValid()) {
            QSize scaled = sourceSize;
            scaled.scale(size, Qt::KeepAspectRatioByExpanding);
            reader.setScaledSize(scaled);
        }
    }

    QImage image = reader.read();
    if (image.isNull()) return {};

    QPixmap pm = QPixmap::fromImage(image);
    if (pm.isNull()) return {};
    pm = pm.copy(0, 0, qMin(size.width(), pm.width()), qMin(size.height(), pm.height()));
    if (pm.size() != size)
        pm = pm.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
               .copy(0, 0, size.width(), size.height());

    m_coverCache.insert(key, new QPixmap(pm));
    return pm;
}

QPixmap BookDelegate::defaultCover(const QSize& size, const QString& format) const
{
    QString key = "default_" + format + QString::number(size.width());
    if (QPixmap* cached = m_coverCache.object(key))
        return *cached;

    QPixmap pm(size);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QColor col = FORMAT_COLORS.value(format, QColor(60, 75, 130));

    QLinearGradient g(0, 0, 0, size.height());
    g.setColorAt(0, col.lighter(140));
    g.setColorAt(1, col.darker(150));
    p.fillRect(pm.rect(), g);

    // Book spine decoration
    p.setPen(QColor(255, 255, 255, 40));
    p.drawLine(8, 0, 8, size.height());

    // Format text
    QFont f = uiFont(qMax(10, size.width() / 8), QFont::Bold);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, 200));
    p.drawText(pm.rect(), Qt::AlignCenter, format);

    // Eagle watermark
    QFont wf = uiFont(7);
    p.setFont(wf);
    p.setPen(QColor(255, 255, 255, 60));
    p.drawText(pm.rect().adjusted(0, 0, 0, -5),
               Qt::AlignBottom | Qt::AlignHCenter, "EAGLE");

    m_coverCache.insert(key, new QPixmap(pm));
    return pm;
}

void BookDelegate::drawStars(QPainter* p, const QRectF& rect, double rating) const
{
    double starSize = rect.height();
    for (int i = 0; i < 5; ++i) {
        QRectF sr(rect.left() + i * (starSize + 1), rect.top(), starSize, starSize);
        double fill = qBound(0.0, rating - i, 1.0);
        if (fill <= 0)
            p->setPen(QColor(100, 100, 120));
        else
            p->setPen(QColor(255, 200, 40));
        p->setBrush(fill > 0.5 ? QColor(255, 200, 40) : QColor(60, 65, 90));
        // Simple star polygon
        const double a = starSize / 2;
        QPointF pts[10];
        for (int k = 0; k < 5; ++k) {
            double angle = -M_PI / 2 + k * 2 * M_PI / 5;
            pts[2*k]   = sr.center() + QPointF(a * cos(angle),       a * sin(angle));
            pts[2*k+1] = sr.center() + QPointF(a * 0.4 * cos(angle + M_PI/5),
                                                 a * 0.4 * sin(angle + M_PI/5));
        }
        QPolygonF poly(QVector<QPointF>(pts, pts + 10));
        p->drawPolygon(poly);
    }
}

void BookDelegate::drawFormatBadge(QPainter* p, const QRect& rect, const QString& fmt) const
{
    if (fmt.isEmpty()) return;
    QColor col = FORMAT_COLORS.value(fmt, QColor(80, 90, 120));
    QRect badge(rect.right() - 38, rect.top() + 5, 34, 16);
    p->setBrush(col);
    p->setPen(Qt::NoPen);
    p->drawRoundedRect(badge, 4, 4);
    QFont f = uiFont(6, QFont::Bold);
    p->setFont(f);
    p->setPen(Qt::white);
    p->drawText(badge, Qt::AlignCenter, fmt.left(4));
}

void BookDelegate::drawFavStar(QPainter* p, const QRect& rect, bool isFav) const
{
    if (!isFav) return;
    QRect sr(rect.left() + 4, rect.top() + 4, 18, 18);
    p->setPen(QColor(255, 200, 40));
    p->setBrush(QColor(255, 200, 40));
    const double a = 9;
    QPointF ctr = sr.center();
    QPointF pts[10];
    for (int k = 0; k < 5; ++k) {
        double angle = -M_PI/2 + k * 2 * M_PI / 5;
        pts[2*k]   = ctr + QPointF(a * cos(angle),       a * sin(angle));
        pts[2*k+1] = ctr + QPointF(a * 0.4 * cos(angle + M_PI/5),
                                    a * 0.4 * sin(angle + M_PI/5));
    }
    p->drawPolygon(QPolygonF(QVector<QPointF>(pts, pts + 10)));
}
