#ifndef PLAYLISTITEMMODEL_H
#define PLAYLISTITEMMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QHash>
#include <QByteArray>
#include <QUrl>

#include "audiotaginfo.h"

class PlaylistItemModel : public QAbstractListModel
{
    Q_OBJECT

    QList<AudioTagInfo> m_playlist;
    int m_currentIndex;

// QAbstractListModel interface
public:
    bool removeRows(int row, int count, const QModelIndex &parent) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    static const QHash<int, QByteArray> m_roles;
    QHash<int, QByteArray> roleNames() const override;

// PlaylistItemModel inteface
public:
    explicit PlaylistItemModel(QObject *parent = nullptr);

    enum PlaylistRoles {
        NameRole = Qt::UserRole + 1,
        ArtistRole,
        FileNameRole,
        DurationRole
    };

    void addPlaylistItem(const AudioTagInfo& item);
    void addFilename(const QString& filename);
    void addFilenameList(const QList<QUrl>& filenameList);

    AudioTagInfo getFileInfo(int index) const;
    AudioTagInfo currentFileInfo() const;

    bool isValidIndex(int index) const;

    int size() const;
    int currentIndex() const;
    void setCurrentIndex(int currentIndex);
    void remove(int index);
    void reset();

signals:
    void fileRemoved(QString filename);
    void fileAdded(QString filename);
    void currentIndexChanged(int index);
};

#endif // PLAYLISTITEMMODEL_H
