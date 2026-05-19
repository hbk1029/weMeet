#pragma once
#include<QObject>

class INet;

class INetMediator : public QObject {
    Q_OBJECT
public:
    INetMediator();
    ~INetMediator();
	//打开网络
	virtual bool openNet() = 0;
	//关闭网络
	virtual void closeNet() = 0;
	//发送数据
	virtual bool sendData(char* data, int len, long to) = 0;
	//转发数据
    virtual void transmitData(char* data, int len, long from) = 0;

protected:
    INet* m_INet;

signals:
    //发送数据的信号
    void sig_dealData(char* data, int len, long from);
};
