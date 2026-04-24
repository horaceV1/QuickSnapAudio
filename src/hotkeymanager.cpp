#include "hotkeymanager.h"
#include <QKeySequence>
#include <QDateTime>
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

bool HotkeyManager::registerHotkey(const QString &id,
                                   const QString &hotkeyStr,
                                   HotkeyCallback shortPress,
                                   HotkeyCallback longPress,
                                   int longPressMs)
{
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
        binding.shortPress = std::move(shortPress);
        binding.longPress = std::move(longPress);
        binding.platformId = platformId;
        binding.qtModifiers = modifiers;
        binding.qtKey = key;
        binding.longPressMs = longPressMs > 0 ? longPressMs : 1200;
        m_bindings.insert(id, binding);
    }

    return ok;
}

void HotkeyManager::unregisterHotkey(const QString &id)
{
    auto it = m_bindings.find(id);
    if (it == m_bindings.end()) return;

    if (it->pollTimer) {
        it->pollTimer->stop();
        it->pollTimer->deleteLater();
        it->pollTimer = nullptr;
    }

#ifdef _WIN32
    WindowsHotkey::unregisterHotkey(it->platformId);
#elif defined(__linux__)
    LinuxHotkey::unregisterHotkey(m_thread, it->platformId);
#endif

    m_bindings.erase(it);
}

void HotkeyManager::unregisterAll()
{
    const QStringList ids = m_bindings.keys();
    for (const auto &id : ids) {
        unregisterHotkey(id);
    }
}

bool HotkeyManager::isKeyStillDown(int qtKey) const
{
#ifdef _WIN32
    return WindowsHotkey::isKeyDown(qtKey);
#elif defined(__linux__)
    return LinuxHotkey::isKeyDown(qtKey);
#else
    Q_UNUSED(qtKey);
    return false;
#endif
}

void HotkeyManager::startLongPressTracking(const QString &id)
{
    auto it = m_bindings.find(id);
    if (it == m_bindings.end()) return;

    HotkeyBinding &binding = it.value();
    binding.pressStartedMs = QDateTime::currentMSecsSinceEpoch();
    binding.longFired = false;

    if (!binding.pollTimer) {
        binding.pollTimer = new QTimer(this);
        binding.pollTimer->setInterval(40);
        QString idCopy = id;
        connect(binding.pollTimer, &QTimer::timeout, this, [this, idCopy]() {
            auto it2 = m_bindings.find(idCopy);
            if (it2 == m_bindings.end()) return;
            HotkeyBinding &b = it2.value();

            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            const qint64 held = now - b.pressStartedMs;
            const bool down = isKeyStillDown(b.qtKey);

            if (!down) {
                b.pollTimer->stop();
                if (!b.longFired) {
                    if (b.shortPress) b.shortPress();
                }
                return;
            }

            if (!b.longFired && held >= b.longPressMs) {
                b.longFired = true;
                if (b.longPress) b.longPress();
                // Continue polling so we know when key is finally released;
                // but no further callback fires. Stop polling once released.
            }
        });
    }

    binding.pollTimer->start();
}

void HotkeyManager::onHotkeyTriggered(int platformId)
{
    for (auto it = m_bindings.begin(); it != m_bindings.end(); ++it) {
        if (it.value().platformId != platformId)
            continue;

        // No long-press handler? Fire short press immediately and we're done.
        if (!it.value().longPress) {
            if (it.value().shortPress) it.value().shortPress();
            return;
        }

        // Long-press capable: do not fire shortPress yet — start polling.
        // If a previous press is still being tracked, ignore (auto-repeat).
        if (it.value().pollTimer && it.value().pollTimer->isActive())
            return;

        startLongPressTracking(it.key());
        return;
    }
}
