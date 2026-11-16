#pragma once

#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable{
public:
    using ThreadInitCallBack = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads)
    {
        m_numThreads = numThreads;
    }

    void start(const ThreadInitCallBack& cb = ThreadInitCallBack());

    // 如果工作在多线程中，baseLoop 会默认以轮询的方式分配 Channel 给 subloop
    EventLoop* getNestLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const { return m_started; }
    const std::string name() const { return m_name; }

private:
    EventLoop* m_baseLoop;
    std::string m_name;
    bool m_started;
    int m_numThreads;
    int m_next;
    std::vector<std::unique_ptr<EventLoopThread>> m_threads;
    std::vector<EventLoop*> m_loops;
};