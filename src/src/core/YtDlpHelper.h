#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>

class Settings;

// ---------------------------------------------------------------------------
// Thin wrapper around the yt-dlp helper tool. Everything happens
// asynchronously via QProcess; results are delivered with signals on the
// GUI thread.
// ---------------------------------------------------------------------------
class YtDlpHelper : public QObject
{
    Q_OBJECT

public:
    explicit YtDlpHelper(Settings *settings, QObject *parent = nullptr);

    // Spawn: yt-dlp --flat-playlist --dump-single-json ytsearchN:query
    void search(const QString &query);
    bool searchRunning() const;

    // Spawn a fetch of a video's metadata (title, duration, thumbnail...).
    void fetchMetadata(const QString &videoUrl);

    // Start streaming a video to stdout. Returns the process (stdout open)
    // or nullptr on failure (yt-dlp missing). The process object is owned
    // by the caller.
    QProcess *startStream(const QString &videoUrl, int maxHeight, qint64 startMs);

    QString formatFor(int maxHeight) const;

    // Query the yt-dlp version (non blocking: error → empty string).
    QString version();

signals:
    void searchFinished(const QVariantList &videos);
    void searchFailed(const QString &error);
    void metadataReady(const QVariantMap &meta);
    void metadataFailed(const QString &error);

private:
    static QString ytDlpExecutable();

    Settings *m_settings;
    QProcess *m_searchProcess = nullptr;
    QProcess *m_metaProcess = nullptr;
};