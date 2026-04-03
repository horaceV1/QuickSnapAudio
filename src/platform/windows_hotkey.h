#ifndef WINDOWS_HOTKEY_H
#define WINDOWS_HOTKEY_H

#ifdef _WIN32

#include <QAbstractNativeEventFilter>
#include "../hotkeymanager.h"

namespace WindowsHotkey {
    bool registerHotkey(int id, int qtModifiers, int qtKey);
    void unregisterHotkey(int id);
}

class HotkeyManager::WinEventFilter : public QAbstractNativeEventFilter {
public:
    WinEventFilter(HotkeyManager *manager);
    ~WinEventFilter();

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    HotkeyManager *m_manager;
};

#endif // _WIN32
#endif // WINDOWS_HOTKEY_H
