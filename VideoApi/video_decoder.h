#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QList>
#include <QMutex>
#include <QByteArray>

#define USE_H264
#ifdef USE_H264
// 抑制 GCC 8.x + FFmpeg + OpenCV 的 SSE 内联函数 constexpr 声明冲突
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#endif

// H.264 解码器 — 独立 Worker 线程
// 主线程投递编码数据 → 环形缓冲队列 → Worker 线程解码 → sig_decodedImage 传回主线程
class VideoDecoder : public QObject
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

public slots:
    // 主线程投递编码数据（通过 QMetaObject::invokeMethod 跨线程调用）
    void slot_decodePacket(QByteArray data);

signals:
    // Worker 线程解码完成后发射，QImage 跨线程安全深拷贝
    void sig_decodedImage(QImage img);

private:
    void processQueue();        // Worker 线程：从队列取包解码
    bool initH264Decoder();     // 初始化 H.264 解码器（首次调用时懒初始化）
    void releaseH264Decoder();  // 释放解码器资源
    void ensureSwsContext(int width, int height, AVPixelFormat fmt); // 确保 SwsContext 匹配尺寸

    // Worker 线程
    QThread* m_pWorkerThread;

    // H.264 解码
    AVCodecContext* m_pDecCtx;
    AVFrame* m_pDecFrame;
    SwsContext* m_pSwsCtx;
    bool m_isDecoderReady;
    int m_lastWidth;   // 上一次 SwsContext 对应的宽度
    int m_lastHeight;  // 上一次 SwsContext 对应的高度

    // 环形缓冲队列（背压控制）
    static const int MAX_QUEUE_SIZE = 4;
    QList<QByteArray> m_packetQueue;
    QMutex m_queueMutex;
};

#endif // VIDEO_DECODER_H
