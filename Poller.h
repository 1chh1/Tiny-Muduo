#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// 多路事件分发器的核心I/O复用模块
class Poller : noncopyable{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);

    virtual ~Poller() = default;

    // 给所有I/O复用保留统一的接口
    virtual Timestamp poll(int timeout, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    // 判断参数 Channel 是否在当前 Poller 当中
    bool hasChannel(Channel* channel) const;

    // EventLoop 可以通过该接口获取默认的I/O复用具体实现
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    // map 的 key : sockfd，value : sockfd 所处的 Channel 通道类型
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap m_channels;

private:
    EventLoop* m_ownerloop; //定义 Poller 所属的事件循环 EventLoop
};