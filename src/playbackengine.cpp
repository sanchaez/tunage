#include "playbackengine.h"

#include <QtDebug>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

#include "audiotaginfo.h"

// some defaults
constexpr auto max_volume = 100;
constexpr auto default_volume = max_volume;

PlaybackEngine::PlaybackEngine()
    : m_currentFile(""),
      m_isMuted(false),
      m_isReady(false),
      m_playback(),
      m_decoder(),
      m_spectrum(m_decoder.visualizer_buffer())
{
    m_spectrum.set_update_callback([this]() {
        emit spectrumDataChanged();
    });

    m_playback.set_playback_buffer(m_decoder.sample_buffer());

    m_decoder.set_position_callback([this]() {
        emit positionChanged();
    });

    m_decoder.set_file_end_callback([this]() {
        //emit playbackStatusChanged(false);
        emit fileEnded();
    });

    m_decoder.start_thread();
    m_spectrum.start_thread();

    readSettings();
}

PlaybackEngine::~PlaybackEngine()
{
    // save settings on exit
    writeSettings();
}

QString PlaybackEngine::currentFile() const
{
    return m_currentFile;
}

void PlaybackEngine::writeSettings()
{
    QSettings settings;

    settings.beginGroup("sound");
    settings.setValue("volume", m_volume);
    settings.endGroup();
}

void PlaybackEngine::readSettings()
{
    QSettings settings;

    settings.beginGroup("sound");
    setVolume(settings.value("volume", default_volume).toInt());
    settings.endGroup();
}

void PlaybackEngine::playbackStreamRestart()
{
    if(m_isReady &&
            (!m_playback.running()
            || m_playback.sample_rate() != m_decoder.sample_rate()
            || m_playback.buffer_size() != m_decoder.buffer_size()))
    {
        m_playback.stream_create(m_decoder.sample_rate(),
                                 m_decoder.buffer_size());
    }
}

void PlaybackEngine::play(bool isSetPlay)
{
    if(!m_isReady) {
        return;
    }

    if(isSetPlay) {
        playbackStreamRestart();
        m_decoder.start();
    } else {
        m_decoder.pause();
    }

    emit playbackStatusChanged(isSetPlay);
}

void PlaybackEngine::stop()
{
    m_decoder.stop();
    //m_playback.stream_stop();

    emit playbackStatusChanged(false);
}

bool PlaybackEngine::loadFile(const QString& localFilename)
{
    Q_ASSERT(!localFilename.isEmpty());

    m_currentFile = localFilename;

    m_isReady = m_decoder.decode_load_single(m_currentFile.toStdString());

    playbackStreamRestart();

    return m_isReady;
}

void PlaybackEngine::preloadFile(const QString &localFilename)
{
    Q_ASSERT(!localFilename.isEmpty());

    m_decoder.decode_to_cache(localFilename.toStdString());
}

bool PlaybackEngine::loadFromCache(const QString &localFilename)
{
    Q_ASSERT(!localFilename.isEmpty());

    m_currentFile = localFilename;

    if(!m_decoder.is_cached(m_currentFile.toStdString())) {
        // file not in cache, add it
        m_decoder.decode_to_cache(m_currentFile.toStdString());
    }

    m_isReady = m_decoder.load_from_cache(m_currentFile.toStdString());

    playbackStreamRestart();

    return m_isReady;
}

void PlaybackEngine::removeFileFromCache(const QString &localFilename)
{
    Q_ASSERT(!localFilename.isEmpty());

    if(localFilename == m_currentFile) {
        stop();
        m_currentFile.clear();
    }

    m_decoder.remove_from_cache(localFilename.toStdString());
}

void PlaybackEngine::clearCache()
{
    stop();
    m_decoder.clear_cache();
    m_isReady = false;
}

void PlaybackEngine::setVolume(int new_volume)
{
    Q_ASSERT(new_volume >= 0);
    Q_ASSERT(new_volume <= max_volume);

    m_volume = new_volume;

    float normalized_volume = new_volume / (float) max_volume;
    m_decoder.set_volume_from_linear(normalized_volume);

    emit volumeChanged();
}

void PlaybackEngine::setPosition(qint64 pos)
{
    Q_ASSERT(pos < duration());
    Q_ASSERT(pos >= 0);

    m_decoder.set_position_miliseconds(pos);
    emit positionChanged();
}

void PlaybackEngine::setMuted(bool muted)
{
    static int lastVolume = m_volume;

    if(muted) {
        lastVolume = m_volume;
    }

    setVolume(muted ? 0 : lastVolume);

    m_isMuted = muted;
}

void PlaybackEngine::restoreSettings()
{
    readSettings();
}

int PlaybackEngine::position() const
{
    return m_decoder.position_miliseconds();
}

int PlaybackEngine::duration() const
{
    return m_decoder.duration_miliseconds();
}

int PlaybackEngine::volume() const
{
    return m_volume;
}

bool PlaybackEngine::isPlaying() const
{
    return m_decoder.playing();
}

std::shared_ptr<RingBufferT<double>> PlaybackEngine::getSpectrumDataBuffer()
{
    return m_spectrum.fft_avg_out;
}
std::shared_ptr<RingBufferT<double>> PlaybackEngine::getWaveformDataBuffer()
{
    return m_spectrum.waveform_avg_out;
}

bool PlaybackEngine::isMuted() const
{
    return m_isMuted;
}

bool PlaybackEngine::isReady() const
{
    return m_isReady;
}

