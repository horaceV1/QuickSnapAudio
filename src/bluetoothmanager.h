#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H

#include <QString>

// Cross-platform helper for detecting and toggling Bluetooth audio devices.
//
// Detection uses a name-based heuristic that works for the vast majority of
// consumer Bluetooth audio devices ("Bluetooth", "Wireless", "AirPods",
// "Buds", "Hands-Free", "Headset", "BT", ...). It is intentionally
// conservative — false positives just mean the long-press disconnect feature
// is exposed for a non-BT device, in which case the underlying disconnect
// call will simply fail (and we surface that to the user via a notification).
//
// Connect/disconnect are best-effort and platform specific:
//   * Windows: uses BluetoothSetServiceState() against the AudioSink and
//              Hands-Free service GUIDs.
//   * Linux:   shells out to `bluetoothctl` (must be on $PATH).
class BluetoothManager {
public:
    BluetoothManager();
    ~BluetoothManager();

    // Heuristic: returns true if the friendly device name looks like a
    // Bluetooth / wireless audio endpoint.
    static bool looksLikeBluetooth(const QString &deviceName);

    // Disconnect the bluetooth device whose friendly name matches deviceName.
    // Returns true on success.
    bool disconnect(const QString &deviceName);

    // (Re)connect the bluetooth device whose friendly name matches deviceName.
    bool connect(const QString &deviceName);
};

#endif // BLUETOOTHMANAGER_H
