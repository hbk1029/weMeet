QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QT += multimedia

LIBS += -lWs2_32 \
    $$PWD/AudioApi/speex/lib/libspeex.dll.a \
    $$PWD/AudioApi/speex/lib/libogg.dll.a

INCLUDEPATH += $$PWD/AudioApi
INCLUDEPATH += $$PWD/VideoApi \
    $$PWD/opencv/opencv-release/include

LIBS += $$PWD/opencv/lib/libopencv_world420.dll.a

SOURCES += \
    AudioApi/audio_read.cpp \
    AudioApi/audio_write.cpp \
    VideoApi/video_read.cpp \
    VideoApi/video_write.cpp \
    Mediator/INetMediator.cpp \
    Mediator/TCPClientMediator.cpp \
    Net/TCPClient.cpp \
    addfrienddialog.cpp \
    friendrequestdialog.cpp \
    friendrequestlistdialog.cpp \
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
    common.h \
    frienditem.h \
    friendlist.h \
    kernel.h \
    logindialog.h \
    meetingdialog.h \
    AudioApi/audio_read.h \
    AudioApi/audio_write.h \
    VideoApi/video_read.h \
    VideoApi/video_write.h

FORMS += \
    addfrienddialog.ui \
    friendrequestdialog.ui \
    frienditem.ui \
    friendlist.ui \
    logindialog.ui \
    meetingdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
