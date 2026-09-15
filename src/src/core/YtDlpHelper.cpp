#include "YtDlpHelper.h"

#include "Settings.h"
#include "VideoItem.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {
QString msToTimestamp(qint64 ms)
{
    const qint64 totalSec = ms / 1000;
    const qint64 h = totalSec / 3600;
    const qint64 m = (totalSec % 3600) / 60;
    const qint64 s = totalSec % 60;
    return QString::asprintf("%02lld:%02lld:%02lld", h, m, s);
}
} // namespace

YtDlpHelper::YtDlpHelper(Settings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
}

bool YtDlpHelper::searchRunning() const
{
    return m_searchProcess && m_searchProcess->state() != QProcess::NotRunning;
}

void YtDlpHelper::search(const QString &query)
{
    if (searchRunning()) {
        m_searchProcess->kill();
        m_searchProcess->waitForFinished(500);
    }

    const int max = m_settings->maxResults();
    const QString selector = QStringLiteral("ytsearch%1:%2").arg(max).arg(query);

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, query](int code, QProcess::ExitStatus status) {
                const QByteArray out = proc->readAllStandardOutput();
                const QByteArray err = proc->readAllStandardError();
                proc->deleteLater();
                if (m_searchProcess == proc)
                    m_searchProcess = nullptr;

                if (code != 0 || status != QProcess::NormalExit) {
                    emit searchFailed(QString::fromUtf8(err).trimmed());
                    return;
                }

                QVariantList videos;
                const QJsonDocument doc = QJsonDocument::fromJson(out);
                if (doc.isNull()) {
                    qWarning() << "yt-dlp search returned invalid JSON for" << query;
                    emit searchFailed(QStringLiteral("yt-dlp returned no data"));
                    return;
                }

                const QJsonArray entries = doc.object().value(QLatin1String("entries"))
                        .toArray(doc.array());
                for (const QJsonValue &value : entries) {
                    const QJsonObject obj = value.toObject();
                    if (obj.value(QLatin1String("id")).toString().isEmpty())
                        continue;
                    VideoItem item;
                    item.setVideoId(obj.value(QLatin1String("id")).toString());
                    item.setTitle(obj.value(QLatin1String("title")).toString());
                    item.setUploader(
                            obj.value(QLatin1String("channel")).toString(
                                    obj.value(QLatin1String("uploader")).toString()));
                    item.setDurationSec(obj.value(QLatin1String("duration")).toInt());
                    videos.append(item.toMap());
                    if (videos.size() >= max)
                        break;
                }
                emit searchFinished(videos);
            });

    proc->setProgram(m_settings->ytDlpPath());
    proc->setArguments({
        QStringLiteral("--flat-playlist"),
        QStringLiteral("--no-warnings"),
        QStringLiteral("--dump-single-json"),
        selector,
    });
    m_searchProcess = proc;
    proc->start();
}

void YtDlpHelper::fetchMetadata(const QString &videoUrl)
{
    if (m_metaProcess) {
        m_metaProcess->kill();
        m_metaProcess->waitForFinished(500);
    }

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, videoUrl](int code, QProcess::ExitStatus) {
                const QByteArray out = proc->readAllStandardOutput();
                const QByteArray err = proc->readAllStandardError();
                proc->deleteLater();
                if (m_metaProcess == proc)
                    m_metaProcess = nullptr;

                if (code != 0) {
                    emit metadataFailed(QString::fromUtf8(err).trimmed());
                    return;
                }

                const QJsonDocument doc = QJsonDocument::fromJson(out);
                if (doc.isNull() || !doc.isObject()) {
                    emit metadataFailed(QStringLiteral("invalid metadata JSON"));
                    return;
                }

                const QJsonObject obj = doc.object();
                QVariantMap map;
                map.insert(QStringLiteral("videoId"), obj.value(QLatin1String("id")).toString());
                map.insert(QStringLiteral("title"), obj.value(QLatin1String("title")).toString());
                map.insert(QStringLiteral("uploader"),
                           obj.value(QLatin1String("channel")).toString(
                                   obj.value(QLatin1String("uploader")).toString()));
                map.insert(QStringLiteral("durationSec"),
                           obj.value(QLatin1String("duration")).toInt());
                map.insert(QStringLiteral("thumbRemote"), VideoItem::playerUrlFromId(
                        obj.value(QLatin1String("id")).toString()));
                emit metadataReady(map);
            });

    proc->setProgram(m_settings->ytDlpPath());
    proc->setArguments({
        QStringLiteral("--no-playlist"),
        QStringLiteral("--no-warnings"),
        QStringLiteral("--dump-single-json"),
        videoUrl,
    });
    m_metaProcess = proc;
    proc->start();
}

QProcess *YtDlpHelper::startStream(const QString &videoUrl, int maxHeight, qint64 startMs)
{
    auto *proc = new QProcess(this);
    proc->setProgram(m_settings->ytDlpPath());
    QStringList args;
    args << QStringLiteral("-f") << formatFor(maxHeight)
         << QStringLiteral("--merge-output-format") << QStringLiteral("mkv")
         << QStringLiteral("--no-playlist")
         << QStringLiteral("--no-warnings")
         << QStringLiteral("-o") << QStringLiteral("-");
    if (startMs > 0)
        args << QStringLiteral("--download-sections")
             << QStringLiteral("*%1-").arg(msToTimestamp(startMs));
    args << videoUrl;
    proc->setArguments(args);
    return proc;
}

QString YtDlpHelper::formatFor(int maxHeight) const
{
    // Single progressive (avc1 + m4a) stream up to 360p for instant start;
    // above that merge the best separate DASH video/audio tracks into MKV.
    // The forced codec (h264/avc1) matches the hardware decoders present on
    // embedded targets (VPU / V4L2 M2M).
    if (maxHeight <= 360) {
        return QStringLiteral("18/b[height<=%1]").arg(maxHeight);
    }
    return QStringLiteral("bv*[height<=%1][vcodec^=avc1][ext=mp4]+ba[ext=m4a]/b[height<=%1][vcodec^=avc1]")
            .arg(maxHeight);
}

QString YtDlpHelper::version()
{
    QProcess proc;
    proc.setProgram(m_settings->ytDlpPath());
    proc.setArguments({ QStringLiteral("--version") });
    proc.start();
    if (!proc.waitForStarted(1000) || !proc.waitForFinished(2000))
        return QString();
    return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
}