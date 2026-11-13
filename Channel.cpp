#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include <sys/epoll.h>

const int Channel::careNoneEvent = 0;
const int Channel::careReadEvent = EPOLLIN | EPOLLPRI; // EPOLLPRI 表示带外紧急数据可读
const int Channel::careWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd):m_loop(loop), 
                                          m_fd(fd), 
                                          m_events(0), 
                                          m_revents(0), 
                                          m_index(-1), // 在 Poller 中的状态标记，-1 通常表示尚未添加到 epoll 中
                                          m_tied(false) {} // 初始未开启生命周期绑定保护

void Channel::tie(const std::shared_ptr<void>& obj)
{
    m_tie = obj;
    m_tied = true; // 标记已绑定保护，执行回调前必须先尝试提升为强指针
}

// 当改变 Channel 所表示的 fd 的 events 事件后，update 负责在 Poller 里面更改 fd 相应的事件 epoll_ctl
void Channel::update()
{
    // 通过 Channel 所属的 EventLoop 调用 Poller 的相应方法，注册 fd 的 events 事件
    m_loop->updateChannel(this);
}

// 在 Channel 所属的 EventLoop 中，把当前的 Channel 删除掉
void Channel::remove()
{
    m_loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if(m_tied)
    {
        guard = m_tie.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据 Poller 通知的 Channel 发生的具体事件，由 Channel 负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents : %d", m_revents);

    if((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN))
    {
        if(m_closeCallBack)
        {
            m_closeCallBack();
        }
    }

    if(m_revents & EPOLLERR)
    {
        if(m_errorCallBack)
        {
            m_errorCallBack();
        }
    }

    if(m_revents & EPOLLIN)
    {
        if(m_readCallBack)
        {
            m_readCallBack(receiveTime);
        }
    }

    if(m_revents & EPOLLOUT)
    {
        if(m_writeCallBack)
        {
            m_writeCallBack();
        }
    }
}