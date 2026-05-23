#include "opengl_render.h"

#ifdef USE_OPENGL

#include "tracelog.h"
#include <QPainter>

OpenGLRender::OpenGLRender(QWidget *parent)
    : QOpenGLWidget(parent), m_hasFrame(false)
{
    setMinimumSize(160, 120);
    setStyleSheet("background: #1A1A2E;");
}

OpenGLRender::~OpenGLRender()
{
}

void OpenGLRender::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.102f, 0.102f, 0.180f, 1.0f);
}

void OpenGLRender::paintGL()
{
    static int paintCount = 0; paintCount++;
    if (paintCount <= 3 || paintCount % 30 == 0)
        TRACE("VDEC_PAINT #%d", paintCount);
    glClear(GL_COLOR_BUFFER_BIT);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    m_mutex.lock();
    bool hasFrame = m_hasFrame && !m_image.isNull();
    if (hasFrame) {
        QImage scaled = m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        m_mutex.unlock();
        painter.drawImage(x, y, scaled);
    } else {
        m_mutex.unlock();
        painter.fillRect(rect(), QColor(0x1A, 0x1A, 0x2E));
        painter.setPen(QColor(0x66, 0x66, 0x66));
        painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("等待视频..."));
    }
}

void OpenGLRender::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void OpenGLRender::slot_updateImage(QImage img)
{
    m_mutex.lock();
    m_image = img;
    m_hasFrame = true;
    m_mutex.unlock();
    update();
}

void OpenGLRender::slot_clearFrame()
{
    m_mutex.lock();
    m_image = QImage();
    m_hasFrame = false;
    m_mutex.unlock();
    update();
}

#endif // USE_OPENGL
