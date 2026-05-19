#include "video_read.h"
#include "myfacedetect.h"
#include <QMessageBox>
#include <QDebug>
#include <QBuffer>
#include <QPainter>

Video_Read::Video_Read(QObject *parent) : QObject(parent),
    m_isFaceDetect(false), m_funnyPic(fp_none), m_frameSkip(0)
{
    m_pTimer = new QTimer(this);
    connect(m_pTimer, &QTimer::timeout, this, &Video_Read::slot_getVideoFrame);

    //加载萌拍图片
    m_tuer.load(":/images/tuer.png");
    m_hat.load(":/images/hat.png");

#ifdef USE_H264
    m_pH264Encoder = new H264Encoder(IMAGE_WIDTH, IMAGE_HEIGHT, this);
    connect(m_pH264Encoder, &H264Encoder::sig_encodedPacket,
            this, &Video_Read::sig_videoFrame);
#endif
}

Video_Read::~Video_Read()
{
    if (m_pTimer) {
        m_pTimer->stop();
    }
}

//摄像头采集 → 人脸检测 → 萌拍 → JPEG编码 → 发射信号
void Video_Read::slot_getVideoFrame()
{
    cv::Mat frame;
    if (!m_cap.read(frame) || frame.empty()) {
        return;
    }

    //人脸检测每5帧运行一次，减少 CPU 占用
    std::vector<cv::Rect> faces;
    if (m_isFaceDetect && m_funnyPic != fp_none) {
        m_frameSkip++;
        if (m_frameSkip >= 5) {
            m_frameSkip = 0;
            MyFaceDetect::detectAndDisplay(frame, faces);
            if (faces.size() > 0) {
                m_vecLastFace = faces;
            }
        }
    }

#ifdef USE_H264
    // H.264 编码：缩放 BGR 帧 → 编码器 → 发射
    cv::Mat frameScaled;
    cv::resize(frame, frameScaled, cv::Size(IMAGE_WIDTH, IMAGE_HEIGHT));
    m_pH264Encoder->slot_encode(frameScaled);
#else
    // BGR → RGB
    cv::Mat frameRGB;
    cv::cvtColor(frame, frameRGB, cv::COLOR_BGR2RGB);
    QImage image(frameRGB.data, frameRGB.cols, frameRGB.rows,
                 frameRGB.step, QImage::Format_RGB888);

    //萌拍：在人脸位置画道具
    if (m_isFaceDetect && m_funnyPic != fp_none && !m_vecLastFace.empty()) {
        QImage* tmpImg = nullptr;
        switch (m_funnyPic) {
        case fp_tuer: tmpImg = &m_tuer; break;
        case fp_hat:  tmpImg = &m_hat;  break;
        }
        if (tmpImg && !tmpImg->isNull()) {
            QPainter paint(&image);
            for (size_t i = 0; i < m_vecLastFace.size(); ++i) {
                cv::Rect rct = m_vecLastFace[i];
                int x = rct.x + rct.width * 0.5 - tmpImg->width() * 0.5 + 20;
                int y = rct.y - tmpImg->height();
                QPoint p(x, y);
                paint.drawImage(p, *tmpImg);
            }
        }
    }

    // 缩放到 320×240
    image = image.scaled(IMAGE_WIDTH, IMAGE_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // JPEG 编码
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "JPG", 40);
    // 发射 JPEG 数据
    Q_EMIT sig_videoFrame(ba.data(), ba.size());
#endif
}

void Video_Read::slot_openVideo()
{
    m_cap.open(0);
    if (!m_cap.isOpened()) {
        QMessageBox::information(NULL, tr("提示"), tr("视频没有打开"));
        m_pTimer->stop();
        return;
    }
    //初始化人脸检测（加载级联文件）
    MyFaceDetect::FaceDetectInit();
    m_pTimer->start(1000 / FRAME_RATE - 10);
}

void Video_Read::slot_closeVideo()
{
    m_pTimer->stop();
    if (m_cap.isOpened()) {
        m_cap.release();
    }
    m_vecLastFace.clear();
}

void Video_Read::slot_setFaceDetect(bool on)
{
    m_isFaceDetect = on;
    if (!on) {
        m_vecLastFace.clear();
    }
}

void Video_Read::slot_setFunnyPic(int type)
{
    m_funnyPic = type;
    if (type == fp_none) {
        m_vecLastFace.clear();
    }
}
