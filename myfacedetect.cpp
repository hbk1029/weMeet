#include "myfacedetect.h"
#include "opencv2/objdetect.hpp"
#include "opencv2/imgproc.hpp"
#include <QCoreApplication>
#include <QDebug>

static cv::CascadeClassifier face_cascade;
static cv::CascadeClassifier eyes_cascade;
static bool g_bInit = false;

MyFaceDetect::MyFaceDetect(QObject *parent) : QObject(parent)
{
}

void MyFaceDetect::FaceDetectInit()
{
    if (g_bInit) return;

    QString face_cascade_name = QCoreApplication::applicationDirPath()
        + "/haarcascade_frontalface_default.xml";
    QString eyes_cascade_name = QCoreApplication::applicationDirPath()
        + "/haarcascade_eye_tree_eyeglasses.xml";

    qDebug() << "Face cascade:" << face_cascade_name;
    if (!face_cascade.load(face_cascade_name.toStdString())) {
        qDebug() << "--(!)Error loading face cascade";
        return;
    }

    qDebug() << "Eyes cascade:" << eyes_cascade_name;
    if (!eyes_cascade.load(eyes_cascade_name.toStdString())) {
        qDebug() << "--(!)Error loading eyes cascade";
        return;
    }

    g_bInit = true;
}

void MyFaceDetect::detectAndDisplay(cv::Mat &frame, std::vector<cv::Rect> &faces)
{
    if (!g_bInit) return;

    cv::Mat frame_gray;
    cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    equalizeHist(frame_gray, frame_gray);

    // 多尺寸检测人脸（最大 600x600 适配高分辨率摄像头）
    face_cascade.detectMultiScale(frame_gray, faces,
        1.1, 6, 0, cv::Size(80, 80), cv::Size(600, 600));
    qDebug() << "Face detect raw count:" << faces.size();

    // 绘制识别的人脸椭圆
    for (auto ite = faces.begin(); ite != faces.end(); ++ite) {
        cv::Rect rct = *ite;
        cv::Point center(rct.x + rct.width * 0.5, rct.y + rct.height * 0.5);
        ellipse(frame, center, cv::Size(rct.width * 0.5, rct.height * 0.5),
                0, 0, 360, cv::Scalar(255, 0, 255), 4, 8, 0);
    }

    // 用眼睛检测验证，过滤误识别
    for (auto ite = faces.begin(); ite != faces.end(); ) {
        cv::Rect rct = *ite;
        cv::Mat faceROI = frame_gray(rct);
        std::vector<cv::Rect> eyes;
        eyes_cascade.detectMultiScale(faceROI, eyes, 1.1, 2, 0, cv::Size(20, 20));

        // 至少检测到1只眼睛 + 眼睛位置在脸上半部分才认为是真脸
        if (eyes.empty()) {
            ite = faces.erase(ite);
        } else {
            if (rct.height * 0.5 < eyes[0].y) {
                ite = faces.erase(ite);
                continue;
            }
            ++ite;
        }
    }
}
