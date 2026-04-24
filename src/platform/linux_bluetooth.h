#ifndef LINUX_BLUETOOTH_H
#define LINUX_BLUETOOTH_H

#ifdef __linux__

#include <QString>

namespace LinuxBluetooth {
    // Connect / disconnect a paired Bluetooth device by its friendly name
    // using `bluetoothctl`. Returns true if the underlying command exits
    // successfully.
    bool setConnected(const QString &deviceName, bool connect);
}

#endif // __linux__
#endif // LINUX_BLUETOOTH_H
