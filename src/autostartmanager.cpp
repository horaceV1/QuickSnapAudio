#include "autostartmanager.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#ifdef _WIN32
#  include <QSettings>
#endif

namespace {

constexpr const char *kAppName = "QuickSnapAudio";

QString currentExecutablePath()
{
    QString path = QCoreApplication::applicationFilePath();
    return QDir::toNativeSeparators(path);
}

#ifdef _WIN32
QString runRegistryKey()
{
    return QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
}
#endif

#ifdef __linux__
QString autostartDesktopFile()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/.config");
    }
    return dir + QStringLiteral("/autostart/") + QString::fromLatin1(kAppName) +
           QStringLiteral(".desktop");
}
#endif

} // namespace

bool AutostartManager::isEnabled()
{
#ifdef _WIN32
    QSettings reg(runRegistryKey(), QSettings::NativeFormat);
    const QString value = reg.value(QString::fromLatin1(kAppName)).toString();
    return !value.isEmpty();
#elif defined(__linux__)
    return QFileInfo::exists(autostartDesktopFile());
#else
    return false;
#endif
}

bool AutostartManager::setEnabled(bool enabled, QString *errorOut)
{
#ifdef _WIN32
    QSettings reg(runRegistryKey(), QSettings::NativeFormat);
    if (enabled) {
        // Quote the path so spaces in "Program Files" are handled.
        const QString quoted = QStringLiteral("\"%1\"").arg(currentExecutablePath());
        reg.setValue(QString::fromLatin1(kAppName), quoted);
    } else {
        reg.remove(QString::fromLatin1(kAppName));
    }
    reg.sync();
    if (reg.status() != QSettings::NoError) {
        if (errorOut) *errorOut = QStringLiteral("could not write to Windows registry");
        return isEnabled() == enabled;
    }
    return isEnabled() == enabled;

#elif defined(__linux__)
    const QString path = autostartDesktopFile();
    if (!enabled) {
        QFile f(path);
        if (f.exists() && !f.remove()) {
            if (errorOut) *errorOut = f.errorString();
            return false;
        }
        return true;
    }

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorOut) *errorOut = f.errorString();
        return false;
    }
    QTextStream out(&f);
    out << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=" << kAppName << "\n"
        << "Comment=Quickly switch audio devices with global hotkeys\n"
        << "Exec=" << currentExecutablePath() << "\n"
        << "Icon=" << kAppName << "\n"
        << "Terminal=false\n"
        << "X-GNOME-Autostart-enabled=true\n"
        << "Hidden=false\n";
    f.close();
    return true;

#else
    Q_UNUSED(enabled);
    if (errorOut) *errorOut = QStringLiteral("unsupported platform");
    return false;
#endif
}
