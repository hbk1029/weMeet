#pragma once
#include"../Net/INet.h"

class INetMediator {
public:
    INet* m_INet;

public:
    INetMediator() : m_INet(nullptr) {}
    virtual ~INetMediator() {}

    virtual bool openNet() = 0;
    virtual void run() = 0;
    virtual void closeNet() = 0;
    virtual bool sendData(char* data, int len, int sockfd) = 0;
    virtual void transmitData(char* data, int len, int sockfd) = 0;
    virtual void onClientDisconnect(int sockfd) = 0;
};
