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
    app.setApplicationVersion("1.0.3");
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

    // Register saved hotkeys on startup
    auto entries = configManager.loadEntries();
    for (const auto &entry : entries) {
        if (!entry.hotkey.isEmpty()) {
            hotkeyManager.registerHotkey(entry.id, entry.hotkey, [&audioManager, &trayIcon, entry]() {
                audioManager.setDefaultDevice(entry.deviceId, entry.isOutput);
                trayIcon.showSwitchedNotification(entry.deviceName);
            });
        }
    }

    return app.exec();
}
