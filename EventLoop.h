#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

/*
事件循环类
主要包含的两个大模块
1. Channel
2. Poller - epoll的抽象
*/
class EventLoop : noncopyable{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return m_pollReturnTime; }

    // 在当前 loop 中执行
    void runInLoop(Functor cb);
    // 把 cb 放入队列中，唤醒 loop 所在的线程执行 cb
    void queueInLoop(Functor cb);

    // 用来唤醒 loop 所在的线程
    void wakeup();

    // EventLoop 的方法 => Poller 的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 判断 EventLoop 对象是否在自己的线程里面
    bool isInLoopThread() const { return m_threadid == CurrentThread::tid(); }

private:
    void handleRead(); // wake up
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool m_looping; // 原子操作，通过 CAS 实现的
    std::atomic_bool m_quit; // 标志退出 loop 循环
    const pid_t m_threadid; // 记录当前 loop 所在线程的 id

    Timestamp m_pollReturnTime; // Poller 返回发生事件的 Channels 的时间点
    std::unique_ptr<Poller> m_poller;

    int m_wakeupfd; // 主要作用，当 mainloop 获取一个新用户的 Channel ，通过轮询算法选择一个 subloop ，通过该成员唤醒 subloop 处理 Channel
    std::unique_ptr<Channel> m_wakeupChannel;

    ChannelList m_activeChannels;

    std::atomic_bool m_callingPendingFunctors; // 标识当前 loop 是否有需要执行的回调操作
    std::vector<Functor> m_pendingFunctors; // 存储 loop 需要执行的所有的回调操作
    std::mutex m_mutex; // 互斥锁，用来保护上面的 vector 容器的线程安全操作
};
