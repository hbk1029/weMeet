TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    Net/TCPServer.cpp \
    Mediator/TCPServerMediator.cpp \
    MySQL/MySQL.cpp \
    Kernel/Kernel.cpp \
    Task/Task.cpp \
    ThreadPool/ThreadPool.cpp \
    main.cpp

HEADERS += \
    Net/def.h \
    Net/INet.h \
    Net/TCPServer.h \
    Mediator/INetMediator.h \
    Mediator/TCPServerMediator.h \
    MySQL/MySQL.h \
    Kernel/Kernel.h \
    Task/Task.h \
    ThreadPool/ThreadPool.h \
    common.h

INCLUDEPATH += \
    /usr/local/include

LIBS += \
    -lpthread \
    -lmysqlclient \
    -L/usr/local/lib -lbcrypt \
    -lcrypt
