#include "public.h"

ctcpclient::ctcpclient():m_clientfd(-1) {}

bool ctcpclient::connect(const string& in_ip,unsigned short in_port)
{
    if(m_clientfd != -1)
    {
        return false; //如果socket已连接，直接返回失败
    }

    m_ip = in_ip;
    m_port = in_port; //把服务端的IP和端口保存到成员变量中

    //第一步，创建客户端的socket
    m_clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(m_clientfd == -1)
    {
        perror("socket");
        return false;
    }

    //第二步，向服务器发起连接请求
    struct sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(m_port); //指定服务端的通信端口
    struct hostent * h; //用于存放服务端IP的结构体
    if((h = gethostbyname(m_ip.c_str())) == 0) //转换成结构体（支持域名/主机名/字符串格式的IP）
    {
        cout << "gethostbyname false" << endl;
        ::close(m_clientfd);
        m_clientfd = -1;
        return false;
    }
    memcpy(&servaddr.sin_addr,h->h_addr,h->h_length); //指定服务端的IP地址
    //地址 192.168.64.129 端口 5005
    if(::connect(m_clientfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) != 0) //向服务端发起连接请求
    {
        perror("connect");
        ::close(m_clientfd);
        m_clientfd = -1;
        return false;
    }

    return true;
}

bool ctcpclient::send(const string& buf)
{
    if(m_clientfd == -1)
    {
        return false;
    }
    //向服务端发送请求报文
    if((::send(m_clientfd,buf.data(),buf.size(),0)) <= 0)
    {
        return false;
    }

    return true;
}

//接收服务端的报文，成功返回true，失败返回false
//buf存放接收到的报文的内容，maxlen是本次接收报文的最大长度
bool ctcpclient::recv(string& buf,size_t maxlen)
{
    //如果直接操作string对象的内存，必须保证：
    //1.不能越界
    //2.操作后手动设置数据的大小
    buf.clear(); //清空容器
    buf.resize(maxlen); //设置容器的大小
    int readn = ::recv(m_clientfd,&buf[0],buf.size(),0); //直接操作buf内存
    if(readn <= 0)
    {
        buf.clear();
        return false;
    }
    buf.resize(readn); //重置buf的实际大小

    return true;
}

bool ctcpclient::close()
{
    if(m_clientfd == -1)
    {
        return false;
    }

    ::close(m_clientfd);
    m_clientfd = -1;
    return true;
}

ctcpclient::~ctcpclient()
{
    close();
}