#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"
#include "trayicon.h"
#include "configmanager.h"
#include "hotkeymanager.h"
#include "audiodevicemanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QuickSnapAudio");
    app.setApplicationVersion("1.0.5");
    app.setOrganizationName("QuickSnapAudio");
    app.setQuitOnLastWindowClosed(false);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "QuickSnapAudio",
                              "System tray is not available on this system.");
        return 1;
    }

    ConfigManager configManager;
    AudioDeviceManager audioManager;
    HotkeyManager hotkeyManager;

    MainWindow mainWindow(&configManager, &audioManager, &hotkeyManager);
    TrayIcon trayIcon(&mainWindow);
    mainWindow.setTrayIcon(&trayIcon);

    trayIcon.show();

    // Show the main window on startup so the user knows the app launched.
    // The window will hide to tray on close or minimize.
    mainWindow.show();
    mainWindow.raise();
    mainWindow.activateWindow();

    return app.exec();
}
