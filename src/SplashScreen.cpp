// ============================================================
//  Eagle Library — SplashScreen.cpp
//  Copyright (c) 2024 Eagle Software. All rights reserved.
// ============================================================

#include "SplashScreen.h"
#include "AppConfig.h"
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QCoreApplication>
#include <QScreen>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QRadialGradient>

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

// ── Eagle Logo — real company logo from resources ─────────────────────────────
void SplashScreen::drawLogo(QPainter* p)
{
    // Load company logo from Qt resources
    QPixmap logo(":/eagle_256.png");
    if (logo.isNull()) {
        // Fallback: try loading from same directory as executable
        logo.load(QCoreApplication::applicationDirPath() + "/eagle_256.png");
    }

    if (!logo.isNull()) {
        // Render the real logo — scale to fit nicely, keep aspect ratio
        const int logoSize = 140;
        QPixmap scaled = logo.scaled(logoSize, logoSize,
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);

        // Center horizontally, position in upper portion of splash
        int x = (W - scaled.width())  / 2;
        int y = 22;

        // Soft glow behind logo
        QRadialGradient glow(W / 2, y + scaled.height() / 2, logoSize);
        glow.setColorAt(0.0, QColor(180, 200, 255, 55));
        glow.setColorAt(1.0, Qt::transparent);
        p->fillRect(QRect(x - 20, y - 10, scaled.width() + 40, scaled.height() + 20), glow);

        p->drawPixmap(x, y, scaled);
    } else {
        // Fallback vector eagle if image not found (keeps app working during dev)
        p->save();
        QPainterPath eagle;
        eagle.addEllipse(QRectF(310, 80, 80, 50));
        eagle.addEllipse(QRectF(370, 68, 30, 28));
        QLinearGradient goldGrad(0, 68, 0, 165);
        goldGrad.setColorAt(0.0, QColor(200, 210, 255));
        goldGrad.setColorAt(1.0, QColor(80, 100, 200));
        p->fillPath(eagle, goldGrad);
        p->restore();
    }

    // ── App name ─────────────────────────────────────────────
    p->save();
    QFont nameFont("Georgia", 34, QFont::Bold);
    p->setFont(nameFont);

    // Shadow
    p->setPen(QColor(0, 0, 0, 100));
    p->drawText(QRect(0, 187, W, 52), Qt::AlignHCenter | Qt::AlignVCenter,
                "EAGLE LIBRARY");

    // Gold gradient text
    QLinearGradient textGrad(0, 190, 0, 232);
    textGrad.setColorAt(0.0, QColor(255, 232, 110));
    textGrad.setColorAt(0.5, QColor(220, 175, 50));
    textGrad.setColorAt(1.0, QColor(175, 125, 18));
    p->setPen(QPen(QBrush(textGrad), 1));
    p->drawText(QRect(0, 185, W, 52), Qt::AlignHCenter | Qt::AlignVCenter,
                "EAGLE LIBRARY");
    p->restore();

    // ── Tagline ───────────────────────────────────────────────
    p->save();
    QFont tagFont("Arial", 10, QFont::Normal);
    tagFont.setLetterSpacing(QFont::AbsoluteSpacing, 3.5);
    p->setFont(tagFont);
    p->setPen(QColor(180, 160, 100, 200));
    p->drawText(QRect(0, 238, W, 24), Qt::AlignHCenter | Qt::AlignVCenter,
                "PROFESSIONAL eBOOK LIBRARY MANAGER");
    p->restore();
}

// ── Progress Bar ─────────────────────────────────────────────────────────────
void SplashScreen::drawProgressBar(QPainter* p)
{
    const int barX = 60, barY = 355, barW = W - 120, barH = 6;
    const int radius = 3;

    // Track
    p->setBrush(QColor(30, 45, 90));
    p->setPen(Qt::NoPen);
    p->drawRoundedRect(barX, barY, barW, barH, radius, radius);

    // Fill
    int fillW = qMax(radius * 2, int(barW * m_progress / 100.0));
    QLinearGradient fillGrad(barX, barY, barX + fillW, barY);
    fillGrad.setColorAt(0.0, QColor(180, 130, 30));
    fillGrad.setColorAt(1.0, QColor(255, 215, 80));
    p->setBrush(fillGrad);
    p->drawRoundedRect(barX, barY, fillW, barH, radius, radius);

    // Glow on tip
    QRadialGradient glowTip(barX + fillW, barY + barH / 2, 10);
    glowTip.setColorAt(0.0, QColor(255, 215, 80, 180));
    glowTip.setColorAt(1.0, Qt::transparent);
    p->setBrush(glowTip);
    p->drawEllipse(barX + fillW - 10, barY - 8, 20, 22);
}

// ── Text (status + copyright) ────────────────────────────────────────────────
void SplashScreen::drawText(QPainter* p)
{
    // Status message
    p->save();
    QFont statusFont("Arial", 9);
    p->setFont(statusFont);
    p->setPen(QColor(160, 185, 220, 220));
    p->drawText(QRect(60, 368, W - 120, 22), Qt::AlignLeft | Qt::AlignVCenter,
                m_message.isEmpty() ? "Initializing…" : m_message);

    // Progress %
    p->setPen(QColor(200, 175, 80, 220));
    p->drawText(QRect(60, 368, W - 120, 22), Qt::AlignRight | Qt::AlignVCenter,
                QString("%1%").arg(m_progress));
    p->restore();

    // Copyright
    p->save();
    QFont copyrightFont("Arial", 8);
    p->setFont(copyrightFont);
    p->setPen(QColor(120, 140, 180, 160));
    p->drawText(QRect(0, 395, W, 20), Qt::AlignHCenter | Qt::AlignVCenter,
                AppConfig::copyright());
    p->restore();

    // Version
    p->save();
    QFont verFont("Arial", 8);
    p->setFont(verFont);
    p->setPen(QColor(100, 120, 160, 130));
    p->drawText(QRect(60, 295, W - 120, 20), Qt::AlignRight | Qt::AlignVCenter,
                QString("v%1").arg(AppConfig::version()));
    p->restore();
}
