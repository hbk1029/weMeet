#include "ffmpeg_player.h"

FFmpegPlayer::FFmpegPlayer(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(160, 120);
    setStyleSheet("background: #1A1A2E;");
}

void FFmpegPlayer::slot_GetOneImage(QImage img)
{
    m_image = img;
    update();
}

void FFmpegPlayer::paintEvent(QPaintEvent *event)
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
