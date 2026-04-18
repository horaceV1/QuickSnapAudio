#ifdef _WIN32

#include "windows_audio.h"

#include <Windows.h>
#include <mmdeviceapi.h>
#include <Propidl.h>
#include <combaseapi.h>
#include <endpointvolume.h>
#include <QString>
#include <initguid.h>

static const PROPERTYKEY s_PKEY_Device_FriendlyName = {
    {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14
};

static const CLSID s_CLSID_MMDeviceEnumerator = {
    0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}
};
static const IID s_IID_IMMDeviceEnumerator = {
    0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}
};

static const CLSID s_CLSID_CPolicyConfigClient = {
    0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}
};

static const IID s_IID_IPolicyConfig = {
    0xf8679f50, 0x850a, 0x41cf, {0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8}
};

class IPolicyConfig : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR, WAVEFORMATEX **) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, WAVEFORMATEX **) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, WAVEFORMATEX *, WAVEFORMATEX *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, PINT64, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, struct DeviceShareMode *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, struct DeviceShareMode *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR, const PROPERTYKEY &, PROPVARIANT *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY &, PROPVARIANT *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR wszDeviceId, ERole eRole) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT) = 0;
};

static bool s_comInitialized = false;

void WindowsAudio::initialize()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    s_comInitialized = SUCCEEDED(hr);
}

void WindowsAudio::cleanup()
{
    if (s_comInitialized) {
        CoUninitialize();
        s_comInitialized = false;
    }
}

QVector<AudioDeviceInfo> WindowsAudio::enumerateDevices()
{
    QVector<AudioDeviceInfo> devices;

    IMMDeviceEnumerator *pEnum = nullptr;
    HRESULT hr = CoCreateInstance(s_CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, s_IID_IMMDeviceEnumerator, (void**)&pEnum);
    if (FAILED(hr) || !pEnum)
        return devices;

    auto enumerateFlow = [&](EDataFlow flow, bool isOutput) {
        IMMDeviceCollection *pCollection = nullptr;
        hr = pEnum->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &pCollection);
        if (FAILED(hr) || !pCollection) return;

        UINT count = 0;
        pCollection->GetCount(&count);

        for (UINT i = 0; i < count; i++) {
            IMMDevice *pDevice = nullptr;
            if (FAILED(pCollection->Item(i, &pDevice))) continue;

            LPWSTR pwszID = nullptr;
            pDevice->GetId(&pwszID);

            IPropertyStore *pProps = nullptr;
            pDevice->OpenPropertyStore(STGM_READ, &pProps);

            PROPVARIANT varName;
            PropVariantInit(&varName);
            pProps->GetValue(s_PKEY_Device_FriendlyName, &varName);

            AudioDeviceInfo info;
            info.id = QString::fromWCharArray(pwszID);
            info.name = QString::fromWCharArray(varName.pwszVal);
            info.isOutput = isOutput;
            devices.append(info);

            PropVariantClear(&varName);
            pProps->Release();
            CoTaskMemFree(pwszID);
            pDevice->Release();
        }
        pCollection->Release();
    };

    enumerateFlow(eRender, true);
    enumerateFlow(eCapture, false);

    pEnum->Release();
    return devices;
}

bool WindowsAudio::setDefaultDevice(const QString &deviceId, bool isOutput)
{
    Q_UNUSED(isOutput);

    IPolicyConfig *pPolicyConfig = nullptr;
    HRESULT hr = CoCreateInstance(s_CLSID_CPolicyConfigClient, nullptr, CLSCTX_ALL, s_IID_IPolicyConfig, (void**)&pPolicyConfig);
    if (FAILED(hr) || !pPolicyConfig)
        return false;

    LPCWSTR wszDeviceId = reinterpret_cast<LPCWSTR>(deviceId.utf16());

    hr = pPolicyConfig->SetDefaultEndpoint(wszDeviceId, eConsole);
    if (SUCCEEDED(hr))
        pPolicyConfig->SetDefaultEndpoint(wszDeviceId, eMultimedia);
    if (SUCCEEDED(hr))
        pPolicyConfig->SetDefaultEndpoint(wszDeviceId, eCommunications);

    pPolicyConfig->Release();
    return SUCCEEDED(hr);
}

static IAudioEndpointVolume *getEndpointVolume(const QString &deviceId)
{
    IMMDeviceEnumerator *pEnum = nullptr;
    HRESULT hr = CoCreateInstance(s_CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL,
                                  s_IID_IMMDeviceEnumerator, (void**)&pEnum);
    if (FAILED(hr) || !pEnum) return nullptr;

    LPCWSTR wszDeviceId = reinterpret_cast<LPCWSTR>(deviceId.utf16());
    IMMDevice *pDevice = nullptr;
    hr = pEnum->GetDevice(wszDeviceId, &pDevice);
    pEnum->Release();
    if (FAILED(hr) || !pDevice) return nullptr;

    IAudioEndpointVolume *pVol = nullptr;
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVol);
    pDevice->Release();
    if (FAILED(hr)) return nullptr;
    return pVol;
}

bool WindowsAudio::setMute(const QString &deviceId, bool isOutput, bool mute)
{
    Q_UNUSED(isOutput);
    IAudioEndpointVolume *pVol = getEndpointVolume(deviceId);
    if (!pVol) return false;
    HRESULT hr = pVol->SetMute(mute ? TRUE : FALSE, nullptr);
    pVol->Release();
    return SUCCEEDED(hr);
}

bool WindowsAudio::isMuted(const QString &deviceId, bool isOutput)
{
    Q_UNUSED(isOutput);
    IAudioEndpointVolume *pVol = getEndpointVolume(deviceId);
    if (!pVol) return false;
    BOOL muted = FALSE;
    HRESULT hr = pVol->GetMute(&muted);
    pVol->Release();
    return SUCCEEDED(hr) && muted;
}

#endif // _WIN32