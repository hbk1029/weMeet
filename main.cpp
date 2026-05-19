#include "logindialog.h"
#include"kernel.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    //启用高DPI自适应缩放
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    //启用高DPI位图,让加载的图片也保持清晰
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication a(argc, argv);
    Kernel::getSingleInstance();
    return a.exec();
}
