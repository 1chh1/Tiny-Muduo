#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>
#include <errno.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s : %s : %d  mainloop is null !", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr):m_loop(CheckLoopNotNull(loop)), 
                                                                                                                                                m_name(nameArg), 
                                                                                                                                                m_status(kConnecting), 
                                                                                                                                                m_reading(true), 
                                                                                                                                                m_socket(new Socket(sockfd)), 
                                                                                                                                                m_channel(new Channel(loop, sockfd)), 
                                                                                                                                                m_localAddr(localAddr), 
                                                                                                                                                m_peerAddr(peerAddr), 
                                                                                                                                                m_highWaterMark(64 * 1024 * 1024)
{
    // 给 channel 设置相应的回调函数，poller 给 channel 通知感兴趣的事件发生了，channel会回调相应的操作函数
    m_channel->setReadCallBack(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    m_channel->setWriteCallBack(std::bind(&TcpConnection::handleWrite, this));
    m_channel->setCloseCallBack(std::bind(&TcpConnection::handleClose, this));
    m_channel->setErrorCallBack(std::bind(&TcpConnection::handleClose, this));
    LOG_INFO("TcpConnection : %s at fd %d", m_name.c_str(), sockfd);
    m_socket->setKeepAlive(true);
}

// 发送数据
void TcpConnection::send(const void* message, int len)
{

}

// 关闭连接
void TcpConnection::shutdown()
{

}

// 连接建立
void TcpConnection::connectEstablished()
{

}

// 连接销毁
void TcpConnection::connectDestroyed()
{

}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    ssize_t n = m_inputBuffer.readfd(m_channel->fd(), &saveErrno);
    if(n > 0)
    {
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调
        m_messageCallBack(shared_from_this(), &m_inputBuffer, receiveTime);
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead fail from %s", __FUNCTION__);
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(m_channel->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = m_outputBuffer.writefd(m_channel->fd(), &savedErrno);
        if(n > 0)
        {
            m_outputBuffer.retrieve(n);
            if(m_outputBuffer.readableBytes() == 0)
            {
                m_channel->disableWriting();
                if(m_writeCompleteCallBack)
                {
                    // 唤醒 m_loop 对应的 thread 线程，执行回调
                    m_loop->queueInLoop(std::bind(m_writeCompleteCallBack, shared_from_this()));
                }
                if(m_status == kDisconnected)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite fail from %s", __FUNCTION__);
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd = %d is down, no more writing", m_channel->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd = %d  status = %d", m_channel->fd(), static_cast<int>(m_status));
    setStatus(kDisconnected);
    m_channel->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    m_connectionCallBack(connPtr); // 执行连接关闭的回调
    m_closeCallBack(connPtr); // 关闭连接的回调
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if(::getsockopt(m_channel->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name : %s - SO_ERROR : %d", m_name.c_str(), err);
}

void TcpConnection::sendInLoop(const void* message, size_t len)
{

}

void TcpConnection::shutdownInLoop()
{

}