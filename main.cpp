#include "logindialog.h"
#include "kernel.h"
#include "tracelog.h"
#include <QApplication>

// SDL 的 SDL_main.h 会 #define main SDL_main，必须取消
#undef main

int main(int argc, char *argv[])
{
    //启用高DPI自适应缩放
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    //启用高DPI位图
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication a(argc, argv);

    // 初始化线程安全诊断日志
    initTraceLog();

    Kernel::getSingleInstance();
    return a.exec();
}
