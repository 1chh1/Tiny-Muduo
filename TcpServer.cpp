#include "TcpServer.h"

EventLoop* CheckLoopNotNull(EventLoop* loop)
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

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{

}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{

}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{

}