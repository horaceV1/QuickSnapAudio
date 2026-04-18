#include "audiodevicemanager.h"

#ifdef _WIN32
#include "platform/windows_audio.h"
#elif defined(__linux__)
#include "platform/linux_audio.h"
#endif

AudioDeviceManager::AudioDeviceManager()
{
#ifdef _WIN32
    WindowsAudio::initialize();
#endif
}

AudioDeviceManager::~AudioDeviceManager()
{
#ifdef _WIN32
    WindowsAudio::cleanup();
#endif
}

QVector<AudioDeviceInfo> AudioDeviceManager::enumerateDevices() const
{
#ifdef _WIN32
    return WindowsAudio::enumerateDevices();
#elif defined(__linux__)
    return LinuxAudio::enumerateDevices();
#else
    return {};
#endif
}

bool AudioDeviceManager::setDefaultDevice(const QString &deviceId, bool isOutput)
{
#ifdef _WIN32
    return WindowsAudio::setDefaultDevice(deviceId, isOutput);
#elif defined(__linux__)
    return LinuxAudio::setDefaultDevice(deviceId, isOutput);
#else
    Q_UNUSED(deviceId);
    Q_UNUSED(isOutput);
    return false;
#endif
}

bool AudioDeviceManager::setMute(const QString &deviceId, bool isOutput, bool mute)
{
#ifdef _WIN32
    return WindowsAudio::setMute(deviceId, isOutput, mute);
#elif defined(__linux__)
    return LinuxAudio::setMute(deviceId, isOutput, mute);
#else
    Q_UNUSED(deviceId);
    Q_UNUSED(isOutput);
    Q_UNUSED(mute);
    return false;
#endif
}

bool AudioDeviceManager::isMuted(const QString &deviceId, bool isOutput) const
{
#ifdef _WIN32
    return WindowsAudio::isMuted(deviceId, isOutput);
#elif defined(__linux__)
    return LinuxAudio::isMuted(deviceId, isOutput);
#else
    Q_UNUSED(deviceId);
    Q_UNUSED(isOutput);
    return false;
#endif
}

bool AudioDeviceManager::toggleMute(const QString &deviceId, bool isOutput, bool *newMutedState)
{
    bool curr = isMuted(deviceId, isOutput);
    bool target = !curr;
    bool ok = setMute(deviceId, isOutput, target);
    if (newMutedState) *newMutedState = ok ? target : curr;
    return ok;
}
