#ifndef LINUX_HOTKEY_H
#define LINUX_HOTKEY_H

#ifdef __linux__

#include <QThread>
#include <QMap>
#include <QMutex>
#include "../hotkeymanager.h"

struct LinuxHotkeyBinding {
    int platformId;
    unsigned int x11Mod;
    unsigned int x11Keycode;
};

class HotkeyManager::LinuxHotkeyThread : public QThread {
    Q_OBJECT
public:
    LinuxHotkeyThread(HotkeyManager *manager);
    void stop();

    bool addHotkey(int platformId, unsigned int mod, unsigned int keycode);
    void removeHotkey(int platformId);

protected:
    void run() override;

private:
    HotkeyManager *m_manager;
    bool m_running;
    QMutex m_mutex;
    QVector<LinuxHotkeyBinding> m_hotkeys;
    void *m_display = nullptr; // X Display*
};

namespace LinuxHotkey {
    bool registerHotkey(HotkeyManager::LinuxHotkeyThread *thread, int platformId, int qtModifiers, int qtKey);
    void unregisterHotkey(HotkeyManager::LinuxHotkeyThread *thread, int platformId);
    bool isKeyDown(int qtKey);
}

#endif // __linux__
#endif // LINUX_HOTKEY_H
