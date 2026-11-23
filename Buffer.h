#pragma once

#include "noncopyable.h"
#include <vector>
#include <string>

// 网络库底层的缓冲区
class Buffer : public noncopyable{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSie = kInitialSize):m_buf(kCheapPrepend + initialSie), 
                                                      m_readindex(kCheapPrepend), 
                                                      m_writeindex(kCheapPrepend) {}

    size_t readableBytes() const { return m_writeindex - m_readindex; }

    size_t writeableBytes() const { return m_buf.size() - m_writeindex; }

    size_t prependableBytes() const { return m_readindex; }

    const char* peek() const { return begin() + m_readindex; } // 返回缓冲区中可读数据的起始地址
    
    void retrieve(size_t len);

    void retrieveAll();

    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); } // 把 onMessage 函数上报的 Buffer 数据，转成 string 类型的数据返回

    std::string retrieveAsString(size_t len);

    void ensureWriteableBytes(size_t len);

    void append(const char* data, size_t len);

    char* beginWrite() { return begin() + m_writeindex; }

    const char* beginWrite() const { return begin() + m_writeindex; }

    // 从 fd 上读取数据
    ssize_t readfd(int fd, int* savederrno);
    // 通过 fd 发送数据
    ssize_t writefd(int fd, int* savederrno);

private:
    char* begin()
    {
        return &*m_buf.begin();
    }
    const char* begin() const
    {
        return &*m_buf.begin();
    }
    void makeSpace(size_t len);

    std::vector<char> m_buf;
    size_t m_readindex;
    size_t m_writeindex;
};