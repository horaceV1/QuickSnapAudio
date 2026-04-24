#include "configmanager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>

ConfigManager::ConfigManager()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_configPath = configDir + QStringLiteral("/config.json");
}

QString ConfigManager::configFilePath() const
{
    return m_configPath;
}

QJsonObject ConfigManager::readRoot() const
{
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray bytes = file.readAll();
    file.close();
    if (bytes.isEmpty()) return {};

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return {};
    return doc.object();
}

bool ConfigManager::writeRoot(const QJsonObject &root)
{
    // QSaveFile writes to a temp file and atomically renames on commit(),
    // preventing a half-written config.json if the process crashes mid-write.
    QSaveFile out(m_configPath);
    if (!out.open(QIODevice::WriteOnly))
        return false;
    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return out.commit();
}

QVector<DeviceEntry> ConfigManager::loadEntries() const
{
    QVector<DeviceEntry> entries;
    const QJsonArray arr = readRoot().value(QStringLiteral("devices")).toArray();
    entries.reserve(arr.size());
    for (const auto &val : arr) {
        const QJsonObject obj = val.toObject();
        DeviceEntry entry;
        entry.id         = obj.value(QStringLiteral("id")).toString();
        entry.deviceId   = obj.value(QStringLiteral("deviceId")).toString();
        entry.deviceName = obj.value(QStringLiteral("deviceName")).toString();
        entry.hotkey     = obj.value(QStringLiteral("hotkey")).toString();
        entry.isOutput   = obj.value(QStringLiteral("isOutput")).toBool(true);
        entries.append(entry);
    }
    return entries;
}

void ConfigManager::saveEntries(const QVector<DeviceEntry> &entries)
{
    QJsonObject root = readRoot();

    QJsonArray arr;
    for (const auto &entry : entries) {
        QJsonObject obj;
        obj[QStringLiteral("id")]         = entry.id;
        obj[QStringLiteral("deviceId")]   = entry.deviceId;
        obj[QStringLiteral("deviceName")] = entry.deviceName;
        obj[QStringLiteral("hotkey")]     = entry.hotkey;
        obj[QStringLiteral("isOutput")]   = entry.isOutput;
        arr.append(obj);
    }
    root[QStringLiteral("devices")] = arr;
    writeRoot(root);
}

QString ConfigManager::loadTheme() const
{
    return readRoot().value(QStringLiteral("theme"))
        .toString(QStringLiteral("Catppuccin Mocha"));
}

void ConfigManager::saveTheme(const QString &themeName)
{
    QJsonObject root = readRoot();
    root[QStringLiteral("theme")] = themeName;
    writeRoot(root);
}

QStringList ConfigManager::loadDisconnectedDevices() const
{
    QStringList result;
    const QJsonArray arr =
        readRoot().value(QStringLiteral("disconnectedDevices")).toArray();
    result.reserve(arr.size());
    for (const auto &v : arr) {
        const QString s = v.toString();
        if (!s.isEmpty()) result.append(s);
    }
    return result;
}

void ConfigManager::saveDisconnectedDevices(const QStringList &names)
{
    QJsonObject root = readRoot();
    QJsonArray arr;
    for (const auto &n : names) arr.append(n);
    root[QStringLiteral("disconnectedDevices")] = arr;
    writeRoot(root);
}

QByteArray ConfigManager::loadWindowGeometry() const
{
    const QString s =
        readRoot().value(QStringLiteral("windowGeometry")).toString();
    if (s.isEmpty()) return {};
    return QByteArray::fromBase64(s.toLatin1());
}

void ConfigManager::saveWindowGeometry(const QByteArray &geometry)
{
    QJsonObject root = readRoot();
    root[QStringLiteral("windowGeometry")] =
        QString::fromLatin1(geometry.toBase64());
    writeRoot(root);
}
