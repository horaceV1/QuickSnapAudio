#ifndef WINDOWS_AUDIO_H
#define WINDOWS_AUDIO_H

#include <QVector>
#include "audiodevicemanager.h"

namespace WindowsAudio {
    void initialize();
    void cleanup();
    QVector<AudioDeviceInfo> enumerateDevices();
    bool setDefaultDevice(const QString &deviceId, bool isOutput);
    bool setMute(const QString &deviceId, bool isOutput, bool mute);
    bool isMuted(const QString &deviceId, bool isOutput);
}

#endif // WINDOWS_AUDIO_H
