#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QVector>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QJsonObject>
#include "deviceentry.h"

class ConfigManager {
public:
    ConfigManager();

    QVector<DeviceEntry> loadEntries() const;
    void saveEntries(const QVector<DeviceEntry> &entries);

    QString loadTheme() const;
    void saveTheme(const QString &themeName);

    // Persisted bluetooth disconnect state (lowercase device names).
    QStringList loadDisconnectedDevices() const;
    void saveDisconnectedDevices(const QStringList &names);

    // Persisted main-window geometry (raw bytes from QWidget::saveGeometry()).
    QByteArray loadWindowGeometry() const;
    void saveWindowGeometry(const QByteArray &geometry);

    QString configFilePath() const;

private:
    QJsonObject readRoot() const;
    bool writeRoot(const QJsonObject &root);

    QString m_configPath;
};

#endif // CONFIGMANAGER_H
