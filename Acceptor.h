#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable{
public:
    using NewConnectionCallBack = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallBack(const NewConnectionCallBack& cb)
    {
        m_newConnectionCallBack = cb;
    }

    bool listening() const { return m_listening; }
    void listen();

private:
    void handleRead();

    EventLoop* m_loop; // Acceptor 用的就是用户定义的 baseloop ，也就是 mainloop
    Socket m_acceptSocket;
    Channel m_acceptChannel;
    NewConnectionCallBack m_newConnectionCallBack;
    bool m_listening;
};