#include "video_write.h"
#include <QDebug>
#include <QDateTime>
#include <cstdio>

Video_Write::Video_Write(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(160, 120);
    setStyleSheet("background: #1A1A2E;");
#ifdef USE_H264
    initH264Decoder();
#endif
}

Video_Write::~Video_Write()
{
#ifdef USE_H264
    releaseH264Decoder();
#endif
}

#ifdef USE_H264
bool Video_Write::initH264Decoder()
{
    avcodec_register_all();

    AVCodec* pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!pCodec) {
        qDebug() << "[Video_Write] H.264 decoder not found";
        return false;
    }

    m_pDecCtx = avcodec_alloc_context3(pCodec);
    if (!m_pDecCtx) {
        qDebug() << "[Video_Write] avcodec_alloc_context3 failed";
        return false;
    }

    if (avcodec_open2(m_pDecCtx, pCodec, nullptr) < 0) {
        qDebug() << "[Video_Write] avcodec_open2 failed";
        releaseH264Decoder();
        return false;
    }

    m_pDecFrame = av_frame_alloc();
    if (!m_pDecFrame) {
        qDebug() << "[Video_Write] av_frame_alloc failed";
        releaseH264Decoder();
        return false;
    }

    qDebug() << "[Video_Write] H.264 decoder initialized";
    return true;
}

void Video_Write::releaseH264Decoder()
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

void Video_Write::slot_recvVideoFrame(const char* data, int len, int codec)
{
    if (!data || len <= 2) return;

    // codec < 0 表示从结构体偏移读取（网络路径），>=0 直接使用（本地预览）
    if (codec < 0) {
        int* pCodec = (int*)(data - sizeof(int));
        codec = *pCodec;
    }
    static int vRecvCount = 0;
    static int vDecodeCount = 0;

#ifdef USE_H264
    if (codec == 1) {
        if (!m_pDecCtx || !m_pDecFrame) {
            fprintf(stderr, "[VDEC] decoder not ready\n");
            return;
        }

        if (++vRecvCount <= 3 || vRecvCount % 30 == 0)
            fprintf(stderr, "[VDEC] recv packet #%d size=%d\n", vRecvCount, len);

        AVPacket packet;
        av_init_packet(&packet);
        packet.data = (uint8_t*)data;
        packet.size = len;

        int ret = avcodec_send_packet(m_pDecCtx, &packet);
        if (ret < 0) {
            fprintf(stderr, "[VDEC] send_packet error: %d\n", ret);
            return;
        }

        ret = avcodec_receive_frame(m_pDecCtx, m_pDecFrame);
        if (ret < 0) {
            return;
        }
        // 非阻塞帧率控制：距上一帧不足 20ms 则跳过本条
        static qint64 lastFrameTime = 0;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (lastFrameTime != 0 && now - lastFrameTime < 20) return;
        lastFrameTime = now;
        fprintf(stderr, "[VDEC] decoded frame #%d %dx%d\n", ++vDecodeCount,
                m_pDecFrame->width, m_pDecFrame->height);

        int width = m_pDecFrame->width;
        int height = m_pDecFrame->height;

        // 首次或尺寸变化时重建 SwsContext
        if (!m_pSwsCtx
            || m_image.width() != width
            || m_image.height() != height) {
            if (m_pSwsCtx) {
                sws_freeContext(m_pSwsCtx);
            }
            m_pSwsCtx = sws_getContext(width, height,
                                       (AVPixelFormat)m_pDecFrame->format,
                                       width, height, AV_PIX_FMT_RGB32,
                                       SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!m_pSwsCtx) {
                qDebug() << "[Video_Write] sws_getContext failed";
                return;
            }
            m_image = QImage(width, height, QImage::Format_RGB32);
        }

        uint8_t* dstData[1] = { m_image.bits() };
        int dstLinesize[1] = { (int)m_image.bytesPerLine() };

        sws_scale(m_pSwsCtx,
                  m_pDecFrame->data, m_pDecFrame->linesize,
                  0, height,
                  dstData, dstLinesize);

        update();
        return;
    }
#endif

    // JPEG 路径 (codec == 0 或 USE_H264 未启用)
    if ((unsigned char)data[0] != 0xFF || (unsigned char)data[1] != 0xD8) return;
    if (m_image.loadFromData((const uchar*)data, len, "JPG")) {
        update();
    }
}

void Video_Write::slot_clearFrame()
{
    m_image = QImage();
    update();
}

void Video_Write::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_image.isNull()) {
        painter.fillRect(rect(), QColor(0x1A, 0x1A, 0x2E));
        painter.setPen(QColor(0x66, 0x66, 0x66));
        painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("等待视频..."));
        return;
    }

    QImage scaled = m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = (width() - scaled.width()) / 2;
    int y = (height() - scaled.height()) / 2;
    painter.drawImage(x, y, scaled);
}
