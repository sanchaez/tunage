#include "playlistitemmodel.h"

#include <QFileInfo>

int PlaylistItemModel::currentIndex() const
{
    return m_currentIndex;
}

void PlaylistItemModel::setCurrentIndex(int currentIndex)
{
    Q_ASSERT(isValidIndex(currentIndex));

    m_currentIndex = currentIndex;
    emit currentIndexChanged(m_currentIndex);
}

PlaylistItemModel::PlaylistItemModel(QObject *parent)
    : QAbstractListModel(parent), m_currentIndex(-1)
{
}

void PlaylistItemModel::addPlaylistItem(const AudioTagInfo& item)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_playlist.push_back(item);
    endInsertRows();
}

void PlaylistItemModel::addFilename(const QString& filename)
{
    auto path = QFileInfo(filename).absoluteFilePath();
    AudioTagInfo item(path);

    addPlaylistItem(std::move(item));
}

void PlaylistItemModel::addFilenameList(const QList<QUrl>& filenameList)
{
    for(auto &&filename : filenameList) {
        addFilename(filename.toLocalFile());
    }
}

void PlaylistItemModel::remove(int index)
{
    Q_ASSERT(isValidIndex(index));

    removeRows(index, 1, QModelIndex());
}

AudioTagInfo PlaylistItemModel::getFileInfo(int index) const
{
    Q_ASSERT(isValidIndex(index));

    return m_playlist.at(index);
}

AudioTagInfo PlaylistItemModel::currentFileInfo() const
{
    if(m_currentIndex >= 0) {
       return getFileInfo(m_currentIndex);
    } else {
       return AudioTagInfo();
    }
}

bool PlaylistItemModel::isValidIndex(int index) const
{
    return (index >= 0) && (index < m_playlist.size());
}

int PlaylistItemModel::size() const
{
    return m_playlist.count();
}

void PlaylistItemModel::reset()
{
    m_currentIndex = -1;

    beginResetModel();
    m_playlist.clear();
    endResetModel();
}

int PlaylistItemModel::rowCount(const QModelIndex& /*parent*/) const
{
    return size();
}

// retreive data by keys
QVariant PlaylistItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() < 0 || index.row() >= m_playlist.count())
        return QVariant();

    const auto& item = m_playlist[index.row()];
    switch(role)
    {
    case NameRole:
        return item.song;
    case ArtistRole:
        return item.artist;
    case FileNameRole:
        return item.fileName;
    case DurationRole:
        return item.duration;
    default:
        return QVariant();
    }
}

// search keys for QML
const QHash<int, QByteArray> PlaylistItemModel::m_roles {{PlaylistItemModel::NameRole, "name"},
                                                         {PlaylistItemModel::ArtistRole, "artist"},
                                                         {PlaylistItemModel::FileNameRole, "fileName"},
                                                         {PlaylistItemModel::DurationRole, "duration"}};

QHash<int, QByteArray> PlaylistItemModel::roleNames() const
{
    return m_roles;
}

bool PlaylistItemModel::removeRows(int row, int count, const QModelIndex& /* parent */)
{
    if(row < 0 || row + count >= m_playlist.size())
        return false;

    int lastRow = row + count - 1;

    beginRemoveRows(QModelIndex(), row, lastRow);
    m_playlist.erase(m_playlist.begin() + row, m_playlist.begin() + lastRow + 1);
    endRemoveRows();

    if(m_playlist.empty()) {
        // no items left
        setCurrentIndex(-1);
    } else if(row >= m_currentIndex
              && lastRow <= m_currentIndex) {
        // the first index after removal would be same as 'row'
        // since 'count' items will be gone
        setCurrentIndex(row);
    }

    return true;
}
