#ifndef VISUALISATION_RENDERER_H
#define VISUALISATION_RENDERER_H

#include <QQuickItem>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QQuickWindow>
#include <QTime>
#include <QTimer>
#include <QSettings>

#include <memory>
#include <mutex>

#include "audio_engine/ringbuffer.h"

class VisualisationRenderer : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    VisualisationRenderer();
    ~VisualisationRenderer();

    void setViewportSize(const QSize &size);

    void setWindow(QQuickWindow *window);
    void setSpectrumData(const std::vector<GLfloat>& data);
    void setWaveformData(const std::vector<GLfloat>& data);

    void setShaderPath(const QUrl& path);

    QString shaderPath() const;

public slots:
    void paint();

private:
    void updateTexture();
    void swapShaders();
    bool m_updateShader;

    QSize m_viewportSize;
    QTime m_ticker;
    std::mutex m_mtx;

    std::vector<GLfloat> m_spectrumData;
    std::vector<GLfloat> m_waveformData;
    std::shared_ptr<QOpenGLTexture> m_texture;

    std::unique_ptr<QOpenGLShaderProgram> m_program;
    std::unique_ptr<QOpenGLShader> m_shader;
    QQuickWindow *m_window;

    QString m_shaderPath;
};

//TODO: add sound parameters
class Visualisation : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(std::shared_ptr<RingBufferT<double>> spectrumBuffer MEMBER m_spectrumBuffer WRITE setSpectrumBuffer)
    Q_PROPERTY(std::shared_ptr<RingBufferT<double>> waveformBuffer MEMBER m_waveformBuffer WRITE setWaveformBuffer)
    Q_PROPERTY(QString currentShader MEMBER m_shader)

    std::vector<GLfloat> readFromBufferToGL(std::shared_ptr<RingBufferT<double>>& buffer);

public:
    Visualisation();

public slots:
    void sync();
    void cleanup();
    void setSpectrumBuffer(const std::shared_ptr<RingBufferT<double>>& buffer);
    void setWaveformBuffer(const std::shared_ptr<RingBufferT<double>>& buffer);

    void refresh();
private slots:
    void handleWindowChanged(QQuickWindow *win);

private:
    VisualisationRenderer *m_renderer;
    std::shared_ptr<RingBufferT<double>> m_spectrumBuffer;
    std::shared_ptr<RingBufferT<double>> m_waveformBuffer;

    QString m_shader;
    QTimer m_updateTimer;
};

#endif // VISUALISATION_RENDERER_H
