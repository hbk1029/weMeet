#include "video_read.h"
#include <QMessageBox>
#include <QDebug>
#include <QBuffer>

Video_Read::Video_Read(QObject *parent) : QObject(parent)
{
    m_pTimer = new QTimer(this);
    connect(m_pTimer, &QTimer::timeout, this, &Video_Read::slot_getVideoFrame);
}

Video_Read::~Video_Read()
{
    if (m_pTimer) {
        m_pTimer->stop();
    }
}

//摄像头采集 → JPEG编码 → 发射信号
void Video_Read::slot_getVideoFrame()
{
    cv::Mat frame;
    if (!m_cap.read(frame) || frame.empty()) {
        return;
    }
    // BGR → RGB (separate output mat, avoid in-place issues)
    cv::Mat frameRGB;
    cv::cvtColor(frame, frameRGB, cv::COLOR_BGR2RGB);
    // Mat → QImage (pass bytesPerLine for stride-safe wrapping)
    QImage image(frameRGB.data, frameRGB.cols, frameRGB.rows,
                 frameRGB.step, QImage::Format_RGB888);
    // 缩放到 320×240 (creates independent QImage)
    image = image.scaled(IMAGE_WIDTH, IMAGE_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // JPEG 编码，质量 50
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "JPG", 40);
    // 发射 JPEG 数据
    Q_EMIT sig_videoFrame(ba.data(), ba.size());
}

void Video_Read::slot_openVideo()
{
    m_cap.open(0);
    if (!m_cap.isOpened()) {
        QMessageBox::information(NULL, tr("提示"), tr("视频没有打开"));
        m_pTimer->stop();
        return;
    }
    m_pTimer->start(1000 / FRAME_RATE - 10);
}

void Video_Read::slot_closeVideo()
{
    m_pTimer->stop();
    if (m_cap.isOpened()) {
        m_cap.release();
    }
}
