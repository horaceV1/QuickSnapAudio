#ifndef NOTIFICATIONPOPUP_H
#define NOTIFICATIONPOPUP_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class NotificationPopup : public QWidget
{
    Q_OBJECT

public:
    explicit NotificationPopup(QWidget *parent = nullptr);

    void showNotification(const QString &message, int durationMs = 2000);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void positionOnScreen();

    QLabel *m_label;
    QTimer *m_hideTimer;
    QGraphicsOpacityEffect *m_opacityEffect;
    QPropertyAnimation *m_fadeInAnim;
    QPropertyAnimation *m_fadeOutAnim;
    QPropertyAnimation *m_slideAnim;
};

#endif // NOTIFICATIONPOPUP_H
