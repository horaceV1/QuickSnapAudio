#ifdef _WIN32

#include "windows_hotkey.h"
#include <Windows.h>
#include <QCoreApplication>
#include <Qt>

static UINT qtModToWin(int qtMod)
{
    UINT mod = 0;
    if (qtMod & Qt::ControlModifier) mod |= MOD_CONTROL;
    if (qtMod & Qt::ShiftModifier)   mod |= MOD_SHIFT;
    if (qtMod & Qt::AltModifier)     mod |= MOD_ALT;
    if (qtMod & Qt::MetaModifier)    mod |= MOD_WIN;
    mod |= MOD_NOREPEAT;
    return mod;
}

static UINT qtKeyToVk(int qtKey)
{
    // Numbers
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        return qtKey; // VK codes match ASCII for 0-9

    // Letters
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
        return qtKey; // VK codes match ASCII for A-Z

    // Function keys
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F24)
        return VK_F1 + (qtKey - Qt::Key_F1);

    switch (qtKey) {
    case Qt::Key_Space:     return VK_SPACE;
    case Qt::Key_Return:    return VK_RETURN;
    case Qt::Key_Enter:     return VK_RETURN;
    case Qt::Key_Escape:    return VK_ESCAPE;
    case Qt::Key_Tab:       return VK_TAB;
    case Qt::Key_Backspace: return VK_BACK;
    case Qt::Key_Delete:    return VK_DELETE;
    case Qt::Key_Insert:    return VK_INSERT;
    case Qt::Key_Home:      return VK_HOME;
    case Qt::Key_End:       return VK_END;
    case Qt::Key_PageUp:    return VK_PRIOR;
    case Qt::Key_PageDown:  return VK_NEXT;
    case Qt::Key_Up:        return VK_UP;
    case Qt::Key_Down:      return VK_DOWN;
    case Qt::Key_Left:      return VK_LEFT;
    case Qt::Key_Right:     return VK_RIGHT;
    default: return 0;
    }
}

bool WindowsHotkey::registerHotkey(int id, int qtModifiers, int qtKey)
{
    UINT winMod = qtModToWin(qtModifiers);
    UINT vk = qtKeyToVk(qtKey);
    if (vk == 0) return false;

    return RegisterHotKey(nullptr, id, winMod, vk) != 0;
}

void WindowsHotkey::unregisterHotkey(int id)
{
    UnregisterHotKey(nullptr, id);
}

// ---- Native event filter ----

HotkeyManager::WinEventFilter::WinEventFilter(HotkeyManager *manager)
    : m_manager(manager)
{
    QCoreApplication::instance()->installNativeEventFilter(this);
}

HotkeyManager::WinEventFilter::~WinEventFilter()
{
    QCoreApplication::instance()->removeNativeEventFilter(this);
}

bool HotkeyManager::WinEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY) {
            int id = static_cast<int>(msg->wParam);
            m_manager->onHotkeyTriggered(id);
            return true;
        }
    }
    return false;
}

#endif // _WIN32
