#include "visualisationrenderer.h"
#include <QTime>
#include <QFileInfo>
#include <QThread>

constexpr auto default_shader = "shaders/waveform.glsl";

Visualisation::Visualisation() : m_renderer(nullptr), m_spectrumData(1024), m_shader(default_shader)
{
    connect(this, &QQuickItem::windowChanged,
            this, &Visualisation::handleWindowChanged);
}

void Visualisation::sync()
{
    if (!m_renderer) {
        m_renderer = new VisualisationRenderer();
        connect(window(), &QQuickWindow::beforeRendering,
                m_renderer, &VisualisationRenderer::paint, Qt::DirectConnection);
    }
    m_renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
    m_renderer->setWindow(window());
    m_renderer->setWaveformData(m_waveformData);
    m_renderer->setSpectrumData(m_spectrumData);
    m_renderer->setShaderPath(m_shader);
}

void Visualisation::cleanup()
{
    if (m_renderer) {
        delete m_renderer;
        m_renderer = nullptr;
    }
}

void Visualisation::setSpectrumData(const QVector<qreal> &data) {
    m_spectrumData = data;
    if (window())
        window()->update();
}

void Visualisation::setWaveformData(const QVector<qreal> &data) {
    m_waveformData = data;
    //if (window())
    //   window()->update();
}

void Visualisation::handleWindowChanged(QQuickWindow *win)
{
    if (win) {
        connect(win, &QQuickWindow::beforeSynchronizing, this, &Visualisation::sync, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &Visualisation::cleanup, Qt::DirectConnection);
        win->setClearBeforeRendering(false);
    }
}


VisualisationRenderer::VisualisationRenderer()
    : m_updateShader(false), m_shaderPath(default_shader) {
    // read settings
    //QSettings settings;

    //settings.beginGroup("viz");
    //m_shaderPath = settings.value("shader", m_shaderPath).toString();
    //settings.endGroup();
}

VisualisationRenderer::~VisualisationRenderer() {
    // write settings
    //QSettings settings;

    //settings.beginGroup("viz");
    //settings.setValue("shader", m_shaderPath);
    //settings.endGroup();
}

/* NOTE: this expects OpenGL shader to have:
    uniform sampler2D fftwave; // frequency data and waveform in one texture
    uniform highp vec2 resolution; // viewport resolution
 */
void VisualisationRenderer::paint()
{
    //std::unique_lock<std::mutex> lock(m_mtx);

    if (!m_program) {
        initializeOpenGLFunctions();
        m_ticker.start();

        m_program = std::make_unique<QOpenGLShaderProgram>();

        m_updateShader = true;
    }

    if(m_updateShader) {
        m_program->removeAllShaders();
        swapShaders();

        m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,
                                                    "attribute highp vec4 vertices;"
                                                    "void main() {"
                                                    "    gl_Position = vertices;"
                                                    "}");
        m_program->addShader(m_shader.get());
        m_program->bindAttributeLocation("vertices", 0);
        m_program->link();

        m_updateShader = false;
    }

    m_program->bind();
    m_program->enableAttributeArray(0);

    if(!m_texture) {
        m_texture = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        m_texture->create();
        m_texture->setFormat(QOpenGLTexture::R32F);
        m_texture->setSize(m_spectrumData.size(), 2);
        m_texture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Float32);
    }
    updateTexture();

    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());

    GLfloat values[] = {
        -1, -1,
        1, -1,
        -1, 1,
        1, 1
    };

    m_program->setAttributeArray(0, GL_FLOAT, values, 2);

    m_program->setUniformValue("fftwave", 0);
    m_program->setUniformValue("resolution", m_viewportSize);
    m_program->setUniformValue("sample_size", (GLint) m_waveformData.size());
    m_program->setUniformValue("time", (GLfloat) m_ticker.elapsed() / 1000.f);

    glDisable(GL_DEPTH_TEST);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    m_texture->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program->disableAttributeArray(0);

    m_texture->release();
    m_program->release();
    //m_window->resetOpenGLState();

    QThread::usleep(100);
}

void VisualisationRenderer::updateTexture() {
    std::vector<GLfloat> texture_data;
    texture_data.reserve(m_spectrumData.size() * 2);

    texture_data.insert(texture_data.begin(), m_spectrumData.begin(), m_spectrumData.end());
    texture_data.insert(texture_data.end(), m_waveformData.begin(), m_waveformData.end());

    m_texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, texture_data.data());
}

void VisualisationRenderer::swapShaders()
{
    //std::unique_lock<std::mutex> lock(m_mtx);
    std::unique_ptr<QOpenGLShader> shader;
    shader = std::make_unique<QOpenGLShader>(QOpenGLShader::Fragment);

    if(shader->compileSourceFile(m_shaderPath)) {
        if(!m_shader) {
            m_shader = std::make_unique<QOpenGLShader>(QOpenGLShader::Fragment);
        }

        m_shader.swap(shader);
    }
}


void VisualisationRenderer::setShaderPath(const QUrl &path) {
    if(path == m_shaderPath) return;

    // cut file://
    m_shaderPath = QFileInfo(path.toLocalFile()).absoluteFilePath();

    m_updateShader = true;
}


QString VisualisationRenderer::shaderPath() const
{
    return m_shaderPath;
}
