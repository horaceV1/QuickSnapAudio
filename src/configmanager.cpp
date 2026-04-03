#include "configmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ConfigManager::ConfigManager()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_configPath = configDir + "/config.json";
}

QString ConfigManager::configFilePath() const
{
    return m_configPath;
}

QVector<DeviceEntry> ConfigManager::loadEntries() const
{
    QVector<DeviceEntry> entries;
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly))
        return entries;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonArray arr = doc.object().value("devices").toArray();
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        DeviceEntry entry;
        entry.id = obj["id"].toString();
        entry.deviceId = obj["deviceId"].toString();
        entry.deviceName = obj["deviceName"].toString();
        entry.hotkey = obj["hotkey"].toString();
        entry.isOutput = obj["isOutput"].toBool(true);
        entries.append(entry);
    }
    return entries;
}

void ConfigManager::saveEntries(const QVector<DeviceEntry> &entries)
{
    QJsonArray arr;
    for (const auto &entry : entries) {
        QJsonObject obj;
        obj["id"] = entry.id;
        obj["deviceId"] = entry.deviceId;
        obj["deviceName"] = entry.deviceName;
        obj["hotkey"] = entry.hotkey;
        obj["isOutput"] = entry.isOutput;
        arr.append(obj);
    }

    QJsonObject root;
    root["devices"] = arr;

    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}
