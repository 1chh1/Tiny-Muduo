#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    ::close(m_sockfd);
}

void Socket::bindAddress(const InetAddress& LocalAddr)
{
    int check = ::bind(m_sockfd, (sockaddr*)LocalAddr.getSockAddr(), sizeof(sockaddr_in));
    if(check != 0)
    {
        LOG_FATAL("bind sockfd : %d fail", m_sockfd);
    }
}

void Socket::listen()
{
    int check = ::listen(m_sockfd, 1024);
    if(check != 0)
    {
        LOG_FATAL("listen sockfd : %d fail", m_sockfd);
    }
}

int Socket::accept(InetAddress* perrAddr)
{
    sockaddr_in addr;
    socklen_t len;
    memset(&addr, 0, sizeof(addr));
    int connfd = ::accept(m_sockfd, (sockaddr*)&addr, &len);
    if(connfd >= 0)
    {
        perrAddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    int check = ::shutdown(m_sockfd, SHUT_WR);
    if(check < 0)
    {
        LOG_ERROR("Socket::shutdownWrite() sockfd : %d error", m_sockfd);
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}
