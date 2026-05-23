#include "video_write.h"
#include <QPainter>

Video_Write::Video_Write(QWidget *parent) : QWidget(parent),
    m_hasFrame(false)
{
    setMinimumSize(160, 120);
    setStyleSheet("background: #1A1A2E;");
}

Video_Write::~Video_Write()
{
}

void Video_Write::slot_updateImage(QImage img)
{
    if (img.isNull()) return;
    m_mutex.lock();
    m_image = img;
    m_hasFrame = true;
    m_mutex.unlock();
    update();
}

void Video_Write::slot_clearFrame()
{
    m_mutex.lock();
    m_image = QImage();
    m_hasFrame = false;
    m_mutex.unlock();
    update();
}

void Video_Write::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    m_mutex.lock();
    bool hasFrame = m_hasFrame && !m_image.isNull();
    if (hasFrame) {
        QImage scaled = m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        m_mutex.unlock();
        painter.drawImage(x, y, scaled);
    } else {
        m_mutex.unlock();
        painter.fillRect(rect(), QColor(0x1A, 0x1A, 0x2E));
        painter.setPen(QColor(0x66, 0x66, 0x66));
        painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("等待视频..."));
    }
}
