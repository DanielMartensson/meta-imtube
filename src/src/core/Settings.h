#pragma once

#include <QObject>
#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
    Q_PROPERTY(QString ytDlpPath READ ytDlpPath WRITE setYtDlpPath NOTIFY ytDlpPathChanged)
    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)
    Q_PROPERTY(QString decoder READ decoder WRITE setDecoder NOTIFY decoderChanged)
    Q_PROPERTY(int maxResults READ maxResults WRITE setMaxResults NOTIFY maxResultsChanged)
    Q_PROPERTY(QString rendererDescription READ rendererDescription NOTIFY rendererDescriptionChanged)

public:
    explicit Settings(QObject *parent = nullptr);

    int resolution() const;
    void setResolution(int height);

    QString ytDlpPath() const;
    void setYtDlpPath(const QString &path);

    bool darkTheme() const;
    void setDarkTheme(bool dark);

    QString decoder() const;
    void setDecoder(const QString &name);

    int maxResults() const;
    void setMaxResults(int count);

    QString rendererDescription() const;
    void setRendererDescription(const QString &text);

    QStringList prefferedDecoderNames() const;

signals:
    void resolutionChanged();
    void ytDlpPathChanged();
    void darkThemeChanged();
    void decoderChanged();
    void maxResultsChanged();
    void rendererDescriptionChanged();

private:
    QSettings m_settings;
    QString m_rendererDescription;
};