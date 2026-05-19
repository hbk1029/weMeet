#ifndef FFMPEG_PLAYER_H
#define FFMPEG_PLAYER_H

#include <QWidget>
#include <QImage>
#include <QPainter>

class FFmpegPlayer : public QWidget
{
    Q_OBJECT
public:
    explicit FFmpegPlayer(QWidget *parent = nullptr);

public slots:
    void slot_GetOneImage(QImage img);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;
};

#endif // FFMPEG_PLAYER_H
