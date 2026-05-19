#include"ThreadPool.h"
#include"../common.h"

ThreadPool::ThreadPool(int thread_max, int thread_min, int queue_max) {
    //线程管理
    m_thread_max = thread_max;
    m_thread_min = thread_min;
    m_thread_alive = 0;
    m_thread_busy = 0;
    m_thread_exitcode = 0;
    m_thread_shutdown = 0;
    m_consumer_tids = new pthread_t[thread_max];
    m_monitor_tid = 0;
    //任务队列
    m_task_queue = new Task[queue_max];
    m_queue_front = 0;
    m_queue_rear = 0;
    m_queue_size = 0;
    m_queue_limit = queue_max;
    //同步资源
    pthread_mutex_init(&m_lock, NULL);
    pthread_cond_init(&m_not_full, NULL);
    pthread_cond_init(&m_not_empty, NULL);
    //预创建
    int err = 0;
    for(int i = 0; i < thread_min; i++) {
        err = pthread_create(&m_consumer_tids[i], NULL, tp_worker, this);
        if(err > 0) {
            cout << "create consumer error:" << strerror(err) << endl;
        }
        else {
           cout << "consumer tid " << m_consumer_tids[i] << endl;
           m_thread_alive++;
        }
    }
    err = pthread_create(&m_monitor_tid, NULL, tp_monitor, this);
    if(err > 0) {
       cout << "create monitor error:" << strerror(err) << endl;
    }
    else {
       cout << "monitor tid " << m_monitor_tid << endl;
    }

}
ThreadPool::~ThreadPool() {
    //停止线程池
    m_thread_shutdown = 1;
    //回收消费者线程
    for(int i = 0; i < m_thread_alive; i++) {
        pthread_join(m_consumer_tids[i], NULL);
    }
    //回收管理者线程
    pthread_join(m_monitor_tid, NULL);
    //销毁同步资源
    pthread_mutex_destroy(&m_lock);
    pthread_cond_destroy(&m_not_empty);
    pthread_cond_destroy(&m_not_full);
    //释放new空间
    delete[] m_consumer_tids;
    delete[] m_task_queue;
}
bool ThreadPool::tp_submit(Task task) {
    if(!m_thread_exitcode) {
        //加锁
        pthread_mutex_lock(&m_lock);
        //条件变量
        while(is_queue_full()) {
            pthread_cond_wait(&m_not_full, &m_lock);
        }
        //入队
        enqueue(task);
        //唤醒一个消费者
        pthread_cond_signal(&m_not_empty);
        //解锁
        pthread_mutex_unlock(&m_lock);
        cout << "producer add task success" << endl;
        return true;
    }
    else {
        cout << "producer add task failed" << endl;
        return false;
    }
}

void ThreadPool::enqueue(Task task) {
    m_task_queue[m_queue_front] = task;
    m_queue_front = (m_queue_front + 1) % m_queue_limit;
    m_queue_size++;
}
Task ThreadPool::dequeue() {
    Task task = m_task_queue[m_queue_rear];
    m_queue_rear = (m_queue_rear + 1) % m_queue_limit;
    m_queue_size--;
    return task;
}
bool ThreadPool::is_queue_empty() const {
    return m_queue_size == 0;
}
bool ThreadPool::is_queue_full() const {
    return m_queue_size == m_queue_limit;
}
void* ThreadPool::tp_worker(void* arg) {
    ThreadPool* tp = (ThreadPool*)arg;
    cout << "consumer " << pthread_self() << " waiting task" << endl;
    while(!tp->m_thread_shutdown) {
            //加锁
            pthread_mutex_lock(&tp->m_lock);
            //队列空就等待
            while(tp->is_queue_empty()) {
                pthread_cond_wait(&tp->m_not_empty, &tp->m_lock);
                //被唤醒先不执行任务，而是先检查开关
                if(tp->m_thread_shutdown) {
                    pthread_mutex_unlock(&tp->m_lock);
                    printf("shutdown close, consumer exit\n");
                    pthread_exit(NULL);
                }
                //被唤醒时为空闲状态，检查退出码
                if(tp->m_thread_exitcode > 0) {
                    tp->m_thread_exitcode--;
                    tp->m_thread_alive--;
                    pthread_mutex_unlock(&tp->m_lock);
                    printf("exitcode close, consumer exit\n");
                    pthread_exit(NULL);
                }
            }
            //出队操作
            Task task = tp->dequeue();
            //唤醒生产者线程
            pthread_cond_signal(&tp->m_not_full);
            //解锁
            tp->m_thread_busy++;
            pthread_mutex_unlock(&tp->m_lock);
            //在锁外执行任务
            task.m_handler(task.m_sockfd, task.m_epfd);
            pthread_mutex_lock(&tp->m_lock);
            tp->m_thread_busy--;
            pthread_mutex_unlock(&tp->m_lock);
        }
        printf("shutdown close, consumer exit\n");
        return NULL;
}
void* ThreadPool::tp_monitor(void* arg) {
    cout << "monitor thread running.." << endl;
    //取出线程池指针
    ThreadPool* tp = (ThreadPool*)arg;
        //线程池运行状态
        while(!tp->m_thread_shutdown) {
            //上锁
            pthread_mutex_lock(&tp->m_lock);
            //获取线程状态信息
            int alive = tp->m_thread_alive;
            int busy = tp->m_thread_busy;
            int idle = alive - busy;
            int task_count = tp->m_queue_size;
            //解锁
            pthread_mutex_unlock(&tp->m_lock);
            printf("info [alive %d] [busy %d] [idle %d] [B/A %.2f%%] [A/M %.2f%%]\n",
            alive, busy, idle, (double)busy / alive * 100, (double)alive / tp->m_thread_max * 100);
            //判断扩容
            if(task_count >= idle || busy > 0.7 * alive) {
                int add = tp->m_thread_min;
                if(alive + add > tp->m_thread_max) {
                    add = tp->m_thread_max - alive;
                }
                for(int i = 0, count = 0; i < tp->m_thread_max && count < add; i++) {
                    int err = pthread_kill(tp->m_consumer_tids[count], 0);
                    //添加线程
                    if(tp->m_consumer_tids[i] == 0 || err == ESRCH) {
                        err = pthread_create(&tp->m_consumer_tids[i], NULL, &tp_worker, (void*)tp);
                        if(err > 0) {
                            printf("create consumer error: %s\n", strerror(err));
                        }
                        else {
                            count++;
                            pthread_mutex_lock(&tp->m_lock);
                            tp->m_thread_alive++;
                            pthread_mutex_unlock(&tp->m_lock);
                        }
                    }
                }
            }
            //判断缩容
            if(tp->m_queue_size == 0 && idle >= 2 * busy) {
                int reduce = tp->m_thread_min;
                pthread_mutex_lock(&tp->m_lock);
                if(alive - reduce < tp->m_thread_min) {
                    reduce = alive - tp->m_thread_min;
                }
                tp->m_thread_exitcode = reduce;
                pthread_mutex_unlock(&tp->m_lock);
                for(int i = 0; i < reduce; i++) {
                    pthread_cond_signal(&tp->m_not_empty);
                }
            }
            //每3秒进行一次判定
            sleep(3);
        }
        printf("shutdown close, monitor exit\n");
        return NULL;
}
