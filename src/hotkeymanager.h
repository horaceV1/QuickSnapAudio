#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QString>
#include <QMap>
#include <QObject>
#include <functional>

class HotkeyManager : public QObject {
    Q_OBJECT

public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager();

    bool registerHotkey(const QString &id, const QString &hotkeyStr, std::function<void()> callback);
    void unregisterHotkey(const QString &id);
    void unregisterAll();

    static QString hotkeyToString(int modifiers, int key);

private:
    struct HotkeyBinding {
        QString hotkeyStr;
        std::function<void()> callback;
        int platformId;
    };

    QMap<QString, HotkeyBinding> m_bindings;
    int m_nextPlatformId = 1;

#ifdef _WIN32
    class WinEventFilter;
    WinEventFilter *m_eventFilter = nullptr;
    friend class WinEventFilter;
#elif defined(__linux__)
    class LinuxHotkeyThread;
    LinuxHotkeyThread *m_thread = nullptr;
    friend class LinuxHotkeyThread;
#endif

    void onHotkeyTriggered(int platformId);

    static bool parseHotkeyString(const QString &str, int &modifiers, int &key);
};

#endif // HOTKEYMANAGER_H
