#include "opengl_render.h"

#ifdef USE_OPENGL

#include "tracelog.h"
#include <QPainter>
#include <QDebug>
#include <QFile>
#include <cstdio>

OpenGLRender::OpenGLRender(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(160, 120);
    setStyleSheet("background: #1A1A2E;");
    m_hasFrame = false;
#ifdef USE_H264
    initH264Decoder();
#endif
}

OpenGLRender::~OpenGLRender()
{
#ifdef USE_H264
    releaseH264Decoder();
#endif
}

#ifdef USE_H264
bool OpenGLRender::initH264Decoder()
{
    avcodec_register_all();

    AVCodec* pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!pCodec) {
        qDebug() << "[OpenGLRender] H.264 decoder not found";
        return false;
    }

    m_pDecCtx = avcodec_alloc_context3(pCodec);
    if (!m_pDecCtx) {
        qDebug() << "[OpenGLRender] avcodec_alloc_context3 failed";
        return false;
    }

    if (avcodec_open2(m_pDecCtx, pCodec, nullptr) < 0) {
        qDebug() << "[OpenGLRender] avcodec_open2 failed";
        releaseH264Decoder();
        return false;
    }

    m_pDecFrame = av_frame_alloc();
    if (!m_pDecFrame) {
        qDebug() << "[OpenGLRender] av_frame_alloc failed";
        releaseH264Decoder();
        return false;
    }

    qDebug() << "[OpenGLRender] H.264 decoder initialized";
    return true;
}

void OpenGLRender::releaseH264Decoder()
{
    if (m_pSwsCtx) {
        sws_freeContext(m_pSwsCtx);
        m_pSwsCtx = nullptr;
    }
    if (m_pDecFrame) {
        av_frame_free(&m_pDecFrame);
        m_pDecFrame = nullptr;
    }
    if (m_pDecCtx) {
        avcodec_close(m_pDecCtx);
        avcodec_free_context(&m_pDecCtx);
        m_pDecCtx = nullptr;
    }
}
#endif

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

void OpenGLRender::slot_recvVideoFrame(const char* data, int len, int codec)
{
    if (!data || len <= 2) return;

    if (codec < 0) {
        int* pCodec = (int*)(data - sizeof(int));
        codec = *pCodec;
    }
    static int vRecvCount = 0;
    static int vDecodeCount = 0;

#ifdef USE_H264
    if (codec == 1) {
        if (!m_pDecCtx || !m_pDecFrame) {
            TRACE("VDEC decoder not ready");
            return;
        }

        if (++vRecvCount <= 3 || vRecvCount % 30 == 0)
            TRACE("VDEC recv #%d size=%d thread=%lu", vRecvCount, len, GetCurrentThreadId());

        AVPacket packet;
        av_init_packet(&packet);
        packet.data = (uint8_t*)data;
        packet.size = len;

        int ret = avcodec_send_packet(m_pDecCtx, &packet);
        if (ret < 0) {
            TRACE("VDEC send_packet err #%d ret=%d", vRecvCount, ret);
            return;
        }

        ret = avcodec_receive_frame(m_pDecCtx, m_pDecFrame);
        if (ret < 0) {
            return;
        }
        if (++vDecodeCount <= 3 || vDecodeCount % 30 == 0)
            TRACE("VDEC decoded #%d %dx%d", vDecodeCount, m_pDecFrame->width, m_pDecFrame->height);

        int width = m_pDecFrame->width;
        int height = m_pDecFrame->height;

        if (!m_pSwsCtx || m_image.width() != width || m_image.height() != height) {
            if (m_pSwsCtx) {
                sws_freeContext(m_pSwsCtx);
            }
            m_pSwsCtx = sws_getContext(width, height,
                                       (AVPixelFormat)m_pDecFrame->format,
                                       width, height, AV_PIX_FMT_RGB32,
                                       SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!m_pSwsCtx) {
                qDebug() << "[OpenGLRender] sws_getContext failed";
                return;
            }
        }

        QImage localImage(width, height, QImage::Format_RGB32);
        uint8_t* dstData[1] = { localImage.bits() };
        int dstLinesize[1] = { (int)localImage.bytesPerLine() };

        sws_scale(m_pSwsCtx,
                  m_pDecFrame->data, m_pDecFrame->linesize,
                  0, height,
                  dstData, dstLinesize);

        m_mutex.lock();
        m_image = localImage;
        m_hasFrame = true;
        m_mutex.unlock();
        update();
        return;
    }
#endif

    if ((unsigned char)data[0] != 0xFF || (unsigned char)data[1] != 0xD8) return;
    QImage img;
    if (img.loadFromData((const uchar*)data, len, "JPG")) {
        m_mutex.lock();
        m_image = img;
        m_hasFrame = true;
        m_mutex.unlock();
        update();
    }
}

#endif // USE_OPENGL
