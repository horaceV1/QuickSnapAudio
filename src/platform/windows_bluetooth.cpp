#ifdef _WIN32

#include "windows_bluetooth.h"

#include <Windows.h>
#include <BluetoothAPIs.h>
#include <initguid.h>
#include <vector>

#pragma comment(lib, "Bthprops.lib")

// Bluetooth audio service GUIDs we attempt to toggle. Disabling any one of
// these typically disconnects the audio device; enabling them re-establishes
// the connection automatically when the device is in range.
//
// {0000110b-0000-1000-8000-00805f9b34fb} - AudioSink (A2DP sink)
// {0000111e-0000-1000-8000-00805f9b34fb} - Handsfree
// {00001108-0000-1000-8000-00805f9b34fb} - Headset
static const GUID kAudioSinkServiceClass_UUID =
    { 0x0000110B, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };
static const GUID kHandsfreeServiceClass_UUID =
    { 0x0000111E, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };
static const GUID kHeadsetServiceClass_UUID =
    { 0x00001108, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };

bool WindowsBluetooth::setConnected(const QString &deviceName, bool connect)
{
    BLUETOOTH_FIND_RADIO_PARAMS radioParams = { sizeof(radioParams) };
    HANDLE hRadio = nullptr;
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&radioParams, &hRadio);
    if (!hFind) return false;

    bool anySuccess = false;
    const std::wstring wantedName = deviceName.toLower().toStdWString();

    do {
        BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = { sizeof(searchParams) };
        searchParams.fReturnAuthenticated = TRUE;
        searchParams.fReturnRemembered    = TRUE;
        searchParams.fReturnConnected     = TRUE;
        searchParams.fReturnUnknown       = FALSE;
        searchParams.fIssueInquiry        = FALSE;
        searchParams.cTimeoutMultiplier   = 0;
        searchParams.hRadio               = hRadio;

        BLUETOOTH_DEVICE_INFO devInfo = { sizeof(devInfo) };
        HBLUETOOTH_DEVICE_FIND hDevFind = BluetoothFindFirstDevice(&searchParams, &devInfo);
        if (hDevFind) {
            do {
                std::wstring devName = devInfo.szName;
                std::wstring devLower = devName;
                for (auto &c : devLower) c = static_cast<wchar_t>(towlower(c));

                // Match if the audio endpoint name contains the BT device name
                // OR vice versa. Audio endpoints are typically named like
                // "Headphones (My Headset Stereo)".
                bool matches = false;
                if (!devLower.empty()) {
                    if (wantedName.find(devLower) != std::wstring::npos ||
                        devLower.find(wantedName) != std::wstring::npos) {
                        matches = true;
                    }
                }

                if (matches) {
                    const DWORD newState = connect
                        ? BLUETOOTH_SERVICE_ENABLE
                        : BLUETOOTH_SERVICE_DISABLE;

                    const GUID services[] = {
                        kAudioSinkServiceClass_UUID,
                        kHandsfreeServiceClass_UUID,
                        kHeadsetServiceClass_UUID,
                    };
                    for (const auto &svc : services) {
                        DWORD r = BluetoothSetServiceState(hRadio, &devInfo, &svc, newState);
                        if (r == ERROR_SUCCESS) anySuccess = true;
                    }
                }
            } while (BluetoothFindNextDevice(hDevFind, &devInfo));
            BluetoothFindDeviceClose(hDevFind);
        }
        CloseHandle(hRadio);
    } while (BluetoothFindNextRadio(hFind, &hRadio));

    BluetoothFindRadioClose(hFind);
    return anySuccess;
}

#endif // _WIN32
