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
    bool setMute(const QString &deviceId, bool isOutput, bool mute);
    bool isMuted(const QString &deviceId, bool isOutput) const;
    bool toggleMute(const QString &deviceId, bool isOutput, bool *newMutedState = nullptr);
};

#endif // AUDIODEVICEMANAGER_H
