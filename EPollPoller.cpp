#include "EPollPoller.h"
#include "Logger.h"
#include <errno.h>
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
        LOG_FATAL("epoll_creat error : %d", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(m_epollfd);
}