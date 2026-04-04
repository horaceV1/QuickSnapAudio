#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>
#include <QMenu>

class MainWindow;
class UpdateChecker;

class TrayIcon : public QObject {
    Q_OBJECT
public:
    explicit TrayIcon(MainWindow *mainWindow, QObject *parent = nullptr);
    ~TrayIcon();

    void show();
    void showSwitchedNotification(const QString &deviceName);

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;
    MainWindow *m_mainWindow;
    UpdateChecker *m_updateChecker;
};

#endif // TRAYICON_H
