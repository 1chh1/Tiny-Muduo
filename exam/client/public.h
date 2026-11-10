#ifndef CTCPCLIENT_H
#define CTCPCLIENT_H

#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

class ctcpclient{
private:
    int m_clientfd;
    string m_ip;
    unsigned short m_port;

public:
    ctcpclient();

    bool connect(const string&,unsigned short); //连接

    bool send(const string&); //发送

    bool recv(string &,const size_t); //接收

    bool close();

    ~ctcpclient();

    ctcpclient(const ctcpclient&) = delete;

    ctcpclient& operator=(const ctcpclient&) = delete;
};

#endif