#ifndef VIDEO_WRITE_H
#define VIDEO_WRITE_H

#include <QWidget>
#include <QImage>
#include <QPainter>

#define USE_H264

#ifdef USE_H264
extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}
#endif

class Video_Write : public QWidget
{
    Q_OBJECT
public:
    explicit Video_Write(QWidget *parent = nullptr);
    ~Video_Write();

public slots:
    void slot_recvVideoFrame(const char* data, int len, int codec = -1);
    void slot_clearFrame();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;

#ifdef USE_H264
    bool initH264Decoder();
    void releaseH264Decoder();

    AVCodecContext* m_pDecCtx = nullptr;
    AVFrame* m_pDecFrame = nullptr;
    SwsContext* m_pSwsCtx = nullptr;
#endif
};

#endif // VIDEO_WRITE_H
