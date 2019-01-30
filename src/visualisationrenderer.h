#ifndef VISUALISATION_RENDERER_H
#define VISUALISATION_RENDERER_H

#include <QQuickItem>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QQuickWindow>
#include <QTime>
#include <QSettings>

#include <memory>
#include <mutex>

class VisualisationRenderer : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    VisualisationRenderer();
    ~VisualisationRenderer();

    void setViewportSize(const QSize &size) {
        m_viewportSize = size;
    }

    void setWindow(QQuickWindow *window) { m_window = window; }
    void setSpectrumData(const QVector<qreal>& data) {
        auto v = data.toStdVector();
        m_spectrumData = std::vector<GLfloat>(v.begin(), v.end());
    }
    void setWaveformData(const QVector<qreal>& data) {
        auto v = data.toStdVector();
        m_waveformData = std::vector<GLfloat>(v.begin(), v.end());
    }

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
    Q_PROPERTY(QVector<qreal> spectrum MEMBER m_spectrumData WRITE setSpectrumData)
    Q_PROPERTY(QVector<qreal> waveform MEMBER m_waveformData WRITE setWaveformData)
    Q_PROPERTY(QString currentShader MEMBER m_shader)

public:
    Visualisation();

public slots:
    void sync();
    void cleanup();
    void setSpectrumData(const QVector<qreal>& data);
    void setWaveformData(const QVector<qreal>& data);

private slots:
    void handleWindowChanged(QQuickWindow *win);

private:
    VisualisationRenderer *m_renderer;
    QVector<qreal> m_spectrumData;
    QVector<qreal> m_waveformData;

    QString m_shader;
};

#endif // VISUALISATION_RENDERER_H
