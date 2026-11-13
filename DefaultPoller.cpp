#include "Poller.h"
#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 生成 poll 的实例，因为此版本仅实现 epoll ，所以此处仅作模块功能的演示，返回空指针
    }
    else
    {
        return nullptr; // 生成 epoll 的实例
    }
}