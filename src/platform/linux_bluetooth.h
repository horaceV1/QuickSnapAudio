#ifndef LINUX_BLUETOOTH_H
#define LINUX_BLUETOOTH_H

#ifdef __linux__

#include <QString>

namespace LinuxBluetooth {
    // Connect / disconnect a paired Bluetooth device by its friendly name
    // using `bluetoothctl`. On failure, errorOut is filled with a
    // human-readable explanation.
    bool setConnected(const QString &deviceName, bool connect, QString *errorOut = nullptr);
}

#endif // __linux__
#endif // LINUX_BLUETOOTH_H
