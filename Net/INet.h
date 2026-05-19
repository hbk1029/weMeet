#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<WinSock2.h>
#include<iostream>
#include<process.h>
using namespace std;

class INetMediator;

class INet {
public:
	virtual bool initNet() = 0;
	virtual bool sendData(char* data, int len, long to) = 0;
	virtual void recvData() = 0;
	virtual void uninitNet() = 0;

	INet() : m_sock(INVALID_SOCKET), m_isRunning(1), m_hand(nullptr), m_INetMedia(nullptr) {

	}
	virtual ~INet() {
		
	}
public:
	SOCKET m_sock;
	int m_isRunning;
	HANDLE m_hand;
	INetMediator* m_INetMedia;
};
