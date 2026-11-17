#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装 socketfd
class Socket : noncopyable{
public:
    explicit Socket(int sockfd):m_sockfd(sockfd) {}

    ~Socket();

    int fd() const { return m_sockfd; }
    void bindAddress(const InetAddress& LocalAddr);
    void listen();
    int accept(InetAddress* perrAddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int m_sockfd;
};