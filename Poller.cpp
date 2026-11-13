#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop):m_ownerloop(loop) {}

bool Poller::hasChannel(Channel* channel) const
{
    ChannelMap::const_iterator it = m_channels.find(channel->fd());
    if(it != m_channels.end() && it->second == channel)
    {
        return true;
    }
    else
    {
        return false;
    }
}
