#include "video_read.h"
#include "tracelog.h"
#include "myfacedetect.h"
#include <QMessageBox>
#include <QDebug>
#include <QBuffer>
#include <QPainter>

Video_Read::Video_Read(QObject *parent) : QObject(parent),
    m_isFaceDetect(false), m_funnyPic(fp_none), m_frameSkip(0),
    m_frameCount(0), m_pWorkerThread(nullptr)
{
    // 加载萌拍图片（主线程安全）
    m_tuer.load(":/images/tuer.png");
    m_hat.load(":/images/hat.png");

#ifdef USE_H264
    m_pH264Encoder = new H264Encoder(IMAGE_WIDTH, IMAGE_HEIGHT, this);
    // QByteArray 跨线程安全深拷贝，消除竞态
    connect(m_pH264Encoder, &H264Encoder::sig_encodedPacket, this,
            [this](const char* data, int len) {
        Q_EMIT sig_videoFrame(QByteArray(data, len));
    });
#endif

    // 将采集编码全部移到 Worker 线程，主线程仅处理 UI 信号
    m_pWorkerThread = new QThread(this);
    m_pWorkerThread->setObjectName("VideoCaptureThread");
    connect(m_pWorkerThread, &QThread::started, this, &Video_Read::slot_startTimerInThread);
    this->moveToThread(m_pWorkerThread);
    m_pWorkerThread->start();
}

Video_Read::~Video_Read()
{
    if (m_pTimer) {
        m_pTimer->stop();
    }
    if (m_pWorkerThread) {
        m_pWorkerThread->quit();
        m_pWorkerThread->wait(3000);
    }
}

void Video_Read::slot_startTimerInThread()
{
    // 在 Worker 线程中创建定时器，确保定时器事件在正确线程触发
    m_pTimer = new QTimer(this);
    connect(m_pTimer, &QTimer::timeout, this, &Video_Read::slot_getVideoFrame);
}

//摄像头采集 → 人脸检测 → 萌拍 → H.264编码 → 发射信号（全部在 Worker 线程）
void Video_Read::slot_getVideoFrame()
{
    cv::Mat frame;
    if (!m_cap.read(frame) || frame.empty()) {
        TRACE("VIDEO_CAP read failed");
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
    // H.264 编码：缩放 BGR 帧 → 萌拍 → 编码器 → 发射
    cv::Mat frameScaled;
    cv::resize(frame, frameScaled, cv::Size(IMAGE_WIDTH, IMAGE_HEIGHT));

    // 转为 QImage 用于萌拍叠加和本地预览
    cv::Mat frameRGB;
    cv::cvtColor(frameScaled, frameRGB, cv::COLOR_BGR2RGB);
    QImage localImg(frameRGB.data, frameRGB.cols, frameRGB.rows,
                    frameRGB.step, QImage::Format_RGB888);
    localImg = localImg.copy();  // 深拷贝，断开 cv::Mat 共享内存

    // 萌拍：在人脸位置画道具（QPainter 叠加到 QImage 上）
    if (m_isFaceDetect && m_funnyPic != fp_none && !m_vecLastFace.empty()) {
        QImage* tmpImg = nullptr;
        switch (m_funnyPic) {
        case fp_tuer: tmpImg = &m_tuer; break;
        case fp_hat:  tmpImg = &m_hat;  break;
        }
        if (tmpImg && !tmpImg->isNull()) {
            QPainter paint(&localImg);
            for (size_t i = 0; i < m_vecLastFace.size(); ++i) {
                cv::Rect rct = m_vecLastFace[i];
                int x = rct.x + rct.width * 0.5 - tmpImg->width() * 0.5 + 20;
                int y = rct.y - tmpImg->height();
                paint.drawImage(QPoint(x, y), *tmpImg);
            }
        }
    }

    // 本地预览：直接发射带萌拍的 QImage，绕过 H.264 色度丢失
    if (m_frameCount <= 3 || m_frameCount % 30 == 0)
        TRACE("VIDEO_CAP frame=%d thread=%lu", m_frameCount, GetCurrentThreadId());
    Q_EMIT sig_localPreviewFrame(localImg);

    // 将带萌拍的 QImage 转回 cv::Mat 送入 H.264 编码器
    // 确保连续内存，避免 QImage 行对齐导致的问题
    QImage rgbCopy = localImg.convertToFormat(QImage::Format_RGB888);
    cv::Mat matRgb(rgbCopy.height(), rgbCopy.width(), CV_8UC3,
                   rgbCopy.bits(), rgbCopy.bytesPerLine());
    cv::Mat frameBGR = matRgb.clone();  // 深拷贝脱离 QImage 内存
    cv::cvtColor(frameBGR, frameBGR, cv::COLOR_RGB2BGR);
    // PTS 使用帧序号，配合编码器 time_base={1,FRAME_RATE} 即每帧递增1
    m_pH264Encoder->setPts(m_frameCount);
    m_frameCount++;
    m_pH264Encoder->slot_encode(frameBGR);

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
    Q_EMIT sig_videoFrame(ba);
#endif
}

void Video_Read::slot_openVideo()
{
    m_cap.open(0);
    if (!m_cap.isOpened()) {
        qWarning() << "[Video_Read] 摄像头打开失败（Worker 线程）";
        if (m_pTimer) m_pTimer->stop();
        return;
    }
    //初始化人脸检测（加载级联文件）
    MyFaceDetect::FaceDetectInit();
    if (m_pTimer) m_pTimer->start(1000 / FRAME_RATE - 10);
}

void Video_Read::slot_closeVideo()
{
    if (m_pTimer) m_pTimer->stop();
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
