#pragma once
#include"INet.h"

class TCPClient :public INet {
public:
	bool initNet();
	bool sendData(char* data, int len, long to);
    void recvData();
	void uninitNet();

	static unsigned __stdcall RecvThread(void* arg);

	TCPClient(INetMediator* pThis);
	~TCPClient();
};
