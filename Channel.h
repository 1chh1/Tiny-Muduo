#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>

class EventLoop;

/*
Channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
还绑定了Poller监听返回的具体事件
*/
class Channel : noncopyable{
public:
    using EventCallBack = std::function<void()>; // 事件的回调
    using ReadEventCallBack = std::function<void(Timestamp)>; // 只读事件的回调

    Channel(EventLoop* Loop, int fd);
    ~Channel();

    // fd得到Poller通知以后，处理事件的，调用相应的回调方法
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallBack(ReadEventCallBack cb)
    {
        m_readCallBack = std::move(cb);
    }
    void setWriteCallBack(EventCallBack cb)
    {
        m_writeCallBack = std::move(cb);
    }
    void setCloseCallBack(EventCallBack cb)
    {
        m_closeCallBack = std::move(cb);
    }
    void setErrorCallBack(EventCallBack cb)
    {
        m_errorCallBack = std::move(cb);
    }

    // 防止当Channel被手动remove掉，Channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const
    {
        return m_fd;
    }
    int events() const
    {
        return m_events;
    }
    int set_revents(int rev)
    {
        m_revents = rev;
    }

    // 设置fd相应的事件状态
    void enableReading()
    {
        m_events |= careReadEvent;
        update();
    }
    void disableReading()
    {
        m_events &= ~careReadEvent;
        update();
    }
    void enableWriting()
    {
        m_events |= careWriteEvent;
        update();
    }
    void disableWriting()
    {
        m_events &= ~careWriteEvent;
        update();
    }
    void disableAll()
    {
        m_events = careNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const
    {
        return m_events == careNoneEvent;
    }
    bool isWriting() const
    {
        return m_events & careWriteEvent;
    }
    bool isReading() const
    {
        return m_events & careReadEvent;
    }

    int index()
    {
        return m_index;
    }
    void set_index(int idx)
    {
        m_index = idx;
    }

    // one loop per thread
    EventLoop* ownerLoop()
    {
        return m_loop;
    }
    void remove();

private:
    static const int careNoneEvent;
    static const int careReadEvent;
    static const int careWriteEvent;

    EventLoop* m_loop; // 事件循环
    const int m_fd; // Poller监听的对象
    int m_events; // 注册fd感兴趣的事件
    int m_revents; // Poller返回的具体发生的事件
    int m_index;

    std::weak_ptr<void> m_tie;
    bool m_tied;

    // 因为Channel通道里面能够获知fd最终发生的具体的事件revents，所以它负责具体事件的回调操作
    ReadEventCallBack m_readCallBack;
    EventCallBack m_writeCallBack;
    EventCallBack m_closeCallBack;
    EventCallBack m_errorCallBack;

    void update();
    void handleEventWithGuard(Timestamp receiveTime);
};