#ifndef VIDEO_READ_H
#define VIDEO_READ_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include "common.h"

//图片的宽高
#define IMAGE_WIDTH (320)
#define IMAGE_HEIGHT (240)

class Video_Read : public QObject
{
    Q_OBJECT
public:
    explicit Video_Read(QObject *parent = nullptr);
    ~Video_Read();

signals:
    void sig_videoFrame(const char* data, int len);

public slots:
    void slot_getVideoFrame();  // 定时采集 → 编码 → 发射信号
    void slot_openVideo();       // 打开摄像头
    void slot_closeVideo();      // 关闭摄像头

private:
    QTimer *m_pTimer;
    cv::VideoCapture m_cap;
};

#endif // VIDEO_READ_H
