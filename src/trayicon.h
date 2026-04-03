#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>
#include <QMenu>

class MainWindow;

class TrayIcon : public QObject {
    Q_OBJECT
public:
    explicit TrayIcon(MainWindow *mainWindow, QObject *parent = nullptr);
    ~TrayIcon();

    void show();

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;
    MainWindow *m_mainWindow;
};

#endif // TRAYICON_H
