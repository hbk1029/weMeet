#include "ffmpeg_decoder.h"
#include <QDebug>
#include <string>

FFmpegDecoder::FFmpegDecoder(const QString& filePath, QObject *parent)
    : QThread(parent)
    , m_fileName(filePath)
    , m_stop(false)
{
}

FFmpegDecoder::~FFmpegDecoder()
{
    stop();
    wait();
}

void FFmpegDecoder::stop()
{
    m_stop = true;
}

void FFmpegDecoder::run()
{
    // 1. 初始化 FFMPEG，注册所有编解码器和封装格式
    av_register_all();

    // 2. 创建 AVFormatContext，FFMPEG 所有操作都通过它进行
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    // 3. 打开视频文件
    std::string path = m_fileName.toStdString();
    const char* file_path = path.c_str();

    if (avformat_open_input(&pFormatCtx, file_path, NULL, NULL) != 0) {
        qDebug() << "can't open file";
        return;
    }

    // 3.1 读取视频文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        qDebug() << "Could't find stream infomation.";
        avformat_close_input(&pFormatCtx);
        return;
    }

    // 4. 查找文件中的视频流
    int videoStream = -1;
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1) {
        qDebug() << "Didn't find a video stream.";
        avformat_close_input(&pFormatCtx);
        return;
    }

    // 5. 查找解码器
    AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        qDebug() << "Codec not found.";
        avformat_close_input(&pFormatCtx);
        return;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        qDebug() << "Could not open codec.";
        avformat_close_input(&pFormatCtx);
        return;
    }

    // 6. 创建需要的结构体
    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameRGB = av_frame_alloc();
    int y_size = pCodecCtx->width * pCodecCtx->height;
    AVPacket *packet = (AVPacket *)malloc(sizeof(AVPacket));
    av_new_packet(packet, y_size);

    // 7. YUV420p → RGB32 转换上下文
    struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                     pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                                     AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
    uint8_t *out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *)pFrameRGB, out_buffer, AV_PIX_FMT_RGB32,
                   pCodecCtx->width, pCodecCtx->height);

    // PTS 同步：记录开始时间和初始 PTS
    int64_t start_time = av_gettime();
    int64_t pts = 0;

    // 8. 循环读取视频帧，转换为 RGB，发射信号去显示
    int ret, got_picture;
    while (!m_stop) {
        if (av_read_frame(pFormatCtx, packet) < 0) {
            break; // 视频读取完毕
        }

        if (packet->stream_index == videoStream) {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                qDebug() << "decode error";
                av_free_packet(packet);
                break;
            }

            if (got_picture) {
                // PTS 同步：等待到正确的显示时间
                int64_t realTime = av_gettime() - start_time;
                while (pts > realTime && !m_stop) {
                    msleep(10);
                    realTime = av_gettime() - start_time;
                }
                if (m_stop) {
                    av_free_packet(packet);
                    break;
                }

                sws_scale(img_convert_ctx,
                          (uint8_t const * const *)pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height,
                          pFrameRGB->data, pFrameRGB->linesize);

                QImage tmpImg((uchar *)out_buffer, pCodecCtx->width, pCodecCtx->height,
                              QImage::Format_RGB32);

                // 10. 每获取一帧图像就发射信号（深拷贝避免 buffer 被覆盖）
                emit SIG_GetOneImage(tmpImg.copy());

                // 获取当前帧的显示时间 pts
                pFrame->pts = pFrame->best_effort_timestamp;
                pts = pFrame->pts;
                pts *= 1000000 * av_q2d(pFormatCtx->streams[videoStream]->time_base);
            }
        }

        av_free_packet(packet);
        msleep(5);
    }

    // 9. 回收资源
    av_free(out_buffer);
    av_free(pFrame);
    av_free(pFrameRGB);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}
