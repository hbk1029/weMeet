#ifndef VIDEO_WRITE_H
#define VIDEO_WRITE_H

#include <QWidget>
#include <QImage>
#include <QMutex>

// 纯渲染组件：通过 slot_updateImage 接收解码后的 QImage 并绘制
class Video_Write : public QWidget
{
    Q_OBJECT
public:
    explicit Video_Write(QWidget *parent = nullptr);
    ~Video_Write();

public slots:
    void slot_updateImage(QImage img);  // 接收解码后的图像
    void slot_clearFrame();             // 清空画面

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;
    QMutex m_mutex;
    bool m_hasFrame;
};

#endif // VIDEO_WRITE_H
