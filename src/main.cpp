#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"
#include "trayicon.h"
#include "configmanager.h"
#include "hotkeymanager.h"
#include "audiodevicemanager.h"
#include "bluetoothmanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QuickSnapAudio");
    app.setApplicationVersion("1.0.6");
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
    BluetoothManager bluetoothManager;

    MainWindow mainWindow(&configManager, &audioManager, &hotkeyManager, &bluetoothManager);
    TrayIcon trayIcon(&mainWindow);
    mainWindow.setTrayIcon(&trayIcon);

    trayIcon.show();

    mainWindow.show();
    mainWindow.raise();
    mainWindow.activateWindow();

    return app.exec();
}
