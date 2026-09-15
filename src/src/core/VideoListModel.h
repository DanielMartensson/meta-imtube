#pragma once

#include <QAbstractListModel>
#include <QVector>

class VideoItem;

class VideoListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        VideoIdRole = Qt::UserRole + 1,
        TitleRole,
        UploaderRole,
        DurationSecRole,
        DurationTextRole,
        ThumbRemoteRole,
        ThumbLocalRole,
        DataRole,
    };
    Q_ENUM(Role)

    explicit VideoListModel(QObject *parent = nullptr);
    ~VideoListModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void setVideos(const QVariantList &videos);
    Q_INVOKABLE void clear();
    Q_INVOKABLE VideoItem *videoAt(int row) const;

    void setThumbLocal(int row, const QString &path);

private:
    QVector<VideoItem *> m_items;
};