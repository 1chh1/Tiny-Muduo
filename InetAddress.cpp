#include "InetAddress.h"
#include <string.h>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIP() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof(buf));
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(m_addr.sin_port);
}

std::string InetAddress::toIPPort() const
{
    char buf[64];
    ::inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(m_addr.sin_port);
    snprintf(buf + end, sizeof(buf - end), ":%u", port);
    return buf;
}
