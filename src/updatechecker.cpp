#include "updatechecker.h"

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QTextStream>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)), m_silent(false)
{
}

UpdateChecker::~UpdateChecker() {}

bool UpdateChecker::isNewerVersion(const QString &remote) const
{
    QString currentStr = QApplication::applicationVersion();
    QString remoteStr = remote;

    // Strip leading 'v' if present
    if (remoteStr.startsWith('v', Qt::CaseInsensitive))
        remoteStr = remoteStr.mid(1);
    if (currentStr.startsWith('v', Qt::CaseInsensitive))
        currentStr = currentStr.mid(1);

    QVersionNumber current = QVersionNumber::fromString(currentStr);
    QVersionNumber latest = QVersionNumber::fromString(remoteStr);

    return latest > current;
}

QString UpdateChecker::platformAssetName() const
{
#ifdef _WIN32
    return QStringLiteral("QuickSnapAudio-Setup.exe");
#else
    return QStringLiteral(".deb");
#endif
}

void UpdateChecker::checkForUpdates(bool silent)
{
    m_silent = silent;

    QNetworkRequest request(QUrl(QString::fromLatin1(GITHUB_API_URL)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "QuickSnapAudio-UpdateChecker");
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReleaseInfoReceived(reply);
    });
}

void UpdateChecker::onReleaseInfoReceived(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        emit updateCheckFailed(err);
        if (!m_silent) {
            QMessageBox::warning(nullptr, "Update Check Failed",
                                 QString("Could not check for updates:\n%1").arg(err));
        }
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit updateCheckFailed("Invalid response from GitHub.");
        if (!m_silent) {
            QMessageBox::warning(nullptr, "Update Check Failed",
                                 "Received an invalid response from GitHub.");
        }
        return;
    }

    QJsonObject release = doc.object();
    QString tagName = release["tag_name"].toString();
    QString body = release["body"].toString();
    QString htmlUrl = release["html_url"].toString();

    if (!isNewerVersion(tagName)) {
        emit noUpdateAvailable();
        if (!m_silent) {
            QMessageBox::information(nullptr, "No Updates",
                                     QString("You are running the latest version (%1).")
                                         .arg(QApplication::applicationVersion()));
        }
        return;
    }

    // Find the matching asset download URL
    QString downloadUrl;
    QString assetName = platformAssetName();
    QJsonArray assets = release["assets"].toArray();
    for (const QJsonValue &val : assets) {
        QJsonObject asset = val.toObject();
        QString name = asset["name"].toString();
#ifdef _WIN32
        if (name == assetName) {
            downloadUrl = asset["browser_download_url"].toString();
            break;
        }
#else
        // On Linux, match any .deb file
        if (name.endsWith(assetName)) {
            downloadUrl = asset["browser_download_url"].toString();
            break;
        }
#endif
    }

    if (downloadUrl.isEmpty()) {
        // No matching asset — fall back to opening the release page
        downloadUrl = htmlUrl;
    }

    emit updateAvailable(tagName, downloadUrl, body);

    // Show update dialog
    QMessageBox msgBox;
    msgBox.setWindowTitle("Update Available");
    msgBox.setText(QString("A new version of QuickSnapAudio is available: <b>%1</b><br>"
                           "You are currently running: <b>%2</b>")
                       .arg(tagName, QApplication::applicationVersion()));
    msgBox.setInformativeText("Would you like to download and install the update?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
        if (downloadUrl == htmlUrl) {
            // No binary asset found, open the release page in browser
            QDesktopServices::openUrl(QUrl(htmlUrl));
        } else {
            downloadAndInstall(downloadUrl);
        }
    }
}

void UpdateChecker::downloadAndInstall(const QString &downloadUrl)
{
    auto *progress = new QProgressDialog("Downloading update...", "Cancel", 0, 100, nullptr);
    progress->setWindowTitle("QuickSnapAudio Update");
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setMinimumDuration(0);
    progress->setValue(0);

    QUrl url(downloadUrl);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::UserAgentHeader, "QuickSnapAudio-UpdateChecker");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this,
            [progress](qint64 received, qint64 total) {
                if (total > 0) {
                    progress->setMaximum(static_cast<int>(total));
                    progress->setValue(static_cast<int>(received));
                }
            });

    connect(progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this, [this, reply, progress]() {
        progress->close();
        progress->deleteLater();
        onDownloadFinished(reply);
    });
}

void UpdateChecker::onDownloadProgress(qint64 received, qint64 total)
{
    Q_UNUSED(received);
    Q_UNUSED(total);
}

void UpdateChecker::onDownloadFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(nullptr, "Download Failed",
                             QString("Failed to download the update:\n%1").arg(reply->errorString()));
        return;
    }

    QByteArray fileData = reply->readAll();
    if (fileData.isEmpty()) {
        QMessageBox::warning(nullptr, "Download Failed", "Downloaded file is empty.");
        return;
    }

    // Determine temp file path
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

#ifdef _WIN32
    QString filePath = QDir(tempDir).filePath("QuickSnapAudio-Setup.exe");
#else
    QString filePath = QDir(tempDir).filePath("quicksnapaudio-update.deb");
#endif

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(nullptr, "Update Failed",
                             QString("Could not write to:\n%1").arg(filePath));
        return;
    }
    file.write(fileData);
    file.close();

#ifdef _WIN32
    // On Windows: write a helper batch script that runs the installer silently,
    // waits for it to finish, then relaunches the app.
    QString appPath = QApplication::applicationFilePath();
    QString batchPath = QDir(tempDir).filePath("quicksnapaudio_update.bat");

    QFile batch(batchPath);
    if (batch.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&batch);
        ts << "@echo off\r\n";
        // Wait a moment for the app to fully exit
        ts << "timeout /t 2 /nobreak >nul\r\n";
        // Run installer silently (NSIS /S flag)
        ts << "\"" << QDir::toNativeSeparators(filePath) << "\" /S\r\n";
        // Wait for installer to finish (it returns when done in silent mode)
        // Then relaunch the application
        ts << "start \"\" \"" << QDir::toNativeSeparators(appPath) << "\"\r\n";
        ts << "del \"%~f0\"\r\n"; // Self-delete the batch file
        batch.close();

        bool launched = QProcess::startDetached("cmd.exe", {"/C", batchPath});
        if (launched) {
            QApplication::quit();
        } else {
            QMessageBox::warning(nullptr, "Update Failed",
                                 "Could not launch the update script. You can run the installer manually from:\n" + filePath);
        }
    } else {
        QMessageBox::warning(nullptr, "Update Failed",
                             "Could not create the update script. You can run the installer manually from:\n" + filePath);
    }
#else
    // On Linux, use pkexec to install the .deb, then restart
    QProcess proc;
    proc.start("pkexec", {"dpkg", "-i", filePath});
    proc.waitForFinished(120000); // 2 min timeout

    if (proc.exitCode() == 0) {
        QMessageBox::information(nullptr, "Update Complete",
                                 "Update installed successfully. The application will now restart.");
        // Restart: launch a new instance and quit
        QString appPath = QApplication::applicationFilePath();
        QProcess::startDetached(appPath, {});
        QApplication::quit();
    } else {
        QString errOutput = proc.readAllStandardError();
        QMessageBox::warning(nullptr, "Update Failed",
                             QString("Installation failed:\n%1\n\nYou can install manually:\nsudo dpkg -i %2")
                                 .arg(errOutput, filePath));
    }
#endif
}
