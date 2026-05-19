#pragma once
#include"INet.h"
#include"../ThreadPool/ThreadPool.h"
#include"common.h"
#include <set>

class TCPServer : public INet {
public:
    int m_listenfd;
    int m_epfd;
    ThreadPool* m_pThreadPool;
    static INetMediator* m_pMediator;
    static pthread_mutex_t m_lock;
    //断点续传
    static unordered_map<int, vector<char>> m_sendBuf;
    static unordered_map<int, vector<char>> m_recvBuf;  // 已接收的包体数据
    static unordered_map<int, int> m_recvLen;                // 还需接收的字节数（0表示等待长度头）
    static std::set<int> m_sendingFds;                       // 正在发送中的fd集合

public:
    TCPServer(INetMediator* pThis);
    ~TCPServer();
    virtual bool initNet() override;
    virtual bool sendData(char* data, int len, int sockfd) override;
    virtual void run() override;
    virtual void uninitNet() override;

    int net_listen_create();
    int ep_init(int sockfd);
    void ev_loop();

public:
    int handle_accept();
    int handle_read(int sockfd);
    int handle_write(int sockfd);
    static void deal_recv(int sockfd, int epfd);
    static void deal_send(int sockfd, int epfd);
    static void update_event(int sockfd, int epfd);
};
