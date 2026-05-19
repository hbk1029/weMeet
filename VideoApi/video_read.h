#ifndef VIDEO_READ_H
#define VIDEO_READ_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <vector>
#include "common.h"
#include "h264_encoder.h"

//图片的宽高
#define IMAGE_WIDTH (320)
#define IMAGE_HEIGHT (240)

//萌拍类型
enum funnyPic_type { fp_none = 0, fp_tuer = 1, fp_hat = 2 };

class Video_Read : public QObject
{
    Q_OBJECT
public:
    explicit Video_Read(QObject *parent = nullptr);
    ~Video_Read();

signals:
    void sig_videoFrame(const char* data, int len);

public slots:
    void slot_getVideoFrame();  // 定时采集 → 人脸检测 → 萌拍 → 编码 → 发射信号
    void slot_openVideo();       // 打开摄像头
    void slot_closeVideo();      // 关闭摄像头

    //人脸检测开关
    void slot_setFaceDetect(bool on);
    //萌拍类型 0=无 1=兔耳朵 2=帽子
    void slot_setFunnyPic(int type);

private:
    QTimer *m_pTimer;
    cv::VideoCapture m_cap;

    //人脸检测 & 萌拍
    bool m_isFaceDetect;
    int m_funnyPic;
    int m_frameSkip; // 跳帧计数，每5帧检测一次
    QImage m_tuer;
    QImage m_hat;
    std::vector<cv::Rect> m_vecLastFace;
#ifdef USE_H264
    H264Encoder* m_pH264Encoder;
#endif
};

#endif // VIDEO_READ_H
