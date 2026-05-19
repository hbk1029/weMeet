#ifndef MYFACEDETECT_H
#define MYFACEDETECT_H

#include <QObject>
#include <QString>
#include <vector>
#include "opencv2/core.hpp"

class MyFaceDetect : public QObject
{
    Q_OBJECT
public:
    explicit MyFaceDetect(QObject *parent = nullptr);

    // 加载 haarcascade XML 文件（从 exe 同级目录）
    static void FaceDetectInit();
    // 检测人脸位置，返回矩形框列表；会用眼睛验证过滤误识别
    static void detectAndDisplay(cv::Mat &frame, std::vector<cv::Rect> &faces);
};

#endif // MYFACEDETECT_H
