#include "h264_encoder.h"
#include "common.h"

#ifdef USE_H264

#include <QDebug>
#include <QFile>
#include <cstdio>

extern "C" {
#include "libavutil/dict.h"
}

H264Encoder::H264Encoder(int width, int height, QObject *parent)
    : QObject(parent)
    , m_pCodecCtx(nullptr)
    , m_pFrame(nullptr)
    , m_pFrameBuffer(nullptr)
    , m_pSwsCtx(nullptr)
    , m_width(width)
    , m_height(height)
    , m_pts(0)
{
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "[H264] encoder not found, trying MPEG4 fallback\n");
        codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    }
    if (!codec) {
        fprintf(stderr, "[H264] no encoder available\n");
        return;
    }
    fprintf(stderr, "[H264] using encoder: %s\n", codec->name);

    m_pCodecCtx = avcodec_alloc_context3(codec);
    if (!m_pCodecCtx) {
        fprintf(stderr, "[H264] Failed to alloc codec context\n");
        return;
    }

    m_pCodecCtx->bit_rate = 400000;
    m_pCodecCtx->width = m_width;
    m_pCodecCtx->height = m_height;
    m_pCodecCtx->time_base = (AVRational){1, FRAME_RATE};  // 与摄像头采集帧率对齐
    m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_pCodecCtx->gop_size = 12;
    m_pCodecCtx->max_b_frames = 0;
    m_pCodecCtx->thread_count = 0;  // 自动选择最优线程数
    m_pCodecCtx->flags2 |= AV_CODEC_FLAG2_LOCAL_HEADER;

    // 按参考项目方式用 av_dict_set, 通过 avcodec_open2 传入参数
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "preset", "superfast", 0);
    av_dict_set(&opts, "tune", "zerolatency", 0);

    if (avcodec_open2(m_pCodecCtx, codec, &opts) < 0) {
        fprintf(stderr, "[H264] Failed to open codec\n");
        av_dict_free(&opts);
        return;
    }
    av_dict_free(&opts);

    m_pFrame = av_frame_alloc();
    if (!m_pFrame) {
        fprintf(stderr, "[H264] Failed to alloc frame\n");
        return;
    }

    m_pFrame->format = m_pCodecCtx->pix_fmt;
    m_pFrame->width = m_pCodecCtx->width;
    m_pFrame->height = m_pCodecCtx->height;

    int frameSize = avpicture_get_size(AV_PIX_FMT_YUV420P, m_width, m_height);
    m_pFrameBuffer = (uint8_t*)av_malloc(frameSize);
    if (!m_pFrameBuffer) {
        fprintf(stderr, "[H264] Failed to alloc frame buffer\n");
        return;
    }

    avpicture_fill((AVPicture*)m_pFrame, m_pFrameBuffer,
                   AV_PIX_FMT_YUV420P, m_width, m_height);

    m_pSwsCtx = sws_getContext(
        m_width, m_height, AV_PIX_FMT_RGB24,
        m_width, m_height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_pSwsCtx) {
        fprintf(stderr, "[H264] Failed to create sws context\n");
    }
}

H264Encoder::~H264Encoder()
{
    if (m_pSwsCtx) {
        sws_freeContext(m_pSwsCtx);
        m_pSwsCtx = nullptr;
    }
    if (m_pFrameBuffer) {
        av_free(m_pFrameBuffer);
        m_pFrameBuffer = nullptr;
    }
    if (m_pFrame) {
        av_frame_free(&m_pFrame);
    }
    if (m_pCodecCtx) {
        avcodec_free_context(&m_pCodecCtx);
    }
}

void H264Encoder::setPts(int64_t pts) { m_pts = pts; }

void H264Encoder::slot_encode(const cv::Mat& bgrFrame)
{
    if (!m_pCodecCtx || !m_pFrame || !m_pFrameBuffer || !m_pSwsCtx) {
        fprintf(stderr, "[H264] encoder not ready, dropping frame\n");
        return;
    }

    // OpenCV 默认 BGR → 转换为 RGB24
    cv::Mat rgbFrame;
    cv::cvtColor(bgrFrame, rgbFrame, cv::COLOR_BGR2RGB);

    // 按参考项目方式: sws_scale 写入临时帧，memcpy 到编码器帧
    AVFrame* pTmpYUV = av_frame_alloc();
    int tmpSize = avpicture_get_size(AV_PIX_FMT_YUV420P, m_width, m_height);
    uint8_t* tmpBuf = (uint8_t*)av_malloc(tmpSize);
    avpicture_fill((AVPicture*)pTmpYUV, tmpBuf, AV_PIX_FMT_YUV420P, m_width, m_height);

    const uint8_t* srcData[1] = { rgbFrame.data };
    int srcStride[1] = { (int)rgbFrame.step };

    sws_scale(m_pSwsCtx, srcData, srcStride, 0, m_height,
              pTmpYUV->data, pTmpYUV->linesize);

    // memcpy 到编码器帧缓冲区 (参考项目核心步骤)
    int y_size = m_width * m_height;
    memcpy(m_pFrameBuffer,                            pTmpYUV->data[0], y_size);
    memcpy(m_pFrameBuffer + y_size,                   pTmpYUV->data[1], y_size/4);
    memcpy(m_pFrameBuffer + y_size + y_size/4,        pTmpYUV->data[2], y_size/4);

    av_free(tmpBuf);
    av_frame_free(&pTmpYUV);

    m_pFrame->pts = m_pts;

    int ret = avcodec_send_frame(m_pCodecCtx, m_pFrame);
    if (ret < 0) {
        fprintf(stderr, "[H264] send_frame error: %d\n", ret);
        return;
    }

    AVPacket* pkt = av_packet_alloc();
    ret = avcodec_receive_packet(m_pCodecCtx, pkt);
    if (ret == 0) {
        static int frameCount = 0;
        if (++frameCount <= 3 || frameCount % 30 == 0)
            fprintf(stderr, "[H264] encoded frame #%d size=%d\n", frameCount, pkt->size);
        Q_EMIT sig_encodedPacket((const char*)pkt->data, pkt->size);
    } else if (ret == AVERROR(EAGAIN)) {
        static int eagainCount = 0;
        if (++eagainCount <= 5)
            fprintf(stderr, "[H264] receive_packet EAGAIN #%d\n", eagainCount);
    } else {
        fprintf(stderr, "[H264] receive_packet error: %d\n", ret);
    }
    av_packet_free(&pkt);
}

#endif // USE_H264
