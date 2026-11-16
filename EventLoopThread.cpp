#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallBack& cb, const std::string& name):m_loop(nullptr), 
                                                                                        m_exiting(false), 
                                                                                        m_thread(std::bind(&EventLoopThread::threadFunc, this), name), 
                                                                                        m_callback(cb) {}

EventLoopThread::~EventLoopThread()
{
    m_exiting = true;
    if(m_loop != nullptr)
    {
        m_loop->quit();
        m_thread.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    m_thread.start(); // 启动底层的新线程

    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while(m_loop == nullptr)
        {
            m_cond.wait(lock);
        }
        loop = m_loop;
    }
    return loop;
}

// 下面这个方法是在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的 EventLoop ，和上面的线程是一一对应的，one loop per thread

    if(m_callback)
    {
        m_callback(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_loop = &loop;
        m_cond.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(m_mutex);
    m_loop = nullptr;
}