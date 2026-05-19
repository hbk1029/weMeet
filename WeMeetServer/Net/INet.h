#pragma once

class INetMediator;

class INet {
public:
    int m_listenfd;
    INetMediator* m_pMediator;

public:
    INet() : m_listenfd(-1), m_pMediator(nullptr) {}
    virtual ~INet() {}

    virtual bool initNet() = 0;
    virtual bool sendData(char* data, int len, int sockfd) = 0;
    virtual void run() = 0;
    virtual void uninitNet() = 0;
};
