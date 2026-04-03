#ifndef DEVICEENTRY_H
#define DEVICEENTRY_H

#include <QString>

struct DeviceEntry {
    QString id;          // Unique entry ID (UUID)
    QString deviceId;    // Platform device identifier
    QString deviceName;  // Human-readable device name
    QString hotkey;      // Hotkey string like "Ctrl+Shift+1"
    bool isOutput;       // true = output/playback, false = input/recording
};

#endif // DEVICEENTRY_H
