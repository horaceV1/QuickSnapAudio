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
