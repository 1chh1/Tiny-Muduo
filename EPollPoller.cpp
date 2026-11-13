#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* loop):Poller(loop), 
                                          m_epollfd(::epoll_create1(EPOLL_CLOEXEC)), 
                                          m_events(InitEventListSize)
{
    if(m_epollfd < 0)
    {
        LOG_FATAL("epoll_create error : %d", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(m_epollfd);
}

void EPollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index();
    LOG_INFO("func = %s => fd = %d  events = %d  index = %d", __FUNCTION__, channel->fd(), channel->events(), index);

    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            m_channels[fd] = channel;
        }
        
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
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
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }

    channel->set_index(kNew);
}

Timestamp EPollPoller::poll(int timeout, ChannelList* activeChannels)
{
    LOG_DEBUG("func = %s => fd total count : %lu", __FUNCTION__, m_channels.size());

    int numEvents = ::epoll_wait(m_epollfd, &*m_events.begin(), static_cast<int>(m_events.size()), timeout);
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
        if(op == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error : %d", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error : %d", errno);
        }
    }
}