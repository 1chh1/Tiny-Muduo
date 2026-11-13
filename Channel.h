#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>

class EventLoop;

/*
Channel 理解为通道，封装了 sockfd 和其感兴趣的 event ，如 EPOLLIN、EPOLLOUT 事件
还绑定了 Poller 监听返回的具体事件
*/
class Channel : noncopyable{
public:
    using EventCallBack = std::function<void()>; // 事件的回调
    using ReadEventCallBack = std::function<void(Timestamp)>; // 只读事件的回调

    Channel(EventLoop* loop, int fd); // loop : 该 Channel 所属的事件循环 | fd : 该 Channel 绑定的文件描述符
    ~Channel() {}

    // fd 得到 Poller 通知以后，处理事件的，调用相应的回调方法
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

    // 防止当 Channel 被手动 remove 掉，Channel 还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    // 状态的获取与设置
    int fd() const
    {
        return m_fd;
    }
    int events() const
    {
        return m_events;
    }
    void set_revents(int rev)
    {
        m_revents = rev;
    }

    // 设置 fd 相应的事件状态
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

    // 返回 fd 当前的事件状态
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

    // 获取与设置当前 Channel 在 Poller 里的状态标识
    int index()
    {
        return m_index;
    }
    void set_index(int idx)
    {
        m_index = idx;
    }

    // 获取该 Channel 所属的事件循环
    EventLoop* ownerloop()
    {
        return m_loop;
    }

    // 将当前 Channel 从底层的 Poller 中移除注销
    void remove();

private:
    static const int careNoneEvent;
    static const int careReadEvent;
    static const int careWriteEvent;

    EventLoop* m_loop; // 所属的事件循环
    const int m_fd; // Poller 监听的对象
    int m_events; // 注册 fd 感兴趣的事件
    int m_revents; // Poller 返回的具体发生的事件
    int m_index; // Channel 在 Poller 里的状态标识

    std::weak_ptr<void> m_tie; // 观察上层对象是否存活
    bool m_tied; // 存活状态标记

    // 因为 Channel 通道里面能够获知fd最终发生的具体的事件 revents ，所以它负责具体事件的回调操作
    ReadEventCallBack m_readCallBack;
    EventCallBack m_writeCallBack;
    EventCallBack m_closeCallBack;
    EventCallBack m_errorCallBack;

    void update(); // 通知底层的 Poller 更新监听的事件列表
    void handleEventWithGuard(Timestamp receiveTime); // 事件分发执行函数
};