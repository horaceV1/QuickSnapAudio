#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QVector>
#include <QString>
#include "deviceentry.h"

class ConfigManager {
public:
    ConfigManager();

    QVector<DeviceEntry> loadEntries() const;
    void saveEntries(const QVector<DeviceEntry> &entries);

    QString configFilePath() const;

private:
    QString m_configPath;
};

#endif // CONFIGMANAGER_H
