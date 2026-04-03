#ifdef _WIN32

#include "windows_audio.h"

#include <Windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <Propidl.h>
#include <combaseapi.h>
#include <endpointvolume.h>
#include <QString>

// PolicyConfig interface for setting default audio endpoint (undocumented but widely used)
// {F8679F50-850A-41CF-9C72-430F290290C8}
static const GUID CLSID_CPolicyConfigClient = {
    0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}
};

// IPolicyConfig interface GUID
// {F8679F50-850A-41CF-9C72-430F290290C8}
MIDL_INTERFACE("f8679f50-850a-41cf-9c72-430f290290c8")
IPolicyConfig : public IUnknown
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
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY &, CPROPVARIANT *) = 0;
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
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   (void**)&pEnum);
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
            pProps->GetValue(PKEY_Device_FriendlyName, &varName);

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
    HRESULT hr = CoCreateInstance(CLSID_CPolicyConfigClient, nullptr, CLSCTX_ALL,
                                   __uuidof(IPolicyConfig), (void**)&pPolicyConfig);
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

#endif // _WIN32
