#include "Buffer.h"
#include <algorithm>
#include <errno.h>
#include <sys/uio.h>

void Buffer::retrieve(size_t len)
{
    if(len < readableBytes())
    {
        m_readindex += len;
    }
    else
    {
        retrieveAll();
    }
}

void Buffer::retrieveAll()
{
    m_readindex = kCheapPrepend;
    m_writeindex = kCheapPrepend;
}

std::string Buffer::retrieveAsString(size_t len)
{
    std::string res(peek(), len);
    retrieve(len); // 上一句把缓冲区中可读的数据读取出来，这一句是对缓冲区进行复位操作
    return res;
}

void Buffer::ensureWriteableBytes(size_t len)
{
    if(writeableBytes() < len)
    {
        makeSpace(len); // 扩容函数
    }
}

void Buffer::makeSpace(size_t len)
{
    if(writeableBytes() + prependableBytes() - kCheapPrepend < len)
    {
        m_buf.resize(m_writeindex + len);
    }
    else
    {
        size_t readable = readableBytes();
        std::copy(begin() + m_readindex, begin() + m_writeindex, begin() + kCheapPrepend);
        m_readindex = kCheapPrepend;
        m_writeindex = m_readindex + readable;
    }
}

void Buffer::append(const char* data, size_t len)
{
    ensureWriteableBytes(len);
    std::copy(data, data + len, beginWrite());
    m_writeindex += len;
}

ssize_t Buffer::readfd(int fd, int* savederrno)
{
    char extrabuf[65536] = {0}; // 64K
    struct iovec vec[2];
    const size_t writeable = writeableBytes();
    vec[0].iov_base = begin() + m_writeindex;
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writeable < sizeof(extrabuf)) ? 2 : 1;
    const size_t n = ::readv(fd, vec, iovcnt);
    if(n < 0)
    {
        *savederrno = errno;
    }
    else if(n <= writeable)
    {
        m_writeindex += n;
    }
    else
    {
        m_writeindex = m_buf.size();
        append(extrabuf, n - writeable);
    }

    return n;
}