#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QUrl>

class Settings;

// ---------------------------------------------------------------------------
// Fetches video thumbnails into a local cache. com.youtube/yt-dlp URLs may
// not be reachable from an embedded device without a proxy; failures are
// swallowed gracefully (the UI falls back to a placeholder).
// ---------------------------------------------------------------------------
class ThumbnailService : public QObject
{
    Q_OBJECT

public:
    explicit ThumbnailService(Settings *settings, QObject *parent = nullptr);

    Q_INVOKABLE void request(const QString &videoId, const QString &remoteUrl);
    Q_INVOKABLE QString cachePath(const QString &videoId) const;

signals:
    void ready(QString videoId, QString path);
    void failed(QString videoId);

private:
    void finish(const QString &videoId, const QString &path, bool ok);
    QNetworkAccessManager m_network;
    Settings *m_settings;
    QSet<QString> m_pending;
    QSet<QString> m_done;
};