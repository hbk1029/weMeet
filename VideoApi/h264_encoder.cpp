#include "h264_encoder.h"

#ifdef USE_H264

#include <QDebug>
#include <cstdio>

H264Encoder::H264Encoder(int width, int height, QObject *parent)
    : QObject(parent)
    , m_pCodecCtx(nullptr)
    , m_pFrame(nullptr)
    , m_pFrameBuffer(nullptr)
    , m_pSwsCtx(nullptr)
    , m_width(width)
    , m_height(height)
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
    m_pCodecCtx->time_base = (AVRational){1, 15};
    m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_pCodecCtx->gop_size = 12;
    m_pCodecCtx->max_b_frames = 0;

    av_opt_set(m_pCodecCtx->priv_data, "preset", "superfast", 0);
    av_opt_set(m_pCodecCtx->priv_data, "tune", "zerolatency", 0);

    if (avcodec_open2(m_pCodecCtx, codec, nullptr) < 0) {
        fprintf(stderr, "[H264] Failed to open codec\n");
        return;
    }

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
        m_width, m_height, AV_PIX_FMT_BGR24,
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

void H264Encoder::slot_encode(const cv::Mat& bgrFrame)
{
    if (!m_pCodecCtx || !m_pFrame || !m_pFrameBuffer || !m_pSwsCtx) {
        fprintf(stderr, "[H264] encoder not ready, dropping frame\n");
        return;
    }

    const uint8_t* srcData[1] = { bgrFrame.data };
    int srcStride[1] = { (int)bgrFrame.step };

    sws_scale(m_pSwsCtx, srcData, srcStride, 0, m_height,
              m_pFrame->data, m_pFrame->linesize);

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
