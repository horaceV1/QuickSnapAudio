#ifdef _WIN32

#include "windows_bluetooth.h"

#include <Windows.h>
#include <BluetoothAPIs.h>
#include <initguid.h>
#include <vector>
#include <string>

#pragma comment(lib, "Bthprops.lib")

// Some MinGW SDKs ship bluetoothapis.h without the service-state constants.
// Values per the Windows SDK headers.
#ifndef BLUETOOTH_SERVICE_DISABLE
#define BLUETOOTH_SERVICE_DISABLE 0x00
#endif
#ifndef BLUETOOTH_SERVICE_ENABLE
#define BLUETOOTH_SERVICE_ENABLE  0x01
#endif

namespace {

QString winErrorToString(DWORD err)
{
    if (err == ERROR_SUCCESS) return QStringLiteral("ok");
    LPWSTR msg = nullptr;
    const DWORD n = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, err, 0, reinterpret_cast<LPWSTR>(&msg), 0, nullptr);
    QString result;
    if (n && msg) {
        result = QString::fromWCharArray(msg, static_cast<int>(n)).trimmed();
        LocalFree(msg);
    }
    if (result.isEmpty())
        result = QStringLiteral("error %1").arg(err);
    return result;
}

std::wstring toLower(std::wstring s)
{
    for (auto &c : s) c = static_cast<wchar_t>(towlower(c));
    return s;
}

// Audio endpoint names look like "Headphones (WH-1000XM4 Stereo)" or
// "Headset (WH-1000XM4 Hands-Free AG Audio)". The actual paired BT device
// name is just "WH-1000XM4". This pulls the inner name out and drops the
// stereo / hands-free trailing tag.
std::wstring normalizeAudioEndpointName(const std::wstring &raw)
{
    std::wstring s = raw;

    // Strip leading "<role> (" prefix and matching trailing ')'.
    auto open = s.find(L'(');
    auto close = s.rfind(L')');
    if (open != std::wstring::npos && close != std::wstring::npos && close > open) {
        s = s.substr(open + 1, close - open - 1);
    }

    // Drop the trailing role tag added by the Windows audio stack.
    static const wchar_t *kSuffixes[] = {
        L" stereo",
        L" hands-free ag audio",
        L" hands-free",
        L" handsfree",
        L" headset",
        L" headphones",
        L" microphone",
        L" mono",
    };
    auto lower = toLower(s);
    for (auto *suf : kSuffixes) {
        const std::wstring suffix = suf;
        if (lower.size() >= suffix.size() &&
            lower.compare(lower.size() - suffix.size(), suffix.size(), suffix) == 0) {
            s = s.substr(0, s.size() - suffix.size());
            break;
        }
    }

    // Trim trailing whitespace
    while (!s.empty() && iswspace(s.back())) s.pop_back();
    return s;
}

// Returns true if `bigger` contains `smaller` (case-insensitive).
bool containsCI(const std::wstring &bigger, const std::wstring &smaller)
{
    if (smaller.empty()) return false;
    return toLower(bigger).find(toLower(smaller)) != std::wstring::npos;
}

// Discover which services are actually installed on the device, then return
// the subset that matches our audio service GUIDs (A2DP sink / Handsfree /
// Headset). Toggling only installed services avoids spurious failures.
std::vector<GUID> installedAudioServices(HANDLE hRadio, BLUETOOTH_DEVICE_INFO &devInfo)
{
    static const GUID kAudioSink = { 0x0000110B, 0x0000, 0x1000,
        { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };
    static const GUID kHandsfree = { 0x0000111E, 0x0000, 0x1000,
        { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };
    static const GUID kHeadset   = { 0x00001108, 0x0000, 0x1000,
        { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };
    static const GUID kAudioSrc  = { 0x0000110A, 0x0000, 0x1000,
        { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };

    std::vector<GUID> wanted = { kAudioSink, kHandsfree, kHeadset, kAudioSrc };

    DWORD count = 0;
    DWORD r = BluetoothEnumerateInstalledServices(hRadio, &devInfo, &count, nullptr);
    if (r != ERROR_MORE_DATA && r != ERROR_SUCCESS) {
        // API not supported or error — fall back to trying all four GUIDs.
        return wanted;
    }
    if (count == 0) return wanted;

    std::vector<GUID> services(count);
    r = BluetoothEnumerateInstalledServices(hRadio, &devInfo, &count, services.data());
    if (r != ERROR_SUCCESS) return wanted;

    std::vector<GUID> matches;
    for (const auto &svc : services) {
        for (const auto &want : wanted) {
            if (memcmp(&svc, &want, sizeof(GUID)) == 0) {
                matches.push_back(svc);
                break;
            }
        }
    }
    return matches.empty() ? wanted : matches;
}

} // namespace

bool WindowsBluetooth::setConnected(const QString &deviceName, bool connect, QString *errorOut)
{
    BLUETOOTH_FIND_RADIO_PARAMS radioParams = { sizeof(radioParams) };
    HANDLE hRadio = nullptr;
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&radioParams, &hRadio);
    if (!hFind) {
        if (errorOut) *errorOut = QStringLiteral("no Bluetooth radio found");
        return false;
    }

    const std::wstring rawName  = deviceName.toStdWString();
    const std::wstring normName = normalizeAudioEndpointName(rawName);

    bool anySuccess = false;
    bool sawMatch   = false;
    DWORD lastErr   = ERROR_SUCCESS;

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
                const std::wstring devName = devInfo.szName;
                if (devName.empty()) continue;

                // Match if any of the names contains either of the others.
                // We test against both the raw audio endpoint name and the
                // stripped/normalized version so e.g. "WH-1000XM4" finds
                // "Headphones (WH-1000XM4 Stereo)".
                const bool matches =
                    containsCI(rawName,  devName) ||
                    containsCI(devName,  rawName) ||
                    (!normName.empty() && (containsCI(normName, devName) ||
                                           containsCI(devName,  normName)));
                if (!matches) continue;

                sawMatch = true;
                const DWORD newState = connect
                    ? BLUETOOTH_SERVICE_ENABLE
                    : BLUETOOTH_SERVICE_DISABLE;

                const auto services = installedAudioServices(hRadio, devInfo);
                for (const auto &svc : services) {
                    GUID svcCopy = svc;  // BluetoothSetServiceState wants non-const
                    DWORD r = BluetoothSetServiceState(hRadio, &devInfo, &svcCopy, newState);
                    if (r == ERROR_SUCCESS) {
                        anySuccess = true;
                    } else {
                        lastErr = r;
                    }
                }
            } while (BluetoothFindNextDevice(hDevFind, &devInfo));
            BluetoothFindDeviceClose(hDevFind);
        }
        CloseHandle(hRadio);
    } while (BluetoothFindNextRadio(hFind, &hRadio));

    BluetoothFindRadioClose(hFind);

    if (!anySuccess && errorOut) {
        if (!sawMatch) {
            *errorOut = QStringLiteral("not paired (no matching Bluetooth device for '%1')")
                            .arg(deviceName);
        } else if (lastErr == ERROR_ACCESS_DENIED) {
            *errorOut = QStringLiteral("permission denied — run QuickSnapAudio as administrator");
        } else if (lastErr != ERROR_SUCCESS) {
            *errorOut = winErrorToString(lastErr);
        } else {
            *errorOut = QStringLiteral("no Bluetooth audio service responded");
        }
    }
    return anySuccess;
}

#endif // _WIN32
