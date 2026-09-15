#pragma once

#include <QList>
#include <QObject>
#include <QVariantMap>

// ---------------------------------------------------------------------------
// Persisted "favorites" library. QSettings storage (~/.config/...).
// QML mirrors the list and refreshes on favoritesChanged().
// ---------------------------------------------------------------------------
class LibraryStore : public QObject
{
    Q_OBJECT

public:
    explicit LibraryStore(QObject *parent = nullptr);

    Q_INVOKABLE bool isFavorite(const QString &videoId) const;
    Q_INVOKABLE void toggleFavorite(const QString &videoId,
                                    const QString &title,
                                    const QString &uploader,
                                    int durationSec,
                                    const QString &thumbRemote);
    Q_INVOKABLE void removeFavorite(const QString &videoId);

    // VideoItem.toMap()-compatible maps, ready for VideoListModel::setVideos.
    QVariantList favorites() const;
    int count() const;

signals:
    void favoritesChanged();

private:
    void save(const QList<QVariantMap> &items);
    QList<QVariantMap> load() const;

    mutable QList<QVariantMap> m_items;
    mutable bool m_loaded = false;
};