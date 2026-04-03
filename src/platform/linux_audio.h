#ifndef LINUX_AUDIO_H
#define LINUX_AUDIO_H

#include <QVector>
#include "audiodevicemanager.h"

namespace LinuxAudio {
    QVector<AudioDeviceInfo> enumerateDevices();
    bool setDefaultDevice(const QString &deviceId, bool isOutput);
}

#endif // LINUX_AUDIO_H
