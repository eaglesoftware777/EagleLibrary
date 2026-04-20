// ============================================================
//  Eagle Library — SplashScreen.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "SplashScreen.h"
#include "AppConfig.h"
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QScreen>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QImage>
#include <QColor>

static const int W = 700;
static const int H = 420;

SplashScreen::SplashScreen(QWidget* parent)
    : QSplashScreen(QPixmap(W, H), Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint)
{
    Q_UNUSED(parent)
    setFixedSize(W, H);

    // Center on primary screen
    if (QScreen* s = QApplication::primaryScreen()) {
        QRect sg = s->geometry();
        move(sg.center() - QPoint(W / 2, H / 2));
    }

    // Initial paint
    QPixmap pm(W, H);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    setPixmap(pm);

    // Use the vectorial eagle mark — renders cleanly at any size on the dark background
    const QIcon markIcon(QStringLiteral(":/eagle_mark.svg"));
    if (!markIcon.isNull())
        m_logo = markIcon.pixmap(200, 200);
}

void SplashScreen::setProgress(int value, const QString& message)
{
    m_progress = qBound(0, value, 100);
    m_message  = message;
    repaint();
}

void SplashScreen::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    drawBackground(&p);
    drawLogo(&p);
    drawProgressBar(&p);
    drawText(&p);
}

void SplashScreen::drawContents(QPainter*) {}  // override to suppress default

// ── Background ────────────────────────────────────────────────────────────────
void SplashScreen::drawBackground(QPainter* p)
{
    // Deep navy gradient
    QLinearGradient bg(0, 0, W, H);
    bg.setColorAt(0.0, QColor(10, 18, 42));
    bg.setColorAt(0.5, QColor(18, 32, 70));
    bg.setColorAt(1.0, QColor(8,  14, 32));
    p->fillRect(rect(), bg);

    // Subtle radial glow top-center (gold accent)
    QRadialGradient glow(W / 2, 0, 350);
    glow.setColorAt(0.0, QColor(200, 160, 60, 60));
    glow.setColorAt(1.0, Qt::transparent);
    p->fillRect(rect(), glow);

    // Thin gold border
    p->setPen(QPen(QColor(180, 140, 50), 1.5));
    p->drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);

    // Inner subtle border
    p->setPen(QPen(QColor(255, 255, 255, 18), 1));
    p->drawRoundedRect(rect().adjusted(4, 4, -4, -4), 10, 10);

    // Decorative horizontal line
    p->setPen(QPen(QColor(180, 140, 50, 80), 1));
    p->drawLine(40, 280, W - 40, 280);
}

// ── Eagle Mark — vectorial SVG logo ──────────────────────────────────────────
void SplashScreen::drawLogo(QPainter* p)
{
    // Tight square target area; the SVG mark renders well on the dark background
    const int markSize = 148;
    const QRect target((W - markSize) / 2, 22, markSize, markSize);

    // Radial gold glow behind the mark
    QRadialGradient glow(target.center(), 110);
    glow.setColorAt(0.0, QColor(210, 165, 55, 70));
    glow.setColorAt(1.0, Qt::transparent);
    p->fillRect(target.adjusted(-30, -20, 30, 20), glow);

    // Dark navy panel with gold border — matches overall splash theme
    p->save();
    p->setPen(QPen(QColor(210, 170, 70, 200), 1.5));
    p->setBrush(QColor(12, 22, 52, 220));
    p->drawRoundedRect(target.adjusted(-8, -6, 8, 6), 20, 20);
    p->restore();

    if (!m_logo.isNull()) {
        const QPixmap scaled = m_logo.scaled(target.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPoint topLeft((W - scaled.width()) / 2, target.y() + (target.height() - scaled.height()) / 2);
        p->drawPixmap(topLeft, scaled);
    }

    // ── App name ─────────────────────────────────────────────
    p->save();
    QFont nameFont = QApplication::font();
    nameFont.setPointSize(34);
    nameFont.setBold(true);
    p->setFont(nameFont);

    // Shadow
    p->setPen(QColor(0, 0, 0, 100));
    p->drawText(QRect(0, 196, W, 52), Qt::AlignHCenter | Qt::AlignVCenter,
                "EAGLE LIBRARY");

    // Gold gradient text
    QLinearGradient textGrad(0, 190, 0, 232);
    textGrad.setColorAt(0.0, QColor(255, 232, 110));
    textGrad.setColorAt(0.5, QColor(220, 175, 50));
    textGrad.setColorAt(1.0, QColor(175, 125, 18));
    p->setPen(QPen(QBrush(textGrad), 1));
    p->drawText(QRect(0, 194, W, 52), Qt::AlignHCenter | Qt::AlignVCenter,
                "EAGLE LIBRARY");
    p->restore();

    // ── Tagline ───────────────────────────────────────────────
    p->save();
    QFont tagFont = QApplication::font();
    tagFont.setPointSize(10);
    tagFont.setWeight(QFont::Normal);
    tagFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.4);
    p->setFont(tagFont);
    p->setPen(QColor(196, 178, 126, 218));
    p->drawText(QRect(0, 244, W, 24), Qt::AlignHCenter | Qt::AlignVCenter,
                "PROFESSIONAL eBOOK LIBRARY MANAGER");
    p->restore();

    p->save();
    QFont companyFont = QApplication::font();
    companyFont.setPointSize(11);
    companyFont.setWeight(QFont::DemiBold);
    p->setFont(companyFont);
    p->setPen(QColor(154, 192, 236, 228));
    p->drawText(QRect(0, 264, W, 20), Qt::AlignHCenter | Qt::AlignVCenter,
                "Eagle Software");
    p->restore();
}

// ── Progress Bar ─────────────────────────────────────────────────────────────
void SplashScreen::drawProgressBar(QPainter* p)
{
    const int barX = 68, barY = 350, barW = W - 136, barH = 10;
    const int radius = 5;

    // Track
    p->setBrush(QColor(21, 34, 66));
    p->setPen(QPen(QColor(72, 96, 142, 120), 1));
    p->drawRoundedRect(barX, barY, barW, barH, radius, radius);

    // Fill
    int fillW = qMax(radius * 2, int(barW * m_progress / 100.0));
    QLinearGradient fillGrad(barX, barY, barX + fillW, barY);
    fillGrad.setColorAt(0.0, QColor(205, 78, 54));
    fillGrad.setColorAt(0.55, QColor(231, 174, 75));
    fillGrad.setColorAt(1.0, QColor(248, 227, 126));
    p->setBrush(fillGrad);
    p->setPen(Qt::NoPen);
    p->drawRoundedRect(barX, barY, fillW, barH, radius, radius);

    // Glow on tip
    QRadialGradient glowTip(barX + fillW, barY + barH / 2, 13);
    glowTip.setColorAt(0.0, QColor(255, 228, 120, 170));
    glowTip.setColorAt(1.0, Qt::transparent);
    p->setBrush(glowTip);
    p->drawEllipse(barX + fillW - 12, barY - 8, 24, 26);
}

// ── Text (status + copyright) ────────────────────────────────────────────────
void SplashScreen::drawText(QPainter* p)
{
    // Status message
    p->save();
    QFont statusFont = QApplication::font();
    statusFont.setPointSize(10);
    p->setFont(statusFont);
    p->setPen(QColor(198, 214, 238, 228));
    p->drawText(QRect(68, 368, W - 136, 24), Qt::AlignLeft | Qt::AlignVCenter,
                m_message.isEmpty() ? "Initializing..." : m_message);

    // Progress %
    p->setPen(QColor(244, 214, 122, 232));
    p->drawText(QRect(68, 368, W - 136, 24), Qt::AlignRight | Qt::AlignVCenter,
                QString("%1%").arg(m_progress));
    p->restore();

    // Copyright
    p->save();
    QFont copyrightFont = QApplication::font();
    copyrightFont.setPointSize(8);
    p->setFont(copyrightFont);
    p->setPen(QColor(122, 145, 184, 168));
    p->drawText(QRect(0, 395, W, 20), Qt::AlignHCenter | Qt::AlignVCenter,
                AppConfig::copyright());
    p->restore();

    // Version
    p->save();
    QFont verFont = QApplication::font();
    verFont.setPointSize(8);
    p->setFont(verFont);
    p->setPen(QColor(120, 138, 176, 150));
    p->drawText(QRect(68, 300, W - 136, 20), Qt::AlignRight | Qt::AlignVCenter,
                QString("v%1").arg(AppConfig::version()));
    p->restore();
}
