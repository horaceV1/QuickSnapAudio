#include "bluetoothmanager.h"

#include <QStringList>

#ifdef _WIN32
#include "platform/windows_bluetooth.h"
#elif defined(__linux__)
#include "platform/linux_bluetooth.h"
#endif

BluetoothManager::BluetoothManager() = default;
BluetoothManager::~BluetoothManager() = default;

bool BluetoothManager::looksLikeBluetooth(const QString &deviceName)
{
    if (deviceName.isEmpty()) return false;

    static const QStringList kBtTokens = {
        QStringLiteral("bluetooth"),
        QStringLiteral("wireless"),
        QStringLiteral("airpods"),
        QStringLiteral("buds"),
        QStringLiteral("hands-free"),
        QStringLiteral("handsfree"),
        QStringLiteral("headset"),
        QStringLiteral("(bt)"),
        QStringLiteral(" bt "),
    };

    const QString lower = deviceName.toLower();
    for (const auto &tok : kBtTokens) {
        if (lower.contains(tok))
            return true;
    }
    // Match a leading or trailing "BT" token (case insensitive).
    if (lower.startsWith(QStringLiteral("bt ")) || lower.endsWith(QStringLiteral(" bt")))
        return true;
    return false;
}

bool BluetoothManager::disconnect(const QString &deviceName)
{
    if (deviceName.isEmpty()) return false;
#ifdef _WIN32
    return WindowsBluetooth::setConnected(deviceName, false);
#elif defined(__linux__)
    return LinuxBluetooth::setConnected(deviceName, false);
#else
    return false;
#endif
}

bool BluetoothManager::connect(const QString &deviceName)
{
    if (deviceName.isEmpty()) return false;
#ifdef _WIN32
    return WindowsBluetooth::setConnected(deviceName, true);
#elif defined(__linux__)
    return LinuxBluetooth::setConnected(deviceName, true);
#else
    return false;
#endif
}
