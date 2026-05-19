#pragma once

class Task;
typedef void (*task_handler_t)(int sockfd, int epfd);

class Task {
public:
    int m_sockfd;
    int m_epfd;
    task_handler_t m_handler;

public:
    Task();
    Task(int sockfd, int epfd, task_handler_t m_handler);
    ~Task();
};
