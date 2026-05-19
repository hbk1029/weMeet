#include "video_write.h"
#include <QDebug>

Video_Write::Video_Write(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(160, 120);
    setStyleSheet("background: #1A1A2E;");
}

void Video_Write::slot_recvVideoFrame(const char* data, int len)
{
    if (!data || len <= 2) return;
    if ((unsigned char)data[0] != 0xFF || (unsigned char)data[1] != 0xD8) return;
    if (m_image.loadFromData((const uchar*)data, len, "JPG")) {
        update();
    }
}

void Video_Write::slot_clearFrame()
{
    m_image = QImage();
    update();
}

void Video_Write::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_image.isNull()) {
        painter.fillRect(rect(), QColor(0x1A, 0x1A, 0x2E));
        painter.setPen(QColor(0x66, 0x66, 0x66));
        painter.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("等待视频..."));
        return;
    }

    QImage scaled = m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = (width() - scaled.width()) / 2;
    int y = (height() - scaled.height()) / 2;
    painter.drawImage(x, y, scaled);
}
