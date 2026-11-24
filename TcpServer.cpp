#include "TcpServer.h"
#include <string.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s : %s : %d  mainloop is null !", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option op):m_loop(CheckLoopNotNull(loop)), 
                                                                                                            m_IpPort(listenAddr.toIPPort()), 
                                                                                                            m_name(nameArg), 
                                                                                                            m_acceptor(new Acceptor(loop, listenAddr, op == kReusePort)), 
                                                                                                            m_threadPool(new EventLoopThreadPool(loop, m_name)), 
                                                                                                            m_connectionCallBack(), 
                                                                                                            m_messageCallBack(), 
                                                                                                            m_nextConnId(1), 
                                                                                                            m_started(0)                                                                                                               
{
    // 当有用户连接时，会执行 TcpServer::newConnection 回调
    m_acceptor->setNewConnectionCallBack(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto& it : m_connections)
    {
        TcpConnectionPtr conn(it.second);
        it.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThread)
{
    m_threadPool->setThreadNum(numThread);
}

void TcpServer::start()
{
    if(m_started.exchange(1) == 0) // 防止一个 TcpServer 对象被 start 多次
    {
        m_threadPool->start(m_threadInitCallBack); // 启动底层的线程池
        m_loop->runInLoop(std::bind(&Acceptor::listen, m_acceptor.get()));
    }
}

// 有一个新的客户端的连接，acceptor 会执行这个回调
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    // 轮询算法选择一个 subloop 来管理 channel
    EventLoop* ioloop = m_threadPool->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", m_IpPort.c_str(), m_nextConnId);
    m_nextConnId++;
    std::string connName = m_name + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s", m_name.c_str(), connName.c_str(), peerAddr.toIPPort().c_str());

    // 通过 sockfd 获取其绑定的本机的 IP 和端口信息
    sockaddr_in tlocal;
    socklen_t addrlen = sizeof(tlocal);
    memset(&tlocal, 0, addrlen);
    if(::getsockname(sockfd, (sockaddr*)&tlocal, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr from %s", __FUNCTION__);
    }
    InetAddress localAddr(tlocal);

    // 根据连接成功的 sockfd ，创建 TcpConnection 连接对象
    TcpConnectionPtr conn(new TcpConnection(ioloop, connName, sockfd, localAddr, peerAddr));

    m_connections[connName] = conn;

    // 下面的回调都是用户设置给 TcpServer => TcpConnection => Channel => Poller => notify channel 调用回调
    conn->setConnectionCallBack(m_connectionCallBack);
    conn->setMessageCallBack(m_messageCallBack);
    conn->setWriteCompleteCallBack(m_writeCompleteCallBack);

    // 设置了如何关闭连接的回调
    conn->setCloseCallBack(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 直接调用 TcpConnection::connectEstablished
    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    m_loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s", m_name.c_str(), conn->name().c_str());

    size_t n = m_connections.erase(conn->name());

    EventLoop* ioloop = conn->getLoop();
    ioloop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}