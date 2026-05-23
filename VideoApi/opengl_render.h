#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#define USE_OPENGL

#ifdef USE_OPENGL

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QImage>
#include <QMutex>

#define USE_H264

#ifdef USE_H264
extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}
#endif

class OpenGLRender : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLRender(QWidget *parent = nullptr);
    ~OpenGLRender();

public slots:
    void slot_updateImage(QImage img);
    void slot_clearFrame();
    void slot_recvVideoFrame(const char* data, int len, int codec = -1);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    QImage m_image;
    QMutex m_mutex;
    bool m_hasFrame;

#ifdef USE_H264
    bool initH264Decoder();
    void releaseH264Decoder();

    AVCodecContext* m_pDecCtx = nullptr;
    AVFrame* m_pDecFrame = nullptr;
    SwsContext* m_pSwsCtx = nullptr;
#endif
};

#endif // USE_OPENGL
#endif // OPENGL_RENDER_H
