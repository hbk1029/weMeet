#pragma once
#include"INetMediator.h"

class TCPClientMediator : public INetMediator {
    Q_OBJECT
public:
	TCPClientMediator();
	~TCPClientMediator();
	//打开网络
	bool openNet();
	//关闭网络
	void closeNet();
	//发送数据
	bool sendData(char* data, int len, long to);
	//转发数据
    void transmitData(char* data, int len, long from);
};
