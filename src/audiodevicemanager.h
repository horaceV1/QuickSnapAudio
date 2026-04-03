#ifndef AUDIODEVICEMANAGER_H
#define AUDIODEVICEMANAGER_H

#include <QString>
#include <QVector>
#include <QPair>

struct AudioDeviceInfo {
    QString id;
    QString name;
    bool isOutput; // true = playback, false = capture
};

class AudioDeviceManager {
public:
    AudioDeviceManager();
    ~AudioDeviceManager();

    QVector<AudioDeviceInfo> enumerateDevices() const;
    bool setDefaultDevice(const QString &deviceId, bool isOutput);
};

#endif // AUDIODEVICEMANAGER_H
