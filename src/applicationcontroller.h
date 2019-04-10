#ifndef APPLICATIONCONTROLLER_H
#define APPLICATIONCONTROLLER_H

#include "audiotaginfo.h"
#include "playlistitemmodel.h"
#include "playbackengine.h"

#include <QObject>

class ApplicationController : public QObject
{
    Q_OBJECT

    // QML interface
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackStatusChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)

    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)

    Q_PROPERTY(int duration READ duration NOTIFY metadataChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY metadataChanged)
    Q_PROPERTY(QString song READ song NOTIFY metadataChanged)
    Q_PROPERTY(QString album READ album NOTIFY metadataChanged)
    Q_PROPERTY(QString coverUrl READ coverUrl NOTIFY metadataChanged)

    Q_PROPERTY(std::shared_ptr<RingBufferT<double>> spectrumBuffer READ spectrumBuffer NOTIFY spectrumChanged)
    Q_PROPERTY(std::shared_ptr<RingBufferT<double>> waveformBuffer READ waveformBuffer NOTIFY spectrumChanged)

    Q_PROPERTY(PlaylistItemModel* playlist MEMBER m_playlistModel NOTIFY modelChanged)

    PlaylistItemModel* m_playlistModel;
    PlaybackEngine* m_soundEngine;

    AudioTagInfo m_currentFileInfo;

    //void writeSettings();
    //void readSettings();

    static constexpr int precache_size = 2;

public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController();

    Q_INVOKABLE PlaylistItemModel* playlistModel();
    Q_INVOKABLE PlaybackEngine* soundEngine();

    bool isMuted() const;
    bool isPlaying() const;

    int volume() const;
    int position() const;
    int duration() const;

    QString artist() const;
    QString song() const;
    QString album() const;
    QString coverUrl() const;

    std::shared_ptr<RingBufferT<double>> spectrumBuffer();
    std::shared_ptr<RingBufferT<double>> waveformBuffer();

signals:
    void playbackStatusChanged(bool isSetPlay);
    void volumeChanged();
    void positionChanged();
    void durationChanged();

    void metadataChanged();

    void spectrumChanged();

    void loadingFileStarted();
    void loadingFileFinished();

    void error(QString what);
    void modelChanged();

public slots:
    // playback controls
    void play(bool isSetPlay);
    void stop();
    void next();
    void previous();

    void setVolume(int volume);
    void setPosition(int position);

    // playlist controls
    void addFiles(const QList<QUrl> &filePathList);
    void removeAllFiles();

    void setCurrentItem(int index);
    void removeItem(int index);

private slots:
    bool loadFileForPlayback(const QString& filename);
    bool loadFileInPlaylist(int index);

    void playNextFile();

    void updateCacheNearIndex(int oldIndex);
};

#endif // APPLICATIONCONTROLLER_H
