QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QT += multimedia

LIBS += -lWs2_32 \
    $$PWD/AudioApi/speex/lib/libspeex.dll.a \
    $$PWD/AudioApi/speex/lib/libogg.dll.a \
    $$PWD/AudioApi/opus/libopus.a -lssp

INCLUDEPATH += $$PWD/AudioApi \
    $$PWD/AudioApi/opus
INCLUDEPATH += $$PWD/VideoApi \
    $$PWD/opencv/include \
    $$PWD/ffmpeg-4.2.2/include

LIBS += $$PWD/opencv/lib/libopencv_core420.dll.a \
    $$PWD/opencv/lib/libopencv_imgproc420.dll.a \
    $$PWD/opencv/lib/libopencv_imgcodecs420.dll.a \
    $$PWD/opencv/lib/libopencv_highgui420.dll.a \
    $$PWD/opencv/lib/libopencv_videoio420.dll.a \
    $$PWD/opencv/lib/libopencv_objdetect420.dll.a \
    $$PWD/opencv/lib/libopencv_flann420.dll.a \
    $$PWD/opencv/lib/libopencv_features2d420.dll.a \
    $$PWD/opencv/lib/libopencv_calib3d420.dll.a \
    $$PWD/ffmpeg-4.2.2/lib/libavcodec.dll.a \
    $$PWD/ffmpeg-4.2.2/lib/libavdevice.dll.a \
    $$PWD/ffmpeg-4.2.2/lib/libavfilter.dll.a \
    $$PWD/ffmpeg-4.2.2/lib/libavformat.dll.a \
    $$PWD/ffmpeg-4.2.2/lib/libavutil.dll.a \
    $$PWD/ffmpeg-4.2.2/lib/libpostproc.dll.a \
    $$PWD/ffmpeg-4.2.2/lib/libswresample.dll.a \
    $$PWD/ffmpeg-4.2.2/lib/libswscale.dll.a

SOURCES += \
    AudioApi/audio_read.cpp \
    AudioApi/audio_write.cpp \
    AudioApi/opus_encoder.cpp \
    AudioApi/opus_decoder.cpp \
    VideoApi/video_read.cpp \
    VideoApi/video_write.cpp \
    VideoApi/ffmpeg_decoder.cpp \
    VideoApi/ffmpeg_player.cpp \
    VideoApi/h264_encoder.cpp \
    Mediator/INetMediator.cpp \
    Mediator/TCPClientMediator.cpp \
    Net/TCPClient.cpp \
    addfrienddialog.cpp \
    friendrequestdialog.cpp \
    friendrequestlistdialog.cpp \
    myfacedetect.cpp \
    frienditem.cpp \
    friendlist.cpp \
    kernel.cpp \
    main.cpp \
    logindialog.cpp \
    meetingdialog.cpp

HEADERS += \
    Mediator/INetMediator.h \
    Mediator/TCPClientMediator.h \
    Net/INet.h \
    Net/TCPClient.h \
    Net/def.h \
    addfrienddialog.h \
    friendrequestdialog.h \
    friendrequestlistdialog.h \
    myfacedetect.h \
    common.h \
    frienditem.h \
    friendlist.h \
    kernel.h \
    logindialog.h \
    meetingdialog.h \
    AudioApi/audio_read.h \
    AudioApi/audio_write.h \
    AudioApi/opus_encoder.h \
    AudioApi/opus_decoder.h \
    VideoApi/video_read.h \
    VideoApi/video_write.h \
    VideoApi/ffmpeg_decoder.h \
    VideoApi/ffmpeg_player.h \
    VideoApi/h264_encoder.h

FORMS += \
    addfrienddialog.ui \
    friendrequestdialog.ui \
    frienditem.ui \
    friendlist.ui \
    logindialog.ui \
    meetingdialog.ui

# 复制 haarcascade XML 到输出目录
CONFIG(release, debug|release) {
    COPIES += haarcascade_files
    haarcascade_files.files = $$PWD/haarcascade_frontalface_default.xml $$PWD/haarcascade_eye_tree_eyeglasses.xml
    haarcascade_files.path = $$OUT_PWD/release
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
