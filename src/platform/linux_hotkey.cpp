#ifdef __linux__

#include "linux_hotkey.h"
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <Qt>
#include <QMetaObject>

static unsigned int qtModToX11(int qtMod)
{
    unsigned int mod = 0;
    if (qtMod & Qt::ControlModifier) mod |= ControlMask;
    if (qtMod & Qt::ShiftModifier)   mod |= ShiftMask;
    if (qtMod & Qt::AltModifier)     mod |= Mod1Mask;
    if (qtMod & Qt::MetaModifier)    mod |= Mod4Mask;
    return mod;
}

static KeySym qtKeyToKeySym(int qtKey)
{
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        return XK_0 + (qtKey - Qt::Key_0);
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
        return XK_a + (qtKey - Qt::Key_A); // lowercase for X11
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F24)
        return XK_F1 + (qtKey - Qt::Key_F1);

    switch (qtKey) {
    case Qt::Key_Space:     return XK_space;
    case Qt::Key_Return:    return XK_Return;
    case Qt::Key_Escape:    return XK_Escape;
    case Qt::Key_Tab:       return XK_Tab;
    case Qt::Key_Backspace: return XK_BackSpace;
    case Qt::Key_Delete:    return XK_Delete;
    case Qt::Key_Insert:    return XK_Insert;
    case Qt::Key_Home:      return XK_Home;
    case Qt::Key_End:       return XK_End;
    case Qt::Key_PageUp:    return XK_Page_Up;
    case Qt::Key_PageDown:  return XK_Page_Down;
    case Qt::Key_Up:        return XK_Up;
    case Qt::Key_Down:      return XK_Down;
    case Qt::Key_Left:      return XK_Left;
    case Qt::Key_Right:     return XK_Right;
    default: return NoSymbol;
    }
}

// ---- LinuxHotkeyThread ----

HotkeyManager::LinuxHotkeyThread::LinuxHotkeyThread(HotkeyManager *manager)
    : QThread(nullptr), m_manager(manager), m_running(false)
{
}

void HotkeyManager::LinuxHotkeyThread::stop()
{
    m_running = false;
}

bool HotkeyManager::LinuxHotkeyThread::addHotkey(int platformId, unsigned int mod, unsigned int keycode)
{
    QMutexLocker lock(&m_mutex);

    if (m_display) {
        Display *dpy = static_cast<Display *>(m_display);
        Window root = DefaultRootWindow(dpy);

        // Grab with and without NumLock/CapsLock
        unsigned int modVariants[] = { mod, mod | Mod2Mask, mod | LockMask, mod | Mod2Mask | LockMask };
        for (auto m : modVariants) {
            XGrabKey(dpy, keycode, m, root, False, GrabModeAsync, GrabModeAsync);
        }
        XFlush(dpy);
    }

    LinuxHotkeyBinding binding;
    binding.platformId = platformId;
    binding.x11Mod = mod;
    binding.x11Keycode = keycode;
    m_hotkeys.append(binding);

    return true;
}

void HotkeyManager::LinuxHotkeyThread::removeHotkey(int platformId)
{
    QMutexLocker lock(&m_mutex);

    for (int i = 0; i < m_hotkeys.size(); ++i) {
        if (m_hotkeys[i].platformId == platformId) {
            if (m_display) {
                Display *dpy = static_cast<Display *>(m_display);
                Window root = DefaultRootWindow(dpy);
                unsigned int mod = m_hotkeys[i].x11Mod;
                unsigned int keycode = m_hotkeys[i].x11Keycode;

                unsigned int modVariants[] = { mod, mod | Mod2Mask, mod | LockMask, mod | Mod2Mask | LockMask };
                for (auto m : modVariants) {
                    XUngrabKey(dpy, keycode, m, root);
                }
                XFlush(dpy);
            }
            m_hotkeys.removeAt(i);
            return;
        }
    }
}

void HotkeyManager::LinuxHotkeyThread::run()
{
    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return;

    {
        QMutexLocker lock(&m_mutex);
        m_display = dpy;
    }

    Window root = DefaultRootWindow(dpy);
    XSelectInput(dpy, root, KeyPressMask);

    m_running = true;
    while (m_running) {
        while (XPending(dpy)) {
            XEvent event;
            XNextEvent(dpy, &event);
            if (event.type == KeyPress) {
                unsigned int keycode = event.xkey.keycode;
                unsigned int state = event.xkey.state & ~(Mod2Mask | LockMask);

                QMutexLocker lock(&m_mutex);
                for (const auto &hk : m_hotkeys) {
                    if (hk.x11Keycode == keycode && hk.x11Mod == state) {
                        int id = hk.platformId;
                        lock.unlock();
                        QMetaObject::invokeMethod(m_manager, [this, id]() {
                            m_manager->onHotkeyTriggered(id);
                        }, Qt::QueuedConnection);
                        break;
                    }
                }
            }
        }
        QThread::msleep(50);
    }

    {
        QMutexLocker lock(&m_mutex);
        m_display = nullptr;
    }
    XCloseDisplay(dpy);
}

// ---- LinuxHotkey namespace ----

bool LinuxHotkey::registerHotkey(HotkeyManager::LinuxHotkeyThread *thread, int platformId, int qtModifiers, int qtKey)
{
    if (!thread || !thread->isRunning()) return false;

    unsigned int x11Mod = qtModToX11(qtModifiers);
    KeySym keysym = qtKeyToKeySym(qtKey);
    if (keysym == NoSymbol) return false;

    // We need an X display to convert keysym to keycode
    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return false;
    unsigned int keycode = XKeysymToKeycode(dpy, keysym);
    XCloseDisplay(dpy);

    if (keycode == 0) return false;

    return thread->addHotkey(platformId, x11Mod, keycode);
}

void LinuxHotkey::unregisterHotkey(HotkeyManager::LinuxHotkeyThread *thread, int platformId)
{
    if (!thread) return;
    thread->removeHotkey(platformId);
}

bool LinuxHotkey::isKeyDown(int qtKey)
{
    KeySym keysym = qtKeyToKeySym(qtKey);
    if (keysym == NoSymbol) return false;

    Display *dpy = XOpenDisplay(nullptr);
    if (!dpy) return false;

    KeyCode keycode = XKeysymToKeycode(dpy, keysym);
    if (keycode == 0) {
        XCloseDisplay(dpy);
        return false;
    }

    char keys[32] = {};
    XQueryKeymap(dpy, keys);
    XCloseDisplay(dpy);

    return (keys[keycode / 8] & (1 << (keycode % 8))) != 0;
}

#endif // __linux__
