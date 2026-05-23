#include "video_decoder.h"
#include "tracelog.h"
#include <QDebug>
#include <QDateTime>

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject(parent)
    , m_pWorkerThread(nullptr)
    , m_pDecCtx(nullptr)
    , m_pDecFrame(nullptr)
    , m_pSwsCtx(nullptr)
    , m_isDecoderReady(false)
    , m_lastWidth(0)
    , m_lastHeight(0)
{
    // 参考 threadworker 模式：创建线程 → moveToThread → 启动
    m_pWorkerThread = new QThread(this);
    m_pWorkerThread->setObjectName("VideoDecoderThread");
    this->moveToThread(m_pWorkerThread);
    m_pWorkerThread->start();

    qDebug() << "[VideoDecoder] Worker 线程已启动";
}

VideoDecoder::~VideoDecoder()
{
    // 先停止 Worker 线程，确保不再访问 FFmpeg 资源
    if (m_pWorkerThread) {
        m_pWorkerThread->quit();
        m_pWorkerThread->wait(3000);
    }
    // 线程停止后再释放解码器（避免 use-after-free）
    releaseH264Decoder();
    qDebug() << "[VideoDecoder] 已销毁";
}

// 主线程通过 QMetaObject::invokeMethod 跨线程调用此槽
void VideoDecoder::slot_decodePacket(QByteArray data)
{
    // --- 环形缓冲入队（背压控制）---
    m_queueMutex.lock();
    if (m_packetQueue.size() >= MAX_QUEUE_SIZE) {
        // 队列满：丢弃最旧帧
        m_packetQueue.removeFirst();
    }
    m_packetQueue.append(data);
    m_queueMutex.unlock();
    processQueue();
}

void VideoDecoder::processQueue()
{
    // 首次调用时懒初始化解码器
    if (!m_isDecoderReady) {
        if (!initH264Decoder()) {
            return;
        }
        m_isDecoderReady = true;
    }

    // 从队列取一帧
    m_queueMutex.lock();
    if (m_packetQueue.isEmpty()) {
        m_queueMutex.unlock();
        return;
    }
    QByteArray data = m_packetQueue.takeFirst();
    m_queueMutex.unlock();

    if (data.size() <= 2) return;

    // --- H.264 解码 ---
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = (uint8_t*)data.constData();
    packet.size = data.size();

    int ret = avcodec_send_packet(m_pDecCtx, &packet);
    if (ret < 0) {
        static int errCount = 0;
        if (++errCount <= 3)
            qDebug() << "[VideoDecoder] avcodec_send_packet 失败:" << ret;
        return;
    }

    // 一次 send 可能产生多个 frame
    while (true) {
        ret = avcodec_receive_frame(m_pDecCtx, m_pDecFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            static int errCount2 = 0;
            if (++errCount2 <= 3)
                qDebug() << "[VideoDecoder] avcodec_receive_frame 失败:" << ret;
            break;
        }

        // 非阻塞帧率控制：距上一帧不足 20ms 则跳过
        static qint64 lastFrameTime = 0;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (lastFrameTime != 0 && now - lastFrameTime < 20) continue;
        lastFrameTime = now;

        static int decodedCount = 0; decodedCount++;
        if (decodedCount <= 3 || decodedCount % 30 == 0)
            qDebug() << "[VideoDecoder] 解码帧 #" << decodedCount
                     << m_pDecFrame->width << "x" << m_pDecFrame->height;

        int width = m_pDecFrame->width;
        int height = m_pDecFrame->height;

        // 确保 SwsContext 与当前帧尺寸匹配
        ensureSwsContext(width, height, (AVPixelFormat)m_pDecFrame->format);

        if (!m_pSwsCtx) continue;

        // YUV → RGB32
        QImage rgbImage(width, height, QImage::Format_RGB32);
        uint8_t* dstData[1] = { rgbImage.bits() };
        int dstLinesize[1] = { (int)rgbImage.bytesPerLine() };

        sws_scale(m_pSwsCtx,
                  m_pDecFrame->data, m_pDecFrame->linesize,
                  0, height,
                  dstData, dstLinesize);

        // 发射解码图像到主线程（QImage 隐式共享，跨线程安全）
        Q_EMIT sig_decodedImage(rgbImage);
    }
}

bool VideoDecoder::initH264Decoder()
{
    avcodec_register_all();

    AVCodec* pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!pCodec) {
        qDebug() << "[VideoDecoder] H.264 解码器未找到";
        return false;
    }

    m_pDecCtx = avcodec_alloc_context3(pCodec);
    if (!m_pDecCtx) {
        qDebug() << "[VideoDecoder] avcodec_alloc_context3 失败";
        return false;
    }

    if (avcodec_open2(m_pDecCtx, pCodec, nullptr) < 0) {
        qDebug() << "[VideoDecoder] avcodec_open2 失败";
        releaseH264Decoder();
        return false;
    }

    m_pDecFrame = av_frame_alloc();
    if (!m_pDecFrame) {
        qDebug() << "[VideoDecoder] av_frame_alloc 失败";
        releaseH264Decoder();
        return false;
    }

    qDebug() << "[VideoDecoder] H.264 解码器初始化完成";
    return true;
}

void VideoDecoder::releaseH264Decoder()
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
    m_isDecoderReady = false;
}

void VideoDecoder::ensureSwsContext(int width, int height, AVPixelFormat fmt)
{
    // 尺寸未变则跳过重建
    if (m_pSwsCtx && m_lastWidth == width && m_lastHeight == height) {
        return;
    }

    if (m_pSwsCtx) {
        sws_freeContext(m_pSwsCtx);
    }
    m_pSwsCtx = sws_getContext(width, height, fmt,
                               width, height, AV_PIX_FMT_RGB32,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_pSwsCtx) {
        qDebug() << "[VideoDecoder] sws_getContext 失败";
    } else {
        m_lastWidth = width;
        m_lastHeight = height;
    }
}
