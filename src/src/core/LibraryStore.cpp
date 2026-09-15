#include "LibraryStore.h"

#include "VideoItem.h"

#include <algorithm>

#include <QDebug>
#include <QSettings>
#include <QVariant>

namespace {
const QLatin1String kFavoritesKey("library/favorites");
} // namespace

LibraryStore::LibraryStore(QObject *parent)
    : QObject(parent)
{
}

bool LibraryStore::isFavorite(const QString &videoId) const
{
    const QList<QVariantMap> items = load();
    for (const QVariantMap &item : items) {
        if (item.value(QStringLiteral("videoId")).toString() == videoId)
            return true;
    }
    return false;
}

void LibraryStore::toggleFavorite(const QString &videoId, const QString &title,
                                  const QString &uploader, int durationSec,
                                  const QString &thumbRemote)
{
    if (isFavorite(videoId)) {
        removeFavorite(videoId);
        return;
    }

    VideoItem item;
    item.setVideoId(videoId);
    item.setTitle(title);
    item.setUploader(uploader);
    item.setDurationSec(durationSec);
    item.setThumbRemote(thumbRemote);

    QList<QVariantMap> items = load();
    items.prepend(item.toMap());
    save(items);
    emit favoritesChanged();
}

void LibraryStore::removeFavorite(const QString &videoId)
{
    QList<QVariantMap> items = load();
    const int before = items.size();
    items.erase(std::remove_if(items.begin(), items.end(),
                               [videoId](const QVariantMap &m) {
                                   return m.value(QStringLiteral("videoId")).toString()
                                           == videoId;
                               }),
                items.end());
    if (items.size() == before)
        return;

    save(items);
    emit favoritesChanged();
}

QVariantList LibraryStore::favorites() const
{
    const QList<QVariantMap> items = load();
    QVariantList list;
    for (const QVariantMap &item : items)
        list.append(item);
    return list;
}

int LibraryStore::count() const
{
    return load().size();
}

void LibraryStore::save(const QList<QVariantMap> &items)
{
    m_items = items;
    m_loaded = true;

    QVariantList list;
    for (const QVariantMap &item : items)
        list.append(item);
    QSettings settings;
    settings.setValue(kFavoritesKey, list);
}

QList<QVariantMap> LibraryStore::load() const
{
    if (m_loaded)
        return m_items;

    QSettings settings;
    const QVariant value = settings.value(kFavoritesKey);
    if (value.isValid() && value.typeId() == QMetaType::QVariantList) {
        for (const QVariant &v : value.toList())
            m_items.append(v.toMap());
    }
    m_loaded = true;
    return m_items;
}