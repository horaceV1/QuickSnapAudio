#ifndef WINDOWS_BLUETOOTH_H
#define WINDOWS_BLUETOOTH_H

#ifdef _WIN32

#include <QString>

namespace WindowsBluetooth {
    // Toggle the connection state of paired Bluetooth audio services for the
    // device whose friendly name matches `deviceName`. On failure, errorOut
    // is filled with a human-readable explanation. Returns true if at least
    // one matching service was successfully toggled.
    bool setConnected(const QString &deviceName, bool connect, QString *errorOut = nullptr);
}

#endif // _WIN32
#endif // WINDOWS_BLUETOOTH_H
