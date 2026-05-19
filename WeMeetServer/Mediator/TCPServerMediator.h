#pragma once
#include"INetMediator.h"

class TCPServerMediator : public INetMediator {
public:
    TCPServerMediator();
    ~TCPServerMediator();

    //打开网络
    bool openNet();
    //启动网络服务
    void run();
    //关闭网络
    void closeNet();
    //发送数据
    bool sendData(char* data, int len, int sockfd);
    //转发数据
    void transmitData(char* data, int len, int sockfd);
    void onClientDisconnect(int sockfd);
};
