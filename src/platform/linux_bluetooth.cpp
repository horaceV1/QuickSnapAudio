#ifdef __linux__

#include "linux_bluetooth.h"

#include <QProcess>
#include <QStringList>
#include <QRegularExpression>

namespace {

// Run `bluetoothctl <args>` synchronously and return stdout.
QString runBluetoothctl(const QStringList &args, int timeoutMs = 4000, int *exitCode = nullptr)
{
    QProcess p;
    p.setProgram(QStringLiteral("bluetoothctl"));
    p.setArguments(args);
    p.start();
    if (!p.waitForStarted(timeoutMs)) {
        if (exitCode) *exitCode = -1;
        return {};
    }
    if (!p.waitForFinished(timeoutMs)) {
        p.kill();
        if (exitCode) *exitCode = -1;
        return {};
    }
    if (exitCode) *exitCode = p.exitCode();
    return QString::fromUtf8(p.readAllStandardOutput());
}

// Find the MAC address of a paired device whose name contains `deviceName`
// (case-insensitive substring match).
QString findDeviceMac(const QString &deviceName)
{
    const QString listing = runBluetoothctl({QStringLiteral("devices")});
    // Format: "Device AA:BB:CC:DD:EE:FF Friendly Name"
    static const QRegularExpression line(
        QStringLiteral(R"(^Device\s+([0-9A-Fa-f:]{17})\s+(.+)$)"));
    const QString needle = deviceName.toLower();
    const auto rows = listing.split(QChar('\n'), Qt::SkipEmptyParts);
    for (const QString &row : rows) {
        const auto m = line.match(row.trimmed());
        if (!m.hasMatch()) continue;
        const QString mac  = m.captured(1);
        const QString name = m.captured(2).trimmed();
        const QString nameLower = name.toLower();
        if (nameLower.contains(needle) || needle.contains(nameLower))
            return mac;
    }
    return {};
}

} // namespace

bool LinuxBluetooth::setConnected(const QString &deviceName, bool connect, QString *errorOut)
{
    const QString mac = findDeviceMac(deviceName);
    if (mac.isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("not paired (no matching device for '%1')")
                                      .arg(deviceName);
        return false;
    }

    const QString cmd = connect ? QStringLiteral("connect")
                                : QStringLiteral("disconnect");
    int code = -1;
    const QString output = runBluetoothctl({cmd, mac}, 8000, &code);
    if (code != 0 && errorOut) {
        *errorOut = output.trimmed().isEmpty()
            ? QStringLiteral("bluetoothctl exited with code %1").arg(code)
            : output.trimmed();
    }
    return code == 0;
}

#endif // __linux__
