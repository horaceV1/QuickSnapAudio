#include "notificationpopup.h"

#include <QApplication>
#include <QScreen>
#include <QHBoxLayout>
#include <QFont>
#include <QPainter>
#include <QPainterPath>

NotificationPopup::NotificationPopup(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::ToolTip | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose, false);

    // Opacity effect for fade animations
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);

    // Label
    m_label = new QLabel(this);
    m_label->setStyleSheet(
        "QLabel {"
        "  color: #ffffff;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 12px 24px;"
        "}"
    );
    m_label->setAlignment(Qt::AlignCenter);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_label);
    setLayout(layout);

    setStyleSheet(
        "NotificationPopup {"
        "  background-color: #89b4fa;"
        "  border-radius: 10px;"
        "}"
    );

    // Timers and animations
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        // Start fade-out
        m_fadeOutAnim->start();
    });

    m_fadeInAnim = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeInAnim->setDuration(150);
    m_fadeInAnim->setStartValue(0.0);
    m_fadeInAnim->setEndValue(1.0);

    m_fadeOutAnim = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeOutAnim->setDuration(300);
    m_fadeOutAnim->setStartValue(1.0);
    m_fadeOutAnim->setEndValue(0.0);
    connect(m_fadeOutAnim, &QPropertyAnimation::finished, this, [this]() {
        hide();
    });

    m_slideAnim = new QPropertyAnimation(this, "pos", this);
    m_slideAnim->setDuration(150);
    m_slideAnim->setEasingCurve(QEasingCurve::OutCubic);

    setFixedHeight(48);
}

void NotificationPopup::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);

    painter.fillPath(path, QColor("#89b4fa"));
}

void NotificationPopup::showNotification(const QString &message, int durationMs)
{
    // Stop any running animations
    m_hideTimer->stop();
    m_fadeInAnim->stop();
    m_fadeOutAnim->stop();
    m_slideAnim->stop();

    m_label->setText(message);
    adjustSize();

    // Ensure minimum width
    int minWidth = m_label->sizeHint().width() + 20;
    if (minWidth < 280) minWidth = 280;
    setFixedWidth(minWidth);

    positionOnScreen();

    m_opacityEffect->setOpacity(0.0);
    show();
    raise();

    // Slide up from below
    QPoint finalPos = pos();
    QPoint startPos = finalPos + QPoint(0, 20);
    m_slideAnim->setStartValue(startPos);
    m_slideAnim->setEndValue(finalPos);
    m_slideAnim->start();

    m_fadeInAnim->start();
    m_hideTimer->start(durationMs);
}

void NotificationPopup::positionOnScreen()
{
    QScreen *screen = QApplication::primaryScreen();
    if (!screen) return;

    QRect screenGeo = screen->availableGeometry();

    // Position at bottom-right, above the taskbar, with some padding
    int x = screenGeo.right() - width() - 20;
    int y = screenGeo.bottom() - height() - 20;

    move(x, y);
}
