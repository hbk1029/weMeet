#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#define USE_OPENGL

#ifdef USE_OPENGL

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QImage>
#include <QMutex>

class OpenGLRender : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLRender(QWidget *parent = nullptr);
    ~OpenGLRender();

public slots:
    void slot_updateImage(QImage img);  // 接收解码后的图像（来自 VideoDecoder）
    void slot_clearFrame();             // 清空画面

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    QImage m_image;
    QMutex m_mutex;
    bool m_hasFrame;
};

#endif // USE_OPENGL
#endif // OPENGL_RENDER_H
