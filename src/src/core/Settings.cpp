#include "Settings.h"

Settings::Settings(QObject *parent)
    : QObject(parent)
{
}

int Settings::resolution() const
{
    return m_settings.value(QStringLiteral("player/resolution"), 720).toInt();
}

void Settings::setResolution(int height)
{
    if (resolution() == height)
        return;
    m_settings.setValue(QStringLiteral("player/resolution"), height);
    emit resolutionChanged();
}

QString Settings::ytDlpPath() const
{
    return m_settings.value(QStringLiteral("ytdlp/path"), QStringLiteral("yt-dlp")).toString();
}

void Settings::setYtDlpPath(const QString &path)
{
    if (ytDlpPath() == path)
        return;
    m_settings.setValue(QStringLiteral("ytdlp/path"), path);
    emit ytDlpPathChanged();
}

bool Settings::darkTheme() const
{
    return m_settings.value(QStringLiteral("ui/darkTheme"), true).toBool();
}

void Settings::setDarkTheme(bool dark)
{
    if (darkTheme() == dark)
        return;
    m_settings.setValue(QStringLiteral("ui/darkTheme"), dark);
    emit darkThemeChanged();
}

QString Settings::decoder() const
{
    return m_settings.value(QStringLiteral("player/decoder"), QStringLiteral("auto")).toString();
}

void Settings::setDecoder(const QString &name)
{
    if (decoder() == name)
        return;
    m_settings.setValue(QStringLiteral("player/decoder"), name);
    emit decoderChanged();
}

int Settings::maxResults() const
{
    return m_settings.value(QStringLiteral("search/maxResults"), 24).toInt();
}

void Settings::setMaxResults(int count)
{
    if (maxResults() == count)
        return;
    m_settings.setValue(QStringLiteral("search/maxResults"), count);
    emit maxResultsChanged();
}

QString Settings::rendererDescription() const
{
    return m_rendererDescription;
}

void Settings::setRendererDescription(const QString &text)
{
    if (m_rendererDescription == text)
        return;
    m_rendererDescription = text;
    emit rendererDescriptionChanged();
}

QStringList Settings::prefferedDecoderNames() const
{
    // Hardware decoders likely to be present on the target (STM32MPx VPU,
    // i.MX, Raspberry Pi ...) probed in order.
    return QStringList()
            << QStringLiteral("v4l2slh264dec")
            << QStringLiteral("v4l2h264dec")
            << QStringLiteral("avdec_h264");
}