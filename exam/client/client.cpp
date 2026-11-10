//此程序用于演示封装socket客户端
#include "public.h"

int main(int argc,char *argv[])
{
    if(argc != 3)
    {
        cout << "using : ./client 服务端的IP 服务端的端口" << endl;
        cout << "Example : ./client 192.168.64.131 6000" << endl;
        return -1;
    }

    ctcpclient tcpclient;
    if(tcpclient.connect(argv[1],atoi(argv[2])) == false)
    {
        perror("connect");
        return -1;
    }

    //第三步，与服务端通信，发送一个请求报文后等待回复，然后再发下一个请求报文
    string buf;
    while(true) //循环三次，将与服务端进行三次通讯
    {
        buf.clear();
        if(!getline(cin,buf))
        {
            break;
        }
        if(buf == "stop")
        {
            break;
        }
        //向服务端发送请求报文
        if(tcpclient.send(buf) == false)
        {
            perror("send");
            break;
        }
        //接受服务端的回应报文，如果服务端没有发送回应报文，recv()函数将阻塞等待
        if((tcpclient.recv(buf,1024)) == false)
        {
            perror("recv");
            break;
        }
        cout << buf << endl;
    }

    //第四步，关闭socket，释放资源，放在了析构函数中
}