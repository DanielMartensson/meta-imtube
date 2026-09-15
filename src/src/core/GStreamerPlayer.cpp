#include "GStreamerPlayer.h"

#include "Settings.h"
#include "VideoSinkFactory.h"
#include "YtDlpHelper.h"

#include <QDebug>

namespace {

// Feed queue sentinels: never collide with real GBytes pointers.
constexpr intptr_t kStopSentinel = 1;
constexpr intptr_t kEosSentinel = 2;

} // namespace

// ---------------------------------------------------------------------------

GStreamerPlayer::GStreamerPlayer(YtDlpHelper *ytDlp,
                                 VideoSinkFactory *sinkFactory,
                                 Settings *settings,
                                 QObject *parent)
    : QObject(parent)
    , m_ytDlp(ytDlp)
    , m_sinkFactory(sinkFactory)
    , m_settings(settings)
    , m_decoder(settings->decoder())
{
    m_feedQueue = g_async_queue_new();
    m_feedThread = std::thread(&GStreamerPlayer::runFeeder, this);

    m_positionTimer.setInterval(250);
    connect(&m_positionTimer, &QTimer::timeout, this, &GStreamerPlayer::updatePosition);

    connect(this, &GStreamerPlayer::stateChanged, this, [this]() {
        if (m_state == Playing || m_state == Buffering)
            m_positionTimer.start();
        else
            m_positionTimer.stop();
    });
}

GStreamerPlayer::~GStreamerPlayer()
{
    stop();
    g_async_queue_push(m_feedQueue, reinterpret_cast<gpointer>(kStopSentinel));
    if (m_feedThread.joinable())
        m_feedThread.join();
    g_async_queue_unref(m_feedQueue);
}

// ---------------------------------------------------------------------------
// QML / public API
// ---------------------------------------------------------------------------

int GStreamerPlayer::state() const { return m_state; }
qint64 GStreamerPlayer::position() const { return m_position; }
qint64 GStreamerPlayer::duration() const { return m_duration; }
int GStreamerPlayer::volume() const { return m_volumePercent; }
bool GStreamerPlayer::muted() const { return m_muted; }
bool GStreamerPlayer::hasVideo() const { return m_state != Idle && m_state != Error; }
QString GStreamerPlayer::currentTitle() const { return m_currentTitle; }
QString GStreamerPlayer::videoId() const { return m_currentVideoId; }
QString GStreamerPlayer::errorMessage() const { return m_errorMessage; }
QString GStreamerPlayer::decoder() const { return m_decoder; }

void GStreamerPlayer::setDecoder(const QString &name)
{
    if (m_decoder == name)
        return;
    m_decoder = name;
    m_settings->setDecoder(name);
    emit decoderChanged();
}

void GStreamerPlayer::setVideoItem(QQuickItem *item)
{
    m_videoItem = item;
    if (m_videoSink && m_videoItem) {
        g_object_set(m_videoSink, "widget", m_videoItem, nullptr);
        qInfo() << "ImTube: QML video item attached to the GStreamer sink";
    }
}

void GStreamerPlayer::playVideo(QString videoId, QString title)
{
    m_currentVideoId = videoId;
    m_currentTitle = title;
    emit videoIdChanged();
    emit currentTitleChanged();

    const QString url =
            QStringLiteral("https://www.youtube.com/watch?v=%1").arg(videoId);
    startStream(url, 0);
    emit videoStarted(videoId, title);
}

void GStreamerPlayer::playUrl(QString url, QString title)
{
    m_currentVideoId.clear();
    m_currentTitle = title;
    emit videoIdChanged();
    emit currentTitleChanged();

    startStream(url, 0);
}

void GStreamerPlayer::togglePause()
{
    if (!m_pipeline)
        return;
    if (m_state == Paused && m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
        m_playing = true;
        m_positionTimer.start();
        setState(Playing);
    } else if (m_state == Playing || m_state == Buffering) {
        gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
        m_playing = false;
        setState(Paused);
    }
}

void GStreamerPlayer::stop()
{
    stopStreamProcess();
    m_pipelineBuilt.store(false);

    if (m_pipeline) {
        m_playing = false;
        if (m_bus) {
            gst_bus_remove_watch(m_bus);
            gst_object_unref(m_bus);
            m_bus = nullptr;
        }
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
        m_appsrc.store(nullptr);
    }
    m_videoSink = nullptr;
    m_demux = nullptr;
    m_videoQueue = nullptr;
    m_audioQueue = nullptr;
    m_h264parse = nullptr;
    m_hwDecoder = nullptr;
    m_volume = nullptr;

    drainFeedQueue();

    m_position = 0;
    m_duration = 0;
    emit positionChanged();
    emit durationChanged();
    setState(Idle);
}

void GStreamerPlayer::seekTo(qint64 ms)
{
    if (m_state != Playing && m_state != Paused)
        return;
    const QString url = m_currentUrl;
    stop();
    m_position = ms;
    emit positionChanged();
    if (!url.isEmpty())
        startStream(url, ms);
}

void GStreamerPlayer::setVolume(int percent)
{
    m_volumePercent = qBound(0, percent, 100);
    if (m_volume) {
        g_object_set(m_volume, "volume", m_volumePercent / 100.0, nullptr);
        g_object_set(m_volume, "mute", m_muted, nullptr);
    }
    emit volumeChanged();
}

void GStreamerPlayer::setMuted(bool muted)
{
    m_muted = muted;
    if (m_volume)
        g_object_set(m_volume, "mute", muted, nullptr);
    emit mutedChanged();
}

// ---------------------------------------------------------------------------
// Stream start
// ---------------------------------------------------------------------------

void GStreamerPlayer::startStream(const QString &videoUrl, qint64 startMs)
{
    m_currentUrl = videoUrl;
    stop();

    if (!m_sinkFactory->sinkAvailable()) {
        m_errorMessage = QStringLiteral(
                "QML video sink not available. Rebuild gst-plugins-good with "
                "the qt6 plugin enabled.");
        emit errorMessageChanged();
        setState(Error);
        return;
    }

    m_errorMessage.clear();
    emit errorMessageChanged();

    buildPipeline();
    if (!m_pipeline) {
        setState(Error);
        return;
    }

    QProcess *proc = m_ytDlp->startStream(videoUrl, m_settings->resolution(), startMs);
    m_streamProc = proc;
    connect(proc, &QProcess::readyReadStandardOutput, this, &GStreamerPlayer::onStdout);
    connect(proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus status) {
                Q_UNUSED(exitCode)
                Q_UNUSED(status)
                onStreamFinished(exitCode, status);
            });

    proc->start();
    if (proc->state() == QProcess::NotRunning) {
        const QString stderrText = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        if (proc->error() == QProcess::FailedToStart) {
            m_errorMessage = QStringLiteral(
                    "yt-dlp could not be started. Run the install script or "
                    "set the path in Settings. Details: ") + stderrText;
        } else {
            m_errorMessage = QStringLiteral("Stream process error: %1").arg(stderrText);
        }
        emit errorMessageChanged();
        setState(Error);
        return;
    }

    m_playing = true;
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    setState(Buffering);
    if (startMs <= 0)
        updatePosition();
}

void GStreamerPlayer::onStdout()
{
    if (m_streamProc)
        pushData(m_streamProc->readAllStandardOutput());
}

void GStreamerPlayer::onStreamFinished(int exitCode, QProcess::ExitStatus status)
{
    QProcess *proc = m_streamProc;
    if (!proc)
        return;

    // Drain whatever is still buffered between the downloaded chunks.
    pushData(proc->readAllStandardOutput());

    const bool ok = status == QProcess::NormalExit && exitCode == 0;
    if (!ok) {
        const QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_errorMessage = QStringLiteral("yt-dlp failed (code %1): %2")
                                 .arg(exitCode).arg(err);
        emit errorMessageChanged();
        qWarning() << "ImTube: yt-dlp exited with code" << exitCode
                   << "- stderr:" << err;
    }

    // Tell the feeder to signal EOS after the last buffer. On an abnormal
    // exit the EOS handler turns into an Error state (see onBusMessage) so
    // the user sees why playback stopped.
    pullEndOfStream();

    m_streamProc = nullptr;
    proc->deleteLater();
}

// ---------------------------------------------------------------------------
// Pipeline construction
// ---------------------------------------------------------------------------

void GStreamerPlayer::buildPipeline()
{
    m_pipeline = gst_pipeline_new("imtube-pipeline");

    GstElement *appsrc = gst_element_factory_make("appsrc", "src");
    m_appsrc.store(appsrc);

    const QString requested = m_settings->decoder();
    const bool forcedDecoder = !requested.isEmpty()
            && requested != QLatin1String("auto")
            && gst_element_factory_find(qPrintable(requested)) != nullptr;
    if (!requested.isEmpty() && requested != QLatin1String("auto") && !forcedDecoder)
        qWarning() << "ImTube: decoder" << requested << "not available, using decodebin";

    // yt-dlp always remuxes to a single-track MKV on stdout, so the demuxer
    // (decodebin or explicit matroskademux) is hinted with the container caps.
    GstCaps *mkvCaps = gst_caps_from_string("video/x-matroska");
    gst_app_src_set_caps(GST_APP_SRC(m_appsrc.load()), mkvCaps);
    gst_caps_unref(mkvCaps);

    if (forcedDecoder) {
        // Single-track MKV (remuxed by yt-dlp). Decodebin would also work,
        // but the explicit demuxer lets us attach a forced hardware decoder.
        m_demux = gst_element_factory_make("matroskademux", "demux");
        m_h264parse = gst_element_factory_make("h264parse", "parse");
        m_hwDecoder = gst_element_factory_make(qPrintable(requested), "decoder");
    } else {
        m_demux = gst_element_factory_make("decodebin", "decodebin");
    }

    m_videoQueue = gst_element_factory_make("queue", "video-queue");
    m_audioQueue = gst_element_factory_make("queue", "audio-queue");
    m_videoSink = gst_element_factory_make(qPrintable(m_sinkFactory->sinkElement()),
                                           "videosink");
    GstElement *audconvert = gst_element_factory_make("audioconvert", "audioconvert");
    GstElement *audresamp = gst_element_factory_make("audioresample", "audioresample");
    m_volume = gst_element_factory_make("volume", "volume");
    GstElement *audsink = gst_element_factory_make("autoaudiosink", "audiosink");

    if (offendingFactory()) {
        teardownNewElements();
        return;
    }

    if (forcedDecoder) {
        gst_bin_add_many(GST_BIN(m_pipeline),
                         m_appsrc.load(), m_demux,
                         m_videoQueue, m_h264parse, m_hwDecoder, m_videoSink,
                         m_audioQueue, audconvert, audresamp, m_volume, audsink, nullptr);

        gst_element_link_many(m_videoQueue, m_h264parse, m_hwDecoder, m_videoSink, nullptr);
        g_signal_connect(m_demux, "pad-added", G_CALLBACK(onDemuxPadAdded), this);
    } else {
        gst_bin_add_many(GST_BIN(m_pipeline),
                         m_appsrc.load(), m_demux,
                         m_videoQueue, m_videoSink,
                         m_audioQueue, audconvert, audresamp, m_volume, audsink, nullptr);

        gst_element_link_many(m_videoQueue, m_videoSink, nullptr);
        g_signal_connect(m_demux, "pad-added", G_CALLBACK(onDecodebinPadAdded), this);
    }

    gst_element_link_many(m_audioQueue, audconvert, audresamp, m_volume, audsink, nullptr);
    if (!gst_element_link(m_appsrc.load(), m_demux))
        qWarning() << "ImTube: failed to link appsrc to demuxer";

    gst_app_src_set_stream_type(GST_APP_SRC(m_appsrc.load()), GST_APP_STREAM_TYPE_STREAM);
    g_object_set(m_appsrc.load(), "format", GST_FORMAT_TIME, nullptr);

    if (m_videoSink) {
        g_object_set(m_videoSink, "force-aspect-ratio", TRUE, nullptr);
        if (m_videoItem)
            g_object_set(m_videoSink, "widget", m_videoItem, nullptr);
    }
    if (m_volume) {
        g_object_set(m_volume, "volume", m_volumePercent / 100.0, nullptr);
        g_object_set(m_volume, "mute", m_muted, nullptr);
    }

    m_bus = gst_element_get_bus(m_pipeline);
    gst_bus_add_watch(m_bus, onBusMessage, this);

    m_pipelineBuilt.store(true);
}

bool GStreamerPlayer::offendingFactory() const
{
    const GstElement *const elems[] = { m_appsrc.load(), m_demux, m_videoQueue,
                                        m_audioQueue, m_videoSink, m_volume,
                                        m_h264parse, m_hwDecoder };
    for (GstElement *e : elems) {
        if (!e) {
            qCritical() << "ImTube: required GStreamer element could not be created";
            return true;
        }
    }
    return false;
}

// Unref only elements that were never added to the pipeline bin.
void GStreamerPlayer::teardownNewElements()
{
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
    // In the failure path nothing was added to the bin yet, so we hold the
    // only references; release every created non-null element.
    GstElement *elements[] = { m_appsrc.load(), m_demux, m_videoQueue, m_audioQueue,
                               m_videoSink, m_volume, m_h264parse, m_hwDecoder };
    for (GstElement *e : elements) {
        if (e)
            gst_object_unref(e);
    }
    m_appsrc.store(nullptr);
    m_demux = nullptr;
    m_videoQueue = nullptr;
    m_audioQueue = nullptr;
    m_videoSink = nullptr;
    m_volume = nullptr;
    m_h264parse = nullptr;
    m_hwDecoder = nullptr;
}

void GStreamerPlayer::stopStreamProcess()
{
    if (!m_streamProc)
        return;
    QProcess *p = m_streamProc;
    m_streamProc = nullptr; // the finished-signal handler re-enters here
    p->kill();
    p->waitForFinished(500);
    p->deleteLater();
}

// ---------------------------------------------------------------------------
// appsrc feeder thread (single thread, whole app lifetime)
// ---------------------------------------------------------------------------

void GStreamerPlayer::runFeeder()
{
    for (;;) {
        gpointer p = g_async_queue_pop(m_feedQueue);
        const intptr_t tag = reinterpret_cast<intptr_t>(p);

        if (tag == kStopSentinel)
            break;

        if (tag == kEosSentinel) {
            GstElement *a = m_appsrc.load();
            if (m_pipelineBuilt.load() && a)
                gst_app_src_end_of_stream(GST_APP_SRC(a));
            continue;
        }

        if (!p)
            continue;

        GBytes *bytes = static_cast<GBytes *>(p);
        GstElement *a = m_appsrc.load();
        if (m_pipelineBuilt.load() && a) {
            GstBuffer *buf = gst_buffer_new_wrapped_full(
                    GST_MEMORY_FLAG_READONLY,
                    const_cast<gpointer>(g_bytes_get_data(bytes)),
                    g_bytes_get_size(bytes), 0, g_bytes_get_size(bytes),
                    bytes, reinterpret_cast<GDestroyNotify>(g_bytes_unref));
            const GstFlowReturn fr = gst_app_src_push_buffer(GST_APP_SRC(a), buf);
            if (fr != GST_FLOW_OK && fr != GST_FLOW_FLUSHING)
                qWarning() << "ImTube: appsrc flow" << fr;
        } else {
            g_bytes_unref(bytes);
        }
    }
}

void GStreamerPlayer::pushData(QByteArray data)
{
    if (data.isEmpty() || !m_feedQueue)
        return;
    GBytes *bytes = g_bytes_new(data.constData(), data.size());
    g_async_queue_push(m_feedQueue, bytes);
}

void GStreamerPlayer::pullEndOfStream()
{
    g_async_queue_push(m_feedQueue, reinterpret_cast<gpointer>(kEosSentinel));
}

void GStreamerPlayer::drainFeedQueue()
{
    gpointer p = nullptr;
    while ((p = g_async_queue_try_pop(m_feedQueue)) != nullptr) {
        const intptr_t tag = reinterpret_cast<intptr_t>(p);
        if (tag != kStopSentinel && tag != kEosSentinel)
            g_bytes_unref(static_cast<GBytes *>(p));
    }
}

// ---------------------------------------------------------------------------
// Position / state
// ---------------------------------------------------------------------------

void GStreamerPlayer::updatePosition()
{
    if (!m_pipeline || !m_playing)
        return;

    gint64 pos = 0;
    gint64 dur = 0;

    if (gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &pos)) {
        const qint64 ms = pos / GST_MSECOND;
        if (ms && m_state == Buffering)
            setState(Playing);
        if (ms != m_position) {
            m_position = ms;
            emit positionChanged();
        }
    }
    if (gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &dur)) {
        const qint64 ms = dur / GST_MSECOND;
        if (ms != m_duration) {
            m_duration = ms;
            emit durationChanged();
        }
    }
}

void GStreamerPlayer::setState(PlaybackState state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// Stream pad routing (GUI/main thread via bus or pad callbacks)
// ---------------------------------------------------------------------------

void GStreamerPlayer::linkStreamPad(GstPad *pad, bool isVideo, bool isAudio)
{
    if (!m_videoQueue || !m_audioQueue)
        return;

    GstPad *target = nullptr;
    if (isVideo)
        target = gst_element_get_static_pad(m_videoQueue, "sink");
    else if (isAudio)
        target = gst_element_get_static_pad(m_audioQueue, "sink");

    if (!target) {
        qWarning() << "ImTube: could not route pad" << gst_pad_get_name(pad);
        return;
    }
    if (gst_pad_link(pad, target) != GST_PAD_LINK_OK)
        qWarning() << "ImTube: failed to link pad" << gst_pad_get_name(pad);
    gst_object_unref(target);
}

void GStreamerPlayer::onDecodebinPadAdded(GstElement *element, GstPad *pad, gpointer user)
{
    Q_UNUSED(element)
    auto *self = static_cast<GStreamerPlayer *>(user);
    if (self->m_state == Idle || self->m_state == Error || self->m_state == Ended)
        return;
    if (gst_pad_is_linked(pad))
        return;

    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        qWarning() << "ImTube: decodebin pad without caps";
        return;
    }

    bool isVideo = false;
    bool isAudio = false;
    if (gst_caps_get_size(caps) > 0) {
        const GstStructure *s = gst_caps_get_structure(caps, 0);
        const gchar *name = gst_structure_get_name(s);
        isVideo = g_str_has_prefix(name, "video/");
        isAudio = g_str_has_prefix(name, "audio/");
    }
    gst_caps_unref(caps);

    if (isVideo || isAudio)
        self->linkStreamPad(pad, isVideo, isAudio);
}

void GStreamerPlayer::onDemuxPadAdded(GstElement *element, GstPad *pad, gpointer user)
{
    Q_UNUSED(element)
    auto *self = static_cast<GStreamerPlayer *>(user);
    if (self->m_state == Idle || self->m_state == Error || self->m_state == Ended)
        return;
    if (gst_pad_is_linked(pad))
        return;

    const gchar *name = gst_pad_get_name(pad);
    const bool isVideo = g_str_has_prefix(name, "video_");
    const bool isAudio = g_str_has_prefix(name, "audio_");
    if (isVideo || isAudio)
        self->linkStreamPad(pad, isVideo, isAudio);
}

gboolean GStreamerPlayer::onBusMessage(GstBus *bus, GstMessage *message, gpointer user)
{
    Q_UNUSED(bus)
    auto *self = static_cast<GStreamerPlayer *>(user);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        gchar *err = nullptr;
        gchar *dbg = nullptr;
        gst_message_parse_error(message, &err, &dbg);
        self->m_errorMessage = QString::fromUtf8(err);
        g_free(err);
        g_free(dbg);
        emit self->errorMessageChanged();
        qCritical() << "ImTube: pipeline error:" << self->m_errorMessage;
        self->setState(Error);
        self->stop();
        break;
    }
    case GST_MESSAGE_EOS:
        if (self->m_position <= 0 && !self->m_errorMessage.isEmpty())
            self->setState(Error);
        else
            self->setState(Ended);
        self->stop();
        break;
    case GST_MESSAGE_STATE_CHANGED:
        if (self->m_pipeline && GST_MESSAGE_SRC(message) == GST_OBJECT(self->m_pipeline)) {
            GstState oldState, curState;
            gst_message_parse_state_changed(message, &oldState, &curState, nullptr);
            if (curState == GST_STATE_PLAYING
                && (self->m_state == Buffering || self->m_state == Idle)) {
                self->setState(Playing);
            }
        }
        break;
    default:
        break;
    }
    return G_SOURCE_CONTINUE;
}