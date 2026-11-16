#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include <cstdio>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg):m_baseLoop(baseLoop), 
                                                                                          m_name(nameArg), 
                                                                                          m_started(false), 
                                                                                          m_numThreads(0), 
                                                                                          m_next(0) {}

EventLoopThreadPool::~EventLoopThreadPool() = default;

void EventLoopThreadPool::start(const ThreadInitCallBack& cb)
{
    m_started = true;

    for(int i = 0;i < m_numThreads;++i)
    {
        char buf[m_name.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", m_name.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        m_threads.push_back(std::unique_ptr<EventLoopThread>(t));
        m_loops.push_back(t->startLoop()); // 底层创建线程，绑定一个新的 EventLoop ，并返回该 loop 的地址
    }

    // 如果整个服务端只有一个线程运行着 baseLoop
    if(m_numThreads == 0 && cb)
    {
        cb(m_baseLoop);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop* loop = m_baseLoop;

    if(!m_loops.empty()) // 通过轮询获取下一个处理事件的 loop
    {
        loop = m_loops[m_next];
        m_next++;
        if(m_next >= m_loops.size())
        {
            m_next = 0;
        }
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if(m_loops.empty())
    {
        return std::vector<EventLoop*>(1, m_baseLoop);
    }
    else
    {
        return m_loops;
    }
}