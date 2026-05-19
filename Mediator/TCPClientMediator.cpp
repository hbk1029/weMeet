#include"TCPClientMediator.h"
#include"../Net/TCPClient.h"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

//打开网络
bool TCPClientMediator::openNet() {
	QDir().mkpath("C:\\temp");
	return m_INet->initNet();
}
//关闭网络
void TCPClientMediator::closeNet() {
	m_INet->uninitNet();
}
//发送数据
bool TCPClientMediator::sendData(char* data, int len, long to) {
	return m_INet->sendData(data, len, to);
}
//转发数据
void TCPClientMediator::transmitData(char* data, int len, long from) {
    int type = *(int*)data;
    QFile f(QString("C:\\temp\\chat_debug_%1.log").arg(QCoreApplication::applicationPid()));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream s(&f);
        s << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
          << " TCP_RECV type=" << type << " len=" << len << "\n";
        f.close();
    }
    Q_EMIT sig_dealData(data, len, from);
}

TCPClientMediator::TCPClientMediator() {
	m_INet = new TCPClient(this);
}
TCPClientMediator::~TCPClientMediator() {

}
