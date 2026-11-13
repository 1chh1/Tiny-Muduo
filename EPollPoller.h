#pragma once

#include "Poller.h"
#include <vector>
#include <sys/epoll.h>

class EPollPoller : public Poller{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    // 重写基类 Poller 的抽象方法
    Timestamp poll(int timeout, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    static const int InitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    // 更新 Channel 通道
    void update(int op, Channel* Channel);

    using EventList = std::vector<epoll_event>;

    int m_epollfd;
    EventList m_events;
};