#include "applicationcontroller.h"

#include <QDebug>

PlaylistItemModel* ApplicationController::playlistModel()
{
    qDebug() << "playlistModel()" << m_playlistModel;
    return m_playlistModel;
}

PlaybackEngine* ApplicationController::soundEngine()
{
    qDebug() << "soundEngine() ->" << m_soundEngine;
    return m_soundEngine;
}

bool ApplicationController::isMuted() const
{
    qDebug() << "isMuted() ->" << m_soundEngine->isMuted();
    return m_soundEngine->isMuted();
}

bool ApplicationController::isPlaying() const
{
    qDebug() << "isPlaying() ->" << m_soundEngine->isPlaying();
    return m_soundEngine->isPlaying();
}

int ApplicationController::volume() const
{
    qDebug() << "volume() ->" << m_soundEngine->volume();
    return m_soundEngine->volume();
}

int ApplicationController::position() const
{
    qDebug() << "position() ->" << m_soundEngine->position();
    return m_soundEngine->position();
}

int ApplicationController::duration() const
{
    qDebug() << "duration() ->" << m_playlistModel->currentFileInfo().duration;// m_soundEngine->duration();
    return m_playlistModel->currentFileInfo().duration;//m_soundEngine->duration();
}

QString ApplicationController::artist() const
{
    qDebug() << "artist() ->" << m_playlistModel->currentFileInfo().artist;
    return m_playlistModel->currentFileInfo().artist;
}

QString ApplicationController::song() const
{
    qDebug() << "song() ->" << m_playlistModel->currentFileInfo().song;
    return m_playlistModel->currentFileInfo().song;
}

QString ApplicationController::album() const
{
    qDebug() << "album() ->" << m_playlistModel->currentFileInfo().album;
    return m_playlistModel->currentFileInfo().album;
}

QString ApplicationController::coverUrl() const
{
    qDebug() << "coverUrl() ->" << m_playlistModel->currentFileInfo().coverUrl;
    return m_playlistModel->currentFileInfo().coverUrl;
}

std::shared_ptr<RingBufferT<double>> ApplicationController::spectrumBuffer()
{
    return m_soundEngine->getSpectrumDataBuffer();
}

std::shared_ptr<RingBufferT<double>> ApplicationController::waveformBuffer()
{
    return m_soundEngine->getWaveformDataBuffer();
}

void ApplicationController::play(bool isSetPlay)
{
    qDebug() << "play():" << isSetPlay;
    m_soundEngine->play(isSetPlay);
}

void ApplicationController::stop()
{
    qDebug() << "stop()";
    m_soundEngine->stop();
}

void ApplicationController::next()
{
    int newIndex = m_playlistModel->currentIndex() + 1;
    bool wasPlaying = isPlaying();

    qDebug() << "next():" << newIndex;

    if(m_playlistModel->isValidIndex(newIndex)) {
        setCurrentItem(newIndex);
        play(wasPlaying);
    } else {
        stop();
    }
}

void ApplicationController::previous()
{
    int newIndex = m_playlistModel->currentIndex() - 1;
    bool wasPlaying = isPlaying();

    qDebug() << "previous():" << newIndex;

    stop();
    if(m_playlistModel->isValidIndex(newIndex)) {
        setCurrentItem(newIndex);
        play(wasPlaying);
    } else {
        stop();
    }
}

void ApplicationController::setVolume(int volume)
{
    qDebug() << "setVolume():" << volume;
    m_soundEngine->setVolume(volume);
}

void ApplicationController::setPosition(int position)
{
    qDebug() << "setPosition():" << position;
    if(position < duration()) {
        m_soundEngine->setPosition(position);
    }
}

void ApplicationController::addFiles(const QList<QUrl> &filePathList)
{
    bool wasEmpty = !m_playlistModel->size();
    int prevSize = m_playlistModel->size();
    int prevIndex = m_playlistModel->currentIndex();
    int newSize, newIndex;


    m_playlistModel->addFilenameList(filePathList);

    newSize = m_playlistModel->size();
    newIndex = m_playlistModel->currentIndex();

    // precache new files after the index
    if(wasEmpty) {
        setCurrentItem(0);
    }
}

void ApplicationController::removeAllFiles()
{
    qDebug() << "removeAllFiles()";
    m_playlistModel->reset();
    m_soundEngine->clearCache();
    emit metadataChanged();
}

void ApplicationController::setCurrentItem(int index)
{
    qDebug() << "setCurrentItem():" << index;
    int oldIndex = m_playlistModel->currentIndex();
    if(index != oldIndex) {
        m_playlistModel->setCurrentIndex(index);
        updateCacheNearIndex(oldIndex);

        emit metadataChanged();
    }
}

void ApplicationController::removeItem(int index)
{
    qDebug() << "removeItem():" << index;
    const bool wasCurrentFile = (index == m_playlistModel->currentIndex());

    if(wasCurrentFile) {
        m_soundEngine->stop();
    }

    m_playlistModel->remove(index);
}

bool ApplicationController::loadFileForPlayback(const QString &filename)
{
    qDebug() << "loadFileForPlayback():" << filename;

    emit loadingFileStarted();

    bool wasPlaying = isPlaying();

    bool status = m_soundEngine->loadFromCache(filename);
    if(!status) {
        emit error(QString("File failed to open:\n%1").arg(filename));
    }

    emit loadingFileFinished();

    if(wasPlaying) {
        play(true);
    }

    return status;
}

bool ApplicationController::loadFileInPlaylist(int index)
{
    return loadFileForPlayback(m_playlistModel->getFileInfo(index).path);
}

void ApplicationController::playNextFile()
{
    int newIndex = m_playlistModel->currentIndex() + 1;

    qDebug() << "playNextFile():" << newIndex;

    if(m_playlistModel->isValidIndex(newIndex)) {
        setCurrentItem(newIndex);
        play(true);
    } else {
        stop();
    }
}

void ApplicationController::updateCacheNearIndex(int oldIndex)
{
    int currentIndex = m_playlistModel->currentIndex();
    if(!m_playlistModel->isValidIndex(currentIndex)
            || oldIndex == currentIndex) {
        return;
    }

    int toBeCachedLeftIndex = currentIndex - precache_size;
    int toBeCachedRightIndex = currentIndex + precache_size;

    if(toBeCachedLeftIndex < 0) {
        toBeCachedLeftIndex = 0;
    }
    if(toBeCachedRightIndex >= m_playlistModel->size()) {
        toBeCachedRightIndex = m_playlistModel->size() - 1;
    }

    // precache
    for(int i = toBeCachedLeftIndex; i <= toBeCachedRightIndex; ++i) {
        m_soundEngine->preloadFile(m_playlistModel->getFileInfo(i).path);
    }

    // uncache others
    if(m_playlistModel->isValidIndex(oldIndex)) {
        int wasCachedLeft = oldIndex - precache_size;
        int wasCachedRight = oldIndex + precache_size;

        if(wasCachedLeft < 0) {
            wasCachedLeft = 0;
        }
        if(wasCachedRight >= m_playlistModel->size()) {
            wasCachedRight = m_playlistModel->size() - 1;
        }

        // do nothing if matches
        if(wasCachedLeft == toBeCachedLeftIndex && wasCachedRight == toBeCachedRightIndex){
            return;
        }

        // check if it overlaps
        if(wasCachedLeft >= toBeCachedRightIndex || wasCachedRight <= toBeCachedLeftIndex) {
            for(int i = wasCachedLeft; i <= wasCachedRight; ++i) {
                m_soundEngine->removeFileFromCache(m_playlistModel->getFileInfo(i).path);
            }
        } else {
            // left side
            for(int i = wasCachedLeft; i < toBeCachedLeftIndex; ++i) {
                m_soundEngine->removeFileFromCache(m_playlistModel->getFileInfo(i).path);
            }
            // right side
            for(int i = toBeCachedRightIndex + 1; i <= wasCachedRight; ++i) {
                m_soundEngine->removeFileFromCache(m_playlistModel->getFileInfo(i).path);
            }
        }
    }
}

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent),
      m_playlistModel(new PlaylistItemModel()),
      m_soundEngine(new PlaybackEngine())
{
    // interconnect
    QObject::connect(m_soundEngine, &PlaybackEngine::spectrumDataChanged,
                     this, &ApplicationController::spectrumChanged,
                     Qt::QueuedConnection);

    QObject::connect(m_soundEngine, &PlaybackEngine::durationChanged,
                    this, &ApplicationController::durationChanged);

    QObject::connect(m_soundEngine, &PlaybackEngine::positionChanged,
                    this, &ApplicationController::positionChanged,
                     Qt::QueuedConnection);

    QObject::connect(m_playlistModel, &PlaylistItemModel::currentIndexChanged,
                     this, &ApplicationController::loadFileInPlaylist,
                     Qt::QueuedConnection);

    QObject::connect(m_soundEngine, &PlaybackEngine::playbackStatusChanged,
                     this, &ApplicationController::playbackStatusChanged);

    QObject::connect(m_soundEngine, &PlaybackEngine::fileEnded,
                     this, &ApplicationController::playNextFile);
}

ApplicationController::~ApplicationController()
{
    delete m_soundEngine;
    delete m_playlistModel;
}
