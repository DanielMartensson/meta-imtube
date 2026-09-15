#pragma once

#include <QObject>
#include <QString>

class VideoItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString videoId READ videoId CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString uploader READ uploader CONSTANT)
    Q_PROPERTY(int durationSec READ durationSec CONSTANT)
    Q_PROPERTY(QString durationText READ durationText CONSTANT)
    Q_PROPERTY(QString thumbRemote READ thumbRemote CONSTANT)
    Q_PROPERTY(QString thumbLocal READ thumbLocal WRITE setThumbLocal NOTIFY thumbLocalChanged)

public:
    explicit VideoItem(QObject *parent = nullptr);

    static VideoItem *fromMap(const QVariantMap &map, QObject *parent = nullptr);

    QString videoId() const;
    void setVideoId(const QString &id);

    QString title() const;
    void setTitle(const QString &title);

    QString uploader() const;
    void setUploader(const QString &uploader);

    int durationSec() const;
    void setDurationSec(int sec);

    QString durationText() const;

    QString thumbRemote() const;
    void setThumbRemote(const QString &url);

    QString thumbLocal() const;
    void setThumbLocal(const QString &path);

    QVariantMap toMap() const;

signals:
    void thumbLocalChanged();

public:
    // Predictable thumbnail URL for a video id (https://i.ytimg.com/vi/...).
    static QString playerUrlFromId(const QString &videoId);

private:
    static QString formatDuration(int sec);

    QString m_videoId;
    QString m_title;
    QString m_uploader;
    int m_durationSec = 0;
    QString m_thumbRemote;
    QString m_thumbLocal;
};