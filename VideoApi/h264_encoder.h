#ifndef H264_ENCODER_H
#define H264_ENCODER_H

#define USE_H264

#ifdef USE_H264

#include <QObject>
#include <opencv2/core.hpp>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
}

class H264Encoder : public QObject
{
    Q_OBJECT
public:
    explicit H264Encoder(int width, int height, QObject *parent = nullptr);
    ~H264Encoder();

signals:
    void sig_encodedPacket(const char* data, int len);

public slots:
    void slot_encode(const cv::Mat& bgrFrame);

private:
    AVCodecContext* m_pCodecCtx;
    AVFrame* m_pFrame;
    uint8_t* m_pFrameBuffer;
    SwsContext* m_pSwsCtx;
    int m_width;
    int m_height;
};

#endif // USE_H264

#endif // H264_ENCODER_H
