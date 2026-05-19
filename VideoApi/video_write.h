#ifndef VIDEO_WRITE_H
#define VIDEO_WRITE_H

#include <QWidget>
#include <QImage>
#include <QPainter>

class Video_Write : public QWidget
{
    Q_OBJECT
public:
    explicit Video_Write(QWidget *parent = nullptr);

public slots:
    void slot_recvVideoFrame(const char* data, int len);  // JPEG 解码 → 刷新
    void slot_clearFrame();  // 清空画面

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;
};

#endif // VIDEO_WRITE_H
