#include"Task.h"
#include"common.h"

Task::Task() : m_sockfd(-1), m_handler(nullptr)
{

}

Task::Task(int sockfd, int epfd, task_handler_t handler) :  m_sockfd(sockfd), m_epfd(epfd), m_handler(handler)
{

}

Task::~Task()
{

}


