#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>
#include <errno.h>
#include <unistd.h>

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

TcpConnection::~TcpConnection() = default;

// 发送数据
void TcpConnection::send(const std::string& buf)
{
    if(m_status == kConnected)
    {
        if(m_loop->isInLoopThread())
        {
            sendInLoop(buf.data(), buf.size());
        }
        else
        {
            TcpConnectionPtr self = shared_from_this();
            std::string cbuf(buf);
            m_loop->runInLoop([self, this, cbuf]{ sendInLoop(cbuf.data(), cbuf.size()); });
        }
    }
}

// 关闭连接
void TcpConnection::shutdown()
{
    if(m_status == kConnected)
    {
        setStatus(kDisconnecting);
        m_loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

// 连接建立
void TcpConnection::connectEstablished()
{
    setStatus(kConnected);
    m_channel->tie(shared_from_this());
    m_channel->enableReading();

    // 新连接建立，执行回调
    m_connectionCallBack(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
    if(m_status == kConnected)
    {
        setStatus(kDisconnected);
        m_channel->disableAll();

        m_connectionCallBack(shared_from_this());
    }
    m_channel->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    if(m_status != kConnected)
    {
        return;
    }

    int saveErrno = 0;
    ssize_t n = m_inputBuffer.readfd(m_channel->fd(), &saveErrno);
    if(n > 0)
    {
        if(m_messageCallBack)
        {
            // 已建立连接的用户，有可读事件发生了，调用用户传入的回调
            m_messageCallBack(shared_from_this(), &m_inputBuffer, receiveTime);
        }
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        if(errno == EAGAIN || errno == EINTR)
        {
            return;
        }
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
                if(m_status == kDisconnecting)
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
    if(m_connectionCallBack)
    {
        m_connectionCallBack(connPtr); // 执行连接关闭的回调
    }
    if(m_closeCallBack)
    {
        m_closeCallBack(connPtr); // 关闭连接的回调
    }
}

void TcpConnection::handleError()
{
    int optval = 0;
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

// 发送数据，应用写的快，而内核发送数据慢，需要把待发送数据写入缓冲区，而且设置了水位回调
void TcpConnection::sendInLoop(const void* message, size_t len)
{
    ssize_t nwrite = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过该 connection 的 shutdown ，不能再进行发送了
    if(m_status == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing from %s", __FUNCTION__);
        return;
    }

    // 表示 channel 第一次开始写数据，而且缓冲区没有待发送数据
    if(!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0)
    {
        nwrite = ::write(m_channel->fd(), message, len);
        if(nwrite >= 0)
        {
            remaining = len - nwrite;
            if(remaining == 0 && m_writeCompleteCallBack)
            {
                // 既然在这里数据全部发送完成，就不用再给 channel 设置 EPOLLOUT 事件了
                m_loop->queueInLoop(std::bind(m_writeCompleteCallBack, shared_from_this()));
            }
        }
        else
        {
            nwrite = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop error from %s", __FUNCTION__);
                if(errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    // 当前这一次 write 并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，然后给 channel 注册 EPOLLOUT 事件，poller 发现发送缓冲区有空间，会通知相应的 channel 调用 handleWrite 回调，也就是调用 TcpConnection::handleWrite 方法，把发送缓冲区的数据全部发送完成
    if(!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的待发送数据的长度
        size_t oldlen = m_outputBuffer.readableBytes();
        if(oldlen + remaining >= m_highWaterMark && oldlen < m_highWaterMark && m_highWaterMarkCallBack)
        {
            m_loop->queueInLoop(std::bind(m_highWaterMarkCallBack, shared_from_this(), oldlen + remaining));
        }
        m_outputBuffer.append((char*)(message) + nwrite, remaining);
        if(!m_channel->isWriting())
        {
            m_channel->enableWriting(); // 这里一定要注册 channel 的写事件，否则 poller 不会给 channel 通知 EPOLLOUT
        }
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!m_channel->isWriting())
    {
        m_socket->shutdownWrite();
    }
}