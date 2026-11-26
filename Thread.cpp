#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>
#include <cstdio>

std::atomic_int32_t Thread::m_numCreated{0};

Thread::Thread(ThreadFunc func, const std::string& name):m_started(false), 
                                                         m_joined(false), 
                                                         m_tid(0), 
                                                         m_func(std::move(func)), 
                                                         m_name(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(m_started && !m_joined)
    {
        m_thread->join(); // thread 类提供的设置分离线程的方法
    }
}

void Thread::start() // 一个 Thread 对象，记录的就是一个新线程的详细信息
{
    m_started = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    // 开启线程
    m_thread = std::make_shared<std::thread>([&](){
        // 获取线程的 tid 值
        m_tid = CurrentThread::tid();

        sem_post(&sem);

        // 开启一个新线程，专门执行该线程函数
        m_func();
    });

    // 这里必须等待获取上面新创建的线程的 tid 值
    sem_wait(&sem);
}

void Thread::join()
{
    m_joined = true;
    m_thread->join();
}

void Thread::setDefaultName()
{
    int num = ++m_numCreated;
    if(m_name.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        m_name = buf;
    }
}
