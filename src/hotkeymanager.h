#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QString>
#include <QMap>
#include <QObject>
#include <QTimer>
#include <functional>

class HotkeyManager : public QObject {
    Q_OBJECT

public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager();

    using HotkeyCallback = std::function<void()>;

    // Register a hotkey with an optional long-press handler.
    //   shortPress:   fired on a normal tap (key released before longPressMs)
    //   longPress:    fired once the key has been held for >= longPressMs
    //   longPressMs:  hold duration that qualifies as a long press
    // If longPress is null, shortPress is dispatched immediately on key-down
    // and no polling is performed (original behaviour).
    bool registerHotkey(const QString &id,
                        const QString &hotkeyStr,
                        HotkeyCallback shortPress,
                        HotkeyCallback longPress = nullptr,
                        int longPressMs = 1200);
    void unregisterHotkey(const QString &id);
    void unregisterAll();

    static QString hotkeyToString(int modifiers, int key);

private:
    struct HotkeyBinding {
        QString hotkeyStr;
        HotkeyCallback shortPress;
        HotkeyCallback longPress;
        int platformId = 0;
        int qtModifiers = 0;
        int qtKey = 0;
        int longPressMs = 0;

        // Long-press tracking state
        QTimer *pollTimer = nullptr;
        qint64 pressStartedMs = 0;
        bool longFired = false;
    };

    QMap<QString, HotkeyBinding> m_bindings;
    int m_nextPlatformId = 1;

public:
#ifdef _WIN32
    class WinEventFilter;
    friend class WinEventFilter;
private:
    WinEventFilter *m_eventFilter = nullptr;
#elif defined(__linux__)
    class LinuxHotkeyThread;
    friend class LinuxHotkeyThread;
private:
    LinuxHotkeyThread *m_thread = nullptr;
#endif

    void onHotkeyTriggered(int platformId);
    void startLongPressTracking(const QString &id);
    bool isKeyStillDown(int qtKey) const;

    static bool parseHotkeyString(const QString &str, int &modifiers, int &key);
};

#endif // HOTKEYMANAGER_H
