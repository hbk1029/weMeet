 #include"TCPServer.h"
#include"common.h"
#include"def.h"
#include"../ThreadPool/ThreadPool.h"
#include"../Kernel/Kernel.h"

pthread_mutex_t TCPServer::m_lock = PTHREAD_MUTEX_INITIALIZER;

INetMediator* TCPServer::m_pMediator = nullptr;
unordered_map<int, vector<char>> TCPServer::m_sendBuf;
unordered_map<int, vector<char>> TCPServer::m_recvBuf;
unordered_map<int, int> TCPServer::m_recvLen;
std::set<int> TCPServer::m_sendingFds;

TCPServer::TCPServer(INetMediator* pThis) : m_listenfd(-1), m_epfd(-1), m_pThreadPool(nullptr)
{
    m_pMediator = pThis;
    m_pThreadPool = new ThreadPool(_DEF_THREAD_MAX, _DEF_THREAD_MIN, _DEF_QUEUE_MAX);
}

TCPServer::~TCPServer()
{

}

bool TCPServer::initNet() //初始化网络 (listen_fd, epoll)
{
    //初始化监听socket
    int listen_fd = net_listen_create();
    if(-1 == listen_fd) {
        cout << "net_listen_create failed" << endl;
        return false;
    }
    else {
        cout << "net_listen_create success" << endl;
    }
    m_listenfd = listen_fd;
    //初始化epoll
    int epfd = ep_init(listen_fd);
    if(-1 == epfd) {
        cout << "ep_init failed" << endl;
        return false;
    }
    else {
        cout << "ep_init success" << endl;
    }
    m_epfd = epfd;
    return true;
}

bool TCPServer::sendData(char* data, int len, int sockfd)
{
    cout << __func__ << endl;
    if(!data || len <= 0) {
        return false;
    }
    //把数据存入m_sendBuf
    vector<char> package(sizeof(int) + len);
    memcpy(package.data(), &len, sizeof(int));         //从内存开头开始写
    memcpy(package.data() + sizeof(int), data, len);   //接着长度头后面写内容

    pthread_mutex_lock(&m_lock);
    //追加数据，防止未发送数据被丢弃
    vector<char>& buf = m_sendBuf[sockfd];
    buf.insert(buf.end(), package.begin(), package.end());
    update_event(sockfd, m_epfd); //立即同步事件
    pthread_mutex_unlock(&m_lock);
    return true;
}

void TCPServer::run() //使用循环监听接收数据
{
    ev_loop();
}

void TCPServer::uninitNet()
{
    //关闭套接字
    close(m_listenfd);
    //关闭epoll
    close(m_epfd);
}

int TCPServer::net_listen_create()
{
    //创建套接字
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == listen_fd) {
        perror("socket call error");
        return -1;
    }
    //绑定
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_DEF_SERVER_PORT);
    inet_pton(AF_INET, _DEF_SERVER_IP, &server_addr.sin_addr);
    int err = bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr));
    if(-1 == err) {
        perror("bind call error");
        return -1;
    }
    //监听
    err = listen(listen_fd, _DEF_TCP_LISTEN_MAX);
    if(-1 == err) {
        perror("listen call error");
        return -1;
    }
    return listen_fd;
}

int TCPServer::ep_init(int listen_fd)
{
    int epfd = epoll_create(_DEF_EPOLL_SIZE);
    if(epfd < 0) {
        perror("epoll_create call error");
        return -1;
    }
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    int err = epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
    if(-1 == err) {
        close(epfd);
        perror("epoll_ctl call error");
        return -1;
    }
    return epfd;
}

void TCPServer::ev_loop()
{
    epoll_event ready_events[_DEF_EPOLL_SIZE];
    while(!m_pThreadPool->m_thread_shutdown) {
        int ready_num = epoll_wait(m_epfd, ready_events, _DEF_EPOLL_SIZE, -1);
        if(ready_num > 0) {
            for(int i = 0; i < ready_num; i++) {
                int fd = ready_events[i].data.fd;
                uint32_t evs = ready_events[i].events;   // 取出原始事件

                            // 添加调试输出（注意 hex 进制，方便识别 EPOLLOUT）
                            cout << "epoll event: fd=" << fd << " events=0x" << hex << evs << dec << endl;
                if(fd == m_listenfd) {
                    handle_accept();
                }
                else if(ready_events[i].events & EPOLLIN) {
                    handle_read(fd);
                }
                else if(ready_events[i].events & EPOLLOUT) {
                    handle_write(fd);
                }
            }
        }
    }
}

int TCPServer::handle_accept()
{
    sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int client_fd = accept(m_listenfd, (sockaddr*)&client_addr, &addrlen);
    if(-1 == client_fd) {
        perror("accept call error");
        return -1;
    }
    //接受连接成功
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    char client_ip[_DEF_IP_SIZE] = "";
    printf("connect %s success\n", inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, _DEF_IP_SIZE));
    //更新epoll信息
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.fd = client_fd;
    epoll_ctl(m_epfd, EPOLL_CTL_ADD, client_fd, &ev);
    return 0;
}

int TCPServer::handle_read(int sockfd)
{
    Task task(sockfd, m_epfd, deal_recv);
    m_pThreadPool->tp_submit(task);
    return 0;
}

int TCPServer::handle_write(int sockfd)
{
    pthread_mutex_lock(&m_lock);
    if (m_sendingFds.find(sockfd) != m_sendingFds.end()) {
        pthread_mutex_unlock(&m_lock);
        return 0;
    }
    m_sendingFds.insert(sockfd);
    pthread_mutex_unlock(&m_lock);
    Task task(sockfd, m_epfd, deal_send);
    m_pThreadPool->tp_submit(task);
    return 0;
}

void TCPServer::deal_send(int sockfd, int epfd)
{
    cout << __func__ << endl;
    while(1) {
        //将查找和准备数据放入锁内，并拷贝待发内容，避免在持锁期间发送
        pthread_mutex_lock(&m_lock);
        auto it = m_sendBuf.find(sockfd);
        if(it == m_sendBuf.end()) {
            //当前没有数据可发, 切回监听读事件
            m_sendingFds.erase(sockfd);
            update_event(sockfd, epfd);
            pthread_mutex_unlock(&m_lock);
            return;
        }
        //从原缓冲区拷贝一份数据，然后解锁，让send在锁外执行
        vector<char> buf = it->second;
        pthread_mutex_unlock(&m_lock);
        int send_len = send(sockfd, buf.data(), buf.size(), 0);
        if(send_len > 0) {
            pthread_mutex_lock(&m_lock);
            //重找迭代器，因为解锁期间m_sendBuf可能被其他线程修改
            it = m_sendBuf.find(sockfd);
            if (it != m_sendBuf.end()) {
                if(send_len == (int)buf.size()) {
                    //发送长度等于原始包长度，但缓冲区可能还有后续追加的数据，只删除已发送部分
                    if (it->second.size() == buf.size()) {
                        m_sendBuf.erase(it);//发送完毕，删除
                    } else {
                        //有新增数据，只移除已发送的部分
                        it->second.erase(it->second.begin(), it->second.begin() + send_len);
                    }
                    update_event(sockfd, epfd);//更新事件
                }
                else {
                    it->second.erase(it->second.begin(), it->second.begin() + send_len);//不需要改事件，还是EPOLLOUT
                }
            }
            pthread_mutex_unlock(&m_lock);
            continue;//继续循环, 检查是否还有新包
        }
        //send返回0视为连接关闭
        else if(0 == send_len) {
            cout << "send return 0, connection closed" << endl;
            pthread_mutex_lock(&m_lock);
            m_sendBuf.erase(sockfd);
            m_sendingFds.erase(sockfd);
            pthread_mutex_unlock(&m_lock);
            m_pMediator->onClientDisconnect(sockfd);
            close(sockfd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, nullptr);
            return;
        }
        else {
            if(EAGAIN == errno) {
                pthread_mutex_lock(&m_lock);
                //缓冲区满, 重新注册写事件, 等待下次触发
                m_sendingFds.erase(sockfd);
                update_event(sockfd, epfd);
                pthread_mutex_unlock(&m_lock);
                return;
            }
            else {
                //发送错误, 关闭连接
                cout << "send call error:" << errno << endl;
                pthread_mutex_lock(&m_lock);
                m_sendBuf.erase(sockfd);
                m_sendingFds.erase(sockfd);
                pthread_mutex_unlock(&m_lock);
                m_pMediator->onClientDisconnect(sockfd);
                close(sockfd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, nullptr);
                return;
            }
        }
    }
    pthread_mutex_lock(&m_lock);
    m_sendingFds.erase(sockfd);
    auto it = m_sendBuf.find(sockfd);
    if (it != m_sendBuf.end() && !it->second.empty()) {
        update_event(sockfd, epfd);
    }
    pthread_mutex_unlock(&m_lock);
}

void TCPServer::deal_recv(int sockfd, int epfd)
{
    cout << __func__ << endl;
    //期望接收的包体字节数
    int& expect_len = m_recvLen[sockfd];
    vector<char>& buf = m_recvBuf[sockfd];
    //EPOLLIN触发, 必须把当前内核缓冲区所有的数据都读出来
    while (1) {
        //先读取一部分
        char tmp[_DEF_BUFFER_SIZE];
        int recv_len = recv(sockfd, tmp, sizeof(tmp), 0);
        if(recv_len > 0) {
            //追加到buf
            buf.insert(buf.end(), tmp, tmp + recv_len);
        }
        else if(0 == recv_len) {
            cout << "client exit (connection closed)" << endl;
            m_pMediator->onClientDisconnect(sockfd);
            //关闭前清除待发数据，防止残留
            pthread_mutex_lock(&m_lock);
            m_sendBuf.erase(sockfd);
            pthread_mutex_unlock(&m_lock);
            close(sockfd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, nullptr);
            return;
        }
        else {
            if (errno == EAGAIN) {
                // 没有更多数据可读，重新注册事件，等待下次通知
                pthread_mutex_lock(&m_lock);
                update_event(sockfd, epfd);
                pthread_mutex_unlock(&m_lock);
                return;   // 本次处理结束，保留缓冲区中的半包数据
            }
            else {
                cout << "recv pack_len error, errno=" << errno << endl;
            m_pMediator->onClientDisconnect(sockfd);
                //清除待发数据，防止残留
                pthread_mutex_lock(&m_lock);
                m_sendBuf.erase(sockfd);
                pthread_mutex_unlock(&m_lock);
                close(sockfd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, nullptr);
                return;
            }
        }
        //拆包, 读取完整包
        while(1) {
            if(0 == expect_len) {
                //需要先从m_recvBuf中取出4字节的包体长度
                if(buf.size() < sizeof(int)) {
                    break;
                }
                //取出4字节
                int pack_len;
                memcpy(&pack_len, buf.data(), sizeof(int));
                //校验包长度
                if(pack_len < 0 || pack_len > 1024 * 1024) {
                    cout << "invalid pack_len" << pack_len << endl;
                    pthread_mutex_lock(&m_lock);
                    m_recvLen.erase(sockfd);
                    m_recvBuf.erase(sockfd);
                    pthread_mutex_unlock(&m_lock);
                    m_pMediator->onClientDisconnect(sockfd);
                    close(sockfd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, nullptr);
                    return;
                }
                expect_len = pack_len;
                //buf中移除4字节
                buf.erase(buf.begin(), buf.begin() + sizeof(int));
            }
            //拆包, 读取一个完整包
            if(expect_len > 0 && buf.size() >= (size_t)expect_len) {
                char* recv_buf = new char[expect_len];
                memcpy(recv_buf, buf.data(), (size_t)expect_len);
                buf.erase(buf.begin(), buf.begin() + expect_len);
                //保存包长度
                int pack_len = expect_len;
                expect_len = 0; //清零,准备接收下一个包的长度头
                //一个包接收完毕，发给中介者
                m_pMediator->transmitData(recv_buf, pack_len, sockfd);
                delete[] recv_buf;
                //继续循环,看缓冲区能不能凑出下一个完整包
            }
            //数据不够完整包, 跳出拆包循环
            else {
                break;
            }
        }
        //如果expect_len > 0 且 缓冲区还有数据，说明还在等待完整包
    }
}

void TCPServer::update_event(int sockfd, int epfd)
{
    int events = EPOLLET | EPOLLONESHOT;
    events |= EPOLLIN;   //总是监听读

    auto it = TCPServer::m_sendBuf.find(sockfd);
    if (it != TCPServer::m_sendBuf.end() && !it->second.empty()) {
        events |= EPOLLOUT;   //有待发数据时同时监听写
    }

    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = events;

    if (epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev) == -1) {
        perror("updateEpollEvent: epoll_ctl");
    }
}

