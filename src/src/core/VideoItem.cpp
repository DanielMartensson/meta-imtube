#include "VideoItem.h"

#include <QDateTime>
#include <QDebug>

VideoItem::VideoItem(QObject *parent)
    : QObject(parent)
{
}

VideoItem *VideoItem::fromMap(const QVariantMap &map, QObject *parent)
{
    auto *item = new VideoItem(parent);
    item->setVideoId(map.value(QStringLiteral("videoId")).toString());
    item->setTitle(map.value(QStringLiteral("title")).toString());
    item->setUploader(map.value(QStringLiteral("uploader")).toString());
    item->setDurationSec(map.value(QStringLiteral("durationSec")).toInt());
    item->setThumbRemote(map.value(QStringLiteral("thumbRemote")).toString());
    return item;
}

QString VideoItem::videoId() const { return m_videoId; }

void VideoItem::setVideoId(const QString &id)
{
    m_videoId = id;
    if (m_thumbRemote.isEmpty())
        m_thumbRemote = playerUrlFromId(id);
}

QString VideoItem::title() const { return m_title; }
void VideoItem::setTitle(const QString &title) { m_title = title; }

QString VideoItem::uploader() const { return m_uploader; }
void VideoItem::setUploader(const QString &uploader) { m_uploader = uploader; }

int VideoItem::durationSec() const { return m_durationSec; }
void VideoItem::setDurationSec(int sec) { m_durationSec = sec; }

QString VideoItem::durationText() const { return formatDuration(m_durationSec); }

QString VideoItem::thumbRemote() const { return m_thumbRemote; }
void VideoItem::setThumbRemote(const QString &url) { m_thumbRemote = url; }

QString VideoItem::thumbLocal() const { return m_thumbLocal; }
void VideoItem::setThumbLocal(const QString &path)
{
    if (m_thumbLocal == path)
        return;
    m_thumbLocal = path;
    emit thumbLocalChanged();
}

QVariantMap VideoItem::toMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("videoId"), m_videoId);
    map.insert(QStringLiteral("title"), m_title);
    map.insert(QStringLiteral("uploader"), m_uploader);
    map.insert(QStringLiteral("durationSec"), m_durationSec);
    map.insert(QStringLiteral("thumbRemote"), m_thumbRemote);
    map.insert(QStringLiteral("thumbLocal"), m_thumbLocal);
    return map;
}

QString VideoItem::formatDuration(int sec)
{
    if (sec <= 0)
        return QStringLiteral("--:--");
    const int h = sec / 3600;
    const int m = (sec % 3600) / 60;
    const int s = sec % 60;
    if (h > 0)
        return QString::asprintf("%d:%02d:%02d", h, m, s);
    return QString::asprintf("%02d:%02d", m, s);
}

QString VideoItem::playerUrlFromId(const QString &videoId)
{
    // YouTube thumbnail URLs are predictable from the video id; this avoids
    // an extra yt-dlp extraction pass just to fetch an image.
    return QStringLiteral("https://i.ytimg.com/vi/%1/mqdefault.jpg").arg(videoId);
}