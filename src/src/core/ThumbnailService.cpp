#include "ThumbnailService.h"

#include "Settings.h"

#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

ThumbnailService::ThumbnailService(Settings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
}

QString ThumbnailService::cachePath(const QString &videoId) const
{
    const QString dir = QStandardPaths::writableLocation(
            QStandardPaths::CacheLocation) + QStringLiteral("/thumbnails");
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + videoId + QStringLiteral(".jpg");
}

void ThumbnailService::request(const QString &videoId, const QString &remoteUrl)
{
    if (videoId.isEmpty() || remoteUrl.isEmpty())
        return;
    if (m_done.contains(videoId)) {
        emit ready(videoId, cachePath(videoId));
        return;
    }

    const QString local = cachePath(videoId);
    if (QFile::exists(local)) {
        m_done.insert(videoId);
        emit ready(videoId, local);
        return;
    }

    if (m_pending.contains(videoId))
        return;
    m_pending.insert(videoId);

    QNetworkRequest request(QUrl(remoteUrl));
    request.setTransferTimeout(15000);
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, videoId, local]() {
        const bool ok = reply->error() == QNetworkReply::NoError;
        if (ok) {
            QSaveFile file(local);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.commit();
            } else {
                qWarning() << "ThumbnailService: cannot write" << local;
            }
        } else if (reply->error() != QNetworkReply::OperationCanceledError) {
            qWarning() << "ThumbnailService: fetch failed for" << videoId
                       << reply->errorString();
        }
        m_pending.remove(videoId);
        if (ok) {
            m_done.insert(videoId);
            emit ready(videoId, local);
        } else {
            emit failed(videoId);
        }
        reply->deleteLater();
    });
}