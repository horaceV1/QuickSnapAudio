#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVersionNumber>

class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker();

    void checkForUpdates(bool silent = false);

signals:
    void updateAvailable(const QString &latestVersion, const QString &downloadUrl, const QString &releaseNotes);
    void noUpdateAvailable();
    void updateCheckFailed(const QString &error);

private slots:
    void onReleaseInfoReceived(QNetworkReply *reply);
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(QNetworkReply *reply);

private:
    void downloadAndInstall(const QString &downloadUrl);
    QString platformAssetName() const;
    bool isNewerVersion(const QString &remote) const;

    QNetworkAccessManager *m_nam;
    bool m_silent;

    static constexpr const char *GITHUB_API_URL =
        "https://api.github.com/repos/horaceV1/QuickSnapAudio/releases/latest";
};

#endif // UPDATECHECKER_H
