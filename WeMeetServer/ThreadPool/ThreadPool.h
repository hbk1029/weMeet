#pragma once
#include<pthread.h>
#include"Task/Task.h"

class ThreadPool {
public:
    //线程管理
    int m_thread_max;
    int m_thread_min;
    int m_thread_alive;
    int m_thread_busy;
    int m_thread_exitcode;
    int m_thread_shutdown;
    pthread_t* m_consumer_tids;
    pthread_t m_monitor_tid;
    //任务队列
    Task* m_task_queue;
    int m_queue_front;
    int m_queue_rear;
    int m_queue_size;
    int m_queue_limit;
    //同步资源
    pthread_mutex_t m_lock;
    pthread_cond_t m_not_full;
    pthread_cond_t m_not_empty;

public:
    ThreadPool(int thread_max, int thread_min, int queue_max);
    ~ThreadPool();
    bool tp_submit(Task task);
private:
    //内部方法
    void enqueue(Task task);
    Task dequeue();
    bool is_queue_empty() const;
    bool is_queue_full() const;
    static void* tp_worker(void* arg);
    static void* tp_monitor(void* arg);
};
