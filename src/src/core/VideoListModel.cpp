#include "VideoListModel.h"

#include "VideoItem.h"

VideoListModel::VideoListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

VideoListModel::~VideoListModel()
{
    qDeleteAll(m_items);
}

int VideoListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant VideoListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const VideoItem *item = m_items.at(index.row());
    switch (role) {
    case VideoIdRole:
        return item->videoId();
    case TitleRole:
        return item->title();
    case UploaderRole:
        return item->uploader();
    case DurationSecRole:
        return item->durationSec();
    case DurationTextRole:
        return item->durationText();
    case ThumbRemoteRole:
        return item->thumbRemote();
    case ThumbLocalRole:
        return item->thumbLocal();
    case DataRole:
        return item->toMap();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> VideoListModel::roleNames() const
{
    return {
        { VideoIdRole, "videoId" },
        { TitleRole, "title" },
        { UploaderRole, "uploader" },
        { DurationSecRole, "durationSec" },
        { DurationTextRole, "durationText" },
        { ThumbRemoteRole, "thumbRemote" },
        { ThumbLocalRole, "thumbLocal" },
        { DataRole, "data" },
    };
}

void VideoListModel::setVideos(const QVariantList &videos)
{
    beginResetModel();
    qDeleteAll(m_items);
    m_items.clear();
    m_items.reserve(videos.size());
    for (const QVariant &v : videos) {
        if (v.canConvert<QVariantMap>())
            m_items.append(VideoItem::fromMap(v.toMap(), this));
    }
    endResetModel();
}

void VideoListModel::clear()
{
    beginResetModel();
    qDeleteAll(m_items);
    m_items.clear();
    endResetModel();
}

VideoItem *VideoListModel::videoAt(int row) const
{
    if (row < 0 || row >= m_items.size())
        return nullptr;
    return m_items.at(row);
}

void VideoListModel::setThumbLocal(int row, const QString &path)
{
    if (row < 0 || row >= m_items.size())
        return;
    m_items.at(row)->setThumbLocal(path);
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, { ThumbLocalRole });
}