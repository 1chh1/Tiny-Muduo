#pragma once

// 用户使用库的接口模块
#include "Acceptor.h"
#include "Buffer.h"
#include "CallBack.h"
#include "Channel.h"
#include "CurrentThread.h"
#include "EPollPoller.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Logger.h"
#include "noncopyable.h"
#include "Poller.h"
#include "Socket.h"
#include "TcpConnection.h"
#include "Thread.h"
#include "Timestamp.h"
#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

// 对外的服务器编程使用的类接口
class TcpServer : noncopyable{
public:
    using ThreadInitCallBack = std::function<void(EventLoop*)>;

    enum Option{
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option op = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallBack(const ThreadInitCallBack& cb) { m_threadInitCallBack = cb; }
    void setConnectionCallBack(const ConnectionCallBack& cb) { m_connectionCallBack = cb; }
    void setMessageCallBack(const MessageCallBack& cb) { m_messageCallBack = cb; }
    void setWriteCompleteCallBack(const WriteCompleteCallBack& cb) { m_writeCompleteCallBack = cb; }

    // 设置底层 subloop 的个数
    void setThreadNum(int numThread);

    // 开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* m_loop; // baseloop

    const std::string m_IpPort;
    const std::string m_name;

    std::unique_ptr<Acceptor> m_acceptor;

    std::unique_ptr<EventLoopThreadPool> m_threadPool;

    ConnectionCallBack m_connectionCallBack; // 有新连接时的回调
    MessageCallBack m_messageCallBack; // 有消息读写时的回调
    WriteCompleteCallBack m_writeCompleteCallBack; // 消息发送完成以后的回调

    ThreadInitCallBack m_threadInitCallBack; // loop 线程初始化时的回调
    std::atomic_int m_started;

    int m_nextConnId;
    ConnectionMap m_connections; // 保存所有的连接
};