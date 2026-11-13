#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>

// 记录 Channel 在 epoll 中的三种状态
const int kNew = -1; // 全新，尚未添加到 epoll 中
const int kAdded = 1; // 已添加，正在 epoll 中受监控
const int kDeleted = 2; // 已删除，之前加过但后来被移出了 epoll，但 Channel 对象还活着

EPollPoller::EPollPoller(EventLoop* loop):Poller(loop), 
                                          m_epollfd(::epoll_create1(EPOLL_CLOEXEC)), // 创建 epoll 实例，并设置 fork 时自动关闭
                                          m_events(InitEventListSize)
{
    if(m_epollfd < 0)
    {
        LOG_FATAL("epoll_create error : %d", errno);
    }
}

EPollPoller::~EPollPoller()
{
    // 关闭 epoll 句柄，内核会自动清理红黑树和就绪链表中的所有节点
    ::close(m_epollfd);
}

// 更新/注册 Channel
void EPollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index(); // 获取该 Channel 当前在 epoll 中的状态
    LOG_INFO("func = %s => fd = %d  events = %d  index = %d", __FUNCTION__, channel->fd(), channel->events(), index);

    // 1. 如果是全新创建，或者之前被删除过的，需要重新添加
    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            m_channels[fd] = channel; // 如果是全新的，先把它存进自己的哈希表
        }
        
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    // 2. 如果已经在 epoll 中，根据事件的变化决定是删除还是修改
    else
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    m_channels.erase(fd);

    LOG_INFO("func = %s => fd = %d", __FUNCTION__, channel->fd());

    int index = channel->index();
    if(index == kAdded) // 如果它还在 epoll 里，从内核中移除
    {
        update(EPOLL_CTL_DEL, channel);
    }

    channel->set_index(kNew);
}

// 事件循环监听
Timestamp EPollPoller::poll(int timeout, ChannelList* activeChannels)
{
    LOG_DEBUG("func = %s => fd total count : %lu", __FUNCTION__, m_channels.size());

    int numEvents = ::epoll_wait(m_epollfd, &*m_events.begin(), static_cast<int>(m_events.size()), timeout);
    // 保存错误码，因为后面调用 LOG 可能会覆盖全局 errno
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0)
    {
        LOG_DEBUG("%d events happened", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == m_events.size())
        {
            m_events.resize(m_events.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("func = %s => EPollPoller::poll() error", __FUNCTION__);
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for(int i = 0;i < numEvents;++i)
    {
        // 直接通过 data.ptr 强转回 Channel* 避免了通过 fd 查找哈希表
        Channel* channel = static_cast<Channel*>(m_events[i].data.ptr);
        channel->set_revents(m_events[i].events);
        activeChannels->push_back(channel);
    }
}

void EPollPoller::update(int op, Channel* channel)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();

    if(::epoll_ctl(m_epollfd, op, fd, &event) < 0)
    {
        // 删除失败，通常是 fd 已经关了，只记录错误，程序继续跑
        if(op == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error : %d", errno);
        }
        // 添加/修改失败，通常是系统资源耗尽，属于致命错误，程序退出
        else
        {
            LOG_FATAL("epoll_ctl add/mod error : %d", errno);
        }
    }
}