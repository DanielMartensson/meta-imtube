#pragma once

#include <QObject>
#include <QProcess>
#include <QQuickItem>
#include <QTimer>

#include <atomic>
#include <thread>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

class VideoSinkFactory;
class Settings;
class YtDlpHelper;

// ---------------------------------------------------------------------------
// GStreamer based player. The media is not handed to GStreamer as a URL:
// yt-dlp downloads/stitches the best video+audio stream and writes a single
// MKV byte stream to stdout, which is fed into an appsrc element. This
// guarantees the demuxer receives one contiguous stream (like ImTube).
//
//   yt-dlp --stdout --> appsrc --> decodebin / matroskademux
//        ├── video to qml6glsink / qml6vulkansink   (GPU rendered, HW decode)
//        └── audio --> audioconvert --> volume --> autoaudiosink
//
// Hardware acceleration is the only path: the video is decoded by the
// platform hardware decoder and rendered by the GPU inside the QML scene
// graph. No software decoding or CPU side frame copies are used.
// ---------------------------------------------------------------------------
class GStreamerPlayer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTitleChanged)
    Q_PROPERTY(QString videoId READ videoId NOTIFY videoIdChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString decoder READ decoder WRITE setDecoder NOTIFY decoderChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY stateChanged)

public:
    enum PlaybackState {
        Idle = 0,
        Buffering = 1,
        Playing = 2,
        Paused = 3,
        Error = 4,
        Ended = 5,
    };
    Q_ENUM(PlaybackState)

    GStreamerPlayer(YtDlpHelper *ytDlp,
                    VideoSinkFactory *sinkFactory,
                    Settings *settings,
                    QObject *parent = nullptr);
    ~GStreamerPlayer() override;

    int state() const;
    qint64 position() const;
    qint64 duration() const;
    int volume() const;
    bool muted() const;
    bool hasVideo() const;
    QString currentTitle() const;
    QString videoId() const;
    QString errorMessage() const;
    QString decoder() const;
    void setDecoder(const QString &name);

public slots:
    // Attach the QML GstGLQt6VideoItem that the qml6 sink renders into.
    void setVideoItem(QQuickItem *item);

    void playVideo(QString videoId, QString title);
    void playUrl(QString url, QString title);
    void togglePause();
    void stop();
    void seekTo(qint64 ms);
    void setVolume(int percent);
    void setMuted(bool muted);

signals:
    void stateChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void mutedChanged();
    void currentTitleChanged();
    void videoIdChanged();
    void errorMessageChanged();
    void decoderChanged();
    void videoStarted(QString videoId, QString title);

private:
    void startStream(const QString &videoUrl, qint64 startMs);
    void buildPipeline();
    void stopStreamProcess();
    void runFeeder();
    void pushData(QByteArray data);
    void pullEndOfStream();
    void drainFeedQueue();
    void updatePosition();
    void setState(PlaybackState state);
    void linkStreamPad(GstPad *pad, bool isVideo, bool isAudio);
    bool offendingFactory() const;
    void teardownNewElements();
    void onStdout();
    void onStreamFinished(int exitCode, QProcess::ExitStatus status);

    static void onDecodebinPadAdded(GstElement *element, GstPad *pad, gpointer user);
    static void onDemuxPadAdded(GstElement *element, GstPad *pad, gpointer user);
    static gboolean onBusMessage(GstBus *bus, GstMessage *message, gpointer user);

    YtDlpHelper *m_ytDlp;
    Settings *m_settings;
    VideoSinkFactory *m_sinkFactory;

    GstElement *m_pipeline = nullptr;
    std::atomic<GstElement *> m_appsrc{ nullptr };
    GstElement *m_demux = nullptr;
    GstElement *m_videoQueue = nullptr;
    GstElement *m_h264parse = nullptr;
    GstElement *m_hwDecoder = nullptr;
    GstElement *m_videoSink = nullptr;
    GstElement *m_audioQueue = nullptr;
    GstElement *m_volume = nullptr;
    QQuickItem *m_videoItem = nullptr;
    GstBus *m_bus = nullptr;

    GAsyncQueue *m_feedQueue = nullptr;
    std::thread m_feedThread;
    std::atomic_bool m_pipelineBuilt{ false };

    QProcess *m_streamProc = nullptr;
    QTimer m_positionTimer;
    bool m_playing = false;

    PlaybackState m_state = Idle;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    int m_volumePercent = 100;
    bool m_muted = false;
    QString m_currentTitle;
    QString m_currentVideoId;
    QString m_currentUrl;
    QString m_errorMessage;
    QString m_decoder;
};