#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sockfd < 0)
    {
        LOG_FATAL("%s : %s : %d listen socket create error : %d", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport):m_loop(loop), 
                                                                                   m_acceptSocket(createNonblocking()), 
                                                                                   m_acceptChannel(loop, m_acceptSocket.fd()), 
                                                                                   m_listening(false)
{
    m_acceptSocket.setReuseAddr(true);
    m_acceptSocket.setReusePort(true);
    m_acceptSocket.bindAddress(listenAddr);
    m_acceptChannel.setReadCallBack(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    m_acceptChannel.disableAll();
    m_acceptChannel.remove();
}

void Acceptor::listen()
{
    m_listening = true;
    m_acceptSocket.listen();
    m_acceptChannel.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = m_acceptSocket.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(m_newConnectionCallBack)
        {
            m_newConnectionCallBack(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s : %s : %d accept error : %d", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE)
        {
            LOG_ERROR("%s : %s : %d sockfd reach limit", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}