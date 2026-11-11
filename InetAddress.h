#pragma once

#include <netinet/in.h>
#include <string>
#include <stdint.h>
#include <arpa/inet.h>

// 封装socket地址类型
class InetAddress{
private:
    sockaddr_in m_addr;

public:
    explicit InetAddress(uint16_t port, std::string ip = "127.0.0.1");

    explicit InetAddress(const sockaddr_in& addr):m_addr(addr) {}

    std::string toIP() const;
    uint16_t toPort() const;
    std::string toIPPort() const;

    const sockaddr_in* getSockAddr() const {return &m_addr;}
};