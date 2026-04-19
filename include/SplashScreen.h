#pragma once
// ============================================================
//  Eagle Library — SplashScreen.h
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include <QSplashScreen>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QLinearGradient>

class SplashScreen : public QSplashScreen
{
    Q_OBJECT
public:
    explicit SplashScreen(QWidget* parent = nullptr);
    void setProgress(int value, const QString& message = {});

protected:
    void drawContents(QPainter* painter) override;
    void paintEvent(QPaintEvent* event) override;

private:
    int     m_progress = 0;
    QString m_message;
    QPixmap m_logo;

    void drawBackground(QPainter* p);
    void drawLogo(QPainter* p);
    void drawProgressBar(QPainter* p);
    void drawText(QPainter* p);
};
