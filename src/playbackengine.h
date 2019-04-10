#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#include <QObject>
#include <QPixmap>

#include "audio_engine/decoder.h"
#include "audio_engine/playback.h"
//#include "audio_engine/splitter.h"
#include "audio_engine/spectrumanalyzer.h"

#include <mutex>

//! Class that handles playback, metadata gather and sound sampling
//! and is also bindable through QML.
class PlaybackEngine : public QObject
{
    Q_OBJECT

    QString m_currentFile;

    int m_volume;
    bool m_isMuted;
    bool m_isReady;

    // engine
    audioengine::Playback m_playback;
    audioengine::Decoder m_decoder;
    audioengine::SpectrumAnalyzer m_spectrum;

    std::mutex m_spectrum_mtx, m_waveform_mtx;

    void writeSettings();
    void readSettings();

    void playbackStreamRestart();

public:
    PlaybackEngine();
    ~PlaybackEngine();

// accessors
    /**
     * @brief position
     * @return Playback position.
     */
    int position() const;

    /**
     * @brief duration
     * @return Playback duration in ms.
     */
    int duration() const;

    /**
     * @brief volume
     * @return Playback volume in percentage.
     */
    int volume() const;

    /**
     * @brief isMuted
     * @return muted status.
     */
    bool isMuted() const;

    /**
     * @brief isLoad
     * @return playback readiness status.
     */
    bool isReady() const;

    /**
     * @brief isPlaying
     * @return playback status.
     */
    bool isPlaying() const;

    /**
     * @brief getSpectrumDataBuffer
     * @return ring buffer with FFT data.
     */
    std::shared_ptr<RingBufferT<double>> getSpectrumDataBuffer();

    /**
     * @brief getSpectrumDataBuffer
     * @return ring buffer with a waveform.
     */
    std::shared_ptr<RingBufferT<double>> getWaveformDataBuffer();

    /**
     * @brief currentFile
     * @return Currently playing file name.
     */
    QString currentFile() const;

public slots:
    /**
     * @brief loadFile
     * @param localFilename
     * @return status
     *
     * Load supported audio file directly and delete all cached files.
     */
    bool loadFile(const QString& localFilename);

    /**
     * @brief preloadFile
     * @param localFilename
     *
     * Preload supported audio file.
     */
    void preloadFile(const QString& localFilename);

    /**
     * @brief loadFromCache
     * @param localFilename
     * @return status
     *
     * Load file from cache for playback.
     */
    bool loadFromCache(const QString &localFilename);

    /**
     * @brief removeFileFromCache
     * @param localFilename
     *
     * Remove a file from cache by filename.
     */
    void removeFileFromCache(const QString &localFilename);

    /**
     * @brief clearCache
     *
     * Clear cache and stop playback.
     */
    void clearCache();

    /**
     * @brief play
     * @param play
     *
     * Play or pause playback.
     */
    void play(bool isSetPlay);

    /**
     * @brief stop
     *
     * Stop playback and reset position.
     */
    void stop();

    /**
     * @brief setPosition
     * @param pos
     *
     * Set position in playback. Values are (0 - duration())
     */
    void setPosition(qint64 pos);

    /**
     * @brief setVolume
     * @param volume
     *
     * Set playback volume in percentage (0 - 100).
     */
    void setVolume(int volume);

    /**
     * @brief setMuted
     * @param muted
     *
     * Mute the playback.
     */
    void setMuted(bool muted);

    /**
     * @brief restoreSettings
     *
     * Load all settings from setting storage.
     */
    void restoreSettings();

signals:
    void playbackStatusChanged(bool isPlaying);

    void positionChanged();
    void durationChanged();
    void bufferSizeChanged();
    void volumeChanged();
    void fileEnded();

    void spectrumDataChanged();

    void error(const QString& what);
};

#endif // SOUND_ENGINE_H
