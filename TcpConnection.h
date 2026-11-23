#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "CallBack.h"
#include "Buffer.h"
#include "Timestamp.h"
#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

//TcpServer => Acceptor => 有一个新用户连接，通过 accept 函数拿到 connfd => TcpConnection 设置回调 => Channel => Poller => Channel 的回调操作
class TcpConnection : noncopyable, 
                      public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, const std::string nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection() = default;

    EventLoop* getLoop() const { return m_loop; }
    const std::string& name() const { return m_name; }
    const InetAddress& localAddress() const { return m_localAddr; }
    const InetAddress& peerAddress() const { return m_peerAddr; }

    bool connected() const { return m_status == kConnected; }
    bool disconnected() const { return m_status == kDisconnected; }

    // 发送数据
    void send(const void* message, int len);
    // 关闭连接
    void shutdown();

    void setConnectionCallBack(const ConnectionCallBack& cb) { m_connectionCallBack = cb; }
    void setMessageCallBack(const MessageCallBack& cb) { m_messageCallBack = cb; }
    void setWriteCompleteCallBack(const WriteCompleteCallBack& cb) { m_writeCompleteCallBack = cb; }
    void setCloseCallBack(const CloseCallBack& cb) { m_closeCallBack = cb; }
    void setHighWaterMarkCallBack(const HighWaterMarkCallBack& cb, size_t highWaterMark)
    {
        m_highWaterMarkCallBack = cb;
        m_highWaterMark = highWaterMark;
    }

    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();

private:
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    enum Status{
        kDisconnected, // 已断开
        kConnecting, // 正在连接
        kConnected, // 已连接
        kDisconnecting // 正在断开连接
    };
    void setStatus(Status status) { m_status = status; }

    EventLoop* m_loop; // 这里绝对不是 baseloop
    const std::string m_name;
    std::atomic_int m_status;
    bool m_reading;

    std::unique_ptr<Socket> m_socket;
    std::unique_ptr<Channel> m_channel;

    const InetAddress m_localAddr;
    const InetAddress m_peerAddr;

    ConnectionCallBack m_connectionCallBack; // 有新连接时的回调
    MessageCallBack m_messageCallBack; // 有消息读写时的回调
    WriteCompleteCallBack m_writeCompleteCallBack; // 消息发送完成以后的回调
    CloseCallBack m_closeCallBack;
    HighWaterMarkCallBack m_highWaterMarkCallBack;
    size_t m_highWaterMark;

    Buffer m_inputBuffer;
    Buffer m_outputBuffer;
};