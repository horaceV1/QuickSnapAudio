#include "hotkeymanager.h"
#include <QKeySequence>
#include <Qt>

#ifdef _WIN32
#include "platform/windows_hotkey.h"
#elif defined(__linux__)
#include "platform/linux_hotkey.h"
#endif

// ---- Platform-independent hotkey string parsing ----

bool HotkeyManager::parseHotkeyString(const QString &str, int &modifiers, int &key)
{
    QKeySequence seq(str);
    if (seq.count() < 1)
        return false;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QKeyCombination combo = seq[0];
    modifiers = static_cast<int>(combo.keyboardModifiers());
    key = combo.key();
#else
    int combined = seq[0];
    modifiers = combined & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);
    key = combined & ~(Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);
#endif
    return key != 0;
}

QString HotkeyManager::hotkeyToString(int modifiers, int key)
{
    QKeySequence seq(modifiers | key);
    return seq.toString(QKeySequence::PortableText);
}

// ---- Common ----

HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
{
#ifdef _WIN32
    m_eventFilter = new WinEventFilter(this);
#elif defined(__linux__)
    m_thread = new LinuxHotkeyThread(this);
    m_thread->start();
#endif
}

HotkeyManager::~HotkeyManager()
{
    unregisterAll();
#ifdef __linux__
    if (m_thread) {
        m_thread->stop();
        m_thread->wait(2000);
        delete m_thread;
    }
#endif
#ifdef _WIN32
    delete m_eventFilter;
#endif
}

bool HotkeyManager::registerHotkey(const QString &id, const QString &hotkeyStr, std::function<void()> callback)
{
    // If this id already has a binding, unregister it first
    if (m_bindings.contains(id)) {
        unregisterHotkey(id);
    }

    int modifiers = 0, key = 0;
    if (!parseHotkeyString(hotkeyStr, modifiers, key))
        return false;

    int platformId = m_nextPlatformId++;
    bool ok = false;

#ifdef _WIN32
    ok = WindowsHotkey::registerHotkey(platformId, modifiers, key);
#elif defined(__linux__)
    ok = LinuxHotkey::registerHotkey(m_thread, platformId, modifiers, key);
#endif

    if (ok) {
        HotkeyBinding binding;
        binding.hotkeyStr = hotkeyStr;
        binding.callback = callback;
        binding.platformId = platformId;
        m_bindings[id] = binding;
    }

    return ok;
}

void HotkeyManager::unregisterHotkey(const QString &id)
{
    if (!m_bindings.contains(id)) return;

    auto &binding = m_bindings[id];

#ifdef _WIN32
    WindowsHotkey::unregisterHotkey(binding.platformId);
#elif defined(__linux__)
    LinuxHotkey::unregisterHotkey(m_thread, binding.platformId);
#endif

    m_bindings.remove(id);
}

void HotkeyManager::unregisterAll()
{
    QStringList ids = m_bindings.keys();
    for (const auto &id : ids) {
        unregisterHotkey(id);
    }
}

void HotkeyManager::onHotkeyTriggered(int platformId)
{
    for (auto it = m_bindings.begin(); it != m_bindings.end(); ++it) {
        if (it.value().platformId == platformId) {
            if (it.value().callback) {
                it.value().callback();
            }
            return;
        }
    }
}
